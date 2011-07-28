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

//paragraph [1] Title: [{\Large \bf \begin {center}] [\end {center}}]
//[TOC] [\tableofcontents]
//[_] [\_]

[1] Implementation of the OSM Algebra

June-November, 2011. Thomas Uchdorf

[TOC]

1 Overview

This implementation file contains the implementation of the class ~OsmAlgebra~.

For more detailed information see OsmAlgebra.h.

2 Defines and Includes

*/

#undef __TRACE__
//#define __TRACE__ cout <<  __FILE__ << "::" << __LINE__;
#define __TRACE__

// --- Including header-files
using namespace std;
#include "OsmAlgebra.h"
#include "AlgebraManager.h"
#include "NestedList.h"
#include "QueryProcessor.h"
#include "Symbols.h"
#include "../Spatial/Geoid.h"
#include "../Spatial/SpatialAlgebra.h"
#include "../FText/FTextAlgebra.h"
#include "ShpFileReader.h"
#include "ConnCodeFinder.h"

// --- Enabling global pointer variables
extern NestedList* nl;
extern QueryProcessor* qp;

// --- Announcing global functions from ImExAlgebra.cpp
//extern string getShpType (const string fname, bool& correct,
//                          string& errorMessage);
//template<int filePos>
//extern int shpimportVM (Word* args, Word& result, int message,
//                        Word& local, Supplier s);

// --- shpimport3-operator
// Specification of operator shpimport3
const string shpimport3Spec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "(<text> text -> stream(T), T in {point, points, line, region}</text--->"
    "<text>shpimport3(_)</text--->"
    "<text>Produces a stream of spatial objects from a shapefile. "
    "The spatial result stream element type T is determined "
    "automatically by inspecting the import file.</text--->"
    "<text> query shpimport3('kinos.shp') count</text--->) )";

// Value-mapping-function of operator shpimport3
template<int filePos>
int shpimport3ValueMap(Word* args, Word& result, int message,
        Word& local, Supplier s){
    //return shpimportVM<filePos> (args, result, message, local, s);
    ShpFileReader* reader = NULL;
    FText* fname = NULL;
    ListExpr type;
    int ret = 0;
    switch(message){
        case OPEN:
            if(local.addr){
                delete (ShpFileReader*)local.addr;
            }
            fname = static_cast<FText*>(args[filePos].addr);
            type = nl->Second(qp->GetType(s));
            local.setAddr(new ShpFileReader(type,fname));
            ret = 0;
            break;
        case REQUEST:
            if(!local.addr){
                ret = CANCEL;
            } else {
                reader = static_cast<ShpFileReader*>(local.addr);
                if(!reader){
                    ret = CANCEL;
                } else {
                    Attribute* next = reader->getNext();
                    result.addr = next;
                    ret = next?YIELD:CANCEL;
                }
            }
            break;
        case CLOSE:
            reader = static_cast<ShpFileReader*>(local.addr);
            if(reader){
                reader->close();
                delete reader;
                local.addr = 0;
            }
            ret = 0;
            break;
        default:
            ret = 0;
            break;
    }
    return ret;
}

// Type-mapping-function of operator shpimport3
ListExpr shpimport3TypeMap(ListExpr args){
   if(nl->ListLength(args)!=1){
      return listutils::typeError("one argument expected");
   }
   ListExpr arg = nl->First(args);
   if(nl->ListLength(arg) !=2){
      return listutils::typeError("Error, argument has to consists of 2 parts");
   }
   ListExpr type = nl->First(arg);
   ListExpr value = nl->Second(arg);

   if(!listutils::isSymbol(type,FText::BasicType())){
       return listutils::typeError("text expected");
   }

   // get the value if possible

   Word res;
   bool success = QueryProcessor::ExecuteQuery(nl->ToString(value),res);
   if(!success){
     return listutils::typeError("could not evaluate the value of  " +
                                  nl->ToString(value) );
   }

   FText* resText = static_cast<FText*>(res.addr);

   if(!resText->IsDefined()){
      resText->DeleteIfAllowed();
       return listutils::typeError("filename evaluated to be undefined");
   }

   string name = resText->GetValue();
   resText->DeleteIfAllowed();

   string shpType;
   bool correct;
   string errmsg;

   shpType = ShpFileReader::getShpType(name, correct, errmsg);
   if(!correct){
      return listutils::typeError(errmsg);
   }
   return nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),
                          nl->SymbolAtom(shpType));
}

