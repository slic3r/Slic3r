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


/// Model Class representing the print bed content
/// \brief Description of a triangular model with multiple materials, multiple instances with various affine transformations
/// and with multiple modifier meshes.
/// A model groups multiple objects, each object having possibly multiple instances,
/// all objects may share multiple materials.
class Model
{
    public:
    ModelMaterialMap materials; ///< \brief Materials are owned by a model and referenced by objects through t_model_material_id.
    ///< Single material may be shared by multiple models.

    ModelObjectPtrs objects; ///< \brief Objects are owned by a model. Each model may have multiple instances, each instance having its own transformation (shift, scale, rotation).

    /// Model constructor
    Model();

    /// Model constructor
    /// \param other model
    Model(const Model &other);

    /// '=' Operator overloading
    /// \param other model
    /// \return
    Model& operator= (Model other);

    /// Swap objects and materials with another model
    /// \param other model
    void swap(Model &other);

    /// Model destructor
    ~Model();

    /// \brief Read a model from file
    /// \brief This function supports the following formats (STL, OBJ, AMF)
    /// \param input_file
    /// \return a model
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

    /// Delete all model objects found in the current model
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

/// Model Material class
/// <\brief Material, which may be shared across multiple ModelObjects of a single Model.
class ModelMaterial
{
    friend class Model;
    public:
    t_model_material_attributes attributes; ///< Attributes are defined by the AMF file format, but they don't seem to be used by Slic3r for any purpose.
    // Todo: @Samir Ask
    DynamicPrintConfig config; ///< Dynamic configuration storage for the object specific configuration values, overriding the global configuration.

    /// Get the parent model woeing this material
    /// \return
    Model* get_model() const { return this->model; };

    /// Apply attributes defined by the AMF file format
    /// \param attributes the attributes map
    void apply(const t_model_material_attributes &attributes);
    
    private:
    Model* model; ///<Parent, owning this material.

    /// Constructor
    /// \param model the parent model owning this material.
    ModelMaterial(Model *model);

    /// Constructor
    /// \param model the parent model owning this material.
    /// \param other the other model material to be copied
    ModelMaterial(Model *model, const ModelMaterial &other);
};

/// Model Object class
/// A printable object, possibly having multiple print volumes (each with its own set of parameters and materials),
/// and possibly having multiple modifier volumes, each modifier volume with its set of parameters and materials.
/// Each ModelObject may be instantiated mutliple times, each instance having different placement on the print bed,
/// different rotation and different uniform scaling.
class ModelObject
{
    friend class Model;
    public:
    std::string name; ///< model object name
    std::string input_file; ///< Input file path

    ModelInstancePtrs instances; ///< Instances of this ModelObject. Each instance defines a shift on the print bed, rotation around the Z axis and a uniform scaling.
    ///< Instances are owned by this ModelObject.

    //Todo @Samir ASK what are modifiers?
    ModelVolumePtrs volumes; ///< Printable and modifier volumes, each with its material ID and a set of override parameters.
    ///< \brief ModelVolumes are owned by this ModelObject.

    //Todo @Samir ASK
    DynamicPrintConfig config; ///< Configuration parameters specific to a single ModelObject, overriding the global Slic3r settings.

    //Todo @Samir ASK
    t_layer_height_ranges layer_height_ranges; ///< Variation of a layer thickness for spans of Z coordinates.

    //Todo @Samir ASK
    Pointf3 origin_translation; ///< This vector accumulates the total translation applied to the object by the
    /// center_around_origin() method. Callers might want to apply the same translation
    /// to new volumes before adding them to this object in order to preserve alignment
    /// when user expects that.


    // these should be private but we need to expose them via XS until all methods are ported
    BoundingBoxf3 _bounding_box;
    bool _bounding_box_valid;  //Todo @Samir ASK

    ///  Get the owning parent Model
    /// \return parent Model
    Model* get_model() const { return this->model; };

    /// Add a new ModelVolume to the current ModelObject
    /// \param mesh TriangularMesh
    /// \return ModelVolume* pointer to the new volume
    ModelVolume* add_volume(const TriangleMesh &mesh);

    /// Add a new ModelVolume to the current ModelObject
    /// \param volume the ModelVolume object to be copied
    /// \return ModelVolume* pointer to the new volume
    ModelVolume* add_volume(const ModelVolume &volume);

    /// Delete a ModelVolume object
    /// \param idx size_t the index of the ModelVolume to be deleted
    void delete_volume(size_t idx);

    /// Delete all ModelVolumes in the
    void clear_volumes();

    /// Add a new ModelInstance to the current ModelObject
    /// \return ModelInstance* a pointer to the new instance
    ModelInstance* add_instance();

    /// Add a new ModelInstance to the current ModelObject
    /// \param instance the ModelInstance to be copied
    /// \return ModelInstance* a pointer to the new instance
    ModelInstance* add_instance(const ModelInstance &instance);

    /// Delete a ModelInstance
    /// \param idx size_t the index of the ModelInstance to be deleted
    void delete_instance(size_t idx);

    //Todo @Samir ASK
    /// Delete the last created ModelInstance object
    void delete_last_instance();

