#ifndef SLIC3R_ZIPARCHIVE_H
#define SLIC3R_ZIPARCHIVE_H

#define MINIZ_HEADER_FILE_ONLY
#define ZIP_DEFLATE_COMPRESSION 8

#include <string>
#include <iostream>
#include "../../miniz/miniz.h"

namespace Slic3r {

/// A zip wrapper for Miniz lib.
class ZipArchive
{
public:

    /// Create a zip archive.
    /// \param zip_archive_name string the name of the zip file.
    /// \param zip_mode char the zip archive mode ('R' means read mode, 'W' means write mode). you cannot change the mode for the current object.
    ZipArchive(std::string zip_archive_name, char zip_mode);

    /// Get the status of the previous operation applied on the zip_archive.
    /// \return mz_bool 0: failure 1: success.
    mz_bool z_stats();

    /// Add a file to the current zip archive.
    /// \param entry_path string the path of the entry in the zip archive.
    /// \param file_path string the path of the file in the disk.
    /// \return mz_bool 0: failure 1: success.
    mz_bool add_entry (std::string entry_path, std::string file_path);

    /// Extract a zip entry to a file on the disk.
    /// \param entry_path string the path of the entry in the zip archive.
    /// \param file_path string the path of the file in the disk.
    /// \return mz_bool 0: failure 1: success.
    mz_bool extract_entry (std::string entry_path, std::string file_path);

    /// Finalize the archive and free any allocated memory.
    /// \return mz_bool 0: failure 1: success.
    mz_bool finalize();

    ~ZipArchive();

private:
    mz_zip_archive archive; ///< Miniz zip archive struct.
    const std::string zip_name; ///< The zip file name.
    const char mode; ///< The zip mode either read or write.
    mz_bool stats; ///< The status of the recently executed operation on the zip archive.
    bool finalized; ///< Whether the zip archive is finalized or not. Used during the destructor of the object.

};

}

#endif //SLIC3R_ZIPARCHIVE_H
