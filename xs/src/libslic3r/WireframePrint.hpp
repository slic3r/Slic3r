#ifndef slic3r_WireframePrint_hpp_
#define slic3r_WireframePrint_hpp_

#include "Print.hpp"
#include "libslic3r.h"

namespace Slic3r {

class WireframePrint : public Print
{
	WireframePrint();
	~WireframePrint();
    
    // methods for handling state
    bool invalidate_state_by_config(const PrintConfigBase &config);
    bool invalidate_step(PrintStep step);
    bool invalidate_all_steps();
    bool step_done(PrintObjectStep step) const;
};

}

#endif