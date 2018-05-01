#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <initializer_list>
#include <memory>

#include "PrintConfig.hpp"

namespace Slic3r {

class Config : public DynamicPrintConfig {
public:
    static std::shared_ptr<Config> new_from_defaults();
    static std::shared_ptr<Config> new_from_defaults(std::initializer_list<std::string> init);
    static std::shared_ptr<Config> new_from_cli(const int& argc, const char* argv[]);

    void write_ini(const std::string& file) { save(file); }
    void read_ini(const std::string& file) { load(file); }
};

} // namespace Slic3r

#endif // CONFIG_HPP
