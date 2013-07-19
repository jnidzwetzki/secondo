/*
 ----
 This file is part of SECONDO.

 Copyright (C) 2009, University in Hagen, Faculty of Mathematics and
 Computer Science, Database Systems for New Applications.

 SECONDO is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published
 by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 SECONDO is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SECONDO; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ----

 //paragraph [1] Title: [{\Large \bf \begin{center}] [\end{center}}]
 //paragraph [10] Footnote: [{\footnote{] [}}]
 //[TOC] [\tableofcontents]

 [TOC]

0 Overview

1 Includes and defines

*/

#include "Toolbox.h"
#include "Point2.h"

namespace p2d {

/*
1 ~createCoordinate~

 If the first argument is of type int or real, then the grid-value and the
 precise- value is calculated and the function returns true. If not, then
 the function returns false.

*/
bool createCoordinate(ListExpr& value, int& grid, mpq_class& precise){
 if (nl->AtomType(value)==IntType){
  grid= nl->IntValue(value);
  precise=0;
  return true;
 } else {
  if (nl->AtomType(value)==RealType){
   double d = nl->RealValue(value);
   createValue(d, grid, precise);

   return true;
  }
 }
 return false;
}

/*
1 ~createValue~

  Computes the grid value ~grid~ and the precise value ~precise~
  from the given ~value~.

*/
void createValue(double value, int& grid, mpq_class& precise){
 grid= (int) value;

 if ((value<0) && (value != grid)){
  //value has a decimal and is less 0, the grid value is the
  //next smallest integer
  grid--;
 }
 assert((0<=(value-grid))&&((value-grid)<1));
 precise = p2d::computeMpqFromDouble((value-grid));
}

/*
1 ~createPoint2~

  Computes a ~Point2~-object from the coordinate (~x~, ~y~).

*/
void createPoint2(const double x, const double y, Point2** result){
 mpq_class preciseX, preciseY;
 int gX, gY;
 createValue(x, gX, preciseX);
 createValue(y, gY, preciseY);
 (*result) = new Point2(true, gX, gY, preciseX, preciseY);

}

const double Factor = 0.00000001;

/*
1 ~AlmostEqual~

 This function was first implemented in the SpatialALgebra.

*/
bool AlmostEqual(double a, double b) {
 double diff = abs(a - b);
 return (diff < Factor);
}

/*
1 ~computeMpqFromDouble~

*/
mpq_class computeMpqFromDouble(double value) {
 int denom = 1;
 int num = 0;
 while (!AlmostEqual(value, 0.0) && denom < 100000000) {

  denom = denom * 10;
  int n = round(value * 10.0);
  num = num * 10 + n;
  value = (value * 10.0) - n;
  if (AlmostEqual(value, 1.0)) {
   num++;
   value = value - 1.0;
  }
 }
 mpq_class result(num, denom);
 result.canonicalize();
 return result;
}

/*
1 ~ceil\_mpq~

  rounds ~value~ up to the next integer.

*/
mpz_class ceil_mpq(mpq_class& value){
 mpz_class numerator =  value.get_num();
 mpz_class denominator = value.get_den();
 mpz_class intValue = numerator / denominator;
 if (cmp(intValue,0)>0 ){
  if (cmp(intValue, value)<0){
   intValue = intValue + 1;
  }
 }
 return intValue;
}

/*
1 ~floor\_mpq~

  rounds ~value~ down to the next integer.

*/
mpz_class floor_mpq(mpq_class& value){
 mpz_class numerator =  value.get_num();
 mpz_class denominator = value.get_den();
 mpz_class intValue = numerator / denominator;
 if (cmp(intValue,0)>0 ){
  if (cmp(intValue, value)>0){
   intValue = intValue - 1;
  }
 }
 return intValue;
}

} /* namespace p2d */
