use Test::More tests => 2;
use strict;
use warnings;

BEGIN {
    use FindBin;
    use lib "$FindBin::Bin/../lib";
    use local::lib "$FindBin::Bin/../local-lib";
}

use List::Util qw(none);
use Slic3r;
use Slic3r::Geometry qw(epsilon);
use Slic3r::Test;

{
    my $config = Slic3r::Config->new_from_defaults;
    my $test = sub {
        my $print = Slic3r::Test::init_print('20mm_cube', config => $config);
        my %speeds_by_z = ();  # z => []
        Slic3r::GCode::Reader->new->parse(my $gcode = Slic3r::Test::gcode($print), sub {
            my ($self, $cmd, $args, $info) = @_;
            
            if ($cmd eq 'G1' && $info->{dist_E} > 0 && $info->{dist_XY} > 0) {
                $speeds_by_z{$self->Z} //= [];
                push @{ $speeds_by_z{$self->Z} }, $self->F/60;
            }
        });
        return %speeds_by_z;
    };
    
    {
        $config->set('perimeter_speed', 0);
        $config->set('external_perimeter_speed', 0);
        $config->set('infill_speed', 0);
        $config->set('support_material_speed', 0);
        $config->set('solid_infill_speed', 0);
        $config->set('first_layer_speed', '50%');
        $config->set('first_layer_height', 0.25);
        my %speeds_by_z = $test->();
        ok !!(none { $_ > $config->max_print_speed/2+&epsilon } @{ $speeds_by_z{$config->first_layer_height} }),
            'percent first_layer_speed is applied over autospeed';
    }
    
    {
        $config->set('first_layer_speed', 33);
        my %speeds_by_z = $test->();
        ok !!(none { $_ > $config->first_layer_speed } @{ $speeds_by_z{$config->first_layer_height} }),
            'absolute first_layer_speed overrides autospeed';
    }
}

__END__
