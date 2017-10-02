// -*- mode: c++; c-basic-offset: 4; -*-
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

/**
 * @file
 *
 * This file defines the implementation for the BSpline and BSplineBase
 * templates.
 **/
#include "BSpline.h"
#include "BandedMatrix.h"

#include <vector>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <iomanip>
#include <map>
#include <assert.h>





/*
 * Original BSplineBase.cpp start here
 */

/*
 * This class simulates a namespace for private symbols used by this template
 * implementation which should not pollute the global namespace.
 */
class my
{
    public:
        template<class T> static inline
        T abs(const T t)
        {
            return (t < 0) ? -t : t;
        }

        template<class T> static inline
        const T& min(const T& a,
                     const T& b)
        {
            return (a < b) ? a : b;
        }

        template<class T> static inline
        const T& max(const T& a,
                     const T& b)
        {
            return (a > b) ? a : b;
        }
};

//////////////////////////////////////////////////////////////////////
template<class T> class Matrix : public BandedMatrix<T>
{
    public:
        Matrix &operator +=(const Matrix &B)
        {
            Matrix &A = *this;
            typename Matrix::size_type M = A.num_rows();
            typename Matrix::size_type N = A.num_cols();

            assert(M==B.num_rows());
            assert(N==B.num_cols());

            typename Matrix::size_type i, j;
            for (i=0; i<M; i++)
                for (j=0; j<N; j++)
                    A[i][j] += B[i][j];
            return A;
        }

        inline Matrix & operator=(const Matrix &b)
        {
            return Copy(*this, b);
        }

        inline Matrix & operator=(const T &e)
        {
            BandedMatrix<T>::operator= (e);
            return *this;
        }

    };
    //////////////////////////////////////////////////////////////////////
    // Our private state structure, which hides our use of some matrix
    // template classes.

template<class T> struct BSplineBaseP
{
        typedef Matrix<T> MatrixT;

        MatrixT Q; // Holds P+Q and its factorization
        std::vector<T> X;
        std::vector<T> Nodes;
};

//////////////////////////////////////////////////////////////////////

// This array contains the beta parameter for the boundary condition
// constraints.  The boundary condition type--0, 1, or 2--is the first
// index into the array, followed by the index of the endpoints.  See the
// Beta() method.
template<class T> const double BSplineBase<T>::BoundaryConditions[3][4] =
    {
            //  0       1       M-1     M
                {
                        -4,
                        -1,
                        -1,
                        -4 },
                {
                        0,
                        1,
                        1,
                        0 },
                {
                        2,
                        -1,
                        -1,
                        2 } };
//////////////////////////////////////////////////////////////////////
template<class T> inline bool BSplineBase<T>::Debug(int on)
{
    static bool debug = false;
    if (on >= 0)
        debug = (on > 0);
    return debug;
}

//////////////////////////////////////////////////////////////////////
template<class T> const double BSplineBase<T>::BS_PI = 3.1415927;
//////////////////////////////////////////////////////////////////////
template<class T> const char * BSplineBase<T>::ImplVersion()
{
    return ("$Id: BSpline.cpp 6352 2008-05-05 04:40:39Z martinc $");
}

