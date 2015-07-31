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

#include "Algebra.h"
#include "NestedList.h"
#include "QueryProcessor.h"
#include <exception>
#include <string>
#include <map>
#include <vector>
#include "ListUtils.h"
#include "Attribute.h"
#include "AlgebraManager.h"
#include "Operator.h"
#include "StandardTypes.h"
#include "NList.h"
#include "Symbols.h"
#include "SecondoCatalog.h"
#include "SecondoSystem.h"
#include "RelationAlgebra.h"
#include "MainMemory.h"
#include "Stream.h"
#include "MMRTree.h"
#include "MovingRegionAlgebra.h"
#include "RectangleAlgebra.h"

#include "AvlTree.h"






using namespace std;
extern NestedList* nl;
extern QueryProcessor *qp;
extern SecondoSystem* instance;
// extern AlgebraManager *am;

namespace mmalgebra {

MemCatalog* catalog;


/*

4 Auxiliary functions

4.1 ~tmStringBool~

Function checks the type mapping string-> bool.
It returns a list expression for the result type,
otherwise the symbol ~typeerror~.

*/

ListExpr tmStringBool(ListExpr args) {
string err = "string expected";
  if(nl->ListLength(args)!=1){
     return listutils::typeError(err + " (wrong number of arguments)");
  }
  if (!CcString::checkType(nl->First(args))) {
  return listutils::typeError(err);
    }
  return listutils::basicSymbol<CcBool>();


}
/*

4.2 ~tmStringStringString~

Function checks the type mapping string x string -> string.
It returns a list expression for the result type,
otherwise the symbol ~typeerror~.

*/
ListExpr tmStringStringString (ListExpr args) {

string err = "string expected";
  if(nl->ListLength(args)!=2){
     return listutils::typeError(err + " (wrong number of arguments)");
  }

  if (!CcString::checkType(nl->First(args))
            ||(!CcString::checkType(nl->Second(args)))) {
      return listutils::typeError(err);
  }
  return listutils::basicSymbol<CcString>();

}


ListExpr tmStringMemloadBool (ListExpr args){

    if(nl->ListLength(args)!=2){
        return listutils::typeError("wrong number of arguments");
    }
    if (!CcString::checkType(nl->First(args))) {
        return listutils::typeError("string expected as first argument");
    };

    if (listutils::isRelDescription(nl->Second(args))) {
        return listutils::basicSymbol<CcBool>();
    };

    if (listutils::isDATA(nl->Second(args))) {
        return listutils::basicSymbol<CcBool>();
    }
    if (listutils::isTupleStream(nl->Second(args))){
        return listutils::basicSymbol<CcBool>();
    }

    return listutils::typeError ("the second argument has to "
    "be of kind DATA or a relation or a tuplestream");

}


/*

4.7 ~relToVector~

Function to fill a ~vector<tuple>~ with the tuples of a given relation

*/

MemoryRelObject* relToVector(GenericRelation* r, ListExpr le = 0) {

    GenericRelationIterator* rit;
    rit = r->MakeScan();
    Tuple* tup;
    int tupleSize=0;
    size_t availableMemSize = catalog->getAvailabeMemSize();
    size_t usedMainMemory=0;

    vector<Tuple*>* mmrel = new vector<Tuple*>();

    while ((tup = rit->GetNextTuple()) != 0){
           // tup->bringToMemory();
            tupleSize = tup->GetMemSize();
            if ((size_t)tupleSize<availableMemSize){
                mmrel->push_back(tup);
                // tup->IncReference(); ???
                usedMainMemory += tupleSize;
                availableMemSize -= tupleSize;
            }
            else{
             cout<< "the memSize is not enough, the object"
            "might be usable but not complete"<<endl;
             break;
            }
    }


    MemoryRelObject* mmRelObject =
        new MemoryRelObject(mmrel, usedMainMemory, nl->ToString(le));


    return mmRelObject;
}

MemoryAttributeObject* attrToMM(Attribute* attr, ListExpr le){

    size_t availableMemSize = catalog->getAvailabeMemSize();
    size_t usedMainMemory=0;
    usedMainMemory = attr->GetMemSize();
    if (usedMainMemory>availableMemSize){
            cout <<"the available main memory size is not enough"<<endl;
            return 0;
        }
        MemoryAttributeObject* mmA =
            new MemoryAttributeObject(attr, usedMainMemory, nl->ToString(le));

    return mmA;

}


MemoryRelObject* tupelStreamToRel(Word arg, ListExpr le){

    vector<Tuple*>* mmrel = new vector<Tuple*>();
    Stream<Tuple> stream(arg);
    size_t availableMemSize = catalog->getAvailabeMemSize();
    size_t usedMainMemory =0;
    Tuple* tup;
    int tupleSize = 0;

    stream.open();

    while( (tup = stream.request()) != 0){
        tupleSize = tup->GetMemSize();
            if ((size_t)tupleSize<availableMemSize){
                mmrel->push_back(tup);
                usedMainMemory += tupleSize;
                availableMemSize -= tupleSize;
            }
            else{
                cout<< "the memSize is not enough, the object "
                        " might be usable but not complete"<<endl;
                break;
            }
    }

    MemoryRelObject* mem =
            new MemoryRelObject(mmrel, usedMainMemory, nl->ToString(le));

    stream.close();

    return mem;
}

/*

5 Creating Operators

5.1 Operator ~memload~

Load a persistent relation into main memory. If there is not enough space
it breaks up. The created ~MemoryRelObject~ is usable but not complete.

5.1.1 Type Mapping Functions of operator ~memload~ (string -> bool)


*/
ListExpr memloadTypeMap(ListExpr args)
{
 return tmStringBool(args);
}




/*
5.1.3  The Value Mapping Functions of operator ~memload~

*/

int memloadValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {


    CcString* oN = (CcString*) args[0].addr;
          if(!oN->IsDefined()){
             return 0;
          }
    result  = qp->ResultStorage(s);
    CcBool* b = static_cast<CcBool*>(result.addr);

    string objectName = oN->GetValue();
    SecondoCatalog* cat = SecondoSystem::GetCatalog();
    bool memloadSucceeded=false;
    Word object; //save the persistent object
    ListExpr objectTypeExpr=0; //type expression of the persistent object
    string objectTypeName=""; //used by cat->GetObjectTypeExpr

    if (!cat->IsObjectName(objectName)){
        cout<<"identifier is not in use"<<endl;
        b->Set(true, memloadSucceeded);
        return 0;
        }
    if (catalog->isMMObject(objectName)){
        memloadSucceeded =true;
        cout<<"the object is already a main memory member"<<endl;
        b->Set(true, memloadSucceeded);
        return 0;
    }

        bool defined = false;
        bool hasTypeName = false;
        memloadSucceeded = cat->GetObjectExpr(objectName,
                objectTypeName,objectTypeExpr,object,defined,hasTypeName);


    // object is a relation
    if (Relation::checkType(objectTypeExpr)&&defined){
        GenericRelation* r= static_cast<Relation*>( object.addr );
        MemoryRelObject* mmRelObject =
                relToVector(r, nl->Second(objectTypeExpr));
        catalog->insert(objectName,mmRelObject);
    }

    // object is attribute
    if (Attribute::checkType(objectTypeExpr)&&defined){
        Attribute* attr = (Attribute*)object.addr;
        MemoryAttributeObject* mmA = attrToMM(attr, objectTypeExpr);

        if (mmA!=0){
        catalog->insert(objectName,mmA);
        }
    }

    // object is neither an attribute nor a relation
    if (!Attribute::checkType(objectTypeExpr)
            &&!Relation::checkType(objectTypeExpr)){
        memloadSucceeded = false;
        cout<<"a relation or an attribute expected"<<endl;
    }


    b->Set(true, memloadSucceeded);

    return 0;
}


/*

5.1.4 Description of operator ~memload~

*/

OperatorSpec memloadSpec(
    "string -> bool",
    "memload(_)",
    "loads a persistent object to main memory "
    "if there is not enough space, the loaded object may be not complete"
    "but usable",
    "query memload('plz')"
);

/*

5.1.5 Instance of operator ~memload~

*/

Operator memloadOp (
    "memload",
    memloadSpec.getStr(),
    memloadValMap,
    Operator::SimpleSelect,
    memloadTypeMap
);

/*
5.2 Operator ~meminit~

Initialises the main memory which is used within the main memory algebra.
The default value is 256MB.
The maximum value is limited by the value set in ~SecondoConfig.ini~.
If the wanted value is smaller then the memory that is already in use,
the value will be set to the smallest possible value
without deleting any main memory objects.

*/

/*
5.2.1 Type Mapping Functions of operator ~meminit~ (int->int)

*/

ListExpr meminitTypeMap(ListExpr args)
{
string err = "int expected";
  if(nl->ListLength(args)!=1){
     return listutils::typeError(err + " (wrong number of arguments)");
  }
  if (!CcInt::checkType(nl->First(args))) {
  return listutils::typeError(err);
  }
  return listutils::basicSymbol<CcInt>();
}


/*

5.2.3  The Value Mapping Functions of operator ~meminit~

*/

int meminitValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

