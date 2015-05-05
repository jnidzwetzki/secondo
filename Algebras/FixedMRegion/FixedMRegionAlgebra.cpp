/*
This is my algebra. It is supportes by other classes in this 
folder.

*/
using namespace std;
#include "FixedMRegionAlgebra.h"
extern NestedList *nl;
extern QueryProcessor *qp;

class FixedMRegionPoint:public Attribute
{
public:
  //FixedMRegionPoint(){}
FixedMRegionPoint (double _x = 0, double _y = 0):
  Attribute (true), x (_x), y (_y)
  {
  } ~FixedMRegionPoint ()
  {
  }
  static const string BasicType ()
  {
    return "pstpoint";
  }
  static const bool checkType (const ListExpr list)
  {
    return listutils::isSymbol (list, BasicType ());
  };

  bool operator== (const FixedMRegionPoint & _b)
  {
    if ((x == _b.x) && (y == _b.y))
      {
        return true;
      }
    return false;
  }
  double getX ()
  {
    return x;
  }
  double getY ()
  {
    return y;
  }
  void set (double _x, double _y)
  {
    x = _x;
    y = _y;
  }
  int NumOfFLOBs () const
  {
    return 0;
  }
  Flob *GetFLOB (const int i)
  {
    assert (false);
    return 0;
  }
  int Compare (const Attribute * arg) const
  {
    FixedMRegionPoint *c = (FixedMRegionPoint *) arg;
    if (x < c->getX ())
        return -1;
    if (x > c->getX ())
        return 1;
    if (y < c->getY ())
        return -1;
    if (y > c->getY ())
        return 1;
      return 0;
  }
  bool Adjacent (const Attribute * arg) const
  {
    return false;
  }
  FixedMRegionPoint *Clone () const
  {
    FixedMRegionPoint *res = new FixedMRegionPoint (x, y);
      return res;
  }
  size_t Sizeof () const
  {
    return sizeof (*this);
  }
  void CopyFrom (const Attribute * arg)
  {
    FixedMRegionPoint *p = (FixedMRegionPoint *) arg;
    x = p->getX ();
    y = p->getY ();
  }
  size_t HashValue () const
  {
    return (size_t) (x + y);
  }
private:double x;
  double y;
};


ListExpr
FixedMRegionPointProperty ()
{
  return nl->TwoElemList (nl->FourElemList (nl->StringAtom ("Signature"),
                                            nl->StringAtom
                                            ("Example Type List"),
                                            nl->StringAtom ("List Rep"),
                                            nl->StringAtom
                                            ("Example List")),
                          nl->FourElemList (nl->StringAtom ("-> DATA"),
                                            nl->StringAtom
                                            (FixedMRegionPoint::BasicType
                                             ()),
                                            nl->StringAtom
                                            ("(real real) = (x,y)"),
                                            nl->StringAtom ("(13.5 -76.0)")));
}


Word
InFixedMRegionPoint (const ListExpr typeInfo,
                     const ListExpr instance,
                     const int errorPos, ListExpr & errorInfo, bool & correct)
{
  Word res ((void *) 0);
  correct = false;
  if (!nl->HasLength (instance, 2))
    {
      return res;
    }
  if (!listutils::isNumeric (nl->First (instance))
      || !listutils::isNumeric (nl->Second (instance)))
    {
      return res;
    }
  double x = listutils::getNumValue (nl->First (instance));
  double y = listutils::getNumValue (nl->Second (instance));

  correct = true;
  res.addr = new FixedMRegionPoint (x, y);
  return res;
}

ListExpr
OutFixedMRegionPoint (ListExpr typeInfo, Word value)
{
  FixedMRegionPoint *k = (FixedMRegionPoint *) value.addr;
  return nl->TwoElemList (nl->RealAtom (k->getX ()), nl->RealAtom (k->getY ()));
}

Word
CreateFixedMRegionPoint (const ListExpr typeInfo)
{
  Word w;
  w.addr = (new FixedMRegionPoint (0, 0));
  return w;
}

void
DeleteFixedMRegionPoint (const ListExpr typeInfo, Word & w)
{
  FixedMRegionPoint *k = (FixedMRegionPoint *) w.addr;
  delete k;
  w.addr = 0;
}

