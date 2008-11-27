/* 
---- 
This file is part of SECONDO.

Copyright (C) 2004-2007, University in Hagen, Faculty of Mathematics and
Computer Science, Database Systems for New Applications.

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

Oct 2004. M. Spiekermann. The class ~SortByLocalInfo~ was revised, since it
doesn't work for relations not fitting into memory. Moreover, some minor
performance tuning changes were made (fixed size for the vector of tuples).

Nov 2004. M. Spiekermann. The Algorithm for external sorting was changed. See
below for details.

Sept. 2005. M. Spiekermann. Class ~SortbyLocalInfo~ was altered to utilize
class ~TupleBuffer~ instead of temporary relation objects. Moreover, a memory
leak in the ~sortmergejoin~ value mapping was fixed. 

January 2006 Victor Almeida. The ~free~ tuples concept was replaced by
reference counting. There are reference counters on tuples and also on
attributes.  Additionally, some assertions in stable parts of the code were
removed.

May 2007, M. Spiekermann. The class ~MergeJoinLocalInfo~ was rearranged. Now it
uses only one ~TupleBuffer~ and creates groups of equal tuples over the 2nd
argument. A new implementation was needed since the old one did not work
correctly when large groups of equal values appeared which must be stored
temporarily on disk. Moreover, the sorting is now done by instantiation of
~SortbyLocalInfo~ objects instead of calling the value mapping function of
the ~sortby~ operator.

[1] Implementation of the Module Extended Relation Algebra for Persistent Storage

[TOC]

0 Overview

This file contains the implementation of algorithms for external sorting,
merging and a simple hash-join. 


1 Includes and defines

*/

#include <vector>
#include <list>
#include <set>
#include <queue>

#include "LogMsg.h"
#include "StandardTypes.h"
#include "RelationAlgebra.h"
#include "CPUTimeMeasurer.h"
#include "QueryProcessor.h"
#include "SecondoInterface.h"
#include "StopWatch.h"
#include "Counter.h"
#include "Progress.h"
#include "RTuple.h"

extern NestedList* nl;
extern QueryProcessor* qp;

/*
2 Operators

2.1 Operators ~sort~ and ~sortby~

This operator sorts a stream of tuples by a given list of attributes.
For each attribute it must be specified wether the list should be sorted
in ascending (asc) or descending (desc) order with regard to that attribute.

2.2.1 Auxiliary definitions for value mapping function of operators ~sort~ and ~sortby~

*/

static LexicographicalTupleSmaller lexCmp;

class TupleAndRelPos {
public:

  TupleAndRelPos()       :
    ref(0),
    pos(0),
    cmpPtr(0) 
  {};
  
  ~TupleAndRelPos()
  {}      

  TupleAndRelPos(Tuple* newTuple, TupleCompareBy* cmpObjPtr = 0, 
                 int newPos = 0) :
    ref( newTuple ),
    pos(newPos),
    cmpPtr(cmpObjPtr)
  {}; 


  inline TupleAndRelPos(const TupleAndRelPos& rhs) :
    ref(rhs.ref),
    pos(rhs.pos),
    cmpPtr(rhs.cmpPtr)  
  {};     

  /* without the code below we get a better performance
   *
  inline TupleAndRelPos& operator=(const TupleAndRelPos& rhs)
  {
    ref = rhs.ref;
    pos = rhs.pos;
    cmpPtr = rhs.cmpPtr;
    return *this;    
  } */      
 
  inline bool operator<(const TupleAndRelPos& rhs) const 
  { 
    // by default < is used to define a sort order
    // the priority queue creates a maximum heap, hence
    // we change the result to create a minimum queue.
    // It would be nice to have also an < operator in the class
    // Tuple. Moreover lexicographical comparison should be done by means of
    // TupleCompareBy and an appropriate sort order specification, 

    if (!this->ref || !rhs.ref) {
      return true;
    }

    //return !cmp->(this->rt.tuple, ref.rt.tuple);

    
    if ( cmpPtr ) {
      return !(*(TupleCompareBy*)cmpPtr)( this->ref, rhs.ref );
    } else {
      return !lexCmp( this->ref, rhs.ref );
    }
    
  }

  inline Tuple* tuple() const { return ref; }

  Tuple* ref;
  int pos;

private:
   void* cmpPtr;

};


class TupleQueue {    
  
  typedef priority_queue<TupleAndRelPos> Queue;
  Queue q;

  public:
  TupleQueue() {}
  ~TupleQueue() {}

  inline const TupleAndRelPos& top() { return q.top(); }

  inline Tuple* topTuple() { return q.top().tuple(); }

  inline void push(const TupleAndRelPos& elem) 
  {
    elem.tuple()->IncReference();         
    q.push(elem); 
  }
 
  inline size_t size() { return q.size(); }

  inline bool empty() { return q.empty(); }

  inline void pop() 
  {
    Tuple* t = q.top().tuple();
    t->DeleteIfAllowed();
    q.pop();
  }       

};

/*class TupleAndRelPos {
public:

  TupleAndRelPos() :
    ref(0),
    pos(0),
    cmpPtr(0) 
  {};
  
  TupleAndRelPos(Tuple* newTuple, TupleCompareBy* cmpObjPtr = 0, 
                 int newPos = 0) :
    ref(newTuple),
    pos(newPos),
    cmpPtr(cmpObjPtr)
  {}; 

  inline bool operator<(const TupleAndRelPos& rhs) const 
  { 
    // by default < is used to define a sort order
    // the priority queue creates a maximum heap, hence
    // we change the result to create a minimum queue.
    // It would be nice to have also an < operator in the class
    // Tuple. Moreover lexicographical comparison should be done by means of
    // TupleCompareBy and an appropriate sort order specification, 

    if (!this->ref.tuple || !rhs.ref.tuple) {
      return true;
    }
    if ( cmpPtr ) {
      return !(*(TupleCompareBy*)cmpPtr)( this->ref.tuple, rhs.ref.tuple );
    } else {
      return !lexCmp( this->ref.tuple, rhs.ref.tuple );
    }
  }

  inline TupleAndRelPos& operator=(const TupleAndRelPos& rhs)
  { 
    this->ref = rhs.ref;
    return *this; 
  }

  RTuple ref;
  int pos;

private:
  void* cmpPtr;

};*/


/*
2.2.2 class ~SortByLocalInfo~

An algorithm for external sorting is implemented inside this class. The
constructor creates sorted partitions of the input stream and stores them
inside temporary relations and two heaps in memory.  By calls of
~NextResultTuple~ tuples are returned in sorted order. The sort order must be
specified in the constructor. The memory usage is bounded, hence only a fixed
number of tuples can be hold in memory.

The algorithm roughly works as follows: First all input tuples are stored in a
minimum heap until no more tuples fit into memory.  Then, a new relation is
created and the minimum is stored there.  Afterwards, the tuples are handled as
follows:

(a) if the next tuple is less or equal than the minimum of the heap and greater
or equal than the last tuple written to disk, it will be appended to the
current relation

(b) if the next tuple is smaller than the last written it will be stored in a
second heap to be used in the next created relation. 

(c) if the next tuple t is greater than the top of the heap, the minimum will be
written to disk and t will be inserted into the heap.

Finally, the minimum tuple of every temporary relation and the two heaps is
inserted into a probably small heap (containing only one tuple for every
partition) and for every request for tuples this minimum is removed and the
next tuple of the partition of the just returned tuples will be inserted into
the heap.

This algorithm reduces the number of comparisons which are quite costly inside
Secondo (due to usage of C++ Polymorphism) even for ~standard~ attributes.

Moreover, if the input stream is already sorted only one partition will be
created and no costs for merging tuples will occur. Unfortunateley this solution
needs more comparisons than sorting.  


Ideas for future improvement: 

All tuples which are not in order should be collected in a buffer and the
others are written into an relation on disk (maybe also buffered to avoid
writing small results to disk). When the buffer of unsorted tuples is full it
will be sorted and written into a new relation.  While filling the buffer we
can keep track if the inserted tuples are in ascending or descending order.
This algorithm will adapt to sorted streams and will only need N (already sorted)
or 2N (sorted in opposite order) comparisons in that case.  

*/


void*
CreateCompareObject(bool lexOrder, Word* args) {

  void* tupleCmp = 0;

  if(lexOrder) 
  {
     tupleCmp = new LexicographicalTupleSmaller();
  }	
  else
  {
    SortOrderSpecification spec;
    int nSortAttrs = StdTypes::GetInt( args[2] );
    for(int i = 1; i <= nSortAttrs; i++)
    {
      int sortAttrIndex = StdTypes::GetInt( args[2 * i + 1] );
      bool sortOrderIsAscending = StdTypes::GetBool( args[2 * i + 2] );
      
      spec.push_back(pair<int, bool>(sortAttrIndex, 
				     sortOrderIsAscending));
    };

    tupleCmp = new TupleCompareBy( spec );
  }
  return tupleCmp;
} 

#ifndef USE_PROGRESS

//-- begin standard version --//

