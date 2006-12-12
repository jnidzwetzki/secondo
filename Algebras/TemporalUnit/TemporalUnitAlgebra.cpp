/*

//paragraph [1] Title: [{\Large \bf \begin{center}] [\end{center}}]
//paragraph [10] Footnote: [{\footnote{] [}}]
//[ue] [\"u]
//[ae] [\"a]

//[TOC] [\tableofcontents]

[1] TemporalUnitAlgebra - Implementing Units-Operators

May 2006, initial version implemented by Thomas Fischer for diploma
thesis with Prof. Dr. G[ue]ting, Fachbereich Informatik,
Feruniversit[ae]t Hagen.

July 2006, Christian D[ue]ntgen: The so far implemented operators do not suffice
to model typical queries using the compact unit representation ~moving(T)~ with
relations of units. Instead, we also need variants of spatiotemporal operators
that process streams of units. Instead of implementing them directly, we will
fake them using the set wrapper-operators ~use~ on the native single-unit
operators.

The ~use~ operators passes single values to an operator one-by-one value and
collects all returned values/streams of values in a flat stream of values.
This does not work for some binary predicates, like equal, but one could implement
an ordered pairwise comparison here.

It may be useful, to have some operators consuming a stream of units and
returning an aggregated vale, as e.g. initial, final, present, never, always.

----

State Operator/Signatures

OK    makemvalue   (**)  stream (tuple ([x1:t1,xi:uT,..,xn:tn])) -->  mT
OK    get_duration                   periods --> duration
(OK)  point2d                        periods --> point
(OK)  queryrect2d                    instant --> rect
OK    circle              point x real x int --> region
OK    uint2ureal:                       uint --> ureal
      the_unit:  For T in {bool, int, string}
Test          point  point  instant instant bool bool --> ubool
Test          ipoint ipoint bool    bool              --> ubool
Test          real real real bool instant instant bool bool --> ureal
Test          iT duration bool bool       --> uT
Test          T instant instant bool bool --> uT

OK    isempty                             U  --> bool (for U in kind UNIT)
OK    trajectory                      upoint --> line
OK    velocity                        mpoint --> mpoint
OK    velocity                        upoint --> upoint
OK    derivable                        mreal --> mbool
OK    derivable                        ureal --> ubool
OK    derivative                       mreal --> mreal
OK    derivative                       ureal --> ureal
OK    speed                           mpoint --> mreal
OK    speed                           upoint --> ureal

      passes:  For T in {bool, int, string, point}:
OK  +                            uT x      T --> bool
n/a +                         ureal x   real --> bool
n/a +                       uregion x region --> bool

OK  + deftime      (**)                   uT --> periods
OK  + atinstant    (**)         uT x instant --> iT
OK  + atperiods    (**)         uT x periods --> (stream uT)
OK  + Initial      (**)                   uT --> iT
OK    Initial      (**)          (stream uT) --> iT
OK  + final        (**)                   uT --> iT
OK    final        (**)          (stream uT) --> iT
OK  + present      (**)         uT x instant --> bool
OK  +              (**)         uT x periods --> bool
n/a                    (stream uT) x instant --> bool (use use2/present)
n/a                    (stream uT) x periods --> bool (use use2/present)

      atmax:  For T in {bool, int, real, string}:
(OK)+                                     uT --> (stream uT)

      atmin:  For T in {bool, int, real, string}:
(OK)+                                     uT --> (stream uT)

      at:     For T in {bool, int, string, point, region*}
OK  +                           uT x       T --> uT
(OK)                         ureal x    real --> (stream ureal)
OK  +                       upoint x  region --> (stream upoint)

      distance:  T in {int, point}
OK  -           uT x uT -> ureal
OK  ?           uT x  T -> ureal
OK  ?            T x uT -> ureal

     intersection: For T in {bool, int, string}:
OK  +          uT x      uT --> (stream uT)
OK  +          uT x       T --> (stream uT)
OK  +           T x      uT --> (stream uT)
(OK)+       ureal x    real --> (stream ureal)
(OK)+        real x   ureal --> (stream ureal)
OK  -      upoint x   point --> (stream upoint) same as at: upoint x point
OK  -       point x  upoint --> (stream upoint) same as at: upoint x point
OK  -      upoint x  upoint --> (stream upoint)
OK  +      upoint x    line --> (stream upoint)
OK  +        line x  upoint --> (stream upoint)
Pre +       ureal x   ureal --> (stream ureal)
Test+      upoint x uregion --> (stream upoint)
Test-     uregion x  upoint --> (stream upoint)
Pre -      upoint x  region --> (stream upoint)
Pre -      region x  upoint --> (stream upoint)
n/a +      upoint x  points --> (stream upoint)
n/a +      points x  upoint --> (stream upoint)

OK  + no_components:     uT --> uint

OK  + and, or: ubool x ubool --> ubool
OK  +           bool x ubool --> ubool
OK  +          ubool x  bool --> ubool

      =, #, <, >, <=, >=:
OK  +                uT x uT --> bool

OK  + not:             ubool --> ubool

  inside:
Test+      upoint x uregion --> (stream ubool)
pre +      upoint x    line --> (stream ubool)
pre +      upoint x  points --> (stream ubool)
pre +     uregion x  points --> (stream ubool)

n/a + mdirection:    upoint --> ureal

n/a + area: uregion --> ureal             see TemporalLiftedAlgebra

Test+ sometimes: (       ubool) --> bool
Test             (stream ubool) --> bool
Test+ never:     (       ubool) --> bool
Test             (stream ubool) --> bool
Test+ always:    (       ubool) --> bool
Test             (stream ubool) --> bool

COMMENTS:

(*):  These operators have been implemented for
      T in {bool, int, real, point}
(**): These operators have been implemented for
      T in {bool, int, real, point, string, region}

Key to STATE of implementation:

   OK : Operator has been implemented and fully tested
  (OK): Operator has been implemented and partially tested
  Test: Operator has been implemented, but tests have not been done
  Pre : Operator has not been functionally implemented, but
        stubs (dummy code) exist
  n/a : Neither functionally nor dummy code exists for this ones

    + : Equivalent exists for according mType
    - : Does nor exist for according mType
    ? : It is unclear, whether it exists or not

----

*/

/*

August 2006, Christian D[ue]ntgen: Added missing checks for ~undefined~
argument values to all value mapping functions. The functions will now
return ~undefined~ result values for these cases.

Changed structure of the file to become ordered by operators rather than by
typemapping, valuemapping etc. This makes the file easier to extend.

*/

/*
0. Bug-List

----

 (none known)

Key:
 (C): system crash
 (R): Wrong result

----

*/

/*

[TOC]

1 Overview

This file contains the implementation of the unit operators and
helping operators for indexing instant values in R-trees.

2 Defines, includes, and constants

*/


#include <cmath>
#include <stack>
#include <limits>
#include <sstream>
#include <vector>

#include "NestedList.h"
#include "NList.h"
#include "QueryProcessor.h"
#include "AlgebraManager.h"
#include "Algebra.h"
#include "StandardTypes.h"
#include "SpatialAlgebra.h"
#include "RelationAlgebra.h"
#include "TemporalAlgebra.h"
#include "TemporalExtAlgebra.h"
#include "MovingRegionAlgebra.h"
#include "RectangleAlgebra.h"


extern NestedList* nl;
extern QueryProcessor* qp;
extern AlgebraManager* am;


#include "DateTime.h"
using namespace datetime;

bool TUA_DEBUG = false; // Set to true to activate debugging code
//bool TUA_DEBUG = true; // Set to true to activate debugging code

/*
2.1 Definition of some constants and auxiliary functions

*/
const double MAXDOUBLE = numeric_limits<double>::max();
const double MINDOUBLE = numeric_limits<double>::min();

/*
2.2 Auxiliary output functions

*/

// make a string from a numeric value
string TUn2s(const double& i)
{
  std::stringstream ss;
  std::string str;
  ss << i;
  ss >> str;
  return str;
}

// make a string representation from a time interval
string TUPrintTimeInterval( Interval<DateTime> iv )
{
  string Result;

  if (iv.lc)
    Result += "[";
  else
    Result += "]";
  Result += iv.start.ToString();
  Result += ", ";
  Result += iv.end.ToString();
  if (iv.rc)
    Result += "]";
  else
    Result += "[";
  return Result;
}

// make a string representation from a point
string TUPrintPoint( const Point& p )
{
  string Result;

  if ( p.IsDefined() )
    Result = "( def  : ";
  else
    Result = "( undef: ";
  Result += TUn2s(p.GetX());
  Result += "/";
  Result += TUn2s(p.GetY());
  Result += " )";

  return Result;
}

// make a string representation from an upoint value
string TUPrintUPoint( const UPoint& upoint )
{
  std::string Result;

  if ( upoint.IsDefined() )
    Result = "( def  : ";
  else
    Result = "( undef: ";
  Result += TUPrintTimeInterval(upoint.timeInterval);
  Result += ", ";
  Result += TUPrintPoint(upoint.p0);
  Result += ", ";
  Result += TUPrintPoint(upoint.p1);
  Result +=" )";
  Result += ((Attribute*)(&upoint))->AttrDelete2string();
  return Result;
}

// make a string representation from an ureal value
string TUPrintUReal( UReal* ureal )
{
  std::string Result;

  if ( ureal->IsDefined() )
    Result = "( def  : ";
  else
    Result = "( undef: ";
  Result += TUPrintTimeInterval(ureal->timeInterval);
  Result += ", ";
  Result += TUn2s(ureal->a);
  Result += ", ";
  Result += TUn2s(ureal->b);
  Result += ", ";
  Result += TUn2s(ureal->c);
  if (ureal->r)
    Result += ", true )";
  else
    Result +=", false )";
  Result += ureal->AttrDelete2string();
  return Result;
}

/*
3 Implementation of the unit class method operators

This section implements operators as member functions of the respective
unit class.

3.1 Operator ~speed~

*/
void MPoint::MSpeed( MReal& result ) const
{
  const UPoint *uPoint;
  UReal uReal(true);
  //  int counter = 0;

  result.Clear();
  if ( !IsDefined() )
    {
      // result.SetDefined( false ); // no undef Mappings by now
      if(TUA_DEBUG) cout << "\nMPoint::MSpeed is undef (undef arg)." << endl;
    }
  else
    {
      result.StartBulkLoad();

      for( int i = 0; i < GetNoComponents(); i++ )
        {
          Get( i, uPoint );

          uPoint->USpeed( uReal );
          if( uReal.IsDefined() )
            {
              result.Add( uReal ); // append ureal to mreal
              //              counter++;
            }
        }
      result.EndBulkLoad( true );
/*
Activating the following snippet would allow for creating undef objects
instead of empty ones. Alas, the SetDefined() and IsDefined() methods do
not have functional implementations by now. Instead, ~undef~ values are
substituted by ~empty~ mappings.

----
      if(counter == 0)
        {
          result.SetDefined( false );
          if(TUA_DEBUG) cout << "\nMPoint::MSpeed is undef (empty result)." << endl;
        }
      else
        {
          result.SetDefined( true );
          if(TUA_DEBUG) cout << "\nMPoint::MSpeed is defined." << endl;
        }

----

*/
    }
  if(TUA_DEBUG) cout << "MPoint::MSpeed() finished!" << endl;
}


/*
3.2 Operator ~Velocity~

*/
void MPoint::MVelocity( MPoint& result ) const
{
  const UPoint *uPoint;
  UPoint p(true);
  //  int counter = 0;

  result.Clear();
  if ( !IsDefined() )
    {
      // result.SetDefined( false ); // no undef Mappings by now
      if(TUA_DEBUG) cout << "\nMPoint::MVelocity: undef unit" << endl;
    }
  else
    {
      result.StartBulkLoad();
      for( int i = 0; i < GetNoComponents(); i++ )
        {
          Get( i, uPoint );
          /*
            Definition of a new point unit p. The velocity is constant
            at all times of the interval of the unit. This is exactly
            the same as within operator ~speed~. The result is a vector
            and can be represented as a upoint.

          */

          uPoint->UVelocity( p );
          if( p.IsDefined() )
            {
              result.Add( p );
              //              counter++;
            }
        }
      result.EndBulkLoad( true );
    }
  if(TUA_DEBUG) cout << "MPoint::MVelocity() finished!" << endl;
}



/*
4 General Selection functions

A selection function is quite similar to a type mapping function. The only
difference is that it doesn't return a type but the index of a value
mapping function within the operator's ~ValueMapping~ array, being able to deal
with the respective combination of input parameter types.

Non-overloaded opertors (having only one single signature) do not need a
selection function or a ValueMapping array.

Note that a selection function does not need to check the correctness of
argument types; it has already been checked by the type mapping function that it
is applied to correct arguments.

Only some more general selection functions are listed here. More of them can
be found in the operator implementation section below.

4.1 Selection function ~UnitSimpleSelect~

Is used for the ~deftime~,~atinstant~,~atperiods~ operations.

*/
int
UnitSimpleSelect( ListExpr args )
{
  ListExpr T = nl->First( args );
  if( nl->SymbolValue( T ) == "ubool" )
    return 0;
  if( nl->SymbolValue( T ) == "uint" )
    return 1;
  if( nl->SymbolValue( T ) == "ureal" )
    return 2;
  if( nl->SymbolValue( T ) == "upoint" )
    return 3;
  if( nl->SymbolValue( T ) == "ustring" )
    return 4;
  if( nl->SymbolValue( T ) == "uregion" )
    return 5;

  return -1; // This point should never be reached
}

/*

4.2 Selection function ~UnitCombinedUnitStreamSelect~

This extended version of ~UnitSimpleSelect~ can map (unit) as well as
(stream unit):

*/

int
UnitCombinedUnitStreamSelect( ListExpr args )
{
  ListExpr arg1 = nl->First( args );

  if (nl->IsAtom( arg1 ) )
    {
      if( nl->SymbolValue( arg1 ) == "ubool" )
        return 0;
      if( nl->SymbolValue( arg1 ) == "uint" )
        return 1;
      if( nl->SymbolValue( arg1 ) == "ureal" )
        return 2;
      if( nl->SymbolValue( arg1 ) == "upoint" )
        return 3;
      if( nl->SymbolValue( arg1 ) == "ustring" )
        return 4;
      if( nl->SymbolValue( arg1 ) == "uregion" )
        return 5;
    }

  if(   !( nl->IsAtom( arg1 ) )
      && ( nl->ListLength(arg1) == 2 )
      && ( TypeOfRelAlgSymbol(nl->First(arg1)) == stream ) )
    { if( nl->IsEqual( nl->Second(arg1), "ubool" ) )
        return 6;
      if( nl->IsEqual( nl->Second(arg1), "uint" ) )
        return 7;
      if( nl->IsEqual( nl->Second(arg1), "ureal" ) )
        return 8;
      if( nl->IsEqual( nl->Second(arg1), "upoint" ) )
        return 9;
      if( nl->IsEqual( nl->Second(arg1), "ustring" ) )
        return 10;
      if( nl->IsEqual( nl->Second(arg1), "uregion" ) )
        return 11;
    }

  return -1; // This point should never be reached
}


/*
5 Implementation of Algebra Operators

5.1 Operator ~speed~

5.1.1 Type mapping function for ~speed~

Type mapping for ~speed~ is

----  mpoint  [->]  mreal

----

*/
ListExpr
TypeMapSpeed( ListExpr args )
{
  if ( nl->ListLength( args ) == 1 )
  {
    ListExpr arg1 = nl->First( args );

    if( nl->IsEqual( arg1, "mpoint" )  )
      return nl->SymbolAtom( "mreal" );
    if( nl->IsEqual( arg1, "upoint" )  )
      return nl->SymbolAtom( "ureal" );
  }
  return nl->SymbolAtom( "typeerror" );
}

/*
5.1.2 Value mapping for operator ~speed~

*/

int MPointSpeed(Word* args, Word& result, int message, Word& local, Supplier s)
{
  result = (qp->ResultStorage( s ));
  MReal  *res   = (MReal*)result.addr;
  MPoint *input = (MPoint*)args[0].addr;

  if ( input->IsDefined() )
    // call member function:
    input->MSpeed( *res );
  else
    {
      res->Clear();             // using empty mvalue instead
      //res->SetDefined(false); // of undef mvalue
    }
  return 0;
}

int UnitPointSpeed(Word* args,Word& result,int message,Word& local,Supplier s)
{
  result = qp->ResultStorage( s );
  UReal  *res   = (UReal*)result.addr;
  UPoint *input = (UPoint*)args[0].addr;

  if ( input->IsDefined() )
    { // call member function:
      input->USpeed( *res );
      if (TUA_DEBUG)
        cout << "UnitPointSpeed(): input def" << endl;
    }
  else
    {
      res->SetDefined(false);
      if (TUA_DEBUG)
        cout << "UnitPointSpeed(): input undef" << endl;
    }
  return 0;
}

/*
5.1.3 Specification for operator ~speed~

*/

const string
TemporalSpecSpeed  =
"( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
"( <text>mpoint -> mreal\n"
"upoint -> ureal</text--->"
"<text>speed( _ )</text--->"
"<text>return the speed of a temporal spatial object in "
"unit/s.</text--->"
"<text>query speed(mp1)</text---> ) )";

/*
5.1.4 Selection Function of operator ~speed~

*/
int
SpeedSelect( ListExpr args )
{
  ListExpr arg1 = nl->First( args );

  if( nl->SymbolValue( arg1 ) == "mpoint" )
    return 0;

  if( nl->SymbolValue( arg1 ) == "upoint" )
    return 1;

  return -1; // This point should never be reached
}

ValueMapping temporalspeedmap[] = { MPointSpeed,
                                    UnitPointSpeed };

/*
5.1.5 Definition of operator ~speed~

*/
Operator temporalspeed( "speed",
                      TemporalSpecSpeed,
                      2,
                      temporalspeedmap,
                      SpeedSelect,
                      TypeMapSpeed);

/*
5.2 Operator ~queryrect2d~

5.2.1 Type Mapping for ~queryresct2d~

Type mapping for ~queryrect2d~ is

----  instant  [->]  rect

----

*/
ListExpr
InstantTypeMapQueryrect2d( ListExpr args )
{
  if ( nl->ListLength( args ) == 1 )
  {
    ListExpr arg1 = nl->First( args );

    if( nl->IsEqual( arg1, "instant" )  )
      return nl->SymbolAtom( "rect" );
  }
  return nl->SymbolAtom( "typeerror" );
}

/*
5.2.2 Value Mapping for ~queryrect2d~

*/

int Queryrect2d(Word* args, Word& result, int message, Word& local, Supplier s)
{
  const unsigned dim = 2;
  double min[dim], max[dim];
  double timevalue;
  double mininst, maxinst;
  Instant* Inv = (Instant*)args[0].addr;
  DateTime tmpinst;

  tmpinst.ToMinimum();
  mininst = tmpinst.ToDouble();
  tmpinst.ToMaximum();
  maxinst = tmpinst.ToDouble();

  result = qp->ResultStorage( s );

  if (Inv->IsDefined()) // Inv is defined: create rect2
    {
      timevalue = Inv->ToDouble();

      min[0] = mininst;   // x1
      min[1] = timevalue; // x2
      max[0] = timevalue; // y1
      max[1] = maxinst;   // y2

      Rectangle<dim>* rect = new Rectangle<dim>(true, min, max);
      ((Rectangle<dim>*)result.addr)->CopyFrom(rect);
      delete rect;
    }
  else // Inv is undefined: return mininst^4
    {
      min[0] = mininst;
      min[1] = mininst;
      max[0] = mininst;
      max[1] = mininst;

      Rectangle<dim>* rect = new Rectangle<dim>(false, min, max);
      ((Rectangle<dim>*)result.addr)->CopyFrom(rect);
      delete rect;
    }

  return 0;
}

/*
5.2.3 Specification for operator ~queryrect2d~

*/

const string
TemporalSpecQueryrect2d  =
"( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
"( <text>(instant) -> rect</text--->"
"<text>queryrect2d( _ )</text--->"
"<text>Translate an instant object to a rect object to query the against "
"a periods object translated to a point using operator 'point2d'. The "
"undef instant is mapped to rect mininst^4</text--->"
"<text>query queryrect2d(instant)</text---> ) )";

/*
5.2.4 Selection Function of operator ~queryrect2d~

Not necessary.

*/

/*
5.2.5  Definition of operator ~queryrect2d~

*/

Operator temporalunitqueryrect2d( "queryrect2d",
                      TemporalSpecQueryrect2d,
                      Queryrect2d,
                      Operator::SimpleSelect,
                      InstantTypeMapQueryrect2d);

/*
5.3 Operator ~point2d~

5.3.1 Type Mapping for ~point2d~

Type mapping for ~point2d~ is

----  periods  [->]  point

----

*/

ListExpr
PeriodsTypeMapPoint2d( ListExpr args )
{

  if ( nl->ListLength( args ) == 1 )
  {
    ListExpr arg1 = nl->First( args );

    if( nl->IsEqual( arg1, "periods" )  )
      return nl->SymbolAtom( "point" );
  }
  return nl->SymbolAtom( "typeerror" );
}

/*
5.3.2 Value Mapping for ~point2d~

*/
int Point2d( Word* args, Word& result, int message, Word& local, Supplier s )
{
  double  X, Y;
  Instant sup, inf;

  result = qp->ResultStorage( s );
  Periods* range = (Periods*)args[0].addr;

  if ( !range->IsDefined() )
    ((Point*)result.addr)->SetDefined( false );
  else
    {
      X = 0;
      Y = 0;
      if( !range->IsEmpty()  )
        {
          const Interval<Instant> *intv1, *intv2;

          range->Get( 0, intv1 );
          range->Get( range->GetNoComponents()-1, intv2 );
          sup = intv1->end;
          inf = intv2->start;
          Y = sup.ToDouble(); // Derives the maximum of all intervals.
          X = inf.ToDouble(); // Derives the minimum of all intervals.
        }
      else
        {
          DateTime tmpinst = DateTime(0,0,instanttype);
          tmpinst.ToMinimum();
          X = tmpinst.ToDouble();
          Y = X;
        }
      ((Point*)result.addr)->SetDefined(true);
      ((Point*)result.addr)->Set(X,Y); // Returns the calculated point.
    }

  return 0;
}


/*
5.3.3 Specification for operator ~point2d~

*/
const string
TemporalSpecPoint2d  =
"( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
"( <text>(periods) -> point</text--->"
"<text>point2d( _ )</text--->"
"<text>Translate a periods value to a point value representing "
"the period's total deftime interval. The empty periods value "
"is mapped to the point corresponding to mininstant^2.</text--->"
"<text>query point2d(periods)</text---> ) )";


/*
5.3.4 Selection Function of operator ~point2d~

Not necessary.

*/

/*
5.3.5  Definition of operator ~point2d~

*/
Operator temporalunitpoint2d( "point2d",
                      TemporalSpecPoint2d,
                      Point2d,
                      Operator::SimpleSelect,
                      PeriodsTypeMapPoint2d);

/*
5.4 Operator ~get\_duration~

5.4.1 Type Mapping for ~get\_duration~

Type mapping for ~get\_duration~ is

----  periods  [->]  duration

----

*/
ListExpr
PeriodsTypeMapGetDuration( ListExpr args )
{
  if ( nl->ListLength( args ) == 1 )
  {
    ListExpr arg1 = nl->First( args );

    if( nl->IsEqual( arg1, "periods" )  )
      return nl->SymbolAtom( "duration" );
  }
  return nl->SymbolAtom( "typeerror" );
}

/*
5.4.2 Value Mapping for ~get\_duration~

*/
int GetDuration( Word* args, Word& result, int message,
                 Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Periods* range = (Periods*)args[0].addr;
  DateTime* Res = ((DateTime*)result.addr);
  *Res = DateTime(0, 0, durationtype);
  if ( !range->IsDefined() )
    ((DateTime*)result.addr)->SetDefined( false );
  else
    {
      Res->SetDefined(true);
      if( !range->IsEmpty()  )
        {
          const Interval<Instant> *intv;

          for( int i = 0; i < range->GetNoComponents(); i++ )
            {
              range->Get( i, intv );
              *Res += (intv->end - intv->start);
            }
        }
    }
  return 0;
}

/*
5.4.3 Specification for operator ~get\_duration~

*/
const string
TemporalSpecGetDuration  =
"( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
"( <text>(periods) -> real</text--->"
"<text>get_duration( _ )</text--->"
"<text>Return the duration in seconds spanned by a periods value "
"as a real value.</text--->"
"<text>query get_duration(periods)</text---> ) )";

/*
5.4.4 Selection Function of operator ~get\_duration~

Not necessary.

*/


/*
5.4.5  Definition of operator ~get\_duration~

*/
Operator temporalunitget_duration( "get_duration",
                      TemporalSpecGetDuration,
                      GetDuration,
                      Operator::SimpleSelect,
                      PeriodsTypeMapGetDuration );

/*
5.5 Operator ~makemvalue~

This operator creates a moving object type mT from a stream of unit type
objects uT. The operator does not expect the stream to be ordered by their
timeintervals. Also, undefined units are allowed (but will be ignored).
If the stream contains amindst 2 units with overlapping timeIntervals,
the operator might crash. If the stream is empty, the result will be an
empty mT.

5.5.1 Type Mapping for ~makemvalue~

Type mapping for ~makemvalue~ is

----  stream (tuple ([x1:t1,x1:ubool,..,[xn:tn)))   ->  mbool
              APPEND (i ti)
      stream (tuple ([x1:t1,x1:uint,..,[xn:tn)))    ->  mint
              APPEND (i ti)
      stream (tuple ([x1:t1,x1:ureal,..,[xn:tn)))   ->  mreal
              APPEND (i ti)
      stream (tuple ([x1:t1,x1:upoint,..,[xn:tn)))  ->  mpoint
              APPEND (i ti)
      stream (tuple ([x1:t1,x1:ustring,..,[xn:tn)))  ->  mstring
              APPEND (i ti)
      stream (tuple ([x1:t1,x1:uregion,..,[xn:tn)))  ->  movingregion
              APPEND (i ti)

----

*/
ListExpr MovingTypeMapMakemvalue( ListExpr args )

{
  ListExpr first, second, rest, listn,
           lastlistn, first2, second2, firstr, listfull, attrtype;

  int j;
  string argstr, argstr2, attrname, inputtype, inputname, fulllist;


  //check the list length.
  CHECK_COND(nl->ListLength(args) == 2,
             "Operator makemvalue expects a list of length two.");

  first = nl->First(args);
  nl->WriteToString(argstr, first);


  // check the structure of the list.
  CHECK_COND(nl->ListLength(first) == 2  &&
             (TypeOfRelAlgSymbol(nl->First(first)) == stream) &&
             (nl->ListLength(nl->Second(first)) == 2) &&
             (TypeOfRelAlgSymbol(nl->First(nl->Second(first))) == tuple) &&
             (nl->ListLength(nl->Second(first)) == 2) &&
             (IsTupleDescription(nl->Second(nl->Second(first)))),
  "Operator makemvalue expects as first argument a list with structure "
  "(stream (tuple ((a1 t1)...(an tn))))\n"
  "Operator makemvalue gets as first argument '" + argstr + "'." );


  // check the given parameter
  second  = nl->Second(args);
  nl->WriteToString(argstr, second);
  CHECK_COND(argstr != "typeerror",
  "Operator makemvalue expects a name of a attribute and not a type.");

  // inputname represented the name of the given attribute
  nl->WriteToString(inputname, second);

  rest = nl->Second(nl->Second(first));
  listn = nl->OneElemList(nl->First(rest));
  lastlistn = listn;
  firstr = nl->First(rest);
  rest = nl->Rest(rest);
  first2 = nl->First(firstr);
  second2 = nl->Second(firstr);
  nl->WriteToString(attrname, first2);
  nl->WriteToString(argstr2, second2);

  // compare with the attributes in the relation
  if (attrname == inputname)
     inputtype = argstr2;

  // save from the detected attribute the type
  while (!(nl->IsEmpty(rest)))
    {
      lastlistn = nl->Append(lastlistn,nl->First(rest));
      firstr = nl->First(rest);
      rest = nl->Rest(rest);
      first2 = nl->First(firstr);
      second2 = nl->Second(firstr);
      nl->WriteToString(attrname, first2);
      nl->WriteToString(argstr2, second2);

      // compare with the attributes in the relation
      if (attrname == inputname)
        inputtype = argstr2;

      // save from the detected attribute the type
    }
  rest = second;
  listfull = listn;
  nl->WriteToString(fulllist, listfull);


  CHECK_COND(!(inputtype == "") ,
             "Operator makemvalue: Attribute name '"+ inputname+
             "' is not known.\n"+
             "Known Attribute(s): " + fulllist);

  CHECK_COND( (inputtype == "ubool"
               || inputtype == "uint"
               || inputtype == "ureal"
               || inputtype == "upoint"
               || inputtype == "ustring"
               || inputtype == "uregion"),
              "Attribute type is not of type ubool, uint, ustring, ureal"
              "upoint or uregion.");

  attrname = nl->SymbolValue(second);
  j = FindAttribute(nl->Second(nl->Second(first)), attrname, attrtype);

  assert( j !=0 );

  if( inputtype == "upoint" )
    attrtype = nl->SymbolAtom( "mpoint" ) ;
  if( inputtype == "ubool" )
    attrtype = nl->SymbolAtom( "mbool" );
  if( inputtype == "ureal" )
    attrtype = nl->SymbolAtom( "mreal" );
  if( inputtype == "uint" )
    attrtype = nl->SymbolAtom( "mint" );
  if( inputtype == "ustring" )
    attrtype = nl->SymbolAtom( "mstring" );
  if( inputtype == "uregion" )
    attrtype = nl->SymbolAtom( "movingregion" );

  return nl->ThreeElemList(nl->SymbolAtom("APPEND"),
           nl->TwoElemList(nl->IntAtom(j),
           nl->StringAtom(nl->SymbolValue(attrtype))), attrtype);

  // Appending the number of the attribute in the relation is very important,
  // because in the other case we can't work with it in the value function.
}

/*
5.5.2 Value Mapping for ~makemvalue~

*/

template <class Mapping, class Unit>
int MappingMakemvalue(Word* args,Word& result,int message,
                      Word& local,Supplier s)
{
  Mapping* m;
  Unit* unit;
  Word currentTupleWord;

  assert(args[2].addr != 0); // assert existence of input
  assert(args[3].addr != 0); // assert existence of input

  int attributeIndex = ((CcInt*)args[2].addr)->GetIntval() - 1;

  qp->Open(args[0].addr);
  qp->Request(args[0].addr, currentTupleWord);

  result = qp->ResultStorage(s);

  m = (Mapping*) result.addr;
  m->Clear();
  m->StartBulkLoad();

  while ( qp->Received(args[0].addr) ) // get all tuples
    {
      Tuple* currentTuple = (Tuple*)currentTupleWord.addr;
      Attribute* currentAttr = (Attribute*)currentTuple->
        GetAttribute(attributeIndex);

      if(currentAttr->IsDefined())
        {
          unit = (Unit*) currentAttr;
          m->Add( *unit );
          currentTuple->DeleteIfAllowed();
        }
      qp->Request(args[0].addr, currentTupleWord);
    }
  m->EndBulkLoad( true ); // force Mapping to sort the units
  qp->Close(args[0].addr);

  return 0;
}

// here comes the version for movingregion, where URegion has a rather
// ugly implementation and thus needs a specialized treatment!
int MappingMakemvalue_mregion(Word* args,Word& result,int message,
                              Word& local,Supplier s)
{
  MRegion* m;
  URegion* unit;
  Word currentTupleWord;

  assert(args[2].addr != 0); // assert existence of input
  assert(args[3].addr != 0); // assert existence of input

  int attributeIndex = ((CcInt*)args[2].addr)->GetIntval() - 1;

  qp->Open(args[0].addr);
  qp->Request(args[0].addr, currentTupleWord);

  result = qp->ResultStorage(s);

  m = (MRegion*) result.addr;
  m->Clear();
  m->StartBulkLoad();

  while ( qp->Received(args[0].addr) ) // get all tuples
    {
      Tuple* currentTuple = (Tuple*)currentTupleWord.addr;
      Attribute* currentAttr = (Attribute*)currentTuple->
        GetAttribute(attributeIndex);

      if(currentAttr->IsDefined())
        {
          unit = (URegion*) currentAttr;
          cout << "MappingMakemvalue_mregion: " << endl;
          unit->Print(cout);
          m->AddURegion( *unit );
          currentTuple->DeleteIfAllowed();
        }
      qp->Request(args[0].addr, currentTupleWord);
    }
  m->EndBulkLoad( true ); // force Mapping to sort the units
  qp->Close(args[0].addr);

  return 0;
}

/*
5.5.3 Specification for operator ~makemvalue~

*/
const string
TemporalSpecMakemvalue  =
"( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
"( <text>For T in {bool, int, string, real, point, region}:"
"((stream (tuple ((x1 t1)...(xn tn)))"
" (uT)))-> mT</text--->"
"<text>_ makemvalue[ _ ]</text--->"
"<text>Create a moving object from a (not necessarily sorted) "
"tuple stream containing units. "
"No two unit timeintervals may overlap. Undefined units are "
"allowed and will be ignored. A stream with less than 1 defined "
"unit will result in an 'empty' moving object, not in an 'undef'.</text--->"
"<text>query units(zug5) transformstream makemvalue[elem]</text---> ) )";

/*
5.5.4 Selection Function of operator ~makemvalue~

*/
int
MakemvalueSelect( ListExpr args )
{

 ListExpr first, second, rest, listn,
           lastlistn, first2, second2, firstr;


  string argstr, argstr2, attrname, inputtype, inputname;


  first = nl->First(args);
  second  = nl->Second(args);
  nl->WriteToString(argstr, first);


  nl->WriteToString(inputname, second);


  rest = nl->Second(nl->Second(first));
  listn = nl->OneElemList(nl->First(rest));
  lastlistn = listn;
  firstr = nl->First(rest);
  rest = nl->Rest(rest);
  first2 = nl->First(firstr);
  second2 = nl->Second(firstr);
  nl->WriteToString(attrname, first2);
  nl->WriteToString(argstr2, second2);
  if (attrname == inputname)
     inputtype = argstr2;

while (!(nl->IsEmpty(rest)))
  {
     lastlistn = nl->Append(lastlistn,nl->First(rest));
     firstr = nl->First(rest);
     rest = nl->Rest(rest);
     first2 = nl->First(firstr);
     second2 = nl->Second(firstr);
     nl->WriteToString(attrname, first2);
     nl->WriteToString(argstr2, second2);
     if (attrname == inputname)
         inputtype = argstr2;
  }

  if( inputtype == "ubool" )
    return 0;

  if( inputtype == "uint" )
    return 1;

  if( inputtype == "ureal" )
    return 2;

  if( inputtype == "upoint" )
    return 3;

  if( inputtype == "ustring" )
    return 4;

  if( inputtype == "uregion" )
    return 5;


  return -1; // This point should never be reached
}

ValueMapping temporalmakemvaluemap[] = {
      MappingMakemvalue<MBool, UBool>,
      MappingMakemvalue<MBool, UBool>,
      MappingMakemvalue<MReal, UReal>,
      MappingMakemvalue<MPoint, UPoint>,
      MappingMakemvalue<MString, UString>,
      MappingMakemvalue_mregion };

/*
5.5.5  Definition of operator ~makemvalue~

*/
Operator temporalunitmakemvalue( "makemvalue",
                        TemporalSpecMakemvalue,
                        6,
                        temporalmakemvaluemap,
                        MakemvalueSelect,
                        MovingTypeMapMakemvalue );

