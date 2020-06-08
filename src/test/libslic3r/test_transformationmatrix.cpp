#include <catch.hpp>

#include <vector>

#include "libslic3r.h"
#include "TransformationMatrix.hpp"

#include "Log.hpp"

using namespace Slic3r;

bool check_elements(TransformationMatrix const & matrix, 
    double m00, double m01, double m02, double m03,
    double m10, double m11, double m12, double m13,
    double m20, double m21, double m22, double m23);
bool check_point(const Pointf3 & point, coordf_t x, coordf_t y, coordf_t z);
double degtorad(double value){ return PI / 180.0 * value; }

SCENARIO("TransformationMatrix: constructors, copytor, comparing"){
    GIVEN("a default constructed Matrix") {
        auto trafo_default = TransformationMatrix();
        THEN("it is equivalent to the eye matrix") {
            REQUIRE(check_elements(trafo_default, 
                1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0));
        }

        WHEN("copied") {
            auto trafo_eq_assigned = trafo_default;
            THEN("the second is also equivalent to the eye matrix") {
                REQUIRE(check_elements(trafo_eq_assigned, 
                    1.0, 0.0, 0.0, 0.0,
                    0.0, 1.0, 0.0, 0.0,
                    0.0, 0.0, 1.0, 0.0));
            }
            THEN("they are regarded as equivalent to each other")
            {
                REQUIRE(trafo_default == trafo_eq_assigned);
                REQUIRE(!(trafo_default != trafo_eq_assigned));
            }
            trafo_eq_assigned.m00 = 2.0;
            THEN("they are not the same object") {
                REQUIRE(trafo_default != trafo_eq_assigned);
            }
        }
    }
    GIVEN("a directly set matrix") {
        WHEN("set via constructor") {
            auto trafo_set = TransformationMatrix(1,2,3,4,5,6,7,8,9,10,11,12);
            THEN("correctly set") {
                REQUIRE(check_elements(trafo_set,
                    1,2,3,4,
                    5,6,7,8,
                    9,10,11,12));
            }
        }
        WHEN("set via vector") {
            std::vector<double> elements;
            elements.reserve(12);
            elements.push_back(1);
            elements.push_back(2);
            elements.push_back(3);
            elements.push_back(4);
            elements.push_back(5);
            elements.push_back(6);
            elements.push_back(7);
            elements.push_back(8);
            elements.push_back(9);
            elements.push_back(10);
            elements.push_back(11);
            elements.push_back(12);
            auto trafo_vec = TransformationMatrix(elements);
            THEN("correctly set") {
                REQUIRE(check_elements(trafo_vec,
                    1,2,3,4,
                    5,6,7,8,
                    9,10,11,12));
            }
        }
    }
    GIVEN("two seperate matrices") {
        auto mat1 = TransformationMatrix(1,2,3,4,5,6,7,8,9,10,11,12);
        auto mat2 = TransformationMatrix(1,4,7,10,2,5,8,11,3,6,9,12);
        THEN("static multiplication produces correct output") {
            auto mat3 = TransformationMatrix::multiply(mat1, mat2);
            REQUIRE(check_elements(mat3,14,32,50,72,38,92,146,208,62,152,242,344));
            mat3 = TransformationMatrix::multiply(mat2, mat1);
            REQUIRE(check_elements(mat3,84,96,108,130,99,114,129,155,114,132,150,180));
        }
        THEN("direct multiplication produces correct output") {
            REQUIRE(check_elements(mat1.multiplyRight(mat2),14,32,50,72,38,92,146,208,62,152,242,344));
            REQUIRE(check_elements(mat2.multiplyLeft(mat1),14,32,50,72,38,92,146,208,62,152,242,344));
        }
    }
    GIVEN("a random transformation-ish matrix") {
        auto mat = TransformationMatrix(
            0.9004,-0.2369,-0.4847,12.9383,
            -0.9311,0.531,-0.5026,7.7931,
            -0.1225,0.5904,0.2576,-7.316);
        THEN("computing the determinante") {
            REQUIRE(std::abs(mat.determinante() - 0.5539) < 1.0e-3);
        }
        THEN("computing the inverse") {
            REQUIRE(check_elements(mat.inverse(),
                0.78273016,-0.40649736,0.67967289,-1.98683622,
                0.54421957,0.31157368,1.63191055,2.46965668,
                -0.87508846,-0.90741083,0.46498424,21.79552507));
        }
    }
}