    int maxSystemMainMemory = qp->GetMemorySize(s);
    int newMainMemorySize = ((CcInt*)args[0].addr)->GetIntval();
    int res=0;

    if (newMainMemorySize<0){
    cout<< "the size must be >0"<<endl;
    res=catalog->getMemSizeTotal();
    }
    else if ((size_t)newMainMemorySize<catalog->getUsedMemSize()/1024/1024){
            res = catalog->getUsedMemSize()/1024/1024;
            catalog->setMemSizeTotal(catalog->getUsedMemSize()/1024/1024);
        }
    else if (newMainMemorySize>maxSystemMainMemory){
            res = maxSystemMainMemory;
            catalog->setMemSizeTotal(maxSystemMainMemory);
    }
    else {
        res = newMainMemorySize;
        catalog->setMemSizeTotal(newMainMemorySize);
    }

    result  = qp->ResultStorage(s);
    CcInt* b = static_cast<CcInt*>(result.addr);
    b->Set(true, res);


    return 0;
}

/*

5.2.4 Description of operator ~meminit~

*/

OperatorSpec meminitSpec(
    "int -> int",
    "meminit(_)",
    "initialises the main memory, the maximum size "
    " is limited by the global memory, set in SecondoConfig.ini",
    "query meminit(256)"
);

/*

5.2.5 Instance of operator ~meminit~

*/

Operator meminitOp (
    "meminit",
    meminitSpec.getStr(),
    meminitValMap,
    Operator::SimpleSelect,
    meminitTypeMap
);


/*
5.3 Operator ~mfeed~

~mfeed~ produces a stream of tuples from a main memory relation,
similar to the ~feed~-operator

*/

/*
5.3.1 Type Mapping Functions of operator ~mfeed~ (string -> stream(tuple))

*/

ListExpr mfeedTypeMap(ListExpr args) {

    if(nl->ListLength(args)!=1){
        return listutils::typeError("wrong number of arguments");
    }

    ListExpr arg = nl->First(args);

    if(!nl->HasLength(arg,2)){
        return listutils::typeError("internal error");
    }

    if (!CcString::checkType(nl->First(arg))) {
        return listutils::typeError("string expected");
    };

    ListExpr fn = nl->Second(arg);


    if(nl->AtomType(fn)!=StringType){
        return listutils::typeError("error");
    }

    string oN = nl->StringValue(fn);
    ListExpr oTeList = catalog->getMMObjectTypeExpr(oN);

    if(!catalog->isMMObject(oN) ||
            !listutils::isTupleDescription(oTeList))
    {
      return listutils::typeError("not a MainMemory member or not a relation");
    }

    return nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),oTeList);
}


/*
s
5.3.3  The Value Mapping Functions of operator ~mfeed~

*/


class mfeedInfo{
  public:
     mfeedInfo(vector<Tuple*>* _relation):relation(_relation){
          it = relation->begin();
     }

    ~mfeedInfo(){}

     Tuple* next(){
       if(it==relation->end()) return 0;
       Tuple* res = *it;
       it++;
       res->IncReference();
       return res;
     }

  private:
     vector<Tuple*>* relation;
     vector<Tuple*>::iterator it;

};


int mfeedValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {


   mfeedInfo* li = (mfeedInfo*) local.addr;

   switch (message)
   {
        case OPEN: {
             if(li){
             delete li;
             local.addr=0;
          }
          CcString* oN = (CcString*) args[0].addr;
          if(!oN->IsDefined()){
             return 0;
          }
          string objectName = oN->GetValue();
          vector<Tuple*>* relation;

          //the type mapping assures that it is a main memory member
          MemoryRelObject* mro =
                    (MemoryRelObject*)catalog->getMMObject(objectName);
          relation = mro->getmmrel();

          local.addr= new mfeedInfo(relation);
          return 0;
        }

        case REQUEST:
            result.addr=(li?li->next():0);
            return result.addr?YIELD:CANCEL;


        case CLOSE:
            if(li)
            {
            delete li;
            local.addr = 0;
            }
            return 0;
   }

    return 0;
}




/*

5.3.4 Description of operator ~mfeed~


*/



OperatorSpec mfeedSpec(
    "string -> stream(tuple)",
    "_ mfeed",
    "produces a stream from a main memory relation",
    "query 'ten' mfeed"
);

/*

5.3.5 Instance of operator ~mfeed~

*/

Operator mfeedOp (
    "mfeed",
    mfeedSpec.getStr(),
    mfeedValMap,
    Operator::SimpleSelect,
    mfeedTypeMap
);




/*

5.4 Operator ~letmconsume~

~letmconsume~ produces a main memory relation from a stream(tuples),
similar to the ~consume~-operator. The name of the main memory relation is given
by the second parameter.

*/

/*

5.4.1 Type Mapping Functions of operator ~letmconsume~
        (stream(tuple) x string -> string)

*/
ListExpr letmconsumeTypeMap(ListExpr args)
{
    if(nl->ListLength(args)!=2){
        return listutils::typeError("(wrong number of arguments)");
    }

    if (!Stream<Tuple>::checkType(nl->First(args))
        || !CcString::checkType(nl->Second(args)) ) {
        return listutils::typeError ("stream(Tuple) x string expected!");
        }

    return listutils::basicSymbol<CcString>();
}


/*

5.4.3  The Value Mapping Functions of operator ~letmconsume~

*/

int letmconsumeValMap (Word* args, Word& result,
                int message, Word& local, Supplier s) {

    result  = qp->ResultStorage(s);
    CcString* str = static_cast<CcString*>(result.addr);

    Supplier t = qp->GetSon( s, 0 );
    ListExpr le = qp->GetType(t);

    MemoryRelObject* mem = tupelStreamToRel(args[0],nl->Second(le));

    CcString* oN = (CcString*) args[1].addr;
    if(!oN->IsDefined()){
                return 0;
          }
    string res = oN->GetValue();
    //im TypeMapping abfangen falls identifier already in use???
    if(catalog->insert(res,mem)){
     str->Set(true,res);
    }
    else{
    str->Set(false,res);
    delete mem;
    }

   return 0;
}




/*

5.4.4 Description of operator ~letmconsume~

*/

OperatorSpec letmconsumeSpec(
    "stream(tuple) x string -> string",
    "(_) letmconsume [_]",
    "produces a main memory relation from a stream(tuple)",
    "query ten feed letmconsume ['zehn']"
);



/*

5.4.5 Instance of operator ~letmconsume~

*/

Operator letmconsumeOp (
    "letmconsume",
    letmconsumeSpec.getStr(),
    letmconsumeValMap,
    Operator::SimpleSelect,
    letmconsumeTypeMap
);





/*
5.6 Operator ~memdelete~

~memdelete~ deletes an object from main memory

*/

/*
5.6.1 Type Mapping Functions of operator ~memdelete~ (string -> bool)

*/
ListExpr memdeleteTypeMap(ListExpr args)
{
   return tmStringBool(args);
}


/*

5.6.3  The Value Mapping Functions of operator ~memdelete~

*/


int memdeleteValMap (Word* args, Word& result,
                int message, Word& local, Supplier s) {
    result  = qp->ResultStorage(s);
    CcBool* b = static_cast<CcBool*>(result.addr);
    bool deletesucceed = false;
    CcString* oN = (CcString*) args[0].addr;
    if(!oN->IsDefined()){
            b->Set(true, deletesucceed);
            return 0;
        }
    string objectName = oN->GetValue();
    deletesucceed = catalog->deleteObject(objectName);

    b->Set(true, deletesucceed);
    return 0;

}


/*

5.6.4 Description of operator ~memdelete~

Similar to the ~property~ function of a type constructor, an operator needs to
be described, e.g. for the ~list operators~ command.  This is now done by
creating a subclass of class ~OperatorInfo~.

*/

