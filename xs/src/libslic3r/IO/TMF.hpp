#ifndef SLIC3R_TMF_H
#define SLIC3R_TMF_H

#include "../IO.hpp"
#include <stdio.h>
#include <string>
#include <cstring>
#include <map>
#include <vector>
#include <algorithm>
#include <math.h>
#include <zip/zip.h>
#include <boost/move/move.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/nowide/iostream.hpp>
#include <expat/expat.h>
#include "../../expat/expat.h" // included only For IDE code suggestions.
#include "../../zip/zip.h" // included only For IDE code suggestions.

#define WRITE_BUFFER_MAX_CAPACITY 10000
#define ZIP_DEFLATE_COMPRESSION 8
#define PI 3.141592653589793238

namespace Slic3r { namespace IO {

/// 3MF Editor class responsible for reading and writing 3mf files.
class TMFEditor
{
public:
    const std::map<std::string, std::string> namespaces = {
            {"3mf", "http://schemas.microsoft.com/3dmanufacturing/core/2015/02"}, // Default XML namespace.
            {"slic3r", "http://link_to_Slic3r_schema.com/2017/06"}, // Slic3r namespace.
            {"m", "http://schemas.microsoft.com/3dmanufacturing/material/2015/02"}, // Material Extension namespace.
            {"s", "http://schemas.microsoft.com/3dmanufacturing/slice/2015/07"}, // Slice Extension.
            {"content_types", "http://schemas.openxmlformats.org/package/2006/content-types"}, // Content_Types namespace.
            {"relationships", "http://schemas.openxmlformats.org/package/2006/relationships"} // Relationships namespace.
    };
    ///< Namespaces in the 3MF document.

    enum material_groups_types{
        UNKNOWEN,
        BASE_MATERIAL,
        COLOR,
        TEXTURE,
        COMPOSITE_MATERIAL,
        MULTI_PROPERTIES
    };
    ///< 3MF material groups in 3MF core and in 3MF materials extension.

    TMFEditor(std::string input_file, Model* model);

    /// Write TMF function called by TMF::write() function.
    bool produce_TMF();

    /// Read TMF function called by TMF::read() function.
    bool consume_TMF();

    ~TMFEditor();

private:
    std::string zip_name; ///< The zip archive file name.
    Model* model; ///< The model to be read or written.
    zip_t* zip_archive; ///< The zip archive object for reading/writing zip files.
    std::string buff; ///< The buffer currently used in write functions.
    ///< When it reaches a max capacity it's written to the current entry in the zip file.

    /// Write the necessary types in the 3MF package. This function is called by produceTMF() function.
    bool write_types();

    /// Write the necessary relationships in the 3MF package. This function is called by produceTMF() function.
    bool write_relationships();

    /// Write the Model in a zip file. This function is called by produceTMF() function.
    bool write_model();

    /// Write the metadata of the model. This function is called by writeModel() function.
    bool write_metadata();

    /// Write Materials. The current supported materials are only of the core specifications.
    // The 3MF core specs support base materials, Each has by default color and name attributes.
    bool write_materials();

    /// Write object of the current model. This function is called by writeModel() function.
    /// \param index int the index of the object to be read
    /// \return bool 1: write operation is successful , otherwise not.
    bool write_object(int index);

    /// Write the build element.
    bool write_build();

    /// Read the Model.
    bool read_model();

    // Helper Functions.
    /// Append the buffer with a string to be written. This function calls write_buffer() if the buffer reached its capacity.
    void append_buffer(std::string s);

    /// This function writes the buffer to the current opened zip entry file if it exceeds a certain capacity.
    void write_buffer();

    template <class T>
    std::string to_string(T number){
        std::ostringstream s;
        s << number;
        return s.str();
    }

};

/// 3MF XML Document parser.
struct TMFParserContext{

