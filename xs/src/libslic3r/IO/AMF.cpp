#include "../IO.hpp"
#include <iostream>
#include <fstream>
#include <cstring>
#include <map>
#include <string>
#include <boost/move/move.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/nowide/iostream.hpp>
#include <boost/algorithm/string.hpp>
#include <expat/expat.h>
#include <miniz/miniz.h>
#include "../Exception.hpp"
#include "miniz_extension.hpp"

namespace Slic3r { namespace IO {
bool load_amf_archive(const char* path, Model* model, bool check_version);
bool extract_model_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat, Model* model, bool check_version);

struct AMFParserContext
{
    AMFParserContext(XML_Parser parser, Model *model) :
        m_parser(parser),
        m_model(*model), 
        m_object(NULL), 
        m_volume(NULL),
        m_material(NULL),
        m_instance(NULL)
    {
        m_path.reserve(12);
    }

    void stop() 
    {
        XML_StopParser(m_parser, 0);
    }

    void startElement(const char *name, const char **atts);
    void endElement(const char *name);
    void endDocument();
    void characters(const XML_Char *s, int len);

    static void XMLCALL startElement(void *userData, const char *name, const char **atts)
    {
        AMFParserContext *ctx = (AMFParserContext*)userData;
        ctx->startElement(name, atts);
    }

    static void XMLCALL endElement(void *userData, const char *name)
    {
        AMFParserContext *ctx = (AMFParserContext*)userData;
        ctx->endElement(name);
    }

    /* s is not 0 terminated. */
    static void XMLCALL characters(void *userData, const XML_Char *s, int len)
    {
        AMFParserContext *ctx = (AMFParserContext*)userData;
        ctx->characters(s, len);    
    }

    static const char* get_attribute(const char **atts, const char *id) {
        if (atts == NULL)
            return NULL;
        while (*atts != NULL) {
            if (strcmp(*(atts ++), id) == 0)
                return *atts;
            ++ atts;
        }
        return NULL;
    }

    enum AMFNodeType {
        NODE_TYPE_INVALID = 0,
        NODE_TYPE_UNKNOWN,
        NODE_TYPE_AMF,                  // amf
                                        // amf/metadata
        NODE_TYPE_MATERIAL,             // amf/material
                                        // amf/material/metadata
        NODE_TYPE_OBJECT,               // amf/object
                                        // amf/object/metadata
        NODE_TYPE_MESH,                 // amf/object/mesh
        NODE_TYPE_VERTICES,             // amf/object/mesh/vertices
        NODE_TYPE_VERTEX,               // amf/object/mesh/vertices/vertex
        NODE_TYPE_COORDINATES,          // amf/object/mesh/vertices/vertex/coordinates
        NODE_TYPE_COORDINATE_X,         // amf/object/mesh/vertices/vertex/coordinates/x
        NODE_TYPE_COORDINATE_Y,         // amf/object/mesh/vertices/vertex/coordinates/y
        NODE_TYPE_COORDINATE_Z,         // amf/object/mesh/vertices/vertex/coordinates/z
        NODE_TYPE_VOLUME,               // amf/object/mesh/volume
                                        // amf/object/mesh/volume/metadata
        NODE_TYPE_TRIANGLE,             // amf/object/mesh/volume/triangle
        NODE_TYPE_VERTEX1,              // amf/object/mesh/volume/triangle/v1
        NODE_TYPE_VERTEX2,              // amf/object/mesh/volume/triangle/v2
        NODE_TYPE_VERTEX3,              // amf/object/mesh/volume/triangle/v3
        NODE_TYPE_CONSTELLATION,        // amf/constellation
        NODE_TYPE_INSTANCE,             // amf/constellation/instance
        NODE_TYPE_DELTAX,               // amf/constellation/instance/deltax
        NODE_TYPE_DELTAY,               // amf/constellation/instance/deltay
        NODE_TYPE_RZ,                   // amf/constellation/instance/rz
        NODE_TYPE_SCALE,                // amf/constellation/instance/scale
        NODE_TYPE_METADATA,             // anywhere under amf/*/metadata
    };

