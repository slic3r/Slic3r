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
    #include <wx/filename.h> 
#endif

namespace Slic3r { namespace GUI {
using namespace std::literals::string_literals;

/// Preset types list. We assign numbers to permit static_casts and use as preset tab indices.
/// Don't skip numbers in the enumeration, we use this as an index into vectors (instead of using std::map).
enum class preset_t : uint8_t {
    Print = 0, Material, Printer,
    Last // This MUST be the last enumeration. Don't use it for anything.
};

/// Convenient constexpr to avoid a thousand static_casts
constexpr uint8_t get_preset(preset_t preset) { return static_cast<uint8_t>(preset); }
constexpr preset_t to_preset(uint8_t preset) { return static_cast<preset_t>(preset); }
/// Convenience counter to determine how many preset tabs exist.
constexpr size_t preset_types = get_preset(preset_t::Last);

/// Convenience/debug method to get a useful name from the enumeration.
const std::string preset_name(preset_t group);


class Preset; 

using Presets = std::vector<Preset>;
using preset_store = std::array<Presets, preset_types>;

class PresetEditor;

class Preset {
public:
    friend class PresetEditor;
    preset_t group; 
    wxString name {""};
    bool external {false};

    /// Preset
    bool default_preset {false};

    /// Search the compatible_printers config option list for this preset name.
    /// Assume that Printer configs are compatible with other Printer configs
    /// \param [in] Printer preset name to use to compare.
    bool compatible(const std::string& printer_name) const;
    bool compatible(const wxString& printer_name) const { return this->compatible(printer_name.ToStdString()); }
    bool compatible(const Preset& other) const {return (this->group == preset_t::Printer || (compatible(other.name) && other.group == preset_t::Printer));}

    /// Format the name appropriately.
    wxString dropdown_name() { return (this->dirty() ? this->name << " " << _("(modified)") : this->name); }

    bool file_exists() const { return this->_file.IsFileReadable(); }

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

    /// Retrieve a shared (cast through a weak) pointer. 
    config_ref config();

    /// Pass-through to Slic3r::Config, returns whether or not a config was loaded.
    bool loaded() { return !this->_config->empty(); }

    /// Clear the dirty config.
    void dismiss_changes();

    void apply_dirty(const Slic3r::Config& other) { this->_dirty_config->apply(other); }
    void apply_dirty(const config_ptr& other) { this->apply_dirty(*other); }
    bool operator==(const wxString& _name) const { return this->operator==(_name.ToStdString()); }
    bool operator==(const std::string& _name) const { return _name.compare(this->name) == 0; }

    /// Constructor for adding a file-backed 
    Preset(std::string load_dir, std::string filename, preset_t p);
    Preset(bool is_default, wxString name, preset_t p);
private:

    /// store to keep config options for this preset
    /// This is intended to be a "pristine" copy from the underlying
    /// file store.
    config_ptr  _config { nullptr };

    /// Alternative config store for a modified configuration.
    /// This is the config reference that the rest of the system gets
    /// from load_config
    config_ptr _dirty_config { nullptr };

    /// Underlying filename for this preset config
    wxFileName _file {wxFileName()};

    /// All the options owned by the corresponding editor
    t_config_option_keys _group_keys() const;
    /// All the override options owned by the corresponding editor
    t_config_option_keys _group_overrides() const;


};

}} // namespace Slic3r::GUI

#endif // PRESET_HPP
