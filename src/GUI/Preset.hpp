#ifndef PRESET_HPP
#define PRESET_HPP

#include "PrintConfig.hpp"

namespace Slic3r { namespace GUI {

/// Preset types list. We assign numbers to permit static_casts and use as preset tab indices
enum class preset_t : uint8_t {
    Print = 0, Material, Printer,
    Last // This MUST be the last enumeration. Don't use it for anything.
};

/// Convenience counter to determine how many preset tabs exist.
constexpr size_t preset_types = static_cast<uint8_t>(preset_t::Last);

class Preset; 

using Presets = std::vector<Preset>;

class Preset {
    preset_t type; 
    std::string name {""};


    /// Search the compatible_printers config option list for this preset name.
    /// Assume that Printer configs are compatible with other Printer configs
    bool compatible(std::string printer_name) { return true; }
    bool compatible(const Preset& other) {return (this->type == preset_t::Printer || (compatible(other.name) && other.type == preset_t::Printer));}
private:
    /// store to keep config options for this preset
    Slic3r::DynamicPrintConfig config { Slic3r::DynamicPrintConfig() };

};

}} // namespace Slic3r::GUI

#endif // PRESET_HPP
