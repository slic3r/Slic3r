#include "Layer.hpp"
#include "ClipperUtils.hpp"
#include "Geometry.hpp"
#include "Print.hpp"

namespace Slic3r {

/// Initialises upper_layer, lower_layer to NULL
/// Initialises slicing_errors to false
Layer::Layer(size_t id, PrintObject *object, coordf_t height, coordf_t print_z,
        coordf_t slice_z)
:   upper_layer(NULL),
    lower_layer(NULL),
    regions(),
    slicing_errors(false),
    slice_z(slice_z),
    print_z(print_z),
    height(height),
    slices(),
    _id(id),
    _object(object)
{
}

/// Removes references to self and clears regions
Layer::~Layer()
{
    // remove references to self
    if (NULL != this->upper_layer) {
        this->upper_layer->lower_layer = NULL;
    }

    if (NULL != this->lower_layer) {
        this->lower_layer->upper_layer = NULL;
    }

    this->clear_regions();
}

/// Getter for this->_id
size_t
Layer::id() const
{
    return this->_id;
}

/// Setter for this->_id
void
Layer::set_id(size_t id)
{
    this->_id = id;
}

/// Getter for this->regions.size()
size_t
Layer::region_count() const
{
    return this->regions.size();
}

/// Deletes all regions using this->delete_region()
void
Layer::clear_regions()
{
    for (int i = this->regions.size()-1; i >= 0; --i)
        this->delete_region(i);
}

/// Creates a LayerRegion from a PrintRegion and adds it to this->regions
LayerRegion*
Layer::add_region(PrintRegion* print_region)
{
    LayerRegion* region = new LayerRegion(this, print_region);
    this->regions.push_back(region);
    return region;
}

/// Deletes an individual region
void
Layer::delete_region(int idx)
{
    LayerRegionPtrs::iterator i = this->regions.begin() + idx;
    LayerRegion* item = *i;
    this->regions.erase(i);
    delete item;
}

/// Merge all regions' slices to get islands
//TODO: is this right?
void
Layer::make_slices()
{
    ExPolygons slices;
    if (this->regions.size() == 1) {
        // optimization: if we only have one region, take its slices
        slices = this->regions.front()->slices;
    } else {
        Polygons slices_p;
        FOREACH_LAYERREGION(this, layerm) {
            Polygons region_slices_p = (*layerm)->slices;
            slices_p.insert(slices_p.end(), region_slices_p.begin(), region_slices_p.end());
        }
        slices = union_ex(slices_p);
    }

    this->slices.expolygons.clear();
    this->slices.expolygons.reserve(slices.size());

    // prepare ordering points
    // While it's more computationally expensive, we use centroid()
    // instead of first_point() because it's [much more] deterministic
    // and preserves ordering across similar layers.
    Points ordering_points;
    ordering_points.reserve(slices.size());
    for (const ExPolygon &ex : slices)
        ordering_points.push_back(ex.contour.centroid());

    // sort slices
    std::vector<Points::size_type> order;
    Slic3r::Geometry::chained_path(ordering_points, order);

    // populate slices vector
    for (const Points::size_type &o : order)
        this->slices.expolygons.push_back(slices[o]);
}

/// Iterates over all of the LayerRegion and invokes LayerRegion->merge_slices()
void
Layer::merge_slices()
{
    FOREACH_LAYERREGION(this, layerm) {
        (*layerm)->merge_slices();
    }
}

/// Uses LayerRegion->slices.any_internal_contains(item)
template <class T>
bool
Layer::any_internal_region_slice_contains(const T &item) const
{
    FOREACH_LAYERREGION(this, layerm) {
        if ((*layerm)->slices.any_internal_contains(item)) return true;
    }
    return false;
}
template bool Layer::any_internal_region_slice_contains<Polyline>(const Polyline &item) const;

/// Uses LayerRegion->slices.any_bottom_contains(item)
template <class T>
bool
Layer::any_bottom_region_slice_contains(const T &item) const
{
    FOREACH_LAYERREGION(this, layerm) {
        if ((*layerm)->slices.any_bottom_contains(item)) return true;
    }
    return false;
}
template bool Layer::any_bottom_region_slice_contains<Polyline>(const Polyline &item) const;

/// The perimeter paths and the thin fills (ExtrusionEntityCollection) are assigned to the first compatible layer region.
/// The resulting fill surface is split back among the originating regions.
void
Layer::make_perimeters()
{
    #ifdef SLIC3R_DEBUG
    printf("Making perimeters for layer %zu\n", this->id());
    #endif

    // keep track of regions whose perimeters we have already generated
    std::set<size_t> done;

    FOREACH_LAYERREGION(this, layerm) {
        size_t region_id = layerm - this->regions.begin();
        if (done.find(region_id) != done.end()) continue;
        done.insert(region_id);
        const PrintRegionConfig &config = (*layerm)->region()->config;

        // find compatible regions
        LayerRegionPtrs layerms;
        layerms.push_back(*layerm);
        for (LayerRegionPtrs::const_iterator it = layerm + 1; it != this->regions.end(); ++it) {
            LayerRegion* other_layerm = *it;
            const PrintRegionConfig &other_config = other_layerm->region()->config;

            if (config.perimeter_extruder   == other_config.perimeter_extruder
                && config.perimeters        == other_config.perimeters
                && config.perimeter_speed   == other_config.perimeter_speed
                && config.gap_fill_speed    == other_config.gap_fill_speed
                && config.overhangs         == other_config.overhangs
                && config.serialize("perimeter_extrusion_width").compare(other_config.serialize("perimeter_extrusion_width")) == 0
                && config.thin_walls        == other_config.thin_walls
                && config.external_perimeters_first == other_config.external_perimeters_first) {
                layerms.push_back(other_layerm);
                done.insert(it - this->regions.begin());
            }
        }

        if (layerms.size() == 1) {  // optimization
            (*layerm)->fill_surfaces.surfaces.clear();
            (*layerm)->make_perimeters((*layerm)->slices, &(*layerm)->fill_surfaces);
        } else {
            // group slices (surfaces) according to number of extra perimeters
            std::map<unsigned short,Surfaces> slices;  // extra_perimeters => [ surface, surface... ]
            for (LayerRegionPtrs::iterator l = layerms.begin(); l != layerms.end(); ++l) {
                for (Surfaces::iterator s = (*l)->slices.surfaces.begin(); s != (*l)->slices.surfaces.end(); ++s) {
                    slices[s->extra_perimeters].push_back(*s);
                }
            }

            // merge the surfaces assigned to each group
            SurfaceCollection new_slices;
            for (const auto &it : slices) {
                ExPolygons expp = union_ex(it.second, true);
                for (ExPolygon &ex : expp) {
                    Surface s = it.second.front();  // clone type and extra_perimeters
                    s.expolygon = ex;
                    new_slices.surfaces.push_back(s);
                }
            }

            // make perimeters
            SurfaceCollection fill_surfaces;
            (*layerm)->make_perimeters(new_slices, &fill_surfaces);

            // assign fill_surfaces to each layer
            if (!fill_surfaces.surfaces.empty()) {
                for (LayerRegionPtrs::iterator l = layerms.begin(); l != layerms.end(); ++l) {
                    ExPolygons expp = intersection_ex(
                        (Polygons) fill_surfaces,
                        (Polygons) (*l)->slices
                    );
                    (*l)->fill_surfaces.surfaces.clear();

                    for (ExPolygons::iterator ex = expp.begin(); ex != expp.end(); ++ex) {
                        Surface s = fill_surfaces.surfaces.front();  // clone type and extra_perimeters
                        s.expolygon = *ex;
                        (*l)->fill_surfaces.surfaces.push_back(s);
                    }
                }
            }
        }
    }
}

/// Iterates over all of the LayerRegion and invokes LayerRegion->make_fill()
/// Asserts that the fills created are not NULL
void
Layer::make_fills()
{
    #ifdef SLIC3R_DEBUG
    printf("Making fills for layer %zu\n", this->id());
    #endif

    FOREACH_LAYERREGION(this, it_layerm) {
        (*it_layerm)->make_fill();

        #ifndef NDEBUG
        for (size_t i = 0; i < (*it_layerm)->fills.entities.size(); ++i)
            assert(dynamic_cast<ExtrusionEntityCollection*>((*it_layerm)->fills.entities[i]) != NULL);
        #endif
    }
}

void
Layer::detect_nonplanar_layers()
{
    PrintObject &object = *this->object();
    for (size_t region_id = 0; region_id < this->regions.size(); ++region_id) {
        LayerRegion &layerm = *this->regions[region_id];
        Layer* const &upper_layer = this->upper_layer;

        const Polygons layerm_slices_surfaces = layerm.slices;
        SurfaceCollection topNonplanar;

        if (upper_layer != NULL) {
            Polygons upper_slices;
            if (object.config.interface_shells.value) {
                const LayerRegion* upper_layerm = upper_layer->get_region(region_id);
                boost::lock_guard<boost::mutex> l(upper_layerm->_slices_mutex);
                upper_slices = upper_layerm->slices;
            } else {
                upper_slices = upper_layer->slices;
            }

            topNonplanar.append(
                union_ex(diff(layerm_slices_surfaces, upper_slices, true)),
                //TODO Check if Non Planar, Now every top surface is non planar
                stTopNonplanar
            );
        } else {
            // if no upper layer, all surfaces of this one are solid
            // we clone surfaces because we're going to clear the slices collection
            topNonplanar = layerm.slices;
            //TODO Check if Non Planar, Now every top surface is non planar
            for (Surface &s : topNonplanar.surfaces) s.surface_type = stTopNonplanar;
        }

        {
            boost::lock_guard<boost::mutex> l(layerm._slices_mutex);
            layerm.slices.clear();
            layerm.slices.append(STDMOVE(topNonplanar));

            // find internal surfaces (difference between top/bottom surfaces and others)
            {
                layerm.slices.append(
                    union_ex(diff(layerm_slices_surfaces, topNonplanar, true)),
                    stInternal
                );
            }
        }
    }
}

void
Layer::project_nonplanar_surfaces()
{
    FOREACH_LAYERREGION(this, it_layerm) {
        //for all perimeters
        for (ExtrusionEntitiesPtr::iterator col_it = (*it_layerm)->perimeters.entities.begin(); col_it != (*it_layerm)->perimeters.entities.end(); ++col_it) {
            ExtrusionEntityCollection* collection = dynamic_cast<ExtrusionEntityCollection*>(*col_it);
            for (ExtrusionEntitiesPtr::iterator loop_it = collection->entities.begin(); loop_it != collection->entities.end(); ++loop_it) {
                ExtrusionLoop* loop = dynamic_cast<ExtrusionLoop*>(*loop_it);
                for (ExtrusionPaths::iterator path_it = loop->paths.begin(); path_it != loop->paths.end(); ++path_it) {
                    project_nonplanar_path(&(*path_it));
                    
                    correct_z_on_path(&(*path_it));
                }
            }
        }
        
        //and all fill paths
        for (ExtrusionEntitiesPtr::iterator col_it = (*it_layerm)->fills.entities.begin(); col_it != (*it_layerm)->fills.entities.end(); ++col_it) {
            ExtrusionEntityCollection* collection = dynamic_cast<ExtrusionEntityCollection*>(*col_it);
            for (ExtrusionEntitiesPtr::iterator path_it = collection->entities.begin(); path_it != collection->entities.end(); ++path_it) {
                project_nonplanar_path(dynamic_cast<ExtrusionPath*>(*path_it));
                
                correct_z_on_path(dynamic_cast<ExtrusionPath*>(*path_it));
            }
        }

    }
}

bool
greater(const Point &p1, const Point &p2)
{
   if (p1.x != p2.x) return p1.x < p2.x;
   return p1.y < p2.y;
}

bool
smaller(const Point &p1, const Point &p2)
{
   if (p1.x != p2.x) return p1.x > p2.x;
   return p1.y > p2.y;
}

void
Layer::project_nonplanar_path(ExtrusionPath *path)
{
    //Skip if surface path is not nonplanar
    if ( path->distance_to_top == -1.0f) return;
    
    PrintObject &object = *this->object();
    //First check all points and project them regarding the triangle mesh
    for (Point& point : path->polyline.points) {
        //std::cout << "X: " << unscale(point.x) << " Y: " << unscale(point.y) << '\n';
        for (auto& facet : object.nonplanar_surfaces) {
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
                point.z = point.z - scale_(path->distance_to_top);
                break;
            } 
        }
    }

