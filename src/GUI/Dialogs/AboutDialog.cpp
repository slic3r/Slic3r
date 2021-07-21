#include "Dialogs/AboutDialog.hpp"

namespace Slic3r { namespace GUI {

static void link_clicked(wxHtmlLinkEvent& e)
{
    wxLaunchDefaultBrowser(e.GetLinkInfo().GetHref());
    e.Skip(0);
}

AboutDialog::AboutDialog(wxWindow* parent) : wxDialog(parent, -1, _("About Slic3r"), wxDefaultPosition, wxSize(600, 460), wxCAPTION)
{ 
        auto* hsizer {new wxBoxSizer(wxHORIZONTAL)} ;

        auto* vsizer {new wxBoxSizer(wxVERTICAL)} ;

        // logo
        auto* logo {new AboutDialogLogo(this)};
        hsizer->Add(logo, 0, wxEXPAND | wxLEFT | wxRIGHT, 30);

        // title
        auto* title     { new wxStaticText(this, -1, "Slic3r", wxDefaultPosition, wxDefaultSize)};
        auto title_font { wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT)};

        title_font.SetWeight(wxFONTWEIGHT_BOLD);
        title_font.SetFamily(wxFONTFAMILY_ROMAN);
        title_font.SetPointSize(24);
        title->SetFont(title_font);

        vsizer->Add(title, 0, wxALIGN_LEFT | wxTOP, 30);

        // version
        
        auto* version {new wxStaticText(this, -1, wxString("Version ") + wxString(SLIC3R_VERSION), wxDefaultPosition, wxDefaultSize)};
        auto version_font { wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT) };
        version_font.SetPointSize((the_os == OS::Windows ? 9 : 11));
        version->SetFont(version_font);
        vsizer->Add(version, 0, wxALIGN_LEFT | wxBOTTOM, 10);

        // text

        wxString text {""};
        text << "<html>"
            << "<body>"
            << "Copyright &copy; 2011-2017 Alessandro Ranellucci. <br />" 
            << "<a href=\"https://slic3r.org/\">Slic3r</a> is licensed under the "
            << "<a href=\"https://www.gnu.org/licenses/agpl-3.0.html\">GNU Affero General Public License, version 3</a>."
            << "<br /><br /><br />"
            << "Contributions by Henrik Brix Andersen, Vojtech Bubnik, Nicolas Dandrimont, Mark Hindess, "
            << "Petr Ledvina, Joseph Lenox, Y. Sapir, Mike Sheldrake, Kliment Yanev and numerous others. "
            << "Manual by Gary Hodgson. Inspired by the RepRap community. <br />"
            << "Slic3r logo designed by Corey Daniels, <a href=\"http://www.famfamfam.com/lab/icons/silk/\">Silk Icon Set</a> designed by Mark James. "
            << "<br /><br />"
            << "Built on " << build_date << " at git version " << git_version << "."
            << "</body>"
            << "</html>";
        auto* html {new wxHtmlWindow(this, -1, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_NEVER)};
        html->SetBorders(2);
        html->SetPage(text);

        html->Bind(wxEVT_HTML_LINK_CLICKED, [=](wxHtmlLinkEvent& e){ link_clicked(e); });

        vsizer->Add(html, 1, wxEXPAND | wxALIGN_LEFT | wxRIGHT | wxBOTTOM, 20);
        // buttons
        auto buttons = this->CreateStdDialogButtonSizer(wxOK);
        this->SetEscapeId(wxID_CLOSE);

        vsizer->Add(buttons, 0, wxEXPAND | wxRIGHT | wxBOTTOM, 3);

        hsizer->Add(vsizer, 1, wxEXPAND, 0);
        this->SetSizer(hsizer);

    };

AboutDialogLogo::AboutDialogLogo(wxWindow* parent) : 
    wxPanel(parent, -1, wxDefaultPosition, wxDefaultSize) 
{ 
    this->logo = wxBitmap(var("Slic3r_192px.png"), wxBITMAP_TYPE_PNG);
    this->SetMinSize(wxSize(this->logo.GetWidth(), this->logo.GetHeight()));

    this->Bind(wxEVT_PAINT, [=](wxPaintEvent& e) { this->repaint(e);});
}

void AboutDialogLogo::repaint(wxPaintEvent& event)
{
    wxPaintDC dc {this};

    dc.SetBackgroundMode(wxPENSTYLE_TRANSPARENT);

    const wxSize size {this->GetSize()} ;
    const auto logo_w {this->logo.GetWidth()};
    const auto logo_h {this->logo.GetHeight()};

    dc.DrawBitmap(this->logo, (size.GetWidth() - logo_w) / 2, (size.GetHeight() - logo_h) / 2, 1);
    event.Skip();
}

}} // namespace Slic3r::GUI
