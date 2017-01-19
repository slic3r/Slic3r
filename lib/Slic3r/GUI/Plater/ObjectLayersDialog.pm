package Slic3r::GUI::Plater::ObjectLayersDialog;
use strict;
use warnings;
use utf8;

use Slic3r::Geometry qw(PI X scale unscale);
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
    
    #IMPORTANT TODO: set correct min / max layer height from config!!!!
    $self->{splineControl}->set_size_parameters(0.1, 0.4, unscale($object->size->z));
        
    # get array of current Z coordinates for selected object
    my @layer_heights = map $_->print_z, @{$object->layers};
    
    $self->{splineControl}->set_interpolation_points(@layer_heights);
    
    return $self;
}

1;
