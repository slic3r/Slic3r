#include "Layer.hpp"
#include "BridgeDetector.hpp"
#include "ClipperUtils.hpp"
#include "Geometry.hpp"
#include "PerimeterGenerator.hpp"
#include "Print.hpp"
#include "Surface.hpp"

namespace Slic3r {

/// Creates a new Flow object with the arguments and the variables of this LayerRegion
Flow
LayerRegion::flow(FlowRole role, bool bridge, double width) const
{
    return this->_region->flow(
        role,
        this->_layer->height,
        bridge,
        this->_layer->id() == 0,
        width,
        *this->_layer->object()
    );
}

/// Merges this->slices with union_ex, and then repopulates this->slices.surfaces
void
LayerRegion::merge_slices()
{
    // without safety offset, artifacts are generated (GH #2494)
    ExPolygons expp = union_ex((Polygons)this->slices, true);
    this->slices.surfaces.clear();
    this->slices.surfaces.reserve(expp.size());
    
    for (ExPolygons::const_iterator expoly = expp.begin(); expoly != expp.end(); ++expoly)
        this->slices.surfaces.push_back(Surface(stInternal, *expoly));
}

/// Creates a new PerimeterGenerator object
/// Which will return the perimeters by its construction
void
LayerRegion::make_perimeters(const SurfaceCollection &slices, SurfaceCollection* fill_surfaces)
{
    this->perimeters.clear();
    this->thin_fills.clear();
    
    PerimeterGenerator g(
        // input:
        &slices,
        this->layer()->height,
        this->flow(frPerimeter),
        &this->region()->config,
        &this->layer()->object()->config,
        &this->layer()->object()->print()->config,
        
        // output:
        &this->perimeters,
        &this->thin_fills,
        fill_surfaces
    );
    
    if (this->layer()->lower_layer != NULL)
        // Cummulative sum of polygons over all the regions.
        g.lower_slices = &this->layer()->lower_layer->slices;
    
    g.layer_id              = this->layer()->id();
    g.ext_perimeter_flow    = this->flow(frExternalPerimeter);
    g.overhang_flow         = this->region()->flow(frPerimeter, -1, true, false, -1, *this->layer()->object());
    g.solid_infill_flow     = this->flow(frSolidInfill);
    
    g.process();
}

/// Processes bridges with holes which are internal features.
/// Detects same-orientation bridges and merges them.
/// Processes and groups top and bottom surfaces
/// This function reads layer->slices and lower_layer->slices
/// and writes this->bridged and this->fill_surfaces, so it's thread-safe.
void
LayerRegion::process_external_surfaces()
{
    Surfaces &surfaces = this->fill_surfaces.surfaces;
    
    for (size_t j = 0; j < surfaces.size(); ++j) {
        // we don't get any reference to surface because it would be invalidated
        // by the erase() call below
        
        if (this->layer()->lower_layer != NULL && surfaces[j].is_bridge()) {
            // If this bridge has one or more holes that are internal surfaces
            // (thus not visible from the outside), like a slab sustained by
            // pillars, include them in the bridge in order to have better and
            // more continuous bridging.
            for (size_t i = 0; i < surfaces[j].expolygon.holes.size(); ++i) {
                // reverse the hole and consider it a polygon
                Polygon h = surfaces[j].expolygon.holes[i];
                h.reverse();
            
                // Is this hole fully contained in the layer slices?
                if (diff(h, this->layer()->slices).empty()) {
                    // remove any other surface contained in this hole
                    for (size_t k = 0; k < surfaces.size(); ++k) {
                        if (k == j) continue;
                        if (h.contains(surfaces[k].expolygon.contour.first_point())) {
                            surfaces.erase(surfaces.begin() + k);
                            if (j > k) --j;
                            --k;
                        }
                    }
                    
                    Polygons &holes = surfaces[j].expolygon.holes;
                    holes.erase(holes.begin() + i);
                    --i;
                }
            }
        }
    }
    
    SurfaceCollection bottom;
    for (const Surface &surface : surfaces) {
        if (!surface.is_bottom()) continue;
        
        /*  detect bridge direction before merging grown surfaces otherwise adjacent bridges
            would get merged into a single one while they need different directions
            also, supply the original expolygon instead of the grown one, because in case
            of very thin (but still working) anchors, the grown expolygon would go beyond them */
        double angle = -1;
        if (this->layer()->lower_layer != NULL && surface.is_bridge()) {
            BridgeDetector bd(
                surface.expolygon,
                this->layer()->lower_layer->slices,
                this->flow(frInfill, true).scaled_width()
            );
            
            #ifdef SLIC3R_DEBUG
            printf("Processing bridge at layer %zu (z = %f):\n", this->layer()->id(), this->layer()->print_z);
            #endif
            
            if (bd.detect_angle()) {
                angle = bd.angle;
            
                if (this->layer()->object()->config.support_material) {
                    append_to(this->bridged, bd.coverage());
                    this->unsupported_bridge_edges.append(bd.unsupported_edges());
                }
            }
        }
        
        const ExPolygons grown = offset_ex(surface.expolygon, +SCALED_EXTERNAL_INFILL_MARGIN);
        Surface templ = surface;
        templ.bridge_angle = angle;
        bottom.append(grown, templ);
    }
    
    SurfaceCollection top;
    for (const Surface &surface : surfaces) {
        if (surface.surface_type != stTop) continue;
        
        // give priority to bottom surfaces
        ExPolygons grown = diff_ex(
            offset(surface.expolygon, +SCALED_EXTERNAL_INFILL_MARGIN),
            (Polygons)bottom
        );
        top.append(grown, surface);
    }
    
    SurfaceCollection nonplanar;
    for (const Surface &surface : surfaces) {
        if (!surface.is_nonplanar()) continue;

        ExPolygons grown = offset_ex(surface.expolygon, +SCALED_EXTERNAL_INFILL_MARGIN);
        nonplanar.append(grown, surface);
    }
    
    /*  if we're slicing with no infill, we can't extend external surfaces
        over non-existent infill */
    SurfaceCollection fill_boundaries;
    if (this->region()->config.fill_density.value > 0) {
        fill_boundaries = SurfaceCollection(surfaces);
    } else {
        for (const Surface &s : surfaces)
            if (s.surface_type != stInternal)
                fill_boundaries.surfaces.push_back(s);
    }
    
    // intersect the grown surfaces with the actual fill boundaries
    SurfaceCollection new_surfaces;
    {
        // merge top, bottom and nonplanar in a single collection
        SurfaceCollection tb = top;
        tb.append(bottom);
        tb.append(nonplanar);
        
        // group surfaces
        std::vector<SurfacesConstPtr> groups;
        tb.group(&groups);
        
        for (const SurfacesConstPtr &g : groups) {
            Polygons subject;
            for (const Surface* s : g)
                append_to(subject, (Polygons)*s);
            
            ExPolygons expp = intersection_ex(
                subject,
                (Polygons)fill_boundaries,
                true // to ensure adjacent expolygons are unified
            );
            
            new_surfaces.append(expp, *g.front());
        }
    }
    
    /* subtract the new top surfaces from the other non-top surfaces and re-add them */
    {
        SurfaceCollection other;
        for (const Surface &s : surfaces)
            if (s.surface_type != stTop && !s.is_bottom() && !s.is_nonplanar())
                other.surfaces.push_back(s);
        
        // group surfaces
        std::vector<SurfacesConstPtr> groups;
        other.group(&groups);
        
        for (const SurfacesConstPtr &g : groups) {
            Polygons subject;
            for (const Surface* s : g)
                append_to(subject, (Polygons)*s);
            
            ExPolygons expp = diff_ex(
                subject,
                (Polygons)new_surfaces
            );
            
            new_surfaces.append(expp, *g.front());
        }
    }
    
    this->fill_surfaces = std::move(new_surfaces);
}

/// If no solid layers are requested, turns top/bottom surfaces to internal
/// Turns too small internal regions into solid regions according to the user setting
void
LayerRegion::prepare_fill_surfaces()
{
    /*  Note: in order to make the psPrepareInfill step idempotent, we should never
        alter fill_surfaces boundaries on which our idempotency relies since that's
        the only meaningful information returned by psPerimeters. */
    
    // if no solid layers are requested, turn top/bottom surfaces to internal
    if (this->region()->config.top_solid_layers == 0) {
        for (Surfaces::iterator surface = this->fill_surfaces.surfaces.begin(); surface != this->fill_surfaces.surfaces.end(); ++surface) {
            if (surface->surface_type == stTop) {
                if (this->layer()->object()->config.infill_only_where_needed) {
                    surface->surface_type = stInternalVoid;
                } else {
                    surface->surface_type = stInternal;
                }
            }
        }
    }
    if (this->region()->config.bottom_solid_layers == 0) {
        for (Surfaces::iterator surface = this->fill_surfaces.surfaces.begin(); surface != this->fill_surfaces.surfaces.end(); ++surface) {
            if (surface->surface_type == stBottom || surface->surface_type == stBottomBridge)
                surface->surface_type = stInternal;
        }
    }
        
    // turn too small internal regions into solid regions according to the user setting
    const float &fill_density = this->region()->config.fill_density;
    if (fill_density > 0 && fill_density < 100) {
        // scaling an area requires two calls!
        // (we don't use scale_() because it would overflow the coord_t range
        const double min_area = this->region()->config.solid_infill_below_area.value / SCALING_FACTOR / SCALING_FACTOR;
        for (Surface &surface : this->fill_surfaces.surfaces) {
            if (surface.surface_type == stInternal && surface.area() <= min_area)
                surface.surface_type = stInternalSolid;
        }
    }
}

///  Gets smallest area by squaring the Flow's scaled spacing
double
LayerRegion::infill_area_threshold() const
{
    double ss = this->flow(frSolidInfill).scaled_spacing();
    return ss*ss;
}

void
LayerRegion::append_nonplanar_surface(NonplanarSurface& surface, float distance_to_top)
{
    for(auto & s : this->nonplanar_surfaces){
        if (s == surface){
            return;
        }    
    }
    nonplanar_surfaces.push_back(surface);
    distances_to_top.push_back(distance_to_top);
}

void
LayerRegion::project_nonplanar_extrusion(ExtrusionEntityCollection* collection)
{
    for (auto& entity : collection->entities) {
        if (ExtrusionLoop* loop = dynamic_cast<ExtrusionLoop*>(entity)) {
            for(auto& path : loop->paths) {
                project_nonplanar_path(&path);
                correct_z_on_path(&path);
            }
        } else if (ExtrusionPath* path = dynamic_cast<ExtrusionPath*>(entity)) {
            project_nonplanar_path(path);
            correct_z_on_path(path);
        } 
        
    }
}

void
LayerRegion::project_nonplanar_surfaces()
{
    //skip if there are no nonplanar_surfaces on this LayerRegion
    if (this->nonplanar_surfaces.size() == 0){
        return;
    }
    
    //for all perimeters do path projection
    for (auto& col : this->perimeters.entities) {
        ExtrusionEntityCollection* collection = dynamic_cast<ExtrusionEntityCollection*>(col);
        this->project_nonplanar_extrusion(collection);
    }

    //and all fill paths do path projection
    for (auto& col : this->fills.entities) {
        ExtrusionEntityCollection* collection = dynamic_cast<ExtrusionEntityCollection*>(col);
        this->project_nonplanar_extrusion(collection);
    }
}

//Sorting functions for soring of paths
bool
greaterX(const Point &a, const Point &b)
{
   return (a.x < b.x);
}

bool
smallerX(const Point &a, const Point &b)
{
   return (a.x > b.x);
}

bool
greaterY(const Point &a, const Point &b)
{
   return (a.y < b.y);
}

bool
smallerY(const Point &a, const Point &b)
{
   return (a.y > b.y);
}

void
LayerRegion::project_nonplanar_path(ExtrusionPath *path)
{
    //First check all points and project them regarding the triangle mesh
    for (Point& point : path->polyline.points) {
        for (auto& surface : this->nonplanar_surfaces) {
            float distance_to_top = surface.stats.max.z - this->layer()->print_z;
            for(auto& facet : surface.mesh) {
                //skip if point is outside of the bounding box of the triangle
                if (unscale(point.x) < std::min({facet.second.vertex[0].x, facet.second.vertex[1].x, facet.second.vertex[2].x}) ||
                    unscale(point.x) > std::max({facet.second.vertex[0].x, facet.second.vertex[1].x, facet.second.vertex[2].x}) ||
                    unscale(point.y) < std::min({facet.second.vertex[0].y, facet.second.vertex[1].y, facet.second.vertex[2].y}) ||
                    unscale(point.y) > std::max({facet.second.vertex[0].y, facet.second.vertex[1].y, facet.second.vertex[2].y}))
                {
                    continue;
                }
                //check if point is inside of Triangle
                if (Slic3r::Geometry::Point_in_triangle(
                    Pointf(unscale(point.x),unscale(point.y)),
                    Pointf(facet.second.vertex[0].x, facet.second.vertex[0].y),
                    Pointf(facet.second.vertex[1].x, facet.second.vertex[1].y),
                    Pointf(facet.second.vertex[2].x, facet.second.vertex[2].y)))
                {
                    Slic3r::Geometry::Project_point_on_plane(Pointf3(facet.second.vertex[0].x,facet.second.vertex[0].y,facet.second.vertex[0].z),
                                                             Pointf3(facet.second.normal.x,facet.second.normal.y,facet.second.normal.z),
                                                             point);
                    //Shift down when on lower layer
                    point.z = point.z - scale_(distance_to_top);
                    //break;
                }
            }
        }
    }

    //Then check all line intersections, cut line on intersection and project new point
    std::vector<Point>::size_type size = path->polyline.points.size();
    for (std::vector<Point>::size_type i = 0; i < size-1; ++i)
    {
        Points intersections;
        //check against every facet if lines intersect
        for (auto& surface : this->nonplanar_surfaces) {
            float distance_to_top = surface.stats.max.z - this->layer()->print_z;
            for(auto& facet : surface.mesh) {
                for(int j= 0; j < 3; j++){
                    //TODO precheck for faster computation
                    Point p1 = Point(scale_(facet.second.vertex[j].x), scale_(facet.second.vertex[j].y), scale_(facet.second.vertex[j].z));
                    Point p2 = Point(scale_(facet.second.vertex[(j+1) % 3].x), scale_(facet.second.vertex[(j+1) % 3].y), scale_(facet.second.vertex[(j+1) % 3].z));
                    Point* p = Slic3r::Geometry::Line_intersection(p1, p2, path->polyline.points[i], path->polyline.points[i+1]);
                    
                    if (p) {
                        //add distance to top for every added point
                        p->z = p->z - scale_(distance_to_top);
                        intersections.push_back(*p);
                    }
                }
            }

        }
        //Stop if no intersections are found
        if (intersections.size() == 0) continue;

        //sort found intersectons if there are more than 1
        if ( intersections.size() > 1 ){
            //sort by X
            if (abs(path->polyline.points[i+1].x - path->polyline.points[i].x) >= abs(path->polyline.points[i+1].y - path->polyline.points[i].y) ){
                if(path->polyline.points[i].x < path->polyline.points[i+1].x) {
                    std::sort(intersections.begin(), intersections.end(),smallerX);
                }else {
                    std::sort(intersections.begin(), intersections.end(),greaterX);
                }
            } else {//sort by Y
                if(path->polyline.points[i].y < path->polyline.points[i+1].y) {
                    std::sort(intersections.begin(), intersections.end(),smallerY);
                }else {
                    std::sort(intersections.begin(), intersections.end(),greaterY);
                }
            }
        }

        //remove duplicates
        Points::iterator point = intersections.begin();
        while (point != intersections.end()-1) {
            bool delete_point = false;
            Points::iterator point2 = point;
            ++point2;
            //compare with next point if they are the same, delete current point
            if ((*point).x == (*point2).x && (*point).y == (*point2).y) {
                    //remove duplicate point
                    delete_point = true;
                    point = intersections.erase(point);
            }
            //continue loop when no point is removed. Otherwise the new point is set while deleting the old one.
            if (!delete_point){
                ++point;
            }
        }

        //insert new points into array
        for (Point p : intersections)
        {
            path->polyline.points.insert(path->polyline.points.begin()+i+1, p);
        }

        //modifiy array boundary
        i = i + intersections.size();
        size = size + intersections.size();
    }
}

void
LayerRegion::correct_z_on_path(ExtrusionPath *path)
{
    for (Point& point : path->polyline.points) {
        if(point.z == -1.0){
            point.z = scale_(this->layer()->print_z);
        }
    }
}

}
