#ifndef GUI_HPP
#define GUI_HPP
#include "MainFrame.hpp"
#include "Notifier.hpp"
#include <string>
#include  <vector>

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

    void check_version(bool manual) { /* stub */}

private:
    std::shared_ptr<Settings> gui_config; // GUI-specific configuration options
    Notifier* notifier;
    std::vector<preset_list> presets { preset_list(), preset_list(), preset_list() };
};

}} // namespace Slic3r::GUI
#endif // GUI_HPP
