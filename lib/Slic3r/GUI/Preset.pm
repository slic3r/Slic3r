package Slic3r::GUI::Preset;
use Moo;

use Unicode::Normalize;
use Wx qw(:dialog :icon :id wxTheApp);

has 'group'     => (is => 'ro', required => 1);
has 'default'   => (is => 'ro', default => sub { 0 });
has 'external'  => (is => 'ro', default => sub { 0 });
has 'name'      => (is => 'rw', required => 1);
has 'file'      => (is => 'rw');
has '_config'   => (is => 'rw', default => sub { Slic3r::Config->new });
has '_dirty_config' => (is => 'ro', default => sub { Slic3r::Config->new });

sub BUILD {
    my ($self) = @_;
    
    $self->name(Unicode::Normalize::NFC($self->name));
}

sub _loaded {
    my ($self) = @_;
    
    return !$self->_config->empty;
}

sub dirty_options {
    my ($self) = @_;
    
    my @dirty = ();
    
    # Options present in both configs with different values:
    push @dirty, @{$self->_config->diff($self->_dirty_config)};
    
    # Overrides added to the dirty config:
    my @extra = $self->_group_class->overriding_options;
    push @dirty, grep { !$self->_config->has($_) && $self->_dirty_config->has($_) } @extra;
    # Overrides removed from the dirty config:
    push @dirty, grep { $self->_config->has($_) && !$self->_dirty_config->has($_) } @extra;
    
    return @dirty;
}

sub dirty {
    my ($self) = @_;
    
    return !!$self->dirty_options;
}

sub dropdown_name {
    my ($self) = @_;
    
    my $name = $self->name;
    $name .= " (modified)" if $self->dirty;
    return $name;
}

sub file_exists {
    my ($self) = @_;
    
    die "Can't call file_exists() on a non-file preset" if !$self->file;
    return -e Slic3r::encode_path($self->file);
}

sub rename {
    my ($self, $name) = @_;
    
    $self->name($name);
    $self->file(sprintf "$Slic3r::GUI::datadir/%s/%s.ini", $self->group, $name);
}

sub prompt_unsaved_changes {
    my ($self, $parent) = @_;
    
    if ($self->dirty) {
        my $name = $self->default ? 'Default preset' : "Preset \"" . $self->name . "\"";
        
        my $opts = '';
        foreach my $opt_key ($self->dirty_options) {
            my $opt = $Slic3r::Config::Options->{$opt_key};
            my $name = $opt->{full_label} // $opt->{label};
            if ($opt->{category}) {
                $name = $opt->{category} . " > $name";
            }
            $opts .= "- $name\n";
        }
        
        my $msg = sprintf "%s has unsaved changes:\n%s\nDo you want to save them?", $name, $opts;
        my $confirm = Wx::MessageDialog->new($parent, $msg,
            'Unsaved Changes', wxYES_NO | wxCANCEL | wxYES_DEFAULT | wxICON_QUESTION);
        $confirm->SetYesNoCancelLabels('Save', 'Discard', 'Cancel');
        my $res = $confirm->ShowModal;
        
        if ($res == wxID_CANCEL) {
            return 0;
        } elsif ($res == wxID_YES) {
            return $self->default ? $self->save_prompt($parent) : $self->save;
        } elsif ($res == wxID_NO) {
            $self->dismiss_changes;
            return 1;
        }
    }
    
    return 1;
}

sub save_prompt {
    my ($self, $parent) = @_;
    
    my $default_name = $self->default ? 'Untitled' : $self->name;
    $default_name =~ s/\.ini$//i;

    my $dlg = Slic3r::GUI::SavePresetWindow->new($parent,
        default => $default_name,
        values  => [ map $_->name, grep !$_->default && !$_->external, @{wxTheApp->presets->{$self->name}} ],
    );
    return 0 unless $dlg->ShowModal == wxID_OK;
    
    $self->save_as($dlg->get_name);
}

sub save {
    my ($self, $opt_keys) = @_;
    
    return $self->save_as($self->name, $opt_keys);
}

sub save_as {
    my ($self, $name, $opt_keys) = @_;
    
    $self->rename($name);
    
    if (!$self->file) {
        die "Calling save() without setting filename";
    }
    
    if ($opt_keys) {
        $self->_config->apply_only($self->_dirty_config, $opt_keys);
    } else {
        $self->_config->clear;
        $self->_config->apply($self->_dirty_config);
    }
    
    # unlink the file first to avoid problems on case-insensitive file systems
    unlink Slic3r::encode_path($self->file);
    $self->_config->save($self->file);
    wxTheApp->load_presets;
    
    return 1;
}

sub dismiss_changes {
    my ($self) = @_;
    
    $self->_dirty_config->clear;
    $self->_dirty_config->apply($self->_config);
}

sub delete {
    my ($self) = @_;
    
    die "Default config can't be deleted" if $self->default;
    die "External configs can't be deleted" if $self->external;
    
    # Otherwise wxTheApp->load_presets() will keep it
    $self->dismiss_changes;
    
    if ($self->file) {
        unlink Slic3r::encode_path($self->file) if $self->file_exists;
        $self->file(undef);
    }
}

# This returns the loaded config with the dirty options applied.
sub dirty_config {
    my ($self) = @_;
    
    $self->load_config if !$self->_loaded;
    
    return $self->_dirty_config->clone;
}

sub load_config {
    my ($self) = @_;
    
    return $self->_config if $self->_loaded;
    
    my @keys = $self->_group_class->options;
    my @extra_keys = $self->_group_class->overriding_options;
    
    if ($self->default) {
        $self->_config(Slic3r::Config->new_from_defaults(@keys));
    } elsif ($self->file) {
        if (!$self->file_exists) {
            Slic3r::GUI::show_error(undef, "The selected preset does not exist anymore (" . $self->file . ").");
            return undef;
        }
        my $external_config = Slic3r::Config->load($self->file);
        if (!@keys) {
            $self->_config($external_config);
        } else {
            # apply preset values on top of defaults
            my $config = Slic3r::Config->new_from_defaults(@keys);
            $config->set($_, $external_config->get($_))
                for grep $external_config->has($_), @keys;
            
            # For extra_keys we don't populate defaults.
            if (@extra_keys && !$self->external) {
                $config->set($_, $external_config->get($_))
                    for grep $external_config->has($_), @extra_keys;
            }
        
            $self->_config($config);
        }
    }
    
    $self->_dirty_config->apply($self->_config);
    
    return $self->_config;
}

sub _group_class {
    my ($self) = @_;
    
    return "Slic3r::GUI::PresetEditor::".ucfirst $self->group;
}

1;
