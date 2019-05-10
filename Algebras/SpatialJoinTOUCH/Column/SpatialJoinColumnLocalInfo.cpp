/*
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

*/

#include "Algebras/CRel/TBlock.h"
#include "Algebras/CRel/TypeConstructors/TBlockTC.h"
#include "Algebras/CRel/SpatialAttrArray.h"

#include "RTreeTouchCol.h"
#include "SpatialJoinColumnLocalInfo.h"
#include "Algebras/Relation-C++/RelationAlgebra.h"
#include "BinaryTuple.h"

class NestedList;
class QueryProcessor;
class nodeCol;

namespace CRelAlgebra {
    class TBlockEntry;
    class TBlock;
    class TBlockIterator;
}

extern NestedList* nl;
extern QueryProcessor *qp;

using namespace std;
using namespace mmrtreetouch;
using namespace CRelAlgebra;


namespace STRColumn {
    vector<mmrtreetouch::nodeCol *> createBuckets(
            vector<binaryTuple *> tuples,
            int firstStreamWordIndex,
            int _numOfPartitions
    );
}


SpatialJoinColumnLocalInfo::~SpatialJoinColumnLocalInfo() {
    tt->DeleteIfAllowed();

    delete rtree;

    for (const TBlock* tb: fTBlockVector) {
        //delete tb;
        if(tb) {
            tb->DecRef();
        }
    }
    fTBlockVector.clear();

    for (const TBlock* tb: sTBlockVector) {
        //delete tb;
        if(tb) {
            tb->DecRef();
        }
    }
    sTBlockVector.clear();

    for (binaryTuple * bt :bts) {
        delete bt;
    }
    bts.clear();

    for (binaryTuple * bt :btsB) {
        delete bt;
    }
    btsB.clear();


    delete[] tuple;
};


SpatialJoinColumnLocalInfo::SpatialJoinColumnLocalInfo(
        Word firstStreamWordParam,
        Word secondStreamWordParam,
        Word _fanout,
        Word _numOfPartitions,
        Word cellFactorWord,
        int firstStreamWordIndex,
        int secondStreamWordIndex,
        ListExpr ttl,
        Supplier s
):rTBlockTypeInfo(CRelAlgebra::TBlockTI(qp->GetType(s), false)),
  rTBlockInfo(rTBlockTypeInfo.GetBlockInfo()),
  rTBlockSize(rTBlockTypeInfo.GetDesiredBlockSize()
              * CRelAlgebra::TBlockTI::blockSizeFactor),
  tuple(0)
{
    tt = new ::TupleType(ttl);

    firstStreamWord = firstStreamWordParam;
    secondStreamWord = secondStreamWordParam;

    int fanout = ((CcInt*)_fanout.addr)->GetIntval();
    int numOfPartitions = ((CcInt*)_numOfPartitions.addr)->GetIntval();
    int cellFactor = ((CcInt*)cellFactorWord.addr)->GetIntval();

    _firstStreamIndex = firstStreamWordIndex;
    _secondStreamIndex = secondStreamWordIndex;

    getAllTuplesFromStream(firstStreamWord, fTBlockVector);
    getAllTuplesFromStream(secondStreamWord, sTBlockVector);

    bts = createBinaryTuplesVector(fTBlockVector, _firstStreamIndex);
    btsB = createBinaryTuplesVector(sTBlockVector, _secondStreamIndex);

    vector<mmrtreetouch::nodeCol * > buckets =
            STRColumn::createBuckets(bts, _firstStreamIndex, numOfPartitions);

    rtree = new RTreeTouchCol(
            tt,
            _firstStreamIndex,
            _secondStreamIndex,
            cellFactor
    );

    rtree->constructTree(buckets, fanout);

    assignTuplesB(btsB);

    findMatchings();
}

void SpatialJoinColumnLocalInfo::assignTuplesB(vector<binaryTuple *> BBTs)
{
    for (binaryTuple * tupleB: BBTs) {
        rtree->assignTupleBs(tupleB);
    }
}

