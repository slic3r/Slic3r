# Cut an object at a Z position, keep either the top or the bottom of the object.
# This dialog gets opened with the "Cut..." button above the platter.

package Slic3r::GUI::Plater::ObjectCutDialog;
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
    $self->{new_model_objects} = [];
    # Mark whether the mesh cut is valid.
    # If not, it needs to be recalculated by _update() on wxTheApp->CallAfter() or on exit of the dialog.
    $self->{mesh_cut_valid} = 0;
    # Note whether the window was already closed, so a pending update is not executed.
    $self->{already_closed} = 0;
    
    $self->{model_object}->transform_by_instance($self->{model_object}->get_instance(0), 1);
    
    # cut options
    my $size_z = $self->{model_object}->instance_bounding_box(0)->size->z;
    $self->{cut_options} = {
        axis            => Z,
        z               => $size_z/2,
        keep_upper      => 0,
        keep_lower      => 1,
        rotate_lower    => 0,
        preview         => 1,
    };
    
    my $optgroup;
    $optgroup = $self->{optgroup} = Slic3r::GUI::OptionsGroup->new(
        parent      => $self,
        title       => 'Cut',
        on_change   => sub {
            my ($opt_id) = @_;
            # There seems to be an issue with wxWidgets 3.0.2/3.0.3, where the slider
            # genates tens of events for a single value change.
            # Only trigger the recalculation if the value changes
            # or a live preview was activated and the mesh cut is not valid yet.
            if ($self->{cut_options}{$opt_id} != $optgroup->get_value($opt_id) ||
                ! $self->{mesh_cut_valid} && $self->_life_preview_active()) {
                $self->{cut_options}{$opt_id} = $optgroup->get_value($opt_id);
                $self->{mesh_cut_valid} = 0;
                wxTheApp->CallAfter(sub {
                    $self->_update;
                });
            }
        },
        label_width  => 120,
    );
    $optgroup->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(
        opt_id      => 'axis',
        type        => 'select',
        label       => 'Axis',
        labels      => ['X','Y','Z'],
        values      => [X,Y,Z],
        default     => $self->{cut_options}{axis},
    ));
    $optgroup->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(
        opt_id      => 'z',
        type        => 'slider',
        label       => 'Z',
        default     => $self->{cut_options}{z},
        min         => 0,
        max         => $size_z,
        full_width  => 1,
    ));
    {
        my $line = Slic3r::GUI::OptionsGroup::Line->new(
            label => 'Keep',
        );
        $line->append_option(Slic3r::GUI::OptionsGroup::Option->new(
            opt_id  => 'keep_upper',
            type    => 'bool',
            label   => 'Upper part',
            default => $self->{cut_options}{keep_upper},
        ));
        $line->append_option(Slic3r::GUI::OptionsGroup::Option->new(
            opt_id  => 'keep_lower',
            type    => 'bool',
            label   => 'Lower part',
            default => $self->{cut_options}{keep_lower},
        ));
        $optgroup->append_line($line);
    }
    $optgroup->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(
        opt_id      => 'rotate_lower',
        label       => 'Rotate lower part upwards',
        type        => 'bool',
        tooltip     => 'If enabled, the lower part will be rotated by 180° so that the flat cut surface lies on the print bed.',
        default     => $self->{cut_options}{rotate_lower},
    ));
    $optgroup->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(
        opt_id      => 'preview',
        label       => 'Show preview',
        type        => 'bool',
        tooltip     => 'If enabled, object will be cut in real time.',
        default     => $self->{cut_options}{preview},
    ));
    {
        my $cut_button_sizer = Wx::BoxSizer->new(wxVERTICAL);
        
        $self->{btn_cut} = Wx::Button->new($self, -1, "Perform cut", wxDefaultPosition, wxDefaultSize);
        $self->{btn_cut}->SetDefault;
        $cut_button_sizer->Add($self->{btn_cut}, 0, wxALIGN_RIGHT | wxALL, 10);
        
        $self->{btn_cut_grid} = Wx::Button->new($self, -1, "Cut by grid…", wxDefaultPosition, wxDefaultSize);
        $cut_button_sizer->Add($self->{btn_cut_grid}, 0, wxALIGN_RIGHT | wxALL, 10);
        
        $optgroup->append_line(Slic3r::GUI::OptionsGroup::Line->new(
            sizer => $cut_button_sizer,
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
        $canvas->SetMinSize($canvas->GetSize);
        $canvas->zoom_to_volumes;
    }
    
    $self->{sizer} = Wx::BoxSizer->new(wxHORIZONTAL);
    $self->{sizer}->Add($left_sizer, 0, wxEXPAND | wxTOP | wxBOTTOM, 10);
    $self->{sizer}->Add($canvas, 1, wxEXPAND | wxALL, 0) if $canvas;
    
    $self->SetSizer($self->{sizer});
    $self->SetMinSize($self->GetSize);
    $self->{sizer}->SetSizeHints($self);
    
    EVT_BUTTON($self, $self->{btn_cut}, sub {
        # Recalculate the cut if the preview was not active.
        $self->_perform_cut() unless $self->{mesh_cut_valid};

        # Adjust position / orientation of the split object halves.
        if (my $lower = $self->{new_model_objects}[0]) {
            if ($self->{cut_options}{rotate_lower} && $self->{cut_options}{axis} == Z) {
                $lower->rotate(PI, X);
            }
            $lower->center_around_origin;  # align to Z = 0
        }
        if (my $upper = $self->{new_model_objects}[1]) {
            $upper->center_around_origin;  # align to Z = 0
        }
        
        # Note that the window was already closed, so a pending update will not be executed.
        $self->{already_closed} = 1;
        $self->EndModal(wxID_OK);
        $self->Destroy();
    });
    
    EVT_BUTTON($self, $self->{btn_cut_grid}, sub {
        my $grid_x = Wx::GetTextFromUser("Enter the width of the desired tiles along the X axis:",
            "Cut by Grid", 100, $self);
        return if !looks_like_number($grid_x) || $grid_x <= 0;
        
        my $grid_y = Wx::GetTextFromUser("Enter the width of the desired tiles along the Y axis:",
            "Cut by Grid", 100, $self);
        return if !looks_like_number($grid_y) || $grid_y <= 0;
        
        my $process_dialog = Wx::ProgressDialog->new('Cutting…', "Cutting model by grid…", 100, $self, 0);
        $process_dialog->Pulse;
        
        my $meshes = $self->{model_object}->mesh->cut_by_grid(Slic3r::Pointf->new($grid_x, $grid_y));
        $self->{new_model_objects} = [];
        
        my $bb = $self->{model_object}->bounding_box;
        $self->{new_model} = my $model = Slic3r::Model->new;
        for my $i (0..$#$meshes) {
            push @{$self->{new_model_objects}}, my $o = $model->add_object(
                name => sprintf('%s (%d)', $self->{model_object}->name, $i+1),
            );
            my $v = $o->add_volume(
                mesh    => $meshes->[$i],
                name    => $o->name,
            );
            $o->center_around_origin;
            my $i = $o->add_instance(
                offset => Slic3r::Pointf->new(@{$o->origin_translation->negative}[X,Y]),
            );
            $i->offset->translate(
                5 * ceil(($i->offset->x - $bb->center->x) / $grid_x),
                5 * ceil(($i->offset->y - $bb->center->y) / $grid_y),
            );
        }
        
        $process_dialog->Destroy;
        
        # Note that the window was already closed, so a pending update will not be executed.
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

    $self->_update;
    
    return $self;
}

# scale Z down to original size since we're using the transformed mesh for 3D preview
# and cut dialog but ModelObject::cut() needs Z without any instance transformation
sub _mesh_slice_z_pos
{
    my ($self) = @_;
    
    my $bb = $self->{model_object}->instance_bounding_box(0);
    my $z = $self->{cut_options}{axis} == X ? $bb->x_min
          : $self->{cut_options}{axis} == Y ? $bb->y_min
          : $bb->z_min;
    
    $z += $self->{cut_options}{z} / $self->{model_object}->instances->[0]->scaling_factor;
    
    return $z;
}

# Only perform live preview if just a single part of the object shall survive.
sub _life_preview_active
{
    my ($self) = @_;
    return $self->{cut_options}{preview} && ($self->{cut_options}{keep_upper} != $self->{cut_options}{keep_lower});
}

# Slice the mesh, keep the top / bottom part.
sub _perform_cut 
{
    my ($self) = @_;

    # Early exit. If the cut is valid, don't recalculate it.
    return if $self->{mesh_cut_valid};

    my $z = $self->_mesh_slice_z_pos();
    
    my ($new_model) = $self->{model_object}->cut($self->{cut_options}{axis}, $z);
    my ($upper_object, $lower_object) = @{$new_model->objects};
    $self->{new_model} = $new_model;
    $self->{new_model_objects} = [];
    if ($self->{cut_options}{keep_upper} && $upper_object->volumes_count > 0) {
        $self->{new_model_objects}[1] = $upper_object;
    }
    if ($self->{cut_options}{keep_lower} && $lower_object->volumes_count > 0) {
        $self->{new_model_objects}[0] = $lower_object;
    }

    $self->{mesh_cut_valid} = 1;
}

sub _update {
    my ($self) = @_;

    # Don't update if the window was already closed.
    # We are not sure whether the action planned by wxTheApp->CallAfter() may be triggered after the window is closed.
    # Probably not, but better be safe than sorry, which is espetially true on multiple platforms.
    return if $self->{already_closed};

    # Only recalculate the cut, if the live cut preview is active.
    my $life_preview_active = $self->_life_preview_active();
    $self->_perform_cut() if $life_preview_active;
    
    {
        # scale Z down to original size since we're using the transformed mesh for 3D preview
        # and cut dialog but ModelObject::cut() needs Z without any instance transformation
        my $z = $self->_mesh_slice_z_pos();
        
        # update canvas
        if ($self->{canvas}) {
            # get volumes to render
            my @objects = ();
            if ($life_preview_active) {
                push @objects, grep defined, @{$self->{new_model_objects}};
            } else {
                push @objects, $self->{model_object};
            }
            
            # get section contour
            my @expolygons = ();
            foreach my $volume (@{$self->{model_object}->volumes}) {
                next if !$volume->mesh;
                next if $volume->modifier;
                my $expp = $volume->mesh->slice_at($self->{cut_options}{axis}, $z);
                push @expolygons, @$expp;
            }
            
            my $offset = $self->{model_object}->instances->[0]->offset;
            foreach my $expolygon (@expolygons) {
                $self->{model_object}->instances->[0]->transform_polygon($_)
                    for @$expolygon;
                
                if ($self->{cut_options}{axis} != X) {
                    $expolygon->translate(0, Slic3r::Geometry::scale($offset->y));  #)
                }
                if ($self->{cut_options}{axis} != Y) {
                    $expolygon->translate(Slic3r::Geometry::scale($offset->x), 0);
                }
            }
            
            $self->{canvas}->reset_objects;
            $self->{canvas}->load_object($_, undef, [0]) for @objects;
            
            my $plane_z = $self->{cut_options}{z};
            $plane_z += 0.02 if !$self->{cut_options}{keep_upper};
            $plane_z -= 0.02 if !$self->{cut_options}{keep_lower};
            $self->{canvas}->SetCuttingPlane(
                $self->{cut_options}{axis},
                $plane_z,
                [@expolygons],
            );
            $self->{canvas}->Render;
        }
    }
    
    # update controls
    {
        my $z = $self->{cut_options}{z};
        my $optgroup = $self->{optgroup};
        {
            my $bb = $self->{model_object}->instance_bounding_box(0);
            my $max = $self->{cut_options}{axis} == X ? $bb->size->x
                    : $self->{cut_options}{axis} == Y ? $bb->size->y ###
                    : $bb->size->z;
            $optgroup->get_field('z')->set_range(0, $max);
        }
        $optgroup->get_field('keep_upper')->toggle(my $have_upper = abs($z - $optgroup->get_option('z')->max) > 0.1);
        $optgroup->get_field('keep_lower')->toggle(my $have_lower = $z > 0.1);
        $optgroup->get_field('rotate_lower')->toggle($z > 0 && $self->{cut_options}{keep_lower} && $self->{cut_options}{axis} == Z);
        $optgroup->get_field('preview')->toggle($self->{cut_options}{keep_upper} != $self->{cut_options}{keep_lower});
    
        # update cut button
        if (($self->{cut_options}{keep_upper} && $have_upper)
            || ($self->{cut_options}{keep_lower} && $have_lower)) {
            $self->{btn_cut}->Enable;
        } else {
            $self->{btn_cut}->Disable;
        }
    }
}

sub NewModelObjects {
    my ($self) = @_;
    return grep defined, @{ $self->{new_model_objects} };
}

1;
