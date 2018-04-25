
#ifndef MAINFRAME_HPP
#define MAINFRAME_HPP
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/aui/auibook.h>
#include <wx/tooltip.h>

#include <memory>
#include <map>


#include "Controller.hpp"
#include "Plater.hpp"
#include "PresetEditor.hpp"
#include "Settings.hpp"
#include "GUI.hpp"

namespace Slic3r { namespace GUI {

template <typename T>
void append_menu_item(wxMenu* menu, const wxString& name,const wxString& help, T lambda, int id = wxID_ANY, const wxBitmap& icon = wxBitmap(), const wxString& accel = "") {
    wxMenuItem* tmp = menu->Append(wxID_ANY, name, help);
    wxAcceleratorEntry* a = new wxAcceleratorEntry();
    if (a->FromString(accel))
        tmp->SetAccel(a); // set the accelerator if and only if the accelerator is fine
    tmp->SetHelp(help);

    menu->Bind(wxEVT_MENU, lambda, tmp->GetId(), tmp->GetId());
}

constexpr unsigned int TOOLTIP_TIMER = 32767;

class MainFrame: public wxFrame
{
public:
    MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
    MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size, std::shared_ptr<Settings> gui_config);
private:
    wxDECLARE_EVENT_TABLE();

    void init_menubar(); //< Routine to intialize all top-level menu items.
    void init_tabpanel(); //< Routine to initialize all of the tabs.

    bool loaded; //< Main frame itself has finished loading.
    // STUB: preset editor tabs storage
    // STUB: Statusbar reference

    wxAuiNotebook* tabpanel;
    Controller* controller;
    Plater* plater;


    std::shared_ptr<Settings> gui_config;
    std::map<wxWindowID, PresetEditor*> preset_editor_tabs;

};

}} // Namespace Slic3r::GUI
#endif // MAINFRAME_HPP
