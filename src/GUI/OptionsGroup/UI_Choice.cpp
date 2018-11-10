#include "OptionsGroup/Field.hpp"

namespace Slic3r { namespace GUI {

UI_Choice::UI_Choice(wxWindow* parent, Slic3r::ConfigOptionDef _opt, wxWindowID id) : UI_Window(parent, _opt) {
    int style {0};
    style |= wxTE_PROCESS_ENTER;

    /// Load the values
    auto values {wxArrayString()};
    for (auto v : opt.enum_values) values.Add(wxString(v));

    if (opt.gui_type.size() > 0 && opt.gui_type.compare("select_open"s)) {
        _choice = new wxChoice(parent, id, 
                wxDefaultPosition, _default_size(), values, style);
        _choice->Bind(wxEVT_CHOICE, [this](wxCommandEvent& e) { this->_on_change(""); e.Skip(); });
        window = _choice;
    } else {
        _combo = new wxComboBox(parent, id, 
                (opt.default_value != nullptr ? opt.default_value->getString() : ""),
                wxDefaultPosition, _default_size(), values, style);
        _combo->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent& e) { this->_on_change(""); e.Skip(); });
        window = _combo;
    }

    window->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent& e) { this->_on_change(""); e.Skip(); });
    window->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { if (this->on_kill_focus != nullptr) {this->on_kill_focus(""); this->_on_change("");} e.Skip(); });
}

std::string UI_Choice::get_string() { 
    if (_combo != nullptr) {
        if (opt.enum_values.size() > 0) {
            auto idx = this->_combo->GetSelection();
            if (idx != wxNOT_FOUND) return this->opt.enum_values.at(idx);
        }
        return this->_combo->GetValue().ToStdString();
    } else {
        if (opt.enum_values.size() > 0) {
            auto idx = this->_choice->GetSelection();
            if (idx != wxNOT_FOUND) return this->opt.enum_values.at(idx);
        }
        return std::string("");
    }
}


void UI_Choice::set_value(boost::any value) {
    auto result {std::find(opt.enum_values.cbegin(), opt.enum_values.cend(), boost::any_cast<std::string>(value))};
    if (_combo != nullptr) {
        if (result == opt.enum_values.cend()) {
            this->_combo->SetValue(wxString(boost::any_cast<std::string>(value)));
        } else {
            auto idx = std::distance(opt.enum_values.cbegin(), result);
            this->_combo->SetSelection(idx);
        }
    } else {
        if (result != opt.enum_values.cend()) {
            auto idx = std::distance(opt.enum_values.cbegin(), result);
            this->_choice->SetSelection(idx);
        }
    }
}

} } // Namespace Slic3r::GUI
