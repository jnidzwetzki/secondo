/*
//paragraph [1] Title: [{\Large \bf \begin{center}] [\end{center}}]
//paragraph [10] Footnote: [{\footnote{] [}}]
//[TOC] [\tableofcontents]

[1] Implementation of Module Relation Algebra Main Memory

[1] Using Storage Manager Berkeley DB

June 1996 Claudia Freundorfer

May 2002 Frank Hoffmann port to C++

November 7, 2002 RHG Corrected the type mapping of ~tcount~.

November 30, 2002 RHG Introduced a function ~RelPersistValue~ instead of
~DefaultPersistValue~ which keeps relations that have been built in memory in a
small cache, so that they need not be rebuilt from then on.

[TOC]

1 Includes, Constants, Globals, Enumerations

*/
using namespace std;

#include "Algebra.h"
#include "AlgebraManager.h"
#include "SecondoSystem.h"
#include "SecondoCatalog.h"
#include "NestedList.h"
#include "QueryProcessor.h"
#include "StandardTypes.h"
#include <iostream>
#include <string>
#include <deque>
#include <set>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>
#include <errno.h>
#include <sstream>
#include <typeinfo>
#include "RelationAlgebraInfo.h"
#include "Tuple.h"

static NestedList* nl;
static QueryProcessor* qp;

const int MaxSizeOfAttr = 10;

enum RelationType { rel, tuple, stream, ccmap, ccbool, error };
/*

2 Auxilary Functions

1.2 Function ~TypeOfRelAlgSymbol~

Transforms a list expression ~symbol~ into one of the values of
type ~RelationType~. ~Symbol~ is allowed to be any list. If it is not one
of these symbols, then the value ~error~ is returned.

*/
static RelationType TypeOfRelAlgSymbol (ListExpr symbol) {

  string s;

  if (nl->AtomType(symbol) == SymbolType)
  {
    s = nl->SymbolValue(symbol);
    if (s == "rel"   ) return rel;
    if (s == "tuple" ) return tuple;
    if (s == "stream") return stream;
    if (s == "map"   ) return ccmap;
    if (s == "bool"  ) return ccbool;
  }
  return error;
}
/*

5.6 Function ~findattr~

Here ~list~ should be a list of pairs of the form (~name~,~datatype~).
The function ~findattr~ determines whether ~attrname~ occurs as one of
the names in this list. If so, the index in the list (counting from 1)
is returned and the corresponding datatype is returned in ~attrtype~.
Otherwise 0 is returned. Used in operator ~attr~.

*/
int findattr( ListExpr list, string attrname, ListExpr& attrtype)
{
  ListExpr first, rest;
  int j;
  string  name;

  if (nl->IsAtom(list))
    return 0;
  rest = list;
  j = 1;
  while (!nl->IsEmpty(rest))
  {
    first = nl->First(rest);
    rest = nl->Rest(rest);
    if ((nl->ListLength(first) == 2) &&
       (nl->AtomType(nl->First(first)) == SymbolType))
    {
      name = nl->SymbolValue(nl->First(first));
      if (name == attrname)
      {
	    attrtype = nl->Second(first);
        return j;
      }
    }
    else
      return 0; // typeerror
    j++;
  }
  return 0; // attrname not found
}
/*

5.6 Function ~ConcatLists~

Concatenates two lists.

*/
ListExpr ConcatLists( ListExpr list1, ListExpr list2)
{
  if (nl->IsEmpty(list1))
  {
    return list2;
  }
  else
  {
    return nl->Cons(nl->First(list1), ConcatLists(nl->Rest(list1), list2));
  }
}

/*

5.6 Function ~IsTupleDescription~

Checks wether a ListExpression is of the form
((a1 t1) ... (ai ti)).

*/
bool IsTupleDescription(ListExpr a)
{
  ListExpr rest = a;
  ListExpr current;

  while(!nl->IsEmpty(rest))
  {
    current = nl->First(rest);
    rest = nl->Rest(rest);
    if((nl->ListLength(current) == 2)
      && (nl->IsAtom(nl->First(current)))
      && (nl->AtomType(nl->First(current)) == SymbolType)
      && (nl->IsAtom(nl->Second(current)))
      && (nl->AtomType(nl->Second(current)) == SymbolType))
    {
    }
    else
    {
      return false;
    }
  }
  return true;
}

/*

5.6 Function ~AttributesAreDisjoint~

Checks wether two ListExpressions are of the form
((a1 t1) ... (ai ti)) and ((b1 d1) ... (bj dj))
and wether the ai and the bi are disjoint.

*/
bool AttributesAreDisjoint(ListExpr a, ListExpr b)
{
  set<string> aNames;
  ListExpr rest = a;
  ListExpr current;

  while(!nl->IsEmpty(rest))
  {
    current = nl->First(rest);
    rest = nl->Rest(rest);
    if((nl->ListLength(current) == 2)
      && (nl->IsAtom(nl->First(current)))
      && (nl->AtomType(nl->First(current)) == SymbolType)
      && (nl->IsAtom(nl->Second(current)))
      && (nl->AtomType(nl->Second(current)) == SymbolType))
    {
      aNames.insert(nl->SymbolValue(nl->First(current)));
    }
    else
    {
      return false;
    }
  }
  rest = b;
  while(!nl->IsEmpty(rest))
  {
    ListExpr current = nl->First(rest);
    rest = nl->Rest(rest);
    if((nl->ListLength(current) == 2)
      && (nl->IsAtom(nl->First(current)))
      && (nl->AtomType(nl->First(current)) == SymbolType)
      && (nl->IsAtom(nl->Second(current)))
      && (nl->AtomType(nl->Second(current)) == SymbolType))
    {
      if(aNames.find(nl->SymbolValue(nl->First(current))) != aNames.end())
      {
        return false;
      }
    }
    else
    {
      return false;
    }
  }
  return true;
}

/*

3 Type constructors of the Algebra

1.3 Type constructor ~tuple~

The list representation of a tuple is:

----	(<attrrep 1> ... <attrrep n>)
----

Typeinfo is:

----	(<NumericType(<type exression>)> <number of attributes>)
----


For example, for

----	(tuple
		(
			(name string)
			(age int)))
----

the typeinfo is

----	(
	    	(2 2)
			(
				(name (1 4))
				(age (1 1)))
		2)
----

The typeinfo list consists of three lists. The first list is a
pair (AlgebraID, Constructor ID). The second list represents the
attributelist of the tuple. This list is a sequence of pairs (attribute
name (AlgebraID ConstructorID)). Here the ConstructorID is the identificator
of a standard data type, e.g. int. The third list is an atom and counts the
number of the tuple's attributes.

1.3.1 Type property of type constructor ~tuple~

*/
static ListExpr TupleProp ()
{
  return (nl->TwoElemList(nl->TwoElemList(nl->SymbolAtom("plus"),
          nl->TwoElemList(nl->SymbolAtom("ident"), nl->SymbolAtom("DATA"))),
          nl->SymbolAtom("TUPLE")));
}
/*

1.3.1 Main memory representation

Each instance of the class defined below will be the main memory
representation of a value of type ~tuple~.

		Figure 1: Main memory representation of a tuple (class ~CcTuple~) [tuple.eps]

*/
class TupleAttributesInfo
{

    TupleAttributes* tupleType;
    AttributeType* attrTypes;

  public:

    TupleAttributesInfo (ListExpr typeInfo, ListExpr value);

};

TupleAttributesInfo::TupleAttributesInfo (ListExpr typeInfo, ListExpr value)
{
  ListExpr attrlist, valuelist,first,firstvalue, errorInfo;
  Word attr;
  int algebraId, typeId, noofattrs;
  attrTypes = new AttributeType[nl->ListLength(value)];
  AlgebraManager* algM = SecondoSystem::GetAlgebraManager();
  bool valueCorrect;

  attrlist = nl->Second(typeInfo);
  valuelist = value;
  noofattrs = 0;

  while (!nl->IsEmpty(attrlist))
  {
    first = nl->First(attrlist);
    attrlist = nl->Rest(attrlist);

    algebraId = nl->IntValue(nl->First(nl->Second(first)));
    typeId = nl->IntValue(nl->Second(nl->Second(first)));

    firstvalue = nl->First(valuelist);
    valuelist = nl->Rest(valuelist);
    attr = (algM->InObj(algebraId, typeId))(nl->Second(first),
              firstvalue, 0, errorInfo, valueCorrect);
    if (valueCorrect)
    {
      AttributeType attrtype = { algebraId, typeId, ((Attribute*)attr.addr)->Sizeof() };
      attrTypes[noofattrs] = attrtype;
      noofattrs++;
    }
  }
  tupleType = new TupleAttributes(noofattrs, attrTypes);
};

static int ccTuplesCreated = 0;
static int ccTuplesDeleted = 0;


class CcTuple
{
  private:

    int NoOfAttr;
    Attribute* AttrList [MaxSizeOfAttr];

    /* if a tuple is free, then a stream receiving the tuple can delete or
       reuse it */
    bool isFree;

  public:

    CcTuple ()
    {
      NoOfAttr = 0;
      for (int i=0; i < MaxSizeOfAttr; i++)
        AttrList[i] = 0;
      ccTuplesCreated++;
    };

    virtual ~CcTuple ()
    {
      ccTuplesDeleted++;
    };

    Attribute* Get (int index) {return AttrList[index];};
    void  Put (int index, Attribute* attr) {AttrList[index] = attr;};
    void  SetNoAttrs (int noattr) {NoOfAttr = noattr;};
    int   GetNoAttrs () {return NoOfAttr;};
    bool IsFree() { return isFree; }
    void SetFree(bool b) { isFree = b; }

    CcTuple* CloneIfNecessary()
    {
      if(IsFree())
      {
        return this;
      }
      else
      {
        CcTuple* result = new CcTuple();
        result->SetFree(true);
        result->SetNoAttrs(GetNoAttrs());
        for(int i = 0; i < GetNoAttrs(); i++)
        {
          Attribute* attr = ((Attribute*)Get(i))->Clone();
          result->Put(i, attr);
        }
        return result;
      }
    }

    void DeleteIfAllowed()
    {
      if(IsFree())
      {
        for(int i = 0; i < GetNoAttrs(); i++)
        {
          Attribute* attr = (Attribute*)Get(i);
          delete attr;
        }
        delete this;
      }
    }

    friend
    ostream& operator<<(ostream& s, CcTuple t);
};

string
ReportTupleStatistics()
{
  ostringstream buf;
  buf << ccTuplesCreated << " tuples created, "
      << ccTuplesDeleted << " tuples deleted, difference is "
      << (ccTuplesCreated - ccTuplesDeleted) << "." << endl;
  return buf.str();
}

/*

The next function supports writing objects of class CcTuple to standard
output. It is only needed for internal tests.

*/
ostream& operator<<(ostream& os, CcTuple t)
{
  TupleElement* attr;

  os << "(";
  for (int i=0; i < t.GetNoAttrs(); i++)
  {
    attr = (TupleElement*)t.Get(i);
    attr->Print(os);
    if (i < (t.GetNoAttrs() - 1)) os << ",";
  }
  os << ")";
  return os;
}

/*

The lexicographical order on CcTuple. To be used in conjunction with
STL algorithms.

*/
class LexicographicalCcTupleCmp
{
public:
  bool operator()(const CcTuple* aConst, const CcTuple* bConst) const
  {
    CcTuple* a = (CcTuple*)aConst;
    CcTuple* b = (CcTuple*)bConst;


    for(int i = 0; i < a->GetNoAttrs(); i++)
    {
      if(((Attribute*)a->Get(i))->Compare(((Attribute*)b->Get(i))) < 0)
      {
        return true;
      }
      else
      {
        if(((Attribute*)a->Get(i))->Compare(((Attribute*)b->Get(i))) > 0)
        {
          return false;
        }
      }
    }
    return false;
  }
};

/*

1.3.2 ~Out~-function of type constructor ~tuple~

The ~out~-function of type constructor ~tuple~ takes as inputs a type
description (~typeInfo~) of the tuples attribute structure in nested list
format and a pointer to a tuple value, stored in main memory.
The function returns the tuple value from main memory storage
in nested list format.

*/
ListExpr OutTuple (ListExpr typeInfo, Word  value)
{
  int attrno, algebraId, typeId;
  ListExpr l, lastElem, attrlist, first, valuelist;
  CcTuple* tupleptr;

  tupleptr = (CcTuple*)value.addr;
  AlgebraManager* algM = SecondoSystem::GetAlgebraManager();
  attrlist = nl->Second(nl->First(typeInfo));
  attrno = 0;
  l = nl->TheEmptyList();
  while (!nl->IsEmpty(attrlist))
  {
    first = nl->First(attrlist);
    attrlist = nl->Rest(attrlist);
    algebraId = nl->IntValue(nl->First(nl->Second(first)));
    typeId = nl->IntValue(nl->Second(nl->Second(first)));
    valuelist = (algM->OutObj(algebraId, typeId))(nl->Rest(first),
                  SetWord(tupleptr->Get(attrno)));
    attrno++;
    if (l == nl->TheEmptyList())
    {
      l = nl->Cons(valuelist, nl->TheEmptyList());
      lastElem = l;
    }
    else
      lastElem = nl->Append(lastElem, valuelist);
  }
  return l;
}
/*

1.3.2 ~In~-function of type constructor ~tuple~

The ~in~-function of type constructor ~tuple~ takes as inputs a type
description (~typeInfo~) of the tuples attribute structure in nested
list format and the tuple value in nested list format. The function
returns a pointer to atuple value, stored in main memory in accordance to
the tuple value in nested list format.

Error handling in ~InTuple~: ~Correct~ is only true if there is the right
number of attribute values and all values have correct list representations.
Otherwise the following error messages are added to ~errorInfo~:

----	(71 tuple 1 <errorPos>)		        atom instead of value list
	(71 tuple 2 <errorPos>)		        not enough values
	(71 tuple 3 <errorPos> <attrno>) 	wrong attribute value in
					        attribute <attrno>
	(71 tuple 4 <errorPos>)		        too many values
----

is added to ~errorInfo~. Here ~errorPos~ is the number of the tuple in the
relation list (passed by ~InRelation~).


*/
static Word InTuple(ListExpr typeInfo, ListExpr value,
          int errorPos, ListExpr& errorInfo, bool& correct)
{
  int  attrno, algebraId, typeId, noOfAttrs;
  Word attr;
  CcTuple* tupleaddr;
  bool valueCorrect;
  ListExpr first, firstvalue, valuelist, attrlist;

  attrno = 0;
  noOfAttrs = 0;
  tupleaddr = new CcTuple();

  attrlist =  nl->Second(nl->First(typeInfo));
  valuelist = value;
  correct = true;
  if (nl->IsAtom(valuelist))
  {
    correct = false;
    errorInfo = nl->Append(errorInfo,
      nl->FourElemList(nl->IntAtom(71), nl->SymbolAtom("tuple"), nl->IntAtom(1),
      nl->IntAtom(errorPos)));
    delete tupleaddr;
    return SetWord(Address(0));
  }
  else
  {

    AlgebraManager* algM = SecondoSystem::GetAlgebraManager();
    while (!nl->IsEmpty(attrlist))
    {
      first = nl->First(attrlist);
      attrlist = nl->Rest(attrlist);
      attrno++;
      algebraId = nl->IntValue(nl->First(nl->Second(first)));
      typeId = nl->IntValue(nl->Second(nl->Second(first)));
      if (nl->IsEmpty(valuelist))
      {
        correct = false;
        errorInfo = nl->Append(errorInfo,
          nl->FourElemList(nl->IntAtom(71), nl->SymbolAtom("tuple"), nl->IntAtom(2),
            nl->IntAtom(errorPos)));
        delete tupleaddr;
        return SetWord(Address(0));

      }
      else
      {
        firstvalue = nl->First(valuelist);
        valuelist = nl->Rest(valuelist);
        attr = (algM->InObj(algebraId, typeId))(nl->Rest(first),
                 firstvalue, attrno, errorInfo, valueCorrect);
        if (valueCorrect)
        {
          correct = true;
          tupleaddr->Put(attrno - 1, (Attribute*)attr.addr);
          noOfAttrs++;
        }
        else
        {
          correct = false;
          errorInfo = nl->Append(errorInfo,
            nl->FiveElemList(nl->IntAtom(71), nl->SymbolAtom("tuple"), nl->IntAtom(3),
          nl->IntAtom(errorPos), nl->IntAtom(attrno)));
          delete tupleaddr;
          return SetWord(Address(0));
        }
      }
    }
    if (!nl->IsEmpty(valuelist))
    {
      correct = false;
      errorInfo = nl->Append(errorInfo,
      nl->FourElemList(nl->IntAtom(71), nl->SymbolAtom("tuple"), nl->IntAtom(4),
      nl->IntAtom(errorPos)));
      delete tupleaddr;
      return SetWord(Address(0));
    }
  }
  tupleaddr->SetNoAttrs(noOfAttrs);
  return (SetWord(tupleaddr));
}
/*

1.3.4 ~Destroy~-function of type constructor ~tuple~

A type constructor's ~destroy~-function is used by the query processor in order
to deallocate memory occupied by instances of Secondo objects. They may have
been created in two ways:

  * as return values of operator calls

  * by calling a type constructor's ~create~-function.

The corresponding function of type constructor ~tuple~ is called ~DeleteTuple~.

*/
void DeleteTuple(Word& w)
{
  CcTuple* tupleptr;
  int attrno;
  tupleptr = (CcTuple*)w.addr;
  attrno = tupleptr->GetNoAttrs();
  for (int i = 0; i <= (attrno - 1); i++)
  {
    delete (TupleElement*)tupleptr->Get(i);
  }
  delete tupleptr;
}
/*

1.3.4 ~Check~-function of type constructor ~tuple~

Checks the specification:

----	(ident x DATA)+		-> TUPLE	tuple
----

with the additional constraint that all identifiers used (attribute names)
must be distinct. Hence a tuple type has the form:

----	(tuple
	    (
		(age x)
		(name y)))
----

and ~x~ and ~y~ must be types of kind DATA. Kind TUPLE introduces the
following error codes:

----	(... 1) 	Empty tuple type
	(... 2 x)  	x is not an attribute list, but an atom
	(... 3 x)	Doubly defined attribute name x
	(... 4 x)	Invalid attribute name x
	(... 5 x)	Invalid attribute definition x (x is not a pair)
	(... 6 x)	Attribute type does not belong to kind DATA
----

*/
static bool CheckTuple(ListExpr type, ListExpr& errorInfo)
{
  vector<string> attrnamelist;
  ListExpr attrlist, pair;
  string attrname;
  bool correct, ckd;
  int unique;
  vector<string>::iterator it;
  AlgebraManager* algMgr;

  if ((nl->ListLength(type) == 2) && (nl->IsEqual(nl->First(type), "tuple",
       true)))
  {
    attrlist = nl->Second(type);
    if (nl->IsEmpty(attrlist))
    {
      errorInfo = nl->Append(errorInfo,
	nl->ThreeElemList(nl->IntAtom(61), nl->SymbolAtom("TUPLE"),
	nl->IntAtom(1)));
      return false;
    }
    if (nl->IsAtom(attrlist))
    {
      errorInfo = nl->Append(errorInfo,
	nl->FourElemList(nl->IntAtom(61), nl->SymbolAtom("TUPLE"),
	nl->IntAtom(2),
        attrlist));
      return false;
    }
    algMgr = SecondoSystem::GetAlgebraManager();
    attrnamelist.resize(MaxSizeOfAttr);
    it = attrnamelist.begin();
    unique = 0;
    correct = true;
    while (!nl->IsEmpty(attrlist))
    {
      pair = nl->First(attrlist);
      attrlist = nl->Rest(attrlist);
      if (nl->ListLength(pair) == 2)
      {
        if ((nl->IsAtom(nl->First(pair))) &&
	    (nl->AtomType(nl->First(pair)) == SymbolType))
        {
          attrname = nl->SymbolValue(nl->First(pair));
          unique = std::count(attrnamelist.begin(), attrnamelist.end(),
	                       attrname);
          if (unique > 0)
          {
	    errorInfo = nl->Append(errorInfo,
	       nl->FourElemList(nl->IntAtom(61), nl->SymbolAtom("TUPLE"),
	       nl->IntAtom(3), nl->First(pair)));
            correct = false;
          }
          *it = attrname;
          ckd =  algMgr->CheckKind("DATA", nl->Second(pair), errorInfo);
          if (!ckd)
          {
            errorInfo = nl->Append(errorInfo,
	      nl->FourElemList(nl->IntAtom(61), nl->SymbolAtom("TUPLE"),
	      nl->IntAtom(6),nl->Second(pair)));
          }
          correct = correct && ckd;
        }
        else
        {
          errorInfo = nl->Append(errorInfo,
          nl->FourElemList(nl->IntAtom(61), nl->SymbolAtom("TUPLE"),
          nl->IntAtom(4),nl->First(pair)));
          correct = false;
        }
      }
      else
      {
        errorInfo = nl->Append(errorInfo,
          nl->FourElemList(nl->IntAtom(61), nl->SymbolAtom("TUPLE"),
          nl->IntAtom(5),pair ));
        correct = false;
      }
      it++;
    }
    return correct;
  }
  else
  {
    errorInfo = nl->Append(errorInfo,
      nl->ThreeElemList(nl->IntAtom(60), nl->SymbolAtom("TUPLE"), type));
    return false;
  }
}
/*

3.2.5 ~Cast~-function of type constructor ~tuple~

*/
static void* CastTuple(void* addr)
{
  return ( 0 );
}
/*

1.3.3 ~Create~-function of type constructor ~tuple~

The function is used to allocate memory sufficient for keeping one instance
of ~tuple~. The ~Size~-parameter is not evaluated.

*/
static Word CreateTuple(int Size)
{
  CcTuple* tup;
  tup = new CcTuple();
  return (SetWord(tup));
}
/*

3.2.5 ~Model~-functions of type constructor ~tuple~

*/
static Word TupleInModel( ListExpr typeExpr, ListExpr list, int objNo )
{
  return (SetWord( Address( 0 ) ));
}

