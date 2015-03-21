/*

This class calculates those values, that are fixed, as soon as its constructor is called.

*/
#ifndef __LATRANSFORM_H
#define __LATRANSFORM_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "LATransform.h"
class LATransform
{
  public:
/*
This is the constructor. It gets a linear movement (x,y), the middle of a circle (xm, ym) and an ankle alpha. 
This is possible because another class knows t and therefore does already know alpha, x and y. 
Later on, the methods of this class can be called for various points, without the already given information. 

*/  
    LATransform(double x, double y, double xm, double ym, double alpha);
/*
This is the standard constructor.

*/
    LATransform(): a00(1), a01(0), a10(0), a11(1), cx(0), cy(0){};
/*
This is the standard destructor.

*/
    ~LATransform();
/*
This method calculates the new x value that the given point will get after its movement.

*/
    double getImgX(double x, double y);
/*
This method calculates the new y value that the given point will get after its movement.

*/
      double getImgY(double x, double y);
    
private:
   double a00, a01, a10, a11;
   double cx, cy;
}; 
#endif