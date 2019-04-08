#ifndef slic3r_SurfaceCollection_hpp_
#define slic3r_SurfaceCollection_hpp_

#include "libslic3r.h"
#include "Surface.hpp"
#include <vector>

namespace Slic3r {

class SurfaceCollection
{
    public:
    Surfaces surfaces;
    
    SurfaceCollection() {};
    SurfaceCollection(const Surfaces &_surfaces)
        : surfaces(_surfaces) {};
    operator Polygons() const;
    operator ExPolygons() const;
    void simplify(double tolerance);
    /// Unifies straight multi-segment lines to a single line (artifacts from stl-triangulation)
    void remove_collinear_points();
    void group(std::vector<SurfacesConstPtr> *retval) const;
    template <class T> bool any_internal_contains(const T &item) const;
    template <class T> bool any_bottom_contains(const T &item) const;
    SurfacesPtr filter_by_type(SurfaceType type) {
        return this->filter_by_type({ type });
    };
    SurfacesPtr filter_by_type(std::initializer_list<SurfaceType> types);
    void filter_by_type(SurfaceType type, Polygons* polygons);
    SurfacesConstPtr filter_by_type(SurfaceType type) const {
        return this->filter_by_type({ type });
    };
    SurfacesConstPtr filter_by_type(std::initializer_list<SurfaceType> types) const;

    /// deletes all surfaces that match the supplied type.
    void remove_type(const SurfaceType type);

    void remove_types(std::initializer_list<SurfaceType> types);

    template<int N>
    void remove_types(std::array<SurfaceType, N> types) {
        remove_types(types.data(), types.size());
    }
    /// group surfaces by common properties
    std::vector<SurfacesPtr> group();
    void group(std::vector<SurfacesPtr> *retval);

    /// Deletes every surface other than the ones that match the provided type.
    void keep_type(const SurfaceType type);
    /// Deletes every surface other than the ones that match the provided types.
    void keep_types(std::initializer_list<SurfaceType> types);

    /// Deletes every surface other than the ones that match the provided types.
    void keep_types(const SurfaceType *types, size_t ntypes);

    /// deletes all surfaces that match the supplied aggregate of types.
    void remove_types(const SurfaceType *types, size_t ntypes);

    void set(const SurfaceCollection &coll) { surfaces = coll.surfaces; }
    void set(SurfaceCollection &&coll) { surfaces = std::move(coll.surfaces); }
    void set(const ExPolygons &src, SurfaceType surfaceType) { clear(); this->append(src, surfaceType); }
    void set(const ExPolygons &src, const Surface &surfaceTempl) { clear(); this->append(src, surfaceTempl); }
    void set(const Surfaces &src) { clear(); this->append(src); }
    void set(ExPolygons &&src, SurfaceType surfaceType) { clear(); this->append(std::move(src), surfaceType); }
    void set(ExPolygons &&src, const Surface &surfaceTempl) { clear(); this->append(std::move(src), surfaceTempl); }
    void set(Surfaces &&src) { clear(); this->append(std::move(src)); }

    void append(const SurfaceCollection &coll);
    void append(const Surface &surface);
    void append(const Surfaces &surfaces);
    void append(const ExPolygons &src, const Surface &templ);
    void append(const ExPolygons &src, SurfaceType surfaceType);
    size_t polygons_count() const;
    bool empty() const { return this->surfaces.empty(); };
    size_t size() const { return this->surfaces.size(); };
    void clear() { this->surfaces.clear(); };
    void erase(size_t i) { this->surfaces.erase(this->surfaces.begin() + i); };
    Surfaces::iterator begin() { return this->surfaces.begin();}
    Surfaces::iterator end() { return this->surfaces.end();}
    Surfaces::const_iterator cbegin() const { return this->surfaces.cbegin();}
    Surfaces::const_iterator cend() const { return this->surfaces.cend();}
    Surfaces::const_iterator begin() const { return this->surfaces.cbegin();}
    Surfaces::const_iterator end() const { return this->surfaces.cend();}
};

}

#endif
