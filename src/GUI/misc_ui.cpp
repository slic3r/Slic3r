#include "misc_ui.hpp"
#include <wx/stdpaths.h>

namespace Slic3r { namespace GUI {


#ifdef SLIC3R_DEV
void check_version(bool manual)  {
}
#else
void check_version(bool manual)  {
}

#endif

const wxString var(const wxString& in) {
    wxFileName f(wxStandardPaths::Get().GetExecutablePath());
    wxString appPath(f.GetPath());

    // replace center string with path to VAR in actual distribution later
    return appPath + "/../var/" + in;
}

/// Returns the path to Slic3r's default user data directory.
const wxString home(const wxString& in) { 
    if (the_os == OS::Windows) 
        return wxGetHomeDir() + "/" + in + "/";
    return wxGetHomeDir() + "/." + in + "/";
}


wxString decode_path(const wxString& in) {
    // TODO Stub
    return in;
}

wxString encode_path(const wxString& in) {
    // TODO Stub
    return in;
}
/*
sub append_submenu {
    my ($self, $menu, $string, $description, $submenu, $id, $icon) = @_;
    
    $id //= &Wx::NewId();
    my $item = Wx::MenuItem->new($menu, $id, $string, $description // '');
    $self->set_menu_item_icon($item, $icon);
    $item->SetSubMenu($submenu);
    $menu->Append($item);
    
    return $item;
}
*/

/*
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
*/
/*
sub show_error {
    my ($parent, $message) = @_;
    Wx::MessageDialog->new($parent, $message, 'Error', wxOK | wxICON_ERROR)->ShowModal;
}
*/

}} // namespace Slic3r::GUI

