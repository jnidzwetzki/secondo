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

//[_] [\_]
//characters      [1]   verbatim:   [$]   [$]
//characters      [2]   formula:    [$]   [$]
//characters      [3]   capital:    [\textsc{]  [}]
//characters      [4]   teletype:   [\texttt{]  [}]

5.1 Implementation of the MTreeAlgebra (file: MTreeAlgebra.cpp)

November/December 2007, Mirko Dibbert

5.1.1 Imcludes and Defines

*/
using namespace std;

#include "Algebra.h"
#include "NestedList.h"
#include "QueryProcessor.h"
#include "RelationAlgebra.h"
#include "TupleIdentifier.h"
#include "MTree.h"
#include "MTreeConfig.h"

extern NestedList* nl;
extern QueryProcessor* qp;
extern AlgebraManager* am;

/*
5.1.2 Type Constructors

5.1.2.1 Type Constructor ~MTree~

*/
static ListExpr
MTreeProp()
{
  ListExpr examplelist = nl->TextAtom();
  nl->AppendText( examplelist, "<relation> createmtree [<attrname>] "
          "where <attrname> is the key" );

  return ( nl->TwoElemList(
         nl->TwoElemList( nl->StringAtom( "Creation" ),
                  nl->StringAtom( "Example Creation" ) ),
         nl->TwoElemList( examplelist,
                  nl->StringAtom( "(let mymtree = ten "
                          "createmtree " ) ) ) );
}

ListExpr
OutMTree( ListExpr typeInfo, Word  value )
{
  MT::MTree* mtree = static_cast<MT::MTree*>(value.addr);

  return nl->FiveElemList(
    nl->StringAtom( "M-Tree statistics" ),
        nl->TwoElemList( nl->StringAtom( "Height" ),
                         nl->IntAtom( mtree->getHeight() ) ),
        nl->TwoElemList( nl->StringAtom( "# of routing nodes" ),
                         nl->IntAtom( mtree->getRoutingCount() ) ),
        nl->TwoElemList( nl->StringAtom( "# of leafes" ),
                         nl->IntAtom( mtree->getLeafCount() ) ),
        nl->TwoElemList( nl->StringAtom( "# of (leaf) entries" ),
                         nl->IntAtom( mtree->getEntryCount() ) ) );
}

Word
InMTree( ListExpr typeInfo, ListExpr value,
        int errorPos, ListExpr &errorInfo, bool &correct )
{
  correct = false;
  return SetWord( 0 );
}

ListExpr
SaveToListMTree( ListExpr typeInfo, Word  value )
{
  return nl->IntAtom( 0 );
}

Word
RestoreFromListMTree( ListExpr typeInfo, ListExpr value,
               int errorPos, ListExpr &errorInfo,
               bool &correct )
{
  return SetWord( Address( 0 ) );
}

Word
CreateMTree( const ListExpr typeInfo )
{
  return SetWord( new MT::MTree() );
}

void
DeleteMTree( const ListExpr typeInfo, Word &w )
{
  static_cast<MT::MTree*>(w.addr)->deleteFile();
  delete static_cast<MT::MTree*>(w.addr);
  w.addr = 0;
}

bool
OpenMTree( SmiRecord &valueRecord,
       size_t &offset,
       const ListExpr typeInfo,
       Word &value )
{
  SmiFileId fileid;
  valueRecord.Read( &fileid, sizeof( SmiFileId ), offset );
  offset += sizeof( SmiFileId );

  MT::MTree* mtree = new MT::MTree( fileid );
  value = SetWord( mtree );
  return true;
}

bool
SaveMTree( SmiRecord &valueRecord,
       size_t &offset,
       const ListExpr typeInfo,
       Word &value )
{
  SmiFileId fileId;
  MT::MTree *mtree = ( MT::MTree* )value.addr;
  fileId = mtree->getFileId();
  if (fileId)
  {
    valueRecord.Write( &fileId, sizeof( SmiFileId ), offset );
    offset += sizeof( SmiFileId );
    return true;
  }
  else
  {
    return false;
  }
}

