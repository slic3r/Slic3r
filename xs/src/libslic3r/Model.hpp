#ifndef slic3r_Model_hpp_
#define slic3r_Model_hpp_

#include "libslic3r.h"
#include "BoundingBox.hpp"
#include "PrintConfig.hpp"
#include "Layer.hpp"
#include "Point.hpp"
#include "TriangleMesh.hpp"
#include "LayerHeightSpline.hpp"
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
/// Description of a triangular model with multiple materials, multiple instances with various affine transformations
/// and with multiple modifier meshes.
/// A model groups multiple objects, each object having possibly multiple instances,
/// all objects may share multiple materials.
class Model
{
    public:
    ModelMaterialMap materials;
    ///< Materials are owned by a model and referenced by objects through t_model_material_id.
    ///< Single material may be shared by multiple models.

    ModelObjectPtrs objects;
    ///< Objects are owned by a model. Each object may have multiple instances
    ///< , each instance having its own transformation (shift, scale, rotation).

    std::map<std::string, std::string> metadata;
    ///< Model metadata <name, value>, this is needed for 3MF format read/write.

    /// Model constructor.
    Model();

    /// Model constructor.
    /// \param other Model the model to be copied
    Model(const Model &other);

    /// = Operator overloading.
    /// \param other Model the model to be copied
    /// \return Model& the current Model to enable operator cascading
    Model& operator= (Model other);

    /// Swap objects and materials with another model.
    /// \param other Model the model to be swapped with
    void swap(Model &other);

    /// Model destructor
    ~Model();

    /// Read a model from file.
    /// This function supports the following formats (STL, OBJ, AMF), It auto-detects the file format from the file name suffix.
    /// \param  input_file std::string the file path expressed in UTF-8
    /// \return Model the read Model
    static Model read_from_file(std::string input_file);

    /// Create a new object and add it to the current Model.
    /// \return ModelObject* a pointer to the new Model
    ModelObject* add_object();

    /// Create a new object and add it to the current Model.
    /// This function copies another model object
    /// \param other ModelObject the ModelObject to be copied
    /// \param copy_volumes if you also want to copy volumes of the other object. By default = true
    /// \return ModelObject* a pointer to the new ModelObject
    ModelObject* add_object(const ModelObject &other, bool copy_volumes = true);

    /// Delete a ModelObject from the current Model.
    /// \param idx size_t the index of the desired ModelObject
    void delete_object(size_t idx);

    /// Delete all ModelObjects found in the current Model.
    void clear_objects();

    /// Add a new ModelMaterial to the model.
    /// \param material_id t_model_material_id the id of the new ModelMaterial to be added
    /// \return ModelMaterial* a pointer to the new ModelMaterial
    ModelMaterial* add_material(t_model_material_id material_id);

    /// Add a new ModelMaterial to the current Model.
    /// This function copies another ModelMaterial, It also delete the current ModelMaterial carrying the same
    /// material id in the map.
    /// \param material_id t_model_material_id the id of the new ModelMaterial to be added
    /// \param other ModelMaterial the model material to be copied
    /// \return ModelMaterial* a pointer to the new ModelMaterial
    ModelMaterial* add_material(t_model_material_id material_id, const ModelMaterial &other);

    /// Get the ModelMaterial object instance having a certain material id.
    /// Returns null if the ModelMaterial object instance is not found.
    /// \param material_id t_model_material_id the id of the needed ModelMaterial object instance
    /// \return ModelMaterial* a pointer to the ModelMaterial object instance or null if not found
    ModelMaterial* get_material(t_model_material_id material_id);

    /// Delete a ModelMaterial carrying a certain material id if found.
    /// \param material_id t_model_material_id the id of the ModelMaterial to be deleted
    void delete_material(t_model_material_id material_id);

    /// Delete all the ModelMaterial objects found in the current Model.
    void clear_materials();

    /// Check if any ModelObject has no ModelInstances.
    /// \return bool true means there exists at least one ModelObject with no ModelInstance objects
    bool has_objects_with_no_instances() const;

    /// Add a new ModelInstance to each ModelObject having no ModelInstance objects
    /// \return bool
    bool add_default_instances();

    /// Get the bounding box of the transformed instances.
    /// \return BoundingBoxf3 a bounding box object.
    BoundingBoxf3 bounding_box() const;

