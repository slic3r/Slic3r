#ifndef slic3r_LOG_HPP
#define slic3r_LOG_HPP

#include <string>

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

    static void info(std::string topic, std::wstring message) {
        std::clog << topic << " INFO" << ": ";
        std::wclog << message << std::endl;
    }

    static void warn(std::string topic, std::wstring message) {
        std::cerr << topic << " WARN" << ": ";
        std::wcerr << message << std::endl;
    }

};

}

#endif // slic3r_LOG_HPP
