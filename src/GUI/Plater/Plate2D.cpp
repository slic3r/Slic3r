#include "Plater/Plate2D.hpp"
#include "Log.hpp"

#include <wx/colour.h>

namespace Slic3r { namespace GUI {

Plate2D::Plate2D(wxWindow* parent, const wxSize& size, std::vector<Plater2DObject>& _objects, std::shared_ptr<Model> _model, std::shared_ptr<Config> _config, std::shared_ptr<Settings> _settings) :
    wxPanel(parent, wxID_ANY, wxDefaultPosition, size, wxTAB_TRAVERSAL), objects(_objects), model(_model), config(_config), settings(_settings)
{ 
    
    this->Bind(wxEVT_PAINT, [=](wxPaintEvent &e) { this->repaint(e); });
    this->Bind(wxEVT_MOTION, [=](wxMouseEvent &e) { this->mouse_drag(e); });
    if (user_drawn_background) {
        this->Bind(wxEVT_ERASE_BACKGROUND, [=](wxEraseEvent& e){ });
    }

    // Bind the varying mouse events

    // Set the brushes
    set_colors();
}

void Plate2D::repaint(wxPaintEvent& e) {
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

} } // Namespace Slic3r::GUI
