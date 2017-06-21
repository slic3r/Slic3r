#include "libslic3r.h"
#include "TriangleMesh.hpp"
#include "SlicingAdaptive.hpp"

#ifdef SLIC3R_DEBUG
    #undef NDEBUG
    #define DEBUG
    #define _DEBUG
#endif

/* This constant essentially describes the volumetric error at the surface which is induced
 * by stacking "elliptic" extrusion threads.
 * It is empirically determined by
 * 1. measuring the surface profile of printed parts to find
 * the ratio between layer height and profile height and then
 * 2. computing the geometric difference between the model-surface and the elliptic profile.
 * [Link to detailed description follows]
 */
#define SURFACE_CONST 0.18403

namespace Slic3r
{

void SlicingAdaptive::clear()
{
    m_meshes.clear();
    m_faces.clear();
    m_face_normal_z.clear();
}

std::pair<float, float> face_z_span(const stl_facet *f)
{
    return std::pair<float, float>(
        std::min(std::min(f->vertex[0].z, f->vertex[1].z), f->vertex[2].z),
        std::max(std::max(f->vertex[0].z, f->vertex[1].z), f->vertex[2].z));
}

void SlicingAdaptive::prepare(coordf_t object_size)
{
    this->object_size = object_size;

    // 1) Collect faces of all meshes.
    int nfaces_total = 0;
    for (std::vector<const TriangleMesh*>::const_iterator it_mesh = m_meshes.begin(); it_mesh != m_meshes.end(); ++ it_mesh)
        nfaces_total += (*it_mesh)->stl.stats.number_of_facets;
    m_faces.reserve(nfaces_total);
    for (std::vector<const TriangleMesh*>::const_iterator it_mesh = m_meshes.begin(); it_mesh != m_meshes.end(); ++ it_mesh)
        for (int i = 0; i < (*it_mesh)->stl.stats.number_of_facets; ++ i)
            m_faces.push_back((*it_mesh)->stl.facet_start + i);

    // 2) Sort faces lexicographically by their Z span.
    std::sort(m_faces.begin(), m_faces.end(), [](const stl_facet *f1, const stl_facet *f2) {
        std::pair<float, float> span1 = face_z_span(f1);
        std::pair<float, float> span2 = face_z_span(f2);
        return span1 < span2;
    });

    // 3) Generate Z components of the facet normals.
    m_face_normal_z.assign(m_faces.size(), 0.f);
    for (size_t iface = 0; iface < m_faces.size(); ++ iface)
        m_face_normal_z[iface] = m_faces[iface]->normal.z;

    // 4) Reset current facet pointer
    this->current_facet = 0;
}

float SlicingAdaptive::next_layer_height(coordf_t z, coordf_t quality_factor, coordf_t min_layer_height, coordf_t max_layer_height)
{
    float height = max_layer_height;
    // factor must be between 0-1, 0 is highest quality, 1 highest print speed.

    // factor must be between 0-1, 0 is highest quality, 1 highest print speed.
    // Invert the slider scale (100% should represent a very high quality for the user)
    quality_factor = std::max(0.f, std::min<float>(1.f, 1 - quality_factor/100.f));

    float delta_min = SURFACE_CONST * min_layer_height;
    float delta_max = SURFACE_CONST * max_layer_height + 0.5 * max_layer_height;
    float scaled_quality_factor = quality_factor * (delta_max - delta_min) + delta_min;

    bool first_hit = false;

    // find all facets intersecting the slice-layer
    int ordered_id = current_facet;
    for (; ordered_id < int(m_faces.size()); ++ ordered_id) {
        std::pair<float, float> zspan = face_z_span(m_faces[ordered_id]);
        // facet's minimum is higher than slice_z -> end loop
        if (zspan.first >= z)
            break;
        // facet's maximum is higher than slice_z -> store the first event for next layer_height call to begin at this point
        if (zspan.second > z) {
            // first event?
            if (! first_hit) {
                first_hit = true;
                current_facet = ordered_id;
            }
            // skip touching facets which could otherwise cause small height values
            if (zspan.second <= z + EPSILON)
                continue;
            // compute height for this facet and store minimum of all heights
            height = std::min(height, this->_layer_height_from_facet(ordered_id, scaled_quality_factor));
        }
    }

    // lower height limit due to printer capabilities
    height = std::max<float>(height, min_layer_height);

    // check for sloped facets inside the determined layer and correct height if necessary
    if (height > min_layer_height) {
        for (; ordered_id < int(m_faces.size()); ++ ordered_id) {
            std::pair<float, float> zspan = face_z_span(m_faces[ordered_id]);
            // facet's minimum is higher than slice_z + height -> end loop
            if (zspan.first >= z + height)
                break;

            // skip touching facets which could otherwise cause small cusp values
            if (zspan.second <= z + EPSILON)
                continue;

            // Compute new height for this facet and check against height.
            float reduced_height = this->_layer_height_from_facet(ordered_id, scaled_quality_factor);

            float z_diff = zspan.first - z;

            if (reduced_height > z_diff) {
                if (reduced_height < height) {
#ifdef DEBUG
                    std::cout << "adaptive layer computation: height is reduced from " << height;
#endif
                    height = reduced_height;
#ifdef DEBUG
                    std::cout << "to " << height << " due to higher facet" << std::endl;
#endif
                }
            } else {
#ifdef DEBUG
                std::cout << "cusp computation, height is reduced from " << height;
#endif
                height = z_diff;
#ifdef DEBUG
                std::cout << "to " << height << " due to z-diff" << std::endl;
#endif
            }
        }
        // lower height limit due to printer capabilities again
        height = std::max(height, float(min_layer_height));
    }

#ifdef DEBUG
    std::cout << "adaptive layer computation, layer-bottom at z:" << z << ", quality_factor:" << quality_factor << ", resulting layer height:" << height << std::endl;
#endif
    return height;
}

// Returns the distance to the next horizontal facet in Z-dir 
// to consider horizontal object features in slice thickness
float SlicingAdaptive::horizontal_facet_distance(coordf_t z, coordf_t max_layer_height)
{
    for (size_t i = 0; i < m_faces.size(); ++ i) {
        std::pair<float, float> zspan = face_z_span(m_faces[i]);
        // facet's minimum is higher than max forward distance -> end loop
        if (zspan.first > z + max_layer_height)
            break;
        // min_z == max_z -> horizontal facet
        if (zspan.first > z && zspan.first == zspan.second)
            return zspan.first - z;
    }

    // objects maximum?
    return (z + max_layer_height > this->object_size) ?
        std::max<float>(this->object_size - z, 0.f) :
        max_layer_height;
}

// for a given facet, compute maximum height within the allowed surface roughness / stairstepping deviation
float SlicingAdaptive::_layer_height_from_facet(int ordered_id, float scaled_quality_factor)
{
    float normal_z = std::abs(m_face_normal_z[ordered_id]);
    float height = scaled_quality_factor/(SURFACE_CONST + normal_z/2);
    return height;
}

}; // namespace Slic3r
