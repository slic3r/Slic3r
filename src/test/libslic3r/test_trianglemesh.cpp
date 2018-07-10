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

        WHEN( "The scaled cube is rotated 45 degrees.") {
            cube.rotate(45.0, Slic3r::Point(20,20));
            THEN( "The X component of the size is sqrt(2)*20") {
                REQUIRE(abs(cube.size().x - sqrt(2.0)*20) < 1e-2);
            }
        }
    }
}

SCENARIO( "TriangleMesh: split functionality.", "[!mayfail]") {
    REQUIRE(false); // TODO
}

SCENARIO( "make_xxx functions produce meshes.", "[!mayfail]") {
    REQUIRE(false); // TODO
}
