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

//import java.util.LinkedList;
import java.util.*;
import java.lang.reflect.*;
import java.io.*;

class Polygons extends Element implements Serializable {
    
    //members
    private TriList trilist;//list of triangles forming the polygons
    public boolean perimeterDefined;
    private double perimeter;
    public boolean areaDefined;
    private double area;
    public boolean borderDefined;
    private SegList border; //REMOVE THIS!!!
    public boolean bboxDefined;
    private Rect bbox;//bounding box
    
    //constructors
    public Polygons() {
	//fill in dummy values
	this.trilist = new TriList();
	this.perimeter = -1;
	this.area = -1;
	this.border = new SegList();
	this.bbox = new Rect();
	this.perimeterDefined = false;
	this.areaDefined = false;
	this.borderDefined = false;
	this.bboxDefined = false;
    }
    
    public Polygons(TriList tl) {
	this.trilist = (TriList)tl.copy();
	//this.area = computeArea();
	this.area = -1;
	//this.border = computeBorder();
	this.border = new SegList();
	//this.perimeter = computePerimeter();
	this.perimeter = -1;
	//computeBbox();
	this.bbox = new Rect();
	
	this.perimeterDefined = false;
	this.areaDefined = false;
	this.borderDefined = false;
	this.bboxDefined = false;
    }
    
    public Polygons(SegList sl) {
	this.border = (SegList)sl.copy();
	this.trilist = computeTriangles(sl);
	//this.area = computeArea();
	this.area = -1;
	//this.perimeter = computePerimeter();
	this.perimeter = -1;
	//computeBbox();
	this.bbox = new Rect();

	this.perimeterDefined = false;
	this.areaDefined = false;
	this.borderDefined = true;
	this.bboxDefined = false;
    }
    
    //methods
    private double computeArea(){
	//coumputes the area of the polygons
	double sum = 0;
	Triangle t = new Triangle();
	for (int i = 0; i < this.trilist.size(); i++) {
	    t = (Triangle)this.trilist.get(i);
	    sum = sum + t.area();
	}//for
	return sum;
    }//end method area


    private void computeBbox() {
	//computes and sets bbox
	//CAUTION: untested method
	//AS EVERY NEW RECT METHOD IN THE ALGEBRA!!!
	if (this.trilist.isEmpty()) { }
	else {
	    Rational leftmost = ((Triangle)trilist.getFirst()).rect().ul.x;
	    Rational upmost = ((Triangle)trilist.getFirst()).rect().ul.y;
	    Rational rightmost = ((Triangle)trilist.getFirst()).rect().lr.x;
	    Rational downmost = ((Triangle)trilist.getFirst()).rect().lr.y;
	    
	    for (int i = 1; i < this.trilist.size(); i++) {
		Rect actrect = ((Triangle)trilist.get(i)).rect();
		if (actrect.ul.x.less(leftmost)) {
		    leftmost = actrect.ul.x; }
		if (actrect.ul.y.greater(upmost)) {
		    upmost = actrect.ul.y; }
		if (actrect.lr.x.greater(rightmost)) {
		    rightmost = actrect.lr.x; }
		if (actrect.lr.y.less(downmost)) {
		    downmost = actrect.lr.y; }
	    }//for i
	    
	    this.bbox = new Rect(leftmost,upmost,rightmost,downmost);
	}//else
    }//end method computeBbox

    
    public Rect rect() {
	//returns the bounding box of this.object
	if (!this.bboxDefined) {
	    computeBbox();
	    this.bboxDefined = true;
	}//if
	return bbox.copy();
    }//end method rect

  
    private double computePerimeter(){
	//computes the perimeter of the polygons
	double sum = 0;
	Segment s = new Segment();
	for (int i = 0; i < this.border.size(); i++){
	    s = (Segment)this.border.get(i);
	    sum = sum + s.length();
	}//for
	return sum;
    }//end method computePerimeter

    private SegList computeBorder(){
	//computes the border of the polygons and returns a SegList of border segments
	//CAUTION: we use the operation from Algebra.java here!
	
	return this.border = ROSEAlgebra.contour(this.trilist,true,true);
    }//end method computeBorder
    

