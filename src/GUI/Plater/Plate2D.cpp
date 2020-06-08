#include "Plater/Plate2D.hpp"

// libslic3r includes
#include "Geometry.hpp"
#include "Point.hpp"
#include "Log.hpp"
#include "ClipperUtils.hpp"
#include "misc_ui.hpp"

// wx includes
#include <wx/colour.h>
#include <wx/dcbuffer.h>

namespace Slic3r { namespace GUI {

Plate2D::Plate2D(wxWindow* parent, const wxSize& size, std::vector<PlaterObject>& _objects, std::shared_ptr<Model> _model, std::shared_ptr<Config> _config) :
    wxPanel(parent, wxID_ANY, wxDefaultPosition, size, wxTAB_TRAVERSAL), objects(_objects), model(_model), config(_config)
{ 
    
    this->Bind(wxEVT_PAINT, [this](wxPaintEvent &e) { this->repaint(e); });
    this->Bind(wxEVT_MOTION, [this](wxMouseEvent &e) { this->mouse_drag(e); });

    // Bind the varying mouse events
    this->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent &e) { this->mouse_down(e); });
    this->Bind(wxEVT_RIGHT_DOWN, [this](wxMouseEvent &e) { this->mouse_down(e); });
    this->Bind(wxEVT_LEFT_UP, [this](wxMouseEvent &e) { this->mouse_up(e); });
    this->Bind(wxEVT_LEFT_DCLICK, [this](wxMouseEvent &e) { this->mouse_dclick(e); });

    if (user_drawn_background) {
        this->Bind(wxEVT_ERASE_BACKGROUND, [this](wxEraseEvent& e){ });
    }

    this->Bind(wxEVT_SIZE, [this](wxSizeEvent &e) { this->update_bed_size(); this->Refresh(); });

    this->Bind(wxEVT_CHAR, [this](wxKeyEvent &e) { this->nudge_key(e);});


    // Set the brushes
    set_colors();
    this->SetBackgroundStyle(wxBG_STYLE_PAINT);

}