OperatorSpec memdeleteSpec(
    "string -> bool",
    "memdelete (_)",
    "deletes a main memory object",
    "query memdelete ('ten')"
);



/*

5.6.5 Instance of operator ~memdelete~

*/

Operator memdeleteOp (
    "memdelete",
    memdeleteSpec.getStr(),
    memdeleteValMap,
    Operator::SimpleSelect,
    memdeleteTypeMap
);


/*
5.7 Operator ~memobject~

~memobject~ gets a name of a main memory object and return a persistent version

*/

/*
5.7.1 Type Mapping Functions of operator ~memobject~ (string -> m:MEMLOADABLE)

*/
ListExpr memobjectTypeMap(ListExpr args) {

    if(nl->ListLength(args)!=1){
        return listutils::typeError("wrong number of arguments");
    }
     ListExpr arg1 = nl->First(args);

    if(!nl->HasLength(arg1,2)){
        return listutils::typeError("internal error");
    }

    if (!CcString::checkType(nl->First(arg1))) {
        return listutils::typeError("string expected as first argument");
    };


    ListExpr str = nl->Second(arg1);

    if(nl->AtomType(str)!=StringType){
            return listutils::typeError("error");
    }

    string oN = nl->StringValue(str);


    if(catalog->isMMObject(oN)){

        ListExpr typeExpr = catalog->getMMObjectTypeExpr(oN);

        if(listutils::isTupleDescription(typeExpr)){
        ListExpr result =
            nl->TwoElemList(nl->SymbolAtom(Relation::BasicType()), typeExpr);
        return result;
        }

        if(listutils::isDATA(typeExpr)) {
          return typeExpr;
        }
    }
return listutils::typeError("string does not belong to a main memory member");
}


/*

5.7.3  The Value Mapping Functions of operator ~memobject~

*/



int memobjectValMap (Word* args, Word& result,
                int message, Word& local, Supplier s) {

        CcString* oN = (CcString*) args[0].addr;
        if(!oN->IsDefined()){
            return 0;
        }
        string objectName = oN->GetValue();
        ListExpr typeExpr = catalog->getMMObjectTypeExpr(objectName);


        if (listutils::isTupleDescription(typeExpr)) {
            MemoryRelObject* memObject =
                    (MemoryRelObject*)catalog->getMMObject(objectName);
            GenericRelation* rel =
                    (GenericRelation*)((qp->ResultStorage(s)).addr);
            if(rel->GetNoTuples() > 0) {
                rel->Clear();
            }

            vector<Tuple*>* relation;
            relation = memObject->getmmrel();
            vector<Tuple*>::iterator it;
            it=relation->begin();


            while( it!=relation->end()){
                Tuple* tup = *it;
                rel->AppendTuple(tup);
                //tup->IncReference();
                //tup->DeleteIfAllowed();
                it++;
            }

            result.setAddr(rel);

            return 0;

        }

        if (listutils::isDATA(typeExpr)) {

            MemoryAttributeObject* memObject =
                    (MemoryAttributeObject*)catalog->getMMObject(objectName);
            Attribute* attr = (Attribute*)((qp->ResultStorage(s)).addr);
            attr = memObject->getAttributeObject();
            result.setAddr(attr);
        return 0;
        }
    return 0;
}




/*

5.7.4 Description of operator ~memobject~

*/



OperatorSpec memobjectSpec(
    "string -> m:MEMLOADABLE",
    "memobject (_)",
    "returns a persistent Object created from a main memory Object",
    "query memobject ('Trains100')"
);


/*

5.7.5 Instance of operator ~memobject~

*/

Operator memobjectOp (
    "memobject",
    memobjectSpec.getStr(),
    memobjectValMap,
    Operator::SimpleSelect,
    memobjectTypeMap
);

/*

5.8 Operator ~memgetcatalog~

Returns a ~stream(tuple)~.
Each tuple describes one element of the main memory catalog.


5.8.1 Type Mapping Functions of operator ~memgetcatalog~ (  -> stream(tuple) )

*/


ListExpr memgetcatalogTypeMap(ListExpr args)
{

     if(nl->ListLength(args)!=0){
        return listutils::typeError("no argument expected");
    }

    string stringlist = "(stream(tuple((TotalMB int)"
        "(UsedMB real)(Name string)"
        "(ObjectType string)(ObjSizeInB int)(ObjSizeInMB real))))";

    ListExpr res =0;
    if(nl->ReadFromString(stringlist, res)){};
    return res;

}


/*

5.8.3  The Value Mapping Functions of operator ~memgetcatalog~

*/

class memgetcatalogInfo{
  public:

       memgetcatalogInfo(ListExpr _resultType){
       resultType = _resultType;
       memContents = catalog->getMemContent();
       it = memContents->begin();
       };
       ~memgetcatalogInfo(){}


    Tuple* next(){
        if(it==memContents->end()) {
            return 0;
        }
        string name = it->first;
        MemoryObject* memobj = it->second;
        string objTyp ="nn";

        ListExpr objectType = catalog->getMMObjectTypeExpr(name);
        if (listutils::isTupleDescription(objectType)){
            objTyp = MemoryRelObject::BasicType();
        }
        if (listutils::isDATA(objectType)){
            objTyp = MemoryAttributeObject::BasicType();
        }

        if (listutils::stringValue(objectType)=="mmrtree"){

                objTyp = MemoryRtreeObject::BasicType();
        }
        TupleType* tt = new TupleType(nl->Second(resultType));
        Tuple *tup = new Tuple( tt );
        tt->DeleteIfAllowed();

        CcInt* totalMB = new CcInt (true, catalog->getMemSizeTotal());
        CcReal* usedMB =
            new CcReal (true, (double)catalog->getUsedMemSize()/1024.0/1024.0);
        CcString* objectName = new CcString(true,name);
        CcString* oT = new CcString(true,objTyp);
        CcInt* memSizeB = new CcInt(true, (int)memobj->getMemSize());
        CcReal* memSizeMB =
            new CcReal(true, (double)memobj->getMemSize()/1024.0/1024.0);

        tup->PutAttribute(0,totalMB);
        tup->PutAttribute(1,usedMB);
        tup->PutAttribute(2,objectName);
        tup->PutAttribute(3,oT);
        tup->PutAttribute(4,memSizeB);
        tup->PutAttribute(5,memSizeMB);

        it++;
        return tup;
    }
 private:

       map<string, MemoryObject*>* memContents;
       map<string, MemoryObject*>::iterator it;
       ListExpr resultType;

};


int memgetcatalogValMap (Word* args, Word& result,
            int message, Word& local, Supplier s) {


   memgetcatalogInfo* li = (memgetcatalogInfo*) local.addr;

   switch (message)
   {
        case OPEN: {
             if(li){
             delete li;
             local.addr=0;

          }
        ListExpr resultType;
        resultType = GetTupleResultType( s );
        local.addr= new memgetcatalogInfo(resultType);
        return 0;
        }

        case REQUEST:
            result.addr=(li?li->next():0);
            return result.addr?YIELD:CANCEL;


        case CLOSE:
            if(li)
            {
            delete li;
            local.addr = 0;
            }
            return 0;
   }

    return 0;

}



/*

5.8.4 Description of operator ~memgetcatalog~

*/

OperatorSpec memgetcatalogSpec(
    " -> stream(tuple)",
    "memgetcatalog()",
    "returns a stream(tuple) with information of main memory objects",
    "query memgetcatalog()"
);

/*

5.8.5 Instance of operator ~memgetcatalog~

*/

Operator memgetcatalogOp (
    "memgetcatalog",
    memgetcatalogSpec.getStr(),
    memgetcatalogValMap,
    Operator::SimpleSelect,
    memgetcatalogTypeMap
);


/*
5.9 Operator ~memlet~
creates a new main memory object. The first parameter is the
name of the new main memory object, the second is the query/the
MEMLOADABLE object from which the mm-object will be created.

*/

/*
5.9.1 Type Mapping Functions of operator ~memlet~

        (string X m:MEMLOADABLE -> bool)

*/
ListExpr memletTypeMap(ListExpr args)
{
    return tmStringMemloadBool(args);
}


/*

5.9.3  The Value Mapping Functions of operator ~memlet~

*/