void CloseMTree( const ListExpr typeInfo, Word &w )
{
  MT::MTree *mtree = ( MT::MTree* )w.addr;
  delete mtree;
}

Word CloneMTree( const ListExpr typeInfo, const Word &w )
{
  MT::MTree* res = new MT::MTree(*static_cast<MT::MTree*>(w.addr));
  return SetWord( res );
}

void *CastMTree( void *addr )
{
  return ( 0 );
}

int SizeOfMTree()
{
  return 0;
}

bool CheckMTree( ListExpr type, ListExpr &errorInfo )
{
  //TODO not yet implemented
  return true;
}

TypeConstructor
mtree( "mtree",      MTreeProp,
     OutMTree,    InMTree,
     SaveToListMTree, RestoreFromListMTree,
     CreateMTree,   DeleteMTree,
     OpenMTree,     SaveMTree,
     CloseMTree,    CloneMTree,
     CastMTree,     SizeOfMTree,
     CheckMTree );

/*
5.1.3 Operators

5.1.3.1 Operator ~createmtree~

*/
int
CreateMTreeValueMapping_Rel( Word  *args, Word  &result,
               int message, Word  &local, Supplier s )
{
  result = qp->ResultStorage( s );
  MT::MTree *mtree = ( MT::MTree* )result.addr;

  Relation* relation = ( Relation* )args[0].addr;
  int attrIndex = (( CcInt* )args[4].addr )->GetIntval();
  string type = (( CcString* )args[5].addr )->GetValue();
  string mfName = (( CcString* )args[6].addr )->GetValue();
  string configName = (( CcString* )args[7].addr )->GetValue();
  Tuple *tuple;
  GenericRelationIterator *iter = relation->MakeScan();

  mtree->initialize( type, mfName, configName );

  #ifdef __MT_PRINT_CONFIG_INFO
  mtree->printMTreeConfig();
  #endif

  while (( tuple = iter->GetNextTuple() ) != 0 )
  {
    Attribute* attr = tuple->GetAttribute( attrIndex );
    if( attr->IsDefined() )
    {
      mtree->insert( attr, tuple->GetTupleId() );
    }
    tuple->DeleteIfAllowed();
  }
  delete iter;

  #ifdef MT_PRINT_INSERT_INFO
  cmsg.info() << endl;
  cmsg.send();
  #endif

  mtree->finalizeInsert();
  return 0;
}

int CreateMTreeValueMapping_Stream( Word  *args, Word  &result,
                  int message, Word  &local, Supplier s )
{
  result = qp->ResultStorage( s );
  MT::MTree *mtree = ( MT::MTree* )result.addr;

  int attrIndex = (( CcInt* )args[4].addr )->GetIntval();
  string type = (( CcString* )args[5].addr )->GetValue();
  string mfName = (( CcString* )args[6].addr )->GetValue();
  string configName = (( CcString* )args[7].addr )->GetValue();

  mtree->initialize( type, mfName, configName );

  #ifdef __MT_PRINT_CONFIG_INFO
  mtree->printMTreeConfig();
  #endif

  Word wTuple;

  assert(mtree != 0);

  qp->Open(args[0].addr);
  qp->Request(args[0].addr, wTuple);
  while (qp->Received(args[0].addr))
  {
    Tuple* tuple = (Tuple*)wTuple.addr;
    Attribute* attr = tuple->GetAttribute( attrIndex );
    AttributeType type =
        tuple->GetTupleType()->GetAttributeType( attrIndex );
    if( attr->IsDefined() )
    {
      mtree->insert( attr, tuple->GetTupleId() );
    }
    tuple->DeleteIfAllowed();
    qp->Request(args[0].addr, wTuple);
  }
  qp->Close(args[0].addr);

  return 0;
}


