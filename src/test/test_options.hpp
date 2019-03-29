#ifndef TEST_OPTIONS_HPP
#include <string>

/// Directory path, passed in from the outside, for the path to the test inputs dir.
constexpr auto* testfile_dir {"C:/local/Slic3rcpp/src/test/inputs/"};

inline std::string testfile(std::string filename) {
    std::string result;
    result.append(testfile_dir);
    result.append(filename);
    return result;
}

#endif // TEST_OPTIONS_HPP
