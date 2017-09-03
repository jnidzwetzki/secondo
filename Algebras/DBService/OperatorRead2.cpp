/*

1.1.1 Class Implementation

----
This file is part of SECONDO.

Copyright (C) 2017,
Faculty of Mathematics and Computer Science,
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

*/
#include "NestedList.h"
#include "StandardTypes.h"

#include "Algebras/Relation-C++/OperatorFeed.h"
#include "Algebras/Relation-C++/RelationAlgebra.h"

#include "Algebras/Distributed2/FileRelations.h"

#include "Algebras/DBService/DebugOutput.hpp"
#include "Algebras/DBService/OperatorRead2.hpp"
#include "Algebras/DBService/ReplicationUtils.hpp"
#include "Algebras/DBService/SecondoUtilsLocal.hpp"
#include "DBServiceClient.hpp"

using namespace std;

namespace DBService {

ListExpr OperatorRead2::mapType(ListExpr nestedList)
{
    print(nestedList);

    ErrorReporter::ReportError(
            "NOT IMPLEMENTED");
    return nl->TypeError();
}

int OperatorRead2::mapValue(Word* args,
                            Word& result,
                            int message,
                            Word& local,
                            Supplier s)
{
    string fileName =
            static_cast<CcString*>(args[1].addr)->GetValue();
    if(fileName.empty())
    {
        return OperatorFeed::Feed(args, result,
                message, local, s);
    }
    ffeed5Info* info = (ffeed5Info*) local.addr;
    switch(message){
    case OPEN:{
        if(info){
            delete info;
            local.addr = 0;
        }
        print("Reading tuple stream from file", fileName);
        info = new ffeed5Info(fileName);
        if(!info->isOK())
        {
            print("Could not read file");
            delete info;
            return 0;
        }
        ListExpr fileType = info->getRelType();
        if(!Relation::checkType(fileType))
        {
            delete info;
            return 0;
        }
        local.addr = info;
        return 0;
    }
    case REQUEST:
        result.addr = info ? info->next() : 0;
        return result.addr? YIELD : CANCEL;
    case CLOSE:
        if(info)
        {
            delete info;
            local.addr = 0;
        }
        return 0;
    }
    return -1;
}

} /* namespace DBService */