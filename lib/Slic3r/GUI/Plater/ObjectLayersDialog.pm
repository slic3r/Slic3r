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
    my $object = $self->{object} = $self->{plater}->{print}->get_object($self->{obj_idx});
    
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

	$self->{splineControl} = Slic3r::GUI::Plater::SplineControl->new($self, Wx::Size->new(150, 200), $object);

	my $quality_slider = $self->{quality_slider} = Wx::Slider->new(
        $self, -1,
        0,                              # default
        0,                              # min
        # we set max to a bogus non-zero value because the MSW implementation of wxSlider
        # will skip drawing the slider if max <= min:
        1,                              # max
        wxDefaultPosition,
        wxDefaultSize,
        wxHORIZONTAL,
    );

    my $quality_label = $self->{quality_label} = Wx::StaticText->new($self, -1, "    <-Quality", wxDefaultPosition,
        [-1,-1], wxALIGN_LEFT);
    my $value_label = $self->{value_label} = Wx::StaticText->new($self, -1, "", wxDefaultPosition,
        [50,-1], wxALIGN_CENTRE_HORIZONTAL | wxST_NO_AUTORESIZE);
    my $speed_label = $self->{speed_label} = Wx::StaticText->new($self, -1, "Speed->", wxDefaultPosition,
        [-1,-1], wxST_NO_AUTORESIZE | wxALIGN_RIGHT);
    $quality_label->SetFont($Slic3r::GUI::small_font);
    $value_label->SetFont($Slic3r::GUI::small_font);
    $speed_label->SetFont($Slic3r::GUI::small_font);
	
	my $quality_label_sizer = Wx::BoxSizer->new(wxHORIZONTAL);	
	$quality_label_sizer->Add($quality_label, 1, wxEXPAND | wxALL, 0);
    $quality_label_sizer->Add($value_label, 1, wxEXPAND | wxALL, 0);
    $quality_label_sizer->Add($speed_label, 1, wxEXPAND | wxALL, 0);
	
	my $right_sizer = Wx::BoxSizer->new(wxVERTICAL);
	$right_sizer->Add($self->{splineControl}, 1, wxEXPAND | wxALL, 0);
	$right_sizer->Add($quality_slider, 0, wxEXPAND | wxALL, 0);
	$right_sizer->Add($quality_label_sizer, 0, wxEXPAND | wxALL, 0);
	
	
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
        $self->{plater}->stop_background_process;
        $self->{object}->invalidate_step(STEP_SLICE);
        $self->{plater}->start_background_process;
    });
    
    $self->{splineControl}->on_z_indicator(sub {
        my ($z) = @_;
        #$self->{preview3D}->canvas->cutting_plane_z($z);
        $self->{preview3D}->canvas->SetCuttingPlane(Z, $z, []);
        $self->{preview3D}->canvas->Render;
    });

    # init quality slider
    if($object->config->adaptive_slicing) {
        my $quality_value = $object->config->get('adaptive_slicing_quality');
        $value_label->SetLabel(sprintf '%.2f', $quality_value);
        $quality_slider->SetRange(0, 100);
        $quality_slider->SetValue($quality_value*100);
    }else{
        # disable slider
        $value_label->SetLabel("");
        $quality_label->Enable(0);
        $quality_slider->Enable(0);
    }

    EVT_SLIDER($self, $quality_slider, sub {
    	$self->{plater}->pause_background_process;
        my $quality_value = $quality_slider->GetValue/100;
        $value_label->SetLabel(sprintf '%.2f', $quality_value);
        my $success = $object->config->set('adaptive_slicing_quality', $quality_value);
        $object->layer_height_spline->setCuspValue($quality_value);
        # trigger re-slicing
        $self->{plater}->stop_background_process;
        $self->{object}->invalidate_step(STEP_SLICE);
        $self->{plater}->schedule_background_process;
    });

    return $self;
}

sub reload_preview {
	my ($self) = @_;
    $self->{splineControl}->update;
	$self->{preview3D}->reload_print;
	if($self->{object}->layer_count-1 > 0) {
		my $top_layer = $self->{object}->get_layer($self->{object}->layer_count-1);
		$self->{preview3D}->set_z($top_layer->print_z);
	}
}

1;
