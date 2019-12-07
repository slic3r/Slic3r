#include "ExtrusionEntityCollection.hpp"
#include "ShortestPath.hpp"
#include <algorithm>
#include <cmath>
#include <map>

namespace Slic3r {

ExtrusionEntityCollection::ExtrusionEntityCollection(const ExtrusionPaths &paths)
    : no_sort(false)
{
    this->append(paths);
}

ExtrusionEntityCollection& ExtrusionEntityCollection::operator= (const ExtrusionEntityCollection &other)
{
    this->entities      = other.entities;
    for (size_t i = 0; i < this->entities.size(); ++i)
        this->entities[i] = this->entities[i]->clone();
    this->no_sort       = other.no_sort;
    return *this;
}

void ExtrusionEntityCollection::swap(ExtrusionEntityCollection &c)
{
    std::swap(this->entities, c.entities);
    std::swap(this->no_sort, c.no_sort);
}

void ExtrusionEntityCollection::clear()
{
	for (size_t i = 0; i < this->entities.size(); ++i)
		delete this->entities[i];
    this->entities.clear();
}

ExtrusionEntityCollection::operator ExtrusionPaths() const
{
    ExtrusionPaths paths;
    for (const ExtrusionEntity *ptr : this->entities) {
        if (const ExtrusionPath *path = dynamic_cast<const ExtrusionPath*>(ptr))
            paths.push_back(*path);
    }
    return paths;
}

ExtrusionEntityCollection* ExtrusionEntityCollection::clone() const
{
    ExtrusionEntityCollection* coll = new ExtrusionEntityCollection(*this);
    for (size_t i = 0; i < coll->entities.size(); ++i)
        coll->entities[i] = this->entities[i]->clone();
    return coll;
}

void ExtrusionEntityCollection::reverse()
{
    for (ExtrusionEntity *ptr : this->entities)
    {
        // Don't reverse it if it's a loop, as it doesn't change anything in terms of elements ordering
        // and caller might rely on winding order
        if (!ptr->can_reverse())
            ptr->reverse();
    }
    std::reverse(this->entities.begin(), this->entities.end());
}

void ExtrusionEntityCollection::replace(size_t i, const ExtrusionEntity &entity)
{
    delete this->entities[i];
    this->entities[i] = entity.clone();
}

void ExtrusionEntityCollection::remove(size_t i)
{
    delete this->entities[i];
    this->entities.erase(this->entities.begin() + i);
}

ExtrusionEntityCollection ExtrusionEntityCollection::chained_path_from(const Point &start_near, ExtrusionRole role) const
{
    ExtrusionEntityCollection out;
    if (this->no_sort) {
        out = *this;
    } else {
        if (role == erMixed)
            out = *this;
        else {
            for (const ExtrusionEntity *ee : this->entities) {
                if (role != erMixed) {
                    // The caller wants only paths with a specific extrusion role.
                    auto role2 = ee->role();
                    if (role != role2) {
                        // This extrusion entity does not match the role asked.
                        assert(role2 != erMixed);
                        continue;
                    }
                }
                out.entities.emplace_back(ee->clone());
            }
        }
        chain_and_reorder_extrusion_entities(out.entities, &start_near);
    }
    return out;
}

void ExtrusionEntityCollection::polygons_covered_by_width(Polygons &out, const float scaled_epsilon) const
{
    for (const ExtrusionEntity *entity : this->entities)
        entity->polygons_covered_by_width(out, scaled_epsilon);
}

void ExtrusionEntityCollection::polygons_covered_by_spacing(Polygons &out, const float scaled_epsilon) const
{
    for (const ExtrusionEntity *entity : this->entities)
        entity->polygons_covered_by_spacing(out, scaled_epsilon);
}

// Recursively count paths and loops contained in this collection.
size_t ExtrusionEntityCollection::items_count() const
{
    return CountEntities().count(*this);
}

void
CountEntities::use(const ExtrusionEntityCollection &coll) {
    for (const ExtrusionEntity* entity : coll.entities) {
        entity->visit(*this);
    }
}

// Returns a single vector of pointers to all non-collection items contained in this one.
ExtrusionEntityCollection ExtrusionEntityCollection::flatten(bool preserve_ordering) const
{
    //ExtrusionEntityCollection coll;
    //this->flatten(&coll, preserve_ordering);
    //return coll;
    return FlatenEntities(preserve_ordering).flatten(*this);

}
void
FlatenEntities::use(const ExtrusionEntityCollection &coll) {
    if (coll.no_sort && preserve_ordering) {
        FlatenEntities unsortable(coll, preserve_ordering);
        for (const ExtrusionEntity* entity : coll.entities) {
            entity->visit(unsortable);
        }
        to_fill.append(std::move(unsortable.to_fill));
    }
    else {
        for (const ExtrusionEntity* entity : coll.entities) {
            entity->visit(*this);
        }
    }
}

ExtrusionEntityCollection&&
FlatenEntities::flatten(const ExtrusionEntityCollection &to_flatten) && {
    use(to_flatten);
    return std::move(to_fill);
}


double
ExtrusionEntityCollection::min_mm3_per_mm() const
{
    double min_mm3_per_mm = std::numeric_limits<double>::max();
    for (const ExtrusionEntity *entity : this->entities)
        min_mm3_per_mm = std::min(min_mm3_per_mm, entity->min_mm3_per_mm());
    return min_mm3_per_mm;
}

}
