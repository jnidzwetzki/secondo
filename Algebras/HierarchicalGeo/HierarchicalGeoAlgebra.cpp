/*
----
This file is part of SECONDO.

Copyright (C) 2004, University in Hagen, Department of Computer Science,
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
----

//paragraph [1] Title: [{\Large \bf \begin{center}] [\end{center}}]
//paragraph [10] Footnote: [{\footnote{] [}}]
//[TOC] [\tableofcontents]

[1] Implementation of Module

April 2007 Sascha Vaut

[TOC]

1 Overview

Up to now, this file contains the implementation of the type constructors 
~uncertain~ and ~cpoint~. The memory data structures used for these 
type constructors are implemented in the HierarchicalGeoAlgebra.h file.

2 Defines, includes, and constants

*/
#include <cmath>
#include <limits>
#include <iostream>
#include <sstream>
#include <string>
#include <stack>
#include <vector>
#include "NestedList.h"
#include "QueryProcessor.h"
#include "Algebra.h"
#include "StandardTypes.h"
#include "SpatialAlgebra.h"
#include "PolySolver.h"
#include "RelationAlgebra.h"
#include "TypeMapUtils.h"
#include <math.h>

extern NestedList* nl;
extern QueryProcessor* qp;

#include "DateTime.h"
#include "HierarchicalGeoAlgebra.h"



/*
2.1 Definition of some constants





3 Type investigation auxiliaries

Within this algebra module, we have to handle with values of four different
uncertain basic-types: ~cbool~, ~cint~, ~creal~, ~cpoint~. 

Later on we will examine nested list type descriptions. In particular, we
are going to check whether they describe one of the four types just introduced.
In order to simplify dealing with list expressions describing these types, we
declare an enumeration, ~UncertainBaseType~, containing the four types, and
 a function, ~UncertainBaseTypeOfSymbol~, taking a nested list as argument
 and returning the corresponding ~UncertainBaseType~ type name.


Some functions of template class ~uncertain~

*/
template <class Alpha>
void Uncertain<Alpha>::Epsilon(CcReal& result)
{
  result = (CcReal)epsilon;
}

/*
4 Type Constructors

4.1 Type Constructor ~CPoint~

Type ~cpoint~ represents an (epsilon, (x, y))-pair.

List Representation

The list representation of a ~cpoint~ is

----    ( epsilon ( x y ) )
----

For example:

----    ( 20.5 ( 329.456 22.289 ) )
----

Function describing the signature of the Type Constructor

*/
ListExpr CPointProperty()
{
  return (nl->TwoElemList(
            nl->FiveElemList(
                  nl->StringAtom("Signature"),
                  nl->StringAtom("Example Type List"),
                  nl->StringAtom("List Rep"),
                  nl->StringAtom("Example List"),
                  nl->StringAtom("Remarks")),
            nl->FiveElemList(
                  nl->StringAtom("-> UNCERTAIN"),
                  nl->StringAtom("cpoint"),
                  nl->StringAtom("(<epsilon>(<x> <y>))"),
                  nl->StringAtom("( 20.5 ( 329.456 22.289) )"),
                  nl->StringAtom(" All 3 values must be of type real." ))));
}

/*
Kind Checking Function

*/

bool CheckCPoint( ListExpr type, ListExpr& errorInfo )
{
  return (nl->IsEqual( type, "cpoint"));
}

/*
~Out~-function

*/
ListExpr OutCPoint( ListExpr typeInfo, Word value )
{
  CPoint* cpoint = (CPoint*)(value.addr);
  
  if ( !cpoint->IsDefined() )
    return (nl->SymbolAtom("undef"));
  else
    {
      ListExpr coordinates = nl->TwoElemList(
            nl->RealAtom( cpoint->value.GetX() ),
            nl->RealAtom( cpoint->value.GetY() ));
                  
      return nl->TwoElemList(
            nl->RealAtom( cpoint->GetEpsilon() ),
            coordinates );
    }
}

/*
~In~-function

*/

