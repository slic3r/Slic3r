package Slic3r::GUI::PresetEditorDialog;
use strict;
use warnings;
use Wx qw(:dialog :id :misc :sizer :button :icon wxTheApp WXK_ESCAPE);
use Wx::Event qw(EVT_CLOSE EVT_CHAR_HOOK);
use base qw(Wx::Dialog Class::Accessor);
use utf8;

__PACKAGE__->mk_accessors(qw(preset_editor));

sub new {
    my ($class, $parent) = @_;
    my $self = $class->SUPER::new($parent, -1, "Settings", wxDefaultPosition, [900,500],
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxDIALOG_EX_METAL);
    
    $self->preset_editor($self->preset_editor_class->new($self));
    $self->SetTitle($self->preset_editor->title);
    
    my $sizer = Wx::BoxSizer->new(wxVERTICAL);
    $sizer->Add($self->preset_editor, 1, wxEXPAND);
    
    $self->SetSizer($sizer);
    #$sizer->SetSizeHints($self);
    
    if (0) {
        my $buttons = $self->CreateStdDialogButtonSizer(wxCLOSE);
        $sizer->Add($buttons, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 10);
    }
    
    wxTheApp->restore_window_pos($self, "preset_editor");
    
    EVT_CLOSE($self, sub {
        my (undef, $event) = @_;
        
        # save window size
        wxTheApp->save_window_pos($self, "preset_editor");
        
        # propagate event
        $event->Skip;
    });
    
    EVT_CHAR_HOOK($self, sub {
        my (undef, $event) = @_;
        
        if ($event->GetKeyCode == WXK_ESCAPE) {
            $self->Close;
        } else {
            $event->Skip;
        }
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
