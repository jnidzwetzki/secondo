
/*
---- 
This file is part of SECONDO.

Copyright (C) 2004-2009, University in Hagen, Faculty of Mathematics and Computer Science, 
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

//[&] [\&]

*/

#ifndef LISTUTILS_H
#define LISTUTILS_H


#include "NestedList.h"


using namespace std;
extern NestedList* nl;

namespace listutils{

/*
~emptyErrorInfo~

Returns a one elem lsit containing the symbol "ERROR". 

*/   
  ListExpr emptyErrorInfo(); 

/*
  
Permforms a kind check for SPATIAL?D

*/
  bool isSpatialType(ListExpr arg);


/*
Check for rect, rect3, rect4, rect8

*/
  bool isRectangle(ListExpr args);


/*
Check for map

*/
  template<int noArgs>
  bool isMap(ListExpr map){

    int a = noArgs+2;
    

    return nl->ListLength(map)==a &&
           nl->AtomType(nl->First(map))==SymbolType &&
           nl->SymbolValue(nl->First(map)) == "map";

  }


/*
Returns the symbol typeerror.

*/
  ListExpr typeError();

/*
Writes the message to the errorReporter and returns the
symbol typeerror.

*/
  ListExpr typeError(string message);


/*
Checks for a valid rtree description.

*/
  bool isRTreeDescription(ListExpr rtree);

/*
Checks for a valid btree description.

*/
  bool isBTreeDescription(ListExpr btree);

/*
Checks for a valid tuple description

*/
  bool isTupleDescription(ListExpr tuple);


/*
Checks for kind DATA

*/
  bool isDATA(ListExpr type);


/*
Checks for a numeric value 

*/
 bool isNumeric(const ListExpr num);

 bool isSymbol(const ListExpr list, const string& v);


/*
Returns the numeric value.

*/
double getNumValue(const ListExpr n);

/*
Checks for a numeric type

*/
bool isNumericType(const ListExpr n);


/*
 Checks for a valid attribute list

*/
  bool isAttrList(ListExpr attrList);


/*
Checks for disjoint attribute lists.

Precondition isAttrList(l1) [&]  isAttrList(l2)

*/
  bool disjointAttrNames(ListExpr l1, ListExpr l2);



  ListExpr getRTreeType(ListExpr rtree);


/*
Checks for a valid description of a relation

*/
  bool isRelDescription(ListExpr rel, const bool trel =false);

/*
Checks for a tuple stream

*/
  bool isTupleStream(ListExpr s);
  

/*
Checks for a stream of kind DATA

*/
  bool isDATAStream(ListExpr s);

/* 

searches for an attribute name within the list.
The attrlist has to be a valid attribute list. If the attribute was found, 
its position is returned and type is set to the corresponding attribute 
type. If not is found, type remains unchanged and the result is 0.

*/
   
  int findAttribute(ListExpr attrList, const string& name, ListExpr& type);

/*
Searches a given type within an attribute list starting at the given position.
If the type was found, its positon is returned and the name is changed to the
name of the attribute. Otherwise, 0 is returned.

*/
  int findType(ListExpr attrList, const ListExpr type, 
                string& name, int start=1);


} // end of namespace
#endif

