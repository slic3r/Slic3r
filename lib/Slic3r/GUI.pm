package Slic3r::GUI;
use strict;
use warnings;
use utf8;

use Wx 0.9901 qw(:bitmap :dialog :icon :id :misc :systemsettings :toplevelwindow
    :filedialog :font);
use Wx::Event qw(EVT_MENU);

BEGIN {
    # Wrap the Wx::_load_plugin() function which doesn't work with non-ASCII paths
    no warnings 'redefine';
    my $orig = *Wx::_load_plugin{CODE};
    *Wx::_load_plugin = sub {
       $_[0] = Slic3r::decode_path($_[0]);
       $orig->(@_);
    };
}

use File::Basename qw(basename);
use FindBin;
use List::Util qw(first any);
use Slic3r::Geometry qw(X Y);

use Slic3r::GUI::2DBed;
use Slic3r::GUI::AboutDialog;
use Slic3r::GUI::BedShapeDialog;
use Slic3r::GUI::BonjourBrowser;
use Slic3r::GUI::ConfigWizard;
use Slic3r::GUI::Controller;
use Slic3r::GUI::Controller::ManualControlDialog;
use Slic3r::GUI::Controller::PrinterPanel;
use Slic3r::GUI::MainFrame;
use Slic3r::GUI::Notifier;
use Slic3r::GUI::Plater;
use Slic3r::GUI::Plater::2D;
use Slic3r::GUI::Plater::2DToolpaths;
use Slic3r::GUI::Plater::3D;
use Slic3r::GUI::Plater::3DPreview;
use Slic3r::GUI::Plater::ObjectPartsPanel;
use Slic3r::GUI::Plater::ObjectCutDialog;
use Slic3r::GUI::Plater::ObjectSettingsDialog;
use Slic3r::GUI::Plater::LambdaObjectDialog;
use Slic3r::GUI::Plater::OverrideSettingsPanel;
use Slic3r::GUI::Plater::SplineControl;
use Slic3r::GUI::Preferences;
use Slic3r::GUI::ProgressStatusBar;
use Slic3r::GUI::Projector;
use Slic3r::GUI::OptionsGroup;
use Slic3r::GUI::OptionsGroup::Field;
use Slic3r::GUI::Preset;
use Slic3r::GUI::PresetEditor;
use Slic3r::GUI::PresetEditorDialog;
use Slic3r::GUI::SLAPrintOptions;
use Slic3r::GUI::ReloadDialog;

our $have_OpenGL = eval "use Slic3r::GUI::3DScene; 1";
our $have_LWP    = eval "use LWP::UserAgent; 1";

use Wx::Event qw(EVT_IDLE EVT_COMMAND);
use base 'Wx::App';

use constant FILE_WILDCARDS => {
    known   => 'Known files (*.stl, *.obj, *.amf, *.xml, *.3mf)|*.3mf;*.3MF;*.stl;*.STL;*.obj;*.OBJ;*.amf;*.AMF;*.xml;*.XML',
    stl     => 'STL files (*.stl)|*.stl;*.STL',
    obj     => 'OBJ files (*.obj)|*.obj;*.OBJ',
    amf     => 'AMF files (*.amf)|*.amf;*.AMF;*.xml;*.XML',
    tmf     => '3MF files (*.3mf)|*.3mf;*.3MF',
    ini     => 'INI files *.ini|*.ini;*.INI',
    gcode   => 'G-code files (*.gcode, *.gco, *.g, *.ngc)|*.gcode;*.GCODE;*.gco;*.GCO;*.g;*.G;*.ngc;*.NGC',
    svg     => 'SVG files *.svg|*.svg;*.SVG',
};
use constant MODEL_WILDCARD => join '|', @{&FILE_WILDCARDS}{qw(known stl obj amf tmf)};
use constant STL_MODEL_WILDCARD => join '|', @{&FILE_WILDCARDS}{qw(stl)};
use constant AMF_MODEL_WILDCARD => join '|', @{&FILE_WILDCARDS}{qw(amf)};
use constant TMF_MODEL_WILDCARD => join '|', @{&FILE_WILDCARDS}{qw(tmf)};

