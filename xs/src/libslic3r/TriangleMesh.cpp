#include "TriangleMesh.hpp"
#include "ClipperUtils.hpp"
#include "Log.hpp"
#include "Geometry.hpp"
#include <cmath>
#include <deque>
#include <queue>
#include <set>
#include <vector>
#include <map>
#include <utility>
#include <algorithm>
#include <math.h>
#include <assert.h>
#include <stdexcept>
#include <boost/version.hpp>
#include <boost/config.hpp>
#include <boost/nowide/convert.hpp>
#if BOOST_VERSION >= 107300
#include <boost/bind/bind.hpp>
#endif

#ifdef SLIC3R_DEBUG
#include "SVG.hpp"
#endif

namespace Slic3r {


#if BOOST_VERSION >= 107300
using boost::placeholders::_1;
#endif

TriangleMesh::TriangleMesh()
    : repaired(false)
{
    stl_initialize(&this->stl);
}

TriangleMesh::TriangleMesh(const Pointf3* points, const Point3* facets, size_t n_facets) 
    : repaired(false)
{
    stl_initialize(&this->stl);
    stl_file &stl = this->stl;
    stl.error = 0;
    stl.stats.type = inmemory;

    // count facets and allocate memory
    stl.stats.number_of_facets = n_facets;
    stl.stats.original_num_facets = stl.stats.number_of_facets;
    stl_allocate(&stl);

    for (int i = 0; i < stl.stats.number_of_facets; i++) {
        stl_facet facet;
        facet.normal.x = 0;
        facet.normal.y = 0;
        facet.normal.z = 0;

        const Pointf3& ref_f1 = points[facets[i].x];
        facet.vertex[0].x = ref_f1.x;
        facet.vertex[0].y = ref_f1.y;
        facet.vertex[0].z = ref_f1.z;

        const Pointf3& ref_f2 = points[facets[i].y];
        facet.vertex[1].x = ref_f2.x;
        facet.vertex[1].y = ref_f2.y;
        facet.vertex[1].z = ref_f2.z;

        const Pointf3& ref_f3 = points[facets[i].z];
        facet.vertex[2].x = ref_f3.x;
        facet.vertex[2].y = ref_f3.y;
        facet.vertex[2].z = ref_f3.z;
        
        facet.extra[0] = 0;
        facet.extra[1] = 0;

        stl.facet_start[i] = facet;
    }
    stl_get_size(&stl);
}

TriangleMesh::TriangleMesh(const TriangleMesh &other)
    : stl(other.stl), repaired(other.repaired)
{
    this->clone(other);
}

TriangleMesh& TriangleMesh::operator= (const TriangleMesh& other)
{
    this->stl = other.stl;
    this->repaired = other.repaired;
    this->clone(other);

    return *this;
}


void TriangleMesh::clone(const TriangleMesh& other) {
    this->stl.heads = NULL;
    this->stl.tail  = NULL;
    this->stl.error = other.stl.error;
    if (other.stl.facet_start != NULL) {
        this->stl.facet_start = (stl_facet*)calloc(other.stl.stats.number_of_facets, sizeof(stl_facet));
        std::copy(other.stl.facet_start, other.stl.facet_start + other.stl.stats.number_of_facets, this->stl.facet_start);
    }
    if (other.stl.neighbors_start != NULL) {
        this->stl.neighbors_start = (stl_neighbors*)calloc(other.stl.stats.number_of_facets, sizeof(stl_neighbors));
        std::copy(other.stl.neighbors_start, other.stl.neighbors_start + other.stl.stats.number_of_facets, this->stl.neighbors_start);
    }
    if (other.stl.v_indices != NULL) {
        this->stl.v_indices = (v_indices_struct*)calloc(other.stl.stats.number_of_facets, sizeof(v_indices_struct));
        std::copy(other.stl.v_indices, other.stl.v_indices + other.stl.stats.number_of_facets, this->stl.v_indices);
    }
    if (other.stl.v_shared != NULL) {
        this->stl.v_shared = (stl_vertex*)calloc(other.stl.stats.shared_vertices, sizeof(stl_vertex));
        std::copy(other.stl.v_shared, other.stl.v_shared + other.stl.stats.shared_vertices, this->stl.v_shared);
    }
}

TriangleMesh::TriangleMesh(TriangleMesh&& other) {
    this->repaired = std::move(other.repaired);
    this->stl = std::move(other.stl);
    stl_initialize(&other.stl);
}

TriangleMesh& TriangleMesh::operator= (TriangleMesh&& other)
{
    this->repaired = std::move(other.repaired);
    this->stl = std::move(other.stl);
    stl_initialize(&other.stl);

    return *this;
}

void
TriangleMesh::swap(TriangleMesh &other)
{
    std::swap(this->stl,      other.stl);
    std::swap(this->repaired, other.repaired);
}

TriangleMesh::~TriangleMesh() {
    stl_close(&this->stl);
}

void
TriangleMesh::ReadSTLFile(const std::string &input_file) {
    #ifdef BOOST_WINDOWS
    stl_open(&stl, boost::nowide::widen(input_file).c_str());
    #else
    stl_open(&stl, input_file.c_str());
    #endif
    if (this->stl.error != 0) throw std::runtime_error("Failed to read STL file");
}

void
TriangleMesh::write_ascii(const std::string &output_file) const
{
    #ifdef BOOST_WINDOWS
    stl_write_ascii(const_cast<stl_file*>(&this->stl), boost::nowide::widen(output_file).c_str(), "");
    #else
    stl_write_ascii(const_cast<stl_file*>(&this->stl), output_file.c_str(), "");
    #endif
}

void
TriangleMesh::write_binary(const std::string &output_file) const
{
    #ifdef BOOST_WINDOWS
    stl_write_binary(const_cast<stl_file*>(&this->stl), boost::nowide::widen(output_file).c_str(), "");
    #else
    stl_write_binary(const_cast<stl_file*>(&this->stl), output_file.c_str(), "");
    #endif
}

void
TriangleMesh::repair() {
    if (this->repaired) return;
    
    // admesh fails when repairing empty meshes
    if (this->stl.stats.number_of_facets == 0) return;
    
    this->check_topology();

    /// Call the stl_repair from admesh rather than reimplementing it ourselves.
    stl_repair(&(this->stl),  // operate on this STL
               true,  // flag: try to fix everything
               true,  // flag: check for perfectly aligned edges
               false, // flag: don't use tolerance
               0.0,    // null tolerance value
               false, // flag: don't increment tolerance
               0.0,   // amount to increment tolerance on each iteration
               true,  // find and try to connect nearby bad facets
               10,    // Perform 10 iterations
               true,  // remove unconnected
               true,  // fill holes
               true,  // fix normal directions
               true,  // fix normal values
               false, // reverse direction of all facets and normals
               0);  // Verbosity
    
    // always calculate the volume and reverse all normals if volume is negative
    (void)this->volume();
    
    // neighbors
    stl_verify_neighbors(&stl);
    
    this->repaired = true;
}

float
TriangleMesh::volume()
{
    if (this->stl.stats.volume == -1) stl_calculate_volume(&this->stl);
    return this->stl.stats.volume;
}

void
TriangleMesh::check_topology()
{
    // checking exact
    stl_check_facets_exact(&stl);
    stl.stats.facets_w_1_bad_edge = (stl.stats.connected_facets_2_edge - stl.stats.connected_facets_3_edge);
    stl.stats.facets_w_2_bad_edge = (stl.stats.connected_facets_1_edge - stl.stats.connected_facets_2_edge);
    stl.stats.facets_w_3_bad_edge = (stl.stats.number_of_facets - stl.stats.connected_facets_1_edge);
    
    // checking nearby
    //int last_edges_fixed = 0;
    float tolerance = stl.stats.shortest_edge;
    float increment = stl.stats.bounding_diameter / 10000.0;
    int iterations = 2;
    if (stl.stats.connected_facets_3_edge < stl.stats.number_of_facets) {
        for (int i = 0; i < iterations; i++) {
            if (stl.stats.connected_facets_3_edge < stl.stats.number_of_facets) {
                //printf("Checking nearby. Tolerance= %f Iteration=%d of %d...", tolerance, i + 1, iterations);
                stl_check_facets_nearby(&stl, tolerance);
                //printf("  Fixed %d edges.\n", stl.stats.edges_fixed - last_edges_fixed);
                //last_edges_fixed = stl.stats.edges_fixed;
                tolerance += increment;
            } else {
                break;
            }
        }
    }
}

bool
TriangleMesh::is_manifold() const
{
    return this->stl.stats.connected_facets_3_edge == this->stl.stats.number_of_facets;
}

void
TriangleMesh::reset_repair_stats() {
    this->stl.stats.degenerate_facets   = 0;
    this->stl.stats.edges_fixed         = 0;
    this->stl.stats.facets_removed      = 0;
    this->stl.stats.facets_added        = 0;
    this->stl.stats.facets_reversed     = 0;
    this->stl.stats.backwards_edges     = 0;
    this->stl.stats.normals_fixed       = 0;
}

bool
TriangleMesh::needed_repair() const
{
    return this->stl.stats.degenerate_facets    > 0
        || this->stl.stats.edges_fixed          > 0
        || this->stl.stats.facets_removed       > 0
        || this->stl.stats.facets_added         > 0
        || this->stl.stats.facets_reversed      > 0
        || this->stl.stats.backwards_edges      > 0;
}

size_t
TriangleMesh::facets_count() const
{
    return this->stl.stats.number_of_facets;
}

void
TriangleMesh::WriteOBJFile(const std::string &output_file) const {
    stl_generate_shared_vertices(const_cast<stl_file*>(&this->stl));
    
    #ifdef BOOST_WINDOWS
    stl_write_obj(const_cast<stl_file*>(&this->stl), boost::nowide::widen(output_file).c_str());
    #else
    stl_write_obj(const_cast<stl_file*>(&this->stl), output_file.c_str());
    #endif
}

void TriangleMesh::scale(float factor)
{
    stl_scale(&(this->stl), factor);
    stl_invalidate_shared_vertices(&this->stl);
}

void TriangleMesh::scale(const Pointf3 &versor)
{
    float fversor[3];
    fversor[0] = versor.x;
    fversor[1] = versor.y;
    fversor[2] = versor.z;
    stl_scale_versor(&this->stl, fversor);
    stl_invalidate_shared_vertices(&this->stl);
}

void TriangleMesh::translate(float x, float y, float z)
{
    stl_translate_relative(&(this->stl), x, y, z);
    stl_invalidate_shared_vertices(&this->stl);
}

void TriangleMesh::translate(Pointf3 vec) {
    this->translate(
        static_cast<float>(vec.x),
        static_cast<float>(vec.y),
        static_cast<float>(vec.z)
    );
}

void TriangleMesh::rotate(float angle, const Axis &axis)
{
    // admesh uses degrees
    angle = Slic3r::Geometry::rad2deg(angle);
    
    if (axis == X) {
        stl_rotate_x(&(this->stl), angle);
    } else if (axis == Y) {
        stl_rotate_y(&(this->stl), angle);
    } else if (axis == Z) {
        stl_rotate_z(&(this->stl), angle);
    }
    stl_invalidate_shared_vertices(&this->stl);
}

void TriangleMesh::rotate_x(float angle)
{
    this->rotate(angle, X);
}

void TriangleMesh::rotate_y(float angle)
{
    this->rotate(angle, Y);
}

void TriangleMesh::rotate_z(float angle)
{
    this->rotate(angle, Z);
}

void TriangleMesh::mirror(const Axis &axis)
{
    if (axis == X) {
        stl_mirror_yz(&this->stl);
    } else if (axis == Y) {
        stl_mirror_xz(&this->stl);
    } else if (axis == Z) {
        stl_mirror_xy(&this->stl);
    }
    stl_invalidate_shared_vertices(&this->stl);
}

void TriangleMesh::mirror_x()
{
    this->mirror(X);
}

void TriangleMesh::mirror_y()
{
    this->mirror(Y);
}

void TriangleMesh::mirror_z()
{
    this->mirror(Z);
}

void TriangleMesh::align_to_origin()
{
    this->translate(
        -(this->stl.stats.min.x),
        -(this->stl.stats.min.y),
        -(this->stl.stats.min.z)
    );
}

void TriangleMesh::center_around_origin()
{
    this->align_to_origin();
    this->translate(
        -(this->stl.stats.size.x/2),
        -(this->stl.stats.size.y/2),
        -(this->stl.stats.size.z/2)
    );
}

void TriangleMesh::rotate(double angle, Point* center)
{
    this->rotate(angle, *center);
}
void TriangleMesh::rotate(double angle, const Point& center)
{
    this->translate(-center.x, -center.y, 0);
    stl_rotate_z(&(this->stl), (float)angle);
    this->translate(+center.x, +center.y, 0);
}

void TriangleMesh::align_to_bed()
{
    stl_translate_relative(&(this->stl), 0.0f, 0.0f, -this->stl.stats.min.z);
    stl_invalidate_shared_vertices(&this->stl);
}

TriangleMesh TriangleMesh::get_transformed_mesh(TransformationMatrix const & trafo) const
{
    TriangleMesh mesh;
    std::vector<double> trafo_arr = trafo.matrix3x4f();
    stl_get_transform(&(this->stl), &(mesh.stl), trafo_arr.data());
    stl_invalidate_shared_vertices(&(mesh.stl));
    return mesh;
}

void TriangleMesh::transform(TransformationMatrix const & trafo)
{
    std::vector<double> trafo_arr = trafo.matrix3x4f();
    stl_transform(&(this->stl), trafo_arr.data());
    stl_invalidate_shared_vertices(&(this->stl));
}

Pointf3s TriangleMesh::vertices()
{
    Pointf3s tmp {};
    if (this->repaired) {
        if (this->stl.v_shared == nullptr) 
            stl_generate_shared_vertices(&stl); // build the list of vertices
        for (auto i = 0; i < this->stl.stats.shared_vertices; i++) {
            const auto& v = this->stl.v_shared[i];
            tmp.emplace_back(Pointf3(v.x, v.y, v.z));
        }
    } else {
        Slic3r::Log::warn("TriangleMesh", "vertices() requires repair()");
    }
    return tmp;
}

Point3s TriangleMesh::facets() 
{
    Point3s tmp {};
    if (this->repaired) {
        if (this->stl.v_shared == nullptr) 
            stl_generate_shared_vertices(&stl); // build the list of vertices
        for (auto i = 0; i < stl.stats.number_of_facets; i++) {
            const auto& v = stl.v_indices[i];
            tmp.emplace_back(Point3(v.vertex[0], v.vertex[1], v.vertex[2]));
        }
    } else {
        Slic3r::Log::warn("TriangleMesh", "facets() requires repair()");
    }
    return tmp;
}

Pointf3s TriangleMesh::normals() const
{
    Pointf3s tmp {};
    if (this->repaired) {
        for (auto i = 0; i < stl.stats.number_of_facets; i++) {
            const auto& n = stl.facet_start[i].normal;
            tmp.emplace_back(Pointf3(n.x, n.y, n.z));
        }
    } else {
        Slic3r::Log::warn("TriangleMesh", "normals() requires repair()");
    }
    return tmp;
}

Pointf3 TriangleMesh::size() const
{
    const auto& sz = stl.stats.size;
    return Pointf3(sz.x, sz.y, sz.z);
}



Pointf3
TriangleMesh::center() const {
    return this->bounding_box().center();
}

std::vector<ExPolygons> 
TriangleMesh::slice(const std::vector<double>& z)
{
    // convert doubles to floats
    std::vector<float> z_f(z.begin(), z.end());
    TriangleMeshSlicer<Z> mslicer(this);
    std::vector<ExPolygons> layers;

    mslicer.slice(z_f, &layers);

    return layers;
}

mesh_stats
TriangleMesh::stats() const {
    mesh_stats tmp_stats;
    tmp_stats.number_of_facets = this->stl.stats.number_of_facets;
    tmp_stats.number_of_parts = this->stl.stats.number_of_parts;
    tmp_stats.volume = this->stl.stats.volume;
    tmp_stats.degenerate_facets = this->stl.stats.degenerate_facets;
    tmp_stats.edges_fixed = this->stl.stats.edges_fixed;
    tmp_stats.facets_removed = this->stl.stats.facets_removed;
    tmp_stats.facets_added = this->stl.stats.facets_added;
    tmp_stats.facets_reversed = this->stl.stats.facets_reversed;
    tmp_stats.backwards_edges = this->stl.stats.backwards_edges;
    tmp_stats.normals_fixed = this->stl.stats.normals_fixed;
    return tmp_stats;
}

BoundingBoxf3 TriangleMesh::bb3() const {
    Pointf3 pmin(this->stl.stats.min.x, this->stl.stats.min.y, this->stl.stats.min.z);
    Pointf3 pmax(this->stl.stats.max.x, this->stl.stats.max.y, this->stl.stats.max.z);
    return BoundingBoxf3(pmin, pmax);
}


void TriangleMesh::cut(Axis axis, double z, TriangleMesh* upper, TriangleMesh* lower) 
{
    switch(axis) {
        case X:
            TriangleMeshSlicer<X>(this).cut(z, upper, lower);
            break;
        case Y:
            TriangleMeshSlicer<Y>(this).cut(z, upper, lower);
            break;
        case Z:
            TriangleMeshSlicer<Z>(this).cut(z, upper, lower);
            break;
        default: 
            Slic3r::Log::error("TriangleMesh", "Invalid Axis supplied to cut()");
    }
}

TriangleMeshPtrs
TriangleMesh::split() const
{
    TriangleMeshPtrs meshes;
    std::set<int> seen_facets;
    
    // we need neighbors
    if (!this->repaired) CONFESS("split() requires repair()");
    
    // loop while we have remaining facets
    while (1) {
        // get the first facet
        std::queue<int> facet_queue;
        std::deque<int> facets;
        for (int facet_idx = 0; facet_idx < this->stl.stats.number_of_facets; facet_idx++) {
            if (seen_facets.find(facet_idx) == seen_facets.end()) {
                // if facet was not seen put it into queue and start searching
                facet_queue.push(facet_idx);
                break;
            }
        }
        if (facet_queue.empty()) break;
        
        while (!facet_queue.empty()) {
            int facet_idx = facet_queue.front();
            facet_queue.pop();
            if (seen_facets.find(facet_idx) != seen_facets.end()) continue;
            facets.push_back(facet_idx);
            for (int j = 0; j <= 2; j++) {
                facet_queue.push(this->stl.neighbors_start[facet_idx].neighbor[j]);
            }
            seen_facets.insert(facet_idx);
        }
        
        TriangleMesh* mesh = new TriangleMesh;
        meshes.push_back(mesh);
        mesh->stl.stats.type = inmemory;
        mesh->stl.stats.number_of_facets = facets.size();
        mesh->stl.stats.original_num_facets = mesh->stl.stats.number_of_facets;
        stl_clear_error(&mesh->stl);
        stl_allocate(&mesh->stl);
        
        int first = 1;
        for (std::deque<int>::const_iterator facet = facets.begin(); facet != facets.end(); ++facet) {
            mesh->stl.facet_start[facet - facets.begin()] = this->stl.facet_start[*facet];
            stl_facet_stats(&mesh->stl, this->stl.facet_start[*facet], first);
            first = 0;
        }
    }
    
    return meshes;
}

TriangleMeshPtrs
TriangleMesh::cut_by_grid(const Pointf &grid) const
{
    TriangleMesh mesh = *this;
    const BoundingBoxf3 bb = mesh.bounding_box();            
    const Sizef3 size = bb.size();
    const size_t x_parts = ceil((size.x - EPSILON)/grid.x);
    const size_t y_parts = ceil((size.y - EPSILON)/grid.y);
    
    TriangleMeshPtrs meshes;
    for (size_t i = 1; i <= x_parts; ++i) {
        TriangleMesh curr;
        if (i == x_parts) {
            curr = mesh;
        } else {
            TriangleMesh next;
            TriangleMeshSlicer<X>(&mesh).cut(bb.min.x + (grid.x * i), &next, &curr);
            curr.repair();
            next.repair();
            mesh = next;
        }
        
        for (size_t j = 1; j <= y_parts; ++j) {
            TriangleMesh* tile;
            if (j == y_parts) {
                tile = new TriangleMesh(curr);
            } else {
                TriangleMesh next;
                tile = new TriangleMesh;
                TriangleMeshSlicer<Y>(&curr).cut(bb.min.y + (grid.y * j), &next, tile);
                tile->repair();
                next.repair();
                curr = next;
            }
            
            meshes.push_back(tile);
        }
    }
    return meshes;
}

void
TriangleMesh::merge(const TriangleMesh &mesh)
{
    // reset stats and metadata
    int number_of_facets = this->stl.stats.number_of_facets;
    stl_invalidate_shared_vertices(&this->stl);
    this->repaired = false;
    
    // update facet count and allocate more memory
    this->stl.stats.number_of_facets = number_of_facets + mesh.stl.stats.number_of_facets;
    this->stl.stats.original_num_facets = this->stl.stats.number_of_facets;
    stl_reallocate(&this->stl);
    
    // copy facets
    std::copy(mesh.stl.facet_start, mesh.stl.facet_start + mesh.stl.stats.number_of_facets, this->stl.facet_start + number_of_facets);
    std::copy(mesh.stl.neighbors_start, mesh.stl.neighbors_start + mesh.stl.stats.number_of_facets, this->stl.neighbors_start + number_of_facets);
    
    // update size
    stl_get_size(&this->stl);
}

/* this will return scaled ExPolygons */
ExPolygons
TriangleMesh::horizontal_projection() const
{
    Polygons pp;
    pp.reserve(this->stl.stats.number_of_facets);
    for (int i = 0; i < this->stl.stats.number_of_facets; i++) {
        stl_facet* facet = &this->stl.facet_start[i];
        Polygon p;
        p.points.resize(3);
        p.points[0] = Point(facet->vertex[0].x / SCALING_FACTOR, facet->vertex[0].y / SCALING_FACTOR);
        p.points[1] = Point(facet->vertex[1].x / SCALING_FACTOR, facet->vertex[1].y / SCALING_FACTOR);
        p.points[2] = Point(facet->vertex[2].x / SCALING_FACTOR, facet->vertex[2].y / SCALING_FACTOR);
        p.make_counter_clockwise();  // do this after scaling, as winding order might change while doing that
        pp.push_back(p);
    }
    
    // the offset factor was tuned using groovemount.stl
    return union_ex(offset(pp, 0.01 / SCALING_FACTOR), true);
}

Polygon
TriangleMesh::convex_hull()
{
    this->require_shared_vertices();
    Points pp;
    pp.reserve(this->stl.stats.shared_vertices);
    for (int i = 0; i < this->stl.stats.shared_vertices; i++) {
        stl_vertex* v = &this->stl.v_shared[i];
        pp.push_back(Point(v->x / SCALING_FACTOR, v->y / SCALING_FACTOR));
    }
    return Slic3r::Geometry::convex_hull(pp);
}

BoundingBoxf3
TriangleMesh::bounding_box() const
{
    BoundingBoxf3 bb;
    bb.min.x = this->stl.stats.min.x;
    bb.min.y = this->stl.stats.min.y;
    bb.min.z = this->stl.stats.min.z;
    bb.max.x = this->stl.stats.max.x;
    bb.max.y = this->stl.stats.max.y;
    bb.max.z = this->stl.stats.max.z;
    return bb;
}

BoundingBoxf3
TriangleMesh::get_transformed_bounding_box(TransformationMatrix const & trafo) const
{
    BoundingBoxf3 bbox;
    for (int i = 0; i < this->stl.stats.number_of_facets; ++ i) {
        const stl_facet &facet = this->stl.facet_start[i];
        for (int j = 0; j < 3; ++ j) {
            double v_x = facet.vertex[j].x;
            double v_y = facet.vertex[j].y;
            double v_z = facet.vertex[j].z;
            Pointf3 poi;
            poi.x = float(trafo.m00*v_x + trafo.m01*v_y + trafo.m02*v_z + trafo.m03);
            poi.y = float(trafo.m10*v_x + trafo.m11*v_y + trafo.m12*v_z + trafo.m13);
            poi.z = float(trafo.m20*v_x + trafo.m21*v_y + trafo.m22*v_z + trafo.m23);
            bbox.merge(poi);
        }
    }
    return bbox;
}

void
TriangleMesh::require_shared_vertices()
{
    if (!this->repaired) this->repair();
    if (this->stl.v_shared == NULL) stl_generate_shared_vertices(&(this->stl));
}

void
TriangleMesh::reverse_normals()
{
    stl_reverse_all_facets(&this->stl);
    if (this->stl.stats.volume != -1) this->stl.stats.volume *= -1.0;
}

void
TriangleMesh::extrude_tin(float offset)
{
    calculate_normals(&this->stl);
    
    const int number_of_facets = this->stl.stats.number_of_facets;
    if (number_of_facets == 0)
        throw std::runtime_error("Error: file is empty");
    
    const float z = this->stl.stats.min.z - offset;
    
    for (int i = 0; i < number_of_facets; ++i) {
        const stl_facet &facet = this->stl.facet_start[i];
        
        if (facet.normal.z < 0)
            throw std::runtime_error("Invalid 2.5D mesh: at least one facet points downwards.");
        
        for (int j = 0; j < 3; ++j) {
            if (this->stl.neighbors_start[i].neighbor[j] == -1) {
                stl_facet new_facet;
                float normal[3];
                
                // first triangle
                new_facet.vertex[0] = new_facet.vertex[2] = facet.vertex[(j+1)%3];
                new_facet.vertex[1] = facet.vertex[j];
                new_facet.vertex[2].z = z;
                stl_calculate_normal(normal, &new_facet);
                stl_normalize_vector(normal);
                new_facet.normal.x = normal[0];
                new_facet.normal.y = normal[1];
                new_facet.normal.z = normal[2];
                stl_add_facet(&this->stl, &new_facet);
                
                // second triangle
                new_facet.vertex[0] = new_facet.vertex[1] = facet.vertex[j];
                new_facet.vertex[2] = facet.vertex[(j+1)%3];
                new_facet.vertex[1].z = new_facet.vertex[2].z = z;
                new_facet.normal.x = normal[0];
                new_facet.normal.y = normal[1];
                new_facet.normal.z = normal[2];
                stl_add_facet(&this->stl, &new_facet);
            }
        }
    }
    stl_get_size(&this->stl);
    
    this->repair();
}

// Generate the vertex list for a cube solid of arbitrary size in X/Y/Z.
TriangleMesh
TriangleMesh::make_cube(double x, double y, double z) {
    Pointf3 pv[8] = { 
        Pointf3(x, y, 0), Pointf3(x, 0, 0), Pointf3(0, 0, 0), 
        Pointf3(0, y, 0), Pointf3(x, y, z), Pointf3(0, y, z), 
        Pointf3(0, 0, z), Pointf3(x, 0, z) 
    };
    Point3 fv[12] = { 
        Point3(0, 1, 2), Point3(0, 2, 3), Point3(4, 5, 6), 
        Point3(4, 6, 7), Point3(0, 4, 7), Point3(0, 7, 1), 
        Point3(1, 7, 6), Point3(1, 6, 2), Point3(2, 6, 5), 
        Point3(2, 5, 3), Point3(4, 0, 3), Point3(4, 3, 5) 
    };

    std::vector<Point3> facets(&fv[0], &fv[0]+12);
    Pointf3s vertices(&pv[0], &pv[0]+8);

    TriangleMesh mesh(vertices ,facets);
    mesh.repair();
    return mesh;
}

// Generate the mesh for a cylinder and return it, using 
// the generated angle to calculate the top mesh triangles.
// Default is 360 sides, angle fa is in radians.
TriangleMesh
TriangleMesh::make_cylinder(double r, double h, double fa) {
    Pointf3s vertices;
    std::vector<Point3> facets;

    // 2 special vertices, top and bottom center, rest are relative to this
    vertices.push_back(Pointf3(0.0, 0.0, 0.0));
    vertices.push_back(Pointf3(0.0, 0.0, h));

    // adjust via rounding to get an even multiple for any provided angle.
    double angle = (2*PI / floor(2*PI / fa));

    // for each line along the polygon approximating the top/bottom of the
    // circle, generate four points and four facets (2 for the wall, 2 for the
    // top and bottom.
    // Special case: Last line shares 2 vertices with the first line.
    unsigned id = vertices.size() - 1;
    vertices.push_back(Pointf3(sin(0) * r , cos(0) * r, 0));
    vertices.push_back(Pointf3(sin(0) * r , cos(0) * r, h));
    for (double i = angle; i < 2*PI - angle; i+=angle) {
        Pointf3 b(0, r, 0);
        Pointf3 t(0, r, h);
        b.rotate(i, Pointf3(0,0,0)); 
        t.rotate(i, Pointf3(0,0,h));
        vertices.push_back(b);
        vertices.push_back(t);
        id = vertices.size() - 1;
        facets.push_back(Point3( 0, id - 1, id - 3)); // top
        facets.push_back(Point3(id,      1, id - 2)); // bottom
        facets.push_back(Point3(id, id - 2, id - 3)); // upper-right of side
        facets.push_back(Point3(id, id - 3, id - 1)); // bottom-left of side
    }
    // Connect the last set of vertices with the first.
    facets.push_back(Point3( 2, 0, id - 1));
    facets.push_back(Point3( 1, 3,     id));
    facets.push_back(Point3(id, 3,      2));
    facets.push_back(Point3(id, 2, id - 1));
    
    TriangleMesh mesh(vertices, facets);
    mesh.repair();
    return mesh;
}

// Generates mesh for a sphere centered about the origin, using the generated angle
// to determine the granularity. 
// Default angle is 1 degree.
TriangleMesh
TriangleMesh::make_sphere(double rho, double fa) {
    Pointf3s vertices;
    std::vector<Point3> facets;

    // Algorithm: 
    // Add points one-by-one to the sphere grid and form facets using relative coordinates.
    // Sphere is composed effectively of a mesh of stacked circles.

    // adjust via rounding to get an even multiple for any provided angle.
    double angle = (2*PI / floor(2*PI / fa));

    // Ring to be scaled to generate the steps of the sphere
    std::vector<double> ring;
    for (double i = 0; i < 2*PI; i+=angle) {
        ring.push_back(i);
    }
    const size_t steps = ring.size(); 
    const double increment = (double)(1.0 / (double)steps);

    // special case: first ring connects to 0,0,0
    // insert and form facets.
    vertices.push_back(Pointf3(0.0, 0.0, -rho));
    size_t id = vertices.size();
    for (size_t i = 0; i < ring.size(); i++) {
        // Fixed scaling 
        const double z = -rho + increment*rho*2.0;
        // radius of the circle for this step.
        const double r = sqrt(std::abs(rho*rho - z*z));
        Pointf3 b(0, r, z);
        b.rotate(ring[i], Pointf3(0,0,z)); 
        vertices.push_back(b);
        if (i == 0) {
            facets.push_back(Point3(1, 0, ring.size()));
        } else {
            facets.push_back(Point3(id, 0, id - 1));
        }
        id++;
    }

    // General case: insert and form facets for each step, joining it to the ring below it.
    for (size_t s = 2; s < steps - 1; s++) {
        const double z = -rho + increment*(double)s*2.0*rho;
        const double r = sqrt(std::abs(rho*rho - z*z));

        for (size_t i = 0; i < ring.size(); i++) {
            Pointf3 b(0, r, z);
            b.rotate(ring[i], Pointf3(0,0,z)); 
            vertices.push_back(b);
            if (i == 0) {
                // wrap around
                facets.push_back(Point3(id + ring.size() - 1 , id, id - 1)); 
                facets.push_back(Point3(id, id - ring.size(),  id - 1)); 
            } else {
                facets.push_back(Point3(id , id - ring.size(), (id - 1) - ring.size())); 
                facets.push_back(Point3(id, id - 1 - ring.size() ,  id - 1)); 
            }
            id++;
        } 
    }


    // special case: last ring connects to 0,0,rho*2.0
    // only form facets.
    vertices.push_back(Pointf3(0.0, 0.0, rho));
    for (size_t i = 0; i < ring.size(); i++) {
        if (i == 0) {
            // third vertex is on the other side of the ring.
            facets.push_back(Point3(id, id - ring.size(),  id - 1));
        } else {
            facets.push_back(Point3(id, id - ring.size() + i,  id - ring.size() + (i - 1)));
        }
    }
    id++;
    TriangleMesh mesh(vertices, facets);
    mesh.repair();
    return mesh;
}

template <Axis A>
void
TriangleMeshSlicer<A>::slice(const std::vector<float> &z, std::vector<Polygons>* layers) const
{
    /**
       This method gets called with a list of unscaled Z coordinates and outputs
       a vector pointer having the same number of items as the original list.
       Each item is a vector of polygons created by slicing our mesh at the 
       given heights.
       
       This method should basically combine the behavior of the existing
       Perl methods defined in lib/Slic3r/TriangleMesh.pm:
       
       - analyze(): this creates the 'facets_edges' and the 'edges_facets'
            tables (we don't need the 'edges' table)
       
       - slice_facet(): this has to be done for each facet. It generates 
            intersection lines with each plane identified by the Z list.
            The get_layer_range() binary search used to identify the Z range
            of the facet is already ported to C++ (see Object.xsp)
       
       - make_loops(): this has to be done for each layer. It creates polygons
            from the lines generated by the previous step.
        
        At the end, we free the tables generated by analyze() as we don't 
        need them anymore.
        
        NOTE: this method accepts a vector of floats because the mesh coordinate
        type is float.
    */
    
    std::vector<IntersectionLines> lines(z.size());
    {
        boost::mutex lines_mutex;
        parallelize<int>(
            0,
            this->mesh->stl.stats.number_of_facets-1,
            boost::bind(&TriangleMeshSlicer<A>::_slice_do, this, _1, &lines, &lines_mutex, z)
        );
    }
    
    // v_scaled_shared could be freed here
    
    // build loops
    layers->resize(z.size());
    parallelize<size_t>(
        0,
        lines.size()-1,
        boost::bind(&TriangleMeshSlicer<A>::_make_loops_do, this, _1, &lines, layers)
    );
}

template <Axis A>
void
TriangleMeshSlicer<A>::_slice_do(size_t facet_idx, std::vector<IntersectionLines>* lines, boost::mutex* lines_mutex, 
    const std::vector<float> &z) const
{
    const stl_facet &facet = this->mesh->stl.facet_start[facet_idx];
    
    // find facet extents
    const float min_z = fminf(_z(facet.vertex[0]), fminf(_z(facet.vertex[1]), _z(facet.vertex[2])));
    const float max_z = fmaxf(_z(facet.vertex[0]), fmaxf(_z(facet.vertex[1]), _z(facet.vertex[2])));
    
    #ifdef SLIC3R_DEBUG
    printf("\n==> FACET %zu (%f,%f,%f - %f,%f,%f - %f,%f,%f):\n", facet_idx,
        _x(facet.vertex[0]), _y(facet.vertex[0]), _z(facet.vertex[0]),
        _x(facet.vertex[1]), _y(facet.vertex[1]), _z(facet.vertex[1]),
        _x(facet.vertex[2]), _y(facet.vertex[2]), _z(facet.vertex[2]));
    printf("z: min = %.2f, max = %.2f\n", min_z, max_z);
    #endif
    
    // find layer extents
    std::vector<float>::const_iterator min_layer, max_layer;
    min_layer = std::lower_bound(z.begin(), z.end(), min_z); // first layer whose slice_z is >= min_z
    max_layer = std::upper_bound(z.begin() + (min_layer - z.begin()), z.end(), max_z) - 1; // last layer whose slice_z is <= max_z
    #ifdef SLIC3R_DEBUG
    printf("layers: min = %d, max = %d\n", (int)(min_layer - z.begin()), (int)(max_layer - z.begin()));
    #endif
    
    for (std::vector<float>::const_iterator it = min_layer; it != max_layer + 1; ++it) {
        std::vector<float>::size_type layer_idx = it - z.begin();
        this->slice_facet(*it / SCALING_FACTOR, facet, facet_idx, min_z, max_z, &(*lines)[layer_idx], lines_mutex);
    }
}

template <Axis A>
void
TriangleMeshSlicer<A>::slice(const std::vector<float> &z, std::vector<ExPolygons>* layers) const
{
    std::vector<Polygons> layers_p;
    this->slice(z, &layers_p);
    
    layers->resize(z.size());
    for (std::vector<Polygons>::const_iterator loops = layers_p.begin(); loops != layers_p.end(); ++loops) {
        #ifdef SLIC3R_DEBUG
        size_t layer_id = loops - layers_p.begin();
        printf("Layer %zu (slice_z = %.2f):\n", layer_id, z[layer_id]);
        #endif
        
        this->make_expolygons(*loops, &(*layers)[ loops - layers_p.begin() ]);
    }
}

template <Axis A>
void
TriangleMeshSlicer<A>::slice(float z, ExPolygons* slices) const
{
    std::vector<float> zz;
    zz.push_back(z);
    std::vector<ExPolygons> layers;
    this->slice(zz, &layers);
    append_to(*slices, layers.front());
}

template <Axis A>
void
TriangleMeshSlicer<A>::slice_facet(float slice_z, const stl_facet &facet, const int &facet_idx,
    const float &min_z, const float &max_z, std::vector<IntersectionLine>* lines,
    boost::mutex* lines_mutex) const
{
    std::vector<IntersectionPoint> points;
    std::vector< std::vector<IntersectionPoint>::size_type > points_on_layer;
    bool found_horizontal_edge = false;
    
    /* reorder vertices so that the first one is the one with lowest Z
       this is needed to get all intersection lines in a consistent order
       (external on the right of the line) */
    int i = 0;
    if (_z(facet.vertex[1]) == min_z) {
        // vertex 1 has lowest Z
        i = 1;
    } else if (_z(facet.vertex[2]) == min_z) {
        // vertex 2 has lowest Z
        i = 2;
    }
    for (int j = i; (j-i) < 3; j++) {  // loop through facet edges
        int edge_id = this->facets_edges[facet_idx][j % 3];
        int a_id = this->mesh->stl.v_indices[facet_idx].vertex[j % 3];
        int b_id = this->mesh->stl.v_indices[facet_idx].vertex[(j+1) % 3];
        stl_vertex* a = &this->v_scaled_shared[a_id];
        stl_vertex* b = &this->v_scaled_shared[b_id];
        
        if (_z(*a) == _z(*b) && _z(*a) == slice_z) {
            // edge is horizontal and belongs to the current layer
            
            stl_vertex &v0 = this->v_scaled_shared[ this->mesh->stl.v_indices[facet_idx].vertex[0] ];
            stl_vertex &v1 = this->v_scaled_shared[ this->mesh->stl.v_indices[facet_idx].vertex[1] ];
            stl_vertex &v2 = this->v_scaled_shared[ this->mesh->stl.v_indices[facet_idx].vertex[2] ];
            IntersectionLine line;
            if (min_z == max_z) {
                line.edge_type = feHorizontal;
                if (_z(this->mesh->stl.facet_start[facet_idx].normal) < 0) {
                    /*  if normal points downwards this is a bottom horizontal facet so we reverse
                        its point order */
                    std::swap(a, b);
                    std::swap(a_id, b_id);
                }
            } else if (_z(v0) < slice_z || _z(v1) < slice_z || _z(v2) < slice_z) {
                line.edge_type = feTop;
                std::swap(a, b);
                std::swap(a_id, b_id);
            } else {
                line.edge_type = feBottom;
            }
            line.a.x    = _x(*a);
            line.a.y    = _y(*a);
            line.b.x    = _x(*b);
            line.b.y    = _y(*b);
            line.a_id   = a_id;
            line.b_id   = b_id;
            if (lines_mutex != NULL) {
                boost::lock_guard<boost::mutex> l(*lines_mutex);
                lines->push_back(line);
            } else {
                lines->push_back(line);
            }
            
            found_horizontal_edge = true;
            
            // if this is a top or bottom edge, we can stop looping through edges
            // because we won't find anything interesting
            
            if (line.edge_type != feHorizontal) return;
        } else if (_z(*a) == slice_z) {
            IntersectionPoint point;
            point.x         = _x(*a);
            point.y         = _y(*a);
            point.point_id  = a_id;
            points.push_back(point);
            points_on_layer.push_back(points.size()-1);
        } else if (_z(*b) == slice_z) {
            IntersectionPoint point;
            point.x         = _x(*b);
            point.y         = _y(*b);
            point.point_id  = b_id;
            points.push_back(point);
            points_on_layer.push_back(points.size()-1);
        } else if ((_z(*a) < slice_z && _z(*b) > slice_z) || (_z(*b) < slice_z && _z(*a) > slice_z)) {
            // edge intersects the current layer; calculate intersection
            
            IntersectionPoint point;
            point.x         = _x(*b) + (_x(*a) - _x(*b)) * (slice_z - _z(*b)) / (_z(*a) - _z(*b));
            point.y         = _y(*b) + (_y(*a) - _y(*b)) * (slice_z - _z(*b)) / (_z(*a) - _z(*b));
            point.edge_id   = edge_id;
            points.push_back(point);
        }
    }
    if (found_horizontal_edge) return;
    
    if (!points_on_layer.empty()) {
        // we can't have only one point on layer because each vertex gets detected
        // twice (once for each edge), and we can't have three points on layer because
        // we assume this code is not getting called for horizontal facets
        assert(points_on_layer.size() == 2);
        assert( points[ points_on_layer[0] ].point_id == points[ points_on_layer[1] ].point_id );
        if (points.size() < 3) return;  // no intersection point, this is a V-shaped facet tangent to plane
        points.erase( points.begin() + points_on_layer[1] );
    }
    
    if (!points.empty()) {
        assert(points.size() == 2); // facets must intersect each plane 0 or 2 times
        IntersectionLine line;
        line.a          = (Point)points[1];
        line.b          = (Point)points[0];
        line.a_id       = points[1].point_id;
        line.b_id       = points[0].point_id;
        line.edge_a_id  = points[1].edge_id;
        line.edge_b_id  = points[0].edge_id;
        if (lines_mutex != NULL) {
            boost::lock_guard<boost::mutex> l(*lines_mutex);
            lines->push_back(line);
        } else {
            lines->push_back(line);
        }
        return;
    }
}

template <Axis A>
void
TriangleMeshSlicer<A>::_make_loops_do(size_t i, std::vector<IntersectionLines>* lines, std::vector<Polygons>* layers) const
{
    this->make_loops((*lines)[i], &(*layers)[i]);
}

template <Axis A>
void
TriangleMeshSlicer<A>::make_loops(std::vector<IntersectionLine> &lines, Polygons* loops) const
{
    /*
    SVG svg("lines.svg");
    svg.draw(lines);
    svg.Close();
    */
    
    // remove tangent edges
    for (IntersectionLines::iterator line = lines.begin(); line != lines.end(); ++line) {
        if (line->skip || line->edge_type == feNone) continue;
        
        /* if the line is a facet edge, find another facet edge
           having the same endpoints but in reverse order */
        for (IntersectionLines::iterator line2 = line + 1; line2 != lines.end(); ++line2) {
            if (line2->skip || line2->edge_type == feNone) continue;
            
            // are these facets adjacent? (sharing a common edge on this layer)
            if (line->a_id == line2->a_id && line->b_id == line2->b_id) {
                line2->skip = true;
                
                /* if they are both oriented upwards or downwards (like a 'V')
                   then we can remove both edges from this layer since it won't 
                   affect the sliced shape */
                /* if one of them is oriented upwards and the other is oriented
                   downwards, let's only keep one of them (it doesn't matter which
                   one since all 'top' lines were reversed at slicing) */
                if (line->edge_type == line2->edge_type) {
                    line->skip = true;
                    break;
                }
            } else if (line->a_id == line2->b_id && line->b_id == line2->a_id) {
                /* if this edge joins two horizontal facets, remove both of them */
                if (line->edge_type == feHorizontal && line2->edge_type == feHorizontal) {
                    line->skip = true;
                    line2->skip = true;
                    break;
                }
            }
        }
    }
    
    // build a map of lines by edge_a_id and a_id
    std::vector<IntersectionLinePtrs> by_edge_a_id, by_a_id;
    by_edge_a_id.resize(this->mesh->stl.stats.number_of_facets * 3);
    by_a_id.resize(this->mesh->stl.stats.shared_vertices);
    for (IntersectionLines::iterator line = lines.begin(); line != lines.end(); ++line) {
        if (line->skip) continue;
        if (line->edge_a_id != -1) by_edge_a_id[line->edge_a_id].push_back(&(*line));
        if (line->a_id != -1) by_a_id[line->a_id].push_back(&(*line));
    }
    
    CYCLE: while (1) {
        // take first spare line and start a new loop
        IntersectionLine* first_line = NULL;
        for (IntersectionLines::iterator line = lines.begin(); line != lines.end(); ++line) {
            if (line->skip) continue;
            first_line = &(*line);
            break;
        }
        if (first_line == NULL) break;
        first_line->skip = true;
        IntersectionLinePtrs loop;
        loop.push_back(first_line);
        
        /*
        printf("first_line edge_a_id = %d, edge_b_id = %d, a_id = %d, b_id = %d, a = %d,%d, b = %d,%d\n", 
            first_line->edge_a_id, first_line->edge_b_id, first_line->a_id, first_line->b_id,
            first_line->a.x, first_line->a.y, first_line->b.x, first_line->b.y);
        */
        
        while (1) {
            // find a line starting where last one finishes
            IntersectionLine* next_line = NULL;
            if (loop.back()->edge_b_id != -1) {
                IntersectionLinePtrs &candidates = by_edge_a_id[loop.back()->edge_b_id];
                for (IntersectionLinePtrs::iterator lineptr = candidates.begin(); lineptr != candidates.end(); ++lineptr) {
                    if ((*lineptr)->skip) continue;
                    next_line = *lineptr;
                    break;
                }
            }
            if (next_line == NULL && loop.back()->b_id != -1) {
                IntersectionLinePtrs &candidates = by_a_id[loop.back()->b_id];
                for (IntersectionLinePtrs::iterator lineptr = candidates.begin(); lineptr != candidates.end(); ++lineptr) {
                    if ((*lineptr)->skip) continue;
                    next_line = *lineptr;
                    break;
                }
            }
            
            if (next_line == NULL) {
                // check whether we closed this loop
                if ((loop.front()->edge_a_id != -1 && loop.front()->edge_a_id == loop.back()->edge_b_id)
                    || (loop.front()->a_id != -1 && loop.front()->a_id == loop.back()->b_id)) {
                    // loop is complete
                    Polygon p;
                    p.points.reserve(loop.size());
                    for (IntersectionLinePtrs::const_iterator lineptr = loop.begin(); lineptr != loop.end(); ++lineptr) {
                        p.points.push_back((*lineptr)->a);
                    }
                    
                    loops->push_back(p);
                    
                    #ifdef SLIC3R_DEBUG
                    printf("  Discovered %s polygon of %d points\n", (p.is_counter_clockwise() ? "ccw" : "cw"), (int)p.points.size());
                    #endif
                    
                    goto CYCLE;
                }
                
                // we can't close this loop!
                //// push @failed_loops, [@loop];
                //#ifdef SLIC3R_DEBUG
                printf("  Unable to close this loop having %d points\n", (int)loop.size());
                //#endif
                goto CYCLE;
            }
            /*
            printf("next_line edge_a_id = %d, edge_b_id = %d, a_id = %d, b_id = %d, a = %d,%d, b = %d,%d\n", 
                next_line->edge_a_id, next_line->edge_b_id, next_line->a_id, next_line->b_id,
                next_line->a.x, next_line->a.y, next_line->b.x, next_line->b.y);
            */
            loop.push_back(next_line);
            next_line->skip = true;
        }
    }
}

class _area_comp {
    public:
    _area_comp(std::vector<double>* _aa) : abs_area(_aa) {};
    bool operator() (const size_t &a, const size_t &b) {
        return (*this->abs_area)[a] > (*this->abs_area)[b];
    }
    
