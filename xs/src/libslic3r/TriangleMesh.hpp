#ifndef slic3r_TriangleMesh_hpp_
#define slic3r_TriangleMesh_hpp_

#include "libslic3r.h"
#include <admesh/stl.h>
#include <vector>
#include <boost/thread.hpp>
#include "BoundingBox.hpp"
#include "Line.hpp"
#include "Point.hpp"
#include "Polygon.hpp"
#include "ExPolygon.hpp"
#include "TransformationMatrix.hpp"

namespace Slic3r {

class TriangleMesh;
template <Axis A> class TriangleMeshSlicer;
typedef std::vector<TriangleMesh*> TriangleMeshPtrs;


/// Interface to available statistics from the underlying mesh. 
struct mesh_stats {
    size_t number_of_facets {0};
    size_t number_of_parts {0};
    double volume {0};
    size_t degenerate_facets {0};
    size_t edges_fixed {0};
    size_t facets_removed {0};
    size_t facets_added {0};
    size_t facets_reversed {0};
    size_t backwards_edges {0};
    size_t normals_fixed {0};
};

class TriangleMesh
{
    public:
    TriangleMesh();

    /// Templated constructor to adapt containers that offer .data() and .size()
    /// First argument is a container (either vector or array) of Pointf3 for the vertex data.
    /// Second argument is container of facets (currently Point3).
    template <typename Vertex_Cont, typename Facet_Cont>
    TriangleMesh(const Vertex_Cont& vertices, const Facet_Cont& facets) : TriangleMesh(vertices.data(), facets.data(), facets.size()) {}

    TriangleMesh(const TriangleMesh &other);
    ///  copy assignment
    TriangleMesh& operator= (const TriangleMesh& other);

    /// Move assignment
    TriangleMesh& operator= (TriangleMesh&& other);
    TriangleMesh(TriangleMesh&& other);

    void swap(TriangleMesh &other);
    ~TriangleMesh();
    void ReadSTLFile(const std::string &input_file);
    void write_ascii(const std::string &output_file) const;
    void write_binary(const std::string &output_file) const;
    void repair();
    void check_topology();
    float volume();
    bool is_manifold() const;
    void WriteOBJFile(const std::string &output_file) const;

    /// Direct manipulators

    void scale(float factor);
    void scale(const Pointf3 &versor);

    /// Translate the mesh to a new location.
    void translate(float x, float y, float z);

    /// Translate the mesh to a new location.
    void translate(Pointf3 vec);


    void rotate(float angle, const Axis &axis);
    void rotate_x(float angle);
    void rotate_y(float angle);
    void rotate_z(float angle);
    void mirror(const Axis &axis);
    void mirror_x();
    void mirror_y();
    void mirror_z();
    void align_to_origin();
    void center_around_origin();

    /// Rotate angle around a specified point.
    void rotate(double angle, const Point& center);
    void rotate(double angle, Point* center);


    void align_to_bed();


    /// Matrix manipulators
    TriangleMesh get_transformed_mesh(TransformationMatrix const & trafo) const;
    void transform(TransformationMatrix const & trafo);


    TriangleMeshPtrs split() const;
    TriangleMeshPtrs cut_by_grid(const Pointf &grid) const;
    void merge(const TriangleMesh &mesh);
    ExPolygons horizontal_projection() const;
    Polygon convex_hull();
    BoundingBoxf3 bounding_box() const;
    BoundingBoxf3 get_transformed_bounding_box(TransformationMatrix const & trafo) const;
    void reset_repair_stats();
    bool needed_repair() const;
    size_t facets_count() const;
    void extrude_tin(float offset);
    void require_shared_vertices();
    void reverse_normals();
    
    /// Return a copy of the vertex array defining this mesh.
    Pointf3s vertices();

    /// Return a copy of the facet array defining this mesh.
    Point3s facets();

    /// Return a copy of the normals array defining this mesh.
    Pointf3s normals() const;

    /// Return the size of the mesh in coordinates.
    Pointf3 size() const;

    /// Return the center of the related bounding box.
    Pointf3 center() const;

    /// Slice this mesh at the provided Z levels and return the vector
    std::vector<ExPolygons> slice(const std::vector<double>& z);

    /// Contains general statistics from underlying mesh structure.
    mesh_stats stats() const;

    BoundingBoxf3 bb3() const;

    /// Perform a cut of the mesh and put the output in upper and lower
    void cut(Axis axis, double z, TriangleMesh* upper, TriangleMesh* lower);
	
	/// Generate a mesh representing a cube with dimensions (x, y, z), with one corner at (0,0,0).
    static TriangleMesh make_cube(double x, double y, double z);
	
	/// Generate a mesh representing a cylinder of radius r and height h, with the base at (0,0,0). 
	/// param[in] r Radius 
	/// param[in] h Height 
	/// param[in] fa Facet angle. A smaller angle produces more facets. Default value is 2pi / 360.  
    static TriangleMesh make_cylinder(double r, double h, double fa=(2*PI/360));
	
	/// Generate a mesh representing a sphere of radius rho, centered about (0,0,0). 
	/// param[in] rho Distance from center to the shell of the sphere. 
	/// param[in] fa Facet angle. A smaller angle produces more facets. Default value is 2pi / 360.  
    static TriangleMesh make_sphere(double rho, double fa=(2*PI/360));

    
    stl_file stl;
	/// Whether or not this mesh has been repaired.
    bool repaired;
    
    private:

    /// Private constructor that is called from the public sphere. 
    /// It doesn't do any bounds checking on points and operates on raw pointers, so we hide it. 
    /// Other constructors can call this one!
    TriangleMesh(const Pointf3* points, const Point3* facets, size_t n_facets); 

