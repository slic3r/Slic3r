#include "IO.hpp"
#include <stdexcept>

namespace Slic3r { namespace IO {

bool
STL::read_file(std::string input_file, Model* model)
{
    // TODO: encode file name
    // TODO: check that file exists
    
    TriangleMesh mesh;
    mesh.ReadSTLFile(input_file);
    mesh.repair();
    
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

} }
