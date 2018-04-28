#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "PrintConfig.hpp"

namespace Slic3r {

class Config : DynamicPrintConfig {
public:
    static Config *new_from_defaults();
    static Config *new_from_cli(const &argc, const char* argv[]);

    void write_ini(const std::string& file) { save(file); }
    void read_ini(const std::string& file) { load(file); }


};

} // namespace Slic3r

#endif // CONFIG_HPP
