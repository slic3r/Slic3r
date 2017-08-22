#include "TMF.hpp"

namespace Slic3r { namespace IO {

bool
TMFEditor::write_types()
{
    // Create a new .[Content_Types].xml file to add to the zip file later.
    boost::nowide::ofstream fout(".[Content_Types].xml", std::ios::out | std::ios::trunc);
    if(!fout.is_open())
        return false;

    // Write 3MF Types.
    fout << "<?xml version=\"1.0\" encoding=\"UTF-8\"?> \n";
    fout << "<Types xmlns=\"" << namespaces.at("content_types") << "\">\n";
    fout << "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>\n";
    fout << "<Default Extension=\"model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>\n";
    fout << "</Types>\n";
    fout.close();

    // Create [Content_Types].xml in the zip archive.
    if(!zip_archive->add_entry("[Content_Types].xml", ".[Content_Types].xml"))
        return false;

    // Remove the created .[Content_Types].xml file.
    if (remove(".[Content_Types].xml") != 0)
        return false;

    return true;
}

bool
TMFEditor::write_relationships()
{
    // Create a new .rels file to add to the zip file later.
    boost::nowide::ofstream fout(".rels", std::ios::out | std::ios::trunc);
    if(!fout.is_open())
        return false;

    // Write the primary 3dmodel relationship.
    fout << "<?xml version=\"1.0\" encoding=\"UTF-8\"?> \n"
                          << "<Relationships xmlns=\"" << namespaces.at("relationships") <<
                  "\">\n<Relationship Id=\"rel0\" Target=\"/3D/3dmodel.model\" Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\" /></Relationships>\n";
    fout.close();

    // Create .rels in "_rels" folder in the zip archive.
    if(!zip_archive->add_entry("_rels/.rels", ".rels"))
        return false;

    // Remove the created .rels file.
    if (remove(".rels") != 0)
        return false;

    return true;
}

bool
TMFEditor::write_model()
{
    // Create a new .3dmodel.model file to add to the zip file later.
    boost::nowide::ofstream fout(".3dmodel.model", std::ios::out | std::ios::trunc);
    if(!fout.is_open())
        return false;

    // Add the XML document header.
    fout << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";

    // Write the model element. Append any necessary namespaces.
    fout << "<model unit=\"millimeter\" xml:lang=\"en-US\"";
    fout << " xmlns=\"" << namespaces.at("3mf") << "\"";
    fout << " xmlns:slic3r=\"" << namespaces.at("slic3r") << "\"> \n";

    // Write metadata.
    write_metadata(fout);

    // Write resources.
    fout << "    <resources> \n";

    // Write Object
    int object_index = 0;
    for(const auto object : model->objects)
        write_object(fout, object, object_index++);

    // Close resources
    fout << "    </resources> \n";

    // Write build element.
    write_build(fout);

    // Close the model element.
    fout << "</model>\n";
    fout.close();

    // Create .3dmodel.model in "3D" folder in the zip archive.
    if(!zip_archive->add_entry("3D/3dmodel.model", ".3dmodel.model"))
        return false;

    // Remove the created .rels file.
    if (remove(".3dmodel.model") != 0)
        return false;

    return true;
}

bool
TMFEditor::write_metadata(boost::nowide::ofstream& fout)
{
    // Write the model metadata.
    for (const auto metadata : model->metadata){
        fout << "    <metadata name=\"" << metadata.first << "\">" << metadata.second << "</metadata>\n";
    }

    // Write Slic3r metadata carrying the version number.
    fout << "    <slic3r:metadata version=\"" << SLIC3R_VERSION << "\"/>\n";

    return true;
}

bool
TMFEditor::write_object(boost::nowide::ofstream& fout, const ModelObject* object, int index)
{
    // Create the new object element.
    fout << "        <object id=\"" << (index + object_id) << "\" type=\"model\"";

    // Add part number if found.
    if (object->part_number != -1)
        fout << " partnumber=\"" << (object->part_number) << "\"";

    fout << ">\n";

    // Write Slic3r custom configs.
    for (const auto &key : object->config.keys()){
        fout << "        <slic3r:object type=\"" << key
                      << "\" config=\"" << object->config.serialize(key) << "\"" << "/>\n";
    }

    // Create mesh element which contains the vertices and the volumes.
    fout << "            <mesh>\n";

    // Create vertices element.
    fout << "                <vertices>\n";

    // Save the start offset of each volume vertices in the object.
    std::vector<int> vertices_offsets;
    int num_vertices = 0;

    for (const auto volume : object->volumes){
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
            fout << "                    <vertex";
            fout << " x=\"" << (stl.v_shared[i].x - object->origin_translation.x) << "\"";
            fout << " y=\"" << (stl.v_shared[i].y - object->origin_translation.y) << "\"";
            fout << " z=\"" << (stl.v_shared[i].z - object->origin_translation.z) << "\"/>\n";
        }
        num_vertices += stl.stats.shared_vertices;
    }

