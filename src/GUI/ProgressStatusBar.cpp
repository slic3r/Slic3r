#include "ProgressStatusBar.hpp"

namespace Slic3r { namespace GUI {

void ProgressStatusBar::SendStatusText(wxEvtHandler* dest, wxWindowID origin, const wxString& msg) {
    wxQueueEvent(dest, new StatusTextEvent(EVT_STATUS_TEXT_POST, origin, msg));

}

}} // Namespace Slic3r::GUI
