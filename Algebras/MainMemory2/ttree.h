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

*/

#ifndef TTREE_H
#define TTREE_H

#include <assert.h>
#include <cstdlib> 
#include <utility>
#include <iostream>
#include <stack>
#include <queue>
#include <vector>
#include <stdlib.h>
#include <algorithm>

#include "MainMemoryExt.h"



namespace ttree{

  
  
  

/*
0. Forward declaration of the TTree class

*/
template <class U, class Comp>
class TTree;


/*
1 class TTreeNode

The class TTreeNode is a class for the nodes of an t-tree.

*/
template<class T, class Comparator>
class TTreeNode{

  template<class U,class Comp> friend class TTree;
    public:
  

/*
1.1 Constructor

This constructor initializes all members

*/
      // creates a new T-Tree node 
      TTreeNode(const size_t _minEntries, const size_t _maxEntries): 
         
        minEntries(_minEntries), maxEntries(_maxEntries), 
        left(0), right(0), count(0), height(0) {
          
          objects = new T*[TTreeNode<T,Comparator>::maxEntries];
          for(size_t i=0;i<TTreeNode<T,Comparator>::maxEntries;i++){
            objects[i] = 0;
          }
        }
        
/*
1.2 Copy constructor

Creates a depth copy of this node.

*/
    TTreeNode(const TTreeNode<T,Comparator>& source):
      objects(source.objects), left(0), right(0), height(source.height){
      
      this->left = 
        source.left==NULL?NULL:new TTreeNode<T,Comparator>(*source.left);
      this->right = 
        source.right==NULL?NULL:new TTreeNode<T,Comparator>(*source.right);
    }

/*
1.3 Assignment Operator

*/
  TTreeNode<T,Comparator>& operator=(const TTreeNode<T,Comparator>&  source) {
     this->objects = source.objects;
     this->left = source.left
                  ? new TTreeNode<T,Comparator>(source.left)
                  :NULL;
     this->right = source.right
                   ? new TTreeNode<T,Comparator>(source.right)
                   :NULL;
     this->height = source.height;
     return *this;
  }
      
/*
1.4 Destructor

Destroy the subtree rooted by this node

*/
     
      ~TTreeNode() {

        for(size_t i=0; i<maxEntries; i++) {
          if(objects[i]) {
            delete objects[i];
          }
        }
        delete[] objects;
        if(left) delete left;
        if(right) delete right;
        
        left = NULL;
        right = NULL;
      }

      
      
/*
1.5 Methods

~isLeaf~

This function returns true if this node represents a leaf.

*/
  
      bool isLeaf() const {
        if(left || right)
          return false;
        return true;
      }

/*
~memSize~

Returns the amount of memory occupied by the subtree represented by this node.

*/

