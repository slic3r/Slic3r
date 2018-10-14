#include "ProgressStatusBar.hpp"
#include "misc_ui.hpp"
namespace Slic3r { namespace GUI {

ProgressStatusBar::ProgressStatusBar(wxWindow* parent, int id) : wxStatusBar(parent, id) {
    this->prog->Hide();
    this->cancelbutton->Hide();

    this->SetFieldsCount(3);
    const int tmpWidths[] {-1, 150, 155}; // need to make the array ahead of time in C++
    this->SetStatusWidths(3, tmpWidths);

    // Assign events.
    this->Bind(wxEVT_TIMER, [=](wxTimerEvent& e){this->OnTimer(e);});
    this->Bind(wxEVT_SIZE, [=](wxSizeEvent& e){this->OnSize(e);});
    this->Bind(wxEVT_BUTTON, [=](wxCommandEvent& e) { this->cancel_cb(); this->cancelbutton->Hide();});
}

ProgressStatusBar::~ProgressStatusBar() {
    if (this->timer != nullptr) {
        if (this->timer->IsRunning())
            this->timer->Stop();
    }
}

/// wxPerl version of this used a impromptu hashmap and a loop
/// which more impractical here. 
/// Opportunity to refactor here.
void ProgressStatusBar::OnSize(wxSizeEvent &e) {

    // position 0 is reserved for status text
    // position 1 is cancel button
    // position 2 is prog

    {
        wxRect rect;
        this->GetFieldRect(1, rect);
        const auto& offset = ( wxGTK ? 1 : 0); // cosmetic 1px offset on wxgtk
        const auto& pos {wxPoint(rect.x + offset, rect.y + offset)};
        this->cancelbutton->Move(pos);
        this->cancelbutton->SetSize(rect.GetWidth() - offset, rect.GetHeight());
    }

    {
        wxRect rect;
        this->GetFieldRect(2, rect);
        const auto& offset = ( wxGTK ? 1 : 0); // cosmetic 1px offset on wxgtk
        const auto& pos {wxPoint(rect.x + offset, rect.y + offset)};
        this->prog->Move(pos);
        this->prog->SetSize(rect.GetWidth() - offset, rect.GetHeight());
    }
    e.Skip();
}

void ProgressStatusBar::SetProgress(size_t val) {
    if (!this->prog->IsShown()) {
        this->ShowProgress(true);
    }
    if (val == this->prog->GetRange()) {
        this->prog->SetValue(0);
        this->ShowProgress(false);
    } else {
        this->prog->SetValue(val);
    }
}

void ProgressStatusBar::OnTimer(wxTimerEvent& e) {
    if (this->prog->IsShown())
        this->timer->Stop();
    if (this->busy)
        this->prog->Pulse();
}

}} // Namespace Slic3r::GUI
