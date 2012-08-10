/*
This file is part of SECONDO.

Copyright (C) 2011, University in Hagen, Department of Computer Science,
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

2012, May- Simone Jandt

1 Includes

*/

#include <assert.h>
#include <utility>
#include "AlgebraTypes.h"
#include "ListUtils.h"
#include "SecondoSystem.h"
#include "Symbols.h"
#include "LogMsg.h"
#include "SpatialAlgebra.h"
#include "Direction.h"
#include "JNetwork.h"
#include "RouteLocation.h"
#include "JRouteInterval.h"
#include "MJPoint.h"

extern NestedList* nl;

const double TOLERANCE = 0.0000001;

/*
1 Helpful Operations

1.1 ~getRelationCopy~

Returns a pointer to the copy of the relation of relPointer.

*/

OrderedRelation* getRelationCopy(const string relTypeInfo,
                                 const OrderedRelation* relPointer)
{
  ListExpr relType;
  nl->ReadFromString ( relTypeInfo, relType );
  ListExpr relNumType = SecondoSystem::GetCatalog()->NumericType(relType);
  Word wrel;
  wrel.setAddr(const_cast<OrderedRelation*> (relPointer));
  Word res = OrderedRelation::Clone(relNumType, wrel);
  return (OrderedRelation*) res.addr;
}

Relation* getRelationCopy(const string relTypeInfo,
                          const Relation* relPointer)
{
  ListExpr strPtr = listutils::getPtrList(relPointer);
  string querystring = "(consume (feed (" + relTypeInfo + "(ptr "+
                        nl->ToString(strPtr) + "))))";
  Word resultWord;
  int QueryExecuted = QueryProcessor::ExecuteQuery(querystring, resultWord);
  assert (QueryExecuted);
  return (Relation*) resultWord.addr;
}

/*
1.1 ~relationToList~

Returns the list representation of rel.

*/

ListExpr relationToList(Relation* rel, const string relTypeInfo)
{
  if (rel != 0)
  {
    GenericRelationIterator* it = rel->MakeScan();
    ListExpr typeInfo;
    nl->ReadFromString(relTypeInfo, typeInfo);
    ListExpr relList = Relation::Out(typeInfo, it);
    return relList;
  }
  else
    return nl->TheEmptyList();
}

ListExpr relationToList(OrderedRelation* rel, const string relTypeInfo)
{
  if (rel != 0)
  {
    ListExpr typeInfo;
    nl->ReadFromString(relTypeInfo, typeInfo);
    Word w;
    w.setAddr(rel);
    ListExpr relList = OrderedRelation::Out(typeInfo, w);
    return relList;
  }
  else
    return nl->TheEmptyList();
}

Relation* relationFromList(const ListExpr nlRel, const string descriptor,
                          const int errorPos, ListExpr& errorInfo,
                          bool& correct)
{
  ListExpr relType;
  nl->ReadFromString(descriptor, relType);
  ListExpr relNumType = SecondoSystem::GetCatalog()->NumericType ( relType );
  return (Relation*) Relation::In(relNumType, nlRel, errorPos, errorInfo,
                                  correct);
}

OrderedRelation* ordRelationFromList(const ListExpr nlRel,
                                     const string descriptor,
                                     const int errorPos, ListExpr& errorInfo,
                                     bool& correct)
{
  ListExpr relType;
  nl->ReadFromString(descriptor, relType);
  ListExpr relNumType = SecondoSystem::GetCatalog()->NumericType ( relType );
  Word wRel = OrderedRelation::In(relNumType, nlRel, errorPos, errorInfo,
                                  correct);
  return (OrderedRelation*) wRel.addr;
}

/*
1.1 ~createBTree~

Creates an BTree over the attribut attr of the given relation rel, which is
described by descriptor.

*/

BTree* createBTree(const Relation* rel, const string descriptor,
                   const string attr)
{
  ListExpr relPtr = listutils::getPtrList(rel);
  string strQuery = "(createbtree (" + descriptor +
                      " (ptr " + nl->ToString(relPtr) + "))" + attr + ")";
  Word w;
  int QueryExecuted = QueryProcessor::ExecuteQuery ( strQuery, w);
  assert ( QueryExecuted );
  return ( BTree* ) w.addr;
}

/*
1.1 ~createRTree~

Creates the RTree over the given spatial attribute attr of the given relation
rel, which is described by descriptor.

*/

R_Tree<2,TupleId>* createRTree(const Relation* rel, const string descriptor,
                               const string attr)
{
  ListExpr relPtr = listutils::getPtrList(rel);
  string strQuery = "(bulkloadrtree(sortby(addid(feed (" + descriptor +
                 " (ptr " + nl->ToString(relPtr) + "))))((" + attr +" asc)))" +
                 attr +" TID)";
  Word w;
  int QueryExecuted = QueryProcessor::ExecuteQuery ( strQuery, w );
  assert ( QueryExecuted );
  return ( R_Tree<2,TupleId>* ) w.addr;
}

/*
1.1 ~openRelation~

Opens the relation described by descriptor from valueRecord starting at offset.

*/


bool openRelation(Relation*& rel, const string descriptor,
                  SmiRecord& valueRecord, size_t& offset)
{
  ListExpr relType;
  nl->ReadFromString ( descriptor, relType );
  ListExpr relNumType = SecondoSystem::GetCatalog()->NumericType(relType);
  rel = Relation::Open(valueRecord,offset,relNumType);
  return (rel != 0);
}

bool openRelation(OrderedRelation*& rel, const string descriptor,
                  SmiRecord& valueRecord, size_t& offset)
{
  rel = 0;
  ListExpr relType;
  nl->ReadFromString ( descriptor, relType );
  ListExpr relNumType = SecondoSystem::GetCatalog()->NumericType(relType);
  Word wrel;
  bool ok = OrderedRelation::Open(valueRecord, offset, relNumType, wrel);
  if (ok)
    rel = (OrderedRelation*) wrel.addr;
  return ok;
}


/*
1.1 ~openBTree~

Opens the btree described by descriptor from valueRecord starting at offset.

*/

bool openBTree(BTree*& tree, const string descriptor, SmiRecord& valueRecord,
                 size_t& offset)
{
  ListExpr treeType;
  nl->ReadFromString(descriptor,treeType);
  ListExpr treeNumType = SecondoSystem::GetCatalog()->NumericType(treeType);
  tree = BTree::Open(valueRecord,offset,treeNumType);
  return (tree != 0);
}

/*
1.1 ~saveRelation~

*/

bool saveRelation(const string descriptor, Relation* rel,
                  SmiRecord& valueRecord, size_t& offset)
{
  ListExpr relType;
  nl->ReadFromString ( descriptor, relType );
  ListExpr relNumType =  SecondoSystem::GetCatalog()->NumericType ( relType );
  return rel->Save(valueRecord, offset, relNumType);
}

bool saveRelation(const string descriptor, OrderedRelation* rel,
                  SmiRecord& valueRecord, size_t& offset)
{
  ListExpr relType;
  nl->ReadFromString ( descriptor, relType );
  ListExpr relNumType = SecondoSystem::GetCatalog()->NumericType ( relType );
  Word wrel;
  wrel.setAddr(rel);
  bool ok = OrderedRelation::Save(valueRecord, offset, relNumType, wrel);
  return ok;
}


/*
1.1 ~saveBTree~

*/

bool saveBTree(const string descriptor, BTree* tree, SmiRecord& valueRecord,
               size_t& offset)
{
  ListExpr treeType;
  nl->ReadFromString ( descriptor, treeType );
  ListExpr treeNumericType =
      SecondoSystem::GetCatalog()->NumericType ( treeType );
  return tree->Save(valueRecord, offset, treeNumericType);
}

/*
1.1 ~comparePairJidDist~

*/

int comparePairJidDistByDist(const void* a, const void* b)
{
  pair<int,double>* ap = (pair<int,double>*) a;
  pair<int,double>* bp = (pair<int,double>*) b;
  if (ap->second < bp->second) return -1;
  if (ap->second > bp->second) return 1;
  if (ap->first < bp->first) return -1;
  if (ap->first > bp->first) return 1;
  return 0;
}

int comparePairJidDistById(const void* a, const void* b)
{
  pair<int,double>* ap = (pair<int,double>*) a;
  pair<int,double>* bp = (pair<int,double>*) b;
  if (ap->first < bp->first) return -1;
  if (ap->first > bp->first) return 1;
  if (ap->second < bp->second) return -1;
  if (ap->second > bp->second) return 1;
  return 0;
}

/*
1.1. reachedEndpoint

Checks if curPQElement endJID is enclosed in endJunctions list

*/

bool reachedEndpoint(const JPQEntry* actEntry,
                     const DbArray<pair<int, double> >* endJunctions,
                     double& dist)
{
  pair<int, double> curEntry;
  for (int i = 0; i < endJunctions->Size(); i++)
  {
    endJunctions->Get(i,curEntry);
    if (curEntry.first == actEntry->GetEndPartJID()){
      dist = curEntry.second;
      return true;
    }
  }
  return false;
}

/*
1.1 cleanSP

Cleans up memory at end of shortest path computation.

*/

void cleanShortestPathMemory(Direction* dir,
                             DbArray<pair<int, double> >* ej,
                             PQManagement* pq, Tuple* start, Tuple* end,
                             Tuple* cj, JPQEntry* jpq)
{
  if (dir != 0) dir->DeleteIfAllowed();
  if (ej != 0)
  {
    ej->Destroy();
    delete ej;
  }
  if (pq != 0)
  {
    pq->Destroy();
    delete pq;
  }
  if (start != 0) start->DeleteIfAllowed();
  if (end != 0) end->DeleteIfAllowed();
  if (cj != 0) cj->DeleteIfAllowed();
  if (jpq != 0) delete jpq;
}

