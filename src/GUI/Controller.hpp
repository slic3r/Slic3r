#ifndef CONTROLLER_UI_HPP
#define CONTROLLER_UI_HPP

namespace Slic3r { namespace GUI {

class Controller : public wxPanel { 
public:
    Controller(wxWindow* parent, const wxString& title) : 
        wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, title)
    { }
};

}} // Namespace Slic3r::GUI

#endif // CONTROLLER_UI_HPP
