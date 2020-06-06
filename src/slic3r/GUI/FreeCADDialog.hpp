#ifndef slic3r_GUI_FreeCADDialog_hpp_
#define slic3r_GUI_FreeCADDialog_hpp_

#include <map>
#include <vector>

#include "GUI_App.hpp"
#include "GUI_Utils.hpp"
#include "MainFrame.hpp"
#include "wxExtensions.hpp"
#include <wx/wx.h>
#include <wx/stc/stc.h>
#include <wx/html/htmlwin.h>
#include <wx/textctrl.h>

namespace Slic3r { 
namespace GUI {

class FreeCADDialog : public DPIDialog
{

public:
    FreeCADDialog(GUI_App* app, MainFrame* mainframe);
    virtual ~FreeCADDialog() { }
    
protected:
    void closeMe(wxCommandEvent& event_args);
    void createSTC();
    void create_geometry(wxCommandEvent& event_args);
    void on_dpi_changed(const wxRect& suggested_rect) override;

    wxStyledTextCtrl* m_text;
    wxTextCtrl* m_errors;
    wxTextCtrl* m_help;
    MainFrame* main_frame;
    GUI_App* gui_app;
};

} // namespace GUI
} // namespace Slic3r

#endif
