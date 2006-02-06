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

//paragraph [1] Title: [{\Large \bf \begin{center}] [\end{center}}]
//paragraph [10] Footnote: [{\footnote{] [}}]
//[TOC] [\tableofcontents]

[1] Implementation of Module Relation Algebra

[1] Separate part of persistent data representation

June 1996 Claudia Freundorfer

May 2002 Frank Hoffmann port to C++

November 7, 2002 RHG Corrected the type mapping of ~tcount~.

November 30, 2002 RHG Introduced a function ~RelPersistValue~ 
instead of ~DefaultPersistValue~ which keeps relations that have 
been built in memory in a small cache, so that they need not be 
rebuilt from then on.

March 2003 Victor Almeida created the new Relational Algebra 
organization

November 2004 M. Spiekermann. The declarations of the 
PrivateRelation have been moved to the files RelationPersistent.h 
and RelationMainMemory.h. This was necessary to implement some 
little functions as inline functions.

June 2005 M. Spiekermann. The tuple's size information will now be i
stored in member variables and only recomputed after attributes 
were changed. Changes in class ~TupleBuffer~ which allow to store 
tuples as "free" or "non-free" tuples in it.

December 2005, Victor Almeida deleted the deprecated algebra levels
(~executable~, ~descriptive~, and ~hibrid~). Only the executable
level remains. Models are also removed from type constructors.

January 2006 Victor Almeida replaced the ~free~ tuples concept to
reference counters. There are reference counters on tuples and also
on attributes. Some assertions were removed, since the code is
stable.

[TOC]

1 Overview

The Relational Algebra basically implements two type constructors, 
namely ~tuple~ and ~rel~.

More information about the Relational Algebra can be found in the 
RelationAlgebra.h header file.

This file contains the implementation of the Persistent Relational 
Algebra, where the type constructors ~tuple~ and ~rel~ are kept in 
secondary memory.

A relation has two files: the tuple file and the LOB file, for 
storing tuples and LOBs respectively.

The tuples can be in two states, namely ~fresh~ and ~solid~. A 
fresh tuple is a tuple created by the ~in~-function of the ~tuple~ 
type constructor. It is stored in memory and looks like the tuple 
in the Main Memory Relational Algebra. An example schema of such 
tuple can be viewed in Figure 1. Attributes in fresh tuples are
created with ~new~ and therefore must be deleted with ~delete~.

                Figure 1: Example schema of a fresh 
                tuple. [FreshTuple.eps]

A solid tuple is created when a fresh tuple is saved on disk, or 
when a tuple is read from disk. The tuple representation on disk 
has two basic parts: the attributes and the tuple extension for 
small FLOBs. Large FLOBs are written in the LOB file of the 
relation. An example schema of two tuples of the same type as 
the one in Figure 1 can be viewed in Figure 2. The first tuple
contains a small region, and the second contains a big one, 
which is saved separately on the LOB file. Attributes of
solid tuples when opened are created with ~malloc~ and must be
delete with ~free~.

                Figure 2: Example schema of two solid tuples 
                with a small FLOB (a) and with a big one (b).
                [SolidTuple.eps]

The state diagram of tuples can be seen in the Figure 3.

                Figure 3: State diagram of tuples.
                [TupleStateDiagram.eps]

2 Includes, Constants, Globals, Enumerations

*/
#ifdef RELALG_PERSISTENT
/*
This ~RELALG\_PERSISTENT~ defines which kind of relational algebra 
is to be compiled. If it is set, the persistent version of the 
relational algebra will be compiled, and otherwise, the main 
memory version will be compiled.

*/

using namespace std;

#include "QueryProcessor.h"
#include "NestedList.h"
#include "RelationAlgebra.h"
#include "SecondoSystem.h"
#include "SecondoSMI.h"
#include "FLOB.h"
#include "RelationPersistent.h"
#include "LogMsg.h"
#include "FLOBCache.h"

extern NestedList *nl;
extern QueryProcessor *qp;
extern AlgebraManager *am;

