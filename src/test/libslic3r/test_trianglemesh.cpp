#include <catch.hpp>

#include "TriangleMesh.hpp"
#include "libslic3r.h"
#include "Point.hpp"
#include "test_options.hpp"
#include "Config.hpp"
#include "Model.hpp"
#include "test_data.hpp"

#include "Log.hpp"

#include <algorithm>
#include <future>
#include <chrono>

using namespace Slic3r;
using namespace std;

SCENARIO( "TriangleMesh: Basic mesh statistics") {
    GIVEN( "A 20mm cube, built from constexpr std::array" ) {
        constexpr std::array<Pointf3, 8> vertices { Pointf3(20,20,0), Pointf3(20,0,0), Pointf3(0,0,0), Pointf3(0,20,0), Pointf3(20,20,20), Pointf3(0,20,20), Pointf3(0,0,20), Pointf3(20,0,20) };
        constexpr std::array<Point3, 12> facets { Point3(0,1,2), Point3(0,2,3), Point3(4,5,6), Point3(4,6,7), Point3(0,4,7), Point3(0,7,1), Point3(1,7,6), Point3(1,6,2), Point3(2,6,5), Point3(2,5,3), Point3(4,0,3), Point3(4,3,5) };
        auto cube {TriangleMesh(vertices, facets)};
        cube.repair();
        
        THEN( "Volume is appropriate for 20mm square cube.") {
            REQUIRE(abs(cube.volume() - 20.0*20.0*20.0) < 1e-2);
        }

        THEN( "Vertices array matches input.") {
            for (auto i = 0U; i < cube.vertices().size(); i++) {
                REQUIRE(cube.vertices().at(i) == vertices.at(i));
            }
            for (auto i = 0U; i < vertices.size(); i++) {
                REQUIRE(vertices.at(i) == cube.vertices().at(i));
            }
        }
        THEN( "Vertex count matches vertex array size.") {
            REQUIRE(cube.facets_count() == facets.size());
        }

        THEN( "Facet array matches input.") {
            for (auto i = 0U; i < cube.facets().size(); i++) {
                REQUIRE(cube.facets().at(i) == facets.at(i));
            }

            for (auto i = 0U; i < facets.size(); i++) {
                REQUIRE(facets.at(i) == cube.facets().at(i));
            }
        }
        THEN( "Facet count matches facet array size.") {
            REQUIRE(cube.facets_count() == facets.size());
        }

        THEN( "Number of normals is equal to the number of facets.") {
            REQUIRE(cube.normals().size() == facets.size());
        }

        THEN( "center() returns the center of the object.") {
            REQUIRE(cube.center() == Pointf3(10.0,10.0,10.0));
        }

        THEN( "Size of cube is (20,20,20)") {
            REQUIRE(cube.size() == Pointf3(20,20,20));
        }

    }
    GIVEN( "A 20mm cube with one corner on the origin") {
        const Pointf3s vertices { Pointf3(20,20,0), Pointf3(20,0,0), Pointf3(0,0,0), Pointf3(0,20,0), Pointf3(20,20,20), Pointf3(0,20,20), Pointf3(0,0,20), Pointf3(20,0,20) };
        const Point3s facets { Point3(0,1,2), Point3(0,2,3), Point3(4,5,6), Point3(4,6,7), Point3(0,4,7), Point3(0,7,1), Point3(1,7,6), Point3(1,6,2), Point3(2,6,5), Point3(2,5,3), Point3(4,0,3), Point3(4,3,5) };

        auto cube {TriangleMesh(vertices, facets)};
        cube.repair();

        THEN( "Volume is appropriate for 20mm square cube.") {
            REQUIRE(abs(cube.volume() - 20.0*20.0*20.0) < 1e-2);
        }

        THEN( "Vertices array matches input.") {
            for (auto i = 0U; i < cube.vertices().size(); i++) {
                REQUIRE(cube.vertices().at(i) == vertices.at(i));
            }
            for (auto i = 0U; i < vertices.size(); i++) {
                REQUIRE(vertices.at(i) == cube.vertices().at(i));
            }
        }
        THEN( "Vertex count matches vertex array size.") {
            REQUIRE(cube.facets_count() == facets.size());
        }

        THEN( "Facet array matches input.") {
            for (auto i = 0U; i < cube.facets().size(); i++) {
                REQUIRE(cube.facets().at(i) == facets.at(i));
            }

            for (auto i = 0U; i < facets.size(); i++) {
                REQUIRE(facets.at(i) == cube.facets().at(i));
            }
        }
        THEN( "Facet count matches facet array size.") {
            REQUIRE(cube.facets_count() == facets.size());
        }

        THEN( "Number of normals is equal to the number of facets.") {
            REQUIRE(cube.normals().size() == facets.size());
        }

        THEN( "center() returns the center of the object.") {
            REQUIRE(cube.center() == Pointf3(10.0,10.0,10.0));
        }

        THEN( "Size of cube is (20,20,20)") {
            REQUIRE(cube.size() == Pointf3(20,20,20));
        }
    }
}

