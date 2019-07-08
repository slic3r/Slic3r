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
    mesh.transform(model_instance->additional_trafo);

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

bool PlaterObject::instance_contains(Slic3r::Point point) const {
    return std::any_of(this->instance_thumbnails.cbegin(), this->instance_thumbnails.cend(), 
    [point](const ExPolygonCollection ep) {
        return ep.contains(point);
    });
}
PlaterObject& PlaterObject::operator=(const PlaterObject& other) {
    if (&other == this) return *this;
    this->name = std::string(other.name);
    this->identifier = other.identifier;
    this->input_file = std::string(other.input_file);
    this->input_file_obj_idx = other.input_file_obj_idx;

    this->selected = false;
    this->selected_instance = -1;

    this->thumbnail = Slic3r::ExPolygonCollection(other.thumbnail);
    this->transformed_thumbnail = Slic3r::ExPolygonCollection(other.transformed_thumbnail);
    
    this->instance_thumbnails = std::vector<Slic3r::ExPolygonCollection>(other.instance_thumbnails);
    return *this;
}

PlaterObject& PlaterObject::operator=(PlaterObject&& other) {
    this->name = std::string(other.name);
    this->identifier = other.identifier;
    this->input_file = std::string(other.input_file);
    this->input_file_obj_idx = other.input_file_obj_idx;

    this->selected = std::move(other.selected);
    this->selected_instance = std::move(other.selected);

    this->thumbnail = Slic3r::ExPolygonCollection(other.thumbnail);
    this->transformed_thumbnail = Slic3r::ExPolygonCollection(other.transformed_thumbnail);
    
    this->instance_thumbnails = std::vector<Slic3r::ExPolygonCollection>(other.instance_thumbnails);
    return *this;
}
} } // Namespace Slic3r::GUI