/*
1 Implementation of class JNetwork

1.1 Constructors and Deconstructor

*/

JNetwork::JNetwork()
{}

JNetwork::JNetwork(const bool def) :
  defined(def), junctions(0), sections(0), routes(0), netdistances(0),
  junctionsBTree(0), junctionsRTree(0), sectionsBTree(0),sectionsRTree(0),
  routesBTree(0)
{}

JNetwork::JNetwork(const string nid, const Relation* injunctions,
                   const Relation* insections, const Relation* inroutes) :
  defined(true),
  junctions(getRelationCopy(junctionsRelationTypeInfo, injunctions)),
  sections(getRelationCopy(sectionsRelationTypeInfo, insections)),
  routes(getRelationCopy(routesRelationTypeInfo, inroutes)),
  netdistances(0), junctionsBTree(0), junctionsRTree(0), sectionsBTree(0),
  sectionsRTree(0), routesBTree(0)
{
  strcpy(id, nid.c_str());
  InitNetdistances();
  CreateTrees();
}

JNetwork::JNetwork(const string nid, const Relation* injunctions,
                   const Relation* insections, const Relation* inroutes,
                   const OrderedRelation* inDist) :
  defined(true),
  junctions(getRelationCopy(junctionsRelationTypeInfo, injunctions)),
  sections(getRelationCopy(sectionsRelationTypeInfo, insections)),
  routes(getRelationCopy(routesRelationTypeInfo, inroutes)),
  netdistances(getRelationCopy(netdistancesRelationTypeInfo, inDist)),
  junctionsBTree(0), junctionsRTree(0), sectionsBTree(0),
  sectionsRTree(0), routesBTree(0)
{
  strcpy(id, nid.c_str());
  CreateTrees();
}


JNetwork::JNetwork(SmiRecord& valueRecord, size_t& offset,
                   const ListExpr typeInfo) :
  defined(false), junctions(0), sections(0), routes(0), netdistances(0),
  junctionsBTree(0), junctionsRTree(0), sectionsBTree(0),sectionsRTree(0),
  routesBTree(0)
{
  Word w;
  ListExpr idLE;
  nl->ReadFromString(CcString::BasicType(), idLE);
  ListExpr numId = SecondoSystem::GetCatalog()->NumericType(idLE);
  bool ok = OpenAttribute<CcString>(valueRecord, offset, numId, w);

  if (ok)
  {
    CcString* stn = (CcString*)w.addr;
    strcpy(id, stn->GetValue().c_str());
    stn->DeleteIfAllowed();
  }


  if (ok && id == Symbol::UNDEFINED())
  {
    ok = false;
  }

  if (ok)
    ok = openRelation(junctions, junctionsRelationTypeInfo, valueRecord,
                      offset);

  if (ok)
    ok = openRelation(sections, sectionsRelationTypeInfo, valueRecord, offset);

  if (ok)
    ok = openRelation(routes, routesRelationTypeInfo, valueRecord, offset);

  if (ok)
    ok = openRelation(netdistances, netdistancesRelationTypeInfo, valueRecord,
                      offset);

  if (ok)
    ok = openBTree(junctionsBTree, junctionsBTreeTypeInfo, valueRecord,
                   offset);

  if (ok)
    ok = openBTree(sectionsBTree, sectionsBTreeTypeInfo, valueRecord, offset);

  if (ok)
    ok = openBTree(routesBTree, routesBTreeTypeInfo, valueRecord, offset);

  Word wTree;

  if (ok)
    ok = junctionsRTree->Open(valueRecord, offset, junctionsRTreeTypeInfo,
                              wTree);

  if (ok)
    junctionsRTree = (R_Tree<2,TupleId>*) wTree.addr;

  if (ok)
    ok = sectionsRTree->Open(valueRecord, offset, sectionsRTreeTypeInfo, wTree);

  if (ok)
    sectionsRTree = (R_Tree<2,TupleId>*) wTree.addr;

  defined = ok;

  if (!ok) Destroy();
}


JNetwork::~JNetwork()
{
  delete junctions;
  delete sections;
  delete routes;
  delete netdistances;
  delete junctionsBTree;
  delete junctionsRTree;
  delete sectionsBTree;
  delete sectionsRTree;
  delete routesBTree;
}

void JNetwork::Destroy()
{
  if (junctions != 0)
  {
    junctions->Delete();
    junctions = 0;
  }

  if (sections != 0)
  {
    sections->Delete();
    sections = 0;
  }

  if (routes != 0)
  {
    routes->Delete();
    sections = 0;
  }

  if (netdistances != 0)
  {
    netdistances->Clear();
    netdistances = 0;
  }

  if (junctionsBTree != 0)
  {
    junctionsBTree->DeleteFile();
    junctionsBTree = 0;
  }

  if (sectionsBTree != 0)
  {
    sectionsBTree->DeleteFile();
    sectionsBTree = 0;
  }

  if (routesBTree != 0)
  {
    routesBTree->DeleteFile();
    routesBTree = 0;
  }

  if (junctionsRTree != 0)
  {
    junctionsRTree->DeleteFile();
    junctionsRTree = 0;
  }

  if (sectionsRTree != 0)
  {
    sectionsRTree->DeleteFile();
    sectionsRTree = 0;
  }

  SetDefined(false);
}

/*
1.1 Get Network Data

*/

bool JNetwork::IsDefined() const
{
  return defined;
}

const STRING_T* JNetwork::GetId() const
{
  return &id;
}

string JNetwork::GetJunctionsRelationType()
{
  return junctionsRelationTypeInfo;
}

string JNetwork::GetSectionsRelationType()
{
  return sectionsRelationTypeInfo;
}

string JNetwork::GetRoutesRelationType()
{
  return routesRelationTypeInfo;
}
string JNetwork::GetNetdistancesRelationType()
{
  return netdistancesRelationTypeInfo;
}

Relation* JNetwork::GetJunctionsCopy() const
{
  return junctions->Clone();
}

Relation* JNetwork::GetRoutesCopy() const
{
  return routes->Clone();
}

Relation* JNetwork::GetSectionsCopy() const
{
  return sections->Clone();
}

OrderedRelation* JNetwork::GetNedistancesRelationCopy() const
{
  return getRelationCopy(netdistancesRelationTypeInfo,
                         netdistances);
}

void JNetwork::SetDefined(const bool def)
{
  defined = def;
}

/*
1.1 Secondo Integration

*/

ListExpr JNetwork::Out(ListExpr typeInfo, Word value)
{
  JNetwork* source = (JNetwork*) value.addr;

  if (source == 0 || !source->IsDefined())
  {
    return nl->SymbolAtom(Symbol::UNDEFINED());
  }
  else
  {
    ListExpr netId = nl->StringAtom(*source->GetId());
    ListExpr junclist = source->JunctionsToList();
    ListExpr sectlist = source->SectionsToList();
    ListExpr routelist = source->RoutesToList();
    ListExpr dislist = source->NetdistancesToList();
    return nl->FiveElemList(netId, junclist, sectlist, routelist, dislist);
  }
}

Word JNetwork::In(const ListExpr typeInfo, const ListExpr instance,
                  const int errorPos, ListExpr& errorInfo, bool& correct)
{
  correct = true;
  if(nl->ListLength(instance) == 1)
  {
    if (nl->IsAtom(instance) &&
        nl->SymbolValue(instance) == Symbol::UNDEFINED())
    {
       correct = true;
       JNetwork* n = new JNetwork(false);
       return SetWord(n);
    }
  }
  else
  {
    if (nl->ListLength(instance) == 5)
    {
      ListExpr netId = nl->First(instance);
      string nid = nl->StringValue(netId);

      Relation* juncRel = relationFromList(nl->Second(instance),
                                           junctionsRelationTypeInfo,
                                           errorPos, errorInfo, correct);
      Relation* sectRel = relationFromList(nl->Third(instance),
                                           sectionsRelationTypeInfo,
                                           errorPos, errorInfo, correct);
      Relation* routeRel = relationFromList(nl->Fourth(instance),
                                            routesRelationTypeInfo,
                                            errorPos, errorInfo, correct);
      OrderedRelation* distRel =
        ordRelationFromList(nl->Fifth(instance), netdistancesRelationTypeInfo,
                            errorPos, errorInfo, correct);
      if (!correct){
        if (juncRel != 0) juncRel->Delete();
        if (sectRel != 0) sectRel->Delete();
        if (routeRel != 0) routeRel->Delete();
        if (distRel != 0) {
          distRel->Clear();
          delete distRel;
        }
        return SetWord(Address(0));
      }

      JNetwork* n = new JNetwork(nid, juncRel, sectRel, routeRel, distRel);

      juncRel->Delete();
      sectRel->Delete();
      routeRel->Delete();
      distRel->Clear();
      delete distRel;
      return SetWord(n);
    }
  }
  correct = false;
  return SetWord(Address(0));
}

Word JNetwork::Create(const ListExpr typeInfo)
{
  return SetWord ( new JNetwork(false));
}

void JNetwork::Delete( const ListExpr typeInfo, Word& w )
{
  JNetwork* net = (JNetwork*) w.addr;
  delete net;
  w.addr = 0;
}

void JNetwork::Close( const ListExpr typeInfo, Word& w )
{
  delete static_cast<JNetwork*> ( w.addr );
  w.addr = 0;
}

