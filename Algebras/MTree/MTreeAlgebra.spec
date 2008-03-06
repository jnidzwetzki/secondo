#This file is part of SECONDO.

#Copyright (C) 2004, University in Hagen, Department of Computer Science,
#Database Systems for New Applications.

#SECONDO is free software; you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation; either version 2 of the License, or
#(at your option) any later version.

#SECONDO is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.

#You should have received a copy of the GNU General Public License
#along with SECONDO; if not, write to the Free Software
#Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

operator createmtree alias CREATEMTREE pattern _ op [_]
# rel/tuple stream x attribute -> mtree

operator createmtree2 alias CREATEMTREE2 pattern _ op [_, _, _]
# rel/tuple stream x attribute x config x distfun -> mtree

operator createmtree3 alias CREATEMTREE3 pattern _ op [_, _, _, _]
# rel/tuple stream x attribute x config x distfun x distdata -> mtree

operator rangesearch alias RANGESEARCH pattern _ op [_, _, _]
# mtree x rel x attr x real -> real

operator nnsearch alias NNSEARCH pattern _ op [_, _, _]
# mtree x rel x attr x int -> real
