#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <initializer_list>
#include <memory>

#include "PrintConfig.hpp"
#include "ConfigBase.hpp"

namespace Slic3r {

class Config;
using config_ptr = std::shared_ptr<Config>;

class Config : public DynamicPrintConfig {
public:
    static std::shared_ptr<Config> new_from_defaults();
    static std::shared_ptr<Config> new_from_defaults(std::initializer_list<std::string> init);
    static std::shared_ptr<Config> new_from_defaults(t_config_option_keys init);
    static std::shared_ptr<Config> new_from_cli(const int& argc, const char* argv[]);

    void write_ini(const std::string& file) { save(file); }
    void read_ini(const std::string& file) { load(file); }

    /// Template function to retrieve and cast in hopefully a slightly nicer 
    /// format than longwinded dynamic_cast<> 
    template <class T>
    T& get(const t_config_option_key& opt_key, bool create=false) {
        return *(dynamic_cast<T*>(this->optptr(opt_key, create)));
    }
};

} // namespace Slic3r

#endif // CONFIG_HPP