Word JNetwork::Clone( const ListExpr typeInfo, const Word& w )
{
  JNetwork* source = (JNetwork*) w.addr;
  JNetwork* clone = new JNetwork(*source->GetId(),
                                 source->junctions,
                                 source->sections,
                                 source->routes,
                                 source->netdistances);
  return SetWord(clone);
}

void* JNetwork::Cast( void* addr )
{
  return (new (addr) JNetwork);
}

bool JNetwork::KindCheck ( ListExpr type, ListExpr& errorInfo )
{
  return nl->IsEqual(type, BasicType());
}

int JNetwork::SizeOf()
{
  return sizeof(JNetwork);
}

bool JNetwork::Save(SmiRecord& valueRecord, size_t& offset,
                    const ListExpr typeInfo, Word& value)
{
  JNetwork* source = (JNetwork*) value.addr;
  if (source->IsDefined())
  {
    return source->Save(valueRecord, offset, typeInfo);
  }
  else
  {
    Word w;
    w.setAddr(new CcString(true,Symbol::UNDEFINED()));
    ListExpr idLE;
    nl->ReadFromString(CcString::BasicType(), idLE);
    ListExpr numId = SecondoSystem::GetCatalog()->NumericType(idLE);
    return SaveAttribute<CcString>(valueRecord, offset, numId, w);
  }
}

bool JNetwork::Save(SmiRecord& valueRecord, size_t& offset,
                    const ListExpr  typeInfo)
{
  Word w;
  CcString* stn = new CcString(true, id);
  w.setAddr(stn);
  ListExpr idLE;
  nl->ReadFromString(CcString::BasicType(), idLE);
  ListExpr numId = SecondoSystem::GetCatalog()->NumericType(idLE);
  bool ok = SaveAttribute<CcString>(valueRecord, offset, numId, w);
  stn->DeleteIfAllowed();

  if (ok)
    ok = saveRelation(junctionsRelationTypeInfo, junctions, valueRecord,
                      offset);

  if (ok)
    ok = saveRelation(sectionsRelationTypeInfo, sections, valueRecord, offset);

  if (ok)
    ok = saveRelation(routesRelationTypeInfo, routes, valueRecord, offset);

  if (ok)
    ok = saveRelation(netdistancesRelationTypeInfo, netdistances, valueRecord,
                      offset);

  if (ok)
    ok = saveBTree(junctionsBTreeTypeInfo, junctionsBTree, valueRecord, offset);

  if (ok)
    ok = saveBTree(sectionsBTreeTypeInfo, sectionsBTree, valueRecord, offset);

  if (ok)
    ok = saveBTree(routesBTreeTypeInfo, routesBTree, valueRecord, offset);

  if (ok)
    ok = junctionsRTree->Save(valueRecord, offset);

  if (ok)
    ok = sectionsRTree->Save(valueRecord, offset);

  return ok;
}

bool JNetwork::Open(SmiRecord& valueRecord, size_t& offset,
                    const ListExpr typeInfo, Word& value )
{
  value.addr = new JNetwork(valueRecord, offset, typeInfo);
  return (value.addr != 0);
}

ListExpr JNetwork::Property()
{
  return nl->TwoElemList(
    nl->FourElemList(
      nl->StringAtom("Signature"),
      nl->StringAtom("Example Type List"),
      nl->StringAtom("List Rep"),
      nl->StringAtom("Example List")),
    nl->FourElemList(
      nl->StringAtom("-> " + Kind::JNETWORK()),
      nl->StringAtom(BasicType()),
      nl->TextAtom("(" + CcString::BasicType() + " " +
        junctionsRelationTypeInfo + " " + sectionsRelationTypeInfo + " " +
        routesRelationTypeInfo +" " + netdistancesRelationTypeInfo + "), the" +
        " string defines the name of the network, it is followed by the " +
        "network data for junctions, sections, routes and network distances "+
        "in nested list format."),
      nl->TextAtom("(netname junctionsrel sectionsrel routesrel distrel)")));
}

/*
1.1 Standard Operations

*/

ostream& JNetwork::Print(ostream& os) const
{
  os << "Network: ";
  if (!IsDefined())
    os << Symbol::UNDEFINED() << endl;
  else
  {
    os << "Id: " << id << endl;

    os << "Junctions: " << endl;
    if (junctions !=  0)junctions->Print(os);
    else os << "not defined" << endl;

    os << "Sections: " << endl;
    if (sections != 0) sections->Print(os);
    else os << "not defined" << endl;

    os << "Routes: " << endl;
    if (routes != 0) routes->Print(os);
    else os << "not defined" << endl;

    os << "Network Distances:" << endl;
    if (netdistances != 0) netdistances->Print(os);
    else os << "not defined" << endl;

  }
  os << endl;
  return os;
}

const string JNetwork::BasicType()
{
  return "jnet";
}

const bool JNetwork::checkType(const ListExpr type)
{
  return listutils::isSymbol(type, BasicType());
}

/*
1.1 Translation operations

*/

RouteLocation* JNetwork::GetNetworkValueOf(const Point* p) const
{
  RouteLocation* res = 0;
  double pos = 0.0;
  Tuple* actSect = GetSectionTupleFor(p, pos);
  if (actSect != 0)
  {
    JRouteInterval* actInt = GetSectionFirstRouteInterval(actSect);
    actSect->DeleteIfAllowed();
    if (actInt->GetSide().Compare((Direction) Down) != 0)
    {
      res = new RouteLocation(actInt->GetRouteId(),
                              actInt->GetFirstPosition() + pos,
                              actInt->GetSide());
    }
    else
    {
      res = new RouteLocation(actInt->GetRouteId(),
                              actInt->GetFirstPosition() - pos,
                              actInt->GetSide());
    }
    actInt->DeleteIfAllowed();
  }
    return res;
}

JListRLoc* JNetwork::GetNetworkValuesOf(const Point* p) const
{
  JListRLoc* res = 0;
  double pos = 0.0;
  Tuple* actSect = GetSectionTupleFor(p, pos);
  if (AlmostEqual(pos, 0.0))
    res = GetSectionStartJunctionRLoc(actSect);
  else
  {
    if (AlmostEqual(pos, GetSectionLength(actSect)))
      res = GetSectionEndJunctionRLoc(actSect);
    else
    {
      res = new JListRLoc(0);
      res->StartBulkload();
      JListRInt* rints = GetSectionListRouteIntervals(actSect);
      JRouteInterval actInt;
      for(int i = 0; i < rints->GetNoOfComponents(); i++)
      {
        rints->Get(i,actInt);
        if (actInt.GetSide().Compare((Direction) Down) != 0)
        {
          res->operator+=(RouteLocation(actInt.GetRouteId(),
                                        actInt.GetFirstPosition() + pos,
                                        actInt.GetSide()));
        }
        else
        {
          res->operator+=(RouteLocation(actInt.GetRouteId(),
                                        actInt.GetFirstPosition() - pos,
                                        actInt.GetSide()));
        }
      }
      res->EndBulkload();
      rints->Destroy();
      rints->DeleteIfAllowed();
    }
  }
  actSect->DeleteIfAllowed();
  return res;
}

JListRLoc* JNetwork::GetNetworkValuesOf(const Tuple* actSect,
                                        const double distStart) const
{
  JListRLoc* res = 0;
  if (AlmostEqual(distStart, 0.0))
    res = GetSectionStartJunctionRLoc(actSect);
  else
  {
    if (AlmostEqual(distStart, GetSectionLength(actSect)))
      res = GetSectionEndJunctionRLoc(actSect);
    else
    {
      if (distStart > 0.0 && distStart < GetSectionLength(actSect))
      {
        JListRInt* rintList = GetSectionListRouteIntervals(actSect);
        if (rintList->IsDefined() && !rintList->IsEmpty())
        {
          res = new JListRLoc(true);
          res->StartBulkload();
          JRouteInterval  actInt;
          for (int i = 0; i < rintList->GetNoOfComponents(); i++)
          {
            rintList->Get(i, actInt);
            if (actInt.GetSide().Compare((Direction) Down) != 0)
            {
              res->operator+=(RouteLocation(actInt.GetRouteId(),
                                         actInt.GetFirstPosition() + distStart,
                                            actInt.GetSide()));
            }
            else
            {
              res->operator+=(RouteLocation(actInt.GetRouteId(),
                                         actInt.GetFirstPosition() - distStart,
                                            actInt.GetSide()));
            }
          }
          res->EndBulkload();
        }
      }
    }
  }
  return res;
}

JListRLoc* JNetwork::GetNetworkValuesOf(const RouteLocation& rloc) const
{
  double pos = 0.0;
  Tuple* actSect = GetSectionTupleFor(rloc,pos);
  JListRLoc* res = GetNetworkValuesOf(actSect,pos);
  actSect->DeleteIfAllowed();
  return res;
}

JRouteInterval* JNetwork::GetNetworkValueOf(const HalfSegment& hs) const
{
  JRouteInterval* res = 0;
  Point lp = hs.GetLeftPoint();
  Point rp = hs.GetRightPoint();
  JListRLoc* leftrlocs = GetNetworkValuesOf(&lp);
  JListRLoc* rightrlocs = GetNetworkValuesOf(&rp);
  res = GetRouteIntervalFor(leftrlocs, rightrlocs, true);
  rightrlocs->DeleteIfAllowed();
  leftrlocs->DeleteIfAllowed();
  return res;
}

Point* JNetwork::GetSpatialValueOf(const RouteLocation& rloc) const
{
  Point* res = 0;
  double relpos = 0.0;
  Tuple* sectTup = GetSectionTupleFor(rloc, relpos);
  res = GetSpatialValueOf(rloc, relpos, sectTup);
  sectTup->DeleteIfAllowed();
  return res;
}

