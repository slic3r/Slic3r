#ifndef PLATER_HPP
#define PLATER_HPP
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

namespace Slic3r { namespace GUI {

class Plater : public wxPanel 
{
public:
    Plater(wxWindow* parent, const wxString& title);
    void add();

};

} } // Namespace Slic3r::GUI

#endif  // PLATER_HPP