      size_t memSize() const {
        size_t res = sizeof(*this) + 
                     sizeof(void*) * TTreeNode<T,Comparator>::maxEntries;
        
        for(int i=0;i<count;i++){
          res += sizeof(* objects[i]);
        }
        
        if(left)
          res += left->memSize();
        if(right)
          res += right->memSize();
        return res;  
      }

      

/*
~getObject~

Returns the content of this node at position i.

*/
      T* getObject(const int i) const {
        return objects[i];
      }
      

/*
~getMinEntries~

Return the minimum number of entries of this node.

*/
      inline size_t getMinEntries() const{
        return minEntries;
      }
   

/*
~getMaxEntries~

Returns the capacity of this node.

*/
      inline int getMaxEntries() const{
        return maxEntries;
      }

     
/*
~getCount~

Returns the number of entries of this node.

*/
      inline size_t getCount() const{
        return count;	
      }


/*
~getLeftSon~

Returns the parent of this node or 0 if this node is the root of the tree.

*/
      inline TTreeNode<T,Comparator>* getLeftSon(){
        return left;
      }
      

/*
~getRightSon~

Returns the parent of this node or 0 if this node is the root of the tree.

*/
      inline TTreeNode<T,Comparator>* getRightSon(){
        return right;
      }
      
/*
~insertionSort~

Sorts the elements of this node with insertionSort.

*/
      void insertionSort (std::vector<int>* attrPos){
        int j;
        T* temp;
                      
        for (size_t i = 0; i < count; i++){
          j = i;
                
          while (j>0 && Comparator::smaller(*this->objects[j],
                                    *this->objects[j-1],attrPos)) {
            temp = this->objects[j];
            this->objects[j] = this->objects[j-1];
            this->objects[j-1] = temp;
            j--;
          }
        }
      }


/*
~updateNode~

Updates the minimum and maximum values in this node after sorting
it's elements.

*/
    void updateNode(std::vector<int>* attrPos) {
      if(count > 1) {
        insertionSort(attrPos);
      }
    }
    

/*
~clear~

Removes the entries from this node. If deleteContent is set to be
true, the sons of this node are destroyed.

*/
      void clear(const bool deleteContent) {
        for(size_t i=0;i<count;i++) {
          if(deleteContent){
            delete objects[i];
          }
          objects[i] = 0;
        }
        if(deleteContent) {
          if(left)
            delete left;
          if(right)
            delete right;
        }
        left = 0;
        right = 0;
        count = 0;
      }

/*
~updateHeight~

Computes the height of this node from the height of
the sons. This method should be called whenever the 
sons are changed. Note that only the direct descendants
are used to compute the new height.

*/
      void updateHeight() {
        int h1 = left==NULL?0:left->height+1;
        int h2 = right==NULL?0:right->height+1;
        height = std::max(h1,h2);  
      }


/*
~balance~

Computes the balance of this node.

*/
      int balance()const{
        int h1 = 0;
        int h2 = 0;
        if(left) 
          h1 = left?left->height+1:0;
        
        if(right)
          h2 = right?right->height+1:0;
        
        return h1-h2;
      }


/*
~print~

write a textual representation for this node to ~out~.

*/
  
      std::ostream& print(std::ostream& out) const{
         out << "(";
      for(int i=0; i<count; i++) {
        
        out << i <<  ":" << " '" << *objects[i] <<"' ";
        
      }
      out << ")";
         return out;
      }
    
      void printtree(std::ostream& out){
          printtree(this,out);
      }


      static void printtree(const TTreeNode<T,Comparator>* root, 
                            std::ostream& out, bool isRoot = true){
        if(isRoot){
          out << "( tree ";
        }
        if(!root){
           out << "' '" << endl;
        } else {
          out << "(";
          out << "'";
          for(size_t i=0;i<root->getCount();i++){
             if(i>0) out << ", ";
             out << *(root->getObject(i));
          }
          out << "'  ";
          printtree(root->left,out,false);
          printtree(root->right,out, false);
          out << " )";
        }
        if(isRoot){
          out << ")";
          out << endl;
        }
    }

      const T& getMinValue(){ 
         assert(count>0);
         return *(objects[0]);
      }

      const T& getMaxValue(){ 
        assert(count>0);
        return *(objects[count-1]);
      }

/*
~hasSpace~

The function checks whether the node can include a further entry.

*/
    inline bool hasSpace() const{
      return count < maxEntries-1;
    }    
  
/*
~insert~

Inserts a new entry into a node having enough space.
The new entry should be created exclusively for the tree,
this method does not create a copy from it.

*/
    void insert(T* value, std::vector<int>* attrPos){
      assert(hasSpace());
      // search insertion position
      size_t pos = 0; 
      while(     (pos < count) 
            &&!Comparator::greater( *objects[pos],*value,attrPos)){
          pos++;
      }    
      for(size_t i = count ; i > pos; i--){
        objects[i] = objects[i-1];
      }    
      objects[pos] = value;
      count++;
    }   


/*
~replaceGreatest~

Replaces the last element of this node with a new value. The old
last element is returned. If the node is empty, null is returned.

*/
  T* replaceGreatest(T* newValue, std::vector<int>* attrPos){
    T* res;
    if(count==0){
        res = 0;
    } else {
      // remove the greatest object
      res = objects[count-1];
      objects[count-1] = 0;
      count--;
    }
    insert(newValue,attrPos);
    return res;
  }



