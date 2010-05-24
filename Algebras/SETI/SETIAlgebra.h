/******************************************************************************
----
This file is part of SECONDO.

Copyright (C) 2004-2010, University in Hagen, Faculty of Mathematics and
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

//paragraph [1] Title: [{\Large \bf] [}]
//paragraph [10] Footnote: [{\footnote{] [}}]
//[TOC] [\tableofcontents]

[1] Declaration of SETIAlgebra

May 2010, Daniel Brockmann

1 Overview

SETI-Algebra implements a SETI index structure. The implementation makes
use of the existing SECONDO component RTree-Algebra. The memory management
is based on SmiUpdateFile. In addition the UploadUnit is an object type of
this Algebra.

SETI-Algebra offers the following methods:

- createSETI        -> Creates a new SETI object.
- insertUpload      -> Inserts a single upload into SETI.
- insertStream      -> Inserts a stream of uploads into SETI.
- intersectsWindow  -> Returns all trajectory segments which
                       intersect the search window.
- insideWindow      -> Returns all trajectory segments
                       inside the search window.
- getTrajectory     -> Returns all trajectory segments wich belongs
                       to the stated moving object.
- currentUpload     -> Returns the current upload.

******************************************************************************/

#ifndef __SETI_ALGEBRA_H__
#define __SETI_ALGEBRA_H__

/******************************************************************************

2 Globals constants and variables

******************************************************************************/

const int     pageSize    = 4096;    // Alternatively: WinUnix::getPageSize()
const int     maxSplits   = 64;      // Number of max splits for one dim
const int     flBuckets   = 100;     // Number of hash buckets in frontline
const int     updateCycle = 10000;   // Update cycle for SmiUpdateFile
const int     maxTrjSeg   = 70;      // Max number of trj segments in page
const double  tol         = 0.00001; // Tolerance for floating points
bool         intersects[4];          // Indicates an intersection of cell ...
                                     // ... border and history unit

/******************************************************************************

3.1 Definition of SETIArea

The SETIArea structure defines the boundary of a SETI grid or a search window.

******************************************************************************/

struct SETIArea
{
  SETIArea(){}
  SETIArea( double X1, double Y1, double X2, double Y2 ):
            x1( X1 ), y1( Y1 ), x2( X2 ), y2( Y2 ){}
  double x1;  // x1 coordinate
  double y1;  // y1 coordinate
  double x2;  // x2 coordinate
  double y2;  // y2 coordinate
};

/******************************************************************************

3.2 Definition of TrjSeg

TrjSeg holds the information of a trajectory segment.

******************************************************************************/

struct TrjSeg
{
  TrjSeg( int MOID, int SEGID, double TIVSTART, double TIVEND,
          UnitPos POS1,  UnitPos POS2 )
  {
    moID      = MOID;
    segID     = SEGID;
    tivStart  = TIVSTART;
    tivEnd    = TIVEND;
    pos1      = POS1;
    pos2      = POS2;
  }
  int moID;         // Moving object id
  int segID;        // Segment id
  double tivStart;  // Start time of segment
  double tivEnd;    // End time of segment
  UnitPos pos1;     // Start position of segment
  UnitPos pos2;     // End position of segment
};

/******************************************************************************

3.3 Definition of SETICell

The SETICell structure contains all information  of a cell in a SETI grid.

******************************************************************************/

struct SETICell
{
  int         numEntries;       // Number of segments in cell
  SETIArea    area;             // Cell area (partition)
  SmiFileId   rtreeFileID;      // RTree file id
  db_pgno_t   currentPage;      // Number of current cell page
  R_Tree<2,TupleId>* rtreePtr;  // RTree pointer
  Interval<Instant> tiv;        // Cell time interval
};

/******************************************************************************

3.4 Definition of SETIHeader

The SETIHeader is used to store the most important SETI data.

******************************************************************************/

struct SETIHeader
{
  SETIHeader()
  { 
    fileID       = (SmiFileId)0;
    headerPageNo = (db_pgno_t)0;
    flPageNo     = (db_pgno_t)0;
    cellPageNo   = (db_pgno_t)0;
    area         = SETIArea(0,0,0,0);
    splits       = 0;
    numCells     = 0;
    numEntries   = 0;
    numFlEntries = 0;
    tiv.start    = DateTime(0,0,instanttype);
    tiv.end      = DateTime(0,0,instanttype);
    tiv.lc       = false;
    tiv.rc       = false;
    rtree0FileID = (SmiFileId)0;
    rtree0Ptr    = (R_Tree<2,TupleId>*)0;
    rtree1FileID = (SmiFileId)0;
    rtree1Ptr    = (R_Tree<2,TupleId>*)0;
    rtree2FileID = (SmiFileId)0;
    rtree2Ptr    = (R_Tree<2,TupleId>*)0;
    rtree3FileID = (SmiFileId)0;
    rtree3Ptr    = (R_Tree<2,TupleId>*)0;
  }
  