SCENARIO( "TriangleMesh: Transformation functions affect mesh as expected.") {
    GIVEN( "A 20mm cube with one corner on the origin") {
        const Pointf3s vertices { Pointf3(20,20,0), Pointf3(20,0,0), Pointf3(0,0,0), Pointf3(0,20,0), Pointf3(20,20,20), Pointf3(0,20,20), Pointf3(0,0,20), Pointf3(20,0,20) };
        const std::vector<Point3> facets { Point3(0,1,2), Point3(0,2,3), Point3(4,5,6), Point3(4,6,7), Point3(0,4,7), Point3(0,7,1), Point3(1,7,6), Point3(1,6,2), Point3(2,6,5), Point3(2,5,3), Point3(4,0,3), Point3(4,3,5) };
        auto cube {TriangleMesh(vertices, facets)};
        cube.repair();

        WHEN( "The cube is scaled 200\% uniformly") {
            cube.scale(2.0);
            THEN( "The volume is equivalent to 40x40x40 (all dimensions increased by 200\%") {
                REQUIRE(abs(cube.volume() - 40.0*40.0*40.0) < 1e-2);
            }
        }
        WHEN( "The resulting cube is scaled 200\% in the X direction") {
            cube.scale(Vectorf3(2.0, 1, 1));
            THEN( "The volume is doubled.") {
                REQUIRE(abs(cube.volume() - 2*20.0*20.0*20.0) < 1e-2);
            }
            THEN( "The X coordinate size is 200\%.") {
                REQUIRE(cube.vertices().at(0).x == 40.0);
            }
        }

        WHEN( "The cube is scaled 25\% in the X direction") {
            cube.scale(Vectorf3(0.25, 1, 1));
            THEN( "The volume is 25\% of the previous volume.") {
                REQUIRE(abs(cube.volume() - 0.25*20.0*20.0*20.0) < 1e-2);
            }
            THEN( "The X coordinate size is 25\% from previous.") {
                REQUIRE(cube.vertices().at(0).x == 5.0);
            }
        }

        WHEN( "The cube is rotated 45 degrees.") {
            cube.rotate(45.0, Slic3r::Point(20,20));
            THEN( "The X component of the size is sqrt(2)*20") {
                REQUIRE(abs(cube.size().x - sqrt(2.0)*20) < 1e-2);
            }
        }

        WHEN( "The cube is translated (5, 10, 0) units with a Pointf3 ") {
            cube.translate(Pointf3(5.0, 10.0, 0.0));
            THEN( "The first vertex is located at 25, 30, 0") {
                REQUIRE(cube.vertices().at(0) == Pointf3(25.0, 30.0, 0.0));
            }
        }

        WHEN( "The cube is translated (5, 10, 0) units with 3 doubles") {
            cube.translate(5.0, 10.0, 0.0);
            THEN( "The first vertex is located at 25, 30, 0") {
                REQUIRE(cube.vertices().at(0) == Pointf3(25.0, 30.0, 0.0));
            }
        }
        WHEN( "The cube is translated (5, 10, 0) units and then aligned to origin") {
            cube.translate(5.0, 10.0, 0.0);
            cube.align_to_origin();
            THEN( "The third vertex is located at 0,0,0") {
                REQUIRE(cube.vertices().at(2) == Pointf3(0.0, 0.0, 0.0));
            }
        }
    }
}

