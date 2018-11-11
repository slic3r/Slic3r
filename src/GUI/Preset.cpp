#include "Preset.hpp"
#include "Config.hpp"
#include <regex>
#include <algorithm>

#include "Dialogs/PresetEditor.hpp"

using namespace std::literals::string_literals;
using namespace boost;

namespace Slic3r { namespace GUI {

Preset::Preset(std::string load_dir, std::string filename, preset_t p) : group(p), _file(wxFileName(load_dir, filename)) {
    this->name = this->_file.GetName();

    this->_dirty_config = std::make_shared<Slic3r::Config>();
    this->_config = std::make_shared<Slic3r::Config>();
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
            if (extra_keys.size() > 0 && !this->external){
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

}} // namespace Slic3r::GUI