/*
3 Type constructor ~tuple~

3.2 Struct ~PrivateTuple~

This struct contains the private attributes of the class ~Tuple~.

*/
int PrivateTuple::Save( SmiRecordFile *tuplefile, 
                        SmiFileId& lobFileId )
{
  int tupleSize = tupleType->GetTotalSize(),
      extensionSize = 0;
  bool hasFLOBs = false;

  // Calculate the size of the small FLOB data which will be 
  // saved together with the tuple attributes and save the LOBs 
  // in the lobFile.
  for( int i = 0; i < tupleType->GetNoAttributes(); i++)
  {
    for( int j = 0; j < attributes[i]->NumOfFLOBs(); j++)
    {
      hasFLOBs = true;
      FLOB *tmpFLOB = attributes[i]->GetFLOB(j);
      tupleSize += tmpFLOB->Size();
      if( !tmpFLOB->IsLob() )
        extensionSize += tmpFLOB->Size();
      else
      {
        tmpFLOB->BringToMemory();
        tmpFLOB->SaveToLob( lobFileId );
      }
    }
  }

  if( state == Solid && hasFLOBs && extensionSize > 0 )
  {
    assert( memoryTuple != 0 );
    assert( (extensionSize == 0 && extensionTuple == 0 ) || 
            (extensionSize != 0 && extensionTuple != 0 ) );

    for( int i = 0; i < tupleType->GetNoAttributes(); i++)
    {
      for( int j = 0; j < attributes[i]->NumOfFLOBs(); j++)
      {
        FLOB *tmpFLOB = attributes[i]->GetFLOB(j);
        if( !tmpFLOB->IsLob() )
          tmpFLOB->SaveToExtensionTuple( 0 );
      }
    }   
  }
  else if( state == Fresh )
  { 
    // Move FLOB data to extension tuple.
    if( hasFLOBs )
    {
      if( extensionSize > 0 )
        extensionTuple = (char *)malloc(extensionSize);

      char *extensionPtr = extensionTuple;
      for( int i = 0; i < tupleType->GetNoAttributes(); i++)
      {
        for( int j = 0; j < attributes[i]->NumOfFLOBs(); j++)
        {
          FLOB *tmpFLOB = attributes[i]->GetFLOB(j);
          if( !tmpFLOB->IsLob() )
          {
            tmpFLOB->SaveToExtensionTuple( extensionPtr );
            extensionPtr += tmpFLOB->Size();
          }
        }
      } 
    }

    // Move external attributes to memory tuple
    assert( memoryTuple == 0 );
    memoryTuple = (char*)malloc( tupleType->GetTotalSize() );
    int offset = 0;
    for( int i = 0; i < tupleType->GetNoAttributes(); i++)
    {
      memcpy( &memoryTuple[offset], attributes[i], 
              tupleType->GetAttributeType(i).size );
      attributes[i]->DeleteIfAllowed();
      attributes[i] =
        (Attribute*)(*(am->Cast(tupleType->GetAttributeType(i).algId, 
                                   tupleType->GetAttributeType(i).typeId)))(&memoryTuple[offset]);
      offset += tupleType->GetAttributeType(i).size;
    }
  }

  tupleFile = tuplefile;
  SmiRecord *tupleRecord = new SmiRecord();
  tupleId = 0;
  bool rc = tupleFile->AppendRecord( tupleId, *tupleRecord );
  rc = 
    tupleRecord->Write(memoryTuple, tupleType->GetTotalSize(), 0) && 
    rc;
  if( extensionSize > 0 )
    rc = tupleRecord->Write( extensionTuple, extensionSize, 
                             tupleType->GetTotalSize() ) && rc;

  tupleRecord->Finish();
  delete tupleRecord;

  state = Solid;
  this->lobFileId = lobFileId;

  if( !rc )
    return 0;
  return tupleSize;
}

