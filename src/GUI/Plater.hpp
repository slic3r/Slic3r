#ifndef PLATER_HPP
#define PLATER_HPP

namespace Slic3r { namespace GUI {

class Plater : public wxPanel 
{
public:
    Plater(wxWindow* parent, const wxString& title) : 
        wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, title)
    { }

};

} } // Namespace Slic3r::GUI

#endif  // PLATER_HPP
