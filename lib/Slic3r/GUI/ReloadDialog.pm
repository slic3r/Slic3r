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
    my ($parent,$default_selection) = @_;
    my $self = $class->SUPER::new($parent, -1, "Additional parts and modifiers detected", wxDefaultPosition, [350,100], wxDEFAULT_DIALOG_STYLE);
    
    # label
    my $text = Wx::StaticText->new($self, -1, "Additional parts and modifiers are loaded in the current model. \n\nHow do you want to proceed?", wxDefaultPosition, wxDefaultSize);

    # selector
    $self->{choice} = my $choice = Wx::Choice->new($self, -1, wxDefaultPosition, wxDefaultSize, []);
    $choice->Append("Reload all linked files");
    $choice->Append("Reload main file, copy added parts & modifiers");
    $choice->Append("Reload main file, discard added parts & modifiers");
    $choice->SetSelection($default_selection);
    
    # checkbox
    $self->{checkbox} = my $checkbox = Wx::CheckBox->new($self, -1, "Don't ask again");

    my $vsizer = Wx::BoxSizer->new(wxVERTICAL);
    my $hsizer = Wx::BoxSizer->new(wxHORIZONTAL);
    $vsizer->Add($text, 0, wxEXPAND | wxALL, 10);
    $vsizer->Add($choice, 0, wxEXPAND | wxALL, 10);
    $hsizer->Add($checkbox, 1, wxEXPAND | wxALL, 10);
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

sub GetSelection {
    my ($self) = @_;
    return $self->{choice}->GetSelection;
}
sub GetHideOnNext {
    my ($self) = @_;
    return $self->{checkbox}->GetValue;
}

1;
