#!/usr/bin/perl

use strict;
use warnings;

use Test::More;

#plan skip_all => 'temporarily disabled';

BEGIN {
    use FindBin;
    use lib "$FindBin::Bin/../../lib";
    use local::lib "$FindBin::Bin/../../local-lib";
}

use Slic3r::XS;

is(1, 1, 'Dummy test');

done_testing();

__END__