SCENARIO( "TriangleMesh: slice behavior.") {
    GIVEN( "A 20mm cube with one corner on the origin") {
        const Pointf3s vertices { Pointf3(20,20,0), Pointf3(20,0,0), Pointf3(0,0,0), Pointf3(0,20,0), Pointf3(20,20,20), Pointf3(0,20,20), Pointf3(0,0,20), Pointf3(20,0,20) };
        const std::vector<Point3> facets { Point3(0,1,2), Point3(0,2,3), Point3(4,5,6), Point3(4,6,7), Point3(0,4,7), Point3(0,7,1), Point3(1,7,6), Point3(1,6,2), Point3(2,6,5), Point3(2,5,3), Point3(4,0,3), Point3(4,3,5) };
        auto cube {TriangleMesh(vertices, facets)};
        cube.repair();

        WHEN("Cube is sliced with z = [0,2,4,8,6,8,10,12,14,16,18,20]") {
            std::vector<double> z { 0,2,4,8,6,8,10,12,14,16,18,20 };
            auto result {cube.slice(z)};
            THEN( "The correct number of polygons are returned per layer.") {
                for (auto i = 0U; i < z.size(); i++) {
                    REQUIRE(result.at(i).size() == 1);
                }
            }
            THEN( "The area of the returned polygons is correct.") {
                for (auto i = 0U; i < z.size(); i++) {
                    REQUIRE(result.at(i).at(0).area() == 20.0*20/(std::pow(SCALING_FACTOR,2)));
                }
            }
        }
    }
    GIVEN( "A STL with an irregular shape.") {
        const Pointf3s vertices {Pointf3(0,0,0),Pointf3(0,0,20),Pointf3(0,5,0),Pointf3(0,5,20),Pointf3(50,0,0),Pointf3(50,0,20),Pointf3(15,5,0),Pointf3(35,5,0),Pointf3(15,20,0),Pointf3(50,5,0),Pointf3(35,20,0),Pointf3(15,5,10),Pointf3(50,5,20),Pointf3(35,5,10),Pointf3(35,20,10),Pointf3(15,20,10)};
        const Point3s facets {Point3(0,1,2),Point3(2,1,3),Point3(1,0,4),Point3(5,1,4),Point3(0,2,4),Point3(4,2,6),Point3(7,6,8),Point3(4,6,7),Point3(9,4,7),Point3(7,8,10),Point3(2,3,6),Point3(11,3,12),Point3(7,12,9),Point3(13,12,7),Point3(6,3,11),Point3(11,12,13),Point3(3,1,5),Point3(12,3,5),Point3(5,4,9),Point3(12,5,9),Point3(13,7,10),Point3(14,13,10),Point3(8,15,10),Point3(10,15,14),Point3(6,11,8),Point3(8,11,15),Point3(15,11,13),Point3(14,15,13)};

        auto cube {TriangleMesh(vertices, facets)};
        cube.repair();
        WHEN(" a top tangent plane is sliced") {
            auto slices {cube.slice({5.0, 10.0})};
            THEN( "its area is included") {
                REQUIRE(slices.at(0).at(0).area() > 0);
                REQUIRE(slices.at(1).at(0).area() > 0);
            }
        }
        WHEN(" a model that has been transformed is sliced") {
            cube.mirror_z();
            auto slices {cube.slice({-5.0, -10.0})};
            THEN( "it is sliced properly (mirrored bottom plane area is included)") {
                REQUIRE(slices.at(0).at(0).area() > 0);
                REQUIRE(slices.at(1).at(0).area() > 0);
            }
        }
    }
}

