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

//Implements DServer

#include "Remote.h"
#include <sstream>
#include "SocketIO.h"
#include "Profiles.h"
#include "CSProtocol.h"

using namespace std;

void extractIds(const ListExpr,int&,int&);
string toString_d(int);

DServer::DServer(string n_host,int n_port,string n_name,ListExpr n_type)
{
        host = n_host;
        port = n_port;
        name = n_name;
        type = n_type;
        
        
        string line;
        
        server = Socket::Connect( host, toString_d(port), 
                                                Socket::SockGlobalDomain );
        if(server!=0 && server->IsOk())
        {
                iostream& iosock = server->GetSocketStream();
                csp = new CSProtocol(nl, iosock);
                getline( iosock, line );
                
                if(line=="<SecondoOk/>")
                {
                        iosock << "<Connect>" << endl << endl 
                                        << endl << "</Connect>" << endl;
                        getline( iosock, line );
                        
                        if( line == "<SecondoIntro>")
                        {
                                do{
                                        getline( iosock, line);}
                                while(line != "</SecondoIntro>");
                                        //cout << "ALLES SUPER!!!";
                        }
                        else cout << "SERVERERROR 2!!!";
                }
                else cout << "SERVERERROR!!!";
        }
        else cout << "CONNECTERROR";
        
        iostream& iosock = server->GetSocketStream();
        iosock << "<Secondo>" << endl << "1" << endl 
                << "open database distributed" << endl 
                << "</Secondo>" << endl;
        getline( iosock, line );
        
        if(line=="<SecondoResponse>")
        {
                do{
                        getline( iosock, line );
                        if (line[line.size() - 1] == '\r')
                        line.resize(line.size() - 1);
                        //cout << line << endl;
                }while(line.find("</SecondoResponse>") == string::npos);
        }
        else cout << "ERROR3";
        
}

void DServer::Terminate()
{
        if(server != 0)
        {
                iostream& iosock = server->GetSocketStream();
                iosock << "<Disconnect/>" << endl;
                server->Close();
                delete server;
                server=0;
                cout << "TERMINATED";
        }
}

void DServer::setCmd(string n_cmd, ListExpr n_arg,Word* n_array)
{
        
        cmd = n_cmd;
        arg = n_arg;
        elements = n_array;
}

