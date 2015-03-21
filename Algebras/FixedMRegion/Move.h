/*
This class calculates the movements.

*/
#ifndef __MOVE_H
#define __MOVE_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "Move.h"
class Move
{
  public:
/*
This is the copy constructor.

*/
    Move(const Move &_m);
/*
This constructor receives a starting point (x0, y0), a starting angle alpha0, 
and a moving vector (vx, vy) and a moving ankle.

*/
    Move(double x0, double y0, double alpha0, double vx, double vy, 
 double valpha);

    
/*
This is the destructor.

*/
    ~Move();
/*
This method calculates the necessary set of (x,y) and angla alpha for a
given time t.

*/    
    double* attime(double t);
    
private:
   double x_0;
   double y_0;
   double alpha_0;
   double v_x;
   double v_y;
   double v_alpha;
};
#endif