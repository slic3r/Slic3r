#include "OptionsGroup/Field.hpp"
#include "misc_ui.hpp"

namespace Slic3r { namespace GUI {

UI_Color::UI_Color(wxWindow* parent, Slic3r::ConfigOptionDef _opt ) : UI_Window(parent, _opt) { 
    wxColour default_color(255,255,255,255);
    if (_opt.default_value != nullptr) {
        default_color = _string_to_color(_opt.default_value->getString());
    }
    this->_picker = new wxColourPickerCtrl(parent, wxID_ANY, default_color, wxDefaultPosition, this->_default_size());
    this->window = dynamic_cast<wxWindow*>(this->_picker);

    this->_picker->Bind(wxEVT_COLOURPICKER_CHANGED, [this](wxColourPickerEvent& e) { this->_on_change(""); e.Skip(); });

}

void UI_Color::set_value(boost::any value) {
    if (value.type() == typeid(wxColour)) {
        _picker->SetColour(boost::any_cast<wxColour>(value));
    } else if (value.type() == typeid(std::string)) {
        _picker->SetColour(wxString(boost::any_cast<std::string>(value)));
    } else if (value.type() == typeid(const char*)) {
        _picker->SetColour(wxString(boost::any_cast<const char*>(value)));
    } else if (value.type() == typeid(wxString)) {
        _picker->SetColour(boost::any_cast<wxString>(value));
    } else {
        Slic3r::Log::warn(this->LogChannel(), LOG_WSTRING("Type " << value.type().name() << " is not handled in set_value."));
    }
}

std::string UI_Color::get_string() {
    return _picker->GetColour().GetAsString(wxC2S_HTML_SYNTAX).ToStdString();
}

wxColour UI_Color::_string_to_color(const std::string& color) {
    // if invalid color string sent, use the default
    wxColour col(255,255,255,255);
    if (col.Set(wxString(color)))
        return col;
    return wxColor();
}

} } // Namespace Slic3r::GUI
