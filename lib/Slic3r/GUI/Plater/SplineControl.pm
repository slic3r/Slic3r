package Slic3r::GUI::Plater::SplineControl;
use strict;
use warnings;
use utf8;

use List::Util qw(min max first);
use Slic3r::Geometry qw(X Y scale unscale convex_hull);
use Slic3r::Geometry::Clipper qw(offset JT_ROUND intersection_pl);
use Wx qw(:misc :pen :brush :sizer :font :cursor wxTAB_TRAVERSAL);
use Wx::Event qw(EVT_MOUSE_EVENTS EVT_PAINT EVT_ERASE_BACKGROUND EVT_SIZE);
use base 'Wx::Panel';

sub new {
    my $class = shift;
    my ($parent, $size) = @_;
    
    my $self = $class->SUPER::new($parent, -1, wxDefaultPosition, $size, wxTAB_TRAVERSAL);
    # This has only effect on MacOS. On Windows and Linux/GTK, the background is painted by $self->repaint().
    $self->SetBackgroundColour(Wx::wxWHITE);

    
    $self->{objects_brush}      = Wx::Brush->new(Wx::Colour->new(210,210,210), wxSOLID);
    $self->{selected_brush}     = Wx::Brush->new(Wx::Colour->new(255,128,128), wxSOLID);
    $self->{dragged_brush}      = Wx::Brush->new(Wx::Colour->new(128,128,255), wxSOLID);
    $self->{transparent_brush}  = Wx::Brush->new(Wx::Colour->new(0,0,0), wxTRANSPARENT);
    $self->{line_pen}           = Wx::Pen->new(Wx::Colour->new(50,50,50), 1, wxSOLID);
    $self->{print_center_pen}   = Wx::Pen->new(Wx::Colour->new(200,200,200), 1, wxSOLID);
    $self->{clearance_pen}      = Wx::Pen->new(Wx::Colour->new(0,0,200), 1, wxSOLID);
    $self->{skirt_pen}          = Wx::Pen->new(Wx::Colour->new(150,150,150), 1, wxSOLID);

    $self->{user_drawn_background} = $^O ne 'darwin';
    
    $self->{min_layer_height} = 0.12345678;
    $self->{max_layer_height} = 0.4;
    $self->{object_height} = 1.0;
    $self->{interpolation_points} = ();
    
    
    EVT_PAINT($self, \&repaint);
    EVT_ERASE_BACKGROUND($self, sub {}) if $self->{user_drawn_background};
    EVT_MOUSE_EVENTS($self, \&mouse_event);
    EVT_SIZE($self, sub {
        $self->update_bed_size;
        $self->Refresh;
    });
    
    return $self;
}

sub repaint {
    my ($self, $event) = @_;
    
    my $dc = Wx::AutoBufferedPaintDC->new($self);
    my $size = $self->GetSize;
    # Set axis orientation to leftRight, bottomUp and reset origin to the lower left corner
    $dc->SetAxisOrientation(1, 1);
    my @size = ($size->GetWidth, $size->GetHeight);
    $dc->SetDeviceOrigin(0, $size[1]);

    print "repaint\n";
    

    if ($self->{user_drawn_background}) {
        # On all systems the AutoBufferedPaintDC() achieves double buffering.
        # On MacOS the background is erased, on Windows the background is not erased 
        # and on Linux/GTK the background is erased to gray color.
        # Fill DC with the background on Windows & Linux/GTK.
        my $brush_background = Wx::Brush->new(Wx::wxWHITE, wxSOLID);
        $dc->SetPen(wxWHITE_PEN);
        $dc->SetBrush($brush_background);
        my $rect = $self->GetUpdateRegion()->GetBox();
        $dc->DrawRectangle($rect->GetLeft(), $rect->GetTop(), $rect->GetWidth(), $rect->GetHeight());
    }
    
    # draw scale (min and max indicator at the bottom)
    $dc->SetTextForeground(Wx::Colour->new(0,0,0));
    $dc->SetFont(Wx::Font->new(10, wxDEFAULT, wxNORMAL, wxNORMAL));
    $dc->DrawLabel(sprintf('%.4g', $self->{min_layer_height}), Wx::Rect->new(0, 15, $size[0], -15), wxALIGN_LEFT | wxALIGN_TOP);
    $dc->DrawLabel(sprintf('%.4g', $self->{max_layer_height}), Wx::Rect->new(0, 15, $size[0], -15), wxALIGN_RIGHT | wxALIGN_TOP);
    
    # draw current layers as lines
    my $scaling_y = $size[1]/$self->{object_height};
    my $scaling_x = $size[0]/($self->{max_layer_height} - $self->{min_layer_height});
    my $last_z = 0.0;
    foreach my $z (@{$self->{interpolation_points}}) {
    	my $layer_h = $z - $last_z;
    	$dc->SetPen($self->{line_pen});
        $dc->DrawLine(0, $z*$scaling_y, ($layer_h-$self->{min_layer_height})*$scaling_x, $z*$scaling_y);
        $last_z = $z;
    }
    
    $event->Skip;
}

