#ifndef slic3r_WireframePrint_hpp_
#define slic3r_WireframePrint_hpp_

#include "Print.hpp"
#include "libslic3r.h"
#include "PrintConfig.hpp"

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
public:
	WireframePrintObject();
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