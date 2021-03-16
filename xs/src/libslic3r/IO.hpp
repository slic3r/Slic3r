#ifndef slic3r_IO_hpp_
#define slic3r_IO_hpp_

#include "libslic3r.h"
#include "Model.hpp"
#include "TriangleMesh.hpp"
#include <map>
#include <string>

namespace Slic3r { namespace IO {

enum ExportFormat { AMF, OBJ, POV, STL, SVG, TMF, Gcode };

extern const std::map<ExportFormat,std::string> extensions;
extern const std::map<ExportFormat,bool(*)(const Model&,std::string)> write_model;

class STL
{
    public:
    static bool read(std::string input_file, TriangleMesh* mesh);
    static bool read(std::string input_file, Model* model);
    static bool write(const Model &model, std::string output_file) {
        return STL::write(model, output_file, true);
    };
    static bool write(const Model &model, std::string output_file, bool binary);
    static bool write(const TriangleMesh &mesh, std::string output_file, bool binary = true);
};

class OBJ
{
    public:
    static bool read(std::string input_file, TriangleMesh* mesh);
    static bool read(std::string input_file, Model* model);
    static bool write(const Model& model, std::string output_file);
    static bool write(const TriangleMesh& mesh, std::string output_file);
};

class AMF
{
    public:
    static bool read(std::string input_file, Model* model);
    static bool write(const Model& model, std::string output_file);
};

class POV
{
    public:
    static bool write(const Model& model, std::string output_file);
    static bool write(const TriangleMesh& mesh, std::string output_file);
};

class TMF
{
    public:
    static bool read(std::string input_file, Model* model);
    static bool write(const Model& model, std::string output_file);
};

} }

#endif
