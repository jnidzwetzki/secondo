/*
//[ae] [\"a]
//[ue] [\"u]
//[oe] [\"o]

1 Header File: Standard Data Types

December 1998 Friedhelm Becker

2002-2003 U. Telle. Diploma thesis "reimplementation of SECONDO"

Nov. 2004. M. Spiekermann. Modifications in CcInt. Using inline directives
and avoiding to dereference pointers in the ~Compare~ method improves performance. 

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
  
  CcInt()
  { 
    intsCreated++; 
  }
  
  CcInt( bool d, int v )
  { 
    defined = d; intval = v; intsCreated++; 
  }
  
  ~CcInt()
  { 
    intsDeleted++; 
  }
  
  inline bool IsDefined() const 
  { 
    return (defined); 
  }
  
  inline void SetDefined(bool defined) 
  { 
    this->defined = defined;
  }
    
  inline int GetIntval() 
  { 
    return (intval); 
  }
  
  inline void Set( int v )
  { 
    defined = true, intval = v; 
  }
  
  inline void Set( bool d, int v )
  { 
    defined = d, intval = v; 
  }
  
  inline size_t HashValue()
  { 
    return (defined ? intval : 0); 
  }
  
  void CopyFrom(StandardAttribute* right);
  
  inline int Compare(Attribute *arg)
  {
    assert(arg);
    CcInt* p = (CcInt*)(arg);
    bool argDefined = p->defined;
    if(!defined && !argDefined) {
      return 0;
    }
    if(!defined) {
      return -1;
    }
    if(!argDefined) {
      return 1;
    }

    if ( intval < p->intval ) {
      return (-1);
    }  
    if ( intval > p->intval) {
      return (1);
    }  
    return (0);
  }

  bool Adjacent(Attribute *arg);
  
  inline CcInt* Clone() 
  { 
    return (new CcInt( this->defined, this->intval )); 
  }
  
  ostream& Print( ostream &os ) { return (os << intval); }

  ListExpr CopyToList( ListExpr typeInfo )
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

      Word result = (algMgr->InObj(algId, typeId))( typeInfo, 
                         instance, errorPos, errorInfo, correct );
      if( correct )
        return result;
      return SetWord( Address(0) );
  }

  static long intsCreated;
  static long intsDeleted;

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
  bool     IsDefined() const;
  void     SetDefined(bool defined);
  float    GetRealval();
  void     Set( float v );
  void     Set( bool d, float v );
  size_t HashValue();
  void CopyFrom(StandardAttribute* right);
  int      Compare( Attribute* arg );
  bool      Adjacent( Attribute* arg );
  CcReal*  Clone() ;
  ostream& Print( ostream &os ) { return (os << realval); }

  static long realsCreated;
  static long realsDeleted;

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
  bool     IsDefined() const;
  void     SetDefined(bool defined);
  bool     GetBoolval();
  void     Set( bool d, bool v );
  size_t HashValue();
  void CopyFrom(StandardAttribute* right);
  int      Compare( Attribute * arg );
  bool     Adjacent( Attribute * arg );
  CcBool*  Clone() ;
  ostream& Print( ostream &os ) {
    if (boolval == true) return (os << "TRUE");
    else return (os << "FALSE");
  }

  static long boolsCreated;
  static long boolsDeleted;

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
  bool      IsDefined() const;
  void      SetDefined(bool defined);
  STRING*   GetStringval();
  void      Set( bool d, const STRING* v );
  size_t HashValue();
  void CopyFrom(StandardAttribute* right);
  int       Compare( Attribute* arg );
  bool      Adjacent( Attribute* arg );
  CcString* Clone() ;
  ostream&  Print( ostream &os ) { return (os << "\"" << stringval << "\""); }

  static long stringsCreated;
  static long stringsDeleted;

 private:
  bool   defined;
  STRING stringval;
};

ostream& ShowStandardTypesStatistics( const bool reset, ostream& o );

/*
6 Some Functions Prototypes

*/
Word InCcBool( ListExpr typeInfo, ListExpr value, 
               int errorPos, ListExpr& errorInfo, bool& correct );
ListExpr OutCcBool( ListExpr typeinfo, Word value );
Word InCcInt( ListExpr typeInfo, ListExpr value, 
              int errorPos, ListExpr& errorInfo, bool& correct );
ListExpr OutCcInt( ListExpr typeinfo, Word value );
Word InCcReal( ListExpr typeInfo, ListExpr value, 
               int errorPos, ListExpr& errorInfo, bool& correct );
ListExpr OutCcReal( ListExpr typeinfo, Word value );

#endif

