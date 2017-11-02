#include <string>

#include <sstream>
#include <exprtk/exprtk.hpp>
#include "ConditionalGcode.hpp"
namespace Slic3r {

// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}
/// Start of recursive function to parse gcode file.
std::string apply_math(const std::string& input) {
    return expression(input);
}

/// Evaluate expressions with exprtk
/// Everything must resolve to a number.
std::string evaluate(const std::string& expression_string) {
    std::stringstream result;

    #if SLIC3R_DEBUG
    std::cerr << __FILE__ << ":" << __LINE__ << " "<< "Evaluating expression: " << expression_string << std::endl;
    #endif
    double num_result = double(0);
    if ( exprtk::compute(expression_string, num_result)) { 
        result << num_result;
    } else {
        #if SLIC3R_DEBUG
        std::cerr << __FILE__ << ":" << __LINE__ << " "<< "Failed to parse: " << expression_string.c_str() << std::endl;
        #endif
    }
    std::string output = result.str();
    trim(output);
    return output;
} 


/// Parse an expression and return a string. We assume that PlaceholderParser has expanded all variables. 
std::string expression(const std::string& input, const int depth) {
    // check for subexpressions first. 
    std::string buffer(input);
    std::stringstream tmp;

    bool is_conditional = false;

    auto open_bracket = std::count(buffer.begin(), buffer.end(), '{');
    auto close_bracket = std::count(buffer.begin(), buffer.end(), '}');
    if (open_bracket != close_bracket) return buffer;

    auto i = 0;
    #ifdef SLIC3R_DEBUG
    std::cerr << __FILE__ << ":" << __LINE__ << " " << "depth " << depth << " input str: " << input << std::endl;
    #endif
    if (open_bracket == 0 && depth > 0) { // no subexpressions, resolve operators.
        
        return evaluate(buffer);
    }
    while (open_bracket > 0) {
        // a subexpression has been found, find the end of it.
        // find the last open bracket, then the first open bracket after it.
        size_t pos_if = buffer.rfind("{if");
        size_t pos    = buffer.rfind("{");
        size_t shift_if = ( pos_if >= pos && pos_if < buffer.size()  ? 3 : 1 );
        is_conditional = (shift_if == 3); // conditional statement
        pos_if = (pos_if > buffer.size() ? pos : pos_if);

        pos = (pos_if > pos ? pos_if : pos);

        #ifdef SLIC3R_DEBUG
        std::cerr << __FILE__ << ":" << __LINE__ << " "<< "depth " << depth << " loop " << i << " pos: " << pos << std::endl;
        #endif

        // find the first bracket after the position
        size_t end_pos = buffer.find("}", pos);

        if (end_pos > buffer.size()) return buffer; // error!
        if (pos > 0) 
            tmp << buffer.substr(0, pos);

        std::string retval = expression(buffer.substr(pos+shift_if, ( end_pos - ( pos+shift_if))), depth+1);

        #ifdef SLIC3R_DEBUG
        if (is_conditional) {
            std::cerr << __FILE__ << ":" << __LINE__ << " "<< "depth " << depth << " loop " << i << " return: '" << retval  << "'" << std::endl;
        }
        #endif

        if (is_conditional && retval == "0") {
            is_conditional = false;
            end_pos = buffer.find('\n', pos); // drop everything to the next line
        } else if (!is_conditional) {
            tmp << retval;
        } // in either case, don't print the output from {if}

        if (end_pos < buffer.size()-1)  // special case, last } is final char
            tmp << buffer.substr(end_pos+1, buffer.size() - (end_pos));
        buffer = tmp.str();

        // flush the internal string.
        tmp.str(std::string());

        #ifdef SLIC3R_DEBUG
        std::cerr << __FILE__ << ":" << __LINE__ << " "<< "depth: " << depth <<" Result from loop " << i << ": " << buffer << std::endl;
        #endif

        open_bracket = std::count(buffer.begin(), buffer.end(), '{');
        close_bracket = std::count(buffer.begin(), buffer.end(), '}');
        i++;
    }
    // {if that resolves to false/0 signifies dropping everything up to the next newline from the input buffer.
    // Also remove the result of the {if itself.
    return buffer;
}


}
