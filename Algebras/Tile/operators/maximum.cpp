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

#include "maximum.h"
#include "../t/tint.h"
#include "../t/treal.h"
#include "../t/tbool.h"
#include "../t/tstring.h"
#include "../mt/mtint.h"
#include "../mt/mtreal.h"
#include "../mt/mtbool.h"
#include "../mt/mtstring.h"


namespace TileAlgebra
{

/*
definition of maximum functions

*/

ValueMapping maximumFunctions[] =
{
  maximumFunction<tint, tProperties<int> >,
  maximumFunction<treal, tProperties<double> >,
  maximumFunction<tbool, tProperties<char> >,
  maximumFunction<tstring, tProperties<std::string> >,
  maximumFunction<mtint, mtProperties<int> >,
  maximumFunction<mtreal, mtProperties<double> >,
  maximumFunction<mtbool, mtProperties<char> >,
  maximumFunction<mtstring, mtProperties<std::string> >,
  0
};

/*
definition of maximum select function

*/

int maximumSelectFunction(ListExpr arguments)
{
  int nSelection = -1;

  if(arguments != 0)
  {
    NList argumentsList(arguments);
    NList argument1 = argumentsList.first();
    const int TYPE_NAMES = 8;
    const std::string TYPE_NAMES_ARRAY[TYPE_NAMES] =
    {
      tint::BasicType(),
      treal::BasicType(),
      tbool::BasicType(),
      tstring::BasicType(),
      mtint::BasicType(),
      mtreal::BasicType(),
      mtbool::BasicType(),
      mtstring::BasicType()
    };

    for(int i = 0; i < TYPE_NAMES; i++)
    {
      if(argument1.isSymbol(TYPE_NAMES_ARRAY[i]))
      {
        nSelection = i;
        break;
      }
    }
  }

  return nSelection;
}

/*
definition of maximum type mapping function

*/

ListExpr maximumTypeMapping(ListExpr arguments)
{
  ListExpr type = NList::typeError("Expecting a t or a mt type.");

  NList argumentsList(arguments);
  NList argument1 = argumentsList.first();

  if(argument1 == NList(tint::BasicType()) ||
     argument1 == NList(mtint::BasicType()))
  {
    type = NList(CcInt::BasicType()).listExpr();
  }

  else if(argument1 == NList(treal::BasicType()) ||
          argument1 == NList(mtreal::BasicType()))
  {
    type = NList(CcReal::BasicType()).listExpr();
  }

  else if(argument1 == NList(tbool::BasicType()) ||
          argument1 == NList(mtbool::BasicType()))
  {
    type = NList(CcBool::BasicType()).listExpr();
  }

  else if(argument1 == NList(tstring::BasicType()) ||
          argument1 == NList(mtstring::BasicType()))
  {
    type = NList(CcString::BasicType()).listExpr();
  }

  return type;
}

}
