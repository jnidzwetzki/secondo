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

[1] Header File of the OsmReader

June-November, 2011. Thomas Uchdorf

[TOC]

1 Overview

This header file essentially contains the definition of the class
~OsmReader~.

2 Defines and includes

*/
#ifndef __OSM_READER_H__
#define __OSM_READER_H__

// --- Including header-files
#include <string>
#include <stack>
#include <iostream>
#include <fstream>
#include "Element.h"
#include "NodeData.h"
#include "WayData.h"
#include "RestrictionData.h"
#include "XmlParserInterface.h"

enum ReaderStates {
    ReaderStateUnknown = 0x0000,   //00000000
    ReaderStateInNode = 0x0001,    //00000001
    ReaderStateInTag = 0x0002,     //00000010
    ReaderStateInWay = 0x0004,     //00000100
    ReaderStateInRelation = 0x0008,//00001000
    ReaderStateInNd = 0x0010,      //00010000
    ReaderStateInNodeTag = 0x0003, //00000011
    ReaderStateInWayNd = 0x0014,   //00010100
    ReaderStateInWayTag = 0x0006   //00000110
};

// --- Including header-files
class OsmReader : public XmlParserInterface{

    public:

        // --- Constructors
        // Default-Constructor
        OsmReader ();
        // Constructor
        OsmReader (const std::string &fileName);
        // Destructor
        virtual ~OsmReader ();

        // --- Methods
        void setFileName (const std::string &fileName);
        const std::string & getFileName () const;
        void readOsmFile ();

        // --- Class-functions
        static int convStrToInt (const std::string &str); 
        static double convStrToDbl (const std::string &str); 

    protected:

        // --- Methods
        void pushEmptyElementToStack (const Element &element);
        void pushElementToStack (const Element &element);
        void popElementFromStack (const Element &element);
        void createNodeFromElement (const Element &element);
        void updateNodeFromElement (const Element &element);
        void addTagElementToNode (const Element &element);
        void createWayFromElement (const Element &element);
        void updateWayFromElement (const Element &element);
        void addNdElementToWay (const Element &element);
        void addTagElementToWay (const Element &element);
        void prepareElement (const Element &element);
        void finalizeElement (const Element &element);
        const int & getReaderState () const;
        void setReaderState (const int & readerState);
        void updateState (const Element &element, bool up);

        // --- Functions of the parser interface 
        virtual void pushedElementToStack (const Element &element);
        virtual void poppedElementFromStack (const Element &element);
        virtual bool isElementInteresting (const Element &element) const;

        // --- Members
        std::string m_fileName;
        int m_readerState;
        NodeData m_currentNode;
        WayData m_currentWay;
        RestrictionData m_currentRestriction;

        // --- Constants
        static const int IN_NODE;
        static const int IN_TAG;
        static const int IN_WAY;
        static const int IN_RELATION;
        static const int IN_ND;

};

#endif /* __OSM_READER_H__ */
