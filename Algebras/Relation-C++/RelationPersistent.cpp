/*
//paragraph [1] Title: [{\Large \bf \begin{center}] [\end{center}}]
//paragraph [10] Footnote: [{\footnote{] [}}]
//[TOC] [\tableofcontents]

[1] Implementation of Module Relation Algebra

[1] Separate part of persistent data representation

June 1996 Claudia Freundorfer

May 2002 Frank Hoffmann port to C++

November 7, 2002 RHG Corrected the type mapping of ~tcount~.

November 30, 2002 RHG Introduced a function ~RelPersistValue~ instead of
~DefaultPersistValue~ which keeps relations that have been built in memory in a
small cache, so that they need not be rebuilt from then on.

March 2003 Victor Almeida created the new Relational Algebra organization

[TOC]

1 Overview

The Relational Algebra basically implements two type constructors, namely ~tuple~ and ~rel~.
More information about the Relational Algebra can be found in the RelationAlgebra.h header
file.

This file contains the implementation of the Persistent Relational Algebra, where the
type constructors ~tuple~ and ~rel~ are kept in secondary memory. This implementation uses
the Tuple Manager.

2 Includes, Constants, Globals, Enumerations

*/
#ifdef RELALG_PERSISTENT
/*
This ~RELALG_PERSISTENT~ defines which kind of relational algebra is to be compiled.
If it is set, the persistent version of the relational algebra will be compiled, and
otherwise, the main memory version will be compiled.

*/

using namespace std;

#include "RelationAlgebra.h"
#include "SecondoSystem.h"
#include "FLOB.h"

extern NestedList *nl;

/*
3 Type constructor ~tuple~

3.1 Class ~TupleId~

This class implements the unique identification for tuples inside a relation. Once a relation
is persistent in an ~SmiFile~ and each tuple is stored in a different ~SmiRecord~ of this file,
the ~SmiRecordId~ will be this identification.

*/
struct TupleId
{
  TupleId():
    value( 0 )
    {}
  TupleId( const SmiRecordId id ):
    value( id )
    {}
  TupleId( const TupleId& id ):
    value( id.value )
    {}
/*
The constructors.

*/
  const TupleId& operator= ( const TupleId& id )
    { value = id.value; return *this; }
  const TupleId& operator+= ( const TupleId& id )
    { value += id.value; return *this; }
  const TupleId& operator-= ( const TupleId& id )
    { value -= id.value; return *this; }
  const TupleId& operator++ ()
    { value++; return *this; }
  TupleId operator++ (int)
    { TupleId result = *this; result += 1; return result; }
  const TupleId& operator-- ()
    { value--; return *this; }
  TupleId operator-- (int)
    { TupleId result = *this; result -= 1; return result; }
  int operator==( const TupleId& id ) const
  { return value == id.value; }
  int operator!=( const TupleId& id ) const
  { return value != id.value; }
  int operator<=( const TupleId& id ) const
  { return value <= id.value; }
  int operator>=( const TupleId& id ) const
  { return value >= id.value; }
  int operator<( const TupleId& id ) const
  { return value < id.value; }
  int operator>( const TupleId& id ) const
  { return value > id.value; }
/*
Operator redefinitions.

*/
  SmiRecordId value;
/*
The ~id~ value.

*/
};

/*
3.2 Struct ~PrivateTuple~

This struct contains the private attributes of the class ~Tuple~.

*/
enum TupleState {Fresh, Solid};

struct PrivateTuple
{
  PrivateTuple( const TupleType& tupleType, const bool isFree ):
    tupleId( 0 ),
    tupleType( tupleType ),
    attributes( new (TupleElement*)[ tupleType.GetNoAttributes() ] ),
    tupleRecord( 0 ),
    lobFile( 0 ),
    tupleFile( 0 ),
    state( Fresh ),
    isFree( isFree ),
    memoryTuple( 0 ),
    extensionTuple( 0 )
    {
      for( int i = 0; i < tupleType.GetNoAttributes(); i++ )
        attributes[i] = 0;
    }
/*
The first constructor. It creates a fresh tuple from a ~tupleType~.

*/
  PrivateTuple( const ListExpr typeInfo, const bool isFree ):
    tupleId( 0 ),
    tupleType( typeInfo ),
    attributes( new (TupleElement*)[ tupleType.GetNoAttributes() ] ),
    tupleRecord( 0 ),
    lobFile( 0 ),
    tupleFile( 0 ),
    state( Fresh ),
    isFree( isFree ),
    memoryTuple( 0 ),
    extensionTuple( 0 )
    {
      for( int i = 0; i < tupleType.GetNoAttributes(); i++ )
        attributes[i] = 0;
    }
/*
The second constructor. It creates a fresh tuple from a ~typeInfo~.

*/
  ~PrivateTuple()
  {
    if( memoryTuple == 0 )
    {
      assert( extensionTuple == 0 );
      // This was a fresh tuple saved. In this way, the attributes were
      // created outside the tuple and inserted in the tuple using the
      // ~PutAttribute~ method. They must be deleted.
      for( int i = 0; i < tupleType.GetNoAttributes(); i++ )
        delete attributes[i];
    }
    else
    {
      free( memoryTuple );
      if( extensionTuple != 0 )
        free( extensionTuple );
    }
    delete []attributes;
    delete tupleRecord;
  }
/*
The destructor.

*/
  const bool Save( SmiRecordFile *tuplefile, SmiRecordFile *lobfile );
/*
Saves a fresh tuple into ~tuplefile~ and ~lobfile~.

*/
  const bool Open( SmiRecordFile *tuplefile, SmiRecordFile *lobfile,
                   SmiRecordId rid );
/*
Opens a solid tuple from ~tuplefile~(~rid~) and ~lobfile~.

*/
  const bool Open( SmiRecordFile *tuplefile, SmiRecordFile *lobfile,
                   PrefetchingIterator *iter );
/*
Opens a solid tuple from ~tuplefile~ and ~lobfile~ reading the current record of ~iter~.

*/
  const bool Open( SmiRecordFile *tuplefile, SmiRecordFile *lobfile,
                   SmiRecord *record );
/*
Opens a solid tuple from ~tuplefile~ and ~lobfile~ reading from ~record~.

*/

