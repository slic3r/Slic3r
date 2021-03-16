
#include <catch.hpp>

#include "Point.hpp"
#include "Polygon.hpp"


using namespace Slic3r;


// This test currently only covers remove_collinear_points.
// All remaining tests are to be ported from xs/t/06_polygon.t

Points collinear_circle({
    Point::new_scale(0, 0), // 3 collinear points at beginning
    Point::new_scale(10, 0),
    Point::new_scale(20, 0),
    Point::new_scale(30, 10),
    Point::new_scale(40, 20), // 2 collinear points
    Point::new_scale(40, 30),
    Point::new_scale(30, 40), // 3 collinear points
    Point::new_scale(20, 40),
    Point::new_scale(10, 40),
    Point::new_scale(-10, 20),
    Point::new_scale(-20, 10),
    Point::new_scale(-20, 0), // 3 collinear points at end
    Point::new_scale(-10, 0),
    Point::new_scale(-5, 0)
});


SCENARIO("Remove collinear points from Polygon") {
    GIVEN("Polygon with collinear points"){
        Polygon p(collinear_circle);
        WHEN("collinear points are removed") {
            p.remove_collinear_points();
            THEN("Leading collinear points are removed") {
                REQUIRE(p.points.front() == Point::new_scale(20, 0));
            }
            THEN("Trailing collinear points are removed") {
                REQUIRE(p.points.back() == Point::new_scale(-20, 0));
            }
            THEN("Number of remaining points is correct") {
                REQUIRE(p.points.size() == 7);
            }
        }
    }
}