/*
5.6 Operator ~trajectory~

5.6.1 Type Mapping for ~trajectory~

*/
ListExpr
UnitTrajectoryTypeMap( ListExpr args )
{
  if ( nl->ListLength( args ) == 1 )
  {
    ListExpr arg1 = nl->First( args );

    if( nl->IsEqual( arg1, "upoint" ) )
      return nl->SymbolAtom( "line" );
  }
  return nl->SymbolAtom( "typeerror" );
}

/*
5.6.2 Value Mapping for ~trajectory~

*/
int UnitPointTrajectory(Word* args, Word& result, int message,
                        Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Line   *line   = ((Line*)result.addr);
  UPoint *upoint = ((UPoint*)args[0].addr);

  line->Clear();                // clear result
  if ( upoint->IsDefined() )
    upoint->UTrajectory( *line );   // call memberfunction
  else
    {
      // line->Clear();             // Use empty value
      // line->SetDefined( false ); // instead of undef
    }
  return 0;
}

/*
5.6.3 Specification for operator ~trajectory~

*/
const string
TemporalSpecTrajectory  =
"( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
"( <text>upoint -> line</text--->"
"<text>trajectory( _ )</text--->"
"<text>get the trajectory of the corresponding"
"unit point object. Static or undef upoint objects "
"yield empty line objects.</text--->"
"<text>trajectory( up1 )</text---> ) )";

/*
5.6.4 Selection Function of operator ~trajectory~

Not necessary.

*/

/*
5.6.5  Definition of operator ~trajectory~

*/
Operator temporalunittrajectory( "trajectory",
                          TemporalSpecTrajectory,
                          UnitPointTrajectory,
                          Operator::SimpleSelect,
                          UnitTrajectoryTypeMap );

/*
5.7 Operator ~deftime~

5.7.1 Type Mapping for ~deftime~

*/
ListExpr
UnitTypeMapPeriods( ListExpr args )
{
  if ( nl->ListLength( args ) == 1 )
  {
    ListExpr arg1 = nl->First( args );

   if(  nl->IsEqual( arg1, "ubool" )  ||
        nl->IsEqual( arg1, "uint" )   ||
        nl->IsEqual( arg1, "ureal" )  ||
        nl->IsEqual( arg1, "upoint" ) ||
        nl->IsEqual( arg1, "ustring" ) ||
        nl->IsEqual( arg1, "uregion" ) )
   return nl->SymbolAtom( "periods" );
  }
  return nl->SymbolAtom( "typeerror" );
}

/*
5.7.2 Value Mapping for ~deftime~

*/
template <class Mapping>
int MappingUnitDefTime( Word* args, Word& result, int message,
                        Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Periods* r = ((Periods*)result.addr);
  Mapping* m = ((Mapping*)args[0].addr);

  if ( !m->IsDefined() )
    r->SetDefined( false );
  else
    {
      assert( r->IsOrdered() );

      r->Clear();
      r->StartBulkLoad();
      r->Add( m->timeInterval );
      r->EndBulkLoad( false );
    }
  return 0;
}

/*
5.7.3 Specification for operator ~deftime~

*/
const string
TemporalSpecDefTime  =
"( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
"( <text>uT -> periods \n"
"(for T in {bool, int, real, string, point, region})</text--->"
"<text>deftime( _ )</text--->"
"<text>get the definition time for the "
" unit data objects.</text--->"
"<text>deftime( up1 )</text---> ) )";

/*
5.7.4 Selection Function of operator ~deftime~

Uses ~UnitSimpleSelect~.

*/

ValueMapping temporalunitdeftimemap[] = { MappingUnitDefTime<UBool>,
                                          MappingUnitDefTime<UInt>,
                                          MappingUnitDefTime<UReal>,
                                          MappingUnitDefTime<UPoint>,
                                          MappingUnitDefTime<UString>,
                                          MappingUnitDefTime<URegion>};


/*
5.7.5  Definition of operator ~deftime~

*/

Operator temporalunitdeftime( "deftime",
                          TemporalSpecDefTime,
                          6,
                          temporalunitdeftimemap,
                          UnitSimpleSelect,
                          UnitTypeMapPeriods );
/*
5.8 Operator ~atinstant~

5.8.1 Type Mapping for ~atinstant~

*/
ListExpr
UnitInstantTypeMapIntime( ListExpr args )
{
  if ( nl->ListLength( args ) == 2 )
  {
    ListExpr arg1 = nl->First( args ),
             arg2 = nl->Second( args );

    if( nl->IsEqual( arg2, "instant" ) )
    {
       if( nl->IsEqual( arg1, "ubool" ) )
        return nl->SymbolAtom( "ibool" );

      if( nl->IsEqual( arg1, "uint" ) )
        return nl->SymbolAtom( "iint" );

      if( nl->IsEqual( arg1, "ureal" ) )
        return nl->SymbolAtom( "ireal" );

      if( nl->IsEqual( arg1, "upoint" ) )
        return nl->SymbolAtom( "ipoint" );

      if( nl->IsEqual( arg1, "ustring" ) )
        return nl->SymbolAtom( "istring" );

      if( nl->IsEqual( arg1, "uregion" ) )
        return nl->SymbolAtom( "intimeregion" );
    }
  }
  return nl->SymbolAtom( "typeerror" );
}

/*
5.8.2 Value Mapping for ~atinstant~

*/
template <class Mapping, class Alpha>
int MappingUnitAtInstant( Word* args, Word& result, int message,
                          Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Intime<Alpha>* pResult = (Intime<Alpha>*)result.addr;
  Mapping* posUnit = ((Mapping*)args[0].addr);
  Instant* t = ((Instant*)args[1].addr);
  Instant t1 = *t;

  if ( !t->IsDefined() || !posUnit->IsDefined() )
    pResult->SetDefined( false );
  else if( posUnit->timeInterval.Contains(t1) )
    {
      posUnit->TemporalFunction( *t, pResult->value );
      pResult->instant = *t;
      pResult->SetDefined( true );
    }
  else    // instant not contained by deftime interval
    pResult->SetDefined( false );

  return 0;
}

/*
5.8.3 Specification for operator ~atinstant~

*/
const string
TemporalSpecAtInstant  =
"( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
"( <text>(uT instant) -> iT\n"
"(T in {bool, int, real, string, point, region})</text--->"
"<text>_ atinstant _ </text--->"
"<text>From a unit type, get the Intime value corresponding to "
"the given instant.</text--->"
"<text>upoint1 atinstant instant1</text---> ) )";

/*
5.8.4 Selection Function of operator ~atinstant~

Uses ~UnitSimpleSelect~.

*/



ValueMapping temporalunitatinstantmap[] =
  {MappingUnitAtInstant<UBool, CcBool>,
   MappingUnitAtInstant<UInt, CcInt>,
   MappingUnitAtInstant<UReal, CcReal>,
   MappingUnitAtInstant<UPoint, Point>,
   MappingUnitAtInstant<UString, CcString>,
   MappingUnitAtInstant<URegion, Region>};


/*
5.8.5  Definition of operator ~atinstant~

*/
Operator temporalunitatinstant( "atinstant",
                            TemporalSpecAtInstant,
                            6,
                            temporalunitatinstantmap,
                            UnitSimpleSelect,
                            UnitInstantTypeMapIntime );

/*
5.9 Operator ~atperiods~

5.9.1 Type Mapping for ~atperiods~

*/
ListExpr
UnitPeriodsTypeMap( ListExpr args )
{
  ListExpr arg1, arg2;
  string argstr;

  if ( nl->ListLength( args ) != 2 )
    {
      ErrorReporter::ReportError("Operator atperiods expects "
                                 "a list of length 2.");
      return nl->SymbolAtom( "typeerror" );
    }

  arg1 = nl->First( args );
  arg2 = nl->Second( args );

  nl->WriteToString(argstr, arg2);
  if ( !( nl->IsEqual( arg2, "periods" ) ) )
    {
      ErrorReporter::ReportError("Operator atperiods expects a second argument"
                             " of type 'periods' but gets '" + argstr + "'.");
      return nl->SymbolAtom( "typeerror" );
    }

  if( nl->IsAtom( arg1 ) )
    {
      if( nl->IsEqual( arg1, "ubool" ) )
        return nl->TwoElemList(nl->SymbolAtom("stream"),
               nl->SymbolAtom("ubool"));
      if( nl->IsEqual( arg1, "uint" ) )
        return nl->TwoElemList(nl->SymbolAtom("stream"),
               nl->SymbolAtom("uint"));
      if( nl->IsEqual( arg1, "ureal" ) )
        return nl->TwoElemList(nl->SymbolAtom("stream"),
               nl->SymbolAtom("ureal"));
      if( nl->IsEqual( arg1, "upoint" ) )
        return nl->TwoElemList(nl->SymbolAtom("stream"),
                               nl->SymbolAtom("upoint"));
      if( nl->IsEqual( arg1, "ustring" ) )
        return nl->TwoElemList(nl->SymbolAtom("stream"),
               nl->SymbolAtom("ustring"));
      if( nl->IsEqual( arg1, "uregion" ) )
        return nl->TwoElemList(nl->SymbolAtom("stream"),
               nl->SymbolAtom("uregion"));

      nl->WriteToString(argstr, arg1);
      ErrorReporter::ReportError("Operator atperiods expects a first argument "
                                 "of type T in {ubool, uint, ureal, upoint, "
                                 "ustring, uregion} but gets a '"
                                 + argstr + "'.");
      return nl->SymbolAtom( "typeerror" );
    }

  if( !( nl->IsAtom( arg1 ) ) && ( nl->ListLength( arg1 ) == 2) )
    {
      nl->WriteToString(argstr, arg1);
      if ( !( TypeOfRelAlgSymbol(nl->First(arg1)) == stream ) )
        {
          ErrorReporter::ReportError("Operator atperiods expects as first "
                                     "argument a list with structure 'T' or "
                                     "'stream(T)', T in {ubool, uint, ureal, "
                                     "upoint, ustring, ureagion} but gets a "
                                     "list with structure '" + argstr + "'.");
          return nl->SymbolAtom( "typeerror" );
        }

      if( nl->IsEqual( nl->Second(arg1), "ubool" ) )
        return nl->TwoElemList(nl->SymbolAtom("stream"),
                               nl->SymbolAtom("ubool"));
      if( nl->IsEqual( nl->Second(arg1), "uint" ) )
        return nl->TwoElemList(nl->SymbolAtom("stream"),
                               nl->SymbolAtom("uint"));
      if( nl->IsEqual( nl->Second(arg1), "ureal" ) )
         return nl->TwoElemList(nl->SymbolAtom("stream"),
                                nl->SymbolAtom("ureal"));
      if( nl->IsEqual( nl->Second(arg1), "upoint" ) )
         return nl->TwoElemList(nl->SymbolAtom("stream"),
                                 nl->SymbolAtom("upoint"));
      if( nl->IsEqual( nl->Second(arg1), "ustring" ) )
         return nl->TwoElemList(nl->SymbolAtom("stream"),
                                 nl->SymbolAtom("ustring"));
      if( nl->IsEqual( nl->Second(arg1), "uregion" ) )
         return nl->TwoElemList(nl->SymbolAtom("stream"),
                                 nl->SymbolAtom("uregion"));

      nl->WriteToString(argstr, nl->Second(arg1));
      ErrorReporter::ReportError("Operator atperiods expects a type "
                              "(stream T); T in {ubool, uint, ureal, upoint, "
                              "ustring, uregion} but gets '(stream "
                              + argstr + ")'.");
      return nl->SymbolAtom( "typeerror" );
    };

  nl->WriteToString( argstr, args );
  ErrorReporter::ReportError("Operator atperiods encountered an "
                             "unmatched typerror for arguments '"
                             + argstr + "'.");
  return nl->SymbolAtom( "typeerror" );
}

/*
5.9.2 Value Mapping for ~atperiods~

*/
struct AtPeriodsLocalInfo
{
  Word uWord;     // the address of the unit value
  Word pWord;    //  the adress of the periods value
  int  j;       //   save the number of the interval
};

/*
Variant 1: first argument is a scalar value

*/

template <class Alpha>
int MappingUnitAtPeriods( Word* args, Word& result, int message,
                          Word& local, Supplier s )
{
  AtPeriodsLocalInfo *localinfo;
  const Interval<Instant> *interval;

  Alpha* unit;
  Alpha r;
  Periods* periods;


  switch( message )
  {
  case OPEN:

    localinfo = new AtPeriodsLocalInfo;
    localinfo->uWord = args[0];
    localinfo->pWord = args[1];
    localinfo->j = 0;
    local = SetWord(localinfo);
    return 0;

  case REQUEST:

    if (TUA_DEBUG) cout << "\nMappingUnitAtPeriods: REQUEST" << endl;
    if( local.addr == 0 )
      return CANCEL;
    localinfo = (AtPeriodsLocalInfo *)local.addr;
    unit = (Alpha*)localinfo->uWord.addr;
    periods = (Periods*)localinfo->pWord.addr;

    if( !unit->IsDefined()    ||
        !periods->IsDefined() ||   // as a set-valued type, periods cannot be
        periods->IsEmpty()       ) // undefined, but only empty
      return CANCEL;
    if (TUA_DEBUG)
      cout << "   Unit's timeInterval u="
           << TUPrintTimeInterval( unit->timeInterval ) << endl;
    if( localinfo->j == periods->GetNoComponents() )
      {
        if (TUA_DEBUG)
          cout << "MappingUnitAtPeriods: REQUEST finished: CANCEL (1)"
               << endl;
        return CANCEL;
      }
    periods->Get( localinfo->j, interval );
    if (TUA_DEBUG) cout << "   Probing timeInterval p ="
                        << TUPrintTimeInterval(*interval)
                        << endl;
    while( interval->Before( unit->timeInterval ) &&
           localinfo->j < periods->GetNoComponents() )
      {
        localinfo->j++,
        periods->Get(localinfo->j, interval);
        if (TUA_DEBUG)
          {
             cout << "   Probing timeInterval="
                  << TUPrintTimeInterval(*interval)
                  << endl;
             if (interval->Before( unit->timeInterval ))
               cout << "     p is before u" << endl;
             if (localinfo->j < periods->GetNoComponents())
               cout << "   j < #Intervals" << endl;
          }
      }

    if( localinfo->j >= periods->GetNoComponents() ) {
      result.addr = 0;
      if (TUA_DEBUG)
        cout << "MappingUnitAtPeriods: REQUEST finished: CANCEL (2)"
             << endl;
      return CANCEL;
    }

    if( unit->timeInterval.Before( *interval ) )
      {
        result.addr = 0;
        if (TUA_DEBUG)
          cout << "MappingUnitAtPeriods: REQUEST finished: CANCEL (3)"
               << endl;
        return CANCEL;
      }
    else
      {
        // create unit restricted to interval
        unit->AtInterval( *interval, r );
        Alpha* aux = new Alpha( r );
        result = SetWord( aux );
        localinfo->j++;
        if (TUA_DEBUG)
          {
            cout << "   Result interval="
                 << TUPrintTimeInterval(aux->timeInterval)
                 << endl;
            cout << "   Result defined=" << aux->IsDefined()
                 << endl;
            cout << "MappingUnitAtPeriods: REQUEST finished: YIELD"
                 << endl;
          }
        return YIELD;
      }

    if (TUA_DEBUG)
      cout << "MappingUnitAtPeriods: REQUEST finished: CANCEL (4)"
           << endl;
    return CANCEL; // should not happen

  case CLOSE:

    if( local.addr != 0 )
      delete (AtPeriodsLocalInfo *)local.addr;
    return 0;
  }
  // should not happen:
  return -1;
}

/*
Variant 2: first argument is a stream
Implemented on 07/17/2006 by Christian D[ue]ntgen

*/

struct AtPeriodsLocalInfoUS
{
  Word uWord;  // address of the input stream
  Word pWord;  // address of the input periods value
  int j;       // interval counter for within periods
};


template <class Alpha>
int MappingUnitStreamAtPeriods( Word* args, Word& result, int message,
                                Word& local, Supplier s )
{
  AtPeriodsLocalInfoUS *localinfo;
  Alpha *unit, *aux;
  Alpha resultUnit;
  Periods *periods;
  const Interval<Instant> *interval;
  bool foundUnit = false;

  switch( message )
  {
  case OPEN:
    localinfo = new AtPeriodsLocalInfoUS;
    localinfo->pWord = args[1];
    localinfo->j = 0;                              // init interval counter
    qp->Open( args[0].addr );                      // open stream of units
    qp->Request( args[0].addr, localinfo->uWord ); // request first unit
    if ( !( qp->Received( args[0].addr) ) )
      {result.addr = 0; return CANCEL; }
    local = SetWord(localinfo);                    // pass up link to localinfo
    return 0;

  case REQUEST:
    if ( local.addr == 0 )
      return CANCEL;
    localinfo = (AtPeriodsLocalInfoUS *) local.addr; // restore local data
    if ( localinfo->uWord.addr == 0 )
      { result.addr = 0; return CANCEL; }
    unit = (Alpha *) localinfo->uWord.addr;
    if ( localinfo->pWord.addr == 0 )
      { result.addr = 0; return CANCEL; }
    periods = (Periods *) localinfo->pWord.addr;

    if( !periods->IsDefined() ||   // by now, periods cannot be undefined
        periods->IsEmpty()       ) // only empty
      return CANCEL;

    // search for a pair of overlapping unit/interval:
    while (1)
      {
        if ( localinfo->j == periods->GetNoComponents() ) // redo first interval
          { localinfo->j = 0;
            unit->DeleteIfAllowed();                // delete original unit?
            foundUnit = false;
            while(!foundUnit)
              {
                qp->Request(args[0].addr, localinfo->uWord);  // get new unit
                if( qp->Received( args[0].addr ) )
                  unit = (Alpha *) localinfo->uWord.addr;
                else
                  { result.addr = 0; return CANCEL; }   // end of unit stream
                foundUnit = unit->IsDefined();
              }
          }
        periods->Get(localinfo->j, interval);       // get an interval
        if (    !( interval->Before( unit->timeInterval ) )
                && !( unit->timeInterval.Before( *interval) ) )
          break;                           // found candidate, break while
        localinfo->j++;                             // next interval, loop
      }

    // We have an interval possibly overlapping the unit's interval now
    // Return unit restricted to overlapping part of both intervals
    unit->AtInterval( *interval, resultUnit); // intersect unit and interval
    aux = new Alpha( resultUnit );
    result = SetWord( aux );
    localinfo->j++;                           // increase interval counter
    return YIELD;

  case CLOSE:
    if ( local.addr != 0 )
      {
        localinfo = (AtPeriodsLocalInfoUS *) local.addr;
        if ( localinfo->uWord.addr != 0 )
          {
            unit = (Alpha *) localinfo->uWord.addr;
            unit->DeleteIfAllowed();   // delete remaining original unit
          }
        delete (AtPeriodsLocalInfoUS *)localinfo;
      }
    return 0;

  } // end switch

  return -1; // should never be reached

} // end MappingUnitStreamAtPeriods

/*
5.9.3 Specification for operator ~atperiods~

*/
const string
TemporalSpecAtPeriods  =
"( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\") "
"( <text>For T in {int, bool, real, string, point, region}:\n"
"(uT periods) -> stream uT\n"
"((stream uT) periods) -> stream uT</text--->"
"<text>_ atperiods _ </text--->"
"<text>restrict the movement to the given"
" periods.</text--->"
"<text>upoint1 atperiods thehour(2003,11,11,8)\n"
"sfeed(upoint1) atperiods thehour(2003,11,11,8)</text--->) )";

/*
5.9.4 Selection Function of operator ~atperiods~

Uses ~UnitCombinedUnitStreamSelect~.

*/

ValueMapping temporalunitatperiodsmap[] =
  { MappingUnitAtPeriods<UBool>,
    MappingUnitAtPeriods<UInt>,
    MappingUnitAtPeriods<UReal>,
    MappingUnitAtPeriods<UPoint>,
    MappingUnitAtPeriods<UString>,
    MappingUnitAtPeriods<URegion>,
    MappingUnitStreamAtPeriods<UBool>,
    MappingUnitStreamAtPeriods<UInt>,
    MappingUnitStreamAtPeriods<UReal>,
    MappingUnitStreamAtPeriods<UPoint>,
    MappingUnitStreamAtPeriods<UString>,
    MappingUnitStreamAtPeriods<URegion>
  };

/*
5.9.5  Definition of operator ~atperiods~

*/
Operator temporalunitatperiods( "atperiods",
                            TemporalSpecAtPeriods,
                            12,
                            temporalunitatperiodsmap,
                            UnitCombinedUnitStreamSelect,
                            UnitPeriodsTypeMap );

/*
5.10 Operators ~initial~ and ~final~

5.10.1 Type Mapping for ~initial~ and ~final~

Signatures

----
    For T in {bool, int, real, string, point, region}:

      (       uT) --> iT
      (stream uT) --> iT

----


*/
ListExpr
UnitTypeMapIntime( ListExpr args )
{
  ListExpr t;

  if ( nl->ListLength( args ) == 1 )
    {
      if (nl->IsAtom(nl->First(args)))
        t = nl->First( args );
      else if (nl->ListLength(nl->First(args))==2 &&
               nl->IsEqual(nl->First(nl->First(args)), "stream"))
        t = nl->Second(nl->First(args));
      else
        {
          ErrorReporter::ReportError
            ("Operator initial/final expects a (stream T)"
             "for T in {bool,int,real,string,point,region}");
          return nl->SymbolAtom( "typeerror" );
        }

      if( nl->IsEqual( t, "ubool" ) )
        return nl->SymbolAtom( "ibool" );

      if( nl->IsEqual( t, "uint" ) )
        return nl->SymbolAtom( "iint" );

      if( nl->IsEqual( t, "ureal" ) )
        return nl->SymbolAtom( "ireal" );

      if( nl->IsEqual( t, "upoint" ) )
        return nl->SymbolAtom( "ipoint" );

      if( nl->IsEqual( t, "ustring" ) )
        return nl->SymbolAtom( "istring" );

      if( nl->IsEqual( t, "uregion" ) )
        return nl->SymbolAtom( "intimeregion" );
    }
  else
    ErrorReporter::ReportError
      ("Operator initial/final expects a list of length one, "
       "containing a value of one type 'T' with T in "
       "{bool,int,real,string,point,region}");
  return nl->SymbolAtom( "typeerror" );
}

/*
5.10.2 Value Mapping for ~initial~ and ~final~

*/

// first come the value mappings for (UNIT) argument
template <class Unit, class Alpha>
int MappingUnitInitial( Word* args, Word& result, int message,
                        Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Unit* unit = ((Unit*)args[0].addr);
  Intime<Alpha>* res = ((Intime<Alpha>*)result.addr);

  if( !unit->IsDefined() || !(unit->timeInterval.start.IsDefined()) )
     res->SetDefined( false );
  else
   {
     unit->TemporalFunction( unit->timeInterval.start, res->value, true );
     res->instant.CopyFrom( &unit->timeInterval.start );
     res->SetDefined( true );
   }
  return 0;
}

template <class Unit, class Alpha>
int MappingUnitFinal( Word* args, Word& result, int message,
                      Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Unit *unit = ((Unit*)args[0].addr);
  Intime<Alpha> *res = ((Intime<Alpha>*)result.addr);

  if( !unit->IsDefined() || !(unit->timeInterval.end.IsDefined()) )
     res->SetDefined( false );
  else
   {
     unit->TemporalFunction( unit->timeInterval.end, res->value, true );
     res->instant.CopyFrom( &unit->timeInterval.end );
     res->SetDefined( true );
   }
  return 0;
}

//now, we give the value mappings for (stream UNIT) argument
// Mode=0 for initial, Mode=1 for final
template <class Unit, class Alpha, int Mode>
int MappingUnitStreamInstantFinal( Word* args, Word& result, int message,
                                   Word& local, Supplier s )
{
  assert(Mode>=0 && Mode<=1);

  result = qp->ResultStorage( s );

  Word elem;
  Unit *U = 0, *SavedUnit = 0;
  Intime<Alpha> *I = ((Intime<Alpha>*)result.addr);


  qp->Open(args[0].addr);              // get first elem from stream
  qp->Request(args[0].addr, elem);     // get first elem from stream
  while ( qp->Received(args[0].addr) ) // there is a element from the stream
    {
      U = (Unit*) elem.addr;
      if ( U->IsDefined() )
        {
          if (SavedUnit == 0)
            SavedUnit = U;
          else
            {
              if(Mode == 0)
                { // initial-mode
                  if ( U->timeInterval.Before((*SavedUnit).timeInterval.start) )
                    {
                      SavedUnit->DeleteIfAllowed();
                      SavedUnit = U;
                    }
                  else
                    U->DeleteIfAllowed();
                }
              else // (Mode == 1)
                { // final-mode
                  if ( U->timeInterval.After((*SavedUnit).timeInterval.end) )
                    {
                      SavedUnit->DeleteIfAllowed();
                      SavedUnit = U;
                    }
                  else
                    {
                      U->DeleteIfAllowed();
                    }
                }
            }
        }
      else
        U->DeleteIfAllowed();
      qp->Request(args[0].addr, elem); // get next stream elem
    }
  qp->Close(args[0].addr); // close the stream

  // create and return the result
  if (SavedUnit == 0)
    I->SetDefined(false);
  else
    if(Mode == 0)
      { // initial-mode
        SavedUnit->TemporalFunction
          ( SavedUnit->timeInterval.start, I->value, true );
        I->instant.CopyFrom( &SavedUnit->timeInterval.start );
        I->SetDefined( true );
        SavedUnit->DeleteIfAllowed();
      }
    else // (Mode == 1)
      { // final-mode
        SavedUnit->TemporalFunction
          ( SavedUnit->timeInterval.end, I->value, true );
        I->instant.CopyFrom( &SavedUnit->timeInterval.end );
        I->SetDefined( true );
        SavedUnit->DeleteIfAllowed();
      }
  return 0;
}


/*
5.10.3 Specification for operator ~initial~ and ~final~

*/
const string
TemporalSpecInitial  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(       uT) -> iT\n"
  "(stream uT) -> iT\n"
  "(T in {bool, int, real, string, point, region})</text--->"
  "<text>initial( _ )</text--->"
  "<text>From a unit type (or a stream of units), get the "
  "intime value corresponding to the (overall) initial instant.</text--->"
  "<text>initial( upoint1 )</text---> ) )";

const string
TemporalSpecFinal  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>)       uT) -> iT\n"
  "(stream uT) -> iT\n"
  "(T in {bool, int, real, string, point, region})</text--->"
  "<text>final( _ )</text--->"
  "<text>get the intime value corresponding "
  "to the (overall) final instant of the (stream of) unit.</text--->"
  "<text>final( upoint1 )</text---> ) )";

/*
5.10.4 Selection Function of operator ~initial~ and ~final~

Using ~UnitCombinedUnitStreamSelect~.

*/

ValueMapping temporalunitinitialmap[] = {
  MappingUnitInitial<UBool, CcBool>,
  MappingUnitInitial<UInt, CcInt>,
  MappingUnitInitial<UReal, CcReal>,
  MappingUnitInitial<UPoint, Point>,
  MappingUnitInitial<UString, CcString>,
  MappingUnitInitial<URegion, Region>,
  MappingUnitStreamInstantFinal<UBool, CcBool, 0>,
  MappingUnitStreamInstantFinal<UInt, CcInt, 0>,
  MappingUnitStreamInstantFinal<UReal, CcReal, 0>,
  MappingUnitStreamInstantFinal<UPoint, Point, 0>,
  MappingUnitStreamInstantFinal<UString, CcString, 0>,
  MappingUnitStreamInstantFinal<URegion, Region, 0>
};

ValueMapping temporalunitfinalmap[] = {
  MappingUnitFinal<UBool, CcBool>,
  MappingUnitFinal<UInt, CcInt>,
  MappingUnitFinal<UReal, CcReal>,
  MappingUnitFinal<UPoint, Point>,
  MappingUnitFinal<UString, CcString>,
  MappingUnitFinal<URegion, Region>,
  MappingUnitStreamInstantFinal<UBool, CcBool, 1>,
  MappingUnitStreamInstantFinal<UInt, CcInt, 1>,
  MappingUnitStreamInstantFinal<UReal, CcReal, 1>,
  MappingUnitStreamInstantFinal<UPoint, Point, 1>,
  MappingUnitStreamInstantFinal<UString, CcString, 1>,
  MappingUnitStreamInstantFinal<URegion, Region, 1>
};

/*
5.10.5  Definition of operator ~initial~ and ~final~

*/
Operator temporalunitinitial
(
 "initial",
 TemporalSpecInitial,
 12,
 temporalunitinitialmap,
 UnitCombinedUnitStreamSelect,
 UnitTypeMapIntime );

Operator temporalunitfinal
(
 "final",
 TemporalSpecFinal,
 12,
 temporalunitfinalmap,
 UnitCombinedUnitStreamSelect,
 UnitTypeMapIntime );

/*
5.11 Operator ~present~

5.11.1 Type Mapping for ~present~

*/
ListExpr
UnitInstantPeriodsTypeMapBool( ListExpr args )
{
   if ( nl->ListLength( args ) == 2 )
  {
    ListExpr arg1 = nl->First( args ),
             arg2 = nl->Second( args );

    if( nl->IsEqual( arg2, "instant" ) ||
        nl->IsEqual( arg2, "periods" ) )
    {
      if( nl->IsEqual( arg1, "ubool" )  ||
          nl->IsEqual( arg1, "uint" )   ||
          nl->IsEqual( arg1, "ureal" )  ||
          nl->IsEqual( arg1, "upoint")  ||
          nl->IsEqual( arg1, "ustring") ||
          nl->IsEqual( arg1, "uregion")    )

        return nl->SymbolAtom( "bool" );
    }
  }
  return nl->SymbolAtom( "typeerror" );
}

/*
5.11.2 Value Mapping for ~present~

*/

template <class Mapping>
int MappingUnitPresent_i( Word* args, Word& result, int message,
                          Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Mapping *m = ((Mapping*)args[0].addr);
  Instant* inst = ((Instant*)args[1].addr);
  Instant t1 = *inst;

  if ( !inst->IsDefined() || !m->IsDefined() )
    ((CcBool *)result.addr)->Set( false, false );

  else if( m->timeInterval.Contains(t1) )
    ((CcBool *)result.addr)->Set( true, true );
  else
    ((CcBool *)result.addr)->Set( true, false );
  return 0;
}

template <class Mapping>
int MappingUnitPresent_p( Word* args, Word& result, int message,
                          Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Mapping *m = ((Mapping*)args[0].addr);
  Periods* periods = ((Periods*)args[1].addr);

  assert( periods->IsOrdered() );

  // create a periods containing the smallest superinterval
  // of all intervals within the periods value.
  Periods deftime( 0 ), defTime( 0 );
  deftime.Clear();
  deftime.StartBulkLoad();
  deftime.Add( m->timeInterval );
  deftime.EndBulkLoad( false );
  deftime.Merge( defTime );

  if ( !m->IsDefined() )
    ((CcBool *)result.addr)->Set( false, false );
  else if( periods->IsEmpty() ) // (undef periods are not defined)
    ((CcBool *)result.addr)->Set( false, false );
  else if( periods->Inside( defTime ) )
    ((CcBool *)result.addr)->Set( true, true );
  else
    ((CcBool *)result.addr)->Set( true, false );

  return 0;
}


/*
5.11.3 Specification for operator ~present~

*/
const string
TemporalSpecPresent  =
"( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
"( <text>({m|u}T instant) -> bool\n"
"({m|u}T periods) -> bool\n"
"(T in {bool, int, real, string, point, region)</text--->"
"<text>_ present _ </text--->"
"<text>whether the moving/unit object is present at the"
" given instant or period. For an empty periods value, "
"the result is undefined.</text--->"
"<text>mpoint1 present instant1</text---> ) )";

/*
5.11.4 Selection Function of operator ~present~

*/

int
UnitInstantPeriodsSelect( ListExpr args )
{
  ListExpr arg1 = nl->First( args ),
           arg2 = nl->Second( args );

  // instant versions:

  if( nl->SymbolValue( arg1 ) == "ubool" &&
      nl->SymbolValue( arg2 ) == "instant" )
    return 0;

  if( nl->SymbolValue( arg1 ) == "uint" &&
     nl->SymbolValue( arg2 ) == "instant" )
    return 1;

  if( nl->SymbolValue( arg1 ) == "ureal" &&
      nl->SymbolValue( arg2 ) == "instant" )
    return 2;

  if( nl->SymbolValue( arg1 ) == "upoint" &&
      nl->SymbolValue( arg2 ) == "instant" )
    return 3;

  if( nl->SymbolValue( arg1 ) == "ustring" &&
      nl->SymbolValue( arg2 ) == "instant" )
    return 4;

  if( nl->SymbolValue( arg1 ) == "uregion" &&
      nl->SymbolValue( arg2 ) == "instant" )
    return 5;

  // periods versions:

  if( nl->SymbolValue( arg1 ) == "ubool" &&
      nl->SymbolValue( arg2 ) == "periods" )
    return 6;

  if( nl->SymbolValue( arg1 ) == "uint" &&
      nl->SymbolValue( arg2 ) == "periods" )
    return 7;

  if( nl->SymbolValue( arg1 ) == "ureal" &&
      nl->SymbolValue( arg2 ) == "periods" )
    return 8;

  if( nl->SymbolValue( arg1 ) == "upoint" &&
      nl->SymbolValue( arg2 ) == "periods" )
    return 9;

  if( nl->SymbolValue( arg1 ) == "ustring" &&
      nl->SymbolValue( arg2 ) == "periods" )
    return 10;

  if( nl->SymbolValue( arg1 ) == "uregion" &&
      nl->SymbolValue( arg2 ) == "periods" )
    return 11;


  return -1; // This point should never be reached
}

ValueMapping temporalunitpresentmap[] = { MappingUnitPresent_i<UBool>,
                                          MappingUnitPresent_i<UInt>,
                                          MappingUnitPresent_i<UReal>,
                                          MappingUnitPresent_i<UPoint>,
                                          MappingUnitPresent_i<UString>,
                                          MappingUnitPresent_i<URegion>,
                                          MappingUnitPresent_p<UBool>,
                                          MappingUnitPresent_p<UInt>,
                                          MappingUnitPresent_p<UReal>,
                                          MappingUnitPresent_p<UPoint>,
                                          MappingUnitPresent_p<UString>,
                                          MappingUnitPresent_p<URegion> };

/*
5.11.5  Definition of operator ~present~

*/
Operator temporalunitpresent( "present",
                          TemporalSpecPresent,
                          12,
                          temporalunitpresentmap,
                          UnitInstantPeriodsSelect,
                          UnitInstantPeriodsTypeMapBool);

/*
5.12 Operator ~passes~

5.12.1 Type Mapping for ~passes~

*/
ListExpr
UnitBaseTypeMapBool( ListExpr args )
{
  ListExpr arg1, arg2;
  if( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );

    if( ((nl->IsEqual( arg1, "upoint" ) && nl->IsEqual( arg2, "point" ))) )
      return nl->SymbolAtom( "bool" );
    if( ((nl->IsEqual( arg1, "uint" ) && nl->IsEqual( arg2, "int" ))) )
      return nl->SymbolAtom( "bool" );
    if( ((nl->IsEqual( arg1, "ubool" ) && nl->IsEqual( arg2, "bool" ))) )
      return nl->SymbolAtom( "bool" );
    if( ((nl->IsEqual( arg1, "ureal" ) && nl->IsEqual( arg2, "real" ))) )
      return nl->SymbolAtom( "bool" );
    if( ((nl->IsEqual( arg1, "ustring" ) && nl->IsEqual( arg2, "string" ))) )
      return nl->SymbolAtom( "bool" );
    if( ((nl->IsEqual( arg1, "uregion" ) && nl->IsEqual( arg2, "point" ))) )
      return nl->SymbolAtom( "bool" );
  }
  return nl->SymbolAtom( "typeerror" );
}

