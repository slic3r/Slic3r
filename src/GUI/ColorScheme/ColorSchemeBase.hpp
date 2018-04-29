#ifndef COLOR_SLIC3R_HPP
#define COLOR_SLIC3R_HPP

namespace Slic3r { namespace GUI {

class ColorScheme {
public:
    virtual const std::string name() const = 0;
    virtual const bool      SOLID_BACKGROUNDCOLOR() const = 0;
    virtual const wxColour SELECTED_COLOR() const = 0;
    virtual const wxColour HOVER_COLOR() const = 0;
    virtual const wxColour TOP_COLOR() const = 0;
    virtual const wxColour BOTTOM_COLOR() const = 0;
    virtual const wxColour BACKGROUND_COLOR() const = 0;
    virtual const wxColour GRID_COLOR() const = 0;
    virtual const wxColour GROUND_COLOR() const = 0;
    virtual const wxColour COLOR_CUTPLANE() const = 0;
    virtual const wxColour COLOR_PARTS() const = 0;
    virtual const wxColour COLOR_INFILL() const = 0;
    virtual const wxColour COLOR_SUPPORT() const = 0;
    virtual const wxColour COLOR_UNKNOWN() const = 0;
    virtual const wxColour BED_COLOR() const = 0;
    virtual const wxColour BED_GRID() const = 0;
    virtual const wxColour BED_SELECTED() const = 0;
    virtual const wxColour BED_OBJECTS() const = 0;
    virtual const wxColour BED_INSTANCE() const = 0;
    virtual const wxColour BED_DRAGGED() const = 0;
    virtual const wxColour BED_CENTER() const = 0;
    virtual const wxColour BED_SKIRT() const = 0;
    virtual const wxColour BED_CLEARANCE() const = 0;
    virtual const wxColour BACKGROUND255() const = 0;
    virtual const wxColour TOOL_DARK() const = 0;
    virtual const wxColour TOOL_SUPPORT() const = 0;
    virtual const wxColour TOOL_INFILL() const = 0;
    virtual const wxColour TOOL_STEPPERIM() const = 0;
    virtual const wxColour TOOL_SHADE() const = 0;
    virtual const wxColour TOOL_COLOR() const = 0;
    virtual const wxColour SPLINE_L_PEN() const = 0;
    virtual const wxColour SPLINE_O_PEN() const = 0;
    virtual const wxColour SPLINE_I_PEN() const = 0;
    virtual const wxColour SPLINE_R_PEN() const = 0;
    virtual const wxColour BED_DARK() const = 0;
};

}} // namespace Slic3r::GUI

#endif