Word InCPoint( const ListExpr typeInfo, const ListExpr instance,
               const int errorPos, ListExpr& errorInfo, bool& correct )
{
  string errmsg;
  if ( nl->ListLength( instance ) == 2 )    
  // 2 arguments are necessary: epsilon and a point
  {
    ListExpr first = nl->First( instance );               // the epsilon value
    ListExpr second = nl->Second( instance );    // the point representation
    
    if ( nl->IsAtom( first ) && nl->AtomType( first ) == RealType )
    {
      if ( nl->ListLength( second ) == 2 &&
            nl->IsAtom( nl->First( second )) &&
            nl->AtomType( nl->First( second )) == RealType &&
            nl->IsAtom( nl->Second( second )) &&
            nl->AtomType( nl->Second( second )) == RealType)
      // if the second list element contains two real-values, representing 
      // point-coordinates
      { 
        correct = true;
        Point *p = (Point *)InPoint( nl->TheEmptyList(), second, 
                                        errorPos, errorInfo, correct ).addr;
        if ( !correct )
        {
          errmsg = "InCPoint(): Second instant must be a representation" 
                         "of a point value.";
          errorInfo = nl->Append(errorInfo, nl->StringAtom(errmsg));
          delete p;
          return SetWord( Address(0) );
        }
        
        CPoint cpoint( nl->RealValue( first ), (StandardAttribute*) p );
        delete p;
        correct = cpoint.IsValid();
      }
      else
      {
        correct = false;
        errmsg = "InCPoint(): Second instant must be a representation" 
                         "of a point value.";
        errorInfo = nl->Append(errorInfo, nl->StringAtom(errmsg));
      }
    }
    else
    {
      correct = false;
      errmsg = "InCPoint(): Error in first instant. First instant must be an "
            "atomic value of type Real.";
      errorInfo = nl->Append(errorInfo, nl->StringAtom(errmsg));
    }
  }
  errmsg = "InCPoint(): List must contain 2 elements. ";
  errorInfo = nl->Append(errorInfo, nl->StringAtom(errmsg));
  return SetWord( Address(0) );
}


/*
~Create~-function

*/
Word CreateCPoint( const ListExpr typeInfo )
{
  return (SetWord( new CPoint() ));
}


/*
~Delete~-function

*/
void DeleteCPoint( const ListExpr typeInfo, Word& w )
{
  delete (CPoint *)w.addr;
  w.addr = 0;
}


/*
~Close~-function

*/
void CloseCPoint( const ListExpr typeInfo, Word& w )
{
  delete (CPoint *)w.addr;
  w.addr = 0;
}


/*
~Clone~-function

*/
Word CloneCPoint( const ListExpr typeInfo, const Word& w )
{
  CPoint *cpoint = (CPoint *)w.addr;
  return SetWord( new CPoint( *cpoint ) );
}


/*
~Sizeof~-function

*/
int SizeOfCPoint()
{
  return sizeof(CPoint);
}


/*
~Cast~-function

*/

void * CastCPoint(void* addr)
{
  return new (addr) CPoint;
}

/*
Creation of the type constructor ~cpoint~

*/

TypeConstructor uncertainpoint(
        "cpoint",                //name
        CPointProperty,     //property function describing signature
        OutCPoint,
        // For consequent implementation, the Out-function in the previous line
        // should be 'OutUncertain<Point, OutPoint>' instead of 'OutCPoint', 
        // but the use of 'OutUncertain...' leads to a compiler error according
        // to this template-function in 'HierarchicalGeoAlgebra.h'! See there 
        // for further information! (Sascha Vaut)
        InUncertain<Point, InPoint>,               //Out and In functions
        0,
        0,                         //SaveToList and RestoreFromList functions
        CreateUncertain<Point>,
        DeleteUncertain<Point>,        //object creation and deletion
        0,
        0,                         // object open and save
        CloseUncertain<Point>,   
        CloneUncertain<Point>,         //object close and clone
        CastUncertain<Point>,           //cast function
        SizeOfUncertain<Point>,       //sizeof function
        CheckCPoint );      //kind checking function


/*
4.2 The Type Constructor ~cupoint~

Type ~cupoint~ represents a pair (epsilon, (tinterval, (x0, y0, x1, y1)))
consisting of an uncertainty-value and a value of type upoint.

4.2.1 List Representation

The list representation of an ~upoint~ is

----   ( epsilon ( timeinterval (x0 yo x1 y1) ) )
----

For example:

----    ( 37.5 ( ( (instant 6.37)  (instant 9.9)   TRUE FALSE)   
----                    (1.0 2.3 4.1 2.1) ) )

4.2.2 function Describing the Signature of the Type Constructor

*/
ListExpr CUPointProperty()
{
  return (nl->TwoElemList(
          nl->FourElemList(nl->StringAtom("Signature"),
                  nl->StringAtom("Example Type List"),
                  nl->StringAtom("List Rep"),
                  nl->StringAtom("Example List")),
          nl->FourElemList(nl->StringAtom("-> UNCERTAIN UNIT"),
                  nl->StringAtom("(cupoint) "),
                  nl->TextAtom("( epsilon ( timeInterval "
                          "(real_x0 real_y0 real_x1 real_y1) ) ) "),
                  nl->StringAtom("(0.7 ((i1 i2 TRUE FALSE)" 
                          "(1.0 2.2 2.5 2.1)))"))));
}


