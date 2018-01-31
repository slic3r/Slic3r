/* -*- mode: c++; c-basic-offset: 4; -*- */
/************************************************************************
 * Copyright 2009 University Corporation for Atmospheric Research.
 * All rights reserved.
 *
 * Use of this code is subject to the standard BSD license:
 *
 *  http://www.opensource.org/licenses/bsd-license.html
 *
 * See the COPYRIGHT file in the source distribution for the license text,
 * or see this web page:
 *
 *  http://www.eol.ucar.edu/homes/granger/bspline/doc/
 *
 *************************************************************************/

#ifndef BSPLINE_H
#define BSPLINE_H

#include <vector>


/*
 * Warning: original BSplineBase.h/cpp merged into BSpline.h/cpp to avoid dependency issues caused by Build::WithXSpp which tries to compile all .cpp files in /src
 * Original BSplineBase.h starts here!!!
 */



#ifndef _BSPLINEBASE_IFACE_ID
#define _BSPLINEBASE_IFACE_ID "$Id: BSpline.h 6353 2008-05-05 19:30:48Z martinc $"
#endif

/**
 * @file
 *
 * This file defines the BSpline library interface.
 *
 */
template <class T> class BSpline;

/*
 * Opaque member structure to hide the matrix implementation.
 */
template <class T> struct BSplineBaseP;