   // this method checks for the subtree represented by this
   // node whether the count condition of a ttree is fullfilled   
   bool checkCount(){
       if(count ==0){
           cout << "found node without elements" << endl;
           return false;
       }
       if(count > maxEntries){
          cout << "found node having more entries than allowed" << endl;
       }
       if(left || right){ // inner node
          if(count < minEntries){
             cout << "found inner node having too less entries" << endl;
             return false;
          }
       }

       bool ok = true;
       if(left){
         ok = ok && left->checkCount();
       }
       if(right){
         ok = ok && right->checkCount();
       }
       return ok;
   }

   bool checkOrder(const T* min, const T* max, std::vector<int>* attrPos){
       // check internal order
       for(size_t i=0;i<count-1; i++){
          if(Comparator::greater(*objects[i], *objects[i+1],attrPos)){
             cout << "found invalid order within a single node" << endl;
             return false;
          }
       }
       if(min && Comparator::smaller(getMinValue(),*min, attrPos) ){
          cout << "found value of a node smaller than allowed" << endl;
          return false; 
       }
       if(max && Comparator::greater(getMaxValue(), *max, attrPos)){
          cout << "found value of a node greater than allowed" << endl;
          return false; 
       }
       bool ok = true;
       if(left){
          ok = ok && left->checkOrder(min, objects[0], attrPos);
       }
       if(right){
          ok = ok && right->checkOrder(objects[count-1], max, attrPos);
       }
       return ok;
   }


 

      
    protected:

/*
1.6 Member variables

*/
      size_t minEntries;
      size_t maxEntries;
      TTreeNode<T,Comparator>* left;
      TTreeNode<T,Comparator>* right;
      size_t count;
      size_t height;
      T** objects;  
};




/*
2 Class Iterator

This inner class provides an iterator for sorted iterating 
trough the tree.

*/
template<class T, class Comparator>
class Iterator{

  public:
 

    Iterator():stack(),pos(0){}

      
/*
2.1 Constructors

Creates an iterator for the given tree. The position 
is set to the smallest entry in the tree.

*/    
    Iterator(TTreeNode<T,Comparator>* root) {
      TTreeNode<T,Comparator>* son = root;
      while(son){
        stack.push(son);
        son = son->getLeftSon();   
      }
      pos = 0;
    }

/*

2.2 Tail constructor

Creates a constructor for a tree using a 
minimum value.

*/
    Iterator(TTreeNode<T,Comparator>* root,
             const T& minV){
       TTreeNode<T,Comparator>* son = root;
       pos = 0; 
       while(son){
         if(Comparator::smaller(minV,son->getMinValue(),0)){
            // value smaller than all entries
            stack.push(son);
            son = son->getLeftSon();
         } else if(Comparator::greater(minV,son->getMaxValue(),0)){
           // value greater than all entries
           son = son->getRightSon();
         } else {
           // value is inside the node
           stack.push(son);
           while(pos<son->getCount()){
             if(!Comparator::smaller(*(son->getObject(pos)),minV,0)){
                return;
             }    
             pos++;
           }    
           // should never be reached
         }    
       }    
   }


    
/*
2.2 Copy Contructor

*/    
    Iterator(const Iterator& it){
       this->stack = it.stack;
       this->pos = it.pos;         
    }

    
/*
2.2 Assignment Operator

*/ 
    Iterator& operator=(const Iterator& it){
       this->stack = it.stack;
       this->pos = it.pos;
       return *this; 
    }

/*
2.3 Increment operator

This operator sets this iterator to the next position 
in the tree.

*/
    Iterator& operator++(int){
      next();
      return *this;
    }

/*
2.4 Dereference operator

This operator returns the element currently under the 
iterator's cursor. If no more elements exist, NULL
is returned.

*/  
    T operator*() {
       assert(!stack.empty());
       const TTreeNode<T,Comparator>* elem = stack.top();
       assert(elem);
       assert(pos < elem->getCount());
       return *(elem->getObject(pos));
    } 
    
    
/*
2.5 Methods

~get~

The ~get~ function returns the value currently under this 
iterator at position i.

*/
  /*
    T get(int i) const {
      const TTreeNode<T,Comparator>* elem = stack.top();
      return *elem->getObject(i);
    }
  */
     
     
/*
~next~

The ~next~ function sets the iterator to the next element. 
If no more elements are available, the result will be __false__
otherwise __true__.

*/
   bool next(){
     if(stack.empty()){
        return false;
     }
     // there are elemnts in the node left
     TTreeNode<T,Comparator>* elem = stack.top();
     if(pos < elem->getCount()-1) {
       pos++;
       return true;
     }

     stack.pop(); // the current node is processed
     pos = 0;     // try to find an unprocessed node
     // go to first element in next node
     TTreeNode<T,Comparator>* son;
     if((son = elem->getRightSon())) {
        stack.push(son);
        while((son = son->getLeftSon())){
          stack.push(son);
        }
        return true;
     } else { // there is no right son
        return !stack.empty();
     }
   }
  

/*
~hasNext~

This function checks whether after the current element
further elements exist.

*/
    
