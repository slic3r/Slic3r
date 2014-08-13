package Slic3r::AdaptiveSlicing;
use Moo;

use List::Util qw(min max);
use Slic3r::Geometry qw(X Y Z triangle_normal scale unscale);

#private
has 'normal'  			=> (is => 'ro', default => sub { [] }); # facet_id => [normal];
has 'ordered_facets' 	=> (is => 'ro', default => sub { [] }); # id => [facet_id, min_z, max_z];
has 'current_facet' 	=> (is => 'rw');

sub BUILD {
    my $self = shift;
    
    my $facet_id = 0; 
    my $facets = $self->mesh->facets;
    my $vertices = $self->mesh->vertices;
    
    # generate facet normals
	foreach my $facet (@{$facets}) {
		my $normal = triangle_normal(map $vertices->[$_], @$facet[-3..-1]);
		# normalize length
		my $normal_length = sqrt($normal->[0]**2 + $normal->[1]**2 + $normal->[2]**2);
		
		if($normal_length > 0) {
			$self->normal->[$facet_id] = [ map $normal->[$_]/$normal_length, (X,Y,Z) ];
		}else{  # facet with area = 0
			$self->normal->[$facet_id] = [0 ,0 ,0];
		}
		$facet_id++;
	}
	
	# generate a list of facet_ids, containing maximum and minimum Z-Value of the facet, ordered by minimum Z
	my @sort_facets;

	for ($facet_id = 0; $facet_id <= $#{$facets}; $facet_id++) {
		my $a = $vertices->[$facets->[$facet_id]->[0]]->[Z];
		my $b = $vertices->[$facets->[$facet_id]->[1]]->[Z];
		my $c = $vertices->[$facets->[$facet_id]->[2]]->[Z];
		my $min_z = min($a, $b, $c);
		my $max_z = max($a, $b, $c);
		push @sort_facets, [$facet_id, $min_z, $max_z];	
	}		
	@sort_facets = sort {$a->[1] <=> $b->[1]} @sort_facets;
	for (my $i = 0; $i <= $#sort_facets; $i++) {
		$self->ordered_facets->[$i] = $sort_facets[$i];
	} 
	# initialize pointer for cusp_height run
	$self->current_facet(0);
}


1;