    public static TriList computeTriangles(SegList border){
	//computes the triangulation of the polygons using a sweep line algo
	//caution: it is not checked wether the border of the polygons
	//builds cycles!!!
	//BUT: this now can be done and removed(!) by the new GRAPH method
	//for finding BCC!!! Try it!
	//
	//CAUTION: A method named sortBorder is used in here. It is probably O(n�) and should
	//be replaced.
	
	//System.out.println("\nentering Pol.computeTriangles...");
	
	if (border.isEmpty()) return new TriList();

	TriList l = new TriList();
	PointList borderVerts = new PointList(); //vertices of polygon border
	PointList xstruct = new PointList(); //x-structure for sweep line algo
	TriList tl = new TriList(); //store the generated triangles in here
	
	//construct borderVerts from this.border
	Point new1 = new Point();
	Point new2 = new Point();
	boolean isnew1 = true;
	boolean isnew2 = true;
	
	Iterator it = border.listIterator(0);
	while (it.hasNext()) {
	    Segment actSeg = (Segment)it.next();
	    borderVerts.add(actSeg.startpoint);
	    borderVerts.add(actSeg.endpoint);
	}//while
	
	//System.out.println("borderVerts("+borderVerts.size()+"):"); borderVerts.print();
	borderVerts = PointList.convert(SetOps.rdup(borderVerts));
	//System.out.println("rdup(borderVerts("+borderVerts.size()+"):"); borderVerts.print();
	
	//sort borderVerts
	//caution: this cannot be done by a simple mergesortX
	//because the sorting regarding Y must be reverse!
	
	SetOps.mergesortXY(borderVerts);
	xstruct = (PointList)borderVerts;
	//xstruct = (PointList)borderVerts.copy();

	//borderVerts.print();
	//System.exit(0);

	//compute sorted border list
	SegList sortedBorder = sortBorder(border,(Point)xstruct.getFirst());

    //
    //DO THE SWEEP!!!
    //
    Point x = new Point();
    boolean first = false;
    Segment found1 = new Segment();
    Segment found2 = new Segment();
    Segment seg1 = new Segment();
    Segment seg2 = new Segment();
    int pCUp = 0;
    int pCDown = 0;
    int yElemPos = 0;
    SweepStElem newElem = new SweepStElem();
    Rational angle = RationalFactory.constRational(0);
    PointList delList = new PointList();
    LinkedList delListInt = new LinkedList();
    String attribute = "";

    //init y-structure
    //ystruct should be organized that way that the element with the lowest y-value
    // is first
    LinkedList ystruct = new LinkedList();
    Segment actSeg = new Segment();
    
    //sweep starts
    //System.out.print("Pol.compTris:triangulating size:"+xstruct.size());
    for (int i = 0; i < xstruct.size(); i++) {
	//System.out.print(".");//just to see that something is done
	//System.out.println("processing point: "+i);
      //check attribute of current Point
      x = (Point)xstruct.get(i);
      //System.out.println("  -> check");
      attribute = attribute(x,sortedBorder);
      //System.out.println("\n*****************************************************");
      //System.out.println("process ("+x.x+","+x.y+") --> attribute: "+attribute+", i:"+i);
      /*
      System.out.println("ystruct:");
      for (int ys = 0; ys < ystruct.size(); ys++) ((SweepStElem)ystruct.get(ys)).pointChain.print();
      System.out.println();
      */
      if (attribute == "start") {
	  
	  //find the corresponding segments seg1, seg2
	  first = false;

	it = border.listIterator(0);
	while (it.hasNext()) {
	    actSeg = (Segment)it.next();
	    if (PointSeg_Ops.isEndpoint(x,actSeg)) {
		if (!first) {
		    found1 = actSeg;
		    first = true;
		}//if
		else {
		    found2 = actSeg;
		    break;
		}//else
	    }//if
	}//while
	
	//build seg1,seg2 from found1,found2
	//such that seg1 is the "lower element" and x is its endpoint
	//and seg2 is the "higher element" and x is its startpoint
	
	//what are the two points which are different from x?
	Point pd1 = new Point();
	Point pd2 = new Point();

	if ((found1.getStartpoint()).equal(x)) {
	    pd1 = found1.getEndpoint(); }
	else { pd1 = found1.getStartpoint(); }
	if ((found2.getStartpoint()).equal(x)) {
	    pd2 = found2.getEndpoint(); }
	else { pd2 = found2.getStartpoint(); }
      
	byte compare = Mathset.pointPosition(x,pd1,pd2);


	//System.out.println("pd1:"); pd1.print();
	//System.out.println("pd2:"); pd2.print();
	//System.out.println("compare: "+compare);
	boolean pd1first = false;

	//now build the segments
	if (compare == -1) {
	    pd1first = true;
	    //seg1 = new Segment(pd2,x);
	    //seg1 = new Segment(pd1,x);
	    //seg2 = new Segment(x,pd1);
	    //seg2 = new Segment(x,pd2);
	}//if
	else if (compare == 1) {
	    //seg1 = new Segment(pd2,x);
	    //seg1 = new Segment(pd1,x);
	    //seg2 = new Segment(x,pd1);
	    //seg2 = new Segment(x,pd2);
	}//if
	else if (compare == 0) {
	    /*
	    if (pd1.y.greater(pd2.y)) {
		seg1 = new Segment(pd2,x);
		seg2 = new Segment(x,pd1);
	    }//if
	    else {
		seg1 = new Segment(pd1,x);
		seg2 = new Segment(x,pd2);
	    }//else
	    */
	    
	    System.out.println("Error in Polygons.computeTriangles: a vertex was identified as 'start' but actually is not a start vertex!");
	    System.exit(0);
	    
	    
	}//else	   
	//System.out.println("seg1:"); seg1.print();
	//System.out.println("seg2:"); seg2.print();
	
	//build three new sweepStructElements and
	//insert them in ystruct in the proper position
	SweepStElem top = new SweepStElem();
	top.is_top = true;
	SweepStElem bottom = new SweepStElem();
	bottom.is_bottom = true;
	SweepStElem start = new SweepStElem();
	if (pd1first) {
	    start.pointChain.add(pd1);
	    start.pointChain.add(x);
	    start.pointChain.add(pd2);
	}//if
	else {
	    start.pointChain.add(pd2);
	    start.pointChain.add(x);
	    start.pointChain.add(pd1);
	}//else

	start.rimoEl = (Point)x.copy();
	start.in = true;
	SweepStElem mt = new SweepStElem();
	
	//find the proper position in ystruct to insert the new elements
	if (ystruct.isEmpty()) {
	    //System.out.println("  ystruct is empty, so insert new elements and finish");
	  ystruct.add(bottom);
	  ystruct.add(start);
	  ystruct.add(top);
	}//if
	else {
	    //yElemPos = interval(x, ystruct);
	    yElemPos = interval3(x, ystruct);
	    //System.out.println("comp: interval:"+interval(x,ystruct)+" - interval2:"+interval2(x,ystruct)+" - interval3:"+interval3(x,ystruct)+" --> "+yElemPos);
	    /*
	    if (yElemPos != interval3(x,ystruct)) {
		System.out.println("P.interval3 doesn't work properly:");
		System.out.println("x:"); x.print();
		System.out.println("attribute: "+attribute);
		System.out.println("ystruct:");
		for (int ys = 0; ys < ystruct.size(); ys++) ((SweepStElem)ystruct.get(ys)).pointChain.print();
		System.out.println();
		System.out.println("comp: interval:"+interval(x,ystruct)+" - interval2:"+interval2(x,ystruct)+" - interval3:"+interval3(x,ystruct)+" --> "+yElemPos);
		System.exit(0);
	    }//if
	    */
	  SweepStElem yElem = (SweepStElem)ystruct.get(yElemPos);
	  if (yElem.pointChain.isEmpty()) {
	    //x lies in an "out" interval of ystruct
	    //insert the elements
	    //System.out.println(" yElem.pointChain is empty, so insert new elements and finish");
	      /*
	      //check and resort pointChain if necessary
	      checkAndResortPointChain(start.pointChain);
	      */
	    ystruct.add(yElemPos, mt);
	    ystruct.add(yElemPos+1, start);
	    ystruct.add(yElemPos+2, mt);
	  }//if
	  else {
	    //x lies in an "in" interval of ystruct
	    //System.out.println("  yElem.pointChain is not empty - compute new triangles and split pointChain");
	    //add triangulation segments to tl
	    //sl.add(new Segment("", x, yElem.rimoEl));//add triangulation segment
	    Point lastP = (Point)yElem.rimoEl.copy();
	    //System.out.println("  lastP: ("+lastP.x+","+lastP.y+")");
	    //compute index of lastP
	    int indexLP = -1;
	    
	    for (int j = 0; j < yElem.pointChain.size(); j++) {
		if (((Point)yElem.pointChain.get(j)).equal(lastP)) {
		    indexLP = j;
		    break;
		}//if
	    }//for j
	    //System.out.println(" indexLP: "+indexLP);
	    if (indexLP == -1) {
	      System.out.println(" FATAL ERROR! No rimoEl in pointChain.");
	      System.exit(0);
	    }//if
	    //System.out.println("  indexOf(lastP): "+indexLP);
	    //set pCUp,pCDown in case the loops aren't entered
	    pCDown = indexLP;
	    pCUp = indexLP;
	    //System.out.println(" pointChain.size(): "+yElem.pointChain.size());
	    for (int j = indexLP; j > 1; j--) {
	      boolean compute = false;
	      if (Mathset.pointPosition(x,(Point)yElem.pointChain.get(j-1),(Point)yElem.pointChain.get(j)) == 1) { compute = true; }
	      //System.out.println(" compute(down): "+compute);
	      if (compute) {
		  Triangle nt = new Triangle(x,
					     (Point)yElem.pointChain.get(j-1),
					     (Point)yElem.pointChain.get(j));
		  tl.add(nt);
		  //System.out.println("generated triangle:"); nt.print();
		  //nt.print();
		pCDown = j-1;//memorize the last valid position
	      }//if
	    }//for j
	    //System.out.println("  indexOf(lastP): "+indexLP);
	    for (int j = indexLP; j < yElem.pointChain.size()-2; j++) {
	      boolean compute = false;
	      if (Mathset.pointPosition(x,(Point)yElem.pointChain.get(j+1),(Point)yElem.pointChain.get(j)) == -1) {compute = true; }
	      //System.out.println(" compute(up):"+compute);
	      if (compute) {
		  Triangle nt = new Triangle(x,
					     (Point)yElem.pointChain.get(j+1),
					     (Point)yElem.pointChain.get(j));
		  tl.add(nt);
		  //System.out.println("generated triangle:"); nt.print();
		  //nt.print();
		pCUp = j+1;//memorize the last valid position
	      }//if
	    }//for j

	    //update ystruct
	    //build two new SweepStElems and insert them in ystruct
	    
	    //find the two neighbour points for x
	    PointList xNb1 = new PointList();
	    PointList xNb2 = new PointList();

	    first = false;
	    for (int j = 0; j < border.size(); j++) {
		if ((((Segment)border.get(j)).getStartpoint()).equal(x) ||
		    (((Segment)border.get(j)).getEndpoint()).equal(x)) {
		    if (!first) {
			//xNb1 = ((Segment)this.border.get(j)).get_vertices();
			xNb1 = ((Segment)border.get(j)).endpoints();
			first = true;
		    }//if
		    else {
			//xNb2 = ((Segment)this.border.get(j)).get_vertices();
			xNb2 = ((Segment)border.get(j)).endpoints();
			break;
		    }//else
		}//if
	    }//for
	    
	    //extract the desired point from segments
	    Point tnb1 = (Point)((Point)xNb1.get(0)).copy();
	    Point tnb2 = (Point)((Point)xNb1.get(1)).copy();
	    Point tnb3 = (Point)((Point)xNb2.get(0)).copy();
	    Point tnb4 = (Point)((Point)xNb2.get(1)).copy();
	    Point nb1 = new Point();
	    Point nb2 = new Point();
	    
	    if (((Point)tnb1).equal(x)) { nb1 = (Point)tnb2.copy(); }
	    else { nb1 = (Point)tnb1.copy(); }
	    if (((Point)tnb3).equal(x)) { nb2 = (Point)tnb4.copy(); }
	    else { nb2 = (Point)tnb3.copy(); }
	    //swap if nb1.y > nb2.y
	    /*
	      System.out.println(" (pre-swap): nb1("+nb1.x+","+nb1.y+")"
	      +" nb2("+nb2.x+","+nb2.y+")");
	    */
	    if (Mathset.pointPosition(x,nb1,nb2) == -1) {
		//System.out.println(" swapped!");
	      Point nb3 = (Point)nb1.copy();
	      nb1 = (Point)nb2.copy();
	      nb2 = (Point)nb3.copy();
	    }//if
	      
	    
	    //build lower element
	    //find the proper order for the elements and run the for-loop
	    //System.out.println(" pCUp: "+pCUp+"  pCDown: "+pCDown);
	    SweepStElem lower = new SweepStElem();
	    if (SegList.pos(border,(new Segment(x,nb1))) > -1) {
	      for (int j = pCDown; j > -1; j--) {
		lower.pointChain.addFirst(((Point)yElem.pointChain.get(j)).copy());
	      }//for j
	    }//if
	    else {
	      for (int j = pCDown; j > -1; j--) {
		lower.pointChain.add(((Point)yElem.pointChain.get(j)).copy());
	      }//for j
	    }//else
	    lower.pointChain.add((Point)x.copy());
	    lower.pointChain.add((Point)nb2.copy());
	    lower.rimoEl = (Point)x.copy();

	    //build upper element
	    SweepStElem upper = new SweepStElem();
	    if (SegList.pos(border,(new Segment(x,nb2))) > -1) {
	      for (int j = pCUp; j < yElem.pointChain.size(); j++) {
		upper.pointChain.add(((Point)yElem.pointChain.get(j)).copy());
	      }//for j
	    }//if
	    else {
	      for (int j = pCUp; j < yElem.pointChain.size(); j++) {
		upper.pointChain.addFirst(((Point)yElem.pointChain.get(j)).copy());
	      }//for
	    }//else
	    upper.pointChain.addFirst((Point)x.copy());
	    upper.pointChain.addFirst((Point)nb1.copy());
	    upper.rimoEl = (Point)x.copy();
	    /*
	    //check and resort elements if necessary
	    checkAndResortPointChain(upper.pointChain);
	    checkAndResortPointChain(lower.pointChain);
	    */
	    //insert elements in ystruct
	    ystruct.set(yElemPos, lower);//replace "in"-element
	    ystruct.add(yElemPos+1, mt);
	    ystruct.add(yElemPos+2, upper);

	  }//else
	}//else
      }//if
      //now all operations for attribute=="start" are done
	
      else {

	if (attribute == "bend") {
	  //find corresponding SweepStElem
	  yElemPos = interval(x, ystruct);
	  //System.out.println("comp: interval:"+interval(x,ystruct)+" - interval2:"+interval2(x,ystruct)+" - interval3:"+interval(x,ystruct)+" --> "+yElemPos);
	  
	  if (yElemPos != interval3(x,ystruct)) {
	      System.out.println("Warning: P.interval3 might not work properly:");
	      System.out.println("x: "); x.print();
	      System.out.println("attribute: "+attribute);
	      
		System.out.println("ystruct:");
		for (int ys = 0; ys < ystruct.size(); ys++) ((SweepStElem)ystruct.get(ys)).pointChain.print();
		System.out.println();
		System.out.println("comp: interval:"+interval(x,ystruct)+" - interval2:"+interval2(x,ystruct)+" - interval3:"+interval3(x,ystruct)+" --> "+yElemPos);
	      //System.exit(0);
	      
	  }//if
	  
	  //System.out.println("yElemPos:"+yElemPos);
	  SweepStElem yElem = (SweepStElem)ystruct.get(yElemPos);

	  //update ystruct
	  //add the "following" element of x to pointChain
	  //find the two neighbour points for x
	  PointList xNb1 = new PointList();
	  PointList xNb2 = new PointList();
	  Point xFoll = new Point();

	  first = false;
	  
	  it = border.listIterator(0);
	  while (it.hasNext()) {
	      actSeg = (Segment)it.next();
	      if (PointSeg_Ops.isEndpoint(x,actSeg)) {
		  if (!first) {
		      xNb1 = actSeg.endpoints();
		      first = true;
		  }//if
		  else {
		      xNb2 = actSeg.endpoints();
		      break;
		  }//else
	      }//if
	  }//while
	 
	  //System.out.println(" choose from ("+xNb1[0].x+","+xNb1[0].y+"), ("+xNb1[1].x+","+xNb1[1].y+"), ("+xNb2[0].x+","+xNb2[0].y+"), ("+xNb2[1].x+","+xNb2[1].y+")");

	  boolean found = false;
	  //extract the desired point from segments
	  Point tnp1 = (Point)((Point)xNb1.get(0)).copy();
	  Point tnp2 = (Point)((Point)xNb1.get(1)).copy();
	  Point tnp3 = (Point)((Point)xNb2.get(0)).copy();
	  Point tnp4 = (Point)((Point)xNb2.get(1)).copy();

	  Point np1 = new Point();
	  Point np2 = new Point();

	  if (tnp1.equal(x)) { np1 = (Point)tnp2.copy(); }
	  else { np1 = (Point)tnp1.copy(); }
	  if (tnp3.equal(x)) { np2 = (Point)tnp4.copy(); }
	  else { np2 = (Point)tnp3.copy(); }

	  if (x.equal((Point)yElem.pointChain.getFirst())) {
	      if (np1.equal((Point)yElem.pointChain.get(1)))
		  {
		      xFoll = (Point)np2.copy();
		      found = true;
		  }//if
	      else {
		  xFoll = (Point)np1.copy();
		  found = true;
	      }//else
	  }//if
	  if (x.equal((Point)yElem.pointChain.getLast())) {
	      if (np1.equal((Point)yElem.pointChain.get(yElem.pointChain.size()-2))) {
		  xFoll = (Point)np2.copy();
		  found = true;
	      }//if
	      else {
		  xFoll = (Point)np1.copy();
		  found = true;
	      }//else
	  }//if
	  
	  if (!found) {
	    System.out.println("Polygons.computeTriangles: FATAL ERROR! Point from xstruct was not found in ystruct!");
	    System.out.println("ystruct:");
	    for (int ys = 0; ys < ystruct.size(); ys++) {
		((SweepStElem)ystruct.get(ys)).pointChain.print(); }
	    System.out.println();
	    System.out.println("x:");
	    x.print();
	    System.exit(0);
	  }
	  //else { System.out.println(" added P("+xFoll.x+","+xFoll.y+") to pointChain"); }
	  
	  
	  //add xFoll to pointChain
	  if (x.equal((Point)yElem.pointChain.getFirst())) {
	      yElem.pointChain.addFirst(xFoll.copy());
	  }//if
	  else { yElem.pointChain.addLast(xFoll.copy()); }
	  yElem.rimoEl = (Point)x.copy();
	  //now ystruct is updated

	  //compute triangles
	  //if there's only one element to check then don't compute triangles:
	  boolean xposup = false;
	  int up = 0;
	  int down = 0;

	  if (yElem.pointChain.size() > 4) {

	      //find out, on which end of pointChain x lies
	      if (x.equal((Point)yElem.pointChain.get(yElem.pointChain.size()-2))){
		  up = yElem.pointChain.size()-3;
		  down = 1;
		  xposup = true;//x is at the upper end of the pointChain(end)
		  //System.out.println(" x is at the upper end of the pointChain(end)");
	      }//if
	      else {
		  up = yElem.pointChain.size()-2;
		  down = 2;
		  xposup = false;//x is at the lower end of the pointChain(beginning)
		  //System.out.println(" x is at the lower end of the pointChain(beginning)");
	      }//else

	      //find out whether x is (in respect to the y value) on the upper side or 
	      //the lower side of the interval
	      //this must be done using the sweepline
	      
	      boolean xIsUpper = false;
	      Segment intSeg;
	      if (xposup) {
		  intSeg = new Segment((Point)yElem.pointChain.get(1),
					       (Point)yElem.pointChain.get(0));
	      }//if
	      else {
		  intSeg = new Segment((Point)yElem.pointChain.getLast(),
					       (Point)yElem.pointChain.get(yElem.pointChain.size()-2));
	      }//else
	      Rational minY,maxY;
	      if (intSeg.startpoint.compY(intSeg.endpoint) == -1) {
		  minY = intSeg.startpoint.y;
		  maxY = intSeg.endpoint.y;
	      }//if
	      else {
		  minY = intSeg.endpoint.y;
		  maxY = intSeg.startpoint.y;
	      }//if
	      Segment sweepline = new Segment(x.x,minY.minus(1),x.x,maxY.plus(1));
	      //System.out.println("sweepline construction[1]");
	      Point intPoint = sweepline.intersection(intSeg);
	      if (intPoint.compY(x) == -1) xIsUpper = true;
	      else xIsUpper = false;
	      
	      //System.out.println("xposup: "+xposup+", xIsUpper: "+xIsUpper);
	      
	      if (xposup) {
		  for (int j = up; j > down; j--) {
		      //System.out.println(" entered for-loop; j:"+j);
		      boolean compute = false;
		      //System.out.println(" check the triangle(1): ");
		      //(new Triangle(x,(Point)yElem.pointChain.get(j-1),(Point)yElem.pointChain.get(j))).print();
		      if (xIsUpper) {
			  if (Mathset.pointPosition(x,(Point)yElem.pointChain.get(j-1),(Point)yElem.pointChain.get(j)) == 1) compute = true; }
		      else {
			  if (Mathset.pointPosition(x,(Point)yElem.pointChain.get(j-1),(Point)yElem.pointChain.get(j)) == -1) compute = true; }
		      if (compute) {
			  Triangle nt = new Triangle(x,
						     (Point)yElem.pointChain.get(j-1),
						     (Point)yElem.pointChain.get(j));
			  tl.add(nt);
			  //System.out.println("generated triangle(1):"); nt.print();
			  //nt.print();
			  //now add points to delList
			  delList.add(yElem.pointChain.get(j));
			  delListInt.add(new Integer(j));
			  //if (!(((Point)yElem.pointChain.get(j)).equal((Point)yElem.pointChain.get(yElem.pointChain.size()-2)))) { delList.add(yElem.pointChain.get(j)); }
		      }//if
		      else { break; }
	      }//for j
	    }// if
	    else {
		for (int j = down; j < up; j++) {
		    //System.out.println(" entered for-loop; j: "+j);
		    boolean compute = false;
		    //System.out.println(" check the triangle(2): ");
		    //(new Triangle(x,(Point)yElem.pointChain.get(j+1),(Point)yElem.pointChain.get(j))).print();
		    if (xIsUpper) {
			if (Mathset.pointPosition(x,(Point)yElem.pointChain.get(j+1),(Point)yElem.pointChain.get(j)) == 1) compute = true; }
		    else {
			if (Mathset.pointPosition(x,(Point)yElem.pointChain.get(j+1),(Point)yElem.pointChain.get(j)) == -1) compute = true; }
		    //System.out.println(" compute: "+compute);
		    if (compute) {
			Triangle nt = new Triangle(x,
						   (Point)yElem.pointChain.get(j+1),
						   (Point)yElem.pointChain.get(j));
			tl.add(nt);
			//System.out.println("generated triangle(2):"); nt.print();
			//nt.print();
			//now add points to delList
			delListInt.add(new Integer(j));
			delList.add(yElem.pointChain.get(j));
			//if (!(((Point)yElem.pointChain.get(j)).equal((Point)yElem.pointChain.get(yElem.pointChain.size()-2)))) { delList.add(yElem.pointChain.get(j)); }
			//System.out.println(" j: "+j);
		    }//if
		    else { break; }
		}//for
	    }//else

	  }//if	     

	  //finally remove the points in delList from yElem
	  //System.out.println(" delete "+delList.size()+" point(s) from pointChain");
	  //System.out.println(" delList:"); delList.print();
	  /*
	  //bugfix: there is an error with one of the points is in delList that
	  //mustn't be there. We'll remove it at this point from delList
	  if (xposup) {
	      int dpos = delList.contains((Point)yElem.pointChain.get(1));
	      if (dpos > -1) {
		  //System.out.println("### don't delete this one ###");
		  delList.remove(dpos);
	      }//if
	  }//if
	  else {
	      int dpos = delList.contains((Point)yElem.pointChain.get(yElem.pointChain.size()-2));
	      if (dpos > -1) {
		  //System.out.println("### don't delete this one ###");
		  delList.remove(dpos);
	      }//if
	  }//else
	  */
	  
	  //sort delListInt
	  for (int si = 0; si < delListInt.size()-1; si++) {
	      Integer actIntI = (Integer)delListInt.get(si);
	      for (int sj = si; sj < delListInt.size(); sj++) {
		  Integer actIntJ = (Integer)delListInt.get(sj);
		  if (actIntI.compareTo(actIntJ) < 0) {
		      Integer swap = actIntI;
		      delListInt.set(si,actIntJ);
		      delListInt.set(sj,swap);
		  }//if
	      }//for sj
	  }//for si
	  
	  /*
	  System.out.println("delListInt:");
	  for (int j = 0; j < delListInt.size(); j++) {
	      System.out.print("["+((Integer)delListInt.get(j)).intValue()+"] "); }
	  System.out.println();
	  */
	  
	  //now delete elements
	  for (int j = 0; j < delListInt.size(); j++) {
	      yElem.pointChain.remove(((Integer)delListInt.get(j)).intValue());
	  }//for j
	  delListInt.clear();
	  delList.clear();
	  
	  /*
	  //System.out.println("\ndeletelist:"); delList.print();
	  for (int j = 0; j < delList.size(); j++) {
	    found = false;
	    //System.out.print("  searching for ("+((Point)delList.get(j)).x+","+((Point)delList.get(j)).y+")... ");
	    for (int k = 0; k < yElem.pointChain.size(); k++) {
		if (((Point)yElem.pointChain.get(k)).equal((Point)delList.get(j))) {
		    yElem.pointChain.remove(k);
		    //System.out.println("deleted");
		    found = true;
		    break;
		}//if
		//else { System.out.println("  not found: ("+((Point)delList.get(j)).x+", "+((Point)delList.get(j)).y+")"); }
	    }//for k
	    if (!found) { //System.out.println("not found!"); 
	    }
	  }//for j
	  delList = new PointList();
	  */
	  
	  //It may happen that a point with attribute "bend" is part
	  //of two yElems. Here usually the first one is taken and the
	  //triangles are computed. We need to check wether we have that
	  //case here and combine both pointChains of that two yElems.
	  //Up to now we observed only one case in which this should
	  //be checked and this will be fixed here.
	  //System.out.println("\nwork on special case...");

	  if (ystruct.size() > yElemPos+3) {
	      SweepStElem current = yElem;
	      SweepStElem next = (SweepStElem)ystruct.get(yElemPos+2);//there is an mt element between them
	      //System.out.println("current:"); current.pointChain.print();
	      //System.out.println("next:"); next.pointChain.print();

	      Point currentLast = (Point)current.pointChain.getLast();
	      Point currentLastButOne = (Point)current.pointChain.get(current.pointChain.size()-2);
	      Point nextFirst = (Point)next.pointChain.getFirst();
	      Point nextSecond = (Point)next.pointChain.get(1);
	      if(currentLast.equal(nextSecond) &&
		 currentLastButOne.equal(nextFirst)) {
		  //now we have exactly that case
		  //System.out.println("### combine the two pointChains!!! ###");
		  //add points of next to current
		  for (int ad = 2; ad < next.pointChain.size(); ad++) {
		      current.pointChain.add(((Point)next.pointChain.get(ad)).copy());
		  }//for ad
		  
		  //remove yElems
		  ystruct.remove(yElemPos+2);
		  ystruct.remove(yElemPos+1); //remove mt element
	      }//if
	  }//if
	  
	}//if			
	

	//now all operations for attribute=="bend" are done


	else {

	  if (attribute == "end") {
	    //find the corresponding segments seg1, seg2
	    first = false;
	    
	    it = border.listIterator(0);
	    while (it.hasNext()) {
		actSeg = (Segment)it.next();
		if (!first) {
		    found1 = (Segment)actSeg.copy();
		    first = true;
		}//if
		else {
		    found2 = (Segment)actSeg.copy();
		    break;
		}//else
	    }//while

	    //find the proper position in ystruct
	    yElemPos = interval(x, ystruct);
	    //yElemPos = interval2(x, ystruct);
	    //System.out.println("comp: interval:"+interval(x,ystruct)+" - interval2:"+interval2(x,ystruct)+" - interval3:"+interval3(x,ystruct)+" --> "+yElemPos);
	    /*
	    if (yElemPos != interval3(x,ystruct)) {
		System.out.println("P.interval3 doesn't work properly:");
		System.out.println("x:"); x.print();
		System.out.println("attribute: "+attribute);
		System.out.println("ystruct:");
		for (int ys = 0; ys < ystruct.size(); ys++) ((SweepStElem)ystruct.get(ys)).pointChain.print();
		System.out.println();
		System.out.println("comp: interval:"+interval(x,ystruct)+" - interval2:"+interval2(x,ystruct)+" - interval3:"+interval3(x,ystruct)+" --> "+yElemPos);
		System.exit(0);
	    }//if
	    */
	    SweepStElem yElem = (SweepStElem)ystruct.get(yElemPos);
	    //determine whether x really lies in the interval and not only on the border

	    if (!yElem.pointChain.isEmpty()) {
		if (x.equal((Point)yElem.pointChain.getFirst()) &&
		    x.equal((Point)yElem.pointChain.getLast())) {
		    //x lies on the left and on the right border
		    //leftright = true;
		}//if
		else {
		    if (x.equal((Point)yElem.pointChain.getFirst())) {
			//x lies on the left border
			//System.out.println(" x lies on left border");
			yElemPos--;
			yElem = (SweepStElem)ystruct.get(yElemPos);
		    }//if
		    else {
			if (x.equal((Point)yElem.pointChain.getLast())) {
			    //x lies on the right border
			    //System.out.println(" x lies on right border");
			    yElemPos++;
			    yElem = (SweepStElem)ystruct.get(yElemPos);
			}//if
		    }//else
		}//else
	    }//if
	    
	    //System.out.println("\nyElemPos:"+yElemPos);
	    if (yElem.pointChain.isEmpty()) {
	      //x lies in an "out" interval of ystruct
	      //compute triangulation segments
	      //System.out.println(" x lies in an 'out'-interval");
	      //System.out.println(" yElemPos: "+yElemPos);
	      //System.out.println(" actual ystruct:");
	      //for (int ys = 0; ys < ystruct.size(); ys++) {
		//  ((SweepStElem)ystruct.get(ys)).pointChain.print(); }

		//find out on which border of the neighbour swstelems the
		//point lies and set prev,foll
		SweepStElem prev = (SweepStElem)ystruct.get(yElemPos-1);
		SweepStElem foll = (SweepStElem)ystruct.get(yElemPos+1);
		boolean prevLiesTop = false;
		boolean follLiesTop = false;
		boolean computedAnyPrev = false;
		boolean computedAnyFoll = false;
		if (((Point)prev.pointChain.getFirst()).equal(x)) { prevLiesTop = true; }
		if (((Point)foll.pointChain.getFirst()).equal(x)) { follLiesTop = true; }
		//System.out.println("prevLiesTop:"+prevLiesTop+", follLiesTop:"+follLiesTop);

		//depending on prevLiesTop,follLiesTop the pointChains are
		//parsed from beginning to end or reverse

		int pCprev = -1;
		int pCfoll = -1;
		if (prevLiesTop) { pCprev = 1; }
		else { pCprev = prev.pointChain.size()-2; }
		if (follLiesTop) { pCfoll = 1; }
		else { pCfoll = foll.pointChain.size()-2; }
		
		//compute triangles for prev
		if (prevLiesTop) {
		    //Trying to handle a special case by substituting this:
		    //for (int j = 1; j < prev.pointChain.size()-2; j++) {
		    //by this:
		    for (int j = 1; j < prev.pointChain.size()-2; j++) {
		   
			boolean compute = false;
			//System.out.println(" checking triangle:");
			//(new Triangle(x,(Point)prev.pointChain.get(j+1),(Point)prev.pointChain.get(j))).print();
			if (Mathset.pointPosition(x,(Point)prev.pointChain.get(j+1),(Point)prev.pointChain.get(j)) == -1) { compute = true; }
			if (compute) {
			    Triangle nt = new Triangle(x,
						       (Point)prev.pointChain.get(j+1),
						       (Point)prev.pointChain.get(j));
			    tl.add(nt);
			    //System.out.println("generated triangle(1)"); nt.print();
			    //newTris.add(nt);
			    pCprev = j+1;
			    computedAnyPrev = true;
			}//if
			else { break; }
		    }//for j
		}//if
		else {
		    //trying to handle a special case by substituting this:
		    //for (int j = prev.pointChain.size()-2; j > 1; j--) {
		    //by this:
		    for (int j = prev.pointChain.size()-2; j > 1; j--) {
			boolean compute = false;
			//System.out.println(" j: "+j+" - checking triangle:");
			//(new Triangle(x,(Point)prev.pointChain.get(j-1),(Point)prev.pointChain.get(j))).print();
			if (Mathset.pointPosition(x,(Point)prev.pointChain.get(j-1),(Point)prev.pointChain.get(j)) == 1) { compute = true; }
			if (compute) {
			    Triangle nt = new Triangle(x,
						       (Point)prev.pointChain.get(j-1),
						       (Point)prev.pointChain.get(j));
			    tl.add(nt);
			    //System.out.println("generated triangle(2)"); nt.print();
			    //newTris.add(nt);
			    pCprev = j-1;
			    computedAnyPrev = true;
			}//if
			else { break; }
		    }//for j
		}//else


		//compute triangles for foll
		if (!follLiesTop) {
		    //trying to handle a special case by substituting this:
		    //for (int j = foll.pointChain.size()-2; j > 1; j--) {
		    //by this:
		    for (int j = foll.pointChain.size()-2; j > 1; j--) {
			boolean compute = false;
			if (Mathset.pointPosition(x,(Point)foll.pointChain.get(j-1),(Point)foll.pointChain.get(j)) == 1) { compute = true; }
			if (compute) {
			    Triangle nt = new Triangle(x,
						       (Point)foll.pointChain.get(j-1),
						       (Point)foll.pointChain.get(j));
			    tl.add(nt);
			    //System.out.println("generated triangle(3)"); nt.print();
			    //newTris.add(nt);
			    pCfoll = j-1;
			    computedAnyFoll = true;
			}//if
			else { break; }
		    }//for j
		}//if
		else {
		    //trying to handle a special case by substituting this:
		    //for (int j = 1; j < foll.pointChain.size()-2; j++) {
		    //by this:
		    for (int j = 1; j < foll.pointChain.size()-2; j++) {
			boolean compute = false;
			if (Mathset.pointPosition(x,(Point)foll.pointChain.get(j+1),(Point)foll.pointChain.get(j)) == -1) { compute = true; }
			if (compute) {
			    Triangle nt = new Triangle(x,
						       (Point)foll.pointChain.get(j+1),
						       (Point)foll.pointChain.get(j));
			    tl.add(nt);
			    //newTris.add(nt);
			    //System.out.println("generated triangle(4)"); nt.print();
			    pCfoll = j+1;
			    computedAnyFoll = true;
			}//if
			else { break; }
		    }//for j
		    
		}//else
		    

		//System.out.println("TriList:"); newTris.print();

		//update ystruct
		//build new SweepStElem with the remaining points
		//and substitute it with the old three elements
		//new = n
		//(mt a mt b mt) -> (mt n mt)
		//System.out.println("pCprev:"+pCprev+", pCfoll:"+pCfoll+", prevLiesTop:"+prevLiesTop+", follLiesTop:"+follLiesTop+", computedAnyPrev:"+computedAnyPrev+", computedAnyFoll:"+computedAnyFoll);	
		//System.out.println("\nprev.pointchain:"); prev.pointChain.print();
		//System.out.println("\nfoll.pointchain:"); foll.pointChain.print();
		newElem = new SweepStElem();
		newElem.rimoEl = (Point)x.copy();
		//first add all remaining elements of prev
		if (!computedAnyPrev) {
		    if (prevLiesTop) {
			for (int j = prev.pointChain.size()-1; j > 0; j--) {
			    newElem.pointChain.add(((Point)prev.pointChain.get(j)).copy());
			}//for j
		    }//if
		    else {
			for (int j = 0; j < prev.pointChain.size()-1; j++) {
			    newElem.pointChain.add(((Point)prev.pointChain.get(j)).copy());
			}//for j
		    }//if
		}//if
		else {
		    if (prevLiesTop) {
			//for (int j = pCprev; j < prev.pointChain.size(); j++) {
			for (int j = prev.pointChain.size()-1; j > pCprev-1; j--) {
			    newElem.pointChain.add(((Point)prev.pointChain.get(j)).copy());
			}//for j
		    }//if
		    else {
			for (int j = 0; j < pCprev+1; j++) {
			    newElem.pointChain.add(((Point)prev.pointChain.get(j)).copy());
			}//for j
		    }//else
		}//else

		//now add x
		newElem.pointChain.add((Point)x.copy());

		//finally add all remaining elements of foll
		if (!computedAnyFoll) {
		    if (follLiesTop) {
			for (int j = 1; j < foll.pointChain.size(); j++) {
			    //for (int j = foll.pointChain.size()-1; j > 0; j--) {
			    newElem.pointChain.add(((Point)foll.pointChain.get(j)).copy());
			}//for j
		    }//if
		    else {
			//for (int j = 0; j < foll.pointChain.size()-1; j++) {
			for (int j = foll.pointChain.size()-2; j > -1; j--) {
			    newElem.pointChain.add(((Point)foll.pointChain.get(j)).copy());
			}//for j
		    }//else
		}//if
		else {
		    if (follLiesTop) {
			for (int j = pCfoll; j < foll.pointChain.size(); j++) {
			    newElem.pointChain.add(((Point)foll.pointChain.get(j)).copy());
			}//for j
		    }//if
		    else {
			for (int j = pCfoll; j >= 0; j--) {
			    newElem.pointChain.add(((Point)foll.pointChain.get(j)).copy());
			}//for j
		    }//else
		}//else
		
		//SPECIAL CASE: A certain case is not included in the algorithm described
		//by Mehlhorn. A point with attribute 'end' may be found as lying in an 
		//'out'-interval (as it is implemented now) but in fact is lying in an 'in'-
		//interval. This is correctly found when using the interval2-method and 
		//checking, whether both endpoints of the interval have the same endpoint.
		//If not, we found the special case. This is implemented in the following.
		
		//find x in pointChain
		int xPos = -1;
		for (int pc = 1; pc < newElem.pointChain.size()-2; pc++) {
		    if (x.equal((Point)newElem.pointChain.get(pc))) {
			xPos = pc;
			break;
		    }//if
		}//for pc
	
		//generate all possible triangles for special case.
		//The special case can be detected, when examining the intersection points
		//of the outermost segments of the interval with the sweep-line. If x lies
		//in the middle of the other points, it lies in an "out"-interval. Thus
		//the special case occurs in the other case.
		if (newElem.pointChain.size() > 4) {
		    //constructing intersection points
		    //find max/min y values

		    Point p01 = (Point)newElem.pointChain.getFirst();
		    Point p02 = (Point)newElem.pointChain.get(1);
		    Point p03 = (Point)newElem.pointChain.get(newElem.pointChain.size()-2);
		    Point p04 = (Point)newElem.pointChain.getLast();
		    PointList pl = new PointList();
		    pl.add(p01);
		    pl.add(p02);
		    pl.add(p03);
		    pl.add(p04);
		    //sort in respect to y value
		    Point actPoint = null;
		    for (int m = 0; m < pl.size()-1; m++) {
			actPoint = (Point)pl.get(m);
			for (int n = m; n < pl.size(); n++) {
			    if (actPoint.compY((Point)pl.get(n)) == 1) {
				Point swap = actPoint;
				pl.set(m,pl.get(n));
				pl.set(n,swap);
				actPoint = (Point)pl.get(m);
			    }//if
			}//for n
		    }//for m

		    Rational minY,maxY;
		    minY = ((Point)pl.getFirst()).y.minus(1);
		    maxY = ((Point)pl.getLast()).y.plus(1);
		    
		    //constructing sweepline
		    Segment sweepline = new Segment(x.x,minY,x.x,maxY);
		    /* 
		       GFXout zzz = new GFXout();
		       zzz.initWindow();
		       zzz.add(sweepline.copy());
		       zzz.add(new Segment(p01,p02));
		       zzz.add(new Segment(p03,p04));
		       zzz.showIt();
		       try { int data = System.in.read(); }
		       catch (Exception e) { System.exit(0); }
		       zzz.kill();
		    */

		    //constructing intersection points
		    //System.out.println("sweepline construction[2]");
		    Point int1 = sweepline.intersection(new Segment(p01,p02));
		    //System.out.println("sweepline construction[3]");
		    Point int2 = sweepline.intersection(new Segment(p03,p04));

		    //now determine, whether we have a special case

		    if ((x.y.less(int1.y) && x.y.less(int2.y)) ||
			(x.y.greater(int1.y) && x.y.greater(int2.y))) {
			//System.out.println("##### SPECIAL CASE #####");
			
			
			boolean topdown = false;
			if (((Point)newElem.pointChain.get(1)).compY((Point)newElem.pointChain.get(newElem.pointChain.size()-2)) == 1) {
			    topdown = true;
			}//if
			
			//System.out.println("**pointChain: "); newElem.pointChain.print();
			//System.out.println("topdown: "+topdown);
			
			
			//for the pointPosition test we have to find out, 
			//whether we are going top-down or vice-versa
			//use topdown for this
			delList = new PointList();
			if (topdown) {
			    for (int k = 1; k < newElem.pointChain.size()-3; k++) {
				Point p1 = (Point)newElem.pointChain.get(k);
				Point p2 = (Point)newElem.pointChain.get(k+1);
				Point p3 = (Point)newElem.pointChain.get(k+2);
				//System.out.println(" checking triangle (special case1):");
				//(new Triangle(p1,p2,p3)).print();
				if (Mathset.pointPosition(p2,p1,p3) == 1) {
				    Triangle nt = new Triangle(p1,p2,p3);
				    tl.add(nt);
				    //System.out.println("generated triangle(special case1)"); nt.print();
				    delList.add(p2);
				}//if
			    }//for k
			    //deleting points using delList
			    boolean found = false;
			    for (int k = 0; k < delList.size(); k++) {
				Point delPoint = (Point)delList.get(k);
				found = false;
				for (int lc = 1; lc < newElem.pointChain.size()-2; lc++) {
				    if (((Point)newElem.pointChain.get(lc)).equal(delPoint)) {
					found = true;
					newElem.pointChain.remove(lc);
					break;
				    }//if
				}//for lc
				if (found) delList.remove(k);
				
				else {
				    System.out.println("Error in P.computeTriangles: Didn't find point (to remove) in pointChain.");
				    System.exit(0);
				}//else
			    }//for k
			}//if
			else {
			    for (int k = newElem.pointChain.size()-3; k > 1; k--) {
				Point p1 = (Point)newElem.pointChain.get(k);
				Point p2 = (Point)newElem.pointChain.get(k-1);
				Point p3 = (Point)newElem.pointChain.get(k-2);
				//System.out.println(" checking triangle (special case2):");
				//(new Triangle(p1,p2,p3)).print();
				if (Mathset.pointPosition(p2,p1,p3) == -1) {
				    Triangle nt = new Triangle(p1,p2,p3);
				    tl.add(nt);
				    //System.out.println("generated triangle(special case2)"); nt.print();
				    delList.add(p2);
				}//if
			    }//for k
			}//else
			
			//deleting points using delList
			boolean found = false;
			for (int k = 0; k < delList.size(); k++) {
			    Point delPoint = (Point)delList.get(k);
			    found = false;
			    for (int lc = 1; lc < newElem.pointChain.size()-2; lc++) {
				if (((Point)newElem.pointChain.get(lc)).equal(delPoint)) {
				    found = true;
				    newElem.pointChain.remove(lc);
				    break;
				}//if
			    }//for lc
			    if (found) delList.remove(k);
			    else {
				System.out.println("Error in P.computeTriangles: Didn't find point (to remove) in pointChain.");
				System.exit(0);
			    }//else
			}//for k
			
			//System.out.println("pointChain after deletion: "); newElem.pointChain.print();

			//set new rimoEl
			//find rimoEl
			Point rmL = (Point)newElem.pointChain.get(1);;
			for (int pc = 1; pc < newElem.pointChain.size()-2; pc++) {
			    if (rmL.compX((Point)newElem.pointChain.get(pc)) == -1) {
				rmL = (Point)newElem.pointChain.get(pc); }
			}//for pc
			newElem.rimoEl = rmL;
		    }//if
		}//if	   
		/*
		//check and resort pointChain if necessary
		checkAndResortPointChain(newElem.pointChain);
		*/
		ystruct.set(yElemPos,newElem);
		ystruct.remove(yElemPos+1);
		ystruct.remove(yElemPos-1);
	    }//if
	      
	    else {
	      //x lies in an "in" interval of ystruct
	      
	      //compute triangles
	      //System.out.println(" x lies in an 'in'-interval");
		
		for (int j = 1; j < yElem.pointChain.size()-2; j++) {
		    Triangle nt = new Triangle(x,
					       (Point)yElem.pointChain.get(j),
					       (Point)yElem.pointChain.get(j+1));
		    tl.add(nt);
		    //System.out.println("generated triangle:");
		    //nt.print();
		    //now add points to delList
		    if (!(((Point)yElem.pointChain.get(j+1)).equal((Point)yElem.pointChain.getLast()))) { delList.add(yElem.pointChain.get(j+1)); }
		}//for
		
	      //update ystruct
	      //remove points in delList from yElem
	      //buggy? delete all(!) elements which are not first or last
	      
	      //??? delete this yElem and one of the neighbour mt-elements from ystruct
	      /*PointList newPL = new PointList();
		newPL.add((Point)yElem.pointChain.getFirst());
		newPL.add((Point)yElem.pointChain.getLast());
		yElem.pointChain = newPL;
	      */
	      ystruct.remove(yElemPos);
	      ystruct.remove(yElemPos-1);
	      //System.out.println(" removed two yElems");
	    }//else
	  }//if
	  //now all operations for attribute=="end" are done

	}//else
      }//else
      
      //clean up ystruct: delete rows of mt-elements
      if (ystruct.size() > 1) {
	for (int j = ystruct.size()-1; j > 0; j--) {
	  //System.out.println(" ystruct.size(): "+ystruct.size()+"   j: "+j);
	  if ((((SweepStElem)ystruct.get(j)).pointChain.size() == 0) &&
	      (((SweepStElem)ystruct.get(j-1)).pointChain.size() == 0)) {
	    ystruct.remove(j);
	    //System.out.println(" removed one mt element from ystruct");
	  }//if
	}//for j
      }//if

    }//for i
    //now the sweep is done

    //System.out.println();
    //System.out.println("---> result of triangulation: "+tl.size()+" triangle(s).");
    //System.out.println();
    //System.out.println("leaving Pol.computeTriangles.");
    return tl;
  }//end method comuteTriangles
  