    bool hasNext(){
      int size = stack.size();
      if(size==0){ // no elements
        return false;
      } 
      if(size>1){ // at least one nn processed node 
        return true;
      }
      TTreeNode<T,Comparator>* elem = stack.top();
      if(pos < elem->getCount()-1){ // node has unprocessed  elements
        return true;
      } 
      return (elem->getRightSon()!=0);
    }

    
/*
~end~

The ~end~ function checks whether the iterator does not have
any elements, i.e. whether Get or [*] would return NULL. 

*/
    bool end(){
      return stack.empty();
    }


  private:
 
     std::stack<TTreeNode<T,Comparator>*> stack;       
     size_t pos;     
};


/*
3 class TTree

This is the main class of this file. It implements a mainmemory-based TTree.

*/
template<class T, class Comparator>
class TTree {
  public:

/*
3.1 Constructor

Creates an empty tree.

*/
    TTree(int _minEntries, int _maxEntries): 
         minEntries(_minEntries), maxEntries(_maxEntries), root(0) {}

/*
3.2 Destructor

Destroys this tree.

*/
    ~TTree(){
       if(root!=NULL) {
         delete root;
         root = NULL;
       }
     }


/*
3.3 Methods

~begin~

This function creates an iterator which is positioned to the
first element in the tree .

*/

    Iterator<T,Comparator> begin() {
      Iterator<T,Comparator> it(root);
      return it;
    }
   
    Iterator<T, Comparator> tail(const T& minV){
      Iterator<T,Comparator> it(root,minV);
      return it;
    }
 
    

/*
~noEntries~

Returns the height of this tree.

*/
    int noEntries() {
      int count;
      noEntries(root,count);
      return count;
    }

    
/*
~isEmpty~

Returns true if this tree is empty.

*/    
    bool isEmpty() const {
       return root==NULL;
    }


/* 
~memSize~

Returns the memory size of this tree.

*/   
    size_t memSize() const {
       size_t res = sizeof(*this);  // size of empty tree
       if(root){
         res += root->memSize();
       }
       return res;
    }



/*
~print~

Prints this tree to the give ostream. 
The format is understood by the tree viewer of Secondo's Javagui.

*/

    void printttree(std::ostream& out) const {
      out << "( tree (" << std::endl;
      if(root) 
        printttree(root, out);
      else
        out << "empty";
      out << "))" << std::endl;
      
    } 
    
