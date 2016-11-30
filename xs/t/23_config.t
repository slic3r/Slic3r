#!/usr/bin/perl
use strict;
use warnings;

use Slic3r::XS;

use Test::More tests => 1;
{
    use Cwd qw(abs_path);
    use File::Basename qw(dirname);
    my $class = Slic3r::Config->new;
    my $path = abs_path($0);
    my $config = $class->_load(dirname($path)."/inc/22_config_bad_config_options.ini");
    ok 1, 'did not crash on reading invalid items in config';
}

__END__