    protected static int interval(Point p, LinkedList ystruct) {
	//supportive method for computeTriangles
	//returns the position of the interval in ystruct which
	//includes p
	//note that the first and last element of intervals are
	//exclusive elements
	//if p is such a border, the interval position-1 is returned
	//or +1 resp.

	//boolean first = false;
	//boolean last = false;
	//boolean second = false;
	//boolean lastButOne = false;
	boolean foundOne = false;
	boolean foundTwo = false;
	int marker = 0;
	int marker2 = 0;
	
	if (ystruct.isEmpty()) {
	    ystruct.add(new SweepStElem());
	    return 0;
	}//if

	//Search the intervals borders for the element
	//One element may occur two times on the borders, which means
	//it is the connecting element. Hence we search for the element
	//two times.
	for (int i = 0; i < ystruct.size(); i++) {
	    SweepStElem actSSE = (SweepStElem)ystruct.get(i);
	    if (!actSSE.pointChain.isEmpty()) {
		if (((Point)actSSE.pointChain.getFirst()).equal(p)) {
		    //first = true;
		    if (!foundOne) { 
			marker = i;
			foundOne = true;
		    }//if
		    else {
			marker2 = i;
			foundTwo = true;
		    }//else
		}//if
		if (((Point)actSSE.pointChain.getLast()).equal(p)) {
		    //last = true;
		    if (!foundOne) {
			marker = i; 
			foundOne = true;
		    }//if
		    else {
			marker2 = i;
			foundTwo = true;
		    }//else
		}//if
		if (((Point)actSSE.pointChain.get(1)).equal(p)) {
		    //second = true;
		    if (!foundOne) {
			marker = i;
			foundOne = true;
		    }//if
		    else {
			marker2 = i; 
			foundTwo = true;
		    }//if
		}//if
		if (((Point)actSSE.pointChain.get(actSSE.pointChain.size()-2)).equal(p)) {
		    //lastButOne = true;
		    if (!foundOne) {
			marker = i;
			foundOne = true;
		    }//if
		    else {
			marker2 = i;
			foundTwo = true;
		    }//else
		}//if
		if (foundTwo) { break; }
	    }//if
	}//for i

	if (foundOne || foundTwo) {
	    if (foundOne && foundTwo) {
		//we found the element two times
		if (marker == marker2) {
		    //both indices are equal, so everythings allright
		    return marker;
		}//if
		if (marker2 != (marker+2)) {
		    //indices should be equal or have a difference of 2
		    //if not, there is a serious error
		    System.out.println("P.interval: 1+1=3? While searching for the proper interval to insert a new SweepStElem, two intervals were found, but they are too far away from eachother!");
		    System.out.println("marker:"+marker+", marker2:"+marker2);
		    System.exit(0);
		}//if
		//System.out.println("P.interval-case0.5");
		return marker+1;
	    }//if
	    //System.out.println("P.interval-case1");
	    return marker;
	}//if

	//so p is not lying on the boundary of one of the intervals
	//now it is checked whether its y-coordinate is lying inside of
	//the boundaries
	else {
	    boolean inside = false;
	    boolean outside = false;
	    Rational ypos = p.y;
	    //System.out.println("ypos:"+ypos);
	    for (int i = 0; i < ystruct.size(); i++) {
		SweepStElem actSSE = (SweepStElem)ystruct.get(i);
		if (!actSSE.pointChain.isEmpty()) {
		    //does p lie in the "in" interval?
		    Point g11 = (Point)actSSE.pointChain.get(0);
		    Point g12 = (Point)actSSE.pointChain.get(1);
		    Point g21 = (Point)actSSE.pointChain.getLast();
		    Point g22 = (Point)actSSE.pointChain.get(actSSE.pointChain.size()-2);
		    if (((Mathset.pointPosition(g11,g12,p) == -1) &&
			 (Mathset.pointPosition(g21,g22,p) == 1)) ||
			((Mathset.pointPosition(g11,g12,p) == 1) &&
			 (Mathset.pointPosition(g21,g22,p) == -1))) {
			marker = i;
			inside = true;
			System.out.println("P.interval-case2");
			break;
		    }//if
		    //ypos is smaller than the next not mt interval,
		    //so return position of last mt interval
		    if (ypos.less(((Point)actSSE.pointChain.get(1)).y) &&
			ypos.less(((Point)actSSE.pointChain.get(actSSE.pointChain.size()-2)).y)) {
			if (i > 0) {
			    marker = i-1;
			    outside = true;
			    System.out.println("P.interval-case3");
			    break;
			}//if
		    }//if
		}//if
	    }//for i
	    if (inside || outside) {
		return marker;
	    }//if
	    
	}//else
	System.out.println("P.interval-case4");
	return ystruct.size()-1;
    }//end method interval
		    
			
    
