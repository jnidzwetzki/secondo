/*
//paragraph	[10]	title:		[{\Large \bf ] [}]
//paragraph	[11]	title:		[{\large \bf ] [}]
//paragraph	[12]	title:		[{\normalsize \bf ] [}]
//paragraph	[21]	table1column:	[\begin{quote}\begin{tabular}{l}]	[\end{tabular}\end{quote}]
//paragraph	[22]	table2columns:	[\begin{quote}\begin{tabular}{ll}]	[\end{tabular}\end{quote}]
//paragraph	[23]	table3columns:	[\begin{quote}\begin{tabular}{lll}]	[\end{tabular}\end{quote}]
//paragraph	[24]	table4columns:	[\begin{quote}\begin{tabular}{llll}]	[\end{tabular}\end{quote}]
//[--------]	[\hline]
//characters	[1]	verbatim:	[$]	[$]
//characters	[2]	formula:	[$]	[$]
//characters    [3]    capital:    [\textsc{]    [}]
//characters    [4]    teletype:   [\texttt{]    [}]
//[ae] [\"a]
//[oe] [\"o]
//[ue] [\"u]
//[ss] [{\ss}]
//[<=] [$\leq$]
//[#]  [\neq]
//[tilde] [\verb|~|]

1 Header File: PArray

Version: 0.7

August 2002 RHG

1.1 Overview

This module offers a generic persistent array implemented on top of the
SecondoSMI interface.



1.2 Interface methods

This module offers the following methods:

[23]	Creation/Removal 	& Access   	& Inquiries	\\ 	
	[--------]
	PArray        		& Get 		& Size		\\  	
	[tilde]PArray		& Put		& Id		\\
	MarkDelete		&		& 		\\

Operations have to follow the protocol shown below:

		Figure 1: Protocol [Protocol.eps]

1.3 Class ~PArray~

An instance of the class is a handle to a persistent array of fixed size with
elements of type ~T~.

*/

#ifndef PARRAY_H
#define PARRAY_H

#include <iostream> 
#include <cassert>
#include <vector>
#include <algorithm>
#include "SecondoSMI.h"

template<class T>
class PArray
{
 public:

  PArray( SmiRecordFile *parrays, const int initsize = 0 );
/*
Creates a new ~SmiRecord~ on the ~SmiRecordFile~ for this
persistent array. One can define an initial size of the persistent
array with the argument ~initsize~. 

*/

  PArray( const int initsize = 0 );
/*
Create a new memory version of the PArray. It is used for temporary arrays.

*/
  
  PArray( SmiRecordFile *parrays, const SmiRecordId& id, const bool update = true );
/*
Opens the ~SmiRecordFile~ and the ~SmiRecord~ for the persistent array. The boolean 
value ~update~ indicates if open mode: ~true~ for update and ~false~ for read-only.

*/

  ~PArray();
/*
Destroys the handle. If the array is marked for deletion, then it also destroys the
persistent array.

*/

  void MarkDelete();
/*
Marks the persistent array for deletion. It will be permanently deleted on the 
destruction of the object.

*Precondition:* The array must be opened in update mode.

*/

  void Put(int const index, const T& elem);
/*
Copies element ~elem~ into the persistent array at index ~index~.

*Precondition:* 0 [<=] ~index~ [<=] ~size~ - 1. The array must be opened in update mode.

*/

  void Get(int const index, T& elem);
/*
Returns the element ~index~ of the array.

*Precondition:* 0 [<=] ~index~ [<=] ~size~ - 1. 

*/

  template<class Compare>
  void Sort( const Compare& cmp )
  {
    if( size <= 1 ) 
      return;

    if( marray == 0 )
    {
      vector<T> aux;
      for( int i = 0; i < size; i++ )
      {
        T t;
        Get( i, t );
        aux.push_back( t );
      }

      sort( aux.begin(), aux.end(), cmp );

      for( int i = 0; i < size; i++ )
      {
        Put( i, aux[i] );
      }
    }
    else
      sort( marray->begin(), marray->end(), cmp );
  }
/*
Sorts the persisten array given the ~cmp~ comparison criteria.

*/

  const int Size() const;
/*
Returns the size of this array.

*/

  const SmiRecordId Id() const;
/*
Returns the identifier of this array.

*/

 private:

  bool writeable;
  SmiRecord record;
  SmiRecordId recid;
  int size;
  bool canDelete;
  SmiRecordFile *parrays;
  vector<T> *marray;

};


/*
2 Implementation of PArray

Version: 0.7

August 2002 RHG

2.1 Overview

This module offers a generic persistent array implemented on top of the
SecondoSMI interface.

*/

template<class T>
PArray<T>::PArray( SmiRecordFile *parrays, const int initsize ) :
writeable( true ),
recid( 0 ),
size( 0 ),
canDelete( false ),
parrays( parrays ),
marray( 0 )
{
  parrays->AppendRecord( recid, record );
  record.Write( &size, sizeof(int), 0 );

  int nil = 0;
  record.Write( &nil, sizeof(int), sizeof(int) + initsize * sizeof(T) - sizeof(int) );
}

template<class T>
PArray<T>::PArray( const int initsize ) :
writeable( true ),
size( 0 ),
canDelete( false ),
parrays( 0 ),
marray( new vector<T>( initsize ) )
{
}

template<class T>
PArray<T>::PArray( SmiRecordFile *parrays, const SmiRecordId& id, const bool update ) :
writeable( update ),
canDelete( false ),
parrays( parrays ),
marray( 0 )
{
  SmiFile::AccessType at = update ? SmiFile::Update : SmiFile::ReadOnly;
  assert( parrays->SelectRecord( id, record, at ) );
  recid = id;
  record.Read( &size, sizeof( int ) );
}

template<class T>
PArray<T>::~PArray()
{
  if( marray )
  {
    delete marray;  
  }  
  else
  {
    if ( canDelete ) 
    {
      parrays->DeleteRecord( recid );
    }
    else if ( writeable )
    {
      record.Write( &size, sizeof( int ) );
    }
  }
}

template<class T>
void PArray<T>::Put(const int index, const T& elem)
{
  assert ( writeable );
  
  if ( size <= index ) 
    size = index + 1;

  if( marray )
  {
    marray->resize( size );
    (*marray)[index] = elem;
  }
  else
  {	
    record.Write(&elem, sizeof(T), sizeof(int) + index * sizeof(T));
  }
}


template<class T>
void PArray<T>::Get(int const index, T& elem)
{
  assert ( 0 <= index && index < size );

  if( marray )
  {
    elem = (*marray)[index];
  }
  else
  {
    record.Read(&elem, sizeof(T), sizeof(int) + index * sizeof(T));
  }
}

template<class T>
void PArray<T>::MarkDelete() 
{
  assert( writeable );
  canDelete = true;
}


template<class T>
const int PArray<T>::Size() const
{
  return size;
}


template<class T>
const SmiRecordId PArray<T>::Id() const 
{ 
  return recid;
}

#endif

