#ifndef slic3r_Config_hpp_
#define slic3r_Config_hpp_

#include <map>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "libslic3r.h"
#include "Point.hpp"

namespace Slic3r {

/// Name of the configuration option.
typedef std::string t_config_option_key;
typedef std::vector<std::string> t_config_option_keys;

extern std::string escape_string_cstyle(const std::string &str);
extern std::string escape_strings_cstyle(const std::vector<std::string> &strs);
extern bool unescape_string_cstyle(const std::string &str, std::string &out);
extern bool unescape_strings_cstyle(const std::string &str, std::vector<std::string> &out);

/// \brief Public interface for configuration options. 
///
/// Defines get/set for all supported data types.
/// Default value for output values is 0 for numeric/boolean types and "" for string types.
/// Subclasses override the appropriate functions in the interface and return real data.
class ConfigOption {
    public:
    virtual ~ConfigOption() {};
    virtual ConfigOption* clone() const = 0;
    virtual std::string serialize() const = 0;
    virtual bool deserialize(std::string str, bool append = false) = 0;
    virtual void set(const ConfigOption &option) = 0;
    virtual int getInt() const { return 0; };
    virtual double getFloat() const { return 0; };
    virtual bool getBool() const { return false; };
    virtual void setInt(int val) {};
    virtual std::string getString() const { return ""; };
    friend bool operator== (const ConfigOption &a, const ConfigOption &b);
    friend bool operator!= (const ConfigOption &a, const ConfigOption &b);
};

/// Value of a single valued option (bool, int, float, string, point, enum)
template <class T>
class ConfigOptionSingle : public ConfigOption {
    public:
    T value;
    ConfigOptionSingle(T _value) : value(_value) {};
    operator T() const { return this->value; };
    
    void set(const ConfigOption &option) {
        const ConfigOptionSingle<T>* other = dynamic_cast< const ConfigOptionSingle<T>* >(&option);
        if (other != NULL) this->value = other->value;
    };
};

/// Virtual base class, represents value of a vector valued option (bools, ints, floats, strings, points)
class ConfigOptionVectorBase : public ConfigOption {
    public:
    virtual ~ConfigOptionVectorBase() {};
    virtual std::vector<std::string> vserialize() const = 0;
};

/// Value of a vector valued option (bools, ints, floats, strings, points), template
template <class T>
class ConfigOptionVector : public ConfigOptionVectorBase
{
    public:
    std::vector<T> values;
    ConfigOptionVector() {};
    ConfigOptionVector(const std::vector<T> _values) : values(_values) {};
    virtual ~ConfigOptionVector() {};
    
    void set(const ConfigOption &option) {
        const ConfigOptionVector<T>* other = dynamic_cast< const ConfigOptionVector<T>* >(&option);
        if (other != NULL) this->values = other->values;
    };
    
    T get_at(size_t i) const {
        try {
            return this->values.at(i);
        } catch (const std::out_of_range& oor) {
            return this->values.front();
        }
    };
};

/// Template specialization for a single ConfigOption
/// Internally resolves to a double.
class ConfigOptionFloat : public ConfigOptionSingle<double>
{
    public:
    ConfigOptionFloat() : ConfigOptionSingle<double>(0) {};
    ConfigOptionFloat(double _value) : ConfigOptionSingle<double>(_value) {};
    ConfigOptionFloat* clone() const { return new ConfigOptionFloat(this->value); };
    
    double getFloat() const { return this->value; };
    
    std::string serialize() const {
        std::ostringstream ss;
        ss << this->value;
        return ss.str();
    };
    
    bool deserialize(std::string str, bool append = false) {
        std::istringstream iss(str);
        iss >> this->value;
        return !iss.fail();
    };
};

/// Vector form of template specialization for floating point numbers.
class ConfigOptionFloats : public ConfigOptionVector<double>
{
    public:
    ConfigOptionFloats() {};
    ConfigOptionFloats(const std::vector<double> _values) : ConfigOptionVector<double>(_values) {};
    ConfigOptionFloats* clone() const { return new ConfigOptionFloats(this->values); };
    
    std::string serialize() const {
        std::ostringstream ss;
        for (std::vector<double>::const_iterator it = this->values.begin(); it != this->values.end(); ++it) {
            if (it - this->values.begin() != 0) ss << ",";
            ss << *it;
        }
        return ss.str();
    };
    
