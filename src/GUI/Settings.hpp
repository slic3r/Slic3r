#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <map>
#include <tuple>

#include "libslic3r.h"

namespace Slic3r { namespace GUI {

enum class PathColor {
    role
};

enum class ColorScheme {
    solarized, slic3r
};

enum class ReloadBehavior {
    all, copy, discard
};

/// Stub class to hold onto GUI-specific settings options. 
/// TODO: Incorporate the system from libslic3r
class Settings { 
    public:
        bool show_host {false};
        bool version_check {true};
        bool autocenter {true};
        bool autoalignz {true};
        bool invert_zoom {false};
        bool background_processing {false};

        bool preset_editor_tabs {false};

        bool hide_reload_dialog {false};

        ReloadBehavior reload {ReloadBehavior::all};
        ColorScheme color {ColorScheme::slic3r};
        PathColor color_toolpaths_by {PathColor::role};

        float nudge {1.0}; //< 2D plater nudge amount in mm

        unsigned int threads {1}; //< Number of threads to use when slicing

        const wxString version { wxString(SLIC3R_VERSION) };

        void save_settings();
        void load_settings();
        
        /// Storage for window positions
        std::map<wxString, std::tuple<wxPoint, wxSize, bool> > window_pos { std::map<wxString, std::tuple<wxPoint, wxSize, bool> >() };

    private:
        const std::string LogChannel {"GUI_Settings"}; //< Which log these messages should go to.
        
};

}} //namespace Slic3r::GUI

#endif // SETTINGS_HPP
