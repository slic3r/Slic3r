#ifndef slic3r_Surface_hpp_
#define slic3r_Surface_hpp_

#include "libslic3r.h"
#include "ExPolygon.hpp"

namespace Slic3r {

/// Surface type enumerations.
/// As it is very unlikely that there will be more than 32 or 64 of these surface types, pack into a flag 
enum SurfaceType : uint16_t { 
    stTop            = 0b1,       /// stTop: it has nothing just on top of it
    stBottom         = 0b10,      /// stBottom: it's a surface with nothing just under it (or the base plate, or a support)
    stInternal       = 0b100,     /// stInternal: not top nor bottom
    stSolid          = 0b1000,    /// stSolid: modify the stInternal to say it should be at 100% infill
    stBridge         = 0b10000,   /// stBridge: modify stBottom or stInternal to say it should be extruded as a bridge
    stVoid           = 0b100000   /// stVoid: modify stInternal to say it should be at 0% infill
};
inline SurfaceType operator|(SurfaceType a, SurfaceType b)
{return static_cast<SurfaceType>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));}
inline SurfaceType operator&(SurfaceType a, SurfaceType b)
{return static_cast<SurfaceType>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));}
inline SurfaceType operator|=(SurfaceType& a, SurfaceType b)
{ a = a | b; return a;}
inline SurfaceType operator&=(SurfaceType& a, SurfaceType b)
{ a = a & b; return a;}

class Surface
{
    public:
    SurfaceType     surface_type;
    ExPolygon       expolygon;
    double          thickness;          // in mm
    unsigned short  thickness_layers;   // in layers
    double          bridge_angle;       // in radians, ccw, 0 = East, only 0+ (negative means undefined)
    unsigned short  extra_perimeters;
    
    Surface(SurfaceType _surface_type, const ExPolygon &_expolygon)
        : surface_type(_surface_type), expolygon(_expolygon),
            thickness(-1), thickness_layers(1), bridge_angle(-1), extra_perimeters(0)
        {};
    operator Polygons() const;
    double area() const;
    bool is_solid() const;
    bool is_external() const;
    bool is_internal() const;
    bool is_top() const;
    bool is_bottom() const;
    bool is_bridge() const;
};

typedef std::vector<Surface> Surfaces;
typedef std::vector<Surface*> SurfacesPtr;
typedef std::vector<const Surface*> SurfacesConstPtr;

inline Polygons
to_polygons(const Surfaces &surfaces)
{
    Slic3r::Polygons pp;
    for (Surfaces::const_iterator s = surfaces.begin(); s != surfaces.end(); ++s)
        append_to(pp, (Polygons)*s);
    return pp;
}

inline Polygons
to_polygons(const SurfacesPtr &surfaces)
{
    Slic3r::Polygons pp;
    for (SurfacesPtr::const_iterator s = surfaces.begin(); s != surfaces.end(); ++s)
        append_to(pp, (Polygons)**s);
    return pp;
}

inline Polygons
to_polygons(const SurfacesConstPtr &surfaces)
{
    Slic3r::Polygons pp;
    for (SurfacesConstPtr::const_iterator s = surfaces.begin(); s != surfaces.end(); ++s)
        append_to(pp, (Polygons)**s);
    return pp;
}

inline ExPolygons to_expolygons(const Surfaces &src)
{
    ExPolygons expolygons;
    expolygons.reserve(src.size());
    for (Surfaces::const_iterator it = src.begin(); it != src.end(); ++it)
        expolygons.push_back(it->expolygon);
    return expolygons;
}

inline ExPolygons to_expolygons(Surfaces &&src)
{
	ExPolygons expolygons;
	expolygons.reserve(src.size());
	for (Surfaces::const_iterator it = src.begin(); it != src.end(); ++it)
		expolygons.emplace_back(ExPolygon(std::move(it->expolygon)));
	src.clear();
	return expolygons;
}

inline ExPolygons to_expolygons(const SurfacesPtr &src)
{
    ExPolygons expolygons;
    expolygons.reserve(src.size());
    for (SurfacesPtr::const_iterator it = src.begin(); it != src.end(); ++it)
        expolygons.push_back((*it)->expolygon);
    return expolygons;
}


// Count a number of polygons stored inside the vector of expolygons.
// Useful for allocating space for polygons when converting expolygons to polygons.
inline size_t number_polygons(const Surfaces &surfaces)
{
    size_t n_polygons = 0;
    for (Surfaces::const_iterator it = surfaces.begin(); it != surfaces.end(); ++ it)
        n_polygons += it->expolygon.holes.size() + 1;
    return n_polygons;
}

inline size_t number_polygons(const SurfacesPtr &surfaces)
{
    size_t n_polygons = 0;
    for (SurfacesPtr::const_iterator it = surfaces.begin(); it != surfaces.end(); ++ it)
        n_polygons += (*it)->expolygon.holes.size() + 1;
    return n_polygons;
}

// Append a vector of Surfaces at the end of another vector of polygons.
inline void polygons_append(Polygons &dst, const Surfaces &src)
{
    dst.reserve(dst.size() + number_polygons(src));
    for (Surfaces::const_iterator it = src.begin(); it != src.end(); ++ it) {
        dst.push_back(it->expolygon.contour);
        dst.insert(dst.end(), it->expolygon.holes.begin(), it->expolygon.holes.end());
    }
}

inline void polygons_append(Polygons &dst, Surfaces &&src)
{
    dst.reserve(dst.size() + number_polygons(src));
    for (Surfaces::iterator it = src.begin(); it != src.end(); ++ it) {
        dst.push_back(std::move(it->expolygon.contour));
        std::move(std::begin(it->expolygon.holes), std::end(it->expolygon.holes), std::back_inserter(dst));
        it->expolygon.holes.clear();
    }
}

// Append a vector of Surfaces at the end of another vector of polygons.
inline void polygons_append(Polygons &dst, const SurfacesPtr &src)
{
    dst.reserve(dst.size() + number_polygons(src));
    for (SurfacesPtr::const_iterator it = src.begin(); it != src.end(); ++ it) {
        dst.push_back((*it)->expolygon.contour);
        dst.insert(dst.end(), (*it)->expolygon.holes.begin(), (*it)->expolygon.holes.end());
    }
}

inline void polygons_append(Polygons &dst, SurfacesPtr &&src)
{
    dst.reserve(dst.size() + number_polygons(src));
    for (SurfacesPtr::const_iterator it = src.begin(); it != src.end(); ++ it) {
        dst.push_back(std::move((*it)->expolygon.contour));
        std::move(std::begin((*it)->expolygon.holes), std::end((*it)->expolygon.holes), std::back_inserter(dst));
        (*it)->expolygon.holes.clear();
    }
}

inline bool surfaces_could_merge(const Surface &s1, const Surface &s2)
{
    return 
        s1.surface_type      == s2.surface_type     &&
        s1.thickness         == s2.thickness        &&
        s1.thickness_layers  == s2.thickness_layers &&
        s1.bridge_angle      == s2.bridge_angle;
}

} // namespace Slic3r

#endif