  TupleId tupleId;
/*
The unique identification of the tuple inside a relation.

*/
  TupleType tupleType;
/*
Stores the tuple type.

*/
  TupleElement **attributes;
/*
The attributes pointer array. The tuple information is kept in memory.

*/
  SmiRecord *tupleRecord;
/*
The record that persistently holds the tuple value.

*/
  SmiRecordFile* lobFile;
/*
Reference to an ~SmiRecordFile~ which contains LOBs.

*/
  SmiRecordFile* tupleFile;
/*
Reference to an ~SmiRecordFile~ which contains the tuple.

*/
  TupleState state;
/*
State of the tuple (Fresh, Solid).

*/
  bool isFree;
/*
A flag that tells if a tuple is free for deletion. If a tuple is free, then a stream receiving
the tuple can delete or reuse it

*/
  char *memoryTuple;
/*
Stores the attributes array in memory.

*/
  char *extensionTuple;
/*
Stores the extension (small FLOBs) in memory.

*/
};

const bool PrivateTuple::Save( SmiRecordFile *tuplefile, SmiRecordFile *lobfile )
{
  assert( state == Fresh &&
          lobFile == 0 && tupleFile == 0 && tupleRecord == 0 &&
          memoryTuple == 0 && extensionTuple == 0 );

  lobFile = lobfile;
  tupleFile = tuplefile;
  tupleRecord = new SmiRecord();

  // Calculate the size of FLOB data
  int extensionSize = 0;
  for( int i = 0; i < tupleType.GetNoAttributes(); i++)
  {
    for( int j = 0; j < attributes[i]->NumOfFLOBs(); j++)
    {
      FLOB *tmpFLOB = attributes[i]->GetFLOB(j);
      attributes[i]->SaveFLOB(j, lobFile);
      if( !tmpFLOB->IsLob() )
        extensionSize += tmpFLOB->GetSize();
    }
  }

  // Move FLOB data to extension tuple
  if( extensionSize > 0 )
  {
    extensionTuple = (char *)malloc(extensionSize);
    char *extensionPtr = extensionTuple;
    for( int i = 0; i < tupleType.GetNoAttributes(); i++)
    {
      for( int j = 0; j < attributes[i]->NumOfFLOBs(); j++)
      {
        FLOB *tmpFLOB = attributes[i]->GetFLOB(j);
        if( !tmpFLOB->IsLob() )
        {
          tmpFLOB->Get(0, tmpFLOB->GetSize(), extensionPtr);
          extensionPtr += tmpFLOB->GetSize();
        }
      }
    }
  }

  // Move external attributes to memory tuple
  memoryTuple = (char *)malloc( tupleType.GetTotalSize() );
  int offset = 0;
  for( int i = 0; i < tupleType.GetNoAttributes(); i++)
  {
    memcpy( &memoryTuple[offset], attributes[i]->GetRootRecord(), tupleType.GetAttributeType(i).size );
    offset += tupleType.GetAttributeType(i).size;
  }

  bool rc = tupleFile->AppendRecord( tupleId.value, *tupleRecord );
  rc = tupleRecord->Write( &extensionSize, sizeof(int), 0) && rc;
  rc = tupleRecord->Write( memoryTuple, tupleType.GetTotalSize(), sizeof(int)) && rc;
  if( extensionSize > 0 )
    rc = tupleRecord->Write( extensionTuple, extensionSize, sizeof(int) + tupleType.GetTotalSize() ) && rc;

  state = Solid;

  if( extensionSize > 0 )
  {
    free( extensionTuple ); extensionTuple = 0;
  }
  free( memoryTuple ); memoryTuple = 0;

  return rc;
}