/*
5.12.2 Value Mapping for ~passes~

*/
template <class Mapping, class Alpha>
int MappingUnitPasses( Word* args, Word& result, int message,
                       Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Mapping *m = ((Mapping*)args[0].addr);
  Alpha* val = ((Alpha*)args[1].addr);

  if( !val->IsDefined() || !m->IsDefined() )
    ((CcBool *)result.addr)->Set( false, false );
  else if( m->Passes( *val ) )
    ((CcBool *)result.addr)->Set( true, true );
  else
    ((CcBool *)result.addr)->Set( true, false );

  return 0;
}

/*
5.12.3 Specification for operator ~passes~

*/
const string
TemporalSpecPasses =
"( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
"( <text>(uT T) -> bool\n"
"for T in {bool, int, real*, string, point, region*}\n"
"(*): Not yet implemented</text--->"
"<text>_ passes _ </text--->"
"<text>whether the object unit passes the given"
" value.</text--->"
"<text>upoint1 passes point1</text---> ) )";

/*
5.12.4 Selection Function of operator ~passes~

*/
int
TUPassesSelect( ListExpr args )
{
  ListExpr arg1 = nl->First( args ),
           arg2 = nl->Second( args );

  if( nl->SymbolValue( arg1 ) == "ubool" &&
      nl->SymbolValue( arg2 ) == "bool" )
    return 0;

  if( nl->SymbolValue( arg1 ) == "uint" &&
      nl->SymbolValue( arg2 ) == "int" )
    return 1;

  if( nl->SymbolValue( arg1 ) == "ureal" &&
      nl->SymbolValue( arg2 ) == "real" )
    return 2;

  if( nl->SymbolValue( arg1 ) == "upoint" &&
      nl->SymbolValue( arg2 ) == "point" )
    return 3;

  if( nl->SymbolValue( arg1 ) == "ustring" &&
      nl->SymbolValue( arg2 ) == "string" )
    return 4;

  if( nl->SymbolValue( arg1 ) == "uregion" &&
      nl->SymbolValue( arg2 ) == "region" )
    return 5;

//  if( nl->SymbolValue( arg1 ) == "upoint" &&
//      nl->SymbolValue( arg2 ) == "region" )
//      return 6;

  return -1; // This point should never be reached
}

ValueMapping temporalunitpassesmap[] = {
  MappingUnitPasses<UBool, CcBool>,      //0
  MappingUnitPasses<UInt, CcInt>,        //1
  MappingUnitPasses<UReal, CcReal>,      //2
  MappingUnitPasses<UPoint, Point>,      //3
  MappingUnitPasses<UString, CcString>,  //4
  MappingUnitPasses<URegion, Region> };  //5

/*
5.12.5  Definition of operator ~passes~

*/
Operator temporalunitpasses( "passes",
                         TemporalSpecPasses,
                         6,
                         temporalunitpassesmap,
                         TUPassesSelect,
                         UnitBaseTypeMapBool);

/*
5.13 Operator ~at~

The operator restrict a unit type to interval, where it's value
is equal to a given value. For base types ~bool~, ~int~ and ~point~,
the result will be only a single unit, but for base type ~real~, there
may be two units, as ~ureal~ is represented by a quadratic polynomial
function (or it's radical).

5.13.1 Type Mapping for ~at~

----
      at:     For T in {bool, int, string, point, region*}
OK  +                           uT x       T --> uT
(OK)                         ureal x    real --> (stream ureal)
OK  +                       upoint x  region --> (stream upoint)

(*): Not yet implemented

----

*/
ListExpr
TemporalUnitAtTypeMapUnit( ListExpr args )
{
  ListExpr arg1, arg2;
  if( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );

    if (TUA_DEBUG) cout << "\nTemporalUnitAtTypeMapUnit: 0" << endl;

    if( nl->IsEqual( arg1, "ubool" ) && nl->IsEqual( arg2, "bool" ) )
      return nl->SymbolAtom( "ubool" );
    if( nl->IsEqual( arg1, "uint" ) && nl->IsEqual( arg2, "int" ) )
      return nl->SymbolAtom( "uint" );
    if( nl->IsEqual( arg1, "upoint" ) && nl->IsEqual( arg2, "point" ) )
      return nl->SymbolAtom( "upoint" );
    // for ureal, _ at _ will return a stream of ureals!
    if( nl->IsEqual( arg1, "ureal" ) && nl->IsEqual( arg2, "real" ) )
      return nl->TwoElemList(nl->SymbolAtom( "stream" ),
                             nl->SymbolAtom( "ureal" ));
    if( nl->IsEqual( arg1, "ustring" ) && nl->IsEqual( arg2, "string" ) )
      return nl->SymbolAtom( "ustring" );
    if( nl->IsEqual( arg1, "uregion" ) && nl->IsEqual( arg2, "region" ) )
      return nl->SymbolAtom( "uregion" );
    if( nl->IsEqual( arg1, "upoint" ) && nl->IsEqual( arg2, "region" ) )
      return nl->TwoElemList(nl->SymbolAtom( "stream" ),
                             nl->SymbolAtom( "upoint" ));
  }
  if (TUA_DEBUG) cout << "\nTemporalUnitAtTypeMapUnit: 1" << endl;
  return nl->SymbolAtom( "typeerror" );
}

/*
5.13.2 Value Mapping for ~at~

We implement three variants, the first for unit types using ~ConstTemporalUnits~
and ~SpatialUnits~, a second one for ~ureal~, a third for ~upoint x region~

*/

// first valuemapping, for all but ureal, and upoint x region:
template <class Unit, class Alpha>
int MappingUnitAt( Word* args, Word& result, int message,
                   Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Word a0, a1;

  a0 = args[0];
  a1 = args[1];

  Unit* unit = ((Unit*)a0.addr);
  Alpha* val = ((Alpha*)a1.addr);

  Unit* pResult = ((Unit*)result.addr);

  if ( !unit->IsDefined() || !val->IsDefined() )
    pResult->SetDefined(false);
  else if (unit->At( *val, *pResult ))
    {
      pResult->SetDefined(true);
      pResult->timeInterval.start.SetDefined(true);
      pResult->timeInterval.end.SetDefined(true);
    }
  else
    {
      pResult->SetDefined(false);
      pResult->timeInterval.start.SetDefined(false);
      pResult->timeInterval.end.SetDefined(true);
    }

  return 0;
}

struct MappingUnitAt_rLocalInfo {
  bool finished;
  int  NoOfResults;  // the number of remaining results
  Word runits[2];    // the results
};

// second value mapping: (ureal real) -> (stream ureal)
int MappingUnitAt_r( Word* args, Word& result, int message,
                   Word& local, Supplier s )
{
  MappingUnitAt_rLocalInfo *localinfo;
  double radicand, a, b, c, r, y;
  DateTime t1 = DateTime(instanttype);
  DateTime t2 = DateTime(instanttype);
  Interval<Instant> rdeftime, deftime;
  UReal *uinput;
  CcReal *value;
  Word a0, a1;
  double tx, t0, A, B, C;


  switch (message)
    {
    case OPEN :

      if(TUA_DEBUG) cout << "\nMappingUnitAt_r: OPEN" << endl;
      localinfo = new MappingUnitAt_rLocalInfo;
      localinfo->finished = true;
      localinfo->NoOfResults = 0;
      if(TUA_DEBUG) cout << "  1" << endl;

      a0 = args[0];
      uinput = (UReal*)(a0.addr);
      if(TUA_DEBUG) cout << "  1.1" << endl;

      a1 = args[1];
      value = (CcReal*)(a1.addr);
      if(TUA_DEBUG) cout << "  1.2" << endl;

      if(TUA_DEBUG) cout << "  2" << endl;

      if(TUA_DEBUG) cout << "  2.1: " << uinput->IsDefined() << endl;
      if(TUA_DEBUG) cout << "  2.2: " << value->IsDefined() << endl;

      if ( !uinput->IsDefined() ||
           !value->IsDefined() )
        { // some input is undefined -> return empty stream
          if(TUA_DEBUG) cout << "  3: Some input is undefined. No result."
                             << endl;
          localinfo->NoOfResults = 0;
          localinfo->finished = true;
          local = SetWord(localinfo);
          if(TUA_DEBUG) cout << "\nMappingUnitAt_r: finished OPEN (1)"
                             << endl;
          return 0;
        }
      if(TUA_DEBUG) cout << "  4" << endl;

      y  = value->GetRealval();
      a  = uinput->a;
      b  = uinput->b;
      c  = uinput->c;
      r  = uinput->r;
      deftime = uinput->timeInterval;
      t0 = deftime.start.ToDouble();

      if(TUA_DEBUG)
        {cout << "    The UReal is" << " a= " << a << " b= "
              << b << " c= " << c << " r= " << r << endl;
          cout << "    The Real is y=" << y << endl;
          cout << "  5" << endl;
        }

      if ( (a == 0) && (b == 0) )
        { // constant function. Possibly return input unit
          if(TUA_DEBUG) cout << "  6: 1st arg is a constant value" << endl;
          if (c != y)
            { // There will be no result, just an empty stream
              if(TUA_DEBUG) cout << "  7" << endl;
              localinfo->NoOfResults = 0;
              localinfo->finished = true;
            }
          else
                { // Return the complete unit
                  if(TUA_DEBUG)
                    {
                      cout << "  8: Found constant solution" << endl;
                      cout << "    T1=" << c << endl;
                      cout << "    Tstart=" << deftime.start.ToDouble() << endl;
                      cout << "    Tend  =" << deftime.end.ToDouble() << endl;
                    }
                  localinfo->runits[localinfo->NoOfResults].addr
                    = uinput->Copy();
                  localinfo->NoOfResults++;
                  localinfo->finished = false;
                  if(TUA_DEBUG) cout << "  9" << endl;
                }
          if(TUA_DEBUG) cout << "  10" << endl;
          local = SetWord(localinfo);
          if(TUA_DEBUG)
            cout << "\nMappingUnitAt_r: finished OPEN (2)" << endl;
          return 0;
        }
      if ( (a == 0) && (b != 0) )
        { // linear function. Possibly return input unit restricted
          // to single value
          if(TUA_DEBUG) cout << "  11: 1st arg is a linear function" << endl;
          double T1 = (y - c + b*t0)/b ;
          if(TUA_DEBUG)
            {
              cout << "    T1=" << T1 << endl;
              cout << "    Tstart=" << deftime.start.ToDouble() << endl;
              cout << "    Tend  =" << deftime.end.ToDouble() << endl;
            }
          t1.ReadFrom( T1 );
          if (deftime.Contains(t1))
            { // value is contained by deftime
              if(TUA_DEBUG)
                cout << "  12: Found valid linear solution." << endl;
              localinfo->runits[localinfo->NoOfResults].addr =
                uinput->Copy();
              ((UReal*)(localinfo
                        ->runits[localinfo->NoOfResults].addr))
                ->timeInterval = Interval<Instant>(t1, t1, true, true);
              // translate result to new starting instant!
              tx =  T1 - deftime.start.ToDouble();
              ((UReal*)(localinfo
                        ->runits[localinfo->NoOfResults].addr))
                ->TranslateParab(tx);
              localinfo->NoOfResults++;
              localinfo->finished = false;
              if(TUA_DEBUG) cout << "  13" << endl;
            }
          else
            { // value is not contained by deftime -> no result
              if(TUA_DEBUG)
                cout << "  14: Found invalid linear solution." << endl;
              localinfo->NoOfResults = 0;
              localinfo->finished = true;
              if(TUA_DEBUG) cout << "  15" << endl;
            }
          if(TUA_DEBUG) cout << "  16" << endl;
          local = SetWord(localinfo);
          cout << "\nMappingUnitAt_r: finished OPEN (3)" << endl;
          return 0;
        }

      if(TUA_DEBUG) cout << "  17" << endl;
      A = a;
      B = b - 2*a*t0;
      C = t0*(a*t0-b)+c;
      radicand = pow(B,2) + 4*a*(y-C);
      if(TUA_DEBUG) cout << "    radicand =" << radicand << endl;
      if ( (a != 0) && (radicand >= 0) )
        { // quadratic function. There are possibly two result units
          // calculate the possible t-values t1, t2

/*
The solution to the equation $at^2 + bt + c = y$ is
\[t_{1,2} = \frac{-b \pm \sqrt{b^2-4a(c-y)}}{2a},\] for $b^2-4a(c-y) = b^2+4a(y-c) \geq 0$.


*/
          if(TUA_DEBUG) cout << "  18: 1st arg is a quadratic function" << endl;
          double T1 = (-B + sqrt(radicand)) / (2*A) ;
          double T2 = (-B - sqrt(radicand)) / (2*A) ;
          if(TUA_DEBUG)
            {
              cout << "    T1=" << T1 << endl;
              cout << "    T2=" << T2 << endl;
              cout << "    Tstart=" << deftime.start.ToDouble() << endl;
              cout << "    Tend  =" << deftime.end.ToDouble() << endl;
            }
          t1.ReadFrom( T1 );
          t2.ReadFrom( T2 );

          // check, whether t1 contained by deftime
          if (deftime.Contains( t1 ))
            {
              if(TUA_DEBUG)
                cout << "  19: Found first quadratic solution" << endl;
              rdeftime.start = t1;
              rdeftime.end = t1;
              rdeftime.lc = true;
              rdeftime.rc = true;
              localinfo->runits[localinfo->NoOfResults].addr =
                new UReal( rdeftime,a,b,c,r );
              ((UReal*) (localinfo->runits[localinfo->NoOfResults].addr))
                ->SetDefined( true );
              // translate result to new starting instant!
              tx = T1 - deftime.start.ToDouble();
              ((UReal*)(localinfo
                        ->runits[localinfo->NoOfResults].addr))
                ->TranslateParab(tx);
              localinfo->NoOfResults++;
              localinfo->finished = false;
              if(TUA_DEBUG) cout << "  20" << endl;
            }
          // check, whether t2 contained by deftime
          if ( !(t1 == t2) && (deftime.Contains( t2 )) )
            {
              if(TUA_DEBUG)
                cout << "  21: Found second quadratic solution" << endl;
              rdeftime.start = t2;
              rdeftime.end = t2;
              rdeftime.lc = true;
              rdeftime.rc = true;
              localinfo->runits[localinfo->NoOfResults].addr =
                new UReal( rdeftime,a,b,c,r );
              ((UReal*) (localinfo->runits[localinfo->NoOfResults].addr))
                ->SetDefined (true );
              // translate result to new starting instant!
              tx =  T2 - deftime.start.ToDouble();
              ((UReal*)(localinfo
                        ->runits[localinfo->NoOfResults].addr))
                ->TranslateParab(tx);
              localinfo->NoOfResults++;
              localinfo->finished = false;
              if(TUA_DEBUG) cout << "  22" << endl;
            }
        }
      else // negative discreminant -> there is no real solution
           //                          and no result unit
        {
          if(TUA_DEBUG) cout << "  23: No real-valued solution" << endl;
          localinfo->NoOfResults = 0;
          localinfo->finished = true;
          if(TUA_DEBUG) cout << "  24" << endl;
        }
      if(TUA_DEBUG) cout << "  25" << endl;
      local = SetWord(localinfo);
      if(TUA_DEBUG)
        cout << "\nMappingUnitAt_r: finished OPEN (4)" << endl;
      return 0;

    case REQUEST :

      if(TUA_DEBUG) cout << "\nMappingUnitAt_r: REQUEST" << endl;
      if (local.addr == 0)
        {
          if(TUA_DEBUG)
            cout << "\nMappingUnitAt_r: finished REQUEST CANCEL (1)" << endl;
          return CANCEL;
        }
      localinfo = (MappingUnitAt_rLocalInfo*) local.addr;
      if(TUA_DEBUG) cout << "\n   localinfo: finished=" << localinfo->finished
                         << " NoOfResults==" << localinfo->NoOfResults << endl;

      if (localinfo->finished)
        {
          if(TUA_DEBUG)
            cout << "\nMappingUnitAt_r: finished REQUEST CANCEL (2)" << endl;
          return CANCEL;
        }
      if ( localinfo->NoOfResults <= 0 )
        { localinfo->finished = true;
          if(TUA_DEBUG)
            cout << "\nMappingUnitAt_r: finished REQUEST CANCEL (3)" << endl;
          return CANCEL;
        }
      localinfo->NoOfResults--;
      result = SetWord( ((UReal*)(localinfo
                                  ->runits[localinfo->NoOfResults].addr))
                        ->Clone() );
      ((UReal*)(localinfo->runits[localinfo->NoOfResults].addr))
        ->DeleteIfAllowed();
      if(TUA_DEBUG) cout << "\nMappingUnitAt_r: finished REQUEST YIELD" << endl;
      return YIELD;

    case CLOSE :

      if(TUA_DEBUG) cout << "\nMappingUnitAt_r: CLOSE" << endl;
      if (local.addr != 0)
        {
          localinfo = (MappingUnitAt_rLocalInfo*) local.addr;
          for (;localinfo->NoOfResults>0;localinfo->NoOfResults--)
            ((UReal*)(localinfo->runits[localinfo->NoOfResults].addr))
              ->DeleteIfAllowed();
          delete localinfo;
        }
      if(TUA_DEBUG) cout << "\nMappingUnitAt_r: finished CLOSE" << endl;
      return 0;
    } // end switch

      // should not be reached
  return 0;
}

struct TUAUPointAtRegionLocalInfo
{
  bool finished;
  int  NoOfResults;
  int  NoOfResultsDelivered;
  MPoint *mpoint;  // Used to store results
};

// value mapping: (upoint region) -> (stream upoint)
int MappingUnitAt_up_rg( Word* args, Word& result, int message,
                         Word& local, Supplier s )
{
  // This operator creates a single-unit MPoint and a single-unit static MRegion
  // and calls MRegion::Intersection(MPoint mpoint, MPoint res)
  TUAUPointAtRegionLocalInfo *sli;
  UPoint  *u;
  Region  *r;
  MPoint  *mp_tmp;
  MRegion *mr_tmp;
  const UPoint* cu;

  switch( message )
    {
    case OPEN:

      if (TUA_DEBUG)
        cerr << "MappingUnitAt_up_rg: Received OPEN"
             << endl;

      sli = new TUAUPointAtRegionLocalInfo;
      sli->finished = true;
      sli->NoOfResults = 0;
      sli->NoOfResultsDelivered = 0;
      sli->mpoint = new MPoint(10);
      local = SetWord(sli);

      u = (UPoint*)(args[0].addr);
      r = (Region*)(args[1].addr);

      // test for definedness
      if ( !u->IsDefined() || !r->IsDefined() )
        {
          if (TUA_DEBUG)
            cerr << "  Undef arg -> Empty Result" << endl << endl;
          // nothing to do
        }
      else
        {
          mp_tmp = new MPoint(1);           // create temporary MPoint
          mp_tmp->Add(*u);
          mp_tmp->SetDefined(true);

          mr_tmp = new MRegion(*mp_tmp, *r);// create temporary MRegion
          mr_tmp->SetDefined(true);

          mr_tmp->Intersection(*mp_tmp, *(sli->mpoint));// get and save result;
          delete mp_tmp;
          delete mr_tmp;
          sli->NoOfResults = sli->mpoint->GetNoComponents();
          sli->finished = (sli->NoOfResults <= 0);
          if (TUA_DEBUG)
            cerr << "  " << sli->NoOfResults << " result units" << endl << endl;
        }
      if (TUA_DEBUG)
        cerr << "MappingUnitAt_up_rg: Finished OPEN"
             << endl;
      return 0;

    case REQUEST:
      if (TUA_DEBUG)
        cerr << "MappingUnitAt_up_rg: Received REQUEST"
             << endl;

      if(local.addr == 0)
        {
          if (TUA_DEBUG)
            cerr << "MappingUnitAt_up_rg: Finished REQUEST (1)"
                 << endl;
          return CANCEL;
        }
      sli = (TUAUPointAtRegionLocalInfo*) local.addr;
      if(sli->finished)
        {
          if (TUA_DEBUG)
            cerr << "MappingUnitAt_up_rg: Finished REQUEST (2)"
                 << endl;
          return CANCEL;
        }
      if(sli->NoOfResultsDelivered < sli->NoOfResults)
        {
          sli->mpoint->Get(sli->NoOfResultsDelivered, cu);
          result = SetWord( cu->Clone() );
          sli->NoOfResultsDelivered++;
          if (TUA_DEBUG)
            cerr << "MappingUnitAt_up_rg: "
                << "Finished REQUEST (YIELD)" << endl;
          return YIELD;
        }
      sli->finished = true;
      if (TUA_DEBUG)
        cerr << "MappingUnitAt_up_rg: Finished REQUEST (3)"
             << endl;
      return CANCEL;

    case CLOSE:

      if (TUA_DEBUG)
        cerr << "MappingUnitAt_up_rg: Received CLOSE"
             << endl;
      if (local.addr != 0)
        {
          sli = (TUAUPointAtRegionLocalInfo*) local.addr;
          delete sli->mpoint;
          delete sli;
        }
      if (TUA_DEBUG)
        cerr << "MappingUnitAt_up_rg: Finished CLOSE"
             << endl;
      return 0;
    } // end switch

  cerr << "MappingUnitAt_up_rg: Received UNKNOWN COMMAND"
       << endl;
  return 0;
}

/*
5.13.3 Specification for operator ~at~

*/
const string
TemporalSpecAt =
"( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
"( <text>For T in {bool, int, string, point, region*}:\n"
"(uT     T     ) -> uT\n"
"(ureal  real  ) -> (stream ureal)\n"
"(upoint region) -> (stream upoint)\n"
"(*): Not yet implemented</text--->"
"<text>_ at _ </text--->"
"<text>restrict the movement to the times "
"where the equality occurs.\n"
"Observe, that for type 'ureal', the result is a "
"'(stream ureal)' rather than a 'ureal'!</text--->"
"<text>upoint1 at point1</text---> ) )";

/*
5.13.4 Selection Function of operator ~at~

*/
int
TUSelectAt( ListExpr args )
{
  ListExpr arg1 = nl->First( args ),
           arg2 = nl->Second( args );

  if( nl->SymbolValue( arg1 ) == "ubool" &&
      nl->SymbolValue( arg2 ) == "bool" )
    return 0;

  if( nl->SymbolValue( arg1 ) == "uint" &&
      nl->SymbolValue( arg2 ) == "int" )
    return 1;

  if( nl->SymbolValue( arg1 ) == "ureal" &&
      nl->SymbolValue( arg2 ) == "real" )
    return 2;

  if( nl->SymbolValue( arg1 ) == "upoint" &&
      nl->SymbolValue( arg2 ) == "point" )
    return 3;

  if( nl->SymbolValue( arg1 ) == "ustring" &&
      nl->SymbolValue( arg2 ) == "string" )
    return 4;

  if( nl->SymbolValue( arg1 ) == "uregion" &&
      nl->SymbolValue( arg2 ) == "region" )
    return 5;

  if( nl->SymbolValue( arg1 ) == "upoint" &&
      nl->SymbolValue( arg2 ) == "region" )
    return 6;

  return -1; // This point should never be reached
}


ValueMapping temporalunitatmap[] = {  MappingUnitAt< UBool, CcBool>,    //0
                                      MappingUnitAt< UInt, CcInt>,      //1
                                      MappingUnitAt_r,                  //2
                                      MappingUnitAt< UPoint, Point>,    //3
                                      MappingUnitAt< UString, CcString>,//4
                                      MappingUnitAt< URegion, Region>,  //5
                                      MappingUnitAt_up_rg};             //6

/*
5.13.5  Definition of operator ~at~

*/
Operator temporalunitat( "at",
                     TemporalSpecAt,
                     7,
                     temporalunitatmap,
                     TUSelectAt,
                     TemporalUnitAtTypeMapUnit );

/*
5.14 Operator ~circle~

5.14.1 Type Mapping for ~circle~

*/
ListExpr
TypeMapCircle( ListExpr args )
{
  ListExpr arg1, arg2, arg3;
  if( nl->ListLength( args ) == 3 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );
    arg3 = nl->Third( args );


    if( nl->IsEqual( arg1, "point" ) && nl->IsEqual( arg2, "real" )
       && nl->IsEqual( arg3, "int" ) )
      return nl->SymbolAtom( "region" );
  }
  return nl->SymbolAtom( "typeerror" );
}

/*
5.14.2 Value Mapping for ~circle~

*/
int Circle( Word* args, Word& result, int message, Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Point* p = (Point*)args[0].addr; // Centre of the circle
  CcReal* r = (CcReal*)args[1].addr; // Radius of the circle.
  CcInt* narg = (CcInt*)args[2].addr; // number of edges
  Region *res = (Region*)result.addr;
  double x, y;
  int n;
  double radius;
  double valueX, valueY;
  double angle;
  int partnerno = 0;
  HalfSegment hs;

  res->Clear();                // clear the result region
  if (!p->IsDefined() || !r->IsDefined() || !narg->IsDefined() )
    { // Nothing to do
      res->SetDefined( false );
    }
  else
    {
      x = p->GetX();
      y = p->GetY();

      n = narg->GetIntval();
      radius = r->GetRealval();

      res->StartBulkLoad();
      if ((n>2)&&(n<101)&&(radius >0.0))
        {

          //  Calculate a polygon with (n) vertices and (n) edges.
          //  To get the vertices, divide 360 degree in n parts using
          //  a standardised circle around p with circumference U = 2 * PI * r.

          for( int i = 0; i < n; i++ )
            {
              // The first point/vertex of the segment
              angle = i * 2 * PI/n; // angle to starting vertex
              valueX = x + radius * cos(angle);
              valueY = y + radius * sin(angle);
              Point v1(true, valueX ,valueY);

              // The second point/vertex of the segment
              if ((i+1) >= n)            // angle to end vertex
                angle = 0 * 2 * PI/n;    // for inner vertex
              else
                angle = (i+1) * 2 * PI/n;// for ending = starting vertex
              valueX = x + radius * cos(angle);
              valueY = y + radius * sin(angle);
              Point v2(true, valueX ,valueY);

              // Create a halfsegment for this segment
              hs.Set(true, v1, v2);
              hs.attr.faceno = 0;         // only one face
              hs.attr.cycleno = 0;        // only one cycle
              hs.attr.edgeno = partnerno;
              hs.attr.partnerno = partnerno++;
              hs.attr.insideAbove = (hs.GetLeftPoint() == v1);

              // Add halfsegments 2 times with opposite LeftDomPoints
              *res += hs;
              hs.SetLeftDomPoint( !hs.IsLeftDomPoint() );
              *res += hs;
            }
        }
      res->EndBulkLoad();
      res->SetDefined( true );
    }
  return 0;
}

/*
5.14.3 Specification for operator ~circle~

*/
const string
TemporalSpecCircle =
"( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
"( <text>(point real int) -> region</text--->"
"<text>circle ( p, r, n ) </text--->"
"<text>Creates a region with a shape approximating a circle "
"with a given a given center point p and radius r>0.0 by a "
"regular polygon with 2<n<101 edges.\n"
"Parameters out of the given perimeters result in an "
"empty region, undef values in an undef one .</text--->"
"<text>circle (p,10.0,10)</text---> ) )";

/*
5.14.4 Selection Function of operator ~circle~

Not necessary.

*/

/*
5.14.5  Definition of operator ~circle~

*/
Operator temporalcircle( "circle",
                         TemporalSpecCircle,
                         Circle,
                         Operator::SimpleSelect,
                         TypeMapCircle);

/*
5.15 Operator ~makepoint~

5.15.1 Type Mapping for ~makepoint~

*/
ListExpr
TypeMapMakepoint( ListExpr args )
{
  ListExpr arg1, arg2;
  if( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );

    if( nl->IsEqual( arg1, "int" ) && nl->IsEqual( arg2, "int" ) )
      return nl->ThreeElemList(nl->SymbolAtom("APPEND"),
               nl->OneElemList(nl->IntAtom(0)), nl->SymbolAtom("point") );

    if( nl->IsEqual( arg1, "real" ) && nl->IsEqual( arg2, "real" ) )
      return nl->ThreeElemList(nl->SymbolAtom("APPEND"),
               nl->OneElemList(nl->IntAtom(1)), nl->SymbolAtom("point") );
  }
  return nl->SymbolAtom( "typeerror" );
}

/*
5.15.2 Value Mapping for ~makepoint~

*/
int MakePoint( Word* args, Word& result, int message, Word& local, Supplier s )
{
  CcInt* value1, *value2;
  CcReal* value3, *value4;
  bool paramtype;

  result = qp->ResultStorage( s );
  if ( ((CcInt*)args[2].addr)->GetIntval() == 0 )
  {
    paramtype = false;
    value1 = (CcInt*)args[0].addr;
    value2 = (CcInt*)args[1].addr;
  }

  if ( ((CcInt*)args[2].addr)->GetIntval() == 1 )
  {
    paramtype = true;
    value3 = (CcReal*)args[0].addr;
    value4 = (CcReal*)args[1].addr;
  }
  if (paramtype)
  {
   if( !value3->IsDefined() || !value4->IsDefined() )
    ((Point*)result.addr)->SetDefined( false );
   else
     ((Point*)result.addr)->Set(value3->GetRealval(),value4->GetRealval() );
  }
  else
  {
   if( !value1->IsDefined() || !value2->IsDefined() )
    ((Point*)result.addr)->SetDefined( false );
   else
     ((Point*)result.addr)->Set(value1->GetIntval(),value2->GetIntval() );
  }
  return 0;
}

/*
5.15.3 Specification for operator ~makepoint~

*/
const string
TemporalSpecMakePoint =
"( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
"( <text>int x int -> point, real x real -> point</text--->"
"<text>makepoint ( _, _ ) </text--->"
"<text>create a point from two "
"given real or integer coordinates.</text--->"
"<text>makepoint (5.0,5.0)</text---> ) )";

/*
5.15.4 Selection Function of operator ~makepoint~

Not necessary.

*/

/*
5.15.5  Definition of operator ~makepoint~

*/
Operator temporalmakepoint( "makepoint",
                            TemporalSpecMakePoint,
                            MakePoint,
                            Operator::SimpleSelect,
                            TypeMapMakepoint);

/*
5.16 Operator ~velocity~

Type mapping for ~velocity~ is

----
      mpoint  ->  mpoint
      upoint  ->  upoint

----

*/
ListExpr
TypeMapVelocity( ListExpr args )
{
  ListExpr arg1;
  if( nl->ListLength( args ) == 1 )
    {
      arg1 = nl->First( args );

      if( nl->IsEqual( arg1, "mpoint" ) )
        return nl->SymbolAtom( "mpoint" );
      if( nl->IsEqual( arg1, "upoint" ) )
        return nl->SymbolAtom( "upoint" );
    }
  return nl->SymbolAtom( "typeerror" );
}

/*
5.16.2 Value Mapping for ~velocity~

*/
int MPointVelocity(Word* args, Word& result, int message,
                   Word& local, Supplier s)
{
  result = (qp->ResultStorage( s ));
  MPoint *res = (MPoint*) result.addr;
  MPoint *input = (MPoint*)args[0].addr;

  res->Clear();
  if ( input->IsDefined() )
    // call member function:
    input->MVelocity( *res );
  else
    {
      res->Clear();             // use empty value
      //res->SetDefined(false); // instead of undef value
    }
  return 0;
}

int UnitPointVelocity(Word* args, Word& result, int message,
                      Word& local, Supplier s)
{
  result = (qp->ResultStorage( s ));
  UPoint *input = (UPoint*)args[0].addr;
  UPoint *res   = (UPoint*)result.addr;

  if ( !input->IsDefined() )
    res->SetDefined( false );
  else
    input->UVelocity( *res );
  return 0;
}

/*
5.16.3 Specification for operator ~velocity~

*/
const string
TemporalSpecVelocity=
"( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
"( <text>mpoint -> mpoint\n"
"upoint -> upoint</text--->"
"<text>velocity ( _ ) </text--->"
"<text>describes the vector of the speed "
"of the given temporal spatial object (i.e. the "
"coponemtwise speed in unit/s).</text--->"
"<text>velocity (mpoint)</text---> ) )";

/*
5.16.4 Selection Function of operator ~velocity~

*/
int
VelocitySelect( ListExpr args )
{
  ListExpr arg1 = nl->First( args );

  if( nl->SymbolValue( arg1 ) == "mpoint")
    return 0;

  if( nl->SymbolValue( arg1 ) == "upoint")
    return 1;

  return -1; // This point should never be reached
}

ValueMapping temporalvelocitymap[] = { MPointVelocity,
                                       UnitPointVelocity };

/*
5.16.5  Definition of operator ~velocity~

*/
Operator temporalvelocity( "velocity",
                           TemporalSpecVelocity,
                           2,
                           temporalvelocitymap,
                           VelocitySelect,
                           TypeMapVelocity);

/*
5.17 Operator ~derivable~

5.17.1 Type Mapping for ~derivable~

Type mapping for ~derivable~ is

----  mreal  ->  mbool

----

*/
ListExpr
TypeMapDerivable( ListExpr args )
{
  ListExpr arg1;
  if( nl->ListLength( args ) == 1 )
    {
      arg1 = nl->First( args );

      if( nl->IsEqual( arg1, "mreal" ) )
        return nl->SymbolAtom( "mbool" );
      if( nl->IsEqual( arg1, "ureal" ) )
        return nl->SymbolAtom( "ubool" );
    }
  return nl->SymbolAtom( "typeerror" );
}

/*
5.17.2 Value Mapping for ~derivable~

*/
int MPointDerivable( Word* args, Word& result, int message,
                     Word& local, Supplier s )
{

  result = qp->ResultStorage( s );
  MReal* value = (MReal*)args[0].addr;
  MBool* res = ((MBool*)result.addr);

  const UReal *uReal;
  CcBool b;

  res->Clear();

  if ( !value->IsDefined() )
    res->SetDefined(false);
  else
    {
      res->StartBulkLoad();
      for( int i = 0; i < value->GetNoComponents(); i++ )
        {
          value->Get( i, uReal ); // Load a real unit.

          // FALSE means in this case that a real unit describes a quadratic
          // polynomial. A derivation is possible and the operator returns TRUE.
          if (uReal->r == false)
            b.Set(true,true);
          else
            b.Set(true,false);

          UBool boolvalue(uReal->timeInterval,b);
          res->Add( boolvalue );
        }
      res->EndBulkLoad( false );
    }
  return 0;
}

int UnitPointDerivable( Word* args, Word& result, int message,
                        Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  UReal* uReal = (UReal*)args[0].addr;
  UBool* res = ((UBool*)result.addr);

  CcBool b;

  if (uReal->IsDefined())
    {
      res->timeInterval = uReal->timeInterval;

      if (uReal->r == false)
        b.Set(true,true);
      else
        b.Set(true,false);

      UBool boolvalue(uReal->timeInterval,b);
      res->CopyFrom(&boolvalue);
    }
  else
    res->SetDefined( false );

  return 0;
}

/*
5.17.3 Specification for operator ~derivable~

*/
const string
TemporalSpecDerivable=
"( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
"( <text>mreal -> mbool\n"
"ureal -> ubool</text--->"
"<text>derivable ( _ ) </text--->"
"<text>Returns a moving/unit bool decribing the "
"derivability of the moving/unit real over time.</text--->"
"<text>derivable (mreal)</text---> ) )";

/*
5.17.4 Selection Function of operator ~derivable~

*/

int
DerivableSelect( ListExpr args )
{
  ListExpr arg1 = nl->First( args );

  if( nl->SymbolValue( arg1 ) == "mreal")
    return 0;

  if( nl->SymbolValue( arg1 ) == "ureal")
    return 1;

  return -1; // This point should never be reached
}

