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

 * JRelMetaData.java
 *
 * Created on 10. Februar 2004, 21:35
 */

package viewer.jpeg;

import javax.swing.tree.*;
import gui.*;
import gui.idmanager.ID;


/** This class holds meta-data of Relations of <strong>Secondo</strong>-objects
 *  containing JPEG- or JINFO objects.
 *
 *  @author Stefan Wich <mailto:stefan.wich@fernuni-hagen.de>
 */
public class JRelMetaData extends JPGViewerMetaData {
    
    private JpegTreeModel jTMRelation;
    /** Creates a new instance of JRelMetaData */
    public JRelMetaData(ID objID) {
        super(objID);
        jTMRelation = new JpegTreeModel(null);
    }
    
    public JRelMetaData(SecondoObject sObj){
        super(sObj.getID());
        jTMRelation = new JpegTreeModel(new JpegTreeNode(sObj.toString()));
    }
    
    public void setRoot(SecondoObject sObj){
        JpegTreeNode jTNRoot = new JpegTreeNode(sObj.toString());
        jTMRelation.setRoot(jTNRoot);
    }
    
    public JpegTreeModel getModel(){ return jTMRelation; }
}