void Plate2D::repaint(wxPaintEvent& e) {

    // Need focus to catch keyboard events.
    this->SetFocus();

    // create the device context.
    wxAutoBufferedPaintDC dc(this);
    const auto& size {wxSize(this->GetSize().GetWidth(), this->GetSize().GetHeight())};


    if (this->user_drawn_background) {
        // On all systems the AutoBufferedPaintDC() achieves double buffering.
        // On MacOS the background is erased, on Windows the background is not erased 
        // and on Linux/GTK the background is erased to gray color.
        // Fill DC with the background on Windows & Linux/GTK.
        const auto& brush_background {wxBrush(ui_settings->color->BACKGROUND255(), wxBRUSHSTYLE_SOLID)};
        const auto& pen_background {wxPen(ui_settings->color->BACKGROUND255(), 1, wxPENSTYLE_SOLID)};
        dc.SetPen(pen_background);
        dc.SetBrush(brush_background);
        const auto& rect {this->GetUpdateRegion().GetBox()};
        dc.DrawRectangle(rect.GetLeft(), rect.GetTop(), rect.GetWidth(), rect.GetHeight());
    }

    // Draw bed
    {
        dc.SetPen(this->print_center_pen);
        dc.SetBrush(this->bed_brush);
        auto tmp {scaled_points_to_pixel(this->bed_polygon, true)};
        dc.DrawPolygon(tmp.size(), tmp.data(), 0, 0);
    }

    // draw print center
    {
        if (this->objects.size() > 0 && ui_settings->autocenter) {
            const auto center = this->unscaled_point_to_pixel(this->print_center);
            dc.SetPen(print_center_pen);
            dc.DrawLine(center.x, 0, center.x, size.y);
            dc.DrawLine(0, center.y, size.x, center.y);
            dc.SetTextForeground(wxColor(0,0,0));
            dc.SetFont(wxFont(10, wxFONTFAMILY_ROMAN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

            dc.DrawLabel(wxString::Format("X = %.0f", this->print_center.x), wxRect(0,0, center.x*2, this->GetSize().GetHeight()), wxALIGN_CENTER_HORIZONTAL | wxALIGN_BOTTOM);
            dc.DrawRotatedText(wxString::Format("Y = %.0f", this->print_center.y), 0, center.y + 15, 90);
        }
    }

    // draw text if plate is empty
    if (this->objects.size() == 0) {
        dc.SetTextForeground(ui_settings->color->BED_OBJECTS());
        dc.SetFont(wxFont(14, wxFONTFAMILY_ROMAN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        dc.DrawLabel(CANVAS_TEXT, wxRect(0,0, this->GetSize().GetWidth(), this->GetSize().GetHeight()), wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL);
    } else { 
        // draw grid
        dc.SetPen(grid_pen);
        // Assumption: grid of lines is arranged
        // as adjacent pairs of wxPoints
        for (auto i = 0U; i < grid.size(); i+=2) {
            dc.DrawLine(grid[i], grid[i+1]);
        }
    }

    // Draw thumbnails

    dc.SetPen(dark_pen); 
    this->clean_instance_thumbnails();
    for (auto& obj : this->objects) {
        Slic3r::Log::info(LogChannel, LOG_WSTRING("Iterating over object " << obj.identifier));
        auto model_object {this->model->objects.at(obj.identifier)};
        Slic3r::Log::info(LogChannel, "at() didn't crash");
        if (obj.thumbnail.expolygons.size() == 0) 
            continue; // if there's no thumbnail move on
        for (size_t instance_idx = 0U; instance_idx < model_object->instances.size(); instance_idx++) {
            auto& instance {model_object->instances.at(instance_idx)};
            Slic3r::Log::info(LogChannel, LOG_WSTRING("Drawing polygon for " << obj.input_file));
            if (obj.transformed_thumbnail.expolygons.size() == 0) continue;
            auto thumbnail {obj.transformed_thumbnail}; // starts in unscaled model coords

            thumbnail.translate(Point::new_scale(instance->offset));

            obj.instance_thumbnails.emplace_back(thumbnail);

            if (0) { // object is dragged 
                dc.SetBrush(dragged_brush);
            } else if (obj.selected && obj.selected_instance >= 0 && obj.selected_instance == static_cast<int>(instance_idx)) { 
                Slic3r::Log::info(LogChannel, L"Using instance brush.");
                dc.SetBrush(instance_brush);
            } else if (obj.selected) {
                Slic3r::Log::info(LogChannel, L"Using selection brush.");
                dc.SetBrush(selected_brush);
            } else {
                Slic3r::Log::info(LogChannel, L"Using default objects brush.");
                dc.SetBrush(objects_brush);
            }
            // porting notes: perl here seems to be making a single-item array of the 
            // thumbnail set.
            // no idea why. It doesn't look necessary, so skip the outer layer
            // and draw the contained expolygons
            for (const auto& points : obj.instance_thumbnails.back().expolygons) {
                auto poly { this->scaled_points_to_pixel(Polygon(points), true) };
                dc.DrawPolygon(poly.size(), poly.data(), 0, 0);
                Slic3r::Log::info(LogChannel, LOG_WSTRING("Drawing polygon for " << obj.input_file));
            }

            // TODO draw bounding box if that debug option is turned on.

            // if sequential printing is enabled and more than one object, draw clearance area
            if (this->config->get<ConfigOptionBool>("complete_objects") && 
            std::count_if(this->model->objects.cbegin(), this->model->objects.cend(), [](const ModelObject* o){ return o->instances.size() > 0; }) > 1) {
                auto clearance {offset(thumbnail.convex_hull(), (scale_(this->config->get<ConfigOptionFloat>("extruder_clearance_radius")) / 2.0), 1.0, ClipperLib::jtRound, scale_(0.1))};
                dc.SetPen(this->clearance_pen);
                dc.SetBrush(this->transparent_brush);
                auto poly { this->scaled_points_to_pixel(Polygon(clearance.front()), true) };
                dc.DrawPolygon(poly.size(), poly.data(), 0, 0);
            }
        }
    }

    // Draw skirt
    if (this->objects.size() > 0 && this->config->get<ConfigOptionInt>("skirts") > 0) 
    {
        // get all of the contours of the instances
        Slic3r::Polygons tmp_cont {};
        for (auto obj : this->objects) {
            for (auto inst : obj.instance_thumbnails) {
                tmp_cont.push_back(inst.convex_hull());
            }
        }

        // Calculate the offset hull and draw the points.
        if (tmp_cont.size() > 0) {
            dc.SetPen(this->skirt_pen);
            dc.SetBrush(this->transparent_brush);
            auto skirt {offset(Slic3r::Geometry::convex_hull(tmp_cont), 
                scale_(this->config->get<ConfigOptionFloat>("brim_width") + this->config->get<ConfigOptionFloat>("skirt_distance")), 1.0, ClipperLib::jtRound, scale_(0.1))};
            auto poly { this->scaled_points_to_pixel(skirt.front(), true) };
            dc.DrawPolygon(poly.size(), poly.data(), 0, 0);
        }
    }

    e.Skip();
}

void Plate2D::clean_instance_thumbnails() {
    for (auto& obj : this->objects) {
        obj.instance_thumbnails.clear();
    }
}

void Plate2D::mouse_drag(wxMouseEvent& e) {
    const auto& point {this->point_to_model_units(e.GetPosition())};
    if (e.Dragging()) {
        if (!this->drag_start_pos.IsFullySpecified()) return; // possible concurrency problems?
        auto model_object = this->model->objects.at(this->drag_object.obj);
        model_object->instances.at(this->drag_object.inst)->offset = 
            Slic3r::Pointf(
                unscale(point.x - this->drag_start_pos.x), 
                unscale(point.y - this->drag_start_pos.y)
            );
        model_object->update_bounding_box();
        this->Refresh();
    } else { // moving, set the cursor to the hand cursor.
        if (std::any_of(this->objects.cbegin(), this->objects.cend(), 
                [=](const Slic3r::GUI::PlaterObject o) { return o.instance_contains(point); })
           ) 
        {
            this->SetCursor(wxCURSOR_HAND);
        } else {
            this->SetCursor(*wxSTANDARD_CURSOR);
        }
    }
}

void Plate2D::mouse_down(wxMouseEvent& e) {
    this->SetFocus(); // Focus needed to move selected instance with keyboard arrows

    const auto& pos = e.GetPosition();
    const auto point = this->point_to_model_units(pos);
    this->on_select_object(-1);
    this->selected_instance = {-1, -1};
    
    Slic3r::Log::info(LogChannel, LOG_WSTRING("Mouse down at scaled point " << point.x << ", " << point.y));

    // Iterate through the list backwards to catch the highest object (last placed/drawn), which 
    // is usually what the user wants.
    for (auto obj = this->objects.rbegin(); obj != this->objects.rend(); obj++) {
        const auto& obj_idx {obj->identifier};
        for (auto thumbnail = obj->instance_thumbnails.crbegin(); thumbnail != obj->instance_thumbnails.crend(); thumbnail++) {
            Slic3r::Log::info(LogChannel, LOG_WSTRING("First point: " << thumbnail->contours()[0].points[0].x << "," << thumbnail->contours()[0].points[0].y));
            if (thumbnail->contains(point)) {
                const auto& instance_idx {std::distance(thumbnail, obj->instance_thumbnails.crend()) - 1};
                Slic3r::Log::info(LogChannel, LOG_WSTRING(instance_idx << " contains this point"));
                this->on_select_object(obj_idx);
                if (e.LeftDown()) {
                    // start dragging
                    auto& instance {this->model->objects.at(obj_idx)->instances.at(instance_idx)};
                    auto instance_origin { Point::new_scale(instance->offset) };

                    this->drag_start_pos = wxPoint(
                        point.x - instance_origin.x,
                        point.y - instance_origin.y
                    );

                    this->drag_object = { obj_idx, instance_idx };
                    this->selected_instance = this->drag_object;

                    obj->selected_instance = instance_idx;

                } else if(e.RightDown()) {
                    this->on_right_click(pos);
                }
            }
        }
    }
    this->Refresh();
}

void Plate2D::mouse_up(wxMouseEvent& e) {
    if (e.LeftUp()) {
        try {
            if (this->drag_object.obj != -1 && this->drag_object.inst != -1) this->on_instances_moved();
        } catch (std::bad_function_call &ex) {
            Slic3r::Log::error(LogChannel, L"On_instances_moved was not initialized to a function.");
        }
        this->drag_start_pos = wxPoint(-1, -1);
        this->drag_object = {-1, -1};
        this->SetCursor(*wxSTANDARD_CURSOR);
    }
}
void Plate2D::mouse_dclick(wxMouseEvent& e) {
    if (e.LeftDClick()) {
        this->on_double_click();
    }
}

void Plate2D::set_colors() {

    this->SetBackgroundColour(ui_settings->color->BACKGROUND255());

    this->objects_brush.SetColour(ui_settings->color->BED_OBJECTS());
    this->objects_brush.SetStyle(wxBRUSHSTYLE_SOLID);
    this->instance_brush.SetColour(ui_settings->color->BED_INSTANCE());
    this->instance_brush.SetStyle(wxBRUSHSTYLE_SOLID);
    this->selected_brush.SetColour(ui_settings->color->BED_SELECTED());
    this->selected_brush.SetStyle(wxBRUSHSTYLE_SOLID);
    this->dragged_brush.SetColour(ui_settings->color->BED_DRAGGED());
    this->dragged_brush.SetStyle(wxBRUSHSTYLE_SOLID);
    this->bed_brush.SetColour(ui_settings->color->BED_COLOR());
    this->bed_brush.SetStyle(wxBRUSHSTYLE_SOLID);
    this->transparent_brush.SetColour(wxColour(0,0,0));
    this->transparent_brush.SetStyle(wxBRUSHSTYLE_TRANSPARENT);

    this->grid_pen.SetColour(ui_settings->color->BED_GRID());
    this->grid_pen.SetWidth(1);
    this->grid_pen.SetStyle(wxPENSTYLE_SOLID);
    this->print_center_pen.SetColour(ui_settings->color->BED_CENTER());
    this->print_center_pen.SetWidth(1);
    this->print_center_pen.SetStyle(wxPENSTYLE_SOLID);
    this->clearance_pen.SetColour(ui_settings->color->BED_CLEARANCE());
    this->clearance_pen.SetWidth(1);
    this->clearance_pen.SetStyle(wxPENSTYLE_SOLID);
    this->skirt_pen.SetColour(ui_settings->color->BED_SKIRT());
    this->skirt_pen.SetWidth(1);
    this->skirt_pen.SetStyle(wxPENSTYLE_SOLID);
    this->dark_pen.SetColour(ui_settings->color->BED_DARK());
    this->dark_pen.SetWidth(1);
    this->dark_pen.SetStyle(wxPENSTYLE_SOLID);

}

void Plate2D::nudge_key(wxKeyEvent& e) {
    const auto key = e.GetKeyCode();

    switch( key ) {
        case WXK_LEFT:
            this->nudge(MoveDirection::Left);
            break;
        case WXK_RIGHT:
            this->nudge(MoveDirection::Right);
            break;
        case WXK_DOWN:
            this->nudge(MoveDirection::Down);
            break;
        case WXK_UP:
            this->nudge(MoveDirection::Up);
            break;
        default:
            break; // do nothing
    }

}

void Plate2D::nudge(MoveDirection dir) {
    if (this->selected_instance.obj < this->objects.size()) {
        auto i = 0U;
        for (auto& obj : this->objects) {
            if (obj.selected) {
                if (obj.selected_instance != -1) {
                }
            }
            i++;
        }
    }
    if (selected_instance.obj >= this->objects.size()) { 
        Slic3r::Log::warn(LogChannel, L"Nudge failed because there is no selected instance.");
        return; // Abort
    }
    auto selected {this->selected_instance};
    auto obj {this->model->objects.at(selected.obj)};
    auto instance {obj->instances.at(selected.inst)};

    Slic3r::Point shift(0,0);

    auto nudge_value {ui_settings->nudge < 0.1 ? 0.1 : scale_(ui_settings->nudge) };

    switch (dir) {
        case MoveDirection::Up:
            shift.y = nudge_value;
            break;
        case MoveDirection::Down:
            shift.y = -1.0f * nudge_value;
            break;
        case MoveDirection::Left:
            shift.x = -1.0f * nudge_value;
            break;
        case MoveDirection::Right:
            shift.x = nudge_value;
            break;
        default:
            Slic3r::Log::error(LogChannel, L"Invalid direction supplied to nudge.");
    }
    Slic3r::Point instance_origin {Slic3r::Point::new_scale(instance->offset)};
    instance->offset = Slic3r::Pointf::new_unscale(shift + instance_origin);

    obj->update_bounding_box();
    this->Refresh();
    this->on_instances_moved();

}

void Plate2D::update_bed_size() {
    const auto& canvas_size {this->GetSize()};
    const auto& canvas_w {canvas_size.GetWidth()};
    const auto& canvas_h {canvas_size.GetHeight()};
    if (canvas_w == 0) return; // Abort early if we haven't drawn canvas yet.

    this->bed_polygon = Slic3r::Polygon(scale(config->get<ConfigOptionPoints>("bed_shape").values));

    const auto& polygon = bed_polygon;

    const auto& bb = bed_polygon.bounding_box();
    const auto& size = bb.size();


    // calculate the scaling factor needed for constraining print bed area inside preview
    this->scaling_factor = std::min(static_cast<double>(canvas_w) / unscale(size.x), static_cast<double>(canvas_h) / unscale(size.y));
    this->bed_origin = wxPoint(
        canvas_w / 2 - (unscale(bb.max.x + bb.min.x)/2 * this->scaling_factor),
        canvas_h - (canvas_h / 2 - (unscale(bb.max.y + bb.min.y)/2 * this->scaling_factor))
    );

    const auto& center = bb.center();
    this->print_center = wxPoint(unscale(center.x), unscale(center.y));

    this->grid.clear(); // clear out the old grid 
    // Cache bed contours and grid
    {
        const auto& step { scale_(10.0) }; // want a 10mm step for the lines
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

std::vector<wxPoint> Plate2D::scaled_points_to_pixel(const Slic3r::Polyline& poly, bool _unscale) {
    std::vector<wxPoint> result;
    for (const auto& pt : poly.points) {
        const auto x {_unscale ? Slic3r::unscale(pt.x) : pt.x};
        const auto y {_unscale ? Slic3r::unscale(pt.y) : pt.y};
        result.push_back(unscaled_point_to_pixel(wxPoint(x, y)));
    }
    return result;
}

wxPoint Plate2D::unscaled_point_to_pixel(const wxPoint& in) {
    const auto& canvas_height {this->GetSize().GetHeight()};
    const auto& zero = this->bed_origin;
    return wxPoint(
            in.x * this->scaling_factor + zero.x, 
            canvas_height - in.y * this->scaling_factor + (zero.y - canvas_height));
}


} } // Namespace Slic3r::GUI