    std::vector<std::string> vserialize() const {
        std::vector<std::string> vv;
        vv.reserve(this->values.size());
        for (std::vector<double>::const_iterator it = this->values.begin(); it != this->values.end(); ++it) {
            std::ostringstream ss;
            ss << *it;
            vv.push_back(ss.str());
        }
        return vv;
    };
    
    bool deserialize(std::string str, bool append = false) {
        if (!append) this->values.clear();
        std::istringstream is(str);
        std::string item_str;
        while (std::getline(is, item_str, ',')) {
            std::istringstream iss(item_str);
            double value;
            iss >> value;
            this->values.push_back(value);
        }
        return true;
    };
};

class ConfigOptionInt : public ConfigOptionSingle<int>
{
    public:
    ConfigOptionInt() : ConfigOptionSingle<int>(0) {};
    ConfigOptionInt(double _value) : ConfigOptionSingle<int>(_value) {};
    ConfigOptionInt* clone() const { return new ConfigOptionInt(this->value); };
    
    int getInt() const { return this->value; };
    void setInt(int val) { this->value = val; };
    
    std::string serialize() const {
        std::ostringstream ss;
        ss << this->value;
        return ss.str();
    };
    
    bool deserialize(std::string str, bool append = false) {
        std::istringstream iss(str);
        iss >> this->value;
        return !iss.fail();
    };
};

class ConfigOptionInts : public ConfigOptionVector<int>
{
    public:
    ConfigOptionInts() {};
    ConfigOptionInts(const std::vector<int> _values) : ConfigOptionVector<int>(_values) {};
    ConfigOptionInts* clone() const { return new ConfigOptionInts(this->values); };
    
    std::string serialize() const {
        std::ostringstream ss;
        for (std::vector<int>::const_iterator it = this->values.begin(); it != this->values.end(); ++it) {
            if (it - this->values.begin() != 0) ss << ",";
            ss << *it;
        }
        return ss.str();
    };
    
    std::vector<std::string> vserialize() const {
        std::vector<std::string> vv;
        vv.reserve(this->values.size());
        for (std::vector<int>::const_iterator it = this->values.begin(); it != this->values.end(); ++it) {
            std::ostringstream ss;
            ss << *it;
            vv.push_back(ss.str());
        }
        return vv;
    };
    
    bool deserialize(std::string str, bool append = false) {
        if (!append) this->values.clear();
        std::istringstream is(str);
        std::string item_str;
        while (std::getline(is, item_str, ',')) {
            std::istringstream iss(item_str);
            int value;
            iss >> value;
            this->values.push_back(value);
        }
        return true;
    };
};

class ConfigOptionString : public ConfigOptionSingle<std::string>
{
    public:
    ConfigOptionString() : ConfigOptionSingle<std::string>("") {};
    ConfigOptionString(std::string _value) : ConfigOptionSingle<std::string>(_value) {};
    ConfigOptionString* clone() const { return new ConfigOptionString(this->value); };
    
    std::string getString() const { return this->value; };
    
    std::string serialize() const { 
        return escape_string_cstyle(this->value);
    };

    bool deserialize(std::string str, bool append = false) {
        return unescape_string_cstyle(str, this->value);
    };
};

/// semicolon-separated strings
class ConfigOptionStrings : public ConfigOptionVector<std::string>
{
    public:
    ConfigOptionStrings() {};
    ConfigOptionStrings(const std::vector<std::string> _values) : ConfigOptionVector<std::string>(_values) {};
    ConfigOptionStrings* clone() const { return new ConfigOptionStrings(this->values); };
    
    std::string serialize() const {
        return escape_strings_cstyle(this->values);
    };
    
    std::vector<std::string> vserialize() const {
        return this->values;
    };
    
    bool deserialize(std::string str, bool append = false) {
        if (!append) this->values.clear();
        return unescape_strings_cstyle(str, this->values);
    };
};

/// \brief Specialized floating point class to represent some percentage value of 
/// another numeric configuration option.
class ConfigOptionPercent : public ConfigOptionFloat
{
    public:
    ConfigOptionPercent() : ConfigOptionFloat(0) {};
    ConfigOptionPercent(double _value) : ConfigOptionFloat(_value) {};
    ConfigOptionPercent* clone() const { return new ConfigOptionPercent(this->value); };
    
    /// Calculate the value of this option as it relates to some 
    /// other numerical value.
    double get_abs_value(double ratio_over) const {
        return ratio_over * this->value / 100;
    };
    
