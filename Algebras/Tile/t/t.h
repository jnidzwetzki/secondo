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

#ifndef TILEALGEBRA_T_H
#define TILEALGEBRA_T_H

#include "TypeConstructor.h"
#include "Symbols.h"
#include "tProperties.h"
#include "Attribute.h"
#include "../grid/tgrid.h"
#include "../../Tools/Flob/Flob.h"
#include "../Index.h"

namespace TileAlgebra
{

/*
declaration of template class t

*/

template <typename Type, typename Properties = tProperties<Type> >
class t : public Attribute
{
  /*
  constructors

  */

  protected:

  t();

  public:

  t(bool bDefined);
  t(const t& rt);

  /*
  destructor

  */

  virtual ~t();

  /*
  operators

  */

  t& operator=(const t& rt);

  /*
  TileAlgebra operator methods

  */

  tgrid GetGrid() const;
  Type GetMinimum() const;
  Type GetMaximum() const;
  // Rectangle<2> GetBoundingBox() const;

  protected:

  /*
  internal methods

  */

  bool SetGrid(const double& rX, const double& rY, const double& rLength);
  void SetMinimum(const Type& rValue);
  void SetMaximum(const Type& rValue);
  Type GetValue(const Index<2>& rIndex);
  bool SetValue(const Index<2>& rIndex, const Type& rValue);

  public:

  /*
  override functions from base class Attribute

  */

  virtual bool Adjacent(const Attribute* pAttribute) const;
  virtual Attribute* Clone() const;
  virtual int Compare(const Attribute* pAttribute) const;
  virtual void CopyFrom(const Attribute* pAttribute);
  virtual Flob* GetFLOB(const int i);
  virtual size_t HashValue() const;
  virtual int NumOfFLOBs() const;
  virtual size_t Sizeof() const;

  /*
  The following functions are used to integrate the ~t~
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

  protected:

  /*
  members

  */