static ListExpr TupleOutModel( ListExpr typeExpr, Word model )
{
  return (0);
}

static Word TupleValueToModel( ListExpr typeExpr, Word value )
{
  return (SetWord( Address( 0 ) ));
}

static Word TupleValueListToModel( const ListExpr typeExpr, const ListExpr valueList,
                       const int errorPos, ListExpr& errorInfo, bool& correct )
{
  correct = true;
  return (SetWord( Address( 0 ) ));
}
/*

1.3.5 Defnition of type constructor ~tuple~

Eventually a type constructor is created by defining an instance of
class ~TypeConstructor~. Constructor's arguments are the type constructor's
name and the eleven functions previously defined.

*/
TypeConstructor cpptuple( "tuple",         TupleProp,
                        OutTuple,          InTuple,     CreateTuple,
                        DeleteTuple,       CastTuple,   CheckTuple,
			0,                 0,
			TupleInModel,      TupleOutModel,
			TupleValueToModel, TupleValueListToModel );
/*

1.4 Type constructor ~rel~

The list representation of a relation is:

----	(<tuplerep 1> ... <tuplerep n>)
----

Typeinfo is:

----	(<NumericType(<type exression>)>)
----

For example, for

----	(rel (tuple ((name string) (age int))))
----

the type info is

----	((2 1) ((2 2) ((name (1 4)) (age (1 1)))))
----

1.3.1 Type property of type constructor ~rel~

*/
static ListExpr RelProp ()
{
  return (nl->TwoElemList(nl->OneElemList(nl->SymbolAtom("TUPLE")),
          nl->SymbolAtom("REL")));
}
/*

1.3.1 Main memory representation

		Figure 2: Main memory representation of a relation (~Compact Table~) [relation.eps]

*/
typedef CTable<CcTuple*>* Relation;

class CcRel
{
  private:

    int NoOfTuples;
    Relation TupleList;
    CTable<CcTuple*>::Iterator rs;

  public:

    CcRel () {NoOfTuples = 0; TupleList = new CTable<CcTuple*>(100);};
    ~CcRel () { delete TupleList; };
    void    NewScan() {rs = TupleList->Begin();};
    bool    EndOfScan() {return (rs == TupleList->End());};
    void    NextScan() {(rs)++;};
    CcTuple* GetTuple() {return ((CcTuple*)(*rs));};
    void    AppendTuple (CcTuple* t)
    {
      TupleList->Add(t);
      NoOfTuples++;
    };

    void    SetNoTuples (int notuples) {NoOfTuples = notuples;};
    int     GetNoTuples () {return NoOfTuples;};

};
/*

1.4.2 ~Out~-function of type constructor ~rel~

*/
ListExpr OutRel(ListExpr typeInfo, Word  value)
{
  CcTuple* t;
  ListExpr l, lastElem, tlist, TupleTypeInfo;
  CcRel* r = (CcRel*)(value.addr);

  r->NewScan();
  l = nl->TheEmptyList();

  while (!r->EndOfScan())
  {
    t = r->GetTuple();
    TupleTypeInfo = nl->TwoElemList(nl->Second(typeInfo),
	  nl->IntAtom(nl->ListLength(nl->Second(nl->Second(typeInfo)))));
    tlist = OutTuple(TupleTypeInfo, SetWord(t));
    if (l == nl->TheEmptyList())
    {
      l = nl->Cons(tlist, nl->TheEmptyList());
      lastElem = l;
    }
    else
      lastElem = nl->Append(lastElem, tlist);
    r->NextScan();
  }
  return l;

}
/*

1.3.3 ~Create~-function of type constructor ~rel~

The function is used to allocate memory sufficient for keeping one instance
of ~rel~. The ~Size~-parameter is not evaluated.

*/
static Word CreateRel(int Size)
{
  CcRel* rel = new CcRel();
  return (SetWord(rel));
}
/*

1.4.2 ~In~-function of type constructor ~rel~

~value~ is the list representation of the relation. The structure of
~typeInfol~ and ~value~ are described above. Error handling in ~InRel~:

The result relation will contain all tuples that have been converted
correctly (have correct list expressions). For all other tuples, an error
message containing the position of the tuple within this relation (list) is
added to ~errorInfo~. (This is done by procedure ~InTuple~ called by ~InRel~).
If any tuple representation is wrong, then ~InRel~ will return ~correct~ as
FALSE and will itself add an error message of the form

----	(InRelation <errorPos>)
----

to ~errorInfo~. The value in ~errorPos~ has to be passed from the environment;
probably it is the position of the relation object in the list of
database objects.

*/
static Word InRel(ListExpr typeInfo, ListExpr value,
          int errorPos, ListExpr& errorInfo, bool& correct)
{
  ListExpr tuplelist, TupleTypeInfo, first;
  CcRel* rel;
  CcTuple* tupleaddr;
  int tupleno, count;
  bool tupleCorrect;

  correct = true;
  count = 0;
  rel = new CcRel();

  tuplelist = value;
  TupleTypeInfo = nl->TwoElemList(nl->Second(typeInfo),
	  nl->IntAtom(nl->ListLength(nl->Second(nl->Second(typeInfo)))));
  tupleno = 0;
  if (nl->IsAtom(value))
  {
    correct = false;
    errorInfo = nl->Append(errorInfo,
    nl->ThreeElemList(nl->IntAtom(70), nl->SymbolAtom("rel"), tuplelist));
    return SetWord(rel);
  }
  else
  { // increase tupleno
    while (!nl->IsEmpty(tuplelist))
    {
      first = nl->First(tuplelist);
      tuplelist = nl->Rest(tuplelist);
      tupleno++;
      tupleaddr = (CcTuple*)(InTuple(TupleTypeInfo, first, tupleno,
        errorInfo, tupleCorrect).addr);
      tupleaddr->SetFree(false);

      if (tupleCorrect)
      {
        rel->AppendTuple(tupleaddr);
        count++;
      }
      else
      {
        correct = false;
      }
    }
    if (!correct)
    {
      errorInfo = nl->Append(errorInfo,
      nl->TwoElemList(nl->IntAtom(72), nl->SymbolAtom("rel")));
    }
    else rel->SetNoTuples(count);

    rel->NewScan();
    while (! rel->EndOfScan())
    {
      rel->NextScan();
    }
    return (SetWord((void*)rel));
  }

}
/*

1.3.4 ~Destroy~-function of type constructor ~rel~


The corresponding function of type constructor ~rel~ is called ~DeleteRel~.

*/
void DeleteRel(Word& w)
{

  CcTuple* t;
  CcRel* r;
  Word v;

  r = (CcRel*)w.addr;
  r->NewScan();
  while (!r->EndOfScan())
  {
    t = r->GetTuple();
    v = SetWord(t);
    DeleteTuple(v);
    r->NextScan();
  }
  delete r;
}
/*

4.3.8 ~Check~-function of type constructor ~rel~

Checks the specification:

----    TUPLE   -> REL          rel
----

Hence the type expression must have the form

----    (rel x)
----

and ~x~ must be a type of kind TUPLE.

*/
static bool CheckRel(ListExpr type, ListExpr& errorInfo)
{
  AlgebraManager* algMgr;

  if ((nl->ListLength(type) == 2) && nl->IsEqual(nl->First(type), "rel"))
  {
    algMgr = SecondoSystem::GetAlgebraManager();
    return (algMgr->CheckKind("TUPLE", nl->Second(type), errorInfo));
  }
  else
  {
    errorInfo = nl->Append(errorInfo,
      nl->ThreeElemList(nl->IntAtom(60), nl->SymbolAtom("REL"), type));
    return false;
  }
}
/*

3.2.5 ~Cast~-function of type constructor ~rel~

*/
static void* CastRel(void* addr)
{
  return ( 0 );
}
/*


3.2.5 ~PersistFunction~ of type constructor ~rel~

This is a slightly modified version of the function ~DefaultPersistValue~ (from
~Algebra~) which creates the relation from the SmiRecord only if it does not
yet exist.

The idea is to maintain a cache containing the relation representations that
have been built in memory. The cache basically stores pairs (recordId, relation
value). If the record Id passed to this function is found, the cached relation
value is returned instead of building a new one.

*/

bool
RelPersistValue( const PersistDirection dir,
		SmiRecord& valueRecord,
		const ListExpr typeInfo,
		Word& value )
{
  NestedList* nl = SecondoSystem::GetNestedList();
  ListExpr valueList;
  string valueString;
  int valueLength;
  
  if ( dir == ReadFrom )
  {
    SmiKey mykey;
    SmiRecordId recId;
    mykey = valueRecord.GetKey();
    if ( ! mykey.GetKey(recId) ) cout << "
	RelPersistValue: Couldn't get the key!" << endl;

    // cout << "the record number is: " << recId << endl;

    static bool firsttime = true;
    const int cachesize = 20;
    static int current = 0;
    static SmiRecordId key[cachesize];
    static Word cache[cachesize];

    // initialize

    if ( firsttime ) {
      for ( int i = 0; i < cachesize; i++ ) { key[i] = 0; }
      firsttime = false;
    }

    // check whether value was cached

    bool found = false;
    int pos;
    for ( int j = 0; j < cachesize; j++ )
      if ( key[j]  == recId ) {
	found = true;
	pos = j;
        break;
      }
    
    if ( found ) {value = cache[pos]; return true;}

    // prepare to cache the value constructed from the list

    if ( key[current] != 0 ) { 
      	// cout << "I do delete!" << endl;
      DeleteRel(cache[current]);
    }

    key[current] = recId;

    ListExpr errorInfo = nl->OneElemList( nl->SymbolAtom( "ERRORS" ) );
    bool correct;
    valueRecord.Read( &valueLength, sizeof( valueLength ), 0 );
    char* buffer = new char[valueLength];
    valueRecord.Read( buffer, valueLength, sizeof( valueLength ) );
    valueString.assign( buffer, valueLength );
    delete []buffer;
    nl->ReadFromString( valueString, valueList );
    value = InRel( nl->First(typeInfo), nl->First(valueList), 1, errorInfo,
	correct); 

    cache[current++] = value;
    if ( current == cachesize ) current = 0;
        
    if ( errorInfo != 0 )     {
      nl->Destroy( errorInfo );
    }
  }
  else // WriteTo
  {
    valueList = OutRel( nl->First(typeInfo), value );
    valueList = nl->OneElemList( valueList );
    nl->WriteToString( valueString, valueList );
    valueLength = valueString.length();
    valueRecord.Write( &valueLength, sizeof( valueLength ), 0 );
    valueRecord.Write( valueString.data(), valueString.length(), sizeof( valueLength ) );

    value = SetWord(Address(0));
  }
  nl->Destroy( valueList );
  return (true);
}


/*

3.2.5 ~Model~-functions of type constructor ~rel~

*/
static Word RelInModel( ListExpr typeExpr, ListExpr list, int objNo )
{
  return (SetWord( Address( 0 ) ));
}

static ListExpr RelOutModel( ListExpr typeExpr, Word model )
{
  return (0);
}

static Word RelValueToModel( ListExpr typeExpr, Word value )
{
  return (SetWord( Address( 0 ) ));
}

static Word RelValueListToModel( const ListExpr typeExpr, const ListExpr valueList,
                       const int errorPos, ListExpr& errorInfo, bool& correct )
{
  correct = true;
  return (SetWord( Address( 0 ) ));
}
/*

1.3.5 Definition of type constructor ~rel~

Eventually a type constructor is created by defining an instance of
class ~TypeConstructor~. Constructor's arguments are the type constructor's
name and the eleven functions previously defined.

*/
TypeConstructor cpprel( "rel",           RelProp,
                        OutRel,          InRel,   CreateRel,
                        DeleteRel,       CastRel,   CheckRel,
			RelPersistValue, 0,
			RelInModel,      RelOutModel,
			RelValueToModel, RelValueListToModel );
/*

4 Operators

4.2 Selection function for non-overloaded operators

For non-overloaded operators, the set of value mapping functions consists
of exactly one element.  Consequently, the selection function of such an
operator always returns 0.

*/
static int simpleSelect (ListExpr args) { return 0; }

/*

4.2 Selection function for type operators

The selection function of a type operator always returns -1.

*/
static int typeOperatorSelect(ListExpr args) { return -1; }


/*

6.1 Type Operator ~TUPLE~

Type operators are used only for inferring argument types of parameter
functions. They have a type mapping but no evaluation function.

6.1.1 Type mapping function of operator ~TUPLE~

Extract tuple type from a stream or relation type given as the first argument.

----    ((stream x) ...)                -> x
        ((rel x)    ...)                -> x
----

*/
ListExpr TUPLETypeMap(ListExpr args)
{
  ListExpr first;
  if(nl->ListLength(args) >= 1)
  {
    first = nl->First(args);
    if(nl->ListLength(first) == 2  )
    {
      if ((TypeOfRelAlgSymbol(nl->First(first)) == stream)  ||
          (TypeOfRelAlgSymbol(nl->First(first)) == rel))
        return nl->Second(first);
    }
  }
  return nl->SymbolAtom("typeerror");
}
/*

4.1.3 Specification of operator ~TUPLE~

*/
const string TUPLESpec =
  "(<text>((stream x)...) -> x, ((rel x)...) -> x</text--->"
  "<text>Extract tuple type from a stream or relation type "
  "given as the first argument.</text--->)";
