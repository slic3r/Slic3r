#ifndef GUI_HPP
#define GUI_HPP
#include <wx/toplevel.h>
#include "MainFrame.hpp"
#include "Notifier.hpp"
#include <string>
#include <vector>


namespace Slic3r { namespace GUI {

// Friendly indices for the preset lists.
enum class PresetID {
    PRINT = 0,
    FILAMENT = 1,
    PRINTER = 2
};
using preset_list = std::vector<std::string>;

class App: public wxApp
{
public:
    virtual bool OnInit();
    App(std::shared_ptr<Settings> config) : wxApp(), gui_config(config) {}

    /// Save position, size, and maximize state for a TopLevelWindow (includes Frames) by name in Settings.
    void save_window_pos(const wxTopLevelWindow* window, const wxString& name );

    /// Move/resize a named TopLevelWindow (includes Frames) from Settings
    void restore_window_pos(wxTopLevelWindow* window, const wxString& name );

private:
    std::shared_ptr<Settings> gui_config; // GUI-specific configuration options
    std::unique_ptr<Notifier> notifier {nullptr};
    std::vector<preset_list> presets { preset_list(), preset_list(), preset_list() };

    void load_presets();

    wxString datadir {""};
    const std::string LogChannel {"GUI"}; //< Which log these messages should go to.
};

}} // namespace Slic3r::GUI
#endif // GUI_HPP
