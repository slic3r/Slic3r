#include <catch.hpp>

#include "TriangleMesh.hpp"
#include "Point.hpp"

using namespace Slic3r;

SCENARIO( "TriangleMesh: Basic mesh statistics") {
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

SCENARIO( "TriangleMesh: split functionality.", "[!mayfail]") {
    REQUIRE(false); // TODO
}

SCENARIO( "TriangleMesh: slice behavior.", "[!mayfail]") {
    REQUIRE(false); // TODO
}

SCENARIO( "make_xxx functions produce meshes.", "[!mayfail]") {
    REQUIRE(false); // TODO
}

SCENARIO( "TriangleMesh: Mesh merge functions", "[!mayfail]") {
    REQUIRE(false); // TODO
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
