/** 
 * https://github.com/alexrj/Slic3r/wiki/Conditional-Gcode-Syntax-Spec
 * 
 */

#include <iostream>
#include <string>
#include <sstream>


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

/// Recursive expression parser. Offloads mathematics to exprtk. 
/// Precondition: All strings inside {} are able to be understood by exprtk (and thus parsed to a number).
/// Starts from the end of the string and works from the inside out.
/// Any statements that resolve to {if0} will remove everything on the same line.
std::string expression(const std::string& input, const int depth = 0);

/// External access function to begin replac
std::string apply_math(const std::string& input);

}