int CreateMTreeSelect( ListExpr args )
{
  if ( nl->IsEqual( nl->First( nl->First( args ) ), "rel" ) )
    return 0;

  if ( nl->IsEqual( nl->First( nl->First( args ) ), "stream" ) )
    return 1;

  return -1;
}

ValueMapping CreateMTreeMap[] = { CreateMTreeValueMapping_Rel,
                  CreateMTreeValueMapping_Stream
                };


ListExpr CreateMTreeTypeMapping( ListExpr args )
{
  string errmsg;
  bool cond;
  NList nl_args( args );

  errmsg = "Operator createmtree expects three arguments.";
  CHECK_COND( nl_args.length() == 4, errmsg );

  NList arg1 = nl_args.first();
  NList arg2 = nl_args.second();
  NList arg3 = nl_args.third();
  NList arg4 = nl_args.fourth();

  // check first argument (should be relation or stream)
  cond = !(arg1.isAtom()) &&
         (
           ( arg1.first().isEqual( "rel" ) &&
             IsRelDescription( arg1.listExpr() )) ||
           ( arg1.first().isEqual( "stream" ) &&
             IsStreamDescription( arg1.listExpr() ))
         );
  errmsg = "Operator createmtree expects a list with structure\n"
           "   rel (tuple ((a1 t1)...(an tn))) or\n"
           "   stream (tuple ((a1 t1)...(an tn)))\n"
           "as first argument, but got a list with structure '" +
       arg1.convertToString() + "'.";
  CHECK_COND( cond , errmsg);

  // check, if third argument is an attribute name
  errmsg = "Operator createmtree expects an attribute name "
           "as fourth argument, but got '" +
           arg4.convertToString() + "'.";
  CHECK_COND( arg4.isSymbol(), errmsg);

  string attrName = arg4.str();
  NList tupleDescription = arg1.second();
  NList attrList = tupleDescription.second();

  // check, if attribute can be found in attribute list
  errmsg = "Attribute name '" + attrName + "' is not known.\n"
           "Known Attribute(s):\n" + attrList.convertToString();
  ListExpr attrTypeLE;
  int attrIndex = FindAttribute( attrList.listExpr(),
                                 attrName, attrTypeLE );
  CHECK_COND( attrIndex > 0, errmsg );
  NList attrType ( attrTypeLE );

  // check if the metric given in second argument is defined
  errmsg = "Operator createmtree expects the name of a registered"
           "metric as second argument, but got a list with structure"
           " '" + arg2.convertToString() + "'.";
  CHECK_COND( arg2.isSymbol(), errmsg);
  string mfName = arg2.str();
  errmsg = "Metric " + mfName + " for type constructor " +
           attrType.convertToString() + " not defined!";
  cond = MetricRegistry::getMetric(
      attrType.convertToString(), mfName ) != 0;
  CHECK_COND( cond, errmsg);

  // check if the mtree-config given in third argument is defined
  errmsg = "Operator createmtree expects the name of a registered"
           "mtree-config object as third argument, but got a list "
           "with structure '" + arg2.convertToString() + "'.";
  CHECK_COND( arg3.isSymbol(), errmsg);
  string configName = arg3.str();
  errmsg = "MTreeConfig " + configName + " for type constructor " +
           attrType.convertToString() + " not defined!";
  cond = MT::MTreeConfigReg::contains( configName );
  CHECK_COND( cond, errmsg);

  NList result (
      NList( "APPEND" ),
      NList(
        attrIndex - 1,
        NList ( attrType.convertToString(), true ),
        NList ( mfName, true ),
        NList ( configName, true ) ),
      NList( NList( "mtree" ), tupleDescription, attrType ) );
  return result.listExpr();
}

struct CreateMTreeInfo : OperatorInfo
{
  CreateMTreeInfo()
  {
    name = "createmtree";
    signature =
      "( <text>(rel (tuple ((id tid) (x1 t1)...(xn tn)))"
      " metricName, mtreeConfigName, xi) ->"
      " (mtree (tuple ((x1 t1)...(xn tn))) ti)";
    syntax = "_ createmtree [_, _, _]";
    meaning = "";
    example = "pictures createmtree [default, default, Pic]";
  }
};