our $datadir;
# If set, the "Controller" tab for the control of the printer over serial line and the serial port settings are hidden.
our $autosave;
our $threads;
our @cb;

our $Settings = {
    _ => {
        version_check => 1,
        autocenter => 1,
        autoalignz => 1,
        invert_zoom => 0,
        background_processing => 0,
        threads => $Slic3r::Config::Options->{threads}{default},
        color_toolpaths_by => 'role',
        tabbed_preset_editors => 1,
        show_host => 0,
        nudge_val => 1,
        reload_hide_dialog => 0,
        reload_behavior => 0
    },
};

our $have_button_icons = &Wx::wxVERSION_STRING =~ / (?:2\.9\.[1-9]|3\.)/;
our $small_font = Wx::SystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
$small_font->SetPointSize(11) if &Wx::wxMAC;
our $small_bold_font = Wx::SystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
$small_bold_font->SetPointSize(11) if &Wx::wxMAC;
$small_bold_font->SetWeight(wxFONTWEIGHT_BOLD);
our $medium_font = Wx::SystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
$medium_font->SetPointSize(12);
our $grey = Wx::Colour->new(200,200,200);

# to use in ScrolledWindow::SetScrollRate(xstep, ystep)
# step related to system font point size
our $scroll_step = Wx::SystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT)->GetPointSize;

our $VERSION_CHECK_EVENT : shared = Wx::NewEventType;

our $DLP_projection_screen;

