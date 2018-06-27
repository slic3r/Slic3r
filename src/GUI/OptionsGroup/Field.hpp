#ifndef SLIC3R_FIELD_HPP
#define SLIC3R_FIELD_HPP

#include <functional>
#include <string>
#include <limits>
#include <boost/any.hpp>
#include "ConfigBase.hpp"
#include "Log.hpp"

#include "wx/spinctrl.h"

namespace Slic3r { namespace GUI {

using namespace std::string_literals;

class UI_Window { 
public:
    UI_Window(wxWindow* _parent, Slic3r::ConfigOptionDef _opt) : parent(_parent), opt(_opt) {};
    virtual ~UI_Window() = default; 

    /// Don't trigger on_change when this is true.
    bool disable_change_event {false};

    /// Set the underlying control to the value (cast it and throw bad_any_cast if there are problems).
    virtual void set_value(boost::any value) = 0;

    /// Enables the underlying UI widget.
    void enable() { this->window->Enable(); }
    
    /// Disables the underlying UI widget.
    void disable() { this->window->Disable(); }

    /// Set the underlying widget to either enabled or disabled.
    void toggle(bool enable = true) { enable ? this->enable() : this->disable(); }

    /// Getter functions for UI_Window items.
    virtual bool get_bool() { Slic3r::Log::warn(this->LogChannel(), "get_bool does not exist"s); return false; } //< return false all the time if this is not implemented.
    virtual int get_int() { Slic3r::Log::warn(this->LogChannel(), "get_int does not exist"s); return 0; } //< return 0 all the time if this is not implemented.

    /// Function to call when the contents of this change.
    std::function<void (const std::string&, bool value)> on_change {nullptr};
    std::function<void (const std::string&)> on_kill_focus {nullptr};

protected:
    wxWindow* parent {nullptr};
    wxWindow* window {nullptr}; //< Pointer copy of the derived classes

    const Slic3r::ConfigOptionDef opt; //< Reference to the UI-specific bits of this option

    virtual std::string LogChannel() { return "UI_Window"s; }

    virtual void _on_change(std::string opt_id) = 0; 

    /// Define a default size for derived classes.
    wxSize _default_size() { return wxSize((opt.width >= 0 ? opt.width : 60), (opt.height != -1 ? opt.height : -1)); }
};

class UI_Checkbox : public UI_Window {
public:
    UI_Checkbox(wxWindow* parent, Slic3r::ConfigOptionDef _opt, wxWindowID checkid = wxID_ANY) : UI_Window(parent, _opt) {
        _check = new wxCheckBox(parent, checkid, "");
        this->window = _check;

        // Set some defaults.
        if (this->opt.readonly) { this->_check->Disable(); }
        if (this->opt.default_value != nullptr) { this->_check->SetValue(this->opt.default_value->getBool()); }
        
        // Set up event handlers
        _check->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& e) { this->_on_change(""); e.Skip(); });
        _check->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { if (this->on_kill_focus != nullptr) {this->on_kill_focus("");} e.Skip(); });
    }

    ~UI_Checkbox() = default;

    /// Returns a bare pointer to the underlying checkbox, usually for test interface
    wxCheckBox* check() { return _check; }

    /// Returns the value of the enclosed checkbox.
    /// Implements get_bool
    virtual bool get_bool() override { return _check->GetValue();}

    /// Casts the containing value to boolean and sets the built-in checkbox appropriately.
    /// implements set_value
    virtual void set_value(boost::any value) override { this->_check->SetValue(boost::any_cast<bool>(value)); }

protected:
    wxCheckBox* _check {nullptr};

    virtual std::string LogChannel() override { return "UI_Checkbox"s; }

    void _on_change(std::string opt_id) {
        if (!this->disable_change_event && this->window->IsEnabled() && this->on_change != nullptr) {
            this->on_change(opt_id, this->get_bool());
        }
    }

};

class UI_SpinCtrl : public UI_Window {
public:
    UI_SpinCtrl(wxWindow* parent, Slic3r::ConfigOptionDef _opt, wxWindowID spinid = wxID_ANY) : UI_Window(parent, _opt) {

        /// Initialize and set defaults, if available.
        _spin = new wxSpinCtrl(parent, spinid, "", wxDefaultPosition, _default_size(), 0,
            (opt.min > 0 ? opt.min : 0), 
            (opt.max > 0 ? opt.max : std::numeric_limits<int>::max()), 
            (opt.default_value != NULL ? opt.default_value->getInt() : 0));

        window = _spin;

        // Set up event handlers
        _spin->Bind(wxEVT_SPINCTRL, [this](wxCommandEvent& e) { this->_on_change(""); e.Skip(); });
        _spin->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { if (this->on_kill_focus != nullptr) {this->on_kill_focus("");} e.Skip(); });
    }
    ~UI_SpinCtrl() = default;
    int get_int() { return this->_spin->GetValue(); }
    void set_value(boost::any value) { this->_spin->SetValue(boost::any_cast<int>(value)); }

    /// Access method for the underlying SpinCtrl
    wxSpinCtrl* spinctrl() { return _spin; }

protected:
    virtual std::string LogChannel() { return "UI_SpinCtrl"s; }

    void _on_change(std::string opt_id) {
        if (!this->disable_change_event && this->window->IsEnabled() && this->on_change != nullptr) {
            this->on_change(opt_id, this->get_int());
        }
    }

private:
    wxSpinCtrl* _spin {nullptr};
    int _tmp {0};
};

} } // Namespace Slic3r::GUI

#endif // SLIC3R_FIELD_HPP
