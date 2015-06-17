/*
----
This file is part of SECONDO.

Copyright (C) 2015,
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

*/


#include <iostream>
#include <vector>
#include <list>

#include "SecondoInterface.h"
#include "SecondoInterfaceCS.h"
#include "FileSystem.h"
#include "Algebra.h"
#include "NestedList.h"
#include "StandardTypes.h"
#include "FTextAlgebra.h"
#include "ListUtils.h"
#include "StringUtils.h"
#include "RelationAlgebra.h"
#include "Stream.h"
#include "NList.h"
#include "ArrayAlgebra.h"


  // use boost for thread handling

#include <boost/thread.hpp>
#include <boost/date_time.hpp>


/*
0 Workaround to the nested list parser

The nested list parser implementation used a lot of global variables.
This makes it impossible to create a nested list from a file or a 
string in parallel. Thus, we introduce a global mutex which is locked
always if a nested list is parsed.

*/
boost::mutex nlparsemtx;


#define FILE_BUFFER_SIZE 1048576



/*
Some Helper functions.

*/
class BinRelWriter{
  public:
     static bool writeHeader(ostream& out, ListExpr type){

         if(!Relation::checkType(type)){
            cerr << "invalid relation type " << nl->ToString(type);
            assert(false);
         }

         string relTypeS = nl->ToString(type);
         uint32_t length = relTypeS.length();
         string magic = "srel";
         out.write(magic.c_str(),4);
         out.write((char*) &length, sizeof(uint32_t));
         out.write(relTypeS.c_str(), length);
         return out.good();
     }

    static bool writeNextTuple(ostream& out,Tuple* tuple){
       // retrieve sizes
       size_t coreSize;
       size_t extensionSize;
       size_t flobSize;
       size_t blocksize = tuple->GetBlockSize(coreSize, extensionSize, 
                                              flobSize);
       // allocate buffer and write flob into it
       char* buffer = new char[blocksize];
       tuple->WriteToBin(buffer, coreSize, extensionSize, flobSize); 
       uint32_t tsize = blocksize;
       out.write((char*) &tsize, sizeof(uint32_t));
       out.write(buffer, tsize);
       delete[] buffer;
       return out.good();
    }
           
    static bool finish(ostream& out){
           uint32_t marker = 0;
           out.write((char*) &marker, sizeof(uint32_t));
           return out.good();
    }
};


class ffeed5Info{

  public:
    ffeed5Info(const string& filename, const ListExpr _tt){
       tt = new TupleType(_tt);
       inBuffer = new char[FILE_BUFFER_SIZE];
       in.open(filename.c_str(), ios::in | ios::binary);
       ok = in.good();
       if(ok){
          in.rdbuf()->pubsetbuf(inBuffer, FILE_BUFFER_SIZE);
          readHeader();
       }
    }

   
    ffeed5Info(const string& filename, TupleType* _tt){
       tt = new TupleType(*_tt);
       in.open(filename.c_str(), ios::in | ios::binary);
       ok = in.good();
       if(ok){
          readHeader();
       }else{
          cerr << "could not open file " << filename << endl; 
       }
    }
    
    ~ffeed5Info(){
      tt->DeleteIfAllowed();
      in.close();
      delete[] inBuffer;
    }

    Tuple* next(){
       if(!ok) {
         return 0;
       }
       if(!in.good() || in.eof()){
          return 0;
       }
       uint32_t size;
       in.read( (char*) &size, sizeof(uint32_t));
       if(size==0){
         return 0;
       }
       char* buffer = new char[size];
       in.read(buffer, size);
       if(!in.good()){
         delete [] buffer;
         return 0;
       }
       Tuple* res = new Tuple(tt);
       res->ReadFromBin(buffer );
       delete[] buffer;
       return res;
    }

  private:
     ifstream in;
     char* inBuffer;
     TupleType* tt; 
     bool ok;


  void readHeader(){
     char marker[4];
     in.read(marker,4);
     string ms(marker,4);
     if(ms!="srel"){
        ok = false;
        return;
     }
     uint32_t length;
     in.read((char*) &length,sizeof(uint32_t));
     char* buffer = new char[length];
     in.read(buffer,length);
     string list(buffer,length);
     delete[] buffer; 
     ok = in.good();
  }

};


template<class T>
bool writeVar(const T& value, SmiRecord& record, size_t& offset){
  bool res = record.Write(&value, sizeof(T), offset) == sizeof(T);
  offset += sizeof(T);
  return res;
}

template<>
bool writeVar<string>(
  const string& value, SmiRecord& record, size_t& offset){

  // write the size of the sting
  size_t len = value.length();
  if(!writeVar<size_t>(len,record,offset)){
     return false;
  }
  if(len==0){
     return true;
  }
  if(record.Write(value.c_str(),len, offset)!=len){
     return false;
  }
  offset+= len;
  return true;
}

template<class T>
bool readVar(T& value, SmiRecord& record, size_t& offset){
  bool res = record.Read(&value, sizeof(T), offset) == sizeof(T);
  offset += sizeof(T);
  return res;
}

template<>
bool readVar<string>(string& value, SmiRecord& record, size_t& offset){
  size_t len;
  if(!readVar<size_t>(len,record,offset)){
    return false;
  }
  if(len==0){
     value = "";
     return true;
  }
  char* cstr = new char[len];
  bool res = record.Read(cstr, len, offset) == len;
  offset += len;
  value.assign(cstr,len);
  delete[] cstr;
  return res;
}


/*
1.0.1 ~rewriteQuery~

This function replaces occurences of [$]<Ident> within the string orig
by corresponding const expressions. If one Ident does not represent
a valid object name, the result will be false and the string is
unchanged.

*/

string rewriteRelType(ListExpr relType){
  if(!Relation::checkType(relType)){
     return nl->ToString(relType);
  }

  ListExpr attrList = nl->Second(nl->Second(relType));

  stringstream ss;
  ss << "rel(tuple([";
  bool first = true;
  while(!nl->IsEmpty(attrList)){
     if(!first){
       ss << ", ";
     } else {
        first = false;
     }
     ListExpr attr = nl->First(attrList);
     attrList = nl->Rest(attrList);
     ss << nl->SymbolValue(nl->First(attr));
     ss << " : ";
     ss << nl->ToString(nl->Second(attr));
  }
  ss << "]))";
  return ss.str();
}

bool getConstEx(const string& objName, string& result){

   SecondoCatalog* ctlg = SecondoSystem::GetCatalog();
   if(!ctlg->IsObjectName(objName)){
       return false;
   }
   result = " [const " + rewriteRelType(ctlg->GetObjectTypeExpr(objName))
          + " value " + nl->ToString(ctlg->GetObjectValue(objName)) + "] ";

   return true;
}


bool rewriteQuery(const string& orig, string& result){

  stringstream out;
  size_t pos = 0;
  size_t f = orig.find_first_of("$", pos);
  map<string,string> used;
  while(f!=string::npos){
     size_t f2 = f +1;
     if(f2 < orig.length()){
        if(stringutils::isLetter(orig[f2])){
           if(f>pos){
              out << orig.substr(pos , f - pos);
           }
           stringstream ident;
           ident << orig[f2];
           f2++;
           while(f2<orig.length() && (stringutils::isLetter(orig[f2]) || 
                 stringutils::isDigit(orig[f2] || (orig[f2]=='_')))){
              ident  << orig[f2];
              f2++;
           }
           string constEx;
           if(used.find(ident.str())!=used.end()){
             constEx = used[ident.str()];
           }  else {
              if(!getConstEx(ident.str(),constEx)){
                  result =  orig;
                  return false;
              }
           }
           out << constEx; 
           pos = f2;
        } else if(orig[f2]=='$'){ // masked dollar
            out <<  orig.substr(pos,f2-pos);
            pos = f2+1;
        } else { // not a indent after $
            out <<  orig.substr(pos,f2+1-pos);
            pos = f2;
           pos++;
        } 
     } else { // end of 
       out << orig.substr(pos,string::npos);
       pos = f2;
     }
     f = orig.find_first_of("$", pos);
  }
   out << orig.substr(pos,string::npos);
   result = out.str();
  return true;
}








/*
1 Class Connection

This class represents a connection to a remote Secondo Server.

*/

class ConnectionInfo{

   public:
     ConnectionInfo(const string& _host, const int _port,
                    const string& _config, SecondoInterfaceCS* _si,
                    NestedList* _mynl):
      host(_host), port(_port), config(_config), si(_si){
          mynl = _mynl;
          serverPID = 0;
      }

      ~ConnectionInfo(){
          simtx.lock();
          si->Terminate();
          simtx.unlock();
          delete si;
          delete mynl;
       }

       string getHost() const{
          return host;
       }

       int getPort() const{
          return port;
       }
       string getConfig() const{
          return config;
       }
    
       bool check() {
          ListExpr res;
          string cmd = "list databases";
          SecErrInfo err;
          simtx.lock();
          si->Secondo(cmd,res,err);
          simtx.unlock();
          return err.code==0;
       }

       void simpleCommand(string command1, int& err, string& result, 
                          bool rewrite){
          string command;
          if(rewrite){
            rewriteQuery(command1,command);
          } else {
            command = command1;
          }
          SecErrInfo serr;
          ListExpr resList;
          simtx.lock();
          si->Secondo(command, resList, serr);
          err = serr.code;
          if(err==0){
             result = mynl->ToString(resList);
          } else {
             result = si->GetErrorMessage(err);
          }
          simtx.unlock();
       }


      bool cleanUp() {
          string command = "query getcatalog() "
                           "filter[.ObjectName startsWith \"TMP_\"] "
                           "extend[OK : deleteObject(.ObjectName)] "
                           " count";
          int err;
          string res;
          simpleCommand(command, err, res, false);
          return err==0;
      }



       bool switchDatabase(const string& dbname, bool createifnotexists){
          boost::lock_guard<boost::mutex> guard(simtx);;
          // close database ignore errors
          SecErrInfo serr;
          ListExpr resList;
          si->Secondo("close database", resList, serr);
          // create database ignore errors
          if(createifnotexists){
              si->Secondo("create database " + dbname, resList, serr);
          }
          // open database 
          si->Secondo("open database "+ dbname, resList, serr);
          bool res = serr.code==0;   
          return res;
       }


       void simpleCommand(const string& command1, int& error, string& errMsg, 
                          string& resList, const bool rewrite){

          string command;
          if(rewrite){
            rewriteQuery(command1,command);
          } else {
            command = command1;
          }
          boost::lock_guard<boost::mutex> guard(simtx);;
          SecErrInfo serr;
          ListExpr myResList = mynl->TheEmptyList();
          si->Secondo(command, myResList, serr);
          error = serr.code;
          errMsg = serr.msg;

          resList = mynl->ToString(myResList);
          mynl->Destroy(myResList);
       }
       
       void simpleCommandFromList(const string& command1, int& error, 
                                  string& errMsg, string& resList, 
                                  const bool rewrite){

          string command;
          if(rewrite){
            rewriteQuery(command1,command);
          } else {
            command = command1;
          }
          boost::lock_guard<boost::mutex> guard(simtx);
          ListExpr cmd = mynl->TheEmptyList();
          nlparsemtx.lock();
          if(! mynl->ReadFromString(command,cmd)){
              nlparsemtx.unlock();
              error = 3;
              errMsg = "error in parsing list";
              return;
          }
          nlparsemtx.unlock();

          SecErrInfo serr;
          ListExpr myResList = mynl->TheEmptyList();
          si->Secondo(cmd, myResList, serr);
          error = serr.code;
          errMsg = serr.msg;

          resList = mynl->ToString(myResList);
          if(mynl->AtomType(cmd)!=NoAtom && !mynl->IsEmpty(cmd)){
              mynl->Destroy(cmd);
          }
          if(mynl->AtomType(myResList)!=NoAtom && !mynl->IsEmpty(myResList)){
              mynl->Destroy(myResList);
          }
          
          
       }


       void simpleCommand(const string& command1, int& error, string& errMsg, 
                          ListExpr& resList, const bool rewrite){


          string command;
          if(rewrite){
            rewriteQuery(command1,command);
          } else {
            command = command1;
          }
          boost::lock_guard<boost::mutex> guard(simtx);;
          SecErrInfo serr;
          ListExpr myResList = mynl->TheEmptyList();
          si->Secondo(command, myResList, serr);
          error = serr.code;
          errMsg = serr.msg;
          // copy resultlist from local nested list to global nested list
          static boost::mutex copylistmutex;
          copylistmutex.lock();
          assert(mynl!=nl);
          resList =  mynl->CopyList(myResList, nl);
          mynl->Destroy(myResList);
          copylistmutex.unlock();
       }

        int serverPid(){
           boost::lock_guard<boost::mutex> guard(simtx);
           if(serverPID==0){
             serverPID = si->getPid(); 
           }
           return serverPID;
        }


        int sendFile( const string& local, const string& remote, 
                      const bool allowOverwrite){
          simtx.lock();
          int res =  si->sendFile(local,remote, allowOverwrite);
          simtx.unlock();
          return res;
        }
        

        int requestFile( const string& remote, const string& local,
                         const bool allowOverwrite){
          simtx.lock();
          int res =  si->requestFile(remote, local, allowOverwrite);
          simtx.unlock();
          return res;
        }

        string getRequestFolder(){
          simtx.lock();
          string res =  si->getRequestFileFolder();
          simtx.unlock();
          return res;
        }
        
        string getSendFolder(){
          simtx.lock();
          string res =  si->getSendFileFolder();
          simtx.unlock();
          return res;
        }

        static ConnectionInfo* createConnection(const string& host, 
                                   const int port, string& config){

              NestedList* mynl = new NestedList();
              SecondoInterfaceCS* si = new SecondoInterfaceCS(true,mynl);
              string user="";
              string passwd = "";
              string errMsg;
              si->setMaxAttempts(4);
              si->setTimeout(1);
              if(! si->Initialize(user, passwd, host, 
                       stringutils::int2str(port), config, 
                       errMsg, true)){
                  delete si;
                  delete mynl;
                  return 0;
             } else {
                  return  new ConnectionInfo(host, port, config, si, mynl);
             }
         }

         bool createOrUpdateObject(const string& name, 
                                   ListExpr typelist, Word& value){
              if(Relation::checkType(typelist)){
                 return createOrUpdateRelation(name, typelist, value);
              }

              boost::lock_guard<boost::mutex> guard(simtx);
              SecErrInfo serr;
              ListExpr resList;
              si->Secondo("delete " + name, resList, serr);
              // ignore error (object must not exist)
              string filename = name + "_" + 
                                stringutils::int2str(WinUnix::getpid())
                                 + ".obj";
              storeObjectToFile(name, value, typelist, filename);
              string cmd = "restore " + name + " from '" + filename + "'";
              si->Secondo(cmd, resList, serr);
              FileSystem::DeleteFileOrFolder(filename);
              return serr.code==0;
         }


         bool createOrUpdateRelation(const string& name, ListExpr typeList,
                                     Word& value){

             // write relation to a file
             string filename = name + "_" 
                               + stringutils::int2str(WinUnix::getpid()) 
                               + ".bin";
             if(!saveRelationToFile(typeList, value, filename)){
                return false;
             }
             // restore remote relation from local file
             bool ok = createOrUpdateRelationFromBinFile(name,filename); 
             // remove temporarly file
             FileSystem::DeleteFileOrFolder(filename);
             return ok;
         }

         bool createOrUpdateRelationFromBinFile(const string& name, 
                                                const string& filename,
                                                const bool allowOverwrite=true){
             boost::lock_guard<boost::mutex> guard(simtx);

             SecErrInfo serr;
             ListExpr resList;
             // transfer file to remote server
             int error = si->sendFile(filename,filename,true);
             if(error!=0){
                return false;
             } 

             // retrieve folder from which the filename can be read
             string sendFolder = si->getSendFilePath();
             
             string rfilename = sendFolder+"/"+filename;
             // delete existing object

             string cmd = "delete " + name;
             if(allowOverwrite){
                si->Secondo(cmd, resList, serr);
             }
       
             cmd = "let " + name + " =  ffeed5('" 
                          + rfilename + "') consume ";

             si->Secondo(cmd, resList, serr);

             bool ok = serr.code == 0;


             cmd = "query removeFile('"+rfilename+"')";

             si->Secondo (cmd,resList,serr);


             return ok;
         }

         bool saveRelationToFile(ListExpr relType, Word& value, 
                                 const string& filename){
            ofstream out(filename.c_str(),ios::out|ios::binary);
            char* buffer = new char[FILE_BUFFER_SIZE];
            out.rdbuf()->pubsetbuf(buffer, FILE_BUFFER_SIZE);
            if(!out.good()){
               delete[] buffer;
               return false;
            }
            if(!BinRelWriter::writeHeader(out,relType)){
               delete[] buffer;
               return false;
            }
            Relation* rel = (Relation*) value.addr;
            GenericRelationIterator* it = rel->MakeScan();
            bool ok = true;
            Tuple* tuple=0;
            while(ok && ((tuple=it->GetNextTuple())!=0)){
               ok = BinRelWriter::writeNextTuple(out,tuple);
               tuple->DeleteIfAllowed();
            }
            delete it;
            BinRelWriter::finish(out);
            out.close();
            delete[] buffer;
            return ok;
         }

         bool storeObjectToFile( const string& objName, Word& value, 
                                 ListExpr typeList, 
                                  const string& fileName){
               SecondoCatalog* ctl = SecondoSystem::GetCatalog();
               ListExpr valueList = ctl->OutObject(typeList, value);
               ListExpr objList = nl->FiveElemList(
                                     nl->SymbolAtom("OBJECT"),
                                     nl->SymbolAtom(objName),
                                     nl->TheEmptyList(),
                                     typeList,
                                     valueList);
              return nl->WriteToFile(fileName, objList);
         }        


          bool retrieve(const string& objName, ListExpr& resType, Word& result, 
                        bool checkType){
              simtx.lock();
              if(Relation::checkType(resType)){
                  simtx.unlock();
                  if(retrieveRelation(objName, resType, result)){
                     return true;
                  } else{
                     simtx.lock();
                  } 
                  cerr << "Could not use fast retrieval for a "
                       << " relation, failback" << endl;
              }
              simtx.unlock();

              // TODO: special treatment for big objects
              boost::lock_guard<boost::mutex> guard(simtx);
              SecErrInfo serr;
              ListExpr myResList;
              si->Secondo("query " + objName, myResList, serr);
              SecondoCatalog* ctlg = SecondoSystem::GetCatalog();
              if(serr.code!=0){
                 return false;
              }
              if(!mynl->HasLength(myResList,2)){
                return false;
              }
              // copy result list into global list memory
              static boost::mutex copylistmutex;
              copylistmutex.lock();
              ListExpr resList =  mynl->CopyList(myResList, nl);
              mynl->Destroy(myResList);
              copylistmutex.unlock();

              ListExpr resType2 = nl->First(resList);
              if(checkType && !nl->Equal(resType,resType2)){
                 // other type than expected
                 return false;
              }
              ListExpr value = nl->Second(resList);

              int errorPos=0;
              ListExpr errorInfo = listutils::emptyErrorInfo();
              bool correct;
              createRelMut.lock();
              result = ctlg->InObject(resType,value, errorPos, 
                                      errorInfo, correct);
              createRelMut.unlock();
              if(!correct){
                 result.addr = 0;
              }
              return correct;
          }

          bool retrieveRelation(const string& objName, 
                                 ListExpr& resType, Word& result
                                ){

             string fname1 = objName+".bin";
             if(!retrieveRelationFile(objName, fname1)){
                return false;
             }
             result = createRelationFromFile(fname1, resType);
             FileSystem::DeleteFileOrFolder(fname1);
             return true;
          }

          bool retrieveRelationFile(const string& objName,
                                    const string& fname1){
             boost::lock_guard<boost::mutex> guard(simtx);
             string rfname = si->getRequestFilePath() + "/"+fname1;
             // save the remove relation into a binary file
             string cmd = "query " + objName + " feed fconsume5['"
                          +rfname+"'] count";

             SecErrInfo serr;
             ListExpr resList;
             si->Secondo(cmd,resList,serr);
             if(serr.code!=0){
                 return false;
             }

             if(si->requestFile(fname1, fname1 ,true)!=0){
                 return false;
             }

             // delete remote file             
             cmd = "query removeFile('"+rfname+"')";
             si->Secondo(cmd,resList,serr);
             return true;
          }

          Word createRelationFromFile(const string& fname, ListExpr resType){
             boost::lock_guard<boost::mutex> guard(simtx);

             // create result relation
             ListExpr tType = nl->Second(resType);
             tType = SecondoSystem::GetCatalog()->NumericType(resType);
             TupleType* tt = new TupleType(nl->Second(tType));

             createRelMut.lock();
             Relation* resultrel = new Relation(tType); 
             createRelMut.unlock();
            
             ffeed5Info reader(fname,tt);
             Tuple* tuple;
             while((tuple=reader.next())){
                 createRelMut.lock();
                 resultrel->AppendTuple(tuple);
                 createRelMut.unlock();
                 tuple->DeleteIfAllowed();
             }
             tt->DeleteIfAllowed();
             Word result((void*) 0);
             result.addr = resultrel;
             return result;
          } 

  private:
    string host;
    int port;
    string config;
    SecondoInterfaceCS* si;
    NestedList* mynl;
    int serverPID;

    boost::mutex simtx; // mutex for synchronizing access to the interface

    static boost::mutex createRelMut;

};

boost::mutex ConnectionInfo::createRelMut;

template<class ResType>
class CommandListener{
 public:

  virtual void jobDone(int id, string& command, size_t no, int error, 
                       string& errMsg, ResType& resList)=0;
};






/*
Forward declaration of class DArray2Element.

2.1.1 Class ~DArray2Element~

This class represents information about a single connection of a DArray.

*/

class DArray2Element{
  public:
     DArray2Element(const string& _server, const int _port,
                    const int _num, const string& _config):
        server(_server), port(_port), num(_num),config(_config) {}

     DArray2Element(const DArray2Element& src):
      server(src.server), port(src.port), num(src.num),
      config(src.config) {}

     DArray2Element& operator=(const DArray2Element& src){
        this->server = src.server;
        this->port = src.port;
        this->num = src.num;
        this->config = src.config;
        return *this;
     }     

     ~DArray2Element() {}


      void setNum(const int num){
         this->num = num;
      }

     void set(const string& server, const int port, 
              const int num, const string& config){
        this->server = server;
        this->port = port;
        this->num = num;
        this->config = config;
     }


     bool operator==(const DArray2Element& other) const{
       return    (this->port == other.port)
              && (this->num == other.num)
              && (this->server == other.server)
              && (this->config == other.config); 
     }
     bool operator!=(const DArray2Element& other) const{
       return   !((*this) == other);
     }

     bool operator<(const DArray2Element& other) const {
        if(this->port < other.port) return true;
        if(this->port > other.port) return false;
        if(this->num < other.num) return true;
        if(this->num > other.num) return false;
        if(this->server < other.server) return true;
        if(this->server > other.server) return false;
        if(this->config < other.config) return true;
        // equal or greater
        return false;
     }

     bool operator>(const DArray2Element& other) const {
        if(this->port > other.port) return true;
        if(this->port < other.port) return false;
        if(this->num > other.num) return true;
        if(this->num < other.num) return false;
        if(this->server > other.server) return true;
        if(this->server < other.server) return false;
        if(this->config > other.config) return true;
        // equal or greater
        return false;
     }
     
     ListExpr toListExpr(){
        return nl->ThreeElemList(
                   nl->TextAtom(server),
                   nl->IntAtom(port),
                   nl->TextAtom(config));   
     }

     bool readFrom(SmiRecord& valueRecord, size_t& offset){
        string s;
        if(!readVar(s,valueRecord,offset)){
            return false;
        }
        uint32_t p;
        if(!readVar(p,valueRecord,offset)){
           return false;
        }
        string c;
        if(!readVar(c,valueRecord,offset)){
           return false;
        }

        server = s;
        port = p;
        num = -1;
        config = c;
        return true;
     }

     bool saveTo(SmiRecord& valueRecord, size_t& offset){
        if(!writeVar(server,valueRecord,offset)){
           return false;
        }
        if(!writeVar(port,valueRecord,offset)){
            return false;
        }
        if(!writeVar(config,valueRecord,offset)){
           return false;
        }
        return true;
     }

     void print(ostream& out)const{
         out << "( S: " << server << ", P : " << port 
             << "Num : " << num
             << ", C : " << config << ")";
     }

     string getHost()const{ return server; }
     int getPort() const {return port; }
     string getConfig() const{ return config; }
     int getNum() const{ return num; }


  private:
     string server;
     uint32_t port;
     uint32_t num;
     string config;
};


ostream& operator<<(ostream& out, const DArray2Element& elem){
  elem.print(out);
  return out;
}



bool InDArray2Element(ListExpr list, DArray2Element& result){
   if(!nl->HasLength(list,3)){
     return false;
   }
   ListExpr e1 = nl->First(list);
   ListExpr e2 = nl->Second(list);
   ListExpr e4 = nl->Third(list);
   string server;
   int port;
   int num;
   string config;

   if(nl->AtomType(e1) == StringType){
      server = nl->StringValue(e1);     
   } else if(nl->AtomType(e1) == TextType){
      server = nl->Text2String(e1);
   } else {
      return false;
   }
   stringutils::trim(server);
   if(server.empty()){
     return false;
   }
   if(nl->AtomType(e2) != IntType){
     return false;
   }
   port = nl->IntValue(e2);
   if(port <=0){
     return false;
   }

   if(nl->AtomType(e4) == StringType){
      config = nl->StringValue(e4);     
   } else if(nl->AtomType(e4) == TextType){
      config = nl->Text2String(e4);
   } else {
      return false;
   }
   stringutils::trim(config);
   if(config.empty()){
      return false;
   }   
   num = -1;
   result.set(server,port,num,config);
   return true;
}


class Distributed2Algebra: public Algebra{

  public:

/*
1.1 Constructor defined at the end of this file

*/
     Distributed2Algebra();


/*
1.2 Destructor

Closes all open connections and destroys them.

*/
     ~Distributed2Algebra(){
        mtx.lock();
        for(size_t i=0;i<connections.size();i++){
           delete connections[i];
        }
        connections.clear();
        mtx.unlock();
        closeAllWorkers();
     }

/*
~addConnection~

Adds a new connection to the connection pool.

*/
     int addConnection(ConnectionInfo* ci){
       mtx.lock();
       int res = connections.size();
       connections.push_back(ci);
       mtx.unlock();
       return res; 
     }

/*
~noConnections~

Returns the number of connections

*/

     size_t noConnections(){
         mtx.lock();
         size_t res =  connections.size();
         mtx.unlock();
         return res;
     }
/*
~getConnections~

Returns a connection

*/

     ConnectionInfo* getConnection(const int i){
         mtx.lock();
         ConnectionInfo* res =  connections[i];
         mtx.unlock();
         return res;
     }


/*
~noValidConnections~

Returns the number of non-null connections.

*/    

   size_t noValidConnections(){
     size_t count = 0;
     mtx.lock();
     for(size_t i=0;i<connections.size();i++){
       if(connections[i]) count++;
     }
     mtx.unlock();
     return count;
   }

/*
~disconnect~

Closes all connections and destroys the instances.
The return value is the number of closed connections.

*/
     int disconnect(){
        int count = 0;
        mtx.lock();
        for(size_t i=0;i<connections.size();i++){
          if(connections[i]){
             delete connections[i];
             count++;
          }
        }
        connections.clear();
        mtx.unlock();
        return count;
     }
     

/*
~disconnect~

Disconnects a specified connection and removes it from the
connection pool. The result is 0 or 1 depending whether the
given argument specifies an existing connection.

*/
     int disconnect( unsigned int position){
        mtx.lock();
        if( position >= connections.size()){
           mtx.unlock();
           return 0;
        }
        if(!connections[position]){
           mtx.unlock();
           return 0;
        }
        delete connections[position];
        connections[position] = 0;
        mtx.unlock();
        return 1;
     }

/*
~isValidServerNo~

checks whether an given integer points to a server

*/
   bool isValidServerNumber(int no){
      if(no < 0){
        return false;
      }
      mtx.lock();
      bool res = no < (int) connections.size();
      mtx.unlock();
      return res; 
   }

/*
~serverExists~

*/   
  bool serverExists(int s){
     return isValidServerNumber(s) && (connections[s]!=0);
  }

/*
~serverPid~

*/

   int serverPid(int s){
      boost::lock_guard<boost::mutex> guard(mtx);
 
      if(s < 0 || (size_t) s >= connections.size()){
          return 0;
      }
      ConnectionInfo* ci = connections[s];
      return ci?ci->serverPid():0;
   }



/*
~sendFile~

Transfers a local file to a remove server.

*/
    int sendFile( const int con, 
                       const string& local,
                       const string& remote,
                       const bool allowOverwrite){
      if(con < 0 ){
       return -3;
      }
      unsigned int con2 = con;
      mtx.lock();
      if(con2 >= connections.size()){
        mtx.unlock();
        return -3;
      }
      ConnectionInfo* c = connections[con2];
      mtx.unlock();
      if(!c){
         return -3;
      }
      return c->sendFile(local,remote, allowOverwrite);
    }
    

    int requestFile( const int con, 
                     const string& remote,
                     const string& local,
                     const bool allowOverwrite){
      if(con < 0 ){
       return -3;
      }
      unsigned int con2 = con;
      mtx.lock();
      if(con2 >= connections.size()){
        mtx.unlock();
        return -3;
      }
      ConnectionInfo* c = connections[con2];
      mtx.unlock();
      if(!c){
        return -3;
      }
      return c->requestFile(remote,local, allowOverwrite);
    }


    string getRequestFolder( int con){
      if(con < 0 ){
       return "";
      }
      unsigned int con2 = con;
      mtx.lock();
      if(con2 >= connections.size()){
        mtx.unlock();
        return "";
      }
      ConnectionInfo* c = connections[con2];
      mtx.unlock();
      if(!c){
         return "";
      }
      return c->getRequestFolder(); 
    }
    
    string getSendFolder( int con){
      if(con < 0 ){
       return "";
      }
      unsigned int con2 = con;
      mtx.lock();
      if(con2 >= connections.size()){
        mtx.unlock();
        return "";
      }
      ConnectionInfo* c = connections[con2];
      mtx.unlock();
      if(!c){
        return "";
      }
      return c->getSendFolder(); 
    }


    string getHost(int con){
      mtx.lock();
      if(con < 0 || con >= (int) connections.size()){
         mtx.unlock();
         return "";
      }
      ConnectionInfo* c = connections[con];
      mtx.unlock();
      if(!c) return "";
      return c->getHost();
    }
    
    int getPort(int con){
      mtx.lock();
      if(con < 0 || con >= (int) connections.size()){
         mtx.unlock();
         return -3;
      }
      ConnectionInfo* c = connections[con];
      mtx.unlock();
      if(!c) return -3;
      return c->getPort();
    }
    
    string getConfig(int con){
      mtx.lock();
      if(con < 0 || con >= (int) connections.size()){
         mtx.unlock();
         return "";
      }
      ConnectionInfo* c = connections[con];
      mtx.unlock();
      if(!c) return "";
      return c->getConfig();
    }

    bool check(int con){
      mtx.lock();
      if(con < 0 || con >= (int) connections.size()){
         mtx.unlock();
         return false;
      }
      ConnectionInfo* c = connections[con];
      mtx.unlock();
      if(!c) return false;
      return c->check();
    }
    


    bool simpleCommand(int con, const string& cmd, int& error, 
                       string& errMsg, ListExpr& resList, const bool rewrite){
      mtx.lock();
      if(con < 0 || con >= (int) connections.size()){
         mtx.unlock();
         error = -3;
         errMsg = "invalid connection number";
         resList = nl->TheEmptyList();
         return false;
      }
      ConnectionInfo* c = connections[con];
      mtx.unlock();
      if(!c) {
         error = -3;
         errMsg = "invalid connection number";
         resList = nl->TheEmptyList();
         return false;
      }
      c->simpleCommand(cmd,error,errMsg,resList, rewrite);
      return true;
    }
    

    bool simpleCommand(int con, const string& cmd, int& error, 
                       string& errMsg, string& resList, const bool rewrite){
      mtx.lock();
      if(con < 0 || con >= (int) connections.size()){
         mtx.unlock();
         error = -3;
         errMsg = "invalid connection number";
         resList = "()";
         return false;
      }
      ConnectionInfo* c = connections[con];
      mtx.unlock();
      if(!c) {
         error = -3;
         errMsg = "invalid connection number";
         resList = "()";
         return false;
      }
      c->simpleCommand(cmd,error,errMsg,resList, rewrite);
      return true;
    }


/*
The next functions are for interacting with workers, i.e.
connections coming from darray2 elements.

*/
  ConnectionInfo* getWorkerConnection( const DArray2Element& info,
                                       const string& dbname ){
     boost::lock_guard<boost::mutex> guard(workerMtx);
     map<DArray2Element, pair<string, ConnectionInfo*> >::iterator it;
     it = workerconnections.find(info);
     pair<string,ConnectionInfo*> pr("",0);
     if(it==workerconnections.end()){
        if(!createWorkerConnection(info,pr)){
             return 0;
        }
        workerconnections[info] = pr;
        it = workerconnections.find(info);
     } else {
         pr = it->second;
     }
     string wdbname = dbname;
     if(pr.first!=wdbname){
         if(!pr.second->switchDatabase(wdbname,true)){
            it->second.first="";
            return 0;
         } else {
             it->second.first=dbname;
         }
     }
     return pr.second;
  }