sub OnInit {
    my ($self) = @_;
    
    $self->SetAppName('Slic3r');
    Slic3r::debugf "wxWidgets version %s, Wx version %s\n", &Wx::wxVERSION_STRING, $Wx::VERSION;
    
    $self->{notifier} = Slic3r::GUI::Notifier->new;
    $self->{presets} = { print => [], filament => [], printer => [] };
    
    # locate or create data directory
    # Unix: ~/.Slic3r
    # Windows: "C:\Users\username\AppData\Roaming\Slic3r" or "C:\Documents and Settings\username\Application Data\Slic3r"
    # Mac: "~/Library/Application Support/Slic3r"
    $datadir ||= Slic3r::decode_path(Wx::StandardPaths::Get->GetUserDataDir);
    my $enc_datadir = Slic3r::encode_path($datadir);
    Slic3r::debugf "Data directory: %s\n", $datadir;
    
    # just checking for existence of $datadir is not enough: it may be an empty directory
    # supplied as argument to --datadir; in that case we should still run the wizard
    my $run_wizard = (-d $enc_datadir && -e "$enc_datadir/slic3r.ini") ? 0 : 1;
    foreach my $dir ($enc_datadir, "$enc_datadir/print", "$enc_datadir/filament", "$enc_datadir/printer") {
        next if -d $dir;
        if (!mkdir $dir) {
            my $error = "Slic3r was unable to create its data directory at $dir ($!).";
            warn "$error\n";
            fatal_error(undef, $error);
        }
    }
    
    # load settings
    my $last_version;
    if (-f "$enc_datadir/slic3r.ini") {
        my $ini = eval { Slic3r::Config->read_ini("$datadir/slic3r.ini") };
        if ($ini) {
            $last_version = $ini->{_}{version};
            $ini->{_}{$_} = $Settings->{_}{$_}
                for grep !exists $ini->{_}{$_}, keys %{$Settings->{_}};
            $Settings = $ini;
        }
        delete $Settings->{_}{mode};  # handle legacy
    }
    $Settings->{_}{version} = $Slic3r::VERSION;
    $Settings->{_}{threads} = $threads if $threads;
    $self->save_settings;
    
    if (-f "$enc_datadir/simple.ini") {
        # The Simple Mode settings were already automatically duplicated to presets
        # named "Simple Mode" in each group, so we already support retrocompatibility.
        unlink "$enc_datadir/simple.ini";
    }
    
    $self->load_presets;
    
    # application frame
    Wx::Image::AddHandler(Wx::PNGHandler->new);
    $self->{mainframe} = my $frame = Slic3r::GUI::MainFrame->new;
    $self->SetTopWindow($frame);
    
    # load init bundle
    {
        my @dirs = ($FindBin::Bin);
        if (&Wx::wxMAC) {
            push @dirs, qw();
        } elsif (&Wx::wxMSW) {
            push @dirs, qw();
        }
        my $init_bundle = first { -e $_ } map "$_/.init_bundle.ini", @dirs;
        if ($init_bundle) {
            Slic3r::debugf "Loading config bundle from %s\n", $init_bundle;
            $self->{mainframe}->load_configbundle($init_bundle, 1);
            $run_wizard = 0;
        }
    }
    
    if (!$run_wizard && (!defined $last_version || $last_version ne $Slic3r::VERSION)) {
        # user was running another Slic3r version on this computer
        if (!defined $last_version || $last_version =~ /^0\./) {
            show_info($self->{mainframe}, "Hello! Support material was improved since the "
                . "last version of Slic3r you used. It is strongly recommended to revert "
                . "your support material settings to the factory defaults and start from "
                . "those. Enjoy and provide feedback!", "Support Material");
        }
        if (!defined $last_version || $last_version =~ /^(?:0|1\.[01])\./) {
            show_info($self->{mainframe}, "Hello! In this version a new Bed Shape option was "
                . "added. If the bed coordinates in the plater preview screen look wrong, go "
                . "to Print Settings and click the \"Set\" button next to \"Bed Shape\".", "Bed Shape");
        }
    }
    $self->{mainframe}->config_wizard if $run_wizard;
    
    $self->check_version
        if $self->have_version_check
            && ($Settings->{_}{version_check} // 1)
            && (!$Settings->{_}{last_version_check} || (time - $Settings->{_}{last_version_check}) >= 86400);
    
    EVT_IDLE($frame, sub {
        while (my $cb = shift @cb) {
            $cb->();
        }
    });
    
    EVT_COMMAND($self, -1, $VERSION_CHECK_EVENT, sub {
        my ($self, $event) = @_;
        my ($success, $response, $manual_check) = @{$event->GetData};
        
        if ($success) {
            if ($response =~ /^obsolete ?= ?([a-z0-9.-]+,)*\Q$Slic3r::VERSION\E(?:,|$)/) {
                my $res = Wx::MessageDialog->new(undef, "A new version is available. Do you want to open the Slic3r website now?",
                    'Update', wxYES_NO | wxCANCEL | wxYES_DEFAULT | wxICON_INFORMATION | wxICON_ERROR)->ShowModal;
                Wx::LaunchDefaultBrowser('http://slic3r.org/') if $res == wxID_YES;
            } else {
                Slic3r::GUI::show_info(undef, "You're using the latest version. No updates are available.") if $manual_check;
            }
            $Settings->{_}{last_version_check} = time();
            $self->save_settings;
        } else {
            Slic3r::GUI::show_error(undef, "Failed to check for updates. Try later.") if $manual_check;
        }
    });
    
    return 1;
}

sub about {
    my ($self) = @_;
    
    my $about = Slic3r::GUI::AboutDialog->new(undef);
    $about->ShowModal;
    $about->Destroy;
}

# static method accepting a wxWindow object as first parameter
sub catch_error {
    my ($self, $cb, $message_dialog) = @_;
    if (my $err = $@) {
        $cb->() if $cb;
        $message_dialog
            ? $message_dialog->($err, 'Error', wxOK | wxICON_ERROR)
            : Slic3r::GUI::show_error($self, $err);
        return 1;
    }
    return 0;
}

# static method accepting a wxWindow object as first parameter
sub show_error {
    my ($parent, $message) = @_;
    Wx::MessageDialog->new($parent, $message, 'Error', wxOK | wxICON_ERROR)->ShowModal;
}

# static method accepting a wxWindow object as first parameter
sub show_info {
    my ($parent, $message, $title) = @_;
    Wx::MessageDialog->new($parent, $message, $title || 'Notice', wxOK | wxICON_INFORMATION)->ShowModal;
}

# static method accepting a wxWindow object as first parameter
sub fatal_error {
    show_error(@_);
    exit 1;
}

# static method accepting a wxWindow object as first parameter
sub warning_catcher {
    my ($self, $message_dialog) = @_;
    return sub {
        my $message = shift;
        return if $message =~ /GLUquadricObjPtr|Attempt to free unreferenced scalar/;
        my @params = ($message, 'Warning', wxOK | wxICON_WARNING);
        $message_dialog
            ? $message_dialog->(@params)
            : Wx::MessageDialog->new($self, @params)->ShowModal;
    };
}

sub notify {
    my ($self, $message) = @_;

    my $frame = $self->GetTopWindow;
    # try harder to attract user attention on OS X
    $frame->RequestUserAttention(&Wx::wxMAC ? wxUSER_ATTENTION_ERROR : wxUSER_ATTENTION_INFO)
        unless ($frame->IsActive);

    $self->{notifier}->notify($message);
}

sub save_settings {
    my ($self) = @_;
    Slic3r::Config->write_ini("$datadir/slic3r.ini", $Settings);
}

sub presets { return $_[0]->{presets}; }

sub load_presets {
    my ($self) = @_;
    
    for my $group (qw(printer filament print)) {
        my $presets = $self->{presets}{$group};
        
        # keep external or dirty presets
        @$presets = grep { ($_->external && $_->file_exists) || $_->dirty } @$presets;
        
        my $dir = "$Slic3r::GUI::datadir/$group";
        opendir my $dh, Slic3r::encode_path($dir)
            or die "Failed to read directory $dir (errno: $!)\n";
        foreach my $file (grep /\.ini$/i, readdir $dh) {
            $file = Slic3r::decode_path($file);
            my $name = basename($file);
            $name =~ s/\.ini$//i;
            
            # skip if we already have it
            next if any { $_->name eq $name } @$presets;
            
            push @$presets, Slic3r::GUI::Preset->new(
                group   => $group,
                name    => $name,
                file    => "$dir/$file",
            );
        }
        closedir $dh;
    
        @$presets = sort { $a->name cmp $b->name } @$presets;
    
        unshift @$presets, Slic3r::GUI::Preset->new(
            group   => $group,
            default => 1,
            name    => '- default -',
        );
    }
}

sub add_external_preset {
    my ($self, $file) = @_;
    
    my $name = basename($file);  # keep .ini suffix
    for my $group (qw(printer filament print)) {
        my $presets = $self->{presets}{$group};
        
        # remove any existing preset with the same name
        @$presets = grep { $_->name ne $name } @$presets;
        
        push @$presets, Slic3r::GUI::Preset->new(
            group    => $group,
            name     => $name,
            file     => $file,
            external => 1,
        );
    }
    return $name;
}

sub have_version_check {
    my ($self) = @_;
    
    # return an explicit 0
    return ($Slic3r::have_threads && $Slic3r::VERSION !~ /-dev$/ && $have_LWP) || 0;
}

sub check_version {
    my ($self, $manual_check) = @_;
    
    Slic3r::debugf "Checking for updates...\n";
    
    @_ = ();
    threads->create(sub {
        my $ua = LWP::UserAgent->new;
        $ua->timeout(10);
        my $response = $ua->get('http://slic3r.org/updatecheck');
        Wx::PostEvent($self, Wx::PlThreadEvent->new(-1, $VERSION_CHECK_EVENT,
            threads::shared::shared_clone([ $response->is_success, $response->decoded_content, $manual_check ])));
        
        Slic3r::thread_cleanup();
    })->detach;
}

sub output_path {
    my ($self, $dir) = @_;
    
    return ($Settings->{_}{last_output_path} && $Settings->{_}{remember_output_path})
        ? $Settings->{_}{last_output_path}
        : $dir;
}

sub open_model {
    my ($self, $window) = @_;
    
    my $dir = $Slic3r::GUI::Settings->{recent}{skein_directory}
           || $Slic3r::GUI::Settings->{recent}{config_directory}
           || '';
    
    my $dialog = Wx::FileDialog->new($window // $self->GetTopWindow, 'Choose one or more files (STL/OBJ/AMF/3MF):', $dir, "",
        MODEL_WILDCARD, wxFD_OPEN | wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST);
    if ($dialog->ShowModal != wxID_OK) {
        $dialog->Destroy;
        return;
    }
    my @input_files = map Slic3r::decode_path($_), $dialog->GetPaths;
    $dialog->Destroy;
    
    return @input_files;
}

sub CallAfter {
    my ($self, $cb) = @_;
    push @cb, $cb;
}

sub scan_serial_ports {
    my ($self) = @_;
    
    my @ports = ();
    
    if ($^O eq 'MSWin32') {
        # Windows
        if (eval "use Win32::TieRegistry; 1") {
            my $ts = Win32::TieRegistry->new("HKEY_LOCAL_MACHINE\\HARDWARE\\DEVICEMAP\\SERIALCOMM",
                { Access => 'KEY_READ' });
            if ($ts) {
                # when no serial ports are available, the registry key doesn't exist and 
                # TieRegistry->new returns undef
                $ts->Tie(\my %reg);
                push @ports, sort values %reg;
            }
        }
    } else {
        # UNIX and OS X
        push @ports, glob '/dev/{ttyUSB,ttyACM,tty.,cu.,rfcomm}*';
    }
    
    return grep !/Bluetooth|FireFly/, @ports;
}

sub append_menu_item {
    my ($self, $menu, $string, $description, $cb, $id, $icon, $kind) = @_;
    
    $id //= &Wx::NewId();
    my $item = Wx::MenuItem->new($menu, $id, $string, $description // '', $kind // 0);
    $self->set_menu_item_icon($item, $icon);
    $menu->Append($item);
    
    EVT_MENU($self, $id, $cb);
    return $item;
}

sub append_submenu {
    my ($self, $menu, $string, $description, $submenu, $id, $icon) = @_;
    
    $id //= &Wx::NewId();
    my $item = Wx::MenuItem->new($menu, $id, $string, $description // '');
    $self->set_menu_item_icon($item, $icon);
    $item->SetSubMenu($submenu);
    $menu->Append($item);
    
    return $item;
}

sub set_menu_item_icon {
    my ($self, $menuItem, $icon) = @_;
    
    # SetBitmap was not available on OS X before Wx 0.9927
    if ($icon && $menuItem->can('SetBitmap')) {
        $menuItem->SetBitmap(Wx::Bitmap->new($Slic3r::var->($icon), wxBITMAP_TYPE_PNG));
    }
}

sub save_window_pos {
    my ($self, $window, $name) = @_;
    
    $Settings->{_}{"${name}_pos"}  = join ',', $window->GetScreenPositionXY;
    $Settings->{_}{"${name}_size"} = join ',', $window->GetSizeWH;
    $Settings->{_}{"${name}_maximized"}      = $window->IsMaximized;
    $self->save_settings;
}

sub restore_window_pos {
    my ($self, $window, $name) = @_;
    
    if (defined $Settings->{_}{"${name}_pos"}) {
        my $size = [ split ',', $Settings->{_}{"${name}_size"}, 2 ];
        $window->SetSize($size);
        
        my $display = Wx::Display->new->GetClientArea();
        my $pos = [ split ',', $Settings->{_}{"${name}_pos"}, 2 ];
        if (($pos->[X] + $size->[X]/2) < $display->GetRight && ($pos->[Y] + $size->[Y]/2) < $display->GetBottom) {
            $window->Move($pos);
        }
        $window->Maximize(1) if $Settings->{_}{"${name}_maximized"};
    }
}

1;
