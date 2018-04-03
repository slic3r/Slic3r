use Test::More tests => 4;
use strict;
use warnings;

BEGIN {
    use FindBin;
    use lib "$FindBin::Bin/../lib";
    use local::lib "$FindBin::Bin/../local-lib";
}

use Slic3r;
use Slic3r::Test;

{
    my $config = Slic3r::Config->new_from_defaults;
    $config->set('perimeter_extrusion_width', '250%');
    ok $config->validate, 'percent extrusion width is validated';

    my $print = Slic3r::Test::init_print('20mm_cube', config => $config, scale => 2);
    {
        my $invalid = $print->apply_config($config);
        ok !($invalid), 're-applying same config does not invalidate';
    }
    $config->set('perimeters', 20);
    {
        my $invalid = $print->apply_config($config);
        ok $invalid, 're-applying with changed perimeters does invalidate previous config';
    }
    $config->set('fill_density', '75%');
    {
        my $invalid = $print->apply_config($config);
        ok $invalid, 're-applying with changed fill_density does invalidate previous config';
    }
}

__END__
