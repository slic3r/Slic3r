package Slic3r::GUI::Preset;
use Moo;
use Unicode::Normalize;

has 'default'   => (is => 'ro', default => sub { 0 });
has 'external'  => (is => 'ro', default => sub { 0 });
has 'name'      => (is => 'rw', required => 1);
has 'file'      => (is => 'rw');

sub BUILD {
    my ($self) = @_;
    
    $self->name(Unicode::Normalize::NFC($self->name));
}

sub config {
    my ($self, $keys, $extra_keys) = @_;
    
    if ($self->default) {
        return Slic3r::Config->new_from_defaults(@$keys);
    } else {
        if (!-e Slic3r::encode_path($self->file)) {
            Slic3r::GUI::show_error(undef, "The selected preset does not exist anymore (" . $self->file . ").");
            return undef;
        }
        my $external_config = $self->load_config;
        if (!$keys) {
            return $external_config;
        } else {
            # apply preset values on top of defaults
            my $config = Slic3r::Config->new_from_defaults(@$keys);
            $config->set($_, $external_config->get($_))
                for grep $external_config->has($_), @$keys;
            
            # For extra_keys we don't populate defaults.
            if ($extra_keys) {
                $config->set($_, $external_config->get($_))
                    for grep $external_config->has($_), @$extra_keys;
            }
        
            return $config;
        }
    }
}

sub load_config {
    my ($self) = @_;
    
    return Slic3r::Config->load($self->file);
}

1;
