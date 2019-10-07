#ifndef slic3r_FillRectilinear2_hpp_
#define slic3r_FillRectilinear2_hpp_

#include "../libslic3r.h"

#include "FillBase.hpp"

namespace Slic3r {

class Surface;
class SegmentedIntersectionLine;
struct ExPolygonWithOffset;

class FillRectilinear2 : public Fill
{
public:
    virtual Fill* clone() const { return new FillRectilinear2(*this); };
    virtual ~FillRectilinear2() {}
    virtual Polylines fill_surface(const Surface *surface, const FillParams &params) override;

protected:
    virtual std::vector<SegmentedIntersectionLine> _vert_lines_for_polygon(const ExPolygonWithOffset &poly_with_offset, const BoundingBox &bounding_box, const FillParams &params, coord_t line_spacing) const;
    virtual coord_t _line_spacing_for_density(float density) const;

    bool fill_surface_by_lines(const Surface *surface, const FillParams &params, float angleBase, float pattern_shift, Polylines &polylines_out);
};

class FillGrid2 : public FillRectilinear2
{
public:
    virtual Fill* clone() const { return new FillGrid2(*this); };
    virtual ~FillGrid2() {}
    virtual Polylines fill_surface(const Surface *surface, const FillParams &params) override;

protected:
    // The grid fill will keep the angle constant between the layers, see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t idx) const { return 0.f; }
};

class FillTriangles : public FillRectilinear2
{
public:
    virtual Fill* clone() const { return new FillTriangles(*this); };
    virtual ~FillTriangles() {}
    virtual Polylines fill_surface(const Surface *surface, const FillParams &params) override;

protected:
    // The grid fill will keep the angle constant between the layers, see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t idx) const { return 0.f; }
};

class FillStars : public FillRectilinear2
{
public:
    virtual Fill* clone() const { return new FillStars(*this); };
    virtual ~FillStars() {}
    virtual Polylines fill_surface(const Surface *surface, const FillParams &params) override;

protected:
    // The grid fill will keep the angle constant between the layers, see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t idx) const { return 0.f; }
};

class FillCubic : public FillRectilinear2
{
public:
    virtual Fill* clone() const { return new FillCubic(*this); };
    virtual ~FillCubic() {}
    virtual Polylines fill_surface(const Surface *surface, const FillParams &params) override;

protected:
    // The grid fill will keep the angle constant between the layers, see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t idx) const { return 0.f; }
};

class FillRectilinear2Peri : public FillRectilinear2
{
public:
    // require bridge flow since it's a pre-bridge over very sparse infill
    virtual bool use_bridge_flow() const { return true; }

    virtual Fill* clone() const { return new FillRectilinear2Peri(*this); };
    virtual ~FillRectilinear2Peri() {}
    //virtual Polylines fill_surface(const Surface *surface, const FillParams &params);
    virtual void fill_surface_extrusion(const Surface *surface, const FillParams &params, ExtrusionEntitiesPtr &out) override;

};


class FillScatteredRectilinear : public FillRectilinear2
{
public:
    virtual Fill* clone() const override{ return new FillScatteredRectilinear(*this); };
    virtual ~FillScatteredRectilinear() {}
    virtual Polylines fill_surface(const Surface *surface, const FillParams &params) override;

protected:
    virtual float _layer_angle(size_t idx) const;
    virtual std::vector<SegmentedIntersectionLine> _vert_lines_for_polygon(const ExPolygonWithOffset &poly_with_offset, const BoundingBox &bounding_box, const FillParams &params, coord_t line_spacing) const;
    virtual coord_t _line_spacing_for_density(float density) const;
};

class FillRectilinearSawtooth : public FillRectilinear2 {
public:
    // require bridge flow since it's a pre-bridge over very sparse infill
    virtual bool use_bridge_flow() const { return true; }

    virtual Fill* clone() const { return new FillRectilinearSawtooth(*this); };
    virtual ~FillRectilinearSawtooth() {}
    virtual void fill_surface_extrusion(const Surface *surface, const FillParams &params, ExtrusionEntitiesPtr &out) override;

};

class FillRectilinear2WGapFill : public FillRectilinear2
{
public:

    virtual Fill* clone() const { return new FillRectilinear2WGapFill(*this); };
    virtual ~FillRectilinear2WGapFill() {}
    virtual void fill_surface_extrusion(const Surface *surface, const FillParams &params, ExtrusionEntitiesPtr &out) override;
    static void split_polygon_gap_fill(const Surface &surface, const FillParams &params, ExPolygons &rectilinear, ExPolygons &gapfill);
};


}; // namespace Slic3r

#endif // slic3r_FillRectilinear2_hpp_