    // Close the vertices element.
    fout << "                </vertices>\n";

    // Append volumes in triangles element.
    fout << "                <triangles>\n";

    // Save the start offset (triangle offset) of each volume (To be saved for writing Slic3r custom configs).
    std::vector<int> triangles_offsets;
    int num_triangles = 0;
    int i_volume = 0;

    for (const auto volume : object->volumes) {
        int vertices_offset = vertices_offsets[i_volume];
        triangles_offsets.push_back(num_triangles);

        // Add the volume triangles to the triangles list.
        for (int i = 0; i < volume->mesh.stl.stats.number_of_facets; ++i){
            fout << "                    <triangle";
            for (int j = 0; j < 3; j++){
                fout << " v" << (j+1) << "=\"" << (volume->mesh.stl.v_indices[i].vertex[j] + vertices_offset) << "\"";
            }
            fout << "/>\n";
            num_triangles++;
        }
        i_volume++;
    }
    triangles_offsets.push_back(num_triangles);

    // Close the triangles element
    fout << "                </triangles>\n";

    // Add Slic3r volumes group.
    fout << "                <slic3r:volumes>\n";

    // Add each volume as <slic3r:volume> element containing Slic3r custom configs.
    // Each volume has the following attributes:
    // ts : "start triangle index", te : "end triangle index".
    i_volume = 0;
    for (const auto volume : object->volumes) {
        fout << "                    <slic3r:volume ts=\"" << (triangles_offsets[i_volume]) << "\""
                      << " te=\"" << (triangles_offsets[i_volume+1] - 1) << "\""
                      << (volume->modifier ? " modifier=\"1\" " : " modifier=\"0\" ")
                      << ">\n";

        for (const std::string &key : volume->config.keys()){
            fout << "                        <slic3r:metadata type=\"" <<  key
                          << "\" config=\"" << volume->config.serialize(key) << "\"/>\n";
        }

        // Close Slic3r volume
        fout << "                    </slic3r:volume>\n";
        i_volume++;
    }

    // Close Slic3r volumes group.
    fout << "                </slic3r:volumes>\n";

    // Close the mesh element.
    fout << "            </mesh>\n";

    // Close the object element.
    fout << "        </object>\n";

