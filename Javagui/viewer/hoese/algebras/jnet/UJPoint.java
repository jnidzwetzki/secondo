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

//2012, July Simone Jandt

package  viewer.hoese.algebras.jnet;

import java.awt.geom.*;
import java.awt.*;
import java.util.*;
import sj.lang.ListExpr;
import viewer.*;
import viewer.hoese.*;
import javax.swing.*;


/**
 * UJPoint
 * Describes an time interval and the movement of a single position within this
 * time interval in the network.
 */
public class UJPoint{

   String netId;
   private JRouteInterval rint;
   private JSection sect;
   private Interval time;


  public UJPoint(ListExpr value) throws JNetworkNotAvailableException {
    if (value.listLength() == 3){
      netId = value.first().stringValue();
      JNetwork jnet = JNetworkManager.getInstance().getNetwork(netId);
      time = LEUtils.readInterval(value.second());
      rint = new JRouteInterval(value.third());
      sect = jnet.getSection(rint);
    } else {
      netId = "undefined";
    }
  }

  public String toString(){
    if (netId.compareTo("undefined") != 0)
      return "ujpoint";
    else
      return "undefined";
  }

  public Rectangle2D.Double getBounds(){
    return sect.getBounds();
  }

  public int numOfShapes(){
    return 1;
  }

  public boolean isPointType(int no){
    return true;
  }

  public boolean isLineType(int no){
    return false;
  }

  public Shape getRenderObject(int no, AffineTransform af, double actTime,
                               double pointSize, boolean asRect){
    if(!time.isDefinedAt(actTime)){ // t is outside from the deftime
      return null;
    }
    double t1 = time.getStart();
    double t2 = time.getEnd();
    double timeDelta = (actTime-t1)/(t2-t1);
    double distOnRoute;
    if (rint.getDir().toString().compareTo("Down") != 0 )
      distOnRoute = rint.getStartPos() + rint.getLength() * timeDelta;
    else
      distOnRoute = rint.getEndPos() - rint.getLength() * timeDelta;
    Point2D.Double actPos = sect.getPosition(distOnRoute);
    Point2D.Double rendPos = new Point2D.Double(0.0,0.0);
    if (ProjectionManager.project(actPos.x, actPos.y, rendPos)){
      actPos = rendPos;
    }
    double pointSizeX = Math.abs(pointSize/af.getScaleX());
    double pointSizeY = Math.abs(pointSize/af.getScaleY());
    Shape shape;
    if (asRect) {
      shape = new Rectangle2D.Double(actPos.getX()- pointSizeX/2,
                                     actPos.getY()- pointSizeY/2,
                                     pointSizeX,
                                     pointSizeY);
    } else {
      shape = new Ellipse2D.Double(actPos.getX() - pointSizeX/2,
                                   actPos.getY() - pointSizeY/2,
                                   pointSizeX,
                                   pointSizeY);
    }
    return  shape;
  }

  public Interval getBoundingInterval (){
    return time;
  }

  public Vector getIntervals(){
    Vector v=new Vector(1,0);
    v.add(time);
    return v;
  }

  public JPanel getTimeRenderer (double PixelTime) {
    int start = 0;
    JLabel label = new JLabel("|"+LEUtils.convertTimeToString(time.getStart()).substring(11,
                              16), JLabel.LEFT);
    label.setBounds(start, 15, 100, 15);
    label.setVerticalTextPosition(JLabel.CENTER);
    label.setHorizontalTextPosition(JLabel.RIGHT);
    JPanel jpan = new JPanel(null);
    jpan.setPreferredSize(new Dimension(100, 25));
    jpan.add(label);
    //Add labels to the JPanel.
    return  jpan;
  }

  public double getStartTime(){
    return time.getStart();
  }

  public double getEndTime(){
    return time.getEnd();
  }

}



