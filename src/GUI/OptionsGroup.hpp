#include "ConfigBase.hpp"
#include "Config.hpp"
#include "OptionsGroup/Field.hpp"
#include <string>
#include <map>
#include <memory>
// wx
#include <wx/statbox.h>

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
        if (_default != nullptr) delete _default; 
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

class OptionsGroup : public wxStaticBox {
public:
    OptionsGroup() : OptionsGroup(nullptr, wxString(""), nullptr) {};
    OptionsGroup(wxWindow* parent, const wxString& title, const std::function<config_ref(void)> _config_cb) : 
        wxStaticBox(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT, title),
        _parent(parent),
        _config_cb(_config_cb) {
            this->_sizer = new wxBoxSizer(wxVERTICAL);
            this->SetSizer(this->_sizer);
    }
    std::shared_ptr<UI_Field> append(const t_config_option_key& opt_id);
protected:
    wxWindow* _parent {nullptr};
    wxSizer* _sizer {nullptr};

    /// Callback function to get a current config reference from the owning PresetPage
    std::function<config_ref(void)> _config_cb {nullptr};
};

class ConfigOptionsGroup : public OptionsGroup {

};

}} // namespace Slic3r::GUI
