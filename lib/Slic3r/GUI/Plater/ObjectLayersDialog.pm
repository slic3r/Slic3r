package Slic3r::GUI::Plater::ObjectLayersDialog;
use strict;
use warnings;
use utf8;

use Slic3r::Geometry qw(PI X scale unscale);
use List::Util qw(min max sum first);
use Wx qw(wxTheApp :dialog :id :misc :sizer wxTAB_TRAVERSAL);
use Wx::Event qw(EVT_CLOSE EVT_BUTTON);
use base 'Wx::Dialog';

sub new {
    my $class = shift;
    my ($parent, %params) = @_;
    my $self = $class->SUPER::new($parent, -1, $params{object}->name, wxDefaultPosition, [500,500], wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    $self->{model_object} = $params{model_object};
   

  	my $model_object = $self->{model_object} = $params{model_object};
  	my $obj_idx = $self->{obj_idx} = $params{obj_idx};
    my $plater = $self->{plater} = $parent;
    
    # Initialize 3D toolpaths preview
    if ($Slic3r::GUI::have_OpenGL) {
        $self->{preview3D} = Slic3r::GUI::Plater::3DPreview->new($self, $plater->{print});
        #$self->{preview3D}->canvas->on_viewport_changed(sub {
        #    $self->{canvas3D}->set_viewport_from_scene($self->{preview3D}->canvas);
        #});
        $self->{preview3D}->canvas->set_auto_bed_shape;
        $self->{preview3D}->canvas->SetSize([500,500]);
        $self->{preview3D}->canvas->SetMinSize($self->{preview3D}->canvas->GetSize);
        $self->{preview3D}->load_print;
        $self->{preview3D}->canvas->zoom_to_volumes;
    }
    
	$self->{splineControl} = Slic3r::GUI::Plater::SplineControl->new($self, Wx::Size->new(100, 200));
	
	my $right_sizer = Wx::BoxSizer->new(wxVERTICAL);
	$right_sizer->Add($self->{splineControl}, 1, wxEXPAND | wxALL, 0);
	
	$self->{sizer} = Wx::BoxSizer->new(wxHORIZONTAL);
	$self->{sizer}->Add($self->{preview3D}, 1, wxEXPAND | wxTOP | wxBOTTOM, 0) if $self->{preview3D};
	$self->{sizer}->Add($right_sizer, 0, wxEXPAND | wxTOP | wxBOTTOM, 10);

    $self->SetSizerAndFit($self->{sizer});
    $self->SetSize([800, 600]);
    $self->SetMinSize($self->GetSize);
    
    # init spline control values    
    my $object = $self->{plater}->{print}->get_object($self->{obj_idx});
    
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
        
    # get array of current Z coordinates for selected object
    my @layer_heights = map $_->print_z, @{$object->layers};
    
    $self->{splineControl}->set_layer_points(@layer_heights);
    
    return $self;
}

sub reload_preview {
	my ($self) = @_;
	$self->{preview3D}->reload_print;
}

1;
