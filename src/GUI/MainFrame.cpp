#include <wx/accel.h>
#include <wx/utils.h> 

#include "libslic3r.h"

#include "MainFrame.hpp"
#include "misc_ui.hpp"
#include "Dialogs/AboutDialog.hpp"

namespace Slic3r { namespace GUI {

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
        : wxFrame(NULL, wxID_ANY, title, pos, size), loaded(false),
        tabpanel(nullptr), controller(nullptr), plater(nullptr), preset_editor_tabs(std::map<preset_t, PresetEditor*>())
{
    this->SetIcon(wxIcon(var("Slic3r_128px.png"), wxBITMAP_TYPE_PNG));        

    this->init_tabpanel();
    this->init_menubar();

    wxToolTip::SetAutoPop(TOOLTIP_TIMER);

    // initialize status bar
    this->statusbar = new ProgressStatusBar(this, -1);
    wxString welcome_text {_("Version SLIC3R_VERSION_REPLACE - Remember to check for updates at https://slic3r.org/")};
    welcome_text.Replace("SLIC3R_VERSION_REPLACE", wxString(SLIC3R_VERSION));
    this->statusbar->SetStatusText(welcome_text);
    this->SetStatusBar(this->statusbar);

    this->loaded = 1;

    // Initialize layout
    {
        wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(this->tabpanel, 1, wxEXPAND);
        sizer->SetSizeHints(this);
        this->SetSizer(sizer);
        this->Fit();
        this->SetMinSize(wxSize(760, 490));
        this->SetSize(this->GetMinSize());
        wxTheApp->SetTopWindow(this);
        ui_settings->restore_window_pos(this, "main_frame");
        this->Show();
        this->Layout();
    }
    // Set up event handlers.
    this->Bind(wxEVT_CLOSE_WINDOW, [=](wxCloseEvent& e) {
        if (e.CanVeto()) {
            if (!this->plater->prompt_unsaved_changes()) {
                e.Veto();
                return;
            }
            /*
            if ($self->{controller} && $self->{controller}->printing) {
                my $confirm = Wx::MessageDialog->new($self, "You are currently printing. Do you want to stop printing and continue anyway?",
                    'Unfinished Print', wxICON_QUESTION | wxYES_NO | wxNO_DEFAULT);
                if ($confirm->ShowModal == wxID_NO) {
                    $event->Veto;
                    return;
                }
            }

            */
            // save window size
            ui_settings->save_window_pos(this, "main_frame");

            // Propagate event
            e.Skip();
        }
    });
}

/// Private initialization function for the main frame tab panel.
void MainFrame::init_tabpanel()
{
    this->tabpanel = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP);
    auto panel = this->tabpanel; 

    panel->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, ([=](wxNotebookEvent& e) 
    { 
        auto tabpanel = this->tabpanel;
        // TODO: trigger processing for activation event
        if (tabpanel->GetSelection() > 1) {
            tabpanel->SetWindowStyle(tabpanel->GetWindowStyleFlag());
        } 
    }), panel->GetId());

    this->plater = new Slic3r::GUI::Plater(panel, _("Plater"));
    this->controller = new Slic3r::GUI::Controller(panel, _("Controller"));

    panel->AddPage(this->plater, this->plater->GetName());
    if (ui_settings->show_host) panel->AddPage(this->controller, this->controller->GetName());
    if (ui_settings->preset_editor_tabs) {
        this->plater->show_preset_editor(preset_t::Print,0);
        this->plater->show_preset_editor(preset_t::Material,0);
        this->plater->show_preset_editor(preset_t::Printer,0);
    }
    
}