/*
4.2.3 Kind Checking Function

*/
bool CheckCUPoint( ListExpr type, ListExpr& errorInfo )
{
  return (nl->IsEqual( type, "cupoint" ));
}

/*
4.2.4 ~Out~-function

*/
ListExpr OutCUPoint( ListExpr typeInfo, Word value )
{
  CUPoint* cupoint = (CUPoint*)(value.addr);
  
  if( !(((CUPoint*)value.addr)->value.IsValid()) )
    return (nl->SymbolAtom("undef"));
  else
    {
      ListExpr timeintervalList = nl->FourElemList(
          OutDateTime( nl->TheEmptyList(),
          SetWord(&cupoint->value.timeInterval.start) ),
          OutDateTime( nl->TheEmptyList(), 
                  SetWord(&cupoint->value.timeInterval.end) ),
          nl->BoolAtom( cupoint->value.timeInterval.lc ),
          nl->BoolAtom( cupoint->value.timeInterval.rc));

      ListExpr pointsList = nl->FourElemList(
          nl->RealAtom( cupoint->value.p0.GetX() ),
          nl->RealAtom( cupoint->value.p0.GetY() ),
          nl->RealAtom( cupoint->value.p1.GetX() ),
          nl->RealAtom( cupoint->value.p1.GetY() ));
      
      ListExpr unitpointList = nl->TwoElemList(
          timeintervalList, pointsList );
          
      return nl->TwoElemList( nl->RealAtom( cupoint->GetEpsilon() ), 
          unitpointList );
    }
}

