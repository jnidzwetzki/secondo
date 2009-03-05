/*

---- 
This file is part of SECONDO.

Copyright (C) 2008, University in Hagen, Faculty of Mathematics and Computer Science, 
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

1 The BBTree class

The BBTree is a data structure which can be used to determine the bounding box
of a moving point for a determines time interval.

*/

#include "BBTree.h"
#include <stack>
#include <iostream>
#include "TemporalAlgebra.h"
#include "SpatialAlgebra.h"


/*
1 Class BBTreeNode

An instance of that class represents a single  node within a BBTree.

*/
class BBTreeNode{
public:
  
/*
1.1 Constructors

1.1.1 Constructor for a leaf

*/  
  BBTreeNode(const UPoint& unit){
     this->unit = new UPoint(unit);
     box = unit.BoundingBoxSpatial();
     left = 0;
     right = 0;
     interval = unit.timeInterval;
  }

/*
1.1.2 Constructor for inner nodes

*/

  BBTreeNode(BBTreeNode* left, BBTreeNode* right){
    unit = 0;
    box = left->box.Union(right->box);
    this->left = left;
    this->right = right;
    Instant minInst(instanttype);
    Instant maxInst(instanttype);
    if(left){
        minInst=left->interval.start;
        maxInst=left->interval.end;   
    }
    if(right){
       if(right->interval.start < minInst){
          minInst = right->interval.start;
       }
       if(right->interval.end > maxInst){
          maxInst = right->interval.end;
       }
    } 
    interval = Interval<Instant>(minInst, maxInst, true, true);
  }

/*
1.1.3 Copy Constructor 

performs a deepth copy

*/
BBTreeNode(const BBTreeNode& src){
   if(src.unit){
     unit = new UPoint(*src.unit);
   } else {
     unit = 0;
   }
   box = src.box;
   if(src.left){
     left = new BBTreeNode(*src.left);
   } else {
     left = 0;
   } 
   if(src.right){
     right = new BBTreeNode(*src.right);
   } else {
     right = 0;
   } 
   interval = src.interval;
}

/*
1.2 Assignment Operator

performs a deep copy

*/
  BBTreeNode& operator=(const BBTreeNode& src){
   if(src.unit){
     unit = new UPoint(*src.unit);
   } else {
     unit = 0;
   }
   box = src.box;
   if(src.left){
     left = new BBTreeNode(*src.left);
   } else {
     left = 0;
   } 
   if(src.right){
     right = new BBTreeNode(*src.right);
   } else {
     right = 0;
   } 
   interval = src.interval;
   return *this;
  }

/*
1.3 Destructor

*/
  ~BBTreeNode(){
      if(unit){
         delete unit;
         unit=0;
      }
      if(left){
        delete left;
        left = 0;
      }
      if(right){
        delete right;
        right = 0;
      }
   }


/*
1.4 ~getBox~

*/

Rectangle<2> getBox(const Interval<Instant> interval) const{

  //disjoint intervals -> return undef
  if(interval.Disjoint(this->interval)){
    Rectangle<2> res(false);
    return res;
  }
  if(this->interval.Inside(interval)){
    return box;
  }

  if(unit){ // a leaf node
     Instant minI(instanttype);
     if(interval.start<this->interval.start){
        minI = this->interval.start;
     } else {
        minI = interval.start;
     }  
     Instant maxI(instanttype);
     if(interval.end<this->interval.end){
        maxI = interval.end;
     } else {
        maxI = this->interval.end;
     }
     Point p0,p1;
     unit->TemporalFunction(minI,p0,true);
     unit->TemporalFunction(maxI,p1,true);
     assert(p0.IsDefined());
     assert(p1.IsDefined());
     return p0.BoundingBox().Union(p1.BoundingBox()); 
  } else { // an inner node
     Rectangle<2> Lres(false);
     if(left){
        Lres  = left->getBox(interval);
     }
     Rectangle<2> Rres(false);
     if(right){
        Rres = right->getBox(interval);
     }
     if(!Lres.IsDefined()){
        return Rres; 
     } else {
       if(!Rres.IsDefined()){
         return Lres;
       } else { // both are defined
         return Lres.Union(Rres);
       }
     }
  }
}

int noLeaves() const{
  if(unit){
     return 1;
  } else {
    int l = left?left->noLeaves():0;
    int r = right?right->noLeaves():0;
    return l+r;
  }
}

int noNodes() const{
    int l = left?left->noNodes():0;
    int r = right?right->noNodes():0;
    return l+r+1;
}

int height() const{
   if(unit) return 0;
   int l = left?left->height():0;
   int r = right?right->height():0;
   return max(l,r) +1;  
}

ostream& print(ostream& o) const{

  if(unit){
     o << "\"" << "U" << "\"";
     return o;
  } else {
     o << "(";
     o << "\"" << "I" << "\"";
     if(left){
       left->print(o);
     } else {
       o << "()";
     }
     if(right){
       right->print(o);
     } else {
       o << "()";
     }
     o << ")";
     return o;
  }
}


private:
  UPoint* unit;
  Rectangle<2> box;
  BBTreeNode* left;
  BBTreeNode* right;
  Interval<Instant> interval;
};



