#ifndef PRESET_CHOOSER_HPP
#define PRESET_CHOOSER_HPP
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
    #include <wx/panel.h>
    #include <wx/sizer.h>
    #include <wx/bmpcbox.h>
#endif

#include "misc_ui.hpp"
#include "Preset.hpp"
#include "GUI.hpp"

namespace Slic3r { namespace GUI {

using Choosers = std::vector<wxBitmapComboBox*>;
using chooser_name_list = std::vector<wxString>;
using chooser_name_map = std::array<chooser_name_list, preset_types>;

class PresetChooser : public wxPanel {
public:

        /// Build a panel to contain a sizer for dropdowns for preset selection.
    PresetChooser(wxWindow* parent, Print& print);
    PresetChooser(wxWindow* parent, Print& print, Settings& external);

    std::array<Choosers, preset_types> preset_choosers;


    /// Load the presets from the backing store and set up the choosers
    void load(std::array<Presets, preset_types> presets);
    
    /// Call load() with the app's own presets
    void load() { this->load(SLIC3RAPP->presets); }

    /// Const reference to internal name map (used for testing)
    const chooser_name_map& _chooser_names() const { return this->__chooser_names; }

private:
    wxSizer* local_sizer {};
    void _on_change_combobox(preset_t preset, wxBitmapComboBox* choice);
    chooser_name_map __chooser_names; 

    /// Reference to a Slic3r::Settings object.
    Settings& _settings;

    /// Reference to owning Plater's print
    Print& _print;

};

}} // Slic3r::GUI
#endif // PRESET_CHOOSER_HPP