ValueMapping temporalderivablemap[] = { MPointDerivable,
                                        UnitPointDerivable };

/*
5.17.5  Definition of operator ~derivable~

*/

Operator temporalderivable( "derivable",
                            TemporalSpecDerivable,
                            2,
                            temporalderivablemap,
                            DerivableSelect,
                            TypeMapDerivable);

/*
5.18 Operator ~derivative~

5.18.1 Type Mapping for ~derivative~

Type mapping for ~derivative~ is

----  mreal  ->  mreal

----

*/
ListExpr
TypeMapDerivative( ListExpr args )
{
  ListExpr arg1;
  if( nl->ListLength( args ) == 1 )
    {
      arg1 = nl->First( args );

      if( nl->IsEqual( arg1, "mreal" ) )
        return nl->SymbolAtom( "mreal" );
      if( nl->IsEqual( arg1, "ureal" ) )
        return nl->SymbolAtom( "ureal" );
    }
  return nl->SymbolAtom( "typeerror" );
}


/*
5.18.2 Value Mapping for ~derivative~

*/
int MPointDerivative( Word* args, Word& result, int message,
                      Word& local, Supplier s )
{

  result = qp->ResultStorage( s );
  MReal* value = (MReal*)args[0].addr;
  MReal* res = ((MReal*)result.addr);

  const UReal *Unit;
  UReal uReal(true);

  res->Clear();
  if ( value->IsDefined() )
    {
      res->StartBulkLoad();

      for( int i = 0; i < value->GetNoComponents(); i++ )
        { // load a real unit
          value->Get( i, Unit );
          if (Unit->IsDefined() && !Unit->r)
            { // The derivative of this quadratic polynom is 2at + b + 0
              uReal.timeInterval = Unit->timeInterval;
              uReal.a = 0;
              uReal.b = 2 * Unit->a;
              uReal.c = Unit->b;
              uReal.r = Unit->r;
              uReal.SetDefined(true);
              res->Add( uReal );
            }
          // else: Do nothing. Do NOT add an undefined unit!
          //       (That would conflict e.g. with operator deftime())
        }
      res->EndBulkLoad( false ); // no sorting, assuming the input was ordered
    }
  else // value is undefined
    res->SetDefined( false );
  return 0;
}

int UnitPointDerivative( Word* args, Word& result, int message,
                         Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  UReal *Unit = (UReal*)args[0].addr;
  UReal *res  = (UReal*)result.addr;

  if (Unit->IsDefined() && !Unit->r)
    {
      res->timeInterval = Unit->timeInterval;
      res->a = 0;
      res->b = 2 * Unit->a;
      res->c = Unit->b;
      res->r = Unit->r;
      res->SetDefined(true);
    }
  else // Unit is undefined
    {
      DateTime t = DateTime(instanttype);
      res->timeInterval = Interval<Instant>(t,t,true,true);
      res->a = 0;
      res->b = 0;
      res->c = 0;
      res->r = false;
      res->SetDefined(false);
    }
  return 0;
}

/*
5.18.3 Specification for operator ~derivative~

*/
const string
TemporalSpecDerivative=
"( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
"( <text>{m|u}real -> {m|u}real</text--->"
"<text>derivative ( _ ) </text--->"
"<text>Determine the derivative"
" of a mreal/ureal value.</text--->"
"<text>derivable (mreal)</text---> ) )";

/*
5.18.4 Selection Function of operator ~deriavtive~

Uses ~DerivableSelect~.

*/

ValueMapping temporalderivativemap[] = { MPointDerivative,
                                         UnitPointDerivative };

/*
5.18.5  Definition of operator ~derivative~

*/
Operator temporalderivative( "derivative",
                             TemporalSpecDerivative,
                             2,
                             temporalderivativemap,
                             DerivableSelect,
                             TypeMapDerivative);

/*
5.19 Operator ~sfeed~

Moved to ~StreamAlgebra~ and renamed it to ~feed~.

*/


/*
5.20 Operator ~suse~

Moved to ~StreamAlgebra~ and renamed it to ~use~ resp. ~use2~.

*/

/*
5.21 Operator ~distance~

The operator calculates the minimum distance between two units of base
types ~int~ or ~point~. The distance is always a non-negative ~ureal~ value.

----
    For T in {int, point}
    distance: uT x uT -> ureal
              uT x  T -> ureal
               T x uT -> ureal

----


5.21.1 Type mapping function for ~distance~

For signatures

----
           For T in {int, point}:
              uT x  T -> ureal
               T x uT -> ureal

----

typemapping appends the argument number of the argument containing
the unitvalue (either 0 or 1).

*/

ListExpr
TypeMapTemporalUnitDistance( ListExpr args )
{
  ListExpr first, second;
  string outstr1, outstr2;

  if ( nl->IsAtom( args ) || nl->ListLength( args ) != 2 )
    {
      nl->WriteToString(outstr1, args);
      ErrorReporter::ReportError("Operator distance expects a list of "
                                 "length two, but gets '" + outstr1 +
                                 "'.");
      return nl->SymbolAtom( "typeerror" );
    }

  first = nl->First(args);
  second = nl->Second(args);

  // check for compatibility of arguments
  if( !nl->IsAtom(first) || !nl->IsAtom(second) )
    {
      nl->WriteToString(outstr1, first);
      nl->WriteToString(outstr2, second);
      ErrorReporter::ReportError("Operator distance expects as arguments "
                                 "lists of length one, but gets '" + outstr1 +
                                 "' and '" + outstr2 + "'.");
      return nl->SymbolAtom( "typeerror" );
    }

  if( nl->IsEqual(first, "upoint") && nl->IsEqual(second, "upoint") )
    {
      return nl->SymbolAtom("ureal");
    }

  if( nl->IsEqual(first, "upoint") && nl->IsEqual(second, "point") )
    {
      return nl->ThreeElemList(nl->SymbolAtom("APPEND"),
                               nl->OneElemList(nl->IntAtom(0)),
                               nl->SymbolAtom( "ureal" ));
    }

  if( nl->IsEqual(first, "point") && nl->IsEqual(second, "upoint") )
    {
      return nl->ThreeElemList(nl->SymbolAtom("APPEND"),
                               nl->OneElemList(nl->IntAtom(1)),
                               nl->SymbolAtom( "ureal" ));
    }

  if( nl->IsEqual(first, "uint") && nl->IsEqual(second, "uint") )
    {
      return nl->SymbolAtom("ureal");
    }

  if( nl->IsEqual(first, "uint") && nl->IsEqual(second, "int") )
    {
      return nl->ThreeElemList(nl->SymbolAtom("APPEND"),
                               nl->OneElemList(nl->IntAtom(0)),
                               nl->SymbolAtom( "ureal" ));
    }

  if( nl->IsEqual(first, "int") && nl->IsEqual(second, "uint") )
    {
      return nl->ThreeElemList(nl->SymbolAtom("APPEND"),
                               nl->OneElemList(nl->IntAtom(1)),
                               nl->SymbolAtom( "ureal" ));
    }

  nl->WriteToString(outstr1, first);
  nl->WriteToString(outstr2, second);
  ErrorReporter::ReportError("Operator distance found wrong argument "
                             "configuration '" + outstr1 +
                             "' and '" + outstr2 + "'.");
  return nl->SymbolAtom( "typeerror" );
}

/*
5.21.2 Value mapping for operator ~distance~
*/


/*
(a1) Value mapping for

---- (upoint upoint) -> ureal

----

Method ~UPointDistance~

Returns the distance between two UPoints in their common interval as UReal

*/
void UPointDistance( const UPoint& p1, const UPoint& p2,
                     UReal& result, Interval<Instant>& iv)
{
  if (TUA_DEBUG)
    {
      cout << "UPointDistance:" << endl;
      cout << "  p1=" << TUPrintUPoint(p1) << endl;
      cout << "  p2=" << TUPrintUPoint(p2) << endl;
    }
  result.timeInterval = iv;

  Point rp10, rp11, rp20, rp21;
  double
    x10, x11, x20, x21,
    y10, y11, y20, y21,
    dx1, dy1,
    dx2, dy2,
    dx12, dy12;

  // for calculation of temporal function:
  // ignore temporal limits
  p1.TemporalFunction(iv.start, rp10, true);
  p1.TemporalFunction(iv.end,   rp11, true);
  p2.TemporalFunction(iv.start, rp20, true);
  p2.TemporalFunction(iv.end,   rp21, true);

  if (TUA_DEBUG)
    {
      cout << "   iv=" << TUPrintTimeInterval(iv) << endl;
      cout << "  rp10=" << TUPrintPoint(rp10) << ", rp11="
           << TUPrintPoint(rp11) << endl;
      cout << "  rp20=" << TUPrintPoint(rp20) << ", rp21="
           << TUPrintPoint(rp21) << endl;
    }
  if ( AlmostEqual(rp10,rp20) && AlmostEqual(rp11,rp21) )
    { // identical points -> zero distance!
      if (TUA_DEBUG)
        cout << "  identical points -> zero distance!" << endl;
      result.a = 0.0;
      result.b = 0.0;
      result.c = 0.0;
      result.r = false;
      return;
    }

  DateTime DT = iv.end - iv.start;
  double   dt = DT.ToDouble();
  //double   t0 = iv.start.ToDouble();
  x10 = rp10.GetX(); y10 = rp10.GetY();
  x11 = rp11.GetX(); y11 = rp11.GetY();
  x20 = rp20.GetX(); y20 = rp20.GetY();
  x21 = rp21.GetX(); y21 = rp21.GetY();
  dx1 = x11 - x10;   // x-difference final-initial for u1
  dy1 = y11 - y10;   // y-difference final-initial for u1
  dx2 = x21 - x20;   // x-difference final-initial for u2
  dy2 = y21 - y20;   // y-difference final-initial for u2
  dx12 = x10 - x20;  // x-distance at initial instant
  dy12 = y10 - y20;  // y-distance at initial instant

  if ( AlmostEqual(dt, 0) )
    { // almost equal start and end time -> constant distance
      if (TUA_DEBUG)
        cout << "  almost equal start and end time -> constant distance!"
             << endl;
      result.a = 0.0;
      result.b = 0.0;
      result.c =   pow( ( (x11-x10) - (x21-x20) ) / 2, 2)
                 + pow( ( (y11-y10) - (y21-y20) ) / 2, 2);
      result.r = true;
      return;
    }

  if (TUA_DEBUG)
    cout << "  Normal distance calculation." << endl;

  double a1 = (pow((dx1-dx2),2)+pow(dy1-dy2,2))/pow(dt,2);
  double b1 = dx12 * (dx1-dx2);
  double b2 = dy12 * (dy1-dy2);

  result.a = a1;
  result.b = 2*(b1+b2)/dt;
  result.c = pow(dx12,2) + pow(dy12,2);
  result.r = true;

/*
For using the original ureal representation (without translation),
use the following code instead:

----

  result.a = a1;
  result.b = -2*(  (t0*a1)
                 - ( b1 + b2 )/dt
               );
  result.c =   pow(t0,2) * a1
             - 2*t0*(b1 + b2)/dt
             + pow(dx12,2) + pow(dy12,2);
  result.r = true;

----

*/


}


int TUDistance_UPoint_UPoint( Word* args, Word& result, int message,
                              Word& local, Supplier s )
{
  Interval<Instant> iv;

  //  Word a1, a2;
  UPoint *u1, *u2;
  result = qp->ResultStorage( s );
  UReal* res = (UReal*) result.addr;

  if (TUA_DEBUG) cout << "TUDistance_UPoint_UPoint: 1" << endl;

  u1 = (UPoint*)(args[0].addr);
  u2 = (UPoint*)(args[1].addr);
  if (!u1->IsDefined() ||
      !u2->IsDefined() ||
      !u1->timeInterval.Intersects( u2->timeInterval ) )
    { // return undefined ureal
      res->SetDefined( false );
      if (TUA_DEBUG) cout << "TUDistance_UPoint_UPoint: 2" << endl;
    }
  else
    { // get intersection of deftime intervals
      if (TUA_DEBUG)
        cout << "TUDistance_UPoint_UPoint:" << endl
             << "   iv1=" << TUPrintTimeInterval(u1->timeInterval) << endl
             << "   iv2=" << TUPrintTimeInterval(u2->timeInterval) << endl;

      if (TUA_DEBUG) cout << "TUDistance_UPoint_UPoint: 3" << endl;
      u1->timeInterval.Intersection( u2->timeInterval, iv );
      if (TUA_DEBUG) cout << "TUDistance_UPoint_UPoint: iv="
                          << TUPrintTimeInterval(iv) << endl;

      // calculate result
      if (TUA_DEBUG) cout << "TUDistance_UPoint_UPoint: 5" << endl;
      UPointDistance( *u1, *u2,  *res, iv);
      if (TUA_DEBUG) cout << "TUDistance_UPoint_UPoint: 6" << endl;
      res->SetDefined( true );
    }
  // pass on result
  if (TUA_DEBUG) cout << "TUDistance_UPoint_UPoint: 7" << endl;
  return 0;
}



/*
(a2) value mapping for

----
     (upoint point) -> ureal
     (point upoint) -> ureal

----

*/

int TUDistance_UPoint_Point( Word* args, Word& result, int message,
                             Word& local, Supplier s )
{
  Word  thePoint, theUPoint;
  int   argConfDescriptor2;

  // get argument configuration
  argConfDescriptor2 = ((CcInt*)args[2].addr)->GetIntval();
  if (argConfDescriptor2 == 0)
    {
      theUPoint = args[0];
      thePoint  = args[1];
    }
  else if (argConfDescriptor2 == 1)
    {
      theUPoint = args[1];
      thePoint  = args[0];
    }
  else
    {
      cout << "\nWrong argument configuration in "
           << "'TUDistance_UPoint_Point'. argConfDescriptor2="
           << argConfDescriptor2 << endl;
      return 0;
    }

  result = qp->ResultStorage( s );

  if ( !((Point*)(thePoint.addr))->IsDefined() ||
       !((UPoint*)(theUPoint.addr))->IsDefined() )
    {
      ((UReal*)(result.addr))->SetDefined ( false );
    }
  else
    {
      ((UPoint*)(theUPoint.addr))->Distance( *((Point*)(thePoint.addr)),
                                             *((UReal*)(result.addr)));
      ((UReal*)(result.addr))->SetDefined ( true );
    }
  return 0;
}


/*
(b1) value mapping for

---- (uint uint) -> ureal

----

*/
int TUDistance_UInt_UInt( Word* args, Word& result, int message,
                          Word& local, Supplier s )
{
  Interval<Instant> iv;

  Word a1, a2;
  UInt *u1, *u2;
  double c1, c2, c;

  result = qp->ResultStorage( s );

  a1 = args[0];
  a2 = args[1];

  u1 = (UInt*)(a1.addr);
  u2 = (UInt*)(a2.addr);

  if (!u1->IsDefined() ||
      !u2->IsDefined() ||
      !u1->timeInterval.Intersects( u2->timeInterval ) )
    { // return undefined ureal
      ((UReal*)(result.addr))->SetDefined( false );
    }
  else
    { // get intersection of deftime intervals
      u1->timeInterval.Intersection( u2->timeInterval, iv );

      // calculate  result
      // (as the result is constant, no translation step is required)
      c1 = (double) u2->constValue.GetIntval();
      c2 = (double) u2->constValue.GetIntval();
      c = fabs(c1 - c2);
      *((UReal*)(result.addr)) = UReal(iv, 0, 0, c, false);
      ((UReal*)(result.addr))->SetDefined( true );
    }
  // pass on result
  return 0;
}

/*
(b2) value mapping for

---- ((uint int) -> ureal) and ((int uint) -> ureal)

----

*/
int TUDistance_UInt_Int( Word* args, Word& result, int message,
                         Word& local, Supplier s )
{
  Word  ii, ui;
  int   argConfDescriptor2;
  UInt  *u;
  CcInt *i;
  double c1, c2, c;

  // get argument configuration
  argConfDescriptor2 = ((CcInt*)args[2].addr)->GetIntval();
  if (argConfDescriptor2 == 0)
    {
      ui = args[0];
      ii = args[1];
    }
  else if (argConfDescriptor2 == 1)
    {
      ui = args[1];
      ii = args[0];
    }
  else
    {
      cout << "\nWrong argument configuration in "
           << "'TUDistance_UInt_Int'. argConfDescriptor2="
           << argConfDescriptor2 << endl;
      return 0;
    }

  result = qp->ResultStorage( s );

  u = (UInt*)(ui.addr);
  i = (CcInt*)(ii.addr);

  if (!u->IsDefined() ||
      !i->IsDefined() )
    { // return undefined ureal
      ((UReal*)(result.addr))->SetDefined( false );
    }
  else
    { // calculate  result
      // (as the result is constant, no translation step is required)
      c1 = (double) u->constValue.GetIntval();
      c2 = (double) i->GetIntval();
      c = fabs(c1 - c2);
      *((UReal*)(result.addr)) = UReal(u->timeInterval, 0, 0, c, false);
      ((UReal*)(result.addr))->SetDefined( true );
    }
  // pass on result
  return 0;
}



/*
5.21.3 Specification for operator ~distance~

*/

const string TemporalSpecDistance =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "("
  "<text>For T in {point, int}:\n"
  "(uT uT) -> ureal\n"
  "(uT  T) -> ureal\n"
  "( T uT) -> ureal</text--->"
  "<text>distance( _, _)</text--->"
  "<text>Calculates the distance of both arguments, "
  "whereof at least one is a unittype, "
  "as a 'ureal' value. </text--->"
  "<text>distance(upoint1,point1)</text--->"
  ") )";

/*
5.21.4 Selection Function of operator ~distance~

*/

ValueMapping temporalunitdistancemap[] =
  {
    TUDistance_UPoint_UPoint,
    TUDistance_UPoint_Point,
    TUDistance_UInt_UInt,
    TUDistance_UInt_Int
  };

int temporalunitDistanceSelect( ListExpr args )
{
  ListExpr first  = nl->First(args);
  ListExpr second = nl->Second(args);

  if( nl->IsEqual(first, "upoint") && nl->IsEqual(second, "upoint") )
    return 0;

  else if( nl->IsEqual(first, "upoint") && nl->IsEqual(second, "point") )
    return 1;

  else if( nl->IsEqual(first, "point") && nl->IsEqual(second, "upoint") )
    return 1;

  else if( nl->IsEqual(first, "uint") && nl->IsEqual(second, "uint") )
    return 2;

  else if( nl->IsEqual(first, "uint") && nl->IsEqual(second, "int") )
    return 3;

  else if( nl->IsEqual(first, "int") && nl->IsEqual(second, "uint") )
    return 3;

  else
    cout << "\nERROR in temporalunitDistanceSelect!" << endl;

  return -1;
}

/*
5.21.5 Definition of operator ~distance~

*/

Operator temporalunitdistance( "distance",
                               TemporalSpecDistance,
                               4,
                               temporalunitdistancemap,
                               temporalunitDistanceSelect,
                               TypeMapTemporalUnitDistance);

/*
5.22 Operator ~atmax~

From a given unit ~u~, the operator creates a stream of units that
are restrictions of ~u~ to the periods where it reaches its maximum
value.

----
       For T in {int, real}
       atmax: uT --> (stream uT)

----

5.22.1 Type mapping function for ~atmax~

*/
ListExpr
UnitBaseTypeMapAtmax( ListExpr args )
{
  ListExpr arg1;
  if( nl->ListLength( args ) == 1 )
    {
      arg1 = nl->First( args );

      if( nl->IsEqual( arg1, "ubool" ) )
        return nl->SymbolAtom( "ubool" );
      if( nl->IsEqual( arg1, "uint" ) )
        return nl->SymbolAtom( "uint" );
      if( nl->IsEqual( arg1, "ustring" ) )
        return nl->SymbolAtom( "ustring" );
      // for ureal, atmax/atmin will return a stream of ureals!
      if( nl->IsEqual( arg1, "ureal" ) )
        return nl->TwoElemList(nl->SymbolAtom( "stream" ),
                               nl->SymbolAtom( "ureal" ));
    }
  return nl->SymbolAtom( "typeerror" );
}
/*
5.22.2 Value mapping for operator ~atmax~

*/

struct AtExtrURealLocalInfo
{
  int NoOfResults;
  int ResultsDelivered;
  UReal* t_res[3];
};

int getMaxValIndex( double& first, double& second, double& third)
{ // returns a 3-bit field indicating smallest values

  if ( (first > second) && (first > third) )
    return 1;
  if ( (second > first) && (second > third) )
    return 2;
  if ( (first == second) && (second > third) )
    return 3;
  if ( (third > first) && (third > second) )
    return 4;
  if ( (first == third) && (first > second) )
    return 5;
  if ( (second == third) && (second > first) )
    return 6;
  return 7; // they are all equal
}

double getValUreal(const double& t,
                   const double& a,
                   const double& b,
                   const double& c,
                   const bool& r)
{
  double tmp;
  tmp = a*pow(t,2) + b*t + c;
  if (r)
    return sqrt(tmp);
  else
    return tmp;
}


int atmaxUReal( Word* args, Word& result, int message,
                Word& local, Supplier s )
{
  AtExtrURealLocalInfo *sli;
  UReal                *ureal;
  double  t_start, t_end, t_extr; // instants of interest
  double  v_start, v_end, v_extr; // values at resp. instants
  double  a, b, c, r, tx;
  int     maxValIndex;
  Instant t = DateTime(instanttype);
  Word    a0;

  result = qp->ResultStorage( s );

  switch (message)
    {
    case OPEN :

      if(TUA_DEBUG) cout << "\natmaxUReal: OPEN " << endl;
      a0 = args[0];
      ureal = (UReal*)(a0.addr);
      if(TUA_DEBUG)
        cout << "  Argument ureal value: " << TUPrintUReal(ureal) << endl
             << "  1" << endl;
      sli = new AtExtrURealLocalInfo;
      sli->NoOfResults = 0;
      sli->ResultsDelivered = 0;
      local = SetWord(sli);
      if(TUA_DEBUG) cout << "  2" << endl;

      if ( !(ureal->IsDefined()) )
        { // ureal undefined
          // -> return empty stream
          if(TUA_DEBUG) cout << "  2.1: ureal undefined" << endl;
          sli->NoOfResults = 0;
          if(TUA_DEBUG)
            cout << "atmaxUReal: OPEN  finished (1)" << endl;
          return 0;
        }
      if(TUA_DEBUG) cout << "  3" << endl;

      if ( (ureal->timeInterval.start).ToDouble() ==
           (ureal->timeInterval.end).ToDouble() )
        { // ureal contains only a single point.
          // -> return a copy of the ureal
          if(TUA_DEBUG)
            cout << "  3.1: ureal contains only a single point" << endl;
          sli->t_res[sli->NoOfResults] = (UReal*) (ureal->Copy());
          if(TUA_DEBUG)
            cout << "       res="
                 << TUPrintUReal(sli->t_res[sli->NoOfResults])
                 << endl;
          sli->NoOfResults++;
          if(TUA_DEBUG)
            cout << "atmaxUReal: OPEN  finished (2)" << endl;
          return 0;
        }
      if(TUA_DEBUG) cout << "  4" << endl;

      if (ureal->a == 0)
        {
          if ( ureal->b == 0 )
            { //  constant function
              // the only result is a copy of the argument ureal
              if(TUA_DEBUG)
                cout << "  4.1: constant function" << endl;
              sli->t_res[sli->NoOfResults] = (UReal*) (ureal->Copy());
              // no translation needed for constant ureal!
              if(TUA_DEBUG)
                cout << "       res " << sli->NoOfResults+1 << "="
                     << TUPrintUReal(sli->t_res[sli->NoOfResults])
                     << endl;
              sli->NoOfResults++;
              if(TUA_DEBUG)
                cout << "atmaxUReal: OPEN  finished (3)" << endl;
              return 0;
            }
          if ( ureal->b < 0 )
            { // linear fuction
              // the result is a clone of the argument, restricted to
              // its starting instant
              sli->t_res[sli->NoOfResults] = ureal->Clone();
              if(TUA_DEBUG)
                cout << "  4.2: linear function/initial" << endl;
              sli->t_res[sli->NoOfResults]->timeInterval.end =
                sli->t_res[sli->NoOfResults]->timeInterval.start;
              sli->t_res[sli->NoOfResults]->timeInterval.rc = true;
              // no translation needed, since remaining starting instant
              if(TUA_DEBUG)
                cout << "       res " << sli->NoOfResults+1 << "="
                     << TUPrintUReal(sli->t_res[sli->NoOfResults])
                     << endl;
              sli->NoOfResults++;
              if(TUA_DEBUG)
                cout << "atmaxUReal: OPEN  finished (4)" << endl;
              return 0;
            }
          if ( ureal->b > 0 )
            { // linear fuction
              // the result is a clone of the argument, restricted to
              // its final instant
              sli->t_res[sli->NoOfResults] = ureal->Clone();
              if(TUA_DEBUG)
                cout << "  4.3: linear function/final" << endl;
              sli->t_res[sli->NoOfResults]->timeInterval.start =
                sli->t_res[sli->NoOfResults]->timeInterval.end;
              sli->t_res[sli->NoOfResults]->timeInterval.lc = true;
              // translate result to new starting instant!
              tx = ureal->timeInterval.end.ToDouble() -
                   ureal->timeInterval.start.ToDouble();
              (sli->t_res[sli->NoOfResults])->TranslateParab(tx);
              if(TUA_DEBUG)
                cout << "       res " << sli->NoOfResults+1 << "="
                     << TUPrintUReal(sli->t_res[sli->NoOfResults])
                     << endl;
              sli->NoOfResults++;
              if(TUA_DEBUG)
                cout << "atmaxUReal: OPEN  finished (5)" << endl;
              return 0;
            }
        }
      if(TUA_DEBUG) cout << "  5" << endl;

      if (ureal->a != 0)
        { // quadratic function
          // we have to additionally check for the extremum
          if(TUA_DEBUG) cout << "  5.1: quadratic function" << endl;

          // get the times of interest
          a = ureal->a;
          b = ureal->b;
          c = ureal->c;
          r = ureal->r;
          t_start = (ureal->timeInterval.start).ToDouble();
          t_extr  = -b/(2*a);
          t_end   = (ureal->timeInterval.end).ToDouble();
          // get the values of interest
          v_start = getValUreal(t_start,a,b,c,r);
          v_extr  = getValUreal(t_extr, a,b,c,r);
          v_end   = getValUreal(t_end,  a,b,c,r);
          if(TUA_DEBUG)
            cout << "  5.2" << endl
                 << "\tt_start=" << t_start << "\t v_start=" << v_start << endl
                 << "\tt_extr =" << t_extr  << "\t v_extr =" << v_extr  << endl
                 << "\tt_end  =" << t_end   << "\t v_end  =" << v_end   << endl;

          // compute, which values are maximal
          if ( (t_start < t_extr) && (t_end   > t_extr) )
            { // check all 3 candidates
              if(TUA_DEBUG) cout << "  5.3: check all 3 candidates" << endl;
              maxValIndex = getMaxValIndex(v_extr,v_start,v_end);
              if(TUA_DEBUG)
                cout << "  5.3  maxValIndex=" << maxValIndex << endl;
            }
          else
            { // extremum not within interval --> possibly 2 results
              if(TUA_DEBUG)
                cout << "  5.4: extremum not in interv (2 candidates)" << endl;
              maxValIndex = 0;
              if (v_start >= v_end) // max at t_start
                maxValIndex += 2;
              if (v_end >= v_start) // max at t_end
                maxValIndex += 4;
              if(TUA_DEBUG)
                cout << "  5.4  maxValIndex=" << maxValIndex << endl;
            }
          if(TUA_DEBUG) cout << "  5.5" << endl;
          if (maxValIndex & 2)
            { // start value
              if(TUA_DEBUG) cout << "  5.6: added start value" << endl;
              sli->t_res[sli->NoOfResults] = ureal->Clone();
              t = ureal->timeInterval.start;
              Interval<Instant> i( t, t, true, true );
              sli->t_res[sli->NoOfResults]->timeInterval = i;
              // no translation required
              if(TUA_DEBUG)
                cout << "       res " << sli->NoOfResults+1 << "="
                     << TUPrintUReal(sli->t_res[sli->NoOfResults])
                     << endl;
              sli->NoOfResults++;
            }
          if ( ( maxValIndex & 4 ) && ( t_end != t_start ) )
            { // end value
              if(TUA_DEBUG) cout << "  5.7: added end value" << endl;
              sli->t_res[sli->NoOfResults] = ureal->Clone();
              t = ureal->timeInterval.end;
              Interval<Instant> i( t, t, true, true );
              sli->t_res[sli->NoOfResults]->timeInterval = i;
              // translate result to new starting instant!
              tx = t.ToDouble() - ureal->timeInterval.start.ToDouble();
              (sli->t_res[sli->NoOfResults])->TranslateParab(tx);
              if(TUA_DEBUG)
                cout << "       res " << sli->NoOfResults+1 << "="
                     << TUPrintUReal(sli->t_res[sli->NoOfResults])
                     << endl;
              sli->NoOfResults++;
            }

          if ( (maxValIndex & 1)   &&
               (t_extr != t_start) &&
               (t_extr != t_end)      )
            {
              if(TUA_DEBUG) cout << "  5.8: added extremum" << endl;
              sli->t_res[sli->NoOfResults] = ureal->Clone();
              t.ReadFrom(t_extr);
              Interval<Instant> i( t, t, true, true );
              sli->t_res[sli->NoOfResults]->timeInterval = i;
              // translate result to new starting instant!
              tx = t_extr - ureal->timeInterval.start.ToDouble();
              (sli->t_res[sli->NoOfResults])->TranslateParab(tx);
              if(TUA_DEBUG)
                cout << "       res " << sli->NoOfResults+1 << "="
                     << TUPrintUReal(sli->t_res[sli->NoOfResults])
                     << endl;
              sli->NoOfResults++;
            }
          if(TUA_DEBUG)
            cout << "atmaxUReal: OPEN  finished (6)" << endl;
          return 0;
        }
      if(TUA_DEBUG) cout << "  6" << endl;
      cout << "\natmaxUReal (OPEN): This should not happen!" << endl;
      if(TUA_DEBUG) cout << "atmaxUReal: OPEN  finished (7)" << endl;
      return 0;

    case REQUEST :

      if(TUA_DEBUG) cout << "\natmaxUReal: REQUEST" << endl;
      if (local.addr == 0)
        {
          if(TUA_DEBUG)
            cout << "atmaxUReal: REQUEST CANCEL(1)" << endl;
          return CANCEL;
        }
      sli = (AtExtrURealLocalInfo*) local.addr;
      if(TUA_DEBUG)
        cout << " 1" << endl;
      if (sli->NoOfResults <= sli->ResultsDelivered)
        {
          if(TUA_DEBUG)
            cout << "atmaxUReal: REQUEST CANCEL (2)" << endl;
          return CANCEL;
        }
      if(TUA_DEBUG)
        cout << " 2" << endl;
      result = SetWord( sli->t_res[sli->ResultsDelivered]->Copy() );
      if(TUA_DEBUG)
        cout << " 3" << endl;
      sli->t_res[sli->ResultsDelivered]->DeleteIfAllowed();
      if(TUA_DEBUG)
        cout << " 4: delivered result[" << sli->ResultsDelivered+1
             << "/" << sli->NoOfResults<< "]="
             << TUPrintUReal((UReal*)(result.addr))
             << endl;
      sli->ResultsDelivered++;
      if(TUA_DEBUG)
        cout << "atmaxUReal: REQUEST YIELD" << endl;
      return YIELD;

    case CLOSE :

      if(TUA_DEBUG) cout << "\natmaxUReal: CLOSE" << endl;
      if (local.addr != 0)
        {
          sli = (AtExtrURealLocalInfo*) local.addr;
          while (sli->NoOfResults > sli->ResultsDelivered)
            {
              sli->t_res[sli->ResultsDelivered]->DeleteIfAllowed();
              sli->ResultsDelivered++;
            }
          delete sli;
        }
      if(TUA_DEBUG) cout << "atmaxUReal: CLOSE finished" << endl;
      return 0;

    } // end switch
  cout << "\natmaxUReal (UNKNOWN COMMAND): This should not happen!" << endl;
  return 0;   // should not be reached
}

template<class T>
int atmaxUConst( Word* args, Word& result, int message,
                 Word& local, Supplier s )
{
  // This operator is not very interesting. It implements
  // the atmax operator for constant unit types, like uint, ustring or ubool.
  // In fact, it returns just a copy of the argument.

  result = SetWord(((ConstTemporalUnit<T>*)(args[0].addr))->Clone());
  return 0;
}

/*
5.22.3 Specification for operator ~atmax~

*/

const string TemporalSpecAtmax =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" "
  "\"Example\" ) "
  "("
  "<text>For T in {int, bool, string}:\n"
  "uT    -> uT\n"
  "ureal -> (stream ureal)</text--->"
  "<text>atmax( _ )</text--->"
  "<text>Restricts a the unittype value to the time where "
  "it takes it's maximum value.\n"
  "Observe, that for type 'ureal', the result is a '(stream ureal)' "
  "rather than a 'ureal'!</text--->"
  "<text>atmax( ureal1 )</text--->"
  ") )";


/*
5.22.4 Selection Function of operator ~atmax~

*/

ValueMapping temporalunitatmaxmap[] =
  {
    atmaxUConst<CcBool>,
    atmaxUConst<CcInt>,
    atmaxUConst<CcString>,
    atmaxUReal
  };

int temporalunitAtmaxSelect( ListExpr args )
{
  ListExpr arg1;
  if( nl->ListLength( args ) == 1 )
    {
      arg1 = nl->First( args );

      if( nl->IsEqual( arg1, "ubool" ) )
        return 0;
      if( nl->IsEqual( arg1, "uint" ) )
        return 1;
      if( nl->IsEqual( arg1, "ustring" ) )
        return 2;
      if( nl->IsEqual( arg1, "ureal" ) )
        return 3;
    }
  cout << "\ntemporalunitAtmaxSelect: Wrong type!" << endl;
  return -1;
}

/*
5.22.5 Definition of operator ~atmax~

*/

Operator temporalunitatmax( "atmax",
                            TemporalSpecAtmax,
                            4,
                            temporalunitatmaxmap,
                            temporalunitAtmaxSelect,
                            UnitBaseTypeMapAtmax);


/*
5.23 Operator ~atmin~

From a given unit ~u~, the operator creates a stream of units that
are restrictions of ~u~ to the periods where it reaches its minimum
value.

----
       For T in {int, real}
       atmin: uT --> (stream uT)

----

5.23.1 Type mapping function for ~atmin~

Uses typemapping ~UnitBaseTypeMapAtmax~ intended for related operator ~atmax~

*/

/*
5.23.2 Value mapping for operator ~atmin~

*/

int getMinValIndex( double& first, double& second, double& third)
{ // returns a 3-bit field indicating smallest values

  if ( (first < second) && (first < third) )
    return 1;
  if ( (second < first) && (second < third) )
    return 2;
  if ( (first == second) && (second < third) )
    return 3;
  if ( (third < first) && (third < second) )
    return 4;
  if ( (first == third) && (first < second) )
    return 5;
  if ( (second == third) && (second < first) )
    return 6;
  return 7; // they are all equal
}

