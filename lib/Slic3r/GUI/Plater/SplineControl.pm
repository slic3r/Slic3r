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
    my ($parent, $size, $object) = @_;
    
    my $self = $class->SUPER::new($parent, -1, wxDefaultPosition, $size, wxTAB_TRAVERSAL);
    
    $self->{object} = $object;
    
    # This has only effect on MacOS. On Windows and Linux/GTK, the background is painted by $self->repaint().
    $self->SetBackgroundColour(Wx::wxWHITE);

    $self->{line_pen}           = Wx::Pen->new(Wx::Colour->new(50,50,50), 1, wxSOLID);
    $self->{original_pen}       = Wx::Pen->new(Wx::Colour->new(200,200,200), 1, wxSOLID);
    $self->{interactive_pen}    = Wx::Pen->new(Wx::Colour->new(255,0,0), 1, wxSOLID);
    $self->{resulting_pen}      = Wx::Pen->new(Wx::Colour->new(50,255,50), 1, wxSOLID);

    $self->{user_drawn_background} = $^O ne 'darwin';

    $self->{scaling_factor_x} = 1;
    $self->{scaling_factor_y} = 1;
    
    $self->{min_layer_height} = 0.1;
    $self->{max_layer_height} = 0.4;
    $self->{object_height} = 1.0;
    
    # initialize values
    $self->update;
    
    EVT_PAINT($self, \&repaint);
    EVT_ERASE_BACKGROUND($self, sub {}) if $self->{user_drawn_background};
    EVT_MOUSE_EVENTS($self, \&mouse_event);
    EVT_SIZE($self, sub {
        $self->_update_canvas_size;
        $self->Refresh;
    });

    return $self;
}

sub repaint {
    my ($self, $event) = @_;
    
    my $dc = Wx::AutoBufferedPaintDC->new($self);
    my $size = $self->GetSize;
    my @size = ($size->GetWidth, $size->GetHeight);
    

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
    $dc->DrawLabel(sprintf('%.4g', $self->{min_layer_height}), Wx::Rect->new(0, $size[1]/2, $size[0], $size[1]/2), wxALIGN_LEFT | wxALIGN_BOTTOM);
    $dc->DrawLabel(sprintf('%.4g', $self->{max_layer_height}), Wx::Rect->new(0, $size[1]/2, $size[0], $size[1]/2), wxALIGN_RIGHT | wxALIGN_BOTTOM);


    # draw original layers as lines
    my $last_z = 0.0;
    my @points = ();
    foreach my $z (@{$self->{original_interpolated_layers}}) {
        my $layer_h = $z - $last_z;
        $dc->SetPen($self->{original_pen});
        my $pl = $self->point_to_pixel(0, $z);
        my $pr = $self->point_to_pixel($layer_h, $z);
        $dc->DrawLine($pl->x, $pl->y, $pr->x, $pr->y);
        push (@points, $pr);
        $last_z = $z;
    }

    $dc->DrawSpline(\@points);       
    
#    # draw interactive (user modified) layers as lines
    $last_z = 0.0;
    @points = ();
    if($self->{interactive_heights}) {
    	foreach my $i (0..@{$self->{interactive_heights}}-1) {
	        my $z = $self->{original_layers}[$i];
	        my $layer_h = $self->{interactive_heights}[$i];
	        $dc->SetPen($self->{interactive_pen});
	        my $pl = $self->point_to_pixel(0, $z);
	        my $pr = $self->point_to_pixel($layer_h, $z);
	        $dc->DrawLine($pl->x, $pl->y, $pr->x, $pr->y);
	        push (@points, $pr);
	    }
	
	    $dc->DrawSpline(\@points);
    }

 
    # draw resulting layers as lines
    $last_z = 0.0;
    @points = ();
    foreach my $z (@{$self->{interpolated_layers}}) {
        my $layer_h = $z - $last_z;
        $dc->SetPen($self->{resulting_pen});
        my $pl = $self->point_to_pixel(0, $z);
        my $pr = $self->point_to_pixel($layer_h, $z);
        $dc->DrawLine($pl->x, $pl->y, $pr->x, $pr->y);
        push (@points, $pr);
        $last_z = $z;
    }

#    $dc->DrawSpline(\@points);

    # Draw current BSpline
    $dc->SetPen($self->{line_pen});
    @points = ();
    foreach my $pixel (0..$size[1]) {
    	my @z = $self->pixel_to_point(Wx::Point->new(0, $pixel));
    	#print "z: " . $z . "\n";
    	my $h = $self->{object}->layer_height_spline->getLayerHeightAt($z[1]);
    	my $p = $self->point_to_pixel($h, $z[1]);
    	push (@points, $p);
    	#print "pixel: " . $pixel . "\n";
    }
    $dc->DrawLines(\@points);
    
    $event->Skip;
}

