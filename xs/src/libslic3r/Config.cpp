#include "Config.hpp"
#include "Log.hpp"

#include <string>

namespace Slic3r {

extern const PrintConfigDef print_config_def;

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
    std::shared_ptr<Config>  my_config(std::make_shared<Config>());
    for (auto& opt_key : init) {
        if (print_config_def.has(opt_key)) {
            const std::string value { print_config_def.get(opt_key).default_value->serialize() };
            my_config->_config.set_deserialize(opt_key, value);
        }
    }

    return my_config;
}

std::shared_ptr<Config> 
Config::new_from_ini(const std::string& inifile) 
{ 
    
    std::shared_ptr<Config> my_config(std::make_shared<Config>());
    my_config->read_ini(inifile);
    return my_config;
}

// TODO: this should be merged into ConfigBase::validate()
bool
Config::validate()
{
    // general validation
    for (auto k : this->_config.keys()) {
        if (print_config_def.options.count(k) == 0) continue; // skip over keys that aren't in the master list
        const ConfigOptionDef& opt { print_config_def.options.at(k) };
        if (opt.cli == "" || std::regex_search(opt.cli, _match_info, _cli_pattern) == false) continue;
        std::string type { _match_info.str(1) };
        std::vector<std::string> values;
        if (std::regex_search(type, _match_info, std::regex("@$"))) {
            type = std::regex_replace(type, std::regex("@$"), std::string("")); // strip off the @ for later;
            ConfigOptionVectorBase* tmp_opt;
            try {
                tmp_opt = get_ptr<ConfigOptionVectorBase>(k);
            } catch (std::bad_cast& e) {
                throw InvalidOptionType((std::string("(cast failure) Invalid value for ") + std::string(k)).c_str());
            }
            const std::vector<std::string> tmp_str { tmp_opt->vserialize() };
            values.insert(values.end(), tmp_str.begin(), tmp_str.end());
        } else {
            Slic3r::Log::debug("Config::validate", std::string("Not an array"));
            Slic3r::Log::debug("Config::validate", type);
            values.emplace_back(get_ptr<ConfigOption>(k)->serialize());
        }
        // Verify each value
        for (auto v : values) {
            if (type == "i" || type == "f" || opt.type == coPercent || opt.type == coFloatOrPercent) {
                if (opt.type == coPercent || opt.type == coFloatOrPercent)
                    v = std::regex_replace(v, std::regex("%$"), std::string(""));
                if ((type == "i" && !is_valid_int(type, opt, v)) || !is_valid_float(type, opt, v)) {
                    throw InvalidOptionValue((std::string("Invalid value for ") + std::string(k)).c_str());
                }
            }
        }
    }
    return true;
}

void
Config::set(const t_config_option_key& opt_key, const std::string& value)
{
    try {
        const auto& def = print_config_def.options.at(opt_key);
        switch (def.type) {
            case coInt:
                {
                    ConfigOptionInt* ptr {dynamic_cast<ConfigOptionInt*>(this->_config.optptr(opt_key, true))};
                    ptr->value = std::stoi(value);
                } break;
            case coInts:
                {
                    ConfigOptionInts* ptr {dynamic_cast<ConfigOptionInts*>(this->_config.optptr(opt_key, true))};
                    if (!ptr->deserialize(value, true) ) {
                        throw InvalidOptionValue(std::string(opt_key) + std::string(" set with invalid value."));
                    }
                } break;
            case coFloat:
                {
                    ConfigOptionFloat* ptr {dynamic_cast<ConfigOptionFloat*>(this->_config.optptr(opt_key, true))};
                    ptr->setFloat(std::stod(value));
                } break;
            case coFloatOrPercent:
                {
                    ConfigOptionFloatOrPercent* ptr {dynamic_cast<ConfigOptionFloatOrPercent*>(this->_config.optptr(opt_key, true))};
                    const size_t perc = value.find("%");
                    ptr->percent = (perc != std::string::npos);
                    if (ptr->percent) {
                        ptr->setFloat(std::stod(std::string(value).replace(value.find("%"), std::string("%").length(), "")));
                        ptr->percent = true;
                    } else {
                        ptr->setFloat(std::stod(value));
                        ptr->percent = false;
                    }
                } break;
            case coFloats:
                {
                    ConfigOptionFloats* ptr {dynamic_cast<ConfigOptionFloats*>(this->_config.optptr(opt_key, true))};
                    if (!ptr->deserialize(value, true) ) {
                        throw InvalidOptionValue(std::string(opt_key) + std::string(" set with invalid value."));
                    }
                } break;
            case coString:
                {
                    ConfigOptionString* ptr {dynamic_cast<ConfigOptionString*>(this->_config.optptr(opt_key, true))};
                    if (!ptr->deserialize(value) ) {
                        throw InvalidOptionValue(std::string(opt_key) + std::string(" set with invalid value."));
                    }
                } break;
            case coBool:
                {
                    ConfigOptionBool* ptr {dynamic_cast<ConfigOptionBool*>(this->_config.optptr(opt_key, true))};
                    ptr->deserialize(value);
                } break;
            default: 
                Slic3r::Log::warn("Config::set", "Unknown set type.");
        }
    } catch (std::invalid_argument& e) {
        throw InvalidOptionValue(std::string(opt_key) + std::string(" set with invalid value."));
    } catch (std::out_of_range& e) {
        throw InvalidOptionType(std::string(opt_key) + std::string(" is an invalid Slic3r option."));
    }
}

