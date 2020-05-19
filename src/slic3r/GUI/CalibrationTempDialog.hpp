#ifndef slic3r_GUI_CalibrationTempDialog_hpp_
#define slic3r_GUI_CalibrationTempDialog_hpp_

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

class CalibrationTempDialog : public DPIDialog
{

public:
    CalibrationTempDialog(GUI_App* app, MainFrame* mainframe);
    virtual ~CalibrationTempDialog(){ if(gui_app!=nullptr) gui_app->change_calibration_dialog(this, nullptr);}
    
protected:
    void on_dpi_changed(const wxRect &suggested_rect) override;

private:
    void closeMe(wxCommandEvent& event_args);
    ModelObject* add_part(ModelObject* model_object, std::string input_file, Vec3d move, Vec3d scale = Vec3d{ 1,1,1 });
    void create_geometry_2(wxCommandEvent& event_args) { create_geometry(2); }
    void create_geometry_4(wxCommandEvent& event_args) { create_geometry(4); }
    void create_geometry(uint8_t nb_delta);
    wxPanel* create_header(wxWindow* parent, const wxFont& bold_font);

    wxHtmlWindow* html_viewer;
    MainFrame* main_frame;
    GUI_App* gui_app;

};

} // namespace GUI
} // namespace Slic3r

#endif
