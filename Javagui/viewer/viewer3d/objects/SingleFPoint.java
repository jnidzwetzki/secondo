package viewer.viewer3d.objects;

import sj.lang.ListExpr;

class SingleFPoint{

int x;
int y;
double z;

SingleFPoint(){
  x=0;
  y=0;
  z=1.0;
}



public boolean readFromListExpr(ListExpr LE){
   if (LE==null)
      return false;
   if(LE.listLength()!=3){
      System.out.println("wrong listlength"+LE.listLength());
      return false;
   }
   ListExpr LE1 = LE.first();
   ListExpr LE2 = LE.second();
   ListExpr LE3 = LE.third();
   int x,y;
   double z;

   if(LE1.isAtom() && LE1.atomType()==ListExpr.INT_ATOM)
      x = LE1.intValue();
   else{
      System.out.println("error reading x:");
      return false;
   }

   if(LE2.isAtom() && LE2.atomType()==ListExpr.INT_ATOM)
      y = LE2.intValue();
    else{
      System.out.println("error reading y");
      return false;
    }

    if(LE3.isAtom() && ( LE3.atomType()==ListExpr.INT_ATOM | LE3.atomType()==ListExpr.REAL_ATOM))
       if (LE3.atomType()==ListExpr.INT_ATOM)
          z=LE3.intValue();
       else
          z=LE3.realValue();
    else{
       System.out.println("error reading z");
       return false;
    }

    if(z<0 | z>1){
       System.out.println("wrong z :"+z);
       return false;
    }

    this.x = x;
    this.y = y;
    this.z = z;
    return true;
}

}
