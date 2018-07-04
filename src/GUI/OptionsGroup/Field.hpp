#ifndef SLIC3R_FIELD_HPP
#define SLIC3R_FIELD_HPP

#include <functional>
#include <string>
#include <limits>
#include <regex>

#include <boost/any.hpp>
#include "ConfigBase.hpp"
#include "Log.hpp"

#include "wx/spinctrl.h"
#include "wx/checkbox.h"
#include "wx/textctrl.h"
#include "wx/combobox.h"
#include "wx/arrstr.h"
#include "wx/stattext.h"
#include "wx/sizer.h"

namespace Slic3r { namespace GUI {

using namespace std::string_literals;

class UI_Field {
public:
    UI_Field(wxWindow* _parent, Slic3r::ConfigOptionDef _opt) : parent(_parent), opt(_opt) { };
    virtual ~UI_Field() = default; 

    /// Don't trigger on_change when this is true.
    bool disable_change_event {false};
    
    /// Set the underlying control to the value (cast it and throw bad_any_cast if there are problems).
    virtual void set_value(boost::any value) = 0;

    /// Enables the underlying UI widget.
    virtual void enable() { this->window->Enable(); }
    
    /// Disables the underlying UI widget.
    virtual void disable() { this->window->Disable(); }
    
    /// Set the underlying widget to either enabled or disabled.
    virtual void toggle(bool enable = true) { enable ? this->enable() : this->disable(); }

    /// Getter functions for UI_Window items.
    virtual bool get_bool() { Slic3r::Log::warn(this->LogChannel(), "get_bool does not exist"s); return false; } //< return false all the time if this is not implemented.
    virtual double get_double() { Slic3r::Log::warn(this->LogChannel(), "get_double does not exist"s); return 0.0; } //< return 0.0 all the time if this is not implemented.
    virtual int get_int() { Slic3r::Log::warn(this->LogChannel(), "get_int does not exist"s); return 0; } //< return 0 all the time if this is not implemented.
    virtual std::string get_string() { Slic3r::Log::warn(this->LogChannel(), "get_string does not exist"s); return 0; } //< return 0 all the time if this is not implemented.

    virtual Slic3r::Pointf get_point() { Slic3r::Log::warn(this->LogChannel(), "get_point does not exist"s); return Slic3r::Pointf(); } //< return 0 all the time if this is not implemented.

    /// Provide access in a generic fashion to the underlying Window.
    virtual wxWindow* get_window() { return this->window; }

    /// Provide access in a generic fashion to the underlying Sizer.
    virtual wxSizer* get_sizer() { return this->sizer; }

    /// Function to call when focus leaves.
    std::function<void (const std::string&)> on_kill_focus {nullptr};

protected:
    wxWindow* parent {nullptr}; //< Cached copy of the parent object
    wxWindow* window {nullptr}; //< Pointer copy of the derived classes
    wxSizer* sizer {nullptr}; //< Pointer copy of the derived classes

    const Slic3r::ConfigOptionDef opt; //< Reference to the UI-specific bits of this option

    virtual std::string LogChannel() { return "UI_Field"s; }
    
    virtual void _on_change(std::string opt_id) = 0; 

    /// Define a default size for derived classes.
    wxSize _default_size() { return wxSize((opt.width >= 0 ? opt.width : 60), (opt.height != -1 ? opt.height : -1)); }
};


/// Organizing class for single-part UI elements.
class UI_Window : public UI_Field { 
public:
    UI_Window(wxWindow* _parent, Slic3r::ConfigOptionDef _opt) : UI_Field(_parent, _opt) {};
    virtual ~UI_Window() = default; 
    virtual std::string LogChannel() override { return "UI_Window"s; }
};

/// Organizing class for multi-part UI elements.
class UI_Sizer : public UI_Field {
public:
    UI_Sizer(wxWindow* _parent, Slic3r::ConfigOptionDef _opt) : UI_Field(_parent, _opt) {};
    virtual ~UI_Sizer() = default; 
    virtual std::string LogChannel() override { return "UI_Sizer"s; }
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

    ~UI_Checkbox() { _check->Destroy(); }

    /// Returns a bare pointer to the underlying checkbox, usually for test interface
    wxCheckBox* check() { return _check; }

