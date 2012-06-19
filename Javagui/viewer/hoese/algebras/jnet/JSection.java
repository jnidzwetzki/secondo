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

//2012, June Simone Jandt

package viewer.hoese.algebras.jnet;

import java.awt.geom.*;
import java.awt.*;
import java.util.*;
import sj.lang.ListExpr;
import viewer.*;
import viewer.hoese.*;



/**
 * JSection
 * Describes the spatial curve between two junctions.
 */
public class JSection{

   private static Area basicArrow;
   private boolean isArrow;
   private Point2D.Double p1;
   private Point2D.Double p2;

   private Vector<JRouteInterval> routeIntervals = new Vector<JRouteInterval>();
   private JDirection dir;
   private boolean startSmaller;
   private GeneralPath curve;
   private Rectangle2D.Double bounds = new Rectangle2D.Double(0.0,0.0,0.0,0.0);


  public JSection(ListExpr value)
  {
    if (value.listLength() == 12){
      dir = new JDirection(value.fifth());
      if (dir.toString().compareTo("Both") != 0){
        isArrow = true;
      } else {
        isArrow = false;
      }
      readCurve(value.second());
      readRouteIntervals(value.eighth());
    }
  }

  public Shape getRenderObject(int i, AffineTransform af, double pointSize){
    if (i == 0){
      return curve;
    } else if (isArrow){
      if (dir.toString().compareTo("Up") == 0){
        return getArrow(af, p1, p2, pointSize);
      } else {
        return getArrow(af, p2, p1, pointSize);
      }
    } else {
      return null;
    }
  }

  public Rectangle2D.Double getBounds(){
    return bounds;
  }

  private void readCurve (ListExpr value){
    if (value.listLength() == 2) {
      if (value.second().atomType() == ListExpr.BOOL_ATOM)
        startSmaller = value.second().boolValue();
      else
        startSmaller = true;
      Vector<Vector<Point2D.Double>> pointSequences =
        new Vector<Vector<Point2D.Double>>();
      ListExpr rest = value.first();
      while (!rest.isEmpty()){
        ListExpr curSegment = rest.first();
        if (curSegment.listLength() == 4){
          Double X1 = LEUtils.readNumeric(curSegment.first());
          Double Y1 = LEUtils.readNumeric(curSegment.second());
          Double X2 = LEUtils.readNumeric(curSegment.third());
          Double Y2 = LEUtils.readNumeric(curSegment.fourth());
          if (X1 != null && Y1 != null && X2 != null && Y2 != null){
            double x1 = X1.doubleValue();
            double y1 = Y1.doubleValue();
            double x2 = X2.doubleValue();
            double y2 = Y2.doubleValue();
            insertSegment(pointSequences, x1,y1,x2,y2);
          }
        }
        rest = rest.rest();
      }
      if (pointSequences.size() == 1){
        Vector<Point2D.Double> sequence = pointSequences.get(0);
        if(isCycle(sequence)){
          int end = findSmallestIndex(sequence);
          int start;
          if(startSmaller){
            start=end-1;
            if(start<0){
              start = sequence.size()-1;
            }
          } else {
            start = end+1;
            if(start>=sequence.size()){
              start=0;
            }
          }
          p1 = sequence.get(start);
          p2 = sequence.get(end);
        } else {
          boolean firstLess = isLess(sequence.get(0) ,
                              sequence.get(sequence.size()-1));
          if(startSmaller == firstLess){
            this.p1 = sequence.get(sequence.size()-2);
            this.p2 = sequence.get(sequence.size()-1);
          } else {
            this.p1 = sequence.get(1);
            this.p2 = sequence.get(0);
          }
        }
      }
      curve = new GeneralPath();
      Point2D.Double rendRes = new Point2D.Double(0.0,0.0);
      Iterator<Vector<Point2D.Double>> it = pointSequences.iterator();
      while(it.hasNext()){
        Vector<Point2D.Double> sequence = it.next();
        for(int i=0 ; i < sequence.size(); i++){
          Point2D.Double p = sequence.get(i);
          if(ProjectionManager.project(p.x,p.y,rendRes)){
            if(i == 0){
              curve.moveTo((float)rendRes.x, (float)rendRes.y);
            } else {
              curve.lineTo((float)rendRes.x,(float)rendRes.y);
            }
          } else {
            if(i == 0){
              curve.moveTo((float)p.x, (float)p.y);
            } else {
              curve.lineTo((float)p.x,(float)p.y);
            }
          }
        }
      }
      bounds.setRect(curve.getBounds2D());
    }
  }

