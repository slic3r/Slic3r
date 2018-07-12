#ifndef GUI_HPP
#define GUI_HPP
#include <wx/toplevel.h>
#include "MainFrame.hpp"
#include "Notifier.hpp"
#include <string>
#include <vector>
#include <stack>
#include <mutex>


#include "Preset.hpp"

namespace Slic3r { namespace GUI {


class App: public wxApp
{
public:
    virtual bool OnInit();
    App() : wxApp() {}

    /// Save position, size, and maximize state for a TopLevelWindow (includes Frames) by name in Settings.
    void save_window_pos(const wxTopLevelWindow* window, const wxString& name );

    /// Move/resize a named TopLevelWindow (includes Frames) from Settings
    void restore_window_pos(wxTopLevelWindow* window, const wxString& name );

    /// Function to add callback functions to the idle loop stack.
    void CallAfter(std::function<void()> cb_function); 


    void OnUnhandledException() override;

    std::vector<Presets> presets { preset_types, Presets() };
private:
    std::unique_ptr<Notifier> notifier {nullptr};

    void load_presets();

    wxString datadir {""};
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
