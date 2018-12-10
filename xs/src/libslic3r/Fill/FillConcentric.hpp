#ifndef slic3r_FillConcentric_hpp_
#define slic3r_FillConcentric_hpp_

#include "FillBase.hpp"

namespace Slic3r {

class FillConcentric : public Fill
{
public:
    virtual ~FillConcentric() {}

protected:
    virtual Fill* clone() const { return new FillConcentric(*this); };
    virtual void fill_surface_extrusion(const Surface *surface, const FillParams &params,
        const Flow &flow, const ExtrusionRole &role, ExtrusionEntitiesPtr &out);

	virtual bool no_sort() const { return true; }
};

} // namespace Slic3r

#endif // slic3r_FillConcentric_hpp_
