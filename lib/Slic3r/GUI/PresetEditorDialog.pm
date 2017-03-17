package Slic3r::GUI::PresetEditorDialog;
use strict;
use warnings;
use Wx qw(:dialog :id :misc :sizer :button :icon wxTheApp);
use Wx::Event qw(EVT_CLOSE);
use base qw(Wx::Dialog Class::Accessor);
use utf8;

__PACKAGE__->mk_accessors(qw(preset_editor));

sub new {
    my ($class, $parent) = @_;
    my $self = $class->SUPER::new($parent, -1, "Settings", wxDefaultPosition, [800,500],
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxDIALOG_EX_METAL);
    
    $self->preset_editor($self->preset_editor_class->new($self));
    $self->SetTitle($self->preset_editor->title);
    $self->preset_editor->load_presets;
    
    my $sizer = Wx::BoxSizer->new(wxVERTICAL);
    $sizer->Add($self->preset_editor, 1, wxEXPAND);
    
    $self->SetSizer($sizer);
    #$sizer->SetSizeHints($self);
    
    if (0) {
        # This does not call the EVT_CLOSE below
        my $buttons = $self->CreateStdDialogButtonSizer(wxCLOSE);
        $sizer->Add($buttons, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 10);
    }
    
    EVT_CLOSE($self, sub {
        my (undef, $event) = @_;
        
        if ($event->CanVeto && !$self->preset_editor->prompt_unsaved_changes) {
            $event->Veto;
            return;
        }
        
        # propagate event
        $event->Skip;
    });
    
    return $self;
}


package Slic3r::GUI::PresetEditorDialog::Printer;
use base qw(Slic3r::GUI::PresetEditorDialog);

sub preset_editor_class { "Slic3r::GUI::PresetEditor::Printer" }


package Slic3r::GUI::PresetEditorDialog::Filament;
use base qw(Slic3r::GUI::PresetEditorDialog);

sub preset_editor_class { "Slic3r::GUI::PresetEditor::Filament" }


package Slic3r::GUI::PresetEditorDialog::Print;
use base qw(Slic3r::GUI::PresetEditorDialog);

sub preset_editor_class { "Slic3r::GUI::PresetEditor::Print" }

1;
