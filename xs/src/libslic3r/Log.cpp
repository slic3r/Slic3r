#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
#include <algorithm>

// Boost
#include <boost/locale.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include "Log.hpp"

/// Local class to suppress output
class NullStream : public std::streambuf
{
public:
    int overflow(int c) { return c; }
};

namespace Slic3r {

static NullStream log_null;
static std::ostream null_log(&log_null);

std::unique_ptr<_Log> slic3r_log {_Log::make_log()};

_Log::_Log() : _out(std::clog) {
}

_Log::_Log(std::ostream& out) : _out(out) {
}

bool _Log::_has_log_level(log_t lvl) {
    if (!this->_inclusive_levels && this->_log_level.find(lvl) != this->_log_level.end()) {
        return true;
    } else if (this->_inclusive_levels && *(std::max_element(this->_log_level.cbegin(), this->_log_level.cend())) >= lvl) {
        return true;
    }
    return false;
}

bool _Log::_has_topic(const std::string& topic) {
    return this->_topics.find(topic) != this->_topics.end() || this->_topics.size() == 0;
}

void _Log::fatal_error(const std::string& topic, const std::wstring& message) { this->fatal_error(topic, boost::locale::conv::utf_to_utf<char>(message)); }
void _Log::error(const std::string& topic, const std::wstring& message) { this->error(topic, boost::locale::conv::utf_to_utf<char>(message)); }
void _Log::warn(const std::string& topic, const std::wstring& message) { this->warn(topic, boost::locale::conv::utf_to_utf<char>(message)); }
void _Log::info(const std::string& topic, const std::wstring& message) { this->info(topic, boost::locale::conv::utf_to_utf<char>(message)); }
void _Log::debug(const std::string& topic, const std::wstring& message) { this->debug(topic, boost::locale::conv::utf_to_utf<char>(message)); }

void _Log::fatal_error(const std::string& topic, const std::string& message) {
    this->fatal_error(topic) << message << std::endl;
}
std::ostream& _Log::fatal_error(const std::string& topic, bool multiline) {
    if (this->_has_log_level(log_t::FERR) && this->_has_topic(topic)) {
        if (!multiline)
            _out << topic << std::setfill(' ') << std::setw(6) << "FERR" << ": ";
        return _out;
    }
    return null_log;
}

void _Log::error(const std::string& topic, const std::string& message) {
    this->error(topic) << message << std::endl;
}
std::ostream& _Log::error(const std::string& topic, bool multiline) {
    if (this->_has_log_level(log_t::ERR) && this->_has_topic(topic)) {
        if (!multiline)
            _out << topic << std::setfill(' ') << std::setw(6) << "ERR" << ": ";
        return _out;
    }
    return null_log;
}

void _Log::info(const std::string& topic, const std::string& message) {
    this->info(topic) << message << std::endl;
}

std::ostream& _Log::info(const std::string& topic, bool multiline) {
    if (this->_has_log_level(log_t::INFO) && this->_has_topic(topic)) {
        if (!multiline)
            _out << topic << std::setfill(' ') << std::setw(6) << "INFO" << ": ";
        return _out;
    }
    return null_log;
}

void _Log::warn(const std::string& topic, const std::string& message) {
    this->warn(topic) << message << std::endl;
}

std::ostream& _Log::warn(const std::string& topic, bool multiline) {
    if (this->_has_log_level(log_t::WARN) && this->_has_topic(topic)) {
        if (!multiline)
            _out << topic << std::setfill(' ') << std::setw(6) << "WARN" << ": ";
        return _out;
    }
    return null_log;
}

void _Log::debug(const std::string& topic, const std::string& message) {
    this->debug(topic) << message << std::endl;
}

std::ostream& _Log::debug(const std::string& topic, bool multiline) {
    if (this->_has_log_level(log_t::DEBUG) && this->_has_topic(topic)) {
        if (!multiline)
            _out << topic << std::setfill(' ') << std::setw(6) << "DEBUG" << ": ";
        return _out;
    }
    return null_log;
}

void _Log::raw(const std::string& message) {
    this->raw() << message << std::endl;
}
void _Log::raw(const std::wstring& message) { this->raw(boost::locale::conv::utf_to_utf<char>(message)); }

std::ostream& _Log::raw() {
    return _out;
}


void _Log::set_level(log_t level) {
    if (this->_inclusive_levels) {
        this->_log_level.clear();
        this->_log_level.insert(level);
    } else if (level == log_t::ALL) {
        this->_log_level.insert(log_t::FERR);
        this->_log_level.insert(log_t::ERR);
        this->_log_level.insert(log_t::WARN);
        this->_log_level.insert(log_t::INFO);
        this->_log_level.insert(log_t::DEBUG);
    } else {
        this->_log_level.insert(level);
    }
}
void _Log::clear_level(log_t level) {
    if (level == log_t::ALL) {
        this->_log_level.clear();
    } else {
        if (this->_log_level.find(level) != this->_log_level.end())
            this->_log_level.erase(level);
    }
}

void _Log::clear_topic(const std::string& topic) {
    if (topic == "") {
        this->_topics.clear();
    } else {
        if (this->_topics.find(topic) != this->_topics.end()) this->_topics.erase(topic);
    }
}

} // Slic3r
