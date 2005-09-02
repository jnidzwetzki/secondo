/*

//paragraph [1] Title: [{\Large \bf \begin {center}] [\end {center}}]
//[TOC] [\tableofcontents]

[1] Implementation of MovingRegion algebra

[TOC]

1 Introduction

Some nice introduction should be found here.

*/

// code requiring attention is marked with *hm*!! 

/*
1 Defines and includes

*/

#include <queue>
#include <stdexcept>

#include "Algebra.h"
#include "NestedList.h"
#include "QueryProcessor.h"
#include "StandardTypes.h"

#include "TemporalAlgebra.h"

#include "DateTime.h"
using namespace datetime;

static NestedList* nl;
static QueryProcessor* qp;

const bool MRA_DEBUG = true;

static int simpleSelect(ListExpr args) {
    return 0;
}

/*
1 Helper functions

*/

const double eps = 0.00001;

static bool nearlyEqual(double a, double b) {
    return abs(a-b) <= eps;
}

static bool lowerOrNearlyEqual(double a, double b) {
    return a < b || nearlyEqual(a, b);
}

static bool greaterOrNearlyEqual(double a, double b) {
    return a > b || nearlyEqual(a, b);
}

static bool lower(double a, double b) {
    return a < b-eps;
}

static void minmax2(double a, 
		    double b, 
		    double& min, 
		    double& max) {
    if (MRA_DEBUG) cerr << "minmax4() called" << endl;

    min = a < b ? a : b;
    max = a > b ? a : b;
}

static void minmax4(double a, 
		    double b, 
		    double c, 
		    double d, 
		    double& min, 
		    double& max) {
    if (MRA_DEBUG) cerr << "minmax4() called" << endl;

    min = a;
    max = a;

    if (b < min) min = b;
    if (b > max) max = b;
    if (c < min) min = c;
    if (c > max) max = c;
    if (d < min) min = d;
    if (d > max) max = d;
}

static void GaussTransform(const unsigned int n, 
			   const unsigned int m, 
			   double** a, 
			   double* b) {
    if (MRA_DEBUG) 
	cerr << "GaussTransform() called, n=" << n << " m=" << m << endl;

    if (MRA_DEBUG)
	for (unsigned int j = 0; j < n; j++) {
	    for (unsigned int k = 0; k < m; k++) 
		fprintf(stderr, "%7.3f ", a[j][k]);
	    fprintf(stderr, "| %7.3f\n", b[j]);
	}

    for (unsigned int i = 0; i < n-1; i++) {
	cerr << "i=" << i << endl;
	unsigned int j;
	for (j = i; j < n && a[j][i] == 0; j++)
	    if (MRA_DEBUG)
		cerr << " failed as pivot: a[" 
		     << j 
		     << "][" 
		     << i 
		     << "]=" 
		     << a[j][i] 
		     << endl;
	if (j != n) {
	    if (MRA_DEBUG)
		cerr << " pivot: a[" 
		     << j 
		     << "][" 
		     << i 
		     << "]=" 
		     << a[j][i] 
		     << endl;
	    if (j != i) {
		double dummy = b[i];
		b[i] = b[j];
		b[j] = dummy;
		for (unsigned int k = 0; k < m; k++) {
		    dummy = a[i][k];
		    a[i][k] = a[j][k];
		    a[j][k] = dummy;
		}
	    }
	    for (j = i+1; j < n; j++) {
		double f = a[j][i]/a[i][i];
		if (MRA_DEBUG) {
		    cerr << " j=" << j << endl;
		    cerr << "  f=" << f << endl;
		}
		a[j][i] = 0;
		for (unsigned int k = i+1; k < m; k++) 
		    a[j][k] -= a[i][k]*f;
		b[j] -= b[i]*f;
	    }
	}

	if (MRA_DEBUG)
	    for (j = 0; j < n; j++) {
		for (unsigned int k = 0; k < m; k++) 
		    fprintf(stderr, "%7.3f ", a[j][k]);
		fprintf(stderr, "| %7.3f", b[j]);
	    }
    }
}

/*
1.1 Function ~specialSegmentIntersects2()~

Returns ~true~ if the specified segment and line intersect.

The segment connects the points $(l1p1x, l1p1y, z)$ and $(l1p2x, l1p2y, z)$,
ie.\ segment 1 is parallel to the $(x, y)$-plane.

The line is $P+Q\cdot t$. The line must not be parallel to the $(x, y)-plane$.

*/

static bool specialSegmentIntersects2(double z,
				      double l1p1x,
				      double l1p1y,
				      double l1p2x,
				      double l1p2y,
				      double* P,
				      double* Q) {
    if (MRA_DEBUG) {
	cerr << "specialSegmentIntersects2() called" 
	     << endl;
	cerr << "specialSegmentIntersects2() line 1: (" 
	     << l1p1x
	     << ", "
	     << l1p1y
	     << ", "
	     << z
	     << ")-("
	     << l1p2x
	     << ", "
	     << l1p2y
	     << ", "
	     << z
	     << ")"
	     << endl;
	cerr << "specialSegmentIntersects2() line 2: (" 
	     << P[0]
	     << ", "
	     << P[1]
	     << ", "
	     << P[2]
	     << ")-("
	     << Q[0]
	     << ", "
	     << Q[1]
	     << ", "
	     << Q[2]
	     << ")*t"
	     << endl;
    }

/*
Calculate point $(x, y)$ where the line intersects the plane parallel to the
$(x, y)$-plane with distance z from the $(x, y)$-plane.

*/

    double t2 = (z-P[2])/Q[2];

    if (MRA_DEBUG)
	cerr << "specialSegmentIntersects2() t=" 
	     << t2
	     << endl;

    double x = P[0]+Q[0]*t2;
    double y = P[1]+Q[1]*t2;

    if (MRA_DEBUG)
	cerr << "specialSegmentIntersects2() x=" 
	     << x
	     << " y="
	     << y
	     << endl;

    double l1MinX = l1p1x < l1p2x ? l1p1x : l1p2x;
    double l1MaxX = l1p1x > l1p2x ? l1p1x : l1p2x;
    double l1MinY = l1p1y < l1p2y ? l1p1y : l1p2y;
    double l1MaxY = l1p1y > l1p2y ? l1p1y : l1p2y;

    if (MRA_DEBUG)
	cerr << "specialSegmentIntersects2() l1MinX=" 
	     << l1MinX
	     << " l1MaxX="
	     << l1MaxX
	     << " l1MinX="
	     << l1MinY
	     << " l1MaxY="
	     << l1MaxY
	     << endl;

    if (nearlyEqual(l1p1x, l1p2x) && nearlyEqual(l1p1y, l1p2y)) {
/*
The segment is actually a point.

*/

	if (nearlyEqual(l1p1x, x) && nearlyEqual(l1p1y, y)) {
	    if (MRA_DEBUG)
		cerr 
		    << "specialSegmentIntersects2() intersects #1" 
		    << endl;

	    return true;
	} else {
	    if (MRA_DEBUG)
		cerr 
		    << "specialSegmentIntersects2() no intersection #1" 
		    << endl;

	    return false;
	}
    } else if (nearlyEqual(l1p1x, l1p2x)) {
/*
The segment is vertical in the $(x, y)$-plane.

*/

	if (nearlyEqual(x, l1p1x)
	    && lowerOrNearlyEqual(l1MinY, y)
	    && lowerOrNearlyEqual(y, l1MaxY)) {
	    if (MRA_DEBUG)
		cerr 
		    << "specialSegmentIntersects2() intersects #2" 
		    << endl;

	    return true;
	} else {
	    if (MRA_DEBUG)
		cerr 
		    << "specialSegmentIntersects2() no intersection #2" 
		    << endl;

	    return false;
	}
    } else if (nearlyEqual(l1p1y, l1p2y)) {
/*
The segment is horizontal in the $(x, y)$-plane.

*/

	if (nearlyEqual(y, l1p1y)
	    && lowerOrNearlyEqual(l1MinX, x)
	    && lowerOrNearlyEqual(x, l1MaxX)) {
	    if (MRA_DEBUG)
		cerr 
		    << "specialSegmentIntersects2() intersects #3" 
		    << endl;

	    return true;
	} else {
	    if (MRA_DEBUG)
		cerr 
		    << "specialSegmentIntersects2() no intersection #3" 
		    << endl;

	    return false;
	}
    } else {
/*
Otherwise, $l1p1x\ne l1p2x$ and $l1p1y\ne l1p2y$ so that we can happily
use the quotients below.

First, check whether $(x, y)$ is on the line through the segment.

*/

	double t1 = (x-l1p1x)/(l1p2x-l1p1x);

	if (!nearlyEqual(t1, (y-l1p1y)/(l1p2y-l1p1y))) {
	    if (MRA_DEBUG)
		cerr 
		    << "specialSegmentIntersects2() no intersection #4a" 
		    << endl;

	    return false;
	}

/*
Verify that $(x, y)$ is actually on the segment: As we already know that
it is on the line through the segment, we just have to check its bounding
box parallel to the $(x, y)$-plane.

*/

	if (lower(x, l1MinX)
	    || lower(l1MaxX, x)
	    || lower(y, l1MinY)
	    || lower(l1MaxY, y)) {
	    if (MRA_DEBUG)
		cerr << "specialSegmentIntersects2() no intersection #4b" 
		     << endl;

	    return false;
	}
	
	if (MRA_DEBUG)
	    cerr << "specialSegmentIntersects2() intersects" << endl;

	return true;
    }
}

/*
1.1 Function ~specialSegmentIntersects1()~
\label{ssi}

Returns ~true~ if the two specified segments intersect.

The two lines must meet the following conditions.

Segment 1 connects the points $(l1p1x, l1p1y, 0)$ and $(l1p2x, l1p2y, dt)$.

Segment 2 connects the points $(l2p1x, l2p1y, 0)$ and $(l2p2x, l2p2y, dt)$.

$z$ will contain the $z$-coordinate of the intersection point, if it 
exists.

*/

static bool specialSegmentIntersects1(double dt,
				      double l1p1x,
				      double l1p1y,
				      double l1p2x,
				      double l1p2y,
				      double l2p1x,
				      double l2p1y,
				      double l2p2x,
				      double l2p2y,
				      double& z) {
    if (MRA_DEBUG) {
	cerr << "specialSegmentIntersects1() w/ z component called" 
	     << endl;
	cerr << "specialSegmentIntersects1() line 1: (" 
	     << l1p1x
	     << ", "
	     << l1p1y
	     << ", 0.0)-("
	     << l1p2x
	     << ", "
	     << l1p2y
	     << ", "
	     << dt
	     << ")"
	     << endl;
	cerr << "specialSegmentIntersects1() line 2: (" 
	     << l2p1x
	     << ", "
	     << l2p1y
	     << ", 0.0)-("
	     << l2p2x
	     << ", "
	     << l2p2y
	     << ", "
	     << dt
	     << ")"
	     << endl;
    }

/*
Check if both segments are identical.

*/
    if (nearlyEqual(l1p1x, l2p1x)
	&& nearlyEqual(l1p1y, l2p1y)
	&& nearlyEqual(l1p2x, l2p2x)
	&& nearlyEqual(l1p2y, l2p2y)) {
	if (MRA_DEBUG)
	    cerr << "specialSegmentIntersects1() same segment" 
		 << endl;
	
	return false;
    }

/*
The line through segment 1 is 
$(l1p1x, l1p1y, 0)+(l1p2x-l1p1x, l1p2y-l1p2y, dt)\cdot s$.
The line through segment 2 is 
$(l2p1x, l2p1y, 0)+(l2p2x-l1p1x, l2p2y-l2p2y, dt)\cdot t$.

The intersection of the two lines is the solution of the following
linear system of equations:
\begin{eqnarray}
\begin{array}{ccc}
\begin{array}{cc}
l1p2x-l1p1x & l2p1x-l2p2x \\
l1p2y-l1p1y & l2p1y-l2p2y \\
dt          & -dt
\end{array} & \left| \begin{array}{c}
l2p1x-l1p1x \\
l2p1y-l1p1y \\
0
\end{array} \right.
\end{array}
\nonumber\end{eqnarray}

We put the left handed sides of the equations in matrix $a$ and the right
handed sides into array $b$ and are applying the Gaussian elimination to these:

*/

    double A[3][2] = 
	{{ l1p2x-l1p1x, l2p1x-l2p2x },
	 { l1p2y-l1p1y, l2p1y-l2p2y },
	 { dt,          -dt         }};
    double B[3] = 
	{ l2p1x-l1p1x, l2p1y-l1p1y, 0 };

    double* Ap[] = { A[0], A[1], A[2] };

    GaussTransform(3, 2, Ap, B);

/*
Now, we will determine the solution $t$ from the transformed system.
We examine each row from the bottom to the top:

*/

    for (int i = 2; i >= 0; i--) {
	if (!nearlyEqual(Ap[i][0], 0.0)) break;

/*
Row is in format $0 c \left| b \right.$.

*/

	if (nearlyEqual(Ap[i][1], 0.0)) {
/*
Row is in format $0 0 \left| b \right.$. Check if there is a contradiction
indicating that there are no solutions.

*/

	    if (nearlyEqual(B[i], 0.0)) 
		continue;
	    else {
		if (MRA_DEBUG)
		    cerr << "specialSegmentIntersects1() do not intersect #1" 
			 << endl;

		return false;
	    }
	} else {
	    z = dt*B[i]/Ap[i][1];

/*
Row is in format $0 c \left| b \right.$ with $c\ne 0$. All rows below are
entirely 0.
Inserting $t=b/c$ into the equation of line 2 yields the value $z=dt\cdot b/c$
for the third component. If $0\le z \le dt$, the two segments intersect.

*/

	    if (MRA_DEBUG)
		cerr << "specialSegmentIntersects1() z=" << z << endl; 

	    if (lowerOrNearlyEqual(0.0, z) && lowerOrNearlyEqual(z, dt)) {
		if (MRA_DEBUG)
		    cerr << "specialSegmentIntersects1() intersect" 
			 << endl;

		return true;
	    } else {
		if (MRA_DEBUG)
		    cerr << "specialSegmentIntersects1() do not intersect #2" 
			 << endl;
		
		return false;
	    }
	}
    }

/*
This should not happen since both segments must not be on the same line.

*/
    assert(false);
}

/*
1.1 Function ~specialSegmentIntersects1()~

Same as function in section \ref{ssi}, but not returning the $z$-coordinate
of the intersection point.

*/

static bool specialSegmentIntersects1(double dt,
				      double l1p1x,
				      double l1p1y,
				      double l1p2x,
				      double l1p2y,
				      double l2p1x,
				      double l2p1y,
				      double l2p2x,
				      double l2p2y) {
    if (MRA_DEBUG)
	cerr << "specialSegmentIntersects1() w/o z-component called" 
	     << endl;

    double dummy;

    return
	specialSegmentIntersects1(dt,
				  l1p1x, l1p1y, l1p2x, l1p2y,
				  l2p1x, l2p1y, l2p2x, l2p2y,
				  dummy);
}

/*
1.1 Function ~specialTrapeziumIntersects()~ (for unit testing)
\label{stiut}

See section \ref{stinu} for a full description of the parameters and 
result of this function except the additional parameter ~detailedResult~,
which will receive a numeric representation of the specific case 
responsible for the return value of the function. ~detailedResult~ is
only used for unit testing: Since this function is quite complex, we have
introduced this parameter to facilitate detailed unit testing. Since
~detailedResult~ is used for unit testing only, it is not described in
complete detail here.

*/