SCENARIO( "make_xxx functions produce meshes.") {
    GIVEN("make_cube() function") {
        WHEN("make_cube() is called with arguments 20,20,20") {
            auto cube {TriangleMesh::make_cube(20,20,20)};
            THEN("The resulting mesh has one and only one vertex at 0,0,0") {
                auto verts {cube.vertices()};
                REQUIRE(std::count_if(verts.begin(), verts.end(), [](Pointf3& t) { return t.x == 0 && t.y == 0 && t.z == 0; } ) == 1);
            }
            THEN("The mesh volume is 20*20*20") {
                REQUIRE(abs(cube.volume() - 20.0*20.0*20.0) < 1e-2);
            }
            THEN("The resulting mesh is in the repaired state.") {
                REQUIRE(cube.repaired == true);
            }
            THEN("There are 12 facets.") {
                REQUIRE(cube.facets().size() == 12);
            }
        }
    }
    GIVEN("make_cylinder() function") {
        WHEN("make_cylinder() is called with arguments 10,10, PI / 3") {
            auto cyl {TriangleMesh::make_cylinder(10, 10, PI / 243.0)};
            double angle = (2*PI / floor(2*PI / (PI / 243.0)));
            THEN("The resulting mesh has one and only one vertex at 0,0,0") {
                auto verts {cyl.vertices()};
                REQUIRE(std::count_if(verts.begin(), verts.end(), [](Pointf3& t) { return t.x == 0 && t.y == 0 && t.z == 0; } ) == 1);
            }
            THEN("The resulting mesh has one and only one vertex at 0,0,10") {
                auto verts {cyl.vertices()};
                REQUIRE(std::count_if(verts.begin(), verts.end(), [](Pointf3& t) { return t.x == 0 && t.y == 0 && t.z == 10; } ) == 1);
            }
            THEN("Resulting mesh has 2 + (2*PI/angle * 2) vertices.") { 
                REQUIRE(cyl.vertices().size() == (2 + ((2*PI/angle)*2)));
            }
            THEN("Resulting mesh has 2*PI/angle * 4 facets") {
                REQUIRE(cyl.facets().size() == (2*PI/angle)*4);
            }
            THEN("The resulting mesh is in the repaired state.") {
                REQUIRE(cyl.repaired == true);
            }
            THEN( "The mesh volume is approximately 10pi * 10^2") {
                REQUIRE(abs(cyl.volume() - (10.0 * M_PI * std::pow(10,2))) < 1);
            }
        }
    }

    GIVEN("make_sphere() function") {
        WHEN("make_sphere() is called with arguments 10, PI / 3") {
            auto sph {TriangleMesh::make_sphere(10, PI / 243.0)};
            double angle = (2.0*PI / floor(2.0*PI / (PI / 243.0)));
            THEN("Resulting mesh has one point at 0,0,-10 and one at 0,0,10") {
                auto verts {sph.vertices()};
                REQUIRE(std::count_if(verts.begin(), verts.end(), [](Pointf3& t) { return t.x == 0 && t.y == 0 && t.z == 10; } ) == 1);
                REQUIRE(std::count_if(verts.begin(), verts.end(), [](Pointf3& t) { return t.x == 0 && t.y == 0 && t.z == -10; } ) == 1);
            }
            THEN("The resulting mesh is in the repaired state.") {
                REQUIRE(sph.repaired == true);
            }
            THEN( "The mesh volume is approximately 4/3 * pi * 10^3") {
                REQUIRE(abs(sph.volume() - (4.0/3.0 * M_PI * std::pow(10,3))) < 1); // 1% tolerance?
            }
        }
    }
}

SCENARIO( "TriangleMesh: split functionality.") {
    GIVEN( "A 20mm cube with one corner on the origin") {
        const Pointf3s vertices { Pointf3(20,20,0), Pointf3(20,0,0), Pointf3(0,0,0), Pointf3(0,20,0), Pointf3(20,20,20), Pointf3(0,20,20), Pointf3(0,0,20), Pointf3(20,0,20) };
        const Point3s facets { Point3(0,1,2), Point3(0,2,3), Point3(4,5,6), Point3(4,6,7), Point3(0,4,7), Point3(0,7,1), Point3(1,7,6), Point3(1,6,2), Point3(2,6,5), Point3(2,5,3), Point3(4,0,3), Point3(4,3,5) };

        auto cube {TriangleMesh(vertices, facets)};
        cube.repair();
        WHEN( "The mesh is split into its component parts.") {
            auto meshes {cube.split()};
            THEN(" The bounding box statistics are propagated to the split copies") {
                REQUIRE(meshes.size() == 1);
                REQUIRE(meshes.at(0)->bb3() == cube.bb3());
            }
        }
    }
    GIVEN( "Two 20mm cubes, each with one corner on the origin, merged into a single TriangleMesh") {
        const Pointf3s vertices { Pointf3(20,20,0), Pointf3(20,0,0), Pointf3(0,0,0), Pointf3(0,20,0), Pointf3(20,20,20), Pointf3(0,20,20), Pointf3(0,0,20), Pointf3(20,0,20) };
        const Point3s facets { Point3(0,1,2), Point3(0,2,3), Point3(4,5,6), Point3(4,6,7), Point3(0,4,7), Point3(0,7,1), Point3(1,7,6), Point3(1,6,2), Point3(2,6,5), Point3(2,5,3), Point3(4,0,3), Point3(4,3,5) };

        auto cube {TriangleMesh(vertices, facets)};
        cube.repair();
        auto cube2 {TriangleMesh(vertices, facets)};
        cube2.repair();

        cube.merge(cube2);
        cube.repair();
        WHEN( "The combined mesh is split") {
            auto meshes {cube.split()};
            THEN( "Two meshes are in the output vector.") {
                REQUIRE(meshes.size() == 2);
            }
        }
    }
}

