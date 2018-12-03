package Slic3r::Surface;
use strict;
use warnings;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK   = qw(S_TYPE_TOP S_TYPE_BOTTOM S_TYPE_INTERNAL S_TYPE_SOLID S_TYPE_BRIDGE S_TYPE_VOID);
our %EXPORT_TAGS = (types => \@EXPORT_OK);

sub p {
    my $self = shift;
    return @{$self->polygons};
}

1;