static bool specialTrapeziumIntersects(double dt,
				       double t1p1x,
				       double t1p1y,
				       double t1p2x,
				       double t1p2y,
				       double t1p3x,
				       double t1p3y,
				       double t1p4x,
				       double t1p4y,
				       double t2p1x,
				       double t2p1y,
				       double t2p2x,
				       double t2p2y,
				       double t2p3x,
				       double t2p3y,
				       double t2p4x,
				       double t2p4y,
				       unsigned int& detailedResult) {
    if (MRA_DEBUG) 
	cerr << "specialTrapeziumIntersects() (w/ detailedResult) called" 
	     << endl;

    detailedResult = 0;

    if (MRA_DEBUG) {
	cerr << "specialTrapeziumIntersects() trapezium 1: (" 
	     << t1p1x
	     << ", "
	     << t1p1y
	     << "), ("
	     << t1p2x
	     << ", "
	     << t1p2y
	     << "), ("
	     << t1p3x
	     << ", "
	     << t1p3y
	     << "), ("
	     << t1p4x
	     << ", "
	     << t1p4y
	     << ")"
	     << endl;
	cerr << "specialTrapeziumIntersects() trapezium 2: (" 
	     << t2p1x
	     << ", "
	     << t2p1y
	     << "), ("
	     << t2p2x
	     << ", "
	     << t2p2y
	     << "), ("
	     << t2p3x
	     << ", "
	     << t2p3y
	     << "), ("
	     << t2p4x
	     << ", "
	     << t2p4y
	     << ")"
	     << endl;
    }

/*
First, lets check the bounding boxes in the $(x, y)$-plane of the two 
trapeziums.

*/

    double t1MinX;
    double t1MaxX;
    double t1MinY;
    double t1MaxY;

    minmax4(t1p1x, t1p2x, t1p3x, t1p4x, t1MinX, t1MaxX);
    minmax4(t1p1y, t1p2y, t1p3y, t1p4y, t1MinY, t1MaxY);

    double t2MinX;
    double t2MaxX;
    double t2MinY;
    double t2MaxY;

    minmax4(t2p1x, t2p2x, t2p3x, t2p4x, t2MinX, t2MaxX);
    minmax4(t2p1y, t2p2y, t2p3y, t2p4y, t2MinY, t2MaxY);

    if (MRA_DEBUG) {
	cerr << "specialTrapeziumIntersects() t1MinX=" 
	     << t1MinX
	     << " t1MaxX="
	     << t1MaxX
	     << " t1MinY="
	     << t1MinY
	     << " t1MaxY="
	     << t1MaxY
	     << endl;
	cerr << "specialTrapeziumIntersects() t2MinX=" 
	     << t2MinX
	     << " t2MaxX="
	     << t2MaxX
	     << " t2MinY="
	     << t2MinY
	     << " t2MaxY="
	     << t2MaxY
	     << endl;
    }

    if (lower(t1MaxX, t2MinX)
	|| lower(t2MaxX, t1MinX)
	|| lower(t1MaxY, t2MinY)
	|| lower(t2MaxY, t1MinY)) {
	if (MRA_DEBUG) 
	    cerr << "specialTrapeziumIntersects() no bbox overlap" 
		 << endl;

	detailedResult = 1;
	return false;
    }

/*
Now, lets see if the trapeziums touch in one edge.

*/

    if ((nearlyEqual(t1p1x, t2p1x) 
	 && nearlyEqual(t1p1y, t2p1y)
	 && nearlyEqual(t1p3x, t2p3x)
	 && nearlyEqual(t1p3y, t2p3y))
	|| (nearlyEqual(t1p1x, t2p2x) 
	    && nearlyEqual(t1p1y, t2p2y)
	    && nearlyEqual(t1p3x, t2p4x)
	    && nearlyEqual(t1p3y, t2p4y))
	|| (nearlyEqual(t1p2x, t2p1x) 
	    && nearlyEqual(t1p2y, t2p1y)
	    && nearlyEqual(t1p4x, t2p3x)
	    && nearlyEqual(t1p4y, t2p3y))
	|| (nearlyEqual(t1p2x, t2p2x) 
	    && nearlyEqual(t1p2y, t2p2y)
	    && nearlyEqual(t1p4x, t2p4x)
	    && nearlyEqual(t1p4y, t2p4y))) {
	if (MRA_DEBUG) 
	    cerr << "specialTrapeziumIntersects() touching" 
		 << endl;

	detailedResult = 2;
	return false;
    }

/*
The bounding boxes of the trapeziums overlap but they do not touch in one
edge. To determine if they actually intersect, we first calculate the 
intersection of the two planes spanned by the trapezium.

Create equations for the two planes spanned by the trapeziums, 
considering that one edge of
each trapezium may be a single point: Plane 1 is
$T1B+T1A\cdot (s, t)$ and Plane 2 is $T2B+T1B\cdot (s', t')$.

*/

    double T1A[3][2];
    double T1B[3];

    if (nearlyEqual(t1p1x, t1p2x) && nearlyEqual(t1p1y, t1p2y)) {
	T1A[0][0] = t1p4x-t1p3x;
	T1A[1][0] = t1p4y-t1p3y;
	T1A[2][0] = 0;

	T1A[0][1] = t1p1x-t1p3x;
	T1A[1][1] = t1p1y-t1p3y;
	T1A[2][1] = -dt;

	T1B[0] = t1p3x;
	T1B[1] = t1p3y;
	T1B[2] = dt;
    } else {
	T1A[0][0] = t1p2x-t1p1x;
	T1A[1][0] = t1p2y-t1p1y;
	T1A[2][0] = 0;

	T1A[0][1] = t1p3x-t1p1x;
	T1A[1][1] = t1p3y-t1p1y;
	T1A[2][1] = dt;

	T1B[0] = t1p1x;
	T1B[1] = t1p1y;
	T1B[2] = 0;
    }

    double T2A[3][2];
    double T2B[3];

    if (nearlyEqual(t2p1x, t2p2x) && nearlyEqual(t2p1y, t2p2y)) {
	T2A[0][0] = t2p4x-t2p3x;
	T2A[1][0] = t2p4y-t2p3y;
	T2A[2][0] = 0;

	T2A[0][1] = t2p1x-t2p3x;
	T2A[1][1] = t2p1y-t2p3y;
	T2A[2][1] = -dt;

	T2B[0] = t2p3x;
	T2B[1] = t2p3y;
	T2B[2] = dt;
    } else {
	T2A[0][0] = t2p2x-t2p1x;
	T2A[1][0] = t2p2y-t2p1y;
	T2A[2][0] = 0;

	T2A[0][1] = t2p3x-t2p1x;
	T2A[1][1] = t2p3y-t2p1y;
	T2A[2][1] = dt;

	T2B[0] = t2p1x;
	T2B[1] = t2p1y;
	T2B[2] = 0;
    }

/*
Create a linear system of equations $A$ and $B$, which represents
$T1B+T1A\cdot (s, t)=T2B+T2A\cdot (s', t')$. Apply Gaussian elimination to
$A$ and $B$.

*/

    double A[3][4];
    double B[3];
    double* Ap[3];

    for (unsigned int i = 0; i < 3; i++) {
	A[i][0] = T1A[i][0];
	A[i][1] = T1A[i][1];
	A[i][2] = -T2A[i][0];
	A[i][3] = -T2A[i][1];
	B[i] = T2B[i]-T1B[i];
	Ap[i] = A[i];
    }

    GaussTransform(3, 4, Ap, B);

/*
Now, the linear system of equation has the following format
\begin{eqnarray}
\begin{array}{ccc}
\begin{array}{cccc}
\ast & \ast & \ast & \ast \\
0    & \ast & \ast & \ast \\
0    & 0    & c1   & c2 
\end{array} & \left| \begin{array}{c}
\ast \\
\ast \\
b
\end{array} \right.
\end{array}
\nonumber\end{eqnarray}
The asterisk denotes arbitrary values.

If $c1=0$, $c2=0$ and $b\ne0$, the two planes are parallel and not identical:
They do not intersect. If $c1=0$, $c2=0$ and $b=0$, the two planes are 
identical and we have to check if the two trapeziums overlap.

*/

    if (nearlyEqual(Ap[2][2], 0.0) && nearlyEqual(Ap[2][3], 0.0)) {
	if (nearlyEqual(B[2], 0.0)) {
/*
They overlap if one of the segments of each trapezium, which are not 
parallel to the $(x, y)$-plane, intersect with another of such segments
of the other trapezium.

*/
	    if (MRA_DEBUG) 
		cerr << "specialTrapeziumIntersects() identical plane" 
		     << endl;

	    if (specialSegmentIntersects1(dt,
					  t1p1x, t1p1y, t1p3x, t1p3y,
					  t2p1x, t2p1y, t2p3x, t2p3y)
		|| specialSegmentIntersects1(dt,
					     t1p1x, t1p1y, t1p3x, t1p3y,
					     t2p2x, t2p2y, t2p4x, t2p4y)
		|| specialSegmentIntersects1(dt,
					     t1p2x, t1p2y, t1p4x, t1p4y,
					     t2p1x, t2p1y, t2p3x, t2p3y)
		|| specialSegmentIntersects1(dt,
					     t1p2x, t1p2y, t1p4x, t1p4y,
					     t2p2x, t2p2y, t2p4x, t2p4y)) {
		if (MRA_DEBUG) 
		    cerr << "specialTrapeziumIntersects() intersects" 
			 << endl;

		detailedResult = 3;
		return true;
	    } else {
		if (MRA_DEBUG) 
		    cerr << "specialTrapeziumIntersects() do not intersect" 
			 << endl;

		detailedResult = 4;
		return false;
	    }
	} else {
	    if (MRA_DEBUG) 
		cerr << "specialTrapeziumIntersects() parallel" 
		     << endl;

	    detailedResult = 5;
	    return false;
	}
    }

/*
Otherwise, the two planes intersect in a line, which we need to
calculate now. We have to consider the following cases: For $c1=0$, it 
follows that $t'=b/c2$ with $s'$ arbitrary; for $c2=0$, it follows that 
$s'=b/c1$ with $t'$ arbitrary; otherwise, it follows that 
$s'=(b-c2\cdot t')/c1$ with $t'$ arbitrary.

Inserting $s'$ and $t'$ into the equation of the plane spanned by 
trapezium 2 provides the equation of the intersection line, which we
will use in notation $P+Q\cdot u$.

*/

    if (MRA_DEBUG) 
	cerr << "specialTrapeziumIntersects() intersect" 
	     << endl;

    double P[3];
    double Q[3];

    if (nearlyEqual(Ap[2][2], 0.0)) {
/*
Case 1: $c1=0$.

*/

	double f = B[2]/Ap[2][3]; // = b/c2

	for (unsigned int i = 0; i < 3; i++) {
	    P[i] = T2B[i]+T2A[i][1]*f;
	    Q[i] = T2A[i][0];
	}
    } else if (nearlyEqual(Ap[2][3], 0.0)) {
/*
Case 2: $c2=0$.

*/

	double f = B[2]/Ap[2][2]; // = b/c1

	for (unsigned int i = 0; i < 3; i++) {
	    P[i] = T2B[i]+T2A[i][0]*f;
	    Q[i] = T2A[i][1];
	}
    } else {
/*
Case 3: $c1\ne 0$ and $c2\ne 0$.

Inserting $s'=(b-c2\cdot t')/c1$ into $T2B+T2A\cdot (s', t')$ yields
\begin{eqnarray}
\left(
\begin{array}{c} 
T2B[0] \\
T2B[1] \\
T2B[2] 
\end{array}
\right)+\left(
\begin{array}{c} 
T2A[0][0] \\
T2A[1][0] \\
T2A[2][0] 
\end{array}
\right)\cdot (b-c2\cdot t')/c1+\left(
\begin{array}{c}
T2A[0][1] \\
T2A[1][1] \\
T2A[2][1] 
\end{array}
\right)\cdot t'
\nonumber\end{eqnarray}
and
\begin{eqnarray}
\left(
\begin{array}{c} 
T2B[0]+T2A[0][0]\cdot b/c1 \\
T2B[1]+T2A[1][0]\cdot b/c1 \\
T2B[2]+T2A[2][0]\cdot b/c1
\end{array}
\right)+
\left(
\begin{array}{c}
 T2A[0][1]-T2A[0][0]\cdot c2/c1 \\
 T2A[1][1]-T2A[1][0]\cdot c2/c1 \\
 T2A[2][1]-T2A[2][0]\cdot c2/c1
\end{array}
\right)\cdot t'
\nonumber\end{eqnarray}

*/

	double f1 = B[2]/Ap[2][2];    // = b/c1
	double f2 = A[2][3]/Ap[2][2]; // = c2/c1

	if (MRA_DEBUG) 
	    cerr << "specialTrapeziumIntersects() f1=" 
		 << f1
		 << " f2="
		 << f2
		 << endl;

	for (unsigned int i = 0; i < 3; i++) {
	    P[i] = T2B[i]+T2A[i][0]*f1;
	    Q[i] = T2A[i][1]-T2A[i][0]*f2;
	}
    }

    if (MRA_DEBUG) 
	cerr << "specialTrapeziumIntersects() (" 
	     << P[0]
	     << ", "
	     << P[1]
	     << ", "
	     << P[2]
	     << ")+("
	     << Q[0]
	     << ", "
	     << Q[1]
	     << ", "
	     << Q[2]
	     << ")*u"
	     << endl;

/*
Now, we have to check whether the intersection line intersects at least one
of the segments of each trapezium.

If $Q[2]=0$, the intersection line is parallel to the $(x, y)$-plane.

*/

    if (nearlyEqual(Q[2], 0.0)) {
/*
Check whether the intersection line is within the z-Range covered by the two
trapeziums.

*/
	if (MRA_DEBUG) 
	    cerr << "specialTrapeziumIntersects() intersection line "
		 << "parallel to (x, y)-plane" 
		 << endl;

	if (lower(P[2], 0) || lower(dt, P[2])) {
	    if (MRA_DEBUG) 
		cerr << "specialTrapeziumIntersects() no intersection" 
		     << endl;

	    detailedResult = 6;
	    return false;
	}

/*
Calculate the points where the trapeziums' segments intersect with the
intersection line.

*/

	double ip1x = t1p1x+(t1p3x-t1p1x)*P[2]/dt;
	double ip1y = t1p1y+(t1p3y-t1p1y)*P[2]/dt;

	double ip2x = t1p2x+(t1p4x-t1p2x)*P[2]/dt;
	double ip2y = t1p2y+(t1p4y-t1p2y)*P[2]/dt;

	double ip3x = t2p1x+(t2p3x-t2p1x)*P[2]/dt;
	double ip3y = t2p1y+(t2p3y-t2p1y)*P[2]/dt;

	double ip4x = t2p2x+(t2p4x-t2p2x)*P[2]/dt;
	double ip4y = t2p2y+(t2p4y-t2p2y)*P[2]/dt;

/*
Check for overlaps of the bounding boxes of the two intersection segments.
If they overlap, the sections intersect.

*/

	double ip1ip2MinX = ip1x < ip2x ? ip1x : ip2x;
	double ip1ip2MaxX = ip1x > ip2x ? ip1x : ip2x;

	double ip1ip2MinY = ip1y < ip2y ? ip1y : ip2y;
	double ip1ip2MaxY = ip1y > ip2y ? ip1y : ip2y;

	double ip3ip4MinX = ip3x < ip4x ? ip3x : ip4x;
	double ip3ip4MaxX = ip3x > ip4x ? ip3x : ip4x;

	double ip3ip4MinY = ip3y < ip4y ? ip3y : ip4y;
	double ip3ip4MaxY = ip3y > ip4y ? ip3y : ip4y;

	if (lower(ip1ip2MaxX, ip3ip4MinX)
	    || lower(ip3ip4MaxX, ip1ip2MinX)
	    || lower(ip1ip2MaxY, ip3ip4MinY)
	    || lower(ip3ip4MaxY, ip1ip2MinY)) {
	    if (MRA_DEBUG) 
		cerr << "specialTrapeziumIntersects() no intersection" 
		     << endl;

/*
This case should never happen.

*/

	    assert(false);

	    // return false;
	} else {
	    if (MRA_DEBUG) 
		cerr << "specialTrapeziumIntersects() intersection" 
		     << endl;

	    detailedResult = 7;
	    return true;
	}
    } else {
/*
Intersection line hits $(x, y)$-plane in $(P[0], P[1], 0)$ and the
plane parallel to $(x, y)$-plane in distance $dt$ in 
$(P[0]+Q[0], P[1]+Q[1], dt)$ as we have handled the case $Q[2]=0$
separately.

*/

	double z;

	double t1zMin = dt;
	double t1zMax = 0;
	bool t1Intersects = false;

	if (specialSegmentIntersects2(0, t1p1x, t1p1y, t1p2x, t1p2y, P, Q)) {
	    t1zMin = t1zMin > 0 ? 0 : t1zMin;
	    t1zMax = t1zMax < 0 ? 0 : t1zMax;
	    t1Intersects = true;
	}
	if (specialSegmentIntersects2(dt, t1p3x, t1p3y, t1p4x, t1p4y, P, Q)) {
	    t1zMin = t1zMin > dt ? dt : t1zMin;
	    t1zMax = t1zMax < dt ? dt : t1zMax;
	    t1Intersects = true;
	}
	if (specialSegmentIntersects1(
		dt, 
		t1p1x, t1p1y, t1p3x, t1p3y, 
		P[0], P[1], P[0]+Q[0], P[1]+Q[1])) {
	    t1zMin = t1zMin > z ? z : t1zMin;
	    t1zMax = t1zMax < z ? z : t1zMax;
	    t1Intersects = true;
	}
	if (specialSegmentIntersects1(
		dt,
		t1p2x, t1p2y, t1p4x, t1p4y, 
		P[0], P[1], P[0]+Q[0], P[1]+Q[1],
		z)) {
	    t1zMin = t1zMin > z ? z : t1zMin;
	    t1zMax = t1zMax < z ? z : t1zMax;
	    t1Intersects = true;
	}

	double t2zMin = dt;
	double t2zMax = 0;
	bool t2Intersects = false;

	if (specialSegmentIntersects2(0, t2p1x, t2p1y, t2p2x, t2p2y, P, Q)) {
	    t2zMin = t2zMin > 0 ? 0 : t2zMin;
	    t2zMax = t2zMax < 0 ? 0 : t2zMax;
	    t2Intersects = true;
	}
	if (specialSegmentIntersects2(dt, t2p3x, t2p3y, t2p4x, t2p4y, P, Q)) {
	    t2zMin = t2zMin > dt ? dt : t2zMin;
	    t2zMax = t2zMax < dt ? dt : t2zMax;
	    t2Intersects = true;
	}
	if (specialSegmentIntersects1(
		dt,
		t2p1x, t2p1y, t2p3x, t2p3y,
		P[0], P[1], P[0]+Q[0], P[1]+Q[1],
		z)) {
	    t2zMin = t2zMin > z ? z : t2zMin;
	    t2zMax = t2zMax < z ? z : t2zMax;
	    t2Intersects = true;
	}
	if (specialSegmentIntersects1(
		dt,
		t2p2x, t2p2y, t2p4x, t2p4y,
		P[0], P[1], P[0]+Q[0], P[1]+Q[1],
		z)) {
	    t2zMin = t2zMin > z ? z : t2zMin;
	    t2zMax = t2zMax < z ? z : t2zMax;
	    t2Intersects = true;
	}

	if (MRA_DEBUG) {
	    cerr << "specialTrapeziumIntersects() t1Intersects="
		 << t1Intersects
		 << endl;
	    if (t1Intersects)
		cerr << "specialTrapeziumIntersects() t1zMin="
		     << t1zMin
		     << " t1zMax="
		     << t1zMax
		     << endl;
	    cerr << "specialTrapeziumIntersects() t2Intersects="
		 << t2Intersects
		 << endl;
	    if (t2Intersects)
		cerr << "specialTrapeziumIntersects() t2zMin="
		     << t2zMin
		     << " t2zMax="
		     << t2zMax
		     << endl;
	}

	if (t1Intersects
	    && t2Intersects
	    && !(lower(t1zMax, t2zMin) || lower(t2zMax, t1zMin))) {
	    if (MRA_DEBUG) 
		cerr << "specialTrapeziumIntersects() intersect" 
		     << endl;
	    
	    detailedResult = 8;
	    return true;
	} else {
	    if (MRA_DEBUG) 
		cerr << "specialTrapeziumIntersects() no intersection" 
		     << endl;

	    detailedResult = 9;
	    return false;
	}
    }
}

