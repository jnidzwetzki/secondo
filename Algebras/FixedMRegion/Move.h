/*
This class calculates the movements.

*/
#ifndef __MOVE_H
#define __MOVE_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "DateTime.h"
#include "Secondo_Include.h"
class Move
{
public:
/*
This is the constructor. Do not use.

*/
  Move ();
/*
This is the copy constructor.

*/
  Move (const Move & _m);
/*
This constructor receives a starting point (x0, y0), a starting angle alpha0, 
and a moving vector (vx, vy) and a moving angle.

*/
    Move (double x0, double y0, double alpha0, double vx, double vy,
          double valpha);

/*
This constructor receives a starting point (xa, ya), a starting angle alphaa,
a start time ta, a moving vector (xe, ye), a moving angle alphae and
an end time te.

*/
    Move (double xa, double ya, double alphaa, double ta, double xe,
          double ye, double alphae, double te);

/*
This constructor receives a starting point (xa, ya), a starting angle alphaa,
a moving vector (xe, ye), a moving angle alphae and time t as Datetime.

*/
    Move (double xa, double ya, double alphaa, DateTime ta, double xe,
          double ye, double alphae, DateTime te);
/*
This is the destructor.

*/
   ~Move ();
/*
This method calculates the necessary set of (x,y) and angle alpha for a
given time t. relatime is not used.

*/
  double *attime (double t, bool relatime) const;
/*
This method calculates the necessary set of (x,y) and angle
alpha for a given time t as an absolute value.

*/
  double *attime (double t) const;

  double *attime (DateTime t) const;
/*
This method returns the start time of the valid interval as DateTime.

*/
  DateTime getStart () const;
/*
This method returns the start time of the valid interval as an double
as absolute time.

*/
  double getStart (const double dummy) const;
/*
This method returns the end time of the valid interval as DateTime.

*/
  DateTime getEnd () const;
/*
This method returns the end time of the valid interval as an double
as absolute time.

*/
  double getEnd (const double dummy) const;

private:
  double x_0;
  double y_0;
  double alpha_0;
  double v_x;
  double v_y;
  double v_alpha;
  DateTime t_a;
  DateTime t_e;
/*
This method converts the given Datetime time into double.

*/
  double convert (DateTime t) const;
/*
This method converts the given double time into Datetime.

*/
  DateTime convert (double t) const;
};
#endif
