/*
//paragraph [1] title: [{\Large \bf ]	[}]



[1] Array Algebra

Apr 2003 Oliver Lueck

The algebra provides a type constructor ~array~, which defines a generic
array. The elements of the array must have a list representation.

The algebra has three operators ~size~, ~get~ and ~put~. The operators
are used to get the size of the array, to retrieve an element with a given
index or to set a value to an element with a given index.

Note that the first element has the index 0. Precondition for the operators
~get~ and ~set~ is a valid index between 0 and the size of the array minus 
1.

Additionally the algebra provides two special operators ~distribute~ and
~summarize~ which help to work with arrays of relations.

1 Preliminaries

1.1 Includes

*/
using namespace std;

#include "Algebra.h"
#include "NestedList.h"
#include "QueryProcessor.h"
#include "StandardTypes.h"
#include "RelationAlgebra.h"

//NestedList* nl;
//QueryProcessor* qp;

/*
1.2 Dummy Functions

Not interesting, but needed in the definition of a type constructor.

*/
//static Word
//NoSpace(int size) { return (SetWord(Address(0))); }

//static void
//DoNothing(Word& w) { w.addr = 0; }
 
static void* DummyCast(void* addr) { return (0); }

/*
2 Type Constructor ~array~

2.1 Data Structure - Class ~Array~

*/
class Array
{
  public:
    Array(int, int, int, Word*);
    Array();
    ~Array();
    void initialize(int, int, int, Word*);
    int getSize();
    int getElemAlgId();
    int getElemTypeId(); 
    Word getElement(int);
    void setElement(int, Word);
  private:
    bool defined;
    int size;
    int elemAlgId;
    int elemTypeId;
    Word* array;
};

Array::Array(int algebraId, int typeId, int n, Word* elements) { 
// Constructor with complete initialization of the array

  defined = true;
  elemAlgId = algebraId;
  elemTypeId = typeId;
  size = n;
  array = new Word[size];

  for (int i=0; i<size; i++) {
    array[i] = elements[i];
  }
}

Array::Array() {

  defined = false;
  size = 0;
  elemAlgId = 0;
  elemTypeId = 0;
}

Array::~Array() {

  delete []array;
}

void Array::initialize(int algebraId, int typeId, int n, Word* elements) {
// Precondition: Array is undefined

  if (!defined) {
    defined = true;
    elemAlgId = algebraId;
    elemTypeId = typeId;
    size = n;
    array = new Word[size];

    for (int i=0; i<size; i++) {
     array[i] = elements[i];
    } 
  }
}

int Array::getSize() { return size; }

int Array::getElemAlgId() { return elemAlgId; }

int Array::getElemTypeId() { return elemTypeId; }

Word Array::getElement(int index) { 
// Precondition: Array is defined and index>=0 and index<size

  if (defined && index>=0 && index<size) {
    return array[index];
  } 
  else {
    return SetWord(Address(0));
  }
}

void Array::setElement(int index, Word element) {
// Precondition: Array is defined and index>=0 and index<size

  if (defined && index>=0 && index<size) {
    array[index] = element;
  }
}

/*
2.2 List Representation

The list representation of an array is

----	(a1 a2 ... an)
----
The representation of the elements of the array depends from their type.
So a1 ... an may be nested lists themselves.

2.3 ~In~ and ~Out~ Functions

These functions use the ~In~ and ~Out~ Functions for the elements of 
the array.

*/
static ListExpr
OutArray( ListExpr typeInfo, Word value )
{
//cout << "In OutArray Funktion!" << endl;

  AlgebraManager* am = SecondoSystem::GetAlgebraManager();

  Array* array = (Array*)(value.addr);
  int algebraId = array->getElemAlgId();
  int typeId = array->getElemTypeId();

  ListExpr typeOfElement = nl->Second(typeInfo);

  ListExpr list;
  ListExpr last;
  ListExpr element;

  for (int i=0; i<array->getSize(); i++) {
    element = (am->OutObj(algebraId, typeId))
                             (typeOfElement, array->getElement(i));
    if (i==0) {
      list = nl->OneElemList(element);
      last = list;
    }
    else {
      last = nl->Append(last, element);
    }
  }

  return list;
}

