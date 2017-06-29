#ifndef SLIC3R_TMF_H
#define SLIC3R_TMF_H

#include "../IO.hpp"
#include <string>
#include <cstring>
#include <map>
#include <vector>
#include <math.h>
#include <zip/zip.h>

#define WRITE_BUFFER_MAX_CAPACITY 10000
#define ZIP_DEFLATE_COMPRESSION 8

namespace Slic3r { namespace IO {

/// 3MF Editor class responsible for reading and writing 3mf files.
class TMFEditor
{
public:
    const std::map<std::string, std::string> namespaces = {
            {"3mf", "http://schemas.microsoft.com/3dmanufacturing/core/2015/02"}, // Default XML namespace.
            {"slic3r", "http://link_to_Slic3r_schema.com/2017/06"}, // Slic3r namespace.
            {"m", "http://schemas.microsoft.com/3dmanufacturing/material/2015/02"}, // Material Extension namespace.
            {"content_types", "http://schemas.openxmlformats.org/package/2006/content-types"}, // Content_Types namespace.
            {"relationships", "http://schemas.openxmlformats.org/package/2006/relationships"} // Relationships namespace.
    };
    ///< Namespaces in the 3MF document.
    enum material_groups_types{
        COLOR,
        TEXTURE,
        COMPOSITE_MATERIAL,
        MULTI_PROPERTIES
    };
    ///< 3MF material groups in the materials extension.

    TMFEditor(std::string input_file, Model* model);

    /// Write TMF function called by TMF::write() function.
    bool produce_TMF();

    /// Read TMF function called by TMF::read() function.
    bool consume_TMF(){
        return true;
    }

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


} }

#endif //SLIC3R_TMF_H
