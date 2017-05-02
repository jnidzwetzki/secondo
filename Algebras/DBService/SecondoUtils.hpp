/*
----
This file is part of SECONDO.

Copyright (C) 2016,
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


//[$][\$]
//[_][\_]

*/
#ifndef ALGEBRAS_DBSERVICE_SecondoUtils_HPP_
#define ALGEBRAS_DBSERVICE_SecondoUtils_HPP_

#include <string>

#include "NestedList.h"

#include "Algebras/Distributed2/ConnectionInfo.h"

namespace DBService {

class SecondoUtils {
public:
    static void readFromConfigFile(std::string& resultValue,
            const char* section,
            const char* key,
            const char* defaultValue);

    static bool executeQueryOnRemoteServer(
            distributed2::ConnectionInfo* connectionInfo,
            const std::string& query);

    static bool executeQueryOnRemoteServer(
            distributed2::ConnectionInfo* connectionInfo,
            const std::string& query,
            std::string& result);

    static bool openDatabaseOnRemoteServer(
            distributed2::ConnectionInfo* connectionInfo,
            const char* dbName);

    static bool createDatabaseOnRemoteServer(
                distributed2::ConnectionInfo* connectionInfo,
                const char* dbName);

    static bool closeDatabaseOnRemoteServer(
                distributed2::ConnectionInfo* connectionInfo);
    static bool executeQueryOnCurrentNode(const std::string& query);

    static bool adjustDatabaseOnCurrentNode(const std::string& databaseName);
    static bool createRelationOnCurrentNode(
            const std::string& queryAsString,
            std::string& errorMessage);
    static bool excuteQueryOnCurrentNode(
            const std::string& queryAsString,
            ListExpr& resultList,
            std::string& errorMessage);
    static bool createRelationFromConsumeResult(
            const std::string& relationName,
            Word& result);
private:
    static bool handleRemoteDatabase(
            distributed2::ConnectionInfo* connectionInfo,
            const std::string& action,
            const std::string& dbName);
    static bool executeQuery(
            const std::string& queryListStr,
            Word& queryResult,
            const size_t availableMemory);
};

} /* namespace DBService */

#endif /* ALGEBRAS_DBSERVICE_SecondoUtils_HPP_ */