int memletValMap (Word* args, Word& result,
                int message, Word& local, Supplier s) {

    result  = qp->ResultStorage(s);
    CcBool* b = static_cast<CcBool*>(result.addr);
    bool memletsucceed = false;

    CcString* oN = (CcString*) args[0].addr;
    if(!oN->IsDefined()){
        return 0;
    }
    string objectName = oN->GetValue();

    if (catalog->isMMObject(objectName)){
        cout<< "identifier already in use"<<endl;
        b->Set(true, memletsucceed);
        return 0;
    }

    Supplier t = qp->GetSon( s, 1 );
    ListExpr le = qp->GetType(t);

    if (listutils::isRelDescription(le)){

        GenericRelation* rel= static_cast<Relation*>( args[1].addr );
        MemoryRelObject* mmRelObject = relToVector(rel,nl->Second(le));

        catalog->insert(objectName, mmRelObject);
        memletsucceed =true;
    }

    if (listutils::isDATA(le)){

        Attribute* attr = (Attribute*)args[1].addr;
        MemoryAttributeObject* mmA = attrToMM(attr, le);

        catalog->insert(objectName,mmA);
        memletsucceed = true;
    }

    if (listutils::isTupleStream(le)){

        MemoryRelObject* mem = tupelStreamToRel(args[1], nl->Second(le));

        catalog->insert(objectName,mem);
        memletsucceed = true;
    }

    b->Set(true, memletsucceed);

    return 0;

}


/*

5.9.4 Description of operator ~memlet~

*/



OperatorSpec memletSpec(
    "string x m:MEMLOADABLE -> bool",
    "memlet (_,_)",
    "creates a main memory object from a given MEMLOADABLE",
    "query memlet ('Trains100', Trains feed head[100])"
);



/*

5.9.5 Instance of operator ~memlet~

*/

Operator memletOp (
    "memlet",
    memletSpec.getStr(),
    memletValMap,
    Operator::SimpleSelect,
    memletTypeMap
);


/*
5.10 Operator ~memupdate~
updates a main memory object. The tuple description for a stream or a relation
must be the same as the one of the main memory object.


*/

/*
5.10.1 Type Mapping Functions of operator ~memupdate~
        (string x m:MEMLOADABLE -> bool)

*/
ListExpr memupdateTypeMap(ListExpr args)
{
    return tmStringMemloadBool(args);
}


/*

5.10.3  The Value Mapping Functions of operator ~memupdate~

*/

int memupdateValMap (Word* args, Word& result,
                int message, Word& local, Supplier s) {

    result  = qp->ResultStorage(s);
    CcBool* b = static_cast<CcBool*>(result.addr);
    bool memupdatesucceed = false;

    CcString* oN = (CcString*) args[0].addr;
    if(!oN->IsDefined()){
        return 0;
    }
    string objectName = oN->GetValue();

    if (!catalog->isMMObject(objectName)){
        cout<< "there is no main memory object: "<<objectName<<endl;
        b->Set(true, memupdatesucceed);
        return 0;
    }

    ListExpr memType = catalog->getMMObjectTypeExpr(objectName);

    Supplier t = qp->GetSon( s, 1 );
    ListExpr le = qp->GetType(t);

    //the object shall be updated by a relation, the object itself is
    //a MemoryRelObject and the tuple descriptions are the same
    if (listutils::isRelDescription(le) &&
                listutils::isTupleDescription(memType) &&
                nl->Equal(nl->Second(le), memType)){

        catalog->deleteObject(objectName);

        GenericRelation* rel= static_cast<Relation*>( args[1].addr );
        MemoryRelObject* mem = relToVector(rel,nl->Second(le));

        catalog->insert(objectName,mem);
        memupdatesucceed =true;
    }


    if (listutils::isDATA(le) && listutils::isDATA(memType) &&
        nl->Equal(le,memType)){

        catalog->deleteObject(objectName);

        Attribute* attr = (Attribute*)args[1].addr;
        MemoryAttributeObject* mem = attrToMM(attr, le);
        if (mem!=0){
            catalog->insert(objectName,mem);
            memupdatesucceed = true;
        }

        }


    if (listutils::isTupleStream(le) && listutils::isTupleDescription(memType)
                && nl->Equal(nl->Second(le), memType)){

        catalog->deleteObject(objectName);
        MemoryRelObject* mem = tupelStreamToRel(args[1], nl->Second(le));

        catalog->insert(objectName,mem);
        memupdatesucceed = true;
    }

    b->Set(true, memupdatesucceed);

    return 0;

}




/*

5.10.4 Description of operator ~memupdate~

*/

OperatorSpec memupdateSpec(
    "string x m:MEMLOADABLE -> bool",
    "memupdate (_,_)",
    "updates a main memory object with a given MEMLOADABLE",
    "query memupdate ('fuenf', ten feed head[7])"
);

/*

5.10.5 Instance of operator ~memupdate~

*/

Operator memupdateOp (
    "memupdate",
    memupdateSpec.getStr(),
    memupdateValMap,
    Operator::SimpleSelect,
    memupdateTypeMap
);


/*
5.10 Operator ~mcreateRtree~
creates a an mmRTree over a given main memory relation

*/

/*
5.10.1 Type Mapping Functions of operator ~mcreateRtree~
        (string x string -> string)

        the first parameter identifies the main memory relation, the
        second parameter identifies the attribute

*/

ListExpr mcreateRtreeTypeMap(ListExpr args){

    return tmStringStringString(args);
}

/*

5.10.3  The Value Mapping Functions of operator ~mcreateRtree~

*/



int mcreateRtreeValMap (Word* args, Word& result,
                int message, Word& local, Supplier s) {

    string res ="undefined";

    result  = qp->ResultStorage(s);
    CcString* str = static_cast<CcString*>(result.addr);
    str->Set(false, res);

    CcString* roN = (CcString*) args[0].addr;
    if(!roN->IsDefined()){
        return 0;
    }
    string relObjectName = roN->GetValue();
    ListExpr memType = catalog->getMMObjectTypeExpr(relObjectName);

    if (!catalog->isMMObject(relObjectName)||
            !listutils::isTupleDescription(memType)){
        cout<< "there is no main memory object: "<<relObjectName<<
        " or is not a MemoryRelObject"<<endl;
        return 0;
    }

    CcString* aN = (CcString*) args[1].addr;
    if (!aN->IsDefined()){
        return 0;
    }

    string attrName = aN->GetValue();
    ListExpr attrType = 0;
    int attrPos = 0;
    ListExpr attrList = nl->Second(memType);

    if (listutils::isAttrList(attrList)){
        attrPos = listutils::findAttribute(attrList, attrName, attrType);
    }

    if (attrPos == 0){
    cout<<"there is no attribute with the name: "<<attrName<< endl;
    return 0;
    }

    // check for spatial attribute
//  listutils::isSpatialType(attrType)|| listutils::isRectangle(attrType)
    if (!listutils::isKind(attrType,Kind::SPATIAL2D()) &&
        !listutils::isKind(attrType,Kind::SPATIAL3D()) &&
        !listutils::isKind(attrType,Kind::SPATIAL4D()) &&
        !listutils::isKind(attrType,Kind::SPATIAL8D())){
        cout<<"Expects key attribute to be of kind SPATIAL,"
        "SPATIAL3D, SPATIAL4D, SPATIAL8D"<<endl;
        return 0;
        }

    MemoryRelObject* mmrel =
        (MemoryRelObject*)catalog->getMMObject(relObjectName);
    vector<Tuple*>* relVec = mmrel->getmmrel();
    vector<Tuple*>::iterator it;
    it=relVec->begin();
    unsigned int i=0;

    if(listutils::isKind(attrType,Kind::SPATIAL2D())){
        mmrtree::RtreeT<2, size_t>* rtree =
                    new mmrtree::RtreeT<2, size_t>(4,8);
        StandardSpatialAttribute<2>* attr=0;
        size_t usedMainMemory=0;
        while( it!=relVec->end()){
            Tuple* tup = *it;
            attr=(StandardSpatialAttribute<2>*)tup->GetAttribute(attrPos-1);
            if (attr==0 || !attr->IsDefined()){
                return 0;
            }
            Rectangle<2> box = attr->BoundingBox();
            rtree->insert(box, i);
            it++;
            i++;
          //  tup->DeleteIfAllowed();
            } // end while

        usedMainMemory = rtree->usedMem();

        MemoryRtreeObject* mmRtreeObject =
            new MemoryRtreeObject(rtree, usedMainMemory,"memoryRtreeObject");

        res = relObjectName +"_"+attrName;
        catalog->insert(res,mmRtreeObject);

      } //end if spatial2D


    str->Set(true, res);
    return 0;
    } //end mcreateRtreeValMap





/*

5.10.4 Description of operator ~mcreateRtree~

*/

