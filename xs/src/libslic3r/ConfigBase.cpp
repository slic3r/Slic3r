#include "ConfigBase.hpp"
#include <assert.h>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <exception> // std::runtime_error
#include <set>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/config.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/nowide/cenv.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <string.h>

namespace Slic3r {

std::string escape_string_cstyle(const std::string &str)
{
    // Allocate a buffer twice the input string length,
    // so the output will fit even if all input characters get escaped.
    std::vector<char> out(str.size() * 2, 0);
    char *outptr = out.data();
    for (size_t i = 0; i < str.size(); ++ i) {
        char c = str[i];
        if (c == '\n' || c == '\r') {
            (*outptr ++) = '\\';
            (*outptr ++) = 'n';
        } else if (c == '\\'){
            (*outptr ++) = '\\';
            (*outptr ++) = '\\';
        } else
            (*outptr ++) = c;
    }
    return std::string(out.data(), outptr - out.data());
}

std::string escape_strings_cstyle(const std::vector<std::string> &strs)
{
    // 1) Estimate the output buffer size to avoid buffer reallocation.
    size_t outbuflen = 0;
    for (size_t i = 0; i < strs.size(); ++ i)
        // Reserve space for every character escaped + quotes + semicolon.
        outbuflen += strs[i].size() * 2 + 3;
    // 2) Fill in the buffer.
    std::vector<char> out(outbuflen, 0);
    char *outptr = out.data();
    for (size_t j = 0; j < strs.size(); ++ j) {
        if (j > 0)
            // Separate the strings.
            (*outptr ++) = ';';
        const std::string &str = strs[j];
        // Is the string simple or complex? Complex string contains spaces, tabs, new lines and other
        // escapable characters. Empty string shall be quoted as well, if it is the only string in strs.
        bool should_quote = strs.size() == 1 && str.empty();
        for (size_t i = 0; i < str.size(); ++ i) {
            char c = str[i];
            if (c == ' ' || c == '\t' || c == '\\' || c == '"' || c == '\r' || c == '\n') {
                should_quote = true;
                break;
            }
        }
        if (should_quote) {
            (*outptr ++) = '"';
            for (size_t i = 0; i < str.size(); ++ i) {
                char c = str[i];
                if (c == '\\' || c == '"') {
                    (*outptr ++) = '\\';
                    (*outptr ++) = c;
                } else if (c == '\n' || c == '\r') {
                    (*outptr ++) = '\\';
                    (*outptr ++) = 'n';
                } else
                    (*outptr ++) = c;
            }
            (*outptr ++) = '"';
        } else {
            memcpy(outptr, str.data(), str.size());
            outptr += str.size();
        }
    }
    return std::string(out.data(), outptr - out.data());
}

bool unescape_string_cstyle(const std::string &str, std::string &str_out)
{
    std::vector<char> out(str.size(), 0);
    char *outptr = out.data();
    for (size_t i = 0; i < str.size(); ++ i) {
        char c = str[i];
        if (c == '\\') {
            if (++ i == str.size())
                return false;
            c = str[i];
            if (c == 'n')
                (*outptr ++) = '\n';
        } else
            (*outptr ++) = c;
    }
    str_out.assign(out.data(), outptr - out.data());
    return true;
}

bool unescape_strings_cstyle(const std::string &str, std::vector<std::string> &out)
{
    if (str.empty())
        return true;

    size_t i = 0;
    for (;;) {
        // Skip white spaces.
        char c = str[i];
        while (c == ' ' || c == '\t') {
            if (++ i == str.size())
                return true;
            c = str[i];
        }
        // Start of a word.
        std::vector<char> buf;
        buf.reserve(16);
        // Is it enclosed in quotes?
        c = str[i];
        if (c == '"') {
            // Complex case, string is enclosed in quotes.
            for (++ i; i < str.size(); ++ i) {
                c = str[i];
                if (c == '"') {
                    // End of string.
                    break;
                }
                if (c == '\\') {
                    if (++ i == str.size())
                        return false;
                    c = str[i];
                    if (c == 'n')
                        c = '\n';
                }
                buf.push_back(c);
            }
            if (i == str.size())
                return false;
            ++ i;
        } else {
            for (; i < str.size(); ++ i) {
                c = str[i];
                if (c == ';')
                    break;
                buf.push_back(c);
            }
        }
        // Store the string into the output vector.
        out.push_back(std::string(buf.data(), buf.size()));
        if (i == str.size())
            return true;
        // Skip white spaces.
        c = str[i];
        while (c == ' ' || c == '\t') {
            if (++ i == str.size())
                // End of string. This is correct.
                return true;
            c = str[i];
        }
        if (c != ';')
            return false;
        if (++ i == str.size()) {
            // Emit one additional empty string.
            out.push_back(std::string());
            return true;
        }
    }
}

bool
operator== (const ConfigOption &a, const ConfigOption &b)
{
    return a.serialize().compare(b.serialize()) == 0;
}

bool
operator!= (const ConfigOption &a, const ConfigOption &b)
{
    return !(a == b);
}

ConfigOptionDef::ConfigOptionDef(const ConfigOptionDef &other)
    : type(other.type), default_value(nullptr),
      gui_type(other.gui_type), gui_flags(other.gui_flags), label(other.label), 
      full_label(other.full_label), category(other.category), tooltip(other.tooltip), 
      sidetext(other.sidetext), cli(other.cli), ratio_over(other.ratio_over), 
      multiline(other.multiline), full_width(other.full_width), readonly(other.readonly), 
      height(other.height), width(other.width), min(other.min), max(other.max),
      aliases(other.aliases), shortcut(other.shortcut), enum_values(other.enum_values),
      enum_labels(other.enum_labels), enum_keys_map(other.enum_keys_map)
{
    if (other.default_value != nullptr)
        this->default_value = other.default_value->clone();
}

ConfigOptionDef::~ConfigOptionDef()
{
    if (this->default_value != nullptr)
        delete this->default_value;
}

std::vector<std::string>
ConfigOptionDef::cli_args() const
{
    std::string cli = this->cli.substr(0, this->cli.find("="));
    boost::trim_right_if(cli, boost::is_any_of("!"));
    std::vector<std::string> args;
    boost::split(args, cli, boost::is_any_of("|"));
    return args;
}

ConfigOptionDef*
ConfigDef::add(const t_config_option_key &opt_key, ConfigOptionType type)
{
    ConfigOptionDef* opt = &this->options[opt_key];
    opt->type = type;
    return opt;
}

ConfigOptionDef*
ConfigDef::add(const t_config_option_key &opt_key, const ConfigOptionDef &def)
{
    this->options.insert(std::make_pair(opt_key, def));
    return &this->options[opt_key];
}

bool
ConfigDef::has(const t_config_option_key &opt_key) const
{
    return this->options.count(opt_key) > 0;
}

const ConfigOptionDef&
ConfigDef::get(const t_config_option_key &opt_key) const
{
    if (this->options.count(opt_key) == 0)
        throw UnknownOptionException(opt_key);
    return this->options.at(opt_key);
}

void
ConfigDef::merge(const ConfigDef &other)
{
    this->options.insert(other.options.begin(), other.options.end());
}

std::ostream&
ConfigDef::print_cli_help(std::ostream& out, bool show_defaults) const
{
    // prepare a function for wrapping text
    auto wrap = [](std::string text, size_t line_length) -> std::string {
        std::istringstream words(text);
        std::ostringstream wrapped;
        std::string word;
 
        if (words >> word) {
            wrapped << word;
            size_t space_left = line_length - word.length();
            while (words >> word) {
                if (space_left < word.length() + 1) {
                    wrapped << '\n' << word;
                    space_left = line_length - word.length();
                } else {
                    wrapped << ' ' << word;
                    space_left -= word.length() + 1;
                }
            }
        }
        return wrapped.str();
    };
    
    // get the unique categories
    std::set<std::string> categories;
    for (const auto& opt : this->options) {
        const ConfigOptionDef& def = opt.second;
        categories.insert(def.category);
    }
    
    for (auto category : categories) {
        if (category != "") {
            out << category << ":" << std::endl;
        } else if (categories.size() > 1) {
            out << "Misc options:" << std::endl;
        }
        
        for (const auto& opt : this->options) {
            const ConfigOptionDef& def = opt.second;
            if (def.category != category) continue;
            
            if (!def.cli.empty()) {
                // get all possible variations: --foo, --foobar, -f...
                auto cli_args = def.cli_args();
                for (auto& arg : cli_args) {
                    arg.insert(0, (arg.size() == 1) ? "-" : "--");
                    if (def.type == coFloat || def.type == coInt || def.type == coFloatOrPercent
                        || def.type == coFloats || def.type == coInts) {
                        arg += " N";
                    } else if (def.type == coPoint) {
                        arg += " X,Y";
                    } else if (def.type == coPoint3) {
                        arg += " X,Y,Z";
                    } else if (def.type == coString || def.type == coStrings) {
                        arg += " ABCD";
                    }
                }
            
                // left: command line options
                const std::string cli = boost::algorithm::join(cli_args, ", ");
                out << " " << std::left << std::setw(20) << cli;
            
                // right: option description
                std::string descr = def.tooltip;
                if (show_defaults && def.default_value != nullptr && def.type != coBool
                    && (def.type != coString || !def.default_value->serialize().empty())) {
                    descr += " (";
                    if (!def.sidetext.empty()) {
                        descr += def.sidetext + ", ";
                    } else if (!def.enum_values.empty()) {
                        descr += boost::algorithm::join(def.enum_values, ", ") + "; ";
                    }
                    descr += "default: " + def.default_value->serialize() + ")";
                }
            
                // wrap lines of description
                descr = wrap(descr, 80);
                std::vector<std::string> lines;
                boost::split(lines, descr, boost::is_any_of("\n"));
            
                // if command line options are too long, print description in new line
                for (size_t i = 0; i < lines.size(); ++i) {
                    if (i == 0 && cli.size() > 19)
                        out << std::endl;
                    if (i > 0 || cli.size() > 19)
                        out << std::string(21, ' ');
                    out << lines[i] << std::endl;
                }
            }
        }
    }
    return out;
}

bool
ConfigBase::has(const t_config_option_key &opt_key) const {
    return this->option(opt_key) != NULL;
}

void
ConfigBase::apply(const ConfigBase &other, bool ignore_nonexistent) {
    // apply all options
    this->apply_only(other, other.keys(), ignore_nonexistent, false);
}

void
ConfigBase::apply_only(const ConfigBase &other, const t_config_option_keys &opt_keys, bool ignore_nonexistent) {
    this->apply_only(other, opt_keys, ignore_nonexistent, false);
}

void
ConfigBase::apply_only(const ConfigBase &other, const t_config_option_keys &opt_keys, bool ignore_nonexistent, bool default_nonexistent) {
    // loop through options and apply them
    for (const t_config_option_key &opt_key : opt_keys) {
        ConfigOption* my_opt = this->option(opt_key, true);
        if (opt_key.size() == 0) continue;
        if (my_opt == NULL) {
            if (ignore_nonexistent == false) throw UnknownOptionException(opt_key);
            continue;
        }
        if (default_nonexistent && !other.has(opt_key)) {
            auto* def_opt = this->def->get(opt_key).default_value->clone();
            // not the most efficient way, but easier than casting pointers to subclasses
            bool res = my_opt->deserialize( def_opt->serialize() );
            if (!res) {
                std::string error = "Unexpected failure when deserializing serialized value for " + opt_key;
                CONFESS(error.c_str());
            }
            continue;
        }
        
        // not the most efficient way, but easier than casting pointers to subclasses
        bool res = my_opt->deserialize( other.option(opt_key)->serialize() );
        if (!res) {
            std::string error = "Unexpected failure when deserializing serialized value for " + opt_key;
            CONFESS(error.c_str());
        }
    }
}

void
ConfigBase::set_defaults(const t_config_option_keys &opt_keys)
{
    // use defaults from definition
    if (this->def == NULL) return;
    for (auto opt_key : opt_keys)
        if (this->def->has(opt_key))
            this->option(opt_key, true)->set(*this->def->options.at(opt_key).default_value);
}

bool
ConfigBase::equals(const ConfigBase &other) const {
    return this->diff(other).empty();
}

// this will *ignore* options not present in both configs
t_config_option_keys
ConfigBase::diff(const ConfigBase &other) const {
    t_config_option_keys diff;
    
    for (const t_config_option_key &opt_key : this->keys())
        if (other.has(opt_key) && other.serialize(opt_key) != this->serialize(opt_key))
            diff.push_back(opt_key);
    
    return diff;
}

std::string
ConfigBase::serialize(const t_config_option_key &opt_key) const {
    const ConfigOption* opt = this->option(opt_key);
    assert(opt != NULL);
    return opt->serialize();
}

bool
ConfigBase::set_deserialize(t_config_option_key opt_key, std::string str, bool append) {
    if (!this->def->has(opt_key)) {
        // If we didn't find an option, look for any other option having this as an alias.
        for (const auto &opt : this->def->options) {
            for (const t_config_option_key& alias : opt.second.aliases) {
                if (alias == opt_key) {
                    opt_key = alias;
                    break;
                }
            }
        }
    }
    if (!this->def->has(opt_key))
        throw UnknownOptionException(opt_key);
    
    const ConfigOptionDef& optdef = this->def->options.at(opt_key);
    if (!optdef.shortcut.empty()) {
        for (const t_config_option_key& shortcut : optdef.shortcut)
            if (!this->set_deserialize(shortcut, str))
                return false;
        
        return true;
    }
    
    ConfigOption* opt = this->option(opt_key, true);
    assert(opt != NULL);
    return opt->deserialize(str, append);
}

void
ConfigBase::set_deserialize_throw(t_config_option_key opt_key, std::string str, bool append) {
    bool res = this->set_deserialize(opt_key, str, append);
    if (!res) throw BadOptionTypeException();
}

// Return an absolute value of a possibly relative config variable.
// For example, return absolute infill extrusion width, either from an absolute value, or relative to the layer height.
double
ConfigBase::get_abs_value(const t_config_option_key &opt_key) const {
    const ConfigOption* opt = this->option(opt_key);
    if (const ConfigOptionFloatOrPercent* optv = dynamic_cast<const ConfigOptionFloatOrPercent*>(opt)) {
        // get option definition
        assert(this->def->has(opt_key));
        const ConfigOptionDef& def = this->def->get(opt_key);
        
        // compute absolute value over the absolute value of the base option
        return optv->get_abs_value(this->get_abs_value(def.ratio_over));
    } else if (const ConfigOptionFloat* optv = dynamic_cast<const ConfigOptionFloat*>(opt)) {
        return optv->value;
    } else {
        throw std::runtime_error("Not a valid option type for get_abs_value()");
    }
}

// Return an absolute value of a possibly relative config variable.
// For example, return absolute infill extrusion width, either from an absolute value, or relative to a provided value.
double
ConfigBase::get_abs_value(const t_config_option_key &opt_key, double ratio_over) const {
    // get stored option value
    const ConfigOptionFloatOrPercent* opt = dynamic_cast<const ConfigOptionFloatOrPercent*>(this->option(opt_key));
    assert(opt != NULL);
    
    // compute absolute value
    return opt->get_abs_value(ratio_over);
}

bool
ConfigBase::getBool(const t_config_option_key &opt_key) const {
    return this->option_throw(opt_key)->getBool();
}

bool
ConfigBase::getBool(const t_config_option_key &opt_key, bool default_value) const {
    const ConfigOption* opt = this->option(opt_key);
    return opt == nullptr ? default_value : opt->getBool();
}

void
ConfigBase::setBool(const t_config_option_key &opt_key, bool value) {
    this->option(opt_key, true)->setBool(value);
}

double
ConfigBase::getFloat(const t_config_option_key &opt_key) const {
    return this->option_throw(opt_key)->getFloat();
}

double
ConfigBase::getFloat(const t_config_option_key &opt_key, double default_value) const {
    const ConfigOption* opt = this->option(opt_key);
    return opt == nullptr ? default_value : opt->getFloat();
}

void
ConfigBase::setFloat(const t_config_option_key &opt_key, double value) {
    this->option(opt_key, true)->setFloat(value);
}

int
ConfigBase::getInt(const t_config_option_key &opt_key) const {
    return this->option_throw(opt_key)->getInt();
}

int
ConfigBase::getInt(const t_config_option_key &opt_key, int default_value) const {
    const ConfigOption* opt = this->option(opt_key);
    return opt == nullptr ? default_value : opt->getInt();
}

void
ConfigBase::setInt(const t_config_option_key &opt_key, int value) {
    this->option(opt_key, true)->setInt(value);
}

std::string
ConfigBase::getString(const t_config_option_key &opt_key) const {
    return this->option_throw(opt_key)->getString();
}

std::string
ConfigBase::getString(const t_config_option_key &opt_key, std::string default_value) const {
    const ConfigOption* opt = this->option(opt_key);
    return opt == nullptr ? default_value : opt->getString();
}

void
ConfigBase::setString(const t_config_option_key &opt_key, std::string value) {
    this->option(opt_key, true)->setString(value);
}

std::vector<std::string>
ConfigBase::getStrings(const t_config_option_key &opt_key) const {
    return this->option_throw(opt_key)->getStrings();
}

std::vector<std::string>
ConfigBase::getStrings(const t_config_option_key &opt_key, std::vector<std::string> default_value) const {
    const ConfigOption* opt = this->option(opt_key);
    return opt == nullptr ? default_value : opt->getStrings();
}

void
ConfigBase::setStrings(const t_config_option_key &opt_key, std::vector<std::string> value) {
    this->option(opt_key, true)->setStrings(value);
}

void
ConfigBase::setenv_()
{
    for (auto &opt_key : this->keys()) {
        std::string envname = "SLIC3R_" + opt_key;
        boost::to_upper(envname);
        boost::nowide::setenv(envname.c_str(), this->serialize(opt_key).c_str(), 1);
    }
}

const ConfigOption*
ConfigBase::option(const t_config_option_key &opt_key) const {
    return const_cast<ConfigBase*>(this)->option(opt_key, false);
}

const ConfigOption*
ConfigBase::option_throw(const t_config_option_key &opt_key) const {
    const auto opt = this->option(opt_key);
    if (opt == nullptr) throw UnknownOptionException(opt_key);
    return opt;
}

ConfigOption*
ConfigBase::option(const t_config_option_key &opt_key, bool create) {
    return this->optptr(opt_key, create);
}

ConfigOption*
ConfigBase::option_throw(const t_config_option_key &opt_key, bool create) {
    auto opt = this->optptr(opt_key, create);
    if (opt == nullptr) throw UnknownOptionException(opt_key);
    return opt;
}

void
ConfigBase::load(const std::string &file)
{
    namespace pt = boost::property_tree;
    pt::ptree tree;
	boost::nowide::ifstream ifs(file);
	pt::read_ini(ifs, tree);
    BOOST_FOREACH(const pt::ptree::value_type &v, tree) {
        try {
            t_config_option_key opt_key = v.first;
            std::string value = v.second.get_value<std::string>();
            this->set_deserialize(opt_key, value);
        } catch (UnknownOptionException &e) {
            // ignore
        }
    }
}

void
ConfigBase::save(const std::string &file) const
{
    using namespace std;
    boost::nowide::ofstream c;
    c.open(file, ios::out | ios::trunc);

    {
        time_t now;
        time(&now);
        char buf[sizeof "0000-00-00 00:00:00"];
        strftime(buf, sizeof buf, "%F %T", gmtime(&now));
        c << "# generated by Slic3r " << SLIC3R_VERSION << " on " << buf << endl;
    }

    t_config_option_keys my_keys = this->keys();
    for (t_config_option_keys::const_iterator opt_key = my_keys.begin(); opt_key != my_keys.end(); ++opt_key)
        c << *opt_key << " = " << this->serialize(*opt_key) << endl;
    c.close();
}

void
ConfigBase::validate() const
{
    for (auto &opt_key : this->keys()) {
        // get option definition
        assert(this->def->has(opt_key));
        const ConfigOptionDef& def = this->def->get(opt_key);
        
        if (def.type == coInt) {
            auto &value = this->opt<ConfigOptionInt>(opt_key)->value;
            if (value < def.min || value > def.max)
                throw InvalidOptionException(opt_key);
        } else if (def.type == coFloat) {
            auto &value = this->opt<ConfigOptionFloat>(opt_key)->value;
            if (value < def.min || value > def.max)
                throw InvalidOptionException(opt_key);
        } else if (def.type == coFloatOrPercent) {
            const auto* opt = this->opt<ConfigOptionFloatOrPercent>(opt_key);
            auto &value = opt->value;
            if (!opt->percent && (value < def.min || value > def.max))
                throw InvalidOptionException(opt_key);
        } else if (def.type == coInts) {
            for (auto &value : this->opt<ConfigOptionInts>(opt_key)->values)
                if (value < def.min || value > def.max)
                    throw InvalidOptionException(opt_key);
        } else if (def.type == coFloats) {
            for (auto &value : this->opt<ConfigOptionFloats>(opt_key)->values)
                if (value < def.min || value > def.max)
                    throw InvalidOptionException(opt_key);
        }
    }
}

DynamicConfig& DynamicConfig::operator= (DynamicConfig other)
{
    this->swap(other);
    return *this;
}

void
DynamicConfig::swap(DynamicConfig &other)
{
    std::swap(this->def, other.def);
    std::swap(this->options, other.options);
}

DynamicConfig::~DynamicConfig () {
    for (t_options_map::iterator it = this->options.begin(); it != this->options.end(); ++it) {
        ConfigOption* opt = it->second;
        if (opt != NULL) delete opt;
    }
}

DynamicConfig::DynamicConfig (const DynamicConfig& other) {
    this->def = other.def;
    this->apply(other, false);
}

ConfigOption*
DynamicConfig::optptr(const t_config_option_key &opt_key, bool create) {
    if (this->options.count(opt_key) == 0) {
        if (create) {
            if (!this->def->has(opt_key)) throw UnknownOptionException(opt_key);
            const ConfigOptionDef& optdef = this->def->options.at(opt_key);
            ConfigOption* opt;
            if (optdef.default_value != nullptr) {
                opt = optdef.default_value->clone();
            } else if (optdef.type == coFloat) {
                opt = new ConfigOptionFloat ();
            } else if (optdef.type == coFloats) {
                opt = new ConfigOptionFloats ();
            } else if (optdef.type == coInt) {
                opt = new ConfigOptionInt ();
            } else if (optdef.type == coInts) {
                opt = new ConfigOptionInts ();
            } else if (optdef.type == coString) {
                opt = new ConfigOptionString ();
            } else if (optdef.type == coStrings) {
                opt = new ConfigOptionStrings ();
            } else if (optdef.type == coPercent) {
                opt = new ConfigOptionPercent ();
            } else if (optdef.type == coFloatOrPercent) {
                opt = new ConfigOptionFloatOrPercent ();
            } else if (optdef.type == coPoint) {
                opt = new ConfigOptionPoint ();
            } else if (optdef.type == coPoint3) {
                opt = new ConfigOptionPoint3 ();
            } else if (optdef.type == coPoints) {
                opt = new ConfigOptionPoints ();
            } else if (optdef.type == coBool) {
                opt = new ConfigOptionBool ();
            } else if (optdef.type == coBools) {
                opt = new ConfigOptionBools ();
            } else if (optdef.type == coEnum) {
                ConfigOptionEnumGeneric* optv = new ConfigOptionEnumGeneric ();
                optv->keys_map = &optdef.enum_keys_map;
                opt = static_cast<ConfigOption*>(optv);
            } else {
                throw std::runtime_error("Unknown option type");
            }
            this->options[opt_key] = opt;
            return opt;
        } else {
            return NULL;
        }
    }
    return this->options[opt_key];
}

t_config_option_keys
DynamicConfig::keys() const {
    t_config_option_keys keys;
    for (t_options_map::const_iterator it = this->options.begin(); it != this->options.end(); ++it)
        keys.push_back(it->first);
    return keys;
}

void
DynamicConfig::erase(const t_config_option_key &opt_key) {
    this->options.erase(opt_key);
}

void
DynamicConfig::clear() {
    this->options.clear();
}

bool
DynamicConfig::empty() const {
    return this->options.empty();
}

void
DynamicConfig::read_cli(const std::vector<std::string> &tokens, t_config_option_keys* extra, t_config_option_keys* keys)
{
    std::vector<char*> _argv;
    
    // push a bogus executable name (argv[0])
    _argv.push_back(const_cast<char*>(""));

    for (size_t i = 0; i < tokens.size(); ++i)
        _argv.push_back(const_cast<char *>(tokens[i].c_str()));
    
    this->read_cli(_argv.size(), &_argv[0], extra, keys);
}

bool
DynamicConfig::read_cli(int argc, char** argv, t_config_option_keys* extra, t_config_option_keys* keys)
{
    // cache the CLI option => opt_key mapping
    std::map<std::string,std::string> opts;
    for (const auto &oit : this->def->options)
        for (auto t : this->def->get(oit.first).cli_args())
            opts[t] = oit.first;
    
    bool parse_options = true;
    for (int i = 1; i < argc; ++i) {
        std::string token = argv[i];
        
        // Store non-option arguments in the provided vector.
        if (!parse_options || !boost::starts_with(token, "-")) {
            extra->push_back(token);
            continue;
        }
        
        
        // Stop parsing tokens as options when -- is supplied.
        if (token == "--") {
            parse_options = false;
            continue;
        }
        
        // Remove leading dashes
        boost::trim_left_if(token, boost::is_any_of("-"));
        
        // Remove the "no-" prefix used to negate boolean options.
        bool no = false;
        if (boost::starts_with(token, "no-")) {
            no = true;
            boost::replace_first(token, "no-", "");
        }
        
        // Read value when supplied in the --key=value form.
        std::string value;
        {
            size_t equals_pos = token.find("=");
            if (equals_pos != std::string::npos) {
                value = token.substr(equals_pos+1);
                token.erase(equals_pos);
            }
        }
        
        // Look for the cli -> option mapping.
        const auto it = opts.find(token);
        if (it == opts.end()) {
            printf("Warning: unknown option --%s\n", token.c_str());
            // instead of continuing, return false to caller
            // to stop execution and print usage
            return false;
            //continue;
        }
        const t_config_option_key opt_key = it->second;
        const ConfigOptionDef &optdef = this->def->options.at(opt_key);
        
        // If the option type expects a value and it was not already provided,
        // look for it in the next token.
        if (optdef.type != coBool && optdef.type != coBools && value.empty()) {
            if (i == (argc-1)) {
                printf("No value supplied for --%s\n", token.c_str());
                continue;
            }
            value = argv[++i];
        }
        
        // Store the option value.
        const bool existing = this->has(opt_key);
        if (keys != nullptr && !existing) {
            // save the order of detected keys
            keys->push_back(opt_key);
        }
        if (ConfigOptionBool* opt = this->opt<ConfigOptionBool>(opt_key, true)) {
            opt->value = !no;
        } else if (ConfigOptionBools* opt = this->opt<ConfigOptionBools>(opt_key, true)) {
            if (!existing) opt->values.clear(); // remove the default values
            opt->values.push_back(!no);
        } else if (ConfigOptionStrings* opt = this->opt<ConfigOptionStrings>(opt_key, true)) {
            if (!existing) opt->values.clear(); // remove the default values
            opt->deserialize(value, true);
        } else if (ConfigOptionFloats* opt = this->opt<ConfigOptionFloats>(opt_key, true)) {
            if (!existing) opt->values.clear(); // remove the default values
            opt->deserialize(value, true);
        } else if (ConfigOptionPoints* opt = this->opt<ConfigOptionPoints>(opt_key, true)) {
            if (!existing) opt->values.clear(); // remove the default values
            opt->deserialize(value, true);
        } else {
            this->set_deserialize(opt_key, value, true);
        }
    }
    return true;
}

void
StaticConfig::set_defaults()
{
    ConfigBase::set_defaults(this->keys());
}

t_config_option_keys
StaticConfig::keys() const {
    t_config_option_keys keys;
    for (t_optiondef_map::const_iterator it = this->def->options.begin(); it != this->def->options.end(); ++it) {
        const ConfigOption* opt = this->option(it->first);
        if (opt != NULL) keys.push_back(it->first);
    }
    return keys;
}

bool
ConfigOptionPoint::deserialize(std::string str, bool append) {
    std::vector<std::string> tokens(2);
    boost::split(tokens, str, boost::is_any_of(",x"));
    try {
        this->value.x = boost::lexical_cast<coordf_t>(tokens[0]);
        this->value.y = boost::lexical_cast<coordf_t>(tokens[1]);
    } catch (boost::bad_lexical_cast &e){
        std::cout << "Exception caught : " << e.what() << std::endl;
        return false;
    }
    return true;
};

bool
ConfigOptionPoint3::deserialize(std::string str, bool append) {
    std::vector<std::string> tokens(3);
    boost::split(tokens, str, boost::is_any_of(",x"));
    try {
        this->value.x = boost::lexical_cast<coordf_t>(tokens[0]);
        this->value.y = boost::lexical_cast<coordf_t>(tokens[1]);
        this->value.z = boost::lexical_cast<coordf_t>(tokens[2]);
    } catch (boost::bad_lexical_cast &e){
        std::cout << "Exception caught : " << e.what() << std::endl;
        return false;
    }
    return true;
};

bool
ConfigOptionPoints::deserialize(std::string str, bool append) {
	if (!append) this->values.clear();

	std::vector<std::string> tokens;
	boost::split(tokens, str, boost::is_any_of("x,"));
	if (tokens.size() % 2) return false;

	try {
		for (size_t i = 0; i < tokens.size(); ++i) {
			Pointf point;
			point.x = boost::lexical_cast<coordf_t>(tokens[i]);
			point.y = boost::lexical_cast<coordf_t>(tokens[++i]);
			this->values.push_back(point);
		}
	} catch (boost::bad_lexical_cast &e) {
		printf("%s\n", e.what());
		return false;
	}

	return true;
}

}