    std::string serialize() const {
        std::ostringstream ss;
        ss << this->value;
        std::string s(ss.str());
        s += "%";
        return s;
    };
    
    bool deserialize(std::string str, bool append = false) {
        // don't try to parse the trailing % since it's optional
        std::istringstream iss(str);
        iss >> this->value;
        return !iss.fail();
    };
};


/// Combination class that can store a raw float or a percentage
/// value. Includes a flag to indicate how it should be interpreted.
class ConfigOptionFloatOrPercent : public ConfigOptionPercent
{
    public:
    bool percent;
    ConfigOptionFloatOrPercent() : ConfigOptionPercent(0), percent(false) {};
    ConfigOptionFloatOrPercent(double _value, bool _percent)
        : ConfigOptionPercent(_value), percent(_percent) {};
    ConfigOptionFloatOrPercent* clone() const { return new ConfigOptionFloatOrPercent(this->value, this->percent); };
    
    void set(const ConfigOption &option) {
        const ConfigOptionFloatOrPercent* other = dynamic_cast< const ConfigOptionFloatOrPercent* >(&option);
        if (other != NULL) {
            this->value = other->value;
            this->percent = other->percent;
        }
    };
    
    double get_abs_value(double ratio_over) const {
        if (this->percent) {
            return ratio_over * this->value / 100;
        } else {
            return this->value;
        }
    };
    
    std::string serialize() const {
        std::ostringstream ss;
        ss << this->value;
        std::string s(ss.str());
        if (this->percent) s += "%";
        return s;
    };
    
    bool deserialize(std::string str, bool append = false) {
        this->percent = str.find_first_of("%") != std::string::npos;
        std::istringstream iss(str);
        iss >> this->value;
        return !iss.fail();
    };
};

/// \brief Configuration option to store a 2D (x,y) tuple. 
class ConfigOptionPoint : public ConfigOptionSingle<Pointf>
{
    public:
    ConfigOptionPoint() : ConfigOptionSingle<Pointf>(Pointf(0,0)) {};
    ConfigOptionPoint(Pointf _value) : ConfigOptionSingle<Pointf>(_value) {};
    ConfigOptionPoint* clone() const { return new ConfigOptionPoint(this->value); };
    
    std::string serialize() const {
        std::ostringstream ss;
        ss << this->value.x;
        ss << ",";
        ss << this->value.y;
        return ss.str();
    };
    
    bool deserialize(std::string str, bool append = false);
};

/// \brief Configuration option to store a 3D (x,y,z) tuple. 
class ConfigOptionPoint3 : public ConfigOptionSingle<Pointf3>
{
    public:
    ConfigOptionPoint3() : ConfigOptionSingle<Pointf3>(Pointf3(0,0,0)) {};
    ConfigOptionPoint3(Pointf3 _value) : ConfigOptionSingle<Pointf3>(_value) {};
    ConfigOptionPoint3* clone() const { return new ConfigOptionPoint3(this->value); };
    
    std::string serialize() const {
        std::ostringstream ss;
        ss << this->value.x;
        ss << ",";
        ss << this->value.y;
        ss << ",";
        ss << this->value.z;
        return ss.str();
    };
    
    bool deserialize(std::string str, bool append = false);
    
    bool is_positive_volume () {
        return this->value.x > 0 && this->value.y > 0 && this->value.z > 0;
    };
};

class ConfigOptionPoints : public ConfigOptionVector<Pointf>
{
    public:
    ConfigOptionPoints() {};
    ConfigOptionPoints(const std::vector<Pointf> _values) : ConfigOptionVector<Pointf>(_values) {};
    ConfigOptionPoints* clone() const { return new ConfigOptionPoints(this->values); };
    
    std::string serialize() const {
        std::ostringstream ss;
        for (Pointfs::const_iterator it = this->values.begin(); it != this->values.end(); ++it) {
            if (it - this->values.begin() != 0) ss << ",";
            ss << it->x;
            ss << "x";
            ss << it->y;
        }
        return ss.str();
    };
    
    std::vector<std::string> vserialize() const {
        std::vector<std::string> vv;
        for (Pointfs::const_iterator it = this->values.begin(); it != this->values.end(); ++it) {
            std::ostringstream ss;
            ss << *it;
            vv.push_back(ss.str());
        }
        return vv;
    };
    
