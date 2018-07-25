#include "OptionsGroup/Field.hpp"
#include "misc_ui.hpp"
#include "utils.hpp"

namespace Slic3r { namespace GUI {

UI_Slider::UI_Slider(wxWindow* parent, Slic3r::ConfigOptionDef _opt, size_t scale) : UI_Sizer(parent, _opt), _scale(scale) {
    double default_value {0.0};
    if (_opt.default_value != nullptr) { default_value = _opt.default_value->getFloat(); }
    sizer = new wxBoxSizer(wxHORIZONTAL);
    _slider = new wxSlider(parent, wxID_ANY, 
                           (default_value < _opt.min ? _opt.min : default_value) * this->_scale,
                           (_opt.min > _opt.max || _opt.min == INT_MIN ? 0 : _opt.min) * this->_scale,
                           (_opt.max <= _opt.min || _opt.max == INT_MAX ? 100 : _opt.max) * this->_scale,
                           wxDefaultPosition,
                           wxSize(_opt.width, _opt.height));

    _textctrl = new wxTextCtrl(parent, wxID_ANY, 
                               trim_zeroes(std::to_string(static_cast<double>(_slider->GetValue()) / this->_scale)), 
                               wxDefaultPosition,
                               wxSize(50, -1),
                               wxTE_PROCESS_ENTER);


    sizer->Add(_slider, 1, wxALIGN_CENTER_VERTICAL, 0);
    sizer->Add(_textctrl, 0, wxALIGN_CENTER_VERTICAL, 0);

    _textctrl->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent& e) { this->_on_change(""); e.Skip(); });
    _textctrl->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { if (this->on_kill_focus != nullptr) {this->on_kill_focus(""); this->_on_change("");} e.Skip(); });

    _slider->Bind(wxEVT_SLIDER, [this](wxCommandEvent& e) { this->_on_change(""); e.Skip(); });



}
UI_Slider::~UI_Slider() { _slider->Destroy(); _textctrl->Destroy(); }

void UI_Slider::set_value(boost::any value) {
    this->disable_change_event = true;
    if (value.type() == typeid(int)) {
        this->_slider->SetValue(boost::any_cast<int>(value) * this->_scale);
    } else if (value.type() == typeid(double)) {
        this->_slider->SetValue(boost::any_cast<double>(value) * this->_scale);
    } else if (value.type() == typeid(float)) {
        this->_slider->SetValue(boost::any_cast<float>(value) * this->_scale);
    } else if (value.type() == typeid(std::string)) {
        std::string _val = boost::any_cast<std::string>(value);
        try {
            this->_slider->SetValue(std::stod(_val) * this->_scale);
        } catch (std::invalid_argument &e) {
            Slic3r::Log::error(this->LogChannel()) << "Conversion to numeric from string failed.\n";
        }
    } else if (value.type() == typeid(wxString)) {
        std::string _val = boost::any_cast<wxString>(value).ToStdString();
        try {
            this->_slider->SetValue(std::stod(_val) * this->_scale);
        } catch (std::invalid_argument &e) {
            Slic3r::Log::error(this->LogChannel()) << "Conversion to numeric from string failed.\n";
        }
    } else {
        Slic3r::Log::warn(this->LogChannel(), LOG_WSTRING("Type " << value.type().name() << " is not handled in set_value."));
    }
    this->_update_textctrl();
    this->disable_change_event = false;
}

double UI_Slider::get_double() {
    return static_cast<double>(this->_slider->GetValue()) / this->_scale;
}

int UI_Slider::get_int() {
    return static_cast<int>(this->_slider->GetValue()) / this->_scale;
}

std::string UI_Slider::get_string() {
    return _trim_zeroes(std::to_string(static_cast<double>(this->_slider->GetValue()) / this->_scale));
}

void UI_Slider::set_scale(size_t new_scale) {
    this->disable_change_event = true;
    auto current_value {this->get_double()};

    this->_slider->SetRange(
            this->_slider->GetMin() / this->_scale * new_scale, 
            this->_slider->GetMax() / this->_scale * new_scale);
    this->_scale = new_scale;
    this->set_value(current_value);

    this->disable_change_event = false;
}

void UI_Slider::_update_textctrl() {
    this->_textctrl->ChangeValue(this->get_string());
    this->_textctrl->SetInsertionPointEnd();
}

void UI_Slider::disable() {
    this->_slider->Disable();
    this->_textctrl->Disable();
    this->_textctrl->SetEditable(false);
}

void UI_Slider::enable() {
    this->_slider->Enable();
    this->_textctrl->Enable();
    this->_textctrl->SetEditable(true);
}

template <typename T>
void UI_Slider::set_range(T min, T max) { 
    this->_slider->SetRange(static_cast<int>(min * static_cast<int>(this->_scale)), static_cast<int>(max * static_cast<int>(this->_scale)));
}

} } // Namespace Slic3r::GUI
