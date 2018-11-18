#include "Preset.hpp"
#include "Config.hpp"
#include <regex>
#include <algorithm>

#include "Dialogs/PresetEditor.hpp"

using namespace std::literals::string_literals;
using namespace boost;

namespace Slic3r { namespace GUI {

Preset::Preset(bool is_default, wxString name, preset_t p) : group(p), name(name), external(false), default_preset(is_default) {
    t_config_option_keys keylist;
    switch (this->group) {
        case preset_t::Print:
            keylist = PrintEditor::options();
            break;
        case preset_t::Material:
            keylist = MaterialEditor::options();
            break;
        case preset_t::Printer:
            keylist = PrinterEditor::options();
            break;
        default: break;
    }
    this->_dirty_config = Slic3r::Config::new_from_defaults(keylist);
    this->_config = Slic3r::Config::new_from_defaults(keylist);
}

Preset::Preset(std::string load_dir, std::string filename, preset_t p) : group(p), _file(wxFileName(load_dir, filename)) {
    this->name = this->_file.GetName();
    this->_dirty_config = Slic3r::Config::new_from_ini(_file.GetFullPath().ToStdString());
    this->_config = Slic3r::Config::new_from_ini(_file.GetFullPath().ToStdString());

}

t_config_option_keys Preset::dirty_options() const {
    t_config_option_keys dirty;

    auto diff_config = this->_config->diff(this->_dirty_config);
    std::move(diff_config.begin(), diff_config.end(), std::back_inserter(dirty));

    auto extra = this->_group_overrides();
    std::copy_if(extra.cbegin(), extra.cend(), std::back_inserter(dirty), 
            [this](const std::string x) { return !this->_config->has(x) && this->_dirty_config->has(x);});

    dirty.erase(std::remove_if(dirty.begin(), dirty.end(), 
            [this](const std::string x) { return this->_config->has(x) && !this->_dirty_config->has(x);}),
            dirty.end());

    return dirty;
}

bool Preset::dirty() const { 
    return this->dirty_options().size() > 0;
}

Slic3r::Config Preset::dirty_config() {
    if (!this->loaded()) load_config();
    Slic3r::Config result { Slic3r::Config(*(this->_dirty_config)) };
    return result;
}

config_ref Preset::config() {
    std::weak_ptr<Slic3r::Config> result { this->_dirty_config };
    return result;
}

config_ptr Preset::load_config() {
    if (this->loaded()) return this->_dirty_config;

    t_config_option_keys keys { this->_group_keys() };
    t_config_option_keys extra_keys { this->_group_overrides() };

    if (this->default_preset) {
        this->_config = Slic3r::Config::new_from_defaults(keys);
    } else if (this->_file.HasName() > 0) {
        config_ptr config = Slic3r::Config::new_from_defaults(keys);
        if (this->file_exists()) {
            auto external_config { Slic3r::Config::new_from_ini(this->_file.GetFullPath().ToStdString()) };
            // Apply preset values on top of defaults
            config = Slic3r::Config::new_from_defaults(keys);
            config->apply_with_defaults(external_config, keys);

            // For extra_keys don't populate defaults.
            if (extra_keys.size() > 0 && !this->external) {
                config->apply(external_config, extra_keys);
            }

            this->_config = config;
        }
    }

    this->_dirty_config->apply(this->_config);
    return this->_dirty_config;
}

t_config_option_keys Preset::_group_keys() const {
    switch (this->group) {
        case preset_t::Print:
            return PrintEditor::options();
        case preset_t::Material:
            return MaterialEditor::options();
        case preset_t::Printer:
            return PrinterEditor::options();
        default:
            return t_config_option_keys();
    }
}
t_config_option_keys Preset::_group_overrides() const {
    switch (this->group) {
        case preset_t::Print:
            return PrintEditor::overriding_options();
        case preset_t::Material:
            return MaterialEditor::overriding_options();
        case preset_t::Printer:
            return PrinterEditor::overriding_options();
        default:
            return t_config_option_keys();
    }
}

bool Preset::compatible(const std::string& printer_name) const {
    if (!this->_dirty_config->has("compatible_printers") || this->default_preset || this->group == preset_t::Printer) {
        return true;
    }
    auto compatible_list {this->_dirty_config->get<ConfigOptionStrings>("compatible_printers").values};
    if (compatible_list.size() == 0) return true;
    return std::any_of(compatible_list.cbegin(), compatible_list.cend(), [printer_name] (const std::string& x) -> bool { return x.compare(printer_name) == 0; });
}

const std::string preset_name(preset_t group) {
    switch(group) {
        case preset_t::Print:
            return "Print"s;
        case preset_t::Printer:
            return "Printer"s;
        case preset_t::Material:
            return "Material"s;
        default:
            return "N/A"s;
    }
}


}} // namespace Slic3r::GUI