    /// Perform the mechanics of a stl copy
    void clone(const TriangleMesh& other);

    friend class TriangleMeshSlicer<X>;
    friend class TriangleMeshSlicer<Y>;
    friend class TriangleMeshSlicer<Z>;
};

enum FacetEdgeType { feNone, feTop, feBottom, feHorizontal };

class IntersectionPoint : public Point
{
    public:
    int point_id;
    int edge_id;
    IntersectionPoint() : point_id(-1), edge_id(-1) {};
};

class IntersectionLine : public Line
{
    public:
    int             a_id;
    int             b_id;
    int             edge_a_id;
    int             edge_b_id;
    FacetEdgeType   edge_type;
    bool            skip;
    IntersectionLine() : a_id(-1), b_id(-1), edge_a_id(-1), edge_b_id(-1), edge_type(feNone), skip(false) {};
};
typedef std::vector<IntersectionLine> IntersectionLines;
typedef std::vector<IntersectionLine*> IntersectionLinePtrs;


/// \brief Class for processing TriangleMesh objects. 
template <Axis A>
class TriangleMeshSlicer
{
    public:
    TriangleMesh* mesh;
    TriangleMeshSlicer(TriangleMesh* _mesh);
    ~TriangleMeshSlicer();
    void slice(const std::vector<float> &z, std::vector<Polygons>* layers) const;
    void slice(const std::vector<float> &z, std::vector<ExPolygons>* layers) const;
    void slice(float z, ExPolygons* slices) const;
    void slice_facet(float slice_z, const stl_facet &facet, const int &facet_idx,
        const float &min_z, const float &max_z, std::vector<IntersectionLine>* lines,
        boost::mutex* lines_mutex = NULL) const;
    
	/// \brief Splits the current mesh into two parts.
	/// \param[in] z Coordinate plane to cut along.
	/// \param[out] upper TriangleMesh object to add the mesh > z. NULL suppresses saving this.
	/// \param[out] lower TriangleMesh object to save the mesh < z. NULL suppresses saving this.
    void cut(float z, TriangleMesh* upper, TriangleMesh* lower) const;
    
    private:
    typedef std::vector< std::vector<int> > t_facets_edges;
    t_facets_edges facets_edges;
    stl_vertex* v_scaled_shared;
    void _slice_do(size_t facet_idx, std::vector<IntersectionLines>* lines, boost::mutex* lines_mutex, const std::vector<float> &z) const;
    void _make_loops_do(size_t i, std::vector<IntersectionLines>* lines, std::vector<Polygons>* layers) const;
    void make_loops(std::vector<IntersectionLine> &lines, Polygons* loops) const;
    void make_expolygons(const Polygons &loops, ExPolygons* slices) const;
    void make_expolygons_simple(std::vector<IntersectionLine> &lines, ExPolygons* slices) const;
    void make_expolygons(std::vector<IntersectionLine> &lines, ExPolygons* slices) const;
    
    float& _x(stl_vertex &vertex) const;
    float& _y(stl_vertex &vertex) const;
    float& _z(stl_vertex &vertex) const;
    const float& _x(stl_vertex const &vertex) const;
    const float& _y(stl_vertex const &vertex) const;
    const float& _z(stl_vertex const &vertex) const;
};

template<> inline float& TriangleMeshSlicer<X>::_x(stl_vertex &vertex) const { return vertex.y; }
template<> inline float& TriangleMeshSlicer<X>::_y(stl_vertex &vertex) const { return vertex.z; }
template<> inline float& TriangleMeshSlicer<X>::_z(stl_vertex &vertex) const { return vertex.x; }
template<> inline float const& TriangleMeshSlicer<X>::_x(stl_vertex const &vertex) const { return vertex.y; }
template<> inline float const& TriangleMeshSlicer<X>::_y(stl_vertex const &vertex) const { return vertex.z; }
template<> inline float const& TriangleMeshSlicer<X>::_z(stl_vertex const &vertex) const { return vertex.x; }

template<> inline float& TriangleMeshSlicer<Y>::_x(stl_vertex &vertex) const { return vertex.z; }
template<> inline float& TriangleMeshSlicer<Y>::_y(stl_vertex &vertex) const { return vertex.x; }
template<> inline float& TriangleMeshSlicer<Y>::_z(stl_vertex &vertex) const { return vertex.y; }
template<> inline float const& TriangleMeshSlicer<Y>::_x(stl_vertex const &vertex) const { return vertex.z; }
template<> inline float const& TriangleMeshSlicer<Y>::_y(stl_vertex const &vertex) const { return vertex.x; }
template<> inline float const& TriangleMeshSlicer<Y>::_z(stl_vertex const &vertex) const { return vertex.y; }

template<> inline float& TriangleMeshSlicer<Z>::_x(stl_vertex &vertex) const { return vertex.x; }
template<> inline float& TriangleMeshSlicer<Z>::_y(stl_vertex &vertex) const { return vertex.y; }
template<> inline float& TriangleMeshSlicer<Z>::_z(stl_vertex &vertex) const { return vertex.z; }
template<> inline float const& TriangleMeshSlicer<Z>::_x(stl_vertex const &vertex) const { return vertex.x; }
template<> inline float const& TriangleMeshSlicer<Z>::_y(stl_vertex const &vertex) const { return vertex.y; }
template<> inline float const& TriangleMeshSlicer<Z>::_z(stl_vertex const &vertex) const { return vertex.z; }

}

#endif