sub mouse_event {
    my ($self, $event) = @_;
    
    my $pos = $event->GetPosition;
    my @obj_pos = $self->pixel_to_point($pos);
    
    if ($event->ButtonDown) {
    	if ($event->LeftDown) {
           # start dragging
           $self->{left_drag_start_pos} = $pos;
        }
        if ($event->RightDown) {
           # start dragging
           $self->{right_drag_start_pos} = $pos;
        }
    } elsif ($event->LeftUp) {
        if($self->{left_drag_start_pos}) {
    		if($self->{interactive_heights}) {
    			$self->{heights} = $self->{interactive_heights};
    			$self->{interactive_heights} = ();
    			# update spline database
    			$self->{object}->layer_height_spline->updateLayerHeights($self->{heights});
    			$self->{interpolated_layers} = $self->{object}->layer_height_spline->getInterpolatedLayers;
    		}
    		$self->Refresh;
    		$self->{object}->layer_height_spline->suppressUpdate;
    		$self->{on_layer_update}->(@{$self->{interpolated_layers}});
    	}
        $self->{left_drag_start_pos} = undef;
    } elsif ($event->RightUp) {
        if($self->{right_drag_start_pos}) {
            if($self->{interactive_heights}) {
                $self->{heights} = $self->{interactive_heights};
                $self->{interactive_heights} = ();
                # update spline database
                $self->{object}->layer_height_spline->updateLayerHeights($self->{heights});
                $self->{interpolated_layers} = $self->{object}->layer_height_spline->getInterpolatedLayers;
            }
            $self->Refresh;
            $self->{object}->layer_height_spline->suppressUpdate;
            $self->{on_layer_update}->(@{$self->{interpolated_layers}});
        }
        $self->{right_drag_start_pos} = undef;
    } elsif ($event->Dragging) {
        if($self->{left_drag_start_pos}) {
        
	        my @start_pos = $self->pixel_to_point($self->{left_drag_start_pos});
	        my $range = abs($start_pos[1] - $obj_pos[1]);

	        # compute updated interactive layer heights
	        $self->_interactive_quadratic_curve($start_pos[1], $obj_pos[0], $range);
	        $self->Refresh;
        } elsif($self->{right_drag_start_pos}) {
            my @start_pos = $self->pixel_to_point($self->{right_drag_start_pos});
            my $range = $obj_pos[1] - $start_pos[1];

            # compute updated interactive layer heights
            $self->_interactive_linear_curve($start_pos[1], $obj_pos[0], $range);
            $self->Refresh;
        }
    } elsif ($event->Moving) {
        if($self->{on_z_indicator}) {
            $self->{on_z_indicator}->($obj_pos[1]);
        }
    } elsif ($event->Leaving) {
        if($self->{on_z_indicator} && !$self->{left_drag_start_pos}) {
            $self->{on_z_indicator}->(undef);
        }
    }
}

# Set basic parameters for this control.
# min/max_layer_height are required to define the x-range, object_height is used to scale the y-range.
# Must be called if object selection changes.
sub set_size_parameters {
	my ($self, $min_layer_height, $max_layer_height, $object_height) = @_;
	
	$self->{min_layer_height} = $min_layer_height;
    $self->{max_layer_height} = $max_layer_height;
    $self->{object_height} = $object_height;
    
    $self->_update_canvas_size;
    $self->Refresh;
}

# Layers have been modified externally, re-initialize this control with new values
sub update {
	my $self = shift;

    $self->{original_layers} = $self->{object}->layer_height_spline->getOriginalLayers;
    $self->{original_interpolated_layers} = $self->{object}->layer_height_spline->getInterpolatedLayers;
    $self->{interpolated_layers} = $self->{object}->layer_height_spline->getInterpolatedLayers; # Initialize to current values

    # initialize height vector
    $self->{heights} = ();
    $self->{interactive_heights} = ();
    my $last_z = 0;
    foreach my $z (@{$self->{original_layers}}) {
        push (@{$self->{heights}}, $z - $last_z);
        $last_z = $z;
    }
    $self->Refresh;
}

