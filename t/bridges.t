use Test::More tests => 20;
use strict;
use warnings;

BEGIN {
    use FindBin;
    use lib "$FindBin::Bin/../lib";
    use local::lib "$FindBin::Bin/../local-lib";
}

use List::Util qw(first sum);
use Slic3r;
use Slic3r::Geometry qw(scale epsilon deg2rad rad2deg PI);
use Slic3r::Test;

{
    my $test = sub {
        my ($bridge_size, $rotate, $expected_angle, $tolerance) = @_;
    
        my ($x, $y) = @$bridge_size;
        my $lower = Slic3r::ExPolygon->new(
            Slic3r::Polygon->new_scale([-2,-2], [$x+2,-2], [$x+2,$y+2], [-2,$y+2]),
            Slic3r::Polygon->new_scale([0,0], [0,$y], [$x,$y], [$x,0]),
        );
        $lower->translate(scale 20, scale 20); # avoid negative coordinates for easier SVG preview
        $lower->rotate(deg2rad($rotate), [$x/2,$y/2]);
        my $bridge = $lower->[1]->clone;
        $bridge->reverse;
        $bridge = Slic3r::ExPolygon->new($bridge);
        
        ok check_angle([$lower], $bridge, $expected_angle, $tolerance), 'correct bridge angle for O-shaped overhang';
    };

    $test->([20,10], 0, 90);
    $test->([10,20], 0, 0);
    $test->([20,10], 45, 135, 20);
    $test->([20,10], 135, 45, 20);
}

{
    my $bridge = Slic3r::ExPolygon->new(
        Slic3r::Polygon->new_scale([0,0], [20,0], [20,10], [0,10]),
    );
    my $lower = [
        Slic3r::ExPolygon->new(
            Slic3r::Polygon->new_scale([-2,0], [0,0], [0,10], [-2,10]),
        ),
    ];
    $_->translate(scale 20, scale 20) for $bridge, @$lower; # avoid negative coordinates for easier SVG preview

    $lower->[1] = $lower->[0]->clone;
    $lower->[1]->translate(scale 22, 0);
    
    ok check_angle($lower, $bridge, 0), 'correct bridge angle for two-sided bridge';
}

{
    my $bridge = Slic3r::ExPolygon->new(
        Slic3r::Polygon->new_scale([0,0], [20,0], [10,10], [0,10]),
    );
    my $lower = [
        Slic3r::ExPolygon->new(
            Slic3r::Polygon->new_scale([0,0], [0,10], [10,10], [10,12], [-2,12], [-2,-2], [22,-2], [22,0]),
        ),
    ];
    $_->translate(scale 20, scale 20) for $bridge, @$lower; # avoid negative coordinates for easier SVG preview
    
    ok check_angle($lower, $bridge, 135), 'correct bridge angle for C-shaped overhang';
}

{
    my $bridge = Slic3r::ExPolygon->new(
        Slic3r::Polygon->new_scale([10,10],[20,10],[20,20], [10,20]),
    );
    my $lower = [
        Slic3r::ExPolygon->new(
            Slic3r::Polygon->new_scale([10,10],[10,20],[20,20],[20,30],[0,30],[0,10]),
        ),
    ];
    $_->translate(scale 20, scale 20) for $bridge, @$lower; # avoid negative coordinates for easier SVG preview
    
    ok check_angle($lower, $bridge, 45, undef, $bridge->area/2), 'correct bridge angle for square overhang with L-shaped anchors';
}

{
    # GH #2477: This test case failed when we computed coverage by summing length of centerlines
    # instead of summing their covered area.
    my $bridge = Slic3r::ExPolygon->new(
        Slic3r::Polygon->new([30299990,14299990],[1500010,14299990],[1500010,1500010],[30299990,1500010]),
    );
    my $lower = [
        Slic3r::ExPolygon->new(
            Slic3r::Polygon->new([31800000,15800000],[0,15800000],[0,0],[31800000,0]),
            Slic3r::Polygon->new([1499999,1500000],[1499999,14300000],[30300000,14300000],[30300000,1500000]),
        ),
    ];
    
    ok check_angle($lower, $bridge, 90, undef, $bridge->area, 500000),
        'correct bridge angle for rectangle';
}

