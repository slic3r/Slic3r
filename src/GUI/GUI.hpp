#ifndef GUI_HPP
#define GUI_HPP
#include "MainFrame.hpp"

namespace Slic3r { namespace GUI {
class App: public wxApp
{
    std::shared_ptr<Settings> gui_config; // GUI-specific configuration options
public:
    virtual bool OnInit();
    App(std::shared_ptr<Settings> config) : wxApp(), gui_config(config) {}
};

}} // namespace Slic3r::GUI
#endif // GUI_HPP