/*

4.1.3 Definition of operator ~TUPLE~

*/
Operator TUPLE (
         "TUPLE",              // name
         TUPLESpec,            // specification
         0,                    // no value mapping
         Operator::DummyModel, // dummy model mapping, defines in Algebra.h
         typeOperatorSelect,   // trivial selection function
         TUPLETypeMap          // type mapping
);
/*

6.1 Type Operator ~TUPLE2~

6.1.1 Type mapping function of operator ~TUPLE2~

Extract tuple type from a stream or relation type given as the second argument.

----    ((stream x) (stream y) ...)          -> y
        ((rel x) (rel y) ...)                -> y
----

*/
ListExpr TUPLE2TypeMap(ListExpr args)
{
  ListExpr second;
  if(nl->ListLength(args) >= 2)
  {
    second = nl->Second(args);
    if(nl->ListLength(second) == 2  )
    {
      if ((TypeOfRelAlgSymbol(nl->First(second)) == stream)  ||
          (TypeOfRelAlgSymbol(nl->First(second)) == rel))
        return nl->Second(second);
    }
  }
  return nl->SymbolAtom("typeerror");
}
/*

4.1.3 Specification of operator ~TUPLE2~

*/
const string TUPLE2Spec =
  "(<text>((stream x) (stream y) ...) -> y, ((rel x) (rel y) ...) -> "
  "y</text---><text>Extract tuple type from a stream or relation type "
  "given as the second argument.</text--->)";
/*

4.1.3 Definition of operator ~TUPLE2~

*/
Operator TUPLE2 (
         "TUPLE2",             // name
         TUPLE2Spec,           // specification
         0,                    // no value mapping
         Operator::DummyModel, // dummy model mapping, defines in Algebra.h
         typeOperatorSelect,   // trivial selection function
         TUPLE2TypeMap         // type mapping
);

/*

6.1 Type Operator ~GROUP~

Type operators are used only for inferring argument types of parameter
functions. They have a type mapping but no evaluation function.

6.1.1 Type mapping function of operator ~GROUP~

----  ((stream x))                -> (rel x)
----

*/
ListExpr GROUPTypeMap(ListExpr args)
{
  ListExpr first;
  ListExpr tupleDesc;

  if(!nl->IsAtom(args) && nl->ListLength(args) >= 1)
  {
    first = nl->First(args);
    if(!nl->IsAtom(first) && nl->ListLength(first) == 2  )
    {
      tupleDesc = nl->Second(first);
      if(TypeOfRelAlgSymbol(nl->First(first)) == stream
        && (!nl->IsAtom(tupleDesc))
        && (nl->ListLength(tupleDesc) == 2)
        && TypeOfRelAlgSymbol(nl->First(tupleDesc)) == tuple
        && IsTupleDescription(nl->Second(tupleDesc)))
        return
          nl->TwoElemList(
            nl->SymbolAtom("rel"),
            tupleDesc);
    }
  }
  return nl->SymbolAtom("typeerror");
}
/*

4.1.3 Specification of operator ~GROUP~

*/
const string GROUPSpec =
  "(<text>((stream x)) -> (rel x)</text---><text>Maps stream type to a rel "
  "type.</text--->)";
/*

4.1.3 Definition of operator ~GROUP~

*/
Operator GROUP (
         "GROUP",              // name
         GROUPSpec,            // specification
         0,                    // no value mapping
         Operator::DummyModel, // dummy model mapping, defines in Algebra.h
         typeOperatorSelect,   // trivial selection function
         GROUPTypeMap          // type mapping
);

/*

4.1 Operator ~feed~

Produces a stream from a relation by scanning the relation tuple by tuple.

4.1.1 Type mapping function of operator ~feed~

A type mapping function takes a nested list as argument. Its contents are
type descriptions of an operator's input parameters. A nested list describing
the output type of the operator is returned.

Result type of feed operation.

----	((rel x))		-> (stream x)
----

*/
static ListExpr FeedTypeMap(ListExpr args)
{
  ListExpr first ;

  if(nl->ListLength(args) == 1)
  {
    first = nl->First(args);
    if(nl->ListLength(first) == 2)
    {
      if (TypeOfRelAlgSymbol(nl->First(first)) == rel)
        return nl->Cons(nl->SymbolAtom("stream"), nl->Rest(first));
    }
  }
  return nl->SymbolAtom("typeerror");
}
/*

4.1.2 Value mapping function of operator ~feed~

*/
static int
Feed(Word* args, Word& result, int message, Word& local, Supplier s)
{
 CcRel* r;

  switch (message)
  {
    case OPEN :

      r = ((CcRel*)args[0].addr);
      r->NewScan();
      local.addr = r;
      return 0;

    case REQUEST :

      r = (CcRel*)local.addr;
      if (!(r->EndOfScan()))
      {
        result = SetWord(r->GetTuple()->CloneIfNecessary());
        r->NextScan();
        return YIELD;
      }
      else
      {
        return CANCEL;
      }

    case CLOSE :

      return 0;
  }
  return 0;
}
/*

4.1.3 Specification of operator ~feed~

*/
const string FeedSpec =
  "(<text>(rel x) -> (stream x)</text---><text>Produces a stream from a "
  "relation by scanning the relation tuple by tuple.</text--->)";
/*

4.1.3 Definition of operator ~feed~

Non-overloaded operators are defined by constructing a new instance of
class ~Operator~, passing all operator functions as constructor arguments.

*/
Operator feed (
          "feed",                // name
          FeedSpec,              // specification
          Feed,                  // value mapping
          Operator::DummyModel, // dummy model mapping, defines in Algebra.h
          simpleSelect,         // trivial selection function
          FeedTypeMap           // type mapping
);
/*
4.1 Operator ~consume~

Collects objects from a stream into a relation.

4.1.1 Type mapping function of operator ~consume~

Operator ~consume~ accepts a stream of tuples and returns a relation.


----    (stream  x)                 -> ( rel x)
----

*/
ListExpr ConsumeTypeMap(ListExpr args)
{
  ListExpr first ;

  if(nl->ListLength(args) == 1)
  {
    first = nl->First(args);
    if(nl->ListLength(first) == 2)
    {
      if (TypeOfRelAlgSymbol(nl->First(first)) == stream)
        return nl->Cons(nl->SymbolAtom("rel"), nl->Rest(first));
    }
  }
  return nl->SymbolAtom("typeerror");
}
/*

4.1.2 Value mapping function of operator ~consume~

*/
static int
Consume(Word* args, Word& result, int message, Word& local, Supplier s)
{
  Word actual;
  CcRel* rel;

  rel = (CcRel*)((qp->ResultStorage(s)).addr);
  qp->Open(args[0].addr);
  qp->Request(args[0].addr, actual);
  while (qp->Received(args[0].addr))
  {
    CcTuple* tuple = (CcTuple*)actual.addr;
    tuple = tuple->CloneIfNecessary();
    tuple->SetFree(false);
    rel->AppendTuple(tuple);
    qp->Request(args[0].addr, actual);
  }

  result = SetWord((void*) rel);

  qp->Close(args[0].addr);
  return 0;
}
/*

4.1.3 Specification of operator ~consume~

*/
const string ConsumeSpec =
  "(<text>(stream x) -> (rel x)</text---><text>Collects objects from a stream "
  "into a relation.</text--->)";
/*

4.1.3 Definition of operator ~consume~

*/
Operator consume (
         "consume",            // name
	 ConsumeSpec,          // specification
	 Consume,              // value mapping
	 Operator::DummyModel, // dummy model mapping, defines in Algebra.h
	 simpleSelect,         // trivial selection function
	 ConsumeTypeMap        // type mapping
);
/*

7.1 Operator ~attr~

7.1.1 Type mapping function of operator ~attr~

Result type attr operation.

----
    ((tuple ((x1 t1)...(xn tn))) xi)    -> ti
                            APPEND (i) ti)
----
This type mapping uses a special feature of the query processor, in that if
requests to append a further argument to the given list of arguments, namely,
the index of the attribute within the tuple. This indes is computed within
the type mapping  function. The request is given through the result expression
of the type mapping which has the form, for example,

----

    (APPEND (1) string)

----

The keyword ~APPEND~ occuring as a first element of a returned type expression
tells the query processor to add the elements of the following list - the
second element of the type expression - as further arguments to the operator
(as if they had been written in the query). The third element  of the query
is then used as the real result type. In this case 1 is the index of the
attribute determined in this procedure. The query processor, more precisely
the procedure ~anotate~ there, will produce the annotation for the constant 1,
append it to the list of annotated arguments, and then use "string" as the
result type of the ~attr~ operation.

*/
ListExpr AttrTypeMap(ListExpr args)
{
  ListExpr first, second, attrtype;
  string  attrname;
  int j;
  if(nl->ListLength(args) == 2)
  {
    first = nl->First(args);
    second  = nl->Second(args);

    if((nl->ListLength(first) == 2  ) &&
        (TypeOfRelAlgSymbol(nl->First(first)) == tuple)  &&
        (nl->IsAtom(second)) &&
        (nl->AtomType(second) == SymbolType))
    {
      attrname = nl->SymbolValue(second);
      j = findattr(nl->Second(first), attrname, attrtype);
      if (j)
      return nl->ThreeElemList(nl->SymbolAtom("APPEND"),
                  nl->OneElemList(nl->IntAtom(j)), attrtype);
    }
    return nl->SymbolAtom("typeerror");
  }
  return nl->SymbolAtom("typeerror");
}
/*

4.1.2 Value mapping function of operator ~attr~

The argument vector ~arg~ contains in the first slot ~args[0]~ the tuple
and in ~args[2]~ the position of the attribute as a number. Returns as
~result~ the value of an attribute at the given position ~args[2]~ in a
tuple object. The attribute name is argument 2 in the query and is used
in the function ~AttributeTypeMap~ to determine the attribute
number ~args[2]~ .

*/
static int
Attr(Word* args, Word& result, int message, Word& local, Supplier s)
{
  CcTuple* tupleptr;
  int index;

  tupleptr = (CcTuple*)args[0].addr;
  index = (int)((StandardAttribute*)args[2].addr)->GetValue();
  if ((1 <= index) && (index <= tupleptr->GetNoAttrs()))
  {
    result = qp->ResultStorage(s);
    ((StandardAttribute*)result.addr)->CopyFrom(
      (StandardAttribute*)tupleptr->Get(index - 1));
    return 0;
  }
  else
  {
    cout << "attribute: index out of range !";
    return -1;
  }
}
/*

4.1.3 Specification of operator ~attr~

*/
const string AttrSpec =
  "(<text>((tuple ((x1 t1)...(xn tn))) xi)  -> ti)</text--->"
  "<text>Returns the value of an attribute at a given position "
  "in a tuple object.</text--->)";
/*

4.1.3 Definition of operator ~attr~

*/
Operator attr (
         "attr",           // name
     AttrSpec,        // specification
     Attr,            // value mapping
     Operator::DummyModel, // dummy model mapping, defines in Algebra.h
     simpleSelect,         // trivial selection function
     AttrTypeMap      // type mapping
);
/*

7.3 Operator ~filter~

Only tuples, fulfilling a certain condition are passed on to the output stream.

7.3.1 Type mapping function of operator ~filter~

Result type of filter operation.

----    ((stream x) (map x bool))       -> (stream x)
----

*/
ListExpr FilterTypeMap(ListExpr args)
{
  ListExpr first, second;
  if(nl->ListLength(args) == 2)
  {
    first = nl->First(args);
    second  = nl->Second(args);

    if ((nl->ListLength(first) == 2 && nl->ListLength(second) == 3 ) &&
        (TypeOfRelAlgSymbol(nl->First(first)) == stream)  &&
    (TypeOfRelAlgSymbol(nl->First(second)) == ccmap)    &&
    (TypeOfRelAlgSymbol(nl->Third(second)) == ccbool)  &&
    (nl->Equal(nl->Second(first),nl->Second(second))))
      return first;
    else
      return nl->SymbolAtom("typeerror");
  }
  return nl->SymbolAtom("typeerror");
}
/*

4.1.2 Value mapping function of operator ~filter~

*/
static int
Filter(Word* args, Word& result, int message, Word& local, Supplier s)
{
  bool found;
  Word elem, funresult;
  ArgVectorPointer funargs;
  CcTuple* tuple;

  switch ( message )
  {

    case OPEN:

      qp->Open (args[0].addr);
      return 0;

    case REQUEST:

      funargs = qp->Argument(args[1].addr);
      qp->Request(args[0].addr, elem);
      found = false;
      while (qp->Received(args[0].addr) && !found)
      {
        tuple = (CcTuple*)elem.addr;
        (*funargs)[0] = elem;
        qp->Request(args[1].addr, funresult);
        if (((StandardAttribute*)funresult.addr)->IsDefined())
          found = (bool)((StandardAttribute*)funresult.addr)->GetValue();
        if (!found)
        {
          tuple->DeleteIfAllowed();
          qp->Request(args[0].addr, elem);
        }
      }
      if (found)
      {
        tuple = tuple->CloneIfNecessary();
        result = SetWord(tuple);
        return YIELD;
      }
      else
        return CANCEL;

    case CLOSE:

      qp->Close(args[0].addr);
      return 0;
  }
  return 0;
}
/*

4.1.3 Specification of operator ~filter~

*/
const string FilterSpec =
  "(<text>((stream x) (map x bool)) -> (stream x)</text---><text>Only "
  "tuples, fulfilling a certain condition are passed on to "
  "the output stream.</text--->)";
/*

4.1.3 Definition of operator ~filter~

*/
Operator tfilter (
         "tfilter",            // name
         FilterSpec,           // specification
         Filter,               // value mapping
         Operator::DummyModel, // dummy model mapping, defines in Algebra.h
         simpleSelect,         // trivial selection function
         FilterTypeMap         // type mapping
);
/*

7.3 Operator ~project~

7.3.1 Type mapping function of operator ~filter~

Result type of project operation.

----	((stream (tuple ((x1 T1) ... (xn Tn)))) (ai1 ... aik))	->

		(APPEND
			(k (i1 ... ik))
			(stream (tuple ((ai1 Ti1) ... (aik Tik))))
		)
----

The type mapping computes the number of attributes and the list of attribute
numbers for the given projection attributes and asks the query processor to
append it to the given arguments.

*/
ListExpr ProjectTypeMap(ListExpr args)
{
  bool firstcall;
  int noAttrs, j;
  ListExpr first, second, first2, attrtype, newAttrList, lastNewAttrList,
           lastNumberList, numberList, outlist;
  string attrname;

  firstcall = true;
  if (nl->ListLength(args) == 2)
  {
    first = nl->First(args);
    second = nl->Second(args);

    if ((nl->ListLength(first) == 2) &&
        (TypeOfRelAlgSymbol(nl->First(first)) == stream) &&
	(nl->ListLength(nl->Second(first)) == 2) &&
	(TypeOfRelAlgSymbol(nl->First(nl->Second(first))) == tuple) &&
	(!nl->IsAtom(second)) &&
	(nl->ListLength(second) > 0))
    {
      noAttrs = nl->ListLength(second);
      while (!(nl->IsEmpty(second)))
      {
        first2 = nl->First(second);
	second = nl->Rest(second);
	if (nl->AtomType(first2) == SymbolType)
	{
	  attrname = nl->SymbolValue(first2);
	}
	else return nl->SymbolAtom("typeerror");
	j = findattr(nl->Second(nl->Second(first)), attrname, attrtype);
	if (j)
	{
	  if (firstcall)
	  {
	    firstcall = false;
	    newAttrList = nl->OneElemList(nl->TwoElemList(first2, attrtype));
	    lastNewAttrList = newAttrList;
	    numberList = nl->OneElemList(nl->IntAtom(j));
	    lastNumberList = numberList;
	  }
	  else
	  {
	    lastNewAttrList =
	      nl->Append(lastNewAttrList, nl->TwoElemList(first2, attrtype));
	    lastNumberList =
	      nl->Append(lastNumberList, nl->IntAtom(j));
	  }
	}
	else return nl->SymbolAtom("typeerror");
      }
      // Check whether all new attribute names are distinct
      // - not yet implemented
      outlist = nl->ThreeElemList(
                 nl->SymbolAtom("APPEND"),
		 nl->TwoElemList(nl->IntAtom(noAttrs), numberList),
		 nl->TwoElemList(nl->SymbolAtom("stream"),
		               nl->TwoElemList(nl->SymbolAtom("tuple"),
			                     newAttrList)));
      // cout << nl->WriteToFile("/dev/tty",outlist) << endl;
      return outlist;
    }
  }
  return nl->SymbolAtom("typeerror");
}
/*

4.1.2 Value mapping function of operator ~project~

*/
static int
Project(Word* args, Word& result, int message, Word& local, Supplier s)
{
  Word elem1, elem2;
  int noOfAttrs, index;
  Supplier son;
  Attribute* attr;
  CcTuple* t;


  switch (message)
  {
    case OPEN :

      qp->Open(args[0].addr);
      return 0;

    case REQUEST :

      qp->Request(args[0].addr, elem1);
      if (qp->Received(args[0].addr))
      {
        t = new CcTuple();
        t->SetFree(true);
        noOfAttrs = ((CcInt*)args[2].addr)->GetIntval();
        t->SetNoAttrs(noOfAttrs);
        for (int i=1; i <= noOfAttrs; i++)
        {
          son = qp->GetSupplier(args[3].addr, i-1);
          qp->Request(son, elem2);
          index = ((CcInt*)elem2.addr)->GetIntval();
          attr = ((CcTuple*)elem1.addr)->Get(index-1);
          t->Put(i-1, ((StandardAttribute*)attr->Clone()));
        }
        ((CcTuple*)elem1.addr)->DeleteIfAllowed();
        result = SetWord(t);
        return YIELD;
      }
      else return CANCEL;

    case CLOSE :

      qp->Close(args[0].addr);
      return 0;
  }
  return 0;
}
/*

4.1.3 Specification of operator ~project~

*/
const string ProjectSpec =
  "(<text>((stream (tuple ((x1 T1) ... (xn Tn)))) (ai1 ... aik)) -> (stream "
  "(tuple ((ai1 Ti1) ... (aik Tik))))</text---><text>Produces a projection "
  "tuple for each tuple of its input stream.</text--->)";
