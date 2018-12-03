package Slic3r::Print::Object;
# extends c++ class Slic3r::PrintObject (Print.xsp)
use strict;
use warnings;

use POSIX;
use List::Util qw(min max sum first any);
use Slic3r::Flow ':roles';
use Slic3r::Geometry qw(X Y Z PI scale unscale chained_path epsilon);
use Slic3r::Geometry::Clipper qw(diff diff_ex intersection intersection_ex union union_ex 
    offset offset_ex offset2 offset2_ex intersection_ppl CLIPPER_OFFSET_SCALE JT_MITER);
use Slic3r::Print::State ':steps';
use Slic3r::Surface ':types';


# TODO: lazy
sub fill_maker {
    my $self = shift;
    return Slic3r::Fill->new(bounding_box => $self->bounding_box);
}

sub region_volumes {
    my $self = shift;
    return [ map $self->get_region_volumes($_), 0..($self->region_count - 1) ];
}

sub layers {
    my $self = shift;
    return [ map $self->get_layer($_), 0..($self->layer_count - 1) ];
}

sub support_layers {
    my $self = shift;
    return [ map $self->get_support_layer($_), 0..($self->support_layer_count - 1) ];
}

sub generate_support_material {
    my $self = shift;
    
    # prerequisites
    $self->slice;
    
    return if $self->step_done(STEP_SUPPORTMATERIAL);
    $self->set_step_started(STEP_SUPPORTMATERIAL);
    
    $self->clear_support_layers;
    
    if ((!$self->config->support_material
		    && $self->config->raft_layers == 0
		    && $self->config->support_material_enforce_layers == 0)
		  || scalar(@{$self->layers}) < 2
		  ) {
        $self->set_step_done(STEP_SUPPORTMATERIAL);
        return;
    }
    $self->print->status_cb->(85, "Generating support material");
    
    $self->_support_material->generate($self);
    
    $self->set_step_done(STEP_SUPPORTMATERIAL);
    my $stats = sprintf "Weight: %.1fg, Cost: %.1f" , $self->print->total_weight, $self->print->total_cost;
    $self->print->status_cb->(85, $stats);
}

sub _support_material {
    my ($self) = @_;
    
    my $first_layer_flow = Slic3r::Flow->new_from_width(
        width               => ($self->print->config->first_layer_extrusion_width || $self->config->support_material_extrusion_width),
        role                => FLOW_ROLE_SUPPORT_MATERIAL,
        nozzle_diameter     => $self->print->config->nozzle_diameter->[ $self->config->support_material_extruder-1 ]
                                // $self->print->config->nozzle_diameter->[0],
        layer_height        => $self->config->get_abs_value('first_layer_height'),
        bridge_flow_ratio   => 0,
    );
    
    return Slic3r::Print::SupportMaterial->new(
        print_config        => $self->print->config,
        object_config       => $self->config,
        first_layer_flow    => $first_layer_flow,
        flow                => $self->support_material_flow(FLOW_ROLE_SUPPORT_MATERIAL),
        interface_flow      => $self->support_material_flow(FLOW_ROLE_SUPPORT_MATERIAL_INTERFACE),
    );
}

1;
