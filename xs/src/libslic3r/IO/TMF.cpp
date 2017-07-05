#include "TMF.hpp"

namespace Slic3r { namespace IO {

TMFEditor::TMFEditor(std::string input_file, Model *model)
{
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

    // Write 3MF Types.
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
//    for(const auto &material_group : model->material_groups){
//
//        // Get the current material type from using the material group id.
//        int type = model->material_groups_types[material_group.first];
//
//        // Write this material group according to its type.
//        switch (type){
//            case COLOR:
//                append_buffer("    <m:colorgroup id=\"" + to_string(material_group.first) + "\">\n");
//                for (const auto &color_material : material_group.second) {
//                    append_buffer("        <m:color color=\"" + color_material.second->attributes["color"] + "\" />\n");
//                }
//                append_buffer("    </m:colorgroup>\n");
//                break;
//            case COMPOSITE_MATERIAL:
//                break;
//            default:
//                break;
//        }
//
//    }

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
TMFEditor::read_model()
{
    // ToDo @Samir55 ask about reading .rels file in order to get the default payload.
    // Extract 3dmodel.model entry.
    zip_entry_open(zip_archive, "3D/3dmodel.model");
    zip_entry_fread(zip_archive, "3dmodel.model");
    zip_entry_close(zip_archive);

    // Read the model XML file.
    XML_Parser parser = XML_ParserCreate(NULL);
    if (! parser) {
        printf("Couldn't allocate memory for parser\n");
        return false;
    }

    boost::nowide::ifstream fin("3dmodel.model", std::ios::in);
    if (!fin.is_open()) {
        boost::nowide::cerr << "Cannot open file: " << "3dmodel.model" << std::endl;
        return false;
    }

    TMFParserContext ctx(parser, model);
    XML_SetUserData(parser, (void*)&ctx);
    XML_SetElementHandler(parser, TMFParserContext::startElement, TMFParserContext::endElement);
    XML_SetCharacterDataHandler(parser, TMFParserContext::characters);

    char buff[8192];
    bool result = false;
    while (!fin.eof()) {
        fin.read(buff, sizeof(buff));
        if (fin.bad()) {
            printf("3MF model parser: Read error\n");
            break;
        }
        if (XML_Parse(parser, buff, fin.gcount(), fin.eof()) == XML_STATUS_ERROR) {
            printf("3MF model parser: Parse error at line %lu:\n%s\n",
                   XML_GetCurrentLineNumber(parser),
                   XML_ErrorString(XML_GetErrorCode(parser)));
            break;
        }
        if (fin.eof()) {
            result = true;
            break;
        }
    }

    XML_ParserFree(parser);
    fin.close();

    // Remove the extracted 3dmodel.model file.
    if (remove("3dmodel.model") != 0)
        return false;

    if (result)
        ctx.endDocument();
    return result;
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

bool
TMFEditor::consume_TMF()
{
    // Open the 3MF package.
    zip_archive = zip_open(zip_name.c_str(), 0, 'r');

    // Check whether it's opened or not.
    if(!zip_archive) return false;

    // Read model.
    if(!read_model())
        return false;

    // Close zip archive.
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

void
TMFEditor::write_buffer()
{
    // Append the current opened entry with the current buffer.
    zip_entry_write(zip_archive, buff.c_str(), buff.size());
    // Clear the buffer.
    buff = "";
}

TMFEditor::~TMFEditor()
{

}

bool
TMF::write(Model& model, std::string output_file)
{
    TMFEditor tmf_writer(output_file, &model);
    return tmf_writer.produce_TMF();
}

bool
TMF::read(std::string input_file, Model* model)
{
    TMFEditor tmf_reader(input_file, model);
    return tmf_reader.consume_TMF();
}

TMFParserContext::TMFParserContext(XML_Parser parser, Model *model):
        m_parser(parser),
        m_model(*model),
        m_object(NULL),
        m_volume(NULL),
        m_material(NULL),
        m_instance(NULL)
{
    m_path.reserve(12); // ToDo @Samir55 to be changed later.
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

const char*
TMFParserContext::get_attribute(const char **atts, const char *id) {
    if (atts == NULL)
        return NULL;
    while (*atts != NULL) {
        if (strcmp(*(atts ++), id) == 0)
            return *atts;
        ++ atts;
    }
    return NULL;
}

void
TMFParserContext::startElement(const char *name, const char **atts)
{
    TMFNodeType node_type_new = NODE_TYPE_UNKNOWN;

    //Debugging statament.
//    std::cout << name << std::endl;

    switch (m_path.size()){
        case 0:
            // Must be <model> tag for 3dmodel.model entry.
            node_type_new = NODE_TYPE_MODEL;
            if (strcmp(name, "model") != 0)
                this->stop();
            break;
        case 1:
            if (strcmp(name, "metadata") == 0){
                // Get the name of the metadata tag.
                const char* name = this->get_attribute(atts, "name");
                if (name != NULL) {
                    m_value[0] = name;
                    node_type_new = NODE_TYPE_METADATA;
                }
            } else if (strcmp(name, "resources") == 0){
                node_type_new = NODE_TYPE_RESOURCES;
            } else if (strcmp(name, "build") == 0){

            }
            break;
        case 2:
            if (strcmp(name, "basematerials") == 0){
                node_type_new = NODE_TYPE_BASE_MATERIALS;
                // Read the current property group id.
                const char* property_group_id = this->get_attribute(atts,"id");
                if (!property_group_id)
                    this->stop();
                // Add a new material_group to the model.
                m_model.add_material_group(TMFEditor::BASE_MATERIAL);
                // Add the index of the current material group in the document and its index in the model.
                material_groups_indices[property_group_id] = m_model.material_groups.size() - 1;
            } else if (strcmp(name, "object") == 0){
                const char* object_id = get_attribute(atts, "id");
                if (!object_id)
                    this->stop();

                // ToDo @Samir55 Ask about why this assert occurs ?
                // Create a new object in the model. This object should be included in another object if
                // it's a component in another object.
                assert(m_object_vertices.empty());
                m_object = m_model.add_object();
                m_objects_indices[object_id] = m_model.objects.size() - 1;

                // Add part number.
                const char* part_number = get_attribute(atts, "partnumber");
                m_object->part_number = (!part_number) ? -1 : atoi(part_number);

                // Add object name.
                const char*  name = get_attribute(atts, "name");
                m_object->name = (!name) ? "" : name;

                node_type_new = NODE_TYPE_OBJECT;
            }
            break;
        case 3:
            if (strcmp(name, "base") == 0){
                node_type_new = NODE_TYPE_BASE;
                // Create a new model material and add it to the current material group.
                m_material = m_model.add_material(m_model.material_groups.size() - 1);
                if(!m_material)
                    this->stop();
                // Add the model material attributes.
                while(*atts != NULL){
                    m_material->attributes[*(atts)] = *(atts + 1);

                    // Debuging statement.
//                    std::cout << *(atts) << " " << *(atts+1) << std::endl;

                    atts += 2;
                }
            } else if (strcmp(name, "mesh") == 0){
                node_type_new = NODE_TYPE_MESH;
            }
            break;
        case 4:
            if (strcmp(name, "vertices") == 0){
                node_type_new = NODE_TYPE_VERTICES;

            } else if (strcmp(name, "triangles") == 0){
                node_type_new = NODE_TYPE_TRIANGLES;
            }
            break;
        case 5:
            if (strcmp(name, "vertex") == 0){
                const char* x = get_attribute(atts, "x");
                const char* y = get_attribute(atts, "y");
                const char* z = get_attribute(atts, "z");
                if ( !x || !y || !z)
                    this->stop();
                m_object_vertices.push_back(atof(x));
                m_object_vertices.push_back(atof(y));
                m_object_vertices.push_back(atof(z));
            } else if (strcmp(name, "triangle") == 0){

            }
            break;
        default:
            break;
    }

    m_path.push_back(node_type_new);
}

void
TMFParserContext::endElement(const char *name)
{
    switch (m_path.back()){
        case NODE_TYPE_METADATA:
            // m_value[0] carries the metadata name and m_value[1] carries the metadata value.
            m_model.metadata[m_value[0]] = m_value[1];
            m_value[1].clear();
            break;
        case NODE_TYPE_BASE:
            break;
        case NODE_TYPE_MESH:
            m_object_vertices.clear();
            break;
        default:
            break;
    }

    m_path.pop_back();
}

void
TMFParserContext::characters(const XML_Char *s, int len)
{
    switch (m_path.back()){
        case NODE_TYPE_METADATA:
            m_value[1].append(s, len);
            break;
        default:
            break;
    }
}

void
TMFParserContext::endDocument()
{

}

void
TMFParserContext::stop()
{

}

} }