    return true;
}

bool
TMFEditor::write_build(boost::nowide::ofstream& fout)
{
    // Create build element.
    fout << "    <build> \n";

    // Write ModelInstances for each ModelObject.
    int object_id = 0;
    for(const auto object : model->objects){
        for (const auto instance : object->instances){
            fout << "        <item objectid=\"" << (object_id + 1) << "\"";

            // Get the rotation about x, y &z, translations and the scale vector.
            double sc = instance->scaling_factor,
                    cosine_rz = cos(instance->rotation),
                    sine_rz = sin(instance->rotation),
                    cosine_ry = cos(instance->y_rotation),
                    sine_ry = sin(instance->y_rotation),
                    cosine_rx = cos(instance->x_rotation),
                    sine_rx = sin(instance->x_rotation),
                    tx = instance->offset.x + object->origin_translation.x ,
                    ty = instance->offset.y + object->origin_translation.y,
                    tz = instance->z_translation;

            // Add the transform
            fout << " transform=\""
                 << (cosine_ry * cosine_rz * sc * instance->scaling_vector.x)
                 << " "
                 << (cosine_ry * sine_rz * sc) << " "
                 << (-1 * sine_ry * sc) << " "
                 << ((sine_rx * sine_ry * cosine_rz -1 * cosine_rx * sine_rz) * sc)  << " "
                 << ((sine_rx * sine_ry * sine_rz + cosine_rx *cosine_rz) * sc * instance->scaling_vector.y) << " "
                 << (sine_rx * cosine_ry * sc) << " "
                 << ((cosine_rx * sine_ry * cosine_rz + sine_rx * sine_rz) * sc) << " "
                 << ((sine_rx * sine_ry * sine_rz - sine_rx * cosine_rz) * sc) << " "
                 << (cosine_rx * cosine_ry * sc * instance->scaling_vector.z) << " "
                 << (tx) << " "
                 << (ty) << " "
                 << (tz)
                 << "\"/>\n";

        }
    object_id++;
    }
    fout << "    </build> \n";
    return true;
}

bool
TMFEditor::read_model()
{
    // Extract 3dmodel.model entry.
    if(!zip_archive->extract_entry("3D/3dmodel.model", "3dmodel.model"))
        return false;

    // Read 3D/3dmodel.model file.
    XML_Parser parser = XML_ParserCreate(NULL);
    if (! parser) {
        std::cout << ("Couldn't allocate memory for parser\n");
        return false;
    }

    boost::nowide::ifstream fin("3dmodel.model", std::ios::in);
    if (!fin.is_open()) {
        boost::nowide::cerr << "Cannot open file: " << "3dmodel.model" << std::endl;
        return false;
    }

    // Create model parser.
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

    // Free the parser and close the file.
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
    zip_archive = new ZipArchive(this->zip_name, 'W');

    // Check it's successfully initialized.
    if(zip_archive->z_stats() == 0) return false;

    // Prepare the 3MF Zip archive by writing the relationships.
    if(!write_relationships())
        return false;

    // Prepare the 3MF Zip archive by writing the types.
    if(!write_types())
        return false;

    // Write the model.
    if(!write_model()) return false;

    // Finalize the archive and end writing.
    zip_archive->finalize();
    return true;
}

bool
TMFEditor::consume_TMF()
{
    // Open the 3MF package.
    zip_archive = new ZipArchive(this->zip_name, 'R');

    // Check it's successfully initialized.
    if(zip_archive->z_stats() == 0) return false;

    // Read model.
    if(!read_model())
        return false;

    // Close zip archive.
    zip_archive->finalize();
    return true;
}

TMFEditor::~TMFEditor(){
    delete zip_archive;
}

bool
TMF::write(Model& model, std::string output_file)
{
    TMFEditor tmf_writer(std::move(output_file), &model);
    return tmf_writer.produce_TMF();
}

bool
TMF::read(std::string input_file, Model* model)
{
    if(!model) return false;
    TMFEditor tmf_reader(std::move(input_file), model);
    return tmf_reader.consume_TMF();
}

TMFParserContext::TMFParserContext(XML_Parser parser, Model *model):
        m_parser(parser),
        m_path(std::vector<TMFNodeType>()),
        m_model(*model),
        m_object(nullptr),
        m_objects_indices(std::map<std::string, int>()),
        m_output_objects(std::vector<bool>()),
        m_object_vertices(std::vector<float>()),
        m_volume(nullptr),
        m_volume_facets(std::vector<int>())
{
    m_path.reserve(9);
    m_value[0] = m_value[1] = m_value[2] = "";
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
    ctx->endElement();
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
    switch (m_path.size()){
        case 0:
            // Must be <model> tag.
            if (strcmp(name, "model") != 0)
                this->stop();
            node_type_new = NODE_TYPE_MODEL;
            break;
        case 1:
            if (strcmp(name, "metadata") == 0) {
                const char* metadata_name = this->get_attribute(atts, "name");
                // Name is required if it's not found stop parsing.
                if (!metadata_name)
                    this->stop();
                m_value[0] = metadata_name;
                node_type_new = NODE_TYPE_METADATA;
            } else if (strcmp(name, "resources") == 0) {
                node_type_new = NODE_TYPE_RESOURCES;
            } else if (strcmp(name, "build") == 0) {
                node_type_new = NODE_TYPE_BUILD;
            }
            break;
        case 2:
            if (strcmp(name, "object") == 0){
                const char* object_id = get_attribute(atts, "id");
                if (!object_id)
                    this->stop();

                if(!m_object_vertices.empty())
                    this->stop();

                // Create a new object in the model. This object should be included in another object if
                // it's a component in another object.
                m_object = m_model.add_object();
                m_objects_indices[object_id] = int(m_model.objects.size()) - 1;
                m_output_objects.push_back(1); // default value 1 means: it's must not be an output.

                // Add part number.
                const char* part_number = get_attribute(atts, "partnumber");
                m_object->part_number = (!part_number) ? -1 : atoi(part_number);

                // Add object name.
                const char*  object_name = get_attribute(atts, "name");
                m_object->name = (!object_name) ? "" : object_name;

                node_type_new = NODE_TYPE_OBJECT;
            } else if (strcmp(name, "item") == 0){
                // Get object id.
                const char* object_id = get_attribute(atts, "objectid");
                if(!object_id)
                    this->stop();
                // Mark object as output.
                m_output_objects[m_objects_indices[object_id]] = 0;

                // Add instance.
                ModelInstance* instance = m_model.objects[m_objects_indices[object_id]]->add_instance();

                // Apply transformation if supplied.
                const char* transformation_matrix = get_attribute(atts, "transform");
                if(transformation_matrix){
                    // Decompose the affine matrix.
                    std::vector<double> transformations;
                    if(!get_transformations(transformation_matrix, transformations))
                        this->stop();

                    if(transformations.size() != 9)
                        this->stop();

                    apply_transformation(instance, transformations);
                }

                node_type_new = NODE_TYPE_ITEM;
            }
            break;
        case 3:
            if (strcmp(name, "mesh") == 0){
                // Create a new model volume.
                if(m_volume)
                    this->stop();
                node_type_new = NODE_TYPE_MESH;
            } else if (strcmp(name, "components") == 0) {
                node_type_new = NODE_TYPE_COMPONENTS;
            } else if (strcmp(name, "slic3r:object") == 0) {
                // Create a config option.
                DynamicPrintConfig *config = nullptr;
                if(m_path.back() == NODE_TYPE_OBJECT && m_object)
                    config = &m_object->config;

                // Get the config key type.
                const char *key = get_attribute(atts, "type");

                if (config && (print_config_def.options.find(key) != print_config_def.options.end())) {
                    // Get the key config string.
                    const char *config_value = get_attribute(atts, "config");

                    config->set_deserialize(key, config_value);
                }
                node_type_new = NODE_TYPE_SLIC3R_OBJECT_CONFIG;
            }
            break;
        case 4:
            if (strcmp(name, "vertices") == 0) {
                node_type_new = NODE_TYPE_VERTICES;
            } else if (strcmp(name, "triangles") == 0) {
                node_type_new = NODE_TYPE_TRIANGLES;
            } else if (strcmp(name, "component") == 0) {
                // Read the object id.
                const char* object_id = get_attribute(atts, "objectid");
                if(!object_id)
                    this->stop();
                ModelObject* component_object = m_model.objects[m_objects_indices[object_id]];
                // Append it to the parent (current m_object) as a mesh since Slic3r doesn't support an object inside another.
                // after applying 3d matrix transformation if found.
                TriangleMesh component_mesh;
                const char* transformation_matrix = get_attribute(atts, "transform");
                if(transformation_matrix){
                    // Decompose the affine matrix.
                    std::vector<double> transformations;
                    if(!get_transformations(transformation_matrix, transformations))
                        this->stop();

                    if( transformations.size() != 9)
                        this->stop();

                    // Create a copy of the current object.
                    ModelObject* object_copy = m_model.add_object(*component_object, true);

                    apply_transformation(object_copy, transformations);

                    // Get the mesh of this instance object.
                    component_mesh = object_copy->raw_mesh();

                    // Delete the copy of the object.
                    m_model.delete_object(m_model.objects.size() - 1);

                } else {
                    component_mesh = component_object->raw_mesh();
                }
                ModelVolume* volume = m_object->add_volume(component_mesh);
                if(!volume)
                    this->stop();
                node_type_new =NODE_TYPE_COMPONENT;
            } else if (strcmp(name, "slic3r:volumes") == 0) {
                node_type_new = NODE_TYPE_SLIC3R_VOLUMES;
            }
            break;
        case 5:
            if (strcmp(name, "vertex") == 0) {
                const char* x = get_attribute(atts, "x");
                const char* y = get_attribute(atts, "y");
                const char* z = get_attribute(atts, "z");
                if ( !x || !y || !z)
                    this->stop();
                m_object_vertices.push_back(atof(x));
                m_object_vertices.push_back(atof(y));
                m_object_vertices.push_back(atof(z));
                node_type_new = NODE_TYPE_VERTEX;
            } else if (strcmp(name, "triangle") == 0) {
                const char* v1 = get_attribute(atts, "v1");
                const char* v2 = get_attribute(atts, "v2");
                const char* v3 = get_attribute(atts, "v3");
                if (!v1 || !v2 || !v3)
                    this->stop();
                // Add it to the volume facets.
                m_volume_facets.push_back(atoi(v1));
                m_volume_facets.push_back(atoi(v2));
                m_volume_facets.push_back(atoi(v3));
                node_type_new = NODE_TYPE_TRIANGLE;
            } else if (strcmp(name, "slic3r:volume") == 0) {
                // Read start offset of the triangles.
                m_value[0] = get_attribute(atts, "ts");
                m_value[1] = get_attribute(atts, "te");
                m_value[2] = get_attribute(atts, "modifier");
                if( m_value[0].empty() || m_value[1].empty() || m_value[2].empty())
                    this->stop();
                // Add a new volume to the current object.
                if(!m_object)
                    this->stop();
                m_volume = add_volume(stoi(m_value[0])*3, stoi(m_value[1]) * 3 + 2, static_cast<bool>(stoi(m_value[2])));
                if(!m_volume)
                    this->stop();
                node_type_new = NODE_TYPE_SLIC3R_VOLUME;
            }
            break;
        case 6:
            if( strcmp(name, "slic3r:metadata") == 0){
                // Create a config option.
                DynamicPrintConfig *config = nullptr;
                if(!m_volume)
                    this->stop();
                config = &m_volume->config;
                const char *key = get_attribute(atts, "type");
                if( config && (print_config_def.options.find(key) != print_config_def.options.end())){
                    const char *config_value = get_attribute(atts, "config");
                    config->set_deserialize(key, config_value);
                }
                node_type_new = NODE_TYPE_SLIC3R_METADATA;
            }
        default:
            break;
    }

    m_path.push_back(node_type_new);
}

void
TMFParserContext::endElement()
{
    switch (m_path.back()){
        case NODE_TYPE_METADATA:
            if( m_path.size() == 2) {
                m_model.metadata[m_value[0]] = m_value[1];
                m_value[1].clear();
            }
            break;
        case NODE_TYPE_MESH:
            // Add the object volume if no there are no added volumes in slic3r:volumes.
            if(m_object->volumes.size() == 0) {
                if(!m_object)
                    this->stop();
                m_volume = add_volume(0, int(m_volume_facets.size()) - 1, 0);
                if (!m_volume)
                    this->stop();
                m_volume = nullptr;
            }
            break;
        case NODE_TYPE_OBJECT:
            if(!m_object)
                this->stop();
            m_object_vertices.clear();
            m_volume_facets.clear();
            m_object = nullptr;
            break;
        case NODE_TYPE_MODEL:
        {
            size_t deleted_objects_count = 0;
            // According to 3MF spec. we must output objects found in item.
            for (size_t i = 0; i < m_output_objects.size(); i++) {
                if (m_output_objects[i]) {
                    m_model.delete_object(i - deleted_objects_count);
                    deleted_objects_count++;
                }
            }
        }
            break;
        case NODE_TYPE_SLIC3R_VOLUME:
            m_volume = nullptr;
            m_value[0].clear();
            m_value[1].clear();
            m_value[2].clear();
            break;
        default:
            break;
    }

    m_path.pop_back();
}

void
TMFParserContext::characters(const XML_Char *s, int len)
{
    switch (m_path.back()) {
        case NODE_TYPE_METADATA:
            if(m_path.size() == 2)
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
    XML_StopParser(m_parser, 0);
}

bool
TMFParserContext::get_transformations(std::string matrix, std::vector<double> &transformations)
{
    // Get the values.
    double m[12];
    int  k = 0;
    std::string tmp = "";
    for (size_t i= 0; i < matrix.size(); i++)
        if ((matrix[i] == ' ' && !tmp.empty()) || (i == matrix.size() - 1 && !tmp.empty())) {
            m[k++] = std::stof(tmp);
            tmp = "";
        }else
            tmp += matrix[i];
    if(tmp != "")
        m[k++] = std::stof(tmp);

    if(k != 12)
        return false;

    // Get the translation (x,y,z) value. Remember the matrix in 3mf is a row major not a column major.
    transformations.push_back(m[9]);
    transformations.push_back(m[10]);
    transformations.push_back(m[11]);

    // Get the scale values.
    double sx = sqrt( m[0] * m[0] + m[1] * m[1] + m[2] * m[2]),
        sy = sqrt( m[3] * m[3] + m[4] * m[4] + m[5] * m[5]),
        sz = sqrt( m[6] * m[6] + m[7] * m[7] + m[8] * m[8]);
    transformations.push_back(sx);
    transformations.push_back(sy);
    transformations.push_back(sz);

    // Get the rotation values.
    // Normalize scale from the rotation matrix.
    m[0] /= sx; m[1] /= sy; m[2] /= sz;
    m[3] /= sx; m[4] /= sy; m[5] /= sz;
    m[6] /= sx; m[7] /= sy; m[8] /= sz;

    // Get quaternion values
    double q_w = sqrt(std::max(0.0, 1.0 + m[0] + m[4] + m[8])) / 2,
        q_x = sqrt(std::max(0.0, 1.0 + m[0] - m[4] - m[8])) / 2,
        q_y = sqrt(std::max(0.0, 1.0 - m[0] + m[4] - m[8])) / 2,
        q_z = sqrt(std::max(0.0, 1.0 - m[0] - m[4] + m[8])) / 2;

    q_x *= ((q_x * (m[5] - m[7])) <= 0 ? -1 : 1);
    q_y *= ((q_y * (m[6] - m[2])) <= 0 ? -1 : 1);
    q_z *= ((q_z * (m[1] - m[3])) <= 0 ? -1 : 1);

    // Normalize quaternion values.
    double q_magnitude = sqrt(q_w * q_w + q_x * q_x + q_y * q_y + q_z * q_z);
    q_w /= q_magnitude;
    q_x /= q_magnitude;
    q_y /= q_magnitude;
    q_z /= q_magnitude;

    double test = q_x * q_y + q_z * q_w;
    double result_x, result_y, result_z;
    // singularity at north pole
    if (test > 0.499)
    {
        result_x = 0;
        result_y = 2 * atan2(q_x, q_w);
        result_z = PI / 2;
    }
        // singularity at south pole
    else if (test < -0.499)
    {
        result_x = 0;
        result_y = -2 * atan2(q_x, q_w);
        result_z = -PI / 2;
    }
    else
    {
        result_x = atan2(2 * q_x * q_w - 2 * q_y * q_z, 1 - 2 * q_x * q_x - 2 * q_z * q_z);
        result_y = atan2(2 * q_y * q_w - 2 * q_x * q_z, 1 - 2 * q_y * q_y - 2 * q_z * q_z);
        result_z = asin(2 * q_x * q_y + 2 * q_z * q_w);

        if (result_x < 0) result_x += 2 * PI;
        if (result_y < 0) result_y += 2 * PI;
        if (result_z < 0) result_z += 2 * PI;
    }
    transformations.push_back(result_x);
    transformations.push_back(result_y);
    transformations.push_back(result_z);

    return true;
}

void
TMFParserContext::apply_transformation(ModelObject *object, std::vector<double> &transformations)
{
    // Apply scale.
    Pointf3 vec(transformations[3], transformations[4], transformations[5]);
    object->scale(vec);

    // Apply x, y & z rotation.
    object->rotate(transformations[6], X);
    object->rotate(transformations[7], Y);
    object->rotate(transformations[8], Z);

    // Apply translation.
    object->translate(transformations[0], transformations[1], transformations[2]);
    return;
}

void
TMFParserContext::apply_transformation(ModelInstance *instance, std::vector<double> &transformations)
{
    // Apply scale.
    instance->scaling_vector = Pointf3(transformations[3], transformations[4], transformations[5]);;

    // Apply x, y & z rotation.
    instance->rotation = transformations[8];
    instance->x_rotation = transformations[6];
    instance->y_rotation = transformations[7];

    // Apply translation.
    instance->offset.x = transformations[0];
    instance->offset.y = transformations[1];
    instance->z_translation = transformations[2];
    return;
}

ModelVolume*
TMFParserContext::add_volume(int start_offset, int end_offset, bool modifier)
{
    ModelVolume* m_volume = nullptr;

    // Add a new volume.
    m_volume = m_object->add_volume(TriangleMesh());
    if(!m_volume || (end_offset < start_offset)) return nullptr;

    // Add the triangles.
    stl_file &stl = m_volume->mesh.stl;
    stl.stats.type = inmemory;
    stl.stats.number_of_facets = (1 + end_offset - start_offset) / 3;
    stl.stats.original_num_facets = stl.stats.number_of_facets;
    stl_allocate(&stl);
    int i_facet = 0;
    for (int i = start_offset; i <= end_offset ;) {
        stl_facet &facet = stl.facet_start[i_facet / 3];
        for (unsigned int v = 0; v < 3; ++v) {
            memcpy(&facet.vertex[v].x, &m_object_vertices[m_volume_facets[i++] * 3], 3 * sizeof(float));
            i_facet++;
        }
    }
    stl_get_size(&stl);
    m_volume->mesh.repair();
    m_volume->modifier = modifier;

    return m_volume;
}

} }
