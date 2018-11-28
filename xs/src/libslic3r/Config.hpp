#ifndef CONFIG_HPP
#define CONFIG_HPP


#include <initializer_list>
#include <memory>
#include <regex>
#include <string>

#include "PrintConfig.hpp"
#include "ConfigBase.hpp"

namespace Slic3r {

class Config;
using config_ptr = std::shared_ptr<Config>;
using config_ref = std::weak_ptr<Config>;

class Config {
public:

    /// Factory method to construct a Config with all default values loaded.
    static std::shared_ptr<Config> new_from_defaults();

    /// Factory method to construct a Config with specific default values loaded.
    static std::shared_ptr<Config> new_from_defaults(std::initializer_list<std::string> init);
    
    /// Factory method to construct a Config with specific default values loaded.
    static std::shared_ptr<Config> new_from_defaults(t_config_option_keys init);

    /// Factory method to construct a Config from an ini file.
    static std::shared_ptr<Config> new_from_ini(const std::string& inifile);
    
    double getFloat(const t_config_option_key& opt_key) const {
        return this->_config.getFloat(opt_key);
    }
    int getInt(const t_config_option_key& opt_key) const {
        return this->_config.getInt(opt_key);
    }
    bool getBool(const t_config_option_key& opt_key) const {
        return this->_config.getBool(opt_key);
    }
    std::string getString(const t_config_option_key& opt_key) const {
        return this->_config.getString(opt_key);
    }


    /// Template function to dynamic cast and leave it in pointer form.
    template <class T>
    T* get_ptr(const t_config_option_key& opt_key, bool create=true) {
        return this->_config.opt_throw<T>(opt_key, create);
    }

    /// Template function to retrieve and cast in hopefully a slightly nicer 
    /// format than longwinded dynamic_cast<> 
    template <class T>
    T& get(const t_config_option_key& opt_key, bool create=true) {
        return *this->_config.opt_throw<T>(opt_key, create);
    }
    
    /// Function to parse value from a string to whatever opt_key is.
    void set(const t_config_option_key& opt_key, const std::string& value) {
        this->_config.set_deserialize_throw(opt_key, value);
    };

    void set(const t_config_option_key& opt_key, const char* value) {
        this->set(opt_key, std::string(value));
    };
    
    /// Function to parse value from an integer to whatever opt_key is, if
    /// opt_key is a numeric type. This will throw an exception and do 
    /// nothing if called with an incorrect type.
    void set(const t_config_option_key& opt_key, const int value) {
        this->_config.setInt(opt_key, value);
    };

    /// Function to parse value from an boolean to whatever opt_key is, if
    /// opt_key is a numeric type. This will throw an exception and do 
    /// nothing if called with an incorrect type.
    void set(const t_config_option_key& opt_key, const bool value) {
        this->_config.setBool(opt_key, value);
    };
    
    /// Function to parse value from an integer to whatever opt_key is, if
    /// opt_key is a numeric type. This will throw an exception and do 
    /// nothing if called with an incorrect type.
    void set(const t_config_option_key& opt_key, const double value) {
        this->_config.setFloat(opt_key, value);
    };

    /// Method to validate the different configuration options. 
    /// It will throw InvalidOptionException exceptions on failure.
    void validate() { this->_config.validate(); };

    const DynamicPrintConfig& config() const { return _config; }
    bool empty() const { return _config.empty(); }

    /// Pass-through of apply()
    void apply(const config_ptr& other) { _config.apply(other->config()); }
    void apply(const Slic3r::Config& other) { _config.apply(other.config()); }
   
    /// Apply only configuration options in the array.
    void apply_with_defaults(const config_ptr& other, const t_config_option_keys& keys) { _config.apply_only(other->_config, keys, false, true); }
    void apply(const config_ptr& other, const t_config_option_keys& keys) { _config.apply_only(other->_config, keys, false, false); }

    /// Allow other configs to be applied to this one.
    void apply(const Slic3r::ConfigBase& other) { _config.apply(other); }

    /// Pass-through of diff()
    t_config_option_keys diff(const config_ptr& other) { return _config.diff(other->config()); };
    t_config_option_keys diff(const Slic3r::Config& other) { return _config.diff(other.config()); }

    /// Return whether or not the underlying ConfigBase contains a key k
    bool has(const t_config_option_key& k) const { return _config.has(k); };

    /// Do not use; prefer static factory methods instead.
    Config() : _config(DynamicPrintConfig()) {}; 

private:
    /// Underlying configuration store.
    DynamicPrintConfig _config {};
};

} // namespace Slic3r

#endif // CONFIG_HPP