/*
4.2.5 ~In~-function

The Nested list form is like this:  
  ( 37.4 ( ( 6.37  9.9  TRUE FALSE)   (1.0 2.3 4.1 2.1) ) )

*/
Word InCUPoint( const ListExpr typeInfo, const ListExpr instance,
               const int errorPos, ListExpr& errorInfo, bool& correct )
{
  string errmsg;
  if ( nl->ListLength( instance ) == 2 )    
  // 2 arguments are necessary: epsilon and a upoint
  {
    ListExpr first = nl->First( instance );               // the epsilon value
    ListExpr second = nl->Second( instance );    // the upoint representation
    
    if ( nl->IsAtom( first ) && nl->AtomType( first ) == RealType )
    {
      if ( nl->ListLength( second ) == 2 )
      // the upoint also consists of two components...
      {
        ListExpr tintvl = nl->First( second );        // the time-interval
        ListExpr endpoints = nl->Second( second );     // the two point values
      
        if( nl->ListLength( tintvl ) == 4 &&
            nl->IsAtom( nl->Third( tintvl ) ) &&
            nl->AtomType( nl->Third( tintvl ) ) == BoolType &&
            nl->IsAtom( nl->Fourth( tintvl ) ) &&
            nl->AtomType( nl->Fourth( tintvl ) ) == BoolType )
        {
          correct = true;
          Instant *start = (Instant *)InInstant( nl->TheEmptyList(),
             nl->First( tintvl ), errorPos, errorInfo, correct ).addr;
    
          if( !correct )
          {
            errmsg = "InCUPoint(): Error in time-interval defining instant.";
            errorInfo = nl->Append(errorInfo, nl->StringAtom(errmsg));
            delete start;
            return SetWord( Address(0) );
          }
      
          Instant *end = (Instant *)InInstant( nl->TheEmptyList(),
              nl->Second( tintvl ), errorPos, errorInfo, correct ).addr;
      
          if( !correct )
          {
            errmsg = "InCUPoint(): Error in time-interval defining instant.";
            errorInfo = nl->Append(errorInfo, nl->StringAtom(errmsg));
            delete start;
            delete end;
            return SetWord( Address(0) );
          }
    
          Interval<Instant> tinterval( *start, *end,
                                       nl->BoolValue( nl->Third( tintvl ) ),
                                       nl->BoolValue( nl->Fourth( tintvl ) ) );
          delete start;
          delete end;
        
          correct = tinterval.IsValid();
          if (!correct)
          {
            errmsg = "InCUPoint(): Non valid time interval.";
            errorInfo = nl->Append(errorInfo, nl->StringAtom(errmsg));
            return SetWord( Address(0) );
          }

          if( nl->ListLength( endpoints ) == 4 &&
              nl->IsAtom( nl->First( endpoints ) ) &&
              nl->AtomType( nl->First( endpoints ) ) == RealType &&
              nl->IsAtom( nl->Second( endpoints ) ) &&
              nl->AtomType( nl->Second( endpoints ) ) == RealType &&
              nl->IsAtom( nl->Third( endpoints ) ) &&
              nl->AtomType( nl->Third( endpoints ) ) == RealType &&
              nl->IsAtom( nl->Fourth( endpoints ) ) &&
              nl->AtomType( nl->Fourth( endpoints ) ) == RealType )
          {
            CUPoint *cupoint = new CUPoint( nl->RealValue( first ), 
                               tinterval,
                               nl->RealValue( nl->First( endpoints ) ),
                               nl->RealValue( nl->Second( endpoints ) ),
                               nl->RealValue( nl->Third( endpoints ) ),
                               nl->RealValue( nl->Fourth( endpoints ) ) );
    
            correct = cupoint->value.IsValid();
            if( correct )
              return SetWord( cupoint );
        
            errmsg = errmsg + "InCUPoint(): Error in start/end point.";
            errorInfo = nl->Append(errorInfo, nl->StringAtom(errmsg));
            delete cupoint;
          }
        }
      }
    }
  }
  else if ( nl->IsAtom( instance ) && nl->AtomType( instance ) == SymbolType
            && nl->SymbolValue( instance ) == "undef" )
  {
    CUPoint *cupoint = new CUPoint(true);
    cupoint->SetDefined(false);
    cupoint->value.timeInterval=
      Interval<DateTime>(DateTime(instanttype),
                         DateTime(instanttype),true,true);
    correct = cupoint->value.timeInterval.IsValid();
    if ( correct )
      return (SetWord( cupoint ));
  }
  errmsg = "InCUPoint(): Error in representation.";
  errorInfo = nl->Append(errorInfo, nl->StringAtom(errmsg));
  correct = false;
  return SetWord( Address(0) );
}


/*
4.2.6 ~Create~-function

*/
Word CreateCUPoint( const ListExpr typeInfo )
{
  return (SetWord( new CUPoint() ));
}

/*
4.2.7 ~Delete~-function

*/
void DeleteCUPoint( const ListExpr typeInfo, Word& w )
{
  delete (CUPoint *)w.addr;
  w.addr = 0;
}

/*
4.2.8 ~Close~-function

*/
void CloseCUPoint( const ListExpr typeInfo, Word& w )
{
  delete (CUPoint *)w.addr;
  w.addr = 0;
}

/*
4.2.9 ~Clone~-function

*/
Word CloneCUPoint( const ListExpr typeInfo, const Word& w )
{
  CUPoint *cupoint = (CUPoint *)w.addr;
  return SetWord( new CUPoint( *cupoint ) );
}

/*
4.2.10 ~Sizeof~-function

*/
int SizeOfCUPoint()
{
  return sizeof(CUPoint);
}

/*
4.2.11 ~Cast~-function

*/
void* CastCUPoint( void* addr ) 
{
  return (new (addr) CUPoint);
}

/*
Creation of the type constructor ~cpoint~

*/

TypeConstructor uncertainunitpoint(
        "cupoint",                //name
        CUPointProperty,     //property function describing signature
        OutCUPoint,
        // For consequent implementation, the Out-function in the previous line
        // should be 'OutUncertain<Point, OutPoint>' instead of 'OutCPoint',
        // but the use of 'OutUncertain...' leads to a compiler error according
        // to this template-function in 'HierarchicalGeoAlgebra.h'! See there 
        // for further information! (Sascha Vaut)
        InCUPoint,               //Out and In functions
        0,
        0,                         //SaveToList and RestoreFromList functions
        CreateUncertain<UPoint>,
        DeleteUncertain<UPoint>,        //object creation and deletion
        0,
        0,                         // object open and save
        CloseUncertain<UPoint>,   
        CloneUncertain<UPoint>,         //object close and clone
        CastUncertain<UPoint>,           //cast function
        SizeOfUncertain<UPoint>,       //sizeof function
        CheckCUPoint );      //kind checking function


