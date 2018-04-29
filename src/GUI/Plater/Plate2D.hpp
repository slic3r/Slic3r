#ifndef PLATE2D_HPP
#define PLATE2D_HPP
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <vector>
#include "Plater.hpp"
#include "ColorScheme.hpp"
#include "Settings.hpp"
#include "Plater/Plater2DObject.hpp"
#include "misc_ui.hpp"


#include "Log.hpp"


namespace Slic3r { namespace GUI {

class Plate2D : public wxPanel {
public:
    Plate2D(wxWindow* parent, const wxSize& size, std::vector<Plater2DObject>& _objects, std::shared_ptr<Model> _model, std::shared_ptr<Config> _config, std::shared_ptr<Settings> _settings);
private:
    std::vector<Plater2DObject>& objects;
    std::shared_ptr<Slic3r::Model> model;
    std::shared_ptr<Slic3r::Config> config;
    std::shared_ptr<Settings> settings;

    // Different brushes to draw with
    wxBrush objects_brush {};
    wxBrush instance_brush {};
    wxBrush selected_brush {};
    wxBrush bed_brush {};
    wxBrush dragged_brush {};
    wxBrush transparent_brush {};

    wxPen grid_pen {};
    wxPen print_center_pen {};
    wxPen clearance_pen {};
    wxPen skirt_pen {};
    wxPen dark_pen {};

    bool user_drawn_background {(the_os == OS::Mac ? false : true)};
    Plater2DObject selected_instance;

    /// Handle mouse-move events
    void mouse_drag(wxMouseEvent& e);

    /// Handle repaint events
    void repaint(wxPaintEvent& e);

    /// Set/Update all of the colors used by the various brushes in the panel.
    void set_colors();
    
    const std::string LogChannel {"GUI_2D"};
};

} } // Namespace Slic3r::GUI
#endif // PLATE2D_HPP
