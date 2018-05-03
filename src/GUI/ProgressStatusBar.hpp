#ifndef PROGRESSSTATUSBAR_HPP
#define PROGRESSSTATUSBAR_HPP
#include <wx/event.h> 
#include <wx/statusbr.h>

namespace Slic3r { namespace GUI {

class StatusTextEvent : public wxEvent {
public:
    StatusTextEvent(wxEventType eventType, int winid, const wxString& msg) 
        : wxEvent(winid, eventType),
        message(msg) { }

    bool ShouldPropagate() const { return true; } // propagate this event

    /// One accessor
    const wxString& GetMessage() const {return message;}
    /// implement the base class pure virtual
    virtual wxEvent *Clone() const { return new StatusTextEvent(*this); }

private:
    const wxString message;
};

wxDEFINE_EVENT(EVT_STATUS_TEXT_POST, StatusTextEvent);

class ProgressStatusBar : public wxStatusBar {
public:
    //< Post an event to owning box and let it percolate up to a window that sets the appropriate status text.
    static void SendStatusText(wxEvtHandler* dest, wxWindowID origin, const wxString& msg);
    ProgressStatusBar(wxWindow* parent, int id) : wxStatusBar(parent, id) { }
};

}} // Namespace Slic3r::GUI
#endif // PROGRESSSTATUSBAR_HPP
