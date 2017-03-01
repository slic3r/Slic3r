#include "ClipperUtils.hpp"
#include "Geometry.hpp"

namespace Slic3r {

//-----------------------------------------------------------
// legacy code from Clipper documentation
void AddOuterPolyNodeToExPolygons(ClipperLib::PolyNode& polynode, ExPolygons* expolygons)
{  
  size_t cnt = expolygons->size();
  expolygons->resize(cnt + 1);
  (*expolygons)[cnt].contour = ClipperPath_to_Slic3rMultiPoint<Polygon>(polynode.Contour);
  (*expolygons)[cnt].holes.resize(polynode.ChildCount());
  for (int i = 0; i < polynode.ChildCount(); ++i)
  {
    (*expolygons)[cnt].holes[i] = ClipperPath_to_Slic3rMultiPoint<Polygon>(polynode.Childs[i]->Contour);
    //Add outer polygons contained by (nested within) holes ...
    for (int j = 0; j < polynode.Childs[i]->ChildCount(); ++j)
      AddOuterPolyNodeToExPolygons(*polynode.Childs[i]->Childs[j], expolygons);
  }
}
 
ExPolygons
PolyTreeToExPolygons(ClipperLib::PolyTree& polytree)
{
    ExPolygons retval;
    for (int i = 0; i < polytree.ChildCount(); ++i)
        AddOuterPolyNodeToExPolygons(*polytree.Childs[i], &retval);
    return retval;
}
//-----------------------------------------------------------

template <class T>
T
ClipperPath_to_Slic3rMultiPoint(const ClipperLib::Path &input)
{
    T retval;
    for (ClipperLib::Path::const_iterator pit = input.begin(); pit != input.end(); ++pit)
        retval.points.push_back(Point( (*pit).X, (*pit).Y ));
    return retval;
}

template <class T>
T
ClipperPaths_to_Slic3rMultiPoints(const ClipperLib::Paths &input)
{
    T retval;
    for (ClipperLib::Paths::const_iterator it = input.begin(); it != input.end(); ++it)
        retval.push_back(ClipperPath_to_Slic3rMultiPoint<typename T::value_type>(*it));
    return retval;
}

ExPolygons
ClipperPaths_to_Slic3rExPolygons(const ClipperLib::Paths &input)
{
    // init Clipper
    ClipperLib::Clipper clipper;
    clipper.Clear();
    
    // perform union
    clipper.AddPaths(input, ClipperLib::ptSubject, true);
    ClipperLib::PolyTree polytree;
    clipper.Execute(ClipperLib::ctUnion, polytree, ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd);  // offset results work with both EvenOdd and NonZero
    
    // write to ExPolygons object
    return PolyTreeToExPolygons(polytree);
}

ClipperLib::Path
Slic3rMultiPoint_to_ClipperPath(const MultiPoint &input)
{
    ClipperLib::Path retval;
    for (Points::const_iterator pit = input.points.begin(); pit != input.points.end(); ++pit)
        retval.push_back(ClipperLib::IntPoint( (*pit).x, (*pit).y ));
    return retval;
}

template <class T>
ClipperLib::Paths
Slic3rMultiPoints_to_ClipperPaths(const T &input)
{
    ClipperLib::Paths retval;
    for (typename T::const_iterator it = input.begin(); it != input.end(); ++it)
        retval.push_back(Slic3rMultiPoint_to_ClipperPath(*it));
    return retval;
}

void
scaleClipperPolygons(ClipperLib::Paths &polygons, const double scale)
{
    for (ClipperLib::Paths::iterator it = polygons.begin(); it != polygons.end(); ++it) {
        for (ClipperLib::Path::iterator pit = (*it).begin(); pit != (*it).end(); ++pit) {
            (*pit).X *= scale;
            (*pit).Y *= scale;
        }
    }
}

ClipperLib::Paths
_offset(const Polygons &polygons, const float delta,
    double scale, ClipperLib::JoinType joinType, double miterLimit)
{
    // read input
    ClipperLib::Paths input = Slic3rMultiPoints_to_ClipperPaths(polygons);
    
    // scale input
    scaleClipperPolygons(input, scale);
    
    // perform offset
    ClipperLib::ClipperOffset co;
    if (joinType == jtRound) {
        co.ArcTolerance = miterLimit;
    } else {
        co.MiterLimit = miterLimit;
    }
    co.AddPaths(input, joinType, ClipperLib::etClosedPolygon);
    ClipperLib::Paths retval;
    co.Execute(retval, (delta*scale));
    
    // unscale output
    scaleClipperPolygons(retval, 1/scale);
    return retval;
}

ClipperLib::Paths
_offset(const Polylines &polylines, const float delta,
    double scale, ClipperLib::JoinType joinType, double miterLimit)
{
    // read input
    ClipperLib::Paths input = Slic3rMultiPoints_to_ClipperPaths(polylines);
    
    // scale input
    scaleClipperPolygons(input, scale);
    
    // perform offset
    ClipperLib::ClipperOffset co;
    if (joinType == jtRound) {
        co.ArcTolerance = miterLimit;
    } else {
        co.MiterLimit = miterLimit;
    }
    co.AddPaths(input, joinType, ClipperLib::etOpenButt);
    ClipperLib::Paths retval;
    co.Execute(retval, (delta*scale));
    
    // unscale output
    scaleClipperPolygons(retval, 1/scale);
    return retval;
}

Polygons
offset(const Polygons &polygons, const float delta,
    double scale, ClipperLib::JoinType joinType, double miterLimit)
{
    // perform offset
    ClipperLib::Paths output = _offset(polygons, delta, scale, joinType, miterLimit);
    
    // convert into Polygons
    return ClipperPaths_to_Slic3rMultiPoints<Polygons>(output);
}

Polygons
offset(const Polylines &polylines, const float delta,
    double scale, ClipperLib::JoinType joinType, double miterLimit)
{
    // perform offset
    ClipperLib::Paths output = _offset(polylines, delta, scale, joinType, miterLimit);
    
    // convert into Polygons
    return ClipperPaths_to_Slic3rMultiPoints<Polygons>(output);
}

Surfaces
offset(const Surface &surface, const float delta,
    double scale, ClipperLib::JoinType joinType, double miterLimit)
{
    // perform offset
    ExPolygons expp = offset_ex(surface.expolygon, delta, scale, joinType, miterLimit);
    
    // clone the input surface for each expolygon we got
    Surfaces retval;
    retval.reserve(expp.size());
    for (ExPolygons::iterator it = expp.begin(); it != expp.end(); ++it) {
        Surface s = surface;  // clone
        s.expolygon = *it;
        retval.push_back(s);
    }
    return retval;
}

ExPolygons
offset_ex(const Polygons &polygons, const float delta,
    double scale, ClipperLib::JoinType joinType, double miterLimit)
{
    // perform offset
    ClipperLib::Paths output = _offset(polygons, delta, scale, joinType, miterLimit);
    
    // convert into ExPolygons
    return ClipperPaths_to_Slic3rExPolygons(output);
}

ExPolygons
offset_ex(const ExPolygons &expolygons, const float delta,
    double scale, ClipperLib::JoinType joinType, double miterLimit)
{
    return offset_ex(to_polygons(expolygons), delta, scale, joinType, miterLimit);
}

ClipperLib::Paths
_offset2(const Polygons &polygons, const float delta1, const float delta2,
    const double scale, const ClipperLib::JoinType joinType, const double miterLimit)
{
    // read input
    ClipperLib::Paths input = Slic3rMultiPoints_to_ClipperPaths(polygons);
    
    // scale input
    scaleClipperPolygons(input, scale);
    
    // prepare ClipperOffset object
    ClipperLib::ClipperOffset co;
    if (joinType == jtRound) {
        co.ArcTolerance = miterLimit;
    } else {
        co.MiterLimit = miterLimit;
    }
    
    // perform first offset
    ClipperLib::Paths output1;
    co.AddPaths(input, joinType, ClipperLib::etClosedPolygon);
    co.Execute(output1, (delta1*scale));
    
    // perform second offset
    co.Clear();
    co.AddPaths(output1, joinType, ClipperLib::etClosedPolygon);
    ClipperLib::Paths retval;
    co.Execute(retval, (delta2*scale));
    
    // unscale output
    scaleClipperPolygons(retval, 1/scale);
    return retval;
}

Polygons
offset2(const Polygons &polygons, const float delta1, const float delta2,
    const double scale, const ClipperLib::JoinType joinType, const double miterLimit)
{
    // perform offset
    ClipperLib::Paths output = _offset2(polygons, delta1, delta2, scale, joinType, miterLimit);
    
    // convert into ExPolygons
    return ClipperPaths_to_Slic3rMultiPoints<Polygons>(output);
}

ExPolygons
offset2_ex(const Polygons &polygons, const float delta1, const float delta2,
    const double scale, const ClipperLib::JoinType joinType, const double miterLimit)
{
    // perform offset
    ClipperLib::Paths output = _offset2(polygons, delta1, delta2, scale, joinType, miterLimit);
    
    // convert into ExPolygons
    return ClipperPaths_to_Slic3rExPolygons(output);
}

template <class T>
T
_clipper_do(const ClipperLib::ClipType clipType, const Polygons &subject, 
    const Polygons &clip, const ClipperLib::PolyFillType fillType, const bool safety_offset_)
{
    // read input
    ClipperLib::Paths input_subject = Slic3rMultiPoints_to_ClipperPaths(subject);
    ClipperLib::Paths input_clip    = Slic3rMultiPoints_to_ClipperPaths(clip);
    
    // perform safety offset
    if (safety_offset_) {
        if (clipType == ClipperLib::ctUnion) {
            safety_offset(&input_subject);
        } else {
            safety_offset(&input_clip);
        }
    }
    
    // init Clipper
    ClipperLib::Clipper clipper;
    clipper.Clear();
    
    // add polygons
    clipper.AddPaths(input_subject, ClipperLib::ptSubject, true);
    clipper.AddPaths(input_clip,    ClipperLib::ptClip,    true);
    
    // perform operation
    T retval;
    clipper.Execute(clipType, retval, fillType, fillType);
    return retval;
}

ClipperLib::PolyTree
_clipper_do(const ClipperLib::ClipType clipType, const Polylines &subject, 
    const Polygons &clip, const ClipperLib::PolyFillType fillType,
    const bool safety_offset_)
{
    // read input
    ClipperLib::Paths input_subject = Slic3rMultiPoints_to_ClipperPaths(subject);
    ClipperLib::Paths input_clip    = Slic3rMultiPoints_to_ClipperPaths(clip);
    
    // perform safety offset
    if (safety_offset_) safety_offset(&input_clip);
    
    // init Clipper
    ClipperLib::Clipper clipper;
    clipper.Clear();
    
    // add polygons
    clipper.AddPaths(input_subject, ClipperLib::ptSubject, false);
    clipper.AddPaths(input_clip,    ClipperLib::ptClip,    true);
    
    // perform operation
    ClipperLib::PolyTree retval;
    clipper.Execute(clipType, retval, fillType, fillType);
    return retval;
}

Polygons
_clipper(ClipperLib::ClipType clipType, const Polygons &subject, 
    const Polygons &clip, bool safety_offset_)
{
    // perform operation
    ClipperLib::Paths output = _clipper_do<ClipperLib::Paths>(clipType, subject, clip, ClipperLib::pftNonZero, safety_offset_);
    
    // convert into Polygons
    return ClipperPaths_to_Slic3rMultiPoints<Polygons>(output);
}

ExPolygons
_clipper_ex(ClipperLib::ClipType clipType, const Polygons &subject, 
    const Polygons &clip, bool safety_offset_)
{
    // perform operation
    ClipperLib::PolyTree polytree = _clipper_do<ClipperLib::PolyTree>(clipType, subject, clip, ClipperLib::pftNonZero, safety_offset_);
    
    // convert into ExPolygons
    return PolyTreeToExPolygons(polytree);
}

Polylines
_clipper_pl(ClipperLib::ClipType clipType, const Polylines &subject, 
    const Polygons &clip, bool safety_offset_)
{
    // perform operation
    ClipperLib::PolyTree polytree = _clipper_do(clipType, subject, clip, ClipperLib::pftNonZero, safety_offset_);
    
    // convert into Polylines
    ClipperLib::Paths output;
    ClipperLib::PolyTreeToPaths(polytree, output);
    return ClipperPaths_to_Slic3rMultiPoints<Polylines>(output);
}

Polylines
_clipper_pl(ClipperLib::ClipType clipType, const Polygons &subject, 
    const Polygons &clip, bool safety_offset_)
{
    // transform input polygons into polylines
    Polylines polylines;
    polylines.reserve(subject.size());
    for (Polygons::const_iterator polygon = subject.begin(); polygon != subject.end(); ++polygon)
        polylines.push_back(*polygon);  // implicit call to split_at_first_point()
    
    // perform clipping
    Polylines retval = _clipper_pl(clipType, polylines, clip, safety_offset_);
    
    /* If the split_at_first_point() call above happens to split the polygon inside the clipping area
       we would get two consecutive polylines instead of a single one, so we go through them in order
       to recombine continuous polylines. */
    for (size_t i = 0; i < retval.size(); ++i) {
        for (size_t j = i+1; j < retval.size(); ++j) {
            if (retval[i].points.back().coincides_with(retval[j].points.front())) {
                /* If last point of i coincides with first point of j,
                   append points of j to i and delete j */
                retval[i].points.insert(retval[i].points.end(), retval[j].points.begin()+1, retval[j].points.end());
                retval.erase(retval.begin() + j);
                --j;
            } else if (retval[i].points.front().coincides_with(retval[j].points.back())) {
                /* If first point of i coincides with last point of j,
                   prepend points of j to i and delete j */
                retval[i].points.insert(retval[i].points.begin(), retval[j].points.begin(), retval[j].points.end()-1);
                retval.erase(retval.begin() + j);
                --j;
            } else if (retval[i].points.front().coincides_with(retval[j].points.front())) {
                /* Since Clipper does not preserve orientation of polylines, 
                   also check the case when first point of i coincides with first point of j. */
                retval[j].reverse();
                retval[i].points.insert(retval[i].points.begin(), retval[j].points.begin(), retval[j].points.end()-1);
                retval.erase(retval.begin() + j);
                --j;
            } else if (retval[i].points.back().coincides_with(retval[j].points.back())) {
                /* Since Clipper does not preserve orientation of polylines, 
                   also check the case when last point of i coincides with last point of j. */
                retval[j].reverse();
                retval[i].points.insert(retval[i].points.end(), retval[j].points.begin()+1, retval[j].points.end());
                retval.erase(retval.begin() + j);
                --j;
            }
        }
    }
    return retval;
}

Lines
_clipper_ln(ClipperLib::ClipType clipType, const Lines &subject, const Polygons &clip,
    bool safety_offset_)
{
    // convert Lines to Polylines
    Polylines polylines;
    polylines.reserve(subject.size());
    for (Lines::const_iterator line = subject.begin(); line != subject.end(); ++line)
        polylines.push_back(*line);
    
    // perform operation
    polylines = _clipper_pl(clipType, polylines, clip, safety_offset_);
    
    // convert Polylines to Lines
    Lines retval;
    for (Polylines::const_iterator polyline = polylines.begin(); polyline != polylines.end(); ++polyline)
        retval.push_back(*polyline);
    return retval;
}

ClipperLib::PolyTree
union_pt(const Polygons &subject, bool safety_offset_)
{
    return _clipper_do<ClipperLib::PolyTree>(ClipperLib::ctUnion, subject, Polygons(), ClipperLib::pftEvenOdd, safety_offset_);
}

Polygons
union_pt_chained(const Polygons &subject, bool safety_offset_)
{
    ClipperLib::PolyTree polytree = union_pt(subject, safety_offset_);
    
    Polygons retval;
    traverse_pt(polytree.Childs, &retval);
    return retval;
}

void
traverse_pt(ClipperLib::PolyNodes &nodes, Polygons* retval)
{
    /* use a nearest neighbor search to order these children
       TODO: supply start_near to chained_path() too? */
    
    // collect ordering points
    Points ordering_points;
    ordering_points.reserve(nodes.size());
    for (ClipperLib::PolyNodes::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        Point p((*it)->Contour.front().X, (*it)->Contour.front().Y);
        ordering_points.push_back(p);
    }
    
    // perform the ordering
    ClipperLib::PolyNodes ordered_nodes;
    Slic3r::Geometry::chained_path_items(ordering_points, nodes, ordered_nodes);
    
    // push results recursively
    for (ClipperLib::PolyNodes::iterator it = ordered_nodes.begin(); it != ordered_nodes.end(); ++it) {
        // traverse the next depth
        traverse_pt((*it)->Childs, retval);
        
        Polygon p = ClipperPath_to_Slic3rMultiPoint<Polygon>((*it)->Contour);
        retval->push_back(p);
        if ((*it)->IsHole()) retval->back().reverse();  // ccw
    }
}

Polygons
simplify_polygons(const Polygons &subject, bool preserve_collinear)
{
    // convert into Clipper polygons
    ClipperLib::Paths input_subject = Slic3rMultiPoints_to_ClipperPaths(subject);
    
    ClipperLib::Paths output;
    if (preserve_collinear) {
        ClipperLib::Clipper c;
        c.PreserveCollinear(true);
        c.StrictlySimple(true);
        c.AddPaths(input_subject, ClipperLib::ptSubject, true);
        c.Execute(ClipperLib::ctUnion, output, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
    } else {
        ClipperLib::SimplifyPolygons(input_subject, output, ClipperLib::pftNonZero);
    }
    
    // convert into Slic3r polygons
    return ClipperPaths_to_Slic3rMultiPoints<Polygons>(output);
}

ExPolygons
simplify_polygons_ex(const Polygons &subject, bool preserve_collinear)
{
    if (!preserve_collinear) {
        return union_ex(simplify_polygons(subject, preserve_collinear));
    }
    
    // convert into Clipper polygons
    ClipperLib::Paths input_subject = Slic3rMultiPoints_to_ClipperPaths(subject);
    
    ClipperLib::PolyTree polytree;
    
    ClipperLib::Clipper c;
    c.PreserveCollinear(true);
    c.StrictlySimple(true);
    c.AddPaths(input_subject, ClipperLib::ptSubject, true);
    c.Execute(ClipperLib::ctUnion, polytree, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
    
    // convert into ExPolygons
    return PolyTreeToExPolygons(polytree);
}

void safety_offset(ClipperLib::Paths* paths)
{
    // scale input
    scaleClipperPolygons(*paths, CLIPPER_OFFSET_SCALE);
    
    // perform offset (delta = scale 1e-05)
    ClipperLib::ClipperOffset co;
    co.MiterLimit = 2;
    co.AddPaths(*paths, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
    co.Execute(*paths, 10.0 * CLIPPER_OFFSET_SCALE);
    
    // unscale output
    scaleClipperPolygons(*paths, 1.0/CLIPPER_OFFSET_SCALE);
}

}