    /// Returns the value of the enclosed checkbox.
    /// Implements get_bool
    virtual bool get_bool() override { return _check->GetValue();}

    /// Casts the containing value to boolean and sets the built-in checkbox appropriately.
    /// implements set_value
    virtual void set_value(boost::any value) override { this->_check->SetValue(boost::any_cast<bool>(value)); }

    /// Function to call when the contents of this change.
    std::function<void (const std::string&, bool value)> on_change {nullptr};

protected:
    virtual std::string LogChannel() override { return "UI_Checkbox"s; }

    void _on_change(std::string opt_id) {
        if (!this->disable_change_event && this->window->IsEnabled() && this->on_change != nullptr) {
            this->on_change(opt_id, this->get_bool());
        }
    }
private:
    wxCheckBox* _check {nullptr};

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
        _spin->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { if (this->on_kill_focus != nullptr) { this->_on_change(""); this->on_kill_focus("");} e.Skip(); });
    }
    ~UI_SpinCtrl() { _spin->Destroy();}
    int get_int() { return this->_spin->GetValue(); }
    void set_value(boost::any value) { this->_spin->SetValue(boost::any_cast<int>(value)); }

    /// Access method for the underlying SpinCtrl
    wxSpinCtrl* spinctrl() { return _spin; }
    
    /// Function to call when the contents of this change.
    std::function<void (const std::string&, int value)> on_change {nullptr};

protected:
    virtual std::string LogChannel() { return "UI_SpinCtrl"s; }

    void _on_change(std::string opt_id) {
        if (!this->disable_change_event && this->window->IsEnabled() && this->on_change != nullptr) {
            this->on_change(opt_id, this->get_int());
        }
    }

private:
    wxSpinCtrl* _spin {nullptr};
};

class UI_TextCtrl : public UI_Window {
public:
    UI_TextCtrl(wxWindow* parent, Slic3r::ConfigOptionDef _opt, wxWindowID id = wxID_ANY) : UI_Window(parent, _opt) {
        int style {0};
        if (opt.multiline) {
            style |= wxHSCROLL;
            style |= wxTE_MULTILINE;
        } else {
            style |= wxTE_PROCESS_ENTER;
        }
        /// Initialize and set defaults, if available.
        _text = new wxTextCtrl(parent, id, 
            (opt.default_value != NULL ? wxString(opt.default_value->getString()) : wxString()), 
            wxDefaultPosition, 
            _default_size(), 
            style);

        window = _text;

        // Set up event handlers
        if (!opt.multiline) {
            _text->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent& e) { this->_on_change(""); e.Skip(); });
        }
        _text->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { if (this->on_kill_focus != nullptr) {this->on_kill_focus(""); this->_on_change("");} e.Skip(); });
    }
    ~UI_TextCtrl() { _text->Destroy(); }
    std::string get_string() { return this->_text->GetValue().ToStdString(); }
    void set_value(boost::any value) { this->_text->SetValue(boost::any_cast<std::string>(value)); }

    /// Access method for the underlying TextCtrl
    wxTextCtrl* textctrl() { return _text; }

    /// Function to call when the contents of this change.
    std::function<void (const std::string&, std::string value)> on_change {nullptr};

protected:
    virtual std::string LogChannel() { return "UI_TextCtrl"s; }

    void _on_change(std::string opt_id) {
        if (!this->disable_change_event && this->window->IsEnabled() && this->on_change != nullptr) {
            this->on_change(opt_id, this->get_string());
        }
    }

private:
    wxTextCtrl* _text {nullptr};
};

class UI_Choice : public UI_Window {
public:
    UI_Choice(wxWindow* parent, Slic3r::ConfigOptionDef _opt, wxWindowID id = wxID_ANY);
    ~UI_Choice() { _choice->Destroy(); }

    std::string get_string() override;

    void set_value(boost::any value) override;

    /// Function to call when the contents of this change.
    std::function<void (const std::string&, std::string value)> on_change {nullptr};

    /// Returns a bare pointer to the underlying combobox, usually for test interface
    wxComboBox* choice() { return this->_choice; }
protected:
    virtual std::string LogChannel() override { return "UI_Choice"s; }

