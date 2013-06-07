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

#ifndef TILEALGEBRA_MTREAL_H
#define TILEALGEBRA_MTREAL_H

#include "mt.h"
#include "mtProperties.h"
#include "../Properties/Propertiesreal.h"
#include "RectangleAlgebra.h"

namespace TileAlgebra
{

/*
typedef of mtreal type

*/

typedef mt<double> mtreal;

/*
declaration of template class mtProperties<double>

*/

template <>
class mtProperties<double>
{
  public:

  typedef Properties<double> TypeProperties;
  typedef Rectangle<3> bboxType;
  typedef mtgrid gridType;
  typedef mtreal mtType;
  static int GetDimensionSize();
  static int GetFlobElements();
  static SmiSize GetFlobSize();
  static std::string GetTypeName(); 
};

}

#endif // TILEALGEBRA_MTREAL_H
