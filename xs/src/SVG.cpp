#include "SVG.hpp"

#include "Line.hpp"
#include "TriangleMesh.hpp"
#include "Polygon.hpp"

namespace Slic3r {

SVG::SVG(const char* filename)
{
    this->f = fopen(filename, "w");
    fprintf(this->f,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.0//EN\" \"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">\n"
        "<svg height=\"2000\" width=\"2000\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:svg=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">\n"
	    "   <marker id=\"endArrow\" markerHeight=\"8\" markerUnits=\"strokeWidth\" markerWidth=\"10\" orient=\"auto\" refX=\"11\" refY=\"5\" viewBox=\"0 0 10 10\">\n"
		"      <polyline fill=\"darkblue\" points=\"0,0 10,5 0,10 1,5\" />\n"
	    "   </marker>\n"
	    );
	this->arrows = true;
}

float
SVG::coordinate(long c)
{
    return (float)unscale(c)*10;
}

void
SVG::AddLine(const Line &line, std::string color, float width)
{
    fprintf(this->f,
        "   <line x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\" style=\"stroke:%s; stroke-width:%.2f\"",
            this->coordinate(line.a.x), this->coordinate(line.a.y), this->coordinate(line.b.x), this->coordinate(line.b.y), color.empty()?"black":color.c_str(), (width<0)?0.5:width
        );
    if (this->arrows)
        fprintf(this->f, " marker-end=\"url(#endArrow)\"");
    fprintf(this->f, "/>\n");
}

void
SVG::AddPolygon(const Polygon& poly, std::string color, std::string fill, std::string desc)
{
    fprintf(this->f,
            "   <path style=\"stroke-width:.2;stroke:%s;fill:%s;fill-opacity:0.3\"\n"
            "      d=\"M ", 
            color.empty()?"black":color.c_str(), fill.empty()?"none":fill.c_str());
    for(Points::const_iterator it=poly.points.begin();it!=poly.points.end();++it)
        fprintf(this->f, "%f %f%s", coordinate(it->x), coordinate(it->y), (it==poly.points.end()-1)?"":",");

    fprintf(this->f, " z\">\n");
    if(!desc.empty()) 
        fprintf(this->f, "      <desc>%s></desc>\n", desc.c_str());
    fprintf(this->f, "   </path>\n");
}

void
SVG::AddPolyline(const Polyline& poly, std::string color, float width)
{
    fprintf(this->f,
            "   <path style=\"stroke-width:%f;stroke:%s;fill:none\"\n"
            "      d=\"M ", 
            (width<0)?.2:width, color.empty()?"black":color.c_str());
    for(Points::const_iterator it=poly.points.begin();it!=poly.points.end();++it)
        fprintf(this->f, "%f %f%s", coordinate(it->x), coordinate(it->y), (it==poly.points.end()-1)?"":",");

    fprintf(this->f, "\">\n");
    fprintf(this->f, "   </path>\n");
}

void
SVG::AddPoint(const Point& p, std::string color, float size, std::string desc)
{
    fprintf(this->f, "   <circle cx=\"%f\" cy=\"%f\" r=\"%f\" style=\"stroke:black;stroke-width:%f;fill:%s\"", coordinate(p.x), coordinate(p.y), size, size/5, color.empty()?"black":color.c_str());
    if(!desc.empty()) 
        fprintf(this->f, ">\n"
                "      <desc>%s></desc>\n"
                "   </circle>", desc.c_str());
    else
        fprintf(this->f, "/>\n");
}
void
SVG::AddLine(const IntersectionLine &line)
{
    this->AddLine(Line(line.a, line.b));
}

void
SVG::Close()
{
    fprintf(this->f, "</svg>\n");
    fclose(this->f);
    printf("SVG file written.\n");
}

}