/*
1.1 Function ~specialTrapeziumIntersects()~ (for normal use)
\label{stinu}

Returns ~true~ if and only if the two specified special trapeziums intersect.

The two trapeziums must meet the following conditions. These conditions are
not checked in the function. If the conditions are not met, the function will
deliver incorrect results.

Trapezium 1 is spanned by segments ~(t1p1x, t1p1y, 0)~ to ~(t1p2x, t1p2y, 0)~, 
~(t1p1x, t1p1y, 0)~ to ~(t1p3x, t1p3y, dt)~, ~(t1p3x, t1p3y, dt)~ to 
~(t1p4x, t1p4x, dt)~ and ~(t1p2x, t1p2y, 0)~ to ~(t1p4x, t1p4x, dt)~.
Allowed are either ~(t1p1x, t1p1y, 0)=(t1p2x, t1p2y, 0)~ or
~(t1p3x, t1p3y, dt)=(t1p4x, t1p4x, dt)~ but not both. Segments 
~(t1p1x, t1p1y, 0)~ to ~(t1p2x, t1p2y, 0)~ and 
~(t1p3x, t1p3y, dt)~ to ~(t1p4x, t1p4x, dt)~ are collinear.

Trapezium 2 is spanned by segments ~(t2p1x, t2p1y, 0)~ to ~(t2p2x, t2p2y, 0)~, 
~(t2p1x, t2p1y, 0)~ to ~(t2p3x, t2p3y, dt)~, ~(t2p3x, t2p3y, dt)~ to
~(t2p4x, t2p4x, dt)~ and ~(t2p2x, t2p2y, 0)~ to ~(t2p4x, t2p4x, dt)~.
Allowed are either ~(t2p1x, t2p1y, 0)=(t2p2x, t2p2y, 0)~ or
~(t2p3x, t2p3y, dt)=(t2p4x, t2p4x, dt)~ but not both. Segments
~(t2p1x, t2p1y, 0)~ to ~(t2p2x, t2p2y, 0)~ and
~(t2p3x, t2p3y, dt)~ to ~(t2p4x, t2p4x, dt)~ are collinear.

$dt$ must be greater than $0$.

*/

static bool specialTrapeziumIntersects(double dt,
				       double t1p1x,
				       double t1p1y,
				       double t1p2x,
				       double t1p2y,
				       double t1p3x,
				       double t1p3y,
				       double t1p4x,
				       double t1p4y,
				       double t2p1x,
				       double t2p1y,
				       double t2p2x,
				       double t2p2y,
				       double t2p3x,
				       double t2p3y,
				       double t2p4x,
				       double t2p4y) {
    if (MRA_DEBUG) 
	cerr << "specialTrapeziumIntersects() called" 
	     << endl;

    unsigned int detailedResult;

/*
This function is just a thin wrapper around the function in section
\ref{stiut}, throwing away the parameter ~detailedResult~, which is 
only used for unit testing.

*/

    return
	specialTrapeziumIntersects(
	    dt,
	    t1p1x, t1p1y, t1p2x, t1p2y, t1p3x, t1p3y, t1p4x, t1p4y,
	    t2p1x, t2p1y, t2p2x, t2p2y, t2p3x, t2p3y, t2p4x, t2p4y,
	    detailedResult);
}

// ************************************************************************

/*
1 Data type ~iregion~

*/

// *hm* Verify list representation

static ListExpr IRegionProperty() {
    if (MRA_DEBUG) cerr << "IRegionProperty() called" << endl;
    
    ListExpr example = nl->TextAtom();
    nl->AppendText(example,
		   "(\"2003-01-10\" (((3 0)(10 1)(3 1))((3.1 0.1)"
		   "(3.1 0.9)(6 0.8))))");
    return 
	nl->TwoElemList(
            nl->FourElemList(
		nl->StringAtom("Signature"),
		nl->StringAtom("Example Type List"),
		nl->StringAtom("List Rep"),
		nl->StringAtom("Example List")),
            nl->FourElemList(
		nl->StringAtom("-> UNIT"),
		nl->StringAtom("(iregion)"),
		nl->StringAtom("(<instant> <region>)"),
		example));
}

static bool CheckIRegion(ListExpr type, ListExpr& errorInfo) {
    if (MRA_DEBUG) cerr << "CheckIRegion() called" << endl;

    return nl->IsEqual(type, "iregion");
}

static TypeConstructor iregion(
    "iregion",
    IRegionProperty,
    OutIntime<CRegion, OutRegion>, 
    InIntime<CRegion, InRegion>,
    0, 0, // SaveToList, RestoreFromList
    CreateIntime<CRegion>,
    DeleteIntime<CRegion>,
    0, 0, // open, save
    CloseIntime<CRegion>,
    CloneIntime<CRegion>,
    CastIntime<CRegion>,
    SizeOfIntime<CRegion>,
    CheckIRegion,
    0, // predef. pers. function for model
    TypeConstructor::DummyInModel,
    TypeConstructor::DummyOutModel,
    TypeConstructor::DummyValueToModel,
    TypeConstructor::DummyValueListToModel );

// ************************************************************************

/*
1 Data type ~uregion~

1.1 Helper class ~MPointData~ 

*/

class MSegmentData {
// *hm* Debug only, set attributes to private later again
// private:
public:
    unsigned int faceno;
    unsigned int cycleno;
    unsigned int segmentno;

    bool insideLeftOrAbove;
    bool degeneratedInitial;
    bool degeneratedFinal;

    double initialStartX;
    double initialStartY;
    double initialEndX;
    double initialEndY;

    double finalStartX;
    double finalStartY;
    double finalEndX;
    double finalEndY;

    bool pointInitial;
    bool pointFinal;

    MSegmentData() {
        if (MRA_DEBUG) 
	    cerr << "MSegmentData::MSegmentData() #1 called" << endl;
    }

    MSegmentData(unsigned int fno, 
		 unsigned int cno, 
		 unsigned int sno,
		 bool iloa,
		 bool di,
		 bool df,
		 double il,
		 double isx,
		 double isy,
		 double iex,
		 double iey,
		 double fsx,
		 double fsy,
		 double fex,
		 double fey) :
        faceno(fno),
        cycleno(cno),
        segmentno(sno),
	insideLeftOrAbove(iloa),
	degeneratedInitial(di),
	degeneratedFinal(df),
	initialStartX(isx),
	initialStartY(isy),
	initialEndX(iex),
	initialEndY(iey),
	finalStartX(fsx),
	finalStartY(fsy),
	finalEndX(fex),
	finalEndY(fey)  {
	if (MRA_DEBUG)
	    cerr << "MSegmentData::MSegmentData() #2 called counter=["
		 << faceno
		 << " "
		 << cycleno
		 << " "
		 << segmentno
		 << "] flags=["
		 << insideLeftOrAbove
		 << " "
		 << degeneratedInitial
		 << " "
		 << degeneratedFinal
		 << "] il="
		 << il
		 << " initial=["
		 << initialStartX
		 << " "
		 << initialStartY
		 << " "
		 << initialEndX
		 << " "
		 << initialEndY
		 << "] final=["
		 << finalStartX
		 << " "
		 << finalStartY
		 << " "
		 << finalEndX
		 << " "
		 << finalEndY
		 << "]"
		 << endl;

/*
Calculate whether segment is point in initial or final instant.

*/
	pointInitial = nearlyEqual(isx, iex) && nearlyEqual(isy, iey);
	pointFinal = nearlyEqual(fsx, fex) && nearlyEqual(fsy, fey);

/*
Check whether initial and final segment are collinear,

*/
	bool collinear;

	if (pointInitial && pointFinal) {
/* 
Error: A segment may not be reduced to a point both in initial and final
instant.

*/
	    if (MRA_DEBUG) 
		cerr << "MSegmentData::MSegmentData() both reduced" 
		     << endl;

	    throw invalid_argument("both initial and final segment "
				   "reduced to point, which is not "
				   "allowed");
	} else if (pointInitial) {
/* 
Only initial segment reduced to point. Initial and final segment are trivially
collinear.

*/
	    if (MRA_DEBUG) 
		cerr << "MSegmentData::MSegmentData() initial reduced" 
		     << endl;

	    collinear = true;
	} else if (pointFinal) {
/*
Only final segment reduced to point. Initial and final segment are trivially
collinear.

*/
	    if (MRA_DEBUG) 
		cerr << "MSegmentData::MSegmentData() final reduced" 
		     << endl;

	    collinear = true;
	} else if (nearlyEqual(isx, iex) && nearlyEqual(fsx, fex)) {
/*
Both segments are vertical. Check if both segments have the same
orientation.

*/
	    if (MRA_DEBUG) 
		cerr << "MSegmentData::MSegmentData() both vertical" << endl;

	    collinear = 
		(lowerOrNearlyEqual(isy, iey) 
		 && lowerOrNearlyEqual(fsy, fey))
		|| (greaterOrNearlyEqual(isy, iey)
		    && greaterOrNearlyEqual(fsy, fey));
	} else if (nearlyEqual(isx, iex) || nearlyEqual(fsx, fex)) {
/*
Only initial or final segment is vertical but not both.

*/
	    if (MRA_DEBUG) 
		cerr << "MSegmentData::MSegmentData() one vertical" << endl;

	    collinear = false;
	} else {
/*
Both segments are not vertical.

*/
	    if (MRA_DEBUG) 
		cerr << "MSegmentData::MSegmentData() none vertical" << endl;

	    double id = (iey-isy)/(iex-isx);
	    double fd = (fey-fsy)/(fex-fsx);

	    if (MRA_DEBUG) 
		cerr << "MSegmentData::MSegmentData() id=" 
		     << id
		     << " fd="
		     << fd 
		     << endl;

	    collinear = nearlyEqual(id, fd);
	}

	if (!collinear)
	    throw invalid_argument("initial and final segment not collinear");
    }

    unsigned int GetFaceNo(void) { return faceno; }
    unsigned int GetCycleNo(void) { return cycleno; }
    unsigned int GetSegmentNo(void) { return segmentno; }
    double GetInitialStartX(void) { return initialStartX; }
    double GetInitialStartY(void) { return initialStartY; }
    double GetInitialEndX(void) { return initialEndX; }
    double GetInitialEndY(void) { return initialEndY; }
    double GetFinalStartX(void) { return finalStartX; }
    double GetFinalStartY(void) { return finalStartY; }
    double GetFinalEndX(void) { return finalEndX; }
    double GetFinalEndY(void) { return finalEndY; }
    bool GetInsideLeftOrAbove(void) { return insideLeftOrAbove; }
/*
Since no other classes are accessing the private attributes of
~MSegmentData~, we declare ~URegion~ as friend. This is shorter,
yet somewhat clumsy than creating attribute access functions.

*/
    // *hm* is there a better solution?
    friend class URegion; 

};

/*
1.1 Class ~URegion~

*/

class URegion : public SpatialTemporalUnit<CRegion, 3> {
private:
    // *hm* Confirm whether two roles of URegion are really required
    enum { UNDEF, NORMAL, EMBEDDED } role;

    DBArray<MSegmentData>* segments;
    unsigned int segmentsStartPos;
    unsigned int segmentsNum;

public:
    URegion() {
        if (MRA_DEBUG) cerr << "URegion::URegion() #1 called" << endl;
    }

    URegion(Interval<Instant>& interval,
	    DBArray<MSegmentData>* segs,
	    unsigned int pos) : 
        SpatialTemporalUnit<CRegion, 3>(interval),
	role(EMBEDDED),
        segments(segs),
	segmentsStartPos(pos),
	segmentsNum(0) {
        if (MRA_DEBUG) 
            cerr << "URegion::URegion() #2 called" << endl;
    }

    URegion(Interval<Instant>& interval) :
	SpatialTemporalUnit<CRegion, 3>(interval),
	role(NORMAL),
        segments(new DBArray<MSegmentData>(0)),
	segmentsStartPos(0),
	segmentsNum(0) {
        if (MRA_DEBUG) 
            cerr << "URegion::URegion() #2 called" << endl;
    }

    void SetMSegmentData(DBArray<MSegmentData>* s) {
        if (MRA_DEBUG) 
            cerr << "URegion::SetMSegmentData() called" << endl;

	segments = s;
    }

    bool AddSegment(CRegion& cr,
		    CRegion& rDir,
		    unsigned int faceno,
		    unsigned int cycleno,
		    unsigned int segmentno,
		    unsigned int pointno,
		    double intervalLen,
		    ListExpr start,
		    ListExpr end);
    void SetSegmentInsideLeftOrAbove(int pos, bool insideLeftOrAbove);
    int GetSegmentsNum(void);
    void GetSegment(int pos, MSegmentData& dms);

    void Destroy(void);

    virtual void TemporalFunction(Instant& t, CRegion& result);
    virtual bool Passes(CRegion& val);
    virtual bool At(CRegion& val, TemporalUnit<CRegion>& result);

    virtual const Rectangle<3> BoundingBox() const;

    virtual URegion* Clone();
    virtual void CopyFrom(StandardAttribute* right);
};

void URegion::Destroy(void) {
    if (MRA_DEBUG) cerr << "URegion::Destroy() called" << endl;

    assert(role == NORMAL || role == EMBEDDED);

    if (role == NORMAL) segments->Destroy();
}

URegion* URegion::Clone(void) {
    if (MRA_DEBUG) cerr << "URegion::Clone() called" << endl;

    assert(false);
}

void URegion::CopyFrom(StandardAttribute* right) {
    if (MRA_DEBUG) cerr << "URegion::CopyFrom() called" << endl;

    assert(false);
}

const Rectangle<3> URegion::BoundingBox() const {
    if (MRA_DEBUG) cerr << "URegion::BoundingBox() called" << endl;

    assert(false);
}

// *hm* directly adapted from InRegion(), probably O(n^2) process 
// *hm* understand insideAbove calculation

