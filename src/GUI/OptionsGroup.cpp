#include "OptionsGroup.hpp"
#include "libslic3r.h"
#include "PrintConfig.hpp"

namespace Slic3r { namespace GUI {

std::shared_ptr<UI_Field> OptionsGroup::append(const t_config_option_key& opt_id) {
    std::shared_ptr<UI_Field> a;
    const ConfigOptionDef& id = print_config_def.options.at(opt_id);

    return a;
}

}} // Slic3r::GUI
