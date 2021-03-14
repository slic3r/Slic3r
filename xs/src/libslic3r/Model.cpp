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
    if (!boost::filesystem::exists(input_file)) {
        throw std::runtime_error("No such file");
    }
    
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

void
Model::merge(const Model &other)
{
    for (ModelObject* o : other.objects)
        this->add_object(*o, true);
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
    delete material;
    
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
    bool added = false;
    for (ModelObject* o : this->objects) {
        if (o->instances.empty()) {
            o->add_instance();
            added = true;
        }
    }
    return added;
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
Model::split()
{
    Model new_model;
    for (ModelObject* o : this->objects)
        o->split(&new_model.objects);
    
    this->clear_objects();
    for (ModelObject* o : new_model.objects)
        this->add_object(*o);
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
    for (const ModelObject* o : this->objects)
        for (size_t i = 0; i < o->instances.size(); ++i)
            instance_sizes.push_back(o->instance_bounding_box(i).size());
    
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
    for (ModelObject* o : this->objects) {
        // make a copy of the pointers in order to avoid recursion when appending their copies
        const auto instances = o->instances;
        for (const ModelInstance* i : instances)
            for (size_t k = 2; k <= copies_num; ++k)
                o->add_instance(*i);
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
            heights.insert(v->bounding_box().min.z);
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
    trafo_obj(other.trafo_obj),
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
    std::swap(this->trafo_obj,              other.trafo_obj);
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
        raw_bbox.merge((*v)->bounding_box());
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

Pointf3 
ModelObject::origin_translation() const
{
    return Pointf3(trafo_obj.m03, trafo_obj.m13, trafo_obj.m23);
}


// flattens all volumes and instances into a single mesh
TriangleMesh
ModelObject::mesh() const
{
    TriangleMesh mesh;

    for (ModelInstancePtrs::const_iterator i = this->instances.begin(); i != this->instances.end(); ++i) {
        TransformationMatrix instance_trafo = (*i)->get_trafo_matrix();
        for (ModelVolumePtrs::const_iterator v = this->volumes.begin(); v != this->volumes.end(); ++v) {
            mesh.merge((*v)->get_transformed_mesh(instance_trafo));
        }
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
    if (this->instances.empty()) CONFESS("Can't call raw_bounding_box() with no instances");
    TransformationMatrix trafo = this->instances.front()->get_trafo_matrix(true);
    for (ModelVolumePtrs::const_iterator v = this->volumes.begin(); v != this->volumes.end(); ++v) {
        if ((*v)->modifier) continue;
        bb.merge((*v)->get_transformed_bounding_box(trafo));
    }
    return bb;
}

// this returns the bounding box of the *transformed* given instance
BoundingBoxf3
ModelObject::instance_bounding_box(size_t instance_idx) const
{
    BoundingBoxf3 bb;
    if (this->instances.size()<=instance_idx) CONFESS("Can't call instance_bounding_box(index) with insufficient amount of instances");
    TransformationMatrix trafo = this->instances[instance_idx]->get_trafo_matrix(true);
    for (ModelVolumePtrs::const_iterator v = this->volumes.begin(); v != this->volumes.end(); ++v) {
        if ((*v)->modifier) continue;
        bb.merge((*v)->get_transformed_bounding_box(trafo));
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
Model::align_to_ground()
{
    BoundingBoxf3 bb = this->bounding_box();
    for (ModelObject* o : this->objects)
        o->translate(0, 0, -bb.min.z);
}

void
ModelObject::align_to_ground()
{
	BoundingBoxf3 bb;
	for (const ModelVolume* v : this->volumes)
		if (!v->modifier)
			bb.merge(v->bounding_box());
    
    this->translate(0, 0, -bb.min.z);
}

void
ModelObject::center_around_origin()
{
    // calculate the displacements needed to 
    // center this object around the origin
	BoundingBoxf3 bb;
	for (ModelVolumePtrs::const_iterator v = this->volumes.begin(); v != this->volumes.end(); ++v)
		if (! (*v)->modifier)
			bb.merge((*v)->bounding_box());
    
    // first align to origin on XYZ
    Vectorf3 vector(-bb.min.x, -bb.min.y, -bb.min.z);
    
    // then center it on XY
    Sizef3 size = bb.size();
    vector.x -= size.x/2;
    vector.y -= size.y/2;

    this->translate(vector);
    
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

TransformationMatrix
ModelObject::get_trafo_to_center() const
{
    BoundingBoxf3 raw_bb = this->raw_bounding_box();
    return TransformationMatrix::mat_translation(raw_bb.center().negative());
}

void
ModelObject::translate(const Vectorf3 &vector)
{
    this->translate(vector.x, vector.y, vector.z);
}

void
ModelObject::translate(coordf_t x, coordf_t y, coordf_t z)
{
    TransformationMatrix trafo = TransformationMatrix::mat_translation(x, y, z);
    this->apply_transformation(trafo);
    
    if (this->_bounding_box_valid) this->_bounding_box.translate(x, y, z);
}

void
ModelObject::scale(double factor)
{
    this->scale(Pointf3(factor, factor, factor));
}

void
ModelObject::scale(const Pointf3 &versor)
{
    if (versor.x == 1 && versor.y == 1 && versor.z == 1) return;

    TransformationMatrix center_trafo = this->get_trafo_to_center();
    TransformationMatrix trafo = TransformationMatrix::multiply(TransformationMatrix::mat_scale(versor.x, versor.y, versor.z), center_trafo);
    trafo.applyLeft(center_trafo.inverse());

    this->apply_transformation(trafo);

    this->invalidate_bounding_box();
}

void
ModelObject::scale_to_fit(const Sizef3 &size)
{
    Sizef3 orig_size = this->bounding_box().size();
    double factor = fminf(
        size.x / orig_size.x,
        fminf(
            size.y / orig_size.y,
            size.z / orig_size.z
        )
    );
    this->scale(factor);
}

void
ModelObject::rotate(double angle, const Axis &axis)
{
    if (angle == 0) return;

    TransformationMatrix center_trafo = this->get_trafo_to_center();
    TransformationMatrix trafo = TransformationMatrix::multiply(TransformationMatrix::mat_rotation(angle, axis), center_trafo);
    trafo.applyLeft(center_trafo.inverse());

    this->apply_transformation(trafo);
    
    this->invalidate_bounding_box();
}

void
ModelObject::rotate(double angle, const Vectorf3 &axis)
{
    if (angle == 0) return;

    TransformationMatrix center_trafo = this->get_trafo_to_center();
    TransformationMatrix trafo = TransformationMatrix::multiply(TransformationMatrix::mat_rotation(angle, axis), center_trafo);
    trafo.applyLeft(center_trafo.inverse());

    this->apply_transformation(trafo);
    
    this->invalidate_bounding_box();
}

void
ModelObject::rotate(const Vectorf3 &origin, const Vectorf3 &target)
{

    TransformationMatrix center_trafo = this->get_trafo_to_center();
    TransformationMatrix trafo = TransformationMatrix::multiply(TransformationMatrix::mat_rotation(origin, target), center_trafo);
    trafo.applyLeft(center_trafo.inverse());

    this->apply_transformation(trafo);
    
    this->invalidate_bounding_box();
}

void
ModelObject::mirror(const Axis &axis)
{
    TransformationMatrix center_trafo = this->get_trafo_to_center();
    TransformationMatrix trafo = TransformationMatrix::multiply(TransformationMatrix::mat_mirror(axis), center_trafo);
    trafo.applyLeft(center_trafo.inverse());
    
    this->apply_transformation(trafo);

    this->invalidate_bounding_box();
}

void ModelObject::reset_undo_trafo()
{
    this->trafo_undo_stack = TransformationMatrix::mat_eye();
}

TransformationMatrix ModelObject::get_undo_trafo() const
{
    return this->trafo_undo_stack;
}

void
ModelObject::apply_transformation(const TransformationMatrix & trafo)
{
    this->trafo_obj.applyLeft(trafo);
    this->trafo_undo_stack.applyLeft(trafo);
    for (ModelVolumePtrs::const_iterator v = this->volumes.begin(); v != this->volumes.end(); ++v) {
        (*v)->apply_transformation(trafo);
    }
}

void
ModelObject::transform_by_instance(ModelInstance instance, bool dont_translate)
{
    // We get instance by copy because we would alter it in the loop below,
    // causing inconsistent values in subsequent instances.
    TransformationMatrix temp_trafo = instance.get_trafo_matrix(dont_translate);

    this->apply_transformation(temp_trafo);

    temp_trafo = temp_trafo.inverse();
    
    /*
      Let:
        * I1 be the trafo of the given instance, 
        * V the original volume trafo and
        * I2 the trafo of the instance to be updated
      
      Then:
        previous: T = I2 * V
        I1 has been applied to V:
            Vnew = I1 * V
            I1^-1 * I1 = eye

            T = I2 * I1^-1 * I1 * V
                ----------   ------
                   I2new      Vnew
    */

    for (ModelInstance* i : this->instances) {
        i->set_complete_trafo(i->get_trafo_matrix().multiplyRight(temp_trafo));
    }
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
    
    // remove extension from filename and add suffix
    if (this->input_file.empty()) {
        upper->input_file = "upper";
        lower->input_file = "lower";
    } else {
        const boost::filesystem::path p{this->input_file};
        upper->input_file = (p.parent_path() / p.stem()).string() + "_upper";
        lower->input_file = (p.parent_path() / p.stem()).string() + "_lower";
    }
    
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
    
    TriangleMesh mesh = this->mesh();
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
:   mesh(mesh), input_file(""), modifier(false), object(object)
{}

ModelVolume::ModelVolume(ModelObject* object, const ModelVolume &other)
:   name(other.name),
    mesh(other.mesh),
    trafo(other.trafo),
    config(other.config),
    input_file(other.input_file),
    input_file_obj_idx(other.input_file_obj_idx),
    input_file_vol_idx(other.input_file_vol_idx),
    modifier(other.modifier),
    object(object)
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
    std::swap(this->trafo,      other.trafo);
    std::swap(this->config,     other.config);
    std::swap(this->modifier,   other.modifier);
	
	std::swap(this->input_file,            other.input_file);
	std::swap(this->input_file_obj_idx,    other.input_file_obj_idx);
	std::swap(this->input_file_vol_idx,    other.input_file_vol_idx);
}

TriangleMesh
ModelVolume::get_transformed_mesh(TransformationMatrix const & trafo) const
{
    return this->mesh.get_transformed_mesh(trafo);
}

BoundingBoxf3
ModelVolume::get_transformed_bounding_box(TransformationMatrix const & trafo) const
{
    return this->mesh.get_transformed_bounding_box(trafo);
}

BoundingBoxf3
ModelVolume::bounding_box() const
{
    return this->mesh.bounding_box();
}

void ModelVolume::translate(double x, double y, double z)
{
    TransformationMatrix trafo = TransformationMatrix::mat_translation(x,y,z);
    this->apply_transformation(trafo);
}

void ModelVolume::scale(double x, double y, double z)
{
    TransformationMatrix trafo = TransformationMatrix::mat_scale(x,y,z);
    this->apply_transformation(trafo);
}

void ModelVolume::mirror(const Axis &axis)
{
    TransformationMatrix trafo = TransformationMatrix::mat_mirror(axis);
    this->apply_transformation(trafo);
}

void ModelVolume::mirror(const Vectorf3 &normal)
{
    TransformationMatrix trafo = TransformationMatrix::mat_mirror(normal);
    this->apply_transformation(trafo);
}

void ModelVolume::rotate(double angle_rad, const Axis &axis)
{
    TransformationMatrix trafo = TransformationMatrix::mat_rotation(angle_rad, axis);
    this->apply_transformation(trafo);
}

void ModelVolume::apply_transformation(TransformationMatrix const & trafo)
{
    this->mesh.transform(trafo);
    this->trafo.applyLeft(trafo);
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
:   rotation(0), scaling_factor(1), object(object)
{}

ModelInstance::ModelInstance(ModelObject *object, const TransformationMatrix & trafo)
:   object(object)
{
    this->set_complete_trafo(trafo);
}


ModelInstance::ModelInstance(ModelObject *object, const ModelInstance &other)
:   rotation(other.rotation), scaling_factor(other.scaling_factor), offset(other.offset), additional_trafo(other.additional_trafo), object(object)
{}

ModelInstance& ModelInstance::operator= (ModelInstance other)
{
    this->swap(other);
    return *this;
}

void
ModelInstance::swap(ModelInstance &other)
{
    std::swap(this->rotation,         other.rotation);
    std::swap(this->scaling_factor,   other.scaling_factor);
    std::swap(this->offset,           other.offset);
    std::swap(this->additional_trafo, other.additional_trafo);
}

void ModelInstance::set_complete_trafo(TransformationMatrix const & trafo)
{
    // Extraction code moved from TMF class

    this->offset.x = trafo.m03;
    this->offset.y = trafo.m13;

    // Get the scale values.
    double sx = sqrt( trafo.m00 * trafo.m00 + trafo.m01 * trafo.m01 + trafo.m02 * trafo.m02),
        sy = sqrt( trafo.m10 * trafo.m10 + trafo.m11 * trafo.m11 + trafo.m12 * trafo.m12),
        sz = sqrt( trafo.m20 * trafo.m20 + trafo.m21 * trafo.m21 + trafo.m22 * trafo.m22);
    
    this->scaling_factor = (sx + sy + sz) / 3;

    // Get the rotation values.
    // Normalize scale from the matrix.
    TransformationMatrix rotmat = trafo.multiplyLeft(TransformationMatrix::mat_scale(1/sx, 1/sy, 1/sz));

    // Get quaternion values
    double q_w = sqrt(std::max(0.0, 1.0 + rotmat.m00 + rotmat.m11 + rotmat.m22)) / 2,
        q_x = sqrt(std::max(0.0, 1.0 + rotmat.m00 - rotmat.m11 - rotmat.m22)) / 2,
        q_y = sqrt(std::max(0.0, 1.0 - rotmat.m00 + rotmat.m11 - rotmat.m22)) / 2,
        q_z = sqrt(std::max(0.0, 1.0 - rotmat.m00 - rotmat.m11 + rotmat.m22)) / 2;

    q_x *= ((q_x * (rotmat.m21 - rotmat.m12)) <= 0 ? -1 : 1);
    q_y *= ((q_y * (rotmat.m02 - rotmat.m20)) <= 0 ? -1 : 1);
    q_z *= ((q_z * (rotmat.m10 - rotmat.m01)) <= 0 ? -1 : 1);

    // Normalize quaternion values.
    double q_magnitude = sqrt(q_w * q_w + q_x * q_x + q_y * q_y + q_z * q_z);
    q_w /= q_magnitude;
    q_x /= q_magnitude;
    q_y /= q_magnitude;
    q_z /= q_magnitude;

    double test = q_x * q_y + q_z * q_w;
    double result_z;
    // singularity at north pole
    if (test > 0.499)
    {
        result_z = PI / 2;
    }
        // singularity at south pole
    else if (test < -0.499)
    {
        result_z = -PI / 2;
    }
    else
    {
        result_z = asin(2 * q_x * q_y + 2 * q_z * q_w);

        if (result_z < 0) result_z += 2 * PI;
    }
    
    this->rotation = result_z;

    this->additional_trafo = TransformationMatrix::mat_eye();

    // Complete = Instance * Additional
    // -> Instance^-1 * Complete = (Instance^-1 * Instance) * Additional
    // -> Instance^-1 * Complete = Additional
    this->additional_trafo = TransformationMatrix::multiply(
        this->get_trafo_matrix().inverse(),
        trafo);

}

void
ModelInstance::transform_mesh(TriangleMesh* mesh, bool dont_translate) const
{
    TransformationMatrix trafo = this->get_trafo_matrix(dont_translate);
    mesh->transform(trafo);
}

TransformationMatrix ModelInstance::get_trafo_matrix(bool dont_translate) const
{
    TransformationMatrix trafo = this->additional_trafo;
    trafo.applyLeft(TransformationMatrix::mat_rotation(this->rotation, Axis::Z));
    trafo.applyLeft(TransformationMatrix::mat_scale(this->scaling_factor));
    if(!dont_translate)
    {
        trafo.applyLeft(TransformationMatrix::mat_translation(this->offset.x, this->offset.y, 0));
    }
    return trafo;
}

BoundingBoxf3 ModelInstance::transform_bounding_box(const BoundingBoxf3 &bbox, bool dont_translate) const
{
    TransformationMatrix trafo = this->get_trafo_matrix(dont_translate);
    Pointf3 Poi_min = bbox.min;
    Pointf3 Poi_max = bbox.max;

    // all 8 corner points needed because the transformation could be anything
    Pointf3 pts[8] = {
        Pointf3(Poi_min.x, Poi_min.y, Poi_min.z),
        Pointf3(Poi_min.x, Poi_min.y, Poi_max.z),
        Pointf3(Poi_min.x, Poi_max.y, Poi_min.z),
        Pointf3(Poi_min.x, Poi_max.y, Poi_max.z),
        Pointf3(Poi_max.x, Poi_min.y, Poi_min.z),
        Pointf3(Poi_max.x, Poi_min.y, Poi_max.z),
        Pointf3(Poi_max.x, Poi_max.y, Poi_min.z),
        Pointf3(Poi_max.x, Poi_max.y, Poi_max.z)
    };
    BoundingBoxf3 out;
    for (int i = 0; i < 8; ++ i) {
        out.merge(trafo.transform(pts[i]));
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
