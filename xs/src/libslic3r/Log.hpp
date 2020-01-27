#ifndef slic3r_LOG_HPP
#define slic3r_LOG_HPP

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <memory>
#include <set>


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
        tmp->_inclusive_levels = true;
        tmp->set_level(log_t::ERR);
        tmp->set_level(log_t::FERR);
        tmp->set_level(log_t::WARN);
        return tmp;
    }
    static std::unique_ptr<_Log> make_log(std::ostream& out) {
        std::unique_ptr<_Log> tmp {new _Log(out)};
        tmp->_inclusive_levels = true;
        tmp->set_level(log_t::ERR);
        tmp->set_level(log_t::FERR);
        tmp->set_level(log_t::WARN);
        return tmp;
    }
    void fatal_error(const std::string& topic, const std::string& message);
    void fatal_error(const std::string& topic, const std::wstring& message);
    std::ostream& fatal_error(const std::string& topic, bool multiline = false);

    void error(const std::string& topic, const std::string& message);
    void error(const std::string& topic, const std::wstring& message);
    std::ostream& error(const std::string& topic, bool multiline = false);

    void info(const std::string& topic, const std::string& message);
    void info(const std::string& topic, const std::wstring& message);
    std::ostream& info(const std::string& topic, bool multiline = false);
    void debug(const std::string& topic, const std::string& message);
    void debug(const std::string& topic, const std::wstring& message);
    std::ostream& debug(const std::string& topic, bool multiline = false);
    void warn(const std::string& topic, const std::string& message);
    void warn(const std::string& topic, const std::wstring& message);
    std::ostream& warn(const std::string& topic, bool multiline = false);
    void raw(const std::string& message);
    void raw(const std::wstring& message);
    std::ostream& raw();

    template <class T>
    void debug_svg(const std::string& topic, const T& path, bool append = true);
    template <class T>
    void debug_svg(const std::string& topic, const T* path, bool append = true);

    void set_level(log_t level);
    void clear_level(log_t level);
    void set_inclusive(bool v) { this->_inclusive_levels = v; }
    void add_topic(const std::string& topic) { this->_topics.insert(topic); }
    void clear_topic(const std::string& topic);

//    _Log(_Log const&)            = delete;
//    void operator=(_Log const&)  = delete;
private:
    std::ostream& _out;
    _Log();
    _Log(std::ostream& out);
    bool _inclusive_levels { true };
    std::set<log_t> _log_level { };
    std::set<std::string> _topics { };

    bool _has_log_level(log_t lvl);
    bool _has_topic(const std::string& topic);

};

/// Global log reference; initialized in Log.cpp
extern std::unique_ptr<_Log> slic3r_log;

/// Static class for referencing the various logging functions. Refers to
///
class Log {
public:

    /// Logs a fatal error with Slic3r.
    /// \param topic [in] file or heading for error
    /// \param message [in] text of the logged error message
    static void fatal_error(std::string topic, std::wstring message) {
        slic3r_log->fatal_error(topic, message);
    }

    /// Logs a regular error with Slic3r.
    /// \param topic [in] file or heading for error
    /// \param message [in] text of the logged error message
    static void error(std::string topic, std::wstring message) {
        slic3r_log->error(topic, message);
    }

    /// Logs a fatal error with Slic3r.
    /// \param topic [in] file or heading for error
    /// \param message [in] text of the logged error message
    static void error(std::string topic, std::string message) {
        slic3r_log->error(topic, message);
    }

    /// Logs an informational message with Slic3r.
    /// \param topic [in] file or heading for message
    /// \param message [in] text of the logged message
    static void info(std::string topic, std::wstring message) {
        slic3r_log->info(topic, message);
    }
    /// Logs an informational message with Slic3r.
    /// \param topic [in] file or heading for message
    /// \param message [in] text of the logged message
    static void info(std::string topic, std::string message) {
        slic3r_log->info(topic, message);
    }

    /// Logs a warning message with Slic3r.
    /// \param topic [in] file or heading for message
    /// \param message [in] text of the logged message
    static void warn(std::string topic, std::wstring message) {
        slic3r_log->warn(topic, message);
    }

    /// Logs a warning message with Slic3r.
    /// \param topic [in] file or heading for message
    /// \param message [in] text of the logged message
    static void warn(std::string topic, std::string message) {
        slic3r_log->warn(topic, message);
    }

    /// Logs a debugging message with Slic3r.
    /// \param topic [in] file or heading for message
    /// \param message [in] text of the logged message
    static void debug(std::string topic, std::wstring message) {
        slic3r_log->debug(topic, message);
    }
    /// Logs a debugging message with Slic3r.
    /// \param topic [in] file or heading for message
    /// \param message [in] text of the logged message
    static void debug(std::string topic, std::string message) {
        slic3r_log->debug(topic, message);
    }


    /// Logs an error message with Slic3r.
    /// \param topic [in] file or heading for message
    /// \param multiline [in] Is this a following part of a multline output (default False)
    /// \return reference to output ostream for << chaining.
    /// \note Developer is expected to add newlines.
    static std::ostream& error(std::string topic, bool multiline = false) {
        return slic3r_log->error(topic, multiline);
    }
    /// Logs a debugging message with Slic3r.
    /// \param topic [in] file or heading for message
    /// \param multiline [in] Is this a following part of a multline output (default False)
    /// \return reference to output ostream for << chaining.
    /// \note Developer is expected to add newlines.
    static std::ostream& debug(std::string topic, bool multiline = false) {
        return slic3r_log->debug(topic, multiline);
    }

    /// Logs a warning message with Slic3r.
    /// \param topic [in] file or heading for message
    /// \param multiline [in] Is this a following part of a multline output (default False)
    /// \return reference to output ostream for << chaining.
    /// \note Developer is expected to add newlines.
    static std::ostream& warn(std::string topic, bool multiline = false) {
        return slic3r_log->warn(topic, multiline);
    }
    /// Logs an informational message with Slic3r.
    /// \param topic [in] file or heading for message
    /// \param multiline [in] Is this a following part of a multline output (default False)
    /// \return reference to output ostream for << chaining.
    /// \note Developer is expected to add newlines.
    static std::ostream& info(std::string topic, bool multiline = false) {
        return slic3r_log->info(topic, multiline);
    }

    /// Unadorned ostream output for multiline constructions.
    static std::ostream& raw() {
        return slic3r_log->raw();
    }

    /// Adds a topic to filter on with Slic3r's debug system.
    /// \param topic [in] name of topic to filter on.
    /// Only shows registered topics.
    static void add_topic(const std::string& topic) {
        slic3r_log->add_topic(topic);
    }

    /// Removes a topic from the filter list with Slic3r's debug system.
    /// \param topic [in] name of topic to remove from filter.
    /// \note Default option removes all filters.
    static void clear_topic(const std::string& topic = "") {
        slic3r_log->clear_topic(topic);
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
