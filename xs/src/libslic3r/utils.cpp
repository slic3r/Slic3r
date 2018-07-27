#include "utils.hpp"
#include <regex>
#ifndef NO_PERL
    #include <xsinit.h>
#else
    #include "Log.hpp"
#endif

#include <cstdarg>

void
confess_at(const char *file, int line, const char *func,
            const char *pat, ...)
{
    #ifdef SLIC3RXS
     va_list args;
     SV *error_sv = newSVpvf("Error in function %s at %s:%d: ", func,
         file, line);

     va_start(args, pat);
     sv_vcatpvf(error_sv, pat, &args);
     va_end(args);

     sv_catpvn(error_sv, "\n\t", 2);

     dSP;
     ENTER;
     SAVETMPS;
     PUSHMARK(SP);
     XPUSHs( sv_2mortal(error_sv) );
     PUTBACK;
     call_pv("Carp::confess", G_DISCARD);
     FREETMPS;
     LEAVE;
    #else
    std::stringstream ss;
    ss << "Error in function " << func << " at " << file << ":" << line << ": ";
    ss << pat << "\n";

    Slic3r::Log::error(std::string("Libslic3r"), ss.str() );
    #endif
}

std::vector<std::string> 
split_at_regex(const std::string& input, const std::string& regex) {
    // passing -1 as the submatch index parameter performs splitting
    std::regex re(regex);
    std::sregex_token_iterator
        first{input.begin(), input.end(), re, -1},
        last;
    return {first, last};
}

std::string _trim_zeroes(std::string in) { return trim_zeroes(in); }
/// Remove extra zeroes generated from std::to_string on doubles
std::string trim_zeroes(std::string in) {
    std::string result {""};
    std::regex strip_zeroes("(0*)$");
    std::regex_replace (std::back_inserter(result), in.begin(), in.end(), strip_zeroes, "");
    if (result.back() == '.') result.append("0");
    return result;
}