OperatorSpec mcreateRtreeSpec(
    "string x string -> string",
    "mcreateRtree (_,_)",
    "creates an mmrtree over a main memory relation given by the"
    "first string and an attribute given by the second string",
    "query mcreateRtree ('WFlaechen', 'GeoData')"
);



/*

5.10.5 Instance of operator ~mcreateRtree~

*/

Operator mcreateRtreeOp (
    "mcreateRtree",
    mcreateRtreeSpec.getStr(),
    mcreateRtreeValMap,
    Operator::SimpleSelect,
    mcreateRtreeTypeMap
);


/*
5.11 Operator ~memsize~

returns the currently set main memory size

*/

/*
5.11.1 Type Mapping Functions of operator ~memsize~ (->int)

*/

ListExpr memsizeTypeMap(ListExpr args)
{

  if(nl->ListLength(args)!=0){
     return listutils::typeError("no argument expected");
  }

  return listutils::basicSymbol<CcInt>();
}


/*

5.11.3  The Value Mapping Functions of operator ~memsize~

*/

int memsizeValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

    int res = catalog->getMemSizeTotal();

    result  = qp->ResultStorage(s);
    CcInt* b = static_cast<CcInt*>(result.addr);
    b->Set(true, res);


    return 0;
}

/*

5.11.4 Description of operator ~memsize~

*/

OperatorSpec memsizeSpec(
    "-> int",
    "memsize()",
    "returns the currently set main memory size ",
    "query memsize()"
);

/*

5.11.5 Instance of operator ~memsize~

*/

Operator memsizeOp (
    "memsize",
    memsizeSpec.getStr(),
    memsizeValMap,
    Operator::SimpleSelect,
    memsizeTypeMap
);



/*
5.12 Operator ~memclear~

deletes all main memory objects

*/

/*
5.12.1 Type Mapping Functions of operator ~memclear~ (-> bool)

*/

ListExpr memclearTypeMap(ListExpr args)
{

  if(nl->ListLength(args)!=0){
     return listutils::typeError("no argument expected");
  }

  return listutils::basicSymbol<CcBool>();
}


/*

5.12.3  The Value Mapping Functions of operator ~memclear~

*/

int memclearValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

    bool res = false;
    catalog->clear();
    res = true;
    result  = qp->ResultStorage(s);
    CcBool* b = static_cast<CcBool*>(result.addr);
    b->Set(true, res);


    return 0;
}

/*

5.12.4 Description of operator ~memclear~

*/

OperatorSpec memclearSpec(
    "-> bool",
    "memclear()",
    "deletes all main memory objects",
    "query memclear()"
);

/*

5.12.5 Instance of operator ~memclear~

*/

Operator memclearOp (
    "memclear",
    memclearSpec.getStr(),
    memclearValMap,
    Operator::SimpleSelect,
    memclearTypeMap
);


/*
5.13 Operator ~minsert~

inserts the tuple of a stream into a existing main memory relation

*/

/*
5.13.1 Type Mapping Functions of operator ~minsert~
    (stream(tuple(x)) x string -> stream(tuple(x))
    the second argument identifies the main memory relation

*/

ListExpr minsertTypeMap(ListExpr args)
{
    if(nl->ListLength(args)!=2){
        return listutils::typeError("two arguments expected");
    }

    ListExpr argFir = nl->First(args); //stream + query
    ListExpr stream = nl->First(argFir);

    ListExpr argSec = nl->Second(args); //string + query

    if (!Stream<Tuple>::checkType(stream)) {
        return listutils::typeError
            ("stream(Tuple) as first argument expected");
    }

    if(!nl->HasLength(argSec,2)){
        return listutils::typeError("internal error");
    }

    if (!CcString::checkType(nl->First(argSec))) {
        return listutils::typeError("string expected");
    };

    ListExpr fn = nl->Second(argSec);

    if(nl->AtomType(fn)!=StringType){
        return listutils::typeError("error");
    }

    string oN = nl->StringValue(fn);
    ListExpr oTeList = catalog->getMMObjectTypeExpr(oN);

    if(!catalog->isMMObject(oN) ||
            !listutils::isTupleDescription(oTeList))
    {
      return listutils::typeError("not a MainMemory member or not a relation");
    }
    cout <<"tupel?"<<nl->ToString(nl->Second(stream))<<endl;
    if (!nl->Equal(nl->Second(stream), oTeList))
    {
      return listutils::typeError("the stream tuple description "
                "does not match the main memory relation ");
    }
  return nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),oTeList);

}


class minsertInfo{
  public:
     minsertInfo(vector<Tuple*>* _relation):relation(_relation){
          it = relation->begin();
     }

    ~minsertInfo(){}

     Tuple* next(){
       if(it==relation->end()) return 0;
       Tuple* res = *it;
       it++;
       res->IncReference();
       return res;
     }

  private:
     vector<Tuple*>* relation;
     vector<Tuple*>::iterator it;

};


/*

5.13.3  The Value Mapping Functions of operator ~minsert~

*/

int minsertValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

    minsertInfo* li = (minsertInfo*) local.addr;

    switch (message)
    {
        case OPEN: {
            if(li){
            delete li;
            local.addr=0;
            }
            CcString* oN = (CcString*) args[1].addr;
            if(!oN->IsDefined()){
                return 0;
            }
            string objectName = oN->GetValue();
            vector<Tuple*>* relation;
            MemoryRelObject* mro =
                    (MemoryRelObject*)catalog->getMMObject(objectName);

            Stream<Tuple> stream(args[0]);
            Tuple* tup;
            stream.open();
            while( (tup = stream.request()) != 0){
                mro->addTuple(tup);
            }
        stream.close();
        relation = mro->getmmrel();
        local.addr= new minsertInfo(relation);
        return 0;
        }

        case REQUEST:
            result.addr=(li?li->next():0);
            return result.addr?YIELD:CANCEL;

        case CLOSE:
            if(li)
            {
            delete li;
            local.addr = 0;
            }
            return 0;
   }

   return 0;
}

/*

5.13.4 Description of operator ~minsert~

*/

OperatorSpec minsertSpec(
    "stream(tuple) x string -> stream(tuple)",
    "minsert(_,_)",
    "inserts the tuple of a stream into a "
    "existing main memory relation",
    "query minsert (ten feed head[5],'ten') count"
);

/*

5.13.5 Instance of operator ~minsert~

*/

Operator minsertOp (
    "minsert",
    minsertSpec.getStr(),
    minsertValMap,
    Operator::SimpleSelect,
    minsertTypeMap
);

/*
5.14 Operator ~mwindowintersects~
        Uses the given MemoryRtreeObject (as first argument)to find all tuples
        in the given MemoryRelObject (as second argument)
        with intersects the third argument value's bounding box.


*/

/*
5.14.1 Type Mapping Functions of operator ~mwindowintersects~
    string x string x T -> stream(tuple)
    where T in {rect<d>} U SPATIAL2D U SPATIAL3D U SPATIAL4D U SPATIAL8D

*/

ListExpr mwindowintersectsTypeMap(ListExpr args)
{
    cout<<"ARGS: "<<nl->ToString(args)<<endl;
    if(nl->ListLength(args)!=3){
     return listutils::typeError("three arguments expected");
    }

      /* Split argument in three parts */
  ListExpr memoryRtreeDescription = nl->First(args);
  ListExpr memoryRelDescription = nl->Second(args);
  ListExpr searchWindow = nl->Third(args);

  // first must be an rtree<dim>
//  if(!listutils::isRTreeDescription(rtreeDescription)){
//    return listutils::typeError("Expects 1st argument to be of type "
//                                "'rtree<dim>(<tuple-type>,bool)'.");
//  }

    if (!CcString::checkType(nl->First(memoryRtreeDescription))) {
        return listutils::typeError("string as first argument expected");
    }
    string oN_Rtree = nl->StringValue(nl->Second(memoryRtreeDescription));
    ListExpr oTE_Rtree = catalog->getMMObjectTypeExpr(oN_Rtree);

    if (!catalog->isMMObject(oN_Rtree) ||
            !MemoryRtreeObject::checkType(oTE_Rtree)){
        return listutils::typeError
                ("first string does not identify a MemoryRTreeObject");
    }

  // second a relation
   if (!CcString::checkType(nl->First(memoryRelDescription))) {
        return listutils::typeError("string as second argument expected");
    }
   string oN_Rel = nl->StringValue(nl->Second(memoryRelDescription));
    ListExpr oTE_Rel = catalog->getMMObjectTypeExpr(oN_Rel);
    if (!catalog->isMMObject(oN_Rel) ||
                !listutils::isTupleDescription(oTE_Rel)){
        return listutils::typeError
                ("second string does not identify a MemoryRelObject");
    }


  // third a type with an MBR
  if(!(    listutils::isSpatialType(nl->First(searchWindow))
        || listutils::isRectangle(nl->First(searchWindow)))){
    return listutils::typeError("expects 3nd argument to be of a type of kind "
      "SPATIAL, SPATIAL3D, SPATIAL4D, SPATIAL8D; or of type 'rect', 'rect3', "
      "'rect4', or 'rect8'.");
  }

return nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),oTE_Rel);
}