  SETIHeader(SETIArea AREA, int SPLITS)
  { 
    fileID       = (SmiFileId)0;
    headerPageNo = (db_pgno_t)0;
    flPageNo     = (db_pgno_t)0;
    cellPageNo   = (db_pgno_t)0;
    area         = AREA;
    splits       = SPLITS;
    numCells     = SPLITS*SPLITS;
    numEntries   = 0;
    numFlEntries = 0;
    tiv.start    = DateTime(0,0,instanttype);
    tiv.end      = DateTime(0,0,instanttype);
    tiv.lc       = false;
    tiv.rc       = false;
    rtree0FileID = (SmiFileId)0;
    rtree0Ptr    = (R_Tree<2,TupleId>*)0;
    rtree1FileID = (SmiFileId)0;
    rtree1Ptr    = (R_Tree<2,TupleId>*)0;
    rtree2FileID = (SmiFileId)0;
    rtree2Ptr    = (R_Tree<2,TupleId>*)0;
    rtree3FileID = (SmiFileId)0;
    rtree3Ptr    = (R_Tree<2,TupleId>*)0;
  }
  
  SmiFileId          fileID;       // SETI file id
  db_pgno_t          headerPageNo; // Header page number
  db_pgno_t          flPageNo;     // Front-line page number
  db_pgno_t          cellPageNo;   // Number of first cell page
  SETIArea           area;         // SETI area
  int                splits;       // Number of SETI partitions for one dim
  int                numCells;     // Number of cells
  int                numEntries;   // Number of TrjSegments/UploadUnits
  int                numFlEntries; // Number of front-line entries
  Interval<Instant>  tiv;          // SETI time interval
  SmiFileId          rtree0FileID; // RTree0 file id
  R_Tree<2,TupleId>* rtree0Ptr;    // RTree0 pointer
  SmiFileId          rtree1FileID; // RTree1 file id
  R_Tree<2,TupleId>* rtree1Ptr;    // RTree1 pointer
  SmiFileId          rtree2FileID; // RTree2 file id
  R_Tree<2,TupleId>* rtree2Ptr;    // RTree2 pointer
  SmiFileId          rtree3FileID; // RTree3 file id
  R_Tree<2,TupleId>* rtree3Ptr;    // RTree3 pointer
};

/******************************************************************************

4 Declaration of class SETI

This class defines a SETI index which consists of a
- header
- frontline
- grid apportioned by cells (including RTrees)
- SmiUpdateFile to store all information in a shared memory

******************************************************************************/

class SETI
{
  public:
    // Basic constructor
    SETI(SETIArea AREA, int SPLITS);
    // Query constructor
    SETI(SmiFileId FILEID);
    // Destructor
    ~SETI();
    
    // The mandatory set of algebra support functions
    static Word    In( const ListExpr typeInfo, const ListExpr instance,
                      const int errorPos, ListExpr& errorInfo, bool& correct );
    static ListExpr Out( ListExpr typeInfo, Word value );
    static Word     Create( const ListExpr typeInfo );
    static void     Delete( const ListExpr typeInfo, Word& w );
    static bool     Open( SmiRecord& valueRecord, size_t& offset,
                          const ListExpr typeInfo, Word& value );
    static bool     Save( SmiRecord& valueRecord, size_t& offset, 
                          const ListExpr typeInfo, Word& w );
    static void     Close( const ListExpr typeInfo, Word& w );
    static Word     Clone( const ListExpr typeInfo, const Word& w );
    static bool     KindCheck( ListExpr type, ListExpr& errorInfo );
    static void*    Cast(void* addr);
    static int      SizeOfObj();  
    static ListExpr Property();
    
    // Writes header information into file
    void UpdateHeader();
    // Writes front-line information into file
    void UpdateFLine();
    // Writes cell information into file
    void UpdateCells();
    // Writes all SETI data into file
    bool UpdateSETI();
    // Returns the SETI file id
    SmiUpdateFile* GetUpdateFile();
    // Returns the pointer to the SETI header
    SETIHeader* GetHeader();
    // Returns the pointer to the stated cell
    SETICell* GetCell(int COL, int ROW);
    // Returns the state of the semaphore
    bool GetSemaphore();
    // Sets the state of the semaphore
    void SetSemaphore(bool VALUE);
    
    // SETI frontline hash
    map<int,UploadUnit> frontline[flBuckets];
    
  private:
    // Reads SETI header, frontline and cell information
    void ReadSETI();
    
    SmiUpdateFile*  suf;                         // SmiUpdateFile
    SETIHeader*     header;                      // SETI header
    SETICell*       cells[maxSplits][maxSplits]; // SETI cells
};

#endif
