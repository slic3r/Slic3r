/** 
 * https://github.com/alexrj/Slic3r/wiki/Conditional-Gcode-Syntax-Spec
 * 
 */

#include <iostream>
#include <sstream>
#include <string>
#include <regex>

// Valid start tokens
// {, {if
// 
// Valid end tokens
// }
//
// Valid operator tokens
//
// &&, ||, and, or, not, xor, ==, equals, !=, <, <=, >,>=
//
// Everything else
// Anything that is not recognized as a token
//
// Special case:
//
// {if is special, it indicates that the rest of the line is dropped (ignored) if 
// it evaluates to False/0.


namespace Slic3r {
using gcode_recurse_t = std::pair < std::string, bool >;

const std::regex binary_op("[ ]*([-.0-9]+|true|false)[ ]*([-+*/^<>]|and|or|xor|equals|==|!=|<=|>=)[ ]*([-.0-9]+|true|false)[ ]*");
const std::regex unary_op("[ ]*(!|not)[ ]*([01]|true|false)");


/// Evaluate operators
std::string evaluate(std::stringstream& val1, std::stringstream& val2, const std::string op) {
    std::stringstream result;
    if (op.compare("+") == 0) {
        double v1, v2;
        val1 >> v1;
        val2 >> v2;
        result << (v1 + v2);
    } else if (op.compare("-") == 0) {
        double v1, v2;
        val1 >> v1;
        val2 >> v2;
        result << (v1 - v2);
    } else if (op.compare("*") == 0) {
        double v1, v2;
        val1 >> v1;
        val2 >> v2;
        result << (v1 * v2);
    } else if (op.compare("/") == 0) {
        double v1, v2;
        val1 >> v1;
        val2 >> v2;
        result << (v1 / v2);
    } else if (op.compare("not") == 0 || op.compare("!") == 0) {
        unsigned v1;
        if (val1.str().compare("true") == 0) {
            v1 = 1;
        } else  
        if (val1.str().compare("false") == 0) {
            v1 = 0;
        } else {
            val1 >> v1;
        }
        result << (!v1);
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
    if (open_bracket == 0) { // no subexpressions, resolve operators.
    // regexes to check:
    // VAL1 OP VAL2
    // OP VAL1
        std::smatch m;
        if (std::regex_match(buffer, m, binary_op)) {
            #ifdef SLIC3R_DEBUG
            std::cerr << "Matched binary op with " << input << " at depth " << depth << std::endl;
            #endif
            std::stringstream val1_s, val2_s, op;
            val1_s << m[1]; val2_s << m[3]; op << m[2];
            std::string result = evaluate(val1_s, val2_s, op.str());
            #ifdef SLIC3R_DEBUG
            std::cerr << "Op Result: " << result << std::endl;
            #endif
            return gcode_recurse_t(result, cond);
        ;
        } else if (std::regex_match(buffer, m, unary_op)) {
            #ifdef SLIC3R_DEBUG
            std::cerr << "Matched unary op with " << input << " at depth " << depth << std::endl;
            #endif
            std::stringstream val1_s, op;
            val1_s << m[1]; op << m[2];
            std::string result = evaluate(val1_s, val1_s, op.str());
            return gcode_recurse_t(result, cond);
        ;
        } else { // no op, do nothing
            #ifdef SLIC3R_DEBUG
            std::cerr << "Matched no op with " << input << " at depth " << depth << std::endl;
            #endif
            return gcode_recurse_t(buffer, cond);
        }
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
