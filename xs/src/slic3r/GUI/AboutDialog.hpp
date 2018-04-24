#ifndef slic3r_GUI_AboutDialog_hpp_
#define slic3r_GUI_AboutDialog_hpp_

#include "GUI.hpp"

namespace Slic3r { namespace GUI {

class AboutDialogLogo : public wxPanel
{
    public:
    AboutDialogLogo(wxWindow* parent);
    
    private:
    wxBitmap logo;
    void onRepaint(wxEvent &event);
};

class AboutDialog : public wxDialog
{
    public:
    AboutDialog();
    
    private:
    void onLinkClicked(wxHtmlLinkEvent &event);
    void onCloseDialog(wxEvent &);
};

} }

#endif