int atminUReal( Word* args, Word& result, int message,
                Word& local, Supplier s )
{
  AtExtrURealLocalInfo *sli;
  UReal                *ureal;
  double  t_start, t_end, t_extr; // instants of interest
  double  v_start, v_end, v_extr; // values at resp. instants
  double  a, b, c, r, tx;
  int     minValIndex;
  Instant t = DateTime(instanttype);
  Word    a0;

  result = qp->ResultStorage( s );

  switch (message)
    {
    case OPEN :

      a0 = args[0];
      ureal = (UReal*)(a0.addr);
      if(TUA_DEBUG)
        cout << "  Argument ureal value: " << TUPrintUReal(ureal) << endl
             << "  1" << endl;

      sli = new AtExtrURealLocalInfo;
      sli->NoOfResults = 0;
      sli->ResultsDelivered = 0;
      local = SetWord(sli);

      if ( !ureal->IsDefined() )
        { // ureal undefined
          // -> return empty stream
          sli->NoOfResults = 0;
          if(TUA_DEBUG) cout << "       ureal undef: no solution" << endl;
          return 0;
        }

      if ( (ureal->timeInterval.start).ToDouble() ==
           (ureal->timeInterval.end).ToDouble() )
        { // ureal contains only a single point.
          // -> return a copy of the ureal
          sli->t_res[sli->NoOfResults] = (UReal*) (ureal->Copy());
          // no translation required
          if(TUA_DEBUG)
            cout << "       single point" << endl
                 << "       res " << sli->NoOfResults+1 << "="
                 << TUPrintUReal(sli->t_res[sli->NoOfResults])
                 << endl;
          sli->NoOfResults++;
          return 0;
        }

      if (ureal->a == 0)
        {
          if ( ureal->b == 0 )
            { //  constant function
              // the only result is a copy of the argument ureal
              sli->t_res[sli->NoOfResults] = (UReal*) (ureal->Copy());
              // no translation required
              if(TUA_DEBUG)
                cout << "       constant function" << endl
                     << "       res " << sli->NoOfResults+1 << "="
                     << TUPrintUReal(sli->t_res[sli->NoOfResults])
                     << endl;
              sli->NoOfResults++;
              return 0;
            }
          if ( ureal->b < 0 )
            { // linear fuction
              // the result is a clone of the argument, restricted to
              // its ending instant
              sli->t_res[sli->NoOfResults] = ureal->Clone();
              sli->t_res[sli->NoOfResults]->timeInterval.end =
                sli->t_res[sli->NoOfResults]->timeInterval.start;
              sli->t_res[sli->NoOfResults]->timeInterval.lc = true;
              // translate result to new starting instant!
              tx = ureal->timeInterval.end.ToDouble() -
                   ureal->timeInterval.start.ToDouble();
              (sli->t_res[sli->NoOfResults])->TranslateParab(tx);
              if(TUA_DEBUG)
                cout << "       linear function: final" << endl
                     << "       res " << sli->NoOfResults+1 << "="
                     << TUPrintUReal(sli->t_res[sli->NoOfResults])
                     << endl;
              sli->NoOfResults++;
              return 0;
            }
          if ( ureal->b > 0 )
            { // linear fuction
              // the result is a clone of the argument, restricted to
              // its starting instant
              sli->t_res[sli->NoOfResults] = ureal->Clone();
              sli->t_res[sli->NoOfResults]->timeInterval.start =
                sli->t_res[sli->NoOfResults]->timeInterval.end;
              sli->t_res[sli->NoOfResults]->timeInterval.rc = true;
              // no translation required
              if(TUA_DEBUG)
                cout << "       linear function: initial" << endl
                     << "       res " << sli->NoOfResults+1 << "="
                     << TUPrintUReal(sli->t_res[sli->NoOfResults])
                     << endl;
              sli->NoOfResults++;
              return 0;
            }
        }

      if (ureal->a !=0)
        { // quadratic function
          // we have to additionally ckeck for the extremum

          // get the times of interest
          a = ureal->a;
          b = ureal->b;
          c = ureal->c;
          r = ureal->r;
          t_extr  = -b/(2*a);
          t_start = (ureal->timeInterval.start).ToDouble();
          t_end   = (ureal->timeInterval.end).ToDouble();
          // get the values of interest
          v_extr  = getValUreal(t_extr, a,b,c,r);
          v_start = getValUreal(t_start,a,b,c,r);
          v_end   = getValUreal(t_end,  a,b,c,r);
          if(TUA_DEBUG)
            cout << "\nQuadratic function. Cadidates are: " << endl
                 << "\tt_start=" << t_start << "\t v_start=" << v_start << endl
                 << "\tt_extr =" << t_extr  << "\t v_extr =" << v_extr  << endl
                 << "\tt_end  =" << t_end   << "\t v_end  =" << v_end   << endl;
          // compute, which values are minimal

          if ( (t_start < t_extr) && (t_end > t_extr) )
            // 3 possible results
            minValIndex = getMinValIndex(v_extr,v_start,v_end);
          else
            { // only 2 possible results
              minValIndex = 0;
              if (v_start <= v_end) // min at start
                minValIndex += 2;
              if (v_end <= v_start) // min at end
                minValIndex += 4;
            }
          if(TUA_DEBUG) cout << "\tminValIndex = " <<  minValIndex << endl;

          if (minValIndex & 2)
            {
              sli->t_res[sli->NoOfResults] = ureal->Clone();
              t = ureal->timeInterval.start;
              Interval<Instant> i( t, t, true, true );
              sli->t_res[sli->NoOfResults]->timeInterval = i;
              // no translation required
              if(TUA_DEBUG)
                cout << "       added start" << endl
                     << "       res " << sli->NoOfResults+1 << "="
                     << TUPrintUReal(sli->t_res[sli->NoOfResults])
                     << endl;
              sli->NoOfResults++;
            }
          if ( (minValIndex & 4) && (t_start != t_end) )
            {
              sli->t_res[sli->NoOfResults] = ureal->Clone();
              t = ureal->timeInterval.end;
              Interval<Instant> i( t, t, true, true );
              sli->t_res[sli->NoOfResults]->timeInterval = i;
              // translate result to new starting instant!
              tx = ureal->timeInterval.end.ToDouble() -
                   ureal->timeInterval.start.ToDouble();
              (sli->t_res[sli->NoOfResults])->TranslateParab(tx);
              if(TUA_DEBUG)
                cout << "       added end" << endl
                     << "       res " << sli->NoOfResults+1 << "="
                     << TUPrintUReal(sli->t_res[sli->NoOfResults])
                     << endl;
              sli->NoOfResults++;
            }
          if ( (minValIndex & 1)   &&
               (t_extr != t_start) &&
               (t_extr != t_end)      )
            {
              sli->t_res[sli->NoOfResults] = ureal->Clone();
              t.ReadFrom(t_extr);
              Interval<Instant> i( t, t, true, true );
              sli->t_res[sli->NoOfResults]->timeInterval = i;
              // translate result to new starting instant!
              tx = t_extr - ureal->timeInterval.start.ToDouble();
              (sli->t_res[sli->NoOfResults])->TranslateParab(tx);
              if(TUA_DEBUG)
                cout << "       added extr" << endl
                     << "       res " << sli->NoOfResults+1 << "="
                     << TUPrintUReal(sli->t_res[sli->NoOfResults])
                     << endl;
              sli->NoOfResults++;
            }
          return 0;
        }
      cout << "\natminUReal (OPEN): This should not happen!" << endl;
      return 0;

    case REQUEST :

      if (local.addr == 0)
        return CANCEL;
      sli = (AtExtrURealLocalInfo*) local.addr;

      if (sli->NoOfResults <= sli->ResultsDelivered)
        return CANCEL;

      result = SetWord( sli->t_res[sli->ResultsDelivered]->Clone() );
      sli->t_res[sli->ResultsDelivered]->DeleteIfAllowed();
      if(TUA_DEBUG)
        cout << "    delivered result[" << sli->ResultsDelivered+1
             << "/" << sli->NoOfResults<< "]="
             << TUPrintUReal((UReal*)(result.addr))
             << endl;
      sli->ResultsDelivered++;
      return YIELD;

    case CLOSE :

      if (local.addr != 0)
        {
          sli = (AtExtrURealLocalInfo*) local.addr;
          while (sli->NoOfResults > sli->ResultsDelivered)
            {
              sli->t_res[sli->ResultsDelivered]->DeleteIfAllowed();
              sli->ResultsDelivered++;
            }
          delete sli;
        }
      return 0;

    } // end switch
  return 0;   // should not be reached
}

template<class T>
int atminUConst( Word* args, Word& result, int message,
                 Word& local, Supplier s )
{
  // This operator is not very interesting. It implements
  // the atmin operator for constant unit types, like uint, ustring or ubool.
  // In fact, it returns just a copy of the argument.

  result = SetWord(((ConstTemporalUnit<T>*)(args[0].addr))->Clone());
  return 0;
}

/*
5.23.3 Specification for operator ~atmin~

*/

const string TemporalSpecAtmin =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" "
  "\"Example\" ) "
  "("
  "<text>For T in {int, bool, string}:\n"
  "uT    -> uT\n"
  "ureal -> (stream ureal)</text--->"
  "<text>atmin( _ )</text--->"
  "<text>Restricts a the unittype value to the time where "
  "it takes it's minimum value.\n"
  "Observe, that for type 'ureal', the result is a '(stream ureal)' "
  "rather than a 'ureal'!</text--->"
  "<text>atmin( ureal1 )</text--->"
  ") )";


/*
5.23.4 Selection Function of operator ~atmin~

Uses selection function ~temporalunitAtmaxSelect~.

*/

ValueMapping temporalunitatminmap[] =
  {
    atminUConst<CcBool>,
    atminUConst<CcInt>,
    atminUConst<CcString>,
    atminUReal
  };

/*
  5.23.5 Definition of operator ~atmin~

*/
Operator temporalunitatmin( "atmin",
                            TemporalSpecAtmin,
                            4,
                            temporalunitatminmap,
                            temporalunitAtmaxSelect,
                            UnitBaseTypeMapAtmax);


/*
5.24 Operator ~saggregate~

Moved to ~StreamAlgebra~ and remaned it to ~aggregateS~

*/

/*
5.25 Operator ~~

*/

/*
5.25.1 Type mapping function for ~~

*/

/*
5.25.2 Value mapping for operator ~~

*/

/*
5.24.3 Specification for operator ~s~

*/

/*
5.25.4 Selection Function of operator ~~

*/

/*
5.25.5 Definition of operator ~~

*/


/*
5.26 Operator

----

     intersection: For T in {bool, int, string}:
OK             uT x      uT --> (stream uT)
OK             uT x       T --> (stream uT)
OK              T x      uT --> (stream uT)
(OK)        ureal x    real --> (stream ureal)
(OK)         real x   ureal --> (stream ureal)
Pre         ureal x   ureal --> (stream ureal)
OK         upoint x   point --> (stream upoint) same as at: upoint x point
OK          point x  upoint --> (stream upoint) same as at: upoint x point
OK         upoint x  upoint --> (stream upoint)
OK         upoint x    line --> (stream upoint)
OK           line x  upoint --> (stream upoint)
Pre        upoint x  region --> (stream upoint)
Pre        region x  upoint --> (stream upoint)
Pre        upoint x uregion --> (stream upoint)
Pre       uregion x  upoint --> (stream upoint)

----

A. (T mT) [->] mT   and   (mT T) -> mT

B. (mT mT) [->] mT

The operator always returns a stream of units (to handle both, empty and set
valued results).

  1. For all types of arguments, we return an empty stream, if both timeIntervals
don't overlap.

  2. Now, the algorithms depends on the actual dataype:

Then, for ~constant temporal units~ (ubool, uint, ustring), the operator
restricts two unit values with equal values to the intersection of both time
intervals, or otherwise (if the const values are nit equal) returns an empty
stream.

For ~ureal~, we test whether the functions of both units are equal, for we
can pass its restriction to the intersection of deftimes then (resp. create an
empty stream). Otherwise, we restrict both arguments' deftimes to their
intersection. Then we calculate the minimum and maximum value for both
arguments with respect to the restricted deftime. If both ranges don't
overlap, we return an empty stream. Otherwise, we compute the intersection
points (1 or 2) and return them as the result - or
an UNDEFINED value, if the result is not represeantable as a ureal value.

For ~upoint~, we interpret the units as straight lines. If both are parallel,
return the empty stream. If both are identical,

Otherwise calculate the intersection point ~S~. From ~S~, calculate the
instants $t_{u1}$ and $t_{u2}$. If both coincide, return that point if within
the common timeInterval, otherwise the empty stream.

C. (upoint line) [->] (stream upoint)  and  (line upoint) [->] (stream upoint)


*/

/*
5.26.1 Type mapping function for ~intersection~

*/

ListExpr TemporalUnitIntersectionTypeMap( ListExpr args )
{
  ListExpr arg1, arg2;
  string argstr1, argstr2;

  if( nl->ListLength( args ) == 2 )
    {
      arg1 = nl->First( args );
      arg2 = nl->Second( args );

      // First case: uT uT -> stream uT
      if (nl->Equal( arg1, arg2 ))
        {
          if( nl->IsEqual( arg1, "ubool" ) )
            return  nl->TwoElemList(nl->SymbolAtom( "stream" ),
                                    nl->SymbolAtom( "ubool" ));
          if( nl->IsEqual( arg1, "uint" ) )
            return nl->TwoElemList(nl->SymbolAtom( "stream" ),
                                   nl->SymbolAtom( "uint" ));
          if( nl->IsEqual( arg1, "ureal" ) )
            return nl->TwoElemList(nl->SymbolAtom( "stream" ),
                                   nl->SymbolAtom( "ureal" ));
          if( nl->IsEqual( arg1, "upoint" ) )
            return nl->TwoElemList(nl->SymbolAtom( "stream" ),
                                   nl->SymbolAtom( "upoint" ));
          if( nl->IsEqual( arg1, "ustring" ) )
            return nl->TwoElemList(nl->SymbolAtom( "stream" ),
                                   nl->SymbolAtom( "ustring" ));
        }

      // Second case: uT T -> stream uT
      if( nl->IsEqual( arg1, "ubool" ) && nl->IsEqual( arg2, "bool") )
        return  nl->TwoElemList(nl->SymbolAtom( "stream" ),
                                nl->SymbolAtom( "ubool" ));
      if( nl->IsEqual( arg1, "uint" ) && nl->IsEqual( arg2, "int") )
        return nl->TwoElemList(nl->SymbolAtom( "stream" ),
                               nl->SymbolAtom( "uint" ));
      if( nl->IsEqual( arg1, "ureal" ) && nl->IsEqual( arg2, "real") )
        return nl->TwoElemList(nl->SymbolAtom( "stream" ),
                               nl->SymbolAtom( "ureal" ));
      if( nl->IsEqual( arg1, "upoint" ) && nl->IsEqual( arg2, "point") )
        return nl->TwoElemList(nl->SymbolAtom( "stream" ),
                               nl->SymbolAtom( "upoint" ));
      if( nl->IsEqual( arg1, "ustring" ) && nl->IsEqual( arg2, "string") )
        return nl->TwoElemList(nl->SymbolAtom( "stream" ),
                               nl->SymbolAtom( "ustring" ));

      // Third case: T uT -> stream uT
      if( nl->IsEqual( arg1, "bool" ) && nl->IsEqual( arg2, "ubool") )
        return  nl->TwoElemList(nl->SymbolAtom( "stream" ),
                                nl->SymbolAtom( "ubool" ));
      if( nl->IsEqual( arg1, "int" ) && nl->IsEqual( arg2, "uint") )
        return nl->TwoElemList(nl->SymbolAtom( "stream" ),
                               nl->SymbolAtom( "uint" ));
      if( nl->IsEqual( arg1, "real" ) && nl->IsEqual( arg2, "ureal") )
        return nl->TwoElemList(nl->SymbolAtom( "stream" ),
                               nl->SymbolAtom( "ureal" ));
      if( nl->IsEqual( arg1, "point" ) && nl->IsEqual( arg2, "upoint") )
        return nl->TwoElemList(nl->SymbolAtom( "stream" ),
                               nl->SymbolAtom( "upoint" ));
      if( nl->IsEqual( arg1, "string" ) && nl->IsEqual( arg2, "ustring") )
        return nl->TwoElemList(nl->SymbolAtom( "stream" ),
                               nl->SymbolAtom( "ustring" ));

      // Fourth case: upoint line -> stream upoint
      if( nl->IsEqual( arg1, "upoint" ) && nl->IsEqual( arg2, "line") )
        return  nl->TwoElemList(nl->SymbolAtom( "stream" ),
                                nl->SymbolAtom( "upoint" ));

      // Fifth case: line upoint -> stream upoint
      if( nl->IsEqual( arg1, "line" ) && nl->IsEqual( arg2, "upoint") )
        return  nl->TwoElemList(nl->SymbolAtom( "stream" ),
                                nl->SymbolAtom( "upoint" ));

      // Sixth case: upoint uregion -> stream upoint
      if( nl->IsEqual( arg1, "upoint" ) && nl->IsEqual( arg2, "uregion") )
        return  nl->TwoElemList(nl->SymbolAtom( "stream" ),
                                nl->SymbolAtom( "upoint" ));

      // Eighth case: uregion upoint -> stream upoint
      if( nl->IsEqual( arg1, "uregion" ) && nl->IsEqual( arg2, "upoint") )
        return  nl->TwoElemList(nl->SymbolAtom( "stream" ),
                                nl->SymbolAtom( "upoint" ));

      // Ninth case: upoint region -> stream upoint
      //if( nl->IsEqual( arg1, "upoint" ) && nl->IsEqual( arg2, "region") )
      //  return  nl->TwoElemList(nl->SymbolAtom( "stream" ),
      //                          nl->SymbolAtom( "upoint" ));

      // Tenth case: region upoint -> stream upoint
      //if( nl->IsEqual( arg1, "region" ) && nl->IsEqual( arg2, "upoint") )
      //  return  nl->TwoElemList(nl->SymbolAtom( "stream" ),
      //                          nl->SymbolAtom( "upoint" ));
    }

  // Error case:
  nl->WriteToString(argstr1, arg1);
  nl->WriteToString(argstr2, arg2);
  ErrorReporter::ReportError(
    "Operator intersection expects two arguments of the same type T, "
    "where T in {ubool, uint, ureal, ustring, upoint}"
    "The passed arguments have types '"+ argstr1 +"' and '"
    + argstr2 + "'.");
  return nl->SymbolAtom("typeerror");
}

/*
5.26.2 Value mapping for operator ~intersection~

*/

struct TUIntersectionLocalInfo
{
  bool finished;
  int  NoOfResults;
  int  NoOfResultsDelivered;
  Word resultValues[2];       // Used if at most 2 results can occur
  vector<Word> resultValues2; // Used if more than 2 results may occur
  MPoint *mpoint;             // Used for upoint x lines
};

// value mapping for constant units (uT uT) -> (stream uT)
template<class T>
int temporalUnitIntersection_CU_CU( Word* args, Word& result, int message,
                                    Word& local, Supplier s )
{
  TUIntersectionLocalInfo *sli;
  Word u1, u2;
  T    *uv1, *uv2;
  Interval<Instant> iv;

  // test for overlapping intervals
  switch( message )
    {
    case OPEN:

      if (TUA_DEBUG)
        cout << "temporalUnitIntersection_CU_CU: received OPEN" << endl;
      sli = new TUIntersectionLocalInfo;
      sli->finished = true;
      sli->NoOfResults = 0;
      sli->NoOfResultsDelivered = 0;

      u1 = args[0];
      u2 = args[1];

      uv1 = (T*) (u1.addr);
      uv2 = (T*) (u2.addr);

      if ( uv1->IsDefined() &&
           uv2->IsDefined() &&
           uv1->timeInterval.Intersects( uv2->timeInterval ) &&
           uv1->EqualValue(*uv2) )
        { // get intersection of deftime intervals
          uv1->timeInterval.Intersection( uv2->timeInterval, iv );
          if (TUA_DEBUG) cout << "  iv=" << TUPrintTimeInterval( iv ) << endl;
          // store result
          sli->resultValues[sli->NoOfResults] = SetWord( uv1->Clone() );
          ((T*)(sli->resultValues[sli->NoOfResults].addr))->timeInterval = iv;
          sli->NoOfResults++;
          sli->finished = false;
          if (TUA_DEBUG)
            cout << "  added result" << endl;
        }// else: no result
      local = SetWord(sli);
      if (TUA_DEBUG)
        cout << "temporalUnitIntersection_CU_CU: finished OPEN" << endl;

      return 0;

    case REQUEST:

      if (TUA_DEBUG)
        cout << "temporalUnitIntersection_CU_CU: received REQUEST" << endl;
      if(local.addr == 0)
        {
          if (TUA_DEBUG)
            cout << "temporalUnitIntersection_CU_CU: CANCEL (1)" << endl;
          return CANCEL;
        }
      sli = (TUIntersectionLocalInfo*) local.addr;
      if(sli->finished)
        {
          if (TUA_DEBUG)
            cout << "temporalUnitIntersection_CU_CU: CANCEL (2)" << endl;
          return CANCEL;
        }
      if(sli->NoOfResultsDelivered < sli->NoOfResults)
        {
          result = SetWord( ((T*)
            (sli->resultValues[sli->NoOfResultsDelivered].addr))->Clone() );
          ((T*)(sli->resultValues[sli->NoOfResultsDelivered].addr))
            ->DeleteIfAllowed();
          sli->NoOfResultsDelivered++;
          if (TUA_DEBUG)
            cout << "temporalUnitIntersection_CU_CU: YIELD" << endl;
          return YIELD;
        }
      sli->finished = true;
      if (TUA_DEBUG)
        cout << "temporalUnitIntersection_CU_CU: CANCEL (3)" << endl;
      return CANCEL;

    case CLOSE:

      if (local.addr != 0)
        {
          sli = (TUIntersectionLocalInfo*) local.addr;
          while(sli->NoOfResultsDelivered < sli->NoOfResults)
            {
              ((T*)(sli->resultValues[sli->NoOfResultsDelivered].addr))
                ->DeleteIfAllowed();
              sli->NoOfResultsDelivered++;
            }
          delete sli;
        }
      return 0;
    } // end switch

  return 0;
}

// value mapping for constant units (uT  T) -> (stream uT)
//                            and   ( T uT) -> (stream uT)
template<class UT, class T, int uargindex>
int temporalUnitIntersection_CU_C( Word* args, Word& result, int message,
                                   Word& local, Supplier s )
{
  TUIntersectionLocalInfo *sli;
  Word u1, u2;
  UT    *uv1;
  T     *uv2;
  Interval<Instant> iv;

  // test for overlapping intervals
  switch( message )
    {
    case OPEN:

      if (TUA_DEBUG)
        cout << "temporalUnitIntersection_CU_C: received OPEN" << endl;
      sli = new TUIntersectionLocalInfo;
      sli->finished = true;
      sli->NoOfResults = 0;
      sli->NoOfResultsDelivered = 0;

      // get arguments, such that u1 is the unit type
      //                and u2 is the simple type
      if (uargindex == 0)
        { u1 = args[0]; u2 = args[1]; }
      else
        { u1 = args[1]; u2 = args[0];}

      if (TUA_DEBUG)
        cout << "  uargindex =" << uargindex << endl;
      uv1 = (UT*) (u1.addr);
      uv2 = (T*) (u2.addr);

      if ( uv1->IsDefined() &&
           uv2->IsDefined() &&
           (uv1->constValue.Compare( uv2 ) == 0 ) )
        { // store result
          sli->resultValues[sli->NoOfResults] = SetWord( uv1->Clone() );
          sli->NoOfResults++;
          sli->finished = false;
          if (TUA_DEBUG) cout << "  Added Result" << endl;
        }// else: no result
      local = SetWord(sli);
      if (TUA_DEBUG)
        cout << "temporalUnitIntersection_CU_C: finished OPEN" << endl;
      return 0;

    case REQUEST:

      if (TUA_DEBUG)
        cout << "temporalUnitIntersection_CU_C: received REQUEST" << endl;
      if(local.addr == 0)
        {
          if (TUA_DEBUG)
            cout << "temporalUnitIntersection_CU_C: finished REQUEST: "
                 << "CANCEL (1)" << endl;
          return CANCEL;
        }
      sli = (TUIntersectionLocalInfo*) local.addr;
      if(sli->finished)
        {
          if (TUA_DEBUG)
            cout << "temporalUnitIntersection_CU_C: finished REQUEST: "
                 << "CANCEL (2)" << endl;
          return CANCEL;
        }
      if(sli->NoOfResultsDelivered < sli->NoOfResults)
        {
          result = SetWord( ((UT*)
            (sli->resultValues[sli->NoOfResultsDelivered].addr))->Clone() );
          ((UT*)(sli->resultValues[sli->NoOfResultsDelivered].addr))
            ->DeleteIfAllowed();
          sli->NoOfResultsDelivered++;
          if (TUA_DEBUG)
            cout << "temporalUnitIntersection_CU_C: finished REQUEST: "
                 << "YIELD" << endl;
          return YIELD;
        }
      sli->finished = true;
      if (TUA_DEBUG)
        cout << "temporalUnitIntersection_CU_C: finished REQUEST: "
             << "CANCEL (3)" << endl;
      return CANCEL;

    case CLOSE:

      if (local.addr != 0)
        {
          sli = (TUIntersectionLocalInfo*) local.addr;
          while(sli->NoOfResultsDelivered < sli->NoOfResults)
            {
              ((UT*)(sli->resultValues[sli->NoOfResultsDelivered].addr))
                ->DeleteIfAllowed();
              sli->NoOfResultsDelivered++;
            }
          delete sli;
        }
      return 0;
    } // end switch

  return 0;

}


/*

Value mapping for ureal

We will calculate the intersection points (time instants) of both ureal
functions. Therefore, we need to distinguish 2 different cases:

 1. $ureal_1.r = ureal_2.r$

  Then, we have to solve the equation
 \[ (a_1-a_2)t^2 + (b_1-b_2)t + (c_1-c_2) = 0 \]
With $a\neq 0$, this computes to
\[ t_{1,2} = \frac{-(b_1-b_2) \pm \sqrt{(b_1-b_2)^2 - 4(a_1-a_2)(c_1-c_2)}}
{2(a_1-a_2)} \]

And depending on the value of $D = (b_1-b_2)^2 - 4(a_1-a_2)(c_1-c_2)$, we will
have 0 ($D<0$), 1 ($D=0$) or 2 ($D>0$) solutions.


 2. $ureal_1.r \neq ureal2_.r$

Then we have to solve the equation
\[ {a_1}^2 t^4 + 2 a_1 b_1 t^3 + (2 a_1 c_1 + {b_1}^2 - a_2)
t^2 + (2 b_1 c_1 - b_2) t - c_2 = 0 \]

  where $x_1$ stands for parameters from the
~ureal~ value that does have $r = false$, $x_2$ for the parameters of the
~ureal~ value with $r = true$.

*/

int temporalUnitIntersection_ureal_ureal( Word* args, Word& result, int message,
                                          Word& local, Supplier s )
{
  cout << "\nATTENTION: temporalUnitIntersection_ureal_ureal "
       << "not yet implemented!" << endl;
  return 0;
}


// value mapping for constant units (uT  T) -> (stream uT)
//                            and   ( T uT) -> (stream uT)
// The method is almost identical to that used for operator
//                            at: (ureal real) -> (stream ureal)
template<int uargindex>
int temporalUnitIntersection_ureal_real( Word* args, Word& result, int message,
                                         Word& local, Supplier s )
{
  MappingUnitAt_rLocalInfo *localinfo;
  double radicand, a, b, c, r, y, t0;
  DateTime t1 = DateTime(instanttype);
  DateTime t2 = DateTime(instanttype);
  Interval<Instant> rdeftime, deftime;
  UReal *uinput;
  CcReal *value;
  Word a0, a1;

  switch (message)
    {
    case OPEN :

      if(TUA_DEBUG)
        cout << "\ntemporalUnitIntersection_ureal_real: OPEN" << endl;
      localinfo = new MappingUnitAt_rLocalInfo;
      localinfo->finished = true;
      localinfo->NoOfResults = 0;

      // initialize arguments, such that a0 always contains the ureal
      //                       and a1 the real
      if (uargindex == 0)
        { a0 = args[0]; a1 = args[1]; }
      else
        { a0 = args[1]; a1 = args[0]; }

      localinfo = new MappingUnitAt_rLocalInfo;
      localinfo->finished = true;
      localinfo->NoOfResults = 0;
      if(TUA_DEBUG) cout << "  1" << endl;

      uinput = (UReal*)(a0.addr);
      if(TUA_DEBUG) cout << "  1.1" << endl;

      value = (CcReal*)(a1.addr);
      if(TUA_DEBUG) cout << "  1.2" << endl;

      if(TUA_DEBUG) cout << "  2" << endl;

      if(TUA_DEBUG) cout << "  2.1: " << uinput->IsDefined() << endl;
      if(TUA_DEBUG) cout << "  2.2: " << value->IsDefined() << endl;

      if ( !uinput->IsDefined() ||
           !value->IsDefined() )
        { // some input is undefined -> return empty stream
          if(TUA_DEBUG) cout << "  3: Some input is undefined. No result."
                             << endl;
          localinfo->NoOfResults = 0;
          localinfo->finished = true;
          local = SetWord(localinfo);
          if(TUA_DEBUG)
            cout << "\ntemporalUnitIntersection_ureal_real: finished OPEN (1)"
                 << endl;
          return 0;
        }
      if(TUA_DEBUG) cout << "  4" << endl;

      y = value->GetRealval();
      a = uinput->a;
      b = uinput->b;
      c = uinput->c;
      r = uinput->r;
      deftime = uinput->timeInterval;
      t0 = deftime.start.ToDouble();

      if(TUA_DEBUG)
        {cout << "    The UReal is" << " a= " << a << " b= "
              << b << " c= " << c << " r= " << r << endl;
          cout << "    The Real is y=" << y << endl;
          cout << "  5" << endl;
        }

      if ( (a == 0) && (b == 0) )
        { // constant function. Possibly return input unit
          if(TUA_DEBUG) cout << "  6: 1st arg is a constant value" << endl;
          if (c != y)
            { // There will be no result, just an empty stream
              if(TUA_DEBUG) cout << "  7" << endl;
              localinfo->NoOfResults = 0;
              localinfo->finished = true;
            }
          else
                { // Return the complete unit
                  if(TUA_DEBUG)
                    {
                      cout << "  8: Found constant solution" << endl;
                      cout << "    T1=" << c << endl;
                      cout << "    Tstart=" << deftime.start.ToDouble() << endl;
                      cout << "    Tend  =" << deftime.end.ToDouble() << endl;
                    }
                  localinfo->runits[localinfo->NoOfResults].addr
                    = uinput->Copy();
                  // no translation required
                  localinfo->NoOfResults++;
                  localinfo->finished = false;
                  if(TUA_DEBUG) cout << "  9" << endl;
                }
          if(TUA_DEBUG) cout << "  10" << endl;
          local = SetWord(localinfo);
          if(TUA_DEBUG)
            cout << "\ntemporalUnitIntersection_ureal_real: finished OPEN (2)"
                 << endl;
          return 0;
        }
      if ( (a == 0) && (b != 0) )
        { // linear function. Possibly return input unit restricted
          // to single value
          if(TUA_DEBUG) cout << "  11: 1st arg is a linear function" << endl;
          double T1 = (y - c)/b + t0; // Add t0 due to representation
          if(TUA_DEBUG)
            {
              cout << "    T1=" << T1 << endl;
              cout << "    Tstart=" << deftime.start.ToDouble() << endl;
              cout << "    Tend  =" << deftime.end.ToDouble() << endl;
            }
          t1.ReadFrom( T1 );
          if (deftime.Contains(t1))
            { // value is contained by deftime
              if(TUA_DEBUG)
                cout << "  12: Found valid linear solution." << endl;
              localinfo->runits[localinfo->NoOfResults].addr =
                uinput->Copy();
              localinfo->runits[localinfo->NoOfResults].addr =
                new UReal( rdeftime,0.0,0.0,y,false );
              ((UReal*)(localinfo
                        ->runits[localinfo->NoOfResults].addr))
                ->timeInterval = Interval<Instant>(t1, t1, true, true);
              // No translation needed
              localinfo->NoOfResults++;
              localinfo->finished = false;
              if(TUA_DEBUG) cout << "  13" << endl;
            }
          else
            { // value is not contained by deftime -> no result
              if(TUA_DEBUG)
                cout << "  14: Found invalid linear solution." << endl;
              localinfo->NoOfResults = 0;
              localinfo->finished = true;
              if(TUA_DEBUG) cout << "  15" << endl;
            }
          if(TUA_DEBUG) cout << "  16" << endl;
          local = SetWord(localinfo);
          if(TUA_DEBUG)
            cout << "\ntemporalUnitIntersection_ureal_real: finished OPEN (3)"
                 << endl;
          return 0;
        }

      if(TUA_DEBUG) cout << "  17" << endl;
      radicand = (b*b + 4*a*(y-c));
      if(TUA_DEBUG) cout << "    radicand =" << radicand << endl;
      if ( (a != 0) && (radicand >= 0) )
        { // quadratic function. There are possibly two result units
          // calculate the possible t-values t1, t2

/*
The solution to the equation $at^2 + bt + c = y$ is
\[t_{1,2} = \frac{-b \pm \sqrt{b^2-4a(c-y)}}{2a},\] for $b^2-4a(c-y) = b^2+4a(y-c) \geq 0$.


*/
          if(TUA_DEBUG) cout << "  18: 1st arg is a quadratic function" << endl;
          double T1 = (-b + sqrt(radicand)) / (2*a) + t0; //Add t0 due to
          double T2 = (-b - sqrt(radicand)) / (2*a) + t0; //  representation
          if(TUA_DEBUG)
            {
              cout << "    T1=" << T1 << endl;
              cout << "    T2=" << T2 << endl;
              cout << "    Tstart=" << deftime.start.ToDouble() << endl;
              cout << "    Tend  =" << deftime.end.ToDouble() << endl;
            }
          t1.ReadFrom( T1 );
          t2.ReadFrom( T2 );

          // check, whether t1 contained by deftime
          if (deftime.Contains( t1 ))
            {
              if(TUA_DEBUG)
                cout << "  19: Found first quadratic solution" << endl;
              rdeftime.start = t1;
              rdeftime.end = t1;
              rdeftime.lc = true;
              rdeftime.rc = true;
              localinfo->runits[localinfo->NoOfResults].addr =
                new UReal( rdeftime,0.0,0.0,y,false );
              ((UReal*) (localinfo->runits[localinfo->NoOfResults].addr))
                ->SetDefined( true );
              // No translation needed
              localinfo->NoOfResults++;
              localinfo->finished = false;
              if(TUA_DEBUG) cout << "  20" << endl;
            }
          // check, whether t2 contained by deftime
          if ( !(t1 == t2) && (deftime.Contains( t2 )) )
            {
              if(TUA_DEBUG)
                cout << "  21: Found second quadratic solution" << endl;
              rdeftime.start = t2;
              rdeftime.end = t2;
              rdeftime.lc = true;
              rdeftime.rc = true;
              localinfo->runits[localinfo->NoOfResults].addr =
                new UReal( rdeftime,0.0,0.0,y,false );
              ((UReal*) (localinfo->runits[localinfo->NoOfResults].addr))
                ->SetDefined (true );
              // No translation needed
              localinfo->NoOfResults++;
              localinfo->finished = false;
              if(TUA_DEBUG) cout << "  22" << endl;
            }
        }
      else // negative discreminant -> there is no real solution
           //                          and no result unit
        {
          if(TUA_DEBUG) cout << "  23: No real-valued solution" << endl;
          localinfo->NoOfResults = 0;
          localinfo->finished = true;
          if(TUA_DEBUG) cout << "  24" << endl;
        }
      if(TUA_DEBUG) cout << "  25" << endl;
      local = SetWord(localinfo);
      if(TUA_DEBUG)
        cout << "\ntemporalUnitIntersection_ureal_real: finished OPEN (4)"
             << endl;
      return 0;

    case REQUEST :

      if(TUA_DEBUG) cout << "\ntemporalUnitIntersection_ureal_real: REQUEST"
                         << endl;
      if (local.addr == 0)
        {
          if(TUA_DEBUG)
            cout << "\ntemporalUnitIntersection_ureal_real: REQUEST CANCEL (1)"
                 << endl;
          return CANCEL;
        }
      localinfo = (MappingUnitAt_rLocalInfo*) local.addr;
      if(TUA_DEBUG) cout << "\n   localinfo: finished=" << localinfo->finished
                         << " NoOfResults==" << localinfo->NoOfResults << endl;

      if (localinfo->finished)
        {
          if(TUA_DEBUG)
            cout << "\ntemporalUnitIntersection_ureal_real: REQUEST CANCEL (2)"
                 << endl;
          return CANCEL;
        }
      if ( localinfo->NoOfResults <= 0 )
        { localinfo->finished = true;
          if(TUA_DEBUG)
            cout << "\ntemporalUnitIntersection_ureal_real: REQUEST CANCEL (3)"
                 << endl;
          return CANCEL;
        }
      localinfo->NoOfResults--;
      result = SetWord( ((UReal*)(localinfo
                                  ->runits[localinfo->NoOfResults].addr))
                        ->Clone() );
      ((UReal*)(localinfo->runits[localinfo->NoOfResults].addr))
        ->DeleteIfAllowed();
      if(TUA_DEBUG)
        cout << "\ntemporalUnitIntersection_ureal_real: REQUEST YIELD" << endl;
      return YIELD;

    case CLOSE :

      if(TUA_DEBUG) cout << "\ntemporalUnitIntersection_ureal_real: CLOSE"
                         << endl;
      if (local.addr != 0)
        {
          localinfo = (MappingUnitAt_rLocalInfo*) local.addr;
          for (;localinfo->NoOfResults>0;localinfo->NoOfResults--)
            ((UReal*)(localinfo->runits[localinfo->NoOfResults].addr))
              ->DeleteIfAllowed();
          delete localinfo;
        }
      if(TUA_DEBUG)
        cout << "\ntemporalUnitIntersection_ureal_real: finished CLOSE" << endl;
      return 0;
    } // end switch

      // should not be reached
  return 0;
}