/*

4.1.3 Definition of operator ~project~

*/
Operator project (
         "project",            // name
         ProjectSpec,          // specification
         Project,              // value mapping
         Operator::DummyModel, // dummy model mapping, defines in Algebra.h
         simpleSelect,         // trivial selection function
         ProjectTypeMap        // type mapping
);
/*

7.3 Operator ~product~

5.6.1 Help Function ~Concat~

Copies the attribute values of two tuples
(words) ~r~ and ~s~ into tuple (word) ~t~.

*/
void Concat (Word r, Word s, Word& t)
{
  int rnoattrs, snoattrs, tnoattrs;
  Attribute* attr;

  rnoattrs = ((CcTuple*)r.addr)->GetNoAttrs();
  snoattrs = ((CcTuple*)s.addr)->GetNoAttrs();
  if ((rnoattrs + snoattrs) > MaxSizeOfAttr)
  {
    tnoattrs = MaxSizeOfAttr;
  }
  else
  {
    tnoattrs = rnoattrs + snoattrs;
  }

  ((CcTuple*)t.addr)->SetNoAttrs(tnoattrs);
  for (int i = 1; i <= rnoattrs; i++)
  {
    attr = ((CcTuple*)r.addr)->Get(i - 1);
    ((CcTuple*)t.addr)->Put((i - 1), ((StandardAttribute*)attr)->Clone());
  }
  for (int j = (rnoattrs + 1); j <= tnoattrs; j++)
  {
    attr = ((CcTuple*)s.addr)->Get(j - rnoattrs - 1);
    ((CcTuple*)t.addr)->Put((j - 1), ((StandardAttribute*)attr)->Clone());
  }
}
/*

7.3.1 Type mapping function of operator ~product~

Result type of product operation.

----	((stream (tuple (x1 ... xn))) (stream (tuple (y1 ... ym))))

	-> (stream (tuple (x1 ... xn y1 ... ym)))
----

*/

bool comparenames(ListExpr list)
{
  vector<string> attrnamestrlist;
  vector<string>::iterator it;
  ListExpr attrnamelist;
  int unique;
  string attrname;

  attrnamelist = list;
  attrnamestrlist.resize(nl->ListLength(list));
  it = attrnamestrlist.begin();
  while (!nl->IsEmpty(attrnamelist))
  {
    attrname = nl->SymbolValue(nl->First(nl->First(attrnamelist)));
    attrnamelist = nl->Rest(attrnamelist);
    unique = std::count(attrnamestrlist.begin(), attrnamestrlist.end(),
	                       attrname);
    *it =  attrname;
    if (unique) return false;
    it++;
  }
  return true;
}

ListExpr ProductTypeMap(ListExpr args)
{
  ListExpr first, second, list, list1, list2, outlist;

  if (nl->ListLength(args) == 2)
  {
    first = nl->First(args); second = nl->Second(args);

    // Check first argument and extract list1
    if (nl->ListLength(first) == 2)
    {
      if (TypeOfRelAlgSymbol(nl->First(first)) == stream)
      {
        if (nl->ListLength(nl->Second(first)) == 2)
        {
          if (TypeOfRelAlgSymbol(nl->First(nl->Second(first))) == tuple)
          {
            list1 = nl->Second(nl->Second(first));
          }
          else return nl->SymbolAtom("typeerror");
        }
        else return nl->SymbolAtom("typeerror");
      }
      else return nl->SymbolAtom("typeerror");
    }
    else return nl->SymbolAtom("typeerror");

    // Check second argument and extract list2
    if (nl->ListLength(second) == 2)
    {
      if (TypeOfRelAlgSymbol(nl->First(second)) == stream)
      {
        if (nl->ListLength(nl->Second(second)) == 2)
        {
          if (TypeOfRelAlgSymbol(nl->First(nl->Second(second))) == tuple)
          {
            list2 = nl->Second(nl->Second(second));
          }
          else return nl->SymbolAtom("typeerror");
        }
        else return nl->SymbolAtom("typeerror");
      }
      else return nl->SymbolAtom("typeerror");
    }
    else return nl->SymbolAtom("typeerror");

    list = ConcatLists(list1, list2);
    // Check whether all new attribute names are distinct
    // - not yet implemented
    
    if ( comparenames(list) )
    {
      outlist = nl->TwoElemList(nl->SymbolAtom("stream"),
        nl->TwoElemList(nl->SymbolAtom("tuple"), list));
      return outlist;
    }
    else return nl->SymbolAtom("typeerror");
  }
  else return nl->SymbolAtom("typeerror");
}
/*

4.1.2 Value mapping function of operator ~product~

*/
static int
Product(Word* args, Word& result, int message, Word& local, Supplier s)
{
  Word r, u, t;
  CcTuple* tuple;

  switch (message)
  {
    case OPEN :

      qp->Open(args[0].addr);
      qp->Open(args[1].addr);
      qp->Request(args[0].addr, r);
      local = (qp->Received(args[0].addr)) ? r : SetWord(Address(0));
      return 0;

    case REQUEST :

      if (local.addr == 0)
      {
        qp->Close(args[1].addr);
        return CANCEL;
      }
      else
      {
        r = local;
        qp->Request(args[1].addr, u);
        if (qp->Received(args[1].addr))
        {
          tuple = new CcTuple();
          tuple->SetFree(true);
          t = SetWord(tuple);
          Concat(r, u, t);
          result = t;
          ((CcTuple*)u.addr)->DeleteIfAllowed();

          return YIELD;
        }
        else
        // second stream exhausted and closed now; must get a
        // new tuple from the first stream and restart second stream
        {
          ((CcTuple*)r.addr)->DeleteIfAllowed();
          qp->Request(args[0].addr, r);
          if (qp->Received(args[0].addr))
          {
            local = r;
            qp->Open(args[1].addr);
            qp->Request(args[1].addr, u);
            if (!qp->Received(args[1].addr)) // second stream is empty
            {
              qp->Close(args[0].addr);
              return CANCEL;
            }
            else
            {
              tuple = new CcTuple();
              tuple->SetFree(true);
              t = SetWord(tuple);
              Concat(r, u, t);
              ((CcTuple*)u.addr)->DeleteIfAllowed();
              result = t;
              return YIELD;
            }
          }
          else return CANCEL; // first stream exhausted
        }
      }

    case CLOSE :

      qp->Close(args[0].addr);
      qp->Close(args[1].addr);
      return 0;
  }
  return 0;
}
/*

4.1.3 Specification of operator ~product~

*/
const string ProductSpec =
  "(<text>((stream (tuple (x1 ... xn))) (stream (tuple (y1 ... ym)))) -> "
  "(stream (tuple (x1 ... xn y1 ... ym)))</text---><text>Computes a Cartesian "
  "product stream from its two argument streams.</text--->)";
/*

4.1.3 Definition of operator ~product~

*/
Operator product (
         "product",            // name
         ProductSpec,          // specification
         Product,              // value mapping
         Operator::DummyModel, // dummy model mapping, defines in Algebra.h
         simpleSelect,         // trivial selection function
         ProductTypeMap        // type mapping
);
/*

7.3 Operator ~cancel~

Transmits tuple from its input stream to its output stream until a tuple
arrives fulfilling some condition.

7.3.1 Type mapping function of operator ~cancel~

Type mapping for ~cancel~ is the same, as type mapping for operator ~filter~.
Result type of cancel operation.

----    ((stream x) (map x bool)) -> (stream x)
----

4.1.2 Value mapping function of operator ~cancel~

*/
static int
Cancel(Word* args, Word& result, int message, Word& local, Supplier s)
{
  Word t, value;
  CcTuple* tuple;
  bool found;
  ArgVectorPointer vector;

  switch (message)
  {
    case OPEN :

      qp->Open(args[0].addr);
      return 0;

    case REQUEST :

      qp->Request(args[0].addr, t);
      found= false;
      if (qp->Received(args[0].addr))
      {
        tuple = (CcTuple*)t.addr;
        vector = qp->Argument(args[1].addr);
        (*vector)[0] = t;
        qp->Request(args[1].addr, value);
        found = ((CcBool*)value.addr)->GetBoolval();
        if (found)
        {
          qp->Close(args[0].addr);
          return CANCEL;
        }
        else
        {
          tuple = tuple->CloneIfNecessary();
          result = SetWord(tuple);
          return YIELD;
        }
      }
      else return CANCEL;

    case CLOSE :

      qp->Close(args[0].addr);
      return 0;
  }
  return 0;
}
/*

4.1.3 Specification of operator ~cancel~

*/
const string CancelSpec =
  "(<text>((stream x) (map x bool)) -> (stream x)</text---><text>Transmits "
  "tuple from its input stream to its output stream until a tuple arrives "
  "fulfilling some condition.</text--->)";
/*

4.1.3 Definition of operator ~cancel~

*/
Operator cancel (
         "cancel",             // name
         CancelSpec,           // specification
         Cancel,               // value mapping
         Operator::DummyModel, // dummy model mapping, defines in Algebra.h
         simpleSelect,         // trivial selection function
         FilterTypeMap         // type mapping
);
/*

7.3 Operator ~tcount~

Count the number of tuples within a stream of tuples.


7.3.1 Type mapping function of operator ~tcount~

Operator ~tcount~ accepts a stream of tuples and returns an integer.

----    (stream  x)                 -> int
----

*/
ListExpr TCountTypeMap(ListExpr args)
{
  ListExpr first ;

  if(nl->ListLength(args) == 1)
  {
    first = nl->First(args);
    if(nl->ListLength(first) == 2)
    {
      if (TypeOfRelAlgSymbol(nl->First(first)) == stream
          || TypeOfRelAlgSymbol(nl->First(first)) == rel)
        return nl->SymbolAtom("int");
    }
  }
  return nl->SymbolAtom("typeerror");
}


/*

4.1.2 Value mapping functions of operator ~tcount~

*/
static int
TCountStream(Word* args, Word& result, int message, Word& local, Supplier s)
{
  Word elem;
  int count = 0;

  qp->Open(args[0].addr);
  qp->Request(args[0].addr, elem);
  while ( qp->Received(args[0].addr) )
  {
    ((CcTuple*)elem.addr)->DeleteIfAllowed();
    count++;
    qp->Request(args[0].addr, elem);
  }
  result = qp->ResultStorage(s);
  ((CcInt*) result.addr)->Set(true, count);
  qp->Close(args[0].addr);
  return 0;
}

static int
TCountRel(Word* args, Word& result, int message, Word& local, Supplier s)
{
  CcRel* rel = (CcRel*)args[0].addr;
  result = qp->ResultStorage(s);
  ((CcInt*) result.addr)->Set(true, rel->GetNoTuples());
  return 0;
}


/*

4.1.3 Specification of operator ~tcount~

*/
const string TCountSpec =
  "(<text>((stream/rel (tuple x))) -> int</text---><text>Count number of tuples "
  "within a stream or a relation of tuples.</text--->)";

/*

4.3.1 Selection function of operator ~tcount~

*/

static int
TCountSelect( ListExpr args )
{
  ListExpr first ;

  if(nl->ListLength(args) == 1)
  {
    first = nl->First(args);
    if(nl->ListLength(first) == 2)
    {
      if (TypeOfRelAlgSymbol(nl->First(first)) == stream)
      {
        return 0;
      }
      else
      {
        if(TypeOfRelAlgSymbol(nl->First(first)) == rel)
        {
          return 1;
        }
      }
    }
  }
  return -1;
}

/*

4.1.3 Definition of operator ~tcount~

*/
static Word
RelNoModelMapping( ArgVector arg, Supplier opTreeNode )
{
  return (SetWord( Address( 0 ) ));
}

ValueMapping tcountmap[] = {TCountStream, TCountRel };
ModelMapping nomodelmap[] = {RelNoModelMapping, RelNoModelMapping};

Operator tcount (
         "tcount",           // name
         TCountSpec,         // specification
         2,                  // number of value mapping functions
         tcountmap,          // value mapping functions
         nomodelmap,         // dummy model mapping functions
         TCountSelect,       // trivial selection function
         TCountTypeMap       // type mapping
);
/*

7.3 Operator ~rename~

Renames all attribute names by adding them with the postfix passed as parameter.

7.3.1 Type mapping function of operator ~rename~

Type mapping for ~rename~ is

----	((stream (tuple([a1:d1, ... ,an:dn)))ar) ->
           (stream (tuple([a1ar:d1, ... ,anar:dn)))
----

*/
static ListExpr
RenameTypeMap( ListExpr args )
{
  ListExpr first, first2, second, rest, listn, lastlistn;
  string  attrname;
  string  attrnamen;
  bool firstcall = true;
  if(nl->ListLength(args) == 2)
  {
    first = nl->First(args);
    second  = nl->Second(args);

    if((nl->ListLength(first) == 2  ) &&
    	(TypeOfRelAlgSymbol(nl->First(first)) == stream) &&
	(TypeOfRelAlgSymbol(nl->First(nl->Second(first))) == tuple) &&
       	nl->IsAtom(second) &&
	(nl->AtomType(second) == SymbolType))
    {
      rest = nl->Second(nl->Second(first));
      while (!(nl->IsEmpty(rest)))
      {
	first2 = nl->First(rest);
	rest = nl->Rest(rest);
	nl->SymbolValue(nl->First(first));
	attrname = nl->SymbolValue(nl->First(first2));
	attrnamen = nl->SymbolValue(second);
	attrname.append(attrnamen);

	if (!firstcall)
	{
	  lastlistn  =
	    nl->Append(lastlistn,
        nl->TwoElemList(nl->SymbolAtom(attrname), nl->Second(first2)));
	}
	else
	{
	  firstcall = false;
 	  listn = nl->OneElemList(nl->TwoElemList(nl->SymbolAtom(attrname),
        nl->Second(first2)));
	  lastlistn = listn;
	}
      }
      return
        nl->TwoElemList(nl->SymbolAtom("stream"),
		nl->TwoElemList(nl->SymbolAtom("tuple"),
		listn));
    }
    return nl->SymbolAtom("typeerror");
  }
  return nl->SymbolAtom("typeerror");
}
/*

4.1.2 Value mapping function of operator ~rename~

*/
static int
Rename(Word* args, Word& result, int message, Word& local, Supplier s)
{
  Word t;
  CcTuple* tuple;

  switch (message)
  {
    case OPEN :

      qp->Open(args[0].addr);
      return 0;

    case REQUEST :

      qp->Request(args[0].addr,t);
      if (qp->Received(args[0].addr))
      {
        tuple = (CcTuple*)t.addr;
        tuple = tuple->CloneIfNecessary();
        result = SetWord(tuple);
        return YIELD;
      }
      else return CANCEL;

    case CLOSE :

      qp->Close(args[0].addr);
      return 0;
  }
  return 0;
}
/*

4.1.3 Specification of operator ~rename~

*/
const string RenameSpec =
  "(<text>((stream (tuple([a1:d1, ... ,an:dn)))ar) -> (stream (tuple([a1ar:d1, "
  "... ,anar:dn)))</text---><text>Renames all attribute names by adding them "
  "with the postfix passed as parameter. NOTE: parameter must "
  "be of symbol type.</text--->)";
/*

4.1.3 Definition of operator ~rename~

*/
Operator cpprename (
         "rename",             // name
         RenameSpec,           // specification
         Rename,               // value mapping
         Operator::DummyModel, // dummy model mapping, defines in Algebra.h
         simpleSelect,         // trivial selection function
         RenameTypeMap         // type mapping
);
/*

7.3 Operator ~extract~

This operator has a stream of tuples and the name of an attribut as input and
returns the value of this attribute
from the first tuple of the input stream. If the input stream is empty a run
time error occurs. In this case value -1 will be returned.

7.3.1 Type mapping function of operator ~extract~

Type mapping for ~extract~ is

----	((stream (tuple ((x1 t1)...(xn tn))) xi) 	-> ti
							APPEND (i) ti)
----

*/
static ListExpr
ExtractTypeMap( ListExpr args )
{
  ListExpr first, second, attrtype;
  string  attrname;
  int j;

  if(nl->ListLength(args) == 2)
  {
    first = nl->First(args);
    second  = nl->Second(args);

    if((nl->ListLength(first) == 2  ) &&
       (TypeOfRelAlgSymbol(nl->First(first)) == stream)  &&
       (TypeOfRelAlgSymbol(nl->First(nl->Second(first))) == tuple)  &&
	(nl->IsAtom(second)) &&
       	(nl->AtomType(second) == SymbolType))
    {
      attrname = nl->SymbolValue(second);
      j = findattr(nl->Second(nl->Second(first)), attrname, attrtype);
      if (j)
	return nl->ThreeElemList(nl->SymbolAtom("APPEND"),
			       nl->OneElemList(nl->IntAtom(j)), attrtype);
    }
    return nl->SymbolAtom("typeerror");
  }
  return nl->SymbolAtom("typeerror");
}
/*

4.1.2 Value mapping function of operator ~extract~

The argument vector ~args~ contains in the first slot ~args[0]~ the tuple
and in ~args[2]~ the position of the attribute as a number. Returns as
~result~ the value of an attribute at the given position ~args[2]~ in a
tuple object.

*/
static int
Extract(Word* args, Word& result, int message, Word& local, Supplier s)
{
  Word t;
  CcTuple* tupleptr;
  int index;
  StandardAttribute* res = (StandardAttribute*)((qp->ResultStorage(s)).addr);
  result = SetWord(res);

  qp->Open(args[0].addr);

  qp->Request(args[0].addr,t);

  if (qp->Received(args[0].addr))
  {
    tupleptr = (CcTuple*)t.addr;
    index = (int)((StandardAttribute*)args[2].addr)->GetValue();
    if ((1 <= index) && (index <= tupleptr->GetNoAttrs()))
    {
      res->CopyFrom((StandardAttribute*)tupleptr->Get(index - 1));
      tupleptr->DeleteIfAllowed();
      return 0;
    }
    else
    {
      tupleptr->DeleteIfAllowed();
      cout << "extract: index out of range !";
      return -1;
    }
  }
  else
    return -1;
}
/*

4.1.3 Specification of operator ~extract~

*/
const string ExtractSpec =
  "(<text>((stream (tuple([a1:d1, ... ,an:dn]))) x ai) -> di</text--->"
  "<text>Returns the value of attribute ai of the first tuple in the "
  "input stream.</text--->)";