class SortByLocalInfo
{
  public:
    SortByLocalInfo( Word stream, 
		     const bool lexicographic, 
		     void *tupleCmp ):
      stream( stream ),
      currentIndex( 0 ),
      lexiTupleCmp( lexicographic ? 
                    (LexicographicalTupleCompare*)tupleCmp : 
                    0 ),
      tupleCmpBy( lexicographic ? 0 : (TupleCompareBy*)tupleCmp ),
      lexicographic( lexicographic )
      {
        // Note: Is is not possible to define a Cmp object using the 
        // constructor 
        // mergeTuples( PairTupleCompareBy( tupleCmpBy )). 
        // It does only work if mergeTuples is a local variable which 
        // does not help us in this case. Is it a Compiler bug or C++ feature?
        // Hence a new class TupleAndRelPos was defined which implements 
        // the comparison operator '<'. 
        TupleQueue* currentRun = &queue[0];
        TupleQueue* nextRun = &queue[1];
       
        Word wTuple(Address(0));
        size_t  c = 0, i = 0, a = 0, n = 0, m = 0, r = 0; // counter variables
        bool newRelation = true;


        MAX_MEMORY = qp->MemoryAvailableForOperator();
        cmsg.info("ERA:ShowMemInfo")
          << "Sortby.MAX_MEMORY (" << MAX_MEMORY/1024 << " kb)" << endl;
        cmsg.send();

        TupleBuffer *rel=0;
        TupleAndRelPos lastTuple(0, tupleCmpBy);

        qp->Request(stream.addr, wTuple);
        TupleAndRelPos minTuple(0, tupleCmpBy);
        while(qp->Received(stream.addr)) // consume the stream completely
        {

          c++; // tuple counter;
          Tuple* t = static_cast<Tuple*>( wTuple.addr );
          TupleAndRelPos nextTuple(t, tupleCmpBy); 
          if( MAX_MEMORY > (size_t)t->GetSize() )
          {
            nextTuple.tuple->IncReference();
            currentRun->push(nextTuple);
            i++; // increment Tuples in memory counter
            MAX_MEMORY -= t->GetSize();
          }
          else 
          { // memory is completely used 
            if ( newRelation ) 
            { // create new relation
              r++;
              rel = new TupleBuffer( 0 );
              GenericRelationIterator *iter = 0;
              relations.push_back( make_pair( rel, iter ) );
              newRelation = false;
              
              // get first tuple and store it in an relation
              nextTuple.tuple->IncReference();
              currentRun->push(nextTuple);
              minTuple = currentRun->top();
              //minTuple.tuple->DecReference();
              rel->AppendTuple( minTuple.tuple );
              lastTuple = minTuple;
              currentRun->pop();              
            } 
            else 
            { // check if nextTuple can be saved in current relation
              TupleAndRelPos copyOfLast = lastTuple;
              if ( nextTuple < lastTuple ) 
              { // nextTuple is in order              
                // Push the next tuple int the heap and append the minimum to 
                // the current relation and push
                nextTuple.tuple->IncReference();
                currentRun->push(nextTuple);
                minTuple = currentRun->top();
                //minTuple.tuple->DecReference();
                rel->AppendTuple( minTuple.tuple );
                lastTuple = minTuple;
                currentRun->pop();
                m++;
              } 
              else 
              { // nextTuple is smaller, save it for the next relation
                nextTuple.tuple->IncReference();
                nextRun->push(nextTuple);
                n++;
                if ( !currentRun->empty() ) 
                {
                  // Append the minimum to the current relation    
                  minTuple = currentRun->top();
                  //minTuple.tuple->DecReference();
                  rel->AppendTuple( minTuple.tuple );
                  lastTuple = minTuple;
                  currentRun->pop();
                } 
                else 
                { //create a new run 
                  newRelation = true;
                  
                  // swap queues
                  TupleQueue *helpRun = currentRun;
                  currentRun = nextRun;
                  nextRun = helpRun;
                  ShowPartitionInfo(c,a,n,m,r,rel);
                  i=n;
                  a=0;
                  n=0;
                  m=0;
                } // end new run               
              } // end next tuple is smaller

              // delete last tuple if saved to relation and
              // not referenced by minTuple
              if ( copyOfLast.tuple && (copyOfLast.tuple != minTuple.tuple) ) 
              {
                copyOfLast.tuple->DeleteIfAllowed();
              }

            } // check if nextTuple can be saved in current relation
          }// memory is completely used
          
          qp->Request(stream.addr, wTuple);
        }
        ShowPartitionInfo(c,a,n,m,r,rel);

        // delete lastTuple and minTuple if allowed
        if ( lastTuple.tuple ) 
        {
          lastTuple.tuple->DeleteIfAllowed();
        }
        if ( (minTuple.tuple != lastTuple.tuple) ) 
        {
          minTuple.tuple->DeleteIfAllowed();
        }

        // the lastRun and NextRun runs in memory having 
        // less than MAX_TUPLE elements
        if( !queue[0].empty() ) 
        {
          Tuple* t = queue[0].top().tuple;
          queue[0].pop();
          mergeTuples.push( TupleAndRelPos(t, tupleCmpBy, -2) );
        } 
        if( !queue[1].empty() ) 
        {
          Tuple* t = queue[1].top().tuple;
          queue[1].pop();
          mergeTuples.push( TupleAndRelPos(t, tupleCmpBy, -1) );
        } 

        // Get next tuple from each relation and push it into the heap.
        for( size_t i = 0; i < relations.size(); i++ )
        {
          relations[i].second = relations[i].first->MakeScan();
          Tuple *t = relations[i].second->GetNextTuple();
          if( t != 0 ) 
          {
            t->IncReference();
            mergeTuples.push( TupleAndRelPos(t, tupleCmpBy, i+1) );
          }
        }
        Counter::getRef("Sortby:ExternPartitions") = relations.size(); 
      }

/*
It may happen, that the localinfo object will be destroyed
before all internal buffered tuples are delivered stream
upwards, e.g. queries which use a ~head~ operator.
In this case we need to delete also all tuples stored in memory.

*/

    ~SortByLocalInfo()
    {
      while( !mergeTuples.empty() ) 
      {
        mergeTuples.top().tuple->DeleteIfAllowed();
        mergeTuples.pop();
      }

      for( int i = 0; i < 2; i++ )
      {
        while( !queue[i].empty() )
        {
          queue[i].top().tuple->DeleteIfAllowed();
          queue[i].pop();
        }  
      }

      // delete information about sorted runs
      for( size_t i = 0; i < relations.size(); i++ )
      {
        delete relations[i].second;
        relations[i].second = 0;
        delete relations[i].first;
        relations[i].first = 0;
      }

      delete lexiTupleCmp;
      lexiTupleCmp = 0;
      delete tupleCmpBy;
      tupleCmpBy = 0;
    }

    Tuple *NextResultTuple()
    {
      if( mergeTuples.empty() ) // stream finished
        return 0;
      else
      {
        // Take the first one.
        TupleAndRelPos p = mergeTuples.top();
        //p.tuple->DecReference();
        mergeTuples.pop();
        Tuple *result = p.tuple;
        Tuple *t = 0;

        if (p.pos > 0) 
          t = relations[p.pos-1].second->GetNextTuple();
        else 
        {
          int idx = p.pos+2;
          if ( !queue[idx].empty() ) 
          {
            t = queue[idx].top().tuple;
            //t->DecReference();
            queue[idx].pop();
          } 
          else 
            t = 0;
        }

        if( t != 0 ) 
        { // run not finished
          p.tuple = t;
          t->IncReference();
          mergeTuples.push( p );
        }
        return result;
      }
    }

  private:

    void ShowPartitionInfo( int c, int a, int n, 
		            int m, int r, GenericRelation* rel ) 
    {
      int rs = (rel != 0) ? rel->GetNoTuples() : 0; 
      if ( RTFlag::isActive("ERA:Sort:PartitionInfo") ) 
      {
        cmsg.info() << "Current run finished: " 
		    << "  processed tuples=" << c 
                    << ", append minimum=" << m 
                    << ", append next=" << n << endl
                    << "  materialized runs=" << r
                    << ", last partition's tuples=" << rs << endl 
                    << "  Runs in memory: queue1= " << queue[0].size() 
                    << ", queue2= " << queue[1].size() << endl;
        cmsg.send();  
      }
    }

    Word stream;
    size_t currentIndex;

    // tuple information
    LexicographicalTupleCompare *lexiTupleCmp;
    TupleCompareBy *tupleCmpBy;
    bool lexicographic;

    // sorted runs created by in memory heap filtering 
    size_t MAX_MEMORY;
    typedef pair<TupleBuffer*, GenericRelationIterator*> SortedRun;
    vector< SortedRun > relations;

    typedef priority_queue<TupleAndRelPos> TupleQueue;
    TupleQueue queue[2];
    TupleQueue mergeTuples;
};

/*
2.1.1 Value mapping function of operator ~sortby~

The argument vector ~args~ contains in the first slot ~args[0]~ the stream and
in ~args[2]~ the number of sort attributes. ~args[3]~ contains the index of the
first sort attribute, ~args[4]~ a boolean indicating wether the stream should
be sorted in ascending order with regard to the sort first attribute. ~args[5]~
and ~args[6]~ contain these values for the second sort attribute and so on.

*/


template<bool lexicographically> int
sortby_vm(Word* args, Word& result, int message, Word& local, Supplier s)
{
  // args[0] : stream
  // args[1] : ignored 
  // args[2] : the number of sort attributes
  // args[3] : the index of the first sort attribute
  // args[4] : a boolean which indicates if sortorder should
  //           be asc or desc.
  // args[5] : Same as 3 but for the second sort attribute 
  // args[6] : Same as 4
  // ....
  //
  switch(message)
  {
    case OPEN:
    {
      qp->Open(args[0].addr);      
      void *tupleCmp = CreateCompareObject(lexicographically, args);
      SortByLocalInfo* li = new SortByLocalInfo( args[0], 
		                     lexicographically,  
                                     tupleCmp );
      local.addr = li;
      // at this point the local value is well defined
      // afterwards QueryProcessor request calls are
      // allowed.

      return 0;
    }
    case REQUEST:
    {
      SortByLocalInfo* sli = static_cast<SortByLocalInfo*>( local.addr );
      result.setAddr( sli->NextResultTuple() );
      return result.addr != 0 ? YIELD : CANCEL;
    }

    case CLOSE:
    {
      if( local.addr )
      {
	qp->Close(args[0].addr);      
        SortByLocalInfo *li = static_cast<SortByLocalInfo*>( local.addr );
        delete li;
	local.addr = 0;
      }
      return 0;
    }
  }
  return 0;
}

//-- end standard version --//

#else

//-- begin progress version --//

