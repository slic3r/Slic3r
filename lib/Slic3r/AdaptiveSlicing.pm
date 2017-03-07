package Slic3r::AdaptiveSlicing;
use Moo;

use List::Util qw(min max);
use Math::Trig qw(asin acos deg2rad rad2deg);
use Slic3r::Geometry qw(X Y Z triangle_normal scale unscale);

# This constant essentially describes the volumetric error at the surface which is induced
# by stacking "elliptic" extrusion threads.
# It is empirically determined by
# 1. measuring the surface profile of printed parts to find
# the ratio between layer height and profile height and then
# 2. computing the geometric difference between the model-surface and the elliptic profile.
# [Link to detailed description follows]
use constant SURFACE_CONST => 0.18403;

# public
has 'mesh' => (is => 'ro', required => 1);
has 'size' => (is => 'ro', required => 1);

#private
has 'normal_z'  			=> (is => 'ro', default => sub { [] }); # facet_id => [normal];
has 'ordered_facets' 	=> (is => 'ro', default => sub { [] }); # id => [facet_id, min_z, max_z];
has 'current_facet' 	=> (is => 'rw');

sub BUILD {
    my $self = shift;
    
    my $facet_id = 0;
    $self->mesh->repair;
    my $facets = $self->mesh->facets;
    my $vertices = $self->mesh->vertices;
    my $normals = $self->mesh->normals;
    
    
    
    # generate facet normals
    for ($facet_id = 0; $facet_id <= $#{$facets}; $facet_id++) {
		my $normal = $normals->[$facet_id];
		my $normal_length = sqrt($normal->[0]**2 + $normal->[1]**2 + $normal->[2]**2);
		
		if($normal_length > 0) {
			$self->normal_z->[$facet_id] = $normal->[Z]/$normal_length;
		}else{  # facet with area = 0
			$self->normal_z->[$facet_id] = 0.01;#[0 ,0 ,0];
			#print "facet with normal 0. p1: " . $vertices->[$facets->[$facet_id]->[0]]->[Z] . " p2: " . $vertices->[$facets->[$facet_id]->[1]]->[Z] . " p3: " . $vertices->[$facets->[$facet_id]->[2]]->[Z] . "\n";
			#print "normal: " . $normal->[0] . ", " . $normal->[1] . ", " . $normal->[2] . "\n";
		}
    }
    
	
	# generate a list of facet_ids, containing maximum and minimum Z-Value of the facet, ordered by minimum Z
	my @sort_facets;

	for ($facet_id = 0; $facet_id <= $#{$facets}; $facet_id++) {
		my $a = $vertices->[$facets->[$facet_id]->[0]]->[Z];
		my $b = $vertices->[$facets->[$facet_id]->[1]]->[Z];
		my $c = $vertices->[$facets->[$facet_id]->[2]]->[Z];
		my $min_z = min($a, $b, $c);
		my $max_z = max($a, $b, $c);
		push @sort_facets, [$facet_id, scale $min_z, scale $max_z];	
	}		
	@sort_facets = sort {$a->[1] <=> $b->[1]} @sort_facets;
	for (my $i = 0; $i <= $#sort_facets; $i++) {
		$self->ordered_facets->[$i] = $sort_facets[$i];
	} 
	# initialize pointer to iterate over the object
	$self->current_facet(0);
}