/*

4.1.3 Definition of operator ~extract~

*/
Operator cppextract (
         "extract",             // name
         ExtractSpec,           // specification
         Extract,               // value mapping
         Operator::DummyModel,  // dummy model mapping, defines in Algebra.h
         simpleSelect,          // trivial selection function
         ExtractTypeMap         // type mapping
);

/*

7.3 Operator ~head~

This operator fetches the first n tuples from a stream.

7.3.1 Type mapping function of operator ~head~

Type mapping for ~head~ is

----	((stream (tuple ((x1 t1)...(xn tn))) int) 	->
							((stream (tuple ((x1 t1)...(xn tn))))
----

*/
static ListExpr
HeadTypeMap( ListExpr args )
{
  ListExpr first, second;

  if(nl->ListLength(args) == 2)
  {
    first = nl->First(args);
    second  = nl->Second(args);

    if((nl->ListLength(first) == 2  )
      && (TypeOfRelAlgSymbol(nl->First(first)) == stream)
      && (TypeOfRelAlgSymbol(nl->First(nl->Second(first))) == tuple)
      && IsTupleDescription(nl->Second(nl->Second(first)))
      && (nl->IsAtom(second))
      && (nl->AtomType(second) == SymbolType)
      && nl->SymbolValue(second) == "int")
    {
      return first;
    }
    return nl->SymbolAtom("typeerror");
  }
  return nl->SymbolAtom("typeerror");
}
/*

4.1.2 Value mapping function of operator ~head~

*/
static int
Head(Word* args, Word& result, int message, Word& local, Supplier s)
{
  int maxTuples;
  Word tupleWord;
  CcTuple* tuple;

  switch(message)
  {
    case OPEN:
      qp->Open(args[0].addr);
      local.ival = 0;
      return 0;
    case REQUEST:
      maxTuples = (int)((StandardAttribute*)args[1].addr)->GetValue();
      if(local.ival >= maxTuples)
      {
        return CANCEL;
      }

      qp->Request(args[0].addr, tupleWord);
      if(qp->Received(args[0].addr))
      {
        tuple = (CcTuple*)tupleWord.addr;
        tuple = tuple->CloneIfNecessary();
        result = SetWord(tuple);
        local.ival++;
        return YIELD;
      }
      else
      {
        return CANCEL;
      }
    case CLOSE:
      qp->Close(args[0].addr);
      return 0;
  }
  return 0;
}
/*

4.1.3 Specification of operator ~head~

*/
const string HeadSpec =
  "(<text>((stream (tuple([a1:d1, ... ,an:dn]))) x int) -> (stream "
  "(tuple([a1:d1, ... ,an:dn])))</text---><text>Returns the first n tuples "
  "in the input stream.</text--->)";
/*

4.1.3 Definition of operator ~head~

*/
Operator cpphead (
         "head",             // name
         HeadSpec,           // specification
         Head,               // value mapping
         Operator::DummyModel,  // dummy model mapping, defines in Algebra.h
         simpleSelect,          // trivial selection function
         HeadTypeMap         // type mapping
);

/*

7.3 Operators ~max~ and ~min~


7.3.1 Type mapping function of Operators ~max~ and ~min~

Type mapping for ~max~ and ~min~ is

----	((stream (tuple ((x1 t1)...(xn tn))) xi) 	-> ti
							APPEND (i ti)
----

*/
static ListExpr
MaxMinTypeMap( ListExpr args )
{
  ListExpr first, second, attrtype;
  string  attrname;
  int j;

  if(nl->ListLength(args) == 2)
  {
    first = nl->First(args);
    second  = nl->Second(args);

    if((nl->ListLength(first) == 2  )
      && (TypeOfRelAlgSymbol(nl->First(first)) == stream)
      && (TypeOfRelAlgSymbol(nl->First(nl->Second(first))) == tuple)
      && IsTupleDescription(nl->Second(nl->Second(first)))
      && (nl->IsAtom(second))
      && (nl->AtomType(second) == SymbolType))
    {
      attrname = nl->SymbolValue(second);
      j = findattr(nl->Second(nl->Second(first)), attrname, attrtype);

      if (j > 0
        && (nl->SymbolValue(attrtype) == "real"
          || nl->SymbolValue(attrtype) == "string"
          || nl->SymbolValue(attrtype) == "bool"
          || nl->SymbolValue(attrtype) == "int"))
      {
        return nl->ThreeElemList(nl->SymbolAtom("APPEND"),
          nl->OneElemList(nl->IntAtom(j)), attrtype);
      }
    }
    return nl->SymbolAtom("typeerror");
  }
  return nl->SymbolAtom("typeerror");
}
/*

4.1.2 Value mapping function of operators ~max~ and ~min~

*/

template<bool isMax> int
MaxMinValueMapping(Word* args, Word& result, int message, Word& local, Supplier s)
{
  bool definedValueFound = false;
  Word currentTupleWord;
  StandardAttribute* extremum = (StandardAttribute*)(qp->ResultStorage(s)).addr;
  extremum->SetDefined(false);
  result = SetWord(extremum);

  assert(args[2].addr != 0);
  int attributeIndex = (int)((StandardAttribute*)args[2].addr)->GetValue() - 1;

  qp->Open(args[0].addr);
  qp->Request(args[0].addr, currentTupleWord);
  while(qp->Received(args[0].addr))
  {
    CcTuple* currentTuple = (CcTuple*)currentTupleWord.addr;
    StandardAttribute* currentAttr =
      (StandardAttribute*)currentTuple->Get(attributeIndex);
    if(currentAttr->IsDefined())
    {
      if(definedValueFound)
      {
        if(isMax)
        {
          if(currentAttr->Compare(extremum) > 0)
          {
            extremum->CopyFrom(currentAttr);
          }
        }
        else
        {
          if(currentAttr->Compare(extremum) < 0)
          {
            extremum->CopyFrom(currentAttr);
          }
        }
      }
      else
      {
        definedValueFound = true;
        extremum->CopyFrom(currentAttr);
      }
    }
    currentTuple->DeleteIfAllowed();
    qp->Request(args[0].addr, currentTupleWord);
  }
  qp->Close(args[0].addr);

  return 0;
}
/*

4.1.3 Specification of operator ~max~

*/
const string MaxOpSpec =
  "(<text>((stream (tuple([a1:d1, ... ,an:dn]))) x ai) -> di</text--->"
  "<text>Returns the maximum value of attribute ai over the input "
  "stream.</text--->)";
/*

4.1.3 Definition of operator ~max~

*/
Operator cppmax (
         "max",             // name
         MaxOpSpec,           // specification
         MaxMinValueMapping<true>,               // value mapping
         Operator::DummyModel,  // dummy model mapping, defines in Algebra.h
         simpleSelect,          // trivial selection function
         MaxMinTypeMap         // type mapping
);

/*

4.1.3 Specification of operator ~min~

*/
const string MinOpSpec =
  "(<text>((stream (tuple([a1:d1, ... ,an:dn]))) x ai) -> di</text--->"
  "<text>Returns the minimum value of attribute ai over the input "
  "stream.</text--->)";
/*

4.1.3 Definition of operator ~min~

*/
Operator cppmin (
         "min",             // name
         MinOpSpec,           // specification
         MaxMinValueMapping<false>,               // value mapping
         Operator::DummyModel,  // dummy model mapping, defines in Algebra.h
         simpleSelect,          // trivial selection function
         MaxMinTypeMap         // type mapping
);

/*

7.3 Operators ~avg~ and ~sum~


7.3.1 Type mapping function of Operators ~avg~ and ~sum~

Type mapping for ~avg~ is

----	((stream (tuple ((x1 t1)...(xn tn))) xi) 	-> real
							APPEND (i ti)
----

Type mapping for ~sum~ is

----	((stream (tuple ((x1 t1)...(xn tn))) xi) 	-> ti
							APPEND (i ti)
----

*/

template<bool isAvg> ListExpr
AvgSumTypeMap( ListExpr args )
{
  ListExpr first, second, attrtype;
  string  attrname;
  int j;

  if(nl->ListLength(args) == 2)
  {
    first = nl->First(args);
    second  = nl->Second(args);

    if((nl->ListLength(first) == 2  )
      && (TypeOfRelAlgSymbol(nl->First(first)) == stream)
      && (TypeOfRelAlgSymbol(nl->First(nl->Second(first))) == tuple)
      && IsTupleDescription(nl->Second(nl->Second(first)))
      && (nl->IsAtom(second))
      && (nl->AtomType(second) == SymbolType))
    {
      attrname = nl->SymbolValue(second);
      j = findattr(nl->Second(nl->Second(first)), attrname, attrtype);

      if (j > 0
        && (nl->SymbolValue(attrtype) == "real"
          || nl->SymbolValue(attrtype) == "int"))
      {
        return nl->ThreeElemList(nl->SymbolAtom("APPEND"),
          nl->TwoElemList(nl->IntAtom(j),
            nl->StringAtom(nl->SymbolValue(attrtype))),
            isAvg ? nl->SymbolAtom("real") : attrtype);
      }
    }
    return nl->SymbolAtom("typeerror");
  }
  return nl->SymbolAtom("typeerror");
}

/*

4.1.2 Value mapping function of operators ~avg~ and ~sum~

*/
template<bool isAvg> int
AvgSumValueMapping(Word* args, Word& result, int message, Word& local, Supplier s)
{
  bool definedValueFound = false;
  Word currentTupleWord;
  Attribute* accumulated = 0;
  int nProcessedItems = 0;

  assert(args[2].addr != 0);
  assert(args[3].addr != 0);

  int attributeIndex = (int)((StandardAttribute*)args[2].addr)->GetValue() - 1;
  char* attributeType = (char*)((StandardAttribute*)args[3].addr)->GetValue();

  qp->Open(args[0].addr);
  qp->Request(args[0].addr, currentTupleWord);
  while(qp->Received(args[0].addr))
  {
    nProcessedItems++;

    CcTuple* currentTuple = (CcTuple*)currentTupleWord.addr;
    Attribute* currentAttr = (Attribute*)currentTuple->Get(attributeIndex);
    if(currentAttr->IsDefined())
    {
      if(definedValueFound)
      {
        if(strcmp(attributeType, "real") == 0)
        {
          CcReal* accumulatedReal = (CcReal*)accumulated;
          CcReal* currentReal = (CcReal*)currentAttr;
          accumulatedReal->Set(currentReal->GetRealval()
            + accumulatedReal->GetRealval());
        }
        else
        {
          CcInt* accumulatedInt = (CcInt*)accumulated;
          CcInt* currentInt = (CcInt*)currentAttr;
          accumulatedInt->Set(currentInt->GetIntval()
            + accumulatedInt->GetIntval());
        }
      }
      else
      {
        definedValueFound = true;
        if(isAvg)
        {
          accumulated = currentAttr->Clone();
        }
        else
        {
          accumulated = (Attribute*)qp->ResultStorage(s).addr;
        }
      }
    }
    currentTuple->DeleteIfAllowed();
    qp->Request(args[0].addr, currentTupleWord);
  }
  qp->Close(args[0].addr);

  if(definedValueFound)
  {
    if(isAvg)
    {
      CcReal* resultAttr = (CcReal*)(qp->ResultStorage(s).addr);
      float nItems = (float)nProcessedItems;

      if(strcmp(attributeType, "real") == 0)
      {
        CcReal* accumulatedReal = (CcReal*)accumulated;
        resultAttr->Set(accumulatedReal->GetRealval() / nItems);
      }
      else
      {
        CcInt* accumulatedInt = (CcInt*)accumulated;
        resultAttr->Set(((float)accumulatedInt->GetIntval()) / nItems);
      }
      delete accumulated;
      result = SetWord(resultAttr);
    }
    else
    {
      result = SetWord(accumulated);
    }
    return 0;
  }
  else
  {
    ((StandardAttribute*)qp->ResultStorage(s).addr)->SetDefined(false);
    result = qp->ResultStorage(s);
    return 0;
  }
}

/*

4.1.3 Specification of operator ~avg~

*/
const string AvgOpSpec =
  "(<text>((stream (tuple([a1:d1, ... ,an:dn]))) x ai) -> real</text--->"
  "<text>Returns the average value of attribute ai over the "
  "input stream.</text--->)";
/*

4.1.3 Definition of operator ~avg~

*/
Operator cppavg (
         "avg",             // name
         AvgOpSpec,           // specification
         AvgSumValueMapping<true>,               // value mapping
         Operator::DummyModel,  // dummy model mapping, defines in Algebra.h
         simpleSelect,          // trivial selection function
         AvgSumTypeMap<true>         // type mapping
);

/*

4.1.3 Specification of operator ~sum~

*/
const string SumOpSpec =
  "(<text>((stream (tuple([a1:d1, ... ,an:dn]))) x ai) -> di</text--->"
  "<text>Returns the sum of the values of attribute ai over the "
  "input stream.</text--->)";
/*

4.1.3 Definition of operator ~sum~

*/
Operator cppsum (
         "sum",             // name
         SumOpSpec,           // specification
         AvgSumValueMapping<false>,               // value mapping
         Operator::DummyModel,  // dummy model mapping, defines in Algebra.h
         simpleSelect,          // trivial selection function
         AvgSumTypeMap<false>         // type mapping
);

/*

7.3 Operator ~sortBy~

This operator sorts a stream of tuples by a given list of attributes.
For each attribute it must be specified wether the list should be sorted
in ascending (asc) or descending (desc) order with regard to that attribute.

7.3.1 Type mapping function of operator ~sortBy~

Type mapping for ~sortBy~ is

----	((stream (tuple ((x1 t1)...(xn tn))) ((xi1 asc/desc) ... (xij asc/desc))) 	
              -> (stream (tuple ((x1 t1)...(xn tn)))
                  APPEND (j i1 asc/desc i2 asc/desc ... ij asc/desc)
----

*/

static char* sortAscending = "asc";
static char* sortDescending = "desc";

static ListExpr
SortByTypeMap( ListExpr args )
{
  ListExpr attrtype;
  string  attrname;

  if(nl->ListLength(args) == 2)
  {
    ListExpr streamDescription = nl->First(args);
    ListExpr sortSpecification  = nl->Second(args);

    if((nl->ListLength(streamDescription) == 2  ) &&
      (TypeOfRelAlgSymbol(nl->First(streamDescription)) == stream)  &&
      (TypeOfRelAlgSymbol(nl->First(nl->Second(streamDescription))) == tuple))
    {
      int numberOfSortAttrs = nl->ListLength(sortSpecification);
      if(numberOfSortAttrs > 0)
      {
        ListExpr sortOrderDescription = nl->OneElemList(nl->IntAtom(numberOfSortAttrs));
        ListExpr sortOrderDescriptionLastElement = sortOrderDescription;
        ListExpr rest = sortSpecification;
        while(!nl->IsEmpty(rest))
        {
          ListExpr attributeSpecification = nl->First(rest);
          rest = nl->Rest(rest);
          if((nl->ListLength(attributeSpecification) == 2)
            && (nl->IsAtom(nl->First(attributeSpecification)))
            && (nl->AtomType(nl->First(attributeSpecification)) == SymbolType)
            && (nl->IsAtom(nl->Second(attributeSpecification)))
            && (nl->AtomType(nl->Second(attributeSpecification)) == SymbolType))
          {
            attrname = nl->SymbolValue(nl->First(attributeSpecification));
            int j = findattr(nl->Second(nl->Second(streamDescription)), attrname, attrtype);
            if ((j > 0)
              && ((nl->SymbolValue(nl->Second(attributeSpecification)) == sortAscending)
                  || (nl->SymbolValue(nl->Second(attributeSpecification)) == sortDescending)))
            {
              sortOrderDescriptionLastElement =
                nl->Append(sortOrderDescriptionLastElement, nl->IntAtom(j));
              bool isAscending =
                nl->SymbolValue(nl->Second(attributeSpecification)) == sortAscending;
              sortOrderDescriptionLastElement =
                nl->Append(sortOrderDescriptionLastElement,
                  nl->BoolAtom(isAscending));
            }
            else
            {
              return nl->SymbolAtom("typeerror");
            }
          }
          else
          {
            return nl->SymbolAtom("typeerror");
          }
        }
        return nl->ThreeElemList(nl->SymbolAtom("APPEND"),
              sortOrderDescription, streamDescription);
      };
      return nl->SymbolAtom("typeerror");
    }
    return nl->SymbolAtom("typeerror");
  }
  return nl->SymbolAtom("typeerror");
}

/*

4.1.2 Value mapping function of operator ~sortBy~

The argument vector ~args~ contains in the first slot ~args[0]~ the stream and
in ~args[2]~ the number of sort attributes. ~args[3]~ contains the index of the first
sort attribute, ~args[4]~ a boolean indicating wether the stream is sorted in
ascending order with regard to the sort first attribute. ~args[5]~ and ~args[6]~
contain these values for the second sort attribute  and so on.

*/

typedef vector< pair<int, bool> > SortOrderSpecification;

class CcTupleCmp
{
public:
  SortOrderSpecification spec;
  bool operator()(const CcTuple* aConst, const CcTuple* bConst) const
  {
    CcTuple* a = (CcTuple*)aConst;
    CcTuple* b = (CcTuple*)bConst;

    SortOrderSpecification::const_iterator iter = spec.begin();
    while(iter != spec.end())
    {
      if(((Attribute*)a->Get(iter->first - 1))->
        Compare(((Attribute*)b->Get(iter->first - 1))) < 0)
      {
        return iter->second;
      }
      else
      {
        if(((Attribute*)a->Get(iter->first - 1))->
          Compare(((Attribute*)b->Get(iter->first - 1))) > 0)
        {
          return !(iter->second);
        }
      }
      iter++;
    }
    return false;
  }
};

