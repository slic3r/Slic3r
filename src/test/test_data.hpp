#ifndef SLIC3R_TEST_DATA_HPP
#include "Point.hpp"
#include "TriangleMesh.hpp"

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

/// Port of Slic3r::Test::Mesh
/// Basic cubes/boxes should call TriangleMesh::make_cube() directly and rescale it
TriangleMesh mesh(TestMesh m);

TriangleMesh mesh(TestMesh m, Pointf3 translate, Pointf3 scale = Pointf3(1.0, 1.0, 1.0));

} } // namespace Slic3r::Test

#endif // SLIC3R_TEST_DATA_HPP
