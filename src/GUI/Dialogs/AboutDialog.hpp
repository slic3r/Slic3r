#ifndef ABOUTDIALOG_HPP
#define ABOUTDIALOG_HPP
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/settings.h>
#include <wx/colour.h>
#include <wx/html/htmlwin.h>
#include <wx/event.h>
#include <string>

#include "libslic3r.h"
#include "misc_ui.hpp"


#ifndef SLIC3R_BUILD_COMMIT
#define SLIC3R_BUILD_COMMIT (Unknown revision)
#endif 

#define VER1_(x) #x
#define VER_(x) VER1_(x)
#define BUILD_COMMIT VER_(SLIC3R_BUILD_COMMIT)

namespace Slic3r { namespace GUI {

const wxString build_date {__DATE__};
const wxString git_version {BUILD_COMMIT};

class AboutDialogLogo : public wxPanel {
private:
    wxBitmap logo;
public:
    AboutDialogLogo(wxWindow* parent);
    void repaint(wxPaintEvent& event);
};

class AboutDialog : public wxDialog {
public:
    /// Build and show the About popup.
    AboutDialog(wxWindow* parent);
};



}} // namespace Slic3r::GUI

#endif // ABOUTDIALOG_HPP
