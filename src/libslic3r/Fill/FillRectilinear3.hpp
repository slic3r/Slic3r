#ifndef slic3r_FillRectilinear3_hpp_
#define slic3r_FillRectilinear3_hpp_

#include "../libslic3r.h"

#include "FillBase.hpp"

namespace Slic3r {

class Surface;

class FillRectilinear3 : public Fill
{
public:
    virtual Fill* clone() const override { return new FillRectilinear3(*this); };
    virtual ~FillRectilinear3() {}
    virtual Polylines fill_surface(const Surface *surface, const FillParams &params) const override;

    struct FillDirParams
    {
        FillDirParams(coordf_t spacing, double angle, coordf_t pattern_shift = 0.f) : 
            spacing(spacing), angle(angle), pattern_shift(pattern_shift) {}
        coordf_t    spacing;
        double      angle;
        coordf_t    pattern_shift;
    };

protected:
	bool fill_surface_by_lines(const Surface *surface, const FillParams &params, std::vector<FillDirParams> &fill_dir_params, Polylines &polylines_out) const;
};

class FillGrid3 : public FillRectilinear3
{
public:
    virtual Fill* clone() const override { return new FillGrid3(*this); };
    virtual ~FillGrid3() {}
    virtual Polylines fill_surface(const Surface *surface, const FillParams &params) const override;

protected:
	// The grid fill will keep the angle constant between the layers, see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t /* idx */) const override { return 0.f; }
};

class FillTriangles3 : public FillRectilinear3
{
public:
    virtual Fill* clone() const override { return new FillTriangles3(*this); };
    virtual ~FillTriangles3() {}
    virtual Polylines fill_surface(const Surface *surface, const FillParams &params) const override;

protected:
	// The grid fill will keep the angle constant between the layers, see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t /* idx */) const override { return 0.f; }
};

class FillStars3 : public FillRectilinear3
{
public:
    virtual Fill* clone() const override { return new FillStars3(*this); };
    virtual ~FillStars3() {}
    virtual Polylines fill_surface(const Surface *surface, const FillParams &params) const override;

protected:
    // The grid fill will keep the angle constant between the layers, see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t /* idx */) const override { return 0.f; }
};

class FillCubic3 : public FillRectilinear3
{
public:
    virtual Fill* clone() const override { return new FillCubic3(*this); };
    virtual ~FillCubic3() {}
    virtual Polylines fill_surface(const Surface *surface, const FillParams &params)const override;

protected:
	// The grid fill will keep the angle constant between the layers, see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t /* idx */) const override { return 0.f; }
};


}; // namespace Slic3r

#endif // slic3r_FillRectilinear3_hpp_
