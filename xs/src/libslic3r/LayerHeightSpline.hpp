#ifndef slic3r_LayerHeightSpline_hpp_
#define slic3r_LayerHeightSpline_hpp_

#include "libslic3r.h"
#include "BSpline/BSpline.h" // Warning: original BSplineBase.h/cpp merged into BSpline.h/cpp to avoid dependency issues caused by Build::WithXSpp which tries to compile all .cpp files in /src

namespace Slic3r {


class LayerHeightSpline
{
    public:
    LayerHeightSpline();
    LayerHeightSpline(const LayerHeightSpline &other);
    LayerHeightSpline& operator=(const LayerHeightSpline &other);
    void setObjectHeight(coordf_t object_height) { this->_object_height = object_height; };
    bool hasData(); // indicate that we have valid data
    bool updateRequired(); // indicate whether we want to generate a new spline from the layers
    void suppressUpdate();
    bool setLayers(std::vector<coordf_t> layers);
    bool updateLayerHeights(std::vector<coordf_t> heights);
    bool layersUpdated() const { return this->_layers_updated; }; // true if the basis set of layers was updated (by the slicing algorithm)
    bool layerHeightsUpdated() const { return this->_layer_heights_updated; }; // true if the heights where updated (by the spline control user interface)
    void clear();
    std::vector<coordf_t> getOriginalLayers() const { return this->_original_layers; };
    std::vector<coordf_t> getInterpolatedLayers() const;
    const coordf_t getLayerHeightAt(coordf_t height);

    private:
    bool _updateBSpline();

    coordf_t _object_height;
    bool _is_valid;
    bool _update_required; // this should be always true except if we want to generate new layers from this spline
    bool _layers_updated;
    bool _layer_heights_updated;
    std::vector<coordf_t> _original_layers;
    std::vector<coordf_t> _internal_layers;
    std::vector<coordf_t> _internal_layer_heights;
    std::unique_ptr<BSpline<double>> _layer_height_spline;
};

}

#endif
