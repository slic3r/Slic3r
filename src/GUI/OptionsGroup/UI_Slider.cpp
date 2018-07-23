#include "OptionsGroup/Field.hpp"
#include "misc_ui.hpp"

namespace Slic3r { namespace GUI {

UI_Slider::UI_Slider(wxWindow* parent, Slic3r::ConfigOptionDef _opt) : UI_Sizer(parent, _opt), _scale(10) {
}
UI_Slider::UI_Slider(wxWindow* parent, Slic3r::ConfigOptionDef _opt, size_t scale ) : UI_Sizer(parent, _opt), _scale(scale) {

}
UI_Slider::~UI_Slider() { _slider->Destroy(); _textctrl->Destroy(); }

void UI_Slider::set_value(boost::any value) {
}

double UI_Slider::get_double() {
    return 0.0;
}

int UI_Slider::get_int() {
    return 0;
}

std::string UI_Slider::get_string() {
    return std::string();
}

void UI_Slider::set_scale(size_t new_scale) {
}
} } // Namespace Slic3r::GUI
