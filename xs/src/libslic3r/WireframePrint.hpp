#ifndef slic3r_WireframePrint_hpp_
#define slic3r_WireframePrint_hpp_

#include "Print.hpp"
#include "libslic3r.h"
#include "PrintConfig.hpp"
#include "Model.hpp"
#include "BoundingBox.hpp"

namespace Slic3r {

class WireframePrint;
class WireframePrintObject;

class WireframePrintRegion
{
	friend class WireframePrint;

public:
	WireframePrintRegionConfig config;

    WireframePrint* print();
    //Flow flow(FlowRole role, double layer_height, bool bridge, bool first_layer, double width, const WireframePrintObject &object) const;
    bool invalidate_state_by_config(const PrintConfigBase &config);

    private:
    WireframePrint* _print;
    
    WireframePrintRegion(WireframePrint* print);
	~WireframePrintRegion();
	
};

class WireframePrintObject
{
	friend class WireframePrint;

public:
	WireframePrintObjectConfig config;

	Point3 size;           // XYZ in scaled coordinates

	LayerPtrs layers;
	PrintState<PrintObjectStep> state;

	WireframePrint* print();
    ModelObject* model_object() { return this->_model_object; };

    BoundingBox bounding_box() const;
    std::set<size_t> extruders() const;

    size_t total_layer_count() const;
    size_t layer_count() const;
    void clear_layers();
    Layer* get_layer(int idx) { return this->layers.at(idx); };
    const Layer* get_layer(int idx) const { return this->layers.at(idx); };
    // print_z: top of the layer; slice_z: center of the layer.
    Layer* add_layer(int id, coordf_t height, coordf_t print_z, coordf_t slice_z);
    void delete_layer(int idx);

    // methods for handling state
    bool invalidate_state_by_config(const PrintConfigBase &config);
    bool invalidate_step(PrintObjectStep step);
    bool invalidate_all_steps();

    void _slice();

    private:
    Print* _print;
    ModelObject* _model_object;

	WireframePrintObject(WireframePrint* print, ModelObject* model_object, const BoundingBoxf3 &modobj_bbox);
	~WireframePrintObject();
	
};

class WireframePrint
{
public:
	WireframePrint();
	~WireframePrint();
};

}

#endif