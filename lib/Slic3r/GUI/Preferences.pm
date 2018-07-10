# Preferences dialog, opens from Menu: File->Preferences

package Slic3r::GUI::Preferences;
use Wx qw(:dialog :id :misc :sizer :systemsettings wxTheApp);
use Wx::Event qw(EVT_BUTTON EVT_TEXT_ENTER);
use base 'Wx::Dialog';

sub new {
    my ($class, $parent) = @_;
    my $self = $class->SUPER::new($parent, -1, "Preferences", wxDefaultPosition, wxDefaultSize);
    $self->{values} = {};
    
    my $optgroup;
    $optgroup = Slic3r::GUI::OptionsGroup->new(
        parent  => $self,
        title   => 'General',
        on_change => sub {
            my ($opt_id) = @_;
            $self->{values}{$opt_id} = $optgroup->get_value($opt_id);
        },
        label_width => 200,
    );
    $optgroup->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(    # version_check
        opt_id      => 'version_check',
        type        => 'bool',
        label       => 'Check for updates',
        tooltip     => 'If this is enabled, Slic3r will check for updates daily and display a reminder if a newer version is available.',
        default     => $Slic3r::GUI::Settings->{_}{version_check} // 1,
        readonly    => !wxTheApp->have_version_check,
    ));
    $optgroup->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(    # remember_output_path
        opt_id      => 'remember_output_path',
        type        => 'bool',
        label       => 'Remember output directory',
        tooltip     => 'If this is enabled, Slic3r will prompt the last output directory instead of the one containing the input files.',
        default     => $Slic3r::GUI::Settings->{_}{remember_output_path},
    ));
    $optgroup->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(    # autocenter
        opt_id      => 'autocenter',
        type        => 'bool',
        label       => 'Auto-center parts (x,y)',
        tooltip     => 'If this is enabled, Slic3r will auto-center objects around the print bed center.',
        default     => $Slic3r::GUI::Settings->{_}{autocenter},
    ));
    $optgroup->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(    # autoalignz
        opt_id      => 'autoalignz',
        type        => 'bool',
        label       => 'Auto-align parts (z=0)',
        tooltip     => 'If this is enabled, Slic3r will auto-align objects z value to be on the print bed at z=0.',
        default     => $Slic3r::GUI::Settings->{_}{autoalignz},
    ));
    $optgroup->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(    # invert_zoom
        opt_id      => 'invert_zoom',
        type        => 'bool',
        label       => 'Invert zoom in previews',
        tooltip     => 'If this is enabled, Slic3r will invert the direction of mouse-wheel zoom in preview panes.',
        default     => $Slic3r::GUI::Settings->{_}{invert_zoom},
    ));
    $optgroup->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(    # background_processing
        opt_id      => 'background_processing',
        type        => 'bool',
        label       => 'Background processing',
        tooltip     => 'If this is enabled, Slic3r will pre-process objects as soon as they\'re loaded in order to save time when exporting G-code.',
        default     => $Slic3r::GUI::Settings->{_}{background_processing},
        readonly    => !$Slic3r::have_threads,
    ));
    $optgroup->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(    # threads
        opt_id      => 'threads',
        type        => 'i',
        label       => 'Threads',
        tooltip     => $Slic3r::Config::Options->{threads}{tooltip},
        default     => $Slic3r::GUI::Settings->{_}{threads},
    ));
    $optgroup->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(    # tabbed_preset_editors
        opt_id      => 'tabbed_preset_editors',
        type        => 'bool',
        label       => 'Display profile editors as tabs',
        tooltip     => 'When opening a profile editor, it will be shown in a dialog or in a tab according to this option.',
        default     => $Slic3r::GUI::Settings->{_}{tabbed_preset_editors},
    ));
    $optgroup->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(    # show_host
        opt_id      => 'show_host',
        type        => 'bool',
        label       => 'Show Controller Tab (requires restart)',
        tooltip     => 'Shows/Hides the Controller Tab. Requires a restart of Slic3r.',
        default     => $Slic3r::GUI::Settings->{_}{show_host},
    ));
    $optgroup->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(    # nudge_val
        opt_id      => 'nudge_val',
        type        => 's',
        label       => '2D plater nudge value',
        tooltip     => 'In 2D plater, Move objects using keyboard by nudge value of',
        default     => $Slic3r::GUI::Settings->{_}{nudge_val},
    ));
    $optgroup->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(    # reload hide dialog
        opt_id      => 'reload_hide_dialog',
        type        => 'bool',
        label       => 'Hide Dialog on Reload',
        tooltip     => 'When checked, the dialog on reloading files with added parts & modifiers is suppressed. The reload is performed according to the option given in \'Default Reload Behavior\'',
        default     => $Slic3r::GUI::Settings->{_}{reload_hide_dialog},
    ));
    $optgroup->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(    # default reload behavior
        opt_id      => 'reload_behavior',
        type        => 'select',
        label       => 'Default Reload Behavior',
        tooltip     => 'Choose the default behavior of the \'Reload from disk\' function regarding additional parts and modifiers.',
        labels      => ['Reload all','Reload main, copy added','Reload main, discard added'],
        values      => [0, 1, 2],
        default     => $Slic3r::GUI::Settings->{_}{reload_behavior},
        width       => 180,
    ));
    $optgroup->append_single_option_line(Slic3r::GUI::OptionsGroup::Option->new(    # colorscheme
        opt_id      => 'colorscheme',
        type        => 'select',
        label       => 'Color Scheme',
        tooltip     => 'Choose between color schemes - restart of Slic3r required.',
        labels      => ['Default','Solarized'], # add more schemes, if you want in ColorScheme.pm.
        values      => ['getDefault','getSolarized'], # add more schemes, if you want - those are the names of the corresponding function in ColorScheme.pm.
        default     => $Slic3r::GUI::Settings->{_}{colorscheme} // 'getDefault',
        width       => 180,
    ));
    
    my $sizer = Wx::BoxSizer->new(wxVERTICAL);
    $sizer->Add($optgroup->sizer, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 10);
    
    my $buttons = $self->CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    EVT_BUTTON($self, wxID_OK, sub { $self->_accept });
    $sizer->Add($buttons, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 10);
    
    $self->SetSizer($sizer);
    $sizer->SetSizeHints($self);
    
    return $self;
}

sub _accept {
    my $self = shift;
    
    if ($self->{values}{mode}) {
        Slic3r::GUI::warning_catcher($self)->("You need to restart Slic3r to make the changes effective.");
    }
    
    $Slic3r::GUI::Settings->{_}{$_} = $self->{values}{$_} for keys %{$self->{values}};
    wxTheApp->save_settings;
    
    $self->EndModal(wxID_OK);
    $self->Close;  # needed on Linux
}

1;