  tgrid m_Grid;
  Type m_Minimum;
  Type m_Maximum;
  Flob m_Flob;
};

/*
implementation of template class t

*/

template <typename Type, typename Properties>
t<Type, Properties>::t()
                    :Attribute()
{

}

template <typename Type, typename Properties>
t<Type, Properties>::t(bool bDefined)
                    :Attribute(bDefined),
                     m_Grid(false),
                     m_Minimum(Properties::TypeProperties::
                               GetUndefinedValue()),
                     m_Maximum(Properties::TypeProperties::
                               GetUndefinedValue()),
                     m_Flob(Properties::GetFlobSize())
{
  int dimensionSize = Properties::GetDimensionSize();
  Type undefinedValue = Properties::TypeProperties::GetUndefinedValue();
  
  for(int row = 0; row < dimensionSize; row++)
  {
    for(int column = 0; column < dimensionSize; column++)
    {
      Index<2> indexes = (int[]){column, row};
      SetValue(indexes, undefinedValue);
    }
  }
}

template <typename Type, typename Properties>
t<Type, Properties>::t(const t<Type, Properties>& rt)
                    :Attribute(rt.IsDefined()),
                     m_Grid(rt.m_Grid),
                     m_Minimum(rt.m_Minimum),
                     m_Maximum(rt.m_Maximum),
                     m_Flob(rt.m_Flob)
{
  
}

template <typename Type, typename Properties>
t<Type, Properties>::~t()
{

}

template <typename Type, typename Properties>
t<Type, Properties>& t<Type, Properties>::operator=
                                          (const t<Type, Properties>& rt)
{
  if(this != &rt)
  {
    Attribute::operator=(rt);
    m_Grid = rt.m_Grid;
    m_Minimum = rt.m_Minimum;
    m_Maximum = rt.m_Maximum;

    bool bOK = false;
    bOK = m_Flob.clean();
    assert(bOK);
    bOK = m_Flob.copyFrom(rt.m_Flob);
    assert(bOK);
  }

  return *this;
}

template <typename Type, typename Properties>
tgrid t<Type, Properties>::GetGrid() const
{
  return m_Grid;
}

template <typename Type, typename Properties>
Type t<Type, Properties>::GetMinimum() const
{
  return m_Minimum;
}

template <typename Type, typename Properties>
Type t<Type, Properties>::GetMaximum() const
{
  return m_Maximum;
}

template <typename Type, typename Properties>
bool t<Type, Properties>::SetGrid(const double& rX,
                                  const double& rY,
                                  const double& rLength)
{
  bool bRetVal = true;

  m_Grid.SetDefined(true);
  bRetVal &= m_Grid.SetX(rX);
  bRetVal &= m_Grid.SetY(rY);
  bRetVal &= m_Grid.SetLength(rLength);

  return bRetVal;
}

template <typename Type, typename Properties>
void t<Type, Properties>::SetMinimum(const Type& rValue)
{
  m_Minimum = rValue;
}

template <typename Type, typename Properties>
void t<Type, Properties>::SetMaximum(const Type& rValue)
{
  m_Maximum = rValue;
}

template <typename Type, typename Properties>
Type t<Type, Properties>::GetValue(const Index<2>& rIndex)
{
  Type value = Properties::TypeProperties::GetUndefinedValue();

  int dimensionSize = Properties::GetDimensionSize();

  if(IsDefined() &&
     rIndex[0] >= 0 &&
     rIndex[0] < dimensionSize &&
     rIndex[1] >= 0 &&
     rIndex[1] < dimensionSize)
  {
    int flobIndex = rIndex[1] * dimensionSize + rIndex[0];
    
    bool bOK = m_Flob.read(reinterpret_cast<const char*>(&value),
                          sizeof(Type),
                          flobIndex * sizeof(Type));
    assert(bOK);
  }

  return value;
}

template <typename Type, typename Properties>
bool t<Type, Properties>::SetValue(const Index<2>& rIndex,
                                   const Type& rValue)
{
  bool bRetVal = false;

  int dimensionSize = Properties::GetDimensionSize();

  if(IsDefined() &&
     rIndex[0] >= 0 &&
     rIndex[0] < dimensionSize &&
     rIndex[1] >= 0 &&
     rIndex[1] < dimensionSize)
  {
    int flobIndex = rIndex[1] * dimensionSize + rIndex[0];
    
    bRetVal = m_Flob.write(reinterpret_cast<const char*>(&rValue),
                           sizeof(Type),
                           flobIndex * sizeof(Type));
  }

  return bRetVal;
}

template <typename Type, typename Properties>
bool t<Type, Properties>::Adjacent(const Attribute* pAttribute) const
{
  return false;
}

template <typename Type, typename Properties>
Attribute* t<Type, Properties>::Clone() const
{
  Attribute* pAttribute = new t<Type, Properties>(*this);
  assert(pAttribute != 0);

  return pAttribute;
}

template <typename Type, typename Properties>
int t<Type, Properties>::Compare(const Attribute* pAttribute) const
{
  int nRetVal = -1;

  if(pAttribute != 0)
  {
    const t<Type, Properties>* pt = dynamic_cast<const t<Type, Properties>*>
                                    (pAttribute);

    if(pt != 0)
    {
      bool bIsDefined = IsDefined();
      bool btIsDefined = pt->IsDefined();

      if(bIsDefined == true)
      {
        if(btIsDefined == true) // defined x defined
        {
          nRetVal = m_Grid.Compare(&(pt->m_Grid));

          if(nRetVal == 0)
          {
            SmiSize flobSize = Properties::GetFlobSize();
            
            char buffer1[flobSize];
            m_Flob.read(buffer1, flobSize, 0);

            char buffer2[flobSize];
            pt->m_Flob.read(buffer2, flobSize, 0);

            nRetVal = memcmp(buffer1, buffer2, flobSize);
          }
        }

        else // defined x undefined
        {
          nRetVal = 1;
        }
      }

      else
      {
        if(btIsDefined == true) // undefined x defined
        {
          nRetVal = -1;
        }

        else // undefined x undefined
        {
          nRetVal = 0;
        }
      }
    }
  }

  return nRetVal;
}

template <typename Type, typename Properties>
void t<Type, Properties>::CopyFrom(const Attribute* pAttribute)
{
  if(pAttribute != 0)
  {
    const t<Type, Properties>* pt = dynamic_cast<const t<Type, Properties>*>
                                    (pAttribute);

    if(pt != 0)
    {
      *this = *pt;
    }
  }
}

template <typename Type, typename Properties>
Flob* t<Type, Properties>::GetFLOB(const int i)
{ 
  Flob* pFlob = 0;
  int nFlobs = NumOfFLOBs();
  
  if(i >= 0 &&
     i < nFlobs)
  {
    switch(i)
    {
      case 0:   pFlob = &m_Flob;
                break;
                
      default:  break;
    }
  }
  
  return pFlob;
}

template <typename Type, typename Properties>
size_t t<Type, Properties>::HashValue() const
{
  size_t hashValue = 0;

  if(IsDefined())
  {
    hashValue = reinterpret_cast<size_t>(this);
  }

  return hashValue;
}

template <typename Type, typename Properties>
int t<Type, Properties>::NumOfFLOBs() const
{ 
  return 1;
}

template <typename Type, typename Properties>
size_t t<Type, Properties>::Sizeof() const
{
  return sizeof(t<Type, Properties>);
}

template <typename Type, typename Properties>
const std::string t<Type, Properties>::BasicType()
{
  return Properties::GetTypeName();
}

template <typename Type, typename Properties>
void* t<Type, Properties>::Cast(void* pVoid)
{
  return new(pVoid)t<Type, Properties>;
}

template <typename Type, typename Properties>
Word t<Type, Properties>::Clone(const ListExpr typeInfo,
                                const Word& rWord)
{
  Word word;

  t<Type, Properties>* pt = static_cast<t<Type, Properties>*>(rWord.addr);

  if(pt != 0)
  {
    word.addr = new t<Type, Properties>(*pt);
    assert(word.addr != 0);
  }

  return word;
}

template <typename Type, typename Properties>
void t<Type, Properties>::Close(const ListExpr typeInfo,
                                Word& rWord)
{
  t<Type, Properties>* pt = static_cast<t<Type, Properties>*>(rWord.addr);

  if(pt != 0)
  {
    delete pt;
    rWord.addr = 0;
  }
}

template <typename Type, typename Properties>
Word t<Type, Properties>::Create(const ListExpr typeInfo)
{
  Word word;

  word.addr = new t<Type, Properties>(true);
  assert(word.addr != 0);

  return word;
}

template <typename Type, typename Properties>
void t<Type, Properties>::Delete(const ListExpr typeInfo,
                                 Word& rWord)
{
  t<Type, Properties>* pt = static_cast<t<Type, Properties>*>(rWord.addr);

  if(pt != 0)
  {
    delete pt;
    rWord.addr = 0;
  }
}

template <typename Type, typename Properties>
TypeConstructor t<Type, Properties>::GetTypeConstructor()
{
  TypeConstructor typeConstructor
  (
    t<Type, Properties>::BasicType(), // type name function
    t<Type, Properties>::Property,    // property function describing signature
    t<Type, Properties>::Out,         // out function
    t<Type, Properties>::In,          // in function
    0,                                // save to list function
    0,                                // restore from list function
    t<Type, Properties>::Create,      // create function
    t<Type, Properties>::Delete,      // delete function
    t<Type, Properties>::Open,        // open function
    t<Type, Properties>::Save,        // save function
    t<Type, Properties>::Close,       // close function
    t<Type, Properties>::Clone,       // clone function
    t<Type, Properties>::Cast,        // cast function
    t<Type, Properties>::SizeOfObj,   // sizeofobj function
    t<Type, Properties>::KindCheck    // kindcheck function
  );

  typeConstructor.AssociateKind(Kind::DATA());

  return typeConstructor;
}

template <typename Type, typename Properties>
Word t<Type, Properties>::In(const ListExpr typeInfo,
                             const ListExpr instance,
                             const int errorPos,
                             ListExpr& rErrorInfo,
                             bool& rCorrect)
{
  Word word;

  NList instanceList(instance);
  rCorrect = false;

  if(instanceList.isAtom() == false)
  {
    NList gridList = instanceList.elem(1);

    if(gridList.length() == 3)
    {
      if(gridList.isReal(1) &&
         gridList.isReal(2) &&
         gridList.isReal(3))
      {
        t<Type, Properties>* pt = new t<Type, Properties>(true);

        if(pt != 0)
        {
          bool bOK = pt->SetGrid(gridList.elem(1).realval(),
                                 gridList.elem(2).realval(),
                                 gridList.elem(3).realval());

          if(bOK == true)
          {
            instanceList.rest();

            if(instanceList.isEmpty() == false)
            {
              NList sizeList = instanceList.elem(1);

              if(sizeList.length() == 2)
              {
                if(sizeList.isInt(1) &&
                   sizeList.isInt(2) &&
                   sizeList.elem(1).intval() > 0 &&
                   sizeList.elem(2).intval() > 0)
                {
                  int sizeX = sizeList.elem(1).intval();
                  int sizeY = sizeList.elem(2).intval();
                  Cardinal valueListLength = static_cast<Cardinal>
                                             (sizeX * sizeY);

                  instanceList.rest();

                  while(bOK &&
                        instanceList.isEmpty() == false)
                  {
                    NList pageList = instanceList.first();

                    if(pageList.length() == 3)
                    {
                      if(pageList.isInt(1) &&
                         pageList.isInt(2))
                      {
                        int indexX = pageList.elem(1).intval();
                        int indexY = pageList.elem(2).intval();
                        int dimensionSize = Properties::GetDimensionSize();

                        if(indexX >= 0 &&
                           indexX <= dimensionSize - sizeX &&
                           indexY >= 0 &&
                           indexY <= dimensionSize - sizeY)
                        {
                          pageList.rest();
                          pageList.rest();

                          NList valueList = pageList.first();

                          if(valueList.length() == valueListLength)
                          {
                            for(int row = 0; row < sizeY; row++)
                            {
                              for(int column = 0; column < sizeX; column++)
                              {
                                int listIndex = row * sizeX + column + 1;
                                Index<2> index = (int[]){(indexX + column),
                                                         (indexY + row)};
                                Type value = Properties::TypeProperties::
                                             GetUndefinedValue();

                                if(valueList.elem(listIndex).
                                   isSymbol(Symbol::UNDEFINED()) == false)
                                {
                                  if(Properties::TypeProperties::
                                     IsValidValueType
                                     (valueList.elem
                                     (listIndex)))
                                  {
                                    value = Properties::TypeProperties::
                                            GetValue
                                            (valueList.elem(listIndex));

                                    Type minimum = pt->GetMinimum();

                                    if(Properties::TypeProperties::
                                       IsUndefinedValue(minimum) ||
                                       value < minimum)
                                    {
                                      pt->SetMinimum(value);
                                    }

                                    Type maximum = pt->GetMaximum();

                                    if(Properties::TypeProperties::
                                       IsUndefinedValue(maximum) ||
                                       value > maximum)
                                    {
                                      pt->SetMaximum(value);
                                    }
                                  }

                                  else
                                  {
                                    bOK = false;
                                    cmsg.inFunError("Type mismatch: "
                                                    "list value in "
                                                    "partial grid has "
                                                    "wrong type.");
                                  }
                                }

                                pt->SetValue(index, value);
                              }
                            }

                            instanceList.rest();
                          }

                          else
                          {
                            bOK = false;
                            cmsg.inFunError("Type mismatch: "
                                            "list for partial grid values "
                                            "is too short or too long.");
                          }
                        }

                        else
                        {
                          bOK = false;
                          cmsg.inFunError("Type mismatch: "
                                          "page list index is "
                                          "out of valid range.");
                        }
                      }

                      else
                      {
                        bOK = false;
                        cmsg.inFunError("Type mismatch: "
                                        "partial grid content must start "
                                        "with two integers.");
                      }
                    }

                    else
                    {
                      bOK = false;
                      cmsg.inFunError("Type mismatch: "
                                      "partial grid content must contain "
                                      "three elements.");
                    }
                  }

                }

                else
                {
                  bOK = false;
                  cmsg.inFunError("Type mismatch: "
                                  "partial grid size must contain "
                                  "two positive integers.");
                }
              }

              else
              {
                bOK = false;
                cmsg.inFunError("Size list must have a length of 2.");
              }
            }
          }

          if(bOK)
          {
            word.addr = pt;
            rCorrect = true;
          }

          else
          {
            delete pt;
            pt = 0;
            rCorrect = false;
          }
        }
      }

      else
      {
        cmsg.inFunError("Type mismatch: expected 3 reals as tgrid sublist.");
      }
    }

    else
    {
      cmsg.inFunError("Type mismatch: list for tgrid is too short "
                      "or too long.");
    }
  }

  else
  {
    cmsg.inFunError("Expected list as first element, got an atom.");
  }

  return word;
}

template <typename Type, typename Properties>
bool t<Type, Properties>::KindCheck(ListExpr type,
                                    ListExpr& rErrorInfo)
{
  bool bRetVal = false;

  if(nl != 0)
  {
    bRetVal = nl->IsEqual(type, t<Type, Properties>::BasicType());
  }

  return bRetVal;
}

template <typename Type, typename Properties>
bool t<Type, Properties>::Open(SmiRecord& rValueRecord,
                               size_t& rOffset,
                               const ListExpr typeInfo,
                               Word& rValue)
{
  bool bRetVal = OpenAttribute<t<Type, Properties> >(rValueRecord,
                                                     rOffset,
                                                     typeInfo,
                                                     rValue);

  return bRetVal;
}

template <typename Type, typename Properties>
ListExpr t<Type, Properties>::Out(ListExpr typeInfo,
                                  Word value)
{
  ListExpr pListExpr = 0;

  if(nl != 0)
  {
    t<Type, Properties>* pt = static_cast<t<Type, Properties>*>(value.addr);

    if(pt != 0)
    {
      if(pt->IsDefined() == true)
      {
        NList instanceList;

        NList gridList;
        gridList.append(pt->m_Grid.GetX());
        gridList.append(pt->m_Grid.GetY());
        gridList.append(pt->m_Grid.GetLength());
        instanceList.append(gridList);

        NList sizeList;
        sizeList.append(Properties::GetDimensionSize());
        sizeList.append(Properties::GetDimensionSize());
        instanceList.append(sizeList);

        NList tintList;
        tintList.append(0);
        tintList.append(0);

        Type undefinedValue = Properties::TypeProperties::GetUndefinedValue();
        NList valueList;

        for(int i = 0; i < Properties::GetFlobElements(); i++)
        {
          Type value = undefinedValue;

          bool bOK = pt->m_Flob.read(reinterpret_cast<char*>(&value),
                                     sizeof(Type),
                                     i * sizeof(Type));
          assert(bOK);

          valueList.append(Properties::TypeProperties::ToNList(value));
        }

        tintList.append(valueList);
        instanceList.append(tintList);

        pListExpr = instanceList.listExpr();
      }

      else
      {
        pListExpr = nl->SymbolAtom(Symbol::UNDEFINED());
      }
    }
  }

  return pListExpr;
}

template <typename Type, typename Properties>
ListExpr t<Type, Properties>::Property()
{
  NList propertyList;

  NList names;
  names.append(NList(std::string("Signature"), true));
  names.append(NList(std::string("Example Type List"), true));
  names.append(NList(std::string("ListRep"), true));
  names.append(NList(std::string("Example List"), true));
  names.append(NList(std::string("Remarks"), true));

  NList values;
  values.append(NList(std::string("-> DATA"), true));
  values.append(NList(BasicType(), true));
  values.append(NList
               (std::string("((x y l) (szx szy) ((ix iy (v*)))*)"),
                true));
  values.append(NList
               (std::string("((0.0 0.0 1.0) (2 2) ((0 0 (0 1 2 3))))"),
                true));
  values.append(NList(std::string(""), true));

  propertyList = NList(names, values);

  return propertyList.listExpr();
}

template <typename Type, typename Properties>
bool t<Type, Properties>::Save(SmiRecord& rValueRecord,
                               size_t& rOffset,
                               const ListExpr typeInfo,
                               Word& rValue)
{
  bool bRetVal = SaveAttribute<t<Type, Properties> >(rValueRecord,
                                                     rOffset,
                                                     typeInfo,
                                                     rValue);

  return bRetVal;
}

template <typename Type, typename Properties>
int t<Type, Properties>::SizeOfObj()
{
  return sizeof(t<Type, Properties>);
}

}

#endif // TILEALGEBRA_T_H