  string getDBName(const DArray2Element& info){
     boost::lock_guard<boost::mutex> guard(workerMtx);
     map<DArray2Element, pair<string, ConnectionInfo*> >::iterator it;
     it = workerconnections.find(info);
     string db = "";
     if(it!=workerconnections.end()){
        db=it->second.first;         
     }
     return db;
  }


/*
This operator closes all non user defined existing server connections.
It returns the numer of closed workers

*/
 int closeAllWorkers(){
    boost::lock_guard<boost::mutex> guard(workerMtx);
    map<DArray2Element, pair<string,ConnectionInfo*> > ::iterator it;
    int count = 0;
    for(it=workerconnections.begin(); it!=workerconnections.end(); it++){
       ConnectionInfo* ci = it->second.second;
       delete ci;
       count++;
    }
    workerconnections.clear();
    return count;
 } 

/*
The operator ~closeWorker~ closes the connections for a 
specified DArray2Element.

*/ 
  bool closeWorker(const DArray2Element& elem){
    boost::lock_guard<boost::mutex> guard(workerMtx);
     typename map<DArray2Element, pair<string,ConnectionInfo*> >::iterator it;
     it = workerconnections.find(elem);
     if(it==workerconnections.end()){
        return false;
     }
     ConnectionInfo* info = it->second.second;
     if(info){
        delete info;
        it->second.second = 0;
     }
     workerconnections.erase(it);
     return true;
  }

  bool workerConnection(const DArray2Element& elem, string& dbname,
                         ConnectionInfo*& ci){
    boost::lock_guard<boost::mutex> guard(workerMtx);
     typename map<DArray2Element, pair<string,ConnectionInfo*> >::iterator it;
     it = workerconnections.find(elem);
     if(it == workerconnections.end()){
        return false;
     } 
     dbname = it->second.first;
     ci = it->second.second;
     return true;
  }

  map<DArray2Element, pair<string,ConnectionInfo*> >::iterator
  workersIterator(){
    return workerconnections.begin();
  }

  bool isEnd( map<DArray2Element, pair<string,ConnectionInfo*> >::iterator& it){
    boost::lock_guard<boost::mutex> guard(workerMtx);
    return it==workerconnections.end();
  } 


  string getTempName(int server){
    boost::lock_guard<boost::mutex> guard(mtx);
    if(server < 0 || (size_t)server<connections.size()){
       return "";
    }
    ConnectionInfo* ci = connections[server];
    if(!ci){
       return "";
    }
    stringstream ss;
    ss << "TMP_" << WinUnix::getpid() 
       << "_" << ci->serverPid() 
       << "_" << nextNameNumber();
    return ss.str();
  } 

  string getTempName(const DArray2Element& elem){
     boost::lock_guard<boost::mutex> guard(workerMtx);
     map<DArray2Element, pair<string,ConnectionInfo*> >::iterator it;
     it = workerconnections.find(elem);
     if(it==workerconnections.end()){
        return "";
     }
     stringstream ss;
     ss << "TMP_" << WinUnix::getpid() 
        << "_" << it->second.second->serverPid() 
        << "_" << nextNameNumber();
    return ss.str();
  }

  string getTempName(){
     boost::lock_guard<boost::mutex> guard1(workerMtx);
     boost::lock_guard<boost::mutex> guard2(mtx);
     ConnectionInfo* ci=0;
     for(size_t i=0;i<connections.size() && !ci; i++){
         ci = connections[i];
     }
     map<DArray2Element, pair<string,ConnectionInfo*> >::iterator it;
     for(it = workerconnections.begin(); 
        it !=workerconnections.end() && !ci; 
        it++){
         ci = it->second.second;
     }
     int spid;
     size_t sh;
     if(ci){
        sh = stringutils::hashCode(ci->getHost());
        spid = ci->serverPid();
     } else {
        srand(time(0));
        spid = rand();
        sh = rand();
     }
     stringstream ss;
     ss << "TMP_" << WinUnix::getpid() 
        << "_" << sh 
        << "_" << spid 
        << "_" << nextNameNumber();
    return ss.str();
  }


  void cleanUp(){
    
     set<pair<string,int> > used;
     vector<ConnectionInfo*> unique;

     for(size_t i=0;i<connections.size();i++){
         ConnectionInfo* ci = connections[i];
         if(ci){
             pair<string,int> p(ci->getHost(), ci->getPort());
             if(used.find(p)==used.end()){
                  unique.push_back(ci);
                  used.insert(p);
             }
         }
     }
     map<DArray2Element, pair<string,ConnectionInfo*> >::iterator it;
     for(it = workerconnections.begin();it!=workerconnections.end();it++){
        ConnectionInfo* ci = it->second.second;
        if(ci){
            pair<string,int> p(ci->getHost(), ci->getPort());
            if(used.find(p)==used.end()){
                 unique.push_back(ci);
                 used.insert(p);
            }
        }
     }

     vector<boost::thread*> runners;
     for(size_t i=0;i<unique.size();i++){
        ConnectionInfo* ci = unique[i];
        boost::thread* r = new boost::thread(&ConnectionInfo::cleanUp, ci);
        runners.push_back(r);
     }
     for(size_t i=0;i<runners.size();i++){
        runners[i]->join();
        delete runners[i];
     }
  }


  private:
    // connections managed by the user
    vector<ConnectionInfo*> connections;
    boost::mutex mtx;

    // connections managed automatically 
    // for darray2 type
    // the key represents the connection information,
    // the string the used database
    // the ConnctionInfo the connection
    map<DArray2Element, pair<string,ConnectionInfo*> > workerconnections;
    boost::mutex workerMtx;

    size_t namecounter;
    boost::mutex namecounteraccess;


   size_t nextNameNumber(){
      boost::lock_guard<boost::mutex> guard(namecounteraccess);
      namecounter++;
      return namecounter;
   } 

    bool createWorkerConnection(const DArray2Element& worker, pair<string, 
                                ConnectionInfo*>& res){
      string host = worker.getHost();
      int port = worker.getPort();
      string config = worker.getConfig();
      ConnectionInfo* ci = ConnectionInfo::createConnection(host, port, config);
      res.first="";
      res.second = ci;
      return ci!=0;
    }



};

Distributed2Algebra* algInstance;

/*
2. Class ConnectionTask

This class manages the task for a single connection.
In an endless loop it will wait for a new command. If a 
new command is available, the command is executed and
the listener is informed about that. Then, then it waits for
the next command.

*/
template<class ResType>
class ConnectionTask{
  public:

     ConnectionTask( int _connection, 
                     CommandListener<ResType>* _listener):
      connection(_connection), listener(_listener), no(0),
      done(false){
        worker = boost::thread(&ConnectionTask::run, this); 
      } 
     
    ~ConnectionTask(){
          listener = 0;
          {
             boost::lock_guard<boost::mutex> lock(condmtx);
             done = true;
          }
          cond.notify_one();
          worker.join(); // wait until worker is done
      }


     void addCommand(const int id, const string& cmd){
       { 
         boost::lock_guard<boost::mutex> lock(condmtx);
         commandList.push_back(make_pair(id, cmd));
       }
       cond.notify_one();        
     }

     void removeListener(){
        listener = 0;
     }
  
     void stop(){
        done = true;
     }

  private:
    
    void run(){
      pair<int,string> cmd1;
      while(!done){
         {
           boost::unique_lock<boost::mutex> lock(condmtx);
           // wait for available command 
           while(!done && commandList.empty()){
              cond.wait(lock);
           }
           if(done){
             return; 
           }
           // get next command from list
           cmd1 = commandList.front();
           commandList.pop_front();
         }
         no++;
         string cmd = cmd1.second;
         int id = cmd1.first;
         int error;
         string errMsg; 
         ResType resList;
         algInstance->simpleCommand(connection, cmd, 
                                    error,errMsg, resList, true);

         if(listener){
            listener->jobDone(id, cmd, no, error, errMsg, resList);
         }
      }
     }

     int connection;            // server number
     CommandListener<ResType>* listener; // informed if a command is processed
     size_t no;                 // a running number
     bool done;                 // finished state
     list<pair<int,string> > commandList; // list of command

     boost::mutex condmtx; 
     boost::condition_variable cond;
     boost::thread worker;
     
};




/*
1 Operators

1.1 Operator  ~connect~

Establishes a connection to a running Secondo Server.
The result of this operator is a boolean indicating the success 
of the operation. 

1.1.1 Type Mapping

This operator gets a hostname, a port and a file for the configuration.

*/
ListExpr connectTM(ListExpr args){
  string err = "{text,string} x int [x {text,string}] = "
               "(host, port[, configfile]) expected";
  if(!nl->HasLength(args,3) && !nl->HasLength(args,2)){
   return listutils::typeError(err);     
  }
  ListExpr first = nl->First(args);
  if(!FText::checkType(first)&& !CcString::checkType(first)){
    return listutils::typeError(err);     
  }
  if(!CcInt::checkType(nl->Second(args))){
    return listutils::typeError(err);     
  }
  if(nl->HasLength(args,3)) {
     ListExpr third = nl->Third(args);
     if(!FText::checkType(third) && !CcString::checkType(third)){
       return listutils::typeError(err);     
     }
     return listutils::basicSymbol<CcBool>();
  }
  return nl->ThreeElemList(
           nl->SymbolAtom(Symbols::APPEND()),
           nl->OneElemList( nl->StringAtom("SecondoConfig.ini")),
           listutils::basicSymbol<CcBool>());
}

/*
1.1.2 Value Mapping

*/
template<class H, class C>
int connectVMT( Word* args, Word& result, int message,
                  Word& local, Supplier s ){
   result = qp->ResultStorage(s);
   CcBool* res = (CcBool*) result.addr;

   H* ahost = (H*) args[0].addr;
   CcInt* aport = (CcInt*) args[1].addr;
   C* afile = (C*) args[2].addr;

   if(!ahost->IsDefined() || !aport->IsDefined() || !afile->IsDefined()){
      res->SetDefined(false);
      return 0;
   }
   string host = ahost->GetValue();
   int port = aport->GetValue();
   string file = afile->GetValue();
   if(port<=0){
      res->SetDefined(false);
      return 0;
   }
   NestedList* mynl = new NestedList();
   SecondoInterfaceCS* si = new SecondoInterfaceCS(true,mynl);
   string user="";
   string passwd = "";
   string errMsg;
   MessageCenter* msgcenter = MessageCenter::GetInstance();
   si->setMaxAttempts(4);
   si->setTimeout(1);
   if(! si->Initialize(user, passwd, host, 
                       stringutils::int2str(port), file, 
                       errMsg, true)){
      msgcenter->Send(nl, nl->TwoElemList( nl->SymbolAtom("simple"),
                                       nl->TextAtom(errMsg)));
     delete si;
     delete mynl;
     res->Set(true,false);
   } else {
     res->Set(true,true);
     ConnectionInfo* ci= new ConnectionInfo(host, port, file, si, mynl);
     algInstance->addConnection(ci);     
   }
   return 0;
}

/*
1.1.3 Specification

*/
OperatorSpec connectSpec(
     "text x int x text -> bool",
     "connect(host, port, configfile)",
     "Connects to a Secondo Server",
     " query connect('localhost', 1234, 'SecondoConfig.ini')"); 


/*
1.1.4 ValueMapping Array

*/
ValueMapping connectVM[] = {
  connectVMT<CcString,CcString>,
  connectVMT<CcString,FText>,
  connectVMT<FText, CcString>,
  connectVMT<FText,FText>
};

/*
1.1.5 Selection Function

*/
int connectSelect(ListExpr args){
  int first = FText::checkType(nl->First(args))?2:0;
  int second = 0;
  if(nl->HasLength(args,3)){
    second = FText::checkType(nl->Third(args))?1:0;
  }
  return first + second;
}


/*
1.1.6 Operator instance


*/
Operator connectOp (
    "connect",             //name
     connectSpec.getStr(),         //specification
     4,
     connectVM,        //value mapping
     connectSelect,   //trivial selection function
     connectTM        //type mapping
);


/*
1.2 Operator checkConnection

This operator checks available connections.
The current implementation soes it sequentially. In the future,
this will be parallelized using the boost library.

1.2.1 Type Mapping

This operator have no arguments. The result is a stream of tuples 
of the form (id, host, port, configfile, state)

*/
ListExpr  checkConnectionsTM(ListExpr args){
  
  if(!nl->IsEmpty(args)){
    return listutils::typeError("no arguments required");
  }
  ListExpr attrList = nl->SixElemList(
    nl->TwoElemList( nl->SymbolAtom("Id"), 
                     listutils::basicSymbol<CcInt>()),
    nl->TwoElemList( nl->SymbolAtom("Host"), 
                     listutils::basicSymbol<FText>()),
    nl->TwoElemList( nl->SymbolAtom("Port"), 
                     listutils::basicSymbol<CcInt>()),
    nl->TwoElemList( nl->SymbolAtom("ConfigFile"), 
                     listutils::basicSymbol<FText>()),
    nl->TwoElemList( nl->SymbolAtom("OK"), 
                     listutils::basicSymbol<CcBool>()),
    nl->TwoElemList( nl->SymbolAtom("PID"), 
                     listutils::basicSymbol<CcInt>()) 
);
    return nl->TwoElemList(
             listutils::basicSymbol<Stream<Tuple> >(),
             nl->TwoElemList(
                 listutils::basicSymbol<Tuple>(),
                  attrList));
}

/*
1.2.2 Value Mapping

*/

class checkConLocal{
 public:
    checkConLocal(ListExpr resType): pos(0){
      tt = new TupleType(resType);
    }
    ~checkConLocal(){
       tt->DeleteIfAllowed();
    }
    Tuple* next(){
      if(pos >= algInstance->noConnections()){
        return 0;
      }
      Tuple* res = new Tuple(tt);
      res->PutAttribute(0, new CcInt(true,pos));
      string h = algInstance->getHost(pos);
      FText* host = h.empty()?new FText(false,""): new FText(true,h);
      res->PutAttribute(1, host);
      int p = algInstance->getPort(pos);
      CcInt* port = p<0?new CcInt(false,0) : new CcInt(true,p);
      res->PutAttribute(2, port);
      string c = algInstance->getConfig(pos);
      FText* config = c.empty()?new FText(false,""): new FText(true,c);
      res->PutAttribute(3, config);
      res->PutAttribute(4, new CcBool(true, algInstance->check(pos)));
      int pid = algInstance->serverPid(pos);
      CcInt* Pid = pid>0?new CcInt(true,pid): new CcInt(false,0);
      res->PutAttribute(5,Pid);
      pos++;
      return res;
    }
 private:
    size_t pos;
    TupleType* tt;

};


int checkConnectionsVM( Word* args, Word& result, int message,
                  Word& local, Supplier s ){

  checkConLocal* li = (checkConLocal*) local.addr;
  switch(message){
     case OPEN: 
         if(li) delete li;
         local.addr = new checkConLocal(nl->Second(GetTupleResultType(s)));
         return 0;
     case REQUEST: 
         result.addr = li?li->next():0;
         return result.addr?YIELD:CANCEL;
     case CLOSE:
           if(li){
             delete li;
             local.addr = 0;
           }              
           return 0;   
  }
  return -1;

}

/*
1.2.3 Specification

*/
OperatorSpec checkConnectionsSpec(
     "-> stream(tuple((No int)(Host text)(Port int)(Config text)(Ok bool)",
     "checkConnections()",
     "Check connections in the Distributed2Algebra",
     " query checkConnections() consume"); 


/*
1.2.4 Operator instance


*/
Operator checkConnectionsOp (
    "checkConnections",             //name
    checkConnectionsSpec.getStr(),         //specification
    checkConnectionsVM,        //value mapping
    Operator::SimpleSelect,   //trivial selection function
    checkConnectionsTM        //type mapping
);



/*
1.3 Operator ~rcmd~

This operator performs a remote command on a server.
The result is a tuple stream containing a single tuple containing the 
result of the command.

1.3.1 Type Mapping

*/
ListExpr rcmdTM(ListExpr args){
  string err = "int x {string, text} expected";
  if(!nl->HasLength(args,2)){
    return listutils::typeError(err);
  }
  if(!CcInt::checkType(nl->First(args))){
    return listutils::typeError(err);
  }
  ListExpr cmdt = nl->Second(args);
  if(!FText::checkType(cmdt) && !CcString::checkType(cmdt)){
    return listutils::typeError(err);
  }
  ListExpr attrList = nl->ThreeElemList(
         nl->TwoElemList( nl->SymbolAtom("Connection"), 
                          listutils::basicSymbol<CcInt>()),
         nl->TwoElemList( nl->SymbolAtom("ErrorCode"), 
                          listutils::basicSymbol<CcInt>()),
         nl->TwoElemList( nl->SymbolAtom("Result"), 
                          listutils::basicSymbol<FText>()));
  return nl->TwoElemList( 
            listutils::basicSymbol<Stream<Tuple> >(),
            nl->TwoElemList(
                listutils::basicSymbol<Tuple>(),
                attrList));
}

/*
1.3.2 Value Mapping

*/
template<class C>
int rcmdVMT( Word* args, Word& result, int message,
                        Word& local, Supplier s ){
   switch(message){
      case OPEN: {
           CcInt* con = (CcInt*) args[0].addr;
           C*  cmd = (C*) args[1].addr;
           if(!con->IsDefined() || !cmd->IsDefined()){
             return 0;
           }
           int conv = con->GetValue();
           int errorCode = 0;
           string result ="";
           string errMsg;
           bool ok = algInstance->simpleCommand(conv, cmd->GetValue(), 
                                           errorCode, errMsg, result, true);
           if(!ok){
             return 0;
           }
           TupleType* tt = new TupleType( nl->Second(GetTupleResultType(s)));
           Tuple* resTuple = new Tuple(tt);
           tt->DeleteIfAllowed();
           resTuple->PutAttribute(0, new CcInt(true,conv));
           resTuple->PutAttribute(1, new CcInt(true,errorCode));
           resTuple->PutAttribute(2, new FText(true, result));
           local.addr = resTuple;
            return 0;
       }
       case REQUEST: {
           result.addr = local.addr;
           local.addr = 0;
           return result.addr?YIELD:CANCEL; 
       }
       case CLOSE: {
          if(local.addr){
             ((Tuple*)local.addr)->DeleteIfAllowed();
             local.addr = 0; 
          }
          return 0;
       }
   }
   return -1;
}

/*
1.3.4 ValueMapping Array

*/
ValueMapping rcmdVM[] = {
    rcmdVMT<CcString>,
    rcmdVMT<FText>
};

/*
1.3.5 Selection function

*/
int rcmdSelect(ListExpr args){
   ListExpr cmd = nl->Second(args);
   return CcString::checkType(cmd)?0:1;
}

/*
1.3.6 Specification

*/
OperatorSpec rcmdSpec(
     "int x {text,string} -> stream(Tuple(C,E,R))",
     "rcmd(Connection, Command)",
     "Performs a remote command at given connection.",
     "query rcmd(1,'list algebras')"); 

/*
1.3.7 Operator instance

*/
Operator rcmdOp (
    "rcmd",             //name
     rcmdSpec.getStr(),         //specification
     2,
     rcmdVM,        //value mapping
     rcmdSelect,   //trivial selection function
     rcmdTM        //type mapping
);


/*
1.4 Operator disconnect

This operator closes an existing connection. The ID's of 
subsequent connects will be decreased. If no special id
id given, all existing connections will be closed.

1.4.1 Type Mapping

*/
ListExpr disconnectTM(ListExpr args){

  string err = "int or nothing expected";
  if(nl->IsEmpty(args)){
    return listutils::basicSymbol<CcInt>();
  }
  if(!nl->HasLength(args,1)){
     return listutils::typeError(err);
  }
  if(!CcInt::checkType(nl->First(args))){
     return listutils::typeError(err);
  }
  return listutils::basicSymbol<CcInt>();
}


int disconnectVM( Word* args, Word& result, int message,
                        Word& local, Supplier s ){
  int resi=0;
  if(qp->GetNoSons(s)==0){
    // remove all commections
    resi = algInstance->disconnect();
  } else {
    // remove special connection
    CcInt* c = (CcInt*) args[0].addr;
    if(c->IsDefined() && c->GetValue()>=0){
       resi = algInstance->disconnect(c->GetValue());
    }
  }
  result = qp->ResultStorage(s);
  CcInt* res = (CcInt*) result.addr;
  res->Set(true,resi);
  return 0;
}

/*
1.4.3 Specification

*/
OperatorSpec disconnectSpec(
     " int -> int , ->int",
     "disconnect(), disconnect(_)",
     "Closes a connection to a remote server. Without any argument"
     " all connections are closed, otherwise only the specified one",
     "query disconnect(0)");

/*
1.4.4 Operator instance

*/
Operator disconnectOp (
    "disconnect",             //name
     disconnectSpec.getStr(),         //specification
     disconnectVM,        //value mapping
     Operator::SimpleSelect,   //trivial selection function
     disconnectTM        //type mapping
);



/*
1..5 Operator rquery

This operator works quite similar to the rcmd command. 
The difference is that the correct type will be produced if possible.
This is done using a query on the specified server within the type mapping.
For this reason. only constants for the server number and the command
can be given. This makes only sense for queries. 

*/
template<class S>
bool getValue(ListExpr in, string&out){
   Word res;
   bool success = QueryProcessor::ExecuteQuery(nl->ToString(in),res);
   if(!success){
     out ="";
     return false;
   }
   S* s = (S*) res.addr;
   if(!s->IsDefined()){
      out = "";
      s->DeleteIfAllowed();
      return false;
   }
   out = s->GetValue();
   s->DeleteIfAllowed();
   return true;
}

bool getValue(ListExpr in, int& out){
   Word res;
   bool success = QueryProcessor::ExecuteQuery(nl->ToString(in),res);
   if(!success){
     out = -1;
     return false;
   }
   CcInt* r = (CcInt*) res.addr;
   if(!r->IsDefined()){
     out = -1;
     r->DeleteIfAllowed();
     return false;
   }
   out = r->GetValue();
   r->DeleteIfAllowed();
   return true;
}


ListExpr rqueryTM(ListExpr args){
  string err = "int x {text,string} expected";
  if(!nl->HasLength(args,2)){
     return listutils::typeError(err);
  }
  ListExpr first = nl->First(args);
  ListExpr second = nl->Second(args);
  if(!nl->HasLength(first,2) || !nl->HasLength(second,2)){
     return listutils::typeError("internal Error: usesArgsInTypeMapping");
  }
  if(!CcInt::checkType(nl->First(first))){
     return listutils::typeError(err);
  }
  ListExpr serverNo = nl->Second(first);
  // get the server number from the expression
   Word res;
   bool success = QueryProcessor::ExecuteQuery(nl->ToString(serverNo),res);
   if(!success){
       return listutils::typeError("could not evaluate the value of  " +
                                    nl->ToString(serverNo) );
   }
   CcInt* serverNoV = (CcInt*) res.addr;
   if(!serverNoV->IsDefined()){
       return listutils::typeError("Undefined Server number");
   }
   int sn = serverNoV->GetValue();
   serverNoV->DeleteIfAllowed();
   if(sn < 0 || sn>= (int)(algInstance->noConnections())){
      return listutils::typeError("Invalid server number");
   }
   // ok server number is ok
   string query;
   bool ok;
   if(FText::checkType(nl->First(second))){
      ok = getValue<FText>(nl->Second(second),query);
   } else if(CcString::checkType(nl->First(second))){
      ok = getValue<CcString>(nl->Second(second),query);
   } else {
      return listutils::typeError(err);
   }
   if(!ok){
      return listutils::typeError("Value of second argument invalid");
   }


   stringutils::trim(query);
   if(!stringutils::startsWith(query,"query")){
     return listutils::typeError("command is not a query");
   }
   query = query.substr(5); // remove leading query
   stringutils::trim(query);
   

   query = "query (" + query + ") getTypeNL";


   int error;
   string errMsg;
   ListExpr reslist;
   ok = algInstance->simpleCommand(sn, query, error, errMsg, reslist, true);
   if(!ok){
     return listutils::typeError("found no connection for specified "
                                 "connection number");
   }
   if(error!=0){
     return listutils::typeError("Problem in determining result type : "
                                 + errMsg + "\n" + query);
   }
   if(!nl->HasLength(reslist,2)){
      return listutils::typeError("Invalid result get from remote server");
   }
   if(!FText::checkType(nl->First(reslist))){
      return listutils::typeError("Invalid result type for getTypeNL");
   }
   if(nl->AtomType(nl->Second(reslist))!=TextType){
     return listutils::typeError("getTypeNL returns invalid result");
   }
   string typeList = nl->Text2String(nl->Second(reslist));
   ListExpr resType;
   nlparsemtx.lock(); 
   if(!nl->ReadFromString(typeList,resType)){
      nlparsemtx.unlock();
     return listutils::typeError("getTypeNL returns no valid list expression");
   }   
   nlparsemtx.unlock();
   if(nl->HasLength(resType,2)){
     first = nl->First(resType);
     if(listutils::isSymbol(first, Stream<Tuple>::BasicType())){
        return listutils::typeError("remote result is a stream");
     }
   }


   return resType; 
}

/*
1.5.2 Value Mapping

*/
void sendMessage(MessageCenter* msg, const string& message){
  ListExpr list = nl->TwoElemList( nl->SymbolAtom("error"),
                                   nl->TextAtom(message));
  msg->Send(nl,list );
  msg->Flush();
}


template<class T>
int rqueryVMT( Word* args, Word& result, int message,
               Word& local, Supplier s ){

  MessageCenter*  msg = MessageCenter::GetInstance();
  result = qp->ResultStorage(s);
  CcInt* server = (CcInt*) args[0].addr;
  if(!server->IsDefined()){
     sendMessage(msg, "Server ID undefined");
     return 0;
  }
  int serv = server->GetValue();
  if(serv < 0 || serv >= algInstance->noConnections()){
    sendMessage(msg, "Invalid Server ID " );
    return 0;
  }
  T* query = (T*) args[1].addr;
  if(!query->IsDefined()){
    sendMessage(msg,"Undefined query");
    return 0;
  }
  int error; 
  string errMsg;
  ListExpr resList;
  bool ok = algInstance->simpleCommand(serv, query->GetValue(), error,
                                       errMsg, resList, true);
  if(!ok){
     sendMessage(msg, "Error during remote command" );
     return 0;
  }
  if(error){
     sendMessage(msg, "Error during remote command" );
     return 0;
  } 
  // Create Object from resList
  if(!nl->HasLength(resList,2)){
     sendMessage(msg,"Invalid result list");
     return 0;
  }
  ListExpr typeList = nl->First(resList);
  ListExpr valueList = nl->Second(resList);
  int algId;
  int typeId;
  string typeName;
  ListExpr nTypeList = SecondoSystem::GetCatalog()->NumericType(typeList);
  ok = SecondoSystem::GetCatalog()->LookUpTypeExpr(typeList, 
                                               typeName, algId, typeId);
  if(!ok){ // could not determine algid and typeid for given type list
     sendMessage(msg, string("could not determine type Id and algebra "
                             "id for ") + nl->ToString(typeList) );
     return 0;
  }
  int errorPos=0;
  ListExpr errorInfo = listutils::emptyErrorInfo();
  bool correct;
  AlgebraManager* am = SecondoSystem::GetAlgebraManager();
  Word res = am->InObj(algId,typeId)(nTypeList, valueList, 
                       errorPos, errorInfo, correct);
  if(!correct){ // nested list could not read in correctly
     sendMessage(msg, "InObject failed" );
     return 0;
  }
  qp->DeleteResultStorage(s);
  qp->ChangeResultStorage(s,res);
  result = qp->ResultStorage(s);
  return 0;
}

/*
1.5.3 ValueMapping Array and Selection Function

*/

ValueMapping rqueryVM[] = {
  rqueryVMT<CcString>,
  rqueryVMT<FText>
};

int rquerySelect(ListExpr args){
  return CcString::checkType(nl->Second(args))?0:1;
}

/*
1.5.4  Specification

*/
OperatorSpec rquerySpec(
     " int x {string,text} -> ??",
     " rquery(server, query)",
     " Performs a remote query at a remote server."
     " The server is specified by its Id as the first argument."
     " The second argument is the query. The result type depends"
     " on the type of the query expression",
     " query rquery(0, 'query ten count') ");

/*
1.4.4 Operator instance

*/
Operator rqueryOp (
    "rquery",             //name
     rquerySpec.getStr(),         //specification
     2,
     rqueryVM,        //value mapping
     rquerySelect,   //trivial selection function
     rqueryTM        //type mapping
);


/*
1.5 Operator prcmd

This operator takes a stream of tuples at least containing an 
integer and a text/string attribute.  These attributes are named 
as secondary arguments. It performed the commands get by the
text attribute on the servers given by the integer attribute. 
While on each connection, the processing is done in a serial way,
the processing between the different servers is processen in 
parallel. Each incoming tuple is extended by the errorCode, the 
errorMessage and the result (as text). Note that the order 
of output tuples depends on the running time of the single tasks,
not on in input order.

*/
ListExpr prcmdTM(ListExpr args){
  string err = "stream(tuple) x attrname x attrname expected";
  if(!nl->HasLength(args,3)){
    return listutils::typeError(err);
  }
  if(!Stream<Tuple>::checkType(nl->First(args))){
     return listutils::typeError(err + " (first arg is not a tuple stream)");
  }
  if(nl->AtomType(nl->Second(args))!=SymbolType) { 
    return listutils::typeError(err + " 2nd arg is not a "
                                      "valid attribute name");
  }
  if(nl->AtomType(nl->Third(args))!=SymbolType) {
    return listutils::typeError(err + " 3th arg is not a "
                                      "valid attribute name");
  }
  string intAttr = nl->SymbolValue(nl->Second(args));
  string textAttr = nl->SymbolValue(nl->Third(args));
  ListExpr attrList = nl->Second(nl->Second(nl->First(args)));
  ListExpr type;
  int indexi = listutils::findAttribute(attrList,  intAttr, type);
  if(indexi==0){
    return listutils::typeError("Attribute name " + intAttr + " unknown ");
  }
  if(!CcInt::checkType(type)){
    return listutils::typeError("Attribute " + intAttr + " not of type int");
  }
  
  int indext = listutils::findAttribute(attrList, textAttr, type);
  if(indext==0){
    return listutils::typeError("Attribute name " + textAttr + " unknown ");
  }
  if(!CcString::checkType(type) && !FText::checkType(type)){
    return listutils::typeError("Attribute " + textAttr + 
                                " not of type string or text");
  }

  ListExpr extAttr = nl->ThreeElemList(
              nl->TwoElemList( nl->SymbolAtom("ErrorCode"), 
                               listutils::basicSymbol<CcInt>()),
              nl->TwoElemList( nl->SymbolAtom("ErrorMsg") , 
                               listutils::basicSymbol<FText>()),
              nl->TwoElemList( nl->SymbolAtom("ResultList"), 
                               listutils::basicSymbol<FText>()));

  ListExpr resAttrList = listutils::concat(attrList, extAttr);
  if(!listutils::isAttrList(resAttrList)){
     return listutils::typeError("Name conflics in result Type, one of the"
                                 " names ErrorCode, ErrorMsg, or ResultList "
                                 "already present in inpout tuple");
  }
  ListExpr extList = nl->TwoElemList( nl->IntAtom(indexi-1), 
                                      nl->IntAtom(indext-1));
  ListExpr resType = nl->TwoElemList(
                       listutils::basicSymbol<Stream<Tuple> >(),
                       nl->TwoElemList(
                            listutils::basicSymbol<Tuple>(),
                             resAttrList));
  return nl->ThreeElemList( nl->SymbolAtom(Symbols::APPEND()),
                            extList,
                             resType);
}



template<class T>
class prcmdInfo: public CommandListener<string>{

  public:
    prcmdInfo(Word _s, int _intAttrPos, int _qAttrPos, ListExpr _tt):
       inStream(_s), stopped(false), noInTuples(-1), noOutTuples(0),
       serverAttrPos(_intAttrPos), queryAttrPos(_qAttrPos)
    {
      tt = new TupleType(_tt);
      inStream.open();
      runner = boost::thread(boost::bind(&prcmdInfo<T>::run, this));
    }