    void printorel(std::ostream& out) const {
      out << "( tree (" << std::endl;
      if(root) 
        printorel(root, out);
      else
        out << "empty";
      out << "))" << std::endl;
      
    } 
    

    
/*
~update~

Updates a given value in this tree with the second value.

*/
    bool update(T& value, T& newValue, std::vector<int>* attrPos){
      bool success;
      root = update(root,value,newValue,attrPos,success);
      return success;
    }
    
    bool update(T& value, T& newValue) {
      std::vector<int>* attrPos = new std::vector<int>();
      attrPos->push_back(1);
      attrPos->push_back(2);
      bool success = update(root,value,newValue,attrPos,success);
      delete attrPos;
      return success;
    }
    
    
/*
~insert~

Adds object ~o~ to this tree.

*/
    bool insert(T& value, std::vector<int>* attrPos) {
      bool success;
      root = insert(root,value,attrPos,success);
      root->updateHeight();
      return success;
    }
    
    bool insert(T& value, int i) {
      std::vector<int>* attrPos = new std::vector<int>();
      attrPos->push_back(i);
      bool success = insert(value,attrPos);
      attrPos->clear();
      delete attrPos;
      return success;
    }
    
    bool insert(T& value) {
      std::vector<int>* attrPos = new std::vector<int>();
      attrPos->push_back(1);
      bool success = insert(value,attrPos);
      attrPos->clear();
      delete attrPos;
      return success;
    }
    

/*
~remove~

Removes object ~o~ from this tree.

*/
    bool remove(T& value, std::vector<int>* attrPos){
      bool success;
      if(!root)
        return false;

      root = remove(root,value,attrPos,success);
      if(root) {
        root->updateHeight();
      }
      return success;
    }
    
    bool remove(T& value){
      bool success;
      if(!root) {
        return false;
      }
      std::vector<int>* attrPos = new std::vector<int>();
      attrPos->push_back(1);
      root = remove(root,value,attrPos,success);
      if(root) {
        root->updateHeight();
      }
      delete attrPos;
      return success;
    }
    
    
/*
~clear~


*/
    void clear(const bool deleteContent) {
      root->clear(deleteContent);
    }
   
    
   private:
     int minEntries;
     int maxEntries;
     TTreeNode<T,Comparator>* root;  

     
     
/*
~noEntries~

Computes the height of this node from the height of
the sons. This method should be called whenever the 
sons are changed. Note that only the direct descendants
are used to compute the new height.

*/
    int noEntries(TTreeNode<T,Comparator>* root, int &count) {
      if(root == 0)
        return 0;
      
      if(root->left) {
        noEntries(root->left,count);
      }
      if(root->right) {
        noEntries(root->right,count);
      }
      count += root->count;
      return count;
    }

       
     
/*
~insert~

This function inserts __value__ into the subtree given by root.
Duplicates are allowed

It returns the root of the new tree.

*/

    TTreeNode<T,Comparator>* insert(TTreeNode<T,Comparator>* root, 
                                    T& value,
                                    std::vector<int>* attrPos,
                                    bool& success) {
       T* v = new T(value);

       TTreeNode<T,Comparator>* res = insert(root,v,attrPos,success);
       if(!success){
          delete v;
       }

       return res;

    }
    

