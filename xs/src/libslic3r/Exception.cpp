#include "ConfigBase.hpp"

namespace Slic3r {
BadOptionTypeException::BadOptionTypeException(const char* _func_name, t_config_option_key _opt_key)
    : func_name(_func_name), opt_key(_opt_key) {
    std::ostringstream s_msg;
    s_msg << "Invalid accessor used: "
          << this->func_name
          << " on "
          << this->opt_key << ".";
    this->msg = s_msg.str();
}
const char* BadOptionTypeException::what() const noexcept {
    return this->msg.c_str();
}
}
