#include "IO.hpp"
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/nowide/fstream.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace Slic3r { namespace IO {

bool
STL::read(std::string input_file, TriangleMesh* mesh)
{
    // TODO: check that file exists
    
    try {
        mesh->ReadSTLFile(input_file);
        mesh->check_topology();
    } catch (...) {
        throw std::runtime_error("Error while reading STL file");
    }
    return true;
}

bool
STL::read(std::string input_file, Model* model)
{
    TriangleMesh mesh;
    if (!STL::read(input_file, &mesh)) return false;
    
    if (mesh.facets_count() == 0)
        throw std::runtime_error("This STL file couldn't be read because it's empty.");
    
    ModelObject* object = model->add_object();
    object->name        = boost::filesystem::path(input_file).filename().string();
    object->input_file  = input_file;
    
    ModelVolume* volume = object->add_volume(mesh);
    volume->name        = object->name;
    
    return true;
}

bool
STL::write(Model& model, std::string output_file, bool binary)
{
    TriangleMesh mesh = model.mesh();
    return STL::write(mesh, output_file, binary);
}

bool
STL::write(TriangleMesh& mesh, std::string output_file, bool binary)
{
    if (binary) {
        mesh.write_binary(output_file);
    } else {
        mesh.write_ascii(output_file);
    }
    return true;
}

bool
OBJ::read(std::string input_file, TriangleMesh* mesh)
{
    Model model;
    OBJ::read(input_file, &model);
    *mesh = model.mesh();
    
    return true;
}

bool
OBJ::read(std::string input_file, Model* model)
{
    // TODO: encode file name
    // TODO: check that file exists
    
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
	boost::nowide::ifstream ifs(input_file);
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, &ifs);
    
    if (!err.empty()) { // `err` may contain warning message.
        std::cerr << err << std::endl;
    }
    
    if (!ret)
        throw std::runtime_error("Error while reading OBJ file");
    
    ModelObject* object = model->add_object();
    object->name        = boost::filesystem::path(input_file).filename().string();
    object->input_file  = input_file;
    
    // Loop over shapes and add a volume for each one.
    for (std::vector<tinyobj::shape_t>::const_iterator shape = shapes.begin();
        shape != shapes.end(); ++shape) {
        
        Pointf3s points;
        std::vector<Point3> facets;
        
        // Read vertices.
        assert((attrib.vertices.size() % 3) == 0);
        for (size_t v = 0; v < attrib.vertices.size(); v += 3) {
            points.push_back(Pointf3(
                attrib.vertices[v],
                attrib.vertices[v+1],
                attrib.vertices[v+2]
            ));
        }
        
        // Loop over facets of the current shape.
        for (size_t f = 0; f < shape->mesh.num_face_vertices.size(); ++f) {
            // tiny_obj_loader should triangulate any facet with more than 3 vertices
            assert((shape->mesh.num_face_vertices[f] % 3) == 0);
            
            facets.push_back(Point3(
                shape->mesh.indices[f*3+0].vertex_index,
                shape->mesh.indices[f*3+1].vertex_index,
                shape->mesh.indices[f*3+2].vertex_index
            ));
        }
        
        TriangleMesh mesh(points, facets);
        mesh.check_topology();
        ModelVolume* volume = object->add_volume(mesh);
        volume->name        = object->name;
    }
    
    return true;
}

bool
OBJ::write(Model& model, std::string output_file)
{
    TriangleMesh mesh = model.mesh();
    return OBJ::write(mesh, output_file);
}

bool
OBJ::write(TriangleMesh& mesh, std::string output_file)
{
    mesh.WriteOBJFile(output_file);
    return true;
}

bool
POV::write(TriangleMesh& mesh, std::string output_file)
{
    TriangleMesh mesh2 = mesh;
    mesh2.center_around_origin();
    
    using namespace std;
    boost::nowide::ofstream pov;
    pov.open(output_file.c_str(), ios::out | ios::trunc);
    for (int i = 0; i < mesh2.stl.stats.number_of_facets; ++i) {
        const stl_facet &f = mesh2.stl.facet_start[i];
        pov << "triangle { ";
        pov << "<" << f.vertex[0].x << "," << f.vertex[0].y << "," << f.vertex[0].z << ">,";
        pov << "<" << f.vertex[1].x << "," << f.vertex[1].y << "," << f.vertex[1].z << ">,";
        pov << "<" << f.vertex[2].x << "," << f.vertex[2].y << "," << f.vertex[2].z << ">";
        pov << " }" << endl;
    }
    pov.close();
    return true;
}

} }
