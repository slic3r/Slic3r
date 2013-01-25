package Slic3r::Format::OBJ;
use Moo;

use constant MERGE_DUPLICATE_VERTICES => 0;

sub read_file {
    my $self = shift;
    my ($file) = @_;
    
    Slic3r::open(\my $fh, '<', $file) or die "Failed to open $file\n";
    my $vertices            = [];
    my %unique_vertices     = ();  # cache for merging duplicate vertices
    my %vertices_map        = ();  # cache for merging duplicate vertices (obj_v_id => v_id)
    my $facets              = [];
    my $facets_materials    = [];
    my %materials           = (); # material_id => material_idx
    my $cur_material_idx    = undef;
    while (my $_ = <$fh>) {
        chomp;
        if (/^v ([^ ]+)\s+([^ ]+)\s+([^ ]+)/) {
            push @$vertices, [$1, $2, $3];
            if (MERGE_DUPLICATE_VERTICES) {
                my $key = "$1-$2-$3";
                if (!exists $unique_vertices{$key}) {
                    $unique_vertices{$key} = (scalar keys %vertices_map);
                }
                $vertices_map{$#$vertices} = $unique_vertices{$key};
            }
        } elsif (/^f (\d+).*? (\d+).*? (\d+).*?/) {
            if (MERGE_DUPLICATE_VERTICES) {
                push @$facets, [ map $vertices_map{$_}, $1-1, $2-1, $3-1 ];
            } else {
                push @$facets, [ $1-1, $2-1, $3-1 ];
            }
            push @$facets_materials, $cur_material_idx;
        } elsif (/^usemtl (.+)/) {
            $materials{$1} //= scalar(keys %materials);
            $cur_material_idx = $materials{$1};
        }
    }
    close $fh;
    
    my $model = Slic3r::Model->new;
    my $object = $model->add_object(vertices => $vertices);
    my $volume = $object->add_volume(
        facets           => $facets,
        facets_materials => $facets_materials,
        materials        => { reverse %materials },
    );
    $model->set_material($_) for keys %materials;use XXX; XXX $model;
    return $model;
}

1;
