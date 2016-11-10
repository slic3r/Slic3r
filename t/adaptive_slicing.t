use Test::More tests => 9;
use strict;
use warnings;

BEGIN {
    use FindBin;
    use lib "$FindBin::Bin/../lib";
}

use List::Util qw(first sum);
use Slic3r;
use Slic3r::Test qw(_eq);
use Slic3r::Geometry qw(Z PI scale unscale);

use Devel::Peek;

my $config = Slic3r::Config->new_from_defaults;

my $generate_gcode = sub {
    my ($conf) = @_;
    $conf ||= $config;
    
    my $print = Slic3r::Test::init_print('slopy_cube', config => $conf);
    
    my @z = ();
    my @increments = ();
    Slic3r::GCode::Reader->new->parse(Slic3r::Test::gcode($print), sub {
        my ($self, $cmd, $args, $info) = @_;
        
        if ($info->{dist_Z}) {
            push @z, 1*$args->{Z};
            push @increments, $info->{dist_Z};
        }
    });
    
    return (@z);
};

my $horizontal_feature_test = sub {
    my (@z) = $generate_gcode->();
    
    ok  (_eq($z[0], $config->get_value('first_layer_height') + $config->z_offset), 'first layer height.');
    
    ok  (_eq($z[1], $config->get_value('first_layer_height') + $config->get('max_layer_height')->[0] + $config->z_offset), 'second layer height.');
        
    cmp_ok((first { _eq($_, 10.0) } @z[1..$#z]), '>', 0, 'horizontal facet matched');
    
    1;
};

my $height_gradation_test = sub {
    my (@z) = $generate_gcode->();
    
    my $gradation = $config->get('adaptive_slicing_z_gradation');
    # +1 is a "dirty fix" to avoid rounding issues with the modulo operator...
    my @results = map {unscale((scale($_)+1) % scale($gradation))} @z;
    
    ok  (_eq(sum(@results), 0), 'layer z is multiple of gradation ' . $gradation );
    
    1;
};



my $print = Slic3r::Test::init_print('slopy_cube', config => $config);
$print->models->[0]->mesh->repair();
my $adaptive_slicing = Slic3r::AdaptiveSlicing->new(
	mesh => Slic3r::Test::mesh('slopy_cube'),
	size => 20
);


subtest 'max cusp_height limited by extruder capabilities' => sub {
    plan tests => 3;

	is  ($adaptive_slicing->cusp_height(scale 1, 0.2, 0.1, 0.15), 0.15, 'low');
	is  ($adaptive_slicing->cusp_height(scale 1, 0.2, 0.1, 0.4), 0.4, 'higher');
	is  ($adaptive_slicing->cusp_height(scale 1, 0.2, 0.1, 0.65), 0.65, 'highest');
};
  
subtest 'min cusp_height limited by extruder capabilities' => sub {
    plan tests => 3;

	is  ($adaptive_slicing->cusp_height(scale 4, 0.01, 0.1, 0.15), 0.1, 'low');
	is  ($adaptive_slicing->cusp_height(scale 4, 0.02, 0.2, 0.4), 0.2, 'higher');
	is  ($adaptive_slicing->cusp_height(scale 4, 0.01, 0.3, 0.65), 0.3, 'highest');
};

subtest 'correct cusp_height depending on the facet normals' => sub {
    plan tests => 3;

	ok  (_eq($adaptive_slicing->cusp_height(scale 1, 0.1, 0.1, 0.5), 0.5), 'limit');
	ok  (_eq($adaptive_slicing->cusp_height(scale 4, 0.1, 0.1, 0.5), 0.1414), '45deg facet, cusp_value: 0.1');
	ok  (_eq($adaptive_slicing->cusp_height(scale 4, 0.15, 0.1, 0.5), 0.2121), '45deg facet, cusp_value: 0.15');
};


# 2.92893 ist lower slope edge
# distance to slope must be higher than min extruder cap.
# slopes cusp height must be greater than the distance to the slope
ok  (_eq($adaptive_slicing->cusp_height(scale 2.798, 0.1, 0.1, 0.5), 0.1414), 'reducing cusp_height due to higher slopy facet');

# slopes cusp height must be smaller than the distance to the slope
ok  (_eq($adaptive_slicing->cusp_height(scale 2.6289, 0.15, 0.1, 0.5), 0.3), 'reducing cusp_height to z-diff');

subtest 'horizontal planes' => sub {
    plan tests => 3;

	print "facet_distance: " . $adaptive_slicing->horizontal_facet_distance(scale 1, 1.2) . "\n";
	ok  (_eq($adaptive_slicing->horizontal_facet_distance(scale 1, 1.2), 1.2), 'max_height limit');
	ok  (_eq($adaptive_slicing->horizontal_facet_distance(scale 8.5, 4), 1.5), 'normal horizontal facet');
	ok  (_eq($adaptive_slicing->horizontal_facet_distance(scale 17, 5), 3.0), 'object maximum');
};

# shrink current layer to fit another layer under horizontal facet
$config->set('start_gcode', '');  # to avoid dealing with the nozzle lift in start G-code
$config->set('z_offset', 0);

$config->set('adaptive_slicing', 1);
$config->set('first_layer_height', 0.42893); # to catch lower slope edge
$config->set('nozzle_diameter', [0.5]);
$config->set('min_layer_height', [0.1]);
$config->set('max_layer_height', [0.5]);
$config->set('cusp_value', [0.19]);
# slope height: 7,07107 (2.92893 to 10)

subtest 'shrink to match horizontal facets' => sub {
	plan tests => 3;
	$horizontal_feature_test->();
};
	
# widen current layer to match horizontal facet
$config->set('cusp_value', [0.1]);

subtest 'widen to match horizontal facets' => sub {
	plan tests => 3;
	$horizontal_feature_test->();
};

subtest 'layer height gradation' => sub {
	plan tests => 5;
	foreach my $gradation (0.001, 0.01, 0.02, 0.05, 0.08) {
		$config->set('adaptive_slicing_z_gradation', $gradation);
		$height_gradation_test->();
	}
};

__END__