    virtual ~prcmdInfo(){
       stopMutex.lock();
       stopped = true;
       stopMutex.unlock();
       // wait until runner is ready
       runner.join();
       // stop and delete running connectionTasks
       map<int, ConnectionTask<string>*>::iterator it1;
       for(it1 = serverTasks.begin(); it1!=serverTasks.end(); it1++){
          delete it1->second;
       }
       // delete non processed input tuples
       map<int,Tuple*>::iterator it2;
       inputTuplesMutex.lock();
       for(it2 = validInputTuples.begin(); it2!= validInputTuples.end(); it2++){
          it2->second->DeleteIfAllowed();
       }
       inputTuplesMutex.unlock();
       inStream.close();
       tt->DeleteIfAllowed();
    }


    Tuple* next(){
      // wait for next available tuple
       boost::unique_lock<boost::mutex> lock(resultMutex);
       while(pendingResults.empty()){
            cond.wait(lock);
        }
        Tuple* res = pendingResults.front();
        pendingResults.pop_front();
        return res;
    }
    

    void jobDone(int id, string& command, size_t no, int error, 
                 string& errMsg, string& resList){
       inputTuplesMutex.lock();
       if(validInputTuples.find(id)==validInputTuples.end()){
          inputTuplesMutex.unlock();
          return;
       }
       Tuple* inTuple = validInputTuples[id];
       validInputTuples.erase(id);
       inputTuplesMutex.unlock();
       createResultTuple(inTuple,error,errMsg, resList);
    }


  private:
    Stream<Tuple> inStream;
    bool stopped;
    int noInTuples;
    int noOutTuples;
    int serverAttrPos;
    int queryAttrPos;
    TupleType* tt;
    list<Tuple*> pendingResults;
    boost::thread runner;
    
    boost::mutex resultMutex; // waiting for new results
    boost::condition_variable cond; // waiting for new results

    boost::mutex stopMutex; // access to stopped variable

    map<int,ConnectionTask<string> *> serverTasks;
    map<int, Tuple*>  validInputTuples;

    boost::mutex tupleCreationMutex;
    boost::mutex inputTuplesMutex;

    void run(){
      Tuple* inTuple = inStream.request();
      int noTuples = 0;
      stopMutex.lock();
      while(inTuple!=0 && !stopped){
         stopMutex.unlock();
         noTuples++;
         processInputTuple(inTuple, noTuples);
         inTuple = inStream.request();
         stopMutex.lock();
      }
      if(stopped){
        if(inTuple){
           inTuple->DeleteIfAllowed();
        }
      } else {
         ;
      }
      stopMutex.unlock();
      setNoInTuples(noTuples);
    }

    void setNoInTuples(int num){
       tupleCreationMutex.lock();
       noInTuples = num;
       if(noInTuples == noOutTuples){
         tupleCreationMutex.unlock();
          {
            boost::lock_guard<boost::mutex> lock(resultMutex);
            pendingResults.push_back(0);
          }
          cond.notify_one();
       } else {
         tupleCreationMutex.unlock();
       } 
    } 



    void processInputTuple(Tuple* inTuple, int tupleId){
       CcInt* ServerNo = (CcInt*) inTuple->GetAttribute(serverAttrPos);
       T* Query = (T*) inTuple->GetAttribute(queryAttrPos);
       if(!ServerNo->IsDefined() || !Query->IsDefined()){
          createResultTuple(inTuple,-3, "Undefined Argument found", "");
          return;
       }
       int serverNo = ServerNo->GetValue();
       if(!algInstance->isValidServerNumber(serverNo)){
          createResultTuple(inTuple, -2, "Invalid server number", "");
          return; 
       }
       // process a valid tuple
       inputTuplesMutex.lock();
       validInputTuples[tupleId] = inTuple; // store input tuple
       inputTuplesMutex.unlock();
       // create a ConnectionTask for server if not present  
       if(serverTasks.find(serverNo)==serverTasks.end()){
          serverTasks[serverNo] = new ConnectionTask<string>(serverNo, this);
       }
       // append the command
       serverTasks[serverNo]->addCommand(tupleId, Query->GetValue());
    }


    void createResultTuple(Tuple* inTuple, int error, const string& errMsg, 
                           const string& resList){

       stopMutex.lock();
       if(stopped){
          inTuple->DeleteIfAllowed();
          stopMutex.unlock();
          return;
       }
       stopMutex.unlock();

       tupleCreationMutex.lock();

       Tuple* resTuple = new Tuple(tt);
       int noAttr = inTuple->GetNoAttributes();
       for(int i=0;i<noAttr;i++){
         resTuple->CopyAttribute(i,inTuple,i);
       }
       inTuple->DeleteIfAllowed();
       resTuple->PutAttribute(noAttr, new CcInt(error));
       resTuple->PutAttribute(noAttr+1, new FText(true,errMsg));
       resTuple->PutAttribute(noAttr+2, new FText(true,resList));
       noOutTuples++;
       {
         boost::lock_guard<boost::mutex> lock(resultMutex);
         pendingResults.push_back(resTuple);
         if(noInTuples==noOutTuples){
            pendingResults.push_back(0);
         }
       }
       tupleCreationMutex.unlock();
       // inform about a new result tuple
       cond.notify_one();
    }
};



/*
1.5.3 Value Mapping

*/

template<class T>
int prcmdVMT( Word* args, Word& result, int message,
               Word& local, Supplier s ){

  prcmdInfo<T>* li = (prcmdInfo<T>*) local.addr;
  switch(message){
    case OPEN: {
       if(li){
         delete li;
       }
       local.addr = new prcmdInfo<T>(args[0], 
                                  ((CcInt*)args[3].addr)->GetValue(),
                                  ((CcInt*)args[4].addr)->GetValue(),
                                  nl->Second(GetTupleResultType(s)));
       return 0;
    }
    case REQUEST:
           result.addr = li?li->next():0;
           return result.addr?YIELD:CANCEL;
    case CLOSE:
         if(li){
        //   boost::thread k(del<prcmdInfo<T> >,li); 
           delete li;
           local.addr =0;
         }
         return 0;
  } 
  return -1;
};


/*
1.6.3 ValueMapping Array and Selection Function

*/

ValueMapping prcmdVM[] = {
  prcmdVMT<CcString>,
  prcmdVMT<FText>
};

int prcmdSelect(ListExpr args){
  ListExpr attrList = nl->Second(nl->Second(nl->First(args)));
  ListExpr textType;
  listutils::findAttribute(attrList, 
                    nl->SymbolValue(nl->Third(args)), textType);
  return CcString::checkType(textType)?0:1;
}

/*
1.5.4  Specification

*/
OperatorSpec prcmdSpec(
     " stream(tuple) x attrName x attrName -> stream(tuple + E)",
     " _ prcmd [ _,_]",
     " Performs a a set of remote queries in parallel. Each incoming tuple"
     " is extended by some status information in attributes ErrorCode (int)"
     "and ErrorMsg (text) as well as the result of the query in form of "
     " a nested list (text attribute)",
     " query intstream(0,3) namedtransformstream[S] "
     "extend[ Q : 'query plz feed count'] prcmd[S,Q] consume ");

/*
1.4.4 Operator instance

*/
Operator prcmdOp (
    "prcmd",             //name
     prcmdSpec.getStr(),         //specification
     2,
     prcmdVM,        //value mapping
     prcmdSelect,   //trivial selection function
     prcmdTM        //type mapping
);



/*
1.6 Operator ~sendFile~ 

Copies a local file to a remove server.  This version works  
serial.

1.6.1 Type Mapping

The arguments are the server number, the name of the local file as well as 
the name of the remote file.

*/
ListExpr sendFileTM(ListExpr args){
  string err = "int x {string, text} x {string, text} [ x bool] expected";

  if(!nl->HasLength(args,3) && !nl->HasLength(args,4)){
    return listutils::typeError(err);
  }
  if(!CcInt::checkType(nl->First(args))){
    return listutils::typeError(err);
  }
  if(    !CcString::checkType(nl->Second(args)) 
      && !FText::checkType(nl->Second(args))){
    return listutils::typeError(err);
  } 
  if(    !CcString::checkType(nl->Third(args)) 
      && !FText::checkType(nl->Third(args))){
    return listutils::typeError(err);
  }

  if(nl->HasLength(args,4)){
     if(!CcBool::checkType(nl->Fourth(args))){
        return listutils::typeError(err);
     }
  }
  return listutils::basicSymbol<CcInt>();
}

template<class L , class R>
int sendFileVMT( Word* args, Word& result, int message,
               Word& local, Supplier s ){


   CcInt* Server = (CcInt*) args[0].addr;
   L* Local = (L*) args[1].addr;
   R* Remote = (R*) args[2].addr;
   CcBool al(true,false);
   CcBool* AllowOverwrite = &al;

   if(qp->GetNoSons(s)==4){
      AllowOverwrite = (CcBool*) args[3].addr;
   }
   result = qp->ResultStorage(s);
   CcInt* res = (CcInt*) result.addr;
   if(!Server->IsDefined() || ! Local->IsDefined() || !Remote->IsDefined()
      || !AllowOverwrite->IsDefined()){
      res->SetDefined(false);
      return 0;
   }
   int server = Server->GetValue();
   if(server < 0 || server >= algInstance->noConnections()){
      res->Set(true,-3);
      return 0;
   }
   res->Set(true, algInstance->sendFile(server, 
                                 Local->GetValue(), 
                                 Remote->GetValue(),
                                 AllowOverwrite->GetValue()));
   return 0;
}

/*
1.6.2 Specification

*/

OperatorSpec sendFileSpec(
     " int x {string, text} x {string, text} -> int",
     " sendFile( serverNo, localFile, remoteFile) ",
     " Transfers a local file to the remote server. "
     " The local file is searched within the current "
     " directory (normally the bin directory of Secondo)"
     " if not given as an absolute path. The name of "
     " the remote file must be a relative path without "
     " any gobacks (..) inside. The file is stored "
     " on the server below the directory "
     " $SECONDO_HOME/filetransfers/$PID, where $SECONDO_HOME"
     " denotes the used database directory and $PID corresponds "
     "to the process id of the connected serveri (see also operator"
     " getSendFolder", 
     " query sendFile( 0, 'local.txt', 'remote.txt' ");

/*
1.6.3 Value Mapping Array and Selection

*/

ValueMapping sendFileVM[] = {
  sendFileVMT<CcString, CcString>,
  sendFileVMT<CcString, FText>,
  sendFileVMT<FText, CcString>,
  sendFileVMT<FText, FText>
};

int sendFileSelect(ListExpr args){

  int n1 = CcString::checkType(nl->Second(args))?0:2;
  int n2 = CcString::checkType(nl->Third(args))?0:1;
  return n1+n2; 
}

/*
1.6.4 Operator instance

*/

Operator sendFileOp (
    "sendFile",             //name
     sendFileSpec.getStr(),         //specification
     4,
     sendFileVM,        //value mapping
     sendFileSelect,   //trivial selection function
     sendFileTM        //type mapping
);


/*
1.7 Operator requestFile

This operator request a file from a remote server.

1.7.1 Type Mapping

The arguments for this operator are the number of the server,
the name of the file at server side and the local file name.
The result is a boolean reporting the success of the file 
transfer.

*/

ListExpr requestFileTM(ListExpr args){
  string err = "int x {string, text} x {string, text} [ x bool] expected";

  if(!nl->HasLength(args,4) && !nl->HasLength(args,3) ){
    return listutils::typeError(err);
  }
  if(!CcInt::checkType(nl->First(args))){
    return listutils::typeError(err);
  }
  if(    !CcString::checkType(nl->Second(args)) 
      && !FText::checkType(nl->Second(args))){
    return listutils::typeError(err);
  } 
  if(    !CcString::checkType(nl->Third(args)) 
      && !FText::checkType(nl->Third(args))){
    return listutils::typeError(err);
  }

  if(nl->HasLength(args,4)){ 
    // full argument set given
     if(!CcBool::checkType(nl->Fourth(args))){
        return listutils::typeError(err);
     }
  }

  return listutils::basicSymbol<CcInt>();
}

/*
1.7.2 Value Mapping

*/

template<class R , class L>
int requestFileVMT( Word* args, Word& result, int message,
               Word& local, Supplier s ){
   CcInt* Server = (CcInt*) args[0].addr;
   R* Remote = (R*) args[1].addr;
   L* Local = (L*) args[2].addr;

   CcBool al(true,false);
   CcBool* AllowOverwrite = &al;
   if(qp->GetNoSons(s)==4){
     AllowOverwrite = (CcBool*) args[3].addr;
   }
   result = qp->ResultStorage(s);
   CcInt* res = (CcInt*) result.addr;
   
   if(!Server->IsDefined() || ! Local->IsDefined() || !Remote->IsDefined()
      || !AllowOverwrite->IsDefined()){
      res->SetDefined(false);
      return 0;
   }
   int server = Server->GetValue();
   res->Set(true, algInstance->requestFile(server, 
                                 Remote->GetValue(), 
                                 Local->GetValue(),
                                 AllowOverwrite->GetValue()));
   return 0;
}


OperatorSpec requestFileSpec(
     " int x {string, text} x {string, text} -> bool",
     " requestFile( serverNo, remoteFile, localFile) ",
     " Transfers a remote file to the local file system. "
     " The local file is stored relative to current directory if"
     " its name is not given as an absolute path. The remote "
     " file is taken relative from $SECONDO_HOME/filetransfers."
     " For security reasons, the remote name cannot be an "
     " absolute path and cannot contains any gobacks (..). ",
     " query requestFile( 0, 'remote.txt', 'local.txt' ");

/*
1.6.3 Value Mapping Array and Selection

*/

ValueMapping requestFileVM[] = {
  requestFileVMT<CcString, CcString>,
  requestFileVMT<CcString, FText>,
  requestFileVMT<FText, CcString>,
  requestFileVMT<FText, FText>
};

int requestFileSelect(ListExpr args){

  int n1 = CcString::checkType(nl->Second(args))?0:2;
  int n2 = CcString::checkType(nl->Third(args))?0:1;
  return n1+n2; 
}

/*
1.6.4 Operator instance

*/

Operator requestFileOp (
    "requestFile",             //name
     requestFileSpec.getStr(),         //specification
     4,
     requestFileVM,        //value mapping
     requestFileSelect,   //trivial selection function
     requestFileTM        //type mapping
);



/*
1.7 Operators ~psendFile~ and ~prequestFile~

These operators transfers files between the local
file system and the remote server.

1.7.1 Type Mapping

The operator receives a stream of tuples at least having a number and 
text/string attributes. Furthermore three attribute names are required. 
The first attribute name points to an integer attribute specifing the 
connection where the file should be transferred to. The second attribute 
name points to the local filename within the tuple. This may be a string 
or a text. The third attribute name points to the name of the file which 
should be created at the remote server. 
The last two attribute names may be the same (in this case the local file 
name is equal to the remote file name.

Each input tuple is extened by a boolean and an text attribute. 
The boolean value points to the success of the file transfer and 
the text value will given an error message if fauled.

*/

ListExpr ptransferFileTM(ListExpr args){

  string err = "stream(tuple) x attrname x attrname x attrname expected";

  if(!nl->HasLength(args,4) && !nl->HasLength(args,5) ){
    return listutils::typeError(err + " (invalid number of attributes)");
  }
  ListExpr stream = nl->First(args);
  ListExpr attrs[] = { nl->Second(args), nl->Third(args), nl->Fourth(args)};

  if(!Stream<Tuple>::checkType(stream)){
    return listutils::typeError(err + " ( first arg is not a tuple stream)");
  }

  for(int i=0;i<3;i++){
    if(nl->AtomType(attrs[i])!=SymbolType){
       return listutils::typeError(err + " ( invalid attribute name found )");
    }
  }
  ListExpr attrList = nl->Second(nl->Second(stream));
  ListExpr type;
  string name = nl->SymbolValue(attrs[0]);
  int indexServer = listutils::findAttribute(attrList, name, type);
  if(!indexServer){
    return listutils::typeError(" attribute " + name + " not found");
  }
  if(!CcInt::checkType(type)){
    return listutils::typeError(" attribute " + name + " not an integer");
  }

  name = nl->SymbolValue(attrs[1]);
  int indexLocal = listutils::findAttribute(attrList, name, type);
  if(!indexLocal){
    return listutils::typeError(" attribute " + name + " not found");
  }
  if(!CcString::checkType(type) &&!FText::checkType(type)){
    return listutils::typeError(" attribute " + name + 
                                " is not of type string or text");
  }

  name = nl->SymbolValue(attrs[2]);
  int indexRemote = listutils::findAttribute(attrList, name, type);
  if(!indexRemote){
    return listutils::typeError(" attribute " + name + " not found");
  }
  if(!CcString::checkType(type) &&!FText::checkType(type)){
    return listutils::typeError(" attribute " + name + 
                                " is not of type string or text");
  }

  int indexOver = -1;
  ListExpr appendList;

  if(nl->HasLength(args,5)){ // full parameter set
    ListExpr overname = nl->Fifth(args);
    if(nl->AtomType(overname)!=SymbolType){
      return listutils::typeError("invalid attribute name detected");
    }
    name = nl->SymbolValue(overname);
    indexOver = listutils::findAttribute(attrList, name, type);
    if(!indexOver){
       return listutils::typeError(" attribute " + name + " not found");
    }
    if(!CcBool::checkType(type)){
       return listutils::typeError(" attribute " + name + 
                                   " is not of type bool");
    }
    appendList = nl->FourElemList(
                          nl->IntAtom(indexServer-1),
                          nl->IntAtom(indexLocal-1),
                          nl->IntAtom(indexRemote-1),
                          nl->IntAtom(indexOver-1));

  } else { // optional overwrite omitten
    appendList = nl->FiveElemList(
                          nl->IntAtom(-3),  
                         // dummy filling up missing attribute name
                          nl->IntAtom(indexServer-1),
                          nl->IntAtom(indexLocal-1),
                          nl->IntAtom(indexRemote-1),
                          nl->IntAtom(indexOver));
  }


  // input ok, generate output
  ListExpr extAttr = nl->TwoElemList(
                    nl->TwoElemList(
                            nl->SymbolAtom("ErrorCode"), 
                            listutils::basicSymbol<CcInt>()),
                    nl->TwoElemList(
                            nl->SymbolAtom("ErrMsg"), 
                            listutils::basicSymbol<FText>()) );

  ListExpr newAttrList = listutils::concat(attrList, extAttr);
  if(!listutils::isAttrList(newAttrList)){
    return listutils::typeError("Attribute ErrorCode or ErrMsg "
                                "already present in tuple");
  }
  ListExpr resultList = nl->TwoElemList(
                            listutils::basicSymbol<Stream<Tuple> >(),
                            nl->TwoElemList(
                                listutils::basicSymbol<Tuple>(),
                                newAttrList));

  return nl->ThreeElemList( nl->SymbolAtom(Symbols::APPEND()),
                            appendList,
                            resultList);  
}


/*
1.7.2 Class ~transferFileListener~

This class is an interface for listening running file transfers.

*/

class transferFileListener{

  public:


/*
~transferFinished~

This function is called if a single transfer is finished. 


*/   
    virtual void  transferFinished(Tuple* intuple, 
                               int errorCode, 
                               const string& msg ) =0;

/*
~setInputTupleNumber~

This function is called if the number of input tuples is known, i.e.
the input stream is completely processed or the processing is
canceled.

*/
    virtual void setInputTupleNumber(int no)=0;
};


/*
struct ~transfer~

This is a class holding the information about a single 
file transfer.

*/
struct transfer{
  transfer(Tuple* _tuple, const string& _local, const string& _remote,
           const bool _allowOverwrite):
    inTuple(_tuple), local(_local), remote(_remote), 
    allowOverwrite(_allowOverwrite){}

  Tuple* inTuple;
  string local;
  string remote;
  bool allowOverwrite;
};

/*
class ~FileTransferator~

This class organizes the transfer of files for a single connection.
All file transfers are handles sequentially. It is possible to add
new transfer tasks during a running file transfer. Furthermore, it
is possible to stop the processing. In the case of stop, the last
started transfer must be finished, since the interface does not
allow to interrupt a transfer.

*/


class sendFunctor{
 public:
  int operator()(const int server, const string& source, 
                 const string& target, const bool allowOverwrite){
    return algInstance->sendFile(server, source, target, allowOverwrite);
  }
};

class requestFunctor{
 public:
  int operator()(const int server, const string& source, 
                 const string& target, const bool allowOverwrite){
    return algInstance->requestFile(server, source, target, allowOverwrite);
  }
};


template<class T>
class fileTransferator{
  public:

/*
~Constructor~

This constructs a filetranferator for a given connection (server) with a
single listener.

*/
     fileTransferator(int _server, transferFileListener* _listener) :
         server(_server), listener(_listener), stopped(false){
         runnerThread = boost::thread(
                        boost::bind(&fileTransferator::run, this));
     }


/*
~Destructor~

This will destroy the instance of this class. 
The last running transfer (if there is one) will be finsihed during 
destroying this object.

*/
     ~fileTransferator(){
        stop();
        runnerThread.join();
        listAccess.lock();
        while(!pendingtransfers.empty()){
           transfer t = pendingtransfers.front();
           t.inTuple->DeleteIfAllowed();
           pendingtransfers.pop_front();
        }
        listAccess.unlock();
     }

/*
~stop~

This will stop the thread processing pending transfers.
After calling this function, this object cannot be 
restarted. The only thing you can do after calling stop 
it to destroy it.

*/    
     void stop(){
         {
            boost::lock_guard<boost::mutex> lock(condmtx);
            stopped = true;
          }
          cond.notify_one();
     }


/*
~addtransfer~

Adds a new transfer to this list. If there is no other transfer
active, the transfer will start immediately. Otherwise the transfer
will be handled as the last one.

*/

      void addTransfer(Tuple* inTuple, const string& local, 
                       const string& remote, const bool allowOverwrite) {
          transfer newTransfer(inTuple, local, remote, allowOverwrite);
          listAccess.lock();
          {
            boost::lock_guard<boost::mutex> lock(condmtx);
            pendingtransfers.push_back(newTransfer);
          }
          listAccess.unlock();
          cond.notify_one();
      }

  private:


/*
~run~

This function works within an own thread. It waits unil new transfer tasks
are available, tankes the first one, and starts the transfer.

*/
      void run(){
          boost::unique_lock<boost::mutex> lock(condmtx);
          while(!stopped){
               // wait for date
               listAccess.lock();
               while(!stopped && pendingtransfers.empty()){
                   listAccess.unlock();
                   cond.wait(lock); 
                   listAccess.lock();
               }
               listAccess.unlock();
               if(!stopped){
                   listAccess.lock();
                   transfer t = pendingtransfers.front();
                   pendingtransfers.pop_front();   
                   listAccess.unlock();
                   int ret = functor(server, t.local, t.remote, 
                                     t.allowOverwrite);
                   if(listener){
                       if(ret == -3){ // algebra specific error code
                          listener->transferFinished(t.inTuple, ret, 
                                              "Conection number invalid.");
                       }  else if(ret==0){ // no error
                          listener->transferFinished(t.inTuple, ret, "");
                       } else { // general error
                          listener->transferFinished(t.inTuple, ret, 
                                   SecondoInterface::GetErrorMessage(ret) );
                       }
                   } else {
                      t.inTuple->DeleteIfAllowed();
                   }
               }
          }
      }

    int server;
    transferFileListener* listener;
    bool stopped;
    list<transfer>  pendingtransfers;
    boost::mutex listAccess;
    boost::thread runnerThread;
    boost::mutex condmtx;
    boost::condition_variable cond;
    T functor;
};


/*

Class ~ptransferFileInputProcessor~

This class processes in input tuples of the stream for the
parallel file transfer.

*/

template<class L, class R, class T>
class ptransferFileInputProcessor{
     public:

/*
~Constructors~

*/
        ptransferFileInputProcessor(Word& _stream, 
                                   int _serverIndex, int _localIndex, 
                       int _remoteIndex, int _overwriteIndex,
                       transferFileListener* _listener):
          stream(_stream), serverIndex(_serverIndex), localIndex(_localIndex),
          remoteIndex(_remoteIndex), overwriteIndex(_overwriteIndex),
          listener(_listener), stopped(false) 
       {
          stream.open();
        }



/*
~Destructors~

*/
        ~ptransferFileInputProcessor(){
           stream.close();
           // stop active threads
           typename map<int, fileTransferator<T>*>::iterator it;
           for(it = activeThreads.begin(); it!=activeThreads.end(); it++){
               it->second->stop();
           }
           // delete active threads
           for(it = activeThreads.begin(); it!=activeThreads.end(); it++){
               delete it->second;
           }
         }


/*
~stop~

This will stop the processing of input tuples.

*/
         void stop(){
            stopped=true;
         }


/*
~run~

This function processes the tuples of the input stream.

*/
         void run(){
            Tuple* inTuple;
            inTuple = stream.request();
            int noInTuples = 0; 
            while(inTuple && !stopped) {
               noInTuples++;
               CcInt* Server = (CcInt*) inTuple->GetAttribute(serverIndex);
               L* Local = (L*) inTuple->GetAttribute(localIndex);
               R* Remote = (R*) inTuple->GetAttribute(remoteIndex);
               CcBool* Overwrite = 0;
               CcBool defaultOverwrite(true,false);
               if(overwriteIndex>=0){
                  Overwrite = (CcBool*) inTuple->GetAttribute(overwriteIndex);
               } else {
                  Overwrite = &defaultOverwrite;
               }

               if(!Server->IsDefined() || !Local->IsDefined()
                  || !Remote->IsDefined() || !Overwrite->IsDefined()){
                   processInvalidTuple(inTuple, "undefined value found");
               } else {
                  int server = Server->GetValue(); 
                  string localname = Local->GetValue();
                  string remotename = Remote->GetValue();
                  bool overwrite = Overwrite->GetValue();
                  if(server < 0 || server >= algInstance->noConnections()){
                     processInvalidTuple(inTuple, "invalid server number");
                  } else {
                    processValidTuple(inTuple, server, localname, 
                                      remotename, overwrite);
                  }
               }
               inTuple = stream.request();
            }
            if(inTuple){
                inTuple->DeleteIfAllowed();
            }
            listener->setInputTupleNumber(noInTuples);
          }


    private:
       Stream<Tuple> stream;  // input stream
       int serverIndex;       // where to find server index in input tuple
       int localIndex;        // where to find the local file name
       int remoteIndex;       // where to find the remote file name
       int overwriteIndex;    // where to find the remote file name, 
                              // < 0 if default
       transferFileListener* listener; // reference to the listener object 
       bool stopped;          // flag for activiness
       map<int, fileTransferator<T>*>
                             activeThreads; //threads for each connection


/*
~processInvalidTuple~

This function is called if an invalid tuple is determined in the input stream.
This is the case if one of the required attributes is undefined or has an
invalid value, e.g., a negative server number.

*/
       void processInvalidTuple(Tuple* inTuple, const string& msg){
            if(listener){
               listener->transferFinished(inTuple, -3, msg);
            } else {
                inTuple->DeleteIfAllowed();
            }
       }


/*
~processValidTuple~

This function is called for each valid input tuple. If there not yet
a processing instance for the given server number, a new instance 
is created. The transfer task is added to the running instance for this
server.


*/
       void processValidTuple(Tuple* inTuple, const int server, 
                              const string& local, const string& remote,
                              const bool allowOverwrite){

          if(activeThreads.find(server)==activeThreads.end()){
              fileTransferator<T>* fi =
                               new fileTransferator<T>(server, listener);
              activeThreads[server] = fi; 
          } 
          activeThreads[server]->addTransfer(inTuple, 
                                           local,remote, allowOverwrite);
       }
  };



/*
Class ~ptransferLocal~

This is the local info class for the ptransfer operator. 

*/

template<class L, class R, class T>
class ptransferLocal: public transferFileListener {

  public:


/*
~Constructor~

*/

     ptransferLocal(Word& stream, int _serverindex,
                    int _localindex, int  _remoteindex,
                    int _overwrite, ListExpr _tt) {
        inputTuples = -1; // unknown number of input tuples
        outputTuples = 0; // up to now, there is no output tuple
        tt = new TupleType(_tt);
        inputProcessor = 
              new ptransferFileInputProcessor<L,R,T>(stream,_serverindex, 
                                           _localindex, _remoteindex,
                                           _overwrite, this);
        runner = new boost::thread( boost::bind(
                &ptransferFileInputProcessor<L,R,T>::run, inputProcessor));
     }

/*
~Destructor~

This will wait until all running transfers are processed. This does not
includes instantiates but not yet started transfers. All present but
not used result tuples are deleted also.

*/
     virtual ~ptransferLocal(){
        inputProcessor->stop(); // stop processing
        runner->join(); // wait until started jobs are done
        delete runner;
        delete inputProcessor;

        list<Tuple*>::iterator it;
        // remove already created result tuples
        listAccess.lock();
        for(it = resultList.begin();it!=resultList.end();it++){
            (*it)->DeleteIfAllowed();
        }
        listAccess.unlock();
        tt->DeleteIfAllowed();
     }


/*
~next~

This function provides the next result tuple.
If there is no result tuple available, it waits 
until it is. 

*/

     Tuple* next(){
        boost::unique_lock<boost::mutex> lock(resultMutex);
        listAccess.lock();
        while(resultList.empty()){
            listAccess.unlock();
            resultAvailable.wait(lock);
            listAccess.lock();
        }
        Tuple* res = resultList.front();
        resultList.pop_front();
        listAccess.unlock();
        return res;
     } 


/*
~transferFinished~

This function is called if a transfer has been finished. 
It will created a new result tuple from the input tuple,
the success flag and the error message.

*/

     // callback function for a single finished transfer
     virtual void transferFinished(Tuple* inTuple,int errorCode, 
                                   const string& msg){

        tupleCreationMtx.lock();
        Tuple* resTuple =  new Tuple(tt);
        // copy original attributes
        for(int i=0;i<inTuple->GetNoAttributes(); i++){
           resTuple->CopyAttribute(i,inTuple,i);
        } 
        resTuple->PutAttribute(inTuple->GetNoAttributes(), 
                               new CcInt(true, errorCode));
        resTuple->PutAttribute(inTuple->GetNoAttributes() + 1, 
                               new FText(true, msg));

        inTuple->DeleteIfAllowed();

        {
           boost::lock_guard<boost::mutex> lock(resultMutex);

           listAccess.lock();
           resultList.push_back(resTuple);
           listAccess.unlock();

           // update counter
           outputTuples++;
          if(inputTuples==outputTuples){
             // this was the last tuple to create
             listAccess.lock();
             resultList.push_back(0);
             listAccess.unlock();
           }
        }
        tupleCreationMtx.unlock();
        resultAvailable.notify_one();
     }


/*
~setInputTupleNumber~

This function is called if the number of input tuples is known.


*/   
     virtual void setInputTupleNumber(int i){
        if(i==0){ // we cannot expect any results
           listAccess.lock();
           {
             boost::lock_guard<boost::mutex> lock(resultMutex);
             resultList.push_back(0);
           }
           listAccess.unlock();
           // inform about new tuple.
           resultAvailable.notify_one(); 
        } else {
          assert(i>0);
          countMtx.lock();
          inputTuples = i;
          if(inputTuples == outputTuples){
             // all output tuples was generated
             countMtx.unlock();
             {
               boost::lock_guard<boost::mutex> lock(resultMutex);
               listAccess.lock();
               resultList.push_back(0);
               listAccess.unlock();
             }
             // inform about new tuple.
             resultAvailable.notify_one(); 
          } else {
             countMtx.unlock();
          }
        }
     }


  private:
     TupleType* tt;
     bool finished;
     ptransferFileInputProcessor<L,R,T>* inputProcessor;
     boost::thread* runner;
     list<Tuple*> resultList;
     boost::condition_variable resultAvailable;
     boost::mutex resultMutex;
     boost::mutex listAccess;
     boost::mutex tupleCreationMtx;
     int inputTuples;
     int outputTuples;
     boost::mutex countMtx;
};


/*
~Value Mapping~

*/

template<class L , class R, class T>
int ptransferFileVMT( Word* args, Word& result, int message,
               Word& local, Supplier s ){

   ptransferLocal<L,R,T>* li = (ptransferLocal<L,R,T>*) local.addr;

   switch(message){
      case OPEN: {
               if(li) delete li;
               int serv = ((CcInt*)args[5].addr)->GetValue();
               int loc = ((CcInt*)args[6].addr)->GetValue();
               int rem = ((CcInt*)args[7].addr)->GetValue();
               int over = ((CcInt*) args[8].addr)->GetValue(); 
               ListExpr tt = nl->Second(GetTupleResultType(s));
               local.addr = new ptransferLocal<L,R,T>(args[0], serv, loc, 
                                                     rem, over, tt);
               return 0;
          }
      case REQUEST:
               result.addr = li?li->next():0;
               return result.addr?YIELD:CANCEL;
      case CLOSE:
             if(li){
                delete li;
                local.addr = 0;                  
             }
             return 0;
   }
   return -1;
}

/*
1.7.2 Specification

*/

