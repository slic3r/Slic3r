package Slic3r::AdaptiveSlicing;
use Moo;

use List::Util qw(min max);
use Slic3r::Geometry qw(X Y Z triangle_normal scale unscale);

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
			$self->normal_z->[$facet_id] = [0 ,0 ,0];
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
	# initialize pointer for cusp_height run
	$self->current_facet(0);
}


sub cusp_height {
	my $self = shift;
	my ($z, $cusp_value, $min_height, $max_height) = @_;
	
	my $height = $max_height;
	my $first_hit = 0;
	
	# find all facets intersecting the slice-layer
	my $ordered_id = $self->current_facet;
	while ($ordered_id <= $#{$self->ordered_facets}) {
		
		# facet's minimum is higher than slice_z -> end loop
		if($self->ordered_facets->[$ordered_id]->[1] >= $z) {
			last;
		}
		
		# facet's maximum is higher than slice_z -> store the first event for next cusp_height call to begin at this point
		if($self->ordered_facets->[$ordered_id]->[2] > $z) {
			# first event?
			if(!$first_hit) {
				$first_hit = 1;
				$self->current_facet($ordered_id);
			} 
			
			#skip touching facets which could otherwise cause small cusp values
			if($self->ordered_facets->[$ordered_id]->[2] <= $z+1)
			{
				$ordered_id++;
				next;
			}
			
			# compute cusp-height for this facet and store minimum of all heights
			my $cusp = $self->_facet_cusp_height($ordered_id, $cusp_value);
			$height = $cusp if($cusp < $height);			
		}
		$ordered_id++;
	}
	# lower height limit due to printer capabilities
	$height = $min_height if($height < $min_height);
	
	
	# check for sloped facets inside the determined layer and correct height if necessary
	if($height > $min_height){
		while ($ordered_id <= $#{$self->ordered_facets}) {
			
			# facet's minimum is higher than slice_z + height -> end loop
			if($self->ordered_facets->[$ordered_id]->[1] >= ($z + scale $height)) {
				last;
			}
			
			#skip touching facets which could otherwise cause small cusp values
			if($self->ordered_facets->[$ordered_id]->[2] <= $z+1)
			{
				$ordered_id++;
				next;
			}
			
			# Compute cusp-height for this facet and check against height.
			my $cusp = $self->_facet_cusp_height($ordered_id, $cusp_value);
			
			my $z_diff = unscale ($self->ordered_facets->[$ordered_id]->[1] - $z);


			# handle horizontal facets
			if ($self->normal_z->[$self->ordered_facets->[$ordered_id]->[0]] > 0.999) {
				Slic3r::debugf "cusp computation, height is reduced from %f", $height;
				$height = $z_diff;
				Slic3r::debugf "to %f due to near horizontal facet\n", $height;
			}else{
				if( $cusp > $z_diff) {
					if($cusp < $height) {
						Slic3r::debugf "cusp computation, height is reduced from %f", $height;
						$height = $cusp;
						Slic3r::debugf "to %f due to new cusp height\n", $height;
					}
				}else{
					Slic3r::debugf "cusp computation, height is reduced from %f", $height;
					$height = $z_diff;
					Slic3r::debugf "to z-diff: %f\n", $height;
				}
			}
			
			$ordered_id++;	
		}
		# lower height limit due to printer capabilities again
		$height = $min_height if($height < $min_height);
	}
	
	Slic3r::debugf "cusp computation, layer-bottom at z:%f, cusp_value:%f, resulting layer height:%f\n", unscale $z, $cusp_value, $height;
	
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