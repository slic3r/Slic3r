# A tiny dialog to select how to reload an object that has additional parts or modifiers.

package Slic3r::GUI::ReloadDialog;
use strict;
use warnings;
use utf8;

use Wx qw(:button :dialog :id :misc :sizer :choicebook wxTAB_TRAVERSAL);
use Wx::Event qw(EVT_CLOSE);
use base 'Wx::Dialog';

sub new {
    my $class = shift;
    my ($parent,$default_selection,$default_preserve) = @_;
    my $self = $class->SUPER::new($parent, -1, "Reload options", wxDefaultPosition, [350,100], wxDEFAULT_DIALOG_STYLE);
    
    # label
    my $text_additional = Wx::StaticText->new($self, -1, "Handling of additional parts and modifiers:", wxDefaultPosition, wxDefaultSize);

    # selector
    $self->{choice} = my $choice = Wx::Choice->new($self, -1, wxDefaultPosition, wxDefaultSize, []);
    $choice->Append("Reload all linked files");
    $choice->Append("Reload main file, copy added parts & modifiers");
    $choice->Append("Reload main file, discard added parts & modifiers");
    $choice->SetSelection($default_selection);
    
    # label
    my $text_trafo = Wx::StaticText->new($self, -1, "Handling of transformations made inside Slic3r:", wxDefaultPosition, wxDefaultSize);
    
    # cb_Transformation
    $self->{cb_Transformation} = my $cb_Transformation = Wx::CheckBox->new($self, -1, "Preserve transformations");
    $cb_Transformation->SetValue($default_preserve);
    
    # cb_HideDialog
    $self->{cb_HideDialog} = my $cb_HideDialog = Wx::CheckBox->new($self, -1, "Don't ask again");

    my $vsizer = Wx::BoxSizer->new(wxVERTICAL);
    my $hsizer = Wx::BoxSizer->new(wxHORIZONTAL);
    $vsizer->Add($text_additional, 0, wxEXPAND | wxALL, 10);
    $vsizer->Add($choice, 0, wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT, 10);
    $vsizer->Add($text_trafo, 0, wxEXPAND | wxALL, 10);
    $vsizer->Add($cb_Transformation, 0, wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT, 10);
    $hsizer->Add($cb_HideDialog, 1, wxEXPAND | wxALL, 10);
    $hsizer->Add($self->CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 10);
    $vsizer->Add($hsizer, 0, wxEXPAND | wxALL, 0);
    
    $self->SetSizer($vsizer);
    $self->SetMinSize($self->GetSize);
    $vsizer->SetSizeHints($self);
    
    # needed to actually free memory
    EVT_CLOSE($self, sub {
        $self->EndModal(wxID_CANCEL);
        $self->Destroy;
    });
    
    return $self;
}

sub GetAdditionalOption {
    my ($self) = @_;
    return $self->{choice}->GetSelection;
}
sub GetPreserveTrafo {
    my ($self) = @_;
    return $self->{cb_Transformation}->GetValue;
}
sub GetHideOnNext {
    my ($self) = @_;
    return $self->{cb_HideDialog}->GetValue;
}

1;
