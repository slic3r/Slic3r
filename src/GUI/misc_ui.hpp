#ifndef MISC_UI_HPP
#define MISC_UI_HPP

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/filename.h>
#include <wx/stdpaths.h>

#include "Settings.hpp"

#include "Log.hpp"

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


/// Mostly useful for Linux distro maintainers, this will change where Slic3r assumes 
/// its ./var directory lives (where its art assets are). 
/// Define VAR_ABS and VAR_ABS_PATH 
#ifndef VAR_ABS
    #define VAR_ABS false
#else 
    #define VAR_ABS true
#endif
#ifndef VAR_ABS_PATH
    #define VAR_ABS_PATH "/usr/share/Slic3r/var"
#endif

#ifndef VAR_REL // Redefine on compile
    #define VAR_REL L"/../var"
#endif

/// Performs a check via the Internet for a new version of Slic3r.
/// If this version of Slic3r was compiled with -DSLIC3R_DEV, check the development 
/// space instead of release.
void check_version(bool manual = false);

/// Provides a path to Slic3r's var dir.
const wxString var(const wxString& in);

/// Provide a path to where Slic3r exec'd from.
const wxString bin();

/// Always returns path to home directory.
const wxString home(const wxString& in = "Slic3r");

/// Shows an error messagebox
void show_error(wxWindow* parent, const wxString& message);

/// Shows an info messagebox.
void show_info(wxWindow* parent, const wxString& message, const wxString& title);

/// Show an error messagebox and then throw an exception.
void fatal_error(wxWindow* parent, const wxString& message);


template <typename T>
void append_menu_item(wxMenu* menu, const wxString& name,const wxString& help, T lambda, int id = wxID_ANY, const wxString& icon = "", const wxString& accel = "") {
    wxMenuItem* tmp = menu->Append(wxID_ANY, name, help);
    wxAcceleratorEntry* a = new wxAcceleratorEntry();
    if (!accel.IsEmpty()) {
        a->FromString(accel);
        tmp->SetAccel(a); // set the accelerator if and only if the accelerator is fine
    }
    tmp->SetHelp(help);
    if (!icon.IsEmpty()) 
        tmp->SetBitmap(wxBitmap(var(icon)));

    if (typeid(lambda) != typeid(nullptr))
        menu->Bind(wxEVT_MENU, lambda, tmp->GetId(), tmp->GetId());
}

/*
sub CallAfter {
    my ($self, $cb) = @_;
    push @cb, $cb;
}
*/

wxString decode_path(const wxString& in);
wxString encode_path(const wxString& in);

std::vector<wxString> open_model(wxWindow* parent, const Settings& settings, wxWindow* top);

}} // namespace Slic3r::GUI

#endif // MISC_UI_HPP