bool PrivateTuple::Open( SmiRecordFile *tuplefile, 
                         SmiFileId lobfileId, SmiRecordId rid )
{
  tupleId = rid;
  SmiRecord *tupleRecord = new SmiRecord();
  this->tupleFile = tuplefile;
  this->lobFileId = lobfileId;
  if( !tupleFile->SelectRecord( tupleId, *tupleRecord ) )
  {
    delete tupleRecord;
    return false;
  }

  size_t offset = 0;
  memoryTuple = (char *)malloc( tupleType->GetTotalSize() );
  if( (int)tupleRecord->Read( memoryTuple, tupleType->GetTotalSize(),
                               offset ) != tupleType->GetTotalSize() )
  {
    tupleRecord->Finish();
    delete tupleRecord;
    free( memoryTuple ); memoryTuple = 0;
    return false;
  }
  offset += tupleType->GetTotalSize();

  // Read attribute values from memoryTuple.
  // Calculate the size of the extension tuple.
  // Set the lobFile for all LOBs.
  size_t extensionSize = 0;
  char *valuePtr = memoryTuple;
  for( int i = 0; i < tupleType->GetNoAttributes(); i++ )
  {
    int algId = tupleType->GetAttributeType(i).algId,
        typeId = tupleType->GetAttributeType(i).typeId;
    attributes[i] = 
      (Attribute*)(*(am->Cast(algId, typeId)))(valuePtr);
    valuePtr += tupleType->GetAttributeType(i).size;

    for( int j = 0; j < attributes[i]->NumOfFLOBs(); j++ )
    {
      FLOB *tmpFLOB = attributes[i]->GetFLOB( j );
      if( !tmpFLOB->IsLob() )
        extensionSize += tmpFLOB->Size();
      else
        tmpFLOB->SetLobFileId( lobFileId );
    }
  }

  // Read the small FLOBs. The read of LOBs is postponed to its 
  // usage. 
  if( extensionSize > 0 )
  {
    extensionTuple = (char*)malloc( extensionSize );
    if( tupleRecord->Read( extensionTuple, extensionSize,
                           offset ) != extensionSize )
    {
      tupleRecord->Finish();
      delete tupleRecord;
      free( memoryTuple ); memoryTuple = 0;
      free( extensionTuple ); extensionTuple = 0;
      return false;
    }

    char *extensionPtr = extensionTuple;
    for( int i = 0; i < tupleType->GetNoAttributes(); i++ )
    {
      for( int j = 0; j < attributes[i]->NumOfFLOBs(); j++ )
      {
        FLOB *tmpFLOB = attributes[i]->GetFLOB( j );
        if( !tmpFLOB->IsLob() )
        {
          tmpFLOB->ReadFromExtensionTuple( extensionPtr );
          extensionPtr += tmpFLOB->Size();
        }
      }
    }
  }

  // Call the Initialize function for every attribute
  for( int i = 0; i < tupleType->GetNoAttributes(); i++ )
    attributes[i]->Initialize();

  tupleRecord->Finish();
  delete tupleRecord;

  state = Solid;
  return true;
}

bool PrivateTuple::Open( SmiRecordFile *tuplefile, 
                         SmiFileId lobfileId, 
                         PrefetchingIterator *iter )
{
  iter->ReadCurrentRecordNumber( tupleId );
  this->tupleFile = tuplefile;
  this->lobFileId = lobfileId;

  size_t offset = 0;
  memoryTuple = (char *)malloc( tupleType->GetTotalSize() );
  if( (int)iter->ReadCurrentData( memoryTuple, 
                                  tupleType->GetTotalSize(), 
                                  offset ) != 
      tupleType->GetTotalSize() )
  {
    free( memoryTuple ); memoryTuple = 0;
    return false;
  }
  offset += tupleType->GetTotalSize();

  // Read attribute values from memoryTuple.
  // Calculate the size of the extension tuple.
  // Set the lobFile for all LOBs.
  size_t extensionSize = 0;
  char *valuePtr = memoryTuple;
  for( int i = 0; i < tupleType->GetNoAttributes(); i++ )
  {
    int algId = tupleType->GetAttributeType(i).algId,
        typeId = tupleType->GetAttributeType(i).typeId;
    attributes[i] =
      (Attribute*)(*(am->Cast(algId, typeId)))(valuePtr);
    valuePtr += tupleType->GetAttributeType(i).size;

    for( int j = 0; j < attributes[i]->NumOfFLOBs(); j++ )
    {
      FLOB *tmpFLOB = attributes[i]->GetFLOB( j );
      if( !tmpFLOB->IsLob() )
        extensionSize += tmpFLOB->Size();
      else
        tmpFLOB->SetLobFileId( lobFileId );
    }
  }

  // Read the small FLOBs. The read of LOBs is postponed to its
  // usage.
  if( extensionSize > 0 )
  {
    extensionTuple = (char*)malloc( extensionSize );
    if( iter->ReadCurrentData( extensionTuple,
                               extensionSize,
                               offset ) !=
        extensionSize )
    {
      free( memoryTuple ); memoryTuple = 0;
      free( extensionTuple ); extensionTuple = 0;
      return false;
    }

    char *extensionPtr = extensionTuple;
    for( int i = 0; i < tupleType->GetNoAttributes(); i++ )
    {
      for( int j = 0; j < attributes[i]->NumOfFLOBs(); j++ )
      {
        FLOB *tmpFLOB = attributes[i]->GetFLOB( j );
        if( !tmpFLOB->IsLob() )
        {
          tmpFLOB->ReadFromExtensionTuple( extensionPtr );
          extensionPtr += tmpFLOB->Size();
        }
      }
    }
  }

  // Call the Initialize function for every attribute
  for( int i = 0; i < tupleType->GetNoAttributes(); i++ )
    attributes[i]->Initialize();

  state = Solid;
  return true;
}

