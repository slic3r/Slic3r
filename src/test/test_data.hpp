#ifndef SLIC3R_TEST_DATA_HPP
#include "Point.hpp"
#include "TriangleMesh.hpp"
#include "Geometry.hpp"

namespace Slic3r { namespace Test {

enum class TestMesh {
    A,
    L,
    V,
    _40x10,
    bridge,
    bridge_with_hole,
    cube_with_concave_hole,
    cube_with_hole,
    gt2_teeth,
    ipadstand,
    overhang,
    pyramid,
    sloping_hole,
    slopy_cube,
    small_dorito,
    step,
    two_hollow_squares
};

/// Port of Slic3r::Test::mesh
/// Basic cubes/boxes should call TriangleMesh::make_cube() directly and rescale/translate it
TriangleMesh mesh(TestMesh m);

TriangleMesh mesh(TestMesh m, Pointf3 translate, Pointf3 scale = Pointf3(1.0, 1.0, 1.0));

/// Templated function to see if two values are equivalent (+/- epsilon)
template <typename T, typename U>
bool _equiv(T a, U b) { return abs(a - b) < Slic3r::Geometry::epsilon; }

} } // namespace Slic3r::Test

#endif // SLIC3R_TEST_DATA_HPP