OperatorSpec psendFileSpec(
     " stream(tuple) x attrName x attrName x attrName -> stream(Tuple + Ext)",
     " _ psendFile[ ServerAttr, LocalFileAttr, RemoteFileAttr]  ",
     " Transfers local files to remote servers in parallel. The ServerAttr "
     " must point to an integer attribute specifying the server number. "
     " The localFileAttr and remoteFileAttr arguments mark the attribute name"
     " for the local and remote file name, respectively. Both arguments can be"
     " of type text or string", 
     " query fileRel feed psendFile[0, LocalFile, RemoteFile] consume");


OperatorSpec prequestFileSpec(
     " stream(tuple) x attrName x attrName x attrName -> stream(Tuple + Ext)",
     " _ prequestFile[ ServerAttr, RemoteFileAttr, LocalFileAttr]  ",
     " Transfers remote files to local file system  in parallel. "
     "The ServerAttr "
     " must point to an integer attribute specifying the server number. "
     " The localFileAttr and remoteFileAttr arguments mark the attribute name"
     " for the local and remote file name, respectively. Both arguments can be"
     " of type text or string", 
     " query fileRel feed prequestFile[0, LocalFile, RemoteFile] consume");

/*
1.7.3 Value Mapping Array and Selection

*/

ValueMapping psendFileVM[] = {
  ptransferFileVMT<CcString, CcString, sendFunctor>,
  ptransferFileVMT<CcString, FText, sendFunctor>,
  ptransferFileVMT<FText, CcString, sendFunctor>,
  ptransferFileVMT<FText, FText, sendFunctor>
};

ValueMapping prequestFileVM[] = {
  ptransferFileVMT<CcString, CcString, requestFunctor>,
  ptransferFileVMT<CcString, FText, requestFunctor>,
  ptransferFileVMT<FText, CcString, requestFunctor>,
  ptransferFileVMT<FText, FText, requestFunctor>
};



int ptransferFileSelect(ListExpr args){
  string name1 = nl->SymbolValue(nl->Third(args));
  string name2 = nl->SymbolValue(nl->Fourth(args));
  ListExpr attrList = nl->Second(nl->Second(nl->First(args)));
  ListExpr type1, type2;
  listutils::findAttribute(attrList, name1, type1);
  listutils::findAttribute(attrList, name2, type2);
  int n1 = CcString::checkType(type1)?0:2;
  int n2 = CcString::checkType(type2)?0:1;

  return n1+n2; 
}

/*
1.6.4 Operator instance

*/

Operator psendFileOp (
    "psendFile",             //name
     psendFileSpec.getStr(), //specification
     4,
     psendFileVM,        //value mapping
     ptransferFileSelect,   //trivial selection function
     ptransferFileTM        //type mapping
);

Operator prequestFileOp (
    "prequestFile",             //name
     prequestFileSpec.getStr(), //specification
     4,
     prequestFileVM,        //value mapping
     ptransferFileSelect,   //trivial selection function
     ptransferFileTM        //type mapping
);

/*
1.7 Operators ~getRequestFolder~ and ~getTransferFolder~

These operator are indeed to get the folder for sending and 
receiving files on remote servers.

1.7.1 Type Mapping

*/
ListExpr getFoldersTM(ListExpr args){

  string err = "int expected";

  if(!nl->HasLength(args,1)){
    return listutils::typeError(err);
  }
  if(!CcInt::checkType(nl->First(args))){
    return listutils::typeError(err);
  } 
  return listutils::basicSymbol<FText>();
}


/*
1.7.2 Value Mapping

*/

template<bool send>
int getFolderVM(Word* args, Word& result, int message,
                Word& local, Supplier s ){

  CcInt* server = (CcInt*) args[0].addr;
  result = qp->ResultStorage(s);
  FText* res = (FText*) result.addr;
  if(!server->IsDefined()){
     res->SetDefined(false);
     return 0;
  }
  int ser = server->GetValue();
  if(ser<0 || ser >=algInstance->noConnections()){
     res->SetDefined(false);
     return 0;
  }
  if(send) {
     res->Set(true, algInstance->getSendFolder(ser));
  } else {
     res->Set(true, algInstance->getRequestFolder(ser));
  }
  return 0;
}


OperatorSpec getRequestFolderSpec(
     " int -> text ",
     " getRequestFolder(_)  ",
     " Returns the name of the folder on the server for a given connection"
     " from where files are requested. ",
     " query getRequestFolder(0)");

OperatorSpec getSendFolderSpec(
     " int -> text ",
     " getSendFolder(_)  ",
     " Returns the name of the folder on the server for a given connection"
     " into which files are written. ",
     " query getSendFolder(0)");


Operator getRequestFolderOp (
    "getRequestFolder",             //name
     getRequestFolderSpec.getStr(), //specification
     getFolderVM<false>,        //value mapping
     Operator::SimpleSelect,   //trivial selection function
     getFoldersTM        //type mapping
);

Operator getSendFolderOp (
    "getSendFolder",             //name
     getSendFolderSpec.getStr(), //specification
     getFolderVM<true>,        //value mapping
     Operator::SimpleSelect,   //trivial selection function
     getFoldersTM        //type mapping
);


/*
1.9 Operator ~pconnect~

This operator connects to different Servers in parallel.

1.9.1 Type Mapping

The operator receives an tuple stream and three attribute names.
The first attribute determines the host name and points to a string
or a text attribute. The second attribute name determines the port und
points to an integer attribute. The third attribute name determines the
SecondoConfig.ini file and may be of type text or of type string.

*/

ListExpr pconnectTM(ListExpr args){

  string err = "stream(tuple) x ID x ID x ID  expected";
  if(!nl->HasLength(args,4) && !nl->HasLength(args,5)){
    return listutils::typeError(err);
  }
  if(!Stream<Tuple>::checkType(nl->First(args))){
    return listutils::typeError("The first attribute is not a tuple stream");
  }
  ListExpr a1 = nl->Second(args);
  ListExpr a2 = nl->Third(args);
  ListExpr a3 = nl->Fourth(args);
  if(nl->AtomType(a1) != SymbolType || 
     nl->AtomType(a2) != SymbolType ||
     nl->AtomType(a3) != SymbolType){
    return listutils::typeError(err);
  }

  string n1 = nl->SymbolValue(a1);
  string n2 = nl->SymbolValue(a2);
  string n3 = nl->SymbolValue(a3);

  ListExpr type;
  ListExpr attrList = nl->Second(nl->Second(nl->First(args)));

  int hostIndex = listutils::findAttribute(attrList, n1, type);
  if(!hostIndex){
    return listutils::typeError("Attribute " + n1 + " not found");
  }
  if(!CcString::checkType(type) && !FText::checkType(type)){
     return listutils::typeError("Attribute " + n1 + 
                                 " not of type string or type text");
  }
  int portIndex = listutils::findAttribute(attrList, n2, type);
  if(!portIndex){
    return listutils::typeError("Attribute " + n2 + " not found");
  }
  if(!CcInt::checkType(type) ){
     return listutils::typeError("Attribute " + n2 + 
                                 " not of type int");
  }
  int configIndex = listutils::findAttribute(attrList, n3, type);
  if(!configIndex){
    return listutils::typeError("Attribute " + n3 + " not found");
  }
  if(!CcString::checkType(type) && !FText::checkType(type)){
     return listutils::typeError("Attribute " + n3 + 
                                 " not of type string or type text");
  }

  if(listutils::findAttribute(attrList, "CNo",type)){
    return listutils::typeError("incoming tuple stream already "
                                "contains CNo attribute");
  }

  ListExpr resAttrList = listutils::concat(attrList, 
                              nl->OneElemList(nl->TwoElemList(
                                      nl->SymbolAtom("CNo"), 
                                     listutils::basicSymbol<CcInt>())));

  ListExpr appendList;
  if(nl->HasLength(args,5)){
     if(!CcBool::checkType(nl->Fifth(args))){
       return listutils::typeError("5th argument must be of type bool");
     }
     appendList = nl->ThreeElemList(
                           nl->IntAtom(hostIndex -1),
                           nl->IntAtom(portIndex -1),
                           nl->IntAtom(configIndex -1));
  } else {
     appendList = nl->FourElemList(
                           nl->BoolAtom(false),
                           nl->IntAtom(hostIndex -1),
                           nl->IntAtom(portIndex -1),
                           nl->IntAtom(configIndex -1));

  }

  ListExpr resList = nl->TwoElemList(
                      listutils::basicSymbol<Stream<Tuple> >(),
                      nl->TwoElemList(
                           listutils::basicSymbol<Tuple>(),
                           resAttrList));
  return nl->ThreeElemList( nl->SymbolAtom(Symbols::APPEND()),
                            appendList,
                            resList); 

}


/*
1.7.2 auxiliary classes


class ~ConnectionListener~

This class contains a callback function that is called when
a attempt for a connection is done. If the attempt was
successful, no is the connection number, other wise no
is negative.

*/
class ConnectionListener{

  public:
    virtual void connectionDone(Tuple* inputTuple, 
                                int no, ConnectionInfo* ci ) =0;

};


/*
Class ~Connector~

This class implements a single attempt to connect 
with a remote server. Directly after calling the constructor,
a thread is start doing the connection attempt.

*/

class Connector{

  public:
     Connector(Tuple* _inTuple, const int _inTupleNo,
               const string& _host, const int _port, 
               const string& _config, ConnectionListener* _listener):
       inTuple(_inTuple), inTupleNo(_inTupleNo), host(_host), port(_port),
       config(_config), 
       listener(_listener){
       mythread = boost::thread(
                      boost::bind(&Connector::run, this));
     }

     ~Connector(){
         mythread.join();
      }

  private:
     Tuple* inTuple;
     int inTupleNo;
     string host;
     int port;
     string config;
     ConnectionListener* listener;
     boost::thread mythread;


     void run(){
        if(port<0 || host.empty() || config.empty()){
           listener->connectionDone(inTuple, inTupleNo, 0);
           return;
        }

        NestedList* mynl = new NestedList();
        SecondoInterfaceCS* si = new SecondoInterfaceCS(false, mynl);
        si->setMaxAttempts(4);
        si->setTimeout(1);
        string errMsg;
        if(!si->Initialize( "", "", host, stringutils::int2str(port),
                           config,errMsg, true)){
           listener->connectionDone(inTuple, inTupleNo, 0);
           delete si;
           delete mynl;
        } else {
           ConnectionInfo* ci = new ConnectionInfo(host,port,config,si,mynl);
           listener->connectionDone(inTuple, inTupleNo, ci);
        }
     }
};


class ConnectInfoElem{

public:

  ConnectInfoElem(Tuple* _inTuple, int _inTupleNo, ConnectionInfo* _ci):
   inTuple(_inTuple), inTupleNo(_inTupleNo), ci(_ci){}


   bool operator<(const ConnectInfoElem& rhs)const{
      return inTupleNo < rhs.inTupleNo;   
   }
   
   bool operator==(const ConnectInfoElem& rhs) const{
      return inTupleNo == rhs.inTupleNo;   
   }

   bool operator>(const ConnectInfoElem& rhs)const{
      return inTupleNo > rhs.inTupleNo;   
   }

  Tuple* inTuple;
  int inTupleNo;
  ConnectionInfo* ci;

};


template<class H, class C>
class pconnectLocal : public ConnectionListener{
  public:
    pconnectLocal(Word& _stream, int _hostIndex, int _portIndex,
                  int _configIndex, ListExpr _tt, bool _det):
       stream(_stream), hostIndex(_hostIndex), portIndex(_portIndex), 
       configIndex(_configIndex), inputTuples(-1), outputTuples(0), 
       stopped(false), det(_det) {
        tt = new TupleType(_tt);
        stream.open(); 
        runnerThread = boost::thread(
                        boost::bind(&pconnectLocal::run, this));
    }

    virtual ~pconnectLocal(){
       stop();
       runnerThread.join();
       stream.close();
       for(size_t i=0;i<connectors.size(); i++){
         delete connectors[i];
       }
       tt->DeleteIfAllowed(); 
       cachemut.lock();
       while(!cache.empty()){
         ConnectInfoElem elem = cache.top();
         cache.pop();
         elem.inTuple->DeleteIfAllowed();
         delete elem.ci;
       }
       cachemut.unlock();
    }


    Tuple* next(){
       boost::unique_lock<boost::mutex> lock(waitmut);
       while(resultTuples.empty()){
          cond.wait(lock);
       }
       Tuple* res = resultTuples.front();
       resultTuples.pop_front();
       return res;
    } 

    void stop(){
      stopped = true;
    }


    virtual void connectionDone(Tuple* inTuple, int inTupleNo, 
                                ConnectionInfo* ci){
       if(!det){
          connectionDoneNonDet(inTuple,inTupleNo, ci);
       } else {
          connectionDoneDet(inTuple, inTupleNo, ci);      
       }
    }

       

  private:
     Stream<Tuple> stream;
     int hostIndex;
     int portIndex;
     int configIndex;
     int inputTuples;
     int outputTuples;
     bool stopped;
     bool det;
     TupleType* tt;
     boost::mutex mut;
     boost::mutex waitmut;
     boost::mutex outmut;
     boost::condition_variable cond;
     list<Tuple*> resultTuples;
     boost::thread runnerThread;
     vector<Connector*> connectors;
     // cache for deterministic variant
     priority_queue<ConnectInfoElem, 
                    vector<ConnectInfoElem>, 
                    greater<ConnectInfoElem> > cache;
     boost::mutex cachemut;
     

     void run(){
        Tuple* inTuple;
        int noTuples = 0;
        inTuple = stream.request();
        while(!stopped && inTuple!=0){
           noTuples++;
           H* Host = (H*) inTuple->GetAttribute(hostIndex);
           CcInt* Port = (CcInt*) inTuple->GetAttribute(portIndex);
           C* Config = (C*) inTuple->GetAttribute(configIndex);
           if(   !Host->IsDefined() || !Port->IsDefined() 
              || !Config->IsDefined()){
              Connector* connector = new Connector(inTuple, noTuples,
                                                    "",-1,"",this);
              connectors.push_back(connector);
           } else {
              Connector* connector = new Connector(inTuple, 
                                        noTuples, Host->GetValue(), 
                                  Port->GetValue(), Config->GetValue(), this);
              connectors.push_back(connector);
           }
           inTuple = stream.request();
        }
        if(inTuple){
          inTuple->DeleteIfAllowed();
        }
        outmut.lock();
        inputTuples = noTuples;
        outmut.unlock();
        if(inputTuples == outputTuples){
          connectionDone(0,noTuples+1,0);
        }    
     }

/*
~connectionDoneNonDet~

In this variant, the order of output tuples as well as the order 
of servernumbers are not deterministic, i.e. the first connection which 
is established will be the
first connection and the first output tuple.

*/
    void connectionDoneNonDet(Tuple* inTuple, int inTupleNo, 
                              ConnectionInfo* ci){
        if(!inTuple){
            {
              boost::lock_guard<boost::mutex> lock(waitmut);
              resultTuples.push_back(0);
            }
            cond.notify_one();
            return;
        }
        mut.lock();
        if(stopped){ // do not produce more output tuples
           inTuple->DeleteIfAllowed();
           mut.unlock();
           return; 
        }
        mut.unlock();
        outmut.lock();
        Tuple* resTuple = new Tuple(tt);
        // copy attributes
        int noa = inTuple->GetNoAttributes();
        for(int i=0;i<noa ; i++){
           resTuple->CopyAttribute(i,inTuple,i);   
        }
        inTuple->DeleteIfAllowed();
        int no = algInstance->addConnection(ci);
        resTuple->PutAttribute(noa, new CcInt(no));
        {
          boost::lock_guard<boost::mutex> lock(waitmut);
          resultTuples.push_back(resTuple);
          outputTuples++;
          if(inputTuples==outputTuples){
             stop();
             resultTuples.push_back(0); // add end marker
          }
        }
        outmut.unlock();
        cond.notify_one();
    }

/*
~connectionDoneDet~

Deterministic variant. Here, the output tuples as well as the 
server numbers are determined by the 

*/
    void connectionDoneDet(Tuple* inTuple, int inTupleNo, 
                              ConnectionInfo* ci){


       cachemut.lock();
       cache.push(ConnectInfoElem(inTuple,inTupleNo,ci));
       ConnectInfoElem elem = cache.top();
       bool produce = elem.inTupleNo == outputTuples+1;
       while( produce && !stopped){
           cache.pop();
           connectionDoneNonDet(elem.inTuple,elem.inTupleNo, elem.ci);
           elem = cache.top();
           produce = elem.inTupleNo == outputTuples+1;
       }
       cachemut.unlock(); 
    }

};


/*
1.7.3 Value Mapping

*/
template<class H, class C>
int pconnectVMT(Word* args, Word& result, int message,
                Word& local, Supplier s ){

   pconnectLocal<H,C>* li = (pconnectLocal<H,C>*) local.addr;
   switch(message){
     case OPEN: {
            if(li) delete li;
            int hostIndex = ((CcInt*)args[5].addr)->GetValue();
            int portIndex = ((CcInt*)args[6].addr)->GetValue();
            int configIndex = ((CcInt*)args[7].addr)->GetValue();
            CcBool* det = (CcBool*) args[4].addr;
            if(!det->IsDefined()){
               local.addr = 0;
               return 0;
            }
            ListExpr tt = nl->Second(GetTupleResultType(s));
            local.addr  = new pconnectLocal<H,C>( args[0], hostIndex,  
                                  portIndex, configIndex, tt, det->GetValue());
            return 0;
          }
     case REQUEST:
            result.addr = li?li->next():0;
            return result.addr?YIELD:CANCEL;
     case CLOSE:
            if(li){
              delete li;
              local.addr = 0;
            }
            return 0;
   }
   return -1;
}

/*
1.7.3 Value Mapping Array and Selection

*/
ValueMapping pconnectVM[] = {
  pconnectVMT<CcString, CcString>,
  pconnectVMT<CcString, FText>,
  pconnectVMT<FText, CcString>,
  pconnectVMT<FText, FText>
};



int pconnectSelect(ListExpr args){
  string name1 = nl->SymbolValue(nl->Second(args));// hostattr
  string name2 = nl->SymbolValue(nl->Fourth(args));// configattr
  ListExpr attrList = nl->Second(nl->Second(nl->First(args)));
  ListExpr type1, type2;
  listutils::findAttribute(attrList, name1, type1);
  listutils::findAttribute(attrList, name2, type2);
  int n1 = CcString::checkType(type1)?0:2;
  int n2 = CcString::checkType(type2)?0:1;
  return n1+n2; 
}

/*
1.7.4 Specification

*/

OperatorSpec pconnectSpec(
     " stream(tuple(X)) x attrName x attrName x attrName "
     "-> stream(tuple(X + (CNo int))) ",
     " _ pconnect[_,_,_]  ",
     " Creates connection to Secondo servers in parallel.  "
     " The first argument is a stream of tuples. The parameters are "
     " the attribute name for the host (string or text), the attribute "
     " name for the port (int), and the attribute name for the "
     " local configuration file (string or text). By this operator, the "
     " input tuples are extended by the connection number. If the number is"
     " negative, no connection could be built up for this input tuple. "
     " The order of the output tuples depends on the connection speeds.",
     " query ConTable feed pconnect[Host, Port, Config] consume");

/*
1.7.5 Operator instance

*/

Operator pconnectOp (
    "pconnect",             //name
     pconnectSpec.getStr(),         //specification
     4,
     pconnectVM,        //value mapping
     pconnectSelect,   //trivial selection function
     pconnectTM        //type mapping
);


/*
1.8 Operator pquery

This operators performs the same query on a set of servers. The result type 
of the query is asked by a specified server. It's assumed that each server 
returns the same type and this type must be in kind DATA.

1.8.1 Type Mapping

The type mapping is

stream(int) x {string.text} x {int} -> stream(tuple(Server : int, Resul : DATA))

This operator uses argument evaluation in type mapping.

*/

bool  getQueryType(const string& query1, int serverNo, 
                   ListExpr& result, string& errMsg){
   string query = query1; 

   stringutils::trim(query);
   if(!stringutils::startsWith(query,"query")){
     return false;
   }
   query = query.substr(5); // remove leading query
   stringutils::trim(query);
   query = "query (" + query + ") getTypeNL";

   int error;
   ListExpr reslist;
   bool ok = algInstance->simpleCommand(serverNo, query, 
                                        error, errMsg, reslist, true);
   if(!ok){
      errMsg = " determining type command failed (invalid server number)";
      return false;
   }
   if(error!=0){
      errMsg = " error in type detecting command (" + errMsg+")";
      return false;
   }
   if(!nl->HasLength(reslist,2)){
      errMsg = "Invalid result get from remote server";
      return false;
   }
   if(!FText::checkType(nl->First(reslist))){
      errMsg = "Invalid result type for getTypeNL";
      return false;
   }
   if(nl->AtomType(nl->Second(reslist))!=TextType){
     errMsg = "getTypeNL returns invalid/undefined result";
     return false;
   }
   string typeList = nl->Text2String(nl->Second(reslist));
   ListExpr resType;
   nlparsemtx.lock();
   if(!nl->ReadFromString(typeList,resType)){
     nlparsemtx.unlock();
     errMsg = "getTypeNL returns no valid list expression";
     return false;
   }   
   nlparsemtx.unlock();
   if(nl->HasLength(resType,2)){
     ListExpr first = nl->First(resType);
     if(listutils::isSymbol(first, Stream<Tuple>::BasicType())){
        errMsg = "remote result is a stream";
        return false;
     }
   }
   result = resType;
   errMsg = "";
   return true;
}

ListExpr pqueryTM(ListExpr args){
  string err="stream(int) x {string.text} x {int} expected";
  if(!nl->HasLength(args,3)){
   return listutils::typeError(err + "( wrong number of args)" );
  }
  ListExpr stream = nl->First(nl->First(args)); // ignore query part
  if(!Stream<CcInt>::checkType(stream)){
    return listutils::typeError(err);
  }

  ListExpr second = nl->Second(args);

  // determine query
  string query;
  bool ok;
  if(FText::checkType(nl->First(second))){
      ok = getValue<FText>(nl->Second(second),query);
  } else if(CcString::checkType(nl->First(second))){
     ok = getValue<CcString>(nl->Second(second),query);
  } else {
      return listutils::typeError(err);
   }
   if(!ok){
      return listutils::typeError("Value of second argument invalid");
   }

   // determine type server
   ListExpr third = nl->Third(args);
   if(!CcInt::checkType(nl->First(third))){
      return listutils::typeError(err);
   }

   int server;
   if(!getValue(nl->Second(third),server)){
      return listutils::typeError("Could not determine server "
                                  "providing the type");
   }

   string errMsg;
   ListExpr resType;
   if(!getQueryType(query,server,resType,errMsg)){
      return listutils::typeError(errMsg);
   }


   // check whether resType is in kind data
   if(!listutils::isDATA(resType)){
      return listutils::typeError("Query result " + nl->ToString(resType) + 
                                  "  not in kind data");
   }

   ListExpr attrList = nl->TwoElemList(
                            nl->TwoElemList(
                               nl->SymbolAtom("ServerNo"),
                               listutils::basicSymbol<CcInt>() ),
                            nl->TwoElemList(
                               nl->SymbolAtom("Result"),
                               resType));

   ListExpr resList = nl->TwoElemList(
             listutils::basicSymbol<Stream<Tuple> >(),
             nl->TwoElemList( listutils::basicSymbol<Tuple>(), attrList));


    return resList; 


}


/*
1.8.2 LocalInfoClass

*/


class PQueryInfo : public CommandListener<ListExpr>{
  public:
    PQueryInfo(Word& _stream , const string& _query,  ListExpr _tt) : 
      stream(_stream), query(_query){
       tt = new TupleType(_tt);
       stream.open();
       stop = false;
       noInputs=-1;
       noOutputs = 0;
       // run processing thread
       nTypeList =nl->Second(nl->Second(nl->Second(_tt)));
       int algId;
       int typeId;
       string typeName;

       bool done = false;
       while(!done){
          if(nl->ListLength(nTypeList)<2){
             resultList.push_back(0);
             return;
          }
          if(nl->AtomType(nl->First(nTypeList))!=IntType){
               nTypeList = nl->First(nTypeList);
          } else {
             if(nl->AtomType(nl->Second(nTypeList))!=IntType){
                resultList.push_back(0);
                return;
             } else {
                algId = nl->IntValue(nl->First(nTypeList));
                typeId = nl->IntValue(nl->Second(nTypeList));
                done = true;
             }
          }
       } 

       AlgebraManager* am = SecondoSystem::GetAlgebraManager();
       inObject = am->InObj(algId,typeId);
       createObject = am->CreateObj(algId,typeId);

       runner = boost::thread(boost::bind(&PQueryInfo::run,this));
    }

    virtual ~PQueryInfo(){
        stop = true;
        runner.join();

        map<int,ConnectionTask<ListExpr>*>::iterator it;
        for(it = connectors.begin(); it!=connectors.end(); it++){
           it->second->stop();
        }
        for(it = connectors.begin(); it!=connectors.end(); it++){
           delete it->second;
        }
        while(!resultList.empty()){
          Tuple* t = resultList.front();
          resultList.pop_front();
          t->DeleteIfAllowed();
        }
        stream.close();
        tt->DeleteIfAllowed();
    }

    Tuple* next(){
       boost::unique_lock<boost::mutex> lock(resmtx);
       while(resultList.empty()){
          cond.wait(lock);
       }
       Tuple* res = resultList.front();
       resultList.pop_front();
       return res;
    }

    virtual void jobDone(int id, string& command, size_t no, int error, 
                       string& errMsg, ListExpr& valueList){

       {
      
          boost::lock_guard<boost::mutex> guard(resmtx);
          bool correct;    
          Word res;
          Tuple* resTuple = new Tuple(tt);
          resTuple->PutAttribute(0, new CcInt(id));
          int errorPos = 0;
          ListExpr errorInfo = listutils::emptyErrorInfo();
          if(nl->HasLength(valueList,2)){
              res = inObject(nTypeList, nl->Second(valueList), 
                         errorPos, errorInfo, correct);
          } else {
             correct = false;
          }
      
          if(correct){ // nested list could not read in correctly
             resTuple->PutAttribute(1, (Attribute*) res.addr); 
          } else {
             Attribute* attr = (Attribute*) createObject(nTypeList).addr;
             attr->SetDefined(false);
             resTuple->PutAttribute(1,attr); 
          }
          resultList.push_back(resTuple);
          noOutputs++;
          if(noInputs==noOutputs){
           resultList.push_back(0);
          }
       }
       cond.notify_one();
    }

  private:
    Stream<CcInt> stream;
    ListExpr nTypeList;
    TupleType* tt;
    string query;
    int noInputs;
    int noOutputs;
    list<Tuple*> resultList;
    boost::mutex resmtx;
    boost::thread runner;
    bool stop;
    map<int,ConnectionTask<ListExpr>*> connectors;
    boost::mutex mapmtx;
    boost::condition_variable cond;
    InObject inObject;
    ObjectCreation createObject; 
    

    
    void run(){
       CcInt* inInt;
       inInt = stream.request();
       int count =0;
       while(inInt && !stop){
          if(inInt->IsDefined()){
             int s = inInt->GetValue();
             if(algInstance->serverExists(s)){
               count++;
               startQuery(s);
             }
          }
          inInt->DeleteIfAllowed();
          inInt = stream.request();
       }
       if(inInt){
          inInt->DeleteIfAllowed();
       }
       noInputs = count;
       if(noInputs==noOutputs){
           {  boost::unique_lock<boost::mutex> lock(resmtx);
              resultList.push_back(0);
           }
           cond.notify_one();
       }
    }

    void startQuery(int s){
       boost::lock_guard<boost::mutex> guard(mapmtx);
       if(connectors.find(s)==connectors.end()){
          connectors[s] = new ConnectionTask<ListExpr>(s,this);
       }
       connectors[s]->addCommand(s,query);
     }
};



/*
1.8.3 Value Mapping

*/
template<class T>
int pqueryVMT(Word* args, Word& result, int message,
                Word& local, Supplier s ){

   PQueryInfo* li = (PQueryInfo*) local.addr;
   switch(message){
     case OPEN:{
           if(li){
               delete li;
               local.addr = 0;
           }
           ListExpr tt = nl->Second(GetTupleResultType(s));
           T* query = (T*) args[1].addr;
           if(!query->IsDefined()){
              return 0;
           }
           local.addr = new PQueryInfo(args[0], query->GetValue(), tt);
           return 0;
       }
     case REQUEST:
             result.addr = li?li->next():0;
             return result.addr?YIELD:CANCEL;
     case CLOSE:
            if(li){
               delete li;
               local.addr = 0;
            }
            return 0;
   }
   return -1;
}

/*
1.8.4 Value Mapping and selection

*/

ValueMapping pqueryVM[] = {
  pqueryVMT<CcString>,
  pqueryVMT<FText>
};

int pquerySelect(ListExpr args){
  return CcString::checkType(nl->Second(args))?0:1;
}

/*
1.8.5 Sepcification

*/
OperatorSpec pquerySpec(
     " stream(int) x {text, string} x int -> stream(tuple(int, DATA))",
     " _ pquery[_,_]  ",
     " Performs the same query on a set of remote servers.  "
     " The first argument is a stream of integer specifying the servers."
     " The second argument is the query to execute. The result of this "
     " query has to be in kind DATA. The third argument is a server number"
     " for determining the exact result type of the query. The result is"
     " a stream of tuples consisting of the server number and the result"
     " of this server.",
     " query intstream(0,3) pquery['query ten count', 0] sum[Result]");

/*
1.8.6

Operator instance

*/
Operator pqueryOp (
    "pquery",             //name
     pquerySpec.getStr(),         //specification
     2,
     pqueryVM,        //value mapping
     pquerySelect,   //trivial selection function
     pqueryTM        //type mapping
);


/*
1.9 Operator ~pquery2~

This operator works quite similar as the pquery operator. The difference is
that the first argument is a tuple stream with at least two integer values.
The first value corresponds to the server number (like in pquery), the 
second number is a parameter for the remote query. The remote query contains 
somewhere the keywords SERVER and PART which are replaced by the 
corresponding values. For determining the result type, some
default parameters for part and server are given.


1.9.1 Type Mapping

*/

ListExpr pquery2TM(ListExpr args){

  string err = "stream(tuple) x {string,text} x AttrName x AttrName "
               "x int x int expected";
  if(!nl->HasLength(args,6)){
    return listutils::typeError(err + " (invalid number of args)");
  }

  ListExpr stream = nl->First(args);
  if(!nl->HasLength(stream,2)){
     return listutils::typeError("internal error");
  }
  stream = nl->First(stream);
  if(!Stream<Tuple>::checkType(stream)){
    return listutils::typeError(err + " (first arg is not a tuple stream)");
  }
  ListExpr query = nl->Second(args);
  if(!nl->HasLength(query,2)){
    return listutils::typeError("internal error");
  }
  query = nl->First(query);
  if(!CcString::checkType(query) && !FText::checkType(query)){
     return listutils::typeError(err + " (second arg is not of string or "
                                 "text)");
  }
  ListExpr serverNo = nl->Third(args);
  if(!nl->HasLength(serverNo,2)){
     return listutils::typeError("internal error");
  }
  serverNo = nl->First(serverNo);
  if(nl->AtomType(serverNo)!=SymbolType){
     return listutils::typeError(err +" (third arg is not an attr name)");
  }
  ListExpr partNo = nl->Fourth(args);
  if(!nl->HasLength(partNo,2)){
     return listutils::typeError("internal error");
  }
  partNo = nl->First(partNo);
  if(nl->AtomType(partNo)!=SymbolType){
     return listutils::typeError(err +" (fourth arg is not an attr name)");
  }
  if(!CcInt::checkType(nl->First(nl->Fifth(args)))){
     return listutils::typeError(err +" (fifth arg is not an int)");
  }
  if(!CcInt::checkType(nl->First(nl->Sixth(args)))){
     return listutils::typeError(err +" (sixth arg is not an int)");
  }
  // check for presence and int type of attributes in tuple stream
  ListExpr attrList = nl->Second(nl->Second(stream));
  ListExpr attrType;
  string sp = nl->SymbolValue(serverNo);
  int serverPos = listutils::findAttribute(attrList, sp,  attrType);
  if(!serverPos){
     return listutils::typeError("attribute " + sp + " not found");
  }
  if(!CcInt::checkType(attrType)){
    return listutils::typeError("attribute " + sp + " not of type int");
  }
  string pp = nl->SymbolValue(partNo);
  int partPos = listutils::findAttribute(attrList, pp, attrType);
  if(!partPos){
     return listutils::typeError("attribute " + pp + " not found");
  }
  if(!CcInt::checkType(attrType)){
     return listutils::typeError("attribute " + pp + " not of type int");
  }

  // get the query as well as the default numbers
  string q;
  bool ok;
  if(CcString::checkType(nl->First(nl->Second(args)))){
    ok = getValue<CcString>(nl->Second(nl->Second(args)),q);
  } else {
    ok = getValue<FText>(nl->Second(nl->Second(args)),q);
  }
  if(!ok){
    return listutils::typeError("problem in determining query text");
  }
  int defaultPart;
  if(!getValue(nl->Second(nl->Fifth(args)),defaultPart)){
    return listutils::typeError("cannot determine default part");
  }
  int defaultServer;
  if(!getValue(nl->Second(nl->Sixth(args)),defaultServer)){
    return listutils::typeError("cannot determine default server");
  }

  // manipulate
  q = stringutils::replaceAll(q,"SERVER",stringutils::int2str(defaultServer));
  q = stringutils::replaceAll(q,"PART",stringutils::int2str(defaultPart));

  
  string errMsg;
  ListExpr resType;

  if(!getQueryType(q,defaultServer,resType, errMsg)){
    listutils::typeError(errMsg);
  }
  if(!listutils::isDATA(resType)){
     return listutils::typeError("result type not in kind DATA");
  }
  ListExpr resAttrList = listutils::concat(attrList, 
                              nl->OneElemList( nl->TwoElemList( 
                                        nl->SymbolAtom("Result"),resType))); 
  if(!listutils::isAttrList(resAttrList)){
    return listutils::typeError("Result already part of tuple ");
  }

  return nl->ThreeElemList(
               nl->SymbolAtom(Symbols::APPEND()),
               nl->TwoElemList( nl->IntAtom(serverPos-1), 
                                nl->IntAtom(partPos-1)),
               nl->TwoElemList(
                     listutils::basicSymbol<Stream<Tuple> >(),
                      nl->TwoElemList(
                           listutils::basicSymbol<Tuple>(),
                           resAttrList)));
}