sub mouse_event {
    my ($self, $event) = @_;
    
#    my $pos = $event->GetPosition;
#    my $point = $self->point_to_model_units([ $pos->x, $pos->y ]);  #]]
#    if ($event->ButtonDown) {
#        $self->{on_select_object}->(undef);
#        # traverse objects and instances in reverse order, so that if they're overlapping
#        # we get the one that gets drawn last, thus on top (as user expects that to move)
#        OBJECTS: for my $obj_idx (reverse 0 .. $#{$self->{objects}}) {
#            my $object = $self->{objects}->[$obj_idx];
#            for my $instance_idx (reverse 0 .. $#{ $object->instance_thumbnails }) {
#                my $thumbnail = $object->instance_thumbnails->[$instance_idx];
#                if (defined first { $_->contour->contains_point($point) } @$thumbnail) {
#                    $self->{on_select_object}->($obj_idx);
#                    
#                    if ($event->LeftDown) {
#                        # start dragging
#                        my $instance = $self->{model}->objects->[$obj_idx]->instances->[$instance_idx];
#                        my $instance_origin = [ map scale($_), @{$instance->offset} ];
#                        $self->{drag_start_pos} = [   # displacement between the click and the instance origin in scaled model units
#                            $point->x - $instance_origin->[X],
#                            $point->y - $instance_origin->[Y],  #-
#                        ];
#                        $self->{drag_object} = [ $obj_idx, $instance_idx ];
#                    } elsif ($event->RightDown) {
#                        $self->{on_right_click}->($pos);
#                    }
#                    
#                    last OBJECTS;
#                }
#            }
#        }
#        $self->Refresh;
#    } elsif ($event->LeftUp) {
#        $self->{on_instances_moved}->()
#            if $self->{drag_object};
#        $self->{drag_start_pos} = undef;
#        $self->{drag_object} = undef;
#        $self->SetCursor(wxSTANDARD_CURSOR);
#    } elsif ($event->LeftDClick) {
#    	$self->{on_double_click}->();
#    } elsif ($event->Dragging) {
#        return if !$self->{drag_start_pos}; # concurrency problems
#        my ($obj_idx, $instance_idx) = @{ $self->{drag_object} };
#        my $model_object = $self->{model}->objects->[$obj_idx];
#        $model_object->instances->[$instance_idx]->set_offset(
#            Slic3r::Pointf->new(
#                unscale($point->[X] - $self->{drag_start_pos}[X]),
#                unscale($point->[Y] - $self->{drag_start_pos}[Y]),
#            ));
#        $model_object->update_bounding_box;
#        $self->Refresh;
#    } elsif ($event->Moving) {
#        my $cursor = wxSTANDARD_CURSOR;
#        if (defined first { $_->contour->contains_point($point) } map @$_, map @{$_->instance_thumbnails}, @{ $self->{objects} }) {
#            $cursor = Wx::Cursor->new(wxCURSOR_HAND);
#        }
#        $self->SetCursor($cursor);
#    }
}

