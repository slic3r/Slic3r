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

/**
 * Template for a diagonally banded matrix.
 **/
#ifndef _BANDEDMATRIX_ID
#define _BANDEDMATRIX_ID "$Id$"

#include <vector>
#include <algorithm>
#include <iostream>

template <class T> class BandedMatrixRow;


template <class T> class BandedMatrix
{
public:
    typedef unsigned int size_type;
    typedef T element_type;

    // Create a banded matrix with the same number of bands above and below
    // the diagonal.
    BandedMatrix (int N_ = 1, int nbands_off_diagonal = 0) : bands(0)
    {
	if (! setup (N_, nbands_off_diagonal))
	    setup ();
    }

    // Create a banded matrix by naming the first and last non-zero bands,
    // where the diagonal is at zero, and bands below the diagonal are
    // negative, bands above the diagonal are positive.
    BandedMatrix (int N_, int first, int last) : bands(0)
    {
	if (! setup (N_, first, last))
	    setup ();
    }

    // Copy constructor
    BandedMatrix (const BandedMatrix &b) : bands(0)
    {
	Copy (*this, b);
    }

    inline bool setup (int N_ = 1, int noff = 0)
    {
	return setup (N_, -noff, noff);
    }

    bool setup (int N_, int first, int last)
    {
	// Check our limits first and make sure they make sense.
	// Don't change anything until we know it will work.
	if (first > last || N_ <= 0)
	    return false;

	// Need at least as many N_ as columns and as rows in the bands.
	if (N_ < abs(first) || N_ < abs(last))
	    return false;

	top = last;
	bot = first;
	N = N_;
	out_of_bounds = T();

	// Finally setup the diagonal vectors
	nbands = last - first + 1;
	if (bands) delete[] bands;
	bands = new std::vector<T>[nbands];
	int i;
	for (i = 0; i < nbands; ++i)
	{
	    // The length of each array varies with its distance from the
	    // diagonal
	    int len = N - (abs(bot + i));
	    bands[i].clear ();
	    bands[i].resize (len);
	}
	return true;
    }

    BandedMatrix<T> & operator= (const BandedMatrix<T> &b) 
    {
	return Copy (*this, b);
    }

    BandedMatrix<T> & operator= (const T &e)
    {
	int i;
	for (i = 0; i < nbands; ++i)
	{
	    std::fill_n (bands[i].begin(), bands[i].size(), e);
	}
	out_of_bounds = e;
	return (*this);
    }

    ~BandedMatrix ()
    {
	if (bands)
	    delete[] bands;
    }

private:
    // Return false if coordinates are out of bounds
    inline bool check_bounds (int i, int j, int &v, int &e) const
    {
	v = (j - i) - bot;
	e = (i >= j) ? j : i;
	return !(v < 0 || v >= nbands || 
		 e < 0 || (unsigned int)e >= bands[v].size());
    }

    static BandedMatrix & Copy (BandedMatrix &a, const BandedMatrix &b)
    {
	if (a.bands) delete[] a.bands;
	a.top = b.top;
	a.bot = b.bot;
	a.N = b.N;
	a.out_of_bounds = b.out_of_bounds;
	a.nbands = a.top - a.bot + 1;
	a.bands = new std::vector<T>[a.nbands];
	int i;
	for (i = 0; i < a.nbands; ++i)
	{
	    a.bands[i] = b.bands[i];
	}
	return a;
    }

public:
    T &element (int i, int j)
    {
	int v, e;
	if (check_bounds(i, j, v, e))
	    return (bands[v][e]);
	else
	    return out_of_bounds;
    }

    const T &element (int i, int j) const
    {
	int v, e;
	if (check_bounds(i, j, v, e))
	    return (bands[v][e]);
	else
	    return out_of_bounds;
    }

    inline T & operator() (int i, int j) 
    {
	return element (i-1,j-1);
    }

    inline const T & operator() (int i, int j) const
    {
	return element (i-1,j-1);
    }

    size_type num_rows() const { return N; }

    size_type num_cols() const { return N; }

    const BandedMatrixRow<T> operator[] (int row) const
    {
	return BandedMatrixRow<T>(*this, row);
    }

    BandedMatrixRow<T> operator[] (int row)
    {
	return BandedMatrixRow<T>(*this, row);
    }


private:

    int top;
    int bot;
    int nbands;
    std::vector<T> *bands;
    int N;
    T out_of_bounds;

};


