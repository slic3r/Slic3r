#include "Log.hpp"
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>

namespace Slic3r {

std::unique_ptr<_Log> slic3r_log {_Log::make_log()};

_Log::_Log() : _out(std::clog), _wout(std::wclog) {
}

_Log::_Log(std::ostream& out) : _out(out), _wout(std::wclog) {
}
_Log::_Log(std::wostream& out) : _out(std::clog), _wout(out) {
}

void _Log::fatal_error(const std::string& topic, const std::wstring& message) {
//    _wout << this->converter.from_bytes(topic);
    _wout << std::setw(6) << "FERR" << ": ";
    _wout << message << std::endl;
}
void _Log::fatal_error(const std::string& topic, const std::string& message) {
    this->fatal_error(topic) << message << std::endl;
}
std::ostream& _Log::fatal_error(const std::string& topic) {
    _out << topic << std::setfill(' ') << std::setw(6) << "FERR" << ": ";
    return _out;
}

void _Log::error(const std::string& topic, const std::string& message) {
    this->error(topic) << message << std::endl;
}
std::ostream& _Log::error(const std::string& topic) {
    _out << topic << std::setfill(' ') << std::setw(6) << "ERR" << ": ";
    return _out;
}

void _Log::info(const std::string& topic, const std::string& message) {
    this->info(topic) << message << std::endl;
}

std::ostream& _Log::info(const std::string& topic) {
    _out << topic << std::setfill(' ') << std::setw(6) << "INFO" << ": ";
    return _out;
}

void _Log::warn(const std::string& topic, const std::string& message) {
    this->warn(topic) << message << std::endl;
}

std::ostream& _Log::warn(const std::string& topic) {
    _out << topic << std::setfill(' ') << std::setw(6) << "WARN" << ": ";
    return _out;
}

void _Log::debug(const std::string& topic, const std::string& message) {
    this->debug(topic) << message << std::endl;
}

std::ostream& _Log::debug(const std::string& topic) {
    _out << topic << std::setfill(' ') << std::setw(6) << "DEBUG" << ": ";
    return _out;
}

void _Log::raw(const std::string& message) {
    this->raw() << message << std::endl;
}
std::ostream& _Log::raw() {
    return _out;
}



} // Slic3r
