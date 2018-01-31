#include "Model.hpp"
#include "Geometry.hpp"
#include "IO.hpp"
#include <iostream>
#include <set>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>

namespace Slic3r {

Model::Model() : metadata(std::map<std::string, std::string>()) {}

Model::Model(const Model &other)
{
    // copy materials
    for (ModelMaterialMap::const_iterator i = other.materials.begin(); i != other.materials.end(); ++i)
        this->add_material(i->first, *i->second);
    
    // copy objects
    this->objects.reserve(other.objects.size());
    for (ModelObjectPtrs::const_iterator i = other.objects.begin(); i != other.objects.end(); ++i)
        this->add_object(**i, true);

    // copy metadata
    this->metadata = other.metadata;
}

Model& Model::operator= (Model other)
{
    this->swap(other);
    return *this;
}

void
Model::swap(Model &other)
{
    std::swap(this->materials,  other.materials);
    std::swap(this->objects,    other.objects);
    std::swap(this->metadata,   other.metadata);
}

Model::~Model()
{
    this->clear_objects();
    this->clear_materials();
}

Model
Model::read_from_file(std::string input_file)
{
    Model model;
    
    if (boost::algorithm::iends_with(input_file, ".stl")) {
        IO::STL::read(input_file, &model);
    } else if (boost::algorithm::iends_with(input_file, ".obj")) {
        IO::OBJ::read(input_file, &model);
    } else if (boost::algorithm::iends_with(input_file, ".amf")
            || boost::algorithm::iends_with(input_file, ".amf.xml")) {
        IO::AMF::read(input_file, &model);
    } else if (boost::algorithm::iends_with(input_file, ".3mf")) {
        IO::TMF::read(input_file, &model);
    } else {
        throw std::runtime_error("Unknown file format");
    }
    
    if (model.objects.empty())
        throw std::runtime_error("The supplied file couldn't be read because it's empty");
    
    for (ModelObjectPtrs::const_iterator o = model.objects.begin(); o != model.objects.end(); ++o)
        (*o)->input_file = input_file;
    
    return model;
}

ModelObject*
Model::add_object()
{
    ModelObject* new_object = new ModelObject(this);
    this->objects.push_back(new_object);
    return new_object;
}

ModelObject*
Model::add_object(const ModelObject &other, bool copy_volumes)
{
    ModelObject* new_object = new ModelObject(this, other, copy_volumes);
    this->objects.push_back(new_object);
    return new_object;
}

void
Model::delete_object(size_t idx)
{
    ModelObjectPtrs::iterator i = this->objects.begin() + idx;
    delete *i;
    this->objects.erase(i);
}

void
Model::clear_objects()
{
    while (!this->objects.empty())
        this->delete_object(0);
}

void
Model::delete_material(t_model_material_id material_id)
{
    ModelMaterialMap::iterator i = this->materials.find(material_id);
    if (i != this->materials.end()) {
        delete i->second;
        this->materials.erase(i);
    }
}

void
Model::clear_materials()
{
    while (!this->materials.empty())
        this->delete_material( this->materials.begin()->first );
}

ModelMaterial*
Model::add_material(t_model_material_id material_id)
{
    ModelMaterial* material = this->get_material(material_id);
    if (material == NULL) {
        material = this->materials[material_id] = new ModelMaterial(this);
    }
    return material;
}

ModelMaterial*
Model::add_material(t_model_material_id material_id, const ModelMaterial &other)
{
    // delete existing material if any
    ModelMaterial* material = this->get_material(material_id);
    if (material != NULL) {
        delete material;
    }
    
    // set new material
    material = new ModelMaterial(this, other);
    this->materials[material_id] = material;
    return material;
}

ModelMaterial*
Model::get_material(t_model_material_id material_id)
{
    ModelMaterialMap::iterator i = this->materials.find(material_id);
    if (i == this->materials.end()) {
        return NULL;
    } else {
        return i->second;
    }
}

bool
Model::has_objects_with_no_instances() const
{
    for (ModelObjectPtrs::const_iterator i = this->objects.begin();
        i != this->objects.end(); ++i)
    {
        if ((*i)->instances.empty()) {
            return true;
        }
    }

    return false;
}

// makes sure all objects have at least one instance
bool
Model::add_default_instances()
{
    // apply a default position to all objects not having one
    for (ModelObjectPtrs::const_iterator o = this->objects.begin(); o != this->objects.end(); ++o) {
        if ((*o)->instances.empty()) {
            (*o)->add_instance();
        }
    }
    return true;
}

// this returns the bounding box of the *transformed* instances
BoundingBoxf3
Model::bounding_box() const
{
    BoundingBoxf3 bb;
    for (ModelObjectPtrs::const_iterator o = this->objects.begin(); o != this->objects.end(); ++o) {
        bb.merge((*o)->bounding_box());
    }
    return bb;
}

void
Model::repair()
{
    for (ModelObjectPtrs::const_iterator o = this->objects.begin(); o != this->objects.end(); ++o)
        (*o)->repair();
}

void
Model::center_instances_around_point(const Pointf &point)
{
    BoundingBoxf3 bb = this->bounding_box();
    
    Sizef3 size = bb.size();
    coordf_t shift_x = -bb.min.x + point.x - size.x/2;
    coordf_t shift_y = -bb.min.y + point.y - size.y/2;
    for (ModelObjectPtrs::const_iterator o = this->objects.begin(); o != this->objects.end(); ++o) {
        for (ModelInstancePtrs::const_iterator i = (*o)->instances.begin(); i != (*o)->instances.end(); ++i) {
            (*i)->offset.translate(shift_x, shift_y);
        }
        (*o)->invalidate_bounding_box();
    }
}

void
Model::translate(coordf_t x, coordf_t y, coordf_t z)
{
    for (ModelObjectPtrs::const_iterator o = this->objects.begin(); o != this->objects.end(); ++o) {
        (*o)->translate(x, y, z);
    }
}

// flattens everything to a single mesh
TriangleMesh
Model::mesh() const
{
    TriangleMesh mesh;
    for (ModelObjectPtrs::const_iterator o = this->objects.begin(); o != this->objects.end(); ++o) {
        mesh.merge((*o)->mesh());
    }
    return mesh;
}

// flattens everything to a single mesh
TriangleMesh
Model::raw_mesh() const
{
    TriangleMesh mesh;
    for (ModelObjectPtrs::const_iterator o = this->objects.begin(); o != this->objects.end(); ++o) {
        mesh.merge((*o)->raw_mesh());
    }
    return mesh;
}

bool
Model::_arrange(const Pointfs &sizes, coordf_t dist, const BoundingBoxf* bb, Pointfs &out) const
{
    // we supply unscaled data to arrange()
    bool result = Slic3r::Geometry::arrange(
        sizes.size(),               // number of parts
        BoundingBoxf(sizes).max,    // width and height of a single cell
        dist,                       // distance between cells
        bb,                         // bounding box of the area to fill
        out                         // output positions
    );
    
    if (!result && bb != NULL) {
        // Try to arrange again ignoring bb
        result = Slic3r::Geometry::arrange(
            sizes.size(),               // number of parts
            BoundingBoxf(sizes).max,    // width and height of a single cell
            dist,                       // distance between cells
            NULL,                         // bounding box of the area to fill
            out                         // output positions
        );
    }
    
    return result;
}

/*  arrange objects preserving their instance count
    but altering their instance positions */
bool
Model::arrange_objects(coordf_t dist, const BoundingBoxf* bb)
{
    // get the (transformed) size of each instance so that we take
    // into account their different transformations when packing
    Pointfs instance_sizes;
    for (ModelObjectPtrs::const_iterator o = this->objects.begin(); o != this->objects.end(); ++o) {
        for (size_t i = 0; i < (*o)->instances.size(); ++i) {
            instance_sizes.push_back((*o)->instance_bounding_box(i).size());
        }
    }
    
    Pointfs positions;
    if (! this->_arrange(instance_sizes, dist, bb, positions))
        return false;
    
    for (ModelObjectPtrs::const_iterator o = this->objects.begin(); o != this->objects.end(); ++o) {
        for (ModelInstancePtrs::const_iterator i = (*o)->instances.begin(); i != (*o)->instances.end(); ++i) {
            (*i)->offset = positions.back();
            positions.pop_back();
        }
        (*o)->invalidate_bounding_box();
    }
    return true;
}

/*  duplicate the entire model preserving instance relative positions */
void
Model::duplicate(size_t copies_num, coordf_t dist, const BoundingBoxf* bb)
{
    Pointfs model_sizes(copies_num-1, this->bounding_box().size());
    Pointfs positions;
    if (! this->_arrange(model_sizes, dist, bb, positions))
        CONFESS("Cannot duplicate part as the resulting objects would not fit on the print bed.\n");
    
    // note that this will leave the object count unaltered
    
    for (ModelObjectPtrs::const_iterator o = this->objects.begin(); o != this->objects.end(); ++o) {
        // make a copy of the pointers in order to avoid recursion when appending their copies
        ModelInstancePtrs instances = (*o)->instances;
        for (ModelInstancePtrs::const_iterator i = instances.begin(); i != instances.end(); ++i) {
            for (Pointfs::const_iterator pos = positions.begin(); pos != positions.end(); ++pos) {
                ModelInstance* instance = (*o)->add_instance(**i);
                instance->offset.translate(*pos);
            }
        }
        (*o)->invalidate_bounding_box();
    }
}

/*  this will append more instances to each object
    and then automatically rearrange everything */
void
Model::duplicate_objects(size_t copies_num, coordf_t dist, const BoundingBoxf* bb)
{
    for (ModelObjectPtrs::const_iterator o = this->objects.begin(); o != this->objects.end(); ++o) {
        // make a copy of the pointers in order to avoid recursion when appending their copies
        ModelInstancePtrs instances = (*o)->instances;
        for (ModelInstancePtrs::const_iterator i = instances.begin(); i != instances.end(); ++i) {
            for (size_t k = 2; k <= copies_num; ++k)
                (*o)->add_instance(**i);
        }
    }
    
    this->arrange_objects(dist, bb);
}

void
Model::duplicate_objects_grid(size_t x, size_t y, coordf_t dist)
{
    if (this->objects.size() > 1) throw std::runtime_error("Grid duplication is not supported with multiple objects");
    if (this->objects.empty()) throw std::runtime_error("No objects!");

    ModelObject* object = this->objects.front();
    object->clear_instances();

    Sizef3 size = object->bounding_box().size();

    for (size_t x_copy = 1; x_copy <= x; ++x_copy) {
        for (size_t y_copy = 1; y_copy <= y; ++y_copy) {
            ModelInstance* instance = object->add_instance();
            instance->offset.x = (size.x + dist) * (x_copy-1);
            instance->offset.y = (size.y + dist) * (y_copy-1);
        }
    }
}

void
Model::print_info() const
{
    for (ModelObjectPtrs::const_iterator o = this->objects.begin(); o != this->objects.end(); ++o)
        (*o)->print_info();
}

bool
Model::looks_like_multipart_object() const
{
    if (this->objects.size() == 1) return false;
    for (const ModelObject* o : this->objects) {
        if (o->volumes.size() > 1) return false;
        if (o->config.keys().size() > 1) return false;
    }
    
    std::set<coordf_t> heights;
    for (const ModelObject* o : this->objects)
        for (const ModelVolume* v : o->volumes)
            heights.insert(v->mesh.bounding_box().min.z);
    return heights.size() > 1;
}

void
Model::convert_multipart_object()
{
    if (this->objects.empty()) return;
    
    ModelObject* object = this->add_object();
    object->input_file = this->objects.front()->input_file;
    
    for (const ModelObject* o : this->objects) {
        for (const ModelVolume* v : o->volumes) {
            ModelVolume* v2 = object->add_volume(*v);
            v2->name = o->name;
        }
    }
    for (const ModelInstance* i : this->objects.front()->instances)
        object->add_instance(*i);
    
    while (this->objects.size() > 1)
        this->delete_object(0);
}

ModelMaterial::ModelMaterial(Model *model) : model(model) {}
ModelMaterial::ModelMaterial(Model *model, const ModelMaterial &other)
    : attributes(other.attributes), config(other.config), model(model)
{}

void
ModelMaterial::apply(const t_model_material_attributes &attributes)
{
    this->attributes.insert(attributes.begin(), attributes.end());
}


ModelObject::ModelObject(Model *model)
    : part_number(-1), _bounding_box_valid(false), model(model)
{}

ModelObject::ModelObject(Model *model, const ModelObject &other, bool copy_volumes)
:   name(other.name),
    input_file(other.input_file),
    instances(),
    volumes(),
    config(other.config),
    layer_height_ranges(other.layer_height_ranges),
    part_number(other.part_number),
    layer_height_spline(other.layer_height_spline),
    origin_translation(other.origin_translation),
    _bounding_box(other._bounding_box),
    _bounding_box_valid(other._bounding_box_valid),
    model(model)
{
    if (copy_volumes) {
        this->volumes.reserve(other.volumes.size());
        for (ModelVolumePtrs::const_iterator i = other.volumes.begin(); i != other.volumes.end(); ++i)
            this->add_volume(**i);
    }
    
    this->instances.reserve(other.instances.size());
    for (ModelInstancePtrs::const_iterator i = other.instances.begin(); i != other.instances.end(); ++i)
        this->add_instance(**i);
}

ModelObject& ModelObject::operator= (ModelObject other)
{
    this->swap(other);
    return *this;
}

void
ModelObject::swap(ModelObject &other)
{
    std::swap(this->input_file,             other.input_file);
    std::swap(this->instances,              other.instances);
    std::swap(this->volumes,                other.volumes);
    std::swap(this->config,                 other.config);
    std::swap(this->layer_height_ranges,    other.layer_height_ranges);
    std::swap(this->layer_height_spline,    other.layer_height_spline);
    std::swap(this->origin_translation,     other.origin_translation);
    std::swap(this->_bounding_box,          other._bounding_box);
    std::swap(this->_bounding_box_valid,    other._bounding_box_valid);
    std::swap(this->part_number,            other.part_number);
}

ModelObject::~ModelObject()
{
    this->clear_volumes();
    this->clear_instances();
}

ModelVolume*
ModelObject::add_volume(const TriangleMesh &mesh)
{
    ModelVolume* v = new ModelVolume(this, mesh);
    this->volumes.push_back(v);
    this->invalidate_bounding_box();
    return v;
}

ModelVolume*
ModelObject::add_volume(const ModelVolume &other)
{
    ModelVolume* v = new ModelVolume(this, other);
    this->volumes.push_back(v);
    this->invalidate_bounding_box();
    return v;
}

void
ModelObject::delete_volume(size_t idx)
{
    ModelVolumePtrs::iterator i = this->volumes.begin() + idx;
    delete *i;
    this->volumes.erase(i);
    this->invalidate_bounding_box();
}

void
ModelObject::clear_volumes()
{
    while (!this->volumes.empty())
        this->delete_volume(0);
}

ModelInstance*
ModelObject::add_instance()
{
    ModelInstance* i = new ModelInstance(this);
    this->instances.push_back(i);
    return i;
}

ModelInstance*
ModelObject::add_instance(const ModelInstance &other)
{
    ModelInstance* i = new ModelInstance(this, other);
    this->instances.push_back(i);
    return i;
}

void
ModelObject::delete_instance(size_t idx)
{
    ModelInstancePtrs::iterator i = this->instances.begin() + idx;
    delete *i;
    this->instances.erase(i);
}

void
ModelObject::delete_last_instance()
{
    this->delete_instance(this->instances.size() - 1);
}

void
ModelObject::clear_instances()
{
    while (!this->instances.empty())
        this->delete_last_instance();

}

// this returns the bounding box of the *transformed* instances
BoundingBoxf3
ModelObject::bounding_box()
{
    if (!this->_bounding_box_valid) this->update_bounding_box();
    return this->_bounding_box;
}

void
ModelObject::invalidate_bounding_box()
{
    this->_bounding_box_valid = false;
}

void
ModelObject::update_bounding_box()
{
//    this->_bounding_box = this->mesh().bounding_box();
    BoundingBoxf3 raw_bbox;
    for (ModelVolumePtrs::const_iterator v = this->volumes.begin(); v != this->volumes.end(); ++v) {
        if ((*v)->modifier) continue;
        raw_bbox.merge((*v)->mesh.bounding_box());
    }
    BoundingBoxf3 bb;
    for (ModelInstancePtrs::const_iterator i = this->instances.begin(); i != this->instances.end(); ++i)
        bb.merge((*i)->transform_bounding_box(raw_bbox));
    this->_bounding_box = bb;
    this->_bounding_box_valid = true;
}

void
ModelObject::repair()
{
    for (ModelVolumePtrs::const_iterator v = this->volumes.begin(); v != this->volumes.end(); ++v)
        (*v)->mesh.repair();
}

// flattens all volumes and instances into a single mesh
TriangleMesh
ModelObject::mesh() const
{
    TriangleMesh mesh;
    TriangleMesh raw_mesh = this->raw_mesh();
    
    for (ModelInstancePtrs::const_iterator i = this->instances.begin(); i != this->instances.end(); ++i) {
        TriangleMesh m(raw_mesh);
        (*i)->transform_mesh(&m);
        mesh.merge(m);
    }
    return mesh;
}

TriangleMesh
ModelObject::raw_mesh() const
{
    TriangleMesh mesh;
    for (ModelVolumePtrs::const_iterator v = this->volumes.begin(); v != this->volumes.end(); ++v) {
        if ((*v)->modifier) continue;
        mesh.merge((*v)->mesh);
    }
    return mesh;
}

BoundingBoxf3
ModelObject::raw_bounding_box() const
{
    BoundingBoxf3 bb;
    for (ModelVolumePtrs::const_iterator v = this->volumes.begin(); v != this->volumes.end(); ++v) {
        if ((*v)->modifier) continue;
        if (this->instances.empty()) CONFESS("Can't call raw_bounding_box() with no instances");
        bb.merge(this->instances.front()->transform_mesh_bounding_box(&(*v)->mesh, true));
    }
    return bb;
}

// this returns the bounding box of the *transformed* given instance
BoundingBoxf3
ModelObject::instance_bounding_box(size_t instance_idx) const
{
    BoundingBoxf3 bb;
    for (ModelVolumePtrs::const_iterator v = this->volumes.begin(); v != this->volumes.end(); ++v) {
        if ((*v)->modifier) continue;
        bb.merge(this->instances[instance_idx]->transform_mesh_bounding_box(&(*v)->mesh, true));
    }
    return bb;
}
	
void
Model::align_instances_to_origin()
{
    BoundingBoxf3 bb = this->bounding_box();
    
    Pointf new_center = (Pointf)bb.size();
    new_center.translate(-new_center.x/2, -new_center.y/2);
    this->center_instances_around_point(new_center);
}

void
ModelObject::align_to_ground()
{
    // calculate the displacements needed to 
    // center this object around the origin
	BoundingBoxf3 bb;
	for (const ModelVolume* v : this->volumes)
		if (!v->modifier)
			bb.merge(v->mesh.bounding_box());
    
    this->translate(0, 0, -bb.min.z);
    this->origin_translation.translate(0, 0, -bb.min.z);
}

void
ModelObject::center_around_origin()
{
    // calculate the displacements needed to 
    // center this object around the origin
	BoundingBoxf3 bb;
	for (ModelVolumePtrs::const_iterator v = this->volumes.begin(); v != this->volumes.end(); ++v)
		if (! (*v)->modifier)
			bb.merge((*v)->mesh.bounding_box());
    
    // first align to origin on XYZ
    Vectorf3 vector(-bb.min.x, -bb.min.y, -bb.min.z);
    
    // then center it on XY
    Sizef3 size = bb.size();
    vector.x -= size.x/2;
    vector.y -= size.y/2;
    
    this->translate(vector);
    this->origin_translation.translate(vector);
    
    if (!this->instances.empty()) {
        for (ModelInstancePtrs::const_iterator i = this->instances.begin(); i != this->instances.end(); ++i) {
            // apply rotation and scaling to vector as well before translating instance,
            // in order to leave final position unaltered
            Vectorf3 v = vector.negative();
            v.rotate((*i)->rotation, (*i)->offset);
            v.scale((*i)->scaling_factor);
            (*i)->offset.translate(v.x, v.y);
        }
        this->invalidate_bounding_box();
    }
}

void
ModelObject::translate(const Vectorf3 &vector)
{
    this->translate(vector.x, vector.y, vector.z);
}

void
ModelObject::translate(coordf_t x, coordf_t y, coordf_t z)
{
    for (ModelVolumePtrs::const_iterator v = this->volumes.begin(); v != this->volumes.end(); ++v) {
        (*v)->mesh.translate(x, y, z);
    }
    if (this->_bounding_box_valid) this->_bounding_box.translate(x, y, z);
}

void
ModelObject::scale(float factor)
{
    this->scale(Pointf3(factor, factor, factor));
}

void
ModelObject::scale(const Pointf3 &versor)
{
    if (versor.x == 1 && versor.y == 1 && versor.z == 1) return;
    for (ModelVolumePtrs::const_iterator v = this->volumes.begin(); v != this->volumes.end(); ++v) {
        (*v)->mesh.scale(versor);
    }
    
    // reset origin translation since it doesn't make sense anymore
    this->origin_translation = Pointf3(0,0,0);
    this->invalidate_bounding_box();
}

void
ModelObject::scale_to_fit(const Sizef3 &size)
{
    Sizef3 orig_size = this->bounding_box().size();
    float factor = fminf(
        size.x / orig_size.x,
        fminf(
            size.y / orig_size.y,
            size.z / orig_size.z
        )
    );
    this->scale(factor);
}

void
ModelObject::rotate(float angle, const Axis &axis)
{
    if (angle == 0) return;
    for (ModelVolumePtrs::const_iterator v = this->volumes.begin(); v != this->volumes.end(); ++v) {
        (*v)->mesh.rotate(angle, axis);
    }
    this->origin_translation = Pointf3(0,0,0);
    this->invalidate_bounding_box();
}

void
ModelObject::mirror(const Axis &axis)
{
    for (ModelVolumePtrs::const_iterator v = this->volumes.begin(); v != this->volumes.end(); ++v) {
        (*v)->mesh.mirror(axis);
    }
    this->origin_translation = Pointf3(0,0,0);
    this->invalidate_bounding_box();
}

void
ModelObject::transform_by_instance(ModelInstance instance, bool dont_translate)
{
    // We get instance by copy because we would alter it in the loop below,
    // causing inconsistent values in subsequent instances.
    this->rotate(instance.rotation, Z);
    this->scale(instance.scaling_factor);
    if (!dont_translate)
        this->translate(instance.offset.x, instance.offset.y, 0);
    
    for (ModelInstance* i : this->instances) {
        i->rotation -= instance.rotation;
        i->scaling_factor /= instance.scaling_factor;
        if (!dont_translate)
            i->offset.translate(-instance.offset.x, -instance.offset.y);
    }
    this->origin_translation = Pointf3(0,0,0);
    this->invalidate_bounding_box();
}

size_t
ModelObject::materials_count() const
{
    std::set<t_model_material_id> material_ids;
    for (ModelVolumePtrs::const_iterator v = this->volumes.begin(); v != this->volumes.end(); ++v) {
        material_ids.insert((*v)->material_id());
    }
    return material_ids.size();
}

size_t
ModelObject::facets_count() const
{
    size_t num = 0;
    for (ModelVolumePtrs::const_iterator v = this->volumes.begin(); v != this->volumes.end(); ++v) {
        if ((*v)->modifier) continue;
        num += (*v)->mesh.stl.stats.number_of_facets;
    }
    return num;
}

bool
ModelObject::needed_repair() const
{
    for (ModelVolumePtrs::const_iterator v = this->volumes.begin(); v != this->volumes.end(); ++v) {
        if ((*v)->modifier) continue;
        if ((*v)->mesh.needed_repair()) return true;
    }
    return false;
}

void
ModelObject::cut(Axis axis, coordf_t z, Model* model) const
{
    // clone this one to duplicate instances, materials etc.
    ModelObject* upper = model->add_object(*this);
    ModelObject* lower = model->add_object(*this);
    upper->clear_volumes();
    lower->clear_volumes();
    upper->input_file = "";
    lower->input_file = "";
    
    for (ModelVolumePtrs::const_iterator v = this->volumes.begin(); v != this->volumes.end(); ++v) {
        ModelVolume* volume = *v;
        if (volume->modifier) {
            // don't cut modifiers
            upper->add_volume(*volume);
            lower->add_volume(*volume);
        } else {
            TriangleMesh upper_mesh, lower_mesh;
            
            if (axis == X) {
                TriangleMeshSlicer<X>(&volume->mesh).cut(z, &upper_mesh, &lower_mesh);
            } else if (axis == Y) {
                TriangleMeshSlicer<Y>(&volume->mesh).cut(z, &upper_mesh, &lower_mesh);
            } else if (axis == Z) {
                TriangleMeshSlicer<Z>(&volume->mesh).cut(z, &upper_mesh, &lower_mesh);
            }
            
            upper_mesh.repair();
            lower_mesh.repair();
            upper_mesh.reset_repair_stats();
            lower_mesh.reset_repair_stats();
            
            if (upper_mesh.facets_count() > 0) {
                ModelVolume* vol    = upper->add_volume(upper_mesh);
                vol->name           = volume->name;
                vol->config         = volume->config;
                vol->set_material(volume->material_id(), *volume->material());
            }
            if (lower_mesh.facets_count() > 0) {
                ModelVolume* vol    = lower->add_volume(lower_mesh);
                vol->name           = volume->name;
                vol->config         = volume->config;
                vol->set_material(volume->material_id(), *volume->material());
            }
        }
    }
}

void
ModelObject::split(ModelObjectPtrs* new_objects)
{
    if (this->volumes.size() > 1) {
        // We can't split meshes if there's more than one volume, because
        // we can't group the resulting meshes by object afterwards
        new_objects->push_back(this);
        return;
    }
    
    ModelVolume* volume = this->volumes.front();
    TriangleMeshPtrs meshptrs = volume->mesh.split();
    for (TriangleMeshPtrs::iterator mesh = meshptrs.begin(); mesh != meshptrs.end(); ++mesh) {
        (*mesh)->repair();
        
        ModelObject* new_object = this->model->add_object(*this, false);
        new_object->input_file  = "";
        new_object->part_number = this->part_number; //According to 3mf part number should be given to the split parts.
        ModelVolume* new_volume = new_object->add_volume(**mesh);
        new_volume->name        = volume->name;
        new_volume->config      = volume->config;
        new_volume->modifier    = volume->modifier;
        new_volume->material_id(volume->material_id());
        
        new_objects->push_back(new_object);
        delete *mesh;
    }
    
    return;
}

void
ModelObject::print_info() const
{
    using namespace std;
    cout << fixed;
    cout << "[" << boost::filesystem::path(this->input_file).filename().string() << "]" << endl;
    
    TriangleMesh mesh = this->raw_mesh();
    mesh.check_topology();
    BoundingBoxf3 bb = mesh.bounding_box();
    Sizef3 size = bb.size();
    cout << "size_x = " << size.x << endl;
    cout << "size_y = " << size.y << endl;
    cout << "size_z = " << size.z << endl;
    cout << "min_x = " << bb.min.x << endl;
    cout << "min_y = " << bb.min.y << endl;
    cout << "min_z = " << bb.min.z << endl;
    cout << "max_x = " << bb.max.x << endl;
    cout << "max_y = " << bb.max.y << endl;
    cout << "max_z = " << bb.max.z << endl;
    cout << "number_of_facets = " << mesh.stl.stats.number_of_facets  << endl;
    cout << "manifold = "   << (mesh.is_manifold() ? "yes" : "no") << endl;
    
    mesh.repair();  // this calculates number_of_parts
    if (mesh.needed_repair()) {
        mesh.repair();
        if (mesh.stl.stats.degenerate_facets > 0)
            cout << "degenerate_facets = "  << mesh.stl.stats.degenerate_facets << endl;
        if (mesh.stl.stats.edges_fixed > 0)
            cout << "edges_fixed = "        << mesh.stl.stats.edges_fixed       << endl;
        if (mesh.stl.stats.facets_removed > 0)
            cout << "facets_removed = "     << mesh.stl.stats.facets_removed    << endl;
        if (mesh.stl.stats.facets_added > 0)
            cout << "facets_added = "       << mesh.stl.stats.facets_added      << endl;
        if (mesh.stl.stats.facets_reversed > 0)
            cout << "facets_reversed = "    << mesh.stl.stats.facets_reversed   << endl;
        if (mesh.stl.stats.backwards_edges > 0)
            cout << "backwards_edges = "    << mesh.stl.stats.backwards_edges   << endl;
    }
    cout << "number_of_parts =  " << mesh.stl.stats.number_of_parts << endl;
    cout << "volume = "           << mesh.volume()                  << endl;
}


ModelVolume::ModelVolume(ModelObject* object, const TriangleMesh &mesh)
:   mesh(mesh), modifier(false), object(object)
{}

ModelVolume::ModelVolume(ModelObject* object, const ModelVolume &other)
:   name(other.name), mesh(other.mesh), config(other.config),
    modifier(other.modifier), object(object)
{
    this->material_id(other.material_id());
}

ModelVolume& ModelVolume::operator= (ModelVolume other)
{
    this->swap(other);
    return *this;
}

void
ModelVolume::swap(ModelVolume &other)
{
    std::swap(this->name,       other.name);
    std::swap(this->mesh,       other.mesh);
    std::swap(this->config,     other.config);
    std::swap(this->modifier,   other.modifier);
}

t_model_material_id
ModelVolume::material_id() const
{
    return this->_material_id;
}

void
ModelVolume::material_id(t_model_material_id material_id)
{
    this->_material_id = material_id;
    
    // ensure this->_material_id references an existing material
    (void)this->object->get_model()->add_material(material_id);
}

ModelMaterial*
ModelVolume::material() const
{
    return this->object->get_model()->get_material(this->_material_id);
}

void
ModelVolume::set_material(t_model_material_id material_id, const ModelMaterial &material)
{
    this->_material_id = material_id;
    (void)this->object->get_model()->add_material(material_id, material);
}

ModelMaterial*
ModelVolume::assign_unique_material()
{
    Model* model = this->get_object()->get_model();
    
    // as material-id "0" is reserved by the AMF spec we start from 1
    this->_material_id = 1 + model->materials.size();  // watchout for implicit cast
    return model->add_material(this->_material_id);
}


ModelInstance::ModelInstance(ModelObject *object)
:   rotation(0), x_rotation(0), y_rotation(0), scaling_factor(1),scaling_vector(Pointf3(1,1,1)), z_translation(0), object(object)
{}

ModelInstance::ModelInstance(ModelObject *object, const ModelInstance &other)
:   rotation(other.rotation), x_rotation(other.x_rotation), y_rotation(other.y_rotation), scaling_factor(other.scaling_factor), scaling_vector(other.scaling_vector), offset(other.offset), z_translation(other.z_translation), object(object)
{}

ModelInstance& ModelInstance::operator= (ModelInstance other)
{
    this->swap(other);
    return *this;
}

void
ModelInstance::swap(ModelInstance &other)
{
    std::swap(this->rotation,       other.rotation);
    std::swap(this->scaling_factor, other.scaling_factor);
    std::swap(this->scaling_vector, other.scaling_vector);
    std::swap(this->x_rotation, other.x_rotation);
    std::swap(this->y_rotation, other.y_rotation);
    std::swap(this->z_translation, other.z_translation);
    std::swap(this->offset,         other.offset);
}

void
ModelInstance::transform_mesh(TriangleMesh* mesh, bool dont_translate) const
{
    mesh->rotate_x(this->x_rotation);
    mesh->rotate_y(this->y_rotation);
    mesh->rotate_z(this->rotation);                 // rotate around mesh origin

    Pointf3 scale_versor = this->scaling_vector;
    scale_versor.scale(this->scaling_factor);
    mesh->scale(scale_versor);              // scale around mesh origin
    if (!dont_translate) {
        float z_trans = 0;
        // In 3mf models avoid keeping the objects under z = 0 plane.
        if (this->y_rotation || this->x_rotation)
            z_trans = -(mesh->stl.stats.min.z);
        mesh->translate(this->offset.x, this->offset.y, z_trans);
    }

}

BoundingBoxf3 ModelInstance::transform_mesh_bounding_box(const TriangleMesh* mesh, bool dont_translate) const
{
    // rotate around mesh origin
    double c = cos(this->rotation);
    double s = sin(this->rotation);
    double cx = cos(this->x_rotation);
    double sx = sin(this->x_rotation);
    double cy = cos(this->y_rotation);
    double sy = sin(this->y_rotation);
    BoundingBoxf3 bbox;
    for (int i = 0; i < mesh->stl.stats.number_of_facets; ++ i) {
        const stl_facet &facet = mesh->stl.facet_start[i];
        for (int j = 0; j < 3; ++ j) {
            stl_vertex v = facet.vertex[j];
            double xold = v.x;
            double yold = v.y;
            double zold = v.z;
            // Rotation around x axis.
            v.z = float(sx * yold + cx * zold);
            yold = v.y = float(cx * yold - sx * zold);
            zold = v.z;
            // Rotation around y axis.
            v.x = float(cy * xold + sy * zold);
            v.z = float(-sy * xold + cy * zold);
            xold = v.x;
            // Rotation around z axis.
            v.x = float(c * xold - s * yold);
            v.y = float(s * xold + c * yold);
            v.x *= float(this->scaling_factor * this->scaling_vector.x);
            v.y *= float(this->scaling_factor * this->scaling_vector.y);
            v.z *= float(this->scaling_factor * this->scaling_vector.z);
            if (!dont_translate) {
                v.x += this->offset.x;
                v.y += this->offset.y;
                if (this->y_rotation || this->x_rotation)
                    v.z += -(mesh->stl.stats.min.z);
            }
            bbox.merge(Pointf3(v.x, v.y, v.z));
        }
    }
    return bbox;
}

BoundingBoxf3 ModelInstance::transform_bounding_box(const BoundingBoxf3 &bbox, bool dont_translate) const
{
    // rotate around mesh origin
    double c = cos(this->rotation);
    double s = sin(this->rotation);
    double cx = cos(this->x_rotation);
    double sx = sin(this->x_rotation);
    double cy = cos(this->y_rotation);
    double sy = sin(this->y_rotation);
    Pointf3 pts[4] = {
        bbox.min,
        bbox.max,
        Pointf3(bbox.min.x, bbox.max.y, bbox.min.z),
        Pointf3(bbox.max.x, bbox.min.y, bbox.max.z)
    };
    BoundingBoxf3 out;
    for (int i = 0; i < 4; ++ i) {
        Pointf3 &v = pts[i];
        double xold = v.x;
        double yold = v.y;
        double zold = v.z;
        // Rotation around x axis.
        v.z = float(sx * yold + cx * zold);
        yold = v.y = float(cx * yold - sx * zold);
        zold = v.z;
        // Rotation around y axis.
        v.x = float(cy * xold + sy * zold);
        v.z = float(-sy * xold + cy * zold);
        xold = v.x;
        // Rotation around z axis.
        v.x = float(c * xold - s * yold);
        v.y = float(s * xold + c * yold);
        v.x *= this->scaling_factor * this->scaling_vector.x;
        v.y *= this->scaling_factor * this->scaling_vector.y;
        v.z *= this->scaling_factor * this->scaling_vector.z;
        if (!dont_translate) {
            v.x += this->offset.x;
            v.y += this->offset.y;
        }
        out.merge(v);
    }
    return out;
}

void
ModelInstance::transform_polygon(Polygon* polygon) const
{
    polygon->rotate(this->rotation, Point(0,0));    // rotate around polygon origin
    polygon->scale(this->scaling_factor);           // scale around polygon origin
}

}