  protected static int interval2(Point p, LinkedList ll) {
    //supportive method for compute_triangles
    //determines whether p lies in an "in" or an "out" interval of ll
    //attention: p does not lie in an interval,
    //if it is the first or last element of it!
    //returns the position of the suitable SweepStElem (element of ll)
      //
      //CAUTION: this is currently only used for one case with attribute = start
      //It is not clear whether it should be used also in the other cases and
      //whether it works properly.
    Rational ypos = RationalFactory.constRational(p.y);

    if (ll.isEmpty()) {
      //ll is empty, so add an empty element to ll and return it
      SweepStElem newOne = new SweepStElem();
      ll.add(newOne);
      return 0;
    }//if
    else {

      //there is a problem with the borders of intervals:
      //it may occur that a point is supposed to lie in an interval
      //that has the proper boundaries but that's only for the reason
      //that not the proper boundaries but the following points on the
      //boundaries are stored! for this reason we first check whether
      //a given point really lies in an interval (by checking all(!)
      //points of the pointchain). this takes a while but prevents errors.
      //solution: don't check the given first and last element but the elements
      //on positions +1 and -1 respectively

      boolean found = false;
      
      for (int i = 0; i < ll.size(); i++) {
	  if (!(((SweepStElem)ll.get(i)).pointChain.isEmpty())) {
	      for (int j = 1; j < ((SweepStElem)ll.get(i)).pointChain.size()-1; j++) {
		  if (p.equal((Point)((SweepStElem)ll.get(i)).pointChain.get(j))) {
		      found = true;
		      break;
		  }//if
	      }//for j
	      if (found) { 
		  //System.out.println(" found p in interval "+i);
		  return i;
	      }//if
	  }//if
      }//for i
      
      //p was not found so it may lie on the interval boundaries
      //repeat the search
      
      for (int i = 0; i < ll.size(); i++) {
	  if (!(((SweepStElem)ll.get(i)).pointChain.isEmpty())) {
	      for (int j = 0; j < ((SweepStElem)ll.get(i)).pointChain.size(); j++) {
		  if (p.equal((Point)((SweepStElem)ll.get(i)).pointChain.get(j))) {
		      found = true;
		      break;
		  }//if
	      }//for j
	      if (found) { 
		  //System.out.println(" found p in interval "+i);
		  return i;
	      }//if
	  }//if
      }//for i
      
      //if the element was not found, do the old code:
      if (!found) {
	for (int i = 0; i < ll.size(); i++) {
	  if (!(((SweepStElem)ll.get(i)).pointChain.isEmpty())) {
	    //p lies in the "in" interval
	    if (((ypos.greater(((Point)((SweepStElem)ll.get(i)).pointChain.get(0)).y)) &&
		 (ypos.less(((Point)((SweepStElem)ll.get(i)).pointChain.get(((SweepStElem)ll.get(i)).pointChain.size()-1)).y))) ||
		((ypos.equal(((Point)((SweepStElem)ll.get(i)).pointChain.getFirst()).y)) ||
		 (ypos.equal(((Point)((SweepStElem)ll.get(i)).pointChain.getLast()).y))))
		{
		    //System.out.println("P.interval2: case3");
		    //System.out.println("greater:"+(ypos.greater(((Point)((SweepStElem)ll.get(i)).pointChain.get(0)).y)));
		    //System.out.println("less:"+(ypos.less(((Point)((SweepStElem)ll.get(i)).pointChain.get(((SweepStElem)ll.get(i)).pointChain.size()-1)).y)));
		    return i;
		}//if
	    //p is smaller than the next "in" interval, so return the last "out" interval
	    if ((ypos.less(((Point)((SweepStElem)ll.get(i)).pointChain.get(0)).y)) &&
		(ypos.less(((Point)((SweepStElem)ll.get(i)).pointChain.get(((SweepStElem)ll.get(i)).pointChain.size()-1)).y))) {
		{ 
		    if (i > 0) {
			//System.out.println("P.interval2:case4");
			return (i-1); } }//if
	    }//if
	  }//if
	}//for i
      }//else
      //no proper interval was found so return the last interval
      //(which hopefully is an "out" interval)
      //System.out.println("---> Triangulation(interval): No proper interval found. Index for last element returned!");
      return (ll.size()-1);
    }//else
  }//end method interval


