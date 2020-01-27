#include "Dialogs/PresetEditor.hpp"
#include "OptionsGroup.hpp"

namespace Slic3r { namespace GUI {
OptionsGroup* PresetPage::add_optgroup(const wxString& title) {
    //this->_config_cb().lock();
    OptionsGroup* tmp { new OptionsGroup(this, title, this->_config_cb, this->fields) };
    this->vsizer->Add(tmp->sizer(), 0, wxEXPAND, 0);
    this->_groups.push_back(tmp);

    return tmp;
}
void PresetPage::reload_config() {
    for (auto* group : this->_groups) {
        group->reload_config();
    }
}
}} // Slic3r::GUI