    //Then check all line intersections, cut line on intersection and project new point
    std::vector<Point>::size_type size = path->polyline.points.size();
    for (std::vector<Point>::size_type i = 0; i < size; ++i)
    {
        if (i == size-1) break;
        //unscale points of line
        Pointf3 p3 = Pointf3(unscale(path->polyline.points[i].x), unscale(path->polyline.points[i].y), unscale(path->polyline.points[i].z));
        Pointf3 p4 = Pointf3(unscale(path->polyline.points[i+1].x), unscale(path->polyline.points[i+1].y), unscale(path->polyline.points[i+1].z));
        
        Points intersections;
        //check against every facet if lines intersect
        for (auto& facet : object.nonplanar_surfaces) {
            for(int j= 0; j < 3; j++){
                //TODO precheck for faster computation
                Pointf3 p1 = Pointf3(facet.second.vertex[j % 3].x, facet.second.vertex[j % 3].y, facet.second.vertex[j % 3].z);
                Pointf3 p2 = Pointf3(facet.second.vertex[(j+1) % 3].x, facet.second.vertex[(j+1) % 3].y, facet.second.vertex[(j+1) % 3].z);
                Point* p = Slic3r::Geometry::Line_intersection(p1,p2,p3,p4);
                if (p) {
                    intersections.push_back(*p);
                }
            }
        }
        //Stop if no intersections are found
        if (intersections.size() == 0) continue;
        
        //sort found intersectons if there are more than 1
        if ( intersections.size() > 1 ){
            if ( p3.x < p4.x || (p3.x == p4.x && p3.y < p4.y )){ 
                std::sort(intersections.begin(), intersections.end(),smaller);
            }
            else {
                std::sort(intersections.begin(), intersections.end(),greater);
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
            //add distance to top for every added point
            p.z = p.z - scale_(path->distance_to_top);
            path->polyline.points.insert(path->polyline.points.begin()+i+1, p);
        }
        
        //modifiy array boundary
        i = i + intersections.size();
        size = size + intersections.size();
    }
}

void
Layer::correct_z_on_path(ExtrusionPath *path)
{
    for (Point& point : path->polyline.points) {
        if(point.z == -1.0){
            point.z = scale_(this->print_z);
        }
    }
}

/// Analyzes slices of a region (SurfaceCollection slices).
/// Each region slice (instance of Surface) is analyzed, whether it is supported or whether it is the top surface.
/// Initially all slices are of type S_TYPE_INTERNAL.
/// Slices are compared against the top / bottom slices and regions and classified to the following groups:
/// S_TYPE_TOP - Part of a region, which is not covered by any upper layer. This surface will be filled with a top solid infill.
/// S_TYPE_BOTTOMBRIDGE - Part of a region, which is not fully supported, but it hangs in the air, or it hangs losely on a support or a raft.
/// S_TYPE_BOTTOM - Part of a region, which is not supported by the same region, but it is supported either by another region, or by a soluble interface layer.
/// S_TYPE_INTERNAL - Part of a region, which is supported by the same region type.
/// If a part of a region is of S_TYPE_BOTTOM and S_TYPE_TOP, the S_TYPE_BOTTOM wins.
void
Layer::detect_surfaces_type()
{
    PrintObject &object = *this->object();

    for (size_t region_id = 0; region_id < this->regions.size(); ++region_id) {
        LayerRegion &layerm = *this->regions[region_id];

        // comparison happens against the *full* slices (considering all regions)
        // unless internal shells are requested

        // We call layer->slices or layerm->slices on these neighbor layers
        // and we convert them into Polygons so we only care about their total
        // coverage. We only write to layerm->slices so we can read layer->slices safely.
        Layer* const &upper_layer = this->upper_layer;
        Layer* const &lower_layer = this->lower_layer;

        // collapse very narrow parts (using the safety offset in the diff is not enough)
        // TODO: this offset2 makes this method not idempotent (see #3764), so we should
        // move it to where we generate fill_surfaces instead and leave slices unaltered
        const float offs = layerm.flow(frExternalPerimeter).scaled_width() / 10.f;

        const Polygons layerm_slices_surfaces = layerm.slices;

        //Find previously marked nonplanar surfaces
        SurfaceCollection nonplanar_surfaces;
        for (Surfaces::iterator surface = layerm.slices.surfaces.begin(); surface != layerm.slices.surfaces.end(); ++surface) {
            if (surface->surface_type == stTopNonplanar || surface->surface_type == stInternalSolidNonplanar) {
                Surface s = *surface;
                nonplanar_surfaces.surfaces.push_back(s);
            }
        }
        //remove non planar surfaces form all surfaces to get planar surfaces
        Polygons planar_surfaces = diff(layerm_slices_surfaces,nonplanar_surfaces,true);

        // find top surfaces (difference between current surfaces
        // of current layer and upper one)
        SurfaceCollection top;
        if (upper_layer != NULL) {
            Polygons upper_slices;
            if (object.config.interface_shells.value) {
                const LayerRegion* upper_layerm = upper_layer->get_region(region_id);
                boost::lock_guard<boost::mutex> l(upper_layerm->_slices_mutex);
                upper_slices = upper_layerm->slices;
            } else {
                upper_slices = upper_layer->slices;
            }
            //difference between all surfaces and upper surfaces subtracted by nonplanar surfaces
            top.append(
                offset2_ex(
                    diff(diff(layerm_slices_surfaces, upper_slices, true),nonplanar_surfaces,true),
                    -offs, offs
                ),
                stTop
            );
        } else {
            // all planar surfaces are top surfaces1
            top.append(
                union_ex(planar_surfaces,true),
                stTop
            );
        }

        // find bottom surfaces (difference between current surfaces
        // of current layer and lower one)
        SurfaceCollection bottom;
        if (lower_layer != NULL) {
            // If we have soluble support material, don't bridge. The overhang will be squished against a soluble layer separating
            // the support from the print.
            const SurfaceType surface_type_bottom =
                (object.config.support_material.value && object.config.support_material_contact_distance.value == 0)
                ? stBottom
                : stBottomBridge;

            // Any surface lying on the void is a true bottom bridge (an overhang)
            bottom.append(
                offset2_ex(
                    diff(diff(layerm_slices_surfaces, lower_layer->slices, true),nonplanar_surfaces,true),
                    -offs, offs
                ),
                surface_type_bottom
            );

            // if user requested internal shells, we need to identify surfaces
            // lying on other slices not belonging to this region
            if (object.config.interface_shells) {
                // non-bridging bottom surfaces: any part of this layer lying
                // on something else, excluding those lying on our own region
                const LayerRegion* lower_layerm = lower_layer->get_region(region_id);
                boost::lock_guard<boost::mutex> l(lower_layerm->_slices_mutex);
                bottom.append(
                    offset2_ex(
                        diff(
                            diff(
                                intersection(layerm_slices_surfaces, lower_layer->slices), // supported
                                lower_layerm->slices,
                                true
                            ),
                            nonplanar_surfaces,
                            true
                        ),
                        -offs, offs
                    ),
                    stBottom
                );
            }
        } else {
            // if no lower layer, all surfaces of this one are solid
            // we clone surfaces because we're going to clear the slices collection
            bottom = layerm.slices;

            // if we have raft layers, consider bottom layer as a bridge
            // just like any other bottom surface lying on the void
            const SurfaceType surface_type_bottom =
                (object.config.raft_layers.value > 0 && object.config.support_material_contact_distance.value > 0)
                ? stBottomBridge
                : stBottom;
            for (Surface &s : bottom.surfaces) s.surface_type = surface_type_bottom;
        }

        // now, if the object contained a thin membrane, we could have overlapping bottom
        // and top surfaces; let's do an intersection to discover them and consider them
        // as bottom surfaces (to allow for bridge detection)
        if (!top.empty() && !bottom.empty()) {
            const Polygons top_polygons = to_polygons(STDMOVE(top));
            top.clear();
            top.append(
                // TODO: maybe we don't need offset2?
                offset2_ex(diff(top_polygons, bottom, true), -offs, offs),
                stTop
            );
        }

        // save surfaces to layer
        {
            boost::lock_guard<boost::mutex> l(layerm._slices_mutex);
            layerm.slices.clear();
            layerm.slices.append(STDMOVE(top));
            layerm.slices.append(STDMOVE(bottom));
            layerm.slices.append(STDMOVE(nonplanar_surfaces));

            // find internal surfaces (difference between top/bottom surfaces and others)
            {
                Polygons solid_surfaces = top;
                append_to(solid_surfaces, (Polygons)bottom);
                append_to(solid_surfaces, (Polygons)nonplanar_surfaces);

                layerm.slices.append(
                    // TODO: maybe we don't need offset2?
                    offset2_ex(
                        diff(layerm_slices_surfaces, solid_surfaces, true),
                        -offs, offs
                    ),
                    stInternal
                );
            }
        }

        #ifdef SLIC3R_DEBUG
        printf("  layer %zu has %zu bottom, %zu top and %zu internal surfaces\n",
            this->id(), bottom.size(), top.size(),
            layerm.slices.size()-bottom.size()-top.size());
        #endif

        {
            /*  Fill in layerm->fill_surfaces by trimming the layerm->slices by the cummulative layerm->fill_surfaces.
                Note: this method should be idempotent, but fill_surfaces gets modified
                in place. However we're now only using its boundaries (which are invariant)
                so we're safe. This guarantees idempotence of prepare_infill() also in case
                that combine_infill() turns some fill_surface into VOID surfaces.  */
            const Polygons fill_boundaries = layerm.fill_surfaces;
            layerm.fill_surfaces.clear();
            // No other instance of this function is writing to this layer, so we can read safely.
            for (const Surface &surface : layerm.slices.surfaces) {
                // No other instance of this function modifies fill_surfaces.
                layerm.fill_surfaces.append(
                    intersection_ex(surface, fill_boundaries),
                    surface.surface_type,
                    surface.distance_to_top
                );
            }
        }
    }
}

///Iterates over all LayerRegions and invokes LayerRegion->process_external_surfaces
void
Layer::process_external_surfaces()
{
    for (LayerRegion* &layerm : this->regions)
        layerm->process_external_surfaces();
}

}
