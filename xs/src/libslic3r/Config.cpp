#include "Config.hpp"
#include "Log.hpp"

namespace Slic3r {

std::shared_ptr<Config> 
Config::new_from_defaults() 
{ 
    return std::make_shared<Config>();
}
std::shared_ptr<Config> 
Config::new_from_defaults(std::initializer_list<std::string> init)
{
    return Config::new_from_defaults(t_config_option_keys(init));
}

std::shared_ptr<Config> 
Config::new_from_defaults(t_config_option_keys init)
{
    auto my_config(std::make_shared<Config>());
    for (auto& opt_key : init) {
        if (print_config_def.has(opt_key)) {
            const std::string value { print_config_def.get(opt_key)->default_value->serialize() };
            my_config->set_deserialize(opt_key, value);
        }
    }

    return my_config;
}

std::shared_ptr<Config> 
new_from_cli(const int& argc, const char* argv[])
{
    return std::make_shared<Config>();
}

} // namespace Slic3r