class SortByLocalInfo : protected ProgressWrapper
{
  public:
    SortByLocalInfo( Word stream, const bool lexicographic,
		     void *tupleCmp, ProgressLocalInfo* p            ):
      ProgressWrapper(p),
      stream( stream ),
      currentIndex( 0 ),
      lexiTupleCmp( lexicographic ?
                    (LexicographicalTupleSmaller*)tupleCmp :
                    0 ),
      tupleCmpBy( lexicographic ? 0 : (TupleCompareBy*)tupleCmp ),
      lexicographic( lexicographic )
      {
        // Note: It is not possible to define a Cmp object using the
        // constructor
        // mergeTuples( PairTupleCompareBy( tupleCmpBy )).
        // It does only work if mergeTuples is a local variable which
        // does not help us in this case. Is it a Compiler bug or C++ feature?
        // Hence a new class TupleAndRelPos was defined which implements
        // the comparison operator '<'.
        Heap* currentRun = &queue[0];
        Heap* nextRun = &queue[1];

        Word wTuple(Address(0));
        size_t  c = 0, i = 0, a = 0, n = 0, m = 0, r = 0; // counter variables
        bool newRelation = true;


        MAX_MEMORY = qp->MemoryAvailableForOperator();
        cmsg.info("ERA:ShowMemInfo")
          << "Sortby.MAX_MEMORY (" << MAX_MEMORY/1024 << " kb)" << endl;
        cmsg.send();

        TupleBuffer *rel=0;
        RTuple lastTuple;

        qp->Request(stream.addr, wTuple);
        RTuple minTuple;
        while(qp->Received(stream.addr)) // consume the stream completely
        {
          progress->read++;
          c++; // tuple counter;
          Tuple *t = static_cast<Tuple*>( wTuple.addr );
          TupleAndRelPos nextTuple(t, tupleCmpBy);
          if( MAX_MEMORY > (size_t)t->GetSize() )
          {
            //nextTuple.tuple->IncReference();
            currentRun->push(nextTuple);
            i++; // increment Tuples in memory counter
            MAX_MEMORY -= t->GetSize();
          }
          else
          { // memory is completely used
            progress->state = 1;
            if ( newRelation )
            { // create new relation
              r++;
              rel = new TupleBuffer( 0 );
              GenericRelationIterator *iter = 0;
              relations.push_back( make_pair( rel, iter ) );
              newRelation = false;

              // get first tuple and store it in an relation
              //nextTuple.ref.tuple->IncReference();
              currentRun->push(nextTuple);
              minTuple = currentRun->topTuple();
              rel->AppendTuple( minTuple.tuple );
              lastTuple = minTuple;
              minTuple.tuple->DeleteIfAllowed(); // remove from input stream
              currentRun->pop();
            }
            else
            { // check if nextTuple can be saved in current relation
              if ( nextTuple < TupleAndRelPos(lastTuple.tuple, tupleCmpBy) )
              { // nextTuple is in order
                // Push the next tuple int the heap and append the minimum to
                // the current relation and push
                //##nextTuple.ref.tuple->IncReference();
                currentRun->push(nextTuple);
                minTuple = currentRun->topTuple();
                rel->AppendTuple( minTuple.tuple );
                minTuple.tuple->DeleteIfAllowed();
                lastTuple = minTuple;
                currentRun->pop();
                m++;
              }
              else
              { // nextTuple is smaller, save it for the next relation
                //##nextTuple.ref.tuple->IncReference();
                nextRun->push(nextTuple);
                n++;
                if ( !currentRun->empty() )
                {
                  // Append the minimum to the current relation
                  minTuple = currentRun->topTuple();
                  rel->AppendTuple( minTuple.tuple );
                  minTuple.tuple->DeleteIfAllowed();
                  lastTuple = minTuple;
                  currentRun->pop();
                }
                else
                { //create a new run
                  newRelation = true;

                  // swap queues
                  TupleQueue *helpRun = currentRun;
                  currentRun = nextRun;
                  nextRun = helpRun;
                  ShowPartitionInfo(c,a,n,m,r,rel);
                  i=n;
                  a=0;
                  n=0;
                  m=0;
                } // end new run
              } // end next tuple is smaller

              // delete last tuple if saved to relation and
              // not referenced by minTuple
              /*if ( copyOfLast.tuple && (copyOfLast.tuple != minTuple.tuple) )
              {
                copyOfLast.tuple->DeleteIfAllowed();
              }*/

            } // check if nextTuple can be saved in current relation
          }// memory is completely used

          qp->Request(stream.addr, wTuple);
        }
        ShowPartitionInfo(c,a,n,m,r,rel);

        // delete lastTuple and minTuple if allowed
        /*##if ( lastTuple.tuple )
        {
          lastTuple.tuple->DeleteIfAllowed();
        }
        if ( (minTuple.tuple != lastTuple.tuple) )
        {
          minTuple.tuple->DeleteIfAllowed();
        }*/

        // the lastRun and NextRun runs in memory having
        // less than MAX_TUPLE elements
        if( !queue[0].empty() )
        {
          Tuple* t = queue[0].topTuple();
          queue[0].pop();
          mergeTuples.push( TupleAndRelPos(t, tupleCmpBy, -2) );
        }
        if( !queue[1].empty() )
        {
          Tuple* t = queue[1].topTuple();
          queue[1].pop();
          mergeTuples.push( TupleAndRelPos(t, tupleCmpBy, -1) );
        }

        // Get next tuple from each relation and push it into the heap.
        for( size_t i = 0; i < relations.size(); i++ )
        {
          relations[i].second = relations[i].first->MakeScan();
          Tuple *t = relations[i].second->GetNextTuple();
          if( t != 0 )
          {
            //t->IncReference();
            mergeTuples.push( TupleAndRelPos(t, tupleCmpBy, i+1) );
          }
        }
        Counter::getRef("Sortby:ExternPartitions") = relations.size();
      }

/*
It may happen, that the localinfo object will be destroyed
before all internal buffered tuples are delivered stream
upwards, e.g. queries which use a ~head~ operator.
In this case we need to delete also all tuples stored in memory.

*/

    ~SortByLocalInfo()
    {
      while( !mergeTuples.empty() )
      {
        mergeTuples.topTuple()->DeleteIfAllowed();
        mergeTuples.pop();
      }

      for( int i = 0; i < 2; i++ )
      {
        while( !queue[i].empty() )
        {
          queue[i].topTuple()->DeleteIfAllowed();
          queue[i].pop();
        }
      }

      // delete information about sorted runs
      for( size_t i = 0; i < relations.size(); i++ )
      {
        delete relations[i].second;
        relations[i].second = 0;
        delete relations[i].first;
        relations[i].first = 0;
      }

      delete lexiTupleCmp;
      lexiTupleCmp = 0;
      delete tupleCmpBy;
      tupleCmpBy = 0;
    }

    Tuple *NextResultTuple()
    {
      if( mergeTuples.empty() ) // stream finished
        return 0;
      else
      {
        // Take the first one.
        TupleAndRelPos p = mergeTuples.top();
        //p.tuple->DecReference();
        mergeTuples.pop();
        Tuple* result = p.tuple();
        Tuple* t = 0;

        if (p.pos > 0)
          t = relations[p.pos-1].second->GetNextTuple();
        else
        {
          int idx = p.pos+2;
          if ( !queue[idx].empty() )
          {
            t = queue[idx].topTuple();
            //t->DecReference();
            queue[idx].pop();
          }
          else
            t = 0;
        }

        if( t != 0 )
        { // run not finished
          p.ref = t;
          //t->IncReference();
          mergeTuples.push( p );
        }
        return result;
      }
    }

  private:

    void ShowPartitionInfo( int c, int a, int n,
		            int m, int r, GenericRelation* rel )
    {
      int rs = (rel != 0) ? rel->GetNoTuples() : 0;
      if ( RTFlag::isActive("ERA:Sort:PartitionInfo") )
      {
        cmsg.info() << "Current run finished: "
		    << "  processed tuples=" << c
                    << ", append minimum=" << m
                    << ", append next=" << n << endl
                    << "  materialized runs=" << r
                    << ", last partition's tuples=" << rs << endl
                    << "  Runs in memory: queue1= " << queue[0].size()
                    << ", queue2= " << queue[1].size() << endl;
        cmsg.send();
      }
    }

    Word stream;
    size_t currentIndex;

    // tuple information
    LexicographicalTupleSmaller *lexiTupleCmp;
    TupleCompareBy *tupleCmpBy;
    bool lexicographic;

    // sorted runs created by in memory heap filtering
    size_t MAX_MEMORY;
    typedef pair<TupleBuffer*, GenericRelationIterator*> SortedRun;
    vector< SortedRun > relations;

    typedef TupleQueue Heap;
    Heap queue[2];
    Heap mergeTuples;



};

/*
2.1.1 Value mapping function of operator ~sortby~

The argument vector ~args~ contains in the first slot ~args[0]~ the stream and
in ~args[2]~ the number of sort attributes. ~args[3]~ contains the index of the
first sort attribute, ~args[4]~ a boolean indicating wether the stream should
be sorted in ascending order with regard to the sort first attribute. ~args[5]~
and ~args[6]~ contain these values for the second sort attribute and so on.

*/