    /// Repair the ModelObjects of the current Model.
    /// This function calls repair function on each TriangleMesh of each model object volume
    void repair();

    /// Center the total bounding box of the instances around a point.
    /// This transformation works in the XY plane only and no transformation in Z is performed.
    /// \param point pointf object to center the model instances of model objects around
    void center_instances_around_point(const Pointf &point);

    void align_instances_to_origin();

    /// Translate each ModelObject with x, y, z units.
    /// \param x coordf_t units in the x direction
    /// \param y coordf_t units in the y direction
    /// \param z coordf_t units in the z direction
    void translate(coordf_t x, coordf_t y, coordf_t z);

    /// Flatten all ModelInstances to a single mesh
    /// after performing instance transformations (if the object was rotated or translated).
    /// \return TriangleMesh a single TriangleMesh object
    TriangleMesh mesh() const;

    /// Flatten all ModelVolumes to a single mesh without any extra processing (i.e. without applying any instance duplication and/or transformation).
    /// \return TriangleMesh a single TriangleMesh object
    TriangleMesh raw_mesh() const;

    /// Arrange ModelInstances. ModelInstances of the same ModelObject do not preserve their relative positions.
    /// It uses the given BoundingBoxf as a hint, but falls back to free arrangement if it's not possible to fit all the parts in it.
    /// \param sizes Pointfs& number of parts
    /// \param dist coordf_t distance between cells
    /// \param bb BoundingBoxf* (optional) pointer to the bounding box of the area to fill
    /// \param out Pointfs& vector of the output positions
    /// \return bool whether the function finished arranging objects or it is impossible to arrange
    bool _arrange(const Pointfs &sizes, coordf_t dist, const BoundingBoxf* bb, Pointfs &out) const;

    /// Arrange ModelObjects preserving their ModelInstance count but altering their ModelInstance positions.
    /// \param dist coordf_t distance between cells
    /// \param bb BoundingBoxf* (optional) pointer to the bounding box of the area to fill
    /// \return bool whether the function finished arranging objects or it is impossible to arrange
    bool arrange_objects(coordf_t dist, const BoundingBoxf* bb = NULL);

    /// Duplicate the ModelInstances of each ModelObject as a whole preserving their relative positions.
    /// This function croaks if the duplicated objects do not fit the print bed.
    /// \param copies_num size_t number of copies
    /// \param dist coordf_t distance between cells
    /// \param bb BoundingBoxf* (optional) pointer to the bounding box of the area to fill
    void duplicate(size_t copies_num, coordf_t dist, const BoundingBoxf* bb = NULL);

    /// Duplicate each entire ModelInstances of the each ModelObject as a whole.
    /// This function will append more instances to each object
    /// and then calls arrange_objects() function to automatically rearrange everything.
    /// \param copies_num size_t number of copies
    /// \param dist coordf_t distance between cells
    /// \param bb BoundingBoxf* (optional) pointer to the bounding box of the area to fill
    void duplicate_objects(size_t copies_num, coordf_t dist, const BoundingBoxf* bb = NULL);


    /// Duplicate a single ModelObject and arranges them on a grid.
    /// Grid duplication is not supported with multiple objects. It throws an exception if there is more than one ModelObject.
    /// It also throws an exception if there are no ModelObjects in the current Model.
    /// \param x size_t number of duplicates in x direction
    /// \param y size_t offset  number of duplicates in y direction
    /// \param dist coordf_t distance supposed to be between the duplicated ModelObjects
    void duplicate_objects_grid(size_t x, size_t y, coordf_t dist);

    /// This function calls the print_info() function of each ModelObject.
    void print_info() const;

    /// Check to see if the current Model has characteristics of having multiple parts (usually multiple volumes, etc).
    /// \return bool
    bool looks_like_multipart_object() const;


    /// Take all of the ModelObjects in the current Model and combines them into a single ModelObject
    void convert_multipart_object();
};

/// Model Material class
/// Material, which may be shared across multiple ModelObjects of a single Model.
class ModelMaterial
{
    friend class Model;
    public:
    t_model_material_attributes attributes;
    ///< Attributes are defined by the AMF file format, but they don't seem to be used by Slic3r for any purpose.

    DynamicPrintConfig config;
    ///< Dynamic configuration storage for the object specific configuration values, overriding the global configuration.

    /// Get the parent model owing this material
    /// \return Model* the onwer Model
    Model* get_model() const { return this->model; };

