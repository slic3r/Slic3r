#include "Plater/PlaterObject.hpp"

#include "Geometry.hpp"
#include "ExPolygon.hpp"
#include "libslic3r.h"
#include "Log.hpp"
#include "misc_ui.hpp"

namespace Slic3r { namespace GUI {

Slic3r::ExPolygonCollection& PlaterObject::make_thumbnail(std::shared_ptr<Slic3r::Model> model, int obj_idx) {

    // make method idempotent
    this->thumbnail.expolygons.clear();

    auto mesh {model->objects[obj_idx]->raw_mesh()};
    auto model_instance {model->objects[obj_idx]->instances[0]};

    // Apply any x/y rotations and scaling vector if this came from a 3MF object.
    mesh.rotate_x(model_instance->x_rotation);
    mesh.rotate_y(model_instance->y_rotation);
    mesh.scale(model_instance->scaling_vector);

    if (mesh.facets_count() <= 5000) {
        auto area_threshold {scale_(1.0)};
        ExPolygons tmp {};
        for (auto p : mesh.horizontal_projection()) {
            if (p.area() >= area_threshold) tmp.push_back(p);
        }
        // return all polys bigger than the area
        this->thumbnail.append(tmp);
        this->thumbnail.simplify(0.5);
    } else {
        auto convex_hull {Slic3r::ExPolygon(mesh.convex_hull())};
        this->thumbnail.append(convex_hull);
    }

    return this->thumbnail;
}


Slic3r::ExPolygonCollection& PlaterObject::transform_thumbnail(std::shared_ptr<Slic3r::Model> model, int obj_idx) {
    if (this->thumbnail.expolygons.size() == 0) return this->thumbnail; 

    const auto& model_object {model->objects[obj_idx] };
    const auto& model_instance {model_object->instances[0]};

    // the order of these transformations MUST be the same everywhere, including
    // in Slic3r::Print->add_model_object()
    auto t {this->thumbnail};
    t.rotate(model_instance->rotation, Slic3r::Point(0,0));
    t.scale(model_instance->scaling_factor);

    this->transformed_thumbnail = t;
    return this->transformed_thumbnail;
}

} } // Namespace Slic3r::GUI
