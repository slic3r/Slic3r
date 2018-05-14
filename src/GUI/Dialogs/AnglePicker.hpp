#ifndef ANGLEPICKER_HPP
#define ANGLEPICKER_HPP

#include <cmath>
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/slider.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>

namespace Slic3r { namespace GUI {

/// Dialog to produce a special dialog with both a slider for +/- 360 degree rotation and a text box.
/// Supports decimal numbers via integer scaling.
template <int scaling = 10000>
class AnglePicker : public wxDialog { 
public:
    AnglePicker(wxWindow* parent, const wxString& title, double initial_angle) : 
        wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE, wxString("AnglePicker")), _angle(scaling * initial_angle) {

            auto* lbl_min = new wxStaticText(this, wxID_ANY, wxString(L"-360°"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
            auto* lbl_max = new wxStaticText(this, wxID_ANY, wxString(L"360°"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
            auto* lbl_txt = new wxStaticText(this, wxID_ANY, wxString("Angle "), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
            
            auto btn_sizer = new wxBoxSizer(wxHORIZONTAL);
            btn_sizer->Add(
                new wxButton(this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize),
                0, 
                wxALL,
                10);
            btn_sizer->Add(
                new wxButton(this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize),
                0, 
                wxALL,
                10);

            
            this->slider = new wxSlider(this, wxID_ANY, _angle, -360*scaling, 360*scaling, wxDefaultPosition, wxSize(255, wxDefaultSize.y));
            this->manual_entry = new wxTextCtrl(this, wxID_ANY, wxString("") << angle(), wxDefaultPosition, wxDefaultSize);
            

            this->hsizer = new wxBoxSizer(wxHORIZONTAL); 
            this->hsizer->Add(lbl_min, wxALIGN_LEFT);
            this->hsizer->Add(this->slider, wxALIGN_CENTER);
            this->hsizer->Add(lbl_max, wxALIGN_RIGHT);

            auto text_sizer {new wxBoxSizer(wxHORIZONTAL)};
            text_sizer->Add(lbl_txt, 0);
            text_sizer->Add(this->manual_entry, 0);

            this->vsizer = new wxBoxSizer(wxVERTICAL); 
            
            this->vsizer->Add(this->hsizer, 0);
            this->vsizer->Add(text_sizer, 0);
            this->vsizer->Add(btn_sizer, 0, wxALIGN_CENTER);
            
            this->SetSizerAndFit(vsizer);

            this->Bind(wxEVT_SLIDER, [this](wxCommandEvent &e){ 
                wxString str;
                _angle = this->slider->GetValue();
                str.Printf("%f", this->angle());
                this->manual_entry->SetValue(str);
            });
            this->Bind(wxEVT_TEXT, [this](wxCommandEvent &e){ 
                wxString str {this->manual_entry->GetValue()};
                double value {0.0};
                if (str.ToDouble(&value))
                    if (value <= 360.0 && value >= -360.0)
                        this->slider->SetValue(value * scaling);
            });
    }
    double angle() { return double(_angle) / double(scaling) ; }

private:
    /// Scaled integer
    int _angle {0};

    wxSlider* slider {nullptr};
    wxTextCtrl* manual_entry {nullptr};
    wxSizer* hsizer {nullptr};
    wxSizer* vsizer {nullptr};

};

}}

#endif// ANGLEPICKER_HPP