const bool PrivateTuple::Open( SmiRecordFile *tuplefile, SmiRecordFile *lobfile, SmiRecordId rid )
{
  assert( state == Fresh &&
          lobFile == 0 && tupleFile == 0 && tupleRecord == 0 &&
          memoryTuple == 0 && extensionTuple == 0 );

  AlgebraManager* algM = SecondoSystem::GetAlgebraManager();

  tupleId = rid;
  tupleRecord = new SmiRecord();
  tupleFile = tuplefile;
  lobFile = lobfile;

  // read tuple header and memory tuple from disk
  bool ok = tupleFile->SelectRecord( tupleId.value, *tupleRecord );
  if( !ok )
  {
    tupleId = 0;
    delete tupleRecord;
    tupleFile = 0;
    lobFile = 0;
    return false;
  }

  int extensionSize = 0;
  ok = tupleRecord->Read( &extensionSize, sizeof(int), 0 );

  memoryTuple = (char *)malloc( tupleType.GetTotalSize() );
  ok = tupleRecord->Read( memoryTuple, tupleType.GetTotalSize(), sizeof(int) ) && ok;

  if( !ok )
  {
    tupleId = 0;
    delete tupleRecord;
    tupleFile = 0;
    lobFile = 0;
    free( memoryTuple ); memoryTuple = 0;
    return false;
  }


  // Read attribute values from memoryTuple.
  char *valuePtr = memoryTuple;
  for( int i = 0; i < tupleType.GetNoAttributes(); i++ )
  {
    int algId = tupleType.GetAttributeType(i).algId;
    int typeId = tupleType.GetAttributeType(i).typeId;
    attributes[i] = (TupleElement *) (*(algM->Cast(algId, typeId)))(valuePtr, lobFile);
    valuePtr += tupleType.GetAttributeType(i).size;
  }

  // move FLOB data to extension tuple if exists.
  if( extensionSize > 0 )
  {
    extensionTuple = (char *)malloc( extensionSize );
    ok = tupleRecord->Read( extensionTuple, extensionSize, sizeof(int) + tupleType.GetTotalSize() ) && ok;
    if( !ok )
    {
      tupleId = 0;
      delete tupleRecord;
      tupleFile = 0;
      lobFile = 0;
      free( memoryTuple ); memoryTuple = 0;
      free( extensionTuple ); extensionTuple = 0;
      return false;
    }

    char *extensionPtr = extensionTuple;
    for( int i = 0; i < tupleType.GetNoAttributes(); i++ )
    {
      for( int j = 0; j < attributes[i]->NumOfFLOBs(); j++ )
      {
        FLOB *tmpFLOB = attributes[i]->GetFLOB( j );
        if( !tmpFLOB->IsLob() )
          extensionPtr = extensionPtr + tmpFLOB->Restore(extensionPtr);
      }
    }
  }

  state = Solid;
  return true;
}

const bool PrivateTuple::Open( SmiRecordFile *tuplefile, SmiRecordFile *lobfile, PrefetchingIterator *iter )
{
  assert( state == Fresh && lobFile == 0 && tupleFile == 0 && tupleRecord == 0 );
  AlgebraManager* algM = SecondoSystem::GetAlgebraManager();

  iter->ReadCurrentRecordNumber( tupleId.value );
  tupleFile = tuplefile;
  lobFile = lobfile;


  int extensionSize = 0;
  bool ok = iter->ReadCurrentData( &extensionSize, sizeof(int), 0 );

  memoryTuple = (char *)malloc( tupleType.GetTotalSize() );
  ok = iter->ReadCurrentData( memoryTuple, tupleType.GetTotalSize(), sizeof(int) ) && ok;

  if( !ok )
  {
    tupleId = 0;
    tupleFile = 0;
    lobFile = 0;
    free( memoryTuple ); memoryTuple = 0;
    return false;
  }


  // Read attribute values from memoryTuple.
  char *valuePtr = memoryTuple;
  for( int i = 0; i < tupleType.GetNoAttributes(); i++ )
  {
    int algId = tupleType.GetAttributeType(i).algId;
    int typeId = tupleType.GetAttributeType(i).typeId;
    attributes[i] = (TupleElement *) (*(algM->Cast(algId, typeId)))(valuePtr, lobFile);
    attributes[i]->SetInsideTuple();
    valuePtr += tupleType.GetAttributeType(i).size;
  }

  // move FLOB data to extension tuple if exists.
  if( extensionSize > 0 )
  {
    extensionTuple = (char *)malloc( extensionSize );
    ok = iter->ReadCurrentData( extensionTuple, extensionSize, sizeof(int) + tupleType.GetTotalSize() ) && ok;
    if( !ok )
    {
      tupleId = 0;
      tupleFile = 0;
      lobFile = 0;
      free( memoryTuple ); memoryTuple = 0;
      free( extensionTuple ); extensionTuple = 0;
      return false;
    }

    char *extensionPtr = extensionTuple;
    for( int i = 0; i < tupleType.GetNoAttributes(); i++ )
    {
      for( int j = 0; j < attributes[i]->NumOfFLOBs(); j++ )
      {
        FLOB *tmpFLOB = attributes[i]->GetFLOB( j );
        if( !tmpFLOB->IsLob() )
          extensionPtr = extensionPtr + tmpFLOB->Restore(extensionPtr);
      }
    }
  }

  state = Solid;
  return true;
}