# find height of the next layer by analyzing the angle of all facets intersecting the new layer
sub next_layer_height {
    my $self = shift;
    my ($z, $quality_factor, $min_height, $max_height) = @_;
    
    # factor must be between 0-1, 0 is highest quality, 1 highest print speed
    if($quality_factor < 0 or $quality_factor > 1) {
        die "Speed / Quality factor must be in the interval [0:1]";
    }

    my $delta_min = SURFACE_CONST * $min_height;
    my $delta_max = SURFACE_CONST * $max_height + 0.5*$max_height;
    
    my $scaled_quality_factor = $quality_factor * ($delta_max-$delta_min) + $delta_min;
    
    my $height = $max_height;
    my $first_hit = 0;
    
    # find all facets intersecting the slice-layer
    my $ordered_id = $self->current_facet;
    while ($ordered_id <= $#{$self->ordered_facets}) {
        
        # facet's minimum is higher than slice_z -> end loop
        if($self->ordered_facets->[$ordered_id]->[1] >= $z) {
            last;
        }
        
        # facet's maximum is higher than slice_z -> store the first event for next layer_height call to begin at this point
        if($self->ordered_facets->[$ordered_id]->[2] > $z) {
            # first event?
            if(!$first_hit) {
                $first_hit = 1;
                $self->current_facet($ordered_id);
            } 
            
            #skip touching facets which could otherwise cause small height values
            if($self->ordered_facets->[$ordered_id]->[2] <= $z+1)
            {
                $ordered_id++;
                next;
            }
            
            # compute layer-height for this facet and store minimum of all heights
            $height = min($height, $self->_facet_height($ordered_id, $scaled_quality_factor));
        }
        $ordered_id++;
    }
    # lower height limit due to printer capabilities
    $height = max($min_height, $height);
    
    
    # check for sloped facets inside the determined layer and correct height if necessary
    if($height > $min_height){
        while ($ordered_id <= $#{$self->ordered_facets}) {
            
            # facet's minimum is higher than slice_z + height -> end loop
            if($self->ordered_facets->[$ordered_id]->[1] >= ($z + scale $height)) {
                last;
            }
            
            #skip touching facets which could otherwise cause small height values
            if($self->ordered_facets->[$ordered_id]->[2] <= $z+1)
            {
                $ordered_id++;
                next;
            }
            
            # Compute new height for this facet and check against height.
            my $reduced_height = $self->_facet_height($ordered_id, $scaled_quality_factor);
            #$area_h = max($min_height, min($max_height, $area_h));
            
            my $z_diff = unscale ($self->ordered_facets->[$ordered_id]->[1] - $z);


#           # handle horizontal facets
#           if ($self->normal_z->[$self->ordered_facets->[$ordered_id]->[0]] > 0.999) {
#               Slic3r::debugf "cusp computation, height is reduced from %f", $height;
#               $height = $z_diff;
#               Slic3r::debugf "to %f due to near horizontal facet\n", $height;
#           }else{
            if( $reduced_height > $z_diff) {
                if($reduced_height < $height) {
                    Slic3r::debugf "adaptive layer computation: height is reduced from %f", $height;
                    $height = $reduced_height;
                    Slic3r::debugf "to %f due to higher facet\n", $height;
                }
            }else{
                Slic3r::debugf "adaptive layer computation: height is reduced from %f", $height;
                $height = $z_diff;
                Slic3r::debugf "to z-diff: %f\n", $height;
            }
#           }
            
            $ordered_id++;  
        }
        # lower height limit due to printer capabilities again
        $height = max($min_height, $height);
    }
    
    Slic3r::debugf "adaptive layer computation, layer-bottom at z:%f, quality_factor:%f, resulting layer height:%f\n", unscale $z, $quality_factor, $height;
    
    return $height; 
}



# computes the cusp height from a given facets normal and the cusp_value
sub _facet_cusp_height {
	my $self = shift;
	my ($ordered_id, $cusp_value) = @_;
	
	my $normal_z = $self->normal_z->[$self->ordered_facets->[$ordered_id]->[0]];
	my $cusp = ($normal_z == 0) ? 9999 : abs($cusp_value/$normal_z);
	return $cusp;
}


sub _facet_height {
	my ($self, $ordered_id, $scaled_quality_factor) = @_;
	
	my $normal_z = abs($self->normal_z->[$self->ordered_facets->[$ordered_id]->[0]]);
    my $height = $scaled_quality_factor/(SURFACE_CONST + $normal_z/2);
    return ($normal_z == 0) ? 9999 : $height;
}

# Returns the distance to the next horizontal facet in Z-dir 
# to consider horizontal object features in slice thickness
sub horizontal_facet_distance {
	my $self = shift;
	my ($z, $max_height) = @_;
	$max_height = scale $max_height;
	
	my $ordered_id = $self->current_facet;
	while ($ordered_id <= $#{$self->ordered_facets}) {
		
		# facet's minimum is higher than max forward distance -> end loop
		if($self->ordered_facets->[$ordered_id]->[1] > $z+$max_height) {
			last;
		}
		 
		# min_z == max_z -> horizontal facet
		if($self->ordered_facets->[$ordered_id]->[1] > $z) {
			if($self->ordered_facets->[$ordered_id]->[1] == $self->ordered_facets->[$ordered_id]->[2]) {
				return unscale($self->ordered_facets->[$ordered_id]->[1] - $z);
			}
		}
		
		$ordered_id++;
	}
	
	# objects maximum?
	if($z + $max_height > scale($self->size)) {
		return 	max($self->size - unscale($z), 0);
	}

	return unscale $max_height;
}

1;