/** 
 * https://github.com/alexrj/Slic3r/wiki/Conditional-Gcode-Syntax-Spec
 * 
 */

#include <iostream>
#include <sstream>
#include <string>

#include <exprtk/exprtk.hpp>

// Valid start tokens
// {, {if
// 
// Valid end tokens
// }
//
// Special case:
//
// {if is special, it indicates that the rest of the line is dropped (ignored) if 
// it evaluates to False/0.


namespace Slic3r {
using gcode_recurse_t = std::pair < std::string, bool >;

/// Evaluate operators
template <typename T>
std::string evaluate(const std::string& expression_string) {
    std::stringstream result;
    typedef exprtk::symbol_table<T> symbol_table_t;
    typedef exprtk::expression<T>     expression_t;
    typedef exprtk::parser<T>             parser_t;

    #if SLIC3R_DEBUG
    std::cerr << "Evaluating expression: " << expression_string << std::endl;
    #endif
    T num_result = T(0);
    if ( exprtk::compute(expression_string, num_result)) { 
        result << num_result;
    } else {
        #if SLIC3R_DEBUG
        std::cerr << "Failed to parse: " << expression_string.c_str() << std::endl;
        #endif
    }

    return result.str();
} 

/// Parse an expression and return a string. We assume that PlaceholderParser has expanded all variables. 
std::pair <std::string, bool> expression(const std::string& input, const bool cond = false, const int depth = 0) {
    // check for subexpressions first. 
    std::string buffer(input);
    std::stringstream tmp;

    bool is_conditional = cond;

    auto open_bracket = std::count(buffer.begin(), buffer.end(), '{');
    auto close_bracket = std::count(buffer.begin(), buffer.end(), '}');
    if (open_bracket != close_bracket) return gcode_recurse_t(buffer, cond);

    auto i = 0;
    #ifdef SLIC3R_DEBUG
    std::cerr << "depth " << depth << " input str: " << input << std::endl;
    #endif
    if (open_bracket == 0 && depth > 0) { // no subexpressions, resolve operators.
        
        return gcode_recurse_t(evaluate<double>(buffer), cond);
    }
    while (open_bracket > 0) {
        // a subexpression has been found, find the end of it.
        // find the last open bracket, then the first open bracket after it.
        size_t pos_if = buffer.rfind("{if");
        size_t pos    = buffer.rfind("{");
        size_t shift_if = ( pos_if >= pos && pos_if < buffer.size()  ? 3 : 1 );
        is_conditional = (cond || shift_if == 3); // conditional statement
        pos_if = (pos_if > buffer.size() ? pos : pos_if);

        pos = (pos_if > pos ? pos_if : pos);

        #ifdef SLIC3R_DEBUG
        std::cerr << "depth " << depth << " loop " << i << " pos: " << pos << std::endl;
        #endif

        // find the first bracket after the position
        size_t end_pos = buffer.find("}", pos);

        if (end_pos > buffer.size()) return gcode_recurse_t(buffer, cond); // error!
        if (pos > 0) 
            tmp << buffer.substr(0, pos);
        gcode_recurse_t retval = expression(buffer.substr(pos+shift_if, ( end_pos - ( pos+shift_if))), is_conditional, depth+1);
        is_conditional = retval.second;
        tmp << retval.first;
        if (end_pos < buffer.size()-1)  // special case, last } is final char
            tmp << buffer.substr(end_pos+1, buffer.size() - (end_pos));
        buffer = tmp.str();
        tmp = std::stringstream();

        #ifdef SLIC3R_DEBUG
        std::cerr << "depth: " << depth <<" Result from loop " << i << ": " << buffer << std::endl;
        #endif

        open_bracket = std::count(buffer.begin(), buffer.end(), '{');
        close_bracket = std::count(buffer.begin(), buffer.end(), '}');
        i++;
    }
    return gcode_recurse_t(buffer, is_conditional);
}

std::string process(const std::string& input) {
    // parse line from beginning
    return expression(input).first;
}
}
