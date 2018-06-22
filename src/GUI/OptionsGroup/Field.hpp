#ifndef SLIC3R_FIELD_HPP
#define SLIC3R_FIELD_HPP

#include <functional>

namespace Slic3r { namespace GUI {

class UI_Checkbox {
public:
    UI_Checkbox(wxWindow* parent, wxWindowID checkid = wxID_ANY) : parent(parent) {
        ui_window = new wxCheckBox(parent, checkid, "");
        _check = dynamic_cast<wxCheckBox*>(ui_window);
        _check->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& e) { this->_on_change(""); e.Skip(); });
    }
    ~UI_Checkbox() { wxDELETE(ui_window); ui_window = _check = nullptr; }
    /// Function to call when the contents of this change.
    std::function<void (const std::string&, bool value)> on_change {nullptr};
    std::function<void (const std::string&)> on_kill_focus {nullptr};

    /// Don't trigger on_change when this is true.
    bool disable_change_event {false};

    void enable() { this->ui_window->Enable(); }
    void disable() { this->ui_window->Disable(); }

    void toggle(bool enable = true) { enable ? this->enable() : this->disable(); }

    wxCheckBox* check() { return _check; }

    bool get_bool() { return _check->GetValue();}
private:
    wxWindow* parent {nullptr};
    wxWindow* ui_window {nullptr};
    wxCheckBox* _check {nullptr};

    void _on_change(std::string opt_id) {
        if (!this->disable_change_event) {
            this->on_change(opt_id, this->get_bool());
        }
    }
};

} } // Namespace Slic3r::GUI

#endif // SLIC3R_FIELD_HPP
