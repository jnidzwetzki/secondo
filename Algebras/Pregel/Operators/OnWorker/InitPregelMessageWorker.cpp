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

[1] Header File of the class ~PregelAlgebra~

November 2018, J. Mende


[TOC]

1 Overview

This file contains definitions of the members of class InitPregelMessageWorker

*/

#include <ListUtils.h>
#include <../../../Relation-C++/RelationAlgebra.h>
#include <../../../FText/FTextAlgebra.h>
#include <StandardTypes.h>
#include "InitPregelMessageWorker.h"
#include "../../MessageBroker/MessageBroker.h"
#include "../../../Stream/Stream.h"

namespace pregel {

 ListExpr InitPregelMessageWorker::typeMapping(ListExpr args) {
  FORCE_LOG
  if (!nl->IsEmpty(args)) {
   return listutils::typeError("You must provide no arguments.");
  }

  return nl->SymbolAtom(CcBool::BasicType());
 }

 int
 InitPregelMessageWorker::valueMapping(Word *, Word &result, int,
                                       Word &, Supplier s) {
  result = qp->ResultStorage(s);
  MessageBroker &broker = MessageBroker::get();

  if (!broker.serverMotherRunning()) {
   BOOST_LOG_TRIVIAL(error)
    << "Message servers aren't running yet. Call \"initPregel(...) first.\"";
   ((CcBool *) result.addr)->Set(true, false);
   return 0;
  }

  broker.expectInitMessages();

  ((CcBool *) result.addr)->Set(true, true);

  return 0;
 }

 OperatorSpec InitPregelMessageWorker::operatorSpec(
  "tuple x string -> bool",
  "#(_,_)",
  "message x attribute with slot nr -> success",
  "query initPregelMessageWorker(message, \"Dest\");"
 );

 Operator InitPregelMessageWorker::initPregelMessageWorker(
  "initPregelMessageWorker",
  InitPregelMessageWorker::operatorSpec.getStr(),
  InitPregelMessageWorker::valueMapping,
  Operator::SimpleSelect,
  InitPregelMessageWorker::typeMapping
 );
}