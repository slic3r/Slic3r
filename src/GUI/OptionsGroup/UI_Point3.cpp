#include "OptionsGroup/Field.hpp"
#include "misc_ui.hpp"
#include "Log.hpp"

#include <regex>

namespace Slic3r { namespace GUI {

using namespace std::string_literals;

std::string UI_Point3::get_string() {
    std::string result {""};
    if (_ctrl_x->GetValue().ToStdString() == "") {
        result.append("0.0");
    } else {
        result.append(trim_zeroes(_ctrl_x->GetValue().ToStdString()));
    }
    result.append(";"s);
    if (_ctrl_y->GetValue().ToStdString() == "") {
        result.append("0.0");
    } else {
        result.append(trim_zeroes(_ctrl_y->GetValue().ToStdString()));
    }
    result.append(";"s);
    if (_ctrl_z->GetValue().ToStdString() == "") {
        result.append("0.0");
    } else {
        result.append(trim_zeroes(_ctrl_z->GetValue().ToStdString()));
    }
    return result;
}

Slic3r::Pointf3 UI_Point3::get_point3() {
    return Pointf3(
        this->_ctrl_x->GetValue().ToStdString() != ""s ? std::stod(this->_ctrl_x->GetValue().ToStdString()) : 0.0, 
        this->_ctrl_y->GetValue().ToStdString() != ""s ? std::stod(this->_ctrl_y->GetValue().ToStdString()) : 0.0, 
        this->_ctrl_z->GetValue().ToStdString() != ""s ? std::stod(this->_ctrl_z->GetValue().ToStdString()) : 0.0
    );
}
Slic3r::Pointf UI_Point3::get_point() {
    return Pointf(
        this->_ctrl_x->GetValue().ToStdString() != ""s ? std::stod(this->_ctrl_x->GetValue().ToStdString()) : 0.0, 
        this->_ctrl_y->GetValue().ToStdString() != ""s ? std::stod(this->_ctrl_y->GetValue().ToStdString()) : 0.0
    );
}
void UI_Point3::set_value(boost::any value) {
    // type detection and handing off to children
    if (value.type() == typeid(Slic3r::Pointf)) {
        this->_set_value(boost::any_cast<Pointf>(value));
    } else if (value.type() == typeid(Slic3r::Pointf3)) {
        this->_set_value(boost::any_cast<Pointf3>(value));
    } else if (value.type() == typeid(std::string)) {
        this->_set_value(boost::any_cast<std::string>(value));
    } else if (value.type() == typeid(wxString)) {
        this->_set_value(boost::any_cast<wxString>(value).ToStdString());
    } else {
        Slic3r::Log::warn(this->LogChannel(), LOG_WSTRING("Type " << value.type().name() << " is not handled in set_value."));
    }
}

void UI_Point3::_set_value(Slic3r::Pointf value) {
    /// load the controls directly from the value
    this->_ctrl_x->SetValue(trim_zeroes(std::to_string(value.x)));
    this->_ctrl_y->SetValue(trim_zeroes(std::to_string(value.y)));
    this->_ctrl_z->SetValue("0.0");
}

void UI_Point3::_set_value(Slic3r::Pointf3 value) {
    /// load the controls directly from the value
    this->_ctrl_x->SetValue(trim_zeroes(std::to_string(value.x)));
    this->_ctrl_y->SetValue(trim_zeroes(std::to_string(value.y)));
    this->_ctrl_z->SetValue(trim_zeroes(std::to_string(value.z)));
}

void UI_Point3::_set_value(std::string value) {
    /// parse the string into the two parts.
    std::regex format_regex(";");
    auto f_begin { std::sregex_token_iterator(value.begin(), value.end(), format_regex, -1) };
    auto f_end { std::sregex_token_iterator() };

    if (f_begin != f_end) {
        auto iter = f_begin;
        this->_ctrl_x->SetValue(trim_zeroes(iter->str()));
        iter++;
        if (iter != f_end)
            this->_ctrl_y->SetValue(trim_zeroes(iter->str()));
        iter++;
        if (iter != f_end)
            this->_ctrl_z->SetValue(trim_zeroes(iter->str()));
    }


}

UI_Point3::UI_Point3(wxWindow* _parent, Slic3r::ConfigOptionDef _opt) : UI_Sizer(_parent, _opt) {
    Slic3r::Pointf3 def_val {_opt.default_value == nullptr ? Pointf3() : Pointf3(*(dynamic_cast<ConfigOptionPoint3*>(_opt.default_value))) };

    this->_ctrl_x = new wxTextCtrl(parent, wxID_ANY, trim_zeroes(wxString::FromDouble(def_val.x)), wxDefaultPosition, this->field_size, wxTE_PROCESS_ENTER);
    this->_ctrl_y = new wxTextCtrl(parent, wxID_ANY, trim_zeroes(wxString::FromDouble(def_val.y)), wxDefaultPosition, this->field_size, wxTE_PROCESS_ENTER);
    this->_ctrl_z = new wxTextCtrl(parent, wxID_ANY, trim_zeroes(wxString::FromDouble(def_val.z)), wxDefaultPosition, this->field_size, wxTE_PROCESS_ENTER);

    this->_lbl_x = new wxStaticText(parent, wxID_ANY, wxString("x:"));
    this->_lbl_y = new wxStaticText(parent, wxID_ANY, wxString("y:"));
    this->_lbl_z = new wxStaticText(parent, wxID_ANY, wxString("z:"));

    this->_sizer = new wxBoxSizer(wxHORIZONTAL);
    this->sizer = _sizer;

    this->_sizer->Add(_lbl_x, 0, wxALIGN_CENTER_VERTICAL, 0);
    this->_sizer->Add(_ctrl_x, 0, wxALIGN_CENTER_VERTICAL, 0);
    this->_sizer->Add(_lbl_y, 0, wxALIGN_CENTER_VERTICAL, 0);
    this->_sizer->Add(_ctrl_y, 0, wxALIGN_CENTER_VERTICAL, 0);
    this->_sizer->Add(_lbl_z, 0, wxALIGN_CENTER_VERTICAL, 0);
    this->_sizer->Add(_ctrl_z, 0, wxALIGN_CENTER_VERTICAL, 0);

    if (this->opt.tooltip != ""s) { 
        this->_ctrl_x->SetToolTip(this->opt.tooltip);
        this->_ctrl_y->SetToolTip(this->opt.tooltip);
        this->_ctrl_z->SetToolTip(this->opt.tooltip);
    }

    // events
    _ctrl_x->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent& e) { this->_on_change(""); e.Skip(); });
    _ctrl_y->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent& e) { this->_on_change(""); e.Skip(); });
    _ctrl_z->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent& e) { this->_on_change(""); e.Skip(); });
    _ctrl_x->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { if (this->on_kill_focus != nullptr) {this->on_kill_focus(""); this->_on_change("");} e.Skip(); });
    _ctrl_y->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { if (this->on_kill_focus != nullptr) {this->on_kill_focus(""); this->_on_change("");} e.Skip(); });
    _ctrl_z->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { if (this->on_kill_focus != nullptr) {this->on_kill_focus(""); this->_on_change("");} e.Skip(); });
}

} } // Namespace Slic3r::GUI
