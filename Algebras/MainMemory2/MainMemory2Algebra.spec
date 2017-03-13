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

operator memload alias MEMLOAD pattern op (_)
operator memloadflob alias MEMLOADFLOB pattern op (_)
operator meminit alias MEMINIT pattern op (_)
operator mfeed alias MFEED pattern _ op
operator letmconsume alias LETMCONSUME pattern _ op [_]
operator letmconsumeflob alias LETMCONSUMEFLOB pattern _ op [_]
operator memdelete alias MEMDELETE pattern op (_)
operator memobject alias MEMOBJECT pattern op (_)
operator memlet alias MEMLET pattern op(_,_)
operator memletflob alias MEMLETFLOB pattern op(_,_)
operator memupdate alias MEMUPDATE pattern op(_,_)
#operator mcreateRtree alias MCREATERTREE pattern op(_,_)
operator mcreateRtree alias MCREATERTREE pattern _ op [_]
operator memgetcatalog alias MEMGETCATALOG pattern op()
operator memsize alias MEMSIZE pattern op()
operator memclear alias MEMCLEAR pattern op()
operator meminsert alias MEMINSERT pattern _ op[_]
operator mwindowintersects alias MWINDOWINTERSECTS pattern _ _ op[_]
operator mwindowintersectsS alias MWINDOWINTERSECTSS pattern  _ op[_]
operator mconsume alias MCONSUME pattern _ op
operator mcreateAVLtree2 alias MCREATEAVLTREE2 pattern _ op[_,_,_]
operator mcreateAVLtree alias  MCREATEAVLTREE pattern _ op [_]
operator mexactmatch alias MEXACTMATCH pattern _ _ op [_]
operator mrange alias MRANGE pattern _ _ op [_, _]
operator matchbelow alias MATCHBELOW pattern _ _ op [_]
operator matchbelow2 alias MATCHBELOW2 pattern _ _ op [_,_,_]
operator mcreateRtree2 alias MCREATERTREE2 pattern _ op [_,_]

operator mcreateMtree2 alias MCREATEMTREE2 pattern _ op [_,_,_]
operator mdistRange2 alias MDISTRANGE2 pattern _ op [_,_]
operator mdistScan2 alias MDISTSCAN2 pattern _ op [_]

operator mcreateMtree alias MCREATEMTREE pattern _ op [_,_]
operator mdistRange alias MDISTRANGE pattern _ _ op [_,_]
operator mdistScan alias MDISTSCAN  pattern _ _ op [_]

operator mexactmatchS alias MEXACTMATCHS pattern _ op [_]
operator mrangeS alias MRANGES pattern _ op[_,_]
operator matchbelowS alias MATCHBELOWS pattern _ op[_]

operator gettuples alias GETTUPLES pattern _ _ op



operator mwrap alias MWRAP pattern op(_)

operator mcreatettree alias  MCREATETTREE pattern _ op [_]
operator minsertttree alias MINSERTTTREE pattern _ op [_,_]
operator mdeletettree alias MDELETETTREE pattern _ op [_,_]

operator minsertavltree alias MINSERTAVLTREE pattern _ op [_,_]
operator mdeleteavltree alias MDELETEAVLTREE pattern _ op [_,_]

operator mcreateinsertrel alias MCREATEINSERTREL pattern op(_)
operator minsert alias MINSERT pattern _ op [_]
operator minsertsave alias MINSERTSAVE pattern _ op [_,_]
operator minserttuple alias MINSERTTUPLE pattern _ op [list]
operator minserttuplesave alias MINSERTTUPLESAVE pattern _ op [list;_]
operator mcreatedeleterel alias MCREATEDELETEREL pattern op(_)
operator mdelete alias MDELETE pattern _ op [_]
operator mdeletesave alias MDELETESAVE pattern _ op [_,_]
operator mdeletebyid alias MDELETEBYID pattern _ op [_]

operator mdeletedirect     alias MDELETEDIRECT pattern _ _ op
operator mdeletedirectsave alias MDELETEDIRECTSAVE pattern _ _ _ op

operator mcreateupdaterel alias MCREATEUPDATEREL pattern op(_)
operator mupdate alias MUPDATE pattern _ op[_; funlist] implicit parameter tuple type MTUPLE2
operator mupdatesave alias MUPDATESAVE pattern _ op[_,_; funlist] implicit parameter tuple type MTUPLE2
operator mupdatebyid alias MUPDATEBYID pattern _ op[_; funlist] 
         implicit parameter tuple type MTUPLE

operator moinsert alias MOINSERT pattern _ op [_]
operator modelete alias MODELETE pattern _ op [_]
operator moconsume alias MOCONSUME pattern _ op[list]
operator letmoconsume alias LETMOCONSUME pattern _ op [_;list]
operator letmoconsumeflob alias LETMOCONSUMEFLOB pattern _ op [_;list]
operator morange alias MORANGE pattern _ op [list; list]
operator moleftrange alias MOLEFTRANGE pattern _ op [list]
operator morightrange alias MORIGHTRANGE pattern _ op [list]
operator moshortestpathd alias MOSHORTESTPATHD pattern _ op [_,_,_; fun] 
         implicit parameter tuple type MTUPLE
