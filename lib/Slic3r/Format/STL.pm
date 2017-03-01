package Slic3r::Format::STL;
use Moo;

sub read_file {
    my $self = shift;
    my ($file) = @_;
    
    my $model = Slic3r::Model->new;
    $model->read_stl($file);
    return $model;
}

sub write_file {
    my $self = shift;
    my ($file, $mesh, %params) = @_;
    
    $mesh = $mesh->mesh if $mesh->isa('Slic3r::Model');
    
    my $path = Slic3r::encode_path($file);
    
    $params{binary}
        ? $mesh->write_binary($path)
        : $mesh->write_ascii($path);
}

1;
