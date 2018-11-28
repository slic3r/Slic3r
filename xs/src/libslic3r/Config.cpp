#include "Config.hpp"

namespace Slic3r {

std::shared_ptr<Config> 
Config::new_from_defaults() 
{ 
    std::shared_ptr<Config> my_config(std::make_shared<Config>());
    my_config->_config.apply(FullPrintConfig());
    return my_config;
}
std::shared_ptr<Config> 
Config::new_from_defaults(std::initializer_list<std::string> init)
{
    return Config::new_from_defaults(t_config_option_keys(init));
}

std::shared_ptr<Config> 
Config::new_from_defaults(t_config_option_keys init)
{
    std::shared_ptr<Config> my_config(std::make_shared<Config>());
    my_config->_config.set_defaults(init);
    return my_config;
}

std::shared_ptr<Config> 
Config::new_from_ini(const std::string& inifile) 
{ 
    std::shared_ptr<Config> my_config(std::make_shared<Config>());
    my_config->_config.load(inifile);
    return my_config;
}

} // namespace Slic3r
