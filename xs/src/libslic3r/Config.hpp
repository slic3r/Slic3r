#ifndef SLIC3RXS

#ifndef CONFIG_HPP
#define CONFIG_HPP


#include <initializer_list>
#include <memory>
#include <regex>

#include "PrintConfig.hpp"
#include "ConfigBase.hpp"

namespace Slic3r {

/// Exception class for invalid (but correct type) option values. 
/// Thrown by validate()
class InvalidOptionValue : public std::runtime_error {
public:
    InvalidOptionValue(const char* v) : runtime_error(v) {}
};

/// Exception class to handle config options that don't exist.
class InvalidConfigOption : public std::runtime_error {};

/// Exception class for type mismatches
class InvalidOptionType : public std::runtime_error {
public:
    InvalidOptionType(const char* v) : runtime_error(v) {}
};

class Config;
using config_ptr = std::shared_ptr<Config>;

class Config : public DynamicPrintConfig {
public:

    /// Factory method to construct a Config with all default values loaded.
    static std::shared_ptr<Config> new_from_defaults();

    /// Factory method to construct a Config with specific default values loaded.
    static std::shared_ptr<Config> new_from_defaults(std::initializer_list<std::string> init);
    
    /// Factory method to construct a Config with specific default values loaded.
    static std::shared_ptr<Config> new_from_defaults(t_config_option_keys init);

    /// Factory method to construct a Config from CLI options.
    static std::shared_ptr<Config> new_from_cli(const int& argc, const char* argv[]);

    /// Factory method to construct a Config from an ini file.
    static std::shared_ptr<Config> new_from_ini(const std::string& inifile);

    /// Write a windows-style opt=value ini file with categories from the configuration store.
    void write_ini(const std::string& file) const;

    /// Parse a windows-style opt=value ini file with categories and load the configuration store.
    void read_ini(const std::string& file);

    /// Template function to retrieve and cast in hopefully a slightly nicer 
    /// format than longwinded dynamic_cast<> 
    template <class T>
    T& get(const t_config_option_key& opt_key, bool create=false) {
        return *(dynamic_cast<T*>(this->optptr(opt_key, create)));
    }

    /// Template function to dynamic cast and leave it in pointer form.
    template <class T>
    T* get_ptr(const t_config_option_key& opt_key, bool create=false) {
        return dynamic_cast<T*>(this->optptr(opt_key, create));
    }

    /// Function to parse value from a string to whatever opt_key is.
    void set(const t_config_option_key& opt_key, const std::string& value);
    
    /// Function to parse value from an integer to whatever opt_key is, if
    /// opt_key is a numeric type. This will throw an exception and do 
    /// nothing if called with an incorrect type.
    void set(const t_config_option_key& opt_key, const int value);
    
    /// Function to parse value from an integer to whatever opt_key is, if
    /// opt_key is a numeric type. This will throw an exception and do 
    /// nothing if called with an incorrect type.
    void set(const t_config_option_key& opt_key, const double value);

    /// Method to validate the different configuration options. 
    /// It will throw InvalidConfigOption exceptions on failure.
    bool validate();

private:
    std::regex _cli_pattern {"=(.+)$"};
    std::smatch _match_info {};
};

bool is_valid_int(const std::string& type, const ConfigOptionDef& opt, const std::string& ser_value);
bool is_valid_float(const std::string& type, const ConfigOptionDef& opt, const std::string& ser_value);

} // namespace Slic3r

#endif // CONFIG_HPP

#endif // SLIC3RXS