    protected static int interval3(Point p, LinkedList ll) {
	//supportive method for compute_triangles
	//determines whether p lies in an "in" or an "out" interval of ll
	//returns the proper position of the suitable SweepStElem (element of ll)
	//THIS method uses a sweepline to find out the proper interval
	//BOTH other interval methods (1,2) don't work properly
	
	//compute a list of pairs which are the _real_ borders of the intervals
	//this means the intersection points of the sweep line and the 'border' segments

	PairList realInts = new PairList();
	ListIterator lit = ll.listIterator(0);
	SweepStElem actSSE = null;
	while (lit.hasNext()) {
	    actSSE = (SweepStElem)lit.next();
	    if (actSSE.pointChain.isEmpty()) {
		realInts.add(new ElemPair());
	    }//if
	    else {
		//compute sweepline
		PointList pll = new PointList();
		pll.add(actSSE.pointChain.getFirst());
		pll.add(actSSE.pointChain.get(1));
		pll.add(actSSE.pointChain.get(actSSE.pointChain.size()-1));
		pll.add(actSSE.pointChain.get(actSSE.pointChain.size()-2));
		//find min,max in respect to y coordinate
		Point max = (Point)pll.getFirst();
		Point min = (Point)pll.getFirst();
		for (int i = 1; i < pll.size(); i++) {
		    if (((Point)pll.get(i)).compY(min) == -1) min = (Point)pll.get(i);
		    else if (((Point)pll.get(i)).compY(max) == 1) max = (Point)pll.get(i);
		}//for i
		Segment sweepline = new Segment(p.x,min.y.minus(1),p.x,max.y.plus(1));

		//compute both intersection points

		//CAUTION: The computation of intersection points using the sweepline
		//doesn't work if the segments are vertical! This must be checked first.

		Point int1 = null;
		if (((Point)actSSE.pointChain.getFirst()).compX((Point)actSSE.pointChain.get(1)) == 0) {
		    int1 = (Point)actSSE.pointChain.getFirst();
		    //System.out.println("Pol.iv3: vertical element!");
		}//if
		else {
		    //System.out.println("Pol.iv3: sweepline construction[1]");
		    int1 = sweepline.intersection(new Segment((Point)actSSE.pointChain.getFirst(),
								    (Point)actSSE.pointChain.get(1)));
		    //System.out.println("Pol.iv3: sweepline construction[2], point:"); p.print();
		}//else
		Point int2 = null;
		if (((Point)actSSE.pointChain.get(actSSE.pointChain.size()-1)).compX((Point)actSSE.pointChain.get(actSSE.pointChain.size()-2)) == 0) {
		    int2 = (Point)actSSE.pointChain.get(actSSE.pointChain.size()-1);
		    //System.out.println("Pol.iv3: vertical element!");
		}//if
		else {
		    //--->>>
		    //remove this
		    if (!sweepline.intersects(new Segment((Point)actSSE.pointChain.get(actSSE.pointChain.size()-1),
							   (Point)actSSE.pointChain.get(actSSE.pointChain.size()-2)))) {
			System.out.println("Pol.interval3: sweepline doesn't intersect segment!");
			System.out.println("pointChain:");
			((PointList)actSSE.pointChain).print();
			System.out.println("\npoint:");
			p.print();
			System.exit(0);
		    }//if
		    //<<<---
		    
		    int2 = sweepline.intersection(new Segment((Point)actSSE.pointChain.get(actSSE.pointChain.size()-1),
								    (Point)actSSE.pointChain.get(actSSE.pointChain.size()-2)));
		}//else
		
		//construct elemPair and add it to realInts
		if (int1.compY(int2) == -1) realInts.add(new ElemPair(int1,int2));
		else realInts.add(new ElemPair(int2,int1));
	    }//else
	}//while lit

	//now check, whether p _really_ lies in an interval or not and return the proper value
	//It is assumed, that the intervals are sorted correctly!
	lit = realInts.listIterator(0);
	while (lit.hasNext()) {
	    //get actual interval:
	    ElemPair actPair = (ElemPair)lit.next();
	    if (actPair.first != null) {
		//if interval is not mt read borders
		Point first = (Point)actPair.first;
		Point second = (Point)actPair.second;

		//compute compare values
		int pCompY1st = p.compY(first);
		int pCompY2nd = p.compY(second);

		//point p lies in between the borders, return interval index:
		if ((pCompY1st == 1) && (pCompY2nd == -1)) return lit.nextIndex()-1;

		//point p is found as first and last point of actual interval AND
		//it is first and last point of the previous non-mt interval
		//then return index of the previous mt interval
		else if (((pCompY1st == 0) || (pCompY2nd == 0)) &&
			 (lit.nextIndex()-3 > 0) &&
			 ((p.compY(((ElemPair)realInts.get(lit.nextIndex()-3)).first) == 0) ||
			  (p.compY(((ElemPair)realInts.get(lit.nextIndex()-3)).second) == 0))) return lit.nextIndex()-2;
		
		//point p is found as first and last point of actual interval
		//return interval index:
		else if ((pCompY1st == 0) && (pCompY2nd == 0)) return lit.nextIndex()-1;

		//p is found as first
		else if (((pCompY1st == 0) && (pCompY2nd != 0)) ||
			 ((pCompY1st != 0) && (pCompY2nd == 0))) return lit.nextIndex()-1;

		//y coordinate of p is smaller than interval borders
		//return index of previous mt interval: 
		else if ((pCompY1st < 0) && (pCompY2nd < 0)) return lit.nextIndex()-2;
		
	    }//if
	}//while lit

	//no proper interval was found, so return last index
	return realInts.size()-1;
    }//end method interval3
	    
						 

    

