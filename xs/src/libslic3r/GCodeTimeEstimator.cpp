#include "GCodeTimeEstimator.hpp"
#include <cmath>
#include <boost/version.hpp>
#if BOOST_VERSION >= 107300
#include <boost/bind/bind.hpp>
#endif

namespace Slic3r {

#if BOOST_VERSION >= 107300
using boost::placeholders::_1;
using boost::placeholders::_2;
#endif

void
GCodeTimeEstimator::parse(const std::string &gcode)
{
    GCodeReader::parse(gcode, boost::bind(&GCodeTimeEstimator::_parser, this, _1, _2));
}

void
GCodeTimeEstimator::parse_file(const std::string &file)
{
    GCodeReader::parse_file(file, boost::bind(&GCodeTimeEstimator::_parser, this, _1, _2));
}

void
GCodeTimeEstimator::_parser(GCodeReader&, const GCodeReader::GCodeLine &line)
{
    // std::cout << "[" << this->time << "] " << line.raw << std::endl;
    if (line.cmd == "G1") {
        const float dist_XY = line.dist_XY();
        const float new_F = line.new_F();
        
        if (dist_XY > 0) {
            //this->time += dist_XY / new_F * 60;
            this->time += _accelerated_move(dist_XY, new_F/60, this->acceleration);
        } else {
            //this->time += std::abs(line.dist_E()) / new_F * 60;
            this->time += _accelerated_move(std::abs(line.dist_E()), new_F/60, this->acceleration);
        }
        //this->time += std::abs(line.dist_Z()) / new_F * 60;
        this->time += _accelerated_move(std::abs(line.dist_Z()), new_F/60, this->acceleration);
    } else if (line.cmd == "M204" && line.has('S')) {
        this->acceleration = line.get_float('S');
    } else if (line.cmd == "G4") { // swell
        if (line.has('S')) {
            this->time += line.get_float('S');
        } else if (line.has('P')) {
            this->time += line.get_float('P')/1000;
        }
    }
}

// Wildly optimistic acceleration "bell" curve modeling.
// Returns an estimate of how long the move with a given accel
// takes in seconds.
// It is assumed that the movement is smooth and uniform.
float
GCodeTimeEstimator::_accelerated_move(double length, double v, double acceleration) 
{
    // for half of the move, there are 2 zones, where the speed is increasing/decreasing and 
    // where the speed is constant.
    // Since the slowdown is assumed to be uniform, calculate the average velocity for half of the 
    // expected displacement.
    // final velocity v = a*t => a * (dx / 0.5v) => v^2 = 2*a*dx
    // v_avg = 0.5v => 2*v_avg = v
    // d_x = v_avg*t => t = d_x / v_avg
    acceleration = (acceleration == 0.0 ? 4000.0 : acceleration); // Set a default accel to use for print time in case it's 0 somehow.
    auto half_length = length / 2.0;
    auto t_init = v / acceleration; // time to final velocity
    auto dx_init = (0.5*v*t_init); // Initial displacement for the time to get to final velocity
    auto t = 0.0;
    if (half_length >= dx_init) {
        half_length -= (0.5*v*t_init);
        t += t_init;
    }
    t += (half_length / v); // constant speed for rest of the time and too short displacements
    return 2.0*t; // cut in half before, so double to get full time spent.
}

}