    void _on_change(std::string opt_id) {
        if (!this->disable_change_event && this->window->IsEnabled() && this->on_change != nullptr) {
            this->on_change(opt_id, this->get_string());
        }
    }
private:
    wxComboBox* _choice {nullptr};
};


class UI_NumChoice : public UI_Window {
public:
    UI_NumChoice(wxWindow* parent, Slic3r::ConfigOptionDef _opt, wxWindowID id = wxID_ANY);
    ~UI_NumChoice() { _choice->Destroy(); }

    /// Returns the underlying value for the selected value (defined by enum_values). If there are labels, this may not 
    /// match the underlying combobox->GetValue(). 
    std::string get_string() override;

    /// Returns the item in the value field of the associated option as an integer.
    int get_int() override { return std::stoi(this->get_string()); }

    /// Returns the item in the value field of the associated option as a double.
    double get_double() override { return std::stod(this->get_string()); }


    /// Returns a bare pointer to the underlying combobox, usually for test interface
    wxComboBox* choice() { return this->_choice; }

    /// For NumChoice, value can be the actual input value or an index into the value.
    void set_value(boost::any value) override;

    /// Function to call when the contents of this change.
    std::function<void (const std::string&, std::string value)> on_change {nullptr};

protected:
    virtual std::string LogChannel() override { return "UI_NumChoice"s; }

    void _set_value(int value, bool show_value = false);
    void _set_value(double value, bool show_value = false);
    void _set_value(std::string value);

    void _on_change(std::string opt_id) {
        if (!this->disable_change_event && this->window->IsEnabled() && this->on_change != nullptr) {
            this->on_change(opt_id, this->get_string());
        }
    }
private:
    wxComboBox* _choice {nullptr};
    std::regex show_value_flag {"\bshow_value\b"};
};

class UI_Point : public UI_Sizer {
public:

    UI_Point(wxWindow* _parent, Slic3r::ConfigOptionDef _opt);
    ~UI_Point() { _lbl_x->Destroy(); _lbl_y->Destroy(); _ctrl_x->Destroy(); _ctrl_y->Destroy(); }
    std::string get_string();

    void set_value(boost::any value) override; //< Implements set_value

    Pointf get_point() override; /// return a Slic3r::Pointf corresponding to the textctrl contents.

    /// Return the underlying sizer.
    wxSizer* get_sizer() { return _sizer; };

    /// Function to call when the contents of this change.
    std::function<void (const std::string&, std::tuple<std::string, std::string> value)> on_change {nullptr};


    void enable() override { _ctrl_x->Enable(); _ctrl_y->Enable(); }
    void disable() override { _ctrl_x->Disable(); _ctrl_y->Disable(); }

    /// Local-access items
    wxTextCtrl* ctrl_x() { return _ctrl_x;}
    wxTextCtrl* ctrl_y() { return _ctrl_y;}

    wxStaticText* lbl_x() { return _lbl_x;}
    wxStaticText* lbl_y() { return _lbl_y;}

protected:
    virtual std::string LogChannel() override { return "UI_Point"s; }

    void _on_change(std::string opt_id) override { 
        if (!this->disable_change_event && this->_ctrl_x->IsEnabled() && this->on_change != nullptr) {
            this->on_change(opt_id, std::make_pair<std::string, std::string>(_ctrl_x->GetValue().ToStdString(), _ctrl_y->GetValue().ToStdString()));
        }
    }

private:
    wxSize field_size {40, 1};
    wxStaticText* _lbl_x {nullptr};
    wxStaticText* _lbl_y {nullptr};

    wxTextCtrl* _ctrl_x {nullptr};
    wxTextCtrl* _ctrl_y {nullptr};

    wxBoxSizer* _sizer {nullptr};

    void _set_value(Slic3r::Pointf value);
    void _set_value(std::string value);

    /// Remove extra zeroes generated from std::to_string on doubles
    std::string trim_zeroes(std::string in) {
        std::string result {""};
        std::regex strip_zeroes("(0*)$");
        std::regex_replace (std::back_inserter(result), in.begin(), in.end(), strip_zeroes, "");
        if (result.back() == '.') result.append("0");
        return result;
    }
    wxString trim_zeroes(wxString in) { return wxString(trim_zeroes(in.ToStdString())); }



};

} } // Namespace Slic3r::GUI

#endif // SLIC3R_FIELD_HPP
