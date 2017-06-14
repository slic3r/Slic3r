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
    std::string buff; ///< The buffer currently used in write functions.
    ///< When it reaches a max capacity it's written to the current entry in the zip file.

    /// Constructor
    TMFEditor(std::string input_file, Model* model){
        zip_name = input_file;
        this->model = model;
        buff = "";
    }

    /// Write the metadata of the model. This function is called by writeModel() function.
    bool write_metadata(){
        // Write the model metadata.
        for(std::map<std::string, std::string>::iterator it = model->metadata.begin(); it != model->metadata.end(); ++it){
            append_buffer("<metadata name=\"" + it->first + "\">" + it->second + "</metadata>\n" );
        }
        return true;
    }

    /// Write object of the current model. This function is called by writeModel() function.
    /// \param index int the index of the object to be read
    /// \return bool 1: write operation is successful , otherwise not.
    bool write_object(int index){
        ModelObject* object = model->objects[index];

        // Create the new object element.
        append_buffer("<object id=\"" + to_string(index + 1) + "\" type=\"model\"");

        // Add part number if found.
        if(object->part_number != -1)
            append_buffer(" partnumber=\"" + to_string(object->part_number) + "\"");

        append_buffer(">");

        // Create mesh element which contains the vertices and the volumes.
        append_buffer("<mesh>\n");

        // Create vertices element.
        append_buffer("<vertices>\n");

        // Save the start offset of each volume vertices in the object.
        std::vector<size_t> vertices_offsets;
        size_t num_vertices = 0;

        for(ModelVolume* volume : object->volumes){
            // Require mesh vertices.
            volume->mesh.require_shared_vertices();

            vertices_offsets.push_back(num_vertices);
            const auto &stl = volume->mesh.stl;
            for (int i = 0; i < stl.stats.shared_vertices; ++i)
            {
                // Subtract origin_translation in order to restore the coordinates of the parts
                // before they were imported. Otherwise, when this 3MF file is reimported parts
                // will be placed in the platter correctly, but we will have lost origin_translation
                // thus any additional part added will not align with the others.
                // In order to do this we compensate for this translation in the instance placement
                // below.
                append_buffer("<vertex");
                append_buffer(" x=\"" + to_string(stl.v_shared[i].x - object->origin_translation.x) + "\"");
                append_buffer(" y=\"" + to_string(stl.v_shared[i].y - object->origin_translation.y) + "\"");
                append_buffer(" z=\"" + to_string(stl.v_shared[i].z - object->origin_translation.z) + "\"/>\n");
            }
            num_vertices += stl.stats.shared_vertices;
        }

        // Close the vertices element.
        append_buffer("</vertices>\n");

        // Append volumes in triangles element.
        append_buffer("<triangles>\n");

        for (size_t i_volume = 0; i_volume < object->volumes.size(); ++i_volume) {
            ModelVolume *volume = object->volumes[i_volume];
            int vertices_offset = vertices_offsets[i_volume];

            // Add the volume triangles to the triangles list.
            for (int i = 0; i < volume->mesh.stl.stats.number_of_facets; ++i){
                append_buffer("<triangle");
                for(int j = 0; j < 3; j++){
                    append_buffer(" v" + to_string(j+1) + "=\"" + to_string(volume->mesh.stl.v_indices[i].vertex[j] + vertices_offset) + "\"");
                }
                if(!volume->material_id().empty())
                    append_buffer(" pid=\"1\" p1=\"" + to_string(volume->material_id()) + "\""); // Base Materials id = 1 and p1 is assigned to the whole triangle.
                append_buffer("/>");
            }
        }

        // Close the triangles element
        append_buffer("</triangles>\n");

        // Close the mesh element.
        append_buffer("</mesh>\n");

        // Close the object element.
        append_buffer("</object>\n");

        return true;
    }

    /// Write the build element.
    bool write_build(){
        // Create build element.
        append_buffer("<build> \n");
//
//        // Write ModelInstances for each ModelObject.
        for(size_t object_id; object_id < model->objects.size(); ++object_id){
            ModelObject* object = model->objects[object_id];

            for (const ModelInstance* instance : object->instances){
//                append_buffer("<item" + "objectid=\"" + to_string(object_id + 1) + "\"");
//                // Add the transform
//                //<item objectid="4" transform="5.96045e-008 0.999997 0 -0.999997 -9.40395e-007 0 0 0 1 126.999 -126.998 0.00197"/>
            }
//                instances
//                    << "    <instance objectid=\"" << object_id << "\">" << endl
//                    << "      <deltax>" << instance->offset.x + object->origin_translation.x << "</deltax>" << endl
//                    << "      <deltay>" << instance->offset.y + object->origin_translation.y << "</deltay>" << endl
//                    << "      <rz>" << instance->rotation << "</rz>" << endl
//                    << "      <scale>" << instance->scaling_factor << "</scale>" << endl
//                    << "    </instance>" << endl;
        }
//
        append_buffer("</build> \n");
        return true;
    }

    /// Write Materials. The current supported materials are only of the core specifications.
    // The 3MF core specs support base materials, Each has by default color and name attributes.
    bool write_materials(){
        if(model->materials.size() == 0)
            return true;

        bool baseMaterialsWritten = false;

        // Write the base materials.
        for(const auto &material : model->materials){
            // If id is empty or if "name" attribute is not found in this material attributes ignore.
            if(material.first.empty() || material.second->attributes.count("name") == 0)
                continue;
            // Add the base materials element if not added.
            if(!baseMaterialsWritten){
                append_buffer("<basematerials id=\"1\">\n");
                baseMaterialsWritten = true;
            }
            // We add it with black color by default.
            append_buffer("<base name=\"" + material.second->attributes["name"] + "\" ");

            // If "displaycolor" attribute is not found, add a default black colour. Color is a must in base material in 3MF.
            append_buffer("displaycolor=\"" + (material.second->attributes.count("displaycolor") > 0 ? material.second->attributes["displaycolor"] : "#000000FF") + "\"/>\n");
            // ToDo @Samir55 to be covered in AMF write.
        }

        // Close base materials if it's written.
        if(baseMaterialsWritten)
            append_buffer("</basematerials>\n");
        return true;
    }

    /// Write the Model in a zip file. This function is called by produceTMF() function.
    bool write_model(){
        // Create a 3dmodel.model entry in /3D/ containing the buffer.
        if(zip_entry_open(zip_archive, "3D/3dmodel.model"))
            return false;

        // add the XML document header.
        append_buffer("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");

        // Write the model element.
        append_buffer("<model unit=\"millimeter\" xml:lang=\"en-US\"");
        append_buffer(" xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\"> \n");

        // Write metadata.
        write_metadata();

        // Write resources.
        append_buffer("<resources> \n");

        // Write Model Material.
        write_materials();

        // Write Object
        for(size_t object_index = 0; object_index < model->objects.size(); object_index++)
            write_object(object_index);

        // Close resources
        append_buffer("</resources> \n");

        // Write build element.
        write_build();

        // Close the model element.
        append_buffer("</model>\n");

        // Write what is found in the buffer.
        write_buffer();

        // Close the 3dmodel.model file in /3D/ directory
        if(zip_entry_close(zip_archive))
            return false;

        return true;
    }

    /// Write the necessary relationships in the 3MF package. This function is called by produceTMF() function.
    bool write_TMF_types(){
        // Create a new zip entry "[Content_Types].xml" at zip directory /.
        if(zip_entry_open(zip_archive, "[Content_Types].xml"))
            return false;

        // Write 3MF Types "3MF OPC relationships".
        append_buffer("<Types>\n");
        append_buffer("<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>\n");
        append_buffer("<Default Extension=\"model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>\n");
        append_buffer("</Types>\n");
        write_buffer();

        // Close [Content_Types].xml zip entry.
        zip_entry_close(zip_archive);

        // Create "_rels" folder in the zip archive (to create a folder instead of file simply put / at the end of the name.
        if(zip_entry_open(zip_archive, "_rels/"))
            return false;

        zip_entry_close(zip_archive);
        return true;
    }

    /// Write TMF function called by TMF::write() function
    bool produce_TMF(){
        // ToDo @Samir55 Throw c++ exceptions instead of returning false (Ask about this).
        // Create a new zip archive object.
        zip_archive = zip_open(zip_name.c_str(), ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');

        // Check whether it's created or not.
        if(!zip_archive) return false;

        // Prepare the 3MF Zip archive by writing the relationships.
        if(!write_TMF_types())
            return false;

        // Write the model.
        if(!write_model()) return false;

        // Finalize the archive and end writing.
        zip_close(zip_archive);
        return true;
    }

    /// Read TMF function called by TMF::read() function
    bool consume_TMF(){
        return true;
    }

    // Helper Functions.
    /// Append the buffer with a string to be written. This function calls write_buffer() if the buffer reached its capacity.
    void append_buffer(std::string s){
        buff += s;
        if(buff.size() + s.size() > WRITE_BUFFER_MAX_CAPACITY)
            write_buffer();
    }

    /// This function writes the buffer to the current opened zip entry file if it exceeds a certain capacity.
    void write_buffer(){
        // Append the current opened entry with the current buffer.
        zip_entry_write(zip_archive, buff.c_str(), buff.size());
        // Clear the buffer.
        buff = "";
    }

    template <class T>
    std::string to_string(T number){
        std::ostringstream s;
        s << number;
        return s.str();
    }

//    string apply
};

bool
TMF::write(Model& model, std::string output_file){
    TMFEditor tmf_writer(output_file, &model);
    return tmf_writer.produce_TMF();
}

bool
TMF::read(std::string input_file, Model* model){
    TMFEditor tmf_reader(input_file, model);
    return tmf_reader.consume_TMF();
}

} }