bool PrivateTuple::Open( SmiRecordFile *tuplefile, 
                         SmiFileId lobfileId, 
                         SmiRecord *record )
{
  SmiKey key;
  key = record->GetKey();
  key.GetKey( tupleId );
  this->tupleFile = tuplefile;
  this->lobFileId = lobfileId;
  if( !tupleFile->SelectRecord( tupleId, *record ) )
  {
    record->Finish();
    return false;
  }

  size_t offset = 0;
  memoryTuple = (char *)malloc( tupleType->GetTotalSize() );
  if( (int)record->Read( memoryTuple, 
                              tupleType->GetTotalSize(), offset ) != 
      tupleType->GetTotalSize() )
  {
    free( memoryTuple ); memoryTuple = 0;
    record->Finish();
    return false;
  }
  offset += tupleType->GetTotalSize();

  // Read attribute values from memoryTuple.
  // Calculate the size of the extension tuple.
  // Set the lobFile for all LOBs.
  size_t extensionSize = 0;
  char *valuePtr = memoryTuple;
  for( int i = 0; i < tupleType->GetNoAttributes(); i++ )
  {
    int algId = tupleType->GetAttributeType(i).algId,
        typeId = tupleType->GetAttributeType(i).typeId;
    attributes[i] =
      (Attribute*)(*(am->Cast(algId, typeId)))(valuePtr);
    valuePtr += tupleType->GetAttributeType(i).size;

    for( int j = 0; j < attributes[i]->NumOfFLOBs(); j++ )
    {
      FLOB *tmpFLOB = attributes[i]->GetFLOB( j );
      if( !tmpFLOB->IsLob() )
        extensionSize += tmpFLOB->Size();
      else
        tmpFLOB->SetLobFileId( lobFileId );
    }
  }

  // Read the small FLOBs. The read of LOBs is postponed to its
  // usage.
  if( extensionSize > 0 )
  {
    extensionTuple = (char*)malloc( extensionSize );
    if( record->Read( extensionTuple, extensionSize,
                      offset ) != extensionSize )
    {
      record->Finish();
      free( memoryTuple ); memoryTuple = 0;
      free( extensionTuple ); extensionTuple = 0;
      return false;
    }

    char *extensionPtr = extensionTuple;
    for( int i = 0; i < tupleType->GetNoAttributes(); i++ )
    {
      for( int j = 0; j < attributes[i]->NumOfFLOBs(); j++ )
      {
        FLOB *tmpFLOB = attributes[i]->GetFLOB( j );
        if( !tmpFLOB->IsLob() )
        {
          tmpFLOB->ReadFromExtensionTuple( extensionPtr );
          extensionPtr += tmpFLOB->Size();
        }
      }
    }
  }

  // Call the Initialize function for every attribute
  for( int i = 0; i < tupleType->GetNoAttributes(); i++ )
    attributes[i]->Initialize();

  state = Solid;
  return true;
}

/*
3.3 Implementation of the class ~Tuple~

This class implements the persistent representation of the type 
constructor ~tuple~.

*/
Tuple *Tuple::RestoreFromList( ListExpr typeInfo, ListExpr value, 
                               int errorPos, ListExpr& errorInfo, 
                               bool& correct )
{
  return 0;
}

ListExpr Tuple::SaveToList( ListExpr typeInfo )
{
  return nl->TheEmptyList();
}

const TupleId& Tuple::GetTupleId() const
{
  return (TupleId&)privateTuple->tupleId;
}

void Tuple::SetTupleId( const TupleId& tupleId )
{
  privateTuple->tupleId = (SmiRecordId)tupleId;
}

void Tuple::PutAttribute( const int index, Attribute* attr )
{
  if( privateTuple->attributes[index] != 0 )
    privateTuple->attributes[index]->DeleteIfAllowed();
  privateTuple->attributes[index] = attr;

  recomputeMemSize = true;
  recomputeTotalSize = true;
}