BBTree::BBTree(const MPoint& p):root(0){
   createFromMPoint(p);
}

BBTree::BBTree(const BBTree& t):root(0){
  if(t.root){
     root = new BBTreeNode(*t.root);
  } else {
     root = 0;
  }
}

BBTree& BBTree::operator=(const BBTree& src){
  if(src.root){
    root = new BBTreeNode(*src.root);
  } else {
    root = 0;
  }
  return *this;
}

BBTree::~BBTree(){
  if(root){
    delete root;
    root = 0;
  }
}

Rectangle<2> BBTree::getBox(Interval<Instant> interval)const{
   if(root){
     return root->getBox(interval);
   } else {
     Rectangle<2> res(false);
     return res;
   }
}

int BBTree::noLeaves() const{
  if(root) {
    return root->noLeaves();
  } else {
    return 0;
  }
}

int BBTree::noNodes() const{
  if(root) {
    return root->noNodes();
   } else {
    return 0;
   }
}
    
int BBTree::height() const{
  if(root){ 
    return root->height();
  } else {
    return -1;
  }
}

void BBTree::createFromMPoint(const MPoint& p){
   int size = p.GetNoComponents();
   if(size==0){
     root = 0;
     return;
   }
   stack<pair<int, BBTreeNode*> > astack;

   for(int i=0; i< size; i++){
      const UPoint* unit;
      p.Get(i,unit);
      BBTreeNode* newNode = new BBTreeNode(*unit);
      pair<int, BBTreeNode*> entry(0, newNode);
      if(astack.size()==0){ // first entry
         astack.push(entry);
      } else {
         pair<int, BBTreeNode*> top = astack.top();
         bool done = false;
         while(!done && (top.first == entry.first)){
           BBTreeNode* next = new  BBTreeNode(top.second,entry.second);
           astack.pop();
           entry = pair<int, BBTreeNode*> (top.first+1,next);
           done = astack.empty();
           if(!done){
               top = astack.top();
           }
         }
         astack.push(entry);
      } 
   }
   pair<int, BBTreeNode*> top = astack.top();
   BBTreeNode* r = top.second;
   astack.pop();
   while(!astack.empty()){
      top = astack.top();
      astack.pop();
      r = new BBTreeNode(top.second,r);
   }
   root = r;
}

ostream& BBTree::print(ostream& o) const{
 if(root){
    o << "( tree ";
    root->print(o);
    o << ")";
    return o;
 } else {
   o << "(tree ())";
   return o;
 }
}

ostream& operator<<(ostream& o, const BBTree& t){
  return t.print(o);
}



/*
2 Class BBTreeNode2

An instance of that class represents a single  node within a BBTree2.
Basically it's the same as the BBTreeNode2. the only difference is that in this
version doubles instead of Instant are used for the intervals. Unfortunately, 
the are some differences in double and datetime so that we cannot use a template
for it.


*/

static Interval<double> toDoubleInterval(const Interval<Instant>& iv){
  Interval<double> result(iv.start.ToDouble(), iv.end.ToDouble(), iv.lc, iv.rc);
  return result;
}