void
Config::set(const t_config_option_key& opt_key, const bool value) 
{
    try {
        const auto& def = print_config_def.options.at(opt_key);
        switch (def.type) {
            case coBool:
                {   
                    ConfigOptionBool* ptr {dynamic_cast<ConfigOptionBool*>(this->_config.optptr(opt_key, true))};
                    ptr->value = value;
                } break;
            case coInt:
                {
                    ConfigOptionInt* ptr {dynamic_cast<ConfigOptionInt*>(this->_config.optptr(opt_key, true))};
                    ptr->setInt(value);
                } break;
            case coInts:
                {
                    ConfigOptionInts* ptr {dynamic_cast<ConfigOptionInts*>(this->_config.optptr(opt_key, true))};
                    ptr->deserialize(std::to_string(value), true);
                } break;
            case coFloat:
                {
                    ConfigOptionFloat* ptr {dynamic_cast<ConfigOptionFloat*>(this->_config.optptr(opt_key, true))};
                    ptr->setFloat(value);
                } break;
            case coFloatOrPercent:
                {
                    ConfigOptionFloatOrPercent* ptr {dynamic_cast<ConfigOptionFloatOrPercent*>(this->_config.optptr(opt_key, true))};
                    ptr->setFloat(value);
                    ptr->percent = false;
                } break;
            case coFloats:
                {
                    ConfigOptionFloats* ptr {dynamic_cast<ConfigOptionFloats*>(this->_config.optptr(opt_key, true))};
                    ptr->deserialize(std::to_string(value), true);
                } break;
            case coString:
                {
                    ConfigOptionString* ptr {dynamic_cast<ConfigOptionString*>(this->_config.optptr(opt_key, true))};
                    if (!ptr->deserialize(std::to_string(value)) ) {
                        throw InvalidOptionValue(std::string(opt_key) + std::string(" set with invalid value."));
                    }
                } break;
            default: 
                Slic3r::Log::warn("Config::set", "Unknown set type.");
        }

    } catch (std::out_of_range &e) {
        throw InvalidOptionType(std::string(opt_key) + std::string(" is an invalid Slic3r option."));
    }

}
void
Config::set(const t_config_option_key& opt_key, const int value)
{
    try {
        const auto& def = print_config_def.options.at(opt_key);
        switch (def.type) {
            case coBool:
                {   
                    ConfigOptionBool* ptr {dynamic_cast<ConfigOptionBool*>(this->_config.optptr(opt_key, true))};
                    ptr->value = (value != 0);
                } break;
            case coInt:
                {
                    ConfigOptionInt* ptr {dynamic_cast<ConfigOptionInt*>(this->_config.optptr(opt_key, true))};
                    ptr->setInt(value);
                } break;
            case coInts:
                {
                    ConfigOptionInts* ptr {dynamic_cast<ConfigOptionInts*>(this->_config.optptr(opt_key, true))};
                    ptr->deserialize(std::to_string(value), true);
                } break;
            case coFloat:
                {
                    ConfigOptionFloat* ptr {dynamic_cast<ConfigOptionFloat*>(this->_config.optptr(opt_key, true))};
                    ptr->setFloat(value);
                } break;
            case coFloatOrPercent:
                {
                    ConfigOptionFloatOrPercent* ptr {dynamic_cast<ConfigOptionFloatOrPercent*>(this->_config.optptr(opt_key, true))};
                    ptr->setFloat(value);
                    ptr->percent = false;
                } break;
            case coFloats:
                {
                    ConfigOptionFloats* ptr {dynamic_cast<ConfigOptionFloats*>(this->_config.optptr(opt_key, true))};
                    ptr->deserialize(std::to_string(value), true);
                } break;
            case coString:
                {
                    ConfigOptionString* ptr {dynamic_cast<ConfigOptionString*>(this->_config.optptr(opt_key, true))};
                    if (!ptr->deserialize(std::to_string(value)) ) {
                        throw InvalidOptionValue(std::string(opt_key) + std::string(" set with invalid value."));
                    }
                } break;
            default: 
                Slic3r::Log::warn("Config::set", "Unknown set type.");
        }

    } catch (std::out_of_range &e) {
        throw InvalidOptionType(std::string(opt_key) + std::string(" is an invalid Slic3r option."));
    }
}

