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
*/

/*
[1] DistributedAlgebra

March 2010 Tobias Timmerscheidt

*/

// Enth�lt die Klassen DServer, DServerManager

#ifndef H_REMOTE_H
#define H_REMOTE_H


#include "StandardTypes.h"
#include "SocketIO.h"
//#include "Profiles.h"
//#include "CSProtocol.h"
#include "zthread/Runnable.h"
#include "zthread/Thread.h"
//#include "zthread/CountedPtr.h"

using namespace std;

class DServer
{
         public:
                  DServer(string,int,string,ListExpr);
                  void Terminate();

                  void setCmd(string,ListExpr,Word*);
                  void run();
                  
                          
                  bool Multiply(int count);
                  DServer** getChilds() { return childs; }
                  void DestroyChilds();
            
                  int status;
                  int getNumChilds() { return num_childs;}
         
         private:
                  string host,name,cmd;
                  int port;
                  ListExpr type,arg;
                  Word* elements;

                  Socket* server;
         
                  Socket* cbworker;
      
                  int num_childs;
                  DServer** childs;
                  
                  bool rel_open;
};

class DServerManager
{
        public:
                DServerManager(ListExpr serverlist_n, 
                    string name_n, ListExpr type, int sizeofarray);
                ~DServerManager();
                DServer* getServerByIndex(int index);
                DServer* getServerbyID(int id);
     
               int getMultipleServerIndex(int index);
                

                ListExpr getIndexList(int id);
                ListExpr getNamedIndexList(int id);
        
                int getNoOfServers();
     
        
        private:
                
                DServer** serverlist;
                ListExpr server;
                int size;
                int array_size;
                string name;
};

class DServerExecutor : public ZThread::Runnable
{
     DServer* server;
     public:
     DServerExecutor(DServer* s) {server=s;}
     
     void run();
};

class RelationWriter : public ZThread::Runnable
{
     DServer* worker;
     Word* elements;
     ListExpr arg;
     public:
     RelationWriter(DServer* s, Word* e, ListExpr a) 
     {worker=s; elements = e; arg = a;}
     
     void run();
};
     
                  
                  
                  


#endif
