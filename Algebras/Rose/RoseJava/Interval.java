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

class Interval {
  //supportive class for class SetOps
  //'right' has to be greater than or equal to 'left'
  
  //members
    Rational left;// = new Rational(0);
    Rational right;// = new Rational(0);
    //boolean leftopen;
    //boolean rightopen;
    String mark;
    Rational x;// = new Rational(0);
    Element ref;
    int number;
  //Triangle reftri;
  
  //constructors
  Interval() {
    this.left = RationalFactory.constRational(0);
    this.right = RationalFactory.constRational(0);
    this.mark = "";
    this.x = RationalFactory.constRational(0);
    this.number = -1;
    //this.reftri = new Triangle();
  }
  
  Interval(Rational l, Rational r, String m, Rational x, Element e, int n) {
    if (l.lessOrEqual(r)) {
      this.left = l.copy();
      this.right = r.copy();
      this.mark = m;
      this.x = x.copy();
      this.ref = e;//.copy()
      this.number = n;
      //this.reftri = t;
    }//if
    else { System.out.println("Error: Couldn't create interval."); }
  }//end constructor
  
  //methods
  Interval copy () {
    //returns a copy of this.object
    return (new Interval(this.left.copy(),this.right.copy(),this.mark,this.x.copy(),(Element)this.ref,this.number));
  }//end constructor
  
  public boolean equal(Interval in) {
    //returns true if this.object and 'in' are equal
    //false else
    //attention: checks only for the interval an NOT for other data
    
    if ((this.left.equal(in.left)) &&
	(this.right.equal(in.right))) {
      return true;
    }//if
    else { return false; }
  }//end method equal

    public boolean equalX(Interval in) {
	//returns true if both object's interval borders AND their
	//x-coordinate are equal
	//false else
	if (this.left.equal(in.left) &&
	    this.right.equal(in.right) &&
	    this.x.equal(in.x)) return true;
	else return false;
    }//end method equalX

    protected void print() {
	//prints the object's data
	System.out.print("Interval:");
	System.out.print(" down: "+this.left.toString());
	System.out.print(" top: "+this.right.toString());
	System.out.print(" mark: "+this.mark);
	System.out.print(" x: "+this.x.toString());
	System.out.println(" number: "+this.number);
	//System.out.println(" ref: ");
	//this.ref.print();
    }//end method print
  

    protected byte comp(Interval in) {
	//returns 0 if this and in are equal
	//returns -1 if x of this is smaller
	//returns +1 if x of this is greater
	//if x is equal, left, right are compared in this order
	if (this.x.less(in.x)) { return -1; }
	if (this.x.greater(in.x)) { return 1; }
	//x must be equal
	if (this.left.less(in.left)) { return -1; }
	if (this.left.greater(in.left)) { return 1; }
	//left must be equal
	if (this.right.less(in.right)) { return -1; }
	if (this.right.greater(in.right)) { return 1; }
	//right must be equal
	//byte erg = this.ref.compX(in.ref);
	//if (erg == 0) { erg = this.ref.compY(in.ref); }
	return 0;
    }//end method comp


}//end class Interval