    /// Delete all ModelInstance objects found in the current ModelObject
    void clear_instances();

    /// Return the bounding box of the *transformed* instances
    BoundingBoxf3 bounding_box();

    /// Invalidate the bounding box in the current ModelObject
    void invalidate_bounding_box();

    /// Repair all TriangleMesh objects found in each ModelVolume
    void repair();

    /// Flatten all volumes and instances into a single mesh and applying all the
    //Todo @Samir ASK
    TriangleMesh mesh() const;

    /// Flatten all volumes and instances into a single mesh
    TriangleMesh raw_mesh() const;

    ///
    BoundingBoxf3 raw_bounding_box() const;

    ///
    BoundingBoxf3 instance_bounding_box(size_t instance_idx) const;

    ///
    void align_to_ground();

    ///
    void center_around_origin();

    ///
    /// \param vector
    void translate(const Vectorf3 &vector);

    ///
    /// \param x
    /// \param y
    /// \param z
    void translate(coordf_t x, coordf_t y, coordf_t z);

    ///
    /// \param factor
    void scale(float factor);

    ///
    /// \param versor
    void scale(const Pointf3 &versor);

    ///
    /// \param size
    void scale_to_fit(const Sizef3 &size);

    ///
    /// \param angle
    /// \param axis
    void rotate(float angle, const Axis &axis);

    ///
    /// \param axis
    void mirror(const Axis &axis);

    ///
    /// \param instance
    /// \param dont_translate
    void transform_by_instance(ModelInstance instance, bool dont_translate = false);

    ///
    /// \return
    size_t materials_count() const;

    ///
    /// \return
    size_t facets_count() const;

    ///
    bool needed_repair() const;

    ///
    /// \param axis
    /// \param z
    /// \param model
    void cut(Axis axis, coordf_t z, Model* model) const;

    ///
    /// \param new_objects
    void split(ModelObjectPtrs* new_objects);

    ///
    void update_bounding_box();   // this is a private method but we expose it until we need to expose it via XS

    ///
    void print_info() const;
    
    private:
    // Parent object, owning this ModelObject.
    Model* model; ///<

    ///
    /// \param model
    ModelObject(Model *model);

    ///
    /// \param model
    /// \param other
    /// \param copy_volumes
    ModelObject(Model *model, const ModelObject &other, bool copy_volumes = true);

    ///
    /// \param other
    /// \return
    ModelObject& operator= (ModelObject other);

    ///
    /// \param other
    void swap(ModelObject &other);

    ///
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

/// A single instance of a ModelObject.
/// Knows the affine transformation of an object.
class ModelInstance
{
    friend class ModelObject;   ///< Rotation around the Z axis, in radians around mesh center point
    public:
    double rotation;            ///< Rotation around the Z axis, in radians around mesh center point
    double scaling_factor;      ///< scaling factor
    Pointf offset;              ///< offset in unscaled coordinates

    /// Get the owning ModelObject
    /// \return ModelObject* pointer to the owing ModelObject
    ModelObject* get_object() const { return this->object; };

    /// Transform an external TriangleMesh object
    /// \param mesh TriangleMesh* pointer to the the mesh
    /// \param dont_translate bool whether to translate the mesh or not
    void transform_mesh(TriangleMesh* mesh, bool dont_translate = false) const;

    // Todo @samir Ask
    /// Calculate a bounding box of a transformed mesh. To be called on an external mesh.
    /// \param mesh TriangleMesh* pointer to the the mesh
    /// \param dont_translate bool whether to translate the bounding box or not
    /// \return BoundingBoxf3 the bounding box after transformation
    BoundingBoxf3 transform_mesh_bounding_box(const TriangleMesh* mesh, bool dont_translate = false) const;

    /// Transform an external bounding box.
    /// \param bbox BoundingBoxf3 the bounding box to be transformed
    /// \param dont_translate bool whether to translate the bounding box or not
    /// \return BoundingBoxf3 the bounding box after transformation
    BoundingBoxf3 transform_bounding_box(const BoundingBoxf3 &bbox, bool dont_translate = false) const;

    /// Rotate or scale an external polygon. It does not translate the polygon.
    /// \param polygon Polygon* a pointer to the Polygon
    void transform_polygon(Polygon* polygon) const;
    
    private:
    ModelObject* object; ///< Parent object, owning this instance.

    /// Constructor
    /// \param object ModelObject* pointer to the owner ModelObject
    ModelInstance(ModelObject *object);

    /// Constructor
    /// \param object ModelObject* pointer to the owner ModelObject
    /// \param other ModelInstance an instance to be copied in the new ModelInstance object
    ModelInstance(ModelObject *object, const ModelInstance &other);

    /// = Operator overloading
    /// \param other ModelInstance an instance to be copied in the cuurent ModelInstance object
    /// \return ModelInstance& the current ModelInstance to enable operator cascading
    ModelInstance& operator= (ModelInstance other);

    /// Swap attributes between another ModelInstance object
    /// \param other ModelInstance& the other instance object
    void swap(ModelInstance &other);
};

}

#endif