const bool PrivateTuple::Open( SmiRecordFile *tuplefile, SmiRecordFile *lobfile, SmiRecord *record )
{
  assert( state == Fresh &&
          lobFile == 0 && tupleFile == 0 && tupleRecord == 0 &&
          memoryTuple == 0 && extensionTuple == 0 );

  AlgebraManager* algM = SecondoSystem::GetAlgebraManager();

  SmiKey key;
  key = record->GetKey();
  key.GetKey( tupleId.value );
  tupleRecord = record;
  tupleFile = tuplefile;
  lobFile = lobfile;

  // read tuple header and memory tuple from disk
  bool ok = tupleFile->SelectRecord( tupleId.value, *tupleRecord );
  if( !ok )
  {
    tupleId = 0;
    delete tupleRecord;
    tupleFile = 0;
    lobFile = 0;
    return false;
  }

  int extensionSize = 0;
  ok = tupleRecord->Read( &extensionSize, sizeof(int), 0 );

  memoryTuple = (char *)malloc( tupleType.GetTotalSize() );
  ok = tupleRecord->Read( memoryTuple, tupleType.GetTotalSize(), sizeof(int) ) && ok;

  if( !ok )
  {
    tupleId = 0;
    delete tupleRecord;
    tupleFile = 0;
    lobFile = 0;
    free( memoryTuple ); memoryTuple = 0;
    return false;
  }


  // Read attribute values from memoryTuple.
  char *valuePtr = memoryTuple;
  for( int i = 0; i < tupleType.GetNoAttributes(); i++ )
  {
    int algId = tupleType.GetAttributeType(i).algId;
    int typeId = tupleType.GetAttributeType(i).typeId;
    attributes[i] = (TupleElement *) (*(algM->Cast(algId, typeId)))(valuePtr, lobFile);
    attributes[i]->SetInsideTuple();
    valuePtr += tupleType.GetAttributeType(i).size;
  }

  // move FLOB data to extension tuple if exists.
  if( extensionSize > 0 )
  {
    extensionTuple = (char *)malloc( extensionSize );
    ok = tupleRecord->Read( extensionTuple, extensionSize, sizeof(int) + tupleType.GetTotalSize() ) && ok;
    if( !ok )
    {
      tupleId = 0;
      delete tupleRecord;
      tupleFile = 0;
      lobFile = 0;
      free( memoryTuple ); memoryTuple = 0;
      free( extensionTuple ); extensionTuple = 0;
      return false;
    }

    char *extensionPtr = extensionTuple;
    for( int i = 0; i < tupleType.GetNoAttributes(); i++ )
    {
      for( int j = 0; j < attributes[i]->NumOfFLOBs(); j++ )
      {
        FLOB *tmpFLOB = attributes[i]->GetFLOB( j );
        if( !tmpFLOB->IsLob() )
          extensionPtr = extensionPtr + tmpFLOB->Restore(extensionPtr);
      }
    }
  }

  state = Solid;
  return true;
}

/*
3.3 Implementation of the class ~Tuple~

This class implements the persistent representation of the type constructor ~tuple~.
A tuple contains a pointer to a ~TMTuple~ from the Tuple Manager. For more information
about tuples in the TupleManager see the file TMTuple.h.

*/
Tuple::Tuple( const TupleType& tupleType, const bool isFree ):
  privateTuple( new PrivateTuple( tupleType, isFree ) )
  {
    tuplesCreated++;
    tuplesInMemory++;
    if( tuplesInMemory > maximumTuples )
      maximumTuples = tuplesInMemory;
  }

Tuple::Tuple( const ListExpr typeInfo, const bool isFree ):
  privateTuple( new PrivateTuple( typeInfo, isFree ) )
  {
    tuplesCreated++;
    tuplesInMemory++;
    if( tuplesInMemory > maximumTuples )
      maximumTuples = tuplesInMemory;
  }

Tuple::~Tuple()
{
  tuplesDeleted++;
  tuplesInMemory--;
  delete privateTuple;
}

const TupleId& Tuple::GetTupleId() const
{
  return privateTuple->tupleId;
}

void Tuple::SetTupleId( const TupleId& tupleId )
{
  privateTuple->tupleId = tupleId;
}

Attribute* Tuple::GetAttribute( const int index ) const
{
  assert( index >= 0 && index < GetNoAttributes() );
  assert( privateTuple->attributes[index] != 0 );

  return (Attribute *)privateTuple->attributes[index];
}

