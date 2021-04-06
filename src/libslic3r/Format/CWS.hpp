#ifndef slic3r_format_CWS_HPP
#define slic3r_format_CWS_HPP

#include "libslic3r/Zipper.hpp"
#include "libslic3r/Format/SL1.hpp"

namespace Slic3r {
// "Masked" CWS as used by Malyan S100
class MaskedCWSArchive : public SLAArchive {
    SLAPrinterConfig m_cfg;
public:
    SLAPrinterConfig& config() override { return m_cfg; }
    const SLAPrinterConfig& config() const override { return m_cfg; }
    MaskedCWSArchive() = default;
    explicit MaskedCWSArchive(const SLAPrinterConfig &cfg): m_cfg(cfg) {}
    explicit MaskedCWSArchive(SLAPrinterConfig &&cfg): m_cfg(std::move(cfg)) {}
    void export_print(Zipper &zipper, const SLAPrint &print, const std::string &projectname = "") override;
};
} // namespace Slic3r

#endif // slic3r_format_CWS_HPP