    enum TMFNodeType {
        NODE_TYPE_UNKNOWN,
        NODE_TYPE_MODEL,
        NODE_TYPE_METADATA,
        NODE_TYPE_RESOURCES,
        NODE_TYPE_BASE_MATERIALS,
        NODE_TYPE_BASE,
        NODE_TYPE_COLOR_GROUP,
        NODE_TYPE_COLOR,
        NODE_TYPE_COMPOSITE_MATERIALS,
        NODE_TYPE_MULTI_PROPERTIES,
        NODE_TYPE_OBJECT,
        NODE_TYPE_MESH,
        NODE_TYPE_VERTICES,
        NODE_TYPE_VERTEX,
        NODE_TYPE_TRIANGLES,
        NODE_TYPE_TRIANGLE,
        NODE_TYPE_COMPONENTS,
        NODE_TYPE_COMPONENT,
        NODE_TYPE_BUILD,
        NODE_TYPE_ITEM,
        NODE_TYPE_SLIC3R_METADATA,
        NODE_TYPE_SLIC3R_VOLUMES,
        NODE_TYPE_SLIC3R_VOLUME,
        NODE_TYPE_SLIC3R_OBJECT_CONFIG,
        NODE_TYPE_SLIC3R_VOLUME_CONFIG,
    };
    ///< Nodes found in 3MF XML document.

    XML_Parser m_parser;
    ///< Current Expat XML parser instance.

    std::vector<TMFNodeType> m_path;
    ///< Current parsing path in the XML file.

    Model &m_model;
    ///< Model to receive objects extracted from an 3MF file.

    int material_group_id;
    ///< Current read material group id.

    int material_group_type;
    ///< Current read group type.

    std::vector<int> material_groups_offset;
    ///< The material group start offset in model materials map;

    std::vector<bool> used_material_groups;
    ///< Keep track of what material groups are used in Slic3r. 0: means not used.

    ModelObject *m_object;
    ///< Current object allocated for an model/object XML subtree.

    std::map<std::string, int> m_objects_indices;
    ///< Mapping the object id in the document to the index in the model objects vector.

    std::vector<bool> m_output_objects;
    ///< a vector determines whether each read object should be ignored (1) or not (0).
    ///< Ignored objects are the ones not referenced in build items.

    std::string m_object_material_group_id;
    ///< The object's material group it belongs to.

    std::string m_object_material_id;
    ///< The object's material id in the material group it belongs to.

    std::vector<float> m_object_vertices;
    ///< Vertices parsed for the current m_object.
    // ToDo Ask: Is this correct to add it all the triangles into a single volume.

    ModelVolume *m_volume;
    ///< Volume allocated for an model/object/mesh.

    std::vector<int> m_volume_facets;
    ///< Faces collected for all volumes of the current object.

    ModelMaterial *m_material;
    ///< Current base material allocated for the current model.

    std::string m_value[3];
    ///< Generic string buffer for metadata, etc.

    static void XMLCALL startElement(void *userData, const char *name, const char **atts);
    static void XMLCALL endElement(void *userData, const char *name);
    static void XMLCALL characters(void *userData, const XML_Char *s, int len); /* s is not 0 terminated. */
    static const char* get_attribute(const char **atts, const char *id);

    TMFParserContext(XML_Parser parser, Model *model);
    void startElement(const char *name, const char **atts);
    void endElement(const char *name);
    void endDocument();
    void characters(const XML_Char *s, int len);
    void stop();

    ///
    /// \param group_type
    /// \return
    bool read_material_group(const char** atts, TMFEditor::material_groups_types group_type);

    ///
    /// \param atts
    /// \return
    bool read_material(const char** atts);

    /// Get scale, rotation and scale transformation from affine matrix.
    /// \param matrix string the 3D matrix where elements are separated by space.
    /// \return vector<double> a vector contains [translation, scale factor, xRotation, yRotation, xRotation].
    bool get_transformations(std::string matrix, std::vector<double>& transformations);

    /// Add a new volume to the current object.
    /// \param start_offset size_t the start index in the m_volume_facets vector.
    /// \param end_offset size_t the end index in the m_volume_facets vector.
    /// \param modifier bool whether the volume is modifier or not.
    /// \return ModelVolume* a pointer to the newly added volume.
    ModelVolume* add_volume(int start_offset, int end_offset, bool modifier, t_model_material_id material_id = "");


    /// Apply scale, rotate & translate to the given object.
    /// \param object ModelObject*
    /// \param transfornmations vector<int>
    void apply_transformation(ModelObject* object, std::vector<double>& transformations);

    /// Apply scale, rotate & translate to the given instance.
    /// \param instance ModelInstance*
    /// \param transfornmations vector<int>
    void apply_transformation(ModelInstance* instance, std::vector<double>& transformations);
};

} }

#endif //SLIC3R_TMF_H