void Tuple::PutAttribute( const int index, Attribute* attr )
{
  assert( index >= 0 && index < GetNoAttributes() );

  if( privateTuple->attributes[index] != 0 )
    delete privateTuple->attributes[index];
  attr->SetInsideTuple();
  privateTuple->attributes[index] = attr;
}

const int Tuple::GetNoAttributes() const
{
  return privateTuple->tupleType.GetNoAttributes();
}

const TupleType& Tuple::GetTupleType() const
{
  return privateTuple->tupleType;
}

const bool Tuple::IsFree() const
{
  return privateTuple->isFree;
}

Tuple *Tuple::Clone( const bool isFree ) const
{
  Tuple *result = new Tuple( this->GetTupleType(), isFree );
  for( int i = 0; i < this->GetNoAttributes(); i++ )
  {
    Attribute *attr = GetAttribute( i )->Clone();
    result->PutAttribute( i, attr );
  }
  return result;
}

Tuple *Tuple::CloneIfNecessary()
{
  if( privateTuple->state == Fresh )
    return this;
  else
    return this->Clone( false );
}

void Tuple::DeleteIfAllowed()
{
  delete this;
}

void Tuple::Delete()
{
  delete this;
}

ostream& operator <<( ostream& o, Tuple& t )
{
  o << "<";
  for( int i = 0; i < t.GetNoAttributes(); i++)
  {
    o << *t.GetAttribute(i);
    if (i < t.GetNoAttributes() - 1)
      o << ", ";
  }
  return o << ">";
}

ostream &operator<< (ostream &os, TupleElement &attrib)
{
  return attrib.Print(os);
}

/*
4 Type constructor ~rel~

4.2 Struct ~RelationDescriptor~

This struct contains necessary information for opening a relation.

*/
struct RelationDescriptor
{
  RelationDescriptor( const int noTuples, const SmiFileId tId, const SmiFileId lId ):
    noTuples( noTuples ),
    tupleFileId( tId ),
    lobFileId( lId )
    {}
/*
The first constructor.

*/
  RelationDescriptor( const RelationDescriptor& desc ):
    noTuples( desc.noTuples ),
    tupleFileId( desc.tupleFileId ),
    lobFileId( desc.lobFileId )
    {}
/*
The copy constructor.

*/

  int noTuples;
/*
The quantity of tuples inside the relation.

*/
  SmiFileId tupleFileId;
/*
The tuple's file identification.

*/
  SmiFileId lobFileId;
/*
The LOB's file identification.

*/
};

/*
4.1 Struct ~PrivateRelation~

This struct contains the private attributes of the class ~Relation~.

*/
struct PrivateRelation
{
  PrivateRelation( const ListExpr typeInfo, const bool isTemporary ):
    noTuples( 0 ),
    tupleType( nl->Second( typeInfo ) ),
    tupleFile( false, 0, isTemporary ),
    lobFile( false, 0, isTemporary )
    {
      assert( tupleFile.Create() );
      assert( lobFile.Create() );
    }
/*
The first constructor. Creates an empty relation from a ~typeInfo~.

*/
  PrivateRelation( const TupleType& tupleType, const bool isTemporary ):
    noTuples( 0 ),
    tupleType( tupleType ),
    tupleFile( false, 0, isTemporary ),
    lobFile( false, 0, isTemporary )
    {
      assert( tupleFile.Create() );
      assert( lobFile.Create() );
    }
/*
The second constructor. Creates an empty relation from a ~tupleType~.

*/
  PrivateRelation( const TupleType& tupleType, const RelationDescriptor& relDesc, const bool isTemporary ):
    noTuples( relDesc.noTuples ),
    tupleType( tupleType ),
    tupleFile( false, 0, isTemporary ),
    lobFile( false, 0, isTemporary )
    {
      assert( tupleFile.Open( relDesc.tupleFileId ) );
      assert( lobFile.Open( relDesc.lobFileId ) );
    }
/*
The third constructor. Opens a previously created relation.

*/
  PrivateRelation( const ListExpr typeInfo, const RelationDescriptor& relDesc, const bool isTemporary ):
    noTuples( relDesc.noTuples ),
    tupleType( nl->Second( nl->First( typeInfo ) ) ),
    tupleFile( false, 0 ),
    lobFile( false, 0 )
    {
      assert( tupleFile.Open( relDesc.tupleFileId ) );
      assert( lobFile.Open( relDesc.lobFileId ) );
    }
/*
The fourth constructor. It opens a previously created relation using the ~typeInfo~ instead of
the ~tupleType~.

*/
  ~PrivateRelation()
  {
    assert( tupleFile.Close() );
    assert( lobFile.Close() );
  }
/*
The destuctor.

*/
  int noTuples;
/*
Contains the number of tuples in the relation.

*/
  TupleType tupleType;
/*
Stores the tuple type for every tuple of this relation.

*/
  SmiRecordFile tupleFile;
/*
The file to store tuples.

*/
  SmiRecordFile lobFile;
/*
The file to store FLOBs

*/
};