static Word 
InArray( const ListExpr typeInfo, const ListExpr instance,
         const int errorPos, ListExpr& errorInfo, bool& correct )
{
//cout << "In InArray Funktion!" << endl;

  AlgebraManager* am = SecondoSystem::GetAlgebraManager();

  Array* newarray;

  Word a[nl->ListLength(instance)];
  int algebraId;
  int typeId;

  if (nl->ListLength(instance) > 0) {
  // Array has to consist of at least one element.

    ListExpr typeOfElement = nl->Second(typeInfo);
    ListExpr listOfElements = instance;
    ListExpr element;

    ListExpr pair;
    if (nl->IsAtom(nl->First(typeOfElement))) {
      pair = typeOfElement;
    }
    else
    {
      pair = nl->First(typeOfElement);
    }
    algebraId = nl->IntValue(nl->First(pair));
    typeId = nl->IntValue(nl->Second(pair));

    int i = 0;

    do
    {
      element = nl->First(listOfElements);
      listOfElements = nl->Rest(listOfElements);

      a[i++] = ((am->InObj(algebraId, typeId))
                       (typeOfElement, element, errorPos, errorInfo, correct));
    }
    while (!nl->IsEmpty(listOfElements) && correct);

    if (correct) {
      newarray = new Array(algebraId, typeId, i, a);
      return SetWord(newarray);
    }
  }

  correct = false;
  return SetWord(Address(0));
}

/*
2.4 Object ~Creation~, ~Deletion~, ~Close~ and ~Clone~ Functions

The ~Deletion~ and the ~Clone~ functions use the ~Delete~ and ~Clone~ 
functions of the elements of the array.

*/
Word CreateArray(const ListExpr typeInfo) {
//cout << "In CreateArray Function!" << endl;

  return SetWord(new Array());
}

void DeleteArray(Word& w) {
//cout << "In DeleteArray Function!" << endl;

  AlgebraManager* am = SecondoSystem::GetAlgebraManager();
  Array* array = (Array*)w.addr;

  for (int i=0; i<array->getSize(); i++) {
    Word element = array->getElement(i);
      (am->DeleteObj(array->getElemAlgId(),array->getElemTypeId()))(element);
  }

  delete array;
  w.addr = 0;
}

Word CloneArray(const Word& w) {
//cout << "In CloneArray Funktion!" << endl;

  AlgebraManager* am = SecondoSystem::GetAlgebraManager();

  Array* array = (Array*)w.addr;
  Array* newarray;

  int n = array->getSize();
  int algebraId = array->getElemAlgId();
  int typeId = array->getElemTypeId();

  Word a[array->getSize()];

  for (int i=0; i < n; i++) {
    a[i] = (am->CloneObj(algebraId, typeId))(array->getElement(i));
  }

  newarray = new Array(algebraId, typeId, n, a);
  return SetWord(newarray);
}

void CloseArray( Word& w ) {
//cout << "In CloseArray Function!" << endl;

  w.addr = 0;
}
/*
2.5 Function Describing the Signature of the Type Constructor

The type of the elements of the array may be described by any valid Type 
Constructor, but the elements must have a list representation.

*/
static ListExpr
ArrayProperty()
{
  return (nl->TwoElemList(
            nl->FiveElemList(
              nl->StringAtom("Signature"),
              nl->StringAtom("Example Type List"),
              nl->StringAtom("List Rep"),
              nl->StringAtom("Example List"),
              nl->StringAtom("Remarks")),
            nl->FiveElemList(
              nl->StringAtom("typeconstructor -> ARRAY"),
              nl->StringAtom("(array int)"),
              nl->StringAtom("(a1 a2 ... an)"),
              nl->StringAtom("(0 1 2 3)"),
              nl->StringAtom("The elements of the array must have a list representation."))));
}

/*
2.6 Kind Checking Function

The Type Constructor of an array is a list (array type). The first element
of that list is the symbol "array" and the second element has to be a valid
Type Constructor for the elements of the array.

So the second element can be a symbol (e.g. int) or - in case of a more 
complex type - a Nested List itself.

*/
static bool
CheckArray( ListExpr type, ListExpr& errorInfo )
{
  if (nl->ListLength(type) == 2) {

    ListExpr First = nl->First(type);
    ListExpr Second = nl->Second(type);

    if (nl->IsEqual(First, "array")) {
      // Check whether Second is a valid Type Constructor

      SecondoCatalog* sc = SecondoSystem::GetCatalog(ExecutableLevel);

      if (sc->KindCorrect(Second, errorInfo)) {
        return true;
      } 
    }
  }

  return false;
}

