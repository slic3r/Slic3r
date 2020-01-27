#ifndef OPTIONSGROUP_HPP
#define OPTIONSGROUP_HPP

#include "ConfigBase.hpp"
#include "Config.hpp"
#include "OptionsGroup/Field.hpp"
#include "Dialogs/PresetEditor.hpp"
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

class OptionsGroup {
public:
    OptionsGroup() : OptionsGroup(nullptr, wxString(""), nullptr, field_storage_ref() ) {};
    OptionsGroup(wxWindow* parent, const wxString& title, const std::function<config_ref(void)> config_cb, 
    field_storage_ref _fields) :
        _parent(parent),
        _config_cb(config_cb),
        fields(_fields) {
            this->_box = new wxStaticBox(parent, wxID_ANY, title);
            this->_sizer = new wxStaticBoxSizer(this->_box, wxVERTICAL);
            this->_sizer->SetMinSize(wxSize(350, -1));
    }
    std::shared_ptr<UI_Field> append(const t_config_option_key& opt_id, wxSizer* above_sizer = nullptr);
    void append(const wxString& label, std::initializer_list<std::string> items);
    wxSizer* sizer() { return this->_sizer; }

    void reload_config();
protected:
    wxWindow* _parent {nullptr};
    wxStaticBoxSizer* _sizer {nullptr};
    wxStaticBox* _box { nullptr };

    /// Callback function to get a current config reference from the owning PresetPage
    std::function<config_ref(void)> _config_cb {nullptr};
    field_storage_ref fields;
    wxString LogChannel() { return "OptionsGroup"s; }
};

class ConfigOptionsGroup : public OptionsGroup {

};

}} // namespace Slic3r::GUI

#endif // OPTIONSGROUP_HPP
