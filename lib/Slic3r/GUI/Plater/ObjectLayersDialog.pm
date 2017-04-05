package Slic3r::GUI::Plater::ObjectLayersDialog;
use strict;
use warnings;
use utf8;

use Slic3r::Geometry qw(PI X Y Z scale unscale);
use Slic3r::Print::State ':steps';
use List::Util qw(min max sum first);
use Wx qw(wxTheApp :dialog :id :misc :sizer :slider :statictext wxTAB_TRAVERSAL);
use Wx::Event qw(EVT_CLOSE EVT_BUTTON EVT_SLIDER);
use base 'Wx::Dialog';

sub new {
    my $class = shift;
    my ($parent, %params) = @_;
    my $self = $class->SUPER::new($parent, -1, $params{object}->name, wxDefaultPosition, [500,500], wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    $self->{model_object} = $params{model_object};
      my $model_object = $self->{model_object} = $params{model_object};
      my $obj_idx = $self->{obj_idx} = $params{obj_idx};
    my $plater = $self->{plater} = $parent;
    my $object = $self->{plater}->{print}->get_object($self->{obj_idx});
    
    $self->{update_spline_control} = 0;

    # Initialize 3D toolpaths preview
    if ($Slic3r::GUI::have_OpenGL) {
        $self->{preview3D} = Slic3r::GUI::Plater::3DPreview->new($self, $plater->{print});
        $self->{preview3D}->canvas->set_auto_bed_shape;
        $self->{preview3D}->canvas->SetSize([500,500]);
        $self->{preview3D}->canvas->SetMinSize($self->{preview3D}->canvas->GetSize);
        $self->{preview3D}->load_print;
        $self->{preview3D}->canvas->zoom_to_volumes;
    }

    $self->{splineControl} = Slic3r::GUI::Plater::SplineControl->new($self, Wx::Size->new(150, 200), $model_object);

    my $optgroup;
    $optgroup = $self->{optgroup} = Slic3r::GUI::OptionsGroup->new(
        parent      => $self,
        title       => 'Adaptive quality %',
        on_change   => sub {
            my ($opt_id) = @_;
            # There seems to be an issue with wxWidgets 3.0.2/3.0.3, where the slider
            # genates tens of events for a single value change.
            # Only trigger the recalculation if the value changes
            # or a live preview was activated and the mesh cut is not valid yet.
            if ($self->{adaptive_quality} != $optgroup->get_value($opt_id)) {
                $self->{adaptive_quality} = $optgroup->get_value($opt_id);
                $self->{model_object}->config->set('adaptive_slicing_quality', $optgroup->get_value($opt_id));
                # trigger re-slicing
                $self->_trigger_slicing;
            }
        },
        label_width  => 0,
    );

    $optgroup->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(
        opt_id      => 'adaptive_slicing_quality',
        type        => 'slider',
        label       => '',
        default     => $object->config->get('adaptive_slicing_quality'),
        min         => 0,
        max         => 100,
        full_width  => 1,
    ));
    $optgroup->get_field('adaptive_slicing_quality')->set_scale(1);
    $self->{adaptive_quality} = $object->config->get('adaptive_slicing_quality');
    # init quality slider
    if(!$object->config->get('adaptive_slicing')) {
        # disable slider
        $optgroup->get_field('adaptive_slicing_quality')->disable;
    }


    my $right_sizer = Wx::BoxSizer->new(wxVERTICAL);
    $right_sizer->Add($self->{splineControl}, 1, wxEXPAND | wxALL, 0);
    $right_sizer->Add($optgroup->sizer, 0, wxEXPAND | wxALL, 0);


    $self->{sizer} = Wx::BoxSizer->new(wxHORIZONTAL);
    $self->{sizer}->Add($self->{preview3D}, 3, wxEXPAND | wxTOP | wxBOTTOM, 0) if $self->{preview3D};
    $self->{sizer}->Add($right_sizer, 1, wxEXPAND | wxTOP | wxBOTTOM, 10);

    $self->SetSizerAndFit($self->{sizer});
    $self->SetSize([800, 600]);
    $self->SetMinSize($self->GetSize);

    # init spline control values
    # determine min and max layer height from perimeter extruder capabilities.
    my %extruders;
    for my $region_id (0 .. ($object->region_count - 1)) {
        foreach (qw(perimeter_extruder infill_extruder solid_infill_extruder)) {
            my $extruder_id = $self->{plater}->{print}->get_region($region_id)->config->get($_)-1;
            $extruders{$extruder_id} = $extruder_id;
        }
    }
    my $min_height = max(map {$self->{plater}->{print}->config->get_at('min_layer_height', $_)} (values %extruders));
    my $max_height = min(map {$self->{plater}->{print}->config->get_at('max_layer_height', $_)} (values %extruders));
    
    $self->{splineControl}->set_size_parameters($min_height, $max_height, unscale($object->size->z));
        
    
    $self->{splineControl}->on_layer_update(sub {
        # trigger re-slicing
        $self->_trigger_slicing;
    });
    
    $self->{splineControl}->on_z_indicator(sub {
        my ($z) = @_;
        $self->{preview3D}->canvas->SetCuttingPlane(Z, $z, []);
        $self->{preview3D}->canvas->Render;
    });

    return $self;
}

sub _trigger_slicing {
    my ($self) = @_;
    my $object = $self->{plater}->{print}->get_object($self->{obj_idx});
    $self->{plater}->pause_background_process;
    $self->{plater}->stop_background_process;
    if (!$Slic3r::GUI::Settings->{_}{background_processing}) {
        $self->{plater}->statusbar->SetCancelCallback(sub {
            $self->{plater}->stop_background_process;
            $self->{plater}->statusbar->SetStatusText("Slicing cancelled");
            $self->{plater}->preview_notebook->SetSelection(0);
        });
        $self->{plater}->{print}->reload_object($self->{obj_idx});
        $self->{plater}->on_model_change;
        $self->{plater}->start_background_process;
    }else{
        $self->{plater}->{print}->reload_object($self->{obj_idx});
        $self->{plater}->on_model_change;
        $self->{plater}->schedule_background_process;
    }
}

sub reload_preview {
    my ($self) = @_;
    $self->{splineControl}->update;
    $self->{preview3D}->reload_print;
    my $object = $self->{plater}->{print}->get_object($self->{obj_idx});
    if($object->layer_count-1 > 0) {
        # causes segfault...
        my $top_layer = $object->get_layer($object->layer_count-1);
        $self->{preview3D}->set_z($top_layer->print_z);
    }
}

1;
