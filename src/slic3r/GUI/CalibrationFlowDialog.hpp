#ifndef slic3r_GUI_CalibrationFlowDialog_hpp_
#define slic3r_GUI_CalibrationFlowDialog_hpp_

#include <wx/wx.h>
#include <map>
#include <vector>

#include "GUI_App.hpp"
#include "GUI_Utils.hpp"
#include "MainFrame.hpp"
#include "wxExtensions.hpp"
#include <wx/html/htmlwin.h>

namespace Slic3r { 
namespace GUI {

class CalibrationFlowDialog : public DPIDialog
{

public:
    CalibrationFlowDialog(GUI_App* app, MainFrame* mainframe);
    
protected:
    void on_dpi_changed(const wxRect &suggested_rect) override;

private:
    void closeMe(wxCommandEvent& event_args);
    void create_geometry_10(wxCommandEvent& event_args);
    void create_geometry_2_5(wxCommandEvent& event_args);
    void add_part(ModelObject* model_object, std::string input_file, Vec3d move, Vec3d scale = Vec3d{ 1,1,1 });
    void create_geometry(float start, float delta);
    wxPanel* create_header(wxWindow* parent, const wxFont& bold_font);

    wxHtmlWindow* html_viewer;
    MainFrame* main_frame;
    GUI_App* gui_app;

};

} // namespace GUI
} // namespace Slic3r

#endif
