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

operator lr_moveto alias LR_MOVETO pattern _ op [_, _]
operator lineTo alias LINETO pattern _ op [_, _]
operator quadTo alias QUADTO pattern _ op [_, _, _, _]
operator closeLine alias CLOSELINE pattern _ op
operator toRegion alias TOREGION pattern _ op
operator lr_contains alias LR_CONTAINS pattern _ op [_, _]
operator lr_intersects alias LR_INTERSECTS pattern _ infixop _
operator getbounds alias GETBOUNDS pattern _ op