void DServer::run()
{

        int arg2;
        ListExpr akt;
        
        if(cmd=="write")
        {
                int algID,typID;
                extractIds(type,algID,typID);
                string daten;
                
                
      
                do {
                if(!nl->IsAtom(arg)) akt = nl->First(arg);
                else akt = arg;
                arg2 = nl->IntValue(akt);
        
                iostream& iosock = server->GetSocketStream();
               string port =toString_d((1800+arg2)); 
               string com = "let r" + name + nl->ToString(akt) + 
                     " = " + "receiveD(h192_168_2_26,p" + port + ")";
                
                
                iosock << "<Secondo>" << endl << "1" << endl << "delete r" 
                        << name << nl->ToString(arg)  << endl << "</Secondo>" 
                        << endl;
                string line;
                getline(iosock,line);
                if(line=="<SecondoResponse>")
                        do
                        getline(iosock,line); 
                        while(line.find("</SecondoResponse") == string::npos);
                
                //else cout << "DATENFEHLER";

                //cout << com << "\n";
                iosock << "<Secondo>" << endl << "1" << endl 
                                << com << endl << "</Secondo>" << endl;
                
                
                Socket* gate = Socket::CreateGlobal( "192.168.2.26", port);

                Socket* worker = gate->Accept();

                iostream& cbsock1 = worker->GetSocketStream();
                cbsock1 << "<TYPE>" << endl << nl->ToString(type) 
                        << endl << "</TYPE>" << endl;
                getline(cbsock1,line);
                if(line!="<CLOSE>") cout << "FEHLER";
                worker->Close();delete worker;
                        
                worker = gate->Accept();
                iostream& cbsock = worker->GetSocketStream();
                
                cbsock << "<TYPE>" << endl << nl->ToString(type) 
                    << endl << "</TYPE>" << endl;

                SmiRecordFile recF(false,0);
                SmiRecord rec;
                SmiRecordId recID;
                
                recF.Open("send");// + nl->ToString(arg));
                recF.AppendRecord(recID,rec);
                size_t size = 0;
                am->SaveObj(algID,typID,rec,size,type,elements[arg2]);
                
                char* buffer = new char[size]; 
                rec.Read(buffer,size,0);
                
                cbsock << "<SIZE>" << endl << size << endl << "</SIZE>" << endl;
                
                worker->Write(buffer,size);
                
                rec.Truncate(3);
                recF.DeleteRecord(recID);
                recF.Close();

                worker->Close();delete worker;worker=0;
                gate->Close();delete gate;gate=0;
                //string line;
                getline(iosock,line);
                
                if(line=="<SecondoResponse>")
                        do
                        {getline(iosock,line);}
                        while(line.find("</SecondoResponse") == string::npos);
                
                else cout << "DATENFEHLER";
                
                if(!nl->IsAtom(arg) && !nl->IsEmpty(arg)) arg = nl->Rest(arg);
                }
                while(!nl->IsAtom(arg) && !nl->IsEmpty(arg));
                //Daten auf den Server schreiben
        }
        
        if(cmd=="read")
        {
                int algID,typID;
                extractIds(type,algID,typID);
                
                string daten;
                SmiRecordFile recF(false,0);
                do {
                if(!nl->IsAtom(arg)) akt = nl->First(arg);
                else akt = arg;
                arg2 = nl->IntValue(akt);
                        
                ListExpr ls;string port =toString_d((1300+arg2));
                
                iostream& iosock = server->GetSocketStream();
                iosock << "<Secondo>" << endl << "1" << endl 
                     << "query sendD (h192_168_2_26,p" << port << ",r" 
                        << name << nl->ToString(akt) << ")" <<  endl 
                         << "</Secondo>" << endl;
                
                     
                Socket* gate = Socket::CreateGlobal( "192.168.2.26", port);

                Socket* worker = gate->Accept();

                iostream& cbsock = worker->GetSocketStream();
                cbsock << "<TYPE>" << endl << nl->ToString(type)
                     << endl << "</TYPE>" << endl;
                             
                string line;
                getline(cbsock,line);
                
                if(line=="<SIZE>")
                {
                     getline(cbsock,line);
                     int size = atoi(line.data());
                     getline(cbsock,line);

                    
                    char* buffer = new char[size];
                    worker->Read(buffer,size);
                     
                     
                     SmiRecord rec;
                     SmiRecordId recID;
                     
                     string n = "receive";// + nl->ToString(arg);
                     recF.Open("rec");
                     recF.AppendRecord(recID,rec);
                     rec.Write(buffer,size,0);
                     
                     size_t s = 0;
                     am->OpenObj(algID,typID,rec,s,type,elements[arg2]);
                     
                     recF.DeleteRecord(recID);
                     recF.Close();
                    worker->Close();delete worker;worker=0;
                    gate->Close();delete gate;gate=0;
                     //elements[arg2].addr = buffer;

                }
                else cout << "FEHLER BEI CALLBACK";
                
               
                
                getline(iosock,line);
      
                
                if(line=="<SecondoResponse>")
                {
                        nl->ReadBinaryFrom(iosock, ls);
                        string debug_out = nl->ToString(ls);
                        //cout << debug_out;
                        
                        do
                                getline(iosock,line);
                        while(line.find("</SecondoResponse>") == string::npos);
                        
                        ls = nl->Second(nl->Fourth(ls));
                        
                        
                        ListExpr errorInfo = nl->OneElemList
                                                (nl->SymbolAtom("ERRORS"));
                        bool correct;
                        //elements[arg2] = ((am->InObj(algID,typID))
                           //             ( type, ls, 1, errorInfo, correct));
                }
                else cout << "DATENFEHLER LESEN";
                
                if(!nl->IsAtom(arg) && !nl->IsEmpty(arg)) arg = nl->Rest(arg);
                }
                while(!nl->IsAtom(arg) && !nl->IsEmpty(arg));
                //recF.Remove();
         }
        
        if(cmd=="delete")
        {
                string line;
                iostream& iosock = server->GetSocketStream();
                iosock << "<Secondo>" << endl << "1" << endl << "delete r" 
                        << name << nl->ToString(arg) << endl << "</Secondo>" 
                        << endl;
                do
                                getline(iosock,line);
                        while(line.find("</SecondoResponse>") == string::npos);
        }
        
        if(cmd=="copy")
        {
                string line;
                iostream& iosock = server->GetSocketStream();
                string to = nl->StringValue(nl->First(arg));
                ListExpr list = nl->Second(arg);
                
                while(!nl->IsAtom(list))
                {
                        string cmd;
                        cmd = "let r" + to + nl->ToString(nl->First(list))
                                        + " = r" + name 
                                        + nl->ToString(nl->First(list));
                        iosock << "<Secondo>" << endl << "1" << endl 
                                << cmd<< endl << "</Secondo>" << endl;
                        list = nl->Rest(list);
                        do
                        {getline(iosock,line); /*cout << line;*/}
                        while(line.find("</SecondoResponse>") == string::npos);
                }
                
                string cmd;
                cmd = "let r" + to + nl->ToString(list) + " = r" 
                                + name + nl->ToString(list);
                iosock << "<Secondo>" << endl << "1" << endl << cmd 
                                << endl << "</Secondo>" << endl;
                do
                {getline(iosock,line);/*cout<<line;*/}
                        while(line.find("</SecondoResponse>") == string::npos);
                        
        }
        
                
}

