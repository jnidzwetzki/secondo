/*
---- 
This file is part of SECONDO.

Copyright (C) 2004-2007, University in Hagen, Faculty of Mathematics and Computer Science, 
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

May 2007, M. Spiekermann. Initial version.

This file should contain all symbols in nested lists which are 
types or reserved words.

Below we define some string constants which correspond to the symbols for type
constructors and operators used in SECONDO algebra modules. These constants will
be used throughout the code below to avoid redundant use of string constants in
the code. This is important (i) to avoid strange runtime errors, e.g. in the
type mapping, which may be caused by a misspelled type name and (ii) to make
type or operator renaming easier.

*/

#ifndef SEC_SYMBOLS_H
#define SEC_SYMBOLS_H

#include <string>

namespace symbols {

 typedef const std::string Sym;

 // standard types
#undef INT
#undef REAL
#undef BOOL
#undef STRING
#undef TEXT 
 Sym INT("int");	
 Sym REAL("real");
 Sym BOOL("bool");
 Sym STRING("string");
 Sym TEXT("text");


 // point rectangle types
#undef XPOINT 
#undef XRECTANGLE
#undef SIMPLE 
 Sym XPOINT("xpoint");
 Sym XRECTANGLE("xrectangle");
 Sym INTERSECTS("intersects");
 Sym INSIDE("inside");
 Sym SIMPLE("SIMPLE");


 // stream example types
#undef INTSTREAM 
#undef REALSTREAM
#undef COUNT
 Sym INTSTREAM("intstream");
 Sym PRINT_INTSTREAM("printintstream");
 Sym REALSTREAM("realstream");
 Sym COUNT("countintstream");
 Sym FILTER("filterintstream");


 // relation algebra
#undef REL 
#undef TUPLE 
 Sym REL("rel");
 Sym TUPLE("tuple");

 // chess algebra
#undef MATERIAL 
#undef POSITION 
 Sym MATERIAL("material");
 Sym POSITION("position");

 // spatial, temporal, temporalunit algebra
#undef MPOINT
#undef POINT
 Sym MPOINT("mpoint");
 Sym POINT("point");

 // some reserved words of the query processor
#undef MAP 
#undef STREAM 
#undef APPEND 
 Sym MAP("map"); 
 Sym STREAM("stream"); 
 Sym APPEND("APPEND"); 

 // symbols for runtime flags
 // and counters
 Sym CTR_CreatedTuples("RA:CreatedTuples");
 Sym CTR_DeletedTuples("RA:DeletedTuples");
 Sym CTR_MaxmemTuples("RA:MaxTuplesInMem");
 Sym CTR_MemTuples("RA:TuplesInMem");
 
 Sym CTR_INT_Created("STD:INT_created");
 Sym CTR_INT_Deleted("STD:INT_deleted");
 Sym CTR_REAL_Created("STD:REAL_created");
 Sym CTR_REAL_Deleted("STD:REAL_deleted");
 Sym CTR_BOOL_Created("STD:BOOL_created");
 Sym CTR_BOOL_Deleted("STD:BOOL_deleted");
 Sym CTR_STR_Created("STD:STRING_created");
 Sym CTR_STR_Deleted("STD:STRING_deleted");

 Sym CTR_ATTR_BASIC_OPS("RA:Attr:BasicOps");
 Sym CTR_ATTR_HASH_OPS("RA:Attr:HashOps");
 Sym CTR_ATTR_COMPARE_OPS("RA:Attr:CompareOps");
 Sym CTR_INT_COMPARE("CcInt::Compare");
 Sym CTR_INT_EQUAL("CcInt::Equal");
 Sym CTR_INT_LESS("CcInt::Less");
 Sym CTR_INT_HASH("CcInt::HashValue");
 Sym CTR_INT_ADJACENT("CcInt::Adjacent");

 Sym CTR_TBUF_BYTES_W("RA:TupleBuf:Write:Bytes");
 Sym CTR_TBUF_PAGES_W("RA:TupleBuf:Write:Pages");
 Sym CTR_TBUF_BYTES_R("RA:TupleBuf:Read:Bytes");
 Sym CTR_TBUF_PAGES_R("RA:TupleBuf:Read:Pages");

} // end of namespace	

#endif
