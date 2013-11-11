#include "Point.hpp"
#include "Line.hpp"
#include <math.h>

namespace Slic3r {

void
Point::scale(double factor)
{
    this->x *= factor;
    this->y *= factor;
}

void
Point::translate(double x, double y)
{
    this->x += x;
    this->y += y;
}

void
Point::rotate(double angle, Point* center)
{
    double cur_x = this->x;
    double cur_y = this->y;
    this->x = center->x + cos(angle) * (cur_x - center->x) - sin(angle) * (cur_y - center->y);
    this->y = center->y + cos(angle) * (cur_y - center->y) + sin(angle) * (cur_x - center->x);
}

bool
Point::coincides_with(const Point* point) const
{
    return this->x == point->x && this->y == point->y;
}

int
Point::nearest_point_index(const Points points) const
{
    int idx = -1;
    double distance = -1;  // double because long is limited to 2147483647 on some platforms and it's not enough
    
    for (Points::const_iterator it = points.begin(); it != points.end(); ++it) {
        /* If the X distance of the candidate is > than the total distance of the
           best previous candidate, we know we don't want it */
        double d = pow(this->x - (*it).x, 2);
        if (distance != -1 && d > distance) continue;
        
        /* If the Y distance of the candidate is > than the total distance of the
           best previous candidate, we know we don't want it */
        d += pow(this->y - (*it).y, 2);
        if (distance != -1 && d > distance) continue;
        
        idx = it - points.begin();
        distance = d;
        
        if (distance < EPSILON) break;
    }
    
    return idx;
}

Point*
Point::nearest_point(Points points) const
{
    return &(points.at(this->nearest_point_index(points)));
}

double
Point::distance_to(const Point* point) const
{
    double dx = point->x - this->x;
    double dy = point->y - this->y;
    return sqrt(dx*dx + dy*dy);
}

double
Point::distance_to(const Line* line) const
{
    if (line->a.coincides_with(&line->b)) return this->distance_to(&line->a);
    
    double n = (line->b.x - line->a.x) * (line->a.y - this->y)
        - (line->a.x - this->x) * (line->b.y - line->a.y);
    
    return abs(n) / line->length();
}

#ifdef SLIC3RXS
SV*
Point::to_SV_ref() {
    SV* sv = newSV(0);
    sv_setref_pv( sv, "Slic3r::Point::Ref", (void*)this );
    return sv;
}

SV*
Point::to_SV_clone_ref() const {
    SV* sv = newSV(0);
    sv_setref_pv( sv, "Slic3r::Point", new Point(*this) );
    return sv;
}

SV*
Point::to_SV_pureperl() const {
    AV* av = newAV();
    av_fill(av, 1);
    av_store(av, 0, newSVnv(this->x));
    av_store(av, 1, newSVnv(this->y));
    return newRV_noinc((SV*)av);
}

void
Point::from_SV(SV* point_sv)
{
    AV* point_av = (AV*)SvRV(point_sv);
    this->x = SvNV(*av_fetch(point_av, 0, 0));
    this->y = SvNV(*av_fetch(point_av, 1, 0));
}

void
Point::from_SV_check(SV* point_sv)
{
    if (sv_isobject(point_sv) && (SvTYPE(SvRV(point_sv)) == SVt_PVMG)) {
        *this = *(Point*)SvIV((SV*)SvRV( point_sv ));
    } else {
        this->from_SV(point_sv);
    }
}
#endif

}
