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

[1] Implementation of the unpin\_nodes Operator 

[TOC]

0 Overview

1 Defines and Includes

*/

#include "op_unpin_nodes.h"

#include "ListUtils.h"
#include "QueryProcessor.h"
#include "RelationAlgebra.h"
#include "TupleIdentifier.h"
#include "BTree2.h"


extern NestedList* nl;
extern QueryProcessor *qp;

namespace BTree2Algebra {
namespace Operators {

/*
2 Operator ~unpin\_nodes~

Signature is

----
    unpin\_nodes: stream(int) x (btree2) --> stream(tuple( (Node int) 
                                                  (Ok bool)))
----

2.1 TypeMapping

*/
ListExpr unpin_nodes::TypeMapping( ListExpr args){
    CHECK_COND(nl->ListLength(args) == 2,
     "Operator expects two arguments");
    NList first (nl->First(args));
    ListExpr second = nl->Second(args);
    CHECK_COND(first.hasLength(2) && first.first().isSymbol() &&
               first.first().str() == "stream" && first.second().isSymbol() 
               && first.second().str() == "int", 
               "Operator expects a stream of ints as first argument:");
      
    CHECK_COND(listutils::isBTree2Description(second),
      "Operator expects a btree2 object as second argument.");
  ListExpr attr = nl->TwoElemList(nl->TwoElemList(nl->SymbolAtom("Node"), 
                nl->SymbolAtom("int")), nl->TwoElemList(nl->SymbolAtom("Ok"), 
                  nl->SymbolAtom("bool")));
  ListExpr res = (nl->TwoElemList(nl->SymbolAtom("stream"), 
                          nl->TwoElemList(nl->SymbolAtom("tuple"),
                          attr)));
  return res; 
}       

/*
2.2 Valuemapping

*/
struct ValueMapInfo{
  BTree2* btree;
  TupleType* tType;
};

int
unpin_nodes::ValueMapping(Word* args, Word& result, int message,
        Word& local, Supplier s)
{
  CcInt* currentInt; 
  Word current;
  Tuple* tuple;
  ValueMapInfo* vmi;
  switch( message )
  {
    case OPEN: { // initialize the local storage 
      vmi = new ValueMapInfo;
      vmi->btree = (BTree2*)args[1].addr;
      ListExpr resultType = GetTupleResultType( s );
      vmi->tType = new TupleType(nl->Second(resultType));
      qp->Open(args[0].addr);
      local.addr = vmi;
      return 0;
    }
    case REQUEST: { // return the next stream element
      vmi = (ValueMapInfo*)local.addr;
      qp->Request(args[0].addr, current);
      if (qp->Received(args[0].addr))
      {
        currentInt = (CcInt*)current.addr;
        bool res = vmi->btree->removeCachePinnedNode(currentInt->GetIntval());
        
        tuple = new Tuple(vmi->tType);
        tuple->PutAttribute(0, currentInt);
        tuple->PutAttribute(1, new CcBool(true, res));
        result.setAddr(tuple);  
        return YIELD;
      }
      else
      {
        result.addr = 0;
        return CANCEL;
      }
    }
    case CLOSE: { // free the local storage
      if (local.addr)
      {
        vmi = (ValueMapInfo*)local.addr;
        vmi->tType->DeleteIfAllowed();
        delete vmi;
        local.addr = 0;
      }
      qp->Close(args[0].addr);
      return 0;
    }
    default: {
      return -1;
    }
  }
}

/*
2.3 Operator specification

*/
struct unpinNodesInfo : OperatorInfo {

  unpinNodesInfo() : OperatorInfo()
  {
    name =      "unpin_nodes";
    signature = "(stream(int)) x (btree2 Tk Td u) -> stream("
                "tuple( (Node int) (Ok bool)))"   ;
    syntax =    "_ unpin_nodes [ _ ]";
    meaning =   "Unpins the nodes with given NodeId in the btree2's cache.";
    example =   "query intstream (0, 4) unpin_nodes [staedte_btree2] consume";
  }
};

Operator unpin_nodes::def( unpinNodesInfo(), unpin_nodes::ValueMapping, 
                         unpin_nodes::TypeMapping);
}
}


