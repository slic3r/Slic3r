#include "misc_ui.hpp"
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

}} // namespace Slic3r::GUI

