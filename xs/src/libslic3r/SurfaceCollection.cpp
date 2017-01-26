#include "SurfaceCollection.hpp"
#include <map>

namespace Slic3r {

SurfaceCollection::operator Polygons() const
{
    Polygons polygons;
    for (Surfaces::const_iterator surface = this->surfaces.begin(); surface != this->surfaces.end(); ++surface) {
        Polygons surface_p = surface->expolygon;
        polygons.insert(polygons.end(), surface_p.begin(), surface_p.end());
    }
    return polygons;
}

SurfaceCollection::operator ExPolygons() const
{
    ExPolygons expp;
    expp.reserve(this->surfaces.size());
    for (Surfaces::const_iterator surface = this->surfaces.begin(); surface != this->surfaces.end(); ++surface) {
        expp.push_back(surface->expolygon);
    }
    return expp;
}

void
SurfaceCollection::simplify(double tolerance)
{
    Surfaces ss;
    for (Surfaces::const_iterator it_s = this->surfaces.begin(); it_s != this->surfaces.end(); ++it_s) {
        ExPolygons expp;
        it_s->expolygon.simplify(tolerance, &expp);
        for (ExPolygons::const_iterator it_e = expp.begin(); it_e != expp.end(); ++it_e) {
            Surface s = *it_s;
            s.expolygon = *it_e;
            ss.push_back(s);
        }
    }
    this->surfaces = ss;
}

/* group surfaces by common properties */
void
SurfaceCollection::group(std::vector<SurfacesConstPtr> *retval) const
{
    for (Surfaces::const_iterator it = this->surfaces.begin(); it != this->surfaces.end(); ++it) {
        // find a group with the same properties
        SurfacesConstPtr* group = NULL;
        for (std::vector<SurfacesConstPtr>::iterator git = retval->begin(); git != retval->end(); ++git) {
            const Surface* gkey = git->front();
            if (   gkey->surface_type      == it->surface_type
                && gkey->thickness         == it->thickness
                && gkey->thickness_layers  == it->thickness_layers
                && gkey->bridge_angle      == it->bridge_angle) {
                group = &*git;
                break;
            }
        }
        
        // if no group with these properties exists, add one
        if (group == NULL) {
            retval->resize(retval->size() + 1);
            group = &retval->back();
        }
        
        // append surface to group
        group->push_back(&*it);
    }
}

template <class T>
bool
SurfaceCollection::any_internal_contains(const T &item) const
{
    for (Surfaces::const_iterator surface = this->surfaces.begin(); surface != this->surfaces.end(); ++surface) {
        if (surface->is_internal() && surface->expolygon.contains(item)) return true;
    }
    return false;
}
template bool SurfaceCollection::any_internal_contains<Polyline>(const Polyline &item) const;

template <class T>
bool
SurfaceCollection::any_bottom_contains(const T &item) const
{
    for (Surfaces::const_iterator surface = this->surfaces.begin(); surface != this->surfaces.end(); ++surface) {
        if (surface->is_bottom() && surface->expolygon.contains(item)) return true;
    }
    return false;
}
template bool SurfaceCollection::any_bottom_contains<Polyline>(const Polyline &item) const;

SurfacesPtr
SurfaceCollection::filter_by_type(SurfaceType type)
{
    SurfacesPtr ss;
    for (Surfaces::iterator surface = this->surfaces.begin(); surface != this->surfaces.end(); ++surface) {
        if (surface->surface_type == type) ss.push_back(&*surface);
    }
    return ss;
}

void
SurfaceCollection::filter_by_type(SurfaceType type, Polygons* polygons)
{
    for (Surfaces::iterator surface = this->surfaces.begin(); surface != this->surfaces.end(); ++surface) {
        if (surface->surface_type == type)
            append_to(*polygons, (Polygons)surface->expolygon);
    }
}

void
SurfaceCollection::append(const SurfaceCollection &coll)
{
    this->append(coll.surfaces);
}

void
SurfaceCollection::append(const Surfaces &surfaces)
{
    append_to(this->surfaces, surfaces);
}

void
SurfaceCollection::append(const ExPolygons &src, const Surface &templ)
{
    this->surfaces.reserve(this->surfaces.size() + src.size());
    for (ExPolygons::const_iterator it = src.begin(); it != src.end(); ++ it) {
        this->surfaces.push_back(templ);
        this->surfaces.back().expolygon = *it;
    }
}

void
SurfaceCollection::append(const ExPolygons &src, SurfaceType surfaceType)
{
    this->surfaces.reserve(this->surfaces.size() + src.size());
    for (ExPolygons::const_iterator it = src.begin(); it != src.end(); ++ it)
        this->surfaces.push_back(Surface(surfaceType, *it));
}

size_t
SurfaceCollection::polygons_count() const
{
    size_t count = 0;
    for (Surfaces::const_iterator it = this->surfaces.begin(); it != this->surfaces.end(); ++ it)
        count += 1 + it->expolygon.holes.size();
    return count;
}

}