    private:
    std::vector<double>* abs_area;
};

template <Axis A>
void
TriangleMeshSlicer<A>::make_expolygons_simple(std::vector<IntersectionLine> &lines, ExPolygons* slices) const
{
    Polygons loops;
    this->make_loops(lines, &loops);
    
    // cache slice contour area
    std::vector<double> area;
    area.resize(slices->size(), -1);
    
    Polygons cw;
    for (const Polygon &loop : loops) {
        const double a = loop.area();
        if (a >= 0) {
            slices->push_back(ExPolygon(loop));
            area.push_back(a);
        } else {
            cw.push_back(loop);
        }
    }
    
    // assign holes to contours
    for (const Polygon &loop : cw) {
        int slice_idx = -1;
        double current_contour_area = -1;
        for (size_t i = 0; i < slices->size(); ++i) {
            if ((*slices)[i].contour.contains(loop.points.front())) {
                if (area[i] == -1) area[i] = (*slices)[i].contour.area();
                if (area[i] < current_contour_area || current_contour_area == -1) {
                    slice_idx = i;
                    current_contour_area = area[i];
                }
            }
        }
        
        // discard holes which couldn't fit inside a contour as they are probably
        // invalid polygons (self-intersecting)
        if (slice_idx > -1)
            (*slices)[slice_idx].holes.push_back(loop);
    }
}

template <Axis A>
void
TriangleMeshSlicer<A>::make_expolygons(const Polygons &loops, ExPolygons* slices) const
{
    /**
        Input loops are not suitable for evenodd nor nonzero fill types, as we might get
        two consecutive concentric loops having the same winding order - and we have to 
        respect such order. In that case, evenodd would create wrong inversions, and nonzero
        would ignore holes inside two concentric contours.
        So we're ordering loops and collapse consecutive concentric loops having the same 
        winding order.
        \todo find a faster algorithm for this, maybe with some sort of binary search.
        If we computed a "nesting tree" we could also just remove the consecutive loops
        having the same winding order, and remove the extra one(s) so that we could just
        supply everything to offset() instead of performing several union/diff calls.
    
        we sort by area assuming that the outermost loops have larger area;
        the previous sorting method, based on $b->contains($a->[0]), failed to nest
        loops correctly in some edge cases when original model had overlapping facets
    */

    std::vector<double> area;
    std::vector<double> abs_area;
    std::vector<size_t> sorted_area;  // vector of indices
    for (Polygons::const_iterator loop = loops.begin(); loop != loops.end(); ++loop) {
        double a = loop->area();
        area.push_back(a);
        abs_area.push_back(std::fabs(a));
        sorted_area.push_back(loop - loops.begin());
    }
    
    std::sort(sorted_area.begin(), sorted_area.end(), _area_comp(&abs_area));  // outer first

    // we don't perform a safety offset now because it might reverse cw loops
    Polygons p_slices;
    for (std::vector<size_t>::const_iterator loop_idx = sorted_area.begin(); loop_idx != sorted_area.end(); ++loop_idx) {
        /* we rely on the already computed area to determine the winding order
           of the loops, since the Orientation() function provided by Clipper
           would do the same, thus repeating the calculation */
        Polygons::const_iterator loop = loops.begin() + *loop_idx;
        if (area[*loop_idx] > +EPSILON) {
            p_slices.push_back(*loop);
        } else if (area[*loop_idx] < -EPSILON) {
            p_slices = diff(p_slices, *loop);
        }
    }

    // perform a safety offset to merge very close facets (TODO: find test case for this)
    double safety_offset = scale_(0.0499);
    ExPolygons ex_slices = offset2_ex(p_slices, +safety_offset, -safety_offset);
    
    #ifdef SLIC3R_DEBUG
    size_t holes_count = 0;
    for (ExPolygons::const_iterator e = ex_slices.begin(); e != ex_slices.end(); ++e) {
        holes_count += e->holes.size();
    }
    printf("%zu surface(s) having %zu holes detected from %zu polylines\n",
        ex_slices.size(), holes_count, loops.size());
    #endif
    
    // append to the supplied collection
    slices->insert(slices->end(), ex_slices.begin(), ex_slices.end());
}

template <Axis A>
void
TriangleMeshSlicer<A>::make_expolygons(std::vector<IntersectionLine> &lines, ExPolygons* slices) const
{
    Polygons pp;
    this->make_loops(lines, &pp);
    this->make_expolygons(pp, slices);
}

template <Axis A>
void
TriangleMeshSlicer<A>::cut(float z, TriangleMesh* upper, TriangleMesh* lower) const
{
    IntersectionLines upper_lines, lower_lines;
    
    const float scaled_z = scale_(z);
    for (int facet_idx = 0; facet_idx < this->mesh->stl.stats.number_of_facets; facet_idx++) {
        stl_facet* facet = &this->mesh->stl.facet_start[facet_idx];
        
        // find facet extents
        float min_z = fminf(_z(facet->vertex[0]), fminf(_z(facet->vertex[1]), _z(facet->vertex[2])));
        float max_z = fmaxf(_z(facet->vertex[0]), fmaxf(_z(facet->vertex[1]), _z(facet->vertex[2])));
        
        // intersect facet with cutting plane
        IntersectionLines lines;
        this->slice_facet(scaled_z, *facet, facet_idx, min_z, max_z, &lines);
        
        // save intersection lines for generating correct triangulations
        for (IntersectionLines::const_iterator it = lines.begin(); it != lines.end(); ++it) {
            if (it->edge_type == feTop) {
                lower_lines.push_back(*it);
            } else if (it->edge_type == feBottom) {
                upper_lines.push_back(*it);
            } else if (it->edge_type != feHorizontal) {
                lower_lines.push_back(*it);
                upper_lines.push_back(*it);
            }
        }
        
        if (min_z > z || (min_z == z && max_z > min_z)) {
            // facet is above the cut plane and does not belong to it
            if (upper != NULL) stl_add_facet(&upper->stl, facet);
        } else if (max_z < z || (max_z == z && max_z > min_z)) {
            // facet is below the cut plane and does not belong to it
            if (lower != NULL) stl_add_facet(&lower->stl, facet);
        } else if (min_z < z && max_z > z) {
            // facet is cut by the slicing plane
            
            // look for the vertex on whose side of the slicing plane there are no other vertices
            int isolated_vertex;
            if ( (_z(facet->vertex[0]) > z) == (_z(facet->vertex[1]) > z) ) {
                isolated_vertex = 2;
            } else if ( (_z(facet->vertex[1]) > z) == (_z(facet->vertex[2]) > z) ) {
                isolated_vertex = 0;
            } else {
                isolated_vertex = 1;
            }
            
            // get vertices starting from the isolated one
            stl_vertex* v0 = &facet->vertex[isolated_vertex];
            stl_vertex* v1 = &facet->vertex[(isolated_vertex+1) % 3];
            stl_vertex* v2 = &facet->vertex[(isolated_vertex+2) % 3];
            
            // intersect v0-v1 and v2-v0 with cutting plane and make new vertices
            stl_vertex v0v1, v2v0;
            _x(v0v1) = _x(*v1) + (_x(*v0) - _x(*v1)) * (z - _z(*v1)) / (_z(*v0) - _z(*v1));
            _y(v0v1) = _y(*v1) + (_y(*v0) - _y(*v1)) * (z - _z(*v1)) / (_z(*v0) - _z(*v1));
            _z(v0v1) = z;
            _x(v2v0) = _x(*v2) + (_x(*v0) - _x(*v2)) * (z - _z(*v2)) / (_z(*v0) - _z(*v2));
            _y(v2v0) = _y(*v2) + (_y(*v0) - _y(*v2)) * (z - _z(*v2)) / (_z(*v0) - _z(*v2));
            _z(v2v0) = z;
            
            // build the triangular facet
            stl_facet triangle;
            triangle.normal = facet->normal;
            triangle.vertex[0] = *v0;
            triangle.vertex[1] = v0v1;
            triangle.vertex[2] = v2v0;
            
            // build the facets forming a quadrilateral on the other side
            stl_facet quadrilateral[2];
            quadrilateral[0].normal = facet->normal;
            quadrilateral[0].vertex[0] = *v1;
            quadrilateral[0].vertex[1] = *v2;
            quadrilateral[0].vertex[2] = v0v1;
            quadrilateral[1].normal = facet->normal;
            quadrilateral[1].vertex[0] = *v2;
            quadrilateral[1].vertex[1] = v2v0;
            quadrilateral[1].vertex[2] = v0v1;
            
            if (_z(*v0) > z) {
                if (upper != NULL) stl_add_facet(&upper->stl, &triangle);
                if (lower != NULL) {
                    stl_add_facet(&lower->stl, &quadrilateral[0]);
                    stl_add_facet(&lower->stl, &quadrilateral[1]);
                }
            } else {
                if (upper != NULL) {
                    stl_add_facet(&upper->stl, &quadrilateral[0]);
                    stl_add_facet(&upper->stl, &quadrilateral[1]);
                }
                if (lower != NULL) stl_add_facet(&lower->stl, &triangle);
            }
        }
    }
    
    // triangulate holes of upper mesh
    if (upper != NULL) {
        // compute shape of section
        ExPolygons section;
        this->make_expolygons_simple(upper_lines, &section);
        
        // triangulate section
        Polygons triangles;
        for (ExPolygons::const_iterator expolygon = section.begin(); expolygon != section.end(); ++expolygon)
            expolygon->triangulate_p2t(&triangles);
        
        // convert triangles to facets and append them to mesh
        for (Polygons::const_iterator polygon = triangles.begin(); polygon != triangles.end(); ++polygon) {
            Polygon p = *polygon;
            p.reverse();
            stl_facet facet;
            _x(facet.normal) = 0;
            _y(facet.normal) = 0;
            _z(facet.normal) = -1;
            for (size_t i = 0; i <= 2; ++i) {
                _x(facet.vertex[i]) = unscale(p.points[i].x);
                _y(facet.vertex[i]) = unscale(p.points[i].y);
                _z(facet.vertex[i]) = z;
            }
            stl_add_facet(&upper->stl, &facet);
        }
    }
    
    // triangulate holes of lower mesh
    if (lower != NULL) {
        // compute shape of section
        ExPolygons section;
        this->make_expolygons_simple(lower_lines, &section);
        
        // triangulate section
        Polygons triangles;
        for (ExPolygons::const_iterator expolygon = section.begin(); expolygon != section.end(); ++expolygon)
            expolygon->triangulate_p2t(&triangles);
        
        // convert triangles to facets and append them to mesh
        for (Polygons::const_iterator polygon = triangles.begin(); polygon != triangles.end(); ++polygon) {
            stl_facet facet;
            _x(facet.normal) = 0;
            _y(facet.normal) = 0;
            _z(facet.normal) = 1;
            for (size_t i = 0; i <= 2; ++i) {
                _x(facet.vertex[i]) = unscale(polygon->points[i].x);
                _y(facet.vertex[i]) = unscale(polygon->points[i].y);
                _z(facet.vertex[i]) = z;
            }
            stl_add_facet(&lower->stl, &facet);
        }
    }
    
    
    stl_get_size(&(upper->stl));
    stl_get_size(&(lower->stl));
}


template <Axis A>
TriangleMeshSlicer<A>::TriangleMeshSlicer(TriangleMesh* _mesh) : mesh(_mesh), v_scaled_shared(NULL)
{
    // build a table to map a facet_idx to its three edge indices
    this->mesh->require_shared_vertices();
    typedef std::pair<int,int>              t_edge;
    typedef std::vector<t_edge>             t_edges;  // edge_idx => a_id,b_id
    typedef std::map<t_edge,int>            t_edges_map;  // a_id,b_id => edge_idx
    
    this->facets_edges.resize(this->mesh->stl.stats.number_of_facets);
    
    {
        t_edges edges;
        // reserve() instead of resize() because otherwise we couldn't read .size() below to assign edge_idx
        edges.reserve(this->mesh->stl.stats.number_of_facets * 3);  // number of edges = number of facets * 3
        t_edges_map edges_map;
        for (int facet_idx = 0; facet_idx < this->mesh->stl.stats.number_of_facets; facet_idx++) {
            this->facets_edges[facet_idx].resize(3);
            for (int i = 0; i <= 2; i++) {
                int a_id = this->mesh->stl.v_indices[facet_idx].vertex[i];
                int b_id = this->mesh->stl.v_indices[facet_idx].vertex[(i+1) % 3];
                
                int edge_idx;
                t_edges_map::const_iterator my_edge = edges_map.find(std::make_pair(b_id,a_id));
                if (my_edge != edges_map.end()) {
                    edge_idx = my_edge->second;
                } else {
                    /* admesh can assign the same edge ID to more than two facets (which is 
                       still topologically correct), so we have to search for a duplicate of 
                       this edge too in case it was already seen in this orientation */
                    my_edge = edges_map.find(std::make_pair(a_id,b_id));
                    
                    if (my_edge != edges_map.end()) {
                        edge_idx = my_edge->second;
                    } else {
                        // edge isn't listed in table, so we insert it
                        edge_idx = edges.size();
                        edges.push_back(std::make_pair(a_id,b_id));
                        edges_map[ edges[edge_idx] ] = edge_idx;
                    }
                }
                this->facets_edges[facet_idx][i] = edge_idx;
                
                #ifdef SLIC3R_DEBUG
                printf("  [facet %d, edge %d] a_id = %d, b_id = %d   --> edge %d\n", facet_idx, i, a_id, b_id, edge_idx);
                #endif
            }
        }
    }
    
    // clone shared vertices coordinates and scale them
    this->v_scaled_shared = (stl_vertex*)calloc(this->mesh->stl.stats.shared_vertices, sizeof(stl_vertex));
    std::copy(this->mesh->stl.v_shared, this->mesh->stl.v_shared + this->mesh->stl.stats.shared_vertices, this->v_scaled_shared);
    for (int i = 0; i < this->mesh->stl.stats.shared_vertices; i++) {
        this->v_scaled_shared[i].x /= SCALING_FACTOR;
        this->v_scaled_shared[i].y /= SCALING_FACTOR;
        this->v_scaled_shared[i].z /= SCALING_FACTOR;
    }
}

template <Axis A>
TriangleMeshSlicer<A>::~TriangleMeshSlicer()
{
    free(this->v_scaled_shared);
}

template class TriangleMeshSlicer<X>;
template class TriangleMeshSlicer<Y>;
template class TriangleMeshSlicer<Z>;

}
