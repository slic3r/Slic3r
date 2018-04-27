#ifndef PRESET_HPP
#define PRESET_HPP

#include "PrintConfig.hpp"

namespace Slic3r { namespace GUI {

class Preset {

private:
    Slic3r::DynamicPrintConfig config { Slic3r::DynamicPrintConfig() };

};

}} // namespace Slic3r::GUI

#endif // PRESET_HPP