// Instance of operator shpimport3
Operator shpimport3( "shpimport3",
                    shpimport3Spec,
                    shpimport3ValueMap<0>,
                    Operator::SimpleSelect,
                    shpimport3TypeMap);

// --- getconnectivitycode-operator
// Specification of operator getconnectivitycode
const string getconnectivitycodeSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "(<text> d1 x d2 x d3 x d4 x o1 x o2 x o3 x o4 -> c"
    ", d1, d2, d3, d4, c int, o1, o2, o3, o4 bool</text--->"
    "<text>getconnectivitycode(_)</text--->"
    "<text>Computes and returns the connectivity code as integer value "
    "from the directions and the one-way data of four crossing "
    "sections</text--->"
    "<text> query getconnectivitycode(sec1, sec2, sec3, sec4, ow1, ow2,"
    " ow3, ow4)</text--->))";

// Value-mapping-function of operator getconnectivitycode
int getconnectivitycodeValueMap(Word* args, Word& result, int message,
        Word& local, Supplier s){
   assert (args != NULL);
   result = qp->ResultStorage (s);
   CcInt *res = static_cast<CcInt*>(result.addr);;
   int dir[4] = {0, 0, 0, 0};
   bool ow[4] = {false, false, false, false};
   CcInt *direction = NULL;
   CcBool *oneWay = NULL;
   int iDir = 0;
   int iOw = 0;
   bool foundUndefined = false;
   for (iDir = 0; iDir < 4; ++iDir)  {
      direction = static_cast<CcInt *>(args[iDir].addr);
      if (!direction->IsDefined()) {
         foundUndefined = true;
      }
      dir[iDir] = direction->GetValue ();
   }
   for (iOw = 0; iOw < 4; ++iOw)  {
      oneWay = static_cast<CcBool *>(args[4 + iOw].addr);
      if (!oneWay->IsDefined()) {
         foundUndefined = true;
      }
      ow[iOw] = oneWay->GetValue ();
   }
   if (foundUndefined)  {
      res->SetDefined(false);
   } else  {
      res->Set (true, ConnCodeFinder::getConnectivityCode (
         dir[0],dir[1],dir[2],dir[3],ow[0],ow[1],ow[2],ow[3]));
   }
   return 0;
}

// Type-mapping-function of operator getconnectivitycode
ListExpr getconnectivitycodeTypeMap(ListExpr args){
   assert (args);
   if(nl->ListLength(args) != 8){
      return listutils::typeError("eight arguments expected");
   }
   ListExpr rest = args;
   int i = 0;
   int val = 0;
   while (!nl->IsEmpty (rest)) {
      ListExpr current = nl->First (rest);
      rest = nl->Rest (rest);
      if (nl->ListLength (current) != 2){
         return listutils::typeError("argument has to consists of 2 parts");
      }
      if (i >= 0 && i < 4)  {
         if (!listutils::isSymbol (nl->First(current), CcInt::BasicType ())) {
            return listutils::typeError("int expected");
         }
         //TODO Reconsider the following line!
         val = (nl->Second(current));
         if (val >= 0 && val < 3)  {
            return listutils::typeError("value between zero and two expected");
         }
      } else if ((i >= 4 && i < 8 &&
         !listutils::isSymbol (nl->First(current), CcBool::BasicType ()))){
         return listutils::typeError("bool expected");
      }
      ++i;
   }
   return nl->SymbolAtom(CcInt::BasicType());
}

// Instance of operator getconnectivitycode
Operator getconnectivitycode( "getconnectivitycode",
                    getconnectivitycodeSpec,
                    getconnectivitycodeValueMap,
                    Operator::SimpleSelect,
                    getconnectivitycodeTypeMap);

// --- Constructors
// Constructor
osm::OsmAlgebra::OsmAlgebra () : Algebra ()
{
    AddOperator(&shpimport3);
    shpimport3.SetUsesArgsInTypeMapping();;
    AddOperator(&getconnectivitycode);
    getconnectivitycode.SetUsesArgsInTypeMapping();;
}

// Destructor
osm::OsmAlgebra::~OsmAlgebra ()
{
    // empty
}

// --- Global initialization function
extern "C"
Algebra*
InitializeOsmAlgebra( NestedList* nlRef, QueryProcessor* qpRef )
{
  nl = nlRef;
  qp = qpRef;
  return (new osm::OsmAlgebra());
}