Point* JNetwork::GetSpatialValueOf(const RouteLocation& rloc,
                                   const double relpos,
                                   const Tuple* actSect) const
{
  Point* res = 0;
  SimpleLine* sectCurve = GetSectionCurve(actSect);
  sectCurve->AtPosition(relpos, *res);
  sectCurve->DeleteIfAllowed();
  return res;
}

/*
1.1 Operations for other network data types

1.1.1 ~Contains~

Checks if the given position(s) exist in the network.

*/

bool JNetwork::Contains(const RouteLocation* rloc) const {
  return (rloc->GetPosition() <= GetRouteLength(rloc->GetRouteId()));
}

bool JNetwork::Contains(const JRouteInterval* rint) const{
  return (rint->GetFirstPosition() >= 0.0 &&
          rint->GetLastPosition()<= GetRouteLength(rint->GetRouteId()));
}

/*
1.1.1 ~SimulateTrip~

*/

MJPoint* JNetwork::SimulateTrip(const RouteLocation& source,
                                const RouteLocation& target,
                                const Point* targetPos,
                                const Instant& starttime,
                                const Instant& endtime,
                                const bool& lc, const bool& rc)
{
  MJPoint* res = new MJPoint(true);
  res->SetNetworkId(*this->GetId());
  double length = 0.0;
  DbArray<JRouteInterval>* sp = ShortestPath(source, target, targetPos, length);
  if (sp->Size() > 0)
  {
    JRouteInterval actInt;
    double duration = (endtime - starttime).ToDouble();
    Instant unitstart = starttime;
    bool unitlc = lc;
    Instant unitend = unitstart;
    bool unitrc = rc;
    res->StartBulkload();
    for (int i = 0; i < sp->Size(); i++)
    {
      sp->Get(i,actInt);
      if (actInt.Contains(target))
      {
        unitend = endtime;
        unitrc = rc;
      }
      else
         unitend = unitstart + (actInt.GetLength()*duration/length);
      res->Add(JUnit(Interval<Instant>(unitstart, unitend, unitlc, unitrc),
                      actInt));
      unitstart = unitend;
      unitlc = !unitrc;
      unitrc = !unitlc;
      unitend = unitstart;
    }
    res->EndBulkload();
  }
  sp->Destroy();
  delete sp;
  return res;
}

/*
1.1.1 ShortestPath

*/

DbArray<JRouteInterval>* JNetwork::ShortestPath(const RouteLocation& source,
                                                const RouteLocation& target,
                                                const Point* targetPos,
                                                double& length)
{
  DbArray<JRouteInterval>* res = new DbArray<JRouteInterval> (0);
  length = 0.0;
  double distSourceStartSect, distTargetStartSect;
  Tuple* startSectTup = GetSectionTupleFor(source, distSourceStartSect);
  Tuple* endSectTup = GetSectionTupleFor(target, distTargetStartSect);
  int startSectId = GetSectionId(startSectTup);
  int endSectId = GetSectionId(endSectTup);
  RouteLocation src(source);
  RouteLocation tgt(target);
  if ((startSectId == endSectId || ExistsCommonRoute(src, tgt)) &&
       DirectConnectionExists(startSectId, endSectId, startSectTup, endSectTup,
                              src, tgt, res, length))
  {
    //Special case computation already finished.
    startSectTup->DeleteIfAllowed();
    endSectTup->DeleteIfAllowed();
    return res;
  }
  Direction* curSectDir = 0;
  DbArray<pair<int, double> >* endJunctions =
      new DbArray<pair<int, double> >(0);
  //compute (set) of junctions where computation ends
  if (AlmostEqual(distTargetStartSect, 0.0) ||
      AlmostEqual(distTargetStartSect, GetSectionLength(endSectTup)))
  {
    //end is junction
    if (AlmostEqual(distTargetStartSect, 0.0))
      endJunctions->Append(make_pair<int, double>(
        GetSectionStartJunctionID(endSectTup), 0.0));
    else
      endJunctions->Append(make_pair<int, double>(
        GetSectionEndJunctionID(endSectTup), 0.0));
  }
  else
  {
    //end inside section.
    //check possible moving direction on end section
    curSectDir = GetSectionDirection(endSectTup);
    if (curSectDir->Compare((Direction) Down) != 0)
      endJunctions->Append(
        make_pair<int, double> (GetSectionStartJunctionID(endSectTup),
                                distTargetStartSect));
    if (curSectDir->Compare((Direction) Up) != 0)
      endJunctions->Append(
        make_pair<int, double> (GetSectionEndJunctionID(endSectTup),
                    fabs(GetSectionLength(endSectTup) - distTargetStartSect)));
  }
  PQManagement* pq = new PQManagement(0);
  JPQEntry* pqEntry = 0;
  int startPathJID = -1;
  int endPathJID = -1;
  Tuple* curJunc = 0;
  //init priority queue with starting intervals
  if (AlmostEqual(distSourceStartSect, 0.0) ||
      AlmostEqual(distSourceStartSect, GetSectionLength(startSectTup)))
  {
    //start on junction
    if (AlmostEqual(distSourceStartSect, 0.0))
    {
      curSectDir = new Direction(Down);
      curJunc = GetSectionStartJunctionTuple(startSectTup);
      startPathJID = GetJunctionId(curJunc);
    }
    else
    {
      curSectDir = new Direction(Up);
      curJunc = GetSectionEndJunctionTuple(startSectTup);
      startPathJID = GetJunctionId(curJunc);
    }
    if (ExistsNetworkdistanceFor(startPathJID, endJunctions, endPathJID))
    {
      WriteShortestPath(source,  distSourceStartSect, target,
                        distTargetStartSect, startSectTup, endSectTup,
                        startPathJID, endPathJID, res, length);
      cleanShortestPathMemory( curSectDir, endJunctions, pq,
                              startSectTup, endSectTup, curJunc, pqEntry);
      return res;
    }
    else
    {
      JListInt* curAdjSec = GetJunctionOutSectionList(curJunc);
      if (pqEntry != 0) delete pqEntry;
      pqEntry = new JPQEntry(*curSectDir, -1, startPathJID, -1, -1,
                             startPathJID, -1, 0.0, 0.0);
      AddAdjacentSections(pq, curAdjSec, *pqEntry, targetPos);
      delete pqEntry;
      curAdjSec->Destroy();
      curAdjSec->DeleteIfAllowed();
    }
  }
  else
  {
    //start within section insert route intervals to the end junction(s) of
    //the section reachable from start.
    //check possible moving direction
    curSectDir = GetSectionDirection(startSectTup);
    JListInt* curAdjSec = 0;
    if (curSectDir->Compare((Direction) Down) != 0)
    {
      //curSectDir Up or Both use end junction for inital entries in pq
      curJunc = GetSectionEndJunctionTuple(startSectTup);
      startPathJID = GetJunctionId(curJunc);
      if (ExistsNetworkdistanceFor(startPathJID, endJunctions, endPathJID))
      {
        WriteShortestPath(source, distSourceStartSect,target,
                          distTargetStartSect, startSectTup, endSectTup,
                          startPathJID, endPathJID, res, length);
        cleanShortestPathMemory( curSectDir, endJunctions, pq,
                                startSectTup, endSectTup, curJunc, pqEntry);
        return res;
      }
      else
      {
        curAdjSec = GetSectionListAdjSectionsUp(startSectTup);
        if (pqEntry != 0) delete pqEntry;
        pqEntry = new JPQEntry((Direction) Up, startSectId, startPathJID, -1,
                               -1, startPathJID, -1,
                               fabs(GetSectionLength(startSectTup) -
                                 distSourceStartSect),
                               fabs(GetSectionLength(startSectTup) -
                                 distSourceStartSect));
        AddAdjacentSections(pq, curAdjSec, *pqEntry, targetPos);
        delete pqEntry;
        curAdjSec->Destroy();
        curAdjSec->DeleteIfAllowed();
      }
    }
    if (curSectDir->Compare((Direction) Up) != 0)
    {
      //curSectDir Down or Both use start junction for inital entries in pq
      curJunc = GetSectionStartJunctionTuple(startSectTup);
      startPathJID = GetJunctionId(curJunc);
      if (ExistsNetworkdistanceFor(startPathJID, endJunctions, endPathJID))
      {
        WriteShortestPath(source, distSourceStartSect, target,
                          distTargetStartSect, startSectTup, endSectTup,
                          startPathJID, endPathJID, res, length);
        cleanShortestPathMemory( curSectDir, endJunctions, pq,
                                startSectTup, endSectTup, curJunc, pqEntry);
        return res;
      }
      else
      {
        curAdjSec = GetSectionListAdjSectionsUp(startSectTup);
        if (pqEntry != 0) delete pqEntry;
        pqEntry = new  JPQEntry((Direction) Down, -1, startPathJID, -1, -1,
                                startPathJID, -1, distSourceStartSect,
                                distSourceStartSect);
        AddAdjacentSections(pq, curAdjSec, *pqEntry, targetPos);
        delete pqEntry;
        curAdjSec->Destroy();
        curAdjSec->DeleteIfAllowed();
      }
    }
  }
  JPQEntry* curPQElement = 0;
  bool found = false;
  double minDist = numeric_limits< double >::max();
  //process priority queue
  while (!found && !pq->IsEmpty())
  {
    if (curPQElement != 0)
      delete curPQElement;
    curPQElement = pq->GetAndDeleteMin();
    InsertNetdistanceTuple(startPathJID, curPQElement);
    double distLastJuncEndPoint;
    if (reachedEndpoint(curPQElement, endJunctions, distLastJuncEndPoint))
    {
      found = true;
      minDist =
        curPQElement->GetDistFromStart() + distLastJuncEndPoint;
      //check if alternative end is possible
      if (endJunctions->Size() > 1)
      {
        // might exist shorter path over other end Junction.
        bool testedOtherEnd = false;
        JPQEntry* test = 0;
        while (!testedOtherEnd && !pq->IsEmpty())
        {
           if (test != 0)
              delete test;
          test = pq->GetAndDeleteMin();
          InsertNetdistanceTuple(startPathJID, test);
          if (test->GetPriority() < minDist)
          {
            double testDistLastJuncEndPoint;
            if (reachedEndpoint(test, endJunctions, testDistLastJuncEndPoint))
            {
              if (minDist > test->GetDistFromStart() +
                testDistLastJuncEndPoint)
              {
                delete curPQElement;
                curPQElement = test;
                minDist = test->GetDistFromStart() + testDistLastJuncEndPoint;
              }
            }
            else
              AddAdjacentSections(pq, *test, targetPos);
          }
          else
            testedOtherEnd = true;
        }
      }
      if (ExistsNetworkdistanceFor(startPathJID, endJunctions, endPathJID))
      {
        WriteShortestPath(source, distSourceStartSect, target,
                          distTargetStartSect, startSectTup, endSectTup,
                          startPathJID, endPathJID, res, length);
        cleanShortestPathMemory( curSectDir, endJunctions, pq,
                                startSectTup, endSectTup, curJunc, pqEntry);
        return res;
      }
    }
    else
      AddAdjacentSections(pq, *curPQElement, targetPos);
  }
  cleanShortestPathMemory( curSectDir, endJunctions, pq,
                          startSectTup, endSectTup, curJunc, pqEntry);
  return res;
}

