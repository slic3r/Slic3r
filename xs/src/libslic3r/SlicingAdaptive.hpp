#ifndef slic3r_SlicingAdaptive_hpp_
#define slic3r_SlicingAdaptive_hpp_

#include "admesh/stl.h"

namespace Slic3r
{

class TriangleMesh;

class SlicingAdaptive
{
public:
    SlicingAdaptive() {};
    ~SlicingAdaptive() {};
    void clear();
    void add_mesh(const TriangleMesh *mesh) { m_meshes.push_back(mesh); }
    void prepare(coordf_t object_size);
    float next_layer_height(coordf_t z, coordf_t quality_factor, coordf_t min_layer_height, coordf_t max_layer_height);
    float horizontal_facet_distance(coordf_t z, coordf_t max_layer_height);

private:
    float _layer_height_from_facet(int ordered_id, float scaled_quality_factor);

protected:
    // id of the current facet from last iteration
    coordf_t                            object_size;
    int                                 current_facet;
    std::vector<const TriangleMesh*>	m_meshes;
    // Collected faces of all meshes, sorted by raising Z of the bottom most face.
    std::vector<const stl_facet*>		m_faces;
    // Z component of face normals, normalized.
    std::vector<float>					m_face_normal_z;
};

}; // namespace Slic3r

#endif /* slic3r_SlicingAdaptive_hpp_ */
