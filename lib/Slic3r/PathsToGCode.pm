package Slic3r::PathsToGCode;
use Moo;

use Slic3r::Geometry qw(X Y unscale);
use XML::SAX;

has 'start_gcode'       => (is => 'ro', default => sub {<<"EOF"});
G21 ; set units to millimeters
G90 ; use absolute coordinates
EOF
has 'end_gcode'         => (is => 'ro', default => sub {''});
has 'before_path_gcode' => (is => 'ro', default => sub {''});
has 'after_path_gcode'  => (is => 'ro', default => sub {''});
has 'placement'         => (is => 'ro', default => sub { 'align' });  # center/align
has 'origin'            => (is => 'ro', default => sub { Slic3r::Point->new(0,0) });  # scaled Slic3r::Point
has 'scale'             => (is => 'ro', default => sub { 1 });  # factor
has 'reorder'           => (is => 'ro', default => sub { 1 });  # bool
has 'start_near'        => (is => 'ro', default => sub { Slic3r::Point->new(0,0) });  # scaled Slic3r::Point

sub process_svg {
    my ($self, $svgfile, $gcodefile) = @_;
    
    open my $fh, '>', $gcodefile or die "Failed to open output file $gcodefile\n";
    print  $fh $self->start_gcode;
    
    my $parser = XML::SAX::ParserFactory->parser(
        Handler => (my $handler = Slic3r::PathsToGCode::SVGParser->new),
    );
    $parser->parse_uri($svgfile);
    my $collection = Slic3r::Polyline::Collection->new(@{ $handler->{_paths} });
    
    $collection->scale($self->scale);
    {
        my $bb = $collection->bounding_box;
        $collection->translate(-$bb->x_min + $self->origin->x, -$bb->y_min + $self->origin->y);  #))
        if ($self->placement eq 'center') {
            my $size = $bb->size;
            $collection->translate(-$size->[X]/2, -$size->[Y]/2);
        }
    }
    
    # reorder paths if requested
    my @polylines = ();
    if ($self->reorder) {
        my $c = $collection->chained_path_from($self->start_near, 0);
        @polylines = map $_->clone, @$c;
    } else {
        @polylines = @$collection;
    }
    
    foreach my $polyline (@polylines) {
        my @points = @$polyline;
        printf $fh "G1 X%.4f Y%.4f\n", map unscale($_), @$_ for shift @points;
        print $fh $self->before_path_gcode . "\n" if $self->before_path_gcode;
        printf $fh "G1 X%.4f Y%.4f\n", map unscale($_), @$_ for @points;
        print $fh $self->after_path_gcode . "\n" if $self->after_path_gcode;
    }
    
    close $fh;
}

package Slic3r::PathsToGCode::SVGParser;
use base qw(XML::SAX::Base);
use Image::SVG::Path qw();
use Slic3r::Geometry qw(X Y);

sub new {
    my $self = shift->SUPER::new(@_);
    $self->{_paths} = [];
    $self;
}

sub start_element {
    my ($self, $el) = @_;
    if ($el->{LocalName} eq 'path') {
        my $d = $el->{Attributes}{'{}d'}{Value};
        my @path_info = extract_path_info($d, {absolute => 1, no_shortcuts => 1});
        
        my @polylines = ();
        foreach my $event (@path_info) {
            if ($event->{type} eq 'moveto') {
                push @polylines, Slic3r::Polyline->new;
                $polylines[-1]->append(Slic3r::Point->new_scale($event->{point}[X], -$event->{point}[Y]));
            } elsif ($event->{type} =~ /cubic-bezier/ || $event->{type} eq 'line-to') {
                $polylines[-1]->append(Slic3r::Point->new_scale($event->{end}[X], -$event->{end}[Y]));
            } elsif ($event->{type} eq 'closepath') {
                $polylines[-1]->append($polylines[-1]->first_point);
            } else {
                die "Failed to parse path item " . $event->{type};
            }
        }
        push @{$self->{_paths}}, @polylines;
    } elsif ($el->{LocalName} eq 'polyline') {
        my @points = map { my @c = split /,/; Slic3r::Point->new_scale($c[0], -$c[1]) }
            split /\s+/, $el->{Attributes}{'{}points'}{Value};
        push @{$self->{_paths}}, Slic3r::Polyline->new(@points);
    }
}