template<bool lexicographically> int
sortby_vm(Word* args, Word& result, int message, Word& local, Supplier s)
{
  // args[0] : stream
  // args[1] : ignored
  // args[2] : the number of sort attributes
  // args[3] : the index of the first sort attribute
  // args[4] : a boolean which indicates if sortorder should
  //           be asc or desc.
  // args[5] : Same as 3 but for the second sort attribute
  // args[6] : Same as 4
  // ....
  //

  LocalInfo<SortByLocalInfo>* li;

  li = static_cast<LocalInfo<SortByLocalInfo>*>( local.addr );

  switch(message)
  {
    case OPEN:
    {
      if (li) {
	delete li;
      }

      li = new LocalInfo<SortByLocalInfo>();
      local.addr = li;

      // at this point the local value is well defined
      // afterwards progress request calls are
      // allowed.

      li->ptr = 0;

      qp->Open(args[0].addr);

      return 0;
    }

    case REQUEST:
    {
      if ( li->ptr == 0 )
      {
        void *tupleCmp = CreateCompareObject(lexicographically, args);

	//Sorting is done in the following constructor. It was moved from
	//OPEN to REQUEST to avoid long delays in the OPEN method, which are
	//a problem for progress estimation 
    
        li->ptr = new SortByLocalInfo( args[0],
		                     lexicographically,
                                     tupleCmp, li       );
      }

      SortByLocalInfo* sli = li->ptr;

      result.setAddr( sli->NextResultTuple() );
      li->returned++;
      return result.addr != 0 ? YIELD : CANCEL;
    }

    case CLOSE:
      qp->Close(args[0].addr);

      // Note: object deletion is handled in OPEN and CLOSEPROGRESS
      return 0;


    case CLOSEPROGRESS:
      if (li) {	      
        delete li;
	local.addr = 0;
      }	
      return 0;


    case REQUESTPROGRESS:

      ProgressInfo p1;
      ProgressInfo *pRes;
      const double uSortBy = 0.000396;   //millisecs per byte input and sort
      const double vSortBy = 0.000194;   //millisecs per byte output
      const double oSortBy = 0.00004;   //offset due to writing to disk
				    //not yet measurable
      pRes = (ProgressInfo*) result.addr;

      if( !li ) return CANCEL;
      else
      {
        if (qp->RequestProgress(args[0].addr, &p1))
        {
          pRes->Card = li->returned == 0 ? p1.Card : li->read;

          pRes->CopySizes(p1);

          pRes->Time =                       //li->state = 0 or 1
            p1.Time 
	    + pRes->Card * p1.Size * (uSortBy + oSortBy * li->state)
            + pRes->Card * p1.Size * vSortBy;

          pRes->Progress =
            (p1.Progress * p1.Time 
             + li->read * p1.Size * (uSortBy + oSortBy * li->state) 
             + li->returned * p1.Size * vSortBy)
            / pRes->Time;

	  pRes->BTime = p1.Time + pRes->Card * p1.Size *  
            (uSortBy + oSortBy * li->state);

	  pRes->BProgress = 
	    (p1.Progress * p1.Time
	    + li->read * p1.Size * (uSortBy + oSortBy * li->state))
	    / pRes->BTime;

          return YIELD;
        }
        else return CANCEL;
      }

  }
  return 0;
}

//-- end progress version --//

#endif



/*
2.2 Operator ~mergejoin~

This operator computes the equijoin of two streams. It uses a text book
algorithm as outlined in A. Silberschatz, H. F. Korth, S. Sudarshan,
McGraw-Hill, 3rd. Edition, 1997.

2.2.1 Auxiliary definitions for value mapping function of operator ~mergejoin~

*/

#ifndef USE_PROGRESS

//-- begin standard version --//

class MergeJoinLocalInfo
{
private:

  // buffer limits	
  size_t MAX_MEMORY;
  size_t MAX_TUPLES_IN_MEMORY;

  // buffer related members
  TupleBuffer *grpB;
  GenericRelationIterator *iter;

  // members needed for sorting the input streams
  LocalInfo<SortByLocalInfo>* liA;
  SortByLocalInfo* sliA;

  LocalInfo<SortByLocalInfo>* liB;
  SortByLocalInfo* sliB;

  Word streamA;
  Word streamB;

  // the current pair of tuples
  Word resultA;
  Word resultB;

  RTuple ptA;
  RTuple ptB;
  RTuple tmpB;

  // the last comparison result
  int cmp;

  // the indexes of the attributes which will
  // be merged and the result type
  int attrIndexA;
  int attrIndexB;

  TupleType *resultTupleType;

  // a flag which indicates if sorting is needed
  bool expectSorted;

  // switch trace messages on/off
  const bool traceFlag; 

  // a flag needed in function NextTuple which tells
  // if the merge with grpB has been finished
  bool continueMerge;

  template<bool BOTH_B>
  int CompareTuples(Tuple* t1, Tuple* t2)
  {

    Attribute* a = 0; 	  
    if (BOTH_B)    
      a = static_cast<Attribute*>( t1->GetAttribute(attrIndexB) );
    else
      a = static_cast<Attribute*>( t1->GetAttribute(attrIndexA) );

    Attribute* b = static_cast<Attribute*>( t2->GetAttribute(attrIndexB) );

    /* tuples with NULL-Values in the join attributes
       are never matched with other tuples. */
    if( !a->IsDefined() )
    {
      return -1;
    }
    if( !b->IsDefined() )
    {
      return 1;
    }

    int cmp = a->Compare(b);
    if (traceFlag) 
    { 
          cmsg.info() 
            << "CompareTuples:" << endl
	    << "  BOTH_B = " << BOTH_B << endl
            << "  tuple_1  = " << *t1 << endl
            << "  tuple_2  = " << *t2 << endl 
            << "  cmp(t1,t2) = " << cmp << endl; 
          cmsg.send(); 
    }
    return cmp;
  }

  inline int CompareTuplesB(Tuple* t1, Tuple* t2) 
  {
    return CompareTuples<true>(t1, t2);
  }

  inline int CompareTuples(Tuple* t1, Tuple* t2) 
  {
    return CompareTuples<false>(t1, t2);
  }

  inline Tuple* NextTuple(Word stream, SortByLocalInfo* sli)
  {
    bool yield = false;
    Word result( Address(0) );

    if(!expectSorted)
      return sli->NextResultTuple();

    qp->Request(stream.addr, result);
    yield = qp->Received(stream.addr);

    if(yield)
    {
      return static_cast<Tuple*>( result.addr );
    }
    else
    {
      result.addr = 0;	    
      return static_cast<Tuple*>( result.addr );
    }
  }

  inline Tuple* NextTupleA()
  {
    return NextTuple(streamA, sliA);
  }  

  inline Tuple* NextTupleB()
  {
    return NextTuple(streamB, sliB);
  }  


public:
  MergeJoinLocalInfo( Word _streamA, Word wAttrIndexA,
                      Word _streamB, Word wAttrIndexB, 
                      bool _expectSorted, Supplier s  ) :
    traceFlag( RTFlag::isActive("ERA:TraceMergeJoin") )
  {
    expectSorted = _expectSorted;
    streamA = _streamA;
    streamB = _streamB;
    attrIndexA = StdTypes::GetInt( wAttrIndexA ) - 1;
    attrIndexB = StdTypes::GetInt( wAttrIndexB ) - 1;
    MAX_MEMORY = 0;

    sliA = 0;
    sliB = 0; 

    if( !expectSorted )
    {
      // sort the input streams

      SortOrderSpecification specA;
      SortOrderSpecification specB;
	 
      specA.push_back( pair<int, bool>(attrIndexA + 1, true) ); 
      specB.push_back( pair<int, bool>(attrIndexB + 1, true) ); 


      void* tupleCmpA = new TupleCompareBy( specA );
      void* tupleCmpB = new TupleCompareBy( specB );

      sliA = new SortByLocalInfo( streamA, 
				  false,  
				  tupleCmpA );

      sliB = new SortByLocalInfo( streamB, 
				  false,  
				  tupleCmpB );

    }

    ListExpr resultType =
                SecondoSystem::GetCatalog()->NumericType( qp->GetType( s ) );
    resultTupleType = new TupleType( nl->Second( resultType ) );

    // read in the first tuple of both input streams
    ptA = RTuple( NextTupleA() );
    ptB = RTuple( NextTupleB() );

    // initialize the status for the result
    // set iteration   
    tmpB = 0;
    cmp = 0;
    continueMerge = false;

    MAX_MEMORY = qp->MemoryAvailableForOperator();
    grpB = new TupleBuffer( MAX_MEMORY );

    cmsg.info("ERA:ShowMemInfo")
      << "MergeJoin.MAX_MEMORY (" << MAX_MEMORY/1024 << " kb)" << endl;
    cmsg.send();

  }

  ~MergeJoinLocalInfo()
  {
    if( !expectSorted )
    {
      // delete the objects instantiated for sorting
      delete sliA;
      delete sliB;
    }

    delete grpB;
    resultTupleType->DeleteIfAllowed();
  }

  Tuple* NextResultTuple()
  {
    Tuple* resultTuple = 0;

    if ( !continueMerge && ptB == 0)
      return 0;	    

    while( ptA != 0 ) {
     
      if (!continueMerge && ptB != 0) {

      //save ptB in tmpB	      
      tmpB = ptB;

      grpB->AppendTuple(tmpB.tuple);

      // advance the tuple pointer
      ptB = RTuple( NextTupleB() );
      
      // collect a group of tuples from B which
      // have the same attribute value
      bool done = false;
      while ( !done && ptB != 0 ) {
      
        int cmp = CompareTuplesB( tmpB.tuple, ptB.tuple );
     
        if ( cmp == 0) 
	{
	  // append equal tuples to group	
          grpB->AppendTuple(ptB.tuple);

	  // release tuple of input B
          ptB = RTuple( NextTupleB() );
	}
        else
	{
	  done = true;	
	}	
      } // end collect group	        

      cmp = CompareTuples( ptA.tuple, tmpB.tuple );

      while ( ptA != 0 && cmp < 0 ) 
      {
        // skip tuples from A while they are smaller than the 
	// value of the tuples in grpB 	      
        
        ptA = RTuple( NextTupleA() );
	if (ptA != 0) {
          cmp = CompareTuples( ptA.tuple, tmpB.tuple );
	}  
      }	      

      }
      // continue or start a merge with grpB   

      while ( ptA != 0 && cmp == 0 )
      {
        // join ptA with grpB
         
	if (!continueMerge) 
	{      
          iter = grpB->MakeScan();
	  continueMerge = true;
	  resultTuple = NextConcat();
	  if (resultTuple)
            return resultTuple;		  
	}  
        else
        {		
          // continue merging, create the next result tuple
	  resultTuple = NextConcat();
	  if (resultTuple) {
            return resultTuple;
          }	    
	  else 
          {
	    // Iteration over the group finished.	  
            // Continue with the next tuple of argument A
	    continueMerge = false;
	    delete iter;
	    iter = 0;
	   
            ptA = RTuple( NextTupleA() );
	    if (ptA != 0) {
              cmp = CompareTuples( ptA.tuple, tmpB.tuple );
	    }  
          }		  
        }	  
      } 	      
      
      grpB->Clear();
      // tpA > tmpB 
      if ( ptB == 0 ) {
        // short exit
	return 0; 
      } 

    } // end of main loop	    

    return 0;  
  }