    protected static String attribute(Point x, SegList sortedBorder) {
	//supportive method for 'triangles'
	//determines whether a Point has attribute start, end or bend
	//returns "start","end" or "bend"
	//caution: this method is used in Triangle.java - SURE???
	//System.out.println("entering Pol.attribute...");
	
	//compute sorted border
	//border = sortBorder(border,x);
	//resort border
	SegList border = resortBorder(sortedBorder,x);
	//System.out.println();
	//System.out.println("sortedBorder");
	//border.print();

	//from sortedborder extract the bordercycle which includes x
	SegList borderCycle = new SegList();
	boolean found = false;
	borderCycle.add(border.getFirst());
	int iterator = 1;
	ListIterator lit = border.listIterator(1);
	Segment actSeg;
	while (!found) {
	    if (lit.hasNext()) {
		actSeg = (Segment)lit.next();
		if (PointSeg_Ops.isEndpoint(x,actSeg)) {
		    found = true; }
		borderCycle.add(actSeg);
		iterator++;
	    }//if
	}//while
		
	//search in borderCycle to find the next points in both directions
	// which have different x-coordinates
	int down = 0;
	int up = 0;
	boolean diffXcoord = false;
	Point nextPoint = new Point();
	Point actPoint = x;
	int actPos = 0;
	actSeg = (Segment)borderCycle.getFirst();
	while (!diffXcoord) {
	    //System.out.println("actpos:"+actPos);
	    nextPoint = actSeg.theOtherOne(actPoint);
	    if (!nextPoint.x.equal(actPoint.x)) {
		if (nextPoint.x.less(actPoint.x)) {
		    up = -1;
		    diffXcoord = true;
		}//if
		else {
		    up = 1;
		    diffXcoord = true;
		}//else
	    }//if
	    else {
		actPos++;
		actSeg = (Segment)borderCycle.get(actPos);
		actPoint = nextPoint;
	    }//else
	}//while
	
	//now do that also in the other direction
	diffXcoord = false;
	nextPoint = new Point();
	actPoint = x;
	actPos = borderCycle.size()-1;
	actSeg = (Segment)borderCycle.getLast();
	while (!diffXcoord) {
	    nextPoint = actSeg.theOtherOne(actPoint);
	    if (!nextPoint.x.equal(actPoint.x)) {
		if (nextPoint.x.less(actPoint.x)) {
		    down = -1;
		    diffXcoord = true;
		}//if
		else {
		    down = 1;
		    diffXcoord = true;
		}//else
	    }//if
	    else {
		actPos--;
		actSeg = (Segment)borderCycle.get(actPos);
		actPoint = nextPoint;
	    }//else
	}//while
	//System.out.println("up:"+up+", down:"+down+", x:"); x.print();
	//find out if up is really the upper point and vice versa
	//also compute attributes for direct neighbours to determine
	//wether a point is "bend"
	int nextUp = 0;
	int nextDown = 0;
	nextUp = (((Segment)borderCycle.getFirst()).endpoint).compX(x);
	//System.out.println("compute nextUp:"+nextUp); ((Segment)borderCycle.getFirst()).print();
	nextDown = (((Segment)borderCycle.getLast()).startpoint).compX(x);
	//System.out.println("compute nextDown:"+nextDown); ((Segment)borderCycle.getLast()).print();
	Point upP = ((Segment)borderCycle.getFirst()).endpoint;
	Point downP = ((Segment)borderCycle.getLast()).startpoint;
	//if (upP.y.less(downP.y)) {
	if (borderCycle.size()<3) {
	    System.out.println("Pol.attribute: borderCycle is smaller than 3!");
	    System.out.println("x:");
	    x.print();
	    System.out.println();
	    borderCycle.print();
	    System.exit(0);
	}//if
	if (!isUpperSegment((Segment)borderCycle.getFirst(),(Segment)borderCycle.getLast())) {
	    //System.out.println("***swap***");
	    //swap up and down
	    int help = up;
	    up = down;
	    down = help;
	    //swap nextUp and nextDown
	    help = nextUp;
	    nextUp = nextDown;
	    nextDown = help;
	}//if
	//System.out.println("up:"+up+", down:"+down+", nextUp:"+nextUp+", nextDown:"+nextDown+", x:"); x.print();

	//now return the right attribute
	if ((nextUp == 1) && (nextDown == 1)) {
	    return "start"; }
	else if ((nextUp == 1) && (nextDown == -1)) {
	    return "bend"; }
	else if ((nextUp == -1) && (nextDown == 1)) {
	    return "bend"; }
	else if ((nextUp == -1) && (nextDown == -1)) {
	    return "end"; }
	else if ((nextUp == 1) && (nextDown == 0) && (down == 1)) {
	    return "start"; }
	else if ((nextUp == 1) && (nextDown == 0) && (down == -1)) {
	    return "start"; } //this was originally bend
	else if ((nextUp == -1) && (nextDown == 0) && (down == 1)) {
	    return "bend"; }
	else if ((nextUp == -1) && (nextDown == 0) && (down == -1)) {
	    return "bend"; }
	else if ((nextUp == 0) && (nextDown == -1) && (up == 1)) {
	    return "end"; } //this was bend 
	else if ((nextUp == 0) && (nextDown == -1) && (up == -1)) {
	    return "end"; }
	else if ((nextUp == 0) && (nextDown == 1) && (up == 1)) {
	    return "bend"; }
	else if ((nextUp == 0) && (nextDown == 1) && (up == -1)) {
	    return "bend"; }
	else if ((nextUp == 0) && (nextDown == 0)) {
	    return "bend"; }
	
	
	System.out.println("Pol.attribute: cant find attribute.");
	System.exit(0);
	return "";

  }//end method attribute
  

