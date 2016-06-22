#ifndef slic3r_IO_hpp_
#define slic3r_IO_hpp_

#include "libslic3r.h"
#include "Model.hpp"
#include "TriangleMesh.hpp"
#include <string>

namespace Slic3r { namespace IO {

class STL
{
    public:
    bool read_file(std::string input_file, Model* model);
    bool write(TriangleMesh& mesh, std::string output_file, bool binary = true);
};

class OBJ
{
    public:
    bool write(TriangleMesh& mesh, std::string output_file);
};

} }

#endif
