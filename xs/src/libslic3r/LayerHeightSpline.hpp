#ifndef slic3r_LayerHeightSpline_hpp_
#define slic3r_LayerHeightSpline_hpp_

#include "libslic3r.h"
#include "BSpline/BSpline.h" // Warning: original BSplineBase.h/cpp merged into BSpline.h/cpp to avoid dependency issues caused by Build::WithXSpp which tries to compile all .cpp files in /src


namespace Slic3r {


class LayerHeightSpline
{
    public:
    LayerHeightSpline(coordf_t object_height);
    ~LayerHeightSpline();
    bool hasData(); // indicate that we have valid data
    bool updateRequired(); // indicate whether we want to generate a new spline from the layers
    void suppressUpdate();
    bool setLayers(std::vector<coordf_t> layers);
    bool updateLayerHeights(std::vector<coordf_t> heights);
    void clear();
    std::vector<coordf_t> getOriginalLayers() const { return this->_original_layers; };
    std::vector<coordf_t> getInterpolatedLayers() const;
    const coordf_t getLayerHeightAt(coordf_t height);

    private:
    bool _updateBSpline();

    coordf_t _object_height;
    bool _is_valid;
    bool _update_required; // this should be always true except if we want to generate new layers from this spline
    std::vector<coordf_t> _original_layers;
    std::vector<coordf_t> _internal_layers;
    std::vector<coordf_t> _internal_layer_heights;
    BSpline<double> *_layer_height_spline;
};

}

#endif
