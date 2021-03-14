#include "ConfigBase.hpp"
#include "Config.hpp"
#include "OptionsGroup/Field.hpp"
#include <string>
#include <map>
#include <memory>

namespace Slic3r { namespace GUI {

class Option;
class Line;
class OptionsGroup;
class ConfigOptionsGroup;

/// Class to 
class Option {
public:
    /// TODO: Constructor needs to include a default value
    /// Reference to the configuration store. 
    ConfigOptionDef& desc;
    Option(const ConfigOptionType& _type, const ConfigOption& _def) : desc(this->_desc) {
        desc.type = _type;
        // set _default to whatever it needs to be based on _type
        
        
        // set desc default value to our stored one.
        desc.default_value = _default;
    }

    // move constructor, owns the Config item.
    Option(ConfigOptionDef&& remote) : desc(this->_desc), _desc(std::move(remote)) { }

    /// Destructor to take care of the owned default value.
    ~Option() {
        delete _default; 
        _default = nullptr;
    }

    
    /// TODO: Represent a sidebar widget
    void* side_widget {nullptr};

private:
    /// Actual configuration store. 
    /// Holds important information related to this configuration item.
    ConfigOptionDef _desc;

    /// default value, passed into _desc on construction.
    /// Owns the data.
    ConfigOption* _default {nullptr};
};

class OptionsGroup {
public:
    OptionsGroup() {}
    ConfigOption* get_field(const t_config_option_key& opt_id) { return nullptr; }
protected:
    std::map<t_config_option_key, std::unique_ptr<Option>> _options;
    std::map<t_config_option_key, std::unique_ptr<UI_Field>> _fields;
};

class ConfigOptionsGroup : public OptionsGroup {
public:

    std::shared_ptr<Slic3r::Config> config;
protected:

};

}} // namespace Slic3r::GUI
