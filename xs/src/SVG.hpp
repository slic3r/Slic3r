#ifndef slic3r_SVG_hpp_
#define slic3r_SVG_hpp_

#include <myinit.h>
#include "TriangleMesh.hpp"

namespace Slic3r {

class Line;
class Polygon;

class SVG
{
    private:
    FILE* f;
    float coordinate(long c);
    public:
    bool arrows;
    SVG(const char* filename);
    void AddLine(const Line &line, std::string color="", float width=-1);
    void AddLine(const IntersectionLine &line);
    void AddPolygon(const Polygon& poly, std::string color, std::string fill, std::string desc);
    void AddPolyline(const Polyline& poly, std::string color="", float width=-1);
    void AddPoint(const Point& p, std::string color, float size, std::string desc="");
    void Close();
};

}

#endif
