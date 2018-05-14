#ifndef SOLARIZED_HPP
#define SOLARIZED_HPP
#include <wx/colour.h>

#include "ColorScheme/ColorSchemeBase.hpp"

namespace Slic3r { namespace GUI {

/// S O L A R I Z E
/// http://ethanschoonover.com/solarized
///
/// Implements a colorscheme lookup of wxColors
class Solarized : public ColorScheme {
public:
    const wxColour SELECTED_COLOR() const { return COLOR_SOLAR_MAGENTA; } 
    const wxColour HOVER_COLOR() const { return COLOR_SOLAR_VIOLET; }     
    const wxColour SUPPORT_COLOR() const { return COLOR_SOLAR_VIOLET; }   

    const wxColour BACKGROUND_COLOR() const { return COLOR_SOLAR_BASE3; } 


private:
    const wxColour COLOR_SOLAR_BASE0   { wxColour(255*0.51373,255*0.58039,255*0.58824)};
    const wxColour COLOR_SOLAR_BASE00  { wxColour(255*0.39608,255*0.48235,255*0.51373)};
    const wxColour COLOR_SOLAR_BASE01  { wxColour(255*0.34510,255*0.43137,255*0.45882)};
    const wxColour COLOR_SOLAR_BASE02  { wxColour(255*0.02745,255*0.21176,255*0.25882)};
    const wxColour COLOR_SOLAR_BASE03  { wxColour(255*0.00000,255*0.16863,255*0.21176)};
    const wxColour COLOR_SOLAR_BASE1   { wxColour(255*0.57647,255*0.63137,255*0.63137)};
    const wxColour COLOR_SOLAR_BASE2   { wxColour(255*0.93333,255*0.90980,255*0.83529)};
    const wxColour COLOR_SOLAR_BASE3   { wxColour(255*0.99216,255*0.96471,255*0.89020)};
    const wxColour COLOR_SOLAR_BLUE    { wxColour(255*0.14902,255*0.54510,255*0.82353)};
    const wxColour COLOR_SOLAR_CYAN    { wxColour(255*0.16471,255*0.63137,255*0.59608)};
    const wxColour COLOR_SOLAR_GREEN   { wxColour(255*0.52157,255*0.60000,255*0.00000)};
    const wxColour COLOR_SOLAR_MAGENTA { wxColour(255*0.82745,255*0.21176,255*0.50980)};
    const wxColour COLOR_SOLAR_ORANGE  { wxColour(255*0.79608,255*0.29412,255*0.08627)};
    const wxColour COLOR_SOLAR_RED     { wxColour(255*0.86275,255*0.19608,255*0.18431)};
    const wxColour COLOR_SOLAR_VIOLET  { wxColour(255*0.42353,255*0.44314,255*0.76863)};
    const wxColour COLOR_SOLAR_YELLOW  { wxColour(255*0.70980,255*0.53725,255*0.00000)};

};
}} // namespace Slic3r::GUI


#endif // SOLARIZED_HPP