bool
SaveFixedMRegionPoint (SmiRecord & valueRecord, size_t & offset,
                       const ListExpr typeInfo, Word & value)
{
  FixedMRegionPoint *k = static_cast < FixedMRegionPoint * >(value.addr);
  size_t size = sizeof (double);
  double v = k->getX ();
  bool ok = valueRecord.Write (&v, size, offset);
  offset += size;
  v = k->getY ();
  ok = ok && valueRecord.Write (&v, size, offset);
  offset += size;
  return ok;
}

bool
OpenFixedMRegionPoint (SmiRecord & valueRecord,
                       size_t & offset, const ListExpr typeInfo, Word & value)
{
  size_t size = sizeof (double);
  double x, y;
  bool ok = valueRecord.Read (&x, size, offset);
  offset += size;
  ok = ok && valueRecord.Read (&y, size, offset);
  offset += size;
  if (ok)
    {
      value.addr = new FixedMRegionPoint (x, y);
    }
  else
    {
      value.addr = 0;
    }
  return ok;
}

void
CloseFixedMRegionPoint (const ListExpr typeInfo, Word & w)
{
  FixedMRegionPoint *k = (FixedMRegionPoint *) w.addr;
  delete k;
  w.addr = 0;
}

Word
CloneFixedMRegionPoint (const ListExpr typeInfo, const Word & w)
{
  FixedMRegionPoint *k = (FixedMRegionPoint *) w.addr;
  Word res;
  res.addr = new FixedMRegionPoint (k->getX (), k->getY ());
  return res;
}

void *
CastFixedMRegionPoint (void *addr)
{
  return (new (addr) FixedMRegionPoint);
}

int
SizeOfFixedMRegionPoint ()
{
  return 2 * sizeof (double);
}

bool
FixedMRegionPointTypeCheck (ListExpr type, ListExpr & errorInfo)
{
  return nl->IsEqual (type, FixedMRegionPoint::BasicType ());
}



TypeConstructor FixedMRegionPointTC (FixedMRegionPoint::BasicType (), 
                                     FixedMRegionPointProperty, 
                                     OutFixedMRegionPoint, 
                                     InFixedMRegionPoint, 0, 0,
                                     CreateFixedMRegionPoint,
                                     DeleteFixedMRegionPoint,
                                     OpenFixedMRegionPoint,
                                     SaveFixedMRegionPoint,
                                     CloseFixedMRegionPoint,
                                     CloneFixedMRegionPoint,
                                     CastFixedMRegionPoint,
                                     SizeOfFixedMRegionPoint,
                                     FixedMRegionPointTypeCheck);




ListExpr
testoperatoraTM (ListExpr args)
{
  string err = "one int is expected";
  if (!nl->HasLength (args, 1))
    {
      return listutils::typeError (err + " (wrong number of arguments)");
    }
  /* if(!listutils::isNumeric(nl->First(args))){
     return listutils::typeError(err);
     } */
  return listutils::basicSymbol < CcInt > ();
}

int
testoperatoraVM (Word * args, Word & result, int message,
                 Word & local, Supplier s)
{

  runTestMethod ();
  runFixedMTestMethod ();

  result = qp->ResultStorage (s);
  CcInt *res __attribute__ ((unused)) = (CcInt *) result.addr;

  res->Set (true, 2);
  //res  ->Set(true, false);
  return 1;
}

OperatorSpec testoperatoraSpec ("int -> int",
            "testoperatora(_)",
            "Computes nothing of an int and returns an int.",
            "query testoperatora(0)");

Operator testoperatoraOp ("testoperatora",
                          testoperatoraSpec.getStr (),
                          testoperatoraVM,
                          Operator::SimpleSelect, testoperatoraTM);



class FixedMRegionAlgebra:public Algebra
{
public:
  FixedMRegionAlgebra ():Algebra ()
  {
    AddTypeConstructor (&FixedMRegionPointTC);
    FixedMRegionPointTC.AssociateKind (Kind::DATA ());

    AddOperator (&testoperatoraOp);
  }
   ~FixedMRegionAlgebra ()
  {
  };
};

extern "C" Algebra *
InitializeFixedMRegionAlgebra (NestedList * nlRef, QueryProcessor * qpRef)
{
  return new FixedMRegionAlgebra;
}



;