struct SortByLocalInfo
{
  vector<CcTuple*>* tuples;
  size_t currentIndex;
};

template<bool lexicographically> int
SortBy(Word* args, Word& result, int message, Word& local, Supplier s)
{
  Word tuple;
  vector<CcTuple*>* tuples;
  SortByLocalInfo* localInfo;
  SortOrderSpecification spec;
  int i;
  size_t j;
  int sortAttrIndex;
  int nSortAttrs;
  bool sortOrderIsAscending;
  CcTupleCmp ccCmp;
  LexicographicalCcTupleCmp lCcCmp;
  CcTuple* t;

  switch(message)
  {
    case OPEN:
      tuples = new vector<CcTuple*>;
      qp->Open(args[0].addr);
      qp->Request(args[0].addr,tuple);
      while(qp->Received(args[0].addr))
      {
        t =(CcTuple*)tuple.addr;
        t = t->CloneIfNecessary();
        tuples->push_back(t);
        qp->Request(args[0].addr,tuple);
      }

      if(lexicographically)
      {
        sort(tuples->begin(), tuples->end(), lCcCmp);
      }
      else
      {
        nSortAttrs = (int)((StandardAttribute*)args[2].addr)->GetValue();
        for(i = 1; i <= nSortAttrs; i++)
        {
          sortAttrIndex =
            (int)((StandardAttribute*)args[2 * i + 1].addr)->GetValue();
          sortOrderIsAscending =
            (bool*)((StandardAttribute*)args[2 * i + 2].addr)->GetValue();
          spec.push_back(pair<int, bool>(sortAttrIndex, sortOrderIsAscending));
        };
        ccCmp.spec = spec;
        sort(tuples->begin(), tuples->end(), ccCmp);
      }

      localInfo = new SortByLocalInfo;
      localInfo->tuples = tuples;
      localInfo->currentIndex = 0;
      local = SetWord(localInfo);
      return 0;
    case REQUEST:
      localInfo = (SortByLocalInfo*)local.addr;
      tuples = localInfo->tuples;
      if(localInfo->currentIndex <= tuples->size() - 1)
      {
        result = SetWord((*tuples)[localInfo->currentIndex]);
        localInfo->currentIndex++;
        return YIELD;
      }
      else
      {
        return CANCEL;
      }
    case CLOSE:
      localInfo = (SortByLocalInfo*)local.addr;

      for(j = localInfo->currentIndex;
        j <= localInfo->tuples->size() - 1; j++)
      {
        (*(localInfo->tuples))[j]->DeleteIfAllowed();
      }

      delete localInfo->tuples;
      delete localInfo;
      return 0;
  }
  return 0;
}
/*

4.1.3 Specification of operator ~sortBy~

*/
const string SortBySpec =
  "(<text>((stream (tuple([a1:d1, ... ,an:dn]))) ((xi1 asc/desc) ... "
  "(xij asc/desc))) -> (stream (tuple([a1:d1, ... ,an:dn])))</text--->"
  "<text>Sorts input stream according to a list of attributes "
  "ai1 ... aij.</text--->)";
/*

4.1.3 Definition of operator ~sortBy~

*/
Operator sortBy (
         "sortby",             // name
         SortBySpec,           // specification
         SortBy<false>,               // value mapping
         Operator::DummyModel,  // dummy model mapping, defines in Algebra.h
         simpleSelect,          // trivial selection function
         SortByTypeMap         // type mapping
);

/*

7.3 Operator ~sort~

This operator sorts a stream of tuples lexicographically.

7.3.1 Type mapping function of operator ~sort~

Type mapping for ~sort~ is

----	((stream (tuple ((x1 t1)...(xn tn)))) 	-> (stream (tuple ((x1 t1)...(xn tn)))

----

*/
static ListExpr
IdenticalTypeMap( ListExpr args )
{
  ListExpr first;

  if(nl->ListLength(args) == 1)
  {
    first = nl->First(args);

    if((nl->ListLength(first) == 2  )
      && (TypeOfRelAlgSymbol(nl->First(first)) == stream)
      && (TypeOfRelAlgSymbol(nl->First(nl->Second(first))) == tuple)
      && IsTupleDescription(nl->Second(nl->Second(first))))
    {
      return first;
    }
    return nl->SymbolAtom("typeerror");
  }
  return nl->SymbolAtom("typeerror");
}

/*

4.1.3 Specification of operator ~sort~

*/
const string SortSpec =
  "(<text>((stream (tuple([a1:d1, ... ,an:dn])))) -> "
  "(stream (tuple([a1:d1, ... ,an:dn])))</text---><text>Sorts input "
  "stream lexicographically.</text--->)";
/*

4.1.3 Definition of operator ~sort~

*/
Operator cppsort (
         "sort",             // name
         SortSpec,           // specification
         SortBy<true>,               // value mapping
         Operator::DummyModel,  // dummy model mapping, defines in Algebra.h
         simpleSelect,          // trivial selection function
         IdenticalTypeMap         // type mapping
);

/*

7.3 Operator ~rdup~

This operator removes duplicates from a sorted stream.

4.1.2 Value mapping function of operator ~rdup~

*/

static int
RdupValueMapping(Word* args, Word& result, int message, Word& local, Supplier s)
{
  Word tuple;
  LexicographicalCcTupleCmp cmp;
  CcTuple* currentTuple;
  CcTuple* lastOutputTuple;

  switch(message)
  {
    case OPEN:
      qp->Open(args[0].addr);
      local = SetWord(0);
      return 0;
    case REQUEST:
      while(true)
      {
        qp->Request(args[0].addr, tuple);
        if(qp->Received(args[0].addr))
        {
          if(local.addr != 0)
          {
            currentTuple = (CcTuple*)tuple.addr;
            lastOutputTuple = (CcTuple*)local.addr;
            if(cmp(currentTuple, lastOutputTuple)
              || cmp(lastOutputTuple, currentTuple))
            {
              currentTuple = currentTuple->CloneIfNecessary();
              local = SetWord(currentTuple);
              result = SetWord(currentTuple);
              return YIELD;
            }
            else
            {
              currentTuple->DeleteIfAllowed();
            }
          }
          else
          {
            currentTuple = (CcTuple*)tuple.addr;
            currentTuple = currentTuple->CloneIfNecessary();
            local = SetWord(currentTuple);
            result = SetWord(currentTuple);
            return YIELD;
          }
        }
        else
        {
          return CANCEL;
        }
      }
    case CLOSE:
      qp->Close(args[0].addr);
      return 0;
  }
  return 0;
}
/*

4.1.3 Specification of operator ~rdup~

*/
const string RdupSpec =
  "(<text>((stream (tuple([a1:d1, ... ,an:dn])))) -> (stream "
  "(tuple([a1:d1, ... ,an:dn])))</text---><text>Removes duplicates from a "
  "sorted stream.</text--->)";
/*

4.1.3 Definition of operator ~rdup~

*/
Operator cpprdup (
         "rdup",             // name
         RdupSpec,           // specification
         RdupValueMapping,               // value mapping
         Operator::DummyModel,  // dummy model mapping, defines in Algebra.h
         simpleSelect,          // trivial selection function
         IdenticalTypeMap         // type mapping
);

/*

7.3 Set Operators

These operators compute set operations on two sorted stream.

7.3.1 Generic Type Mapping for Set Operations

*/

static ListExpr
SetOpTypeMap( ListExpr args )
{
  ListExpr first, second;

  if(nl->ListLength(args) == 2)
  {
    first = nl->First(args);
    second = nl->Second(args);

    if((nl->ListLength(first) == 2  )
      && (TypeOfRelAlgSymbol(nl->First(first)) == stream)
      && (TypeOfRelAlgSymbol(nl->First(nl->Second(first))) == tuple)
      && IsTupleDescription(nl->Second(nl->Second(first)))
      && (nl->Equal(first, second)))
    {
      return first;
    }
    return nl->SymbolAtom("typeerror");
  }
  return nl->SymbolAtom("typeerror");
}

/*

7.3.2 Auxiliary Class for Set Operations

*/

class SetOperation
{
public:
  bool outputAWithoutB;
  bool outputBWithoutA;
  bool outputMatches;

private:
  LexicographicalCcTupleCmp smallerThan;

  Word streamA;
  Word streamB;

  CcTuple* currentATuple;
  CcTuple* currentBTuple;

  CcTuple* NextATuple(bool deleteOldTuple)
  {
    Word tuple;
    if(deleteOldTuple && currentATuple != 0)
    {
      currentATuple->DeleteIfAllowed();
    }

    qp->Request(streamA.addr, tuple);
    if(qp->Received(streamA.addr))
    {
      tuple = SetWord(((CcTuple*)tuple.addr)->CloneIfNecessary());
      currentATuple = (CcTuple*)tuple.addr;
      return currentATuple;
    }
    else
    {
      currentATuple = 0;
      return 0;
    }
  }

  CcTuple* NextBTuple(bool deleteOldTuple)
  {
    Word tuple;
    if(deleteOldTuple && currentBTuple != 0)
    {
      currentBTuple->DeleteIfAllowed();
    }

    qp->Request(streamB.addr, tuple);
    if(qp->Received(streamB.addr))
    {
      tuple = SetWord(((CcTuple*)tuple.addr)->CloneIfNecessary());
      currentBTuple = (CcTuple*)tuple.addr;
      return currentBTuple;
    }
    else
    {
      currentBTuple = 0;
      return 0;
    }
  }

  bool TuplesEqual(CcTuple* a, CcTuple* b)
  {
    return !(smallerThan(a, b) || smallerThan(b, a));
  }

public:

  SetOperation(Word streamA, Word streamB)
  {
    this->streamA = streamA;
    this->streamB = streamB;

    currentATuple = 0;
    currentBTuple = 0;

    qp->Open(streamA.addr);
    qp->Open(streamB.addr);

    NextATuple(false);
    NextBTuple(false);
  }

  virtual ~SetOperation()
  {
    qp->Close(streamA.addr);
    qp->Close(streamB.addr);
  }

  CcTuple* NextResultTuple()
  {
    CcTuple* result = 0;
    while(result == 0)
    {
      if(currentATuple == 0)
      {
        if(currentBTuple == 0)
        {
          return 0;
        }
        else
        {
          if(outputBWithoutA)
          {
            result = currentBTuple;
            NextBTuple(false);
            while(currentBTuple != 0 && TuplesEqual(result, currentBTuple))
            {
              NextBTuple(true);
            }
          }
          else
          {
            currentBTuple->DeleteIfAllowed();
            return 0;
          }
        }
      }
      else
      {
        if(currentBTuple == 0)
        {
          if(outputAWithoutB)
          {
            result = currentATuple;
            NextATuple(false);
            while(currentATuple != 0 && TuplesEqual(result, currentATuple))
            {
              NextATuple(true);
            }
          }
          else
          {
            currentATuple->DeleteIfAllowed();
            return 0;
          }
        }
        else
        {
          /* both current tuples != 0 */
          if(smallerThan(currentATuple, currentBTuple))
          {
            if(outputAWithoutB)
            {
              result = currentATuple;
            }

            CcTuple* tmp = currentATuple;
            NextATuple(false);
            while(currentATuple != 0 && TuplesEqual(tmp, currentATuple))
            {
              NextATuple(true);
            }
            if(!outputAWithoutB)
            {
              tmp->DeleteIfAllowed();
            }
          }
          else if(smallerThan(currentBTuple, currentATuple))
          {
            if(outputBWithoutA)
            {
              result = currentBTuple;
            }

            CcTuple* tmp = currentBTuple;
            NextBTuple(false);
            while(currentBTuple != 0 && TuplesEqual(tmp, currentBTuple))
            {
              NextBTuple(true);
            }
            if(!outputBWithoutA)
            {
              tmp->DeleteIfAllowed();
            }

          }
          else
          {
            /* found match */
            assert(TuplesEqual(currentATuple, currentBTuple));
            CcTuple* match = currentATuple;
            if(outputMatches)
            {
              result = match;
            }

            NextATuple(false);
            while(currentATuple != 0 && TuplesEqual(match, currentATuple))
            {
              NextATuple(true);
            }
            while(currentBTuple != 0 && TuplesEqual(match, currentBTuple))
            {
              NextBTuple(true);
            }
            if(!outputMatches)
            {
              match->DeleteIfAllowed();
            }
          }
        }
      }
    }
    return result;
  }
};

/*

7.3.2 Generic Value Mapping Function for Set Operations

*/

template<bool outputAWithoutB, bool outputBWithoutA, bool outputMatches> int
SetOpValueMapping(Word* args, Word& result, int message, Word& local, Supplier s)
{
  SetOperation* localInfo;

  switch(message)
  {
    case OPEN:
      localInfo = new SetOperation(args[0], args[1]);
      localInfo->outputBWithoutA = outputBWithoutA;
      localInfo->outputAWithoutB = outputAWithoutB;
      localInfo->outputMatches = outputMatches;

      local = SetWord(localInfo);
      return 0;
    case REQUEST:
      localInfo = (SetOperation*)local.addr;
      result = SetWord(localInfo->NextResultTuple());
      return result.addr != 0 ? YIELD : CANCEL;
    case CLOSE:
      localInfo = (SetOperation*)local.addr;
      delete localInfo;
      return 0;
  }
  return 0;
}

/*

4.1.3 Specification of Operator ~mergesec~

*/
const string MergeSecSpec =
  "(<text>((stream (tuple ((x1 t1) ... (xn tn)))) stream (tuple "
  "((x1 t1) ... (xn tn))))) -> (stream (tuple ((x1 t1) ... (xn tn))))"
  "</text---><text>Computes the intersection of two sorted streams.</text--->)";
/*

4.1.3 Definition of Operator ~mergesec~

*/
Operator cppmergesec(
         "mergesec",        // name
         MergeSecSpec,     // specification
         SetOpValueMapping<false, false, true>,         // value mapping
         Operator::DummyModel,  // dummy model mapping, defines in Algebra.h
         simpleSelect,          // trivial selection function
         SetOpTypeMap   // type mapping
);

/*

4.1.3 Specification of Operator ~mergediff~

*/
const string MergeDiffSpec =
  "(<text>((stream (tuple ((x1 t1) ... (xn tn)))) stream (tuple ((x1 t1) "
  "... (xn tn))))) -> (stream (tuple ((x1 t1) ... (xn tn))))</text--->"
  "<text>Computes the difference of two sorted streams.</text--->)";
/*

4.1.3 Definition of Operator ~mergediff~

*/
Operator cppmergediff(
         "mergediff",        // name
         MergeDiffSpec,     // specification
         SetOpValueMapping<true, false, false>,         // value mapping
         Operator::DummyModel,  // dummy model mapping, defines in Algebra.h
         simpleSelect,          // trivial selection function
         SetOpTypeMap   // type mapping
);

/*

4.1.3 Specification of Operator ~mergeunion~

*/
const string MergeUnionSpec =
  "(<text>((stream (tuple ((x1 t1) ... (xn tn)))) stream (tuple "
  "((x1 t1) ... (xn tn))))) -> (stream (tuple ((x1 t1) ... (xn tn))))"
  "</text---><text>Computes the union of two sorted streams.</text--->)";
/*

4.1.3 Definition of Operator ~mergeunion~

*/
Operator cppmergeunion(
         "mergeunion",        // name
         MergeUnionSpec,     // specification
         SetOpValueMapping<true, true, true>,         // value mapping
         Operator::DummyModel,  // dummy model mapping, defines in Algebra.h
         simpleSelect,          // trivial selection function
         SetOpTypeMap   // type mapping
);

/*

7.3 Operator ~mergejoin~

This operator computes the equijoin two streams.

7.3.1 Type mapping function of operators ~mergejoin~ and ~hashjoin~

Type mapping for ~mergejoin~ is

----	((stream (tuple ((x1 t1) ... (xn tn)))) (stream (tuple ((y1 d1) ... (ym dm)))) xi yj)

      -> (stream (tuple ((x1 t1) ... (xn tn) (y1 d1) ... (ym dm)))) APPEND (i j)
----

Type mapping for ~hashjoin~ is

----	((stream (tuple ((x1 t1) ... (xn tn)))) (stream (tuple ((y1 d1) ... (ym dm)))) xi yj int)

      -> (stream (tuple ((x1 t1) ... (xn tn) (y1 d1) ... (ym dm)))) APPEND (i j)
----


*/
template<bool expectIntArgument> ListExpr JoinTypeMap
(ListExpr args)
{
  ListExpr attrTypeA, attrTypeB;
  ListExpr streamA, streamB, list, list1, list2, outlist;
  if (nl->ListLength(args) == (expectIntArgument ? 5 : 4))
  {
    streamA = nl->First(args); streamB = nl->Second(args);
    if (nl->ListLength(streamA) == 2)
    {
      if (TypeOfRelAlgSymbol(nl->First(streamA)) == stream)
      {
        if (nl->ListLength(nl->Second(streamA)) == 2)
        {
          if (TypeOfRelAlgSymbol(nl->First(nl->Second(streamA))) == tuple)
          {
            list1 = nl->Second(nl->Second(streamA));
          }
          else return nl->SymbolAtom("typeerror");
        }
        else return nl->SymbolAtom("typeerror");
      }
      else return nl->SymbolAtom("typeerror");
    }
    else return nl->SymbolAtom("typeerror");

    if (nl->ListLength(streamB) == 2)
    {
      if (TypeOfRelAlgSymbol(nl->First(streamB)) == stream)
      {
        if (nl->ListLength(nl->Second(streamB)) == 2)
        {
          if (TypeOfRelAlgSymbol(nl->First(nl->Second(streamB))) == tuple)
          {
            list2 = nl->Second(nl->Second(streamB));
          }
          else return nl->SymbolAtom("typeerror");
        }
        else return nl->SymbolAtom("typeerror");
      }
      else return nl->SymbolAtom("typeerror");
    }
    else return nl->SymbolAtom("typeerror");

    if(!AttributesAreDisjoint(list1, list2))
    {
      return nl->SymbolAtom("typeerror");
    }

    list = ConcatLists(list1, list2);
    outlist = nl->TwoElemList(nl->SymbolAtom("stream"),
      nl->TwoElemList(nl->SymbolAtom("tuple"), list));

    ListExpr joinAttrDescription;
    string attrAName = nl->SymbolValue(nl->Third(args));
    string attrBName = nl->SymbolValue(nl->Fourth(args));
    int attrAIndex = findattr(nl->Second(nl->Second(streamA)), attrAName, attrTypeA);
    int attrBIndex = findattr(nl->Second(nl->Second(streamB)), attrBName, attrTypeB);
    if(attrAIndex <= 0 || attrBIndex <= 0 || !nl->Equal(attrTypeA, attrTypeB))
    {
      return nl->SymbolAtom("typeerror");
    }

    if(expectIntArgument && nl->SymbolValue(nl->Fifth(args)) != "int")
    {
      return nl->SymbolAtom("typeerror");
    }

    joinAttrDescription =
      nl->TwoElemList(nl->IntAtom(attrAIndex), nl->IntAtom(attrBIndex));
    return nl->ThreeElemList(nl->SymbolAtom("APPEND"),
              joinAttrDescription, outlist);
  }
  else return nl->SymbolAtom("typeerror");
}