DServerManager::DServerManager(ListExpr serverlist_n, 
                                                      string name_n, 
                                                      ListExpr type,
                                                      int sizeofarray)
{
        array_size = sizeofarray;
        
        name = name_n;
        
        size = nl->ListLength(serverlist_n);
        if(size==-1) size=1;
        

        serverlist = new DServer*[size];
        
        ListExpr elem = nl->First(serverlist_n);
        serverlist_n = nl->Rest(serverlist_n);
        
        for(int i = 0; i<size; i++)
        {
            serverlist[i] = new DServer(nl->StringValue(nl->First(elem)),
                                                nl->IntValue(nl->Second(elem)), 
                                                        name,
                                                        type);
                if(i < size-1){elem = nl->First(serverlist_n);
                serverlist_n = nl->Rest(serverlist_n);}
        }
}

DServerManager::~DServerManager()
{
        for(int i = 0; i<size; i++)
        {
                if(serverlist[i] != 0) serverlist[i]->Terminate();
                delete serverlist[i];
        }
        
        delete serverlist;
}

DServer* DServerManager::getServerbyID(int id)
{
        return serverlist[id];
}

DServer* DServerManager::getServerByIndex(int index)
{
        return serverlist[index % size];
}

ListExpr DServerManager::getIndexList(int id)
{
        ListExpr res = nl->TheEmptyList();
        for(int i = id; i<array_size; i+=size)
        {
                res = nl->Cons(nl->IntAtom(i), res);
        }
        return res;
}
                
ListExpr DServerManager::getNamedIndexList(int id)
{
        ListExpr res = nl->TheEmptyList();
        for(int i = id; i<array_size; i+=size)
        {
                res = nl->Cons(nl->TwoElemList(nl->StringAtom(name),
                                        nl->IntAtom(i)), res);
        }
        return res;
}
        
int DServerManager::getNoOfServers() {return size;}        
        
        
                        