sub update_bed_size {
    my $self = shift;
    
#    # when the canvas is not rendered yet, its GetSize() method returns 0,0
#    my $canvas_size = $self->GetSize;
#    my ($canvas_w, $canvas_h) = ($canvas_size->GetWidth, $canvas_size->GetHeight);
#    return if $canvas_w == 0;
#    
#    # get bed shape polygon
#    $self->{bed_polygon} = my $polygon = Slic3r::Polygon->new_scale(@{$self->{config}->bed_shape});
#    my $bb = $polygon->bounding_box;
#    my $size = $canvas_size;
#    
#    # calculate the scaling factor needed for constraining print bed area inside preview
#    # scaling_factor is expressed in pixel / mm
#    $self->{scaling_factor} = min($canvas_w / unscale($size->x), $canvas_h / unscale($size->y)); #)
#    
#    # calculate the displacement needed to center bed
#    $self->{bed_origin} = [
#        $canvas_w/2  - (unscale($bb->x_max + $bb->x_min)/2 * $self->{scaling_factor}),
#        $canvas_h - ($canvas_h/2 - (unscale($bb->y_max + $bb->y_min)/2 * $self->{scaling_factor})),
#    ];
#    
#    # calculate print center
#    my $center = $bb->center;
#    $self->{print_center} = [ unscale($center->x), unscale($center->y) ]; #))
#    
#    # cache bed contours and grid
#    {
#        my $step = scale 10;  # 1cm grid
#        my @polylines = ();
#        for (my $x = $bb->x_min - ($bb->x_min % $step) + $step; $x < $bb->x_max; $x += $step) {
#            push @polylines, Slic3r::Polyline->new([$x, $bb->y_min], [$x, $bb->y_max]);
#        }
#        for (my $y = $bb->y_min - ($bb->y_min % $step) + $step; $y < $bb->y_max; $y += $step) {
#            push @polylines, Slic3r::Polyline->new([$bb->x_min, $y], [$bb->x_max, $y]);
#        }
#        @polylines = @{intersection_pl(\@polylines, [$polygon])};
#        $self->{grid} = [ map $self->scaled_points_to_pixel([ @$_[0,-1] ], 1), @polylines ];
#    }
}

# Set basic parameters for this control.
# min/max_layer_height are required to define the x-range, object_height is used to scale the y-range.
# Must be called if object selection changes.
sub set_size_parameters {
	my ($self, $min_layer_height, $max_layer_height, $object_height) = @_;
	
	$self->{min_layer_height} = $min_layer_height;
    $self->{max_layer_height} = $max_layer_height;
    $self->{object_height} = $object_height;
	
	#$self->repaint;
}

sub set_interpolation_points {
	my ($self, @interpolation_points) = @_;

	$self->{interpolation_points} = [@interpolation_points];
	print "set_interpolation_points\n";
	$self->Refresh;
	print "refresh\n";
}

# convert a model coordinate into a pixel coordinate
sub unscaled_point_to_pixel {
    my ($self, $point) = @_;
    
    my $canvas_height = $self->GetSize->GetHeight;
    my $zero = $self->{bed_origin};
    return [
        $point->[X] * $self->{scaling_factor} + $zero->[X],
        $canvas_height - $point->[Y] * $self->{scaling_factor} + ($zero->[Y] - $canvas_height),
    ];
}

sub scaled_points_to_pixel {
    my ($self, $points, $unscale) = @_;
    
    my $result = [];
    foreach my $point (@$points) {
        $point = [ map unscale($_), @$point ] if $unscale;
        push @$result, $self->unscaled_point_to_pixel($point);
    }
    return $result;
}

sub point_to_model_units {
    my ($self, $point) = @_;
    
    my $zero = $self->{bed_origin};
    return Slic3r::Point->new(
        scale ($point->[X] - $zero->[X]) / $self->{scaling_factor},
        scale ($zero->[Y] - $point->[Y]) / $self->{scaling_factor},
    );
}

1;