    TTreeNode<T,Comparator>* insert(TTreeNode<T,Comparator>* root, 
                                    T* value,
                                    std::vector<int>* attrPos,
                                    bool& success) {
      // leaf reached
      if(root == NULL){ 
        root = new TTreeNode<T,Comparator>(minEntries,maxEntries);
        root->insert(value, attrPos);
        success = true;
        return root;
      }
     
      if(root->isLeaf() && root->hasSpace()){
         root->insert(value,attrPos);
         success = true;   
         return root;
      }

 
      if(   !Comparator::smaller(*value,root->getMinValue(),attrPos)
         && !Comparator::greater(*value,root->getMaxValue(), attrPos)){
         // bounding node
         if( root->hasSpace()){
           // we have space, insert here
           root->insert(value,attrPos);
           success = true;   
           return root;
         } else {
         // there is no space, replace value with the 
         // rightmost value in the node
           value=root->replaceGreatest(value,attrPos);
        }
      }
      
      

      // search in the left subtree
      if(!Comparator::greater(*value, root->getMinValue(), attrPos)){
        root->left = insert(root->left,value,attrPos,success);
        root->updateHeight();
        // rotation or double rotation required
        if(abs(root->balance()) > 1) { 
          // single rotation is sufficient
          if(root->left->balance() > 0) {
            return rotateRight(root); 
          }
          // left-right rotation is required
          if(root->left->balance() < 0) {
            return rotateLeftRight(root,attrPos);
          } 
          assert(false); // should never be reached
          return NULL; 
        } 
        // root remains balanced
        else { 
          success = true;
          return root;
        }
        assert(false);
      }
      
      // search right subtree
      if(!Comparator::smaller(*value,root->getMaxValue(),attrPos)) {
        root->right = insert(root->right,value,attrPos,success);
        root->updateHeight();
        // rotation or double rotation required
        if(abs(root->balance())>1) {
          // single left rotation sufficient
          if(root->right->balance() < 0) { 
            return rotateLeft(root);
          }

          // right-left rotation required
          if(root->right->balance() > 0){
            return rotateRightLeft(root,attrPos);
          }
          assert(false); // should never be reached
          success = false;
          return NULL;
        } 
        // no rotation required
        else { 
          success = true;
          return root;
        }
      }
      assert(false);
      return 0;  // should never be reached
    }

    
    void printNodeInfo(TTreeNode<T,Comparator>* node) {
      std::cout << std::endl << "-->NODE INFO<---------------" << std::endl;
      node->print(std::cout);
      std::cout << std::endl;
      std::cout << "minValue: " << node->minValue << std::endl;
      std::cout << "maxValue: " << node->maxValue << std::endl;
      std::cout << "count: " << node->count << std::endl;
      if(node->isLeaf())
        std::cout << "isLeaf: " << "yes" << std::endl;
      else
        std::cout << "isLeaf: " << "no" << std::endl;
      std::cout << "------------------>NODE INFO<--" << std::endl << std::endl;
    }
    
    
/*
~remove~

This function removes __value__ from the subtree given by root.

*/
    TTreeNode<T,Comparator>* remove(TTreeNode<T,Comparator>* root, 
                                    T& value,
                                    std::vector<int>* attrPos,
                                    bool& success) {
      
      if(root->count == 1 && root->isLeaf()) {
        if(Comparator::equal(value,root->getMinValue(),attrPos)) {
          delete root;
          root = 0;
          success = true;
          return root;
          
        }
      }
      
      // search in the left subtree
      if(root->left && 
            Comparator::smaller(value,root->getMinValue(),attrPos)) { 
        root->left = remove(root->left,value,attrPos,success);
        root->updateHeight();
        root->updateNode(attrPos);
        if(root->right==NULL){ // because we have deleted  
                               // in the left part, we cannot 
                               // get any unbalanced subtree when right is null
         return root; 
      }
        
        if(abs(root->balance())>1) {
          // single left rotation sufficient
          if(root->right->balance() <= 0) { 
            return rotateLeft(root);
          }
          // right-left rotation required
          if(root->right->balance() > 0){
            return rotateRightLeft(root,attrPos);
          }
          assert(false); // should never be reached
          return NULL;
        } 
        // no rotation required
        else { 
          return root;
        }       
      }
      
      // search right subtree
      else if(root->right && 
            Comparator::greater(value,root->getMaxValue(),attrPos)) {
        root->right = remove(root->right,value,attrPos,success);
        root->updateHeight();
        root->updateNode(attrPos);
        if(root->left==NULL){
         return root;
       }
        
        // rotation or double rotation required
        if(abs(root->balance()) > 1) { 
          // single rotation is sufficient
          if(root->left->balance() >= 0) { 
            return rotateRight(root); 
          }
          // left-right rotation is required
          if(root->left->balance() < 0) {
            return rotateLeftRight(root,attrPos);
          } 
          assert(false); // should never be reached
          return NULL; 
        } 
        // root remains balanced
        else { 
          return root;
        }
      }
      
      // bounding node found, searching for value
      else {
        if(root->count > root->minEntries || root->isLeaf()) {
          for(size_t i=0; i<root->count; i++) {
            if(Comparator::equal(value,*root->objects[i],attrPos)) {
              // swap value to be deleted with last element
              swap(root,i,root,root->count-1);
              // delete last element
              remove(root,root->count-1);
              root->updateNode(attrPos);
              success = true;
              return root;
            }
          }
          success = false;
          return root;
        }
        else { // count < minEntries
          for(size_t i=0; i<root->count; i++) {
            if(Comparator::equal(value,*root->objects[i],attrPos)) {
              
              if(root->left) {
                swap(root,i,root->left,root->left->count-1);
                root->left = remove(root->left,value,attrPos,success);
                root->updateHeight();
                return root;
              }
              else {
                swap(root,i,root->right,0);
                root->right = remove(root->right,value,attrPos,success);
                root->updateHeight();
                return root;
              }
            }
          }
        }
      }
      return 0;  // should never be reached
    }
    
    
/*
~update~

This function updates __value__ in the subtree given by root.

*/
    TTreeNode<T,Comparator>* update(TTreeNode<T,Comparator>* root, 
                                    T& value,
                                    T& newValue,
                                    std::vector<int>* attrPos,
                                    bool& success) {
      
      root = remove(root,value,attrPos,success);
      root = insert(root,newValue,attrPos,success);
      
      return root;  
    }
    
    
/*
~swap~

Swaps the element at position i in node __root__ with the element
at position j in __node__.

*/      
    void swap(TTreeNode<T,Comparator>* root, int i, 
              TTreeNode<T,Comparator>* node, int j) {
      T* tmp = root->objects[i];     
      root->objects[i] = node->objects[j];     
      node->objects[j] = tmp;
    }
    
    
/*
~remove~

Removes the element at position i in the node __root__.

*/    
    void remove(TTreeNode<T,Comparator>*& root, int i) {
      
      delete root->objects[i];
      root->objects[i] = 0;
      root->count--;
      
      if(root->count < 1) {
        delete root;
        root=0;
      }
    
      
    }
   
   
/*
~rotateRight~

Performs a right rotation in the subtree given by root.

*/    
    TTreeNode<T,Comparator>* rotateRight(TTreeNode<T,Comparator>* root) {
      TTreeNode<T,Comparator>* y = root->left;
      TTreeNode<T,Comparator>* B = y->right;
      TTreeNode<T,Comparator>* C = root->right;
      root->left=B;
      root->right=C;
      root->updateHeight();
      y->right=root;
      y->updateHeight();
      return y;
    }

    
/*
~rotateRightLeft~

Performs a right-left rotation in the subtree given by root.

*/    
    TTreeNode<T,Comparator>* rotateRightLeft(TTreeNode<T,Comparator>* root, 
                                             std::vector<int>* attrPos) {
      
      TTreeNode<T,Comparator>* x = root;
      TTreeNode<T,Comparator>* z = x->right;
      TTreeNode<T,Comparator>* y = z->left;
      TTreeNode<T,Comparator>* B1 = y->left;
      TTreeNode<T,Comparator>* B2 = y->right;
      
      //special case:
      //if y is leaf, move entries from z to y till y has the 
      // minimum number of entries
      // y contains smaller values than z, so we have to
      // move from the beginning of z to the end of y
     

      if(y->isLeaf() && (y->getCount() < y->getMinEntries())){
         size_t entriesToTransfer = y->getMinEntries() - y->getCount();
         assert(z->getCount() > entriesToTransfer);

         // transfer the entries
         for(size_t i=0; i< entriesToTransfer; i++){
            y->objects[y->getCount()+i] = z->objects[i];
         }
         // move entries in z to left
         for(size_t i=0; i< z->getCount() - entriesToTransfer; i++){
            z->objects[i] = z->objects[i+entriesToTransfer];
         } // fill z up with 0
         for(size_t i = z->getCount() - entriesToTransfer ; 
            i< z->getCount(); i++){
            z->objects[i] = 0;
         }
         y->count += entriesToTransfer;
         z->count -= entriesToTransfer;
      }


      x->right=B1;
      x->updateHeight();
      z->left = B2;
      z->updateHeight();
      y->left = x;
      y->right=z;
      y->updateHeight();
      return y;
    }
    
    
/*
~rotateLeft~

Performs a left rotation in the subtree given by root.

*/    
    TTreeNode<T,Comparator>* rotateLeft(TTreeNode<T,Comparator>* root) {
      TTreeNode<T,Comparator>* y = root->right;
      TTreeNode<T,Comparator>* B = y->left;
      root->right = B;
      root->updateHeight();
      y->left=root;
      y->updateHeight();
      return y;
    }

    
/*
~rotateLeftRight~

Performs a left-right rotation in the subtree given by root.

*/     
    TTreeNode<T,Comparator>* rotateLeftRight(TTreeNode<T,Comparator>* root, 
                                             std::vector<int>* attrPos) {
      
      TTreeNode<T,Comparator>* x = root;
      TTreeNode<T,Comparator>* z = root->left;
      TTreeNode<T,Comparator>* y = z->right;
      TTreeNode<T,Comparator>* A = z->left;
      TTreeNode<T,Comparator>* B = y->left;
      TTreeNode<T,Comparator>* C = y->right;
     
      // special case:
      // if y is leaf, move entries from z to y till y is full node
      if(y->isLeaf() && (y->getCount() < y->getMinEntries())) {
        size_t entriesToMove = y->getMinEntries() - y->getCount();
        // create place in the beginning of y
        for(size_t i = y->getCount() ; i>0; i--){
           y->objects[i+entriesToMove -1] = y->objects[i -1];
        }
        // move entries from z to y
        for(size_t i=0;i<entriesToMove; i++){
           size_t z_i = z->getCount()  - entriesToMove + i;
           y->objects[i] = z->objects[z_i];
           z->objects[z_i] = 0;
        }
        y->count += entriesToMove;
        z->count -= entriesToMove;
      }    
      
      z->left=A;
      z->right=B;
      z->updateHeight();
      x->left=C;
      x->updateHeight();
      y->left=z;
      y->right=x;
      y->updateHeight();
      return y;
    }
	
/*
~print~

Prints the subtree given by root to the console.

*/   
    void printttree(const TTreeNode<T,Comparator>* root, 
                    std::ostream& out) const {
      if(!root){
        out << " 'empty' ";
        return;
      }
      
      out << " -->ROOT(";
      for(int i=0; i<root->count; i++) {
        out << i <<  ":" << " '" << *root->objects[i] <<"' ";
      }
      out << ") \n";
      
      out << " ----->LEFT(";
      printttree(root->left,out);
      out << ") \n";
      
      out << "         ------->RIGHT(";
      printttree(root->right,out);
      out << ") \n";
      
    }
    
    
    void printorel(const TTreeNode<T,Comparator>* root, 
                   std::ostream& out) const {
      if(!root){
        out << " 'empty' ";
        return;
      }
      
      out << " -->ROOT(";
      for(int i=0; i<root->count; i++) {
        out << i <<  ":" << " '" << **root->objects[i] <<"' ";
      }
      out << ") \n";
      
      out << " ----->LEFT(";
      printorel(root->left,out);
      out << ") \n";
      
      out << "         ------->RIGHT(";
      printorel(root->right,out);
      out << ") \n";
      
    }


 
};


} // end namespace

#endif