    /// Apply attributes defined by the AMF file format
    /// \param attributes t_model_material_attributes the attributes map
    void apply(const t_model_material_attributes &attributes);

    private:
    Model* model; ///<Parent, owning this material.

    /// Constructor
    /// \param model the parent model owning this material.
    ModelMaterial(Model *model);

    /// Constructor
    /// \param model Model* the parent model owning this material.
    /// \param other ModelMaterial& the other model material to be copied
    ModelMaterial(Model *model, const ModelMaterial &other);
};

/// Model Object class
/// A printable object, possibly having multiple print volumes (each with its own set of parameters and materials),
/// and possibly having multiple modifier volumes, each modifier volume with its set of parameters and materials.
/// Each ModelObject may be instantiated multiple times, each instance having different placement on the print bed,
/// different rotation and different uniform scaling.
class ModelObject
{
    friend class Model;
    public:
    std::string name;
    ///< This ModelObject name.

    std::string input_file;
    ///< Input file path.

    ModelInstancePtrs instances;
    ///< Instances of this ModelObject. Each instance defines a shift on the print bed, rotation around the Z axis and a uniform scaling.
    ///< Instances are owned by this ModelObject.

    ModelVolumePtrs volumes;
    ///< Printable and modifier volumes, each with its material ID and a set of override parameters.
    ///< ModelVolumes are owned by this ModelObject.

    DynamicPrintConfig config; ///< Configuration parameters specific to a single ModelObject, overriding the global Slic3r settings.

    t_layer_height_ranges layer_height_ranges; ///< Variation of a layer thickness for spans of Z coordinates.

    int part_number; ///< It's used for the 3MF items part numbers in the build element.
    LayerHeightSpline layer_height_spline;     ///< Spline based variations of layer thickness for interactive user manipulation

    Pointf3 origin_translation;
    ///< This vector accumulates the total translation applied to the object by the
    ///< center_around_origin() method. Callers might want to apply the same translation
    ///< to new volumes before adding them to this object in order to preserve alignment
    ///< when user expects that.

    // these should be private but we need to expose them via XS until all methods are ported
    BoundingBoxf3 _bounding_box;
    bool _bounding_box_valid;

    ///  Get the owning parent Model.
    /// \return parent Model* pointer to the owner Model
    Model* get_model() const { return this->model; };

    /// Add a new ModelVolume to the current ModelObject. The mesh is copied into the newly created ModelVolume.
    /// \param mesh TriangularMesh
    /// \return ModelVolume* pointer to the new volume
    ModelVolume* add_volume(const TriangleMesh &mesh);

    /// Add a new ModelVolume to the current ModelObject.
    /// \param volume the ModelVolume object to be copied
    /// \return ModelVolume* pointer to the new volume
    ModelVolume* add_volume(const ModelVolume &volume);

    /// Delete a ModelVolume object.
    /// \param idx size_t the index of the ModelVolume to be deleted
    void delete_volume(size_t idx);

    /// Delete all ModelVolumes in the
    void clear_volumes();

    /// Add a new ModelInstance to the current ModelObject.
    /// \return ModelInstance* a pointer to the new instance
    ModelInstance* add_instance();

    /// Add a new ModelInstance to the current ModelObject.
    /// \param instance the ModelInstance to be copied
    /// \return ModelInstance* a pointer to the new instance
    ModelInstance* add_instance(const ModelInstance &instance);

    /// Delete a ModelInstance.
    /// \param idx size_t the index of the ModelInstance to be deleted
    void delete_instance(size_t idx);

    /// Delete the last created ModelInstance object.
    void delete_last_instance();

    /// Delete all ModelInstance objects found in the current ModelObject.
    void clear_instances();

    /// Get the bounding box of the *transformed* instances.
    BoundingBoxf3 bounding_box();

    /// Invalidate the bounding box in the current ModelObject.
    void invalidate_bounding_box();

    /// Repair all TriangleMesh objects found in each ModelVolume.
    void repair();

    /// Flatten all volumes and instances into a single mesh and applying all the ModelInstances transformations.
    TriangleMesh mesh() const;

    /// Flatten all volumes into a single mesh.
    TriangleMesh raw_mesh() const;

    /// Get the raw bounding box.
    /// This function croaks when there are no ModelInstances for this ModelObject
    /// \return BoundingBoxf3
    BoundingBoxf3 raw_bounding_box() const;