void SpatialJoinColumnLocalInfo::findMatchings()
{
    vector<pair<binaryTuple, binaryTuple >> matchings = rtree->findMatchings();

    const size_t fNumColumns = fTBlockVector[0]->GetColumnCount();
    const size_t sNumColumns = sTBlockVector[0]->GetColumnCount();

    tuple = new AttrArrayEntry[fNumColumns + sNumColumns];

    tempTBlock = new TBlock(rTBlockInfo, 0, 0);

    for (pair<binaryTuple, binaryTuple> btPair: matchings) {

        addbinaryTupleToTBlock(
                btPair,
                tuple,
                fNumColumns,
                sNumColumns);
    }

    if (tempTBlock->GetRowCount() > 0) {
        matchingVector.push_back(tempTBlock);
        tempTBlock = 0;
    }

}

TBlock* SpatialJoinColumnLocalInfo::NextResultTBlock () {
    if (!matchingVector.empty()) {
        TBlock *tBlockToReturn = matchingVector.back();
        matchingVector.pop_back();

        return tBlockToReturn;
    } else {
        return 0;
    }
}



// TODO: use second stream
void SpatialJoinColumnLocalInfo::addbinaryTupleToTBlock(
        pair<binaryTuple, binaryTuple> btPair,
        AttrArrayEntry* tuple,
        const size_t fNumColumns,
        const size_t sNumColumns
        )
{
    binaryTuple fBT = btPair.first;
    binaryTuple sBT = btPair.second;

    const CRelAlgebra::TBlockEntry &fTuple = CRelAlgebra::TBlockEntry(
            fTBlockVector[fBT.blockNum],
            fBT.row);

    const CRelAlgebra::TBlockEntry &sTuple = CRelAlgebra::TBlockEntry(
            sTBlockVector[sBT.blockNum],
            sBT.row
    );

    for(uint64_t i = 0; i < fNumColumns; i++) {
        tuple[i] = fTuple[i];
    }

    for(uint64_t i = 0; i < sNumColumns; i++) {
        tuple[fNumColumns + i] = sTuple[i];
    }

    tempTBlock->Append(tuple);

    if(tempTBlock->GetSize() >= rTBlockSize) { // if tempTBlock is full
        matchingVector.push_back(tempTBlock);
        tempTBlock = new TBlock(rTBlockInfo, 0, 0);
    }
}

void SpatialJoinColumnLocalInfo::getAllTuplesFromStream(
        const Word stream, vector<const TBlock*> &tBlockVector)
{
    Word streamTBlockWord;

    qp->Open (stream.addr);
    qp->Request (stream.addr, streamTBlockWord);

    while (qp->Received (stream.addr) )
    {
        TBlock* tBlock = (TBlock*) streamTBlockWord.addr;

        tBlockVector.push_back(tBlock);

        qp->Request ( stream.addr, streamTBlockWord);
    }

    qp->Close (stream.addr);
}

vector<binaryTuple *> SpatialJoinColumnLocalInfo::createBinaryTuplesVector(
        vector<const TBlock*> &TBlockVector,
        const uint64_t &joinIndex
        ) {

    vector<binaryTuple* > BT;
    uint64_t tBlockNum = 0;
    binaryTuple * temp;

    uint64_t tblockVectorSize = TBlockVector.size();
    while(tBlockNum < tblockVectorSize) {

        CRelAlgebra::TBlockIterator tBlockIter =
                TBlockVector[tBlockNum]->GetIterator();
        uint64_t row = 0;

        while(tBlockIter.IsValid()) {

            const CRelAlgebra::TBlockEntry &tuple = tBlockIter.Get();
            temp = new binaryTuple();
            temp->blockNum = tBlockNum;
            temp->row = row;

            CRelAlgebra::SpatialAttrArrayEntry<2> attribute = tuple[joinIndex];
            Rectangle<2> rec = attribute.GetBoundingBox();
            temp->xMin = rec.MinD(0);
            temp->xMax = rec.MaxD(0);
            temp->yMin = rec.MinD(1);
            temp->yMax = rec.MaxD(1);

            BT.push_back(temp);
            ++row;
            tBlockIter.MoveToNext();
        }
        ++tBlockNum;
    }

    return BT;
}