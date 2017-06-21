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
    bool hasData(); // indicate that we have valid data;
    bool setLayers(std::vector<coordf_t> layers);
    bool updateLayerHeights(std::vector<coordf_t> heights);
    bool layersUpdated() const { return this->_layers_updated; }; // true if the basis set of layers was updated (by the slicing algorithm)
    bool layerHeightsUpdated() const { return this->_layer_heights_updated; }; // true if the heights where updated (by the spline control user interface)
    void clear();
    std::vector<coordf_t> getOriginalLayers() const { return this->_layers; };
    std::vector<coordf_t> getInterpolatedLayers() const;
    const coordf_t getLayerHeightAt(coordf_t height);

    private:
    bool _updateBSpline();

    coordf_t _object_height;
    bool _is_valid;
    bool _layers_updated;
    bool _layer_heights_updated;
    std::vector<coordf_t> _layers;
    std::vector<coordf_t> _layer_heights;
    std::vector<coordf_t> _spline_layers;
    std::vector<coordf_t> _spline_layer_heights;
    std::unique_ptr<BSpline<double>> _layer_height_spline;
};

}

#endif