/*
4.2 Implementation of the class ~Relation~

This class implements the persistent representation of the type constructor ~rel~.
A relation is stored into two files: one for the tuples and another for the large
objects (FLOBs) of the tuples.

*/
Relation::Relation( const ListExpr typeInfo, const bool isTemporary ):
  privateRelation( new PrivateRelation( typeInfo, isTemporary ) )
  {}

Relation::Relation( const TupleType& tupleType, const bool isTemporary ):
  privateRelation( new PrivateRelation( tupleType, isTemporary ) )
  {}

Relation::Relation( const TupleType& tupleType, const RelationDescriptor& relDesc, const bool isTemporary ):
  privateRelation( new PrivateRelation( tupleType, relDesc, isTemporary ) )
  {}

Relation::Relation( const ListExpr typeInfo, const RelationDescriptor& relDesc, const bool isTemporary ):
  privateRelation( new PrivateRelation( typeInfo, relDesc, isTemporary ) )
  {}

Relation::~Relation()
{
  delete privateRelation;
}

bool Relation::Open( SmiRecord& valueRecord, const ListExpr typeInfo, Relation*& value )
{
  SmiFileId tupleId, lobId;
  int noTuples;
  valueRecord.Read( &tupleId, sizeof( SmiFileId ), 0 );
  valueRecord.Read( &lobId, sizeof( SmiFileId ), sizeof( SmiFileId ) );
  valueRecord.Read( &noTuples, sizeof( int ), 2 * sizeof( SmiFileId ) );

  RelationDescriptor relDesc( noTuples, tupleId, lobId );
  value = new Relation( typeInfo, relDesc );

  return true;
}

bool Relation::Save( SmiRecord& valueRecord, const ListExpr typeInfo )
{
  SmiFileId tupleId = privateRelation->tupleFile.GetFileId(),
            lobId = privateRelation->lobFile.GetFileId();
  valueRecord.Write( &tupleId, sizeof( SmiFileId ), 0 );
  valueRecord.Write( &lobId, sizeof( SmiFileId ), sizeof( SmiFileId ) );
  valueRecord.Write( &(privateRelation->noTuples), sizeof( int ), 2 * sizeof( SmiFileId ) );

  return true;
}

void Relation::Close()
{
  delete this;
}

void Relation::Delete()
{
  privateRelation->tupleFile.Drop();
  privateRelation->lobFile.Drop();
  delete this;
}

void Relation::AppendTuple( Tuple *tuple )
{
  tuple->GetPrivateTuple()->Save( &privateRelation->tupleFile, &privateRelation->lobFile );
  privateRelation->noTuples += 1;
}

void Relation::Clear()
{
  privateRelation->noTuples = 0;
  assert( privateRelation->tupleFile.Drop() );
  assert( privateRelation->tupleFile.Create() );
  assert( privateRelation->lobFile.Drop() );
  assert( privateRelation->lobFile.Create() );
}

const int Relation::GetNoTuples() const
{
  return privateRelation->noTuples;
}

RelationIterator *Relation::MakeScan() const
{
  return new RelationIterator( *this );
}

RelationIterator *Relation::MakeSortedScan( const TupleCompare* tupleCompare ) const
{
  return new RelationIterator( *this, tupleCompare );
}

#ifdef _PREFETCHING_
/*
4.3 Struct ~PrivateRelationIterator~ (using ~PrefetchingIterator~)

This struct contains the private attributes of the class ~RelationIterator~.

*/
struct PrivateRelationIterator
{
  PrivateRelationIterator( const Relation& rel, const TupleCompare* tupleCompare ):
    iterator( rel.privateRelation->tupleFile.SelectAllPrefetched() ),
    order( tupleCompare == 0 ? 0 : new vector<TupleId>() ),
    currentTuple( 0 ),
    tupleCompare( tupleCompare ),
    relation( rel ),
    endOfScan( false )
    {
      if( tupleCompare != 0 )
      {
        while( iterator->Next() )
        {
          TupleId id;
          iterator->ReadCurrentRecordNumber( id.value );
          order->push_back( id );
        }
        delete iterator;
        Sort();
      }
    }
/*
The constructor.

*/
  ~PrivateRelationIterator()
  {
    if( tupleCompare )
      delete order;
    else
      delete iterator;
  }
/*
The destructor.

*/
  PrefetchingIterator *iterator;
/*
The iterator.

*/
  vector<TupleId> *order;
/*
The order of the tuples to be read. This works for a sorted iterator.

*/
  unsigned int currentTuple;
/*
The current tuple in the order vector. Only used for sorted iteration.

*/
  const TupleCompare* tupleCompare;
/*
The tuple comparison criteria.

*/
  const Relation& relation;
/*
A reference to the relation.

*/
  bool endOfScan;
/*
Stores the state of the iterator.

*/
  private:
    void Sort();
    void QuickSortRecursive( const int low, const int high );
/*
Functions for sorting the iterator.

*/
};