/*
1.1 Relation Descriptors

*/

string JNetwork::junctionsTupleTypeInfo = Tuple::BasicType() + "((Id " +
  CcInt::BasicType() + ") (Pos " + Point::BasicType() + ") (Listjuncpos " +
  JListRLoc::BasicType() + ") (Listinsections " + JListInt::BasicType() +
  ") (Listoutsections " + JListInt::BasicType() + "))";

string JNetwork::sectionsTupleTypeInfo =  Tuple::BasicType() + "((Id " +
  CcInt::BasicType() + ") (Curve " + SimpleLine::BasicType() +
  ") (StartJunctionId " + CcInt::BasicType() + ") (EndJunctionId " +
  CcInt::BasicType() + ") (Direction " + Direction::BasicType() +
  ") (VMax " + CcReal::BasicType() + ") (Lenth " + CcReal::BasicType() +
  ") (ListSectRouteIntervals " +  JListRInt::BasicType() +
  ") (ListAdjSectUp " + JListInt::BasicType() +
  ") (ListAdjSectDown " + JListInt::BasicType() + ") (ListRevAdjSectUp " +
  JListInt::BasicType() + ") (ListRevAdjSectDown " + JListInt::BasicType() +
  "))";

string JNetwork::routesTupleTypeInfo = Tuple::BasicType() + "((Id " +
  CcInt::BasicType() + ") (ListJunctions " + JListInt::BasicType() +
  ") (ListSections " + JListInt::BasicType() + ") (Lenth " +
  CcReal::BasicType() + "))";

string JNetwork::junctionsRelationTypeInfo = "("+ Relation::BasicType() + "(" +
  junctionsTupleTypeInfo + "))";

string JNetwork::sectionsRelationTypeInfo = "("+ Relation::BasicType() + "(" +
  sectionsTupleTypeInfo + "))";

string JNetwork::routesRelationTypeInfo = "("+ Relation::BasicType() + "(" +
  routesTupleTypeInfo + "))";

string JNetwork::netdistancesRelationTypeInfo = "("+
  OrderedRelation::BasicType() + "("+ Tuple::BasicType() +
  "((Source " + CcInt::BasicType() + ")(Target " + CcInt::BasicType() +
  ")(NextJunct " + CcInt::BasicType() + ")(ViaSect " + CcInt::BasicType() +
  ")(NetDist " + CcReal::BasicType() + "))) (Source Target))";

string JNetwork::junctionsBTreeTypeInfo = "("+ BTree::BasicType() + "(" +
  junctionsTupleTypeInfo + ")" + CcInt::BasicType() + ")";

string JNetwork::junctionsRTreeTypeInfo = "("+ R_Tree<2,TupleId>::BasicType() +
  "(" + junctionsTupleTypeInfo + ")" + Point::BasicType() + " FALSE)";

string JNetwork::sectionsBTreeTypeInfo = "("+ BTree::BasicType() + "(" +
  sectionsTupleTypeInfo + ")" + CcInt::BasicType() + ")";

string JNetwork::sectionsRTreeTypeInfo = "("+ R_Tree<2, TupleId>::BasicType() +
 "(" + sectionsTupleTypeInfo + ")" + SimpleLine::BasicType() + " FALSE)";

string JNetwork::routesBTreeTypeInfo = "("+ BTree::BasicType() + "(" +
  routesTupleTypeInfo + ")" + CcInt::BasicType() + ")";


/*
1.1 Private functions

1.1.1 Build ListExpr for Out-Function of Network

*/

ListExpr JNetwork::JunctionsToList() const
{
  return relationToList(junctions, junctionsRelationTypeInfo);
}

ListExpr JNetwork::SectionsToList() const
{
  return relationToList(sections, sectionsRelationTypeInfo);
}

ListExpr JNetwork::RoutesToList() const
{
  return relationToList(routes, routesRelationTypeInfo);
}

ListExpr JNetwork::NetdistancesToList() const
{
  return relationToList(netdistances, netdistancesRelationTypeInfo);
}


/*
1.1 Creates Trees

*/

void JNetwork::CreateTrees()
{
  junctionsBTree = createBTree(junctions, junctionsRelationTypeInfo, "Id");
  junctionsRTree = createRTree(junctions, junctionsRelationTypeInfo, "Pos");
  sectionsBTree = createBTree(sections, sectionsRelationTypeInfo, "Id");
  sectionsRTree = createRTree(sections, sectionsRelationTypeInfo, "Curve");
  routesBTree = createBTree(routes, routesRelationTypeInfo,"Id");
}

/*
1.1 Initialize netdistance ordered relation

*/

void JNetwork::InitNetdistances()
{
  ListExpr relType;
  nl->ReadFromString(netdistancesRelationTypeInfo, relType);
  ListExpr relNumType = SecondoSystem::GetCatalog()->NumericType ( relType );
  netdistances = new OrderedRelation(relNumType);
  GenericRelationIterator* it = sections->MakeScan();
  Tuple* actTuple = 0;
  while ((actTuple = it->GetNextTuple()) != 0)
  {
    Direction* sectDir = GetSectionDirection(actTuple);
    if (sectDir->Compare((Direction) Down) != 0)
    {
      InsertNetdistanceTuple(
        ((CcInt*)actTuple->GetAttribute(SEC_STARTNODE_ID))->GetIntval(),
        ((CcInt*)actTuple->GetAttribute(SEC_ENDNODE_ID))->GetIntval(),
        ((CcInt*)actTuple->GetAttribute(SEC_ENDNODE_ID))->GetIntval(),
        ((CcInt*)actTuple->GetAttribute(SEC_ID))->GetIntval(),
        ((CcReal*)actTuple->GetAttribute(SEC_LENGTH))->GetRealval());
    }
    if (sectDir->Compare((Direction) Up) != 0)
    {
      InsertNetdistanceTuple(
        ((CcInt*)actTuple->GetAttribute(SEC_ENDNODE_ID))->GetIntval(),
        ((CcInt*)actTuple->GetAttribute(SEC_STARTNODE_ID))->GetIntval(),
        ((CcInt*)actTuple->GetAttribute(SEC_STARTNODE_ID))->GetIntval(),
        ((CcInt*)actTuple->GetAttribute(SEC_ID))->GetIntval(),
        ((CcReal*)actTuple->GetAttribute(SEC_LENGTH))->GetRealval());
    }
    actTuple->DeleteIfAllowed();
    actTuple = 0;
    sectDir->DeleteIfAllowed();
  }
  delete it;
  it = junctions->MakeScan();
  while ((actTuple = it->GetNextTuple()) != 0)
  {
    InsertNetdistanceTuple(
      ((CcInt*)actTuple->GetAttribute(JUNC_ID))->GetIntval(),
      ((CcInt*)actTuple->GetAttribute(JUNC_ID))->GetIntval(),
      ((CcInt*)actTuple->GetAttribute(JUNC_ID))->GetIntval(),
      -1, 0.0);
    actTuple->DeleteIfAllowed();
    actTuple = 0;
  }
  delete it;
}

void JNetwork::InsertNetdistanceTuple(const int fromjid, const JPQEntry* jp)
{
  InsertNetdistanceTuple(fromjid,jp->GetEndPartJID(),jp->GetStartNextJID(),
                         jp->GetStartNextSID(), jp->GetDistFromStart());
}

void JNetwork::InsertNetdistanceTuple(const int fromjid, const int tojid,
                                      const int viajid, const int viasid,
                                      const double dist)
{
  ListExpr relType;
  nl->ReadFromString(netdistancesRelationTypeInfo, relType);
  ListExpr relNumType = SecondoSystem::GetCatalog()->NumericType ( relType );
  Tuple* insertTuple = 0;
  insertTuple = new Tuple(nl->Second(relNumType));
  insertTuple->PutAttribute(NETDIST_FROM_JID, new CcInt(true, fromjid));
  insertTuple->PutAttribute(NETDIST_TO_JID, new CcInt(true, tojid));
  insertTuple->PutAttribute(NETDIST_NEXT_JID, new CcInt(true, viajid));
  insertTuple->PutAttribute(NETDIST_NEXT_SID, new CcInt(true, viasid));
  insertTuple->PutAttribute(NETDIST_DIST, new CcReal(true, dist));
  netdistances->AppendTuple(insertTuple);
  insertTuple->DeleteIfAllowed();
}