    bool deserialize(std::string str, bool append = false);
};


/// \brief Represents a boolean flag 
class ConfigOptionBool : public ConfigOptionSingle<bool>
{
    public:
    ConfigOptionBool() : ConfigOptionSingle<bool>(false) {};
    ConfigOptionBool(bool _value) : ConfigOptionSingle<bool>(_value) {};
    ConfigOptionBool* clone() const { return new ConfigOptionBool(this->value); };
    
    bool getBool() const { return this->value; };
    
    std::string serialize() const {
        return std::string(this->value ? "1" : "0");
    };
    
    bool deserialize(std::string str, bool append = false) {
        this->value = (str.compare("1") == 0);
        return true;
    };
};

class ConfigOptionBools : public ConfigOptionVector<bool>
{
    public:
    ConfigOptionBools() {};
    ConfigOptionBools(const std::vector<bool> _values) : ConfigOptionVector<bool>(_values) {};
    ConfigOptionBools* clone() const { return new ConfigOptionBools(this->values); };
    
    std::string serialize() const {
        std::ostringstream ss;
        for (std::vector<bool>::const_iterator it = this->values.begin(); it != this->values.end(); ++it) {
            if (it - this->values.begin() != 0) ss << ",";
            ss << (*it ? "1" : "0");
        }
        return ss.str();
    };
    
    std::vector<std::string> vserialize() const {
        std::vector<std::string> vv;
        for (std::vector<bool>::const_iterator it = this->values.begin(); it != this->values.end(); ++it) {
            std::ostringstream ss;
            ss << (*it ? "1" : "0");
            vv.push_back(ss.str());
        }
        return vv;
    };
    
    bool deserialize(std::string str, bool append = false) {
        if (!append) this->values.clear();
        std::istringstream is(str);
        std::string item_str;
        while (std::getline(is, item_str, ',')) {
            this->values.push_back(item_str.compare("1") == 0);
        }
        return true;
    };
};

/// Map from an enum name to an enum integer value.
typedef std::map<std::string,int> t_config_enum_values;

/// \brief Templated enumeration representation. 
template <class T>
class ConfigOptionEnum : public ConfigOptionSingle<T>
{
    public:
    // by default, use the first value (0) of the T enum type
    ConfigOptionEnum() : ConfigOptionSingle<T>(static_cast<T>(0)) {};
    ConfigOptionEnum(T _value) : ConfigOptionSingle<T>(_value) {};
    ConfigOptionEnum<T>* clone() const { return new ConfigOptionEnum<T>(this->value); };
    
    std::string serialize() const {
        t_config_enum_values enum_keys_map = ConfigOptionEnum<T>::get_enum_values();
        for (t_config_enum_values::iterator it = enum_keys_map.begin(); it != enum_keys_map.end(); ++it) {
            if (it->second == static_cast<int>(this->value)) return it->first;
        }
        return "";
    };

    bool deserialize(std::string str, bool append = false) {
        t_config_enum_values enum_keys_map = ConfigOptionEnum<T>::get_enum_values();
        if (enum_keys_map.count(str) == 0) return false;
        this->value = static_cast<T>(enum_keys_map[str]);
        return true;
    };

    /// Map from an enum name to an enum integer value.
    //FIXME The map is called often, it shall be initialized statically.
    static t_config_enum_values get_enum_values();
};

/// \brief Generic enum configuration value.
///
/// We use this one in DynamicConfig objects when creating a config value object for ConfigOptionType == coEnum.
/// In the StaticConfig, it is better to use the specialized ConfigOptionEnum<T> containers.
class ConfigOptionEnumGeneric : public ConfigOptionInt
{
    public:
    const t_config_enum_values* keys_map;
    
    std::string serialize() const {
        for (t_config_enum_values::const_iterator it = this->keys_map->begin(); it != this->keys_map->end(); ++it) {
            if (it->second == this->value) return it->first;
        }
        return "";
    };

