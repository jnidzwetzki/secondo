/*
This file is part of SECONDO.

Copyright (C) 2011, University in Hagen, Department of Computer Science,
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

#ifndef TILEALGEBRA_TINTARRAY_H
#define TILEALGEBRA_TINTARRAY_H

#include "Attribute.h"
#include "../Constants.h"
#include "../Grid/Grid2.h"

namespace TileAlgebra
{

class tintArray : public Attribute
{  
  /*
  constructors
  
  */

  private:

  tintArray();

  public:

  tintArray(bool bDefined);
  tintArray(const tintArray& rtintArray);
  
  /*
  destructor
  
  */
  
  virtual ~tintArray();
  
  /*
  operators
  
  */
  
  tintArray& operator=(const tintArray& rtintArray);
  bool operator==(const tintArray& rtintArray) const;

  /*
  methods

  */

  bool Load();
  bool SetGrid(const double& rX, const double& rY, const double& rLength);
  bool SetValue(int nIndex, int nValue);

  /*
  override functions from base class Attribute
  
  */

  virtual bool Adjacent(const Attribute* pAttribute) const;
  virtual Attribute* Clone() const;
  virtual int Compare(const Attribute* pAttribute) const;
  virtual void CopyFrom(const Attribute* pAttribute);
  virtual size_t HashValue() const;
  virtual size_t Sizeof() const;
  
  /*
  The following functions are used to integrate the ~tintArray~
  datatype into secondo.

  */
  
  static const std::string BasicType();
  static void* Cast(void* pVoid);
  static Word Clone(const ListExpr typeInfo,
                    const Word& rWord);
  static void Close(const ListExpr typeInfo,
                    Word& rWord);
  static Word Create(const ListExpr typeInfo);
  static void Delete(const ListExpr typeInfo,
                     Word& rWord);
  static TypeConstructor GetTypeConstructor();
  static Word In(const ListExpr typeInfo,
                 const ListExpr instance,
                 const int errorPos,
                 ListExpr& rErrorInfo,
                 bool& rCorrect);
  static bool KindCheck(ListExpr type,
                        ListExpr& rErrorInfo);
  static bool Open(SmiRecord& rValueRecord,
                   size_t& rOffset,
                   const ListExpr typeInfo,
                   Word& rValue);
  static ListExpr Out(ListExpr typeInfo,
                      Word value);
  static ListExpr Property();
  static bool Save(SmiRecord& rValueRecord,
                   size_t& rOffset,
                   const ListExpr typeInfo,
                   Word& rValue);
  static int SizeOfObj();
    
  private:

  /*
  members
   
  */

  Grid2 m_Grid;
  int m_Array[961];
};

}

#endif // TILEALGEBRA_TINTARRAY_H