class mwiInfo{
  public:
     mwiInfo(mmrtree::RtreeT<2, size_t>* _rtree,
                vector<Tuple*>* _relation, Rectangle<2> _searchBox)
     :relation(_relation),rtree(_rtree),searchBox(_searchBox)
     {
        rtree->findAll(searchBox,res);
        it = res.begin();
     }

    ~mwiInfo(){
        res.clear();
    }


     Tuple* next(){
     if(it==res.end()) return 0;
     Tuple* result = relation->at(*it);
     result->IncReference();
     it++;
     return result;

     }

  private:
     vector<Tuple*>* relation;
     mmrtree::RtreeT<2, size_t>* rtree;
     Rectangle<2> searchBox;
     set<size_t> res;
     set<size_t>::iterator it;
};

/*

5.14.3  The Value Mapping Functions of operator ~mwindowintersects~

*/

int mwindowintersectsValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

   mwiInfo* li = (mwiInfo*) local.addr;

   switch (message)
   {
        case OPEN: {
             if(li){
             delete li;
             local.addr=0;
          }
          //first argument MemoryRtreeObject
            CcString* oN_0 = (CcString*) args[0].addr;
          if(!oN_0->IsDefined()){
             return 0;
          }
          string objectName_0 = oN_0->GetValue();
          mmrtree::RtreeT<2, size_t>* rtree;

          MemoryRtreeObject* mrTreeO =
                    (MemoryRtreeObject*)catalog->getMMObject(objectName_0);
          rtree = mrTreeO->getrtree();

          //second argument MemoryRelObject
          CcString* oN_1 = (CcString*) args[1].addr;
          if(!oN_1->IsDefined()){
             return 0;
          }
          string objectName_1 = oN_1->GetValue();
          vector<Tuple*>* relation;

          MemoryRelObject* mro =
                    (MemoryRelObject*)catalog->getMMObject(objectName_1);
          relation = mro->getmmrel();

          //third argument Searchwindow

          StandardSpatialAttribute<2>* attr =
                    (StandardSpatialAttribute<2>*)args[2].addr;
          Rectangle<2> box = attr->BoundingBox();
          local.addr= new mwiInfo(rtree,relation,box);
          return 0;
        }

        case REQUEST:
            result.addr=(li?li->next():0);
            return result.addr?YIELD:CANCEL;


        case CLOSE:
            if(li)
            {
            delete li;
            local.addr = 0;

            }
            return 0;
   }

    return 0;
}

/*

5.14.4 Description of operator ~mwindowintersects~

*/

OperatorSpec mwindowintersectsSpec(
    "string x string x T -> stream(tuple) "
    "where T in {rect<d>} U SPATIAL2D U SPATIAL3D U SPATIAL4D U SPATIAL8D",
    "mwindowintersects(_,_,_)",
    "Uses the given rtree to find all tuples"
      " in the given relation which intersects the "
      " argument value's bounding box.",
    "query mwindowintersects"
    "('strassen_GeoData', 'strassen', bbox(thecenter)) count"
);

/*

5.14.5 Instance of operator ~mwindowintersects~

*/

Operator mwindowintersectsOp (
    "mwindowintersects",
    mwindowintersectsSpec.getStr(),
    mwindowintersectsValMap,
    Operator::SimpleSelect,
    mwindowintersectsTypeMap
);


/*

5.15 Operator ~mconsume~

~mconsume~ Collects objects from a stream in a ~MemoryRelObject~

*/

/*

5.4.1 Type Mapping Functions of operator ~mconsume~
        (stream(tuple) -> memoryRelObject)

*/
ListExpr mconsumeTypeMap(ListExpr args)
{
    if(nl->ListLength(args)!=1){
        return listutils::typeError("(wrong number of arguments)");
    }

    if (!Stream<Tuple>::checkType(nl->First(args))) {
        return listutils::typeError ("stream(Tuple) expected!");
        }

    ListExpr l1 = nl->Second(nl->First(args));
    ListExpr l2 = nl->SymbolAtom(MemoryRelObject::BasicType());

    return nl->TwoElemList (l2,l1);;
}


/*

5.15.3  The Value Mapping Functions of operator ~mconsume~

*/

int mconsumeValMap (Word* args, Word& result,
                int message, Word& local, Supplier s) {

    Supplier t = qp->GetSon( s, 0 );
    ListExpr le = qp->GetType(t);
    MemoryRelObject* mrel = new MemoryRelObject(nl->ToString(nl->Second(le)));
    Stream<Tuple> stream(args[0]);
    stream.open();
    Tuple* tup=0;
    while( (tup = stream.request()) != 0){
        mrel->addTuple(tup);
        tup->DeleteIfAllowed();
    }
    stream.close();
    result  = qp->ResultStorage(s);
    result.setAddr(mrel);
    return 0;
}




/*

5.15.4 Description of operator ~mconsume~

*/

OperatorSpec mconsumeSpec(
    "stream(tuple) -> memoryrelobject",
    "_ mconsume",
    "collects the objects from a stream(tuple)",
    "query 'ten' mfeed mconsume"
);



/*

5.15.5 Instance of operator ~mconsume~

*/

Operator mconsumeOp (
    "mconsume",
    mconsumeSpec.getStr(),
    mconsumeValMap,
    Operator::SimpleSelect,
    mconsumeTypeMap
);



/*
5.16 Operator ~mcreateAVLtree~
creates a an AVLTree over a given main memory relation

*/

/*
5.16.1 Type Mapping Functions of operator ~mcreateAVLtree~
        (string x string -> string)

        the first parameter identifies the main memory relation, the
        second parameter identifies the attribute

*/

ListExpr mcreateAVLtreeTypeMap(ListExpr args){

    if(nl->ListLength(args)!=2){
     return listutils::typeError("two arguments expected");
    }

// Split argument in two parts
    ListExpr memoryRelDescription = nl->First(args);
    ListExpr attributeDescription = nl->Second(args);

// first must be a relation
    if (!CcString::checkType(nl->First(memoryRelDescription))) {
        return listutils::typeError("string as first argument expected");
    }
    string oN_Rel = nl->StringValue(nl->Second(memoryRelDescription));
    ListExpr oTE_Rel = catalog->getMMObjectTypeExpr(oN_Rel);
    if (!listutils::isTupleDescription(oTE_Rel)){
        return listutils::typeError
                ("string does not identify a MemoryRelObject");
    }

    if (!CcString::checkType(nl->First(attributeDescription))) {
        return listutils::typeError("string as second argument expected");
    }

    string attrName = nl->StringValue(nl->Second(attributeDescription));
    ListExpr attrType = 0;
    int attrPos = 0;
    ListExpr attrList = nl->Second(oTE_Rel);

    if (listutils::isAttrList(attrList)){
        attrPos = listutils::findAttribute(attrList, attrName, attrType);
    }

    if (attrPos == 0){
    return listutils::typeError("there is no attribute with the given name");
    }

    return nl->ThreeElemList(
                nl->SymbolAtom(Symbols::APPEND()),
                nl->TwoElemList
                (nl->IntAtom(attrPos),nl->StringAtom(nl->ToString(attrType))),
                listutils::basicSymbol<CcString>());
}

/*

5.16.3  The Value Mapping Functions of operator ~mcreateAVLtree~

*/

