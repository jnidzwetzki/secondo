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

//paragraph [1] Title: [{\Large \bf \begin {center}] [\end {center}}]
//[TOC] [\tableofcontents]
//[_] [\_]

[1] Header File of the OSM Algebra

June-November, 2011. Thomas Uchdorf

[TOC]

1 Overview

This header file essentially contains the definition of the classes ~OsmAlgebra~.

2 Defines and includes

*/
// [...]
#ifndef __OSM_ALGEBRA_H__
#define __OSM_ALGEBRA_H__

// --- Including header-files
#include "Algebra.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "RelationAlgebra.h"

class FullOsmImport {
  public:
     FullOsmImport(const string& fileName, const string& prefix);
    ~FullOsmImport();
    
    bool initRelations(const string& prefix);
    bool openFile(const string&fileName);
    void defineRelations();
    void fillRelations();
    void storeRelations();
    
    SecondoCatalog* sc;
    bool isTemp;
    bool relationsInitialized;
    bool fileOk;
    xmlDocPtr doc;    
    xmlNodePtr cur, curChild;
    Relation *nodeRel, *nodeTagRel, *wayRel, *wayTagRel, *restRel,
        *restTagRel;
    TupleType *nodeType, *nodeTagType, *wayType, *wayTagType,
        *restType, *restTagType;
    ListExpr nodeTypeInfo, nodeTagTypeInfo, wayTypeInfo, wayTagTypeInfo,
        restTypeInfo, restTagTypeInfo, numNodeTypeInfo, numNodeTagTypeInfo, 
        numWayTypeInfo, numWayTagTypeInfo, numRestTypeInfo, numRestTagTypeInfo;
    string relNames[6];
    int tupleCount[6];
    string relKinds[6];
};


namespace osm {

// OSM-algebra
class OsmAlgebra : public Algebra
{
    public:
        // --- Constructors
        // Constructor
        OsmAlgebra();
        // Destructor
        ~OsmAlgebra ();
};

} // end of namespace osm

#endif /* __OSM_ALGEBRA_H__ */