  inline Tuple* NextConcat() 
  {
    Tuple* t = iter->GetNextTuple();
    if( t != 0 ) {

     Tuple* result = new Tuple( resultTupleType );
     Concat( ptA.tuple, t, result );
     return result;  
    }
    return 0;
  }

};

/*
2.2.2 Value mapping function of operator ~mergejoin~

*/


//CPUTimeMeasurer mergeMeasurer;

template<bool expectSorted> int
mergejoin_vm(Word* args, Word& result, int message, Word& local, Supplier s)
{
  MergeJoinLocalInfo* localInfo;

  switch(message)
  {
    case OPEN:
      qp->Open(args[0].addr);
      qp->Open(args[1].addr);

      localInfo = new MergeJoinLocalInfo
        (args[0], args[4], args[1], args[5], expectSorted, s);
      local.setAddr(localInfo);
      return 0;

    case REQUEST:
      //mergeMeasurer.Enter();
      localInfo = (MergeJoinLocalInfo*)local.addr;
      result.setAddr(localInfo->NextResultTuple());
      //mergeMeasurer.Exit();
      return result.addr != 0 ? YIELD : CANCEL;

    case CLOSE:
      //mergeMeasurer.PrintCPUTimeAndReset("CPU Time for Merging Tuples : ");

      qp->Close(args[0].addr);
      qp->Close(args[1].addr);

      localInfo = (MergeJoinLocalInfo*)local.addr;
      delete localInfo;
      local.addr = 0;
      return 0;
  }
  return 0;
}

//-- end standard version --//

#else

//-- begin progress version --//


class MergeJoinLocalInfo: protected ProgressWrapper
{
private:

  // buffer limits	
  size_t MAX_MEMORY;
  size_t MAX_TUPLES_IN_MEMORY;

  // buffer related members
  TupleBuffer *grpB;
  GenericRelationIterator *iter;

  // members needed for sorting the input streams
  LocalInfo<SortByLocalInfo>* liA;
  SortByLocalInfo* sliA;

  LocalInfo<SortByLocalInfo>* liB;
  SortByLocalInfo* sliB;

  Word streamA;
  Word streamB;

  // the current pair of tuples
  Word resultA;
  Word resultB;

  RTuple ptA;
  RTuple ptB;
  RTuple tmpB;

  // the last comparison result
  int cmp;

  // the indexes of the attributes which will
  // be merged and the result type
  int attrIndexA;
  int attrIndexB;

  TupleType *resultTupleType;

  // a flag which indicates if sorting is needed
  bool expectSorted;

  // switch trace messages on/off
  const bool traceFlag; 

  // a flag needed in function NextTuple which tells
  // if the merge with grpB has been finished
  bool continueMerge;

  template<bool BOTH_B>
  int CompareTuples(Tuple* t1, Tuple* t2)
  {

    Attribute* a = 0; 	  
    if (BOTH_B)    
      a = static_cast<Attribute*>( t1->GetAttribute(attrIndexB) );
    else
      a = static_cast<Attribute*>( t1->GetAttribute(attrIndexA) );

    Attribute* b = static_cast<Attribute*>( t2->GetAttribute(attrIndexB) );

    /* tuples with NULL-Values in the join attributes
       are never matched with other tuples. */
    if( !a->IsDefined() )
    {
      return -1;
    }
    if( !b->IsDefined() )
    {
      return 1;
    }

    int cmp = a->Compare(b);
    if (traceFlag) 
    { 
          cmsg.info() 
            << "CompareTuples:" << endl
	    << "  BOTH_B = " << BOTH_B << endl
            << "  tuple_1  = " << *t1 << endl
            << "  tuple_2  = " << *t2 << endl 
            << "  cmp(t1,t2) = " << cmp << endl; 
          cmsg.send(); 
    }
    return cmp;
  }

  inline int CompareTuplesB(Tuple* t1, Tuple* t2) 
  {
    return CompareTuples<true>(t1, t2);
  }

  inline int CompareTuples(Tuple* t1, Tuple* t2) 
  {
    return CompareTuples<false>(t1, t2);
  }

  inline Tuple* NextTuple(Word stream, SortByLocalInfo* sli)
  {
    bool yield = false;
    Word result( Address(0) );

    if(!expectSorted)
      return sli->NextResultTuple();

    qp->Request(stream.addr, result);
    yield = qp->Received(stream.addr);

    if(yield)
    {
      return static_cast<Tuple*>( result.addr );
    }
    else
    {
      result.addr = 0;	    
      return static_cast<Tuple*>( result.addr );
    }
  }

  inline Tuple* NextTupleA()
  {
    progress->readFirst++;
    return NextTuple(streamA, sliA);
  }  

  inline Tuple* NextTupleB()
  {
    progress->readSecond++;
    return NextTuple(streamB, sliB);
  }  


public:
  MergeJoinLocalInfo( Word _streamA, Word wAttrIndexA,
                      Word _streamB, Word wAttrIndexB, 
                      bool _expectSorted, Supplier s,
                      ProgressLocalInfo* p ) :
    ProgressWrapper(p), 
    traceFlag( RTFlag::isActive("ERA:TraceMergeJoin") )
  {
    expectSorted = _expectSorted;
    streamA = _streamA;
    streamB = _streamB;
    attrIndexA = StdTypes::GetInt( wAttrIndexA ) - 1;
    attrIndexB = StdTypes::GetInt( wAttrIndexB ) - 1;
    MAX_MEMORY = 0;

    liA = 0;
    sliA = 0;

    liB = 0;
    sliB = 0; 

    if( !expectSorted )
    {
      // sort the input streams

      SortOrderSpecification specA;
      SortOrderSpecification specB;
	 
      specA.push_back( pair<int, bool>(attrIndexA + 1, true) ); 
      specB.push_back( pair<int, bool>(attrIndexB + 1, true) ); 


      void* tupleCmpA = new TupleCompareBy( specA );
      void* tupleCmpB = new TupleCompareBy( specB );

      liA = new LocalInfo<SortByLocalInfo>();
      progress->firstLocalInfo = liA;
      sliA = new SortByLocalInfo( streamA, 
				  false,  
				  tupleCmpA, liA );

      liB = new LocalInfo<SortByLocalInfo>();
      progress->secondLocalInfo = liB;
      sliB = new SortByLocalInfo( streamB, 
				  false,  
				  tupleCmpB, liB );

    }

    ListExpr resultType =
                SecondoSystem::GetCatalog()->NumericType( qp->GetType( s ) );
    resultTupleType = new TupleType( nl->Second( resultType ) );

    // read in the first tuple of both input streams
    ptA.setTuple( NextTupleA() );
    ptB.setTuple( NextTupleB() );

    // initialize the status for the result
    // set iteration   
    tmpB = 0;
    cmp = 0;
    continueMerge = false;

    MAX_MEMORY = qp->MemoryAvailableForOperator();
    grpB = new TupleBuffer( MAX_MEMORY );

    cmsg.info("ERA:ShowMemInfo")
      << "MergeJoin.MAX_MEMORY (" << MAX_MEMORY/1024 << " kb)" << endl;
    cmsg.send();

  }

  ~MergeJoinLocalInfo()
  {
    if( !expectSorted )
    {
      // delete the objects instantiated for sorting
      delete sliA;
      delete sliB;
      delete liA;
      delete liB;
    }

    delete grpB;
    resultTupleType->DeleteIfAllowed();
  }


  Tuple* NextResultTuple()
  {
    Tuple* resultTuple = 0;

    if ( !continueMerge && ptB == 0)
      return 0;	    

    while( ptA != 0 ) {
     
      if (!continueMerge && ptB != 0) {

      //save ptB in tmpB	      
      tmpB = ptB;

      grpB->AppendTuple(tmpB.tuple);

      // advance the tuple pointer
      ptB.setTuple( NextTupleB() );
      
      // collect a group of tuples from B which
      // have the same attribute value
      bool done = false;
      while ( !done && ptB != 0 ) {
      
        int cmp = CompareTuplesB( tmpB.tuple, ptB.tuple );
     
        if ( cmp == 0) 
	{
	  // append equal tuples to group	
          grpB->AppendTuple(ptB.tuple);

	  // release tuple of input B
          ptB.setTuple( NextTupleB() );
	}
        else
	{
	  done = true;	
	}	
      } // end collect group	        

      cmp = CompareTuples( ptA.tuple, tmpB.tuple );

      while ( ptA != 0 && cmp < 0 ) 
      {
        // skip tuples from A while they are smaller than the 
	// value of the tuples in grpB 	      
        
        ptA.setTuple( NextTupleA() );
	if (ptA != 0) {
          cmp = CompareTuples( ptA.tuple, tmpB.tuple );
	}  
      }	      

      }
      // continue or start a merge with grpB   

      while ( ptA != 0 && cmp == 0 )
      {
        // join ptA with grpB
         
	if (!continueMerge) 
	{      
          iter = grpB->MakeScan();
	  continueMerge = true;
	  resultTuple = NextConcat();
	  if (resultTuple)
            return resultTuple;		  
	}  
        else
        {		
          // continue merging, create the next result tuple
	  resultTuple = NextConcat();
	  if (resultTuple) {
            return resultTuple;
          }	    
	  else 
          {
	    // Iteration over the group finished.	  
            // Continue with the next tuple of argument A
	    continueMerge = false;
	    delete iter;
	    iter = 0;
	   
            ptA.setTuple( NextTupleA() );
	    if (ptA != 0) {
              cmp = CompareTuples( ptA.tuple, tmpB.tuple );
	    }  
          }		  
        }	  
      } 	      
      
      grpB->Clear();
      // tpA > tmpB 
      if ( ptB == 0 ) {
        // short exit
	return 0; 
      } 

    } // end of main loop	    

    return 0;  
  }


