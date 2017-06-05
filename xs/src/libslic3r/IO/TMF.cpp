#include "../IO.hpp"
#include <string>
#include <cstring>
#include <map>
#include <vector>
#include <zip/zip.h>
#include "../../zip/zip.h"

#define WRITE_BUFFER_MAX_CAPACITY 10000

namespace Slic3r { namespace IO {

/// 3MF Editor structure responsible for reading and writing 3mf files.
struct TMFEditor
{
    zip_t* zip_archive; ///< The zip archive object for reading/writing zip files.
    std::string zip_name; ///< The zip archive file name.
    Model* model; ///< The model to be read or written.
    string buff; ///< The buffer currently used in write functions.
    ///< When it reaches a max capacity it's written to the current entry in the zip file.

    /// Constructor
    TMFEditor(std::string input_file, Model* model){
        zip_name = input_file;
        this->model = model;
        buff = "";
    }

    /// Write the metadata of the model. This function is called by writeModel() function.
    bool writeMetadata(){
        // Write the model metadata.
        for(std::map<std::string, std::string>::iterator it = model.metadata.begin(); it != model.metadata.end(); ++it){
            appendBuffer("<metadata name=\"" + it->first + "\">" + it->second + "</metadata>\n" );
        }
        return true;
    }

    /// Write object of the current model. This function is called by writeModel() function.
    /// \param index int the index of the object to be read
    /// \return bool 0: write operation is successful , otherwise not.
    bool writeObject(int index){
        return true;
    }

    /// Write the build element.
    bool writeBuild(){
        return true;
    }

    /// Write Materials
    bool writeMaterials(){
        return true;
    }

    /// Write the Model in a zip file. This function is called by produceTMF() function.
    bool writeModel(){
        // Create a 3dmodel.model entry in /3D/ containing the buffer.
        if(zip_entry_open(zip_archive, "3D/3dmodel.model"))
            return false;

        // add the XML document header.
        appendBuffer("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");

        // Write the model element.
        appendBuffer("<model unit=\"millimeter\" xml:lang=\"en-US\"");
        appendBuffer(" xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\"> \n");

        // Write metadata.
        writeMetadata();

        // Write resources.
        appendBuffer("<resources> \n");

        // Write Model Material
        writeMaterials();

        // Write Object
        for(int object_index = 0; object_index < model->objects.size(); object_index++)
            writeObject(object_index);

        // Write build element.
        writeBuild();

        // Close the model element.
        appendBuffer("</model>\n");

        // Close the 3dmodel.model file in /3D/ directory
        if(zip_entry_close(zip_archive))
            return false;

        return true;
    }

    /// Write the necessary relationships in the 3MF package. This function is called by produceTMF() function.
    bool writeTMFRelations(){
        return true;
    }

    /// Write TMF function called by TMF::write() function
    bool produceTMF(){
        // ToDo @Samir55 Throw c++ exceptions instead of returning false (Ask about this).

        // Create a new zip archive object.
        zip_archive = zip_open(zip_name.c_str(), ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');

        // Check whether it's created or not.
        if(!zip_archive) return false;

        // Prepare the 3MF Zip archive by writing the relationships.
        if(!writeTMFRelations())
            return false;

        // Write the model.
        if(!writeModel())
            return false;

        // Finalize the archive and end writing.
        zip_close(zip_archive);
        return true;
    }

    /// Read TMF function called by TMF::read() function
    bool consumeTMF(){
        return true;
    }

    // Helper Functions.
    /// Append the buffer with a string to be written. This function calls writeBuffer() if the buffer reached its capacity.
    void appendBuffer(std::string s){
        buff += s;
        if(buff.size() + s.size > WRITE_BUFFER_MAX_CAPACITY)
            writeBuffer(buff);
    }

    /// This function writes the buffer to the current opened zip entry file if it exceeds a certain capacity.
    void writeBuffer(){
        // Append the current opened entry with the current buffer.
        zip_entry_write(zip_archive, buff.c_str(), buff.size());
        // Clear the buffer.
        buff = "";
    }
};

bool
TMF::write(Model& model, std::string output_file){
    TMFEditor TMF_writer(output_file, model);
    return TMF_writer.produceTMF();
}

bool
TMF::read(std::string input_file, Model* model){
    TMFEditor TMF_reader(input_file, model);
    return TMF_reader.consumeTMF();
}

} }