template <class T>
std::ostream &operator<< (std::ostream &out, const BandedMatrix<T> &m)
{
    unsigned int i, j;
    for (i = 0; i < m.num_rows(); ++i)
    {
	for (j = 0; j < m.num_cols(); ++j)
	{
	    out << m.element (i, j) << " ";
	}
	out << std::endl;
    }
    return out;
}



/*
 * Helper class for the intermediate in the [][] operation.
 */
template <class T> class BandedMatrixRow
{
public:
    BandedMatrixRow (const BandedMatrix<T> &_m, int _row) : bm(_m), i(_row)
    { }

    BandedMatrixRow (BandedMatrix<T> &_m, int _row) : bm(_m), i(_row)
    { }

    ~BandedMatrixRow () {}

    typename BandedMatrix<T>::element_type & operator[] (int j)
    {
	return const_cast<BandedMatrix<T> &>(bm).element (i, j);
    }

    const typename BandedMatrix<T>::element_type & operator[] (int j) const
    {
	return bm.element (i, j);
    }

private:
    const BandedMatrix<T> &bm;
    int i;
};


/*
 * Vector multiplication
 */

template <class Vector, class Matrix>
Vector operator* (const Matrix &m, const Vector &v)
{
    typename Matrix::size_type M = m.num_rows();
    typename Matrix::size_type N = m.num_cols();

    assert (N <= v.size());
    //if (M > v.size())
    //	return Vector();

    Vector r(N);
    for (unsigned int i = 0; i < M; ++i)
    {
	typename Matrix::element_type sum = 0;
	for (unsigned int j = 0; j < N; ++j)
	{
	    sum += m[i][j] * v[j];
	}
	r[i] = sum;
    }
    return r;
}



/*
 * LU factor a diagonally banded matrix using Crout's algorithm, but
 * limiting the trailing sub-matrix multiplication to the non-zero
 * elements in the diagonal bands.  Return nonzero if a problem occurs.
 */
template <class MT>
int LU_factor_banded (MT &A, unsigned int bands)
{
    typename MT::size_type M = A.num_rows();
    typename MT::size_type N = A.num_cols();
    if (M != N)
	return 1;

    typename MT::size_type i,j,k;
    typename MT::element_type sum;

    for (j = 1; j <= N; ++j)
    {
	// Check for zero pivot
        if ( A(j,j) == 0 )                 
            return 1;

	// Calculate rows above and on diagonal. A(1,j) remains as A(1,j).
	for (i = (j > bands) ? j-bands : 1; i <= j; ++i)
	{	
	    sum = 0;
	    for (k = (j > bands) ? j-bands : 1; k < i; ++k)
	    {
		sum += A(i,k)*A(k,j);
	    }
	    A(i,j) -= sum;
	}

	// Calculate rows below the diagonal.
	for (i = j+1; (i <= M) && (i <= j+bands); ++i)
	{
	    sum = 0;
	    for (k = (i > bands) ? i-bands : 1; k < j; ++k)
	    {
		sum += A(i,k)*A(k,j);
	    }
	    A(i,j) = (A(i,j) - sum) / A(j,j);
	}
    }

    return 0;
}   



/*
 * Solving (LU)x = B.  First forward substitute to solve for y, Ly = B.
 * Then backwards substitute to find x, Ux = y.  Return nonzero if a
 * problem occurs.  Limit the substitution sums to the elements on the
 * bands above and below the diagonal.
 */
template <class MT, class Vector>
int LU_solve_banded(const MT &A, Vector &b, unsigned int bands)
{
    typename MT::size_type i,j;
    typename MT::size_type M = A.num_rows();
    typename MT::size_type N = A.num_cols();
    typename MT::element_type sum;

    if (M != N || M == 0)
	return 1;

    // Forward substitution to find y.  The diagonals of the lower
    // triangular matrix are taken to be 1.
    for (i = 2; i <= M; ++i)
    {
	sum = b[i-1];
	for (j = (i > bands) ? i-bands : 1; j < i; ++j)
	{
	    sum -= A(i,j)*b[j-1];
	}
	b[i-1] = sum;
    }

    // Now for the backward substitution
    b[M-1] /= A(M,M);
    for (i = M-1; i >= 1; --i)
    {
	if (A(i,i) == 0)	// oops!
	    return 1;
	sum = b[i-1];
	for (j = i+1; (j <= N) && (j <= i+bands); ++j)
	{
	    sum -= A(i,j)*b[j-1];
	}
	b[i-1] = sum / A(i,i);
    }

    return 0;
}


#endif /* _BANDEDMATRIX_ID */

