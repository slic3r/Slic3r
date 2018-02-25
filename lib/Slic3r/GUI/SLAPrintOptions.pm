package Slic3r::GUI::SLAPrintOptions;
use Wx qw(:dialog :id :misc :sizer :systemsettings wxTheApp);
use Wx::Event qw(EVT_BUTTON EVT_TEXT_ENTER);
use base qw(Wx::Dialog Class::Accessor);

__PACKAGE__->mk_accessors(qw(config));

sub new {
    my ($class, $parent) = @_;
    my $self = $class->SUPER::new($parent, -1, "SLA/DLP Print", wxDefaultPosition, wxDefaultSize);
    
    $self->config(Slic3r::Config::SLAPrint->new);
    $self->config->apply_dynamic(wxTheApp->{mainframe}->{plater}->config);
    
    # Set some defaults
    $self->config->set('infill_extrusion_width', 0.5)  if $self->config->infill_extrusion_width == 0;
    $self->config->set('support_material_extrusion_width', 1)  if $self->config->support_material_extrusion_width == 0;
    $self->config->set('perimeter_extrusion_width', 1) if $self->config->perimeter_extrusion_width == 0;
    
    my $sizer = Wx::BoxSizer->new(wxVERTICAL);
    my $new_optgroup = sub {
        my ($title) = @_;
        
        my $optgroup = Slic3r::GUI::ConfigOptionsGroup->new(
            parent      => $self,
            title       => $title,
            config      => $self->config,
            label_width => 200,
        );
        $sizer->Add($optgroup->sizer, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 10);
        return $optgroup;
    };
    {
        my $optgroup = $new_optgroup->('Layers');
        $optgroup->append_single_option_line('layer_height');
        $optgroup->append_single_option_line('first_layer_height');
    }
    {
        my $optgroup = $new_optgroup->('Infill');
        $optgroup->append_single_option_line('fill_density');
        $optgroup->append_single_option_line('fill_pattern');
        {
            my $line = $optgroup->create_single_option_line('perimeter_extrusion_width');
            $line->label('Shell thickness');
            my $opt = $line->get_options->[0];
            $opt->sidetext('mm');
            $opt->tooltip('Thickness of the external shell (both horizontal and vertical).');
            $optgroup->append_line($line);
        }
        {
            my $line = $optgroup->create_single_option_line('infill_extrusion_width');
            $line->label('Infill thickness');
            my $opt = $line->get_options->[0];
            $opt->sidetext('mm');
            $opt->tooltip('Thickness of the infill lines.');
            $optgroup->append_line($line);
        }
        $optgroup->append_single_option_line('fill_angle');
    }
    {
        my $optgroup = $new_optgroup->('Raft');
        $optgroup->append_single_option_line('raft_layers');
        $optgroup->append_single_option_line('raft_offset');
    }
    {
        my $optgroup = $new_optgroup->('Support Material');
        $optgroup->append_single_option_line('support_material');
        {
            my $line = $optgroup->create_single_option_line('support_material_spacing');
            $line->label('Pillars spacing');
            my $opt = $line->get_options->[0];
            $opt->tooltip('Max spacing between support material pillars.');
            $optgroup->append_line($line);
        }
        {
            my $line = $optgroup->create_single_option_line('support_material_extrusion_width');
            $line->label('Pillars diameter');
            my $opt = $line->get_options->[0];
            $opt->sidetext('mm');
            $opt->tooltip('Diameter of the cylindrical support pillars. 0.4mm and higher is supported.');
            $optgroup->append_line($line);
        }
    }
    
    
    my $buttons = $self->CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    EVT_BUTTON($self, wxID_OK, sub { $self->_accept });
    $sizer->Add($buttons, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 10);
    
    $self->SetSizer($sizer);
    $sizer->SetSizeHints($self);
    
    return $self;
}

sub _accept {
    my $self = shift;
    
    # validate config
    eval {
        die "Invalid shell thickness (must be greater than 0).\n"
            if $self->config->fill_density < 100 && $self->config->perimeter_extrusion_width == 0;
        die "Invalid infill thickness (must be greater than 0).\n"
            if $self->config->fill_density < 100 && $self->config->infill_extrusion_width == 0;
    };
    if ($@) {
        Slic3r::GUI::show_error($self, $@);
        return;
    }
    
    wxTheApp->{mainframe}->{slaconfig}->apply_static($self->config);
    $self->EndModal(wxID_OK);
    $self->Close;  # needed on Linux
    
    my $projector = Slic3r::GUI::Projector->new($self->GetParent);
            
    # this double invocation is needed for properly hiding the MainFrame
    $projector->Show;
    $projector->ShowModal;
    
    # TODO: diff the new config with the selected presets and prompt the user for
    # applying the changes to them.
}

1;