/*

4.1.2 Auxiliary definitions for value mapping function of operator ~mergejoin~

*/

static CcInt oneCcInt(true, 1);
static CcBool trueCcBool(true, true);

class MergeJoinLocalInfo
{
private:
  vector<CcTuple*> bucketA;
  vector<CcTuple*> bucketB;
  deque<CcTuple*> resultBucket;

  Word aResult;
  Word bResult;

  Word streamALocalInfo;
  Word streamBLocalInfo;

  Word streamA;
  Word streamB;

  ArgVector aArgs;
  ArgVector bArgs;

  int attrIndexA;
  int attrIndexB;

  bool expectSorted;

  int CompareCcTuples(CcTuple* a, CcTuple* b)
  {
    return ((Attribute*)a->Get(attrIndexA))->Compare((Attribute*)b->Get(attrIndexB));
  }

  void SetArgs(ArgVector& args, Word stream, Word attrIndex)
  {
    args[0] = SetWord(stream.addr);
    args[2] = SetWord(&oneCcInt);
    args[3] = SetWord(attrIndex.addr);
    args[4] = SetWord(&trueCcBool);
  }

  CcTuple* nextATuple()
  {
    bool yield;
    if(expectSorted)
    {
      qp->Request(streamA.addr, aResult);
      yield = qp->Received(streamA.addr);
    }
    else
    {
      int errorCode = SortBy<false>(aArgs, aResult, REQUEST, streamALocalInfo, 0);
      yield = (errorCode == YIELD);
    }

    if(yield)
    {
      aResult = SetWord(((CcTuple*)aResult.addr)->CloneIfNecessary());
      return (CcTuple*)aResult.addr;
    }
    else
    {
      aResult = SetWord((void*)0);
      return 0;
    }
  }

  CcTuple* nextBTuple()
  {
    bool yield;
    if(expectSorted)
    {
      qp->Request(streamB.addr, bResult);
      yield = qp->Received(streamB.addr);
    }
    else
    {
      int errorCode = SortBy<false>(bArgs, bResult, REQUEST, streamBLocalInfo, 0);
      yield = (errorCode == YIELD);
    }

    if(yield)
    {
      bResult = SetWord(((CcTuple*)bResult.addr)->CloneIfNecessary());
      return (CcTuple*)bResult.addr;
    }
    else
    {
      bResult = SetWord((void*)0);
      return 0;
    }
  }

  bool FetchNextMatch()
  {
    CcTuple* aCcTuple = (CcTuple*)aResult.addr;
    CcTuple* bCcTuple = (CcTuple*)bResult.addr;
    if(aCcTuple == 0 || bCcTuple == 0)
    {
      if(aCcTuple != 0)
        aCcTuple->DeleteIfAllowed();
      if(bCcTuple != 0)
        bCcTuple->DeleteIfAllowed();

      return false;
    }
    else
    {
      int cmpResult = CompareCcTuples((CcTuple*)aResult.addr, (CcTuple*)bResult.addr);
      while(cmpResult != 0)
      {
        if(cmpResult < 0)
        {
          ((CcTuple*)aResult.addr)->DeleteIfAllowed();
          if(nextATuple() == 0)
          {
            ((CcTuple*)bResult.addr)->DeleteIfAllowed();
            return false;
          }
        }
        else
        {
          ((CcTuple*)bResult.addr)->DeleteIfAllowed();
          if(nextBTuple() == 0)
          {
            ((CcTuple*)aResult.addr)->DeleteIfAllowed();
            return false;
          }
        }
        cmpResult = CompareCcTuples((CcTuple*)aResult.addr, (CcTuple*)bResult.addr);
      }
      return true;
    }
  }

  void ComputeProductOfBuckets()
  {
    assert(!bucketA.empty());
    assert(!bucketB.empty());

    vector<CcTuple*>::iterator iterA = bucketA.begin();
    vector<CcTuple*>::iterator iterB = bucketB.begin();
    for(; iterA != bucketA.end(); iterA++)
    {
      for(iterB = bucketB.begin(); iterB != bucketB.end(); iterB++)
      {
        CcTuple* resultTuple = new CcTuple;
        resultTuple->SetFree(true);
        Word resultWord = SetWord(resultTuple);
        Word aWord = SetWord(*iterA);
        Word bWord = SetWord(*iterB);
        Concat(aWord, bWord, resultWord);
        resultBucket.push_back(resultTuple);
      }
    }
  }

  void ClearBuckets()
  {
    vector<CcTuple*>::iterator iterA = bucketA.begin();
    vector<CcTuple*>::iterator iterB = bucketB.begin();

    for(; iterA != bucketA.end(); iterA++)
    {
      (*iterA)->DeleteIfAllowed();
    }

    for(; iterB != bucketB.end(); iterB++)
    {
      (*iterB)->DeleteIfAllowed();
    }

    bucketA.clear();
    bucketB.clear();
  }

  void FillResultBucket()
  {
    assert((CcTuple*)aResult.addr != 0);
    assert((CcTuple*)bResult.addr != 0);

    CcTuple* aMatch = (CcTuple*)aResult.addr;
    CcTuple* bMatch = (CcTuple*)bResult.addr;
    assert(CompareCcTuples(aMatch, bMatch) == 0);

    CcTuple* currentA = aMatch;
    CcTuple* currentB = bMatch;

    while(currentA != 0 && CompareCcTuples(currentA, bMatch) == 0)
    {
      bucketA.push_back(currentA);
      currentA = nextATuple();
    }

    while(currentB != 0 && CompareCcTuples(aMatch, currentB) == 0)
    {
      bucketB.push_back(currentB);
      currentB = nextBTuple();
    }

    ComputeProductOfBuckets();
    ClearBuckets();
  }

public:
  MergeJoinLocalInfo(Word streamA, Word attrIndexA,
    Word streamB, Word attrIndexB, bool expectSorted)
  {
    assert(streamA.addr != 0);
    assert(streamB.addr != 0);
    assert(attrIndexA.addr != 0);
    assert(attrIndexB.addr != 0);
    assert((int)((StandardAttribute*)attrIndexA.addr)->GetValue() > 0);
    assert((int)((StandardAttribute*)attrIndexB.addr)->GetValue() > 0);

    aResult = SetWord(0);
    bResult = SetWord(0);

    this->expectSorted = expectSorted;
    this->streamA = streamA;
    this->streamB = streamB;
    this->attrIndexA = (int)((StandardAttribute*)attrIndexA.addr)->GetValue() - 1;
    this->attrIndexB = (int)((StandardAttribute*)attrIndexB.addr)->GetValue() - 1;

    if(expectSorted)
    {
      qp->Open(streamA.addr);
      qp->Open(streamB.addr);
    }
    else
    {
      SetArgs(aArgs, streamA, attrIndexA);
      SetArgs(bArgs, streamB, attrIndexB);
      SortBy<false>(aArgs, aResult, OPEN, streamALocalInfo, 0);
      SortBy<false>(bArgs, bResult, OPEN, streamBLocalInfo, 0);
    }

    nextATuple();
    nextBTuple();
  }

  ~MergeJoinLocalInfo()
  {
    if(expectSorted)
    {
      qp->Close(streamA.addr);
      qp->Close(streamB.addr);
    }
    else
    {
      SortBy<false>(aArgs, aResult, CLOSE, streamALocalInfo, 0);
      SortBy<false>(bArgs, bResult, CLOSE, streamBLocalInfo, 0);
    };
  }

  CcTuple* NextResultTuple()
  {
    if(resultBucket.empty())
    {
      if(FetchNextMatch())
      {
        FillResultBucket();
      }
      else
      {
        return 0;
      }
    }
    CcTuple* next = resultBucket.front();
    resultBucket.pop_front();
    return next;
  }
};

/*

4.1.2 Value mapping function of operator ~mergejoin~

*/

template<bool expectSorted> int
MergeJoin(Word* args, Word& result, int message, Word& local, Supplier s)
{
  MergeJoinLocalInfo* localInfo;

  switch(message)
  {
    case OPEN:
      localInfo = new MergeJoinLocalInfo
        (args[0], args[4], args[1], args[5], expectSorted);
      local = SetWord(localInfo);
      return 0;
    case REQUEST:
      localInfo = (MergeJoinLocalInfo*)local.addr;
      result = SetWord(localInfo->NextResultTuple());
      return result.addr != 0 ? YIELD : CANCEL;
    case CLOSE:
      localInfo = (MergeJoinLocalInfo*)local.addr;
      delete localInfo;
      return 0;
  }
  return 0;
}

/*

4.1.3 Specification of operator ~mergejoin~

*/
const string MergeJoinSpec =
  "(<text>((stream (tuple ((x1 t1) ... (xn tn)))) (stream (tuple ((y1 d1) "
  "... (ym dm)))) xi yj) -> (stream (tuple ((x1 t1) ... (xn tn) (y1 d1) ... "
  "(ym dm))))</text---><text>Computes the equijoin two streams. Expects that "
  "input streams are sorted.</text--->)";
/*

4.1.3 Definition of operator ~mergejoin~

*/
Operator MergeJoinOperator(
         "mergejoin",        // name
         MergeJoinSpec,     // specification
         MergeJoin<true>,         // value mapping
         Operator::DummyModel,  // dummy model mapping, defines in Algebra.h
         simpleSelect,          // trivial selection function
         JoinTypeMap<false>   // type mapping
);

/*

7.3 Operator ~sortmergejoin~

This operator sorts two input streams and computes their equijoin.

4.1.3 Specification of operator ~sortmergejoin~

*/
const string SortMergeJoinSpec =
  "(<text>((stream (tuple ((x1 t1) ... (xn tn)))) (stream (tuple "
  "((y1 d1) ... (ym dm)))) xi yj) -> (stream (tuple ((x1 t1) ... (xn tn) "
  "(y1 d1) ... (ym dm))))</text---><text>Computes the equijoin two "
  "streams.</text--->)";
/*

4.1.3 Definition of operator ~sortmergejoin~

*/
Operator SortMergeJoinOperator(
         "sortmergejoin",        // name
         SortMergeJoinSpec,     // specification
         MergeJoin<false>,         // value mapping
         Operator::DummyModel,  // dummy model mapping, defines in Algebra.h
         simpleSelect,          // trivial selection function
         JoinTypeMap<false>   // type mapping
);


/*

7.3 Operator ~hashjoin~

This operator computes the equijoin two streams via a hash join.
The user can specify the number of hash buckets.

7.3.1 Auxiliary Class for Operator ~hashjoin~

*/

class HashJoinLocalInfo
{
private:
  static const size_t MAX_BUCKETS = 6151;
  static const size_t MIN_BUCKETS = 1;
  static const size_t DEFAULT_BUCKETS = 97;
  size_t nBuckets;
  size_t currentBucket;

  int attrIndexA;
  int attrIndexB;

  Word streamA;
  Word streamB;

  vector<vector< CcTuple*> > bucketsA;
  vector<vector< CcTuple*> > bucketsB;
  vector<CcTuple*> resultBucket;

  int CompareCcTuples(CcTuple* a, CcTuple* b)
  {
    return ((Attribute*)a->Get(attrIndexA))->
      Compare((Attribute*)b->Get(attrIndexB));
  }

  size_t HashTuple(CcTuple* tuple, int attrIndex)
  {
    return (((StandardAttribute*)tuple->Get(attrIndex))->HashValue() % nBuckets);
  }

  void FillHashBuckets(Word stream, int attrIndex,
    vector<vector< CcTuple*> >& buckets)
  {
    Word tupleWord;
    qp->Open(stream.addr);
    qp->Request(stream.addr, tupleWord);
    while(qp->Received(stream.addr))
    {
      CcTuple* tuple = (CcTuple*)tupleWord.addr;
      tuple = tuple->CloneIfNecessary();
      buckets[HashTuple(tuple, attrIndex)].push_back(tuple);
      qp->Request(stream.addr, tupleWord);
    }
  }
  
  void ClearBucket(vector<CcTuple*>& bucket)
  {
    vector<CcTuple*>::iterator iter = bucket.begin();
    while(iter != bucket.end())
    {
      (*iter)->DeleteIfAllowed();
      iter++;
    }
  }

  bool FillResultBucket()
  {
    while(resultBucket.empty() && currentBucket < nBuckets)
    {
      vector<CcTuple*>& a = bucketsA[currentBucket];
      vector<CcTuple*>& b = bucketsB[currentBucket];

      vector<CcTuple*>::iterator iterA = a.begin();
      vector<CcTuple*>::iterator iterB;
      for(; iterA != a.end(); iterA++)
      {
        for(iterB = b.begin(); iterB != b.end(); iterB++)
        {
          if(CompareCcTuples(*iterA, *iterB) == 0)
          {
            CcTuple* resultTuple = new CcTuple;
            resultTuple->SetFree(true);
            Word resultWord = SetWord(resultTuple);
            Word aWord = SetWord(*iterA);
            Word bWord = SetWord(*iterB);
            Concat(aWord, bWord, resultWord);
            resultBucket.push_back(resultTuple);
          };
        }
      }

      ClearBucket(a);
      ClearBucket(b);
      currentBucket++;
    }
    return !resultBucket.empty();
  };

public:
  HashJoinLocalInfo(Word streamA, Word attrIndexAWord,
    Word streamB, Word attrIndexBWord, Word nBucketsWord)
  {
    this->streamA = streamA;
    this->streamB = streamB;
    currentBucket = 0;

    attrIndexA = (int)((StandardAttribute*)attrIndexAWord.addr)->GetValue() - 1;
    attrIndexB = (int)((StandardAttribute*)attrIndexBWord.addr)->GetValue() - 1;
    nBuckets = (int)((StandardAttribute*)nBucketsWord.addr)->GetValue();
    if(nBuckets < MIN_BUCKETS || nBuckets > MAX_BUCKETS)
    {
      nBuckets = DEFAULT_BUCKETS;
    }
    bucketsA.resize(nBuckets);
    bucketsB.resize(nBuckets);

    FillHashBuckets(streamA, attrIndexA, bucketsA);
    FillHashBuckets(streamB, attrIndexB, bucketsB);
  }

  ~HashJoinLocalInfo()
  {
  }

  CcTuple* NextResultTuple()
  {
    if(resultBucket.empty())
    {
      if(!FillResultBucket())
      {
        return 0;
      }
    }
    CcTuple* result = resultBucket.back();
    resultBucket.pop_back();
    return result;
  }
};

/*

7.3.2 Value Mapping Function of Operator ~hashjoin~

*/

static int
HashJoin(Word* args, Word& result, int message, Word& local, Supplier s)
{
  HashJoinLocalInfo* localInfo;

  switch(message)
  {
    case OPEN:
      localInfo = new HashJoinLocalInfo(args[0], args[5],
        args[1], args[6], args[4]);
      local = SetWord(localInfo);
      return 0;
    case REQUEST:
      localInfo = (HashJoinLocalInfo*)local.addr;
      result = SetWord(localInfo->NextResultTuple());
      return result.addr != 0 ? YIELD : CANCEL;
    case CLOSE:
      localInfo = (HashJoinLocalInfo*)local.addr;
      delete localInfo;
      return 0;
  }
  return 0;
}

/*

4.1.3 Specification of Operator ~hashjoin~

*/
const string HashJoinSpec =
  "(<text>((stream (tuple ((x1 t1) ... (xn tn)))) (stream (tuple "
  "((y1 d1) ... (ym dm)))) xi yj nbuckets) -> (stream (tuple ((x1 t1) ... "
  "(xn tn) (y1 d1) ... (ym dm))))</text---><text>Computes the equijoin two "
  "streams via a hash join. The number of hash buckets is given by the "
  "parameter nBuckets.</text--->)";
/*

4.1.3 Definition of Operator ~hashjoin~

*/
Operator HashJoinOperator(
         "hashjoin",        // name
         HashJoinSpec,     // specification
         HashJoin,         // value mapping
         Operator::DummyModel,  // dummy model mapping, defines in Algebra.h
         simpleSelect,          // trivial selection function
         JoinTypeMap<true>   // type mapping
);

