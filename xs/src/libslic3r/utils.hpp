#ifndef UTILS_HPP
#define UTILS_HPP

#include <vector>
#include <string>

/// Utility functions that aren't necessarily part of libslic3r but are used by it.

/// Separate a string based on some regular expression string.
std::vector<std::string> 
split_at_regex(const std::string& input, const std::string& regex);

#endif // UTILS_HPP
