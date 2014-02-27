/*
 ----
 This file is part of SECONDO.

 Copyright (C) 2014, University in Hagen, Faculty of Mathematics and
 Computer Science, Database Systems for New Applications.

 SECONDO is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published
 by
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

 Jan Kristof Nidzwetzki

 //paragraph [1] Title: [{\Large \bf \begin{center}] [\end{center}}]
 //paragraph [10] Footnote: [{\footnote{] [}}]
 //[TOC] [\tableofcontents]

 [TOC]

 0 Overview

 1 Includes and defines

*/

#ifndef _CASSANDRA_H
#define _CASSANDRA_H

#include <string.h>
#include <iostream>

#include <cassert>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include <cql/cql.hpp>
#include <cql/cql_error.hpp>
#include <cql/cql_event.hpp>
#include <cql/cql_connection.hpp>
#include <cql/cql_session.hpp>
#include <cql/cql_cluster.hpp>
#include <cql/cql_builder.hpp>
#include <cql/cql_execute.hpp>
#include <cql/cql_result.hpp>

using namespace std;

//namespace to avoid name conflicts
namespace cassandra {

/*
2.1 Helper Classes

*/
class CassandraHelper {

public:
  
/*
2.1.1 Return true if the string matches a known
      Consistence level, false otherwise

*/
    static bool checkConsistenceLevel(string consistenceLevel) {
        if ((consistenceLevel.compare("ANY") == 0)
                || (consistenceLevel.compare("ONE") == 0)
                || (consistenceLevel.compare("QUORUM") == 0)
                || (consistenceLevel.compare("ALL") == 0)) {

            return true;
        }

        return false;
    }
    
/*
2.1.1 Converts a string into a ~cql\_consistency\_enum~

*/    
    static cql::cql_consistency_enum convertConsistencyStringToEnum
      (string consistenceLevel) {
        
        if(consistenceLevel.compare("ANY") == 0) {
          return cql::CQL_CONSISTENCY_ANY;
        }
        
        if(consistenceLevel.compare("ONE") == 0) {
          return cql::CQL_CONSISTENCY_ONE;
        }
        
        if(consistenceLevel.compare("QUORUM") == 0) {
          return cql::CQL_CONSISTENCY_QUORUM;
        }
        
        if(consistenceLevel.compare("ALL") == 0) {
          return cql::CQL_CONSISTENCY_ONE;
        }
        
        return cql::CQL_CONSISTENCY_ONE;
    }

};


/*
2.2 This class is used as return value for CQL querys
    You can use it to iteratate over the result set

*/
class CassandraResult {
  
public:
     CassandraResult(cql::cql_result_t& myResult) : result(myResult) {
     }
     
     bool hasNext() {
       return result.next();
     }
     
     void getStringValue(string &resultString, int pos) {
        result.get_string(pos, resultString);
     }
  
private:
     cql::cql_result_t& result;
};

/*
2.3 Adapter for cassandra

*/
class CassandraAdapter {

public:

/*
2.3.1 Constructor

1. Parameter the contactpoint of the cassadra cluster
2. Parameter the keyspace to use (e.g. secondo)

*/
    CassandraAdapter(string myContactpoint, string myKeyspace) 
      : contactpoint(myContactpoint), keyspace(myKeyspace) {
    
        builder = cql::cql_cluster_t::builder();
    }
    
    virtual ~CassandraAdapter() {
         disconnect();
    }
    

/*
2.3.2 Open the connection the cassandra cluster

*/
    void connect();

/*
2.3.3 Write a tuple to the cluster

1. Parameter is the unique key of the data
2. Parameter is the data
3. Parameter is the name of the relation (e.g. plz)
4. Parameter is the consistence level used for writing
5. Parameter specifies to use synchronus or asynchronus writes

*/
    void writeDataToCassandra(string key, string value, 
                              string relation, string consistenceLevel,
                              bool sync
                             );
  
/* 
2.3.4 Same as writeDataToCassandra, but with prepared statements

*/
    void writeDataToCassandraPrepared(string key, string value,
                              string relation, string consistenceLevel,
                              bool sync
                             );

/*
2.3.5 Read data rom cassandra

1. Parameter is the relation to read
2. Parameter is the consistence level used for writing

*/
    CassandraResult* readDataFromCassandra(string relation, 
                                           string consistenceLevel);

/*
2.3.6 Create a new relation in cassandra. Should be called before
      the first write request for the relation is called. Returns
      true if the relation could be created, false otherwise.

1. Parameter is the name of the relation

*/
    bool createTable(string tablename);

/*
2.3.7 Remove a relation from the cassandra cluster. Returns true if
      the relation could be successfully removed. False otherwise. 

1. Parameter is the name of the relation

*/
    bool dropTable(string tablename);
    
/*
2.3.8 Disconnect from our cassandra cluster. This method waits for
      all pending requests before the connection is closed. So the 
      call can take some time to finish.
      
*/
    void disconnect();
  
/*
2.3.9 Is the connection to the cluster open? Return true if the 
      connection is open. False otherweise.
      
*/    
    bool isConnected() {
      if(session) {
        return true;
      } else {
        return false;
      }
    }
    
protected:

/*
2.3.10 Execute the cql statement with a given consistence level synchronus

*/    
  bool executeCQLSync(string cql, cql::cql_consistency_enum consistency);

/*
2.3.11 Execute the cql statement with a given consistence level asynchronus

*/    
  bool executeCQLASync(string cql, cql::cql_consistency_enum consistency);

/*
2.3.12 Execute the given cql future and check for errors. Returns
       true if the future is executed successfully. False otherwise.
       
*/    
  bool executeCQLFutureSync(
    boost::shared_future<cql::cql_future_result_t> cqlFuture);

/*
2.3.13 Execute the given cql. Returns a future containing the
       Query result.
       
*/    
  boost::shared_future<cql::cql_future_result_t> 
     executeCQL(string cql, cql::cql_consistency_enum consistency);

/*
2.3.14 Returns a CQL statement for inserting a new row. The
       first parameter is the key, the second parameter is the
       value. The last parameter is the relation for this request.
       
*/    
  string getInsertCQL(string key, string value, string relation);

/*
2.3.15 Create a pepared statement for inserting data into the 
       relation spoecified in the first parameter.
       
*/    
  bool prepareCQLInsert(string relation, string consistenceLevel);

/*
2.3.16 Iterate over all pending futures (e.g. writes), reports
       errors and remove finished futures from our future list.
       
*/    
  void removeFinishedFutures();
  
private:
  string contactpoint;            // Our cassandra contact point
  string keyspace;                // Our keyspace
  string relation;                // Our relation
  boost::shared_ptr<cql::cql_builder_t> builder;
  boost::shared_ptr<cql::cql_cluster_t> cluster;
  boost::shared_ptr<cql::cql_session_t> session;
  
  std::vector<cql::cql_byte_t> insertCQLid;  // Query ID for prepared insert 
                                             // statement
                                             
  std::vector<boost::shared_future<cql::cql_future_result_t> > 
      pendingFutures;             // Pending futures (e.g. write requests)
};

}

#endif
