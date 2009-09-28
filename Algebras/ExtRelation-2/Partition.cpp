/*
----
This file is part of SECONDO.

Copyright (C) 2009, University in Hagen, Faculty of Mathematics and
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

1 Implementation File Partition.cpp

June 2009, Sven Jungnickel. Initial version

2 Includes and defines

*/

#include "Partition.h"

/*
3 Implementation of class ~PartitionProgressInfo~

*/
namespace extrel2
{

/*
4 Implementation of class ~PartitionHistogram~

*/

PartitionHistogram::PartitionHistogram(PInterval& i)
: interval(i)
, data(i.GetLength())
, tuples(0)
, totalSize(0)
, totalExtSize(0)
{
}

PartitionHistogram::PartitionHistogram( PartitionHistogram& obj,
                                        size_t start, size_t end )
: interval( obj.GetInterval().GetLow() + start,
            obj.GetInterval().GetLow() + end )
, data(interval.GetLength())
{
  assert( (end - start) < obj.GetInterval().GetLength() );

  for ( size_t i = 0, j = start;
        j <= end;
        i++, j++ )
  {
    this->data[i] = obj.data[j];
    tuples += obj.data[j].count;
    totalSize += obj.data[j].totalSize;
    totalExtSize += obj.data[j].totalExtSize;
  }
}

void PartitionHistogram::Insert(Tuple* t, size_t hashFuncValue)
{
  assert(interval.IsAt(hashFuncValue));

  int hIndex = hashFuncValue - interval.GetLow();

  data[hIndex].value = hashFuncValue;
  data[hIndex].count++;
  data[hIndex].totalSize += t->GetSize();
  data[hIndex].totalExtSize += t->GetExtSize();

  return;
}

PartitionHistogramEntry& PartitionHistogram::GetHistogramEntry(size_t n)
{
  assert(n < data.size());
  return data[n];
}

/*
4 Implementation of class ~Partition~

*/

Partition::Partition(PInterval i, size_t bufferSize)
: interval(i)
, histogram(i)
, subpartitioned(false)
{
  buffer = new TupleBuffer(bufferSize);
}

Partition::~Partition()
{
  if ( buffer )
  {
    delete buffer;
    buffer = 0;
  }
}

PartitionIterator* Partition::MakeScan()
{
  return new PartitionIterator(*this);
}

void Partition::Insert(Tuple* t, size_t hashFuncValue)
{
  buffer->AppendTuple(t);
  histogram.Insert(t, hashFuncValue);
}

ostream& Partition::Print(ostream& os)
{
    os << "[" << this->interval.GetLow() << ", "
       << this->interval.GetHigh() << "] -> "
       << this->interval.GetLength() << " bucket numbers, "
       << this->GetNoTuples() << " tuples, "
       << this->GetTotalSize() << " bytes (Size), "
       << this->GetTotalExtSize() << " bytes (ExtSize)"
       << endl;

  return os;
}

/*
5 Implementation of class ~PartitionManager~

*/
PartitionManager::PartitionManager( HashFunction* h,
                                    size_t nBuckets,
                                    size_t nPartitions,
                                    size_t p0,
                                    PartitionManagerProgressInfo* pInfo )
: iter(0)
, hashFunc(h)
, p0(p0)
, tuples(0)
, subpartitioned(false)
, progressInfo(pInfo)
{
  // calculate buckets per partition
  size_t step = nBuckets / nPartitions;
  size_t rest = nBuckets % nPartitions;
  size_t low = 0;

  // create partitions
  for(size_t i = 0; i < nPartitions; i++)
  {
    PInterval interval;

    if ( rest != 0 && i >= ( nPartitions - rest ))
    {
      interval = PInterval(low, low + step);
      low += step + 1;
    }
    else
    {
      interval = PInterval(low, low + (step-1));
      low += step;
    }

    size_t bufferSize = 0;

    // set buffer size of partition 0
    if ( i == 0 && p0 != UINT_MAX )
    {
      bufferSize = p0;
    }

    partitions.push_back( new Partition(interval, bufferSize) );

    if ( progressInfo != 0 )
    {
      progressInfo->partitionProgressInfo.push_back(PartitionProgressInfo());
    }
  }
}

PartitionManager::PartitionManager( HashFunction* h,
                                    PartitionManager& pm,
                                    PartitionManagerProgressInfo* pInfo )
: iter(0)
, hashFunc(h)
, p0(0)
, subpartitioned(false)
, progressInfo(pInfo)
{
  // create partitions with intervals from pm
  for(size_t i = 0; i < pm.partitions.size(); i++)
  {
    partitions.push_back( new Partition(pm.partitions[i]->GetInterval(), 0) );

    if ( progressInfo != 0 )
    {
      progressInfo->partitionProgressInfo.push_back(PartitionProgressInfo());
    }
  }
}

PartitionManager::~PartitionManager()
{
  for(size_t i = 0; i < partitions.size(); i++)
  {
    delete partitions[i];
  }

  partitions.clear();

  if ( iter != NULL )
  {
    delete iter;
  }

  if ( hashFunc != NULL )
  {
    delete hashFunc;
  }

  progressInfo = 0;
}

void PartitionManager::Insert(Tuple* t)
{
  // calculate bucket number
  size_t b = hashFunc->Value(t);

  // find partition index
  size_t p = findPartition(b);

  // insert tuple into partition
  partitions[p]->Insert(t,b);

  tuples++;

  // update progress info if necessary
  if ( progressInfo != 0 )
  {
    progressInfo->partitionProgressInfo[p].tuples++;
    progressInfo->partitionProgressInfo[p].noOfPasses =
      (int)ceil( (double)partitions[p]->GetTotalExtSize()
          / (double)qp->MemoryAvailableForOperator() );

    if ( subpartitioned = false && ( tuples % SUBPARTITION_UPDATE ) == 0 )
    {
      calcSubpartitionTupleCount( qp->MemoryAvailableForOperator(),
                                  SUBPARTITION_MAX_LEVEL );
    }
  }
}

size_t PartitionManager::PartitionStream(Word stream)
{
  Tuple* t;
  size_t b, last, read = 0;
  size_t p = UINT_MAX;

  while ( ( t = readFromStream(stream) ) )
  {
    read++;

    // calculate bucket number
    b = hashFunc->Value(t);

    // determine partition if necessary
    if ( last != b || p == UINT_MAX )
    {
      p = findPartition(b);
    }

    // insert tuple into partition
    partitions[p]->Insert(t,b);

    if ( progressInfo != 0 )
    {
      progressInfo->partitionProgressInfo[p].tuples++;
      progressInfo->partitionProgressInfo[p].noOfPasses =
        (int)ceil( (double)partitions[p]->GetTotalExtSize()
            / (double)qp->MemoryAvailableForOperator() );
    }

    // save last bucket number
    last = b;
  }

  return read;
}

void PartitionManager::Subpartition()
{
  // Subpartition if necessary
  for(size_t i = 0; i < partitions.size(); i++)
  {
    subpartition( i, qp->MemoryAvailableForOperator(),
                  SUBPARTITION_MAX_LEVEL, 1);
  }

  // Sort partitions array
  PartitionCompareLesser cmp;
  sort(partitions.begin(), partitions.end(), cmp);

  subpartitioned = true;
}

void PartitionManager::subpartition( size_t n,
                                     size_t maxSize,
                                     int maxRecursion,
                                     int level )
{
  // check partition size
  if ( partitions[n]->GetTotalExtSize() <= maxSize )
  {
    partitions[n]->SetSubpartitioned();
    return;
  }

  // check if maximum recursion level is reached
  if ( level > maxRecursion )
  {
    partitions[n]->SetSubpartitioned();
    return;
  }

  // check if partition contains at least 4 buckets
  if ( partitions[n]->GetInterval().GetLength() < 4 )
  {
    partitions[n]->SetSubpartitioned();
    return;
  }

  // create two new partitions with half the interval size
  size_t low = partitions[n]->GetInterval().GetLow();
  size_t high = partitions[n]->GetInterval().GetHigh();
  size_t m = low + partitions[n]->GetInterval().GetLength() / 2 - 1;

  PInterval i1 = PInterval(low, m);
  PInterval i2 = PInterval(m+1, high);

  Partition* s1 = new Partition(i1, ( n == 0 && p0 > 0 ) ? p0 : 0 );
  Partition* s2 = new Partition(i2, 0);

  if ( traceMode )
  {
    cmsg.info() << "Subpartition of partition " << n << endl;
    cmsg.info() << n << ": ";
    partitions[n]->Print(cmsg.info());
    cmsg.info() << "New partitions " << n << endl;
    cmsg.info() << "s1: ";
    s1->Print(cmsg.info());
    cmsg.info() << "s2: ";
    s2->Print(cmsg.info());
    cmsg.send();
  }

  // scan partition and put tuples in s1 or s2
  PartitionIterator* iter = partitions[n]->MakeScan();

  Tuple* t;
  size_t counter = 0;
  while( ( t = iter->GetNextTuple() ) != 0 )
  {
    size_t h = hashFunc->Value(t);

    if ( i1.IsAt(h) )
    {
      s1->Insert(t,h);
    }
    else
    {
      s2->Insert(t,h);
    }

    // update progress information if necessary
    if ( progressInfo != 0 )
    {
      progressInfo->subTuples++;

      // propagate progress message if necessary
      if ( ( counter++ % 200 ) == 0)
      {
        qp->CheckProgress();
      }
    }
  }

  // delete empty partitions, store new ones and subpartition
  // if necessary (Note: only one partition can be empty)
  if ( s1->GetNoTuples() == 0 && s2->GetNoTuples() > 0 )
  {
    delete s1;
    delete partitions[n];
    partitions[n] = s2;

    if ( progressInfo != 0 )
    {
      progressInfo->partitionProgressInfo[n].tuples = s2->GetNoTuples();
      progressInfo->partitionProgressInfo[n].noOfPasses =
        (int)ceil( (double)s2->GetTotalExtSize()
            / (double)qp->MemoryAvailableForOperator() );
    }

    subpartition(n, maxSize, maxRecursion, ++level);
  }
  else if ( s1->GetNoTuples() > 0 && s2->GetNoTuples() == 0 )
  {
    delete s2;
    delete partitions[n];
    partitions[n] = s1;

    if ( progressInfo != 0 )
    {
      progressInfo->partitionProgressInfo[n].tuples = s1->GetNoTuples();
      progressInfo->partitionProgressInfo[n].noOfPasses =
        (int)ceil( (double)s1->GetTotalExtSize()
            / (double)qp->MemoryAvailableForOperator() );
    }

    subpartition(n, maxSize, maxRecursion, ++level);
  }
  else
  {
    delete partitions[n];
    partitions[n] = s1;
    partitions.push_back(s2);
    size_t level1 = level;
    size_t level2 = level;
    size_t m = partitions.size()-1;

    if ( progressInfo != 0 )
    {
      progressInfo->partitionProgressInfo[n].tuples = s1->GetNoTuples();
      progressInfo->partitionProgressInfo[n].noOfPasses =
        (int)ceil( (double)s1->GetTotalExtSize()
            / (double)qp->MemoryAvailableForOperator() );

      progressInfo->partitionProgressInfo[m].tuples = s2->GetNoTuples();
      progressInfo->partitionProgressInfo[m].noOfPasses =
        (int)ceil( (double)s2->GetTotalExtSize()
            / (double)qp->MemoryAvailableForOperator() );
    }

    subpartition(n, maxSize, maxRecursion, ++level1);
    subpartition(m, maxSize, maxRecursion, ++level2);
  }
}

int PartitionManager::calcSubpartitionTupleCount( size_t maxSize,
                                                  int maxRecursion )
{
  int count = 0;

  // Simulate sub-partitioning
  for(size_t i = 0; i < partitions.size(); i++)
  {
    count += simsubpartition( partitions[i]->GetPartitionHistogram(),
                              maxSize, maxRecursion, 1);
  }

  return count;
}

int PartitionManager::simsubpartition( PartitionHistogram& ph,
                                       size_t maxSize,
                                       int maxRecursion,
                                       int level )
{
  int tuples = 0;

  if ( ph.GetTotalExtSize() <= maxSize )
  {
    return tuples;
  }

  // check if maximum recursion level is reached
  if ( level > maxRecursion )
  {
    return tuples;
  }

  // check if partition contains at least 4 hash function values
  if ( ph.GetInterval().GetLength() < 4 )
  {
    return tuples;
  }

  size_t m = ( ph.GetSize() / 2 ) - 1;

  // create two new partition histograms with half the size
  PartitionHistogram ph1(ph, 0, m);
  PartitionHistogram ph2(ph, m+1, ph.GetSize() - 1);

  tuples = ph.GetNoTuples();

  int noTuples1 = ph1.GetNoTuples();
  int noTuples2 = ph2.GetNoTuples();

  // delete empty partitions, store new ones and subpartition
  // if necessary (Note: only one partition can be empty)
  if ( noTuples1 == 0 && noTuples2 > 0 )
  {
    tuples += simsubpartition(ph2, maxSize, maxRecursion, ++level);
  }
  else if ( noTuples1 > 0 && noTuples2 == 0 )
  {
    tuples += simsubpartition(ph1, maxSize, maxRecursion, ++level);
  }
  else
  {
    size_t level1 = level;
    size_t level2 = level;
    tuples += simsubpartition(ph1, maxSize, maxRecursion, ++level1);
    tuples += simsubpartition(ph2, maxSize, maxRecursion, ++level2);
  }

  return tuples;
}

void PartitionManager::InitPartitions(HashTable* h)
{
  size_t p = 0;

  for(size_t i = 0; i < h->GetNoBuckets(); i++)
  {
    if ( !partitions[p]->GetInterval().IsAt(i) )
    {
      p++;
    }

    vector<Tuple*> arr = h->GetTuples(i);

    for(size_t j = 0; j < arr.size(); j++)
    {
      partitions[p]->Insert(arr[j],i);

      if ( progressInfo != 0 )
      {
        progressInfo->partitionProgressInfo[p].tuplesProc++;
      }
    }
  }
}

bool PartitionManager::LoadPartition( int n,
                                      HashTable* h,
                                      size_t maxMemory )
{
  assert(h);
  assert(n < (int)partitions.size());

  Tuple* t;

  // Clear hash table
  h->Clear();

  if ( iter == 0 )
  {
    // start new partition scan
    iter = partitions[n]->MakeScan();
  }

  while( ( t = iter->GetNextTuple() ) != 0 )
  {
    h->Insert(t);
    maxMemory -= t->GetExtSize();

    if ( progressInfo != 0 )
    {
      progressInfo->partitionProgressInfo[n].tuplesProc++;
    }

    if (maxMemory <= 0 )
    {
      // memory is filled but partition is not finished
      return false;
    }
  }

  delete iter;
  iter = 0;

  // partition is finished and fits into memory
  return true;
}

ostream& PartitionManager::Print(ostream& os)
{
  os << "-------------------- Partitioning -----------------------" << endl;

  for(size_t i = 0; i < partitions.size(); i++)
  {
    os << "Partition: " << i ;
    partitions[i]->Print(os);
  }

  return os;
}

} // end of namespace extrel2