# Callback to notify parent element if layers have changed and reslicing should be triggered
sub on_layer_update {
    my ($self, $cb) = @_;
    $self->{on_layer_update} = $cb;
}

# Callback to tell parent element at which z-position the mouse currently hovers to update indicator in 3D-view
sub on_z_indicator {
	my ($self, $cb) = @_;
    $self->{on_z_indicator} = $cb;
}

# Internal function to cache scaling factors
sub _update_canvas_size {
    my $self = shift;
    
    # when the canvas is not rendered yet, its GetSize() method returns 0,0
    my $canvas_size = $self->GetSize;
    my ($canvas_w, $canvas_h) = ($canvas_size->GetWidth, $canvas_size->GetHeight);
    return if $canvas_w == 0;

    my $size = $canvas_size;
    my @size = ($size->GetWidth, $size->GetHeight);
    
    $self->{scaling_factor_x} = $size[0]/($self->{max_layer_height} - $self->{min_layer_height});
    $self->{scaling_factor_y} = $size[1]/$self->{object_height};
}

sub _interactive_quadratic_curve {
	my ($self, $mod_z, $target_layer_height, $range) = @_;
	
	$self->{interactive_heights} = (); # reset interactive curve
	
	# iterate over original points provided by spline
	my $last_z = 0;
	foreach my $i (0..@{$self->{heights}}-1  ) {
		my $z = $self->{original_layers}[$i];
		my $layer_h = $self->{heights}[$i];
		my $quadratic_factor = $self->_quadratic_factor($mod_z, $range, $z);
		my $diff = $target_layer_height - $layer_h;
		$layer_h  += $diff * $quadratic_factor;
		push (@{$self->{interactive_heights}}, $layer_h);
	}
}

sub _interactive_linear_curve {
	my ($self, $mod_z, $target_layer_height, $range) = @_;

    $self->{interactive_heights} = (); # reset interactive curve
    my $from;
    my $to;

    if($range >= 0) {
        $from = $mod_z;
        $to = $mod_z + $range;
    }else{
        $from = $mod_z + $range;
        $to = $mod_z;
    }

    # iterate over original points provided by spline
    foreach my $i (0..@{$self->{heights}}-1  ) {
        if(($self->{original_layers}[$i]) >= $from && ($self->{original_layers}[$i]< $to)) {
            push (@{$self->{interactive_heights}}, $target_layer_height);
        }else{
           push (@{$self->{interactive_heights}}, $self->{heights}[$i]);
        }
    }
}

sub _quadratic_factor {
	my ($self, $fixpoint, $range, $value) = @_;
	
	# avoid division by zero
	$range = 0.00001 if $range <= 0;
	
	my $dist = abs($fixpoint - $value);
	my $x = $dist/$range; # normalize
	my $result = 1-($x*$x);
	
	return max(0, $result);
}

# Takes a 2-tupel [layer_height (x), height(y)] and converts it
# into a Wx::Point in scaled canvas coordinates
sub point_to_pixel {
	my ($self, @point) = @_;
	
	my $size = $self->GetSize;
    my @size = ($size->GetWidth, $size->GetHeight);
    
    my $x = ($point[0] - $self->{min_layer_height})*$self->{scaling_factor_x};
    my $y = $size[1] - $point[1]*$self->{scaling_factor_y}; # invert y-axis

    return Wx::Point->new(min(max($x, 0), $size[0]), min(max($y, 0), $size[1])); # limit to canvas size  
}

# Takes a Wx::Point in scaled canvas coordinates and converts it
# into a 2-tupel [layer_height (x), height(y)]
sub pixel_to_point {
    my ($self, $point) = @_;
    
    my $size = $self->GetSize;
    my @size = ($size->GetWidth, $size->GetHeight);
    
    my $x = $point->x/$self->{scaling_factor_x} + $self->{min_layer_height};
    my $y = ($size[1] - $point->y)/$self->{scaling_factor_y}; # invert y-axis
    
    return (min(max($x, $self->{min_layer_height}), $self->{max_layer_height}), min(max($y, 0), $self->{object_height})); # limit to object size and layer constraints 
}

1;