/*
1.1 Get Tuples by identifier

*/

Tuple* JNetwork::GetRouteTupleWithId(const int rid) const
{
  return GetTupleWithId(routesBTree, routes, rid);
}

Tuple* JNetwork::GetSectionTupleWithId(const int sid) const
{
  return GetTupleWithId(sectionsBTree, sections, sid);
}

Tuple* JNetwork::GetJunctionTupleWithId(const int jid) const
{
  return GetTupleWithId(junctionsBTree, junctions, jid);
}

Tuple* JNetwork::GetTupleWithId(BTree* tree, const Relation* rel,
                                const int id) const
{
  CcInt* cid = new CcInt(true, id);
  Tuple* res = 0;
  BTreeIterator* it = tree->ExactMatch(cid);
  cid->DeleteIfAllowed();
  if (it->Next() != 0)
    res = rel->GetTuple(it->GetId(), false);
  delete it;
  return res;
}

Tuple* JNetwork::GetNetdistanceTupleFor(const int fid, const int tid) const
{
  vector<void*> attributes(2);
  vector<SmiKey::KeyDataType> kElems(2);
  SmiKey test((long int) 0);
  kElems[0] = test.GetType();
  kElems[1] = test.GetType();
  CcInt* from = new CcInt(true,fid);
  CcInt* to = new CcInt(true,tid);
  attributes[0] = from;
  attributes[1] = to;
  CompositeKey actKey(attributes,kElems,false);
  Tuple* res = netdistances->GetTuple(actKey);
  from->DeleteIfAllowed();
  to->DeleteIfAllowed();
  attributes.clear();
  kElems.clear();
  return res;
}


/*
1.1. Get Tuple by Spatial Position

*/

Tuple* JNetwork::GetSectionTupleFor(const Point* p, double& pos) const
{
  const Rectangle<2> pbox = p->BoundingBox();
  const Rectangle<2> searchbox(true,
                               pbox.MinD(0) - TOLERANCE,
                               pbox.MaxD(0) + TOLERANCE,
                               pbox.MinD(1) + TOLERANCE,
                               pbox.MaxD(1) + TOLERANCE);
  R_TreeLeafEntry<2,TupleId> curEntry;
  Tuple* actSect = 0;
  if (sectionsRTree->First(searchbox, curEntry))
    actSect = sections->GetTuple(curEntry.info, false);
  else
    return 0;
  double diff = numeric_limits< double >::max();
  double mindiff = numeric_limits< double >::max();
  TupleId minTupId = curEntry.info;
  bool found = false;
  SimpleLine* actCurve = 0;
  while (!found)
  {
    actCurve = GetSectionCurve(actSect);
    if (actCurve->AtPoint(*p, pos))
    {
      found = true;
    }
    else
    {
      diff = actCurve->Distance(p);
      if (diff < mindiff)
      {
        mindiff = diff;
        minTupId = curEntry.info;
      }
    }
    if (!found)
    {
      if (sectionsRTree->Next(curEntry))
      {
        actSect->DeleteIfAllowed();
        actSect = sections->GetTuple(curEntry.info, false);
        actCurve->DeleteIfAllowed();
      }
      else
      {
        break;
      }
    }
  }
  diff = mindiff;
  if (!found)
  {
    actSect->DeleteIfAllowed();
    actSect = sections->GetTuple(minTupId, false);
    actCurve = GetSectionCurve(actSect);
    HalfSegment hs;
    double k1, k2;
    Point left, right;
    for ( int i = 0; i < actCurve->Size()-1; i++ )
    {
      actCurve->Get ( i, hs );
      left = hs.GetLeftPoint();
      right = hs.GetRightPoint();
      Coord xl = left.GetX(),
            yl = left.GetY(),
            xr = right.GetX(),
            yr = right.GetY(),
             x = p->GetX(),
             y = p->GetY();
      if((AlmostEqualAbsolute(x, xl, TOLERANCE) &&
          AlmostEqualAbsolute(y, yl, TOLERANCE)) ||
         (AlmostEqualAbsolute(x, xr, TOLERANCE) &&
          AlmostEqualAbsolute(y, yr, TOLERANCE)))
      {
        diff = 0.0;
        found = true;
      }
      else
      {
        if ( xl != xr && xl != x )
        {
          k1 = ( y - yl ) / ( x - xl );
          k2 = ( yr - yl ) / ( xr - xl );
          if ((( xl < xr &&
                 ( x > xl || AlmostEqualAbsolute(x, xl, TOLERANCE)) &&
                 ( x < xr || AlmostEqualAbsolute(x, xr, TOLERANCE))) ||
               ( xl > xr &&
                 ( x < xl || AlmostEqualAbsolute(x, xl, TOLERANCE)) &&
                 ( x > xr || AlmostEqualAbsolute(x, xr, TOLERANCE)))) &&
              (( ( yl < yr || AlmostEqualAbsolute(yl, yr, TOLERANCE)) &&
                 ( y > yl || AlmostEqualAbsolute(y, yl, TOLERANCE)) &&
                 ( y < yr || AlmostEqualAbsolute(y, yr, TOLERANCE))) ||
               ( yl > yr &&
                 ( y < yl || AlmostEqualAbsolute(y, yl, TOLERANCE)) &&
                 ( y > yr || AlmostEqualAbsolute(y, yr, TOLERANCE)))))
          {
            diff = fabs ( k1-k2 );
            found = true;
          }
          else
          {
            found = false;
          }
        }
        else
        {
          if((AlmostEqualAbsolute(xl, xr, TOLERANCE) &&
              AlmostEqualAbsolute(xl, x, TOLERANCE)) &&
             ((( yl < yr || AlmostEqualAbsolute(yl, yr, TOLERANCE)) &&
               ( yl < y || AlmostEqualAbsolute(yl, y , TOLERANCE)) &&
               ( y < yr || AlmostEqualAbsolute(y, yr, TOLERANCE))) ||
              ( yl > yr  &&
               ( yl > y || AlmostEqualAbsolute(yl, y, TOLERANCE)) &&
               ( y > yr || AlmostEqualAbsolute(y, yr, TOLERANCE)))))
          {
            diff = 0.0;
            found = true;
          }
          else
          {
            found = false;
          }
        }
      }
      if ( found )
      {
        LRS lrs;
        actCurve->Get ( hs.attr.edgeno, lrs );
        actCurve->Get ( lrs.hsPos, hs );
        pos = lrs.lrsPos + p->Distance(hs.GetDomPoint());
        if(!actCurve->GetStartSmaller())
          pos = actCurve->Length() - pos;
        if ( pos < 0.0 || AlmostEqualAbsolute( pos, 0.0, TOLERANCE))
          pos = 0.0;
        else if ( pos > actCurve->Length() ||
          AlmostEqualAbsolute(pos, actCurve->Length(), TOLERANCE))
          pos = actCurve->Length();
      }
    }
  }
  actCurve->DeleteIfAllowed();
  return actSect;
}

Tuple* JNetwork::GetSectionTupleFor(const RouteLocation& rloc, double& relpos)
const
{
  Tuple* actSect = 0;
  JListInt* sectList = GetRouteSectionList(rloc.GetRouteId());
  if (sectList->IsDefined() && !sectList->IsEmpty())
  {
    int i = 0;
    CcInt actSid;
    while (i < sectList->GetNoOfComponents())
    {
      sectList->Get(i, actSid);
      if (actSect != 0) actSect->DeleteIfAllowed();
      actSect = GetSectionTupleWithId(actSid.GetIntval());
      JListRInt* rintList = GetSectionListRouteIntervals(actSect);
      if (rintList->IsDefined() && !rintList->IsEmpty())
      {
        JRouteInterval actInt;
        int j = 0;
        while (j < rintList->GetNoOfComponents())
        {
          rintList->Get(j,actInt);
          if (actInt.Contains(rloc))
          {
            relpos = fabs(rloc.GetPosition() - actInt.GetFirstPosition());
            j = rintList->GetNoOfComponents();
            i = sectList->GetNoOfComponents();
          }
          j++;
        }
      }
      rintList->Destroy();
      rintList->DeleteIfAllowed();
      i++;
    }
  }
  sectList->Destroy();
  sectList->DeleteIfAllowed();
  return actSect;
}


/*
1.1 Access to attributes of the relations

1.1.1 Routes Relation Attributes

*/

double JNetwork::GetRouteLength(const int rid) const
{
  Tuple* routeTup = GetRouteTupleWithId(rid);
  double res = GetRouteLength(routeTup);
  routeTup->DeleteIfAllowed();
  return res;
}

double JNetwork::GetRouteLength(const Tuple* routeTuple) const
{
  return ((CcReal*)routeTuple->GetAttribute(ROUTE_LENGTH))->GetRealval();
}

JListInt* JNetwork::GetRouteSectionList(const int rid) const
{
  Tuple* actRouteTup = GetRouteTupleWithId(rid);
  JListInt* res = GetRouteSectionList(actRouteTup);
  actRouteTup->DeleteIfAllowed();
  return res;
}

JListInt* JNetwork::GetRouteSectionList(const Tuple* actRoute) const
{
  return
    new JListInt(*((JListInt*) actRoute->GetAttribute(ROUTE_LIST_SECTIONS)));
}