/**
 * @class BSplineBase
 *
 * The base class for a spline object containing the nodes for a given
 * domain, cutoff wavelength, and boundary condition.

 * To smooth a single curve, the BSpline interface contains a constructor
 * which both sets up the domain and solves for the spline.  Subsequent
 * curves over the same domain can be created by apply()ing them to the
 * BSpline object, where apply() is a BSplineBase method.  [See apply().]
 * New curves can also be smoothed within the same BSpline object by
 * calling solve() with the new set of y values.  [See BSpline.]  A
 * BSplineBase can be created on its own, in which case all of the
 * computations dependent on the x values, boundary conditions, and cutoff
 * wavelength have already been completed.
 *
 * The solution of the cubic b-spline is divided into two parts.  The first
 * is the setup of the domain given the x values, boundary conditions, and
 * wavelength.  The second is the solution of the spline for a set of y
 * values corresponding to the x values in the domain.  The first part is
 * done in the creation of the BSplineBase object (or when calling the
 * setDomain method).  The second part is done when creating a BSpline
 * object (or calling solve() on a BSpline object).
 *
 * A BSpline object can be created with either one of its constructors, or
 * by calling apply() on an existing BSplineBase object.  Once a spline has
 * been solved, it can be evaluated at any x value.  The following example
 * creates a spline curve and evaluates it over the domain:
 *
@verbatim

    vector<float> x;
    vector<float> y;
    { ... }
    int bc = BSplineBase<float>::BC_ZERO_SECOND;
    BSpline<float>::Debug = true;
    BSpline<float> spline (x.begin(), x.size(), y.begin(), wl, bc);
    if (spline.ok())
    {
        ostream_iterator<float> of(cout, "\t ");
        float xi = spline.Xmin();
    float xs = (spline.Xmax() - xi) / 2000.0;
    for (; xi <= spline.Xmax(); xi += xs)
    {
        *of++ = spline.evaluate (xi);
    }
    }

@endverbatim
 *
 * In the usual usage, the BSplineBase can compute a reasonable number of
 * nodes for the spline, balancing between a few desirable factors.  There
 * needs to be at least 2 nodes per cutoff wavelength (preferably 4 or
 * more) for the derivative constraint to reliably approximate a lo-pass
 * filter.  There should be at least 1 and preferably about 2 data points
 * per node (measured just by their number and not by any check of the
 * density of points across the domain).  Lastly, of course, the fewer the
 * nodes then the faster the computation of the spline.  The computation of
 * the number of nodes happens in the Setup() method during BSplineBase
 * construction and when setDomain() is called.  If the setup fails to find
 * a desirable number of nodes, then the BSplineBase object will return
 * false from ok().
 *
 * The ok() method returns false when a BSplineBase or BSpline could not
 * complete any operation successfully.  In particular, as mentioned above,
 * ok() will return false if some problem was detected with the domain
 * values or if no reasonable number of nodes could be found for the given
 * cutoff wavelength.  Also, ok() on a BSpline object will return false if
 * the matrix equation could not be solved, such as after BSpline
 * construction or after a call to apply().
 *
 * If letting Setup() determine the number of nodes is not acceptable, the
 * constructors and setDomain() accept the parameter num_nodes.  By
 * default, num_nodes is passed as zero, forcing Setup() to calculate the
 * number of nodes.  However, if num_nodes is passed as 2 or greater, then
 * Setup() will bypass its own algorithm and accept the given number of
 * nodes instead.  Obviously, it's up to the programmer to understand the
 * affects of the number of nodes on the representation of the data and on
 * the solution (or non-solution) of the spline.  Remember to check the
 * ok() method to detect when the spline solution has failed.
 *
 * The interface for the BSplineBase and BSpline templates is defined in
 * the header file BSpline.h.  The implementation is defined in BSpline.cpp.
 * Source files which will instantiate the template should include the
 * implementation file and @em not the interface.  If the implementation
 * for a specific type will be linked from elsewhere, such as a
 * static library or Windows DLL, source files should only include the
 * interface file.  On Windows, applications should link with the import
 * library BSpline.lib and make sure BSpline.dll is on the path.  The DLL
 * contains an implementation for BSpline<float> and BSpline<double>.
 * For debugging, an application can include the implementation to get its
 * own instantiation.
 *
 * The algorithm is based on the cubic spline described by Katsuyuki Ooyama
 * in Montly Weather Review, Vol 115, October 1987.  This implementation
 * has benefited from comparisons with a previous FORTRAN implementation by
 * James L. Franklin, NOAA/Hurricane Research Division.  In particular, the
 * algorithm in the Setup() method is based mostly on his implementation
 * (VICSETUP).  The Setup() method finds a suitable default for the number
 * of nodes given a domain and cutoff frequency.  This implementation
 * adopts most of the same constraints, including a constraint that the
 * cutoff wavelength not be greater than the span of the domain values: wl
 * < max(x) - min(x). If this is not an acceptable constraint, then use the
 * num_nodes parameter to specify the number of nodes explicitly.
 *
 * The cubic b-spline is formulated as the sum of some multiple of the
 * basis function centered at each node in the domain.  The number of nodes
 * is determined by the desired cutoff wavelength and a desirable number of
 * x values per node.  The basis function is continuous and differentiable
 * up to the second degree.  A derivative constraint is included in the
 * solution to achieve the effect of a low-pass frequency filter with the
 * given cutoff wavelength.  The derivative constraint can be disabled by
 * specifying a wavelength value of zero, which reduces the analysis to a
 * least squares fit to a cubic b-spline.  The domain nodes, boundary
 * constraints, and wavelength determine a linear system of equations,
 * Qa=b, where a is the vector of basis function coefficients at each node.
 * The coefficient vector is solved by first LU factoring along the
 * diagonally banded matrix Q in BSplineBase.  The BSpline object then
 * computes the B vector for a set of y values and solves for the
 * coefficient vector with the LU matrix.  Only the diagonal bands are
 * stored in memory and calculated during LU factoring and back
 * substitution, and the basis function is evaluated as few times as
 * possible in computing the diagonal matrix and B vector.
 *
 * @author Gary Granger (http://www.eol.ucar.edu/homes/granger)
 *
@verbatim
Copyright (c) 1998-2009
University Corporation for Atmospheric Research, UCAR
All rights reserved.
@endverbatim
 **/

template <class T>
class BSplineBase
{
public:
    // Datum type
    typedef T datum_type;

