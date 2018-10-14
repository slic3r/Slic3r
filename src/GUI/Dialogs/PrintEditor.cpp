#include "Dialogs/PresetEditor.hpp"
namespace Slic3r { namespace GUI {

PrintEditor::PrintEditor(wxWindow* parent, t_config_option_keys options) : 
    PresetEditor(parent, options) {
    
    this->config = Slic3r::Config::new_from_defaults(this->my_options());
    this->_update_tree();
    this->load_presets();
    this->_update();
    this->_build();
    }

void PrintEditor::_update(const std::string& opt_key) {
}


// TODO
void PrintEditor::_on_preset_loaded() {
}

void PrintEditor::_build() {
}
}} // namespace Slic3r::GUI
