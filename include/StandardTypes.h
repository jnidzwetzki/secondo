/*
//[ae] [\"a]
//[ue] [\"u]
//[oe] [\"o]
 
1 Header File: Standard Data Types 

December 1998 Friedhelm Becker
 
1.1 Overview
 
This file defines four classes: CcInt, CcReal, CcBool and CcString. They
are the data types which are provided by the Standardalgebra.

*/

#ifndef STANDARDTYPES_H
#define STANDARDTYPES_H

#include <string>
#include "StandardAttribute.h"

/*
2.1 CcInt

*/


class CcInt : public StandardAttribute
{
 public:
  CcInt();
  CcInt( bool d, int v );
  ~CcInt();
  bool     IsDefined();
  void     SetDefined(bool defined);
  int      GetIntval();
  void*    GetValue();
  void     Set( int v );
  void     Set( bool d, int v );
  size_t HashValue();
  void CopyFrom(StandardAttribute* right);
  int      Compare(Attribute *arg);
  int      Adjacent(Attribute *arg);
  int      Sizeof() ;
  CcInt*   Clone() ;
  ostream& Print( ostream &os ) { return (os << intval); }

    ListExpr   CopyToList( ListExpr typeInfo )
    {
      cout << "CcInt CopyToList" << endl;
      NestedList *nl = SecondoSystem::GetNestedList();
      AlgebraManager* algMgr = SecondoSystem::GetAlgebraManager();
      int algId = nl->IntValue( nl->First( nl->First( typeInfo ) ) ),
          typeId = nl->IntValue( nl->Second( nl->First( typeInfo ) ) );

      return (algMgr->OutObj(algId, typeId))( typeInfo, SetWord(this) );
    }

    Word CreateFromList( const ListExpr typeInfo, const ListExpr instance,
                         const int errorPos, ListExpr& errorInfo, bool& correct )
    {
      cout << "CcInt CreateFromList" << endl;
      NestedList *nl = SecondoSystem::GetNestedList();
      AlgebraManager* algMgr = SecondoSystem::GetAlgebraManager();
      int algId = nl->IntValue( nl->First( nl->First( typeInfo ) ) ),
          typeId = nl->IntValue( nl->Second( nl->First( typeInfo ) ) );

      Word result = (algMgr->InObj(algId, typeId))( typeInfo, instance, errorPos, errorInfo, correct );
      if( correct )
        return result;
      return SetWord( Address(0) );
    }

 private:
  bool defined;
  int  intval;
};

/*
3.1 CcReal

*/

class CcReal : public StandardAttribute
{
 public:
  CcReal();
  CcReal( bool d, float v );
  ~CcReal();
  bool     IsDefined();
  void     SetDefined(bool defined);
  float    GetRealval();
  void*    GetValue();
  void     Set( float v );
  void     Set( bool d, float v );
  size_t HashValue();
  void CopyFrom(StandardAttribute* right);
  int      Compare( Attribute* arg );
  int      Adjacent( Attribute* arg );
  int      Sizeof() ;
  CcReal*  Clone() ;
  ostream& Print( ostream &os ) { return (os << realval); }
 private:
  bool  defined;
  float realval;
};

/*
4.1 CcBool

*/

class CcBool : public StandardAttribute
{
 public:
  CcBool();
  CcBool( bool d, int v );
  ~CcBool();
  bool     IsDefined();
  void     SetDefined(bool defined);
  bool     GetBoolval();
  void*    GetValue();
  void     Set( bool d, bool v );
  size_t HashValue();
  void CopyFrom(StandardAttribute* right);
  int      Compare( Attribute * arg );
  int      Adjacent( Attribute * arg );
  int      Sizeof()  ;
  CcBool*  Clone() ;
  ostream& Print( ostream &os ) {
    if (boolval == true) return (os << "TRUE");
    else return (os << "FALSE");
  }
 private:
  bool defined;
  bool boolval;
};

/*
5.1 CcString

*/

typedef char STRING[49];

class CcString : public StandardAttribute
{
 public:
  CcString();
  CcString( bool d, const STRING* v );
  ~CcString();
  bool      IsDefined();
  void      SetDefined(bool defined);
  STRING*   GetStringval();
  void*     GetValue();
  void      Set( bool d, const STRING* v );
  size_t HashValue();
  void CopyFrom(StandardAttribute* right);
  int       Compare( Attribute* arg );
  int       Adjacent( Attribute* arg );
  int       Sizeof()  ;
  CcString* Clone() ;
  ostream&  Print( ostream &os ) { return (os << "\"" << stringval << "\""); }
 private:
  bool   defined;
  STRING stringval;
};

#endif

