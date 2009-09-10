//This file is part of SECONDO.

//Copyright (C) 2004, University in Hagen, Department of Computer Science, 
//Database Systems for New Applications.

//SECONDO is free software; you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation; either version 2 of the License, or
//(at your option) any later version.

//SECONDO is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

//You should have received a copy of the GNU General Public License
//along with SECONDO; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


package  viewer.hoese.algebras;

import  sj.lang.ListExpr;
import  viewer.*;
import viewer.hoese.*;
import  java.util.*;
import  javax.swing.*;
import tools.Reporter;


/**
 * A displayclass for the intimereal-type (spatiotemp algebra), alphanumeric with TimePanel
 */
public class Dsplintimereal extends Dsplinstant {
  double Wert;

  /**
   * A method of the Timed-Interface to render the content of the TimePanel
   * @param PixelTime pixel per hour
   * @return A JPanel component with the renderer
   * @see <a href="Dsplintimerealsrc.html#getTimeRenderer">Source</a>
   */
  public JPanel getTimeRenderer (double PixelTime) {
    if(!defined){
      return new JPanel(); 
    }
    JPanel jp = super.getTimeRenderer(PixelTime);
    JLabel jl = (JLabel)jp.getComponent(0);
    jl.setText(jl.getText() + "  " + Wert);
    return  jp;
  }

  /**
   * Scans the representation of a intimereal datatype 
   * @param v An instant and a real value
   * @see sj.lang.ListExpr
   * @see <a href="Dsplintimerealsrc.html#ScanValue">Source</a>
   */
  public String getString(ListExpr v) {
    if(isUndefined(v)){
       defined = false;
       return "undefined";
    }
    if (v.listLength() != 2) {
      Reporter.writeError("Error: No correct intimebool expression: 2 elements needed");
      err = true;
      return "<error>";
    }
    String v2 = super.getString(v.first());
    if (err){
      defined=false;
      return "<error>";
    } 
    err = true;
    Double d = LEUtils.readNumeric(v.second());
    if (d == null){
      defined=false; 
      return "<error>";

    }
    Wert = d.doubleValue();
    defined = true;
    err = false;
    return "["+v2+"] "+Wert;
  }

  public String toString () {
    return entry; 
  }

  public void init (String name, int nameWidth, int indent, 
                    ListExpr type, ListExpr value, QueryResult qr)
  {
     String T = name;
     String V = getString(value);
     T=extendString(T,nameWidth, indent);
     entry=(T + " : " + V);
     if(!err){
        qr.addEntry(this);
     }else{
        qr.addEntry(entry);
     }
     return;
  }



}