void MainFrame::init_menubar()
{

    wxMenu* menuFile = new wxMenu();
    {
        append_menu_item(menuFile, _(L"Open STL/OBJ/AMF/3MF…"), _("Open a model"), [=](wxCommandEvent& e) { if (this->plater != nullptr) this->plater->add();}, wxID_ANY, "brick_add.png", "Ctrl+O");
    }
    
    wxMenu* menuPlater = this->plater_menu = new wxMenu();
    {
        wxMenu* selectMenu = this->plater_select_menu = new wxMenu();
        append_submenu(menuPlater, _("Select"), _("Select an object in the plater"), selectMenu, wxID_ANY, "brick.png"); 
        append_menu_item(menuPlater, _("Undo"), _("Undo"), [this](wxCommandEvent& e) { this->plater->undo(); }, wxID_ANY, "arrow_undo.png", "Ctrl+Z");
        append_menu_item(menuPlater, _("Redo"), _("Redo"), [this](wxCommandEvent& e) { this->plater->redo(); }, wxID_ANY, "arrow_redo.png", "Ctrl+Shift+Z");
        append_menu_item(menuPlater, _("Select Next Object"), _("Select Next Object in the plater"), 
                [this](wxCommandEvent& e) { this->plater->select_next(); }, wxID_ANY, "arrow_right.png", "Ctrl+Right");
        append_menu_item(menuPlater, _("Select Prev Object"), _("Select Previous Object in the plater"), 
                [this](wxCommandEvent& e) { this->plater->select_prev(); }, wxID_ANY, "arrow_left.png", "Ctrl+Left");
        append_menu_item(menuPlater, _("Zoom In"), _("Zoom In"), 
                [this](wxCommandEvent& e) { this->plater->zoom(Zoom::In); }, wxID_ANY, "zoom_in.png", "Ctrl+Up");
        append_menu_item(menuPlater, _("Zoom Out"), _("Zoom Out"), 
                [this](wxCommandEvent& e) { this->plater->zoom(Zoom::In); }, wxID_ANY, "zoom_out.png", "Ctrl+Down");
        menuPlater->AppendSeparator();
        append_menu_item(menuPlater, _("Export G-code..."), _("Export current plate as G-code"), 
                [this](wxCommandEvent& e) { this->plater->export_gcode(); }, wxID_ANY, "cog_go.png");
        append_menu_item(menuPlater, _("Export plate as STL..."), _("Export current plate as STL"), 
                [this](wxCommandEvent& e) { this->plater->export_stl(); }, wxID_ANY, "brick_go.png");
        append_menu_item(menuPlater, _("Export plate with modifiers as AMF..."), _("Export current plate as AMF, including all modifier meshes"), 
                [this](wxCommandEvent& e) { this->plater->export_amf(); }, wxID_ANY, "brick_go.png");
        append_menu_item(menuPlater, _("Export plate with modifiers as 3MF..."), _("Export current plate as 3MF, including all modifier meshes"), 
                [this](wxCommandEvent& e) { this->plater->export_tmf(); }, wxID_ANY, "brick_go.png");



    }
    wxMenu* menuObject = this->plater->object_menu();
    this->on_plater_object_list_changed(false);
    this->on_plater_selection_changed(false);

    wxMenu* menuSettings = new wxMenu();
    {
    }
    wxMenu* menuView = new wxMenu();
    {
    }
    wxMenu* menuWindow = new wxMenu();
    {
    }
    wxMenu* menuHelp = new wxMenu();
    {
        // TODO: Reimplement config wizard
        //menuHelp->AppendSeparator();
        append_menu_item(menuHelp, _("Slic3r &Website"), _("Open the Slic3r website in your browser"), [=](wxCommandEvent& e) 
        {
            wxLaunchDefaultBrowser("http://www.slic3r.org");
        });
        append_menu_item(menuHelp, _("Check for &Updates..."), _("Check for new Slic3r versions"), [=](wxCommandEvent& e)
        {
            check_version(true);
        });
        append_menu_item(menuHelp, _("Slic3r &Manual"), _("Open the Slic3r manual in your browser"), [=](wxCommandEvent& e) 
        {
            wxLaunchDefaultBrowser("http://manual.slic3r.org/");
        });
        append_menu_item(menuHelp, _("&About Slic3r"), _("Show about dialog"), [=](wxCommandEvent& e) 
        {
            auto about = new AboutDialog(nullptr);
            about->ShowModal();
            about->Destroy();
        }, wxID_ABOUT);
        
    }

    wxMenuBar* menubar = new wxMenuBar();
    menubar->Append(menuFile, _("&File"));
    menubar->Append(menuPlater, _("&Plater"));
    menubar->Append(menuObject, _("&Object"));
    menubar->Append(menuSettings, _("&Settings"));
    menubar->Append(menuView, _("&View"));
    menubar->Append(menuWindow, _("&Window"));
    menubar->Append(menuHelp, _("&Help"));

    this->SetMenuBar(menubar);

}

}} // Namespace Slic3r::GUI