/*
1.9.2 LocalInfo Class

*/
class PQuery2Info : public CommandListener<ListExpr>{
  public:
    PQuery2Info(Word& _stream , const string& _query,  
                int _serverAttr, int _partAttr, ListExpr _tt) : 
      stream(_stream), serverAttr(_serverAttr), 
      partAttr(_partAttr), query(_query) {
    
       stream.open();
       stop = false;
       noInputs=-1;
       noOutputs = 0;
       // run processing thread
       string typeName;

       // determine result type
       ListExpr attrList = nl->Second(nl->Second(_tt));
       while(!nl->HasLength(attrList,1)){
          attrList = nl->Rest(attrList);
       } 
       resType = nl->Second(nl->First(attrList)); 

       tt = new TupleType(
              SecondoSystem::GetCatalog()->NumericType(nl->Second(_tt)));


       string tname;
       int algId, typeId;
       SecondoSystem::GetCatalog()->LookUpTypeExpr(resType, tname, 
                                                   algId, typeId);


       AlgebraManager* am = SecondoSystem::GetAlgebraManager();
       inObject = am->InObj(algId,typeId);
       createObject = am->CreateObj(algId,typeId);
       runner = boost::thread(boost::bind(&PQuery2Info::run,this));
    }

    virtual ~PQuery2Info(){
        stop = true;
        runner.join();

        map<int,ConnectionTask<ListExpr>*>::iterator it;
        for(it = connectors.begin(); it!=connectors.end(); it++){
           it->second->stop();
        }
        for(it = connectors.begin(); it!=connectors.end(); it++){
           delete it->second;
        }
        while(!resultList.empty()){
          Tuple* t = resultList.front();
          resultList.pop_front();
          t->DeleteIfAllowed();
        }
        stream.close();
        tt->DeleteIfAllowed();
        // delete non processed input tuples
        boost::lock_guard<boost::mutex> gurad(mapmtx);
        map<int,Tuple*>::iterator it2;
        for(it2=inTuples.begin();it2!=inTuples.end();it2++){
           it2->second->DeleteIfAllowed();
        }
    }

    Tuple* next(){
       boost::unique_lock<boost::mutex> lock(resmtx);
       while(resultList.empty()){
          cond.wait(lock);
       }
       Tuple* res = resultList.front();
       resultList.pop_front();
       return res;
    }

    virtual void jobDone(int id, string& command, size_t no, int error, 
                       string& errMsg, ListExpr& valueList){

       {


          boost::lock_guard<boost::mutex> guard(resmtx);
          bool correct;    
          Word res;
          Tuple* resTuple = new Tuple(tt);


          mapmtx.lock();
          map<int,Tuple*>::iterator it = inTuples.find(id);
          if(it == inTuples.end()){
              cerr << "Task with id " << id 
                   << "finshed, but no input tuple is found" << endl;
              return;
          }
          Tuple* inTuple = it->second;
          inTuples.erase(it);
          mapmtx.unlock();
          // copy original tuples
          int noAttributes = inTuple->GetNoAttributes();
          for(int i=0;i<noAttributes;i++){
            resTuple->CopyAttribute(i,inTuple,i);
          }
          inTuple->DeleteIfAllowed();

          int errorPos = 0;
          ListExpr errorInfo = listutils::emptyErrorInfo();
          Attribute* resAttr;

          if(nl->HasLength(valueList,2)){
              res = inObject(resType, nl->Second(valueList), 
                         errorPos, errorInfo, correct);
          } else {
             correct = false;
          }
      
          if(correct){ // nested list could not read in correctly
             resAttr = (Attribute*) res.addr; 
          } else {
             Attribute* attr = (Attribute*) createObject(resType).addr;
             attr->SetDefined(false);
             resAttr = attr;; 
          }
          resTuple->PutAttribute(noAttributes,resAttr);
          resultList.push_back(resTuple);
          noOutputs++;
          if(noInputs==noOutputs){
           resultList.push_back(0);
          }
       }
       cond.notify_one();
    }

  private:
    Stream<Tuple> stream;
    int serverAttr;
    int partAttr;
    string query;
    TupleType* tt;
    int noInputs;
    int noOutputs;
    ListExpr resType;
    list<Tuple*> resultList;
    map<int, Tuple*> inTuples;
    boost::mutex resmtx;
    boost::thread runner;
    bool stop;
    map<int,ConnectionTask<ListExpr>*> connectors;
    boost::mutex mapmtx;
    boost::condition_variable cond;
    InObject inObject;
    ObjectCreation createObject; 
    

    
    void run(){
       Tuple* inTuple = stream.request();
       int count =0;
       while(inTuple && !stop){
          CcInt* serverNo = (CcInt*) inTuple->GetAttribute(serverAttr);
          CcInt* partNo = (CcInt*) inTuple->GetAttribute(partAttr);



          if(serverNo->IsDefined() && partNo->IsDefined()){
              int snum = serverNo->GetValue();
              int pnum = partNo->GetValue();
              if(algInstance->serverExists(snum)){
                  startQuery(inTuple, snum,pnum, count);
              } else {
                 inTuple->DeleteIfAllowed();
              } 
          }  
          inTuple = stream.request();
       }
       if(inTuple){
          inTuple->DeleteIfAllowed();
       }
       noInputs = count;
       if(noInputs==noOutputs){
           {  boost::unique_lock<boost::mutex> lock(resmtx);
              resultList.push_back(0);
           }
           cond.notify_one();
       }
    }

    void startQuery(Tuple* inTuple,int s, int p, int &id){
       boost::lock_guard<boost::mutex> guard(mapmtx);
       inTuples[id] = inTuple;
       if(connectors.find(s)==connectors.end()){
          connectors[s] = new ConnectionTask<ListExpr>(s,this);
       }
       string q = stringutils::replaceAll(query,"PART",stringutils::int2str(p));
       q = stringutils::replaceAll(q,"SERVER",stringutils::int2str(s));
       connectors[s]->addCommand(id,q);
       id++;
     }
};


/*
1.9.3 Value Mapping

*/

template<class T>
int pquery2VMT(Word* args, Word& result, int message,
                Word& local, Supplier s ){

   PQuery2Info* li = (PQuery2Info*) local.addr;

   switch(message){
     case OPEN : {
                   if(li) {
                      delete li;
                      local.addr = 0;
                   }
                   T* q = (T*) args[1].addr;
                   if(!q->IsDefined()){
                     return 0;
                   }
                   local.addr = new PQuery2Info(args[0], q->GetValue(),
                                 ((CcInt*) args[6].addr)->GetValue(),
                                 ((CcInt*) args[7].addr)->GetValue(),
                                 qp->GetType(s));
                   return 0;
                 }
      case REQUEST: 
                 result.addr=li?li->next():0;
                 return result.addr?YIELD:CANCEL;
      case CLOSE:
                 if(li){
                    delete li;
                    local.addr =0;
                 }
                 return 0;
   }
   return -1; 
}


/*
1.9.4 Value Mapping and selection

*/

ValueMapping pquery2VM[] = {
  pquery2VMT<CcString>,
  pquery2VMT<FText>
};

int pquery2Select(ListExpr args){
  return CcString::checkType(nl->Second(args))?0:1;
}

/*
1.8.5 Sepcification

*/
OperatorSpec pquery2Spec(
     " stream(tuple(...)) x {text, string} x AttrName x AttrName x "
     "int x int -> stream(tuple)",
     " _ pquery2[_,_,_,_,_]  ",
     " Performs a slightly different  query on a set of remote servers.  "
     " The first argument is a stream of tuples containing the server numbers"
     " as well as the modification number."
     " The second argument is the query to execute. The parts SERVER and PART "
     " are replaced by the corresponding numbers of the stream. "
     " The third argument specifies the Attribute name of the server number,"
     " the fourth argument specifies the attribute name for the part component."
     " The last two arguments are integer numbers used in typemapping as "
     " default values for server and part.",
     " query serverparts feed  pquery2['query ten_SERVER_PART count', Server, "
     "PART,0.5] sum[Result]");

/*
1.9.6

Operator instance

*/
Operator pquery2Op (
    "pquery2",             //name
     pquery2Spec.getStr(),         //specification
     2,
     pquery2VM,        //value mapping
     pquerySelect,   //trivial selection function
     pquery2TM        //type mapping
);




/*
2 Implementation of the DistributedAlgebra Functionality

Here, the functionality of the Distributed algebra is implemented using the
parts from before.

2.1 Type ~darray2~

A ~darray2~ contains the remote type as well as a server configuration 
consiting of the name of the server, the used port, and the used 
configuration file.  When accessing a  ~darray~ element, it is checked 
whether there is already a connection to the remote server. To avoid
 conflicts between user defined connections and connections used by 
darray2 values, a second structure is hold within the algebra
which opens and closes connections automatically.

*/


/*
2.1.2 Class ~DArray2~

This class represents the Secondo type ~darray2~. It just stores the information
about a connection to a remote server. The actual connections are stored within
the algebra instance.

*/

class DArray2{
  public:
     DArray2(size_t _size, const string& _name): 
           worker(),size(_size), name(_name) {

        if(!stringutils::isIdent(name) || size ==0){ // invalid
           name = "";
           defined = false;
           return;
        }
        defined = true;
     }

     

     DArray2(size_t _size, const string& _name, 
               const vector<DArray2Element>& _worker): 
         worker(_worker),size(_size), name(_name) {

        if(!stringutils::isIdent(name) || size ==0){ // invalid
           name = "";
           defined = false;
           return;
        }
        defined = true;
     }

     explicit DArray2(int dummy) {} // only for cast function

 
     DArray2(const DArray2& src): 
             worker(src.worker), size(src.size), name(src.name), 
             defined(src.defined)
       {
       }

     DArray2& operator=(const DArray2& src) {
        this->worker = src.worker;
        if(this->size != src.size){
           this->size = src.size;
        }
        this->name = src.name;
        this->defined = src.defined;
        return *this;
     }     
 
     ~DArray2() {
     }
      

     void set(const size_t size, const string& name, 
              const vector<DArray2Element>& worker){
        if(!stringutils::isIdent(name) || size ==0){ // invalid
           this->name = "";
           this->defined = false;
           return;
        }
        defined = true;
        this->name = name;
        this->size = size;
        this->worker = worker;
     }


     bool IsDefined(){
        return defined;
     }

     static const string BasicType() { 
        return "darray2";
     }

     static const bool checkType(const ListExpr list){

         if(!nl->HasLength(list,2)){
            return false;
         }  
         if(!listutils::isSymbol(nl->First(list), BasicType())){
             return false;
         }
         SecondoCatalog* ctl = SecondoSystem::GetCatalog();
         string name;
         int algid, type;
         if(!ctl->LookUpTypeExpr(nl->Second(list), name, algid, type)){
            return false;
         }
         AlgebraManager* am = SecondoSystem::GetAlgebraManager();
         ListExpr errorInfo = listutils::emptyErrorInfo();
         if(!am->TypeCheck(algid,type,nl->Second(list),errorInfo)){
            return false;
         }
         return true;
     }

     size_t numOfWorkers() const{
       return worker.size();
     }
     size_t getSize() const{
        return size;
     }

     void setSize(size_t newSize){
        size = newSize;
     }
  

     DArray2Element getWorker(int i){
        if(i< 0 || i >= (int) worker.size()){
            assert(false);
           // throw "Invalid worker number";
        }
        return worker[i];
     }

     ListExpr toListExpr(){
       if(!defined){
         return listutils::getUndefined();
       }

       ListExpr wl;

       if(worker.empty()){
         wl =  nl->TheEmptyList();
       } else {
       
           wl = nl->OneElemList(
                        worker[0].toListExpr());
           ListExpr last = wl;
           for(size_t i=1;i<worker.size();i++){
             last = nl->Append(last, worker[i].toListExpr());
           }
       }
       return nl->ThreeElemList(nl->SymbolAtom(name), nl->IntAtom(size), wl); 
     }


     static DArray2* readFrom(ListExpr list){
        if(listutils::isSymbolUndefined(list)){
           return new DArray2(0,"");
        }
        if(!nl->HasLength(list,3)){
           return 0;
        }
        ListExpr Name = nl->First(list);
        ListExpr Size = nl->Second(list);
        ListExpr Workers = nl->Third(list);
        if(   (nl->AtomType(Name) != SymbolType)
            ||(nl->AtomType(Size) != IntType)
            ||(nl->AtomType(Workers)!=NoAtom)){
           return 0;
        }
        string name = nl->SymbolValue(Name);
        if(!stringutils::isIdent(name)){
           return 0;
        }
        int size = nl->IntValue(Size);
        if(size <=0){
           return 0;
        }
        vector<DArray2Element> v;
        int wn = 0;
        while(!nl->IsEmpty(Workers)){
           DArray2Element elem("",0,0,"");
           if(!InDArray2Element(nl->First(Workers), elem)){
              return 0;
           }
           elem.setNum(wn);
           wn++;
           v.push_back(elem);
           Workers = nl->Rest(Workers);
        }
        DArray2* result = new DArray2(size,name);
        swap(result->worker,v);
        result->defined = true;
        return result;
     }



     bool saveTo(SmiRecord& valueRecord, size_t& offset){
        size_t size = worker.size();
        if(!valueRecord.Write(&size,sizeof(size),offset)){
            return false;
        }  
        offset += sizeof(size);
        for(size_t i=0;i<worker.size();i++){
           if(!worker[i].saveTo(valueRecord,offset)){
               return false;
           }
        }
        return true;
     }

     static bool open(SmiRecord& valueRecord, size_t& offset, 
                      const ListExpr typeInfo, Word& result){

        bool defined;
        result.addr = 0;
        if(!readVar<bool>(defined,valueRecord,offset)){
           return false;
        } 
        if(!defined){
          result.addr = new DArray2(0,"");
          return true;
        }
        // array in smirecord is defined, read size
        size_t size;
        if(!readVar<size_t>(size,valueRecord,offset)){
           return false;
        }
        // read name
        string name;
        if(!readVar<string>(name,valueRecord, offset)){
            return false;
        }
        // append workers
        size_t numWorkers;
        if(!readVar<size_t>(numWorkers,valueRecord, offset)){
          return false;
        }
        // create result
        DArray2* res = new DArray2(size,name);
        int wn = 0;
        for(size_t i=0; i< numWorkers; i++){
           DArray2Element elem("",0,0,"");
           if(!elem.readFrom(valueRecord, offset)){
               delete res;
               return false;
           }
           elem.setNum(wn);
           wn++;
           res->worker.push_back(elem);
        }
        result.addr = res;
        return true;
     }

     static bool save(SmiRecord& valueRecord, size_t& offset,
                      const ListExpr typeInfo, Word& value) {

         DArray2* a = (DArray2*) value.addr;
         if(!writeVar(a->defined,valueRecord,offset)){
           return false;
         }
         if(!a->defined){
            return true;
         }
         if(!writeVar(a->size,valueRecord,offset)){
           return false;
         }
         if(!writeVar(a->name, valueRecord, offset)){
           return false;
         }
         if(!writeVar(a->worker.size(), valueRecord, offset)){
           return false;
         }
         for(size_t i=0;i<a->worker.size();i++){
              if(!a->worker[i].saveTo(valueRecord,offset)){
                 return false;
              }
         }
         return true; 
     }

     void print(ostream& out){
       if(!defined){
          out << "undefined";
          return;
       }

       out << "Name : " << name <<", size : " << size 
           << " workers : [" ;
       for(size_t i =0;i<worker.size();i++){
          if(i>0) out << ", ";
          worker[i].print(out);
       }
       out << "]";
     }

     void makeUndefined(){
        worker.clear();
        size = 0;
        name = "";
        defined = false;
     }

     string getName(){
        return name;
     }

      bool setName( const string& n){
        if(!stringutils::isIdent(n)){
           return false;
        }
        name = n;
        return true;
      }



  private:
    vector<DArray2Element> worker; // connection information
    size_t size;  // size of the array itselfs
    string name;  // the basic name used on workers
    bool defined; // defined state of this array
};

/*
2.1.2.1 Property function

*/
ListExpr DArray2Property(){
   return nl->TwoElemList(
            nl->FourElemList(
                 nl->StringAtom("Signature"),
                 nl->StringAtom("Example Type List"), 
                 nl->StringAtom("List Rep"),
                 nl->StringAtom("Example List")),
            nl->FourElemList(
                 nl->StringAtom(" -> SIMPLE"),
                 nl->StringAtom(" (darray2 <basictype>)"),
                 nl->TextAtom("(name size ( s1 s2  ...)) where "
                                "s_i =(server port config)"),
                 nl->TextAtom(" ( mydarray2 42 ('localhost' 1234 'config.ini')"
                              " ('localhost'  1235 'config.ini'))")));
}

/*
2.1.2.2 IN function

*/


Word InDArray2(const ListExpr typeInfo, const ListExpr instance,
               const int errorPos, ListExpr& errorInfo, bool& correct){

   Word res((void*)0);
   res.addr = DArray2::readFrom(instance);
   correct = res.addr!=0;
   return res;
}

/*

2.1.2.3 Out function

*/

ListExpr OutDArray2(ListExpr typeInfo, Word value){
   DArray2* da = (DArray2*) value.addr;
   return da->toListExpr();
}

/*

2.1.2.4 Create function

*/
Word CreateDArray2(const ListExpr typeInfo){

  Word w;
  w.addr = new DArray2(0,"");
  return w;
}

/*

2.1.2.4 Delete function

*/
void DeleteDArray2(const ListExpr typeInfo, Word& w){
  DArray2* a = (DArray2*) w.addr;
  delete a;
  w.addr = 0;
}


/*

2.1.2.4 Open function

*/


void CloseDArray2(const ListExpr typeInfo, Word& w){
  DArray2* a = (DArray2*) w.addr;
  delete a;
  w.addr = 0;
}


Word CloneDArray2(const ListExpr typeInfo, const Word& w){
    DArray2* a = (DArray2*) w.addr;
    Word res;
    res.addr = new DArray2(*a);
    return res;
}

void* CastDArray2(void* addr){
   return (new (addr) DArray2(0));   
}

bool DArray2TypeCheck(ListExpr type, ListExpr& errorInfo){
    return DArray2::checkType(type);
}


int SizeOfDArray2(){
  return 42; // a magic number
}


TypeConstructor DArray2TC(
  DArray2::BasicType(),
  DArray2Property,
  OutDArray2, InDArray2,
  0,0,
  CreateDArray2, DeleteDArray2,
  DArray2::open, DArray2::save,
  CloseDArray2, CloneDArray2,
  CastDArray2,
  SizeOfDArray2,
  DArray2TypeCheck );



/*
3. DArray2 Operators

3.1 Operator ~put~

This operator puts a new value into a DArray. It returns a clone of
the argument array. If there is a problem during storing the value,
the resulting array will be undefined. 

3.1.1 Type Mapping

the signature is darray2(t) x int x t -> darray2(t)

*/
ListExpr putTM(ListExpr args){
   string err = "darray2(t) x int x t expected";

   if(!nl->HasLength(args,3)){
     return listutils::typeError(err + " (invalid number of args)");
   } 
   if( !DArray2::checkType(nl->First(args))){
     return listutils::typeError(err + " ( first arg is not a darray)");
   }
   if( !CcInt::checkType(nl->Second(args))){
     return listutils::typeError(err + " ( second arg is not an int)");
   }


   ListExpr subtype = nl->Second(nl->First(args));
   if(!nl->Equal(subtype, nl->Third(args))){
    return listutils::typeError("type conflic between darray2 "
                                 "and argument type");
   }
   return nl->First(args);
}

/*
3.1.2 ~put~ Value Mapping

Note: the current implementation uses just nested list 
even for relations, here, a special threatment for big objects
should be implemented for transferring big objects to the
remote server;

*/

int putVM(Word* args, Word& result, int message,
                Word& local, Supplier s ){


  DArray2* array = (DArray2*) args[0].addr;
  CcInt* index = (CcInt*) args[1].addr;
  result = qp->ResultStorage(s);
  DArray2* res = (DArray2*) result.addr;
  if(!array->IsDefined() || !index->IsDefined()){
     res->makeUndefined();
     return 0;
  } 

  int i = index->GetValue();

  if(i < 0 || i >= (int) array->getSize()){
     res->makeUndefined();
     return 0;
  }
  if(array->numOfWorkers() < 1){
       res->makeUndefined();
  }

  int workerindex =  i % array->numOfWorkers();

  // retrieve the current database name on the master

  string dbname = SecondoSystem::GetInstance()->GetDatabaseName();

  
  DArray2Element elem = array->getWorker(workerindex);

  ConnectionInfo* ci = algInstance->getWorkerConnection(elem, dbname);

  if(!ci){ // problem with connection the monitor or opening the database
     res->makeUndefined();
     return 0;
  }

  if(! ci->createOrUpdateObject(  array->getName() + "_" 
                                + stringutils::int2str(i),
                                nl->Second(qp->GetType(s)),
                                args[2])){
      res->makeUndefined();
      return 0;
  }
  (*res) = *array;
  return 0;                           
}


/*
3.1.3 Specification

*/

OperatorSpec putSpec(
     " darray2(T) x int x T -> darray2(T) ",
     " put(_,_,_)",
     "Puts an element at a specific position of a darray2. "
     "If there is some problem, the result is undefined; otherwise "
     "the result represents the same array as before with "
     "a changed element",
     "query put([const darray2(int) value"
     "           (da1 4 ((\"host1\" 1234 \"Config1.ini\")"
     "           (\"host2\" 1234 \"Config2.ini\")))] , 1, 27)"
     );


/*
3.1.4 Operator Instance

*/

Operator putOp(
           "put",
           putSpec.getStr(),
           putVM,
           Operator::SimpleSelect,
           putTM);


/*
3.2 Operator get

This operator retrieves an remove object from a server

3.2.1 Type Mapping

Signature is darray2(T) x int -> T

*/
ListExpr getTM(ListExpr args){

  string err = "darray2(T) x int expected";
  if(!nl->HasLength(args,2)){
     return listutils::typeError(err + " (invalid number of args)");
  }
  if(!DArray2::checkType(nl->First(args))){
     return listutils::typeError(err+" ( first is not a darray2)");
  }
  if(!CcInt::checkType(nl->Second(args))){
     return listutils::typeError(err+"(second arg is not of type int)");
  }
  return nl->Second(nl->First(args));
}

/*
3.2.2 Value Mapping

*/
int getVM(Word* args, Word& result, int message,
          Word& local, Supplier s ){

  DArray2* array = (DArray2*) args[0].addr;
  CcInt* index = (CcInt*) args[1].addr;
  result = qp->ResultStorage(s);
  bool isData = listutils::isDATA(qp->GetType(s));
  if(!array->IsDefined() || !index->IsDefined()){
     cerr << " undefined argument" << endl;
     if(isData){
        ((Attribute*) result.addr)->SetDefined(false);
     } else {
        cerr << "Undefined element found" << endl;
     }
     return 0;
  } 

  int i = index->GetValue();
  if(i<0 || i>=(int)array->getSize()){
     if(isData){
        ((Attribute*) result.addr)->SetDefined(false);
     } else {
        cerr << "Undefined element found" << endl;
     }
     return 0;
  }
  if(array->numOfWorkers() <1){
     if(isData){
        ((Attribute*) result.addr)->SetDefined(false);
     } else {
        cerr << "Undefined element found" << endl;
     }
     return 0;
  }


  DArray2Element elem = array->getWorker(i % array->numOfWorkers());
  string dbname = SecondoSystem::GetInstance()->GetDatabaseName();
  ConnectionInfo* ci = algInstance->getWorkerConnection(elem, dbname);
  if(!ci){ // problem with connection the monitor or opening the databaseA
     if(isData){
        ((Attribute*) result.addr)->SetDefined(false);
     } else {
        cerr << "Undefined element found" << endl;
     }
     return 0;
  }

  Word r;
  ListExpr resType = qp->GetType(s);
  bool ok =  ci->retrieve(array->getName() + "_"
                                + stringutils::int2str(i),
                                resType, r, true);

  if(!ok){ // in case of an error, res.addr point to 0A
     if(isData){
        ((Attribute*) result.addr)->SetDefined(false);
     } else {
        cerr << "Undefined element found" << endl;
     }
     return 0;
  }
  if(!nl->Equal(resType, qp->GetType(s))){
     // result type  differs from expected type
     cerr << "Types in darray2 differs from type in remote db" << endl;
     string typeName; 
     int algebraId;
     int typeId;
     SecondoCatalog* ctlg = SecondoSystem::GetCatalog();
     AlgebraManager* am = SecondoSystem::GetAlgebraManager();
     if( ctlg->LookUpTypeExpr( resType, typeName, algebraId, typeId ) ){
         am->DeleteObj(algebraId, typeId)(resType,r);    

     } else {
        cerr << "internal error leads to memory leak" << endl;
     }
     return 0;
  }


  // in case of no error, r.addr points to the result
  qp->DeleteResultStorage(s);
  qp->ChangeResultStorage(s,r);
  result = qp->ResultStorage(s);


  return 0;
}


OperatorSpec getSpec(
     " darray2(T) x int -> T ",
     " get(_,_)",
     " Retrieves an element at a specific position of a darray2.  ",
     " query get([const darray2(int) value"
     "           (da1 4 ((\"host1\" 1234 \"Config1.ini\")"
     "           (\"host2\" 1234 \"Config2.ini\")))] , 1, )"
     );

Operator getOp(
           "get",
           getSpec.getStr(),
           getVM,
           Operator::SimpleSelect,
           getTM);


/*
3.4.4 Operator size

This operator just asks for the size of a darray2 instance.

*/
ListExpr sizeTM(ListExpr args){
  string err  = "darray2 expected";
  if(!nl->HasLength(args,1)){
    return listutils::typeError(err);
  }
  if(!DArray2::checkType(nl->First(args))){
    return listutils::typeError(err);
  }
  return listutils::basicSymbol<CcInt>();
}

int sizeVM(Word* args, Word& result, int message,
          Word& local, Supplier s ){

  result = qp->ResultStorage(s);
  CcInt* res = (CcInt*) result.addr;

  DArray2* arg  = (DArray2*) args[0].addr;
  if(!arg->IsDefined()){
    res->SetDefined(false);
  } else {
    res->Set(true,arg->getSize());
  }
  return 0;
}

OperatorSpec sizeSpec(
     " darray2(T) -> int ",
     " size(_)",
     " Returns the number of slots of a darray2.",
     " query size([const darray2(int) value"
     "           (da1 4 ((\"host1\" 1234 \"Config1.ini\")"
     "           (\"host2\" 1234 \"Config2.ini\")))] )"
     );


Operator sizeOp(
           "size",
           sizeSpec.getStr(),
           sizeVM,
           Operator::SimpleSelect,
           sizeTM);

/*
Operator getWorkers

*/
ListExpr getWorkersTM(ListExpr args){
  string err = "darray2  expected";
  if(!nl->HasLength(args,1)){
    return listutils::typeError(err);
  }
  if(!DArray2::checkType(nl->First(args))){
    return listutils::typeError(err);
  }

  ListExpr attrList = nl->FourElemList(
        nl->TwoElemList(
           nl->SymbolAtom("Host"),
           listutils::basicSymbol<FText>()),
        nl->TwoElemList(
           nl->SymbolAtom("Port"),
           listutils::basicSymbol<CcInt>()),
        nl->TwoElemList(
           nl->SymbolAtom("Config"),
           listutils::basicSymbol<FText>()),
        nl->TwoElemList(
           nl->SymbolAtom("No"),
           listutils::basicSymbol<CcInt>())
      );
  return nl->TwoElemList(
      listutils::basicSymbol<Stream<Tuple> >(),
      nl->TwoElemList(
         listutils::basicSymbol<Tuple>(),
         attrList));
}

class getWorkersInfo{
  public:
     getWorkersInfo(DArray2* _array, ListExpr _tt):
        array(_array), pos(0){
         tt = new TupleType(_tt);
     }

     ~getWorkersInfo(){
         tt->DeleteIfAllowed();
      }

     Tuple* next(){
         if(pos >= array->numOfWorkers()){
            return 0;
         }
         DArray2Element e = array->getWorker(pos);
         pos++;
         Tuple* tuple = new Tuple(tt);
         tuple->PutAttribute(0, new FText(true,e.getHost()));
         tuple->PutAttribute(1, new CcInt(e.getPort()));
         tuple->PutAttribute(2, new FText(true,e.getConfig()));
         tuple->PutAttribute(3, new CcInt(e.getNum()));
         return tuple;
     }

  private:
     DArray2* array;
     size_t pos;
     TupleType* tt;

};


int getWorkersVM(Word* args, Word& result, int message,
          Word& local, Supplier s ){

   getWorkersInfo* li = (getWorkersInfo*) local.addr;
   switch(message){
      case OPEN:{
            if(li){
               delete li;
               local.addr = 0;
            }
            DArray2* arg = (DArray2*) args[0].addr;
            if(arg->IsDefined()){
              local.addr = new getWorkersInfo(arg, 
                              nl->Second(GetTupleResultType(s)));
            }
            return 0;
         }   
       case REQUEST:
              result.addr = li?li->next():0;
              return result.addr?YIELD:CANCEL;
       case CLOSE:
              if(li){
                 delete li;
                 local.addr = 0;
              } 
              return 0;
   }
   return -1;
}


OperatorSpec getWorkersSpec(
     " darray2(T) -> stream(tuple(...)) ",
     " getWorkers(_)",
     " Returns information about workers in a darray2.",
     " query getWorkers([const darray2(int) value"
     "           (da1 4 ((\"host1\" 1234 \"Config1.ini\")"
     "           (\"host2\" 1234 \"Config2.ini\")))] ) count"
     );


Operator getWorkersOp(
           "getWorkers",
           getWorkersSpec.getStr(),
           getWorkersVM,
           Operator::SimpleSelect,
           getWorkersTM);


/*
1.4 Operator ~fconsume5~

This operator stores a tuple stream into a single binary file.
The incoming tuple stream is also the output stream.

1.4.1 Type Mapping

*/
ListExpr fconsume5TM(ListExpr args){
  string err = "stream(TUPLE) x {string.text} expected";

  if(!nl->HasLength(args,2)){
    return listutils::typeError(err);
  }
  if(!Stream<Tuple>::checkType(nl->First(args))){
    return listutils::typeError(err);
  }
  ListExpr fn = nl->Second(args);
  if(!CcString::checkType(fn) && !FText::checkType(fn)){
    return listutils::typeError(err);
  }
  return nl->First(args);
}

/*
1.4.2 Local Info class

*/
class fconsume5Info{
  public:

    fconsume5Info(Word& _stream,
                 const string& filename, 
                 const ListExpr typeList):
       in(_stream){
       out.open(filename.c_str(),ios::out|ios::binary);
       ok = out.good();
       buffer = new char[FILE_BUFFER_SIZE];
       out.rdbuf()->pubsetbuf(buffer, FILE_BUFFER_SIZE);
       if(ok){
         BinRelWriter::writeHeader(out,typeList);
       } 
       ok = out.good();
       in.open();
       firstError = true;
    }

    fconsume5Info(Word& _stream,
                 const ListExpr typeList):
       in(_stream){
       ok = false;
       firstError = true;
    }

    ~fconsume5Info(){
        if(out.is_open()){
           BinRelWriter::finish(out);
           out.close();
        }
        in.close();
        delete[] buffer;
     }