//////////////////////////////////////////////////////////////////////
template<class T> const char * BSplineBase<T>::IfaceVersion()
{
    return (_BSPLINEBASE_IFACE_ID);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

template<class T> BSplineBase<T>::~BSplineBase()
{
    delete base;
}

// This is a member-wise copy except for replacing our
// private base structure with the source's, rather than just copying
// the pointer.  But we use the compiler's default copy constructor for
// constructing our BSplineBaseP.
template<class T> BSplineBase<T>::BSplineBase(const BSplineBase<T> &bb) :
    K(bb.K), BC(bb.BC), OK(bb.OK), base(new BSplineBaseP<T>(*bb.base))
{
    xmin = bb.xmin;
    xmax = bb.xmax;
    alpha = bb.alpha;
    waveLength = bb.waveLength;
    DX = bb.DX;
    M = bb.M;
    NX = base->X.size();
}
//////////////////////////////////////////////////////////////////////
template<class T> BSplineBase<T>::BSplineBase(const T *x,
                                              int nx,
                                              double wl,
                                              int bc,
                                              int num_nodes) :
    NX(0), K(2), OK(false), base(new BSplineBaseP<T>)
{
    setDomain(x, nx, wl, bc, num_nodes);
}

//////////////////////////////////////////////////////////////////////
// Methods
template<class T> bool BSplineBase<T>::setDomain(const T *x,
                                                 int nx,
                                                 double wl,
                                                 int bc,
                                                 int num_nodes)
{
    if ((nx <= 0) || (x == 0) || (wl< 0) || (bc< 0) || (bc> 2)) {
        return false;
    }
    OK = false;
    waveLength = wl;
    BC = bc;
    // Copy the x array into our storage.
    base->X.resize(nx);
    std::copy(x, x+nx, base->X.begin());
    NX = base->X.size();

    // The Setup() method determines the number and size of node intervals.
    if (Setup(num_nodes)) {
        if (Debug()) {
            std::cerr << "Using M node intervals: " << M << " of length DX: "
                    << DX << std::endl;
            std::cerr << "X min: " << xmin << " ; X max: " << xmax << std::endl;
            std::cerr << "Data points per interval: " << (float)NX/(float)M
                    << std::endl;
            std::cerr << "Nodes per wavelength: " << (float)waveLength
                    /(float)DX << std::endl;
            std::cerr << "Derivative constraint degree: " << K << std::endl;
        }

        // Now we can calculate alpha and our Q matrix
        alpha = Alpha(waveLength);
        if (Debug()) {
            std::cerr << "Cutoff wavelength: " << waveLength << " ; "
                    << "Alpha: " << alpha << std::endl;
            std::cerr << "Calculating Q..." << std::endl;
        }
        calculateQ();
        if (Debug() && M < 30) {
            std::cerr.fill(' ');
            std::cerr.precision(2);
            std::cerr.width(5);
            std::cerr << base->Q << std::endl;
        }

        if (Debug())
            std::cerr << "Calculating P..." << std::endl;
        addP();
        if (Debug()) {
            std::cerr << "Done." << std::endl;
            if (M < 30) {
                std::cerr << "Array Q after addition of P." << std::endl;
                std::cerr << base->Q;
            }
        }

        // Now perform the LU factorization on Q
        if (Debug())
            std::cerr << "Beginning LU factoring of P+Q..." << std::endl;
        if (!factor()) {
            if (Debug())
                std::cerr << "Factoring failed." << std::endl;
        } else {
            if (Debug())
                std::cerr << "Done." << std::endl;
            OK = true;
        }
    }
    return OK;
}
//////////////////////////////////////////////////////////////////////
/*
 * Calculate the alpha parameter given a wavelength.
 */
template<class T> double BSplineBase<T>::Alpha(double wl)
{
    // K is the degree of the derivative constraint: 1, 2, or 3
    double a = (double) (wl / (2 * BS_PI * DX));
    a *= a; // a^2
    if (K == 2)
        a = a * a; // a^4
    else if (K == 3)
        a = a * a * a; // a^6
    return a;
}
//////////////////////////////////////////////////////////////////////
/*
 * Return the correct beta value given the node index.  The value depends
 * on the node index and the current boundary condition type.
 */
template<class T> inline double BSplineBase<T>::Beta(int m)
{
    if (m > 1 && m < M-1)
        return 0.0;
    if (m >= M-1)
        m -= M-3;
    assert(0 <= BC && BC <= 2);
    assert(0 <= m && m <= 3);
    return BoundaryConditions[BC][m];
}
//////////////////////////////////////////////////////////////////////
/*
 * Given an array of y data points defined over the domain
 * of x data points in this BSplineBase, create a BSpline
 * object which contains the smoothed curve for the y array.
 */
template<class T> BSpline<T> * BSplineBase<T>::apply(const T *y)
{
    return new BSpline<T> (*this, y);
}
//////////////////////////////////////////////////////////////////////
/*
 * Evaluate the closed basis function at node m for value x,
 * using the parameters for the current boundary conditions.
 */
template<class T> double BSplineBase<T>::Basis(int m,
                                               T x)
{
    double y = 0;
    double xm = xmin + (m * DX);
    double z = my::abs((double)(x - xm) / (double)DX);
    if (z < 2.0) {
        z = 2 - z;
        y = 0.25 * (z*z*z);
        z -= 1.0;
        if (z > 0)
            y -= (z*z*z);
    }

    // Boundary conditions, if any, are an additional addend.
    if (m == 0 || m == 1)
        y += Beta(m) * Basis(-1, x);
    else if (m == M-1 || m == M)
        y += Beta(m) * Basis(M+1, x);

    return y;
}
//////////////////////////////////////////////////////////////////////
/*
 * Evaluate the deriviative of the closed basis function at node m for
 * value x, using the parameters for the current boundary conditions.
 */
template<class T> double BSplineBase<T>::DBasis(int m,
                                                T x)
{
    double dy = 0;
    double xm = xmin + (m * DX);
    double delta = (double)(x - xm) / (double)DX;
    double z = my::abs(delta);
    if (z < 2.0) {
        z = 2.0 - z;
        dy = 0.25 * z * z;
        z -= 1.0;

        if (z > 0) {
            dy -= z * z;
        }
        dy *= ((delta > 0) ? -1.0 : 1.0) * 3.0 / DX;
    }

    // Boundary conditions, if any, are an additional addend.
    if (m == 0 || m == 1)
        dy += Beta(m) * DBasis(-1, x);
    else if (m == M-1 || m == M)
        dy += Beta(m) * DBasis(M+1, x);

    return dy;
}
//////////////////////////////////////////////////////////////////////
template<class T> double BSplineBase<T>::qDelta(int m1,
                                                int m2)
/*
 * Return the integral of the product of the basis function derivative
 * restricted to the node domain, 0 to M.
 */
{
    // These are the products of the Kth derivative of the
    // normalized basis functions
    // given a distance m nodes apart, qparts[K-1][m], 0 <= m <= 3
    // Each column is the integral over each unit domain, -2 to 2
    static const double qparts[3][4][4] =
        {
                    {
                                {
                                        0.11250,
                                        0.63750,
                                        0.63750,
                                        0.11250 },
                                {
                                        0.00000,
                                        0.13125,
                                        -0.54375,
                                        0.13125 },
                                {
                                        0.00000,
                                        0.00000,
                                        -0.22500,
                                        -0.22500 },
                                {
                                        0.00000,
                                        0.00000,
                                        0.00000,
                                        -0.01875 } },
                    {
                                {
                                        0.75000,
                                        2.25000,
                                        2.25000,
                                        0.75000 },
                                {
                                        0.00000,
                                        -1.12500,
                                        -1.12500,
                                        -1.12500 },
                                {
                                        0.00000,
                                        0.00000,
                                        0.00000,
                                        0.00000 },
                                {
                                        0.00000,
                                        0.00000,
                                        0.00000,
                                        0.37500 } },
                    {
                                {
                                        2.25000,
                                        20.25000,
                                        20.25000,
                                        2.25000 },
                                {
                                        0.00000,
                                        -6.75000,
                                        -20.25000,
                                        -6.75000 },
                                {
                                        0.00000,
                                        0.00000,
                                        6.75000,
                                        6.75000 },
                                {
                                        0.00000,
                                        0.00000,
                                        0.00000,
                                        -2.25000 } } };

    if (m1 > m2)
        std::swap(m1, m2);

    if (m2 - m1 > 3)
        return 0.0;

    double q = 0;
    for (int m = my::max(m1-2, 0); m < my::min(m1+2, M); ++m)
        q += qparts[K-1][m2-m1][m-m1+2];
    return q * alpha;
}
//////////////////////////////////////////////////////////////////////
template<class T> void BSplineBase<T>::calculateQ()
{
    Matrix<T> &Q = base->Q;
    Q.setup(M+1, 3);
    Q = 0;
    if (alpha == 0)
        return;

    // First fill in the q values without the boundary constraints.
    int i;
    for (i = 0; i <= M; ++i) {
        Q[i][i] = qDelta(i, i);
        for (int j = 1; j < 4 && i+j <= M; ++j) {
            Q[i][i+j] = Q[i+j][i] = qDelta(i, i+j);
        }
    }

    // Now add the boundary constraints:
    // First the upper left corner.
    float b1, b2, q;
    for (i = 0; i <= 1; ++i) {
        b1 = Beta(i);
        for (int j = i; j < i+4; ++j) {
            b2 = Beta(j);
            assert(j-i >= 0 && j - i < 4);
            q = 0.0;
            if (i+1 < 4)
                q += b2*qDelta(-1, i);
            if (j+1 < 4)
                q += b1*qDelta(-1, j);
            q += b1*b2*qDelta(-1, -1);
            Q[j][i] = (Q[i][j] += q);
        }
    }

    // Then the lower right
    for (i = M-1; i <= M; ++i) {
        b1 = Beta(i);
        for (int j = i - 3; j <= i; ++j) {
            b2 = Beta(j);
            q = 0.0;
            if (M+1-i < 4)
                q += b2*qDelta(i, M+1);
            if (M+1-j < 4)
                q += b1*qDelta(j, M+1);
            q += b1*b2*qDelta(M+1, M+1);
            Q[j][i] = (Q[i][j] += q);
        }
    }
}
//////////////////////////////////////////////////////////////////////
template<class T> void BSplineBase<T>::addP()
{
    // Add directly to Q's elements
    Matrix<T> &P = base->Q;
    std::vector<T> &X = base->X;

    // For each data point, sum the product of the nearest, non-zero Basis
    // nodes
    int mx, m, n, i;
    for (i = 0; i < NX; ++i) {
        // Which node does this put us in?
        T &x = X[i];
        mx = (int)((x - xmin) / DX);

        // Loop over the upper triangle of nonzero basis functions,
        // and add in the products on each side of the diagonal.
        for (m = my::max(0, mx-1); m <= my::min(M, mx+2); ++m) {
            float pn;
            float pm = Basis(m, x);
            float sum = pm * pm;
            P[m][m] += sum;
            for (n = m+1; n <= my::min(M, mx+2); ++n) {
                pn = Basis(n, x);
                sum = pm * pn;
                P[m][n] += sum;
                P[n][m] += sum;
            }
        }
    }
}
//////////////////////////////////////////////////////////////////////
template<class T> bool BSplineBase<T>::factor()
{
    Matrix<T> &LU = base->Q;

    if (LU_factor_banded(LU, 3) != 0) {
        if (Debug())
            std::cerr << "LU_factor_banded() failed." << std::endl;
        return false;
    }
    if (Debug() && M < 30)
        std::cerr << "LU decomposition: " << std::endl << LU << std::endl;
    return true;
}
//////////////////////////////////////////////////////////////////////
template<class T> inline double BSplineBase<T>::Ratiod(int &ni,
                                                       double &deltax,
                                                       double &ratiof)
{
    deltax = (xmax - xmin) / ni;
    ratiof = waveLength / deltax;
    double ratiod = (double) NX / (double) (ni + 1);
    return ratiod;
}
//////////////////////////////////////////////////////////////////////
// Setup the number of nodes (and hence deltax) for the given domain and
// cutoff wavelength.  According to Ooyama, the derivative constraint
// approximates a lo-pass filter if the cutoff wavelength is about 4*deltax
// or more, but it should at least be 2*deltax.  We can increase the number
// of nodes to increase the number of nodes per cutoff wavelength.
// However, to get a reasonable representation of the data, the setup
// enforces at least as many nodes as data points in the domain.  (This
// constraint assumes reasonably even distribution of data points, since
// its really the density of data points which matters.)
//
// Return zero if the setup fails, non-zero otherwise.
//
// The algorithm in this routine is mostly taken from the FORTRAN
// implementation by James Franklin, NOAA/HRD.
//
template<class T> bool BSplineBase<T>::Setup(int num_nodes)
{
    std::vector<T> &X = base->X;

    // Find the min and max of the x domain
    xmin = X[0];
    xmax = X[0];

    int i;
    for (i = 1; i < NX; ++i) {
        if (X[i] < xmin)
            xmin = X[i];
        else if (X[i] > xmax)
            xmax = X[i];
    }
    if (Debug())
	std::cerr << "Xmax=" << xmax << ", Xmin=" << xmin << std::endl;

    // Number of node intervals (number of spline nodes - 1).
    int ni;
    double deltax;

    if (num_nodes >= 2) {
        // We've been told explicitly the number of nodes to use.
        ni = num_nodes - 1;
        if (waveLength == 0) {
            waveLength = 1.0;
        }
	if (Debug())
	{
	    std::cerr << "Num nodes explicitly given as " << num_nodes
		      << ", wavelength set to " << waveLength << std::endl;
	}
    } else if (waveLength == 0) {
        // Turn off frequency constraint and just set two node intervals per
        // data point.
        ni = NX * 2;
        waveLength = 1;
	if (Debug())
	{
	    std::cerr << "Frequency constraint disabled, using 2 intervals "
		      << "per node: " << ni << " intervals, wavelength="
		      << waveLength << std::endl;
	}
    } else if (waveLength > xmax - xmin) {
	if (Debug())
	{
	    std::cerr << "Wavelength " << waveLength << " exceeds X span: "
		      << xmin << " - " << xmax << std::endl;
	}
        return (false);
    } else {
	if (Debug())
	{
	    std::cerr << "Searching for a reasonable number of "
		      << "intervals for wavelength " << waveLength
		      << " while keeping at least 2 intervals per "
		      << "wavelength ..." << std::endl;
	}
        // Minimum acceptable number of node intervals per cutoff wavelength.
        static const double fmin = 2.0;

	// Start out at a minimum number of intervals, meaning the maximum
	// number of points per interval, then work up to the maximum
	// number of intervals for which the intervals per wavelength is
	// still adequate.  I think the minimum must be more than 2 since
	// the basis function is evaluated on multiple nodes.
	ni = 5;

        double ratiof; // Nodes per wavelength for current deltax
        double ratiod; // Points per node interval

        // Increase the number of node intervals until we reach the minimum
        // number of intervals per cutoff wavelength, but only as long as
        // we can maintain at least one point per interval.
        do {
            if (Ratiod(++ni, deltax, ratiof) < 1.0)
	    {
		if (Debug())
		{
		    std::cerr << "At " << ni << " intervals, fewer than "
			      << "one point per interval, and "
			      << "intervals per wavelength is "
			      << ratiof << "." << std::endl;
		}
                return false;
	    }
        } while (ratiof < fmin);

        // Now increase the number of intervals until we have at least 4
        // intervals per cutoff wavelength, but only as long as we can
        // maintain at least 2 points per node interval.  There's also no
        // point to increasing the number of intervals if we already have
        // 15 or more nodes per cutoff wavelength.
        //
        do {
            if ((ratiod = Ratiod(++ni, deltax, ratiof)) < 1.0 || ratiof > 15.0) {
                --ni;
                break;
            }
        } while (ratiof < 4 || ratiod > 2.0);

	if (Debug())
	{
	    std::cerr << "Found " << ni << " intervals, "
		      << "length " << deltax << ", "
		      << ratiof << " nodes per wavelength " << waveLength
		      << ", "
		      << ratiod << " data points per interval." << std::endl;
	}
    }

    // Store the calculations in our state
    M = ni;
    DX = (xmax - xmin) / ni;

    return (true);
}
//////////////////////////////////////////////////////////////////////
template<class T> const T * BSplineBase<T>::nodes(int *nn)
{
    if (base->Nodes.size() == 0) {
        base->Nodes.reserve(M+1);
        for (int i = 0; i <= M; ++i) {
            base->Nodes.push_back(xmin + (i * DX));
        }
    }

    if (nn)
        *nn = base->Nodes.size();

    assert(base->Nodes.size() == (unsigned)(M+1));
    return &base->Nodes[0];
}
//////////////////////////////////////////////////////////////////////
template<class T> std::ostream &operator<<(std::ostream &out,
                                           const std::vector<T> &c)
{
    for (typename std::vector<T>::const_iterator it = c.begin(); it < c.end(); ++it)
        out << *it << ", ";
    out << std::endl;
    return out;
}





/*
 * Original BSpline.cpp start here
 */


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// BSpline Class
//////////////////////////////////////////////////////////////////////

template<class T> struct BSplineP {
        std::vector<T> spline;
        std::vector<T> A;
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

/*
 * This BSpline constructor constructs and sets up a new base and
 * solves for the spline curve coeffiecients all at once.
 */
template<class T> BSpline<T>::BSpline(const T *x,
                                      int nx,
                                      const T *y,
                                      double wl,
                                      int bc_type,
                                      int num_nodes) :
    BSplineBase<T>(x, nx, wl, bc_type, num_nodes), s(new BSplineP<T>) {
    solve(y);
}
//////////////////////////////////////////////////////////////////////
/*
 * Create a new spline given a BSplineBase.
 */
template<class T> BSpline<T>::BSpline(BSplineBase<T> &bb,
                                      const T *y) :
    BSplineBase<T>(bb), s(new BSplineP<T>) {
    solve(y);
}
//////////////////////////////////////////////////////////////////////
/*
 * (Re)calculate the spline for the given set of y values.
 */
template<class T> bool BSpline<T>::solve(const T *y) {
    if (!OK)
        return false;

    // Any previously calculated curve is now invalid.
    s->spline.clear();
    OK = false;

    // Given an array of data points over x and its precalculated
    // P+Q matrix, calculate the b vector and solve for the coefficients.
    std::vector<T> &B = s->A;
    std::vector<T> &A = s->A;
    A.clear();
    A.resize(M+1);

    if (Debug())
        std::cerr << "Solving for B..." << std::endl;

    // Find the mean of these data
    mean = 0.0;
    int i;
    for (i = 0; i < NX; ++i) {
        mean += y[i];
    }
    mean = mean / (double)NX;
    if (Debug())
        std::cerr << "Mean for y: " << mean << std::endl;

    int mx, m, j;
    for (j = 0; j < NX; ++j) {
        // Which node does this put us in?
        T &xj = base->X[j];
        T yj = y[j] - mean;
        mx = (int)((xj - xmin) / DX);

        for (m = my::max(0, mx-1); m <= my::min(mx+2, M); ++m) {
            B[m] += yj * Basis(m, xj);
        }
    }

    if (Debug() && M < 30) {
        std::cerr << "Solution a for (P+Q)a = b" << std::endl;
        std::cerr << " b: " << B << std::endl;
    }

    // Now solve for the A vector in place.
    if (LU_solve_banded(base->Q, A, 3) != 0) {
        if (Debug())
            std::cerr << "LU_solve_banded() failed." << std::endl;
    } else {
        OK = true;
        if (Debug())
            std::cerr << "Done." << std::endl;
        if (Debug() && M < 30) {
            std::cerr << " a: " << A << std::endl;
            std::cerr << "LU factor of (P+Q) = " << std::endl << base->Q
                    << std::endl;
        }
    }
    return (OK);
}
//////////////////////////////////////////////////////////////////////
template<class T> BSpline<T>::~BSpline() {
    delete s;
}
//////////////////////////////////////////////////////////////////////
template<class T> T BSpline<T>::coefficient(int n) {
    if (OK)
        if (0 <= n && n <= M)
            return s->A[n];
    return 0;
}
//////////////////////////////////////////////////////////////////////
template<class T> T BSpline<T>::evaluate(T x) {
    T y = 0;
    if (OK) {
        int n = (int)((x - xmin)/DX);
        for (int i = my::max(0, n-1); i <= my::min(M, n+2); ++i) {
            y += s->A[i] * Basis(i, x);
        }
        y += mean;
    }
    return y;
}
//////////////////////////////////////////////////////////////////////
template<class T> T BSpline<T>::slope(T x) {
    T dy = 0;
    if (OK) {
        int n = (int)((x - xmin)/DX);
        for (int i = my::max(0, n-1); i <= my::min(M, n+2); ++i) {
            dy += s->A[i] * DBasis(i, x);
        }
    }
    return dy;
}







/// Instantiate BSplineBase
template class  BSplineBase<double>;
//template class  BSplineBase<float>;

/// Instantiate BSpline
template class BSpline<double>;
//template class BSpline<float>;