    /// Get the bounding box of the *transformed* given instance.
    /// \param instance_idx size_t the index of the ModelInstance in the ModelInstance vector
    /// \return BoundingBoxf3 the bounding box at the given index
    BoundingBoxf3 instance_bounding_box(size_t instance_idx) const;

    /// Align the current ModelObject to ground by translating the ModelVolumes in the z axis the needed units.
    void align_to_ground();

    /// Center the current ModelObject to origin by translating the ModelVolumes
    void center_around_origin();

    /// Translate the current ModelObject by translating ModelVolumes with (x,y,z) units.
    /// This function calls translate(coordf_t x, coordf_t y, coordf_t z) to translate every TriangleMesh in each ModelVolume.
    /// \param vector Vectorf3 the translation vector
    void translate(const Vectorf3 &vector);

    /// Translate the current ModelObject by translating ModelVolumes with (x,y,z) units.
    /// \param x coordf_t the x units
    /// \param y coordf_t the y units
    /// \param z coordf_t the z units
    void translate(coordf_t x, coordf_t y, coordf_t z);

    /// Scale the current ModelObject by scaling its ModelVolumes.
    /// This function calls scale(const Pointf3 &versor) to scale every TriangleMesh in each ModelVolume.
    /// \param factor float the scaling factor
    void scale(float factor);

    /// Scale the current ModelObject by scaling its ModelVolumes.
    /// \param versor Pointf3 the scaling factor in a 3d vector.
    void scale(const Pointf3 &versor);

    /// Scale the current ModelObject to fit by altering the scaling factor of ModelInstances.
    /// It operates on the total size by duplicating the object according to all the instances.
    /// \param size Sizef3 the size vector
    void scale_to_fit(const Sizef3 &size);

    /// Rotate the current ModelObject by rotating ModelVolumes.
    /// \param angle float the angle in radians
    /// \param axis Axis the axis to be rotated around
    void rotate(float angle, const Axis &axis);

    /// Mirror the current Model around a certain axis.
    /// \param axis Axis enum member
    void mirror(const Axis &axis);

    /// Transform the current ModelObject by a certain ModelInstance attributes.
    /// Inverse transformation is applied to all the ModelInstances, so that the final size/position/rotation of the transformed objects doesn't change.
    /// \param instance ModelInstance the instance used to transform the current ModelObject
    /// \param dont_translate bool whether to translate the current ModelObject or not
    void transform_by_instance(ModelInstance instance, bool dont_translate = false);

    /// Get the number of the unique ModelMaterial objects in this ModelObject.
    /// \return size_t the materials count
    size_t materials_count() const;

    /// Get the number of the facets found in all ModelVolume objects in this ModelObject which are not modifier volumes.
    /// \return size_t the facets count
    size_t facets_count() const;

    /// Know whether there exists a TriangleMesh object that needed repair or not.
    /// \return bool
    bool needed_repair() const;

    /// Cut (Slice) the current ModelObject along a certain axis at a certain coordinate.
    /// \param axis Axis the axis to slice at (X = 0 or Y or Z)
    /// \param z coordf_t the point at the certain axis to cut(slice) the Model at
    /// \param model Model* pointer to the Model which will get the resulting objects added
    void cut(Axis axis, coordf_t z, Model* model) const;

    /// Split the meshes of the ModelVolume in this ModelObject if there exists only one ModelVolume in this ModelObject.
    /// \param new_objects ModelObjectPtrs the generated ModelObjects after the single ModelVolume split
    void split(ModelObjectPtrs* new_objects);

    /// Update the bounding box in this ModelObject
    void update_bounding_box();   // this is a private method but we expose it until we need to expose it via XS

    /// Print the current info of this ModelObject
    void print_info() const;

    private:
    Model* model; ///< Parent object, owning this ModelObject.

    /// Constructor
    /// \param model Model the owner Model.
    ModelObject(Model *model);

    /// Constructor
    /// \param model Model the owner Model.
    /// \param other ModelObject the other ModelObject to be copied
    /// \param copy_volumes bool whether to also copy its volumes or not, by default = true
    ModelObject(Model *model, const ModelObject &other, bool copy_volumes = true);

    /// = Operator overloading
    /// \param other ModelObject the other ModelObject to be copied
    /// \return ModelObject& the current ModelObject to enable operator cascading
    ModelObject& operator= (ModelObject other);

