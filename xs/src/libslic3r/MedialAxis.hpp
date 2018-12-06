#ifndef slic3r_MedialAxis_hpp_
#define slic3r_MedialAxis_hpp_

#include "libslic3r.h"
#include "ExPolygon.hpp"
#include "Polyline.hpp"
#include "Geometry.hpp"
#include <vector>

#include "boost/polygon/voronoi.hpp"
using boost::polygon::voronoi_builder;
using boost::polygon::voronoi_diagram;

namespace Slic3r {

    class MedialAxis {
    public:
        Lines lines; //lines is here only to avoid appassing it in argument of amny method. Initialized in polyline_from_voronoi.
        ExPolygon expolygon;
        const ExPolygon& bounds;
        const ExPolygon& surface;
        const double max_width;
        const double min_width;
        const double height;
        bool do_not_overextrude = true;
        MedialAxis(const ExPolygon &_expolygon, const ExPolygon &_bounds, const double _max_width, const double _min_width, const double _height)
            : surface(_expolygon), bounds(_bounds), max_width(_max_width), min_width(_min_width), height(_height) {
        };
        void build(ThickPolylines* polylines_out);
        void build(Polylines* polylines);
        
    private:
        static int id;
        class VD : public voronoi_diagram<double> {
        public:
            typedef double                                          coord_type;
            typedef boost::polygon::point_data<coordinate_type>     point_type;
            typedef boost::polygon::segment_data<coordinate_type>   segment_type;
            typedef boost::polygon::rectangle_data<coordinate_type> rect_type;
        };
        VD vd;
        std::set<const VD::edge_type*> edges, valid_edges;
        std::map<const VD::edge_type*, std::pair<coordf_t, coordf_t> > thickness;
        void process_edge_neighbors(const VD::edge_type* edge, ThickPolyline* polyline);
        bool validate_edge(const VD::edge_type* edge);
        const Line& retrieve_segment(const VD::cell_type* cell) const;
        const Point& retrieve_endpoint(const VD::cell_type* cell) const;
        void polyline_from_voronoi(const Lines& voronoi_edges, ThickPolylines* polylines_out);

        ExPolygon simplify_polygon_frontier();
        void fusion_curve(ThickPolylines &pp);
        void main_fusion(ThickPolylines& pp);
        void fusion_corners(ThickPolylines &pp);
        void extends_line(ThickPolyline& polyline, const ExPolygons& anchors, const coord_t join_width);
        void remove_too_thin_extrusion(ThickPolylines& pp);
        void concatenate_polylines_with_crossing(ThickPolylines& pp);
        void remove_too_thin_points(ThickPolylines& pp);
        void remove_too_short_polylines(ThickPolylines& pp, const coord_t min_size);
        void ensure_not_overextrude(ThickPolylines& pp);
    };
}


#endif
