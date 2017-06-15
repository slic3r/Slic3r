#ifndef slic3r_WireframePrint_hpp_
#define slic3r_WireframePrint_hpp_

#include "Print.hpp"
#include "libslic3r.h"

namespace Slic3r {

class WireframePrintRegion
{
public:
	WireframePrintRegion();
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