     Tuple* next(){
       Tuple* tuple = in.request();
       if(!tuple){
         return 0;
       }
       if(ok){
         if(!BinRelWriter::writeNextTuple(out,tuple)){
            ok = false;
            if(firstError){
              cerr << "Problem in writing tuple" << endl;
              firstError = false;
            }
         }
       }
       return tuple;
     }
     
  private:
     Stream<Tuple> in;
     ofstream out;
     char* buffer;
     bool ok;
     bool firstError;
};

/*
1.4.3 Value Mapping Template

*/

template<class T>
int fconsume5VMT(Word* args, Word& result, int message,
          Word& local, Supplier s ){

    fconsume5Info* li = (fconsume5Info*) local.addr;

    switch(message){
      case OPEN : {
         ListExpr type = qp->GetType(s);
         T* fn = (T*) args[1].addr;
         if(li){
            delete li;
            local.addr = 0;
         }
         if(!fn->IsDefined()){
             local.addr = new fconsume5Info(args[0],type);
             return 0;
         }
         string fns = fn->GetValue();
         type = nl->TwoElemList( listutils::basicSymbol<Relation>(),
                                 nl->Second(type));
         local.addr = new fconsume5Info(args[0],fns,type);
         return 0;
      }
      case REQUEST:
             result.addr = li?li->next():0;
             return result.addr?YIELD:CANCEL;
      case CLOSE:{
            if(li) {
               delete li;
               local.addr = 0;
            }
            return 0;
      }   
    }
    return -1;

}

/*
1.4.4 Value Mapping Array and Selection

*/
int fconsume5Select(ListExpr args){
  return CcString::checkType(nl->Second(args))?0:1;
}

ValueMapping fconsume5VM[] = {
  fconsume5VMT<CcString>,
  fconsume5VMT<FText>
};


/*
1.4.5 Specification

*/

OperatorSpec fconsume5Spec(
     " stream(TUPLE) x {string, text} -> stream(TUPLE) ",
     " _ fconsume5[_]",
     " Stores a tuple stream into a binary file. ",
     " query ten feed fconsume5['ten.bin'] count"
     );

/*
1.4.6 Instance

*/
Operator fconsume5Op(
   "fconsume5",
   fconsume5Spec.getStr(),
   2,
   fconsume5VM,
   fconsume5Select,
   fconsume5TM
);


/*
1.5 Operator ~ffeed5~

This operator produces a stream of tuples from a file.

*/
ListExpr ffeed5TM(ListExpr args){
  string err = "string or text expected";
  if(!nl->HasLength(args,1)){
    return listutils::typeError(err);
  }
  ListExpr argwt = nl->First(args);
  if(!nl->HasLength(argwt,2)){
     return listutils::typeError("internal error");
  }
  ListExpr arg = nl->First(argwt);
  if(!CcString::checkType(arg) && !FText::checkType(arg)){
    return listutils::typeError(err);
  }

  ListExpr query = nl->Second(argwt);
  Word queryResult;
  string typeString = "";
  string errorString = "";
  bool correct;
  bool evaluable;
  bool defined;
  bool isFunction;
  qp->ExecuteQuery(query, queryResult,
                    typeString, errorString, correct,
                    evaluable, defined, isFunction);
  if(!correct || !evaluable || !defined || isFunction){
     return listutils::typeError("could not extract filename ("+
                                  errorString + ")");
  }
  string filename;
  if(CcString::checkType(arg)){
     CcString* res = (CcString*) queryResult.addr;
     if(!res->IsDefined()){
       res->DeleteIfAllowed();
       return listutils::typeError("undefined filename");
     } else {
        filename = res->GetValue();
        res->DeleteIfAllowed();
     }
  }else {
     FText* res = (FText*) queryResult.addr;
     if(!res->IsDefined()){
       res->DeleteIfAllowed();
       return listutils::typeError("undefined filename");
     } else {
        filename = res->GetValue();
        res->DeleteIfAllowed();
     }
  }
  // access file for extracting the type 
  ifstream in(filename.c_str(), ios::in | ios::binary);
  if(!in.good()){
     return listutils::typeError("could not open file " + filename);
  }
  char marker[4];
  in.read(marker,4);
  if(!in.good()){
     in.close();
     return listutils::typeError("problem in reading from file " 
                                 + filename);
  }
  string m(marker,4);

  if(m!="srel"){
    in.close();
    return listutils::typeError("not a binary relation file");
  }
  uint32_t typeLength;
  in.read((char*) &typeLength, sizeof(uint32_t));
  char* buffer = new char[typeLength];
  in.read(buffer,typeLength);
  if(!in.good()){
    in.close();
    return listutils::typeError("problem in reading from file");
  }  
  string typeS(buffer, typeLength);
  delete [] buffer;
  ListExpr relType;
  nlparsemtx.lock();
  if(!nl->ReadFromString(typeS, relType)){
    nlparsemtx.unlock();
    in.close();
    return listutils::typeError("problem in determining rel type");
  } 
  nlparsemtx.unlock();

  if(!Relation::checkType(relType)){
     in.close();
     return listutils::typeError("not a valid relation type " 
                    + nl->ToString(relType) );
  }


  in.close();
  return nl->TwoElemList( listutils::basicSymbol<Stream<Tuple> >(),
                          nl->Second(relType));
}


/*
1.5.3 LocalInfo

is defined before

*/


template<class T>
int ffeed5VMT(Word* args, Word& result, int message,
          Word& local, Supplier s ){

   ffeed5Info* li = (ffeed5Info*) local.addr;
   switch(message){
      case OPEN: {
          if(li){
             delete li;
             local.addr = 0;
          }
          T* fn = (T*) args[0].addr;
          if(!fn->IsDefined()){
             return 0;
          }
          local.addr = new ffeed5Info( fn->GetValue(),
                                       nl->Second(GetTupleResultType(s)));
          return 0;
      }
      case REQUEST:{
           result.addr = li?li->next():0;
           return result.addr?YIELD:CANCEL;
      }
      case CLOSE: {
           if(li){
              delete li;
              local.addr = 0;
           }
           return 0;
      }
   }
   return -1;
}


/*
1.4.4 Value Mapping Array and Selection

*/
int ffeed5Select(ListExpr args){
  return CcString::checkType(nl->First(args))?0:1;
}

ValueMapping ffeed5VM[] = {
  ffeed5VMT<CcString>,
  ffeed5VMT<FText>
};


/*
1.4.5 Specification

*/

OperatorSpec ffeed5Spec(
     " {string, text} -> stream(TUPLE) ",
     " ffeed5(_)",
     " Restores  a tuple stream from a binary file. ",
     " query ffeed5('ten.bin') count "
     );

/*
1.4.6 Instance

*/
Operator ffeed5Op(
   "ffeed5",
   ffeed5Spec.getStr(),
   2,
   ffeed5VM,
   ffeed5Select,
   ffeed5TM
);

/*
1.5 Operator create darray2

This operator creates a darray2 from a stream specifying the workers. 
As well as a template type.

1.5.1 Type Mapping 

*/ 
ListExpr createDarray2TM(ListExpr args){
   string err = "stream(tuple) x int x string x any "
                               "x Ident x Ident x Ident expected";
   if(!nl->HasLength(args,7)){
      return listutils::typeError(err);
   }
   ListExpr stream = nl->First(args);
   if(!Stream<Tuple>::checkType(stream)){
     return listutils::typeError("First argument must be a tuple stream");
   }

   if(!CcInt::checkType(nl->Second(args))){
     return listutils::typeError("Second argument must be an int");
   }
   if(!CcString::checkType(nl->Third(args))){
     return listutils::typeError("Third argument must be a string");
   }

   ListExpr ha = nl->Fifth(args);
   ListExpr pa = nl->Sixth(args);
   ListExpr ca = nl->Seventh(args);
   if(    (nl->AtomType(ha) != SymbolType)
        ||(nl->AtomType(pa) != SymbolType)
        ||(nl->AtomType(ca) != SymbolType)){
     return listutils::typeError("One of the last three argument"
                                 " is not an Identifier");
   }
   ListExpr ht; // host type
   int hp; // host position
   string hn = nl->SymbolValue(ha);
   ListExpr attrList = nl->Second(nl->Second(stream));
   hp = listutils::findAttribute(attrList, hn, ht);
   if(!hp){
      return listutils::typeError("Attribute " + hn + " not found");
   }

   // fo the same for the port
   ListExpr pt;
   int pp; 
   string pn = nl->SymbolValue(pa);
   pp = listutils::findAttribute(attrList, pn, pt);
   if(!pp){
      return listutils::typeError("Attribute " + pn + " not found");
   }
   // and for the configuration
   ListExpr ct;
   int cp; 
   string cn = nl->SymbolValue(ca);
   cp = listutils::findAttribute(attrList, cn, ct);
   if(!cp){
      return listutils::typeError("Attribute " + cn + " not found");
   }
    
   // check correct types
   if(!CcInt::checkType(pt)){
      return listutils::typeError(  "port attribute " + pn 
                                  + " not of type int");
   }

   if(!CcString::checkType(ht) && !FText::checkType(ht)){
      return listutils::typeError( "host attribute " + hn 
                               + " must be of type text or string");

   }
   if(!CcString::checkType(ct) && !FText::checkType(ct)){
      return listutils::typeError( "Config attribute " + cn 
                               + " must be of type text or string");
   }

   ListExpr resType = nl->TwoElemList(
                           listutils::basicSymbol<DArray2>(),
                           nl->Fourth(args));

   if(!DArray2::checkType(resType)){
      return listutils::typeError("the fourth element does not "
                                  "describe a valid type");
   } 


   return nl->ThreeElemList(
                 nl->SymbolAtom(Symbols::APPEND()),
                 nl->ThreeElemList( nl->IntAtom(hp-1),
                                    nl->IntAtom(pp-1),
                                    nl->IntAtom(cp-1)),
                 resType);
}

/*
1.5.3 Value Mapping Template

*/
template<class H, class C>
int createDarray2VMT(Word* args, Word& result, int message,
          Word& local, Supplier s ){

   int host = ((CcInt*) args[7].addr)->GetValue();
   int port = ((CcInt*) args[8].addr)->GetValue();
   int config = ((CcInt*) args[9].addr)->GetValue();


   result = qp->ResultStorage(s);
   DArray2* res = (DArray2*) result.addr;

   CcInt* size = (CcInt*) args[1].addr;
   CcString* name = (CcString*) args[2].addr;
   if(!size->IsDefined() || !name->IsDefined()){
      res->makeUndefined();
      return 0; 
   }
   int si = size->GetValue();
   string n = name->GetValue();
   if(si<=0 || !stringutils::isIdent(n)){
      res->makeUndefined();
      return 0; 
   }
   vector<DArray2Element> v;
   Stream<Tuple> stream(args[0]);
   stream.open();
   Tuple* tuple;
   int count = 0;
   while((tuple=stream.request())){
     H* h = (H*)  tuple->GetAttribute(host); 
     CcInt* p = (CcInt*) tuple->GetAttribute(port);
     C* c = (C*) tuple->GetAttribute(config);
     if(h->IsDefined() && p->IsDefined() && c->IsDefined()){
        string ho = h->GetValue();
        int po = p->GetValue();
        string co = c->GetValue();
        if(po>0){
           DArray2Element elem(ho,po,count,co);
            count++;
            v.push_back(elem);
        }
     }
     tuple->DeleteIfAllowed();
   }
   stream.close();
   res->set(si,n,v);
   return 0;
}

/*
1.4.5 Value Mapping Array and Selection Function

*/
ValueMapping createDarray2VM[] = {
   createDarray2VMT<CcString,CcString>,
   createDarray2VMT<CcString,FText>,
   createDarray2VMT<FText,CcString>,
   createDarray2VMT<FText,FText>
};

int createDarray2Select(ListExpr args){
  ListExpr H = nl->Fifth(args);
  ListExpr C = nl->Seventh(args);

  ListExpr ht;
  ListExpr ct;
  ListExpr s = nl->Second(nl->Second(nl->First(args)));

  listutils::findAttribute(s,nl->SymbolValue(H),ht);
  listutils::findAttribute(s,nl->SymbolValue(C),ct);

  int n1 = CcString::checkType(ht)?0:1; 
  int n2 = CcString::checkType(ct)?0:1; 
  return n2 + n1*2;
}

/*
1.4.6 Specification

*/
OperatorSpec createDarray2Spec(
     " stream<TUPLE> x int x string x ANY x attrName x "
     "attrName x attrName  -> darray2",
     " _ createDarray2[size, name, type template , HostAttr, "
                     "PortAttr, ConfigAttr]",
     " Creates a darray 2. The workers are given by the input stream. ",
     " query workers feed createDarray2[6,\"obj\",streets, Host, Port, Config] "
     );

/*
1.4.7 Operator instance

*/
Operator createDarray2Op(
  "createDarray2",
  createDarray2Spec.getStr(),
  4,
  createDarray2VM,
  createDarray2Select,
  createDarray2TM
);


/*
1.5 Operator pput

This operator sets some values of the dbarray in parallel.



*/
ListExpr pputTM(ListExpr args){
  string err = "darray2(T) x (int x T)+ expected";
  if(nl->ListLength(args) <2){
    return listutils::typeError(err);
  }
  ListExpr darray = nl->First(args);
  ListExpr pairs = nl->Rest(args);
  if(!DArray2::checkType(darray)){
    return listutils::typeError(err);
  }
  ListExpr subType = nl->Second(darray);
  bool index = true;
  while(nl->IsEmpty(pairs)){
    if(index){
       if(!CcInt::checkType(nl->First(pairs))){
          return listutils::typeError("array index not an int");
       }
    } else {
       if(!nl->Equal(subType,nl->First(pairs))){
         return listutils::typeError("expression does not fit darray subtype");
       }
    }
    pairs = nl->Rest(pairs);
    index = !index; 
  }
  if(!index){
    return listutils::typeError("Missing expression for index");
  }
  return darray;
}


class SinglePutter{

  public:
    SinglePutter(ListExpr _type, DArray2* _array, 
                 int _arrayIndex, Word& _value):
     type(_type), array(_array), arrayIndex(_arrayIndex), value(_value){
       runner = boost::thread(&SinglePutter::run,this);
    }

    ~SinglePutter(){
        runner.join();
    }
 

  private:
     ListExpr type;
     DArray2* array;
     int arrayIndex;
     Word value;
     boost::thread runner;


    void run(){
      string dbname = SecondoSystem::GetInstance()->GetDatabaseName();
      DArray2Element elem = 
                    array->getWorker(arrayIndex % array->numOfWorkers());
      ConnectionInfo* ci = algInstance->getWorkerConnection(elem, dbname);
      if(!ci){
           cerr << "could not connect to server " << elem << endl;
           return;
      } 
      string objname = array->getName() + "_"+stringutils::int2str(arrayIndex); 
      ci->createOrUpdateObject(objname, type, value);
    }


};

class PPutter{
  public:

    PPutter(ListExpr _type,
            DArray2* _arg, vector<pair<int,Word> >& _values){
      this->type = _type;
      this->source = _arg;
      set<int> used;
      typename vector<pair<int,Word> >::iterator it;
      for(it = _values.begin(); it!=_values.end(); it++){
            pair<int,Word> p = *it;
            int index = p.first;
            if( (index >= 0) && ( (size_t)index < source->getSize())
                 && (used.find(index)==used.end())){
              values.push_back(p);
            }
      }
      started = false;
    }

    void start(){
       boost::lock_guard<boost::mutex> guard(mtx);
       if(!started){
         started=true;
         for(size_t i=0;i<values.size();i++){
            SinglePutter* sp = new SinglePutter(type,source,values[i].first, 
                                                values[i].second);
            putters.push_back(sp);
         }
       }
    }  

    ~PPutter(){
         for(size_t i=0;i<putters.size();i++){
            delete putters[i];
         }
      
    } 

  private:
    ListExpr type;
    DArray2* source;
    vector<pair<int,Word> > values;
    vector<SinglePutter*> putters;
    bool started;
    boost::mutex mtx;

};

/*
1.4.2 Value Mapping

*/

int pputVM(Word* args, Word& result, int message,
           Word& local, Supplier s ){

   result = qp->ResultStorage(s);
   DArray2* res = (DArray2*) result.addr;

   DArray2* arg = (DArray2*) args[0].addr;

   // collect the value to set into an vector
   // ignoring undefined indexes or indexes 
   // out of range, if an index is specified
   // more than once, the first one wins
   int sons = qp->GetNoSons(s);
   int i=1;
   set<int> used;

   vector<pair<int, Word> > pairs;

   while(i<sons){
     CcInt* Index = (CcInt*) args[i].addr;
     i++;
     Word w = args[i];
     i++;
     if(Index->IsDefined()){
       int index = Index->GetValue();
       if( (index>=0 )&& 
           ((size_t) index < arg->getSize()) && 
           (used.find(index)==used.end())){
           pair<int,Word> p(index,w);
           pairs.push_back(p);
           used.insert(index);
       }
     }
   }

   
   PPutter* pput = new PPutter(nl->Second(qp->GetType(s)),arg, pairs);
   pput->start(); 
   delete pput;
   *res = *arg;
   return 0;
}

/*
1.4.3 Specification

*/


OperatorSpec pputSpec(
     " darray2(T) x (int x T)+ -> darray2 ",
     " _ pput[ index , value , index, value ,...]",
     " Puts elements into a darray2 in parallel. ",
     " query da pput[0, streets1, 1, streets2]  "
     );


/*
1.4.4 Operator instance

*/

Operator pputOp(
  "pput",
  pputSpec.getStr(),
  pputVM,
  Operator::SimpleSelect,
  pputTM
);


/*
1.6 Operator ddistribute2

1.6.1 Type Mapping

*/

ListExpr ddistribute2TM(ListExpr args){
  string err = "stream(tuple(X)) x ident x darray2(Y) [ x string] expected";
  if(!nl->HasLength(args,3) && !nl->HasLength(args,4)){
     return listutils::typeError(err);
  }
  if(nl->HasLength(args,4)){
    if(!CcString::checkType(nl->Fourth(args))){
        return listutils::typeError(err);
    }
  }
  if(!Stream<Tuple>::checkType(nl->First(args))){
     return listutils::typeError(err);
  }
  if(nl->AtomType(nl->Second(args))!=SymbolType){
     return listutils::typeError(err);
  }
  if(!DArray2::checkType(nl->Third(args))){
     return listutils::typeError(err);
  }

  ListExpr attrList = nl->Second(nl->Second(nl->First(args)));
  string ident = nl->SymbolValue(nl->Second(args));
  ListExpr dType;
  int pos = listutils::findAttribute(attrList, ident ,dType);
  if(!pos){
     return listutils::typeError("Attribute " + ident + " unknown");
  }
  if(!CcInt::checkType(dType)){
     return listutils::typeError("Attribute " + ident + " not of type int");
  }
  ListExpr res = nl->TwoElemList(
                   listutils::basicSymbol<DArray2>(),
                   nl->TwoElemList(
                      listutils::basicSymbol<Relation>(),
                      nl->Second(nl->First(args))));
  return nl->ThreeElemList(
                  nl->SymbolAtom(Symbols::APPEND()),
                  nl->OneElemList( nl->IntAtom(pos-1)),
                  res);
}

/*
1.6.2 Auxiliary class ~RelFileRestore~

This class restores a relation stored within a binary file
on a remote server within an own thread.

*/

class RelFileRestorer{

 public:
   RelFileRestorer(ListExpr _relType, const string& _objName,
                   DArray2* _array, int _arrayIndex, 
                   const string& _filename):
    relType(_relType), objName(_objName), array(_array), 
    arrayIndex(_arrayIndex), filename(_filename),
    started(false){
    int workerIndex = arrayIndex % array->numOfWorkers();
    string dbname = SecondoSystem::GetInstance()->GetDatabaseName();
    ci = algInstance->getWorkerConnection(
                         array->getWorker(workerIndex),dbname);
   }

   ~RelFileRestorer(){
     boost::lock_guard<boost::mutex> guard(mtx);
     runner.join();
    }

   void start(){
     boost::lock_guard<boost::mutex> guard(mtx);
     if(!started && ci ){
        started = true;
        res = true;
        checkServerFree();
        runner = boost::thread(&RelFileRestorer::run, this);
     }
   }

 private:
    ListExpr relType;
    string objName;
    DArray2* array;
    int arrayIndex;
    string filename;
    bool started;
    boost::mutex mtx;
    boost::thread runner;
    bool res;
    ConnectionInfo* ci;

    static map<pair<string,int>,boost::mutex*> serializer;
    static boost::mutex sermtx;


    void run(){
      if(ci){
         res = ci->createOrUpdateRelationFromBinFile(objName,filename);
         if(!res){
           cerr << "createorUpdateObject failed" << endl;
         }
         sermtx.lock();
         pair<string,int> p(ci->getHost(), ci->getPort());
         serializer[p]->unlock();
         sermtx.unlock();
      } else {
        cerr << "connection failed" << endl;
      }
    }

    void checkServerFree(){
       sermtx.lock();
       map<pair<string,int>,boost::mutex*>::iterator it;
       pair<string,int> p(ci->getHost(), ci->getPort());
       it = serializer.find(p);
       if(it==serializer.end()){
          boost::mutex* mtx = new boost::mutex();
          serializer[p] = mtx;
       }        
       boost::mutex* m = serializer[p];
       sermtx.unlock();
       m->lock();
    }



};

map<pair<string,int>,boost::mutex*> RelFileRestorer::serializer;
boost::mutex RelFileRestorer::sermtx;

/*
1.6.3 Value Mapping

*/

int ddistribute2VM(Word* args, Word& result, int message,
           Word& local, Supplier s ){

   result = qp->ResultStorage(s);
   DArray2* res = (DArray2*) result.addr;
   DArray2* temp = (DArray2*) args[2].addr;


   int noSons = qp->GetNoSons(s);
   int pos = ((CcInt*) args[noSons-1].addr)->GetValue();
   *res = *temp;

   if(noSons==5){
     CcString* n =  (CcString*) args[3].addr;
     if(!n->IsDefined()){
        res->makeUndefined();
        return 0;
     }
     string name = n->GetValue();
     if(!stringutils::isIdent(name)){
        res->makeUndefined();
        return 0;
     }
     res->setName(name);
   }

   if(!res->IsDefined() || (res->numOfWorkers()<1) || (res->getSize() < 1)){
      res->makeUndefined();
      return 0;
   }

   // distribute the incoming tuple stream to a set of files
   // if the distribution number is not defined, the tuple is
   // treated as for number 0
   map<int , pair<string, pair<ofstream*, char*> > > files;
   Stream<Tuple> stream(args[0]);
   stream.open();
   Tuple* tuple;
   string name = res->getName();

   ListExpr relType = nl->Second(qp->GetType(s));

   size_t size = temp->getSize();
   size_t bufsize = max((size_t)4096u, FILE_BUFFER_SIZE*10/size);

   while((tuple=stream.request())){
      CcInt* D = (CcInt*) tuple->GetAttribute(pos);
      int index = D->IsDefined()?D->GetValue():0;
      index = index % temp->getSize();
      string fn = name + "_" + stringutils::int2str(index)+".bin";
      if(files.find(index)==files.end()){
          ofstream* out = new ofstream(fn.c_str(), ios::out | ios::binary);
          char* buffer = new char[bufsize];
          out->rdbuf()->pubsetbuf(buffer, bufsize); 
          pair<ofstream*, char*> p1(out,buffer);
          pair<string, pair<ofstream*, char* > > p(fn,p1);
          files[index] = p;
          BinRelWriter::writeHeader(*out,relType); 
      }
      BinRelWriter::writeNextTuple(*(files[index].second.first),tuple);
      tuple->DeleteIfAllowed();
   }
   stream.close();
   // finalize files


   vector<RelFileRestorer*> restorers;

   
   typename map<int, pair<string,pair<ofstream*, char*> > >::iterator it;
   for(it = files.begin(); it!=files.end();it++){
      BinRelWriter::finish(*(it->second.second.first));
      // close and delete ofstream
      it->second.second.first->close();
      delete it->second.second.first;
      delete[] it->second.second.second;
      string objName = res->getName()+"_"+stringutils::int2str(it->first);
      restorers.push_back(new RelFileRestorer(relType, objName,res, 
                                              it->first, it->second.first));
   }


   // distribute the files to the workers and restore relations
   for(size_t i=0;i<restorers.size();i++){
     restorers[i]->start();
   } 
   // wait for finishing restore

   
   for(size_t i=0;i<restorers.size();i++){
     delete restorers[i];
   } 
   for(it = files.begin(); it!=files.end();it++){
      FileSystem::DeleteFileOrFolder(it->second.first);
   }

   return 0;   
}

/*
1.6.4 Specification

*/
OperatorSpec ddistribute2Spec(
     " stream(tuple(X)) x ident x darray2(Y) -> darray2(X) ",
     " _ ddistribute2[ _, _]",
     " Distributes a locally stored relation into a darray2.",
     " query strassen feed addcounter[No,1] ddistribute[No, da2]  "
     );


/*
1.6.5 Operator instance

*/
Operator ddistribute2Op(
  "ddistribute2",
  ddistribute2Spec.getStr(),
  ddistribute2VM,
  Operator::SimpleSelect,
  ddistribute2TM
);



/*
1.7 Operator ~ddistribute3~

Similar to the ddistribute2 operator, this operator distributes a tuple strean
into a darray2 object. The difference is how the tuples of the incoming stream
are distributed.  While the ddistribute2 operator requires an integer attribute
signaling to which slow of the array the tuple should be stored, this operator
get a partion size of the size of the array. The variant (meaning of the int
argument) is chosen by a boolean argument.

1.7.1 Type Mapping

*/

ListExpr ddistribute3TM(ListExpr args){


  string err = "stream(tuple) x int x bool x darray2 x string expected";
  if(!nl->HasLength(args,5)){
    return listutils::typeError(err+": wrong number of args");
  }
  if(   !Stream<Tuple>::checkType(nl->First(args))
     || !CcInt::checkType(nl->Second(args))
     || !CcBool::checkType(nl->Third(args))   
     || !DArray2::checkType(nl->Fourth(args))
     || !CcString::checkType(nl->Fifth(args))){
    return listutils::typeError(err);
  }
  ListExpr relType = nl->TwoElemList(
                        listutils::basicSymbol<Relation>(),
                        nl->Second(nl->First(args)));

  return nl->TwoElemList(
                 listutils::basicSymbol<DArray2>(),
                  relType); 
}



/*
1.7.3 Value Mapping

*/

int ddistribute3VM(Word* args, Word& result, int message,
           Word& local, Supplier s ){

   result = qp->ResultStorage(s);
   DArray2* res = (DArray2*) result.addr;

   CcInt* size = (CcInt*) args[1].addr;
   CcBool* method = (CcBool*) args[2].addr;
   DArray2* array = (DArray2*) args[3].addr;
   CcString* n = (CcString*) args[4].addr;

   if(!size->IsDefined() || !method->IsDefined() 
      || !array->IsDefined() || !n->IsDefined()){
     res->makeUndefined();
     return 0;
   }

   int sizei = size->GetValue();
   if(sizei<=0){
     res->makeUndefined();
     return 0;
   }
   string name = n->GetValue();
   if(!stringutils::isIdent(name)){
     res->makeUndefined();
     return 0;
   }
   if(array->numOfWorkers() < 1 ){
     res->makeUndefined();
     return 0;
   }


   // distribute incoming tuple stream 
   // to files

   bool methodb = method->GetValue();
   *res = *array;

   res->setName(name);
    


   vector< pair<ofstream*, char*> > files;
   Stream<Tuple> stream(args[0]);
   stream.open();
   Tuple* tuple;
   ListExpr relType = nl->Second(qp->GetType(s));
   if(methodb){ // size is the size of the darray
      // circular distribution of 
      res->setSize(sizei);
      int index = 0;
      ofstream* current=0;
      size_t bufsize = max(4096, (FILE_BUFFER_SIZE*16) / sizei);

      while((tuple=stream.request())){
         size_t index1 = index % sizei;
         index++;
         if(index1 >= files.size()){
             string fn = name + "_" + stringutils::int2str(index1)+".bin";
             current = new ofstream(fn.c_str(), ios::out | ios::binary);
             char* buf = new char[bufsize];
             current->rdbuf()->pubsetbuf(buf,bufsize);
             pair<ofstream*, char*> p(current,buf);
             files.push_back(p);
             BinRelWriter::writeHeader(*current,relType); 
          } else {
              current = files[index1].first;
          }
          BinRelWriter::writeNextTuple(*current,tuple);
          tuple->DeleteIfAllowed();
      }
      for(size_t i=0;i<files.size();i++){
         BinRelWriter::finish(*(files[i].first));
         files[i].first->close();
         delete files[i].first;
         delete[] files[i].second;
      }
   } else {
      int written = 0;
      ofstream* current=0;
      char* buf = 0;
      while((tuple=stream.request())){
         if(written==0){
           string fn = name + "_" + stringutils::int2str(files.size())+".bin";
           current = new ofstream(fn.c_str(), ios::out | ios::binary);
           buf = new char[FILE_BUFFER_SIZE];
           current->rdbuf()->pubsetbuf(buf,FILE_BUFFER_SIZE);
           pair<ofstream*, char*> p(current, buf);
           files.push_back(p);
           BinRelWriter::writeHeader(*current,relType); 
         }
         BinRelWriter::writeNextTuple(*current,tuple);
         written++;
         if(written==sizei){
           BinRelWriter::finish(*current);
           current->close();
           delete current;
           delete[] buf;
           current=0;
           written = 0;
         }
      }     
      if(current){
        BinRelWriter::finish(*current);
        current->close();
        delete current;
        delete[] buf;
        current=0;
      } 
      res->setSize(files.size());
   }
   stream.close();

    // now, all tuples are distributed to files.
    // we have to put the relations stored in these
    // files to the workers

   vector<RelFileRestorer*> restorers;

   
   for(size_t i = 0; i<files.size(); i++) {
      string objName = res->getName()+"_"+stringutils::int2str(i);     
      string fn = objName + ".bin";
      restorers.push_back(new RelFileRestorer(relType, objName,res, 
                                              i, fn));
   }


   // distribute the files to the workers and restore relations
   for(size_t i=0;i<restorers.size();i++){
     restorers[i]->start();
   } 
   // wait for finishing restore

   
   for(size_t i=0;i<restorers.size();i++){
     delete restorers[i];
   }
   // delete local files
   for(size_t i=0;i<files.size();i++){
      string fn = res->getName()+"_"+stringutils::int2str(i)+".bin";     
      FileSystem::DeleteFileOrFolder(fn); 
   }
 

   return 0;   
}


/*
1.7.4 Specification

*/
OperatorSpec ddistribute3Spec(
     " stream(tuple(X)) x int x bool x darray2(Y) x string-> darray2(X) ",
     " _ ddistribute3[ _, _,_,_]",
     " Distributes a ituple streanm into a darray. The boolean "
     "flag controls the method of distribution. If the flag is set to "
     " true, the integer argument specifies the target size of the "
     " resulting darray and the tuples are distributed in a circular way."
     "In the other case, this number represents the size of a single "
     "array slot. A slot is filled until the size is reached. After that "
     " a new slot is opened. The string attribute gives the name of the "
     "result",
     " query strassen feed ddistribute3[10, TRUE, da8, \"da28\" ]  "
     );


/*
1.7.5 Operator instance

*/
Operator ddistribute3Op(
  "ddistribute3",
  ddistribute3Spec.getStr(),
  ddistribute3VM,
  Operator::SimpleSelect,
  ddistribute3TM
);


/*
1.8 Operator ~ddistribute4~

This Operator uses a function: tuple -> int for distributing
a tuple stream to an darray.

1.8.1 Type Mapping

*/
ListExpr ddistribute4TM(ListExpr args){
  string err ="stream(tuple) x (tuple->int) x darray2 x string   expected";
  if(!nl->HasLength(args,4)){
    return listutils::typeError(err);
  }
  if(    !Stream<Tuple>::checkType(nl->First(args))
      || !listutils::isMap<1>(nl->Second(args))
      || !DArray2::checkType(nl->Third(args))
      || !CcString::checkType(nl->Fourth(args))){
    return listutils::typeError(err);
  }
  ListExpr funargType = nl->Second(nl->Second(args));
  if(!nl->Equal(funargType, nl->Second(nl->First(args)))){
    return listutils::typeError("tuple type and function arg type differ");
  }
  ListExpr funResType = nl->Third(nl->Second(args));
  if(!CcInt::checkType(funResType)){
    return listutils::typeError("result of function not an int");
  }

  return nl->TwoElemList(
              listutils::basicSymbol<DArray2>(),
              nl->TwoElemList(
                  listutils::basicSymbol<Relation>(),
                  nl->Second(nl->First(args))));
}


