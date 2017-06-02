#include "../IO.hpp"
#include <string>
#include <cstring>
#include <miniz/miniz.c>

namespace Slic3r { namespace IO {

bool
TMF::write(Model& model, std::string output_file){
    // Create a new miniz archive object.
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    // Create a new Zip file names with output_file.
    if(!mz_zip_writer_init_file(&zip_archive, output_file.c_str(), 0))
        return false;

    // Create a buffer to carry data to pass to the zip entry.
    std::string buff = "TEST";

    // Create a 3dmodel.model entry in /3D/ .
    mz_zip_writer_add_mem(&(zip_archive), "3D/3dmodel.model", buff.c_str(), buff.size(), MZ_BEST_COMPRESSION );

    // Test to append the zib file
    mz_zip_add_mem_to_archive_file_in_place( output_file.c_str(), "3D/3dmodel.model", buff.c_str(), buff.size(), NULL, 0, MZ_BEST_COMPRESSION);

    // Finalize the archive and end writing.
    mz_zip_writer_finalize_archive(&zip_archive);
    mz_zip_writer_end(&zip_archive);

    return false;
}

// To be populated later
bool
TMF::read(std::string input_file, Model* model){
    return false;
}

} }