int mcreateAVLtreeValMap (Word* args, Word& result,
                int message, Word& local, Supplier s) {

    result  = qp->ResultStorage(s);
    CcString* str = static_cast<CcString*>(result.addr);

// the main memory relation
    CcString* roN = (CcString*) args[0].addr;
    if(!roN->IsDefined()){
        return 0;
    }
    string relObjectName = roN->GetValue();

//the attribute
    CcString* attrN = (CcString*) args[1].addr;
    if(!attrN->IsDefined()){
        return 0;
    }
    string attrName = attrN->GetValue();

// the appended value attribute Position and attribute type
    CcInt* append = (CcInt*) args[2].addr;

    int attrPos = append->GetValue();

    CcString* aT = (CcString*)args[3].addr;
    string attrType = aT->GetValue();

    MemoryRelObject* mmrel =
        (MemoryRelObject*)catalog->getMMObject(relObjectName);
    vector<Tuple*>* relVec = mmrel->getmmrel();
    vector<Tuple*>::iterator it;
    it=relVec->begin();
    unsigned int i=0;

    avltree::AVLTree< pair<Attribute*,size_t>, KeyComparator >* tree
            = new avltree::AVLTree< pair<Attribute*,size_t>, KeyComparator >;
    Attribute* attr;
    pair<Attribute*,size_t> aPair;
    size_t usedMainMemory = 0;

    while ( it!=relVec->end()){
        Tuple* tup = *it;
        attr=tup->GetAttribute(attrPos-1);
        if(attr==0 || !attr->IsDefined()){
            return 0;
        }
        aPair = pair<Attribute*,size_t>(attr,i);
        tree->insert(aPair);
        it++;
        i++;
//        tup->DeleteIfAllowed();
    }

    // berechnen wie groß ein pair ist und aufaddieren...!!!!!!!!
    usedMainMemory = 1000;

    string  res = relObjectName +"_"+attrName;
    MemoryAVLObject* avlObject =
        new MemoryAVLObject(tree, usedMainMemory,"memoryAVLObject",attrType);
    catalog->insert(res,avlObject);

    str->Set(true, res);
    return 0;
    } //end mcreateAVLtreeValMap





/*

5.16.4 Description of operator ~mcreateAVLtree~

*/

OperatorSpec mcreateAVLtreeSpec(
    "string x string -> string",
    "mcreateAVLtree (_,_)",
    "creates an AVLtree over a main memory relation given by the"
    "first string and an attribute given by the second string",
    "query mcreateAVLtree ('Staedte', 'SName')"
);



/*

5.16.5 Instance of operator ~mcreateAVLtree~

*/

Operator mcreateAVLtreeOp (
    "mcreateAVLtree",
    mcreateAVLtreeSpec.getStr(),
    mcreateAVLtreeValMap,
    Operator::SimpleSelect,
    mcreateAVLtreeTypeMap
);


/*
5.17 Operator ~mexactmatch~
        Uses the given MemoryAVLObject (as first argument)to find all tuples
        in the given MemoryRelObject (as second argument)
        with the same key value


*/

/*
5.17.1 Type Mapping Functions of operator ~mexactmatch~
    string x string x key -> stream(tuple)


*/

ListExpr mexactmatchTypeMap(ListExpr args)
{
    if(nl->ListLength(args)!=3){
        return listutils::typeError("three arguments expected");
    }

    /* Split argument in three parts */
    ListExpr memoryAVLDescription = nl->First(args);
    ListExpr memoryRelDescription = nl->Second(args);
    ListExpr keyDescription = nl->Third(args);


    if (!CcString::checkType(nl->First(memoryAVLDescription))) {
        return listutils::typeError("string as first argument expected");
    }
    string oN_AVL = nl->StringValue(nl->Second(memoryAVLDescription));
    ListExpr oTE_AVL = catalog->getMMObjectTypeExpr(oN_AVL);

    if (!catalog->isMMObject(oN_AVL) ||
            !MemoryAVLObject::checkType(oTE_AVL)){
        return listutils::typeError
                ("first string does not identify a MemoryAVLObject");
    }

    // second a relation
    if (!CcString::checkType(nl->First(memoryRelDescription))) {
        return listutils::typeError("string as second argument expected");
    }
    string oN_Rel = nl->StringValue(nl->Second(memoryRelDescription));
    ListExpr oTE_Rel = catalog->getMMObjectTypeExpr(oN_Rel);
    if (!catalog->isMMObject(oN_Rel) ||
                !listutils::isTupleDescription(oTE_Rel)){
        return listutils::typeError
                ("second string does not identify a MemoryRelObject");
    }


    // third a key
    string keyTypeAttr = nl->ToString(nl->First(keyDescription));
    if(!listutils::isDATA(nl->First(keyDescription))){
    return listutils::typeError("key attribute expected");
    }

    MemoryAVLObject* avlobj =(MemoryAVLObject*)(catalog->getMMObject(oN_AVL));
    string keyTypeAVL = avlobj->getKeyType();

    if (keyTypeAttr!=keyTypeAVL){
        return listutils::typeError ("type conflict between keys");
    }

    return nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),oTE_Rel);
}


class mexactmatchLI{
    public:
        mexactmatchLI(avltree::AVLTree< pair<Attribute*,size_t>,
           KeyComparator >* _tree,vector<Tuple*>* _relation, Attribute* _attr1)
           :relation(_relation),tree(_tree),attr1(_attr1){

            //diese Abfrage nötig???
            if (tree->Size()!=0){
                it = tree->tail(pair<Attribute*,size_t>(attr1,0));
            }
            attr2 = attr1;

            }
    mexactmatchLI(avltree::AVLTree< pair<Attribute*,size_t>,
           KeyComparator >* _tree,vector<Tuple*>* _relation, Attribute* _attr1,
           Attribute* _attr2)
           :relation(_relation),tree(_tree),attr1(_attr1),attr2(_attr2){

            //diese Abfrage nötig???
            if (tree->Size()!=0){
                it = tree->tail(pair<Attribute*,size_t>(attr1,0));
            }


            }

        ~mexactmatchLI(){}


        Tuple* next(){
            if(it.onEnd()) return 0;
            hit = it.Get();
            if(((hit->first)->Compare(attr1)==0 ||
                        (hit->first)->Compare(attr1)==1) &&
                    ((hit->first)->Compare(attr2)==0 ||
                        (hit->first)->Compare(attr2)== -1)){
                Tuple* result = relation->at(hit->second);
                result->IncReference(); //richtig???
                it.Next();
                return result;
            }
            return 0;

        }

    private:
        vector<Tuple*>* relation;
        avltree::AVLTree< pair<Attribute*,size_t>, KeyComparator >* tree;
        Attribute* attr1;
        Attribute* attr2;
        avltree::AVLTree< pair<Attribute*, size_t>,
                            KeyComparator >::iterator it;
        const pair<Attribute*,size_t>* hit;
};

/*

5.17.3  The Value Mapping Functions of operator ~mexactmatch~

*/

int mexactmatchValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

    mexactmatchLI* li = (mexactmatchLI*) local.addr;

    switch (message)
    {
        case OPEN: {
            if(li){
                delete li;
                local.addr=0;
            }
            //first argument MemoryAVLObject
            CcString* oN_0 = (CcString*) args[0].addr;
            if(!oN_0->IsDefined()){
                return 0;
            }
            string objectName_0 = oN_0->GetValue();
            avltree::AVLTree< pair<Attribute*,size_t>, KeyComparator >* tree;
            MemoryAVLObject* avlObject =
                    (MemoryAVLObject*)catalog->getMMObject(objectName_0);
            tree = avlObject->getAVLtree();

            //second argument MemoryRelObject
            CcString* oN_1 = (CcString*) args[1].addr;
            if(!oN_1->IsDefined()){
                return 0;
            }
            string objectName_1 = oN_1->GetValue();
            vector<Tuple*>* relation;
            MemoryRelObject* mro =
                    (MemoryRelObject*)catalog->getMMObject(objectName_1);
            relation = mro->getmmrel();

            // third argument key value
            Attribute* attr = (Attribute*)args[2].addr;
            local.addr= new mexactmatchLI(tree,relation,attr);
            return 0;
        }

        case REQUEST:
            result.addr=(li?li->next():0);
            return result.addr?YIELD:CANCEL;


        case CLOSE:
            if(li)
            {
            delete li;
            local.addr = 0;
            }
            return 0;
   }

    return 0;
}

/*

5.17.4 Description of operator ~mexactmatch~

*/

OperatorSpec mexactmatchSpec(
    "string x string x key -> stream(tuple) ",
    "_ _ mexactmatch [_]",
    "Uses the given avltree to find all tuples"
      " in the given relation with the same key value.",
    "query 'Staedte_SName''Staedte' mexactmatch"
    "['Wuppertal'] count"
);

/*

5.17.5 Instance of operator ~mexactmatch~

*/

