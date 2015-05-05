/*

This class is a FixedMRegion.

*/
#ifndef __FIXEDMREGION_H
#define __FIXEDMREGION_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "Secondo_Include.h"
#include "MovingRegionAlgebra.h"
#include "Move.h"
#include "LATransform.h"

class FixedMRegion
{
public:
/*
This is the default constructor. Do not use.

*/

  FixedMRegion ();
/*
This is the copy constructor.

*/

  FixedMRegion (const FixedMRegion & f);

/*
This is the constructor that gets necessary information. 
region: object that shall be moved
move: the move object
rot\_center: the center of the rotation. It will be moved with the object.
starttime: the start time of the movement

*/
    FixedMRegion (Region * _region, const Move & _move,
                  const Point & rot_center, double _starttime);
/*
deprecated
This is the constructor that should not be used externally. It gets necessary information. 
\_t: never used
\_xm: x-value of the rotation center, that will be moved, too
\_ym: y-value of the rotation center, that will be moved, too
\_r: the region to be moved
\_x0: the x-startvalue of the linear movement
\_y0: the y-startvalue of the linear movement
\_alpha0: the starting angle
\_vx: x-value of the linear movement
\_vy: y-value of the linear movement
\_valpha: the angle of movement

*/
    FixedMRegion (double _t, double _xm, double _ym,
                  Region * _r, double _x0, double _y0, double _alpha0,
                  double _vx, double _vy, double _valpha);


/*
This is the constructor that should be used externally. It gets necessary information.
region: object that shall be moved
start: point at which the object ist placed at the beginning $t=0$
alpha\_start: angle of object at the bginning $t=0$
speed: speed or velociy (v) of the movement
alpha\_speed: speed or velociy (v) of the angle movement
rot\_center: the rotational center of the object
starttime: the start time of the movement

*/
    FixedMRegion (Region * _region, const Point & _start,
                  double alpha_start, const Point & _speed,
                  double alpha_speed, const Point & rot_center,
                  double _starttime = 0.0);

/*
This is the standard destructor.

*/
   ~FixedMRegion ();

/*
This is a method that accepts a list of regions. The regions represent 
spots and the movement will be calculated. The constructor expects identical 
regions that can be transformed by a translation or rotation. The region itself
 cannot cahnge its shape.

*/
  Region interpolate (Region * spots);
//TODO

/*
This method will return a region that the FMRegion will have at the given 
time ti.

*/
  Region *atinstant (double ti);

/*
This method will calculate a MPoint with which is defined, if the MPoint mp 
is inside the FMRegion at the given time and else not defined. The MPoint 
will be calculated for the time intervall ta to te.

*/
  MBool inside (const MPoint & mp);


/*
This method will calculate a MPoint which is defined, if the MPoint mp 
intersects with the FMRegion at the given time and else not defined. The 
MPoint will be calculated for the time intervall ta to te.

*/
  MPoint intersection (MPoint & mp);

/*
This method will calculate the Region which contains all points / areas, that
the FMRegion has at least on time (or more often) traversed in the given 
intervall ta to te. 
deprecated!

*/
  Region *traversed2 (double ta, double te, double precision = 0.001);
/*
This method will calculate the Region which contains all points / areas, that
the FMRegion has at least on time (or more often) traversed in the given 
intervall ta to te. 

*/
Region *traversed (double ta, double te, double precision = 0.001);
/*
This method returns the non moving region.

*/
  const Region *getRegion () const;

/*
This method sets the non moving region.

*/
  void setRegion (Region * _r);

/*
This method returns the time t.
not used

*/ 
  const double gett () const;

/*
This method sets the time t.
not used

*/ 
  void sett (double _t);

private:

  double t;//time, never used
  Move m;
  Region *r;//non moving region
  LATransform l;
  double xm;//rotational center
  double ym;//rotational center

/*
This method calculates the 
intersection point of the given lines p1p2 and p3p4.

*/
  Point getIntersectionPoint (const Point & p1, const Point & p2,
                              const Point & p3, const Point & p4);

/*
This methods calculates the case of the intersection type. There are
a lot of different cases. This method will find out, which case the 
two lines p1p2 and p3p4 belong to. The cases have got a short explanation 
in getTraversedArea. A two digit value in a number system of base 4 is 
the result.

*/
  int getTraversedCase (const Point & p1, const Point & p2,
                        const Point & p3, const Point & p4);
/*
This methods finds ou the correct case and calls other methods to calculate 
the polygon that was traversed in this step. It uses the halfsegments' start 
and end position. It has to deal with al lot of different cases.

*/
    vector < vector < Point > >getTraversedArea (const HalfSegment & hsold,
                                                 const HalfSegment & hsnew);

/*
This method orientates the given polygons clockwise. Therefore, they will be 
faces.

*/
  void traversedCreateCyclesNotHoles (vector < vector < Point > >&v_list);
/*
This methods creates a region that contains the traversed area between the 
step's region at t\_step\_start and t\_step\_end.

*/
  Region *getDiffRegion (const vector < HalfSegment > *resultold,
                         const vector < HalfSegment > *resultnew);
/*
This methods creates a polygon of the given Points.
This method adds four edges, the given points, to a vector. This is a 
preparation to create a quadrangle.

*/
vector < vector < Point > >traversedCalculateQuadrangle (const Point & p1, 
const Point & p2, const Point & p3, const Point & p4);
/*
This methods creates a polygon of the given Points.
This method adds three edges, the given points, to a vector. This is a 
preparation to create a triangle.

*/
 vector < vector < Point > >traversedCalculateTriangle (const Point & p1,
 const Point & p2,const Point & p3);

/*
This is an Internal method that is used from traversedCalculateTriangle.
This methods creates a polygon of the given Points.
This method adds three edges, the given points, to a vector. This is a 
preparation to create a triangle.

*/
vector < Point > traversedCalcTriangle (const Point & p1,
                                            const Point & p2, const Point & p3);
/*
This methods creates two polygons of the given Points, using the intersection
of the lines p1p2 and p3p4.
It will prepare two vectors with triangles and gives them back in a vectorvector.

*/
    vector < vector < Point > >traversedCalculateTwoTriangles (Point p1,
    Point p2, Point p3, Point p4);
/*
This method creates a vectorvector and puts the given vector into it.

*/
    vector < vector < Point > >traversedGetVectorVector (vector < Point > v);
/*
This method calculates the internal variables. It is used, when
the time has changed.

*/
void calculateInternalVars ();
/*
This method returns the LATransform.

*/
const LATransform & getLATransform ();
/*
This method sets the LATransform.

*/
void setLATransform (const LATransform & _l);
/*
This method returns the Move.

*/
const Move & getMove ();
/*
This method sets the Move.

*/
void setMove (const Move & _m);

/*
This method will return a list of Halfsegments that the FMRegion will have at the given 
time ti.

*/
vector < HalfSegment > *atinstant (double ti, const vector < HalfSegment > &v);
/*
This method extracts a list of halfsegments from the region.

*/
 vector < HalfSegment > getHSFromRegion ();
/*
This method calculates the aprroximated movement of the given MPoint
with the given precision unter the condition that the FixedMRegion does
not move. Therefore, it uses an inverse movement of the Region and lets
the MPoint move that way in addition to its own movement.

*/
  MPoint approxMovement (const MPoint & p, double precision) const;
/*
This method moves the given Point inverse to the movement of the FixedMRegion
and gives it back.

*/
 Point getInv(double t, const Point & p) const;
/*
This method creates a non moving MRegion.

*/
 MRegion *buildMovingRegion ();
/*
This method will remove the inverse movement of the region from the given 
MPoint. This movement of the Region should have been added some steps 
before from approxMovement.

*/
 MPoint reverseMovement (const MPoint & mp) const;
/*
This method moves the given Point according to the movement of the FixedMRegion
and gives it back.

*/
 Point getTransPoint (DateTime t, const Point & p) const;
};

#endif