  inline Tuple* NextConcat() 
  {
    Tuple* t = iter->GetNextTuple();
    if( t != 0 ) {

     Tuple* result = new Tuple( resultTupleType );
     Concat( ptA.tuple, t, result );
     t->DeleteIfAllowed();

     return result;  
    }
    return 0;
  }

};

/*
2.2.2 Value mapping function of operator ~mergejoin~

*/


//CPUTimeMeasurer mergeMeasurer;

template<bool expectSorted> int
mergejoin_vm(Word* args, Word& result, int message, Word& local, Supplier s)
{
  typedef LocalInfo<MergeJoinLocalInfo> LocalType;
  LocalType* li = static_cast<LocalType*>( local.addr );

  switch(message)
  {
    case OPEN:

      if ( li ) {
        delete li;
      }
      li = new LocalType();
      local.addr = li;

      qp->Open(args[0].addr);
      qp->Open(args[1].addr);

      li->ptr = 0;

      return 0;

    case REQUEST: {
      //mergeMeasurer.Enter();

      if ( li->ptr == 0 )	//first request;
				//constructor put here to avoid delays in OPEN
				//which are a problem for progress estimation
      {
        li->ptr = new MergeJoinLocalInfo
          (args[0], args[4], args[1], args[5], expectSorted, s, li);
      }

      MergeJoinLocalInfo* mli = li->ptr;
      result.addr = mli->NextResultTuple();
      li->returned++;

      //mergeMeasurer.Exit();

      return result.addr != 0 ? YIELD : CANCEL;

    }

    case CLOSE:
      //mergeMeasurer.PrintCPUTimeAndReset("CPU Time for Merging Tuples : ");

      qp->Close(args[0].addr);
      qp->Close(args[1].addr);

      //nothing is deleted on close because the substructures are still 
      //needed for progress estimation. Instead, everything is deleted on 
      //(repeated) OPEN and on CLOSEPROGRESS

      return 0;

    case CLOSEPROGRESS:
      if (li) {
        delete li;
	local.addr = 0;
      }

      return 0;


    case REQUESTPROGRESS:
    {
      ProgressInfo p1, p2;
      ProgressInfo* pRes = static_cast<ProgressInfo*>( result.addr );

      const double uSortBy = 0.00043;   //millisecs per byte read in sort step

      const double uMergeJoin = 0.0008077;  //millisecs per tuple read
                                        //in merge step (merge)

      const double wMergeJoin = 0.0001738; //millisecs per byte read in 
                                          //merge step (sortmerge)

      const double xMergeJoin = 0.0012058; //millisecs per result tuple in 
                                          //merge step 

      const double yMergeJoin = 0.0001072; //millisecs per result attribute in 
                                          //merge step 

                                      //see file ConstantsSortmergejoin.txt


      LocalInfo<SortByLocalInfo>* liFirst;
      LocalInfo<SortByLocalInfo>* liSecond;

      if( !li ) return CANCEL;
      else
      {

        liFirst = static_cast<LocalInfo<SortByLocalInfo>*> 
	  (li->firstLocalInfo);
        liSecond = static_cast<LocalInfo<SortByLocalInfo>*> 
	  (li->secondLocalInfo);

        if (qp->RequestProgress(args[0].addr, &p1)
         && qp->RequestProgress(args[1].addr, &p2))
        {
	  li->SetJoinSizes(p1, p2);

	  pRes->CopySizes(li);
	  
	      double factor = (double) li->readFirst / p1.Card;
	  
	      if ( (qp->GetSelectivity(s) != 0.1) && 
		          (li->returned > enoughSuccessesJoin) )
		    pRes->Card = factor * ((double) li->returned) * p1.Card
			  / ((double) li->readFirst) +
			  (1.0 - factor) * p1.Card * p2.Card 
			    * qp->GetSelectivity(s);
		  else

          if ( li->returned > enoughSuccessesJoin ) 	// stable state 
          {
            pRes->Card = ((double) li->returned) * p1.Card
            /  ((double) li->readFirst);
          }
          else
          {
            pRes->Card = p1.Card * p2.Card * qp->GetSelectivity(s);
          }


          if ( expectSorted )   
          {
            pRes->Time = p1.Time + p2.Time +
              (p1.Card + p2.Card) * uMergeJoin +
              pRes->Card * (xMergeJoin + pRes->noAttrs * yMergeJoin);

            pRes->Progress =
              (p1.Progress * p1.Time + p2.Progress * p2.Time +
                (((double) li->readFirst) + ((double) li->readSecond)) 
                * uMergeJoin +
              ((double) li->returned) 
                * (xMergeJoin + pRes->noAttrs * yMergeJoin))
              / pRes->Time;

	    pRes->CopyBlocking(p1, p2);	   //non-blocking in this case
          }
          else
          {
            pRes->Time =
              p1.Time + 
	      p2.Time +
              p1.Card * p1.Size * uSortBy + 
              p2.Card * p2.Size * uSortBy +
              (p1.Card * p1.Size + p2.Card * p2.Size) * wMergeJoin +
              pRes->Card * (xMergeJoin + pRes->noAttrs * yMergeJoin);

            long readFirst = (liFirst ? liFirst->read : 0);
            long readSecond = (liSecond ? liSecond->read : 0);

            pRes->Progress =
              (p1.Progress * p1.Time + 
              p2.Progress * p2.Time +
              ((double) readFirst) * p1.Size * uSortBy + 
              ((double) readSecond) * p2.Size * uSortBy +
              (((double) li->readFirst) * p1.Size + 
               ((double) li->readSecond) * p2.Size) * wMergeJoin +
              ((double) li->returned) 
                * (xMergeJoin + pRes->noAttrs * yMergeJoin))
              / pRes->Time;

            pRes->BTime = p1.Time + p2.Time               
	      + p1.Card * p1.Size * uSortBy 
              + p2.Card * p2.Size * uSortBy;

	    pRes->BProgress = 
	      (p1.Progress * p1.Time + p2.Progress * p2.Time 
              + ((double) readFirst) * p1.Size * uSortBy
              + ((double) readSecond) * p2.Size * uSortBy)
	      / pRes->BTime;
          }
       
          return YIELD;
        }
        else return CANCEL;

      }
    }
  }
  return 0;
}

//-- end progress version --//

#endif



/*
2.3 Operator ~hashjoin~

This operator computes the equijoin two streams via a hash join.  The user can
specify the number of hash buckets.

The implementation loops for each tuple of the first argument over a (partial)
hash-table of the second argument. If the hash-table of the second argument
does not fit into memory it needs to materialize the second arguments and must
scan it several times.


2.3.1 Auxiliary definitions for value mapping function of operator ~hashjoin~

*/

#ifndef USE_PROGRESS

//-- begin standard version --//

class HashJoinLocalInfo
{
private:
  size_t nBuckets;

  int attrIndexA;
  int attrIndexB;

  Word streamA;
  Word streamB;
  bool streamAClosed;
  bool streamBClosed;

  Tuple *tupleA;
  TupleBuffer* relA;
  GenericRelationIterator* iterTuplesRelA;
  size_t relA_Mem;
  bool firstPassA;
  bool memInfoShown;
  bool showMemInfo;
  size_t hashA;

  vector< vector<Tuple*> > bucketsB;
  vector<Tuple*>::iterator iterTuplesBucketB;
  size_t bucketsB_Mem;
  bool remainTuplesB, bFitsInMemory;
  Word wTupleB;

  TupleType *resultTupleType;

  int CompareTuples(Tuple* a, Tuple* b)
  {
    /* tuples with NULL-Values in the join attributes
       are never matched with other tuples. */
    if(!((Attribute*)a->GetAttribute(attrIndexA))->IsDefined())
    {
      return -1;
    }
    if(!((Attribute*)b->GetAttribute(attrIndexB))->IsDefined())
    {
      return 1;
    }

    return ((Attribute*)a->GetAttribute(attrIndexA))->
      Compare((Attribute*)b->GetAttribute(attrIndexB));
  }

  size_t HashTuple(Tuple* tuple, int attrIndex)
  {
    return 
      (((StandardAttribute*)tuple->GetAttribute(attrIndex))->HashValue() % 
      nBuckets);
  }

  void ClearBucket( vector<Tuple*>& bucket )
  {
    vector<Tuple*>::iterator i = bucket.begin();
    while( i != bucket.end() )
    {
      (*i)->DeleteIfAllowed();
      i++;
    }
    bucket.clear();
  }

  void ClearBucketsB()
  {
    vector< vector<Tuple*> >::iterator iterBuckets = bucketsB.begin();

    while(iterBuckets != bucketsB.end() )
    {
      ClearBucket( *iterBuckets );
      iterBuckets++;
    }
  }

  bool FillHashBucketsB()
  {
    bucketsB_Mem = (3 * qp->MemoryAvailableForOperator())/4;
    if( firstPassA )
    {
      qp->Request(streamB.addr, wTupleB);
      if(qp->Received(streamB.addr))
      {
        // reserve 3/4 of memory for buffering tuples of B;
        // Before retrieving the allowed memory size from the
        // configuration file it was set to 12MB for B and 4MB for A (see below)
        relA_Mem = qp->MemoryAvailableForOperator()/4;

	if (showMemInfo) {
        cmsg.info()
          << "HashJoin.MAX_MEMORY ("
          << qp->MemoryAvailableForOperator()/1024
          << " kb - A: " << relA_Mem/1024 << "kb B: "
          << bucketsB_Mem/1024 << "kb)" << endl
          << "Stream A is stored in a Tuple Buffer" << endl;
        cmsg.send();
	}
      }
    }

    size_t b = 0, i = 0;
    while(qp->Received(streamB.addr) )
    {
      Tuple* tupleB = (Tuple*)wTupleB.addr;
      b += tupleB->GetExtSize();
      i++;
      if( b > bucketsB_Mem )
      {
        if (showMemInfo) {
        cmsg.info()
          << "HashJoin - Stream B does not fit in memory" << endl
          << "Memory used up to now: " << b / 1024 << "kb" << endl
          << "Tuples in memory: " << i << endl;
        cmsg.send();
	}

        break;
      }

      size_t hashB = HashTuple(tupleB, attrIndexB);
      tupleB->IncReference();
      bucketsB[hashB].push_back( tupleB );
      qp->Request(streamB.addr, wTupleB);
    }

    bool remainTuples = false;
    if( b > bucketsB_Mem && qp->Received(streamB.addr) )
      remainTuples = true;

    if( !remainTuples )
    {
      qp->Close(streamB.addr);
      streamBClosed = true;
    }

    return remainTuples;
  }

public:
  static const size_t MIN_BUCKETS = 3;
  static const size_t DEFAULT_BUCKETS = 97;