void URegion::TemporalFunction(Instant& t, CRegion& res) {
    if (MRA_DEBUG) cerr << "URegion::TemporalFunction() called" << endl;

    assert(false);

#ifdef SCHMUH

    assert(t.IsDefined());
    assert(timeInterval.Contains(t));

    Instant t0 = timeInterval.start;
    Instant t1 = timeInterval.end;

    if (MRA_DEBUG) cerr << "URegion::TemporalFunction() trace#1.1" << endl;
    CRegion* cr = new CRegion(0);

    cr->StartBulkLoad();

    // all half segments in one face will have the same fcno
    // all half segments in one cycle will have the same ccno
    // two half segments will have the same edno. edno is only unique 
    // over these pairs within one cycle.
    // two half segments will have the same partnerno. edno is unique
    // over all these pairs.
    int fcno = -1;
    int ccno = -1;
    int edno = -1;
    int partnerno = 0;

    unsigned int pos = 0;

    // this loop goes through all faces...
    while (pos < pointsNum) {
	if (MRA_DEBUG) 
	    cerr << "URegion::TemporalFunction() new face" << endl;

	// isCycle will be true if current cycle is not a hole cycle
	bool isCycle = true;

	fcno++;
	ccno = -1;
	edno = -1;

	// this loop goes through all cycles of the current face
	// *hm* No exit condition for current face -> bug!
	while (pos < pointsNum) {
	    if (MRA_DEBUG) 
		cerr << "URegion::TemporalFunction() new cyle" << endl;

	    ccno++;
	    edno = -1;

	    // cycle must have at least three edges
	    assert(pos+2 < pointsNum);

	    // cyclepoints is used to check whether we have duplicate points
	    // in the cycle
	    Points cyclepoints(0);

	    // *hm* dunno
	    CRegion* rDir = new CRegion(0);

	    rDir->StartBulkLoad();

	    // get first point from DBArray
	    MPointData dmp;
	    points->Get(pointsStartPos+pos, dmp);
	    pos++;

	    if (MRA_DEBUG) 
		cerr << "URegion::TemporalFunction() first vertex pos="
		     << pos
		     << " x=" 
		     << dmp.GetStartX()
		     << " y="
		     << dmp.GetStartY()
		     << endl;

	    // create Point instance with coordinates of first point at 
	    // instant t
	    Point currvertex(true, 
			     (dmp.GetEndX()-dmp.GetStartX())*((t-t0)/(t1-t0))
			     +dmp.GetStartX(),
			     (dmp.GetEndY()-dmp.GetStartY())*((t-t0)/(t1-t0))
			     +dmp.GetStartY());

	    // store first point in cyclepoints
	    cyclepoints.StartBulkLoad();
	    cyclepoints += currvertex;
	    cyclepoints.EndBulkLoad();

	    // since we are adding half segments, we need to remember
	    // the previous point: current half segment connects previous
	    // and current point
	    Point prevvertex = currvertex;

	    // required later to connect first and last point to close
	    // the cycle
	    Point firstvertex = currvertex;

	    // *hm* Not sure. I think, though, that p1 and p2 are a complicated
	    // way to remember the previous point, and that proper usage of 
	    // prevvertex would make these obsolete
	    Point p1 = currvertex;
	    Point p2;

	    // this loop goes through all points of the current cycle
	    // *hm* No exit condition for current cycle -> bug!
	    while (pos < pointsNum) {
		if (MRA_DEBUG) 
		    cerr << "URegion::TemporalFunction() new edge" << endl;

		// get current point from DBArray
		points->Get(pointsStartPos+pos, dmp);
		pos++;

		if (MRA_DEBUG) 
		    cerr << "URegion::TemporalFunction() current vertex pos="
			 << pos
			 << " x=" 
			 << dmp.GetStartX()
			 << " y="
			 << dmp.GetStartY()
			 << endl;
		if (MRA_DEBUG) 
		    cerr << "URegion::TemporalFunction() previous vertex pos="
			 << pos
			 << " x=" 
			 << prevvertex.GetX()
			 << " y="
			 << prevvertex.GetY()
			 << endl;

		// set previously created Point instance to coordinates of 
		// current point at instant t
		currvertex.Set(
		    (dmp.GetEndX()-dmp.GetStartX())*((t-t0)/(t1-t0))
		    +dmp.GetStartX(),
		    (dmp.GetEndY()-dmp.GetStartY())*((t-t0)/(t1-t0))
		    +dmp.GetStartY());

		// check if current point is a duplicate
		assert(!cyclepoints.Contains(currvertex));

		if (MRA_DEBUG) 
		    cerr << "URegion::TemporalFunction() trace#2" << endl;

		// add current point to cyclepoints
		cyclepoints.StartBulkLoad();
		cyclepoints += currvertex;
		cyclepoints.EndBulkLoad();

		// *hm* dunno
		p2 = currvertex;

		if (MRA_DEBUG) 
		    cerr << "URegion::TemporalFunction() trace#3" << endl;

		// create the half segment, which connects prevvertex
		// and currvertex
		CHalfSegment chs(true, true, prevvertex, currvertex);

		// remember currvertex for next half segment in next iteration
		prevvertex = currvertex;

		// assign fcno, ccno, edno and partnerno as described above
		edno++;

		chs.attr.faceno = fcno;
		chs.attr.cycleno = ccno;
		chs.attr.edgeno = edno;
		chs.attr.partnerno = partnerno;
		partnerno++;

		// *hm* Based on the assumption, that p1 contains the
		// previous point, the next statement will store in
		// chs.attr.insideAbove whether chs contains the two
		// points in swapped order.


// observation for cycles (for whole cycle, exactly the opposite holds):
// 
// cylce clockwise && p1 > p2 => inside is left or above of seg(p1, p2)
// cycle clockwise && p1 < p2 => inside is right or below of seg(p1, p2)
// 
// cycle counterclockwise && p1 < p2 => inside is left or above of seg(p1, p2)
// cycle counterclockwise && p1 > p2 => inside is right or below of seg(p1, p2)
// 
// algorithm:
// 
// calculate insideAbove for each half segment. insideAbove will be true if and
// only if the inside is left or above the half segment
// 
// 1. check for each segment whether p1 < p2 and store true in insideAbove,
//    if so.
// 
// 2. correct insideAbove:
// 
//    (a) if cycle clockwise & p1 < p2 => insideAbove = false
//    (b) if cycle counterclockwise & p1 > p2 => insideAbove = false
// 
// 3. correct insideAbove again:
// 
//    if we are dealing with a hole cycle, invert insideAbove.

		chs.attr.insideAbove = chs.GetLP() == p1;
		if (MRA_DEBUG)
		    cerr << "URegion::TemporalFunction() p1 x="
			 << p1.GetX()
			 << " y="
			 << p1.GetY()
			 << endl;
		if (MRA_DEBUG)
		    cerr << "URegion::TemporalFunction() prevvertex x="
			 << prevvertex.GetX()
			 << " y="
			 << prevvertex.GetY()
			 << endl;
		if (MRA_DEBUG)
		    cerr << "URegion::TemporalFunction() chs.GetLP() x="
			 << chs.GetLP().GetX()
			 << " y="
			 << chs.GetLP().GetY()
			 << endl;

		// hzm *dunno*
		p1 = p2;

		if (MRA_DEBUG) 
		    cerr << "URegion::TemporalFunction() trace#4" << endl;

		// *hm* dunno
		assert(cr->insertOK(chs));

		if (MRA_DEBUG) 
		    cerr << "URegion::TemporalFunction() trace#5.1" << endl;

		// add half segment to result
		*cr += chs;
		if (MRA_DEBUG) 
		    cerr << "URegion::TemporalFunction() trace#5.2" << endl;

		// *hm* dunno
		*rDir += chs;

		if (MRA_DEBUG) 
		    cerr << "URegion::TemporalFunction() trace#6" << endl;

		// add other side of half segment
		chs.SetLDP(false);

		*cr += chs;

		if (MRA_DEBUG) 
		    cerr << "URegion::TemporalFunction() trace#7" << endl;
	    }

	    if (MRA_DEBUG) 
		cerr << "URegion::TemporalFunction() trace#8.2" << endl;

	    // create half segment connect first and last point
	    CHalfSegment chs(true, true, firstvertex, currvertex);

	    if (MRA_DEBUG) 
		cerr << "URegion::TemporalFunction() first vertex pos="
		     << pos
		     << " x=" 
		     << chs.GetLP().GetX()
		     << " y="
		     << chs.GetLP().GetY()
		     << endl;
	    if (MRA_DEBUG) 
		cerr << "URegion::TemporalFunction() current vertex pos="
		     << pos
		     << " x=" 
		     << chs.GetRP().GetX()
		     << " y="
		     << chs.GetRP().GetY()
		     << endl;

	    // assign fcno, ccno, edno and partnerno as described above
	    edno++;

	    chs.attr.faceno = fcno;
	    chs.attr.cycleno = ccno;
	    chs.attr.edgeno = edno;
	    chs.attr.partnerno = partnerno;
	    partnerno++;

	    // *hm* dunno
	    chs.attr.insideAbove = chs.GetRP() == firstvertex;

	    // *hm* dunno
	    assert(cr->insertOK(chs));

	    // add half segment to result
	    *cr += chs;

	    // *hm* dunno
	    *rDir += chs;

	    // add other side of half segment
	    chs.SetLDP(false);

	    *cr += chs;

	    rDir->EndBulkLoad();
	    
	    if (MRA_DEBUG) 
		cerr << "URegion::TemporalFunction() trace#8.2" << endl;

	    // direction = true <=> cycle goes clockwise
	    // direction = false <=> cycle goes counterclockwise
	    bool direction = rDir->GetCycleDirection();

	    if (MRA_DEBUG) 
		cerr << "URegion::TemporalFunction() trace#8.3" << endl;

	    int h = cr->Size()-(rDir->Size()*2);
	    while (h < cr->Size()) {
		CHalfSegment chsInsideAbove;
		bool insideAbove;

		cr->Get(h, chsInsideAbove);

		if (direction == chsInsideAbove.attr.insideAbove) 
		    insideAbove = false;
		else
		    insideAbove = true;
		if (!isCycle) insideAbove = !insideAbove;

		chsInsideAbove.attr.insideAbove = insideAbove;
		cr->UpdateAttr(h, chsInsideAbove.attr);

		cr->Get(h+1, chsInsideAbove);
		chsInsideAbove.attr.insideAbove = insideAbove;
		cr->UpdateAttr(h+1, chsInsideAbove.attr);

		h += 2;
	    }

	    delete rDir;

	    // only the first cycle is not hole cycle. since we already
	    // processed the first cycle, all remaining cycles are hole
	    // cycles.
	    isCycle = false;

	    if (MRA_DEBUG) 
		cerr << "URegion::TemporalFunction() trace#9" << endl;
	}
    }

    if (MRA_DEBUG) 
	cerr << "URegion::TemporalFunction() trace#10" << endl;

    // *hm* add marker for end of half segment list
    CHalfSegment chs(false);
    assert(cr->insertOK(chs));

    if (MRA_DEBUG) 
	cerr << "URegion::TemporalFunction() trace#11" << endl;

    cr->EndBulkLoad();

    if (MRA_DEBUG) 
	cerr << "URegion::TemporalFunction() trace#12" << endl;

    // *hm* dunno
    cr->SetPartnerNo();

    if (MRA_DEBUG) 
	cerr << "URegion::TemporalFunction() trace#13" << endl;

    // *hm* UGLY & MEMORY LEAK
    memcpy(&res, cr, sizeof(*cr));

#endif // SCHMUH
}

bool URegion::Passes(CRegion& val) {
    if (MRA_DEBUG) cerr << "URegion::Passes() called" << endl;

    assert(false);
}

bool URegion::At(CRegion& val, TemporalUnit<CRegion>& result) {
    if (MRA_DEBUG) cerr << "URegion::At() called" << endl;

    assert(false);
}

bool URegion::AddSegment(CRegion& cr,
			 CRegion& rDir,
			 unsigned int faceno,
			 unsigned int cycleno,
			 unsigned int segmentno,
			 unsigned int partnerno,
			 double intervalLen,
			 ListExpr start, 
			 ListExpr end) {
    if (MRA_DEBUG) 
	cerr << "URegion::AddSegment() called " 
	     << faceno
	     << " "
	     << cycleno
	     << " "
	     << segmentno
	     << endl;

    assert(role == NORMAL || role == EMBEDDED);

    try {
	if (nl->ListLength(start) != 4
	    || !nl->IsAtom(nl->First(start))
	    || nl->AtomType(nl->First(start)) != RealType
	    || !nl->IsAtom(nl->Second(start))
	    || nl->AtomType(nl->Second(start)) != RealType
	    || !nl->IsAtom(nl->Third(start))
	    || nl->AtomType(nl->Third(start)) != RealType
	    || !nl->IsAtom(nl->Fourth(start))
	    || nl->AtomType(nl->Fourth(start)) != RealType)
	    throw invalid_argument(
		"start point "
		+nl->ToString(start)
		+" not in format (<real> <real> real> <real>)");

	if (nl->ListLength(end) != 4
	    || !nl->IsAtom(nl->First(end))
	    || nl->AtomType(nl->First(end)) != RealType
	    || !nl->IsAtom(nl->Second(end))
	    || nl->AtomType(nl->Second(end)) != RealType
	    || !nl->IsAtom(nl->Third(end))
	    || nl->AtomType(nl->Third(end)) != RealType
	    || !nl->IsAtom(nl->Fourth(end))
	    || nl->AtomType(nl->Fourth(end)) != RealType)
	    throw invalid_argument(
		"end point " 
		+nl->ToString(end) 
		+" not in format (<real> <real> real> <real>)");
	
	MSegmentData dms(faceno, 
			 cycleno, 
			 segmentno, 
			 false,
			 false,
			 false,
			 intervalLen,
			 nl->RealValue(nl->First(start)), 
			 nl->RealValue(nl->Second(start)), 
			 nl->RealValue(nl->First(end)), 
			 nl->RealValue(nl->Second(end)), 
			 nl->RealValue(nl->Third(start)), 
			 nl->RealValue(nl->Fourth(start)), 
			 nl->RealValue(nl->Third(end)), 
			 nl->RealValue(nl->Fourth(end)));

	for (unsigned int i = 0; i < segmentsNum; i++) {
	    if (MRA_DEBUG) cerr << "URegion::AddSegment() i=" << i << endl;

	    MSegmentData existingDms;
	    
	    segments->Get(i, existingDms);

	    if (!dms.pointInitial
		&& !existingDms.pointInitial
		&& ((nearlyEqual(dms.initialStartX, 
				 existingDms.initialStartX)
		     && nearlyEqual(dms.initialStartY, 
				    existingDms.initialStartY)
		     && nearlyEqual(dms.initialEndX, 
				    existingDms.initialEndX)
		     && nearlyEqual(dms.initialEndY, 
				    existingDms.initialEndY))
		    || (nearlyEqual(dms.initialStartX, 
				    existingDms.initialEndX)
			&& nearlyEqual(dms.initialStartY, 
				       existingDms.initialEndY)
			&& nearlyEqual(dms.initialEndX, 
				       existingDms.initialStartX)
			&& nearlyEqual(dms.initialEndY, 
				       existingDms.initialStartY)))) {
		if (MRA_DEBUG) 
		    cerr << "URegion::AddSegment() degen'ed initial in " 
			 << i
			 << endl;

		dms.degeneratedInitial = true;
		existingDms.degeneratedInitial = true;

		segments->Put(i, existingDms);
	    }

	    if (!dms.pointFinal
		&& !existingDms.pointFinal
	        && ((nearlyEqual(dms.finalStartX, 
				 existingDms.finalStartX)
		     && nearlyEqual(dms.finalStartY, 
				    existingDms.finalStartY)
		     && nearlyEqual(dms.finalEndX, 
				    existingDms.finalEndX)
		     && nearlyEqual(dms.finalEndY, 
				    existingDms.finalEndY))
		    || (nearlyEqual(dms.finalStartX, 
				    existingDms.finalEndX)
			&& nearlyEqual(dms.finalStartY, 
				       existingDms.finalEndY)
			&& nearlyEqual(dms.finalEndX, 
				       existingDms.finalStartX)
			&& nearlyEqual(dms.finalEndY, 
				       existingDms.finalStartY)))) {
		if (MRA_DEBUG) 
		    cerr << "URegion::AddSegment() degen'ed final in " 
			 << i
			 << endl;

		dms.degeneratedFinal = true;
		existingDms.degeneratedFinal = true;

		segments->Put(i, existingDms);
	    }
	    
	    if (nearlyEqual(intervalLen, 0.0)
		&& (dms.degeneratedInitial || dms.degeneratedFinal))
		throw invalid_argument(
		    "units with zero length time interval must not "
		    "be degenerated");
	    else if (!nearlyEqual(intervalLen, 0.0)
		       && dms.degeneratedInitial
		       && dms.degeneratedFinal) 
		throw invalid_argument(
		    "units must not degenerate both in initial and "
		    "final instant");

	    specialTrapeziumIntersects(intervalLen,
				       existingDms.initialStartX,
				       existingDms.initialStartY,
				       existingDms.initialEndX,
				       existingDms.initialEndY,
				       existingDms.finalStartX,
				       existingDms.finalStartY,
				       existingDms.finalEndX,
				       existingDms.finalEndY,
				       dms.initialStartX,
				       dms.initialStartY,
				       dms.initialEndX,
				       dms.initialEndY,
				       dms.finalStartX,
				       dms.finalStartY,
				       dms.finalEndX,
				       dms.finalEndY);
	}

	double t = nearlyEqual(intervalLen, 0.0) ? 0 : 0.5;
	double xs = dms.initialStartX+(dms.finalStartX-dms.initialStartX)*t;
	double ys = dms.initialStartY+(dms.finalStartY-dms.initialStartY)*t;
	double xe = dms.initialEndX+(dms.finalEndX-dms.initialEndX)*t;
	double ye = dms.initialEndY+(dms.finalEndY-dms.initialEndY)*t;

	assert(!nearlyEqual(xs, xe) || !nearlyEqual(ys, ye));

	Point s(true, xs, ys);
	Point e(true, xe, ye);
	CHalfSegment chs(true, true, s, e);

	chs.attr.faceno = faceno;
	chs.attr.cycleno = cycleno;
	chs.attr.partnerno = partnerno;

	chs.attr.insideAbove = chs.GetLP() == s;

	if (MRA_DEBUG) {
	    cerr << "URegion::AddSegment() initial "
		 << dms.initialStartX
		 << " "
		 << dms.initialStartY
		 << " "
		 << dms.initialEndX
		 << " "
		 << dms.initialEndY
		 << endl;
	    cerr << "URegion::AddSegment() final "
		 << dms.finalStartX
		 << " "
		 << dms.finalStartY
		 << " "
		 << dms.finalEndX
		 << " "
		 << dms.finalEndY
		 << endl;
	}

	if (MRA_DEBUG) 
	    cerr << "URegion::AddSegment() "
		 << faceno
		 << " "
		 << cycleno
		 << " "
		 << partnerno
		 << " ("
		 << xs
		 << ", "
		 << ys
		 << ")-("
		 << xe
		 << ", "
		 << ye
		 << ") iloa="
		 << chs.attr.insideAbove
		 << endl;

	if (!cr.insertOK(chs)) 
	    throw invalid_argument("CRegion checks for segment failed");

	cr += chs;
	rDir += chs;

	chs.SetLDP(false);
	cr += chs;

	segments->Resize(segmentsStartPos+segmentsNum+1);
	segments->Put(segmentsStartPos+segmentsNum, dms);
	segmentsNum++;
    } catch (invalid_argument& e) {
	cerr << "checking segment " 
	     << nl->ToString(start) 
	     << " - "
	     << nl->ToString(end) 
	     << " failed: " 
	     << e.what()
	     << endl;
	return false;
    }

    return true;
}

void URegion::SetSegmentInsideLeftOrAbove(int pos, bool insideLeftOrAbove) {
    if (MRA_DEBUG) cerr << "URegion::GetSegmentLeftOrAbove() called" << endl;

    MSegmentData dms;
    segments->Get(segmentsStartPos+pos, dms);
    dms.insideLeftOrAbove = insideLeftOrAbove;
    segments->Put(segmentsStartPos+pos, dms);
}

