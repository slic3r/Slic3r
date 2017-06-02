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
    static bool read(std::string input_file, TriangleMesh* mesh);
    static bool read(std::string input_file, Model* model);
    static bool write(Model& model, std::string output_file, bool binary = true);
    static bool write(TriangleMesh& mesh, std::string output_file, bool binary = true);
};

class OBJ
{
    public:
    static bool read(std::string input_file, TriangleMesh* mesh);
    static bool read(std::string input_file, Model* model);
    static bool write(Model& model, std::string output_file);
    static bool write(TriangleMesh& mesh, std::string output_file);
};

class AMF
{
    public:
    static bool read(std::string input_file, Model* model);
    static bool write(Model& model, std::string output_file);
};

class POV
{
    public:
    static bool write(TriangleMesh& mesh, std::string output_file);
};

class TMF
{
    public:
    static bool read(std::string input_file, Model* model);
    static bool write(Model& model, std::string output_file);
};

} }

#endif
