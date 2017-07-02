#include "TMF.hpp"

namespace Slic3r { namespace IO {

TMFEditor::TMFEditor(std::string input_file, Model *model) {
    zip_name = input_file;
    this->model = model;
    buff = "";
}

bool
TMFEditor::write_types()
{
    // Create a new zip entry "[Content_Types].xml" at zip directory /.
    if(zip_entry_open(zip_archive, "[Content_Types].xml"))
        return false;

    // Write 3MF Types "3MF OPC relationships".
    append_buffer("<?xml version=\"1.0\" encoding=\"UTF-8\"?> \n");
    append_buffer("<Types xmlns=\"" + namespaces.at("content_types") + "\">\n");
    append_buffer("<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>\n");
    append_buffer("<Default Extension=\"model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>\n");
    append_buffer("</Types>\n");
    write_buffer();

    // Close [Content_Types].xml zip entry.
    zip_entry_close(zip_archive);

    return true;
}

bool
TMFEditor::write_relationships()
{
    // Create .rels in "_rels" folder in the zip archive.
    if(zip_entry_open(zip_archive, "_rels/.rels"))
        return false;

    // Write the primary 3dmodel relationship.
    append_buffer("<?xml version=\"1.0\" encoding=\"UTF-8\"?> \n"
                          "<Relationships xmlns=\"" + namespaces.at("relationships") +
                  "\">\n<Relationship Id=\"rel0\" Target=\"/3D/3dmodel.model\" Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\" /></Relationships>\n");
    write_buffer();

    // Close _rels.rels
    zip_entry_close(zip_archive);
    return true;
}

bool
TMFEditor::write_model()
{
    // Create a 3dmodel.model entry in /3D/ containing the buffer.
    if(zip_entry_open(zip_archive, "3D/3dmodel.model"))
        return false;

    // add the XML document header.
    append_buffer("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");

    // Write the model element. Append any necessary namespaces.
    append_buffer("<model unit=\"millimeter\" xml:lang=\"en-US\"");
    append_buffer(" xmlns=\"" + namespaces.at("3mf") + "\"");
    append_buffer(" xmlns:m=\"" + namespaces.at("m") + "\"");
    append_buffer(" xmlns:slic3r=\"" + namespaces.at("slic3r") + "\"> \n");

    // Write metadata.
    write_metadata();

    // Write resources.
    append_buffer("    <resources> \n");

    // Write Model Material.
    write_materials();

    // Write Object
    for(size_t object_index = 0; object_index < model->objects.size(); object_index++)
        write_object(object_index);

    // Close resources
    append_buffer("    </resources> \n");

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

bool
TMFEditor::write_metadata()
{
    // Write the model metadata.
    for(std::map<std::string, std::string>::iterator it = model->metadata.begin(); it != model->metadata.end(); ++it){
        append_buffer("    <metadata name=\"" + it->first + "\">" + it->second + "</metadata>\n" );
    }

    // Write Slic3r metadata carrying the version number.
    append_buffer("    <slic3r:metadata type=\"version\">" + to_string(SLIC3R_VERSION) + "</slic3r:metadata>\n");

    return true;
}

bool
TMF::write(Model& model, std::string output_file)
{
    TMFEditor tmf_writer(output_file, &model);
    return tmf_writer.produce_TMF();
}

bool
TMFEditor::write_materials()
{
    if (model->materials.size() == 0)
        return true;

    bool base_materials_written = false;

    // Write the base materials.
    for (const auto &material : model->materials){
        // If id is empty or if "name" attribute is not found in this material attributes ignore.
        if (material.first.empty() || material.second->attributes.count("name") == 0)
            continue;
        // Add the base materials element if not added.
        if (!base_materials_written){
            append_buffer("    <basematerials id=\"1\">\n");
            base_materials_written = true;
        }
        // We add it with black color by default.
        append_buffer("        <base name=\"" + material.second->attributes["name"] + "\" ");

        // If "displaycolor" attribute is not found, add a default black colour. Color is a must in base material in 3MF.
        append_buffer("displaycolor=\"" + (material.second->attributes.count("displaycolor") > 0 ? material.second->attributes["displaycolor"] : "#000000FF") + "\"/>\n");
        // ToDo @Samir55 to be covered in AMF write and ask about default color.
    }

    // Close base materials if it's written.
    if (base_materials_written)
        append_buffer("    </basematerials>\n");

    // Write Slic3r custom config data group.
    // It has the following structure:
    // 1. All Slic3r metadata configs are stored in <Slic3r:materials> element which contains
    // <Slic3r:material> element having the following attributes:
    // material id "mid" it points to, type and then the serialized key.

    // Write Sil3r materials custom configs if base materials are written above.
    if (base_materials_written) {
        // Add Slic3r material config group.
        append_buffer("    <slic3r:materials>\n");

        // Keep an index to keep track of which material it points to.
        int material_index = 0;

        for (const auto &material : model->materials) {
            // If id is empty or if "name" attribute is not found in this material attributes ignore.
            if (material.first.empty() || material.second->attributes.count("name") == 0)
                continue;
            for (const std::string &key : material.second->config.keys()) {
                append_buffer("        <slic3r:material mid=\"" + to_string(material_index)
                              + "\" type=\"" + key + "\">"
                              + material.second->config.serialize(key) + "</slic3r:material>\n"
                );
            }
        }

        // close Slic3r material config group.
        append_buffer("    </slic3r:materials>\n");
    }

    // Write 3MF material groups found in 3MF extension.
    for(const auto &material_group : model->material_groups){

        // Get the current material type from using the material group id.
        int type = model->material_groups_types[material_group.first];

        // Write this material group according to its type.
        switch (type){
            case COLOR:
                append_buffer("    <m:colorgroup id=\"" + to_string(material_group.first) + "\">\n");
                for (const auto &color_material : material_group.second) {
                    append_buffer("        <m:color color=\"" + color_material.second->attributes["color"] + "\" />\n");
                }
                append_buffer("    </m:colorgroup>\n");
                break;
            case COMPOSITE_MATERIAL:
                break;
            default:
                break;
        }

    }

    return true;

}

bool
TMFEditor::write_object(int index)
{
    ModelObject* object = model->objects[index];

    // Create the new object element.
    append_buffer("        <object id=\"" + to_string(index + 1) + "\" type=\"model\"");

    // Add part number if found.
    if (object->part_number != -1)
        append_buffer(" partnumber=\"" + to_string(object->part_number) + "\"");

    append_buffer(">\n");

    // Write Slic3r custom configs.
    for (const std::string &key : object->config.keys()){
        append_buffer("        <slic3r:object type=\"slic3r." + key + "\">"
                      + object->config.serialize(key) + "</slic3r:object>\n");
    }

    // Create mesh element which contains the vertices and the volumes.
    append_buffer("            <mesh>\n");

    // Create vertices element.
    append_buffer("                <vertices>\n");

    // Save the start offset of each volume vertices in the object.
    std::vector<size_t> vertices_offsets;
    size_t num_vertices = 0;

    for (ModelVolume* volume : object->volumes){
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
            append_buffer("                    <vertex");
            append_buffer(" x=\"" + to_string(stl.v_shared[i].x - object->origin_translation.x) + "\"");
            append_buffer(" y=\"" + to_string(stl.v_shared[i].y - object->origin_translation.y) + "\"");
            append_buffer(" z=\"" + to_string(stl.v_shared[i].z - object->origin_translation.z) + "\"/>\n");
        }
        num_vertices += stl.stats.shared_vertices;
    }

    // Close the vertices element.
    append_buffer("                </vertices>\n");

    // Append volumes in triangles element.
    append_buffer("                <triangles>\n");

    // Save the start offset (triangle offset) of each volume (To be saved for writing Slic3r custom configs).
    std::vector<size_t> triangles_offsets;
    size_t num_triangles = 0;

    for (size_t i_volume = 0; i_volume < object->volumes.size(); ++i_volume) {
        ModelVolume *volume = object->volumes[i_volume];
        int vertices_offset = vertices_offsets[i_volume];
        triangles_offsets.push_back(num_triangles);

        // Add the volume triangles to the triangles list.
        for (int i = 0; i < volume->mesh.stl.stats.number_of_facets; ++i){
            append_buffer("                    <triangle");
            for (int j = 0; j < 3; j++){
                append_buffer(" v" + to_string(j+1) + "=\"" + to_string(volume->mesh.stl.v_indices[i].vertex[j] + vertices_offset) + "\"");
            }
            if (!volume->material_id().empty())
                append_buffer(" pid=\"1\" p1=\"" + to_string(volume->material_id()) + "\""); // Base Materials id = 1 and p1 is assigned to the whole triangle.
            append_buffer("/>\n");
            num_triangles++;
        }
    }

    // Close the triangles element
    append_buffer("                </triangles>\n");

    // Add Slic3r volumes group.
    append_buffer("                <slic3r:volumes>\n");

    // Add each volume as <slic3r:volume> element containing Slic3r custom configs.
    // Each volume has the following attributes:
    // ts : "start triangle index", te : "end triangle index".

    for (size_t i_volume = 0; i_volume < object->volumes.size(); ++i_volume) {
        ModelVolume *volume = object->volumes[i_volume];

        append_buffer("                    <slic3r:volume ts=\"" + to_string(triangles_offsets[i_volume]) + "\""
                      + " te=\"" + ((i_volume < object->volumes.size() - 1) ? to_string(triangles_offsets[i_volume+1] - 1) : to_string(num_triangles-1)) + "\""
                      + (volume->modifier ? " modifier=\"1\" " : " modifier=\"0\" ")
                      + ">\n");

        for (const std::string &key : volume->config.keys()){
            append_buffer("                        <slic3r:metadata type=\"slic3r." +  key
                          + "\">" + volume->config.serialize(key)
                          + "</slic3r:metadata>\n");
        }

        // Close Slic3r volume
        append_buffer("                    </slic3r:volume>\n");
    }

    // Close Slic3r volumes group.
    append_buffer("                </slic3r:volumes>\n");

    // Close the mesh element.
    append_buffer("            </mesh>\n");

    // Close the object element.
    append_buffer("        </object>\n");

    return true;
}

bool
TMFEditor::write_build()
{
    // Create build element.
    append_buffer("    <build> \n");

    // Write ModelInstances for each ModelObject.
    for(size_t object_id = 0; object_id < model->objects.size(); ++object_id){
        ModelObject* object = model->objects[object_id];
        for (const ModelInstance* instance : object->instances){
            append_buffer("        <item objectid=\"" + to_string(object_id + 1) + "\"");

            // Get the rotation about z and the scale factor.
            double sc = instance->scaling_factor, cosine_rz = cos(instance->rotation) , sine_rz = sin(instance->rotation);
            double tx = instance->offset.x + object->origin_translation.x , ty = instance->offset.y + object->origin_translation.y;

            std::string transform = to_string(cosine_rz * sc) + " " + to_string(sine_rz * sc) + " 0 "
                                    + to_string(-1 * sine_rz * sc) + " " + to_string(cosine_rz*sc) + " 0 "
                                    + "0 " + "0 " + to_string(sc) + " "
                                    + to_string(tx) + " " + to_string(ty) + " " + "0";

            // Add the transform
            append_buffer(" transform=\"" + transform + "\"/>\n");

        }
    }
    append_buffer("    </build> \n");
    return true;
}

bool
TMFEditor::produce_TMF()
{
    // Create a new zip archive object.
    zip_archive = zip_open(zip_name.c_str(), ZIP_DEFLATE_COMPRESSION, 'w');

    // Check whether it's created or not.
    if(!zip_archive) return false;

    // Prepare the 3MF Zip archive by writing the relationships.
    if(!write_relationships())
        return false;

    // Prepare the 3MF Zip archive by writing the types.
    if(!write_types())
        return false;

    // Write the model.
    if(!write_model()) return false;

    // Finalize the archive and end writing.
    zip_close(zip_archive);
    return true;
}

void
TMFEditor::append_buffer(std::string s)
{
    buff += s;
    if(buff.size() + s.size() > WRITE_BUFFER_MAX_CAPACITY)
        write_buffer();
}

TMFEditor::~TMFEditor()
{

}

void
TMFEditor::write_buffer()
{
    // Append the current opened entry with the current buffer.
    zip_entry_write(zip_archive, buff.c_str(), buff.size());
    // Clear the buffer.
    buff = "";
}

bool
TMF::read(std::string input_file, Model* model)
{
    TMFEditor tmf_reader(input_file, model);
    return tmf_reader.consume_TMF();
}

void XMLCALL
TMFParserContext::startElement(void *userData, const char *name, const char **atts){
    TMFParserContext *ctx = (TMFParserContext*) userData;
    ctx->startElement(name, atts);
}

void XMLCALL
TMFParserContext::endElement(void *userData, const char *name)
{
    TMFParserContext *ctx = (TMFParserContext*)userData;
    ctx->endElement(name);
}

void XMLCALL
TMFParserContext::characters(void *userData, const XML_Char *s, int len)
{
    TMFParserContext *ctx = (TMFParserContext*)userData;
    ctx->characters(s, len);
}

void
TMFParserContext::startElement(const char *name, const char **atts) {

}

void
TMFParserContext::endElement(const char *name) {

}

void
TMFParserContext::characters(const XML_Char *s, int len) {

}

void
TMFParserContext::endDocument() {

}

} }