int URegion::GetSegmentsNum(void) {
    if (MRA_DEBUG) 
	cerr << "URegion::GetSegmentsNum() called, num=" 
	     << segmentsNum
	     << endl;

    assert(role == NORMAL || role == EMBEDDED);

    return segmentsNum;
}

void URegion::GetSegment(int pos, MSegmentData& dms) {
    if (MRA_DEBUG) cerr << "URegion::GetSegment() called, pos=" << pos << endl;

    assert(role == NORMAL || role == EMBEDDED);

    segments->Get(segmentsStartPos+pos, dms);
}

/*
1.1 Algebra integration

*/

// *hm* Verify list representation

static ListExpr URegionProperty() {
    if (MRA_DEBUG) cerr << "URegionProperty() called" << endl;

    ListExpr listrep = nl->TextAtom();
    nl->AppendText(listrep,
		   "(<interval> <face>*), where <interval> is "
		   "(<real> <real> <bool> <bool>) and where "
		   "<face> is (<outercycle> <holecycle>*), "
                   "where <outercycle> and <holecycle> are "
		   "(<real> <real> <real> <real>), representing "
		   "start X, start Y, end X and end Y values.");
    ListExpr example = nl->TextAtom();
    nl->AppendText(example,
		   "((0.0 10.0 TRUE FALSE) (((3 0)(10 1)(3 1))((3.1 0.1)"
		   "(3.1 0.9)(6 0.8))))");
    ListExpr remarks = nl->TextAtom();
    nl->AppendText(remarks,
		   "All <holecycle> must be completely within "
		   "corresponding <outercylce>.");

    return 
	nl->TwoElemList(
            nl->FiveElemList(
		nl->StringAtom("Signature"),
		nl->StringAtom("Example Type List"),
		nl->StringAtom("List Rep"),
		nl->StringAtom("Example List"),
		nl->StringAtom("Remarks")),
            nl->FiveElemList(
		nl->StringAtom("-> UNIT"),
		nl->StringAtom("(uregion)"),
		listrep,
		example,
		remarks));
}

static bool CheckURegion(ListExpr type, ListExpr& errorInfo) {
    if (MRA_DEBUG) cerr << "CheckURegion() called" << endl;

    return nl->IsEqual(type, "uregion");
}

static ListExpr OutURegion(ListExpr typeInfo, Word value) {
     if (MRA_DEBUG) cerr << "OutURegion() called" << endl;

    URegion* ur = (URegion*) value.addr;

    int num = ur->GetSegmentsNum();

    ListExpr faces = nl->TheEmptyList();
    ListExpr facesLastElem = faces;

    ListExpr face = nl->TheEmptyList();
    ListExpr faceLastElem = face;
    ListExpr cycle = nl->TheEmptyList();
    ListExpr cycleLastElem = cycle;

    for (int i = 0; i < num; i++) {
        if (MRA_DEBUG) cerr << "OutURegion() segment #" << i << endl;

	MSegmentData dms;
	ur->GetSegment(i, dms);

        if (MRA_DEBUG) 
            cerr << "OutURegion() point is "
		 << dms.GetFaceNo()
		 << " "
		 << dms.GetCycleNo()
		 << " ("
                 << dms.GetInitialStartX()
                 << ", "
                 << dms.GetInitialStartY()
                 << ", "
                 << dms.GetFinalStartX()
                 << ", "
                 << dms.GetFinalStartY()
                 << ")"
                 << endl;

        ListExpr p = 
            nl->FourElemList(
                nl->RealAtom(dms.GetInitialStartX()),
                nl->RealAtom(dms.GetInitialStartY()),
                nl->RealAtom(dms.GetFinalStartX()),
                nl->RealAtom(dms.GetFinalStartY()));

        if (cycle == nl->TheEmptyList()) {
            if (MRA_DEBUG) cerr << "OutURegion() new cycle" << endl;
            cycle = nl->OneElemList(p);
            cycleLastElem = cycle;
        } else {
            if (MRA_DEBUG) cerr << "OutURegion() existing cycle" << endl;
            cycleLastElem = nl->Append(cycleLastElem, p);
        }

	MSegmentData nextDms;
	if (i < num-1) ur->GetSegment(i+1, nextDms);

        if (i == num-1 || dms.GetCycleNo() != nextDms.GetCycleNo()) {
            if (MRA_DEBUG) cerr << "OutURegion() end of cycle" << endl;

            if (face == nl->TheEmptyList()) {
                if (MRA_DEBUG) cerr << "OutURegion() new face" << endl;
                face = nl->OneElemList(cycle);
                faceLastElem = face;
            } else {
                if (MRA_DEBUG) cerr << "OutURegion() existing face" << endl;
                faceLastElem = nl->Append(faceLastElem, cycle);
            }

            if (i == num-1 || dms.GetFaceNo() != nextDms.GetFaceNo()) {
                if (MRA_DEBUG) cerr << "OutURegion() end of face" << endl;
		if (faces == nl->TheEmptyList()) {
		    faces = nl->OneElemList(face);
		    facesLastElem = faces;
		} else 
		    facesLastElem = nl->Append(facesLastElem, face);
                
                face = nl->TheEmptyList();
                faceLastElem = face;
            }

            cycle = nl->TheEmptyList();
            cycleLastElem = cycle;
        }
    }

    ListExpr res = 
        nl->TwoElemList(
            nl->FourElemList(
                OutDateTime(nl->TheEmptyList(), 
                            SetWord(&ur->timeInterval.start)),
                OutDateTime(nl->TheEmptyList(), 
                            SetWord(&ur->timeInterval.end)),
                nl->BoolAtom(ur->timeInterval.lc),
                nl->BoolAtom(ur->timeInterval.rc)),
	    faces);

    return res;

#ifdef SCHMUH
    URegion* ur = (URegion*) value.addr;

    ListExpr res = 
        nl->OneElemList(
            nl->FourElemList(
                OutDateTime(nl->TheEmptyList(), 
                            SetWord(&ur->timeInterval.start)),
                OutDateTime(nl->TheEmptyList(), 
                            SetWord(&ur->timeInterval.end)),
                nl->BoolAtom(ur->timeInterval.lc),
                nl->BoolAtom(ur->timeInterval.rc)));
    ListExpr resLastElem = res;

    int num = ur->GetPointsNum();
    if (MRA_DEBUG) cerr << "OutURegion() #points=" << num << endl;

    ListExpr face = nl->TheEmptyList();
    ListExpr faceLastElem = face;
    ListExpr cycle = nl->TheEmptyList();
    ListExpr cycleLastElem = cycle;

    MPointData nextDmp;
    if (num > 0) ur->GetPoint(0, nextDmp);

    for (int i = 0; i < num; i++) {
        if (MRA_DEBUG) cerr << "OutURegion() point #" << i << endl;

        MPointData dmp = nextDmp;
        if (i != num-1) ur->GetPoint(i+1, nextDmp);

        if (MRA_DEBUG) 
            cerr << "OutURegion() point is ("
                 << dmp.GetFaceNo()
                 << ", "
                 << dmp.GetCycleNo()
                 << ", "
                 << dmp.GetStartX()
                 << ", "
                 << dmp.GetStartY()
                 << ", "
                 << dmp.GetEndX()
                 << ", "
                 << dmp.GetEndY()
                 << ")"
                 << endl;

        ListExpr p = 
            nl->FourElemList(
                nl->RealAtom(dmp.GetStartX()),
                nl->RealAtom(dmp.GetStartY()),
                nl->RealAtom(dmp.GetEndX()),
                nl->RealAtom(dmp.GetEndY()));

        if (cycle == nl->TheEmptyList()) {
            if (MRA_DEBUG) cerr << "OutURegion() new cycle" << endl;
            cycle = nl->OneElemList(p);
            cycleLastElem = cycle;
        } else {
            if (MRA_DEBUG) cerr << "OutURegion() existing cycle" << endl;
            cycleLastElem = nl->Append(cycleLastElem, p);
        }

        if (i == num-1 || dmp.GetCycleNo() != nextDmp.GetCycleNo()) {
            if (MRA_DEBUG) cerr << "OutURegion() end of cycle" << endl;

            if (face == nl->TheEmptyList()) {
                if (MRA_DEBUG) cerr << "OutURegion() new face" << endl;
                face = nl->OneElemList(cycle);
                faceLastElem = face;
            } else {
                if (MRA_DEBUG) cerr << "OutURegion() existing face" << endl;
                faceLastElem = nl->Append(faceLastElem, cycle);
            }

            if (i == num-1 || dmp.GetFaceNo() != nextDmp.GetFaceNo()) {
                if (MRA_DEBUG) cerr << "OutURegion() end of face" << endl;
                resLastElem = nl->Append(resLastElem, face);
                
                face = nl->TheEmptyList();
                faceLastElem = face;
            }

            cycle = nl->TheEmptyList();
            cycleLastElem = cycle;
        }
    }

    return res;
#endif // SCHMUH
}

/*

The following approach is used to check the validity of the points in
each unit of a moving region:

If $(t_1, t_2, true, true)$ is the interval of the unit,
the moving region is allowed to degenerate in instants $t_1$ and $t_2$. If
the unit's interval is half-open, ie.\ $(t_1, t_2, false, true)$ or 
$(t_1, t_2, true, false)$, the moving region is allowed to generate in 
instants $t_1$ or $t_2$. However, for all all instants $t$ with 
$t_1 < t < t_2$, the moving region must not degenerate. 

Two distinct cases are considered:

\begin{enumerate}
\item
For point intervals $(t, t, true, true)$, the moving region must not
degenerate in instant $t$ and the unit is checked like a spatial region.

\item
For non-point intervals $(t_1, t_2, lc, rc)$ with $t_1 \ne t_2$ and 
$lc, rc \in \{true, false\}$, the value of the unit at instant 
$t = (t2-t1)/2$ is calculated. As the moving region must not degenerate
at instant $t$, this value is checked like a spatial region. If successful,
it is checked that no trapezium, which is spanned by a moving segment in
the unit, is intersecting or touching in its interior any other moving
segment.
\end{enumerate}

The spatial region in both cases is used to calculate the value of the
~insideLeftOrAbove~ attribute of each segment.

*/

// *hm* No verification on data done yet

static Word InURegionEmbedded(const ListExpr typeInfo, 
			      const ListExpr instance,
			      const int errorPos, 
			      ListExpr& errorInfo, 
			      bool& correct,
			      DBArray<MSegmentData>* segments,
			      unsigned int segmentsStartPos) {
    if (MRA_DEBUG) cerr << "InURegionEmbedded() called" << endl;

    if (nl->ListLength(instance) == 0) {
	cerr << "uregion not in format (<interval> <face>*)" << endl;
        correct = false;
        return SetWord(Address(0));
    }

    if (MRA_DEBUG) 
        cerr << "InURegion() (<interval> <face>*) found" << endl;

    ListExpr interval = nl->First(instance);

    if (nl->ListLength(interval) != 4
        || !nl->IsAtom(nl->First(interval))
        || (nl->AtomType(nl->First(interval)) != StringType
            && nl->AtomType(nl->First(interval)) != RealType)
        || !nl->IsAtom(nl->Second(interval))
        || (nl->AtomType(nl->Second(interval)) != StringType
            && nl->AtomType(nl->Second(interval)) != RealType)
        || !nl->IsAtom(nl->Third(interval))
        || nl->AtomType(nl->Third(interval)) != BoolType
        || !nl->IsAtom(nl->Fourth(interval))
        || nl->AtomType(nl->Fourth(interval)) != BoolType) {
	cerr << "uregion interval not in format (<real> <real> <bool> <bool>)" 
	     << endl;
        correct = false;
        return SetWord(Address(0));
    }

    Instant *start = 
        (Instant *) InInstant(nl->TheEmptyList(), 
                              nl->First(interval),
                              errorPos, 
                              errorInfo, 
                              correct).addr;
    if (!correct) {
        cerr << "uregion interval invalid start time" << endl;
        correct = false;
        return SetWord(Address(0));
    }

    Instant *end = 
        (Instant *) InInstant(nl->TheEmptyList(), 
                              nl->Second(interval),
                              errorPos, 
                              errorInfo, 
                              correct ).addr;
    if (!correct) {
        cerr << "uregion interval invalid end time" << endl;
        correct = false;
        delete start;
        return SetWord(Address(0));
    }

    bool lc = nl->BoolValue(nl->Third(interval));
    bool rc = nl->BoolValue(nl->Fourth(interval));

    Interval<Instant> tinterval(*start, *end, lc, rc);

    double intervalLen = end->ToDouble()-start->ToDouble();

    delete start;
    delete end;

    URegion* uregion =
	segments == 0 
	? new URegion(tinterval)
	: new URegion(tinterval, segments, segmentsStartPos);

    unsigned int faceno = 0;
    unsigned int partnerno = 0;
    ListExpr faces = nl->Second(instance);

    CRegion cr(0);
    cr.StartBulkLoad();

    if (nl->ListLength(faces) == 0) {
	cerr << "uregion should contain at least one face" << endl;
	delete uregion;
        correct = false;
        return SetWord(Address(0));
    }

    while (!nl->IsEmpty(faces)) {
        if (MRA_DEBUG) cerr << "InURegion() face #" << faceno << endl;

        ListExpr cycles = nl->First(faces);

	if (nl->ListLength(cycles) == 0) {
	    cerr << "uregion face should contain at least one cycle" << endl;
	    delete uregion;
	    correct = false;
	    return SetWord(Address(0));
	}

        unsigned int cycleno = 0;
	unsigned int pointno;

        while (!nl->IsEmpty(cycles)) {
	    if (MRA_DEBUG) cerr << "InURegion()   cycle #" << cycleno << endl;

	    pointno = 0;

	    CRegion rDir(0);
	    rDir.StartBulkLoad();

            ListExpr cyclepoints = nl->First(cycles);

	    if (nl->ListLength(cyclepoints) < 3) {
		cerr << "uregion cycle should contain at least three points" 
		     << endl;
		delete uregion;
		correct = false;
		return SetWord(Address(0));
	    }
	    
	    ListExpr firstPoint = nl->First(cyclepoints);
	    ListExpr prevPoint = 0;

	    unsigned int initialSegmentsNum = uregion->GetSegmentsNum();

            while (!nl->IsEmpty(cyclepoints)) {
		if (MRA_DEBUG) 
		    cerr << "InURegion()     point #" << pointno << endl;

                ListExpr point = nl->First(cyclepoints);

		if (prevPoint != 0 
		    && !uregion->AddSegment(cr,
					    rDir,
					    faceno, 
					    cycleno, 
					    pointno, 
					    partnerno,
					    intervalLen,
					    prevPoint, 
					    point)) {
		    cerr << "uregion's segment checks failed" << endl;
		    delete uregion;
		    correct = false;
		    return SetWord(Address(0));
		}

		prevPoint = point;
                cyclepoints = nl->Rest(cyclepoints);
                pointno++;
		partnerno++;
	    }

	    if (!uregion->AddSegment(cr,
				     rDir,
				     faceno,
				     cycleno,
				     pointno,
				     partnerno,
				     intervalLen,
				     prevPoint, 
				     firstPoint)) {
		cerr << "uregion's segment checks failed" << endl;
		delete uregion;
		correct = false;
		return SetWord(Address(0));
	    }

	    partnerno++;

	    rDir.EndBulkLoad();

	    bool direction = rDir.GetCycleDirection();

	    int h = cr.Size()-(rDir.Size()*2);
	    int i = initialSegmentsNum;
	    while (h < cr.Size()) {
                CHalfSegment chsInsideAbove;
                bool insideAbove;

                cr.Get(h, chsInsideAbove);

		if (MRA_DEBUG) 
		    cerr << "InURegion() i=" 
			 << i 
			 << " insideAbove=" 
			 << chsInsideAbove.attr.insideAbove 
			 << endl;

                if (direction == chsInsideAbove.attr.insideAbove) 
                    insideAbove = false;
                else
                    insideAbove = true;
                if (cycleno > 0) insideAbove = !insideAbove;

                //chsInsideAbove.attr.insideAbove = insideAbove;
                //cr.UpdateAttr(h, chsInsideAbove.attr);

                //cr.Get(h+1, chsInsideAbove);
                //chsInsideAbove.attr.insideAbove = insideAbove;
                //cr.UpdateAttr(h+1, chsInsideAbove.attr);

		uregion->SetSegmentInsideLeftOrAbove(i, insideAbove);

                h += 2;
		i++;
	    }

            cycles = nl->Rest(cycles);
            cycleno++;
	}

        faces = nl->Rest(faces);
	faceno++;
    }

    cr.EndBulkLoad();

    bool nonTrivialInitial = false;
    bool nonTrivialFinal = false;

    if (MRA_DEBUG) 
	for (int i = 0; i < uregion->GetSegmentsNum(); i++) {
	    MSegmentData dms;

	    uregion->GetSegment(i, dms);

	    cerr << "InURegion() segment #"
		 << i
		 << ": "
		 << dms.faceno
		 << " "
		 << dms.cycleno
		 << " "
		 << dms.segmentno
		 << " i=("
		 << dms.initialStartX
		 << ", "
		 << dms.initialStartY
		 << ", "
		 << dms.initialEndX
		 << ", "
		 << dms.initialEndY
		 << ") f=("
		 << dms.finalStartX
		 << ", "
		 << dms.finalStartY
		 << ", "
		 << dms.finalEndX
		 << ", "
		 << dms.finalEndY
		 << ") flags="
		 << dms.insideLeftOrAbove
		 << " "
		 << dms.pointInitial
		 << " "
		 << dms.pointFinal
		 << " "
		 << dms.degeneratedInitial
		 << " "
		 << dms.degeneratedFinal
		 << endl;
	}

    for (int i = 0; i < uregion->GetSegmentsNum(); i++) {
	    MSegmentData dms;
	    uregion->GetSegment(i, dms);

	    if (!dms.pointInitial && !dms.degeneratedInitial) 
		nonTrivialInitial = true;
	    if (!dms.pointFinal && !dms.degeneratedFinal) 
		nonTrivialFinal = true;
    }

    if (lc && !nonTrivialInitial) {
	cerr << "no non-trivial segments in initial instant but " 
	     << "time interval closed on left side" 
	     << endl;
	delete uregion;
	correct = false;
	return SetWord(Address(0));
    }

    if (rc && !nonTrivialFinal) {
	cerr << "no non-trivial segments in final instant but " 
	     << "time interval closed on right side" 
	     << endl;
	delete uregion;
	correct = false;
	return SetWord(Address(0));
    }

    correct = true;
    return SetWord(Address(uregion));

#ifdef SCHMUH

    unsigned int faceno = 0;
    ListExpr faces = nl->Rest(instance);
    while (!nl->IsEmpty(faces)) {
        if (MRA_DEBUG) cerr << "InURegion() next face" << endl;

        ListExpr cycles = nl->First(faces);
        unsigned int cycleno = 0;
        unsigned int pointno = 0;
        while (!nl->IsEmpty(cycles)) {
            if (MRA_DEBUG) cerr << "InURegion() next cycle" << endl;

            ListExpr cyclepoints = nl->First(cycles);
            while (!nl->IsEmpty(cyclepoints)) {
                if (MRA_DEBUG) cerr << "InURegion() next point" << endl;

                ListExpr point = nl->First(cyclepoints);

                if (nl->ListLength(point) != 4
                    || !nl->IsAtom(nl->First(point))
                    || nl->AtomType(nl->First(point)) != RealType
                    || !nl->IsAtom(nl->Second(point))
                    || nl->AtomType(nl->Second(point)) != RealType
                    || !nl->IsAtom(nl->Third(point))
                    || nl->AtomType(nl->Third(point)) != RealType
                    || !nl->IsAtom(nl->Fourth(point))
                    || nl->AtomType(nl->Fourth(point)) != RealType) {
                    if (MRA_DEBUG) 
                        cerr << "InURegion() "
                             << "point not in format "
                             << "(<real> <real> <real> <real>)" 
                             << endl;
                    correct = false;
                    delete uregion;
                    return SetWord(Address(0));
                }

                uregion->AddPoint(pointno, 
                                  faceno,
                                  cycleno,
                                  nl->RealValue(nl->First(point)),
                                  nl->RealValue(nl->Second(point)),
                                  nl->RealValue(nl->Third(point)),
                                  nl->RealValue(nl->Fourth(point)));

                cyclepoints = nl->Rest(cyclepoints);
                pointno++;
            }

            cycles = nl->Rest(cycles);
            cycleno++;
        }

        faces = nl->Rest(faces);
    }

    int num = uregion->GetPointsNum();
    if (MRA_DEBUG) cerr << "InURegion() #points=" << num << endl;

    if (MRA_DEBUG) cerr << "InURegion() success" << endl;

    correct = true;
    return SetWord(Address(uregion));
#endif // SCHMUH
}

