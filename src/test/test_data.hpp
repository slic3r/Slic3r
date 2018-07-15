#ifndef SLIC3R_TEST_DATA_HPP
#include "Point.hpp"
#include "TriangleMesh.hpp"
#include "Geometry.hpp"
#include "Model.hpp"
#include "Print.hpp"
#include "Config.hpp"

#include <unordered_map>

namespace Slic3r { namespace Test {

/// Enumeration of test meshes
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

class Print {
public:
    Print(std::shared_ptr<Slic3r::Print> _print, std::vector<Slic3r::Model> _models) : models(_models), _print(_print) {}
    void process() { _print->process(); }
    void apply_config(config_ptr _config) { _print->apply_config(_config); }
    Slic3r::Print& print() { return *(_print); }

    const std::vector<Slic3r::Model> models {};
private:
    std::shared_ptr<Slic3r::Print> _print {nullptr};
};


/// Mesh enumeration to name mapping
extern const std::unordered_map<TestMesh, const char*> mesh_names;

/// Port of Slic3r::Test::mesh
/// Basic cubes/boxes should call TriangleMesh::make_cube() directly and rescale/translate it
TriangleMesh mesh(TestMesh m);

TriangleMesh mesh(TestMesh m, Pointf3 translate, Pointf3 scale = Pointf3(1.0, 1.0, 1.0));
TriangleMesh mesh(TestMesh m, Pointf3 translate, double scale = 1.0);

/// Templated function to see if two values are equivalent (+/- epsilon)
template <typename T, typename U>
bool _equiv(const T& a, const U& b) { return abs(a - b) < Slic3r::Geometry::epsilon; }

template <typename T, typename U>
bool _equiv(const T& a, const U& b, double epsilon) { return abs(a - b) < epsilon; }

//Slic3r::Model model(const std::string& model_name, TestMesh m, Pointf3 translate = Pointf3(0,0,0), Pointf3 scale = Pointf3(1.0,1.0,1.0));
//Slic3r::Model model(const std::string& model_name, TestMesh m, Pointf3 translate = Pointf3(0,0,0), double scale = 1.0);

Slic3r::Model model(const std::string& model_name, TriangleMesh&& _mesh);

Slic3r::Test::Print init_print(std::tuple<int,int,int> cube, config_ptr _config = Slic3r::Config::new_from_defaults());

void gcode(std::stringstream& gcode, Slic3r::Test::Print& print);

} } // namespace Slic3r::Test


#endif // SLIC3R_TEST_DATA_HPP
