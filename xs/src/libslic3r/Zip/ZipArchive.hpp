#ifndef SLIC3R_ZIPARCHIVE_H
#define SLIC3R_ZIPARCHIVE_H

#define MINIZ_HEADER_FILE_ONLY
#define ZIP_DEFLATE_COMPRESSION 8

#include <zip/miniz.h>
#include <string>
#include "../../zip/miniz.h" // For IDE code suggestions only.

namespace Slic3r {

class ZipArchive
{
public:

    ZipArchive(char* zip_archive_name, char zip_mode) : archive(mz_zip_archive()), zip_name(zip_archive_name), mode(zip_mode), stats(0){
        // Init the miniz zip archive struct.
        memset(&archive, 0, sizeof(archive));

        if( mode == 'W'){
            stats = mz_zip_writer_init_file(&archive, zip_name, 0);
        } else if (mode == 'R') {
            stats = mz_zip_reader_init_file(&archive, zip_name, 0); // ToDo @Samir55 add flags.
        } else {
            std::cout << "Error unknown zip mode" << std::endl;
        }
    }

    mz_bool z_stats() {
        return stats;
    }

    mz_bool add_entry(char* entry_path, char* file_path){
        stats = 0;
        // Check if it's in the write mode.
        if(mode != 'W')
            return 0;
        stats = mz_zip_writer_add_file(&archive, entry_path, file_path, nullptr, 0, ZIP_DEFLATE_COMPRESSION);
        return stats;
    }

    mz_bool finalize(){
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
    char* zip_name;
    char mode;
    mz_bool stats;
};

}

#endif //SLIC3R_ZIPARCHIVE_H