/*
Type Constructor +++++ hier weitere Typkonstruktoren anfuegen +++++


Type mapping functions

A type mapping function takes a nested list as argument. Its contents are
type descriptions of an operator's input parameters. A nested list describing
the output type of the operator is returned.


Type mapping function ~UncertainTypeMapReal~

This type mapping function is used for the Operation ~Epsylon()~.

*/
ListExpr UncertainTypeMapReal( ListExpr args )
{
  if ( nl->ListLength( args ) == 1 )
  {
    ListExpr arg1 = nl->First( args );
    
    if ( nl->IsEqual( arg1, "cpoint" ) || 
          nl->IsEqual( arg1, "cupoint" ) )
      return nl->SymbolAtom("real");
    if (nl->AtomType( args ) == SymbolType)
    {
      ErrorReporter::ReportError("Type mapping function got a "
              "parameter of type " +nl->SymbolValue(args) + ".");
      return nl->SymbolAtom("typeerror");
    }
  }
  ErrorReporter::ReportError("Type mapping function got a "
        "parameter of length != 1.");
  return nl->SymbolAtom("typeerror");
}

/*
Type mapping function ~UncertainTypeMapBase~

This type mapping function is used for the Operation ~Val()~. The keyword
'base' indicates a reduction of an uncertain type to its particular base type.
So in this case a 'base type' can also be a spatial or temporal type.

*/

ListExpr UncertainTypeMapBase( ListExpr args )
{
  if ( nl->ListLength( args ) == 1 )
  {
    ListExpr arg1 = nl->First( args );
    
    if( nl->IsEqual( arg1, "cpoint") )
      return nl->SymbolAtom( "point" );
      
    if( nl->IsEqual( arg1, "cupoint") )
      return nl->SymbolAtom( "upoint" );
    if (nl->AtomType( args ) == SymbolType)
    {
      ErrorReporter::ReportError("Type mapping function got a "
              "parameter of type " +nl->SymbolValue(args) + 
              "which is no uncertain type.");
      return nl->SymbolAtom("typeerror");
    }
  }
  ErrorReporter::ReportError("Type mapping function got a "
        "parameter of length != 1.");
  return nl->SymbolAtom( "typeerror" );
}


/*
Type mapping function ~CertainToUncertain~

This type mapping function is used for the ~<certaintype>to<uncertaintype>~ 
Operations.

*/

ListExpr CertainToUncertain( ListExpr args )
{
  if ( nl->ListLength( args ) == 2 )
  {
    ListExpr first = nl->First(args);
    ListExpr second = nl->Second(args);
    if ( nl->IsEqual( first, "real" ) )
    {
      // find out if the second argument is of an implemented uncertain type
      if ( nl->IsEqual( second, "point" ) )
        return nl->SymbolAtom("cpoint");
      if ( nl->IsEqual( second, "upoint" ) )
        return nl->SymbolAtom("cupoint");
    }
    if ( (nl->AtomType(first) == SymbolType) && (nl->AtomType(second) == 
            SymbolType))
      ErrorReporter::ReportError("Type mapping function got parameters of "
        "type "
          + nl->SymbolValue(first) + " and "
          + nl->SymbolValue(second));
    else
      ErrorReporter::ReportError("Type mapping function got wrong types "
        "as parameters.");
  }
  ErrorReporter::ReportError("Type mapping function got a parameter of length "
    "!= 2.");
  return nl->SymbolAtom("typeerror");
}


/*
16.2 Selection function

A selection function is quite similar to a type mapping function. The only
difference is that it doesn't return a type but the index of a value
mapping function being able to deal with the respective combination of
input parameter types.

Note that a selection function does not need to check the correctness of
argument types; it has already been checked by the type mapping function that
it is applied to correct arguments.

16.2.1 Selection function ~UncertainSimpleSelect~

Is used for the ~epsilon~ and ~val~ operators.

*/
int UncertainSimpleSelect( ListExpr args )
{
  ListExpr arg1 = nl->First( args );
  
  if( nl->SymbolValue( arg1 ) == "cpoint" )
    return 0;
    
  if( nl->SymbolValue( arg1 ) == "cupoint" )
    return 1;
    
  // ...space for further possible argument types
  
  return -1; // This point should never be reached
}



