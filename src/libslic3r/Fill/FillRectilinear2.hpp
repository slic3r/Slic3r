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
    virtual Fill* clone() const override { return new FillRectilinear2(*this); };
    virtual ~FillRectilinear2() = default;
    virtual void init_spacing(coordf_t spacing, const FillParams &params) override;
    virtual Polylines fill_surface(const Surface *surface, const FillParams &params) const override;

protected:
    virtual std::vector<SegmentedIntersectionLine> _vert_lines_for_polygon(const ExPolygonWithOffset &poly_with_offset, const BoundingBox &bounding_box, const FillParams &params, coord_t line_spacing) const;

    bool fill_surface_by_lines(const Surface *surface, const FillParams &params, float angleBase, float pattern_shift, Polylines &polylines_out) const;
};

class FillMonotonous : public FillRectilinear2
{
public:
    virtual Fill* clone() const { return new FillMonotonous(*this); };
    virtual ~FillMonotonous() = default;
    virtual Polylines fill_surface(const Surface * surface, const FillParams & params) const override;
    virtual bool no_sort() const { return true; }
};

class FillGrid2 : public FillRectilinear2
{
public:
    virtual Fill* clone() const override { return new FillGrid2(*this); };
    virtual ~FillGrid2() = default;
    virtual Polylines fill_surface(const Surface *surface, const FillParams &params) const override;

protected:
    // The grid fill will keep the angle constant between the layers, see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t idx) const override { return 0.f; }
};

class FillTriangles : public FillRectilinear2
{
public:
    virtual Fill* clone() const override { return new FillTriangles(*this); };
    virtual ~FillTriangles() = default;
    virtual Polylines fill_surface(const Surface *surface, const FillParams &params) const override;

protected:
    // The grid fill will keep the angle constant between the layers, see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t idx) const override { return 0.f; }
};

class FillStars : public FillRectilinear2
{
public:
    virtual Fill* clone() const override { return new FillStars(*this); };
    virtual ~FillStars() = default;
    virtual Polylines fill_surface(const Surface *surface, const FillParams &params) const override;

protected:
    // The grid fill will keep the angle constant between the layers, see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t idx) const override { return 0.f; }
};

class FillCubic : public FillRectilinear2
{
public:
    virtual Fill* clone() const override { return new FillCubic(*this); };
    virtual ~FillCubic() = default;
    virtual Polylines fill_surface(const Surface *surface, const FillParams &params) const override;

protected:
    // The grid fill will keep the angle constant between the layers, see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t idx) const override { return 0.f; }
};

class FillRectilinear2Peri : public FillRectilinear2
{
public:

    virtual Fill* clone() const override { return new FillRectilinear2Peri(*this); };
    virtual ~FillRectilinear2Peri() = default;
    //virtual Polylines fill_surface(const Surface *surface, const FillParams &params);
    virtual void fill_surface_extrusion(const Surface *surface, const FillParams &params, ExtrusionEntitiesPtr &out) const override;

};


class FillScatteredRectilinear : public FillRectilinear2
{
public:
    virtual Fill* clone() const override{ return new FillScatteredRectilinear(*this); };
    virtual ~FillScatteredRectilinear() = default;
    virtual Polylines fill_surface(const Surface *surface, const FillParams &params) const override;

protected:
    virtual float _layer_angle(size_t idx) const override;
    virtual std::vector<SegmentedIntersectionLine> _vert_lines_for_polygon(const ExPolygonWithOffset &poly_with_offset, const BoundingBox &bounding_box, const FillParams &params, coord_t line_spacing) const override;
    virtual coord_t _line_spacing_for_density(float density) const override;
};

class FillRectilinearSawtooth : public FillRectilinear2 {
public:

    virtual Fill* clone() const override { return new FillRectilinearSawtooth(*this); };
    virtual ~FillRectilinearSawtooth() = default;
    virtual void fill_surface_extrusion(const Surface *surface, const FillParams &params, ExtrusionEntitiesPtr &out) const override;

};

class FillRectilinear2WGapFill : public FillRectilinear2
{
public:

    virtual Fill* clone() const { return new FillRectilinear2WGapFill(*this); };
    virtual ~FillRectilinear2WGapFill() = default;
    virtual void fill_surface_extrusion(const Surface *surface, const FillParams &params, ExtrusionEntitiesPtr &out) const override;
    static void split_polygon_gap_fill(const Surface &surface, const FillParams &params, ExPolygons &rectilinear, ExPolygons &gapfill);
};


}; // namespace Slic3r

#endif // slic3r_FillRectilinear2_hpp_
