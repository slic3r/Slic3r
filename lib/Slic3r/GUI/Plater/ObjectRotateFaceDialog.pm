# Rotate an object such that a face is aligned with a specified plane.
# This dialog gets opened with the "Rotate face" button above the platter.

package Slic3r::GUI::Plater::ObjectRotateFaceDialog;
use strict;
use warnings;
use utf8;

use POSIX qw(ceil);
use Scalar::Util qw(looks_like_number);
use Slic3r::Geometry qw(PI X Y Z);
use Wx qw(wxTheApp :dialog :id :misc :sizer wxTAB_TRAVERSAL);
use Wx::Event qw(EVT_CLOSE EVT_BUTTON);
use base 'Wx::Dialog';

sub new {
    my $class = shift;
    my ($parent, %params) = @_;
    my $self = $class->SUPER::new($parent, -1, $params{object}->name, wxDefaultPosition, [500,500], wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    $self->{model_object_idx} = $params{model_object_idx};
    $self->{model_object} = $params{model_object};
    $self->{normal} = undef;
    # Note whether the window was already closed, so a pending update is not executed.
    $self->{already_closed} = 0;
    $self->{model_object}->transform_by_instance($self->{model_object}->get_instance(0), 1);
    
    # Options
    $self->{options} = {
        axis            => Z,
    };
    my $optgroup;
    $optgroup = $self->{optgroup} = Slic3r::GUI::OptionsGroup->new(
        parent      => $self,
        title       => 'Rotate to Align Face with Plane',
        on_change   => sub {
            my ($opt_id) = @_;
            if ($self->{options}{$opt_id} != $optgroup->get_value($opt_id)){
                $self->{options}{$opt_id} = $optgroup->get_value($opt_id);
            }
        },
        label_width  => 120,
    );
    $optgroup->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(
        opt_id      => 'axis',
        type        => 'select',
        label       => 'Plane',
        labels      => ['YZ','XZ','XY'],
        values      => [X,Y,Z],
        default     => $self->{options}{axis},
    ));
    
    {
        my $button_sizer = Wx::BoxSizer->new(wxVERTICAL);
        
        $self->{btn_rot} = Wx::Button->new($self, -1, "Rotate to Plane", wxDefaultPosition, wxDefaultSize);
        $self->{btn_rot}->SetDefault;
        $self->{btn_rot}->Disable;
        $button_sizer->Add($self->{btn_rot}, 0, wxALIGN_RIGHT | wxALL, 10);
        $optgroup->append_line(Slic3r::GUI::OptionsGroup::Line->new(
            sizer => $button_sizer,
        ));
    }
    
    # left pane with tree
    my $left_sizer = Wx::BoxSizer->new(wxVERTICAL);
    $left_sizer->Add($optgroup->sizer, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 10);
    
    # right pane with preview canvas
    my $canvas;
    if ($Slic3r::GUI::have_OpenGL) {
        $canvas = $self->{canvas} = Slic3r::GUI::3DScene->new($self);
        $canvas->load_object($self->{model_object}, undef, [0]);
        $canvas->set_auto_bed_shape;
        $canvas->SetSize([500,500]);
        $canvas->enable_picking(1);
        $canvas->enable_face_select(1);
        $canvas->SetMinSize($canvas->GetSize);
        $canvas->zoom_to_volumes;
        $canvas->on_select(sub {
            my ($volume_idx) = @_;
            $self->{btn_rot}->Disable;
            $self->{normal} = $canvas->calculate_normal($volume_idx);
            $self->{btn_rot}->Enable if defined $self->{normal};
        });
    }
    
    $self->{sizer} = Wx::BoxSizer->new(wxHORIZONTAL);
    $self->{sizer}->Add($left_sizer, 0, wxEXPAND | wxTOP | wxBOTTOM, 10);
    $self->{sizer}->Add($canvas, 1, wxEXPAND | wxALL, 0) if $canvas;
    
    $self->SetSizer($self->{sizer});
    $self->SetMinSize($self->GetSize);
    $self->{sizer}->SetSizeHints($self);
    
    EVT_BUTTON($self, $self->{btn_rot}, sub {
        $self->{already_closed} = 1;
        $self->EndModal(wxID_OK);
        $self->Destroy();
    });
    
    EVT_CLOSE($self, sub {
        # Note that the window was already closed, so a pending update will not be executed.
        $self->{already_closed} = 1;
        $self->EndModal(wxID_CANCEL);
        $self->Destroy();
    });
    
    return $self;
}

sub SelectedNormal {
    my ($self) = @_;
    return  $self->{normal};
}

sub SelectedAxis {
    my ($self) = @_;
    return  $self->{options}->{axis};
}

1;