    bool deserialize(std::string str, bool append = false) {
        if (this->keys_map->count(str) == 0) return false;
        this->value = (*const_cast<t_config_enum_values*>(this->keys_map))[str];
        return true;
    };
};

/// Type of a configuration value.
enum ConfigOptionType {
    coNone,
    /// single float
    coFloat,
    /// vector of floats
    coFloats,
    /// single int
    coInt,
    /// vector of ints
    coInts,
    /// single string
    coString,
    /// vector of strings
    coStrings,
    /// percent value. Currently only used for infill.
    coPercent,
    /// a fraction or an absolute value
    coFloatOrPercent,
    /// single 2d point. Currently not used.
    coPoint,
    /// vector of 2d points. Currently used for the definition of the print bed and for the extruder offsets.
    coPoints,
    coPoint3,
    /// single boolean value
    coBool,
    /// vector of boolean values
    coBools,
    /// a generic enum
    coEnum,
};

/// Definition of a configuration value for the purpose of GUI presentation, editing, value mapping and config file handling.
class ConfigOptionDef
{
    public:
    /// \brief Type of option referenced.
    ///
    /// The following (and any vector version) are supported:
    /// \sa ConfigOptionFloat
    /// \sa ConfigOptionInt
    /// \sa ConfigOptionString
    /// \sa ConfigOptionPercent
    /// \sa ConfigOptionFloatOrPercent
    /// \sa ConfigOptionPoint
    /// \sa ConfigOptionBool
    /// \sa ConfigOptionPoint3
    /// \sa ConfigOptionBool
    ConfigOptionType type;
    /// \brief Default value of this option. 
    ///
    /// The default value object is owned by ConfigDef, it is released in its destructor.
    ConfigOption* default_value;

    /// \brief Specialization to indicate to the GUI what kind of control is more appropriate.
    ///
    /// Usually empty. 
    /// Special values - "i_enum_open", "f_enum_open" to provide combo box for int or float selection,
    /// "select_open" - to open a selection dialog (currently only a serial port selection).
    std::string gui_type;
    /// The flags may be combined.
    /// "show_value" - even if enum_values / enum_labels are set, still display the value, not the enum label.
    /// "align_label_right" - align label to right
    std::string gui_flags;
    /// Label of the GUI input field.
    /// In case the GUI input fields are grouped in some views, the label defines a short label of a grouped value,
    /// while full_label contains a label of a stand-alone field.
    /// The full label is shown, when adding an override parameter for an object or a modified object.
    std::string label;
    std::string full_label;
    /// Category of a configuration field, from the GUI perspective.
    /// One of: "Layers and Perimeters", "Infill", "Support material", "Speed", "Extruders", "Advanced", "Extrusion Width"
    std::string category;
    /// A tooltip text shown in the GUI.
    std::string tooltip;
    /// Text right from the input field, usually a unit of measurement.
    std::string sidetext;
    /// Format of this parameter on a command line.
    std::string cli;
    /// Set for type == coFloatOrPercent.
    /// It provides a link to a configuration value, of which this option provides a ratio.
    /// For example, 
    /// For example external_perimeter_speed may be defined as a fraction of perimeter_speed.
    t_config_option_key ratio_over;
    /// True for multiline strings.
    bool multiline;
    /// For text input: If true, the GUI text box spans the complete page width.
    bool full_width;

    /// This configuration item is not editable. 
    /// Currently only used for the display of the number of threads.
    bool readonly;
    /// Height of a multiline GUI text box.
    int height;
    /// Optional width of an input field.
    int width;
    /// <min, max> limit of a numeric input.
    /// If not set, the <min, max> is set to <INT_MIN, INT_MAX>
    /// By setting min=0, only nonnegative input is allowed.
    int min;
    int max;

    /// Legacy names for this configuration option.
    /// Used when parsing legacy configuration file.
    std::vector<t_config_option_key> aliases;
    /// Sometimes a single value may well define multiple values in a "beginner" mode.
    /// Currently used for aliasing "solid_layers" to "top_solid_layers", "bottom_solid_layers".
    std::vector<t_config_option_key> shortcut;
    /// Definition of values / labels for a combo box.
    /// Mostly used for enums (when type == coEnum), but may be used for ints resp. floats, if gui_type is set to "i_enum_open" resp. "f_enum_open".
    std::vector<std::string> enum_values;
    std::vector<std::string> enum_labels;
    /// For enums (when type == coEnum). Maps enum_values to enums.
    /// Initialized by ConfigOptionEnum<xxx>::get_enum_values()
    t_config_enum_values enum_keys_map;

    ConfigOptionDef() : type(coNone), default_value(NULL),
                        multiline(false), full_width(false), readonly(false),
                        height(-1), width(-1), min(INT_MIN), max(INT_MAX) {};
    ConfigOptionDef(const ConfigOptionDef &other);
    ~ConfigOptionDef();
    
