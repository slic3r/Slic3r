#ifndef PLATE2D_HPP
#define PLATE2D_HPP
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <vector>
#include <functional>
#include "Plater.hpp"
#include "ColorScheme.hpp"
#include "Settings.hpp"
#include "Plater/Plater2DObject.hpp"
#include "misc_ui.hpp"


#include "Log.hpp"


namespace Slic3r { namespace GUI {

enum class MoveDirection {
    Up, Down, Left, Right
};

class Plate2D : public wxPanel {
public:
    Plate2D(wxWindow* parent, const wxSize& size, std::vector<Plater2DObject>& _objects, std::shared_ptr<Model> _model, std::shared_ptr<Config> _config, std::shared_ptr<Settings> _settings);


//    std::function<> on_select_object {};
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
    size_t selected_instance;

    /// Handle mouse-move events
    void mouse_drag(wxMouseEvent& e);

    /// Handle repaint events
    void repaint(wxPaintEvent& e);

    void nudge_key(wxKeyEvent& e);

    void nudge(MoveDirection dir);

    /// Set/Update all of the colors used by the various brushes in the panel.
    void set_colors();

    /// Convert a scale point array to a pixel polygon suitable for DrawPolygon
    std::vector<wxPoint> scaled_points_to_pixel(std::vector<wxPoint> points, bool unscale);

    // For a specific point, unscaled it 
    wxPoint unscaled_point_to_pixel(const wxPoint& in) {
        const auto& canvas_height {this->GetSize().GetHeight()};
        const auto& zero = this->bed_origin;
        return wxPoint(in.x * this->scaling_factor + zero.x, 
                       in.y * this->scaling_factor + (zero.y - canvas_height));
    }

    /// Read print bed size from config.
    void update_bed_size();

    const std::string LogChannel {"GUI_2D"};

    wxPoint bed_origin {};
    Slic3r::Polygon bed_polygon {};

    /// How much to scale the points to fit in the draw bounding box area.
    /// Expressed as pixel / mm
    double scaling_factor {1.0};

};

} } // Namespace Slic3r::GUI
#endif // PLATE2D_HPP
