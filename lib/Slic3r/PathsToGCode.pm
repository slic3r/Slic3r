package Slic3r::PathsToGCode;
use Moo;

use XML::SAX;

has 'start_gcode'   => (is => 'ro', default => sub {<<"EOF"});
G21 ; set units to millimeters
G90 ; use absolute coordinates
EOF
has 'end_gcode'     => (is => 'ro', default => sub {''});
has 'path_speed'    => (is => 'ro');  # mm/sec
has 'travel_speed'  => (is => 'ro');  # mm/sec
has 'placement'     => (is => 'ro', default => sub { 'align' });  # center/align
has 'origin'        => (is => 'ro', default => sub { Slic3r::Point->new(0,0) });  # scaled Slic3r::Point
has 'scale'         => (is => 'ro', default => sub { 1 });  # factor

sub process_svg {
    my ($self, $svgfile, $gcodefile) = @_;
    
    open my $fh, '>', $gcodefile or die "Failed to open output file $gcodefile\n";
    print  $fh $self->start_gcode;
    printf $fh "G1 F%d\n", $self->speed * 60 if $self->speed;
    
    my $parser = XML::SAX::ParserFactory->parser(
        Handler => (my $handler = Slic3r::PathsToGCode::SVGParser->new),
    );
    $parser->parse_uri($svgfile);
    my $collection = Slic3r::Polyline::Collection->new(@{ $handler->{_paths} });
    
    my $bb = $collection->bounding_box;
    if ($self->placement eq 'align') {
        
    }
    
    close $fh;
}

package Slic3r::PathsToGCode::SVGParser;
use base qw(XML::SAX::Base);
use Image::SVG::Path qw(extract_path_info);

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
                $polylines[-1]->append(Slic3r::Point->new_scale(@{ $event->{point} }));
            } elsif ($event->{type} =~ /cubic-bezier/ || $event->{type} eq 'line-to') {
                $polylines[-1]->append(Slic3r::Point->new_scale(@{ $event->{end} }));
            } elsif ($event->{type} eq 'closepath') {
                $polylines[-1]->append($polylines[-1]->first_point);
            } else {
                die "Failed to parse path item " . $event->{type};
            }
        }
        
        push @{$self->{_paths}}, @polylines;
    }
}

1;