    private:
    ConfigOptionDef& operator= (ConfigOptionDef other);
};

/// Map from a config option name to its definition.
//i The definition does not carry an actual value of the config option, only its constant default value.
//i t_config_option_key is std::string
typedef std::map<t_config_option_key,ConfigOptionDef> t_optiondef_map;

/// Definition of configuration values for the purpose of GUI presentation, editing, value mapping and config file handling.
/// The configuration definition is static: It does not carry the actual configuration values,
/// but it carries the defaults of the configuration values.
class ConfigDef
{
    public:
    t_optiondef_map options;
    ConfigOptionDef* add(const t_config_option_key &opt_key, ConfigOptionType type);
    ConfigOptionDef* add(const t_config_option_key &opt_key, const ConfigOptionDef &def);
    bool has(const t_config_option_key &opt_key) const;
    const ConfigOptionDef* get(const t_config_option_key &opt_key) const;
    void merge(const ConfigDef &other);
};

/// An abstract configuration store.
class ConfigBase
{
    public:
    /// Definition of configuration values for the purpose of GUI presentation, editing, value mapping and config file handling.
    /// The configuration definition is static: It does not carry the actual configuration values,
    /// but it carries the defaults of the configuration values.
    /// ConfigBase does not own ConfigDef, it only references it.
    const ConfigDef* def;
    
    ConfigBase() : def(NULL) {};
    ConfigBase(const ConfigDef* def) : def(def) {};
    virtual ~ConfigBase() {};
    bool has(const t_config_option_key &opt_key) const;
    const ConfigOption* option(const t_config_option_key &opt_key) const;
    ConfigOption* option(const t_config_option_key &opt_key, bool create = false);
    template<class T> T* opt(const t_config_option_key &opt_key, bool create = false) {
        return dynamic_cast<T*>(this->option(opt_key, create));
    };
    template<class T> const T* opt(const t_config_option_key &opt_key) const {
        return dynamic_cast<const T*>(this->option(opt_key));
    };
    virtual ConfigOption* optptr(const t_config_option_key &opt_key, bool create = false) = 0;
    virtual t_config_option_keys keys() const = 0;
    void apply(const ConfigBase &other, bool ignore_nonexistent = false);
    void apply_only(const ConfigBase &other, const t_config_option_keys &opt_keys, bool ignore_nonexistent = false);
    bool equals(const ConfigBase &other) const;
    t_config_option_keys diff(const ConfigBase &other) const;
    std::string serialize(const t_config_option_key &opt_key) const;
    virtual bool set_deserialize(t_config_option_key opt_key, std::string str, bool append = false);
    double get_abs_value(const t_config_option_key &opt_key) const;
    double get_abs_value(const t_config_option_key &opt_key, double ratio_over) const;
    void setenv_();
    void load(const std::string &file);
    void save(const std::string &file) const;
};

/// Configuration store with dynamic number of configuration values.
/// In Slic3r, the dynamic config is mostly used at the user interface layer.
class DynamicConfig : public virtual ConfigBase
{
    public:
    DynamicConfig() {};
    DynamicConfig(const ConfigDef* def) : ConfigBase(def) {};
    DynamicConfig(const DynamicConfig& other);
    DynamicConfig& operator= (DynamicConfig other);
    void swap(DynamicConfig &other);
    virtual ~DynamicConfig();
    virtual ConfigOption* optptr(const t_config_option_key &opt_key, bool create = false);
    t_config_option_keys keys() const;
    void erase(const t_config_option_key &opt_key);
    void clear();
    bool empty() const;
    void read_cli(const std::vector<std::string> &tokens, t_config_option_keys* extra);
    void read_cli(int argc, char** argv, t_config_option_keys* extra);
    
    private:
    typedef std::map<t_config_option_key,ConfigOption*> t_options_map;
    t_options_map options;
};

/// Configuration store with a static definition of configuration values.
/// In Slic3r, the static configuration stores are during the slicing / g-code generation for efficiency reasons,
/// because the configuration values could be accessed directly.
class StaticConfig : public virtual ConfigBase
{
    public:
    StaticConfig() : ConfigBase() {};
    /// Gets list of config option names for each config option of this->def, which has a static counter-part defined by the derived object
    /// and which could be resolved by this->optptr(key) call.
    t_config_option_keys keys() const;
    /// Set all statically defined config options to their defaults defined by this->def.
    void set_defaults();
    /// The derived class has to implement optptr to resolve a static configuration value.
    /// virtual ConfigOption* optptr(const t_config_option_key &opt_key, bool create = false) = 0;
};

/// Specialization of std::exception to indicate that an unknown config option has been encountered.
class UnknownOptionException : public std::exception {};

}

#endif
