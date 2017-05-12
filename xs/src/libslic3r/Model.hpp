#ifndef slic3r_Model_hpp_
#define slic3r_Model_hpp_

#include "libslic3r.h"
#include "BoundingBox.hpp"
#include "PrintConfig.hpp"
#include "Layer.hpp"
#include "Point.hpp"
#include "TriangleMesh.hpp"
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace Slic3r {

class ModelInstance;
class ModelMaterial;
class ModelObject;
class ModelVolume;

typedef std::string t_model_material_id;
typedef std::string t_model_material_attribute;
typedef std::map<t_model_material_attribute,std::string> t_model_material_attributes;

typedef std::map<t_model_material_id,ModelMaterial*> ModelMaterialMap;
typedef std::vector<ModelObject*> ModelObjectPtrs;
typedef std::vector<ModelVolume*> ModelVolumePtrs;
typedef std::vector<ModelInstance*> ModelInstancePtrs;

// The print bed content.
// Description of a triangular model with multiple materials, multiple instances with various affine transformations
// and with multiple modifier meshes.
// A model groups multiple objects, each object having possibly multiple instances,
// all objects may share mutliple materials.
/// Model Class representing the print bed content
/// \brief Description of a triangular model with multiple materials, multiple instances with various affine transformations
/// and with multiple modifier meshes.
/// A model groups multiple objects, each object having possibly multiple instances,
/// all objects may share multiple materials.
class Model
{
    public:
    // Materials are owned by a model and referenced by objects through t_model_material_id.
    // Single material may be shared by multiple models.
    ModelMaterialMap materials; ///< \brief Materials are owned by a model and referenced by objects through t_model_material_id.
    ///< Single material may be shared by multiple models.
    // Objects are owned by a model. Each model may have multiple instances, each instance having its own transformation (shift, scale, rotation).
    ModelObjectPtrs objects; ///< \brief Objects are owned by a model. Each model may have multiple instances, each instance having its own transformation (shift, scale, rotation).

    /// Model constructor
    Model();

    /// Model constructor
    /// \param other model object
    Model(const Model &other);

    /// '=' Operator overloading
    /// \param other model object
    /// \return
    Model& operator= (Model other);

    /// Swap objects and materials with another model object
    /// \param other model object
    void swap(Model &other);

    /// Destructor
    ~Model();

    /// Read a model from file
    /// \brief This function supports the following formats (STL, OBJ, AMF)
    /// \param input_file
    /// \return a model object
    static Model read_from_file(std::string input_file);

    /// Create a new object and add it to the model
    /// \return a pointer to the new model object
    ModelObject* add_object();

    /// Create a new object and add it to the model
    /// \brief This function copies another model object
    /// \param other model object to be copied
    /// \param copy_volumes if you also want to copy volumes of the other object. By default = true
    /// \return a pointer to the new model object
    ModelObject* add_object(const ModelObject &other, bool copy_volumes = true);

    /// Delete a model object from the model
    /// \param idx the index of the desired object
    void delete_object(size_t idx);

    /// Delete all objects found in the current model
    void clear_objects();

    /// Add a new material to the model
    /// \param material_id the id of the new model material to be added
    /// \return a pointer to the new model material
    ModelMaterial* add_material(t_model_material_id material_id);

    /// Add a new material to the current model
    /// \brief This function copies another model material, It also delete the current material carrying the same
    /// material id in the map
    /// \param material_id the id of the new model material to be added
    /// \param other model material to be copied
    /// \return a pointer to the new model material
    ModelMaterial* add_material(t_model_material_id material_id, const ModelMaterial &other);

    /// Get the model material having a certain material id
    /// \brief Returns null if the model material is not found
    /// \param material_id the id of the needed model material
    /// \return a pointer to the model material or null if not found
    ModelMaterial* get_material(t_model_material_id material_id);

    /// Delete a model material carrying a certain material id if found
    /// \param material_id the id of the model material to be deleted
    void delete_material(t_model_material_id material_id);

    /// Delete all the model materials found in the current model
    void clear_materials();

    /// Check if any model object has no model instances
    /// \return boolean value where true means there exists at least one model object with no instances
    bool has_objects_with_no_instances() const;

    /// Add a new instance to the model objects having no model instances
    /// \return true
    bool add_default_instances();

    // Todo: @Samir Ask what are the transformed instances?
    /// Get the bounding box of the transformed instances
    /// \return a bounding box object
    BoundingBoxf3 bounding_box() const;

    /// Repair the model objects of the current model
    /// \brief This function calls repair function on each mesh of each model object volume
    void repair();

    // Todo: @Samir Check if this is correct
    /// Center each model object instance around a point
    /// \param point pointf object to center the model instances of model objects around
    void center_instances_around_point(const Pointf &point);

    // Todo: @Samir Check if this is correct
    /// Center each model object instance around the origin point
    void align_instances_to_origin();

    /// Translate each model object with x, y, z units
    /// \param x units in the x direction
    /// \param y units in the y direction
    /// \param z units in the z direction
    void translate(coordf_t x, coordf_t y, coordf_t z);

    // Todo: @Samir Ask about the difference
    /// Flatten all model objects of the current model to a single mesh
    /// \return a single triangle mesh object
    TriangleMesh mesh() const;

    /// Flatten all model objects of the current model to a single mesh
    /// \return a single triangle mesh object
    TriangleMesh raw_mesh() const;

    // Todo: @Samir Ask or try to figure out
    ///
    /// \param sizes
    /// \param dist
    /// \param bb
    /// \param out
    /// \return
    bool _arrange(const Pointfs &sizes, coordf_t dist, const BoundingBoxf* bb, Pointfs &out) const;

    // Todo: @Samir Ask or try to figure out
    /// Arrange model objects preserving their instance count but altering their instance positions
    /// \param dist
    /// \param bb
    /// \return
    bool arrange_objects(coordf_t dist, const BoundingBoxf* bb = NULL);
    // Croaks if the duplicated objects do not fit the print bed.

    // Todo: @Samir Ask or try to figure out
    /// Duplicate the entire model object preserving model instance relative positions
    /// \param copies_num number of copies
    /// \param dist distance between cells
    /// \param bb bounding box object
    void duplicate(size_t copies_num, coordf_t dist, const BoundingBoxf* bb = NULL);

    // Todo: @Samir Ask or try to figure out
    /// Duplicate the entire model object preserving model instance relative positions
    /// \brief This function will append more instances to each object
    /// and then automatically rearrange everything
    /// \param copies_num number of copies
    /// \param dist dist distance between cells
    /// \param bb bounding box object
    void duplicate_objects(size_t copies_num, coordf_t dist, const BoundingBoxf* bb = NULL);

    // Todo: @Samir Ask or try to figure out
    ///
    /// \param x
    /// \param y
    /// \param dist
    void duplicate_objects_grid(size_t x, size_t y, coordf_t dist);

    /// Print info about each model object in the model
    void print_info() const;

    // Todo: @Samir Ask or try to figure out
    ///
    /// \return
    bool looks_like_multipart_object() const;

    // Todo: @Samir Ask or try to figure out
    ///
    void convert_multipart_object();
};

// Material, which may be shared across multiple ModelObjects of a single Model.
/// Model Material class
/// <\brief Material, which may be shared across multiple ModelObjects of a single Model.
class ModelMaterial
{
    friend class Model;
    public:
    // Attributes are defined by the AMF file format, but they don't seem to be used by Slic3r for any purpose.
    t_model_material_attributes attributes; ///< Attributes are defined by the AMF file format, but they don't seem to be used by Slic3r for any purpose.
    // Dynamic configuration storage for the object specific configuration values, overriding the global configuration.
    // Todo: @Samir Ask
    DynamicPrintConfig config; ///< Dynamic configuration storage for the object specific configuration values, overriding the global configuration.

    /// Get the parent model woeing this material
    /// \return
    Model* get_model() const { return this->model; };

    /// Apply attributes defined by the AMF file format
    /// \param attributes the attributes map
    void apply(const t_model_material_attributes &attributes);
    
    private:
    // Parent, owning this material.
    Model* model; ///<Parent, owning this material.

    /// Constructor
    /// \param model the parent model owning this material.
    ModelMaterial(Model *model);

    /// Constructor
    /// \param model the parent model owning this material.
    /// \param other the other model material to be copied
    ModelMaterial(Model *model, const ModelMaterial &other);
};

// A printable object, possibly having multiple print volumes (each with its own set of parameters and materials),
// and possibly having multiple modifier volumes, each modifier volume with its set of parameters and materials.
// Each ModelObject may be instantiated mutliple times, each instance having different placement on the print bed,
// different rotation and different uniform scaling.
class ModelObject
{
    friend class Model;
    public:
    std::string name;
    std::string input_file;
    // Instances of this ModelObject. Each instance defines a shift on the print bed, rotation around the Z axis and a uniform scaling.
    // Instances are owned by this ModelObject.
    ModelInstancePtrs instances;
    // Printable and modifier volumes, each with its material ID and a set of override parameters.
    // ModelVolumes are owned by this ModelObject.
    ModelVolumePtrs volumes;
    // Configuration parameters specific to a single ModelObject, overriding the global Slic3r settings.
    DynamicPrintConfig config;
    // Variation of a layer thickness for spans of Z coordinates.
    t_layer_height_ranges layer_height_ranges;

    /* This vector accumulates the total translation applied to the object by the
        center_around_origin() method. Callers might want to apply the same translation
        to new volumes before adding them to this object in order to preserve alignment
        when user expects that. */
    Pointf3 origin_translation;
    
    // these should be private but we need to expose them via XS until all methods are ported
    BoundingBoxf3 _bounding_box;
    bool _bounding_box_valid;
    
    Model* get_model() const { return this->model; };
    
    ModelVolume* add_volume(const TriangleMesh &mesh);
    ModelVolume* add_volume(const ModelVolume &volume);
    void delete_volume(size_t idx);
    void clear_volumes();

    ModelInstance* add_instance();
    ModelInstance* add_instance(const ModelInstance &instance);
    void delete_instance(size_t idx);
    void delete_last_instance();
    void clear_instances();

    BoundingBoxf3 bounding_box();
    void invalidate_bounding_box();

    void repair();
    TriangleMesh mesh() const;
    TriangleMesh raw_mesh() const;
    BoundingBoxf3 raw_bounding_box() const;
    BoundingBoxf3 instance_bounding_box(size_t instance_idx) const;
    void align_to_ground();
    void center_around_origin();
    void translate(const Vectorf3 &vector);
    void translate(coordf_t x, coordf_t y, coordf_t z);
    void scale(float factor);
    void scale(const Pointf3 &versor);
    void scale_to_fit(const Sizef3 &size);
    void rotate(float angle, const Axis &axis);
    void mirror(const Axis &axis);
    void transform_by_instance(ModelInstance instance, bool dont_translate = false);
    size_t materials_count() const;
    size_t facets_count() const;
    bool needed_repair() const;
    void cut(Axis axis, coordf_t z, Model* model) const;
    void split(ModelObjectPtrs* new_objects);
    void update_bounding_box();   // this is a private method but we expose it until we need to expose it via XS
    void print_info() const;
    
    private:
    // Parent object, owning this ModelObject.
    Model* model;
    
    ModelObject(Model *model);
    ModelObject(Model *model, const ModelObject &other, bool copy_volumes = true);
    ModelObject& operator= (ModelObject other);
    void swap(ModelObject &other);
    ~ModelObject();
};

// An object STL, or a modifier volume, over which a different set of parameters shall be applied.
// ModelVolume instances are owned by a ModelObject.
class ModelVolume
{
    friend class ModelObject;
    public:
    std::string name;
    // The triangular model.
    TriangleMesh mesh;
    // Configuration parameters specific to an object model geometry or a modifier volume, 
    // overriding the global Slic3r settings and the ModelObject settings.
    DynamicPrintConfig config;
    // Is it an object to be printed, or a modifier volume?
    bool modifier;
    
    // A parent object owning this modifier volume.
    ModelObject* get_object() const { return this->object; };
    t_model_material_id material_id() const;
    void material_id(t_model_material_id material_id);
    ModelMaterial* material() const;
    void set_material(t_model_material_id material_id, const ModelMaterial &material);
    
    ModelMaterial* assign_unique_material();
    
    private:
    // Parent object owning this ModelVolume.
    ModelObject* object;
    t_model_material_id _material_id;
    
    ModelVolume(ModelObject *object, const TriangleMesh &mesh);
    ModelVolume(ModelObject *object, const ModelVolume &other);
    ModelVolume& operator= (ModelVolume other);
    void swap(ModelVolume &other);
};

// A single instance of a ModelObject.
// Knows the affine transformation of an object.
class ModelInstance
{
    friend class ModelObject;
    public:
    double rotation;            // Rotation around the Z axis, in radians around mesh center point
    double scaling_factor;
    Pointf offset;              // in unscaled coordinates
    
    ModelObject* get_object() const { return this->object; };

    // To be called on an external mesh
    void transform_mesh(TriangleMesh* mesh, bool dont_translate = false) const;
    // Calculate a bounding box of a transformed mesh. To be called on an external mesh.
    BoundingBoxf3 transform_mesh_bounding_box(const TriangleMesh* mesh, bool dont_translate = false) const;
    // Transform an external bounding box.
    BoundingBoxf3 transform_bounding_box(const BoundingBoxf3 &bbox, bool dont_translate = false) const;
    // To be called on an external polygon. It does not translate the polygon, only rotates and scales.
    void transform_polygon(Polygon* polygon) const;
    
    private:
    // Parent object, owning this instance.
    ModelObject* object;
    
    ModelInstance(ModelObject *object);
    ModelInstance(ModelObject *object, const ModelInstance &other);
    ModelInstance& operator= (ModelInstance other);
    void swap(ModelInstance &other);
};

}

#endif
