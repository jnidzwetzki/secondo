/*
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


//[$][\$]
//[_][\_]

*/
#ifndef ALGEBRAS_DBSERVICE_RELATIONINFO_HPP_
#define ALGEBRAS_DBSERVICE_RELATIONINFO_HPP_

#include <map>
#include <string>
#include <vector>

// TODO use fully qualified path (need to change test makefile)
//#include "Algebras/DBService/LocationInfo.hpp"
#include "LocationInfo.hpp"

namespace DBService
{

typedef size_t ConnectionID;

class RelationInfo
{
public:
    RelationInfo(const std::string& dbName,
                 const std::string& relName,
                 const std::string& host,
                 const std::string& port,
                 const std::string& disk);
    const std::string& getDatabaseName() const;
    const std::string& getRelationName() const;
    void addNode(ConnectionID id);
    void addNodes(std::vector<ConnectionID>& nodesToAdd);
    const std::map<ConnectionID, bool>::const_iterator nodesBegin() const;
    const std::map<ConnectionID, bool>::const_iterator nodesEnd() const;
    const LocationInfo& getOriginalLocation() const;
    const std::string toString() const;
    static std::string getIdentifier(const std::string dbName,
                                     const std::string relName);
    const size_t getNodeCount();
    const ConnectionID getRandomReplicaLocation();
    void updateReplicationStatus(ConnectionID connID, bool replicated);

private:
    const std::string& databaseName;
    const std::string relationName;
    std::map<ConnectionID, bool> nodes;
    const LocationInfo originalLocation;

};

} /* namespace DBService */

#endif /* ALGEBRAS_DBSERVICE_RELATIONINFO_HPP_ */
