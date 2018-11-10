#include "Preset.hpp"
#include "Config.hpp"
#include <regex>

using namespace std::literals::string_literals;

namespace Slic3r { namespace GUI {

    Preset::Preset(std::string load_dir, std::string filename, preset_t p) : dir(load_dir), file(filename), group(p) {
        // find last .ini at the end of the filename.
        std::regex ini (".ini[ ]*$");
        this->name = std::regex_replace(filename, ini, "$1");
    }

    bool Preset::dirty() const { 
        return false;
    }

}} // namespace Slic3r::GUI