static Word InURegion(const ListExpr typeInfo, 
		      const ListExpr instance,
		      const int errorPos, 
		      ListExpr& errorInfo, 
		      bool& correct) {
    if (MRA_DEBUG) cerr << "InURegion() called" << endl;

    return 
	InURegionEmbedded(typeInfo, 
			  instance, 
			  errorPos, 
			  errorInfo, 
			  correct, 
			  0, 
			  0);
}

static Word CreateURegion(const ListExpr typeInfo) {
    if (MRA_DEBUG) cerr << "CreateURegion() called" << endl;

    assert(false);
}

static void DeleteURegion(Word& w) {
    if (MRA_DEBUG) cerr << "DeleteURegion() called" << endl;

    URegion *ur = (URegion *)w.addr;
    delete ur;
    w.addr = 0;
}

static void CloseURegion(Word& w) {
    if (MRA_DEBUG) cerr << "CloseURegion() called" << endl;

    delete (URegion *) w.addr;
    w.addr = 0;
}

static Word CloneURegion(const Word& w) {
    if (MRA_DEBUG) cerr << "CloneURegion() called" << endl;

    assert(false);
}

static void* CastURegion(void* addr) {
    if (MRA_DEBUG) cerr << "CastURegion() called" << endl;

    assert(false);
}

static int SizeOfURegion() {
    if (MRA_DEBUG) cerr << "SizeOfURegion() called" << endl;

    assert(false);
}

static TypeConstructor uregion(
    "uregion",
    URegionProperty,
    OutURegion, 
    InURegion,
    0, 0, // SaveToList, RestoreFromList
    CreateURegion,
    DeleteURegion,
    0, 0, // open, save
    CloseURegion,
    CloneURegion,
    CastURegion,
    SizeOfURegion,
    CheckURegion,
    0, // predef. pers. function for model
    TypeConstructor::DummyInModel,
    TypeConstructor::DummyOutModel,
    TypeConstructor::DummyValueToModel,
    TypeConstructor::DummyValueListToModel );

// ************************************************************************

/*
1 ~mregion~

1.1 Helper class ~PlaneSweepStopPoint~

*/

class PlaneSweepStopPoint {
private:
    double x;
    double y;
    double partnerX;
    double partnerY;
    bool insideLeftOrAbove;

public:
    PlaneSweepStopPoint(double xx, 
			double yy, 
			double xp, 
			double yp, 
			bool iloa) : 
	x(xx), y(yy), partnerX(xp), partnerY(yp), insideLeftOrAbove(iloa) {}

    double GetX() { return x; }
    double GetY() { return y; }
    double GetPartnerX() { return partnerX; }
    double GetPartnerY() { return partnerY; }
    bool GetInsideLeftOrAbove() { return insideLeftOrAbove; }

    bool operator<(const PlaneSweepStopPoint& p) const {
	return x > p.x || (x == p.x && y > p.y);
    }
};

/*
1.1 Class ~MRegion~

*/

class MRegion : public Mapping<URegion, CRegion> {
private: 
    DBArray<MSegmentData> msegmentdata;

    void AddRefinementUnits(vector<URegion*>& vur,
			    vector<UPoint*>& vup,
			    URegion& ur,
			    UPoint& up,
			    Instant& start,
			    Instant& end,
			    bool lc,
			    bool rc);
    void RefinementPartition(MPoint& mp,
			     vector<URegion*>& vur,
			     vector<UPoint*>& vup);

public:
    MRegion() {
        if (MRA_DEBUG) cerr << "MRegion::MRegion(int) called" << endl;
    }

    MRegion(const int n) :
        Mapping<URegion, CRegion>(n),
	msegmentdata(0) {
        if (MRA_DEBUG) cerr << "MRegion::MRegion(int) called" << endl;
    }

    int NumOfFLOBs();
    FLOB *GetFLOB(const int i);

    void Traversed(void);
    void Intersection(MPoint& mp);

    friend Word InMRegion(const ListExpr typeInfo, 
			  const ListExpr instance,
			  const int errorPos, 
			  ListExpr& errorInfo, 
			  bool& correct);

    void Get(const int i, URegion& ur) {
        if (MRA_DEBUG) 
	    cerr << "MRegion::Get() called i=" << i << endl;

	Mapping<URegion, CRegion>::Get(i, ur);
	ur.SetMSegmentData(&msegmentdata);
    }

/*
For unit testing only.

*/

    int Unittest2(int pos) {
        if (MRA_DEBUG) 
	    cerr << "MRegion::Unittest2() called pos=" << pos << endl;

	if (pos < 0 || pos >= msegmentdata.Size()) return -1;

	MSegmentData dms;
	msegmentdata.Get(pos, dms);

	return dms.GetInsideLeftOrAbove() ? 1 : 0;
    }
};

int MRegion::NumOfFLOBs() {
    if (MRA_DEBUG) cerr << "MRegion::NumOfFLOBs() called" << endl;

    return 2;
}

FLOB* MRegion::GetFLOB(const int i) {
    if (MRA_DEBUG) cerr << "MRegion::GetFLOB() called" << endl;

    assert(i == 0 || i == 1);

    return i == 0 ? Mapping<URegion, CRegion>::GetFLOB(0) : &msegmentdata;
}

static void addSegment(priority_queue<PlaneSweepStopPoint>& pq,
		       double startX, 
		       double startY, 
		       double endX, 
		       double endY, 
		       bool insideLeftOrAbove) {
    cerr << "segment: (" 
	 << startX 
	 << ", " 
	 << startY 
	 << ")-(" 
	 << endX 
	 << ", " 
	 << endY 
	 << ") insideAbove=" 
	 << insideLeftOrAbove 
	 << endl;

    PlaneSweepStopPoint sp1(startX, startY, endX, endY, insideLeftOrAbove);
    PlaneSweepStopPoint sp2(endX, endY, startX, startY, insideLeftOrAbove);

    pq.push(sp1);
    pq.push(sp2);
}


// *hm* no holes covered

void MRegion::Traversed(void) {
    if (MRA_DEBUG) cerr << "MRegion::Traversed() called" << endl;

    assert(false);

#ifdef SCHMUH
    priority_queue<PlaneSweepStopPoint> pq;

    for (int i = 0; i < GetNoComponents(); i++) {
	if (MRA_DEBUG) cerr << "MRegion::Traversed() unit #" << i << endl;

	URegion ur;
	Get(i, ur);

	// *hm* how to handle uregions, which are degenerated during initial
	// instant?

	CRegion cr;
	ur.TemporalFunction(ur.timeInterval.start, cr);

	for (int j = 0; j < cr.Size(); j++) {
	    CHalfSegment chs;
	    cr.Get(j, chs);

	    if (!chs.GetLDP()) continue;

	    if (MRA_DEBUG) 
		cerr << "MRegion::Traversed() chs #" << j << endl;

	    if (MRA_DEBUG) 
		cerr << "MRgion::Traversed()   start ("
		     << chs.GetLP().GetX()
		     << ", "
		     << chs.GetLP().GetY()
		     << ")-("
		     << chs.GetRP().GetX()
		     << ", "
		     << chs.GetRP().GetY()
		     << ") LDP="
		     << chs.GetLDP()
		     << " insideLeftOrAbove="
		     << chs.GetAttr().insideAbove
		     << " partnerno="
		     << chs.GetAttr().partnerno
		     << " edgeno="
		     << chs.GetAttr().edgeno
		     << endl;

	    addSegment(pq,
		       chs.GetLP().GetX(),
		       chs.GetLP().GetY(),
		       chs.GetRP().GetX(),
		       chs.GetRP().GetY(),
		       chs.GetAttr().insideAbove);

	    MPointData dmp1;
	    ur.GetPoint(chs.GetAttr().edgeno, dmp1);

	    MPointData dmp2;
	    if (chs.GetAttr().edgeno+1 == ur.GetPointsNum()) 
		ur.GetPoint(0, dmp2);
	    else
		ur.GetPoint(chs.GetAttr().edgeno+1, dmp2);

	    if (dmp1.GetEndX() > dmp2.GetEndX()
		|| (dmp1.GetEndX() == dmp2.GetEndX()
		    && dmp1.GetEndY() == dmp2.GetEndY())) {
		if (MRA_DEBUG) 
		    cerr << "MRegion::Traversed()   swapping" << endl;

		MPointData dummy = dmp1;
		dmp1 = dmp2;
		dmp2 = dummy;
	    }

	    if (MRA_DEBUG) 
		cerr << "MRegion::Traversed()   end ("
		     << dmp1.GetEndX()
		     << ", "
		     << dmp1.GetEndY()
		     << ")-("
		     << dmp2.GetEndX()
		     << ", "
		     << dmp2.GetEndY()
		     << ")"
		     << endl;

	    if (chs.GetLP().GetY() == chs.GetRP().GetY()) {
		if (MRA_DEBUG) 
		    cerr << "MRegion::Traversed()   horizontal" << endl;

		assert(dmp1.GetEndY() == dmp2.GetEndY());

		if (chs.GetLP().GetY() < dmp1.GetEndY()) {
		    addSegment(pq,
			       chs.GetLP().GetX(),
			       chs.GetLP().GetY(),
			       chs.GetRP().GetX(),
			       chs.GetRP().GetY(),
			       true);
		    addSegment(pq,
			       dmp1.GetEndX(),
			       dmp1.GetEndY(),
			       dmp2.GetEndX(),
			       dmp2.GetEndY(),
			       false);
		    addSegment(pq,
			       chs.GetLP().GetX(),
			       chs.GetLP().GetY(),
			       dmp1.GetEndX(),
			       dmp1.GetEndY(),
			       chs.GetLP().GetX() > dmp1.GetEndX());
		    addSegment(pq,
			       chs.GetRP().GetX(),
			       chs.GetRP().GetY(),
			       dmp2.GetEndX(),
			       dmp2.GetEndY(),
			       chs.GetRP().GetX() <= dmp2.GetEndX());
		} else if (chs.GetLP().GetY() > dmp1.GetEndY()) {
		    addSegment(pq,
			       chs.GetLP().GetX(),
			       chs.GetLP().GetY(),
			       chs.GetRP().GetX(),
			       chs.GetRP().GetY(),
			       false);
		    addSegment(pq,
			       dmp1.GetEndX(),
			       dmp1.GetEndY(),
			       dmp2.GetEndX(),
			       dmp2.GetEndY(),
			       true);
		    addSegment(pq,
			       chs.GetLP().GetX(),
			       chs.GetLP().GetY(),
			       dmp1.GetEndX(),
			       dmp1.GetEndY(),
			       chs.GetLP().GetX() < dmp1.GetEndX());
		    addSegment(pq,
			       chs.GetRP().GetX(),
			       chs.GetRP().GetY(),
			       dmp2.GetEndX(),
			       dmp2.GetEndY(),
			       chs.GetRP().GetX() >= dmp2.GetEndX());
		} else
		    // *hm* what to do here?
		    assert(false);
	    } else {
		if (MRA_DEBUG) 
		    cerr << "MRegion::Traversed()   other" << endl;
// Algorithm:
// 
// 1. Calculate X coordinate x1, where extension of chs touches X axis.
// 2. Calculate X coordinate x2, where extension of dmp1-dmp2 touches X axis.
// 3. If x1 < x2: 
//      insideLeftOrAbove(chs) = !(ascending(chs) || vertical(chs))
//      insideLeftOrAbove(dmp1-dmp2) = ascending(dmp1-dmp2) || vertical(dmp1-dmp2)
// 
//    If x1 > x2: 
//      insideLeftOrAbove(chs) = ascending(chs) || vertical(chs)
//      insideLeftOrAbove(dmp1-dmp2) = !(ascending(dmp1-dmp2) || vertical(dmp1-dmp2))
//      If x1 = x2: ignore this area?
// 
// Line (x1, y1, x2, y2),
// y1+(y2-y1)*t0 = 0 <=> t0 = y1 / (y1-y2),
// extension of line hits X axis in point (x1+(x2-x1)*t0, 0).

		double start_t0 = 
		    chs.GetLP().GetY()/(chs.GetLP().GetY()-chs.GetRP().GetY());
		double start_x0 = 
		    chs.GetLP().GetX()
		    +(chs.GetRP().GetX()-chs.GetLP().GetX())*start_t0;

		if (MRA_DEBUG) 
		    cerr << "MRegion::Traversed()   start_x0=" 
			 << start_x0 
			 << endl;

		double end_t0 = 
		    dmp1.GetEndY()/(dmp1.GetEndY()-dmp2.GetEndY());
		double end_x0 = 
		    dmp1.GetEndX()+(dmp2.GetEndX()-dmp1.GetEndX())*end_t0;

		if (MRA_DEBUG) 
		    cerr << "MRegion::Traversed()   end_x0=" 
			 << end_x0 
			 << endl;

		if (start_x0 < end_x0) {
		    addSegment(pq,
			       chs.GetLP().GetX(),
			       chs.GetLP().GetY(),
			       chs.GetRP().GetX(),
			       chs.GetRP().GetY(),
			       !(chs.GetRP().GetY() > chs.GetLP().GetY()
				 || chs.GetRP().GetX() == chs.GetLP().GetX()));
		    addSegment(pq,
			       dmp1.GetEndX(),
			       dmp1.GetEndY(),
			       dmp2.GetEndX(),
			       dmp2.GetEndY(),
			       dmp2.GetEndY() > dmp1.GetEndY()
			       || dmp1.GetEndX() == dmp2.GetEndX());
		    addSegment(pq,
			       chs.GetLP().GetX(),
			       chs.GetLP().GetY(),
			       dmp1.GetEndX(),
			       dmp1.GetEndY(),
			       chs.GetLP().GetY() < chs.GetRP().GetY());
		    addSegment(pq,
			       chs.GetRP().GetX(),
			       chs.GetRP().GetY(),
			       dmp2.GetEndX(),
			       dmp2.GetEndY(),
			       chs.GetLP().GetY() > chs.GetRP().GetY());
		} else if (start_x0 > end_x0) {
		    addSegment(pq,
			       chs.GetLP().GetX(),
			       chs.GetLP().GetY(),
			       chs.GetRP().GetX(),
			       chs.GetRP().GetY(),
			       chs.GetRP().GetY() > chs.GetLP().GetY()
				 || chs.GetRP().GetX() == chs.GetLP().GetX());
		    addSegment(pq,
			       dmp1.GetEndX(),
			       dmp1.GetEndY(),
			       dmp2.GetEndX(),
			       dmp2.GetEndY(),
			       !(dmp2.GetEndY() > dmp1.GetEndY()
				 || dmp1.GetEndX() == dmp2.GetEndX()));
		    addSegment(pq,
			       chs.GetLP().GetX(),
			       chs.GetLP().GetY(),
			       dmp1.GetEndX(),
			       dmp1.GetEndY(),
			       chs.GetLP().GetY() < chs.GetRP().GetY());
		    addSegment(pq,
			       chs.GetRP().GetX(),
			       chs.GetRP().GetY(),
			       dmp2.GetEndX(),
			       dmp2.GetEndY(),
			       chs.GetLP().GetY() > chs.GetRP().GetY());
		} else
		    // *hm* what to do here?
		    assert(false);

	    }
	}
    }


    while (!pq.empty()) {
	PlaneSweepStopPoint sp = pq.top();

	if (MRA_DEBUG) 
	    cerr << "MRegion::Traversed() stop point (" 
		 << sp.GetX()
		 << ", "
		 << sp.GetY()
		 << ")-("
		 << sp.GetPartnerX()
		 << ", "
		 << sp.GetPartnerY()
		 << ") insideLeftOrAbove="
		 << sp.GetInsideLeftOrAbove()
		 << endl;

	pq.pop();
    }

    assert(false);
#endif // SCHMUH
}