/*
1.1.1 Sections Relation Attributes

*/

SimpleLine* JNetwork::GetSectionCurve(const int sid) const
{
  Tuple* actSect = GetSectionTupleWithId(sid);
  SimpleLine* res = GetSectionCurve(actSect);
  actSect->DeleteIfAllowed();
  return res;
}

SimpleLine* JNetwork::GetSectionCurve(const Tuple* sectTuple) const
{
  return new SimpleLine(*((SimpleLine*) sectTuple->GetAttribute(SEC_CURVE)));
}

SimpleLine* JNetwork::GetSectionCurve(const RouteLocation& rloc, double& relpos)
const
{
  Tuple* actSect = GetSectionTupleFor(rloc, relpos);
  SimpleLine* res = GetSectionCurve(actSect);
  actSect->DeleteIfAllowed();
  return res;
}

JListRInt* JNetwork::GetSectionListRouteIntervals(const int sectid) const
{
  Tuple* sectTup = GetSectionTupleWithId(sectid);
  JListRInt* res = GetSectionListRouteIntervals(sectTup);
  sectTup->DeleteIfAllowed();
  return res;
}

JListRInt* JNetwork::GetSectionListRouteIntervals(const Tuple* sectTuple) const
{
  return
    new JListRInt(*(
      (JListRInt*) sectTuple->GetAttribute(SEC_LIST_ROUTEINTERVALS)));
}

JRouteInterval* JNetwork::GetSectionFirstRouteInterval(const Tuple* sectTuple)
const
{
  JRouteInterval res;
  JListRInt* rintList = GetSectionListRouteIntervals(sectTuple);
  rintList->Get(0,res);
  rintList->DeleteIfAllowed();
  return new JRouteInterval(res);
}

double JNetwork::GetSectionLength(const Tuple* actSect) const
{
  return ((CcReal*)actSect->GetAttribute(SEC_LENGTH))->GetRealval();
}

Direction* JNetwork::GetSectionDirection(const Tuple* actSect) const
{
  return new Direction(*((Direction*)actSect->GetAttribute(SEC_DIRECTION)));
}

int JNetwork::GetSectionId(const Tuple* actSect) const
{
  return ((CcInt*)actSect->GetAttribute(SEC_ID))->GetIntval();
}

JRouteInterval* JNetwork::GetRouteIntervalFor(const JListRLoc* leftrlocs,
                                              const JListRLoc* rightrlocs,
                                              const bool allowResetSide) const
{
  JRouteInterval* res = 0;
  if (leftrlocs->IsDefined() && rightrlocs->IsDefined() &&
      !leftrlocs->IsEmpty() && !rightrlocs->IsEmpty())
  {
    int i = 0;
    int j = 0;
    while (i < leftrlocs->GetNoOfComponents() &&
           j < rightrlocs->GetNoOfComponents())
    {
      RouteLocation left, right;
      leftrlocs->Get(i,left);
      rightrlocs->Get(j,right);
      if (left.IsOnSameRoute(right))
      {
        res = new JRouteInterval(left, right, allowResetSide);
        i = leftrlocs->GetNoOfComponents();
        j = rightrlocs->GetNoOfComponents();
      }
      else
      {
        switch(left.Compare(right))
        {
          case -1:
          {
            i++;
            break;
          }

          case 1:
          {
            j++;
            break;
          }

          default:
          {//should never happen
            assert(false);
            i = leftrlocs->GetNoOfComponents();
            j = rightrlocs->GetNoOfComponents();
            break;
          }
        }
      }
    }
  }
  return res;
}

JRouteInterval* JNetwork::GetRouteIntervalFor(const RouteLocation& left,
                                              const RouteLocation& right,
                                              const bool allowResetSide) const
{
  JListRLoc* leftrlocs = GetNetworkValuesOf(left);
  JListRLoc* rightrlocs = GetNetworkValuesOf(right);
  JRouteInterval* res =
    GetRouteIntervalFor(leftrlocs, rightrlocs, allowResetSide);
  leftrlocs->Destroy();
  leftrlocs->DeleteIfAllowed();
  rightrlocs->Destroy();
  rightrlocs->DeleteIfAllowed();
  return res;
}

JListInt* JNetwork::GetSectionListAdjSectionsUp(const Tuple* sectTuple) const
{
  return new JListInt(*(
      (JListInt*)sectTuple->GetAttribute(SEC_LIST_ADJ_SECTIONS_UP)));
}

JListInt* JNetwork::GetSectionListAdjSectionsDown(const Tuple* sectTuple) const
{
  return new JListInt(*(
        (JListInt*)sectTuple->GetAttribute(SEC_LIST_ADJ_SECTIONS_DOWN)));
}

JListInt* JNetwork::GetSectionListReverseAdjSectionsUp(const Tuple* sectTuple)
  const
{
  return new JListInt(*(
        (JListInt*)sectTuple->GetAttribute(SEC_LIST_REV_ADJ_SECTIONS_UP)));
}

JListInt* JNetwork::GetSectionListReverseAdjSectionsDown(const Tuple* sectTuple)
  const
{
  return new JListInt(*(
     (JListInt*)sectTuple->GetAttribute(SEC_LIST_REV_ADJ_SECTIONS_DOWN)));
}

Tuple* JNetwork::GetSectionStartJunctionTuple(const Tuple* sectTuple) const
{
  return GetJunctionTupleWithId(
    ((CcInt*)sectTuple->GetAttribute(SEC_STARTNODE_ID))->GetIntval());
}

Tuple* JNetwork::GetSectionEndJunctionTuple(const Tuple* sectTuple) const
{
  return GetJunctionTupleWithId(
    ((CcInt*)sectTuple->GetAttribute(SEC_ENDNODE_ID))->GetIntval());
}

JListRLoc* JNetwork::GetSectionStartJunctionRLoc(const Tuple* sectTuple) const
{
  Tuple* juncTup = GetSectionStartJunctionTuple(sectTuple);
  JListRLoc* res =  GetJunctionListRLoc(juncTup);
  juncTup->DeleteIfAllowed();
  return res;
}

JListRLoc* JNetwork::GetSectionEndJunctionRLoc(const Tuple* sectTuple) const
{
  Tuple* juncTup = GetSectionEndJunctionTuple(sectTuple);
  JListRLoc* res = GetJunctionListRLoc(juncTup);
  juncTup->DeleteIfAllowed();
  return res;
}

int JNetwork::GetSectionStartJunctionID(const Tuple* sectTuple) const
{
  return ((CcInt*)sectTuple->GetAttribute(SEC_STARTNODE_ID))->GetIntval();
}

int JNetwork::GetSectionEndJunctionID(const Tuple* sectTuple) const
{
  return ((CcInt*)sectTuple->GetAttribute(SEC_ENDNODE_ID))->GetIntval();
}

/*
1.1.1 Juctions Attributes

*/

int JNetwork::GetJunctionId(const Tuple* juncTup) const
{
  return ((CcInt*)juncTup->GetAttribute(JUNC_ID))->GetIntval();
}

JListRLoc* JNetwork::GetJunctionListRLoc(const Tuple* juncTuple) const
{
  return
    new JListRLoc(*((JListRLoc*)
      juncTuple->GetAttribute(JUNC_LIST_ROUTEPOSITIONS)));
}

Point* JNetwork::GetJunctionSpatialPos(const Tuple* juncTuple) const
{
  return new Point(*((Point*) juncTuple->GetAttribute(JUNC_POS)));
}

JListInt* JNetwork::GetJunctionOutSectionList(const Tuple* juncTup) const
{
  return new JListInt(*(
    (JListInt*) juncTup->GetAttribute(JUNC_LIST_OUTSECTIONS)));
}

JListInt* JNetwork::GetJunctionInSectionList(const Tuple* juncTup) const
{
  return new JListInt(*(
    (JListInt*) juncTup->GetAttribute(JUNC_LIST_INSECTIONS)));
}

/*
1.1.1 Netdistance Relation

*/

int JNetwork::GetNetdistanceNextSID(const Tuple* actNetDistTup) const
{
  return ((CcInt*)actNetDistTup->GetAttribute(NETDIST_NEXT_SID))->GetIntval();
}

bool JNetwork::ExistsNetworkdistanceFor(const int startPathJID,
                              const DbArray<pair<int, double> >* endJunctions,
                              const int endPathJID) const
{
  pair<int, double> curEntry;
  for (int i = 0; i < endJunctions->Size();i++)
  {
    endJunctions->Get(i,curEntry);
    if (curEntry.first == endPathJID)
      return true;
  }
  return false;
}