/*
2.7 Creation of the Type Constructor Instance

Here an object of the type TypeConstructor is created. The constructor for
an instance of the class TypeConstructor is called with the properties and 
functions for the array as parameters.

*/
TypeConstructor array(
	"array",                      // name		
	ArrayProperty,                // property function describing signature
	OutArray, InArray,            // out and in functions
	CreateArray, DeleteArray,     // object creation and deletion
	0, 0,                         // default object open and save
	CloseArray, CloneArray,       // opject close and clone
	DummyCast,                    // cast function
	CheckArray,                   // kind checking function
	0,                            // predef. pers. function for model
	TypeConstructor::DummyInModel, 	
	TypeConstructor::DummyOutModel,
	TypeConstructor::DummyValueToModel,
	TypeConstructor::DummyValueListToModel );

/*
3 Creating Operators

3.1 Trivial Selection Function

This selection function can be used for the non-overloaded operators 
of the algebra. Currently all operators of this algebra can use the
trivial selection function.

*/
static int
simpleSelect (ListExpr args) { return 0; }

/*
3.1 Operator ~size~

The operator ~size~ returns the number of elements of an array.

The type mapping is

---- ((array x)) -> int
----

*/
static ListExpr
sizeTypeMap( ListExpr args )
{
  if (nl->ListLength(args) == 1)
  {
    ListExpr arg1 = nl->First(args);

    if (!nl->IsAtom(arg1) && nl->IsEqual(nl->First(arg1), "array")) {
      return nl->SymbolAtom("int");
    }
  }

  return nl->SymbolAtom("typeerror");
}

static int
sizeFun (Word* args, Word& result, int message, Word& local, Supplier s)
{
  Array* array = ((Array*)args[0].addr);

  result = qp->ResultStorage(s);

  ((CcInt*)result.addr)->Set(true, array->getSize());

  return 0;
}

const string sizeSpec = 
          "(( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
           " ( <text>((array x)) -> int</text--->"
            "  <text>size( _ )</text--->"
              "<text>Returns the size of an array.</text--->"
              "<text>query size(ai)</text---> ))";

Operator size (
	"size", 	       	   //name
	sizeSpec,                  //specification
	sizeFun,	      	   //value mapping
	Operator::DummyModel,      //dummy model mapping, defined in Algebra.h
	simpleSelect,              //trivial selection function 
	sizeTypeMap                //type mapping 
);

/*
3.2 Operator ~get~

The operator ~get~ returns the element with a given index. So the result type 
of the operator is the type of the array's elements.

The type mapping is

---- ((array x) int) -> x
----

*/
static ListExpr
getTypeMap( ListExpr args )
{
  if (nl->ListLength(args) == 2)
  {
    ListExpr arg1 = nl->First(args);
    ListExpr arg2 = nl->Second(args);

    if (!nl->IsAtom(arg1) && nl->IsEqual(nl->First(arg1), "array") 
          && nl->IsEqual(arg2, "int")) {
      // The second item of arg1 is the type of the array's elements.

      ListExpr resultType = nl->Second(arg1);
      return resultType;
    }
  }

  return nl->SymbolAtom("typeerror");
}

/*
Precondition of the value mapping function is a valid index. This means
an index between 0 and the size of the array minus 1. 

The list representation of the elements is used for cloning them because 
some types may have a dummy implementation of their ~Clone~ function.

*/
static int
getFun (Word* args, Word& result, int message, Word& local, Supplier s)
{
  Array* array = ((Array*)args[0].addr);
  CcInt* index = ((CcInt*)args[1].addr);

  int i = index->GetIntval();

  if (i<0 || i >= array->getSize()) {
  // error handling

    cout << "*** Error in Operator get: " << endl;
    cout << "Index " << i << " out of range [0;" 
         << array->getSize() - 1 << "], ";
    cout << "first element will be returned." << endl;
    i = 0;
  }

  if (i>=0 && i < array->getSize()) {
  // should always be true

    SecondoCatalog* sc = SecondoSystem::GetCatalog(ExecutableLevel);
    AlgebraManager* am = SecondoSystem::GetAlgebraManager();

    Word element = array->getElement(i);

    Word clonedElement;
    ListExpr elemLE;

    int errorPos;
    ListExpr errorInfo;
    bool correct;

    int algebraId = array->getElemAlgId();
    int typeId = array->getElemTypeId();

    ListExpr resultType = qp->GetType(s);
    resultType = sc->NumericType(resultType);

    elemLE = (am->OutObj(algebraId, typeId))(resultType, element);
    clonedElement = (am->InObj(algebraId, typeId))
                           (resultType, elemLE, errorPos, errorInfo, correct);

    result.addr = clonedElement.addr;

    return 0;
  }
  else {
    return 1;
  }
}