{
    # GH #3929: This test case checks that narrow gaps in lower slices don't prevent correct
    # direction detection.
    my $bridge = Slic3r::ExPolygon->new(
        Slic3r::Polygon->new([10099996,45867519],[3762370,45867519],[3762370,2132479],[10099996,2132479]),
    );
    my $lower = [
        Slic3r::ExPolygon->new(
            Slic3r::Polygon->new([13534103,210089],[13629884,235753],[14249999,401901],[14269611,421510],[14272931,424830],[14287518,439411],[14484206,636101],[15348099,1500000],[15360812,1547449],[15365467,1564815],[15388623,1651235],[15391897,1663454],[15393088,1667906],[15399044,1690134],[15457593,1908648],[15750000,2999999],[15750000,45000000],[15742825,45026783],[15741540,45031580],[15735900,45052628],[15663980,45321047],[15348099,46500000],[15151410,46696691],[14287518,47560587],[14267907,47580196],[14264587,47583515],[14249999,47598100],[14211041,47608539],[14204785,47610215],[14176024,47617916],[14105602,47636784],[14097768,47638884],[14048000,47652220],[13871472,47699515],[12750000,48000000],[10446106,48000000],[10446124,47990347],[10446124,9652],[10446106,0],[12750000,0]),
            Slic3r::Polygon->new([10251886,5013],[10251886,47994988],[10251907,48000000],[10100006,48000000],[10100006,0],[10251907,0]),
            Slic3r::Polygon->new([3762360,17017],[3762360,47982984],[3762397,48000000],[1249999,48000000],[536029,47808700],[456599,47787419],[73471,47684764],[0,47665076],[0,23124327],[119299,22907322],[159278,22834601],[196290,22690451],[239412,22522516],[303787,22271780],[639274,20965103],[639274,19034896],[616959,18947983],[607651,18911729],[559146,18722807],[494769,18472073],[159278,17165397],[38931,16946491],[0,16875676],[0,334922],[128529,300484],[1250000,0],[3762397,0]),
        ),
    ];
    
    ok check_angle($lower, $bridge, 0, undef, $bridge->area, 500000),
        'correct bridge angle when lower slices have narrow gap';
}

sub check_angle {
    my ($lower, $bridge, $expected, $tolerance, $expected_coverage, $extrusion_width) = @_;
    
    if (ref($lower) eq 'ARRAY') {
        $lower = Slic3r::ExPolygon::Collection->new(@$lower);
    }
    
    $expected_coverage //= -1;
    $expected_coverage = $bridge->area if $expected_coverage == -1;
    $extrusion_width //= scale 0.5;
    
    my $bd = Slic3r::BridgeDetector->new($bridge, $lower, $extrusion_width);
    
    $tolerance //= rad2deg($bd->resolution) + epsilon;
    $bd->detect_angle;
    my $result = $bd->angle;
    my $coverage = $bd->coverage;
    is sum(map $_->area, @$coverage), $expected_coverage, 'correct coverage area';
    
    # our epsilon is equal to the steps used by the bridge detection algorithm
    ###use XXX; YYY [ rad2deg($result), $expected ];
    # returned value must be non-negative, check for that too
    my $delta = rad2deg($result) - $expected;
    $delta -= 180 if $delta >= 180 - epsilon;
    return defined $result && $result >= 0 && abs($delta) < $tolerance;
}

{
    my $config = Slic3r::Config->new_from_defaults;
    $config->set('top_solid_layers', 0);            # to prevent bridging on sparse infill
    $config->set('bridge_speed', 99);
    
    my $print = Slic3r::Test::init_print('bridge', config => $config);
    
    my %extrusions = ();  # angle => length
    Slic3r::GCode::Reader->new->parse(Slic3r::Test::gcode($print), sub {
        my ($self, $cmd, $args, $info) = @_;
        
        if ($cmd eq 'G1' && ($args->{F} // $self->F)/60 == $config->bridge_speed) {
            my $line = Slic3r::Line->new_scale(
                [ $self->X, $self->Y ],
                [ $info->{new_X}, $info->{new_Y} ],
            );
            my $angle = $line->direction;
            $extrusions{$angle} //= 0;
            $extrusions{$angle} += $line->length;
        }
    });
    ok !!%extrusions, "bridge is generated";
    my ($main_angle) = sort { $extrusions{$b} <=> $extrusions{$a} } keys %extrusions;
    is $main_angle, 0, "bridge has the expected direction";
}

__END__
