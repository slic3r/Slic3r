#include <catch.hpp>
#include "ExtrusionEntityCollection.hpp"
#include "ExtrusionEntity.hpp"
#include "Point.hpp"
#include <cstdlib>

Point random_point(float LO=-50, float HI=50) {
    float x = LO + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(HI-LO)));
    float y = LO + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(HI-LO)));
    return Point(x, y);
}

// build a sample extrusion entity collection with random start and end points.
ExtrusionPath random_path(size_t length = 20, float LO=-50, float HI=50) {
    ExtrusionPath t {erPerimeter, 1.0, 1.0, 1.0};
    for (size_t j = 0; j < length; j++) {
        t.polyline.append(random_point(LO, HI));
    }
    return t;
}

ExtrusionPaths random_paths(size_t count=10, size_t length=20, float LO=-50, float HI=50) {
    ExtrusionPaths p;
    for (size_t i = 0; i < count; i++)
        p.push_back(random_path(length, LO, HI));
    return p;
}

using namespace Slic3r;

SCENARIO("ExtrusionEntityCollection: Polygon flattening") {
    srand(0xDEADBEEF); // consistent seed for test reproducibility.

    // Generate one specific random path set and save it for later comparison
    ExtrusionPaths nosort_path_set {random_paths()};

    ExtrusionEntityCollection sub_nosort;
    sub_nosort.append(nosort_path_set);
    sub_nosort.no_sort = true;

    ExtrusionEntityCollection sub_sort;
    sub_sort.no_sort = false;
    sub_sort.append(random_paths());


    GIVEN("A Extrusion Entity Collection with a child that has one child that is marked as no-sort") {
        ExtrusionEntityCollection sample;
        ExtrusionEntityCollection output;

        sample.append(sub_sort);
        sample.append(sub_nosort);
        sample.append(sub_sort);
        WHEN("The EEC is flattened with default options (preserve_order=false)") {
            sample.flatten(&output);
            THEN("The output EEC contains no Extrusion Entity Collections") {
                CHECK(std::count_if(output.entities.cbegin(), output.entities.cend(), [=](const ExtrusionEntity* e) {return e->is_collection();}) == 0);
            }
        }
        WHEN("The EEC is flattened with preservation (preserve_order=true)") {
            sample.flatten(&output, true);
            THEN("The output EECs contains one EEC.") {
                CHECK(std::count_if(output.entities.cbegin(), output.entities.cend(), [=](const ExtrusionEntity* e) {return e->is_collection();}) == 1);
            }
            AND_THEN("The ordered EEC contains the same order of elements than the original") {
                // find the entity in the collection
                for (auto e : output.entities) {
                    if (!e->is_collection()) continue;
                    ExtrusionEntityCollection* temp = dynamic_cast<ExtrusionEntityCollection*>(e);
                    // check each Extrusion path against nosort_path_set to see if the first and last match the same
                    CHECK(nosort_path_set.size() == temp->size());
                    for (size_t i = 0; i < nosort_path_set.size(); i++) {
                        CHECK(temp->entities[i]->first_point() == nosort_path_set[i].first_point());
                        CHECK(temp->entities[i]->last_point() == nosort_path_set[i].last_point());
                    }
                }
            }
        }
    }
}
