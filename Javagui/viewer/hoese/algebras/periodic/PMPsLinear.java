package viewer.hoese.algebras.periodic;

import viewer.hoese.*;
import sj.lang.ListExpr;
import java.awt.geom.*;
import java.util.*;

public class PMPsLinear extends LinearMove{

  public PMPsLinear(){
    interval = new RelInterval();
    defined = false;
  }

  public String getName(){
    return "pmpoints";
  }


  /** returns a Point2D.Double */
  public Object getObjectAt(Time T){
     if(!defined){
        if(Environment.DEBUG_MODE)
	   System.err.println("PMPLinear.getObjectAt called on an undefined instance");
        return null;
     }
     if(!interval.contains(T)){
        return null;
     }
     double p = interval.where(T);
     Point2D.Double[] res = new Point2D.Double[thePoints.size()];
     Iterator it = thePoints.iterator();
     int pos = 0;
     while(it.hasNext()){
        SingleMove SM = (SingleMove) it.next();
        res[pos] = new Point2D.Double(SM.x1+p*(SM.x2-SM.x1),SM.y1+p*(SM.y2-SM.y1));
	pos++;
     }
     return res;
  }

  protected boolean readStartEnd(ListExpr starts, ListExpr ends){
     if(starts.listLength()!=ends.listLength()){
       if(Environment.DEBUG_MODE)
          System.err.println("PMPLinear.readStartEnd different  ListLengths");
       return false;
     }
     thePoints = new TreeSet();
     isstatic=true;
     bounds = new BBox();
     defined = false;
     while(!starts.isEmpty()){
        ListExpr start = starts.first();
	ListExpr end = ends.first();
	starts = starts.rest();
	ends = ends.rest();
        if(start.listLength()!=2 || end.listLength()!=2){
           if(Environment.DEBUG_MODE)
	     System.err.println("PMPLinear.readStartEnd wrong listlength");
	     thePoints = null;  
	   return false;  
	}

	 Double X1 = LEUtils.readNumeric(start.first());
	 Double Y1 = LEUtils.readNumeric(start.second());
	 Double X2 = LEUtils.readNumeric(end.first());
	 Double Y2 = LEUtils.readNumeric(end.second());
	 if(X1==null || X2==null || Y1==null || Y2 == null){
	   if(Environment.DEBUG_MODE)
	     System.err.println("PMPLinear.readStartEnd : found a non-numeric");
	   return false;
	 }
	 double x1 = X1.doubleValue();
	 double x2 = X2.doubleValue();
	 double y1 = Y1.doubleValue();
	 double y2 = Y2.doubleValue();
	 SingleMove M = new SingleMove(x1,y2,x2,y2);
	 thePoints.add(M);
	 isstatic = isstatic && M.isStatic();
	 bounds.union(x1,x2);
	 bounds.union(y1,y2);
     }
     defined = true;
     return true;
  }

  public BBox getBoundingBox(){
      if(!defined){
         if(Environment.DEBUG_MODE)
	    System.err.println("PMPLinear.getBoundingBox called with an undefined instance");
         return null;
      }
      if(bounds==null & Environment.DEBUG_MODE){
           System.err.println("PMPLinear.getBoundingBox called without a bounding box");
      }
      return bounds;
  }

 private class SingleMove implements Comparable{
  SingleMove(double x1, double y1, double x2, double y2){
      this.x1=x1;
      this.x2=x2;
      this.y1=y1;
      this.y2=y2;
  }
  int compareTo(SingleMove M){
     if(x1<M.x1) return -1;
     if(x1>M.x2) return 1;
     
     if(x2<M.x2) return -1;
     if(x2>M.x2) return 1;
     
     if(y1<M.y1) return -1;
     if(y1>M.y1) return 1;
     
     if(y2<M.y2) return -1;
     if(y2>M.y2) return 1;
     isstatic =(x1==x2 && y1==y2);
     return 0;
  }
  public int compareTo(Object o){
     if(! (o instanceof SingleMove))
        return -1;
     return compareTo((SingleMove)o);	
  }
  boolean isStatic(){ return isstatic;}
  
  private double x1;
  private double x2;
  private double y1;
  private double y2;
  private boolean isstatic;
}
  /*
  The use of TreeSet avoids to store a SingleMove twice.
  */
  private TreeSet thePoints; 
  private BBox bounds;
}
