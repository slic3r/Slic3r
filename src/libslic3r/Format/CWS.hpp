#ifndef slic3r_format_CWS_HPP
#define slic3r_format_CWS_HPP

#include "libslic3r/Zipper.hpp"
#include "libslic3r/Format/SL1.hpp"

namespace Slic3r {
// "Masked" CWS as used by Malyan S100
class MaskedCWSArchive : public SL1Archive {
    SLAPrinterConfig m_cfg;
public:
    MaskedCWSArchive() = default;
    explicit MaskedCWSArchive(const SLAPrinterConfig &cfg): m_cfg(cfg) {}
    explicit MaskedCWSArchive(SLAPrinterConfig &&cfg): m_cfg(std::move(cfg)) {}
    void export_print(Zipper &zipper, const SLAPrint &print, const std::string &projectname = "") override;
};
} // namespace Slic3r

#endif // slic3r_format_CWS_HPP
