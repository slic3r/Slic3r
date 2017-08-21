#ifndef SLIC3R_ZIPARCHIVE_H
#define SLIC3R_ZIPARCHIVE_H


#define ZIP_DEFLATE_COMPRESSION 8

#include <string>
#include <iostream>
#include "../../zip/miniz.h" // For IDE code suggestions only.

namespace Slic3r {

class ZipArchive
{
public:

    ZipArchive (std::string zip_archive_name,const char zip_mode) : archive(mz_zip_archive()), zip_name(zip_archive_name), mode(zip_mode), stats(0){
        // Init the miniz zip archive struct.
        memset(&archive, 0, sizeof(archive));

        if( mode == 'W'){
            stats = mz_zip_writer_init_file(&archive, zip_name.c_str(), 0);
        } else if (mode == 'R') {
            stats = mz_zip_reader_init_file(&archive, zip_name.c_str(), MZ_DEFAULT_STRATEGY); // ToDo @Samir55 add flags.
        } else {
            std::cout << "Error unknown zip mode" << std::endl;
        }
    }

    mz_bool z_stats() {
        return stats;
    }

    mz_bool add_entry (std::string entry_path, std::string file_path){
        stats = 0;
        // Check if it's in the write mode.
        if(mode != 'W')
            return stats;
        stats = mz_zip_writer_add_file(&archive, entry_path.c_str(), file_path.c_str(), nullptr, 0, ZIP_DEFLATE_COMPRESSION);
        return stats;
    }

    mz_bool extract_entry (std::string entry_path, std::string file_path){
        stats = 0;
        // Check if it's in the read mode.
        if (mode != 'R')
            return stats;
        stats = mz_zip_reader_extract_file_to_file(&archive, entry_path.c_str(), file_path.c_str(), 0);
        return stats;
    }

    mz_bool finalize() {
        stats = 0;
        // Finalize the archive and end writing if it's in the write mode.
        if(mode == 'W') {
            stats = mz_zip_writer_finalize_archive(&archive);
            stats |= mz_zip_writer_end(&archive);
        } else if (mode == 'R'){
            stats = mz_zip_reader_end(&archive);
        }
        return stats;
    }

private:
    mz_zip_archive archive;
    const std::string zip_name;
    const char mode;
    mz_bool stats;
};

}

#endif //SLIC3R_ZIPARCHIVE_H