/*

value mapping for

----
     (upoint upoint) -> (stream upoint)

----

*/
int
temporalUnitIntersection_upoint_upoint( Word* args, Word& result, int message,
                                        Word& local, Supplier s )
{
  TUIntersectionLocalInfo *sli;
  Word u1, u2;
  UPoint *uv1, *uv2;
  Interval<Instant> iv;
  Instant t;
  Point p1n_start, p1n_end, p2n_start, p2n_end, p_intersect, d1, d2, p1;
  UPoint p1norm(true), p2norm(true);
  double t_x, t_y, t1, t2, dxp1, dxp2, dyp1, dyp2, dt;

  // test for overlapping intervals
  switch( message )
    {
    case OPEN:

      if (TUA_DEBUG)
        cout << "temporalUnitIntersection_upoint_upoint: received OPEN" << endl;
      sli = new TUIntersectionLocalInfo;
      sli->finished = true;
      sli->NoOfResults = 0;
      sli->NoOfResultsDelivered = 0;
      local = SetWord(sli);
      u1 = args[0];
      u2 = args[1];
      uv1 = (UPoint*) (u1.addr);
      uv2 = (UPoint*) (u2.addr);

      if ( !uv1->IsDefined() ||
           !uv2->IsDefined() ||
           !uv1->timeInterval.Intersects( uv2->timeInterval ) )
        {
          if (TUA_DEBUG)
            cout << "temporalUnitIntersection_upoint_upoint: finished OPEN (1)"
                 << endl;
          return 0; // nothing to do
        }

      // get common time interval
      uv1->timeInterval.Intersection(uv2->timeInterval, iv);

      // normalize both starting and ending points to interval
      uv1->AtInterval(iv, p1norm);
      p1n_start = p1norm.p0;
      p1n_end   = p1norm.p1;

      uv2->AtInterval(iv, p2norm);
      p2n_start = p2norm.p0;
      p2n_end   = p2norm.p1;

      if (TUA_DEBUG)
        {
          cout << "  uv1=" << TUPrintUPoint( *uv1 ) << endl
               << "  uv2=" << TUPrintUPoint( *uv2 ) << endl
               << "  iv=" << TUPrintTimeInterval( iv ) << endl
               << "  p1norm=" << TUPrintUPoint( p1norm ) << endl
               << "  p2norm=" << TUPrintUPoint( p2norm ) << endl;
        }

      // test for identity:
      if ( p1norm.EqualValue( p2norm ))
        { // both upoints have the same linear function
          sli->resultValues[sli->NoOfResults] = SetWord( p1norm.Clone() );
          sli->NoOfResults++;
          sli->finished = false;
          if (TUA_DEBUG)
            cout << "  Identity test passed. Added result." << endl
                 << "temporalUnitIntersection_upoint_upoint: finished OPEN (3)"
                 << endl;
          return 0;
        }
      if (TUA_DEBUG)
        cout << "  Identity test failed." << endl;

      // test for ordinary intersection of the normalized upoints
      d1 = p2n_start - p1n_start;
      d2 = p2n_end   - p1n_end;
      if ( ((d1.GetX() > 0) && (d2.GetX() > 0)) ||
           ((d1.GetX() < 0) && (d2.GetX() < 0)) ||
           ((d1.GetY() > 0) && (d2.GetY() > 0)) ||
           ((d1.GetY() < 0) && (d2.GetY() < 0)))
        { // no intersection
          if (TUA_DEBUG)
            cout << "  No intersection. No Result." << endl
                 << "temporalUnitIntersection_upoint_upoint: finished OPEN (4)"
                 << endl;
          return 0; // nothing to do
        }
      if (TUA_DEBUG) cout << "  Some Intersection." << endl;

      dxp1 = (p1n_end - p1n_start).GetX();
      dyp1 = (p1n_end - p1n_start).GetY();
      dxp2 = (p2n_end - p2n_start).GetX();
      dyp2 = (p2n_end - p2n_start).GetY();

/*
Trying to find an intersection point $t$ with $A_1t + B_1 = A_2t + B_2$
we get:

\[ t_x = \frac{px_{21} - px_{11}}{dxp_1 - dxp_2} \quad
t_y = \frac{py_{21} - py_{11}}{dyp_1 - pyp_2} \]

where $t = t_x = t_y$. If $t_x \neq t_y$, then there is no intersection!

*/

      dt = (iv.end - iv.start).ToDouble();

      t1 = iv.start.ToDouble();
      t2 = iv.end.ToDouble();

      t_x = (dt*d1.GetX() + t1*(dxp1-dxp2)) / (dxp1-dxp2);
      t_y = (dt*d1.GetY() + t1*(dyp1-dyp2)) / (dyp1-dyp2);

      if (TUA_DEBUG)
        {
          cout << "  dt =" << dt  << endl
               << "  t_x=" << t_x << endl
               << "  t_y=" << t_y << endl
               << "  t1 =" << t1  << endl
               << "  t2 =" << t2  << endl;
        }

      if ( AlmostEqual(t_x, t_y) &&
           ( t_x >= t1) &&
           ( t_x <= t2) )
        { // We found an intersection
          t.ReadFrom(t_x); // create Instant
          t.SetType(instanttype);
          iv = Interval<Instant>( t, t, true, true ); // create Interval
          uv1->TemporalFunction(t, p1, true);
          sli->resultValues[sli->NoOfResults] =
            SetWord( new UPoint(iv, p1, p1) );
          ((UPoint*)(sli->resultValues[sli->NoOfResults].addr))
            ->timeInterval=iv;
          sli->NoOfResults++;
          sli->finished = false;
          if (TUA_DEBUG)
            cout << "  Found intersection. Added result." << endl;
        }
      // else: no result
      if (TUA_DEBUG)
        cout << "temporalUnitIntersection_upoint_upoint: finished OPEN (6)"
             << endl;
      return 0;

    case REQUEST:

      if (TUA_DEBUG)
        cout << "temporalUnitIntersection_upoint_upoint: received REQUEST"
             << endl;
      if(local.addr == 0)
        {
          if (TUA_DEBUG)
            cout << "temporalUnitIntersection_upoint_upoint: CANCEL (1)"
                 << endl;
          return CANCEL;
        }
      sli = (TUIntersectionLocalInfo*) local.addr;
      if(sli->finished)
        {
          if (TUA_DEBUG)
            cout << "temporalUnitIntersection_upoint_upoint: CANCEL (2)"
                 << endl;
          return CANCEL;
        }
      if (TUA_DEBUG)
        {
          cout << "  NoOfResults=" << sli->NoOfResults << endl
               << "  NoOfResultsDelivered=" << sli->NoOfResultsDelivered
               << endl;
        }
      if(sli->NoOfResultsDelivered < sli->NoOfResults)
        {
          result = SetWord( ((UPoint*)
             (sli->resultValues[sli->NoOfResultsDelivered].addr))->Clone() );
          ((UPoint*)(sli->resultValues[sli->NoOfResultsDelivered].addr))
            ->DeleteIfAllowed();
          sli->NoOfResultsDelivered++;
          if (TUA_DEBUG)
            cout << "temporalUnitIntersection_upoint_upoint: YIELD"
                 << endl;
          return YIELD;
        }
      sli->finished = true;
      if (TUA_DEBUG)
        cout << "temporalUnitIntersection_upoint_upoint: CANCEL (3)" << endl;
      return CANCEL;

    case CLOSE:

      if (local.addr != 0)
        {
          sli = (TUIntersectionLocalInfo*) local.addr;
          while(sli->NoOfResultsDelivered < sli->NoOfResults)
            {
              ((UPoint*)(sli->resultValues[sli->NoOfResultsDelivered].addr))
                ->DeleteIfAllowed();
              sli->NoOfResultsDelivered++;
            }
          delete sli;
        }
      return 0;
    } // end switch

  return 0;
}

/*
   value mapping for (upoint point) -> (stream upoint)
                 and (point upoint) -> (stream upoint)

   is identical with    at: (upoint point) -> (stream upoint).
   We just add switches for both signatures and the stream framework

*/

template<int uargindex>
int
temporalUnitIntersection_upoint_point( Word* args, Word& result, int message,
                                       Word& local, Supplier s )
{
  TUIntersectionLocalInfo *sli;
  Word a0, a1;
  UPoint *unit, pResult(true);
  Point *val;


  switch( message )
    {
    case OPEN:

      sli = new TUIntersectionLocalInfo;
      sli->finished = true;
      sli->NoOfResults = 0;
      sli->NoOfResultsDelivered = 0;
      local = SetWord(sli);

      if (uargindex == 0)
        { a0 = args[0]; a1 = args[1]; }
      else
        { a0 = args[1]; a1 = args[0]; }

      unit = ((UPoint*)a0.addr);
      val  = ((Point*) a1.addr);

      if ( !unit->IsDefined() || !val->IsDefined() )
        return 0;

      if (unit->At( *val, pResult ))
        {
          pResult.SetDefined(true);
          pResult.timeInterval.start.SetDefined(true);
          pResult.timeInterval.end.SetDefined(true);
          sli->resultValues[sli->NoOfResults] = SetWord( pResult.Clone() );
          sli->NoOfResults++;
          sli->finished = false;
        }

      return 0;

    case REQUEST:

      if(local.addr == 0)
        return CANCEL;
      sli = (TUIntersectionLocalInfo*) local.addr;
      if(sli->finished)
        return CANCEL;
      if(sli->NoOfResultsDelivered < sli->NoOfResults)
        {
          result = SetWord( ((UPoint*)
              (sli->resultValues[sli->NoOfResultsDelivered].addr))->Clone() );
          ((UPoint*)(sli->resultValues[sli->NoOfResultsDelivered].addr))
            ->DeleteIfAllowed();
          sli->NoOfResultsDelivered++;
          return YIELD;
        }
      sli->finished = true;
      return CANCEL;

    case CLOSE:

      if (local.addr != 0)
        {
          sli = (TUIntersectionLocalInfo*) local.addr;
          while(sli->NoOfResultsDelivered < sli->NoOfResults)
            {
              ((UPoint*)(sli->resultValues[sli->NoOfResultsDelivered].addr))
                ->DeleteIfAllowed();
              sli->NoOfResultsDelivered++;
            }
          delete sli;
        }
      return 0;
    } // end switch

  return 0;
}

/*
Value mapping for

----

  intersection:     (upoint line) -> (stream upoint)
                    (line upoint) -> (stream upoint)

----

Method ~TUUPointInsideLine~

Copied from TempralLiftedAlgebra

calcultates the periods where the given UPoint lies
inside the given Line. It returns the existing intervals in a Periods-Object.

*/
static void TUUPointInsideLine(UPoint *u, Line& ln, Periods& pResult)
{
  if(TUA_DEBUG)
    cout<<"MPointLineInside called"<<endl;
  const HalfSegment *l;

  const UPoint* up = (UPoint*) u;
  Periods* period = new Periods(0);
  Periods* between = new Periods(0);
  Point pt;
  Interval<Instant> newper; //part of the result

  pResult.Clear();

    if(TUA_DEBUG){
      cout<<"UPoint # "<<" ["<<up->timeInterval.start.ToString()<<" "
      <<up->timeInterval.end.ToString()<<" "<<up->timeInterval.lc<<" "
      <<up->timeInterval.rc<<"] ("<<up->p0.GetX()<<" "<<up->p0.GetY()
      <<")->("<<up->p1.GetX()<<" "<<up->p1.GetY()<<")"<<endl;}

    for( int n = 0; n < ln.Size(); n++)
    {
      Instant t;
      ln.Get(n, l);
      if(TUA_DEBUG){
        cout<<"UPoint # "<<" ["<<up->timeInterval.start.ToString()
        <<" "<<up->timeInterval.end.ToString()<<" "<<up->timeInterval.lc
        <<" "<<up->timeInterval.rc<<"] ("<<up->p0.GetX()<<" "<<up->p0.GetY()
        <<")->("<<up->p1.GetX()<<" "<<up->p1.GetY()<<")"<<endl;
        cout<<"l      # "<<n<<" ("<<l->GetLeftPoint().GetX()
        <<" "<<l->GetLeftPoint().GetY()
        <<" "<<l->GetRightPoint().GetX()<<" "
        <<l->GetRightPoint().GetY()<<") "<<endl;}
      if (l->GetRightPoint().GetX() == l->GetDomPoint().GetX()
       && l->GetRightPoint().GetY() == l->GetDomPoint().GetY()) {
        if(TUA_DEBUG)
          cout<<"right point is dominating -> continue"<<endl;
        continue;
      }
      if(( l->GetRightPoint().GetX() < up->p0.GetX()
       &&  l->GetRightPoint().GetX() < up->p1.GetX())
       || (l->GetLeftPoint().GetX() > up->p0.GetX()
       &&  l->GetLeftPoint().GetX() > up->p1.GetX())
       || (l->GetRightPoint().GetY() < up->p0.GetY()
       &&  l->GetRightPoint().GetY() < up->p1.GetY()
       && (l->GetLeftPoint().GetY() < up->p0.GetY()
       &&  l->GetLeftPoint().GetY() < up->p1.GetY()))
       || (l->GetRightPoint().GetY() > up->p0.GetY()
       &&  l->GetRightPoint().GetY() > up->p1.GetY()
       && (l->GetLeftPoint().GetY() > up->p0.GetY()
       &&  l->GetLeftPoint().GetY() > up->p1.GetY()))) {
        if(TUA_DEBUG)
          cout<<"Bounding Boxes not crossing!"<<endl;
        continue;
      }
      double al, bl, aup, bup;
      bool vl, vup;
      vl = l->GetRightPoint().GetX() == l->GetLeftPoint().GetX();
      if(!vl){
        al = (l->GetRightPoint().GetY() - l->GetLeftPoint().GetY())
           / (l->GetRightPoint().GetX() - l->GetLeftPoint().GetX());
        bl =  l->GetLeftPoint().GetY() - l->GetLeftPoint().GetX() * al;
          if(TUA_DEBUG)
            cout<<"al: "<<al<<" bl: "<<bl<<endl;
      }
      else
        if(TUA_DEBUG)
          cout<<"l is vertical"<<endl;
      vup = up->p1.GetX() == up->p0.GetX();
      if(!vup){
        aup = (up->p1.GetY() - up->p0.GetY())
            / (up->p1.GetX() - up->p0.GetX());
        bup =  up->p0.GetY() - up->p0.GetX() * aup;
        if(TUA_DEBUG)
          cout<<"aup: "<<aup<<" bup: "<<bup<<endl;
      }
      else
        if(TUA_DEBUG)
          cout<<"up is vertical"<<endl;
      if(vl && vup){
        if(TUA_DEBUG)
          cout<<"both elements are vertical!"<<endl;
        if(up->p1.GetX() != l->GetLeftPoint().GetX()){
        if(TUA_DEBUG)
          cout<<"elements are vertical but not at same line"<<endl;
          continue;
        }
        else {
          if(TUA_DEBUG)
            cout<<"elements on same line"<<endl;
          if(up->p1.GetY() < l->GetLeftPoint().GetY()
           && up->p0.GetY() < l->GetLeftPoint().GetY()){
            if(TUA_DEBUG)
              cout<<"uPoint lower as linesegment"<<endl;
            continue;
          }
          else if(up->p1.GetY() > l->GetRightPoint().GetY()
           && up->p0.GetY() > l->GetRightPoint().GetY()){
            if(TUA_DEBUG)
              cout<<"uPoint higher as linesegment"<<endl;
            continue;
          }
          else{
            if(TUA_DEBUG)
              cout<<"uPoint and linesegment partequal"<<endl;
            if (up->p0.GetY() <= l->GetLeftPoint().GetY()
             && up->p1.GetY() >= l->GetLeftPoint().GetY()){
              if(TUA_DEBUG)
                cout<<"uPoint starts below linesegemet"<<endl;
              t.ReadFrom((l->GetLeftPoint().GetY() - up->p0.GetY())
                     / (up->p1.GetY() - up->p0.GetY())
                     * (up->timeInterval.end.ToDouble()
                     -  up->timeInterval.start.ToDouble())
                     +  up->timeInterval.start.ToDouble());
              t.SetType(instanttype);
              if(TUA_DEBUG)
                cout<<"t "<<t.ToString()<<endl;
              newper.start = t;
              newper.lc = (up->timeInterval.start == t)
                         ? up->timeInterval.lc : true;
            }
            if(up->p1.GetY() <= l->GetLeftPoint().GetY()
             && up->p0.GetY() >= l->GetLeftPoint().GetY()){
              if(TUA_DEBUG)
                cout<<"uPoint ends below linesegemet"<<endl;
              t.ReadFrom((l->GetLeftPoint().GetY() - up->p0.GetY())
                      / (up->p1.GetY() - up->p0.GetY())
                      * (up->timeInterval.end.ToDouble()
                      -  up->timeInterval.start.ToDouble())
                      +  up->timeInterval.start.ToDouble());
              t.SetType(instanttype);
              if(TUA_DEBUG)
                cout<<"t "<<t.ToString()<<endl;
              newper.end = t;
              newper.rc = (up->timeInterval.end == t)
                         ? up->timeInterval.rc : true;
            }
            if(up->p0.GetY() <= l->GetRightPoint().GetY()
             && up->p1.GetY() >= l->GetRightPoint().GetY()){
              if(TUA_DEBUG)
                cout<<"uPoint ends above linesegemet"<<endl;
              t.ReadFrom((l->GetRightPoint().GetY() - up->p0.GetY())
                      / (up->p1.GetY() - up->p0.GetY())
                      * (up->timeInterval.end.ToDouble()
                      -  up->timeInterval.start.ToDouble())
                      +  up->timeInterval.start.ToDouble());
              t.SetType(instanttype);
              if(TUA_DEBUG)
                cout<<"t "<<t.ToString()<<endl;
              newper.end = t;
              newper.rc = (up->timeInterval.end == t)
                         ? up->timeInterval.rc : true;
            }
            if(up->p1.GetY() <= l->GetRightPoint().GetY()
             && up->p0.GetY() >= l->GetRightPoint().GetY()){
              if(TUA_DEBUG)
                cout<<"uPoint starts above linesegemet"<<endl;
              t.ReadFrom((l->GetRightPoint().GetY() - up->p0.GetY())
                      / (up->p1.GetY() - up->p0.GetY())
                      * (up->timeInterval.end.ToDouble()
                      - up->timeInterval.start.ToDouble())
                      + up->timeInterval.start.ToDouble());
              t.SetType(instanttype);
              if(TUA_DEBUG)
                cout<<"t "<<t.ToString()<<endl;
              newper.start = t;
              newper.lc = (up->timeInterval.start == t)
                         ? up->timeInterval.lc : true;
            }
            if (up->p0.GetY() <= l->GetRightPoint().GetY()
             && up->p0.GetY() >= l->GetLeftPoint().GetY()){
              if(TUA_DEBUG)
                cout<<"uPoint starts inside linesegemet"<<endl;
              newper.start = up->timeInterval.start;
              newper.lc =    up->timeInterval.lc;
            }
            if( up->p1.GetY() <= l->GetRightPoint().GetY()
             && up->p1.GetY() >= l->GetLeftPoint().GetY()){
              if(TUA_DEBUG)
                cout<<"uPoint ends inside linesegemet"<<endl;
              newper.end = up->timeInterval.end;
              newper.rc =  up->timeInterval.rc;
            }
            if(newper.start == newper.end
             && (!newper.lc || !newper.rc)){
              if(TUA_DEBUG)
                cout<<"not an interval"<<endl;
              continue;
            }
          }
        }
      }
      else if(vl){
        if(TUA_DEBUG)
          cout<<"vl is vertical vup not"<<endl;
        t.ReadFrom((l->GetRightPoint().GetX() - up->p0.GetX())
                / (up->p1.GetX() - up->p0.GetX())
                * (up->timeInterval.end.ToDouble()
                -  up->timeInterval.start.ToDouble())
                +  up->timeInterval.start.ToDouble());
        t.SetType(instanttype);
        if(TUA_DEBUG)
          cout<<"t "<<t.ToString()<<endl;
        if((up->timeInterval.start == t && !up->timeInterval.lc)
         ||  (up->timeInterval.end == t && !up->timeInterval.rc))
          continue;

        if(up->timeInterval.start > t|| up->timeInterval.end < t){
          if(TUA_DEBUG)
            cout<<"up outside line"<<endl;
          continue;
        }
        up->TemporalFunction(t, pt);
        if(  pt.GetX() < l->GetLeftPoint().GetX() ||
             pt.GetX() > l->GetRightPoint().GetX()
         || (pt.GetY() < l->GetLeftPoint().GetY() &&
             pt.GetY() < l->GetRightPoint().GetY())
         || (pt.GetY() > l->GetLeftPoint().GetY() &&
             pt.GetY() > l->GetRightPoint().GetY())){
          if(TUA_DEBUG)
            cout<<"pt outside up!"<<endl;
          continue;
        }

        newper.start = t;
        newper.lc = true;
        newper.end = t;
        newper.rc = true;
      }
      else if(vup){
        if(TUA_DEBUG)
          cout<<"vup is vertical vl not"<<endl;
        if(up->p1.GetY() != up->p0.GetY()) {
          t.ReadFrom((up->p0.GetX() * al + bl - up->p0.GetY())
                  / (up->p1.GetY() - up->p0.GetY())
                  * (up->timeInterval.end.ToDouble()
                  -  up->timeInterval.start.ToDouble())
                  +  up->timeInterval.start.ToDouble());
          t.SetType(instanttype);
          if(TUA_DEBUG)
            cout<<"t "<<t.ToString()<<endl;
          if((up->timeInterval.start == t && !up->timeInterval.lc)
           ||  (up->timeInterval.end == t && !up->timeInterval.rc)){
            if(TUA_DEBUG)
              cout<<"continue"<<endl;
            continue;
          }

          if(up->timeInterval.start > t|| up->timeInterval.end < t){
            if(TUA_DEBUG)
              cout<<"up outside line"<<endl;
            continue;
          }
          up->TemporalFunction(t, pt);
          if(  pt.GetX() < l->GetLeftPoint().GetX() ||
               pt.GetX() > l->GetRightPoint().GetX()
           || (pt.GetY() < l->GetLeftPoint().GetY() &&
               pt.GetY() < l->GetRightPoint().GetY())
           || (pt.GetY() > l->GetLeftPoint().GetY() &&
               pt.GetY() > l->GetRightPoint().GetY())){
            if(TUA_DEBUG)
              cout<<"pt outside up!"<<endl;
            continue;
          }

          newper.start = t;
          newper.lc = true;
          newper.end = t;
          newper.rc = true;
        }
        else {
          if(TUA_DEBUG)
            cout<<"up is not moving"<<endl;
          if(al * up->p1.GetX() + bl == up->p1.GetY()){
            if(TUA_DEBUG)
              cout<<"Point lies on line"<<endl;
            newper = up->timeInterval;
          }
          else {
            if(TUA_DEBUG)
              cout<<"continue 2"<<endl;
            continue;
          }
        }
      }
      else if(aup == al){
        if(TUA_DEBUG)
          cout<<"both lines have same gradient"<<endl;
        if(bup != bl){
          if(TUA_DEBUG)
            cout<<"colinear but not equal"<<endl;
          continue;
        }
         if(up->p0.GetX() <= l->GetLeftPoint().GetX()
         && up->p1.GetX() >= l->GetLeftPoint().GetX()){
           if(TUA_DEBUG)
             cout<<"uPoint starts left of linesegemet"<<endl;
           t.ReadFrom((l->GetLeftPoint().GetX() - up->p0.GetX())
                   / (up->p1.GetX() - up->p0.GetX())
                   * (up->timeInterval.end.ToDouble()
                   -  up->timeInterval.start.ToDouble())
                   +  up->timeInterval.start.ToDouble());
           t.SetType(instanttype);
           if(TUA_DEBUG)
             cout<<"t "<<t.ToString()<<endl;
           newper.start = t;
           newper.lc = (up->timeInterval.start == t)
                      ? up->timeInterval.lc : true;
        }
        if(up->p1.GetX() <= l->GetLeftPoint().GetX()
        && up->p0.GetX() >= l->GetLeftPoint().GetX()){
           if(TUA_DEBUG)
             cout<<"uPoint ends left of linesegemet"<<endl;
           t.ReadFrom((l->GetLeftPoint().GetX() - up->p0.GetX())
                   / (up->p1.GetX() - up->p0.GetX())
                   * (up->timeInterval.end.ToDouble()
                   -  up->timeInterval.start.ToDouble())
                   +  up->timeInterval.start.ToDouble());
           t.SetType(instanttype);
           if(TUA_DEBUG)
             cout<<"t "<<t.ToString()<<endl;
           newper.end = t;
           newper.rc = (up->timeInterval.end == t)
                      ? up->timeInterval.rc : true;
        }
        if(up->p0.GetX() <= l->GetRightPoint().GetX()
        && up->p1.GetX() >= l->GetRightPoint().GetX()){
           if(TUA_DEBUG)
             cout<<"uPoint ends right of linesegemet"<<endl;
           t.ReadFrom((l->GetRightPoint().GetX() - up->p0.GetX())
                   / (up->p1.GetX() - up->p0.GetX())
                   * (up->timeInterval.end.ToDouble()
                   -  up->timeInterval.start.ToDouble())
                   +  up->timeInterval.start.ToDouble());
           t.SetType(instanttype);
           if(TUA_DEBUG)
             cout<<"t "<<t.ToString()<<endl;
           newper.end = t;
           newper.rc = (up->timeInterval.end == t)
                      ? up->timeInterval.rc : true;
        }
        if(up->p1.GetX() <= l->GetRightPoint().GetX()
        && up->p0.GetX() >= l->GetRightPoint().GetX()){
           if(TUA_DEBUG)
             cout<<"uPoint starts right of linesegemet"<<endl;
           t.ReadFrom((l->GetRightPoint().GetX() - up->p0.GetX())
                   / (up->p1.GetX() - up->p0.GetX())
                   * (up->timeInterval.end.ToDouble()
                   -  up->timeInterval.start.ToDouble())
                   +  up->timeInterval.start.ToDouble());
           t.SetType(instanttype);
           if(TUA_DEBUG)
             cout<<"t "<<t.ToString()<<endl;
           newper.start = t;
           newper.lc = (up->timeInterval.start == t)
                      ? up->timeInterval.lc : true;
        }
        if(up->p0.GetX() <= l->GetRightPoint().GetX()
        && up->p0.GetX() >= l->GetLeftPoint().GetX()){
           if(TUA_DEBUG)
             cout<<"uPoint starts inside linesegemet"<<endl;
           newper.start = up->timeInterval.start;
           newper.lc =    up->timeInterval.lc;
        }
        if(up->p1.GetX() <= l->GetRightPoint().GetX()
        && up->p1.GetX() >= l->GetLeftPoint().GetX()){
           if(TUA_DEBUG)
             cout<<"uPoint ends inside linesegemet"<<endl;
           newper.end = up->timeInterval.end;
           newper.rc =  up->timeInterval.rc;
        }
        if(newper.start == newper.end
        && (!newper.lc || !newper.rc)){
          if(TUA_DEBUG)
            cout<<"not an interval"<<endl;
          continue;
        }
      }
      else{
        if(TUA_DEBUG)
          cout<<"both lines have different gradients"<<endl;
        t.ReadFrom(((bl - bup) / (aup - al) - up->p0.GetX())
                / (up->p1.GetX() - up->p0.GetX())
                * (up->timeInterval.end.ToDouble()
                -  up->timeInterval.start.ToDouble())
                +  up->timeInterval.start.ToDouble());
        t.SetType(instanttype);
        if((up->timeInterval.start == t && !up->timeInterval.lc)
         ||  (up->timeInterval.end == t && !up->timeInterval.rc)){
          if(TUA_DEBUG)
            cout<<"continue"<<endl;
          continue;
        }

        if(up->timeInterval.start > t|| up->timeInterval.end < t){
          if(TUA_DEBUG)
            cout<<"up outside line"<<endl;
          continue;
        }
        up->TemporalFunction(t, pt);
        if(  pt.GetX() < l->GetLeftPoint().GetX() ||
             pt.GetX() > l->GetRightPoint().GetX()
         || (pt.GetY() < l->GetLeftPoint().GetY() &&
             pt.GetY() < l->GetRightPoint().GetY())
         || (pt.GetY() > l->GetLeftPoint().GetY() &&
             pt.GetY() > l->GetRightPoint().GetY())){
          if(TUA_DEBUG)
            cout<<"pt outside up!"<<endl;
          continue;
        }

        newper.start = t;
        newper.lc = true;
        newper.end = t;
        newper.rc = true;
      }
      if(TUA_DEBUG){
        cout<<"newper ["<< newper.start.ToString()<<" "<<newper.end.ToString()
        <<" "<<newper.lc<<" "<<newper.rc<<"]"<<endl;}
      period->Clear();
      period->StartBulkLoad();
      period->Add(newper);
      period->EndBulkLoad(false);
      if (!pResult.IsEmpty()) {
        between->Clear();
        period->Union(pResult, *between);
        pResult.Clear();
        pResult.CopyFrom(between);
      }
      else
        pResult.CopyFrom(period);
    }
  delete between;
  delete period;
}

/*
Method ~TUCompletePeriods2MPoint~

Copied from TempralLiftedAlgebra

Completes a Periods-value to a MPoint-value. For this it adds the starting
and end points.

*/
static void TUCompletePeriods2MPoint(UPoint* u, Periods* pResult,
  MPoint* endResult){
  if(TUA_DEBUG)
    cout<<"TUCompletePeriods2MPoint called"<<endl;

  const UPoint* up = (UPoint*) u;
  endResult->Clear();
  endResult->StartBulkLoad();
  const Interval<Instant> *per;
  UPoint newUp(true);
  Point pt;
  int m = 0;
  bool pfinished = (pResult->GetNoComponents() == 0);
  for ( int i = 0; i < 1; i++) {
    if(!up->IsDefined())
        continue;
    if(TUA_DEBUG){
      cout<<"UPoint # "<<" ["<<up->timeInterval.start.ToString()
      <<" "<<up->timeInterval.end.ToString()<<" "<<up->timeInterval.lc<<" "
      <<up->timeInterval.rc<<"] ("<<up->p0.GetX()<<" "<<up->p0.GetY()<<")->("
      <<up->p1.GetX()<<" "<<up->p1.GetY()<<")"<<endl;}
    if(!pfinished) {
      pResult->Get(m, per);
      if(TUA_DEBUG){
        cout<<"per "<<m<<" ["<<per->start.ToString()<<" "
        <<per->end.ToString()<<" "<<per->lc<<" "<<per->rc<<"]"<<endl;}
    }
    if(pfinished) {
      if(TUA_DEBUG)
        cout<<"no per any more. break 1"<<endl;
      break;
    }
    if(!(pfinished || up->timeInterval.end < per->start
     || (up->timeInterval.end == per->start
     && !up->timeInterval.rc && per->lc))) {
      if(TUA_DEBUG)
        cout<<"per not totally after up"<<endl;
      if(up->timeInterval.start < per->start
       || (up->timeInterval.start == per->start
       && up->timeInterval.lc && !per->lc)) {
        if(TUA_DEBUG)
          cout<<"up starts before per"<<endl;
        newUp.timeInterval = *per;
      }
      else {
        if(TUA_DEBUG)
          cout<<"per starts before or with up"<<endl;
        newUp.timeInterval.start = up->timeInterval.start;
        newUp.timeInterval.lc = up->timeInterval.lc;
      }
      while(true) {
        if(up->timeInterval.end < per->end
         || (up->timeInterval.end == per->end
         && per->rc && !up->timeInterval.rc)) {
            if(TUA_DEBUG)
              cout<<"per ends after up (break)"<<endl;
            newUp.timeInterval.end = up->timeInterval.end;
            newUp.timeInterval.rc = up->timeInterval.rc;
            up->TemporalFunction(newUp.timeInterval.start, pt, true);
            newUp.p0 = pt;
            up->TemporalFunction(newUp.timeInterval.end, pt, true);
            newUp.p1 = pt;
            if(TUA_DEBUG){
              cout<<"Add3 ("<<newUp.p0.GetX()<<" "<<newUp.p0.GetY()
              <<")->("<<newUp.p1.GetX()<<" "<<newUp.p1.GetY()
              <<") ["<<newUp.timeInterval.start.ToString()<<" "
              <<newUp.timeInterval.end.ToString()<<" "
              <<newUp.timeInterval.lc<<" "<<newUp.timeInterval.rc<<"]"<<endl;}
            endResult->Add(newUp);
            break;
        }
        else {
          if(TUA_DEBUG)
            cout<<"per ends inside up"<<endl;
          newUp.timeInterval.end = per->end;
          newUp.timeInterval.rc = per->rc;
          up->TemporalFunction(newUp.timeInterval.start, pt, true);
          newUp.p0 = pt;
          up->TemporalFunction(newUp.timeInterval.end, pt, true);
          newUp.p1 = pt;
          if(TUA_DEBUG){
            cout<<"Add4 ("<<newUp.p0.GetX()<<" "<<newUp.p0.GetY()
             <<")->("<<newUp.p1.GetX()<<" "<<newUp.p1.GetY()
            <<") ["<<newUp.timeInterval.start.ToString()<<" "
            <<newUp.timeInterval.end.ToString()<<" "
            <<newUp.timeInterval.lc<<" "<<newUp.timeInterval.rc<<"]"<<endl;}
          endResult->Add(newUp);
        }
        if(m == pResult->GetNoComponents() - 1){
          if(TUA_DEBUG)
            cout<<"last per"<<endl;
          pfinished = true;
        }
        else {
          pResult->Get(++m, per);
          if(TUA_DEBUG){
            cout<<"per "<<m<<" ["<<per->start.ToString()
            <<" "<<per->end.ToString()<<" "<<per->lc<<" "<<per->rc<<"]"<<endl;}
        }
        if(!pfinished && (per->start < up->timeInterval.end
           || (per->start == up->timeInterval.end
           && up->timeInterval.rc && per->rc))){
          if(TUA_DEBUG)
            cout<<"next per starts in same up"<<endl;
          newUp.timeInterval.start = per->start;
          newUp.timeInterval.lc = per->lc;
        }
        else {
          if(TUA_DEBUG)
            cout<<"next interval after up -> finish up"<<endl;
          break;
        }
      } //while
    }
  }
  endResult->EndBulkLoad(false);
}

