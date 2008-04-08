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

//paragraph [1] Title: [{\Large \bf \begin{center}] [\end{center}}]
//paragraph [10] Footnote: [{\footnote{] [}}]

1.1 Declaration of Operator nocomponents

April 2008 Simone Jandt

Defines, includes, and constants

*/
#ifndef OPNOCOMPONENTS_H
#define OPNOCOMPONENTS_H

/*
Returns the number of route intervals of the given gline.

*/

class OpNoComponents {
  public:

/*
TypeMap Function of the operator nocomponents

*/

    static ListExpr TypeMap(ListExpr args);

/*
ValueMapping function of the operator nocomponents

*/

    static int ValueMapping (Word* in_pArgs, Word& in_pResult, int in_iMessage,
      Word& in_pLocal, Supplier in_pSupplier);

/*
Specification of operator nocomponents:

*/

    static const string Spec;

};

#endif /*OPNOCOMPONENTS_H*/