/*
1.1 DirectConnectionExists

*/
bool JNetwork::DirectConnectionExists(const int startSID,
                                      const int endSID,
                                      const Tuple* sourceSectTup,
                                      const Tuple* targetSectTup,
                                      const RouteLocation& source,
                                      const RouteLocation& target,
                                      DbArray<JRouteInterval>* res,
                                      double& length) const
{
  Direction* sectDir = GetSectionDirection(sourceSectTup);
  Direction movDir(Up);
  if (source.GetPosition() > target.GetPosition())
    movDir.SetDirection((Direction) Down);
  if (startSID == endSID && sectDir->SameSide(movDir, false))
  {
    res->Append(JRouteInterval(source.GetRouteId(), source.GetPosition(),
                               target.GetPosition(), movDir));
    length = length + fabs(target.GetPosition()-source.GetPosition());
    sectDir->DeleteIfAllowed();
    return true;
  }
  else
  {
    JListInt* sectlist = GetRouteSectionList(source.GetRouteId());
    CcInt curSid;
    for (int i = 0; i < sectlist->GetNoOfComponents(); i++)
    {
      sectlist->Get(i,curSid);
      Tuple* actSect = GetSectionTupleWithId(curSid.GetIntval());
      JListRInt* sectRis = GetSectionListRouteIntervals(actSect);
      JRouteInterval actInt;
      int j = 0;
      while (j < sectRis->GetNoOfComponents())
      {
        sectRis->Get(j,actInt);
        if (actInt.GetRouteId() == source.GetRouteId() &&
            (actInt.Between(source,target) ||
             actInt.Contains(source) ||
             actInt.Contains(target)) &&
            !movDir.SameSide(actInt.GetSide(), false))
        {
          sectRis->Destroy();
          sectRis->DeleteIfAllowed();
          sectlist->Destroy();
          sectlist->DeleteIfAllowed();
          actSect->DeleteIfAllowed();
          sectDir->DeleteIfAllowed();
          return false;
        }
        j++;
      }
      sectRis->Destroy();
      sectRis->DeleteIfAllowed();
      actSect->DeleteIfAllowed();
    }
    sectlist->Destroy();
    sectlist->DeleteIfAllowed();
    sectDir->DeleteIfAllowed();
    res->Append(JRouteInterval(source.GetRouteId(), source.GetPosition(),
                               target.GetPosition(), movDir));
    length = length + fabs(target.GetPosition()-source.GetPosition());
    return true;
  }
  sectDir->DeleteIfAllowed();
  return false;
}

/*
1.1.1.1 AddAdjacentSections

*/

void JNetwork::AddAdjacentSections(PQManagement* pq, JPQEntry curEntry,
                                   const Point* targetPos)
{
  Tuple* pqSectTup = GetSectionTupleWithId(curEntry.GetSectionId());
  Direction movDir = curEntry.GetDirection();
  JListInt* listSID = 0;
  if (movDir.Compare((Direction) Up) == 0)
    listSID = GetSectionListAdjSectionsUp(pqSectTup);
  else
    listSID = GetSectionListAdjSectionsUp(pqSectTup);
  AddAdjacentSections(pq, listSID, curEntry, targetPos);
  listSID->Destroy();
  listSID->DeleteIfAllowed();
  pqSectTup->DeleteIfAllowed();
}

void JNetwork::AddAdjacentSections(PQManagement* pq, const JListInt* listSID,
                                   JPQEntry curEntry, const Point* targetPos)
{
  Tuple* curSectTup = 0;
  CcInt nextSID;
  for (int i = 0; i < listSID->GetNoOfComponents(); i++)
  {
    listSID->Get(i,nextSID);
    curSectTup = GetSectionTupleWithId(nextSID.GetIntval());
    int curSID = GetSectionId(curSectTup);
    if (curEntry.GetStartNextJID() < 0)
    {
      curEntry.SetStartNextSID(curSID);
      if (GetSectionEndJunctionID(curSectTup) == curEntry.GetStartPathJID())
      {
        curEntry.SetDirection((Direction) Down);
        curEntry.SetStartNextJID(GetSectionStartJunctionID(curSectTup));
        curEntry.SetStartPartJID(GetSectionEndJunctionID(curSectTup));
        curEntry.SetEndPartJID(GetSectionStartJunctionID(curSectTup));
      }
      else
      {
        curEntry.SetStartNextJID(GetSectionEndJunctionID(curSectTup));
        curEntry.SetStartPartJID(GetSectionStartJunctionID(curSectTup));
        curEntry.SetEndPartJID(GetSectionEndJunctionID(curSectTup));
      }

    }
    else
    {
      if (GetSectionEndJunctionID(curSectTup) == curEntry.GetEndPartJID())
      {
        curEntry.SetDirection((Direction) Down);
        curEntry.SetStartPartJID(GetSectionEndJunctionID(curSectTup));
        curEntry.SetEndPartJID(GetSectionStartJunctionID(curSectTup));
      }
      else
      {
        curEntry.SetStartPartJID(GetSectionStartJunctionID(curSectTup));
        curEntry.SetEndPartJID(GetSectionEndJunctionID(curSectTup));
      }
    }
    Tuple* curJunc = GetJunctionTupleWithId(curEntry.GetEndPartJID());
    Point* curEndPoint = GetJunctionSpatialPos(curJunc);
    double curDist =
      curEntry.GetDistFromStart() + GetSectionLength(curSectTup);
    double prio = curDist + curEndPoint->Distance(targetPos);
    pq->Insert(JPQEntry(curEntry.GetDirection(),
                        nextSID.GetIntval(),
                        curEntry.GetStartPathJID(),
                        curEntry.GetStartNextJID(),
                        curEntry.GetStartNextSID(),
                        curEntry.GetStartPartJID(),
                        curEntry.GetEndPartJID(),
                        curDist,
                        prio));
    curJunc->DeleteIfAllowed();
    curEndPoint->DeleteIfAllowed();
    curSectTup->DeleteIfAllowed();
  }
}

/*
1.1.1.1 WriteShortestPath

*/

void JNetwork::WriteShortestPath(const RouteLocation& source,
                                 const double distSourceStartSect,
                                 const RouteLocation& target,
                                 const double distTargetStartSect,
                                 const Tuple* startSectTup,
                                 const Tuple* endSectTup,
                                 const int startPathJID,
                                 const int endPathJID,
                                 DbArray< JRouteInterval >* result,
                                 double& length) const
{
  bool found = false;
  int curStartJID = startPathJID;
  JRouteInterval* curRint = 0;
  curRint = GetSectionFirstRouteInterval(startSectTup);
  //write first part of path
  if (startPathJID == GetSectionStartJunctionID(startSectTup))
  {
    curRint->SetSide((Direction) Down);
    curRint->SetInterval(curRint->GetFirstPosition(),
                         curRint->GetFirstPosition()+distSourceStartSect);
    result->Append(*curRint);
    length = length + distSourceStartSect;
    if (endPathJID == GetSectionEndJunctionID(startSectTup))
    {
      found = true;
      curStartJID = endPathJID;
    }
  }
  else
  {
    curRint->SetSide((Direction) Up);
    curRint->SetInterval(curRint->GetLastPosition() -
                  fabs(GetSectionLength(startSectTup)-distSourceStartSect),
                   curRint->GetLastPosition());
    result->Append(*curRint);
    length = length + fabs(GetSectionLength(startSectTup) -
          distSourceStartSect);
    if (endPathJID == GetSectionStartJunctionID(startSectTup))
    {
      found = true;
      curStartJID = endPathJID;
    }
  }
  Tuple* actNetDistTup = 0;
  Tuple* curSectTup = 0;
  //get path between start and end section
  while (!found)
  {
    if (actNetDistTup != 0)
      actNetDistTup->DeleteIfAllowed();
    actNetDistTup = GetNetdistanceTupleFor(curStartJID, endPathJID);
    if (curSectTup != 0)
      curSectTup->DeleteIfAllowed();
    curSectTup = GetSectionTupleWithId(GetNetdistanceNextSID(actNetDistTup));
    if (curRint != 0) curRint->DeleteIfAllowed();
    curRint = GetSectionFirstRouteInterval(curSectTup);
    if (curStartJID == GetSectionStartJunctionID(curSectTup))
    {
      curRint->SetSide((Direction) Up);
      curStartJID = GetSectionEndJunctionID(curSectTup);
    }
    else
    {
      curRint->SetSide((Direction) Down);
      curStartJID = GetSectionStartJunctionID(curSectTup);
    }
    result->Append(*curRint);
    length = length + GetSectionLength(curSectTup);
    if (endPathJID == curStartJID)
      found = true;
  }
  curRint = GetSectionFirstRouteInterval(endSectTup);
  //write last part
  if (endPathJID == GetSectionStartJunctionID(endSectTup))
  {
    curRint->SetSide((Direction) Up);
    curRint->SetInterval(curRint->GetFirstPosition(),
                         curRint->GetFirstPosition()+distTargetStartSect);
    result->Append(*curRint);
    length = length + distTargetStartSect;
  }
  else
  {
    curRint->SetSide((Direction) Down);
    curRint->SetInterval(curRint->GetLastPosition(),
                         curRint->GetLastPosition() -
                        fabs(GetSectionLength(endSectTup)-distTargetStartSect));
    result->Append(*curRint);
    length = length + fabs(GetSectionLength(endSectTup)-distTargetStartSect);
  }
  if (actNetDistTup != 0) actNetDistTup->DeleteIfAllowed();
  if (curSectTup != 0) curSectTup->DeleteIfAllowed();
  if (curRint != 0) curRint->DeleteIfAllowed();
}

/*
1.1.1 ExistsCommonRoute

*/

bool JNetwork::ExistsCommonRoute(RouteLocation& src, RouteLocation& tgt) const
{
  if (src.IsOnSameRoute(tgt)) return true;
  else
  {
    JListRLoc* left = GetNetworkValuesOf(src);
    JListRLoc* right = GetNetworkValuesOf(tgt);
    int i = 0;
    while (i < left->GetNoOfComponents())
    {
      left->Get(i,src);
      int j = 0;
      while (j < right->GetNoOfComponents())
      {
        right->Get(j,tgt);
        if (src.IsOnSameRoute(tgt))
        {
          left->Destroy();
          left->DeleteIfAllowed();
          right->Destroy();
          right->DeleteIfAllowed();
          return true;
        }
        j++;
      }
      i++;
    }
    left->Destroy();
    left->DeleteIfAllowed();
    right->Destroy();
    right->DeleteIfAllowed();
    return false;
  }
}


/*
1 Overwrite output operator

*/
ostream& operator<< (ostream& os, const JNetwork& n)
{
  n.Print(os);
  return os;
}