/*
The value mapping function:

*/

template<int uargindex>
int temporalUnitIntersection_upoint_line( Word* args, Word& result,
                                          int message,
                                          Word& local, Supplier s )
{
  TUIntersectionLocalInfo *sli;
  Word     a0, a1;
  UPoint   res(true);
  UPoint  *u;
  Line    *l;
  Periods *p;

  const UPoint* cu;

  switch( message )
    {
    case OPEN:

      if (TUA_DEBUG)
        cout << "temporalUnitIntersection_upoint_line<"
             << uargindex << ">: Received OPEN" << endl;

      p = new Periods(10);
      sli = new TUIntersectionLocalInfo;
      sli->finished = true;
      sli->NoOfResults = 0;
      sli->NoOfResultsDelivered = 0;
      sli->mpoint = new MPoint(10);
      local = SetWord(sli);

      // initialize arguments, such that a0 always contains the upoint
      //                       and a1 the line
      if (TUA_DEBUG) cout << "  uargindex=" << uargindex << endl;
      if (uargindex == 0)
        { a0 = args[0]; a1 = args[1]; }
      else
        { a0 = args[1]; a1 = args[0]; }
      u = (UPoint*)(a0.addr);
      l = (Line*)(a1.addr);

      // test for definedness
      if ( !u->IsDefined() || !l->IsDefined() || l->IsEmpty() )
        {
          if (TUA_DEBUG)
            cout << "  Undef/Empty arg -> Empty Result" << endl << endl;
          // nothing to do
        }
      else
        {
          TUUPointInsideLine(u, *l, *p);    // get intersecting timeintervals
          TUCompletePeriods2MPoint(u, p, sli->mpoint); // create upoints
          sli->NoOfResults = sli->mpoint->GetNoComponents();
          sli->finished = (sli->NoOfResults <= 0);
          if (TUA_DEBUG)
            cout << "  " << sli->NoOfResults << " result units" << endl << endl;
        }
      delete p;
      if (TUA_DEBUG)
        cout << "temporalUnitIntersection_upoint_line: Finished OPEN"
             << endl;
      return 0;

    case REQUEST:

      if(local.addr == 0)
        return CANCEL;
      sli = (TUIntersectionLocalInfo*) local.addr;
      if(sli->finished)
        return CANCEL;
      if(sli->NoOfResultsDelivered < sli->NoOfResults)
        {
          sli->mpoint->Get(sli->NoOfResultsDelivered, cu);
          result = SetWord( cu->Clone() );
          sli->NoOfResultsDelivered++;
          return YIELD;
        }
      sli->finished = true;
      return CANCEL;

    case CLOSE:

      if (local.addr != 0)
        {
          sli = (TUIntersectionLocalInfo*) local.addr;
          delete sli->mpoint;
          delete sli;
        }
      return 0;
    } // end switch

  return 0;
}

// for (upoint uregion) -> (stream upoint)
//     (uregion upoint) -> (stream upoint)
template<int uargindex>
int temporalUnitIntersection_upoint_uregion( Word* args, Word& result,
                                             int message,
                                             Word& local, Supplier s )
{
// This implementation uses class function
//   MRegion::Intersection(MPoint& mp, MPoint& res)
//   by creating a single-unit MRegion and a single-unit MPoint

  TUIntersectionLocalInfo *sli;
  Word    a0, a1;
  UPoint  *u;
  URegion *r;
  MPoint  mp_tmp(1);
  MRegion mr_tmp(1);
  const UPoint* cu;

  switch( message )
    {
    case OPEN:

      if (TUA_DEBUG)
        cerr << "temporalUnitIntersection_upoint_uregion<"
             << uargindex << ">: Received OPEN" << endl;

      sli = new TUIntersectionLocalInfo;
      sli->finished = true;
      sli->NoOfResults = 0;
      sli->NoOfResultsDelivered = 0;
      sli->mpoint = new MPoint(10);
      local = SetWord(sli);

      // initialize arguments, such that a0 always contains the upoint
      //                       and a1 the uregion
      if (TUA_DEBUG) cerr << "  uargindex=" << uargindex << endl;
      if (uargindex == 0)
        { a0 = args[0]; a1 = args[1]; }
      else if (uargindex == 1)
        { a0 = args[1]; a1 = args[0]; }
      else
        {
          cerr << "temporalUnitIntersection_upoint_uregion<"
               << uargindex << ">: WRONG uargindex!" << endl;
          return -1;
        }
      u = (UPoint*)(a0.addr);
      r = (URegion*)(a1.addr);

      // test for definedness
      if ( !u->IsDefined() || !r->IsDefined() )
        {
          if (TUA_DEBUG)
            cerr << "  Undef arg -> Empty Result" << endl << endl;
          // nothing to do
        }
      else
        {
          mp_tmp.Clear();         // create temporary MPoint
          mp_tmp.Add(*u);
          mp_tmp.SetDefined(true);

          mr_tmp.Clear();         // create temporary MRegion
          //mr_tmp.StartBulkLoad();
          mr_tmp.AddURegion(*r);
          //mr_tmp.EndBulkLoad();
          mr_tmp.SetDefined(true);

          mr_tmp.Intersection(mp_tmp, *(sli->mpoint)); // get and save result;
          sli->NoOfResults = sli->mpoint->GetNoComponents();
          sli->finished = (sli->NoOfResults <= 0);
          if (TUA_DEBUG)
            cerr << "  " << sli->NoOfResults << " result units" << endl << endl;
        }
      if (TUA_DEBUG)
        cerr << "temporalUnitIntersection_upoint_uregion: Finished OPEN"
             << endl;
      return 0;

    case REQUEST:
      if (TUA_DEBUG)
        cerr << "temporalUnitIntersection_upoint_uregion<"
             << uargindex << ">: Received REQUEST" << endl;

      if(local.addr == 0)
        {
          if (TUA_DEBUG)
            cerr << "temporalUnitIntersection_upoint_uregion<"
                 << uargindex << ">: Finished REQUEST (1)" << endl;
          return CANCEL;
        }
      sli = (TUIntersectionLocalInfo*) local.addr;
      if(sli->finished)
        {
          if (TUA_DEBUG)
            cerr << "temporalUnitIntersection_upoint_uregion<"
                 << uargindex << ">: Finished REQUEST (2)" << endl;
          return CANCEL;
        }
      if(sli->NoOfResultsDelivered < sli->NoOfResults)
        {
          sli->mpoint->Get(sli->NoOfResultsDelivered, cu);
          result = SetWord( cu->Clone() );
          sli->NoOfResultsDelivered++;
          if (TUA_DEBUG)
            cerr << "temporalUnitIntersection_upoint_uregion<"
                 << uargindex << ">: Finished REQUEST (YIELD)" << endl;
          return YIELD;
        }
      sli->finished = true;
      if (TUA_DEBUG)
        cerr << "temporalUnitIntersection_upoint_uregion<"
             << uargindex << ">: Finished REQUEST (3)" << endl;
      return CANCEL;

    case CLOSE:

      if (TUA_DEBUG)
        cerr << "temporalUnitIntersection_upoint_uregion<"
             << uargindex << ">: Received CLOSE" << endl;
      if (local.addr != 0)
        {
          sli = (TUIntersectionLocalInfo*) local.addr;
          delete sli->mpoint;
          delete sli;
        }
      if (TUA_DEBUG)
        cerr << "temporalUnitIntersection_upoint_uregion<"
             << uargindex << ">: Finished CLOSE" << endl;
      return 0;
    } // end switch

  cerr << "temporalUnitIntersection_upoint_uregion<"
       << uargindex << ">: Received UNKNOWN COMMAND" << endl;
  return 0;
}

// for (upoint region) -> (stream upoint)
//     (region upoint) -> (stream upoint)
template<int uargindex>
int temporalUnitIntersection_upoint_region( Word* args, Word& result,
                                            int message,
                                            Word& local, Supplier s )
{
  cout << "\nATTENTION: temporalUnitIntersection_upoint_region "
       << "not yet implemented!" << endl;

  return 0;
}

/*
5.26.3 Specification for operator ~intersection~

*/

const string  TemporalUnitIntersectionSpec  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" "
  "\"Example\" ) "
  "("
  "<text>For T in {bool, int, real, string, point}:\n"
  "(uT uT) -> (stream uT)**\n"
  "(uT  T) -> (stream uT)\n"
  "( T uT) -> (stream uT)\n"
  "(line upoint) -> (stream upoint)\n"
  "(upoint line) -> (stream upoint)\n"
  "(upoint uregion) -> (stream upoint)\n"
  "(uregion upoint) -> (stream upoint)\n"
  "(upoint region) -> (stream upoint)*\n"
  "(region upoint) -> (stream upoint)*\n"
  "(*):  Not yet implemented\n"
  "(**): Not yet implemented for T = real</text--->"
  "<text>intersection(_, _)</text--->"
  "<text>Returns the intersection of two unit datatype values "
  "or of a unit datatype and its corrosponding simple datatype "
  "as a stream of that unit datatype.</text--->"
  "<text>query intersection(upoint1, upoint2) count</text--->"
  ") )";


/*
5.26.4 Selection Function of operator ~intersection~

*/

ValueMapping temporalunitintersectionmap[] =
  {
    temporalUnitIntersection_CU_CU<UBool>,           // 0
    temporalUnitIntersection_CU_CU<UInt>,
    temporalUnitIntersection_ureal_ureal,
    temporalUnitIntersection_upoint_upoint,
    temporalUnitIntersection_CU_CU<UString>,

    temporalUnitIntersection_CU_C<UBool, CcBool, 0>, // 5
    temporalUnitIntersection_CU_C<UInt, CcInt, 0>,
    temporalUnitIntersection_ureal_real<0>,
    temporalUnitIntersection_upoint_point<0>,
    temporalUnitIntersection_CU_C<UString, CcString, 0>,

    temporalUnitIntersection_CU_C<UBool, CcBool, 1>, // 10
    temporalUnitIntersection_CU_C<UInt, CcInt, 1>,
    temporalUnitIntersection_ureal_real<1>,
    temporalUnitIntersection_upoint_point<1>,
    temporalUnitIntersection_CU_C<UString, CcString, 1>,

    temporalUnitIntersection_upoint_line<0>,        // 15
    temporalUnitIntersection_upoint_line<1>,

    temporalUnitIntersection_upoint_uregion<0>,
    temporalUnitIntersection_upoint_uregion<1>,

    temporalUnitIntersection_upoint_region<0>,
    temporalUnitIntersection_upoint_region<1>       // 20
  };

int temporalunitIntersectionSelect( ListExpr args )
{
  ListExpr arg1 = nl->First(args);
  ListExpr arg2 = nl->Second(args);
  if( nl->IsEqual( arg1, "ubool" )   &&
      nl->IsEqual( arg2, "ubool") )   return 0;
  if( nl->IsEqual( arg1, "uint" )    &&
      nl->IsEqual( arg2, "uint" ) )    return 1;
  if( nl->IsEqual( arg1, "ureal" )   &&
      nl->IsEqual( arg2, "ureal" ) )   return 2;
  if( nl->IsEqual( arg1, "upoint" )  &&
      nl->IsEqual( arg2, "upoint" ) )  return 3;
  if( nl->IsEqual( arg1, "ustring" ) &&
      nl->IsEqual( arg2, "ustring" ) ) return 4;

  if( nl->IsEqual( arg1, "ubool" )   &&
      nl->IsEqual( arg2, "bool" ))     return 5;
  if( nl->IsEqual( arg1, "uint" )    &&
      nl->IsEqual( arg2, "int" ))      return 6;
  if( nl->IsEqual( arg1, "ureal" )   &&
      nl->IsEqual( arg2, "real" ))     return 7;
  if( nl->IsEqual( arg1, "upoint" )  &&
      nl->IsEqual( arg2, "point"  ) )  return 8;
  if( nl->IsEqual( arg1, "ustring" ) &&
      nl->IsEqual( arg2, "string" ))   return 9;

  if( nl->IsEqual( arg2, "ubool" )   &&
      nl->IsEqual( arg1, "bool" ))     return 10;
  if( nl->IsEqual( arg2, "uint" )    &&
      nl->IsEqual( arg1, "int" ))      return 11;
  if( nl->IsEqual( arg2, "ureal" )   &&
      nl->IsEqual( arg1, "real" ))     return 12;
  if( nl->IsEqual( arg2, "upoint" )  &&
      nl->IsEqual( arg1, "point" ))    return 13;
  if( nl->IsEqual( arg2, "ustring" ) &&
      nl->IsEqual( arg1, "ustring" ))  return 14;

  if( nl->IsEqual( arg1, "upoint" )    &&
      nl->IsEqual( arg2, "line" ))       return 15;
  if( nl->IsEqual( arg1, "line" )      &&
      nl->IsEqual( arg2, "upoint" ) )    return 16;
  if( nl->IsEqual( arg1, "upoint" )    &&
      nl->IsEqual( arg2, "uregion" ) )   return 17;
  if( nl->IsEqual( arg1, "uregion" )   &&
      nl->IsEqual( arg2, "upoint" ) )    return 18;
  if( nl->IsEqual( arg1, "upoint" )    &&
      nl->IsEqual( arg2, "region" ) )    return 19;
  if( nl->IsEqual( arg1, "region" )    &&
      nl->IsEqual( arg2, "upoint" ) )    return 20;

  cerr << "ERROR: Unmatched case in temporalunitIntersectionSelect" << endl;
  string argstr;
  nl->WriteToString(argstr, args);
  cerr << "       Argumets = '" << argstr << "'." << endl;
  return -1;
}


/*
5.26.5 Definition of operator ~intersection~

*/

Operator temporalunitintersection( "intersection",
                                   TemporalUnitIntersectionSpec,
                                   19,
                                   temporalunitintersectionmap,
                                   temporalunitIntersectionSelect,
                                   TemporalUnitIntersectionTypeMap);



/*
5.31 Operator ~no_components~

Return the number of components (units) contained by the object. For unit types,
the result is either undef (for undef values) or a const unit with value=1
(otherwise).

----
     n/a + no_components:     (uT) --> uint

----

*/

/*
5.31.1 Type mapping function for ~no_components~

*/

static ListExpr TUNoComponentsTypeMap(ListExpr args) {

  if (nl->ListLength(args) == 1)
    {
      if (nl->IsEqual(nl->First(args), "ubool"))
        return nl->SymbolAtom("uint");
      if (nl->IsEqual(nl->First(args), "uint"))
        return nl->SymbolAtom("uint");
      if (nl->IsEqual(nl->First(args), "ureal"))
        return nl->SymbolAtom("uint");
      if (nl->IsEqual(nl->First(args), "ustring"))
        return nl->SymbolAtom("uint");
      if (nl->IsEqual(nl->First(args), "upoint"))
        return nl->SymbolAtom("uint");
      if (nl->IsEqual(nl->First(args), "uregion"))
        return nl->SymbolAtom("uint");
    }
  return nl->SymbolAtom("typeerror");
}

/*
5.31.2 Value mapping for operator ~no_components~

*/

template<class T>
int TUNoComponentsValueMap(Word* args, Word& result,
                           int message, Word& local, Supplier s)
{
  result = (qp->ResultStorage( s ));
  UInt  *res   = (UInt*)result.addr;
  T     *input = (T*)args[0].addr;

  if ( input->IsDefined() )
    {
      res->SetDefined(true);
      res->timeInterval.CopyFrom(input->timeInterval);
      res->constValue.Set(true,1);
    }
  else
    {
      res->SetDefined(false);
      res->constValue.Set(true,0);
    }
  return 0;
}

/*
5.31.3 Specification for operator ~no_components~

*/
const string TUNoComponentsSpec  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>For T in {bool, int, real, point, region}:\n"
  "(uT) -> uint</text--->"
  "<text>no_components( _ )</text--->"
  "<text>Returns an undef uint for undef unit, an uint with the "
  "argument's deftime and value '1' otherwise.</text--->"
  "<text>query no_components([const uint value undef])</text--->"
  ") )";

/*
5.31.4 Selection Function of operator ~no_components~

Uses ~UnitSimpleSelect( ListExpr args )~

*/

ValueMapping temporalunitNoComponentsvaluemap[] = {
  TUNoComponentsValueMap<UBool>,
  TUNoComponentsValueMap<UInt>,
  TUNoComponentsValueMap<UReal>,
  TUNoComponentsValueMap<UPoint>,
  TUNoComponentsValueMap<UString>,
  TUNoComponentsValueMap<URegion>};
/*
5.31.5 Definition of operator ~no_components~

*/
Operator temporalunitnocomponents
(
 "no_components",
 TUNoComponentsSpec,
 6,
 temporalunitNoComponentsvaluemap,
 UnitSimpleSelect,
 TUNoComponentsTypeMap
);

/*
5.32 Operator ~isempty~

The operator returns TRUE, if the unit is undefined, otherwise it returns FALSE.
It will never return an undefined result!

----
      For U in kind UNIT and U in kind DATA:
      isempty  U --> bool

----

*/

/*
5.32.1 Type mapping function for ~isempty~

*/
ListExpr
TUIsemptyTypeMap( ListExpr args )
{
  ListExpr arg1;
  ListExpr errorInfo = nl->OneElemList(nl->SymbolAtom("ERROR"));

  if ( ( nl->ListLength(args) == 1 ) && ( nl->IsAtom(nl->First(args) ) ) )
    {
      arg1 = nl->First(args);
      if( am->CheckKind("DATA", arg1, errorInfo) &&
          am->CheckKind("UNIT", arg1, errorInfo))
        return arg1;
    }
  ErrorReporter::ReportError("Operator isempty expects a list of length one, "
                             "containing a value of type 'U' with U in "
                             "kind UNIT and in kind DATA.");
  return nl->SymbolAtom( "typeerror" );
}

/*
5.32.2 Value mapping for operator ~isempty~

*/
int TUIsemptyValueMap( Word* args, Word& result, int message,
                       Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Attribute* val = ((Attribute*)args[0].addr);
  if( val->IsDefined() )
    ((CcBool *)result.addr)->Set( true, true );
  else
    ((CcBool *)result.addr)->Set( true, false );
  return 0;
}

/*
5.32.3 Specification for operator ~isempty~

*/
const string TUIsemptySpec  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>For U in kind UNIT and in kind DATA:\n"
  "U -> bool</text--->"
  "<text>isempty( _ )</text--->"
  "<text>The operator returns TRUE, if the unit is undefined, "
  "otherwise it returns FALSE. It will never return an "
  "undefined result!</text--->"
  "<text>query is_empty([const uint value undef])</text--->"
  ") )";

/*
5.32.4 Selection Function of operator ~isempty~

Simple selection is used, as there is only one value mapping function.

*/

ValueMapping temporalunitIsemptyvaluemap[] = { TUIsemptyValueMap };

/*
5.32.5 Definition of operator ~isempty~

*/
Operator temporalunitisempty
(
 "isempty",
 TUIsemptySpec,
 1,
 temporalunitIsemptyvaluemap,
 Operator::SimpleSelect,
 TUIsemptyTypeMap
);

/*
5.33 Operator ~not~

Negates an ubool.

----
     not:   ubool --> ubool

----

*/

/*
5.33.1 Type mapping function for ~not~

*/

ListExpr
TUNotTypeMap( ListExpr args )
{
  if ( nl->ListLength( args ) == 1 )
  {
    ListExpr arg1 = nl->First( args );

    if( nl->IsEqual( arg1, "ubool" )  )
      return nl->SymbolAtom( "ubool" );
  }
  return nl->SymbolAtom( "typeerror" );
}
/*
5.33.2 Value mapping for operator ~not~

*/

int TUNotValueMap(Word* args, Word& result, int message,
                      Word& local, Supplier s)
{
  result = (qp->ResultStorage( s ));
  UBool *res = (UBool*) result.addr;
  UBool *input = (UBool*)args[0].addr;

  if(!input->IsDefined())
    res->SetDefined( false );
  else
    {
      res->SetDefined( true );
      res->CopyFrom(input);
      res->constValue.Set(res->constValue.IsDefined(),
                          !(res->constValue.GetBoolval()));
    }
  return 0;
}
/*
5.33.3 Specification for operator ~not~

*/
const string TUNotSpec  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>ubool -> ubool</text--->"
  "<text>not( _ )</text--->"
  "<text>The operator returns the logical complement of "
  "its argument. Undef argument resullts in an undef result."
  "</text--->"
  "<text>query not([const ubool value ((\"2010-11-11\" "
  "\"2011-01-03\" TRUE FALSE) TRUE)])</text--->"
  ") )";
/*
5.33.4 Selection Function of operator ~not~

We use simple selection:

*/

ValueMapping temporalunitNotvaluemap[] = { TUNotValueMap };

/*
5.33.5 Definition of operator ~not~

*/
Operator temporalunitnot
(
 "not",
 TUNotSpec,
 1,
 temporalunitNotvaluemap,
 Operator::SimpleSelect,
 TUNotTypeMap
);

/*
5.34 Operators ~and~ and ~or~

These operators perform the binary conjunction/disjunction for ubool (and bool):

----

    and, or:  ubool x ubool -> ubool
               bool x ubool -> ubool
              ubool x  bool -> ubool
----

*/

/*
5.34.1 Type mapping function for ~and~ and ~or~

We implement a genric function for binary boolean functions here:

*/

ListExpr TUBinaryBoolFuncTypeMap( ListExpr args )
{
  ListExpr arg1, arg2;
  string argstr1, argstr2;

  if( nl->ListLength( args ) == 2 )
    {
      arg1 = nl->First( args );
      arg2 = nl->Second( args );

      // First case: ubool ubool -> ubool
      if ( nl->IsEqual( arg1, "ubool" ) && nl->IsEqual( arg2, "ubool" ) )
        return nl->SymbolAtom("ubool");

      // Second case: ubool bool -> ubool
      if( nl->IsEqual( arg1, "ubool" ) && nl->IsEqual( arg2, "bool") )
        return nl->SymbolAtom("ubool");

      // Third case: bool ubool -> ubool
      if( nl->IsEqual( arg1, "bool" ) && nl->IsEqual( arg2, "ubool") )
        return nl->SymbolAtom("ubool");
    }

  // Error case:
  nl->WriteToString(argstr1, arg1);
  nl->WriteToString(argstr2, arg2);
  ErrorReporter::ReportError(
    "Binary booleon operator expects any combination of {bool, ubool} "
    "as arguments, but the passed arguments have types '"+ argstr1 +
    "' and '" + argstr2 + "'.");
  return nl->SymbolAtom("typeerror");
}
/*
5.34.2 Value mapping for operators ~and~ and ~or~

*/
template<int ArgConf>
int TUAndValueMap(Word* args, Word& result, int message,
                  Word& local, Supplier s)
{
  assert( (ArgConf>=0) && (ArgConf<=2));

  result = (qp->ResultStorage( s ));
  UBool *res = (UBool*) result.addr;
  UBool *u1, *u2;
  CcBool *cb;
  Interval<Instant> iv;

  if(ArgConf == 0) // case: ubool x ubool -> ubool
    {
      u1 = (UBool*) args[0].addr;
      u2 = (UBool*) args[1].addr;

      if  (!u1->IsDefined() ||
           !u2->IsDefined() ||
           !u1->timeInterval.Intersects( u2->timeInterval) )
        res->SetDefined( false );
      else
        {
          u1->timeInterval.Intersection( u2->timeInterval, iv );
          res->SetDefined( true );
          res->timeInterval = iv;
          res->constValue.Set(
            u1->constValue.IsDefined() && u2->constValue.IsDefined(),
            u1->constValue.GetBoolval() && u2->constValue.GetBoolval());
        }
      return 0;
    }
  else if(ArgConf == 1) // case: ubool x bool -> ubool
    {
      u1 = (UBool*)args[0].addr;
      cb = (CcBool*)args[1].addr;
    }
  else if(ArgConf == 2) // case: bool x ubool -> ubool
    {
      u1 = (UBool*)args[1].addr;
      cb = (CcBool*)args[0].addr;
    }
  if (!u1->IsDefined() || !cb->IsDefined())
    res->SetDefined( false );
  else
    {
      res->CopyFrom(u1);
      res->constValue.Set(u1->constValue.IsDefined(),
           u1->constValue.GetBoolval() && cb->GetBoolval() );
    }
  return 0;
}

template<int ArgConf>
int TUOrValueMap(Word* args, Word& result, int message,
                 Word& local, Supplier s)
{
  assert( (ArgConf>=0) && (ArgConf<=2));

  result = (qp->ResultStorage( s ));
  UBool *res = (UBool*) result.addr;
  UBool *u1, *u2;
  CcBool *cb;
  Interval<Instant> iv;

  if(ArgConf == 0) // case: ubool x ubool -> ubool
    {
      u1 = (UBool*) args[0].addr;
      u2 = (UBool*) args[1].addr;

      if  (!u1->IsDefined() ||
           !u2->IsDefined() ||
           !u1->timeInterval.Intersects( u2->timeInterval) )
        res->SetDefined( false );
      else
        {
          u1->timeInterval.Intersection( u2->timeInterval, iv );
          res->SetDefined( true );
          res->timeInterval = iv;
          res->constValue.Set(
            u1->constValue.IsDefined() && u2->constValue.IsDefined(),
            u1->constValue.GetBoolval() || u2->constValue.GetBoolval());
        }
      return 0;
    }
  else if(ArgConf == 1) // case: ubool x bool -> ubool
    {
      u1 = (UBool*)args[0].addr;
      cb = (CcBool*)args[1].addr;
    }
  else if(ArgConf == 2) // case: bool x ubool -> ubool
    {
      u1 = (UBool*)args[1].addr;
      cb = (CcBool*)args[0].addr;
    }
  if (!u1->IsDefined() || !cb->IsDefined())
    res->SetDefined( false );
  else
    {
      res->CopyFrom(u1);
      res->constValue.Set(u1->constValue.IsDefined(),
           u1->constValue.GetBoolval() || cb->GetBoolval() );
    }
  return 0;
}



/*
5.34.3 Specification for operators ~and~ and ~or~

*/
const string TUAndSpec  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(ubool ubool) -> ubool\n"
  "(ubool  bool) -> ubool\n"
  "( bool ubool) -> ubool</text--->"
  "<text>_ and _</text--->"
  "<text>The operator returns the logical conjunction of its "
  "arguments, restricted to their common deftime. Any undefined "
  "argument or non-overlapping intervals result in an "
  "undefined result."
  "</text--->"
  "<text>query [const ubool value ((\"2010-11-11\" "
  "\"2011-01-03\" TRUE FALSE) TRUE)] and [const ubool value "
  "((\"2011-01-01\"2012-09-17\" FALSE TRUE) TRUE)]</text--->"
  ") )";

const string TUOrSpec  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(ubool ubool) -> ubool\n"
  "(ubool  bool) -> ubool\n"
  "( bool ubool) -> ubool</text--->"
  "<text>_ or _</text--->"
  "<text>The operator returns the logical disjunction of its "
  "arguments, restricted to their common deftime. Any undefined "
  "argument or non-overlapping intervals result in an "
  "undefined result."
  "</text--->"
  "<text>query [const ubool value ((\"2010-11-11\" "
  "\"2011-01-03\" TRUE FALSE) TRUE)] or [const ubool value "
  "((\"2011-01-01\"2012-09-17\" FALSE TRUE) TRUE)]</text--->"
  ") )";


/*
5.34.4 Selection Function of operators ~and~ and ~or~

*/
int TUBinaryBoolFuncSelect( ListExpr args )
{
  ListExpr arg1 = nl->First( args ),
           arg2 = nl->Second( args );

  if( nl->SymbolValue( arg1 ) == "ubool"
   && nl->SymbolValue( arg2 ) == "ubool" )
    return 0;
  if( nl->SymbolValue( arg1 ) == "ubool"
   && nl->SymbolValue( arg2 ) == "bool" )
    return 1;
  if( nl->SymbolValue( arg1 ) == "bool"
   && nl->SymbolValue( arg2 ) == "ubool" )
    return 2;

  return -1; // This point should never be reached
}

ValueMapping temporalunitAndValuemap[] = {
  TUAndValueMap<0>,
  TUAndValueMap<1>,
  TUAndValueMap<2>
};

ValueMapping temporalunitOrValuemap[] = {
  TUOrValueMap<0>,
  TUOrValueMap<1>,
  TUOrValueMap<2>
};

/*
5.34.5 Definition of operators ~and~ and ~or~

*/
Operator temporalunitand
(
 "and",
 TUAndSpec,
 3,
 temporalunitAndValuemap,
 TUBinaryBoolFuncSelect,
 TUBinaryBoolFuncTypeMap
);

Operator temporalunitor
(
 "or",
 TUOrSpec,
 3,
 temporalunitOrValuemap,
 TUBinaryBoolFuncSelect,
 TUBinaryBoolFuncTypeMap
);


/*
5.35 Operator ~ComparePredicates~

Here, we implement the binary comparison operators/predicates for (uT uT).
The predicates are = (equality), # (unequality), < (smaller than),
> (bigger than), <= (smaller tah or equal to), >= (bigger than or equal to).

The operators use the internat ~Compare~ function, which implements an ordering on the
elements, but does not need to respect inuitive operator semantics (e.g. in case ureal).

----
      =, #, <, >, <=, >=: For T in {int, bool, real, string, point, region}
n/a +        uT x uT --> bool

----

*/

/*
5.35.1 Type mapping function for ~ComparePredicates~

*/
ListExpr TUComparePredicatesTypeMap( ListExpr args )
{
  ListExpr arg1, arg2;
  string argstr1, argstr2;

  if( nl->ListLength( args ) == 2 )
    {
      arg1 = nl->First( args );
      arg2 = nl->Second( args );
      if (nl->Equal( arg1, arg2 ))
        {
          if( (nl->IsEqual( arg1, "ubool" ) )   ||
              (nl->IsEqual( arg1, "uint" ) )    ||
              (nl->IsEqual( arg1, "ureal" ) )   ||
              (nl->IsEqual( arg1, "upoint" ) )  ||
              (nl->IsEqual( arg1, "ustring" ) ) ||
              (nl->IsEqual( arg1, "uregion" ) ) ||
              (nl->IsEqual( arg1, "upoint" ) ) )
            return nl->SymbolAtom( "bool" );
        }

    }

  // Error case:
  nl->WriteToString(argstr1, arg1);
  nl->WriteToString(argstr2, arg2);
  ErrorReporter::ReportError(
    "Compare Operator (one of =, #, <, >, , <=, >=) expects two arguments of "
    "type 'uT', where T in {bool, int, real, string, point, region}. The "
    "passed arguments have types '"+ argstr1 +"' and '"
    + argstr2 + "'.");
  return nl->SymbolAtom("typeerror");
}
/*
5.35.2 Value mapping for operator ~ComparePredicates~

template parameter ~OPType~ gives the character of the operator: 0 =, 1 #, 2 <, 3 >, 4 <=, 5 >=

*/

template<int OpType>
int TUComparePredicatedValueMap(Word* args, Word& result, int message,
                                Word& local, Supplier s)
{
  assert( (OpType>=0) && (OpType<=5));

  result = (qp->ResultStorage( s ));
  CcBool *res = (CcBool*) result.addr;
  Attribute *u1, *u2;
  int p1;

  u1 = (Attribute*)args[0].addr;
  u2 = (Attribute*)args[1].addr;

  p1 = u1->Compare(u2);

  switch (OpType)
    {
    case 0: // is_equal
      res->Set(true, (p1 == 0));
      return 0;

    case 1: // is_not_equal
      res->Set(true, (p1 != 0));
      return 0;

    case 2: // less_than
      res->Set(true, (p1 == -1));
      return 0;

    case 3: // bigger_than
      res->Set(true, (p1 == 1));
      return 0;

    case 4: // less_or_equal
      res->Set(true, (p1 < 1) );
      return 0;

    case 5: // bigger_or_equal
      res->Set(true, (p1 > -1));
      return 0;
    }

  res->Set(false, false);
  return -1; // should not happen
}



/*
5.35.3 Specification for operator ~ComparePredicates~

*/
const string TUEqSpec  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(uT uT) -> bool\n</text--->"
  "<text>_ = _</text--->"
  "<text>The operator checks if the internal ordering predicate "
  "holds for both arguments.</text--->"
  "<text>query [const ubool value ((\"2010-11-11\" "
  "\"2011-01-03\" TRUE FALSE) TRUE)] = [const ubool value "
  "((\"2011-01-01\"2012-09-17\" FALSE TRUE) TRUE)]</text--->"
  ") )";

const string TUNEqSpec  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(uT uT) -> bool\n</text--->"
  "<text>_ # _</text--->"
  "<text>The operator checks if the internal ordering predicate "
  "holds for both arguments.</text--->"
  "<text>query [const ubool value ((\"2010-11-11\" "
  "\"2011-01-03\" TRUE FALSE) TRUE)] # [const ubool value "
  "((\"2011-01-01\"2012-09-17\" FALSE TRUE) TRUE)]</text--->"
  ") )";

const string TULtSpec  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(uT uT) -> bool\n</text--->"
  "<text>_ < _</text--->"
  "<text>The operator checks if the internal ordering predicate "
  "holds for both arguments.</text--->"
  "<text>query [const ubool value ((\"2010-11-11\" "
  "\"2011-01-03\" TRUE FALSE) TRUE)] < [const ubool value "
  "((\"2011-01-01\"2012-09-17\" FALSE TRUE) TRUE)]</text--->"
  ") )";

const string TUBtSpec  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(uT uT) -> bool\n</text--->"
  "<text>_ > _</text--->"
  "<text>The operator checks if the internal ordering predicate "
  "holds for both arguments.</text--->"
  "<text>query [const ubool value ((\"2010-11-11\" "
  "\"2011-01-03\" TRUE FALSE) TRUE)] > [const ubool value "
  "((\"2011-01-01\"2012-09-17\" FALSE TRUE) TRUE)]</text--->"
  ") )";

const string TULtEqSpec  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(uT uT) -> bool\n</text--->"
  "<text>_ <= _</text--->"
  "<text>The operator checks if the internal ordering predicate "
  "holds for both arguments.</text--->"
  "<text>query [const ubool value ((\"2010-11-11\" "
  "\"2011-01-03\" TRUE FALSE) TRUE)] <= [const ubool value "
  "((\"2011-01-01\"2012-09-17\" FALSE TRUE) TRUE)]</text--->"
  ") )";

const string TUBtEqSpec  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(uT uT) -> bool\n</text--->"
  "<text>_ >= _</text--->"
  "<text>The operator checks if the internal ordering predicate "
  "holds for both arguments.</text--->"
  "<text>query [const ubool value ((\"2010-11-11\" "
  "\"2011-01-03\" TRUE FALSE) TRUE)] >= [const ubool value "
  "((\"2011-01-01\"2012-09-17\" FALSE TRUE) TRUE)]</text--->"
  ") )";

/*
5.35.4 Selection Function of operator ~ComparePredicates~

We can use Operator::SimpleSelect:

*/

/*
5.35.5 Definition of operator ~ComparePredicates~

*/
Operator temporalunitisequal
(
 "=",
 TUEqSpec,
 TUComparePredicatedValueMap<0>,
 Operator::SimpleSelect,
 TUComparePredicatesTypeMap
 );

