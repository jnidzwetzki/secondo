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

operator intersects alias INTERSECTS pattern _ infixop _
operator inside alias INSIDE pattern _ infixop _
operator before alias BEFORE pattern _ infixop _
operator union alias UNION pattern _ infixop _
operator minimum alias MINIMUM pattern op ( _ )
operator maximum alias MAXIMUM pattern op ( _ )
operator inst alias INST pattern op ( _ )
operator val alias VAL pattern op ( _ )
operator atinstant alias ATINSTANT pattern _ infixop _
operator atperiods alias ATPERIODS pattern _ infixop _
operator deftime alias DEFTIME pattern op ( _ )
operator trajectory alias TRAJECTORY pattern op ( _ )
operator present alias PRESENT pattern _ infixop _
operator passes alias PASSES pattern _ infixop _
operator initial alias INITIAL pattern op ( _ )
operator final alias FINAL pattern op ( _ )
operator at alias AT pattern _ infixop _
operator units alias UNITS pattern op ( _ )
operator theyear alias THEYEAR pattern op ( _ )
operator themonth alias THEMONTH pattern op ( _, _ )
operator theday alias THEDAY pattern op ( _, _, _ )
operator thehour alias THEHOUR pattern op ( _, _, _ ,_ )
operator theminute alias THEMINUTE pattern op ( _, _, _ ,_ , _ )
operator thesecond alias THESECOND pattern op ( _, _, _ ,_ , _, _ )
operator theperiod alias THEPERIOD pattern op ( _, _ )
operator theRange alias THERANGE pattern op( _ , _, _, _ )

operator equal alias EQUAL pattern _ infixop _
operator nonequal alias NONEQUAL pattern _ infixop _

operator box2d alias BOX2D pattern op ( _ )
operator bbox2d alias BBOX2D pattern op ( _ )
operator simplify alias SIMPLIFY pattern op (_,_)
operator breakpoints alias BREAKPOINTS pattern op(_,_)
operator integrate alias INTEGRATE pattern op(_)
operator minimum alias MINIMUM pattern op(_)
operator maximum alias MAXIMUM pattern op(_)

operator approximate alias APPROXIMATE pattern _ op [_, _ ]
operator translateappend alias TRANSLATEAPPEND pattern _ op [_, _ ]
operator translateappendS alias TRANSLATEAPPENDS pattern _ op [_, _ ]
operator reverse alias REVERSE pattern op ( _ )
