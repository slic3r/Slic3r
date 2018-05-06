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
    const wxColour SELECTED_COLOR() { return COLOR_MAGENTA; }
    const wxColour HOVER_COLOR() { return COLOR_VIOLET; }
    const wxColour SUPPORT_COLOR() { return COLOR_VIOLET; }

    const wxColour BACKGROUND_COLOR() { return COLOR_BASE3; }


private:
    const wxColour COLOR_BASE0    {wxColour(0.51373,0.58039,0.58824)};
    const wxColour COLOR_BASE00   {wxColour(0.39608,0.48235,0.51373)};
    const wxColour COLOR_BASE01   {wxColour(0.34510,0.43137,0.45882)};
    const wxColour COLOR_BASE02   {wxColour(0.02745,0.21176,0.25882)};
    const wxColour COLOR_BASE03   {wxColour(0.00000,0.16863,0.21176)};
    const wxColour COLOR_BASE1    {wxColour(0.57647,0.63137,0.63137)};
    const wxColour COLOR_BASE2    {wxColour(0.93333,0.90980,0.83529)};
    const wxColour COLOR_BASE3    {wxColour(0.99216,0.96471,0.89020)};
    const wxColour COLOR_BLUE     {wxColour(0.14902,0.54510,0.82353)};
    const wxColour COLOR_CYAN     {wxColour(0.16471,0.63137,0.59608)};
    const wxColour COLOR_GREEN    {wxColour(0.52157,0.60000,0.00000)};
    const wxColour COLOR_MAGENTA  {wxColour(0.82745,0.21176,0.50980)};
    const wxColour COLOR_ORANGE   {wxColour(0.79608,0.29412,0.08627)};
    const wxColour COLOR_RED      {wxColour(0.86275,0.19608,0.18431)};
    const wxColour COLOR_VIOLET   {wxColour(0.42353,0.44314,0.76863)};
    const wxColour COLOR_YELLOW   {wxColour(0.70980,0.53725,0.00000)};

};
}} // namespace Slic3r::GUI


#endif // SOLARIZED_HPP
