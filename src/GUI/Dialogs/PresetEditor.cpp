#include "Dialogs/PresetEditor.hpp"
#include "misc_ui.hpp"
#include "GUI.hpp"
#include <wx/bookctrl.h>


namespace Slic3r { namespace GUI {

PresetEditor::PresetEditor(wxWindow* parent, t_config_option_keys options) : 
    wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBK_LEFT | wxTAB_TRAVERSAL) {

    this->_sizer = new wxBoxSizer(wxHORIZONTAL);
    this->SetSizer(this->_sizer);

    wxSizer* left_sizer { new wxBoxSizer(wxVERTICAL) };

    {
        // choice menu
        this->_presets_choice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(left_col_width, -1));
        this->_presets_choice->SetFont(ui_settings->small_font());


        // buttons
        this->_btn_save_preset = new wxBitmapButton(this, wxID_ANY, wxBitmap(var("disk.png"), wxBITMAP_TYPE_PNG), wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        this->_btn_delete_preset = new wxBitmapButton(this, wxID_ANY, wxBitmap(var("delete.png"), wxBITMAP_TYPE_PNG), wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);

        this->set_tooltips();
        this->_btn_delete_preset->Disable();

        wxBoxSizer* hsizer {new wxBoxSizer(wxHORIZONTAL)};
        left_sizer->Add(hsizer, 0, wxEXPAND | wxBOTTOM, 5);
        hsizer->Add(this->_presets_choice, 1, wxRIGHT | wxALIGN_CENTER_VERTICAL, 3);
        hsizer->Add(this->_btn_save_preset, 0, wxALIGN_CENTER_VERTICAL);
        hsizer->Add(this->_btn_delete_preset, 0, wxALIGN_CENTER_VERTICAL);

    }

    // tree
    this->_treectrl = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(left_col_width, -1), wxTR_NO_BUTTONS | wxTR_HIDE_ROOT | wxTR_SINGLE | wxTR_NO_LINES | wxBORDER_SUNKEN | wxWANTS_CHARS);

    left_sizer->Add(this->_treectrl, 1, wxEXPAND);
    this->_icons = new wxImageList(16, 16, 1);
    this->_treectrl->AssignImageList(this->_icons);
    this->_iconcount = -1;

    this->_treectrl->AddRoot("root");
    this->_treectrl->SetIndent(0);
    this->disable_tree_sel_changed_event = false;

    /// bind a lambda for the event EVT_TREE_SEL_CHANGED 
   
    /// bind a lambda for the event EVT_KEY_DOWN 
    
    /// bind a lambda for the event EVT_CHOICE
    
    /// bind a lambda for the event EVT_KEY_DOWN 
    
    /// bind a lambda for the event EVT_BUTTON from btn_save_preset 
    
    /// bind a lambda for the event EVT_BUTTON from btn_delete_preset 
    
}

void PresetEditor::save_preset() {
}


/// TODO: Can this get deleted before the callback executes?
void PresetEditor::_on_value_change(std::string opt_key) {
    SLIC3RAPP->CallAfter(
    [this, opt_key]() {
        this->current_preset->apply_dirty(this->config);
        if (this->on_value_change != nullptr) this->on_value_change(opt_key);
        this->load_presets();
        this->_update(opt_key);
    } ); 
}

// TODO
void PresetEditor::_on_select_preset(bool force) {
}


void PresetEditor::select_preset(int id, bool force) {
    this->_presets_choice->SetSelection(id);
    this->_on_select_preset(force);
}

void PresetEditor::select_preset_by_name(const wxString& name, bool force) {
    const auto presets {SLIC3RAPP->presets.at(this->typeId())};
    int id = -1;
    auto result = std::find(presets.cbegin(), presets.cend(), name);
    if (result != presets.cend()) id = std::distance(presets.cbegin(), result);
    if (id == -1) {
        Slic3r::Log::warn(this->LogChannel(), LOG_WSTRING("No preset named" + name)); 
        return;
    }
    this->_presets_choice->SetSelection(id);
    this->_on_select_preset(force);
}

PresetPage* PresetEditor::add_options_page(const wxString& _title, const wxString& _icon) {
    
    if (_icon.size() > 0) {
        auto bitmap { wxBitmap(var(_icon), wxBITMAP_TYPE_PNG) };
        this->_icons->Add(bitmap);
        this->_iconcount += 1;
    }

    PresetPage* page {new PresetPage(this, _title, this->_iconcount)};
    page->Hide();
    this->_sizer->Add(page, 1, wxEXPAND | wxLEFT, 5); 
    _pages.push_back(page);
    return page;
}

// TODO
void PresetEditor::reload_config() {
}

// TODO
void PresetEditor::reload_preset() {
}

// TODO
void PresetEditor::_update_tree() {
}

// TODO
void PresetEditor::load_presets() {
}

}} // namespace Slic3r::GUI
