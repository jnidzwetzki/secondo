/*
//paragraph	[10]	title:		[{\Large \bf ] [}]
//paragraph	[11]	title:		[{\large \bf ] [}]
//paragraph	[12]	title:		[{\normalsize \bf ] [}]
//paragraph	[21]	table1column:	[\begin{quote}\begin{tabular}{l}]	[\end{tabular}\end{quote}]
//paragraph	[22]	table2columns:	[\begin{quote}\begin{tabular}{ll}]	[\end{tabular}\end{quote}]
//paragraph	[23]	table3columns:	[\begin{quote}\begin{tabular}{lll}]	[\end{tabular}\end{quote}]
//paragraph	[24]	table4columns:	[\begin{quote}\begin{tabular}{llll}]	[\end{tabular}\end{quote}]
//[--------]	[\hline]
//characters	[1]	verbatim:	[$]	[$]
//characters	[2]	formula:	[$]	[$]
//characters    [3]    capital:    [\textsc{]    [}]
//characters	[4]	teletype:	[\texttt{]	[}]
//[ae] [\"a]
//[oe] [\"o]
//[ue] [\"u]
//[ss] [{\ss}]
//[<=] [\leq]
//[#]  [\neq]
//[tilde] [\verb|~|]

1 Header File: Attribute

May 1998 Stefan Dieker

April 2002 Ulrich Telle Adjustments for the new Secondo version

Oct 2004 M. Spiekermann. Adding some more detailed documentation and some 
thoughts about redesign and performance. 

1.1 Overview

Classes implementing attribute data types have to be subtypes of class
attribute. Whatever the shape of such derived attribute classes might be,
their instances can be aggregated and made persistent via instances of class
~Tuple~, while the user is (almost) not aware of the additional management
actions arising from persistence.

1.1 Class "Attribute"[1]

The class ~Attribute~ defines several pure virtual methods which every
derived attribute class must implement.

*/
#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include "SecondoSystem.h"
#include "NestedList.h"
#include "AlgebraManager.h"
#include "FLOB.h"
#include "TupleElement.h"

class Attribute : public TupleElement
{
public:
  virtual Attribute* Clone()     = 0;
/*
Warning: The simple implementation 

----
X::X(const X&) { // do a deep copy here }
X::Clone() { return new X(*this); }
----

does only work correctly if the copy constructor is implemented otherwise 
the compiler will not complain and replaces the call by a default implementation
returning just the this pointer, hence no new object will be created.

*/
  virtual bool IsDefined() const = 0;
  virtual void SetDefined(bool defined) = 0;
  virtual int Compare( Attribute *attrib ) = 0;
/*
This function should define an order on the attribute values. 
Return values are 0: for equal, -1: this < attrib, and 1: this > attrib. 
The implementaion must also consider that values may be undefined. 

*/
  virtual bool Adjacent( Attribute *attrib ) = 0;

/*

However, TupleElement is the base class for the hierachy

----
TupleElement -> Attribute -> StandardAttribute -> IndexableStandardAttribute
----

In a future redesign this should be structured more simple. Moreover Polymorphism
is known to be a performance brake since the derived classes need to store a pointer 
to a table of functions. Moreover, this pointer is also saved in our standard 
mechanism of making attributes persistent on disk. For small data types like int this
(and the defined flag) blows up the size dramatically (12 bytes instead of 4).  

Below theres a proposal for a simple redesign concerning the defined status of 
an attribute:

*/
#ifdef COMPILE_NEW_IDEAS

  inline const bool& IsDefined() const { return &defined };
  inline void SetDefined(){ defined = true; };
  inline void SetUndefined(){ defined = false; };

private:
  bool defined;  

#endif

};

#endif