class BBTreeNode2{
public:
  
/*
2.1 Constructors

2.1.1 Constructor for a leaf

*/  
  BBTreeNode2(const UPoint& unit):
  unit(new UPoint(unit)),
  box(unit.BoundingBoxSpatial()),
  left(0), right(0),
  interval(toDoubleInterval(unit.timeInterval))
  {
  } 
/*
2.1.2 Constructor for inner nodes

*/

  BBTreeNode2(BBTreeNode2* left, BBTreeNode2* right){
    unit = 0;
    if(left){
      if(right){
        box = left->box.Union(right->box);
      } else {
        box = left->box;
      }
    } else {
      if(right){
        box = right->box;
      }
    }
    this->left = left;
    this->right = right;
    double min(numeric_limits<double>::max());
    double max(numeric_limits<double>::min());
    if(left){
        min=left->interval.start;
        max=left->interval.end;   
    }
    if(right){
       if(right->interval.start < min){
          min = right->interval.start;
       }
       if(right->interval.end > max){
          max = right->interval.end;
       }
    } 
    interval.start = min;
    interval.end = max;
    interval.lc = true;
    interval.rc = true;
  }

/*
2.1.3 Copy Constructor 

performs a deepth copy

*/
BBTreeNode2(const BBTreeNode2& src){
   if(src.unit){
     unit = new UPoint(*src.unit);
   } else {
     unit = 0;
   }
   box = src.box;
   if(src.left){
     left = new BBTreeNode2(*src.left);
   } else {
     left = 0;
   } 
   if(src.right){
     right = new BBTreeNode2(*src.right);
   } else {
     right = 0;
   } 
   interval.start = src.interval.start;
   interval.end = src.interval.end;
   interval.lc  = src.interval.lc;
   interval.rc = src.interval.rc;
}

/*
2.2 Assignment Operator

performs a deep copy

*/
  BBTreeNode2& operator=(const BBTreeNode2& src){
   if(src.unit){
     unit = new UPoint(*src.unit);
   } else {
     unit = 0;
   }
   box = src.box;
   if(src.left){
     left = new BBTreeNode2(*src.left);
   } else {
     left = 0;
   } 
   if(src.right){
     right = new BBTreeNode2(*src.right);
   } else {
     right = 0;
   } 
   interval.start = src.interval.start;
   interval.end = src.interval.end;
   interval.lc  = src.interval.lc;
   interval.rc = src.interval.rc;
   return *this;
  }

/*
2.3 Destructor

*/
  ~BBTreeNode2(){
      if(unit){
         delete unit;
         unit=0;
      }
      if(left){
        delete left;
        left = 0;
      }
      if(right){
        delete right;
        right = 0;
      }
   }


/*
2.4 ~getBox~

*/
inline bool ivInside(const Interval<double>& iv1, 
                     const Interval<double>& iv2) const{
  return( ( (iv1.start > iv2.start) || 
            (AlmostEqual(iv1.start, iv2.start) && ( !iv1.lc || iv2.lc ) ) ) &&
          ( (iv1.end < iv2.end) || 
            ( AlmostEqual(iv1.end, iv2.end) && ( !iv1.rc || iv2.rc ) ) ) );
}

inline bool ivDisjoint(const Interval<double>& iv1, 
                       const Interval<double>& iv2) const{
  return( ( (iv1.end < iv2.start) || 
            ( AlmostEqual(iv1.end, iv2.start) && ( !iv1.lc || !iv2.lc ) ) ) ||
          ( (iv1.start > iv2.end) || 
            ( AlmostEqual(iv1.start, iv2.end) && ( !iv1.rc || !iv2.rc ) ) ) );
}


Rectangle<2> getBox(const Interval<double> interval) const{

  //disjoint intervals -> return undef
  if(ivDisjoint(interval,this->interval)){
    Rectangle<2> res(false);
    return res;
  }
  if(ivInside(this->interval, interval)){
    return box;
  }

  if(unit){ // a leaf node
     double mind;
     if(interval.start<this->interval.start){
        mind = this->interval.start;
     } else {
        mind = interval.start;
     }  
     double maxd;
     if(interval.end<this->interval.end){
        maxd = interval.end;
     } else {
        maxd = this->interval.end;
     }
     Point p0,p1;
     unit->TemporalFunction(DateTime(mind),p0,true);
     unit->TemporalFunction(DateTime(maxd),p1,true);
     assert(p0.IsDefined());
     assert(p1.IsDefined());
     return p0.BoundingBox().Union(p1.BoundingBox()); 
  } else { // an inner node
     Rectangle<2> Lres(false);
     if(left){
        Lres  = left->getBox(interval);
     }
     Rectangle<2> Rres(false);
     if(right){
        Rres = right->getBox(interval);
     }
     if(!Lres.IsDefined()){
        return Rres; 
     } else {
       if(!Rres.IsDefined()){
         return Lres;
       } else { // both are defined
         return Lres.Union(Rres);
       }
     }
  }
}

int noLeaves() const{
  if(unit){
     return 1;
  } else {
    int l = left?left->noLeaves():0;
    int r = right?right->noLeaves():0;
    return l+r;
  }
}

int noNodes() const{
    int l = left?left->noNodes():0;
    int r = right?right->noNodes():0;
    return l+r+1;
}

int height() const{
   if(unit) return 0;
   int l = left?left->height():0;
   int r = right?right->height():0;
   return max(l,r) +1;  
}

ostream& print(ostream& o) const{

  if(unit){
     o << "\"" << "U" << "\"";
     return o;
  } else {
     o << "(";
     o << "\"" << "I" << "\"";
     if(left){
       left->print(o);
     } else {
       o << "()";
     }
     if(right){
       right->print(o);
     } else {
       o << "()";
     }
     o << ")";
     return o;
  }
}


private:
  UPoint* unit;
  Rectangle<2> box;
  BBTreeNode2* left;
  BBTreeNode2* right;
  Interval<double> interval;
};