void MRegion::AddRefinementUnits(vector<URegion*>& vur,
				 vector<UPoint*>& vup,
				 URegion& ur,
				 UPoint& up,
				 Instant& start,
				 Instant& end,
				 bool lc,
				 bool rc) {
    if (MRA_DEBUG) cerr << "MRegion::AddRefinementUnits() called" << endl;

    assert(false);

#ifdef SCHMUH

    if (MRA_DEBUG) 
	cerr << "MRegion::AddRefinementUnits() start="
	     << start.ToDouble()
	     << " end="
	     << end.ToDouble()
	     << " lc="
	     << lc
	     << " rc="
	     << rc
	     << endl;

    Interval<Instant> iv(start, end, lc, rc);

    URegion* rur = new URegion(iv);

    double t0 = 
	(start.ToDouble()-ur.timeInterval.start.ToDouble())/
	(ur.timeInterval.end.ToDouble()-ur.timeInterval.start.ToDouble());
    double t1 = 
	(end.ToDouble()-ur.timeInterval.start.ToDouble())/
	(ur.timeInterval.end.ToDouble()-ur.timeInterval.start.ToDouble());

    if (MRA_DEBUG) 
	cerr << "MRegion::AddRefinementUnits() URegion t0="
	     << t0
	     << " t1="
	     << t1
	     << endl;

    for (int i = 0; i < ur.GetPointsNum(); i++) {
	if (MRA_DEBUG) 
	    cerr << "MRegion::AddRefinementUnits() point #" << i << endl;

	MPointData dmp;
	ur.GetPoint(i, dmp);

	rur->AddPoint(
	    i,
	    dmp.GetFaceNo(),
	    dmp.GetCycleNo(),
	    dmp.GetStartX()+(dmp.GetEndX()-dmp.GetStartX())*t0,
	    dmp.GetStartY()+(dmp.GetEndY()-dmp.GetStartY())*t0,
	    dmp.GetStartX()+(dmp.GetEndX()-dmp.GetStartX())*t1,
	    dmp.GetStartY()+(dmp.GetEndY()-dmp.GetStartY())*t1);
    }

    vur.push_back(rur);

    t0 = (start.ToDouble()-up.timeInterval.start.ToDouble())/
	 (up.timeInterval.end.ToDouble()-up.timeInterval.start.ToDouble());
    t1 = (end.ToDouble()-up.timeInterval.start.ToDouble())/
	 (up.timeInterval.end.ToDouble()-up.timeInterval.start.ToDouble());

    if (MRA_DEBUG) 
	cerr << "MRegion::AddRefinementUnits() UPoint t0="
	     << t0
	     << " t1="
	     << t1
	     << endl;

    vup.push_back(
	new UPoint(
	    iv,
	    up.p0.GetX()+(up.p1.GetX()-up.p0.GetX())*t0,
	    up.p0.GetY()+(up.p1.GetY()-up.p0.GetY())*t0,
	    up.p0.GetX()+(up.p1.GetX()-up.p0.GetX())*t1,
	    up.p0.GetY()+(up.p1.GetY()-up.p0.GetY())*t1));
#endif // SCHMUH
}

void MRegion::RefinementPartition(MPoint& mp,
				  vector<URegion*>& vur,
				  vector<UPoint*>& vup) {
    if (MRA_DEBUG) cerr << "MRegion::RefinementPartition() called" << endl;

    int mrUnit = 0;
    int mpUnit = 0;

    URegion ur;
    UPoint up;

    Get(0, ur);
    mp.Get(0, up);

    while (mrUnit < GetNoComponents() && mpUnit < mp.GetNoComponents()) {
	if (MRA_DEBUG) 
	    cerr << "MRegion::Intersection() mrUnit=" 
		 << mrUnit 
		 << " mpUnit=" 
		 << mpUnit 
		 << endl;

	if (ur.timeInterval.start.Compare(&up.timeInterval.start) == 0
	    && ur.timeInterval.end.Compare(&up.timeInterval.end) == 0) {
	    // case 1

	    if (MRA_DEBUG) {
		cerr << "MRegion::Intersection()   ur: |-----|" << endl;
		cerr << "MRegion::Intersection()   up: |-----|" << endl;
	    }

	    AddRefinementUnits(
		vur,
		vup,
		ur,
		up,
		ur.timeInterval.start,
		ur.timeInterval.end,
		ur.timeInterval.lc && up.timeInterval.lc,
		ur.timeInterval.rc && up.timeInterval.rc);

	    if (++mrUnit < GetNoComponents()) Get(mrUnit, ur);
	    if (++mpUnit < mp.GetNoComponents()) mp.Get(mpUnit, up);
	} else if (ur.timeInterval.Inside(up.timeInterval)) {
	    // case 2

	    if (MRA_DEBUG) {
		cerr << "MRegion::Intersection()   ur:  |---|" << endl;
		cerr << "MRegion::Intersection()   up: |-----|" << endl;
	    }

	    AddRefinementUnits(
		vur,
		vup,
		ur,
		up,
		ur.timeInterval.start,
		ur.timeInterval.end,
		ur.timeInterval.lc,
		ur.timeInterval.rc);

	    if (++mrUnit < GetNoComponents()) Get(mrUnit, ur);
	} else if (up.timeInterval.Inside(ur.timeInterval)) {
	    // case 3

	    if (MRA_DEBUG) {
		cerr << "MRegion::Intersection()   ur: |-----|" << endl;
		cerr << "MRegion::Intersection()   up:  |---|" << endl;
	    }

	    AddRefinementUnits(
		vur,
		vup,
		ur,
		up,
		up.timeInterval.start,
		up.timeInterval.end,
		up.timeInterval.lc,
		up.timeInterval.rc);

	    if (++mpUnit < mp.GetNoComponents()) mp.Get(mpUnit, up);
	} else if (ur.timeInterval.Intersects(up.timeInterval)) {
	    if (MRA_DEBUG) 
		cerr << "MRegion::Intersection()   intersect" << endl;

	    if (ur.timeInterval.start.Compare(&up.timeInterval.end) == 0 
		&& ur.timeInterval.lc 
		&& up.timeInterval.rc) {
		// case 4.1

		if (MRA_DEBUG) {
		    cerr << "MRegion::Intersection()   ur:     [---|" << endl;
		    cerr << "MRegion::Intersection()   up: |---]" << endl;
		}

		AddRefinementUnits(
		    vur,
		    vup,
		    ur,
		    up,
		    ur.timeInterval.start,
		    ur.timeInterval.start,
		    true,
		    true);

		if (++mpUnit < mp.GetNoComponents()) mp.Get(mpUnit, up);
	    } else if (ur.timeInterval.end.Compare(&up.timeInterval.start) == 0
		       && ur.timeInterval.rc
		       && up.timeInterval.lc) {
		// case 4.2

		if (MRA_DEBUG) {
		    cerr << "MRegion::Intersection()   ur: |---]" << endl;
		    cerr << "MRegion::Intersection()   up:     [---|" << endl;
		}

		AddRefinementUnits(
		    vur,
		    vup,
		    ur,
		    up,
		    ur.timeInterval.end,
		    ur.timeInterval.end,
		    true,
		    true);

		if (++mrUnit < GetNoComponents()) Get(mrUnit, ur);
	    } else if (ur.timeInterval.start.Compare(
			   &up.timeInterval.start) < 0) {
		// case 4.3

		if (MRA_DEBUG) {
		    cerr << "MRegion::Intersection()   ur: |----|" << endl;
		    cerr << "MRegion::Intersection()   up:    |----|" << endl;
		}

		AddRefinementUnits(
		    vur,
		    vup,
		    ur,
		    up,
		    up.timeInterval.start,
		    ur.timeInterval.end,
		    up.timeInterval.lc,
		    ur.timeInterval.rc);

		if (++mrUnit < GetNoComponents()) Get(mrUnit, ur);
	    } else if (ur.timeInterval.start.Compare(
			   &up.timeInterval.start) == 0) {
		// If the following assertion would not hold, we had 
		// case 2 or 3
		assert(!ur.timeInterval.lc || !up.timeInterval.rc);

		if (ur.timeInterval.end.Compare(&up.timeInterval.end) < 0) {
		    // case 4.4.1

		    if (MRA_DEBUG) {
			cerr << "MRegion::Intersection()   ur: )---|" 
			     << endl;
			cerr << "MRegion::Intersection()   up: (------|" 
			     << endl;
		    }

		    AddRefinementUnits(
			vur,
			vup,
			ur,
			up,
			ur.timeInterval.start,
			ur.timeInterval.end,
			ur.timeInterval.lc && up.timeInterval.lc,
			ur.timeInterval.rc);

		    if (++mrUnit < GetNoComponents()) Get(mrUnit, ur);
		} else {
		    // case 4.4.2

		    // If the following assertion would not hold, we had
		    // case 2 or 3 again.
		    assert(ur.timeInterval.end.Compare(
			       &up.timeInterval.end) > 0);

		    if (MRA_DEBUG) {
			cerr << "MRegion::Intersection()   ur: )------|" 
			     << endl;
			cerr << "MRegion::Intersection()   up: (---|" 
			     << endl;
		    }

		    AddRefinementUnits(
			vur,
			vup,
			ur,
			up,
			ur.timeInterval.start,
			up.timeInterval.end,
			ur.timeInterval.lc && up.timeInterval.lc,
			up.timeInterval.rc);

		    if (++mpUnit < mp.GetNoComponents()) mp.Get(mpUnit, up);
		}
	    } else {
		// case 4.5

		if (MRA_DEBUG) {
		    cerr << "MRegion::Intersection()   ur:    |----|" << endl;
		    cerr << "MRegion::Intersection()   up: |----|" << endl;
		}

		AddRefinementUnits(
		    vur,
		    vup,
		    ur,
		    up,
		    ur.timeInterval.start,
		    up.timeInterval.end,
		    ur.timeInterval.lc,
		    up.timeInterval.rc);

		if (++mpUnit < mp.GetNoComponents()) mp.Get(mpUnit, up);
	    }
	} else if (ur.timeInterval.start.Compare(
		       &up.timeInterval.start) <= 0) {
	    // case 5

	    if (MRA_DEBUG) {
		cerr << "MRegion::Intersection()   ur: |--|" << endl;
		cerr << "MRegion::Intersection()   up:      |--|" << endl;
	    }
	    
	    if (++mrUnit < GetNoComponents()) Get(mrUnit, ur);
	} else {
	    // case 6

	    if (MRA_DEBUG) {
		cerr << "MRegion::Intersection()   ur:      |--|" << endl;
		cerr << "MRegion::Intersection()   up: |--|" << endl;
	    }

	    if (++mpUnit < mp.GetNoComponents()) mp.Get(mpUnit, up);
	}
    }
}

void MRegion::Intersection(MPoint& mp) {
    if (MRA_DEBUG) cerr << "MRegion::Intersection() called" << endl;

    vector<URegion*> vur;
    vector<UPoint*> vup;

    RefinementPartition(mp, vur, vup);

    for (unsigned int i = 0; i < vur.size(); i++) {
	delete vur[i];
	delete vup[i];
    }

    assert(false);
}

/*
1.1 Algebra integration

*/

// *hm* Verify list representation

static ListExpr MRegionProperty() {
    if (MRA_DEBUG) cerr << "MRegionProperty() called" << endl;

    ListExpr listrep = nl->TextAtom();
    nl->AppendText(listrep,
		   "(<interval> <uregion>*), where <interval> is "
		   "(<real> <real> <bool> <bool>) and where "
		   "<uregion> is uregion list representation.");
    ListExpr example = nl->TextAtom();
    nl->AppendText(example,
		   "(((0.0 10.0 TRUE FALSE) (((3 0)(10 1)(3 1))((3.1 0.1)"
		   "(3.1 0.9)(6 0.8))))((10.0 20.0 TRUE FALSE) "
		   "(((0.0 0.0 -1.0 -1.0) (1.0 0.0 2.0 -1.0) "
		   "(1.0 1.0 2.0 2.0) (0.0 1.0 -1.0 2.0)))) "
		   "((20.0 30.0 TRUE FALSE) (((4.0 4.0 3.0 3.0) "
		   "(5.0 4.0 6.0 3.0) (5.0 5.0 6.0 6.0) "
		   "(4.0 5.0 3.0 6.0)))))");

    return 
	nl->TwoElemList(
            nl->FourElemList(
		nl->StringAtom("Signature"),
		nl->StringAtom("Example Type List"),
		nl->StringAtom("List Rep"),
		nl->StringAtom("Example List")),
            nl->FourElemList(
		nl->StringAtom("-> MAPPING"),
		nl->StringAtom("(mregion)"),
		listrep,
		example));
}

static bool CheckMRegion(ListExpr type, ListExpr& errorInfo) {
    if (MRA_DEBUG) cerr << "CheckMRegion() called" << endl;

    return nl->IsEqual(type, "movingregion");
}

static ListExpr OutMRegion(ListExpr typeInfo, Word value) {
    if (MRA_DEBUG) cerr << "OutMRegion() called" << endl;

    MRegion* mr = (MRegion*) value.addr;

    if (mr->IsEmpty()) return (nl->TheEmptyList());

    assert(mr->IsOrdered());

    ListExpr l = nl->TheEmptyList();
    ListExpr lastElem;
    ListExpr unitList;

    for(int i = 0; i < mr->GetNoComponents(); i++) {
	URegion ur;
	mr->Get(i, ur);
	unitList = OutURegion(nl->TheEmptyList(), SetWord(&ur));
	if (l == nl->TheEmptyList()) {
	    l = nl->Cons(unitList, nl->TheEmptyList());
	    lastElem = l;
	} else
	    lastElem = nl->Append(lastElem, unitList);
    }
    return l;
}

Word InMRegion(const ListExpr typeInfo, 
		      const ListExpr instance,
		      const int errorPos, 
		      ListExpr& errorInfo, 
		      bool& correct) {
    if (MRA_DEBUG) cerr << "InMRegion() called" << endl;

    MRegion* mr = new MRegion(0);

    mr->StartBulkLoad();

    ListExpr rest = instance;
    while(!nl->IsEmpty(rest)) {
	ListExpr first = nl->First(rest);
	rest = nl->Rest(rest);

	URegion* ur = 
	    (URegion*) InURegionEmbedded(
		nl->TheEmptyList(), 
		first,
		errorPos, 
		errorInfo,
		correct,
		&mr->msegmentdata,
		mr->msegmentdata.Size()).addr;
	if(correct == false) return SetWord(Address(0));
	mr->Add(*ur);
	delete ur;
    }

    mr->EndBulkLoad(true);

    if (mr->IsValid()) {
	correct = true;
	return SetWord(mr);
    } else {
	correct = false;
	mr->Destroy();
	delete mr;
	return SetWord(0);
    }
}

static TypeConstructor mregion(
    "movingregion",
    MRegionProperty,
    OutMRegion, 
    InMRegion,
    0, 0, // SaveToList, RestoreFromList
    CreateMapping<MRegion>, DeleteMapping<MRegion>,
    0, 0, // open, save
    CloseMapping<MRegion>, CloneMapping<MRegion>,
    CastMapping<MRegion>,
    SizeOfMapping<MRegion>,
    CheckMRegion,
    0, // predef. pers. function for model
    TypeConstructor::DummyInModel,
    TypeConstructor::DummyOutModel,
    TypeConstructor::DummyValueToModel,
    TypeConstructor::DummyValueListToModel);

// ************************************************************************

/*
1 Operators

1.1 Dummy model mapping

*/

static Word NoModelMapping(ArgVector arg, Supplier opTreeNode) {
    if (MRA_DEBUG) cerr << "NoModelMapping() called" << endl;

    return SetWord(Address(0));
}

static ModelMapping nomodelmap[] = { NoModelMapping };

/*
1.1 Type mapping functions

*/

// used by intersection

static ListExpr MPointMRegionToMPointTypeMap(ListExpr args) {
    if (MRA_DEBUG) cerr << "MPointMRegionToMPointTypeMap() called" << endl;

    cerr << nl->SymbolValue(nl->First(args)) << endl;
    cerr << nl->SymbolValue(nl->Second(args)) << endl;

    if (nl->ListLength(args) == 2 
	&& nl->IsEqual(nl->First(args), "mpoint")
	&& nl->IsEqual(nl->Second(args), "movingregion"))
	return nl->SymbolAtom("mpoint");
    else
	return nl->SymbolAtom("typeerror");
}

