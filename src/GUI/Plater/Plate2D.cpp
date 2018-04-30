#include "Plater/Plate2D.hpp"

// libslic3r includes
#include "Geometry.hpp"
#include "Log.hpp"

// wx includes
#include <wx/colour.h>
#include <wx/dcbuffer.h>

namespace Slic3r { namespace GUI {

Plate2D::Plate2D(wxWindow* parent, const wxSize& size, std::vector<Plater2DObject>& _objects, std::shared_ptr<Model> _model, std::shared_ptr<Config> _config, std::shared_ptr<Settings> _settings) :
    wxPanel(parent, wxID_ANY, wxDefaultPosition, size, wxTAB_TRAVERSAL), objects(_objects), model(_model), config(_config), settings(_settings)
{ 
    
    this->Bind(wxEVT_PAINT, [=](wxPaintEvent &e) { this->repaint(e); });
    this->Bind(wxEVT_MOTION, [=](wxMouseEvent &e) { this->mouse_drag(e); });
    if (user_drawn_background) {
        this->Bind(wxEVT_ERASE_BACKGROUND, [=](wxEraseEvent& e){ });
    }

    this->Bind(wxEVT_SIZE, [=](wxSizeEvent &e) { this->update_bed_size(); this->Refresh(); });

    // Bind the varying mouse events

    // Set the brushes
    set_colors();
}

void Plate2D::repaint(wxPaintEvent& e) {

    // Need focus to catch keyboard events.
    this->SetFocus();

    auto dc {new wxAutoBufferedPaintDC(this)};
    const auto& size = this->GetSize();


    if (this->user_drawn_background) {
        // On all systems the AutoBufferedPaintDC() achieves double buffering.
        // On MacOS the background is erased, on Windows the background is not erased 
        // and on Linux/GTK the background is erased to gray color.
        // Fill DC with the background on Windows & Linux/GTK.
        const auto& brush_background {wxBrush(this->settings->color->BACKGROUND255(), wxBRUSHSTYLE_SOLID)};
        const auto& pen_background {wxPen(this->settings->color->BACKGROUND255(), 1, wxPENSTYLE_SOLID)};
        dc->SetPen(pen_background);
        dc->SetBrush(brush_background);
        const auto& rect {this->GetUpdateRegion().GetBox()};
        dc->DrawRectangle(rect.GetLeft(), rect.GetTop(), rect.GetWidth(), rect.GetHeight());
    }

    // Draw bed
    {
        dc->SetPen(this->print_center_pen);
        dc->SetBrush(this->bed_brush);
        
        dc->DrawPolygon(scaled_points_to_pixel(this->bed_polygon, true), 0, 0);
    }


}

void Plate2D::mouse_drag(wxMouseEvent& e) {
    if (e.Dragging()) {
        Slic3r::Log::info(LogChannel, L"Mouse dragging");
    } else {
        Slic3r::Log::info(LogChannel, L"Mouse moving");
    }
}

void Plate2D::set_colors() {

    this->SetBackgroundColour(settings->color->BACKGROUND255());

    this->objects_brush.SetColour(settings->color->BED_OBJECTS());
    this->objects_brush.SetStyle(wxBRUSHSTYLE_SOLID);
    this->instance_brush.SetColour(settings->color->BED_INSTANCE());
    this->instance_brush.SetStyle(wxBRUSHSTYLE_SOLID);
    this->selected_brush.SetColour(settings->color->BED_SELECTED());
    this->selected_brush.SetStyle(wxBRUSHSTYLE_SOLID);
    this->dragged_brush.SetColour(settings->color->BED_DRAGGED());
    this->dragged_brush.SetStyle(wxBRUSHSTYLE_SOLID);
    this->bed_brush.SetColour(settings->color->BED_COLOR());
    this->bed_brush.SetStyle(wxBRUSHSTYLE_SOLID);
    this->transparent_brush.SetColour(wxColour(0,0,0));
    this->transparent_brush.SetStyle(wxBRUSHSTYLE_TRANSPARENT);

    this->grid_pen.SetColour(settings->color->BED_GRID());
    this->grid_pen.SetWidth(1);
    this->grid_pen.SetStyle(wxPENSTYLE_SOLID);
    this->print_center_pen.SetColour(settings->color->BED_CENTER());
    this->print_center_pen.SetWidth(1);
    this->print_center_pen.SetStyle(wxPENSTYLE_SOLID);
    this->clearance_pen.SetColour(settings->color->BED_CLEARANCE());
    this->clearance_pen.SetWidth(1);
    this->clearance_pen.SetStyle(wxPENSTYLE_SOLID);
    this->skirt_pen.SetColour(settings->color->BED_SKIRT());
    this->skirt_pen.SetWidth(1);
    this->skirt_pen.SetStyle(wxPENSTYLE_SOLID);
    this->dark_pen.SetColour(settings->color->BED_DARK());
    this->dark_pen.SetWidth(1);
    this->dark_pen.SetStyle(wxPENSTYLE_SOLID);

}

void Plate2D::nudge_key(wxKeyEvent& e) {
    const auto key = e.GetKeyCode();

    switch( key ) {
        case WXK_LEFT:
            this->nudge(MoveDirection::Left);
        case WXK_RIGHT:
            this->nudge(MoveDirection::Right);
        case WXK_DOWN:
            this->nudge(MoveDirection::Down);
        case WXK_UP:
            this->nudge(MoveDirection::Up);
        default:
            break; // do nothing
    }

}

void Plate2D::nudge(MoveDirection dir) {
    if (this->selected_instance < this->objects.size()) {
        auto i = 0U;
        for (auto& obj : this->objects) {
            if (obj.selected()) {
                if (obj.selected_instance != -1) {
                }
            }
            i++;
        }
    }
    if (selected_instance >= this->objects.size()) { 
        Slic3r::Log::warn(LogChannel, L"Nudge failed because there is no selected instance.");
        return; // Abort
    }
}

void Plate2D::update_bed_size() {
    const auto& canvas_size {this->GetSize()};
    const auto& canvas_w {canvas_size.GetWidth()};
    const auto& canvas_h {canvas_size.GetHeight()};
    if (canvas_w == 0) return; // Abort early if we haven't drawn canvas yet.

    this->bed_polygon = Slic3r::Polygon();
    const auto& polygon = bed_polygon;

    const auto& bb = bed_polygon.bounding_box();
    const auto& size = bb.size();

    this->scaling_factor = std::min(static_cast<double>(canvas_w) / unscale(size.x), 
    static_cast<double>(canvas_h) / unscale(size.y));

    assert(this->scaling_factor != 0);


}

} } // Namespace Slic3r::GUI