/*
4.4 Implementation of the class ~RelationIterator~ (using ~PrefetchingIterator~)

This class is used for scanning (iterating through) relations.

*/
RelationIterator::RelationIterator( const Relation& relation, const TupleCompare* tupleCompare ):
  privateRelationIterator( new PrivateRelationIterator( relation, tupleCompare ) )
  {}

RelationIterator::~RelationIterator()
{
  delete privateRelationIterator;
}

Tuple* RelationIterator::GetNextTuple()
{
  if( privateRelationIterator->tupleCompare == 0 )
  {
    if( !privateRelationIterator->iterator->Next() )
    {
      privateRelationIterator->endOfScan = true;
      return 0;
    }

    Tuple *result = new Tuple( privateRelationIterator->relation.privateRelation->tupleType );
    result->GetPrivateTuple()->Open( &privateRelationIterator->relation.privateRelation->tupleFile,
                                     &privateRelationIterator->relation.privateRelation->lobFile,
                                     privateRelationIterator->iterator );
    return result;
  }
  else
  {
    if( privateRelationIterator->currentTuple == privateRelationIterator->order->size() )
    {
      privateRelationIterator->endOfScan = true;
      return 0;
    }

    Tuple *result = new Tuple( privateRelationIterator->relation.privateRelation->tupleType );
    result->GetPrivateTuple()->Open( &privateRelationIterator->relation.privateRelation->tupleFile,
                                     &privateRelationIterator->relation.privateRelation->lobFile,
                                     (*privateRelationIterator->order)[privateRelationIterator->currentTuple].value );
    privateRelationIterator->currentTuple++;
    return result;
  }
}

const bool RelationIterator::EndOfScan()
{
  return privateRelationIterator->endOfScan;
}
#else
/*
4.5 Struct ~PrivateRelationIterator~ (using ~SmiRecordFileIterator~)

This struct contains the private attributes of the class ~RelationIterator~.

*/
struct PrivateRelationIterator
{
  PrivateRelationIterator( const Relation& rel, const TupleCompare *tupleCompare ):
    iterator(),
    order( tupleCompare == 0 ? 0 : new vector<TupleId>() ),
    currentTuple( 0 ),
    tupleCompare( tupleCompare ),
    relation( rel )
    {
      rel.privateRelation->tupleFile.SelectAll( iterator );
      if( tupleCompare != 0 )
      {
        SmiRecord record;
        while( iterator.Next( record ) )
        {
          TupleId id;
          SmiKey mykey;
          mykey = record.GetKey();
          mykey.GetKey( id.value );
          order->push_back( id );
        }
        Sort();
      }
    }
/*
The constructor.

*/
  ~PrivateRelationIterator()
  {}
/*
The destructor.

*/
  SmiRecordFileIterator iterator;
/*
The iterator.

*/
  vector<TupleId> *order;
/*
The order of the tuples to be read. This works for a sorted iterator.

*/
  unsigned int currentTuple;
/*
The current tuple in the order vector. Only used for sorted iteration.

*/
  const TupleCompare* tupleCompare;
/*
The tuple comparison criteria.

*/
  const Relation& relation;
/*
A reference to the relation.

*/
  private:
    void Sort();
    void QuickSortRecursive( const int low, const int high );
/*
Functions for sorting the iterator.

*/
};

/*
4.6 Implementation of the class ~RelationIterator~ (using ~SmiRecordFileIterator~)

This class is used for scanning (iterating through) relations.

*/
RelationIterator::RelationIterator( const Relation& relation, const TupleCompare* tupleCompare ):
  privateRelationIterator( new PrivateRelationIterator( relation, tupleCompare ) )
  {}

RelationIterator::~RelationIterator()
{
  delete privateRelationIterator;
}

Tuple* RelationIterator::GetNextTuple()
{
  if( privateRelationIterator->tupleCompare == 0 )
  {
    SmiRecord *record = new SmiRecord();
    privateRelationIterator->iterator.Next( *record );

    if( EndOfScan() )
      return 0;

    Tuple *result = new Tuple( privateRelationIterator->relation.privateRelation->tupleType );
    result->GetPrivateTuple()->Open( &privateRelationIterator->relation.privateRelation->tupleFile,
                                     &privateRelationIterator->relation.privateRelation->lobFile,
                                     record );
    return result;
  }
  else
  {
    if( privateRelationIterator->currentTuple == privateRelationIterator->order->size() )
      return 0;

    Tuple *result = new Tuple( privateRelationIterator->relation.privateRelation->tupleType );
    result->GetPrivateTuple()->Open( &privateRelationIterator->relation.privateRelation->tupleFile,
                                     &privateRelationIterator->relation.privateRelation->lobFile,
                                     (*privateRelationIterator->order)[privateRelationIterator->currentTuple] );
    privateRelationIterator->currentTuple++;
    return result;

  }
}

const bool RelationIterator::EndOfScan()
{
  if( privateRelationIterator->tupleCompare != 0 )
    return ( privateRelationIterator->currentTuple == privateRelationIterator->order->size() );
  else
    return privateRelationIterator->iterator.EndOfScan();
}

