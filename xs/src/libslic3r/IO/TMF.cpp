#include "../IO.hpp"
#define MINIZ_HEADER_FILE_ONLY
#include <miniz/miniz.c>

namespace Slic3r { namespace IO {

// To be populated later
static bool
write(Model& model, std::string output_file){
    return false;
}

// To be populated later
static bool
read(std::string input_file, Model* model){
    return false;
}

} }