#ifndef COLOR_DEFAULT_HPP
#define COLOR_DEFAULT_HPP

namespace Slic3r { namespace GUI {

class DefaultColor : public ColorScheme {
public:
    const std::string name() const { return "Default"; }
    const bool SOLID_BACKGROUNDCOLOR() const { return false; };
    const wxColour SELECTED_COLOR() const { return wxColour(0, 255, 0); };
    const wxColour HOVER_COLOR() const { return wxColour(255*0.4, 255*0.9, 0); };            //<Hover over Model
    const wxColour TOP_COLOR() const { return wxColour(10,98,144); };    //<TOP Background color
    const wxColour BOTTOM_COLOR() const { return wxColour(0,0,0); };                 //<BOTTOM Background color
    const wxColour BACKGROUND_COLOR() const {return TOP_COLOR();}                      //< SOLID background color
    const wxColour GRID_COLOR() const { return wxColour(255*0.2, 255*0.2, 255*0.2, 255*0.4); };      //<Grid color
    const wxColour GROUND_COLOR() const { return wxColour(255*0.8, 255*0.6, 255*0.5, 255*0.4); };    //<Ground or Plate color
    const wxColour COLOR_CUTPLANE() const { return wxColour(255*0.8, 255*0.8, 255*0.8, 255*0.5); };
    const wxColour COLOR_PARTS() const { return wxColour(255, 255*0.95, 255*0.2); };        //<Perimeter color
    const wxColour COLOR_INFILL() const { return wxColour(255, 255*0.45, 255*0.45); };
    const wxColour COLOR_SUPPORT() const { return wxColour(255*0.5, 1, 255*0.5); };
    const wxColour COLOR_UNKNOWN() const { return wxColour(255*0.5, 255*0.5, 1); };
    const wxColour BED_COLOR() const { return wxColour(255, 255, 255); };
    const wxColour BED_GRID() const { return wxColour(230, 230, 230); };
    const wxColour BED_SELECTED() const { return wxColour(255, 166, 128); };
    const wxColour BED_OBJECTS() const { return wxColour(210, 210, 210); };
    const wxColour BED_INSTANCE() const { return wxColour(255, 128, 128); };
    const wxColour BED_DRAGGED() const { return wxColour(128, 128, 255); };
    const wxColour BED_CENTER() const { return wxColour(200, 200, 200); };
    const wxColour BED_SKIRT() const { return wxColour(150, 150, 150); };
    const wxColour BED_CLEARANCE() const { return wxColour(0, 0, 200); };
    const wxColour BACKGROUND255() const { return wxColour(255, 255, 255); };
    const wxColour TOOL_DARK() const { return wxColour(0, 0, 0); };
    const wxColour TOOL_SUPPORT() const { return wxColour(0, 0, 0); };
    const wxColour TOOL_INFILL() const { return wxColour(0, 0, 255*0.7); };
    const wxColour TOOL_STEPPERIM() const { return wxColour(255*0.7, 0, 0); };
    const wxColour TOOL_SHADE() const { return wxColour(255*0.95, 255*0.95, 255*0.95); };
    const wxColour TOOL_COLOR() const { return wxColour(255*0.9, 255*0.9, 255*0.9); };
    const wxColour SPLINE_L_PEN() const { return wxColour(50, 50, 50); };
    const wxColour SPLINE_O_PEN() const { return wxColour(200, 200, 200); };
    const wxColour SPLINE_I_PEN() const { return wxColour(255, 0, 0); };
    const wxColour SPLINE_R_PEN() const { return wxColour(5, 120, 160); };
    const wxColour BED_DARK() const { return wxColour(0, 0, 0); };
};

}} // namespace Slic3r::GUI

#endif
