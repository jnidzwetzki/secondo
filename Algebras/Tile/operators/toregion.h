/*
This file is part of SECONDO.

Copyright (C) 2013, University in Hagen, Department of Computer Science,
Database Systems for New Applications.

SECONDO is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

SECONDO is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with SECONDO; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#ifndef TILEALGEBRA_TOREGION_H
#define TILEALGEBRA_TOREGION_H

#include "AlgebraTypes.h"
#include "Operator.h"
#include "QueryProcessor.h"
#include "SpatialAlgebra.h"
#include "../t/tbool.h"

namespace TileAlgebra
{

/*
definition of toregion Operator Info structure

*/
  
struct toregionInfo : OperatorInfo
{
  toregionInfo()
  { 
    name      = "toregion";
    syntax    = "_ toregion";
    meaning   = "Maps a tbool object to a region.";
    signature = tbool::BasicType() + " -> " + Region::BasicType();
  }
};

/*
declaration of toregion function

*/

int toregionFunction(Word* pArguments,
                     Word& rResult,
                     int message,
                     Word& rLocal,
                     Supplier supplier);

/*
declaration of toregion type mapping function

*/

ListExpr toregionTypeMappingFunction(ListExpr arguments);

}

#endif // TILEALGEBRA_TOREGION_H
