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
    const wxColour SELECTED_COLOR() const { return COLOR_MAGENTA; }
    const wxColour HOVER_COLOR() const { return COLOR_VIOLET; }
    const wxColour SUPPORT_COLOR() const { return COLOR_VIOLET; }

    const wxColour BACKGROUND_COLOR() const { return COLOR_BASE3; }


private:
    const wxColour COLOR_BASE0()    const {wxColour(255*0.51373,255*0.58039,255*0.58824)};
    const wxColour COLOR_BASE00()   const {wxColour(255*0.39608,255*0.48235,255*0.51373)};
    const wxColour COLOR_BASE01()   const {wxColour(255*0.34510,255*0.43137,255*0.45882)};
    const wxColour COLOR_BASE02()   const {wxColour(255*0.02745,255*0.21176,255*0.25882)};
    const wxColour COLOR_BASE03()   const {wxColour(255*0.00000,255*0.16863,255*0.21176)};
    const wxColour COLOR_BASE1()    const {wxColour(255*0.57647,255*0.63137,255*0.63137)};
    const wxColour COLOR_BASE2()    const {wxColour(255*0.93333,255*0.90980,255*0.83529)};
    const wxColour COLOR_BASE3()    const {wxColour(255*0.99216,255*0.96471,255*0.89020)};
    const wxColour COLOR_BLUE()     const {wxColour(255*0.14902,255*0.54510,255*0.82353)};
    const wxColour COLOR_CYAN()     const {wxColour(255*0.16471,255*0.63137,255*0.59608)};
    const wxColour COLOR_GREEN()    const {wxColour(255*0.52157,255*0.60000,255*0.00000)};
    const wxColour COLOR_MAGENTA()  const {wxColour(255*0.82745,255*0.21176,255*0.50980)};
    const wxColour COLOR_ORANGE()   const {wxColour(255*0.79608,255*0.29412,255*0.08627)};
    const wxColour COLOR_RED()      const {wxColour(255*0.86275,255*0.19608,255*0.18431)};
    const wxColour COLOR_VIOLET()   const {wxColour(255*0.42353,255*0.44314,255*0.76863)};
    const wxColour COLOR_YELLOW()   const {wxColour(255*0.70980,255*0.53725,255*0.00000)};

};
}} // namespace Slic3r::GUI


#endif // SOLARIZED_HPP
