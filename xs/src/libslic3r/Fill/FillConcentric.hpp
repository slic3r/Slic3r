#ifndef slic3r_FillConcentric_hpp_
#define slic3r_FillConcentric_hpp_

#include "Fill.hpp"

namespace Slic3r {

class FillConcentric : public Fill
{
public:
    virtual ~FillConcentric() {}

protected:
    virtual Fill* clone() const { return new FillConcentric(*this); };
	virtual void _fill_surface_single(
	    unsigned int                     thickness_layers,
	    const direction_t               &direction, 
	    ExPolygon                       &expolygon, 
	    Polylines*                      polylines_out);

	virtual bool no_sort() const { return true; }
    virtual bool can_solid() const { return true; };
};

} // namespace Slic3r

#endif // slic3r_FillConcentric_hpp_