// used by inside

static ListExpr MPointMRegionToMBoolTypeMap(ListExpr args) {
    if (MRA_DEBUG) cerr << "MPointMRegionToMBoolTypeMap() called" << endl;

    if (nl->ListLength(args) == 2 
	&& nl->IsEqual(nl->First(args), "mpoint")
	&& nl->IsEqual(nl->Second(args), "movingregion"))
	return nl->SymbolAtom("mbool");
    else
	return nl->SymbolAtom("typeerror");
}

// used by atinstant

static ListExpr MRegionInstantToIRegionTypeMap(ListExpr args) {
    if (MRA_DEBUG) cerr << "MRegionInstantToIRegionTypeMap() called" << endl;

    if (nl->ListLength(args) == 2 
	&& nl->IsEqual(nl->First(args), "movingregion")
	&& nl->IsEqual(nl->Second(args), "instant"))
	return nl->SymbolAtom("iregion");
    else
	return nl->SymbolAtom("typeerror");
}

// used by traversed

static ListExpr MRegionToRegionTypeMap(ListExpr args) {
    if (MRA_DEBUG) cerr << "MRegionToRegionTypeMap() called" << endl;

    if (nl->ListLength(args) == 1 
	&& nl->IsEqual(nl->First(args), "movingregion"))
	return nl->SymbolAtom("region");
    else
	return nl->SymbolAtom("typeerror");
}

// used by initial, final

static ListExpr MRegionToIRegionTypeMap(ListExpr args) {
    if (MRA_DEBUG) cerr << "MRegionToIRegionTypeMap() called" << endl;

    if (nl->ListLength(args) == 1 
	&& nl->IsEqual(nl->First(args), "movingregion"))
	return nl->SymbolAtom("iregion");
    else
	return nl->SymbolAtom("typeerror");
}

// used by deftime

static ListExpr MRegionToPeriodsTypeMap(ListExpr args) {
    if (MRA_DEBUG) cerr << "MRegionToPeriodsTypeMap() called" << endl;

    if (nl->ListLength(args) == 1 
	&& nl->IsEqual(nl->First(args), "movingregion"))
	return nl->SymbolAtom("periods");
    else
	return nl->SymbolAtom("typeerror");
}

// used by inst

static ListExpr IRegionToInstantTypeMap(ListExpr args) {
    if (MRA_DEBUG) cerr << "IRegionToInstantTypeMap() called" << endl;

    if (nl->ListLength(args) == 1 
	&& nl->IsEqual(nl->First(args), "iregion"))
	return nl->SymbolAtom("instant");
    else
	return nl->SymbolAtom("typeerror");
}

// used by val

static ListExpr IRegionToRegionTypeMap(ListExpr args) {
    if (MRA_DEBUG) cerr << "IRegionToRegionTypeMap() called" << endl;

    if (nl->ListLength(args) == 1 
	&& nl->IsEqual(nl->First(args), "iregion"))
	return nl->SymbolAtom("region");
    else
	return nl->SymbolAtom("typeerror");
}

// used by present

static ListExpr PresentTypeMap(ListExpr args) {
    if (MRA_DEBUG) cerr << "PresentTypeMap() called" << endl;

    if (nl->ListLength(args) == 2 
	&& nl->IsEqual(nl->First(args), "movingregion")
	&& (nl->IsEqual(nl->Second(args), "instant")
	    || nl->IsEqual(nl->Second(args), "periods"))) 
	return nl->SymbolAtom("bool");
    else
	return nl->SymbolAtom("typeerror");
}

/*
Used for unit testing only.

*/

static ListExpr Unittest1TypeMap(ListExpr args) {
    if (MRA_DEBUG) cerr << "Unittest1TypeMap() called" << endl;

    if (nl->ListLength(args) != 17) return nl->SymbolAtom("typeerror");

    for (; !nl->IsEmpty(args); args = nl->Rest(args))
	if (nl->SymbolValue(nl->First(args)) != "real") 
	    return nl->SymbolAtom("typeerror");

    return nl->SymbolAtom("int");
}

static ListExpr Unittest2TypeMap(ListExpr args) {
    if (MRA_DEBUG) cerr << "Unittest2TypeMap() called" << endl;

    if (nl->ListLength(args) == 2 
	&& nl->IsEqual(nl->First(args), "movingregion")
	&& nl->IsEqual(nl->Second(args), "int")) 
	return nl->SymbolAtom("int");
    else
	return nl->SymbolAtom("typeerror");
}

/*
1.1 Selection functions

*/

// used by intersection, inside

static int MPointMRegionSelect(ListExpr args) {
    if (MRA_DEBUG) cerr << "MPointMRegionSelect() called" << endl;

    if (nl->ListLength(args) == 2
	&& nl->SymbolValue(nl->First(args)) == "mpoint"
	&& nl->SymbolValue(nl->Second(args)) == "movingregion")
	return 0;
    else
	return -1;
}

// used by initial, final, deftime, traversed

static int MRegionSelect(ListExpr args) {
    if (MRA_DEBUG) cerr << "MRegionSelect() called" << endl;

    if (nl->ListLength(args) == 1
	&& nl->SymbolValue(nl->First(args)) == "movingregion")
	return 0;
    else
	return -1;
}

// used by atinstant

static int MRegionInstantSelect(ListExpr args) {
    if (MRA_DEBUG) cerr << "MRegionInstantSelect() called" << endl;

    if (nl->ListLength(args) == 2
	&& nl->SymbolValue(nl->First(args)) == "movingregion"
	&& nl->SymbolValue(nl->Second(args)) == "instant")
	return 0;
    else
	return -1;
}

// used by inst, val

static int IRegionSelect(ListExpr args) {
    if (MRA_DEBUG) cerr << "IRegionSelect() called" << endl;

    if (MRA_DEBUG) 
	cerr << "IRegionSelect() len=" << nl->ListLength(args) << endl;
    if (MRA_DEBUG) 
	cerr << "IRegionSelect() symbolvalue(first)=" 
	     << nl->SymbolValue(nl->First(args)) 
	     << endl;

    if (nl->ListLength(args) == 1
	&& nl->SymbolValue(nl->First(args)) == "iregion")
	return 0;
    else
	return -1;
}

static int PresentSelect(ListExpr args) {
    if (MRA_DEBUG) cerr << "PresentSelect() called" << endl;

    if (nl->ListLength(args) == 2
	&& nl->SymbolValue(nl->First(args)) == "movingregion"
	&& nl->SymbolValue(nl->Second(args)) == "instant")
	return 0;
    else if (nl->ListLength(args) == 2
	&& nl->SymbolValue(nl->First(args)) == "movingregion"
	&& nl->SymbolValue(nl->Second(args)) == "periods")
	return 1;
    else
	return -1;
}

/*
1.1 Value mapping functions

*/

static int TraversedValueMap(Word* args, 
			     Word& result, 
			     int message, 
			     Word& local, 
			     Supplier s) {
    if (MRA_DEBUG) cerr << "TraversedValueMap() called" << endl;

    ((MRegion*) args[0].addr)->Traversed();
    
    assert(false);
}


static int IntersectionValueMap(Word* args, 
				Word& result, 
				int message, 
				Word& local, 
				Supplier s) {
    if (MRA_DEBUG) cerr << "IntersectionValuMap() called" << endl;

    ((MRegion*) args[1].addr)->Intersection(* (MPoint*) args[0].addr);

    assert(false);
}

static int InsideValueMap(Word* args, 
			  Word& result, 
			  int message, 
			  Word& local, 
			  Supplier s) {
    if (MRA_DEBUG) cerr << "InsideValuMap() called" << endl;

    assert(false);
}

/*
Used for unit testing only.

*/

static int Unittest1ValueMap(Word* args, 
			     Word& result, 
			     int message, 
			     Word& local, 
			     Supplier s) {
    if (MRA_DEBUG) cerr << "Unittest1ValueMap() called" << endl;

    unsigned int detailedResult;

    specialTrapeziumIntersects(
	((CcReal*) args[0].addr)->GetRealval(),  // dt
	((CcReal*) args[1].addr)->GetRealval(),  // t1p1x
	((CcReal*) args[2].addr)->GetRealval(),  // t1p1y
	((CcReal*) args[3].addr)->GetRealval(),  // t1p2x
	((CcReal*) args[4].addr)->GetRealval(),  // t1p2y
	((CcReal*) args[5].addr)->GetRealval(),  // t1p3x
	((CcReal*) args[6].addr)->GetRealval(),  // t1p3y
	((CcReal*) args[7].addr)->GetRealval(),  // t1p4x
	((CcReal*) args[8].addr)->GetRealval(),  // t1p4y
	((CcReal*) args[9].addr)->GetRealval(),  // t2p1x
	((CcReal*) args[10].addr)->GetRealval(), // t2p1y
	((CcReal*) args[11].addr)->GetRealval(), // t2p2x
	((CcReal*) args[12].addr)->GetRealval(), // t2p2y
	((CcReal*) args[13].addr)->GetRealval(), // t2p3x
	((CcReal*) args[14].addr)->GetRealval(), // t2p3y
	((CcReal*) args[15].addr)->GetRealval(), // t2p4x
	((CcReal*) args[16].addr)->GetRealval(), // t2p4y
	detailedResult);

    if (MRA_DEBUG) cerr << "Unittest1ValueMap() #1" << endl;

    result = qp->ResultStorage(s);
    ((CcInt *)result.addr)->Set(true, detailedResult);

    if (MRA_DEBUG) cerr << "Unittest1ValueMap() #2" << endl;

    return 0;
}

static int Unittest2ValueMap(Word* args, 
			     Word& result, 
			     int message, 
			     Word& local, 
			     Supplier s) {
    if (MRA_DEBUG) cerr << "Unittest2ValueMap() called" << endl;

    result = qp->ResultStorage(s);
    ((CcInt *)result.addr)->Set(
	true, 
	((MRegion*) args[0].addr)->Unittest2(
	    ((CcInt*) args[1].addr)->GetIntval()));

    return 0;
}


/*
1.1 Value mapping arrays

*/

// *hm* remove value mapping array from non-overloaded operators

static ValueMapping atinstantvaluemap[] = 
    { MappingAtInstant<MRegion, CRegion> };

static ValueMapping initialvaluemap[] = 
    { MappingInitial<MRegion, URegion, CRegion> };

static ValueMapping finalvaluemap[] = 
    { MappingFinal<MRegion, URegion, CRegion> };

static ValueMapping instvaluemap[] = 
    { IntimeInst<CRegion> };

static ValueMapping valvaluemap[] = 
    { IntimeVal<CRegion> };

static ValueMapping deftimevaluemap[] = 
    { MappingDefTime<MRegion> };

static ValueMapping presentvaluemap[] = 
    { MappingPresent_i<MRegion>,
      MappingPresent_p<MRegion> };

static ValueMapping traversedvaluemap[] = 
    { TraversedValueMap };

static ValueMapping intersectionvaluemap[] = 
    { IntersectionValueMap };

static ValueMapping insidevaluemap[] = 
    { InsideValueMap };

/*
1.1 Operator specifications

*/

static const string atinstantspec  = 
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "  ( <text>(mregion instant) -> iregion</text--->"
    "    <text>_ atinstant _ </text--->"
    "    <text>Get the iregion value corresponding to the instant.</text--->"
    "    <text>mregion1 atinstant instant1</text---> ) )";

static const string initialspec = 
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "  ( <text>mregion -> iregion</text--->"
    "    <text>initial( _ )</text--->"
    "    <text>Get the iregion value corresponding to the initial instant."
    "    </text--->"
    "    <text>initial( mregion1 )</text---> ) )";

static const string finalspec = 
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "  ( <text>mregion -> iregion</text--->"
    "    <text>final( _ )</text--->"
    "    <text>Get the iregion value corresponding to the final instant."
    "    </text--->"
    "    <text>final( mregion1 )</text---> ) )";

static const string instspec = 
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "  ( <text>iregion -> instant</text--->"
    "    <text>inst ( _ )</text--->"
    "    <text>iregion time instant.</text--->"
    "    <text>inst ( iregion1 )</text---> ) )";

static const string valspec =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "  ( <text>iregion -> region</text--->"
    "    <text>val ( _ )</text--->"
    "    <text>Intime value.</text--->"
    "    <text>val ( iregion1 )</text---> ) )";

static const string deftimespec = 
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "  ( <text>mregion -> periods</text--->"
    "    <text>deftime( _ )</text--->"
    "    <text>Get the defined time of the mregion object.</text--->"
    "    <text>deftime( mregion1 )</text---> ) )";

static const string presentspec = 
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "  ( <text>(mregion instant) -> bool, (mregion periods) -> bool</text--->"
    "    <text>_ present _ </text--->"
    "    <text>Whether the object is present at the given instant or"
    "    period.</text--->"
    "    <text>mregion1 present instant1</text---> ) )";

static const string traversedspec = 
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "  ( <text>mregion -> region</text--->"
    "    <text>traversed( _ )</text--->"
    "    <text>Projection of a moving region into the plane.</text--->"
    "    <text>traversed(mregion1)</text---> ) )";

static const string intersectionspec = 
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "  ( <text>(mpoint mregion) -> mpoint</text--->"
    "    <text>_ intersection _</text--->"
    "    <text>Intersection between mpoint and mregion.</text--->"
    "    <text>mpoint1 intersection mregion1</text---> ) )";

static const string insidespec = 
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "  ( <text>(mpoint mregion) -> mbool</text--->"
    "    <text>_ intersection _</text--->"
    "    <text>Calculates if and when mpoint is inside mregion.</text--->"
    "    <text>mpoint1 inside mregion1</text---> ) )";

/*
Used for unit testing only.

*/

static const string unittestspec = 
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "  ( <text>unit testing only</text--->"
    "    <text>unit testing only</text--->"
    "    <text>unit testing only</text--->"
    "    <text>unit testing only</text---> ) )";

/*
1.1 Operator creation

*/

static Operator atinstant("atinstant",
			  atinstantspec,
			  1,
			  atinstantvaluemap,
			  nomodelmap,
			  MRegionInstantSelect,
			  MRegionInstantToIRegionTypeMap);

static Operator initial("initial",
			initialspec,
			1,
			initialvaluemap,
			nomodelmap,
			MRegionSelect,
			MRegionToIRegionTypeMap);

static Operator final("final",
		      finalspec,
		      1,
		      finalvaluemap,
		      nomodelmap,
		      MRegionSelect,
		      MRegionToIRegionTypeMap);

static Operator inst("inst",
		     instspec,
		     1,
		     instvaluemap,
		     nomodelmap,
		     IRegionSelect,
		     IRegionToInstantTypeMap);

static Operator val("val",
		    valspec,
		    1,
		    valvaluemap,
		    nomodelmap,
		    IRegionSelect,
		    IRegionToRegionTypeMap);

static Operator deftime("deftime",
			deftimespec,
			1,
			deftimevaluemap,
			nomodelmap,
			MRegionSelect,
			MRegionToPeriodsTypeMap);

static Operator present("present",
			presentspec,
			2,
			presentvaluemap,
			nomodelmap,
			PresentSelect,
			PresentTypeMap);

static Operator traversed("traversed",
			  traversedspec,
			  1,
			  traversedvaluemap,
			  nomodelmap,
			  MRegionSelect,
			  MRegionToRegionTypeMap);

static Operator intersection("intersection",
			     intersectionspec,
			     1,
			     intersectionvaluemap,
			     nomodelmap,
			     MPointMRegionSelect,
			     MPointMRegionToMPointTypeMap);

static Operator inside("inside",
		       insidespec,
		       1,
		       insidevaluemap,
		       nomodelmap,
		       MPointMRegionSelect,
		       MPointMRegionToMBoolTypeMap);

/*
Used for unit testing only.

*/

static Operator unittest1("unittest1",
			  unittestspec,
			  Unittest1ValueMap,
			  Operator::DummyModel,
			  simpleSelect,
			  Unittest1TypeMap);
static Operator unittest2("unittest2",
			  unittestspec,
			  Unittest2ValueMap,
			  Operator::DummyModel,
			  simpleSelect,
			  Unittest2TypeMap);

// ************************************************************************

/*
1 Algebra creation

*/

class MovingRegionAlgebra : public Algebra {
public:
    MovingRegionAlgebra() : Algebra() {
	AddTypeConstructor(&iregion);
	AddTypeConstructor(&uregion);
	AddTypeConstructor(&mregion);

	iregion.AssociateKind("TEMPORAL");
	iregion.AssociateKind("DATA");

	uregion.AssociateKind("TEMPORAL");
	uregion.AssociateKind("DATA");
	uregion.AssociateKind("SPATIAL3D");

	mregion.AssociateKind("TEMPORAL");
	mregion.AssociateKind("DATA");

	AddOperator(&atinstant);
	AddOperator(&initial);
	AddOperator(&final);
	AddOperator(&inst);
	AddOperator(&val);
	AddOperator(&deftime);
	AddOperator(&present);
	AddOperator(&traversed);
	AddOperator(&intersection);
	AddOperator(&inside);

/*
Used for unit testing only.

*/
	AddOperator(&unittest1);
	AddOperator(&unittest2);
    }
    ~MovingRegionAlgebra() {}
};

static MovingRegionAlgebra movingRegionAlgebra;

extern "C"
Algebra* InitializeMovingRegionAlgebra(NestedList* nlRef, 
				      QueryProcessor *qpRef) {
    nl = nlRef;
    qp = qpRef;
    return &movingRegionAlgebra;
}