  HashJoinLocalInfo(Word streamA, Word attrIndexAWord,
    Word streamB, Word attrIndexBWord, Word nBucketsWord,
    Supplier s)
  {
    memInfoShown = false;
    showMemInfo = RTFlag::isActive("ERA:ShowMemInfo");
    this->streamA = streamA;
    this->streamB = streamB;

    ListExpr resultType = 
      SecondoSystem::GetCatalog()->NumericType( qp->GetType( s ) );
    resultTupleType = new TupleType( nl->Second( resultType ) );

    attrIndexA = StdTypes::GetInt( attrIndexAWord ) - 1;
    attrIndexB = StdTypes::GetInt( attrIndexBWord ) - 1;
    nBuckets = StdTypes::GetInt( nBucketsWord );
    if(nBuckets > qp->MemoryAvailableForOperator() / 1024)
      nBuckets = qp->MemoryAvailableForOperator() / 1024;
    if(nBuckets < MIN_BUCKETS)
      nBuckets = MIN_BUCKETS;

    bucketsB.resize(nBuckets);
    relA = 0;
    iterTuplesRelA = 0;
    firstPassA = true;
    tupleA = 0;

    qp->Open(streamB.addr);
    streamBClosed = false;
    remainTuplesB = FillHashBucketsB();
    bFitsInMemory  = !remainTuplesB;

    if( !bFitsInMemory ) 
      // reserve 1/4 of the allowed memory for buffering tuples of A
      relA = new TupleBuffer( relA_Mem );

    qp->Open(streamA.addr);
    streamAClosed = false;
    NextTupleA();
/*
At this moment we have a tuple of the stream A and a hash table in memory
of the stream B. There is a possibility that the stream B does not fit in
memory, which is kept in the variable ~bFitsInMemory~. The iterator for the
bucket that the tuple coming from A hashes is also initialized.

*/
  }

  ~HashJoinLocalInfo()
  {
    ClearBucketsB();

    // delete tuple buffer and its iterator if necessary
    if( !bFitsInMemory )
    {
      if ( iterTuplesRelA )
        delete iterTuplesRelA;
      relA->Clear();
      delete relA;
    }

    // close open streams if necessary
    if ( !streamAClosed )
      qp->Close(streamA.addr);
    if ( !streamBClosed )
      qp->Close(streamB.addr);

    resultTupleType->DeleteIfAllowed();
  }

  bool NextTupleA()
  {
    if( tupleA != 0 )
    {
      if( firstPassA && !bFitsInMemory ) {
        relA->AppendTuple( tupleA );
      }	
      tupleA->DeleteIfAllowed();
    }

    if( firstPassA )
    {
      Word wTupleA;
      qp->Request( streamA.addr, wTupleA );
      if( qp->Received(streamA.addr) )
      {
        tupleA = (Tuple*)wTupleA.addr;
        if (!memInfoShown && showMemInfo) 
        {
          cmsg.info() 
            << "TupleBuffer for relA can hold " 
            << relA_Mem / tupleA->GetExtSize() << " tuples" << endl;
          cmsg.send();
          memInfoShown = true;
        }
      }
      else
      {
        tupleA = 0;
        qp->Close(streamA.addr);
        streamAClosed = true;
        return false;
      }
    }
    else
    {
      if( (tupleA = iterTuplesRelA->GetNextTuple()) == 0 )
      {
        delete iterTuplesRelA;
        iterTuplesRelA = 0;
        return false;
      }
    }

    hashA = HashTuple( tupleA, attrIndexA );
    iterTuplesBucketB = bucketsB[hashA].begin();
    return true;
  }

  Tuple* NextResultTuple()
  {
    while( tupleA != 0 )
    {
      while( iterTuplesBucketB != bucketsB[hashA].end() )
      {
        Tuple *tupleB = *iterTuplesBucketB++;

        if( CompareTuples( tupleA, tupleB ) == 0 )
        {
          Tuple *result = new Tuple( resultTupleType );
          Concat( tupleA, tupleB, result );
          return result;
        }
      }

      if( !NextTupleA() )
      {
        if( remainTuplesB )
        {
          firstPassA = false;
          ClearBucketsB();
          remainTuplesB = FillHashBucketsB();
          iterTuplesRelA = relA->MakeScan();
          NextTupleA();
        }
      }
    }

    return 0;
  }
};

/*
2.3.2 Value Mapping Function of Operator ~hashjoin~

*/
int HashJoin(Word* args, Word& result, int message, Word& local, Supplier s)
{
  HashJoinLocalInfo* localInfo;

  switch(message)
  {
    case OPEN:
      localInfo = 
        new HashJoinLocalInfo(args[0], args[5], args[1], args[6], args[4], s);
      local.setAddr(localInfo);
      return 0;
    case REQUEST:
      localInfo = (HashJoinLocalInfo*)local.addr;
      result.setAddr(localInfo->NextResultTuple());
      return result.addr != 0 ? YIELD : CANCEL;
    case CLOSE:
      localInfo = (HashJoinLocalInfo*)local.addr;
      delete localInfo;
      local.addr = 0;
      return 0;
  }
  return 0;
}


//-- end standard version --//

#else

//-- begin progress version --//


class HashJoinLocalInfo : protected ProgressWrapper
{
private:
  size_t nBuckets;

  int attrIndexA;
  int attrIndexB;

  Word streamA;
  Word streamB;
  bool streamAClosed;
  bool streamBClosed;

  Tuple *tupleA;
  TupleBuffer* relA;
  GenericRelationIterator* iterTuplesRelA;
  size_t relA_Mem;
  bool firstPassA;
  bool memInfoShown;
  bool showMemInfo;
  size_t hashA;

  vector< vector<Tuple*> > bucketsB;
  vector<Tuple*>::iterator iterTuplesBucketB;
  size_t bucketsB_Mem;
  bool remainTuplesB, bFitsInMemory;
  Word wTupleB;

  TupleType *resultTupleType;

  int CompareTuples(Tuple* a, Tuple* b)
  {
    /* tuples with NULL-Values in the join attributes
       are never matched with other tuples. */
    if(!((Attribute*)a->GetAttribute(attrIndexA))->IsDefined())
    {
      return -1;
    }
    if(!((Attribute*)b->GetAttribute(attrIndexB))->IsDefined())
    {
      return 1;
    }

    return ((Attribute*)a->GetAttribute(attrIndexA))->
      Compare((Attribute*)b->GetAttribute(attrIndexB));
  }

  size_t HashTuple(Tuple* tuple, int attrIndex)
  {
    return
      (((StandardAttribute*)tuple->GetAttribute(attrIndex))->HashValue() %
      nBuckets);
  }

  void ClearBucket( vector<Tuple*>& bucket )
  {
    vector<Tuple*>::iterator i = bucket.begin();
    while( i != bucket.end() )
    {
      (*i)->DeleteIfAllowed();
      i++;
    }
    bucket.clear();
  }

  void ClearBucketsB()
  {
    vector< vector<Tuple*> >::iterator iterBuckets = bucketsB.begin();

    while(iterBuckets != bucketsB.end() )
    {
      ClearBucket( *iterBuckets );
      iterBuckets++;
    }
  }

  bool FillHashBucketsB()
  {
    bucketsB_Mem = (3 * qp->MemoryAvailableForOperator())/4;
    if( firstPassA )
    {
      qp->Request(streamB.addr, wTupleB);
      if(qp->Received(streamB.addr))
      {
        // reserve 3/4 of memory for buffering tuples of B;
        // Before retrieving the allowed memory size from the
        // configuration file it was set to 12MB for B and 4MB for A (see below)
        relA_Mem = qp->MemoryAvailableForOperator()/4;

	progress->memoryFirst = relA_Mem;
	progress->memorySecond = bucketsB_Mem;

	if (showMemInfo) {
        cmsg.info()
          << "HashJoin.MAX_MEMORY ("
          << qp->MemoryAvailableForOperator()/1024
          << " kb - A: " << relA_Mem/1024 << "kb B: "
          << bucketsB_Mem/1024 << "kb)" << endl
          << "Stream A is stored in a Tuple Buffer" << endl;
        cmsg.send();
	}
      }
    }

    size_t b = 0, i = 0;
    while(qp->Received(streamB.addr) )
    {
      progress->readSecond++;
      Tuple* tupleB = (Tuple*)wTupleB.addr;
      b += tupleB->GetExtSize();
      i++;
      if( b > bucketsB_Mem )
      {
        if (showMemInfo) {
        cmsg.info()
          << "HashJoin - Stream B does not fit in memory" << endl
          << "Memory used up to now: " << b / 1024 << "kb" << endl
          << "Tuples in memory: " << i << endl;
        cmsg.send();
	}

        break;
      }

      size_t hashB = HashTuple(tupleB, attrIndexB);
      //tupleB->IncReference();
      bucketsB[hashB].push_back( tupleB );
      qp->Request(streamB.addr, wTupleB);
    }

    bool remainTuples = false;
    if( b > bucketsB_Mem && qp->Received(streamB.addr) )
      remainTuples = true;

    if( !remainTuples )
    {
      qp->Close(streamB.addr);
      streamBClosed = true;
    }
    else progress->state = 1;

    return remainTuples;
  }

public:
  static const size_t MIN_BUCKETS = 3;
  static const size_t DEFAULT_BUCKETS = 97;

