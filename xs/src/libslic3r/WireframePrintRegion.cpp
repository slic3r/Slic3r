#include "WireframePrint.hpp"

namespace Slic3r
{
	
WireframePrintRegion::WireframePrintRegion(WireframePrint* print)
    : _print(print)
{
}

WireframePrintRegion::~WireframePrintRegion()
{
}

WireframePrint*
WireframePrintRegion::print()
{
    return this->_print;
}

}

bool
WireframePrintRegion::invalidate_state_by_config(const PrintConfigBase &config)
{

	return false;

}