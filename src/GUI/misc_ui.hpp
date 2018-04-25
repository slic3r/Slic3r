#ifndef MISC_UI_HPP
#define MISC_UI_HPP

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/filename.h>
#include <wx/stdpaths.h>

/// Common static (that is, free-standing) functions, not part of an object hierarchy.

namespace Slic3r { namespace GUI {

enum class OS { Linux, Mac, Windows } ;
// constants to reduce the proliferation of macros in the rest of the code
#ifdef __WIN32
constexpr OS the_os = OS::Windows;
#elif __APPLE__
constexpr OS the_os = OS::Mac;
#elif __linux__
constexpr OS the_os = OS::Linux;
#endif

#ifdef SLIC3R_DEV
constexpr bool isDev = true;
#else
constexpr bool isDev = false;
#endif

/// Performs a check via the Internet for a new version of Slic3r.
/// If this version of Slic3r was compiled with -DSLIC3R_DEV, check the development 
/// space instead of release.
void check_version(bool manual = false);

const wxString var(const wxString& in);

/// Always returns path to home directory.
const wxString home(const wxString& in = "Slic3r");

}} // namespace Slic3r::GUI

#endif // MISC_UI_HPP
