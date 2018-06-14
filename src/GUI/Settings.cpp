#include "Settings.hpp"
#include "misc_ui.hpp"

namespace Slic3r { namespace GUI {

std::unique_ptr<Settings> ui_settings {nullptr}; 

Settings::Settings() {
    // Initialize fonts
    _small_font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    if (the_os == OS::Mac) _small_font.SetPointSize(11);
    _small_bold_font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    if (the_os == OS::Mac) _small_bold_font.SetPointSize(11);
    _small_bold_font.MakeBold();
    _medium_font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    _medium_font.SetPointSize(12);

    _scroll_step = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetPointSize();
}
void Settings::save_settings() {
/*
sub save_settings {
    my ($self) = @_;
    Slic3r::Config->write_ini("$datadir/slic3r.ini", $Settings);
}

*/

}

void Settings::save_window_pos(wxWindow* ref, wxString name) {
}

void Settings::restore_window_pos(wxWindow* ref, wxString name) {
}
}} // namespace Slic3r::GUI