Operator temporalunitisnotequal
(
 "#",
 TUNEqSpec,
 TUComparePredicatedValueMap<1>,
 Operator::SimpleSelect,
 TUComparePredicatesTypeMap
 );

Operator temporalunitsmaller
(
 "<",
 TULtSpec,
 TUComparePredicatedValueMap<2>,
 Operator::SimpleSelect,
 TUComparePredicatesTypeMap
 );

Operator temporalunitbigger
(
 ">",
 TUBtSpec,
 TUComparePredicatedValueMap<3>,
 Operator::SimpleSelect,
 TUComparePredicatesTypeMap
 );

Operator temporalunitsmallereq
(
 "<=",
 TULtEqSpec,
 TUComparePredicatedValueMap<4>,
 Operator::SimpleSelect,
 TUComparePredicatesTypeMap
 );

Operator temporalunitbiggereq
(
 ">=",
 TUBtEqSpec,
 TUComparePredicatedValueMap<5>,
 Operator::SimpleSelect,
 TUComparePredicatesTypeMap
 );
/*
5.36 Operator ~uint2ureal~

This operator creates an ureal value from an uint value

----
     uint --> ureal

----

*/

/*
5.36.1 Type mapping function for ~uint2ureal~

*/
ListExpr TUuint2urealTypeMap( ListExpr args )
{
  ListExpr arg1;
  string argstr1;

  if( nl->ListLength( args ) == 1 )
    {
      arg1 = nl->First( args );
      if( nl->IsEqual( arg1, "uint" ) )
        return nl->SymbolAtom("ureal");
    }

  // Error case:
  nl->WriteToString(argstr1, arg1);
  ErrorReporter::ReportError(
    "Operator uint2ureal expects an argument of type 'uint', "
    "but the passed argument has type '"+ argstr1 + "'.");
  return nl->SymbolAtom("typeerror");
}
/*
5.36.2 Value mapping for operator ~uint2ureal~

*/
int TUuint2urealValueMap(Word* args, Word& result, int message,
                                Word& local, Supplier s)
{
  result = (qp->ResultStorage( s ));
  UReal *res = (UReal*) result.addr;
  UInt *u1;

  u1 = (UInt*)args[0].addr;

  res->SetDefined(u1->IsDefined());
  if ( u1->IsDefined() )
    {
      res->timeInterval.CopyFrom(u1->timeInterval);
      res->a = 0.0;
      res->b = 0.0;
      res->c = u1->constValue.GetIntval();
      res->r = false;
    }
  return 0;
}
/*
5.36.3 Specification for operator ~uint2ureal~

*/
const string TUuint2urealSpec  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(uT uT) -> bool\n</text--->"
  "<text>uint2ureal( _ )</text--->"
  "<text>The operator creates a ureal value from an uint value.</text--->"
  "<text>query uint2ureal([const uint value ((\"2010-11-11\" "
  "\"2011-01-03\" TRUE FALSE) -5.4)])</text--->"
  ") )";

/*
5.36.4 Selection Function of operator ~uint2ureal~

We use Operator::SimpleSelect here.

*/

/*
5.36.5 Definition of operator ~uint2ureal~

*/
Operator temporalunituint2ureal
(
 "uint2ureal",
 TUuint2urealSpec,
 TUuint2urealValueMap,
 Operator::SimpleSelect,
 TUuint2urealTypeMap
 );

/*
5.37 Operator

----
      inside:
        pre +      upoint x uregion --> (stream ubool)
        pre +      upoint x    line --> (stream ubool)
        pre +      upoint x  points --> (stream ubool)
        pre +     uregion x  points --> (stream ubool)

----


*/

/*
5.37.1 Type mapping function for ~inside~

*/
ListExpr TemporalUnitInsideTypeMap( ListExpr args )
{
  ListExpr arg1, arg2;
  string argstr;

  if( nl->ListLength( args ) == 2 )
    {
      arg1 = nl->First( args );
      arg2 = nl->Second( args );

      if( nl->IsEqual( arg1, "upoint" ) && nl->IsEqual( arg2, "uregion") )
        return  nl->TwoElemList(nl->SymbolAtom( "stream" ),
                                nl->SymbolAtom( "ubool" ));
      if( nl->IsEqual( arg1, "upoint" ) && nl->IsEqual( arg2, "line") )
        return nl->TwoElemList(nl->SymbolAtom( "stream" ),
                               nl->SymbolAtom( "ubool" ));
      if( nl->IsEqual( arg1, "upoint" ) && nl->IsEqual( arg2, "points") )
        return nl->TwoElemList(nl->SymbolAtom( "stream" ),
                               nl->SymbolAtom( "ubool" ));
      if( nl->IsEqual( arg1, "uregion" ) && nl->IsEqual( arg2, "points") )
        return nl->TwoElemList(nl->SymbolAtom( "stream" ),
                               nl->SymbolAtom( "ubool" ));
    }

  // Error case:
  nl->WriteToString(argstr, args);
  ErrorReporter::ReportError(
    "Operator inside expects a list of length two with a certain signature. "
    "But it gets '" + argstr + "'.");
  return nl->SymbolAtom("typeerror");
}

/*
5.37.2 Value mapping for operator ~inside~

*/
struct TUInsideLocalInfo
{
  bool finished;
  int  NoOfResults;
  int  NoOfResultsDelivered;
  MBool *mbool;  // Used to store results
};
// case      upoint x uregion --> (stream ubool)
int temporalUnitInside_up_ur( Word* args, Word& result, int message,
                                    Word& local, Supplier s )
{
  // This implementation uses class-function
  //      void MRegion::Inside(MPoint& mp, MBool& res)
  TUInsideLocalInfo *sli;
  Word    a0, a1;
  UPoint  *u;
  URegion *r;
  MPoint  mp_tmp(1);
  MRegion mr_tmp(1);
  const UBool* cu;

  switch( message )
    {
    case OPEN:

      if (TUA_DEBUG)
        cerr << "temporalUnitInside_up_ur: Received OPEN"
             << endl;

      sli = new TUInsideLocalInfo;
      sli->finished = true;
      sli->NoOfResults = 0;
      sli->NoOfResultsDelivered = 0;
      sli->mbool = new MBool(10);
      local = SetWord(sli);

      // initialize arguments, such that a0 always contains the upoint
      //                       and a1 the uregion
      a0 = args[0];
      a1 = args[1];
      u = (UPoint*)(a0.addr);
      r = (URegion*)(a1.addr);

      // test for definedness
      if ( !u->IsDefined() || !r->IsDefined() )
        {
          if (TUA_DEBUG)
            cerr << "  Undef arg -> Empty Result" << endl << endl;
          // nothing to do
        }
      else
        {
          mp_tmp.Clear();         // create temporary MPoint
          mp_tmp.Add(*u);
          mp_tmp.SetDefined(true);

          mr_tmp.Clear();         // create temporary MRegion
          //mr_tmp.StartBulkLoad();
          mr_tmp.AddURegion(*r);
          //mr_tmp.EndBulkLoad();
          mr_tmp.SetDefined(true);
          mr_tmp.Inside(mp_tmp, *(sli->mbool)); // get and save result;
          sli->NoOfResults = sli->mbool->GetNoComponents();
          sli->finished = (sli->NoOfResults <= 0);
          if (TUA_DEBUG)
            cerr << "  " << sli->NoOfResults << " result units" << endl << endl;
        }
      if (TUA_DEBUG)
        cerr << "temporalUnitInside_up_ur: Finished OPEN"
             << endl;
      return 0;

    case REQUEST:
      if (TUA_DEBUG)
        cerr << "temporalUnitInside_up_ur: Received REQUEST"
             << endl;

      if(local.addr == 0)
        {
          if (TUA_DEBUG)
            cerr << "temporalUnitInside_up_ur: Finished REQUEST (1)"
                 << endl;
          return CANCEL;
        }
      sli = (TUInsideLocalInfo*) local.addr;
      if(sli->finished)
        {
          if (TUA_DEBUG)
            cerr << "temporalUnitInside_up_ur: Finished REQUEST (2)"
                 << endl;
          return CANCEL;
        }
      if(sli->NoOfResultsDelivered < sli->NoOfResults)
        {
          sli->mbool->Get(sli->NoOfResultsDelivered, cu);
          result = SetWord( cu->Clone() );
          sli->NoOfResultsDelivered++;
          if (TUA_DEBUG)
            cerr << "temporalUnitInside_up_ur: "
                << "Finished REQUEST (YIELD)" << endl;
          return YIELD;
        }
      sli->finished = true;
      if (TUA_DEBUG)
        cerr << "temporalUnitInside_up_ur: Finished REQUEST (3)"
             << endl;
      return CANCEL;

    case CLOSE:

      if (TUA_DEBUG)
        cerr << "temporalUnitInside_up_ur: Received CLOSE"
             << endl;
      if (local.addr != 0)
        {
          sli = (TUInsideLocalInfo*) local.addr;
          delete sli->mbool;
          delete sli;
        }
      if (TUA_DEBUG)
        cerr << "temporalUnitInside_up_ur: Finished CLOSE"
             << endl;
      return 0;
    } // end switch

  cerr << "temporalUnitInside_up_ur: Received UNKNOWN COMMAND"
       << endl;
  return 0;
}

// case      upoint x    line --> (stream ubool)
int temporalUnitInside_up_l( Word* args, Word& result, int message,
                                    Word& local, Supplier s )
{
  cout << "\nATTENTION: temporalUnitInside_up_l "
       << "not yet implemented!" << endl;
  return 0;
}

// case      upoint x  points --> (stream ubool)
int temporalUnitInside_up_pts( Word* args, Word& result, int message,
                                    Word& local, Supplier s )
{
  cout << "\nATTENTION: temporalUnitInside_up_pts "
       << "not yet implemented!" << endl;
  return 0;
}

// case      uregion x  points --> (stream ubool)
int temporalUnitInside_ur_pts( Word* args, Word& result, int message,
                                    Word& local, Supplier s )
{
  cout << "\nATTENTION: temporalUnitInside_ur_pts "
       << "not yet implemented!" << endl;
  return 0;
}

/*
5.37.3 Specification for operator ~inside~

*/

const string  TemporalUnitInsideSpec  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" "
  "\"Example\" ) "
  "("
  "<text>"
  "(upoint  uregion) -> (stream ubool)\n"
  "(upoint     line) -> (stream ubool)*\n"
  "(upoint   points) -> (stream ubool)*\n"
  "(uregion  points) -> (stream ubool)*\n"
  "(*):  Not yet implemented</text--->"
  "<text>_ inside _</text--->"
  "<text>Returns a stream of ubool indicating, whether the first "
  "object is fully included by the second one.</text--->"
  "<text>query upoint1 inside uregion1 count</text--->"
  ") )";

/*
5.37.4 Selection Function of operator ~inside~

*/
ValueMapping temporalunitinsidemap[] =
  {
    temporalUnitInside_up_ur,
    temporalUnitInside_up_l,
    temporalUnitInside_up_pts,
    temporalUnitInside_ur_pts
  };

int temporalunitInsideSelect( ListExpr args )
{
  ListExpr arg1 = nl->First(args);
  ListExpr arg2 = nl->Second(args);
  if( nl->IsEqual( arg1, "upoint" )   &&
      nl->IsEqual( arg2, "uregion") )   return 0;
  if( nl->IsEqual( arg1, "upoint" )    &&
      nl->IsEqual( arg2, "line" ) )    return 1;
  if( nl->IsEqual( arg1, "upoint" )   &&
      nl->IsEqual( arg2, "points" ) )   return 2;
  if( nl->IsEqual( arg1, "uregion" )  &&
      nl->IsEqual( arg2, "points" ) )  return 3;

  cerr << "ERROR: Unmatched case in temporalunitInsideSelect" << endl;
  string argstr;
  nl->WriteToString(argstr, args);
  cerr << "       Argumets = '" << argstr << "'." << endl;
  return -1;
}

/*
5.37.5 Definition of operator ~inside~

*/
Operator temporalunitinside( "inside",
                             TemporalUnitInsideSpec,
                             4,
                             temporalunitinsidemap,
                             temporalunitInsideSelect,
                             TemporalUnitInsideTypeMap);

/*
5.38 Operator ~sometimes~

----

 sometimes: (       ubool) --> bool
            (stream ubool) --> bool

----

*/

/*
5.38.1 Type mapping function for ~sometimes~

*/
ListExpr TemporalUnitBoolAggrTypeMap( ListExpr args )
{
  ListExpr t;
  string argstr;

  if ( nl->ListLength( args ) == 1 )
    {
      if (nl->IsAtom(nl->First(args)))
        t = nl->First( args );
      else if (nl->ListLength(nl->First(args))==2 &&
               nl->IsEqual(nl->First(nl->First(args)), "stream"))
        t = nl->Second(nl->First(args));
      else
        {
          ErrorReporter::ReportError
            ("Operator sometimes/always/never expects a (stream ubool)"
             "or (ubool).");
          return nl->SymbolAtom( "typeerror" );
        }

      if( nl->IsEqual( t, "ubool" ) )
        return nl->SymbolAtom( "bool" );
    }
  nl->WriteToString(argstr, args);
  ErrorReporter::ReportError
    ("Operator sometimes/always/never expects a list of length one, "
     "having list structure (ubool) or (stream ubool), but it gets '"
     + argstr + "'.");
  return nl->SymbolAtom( "typeerror" );
}

/*
5.38.2 Value mapping for operator ~sometimes~

*/

int TemporalUnitSometimes_streamubool( Word* args, Word& result, int message,
                                       Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  CcBool *res = (CcBool*) result.addr;
  Word elem;
  UBool *U;
  bool found = false;

  qp->Open(args[0].addr);              // open input stream
  qp->Request(args[0].addr, elem);     // get first elem from stream
  while ( !found && qp->Received(args[0].addr) )
    {
      U = (UBool*) elem.addr;
      if ( U->IsDefined() && U->constValue.GetBoolval() )
        found = true;
      else
        qp->Request(args[0].addr, elem);
      U->DeleteIfAllowed();
    }
  qp->Close(args[0].addr); // close the stream

  // create and return the result
  res->Set( true, found );
  return 0;
}

int TemporalUnitSometimes_ubool( Word* args, Word& result, int message,
                                 Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  CcBool *res = (CcBool*) result.addr;
  UBool *U = (UBool*) args[0].addr;

  if( U->IsDefined() && U->constValue.GetBoolval())
    res->Set( true, true );
  else
    res->Set( true, false );
  return 0;
}
/*
5.38.3 Specification for operator ~sometimes~

*/

const string  TemporalUnitSometimesSpec =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" "
  "\"Example\" ) "
  "("
  "<text>"
  "ubool -> bool\n"
  "ubool -> bool\n"
  "</text--->"
  "<text>sometimes( _ )</text--->"
  "<text>Returns 'true', iff the ubool/stream of ubool is 'true'"
  "at least once, otherwise 'false'. Never returns 'undef'.</text--->"
  "<text>query sometimes(ubool1)</text--->"
  ") )";

/*
5.38.4 Selection Function of operator ~sometimes~

*/

int
TemporalUnitBoolAggrSelect( ListExpr args )
{
  ListExpr arg1 = nl->First( args );

  if (nl->IsAtom( arg1 ) )
    if( nl->SymbolValue( arg1 ) == "ubool" )
      return 0;
  if(   !( nl->IsAtom( arg1 ) )
      && ( nl->ListLength(arg1) == 2 )
      && ( TypeOfRelAlgSymbol(nl->First(arg1)) == stream ) )
    { if( nl->IsEqual( nl->Second(arg1), "ubool" ) )
        return 1;
    }
  cerr << "Problem in TemporalUnitBoolAggrSelect!";
  return -1; // This point should never be reached
}

ValueMapping TemporalUnitSometimesMap[] = {
  TemporalUnitSometimes_ubool,
  TemporalUnitSometimes_streamubool,
};

/*
5.38.5 Definition of operator ~sometimes~

*/
Operator temporalunitsometimes( "sometimes",
                                TemporalUnitSometimesSpec,
                                2,
                                TemporalUnitSometimesMap,
                                TemporalUnitBoolAggrSelect,
                                TemporalUnitBoolAggrTypeMap);

/*
5.38 Operator ~never~

----

     never: (       ubool) --> bool
            (stream ubool) --> bool

----

*/

/*
5.39.1 Type mapping function for ~never~

Using ~TemporalUnitBoolAggrTypeMap~

*/

/*
5.39.2 Value mapping for operator ~never~

*/
int TemporalUnitNever_streamubool( Word* args, Word& result, int message,
                                       Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  CcBool *res = (CcBool*) result.addr;
  Word elem;
  UBool *U;
  bool found = true;

  qp->Open(args[0].addr);              // open input stream
  qp->Request(args[0].addr, elem);     // get first elem from stream
  while ( found && qp->Received(args[0].addr) )
    {
      U = (UBool*) elem.addr;
      if ( U->IsDefined() && U->constValue.GetBoolval() )
        found = false;
      else
        qp->Request(args[0].addr, elem);
      U->DeleteIfAllowed();
    }
  qp->Close(args[0].addr); // close the stream

  // create and return the result
  res->Set( true, found );
  return 0;
}

int TemporalUnitNever_ubool( Word* args, Word& result, int message,
                                 Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  CcBool *res = (CcBool*) result.addr;
  UBool *U = (UBool*) args[0].addr;

  if( U->IsDefined() && U->constValue.GetBoolval())
    res->Set( true, false );
  else
    res->Set( true, true );
  return 0;
}
/*
5.39.3 Specification for operator ~never~

*/
const string  TemporalUnitNeverSpec =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" "
  "\"Example\" ) "
  "("
  "<text>"
  "ubool -> bool\n"
  "ubool -> bool\n"
  "</text--->"
  "<text>never( _ )</text--->"
  "<text>Returns 'true', iff the ubool/stream does never take value 'true', "
  "otherwise 'false'. Never returns 'undef'.</text--->"
  "<text>query never(ubool1)</text--->"
  ") )";

/*
5.39.4 Selection Function of operator ~never~

Using ~TemporalUnitBoolAggrSelect~

*/
ValueMapping TemporalUnitNeverMap[] = {
  TemporalUnitNever_ubool,
  TemporalUnitNever_streamubool,
};


/*
5.39.5 Definition of operator ~never~

*/
Operator temporalunitnever( "never",
                            TemporalUnitNeverSpec,
                            2,
                            TemporalUnitNeverMap,
                            TemporalUnitBoolAggrSelect,
                            TemporalUnitBoolAggrTypeMap);

/*
5.40 Operator ~always~

----

    always: (       ubool) --> bool
            (stream ubool) --> bool

----

*/

/*
5.40.1 Type mapping function for ~always~

Using ~TemporalUnitBoolAggrTypeMap~

*/

/*
5.40.2 Value mapping for operator ~always~

*/
int TemporalUnitAlways_streamubool( Word* args, Word& result, int message,
                                       Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  CcBool *res = (CcBool*) result.addr;
  Word elem;
  UBool *U;
  bool found = true;

  qp->Open(args[0].addr);              // open input stream
  qp->Request(args[0].addr, elem);     // get first elem from stream
  while ( found && qp->Received(args[0].addr) )
    {
      U = (UBool*) elem.addr;
      if ( U->IsDefined() && !U->constValue.GetBoolval() )
        found = false;
      else
        qp->Request(args[0].addr, elem);
      U->DeleteIfAllowed();
    }
  qp->Close(args[0].addr); // close the stream

  // create and return the result
  res->Set( true, found );
  return 0;
}

int TemporalUnitAlways_ubool( Word* args, Word& result, int message,
                                 Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  CcBool *res = (CcBool*) result.addr;
  UBool *U = (UBool*) args[0].addr;

  if( U->IsDefined() && !U->constValue.GetBoolval())
    res->Set( true, false );
  else
    res->Set( true, true );
  return 0;
}
/*
5.40.3 Specification for operator ~always~

*/
const string  TemporalUnitAlwaysSpec =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" "
  "\"Example\" ) "
  "("
  "<text>"
  "ubool -> bool\n"
  "ubool -> bool"
  "</text--->"
  "<text>always( _ )</text--->"
  "<text>Returns 'false', iff the ubool/stream takes value 'true' "
  "at least once, otherwise 'true'. Never returns 'undef'.</text--->"
  "<text>query never(ubool1)</text--->"
  ") )";

/*
5.40.4 Selection Function of operator ~always~

Using ~TemporalUnitBoolAggrSelect~

*/
ValueMapping TemporalUnitAlwaysMap[] = {
  TemporalUnitAlways_ubool,
  TemporalUnitAlways_streamubool,
};

/*
5.40.5 Definition of operator ~always~

*/
Operator temporalunitalways( "always",
                             TemporalUnitAlwaysSpec,
                             2,
                             TemporalUnitAlwaysMap,
                             TemporalUnitBoolAggrSelect,
                             TemporalUnitBoolAggrTypeMap);

/*
5.41 Operator ~the_upoint~

----

   the_unit:  For T in {bool, int, string}
              point  point  instant instant bool bool --> ubool
              ipoint ipoint bool    bool              --> ubool
              real real real bool instant instant bool bool --> ureal
              iT duration bool bool --> uT
              T instant instant bool bool --> uT

----

*/

/*
5.41.1 Type mapping function for ~the_unit~

*/
ListExpr TU_TM_TheUnit( ListExpr args )
{
  string argstr;
  nl->WriteToString(argstr, args);
  // cerr << argstr << endl;
  if (argstr == "(point point instant instant bool bool)")
    return nl->SymbolAtom( "upoint" );
  if (argstr == "(ipoint ipoint bool bool)")
    return nl->SymbolAtom( "upoint" );

  if (argstr == "(real real real bool instant instant bool bool)")
    return nl->SymbolAtom( "ureal" );

  if (argstr == "(bool instant instant bool bool)")
    return nl->SymbolAtom( "ubool" );
  if (argstr == "(ibool duration bool bool)")
    return nl->SymbolAtom( "ubool" );

  if (argstr == "(int instant instant bool bool)")
    return nl->SymbolAtom( "uint" );
  if (argstr == "(iint duration bool bool)")
    return nl->SymbolAtom( "uint" );

  if (argstr == "(string instant instant bool bool)")
    return nl->SymbolAtom( "ustring" );
  if (argstr == "(istring duration bool bool)")
    return nl->SymbolAtom( "ustring" );

  ErrorReporter::ReportError
    ("Operator 'the_upoint' expects a list with structure "
     "'(point point instant instant bool bool)' or"
     "'(ipoint ipoint bool bool)'"
     ", but it gets a list of type '" + argstr + "'.");
  return nl->SymbolAtom( "typeerror" );
}

/*
5.41.2 Value mapping for operator ~the_unit~

*/
// point point instant instant bool bool --> ubool
int TU_VM_TheUnit_ppiibb(Word* args, Word& result,
                           int message, Word& local, Supplier s)
{
  result = (qp->ResultStorage( s ));
  UPoint *res   = (UPoint*)result.addr;
  Point *p1 = (Point*)args[0].addr;
  Point *p2 = (Point*)args[1].addr;
  Instant *i1 = (Instant*)args[2].addr;
  Instant *i2 = (Instant*)args[3].addr;
  CcBool *cl = (CcBool*)args[4].addr;
  CcBool *cr = (CcBool*)args[5].addr;
  bool clb, crb;

  // Test arguments for definedness
  if ( !p1->IsDefined() || !p2->IsDefined() ||
       !i1->IsDefined() || !i2->IsDefined() ||
       !cl->IsDefined() || !cr->IsDefined()    )
    res->SetDefined( false );
  clb = cl->GetBoolval();
  crb = cr->GetBoolval();

  if ( (*i1 == *i2) && (!clb || !crb) ) // illegal interval setting
    { res->SetDefined( false ); return 0; }
  if ( *i1 < *i2 ) // sort instants
  {
    Interval<Instant> interval( *i1, *i2, clb, crb );
    *res = UPoint( interval, *p1, *p2 );
  }
  else
  {
    Interval<Instant> interval( *i2, *i1, clb, crb );
    *res = UPoint( interval, *p2, *p1 );
  }
  return 0;
}

// ipoint ipoint bool bool --> upoint
int TU_VM_TheUnit_ipipbb(Word* args, Word& result,
                           int message, Word& local, Supplier s)
{
  result = (qp->ResultStorage( s ));
  UPoint *res   = (UPoint*)result.addr;
  Intime<Point> *ip1 = (Intime<Point> *)args[0].addr;
  Intime<Point> *ip2 = (Intime<Point> *)args[1].addr;
  CcBool *cl = (CcBool*)args[2].addr;
  CcBool *cr = (CcBool*)args[3].addr;
  bool clb, crb;

  // Test arguments for definedness
  if ( !ip1->IsDefined() || !ip2->IsDefined() ||
       !cl->IsDefined() || !cr->IsDefined()    )
    res->SetDefined( false );
  clb = cl->GetBoolval();
  crb = cr->GetBoolval();

  if ( (ip1->instant == ip2->instant) && (!clb || !crb) )
    // illegal interval setting
    { res->SetDefined( false ); return 0; }
  if ( ip1->instant < ip2->instant ) // sort instants
  {
    Interval<Instant> interval( ip1->instant, ip2->instant, clb, crb );
    *res = UPoint( interval, ip1->value, ip2->value );
  }
  else
  {
    Interval<Instant> interval( ip2->instant, ip1->instant, clb, crb );
    *res = UPoint( interval, ip2->value, ip1->value );
  }
  return 0;
}

// real real real bool instant instant bool bool -> ubool
int TU_VM_TheUnit_rrrbiibb(Word* args, Word& result,
                           int message, Word& local, Supplier s)
{
  result = (qp->ResultStorage( s ));
  UReal *res   = (UReal*)result.addr;
  CcReal *a = (CcReal*)args[0].addr;
  CcReal *b = (CcReal*)args[1].addr;
  CcReal *c = (CcReal*)args[2].addr;
  CcBool *r = (CcBool*)args[3].addr;
  Instant *i1 = (Instant*)args[4].addr;
  Instant *i2 = (Instant*)args[5].addr;
  CcBool *cl = (CcBool*)args[6].addr;
  CcBool *cr = (CcBool*)args[7].addr;
  bool clb, crb;

  // Test arguments for definedness
  if ( !a->IsDefined() || !b->IsDefined() ||
       !c->IsDefined() || !r->IsDefined() ||
       !i1->IsDefined() || !i2->IsDefined() ||
       !cl->IsDefined() || !cr->IsDefined()    )
    res->SetDefined( false );
  clb = cl->GetBoolval();
  crb = cr->GetBoolval();

  if ( (*i1 == *i2) && (!clb || !crb) ) // illegal interval setting
    { res->SetDefined( false ); return 0; }
  if ( *i1 < *i2 ) // sort instants
  {
    Interval<Instant> interval( *i1, *i2, clb, crb );
    *res = UReal( interval, a->GetRealval(), b->GetRealval(),
                            c->GetRealval(), r->GetBoolval() );
  }
  else
  {
    Interval<Instant> interval( *i2, *i1, clb, crb );
    *res = UReal( interval, a->GetRealval(), b->GetRealval(),
                            c->GetRealval(), r->GetBoolval() );
  };
  return 0;
}

// template for constant unit types:
// iT duration bool bool -> uT
template<class T>
int TU_VM_TheUnit_iTdbb(Word* args, Word& result,
                        int message, Word& local, Supplier s)
{
  result = (qp->ResultStorage( s ));
  ConstTemporalUnit<T> *res   = (ConstTemporalUnit<T> *)result.addr;
  Intime<T> *ip = (Intime<T> *)args[0].addr;
  DateTime *dur = (DateTime*) args[1].addr;
  CcBool *cl = (CcBool*)args[2].addr;
  CcBool *cr = (CcBool*)args[3].addr;
  bool clb, crb;

  // Test arguments for definedness
  if ( !ip->IsDefined() || !dur->IsDefined() ||
       !cl->IsDefined() || !cr->IsDefined()    )
    res->SetDefined( false );
  clb = cl->GetBoolval();
  crb = cr->GetBoolval();

  assert(dur->GetType() == durationtype);
  if ( (*dur == DateTime(0,0,durationtype)) && (!clb || !crb) )
    // illegal interval setting
    { res->SetDefined( false ); return 0; }
  Interval<Instant> interval( ip->instant, ip->instant+(*dur), clb, crb );
  *res = ConstTemporalUnit<T>( interval, ip->value );
  return 0;
}


// template for constant unit types:
// T instant instant bool bool -> uT
template<class T>
int TU_VM_TheUnit_Tiibb(Word* args, Word& result,
                        int message, Word& local, Supplier s)
{
  result = (qp->ResultStorage( s ));
  ConstTemporalUnit<T> *res   = (ConstTemporalUnit<T> *)result.addr;
  T       *value = (T*) args[0].addr;
  Instant *i1    = (DateTime*) args[1].addr;
  Instant *i2    = (DateTime*) args[2].addr;
  CcBool  *cl    = (CcBool*)args[3].addr;
  CcBool  *cr    = (CcBool*)args[4].addr;
  bool clb, crb;

  // Test arguments for definedness
  if ( !value->IsDefined() ||
       !i1->IsDefined() || !i2->IsDefined() ||
       !cl->IsDefined() || !cr->IsDefined()    )
    res->SetDefined( false );
  clb = cl->GetBoolval();
  crb = cr->GetBoolval();

  if ( (*i1 == *i2) && (!clb || !crb) ) // illegal interval setting
    { res->SetDefined( false ); return 0; }
  if ( *i1 < *i2 ) // sort instants
  {
    Interval<Instant> interval( *i1, *i2, clb, crb );
    *res = ConstTemporalUnit<T>( interval, *value );
  }
  else
  {
    Interval<Instant> interval( *i2, *i1, clb, crb );
    *res = ConstTemporalUnit<T>( interval, *value );
  }
  return 0;

}

/*
5.41.3 Specification for operator ~the_unit~

*/
const string  TU_Spec_TheUnit =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" "
  "\"Example\" ) "
  "("
  "<text>For T in {bool, int, string}:\n"
  "point  point  instant instant bool bool --> ubool\n"
  "ipoint ipoint bool    bool              --> ubool\n"
  "real real real bool instant instant bool bool --> ureal\n"
  "iT duration bool bool --> uT\n"
  "T instant instant bool bool --> uT"
  "</text--->"
  "<text>the_unit( pstart, pend, tstart, tend, cl, cr )\n"
  "the_unit( ip1, ip2, cl, cr )</text--->"
  "<text>Creates a unit value from the argument list. The instants/ipoints"
  "will be sorted automatically.</text--->"
  "<text>query the_unit(point1, point2, instant1, instant2, bool1, bool2)"
  "</text--->"
  ") )";

/*
5.41.4 Selection Function of operator ~the_unit~

*/
int TU_Select_TheUnit( ListExpr args )
{
  string argstr;
  nl->WriteToString(argstr, args);

  if (argstr == "(point point instant instant bool bool)")
    return 0;
  if (argstr == "(ipoint ipoint bool bool)")
    return 1;

  if (argstr == "(real real real bool instant instant bool bool)")
    return 2;

  if (argstr == "(bool instant instant bool bool)")
    return 3;
  if (argstr == "(ibool duration bool bool)")
    return 4;

  if (argstr == "(int instant instant bool bool)")
    return 5;
  if (argstr == "(iint duration bool bool)")
    return 6;

  if (argstr == "(string instant instant bool bool)")
    return 7;
  if (argstr == "(istring duration bool bool)")
    return 8;

  return -1; // should not be reached!
}

ValueMapping TU_VMMap_TheUnit[] =
  {
    TU_VM_TheUnit_ppiibb,
    TU_VM_TheUnit_ipipbb,
    TU_VM_TheUnit_rrrbiibb,
    TU_VM_TheUnit_Tiibb<CcBool>,
    TU_VM_TheUnit_iTdbb<CcBool>,
    TU_VM_TheUnit_Tiibb<CcInt>,
    TU_VM_TheUnit_iTdbb<CcInt>,
    TU_VM_TheUnit_Tiibb<CcString>,
    TU_VM_TheUnit_iTdbb<CcString>
  };
/*
5.41.5 Definition of operator ~the_unit~

*/
Operator temporalunittheupoint( "the_unit",
                            TU_Spec_TheUnit,
                            9,
                            TU_VMMap_TheUnit,
                            TU_Select_TheUnit,
                            TU_TM_TheUnit);
/*
5.41 Operator ~~

----
     (insert signature here)

----

*/

/*
5.42.1 Type mapping function for ~~

*/

/*
5.42.2 Value mapping for operator ~~

*/

/*
5.42.3 Specification for operator ~~

*/

/*
5.42.4 Selection Function of operator ~~

*/

/*
5.42.5 Definition of operator ~~

*/

/*
5.41 Operator ~~

----
     (insert signature here)

----

*/

/*
5.43.1 Type mapping function for ~~

*/

/*
5.43.2 Value mapping for operator ~~

*/

/*
5.43.3 Specification for operator ~~

*/

/*
5.43.4 Selection Function of operator ~~

*/

/*
5.43.5 Definition of operator ~~

*/

/*
5.41 Operator ~~

----
     (insert signature here)

----

*/

/*
5.44.1 Type mapping function for ~~

*/

/*
5.44.2 Value mapping for operator ~~

*/

/*
5.44.3 Specification for operator ~~

*/

/*
5.44.4 Selection Function of operator ~~

*/

/*
5.44.5 Definition of operator ~~

*/


/*
6 Creating the Algebra

*/

class TemporalUnitAlgebra : public Algebra
{
public:
  TemporalUnitAlgebra() : Algebra()
  {
    AddOperator( &temporalunitmakemvalue );
    AddOperator( &temporalunitqueryrect2d );
    AddOperator( &temporalunitpoint2d );
    AddOperator( &temporalcircle );
    AddOperator( &temporalmakepoint );
    AddOperator( &temporalunitisempty );
    AddOperator( &temporalunitdeftime );
    AddOperator( &temporalunitpresent );
    AddOperator( &temporalunitinitial );
    AddOperator( &temporalunitfinal );
    AddOperator( &temporalunitatinstant );
    AddOperator( &temporalunitatperiods );
    AddOperator( &temporalunitat );
    AddOperator( &temporalunitatmax );
    AddOperator( &temporalunitatmin );
    AddOperator( &temporalunitintersection );
    AddOperator( &temporalunitinside );
    AddOperator( &temporalunitpasses );
    AddOperator( &temporalunitget_duration );
    AddOperator( &temporalunittrajectory );
    AddOperator( &temporalunitdistance );
    AddOperator( &temporalspeed );
    AddOperator( &temporalvelocity );
    AddOperator( &temporalderivable );
    AddOperator( &temporalderivative );
    AddOperator( &temporalunitnocomponents );
    AddOperator( &temporalunitnot );
    AddOperator( &temporalunitand );
    AddOperator( &temporalunitor );
    AddOperator( &temporalunitsometimes );
    AddOperator( &temporalunitnever );
    AddOperator( &temporalunitalways );
    AddOperator( &temporalunitisequal );
    AddOperator( &temporalunitisnotequal );
    AddOperator( &temporalunitsmaller );
    AddOperator( &temporalunitbigger );
    AddOperator( &temporalunitsmallereq );
    AddOperator( &temporalunitbiggereq );
    AddOperator( &temporalunituint2ureal );
    AddOperator( &temporalunittheupoint );
  }
  ~TemporalUnitAlgebra() {};
};

TemporalUnitAlgebra temporalUnitAlgebra;

/*
7 Initialization

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
InitializeTemporalUnitAlgebra( NestedList* nlRef, QueryProcessor* qpRef )
{
  nl = nlRef;
  qp = qpRef;
  return (&temporalUnitAlgebra);
}