## The following is temporarily extracted from Image::SVG::Path 
## waiting for my patch to be merged.

use Carp;
sub position_type
{
    my ($curve_type) = @_;
    if (lc $curve_type eq $curve_type) {
        return "relative";
    }
    elsif (uc $curve_type eq $curve_type) {
        return "absolute";
    }
    else {
        croak "I don't know what to do with '$curve_type'";
    }
}

sub add_coords
{
    my ($first_ref, $to_add_ref) = @_;
    $first_ref->[0] += $to_add_ref->[0];
    $first_ref->[1] += $to_add_ref->[1];
}

# The following regular expression splits the path into pieces
 
my $split_re = qr/(?:,|(?<!e)(?=-)|\s+)/;
 
sub extract_path_info
{
    my ($path, $options_ref) = @_;
    my $me = 'extract_path_info';
    if (! $path) {
        croak "$me: no input";
    }
    # Create an empty options so that we don't have to
    # keep testing whether the "options" string is defined or not
    # before trying to read a hash value from it.
    if ($options_ref) {
        if (ref $options_ref ne 'HASH') {
            croak "$me: second argument should be a hash reference";
        }
    }
    else {
        $options_ref = {};
    }
    if (! wantarray) {
        croak "$me: extract_path_info returns an array of values";
    }
    my $verbose = $options_ref->{verbose};
    if ($verbose) {
        print "$me: I am trying to split up '$path'.\n";
    }
    my @path_info;
    my $has_moveto = ($path =~ /^([Mm])\s*,?\s*([-0-9.,]+)(.*)$/);
    if (! $has_moveto) {
        croak "No moveto at start of path '$path'";
    }
    my ($moveto_type, $move_to, $curves) = ($1, $2, $3);
    if ($verbose) {
        print "$me: The initial moveto looks like '$moveto_type' '$move_to'.\n";
    }
    # Deal with the initial "move to" command.
    my $position = position_type ($moveto_type);
    my ($x, $y) = split $split_re, $move_to, 2;
    push @path_info, {
        type => 'moveto',
        position => $position,
        point => [$x, $y],
        svg_key => $moveto_type,
    };
    # Deal with the rest of the path.
    my @curves;
    while ($curves =~ /([cslqtahvz])\s*([-0-9.,e\s]*)/gi) {
        push @curves, [$1, $2];
    }
    if (@curves == 0) {
        croak "No curves found in '$curves' (full: $path)";
    }
    for my $curve_data (@curves) {
        my ($curve_type, $curve) = @$curve_data;
        $curve =~ s/^,//;
        my @numbers = split $split_re, $curve;
        if ($verbose) {
            print "Extracted numbers: @numbers\n";
        }
        if (uc $curve_type eq 'C') {
            my $expect_numbers = 6;
            if (@numbers % 6 != 0) {
                croak "Wrong number of values for a C curve " .
                    scalar @numbers . " in '$path'";
            }
            my $position = position_type ($curve_type);
            for (my $i = 0; $i < @numbers / 6; $i++) {
                my $offset = 6 * $i;
                my @control1 = @numbers[$offset + 0, $offset + 1];
                my @control2 = @numbers[$offset + 2, $offset + 3];
                my @end      = @numbers[$offset + 4, $offset + 5];
                # Put each of these abbreviated things into the list
                # as a separate path.
                push @path_info, {
                    type => 'cubic-bezier',
                    position => $position,
                    control1 => \@control1,
                    control2 => \@control2,
                    end => \@end,
                    svg_key => $curve_type,
                };
            }
        }
        elsif (uc $curve_type eq 'S') {
            my $expect_numbers = 4;
            if (@numbers % $expect_numbers != 0) {
                croak "Wrong number of values for an S curve " .
                    scalar @numbers . " in '$path'";
            }
            my $position = position_type ($curve_type);
            for (my $i = 0; $i < @numbers / $expect_numbers; $i++) {
                my $offset = $expect_numbers * $i;
                my @control2 = @numbers[$offset + 0, $offset + 1];
                my @end      = @numbers[$offset + 2, $offset + 3];
                push @path_info, {
                    type => 'shortcut-cubic-bezier',
                    position => $position,
                    control2 => \@control2,
                    end => \@end,
                    svg_key => $curve_type,
                };
            }
        }
        elsif (uc $curve_type eq 'L') {
            my $expect_numbers = 2;
            if (@numbers % $expect_numbers != 0) {
                croak "Wrong number of values for an L command " .
                    scalar @numbers . " in '$path'";
            }
            my $position = position_type ($curve_type);
            for (my $i = 0; $i < @numbers / $expect_numbers; $i++) {
                my $offset = $expect_numbers * $i;
                push @path_info, {
                    type => 'line-to',
                    position => $position,
                    end => [@numbers[$offset, $offset + 1]],
                    svg_key => $curve_type,
                }
            }
        }
        elsif (uc $curve_type eq 'Z') {
            if (@numbers > 0) {
                croak "Wrong number of values for a Z command " .
                    scalar @numbers . " in '$path'";
            }
            my $position = position_type ($curve_type);
            push @path_info, {
                type => 'closepath',
                position => $position,
                svg_key => $curve_type,
            }
        }
        else {
            croak "I don't know what to do with a curve type '$curve_type'";
        }
    }
    # Now sort it out if the user wants to get rid of the absolute
    # paths etc. 
     
    my $absolute = $options_ref->{absolute};
    my $no_shortcuts = $options_ref->{no_shortcuts};
    if ($absolute) {
        if ($verbose) {
            print "Making all coordinates absolute.\n";
        }
        my $abs_pos;
        my $previous;
        for my $element (@path_info) {
            if ($element->{type} eq 'moveto') {
                if ($element->{position} eq 'relative') {
                    my $ip = $options_ref->{initial_position};
                    if ($ip) {
                        if (ref $ip ne 'ARRAY' ||
                            scalar @$ip != 2) {
                            croak "The initial position supplied doesn't look like a pair of coordinates";
                        }
                        add_coords ($element->{point}, $ip);
                    }
                }
                $abs_pos = $element->{point};
            }
            elsif ($element->{type} eq 'line-to') {
                if ($element->{position} eq 'relative') {
                    add_coords ($element->{end},      $abs_pos);
                }
                $abs_pos = $element->{end};
            }
            elsif ($element->{type} eq 'cubic-bezier') {
                if ($element->{position} eq 'relative') {
                    add_coords ($element->{control1}, $abs_pos);
                    add_coords ($element->{control2}, $abs_pos);
                    add_coords ($element->{end},      $abs_pos);
                }
                $abs_pos = $element->{end};
            }
            elsif ($element->{type} eq 'shortcut-cubic-bezier') {
                if ($element->{position} eq 'relative') {
                    add_coords ($element->{control2}, $abs_pos);
                    add_coords ($element->{end},      $abs_pos);
                }
                if ($no_shortcuts) {
                    if (!$previous) {
                        die "No previous element";
                    }
                    if ($previous->{type} ne 'cubic-bezier') {
                        die "Bad previous element type $previous->{type}";
                    }
                    $element->{type} = 'cubic-bezier';
                    $element->{svg_key} = 'C';
                    $element->{control1} = [
                        2 * $abs_pos->[0] - $previous->{control2}->[0],
                        2 * $abs_pos->[1] - $previous->{control2}->[1],
                    ];
                }
                $abs_pos = $element->{end};
            }
            $element->{position} = 'absolute';
            $element->{svg_key} = uc $element->{svg_key};
            $previous = $element;
        }
    }
    return @path_info;
}

1;