/********************************************************************
5.1.3.2 Operator range

********************************************************************/

struct RangeSearchLocalInfo
{
  Relation* relation;
  list<TupleId>* results;
  list<TupleId>::iterator iter;

  RangeSearchLocalInfo( Relation* rel ) :
    relation( rel ),
    results( new list<TupleId> )
    {}

  void initResultIterator()
  {
    iter = results->begin();
  }

  ~RangeSearchLocalInfo()
  {
    delete results;
  }

  TupleId next()
  {
    if ( iter != results->end() )
    {
      TupleId tid = *iter;
      *iter++;
      return tid;
    }
    else
    {
      return 0;
    }
  }
};

int RangeSearchValueMapping_Rel( Word  *args, Word  &result,
                  int message, Word  &local, Supplier s )
{
  RangeSearchLocalInfo *localInfo;

  switch (message)
  {
    case OPEN :
    {
      localInfo = new RangeSearchLocalInfo(
          static_cast<Relation*>( args[0].addr ) );
      MT::MTree* mtree = static_cast<MT::MTree*>( args[1].addr );
      Attribute* attr = static_cast<Attribute*>( args[2].addr );
      double searchRad = ((CcReal*)args[3].addr)->GetValue();

      #ifdef __MT_PRINT_CONFIG_INFO
      mtree->printMTreeConfig();
      #endif

      mtree->rangeSearch( attr, searchRad, localInfo->results );
      localInfo->initResultIterator();

      assert(localInfo->relation != 0);
      local = SetWord(localInfo);
      return 0;
    }

    case REQUEST :
    {
      localInfo = (RangeSearchLocalInfo*)local.addr;

      TupleId tid = localInfo->next();
      if( tid )
      {
        Tuple *tuple = localInfo->relation->GetTuple( tid );
        result = SetWord( tuple );
        return YIELD;
      }
      else
      {
        return CANCEL;
      }
    }

    case CLOSE :
    {
      localInfo = (RangeSearchLocalInfo*)local.addr;
      delete localInfo;
      return 0;
    }
  }
  return 0;
}

int
RangeSearchValueMapping_Stream( Word  *args, Word  &result,
               int message, Word  &local, Supplier s )
{
  return 0;
}

int RangeSearchSelect( ListExpr args )
{
  if ( nl->IsEqual( nl->First( nl->First( args ) ), "rel" ) )
    return 0;

  if ( nl->IsEqual( nl->First( nl->First( args ) ), "stream" ) )
    return 1;

  return -1;
}

ValueMapping RangeSearchMap[] = { RangeSearchValueMapping_Rel,
                  RangeSearchValueMapping_Stream
                };


