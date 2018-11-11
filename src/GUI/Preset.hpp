#ifndef PRESET_HPP
#define PRESET_HPP

// Libslic3r 
#include "PrintConfig.hpp"
#include "Config.hpp"

// wx
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
    #include <wx/string.h>
    #include <wx/filefn.h> 
#endif

namespace Slic3r { namespace GUI {


/// Preset types list. We assign numbers to permit static_casts and use as preset tab indices.
/// Don't skip numbers in the enumeration, we use this as an index into vectors (instead of using std::map).
enum class preset_t : uint8_t {
    Print = 0, Material, Printer,
    Last // This MUST be the last enumeration. Don't use it for anything.
};

/// Convenience counter to determine how many preset tabs exist.
constexpr uint8_t preset(preset_t preset) { return static_cast<uint8_t>(preset); }
constexpr size_t preset_types = preset(preset_t::Last);

class Preset; 

using Presets = std::vector<Preset>;

class PresetEditor;

class Preset {
public:
    friend class PresetEditor;
    preset_t group; 
    std::string name {""};
    bool external {false};

    /// Preset
    bool default_preset {false};

    /// Search the compatible_printers config option list for this preset name.
    /// Assume that Printer configs are compatible with other Printer configs
    bool compatible(std::string printer_name) { return true; }
    bool compatible(const Preset& other) {return (this->group == preset_t::Printer || (compatible(other.name) && other.group == preset_t::Printer));}

    /// Format the name appropriately.
    wxString dropdown_name() { return (this->dirty() ? wxString(this->name) << " " << _("(modified)") : wxString(this->name)); }

    bool file_exists() const { return wxFileExists(this->name); }

    bool prompt_unsaved_changes(wxWindow* parent);

    /// Apply dirty config to config and save.
    bool save(t_config_option_keys opt_keys);

    /// Apply dirty config to config and save with an alternate name.
    bool save_as(wxString name, t_config_option_keys opt_keys);

    /// Delete this preset from the system.
    void delete_preset();

    /// Returns list of options that have been modified from the config.
    t_config_option_keys dirty_options() const;

    /// Returns whether or not this config is different from its modified state.
    bool dirty() const;

    /// Loads the selected config from file and return a shared_ptr to the dirty config
    config_ptr load_config(); 

    /// Retrieve a copy of the loaded version of the configuration with any options applied.
    Slic3r::Config dirty_config(); 

    /// Pass-through to Slic3r::Config, returns whether or not a config was loaded.
    bool loaded() { return !this->_config->empty(); }

    /// Clear the dirty config.
    void dismiss_changes();

    void apply_dirty(const Slic3r::Config& other) { this->_dirty_config->apply(other); }
    void apply_dirty(const config_ptr& other) { this->apply_dirty(*other); }
    bool operator==(const wxString& _name) const { return this->operator==(_name.ToStdString()); }
    bool operator==(const std::string& _name) const { return _name.compare(this->name) == 0; }

    Preset(std::string load_dir, std::string filename, preset_t p);
private:

    /// store to keep config options for this preset
    /// This is intented to be a "pristine" copy from the underlying
    /// file store.
    config_ptr  _config { nullptr };

    /// Alternative config store for a modified configuration.
    /// This is the config reference that the rest of the system gets
    /// from load_config
    config_ptr _dirty_config { nullptr };

    /// Underlying filename for this preset config
    std::string file {""};

    /// dirname for the file.
    std::string dir  {""};

    /// All the options owned by the corresponding editor
    t_config_option_keys _group_keys() const;
    /// All the override options owned by the corresponding editor
    t_config_option_keys _group_overrides() const;


};

}} // namespace Slic3r::GUI

#endif // PRESET_HPP
