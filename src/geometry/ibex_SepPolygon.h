//============================================================================
//                                  I B E X
// File        : Separator for the constraint : Point inside a ppolygon
// Author      : Benoit Desrochers
// Copyright   : Ecole des Mines de Nantes (France)
// License     : See the LICENSE file
// Created     : Mar 24, 2014
// Last Update : Mar 24, 2014
//============================================================================

#ifndef __IBEX_SEP_POINT_IN_POLYGON_H__
#define __IBEX_SEP_POINT_IN_POLYGON_H__

#include "ibex_Sep.h"
#include "ibex_CtcUnion.h"

using namespace std;

namespace ibex {

/**
 * \ingroup iset
 *
 * \brief Separator for Point inside a polygon.
 */
class SepPolygon : public Sep {

public:

	/**
	 * Create a Separator with the polygone passed as argument.
     * 
     * polygon is defined as an union of segments given in a clockwise order.
     * Si unit test for an example of usage
     * \param _ax list of x coordinate of the first point of each segment
     * \param _ay list of y coordinate of the first point of each segment
     * \param _bx list of x coordinate of the second point of each segment
     * \param _by list of y coordinate of the second point of each segment
	 */
    SepPolygon(vector<double>& _ax, vector<double>& _ay, vector<double>& _bx, vector<double>& _by);

	/**
	 * \brief Delete this.
	 */
   ~SepPolygon();

	/**
	 * \brief Separate the box.
	 */
    virtual void separate(IntervalVector& x_in, IntervalVector& x_out);

	/**
	 * TODO: add comment
	 */
    bool pointInPolygon2(double x, double y);

	/**
	 * TODO: add comment
	 */
    void inv();

private:
    // TODO: Benoit, check this.
    //CtcPointInSegments C;
    CtcUnion cseg;

    vector<double>& ax;
    vector<double>& ay;
    vector<double>& bx;
    vector<double>& by;
    bool inverse;

    vector<double> multiple, constant;
    bool check(IntervalVector &X);
    void precalc_values();
    bool pointInPolygon(double x, double y);
    int pnpoly(double testx, double testy);
};

} // end namespace

#endif // __IBEX_SEP_POINT_IN_POLYGON_H__