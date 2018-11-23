#ifndef slic3r_LOG_HPP
#define slic3r_LOG_HPP

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <memory>
#include <locale>
#include <set>
#include <codecvt> // good until c++17


namespace Slic3r {

/// All available logging levels.
enum class log_t : uint8_t { FERR = 0, ERR = 4, WARN = 8, INFO = 16, DEBUG = 32, ALL = 255 };

inline bool operator>(const log_t lhs, const log_t rhs) { return static_cast<uint8_t>(lhs) > static_cast<uint8_t>(rhs); }
inline bool operator<(const log_t lhs, const log_t rhs) { return static_cast<uint8_t>(lhs) < static_cast<uint8_t>(rhs); }
inline bool operator>=(const log_t lhs, const log_t rhs) { return static_cast<uint8_t>(lhs) > static_cast<uint8_t>(rhs) || lhs == rhs; }
inline bool operator<=(const log_t lhs, const log_t rhs) { return static_cast<uint8_t>(lhs) < static_cast<uint8_t>(rhs) || lhs == rhs; }

/// Singleton instance implementing logging functionality in Slic3r
/// Basic functionality is stubbed in currently, may pass through to Boost::Log
/// eventually.
class _Log {
public:
    static std::unique_ptr<_Log> make_log() {
        std::unique_ptr<_Log> tmp {new _Log()};
        return tmp;
    }
    static std::unique_ptr<_Log> make_log(std::ostream& out) {
        std::unique_ptr<_Log> tmp {new _Log(out)};
        return tmp;
    }
    static std::unique_ptr<_Log> make_log(std::wostream& out) {
        std::unique_ptr<_Log> tmp {new _Log(out)};
        return tmp;
    }
    void fatal_error(const std::string& topic, const std::wstring& message);
    void fatal_error(const std::string& topic, const std::string& message);
    std::ostream& fatal_error(const std::string& topic);

    void error(const std::string& topic, const std::wstring& message);
    void error(const std::string& topic, const std::string& message);
    std::ostream& error(const std::string& topic);

    void info(const std::string& topic, const std::string& message);
    std::ostream& info(const std::string& topic);
    void debug(const std::string& topic, const std::string& message);
    std::ostream& debug(const std::string& topic);
    void warn(const std::string& topic, const std::string& message);
    std::ostream& warn(const std::string& topic);
    void raw(const std::string& message);
    std::ostream& raw();

    template <class T>
    void debug_svg(const std::string& topic, const T& path, bool append = true);
    template <class T>
    void debug_svg(const std::string& topic, const T* path, bool append = true);

    void set_level(log_t level);
    void clear_level(log_t level);
    void set_inclusive(bool v) { this->_inclusive_levels = v; }

//    _Log(_Log const&)            = delete;
//    void operator=(_Log const&)  = delete;
private:
    std::ostream& _out;
    std::wostream& _wout;
    _Log();
    _Log(std::ostream& out);
    _Log(std::wostream& out);
    bool _inclusive_levels { true };
    std::set<log_t> _log_level { };

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    bool _has_log_level(log_t lvl);

};

/// Global log reference; initialized in Log.cpp
extern std::unique_ptr<_Log> slic3r_log;

/// Static class for referencing the various logging functions. Refers to
///
class Log {
public:
    static void fatal_error(std::string topic, std::wstring message) {
        slic3r_log->fatal_error(topic, message);
    }
    static void error(std::string topic, std::wstring message) {
        std::cerr << topic << "  ERR" << ": ";
        std::wcerr << message << std::endl;
    }

    static void error(std::string topic, std::string message) {
        std::cerr << topic << "  ERR" << ": ";
        std::cerr << message << std::endl;
    }

    static void info(std::string topic, std::wstring message) {
        std::clog << topic << " INFO" << ": ";
        std::wclog << message << std::endl;
    }
    static void info(std::string topic, std::string message) {
        std::clog << topic << " INFO" << ": ";
        std::clog << message << std::endl;
    }

    static void warn(std::string topic, std::wstring message) {
        std::cerr << topic << " WARN" << ": ";
        std::wcerr << message << std::endl;
    }
    static void warn(std::string topic, std::string message) {
        std::cerr << topic << " WARN" << ": ";
        std::cerr << message << std::endl;
    }

    static void debug(std::string topic, std::wstring message) {
        std::cerr << topic << " DEBUG" << ": ";
        std::wcerr << message << std::endl;
    }
    static void debug(std::string topic, std::string message) {
        std::cerr << topic << " DEBUG" << ": ";
        std::cerr << message << std::endl;
    }
    static std::ostream& error(std::string topic) {
        std::cerr << topic << "   ERR" << ": ";
        return std::cerr;
    }
    static std::ostream& debug(std::string topic) {
        std::cerr << topic << " DEBUG" << ": ";
        return std::cerr;
    }

    static std::ostream& warn(std::string topic) {
        std::cerr << topic << "  WARN" << ": ";
        return std::cerr;
    }
    static std::ostream& info(std::string topic) {
        std::cerr << topic << "  INFO" << ": ";
        return std::cerr;
    }

    /// Unadorned ostream output for multiline constructions.
    static std::ostream& raw() {
        return std::cerr;
    }
};

/// Utility debug function to transform a std::vector of anything that
/// supports ostream& operator<<() into a std::string.
template <typename T>
std::string
log_string(const std::vector<T>& in)
{
    std::stringstream ss;
    bool first {true};
    ss << "[ ";
    for (auto& c : in) {
        if (!first) {
            ss << ", ";
        }
        ss << c;
        first = false;
    }

    ss << " ]";

    return ss.str();
}

}

#endif // slic3r_LOG_HPP