operator moshortestpatha alias MOSHORTESTPATHA pattern _ op [_,_,_; fun,fun] 
         implicit parameter tuple type MTUPLE
operator moconnectedcomponents alias MOCONNECTEDCOMPONENTS pattern _ op 
         
operator mquicksort alias MQUICKSORT pattern _ op
operator mquicksortby alias MQUICKSORTBY pattern _ op [list]

operator memglet alias MEMGLET pattern op(_,_)
operator memgletflob alias MEMGLETFLOB pattern op(_,_)
operator mgshortestpathd alias MGSHORTESTPATHD pattern _ op [_,_,_; fun] 
         implicit parameter tuple type MTUPLE
operator mgshortestpatha alias MGSHORTESTPATHA pattern _ op [_,_,_; fun, fun] 
         implicit parameter tuple type MTUPLE
operator mgconnectedcomponents_old alias MGCONNECTEDCOMPONENTS_OLD pattern _ op 
operator mgconnectedcomponents alias MGCONNECTEDCOMPONENTS pattern _ op 
         
operator momapmatchmht alias MOMAPMATCHMHT pattern op(_,_,_,_)

         
operator collect_mvector alias COLLECT_MVECTOR pattern _ op[_,_]

operator pwrap alias PWRAP pattern op(_)

operator sizemv alias SIZEMV pattern op(_)
operator getmv alias GETMV pattern _ op[_]
operator putmv alias putMV pattern _ op[_,_]
operator isSortedmv alias ISSORTEDMV pattern op(_)
operator sortmv alias SORTMV pattern op(_)
operator feedmv alias FEEDMV pattern  _ op
operator findmv alias FINDMV pattern  op(_,_)
operator matchbelowmv alias MATCHBELOWMV pattern  _ op [_] 

operator insertmv alias INSERTMV pattern _ op [_,_]


operator count alias COUNT pattern _ op
operator mblock alias MBLOCK pattern _ op 


operator mcreatepqueue alias mcreatepqueue pattern _ op[_,_]
operator size alias SIZE pattern op(_)
operator mfeedpq alias MFEEDPQ pattern _ op
operator mfeedpqAbort alias MFEEDPQABORT pattern _ op [_ ]
operator minserttuplepq alias MINSERTTUPLEPQ pattern op(_,_,_,_)
operator minserttuplepqprojectU alias MINSERTTUPLEPQPROJECTU pattern _ op[_,_,_;list]
operator minserttuplepqproject alias MINSERTTUPLEPQPROJECT pattern _ op[_,_;list]


operator mcreatestack alias MCREATESTACK pattern _ op [_]
operator mcreatestackflob alias MCREATESTACKFLOB pattern _ op [_]
operator mfeedstack alias MFEEDSTACK pattern _ op
operator stacksize alias STACKSIZE pattern op(_)
operator insertmstack alias INSERTMSTACK pattern _ op [_]


operator createmgraph2 alias CREATEMGRAPH2 pattern _ op [_,_,fun,_] implicit parameter streamelem type STREAMELEM
operator mg2insertorig alias MG2INSERTORIG pattern _ op [_,_,fun,_] implicit parameter streamelem type STREAMELEM
operator mg2insert alias MG2INSERT pattern _ op [_]
operator mg2feed alias MG2FEED pattern _ op
operator mg2nodemap alias MG2NODEMAP pattern _ op[_]
operator mg2numvertices alias MG2NUMVERTICES pattern op(_)
operator mg2successors alias MG2SUCCESSORS pattern _ op[_]
operator mg2predecessors alias MG2PREDECESSORS pattern _ op[_]
operator mg2numsuccessors alias MG2NUMSUCCESSORS pattern _ op[_]
operator mg2numpredecessors alias MG2NUMPREDECESSORS pattern _ op[_]
operator mg2disconnect alias MG2NUMPREDECESSORS pattern _ op[_]
operator mg2connectedcomponents alias MG2CONNECTEDCOMPONENTS pattern _ op

operator createmgraph3 alias CREATEMGRAPH3 pattern _ op [_,_,_,_,_]
operator mg3insert alias MG3INSERT pattern _ op [_]
operator mg3feed alias mg3feed pattern _ op
operator mg3numvertices alias MG3NUMVERTICES pattern op(_)
operator mg3successors alias MG3SUCCESSORS pattern _ op[_]
operator mg3predecessors alias MG3PREDECESSORS pattern _ op[_]
operator mg3numsuccessors alias MG3NUMSUCCESSORS pattern _ op[_]
operator mg3numpredecessors alias MG3NUMPREDECESSORS pattern _ op[_]
operator mg3disconnect alias MG3NUMPREDECESSORS pattern _ op[_]
operator mg3connectedcomponents alias MG3CONNECTEDCOMPONENTS pattern _ op 