SCENARIO( "TriangleMesh: Mesh merge functions") {
    GIVEN( "Two 20mm cubes, each with one corner on the origin") {
        const Pointf3s vertices { Pointf3(20,20,0), Pointf3(20,0,0), Pointf3(0,0,0), Pointf3(0,20,0), Pointf3(20,20,20), Pointf3(0,20,20), Pointf3(0,0,20), Pointf3(20,0,20) };
        const Point3s facets { Point3(0,1,2), Point3(0,2,3), Point3(4,5,6), Point3(4,6,7), Point3(0,4,7), Point3(0,7,1), Point3(1,7,6), Point3(1,6,2), Point3(2,6,5), Point3(2,5,3), Point3(4,0,3), Point3(4,3,5) };

        auto cube {TriangleMesh(vertices, facets)};
        cube.repair();
        auto cube2 {TriangleMesh(vertices, facets)};
        cube2.repair();

        WHEN( "The two meshes are merged") {
            cube.merge(cube2);
            cube.repair();
            THEN( "There are twice as many facets in the merged mesh as the original.") {
                REQUIRE(cube.stats().number_of_facets == 2 * cube2.stats().number_of_facets);
            }
        }
    }
}

SCENARIO( "TriangleMeshSlicer: Cut behavior.") {
    GIVEN( "A 20mm cube with one corner on the origin") {
        const Pointf3s vertices { Pointf3(20,20,0), Pointf3(20,0,0), Pointf3(0,0,0), Pointf3(0,20,0), Pointf3(20,20,20), Pointf3(0,20,20), Pointf3(0,0,20), Pointf3(20,0,20) };
        const Point3s facets { Point3(0,1,2), Point3(0,2,3), Point3(4,5,6), Point3(4,6,7), Point3(0,4,7), Point3(0,7,1), Point3(1,7,6), Point3(1,6,2), Point3(2,6,5), Point3(2,5,3), Point3(4,0,3), Point3(4,3,5) };

        auto cube {TriangleMesh(vertices, facets)};
        cube.repair();
        WHEN( "Object is cut at the bottom") {
            TriangleMesh upper {};
            TriangleMesh lower {};
            cube.cut(Z, 0, &upper, &lower);
            THEN("Upper mesh has all facets except those belonging to the slicing plane.") {
                REQUIRE(upper.facets_count() == 12);
            }
            THEN("Lower mesh has no facets.") {
                REQUIRE(lower.facets_count() == 0);
            }
        }
        WHEN( "Object is cut at the center") {
            TriangleMesh upper {};
            TriangleMesh lower {};
            cube.cut(Z, 10, &upper, &lower);
            THEN("Upper mesh has 2 external horizontal facets, 3 facets on each side, and 6 facets on the triangulated side (2 + 12 + 6).") {
                REQUIRE(upper.facets_count() == 2+12+6);
            }
            THEN("Lower mesh has 2 external horizontal facets, 3 facets on each side, and 6 facets on the triangulated side (2 + 12 + 6).") {
                REQUIRE(lower.facets_count() == 2+12+6);
            }
        }
    }
}
#ifdef TEST_PERFORMANCE
TEST_CASE("Regression test for issue #4486 - files take forever to slice") {
    TriangleMesh mesh;
    auto config {Slic3r::Config::new_from_defaults()};
    mesh.ReadSTLFile(std::string(testfile_dir) + "test_trianglemesh/4486/100_000.stl");
    mesh.repair();

    config->set("layer_height", 500);
    config->set("first_layer_height", 250);
    config->set("nozzle_diameter", 500);

    Slic3r::Model model;
    auto print {Slic3r::Test::init_print({mesh}, model, config)};

    print->status_cb = [] (int ln, const std::string& msg) { Slic3r::Log::info("Print") << ln << " " << msg << "\n";};

    std::future<void> fut = std::async([&print] () { print->process(); });
    std::chrono::milliseconds span {120000};
    bool timedout {false};
    if(fut.wait_for(span) == std::future_status::timeout) {
        timedout = true;
    }
    REQUIRE(timedout == false);

}
#endif // TEST_PERFORMANCE

#ifdef BUILD_PROFILE
TEST_CASE("Profile test for issue #4486 - files take forever to slice") {
    TriangleMesh mesh;
    auto config {Slic3r::Config::new_from_defaults()};
    mesh.ReadSTLFile(std::string(testfile_dir) + "test_trianglemesh/4486/10_000.stl");
    mesh.repair();

    config->set("layer_height", 500);
    config->set("first_layer_height", 250);
    config->set("nozzle_diameter", 500);
    config->set("fill_density", "5%");

    Slic3r::Model model;
    auto print {Slic3r::Test::init_print({mesh}, model, config)};

    print->status_cb = [] (int ln, const std::string& msg) { Slic3r::Log::info("Print") << ln << " " << msg << "\n";};

    print->process();

    REQUIRE(true);

}
#endif //BUILD_PROFILE
