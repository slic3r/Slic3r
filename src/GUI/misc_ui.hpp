#ifndef MISC_UI_HPP
#define MISC_UI_HPP

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
    #include <wx/dir.h>
#endif

#include <wx/settings.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <map>
#include <utility>
#include <memory>

#include "Settings.hpp"

#include "Log.hpp"
#include "Point.hpp"


/// Macro to build std::wstring that slic3r::log expects using << syntax of wxString
/// Avoids wx pollution of libslic3r
#define LOG_WSTRING(...) ((wxString("") << __VA_ARGS__).ToStdWstring())


/// Common static (that is, free-standing) functions, not part of an object hierarchy.

namespace Slic3r { namespace GUI {

enum class OS { Linux, Mac, Windows } ;
// constants to reduce the proliferation of macros in the rest of the code
#ifdef __WIN32
constexpr OS the_os = OS::Windows;
constexpr bool wxGTK {false};
#elif __APPLE__
constexpr OS the_os = OS::Mac;
constexpr bool wxGTK {false};
#elif __linux__
constexpr OS the_os = OS::Linux;
    #ifdef __WXGTK__
    constexpr bool wxGTK {true};
    #else 
    constexpr bool wxGTK {false};
    #endif
#endif

#ifdef SLIC3R_DEV
constexpr bool isDev = true;
#else
constexpr bool isDev = false;
#endif

constexpr bool threaded = false;


// hopefully the compiler is smart enough to figure this out
const std::map<const std::string, const std::string> FILE_WILDCARDS { 
      std::make_pair("known", "Known files (*.stl, *.obj, *.amf, *.xml, *.3mf)|*.3mf;*.3MF;*.stl;*.STL;*.obj;*.OBJ;*.amf;*.AMF;*.xml;*.XML"),
      std::make_pair("stl", "STL files (*.stl)|*.stl;*.STL"),
      std::make_pair("obj", "OBJ files (*.obj)|*.obj;*.OBJ"),
      std::make_pair("amf", "AMF files (*.amf)|*.amf;*.AMF;*.xml;*.XML"),
      std::make_pair("tmf", "3MF files (*.3mf)|*.3mf;*.3MF"),
      std::make_pair("ini", "INI files *.ini|*.ini;*.INI"),
      std::make_pair("gcode", "G-code files (*.gcode, *.gco, *.g, *.ngc)|*.gcode;*.GCODE;*.gco;*.GCO;*.g;*.G;*.ngc;*.NGC"),
      std::make_pair("svg", "SVG files *.svg|*.svg;*.SVG")
    };

const std::string MODEL_WILDCARD { FILE_WILDCARDS.at("known") + std::string("|") + FILE_WILDCARDS.at("stl")+ std::string("|") + FILE_WILDCARDS.at("obj") + std::string("|") + FILE_WILDCARDS.at("amf")+ std::string("|") + FILE_WILDCARDS.at("tmf")};
const std::string STL_MODEL_WILDCARD { FILE_WILDCARDS.at("stl") };
const std::string AMF_MODEL_WILDCARD { FILE_WILDCARDS.at("amf") };
const std::string TMF_MODEL_WILDCARD { FILE_WILDCARDS.at("tmf") };


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

/// Assign a menu item icon
void set_menu_item_icon(wxMenuItem* item, const wxString& icon);

/// Construct a menu item for Slic3r, append it to a menu, and return it. 
/// Automatically binds the lambda method to the event handler of the menu for this menu item's id.
/// Assign the accelerator separately if one is desired (instead of the \t interface in the name) 
/// to permit translation.
template <typename T>
wxMenuItem* append_menu_item(wxMenu* menu, const wxString& name,const wxString& help, T lambda, int id = wxID_ANY, const wxString& icon = "", const wxString& accel = "", const wxItemKind kind = wxITEM_NORMAL) {
    wxMenuItem* tmp = menu->Append(wxID_ANY, name, help, kind);
    wxAcceleratorEntry* a = new wxAcceleratorEntry();
    if (!accel.IsEmpty()) {
        a->FromString(accel);
        tmp->SetAccel(a); // set the accelerator if and only if the accelerator is fine
    }
    tmp->SetHelp(help);
    set_menu_item_icon(tmp, icon);

    if (typeid(lambda) != typeid(nullptr))
        menu->Bind(wxEVT_MENU, lambda, tmp->GetId(), tmp->GetId());

    return tmp;
}

/// Construct and return a submenu to the menu, optionally with an icon. 
wxMenuItem* append_submenu(wxMenu* menu, const wxString& name, const wxString& help, wxMenu* submenu, int id = wxID_ANY, const wxString& icon = "");

/*
sub CallAfter {
    my ($self, $cb) = @_;
    push @cb, $cb;
}
*/

wxString decode_path(const wxString& in);
wxString encode_path(const wxString& in);

std::vector<wxString> open_model(wxWindow* parent, wxWindow* top);

inline Slic3r::Point new_scale(const wxPoint& p) { return Slic3r::Point::new_scale(p.x, p.y); }

/// Singleton for UI settings.
extern std::unique_ptr<Settings> ui_settings; 

wxString trim_zeroes(wxString in);

/// Extensible directory traversal sink, cribbed from wxwidgets docs
class wxDirTraverserSimple : public wxDirTraverser
{
public:
    std::function<void(const wxString&)> file_cb {nullptr};
    std::function<void(const wxString&)> dir_cb {nullptr};
    wxDirTraverserSimple() { }
    virtual wxDirTraverseResult OnFile(const wxString& filename)
    {
        if (this->file_cb != nullptr) this->file_cb(filename);
        return wxDIR_CONTINUE;
    }
    virtual wxDirTraverseResult OnDir(const wxString& dirname)
    {
        if (this->dir_cb != nullptr) this->file_cb(dirname);
        return wxDIR_CONTINUE;
    }
};

/// Quick implementation of grep to filter containers based on some lambda function.
template<class T, class G>
T grep(const T& container, G lambda_func) {
    T result;
    std::copy_if(container.cbegin(), container.cend(),  std::back_inserter(result), lambda_func);
    return result;
}

}} // namespace Slic3r::GUI

#endif // MISC_UI_HPP
