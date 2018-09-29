#ifndef slic3r_FillSmooth_hpp_
#define slic3r_FillSmooth_hpp_

#include "../libslic3r.h"

#include "Fill.hpp"

namespace Slic3r {

class FillSmooth : public Fill
{
public:
    FillSmooth() {
		nbPass = 2;
		anglePass[0] = float(M_PI/4);
		anglePass[1] = -float(M_PI/4);
		anglePass[2] = 0;
		fillPattern[0] = InfillPattern::ipRectilinear;
		fillPattern[1] = InfillPattern::ipRectilinear;
		fillPattern[2] = InfillPattern::ipRectilinear;
		rolePass[0] = erSolidInfill;
		rolePass[1] = erTopSolidInfill;
		rolePass[2] = erSolidInfill;
		percentWidth[0] = 0.9;
		percentWidth[1] = 2;
		percentWidth[2] = 1.0;
		percentFlow[0] = 0.7;
		percentFlow[1] = 0.3;
		percentFlow[2] = 0.0;
		double extrusionMult = 1.0;
		percentFlow[0] *= extrusionMult;
		percentFlow[1] *= extrusionMult;
		percentFlow[2] *= extrusionMult;
	}
    virtual bool can_solid() const { return true; };
    virtual Fill* clone() const { return new FillSmooth(*this); }

	
    // Perform the fill (call fill_surface)
    virtual void fill_surface_extrusion(const Surface &surface, const Flow &flow, 
        ExtrusionEntitiesPtr &out, const ExtrusionRole &role);
	
protected:
	int nbPass=2;
	double percentWidth[3];
	double percentFlow[3];
	float anglePass[3];
	ExtrusionRole rolePass[3];
	InfillPattern fillPattern[3];
};

} // namespace Slic3r

#endif // slic3r_FillSmooth_hpp_
