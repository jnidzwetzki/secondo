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

This file defines the members of class RemotePregelCommand

*/

#include "RemotePregelCommand.h"
#include "../../typedefs.h"
#include "../../Helpers/Commander.h"
#include "../../Helpers/WorkerConfig.h"
#include <StandardTypes.h>
#include <ListUtils.h>
#include "../../../FText/FTextAlgebra.h"
#include "Algebras/Distributed2/CommandLog.h"
#include "QueryRunner.h"


distributed2::CommandLog commandLog;

namespace pregel {
 ListExpr RemotePregelCommand::typeMapping(ListExpr args) {
  if (!nl->HasLength(args, 1)) {
   return listutils::typeError("You must provide 1 argument.");
  }

  auto expression = nl->First(args);

  if (!FText::checkType(expression)) {
   return listutils::typeError("second argument (command) must be a text.");
  }

  return nl->SymbolAtom(CcBool::BasicType());
 }



 QueryRunner::QueryRunner(WorkerConnection* _connection, 
                  const std::string& _query): connection(_connection),
                                              query(_query){

         runner = new boost::thread(&QueryRunner::run, this);
 }

 QueryRunner::~QueryRunner() {
     runner->join();
     delete runner;
 }

 std::string QueryRunner::getResult(){
    runner->join();
    return result;
 }

 bool QueryRunner::successful(){
    runner->join();
    return err==0;          
 }

 std::string QueryRunner::errorMessage(){
     runner->join();
     return errMsg;
 }

 void QueryRunner::run(){
   double runtime = 0;
   connection->simpleCommand(query, err, errMsg, result, false,
                             runtime, false, commandLog);
 }


 int RemotePregelCommand::valueMapping(Word *args, Word &result, int, Word &,
                                       void *s) {
  result = qp->ResultStorage(s);
  CcBool* res = static_cast<CcBool*>(result.addr);
  FText* commandText = (FText *) args[0].addr;
  if(!commandText->IsDefined()){
     res->Set(true,false);
     return 0;
  }

  std::string query = commandText->GetValue();

  const supplier<pregel::WorkerConfig> &workers =
   PregelContext::get().getWorkers();

  std::function<std::string *(std::string &)> mapper = [](std::string &result) {
    return new std::string(result);
  };


  std::vector<QueryRunner*> runners;

  for (auto worker = workers(); worker != nullptr; worker = workers()) {
     WorkerConnection* w = worker->connection;
     if(w){
         runners.push_back(new QueryRunner(w,query));
     }
  }
  
  size_t success = 0;
  size_t failed  = 0;
  std::string err;
  for(QueryRunner* qr : runners){
     if(qr->successful()){
        success++;
     } else {
        if(failed == 0){
           err = qr->errorMessage();
        }
        failed++;
     } 
     delete qr;
  }

  if(failed>0){
    cout << "command '" << query <<"' failed on " << failed 
         << " workers " << endl;
    cout << "the error on the first failed worker is " << err << endl;
    res->Set(true,false);
  } else {
    res->Set(true,true);
  }

  return 0;
 }

 OperatorSpec RemotePregelCommand::operatorSpec(
  "text -> bool",
  "# (_)",
  "This operator is an auxiliary operator of the Pregel Algebra."
  "It allows to execute an arbitrary SECONDO command on all workers."
  "It takes the command as a 'text' and returns TRUE, if the commands were "
  "valid and could be executed."
  "NOTE: Use this operator to manage your distributed data."
  "You may want to create in memory objects from persisted relations "
  "for example. This can circumvent issues that exist when opening the same "
  "database with multiple SECONDO servers as is a common source for problems "
  "when using the Distributed2Algebra in conjunction with the Pregel algebra."
  "For more information see the bachelor thesis that thematized this algebra.",
  "query remotePregelCommand('query pregelStatus();');",
  "This operator belongs to the Pregel API."
  "It may require knowledge of the system to effectively understand and "
  "use all the operators that are provided."
 );

 Operator RemotePregelCommand::remotePregelCommand(
  "remotePregelCommand",
  RemotePregelCommand::operatorSpec.getStr(),
  RemotePregelCommand::valueMapping,
  Operator::SimpleSelect,
  RemotePregelCommand::typeMapping
 );
}