  private void insertSegment(Vector<Vector<Point2D.Double>> sequences,
                             double x1, double y1, double x2, double y2){
    Point2D.Double p1 = new Point2D.Double(x1,y1);
    Point2D.Double p2 = new Point2D.Double(x2,y2);
    Vector<Vector<Point2D.Double>> connectedSequences =
      new Vector<Vector<Point2D.Double>>();
    Iterator<Vector<Point2D.Double>> it = sequences.iterator();
    while(it.hasNext() && connectedSequences.size() < 2){
      Vector<Point2D.Double> seq = it.next();
      if( almostEqual(seq.get(0),p1) || almostEqual(seq.get(0),p2) ||
          almostEqual(seq.get(seq.size()-1),p1) ||
          almostEqual(seq.get(seq.size()-1),p2)){
        connectedSequences.add(seq);
      }
    }
    if(connectedSequences.size()==0){ // new unconnected segment
      Vector<Point2D.Double> newSeq = new Vector<Point2D.Double>();
       newSeq.add(p1);
       newSeq.add(p2);
       sequences.add(newSeq);
    } else { // new segment extends a single sequence
      // extend the first sequence by one point
      Vector<Point2D.Double> seq = connectedSequences.get(0);
      if(almostEqual(p1,seq.get(0))){
        seq.add(0,p2);
      } else if(almostEqual(p2,seq.get(0))){
        seq.add(0,p1);
      } else if(almostEqual(p1,seq.get(seq.size()-1))){
        seq.add(p2);
      } else {
        seq.add(p1);
      }
      if(connectedSequences.size()==2){ // we have to connect both sequences
        Vector<Point2D.Double> seq2 = connectedSequences.get(1);
        if(almostEqual(seq.get(seq.size()-1), seq2.get(0))){ // seq2 extends seq
          sequences.remove(seq2);
          seq.addAll(seq2);
        } else if(almostEqual(seq2.get(seq2.size()-1),seq.get(0))){ // seq extends seq2
          seq2.addAll(seq);
          sequences.remove(seq);
        } else if( almostEqual(seq.get(seq.size()-1),seq2.get(seq2.size()-1))) { // both endpoints are equal
          sequences.remove(seq2);
          reverse(seq2);
          seq.addAll(seq2);
        } else if(almostEqual(seq.get(0),seq2.get(0))){ // both starting points are equal
          reverse(seq);
          seq.addAll(seq2);
          sequences.remove(seq2);
        }
      }
    }
  }

private static boolean almostEqual(double a, double b){
  return Math.abs(a-b) < 0.00000001;
}

private static boolean almostEqual(Point2D.Double p1, Point2D.Double p2){
  return almostEqual(p1.x,p2.x) && almostEqual(p1.y,p2.y);
}

private static boolean isLess(Point2D.Double p1, Point2D.Double p2){
  if(almostEqual(p1.x, p2.x)){
    if(almostEqual(p1.y, p2.y)){
      return false;
    } else {
      return p1.y < p2.y;
    }
  }
  return p1.x < p2.x;
}

private static void reverse(Vector<Point2D.Double >  v){
  Stack<Point2D.Double> stack = new Stack<Point2D.Double>();
  for(int i = 0; i < v.size() ; i++){
       stack.push(v.get(i));
  }
  v.clear();
  while(!stack.isEmpty()){
     v.add(stack.pop());
  }
}

 private static boolean isCycle(Vector<Point2D.Double> s){
    if(s.size()<2){
      return false;
    }
    return s.get(0).equals(s.get(s.size()-1));
  }

  private static int findSmallestIndex(Vector<Point2D.Double> s){
     int index = 0;
     Point2D.Double p = s.get(0);
     for(int i=1;i<s.size();i++){
       Point2D.Double p2 = s.get(i);
       if(isLess(p2,p)){
         index = i;
         p = p2;
       }
     }
     return index;
  }

  private static void resort(Vector<Point2D.Double> sequence){
     if(sequence.size()<2){
        return;
     }

     if(! sequence.get(0).equals(sequence.get(sequence.size()-1))){
        // not a cycle
        return;
     }

     // cycle found, sort sequence, that the smallest contained point
     // is the first one
     // part 1 search the index of the smallest point
     int index = findSmallestIndex(sequence);

     // TODO implement this algorithm without copying vectors
     Vector<Point2D.Double> sequenceCopy = new Vector<Point2D.Double>(sequence);
     int size = sequence.size();
     for(int i=0;i<size;i++){
        System.out.println((i+index)%size + "->" + i);
        sequence.set(i, sequenceCopy.get((i+index)%size));
     }
  }

  private Shape getArrow(AffineTransform af, Point2D.Double point1, Point2D.Double point2, double pointSize){
   createBasicArrow(pointSize);
   // transform the basicArrow to be in the correct angle at the end of the connection
   double x1 = point1.getX();
   double x2 = point2.getX();
   double y1 = point1.getY();
   double y2 = point2.getY();
   AffineTransform aat = new AffineTransform();
   double sx = af.getScaleX();
   double sy = af.getScaleY();
   AffineTransform trans = AffineTransform.getTranslateInstance(x2,y2);
   aat.concatenate(trans);
   // normalize
   double dx =  x1-x2;
   double dy = y1-y2;
   double len = Math.sqrt(dx*dx+dy*dy); // the length
   dx = dx / len;
   dy = dy / len;
   AffineTransform Rot = new AffineTransform(dx,dy,-dy,dx,0,0);
   aat.concatenate(Rot);
   AffineTransform scale = AffineTransform.getScaleInstance(5/sx,5/sy);
   aat.concatenate(scale);
   Shape S = aat.createTransformedShape(basicArrow);
   return S;
  }


 private void createBasicArrow(double x){
    GeneralPath gparrow = new GeneralPath();
    gparrow.moveTo(0,0);
    gparrow.lineTo(5,-1);
    gparrow.lineTo(3,0);
    gparrow.lineTo(5,1);
    gparrow.lineTo(0,0);
    Shape s = gparrow;
    if(x > 0){
      AffineTransform at = AffineTransform.getScaleInstance(x/5,x/5);
      s =  at.createTransformedShape(gparrow);
    } else {
      basicArrow=null;
      return;
    }
    basicArrow = new Area(s);
  }

  private void readRouteIntervals(ListExpr value){
    while (!value.isEmpty()){
      routeIntervals.add(new JRouteInterval(value.first()));
      value = value.rest();
    }
  }
}



