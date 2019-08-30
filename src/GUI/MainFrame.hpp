
#ifndef MAINFRAME_HPP
#define MAINFRAME_HPP
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/tooltip.h>
#include <wx/notebook.h>

#include <memory>
#include <map>


#include "Controller.hpp"
#include "Plater.hpp"
#include "Dialogs/PresetEditor.hpp"
#include "Settings.hpp"
#include "GUI.hpp"
#include "ProgressStatusBar.hpp"

namespace Slic3r { namespace GUI {

class Plater;

constexpr unsigned int TOOLTIP_TIMER = 32767;

class MainFrame: public wxFrame
{
public:
    MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
    ProgressStatusBar* statusbar {new ProgressStatusBar(this, -1)};

    bool has_plater_menu() { return this->plater_menu != nullptr; }
    wxMenu* plater_select_menu {nullptr};
    wxNotebook* tabs() { return tabpanel; }

    std::map<preset_t, PresetEditor*> preset_editor_tabs;
private:
    wxDECLARE_EVENT_TABLE();

    void init_menubar(); //< Routine to initialize all top-level menu items.
    void init_tabpanel(); //< Routine to initialize all of the tabs.

    bool loaded; //< Main frame itself has finished loading.
    // STUB: preset editor tabs storage
    // STUB: Statusbar reference

    wxNotebook* tabpanel;
    Controller* controller;
    Plater* plater;

    wxMenu* plater_menu {nullptr};


    void on_plater_object_list_changed(bool force) {};
    void on_plater_selection_changed(bool force) {};


};

}} // Namespace Slic3r::GUI
#endif // MAINFRAME_HPP