SCENARIO("TransformationMatrix: application") {
    GIVEN("two vectors to validate geometric transformations") {
        auto vec1 = Pointf3(1,2,3);
        auto vec2 = Pointf3(-4,3,-2);
        THEN("testing general point transformation") {
            auto mat = TransformationMatrix(1,2,3,4,5,6,7,8,9,10,11,12);
            REQUIRE(check_point(mat.transform(vec1),18,46,74));  // default arg should be like a point
            REQUIRE(check_point(mat.transform(vec1,1),18,46,74));
            REQUIRE(check_point(mat.transform(vec1,0),14,38,62));
        }
        WHEN("testing scaling") {
            THEN("testing universal scaling") {
                auto mat = TransformationMatrix::mat_scale(3);
                REQUIRE(check_point(mat.transform(vec1),3,6,9));
            }
            THEN("testing vector like scaling") {
                auto mat = TransformationMatrix::mat_scale(2,3,4);
                REQUIRE(check_point(mat.transform(vec1),2,6,12));
            }
        }
        WHEN("testing mirroring") {
            THEN("testing axis aligned mirroring") {
                auto mat = TransformationMatrix::mat_mirror(Axis::X);
                REQUIRE(check_point(mat.transform(vec1),-1,2,3));
                mat = TransformationMatrix::mat_mirror(Axis::Y);
                REQUIRE(check_point(mat.transform(vec1),1,-2,3));
                mat = TransformationMatrix::mat_mirror(Axis::Z);
                REQUIRE(check_point(mat.transform(vec1),1,2,-3));
            }
            THEN("testing arbituary axis mirroring") {
                auto mat = TransformationMatrix::mat_mirror(vec2);
                REQUIRE(check_point(mat.transform(vec1),-0.1034,2.8276,2.4483));
                REQUIRE(std::abs(mat.determinante() + 1.0) < 1.0e-3);
            }
        }
        WHEN("testing translation") {
            THEN("testing xyz translation") {
                auto mat = TransformationMatrix::mat_translation(4,2,5);
                REQUIRE(check_point(mat.transform(vec1),5,4,8));
            }
            THEN("testing vector-defined translation") {
                auto mat = TransformationMatrix::mat_translation(vec2);
                REQUIRE(check_point(mat.transform(vec1),-3,5,1));
            }
        }
        WHEN("testing rotation") {
            THEN("testing axis aligned rotation") {
                auto mat = TransformationMatrix::mat_rotation(degtorad(90), Axis::X);
                REQUIRE(check_point(mat.transform(vec1),1,-3,2));
                mat = TransformationMatrix::mat_rotation(degtorad(90), Axis::Y);
                REQUIRE(check_point(mat.transform(vec1),3,2,-1));
                mat = TransformationMatrix::mat_rotation(degtorad(90), Axis::Z);
                REQUIRE(check_point(mat.transform(vec1),-2,1,3));
            }
            THEN("testing arbituary axis rotation") {
                auto mat = TransformationMatrix::mat_rotation(degtorad(80), vec2);
                REQUIRE(check_point(mat.transform(vec1),3.0069,1.8341,-1.2627));
                REQUIRE(std::abs(mat.determinante() - 1.0) < 1.0e-3);
            }
            THEN("testing quaternion rotation") {
                auto mat = TransformationMatrix::mat_rotation(-0.4775,0.3581,-0.2387,0.7660);
                REQUIRE(check_point(mat.transform(vec1),3.0069,1.8341,-1.2627));
                REQUIRE(std::abs(mat.determinante() - 1.0) < 1.0e-3);
            }
            THEN("testing vector to vector") {
                auto mat = TransformationMatrix::mat_rotation(vec1,vec2);
                REQUIRE(check_point(mat.transform(vec1),-2.7792,2.0844,-1.3896));
                REQUIRE(std::abs(mat.determinante() - 1.0) < 1.0e-3);
                mat = TransformationMatrix::mat_rotation(vec1,vec1.negative());
                REQUIRE(check_point(mat.transform(vec1),-1,-2,-3)); // colinear, opposite direction
                mat = TransformationMatrix::mat_rotation(vec1,vec1);
                REQUIRE(check_elements(mat, 
                1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0)); // colinear, same direction
            }
        }
    }
}

bool check_elements(const TransformationMatrix & matrix, 
    double m00, double m01, double m02, double m03,
    double m10, double m11, double m12, double m13,
    double m20, double m21, double m22, double m23)
{
    const double EPSILON = 1.0e-3;
    bool equal = true;
    equal &= std::abs(matrix.m00 - m00) < EPSILON;
    equal &= std::abs(matrix.m01 - m01) < EPSILON;
    equal &= std::abs(matrix.m02 - m02) < EPSILON;
    equal &= std::abs(matrix.m03 - m03) < EPSILON;
    equal &= std::abs(matrix.m10 - m10) < EPSILON;
    equal &= std::abs(matrix.m11 - m11) < EPSILON;
    equal &= std::abs(matrix.m12 - m12) < EPSILON;
    equal &= std::abs(matrix.m13 - m13) < EPSILON;
    equal &= std::abs(matrix.m20 - m20) < EPSILON;
    equal &= std::abs(matrix.m21 - m21) < EPSILON;
    equal &= std::abs(matrix.m22 - m22) < EPSILON;
    equal &= std::abs(matrix.m23 - m23) < EPSILON;
    return equal;
}

bool check_point(const Pointf3 & point, coordf_t x, coordf_t y, coordf_t z)
{
    const coordf_t EPSILON = 1.0e-3;
    bool equal = true;
    equal &= std::abs(point.x - x) < EPSILON;
    equal &= std::abs(point.y - y) < EPSILON;
    equal &= std::abs(point.z - z) < EPSILON;
    return equal;
}