/*
3.9 Class ~TupleBuffer~

This class is used to collect tuples for sorting, for example, or
to do a cartesian product. In this persistent version, if the buffer
is small it will be stored in memory and if it is large, it will be
stored into a disk file.

3.9.1 Struct ~PrivateTupleBuffer~

*/
struct PrivateTupleBuffer
{
  PrivateTupleBuffer( const size_t maxMemorySize ):
    MAX_MEMORY_SIZE( maxMemorySize ),
    diskBuffer( 0 ),
    inMemory( true ),
    totalSize( 0 )
    {}
/*
The constructor.

*/
  ~PrivateTupleBuffer()
  {
    if( !inMemory )
      diskBuffer->Delete();
    else
    {
      for( size_t i = 0; i < memoryBuffer.size(); i++ )
        delete memoryBuffer[i];
    }
  }
/*
The destructor.

*/
  const size_t MAX_MEMORY_SIZE;
/*
The maximum size of the memory in bytes. 32 MBytes being used.

*/
  vector<Tuple*> memoryBuffer;
/*
The memory buffer which is a ~vector~ from STL.

*/
  Relation* diskBuffer;
/*
The buffer stored on disk.

*/
  bool inMemory;
/*
A flag that tells if the buffer fit in memory or not.

*/
  double totalSize;
/*
The total size occupied by the tuples in the buffer.

*/
};

/*
3.9.2 Implementation of the class ~TupleBuffer~

*/
TupleBuffer::TupleBuffer( const size_t maxMemorySize ):
privateTupleBuffer( new PrivateTupleBuffer( maxMemorySize ) )
{
  if (RTFlag::isActive("RA:TupleBufferInfo")) 
  {
    cmsg.info() << "New Instance of TupleBuffer with size " 
                << maxMemorySize/1024 
                << " address = " << (void*)this << endl;
    cmsg.send();
  }
}

TupleBuffer::~TupleBuffer()
{
  delete privateTupleBuffer;
}

int TupleBuffer::GetNoTuples() const
{
  if( privateTupleBuffer->inMemory )
    return privateTupleBuffer->memoryBuffer.size();
  else
    return privateTupleBuffer->diskBuffer->GetNoTuples();
}

double TupleBuffer::GetTotalSize() const
{
  if( privateTupleBuffer->inMemory )
    return privateTupleBuffer->totalSize;
  else
    return privateTupleBuffer->diskBuffer->GetTotalSize();
}

bool TupleBuffer::IsEmpty() const
{
  if( privateTupleBuffer->inMemory )
    return privateTupleBuffer->memoryBuffer.empty();
  else
    return false;
}

void TupleBuffer::Clear()
{
  if( privateTupleBuffer->inMemory )
  {
    for( size_t i = 0; 
         i < privateTupleBuffer->memoryBuffer.size(); 
         i++ )
      delete privateTupleBuffer->memoryBuffer[i];
    privateTupleBuffer->memoryBuffer.clear();
    privateTupleBuffer->totalSize = 0;
  }
  else
  {
    privateTupleBuffer->diskBuffer->Clear();
  }
}

void TupleBuffer::AppendTuple( Tuple *t )
{
  if( privateTupleBuffer->inMemory )
  {
    if( privateTupleBuffer->totalSize + t->GetMemorySize() <= 
        privateTupleBuffer->MAX_MEMORY_SIZE )
    {
      t->IncReference();
      privateTupleBuffer->memoryBuffer.push_back( t );
      privateTupleBuffer->totalSize += t->GetMemorySize();
    }
    else
    {
      if (RTFlag::isActive("RA:TupleBufferInfo")) 
      {
        cmsg.info() << "Changing TupleBuffer's state from inMemory "
                    << "-> !inMemory" << endl;
        cmsg.send();
      }
      privateTupleBuffer->diskBuffer = 
        new Relation( t->GetTupleType(), true );

      vector<Tuple*>::iterator iter = 
        privateTupleBuffer->memoryBuffer.begin();
      while( iter != privateTupleBuffer->memoryBuffer.end() )
      {
        privateTupleBuffer->diskBuffer->AppendTuple( *iter );
        (*iter)->DecReference();
        (*iter)->DeleteIfAllowed();
        iter++;
      }
      privateTupleBuffer->memoryBuffer.clear();
      privateTupleBuffer->totalSize = 0;
      privateTupleBuffer->diskBuffer->AppendTuple( t );
      privateTupleBuffer->inMemory = false;
    }
  }
  else
  {
    return privateTupleBuffer->diskBuffer->AppendTuple( t );
  }
}

Tuple *TupleBuffer::GetTuple( const TupleId& id ) const
{
  if( privateTupleBuffer->inMemory )
  {
	  if( id >= 0 && 
        id < (TupleId)privateTupleBuffer->memoryBuffer.size() &&
        privateTupleBuffer->memoryBuffer[id] != 0 )
      return privateTupleBuffer->memoryBuffer[id];
    return 0;
  }
  else
    return privateTupleBuffer->diskBuffer->GetTuple( id );
}

