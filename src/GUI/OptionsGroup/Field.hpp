#ifndef SLIC3R_FIELD_HPP
#define SLIC3R_FIELD_HPP

#include <functional>
#include <boost/any.hpp>
#include "ConfigBase.hpp"

namespace Slic3r { namespace GUI {

class UI_Checkbox {
public:
    UI_Checkbox(wxWindow* parent, Slic3r::ConfigOptionDef _opt, wxWindowID checkid = wxID_ANY) : parent(parent), opt(_opt) {
        ui_window = new wxCheckBox(parent, checkid, "");
        _check = dynamic_cast<wxCheckBox*>(ui_window);

        // Set some defaults.
        if (this->opt.readonly) { this->_check->Disable(); }
        if (this->opt.default_value != nullptr) { this->_check->SetValue(this->opt.default_value->getBool()); }
        
        // Set up event handlers
        _check->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& e) { this->_on_change(""); e.Skip(); });
        _check->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { if (this->on_kill_focus != nullptr) {this->on_kill_focus("");} e.Skip(); });
    }
    ~UI_Checkbox() { wxDELETE(_check); ui_window = _check = nullptr; }
    /// Function to call when the contents of this change.
    std::function<void (const std::string&, bool value)> on_change {nullptr};
    std::function<void (const std::string&)> on_kill_focus {nullptr};

    /// Don't trigger on_change when this is true.
    bool disable_change_event {false};

    /// Enables the underlying UI widget.
    void enable() { this->_check->Enable(); }
    
    /// Disables the underlying UI widget.
    void disable() { this->ui_window->Disable(); }

    /// Set the underlying widget to either enabled or disabled.
    void toggle(bool enable = true) { enable ? this->enable() : this->disable(); }

    wxCheckBox* check() { return _check; }

    /// Returns the value of the enclosed checkbox.
    /// Implements get_bool
    bool get_bool() { return _check->GetValue();}

    /// Casts the containing value to boolean and sets the built-in checkbox appropriately.
    /// implements set_value
    void set_value(boost::any value) { this->_check->SetValue(boost::any_cast<bool>(value)); }
private:
    wxWindow* parent {nullptr};
    wxWindow* ui_window {nullptr};
    wxCheckBox* _check {nullptr};
    const Slic3r::ConfigOptionDef opt; //< Reference to the UI-specific bits of this option

    void _on_change(std::string opt_id) {
        if (!this->disable_change_event && this->on_change != nullptr) {
            this->on_change(opt_id, this->get_bool());
        }
    }
};

} } // Namespace Slic3r::GUI

#endif // SLIC3R_FIELD_HPP
