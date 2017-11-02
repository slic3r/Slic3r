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
        // merge top and bottom in a single collection
        SurfaceCollection tb = top;
        tb.append(bottom);
        
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
            if (s.surface_type != stTop && !s.is_bottom())
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

}