TupleBufferIterator *TupleBuffer::MakeScan() const
{
  return new TupleBufferIterator( *this );
}

/*
3.9.3 Struct ~PrivateTupleBufferIterator~

*/
struct PrivateTupleBufferIterator
{
  PrivateTupleBufferIterator( const TupleBuffer& tupleBuffer ):
    tupleBuffer( tupleBuffer ),
    currentTuple( 0 ),
    diskIterator( 
      tupleBuffer.privateTupleBuffer->inMemory ?  
        0 : 
        tupleBuffer.privateTupleBuffer->diskBuffer->MakeScan() )
    {
    }
/*
The constructor.

*/
  ~PrivateTupleBufferIterator()
  {
    delete diskIterator;
  }
/*
The destructor.

*/
  const TupleBuffer& tupleBuffer;
/*
A pointer to the tuple buffer.

*/
  size_t currentTuple;
/*
The current tuple if it is in memory.

*/
  RelationIterator *diskIterator;
/*
The iterator if it is not in memory.

*/
};

/*
3.9.3 Implementation of the class ~TupleBufferIterator~

*/
TupleBufferIterator::
TupleBufferIterator( const TupleBuffer& tupleBuffer ):
  privateTupleBufferIterator( 
    new PrivateTupleBufferIterator( tupleBuffer ) )
  { 
  }

TupleBufferIterator::~TupleBufferIterator()
{
  delete privateTupleBufferIterator;
}

Tuple *TupleBufferIterator::GetNextTuple()
{
  if( privateTupleBufferIterator->diskIterator )
  {
    return privateTupleBufferIterator->diskIterator->GetNextTuple();
  }
  else
  {
    if( privateTupleBufferIterator->currentTuple == 
        privateTupleBufferIterator->
          tupleBuffer.privateTupleBuffer->memoryBuffer.size() )
      return 0;

    Tuple *result = 
      privateTupleBufferIterator->tupleBuffer.privateTupleBuffer->
        memoryBuffer[privateTupleBufferIterator->currentTuple];
    privateTupleBufferIterator->currentTuple++;

    return result;
  }
}

TupleId TupleBufferIterator::GetTupleId() const
{
  if( privateTupleBufferIterator->diskIterator )
  {
    return privateTupleBufferIterator->diskIterator->GetTupleId();
  }
  else
  {
    return privateTupleBufferIterator->currentTuple-1;
  }
}

/*
4 Type constructor ~rel~

4.2 Implementation of the class ~Relation~

This class implements the persistent representation of the type 
constructor ~rel~.

*/
map<RelationDescriptor, Relation*, RelationDescriptorCompare> 
Relation::pointerTable;

Relation::Relation( const ListExpr typeInfo, bool isTemp ):
privateRelation( new PrivateRelation( typeInfo, isTemp ) )
{
  RelationDescriptor d( privateRelation->noTuples,
                        privateRelation->totalSize,
                        privateRelation->tupleFile.GetFileId(),
                        privateRelation->lobFileId );
  if( pointerTable.find( d ) == pointerTable.end() )
    pointerTable.insert( make_pair( d, this ) );
}

Relation::Relation( TupleType *tupleType, bool isTemp ):
privateRelation( new PrivateRelation( tupleType, isTemp ) )
{
  RelationDescriptor d( privateRelation->noTuples,
                        privateRelation->totalSize,
                        privateRelation->tupleFile.GetFileId(),
                        privateRelation->lobFileId );
  if( pointerTable.find( d ) == pointerTable.end() )
    pointerTable.insert( make_pair( d, this ) );
}

Relation::Relation( TupleType *tupleType, 
                    const RelationDescriptor& relDesc, 
                    bool isTemp ):
privateRelation( new PrivateRelation( tupleType, relDesc, isTemp ) )
{
  if( pointerTable.find( relDesc ) == pointerTable.end() )
    pointerTable.insert( make_pair( relDesc, this ) );
}

Relation::Relation( const ListExpr typeInfo, 
                    const RelationDescriptor& relDesc, 
                    bool isTemp ):
privateRelation( new PrivateRelation( typeInfo, relDesc, isTemp ) )
{
  if( pointerTable.find( relDesc ) == pointerTable.end() )
    pointerTable.insert( make_pair( relDesc, this ) );
}

Relation::~Relation()
{
  delete privateRelation;
}

Relation *Relation::GetRelation( const RelationDescriptor& d )
{
  map<RelationDescriptor, Relation*>::iterator 
    i = pointerTable.find( d );
  if( i == pointerTable.end() )
    return 0;
  else
    return i->second;
}