  HashJoinLocalInfo(Word streamA, Word attrIndexAWord,
    Word streamB, Word attrIndexBWord, Word nBucketsWord,
    Supplier s, ProgressLocalInfo* p) : ProgressWrapper(p)
  {
    memInfoShown = false;
    showMemInfo = RTFlag::isActive("ERA:ShowMemInfo");
    this->streamA = streamA;
    this->streamB = streamB;

    ListExpr resultType =
      SecondoSystem::GetCatalog()->NumericType( qp->GetType( s ) );
    resultTupleType = new TupleType( nl->Second( resultType ) );

    attrIndexA = StdTypes::GetInt( attrIndexAWord ) - 1;
    attrIndexB = StdTypes::GetInt( attrIndexBWord ) - 1;
    nBuckets = StdTypes::GetInt( nBucketsWord );
    if(nBuckets > qp->MemoryAvailableForOperator() / 1024)
      nBuckets = qp->MemoryAvailableForOperator() / 1024;
    if(nBuckets < MIN_BUCKETS)
      nBuckets = MIN_BUCKETS;

    bucketsB.resize(nBuckets);
    relA = 0;
    iterTuplesRelA = 0;
    firstPassA = true;
    tupleA = 0;


    streamBClosed = false;
    remainTuplesB = FillHashBucketsB();
    bFitsInMemory  = !remainTuplesB;

    if( !bFitsInMemory )
      // reserve 1/4 of the allowed memory for buffering tuples of A
      relA = new TupleBuffer( relA_Mem );

    streamAClosed = false;
    NextTupleA();
/*
At this moment we have a tuple of the stream A and a hash table in memory
of the stream B. There is a possibility that the stream B does not fit in
memory, which is kept in the variable ~bFitsInMemory~. The iterator for the
bucket that the tuple coming from A hashes is also initialized.

*/
  }

  ~HashJoinLocalInfo()
  {
    ClearBucketsB();

    // delete tuple buffer and its iterator if necessary
    if( !bFitsInMemory )
    {
      if ( iterTuplesRelA )
        delete iterTuplesRelA;
      relA->Clear();
      delete relA;
    }

    // close open streams if necessary
    Close();

    resultTupleType->DeleteIfAllowed();
  }

  void Close(){
    if ( !streamAClosed )
      qp->Close(streamA.addr);
    if ( !streamBClosed )
      qp->Close(streamB.addr);
  }


  bool NextTupleA()
  {
    if( tupleA != 0 )
    {
      if( firstPassA && !bFitsInMemory ) {
        relA->AppendTuple( tupleA );
      }
      tupleA->DeleteIfAllowed();
    }

    if( firstPassA )
    {
      Word wTupleA;
      qp->Request( streamA.addr, wTupleA );
      if( qp->Received(streamA.addr) )
      {
        progress->readFirst++;
        tupleA = (Tuple*)wTupleA.addr;
        if (!memInfoShown && showMemInfo)
        {
          cmsg.info()
            << "TupleBuffer for relA can hold "
            << relA_Mem / tupleA->GetExtSize() << " tuples" << endl;
          cmsg.send();
          memInfoShown = true;
        }
      }
      else
      {
        tupleA = 0;
        qp->Close(streamA.addr);
        streamAClosed = true;
        return false;
      }
    }
    else
    {
      if( (tupleA = iterTuplesRelA->GetNextTuple()) == 0 )
      {
        delete iterTuplesRelA;
        iterTuplesRelA = 0;
        return false;
      }
      else progress->readFirst++;
    }

    hashA = HashTuple( tupleA, attrIndexA );
    iterTuplesBucketB = bucketsB[hashA].begin();
    return true;
  }

  Tuple* NextResultTuple()
  {
    while( tupleA != 0 )
    {
      while( iterTuplesBucketB != bucketsB[hashA].end() )
      {
        Tuple *tupleB = *iterTuplesBucketB++;

        if( CompareTuples( tupleA, tupleB ) == 0 )
        {
          Tuple *result = new Tuple( resultTupleType );
          Concat( tupleA, tupleB, result );
          return result;
        }
      }

      if( !NextTupleA() )
      {
        if( remainTuplesB )
        {
          firstPassA = false;
	  if (showMemInfo) {
            cmsg.info() 
	      << "Create a hash table for the remaining tuples of B " << endl
              << "and initialize new scan over A." << endl;
	    cmsg.send();
          }		  
          ClearBucketsB();
          remainTuplesB = FillHashBucketsB();
          iterTuplesRelA = relA->MakeScan();
          NextTupleA();
        }
      }
    }

    return 0;
  }
};

/*
2.3.2 Value Mapping Function of Operator ~hashjoin~

*/

double minimum(double a, double b) {return (a < b ? a : b);}

int HashJoin(Word* args, Word& result, int message, Word& local, Supplier s)
{

  typedef LocalInfo<HashJoinLocalInfo>  LocalType;
  LocalType* li;
  HashJoinLocalInfo* hli;

  li = static_cast<LocalType*>( local.addr );

  switch(message)
  {
    case OPEN:

      if ( li ) { 
	delete li;
      }	

      li = new LocalType();
      li->memorySecond = 12582912;	 //default, reset by constructor below
      local.addr = li;

      li->ptr = 0;

      qp->Open(args[0].addr);
      qp->Open(args[1].addr);

      return 0;

    case REQUEST:

      if ( li->ptr == 0 )	//first request;
				//constructor moved here to avoid delays in OPEN
				//which are a problem for progress estimation
      {
        li->ptr = new HashJoinLocalInfo(args[0], args[5], args[1],
                                      args[6], args[4], s, li);
      }

      hli = li->ptr;
      result.setAddr( hli->NextResultTuple() );
      li->returned++;

      return result.addr != 0 ? YIELD : CANCEL;

    case CLOSE:
      if(li){
        if(li->ptr){
           li->ptr->Close();
        }
      }
      // Note: object deletion is handled in OPEN and CLOSEPROGRESS
      return 0;


    case CLOSEPROGRESS:

      if (li) {
        delete li;
        local.addr = 0;
      }	

      return 0;
    

    case REQUESTPROGRESS:

    {
      bool trace = false;

      ProgressInfo p1, p2;
      ProgressInfo* pRes = static_cast<ProgressInfo*>( result.addr );
      const double uHashJoin = 0.023;  //millisecs per probe tuple
      const double vHashJoin = 0.0067;  //millisecs per tuple right
      const double wHashJoin = 0.0025;  //millisecs per tuple returned

      if( !li ) {
         //cerr << "CANCEL PROGRESS (1)" << endl; 		  
         return CANCEL;
      }	      
      else
      {
       if (qp->RequestProgress(args[0].addr, &p1)
         && qp->RequestProgress(args[1].addr, &p2))
        {
	  li->SetJoinSizes(p1, p2);

	  double defaultSelectivity = 
	    minimum( 1 / p1.Card, 1 / p2.Card);

	  int noPasses = 1 + (int) (p2.Card * p2.SizeExt) / li->memorySecond;

	  double firstBuffer = 
	    minimum(((double) li->memorySecond / p2.SizeExt), p2.Card);

          if ( li->returned > enoughSuccessesJoin ) // stable state  
          {
            pRes->Card = p1.Card * noPasses *
              ((double) li->returned / (double) li->readFirst);
          }
          else
          {
            pRes->Card = p1.Card * p2.Card * 
              (qp->GetSelectivity(s) == 0.1 ? defaultSelectivity : 
	        qp->GetSelectivity(s));
          }

	  pRes->CopySizes(li);

          pRes->Time = p1.Time + p2.Time
	    + p2.Card * vHashJoin	      //reading into hashtable
	    + p1.Card * noPasses * uHashJoin  //probing
            + pRes->Card * wHashJoin;	      //output tuples

	  pRes->Progress = (p1.Progress * p1.Time + p2.Progress * p2.Time
	    + li->readSecond * vHashJoin
	    + li->readFirst * uHashJoin
	    + minimum(li->returned, pRes->Card) * wHashJoin)
            / pRes->Time;

	  pRes->BTime = 
	    p1.BTime 
	    + p2.Time * (firstBuffer / p2.Card)
	    + firstBuffer * vHashJoin;

	  pRes->BProgress = 
	    (p1.BProgress * p1.BTime 
            + p2.Time * minimum((double) li->readSecond, firstBuffer) / p2.Card 
	    + minimum((double) li->readSecond, firstBuffer) * vHashJoin)
            / pRes->BTime;

	      if ( trace ) {
		cout << "Number of passes: " << noPasses << endl;
		cout << "No first buffer = " << firstBuffer << endl;
		cout << "li->readFirst = " << li->readFirst << endl;
		cout << "li->readSecond = " << li->readSecond << endl;
		cout << "li->returned = " << li->returned << endl;
		cout << "li->state = " << li->state << endl;
		cout << "li->memorySecond = " << li->memorySecond << endl;
	      }

          //cerr << "YIELD PROGRESS" << endl; 		  
          return YIELD;
        }
        else
        {
          //cerr << "CANCEL PROGRESS (2)" << endl; 		  
          return CANCEL;
        }
      }
    }

  }
  return 0;
}

//-- end progress version --//

#endif


/*
3 Instantiation of Template Functions

For some reasons the compiler cannot expand these template functions in
the file ~ExtRelationAlgebra.cpp~, thus the value mapping functions
are instatiated here.

*/

template int
sortby_vm<false>(Word* args, Word& result, int message, 
              Word& local, Supplier s);

template int
sortby_vm<true>(Word* args, Word& result, int message, 
             Word& local, Supplier s);

template int
mergejoin_vm<true>(Word* args, Word& result, int message, 
                Word& local, Supplier s);

template int
mergejoin_vm<false>(Word* args, Word& result, int message, 
                 Word& local, Supplier s);

