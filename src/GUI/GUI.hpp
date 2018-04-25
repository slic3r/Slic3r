#ifndef GUI_HPP
#define GUI_HPP
#include "MainFrame.hpp"

namespace Slic3r { namespace GUI {

enum class PresetID {
    PRINT = 0,
    FILAMENT = 1,
    PRINTER = 2
};

class App: public wxApp
{
public:
    virtual bool OnInit();
    App(std::shared_ptr<Settings> config) : wxApp(), gui_config(config) {}

    void check_version(bool manual) { /* stub */}

private:
    std::shared_ptr<Settings> gui_config; // GUI-specific configuration options
    Notifier* notifier;
    
};

}} // namespace Slic3r::GUI
#endif // GUI_HPP
