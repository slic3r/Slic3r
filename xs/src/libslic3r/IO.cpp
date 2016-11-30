#include "IO.hpp"
#include <stdexcept>
#include <fstream>
#include <iostream>

namespace Slic3r { namespace IO {

bool
STL::read(std::string input_file, TriangleMesh* mesh)
{
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
    // TODO: encode file name
    // TODO: check that file exists
    
    TriangleMesh mesh;
    if (!STL::read(input_file, &mesh)) return false;
    
    if (mesh.facets_count() == 0)
        throw std::runtime_error("This STL file couldn't be read because it's empty.");
    
    ModelObject* object = model->add_object();
    object->name        = input_file;  // TODO: use basename()
    object->input_file  = input_file;
    
    ModelVolume* volume = object->add_volume(mesh);
    volume->name        = input_file;   // TODO: use basename()
    
    return true;
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
    ofstream pov;
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