    struct Instance {
        Instance() : deltax_set(false), deltay_set(false), rz_set(false), scale_set(false) {}
        // Shift in the X axis.
        float deltax;
        bool  deltax_set;
        // Shift in the Y axis.
        float deltay;
        bool  deltay_set;
        // Rotation around the Z axis.
        float rz;
        bool  rz_set;
        // Scaling factor
        float scale;
        bool  scale_set;
    };

    struct Object {
        Object() : idx(-1) {}
        int                     idx;
        std::vector<Instance>   instances;
    };

    // Current Expat XML parser instance.
    XML_Parser               m_parser;
    // Model to receive objects extracted from an AMF file.
    Model                   &m_model;
    // Current parsing path in the XML file.
    std::vector<AMFNodeType> m_path;
    // Current object allocated for an amf/object XML subtree.
    ModelObject             *m_object;
    // Map from object name to object idx & instances.
    std::map<std::string, Object> m_object_instances_map;
    // Vertices parsed for the current m_object.
    std::vector<float>       m_object_vertices;
    // Current volume allocated for an amf/object/mesh/volume subtree.
    ModelVolume             *m_volume;
    // Faces collected for the current m_volume.
    std::vector<int>         m_volume_facets;
    // Current material allocated for an amf/metadata subtree.
    ModelMaterial           *m_material;
    // Current instance allocated for an amf/constellation/instance subtree.
    Instance                *m_instance;
    // Generic string buffer for vertices, face indices, metadata etc.
    std::string              m_value[3];
};

void AMFParserContext::startElement(const char *name, const char **atts)
{
    AMFNodeType node_type_new = NODE_TYPE_UNKNOWN;
    switch (m_path.size()) {
    case 0:
        // An AMF file must start with an <amf> tag.
        node_type_new = NODE_TYPE_AMF;
        if (strcmp(name, "amf") != 0)
            this->stop();
        break;
    case 1:
        if (strcmp(name, "metadata") == 0) {
            const char *type = get_attribute(atts, "type");
            if (type != NULL) {
                m_value[0] = type;
                node_type_new = NODE_TYPE_METADATA;
            }
        } else if (strcmp(name, "material") == 0) {
            const char *material_id = get_attribute(atts, "id");
            m_material = m_model.add_material((material_id == NULL) ? "_" : material_id);
            node_type_new = NODE_TYPE_MATERIAL;
        } else if (strcmp(name, "object") == 0) {
            const char *object_id = get_attribute(atts, "id");
            if (object_id == NULL)
                this->stop();
            else {
				assert(m_object_vertices.empty());
                m_object = m_model.add_object();
                m_object_instances_map[object_id].idx = int(m_model.objects.size())-1;
                node_type_new = NODE_TYPE_OBJECT;
            }
        } else if (strcmp(name, "constellation") == 0) {
            node_type_new = NODE_TYPE_CONSTELLATION;
        }
        break;
    case 2:
        if (strcmp(name, "metadata") == 0) {
            if (m_path[1] == NODE_TYPE_MATERIAL || m_path[1] == NODE_TYPE_OBJECT) {
                m_value[0] = get_attribute(atts, "type");
                node_type_new = NODE_TYPE_METADATA;
            }
        } else if (strcmp(name, "mesh") == 0) {
            if (m_path[1] == NODE_TYPE_OBJECT)
                node_type_new = NODE_TYPE_MESH;
        } else if (strcmp(name, "instance") == 0) {
            if (m_path[1] == NODE_TYPE_CONSTELLATION) {
                const char *object_id = get_attribute(atts, "objectid");
                if (object_id == NULL)
                    this->stop();
                else {
                    m_object_instances_map[object_id].instances.push_back(AMFParserContext::Instance());
                    m_instance = &m_object_instances_map[object_id].instances.back(); 
                    node_type_new = NODE_TYPE_INSTANCE;
                }
            }
            else
                this->stop();
        }
        break;
    case 3:
        if (m_path[2] == NODE_TYPE_MESH) {
			assert(m_object);
            if (strcmp(name, "vertices") == 0)
                node_type_new = NODE_TYPE_VERTICES;
			else if (strcmp(name, "volume") == 0) {
				assert(! m_volume);
				m_volume = m_object->add_volume(TriangleMesh());
				node_type_new = NODE_TYPE_VOLUME;
			}
        } else if (m_path[2] == NODE_TYPE_INSTANCE) {
            assert(m_instance);
            if (strcmp(name, "deltax") == 0)
                node_type_new = NODE_TYPE_DELTAX; 
            else if (strcmp(name, "deltay") == 0)
                node_type_new = NODE_TYPE_DELTAY;
            else if (strcmp(name, "rz") == 0)
                node_type_new = NODE_TYPE_RZ;
            else if (strcmp(name, "scale") == 0)
                node_type_new = NODE_TYPE_SCALE;
        }
        break;
    case 4:
        if (m_path[3] == NODE_TYPE_VERTICES) {
            if (strcmp(name, "vertex") == 0)
                node_type_new = NODE_TYPE_VERTEX; 
        } else if (m_path[3] == NODE_TYPE_VOLUME) {
            if (strcmp(name, "metadata") == 0) {
                const char *type = get_attribute(atts, "type");
                if (type == NULL)
                    this->stop();
                else {
                    m_value[0] = type;
                    node_type_new = NODE_TYPE_METADATA;
                }
            } else if (strcmp(name, "triangle") == 0)
                node_type_new = NODE_TYPE_TRIANGLE;
        }
        break;
    case 5:
        if (strcmp(name, "coordinates") == 0) {
            if (m_path[4] == NODE_TYPE_VERTEX) {
                node_type_new = NODE_TYPE_COORDINATES; 
            } else
                this->stop();
        } else if (name[0] == 'v' && name[1] >= '1' && name[1] <= '3' && name[2] == 0) {
            if (m_path[4] == NODE_TYPE_TRIANGLE) {
                node_type_new = AMFNodeType(NODE_TYPE_VERTEX1 + name[1] - '1');
            } else
                this->stop();
        }
        break;
    case 6:
        if ((name[0] == 'x' || name[0] == 'y' || name[0] == 'z') && name[1] == 0) {
            if (m_path[5] == NODE_TYPE_COORDINATES)
                node_type_new = AMFNodeType(NODE_TYPE_COORDINATE_X + name[0] - 'x');
            else
                this->stop();
        }
        break;
    default:
        break;
    }

    m_path.push_back(node_type_new);
}

void AMFParserContext::characters(const XML_Char *s, int len)
{
    if (m_path.back() == NODE_TYPE_METADATA) {
        m_value[1].append(s, len);
    }
    else
    {
        switch (m_path.size()) {
        case 4:
            if (m_path.back() == NODE_TYPE_DELTAX || m_path.back() == NODE_TYPE_DELTAY || m_path.back() == NODE_TYPE_RZ || m_path.back() == NODE_TYPE_SCALE)
                m_value[0].append(s, len);
            break;
        case 6:
            switch (m_path.back()) {
                case NODE_TYPE_VERTEX1: m_value[0].append(s, len); break;
                case NODE_TYPE_VERTEX2: m_value[1].append(s, len); break;
                case NODE_TYPE_VERTEX3: m_value[2].append(s, len); break;
                default: break;
            }
        case 7:
            switch (m_path.back()) {
                case NODE_TYPE_COORDINATE_X: m_value[0].append(s, len); break;
                case NODE_TYPE_COORDINATE_Y: m_value[1].append(s, len); break;
                case NODE_TYPE_COORDINATE_Z: m_value[2].append(s, len); break;
                default: break;
            }
        default:
            break;
        }
    }
}

void AMFParserContext::endElement(const char *name)
{
    switch (m_path.back()) {

    // Constellation transformation:
    case NODE_TYPE_DELTAX:
        assert(m_instance);
        m_instance->deltax = float(atof(m_value[0].c_str()));
        m_instance->deltax_set = true;
        m_value[0].clear();
        break;
    case NODE_TYPE_DELTAY:
        assert(m_instance);
        m_instance->deltay = float(atof(m_value[0].c_str()));
        m_instance->deltay_set = true;
        m_value[0].clear();
        break;
    case NODE_TYPE_RZ:
        assert(m_instance);
        m_instance->rz = float(atof(m_value[0].c_str()));
        m_instance->rz_set = true;
        m_value[0].clear();
        break;
    case NODE_TYPE_SCALE:
        assert(m_instance);
        m_instance->scale = float(atof(m_value[0].c_str()));
        m_instance->scale_set = true;
        m_value[0].clear();
        break;

    // Object vertices:
    case NODE_TYPE_VERTEX:
        assert(m_object);
        // Parse the vertex data
        m_object_vertices.push_back(atof(m_value[0].c_str()));
        m_object_vertices.push_back(atof(m_value[1].c_str()));
        m_object_vertices.push_back(atof(m_value[2].c_str()));
        m_value[0].clear();
        m_value[1].clear();
        m_value[2].clear();
        break;

    // Faces of the current volume:
    case NODE_TYPE_TRIANGLE:
        assert(m_object && m_volume);
        if (strtoul(m_value[0].c_str(), nullptr, 10) < m_object_vertices.size() &&
            strtoul(m_value[1].c_str(), nullptr, 10) < m_object_vertices.size() &&
            strtoul(m_value[2].c_str(), nullptr, 10) < m_object_vertices.size()) {
            m_volume_facets.push_back(atoi(m_value[0].c_str()));
            m_volume_facets.push_back(atoi(m_value[1].c_str()));
            m_volume_facets.push_back(atoi(m_value[2].c_str()));
        }
        m_value[0].clear();
        m_value[1].clear();
        m_value[2].clear();
        break;

    // Closing the current volume. Create an STL from m_volume_facets pointing to m_object_vertices.
    case NODE_TYPE_VOLUME:
    {
		assert(m_object && m_volume);
        stl_file &stl = m_volume->mesh.stl;
        stl.stats.type = inmemory;
        stl.stats.number_of_facets = int(m_volume_facets.size() / 3);
        stl.stats.original_num_facets = stl.stats.number_of_facets;
        stl_allocate(&stl);
        for (size_t i = 0; i < m_volume_facets.size();) {
            stl_facet &facet = stl.facet_start[i/3];
            for (unsigned int v = 0; v < 3; ++ v)
                memcpy(&facet.vertex[v].x, &m_object_vertices[m_volume_facets[i ++] * 3], 3 * sizeof(float));
        }
        stl_get_size(&stl);
        m_volume->mesh.repair();
        m_volume_facets.clear();
        m_volume = NULL;
        break;
    }

    case NODE_TYPE_OBJECT:
        assert(m_object);
        m_object_vertices.clear();
        m_object = NULL;
        break;

    case NODE_TYPE_MATERIAL:
        assert(m_material);
        m_material = NULL;
        break;

    case NODE_TYPE_INSTANCE:
        assert(m_instance);
        m_instance = NULL;
        break;

    case NODE_TYPE_METADATA:
        if (strncmp(m_value[0].c_str(), "slic3r.", 7) == 0) {
            const char *opt_key = m_value[0].c_str() + 7;
            if (print_config_def.options.find(opt_key) != print_config_def.options.end()) {
                DynamicPrintConfig *config = NULL;
                if (m_path.size() == 3) {
                    if (m_path[1] == NODE_TYPE_MATERIAL && m_material)
                        config = &m_material->config;
                    else if (m_path[1] == NODE_TYPE_OBJECT && m_object)
                        config = &m_object->config;
                } else if (m_path.size() == 5 && m_path[3] == NODE_TYPE_VOLUME && m_volume)
                    config = &m_volume->config;
                if (config)
                    config->set_deserialize(opt_key, m_value[1]);
            } else if (m_path.size() == 5 && m_path[3] == NODE_TYPE_VOLUME && m_volume && strcmp(opt_key, "modifier") == 0) {
                // Is this volume a modifier volume?
                m_volume->modifier = atoi(m_value[1].c_str()) == 1;
            }
        } else if (m_path.size() == 3) {
            if (m_path[1] == NODE_TYPE_MATERIAL) {
                if (m_material)
                    m_material->attributes[m_value[0]] = m_value[1];
            } else if (m_path[1] == NODE_TYPE_OBJECT) {
                if (m_object && m_value[0] == "name")
                    m_object->name = boost::move(m_value[1]);
            }
        } else if (m_path.size() == 5 && m_path[3] == NODE_TYPE_VOLUME) {
            if (m_volume && m_value[0] == "name")
                m_volume->name = boost::move(m_value[1]);
        }
        m_value[0].clear();
        m_value[1].clear();
        break;
    default:
        break;
    }

    m_path.pop_back();
}

void AMFParserContext::endDocument()
{
    for (const auto &object : m_object_instances_map) {
        if (object.second.idx == -1) {
            printf("Undefined object %s referenced in constellation\n", object.first.c_str());
            continue;
        }
        for (const Instance &instance : object.second.instances)
            if (instance.deltax_set && instance.deltay_set) {
                ModelInstance *mi = m_model.objects[object.second.idx]->add_instance();
                mi->offset.x = instance.deltax;
                mi->offset.y = instance.deltay;
                mi->rotation = instance.rz_set ? instance.rz : 0.f;
                mi->scaling_factor = instance.scale_set ? instance.scale : 1.f;
            }
    }
}

bool
AMF::read(std::string input_file, Model* model)
{
    // Quick and dirty hack from PrusaSlic3r's AMF deflate
    if (boost::iends_with(input_file.c_str(), ".amf")) {
        boost::nowide::ifstream file(input_file.c_str(), boost::nowide::ifstream::binary);
        if (!file.good())
            return false;

        char buffer[3];
        file.read(buffer, 2);
        buffer[2] = '\0';
        file.close();
        if (std::strcmp(buffer, "PK") == 0)
            return load_amf_archive(input_file.c_str(), model, false);
    }

    XML_Parser parser = XML_ParserCreate(NULL); // encoding
    if (! parser) {
        printf("Couldn't allocate memory for parser\n");
        return false;
    }
    
    boost::nowide::ifstream fin(input_file, std::ios::in);
    if (!fin.is_open()) {
        boost::nowide::cerr << "Cannot open file: " << input_file << std::endl;
        return false;
    }

    AMFParserContext ctx(parser, model);
    XML_SetUserData(parser, (void*)&ctx);
    XML_SetElementHandler(parser, AMFParserContext::startElement, AMFParserContext::endElement);
    XML_SetCharacterDataHandler(parser, AMFParserContext::characters);

    char buff[8192];
    bool result = false;
    while (!fin.eof()) {
		fin.read(buff, sizeof(buff));
        if (fin.bad()) {
            printf("AMF parser: Read error\n");
            break;
        }
        if (XML_Parse(parser, buff, fin.gcount(), fin.eof()) == XML_STATUS_ERROR) {
            printf("AMF parser: Parse error at line %lu:\n%s\n",
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

    if (result)
        ctx.endDocument();
    return result;
}

bool
AMF::write(const Model& model, std::string output_file)
{
    using namespace std;
    
    boost::nowide::ofstream file;
    file.open(output_file, ios::out | ios::trunc);
    
    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl
         << "<amf unit=\"millimeter\">" << endl
         << "  <metadata type=\"cad\">Slic3r " << SLIC3R_VERSION << "</metadata>" << endl;
    
    for (const auto &material : model.materials) {
        if (material.first.empty())
            continue;
        // note that material-id must never be 0 since it's reserved by the AMF spec
        file << "  <material id=\"" << material.first << "\">" << endl;
        for (const auto &attr : material.second->attributes)
             file << "    <metadata type=\"" << attr.first << "\">" << attr.second << "</metadata>" << endl;
        for (const std::string &key : material.second->config.keys())
             file << "    <metadata type=\"slic3r." << key << "\">"
                  << material.second->config.serialize(key) << "</metadata>" << endl;
        file << "  </material>" << endl;
    }
    
    ostringstream instances;
    for (size_t object_id = 0; object_id < model.objects.size(); ++object_id) {
        ModelObject *object = model.objects[object_id];
        file << "  <object id=\"" << object_id << "\">" << endl;
        
        for (const std::string &key : object->config.keys())
            file << "    <metadata type=\"slic3r." << key << "\">"
                 << object->config.serialize(key) << "</metadata>" << endl;
        
        if (!object->name.empty())
            file << "    <metadata type=\"name\">" << object->name << "</metadata>" << endl;

        //FIXME: Store the layer height ranges (ModelObject::layer_height_ranges)
        file << "    <mesh>" << endl;
        file << "      <vertices>" << endl;
        
        std::vector<size_t> vertices_offsets;
        size_t num_vertices = 0;

        Pointf3 origin_translation = object->origin_translation();
        
        for (ModelVolume *volume : object->volumes) {
            volume->mesh.require_shared_vertices();
            vertices_offsets.push_back(num_vertices);
            const auto &stl = volume->mesh.stl;
            for (size_t i = 0; i < static_cast<size_t>(stl.stats.shared_vertices); ++i)
                // Subtract origin_translation in order to restore the coordinates of the parts
                // before they were imported. Otherwise, when this AMF file is reimported parts
                // will be placed in the plater correctly, but we will have lost origin_translation
                // thus any additional part added will not align with the others.
                // In order to do this we compensate for this translation in the instance placement
                // below.
                file << "         <vertex>" << endl
                     << "           <coordinates>" << endl
                     << "             <x>" << (stl.v_shared[i].x - origin_translation.x) << "</x>" << endl
                     << "             <y>" << (stl.v_shared[i].y - origin_translation.y) << "</y>" << endl
                     << "             <z>" << (stl.v_shared[i].z - origin_translation.z) << "</z>" << endl
                     << "           </coordinates>" << endl
                     << "         </vertex>" << endl;
            
            num_vertices += stl.stats.shared_vertices;
        }
        file << "      </vertices>" << endl;
        
        for (size_t i_volume = 0; i_volume < object->volumes.size(); ++i_volume) {
            ModelVolume *volume = object->volumes[i_volume];
            int vertices_offset = vertices_offsets[i_volume];
            
            if (volume->material_id().empty())
                file << "      <volume>" << endl;
            else
                file << "      <volume materialid=\"" << volume->material_id() << "\">" << endl;
            
            for (const std::string &key : volume->config.keys())
                file << "        <metadata type=\"slic3r." << key << "\">"
                     << volume->config.serialize(key) << "</metadata>" << endl;
            
            if (!volume->name.empty())
                file << "        <metadata type=\"name\">" << volume->name << "</metadata>" << endl;
            
            if (volume->modifier)
                file << "        <metadata type=\"slic3r.modifier\">1</metadata>" << endl;
            
            for (int i = 0; i < volume->mesh.stl.stats.number_of_facets; ++i) {
                file << "        <triangle>" << endl;
                for (int j = 0; j < 3; ++ j)
                    file << "          <v" << (j+1) << ">"
                         << (volume->mesh.stl.v_indices[i].vertex[j] + vertices_offset)
                         << "</v" << (j+1) << ">" << endl;
                file << "        </triangle>" << endl;
            }
            file << "      </volume>" << endl;
        }
        file << "    </mesh>" << endl;
        file << "  </object>" << endl;
        
        for (const ModelInstance* instance : object->instances)
            instances
                << "    <instance objectid=\"" << object_id << "\">" << endl
                << "      <deltax>" << instance->offset.x + origin_translation.x << "</deltax>" << endl
                << "      <deltay>" << instance->offset.y + origin_translation.y << "</deltay>" << endl
                << "      <rz>" << instance->rotation << "</rz>" << endl
                << "      <scale>" << instance->scaling_factor << "</scale>" << endl
                << "    </instance>" << endl;
    }
    
    std::string instances_str = instances.str();
    if (!instances_str.empty())
        file << "  <constellation id=\"1\">" << endl
             << instances_str
             << "  </constellation>" << endl;
    
    file << "</amf>" << endl;
    
    file.close();
    return true;
}

bool extract_model_from_archive(mz_zip_archive& archive, const mz_zip_archive_file_stat& stat, Model* model, bool check_version)
{
    if (stat.m_uncomp_size == 0)
    {
        printf("Found invalid size\n");
        close_zip_reader(&archive);
        return false;
    }

    XML_Parser parser = XML_ParserCreate(nullptr); // encoding
    if (!parser) {
        printf("Couldn't allocate memory for parser\n");
        close_zip_reader(&archive);
        return false;
    }

    AMFParserContext ctx(parser, model);
    XML_SetUserData(parser, (void*)&ctx);
    XML_SetElementHandler(parser, AMFParserContext::startElement, AMFParserContext::endElement);
    XML_SetCharacterDataHandler(parser, AMFParserContext::characters);

    struct CallbackData
    {
        XML_Parser& parser;
        const mz_zip_archive_file_stat& stat;

        CallbackData(XML_Parser& parser, const mz_zip_archive_file_stat& stat) : parser(parser), stat(stat) {}
    };

    CallbackData data(parser, stat);

    mz_bool res = 0;

    try
    {
        res = mz_zip_reader_extract_file_to_callback(&archive, stat.m_filename, [](void* pOpaque, mz_uint64 file_ofs, const void* pBuf, size_t n)->size_t {
            CallbackData* data = (CallbackData*)pOpaque;
            if (!XML_Parse(data->parser, (const char*)pBuf, (int)n, (file_ofs + n == data->stat.m_uncomp_size) ? 1 : 0))
            {
                char error_buf[1024];
                ::sprintf(error_buf, "Error (%s) while parsing '%s' at line %d", XML_ErrorString(XML_GetErrorCode(data->parser)), data->stat.m_filename, (int)XML_GetCurrentLineNumber(data->parser));
                throw Slic3r::FileIOError(error_buf);
            }

            return n;
            }, &data, 0);
    }
    catch (std::exception& e)
    {
        printf("%s\n", e.what());
        close_zip_reader(&archive);
        return false;
    }

    if (res == 0)
    {
        printf("Error while extracting model data from zip archive");
        close_zip_reader(&archive);
        return false;
    }

    ctx.endDocument();

    return true;
}

// Load an AMF archive into a provided model.
bool load_amf_archive(const char* path, Model* model, bool check_version)
{
    if ((path == nullptr) || (model == nullptr))
        return false;

    mz_zip_archive archive;
    mz_zip_zero_struct(&archive);

    if (!open_zip_reader(&archive, path))
    {
        printf("Unable to init zip reader\n");
        return false;
    }

    mz_uint num_entries = mz_zip_reader_get_num_files(&archive);

    mz_zip_archive_file_stat stat;
    // we first loop the entries to read from the archive the .amf file only, in order to extract the version from it
    for (mz_uint i = 0; i < num_entries; ++i)
    {
        if (mz_zip_reader_file_stat(&archive, i, &stat))
        {
            if (boost::iends_with(stat.m_filename, ".amf"))
            {
                try
                {
                    if (!extract_model_from_archive(archive, stat, model, check_version))
                    {
                        close_zip_reader(&archive);
                        printf("Archive does not contain a valid model");
                        return false;
                    }
                }
                catch (const std::exception& e)
                {
                    // ensure the zip archive is closed and rethrow the exception
                    close_zip_reader(&archive);
                    throw std::runtime_error(e.what());
                }

                break;
            }
        }
    }

#if 0 // forward compatibility
    // we then loop again the entries to read other files stored in the archive
    for (mz_uint i = 0; i < num_entries; ++i)
    {
        if (mz_zip_reader_file_stat(&archive, i, &stat))
        {
            // add code to extract the file
        }
    }
#endif // forward compatibility

    close_zip_reader(&archive);

    return true;
}


} }