Relation *
Relation::RestoreFromList( ListExpr typeInfo, ListExpr value, 
                           int errorPos, ListExpr& errorInfo, 
                           bool& correct )
{
  RelationDescriptor relDesc( nl->IntValue( nl->First( value ) ),
                              nl->RealValue( nl->Second( value ) ),
                              nl->IntValue( nl->Third( value ) ),
                              nl->IntValue( nl->Fourth( value ) ));
  return new Relation( typeInfo, relDesc );
}

ListExpr 
Relation::SaveToList( ListExpr typeInfo )
{
  return nl->FourElemList( 
           nl->IntAtom( privateRelation->noTuples ),
           nl->RealAtom( privateRelation->totalSize ),
           nl->IntAtom( privateRelation->tupleFile.GetFileId() ),
           nl->IntAtom( privateRelation->lobFileId ) );
}

Relation *
Relation::Open( SmiRecord& valueRecord, size_t& offset, 
                const ListExpr typeInfo )
{
  SmiFileId tupleId, lobId;
  int noTuples;
  double totalSize;
  valueRecord.Read( &tupleId, sizeof( SmiFileId ), offset );
  offset += sizeof( SmiFileId );
  valueRecord.Read( &lobId, sizeof( SmiFileId ), offset );
  offset += sizeof( SmiFileId );
  valueRecord.Read( &noTuples, sizeof( int ), offset );
  offset += sizeof( int );
  valueRecord.Read( &totalSize, sizeof( double ), offset );
  offset += sizeof( double );

  RelationDescriptor relDesc( noTuples, totalSize, tupleId, lobId );
  return new Relation( typeInfo, relDesc );
}

bool 
Relation::Save( SmiRecord& valueRecord, size_t& offset, 
                const ListExpr typeInfo )
{
  SmiFileId tupleId = privateRelation->tupleFile.GetFileId(),
            lobId = privateRelation->lobFileId;
  valueRecord.Write( &tupleId, sizeof(SmiFileId), offset );
  offset += sizeof(SmiFileId);
  valueRecord.Write( &lobId, sizeof(SmiFileId), offset );
  offset += sizeof(SmiFileId);
  valueRecord.Write( &(privateRelation->noTuples), sizeof(int), 
                     offset );
  offset += sizeof(int);
  valueRecord.Write( &(privateRelation->totalSize), sizeof(double),
                     offset );
  offset += sizeof(double);

  return true;
}

void Relation::Close()
{
  delete this;
}

void Relation::Delete()
{
  privateRelation->tupleFile.Close();
  privateRelation->tupleFile.Drop();
  qp->GetFLOBCache()->Drop( privateRelation->lobFileId, 
                            privateRelation->isTemp );
  delete this;
}

Relation *Relation::Clone()
{
  Relation *r = new Relation( privateRelation->tupleType );

  Tuple *t;
  RelationIterator *iter = MakeScan();
  while( (t = iter->GetNextTuple()) != 0 )
  {
    r->AppendTuple( t );
    t->DeleteIfAllowed();
  }
  delete iter;

  return r;
}

void Relation::AppendTuple( Tuple *tuple )
{
  privateRelation->totalSize +=
    tuple->GetPrivateTuple()->Save( &privateRelation->tupleFile, 
                                    privateRelation->lobFileId );
  privateRelation->noTuples += 1;
}
 
void Relation::Clear()
{
  privateRelation->noTuples = 0;
  privateRelation->totalSize = 0;
  privateRelation->tupleFile.Truncate();
  qp->GetFLOBCache()->Truncate( privateRelation->lobFileId, 
                                privateRelation->isTemp );
}

Tuple *Relation::GetTuple( const TupleId& id ) const
{
  Tuple *result = new Tuple( privateRelation->tupleType );
  if( result->GetPrivateTuple()->Open( &privateRelation->tupleFile,
                                       privateRelation->lobFileId,
                                       id ) )
    return result;

  delete result;
  return 0;
}

int Relation::GetNoTuples() const
{
  return privateRelation->noTuples;
}

TupleType *Relation::GetTupleType() const
{
  return privateRelation->tupleType;
}

double Relation::GetTotalSize() const
{
  return privateRelation->totalSize;
}

RelationIterator *Relation::MakeScan() const
{
  return new RelationIterator( *this );
}

