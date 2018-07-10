#include "../../miniz/miniz.h"
#include "../Zip/ZipArchive.hpp"

namespace Slic3r {

ZipArchive::ZipArchive (std::string zip_archive_name, char zip_mode) : archive(mz_zip_archive()), zip_name(zip_archive_name), mode(zip_mode), stats(0), finalized(false)
{
    // Initialize the miniz zip archive struct.
    memset(&archive, 0, sizeof(archive));

    if( mode == 'W'){
        stats = mz_zip_writer_init_file(&archive, zip_name.c_str(), 0);
    } else if (mode == 'R') {
        stats = mz_zip_reader_init_file(&archive, zip_name.c_str(), 0);
    } else {
        std::cout << "Error:: Unknown zip mode" << std::endl;
    }
}

mz_bool
ZipArchive::z_stats()
{
    return stats;
}

mz_bool
ZipArchive::add_entry (std::string entry_path, std::string file_path)
{
    stats = 0;
    // Check if it's in the write mode.
    if(mode != 'W')
        return stats;
    stats = mz_zip_writer_add_file(&archive, entry_path.c_str(), file_path.c_str(), nullptr, 0, ZIP_DEFLATE_COMPRESSION);
    return stats;
}

mz_bool
ZipArchive::extract_entry (std::string entry_path, std::string file_path)
{
    stats = 0;
    // Check if it's in the read mode.
    if (mode != 'R')
        return stats;
    stats = mz_zip_reader_extract_file_to_file(&archive, entry_path.c_str(), file_path.c_str(), 0);
    return stats;
}

mz_bool
ZipArchive::finalize()
{
    stats = 0;
    // Finalize the archive and end writing if it's in the write mode.
    if(mode == 'W') {
        stats = mz_zip_writer_finalize_archive(&archive);
        stats |= mz_zip_writer_end(&archive);
    } else if (mode == 'R'){
        stats = mz_zip_reader_end(&archive);
    }
    if(stats)
        finalized = true;
    return stats;
}

ZipArchive::~ZipArchive() {
    if(!finalized)
        this->finalize();
}

}
