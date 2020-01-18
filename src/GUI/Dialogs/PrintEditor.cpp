#include "Dialogs/PresetEditor.hpp"
#include "OptionsGroup.hpp"
namespace Slic3r { namespace GUI {

PrintEditor::PrintEditor(wxWindow* parent, t_config_option_keys options) : 
    PresetEditor(parent, options) {
    
    this->config = Slic3r::Config::new_from_defaults(this->my_options());
    this->_build();
    this->_update_tree();
    this->load_presets();
    this->_update();

    this->_sizer->Fit(this);
    }

void PrintEditor::_update(const std::string& opt_key) {
}


// TODO
void PrintEditor::_on_preset_loaded() {
}

void PrintEditor::_build() {
    {
        auto page = this->add_options_page(_("Layers and perimeters"), "layers.png");
        {
            auto optgroup = page->add_optgroup(_("Layer height"));
            optgroup->append("layer_height");
            optgroup->append("first_layer_height");

            // note: candidate for a Line
            optgroup->append("adaptive_slicing");
            optgroup->append("adaptive_slicing_quality");
            optgroup->append("layer_height");
            //this->get_field<UI_Slider>("adaptive_slicing_quality")->set_scale(1);
            optgroup->append("match_horizontal_surfaces");
        }
        page->Show();
        this->_sizer->Layout();
    }
}
}} // namespace Slic3r::GUI
