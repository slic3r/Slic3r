#ifndef slic3r_ExtrusionEntityCollection_hpp_
#define slic3r_ExtrusionEntityCollection_hpp_

#include "libslic3r.h"
#include "ExtrusionEntity.hpp"
#include "Polyline.hpp"

namespace Slic3r {

class ExtrusionEntityCollection : public ExtrusionEntity
{
    public:
    ExtrusionEntityCollection* clone() const;

    /// Owned ExtrusionEntities and descendent ExtrusionEntityCollections.
    /// Iterating over this needs to check each child to see if it, too is a collection.
    ExtrusionEntitiesPtr entities;     // we own these entities

    std::vector<size_t> orig_indices;  // handy for XS
    bool no_sort;
    ExtrusionEntityCollection(): no_sort(false) {};
    ExtrusionEntityCollection(const ExtrusionEntityCollection &collection);
    ExtrusionEntityCollection(const ExtrusionPaths &paths);
    ExtrusionEntityCollection& operator= (const ExtrusionEntityCollection &other);
    ~ExtrusionEntityCollection();

    /// Operator to convert and flatten this collection to a single vector of ExtrusionPaths.
    operator ExtrusionPaths() const;
   
    /// This particular ExtrusionEntity is a collection.
    bool is_collection() const {
        return true;
    };

    bool can_reverse() const {
        return !this->no_sort;
    };
    bool empty() const {
        return this->entities.empty();
    };
    void clear();
    size_t size() const { return this->entities.size(); }
    void swap (ExtrusionEntityCollection &c);
    void append(const ExtrusionEntity &entity);
    void append(const ExtrusionEntitiesPtr &entities);
    void append(const ExtrusionPaths &paths);
    void append(const Polylines &polylines, const ExtrusionPath &templ);
    void replace(size_t i, const ExtrusionEntity &entity);
    void remove(size_t i);
    ExtrusionEntityCollection chained_path(bool no_reverse = false, std::vector<size_t>* orig_indices = NULL) const;
    void chained_path(ExtrusionEntityCollection* retval, bool no_reverse = false, std::vector<size_t>* orig_indices = NULL) const;
    void chained_path_from(Point start_near, ExtrusionEntityCollection* retval, bool no_reverse = false, std::vector<size_t>* orig_indices = NULL) const;
    void reverse();
    Point first_point() const;
    Point last_point() const;
    Polygons grow() const;

    /// Recursively count paths and loops contained in this collection 
    size_t items_count() const;

    /// Returns a single vector of pointers to all non-collection items contained in this one
    /// \param retval a pointer to the output memory space.
    /// \param preserve_ordering Flag to method that will flatten if and only if the underlying collection is sortable when True (default: False).
    void flatten(ExtrusionEntityCollection* retval, bool preserve_ordering = false) const;

    /// Returns a flattened copy of this ExtrusionEntityCollection. That is, all of the items in its entities vector are not collections.
    /// You should be iterating over flatten().entities if you are interested in the underlying ExtrusionEntities (and don't care about hierarchy).
    /// \param preserve_ordering Flag to method that will flatten if and only if the underlying collection is sortable when True (default: False).
    ExtrusionEntityCollection flatten(bool preserve_ordering = false) const;


    double min_mm3_per_mm() const;
    Polyline as_polyline() const {
        CONFESS("Calling as_polyline() on a ExtrusionEntityCollection");
        return Polyline();
    };

    ExtrusionEntitiesPtr::iterator begin() { return entities.begin(); }
    ExtrusionEntitiesPtr::iterator end() { return entities.end(); }
    ExtrusionEntitiesPtr::const_iterator begin() const { return entities.cbegin(); }
    ExtrusionEntitiesPtr::const_iterator end() const { return entities.cend(); }
    ExtrusionEntitiesPtr::const_iterator cbegin() const { return entities.cbegin(); }
    ExtrusionEntitiesPtr::const_iterator cend() const { return entities.cend(); }

    
};

}

#endif
