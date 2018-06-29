#ifndef slic3r_LOG_HPP
#define slic3r_LOG_HPP

#include <string>
#include <vector>
#include <sstream>
#include <iostream>

namespace Slic3r {

class Log {
public:
    static void fatal_error(std::string topic, std::wstring message) {
        std::cerr << topic << " FERR" << ": ";
        std::wcerr << message << std::endl;
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