#endif // _PREFETCHING_

/*
4.7 Implementation of class ~PrivateRelationIterator~

Implementation of common functions (sorting functions) of the ~PrivateRelationIterator~ class
for prefetching and non-prefetching iterators.

*/
void PrivateRelationIterator::Sort()
{
  assert( order != 0 && tupleCompare != 0 );
  if( order->size() > 1 )
  {
    int left = 0, right = order->size() - 1;
    QuickSortRecursive( left, right );
  }
}

void PrivateRelationIterator::QuickSortRecursive( const int low, const int high )
{
  int l = low;
  int h = high;
  Tuple *tl = 0, *th = 0;

  if (l >= h)
  {
    return;
  }
  else if( l == h - 1 )
    // sort a two element list by swapping if necessary
  {
    tl = new Tuple( relation.privateRelation->tupleType );
    tl->GetPrivateTuple()->Open( &relation.privateRelation->tupleFile,
                                 &relation.privateRelation->lobFile,
                                 (*order)[l].value );

    th = new Tuple( relation.privateRelation->tupleType );
    th->GetPrivateTuple()->Open( &relation.privateRelation->tupleFile,
                                 &relation.privateRelation->lobFile,
                                 (*order)[h].value );

    if( (*tupleCompare)( th, tl ) )
    {
      TupleId aux = (*order)[l];
      (*order)[l] = (*order)[h];
      (*order)[h] = aux;
    }
    delete tl;
    delete th;
    return;
  }

  // Pick a pivot and move it out of the way
  Tuple *pivot = new Tuple( relation.privateRelation->tupleType );
  pivot->GetPrivateTuple()->Open( &relation.privateRelation->tupleFile,
                                  &relation.privateRelation->lobFile,
                                  (*order)[(l + h) / 2].value );

  TupleId pivotPos = (*order)[(l + h) / 2];
  (*order)[(l + h) / 2] = (*order)[h];
  (*order)[h] = pivotPos;

  while( l < h )
  {
    // Search forward from a[lo] until an element is found that
    // is greater than the pivot or lo >= hi
    delete tl;
    tl = new Tuple( relation.privateRelation->tupleType );
    tl->GetPrivateTuple()->Open( &relation.privateRelation->tupleFile,
                                 &relation.privateRelation->lobFile,
                                 (*order)[l].value );
    while( !(*tupleCompare)( pivot, tl ) && l < h )
    {
      l++;
      delete tl;
      tl = new Tuple( relation.privateRelation->tupleType );
      tl->GetPrivateTuple()->Open( &relation.privateRelation->tupleFile,
                                   &relation.privateRelation->lobFile,
                                   (*order)[l].value );
    }

    // Search backward from a[hi] until element is found that
    // is less than the pivot, or lo >= hi
    delete th;
    th = new Tuple( relation.privateRelation->tupleType );
    th->GetPrivateTuple()->Open( &relation.privateRelation->tupleFile,
                                 &relation.privateRelation->lobFile,
                                 (*order)[h].value );
    while( !(*tupleCompare)( th, pivot ) && l < h )
    {
      h--;
      delete th;
      th = new Tuple( relation.privateRelation->tupleType );
      th->GetPrivateTuple()->Open( &relation.privateRelation->tupleFile,
                                   &relation.privateRelation->lobFile,
                                   (*order)[h].value );
    }

    // Swap elements a[lo] and a[hi]
    if( l < h )
    {
      TupleId aux = (*order)[l];
      (*order)[l] = (*order)[h];
      (*order)[h] = aux;
    }
  }

  // Put the median in the "center" of the list
  (*order)[high] = (*order)[h];
  (*order)[h] = pivotPos;

  delete pivot;
  delete tl;
  delete th;

  // Recursive calls, elements a[lo0] to a[lo-1] are less than or
  // equal to pivot, elements a[hi+1] to a[hi0] are greater than
  // pivot.
  QuickSortRecursive(low, l-1);
  QuickSortRecursive(h+1, high);
}

/*
5 Auxiliary functions

5.1 Function ~Concat~

Copies the attribute values of two tuples
(words) ~r~ and ~s~ into tuple (word) ~t~.

*/
void Concat( Tuple *r, Tuple *s, Tuple *t )
{
  int rnoattrs, snoattrs, tnoattrs;
  Attribute* attr;

  rnoattrs = r->GetNoAttributes();
  snoattrs = s->GetNoAttributes();
  tnoattrs = rnoattrs + snoattrs;

  assert( t->GetNoAttributes() == tnoattrs );

  for( int i = 0; i < rnoattrs; i++)
  {
    attr = r->GetAttribute( i )->Clone();
    t->PutAttribute( i, attr );
  }
  for (int j = 0; j < snoattrs; j++)
  {
    attr = s->GetAttribute( j )->Clone();
    t->PutAttribute( rnoattrs + j, attr );
  }
}

#endif // RELALG_PERSISTENT