    public double area(){
	//returns the area of the polygons
	if (!this.areaDefined) {
	    this.area = computeArea();
	    this.areaDefined = true;
	}//if
	return this.area;
    }//end method get_area
  
    public double perimeter(){
	//returns the perimeter of the polygons
	if (!this.perimeterDefined) {
	    this.perimeter = computePerimeter();
	    this.perimeterDefined = true;
	}//if
	return this.perimeter;
    }//end method get_perimeter

  public void set(TriList tlist){
    //sets the triangle list of the polygons
    this.trilist = (TriList)tlist.copy();
    
    this.perimeterDefined = false;
    this.areaDefined = false;
    this.borderDefined = false;
    this.bboxDefined = false;
  }//end method set

    public SegList border(){
	//returns the border of the polygons
	if (!this.borderDefined) {
	    this.border = computeBorder();
	    this.borderDefined = true;
	}//if
	return (SegList)this.border.copy();
  }//end method getBorder

  public TriList triangles(){
    //returns the triangle list of the polygons
    TriList t = (TriList)(this.trilist.copy());
    return t;
  }//end method triangles

  public Element copy(){
    //returns a copy of the polygons
    Polygons copy = new Polygons();
    copy.trilist = (TriList)(this.trilist.copy());
    copy.perimeter = this.perimeter;
    copy.area = this.area;
  return copy;
  }//end method copy

  public PointList vertices() {
    //returns a PointList of the polygons' vertices

      //System.out.println("Polygon.vertices calling...");

    PointList pl = new PointList();
    for (int i = 0; i < border.size(); i++) {
	//System.out.println("generating vertices from edge "+i);
	pl.add(((Segment)border.get(i)).startpoint);
	pl.add(((Segment)border.get(i)).endpoint);
    }//for i
    try {
	//System.out.println("rdup");
	pl = PointList.convert(SetOps.rdup(pl));
    }//try
    catch (Exception e) {
	System.out.println("Exception was thrown in Polygons.vertices:");
	System.out.println("Exception: "+e.getClass()+" --- "+e.getMessage());
	System.out.println("Exception Cause: "+e.getCause());
	System.out.println("Exception String: "+e.toString());
	System.exit(0);
    }//catch
    //System.out.println("Polygons.vertices exit.");
    return pl;
  }//end method vertices

    
    public Rational dist (Element inElement) throws WrongTypeException {
	//comment missing
	if (inElement instanceof Polygons) {
	    Polygons inPol = (Polygons)inElement;
	    Rational retVal = RationalFactory.constRational(0);
	    Class c = (new Triangle()).getClass();
	    Class[] paramList = new Class[1];

	    try {
		paramList[0] = Class.forName("Element");
		Method m = c.getMethod("dist",paramList);
		ElemPair retPair = SetOps.min(this.trilist,inPol.trilist,m);
		retVal = retPair.first.dist(retPair.second);
	    }//try
	    catch (Exception e) {
		System.out.println("Exception was thrown in Polygons.dist:");
		System.out.println("Exception: "+e.getClass()+" --- "+e.getMessage());
		System.out.println("Exception cause: "+e.getCause());
		System.out.println("Exception String: "+e.toString());
		System.exit(0);
	    }//catch
	    return retVal;
	}//if
	else { throw new WrongTypeException(); }
    }//end method dist

    public boolean intersects (Element inElement) throws WrongTypeException {
	//comment missing
	//CAUTION: untested method
	if (inElement instanceof Polygons){
	    Polygons inPol = (Polygons)inElement;
	    Class c = (new Triangle()).getClass();
	    PairList retList = new PairList();
	    Class[] paramList = new Class[1];
	    try {
		paramList[0] = Class.forName("Element");
		Method m = c.getMethod("intersects",paramList);
		retList = SetOps.join(this.trilist,inPol.trilist,m);
	    }//try
	    catch (Exception e) {
		System.out.println("Exception was thrown in Polygons.print:");
		System.out.println("Exception: "+e.getClass()+" --- "+e.getMessage());
		System.out.println("Exceptoin cause: "+e.getCause());
		System.out.println("Exception string: "+e.toString());
		System.exit(0);
	    }//catch
	    if (retList.isEmpty()) { return false; }
	    else { return true; }
	}//if
	else { throw new WrongTypeException(); }
    }//end method intersects
    
    public void print() {
	//comment missing
	System.out.println("Polygons:");
	System.out.println("Perimeter: "+perimeter);
	System.out.println("Area: "+area);
	System.out.println("trilist:");
	for (int i = 0; i < trilist.size(); i++) {
	    ((Triangle)trilist.get(i)).print();
	}//for i
	System.out.println();
    }//end method print

    /*
    public void update() {
	//comment missing
	this.area = computeArea();
	this.border = computeBorder();
	this.perimeter = computePerimeter();
	computeBbox();
    }//end method update
    */

    public byte compare (Element inElement) throws WrongTypeException {
	//...
	System.out.println("CAUTION: Polygons.compare is currently not implemented.");
	return 0;
    }//end method compare

    public byte compX (Element inElement) throws WrongTypeException {
	//comment missing
	//CAUTION: untested method
	if (inElement instanceof Polygons) {
	    Polygons inPol = (Polygons)inElement;
	    if (this.bbox.ul.x.less(inPol.bbox.ul.x)) {
		return -1; }
	    else {
		if (this.bbox.ul.x.equal(inPol.bbox.ul.x)) {
		    return 0; }
		else { return 1; }
	    }//else
	}//if
	else { throw new WrongTypeException(); }
    }//end method compX

    public byte compY (Element inElement) throws WrongTypeException {
	//comment missing
	//CAUTION: untested method
	if (inElement instanceof Polygons) {
	    Polygons inPol = (Polygons)inElement;
	    if (this.bbox.ll.y.less(inPol.bbox.ll.y)) {
		return -1; }
	    else { 
		if (this.bbox.ll.y.less(inPol.bbox.ll.y)) {
		    return 0; }
		else { return 1; }
	    }//else
	}//if
	else { throw new WrongTypeException(); }
    }//end method compY


    public boolean equal (Element inElement) throws WrongTypeException {
	if (inElement instanceof Polygons) {
	    Polygons inPol = (Polygons)inElement;
	    if (SetOps.equal(this.trilist,inPol.trilist)) {
		return true;
	    }//if
	}//if
	else { throw new WrongTypeException(); }
	return false;
    }//end method equal
    

