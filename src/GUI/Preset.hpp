#ifndef PRESET_HPP
#define PRESET_HPP

#include "PrintConfig.hpp"
#include "Config.hpp"

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

class Preset {
public:
    preset_t group; 
    std::string name {""};

    /// Preset
    bool default_preset {false};

    /// Search the compatible_printers config option list for this preset name.
    /// Assume that Printer configs are compatible with other Printer configs
    bool compatible(std::string printer_name) { return true; }
    bool compatible(const Preset& other) {return (this->group == preset_t::Printer || (compatible(other.name) && other.group == preset_t::Printer));}

    /// Format the name appropriately.
    wxString dropdown_name() { return (this->dirty() ? wxString(this->name) << " " << _("(modified)") : this->name); }

    bool file_exists(wxString name);

    bool prompt_unsaved_changes(wxWindow* parent);

    /// Apply dirty config to config and save.
    bool save(t_config_option_keys opt_keys);

    /// Apply dirty config to config and save with an alternate name.
    bool save_as(wxString name, t_config_option_keys opt_keys);

    /// Delete this preset from the system.
    void delete_preset();

    /// Returns list of options that have been modified from the config.
    t_config_option_keys dirty_options();

    /// Returns whether or not this config is different from its modified state.
    bool dirty();

    /// Loads the selected config from file and return a reference.
    Slic3r::Config& load_config(); 

    /// Pass-through to Slic3r::Config, returns whether or not a config was loaded.
    bool loaded() { return !this->config.empty(); }

    /// Clear the dirty config.
    void dismiss_changes();

    void apply_dirty(const Slic3r::Config& other) { this->dirty_config.apply(other); }
    void apply_dirty(const config_ptr& other) { this->apply_dirty(*other); }
    bool operator==(const wxString& _name) const { return this->operator==(_name.ToStdString()); }
    bool operator==(const std::string& _name) const { return _name.compare(this->name) == 0; }
private:
    bool external {false};

    /// store to keep config options for this preset
    Slic3r::Config config { Slic3r::Config() };

    /// Alternative config store for a modified configuration.
    Slic3r::Config dirty_config {Slic3r::Config()};

    std::string file {""};

    /// reach through to the appropriate material type
    t_config_option_keys _group_class(); 


};

}} // namespace Slic3r::GUI

#endif // PRESET_HPP
