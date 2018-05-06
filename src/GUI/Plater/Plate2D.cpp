#include "Plater/Plate2D.hpp"

// libslic3r includes
#include "Geometry.hpp"
#include "Point.hpp"
#include "Log.hpp"
#include "ClipperUtils.hpp"

// wx includes
#include <wx/colour.h>
#include <wx/dcbuffer.h>

namespace Slic3r { namespace GUI {

Plate2D::Plate2D(wxWindow* parent, const wxSize& size, std::vector<PlaterObject>& _objects, std::shared_ptr<Model> _model, std::shared_ptr<Config> _config, std::shared_ptr<Settings> _settings) :
    wxPanel(parent, wxID_ANY, wxDefaultPosition, size, wxTAB_TRAVERSAL), objects(_objects), model(_model), config(_config), settings(_settings)
{ 
    
    this->Bind(wxEVT_PAINT, [=](wxPaintEvent &e) { this->repaint(e); });
    this->Bind(wxEVT_MOTION, [=](wxMouseEvent &e) { this->mouse_drag(e); });
    this->Bind(wxEVT_LEFT_DOWN, [=](wxMouseEvent &e) { this->mouse_down(e); });
    this->Bind(wxEVT_LEFT_UP, [=](wxMouseEvent &e) { this->mouse_up(e); });
    this->Bind(wxEVT_LEFT_DCLICK, [=](wxMouseEvent &e) { this->mouse_dclick(e); });

    if (user_drawn_background) {
        this->Bind(wxEVT_ERASE_BACKGROUND, [=](wxEraseEvent& e){ });
    }

    this->Bind(wxEVT_SIZE, [=](wxSizeEvent &e) { this->update_bed_size(); this->Refresh(); });

    // Bind the varying mouse events

    // Set the brushes
    set_colors();
    this->SetBackgroundStyle(wxBG_STYLE_PAINT);

}

void Plate2D::repaint(wxPaintEvent& e) {

    // Need focus to catch keyboard events.
    this->SetFocus();

    auto dc {new wxAutoBufferedPaintDC(this)};
    const auto& size {wxSize(this->GetSize().GetWidth(), this->GetSize().GetHeight())};


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
        auto tmp {scaled_points_to_pixel(this->bed_polygon, true)};
        dc->DrawPolygon(this->bed_polygon.points.size(), tmp.data(), 0, 0);
    }

    // draw print center
    {
        if (this->objects.size() > 0 && settings->autocenter) {
            const auto center = this->unscaled_point_to_pixel(this->print_center);
            dc->SetPen(print_center_pen);
            dc->DrawLine(center.x, 0, center.x, size.y);
            dc->DrawLine(0, center.y, size.x, center.y);
            dc->SetTextForeground(wxColor(0,0,0));
            dc->SetFont(wxFont(10, wxFONTFAMILY_ROMAN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

            dc->DrawLabel(wxString::Format("X = %.0f", this->print_center.x), wxRect(0,0, center.x*2, this->GetSize().GetHeight()), wxALIGN_CENTER_HORIZONTAL | wxALIGN_BOTTOM);
            dc->DrawRotatedText(wxString::Format("Y = %.0f", this->print_center.y), 0, center.y + 15, 90);
        }
    }

    // draw text if plate is empty
    if (this->objects.size() == 0) {
        dc->SetTextForeground(settings->color->BED_OBJECTS());
        dc->SetFont(wxFont(14, wxFONTFAMILY_ROMAN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        dc->DrawLabel(CANVAS_TEXT, wxRect(0,0, this->GetSize().GetWidth(), this->GetSize().GetHeight()), wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
    } else { 
        // draw grid
        dc->SetPen(grid_pen);
        // Assumption: grid of lines is arranged
        // as adjacent pairs of wxPoints
        for (auto i = 0U; i < grid.size(); i+=2) {
            dc->DrawLine(grid[i], grid[i+1]);
        }
    }


}

void Plate2D::mouse_drag(wxMouseEvent& e) {
    const auto pos {e.GetPosition()};
    const auto& point {this->point_to_model_units(e.GetPosition())};
    if (e.Dragging()) {
        Slic3r::Log::info(LogChannel, L"Mouse dragging");
    } else {
        auto cursor = wxSTANDARD_CURSOR;
        /*
        if (find_first_of(this->objects.begin(), this->objects.end(); [=](const PlaterObject& o) { return o.contour->contains_point(point);} ) == this->object.end()) {
            cursor = wxCursor(wxCURSOR_HAND);
        }
        */
        this->SetCursor(*cursor);
    }
}

void Plate2D::mouse_down(wxMouseEvent& e) {
}
void Plate2D::mouse_up(wxMouseEvent& e) {
}
void Plate2D::mouse_dclick(wxMouseEvent& e) {
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
            if (obj.selected) {
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

    this->bed_polygon = Slic3r::Polygon(scale(dynamic_cast<ConfigOptionPoints*>(config->optptr("bed_shape"))->values));

    const auto& polygon = bed_polygon;

    const auto& bb = bed_polygon.bounding_box();
    const auto& size = bb.size();

    this->scaling_factor = std::min(canvas_w / unscale(size.x), canvas_h / unscale(size.y));

    this->bed_origin = wxPoint(
        canvas_w / 2 - (unscale(bb.max.x + bb.min.x)/2 * this->scaling_factor),
        canvas_h - (canvas_h / 2 - (unscale(bb.max.y + bb.min.y)/2 * this->scaling_factor))
    );

    const auto& center = bb.center();
    this->print_center = wxPoint(unscale(center.x), unscale(center.y));

    // Cache bed contours and grid
    {
        const auto& step { scale_(10) };
        auto grid {Polylines()};

        for (coord_t x = (bb.min.x - (bb.min.x % step) + step); x < bb.max.x; x += step) {
            grid.push_back(Polyline());
            grid.back().append(Point(x, bb.min.y));
            grid.back().append(Point(x, bb.max.y));
        };

        for (coord_t y = (bb.min.y - (bb.min.y % step) + step); y < bb.max.y; y += step) {
            grid.push_back(Polyline());
            grid.back().append(Point(bb.min.x, y));
            grid.back().append(Point(bb.max.x, y));
        };

        grid = intersection_pl(grid, polygon);
        for (auto& i : grid) {
            const auto& tmpline { this->scaled_points_to_pixel(i, 1) };
            this->grid.insert(this->grid.end(), tmpline.begin(), tmpline.end());
        }
    }
}

std::vector<wxPoint> Plate2D::scaled_points_to_pixel(const Slic3r::Polygon& poly, bool unscale) {
    return this->scaled_points_to_pixel(Polyline(poly),  unscale);
}

std::vector<wxPoint> Plate2D::scaled_points_to_pixel(const Slic3r::Polyline& poly, bool unscale) {
    std::vector<wxPoint> result;
    for (const auto& pt : poly.points) {
        const auto tmp {wxPoint(pt.x, pt.y)};
        result.push_back( (unscale ? unscaled_point_to_pixel(tmp) : tmp) );
    }
    return result;
}



} } // Namespace Slic3r::GUI
