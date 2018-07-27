#include "misc_ui.hpp"
#include "utils.hpp"
#include <wx/stdpaths.h>
#include <wx/msgdlg.h>
#include <wx/arrstr.h>

#include <exception>
#include <stdexcept>
#include <regex>


namespace Slic3r { namespace GUI {


#ifdef SLIC3R_DEV
void check_version(bool manual)  {
}
#else
void check_version(bool manual)  {
}

#endif

const wxString var(const wxString& in) {
    // TODO replace center string with path to VAR in actual distribution later
    if (VAR_ABS) {
        return wxString(VAR_ABS_PATH) + "/" + in;
    } else {
        return bin() + wxString(VAR_REL) + "/" + in;
    }
}

const wxString bin() { 
    wxFileName f(wxStandardPaths::Get().GetExecutablePath());
    wxString appPath(f.GetPath());
    return appPath;
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

void show_error(wxWindow* parent, const wxString& message) {
    wxMessageDialog(parent, message, _("Error"), wxOK | wxICON_ERROR).ShowModal();
}

void show_info(wxWindow* parent, const wxString& message, const wxString& title = _("Notice")) {
    wxMessageDialog(parent, message, title, wxOK | wxICON_INFORMATION).ShowModal();
}

void fatal_error(wxWindow* parent, const wxString& message) {
    show_error(parent, message);
    throw std::runtime_error(message.ToStdString());
}

wxMenuItem* append_submenu(wxMenu* menu, const wxString& name, const wxString& help, wxMenu* submenu, int id, const wxString& icon) {
    auto* item {new wxMenuItem(menu, id, name, help)};
    
    set_menu_item_icon(item,icon);
    item->SetSubMenu(submenu);
    menu->Append(item);
    return item;
}

void set_menu_item_icon(wxMenuItem* item, const wxString& icon) {
    if (!icon.IsEmpty()) {
        wxBitmap ico;
        if(ico.LoadFile(var(icon), wxBITMAP_TYPE_PNG))  
            item->SetBitmap(ico);
        else 
            std::cerr<< var(icon) << " failed to load \n";
    }
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

std::vector<wxString> open_model(wxWindow* parent, wxWindow* top) {
    auto dialog {new wxFileDialog((parent != nullptr ? parent : top), _("Choose one or more files") + wxString(" (STL/OBJ/AMF/3MF):"), ".", "",
    MODEL_WILDCARD, wxFD_OPEN | wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST)};
    if (dialog->ShowModal() != wxID_OK) { 
        dialog->Destroy(); 
        return std::vector<wxString>();
    }
    std::vector<wxString> tmp;
    wxArrayString tmpout;
    dialog->GetPaths(tmpout);
    for (const auto& i : tmpout) {
        tmp.push_back(i);
    }
    dialog->Destroy(); 
    return tmp;
}



wxString trim_zeroes(wxString in) { return wxString(_trim_zeroes(in.ToStdString())); }


}} // namespace Slic3r::GUI