    /// Swap the attributes between another ModelObject
    /// \param other ModelObject the other ModelObject to be swapped with.
    void swap(ModelObject &other);

    /// Destructor
    ~ModelObject();
};

/// An object STL, or a modifier volume, over which a different set of parameters shall be applied.
/// ModelVolume instances are owned by a ModelObject.
class ModelVolume
{
    friend class ModelObject;
    public:

    std::string name;   ///< Name of this ModelVolume object
    TriangleMesh mesh;  ///< The triangular model.
    DynamicPrintConfig config;
    ///< Configuration parameters specific to an object model geometry or a modifier volume,
    ///< overriding the global Slic3r settings and the ModelObject settings.

    bool modifier;  ///< Is it an object to be printed, or a modifier volume?

    /// Get the parent object owning this modifier volume.
    /// \return ModelObject* pointer to the owner ModelObject
    ModelObject* get_object() const { return this->object; };

    /// Get the material id of this ModelVolume object
    /// \return t_model_material_id the material id string
    t_model_material_id material_id() const;

    /// Set the material id to this ModelVolume object
    /// \param material_id t_model_material_id the id of the material
    void material_id(t_model_material_id material_id);

    /// Get the current ModelMaterial in this ModelVolume object
    /// \return ModelMaterial* a pointer to the ModelMaterial
    ModelMaterial* material() const;

    /// Add a new ModelMaterial to this ModelVolume
    /// \param material_id t_model_material_id the id of the material to be added
    /// \param material ModelMaterial the material to be coppied
    void set_material(t_model_material_id material_id, const ModelMaterial &material);

    /// Add a unique ModelMaterial to the current ModelVolume
    /// \return ModelMaterial* pointer to the new ModelMaterial
    ModelMaterial* assign_unique_material();

    private:
    ///< Parent object owning this ModelVolume.
    ModelObject* object;
    ///< The id of the this ModelVolume
    t_model_material_id _material_id;

    /// Constructor
    /// \param object ModelObject* pointer to the owner ModelObject
    /// \param mesh TriangleMesh the mesh of the new ModelVolume object
    ModelVolume(ModelObject *object, const TriangleMesh &mesh);

    /// Constructor
    /// \param object ModelObject* pointer to the owner ModelObject
    /// \param other ModelVolume the ModelVolume object to be copied
    ModelVolume(ModelObject *object, const ModelVolume &other);

    /// = Operator overloading
    /// \param other ModelVolume a volume to be copied in the current ModelVolume object
    /// \return ModelVolume& the current ModelVolume to enable operator cascading
    ModelVolume& operator= (ModelVolume other);

    /// Swap attributes between another ModelVolume object
    /// \param other ModelVolume the other volume object
    void swap(ModelVolume &other);
};

/// A single instance of a ModelObject.
/// Knows the affine transformation of an object.
class ModelInstance
{
    friend class ModelObject;
    public:
    double rotation;            ///< Rotation around the Z axis, in radians around mesh center point.
    double x_rotation;          ///< Rotation around the X axis, in radians around mesh center point. Specific to 3MF format.
    double y_rotation;          ///< Rotation around the Y axis, in radians around mesh center point. Specific to 3MF format.
    double scaling_factor;      ///< uniform scaling factor.
    Pointf3 scaling_vector;     ///< scaling vector. Specific to 3MF format.
    Pointf offset;              ///< offset in unscaled coordinates.
    double z_translation;       ///< translation in z axis. Specific to 3MF format. It's not used anywhere in Slic3r except at writing/reading 3mf.

    /// Get the owning ModelObject
    /// \return ModelObject* pointer to the owner ModelObject
    ModelObject* get_object() const { return this->object; };

    /// Transform an external TriangleMesh object
    /// \param mesh TriangleMesh* pointer to the the mesh
    /// \param dont_translate bool whether to translate the mesh or not
    void transform_mesh(TriangleMesh* mesh, bool dont_translate = false) const;

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
    /// \param other ModelInstance an instance to be copied in the current ModelInstance object
    /// \return ModelInstance& the current ModelInstance to enable operator cascading
    ModelInstance& operator= (ModelInstance other);

    /// Swap attributes between another ModelInstance object
    /// \param other ModelInstance& the other instance object
    void swap(ModelInstance &other);
};

}

#endif