/*

7.3 Operator ~extend~

Extends each input tuple by new attributes as specified in the parameter list.

7.3.1 Type mapping function of operator ~extend~

Type mapping for ~extend~ is

----     ((stream x) ((b1 (map x y1)) ... (bm (map x ym))))

        -> (stream (tuple ((a1 x1) ... (an xn) (b1 y1 ... bm ym))))

        wobei x = (tuple ((a1 x1) ... (an xn)))
----

*/
static ListExpr
ExtendTypeMap( ListExpr args )
{
  ListExpr first, second, rest, listn, errorInfo,
           lastlistn, first2, second2, firstr, outlist;
  bool loopok;
  AlgebraManager* algMgr;

  algMgr = SecondoSystem::GetAlgebraManager();
  errorInfo = nl->OneElemList(nl->SymbolAtom("ERROR"));
  if(nl->ListLength(args) == 2)
  {
    first = nl->First(args);
    second  = nl->Second(args);
    if((nl->ListLength(first) == 2)  &&
	(TypeOfRelAlgSymbol(nl->First(first)) == stream) &&
	(!nl->IsAtom(second)) &&
	(nl->ListLength(second) > 0))
    {
      rest = nl->Second(nl->Second(first));
      listn = nl->OneElemList(nl->First(rest));
      lastlistn = listn;
      rest = nl->Rest(rest);
      while (!(nl->IsEmpty(rest)))
      {
   	lastlistn = nl->Append(lastlistn,nl->First(rest));
   	rest = nl->Rest(rest);
      }
      loopok = true;
      rest = second;
      while (!(nl->IsEmpty(rest)))
      {
	firstr = nl->First(rest);
	rest = nl->Rest(rest);
	first2 = nl->First(firstr);
	second2 = nl->Second(firstr);
	if ((nl->IsAtom(first2)) &&
	    (nl->ListLength(second2) == 3) &&
	    (nl->AtomType(first2) == SymbolType) &&
	    (TypeOfRelAlgSymbol(nl->First(second2)) == ccmap) &&
	    (algMgr->CheckKind("DATA", nl->Third(second2), errorInfo)) &&
            (nl->Equal(nl->Second(first),nl->Second(second2))))
	{
	  lastlistn = nl->Append(lastlistn,
	  	(nl->TwoElemList(first2,nl->Third(second2))));
	}
	else{
	  loopok = false;
	}
      }
      if ((loopok) && (comparenames(listn)))
      {
        outlist = nl->TwoElemList(nl->SymbolAtom("stream"),
			nl->TwoElemList(nl->SymbolAtom("tuple"),listn));
        return outlist;
      }
      else return nl->SymbolAtom("typeerror");
    }
    else return nl->SymbolAtom("typeerror");
  }
   return nl->SymbolAtom("typeerror");
}
/*

4.1.2 Value mapping function of operator ~extend~

*/
static int
Extend(Word* args, Word& result, int message, Word& local, Supplier s)
{
  Word t, value;
  CcTuple* tup;
  Supplier supplier, supplier2, supplier3;
  int noofoldattrs, nooffun, noofsons;
  ArgVectorPointer funargs;

  switch (message)
  {
    case OPEN :

      qp->Open(args[0].addr);
      return 0;

    case REQUEST :

      qp->Request(args[0].addr,t);
      if (qp->Received(args[0].addr))
      {
        tup = (CcTuple*)t.addr;
        tup = tup->CloneIfNecessary();
        noofoldattrs = tup->GetNoAttrs();
        supplier = args[1].addr;
        nooffun = qp->GetNoSons(supplier);
        for (int i=0; i < nooffun;i++)
        {
          supplier2 = qp->GetSupplier(supplier, i);
          noofsons = qp->GetNoSons(supplier2);
          supplier3 = qp->GetSupplier(supplier2, 1);
          funargs = qp->Argument(supplier3);
          (*funargs)[0] = SetWord(tup);
          qp->Request(supplier3,value);
          tup->Put(noofoldattrs+i,((StandardAttribute*)value.addr)->Clone());
        }
        
        tup->SetNoAttrs(noofoldattrs + nooffun);
        result = SetWord(tup);
        return YIELD;
      }
      else
        return CANCEL;

    case CLOSE :

      qp->Close(args[0].addr);
      return 0;
  }
  return 0;
}
/*

4.1.3 Specification of operator ~extend~

*/
const string ExtendSpec =
  "(<text>(stream(tuple(x)) x [(a1, (tuple(x) -> d1)) ... (an, (tuple(x) -> "
  "dn))] -> stream(tuple(x@[a1:d1, ... , an:dn])))</text---><text>Extends "
  "each input tuple by new attributes as specified in the parameter "
  "list.</text--->)";
/*

4.1.3 Definition of operator ~extend~

*/
Operator cppextend (
         "extend",              // name
         ExtendSpec,            // specification
         Extend,                // value mapping
         Operator::DummyModel,  // dummy model mapping, defines in Algebra.h
         simpleSelect,          // trivial selection function
         ExtendTypeMap          // type mapping
);
/*

7.3 Operator ~concat~

7.3.1 Type mapping function of operator ~concat~

Type mapping for ~concat~ is

----    ((stream (tuple (a1:d1 ... an:dn))) (stream (tuple (b1:d1 ... bn:dn))))

        -> (stream (tuple (a1:d1 ... an:dn)))
----

*/
ListExpr GetAttrTypeList (ListExpr l)
{
  ListExpr first, olist, lastolist, attrlist;

  olist = nl->TheEmptyList();
  attrlist = l;
  while (!nl->IsEmpty(attrlist))
  {
    first = nl->First(attrlist);
    attrlist = nl->Rest(attrlist);
    if (olist == nl->TheEmptyList())
    {
      olist = nl->Cons(nl->Second(first), nl->TheEmptyList());
      lastolist = olist;
    }
    else
    {
      lastolist = nl->Append(lastolist, nl->Second(first));
    }
  }
  return olist;
}

static ListExpr
ConcatTypeMap( ListExpr args )
{
  ListExpr first, second;
  if(nl->ListLength(args)  == 2)
  {
    first = nl->First(args);
    second = nl->Second(args);

    if((nl->ListLength(first) == 2) &&
       (TypeOfRelAlgSymbol(nl->First(first)) == stream) &&
       (nl->ListLength(nl->Second(first)) == 2) &&
       (TypeOfRelAlgSymbol(nl->First(nl->Second(first))) == tuple) &&
       (nl->ListLength(second) == 2) &&
       (TypeOfRelAlgSymbol(nl->First(second)) == stream) &&
       (nl->ListLength(nl->Second(second)) == 2) &&
       (TypeOfRelAlgSymbol(nl->First(nl->Second(second))) == tuple) &&
       (nl->Equal(GetAttrTypeList(nl->Second(nl->Second(first))),
	          GetAttrTypeList(nl->Second(nl->Second(second))))))
       return first;
    else
      return nl->SymbolAtom("typeerror");
  }
  return nl->SymbolAtom("typeerror");
}
/*

4.1.2 Value mapping function of operator ~concat~

*/
static int
Concat(Word* args, Word& result, int message, Word& local, Supplier s)
{
  Word t;
  CcTuple* tuple;

  switch (message)
  {
    case OPEN :

      qp->Open(args[0].addr);
      qp->Open(args[1].addr);
      local = SetWord(new CcInt(true, 0));
      return 0;

    case REQUEST :
      if ( (((CcInt*)local.addr)->GetIntval()) == 0)
      {
        qp->Request(args[0].addr, t);
        if (qp->Received(args[0].addr))
        {
          tuple = (CcTuple*)t.addr;
          tuple = tuple->CloneIfNecessary();
          result = SetWord(tuple);
          return YIELD;
        }
        else
        {
          ((CcInt*)local.addr)->Set(1);
        }
      }
      qp->Request(args[1].addr, t);
      if (qp->Received(args[1].addr))
      {
        tuple = (CcTuple*)t.addr;
        tuple = tuple->CloneIfNecessary();
        result = SetWord(tuple);
        return YIELD;
      }
      else
        return CANCEL;

    case CLOSE :

      qp->Close(args[0].addr);
      qp->Close(args[1].addr);
      delete (CcInt*)local.addr;
      return 0;
  }
  return 0;
}
/*

4.1.3 Specification of operator ~concat~

*/
const string ConcatSpec =
  "(<text>((stream (tuple (a1:d1 ... an:dn))) (stream (tuple (b1:d1 ... "
  "bn:dn)))) -> (stream (tuple (a1:d1 ... an:dn)))</text---><text>Union "
  "(without duplicate removal.</text--->)";
/*

4.1.3 Definition of operator ~concat~

*/
Operator cppconcat (
         "concat",              // name
         ConcatSpec,            // specification
         Concat,                // value mapping
         Operator::DummyModel,  // dummy model mapping, defines in Algebra.h
         simpleSelect,          // trivial selection function
         ConcatTypeMap          // type mapping
);

/*

7.20 Operator ~groupby~


7.20.1 Type mapping function of operator ~groupby~


Result type of ~groupby~ operation.

----    ((stream (tuple (xi1 ... xin))) ((namei1(fun x y1)) .. (namein (fun x ym)))

        -> (stream (tuple (xi1 .. xin y1 .. ym)))		APPEND (i1,...in)
----

*/
ListExpr GroupByTypeMap(ListExpr args)
{
  ListExpr first, second, third, rest, listn, lastlistn, first2,
    second2, firstr, attrtype, listp, lastlistp;
  ListExpr groupType;
  bool loopok;
  string  attrname;
  int j;
  bool firstcall = true;
  int numberatt;
  string listString;

  if(nl->ListLength(args) == 3)
  {
    first = nl->First(args);
    second  = nl->Second(args);
    third  = nl->Third(args);

    if(nl->ListLength(first) == 2  &&
      (TypeOfRelAlgSymbol(nl->First(first)) == stream) &&
      (TypeOfRelAlgSymbol(nl->First(nl->Second(first))) == tuple) &&
      (!nl->IsAtom(second)) &&
      (nl->ListLength(second) > 0))
    {
      numberatt = nl->ListLength(second);
      rest = second;
      while (!nl->IsEmpty(rest))
      {
        first2 = nl->First(rest);
        rest = nl->Rest(rest);
        attrname = nl->SymbolValue(first2);
        j =   findattr(nl->Second(nl->Second(first)), attrname, attrtype);
        if (j)
        {
          if (!firstcall)
          {
            lastlistn  = nl->Append(lastlistn,nl->TwoElemList(first2,attrtype));
            lastlistp = nl->Append(lastlistp,nl->IntAtom(j));
          }
          else
          {
            firstcall = false;
            listn = nl->OneElemList(nl->TwoElemList(first2,attrtype));
            lastlistn = listn;
            listp = nl->OneElemList(nl->IntAtom(j));
            lastlistp = listp;
          }
        }
        else
          return nl->SymbolAtom("typeerror");

      }
      loopok = true;
      rest = third;

      groupType =
        nl->TwoElemList(
          nl->SymbolAtom("rel"),
          nl->Second(first));

      while (!(nl->IsEmpty(rest)))
      {
        firstr = nl->First(rest);

        rest = nl->Rest(rest);
        first2 = nl->First(firstr);
        second2 = nl->Second(firstr);

        if((nl->IsAtom(first2)) &&
          (nl->ListLength(second2) == 3) &&
          (nl->AtomType(first2) == SymbolType) &&
          (TypeOfRelAlgSymbol(nl->First(second2)) == ccmap) &&
          (nl->Equal(groupType, nl->Second(second2))))
        {
          lastlistn = nl->Append(lastlistn,
          (nl->TwoElemList(first2,nl->Third(second2))));
        }
        else
          loopok = false;
        }
      }
    if ((loopok) && (comparenames(listn)))
    {
      return
        nl->ThreeElemList(
          nl->SymbolAtom("APPEND"),
          nl->Cons(nl->IntAtom(nl->ListLength(listp)), listp),
          nl->TwoElemList(
            nl->SymbolAtom("stream"),
            nl->TwoElemList(
              nl->SymbolAtom("tuple"),
              listn)));
    }
  }
  return nl->SymbolAtom("typeerror");
}

/*

7.20.2 Value mapping function of operator ~groupby~

*/

int GroupByValueMapping
(Word* args, Word& result, int message, Word& local, Supplier supplier)
{
  CcTuple *t;
  CcTuple *s;
  Word sWord;
  Word relWord;
  CcRel* tp;
  int i, j, k;
  int numberatt;
  bool ifequal;
  Word value;
  Supplier  value2;
  Supplier supplier1;
  Supplier supplier2;
  int ind;
  int noOffun;
  ArgVectorPointer vector;
  const int indexOfCountArgument = 3;
  const int startIndexOfExtraArguments = indexOfCountArgument +1;
  int attribIdx;

  switch(message)
  {
    case OPEN:
      qp->Open (args[0].addr);
      qp->Request(args[0].addr, sWord);
      if (qp->Received(args[0].addr))
      {
        local = SetWord((CcTuple*)sWord.addr);
      }
      else
      {
        local = SetWord(0);
      }
      return 0;

    case REQUEST:
      tp = new CcRel;
      if(local.addr == 0)
      {
        delete tp;
        return CANCEL;
      }
      else
      {
        t = (CcTuple*)local.addr;
        t = t->CloneIfNecessary();
        t->SetFree(false);
        tp->AppendTuple(t);
      }
      numberatt = ((CcInt*)args[indexOfCountArgument].addr)->GetIntval();

      ifequal = true;
      qp->Request(args[0].addr, sWord);
      while ((qp->Received(args[0].addr)) && ifequal)
      {
        s = (CcTuple*)sWord.addr;
        for (k = 0; k < numberatt; k++)
        {
          attribIdx = ((CcInt*)args[startIndexOfExtraArguments+k].addr)->GetIntval();
          j = attribIdx - 1;
          if (((Attribute*)t->Get(j))->Compare((Attribute *)s->Get(j)))
            ifequal = false;
        }
        if (ifequal)
        {
          s = s->CloneIfNecessary();
          s->SetFree(false);
          tp->AppendTuple(s);
          qp->Request(args[0].addr, sWord);
        }
        else
          local = SetWord((CcTuple*)sWord.addr);
      }
      if(ifequal)
      {
        local = SetWord(0);
      }

      t = new CcTuple;
      t->SetFree(true);
      tp->NewScan();
      s = tp->GetTuple();

      for(i = 0; i < numberatt; i++)
      {
        attribIdx = ((CcInt*)args[startIndexOfExtraArguments+i].addr)->GetIntval();
        t->Put(i, ((Attribute*)s->Get(attribIdx - 1))->Clone());
      }
      value2 = (Supplier)args[2].addr;
      noOffun  =  qp->GetNoSons(value2);
      t->SetNoAttrs(numberatt + noOffun);

      for(ind = 0; ind < noOffun; ind++)
      {
        tp->NewScan();
        supplier1 = qp->GetSupplier(value2, ind);
        supplier2 = qp->GetSupplier(supplier1, 1);
        vector = qp->Argument(supplier2);
        (*vector)[0] = SetWord(tp);
        qp->Request(supplier2, value);
        t->Put(numberatt + ind, ((Attribute*)value.addr)->Clone()) ;
      }
      result = SetWord(t);
      relWord = SetWord(tp);
      DeleteRel(relWord);
      return YIELD;

    case CLOSE:
      qp->Close(args[0].addr);
      return 0;
  }
  return 0;
}

/*

4.1.3 Specification of operator ~groupby~

*/
const string GroupBySpec =
  "(<text>((stream (tuple (a1:d1 ... an:dn))) (ai1 ... aik) ((bj1 (fun "
  "(rel (tuple (a1:d1 ... an:dn))) (_))) ... (bjl (fun (rel (tuple "
  "(a1:d1 ... an:dn))) (_))))) -> (stream (tuple (ai1:di1 ... aik:dik bj1 ... "
  "bjl)))</text---><text>Groups a relation according to attributes "
  "ai1, ..., aik and feeds the groups to other functions. The results of those "
  "functions are appended to the grouping attributes.</text--->";

/*

4.1.3 Definition of operator ~groupby~

*/
Operator cppgroupby (
         "groupby",             // name
         GroupBySpec,           // specification
         GroupByValueMapping,   // value mapping
         Operator::DummyModel,  // dummy model mapping, defines in Algebra.h
         simpleSelect,          // trivial selection function
         GroupByTypeMap         // type mapping
);

/*

6 Class ~RelationAlgebra~

A new subclass ~RelationAlgebra~ of class ~Algebra~ is declared. The only
specialization with respect to class ~Algebra~ takes place within the
constructor: all type constructors and operators are registered at the actual algebra.

After declaring the new class, its only instance ~RelationAlgebra~ is defined.

*/

class RelationAlgebra : public Algebra
{
 public:
  RelationAlgebra() : Algebra()
  {
    AddTypeConstructor( &cpptuple );
    AddTypeConstructor( &cpprel );

    AddOperator(&feed);
    AddOperator(&consume);
    AddOperator(&TUPLE);
    AddOperator(&GROUP);
    AddOperator(&TUPLE2);
    AddOperator(&attr);
    AddOperator(&tfilter);
    AddOperator(&project);
    AddOperator(&product);
    AddOperator(&cancel);
    AddOperator(&tcount);
    AddOperator(&cpprename);
    AddOperator(&cppextract);
    AddOperator(&cppextend);
    AddOperator(&cppconcat);
    AddOperator(&cppmax);
    AddOperator(&cppmin);
    AddOperator(&cppavg);
    AddOperator(&cppsum);
    AddOperator(&cpphead);
    AddOperator(&sortBy);
    AddOperator(&cppsort);
    AddOperator(&cpprdup);
    AddOperator(&cppmergesec);
    AddOperator(&cppmergediff);
    AddOperator(&cppmergeunion);
    AddOperator(&MergeJoinOperator);
    AddOperator(&SortMergeJoinOperator);
    AddOperator(&HashJoinOperator);
    AddOperator(&cppgroupby);

    cpptuple.AssociateKind( "TUPLE" );
    cpprel.AssociateKind( "REL" );

  }
  ~RelationAlgebra() {};
};

RelationAlgebra relationalgebra;
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
dynamically at runtime.

*/

extern "C"
Algebra*
InitializeRelationAlgebra( NestedList* nlRef, QueryProcessor* qpRef )
{
  nl = nlRef;
  qp = qpRef;
  return (&relationalgebra);
}