ListExpr RangeSearchTypeMapping( ListExpr args )
{
  string errmsg;
  bool cond;
  NList nl_args( args );

  errmsg = "Operator range expects three arguments.";
  CHECK_COND( nl_args.length() == 4, errmsg );

  NList arg1 = nl_args.first();
  NList arg2 = nl_args.second();
  NList arg3 = nl_args.third();
  NList arg4 = nl_args.fourth();

  // check first argument (should be relation or stream)
  cond = !(arg1.isAtom()) &&
         (
           ( arg1.first().isEqual( "rel" ) &&
             IsRelDescription( arg1.listExpr() )) ||
           ( arg1.first().isEqual( "stream" ) &&
             IsStreamDescription( arg1.listExpr() ))
         );
  errmsg = "Operator rangesearch expects a list with structure\n"
           "   rel (tuple ((a1 t1)...(an tn))) or\n"
           "   stream (tuple ((a1 t1)...(an tn)))\n"
           "as first argument, but got a list with structure '" +
       arg1.convertToString() + "'.";
  CHECK_COND( cond , errmsg );

  cond = !(arg2.isAtom()) &&
         arg2.first().isEqual( "mtree" ) &&
         arg2.second().first().isEqual( "tuple" ) &&
         IsTupleDescription( arg2.second().second().listExpr() ) &&
         arg2.third().isSymbol();

  errmsg = "Operator rangesearch expects a mtree "
           "as second argument, but got '" +
           arg2.convertToString() + "'.";
  CHECK_COND( cond , errmsg );

  // check second argument

  // check if used attribute is equal to attribute used in m-tree
  cond = arg2.third().isEqual( arg3.convertToString() );
  errmsg = "The used m-tree contains attributes of type " +
           arg2.third().convertToString() + ", but the given "
           " attribute argument is of type " +
           arg3.convertToString();
  CHECK_COND( cond, errmsg );

  // check fourth argument
  errmsg = "Operator createmtree expects an real value as fourth "
           "argument, but got '" + arg4.convertToString() + "'.";
  CHECK_COND( arg4.isEqual( "real" ), errmsg );

  return
    nl->TwoElemList(
      nl->SymbolAtom("stream"),
      arg1.second().listExpr());
}

struct RangeSearchInfo : OperatorInfo
{
  RangeSearchInfo()
  {
    name = "rangesearch";
    signature =
      "( <text>(rel (tuple ((id tid) (x1 t1)...(xn tn)))) x"
      " (mtree (tuple ((x1 t1)...(xn tn))) ti) ->"
      " (stream (tuple ((x1 t1)...(xn tn))))";
    syntax = "_ _ rangesearch [_, _]";
    meaning = "";
    example = "pictures pictree rangesearch [pic1, 0.5]";
    remark = "";
  }
};

/********************************************************************
5.1.3.3 Operator nnsearch

********************************************************************/

struct NNSearchLocalInfo
{
  Relation* relation;
  list<TupleId>* results;
  list<TupleId>::iterator iter;

  NNSearchLocalInfo( Relation* rel ) :
    relation( rel ),
    results( new list<TupleId> )
    {}

  void initResultIterator()
  {
    iter = results->begin();
  }

  ~NNSearchLocalInfo()
  {
    delete results;
  }

  TupleId next()
  {
    if ( iter != results->end() )
    {
      TupleId tid = *iter;
      *iter++;
      return tid;
    }
    else
    {
      return 0;
    }
  }
};

int NNSearchValueMapping_Rel( Word  *args, Word  &result,
                  int message, Word  &local, Supplier s )
{
  NNSearchLocalInfo *localInfo;

  switch (message)
  {
    case OPEN :
    {
      localInfo = new NNSearchLocalInfo(
          static_cast<Relation*>( args[0].addr ) );
      MT::MTree* mtree = static_cast<MT::MTree*>( args[1].addr );
      Attribute* attr = static_cast<Attribute*>( args[2].addr );
      int nncount= ((CcInt*)args[3].addr)->GetValue();

      #ifdef __MT_PRINT_CONFIG_INFO
      mtree->printMTreeConfig();
      #endif

      mtree->nnSearch( attr, nncount, localInfo->results );
      localInfo->initResultIterator();

      assert(localInfo->relation != 0);
      local = SetWord(localInfo);
      return 0;
    }

    case REQUEST :
    {
      localInfo = (NNSearchLocalInfo*)local.addr;

      TupleId tid = localInfo->next();
      if( tid )
      {
        Tuple *tuple = localInfo->relation->GetTuple( tid );
        result = SetWord( tuple );
        return YIELD;
      }
      else
      {
        return CANCEL;
      }
    }

    case CLOSE :
    {
      localInfo = (NNSearchLocalInfo*)local.addr;
      delete localInfo;
      return 0;
    }
  }
  return 0;
}

int
NNSearchValueMapping_Stream( Word  *args, Word  &result,
               int message, Word  &local, Supplier s )
{
  return 0;
}