void
Config::set(const t_config_option_key& opt_key, const double value)
{
    try {
        const auto& def = print_config_def.options.at(opt_key);
        switch (def.type) {
            case coInt:
                {
                    ConfigOptionInt* ptr {dynamic_cast<ConfigOptionInt*>(this->_config.optptr(opt_key, true))};
                    ptr->setInt(std::round(value));
                } break;
            case coInts:
                {
                    ConfigOptionInts* ptr {dynamic_cast<ConfigOptionInts*>(this->_config.optptr(opt_key, true))};
                    ptr->deserialize(std::to_string(std::round(value)), true);
                } break;
            case coFloat:
                {
                    ConfigOptionFloat* ptr {dynamic_cast<ConfigOptionFloat*>(this->_config.optptr(opt_key, true))};
                    ptr->setFloat(value);
                } break;
            case coFloatOrPercent:
                {
                    ConfigOptionFloatOrPercent* ptr {dynamic_cast<ConfigOptionFloatOrPercent*>(this->_config.optptr(opt_key, true))};
                    ptr->setFloat(value);
                    ptr->percent = false;
                } break;
            case coFloats:
                {
                    ConfigOptionFloats* ptr {dynamic_cast<ConfigOptionFloats*>(this->_config.optptr(opt_key, true))};
                    ptr->deserialize(std::to_string(value), true);
                } break;
            case coString:
                {
                    ConfigOptionString* ptr {dynamic_cast<ConfigOptionString*>(this->_config.optptr(opt_key, true))};
                    if (!ptr->deserialize(std::to_string(value)) ) {
                        throw InvalidOptionValue(std::string(opt_key) + std::string(" set with invalid value."));
                    }
                } break;
            default: 
                Slic3r::Log::warn("Config::set", "Unknown set type.");
        }
    } catch (std::out_of_range &e) {
        throw InvalidOptionType(std::string(opt_key) + std::string(" is an invalid Slic3r option."));
    }
}

void
Config::read_ini(const std::string& file)
{
    this->_config.load(file);
}

void
Config::write_ini(const std::string& file) const
{
}

bool
is_valid_int(const std::string& type, const ConfigOptionDef& opt, const std::string& ser_value)
{
    std::regex _valid_int {"^-?\\d+$"};
    std::smatch match;
    ConfigOptionInt tmp;
    
    bool result {type == "i"};
    if (result) 
        result = result && std::regex_search(ser_value, match, _valid_int);
    if (result) {
        tmp.deserialize(ser_value);
        result = result & (tmp.getInt() <= opt.max && tmp.getInt() >= opt.min);
    }

    return result;
}

bool
is_valid_float(const std::string& type, const ConfigOptionDef& opt, const std::string& ser_value)
{
    std::regex _valid_float {"^-?(?:\\d+|\\d*\\.\\d+)$"};
    std::smatch match;
    ConfigOptionFloat tmp;
    Slic3r::Log::debug("is_valid_float", ser_value);

    bool result {type == "f" || opt.type == coPercent || opt.type == coFloatOrPercent};
    if (result) 
        result = result && std::regex_search(ser_value, match, _valid_float);
    if (result) {
        tmp.deserialize(ser_value);
        result = result & (tmp.getFloat() <= opt.max && tmp.getFloat() >= opt.min);
    }
    return result;
}

Config::Config() : _config(DynamicPrintConfig()) {};

} // namespace Slic3r