const string getSpec = 
          "(( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
            "( <text>((array x) int) -> x</text--->"
              "<text>_ get [ _ ]</text--->"
              "<text>Returns an element with a given index.</text--->"
              "<text>query ai get [3]</text---> ))";

Operator get (
	"get",
	getSpec,
	getFun,
	Operator::DummyModel,
	simpleSelect,
	getTypeMap
);
/*
3.3 Operator ~put~

The operator ~put~ assigns a value to an element of an array.

The type mapping is

---- ((array x) x int) -> (array x) 
----

*/
static ListExpr
putTypeMap( ListExpr args )
{
  if (nl->ListLength(args) == 3)
  {
    ListExpr arg1 = nl->First(args);
    ListExpr arg2 = nl->Second(args);
    ListExpr arg3 = nl->Third(args);

    if (!nl->IsAtom(arg1) && nl->IsEqual(nl->First(arg1), "array")
        && nl->Equal(nl->Second(arg1), arg2) && nl->IsEqual(arg3, "int")) {
      return arg1;
    }
  }

  return nl->SymbolAtom("typeerror");
}

/*
Precondition of the value mapping function is a valid index. This means
an index between 0 and the size of the array minus 1.

The list representation of the elements is used for cloning them because 
some types may have a dummy implementation of their ~Clone~ function.

*/
static int
putFun (Word* args, Word& result, int message, Word& local, Supplier s)
{
  Array* array = ((Array*)args[0].addr);
  Word newelement = args[1];
  int i = ((CcInt*)args[2].addr)->GetIntval();

  if (i<0 || i >= array->getSize()) {
  // error handling

    cout << "*** Error in Operator put: " << endl;
    cout << "Index " << i << " out of range [0;"
         << array->getSize() - 1 << "], ";
    cout << "first element will be replaced." << endl;
    i = 0;
  }

  if (i>=0 && i < array->getSize()) {
  // should always be true

    SecondoCatalog* sc = SecondoSystem::GetCatalog(ExecutableLevel);
    AlgebraManager* am = SecondoSystem::GetAlgebraManager();

    int n = array->getSize();
    int algebraId = array->getElemAlgId();
    int typeId = array->getElemTypeId();

    Word a[array->getSize()];
    Word element;    
    ListExpr elemLE;

    int errorPos;
    ListExpr errorInfo;
    bool correct;

    ListExpr resultType = qp->GetType(s);
    ListExpr typeOfElement = sc->NumericType(nl->Second(resultType));

    for (int l=0; l < n; l++) {
      element = (l!=i) ? array->getElement(l) : newelement;
      elemLE = (am->OutObj(algebraId, typeId))(typeOfElement, element);
      a[l] = (am->InObj(algebraId, typeId))
                    (typeOfElement, elemLE, errorPos, errorInfo, correct);
    }

    result = qp->ResultStorage(s);

    ((Array*)result.addr)->initialize(algebraId, typeId, n, a);

    return 0;
  }
  else { 
    return 1; 
  }
}

const string putSpec = 
          "(( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
            "( <text>((array x) x int) -> (array x)</text--->"
              "<text>_ _ put [ _ ]</text--->"
              "<text>Replaces an element at a given index.</text--->"
              "<text>query ai 9 put [3]</text---> ))";

Operator put (
	"put",
	putSpec,
	putFun,
	Operator::DummyModel,
	simpleSelect,
	putTypeMap
);

