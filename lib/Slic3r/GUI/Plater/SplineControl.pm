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

    $self->{line_pen}           = Wx::Pen->new(Wx::Colour->new(50,50,50), 1, wxSOLID);
    $self->{interactive_pen}    = Wx::Pen->new(Wx::Colour->new(255,0,0), 1, wxSOLID);
    $self->{resulting_pen}      = Wx::Pen->new(Wx::Colour->new(0,255,0), 1, wxSOLID);

    $self->{user_drawn_background} = $^O ne 'darwin';
    
    $self->{scaling_factor_x} = 1;
    $self->{scaling_factor_y} = 1;
    
    $self->{min_layer_height} = 0.1;
    $self->{max_layer_height} = 0.4;
    $self->{object_height} = 1.0;
    $self->{layer_points} = ();
    $self->{interactive_points} = ();
    $self->{resulting_points} = ();
    
    
    EVT_PAINT($self, \&repaint);
    EVT_ERASE_BACKGROUND($self, sub {}) if $self->{user_drawn_background};
    EVT_MOUSE_EVENTS($self, \&mouse_event);
    EVT_SIZE($self, sub {
        $self->update_canvas_size;
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
    
    
    # draw interpolated (user modified) layers as lines
    my $last_z = 0.0;
    my @points = ();
    foreach my $z (@{$self->{interactive_points}}) {
        my $layer_h = $z - $last_z;
        $dc->SetPen($self->{interactive_pen});
        my $pl = $self->point_to_pixel(0, $z);
        my $pr = $self->point_to_pixel($layer_h, $z);
        $dc->DrawLine($pl->x, $pl->y, $pr->x, $pr->y);
        push (@points, $pr);
        $last_z = $z;
    }

    $dc->DrawSpline(\@points);
    
    # draw current layers as lines
    $last_z = 0.0;
    @points = ();
    foreach my $z (@{$self->{layer_points}}) {
    	my $layer_h = $z - $last_z;
    	$dc->SetPen($self->{line_pen});
        my $pl = $self->point_to_pixel(0, $z);
        my $pr = $self->point_to_pixel($layer_h, $z);
        $dc->DrawLine($pl->x, $pl->y, $pr->x, $pr->y);
        push (@points, $pr);
        $last_z = $z;
    }

    $dc->DrawSpline(\@points);
    
    # draw resulting layers as lines
    $last_z = 0.0;
    @points = ();
    foreach my $z (@{$self->{resulting_points}}) {
        my $layer_h = $z - $last_z;
        $dc->SetPen($self->{resulting_pen});
        my $pl = $self->point_to_pixel(0, $z);
        my $pr = $self->point_to_pixel($layer_h, $z);
        $dc->DrawLine($pl->x, $pl->y, $pr->x, $pr->y);
        push (@points, $pr);
        $last_z = $z;
    }

    $dc->DrawSpline(\@points);
    
    $event->Skip;
}

sub mouse_event {
    my ($self, $event) = @_;
    
    my $pos = $event->GetPosition;
    my @obj_pos = $self->pixel_to_point($pos);
    
    if ($event->ButtonDown) {
    	if ($event->LeftDown) {
           # start dragging
           $self->{drag_start_pos} = $pos;
        }
    } elsif ($event->LeftUp) {
    	if($self->{drag_start_pos}) {
    		$self->{resulting_points} = $self->{interactive_points};
    		$self->Refresh;
    	}
        $self->{drag_start_pos} = undef;
    } elsif ($event->Dragging) {
    	print "dragging, pos: " . $pos->x . ":" . $pos->y . "\n";
        return if !$self->{drag_start_pos}; # concurrency problems
        
        my @start_pos = $self->pixel_to_point($self->{drag_start_pos});
        my $range = abs($start_pos[1] - $obj_pos[1]);
        
        # compute updated interactive layer heights
        $self->interactive_curve($start_pos[1], $obj_pos[0], $range);
        $self->Refresh;

    }# elsif ($event->Moving) {
#        my $cursor = wxSTANDARD_CURSOR;
#        if (defined first { $_->contour->contains_point($point) } map @$_, map @{$_->instance_thumbnails}, @{ $self->{objects} }) {
#            $cursor = Wx::Cursor->new(wxCURSOR_HAND);
#        }
#        $self->SetCursor($cursor);
#    }
}

# Internal function to cache scaling factors
sub update_canvas_size {
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

# Set basic parameters for this control.
# min/max_layer_height are required to define the x-range, object_height is used to scale the y-range.
# Must be called if object selection changes.
sub set_size_parameters {
	my ($self, $min_layer_height, $max_layer_height, $object_height) = @_;
	
	$self->{min_layer_height} = $min_layer_height;
    $self->{max_layer_height} = $max_layer_height;
    $self->{object_height} = $object_height;
    
    $self->update_canvas_size;
    $self->Refresh;
}

# Set the current layer height values as basis for user manipulation
sub set_layer_points {
	my ($self, @layer_points) = @_;

	$self->{layer_points} = [@layer_points];
	$self->{interactive_points} = [@layer_points]; # Initialize to current values
	$self->{resulting_points} = [@layer_points]; # Initialize to current values
	$self->Refresh;
}


sub interactive_curve {
	my ($self, $mod_z, $target_layer_height, $range) = @_;
	
	$self->{interactive_points} = (); # reset interactive curve
	
	my $z = 0.0;
	my $layer_h = $self->{resulting_points}[0];
	my $i = 0;
	# copy points which are not going to be modified
	while(($z+$self->{resulting_points}[$i] < $mod_z-$range) && ($i < @{$self->{resulting_points}})) {
		$layer_h = $self->{resulting_points}[$i] - $z;
		$z = $self->{resulting_points}[$i];
		push (@{$self->{interactive_points}}, $z);
		$i +=1;
	}
	
	print "last original z: " . $z . "\n";
	# interpolate next points
	while($z < $self->{object_height}) {
	   	$layer_h = $self->_interpolate_next_layer_h($z);
	   	my $diff = $target_layer_height - $layer_h;
	   	my $quadratic_factor = $self->_quadratic_factor($mod_z, $range, $z+$layer_h);
	   	$layer_h += $diff * $quadratic_factor;
	   	$z += $layer_h;
	   	push (@{$self->{interactive_points}}, $z);
	} 
	
	# remove top layer if n-1 is higher than object_height!!!
}

sub _interpolate_next_layer_h {
	my ($self, $z) = @_;
	my $layer_h = $self->{resulting_points}[0];
	my $array_size = @{$self->{resulting_points}};
	my $i = 1;
	# find current layer
	while(($self->{resulting_points}[$i] <= $z) && ($array_size-1 > $i)) {
		$i += 1;
	}
	if($i == 1) {return $layer_h}; # first layer, nothing to interpolate
	
	$layer_h = $self->{resulting_points}[$i] - $self->{resulting_points}[$i-1];
	my $tmp = $layer_h;
	# interpolate
	if($array_size-1 > $i) {
		my $next_layer_h = $self->{resulting_points}[$i+1] - $self->{resulting_points}[$i];
		$layer_h = $layer_h * ($self->{resulting_points}[$i] - $z)/$layer_h + $next_layer_h * min(1, ($z+$layer_h-$self->{resulting_points}[$i])/$next_layer_h);
	}
#	if(abs($layer_h - $tmp) > 0.001) {
#		print "z: " . $z . "\n";
#		my $layer_i = $self->{resulting_points}[$i] - $self->{resulting_points}[$i-1];
#		my $layer_i1 = $self->{resulting_points}[$i+1] - $self->{resulting_points}[$i];
#		print "layer i: " . $layer_i . " -> " . $self->{resulting_points}[$i] . "\n";
#		print "layer i+1: " . $layer_i1 . " -> " . $self->{resulting_points}[$i+1] . "\n";
#		print "layer_h_1: " . $tmp . " layer_h_2: " . $layer_h . "\n\n";
#	}
	return $layer_h;	
}

sub _quadratic_factor {
	my ($self, $fixpoint, $range, $value) = @_;
	
	# avoid division by zero
	$range = 0.00001 if $range <= 0;
	
	my $dist = abs($fixpoint - $value);
	my $x = $dist/$range; # normalize
	my $result = 1-($x*$x);
	
	print "fixpoint: " . $fixpoint . " range: " . $range . " value: " . $value . " result: " . $result . "\n";
	
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
