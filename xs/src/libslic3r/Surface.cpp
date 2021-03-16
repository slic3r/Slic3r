#include "Surface.hpp"

namespace Slic3r {

Surface::operator Polygons() const
{
    return this->expolygon;
}

double
Surface::area() const
{
    return this->expolygon.area();
}

bool
Surface::is_solid() const
{
    return (this->surface_type & (stTop | stBottom | stSolid | stBridge)) != 0;
}

bool
Surface::is_external() const
{
    return is_top() || is_bottom();
}

bool
Surface::is_internal() const
{
    return (this->surface_type & stInternal) != 0;
}

bool
Surface::is_bottom() const
{
    return (this->surface_type & stBottom) != 0;
}

bool
Surface::is_top() const
{
    return (this->surface_type & stTop) != 0;
}

bool
Surface::is_bridge() const
{
    return (this->surface_type & stBridge) != 0;
}

}