/*
3.4 Operator ~distribute~

The operator ~distribute~ builds an array of relations from an incoming stream 
of tuples. The index of the appropriate relation has to be given by an integer 
attribute of the tuple.

This integer attribute is removed from the tuples in the resulting relations. 
So the incoming tuples have to consist of at least two attributes.

The formal specification of type mapping is:

---- ((stream (tuple ((x1 t1) ... (xn tn)))) xi) 

     -> (array (rel (tuple ((x1 t1) ... (xi-1 ti-1) (xi+1 ti+1) ... (xn tn)))))

     at which n>=2, 1<=i<=n and ti (the type of xi) = int
----

The index of the attribute ai is appended to the result type, because this
information is needed by the value mapping function.

*/
static ListExpr
distributeTypeMap( ListExpr args )
{
  if (nl->ListLength(args) == 2)
  {
    ListExpr streamDesc = nl->First(args);
    ListExpr attrNameLE = nl->Second(args);

    if (nl->IsEqual(nl->First(streamDesc), "stream") 
        && (nl->ListLength(streamDesc) == 2)
        && (nl->AtomType(attrNameLE) == SymbolType))
    {
      ListExpr tupleDesc = nl->Second(streamDesc);
      string attrName = nl->SymbolValue(attrNameLE);

      if (nl->IsEqual(nl->First(tupleDesc), "tuple")
          && (nl->ListLength(tupleDesc) == 2))
      {
        ListExpr attrList = nl->Second(tupleDesc);

        if (IsTupleDescription(attrList, nl))
        {
          int attrIndex;
          ListExpr attrType;

          attrIndex = findattr(attrList, attrName, attrType, nl);

          if (nl->ListLength(attrList) > 1 && attrIndex > 0 
              && nl->IsEqual(attrType, "int"))
          {
            ListExpr attrList2 = nl->TheEmptyList();  
            ListExpr last;

            while (!nl->IsEmpty(attrList)) {
              ListExpr attr = nl->First(attrList);

              if (nl->SymbolValue(nl->First(attr)) != attrName) {
                if (nl->IsEmpty(attrList2)) {
                  attrList2 = nl->OneElemList(attr);
                  last = attrList2;
                }
                else {
                  last = nl->Append(last, attr);
                }
              }

              attrList = nl->Rest(attrList);
            }

            return nl->ThreeElemList(
                         nl->SymbolAtom("APPEND"),
                         nl->OneElemList(nl->IntAtom(attrIndex)),
                         nl->TwoElemList(
                           nl->SymbolAtom("array"), 
                           nl->TwoElemList(
                             nl->SymbolAtom("rel"),
                             nl->TwoElemList(nl->SymbolAtom("tuple"),
                                             attrList2))));
          }
        }
      }
    }
  }

  return nl->SymbolAtom("typeerror");
}

/*
The value mapping function implements the operator ~distribute~. An integer 
constant defines the maximum number of relations in the resulting array.

Tuples with an index smaller than 0 or an index greater than the maximum number 
of relations are distributed to the first respectively the last relation.

*/
static int
distributeFun (Word* args, Word& result, int message, Word& local, Supplier s) {

  const int MAX_PKG = 256;

  CcInt* indexAttrCcInt = (CcInt*)args[2].addr;
  int pkgAttr = (indexAttrCcInt->GetIntval()) - 1;

  CcRel* relPkg[MAX_PKG] = { 0 };

  int n = 0;

  relPkg[0] = new CcRel();
  relPkg[0]->Empty();

  CcInt* pkgNrCcInt;
  int pkgNr;

  Word actual;

  qp->Open(args[0].addr);
  qp->Request(args[0].addr, actual);

  while(qp->Received(args[0].addr))
  {
    CcTuple* tuple = (CcTuple*)actual.addr;
    tuple = tuple->CloneIfNecessary();
    tuple->SetFree(false);

    CcTuple* tuple2 = new CcTuple();
    tuple2->SetNoAttrs(tuple->GetNoAttrs() - 1);

    int j = 0;
    for (int i=0; i<tuple->GetNoAttrs(); i++) {
      if (i!=pkgAttr) {
       tuple2->Put(j++, tuple->Get(i));
      }
    }
    tuple2->SetFree(false);

    tuple->DeleteIfAllowed();

    pkgNrCcInt = (CcInt*)(tuple->Get(pkgAttr));
    pkgNr = pkgNrCcInt->GetIntval();

    if (pkgNr < 0) { pkgNr = 0; }
    if (pkgNr > MAX_PKG - 1) { pkgNr = MAX_PKG - 1; }

    while (n < pkgNr) {
      relPkg[++n] = new CcRel();
      relPkg[n]->Empty();
    }
    relPkg[pkgNr]->AppendTuple(tuple2);

    qp->Request(args[0].addr, actual);
  }

  qp->Close(args[0].addr);

  result = qp->ResultStorage(s);

  Word a[++n];

  for (int i=0; i<n; i++) {
    a[i] = SetWord(relPkg[i]);
  }

  int algebraId;
  int typeId;

  SecondoCatalog* sc = SecondoSystem::GetCatalog(ExecutableLevel);

  if (sc->GetTypeId("rel", algebraId, typeId)) {
    Array* newarray = new Array(algebraId, typeId, n, a);
    result = SetWord(newarray);

    return 0;
  }
  else {
    return 1;
  }
}

