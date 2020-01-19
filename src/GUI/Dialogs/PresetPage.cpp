#include "Dialogs/PresetEditor.hpp"
#include "OptionsGroup.hpp"

namespace Slic3r { namespace GUI {
    OptionsGroup* PresetPage::add_optgroup(const wxString& title) {
        OptionsGroup* tmp { new OptionsGroup(this, title, this->_config_cb, this->fields) };
        this->vsizer->Add(tmp->sizer(), 0, wxEXPAND, 0);
        this->_groups.push_back(tmp);

        return tmp;
    }
}} // Slic3r::GUI
