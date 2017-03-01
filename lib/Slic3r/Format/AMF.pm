package Slic3r::Format::AMF;
use Moo;

sub read_file {
    my $self = shift;
    my ($file) = @_;
    
    my $model = Slic3r::Model->new;
    $model->read_amf($file);
    return $model;
}

1;