    /// Return a string describing the implementation version.
    static const char *ImplVersion();

    /// Return a string describing the interface version.
    static const char *IfaceVersion();

    /**
     * Call this class method with a value greater than zero to enable
     * debug messages, or with zero to disable messages.  Calling with
     * no arguments returns true if debugging enabled, else false.
     */
    static bool Debug (int on = -1);

    /**
     * Boundary condition types.
     */
    enum BoundaryConditionTypes
    {
    /// Set the endpoints of the spline to zero.
    BC_ZERO_ENDPOINTS = 0,
    /// Set the first derivative of the spline to zero at the endpoints.
    BC_ZERO_FIRST = 1,
    /// Set the second derivative to zero.
    BC_ZERO_SECOND = 2
    };

public:

    /**
     * Construct a spline domain for the given set of x values, cutoff
     * wavelength, and boundary condition type.  The parameters are the
     * same as for setDomain().  Call ok() to check whether domain
     * setup succeeded after construction.
     */
    BSplineBase (const T *x, int nx,
         double wl, int bc_type = BC_ZERO_SECOND,
         int num_nodes = 0);

    /// Copy constructor
    BSplineBase (const BSplineBase &);

    /**
     * Change the domain of this base.  [If this is part of a BSpline
     * object, this method {\em will not} change the existing curve or
     * re-apply the smoothing to any set of y values.]
     *
     * The x values can be in any order, but they must be of sufficient
     * density to support the requested cutoff wavelength.  The setup of
     * the domain may fail because of either inconsistency between the x
     * density and the cutoff wavelength, or because the resulting matrix
     * could not be factored.  If setup fails, the method returns false.
     *
     * @param x     The array of x values in the domain.
     * @param nx    The number of values in the @p x array.
     * @param wl    The cutoff wavelength, in the same units as the
     *          @p x values.  A wavelength of zero disables
     *          the derivative constraint.
     * @param bc_type   The enumerated boundary condition type.  If
     *          omitted it defaults to BC_ZERO_SECOND.
     * @param num_nodes The number of nodes to use for the cubic b-spline.
     *          If less than 2 a reasonable number will be
     *          calculated automatically, if possible, taking
     *          into account the given cutoff wavelength.
     *
     * @see ok().
     */
    bool setDomain (const T *x, int nx, double wl,
            int bc_type = BC_ZERO_SECOND,
            int num_nodes = 0);

    /**
     * Create a BSpline smoothed curve for the given set of NX y values.
     * The returned object will need to be deleted by the caller.
     * @param y The array of y values corresponding to each of the nX()
     *      x values in the domain.
     * @see ok()
     */
    BSpline<T> *apply (const T *y);

    /**
     * Return array of the node coordinates.  Returns 0 if not ok().  The
     * array of nodes returned by nodes() belongs to the object and should
     * not be deleted; it will also be invalid if the object is destroyed.
     */
    const T *nodes (int *nnodes);

    /**
     * Return the number of nodes (one more than the number of intervals).
     */
    int nNodes () { return M+1; }

    /**
     * Number of original x values.
     */
    int nX () { return NX; }

    /// Minimum x value found.
    T Xmin () { return xmin; }

    /// Maximum x value found.
    T Xmax () { return xmin + (M * DX); }

    /**
     * Return the Alpha value for a given wavelength.  Note that this
     * depends on the current node interval length (DX).
     */
    double Alpha (double wavelength);

    /**
     * Return alpha currently in use by this domain.
     */
    double Alpha () { return alpha; }

    /**
     * Return the current state of the object, either ok or not ok.
     * Use this method to test for valid state after construction or after
     * a call to setDomain().  ok() will return false if either fail, such
     * as when an appropriate number of nodes and node interval cannot be
     * found for a given wavelength, or when the linear equation for the
     * coefficients cannot be solved.
     */
    bool ok () { return OK; }

    virtual ~BSplineBase();

protected:

    typedef BSplineBaseP<T> Base;

    // Provided
    double waveLength;  // Cutoff wavelength (l sub c)
    int NX;
    int K;  // Degree of derivative constraint (currently fixed at 2)
    int BC;         // Boundary conditions type (0,1,2)

    // Derived
    T xmax;
    T xmin;
    int M;          // Number of intervals (M+1 nodes)
    double DX;          // Interval length in same units as X
    double alpha;
    bool OK;
    Base *base;         // Hide more complicated state members
                    // from the public interface.

    bool Setup (int num_nodes = 0);
    void calculateQ ();
    double qDelta (int m1, int m2);
    double Beta (int m);
    void addP ();
    bool factor ();
    double Basis (int m, T x);
    double DBasis (int m, T x);

    static const double BoundaryConditions[3][4];
    static const double BS_PI;

    double Ratiod (int&, double &, double &);
};



/*
 * Original BSpline.h start here
 */



template <class T> struct BSplineP;


/**
 * Used to evaluate a BSpline.
 * Inherits the BSplineBase domain information and interface and adds
 * smoothing.  See the BSplineBase documentation for a summary of the
 * BSpline interface.
 */
template <class T>
class BSpline : public BSplineBase<T>
{
public:
    /**
     * Create a single spline with the parameters required to set up
     * the domain and subsequently smooth the given set of y values.
     * The y values must correspond to each of the values in the x array.
     * If either the domain setup fails or the spline cannot be solved,
     * the state will be set to not ok.
     *
     * @see ok().
     *
     * @param x		The array of x values in the domain.
     * @param nx	The number of values in the @p x array.
     * @param y		The array of y values corresponding to each of the
     *			nX() x values in the domain.
     * @param wl	The cutoff wavelength, in the same units as the
     *			@p x values.  A wavelength of zero disables
     *			the derivative constraint.
     * @param bc_type	The enumerated boundary condition type.  If
     *			omitted it defaults to BC_ZERO_SECOND.
     * @param num_nodes The number of nodes to use for the cubic b-spline.
     *			If less than 2 a "reasonable" number will be
     *			calculated automatically, taking into account
     *			the given cutoff wavelength.
     */
    BSpline (const T *x, int nx, 		/* independent variable */
	     const T *y,			/* dependent values @ ea X */
	     double wl,				/* cutoff wavelength */
	     int bc_type = BSplineBase<T>::BC_ZERO_SECOND,
	     int num_nodes = 0);

    /**
     * A BSpline curve can be derived from a separate @p base and a set
     * of data points @p y over that base.
     */
    BSpline (BSplineBase<T> &base, const T *y);

    /**
     * Solve the spline curve for a new set of y values.  Returns false
     * if the solution fails.
     *
     * @param y The array of y values corresponding to each of the nX()
     *		x values in the domain.
     */
    bool solve (const T *y);

    /**
     * Return the evaluation of the smoothed curve
     * at a particular @p x value.  If current state is not ok(), returns 0.
     */
    T evaluate (T x);

    /**
     * Return the first derivative of the spline curve at the given @p x.
     * Returns zero if the current state is not ok().
     */
    T slope (T x);

    /**
     * Return the @p n-th basis coefficient, from 0 to M.  If the current
     * state is not ok(), or @p n is out of range, the method returns zero.
     */
    T coefficient (int n);

    virtual ~BSpline();

    using BSplineBase<T>::Debug;
    using BSplineBase<T>::Basis;
    using BSplineBase<T>::DBasis;

protected:

    using BSplineBase<T>::OK;
    using BSplineBase<T>::M;
    using BSplineBase<T>::NX;
    using BSplineBase<T>::DX;
    using BSplineBase<T>::base;
    using BSplineBase<T>::xmin;
    using BSplineBase<T>::xmax;

    // Our hidden state structure
    BSplineP<T> *s;
    T mean;			// Fit without mean and add it in later

};

#endif 
