#include <wx/stattext.h>

#include "OptionsGroup.hpp"
#include "libslic3r.h"
#include "ConfigBase.hpp"
#include "PrintConfig.hpp"

#include "misc_ui.hpp"

namespace Slic3r { namespace GUI {

std::shared_ptr<UI_Field> OptionsGroup::append(const t_config_option_key& opt_id, wxSizer* above_sizer) {
    std::shared_ptr<UI_Field> a;
    wxSizer* used_sizer = (above_sizer == nullptr ? this->_sizer : above_sizer);
    const ConfigOptionDef& id = print_config_def.options.at(opt_id);
    auto field_map = this->fields.lock();

    auto* tmp_sizer { new wxBoxSizer(wxHORIZONTAL) };
    used_sizer->Add(tmp_sizer, 0, wxEXPAND, 5);
    auto* text {new wxStaticText(this->_parent, wxID_ANY, _(id.label) + wxString(":"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT) };
    text->SetFont(ui_settings->small_font());
    tmp_sizer->Add(text, 0, wxALIGN_CENTER_VERTICAL);

    if (id.gui_type.length() > 0) {
        // use the gui type
        if (id.gui_type == "slider") {
            a = std::shared_ptr<UI_Slider>(new UI_Slider(this->_parent, id));
            tmp_sizer->Add(a->get_sizer(), 0, wxALIGN_CENTER_VERTICAL);
        }
    } else {
        // use the type from the config option
        switch (id.type) {
            case coBool: 
                a = std::shared_ptr<UI_Checkbox>(new UI_Checkbox(this->_parent, id));
                tmp_sizer->Add(a->get_window(), 0, wxALIGN_CENTER_VERTICAL);
                           //a = std::shared_ptr<UI_Checkbox>(new UI_Checkbox(this, id)); break;
                          break;
            case coPoint:
                a = std::shared_ptr<UI_Point>(new UI_Point(this->_parent, id));
                tmp_sizer->Add(a->get_sizer(), 0, wxALIGN_CENTER_VERTICAL);
                break;
            case coPoint3: 
                a = std::shared_ptr<UI_Point3>(new UI_Point3(this->_parent, id));
                tmp_sizer->Add(a->get_sizer(), 0, wxALIGN_CENTER_VERTICAL);
                break;
            case coFloat:
            case coFloatOrPercent:
            case coString:
            case coPercent:
                {
                auto tmp { std::shared_ptr<UI_TextCtrl>(new UI_TextCtrl(this->_parent, id)) };
                tmp_sizer->Add(tmp->get_window(), 0, wxALIGN_CENTER_VERTICAL);
                tmp->on_change = [this] (t_config_option_key opt_id, std::string value)
                    {
                        std::cerr << "Locking config\n";
                        auto config { this->_config_cb().lock() };
                        std::cerr << "calling set on config with value " << value << "\n";
                        config->set(opt_id, value); };
                a = tmp;
                }
                break;
            default: break;
        }
    }
    this->_parent->Fit();
    field_map->insert(std::make_pair(opt_id, a));
    return a;
}

void OptionsGroup::append(const wxString& label, std::initializer_list<std::string> items) {
    auto* tmp_sizer { new wxBoxSizer(wxHORIZONTAL) };
    auto* text {new wxStaticText(this->_parent, wxID_ANY, label + wxString(":"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT) };
    tmp_sizer->Add(text, 0, wxALIGN_CENTER_VERTICAL);

    for (auto item : items) {
        this->append(item, tmp_sizer);
    }
    this->_sizer->Add(tmp_sizer);
}

}} // Slic3r::GUI