    private static SegList sortBorder(SegList inBorder, Point x) {
	//sort border, which means: sort the segments beginning with
	//a segment containing x as startpoint and ending with the
	//other segment containing x as endpoint
	//e.g.: (X,A)(A,B)(B,C)(C,D)(D,X)
	//System.out.println("entering P.sortBorder...");
	//SegList border = (SegList)inBorder.copy();
	SegList border = (SegList)inBorder.clone();
	SegList retList = new SegList();

	//search for the positions of the two segments containing x in border
	int pos1 = 0;
	int pos2 = 0;
	boolean first = false;

	ListIterator lit = border.listIterator(0);
	Segment actSeg = new Segment();
	Segment actSeg1 = new Segment();
	Segment actSeg2 = new Segment();
	while (lit.hasNext()) {
	    actSeg = (Segment)lit.next();
	    if (PointSeg_Ops.isEndpoint(x,actSeg)) {
		if (!first) {
		    pos1 = lit.nextIndex()-1;
		    actSeg1 = (Segment)actSeg.copy();
		    first = true;
		}//if
		else {
		    actSeg2 = (Segment)actSeg.copy();
		    pos2 = lit.nextIndex()-1;
		    break;
		}//else
	    }//if
	}//while

	//get segments
	//Segment seg1 = (Segment)border.get(pos1);
	//Segment seg2 = (Segment)border.get(pos2);
	Segment seg1 = actSeg1;
	Segment seg2 = actSeg2;

	//add first element to retList
	if (seg1.startpoint.equal(x)) {
	    retList.add(seg1); }
	else { retList.add(seg1.turn()); }
	//remove first element from border
	border.remove(pos1);

	//now add all other elements
	Point searchedPoint = seg1.theOtherOne(x);
	int actpos = -1;
	//actSeg = new Segment();
	Point actX = x; //the actual first point of a found cycle
	int xpos = 0; //the actual position of actX in retList
	//System.out.println("x:"); x.print();
	while (border.size() > 0) {
	    //search next element
	    actpos = -1;
	    ListIterator it = border.listIterator(0);
	    while (it.hasNext()) {
		actSeg = (Segment)it.next();
		if (PointSeg_Ops.isEndpoint(searchedPoint,actSeg)) {
		    actpos = it.nextIndex()-1;
		    break;
		}//if
	    }//while
	    if (actpos == -1) {
		//the polygons value consists of more than one object
		if (searchedPoint.equal(((Segment)retList.get(xpos)).startpoint)) {
		    //System.out.println("--->found cycle");
		    //a cycle was found, now compute the other ones
		    //define new x
		    x = ((Segment)border.getFirst()).startpoint;
		    retList.add((Segment)border.getFirst());
		    searchedPoint = ((Segment)border.getFirst()).endpoint;
		    border.remove(0);
		    xpos = retList.size()-1;
		}//if
		else {
		    System.out.println("Pol.sortBorder: Can't find next segment.");
		    System.exit(0);
		}//else
	    }//if
	    else {
		//add element to retList
		if (actSeg.startpoint.equal(searchedPoint)) {
		    retList.add(actSeg); }
		else { retList.add(actSeg.turn()); }
		//remove element from border
		//System.out.println("actpos:"+actpos+", border.size():"+border.size());
		border.remove(actpos);
		searchedPoint = actSeg.theOtherOne(searchedPoint);
	    }//else
	}//while
	//System.out.println("Pol.leaving sortBorder");
	return retList;
    }//end method sortBorder

    
     protected static boolean isUpperSegment(Segment seg1, Segment seg2) {
	//checks for two segments that form a line 
	//if seg1 is the upper one and returns true in that case
	//note: it seems that only four cases are relevant for the
	//attribute algorithm that uses this method
	//so only these four cases are examined
	if (!SegSeg_Ops.formALine(seg1,seg2)) {
	    System.out.println("SSO.isUpperSegment: segments don't form a line");
	    System.out.println("segment1: "); seg1.print();
	    System.out.println("segment2: "); seg2.print();
	    System.exit(0);
	}//if
	//find common point
	Point x = new Point();
	if (PointSeg_Ops.isEndpoint(seg1.startpoint,seg2)) {
	    x = seg1.startpoint; }
	else { x = seg1.endpoint; }
	Point seg1o = seg1.theOtherOne(x);
	Point seg2o = seg2.theOtherOne(x);
	
	if (seg1o.x.equal(x.x) && seg1o.y.greater(x.y)) {
	    return true; }
	if (seg1o.x.equal(x.x) && seg1o.y.less(x.y)) {
	    return false; }
	if (seg2o.x.equal(x.x) && seg2o.y.greater(x.y)) {
	    return false; }
	if (seg2o.x.equal(x.x) && seg2o.y.less(x.y)) {
	    return true; }
	
	return true;
    }//end method isUpperSegment
	

    private static SegList resortBorder(SegList border, Point x) {
	//supportive method for attribute
	//sort border the following way:
	//(x,a)(a,b)(b,x)
	
	SegList retList = new SegList();
	
	//all work is already done:
	if (((Segment)border.getFirst()).startpoint.equal(x)) {
	    return border; }

	Segment actSeg = new Segment();
	//find segment with (x,a)
	ListIterator it = border.listIterator(0);
	while (it.hasNext()) {
	    actSeg = (Segment)it.next();
	    if (actSeg.startpoint.equal(x)) {
		//found that segment!
		break; }
	}//while
	
	int posOfX = it.nextIndex()-1;

	//copy all following segments to retList
	retList.add(actSeg);
	while (it.hasNext()) {
	    retList.add(it.next()); }
	
	//now copy all segments from the beginning
	//up to the position of x
	if (!(posOfX == 0)) {
	    it = border.listIterator(0);
	    while (it.nextIndex() < posOfX) {
		retList.add(it.next()); }
	}//if

	return retList;
    }//end method resortBorder
	

    public ElemListListList cyclesSegments() {
	//returns the cycles of this as an ElemListList
	//For every component there is a single list.
	//The first element is the outer cycle and the 
	//following elements represent the hole cycles.

	//System.out.print("-->computing segments for NestedList conversion(Polygons.cyclesSegments)...");

	if (!this.borderDefined) this.border = computeBorder();
	//System.out.println("actual border("+border.size()+":"); border.print();

	if (this.border.isEmpty()) return new ElemListListList();

	//compute a sorted list (sorted by cycles)
	SegList sortedBorder = sortBorder(border,((Segment)border.getFirst()).startpoint);

	//System.out.println("sortedBorder:"); sortedBorder.print();
	//System.exit(0);

	//put every cycle in an own list
	ElemListList cycList = new ElemListList();
	int cycCount = 0;
	ListIterator lit = sortedBorder.listIterator(0);
	Segment actSeg = (Segment)sortedBorder.getFirst();
	Point startP = actSeg.startpoint;
	//add first element to cycle
	cycList.add(new ElemList());
	((ElemList)cycList.getFirst()).add(actSeg);
	if (lit.hasNext()) actSeg = (Segment)lit.next();
	else { return (new ElemListListList()); }
	while (lit.hasNext()) {
	    actSeg = (Segment)lit.next();
	    //add actual segment to actual cycle
	    ((ElemList)cycList.get(cycCount)).add(actSeg);
	    if (PointSeg_Ops.isEndpoint(startP,actSeg)) {
		//cycle completed
		if (lit.hasNext()) {
		    //there are more cycles
		    actSeg = (Segment)lit.next();
		    cycCount++;
		    startP = actSeg.startpoint;
		    cycList.add(new ElemList());
		    ((ElemList)cycList.get(cycCount)).add(actSeg);
		}//if
		else {
		    //there are no more cycles
		    break;
		}//else
	    }//if
	}//while
	
	//System.out.println("Cyclelist: ");
	//cycList.print();
	//System.exit(0);

	//find the outer cycles
	ElemListList outerCycles = new ElemListList();
	ElemListList innerCycles = new ElemListList();
	//lit = cycList.listIterator();
	SegList firstCyc;
	//while (lit.hasNext()) {
	for (int i = 0; i < cycList.size(); i++) {
	    firstCyc = SegList.convert((ElemList)cycList.get(i));
	    //ListIterator lit2 = cycList.listIterator(0);
	    SegList actCyc;
	    boolean inside;
	    //while (lit2.hasNext()) {
	    for (int j = 0; j < cycList.size(); j++) {
		//traverse the list and move all cycles that are inside of this
		//to innerCycles
		//if (!(lit.nextIndex() == lit2.nextIndex())) {
		if (i != j) {
		    actCyc = SegList.convert((ElemList)cycList.get(j));
		    //System.out.println("\nfirstCyc:"+i); firstCyc.print();
		    //System.out.println("actCyc:"+j); actCyc.print();
		    inside = Algebra.lr_inside(actCyc,computeTriangles(firstCyc));
		    if (inside) {
			//move actCyc to innerCycles
			innerCycles.add(actCyc);
			//System.out.println("------->move to innerCycles");
			//remove actCyc from cycList
			//lit2.remove();
			cycList.remove(j);
			if (j < i) i--;
			j--;
			//break;
		    }//if
		}//if
	    }//while
	}//while
	//all remaining cycles must be outer cycles
	//so move them to outerCycles
	lit = cycList.listIterator(0);
	while (lit.hasNext()) outerCycles.add(lit.next());

	//System.out.println("outerCycles:"); outerCycles.print();
	//System.out.println("\ninnerCycles:"); innerCycles.print();
	//System.exit(0);

	//find all the hole cycles that belong to every outer cycle
	//build resulting cycle list
	ElemListListList resList = new ElemListListList();
	lit = outerCycles.listIterator(0);
	while (lit.hasNext()) {
	    ElemListList outerList = new ElemListList();
	    outerList.add(lit.next());
	    resList.add(outerList);
	}//while

	lit = innerCycles.listIterator(0);
	while (lit.hasNext()) {
	    SegList actCyc = (SegList)lit.next();
	    ListIterator litO = resList.listIterator(0);
	    ElemListList actOuterList;
	    SegList actOuter;
	    while (litO.hasNext()) {
		actOuterList = (ElemListList)litO.next();
		actOuter = SegList.convert((ElemList)actOuterList.getFirst());
		if (Algebra.lr_inside(actCyc,computeTriangles(actOuter))) {
		    //add innerCycle to appropriate outerCycle in resList
		    actOuterList.add(actCyc);
		    //remove innerCycle from innerCycles
		    lit.remove();
		    break;
		}//if
	    }//while
	}//while

	//System.out.println("done.");

	return resList;
    }//end method cyclesSegments
	

    public ElemListListList cyclesPoints () {
	//returns the cycles of this as a ElemListListList
	//which has points as elements
	//System.out.print("-->computing cycles for NestedList conversion(Polygons.cyclesPoints)...");

	ElemListListList retList = new ElemListListList();
	ElemListListList cyc = this.cyclesSegments();

	//System.out.println("computed ElemListList using cyclesSegments: "); cyc.print();

	ListIterator lit1 = cyc.listIterator(0);
	ElemListList actComp;
	while (lit1.hasNext()) {
	    actComp = (ElemListList)lit1.next();
	    ElemListList actCompPL = new ElemListList();
	    SegList actCyc;
	    ListIterator lit2 = actComp.listIterator(0);
	    while (lit2.hasNext()) {
		actCyc = SegList.convert((ElemList)lit2.next());
		actCyc = sortBorder(actCyc,((Segment)actCyc.getFirst()).startpoint);
		PointList actPL = actCyc.generatePointList();
		actCompPL.add(actPL);
	    }//while
	    retList.add(actCompPL); 
	}//while
	//System.out.println("done.");
	return retList;
    }//end method cyclesPoints


} // end class Polygons


  
