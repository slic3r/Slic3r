package Slic3r::Format::OBJ;
use Moo;

sub read_file {
    my $self = shift;
    my ($file) = @_;
    
    my $model = Slic3r::Model->new;
    $model->read_obj($file);
    return $model;
}

1;
