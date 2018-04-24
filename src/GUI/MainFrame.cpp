#include "MainFrame.hpp"
#include "Plater.hpp"
#include "Controller.hpp"

namespace Slic3r { namespace GUI {

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
        : MainFrame(title, pos, size, nullptr) {}
MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size, std::shared_ptr<Settings> config)
        : wxFrame(NULL, wxID_ANY, title, pos, size), loaded(false),
        tabpanel(nullptr), controller(nullptr), plater(nullptr), gui_config(config)
{
    // Set icon to either the .ico if windows or png for everything else.

    this->init_tabpanel();
    this->init_menubar();

    wxToolTip::SetAutoPop(TOOLTIP_TIMER);

    // STUB: Initialize status bar with text.
    /*    # initialize status bar
    $self->{statusbar} = Slic3r::GUI::ProgressStatusBar->new($self, -1);
    $self->{statusbar}->SetStatusText("Version $Slic3r::VERSION - Remember to check for updates at http://slic3r.org/");
    $self->SetStatusBar($self->{statusbar}); */

    this->loaded = 1;

    // Initialize layout
    {
        wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(this->tabpanel, 1, wxEXPAND);
        sizer->SetSizeHints(this);
        this->Fit();
        this->SetMinSize(wxSize(760, 490));
        this->SetSize(this->GetMinSize());
        wxTheApp->SetTopWindow(this);
        this->Show();
        this->Layout();
    }

}

/// Private initialization function for the main frame tab panel.
void MainFrame::init_tabpanel()
{
    this->tabpanel = new wxAuiNotebook(this, -1, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TOP);
    auto panel = this->tabpanel; 

    panel->Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, ([=](wxAuiNotebookEvent& e) 
    { 
        auto panel = this->tabpanel->GetPage(this->tabpanel->GetSelection());
        auto tabpanel = this->tabpanel;
        // todo: trigger processing for activation(?)
        if (tabpanel->GetSelection() > 1) {
            tabpanel->SetWindowStyle(tabpanel->GetWindowStyleFlag() | wxAUI_NB_CLOSE_ON_ACTIVE_TAB);
        } else if (this->gui_config->show_host == false && tabpanel->GetSelection() == 1) {
            tabpanel->SetWindowStyle(tabpanel->GetWindowStyleFlag() | wxAUI_NB_CLOSE_ON_ACTIVE_TAB);
        } else {
            tabpanel->SetWindowStyle(tabpanel->GetWindowStyleFlag() | ~wxAUI_NB_CLOSE_ON_ACTIVE_TAB);
        }
    }), panel->GetId());

    this->plater = new Slic3r::GUI::Plater(panel, _("Plater"));
    this->controller = new Slic3r::GUI::Controller(panel, _("Controller"));
    
/* 
sub _init_tabpanel {
    my ($self) = @_;
    
    $self->{tabpanel} = my $panel = Wx::AuiNotebook->new($self, -1, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TOP);
    EVT_AUINOTEBOOK_PAGE_CHANGED($self, $self->{tabpanel}, sub {
        my $panel = $self->{tabpanel}->GetPage($self->{tabpanel}->GetSelection);
        $panel->OnActivate if $panel->can('OnActivate');
        if ($self->{tabpanel}->GetSelection > 1) {
            $self->{tabpanel}->SetWindowStyle($self->{tabpanel}->GetWindowStyleFlag | wxAUI_NB_CLOSE_ON_ACTIVE_TAB);
        } elsif(($Slic3r::GUI::Settings->{_}{show_host} == 0) && ($self->{tabpanel}->GetSelection == 1)){
            $self->{tabpanel}->SetWindowStyle($self->{tabpanel}->GetWindowStyleFlag | wxAUI_NB_CLOSE_ON_ACTIVE_TAB);
        } else {
            $self->{tabpanel}->SetWindowStyle($self->{tabpanel}->GetWindowStyleFlag & ~wxAUI_NB_CLOSE_ON_ACTIVE_TAB);
        }
    });
    EVT_AUINOTEBOOK_PAGE_CLOSE($self, $self->{tabpanel}, sub {
        my $panel = $self->{tabpanel}->GetPage($self->{tabpanel}->GetSelection);
        if ($panel->isa('Slic3r::GUI::PresetEditor')) {
            delete $self->{preset_editor_tabs}{$panel->name};
        }
        wxTheApp->CallAfter(sub {
            $self->{tabpanel}->SetSelection(0);
        });
    });
    
    $panel->AddPage($self->{plater} = Slic3r::GUI::Plater->new($panel), "Plater");
    $panel->AddPage($self->{controller} = Slic3r::GUI::Controller->new($panel), "Controller")
        if ($Slic3r::GUI::Settings->{_}{show_host});
*/

}

void MainFrame::init_menubar()
{
}

}} // Namespace Slic3r::GUI
