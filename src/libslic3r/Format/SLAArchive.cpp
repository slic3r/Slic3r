#include "Format/SLAArchive.hpp"
namespace Slic3r {

using ConfMap = std::map<std::string, std::string>;

namespace {

std::string get_cfg_value(const DynamicPrintConfig &cfg, const std::string &key)
{
    std::string ret;
    
    if (cfg.has(key)) {
        auto opt = cfg.option(key);
        if (opt) ret = opt->serialize();
    }
    
    return ret;    
}

} // namespace
uqptr<sla::RasterBase> SLAArchive::create_raster() const
{
    sla::RasterBase::Resolution res;
    sla::RasterBase::PixelDim   pxdim;
    std::array<bool, 2>         mirror;

    double w  = this->config().display_width.getFloat();
    double h  = this->config().display_height.getFloat();
    auto   pw = size_t(this->config().display_pixels_x.getInt());
    auto   ph = size_t(this->config().display_pixels_y.getInt());

    mirror[X] = this->config().display_mirror_x.getBool();
    mirror[Y] = this->config().display_mirror_y.getBool();
    
    auto ro = this->config().display_orientation.getInt();
    sla::RasterBase::Orientation orientation =
        ro == sla::RasterBase::roPortrait ? sla::RasterBase::roPortrait :
                                            sla::RasterBase::roLandscape;
    
    if (orientation == sla::RasterBase::roPortrait) {
        std::swap(w, h);
        std::swap(pw, ph);
    }

    res   = sla::RasterBase::Resolution{pw, ph};
    pxdim = sla::RasterBase::PixelDim{w / pw, h / ph};
    sla::RasterBase::Trafo tr{orientation, mirror};

    double gamma = this->config().gamma_correction.getFloat();

    return sla::create_raster_grayscale_aa(res, pxdim, gamma, tr);
}

sla::RasterEncoder SLAArchive::get_encoder() const
{
    return sla::PNGRasterEncoder{};
}

} // namespace Slic3r
