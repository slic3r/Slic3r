#ifndef slic3r_FillGyroid_hpp_
#define slic3r_FillGyroid_hpp_

#include "../libslic3r.h"

#include "Fill.hpp"

namespace Slic3r {

class FillGyroid : public Fill
{
public:

    FillGyroid(){}
    virtual Fill* clone() const { return new FillGyroid(*this); };
    virtual ~FillGyroid() {}

    /// require bridge flow since most of this pattern hangs in air
	// but it's not useful as most of it is on the previous layer.
	// it's just slowing it down => set it to false!
    virtual bool use_bridge_flow() const { return false; }

protected:

    virtual void _fill_surface_single(
        unsigned int                     thickness_layers,
        const std::pair<float, Point>   &direction, 
        ExPolygon                       &expolygon, 
        Polylines                       *polylines_out);
};

} // namespace Slic3r

#endif // slic3r_FillGyroid_hpp_