Operator mexactmatchOp (
    "mexactmatch",
    mexactmatchSpec.getStr(),
    mexactmatchValMap,
    Operator::SimpleSelect,
    mexactmatchTypeMap
);



/*
5.18 Operator ~mrange~
        Uses the given MemoryAVLObject (as first argument)to find all tuples
        in the given MemoryRelObject (as second argument)
        which are between the first and the second attribute value
        (as third and fourth argument)

*/

/*
5.18.1 Type Mapping Functions of operator ~mrange~
    string x string x key x key -> stream(tuple)


*/

ListExpr mrangeTypeMap(ListExpr args)
{
    if(nl->ListLength(args)!=4){
        return listutils::typeError("four arguments expected");
    }

    /* Split argument in three parts */
    ListExpr memoryAVLDescription = nl->First(args);
    ListExpr memoryRelDescription = nl->Second(args);
    ListExpr keyDescription1 = nl->Third(args);
    ListExpr keyDescription2 = nl->Fourth(args);


    if (!CcString::checkType(nl->First(memoryAVLDescription))) {
        return listutils::typeError("string as first argument expected");
    }
    string oN_AVL = nl->StringValue(nl->Second(memoryAVLDescription));
    ListExpr oTE_AVL = catalog->getMMObjectTypeExpr(oN_AVL);

    if (!catalog->isMMObject(oN_AVL) ||
            !MemoryAVLObject::checkType(oTE_AVL)){
        return listutils::typeError
                ("first string does not identify a MemoryAVLObject");
    }

    // second a relation
    if (!CcString::checkType(nl->First(memoryRelDescription))) {
        return listutils::typeError("string as second argument expected");
    }
    string oN_Rel = nl->StringValue(nl->Second(memoryRelDescription));
    ListExpr oTE_Rel = catalog->getMMObjectTypeExpr(oN_Rel);
    if (!catalog->isMMObject(oN_Rel) ||
                !listutils::isTupleDescription(oTE_Rel)){
        return listutils::typeError
                ("second string does not identify a MemoryRelObject");
    }


    // third a key
    string keyTypeAttr1 = nl->ToString(nl->First(keyDescription1));
    string keyTypeAttr2 = nl->ToString(nl->First(keyDescription2));
    if(!listutils::isDATA(nl->First(keyDescription1)) ||
            !listutils::isDATA(nl->First(keyDescription2))){
        return listutils::typeError("key attribute expected");
    }

    MemoryAVLObject* avlobj =(MemoryAVLObject*)(catalog->getMMObject(oN_AVL));
    string keyTypeAVL = avlobj->getKeyType();

    if (keyTypeAttr1!=keyTypeAVL || (keyTypeAttr2!=keyTypeAVL)){
        return listutils::typeError ("type conflict between keys");
    }

    return nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),oTE_Rel);
}

//////// für Range Abfrage:
//////// if(!nl->Equal(keyDescription, secondKeyDescription)){
////////       return listutils::typeError("different key types");
////////    }

/*

5.18.3  The Value Mapping Functions of operator ~mrange~

*/

int mrangeValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

    mexactmatchLI* li = (mexactmatchLI*) local.addr;

    switch (message)
    {
        case OPEN: {
            if(li){
                delete li;
                local.addr=0;
            }
            //first argument MemoryAVLObject
            CcString* oN_0 = (CcString*) args[0].addr;
            if(!oN_0->IsDefined()){
                return 0;
            }
            string objectName_0 = oN_0->GetValue();
            avltree::AVLTree< pair<Attribute*,size_t>, KeyComparator >* tree;
            MemoryAVLObject* avlObject =
                    (MemoryAVLObject*)catalog->getMMObject(objectName_0);
            tree = avlObject->getAVLtree();

            //second argument MemoryRelObject
            CcString* oN_1 = (CcString*) args[1].addr;
            if(!oN_1->IsDefined()){
                return 0;
            }
            string objectName_1 = oN_1->GetValue();
            vector<Tuple*>* relation;
            MemoryRelObject* mro =
                    (MemoryRelObject*)catalog->getMMObject(objectName_1);
            relation = mro->getmmrel();

            // third argument key value
            Attribute* attr1 = (Attribute*)args[2].addr;
            Attribute* attr2 = (Attribute*)args[3].addr;
            local.addr= new mexactmatchLI(tree,relation,attr1, attr2);
            return 0;
        }

        case REQUEST:
            result.addr=(li?li->next():0);
            return result.addr?YIELD:CANCEL;


        case CLOSE:
            if(li)
            {
            delete li;
            local.addr = 0;
            }
            return 0;
   }

    return 0;
}


/*

5.18.4 Description of operator ~range~

*/

OperatorSpec mrangeSpec(
    "string x string x key x key -> stream(tuple) ",
    "_ _ mrange[_,_]",
    "Uses the given rtree to find all tuples"
      " in the given relation which are between "
      "the first and the second attribute value.",
    "query 'Staedte_SName' 'Staedte' mrange ['Aachen','Dortmund'] count"

);

/*

5.18.5 Instance of operator ~mrange~

*/

Operator mrangeOp (
    "mrange",
    mrangeSpec.getStr(),
    mrangeValMap,
    Operator::SimpleSelect,
    mrangeTypeMap
);



TypeConstructor MemoryRelObjectTC(
    MemoryRelObject::BasicType(),     // name of the type in SECONDO
    MemoryRelObject::Property,        // property function describing signature
    MemoryRelObject::Out, MemoryRelObject::In,          // out und in functions
    0, 0,                             // SaveToList, RestoreFromList functions
    // object creation and deletion create und delete
    MemoryRelObject::create,MemoryRelObject::deleteMemoryRelObject,
    0, 0,                            // object open, save
    0, 0,                             // close and clone
    0,                                // cast function
    MemoryRelObject::SizeOfObj,      // sizeof function
    MemoryRelObject::KindCheck);      // kind checking



class MainMemoryAlgebra : public Algebra
{


    public:
        MainMemoryAlgebra() : Algebra()
        {
            catalog = new MemCatalog();

/*

6.2 Registration of Types


*/

        AddTypeConstructor (&MemoryRelObjectTC);
        MemoryRelObjectTC.AssociateKind( Kind::SIMPLE() );

/*
6.3 Registration of Operators

*/
        AddOperator (&memloadOp);
        AddOperator (&meminitOp);
        meminitOp.SetUsesMemory();
        AddOperator (&mfeedOp);
        mfeedOp.SetUsesArgsInTypeMapping();
        AddOperator (&letmconsumeOp);
        AddOperator (&memdeleteOp);
        AddOperator (&memobjectOp);
        memobjectOp.SetUsesArgsInTypeMapping();
        AddOperator (&memgetcatalogOp);
        AddOperator (&memletOp);
        AddOperator (&memupdateOp);
        AddOperator (&mcreateRtreeOp);
        AddOperator (&memsizeOp);
        AddOperator (&memclearOp);
        AddOperator (&minsertOp);
        minsertOp.SetUsesArgsInTypeMapping();
        AddOperator (&mwindowintersectsOp);
        mwindowintersectsOp.SetUsesArgsInTypeMapping();
        AddOperator (&mconsumeOp);
        AddOperator (&mcreateAVLtreeOp);
        mcreateAVLtreeOp.SetUsesArgsInTypeMapping();
        AddOperator (&mexactmatchOp);
        mexactmatchOp.SetUsesArgsInTypeMapping();
        AddOperator (&mrangeOp);
        mrangeOp.SetUsesArgsInTypeMapping();
        }
        ~MainMemoryAlgebra() {
          delete catalog;
          catalog = 0;
        };
};




} // ende namespace mmalgebra

/*
7 Initialization

Each algebra module needs an initialization function. The algebra manager
has a reference to this function if this algebra is included in the list
of required algebras, thus forcing the linker to include this module.

The algebra manager invokes this function to get a reference to the instance
of the algebra class and to provide references to the global nested list
container (used to store constructor, type, operator and object information)
and to the query processor.

The function has a C interface to make it possible to load the algebra
dynamically at runtime (if it is built as a dynamic link library). The name
of the initialization function defines the name of the algebra module. By
convention it must start with "Initialize<AlgebraName>".

To link the algebra together with the system you must create an
entry in the file "makefile.algebra" and to define an algebra ID in the
file "Algebras/Management/AlgebraList.i.cfg".

*/

extern "C"
Algebra*
InitializeMainMemoryAlgebra(NestedList* nlRef, QueryProcessor* qpRef)
{

  return (new mmalgebra::MainMemoryAlgebra);
}