int NNSearchSelect( ListExpr args )
{
  if ( nl->IsEqual( nl->First( nl->First( args ) ), "rel" ) )
    return 0;

  if ( nl->IsEqual( nl->First( nl->First( args ) ), "stream" ) )
    return 1;

  return -1;
}

ValueMapping NNSearchMap[] = { NNSearchValueMapping_Rel,
                  NNSearchValueMapping_Stream
                };


ListExpr NNSearchTypeMapping( ListExpr args )
{
  string errmsg;
  bool cond;
  NList nl_args( args );

  errmsg = "Operator nnsearch expects three arguments.";
  CHECK_COND( nl_args.length() == 4, errmsg );

  NList arg1 = nl_args.first();
  NList arg2 = nl_args.second();
  NList arg3 = nl_args.third();
  NList arg4 = nl_args.fourth();

  // check first argument (should be relation or stream)
  cond = !(arg1.isAtom()) &&
         (
           ( arg1.first().isEqual( "rel" ) &&
             IsRelDescription( arg1.listExpr() )) ||
           ( arg1.first().isEqual( "stream" ) &&
             IsStreamDescription( arg1.listExpr() ))
         );
  errmsg = "Operator nnsearch expects a list with structure\n"
           "   rel (tuple ((a1 t1)...(an tn))) or\n"
           "   stream (tuple ((a1 t1)...(an tn)))\n"
           "as first argument, but got a list with structure '" +
       arg1.convertToString() + "'.";
  CHECK_COND( cond , errmsg);

  // check second argument
  errmsg = "Operator nnearch expects a mtree "
           "as second argument, but got '" +
           arg2.convertToString() + "'.";
  CHECK_COND( arg2.first().isEqual( "mtree" ), errmsg );

  // check if used attribute is equal to attribute used in m-tree
  cond = arg2.third().isEqual( arg3.convertToString() );
  errmsg = "The used m-tree contains attributes of type " +
           arg2.third().convertToString() + ", but the given "
           " attribute argument is of type " +
           arg3.convertToString();
  CHECK_COND( cond, errmsg );

  // check fourth argument
  errmsg = "Operator nnsearch expects an int value as fourth "
           "argument, but got '" + arg4.convertToString() + "'.";
  CHECK_COND( arg4.isEqual( "int" ), errmsg );

  return
    nl->TwoElemList(
      nl->SymbolAtom("stream"),
      arg1.second().listExpr());
}

struct NNSearchInfo : OperatorInfo
{
  NNSearchInfo()
  {
    name = "nnsearch";
    signature =
      "( <text>(rel (tuple ((id tid) (x1 t1)...(xn tn)))) x"
      " (mtree (tuple ((x1 t1)...(xn tn))) ti) ->"
      " (stream (tuple ((x1 t1)...(xn tn))))";
    syntax = "_ _ nnsearch [_, _]";
    meaning = "";
    example = "pictures pictree nnsearch [pic1, 0.5]";
    remark = "";
  }
};

/*
5.1.5 Create and initialize the Algebra

*/

class MTreeAlgebra : public Algebra
{

public:
  MTreeAlgebra() : Algebra()
  {
    AddTypeConstructor( &mtree );
    AddOperator(
        CreateMTreeInfo(),
        CreateMTreeMap,
        CreateMTreeSelect,
        CreateMTreeTypeMapping );

    AddOperator(
        RangeSearchInfo(),
        RangeSearchMap,
        RangeSearchSelect,
        RangeSearchTypeMapping );

    AddOperator(
        NNSearchInfo(),
        NNSearchMap,
        NNSearchSelect,
        NNSearchTypeMapping );
  }

  ~MTreeAlgebra() {};
};

MTreeAlgebra mtreeAlgebra;

extern "C"
  Algebra*
  InitializeMTreeAlgebra( NestedList  *nlRef,
              QueryProcessor  *qpRef )
{
  nl = nlRef;
  qp = qpRef;
  return ( &mtreeAlgebra );
}
