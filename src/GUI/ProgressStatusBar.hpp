#ifndef PROGRESSSTATUSBAR_HPP
#define PROGRESSSTATUSBAR_HPP
#include <wx/event.h> 
#include <wx/statusbr.h>

namespace Slic3r { namespace GUI {

class ProgressStatusBar : public wxStatusBar {
public:
    /// Constructor stub from parent
    ProgressStatusBar(wxWindow* parent, int id) : wxStatusBar(parent, id) { }
};

}} // Namespace Slic3r::GUI
#endif // PROGRESSSTATUSBAR_HPP