/*
Value mapping functions

Value mapping functions for class cpoint


Value mapping function for operator ~tocpoint~

*/

int ToCPoint( Word* args, Word& result, int message, Word& local,
                                        Supplier s )
{
  result = qp->ResultStorage( s );
  CcReal* epsilon = (CcReal*)args[0].addr;
  Point* p = (Point*)args[1].addr;
  
  if ( epsilon >= 0 )
    if ( p->IsDefined() )
    {
      CPoint cp( epsilon->GetValue(), (StandardAttribute*) p );
      ((CPoint*)result.addr)->Set(cp);
    }
    else
    {
      ((CPoint*)result.addr)->SetDefined( false );
      cerr << "Result object is set to state: defined = false." << endl;
    }
  return 0;
}



/*
Definition of operators

Definition of operators is done in a way similar to definition of 
type constructors: an instance of class ~Operator~ is defined.

Because almost all operators are overloaded, we have first to define an 
array of value mapping functions for each operator. For nonoverloaded
operators there is also such an array defined, so it is easier to make them
overloaded.

ValueMapping arrays

*/

ValueMapping uncertainepsilonmap[] = { 
                                      UncertainEpsilon<Point>,
                                      UncertainEpsilon<UPoint> };


ValueMapping uncertainvalmap[] = {
                                      UncertainVal<Point>,
                                      UncertainVal<UPoint> };
/*
Specification strings

*/

const string UncertainSpecEpsilon  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>uT -> epsilon</text--->"
  "<text>epsilon ( _ )</text--->"
  "<text>Returns an uncertain values' epsilon value.</text--->"
  "<text>epsilon ( i1 )</text--->"
  ") )";


const string UncertainSpecVal =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>uT -> x</text--->"
  "<text>val ( _ )</text--->"
  "<text>Returns an uncertain value's value.</text--->"
  "<text>val ( i1 )</text--->"
  ") )";


const string CPointSpecToCPoint =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>point, real -> cpoint</text--->"
  "<text>toCPoint ( _, _ )</text--->"
  "<text>Builds a new CPoint from the given Real- and Point-values.</text--->"
  "<text>cpt = tocpoint ( 50.0, alexanderplatz )</text--->"
  ") )";


/*
Operators

*/

Operator uncertainepsilon( "epsilon",
                              UncertainSpecEpsilon,
                              2,
                              uncertainepsilonmap,
                              UncertainSimpleSelect,
                              UncertainTypeMapReal );

Operator uncertainval( "val",
                              UncertainSpecVal,
                              2,
                              uncertainvalmap,
                              UncertainSimpleSelect,
                              UncertainTypeMapBase );
                             
Operator tocpoint( "tocpoint",
                              CPointSpecToCPoint,
                              ToCPoint,
                              Operator::SimpleSelect,
                              CertainToUncertain );
                              
/*
Creating the Algebra
 
*/
class HierarchicalGeoAlgebra : public Algebra
{
  public:
  HierarchicalGeoAlgebra() : Algebra()
  {
    AddTypeConstructor( &uncertainpoint );
    uncertainpoint.AssociateKind( "UNCERTAIN" );
    //uncertainpoint.AssociateKind( "SPATIAL" );
    AddTypeConstructor( &uncertainunitpoint );
    uncertainunitpoint.AssociateKind( "UNCERTAIN" );
    //uncertainunitpoint.AssociateKind( "TEMPORAL" );
    AddOperator( &uncertainepsilon );
    AddOperator( &uncertainval );
    AddOperator( &tocpoint );
  }
  ~HierarchicalGeoAlgebra() {};
};
HierarchicalGeoAlgebra hierarchicalGeoAlgebra;

/*
Initialization

Each algebra module needs an initialization function. The algebra manager
has a reference to this function if this algebra is included in the list
of required algebras, thus forcing the linker to include this module.

The algebra manager invokes this function to get a reference to the instance
of the algebra class and to provide references to the global nested list
container (used to store constructor, type, operator and object information)
and to the query processor.

The function has a C interface to make it possible to load the algebra
dynamically at runtime.

*/

extern "C"
Algebra*
InitializeHierarchicalGeoAlgebra( NestedList* nlRef, 
                                    QueryProcessor* qpRef )
{
  nl = nlRef;
  qp = qpRef;
  return (&hierarchicalGeoAlgebra);
}