#ifdef _PREFETCHING_
/*
4.3 Struct ~PrivateRelationIterator~ (using ~PrefetchingIterator~)

This struct contains the private attributes of the class 
~RelationIterator~.

*/
struct PrivateRelationIterator
{
  PrivateRelationIterator( const Relation& rel ):
    iterator( rel.privateRelation->tupleFile.SelectAllPrefetched() ),
    relation( rel ),
    endOfScan( false ),
    currentTupleId( -1 )
    {
    }
/*
The constructor.

*/
  ~PrivateRelationIterator()
  {
    delete iterator;
  }
/*
The destructor.

*/
  PrefetchingIterator *iterator;
/*
The iterator.

*/
  const Relation& relation;
/*
A reference to the relation.

*/
  bool endOfScan;
/*
Stores the state of the iterator.

*/
  TupleId currentTupleId;
/*
Stores the identification of the current tuple.

*/
};

/*
4.4 Implementation of the class ~RelationIterator~ 
(using ~PrefetchingIterator~)

This class is used for scanning (iterating through) relations.

*/
RelationIterator::RelationIterator( const Relation& relation ):
  privateRelationIterator( new PrivateRelationIterator( relation ) )
  {}

RelationIterator::~RelationIterator()
{
  delete privateRelationIterator;
}

Tuple* RelationIterator::GetNextTuple()
{
  if( !privateRelationIterator->iterator->Next() )
  {
    privateRelationIterator->endOfScan = true;
    privateRelationIterator->currentTupleId = -1;
    return 0;
  }

  Tuple *result = new Tuple( 
    privateRelationIterator->relation.privateRelation->tupleType );
  result->GetPrivateTuple()->Open( 
    &privateRelationIterator->relation.privateRelation->tupleFile,
    privateRelationIterator->relation.privateRelation->lobFileId,
    privateRelationIterator->iterator );
  privateRelationIterator->currentTupleId = result->GetTupleId();
  return result;
}

TupleId RelationIterator::GetTupleId() const
{
  return privateRelationIterator->currentTupleId;
}

bool RelationIterator::EndOfScan()
{
  return privateRelationIterator->endOfScan;
}
#else
/*
4.5 Struct ~PrivateRelationIterator~ (using ~SmiRecordFileIterator~)

This struct contains the private attributes of the class 
~RelationIterator~.

*/
struct PrivateRelationIterator
{
  PrivateRelationIterator( const Relation& rel ):
    iterator(),
    relation( rel ), currentTupleId( -1 )
    {
      rel.privateRelation->tupleFile.SelectAll( iterator );
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
  const Relation& relation;
/*
A reference to the relation.

*/

  TupleId currentTupleId;
/*
Stores the identification of the current tuple.

*/

};

/*
4.6 Implementation of the class ~RelationIterator~ 
(using ~SmiRecordFileIterator~)

This class is used for scanning (iterating through) relations.

*/
RelationIterator::RelationIterator( const Relation& relation ):
  privateRelationIterator( new PrivateRelationIterator( relation ) )
  {}

RelationIterator::~RelationIterator()
{
  delete privateRelationIterator;
}

Tuple* RelationIterator::GetNextTuple()
{
  SmiRecord *record = new SmiRecord();
  privateRelationIterator->iterator.Next( *record );

  if( EndOfScan() ){
  	privateRelationIterator->currentTupleId = -1;
    return 0;
  }

  Tuple *result = new Tuple( 
    privateRelationIterator->relation.privateRelation->tupleType );
  result->GetPrivateTuple()->Open( 
    &privateRelationIterator->relation.privateRelation->tupleFile,
    &privateRelationIterator->relation.privateRelation->lobFile,
    record );
  privateRelationIterator->currentTupleId = result->GetTupleId();
  return result;
}

const bool RelationIterator::EndOfScan()
{
  return privateRelationIterator->iterator.EndOfScan();
}

TupleId RelationIterator::GetTupleId() const
{
  return privateRelationIterator->currentTupleId;
}

#endif // _PREFETCHING_

/*
5 Auxiliary functions

5.1 Function ~Concat~

Copies the attribute values of two tuples
(words) ~r~ and ~s~ into tuple (word) ~t~.

*/
void Concat( Tuple *r, Tuple *s, Tuple *t )
{
  int rnoattrs, snoattrs, tnoattrs;

  rnoattrs = r->GetNoAttributes();
  snoattrs = s->GetNoAttributes();
  tnoattrs = rnoattrs + snoattrs;

  for( int i = 0; i < rnoattrs; i++)
    t->CopyAttribute( i, r, i );
  for (int j = 0; j < snoattrs; j++)
    t->CopyAttribute( j, s, rnoattrs + j );
}

#endif // RELALG_PERSISTENT
