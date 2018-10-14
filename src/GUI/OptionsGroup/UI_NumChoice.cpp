#include "OptionsGroup/Field.hpp"
#include "misc_ui.hpp"
#include <typeinfo>

namespace Slic3r { namespace GUI {

void UI_NumChoice::set_value(boost::any value) {
    const bool show_value { std::regex_search(this->opt.gui_flags, show_value_flag) };

    if (value.type() == typeid(int)) {
        this->_set_value(boost::any_cast<int>(value), show_value); return;
    } else if (value.type() == typeid(double)) {
        this->_set_value(boost::any_cast<double>(value), show_value); return;
    } else if (value.type() == typeid(float)) {
        this->_set_value(boost::any_cast<float>(value), show_value); return;
    } else if (value.type() == typeid(std::string)) {
        this->_set_value(boost::any_cast<std::string>(value)); return;
    } else if (value.type() == typeid(wxString)) {
        this->_set_value(boost::any_cast<wxString>(value).ToStdString()); return;
    } else {
        Slic3r::Log::warn(this->LogChannel(), LOG_WSTRING(wxString("Unsupported type ") + value.type().name() + wxString(" for set_value") ));
    }
}

// Specialized set_value function for an input that is either a direct input or 
// an index to the label vector.
void UI_NumChoice::_set_value(int value, bool show_value) {
    auto& vlist {this->opt.enum_values}; // alias the vector to type less
    auto& llist {this->opt.enum_labels};
    if (show_value) {
        Slic3r::Log::info(this->LogChannel(), "Using show_value branch");
        this->_choice->ChangeValue(std::to_string(value)); 
    } else {
        if (vlist.size() > 0) {
            Slic3r::Log::info(this->LogChannel(), LOG_WSTRING(wxString("Searching values vector for ") + wxString(std::to_string(value))));
            // search the values vector for our input
            auto result {std::find(vlist.cbegin(), vlist.cend(), std::to_string(value))};
            if (result != vlist.cend()) {
                auto value_idx {std::distance(vlist.cbegin(), result)};
                Slic3r::Log::info(this->LogChannel(), LOG_WSTRING(wxString("Found. Setting selection to ") + wxString(std::to_string(value_idx))));
                this->_choice->SetSelection(value_idx);
                this->disable_change_event = false;
                return;
            }
        } else if (llist.size() > 0 && static_cast<size_t>(value) < llist.size()) {
            Slic3r::Log::info(this->LogChannel(), LOG_WSTRING(wxString("Setting label value to ") + wxString(llist.at(value))));
            /// if there isn't a value list but there is a label list, this is an index to that list.
            this->_choice->SetValue(wxString(llist.at(value)));
            this->disable_change_event = false;
            return;
        }
        this->_choice->SetValue(wxString(std::to_string(value)));
    }
    this->disable_change_event = false;
}

/// In this case, can't be a numeric index.
void UI_NumChoice::_set_value(double value, bool show_value) {
    if (show_value) {
        this->_choice->ChangeValue(std::to_string(value)); 
    }
}

/// In this case, can't be a numeric index.
void UI_NumChoice::_set_value(std::string value) {
    this->_choice->ChangeValue(value); 
}

std::string UI_NumChoice::get_string() { 
    if (opt.enum_values.size() > 0) {
        auto idx = this->_choice->GetSelection();
        Slic3r::Log::debug(this->LogChannel(), LOG_WSTRING(this->_choice->GetString(idx) + " <-- label"));
        Slic3r::Log::debug(this->LogChannel(), LOG_WSTRING(wxString("Selection for ") + this->_choice->GetValue() + wxString(": ") + wxString(std::to_string(idx))));
        if (idx != wxNOT_FOUND) return this->opt.enum_values.at(idx);
    }
    Slic3r::Log::debug(this->LogChannel(), "Returning label as value");
    return this->_choice->GetValue().ToStdString();
}

UI_NumChoice::UI_NumChoice(wxWindow* parent, Slic3r::ConfigOptionDef _opt, wxWindowID id) : UI_Window(parent, _opt) {
    int style {0};
    style |= wxTE_PROCESS_ENTER;
    if (opt.gui_type.size() > 0 && opt.gui_type.compare("select_open"s)) style |= wxCB_READONLY;

    /// Load the values
    auto values {wxArrayString()};
    if (opt.enum_labels.size() > 0) // if we have labels, use those instead.
        for (auto v : opt.enum_labels) values.Add(wxString(v));
    else 
        for (auto v : opt.enum_values) values.Add(wxString(v));


    _choice = new wxComboBox(parent, id, 
            (opt.default_value != nullptr ? opt.default_value->getString() : ""),
            wxDefaultPosition, _default_size(), values, style);
    window = _choice;

    this->set_value(opt.default_value != nullptr ? opt.default_value->getString() : "");


    // Event handler for data entry and changing the combobox
    auto pickup = [this](wxCommandEvent& e) 
    { 
        auto disable_change {this->disable_change_event};
        this->disable_change_event = true;

        auto idx {this->_choice->GetSelection()};
        wxString lbl {""};

        if (this->opt.enum_labels.size() > 0 && (unsigned)idx <= this->opt.enum_labels.size()) {
            lbl << this->opt.enum_labels.at(idx);
        } else if (this->opt.enum_values.size() > 0 && (unsigned)idx <= this->opt.enum_values.size()) {
            lbl << this->opt.enum_values.at(idx);
        } else {
            lbl << idx;
        }

        // avoid leaving field blank on wxMSW
        this->_choice->CallAfter([this,lbl]() { 
                auto dce {this->disable_change_event};
                this->disable_change_event = true;
                this->_choice->SetValue(lbl);
                this->disable_change_event = dce;
                });

        this->disable_change_event = disable_change;
        this->_on_change("");
    };
    _choice->Bind(wxEVT_COMBOBOX, pickup);
    _choice->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent& e) { this->set_value(this->_choice->GetValue()); this->_on_change(""); } );
    _choice->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { if (this->on_kill_focus != nullptr) {this->on_kill_focus(""); this->_on_change("");} e.Skip(); });
}


} } // Namespace Slic3r::GUI
