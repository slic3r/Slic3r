#include "../IO.hpp"
#include <string>
#include <cstring>
#include <map>
// Include miniz.h , if you want to include it in other file use #define MINIZ_HEADER_FILE_ONLY before including it.
#include <zip/zip.h>
//#include "../../zip/zip.h"

namespace Slic3r { namespace IO {

bool
TMF::write(Model& model, std::string output_file){
    // Create a new zip archive object.
    struct zip_t* zip_archive = zip_open(output_file.c_str(),ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');

    // Check whether it's created or not
    if(!zip_archive) return false;

    // Create a 3dmodel.model entry in /3D/ containing the buffer.
    if(!zip_entry_open(zip_archive, "3D/3dmodel.model"))
        return false;

    // Create a buffer to carry data to pass to the zip entry.
    // When the buffer reaches a certain length write to the file then make it empty.
    std::string buff = "";

    // add the XML document header.
    buff += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";

    // Write the model element.
    buff += "<model unit=\"millimeter\" xml:lang=\"en-US\"";
    buff += " xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\"> \n";

    // Write the model metadata.
    for(std::map<std::string, std::string>::iterator it = model.metadata.begin(); it != model.metadata.end(); ++it){
        buff += ("<metadata name=\"" + it->first + "\">" + it->second + "</metadata>\n" );
    }

    // Close the model element.
    buff += "</model>\n";

    // Debugging code
    //std::cout << buff ;

    // Write the model material


    // append the current opened entry with the current buffer.
    zip_entry_write(zip_archive, buff.c_str(), buff.size());

    // Close the 3dmodel.model file in /3D/ directory
    if(!zip_entry_close(zip_archive))
        return false;

    // Finalize the archive and end writing.
    zip_close(zip_archive);

    return true;
}

// To be populated later
bool
TMF::read(std::string input_file, Model* model){
    return false;
}

} }