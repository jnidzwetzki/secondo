

package  viewer.hoese.algebras;

import  java.awt.geom.*;
import  java.awt.*;
import  sj.lang.ListExpr;
import  java.util.*;
import  viewer.*;
import viewer.hoese.*;


/**
 * The displayclass of the points datatype (Rose algebra).
 */
public class Dsplpoints extends DisplayGraph {
  Vector points;
  Rectangle2D.Double bounds;

  /**
   * Scans the representation of the points datatype and constructs the points Vector.
   * @param v the numeric value of a list of x- and y-coordinate
   * @see sj.lang.ListExpr
   * @see <a href="Dsplpointssrc.html#ScanValue">Source</a>
   */
  public void ScanValue (ListExpr value) {
    double koord[] = new double[2];
    double x,y;
    points = new Vector(20, 20);
    while (!value.isEmpty()) {
      ListExpr v = value.first();
      //System.out.println(v.writeListExprToString());
      if (v.listLength() != 2) {
        System.out.println("Error: No correct points expression: 2 elements needed");
        err = true;
        return;
      }
      for (int koordindex = 0; koordindex < 2; koordindex++) {
        Double d = LEUtils.readNumeric(v.first());
        if (d == null) {
          err = true;
          return;
        }
        koord[koordindex] = d.doubleValue();
        v = v.rest();
      }
      if (!err) {
        try{
          x = ProjectionManager.getPrjX(koord[0],koord[1]);
	  y = ProjectionManager.getPrjY(koord[0],koord[1]); 
          Point2D.Double point = new Point2D.Double(x, y);
          points.add(point);
	} catch(Exception e){
          System.out.println("error in project : ("+koord[0]+","+koord[1]+")");
	}
      }
      value = value.rest();
    }
  }

  /**
   * Init. the Dsplpoints instance.
   * @param type The symbol points
   * @param value The numeric value of a list of points.
   * @param qr queryresult to display output.
   * @see generic.QueryResult
   * @see sj.lang.ListExpr
   * @see <a href="Dsplpointssrc.html#init">Source</a>
   */
  public void init (ListExpr type, ListExpr value, QueryResult qr) {
    AttrName = type.symbolValue();
    ScanValue(value);
    if (err) {
      System.out.println("Error in ListExpr :parsing aborted");
      qr.addEntry(new String("(" + AttrName + ": GA(points))"));
      return;
    }
    else
      qr.addEntry(this);
    ListIterator li = points.listIterator();
    bounds = null;
    while (li.hasNext()) {
      Point2D.Double p = ((Point2D.Double)li.next());
      if (bounds == null)
        bounds = new Rectangle2D.Double(p.getX(), p.getY(), 0, 0);
      else
        bounds = (Rectangle2D.Double)bounds.createUnion(new Rectangle2D.Double(p.getX(),
            p.getY(), 0, 0));
    }
    //System.out.println("Pointsb:"+bounds);
    RenderObject = bounds;
  }

  /**
   * @return The boundingbox of all the points
   * @see <a href="Dsplpointssrc.html#getBounds">Source</a>
   */
  public Rectangle2D.Double getBounds () {
    //System.out.println(bounds);
    return  bounds;
  }

  /**
   * Tests if a given position is contained in any of the points.
   * @param xpos The x-Position to test.
   * @param ypos The y-Position to test.
   * @param scalex The actual x-zoomfactor
   * @param scaley The actual y-zoomfactor
   * @return true if x-, ypos is contained in this points type
   * @see <a href="Dsplpointssrc.html#contains">Source</a>
   */
  public boolean contains (double xpos, double ypos, double scalex, double scaley) {
    boolean hit = false;
    ListIterator li = points.listIterator();
    double scale = Cat.getPointSize()*0.7*scalex;               //Pythagoras Math.max(scalex,scaley);
    while (li.hasNext())
      hit |= (((Point2D.Double)li.next()).distance(xpos, ypos) <= scale);
    return  hit;
  }

  /**
   * Draws the included points by creating a Dsplpoint and calling its draw-method.
   * @param g The graphics context
   * @see <a href="Dsplpointssrc.html#draw">Source</a>
   */
  public void draw (Graphics g) {
    ListIterator li = points.listIterator();
    while (li.hasNext()) {
      Point2D.Double p = (Point2D.Double)li.next();
      new Dsplpoint(p, this).draw(g);
      //g2.fillOval((int)  p.getX()-3  ,(int) p.getY()-3,6,6);
    }
    drawLabel(g, bounds);
  }
}