BBTree2::BBTree2(const MPoint& p):root(0){
   createFromMPoint(p);
}

BBTree2::BBTree2(const BBTree2& t):root(0){
  if(t.root){
     root = new BBTreeNode2(*t.root);
  } else {
     root = 0;
  }
}

BBTree2& BBTree2::operator=(const BBTree2& src){
  if(src.root){
    root = new BBTreeNode2(*src.root);
  } else {
    root = 0;
  }
  return *this;
}

BBTree2::~BBTree2(){
  if(root){
    delete root;
    root = 0;
  }
}

Rectangle<2> BBTree2::getBox(Interval<double> interval)const{
   if(root){
     return root->getBox(interval);
   } else {
     Rectangle<2> res(false);
     return res;
   }
}

int BBTree2::noLeaves() const{
  if(root) {
    return root->noLeaves();
  } else {
    return 0;
  }
}

int BBTree2::noNodes() const{
  if(root) {
    return root->noNodes();
   } else {
    return 0;
   }
}
    
int BBTree2::height() const{
  if(root){ 
    return root->height();
  } else {
    return -1;
  }
}

void BBTree2::createFromMPoint(const MPoint& p){
   int size = p.GetNoComponents();
   if(size==0){
     root = 0;
     return;
   }
   stack<pair<int, BBTreeNode2*> > astack;

   for(int i=0; i< size; i++){
      const UPoint* unit;
      p.Get(i,unit);
      BBTreeNode2* newNode = new BBTreeNode2(*unit);
      pair<int, BBTreeNode2*> entry(0, newNode);
      if(astack.size()==0){ // first entry
         astack.push(entry);
      } else {
         pair<int, BBTreeNode2*> top = astack.top();
         bool done = false;
         while(!done && (top.first == entry.first)){
           BBTreeNode2* next = new  BBTreeNode2(top.second,entry.second);
           astack.pop();
           entry = pair<int, BBTreeNode2*> (top.first+1,next);
           done = astack.empty();
           if(!done){
               top = astack.top();
           }
         }
         astack.push(entry);
      } 
   }
   pair<int, BBTreeNode2*> top = astack.top();
   BBTreeNode2* r = top.second;
   astack.pop();
   while(!astack.empty()){
      top = astack.top();
      astack.pop();
      r = new BBTreeNode2(top.second,r);
   }
   root = r;
}

ostream& BBTree2::print(ostream& o) const{
 if(root){
    o << "( tree ";
    root->print(o);
    o << ")";
    return o;
 } else {
   o << "(tree ())";
   return o;
 }
}

ostream& operator<<(ostream& o, const BBTree2& t){
  return t.print(o);
}

