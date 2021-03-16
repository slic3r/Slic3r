#ifndef GUI_HPP
#define GUI_HPP
#include <wx/toplevel.h>
#include "MainFrame.hpp"
#include "Notifier.hpp"
#include <string>
#include <vector>
#include <array>
#include <stack>
#include <mutex>


#include "Preset.hpp"

namespace Slic3r { namespace GUI {


class App: public wxApp
{
public:
    /// If set to a file path, Slic3r will automatically export to it the active configuration whenever an option is changed or a preset or selected.
    wxString autosave {""};
    
    /// The directory where presets and config are stored. If empty, Slic3r will default to the location provided by wxWidgets.
    wxString datadir {""};
    
    virtual bool OnInit() override;
    App() : wxApp() {}

    /// Save position, size, and maximize state for a TopLevelWindow (includes Frames) by name in Settings.
    void save_window_pos(const wxTopLevelWindow* window, const wxString& name );

    /// Move/resize a named TopLevelWindow (includes Frames) from Settings
    void restore_window_pos(wxTopLevelWindow* window, const wxString& name );

    /// Function to add callback functions to the idle loop stack.
    void CallAfter(std::function<void()> cb_function); 


    void OnUnhandledException() override;

    preset_store presets { Presets() };
    std::array<wxString, preset_types> preset_ini { };
    Settings* settings() { return ui_settings.get(); }

private:
    std::unique_ptr<Notifier> notifier {nullptr};

    void load_presets();

    const std::string LogChannel {"APP"}; //< Which log these messages should go to.

    /// Lock to guard the callback stack
    std::mutex callback_register;
    
    /// callbacks registered to run during idle event.
    std::stack<std::function<void()> > cb {};
};


/// Quick reference to this app with its cast applied.
#define SLIC3RAPP (dynamic_cast<App*>(wxTheApp))


}} // namespace Slic3r::GUI
#endif // GUI_HPP