const string distributeSpec = 
          "(( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
            "( <text>((stream (tuple ((x1 t1) ... (xn tn)))) xi) -> (array (rel (tuple ((x1 t1) ... (xi-1 ti-1) (xi+1 ti+1) ... (xn tn)))))</text--->"
              "<text>_ distribute [ _ ]</text--->"
              "<text>Distributes a stream of tuples into an array or relations. The attribute xi determines the index of the relation, therefore ti must be int.</text--->"
              "<text>let prel = plz feed distribute [pkg]</text---> ))";

Operator distribute (
	"distribute",
	distributeSpec,
	distributeFun,
	Operator::DummyModel,
	simpleSelect,
	distributeTypeMap
);

/*
3.5 Operator ~summarize~

The operator ~summarize~ produces a stream of tuples from an array of 
relations. The operator reads the tuples of all relations beginning
with the first relation of the array.

The formal specification of type mapping is:

---- ((array (rel x))) -> (stream x)
----

Note that the operator ~summarize~ is not exactly inverse to the operator
~distribute~ because the index of the relation is not appended to the
attributes of the outgoing tuples.

If the array has been constructed by the operator ~distribute~ the order
of the resulting stream in most cases does not correspond to the order of
the input stream of the operator ~distribute~.

*/
static ListExpr
summarizeTypeMap( ListExpr args )
{
  if (nl->ListLength(args) == 1) 
  {
    ListExpr arrayDesc = nl->First(args);

    if (nl->ListLength(arrayDesc) == 2 
        && nl->IsEqual(nl->First(arrayDesc), "array")) 
    {
      ListExpr relDesc = nl->Second(arrayDesc);

      if (nl->ListLength(relDesc) == 2
          && nl->IsEqual(nl->First(relDesc), "rel")) 
      {
        return nl->TwoElemList(nl->SymbolAtom("stream"), nl->Second(relDesc));
      }
    }
  }

  return nl->SymbolAtom("typeerror");
}

static int
summarizeFun (Word* args, Word& result, int message, Word& local, Supplier s) {

  struct ArrayIterator{int current; CcRelIT* rit;}* ait;

  Array* array;
  CcRel* r;
  Word argArray;
  Word element;

  switch (message) {
    case OPEN :
      ait = new ArrayIterator;
      ait->current = -1;
      local.addr = ait;
      return 0;

    case REQUEST : 
      ait = (ArrayIterator*)local.addr;

      if (ait->current < 0 || ait->rit->EndOfScan()) {
        qp->Request(args[0].addr, argArray);
        array = (Array*)argArray.addr;
      
        while (ait->current < 0 
           || (ait->rit->EndOfScan() && ait->current < array->getSize()-1)) {
          element = array->getElement(++(ait->current));
          r = (CcRel*)element.addr;
          ait->rit = r->MakeNewScan();
        }
      }

      if (!(ait->rit->EndOfScan())) {
        result = SetWord(ait->rit->GetTuple());
        ait->rit->Next();
        return YIELD;
      }
      else {
        return CANCEL;
      }

    case CLOSE : 
      ait = (ArrayIterator*)local.addr;
      delete ait->rit;
      return 0;
  }
  return 0;
}

const string summarizeSpec = 
          "(( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
            "( <text>((array (rel x))) -> (stream x)</text--->"
              "<text>_ summarize</text--->"
              "<text>Produces a stream of the tuples from all relations in the array.</text--->"
              "<text>query prel summarize consume</text---> ))";

Operator summarize (
	"summarize",
	summarizeSpec,
	summarizeFun,
	Operator::DummyModel,
	simpleSelect,
	summarizeTypeMap
);

/*
4 Creating the Algebra

*/
class ArrayAlgebra : public Algebra
{
 public:
  ArrayAlgebra() : Algebra()
  {
    AddTypeConstructor( &array );

    array.AssociateKind("ARRAY");

    AddOperator( &size );
    AddOperator( &get );
    AddOperator( &put );

    AddOperator( &distribute );
    AddOperator( &summarize );
  }
  ~ArrayAlgebra() {};
};


ArrayAlgebra arrayAlgebra; 

/*
5 Initialization

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
InitializeArrayAlgebra( NestedList* nlRef, QueryProcessor* qpRef )
{
  nl = nlRef;
  qp = qpRef;
  return (&arrayAlgebra);
}