/*
1.8.2 Value Mapping

*/
int ddistribute4VM(Word* args, Word& result, int message,
           Word& local, Supplier s ){

   result = qp->ResultStorage(s);
   DArray2* res   = (DArray2*) result.addr;
   DArray2* array = (DArray2*) args[2].addr;
   CcString* n = (CcString*) args[3].addr;
   if(!array->IsDefined() || !n->IsDefined()){
     res->makeUndefined();
     return 0;
   }
   string name = n->GetValue();
   if(!stringutils::isIdent(name)){
      res->makeUndefined();
      return 0;
   }

   *res = *array;
    res->setName(name);

   if(!res->IsDefined() || (res->numOfWorkers()<1) || (res->getSize() < 1)){
      res->makeUndefined();
      return 0;
   }

   // distribute the incoming tuple stream to a set of files
   // if the distribution number is not defined, the tuple is
   // treated as for number 0
   map<int , pair<string,pair<ofstream*, char*> > > files;
   Stream<Tuple> stream(args[0]);
   stream.open();
   Tuple* tuple;

   ListExpr relType = nl->Second(qp->GetType(s));

   ArgVectorPointer funargs = qp->Argument(args[1].addr);
   Word funres;

   size_t bufsize = max((size_t)4096, (FILE_BUFFER_SIZE*16) / array->getSize());

   while((tuple=stream.request())){
      (* funargs[0]) = tuple;
      qp->Request(args[1].addr, funres);

      CcInt* fr = (CcInt*) funres.addr;
      int index = fr->IsDefined()?fr->GetValue():0;
      index = index % array->getSize();
      

      string fn = name + "_" + stringutils::int2str(index)+".bin";
      if(files.find(index)==files.end()){
          ofstream* out = new ofstream(fn.c_str(), ios::out | ios::binary);
          char* buf = new char[bufsize];
          out->rdbuf()->pubsetbuf(buf,bufsize);
          pair<string, pair<ofstream*, char*> >
                        p(fn,pair<ofstream*,char*>(out,buf));
          files[index] = p;
          BinRelWriter::writeHeader(*out,relType); 
      }
      BinRelWriter::writeNextTuple(*(files[index].second.first),tuple);
      tuple->DeleteIfAllowed();
   }
   stream.close();
   // finalize files

   vector<RelFileRestorer*> restorers;

   
   typename map<int, pair<string, pair<ofstream*, char*> > >::iterator it;
   for(it = files.begin(); it!=files.end();it++){
      BinRelWriter::finish(*(it->second.second.first));
      // close and delete ofstream
      it->second.second.first->close();
      delete it->second.second.first;
      delete[] it->second.second.second;
      string objName = res->getName()+"_"+stringutils::int2str(it->first);
      restorers.push_back(new RelFileRestorer(relType, objName,res, 
                                              it->first, it->second.first));
   }


   // distribute the files to the workers and restore relations
   for(size_t i=0;i<restorers.size();i++){
     restorers[i]->start();
   } 
   // wait for finishing restore

   
   for(size_t i=0;i<restorers.size();i++){
     delete restorers[i];
   } 
   for(it = files.begin(); it!=files.end();it++){
      FileSystem::DeleteFileOrFolder(it->second.first);
   }

   return 0;   
}

OperatorSpec ddistribute4Spec(
     " stream(tuple(X)) x (tuple->int) x darray2(Y) x string-> darray2(X) ",
     " _ ddistribute4[ _, _,_]",
     " Distributes a locally stored relation into a darray2 ",
     " query strassen feed  ddistribute4[ hashvalue(.Name,2000),"
     " da2, \"da8\"]  "
     );

/*
1.7.5 Operator instance

*/
Operator ddistribute4Op(
  "ddistribute4",
  ddistribute4Spec.getStr(),
  ddistribute4VM,
  Operator::SimpleSelect,
  ddistribute4TM
);




/*
1.7 fdistribute

*/
ListExpr fdistribute5TM(ListExpr args){
  string err = "stream(tuple) x string x int x attrName expected";
  // stream <base file name> <number of files> <attrName for distribute>
  if(!nl->HasLength(args,4)){
    return listutils::typeError(err);
  }
  if( !Stream<Tuple>::checkType(nl->First(args))
     || (   !CcString::checkType(nl->Second(args)) 
         && !FText::checkType(nl->Second(args)))
     || !CcInt::checkType(nl->Third(args))
     || (nl->AtomType(nl->Fourth(args))!=SymbolType)){
    return listutils::typeError(err);
  }
  string attrName = nl->SymbolValue(nl->Fourth(args));
  ListExpr attrList = nl->Second(nl->Second(nl->First(args)));
  ListExpr type;
  int pos = listutils::findAttribute(attrList,attrName,type);
  if(!pos){
     return listutils::typeError("AttrName " + attrName + " not found");
  }
  if(!CcInt::checkType(type)){
     return listutils::typeError("AttrName " + attrName + " not of type int");
  }
  return nl->ThreeElemList(
                nl->SymbolAtom(Symbols::APPEND()),
                nl->OneElemList(nl->IntAtom(pos-1)),
                nl->First(args));

}

template<class T>
class fdistribute5Info{
  public:
     fdistribute5Info(Word& _stream, CcInt* _size, T* _name,int _pos, 
                      ListExpr relType)
       : stream(_stream){
        write = true;
        if(!_size->IsDefined() || (_size->GetValue() <=0)){
          write = false;
        } else {
          this->size = _size->GetValue();
          bufsize = max(( FILE_BUFFER_SIZE * 10 ) / this->size, 4096);
        }
        if(!_name->IsDefined()){
           write = false;
        } else {
           basename = _name->GetValue();
        }
        this->pos = _pos;
        this->relType = relType;
        stream.open();
     }

     Tuple* next(){
        Tuple* tuple = stream.request();
        if(write && tuple){
            writeNextTuple(tuple);
        }
        return tuple;
     }

     ~fdistribute5Info(){
        map<int, pair<ofstream*,char*> >::iterator it;
        for(it = writers.begin();it!=writers.end() ; it++){
           BinRelWriter::finish(*(it->second.first));
           delete it->second.first;
           delete [] it->second.second;
        }
        stream.close();
     }


  private:
     Stream<Tuple> stream;
     bool write;
     string basename;
     int pos;
     int size;
     ListExpr relType;
     map<int, pair< ofstream*, char*> > writers;
     size_t bufsize;

    
     void writeNextTuple(Tuple* tuple){
        CcInt* num = (CcInt*) tuple->GetAttribute(pos);
        int f =0;
        if(num->IsDefined()){
            f = num->GetValue() % size;
        }
        map<int, pair<ofstream*, char*> >::iterator it = writers.find(f);
        ofstream* out;
        if(it==writers.end()){
           out = new ofstream((basename +"_"+stringutils::int2str(f)).c_str(),
                              ios::out | ios::binary);
           char* buf = new char[bufsize];
           out->rdbuf()->pubsetbuf(buf, bufsize);
           pair<ofstream*, char*> p(out,buf);
           writers[f] = p;
           BinRelWriter::writeHeader(*out,relType);
        } else {
           out = it->second.first;
        }
        BinRelWriter::writeNextTuple(*out, tuple);
     }

};


template<class T>
int fdistribute5VMT(Word* args, Word& result, int message,
           Word& local, Supplier s ){
 
  fdistribute5Info<T>* li = (fdistribute5Info<T>*) local.addr;
  switch(message){
    case OPEN: { 
      T* fname = (T*) args[1].addr;
      CcInt* size = (CcInt*) args[2].addr;
      int attrPos = ((CcInt*)args[4].addr)->GetValue();
      if(li){
         delete li;
      }
      ListExpr relType = nl->TwoElemList(
                            listutils::basicSymbol<Relation>(),
                            nl->Second(qp->GetType(s)));
      local.addr = new fdistribute5Info<T>(args[0],size, fname, attrPos,
                                           relType);
      return 0;
    }
   case REQUEST:
           result.addr = li?li->next():0;
           return result.addr?YIELD:CANCEL;
    case CLOSE:
          if(li){
             delete li;
             local.addr = 0;
          }
          return 0;
   }

    
  return -1;

}

/*
1.7.3 Selction and value mapping

*/

int fdistribute5Select(ListExpr args){
 return CcString::checkType(nl->Second(args))?0:1;
}

ValueMapping fdistribute5VM[] = {
   fdistribute5VMT<CcString>,
   fdistribute5VMT<FText>
};

/*
1.7.4 Specification

*/
OperatorSpec fdistribute5Spec(
     " stream(tuple(X)) x string,text} x int x attrName ",
     " _ ddistribute2[ _, _,_]",
     " Distributes a locally stored relation into a set of files."
     "The file names are given by the second attribute extended by"
     " the array index.",
     " query strassen feed addcounter[No,1] ifdistribute5['strassen',10,No]"
     "  count"
     );

/*
1.7.5 Operator Instance

*/
Operator fdistribute5Op(
   "fdistribute5",
   fdistribute5Spec.getStr(),
   2,
   fdistribute5VM,
   fdistribute5Select,
   fdistribute5TM
);




/*
1.7 Operator closeWorkers

This operator closes the worker connections either dor a spoecified
darray2 instance or all existing connections.

*/
ListExpr closeWorkersTM(ListExpr args){
  string err = " no argument or darray2 expected";
  if(!nl->IsEmpty(args) && !nl->HasLength(args,1)){
    return listutils::typeError(err);
  }
  if(nl->HasLength(args,1)){
    if(!DArray2::checkType(nl->First(args))){
       return listutils::typeError(err);
    }
  }
  return listutils::basicSymbol<CcInt>();
}


int closeWorkersVM(Word* args, Word& result, int message,
           Word& local, Supplier s ){

    result = qp->ResultStorage(s);
    CcInt* res = (CcInt*) result.addr;
    if(qp->GetNoSons(s)==0){
       res->Set(true, algInstance->closeAllWorkers());  
    } else {
       DArray2* arg = (DArray2*) args[0].addr;
       if(!arg->IsDefined()){
         res->SetDefined(false);
         return 0;
       }
       int count = 0;
       for(size_t i=0;i<arg->numOfWorkers();i++){
          bool closed = algInstance->closeWorker(arg->getWorker(i));
          if(closed){
             count++;
          }
       }
       res->Set(true,count);
    }
    return 0;
}


OperatorSpec closeWorkersSpec(
     " -> int, darray2 -> int ",
     " closeWorkers(_)",
     " Closes either all connections to workers (no argument)"
     ", or connections of a specified darray2 instance.",
     " query closeWorkerConnections()  "
     );

Operator closeWorkersOp(
  "closeWorkers",
  closeWorkersSpec.getStr(),
  closeWorkersVM,
  Operator::SimpleSelect,
  closeWorkersTM
);


/*
1.8 Operator ~showWorkers~

This operator shows the information about existing worker connections.
If the optional argument is given, only open conections of this array
are shown, otherwise infos about all existing workers.

*/

ListExpr showWorkersTM(ListExpr args){
  string err = "nothing or darray2 expected" ;
  if(!nl->IsEmpty(args) && !nl->HasLength(args,1)){
     return listutils::typeError(err);
  }
  if(nl->HasLength(args,1) && !DArray2::checkType(nl->First(args))){
     return listutils::typeError(err);
  }
  ListExpr attrList = nl->SixElemList(
    nl->TwoElemList( nl->SymbolAtom("Host"), 
                     listutils::basicSymbol<FText>()),
    nl->TwoElemList( nl->SymbolAtom("Port"), 
                     listutils::basicSymbol<CcInt>()),
    nl->TwoElemList( nl->SymbolAtom("ConfigFile"), 
                     listutils::basicSymbol<FText>()),
    nl->TwoElemList( nl->SymbolAtom("Num"),
                     listutils::basicSymbol<CcInt>()),
    nl->TwoElemList( nl->SymbolAtom("DBName"),
                      listutils::basicSymbol<CcString>()),
    nl->TwoElemList( nl->SymbolAtom("OK"), 
                     listutils::basicSymbol<CcBool>()));
    return nl->TwoElemList(
             listutils::basicSymbol<Stream<Tuple> >(),
             nl->TwoElemList(
                 listutils::basicSymbol<Tuple>(),
                  attrList));

}


class showWorkersInfo{

  public:
    showWorkersInfo(ListExpr resType) : array(0){
       iter = algInstance->workersIterator();
       tt = new TupleType(resType);
    }
    showWorkersInfo(DArray2* _array, ListExpr resType): array(_array), pos(0){
       tt = new TupleType(resType);
    }
    ~showWorkersInfo(){
       tt->DeleteIfAllowed();
    }

    Tuple* next(){
      if(array){
        return nextFromArray();
      } else {
        return nextFromAll();
      }
    }


  private:
    DArray2* array;
    size_t pos;
    typename map<DArray2Element, pair<string, ConnectionInfo*> >::iterator iter;
    TupleType* tt;

    Tuple* nextFromArray(){
      if(!array->IsDefined()){
         return 0;
      }
      while(pos < array->numOfWorkers()){
        DArray2Element elem = array->getWorker(pos);
        pos++;
        string dbname = algInstance->getDBName(elem);
        ConnectionInfo* connectionInfo;
        if(algInstance->workerConnection(elem, dbname, connectionInfo)){
           return createTuple(elem, dbname, connectionInfo);
        }
     }
     return 0;
    }

    Tuple* nextFromAll(){
      if(algInstance->isEnd(iter)){
         return 0;
      }
      Tuple* res = createTuple(iter->first, iter->second.first, 
                               iter->second.second);
      iter++;
      return res;
    }

    Tuple* createTuple(const DArray2Element& elem, string& dbname,
                        ConnectionInfo* ci){
      Tuple* res = new Tuple(tt);
      res->PutAttribute(0, new FText(true, elem.getHost()));
      res->PutAttribute(1, new CcInt(true, elem.getPort()));
      res->PutAttribute(2, new FText(true, elem.getConfig()));
      res->PutAttribute(3, new CcInt(true, elem.getNum()));
      res->PutAttribute(4, new CcString(true, dbname));
      bool ok = ci?ci->check():false;
      res->PutAttribute(5, new CcBool(true,ok));
      return res;
    }

};



int showWorkersVM(Word* args, Word& result, int message,
           Word& local, Supplier s ){

  showWorkersInfo* li = (showWorkersInfo*) local.addr;
  switch(message){
    case OPEN: {

                 if(li){
                    delete li;
                    local.addr =0;
                 }
                 ListExpr tt = nl->Second(GetTupleResultType(s));
                 if(qp->GetNoSons(s)==0){
                   local.addr = new showWorkersInfo(tt);
                 } else {
                   local.addr = new showWorkersInfo((DArray2*) args[0].addr,tt);
                 }
                 return 0;
    }
    case REQUEST:
                 result.addr = li?li->next():0;
                 return result.addr?YIELD:CANCEL;
    case CLOSE:
           if(li){
             delete li;
             local.addr = 0;
           }
           return 0;
  }
  return -1; 
}

OperatorSpec showWorkersSpec(
     " -> stream(tuple), darray2 -> stream(tuple) ",
     " showWorkers([_])",
     "This operator shows information about either all connections to workers "
     "(no argument),  "
     "or connections of a specified darray2 instance.",
     " query showWorkers()  consume "
     );

Operator showWorkersOp(
  "showWorkers",
  showWorkersSpec.getStr(),
  showWorkersVM,
  Operator::SimpleSelect,
  showWorkersTM
);


/*
1.9 Operator dloop2

This operator performs a function over all entries of a darray2.

1.9.1 Type Mpping

Signature: darray2(X) x string x (X->Y) -> darray2(Y)

*/

ListExpr dloop2TM(ListExpr args){


  string err = "darray(X) x string x fun: X -> Y  expected";
  if(!nl->HasLength(args,3) ){
    return listutils::typeError(err + "(wrong number of args)");
  }

  ListExpr temp = args;
  while(!nl->IsEmpty(temp)){
     if(!nl->HasLength(nl->First(temp),2)){
        return listutils::typeError("internal Error");
     }
     temp = nl->Rest(temp);
  }

  ListExpr darray = nl->First(args);
  ListExpr fun;

  if(!CcString::checkType(nl->First(nl->Second(args)))){
     return listutils::typeError("Second arg not of type string");
  }
  fun = nl->Third(args);

  ListExpr funType = nl->First(fun);
  ListExpr arrayType = nl->First(darray);

  if(!DArray2::checkType(arrayType)){
    return listutils::typeError(err + ": first arg not a darray2");
  }
  if(!listutils::isMap<1>(funType)){
    return listutils::typeError(err + ": last arg is not a function");
  }

  if(!nl->Equal(nl->Second(arrayType), nl->Second(funType))){
    return listutils::typeError("type mismatch between darray2 and "
                                "function arg");
  }

  ListExpr result = nl->TwoElemList(listutils::basicSymbol<DArray2>(),
                                    nl->Third(funType));

  if(!DArray2::checkType(result)){
    return listutils::typeError("Invalid function result");
  }

  ListExpr funquery = nl->Second(fun);
  
  ListExpr funargs = nl->Second(funquery);

  ListExpr dat = nl->Second(arrayType);

  ListExpr rfunargs = nl->TwoElemList(
                        nl->First(funargs),
                        dat);
  ListExpr rfun = nl->ThreeElemList(
                        nl->First(funquery),
                        rfunargs,
                        nl->Third(funquery));   


  ListExpr res =  nl->ThreeElemList(
               nl->SymbolAtom(Symbols::APPEND()),
               nl->OneElemList(nl->TextAtom(nl->ToString(rfun))),
               result);

  return res;   


}



class dloop2Info{

  public:
   dloop2Info(DArray2* _array, int _index, string& _resName,string& _fun) : 
     index(_index), resName(_resName), fun(_fun), elem("",0,0,""){
     int wn = _index % _array->numOfWorkers();
     elem = _array->getWorker(wn);
     srcName = _array->getName();
     runner = boost::thread(&dloop2Info::run, this);
  }

  ~dloop2Info(){
     runner.join();
  }


  private:
    int index;
    string resName;
    string fun;
    DArray2Element elem;
    string srcName;
    boost::thread runner;

    void run(){
       string dbname = SecondoSystem::GetInstance()->GetDatabaseName();
       ConnectionInfo* ci = algInstance->getWorkerConnection(elem,dbname);
       if(!ci){
          sendError("Cannot find connection ");
          return;
       }

       string bn = resName + "_" + stringutils::int2str(index);
       string fn = bn+"_fun";
       // delete eventually existing old function
       int err;
       string strres;
       ci->simpleCommand("delete " + fn,err,strres, false);
       // ignore error, standard case: object does not exist
       // create new function object
       string errMsg;
       string cmd = "(let " + fn + " = " + fun +")";
       ci->simpleCommandFromList(cmd, err,errMsg, strres, true);
       if(err!=0){ 
           sendError(" Problem in command " + cmd + ":" + errMsg);
           return;
       }
       // delete old array content
       ci->simpleCommand("delete "+ bn, err,strres, false);
       // ignore error, frequently entry is not there
       cmd = "let " + bn + " =  "  + fn + "("+ srcName + "_" 
                    + stringutils::int2str(index) +")";
       ci->simpleCommand(cmd, err,errMsg, strres, false);
       if(err!=0){ 
           sendError(" Problem in command " + cmd + ":" + errMsg);
       }
       ci->simpleCommand("delete " + fn, err, strres,false);
    }
    void sendError(const string&msg){
       cmsg.error() << msg;
       cmsg.send();
    }

};


int dloop2VM(Word* args, Word& result, int message,
           Word& local, Supplier s ){

   string dbname = SecondoSystem::GetInstance()->GetDatabaseName();
   DArray2* array = (DArray2*) args[0].addr;
   result = qp->ResultStorage(s);
   DArray2* res = (DArray2*) result.addr;
   
  if(!array->IsDefined()){
      res->makeUndefined();
      return 0;
   }  

   string n;
   FText* fun = 0;
   CcString* name = (CcString*) args[1].addr;
   if(!name->IsDefined() || (name->GetValue().length()==0)){
      algInstance->getWorkerConnection(
                          array->getWorker(0),dbname);
      n = algInstance->getTempName();
   } else {
      n = name->GetValue();
   }
   fun = (FText*) args[3].addr;

   if(!stringutils::isIdent(n)){
      res->makeUndefined();
      return 0;
   }
   (*res) = (*array);
   res->setName(n);

   vector<dloop2Info*> runners;
   string f = fun->GetValue();

   for(size_t i=0; i<array->getSize(); i++){
     dloop2Info* r = new dloop2Info(array,i,n,f);
     runners.push_back(r);
   }

   for(size_t i=0;i<runners.size();i++){
     delete runners[i];
   }
   return 0; 
}


OperatorSpec dloop2Spec(
     " darray2(X) x string x  (X->Y) -> darray2(Y)",
     " _ dloop2[_,_]",
     "Performs a function on each element of an darray2 instance."
     "The string argument specifies the name of the result. If the name"
     " is undefined or an empty string, a name is generated automatically.",
     "query da2 dloop2[\"da3\", . count"
     );

Operator dloop2Op(
  "dloop2",
  dloop2Spec.getStr(),
  dloop2VM,
  Operator::SimpleSelect,
  dloop2TM
);


ListExpr
DARRAY2ELEMTM( ListExpr args )
{

  if(!nl->HasMinLength(args,1)){
    return listutils::typeError("at least one argument required");
  }
  ListExpr first = nl->First(args);
  if(!DArray2::checkType(first)){
    return listutils::typeError("darray2 expected");
  }
  return nl->Second(first);
}

OperatorSpec DARRAY2ELEMSpec(
     "darray2(X) -> X ",
     "DARRAY2ELEM(_)",
     "Type Mapping Operator. Extract the type of a darray2.",
     "query da2 dloop2[\"da3\", . count"
     );

Operator DARRAY2ELEMOp (
      "DARRAY2ELEM",
      DARRAY2ELEMSpec.getStr(),
      0,   
      Operator::SimpleSelect,
      DARRAY2ELEMTM );


ListExpr
DARRAY2ELEM2TM( ListExpr args )
{
  if(!nl->HasMinLength(args,2)){
    return listutils::typeError("at least one argument required");
  }
  ListExpr second = nl->Second(args);
  if(!DArray2::checkType(second)){
    return listutils::typeError("darray2 expected");
  }
  ListExpr res =  nl->Second(second);
  return res;
}

OperatorSpec DARRAY2ELEM2Spec(
     "T x darray2(Y) x ... -> Y ",
     "DARRAY2ELEM2(_)",
     "Type Mapping Operator. Extract the type of a darray2.",
     "query da2 da3 dloop2[\"da3\", .. count"
     );

Operator DARRAY2ELEM2Op (
      "DARRAY2ELEM2",
      DARRAY2ELEM2Spec.getStr(),
      0,   
      Operator::SimpleSelect,
      DARRAY2ELEM2TM );
/*
1.8 Operator ~dsummarize2~

This operator puts all parts of an darray2 instance into a single stream.
If relations are stored within the darray, each tuple is a stream element.
If attributes are stored, each element corresponds to a single token in 
the resulting stream. Other subtypes are not supported.

*/
ListExpr dsummarize2TM(ListExpr args){

  string err="darray2(X) with X in rel or DATA expected";
  if(!nl->HasLength(args,1)){
    return listutils::typeError(err);
  }
  ListExpr arg = nl->First(args);
  if(!DArray2::checkType(arg)){
    return listutils::typeError(err);
  }
  ListExpr subtype = nl->Second(arg);
  if(!Relation::checkType(subtype) && !listutils::isDATA(subtype)){
    return listutils::typeError("subtype not supported");
  }
  ListExpr streamtype;
  if(Relation::checkType(subtype)){
     streamtype = nl->Second(subtype);
  } else {
     streamtype = subtype;
  }
  return nl->TwoElemList(listutils::basicSymbol<Stream<Tuple> >(),
                         streamtype);
  
}

/*
1.8.2 LocalInfo for Attributes

*/
class successListener{
  public:
     virtual void jobDone(int id, bool success)=0;
};

class dsummarize2AttrInfoRunner{

  public:
     dsummarize2AttrInfoRunner(DArray2Element& _elem, const string& _objName,
                               ListExpr _resType,
                               const string& _db, int _index,
                               successListener* _listener): 
       elem(_elem), objName(_objName), resType(_resType),db(_db),
       index(_index), listener(_listener), 
       attribute(0){

     }

     void start(){
       runner = boost::thread(&dsummarize2AttrInfoRunner::run,this);
     }
      

     ~dsummarize2AttrInfoRunner(){
          runner.join();
          if(attribute){
              attribute->DeleteIfAllowed();
          }
      }

      Attribute* getAttribute(){
         Attribute* res = attribute;
         attribute = 0;
         return res;
      }

  private:
     DArray2Element elem;
     string objName;
     ListExpr resType;
     string db;
     int index;
     successListener* listener;
     Attribute* attribute;
     boost::thread runner;

     void run(){
         ConnectionInfo* ci = algInstance->getWorkerConnection(elem,db);
         if(!ci){
            listener->jobDone(index, false);
            return;
         } 
         Word result;
         bool ok = ci->retrieve(objName,resType, result,true);
         if(!ok){
           listener->jobDone(index,false);
         } else {
           attribute = (Attribute*) result.addr;
           listener->jobDone(index,true);
         }
     }
};

class dsummarize2AttrInfo : public successListener{

   public:
      dsummarize2AttrInfo(DArray2* _array, ListExpr _resType) : 
        array(_array), resType(_resType),stopped(false), pos(0){
        runner = boost::thread(&dsummarize2AttrInfo::run, this);       
      }

      virtual ~dsummarize2AttrInfo(){
         stopped = true;
         runner.join();
         for(size_t i=0;i<runners.size();i++){
             if(runners[i].first){
                 delete runners[i].first;
             }
         }
      }

      Attribute* next(){
        while(pos < array->getSize()){
           boost::unique_lock<boost::mutex> lock(mtx);
           while(runners.size() <= pos || !runners[pos].second){
               cond.wait(lock);
           }
           Attribute* result = runners[pos].first->getAttribute();
           delete runners[pos].first;
           runners[pos].first = 0;
           pos++;
           if(result){
              return result;
           }
        }
        return 0;
      }

      void jobDone(int index, bool success){
         {
            boost::lock_guard<boost::mutex> lock(mtx);
            runners[index].second = true;
         }
         cond.notify_one();
      } 
   
   private:
      DArray2* array;
      ListExpr resType;
      bool stopped;
      size_t pos;
      vector<pair<dsummarize2AttrInfoRunner*,bool> > runners;
      boost::thread runner;
      boost::condition_variable cond;
      boost::mutex mtx;
      
      void run(){
           string dbname = SecondoSystem::GetInstance()->GetDatabaseName();
           string aname = array->getName();
           while(!stopped && runners.size() < array->getSize()){
              int wnum = runners.size() % array->numOfWorkers();
              DArray2Element elem = array->getWorker(wnum);
              string objname = aname + "_" + 
                               stringutils::int2str(runners.size());
              dsummarize2AttrInfoRunner* r = new 
                      dsummarize2AttrInfoRunner( elem, 
                             objname, resType, dbname, runners.size(), this);
              runners.push_back(make_pair(r,false));
              r->start();
           }
      }
};


/*
1.8.3 LocalInfo for Relations

*/
class dsummarize2RelListener{
  public:
     virtual void connectionFailed(int id) = 0;
     virtual void fileAvailable(int id, const string& fname)=0;
};


class RelationFileGetter{



  public:
     RelationFileGetter(DArray2* _array, int _index, 
                        dsummarize2RelListener* _listener):
      array(_array), index(_index), listener(_listener){
     }

     void operator()(){
       // get the connection
       string dbname = SecondoSystem::GetInstance()->GetDatabaseName();
       DArray2Element elem = array->getWorker(index % array->numOfWorkers());
       ConnectionInfo* ci = algInstance->getWorkerConnection(elem,dbname);
       if(!ci){ // connection failed
          listener->connectionFailed(index);
          return;
       }
       string objName = array->getName()+"_"+stringutils::int2str(index);
       string fname = objName+".bin";
       if(!ci->retrieveRelationFile(objName,fname)){
          listener->connectionFailed(index);
          return;
       }
       listener->fileAvailable(index, fname);
     }

   private:
     DArray2* array;
     int      index;
     dsummarize2RelListener* listener;


};



class dsummarize2RelInfo: public dsummarize2RelListener{

  public:
     dsummarize2RelInfo(DArray2* _array, ListExpr _resType):
         array(_array), currentIndex(0), currentFeeder(0), resType(_resType)
    {
        start();
     }

     Tuple* next() { 
        while(true){
           if(currentFeeder){
              Tuple* res = currentFeeder->next();
              if(res){
                 return res;
              } else {
                 delete currentFeeder;
                 currentFeeder = 0;
                 currentIndex ++;
              }
           }
           if(currentIndex >= array->getSize()){
             return 0;
           }  
           boost::unique_lock<boost::mutex> lock(mtx);
           while(getters[currentIndex]){
               cond.wait(lock);
           }
           if(filenames[currentIndex].length()>0){
              ListExpr tType = 
                  SecondoSystem::GetCatalog()->NumericType(resType);
              currentFeeder = new ffeed5Info(filenames[currentIndex],
                     tType);
           }
         }
         
                

     }
 
     virtual ~dsummarize2RelInfo(){
        for(size_t i=0;i<runners.size();i++){
           if(runners[i]){
              runners[i]->join();
              delete runners[i];
           }
           if(getters[i]){
              delete getters[i];
           }
        }
        if(currentFeeder){
           delete currentFeeder;
        }

     } 

     void connectionFailed(int index){
       { boost::lock_guard<boost::mutex> guard(mtx);
        delete getters[index];
        getters[index] = 0;
       } 
       cond.notify_one();
     }

     void fileAvailable(int index, const string& filename){
       { boost::lock_guard<boost::mutex> guard(mtx);
         filenames[index] = filename;
         delete getters[index];
         getters[index] = 0;
        }
        cond.notify_one();
     }




  private:
     DArray2* array;
     size_t currentIndex; 
     ffeed5Info* currentFeeder;
     ListExpr resType;
     vector<boost::thread*> runners;
     vector<RelationFileGetter*> getters;
     vector<string> filenames;
     boost::mutex mtx;
     boost::condition_variable cond;


     void start(){
        for(size_t i=0;i< array->getSize();i++){
           RelationFileGetter* getter = new RelationFileGetter(array,i,this);
           getters.push_back(getter);
           filenames.push_back("");
           runners.push_back(new boost::thread(*getter));
        }
     }
};


template<class T>
int dsummarize2VMT(Word* args, Word& result, int message,
           Word& local, Supplier s ){

   T* li = (T*) local.addr;
   switch(message){
     case OPEN: if(li) delete li;
                local.addr = new T((DArray2*) args[0].addr, 
                                    nl->Second(qp->GetType(s)));
                return 0;
     case REQUEST:
                 result.addr = li?li->next():0;
                 return result.addr?YIELD:CANCEL;
     case CLOSE:
            if(li){
              delete li;
              local.addr = 0;
            }
   }
   return -1;
}

/*
1.8.4 Selection and Value Mapping

*/

ValueMapping dsummarize2VM[] = {
    dsummarize2VMT<dsummarize2RelInfo>,
    dsummarize2VMT<dsummarize2AttrInfo>
};


int dsummarize2Select(ListExpr args){
  return Relation::checkType(nl->Second(nl->First(args)))?0:1;
}

/*
1.8.5 Specification

*/

OperatorSpec dsummarize2Spec(
     "darray2(DATA) -> stream(DATA> , darray2(rel(X)) -> stream(X)",
     "_ dsummarize",
     "Produces a stream of the darray elements.",
     "query da2 dsummarize2 count"
     );

/*
1.8.6 Operator instance

*/


Operator dsummarize2Op(
  "dsummarize2",
  dsummarize2Spec.getStr(),
  2,
  dsummarize2VM,
  dsummarize2Select,
  dsummarize2TM
);


/*
1.10 Operator ~getValue~

The getValue operator converts a distributed array into a 
normal array from the ArrayAlgebra.

*/

ListExpr getValueTM(ListExpr args){
  string err ="darray2 expected";
  if(!nl->HasLength(args,1)){
    return listutils::typeError(err);
  }
  ListExpr a1 = nl->First(args);
  if(!DArray2::checkType(a1)){
    return listutils::typeError(err);
  }
  return nl->TwoElemList( 
              listutils::basicSymbol<arrayalgebra::Array>(),
              nl->Second(a1));

}

class getValueListener{
  public:
    virtual void  jobDone(int index, Word result)=0;

};

class getValueGetter{
  public:
    getValueGetter(DArray2* _array, int _index, getValueListener* _listener, 
      ListExpr _resType):
       array(_array), index(_index), resType(_resType),listener(_listener){}
       

   void operator()(){

      string dbname = SecondoSystem::GetInstance()->GetDatabaseName();
      int workerIndex = index % array->numOfWorkers();
      ConnectionInfo* ci = algInstance->getWorkerConnection(
                              array->getWorker(workerIndex),dbname);
      Word res((void*)0);
      if(!ci){
          cerr << "workerconnection not found";
          listener->jobDone(index,res); 
          return;
      }
      string name = array->getName()+"_"+stringutils::int2str(index);
      ci->retrieve(name, resType, res,true);
      listener->jobDone(index,res); 
   }

  private:
    DArray2* array;
    int index;
    ListExpr resType;
    getValueListener* listener;

};


class getValueInfo : public getValueListener{

  public:
   getValueInfo(DArray2* _arg, arrayalgebra::Array* _result, ListExpr _resType):
    arg(_arg), result(_result), resType(_resType), algId(0),
    typeId(0),n(-1),values(0){
      isData = listutils::isDATA(nl->Second(_resType));
   }

   virtual ~getValueInfo(){
      delete[] values;
   }


   void convert(){
     if(!init()){
        return;
     }
    vector<getValueGetter*> getters;
    vector<boost::thread*> threads;
    for(int i=0;i<n;i++){
      getValueGetter* getter = new getValueGetter(arg,i,this,
                                                  nl->Second(resType));
      getters.push_back(getter);
      boost::thread* t = new boost::thread(*getter); 
      threads.push_back(t);
    }
    // for for finishing retrieving elements
    for(size_t i =0 ;i<threads.size();i++){
        threads[i]->join();
    }
    for(size_t i =0 ;i<threads.size();i++){
        delete threads[i];
    }
    for(size_t i =0 ;i<getters.size();i++){
       delete getters[i];  
    }
    result->initialize(algId,typeId,n,values);
  } 

  void jobDone(int id, Word value){
    if(value.addr==0){ // problem in getting element
       // set some default value 
       value = am->CreateObj(algId,typeId)(nl->Second(resType));
       if(isData){
          ((Attribute*)value.addr)->SetDefined(false);
       }
    }

    values[id] = value;

  }

  private:
    DArray2* arg;
    arrayalgebra::Array* result;
    ListExpr resType;
    int algId;
    int typeId;
    int n;
    Word* values;
    bool isData;
    
    bool init(){
       if(!arg->IsDefined()){
         return false;
       }
       string typeName;
       if(!SecondoSystem::GetCatalog()->LookUpTypeExpr(nl->Second(resType), 
                                                 typeName, algId, typeId)){
          cerr << "internal error, could not determine algid and typeId"
               << " for " << nl->ToString(nl->Second( resType)) << endl;
          return false;
       }
       n = arg->getSize();
       Word std((void*)0);
       values = new Word[n];
       for(int i=0;i<n;i++){
         values[i] = std;
       }
       return true;
    }



};



int getValueVM(Word* args, Word& result, int message,
            Word& local, Supplier s ){

   result = qp->ResultStorage(s);
   getValueInfo info((DArray2*)args[0].addr, (arrayalgebra::Array*) result.addr,
                   qp->GetType(s)); 
   info.convert();
   return 0;
}


OperatorSpec getValueSpec(
     "darray2(T) -> array(T)",
     "getValue(_)",
     "Converts a distributed array into a normal one.",
     "query getValue(da2)"
     );


Operator getValueOp(
  "getValue",
  getValueSpec.getStr(),
  getValueVM,
  Operator::SimpleSelect,
  getValueTM
);


/*
1.10 OPerator ~dloop2a~

This operator performs a function on the elements of two darray2 values.
The workerlist of both darray2 instances must be the same. The smaller
darray2 instance determines the size of the result. Because the workerlist
must be equal, the single elements of the arrays are on the same worker
and no data must be tramsferred.

1.10.1 Type Mapping

darray2(X) x darray2(Y) x string x (fun: X x Y [->] Z) [->] darray2(Z)

*/
ListExpr dloop2aTM(ListExpr args){
   string err ="darray2(X) x darray2(Y) x string x (X x Y ->Z) expected";
   if(!nl->HasLength(args,4)){
      return listutils::typeError(err);
   } 
   // check wether each element consists of two elements
   // uses args in type mapping
   ListExpr args2 = args;
   while(!nl->IsEmpty(args2)){
      if(!nl->HasLength(nl->First(args2),2)){
        return listutils::typeError("internal error");
      }
      args2 = nl->Rest(args2);
   }
   
   if(    !DArray2::checkType(nl->First(nl->First(args))) 
       || !DArray2::checkType(nl->First(nl->Second(args)))
       || !CcString::checkType(nl->First(nl->Third(args)))
       || !listutils::isMap<2>(nl->First(nl->Fourth(args)))){
      return listutils::typeError(err);    
   }

   ListExpr fa1 = nl->Second(nl->First(nl->First(args)));
   ListExpr fa2 = nl->Second(nl->First(nl->Second(args)));
   ListExpr a1 = nl->Second(nl->First(nl->Fourth(args)));
   ListExpr a2 = nl->Third(nl->First(nl->Fourth(args)));
   if(   !nl->Equal(fa1,a1)
      || !nl->Equal(fa2,a2)){
     return listutils::typeError("function arguments does not fit "
                                 "to the darray2 subtypes");
   }

   ListExpr funRes = nl->Fourth(nl->First(nl->Fourth(args)));
   ListExpr resType = nl->TwoElemList(
                        listutils::basicSymbol<DArray2>(),
                        funRes);
   ListExpr funQuery = nl->Second(nl->Fourth(args));

 
   // replace eventually type mapping operators by the
   // resulting types

   ListExpr fa1o = nl->Second(funQuery);
   fa1 = nl->TwoElemList(
                       nl->First(fa1o),
                       nl->Second(nl->First(nl->First(args))));
   ListExpr fa2o = nl->Third(funQuery);
   fa2 = nl->TwoElemList(
                       nl->First(fa2o),
                       nl->Second(nl->First(nl->Second(args))));
   funQuery = nl->FourElemList(
                   nl->First(funQuery),
                   fa1, fa2, nl->Fourth(funQuery));
 

   string funstr = nl->ToString(funQuery);
 
   return nl->ThreeElemList(nl->SymbolAtom(Symbols::APPEND()),
                            nl->OneElemList(
                               nl->TextAtom(funstr)),
                            resType);
      
}

class dloop2aRunner{
  public:

     dloop2aRunner(DArray2* _a1, DArray2* _a2, int _index, 
                   const string& _fun, const string& _rname):
       a1(_a1), a2(_a2), index(_index),fun(_fun), rname(_rname){
     }

     void operator()(){
      string dbname = SecondoSystem::GetInstance()->GetDatabaseName();
      int workerIndex = index % a1->numOfWorkers();
      ConnectionInfo* ci = algInstance->getWorkerConnection(
                              a1->getWorker(workerIndex),dbname);
      if(!ci){
         cmsg.error() << "could not open connection" ;
         cmsg.send();
         return;
      }
      // name of the function to create
      string fn = rname + "_" + stringutils::int2str(index)+"_fun";
      string cmd ="(let " + fn + " = " + fun +")";
      int err;
      string errMsg;
      string strres;
      // delete function if exists
      ci->simpleCommand("delete " + fn , err,errMsg, strres, false);
      // ignore error
      // create new function
      ci->simpleCommandFromList(cmd, err,errMsg, strres, true);
      if(err!=0){
         cerr << "error in creating function via " << cmd << endl;
         cerr << errMsg << endl;
         return;
      }
      // apply function to array elements
      string tname = rname + "_" + stringutils::int2str(index);
      // remove old stuff
      ci->simpleCommand("delete " + tname , err,errMsg, strres, false);
      // create new one
      string a1o = a1->getName()+"_"+stringutils::int2str(index);
      string a2o = a2->getName()+"_"+stringutils::int2str(index);
      cmd = "let " + tname + " = " + fn +"("+ a1o + ", " + a2o+")";
      ci->simpleCommand(cmd, err,errMsg, strres, false);
      if(err!=0){
         cerr << "error in creating result via " << cmd << endl;
         cerr << errMsg << endl;
         return;
      }
      ci->simpleCommand("delete " + fn , err,errMsg,strres, false); 
     }



  private:
      DArray2* a1;
      DArray2* a2;
      int index;
      string fun;
      string rname;
};



int dloop2aVM(Word* args, Word& result, int message,
            Word& local, Supplier s ){

  DArray2* a1 = (DArray2*) args[0].addr;
  DArray2* a2 = (DArray2*) args[1].addr;
  result = qp->ResultStorage(s);
  DArray2* res = (DArray2*) result.addr;
  CcString* name = (CcString*) args[2].addr;
  FText* funQuery = (FText*) args[4].addr;
  string dbname = SecondoSystem::GetInstance()->GetDatabaseName();

  if(!a1->IsDefined() || !a2->IsDefined() ){
      res->makeUndefined();
      return 0;
  }
  string n;
  if(!name->IsDefined() || (name->GetValue().length()==0)){
      algInstance->getWorkerConnection(
                          a1->getWorker(0),dbname);
      n = algInstance->getTempName();
  } else {
      n = name->GetValue();
  }
  if(!stringutils::isIdent(n)){
      res->makeUndefined();
      return 0;
  }


  // check whether workers of both arrays are the same
  if(a1->numOfWorkers()!=a2->numOfWorkers()){
    cmsg.error() << "Different workers in dloop2a";
    cmsg.send();
    res->makeUndefined();
    return 0;
  } 

  vector<DArray2Element> workers;

  for(size_t i=0;i<a1->numOfWorkers();i++){
    if(a1->getWorker(i) != a2->getWorker(i)){
        cmsg.error() << "Different workers in dloop2a";
        cmsg.send();
        res->makeUndefined();
        return 0;
    }
    workers.push_back(a1->getWorker(i));
  } 

  int max = min(a1->getSize(), a2->getSize());

  res->set(max,n,workers);

  vector<dloop2aRunner*> runners;
  vector<boost::thread*> threads;
  string funstr = funQuery->GetValue();


  for(int i=0;i<max;i++){
      dloop2aRunner* runner = new dloop2aRunner(a1,a2,i,funstr, n);
      runners.push_back(runner);
      boost::thread* thread = new boost::thread(*runner);
      threads.push_back(thread);
  }

  for(size_t i=0;i<threads.size(); i++){
     threads[i]->join();
     delete threads[i];
     delete runners[i];
  }

  return 0;
}

/*
11.3 Specification

*/
OperatorSpec dloop2aSpec(
     "darray2(X) x darray2(Y) x string x (fun : X x Y -> Z) -> darray2(Z)",
     "_ _ dloop2a[_,_]",
     "Performs a funtion on the elements of two darray2 instances. The "
     "string argument specifies the name of the resulting darray2." 
     " If the string is undefined or empty, a name is generated automatically",
     "query da1 da2 dloop2a[\"newName\", fun(i1 : int, i2 : int) i1 + i2) "
     );
/*
11.4 Operator instance

*/


Operator dloop2aOp(
  "dloop2a",
  dloop2aSpec.getStr(),
  dloop2aVM,
  Operator::SimpleSelect,
  dloop2aTM
);





/*
1.11 Operator fdistribute6

1.11.1 TRype Mapping

 stream(tuple) x {string,text} x int -> stream(tuple)


*/

ListExpr fdistribute6TM(ListExpr args){
  string err = "stream(tuple) x {string,text} x int expected";
  if(!nl->HasLength(args,3)){
    return listutils::typeError(err);
  }
  if(!Stream<Tuple>::checkType(nl->First(args))){
    return listutils::typeError(err);
  }
  if(!CcString::checkType(nl->Second(args)) &&
     !FText::checkType(nl->Second(args))){
    return listutils::typeError(err);
  }
  if(!CcInt::checkType(nl->Third(args))){
    return listutils::typeError(err);
  }
  return nl->First(args);
}

/*
1.11.2 LocalInfo


*/
template<class T>
class fdistribute6Info{
  public:
    fdistribute6Info(Word _stream, T* _fname, CcInt* _max, 
                     ListExpr _resultType): 
      stream(_stream){
       counter = 0;
       fileCounter = 0;
       out = 0;
       buffer = 0;
       if(!_fname->IsDefined() || !_max->IsDefined()){
            c = 0;
            return;
       }
       bname = ( _fname->GetValue());
       stringutils::trim(bname);
       if(bname.length()==0){
          c=0;
          return;
       } 
       c = _max->GetValue();
       if(c<0){
          c = 0;
       }
       relType = nl->TwoElemList(listutils::basicSymbol<Relation>(),
                                 nl->Second(_resultType));
       stream.open();
    }
    
    ~fdistribute6Info(){
      finishCurrentFile();
      stream.close();
    }


    Tuple* next(){
      Tuple* in = stream.request();
      if(c==0){
         return in;
      }
      if(!c){
        finishCurrentFile();
      } else {
        storeTuple(in);
      }
      return in;
    }



  private:
    Stream<Tuple> stream;
    string bname;
    int c;
    ListExpr relType;
    int counter;
    int fileCounter;
    ofstream* out;
    char* buffer;


    void finishCurrentFile(){
       if(out){
          BinRelWriter::finish(*out);
          out->close();
          delete out;
          delete[] buffer;
          out=0;
          buffer = 0;
       }
    }

    void storeTuple(Tuple* tuple){
       if(!tuple){
          return;
       }
       if(!out){
          createNextOutputFile();
       }
       BinRelWriter::writeNextTuple(*out,tuple);
       counter++;
       if(counter==c){
         finishCurrentFile();
         counter = 0;
       }
    } 

    void createNextOutputFile(){
      string fname = bname +"_"+stringutils::int2str(fileCounter);
      fileCounter++;
      out = new ofstream(fname.c_str(), ios::binary | ios::out);
      buffer = new char[FILE_BUFFER_SIZE];
      out->rdbuf()->pubsetbuf(buffer, FILE_BUFFER_SIZE);
      BinRelWriter::writeHeader(*out,relType);
    }

};


template<class T>
int fdistribute6VMT(Word* args, Word& result, int message,
            Word& local, Supplier s ){

  fdistribute6Info<T>* li = (fdistribute6Info<T>*) local.addr;

  switch(message){
     case OPEN:
            if(li) delete li;
            local.addr = new fdistribute6Info<T>(args[0], (T*) args[1].addr, 
                                             (CcInt*) args[2].addr, 
                                              qp->GetType(s));
            return 0;
     case REQUEST:
            result.addr = li?li->next():0;
            return result.addr?YIELD:CANCEL;
     case CLOSE:
           if(li){
              delete li;
              local.addr = 0;
           }
           return 0;
  }
  return -1; 
}

/*
11.3 Value Mapping and Selection

*/

int fdistribute6Select(ListExpr args){
  return CcString::checkType(nl->Second(args))?0:1;
}

ValueMapping fdistribute6VM[] = {
   fdistribute6VMT<CcString>,
   fdistribute6VMT<FText>
};

/*
11.4 Specification

*/

OperatorSpec fdistribute6Spec(
     " stream(tuple> x {string,text} x int -> stream(tuple)",
     "_ fdistribute6[<filename>, <numElemsPerFile>]",
     "Distributes a tuple stream into a set of files. " 
     " The first <numOfElemsPerFile> elemenst of the stream "
     " are stored into the first file and so on. "
     "The given basicfilename is extended by an underscore and"
     " a running number" ,
     "query strassen feed fdistribute6['strassen',1000] count"
     );

/*
11.5 Operator instance

*/
Operator fdistribute6Op(
  "fdistribute6",
  fdistribute6Spec.getStr(),
  2,
  fdistribute6VM,
  fdistribute6Select,
  fdistribute6TM
);


/*
12 Operator ~deleteRemoteObjects~

12.1 Type Mapping

This operator get a darray2 instance and optionally an integer 
value. If the integer value is given, only the object for the 
specified index is deleted otherwise all objects handled by this
darray2 object. The return value is the number of successfully
deleted objects.

*/
ListExpr deleteRemoteObjectsTM(ListExpr args){
  string err = " darray2 [x int] expected";
  if(!nl->HasLength(args,1) && !nl->HasLength(args,2)){
    return listutils::typeError(err + ": invalid number of args" );
  }
  if(!DArray2::checkType(nl->First(args))){
    return listutils::typeError(err + ": first arg not a darray2");
  }
  if(nl->HasLength(args,2)){
    if(!CcInt::checkType(nl->Second(args))){
       return listutils::typeError(err+  ": second arg is not an int");
    }
  }
  return listutils::basicSymbol<CcInt>();
}


class Object_Del{

   public:

      Object_Del(DArray2* _array, int _index):
       array(_array),index(_index), del(0){
      }

      Object_Del(Object_Del& src): array(src.array),
         index(src.index), del(src.del){}


      void operator()(){
        string dbname = SecondoSystem::GetInstance()->GetDatabaseName();
        int workerIndex = index % array->numOfWorkers();
        ConnectionInfo* ci = algInstance->getWorkerConnection(
                              array->getWorker(workerIndex),dbname);
        if(!ci){
           cmsg.error() << "could not open connection" ;
           cmsg.send();
           return;
        }
        string objName = array->getName() +"_" + stringutils::int2str(index);
        int err;
        string errMsg;
        string resstr;
        ci->simpleCommand("delete " + objName, err, errMsg, resstr, false);
        if(err==0){
          del = 1;
        } 
      }

      int getNumber(){
        return del;
      }




   private:
      DArray2* array;
      int index;
      int del;

};


int deleteRemoteObjectsVM(Word* args, Word& result, int message,
            Word& local, Supplier s ){

  result = qp->ResultStorage(s);
  CcInt* res = (CcInt*) result.addr;
  DArray2* array = (DArray2*) args[0].addr;
  if(!array->IsDefined()){
     res->Set(true,0);
     return 0;
  }
  bool all = true;
  int index = 0;
  if(qp->GetNoSons(s)==2){
     all = false;
     CcInt* i = (CcInt*) args[1].addr;
     if(!i->IsDefined()){
       res->Set(true,0);
       return 0;
     }
     index = i->GetValue();
     if(index < 0 || index >= (int) array->getSize()){
        res->Set(true,0);
        return 0;
     }
  }

  vector<Object_Del*> deleters;
  vector<boost::thread*> threads;
  int count = 0;
  if(!all){
    Object_Del* del = new Object_Del(array,index);
    deleters.push_back(del);
    boost::thread* thread = new boost::thread(&Object_Del::operator(),del);
    threads.push_back(thread);
  } else {
    for(size_t i=0;i<array->getSize();i++){
       Object_Del* del = new Object_Del(array,i);
       deleters.push_back(del);
       boost::thread* thread = new boost::thread(&Object_Del::operator(),del);
       threads.push_back(thread);
    }
  }

  for(size_t i=0;i<threads.size();i++){
     threads[i]->join();
     delete threads[i];
     count += deleters[i]->getNumber();
     delete deleters[i];
  }
  res->Set(true,count);
  return 0;
}



OperatorSpec deleteRemoteObjectsSpec(
     " darray2 [x int] -> int",
     "deleteRemoteObjects(_,_)",
     "Deletes the remote objects managed by a darray2 object. " 
     "If the optionally integer argument is given, only the "
     "object at the specified index is deleted. ",
     "query deleteRemoteObjects(da2)"
 );

Operator deleteRemoteObjectsOp(
  "deleteRemoteObjects",
  deleteRemoteObjectsSpec.getStr(),
  deleteRemoteObjectsVM,
  Operator::SimpleSelect,
  deleteRemoteObjectsTM
);


/*
12 Operator ~clone~

This operator gets a darray2 and a string. It produces a new darray2 
as a clone of the first argument having a new name.

12.1 Type Mapping

*/
ListExpr cloneTM(ListExpr args){
  string err = "darray2 x string expected";
  if(!nl->HasLength(args,2)){
     return listutils::typeError(err + ": invalid number of args");
  }
  if(!DArray2::checkType(nl->First(args)) ||
     !CcString::checkType(nl->Second(args))){
    return listutils::typeError(err);
  }
  return nl->First(args);
}

/*
12.2 Class for a single task

*/
class cloneTask{

  public:

     cloneTask(DArray2* _array, int _index, const string& _name) :
        array(_array), index(_index), name(_name){}


     void run(){
        string dbname = SecondoSystem::GetInstance()->GetDatabaseName();
        int workerIndex = index % array->numOfWorkers();
        ConnectionInfo* ci = algInstance->getWorkerConnection(
                              array->getWorker(workerIndex),dbname);
        if(!ci){
           cmsg.error() << "could not open connection" ;
           cmsg.send();
           return;
        }
        string objName = array->getName() +"_" + stringutils::int2str(index);
        string newName = name + "_" + stringutils::int2str(index);
        int err;
        string errMsg;
        string resstr;
        ci->simpleCommand("let " + newName + " = " + objName , 
                          err, errMsg, resstr, false);
     }


  private:
      DArray2* array;
      int index;
      string name;
};


/*
12.3 Value Mapping Function

*/

int cloneVM(Word* args, Word& result, int message,
            Word& local, Supplier s ){

   DArray2* array = (DArray2*) args[0].addr;
   CcString* name = (CcString*) args[1].addr;
   result = qp->ResultStorage(s);
   DArray2* res = (DArray2*) result.addr;
   (*res) = (*array);

   if(!array->IsDefined() || !name->IsDefined()){
      res->makeUndefined();
      return 0;
   }
   string n = name->GetValue();

   if(!stringutils::isIdent(n)){
      res->makeUndefined();
      return 0;
   }
   res->setName(n);

   vector<cloneTask*> cloner;
   vector<boost::thread*> threads;

   for(size_t i=0; i<array->getSize();i++){
      cloneTask* ct = new cloneTask(array,i,n);
      cloner.push_back(ct);
      boost::thread* t = new boost::thread(&cloneTask::run, ct);
      threads.push_back(t);
   }

   for(size_t i=0;i<array->getSize(); i++){
       threads[i]->join();
       delete threads[i];
       delete cloner[i];
   }
   return 0;
}



/*
12.4 Specification

*/
OperatorSpec cloneSpec(
     " darray2 x string -> darray2",
     " _ clone[_]",
     "Creates a copy of a darray2 with a new name. " ,
     "let da9 = da8 clone[\"da9\"] "
 );


/*
12.5 Operator instance

*/
Operator cloneOp(
  "clone",
  cloneSpec.getStr(),
  cloneVM,
  Operator::SimpleSelect,
  cloneTM
);


/*
13 Operator ~share~

This operator stores a local object to all connections either of a given darray2
or all user defined connections.

*/

ListExpr shareTM(ListExpr args){
  string err = "string x bool [x darray2] expected";
  if(!nl->HasLength(args,2) && !nl->HasLength(args,3)){
    return listutils::typeError(err);
  }
  if(!CcString::checkType(nl->First(args))){
    return listutils::typeError(err);
  }
  if(!CcBool::checkType(nl->Second(args))){
    return listutils::typeError(err);
  }
  if(nl->HasLength(args,3)){
    if(!DArray2::checkType(nl->Third(args))){
       return listutils::typeError(err);
    }
  }
  return listutils::basicSymbol<FText>(); 
}


class shareRunner{

 public:
    shareRunner(ConnectionInfo* _ci, const string& _objName, 
                const string& _fileName, const bool _relation, 
                int _id, bool _allowOverwrite, successListener* _listener):
            ci(_ci), objName(_objName), fileName(_fileName),relation(_relation),
            id(_id), allowOverwrite(_allowOverwrite), listener(_listener) {}
 

    void operator()(){
      if(relation){
         bool r = ci->createOrUpdateRelationFromBinFile(objName, 
                                                 fileName, allowOverwrite);
         listener->jobDone(id,r);
      } else {
          int err;
          string res;
          string cmd = "delete " + objName;
          if(allowOverwrite){
             ci->simpleCommand(cmd,err,res, false);
          }
          cmd = "restore " +  objName + " from '" + fileName +"'";           
          ci->simpleCommand(cmd,err,res,false);
          listener->jobDone(id, err==0);
      }
    }

 private:
     ConnectionInfo* ci;
     string objName;
     string fileName;
     bool relation;
     int id;
     bool allowOverwrite;
     successListener* listener;

};


class shareInfo: public successListener{

  public:
    shareInfo(const string& _name, const bool _allowOverwrite,
              DArray2* _array, FText* _result): name(_name),
              allowOverwrite(_allowOverwrite), array(_array),
              result(_result), fileCreated(false) {
      failed = 0;
      success = 0;
   }


    void share(){
       SecondoCatalog* ctlg = SecondoSystem::GetCatalog();
       string tn;
       bool defined;
       bool hasTypeName;
       if(!ctlg->GetObjectExpr(name, tn, typeList, value, 
                               defined, hasTypeName)){
          result->Set(true, "Name " + name + " is not on object");
          return;
       }
       if(!defined){
         result->Set("Undefined objects cannot be shared");
         return;
       }

       if(array){
         shareArray();
       } else {
         shareUser();
       }
   
       for(size_t i=0;i<runners.size();i++){
         runners[i]->join();
         delete runners[i];
       }
       string t = "success : " + stringutils::int2str(success)+ ", failed "
                  + stringutils::int2str(failed);
       result->Set(true,t);
       FileSystem::DeleteFileOrFolder(filename); 

    }

    void jobDone(int id, bool success){
       if(success){
          this->success++;
       } else {
          this->failed++;
       }
    }


  private:
    string name;
    bool allowOverwrite;
    DArray2* array;
    FText* result;
    bool fileCreated;
    Word value;
    ListExpr typeList;
    set<pair <string, int> > cons;
    string filename;
    bool isRelation;
    boost::mutex createFileMutex;
    vector<boost::thread*> runners;
    int failed;
    int success;

    void shareArray() {
        
       string dbname = SecondoSystem::GetInstance()->GetDatabaseName();
       for(size_t i=0;i<array->numOfWorkers();i++){
          ConnectionInfo* ci = algInstance->getWorkerConnection(
                              array->getWorker(i),dbname);
          share(ci);
       }
    }

    void shareUser() {
       string dbname = SecondoSystem::GetInstance()->GetDatabaseName();
       for(size_t i=0; i< algInstance->noConnections(); i++){
           ConnectionInfo* ci = algInstance->getConnection(i);
           if(ci){
              ci->switchDatabase(dbname, true);
              share(ci); 
           }
       } 
    }

    void share(ConnectionInfo* ci){
      pair<string,int> con(ci->getHost(), ci->getPort());
      if(cons.find(con)==cons.end()){
         cons.insert(con);
         createFile(ci); 
         shareRunner runner(ci,name,filename, isRelation, cons.size(), 
                            allowOverwrite, this);
         boost::thread * t = new boost::thread(runner);
         runners.push_back(t);
      }
    }

    void createFile(ConnectionInfo* ci){
      boost::lock_guard<boost::mutex> guard(createFileMutex);
      if(!fileCreated){
          isRelation = Relation::checkType(typeList);
          filename = name + "_" + stringutils::int2str(WinUnix::getpid()) 
                     + ".bin";
          if(isRelation){
             ci->saveRelationToFile(typeList, value, filename);
          } else {
             ci->storeObjectToFile(name, value, typeList, filename); 
          }
          fileCreated=true;
      }      
    }

};


int shareVM(Word* args, Word& result, int message,
            Word& local, Supplier s ){

  CcString* objName = (CcString*) args[0].addr;
  CcBool* overwrite = (CcBool*) args[1].addr;
  DArray2* array = qp->GetNoSons(s)==3?(DArray2*) args[2].addr: 0;
  result = qp->ResultStorage(s);
  FText* res = (FText*) result.addr;
  if(array && !array->IsDefined()){
     res->SetDefined(false);
     return 0;
  }
  if(!objName->IsDefined() || !overwrite->IsDefined()){
     res->SetDefined(false);
     return 0;
  }
  shareInfo info(objName->GetValue(), overwrite->GetValue(), array, res);
  info.share();
  return 0;
}


OperatorSpec shareSpec(
     "string x bool [x darray2] -> text",
     "share(ObjectName, allowOverwrite, workerArray)",
     "distributes an object from local database to the workers."
     "The allowOverwrite flag controls whether existing objects "
     "with the same name should be overwritten. Id the optional "
     "darray2 argument is given, the object is stored at all "
     "workers contained within this array. If this argument is "
     "missing the user defined connections are used as workers.",
     "query share(\"ten\", TRUE,da8) "
 );


/*
12.5 Operator instance

*/
Operator shareOp(
  "share",
  shareSpec.getStr(),
  shareVM,
  Operator::SimpleSelect,
  shareTM
);


/*
13 Operator ~cleanUp~

This operators removes all temporarly objects from remote server
either from a given darray2 object or all connected objects (including
worker connections).

13.1 Type Mapping

*/
ListExpr cleanUpTM(ListExpr args){
  string err= " no argument or darray2 expected";
  if(!nl->HasLength(args,1) && !nl->IsEmpty(args)){
     return listutils::typeError(err);
  }
  if(nl->HasLength(args,1)){
     if(!DArray2::checkType(nl->First(args))){
        return listutils::typeError(err);
     }
  }
  return listutils::basicSymbol<CcBool>();
}


/*
13.2 Value Mapping

*/
int cleanUpVM(Word* args, Word& result, int message,
            Word& local, Supplier s ){
  result = qp->ResultStorage(s);
  CcBool* res = (CcBool*) result.addr;
  res->Set(true,true);

  if(qp->GetNoSons(s)==0){
    algInstance->cleanUp();
  } else {
    set<pair<string,int> > used;
    DArray2* arg = (DArray2*) args[0].addr;
    if(!arg->IsDefined()){
      res->SetDefined(false);
      return 0;
    }
    
    string dbname = SecondoSystem::GetInstance()->GetDatabaseName();
    vector<boost::thread*> runners;
    for(size_t i=0;i<arg->numOfWorkers();i++){
      DArray2Element e = arg->getWorker(i);
      pair<string,int> p(e.getHost(),e.getPort());
      if(used.find(p)==used.end()){
         used.insert(p);
         ConnectionInfo* ci = algInstance->getWorkerConnection(e,dbname);
         if(ci){
             boost::thread* r = new boost::thread(&ConnectionInfo::cleanUp,ci);
             runners.push_back(r);
         }
      }
    }
    for(size_t i=0;i<runners.size();i++){
        runners[i]->join();
        delete runners[i];
    }
  }
  return 0;
}


/*
13.3 Specification

*/
OperatorSpec cleanUpSpec(
     "-> bool , darray2 -> bool",
     "cleanUp(_)",
     "Removes temporarly objects, i.e. objects whose name starts with TMP_, "
     "from remote servers. If no argument is given, all open connections to "
     "servers are used for removing objects. If the darray2 argument is "
     "present, only the workers specified by this argument are used.",
     "query cleanUp() "
 );

/*
13.4 Operator instnace

*/

Operator cleanUpOp(
  "cleanUp",
  cleanUpSpec.getStr(),
  cleanUpVM,
  Operator::SimpleSelect,
  cleanUpTM
);



/*
3 Implementation of the Algebra

*/
Distributed2Algebra::Distributed2Algebra(){
   namecounter = 0;

   AddTypeConstructor(&DArray2TC);
   DArray2TC.AssociateKind(Kind::SIMPLE());
   
   AddOperator(&connectOp);
   AddOperator(&checkConnectionsOp);
   AddOperator(&rcmdOp);
   AddOperator(&disconnectOp);
   AddOperator(&rqueryOp);
   rqueryOp.SetUsesArgsInTypeMapping();
   AddOperator(&prcmdOp);
   AddOperator(&sendFileOp);
   AddOperator(&requestFileOp);
   AddOperator(&psendFileOp);
   AddOperator(&prequestFileOp);
   AddOperator(&getRequestFolderOp);
   AddOperator(&getSendFolderOp);
   AddOperator(&pconnectOp);
   AddOperator(&pqueryOp);
   pqueryOp.SetUsesArgsInTypeMapping();
   AddOperator(&pquery2Op);
   pquery2Op.SetUsesArgsInTypeMapping();

   AddOperator(&putOp);
   AddOperator(&getOp);
   AddOperator(&sizeOp);
   AddOperator(&getWorkersOp);

   AddOperator(&fconsume5Op);
   AddOperator(&ffeed5Op);
   ffeed5Op.SetUsesArgsInTypeMapping();

   AddOperator(&createDarray2Op);

   AddOperator(&pputOp);
   AddOperator(&ddistribute2Op);
   AddOperator(&ddistribute3Op);
   AddOperator(&ddistribute4Op);
   AddOperator(&fdistribute5Op);
   AddOperator(&fdistribute6Op);
   AddOperator(&closeWorkersOp);
   AddOperator(&showWorkersOp);
   AddOperator(&dloop2Op);
   dloop2Op.SetUsesArgsInTypeMapping();
   AddOperator(&dloop2aOp);
   dloop2aOp.SetUsesArgsInTypeMapping();
   AddOperator(&DARRAY2ELEMOp);
   AddOperator(&DARRAY2ELEM2Op);

   AddOperator(&dsummarize2Op);
   AddOperator(&getValueOp);
   AddOperator(&deleteRemoteObjectsOp);
   AddOperator(&cloneOp);
   AddOperator(&shareOp);
   AddOperator(&cleanUpOp);
}


extern "C"
Algebra*
   InitializeDistributed2Algebra( NestedList* nlRef,
                             QueryProcessor* qpRef,
                             AlgebraManager* amRef ) {

   algInstance = new Distributed2Algebra();
   return algInstance;
}


