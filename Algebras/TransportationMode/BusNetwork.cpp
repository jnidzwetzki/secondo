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

//paragraph [1] Title: [{\Large \bf \begin {center}] [\end {center}}]
//[TOC] [\tableofcontents]

[1] Source File of the Transportation Mode Algebra

Oct, 2010 Jianqiu Xu

[TOC]

1 Overview

This source file essentially contains the necessary implementations for
creating bus network 

*/
#include "BusNetwork.h"

/*
create route bus route 
a pair of cells, start cell-end cell.
the start cell can be considered as the start location of a bus route
and the end cell is the end location of a bus route.
Then it randomly selectes two locations in these two cells 

*/
void BusRoute::CreateRoute1(int attr1,int attr2,int attr3,int attr4)
{
//  cout<<"attr1 "<<attr1<<" attr2 "<<attr2<<" attr3 "<<attr3<<endl; 
//  cout<<"CreateBusRoute()"<<endl; 

  int max_cell_id = -1; 
  int max_cnt = -1;
  for(int i = 1;i <= rel1->GetNoTuples();i++){
    Tuple* tuple_cell = rel1->GetTuple(i,false);
    CcInt* cell_id = (CcInt*)tuple_cell->GetAttribute(attr2); 
    CcInt* cnt = (CcInt*)tuple_cell->GetAttribute(attr3); 
    
//    cout<<sec_id->GetIntval()<<" "
//        <<cell_id->GetIntval()<<" "<<cnt->GetIntval()<<endl; 
  
    if(cell_id->GetIntval() > max_cell_id)
        max_cell_id = cell_id->GetIntval(); 
    
    if(cnt->GetIntval() > max_cnt) 
      max_cnt = cnt->GetIntval(); 
      
    tuple_cell->DeleteIfAllowed(); 
  }

//  cout<<"max_cell_id "<<max_cell_id<<endl;
//    cout<<"max_cnt "<<max_cnt<<endl; 
    
  /********initialize the array storing section_cell structure**************/
  vector<Section_Cell> cell_list; //////////////////////array 
  
//  cell_list.resize(max_cell_id); 
//  for(int i = 0;i < max_cell_id;i++)
//    cell_list[i].def = false;  //crashes if all BuildRoute is called 3 times 
  
  for(int i = 0;i < max_cell_id;i++){
    double min[2], max[2];
    min[0] = 0;
    min[1] = 0;
    max[0] = 1;
    max[1] = 1;
    BBox<2>* reg = new BBox<2>(true,min,max);
    Section_Cell* sc_ptr = new Section_Cell(0,0,0,false,*reg);
    cell_list.push_back(*sc_ptr);
    delete reg;
    delete sc_ptr;
  }

  ///////////////////////////////////////////////////////////////////

  float div_number1 = 0.5; 
  float div_number2 = 0.2; 
  
  
  for(int i = 1;i <= rel1->GetNoTuples();i++){
    Tuple* tuple_cell = rel1->GetTuple(i,false);
//    CcInt* sec_id = (CcInt*)tuple_cell->GetAttribute(attr1); 
    CcInt* cell_id = (CcInt*)tuple_cell->GetAttribute(attr2); 
    CcInt* cnt = (CcInt*)tuple_cell->GetAttribute(attr3); 
    Region* reg = (Region*)tuple_cell->GetAttribute(attr4);
    
    int cellid = cell_id->GetIntval();
    cell_list[cellid - 1].cell_id = cellid; 
    cell_list[cellid - 1].sec_intersect_count = cnt->GetIntval();
    cell_list[cellid - 1].reg = reg->BoundingBox(); 
  
    if(cnt->GetIntval() > (int)(max_cnt*div_number1)){  
        cell_list[cellid - 1].count_1 = 3;
    }    
    else if(cnt->GetIntval() > (int)(max_cnt*div_number2) ){
        cell_list[cellid - 1].count_1 = 2;
    }    
    else{
        cell_list[cellid - 1].count_1 = 1;
    }
    
    cell_list[cellid - 1].def = true; 
      
    tuple_cell->DeleteIfAllowed(); 
  }
  
  //create priority_queue 
  int count = 0;
  
  priority_queue<Section_Cell> cell_queue; //////////priority_queue
  
  for(unsigned int i = 0;i < cell_list.size();i++)
      if(cell_list[i].def){
//          cell_list[i].Print(); 
          count++;
          cell_queue.push(cell_list[i]);
      }    
//  cout<<"count "<<count<<endl; 


  vector<Section_Cell> cell_list3; //////////////////////array
  vector<Section_Cell> cell_list2; //////////////////////array 
  vector<Section_Cell> cell_list1; //////////////////////array 

  while(cell_queue.empty() == false){
    Section_Cell top = cell_queue.top();
    cell_queue.pop();
//    top.Print();
    
    if(top.count_1 == 3)
      cell_list3.push_back(top);
    if(top.count_1 == 2)
      cell_list2.push_back(top);
    if(top.count_1 == 1)
      cell_list1.push_back(top);
  }
  //3---13  2---49  1---204
//  cout<<cell_list3.size()<<" "
//        <<cell_list2.size()<<" "<<cell_list1.size()<<endl;

  int bus_no;
   bus_no = 3;
   
   BuildRoute(cell_list3, cell_list1, attr1, bus_no);
   bus_no = 2;
   
   BuildRoute(cell_list2, cell_list1, attr1, bus_no);
  
  bus_no = 1;
  unsigned int limit_no = 20; 
  
  BuildRoute_Limit(cell_list1, cell_list1, attr1, bus_no, limit_no);
  
}

/*
highest density area with low density area

*/
void BusRoute::BuildRoute(vector<Section_Cell>& from_cell_list,
                             vector<Section_Cell> to_cell_list,
                             int attr1, int bus_no)
{
  for(unsigned int i = 0;i < from_cell_list.size();i++){
    Section_Cell elem = from_cell_list[i];
    
    int temp_bus_no = bus_no;
    
    while(temp_bus_no > 0){
//      float dist_val = 10000.0;//Euclidean distance between two cells
      
//      float dist_val = 12000.0;//Euclidean distance between two cells
            
      float dist_val = 16000.0;//Euclidean distance between two cells
      
      int end_cellid = FindEndCell(from_cell_list[i],to_cell_list,dist_val); 
    
      if(end_cellid >= 0){
//        cout<<"start cell "<<from_cell_list[i].cell_id
//          <<" end cell "<<to_cell_list[end_cellid].cell_id<<endl; 
        
          start_cells.push_back(from_cell_list[i].reg);
          end_cells.push_back(to_cell_list[end_cellid].reg);
          start_cell_id.push_back(from_cell_list[i].cell_id);
          end_cell_id.push_back(to_cell_list[end_cellid].cell_id);
          
          if(bus_no == 3)bus_route_type.push_back(1);//the first route
          if(bus_no == 2)bus_route_type.push_back(2);//the second route
          
      }
    
      temp_bus_no--; 
    
    }

    ////use btree to get the sections that this cell interesects////////
/*    CcInt* search_cell_id = new CcInt(true, elem.cell_id);
    BTreeIterator* btree_iter = btree->ExactMatch(search_cell_id);
    vector<int> sec_id_list; 
    while(btree_iter->Next()){
        Tuple* tuple_cell = rel1->GetTuple(btree_iter->GetId(), false);
        CcInt* sec_id = (CcInt*)tuple_cell->GetAttribute(attr1);
        sec_id_list.push_back(sec_id->GetIntval());
        tuple_cell->DeleteIfAllowed();
    }
    delete btree_iter;
    delete search_cell_id;
    ////////////////////////////////////////////////////////////////////
    cout<<"cell_id "<<cell_list3[i].cell_id
        <<"sec no "<<sec_id_list.size()<<endl; 
        */
  }

}

/*
lown density area with low density area
with the limit number of bus routes 

*/
void BusRoute::BuildRoute_Limit(vector<Section_Cell>& from_cell_list,
                             vector<Section_Cell> to_cell_list,
                             int attr1, int bus_no, unsigned int limit_no)
{
  for(unsigned int i = 0;i < from_cell_list.size();i++){
    Section_Cell elem = from_cell_list[i];
    
    if(from_cell_list[i].def == false) continue; 
    
    int temp_bus_no = bus_no;
    
    while(temp_bus_no > 0){
//      float dist_val = 15000.0;//Euclidean distance between two cells
      float dist_val = 25000.0;//Euclidean distance between two cells
      int end_cellid = FindEndCell(from_cell_list[i],to_cell_list,dist_val); 
    
      if(end_cellid >= 0){
//        cout<<"start cell "<<from_cell_list[i].cell_id
//          <<" end cell "<<to_cell_list[end_cellid].cell_id<<endl; 
        
          start_cells.push_back(from_cell_list[i].reg);
          end_cells.push_back(to_cell_list[end_cellid].reg);
          start_cell_id.push_back(from_cell_list[i].cell_id);
          end_cell_id.push_back(to_cell_list[end_cellid].cell_id); 
     
          bus_route_type.push_back(3);//the lowest route 
          
          to_cell_list[i].def = false; 
          from_cell_list[end_cellid].def = false; 
          
      }
     
      temp_bus_no--; 
    
    }
    if(i > limit_no) break; 
    
  }

}

/*
use the first paramter as the start cell and find a cell from the
second parameter that the distance between them is larger than a value 

*/
int BusRoute::FindEndCell(Section_Cell& start_cell,
                          vector<Section_Cell>& cell_list, float dist_val)
{
  for(unsigned int i = 0;i < cell_list.size();i++){
    if(cell_list[i].def){
        if(start_cell.reg.Distance(cell_list[i].reg) > dist_val){
          cell_list[i].def = false;
          return i; 
        }
    }
  }
  cout<<"do not find an end cell for cell "<<start_cell.cell_id<<endl; 
  return -1; 
}


/*
create bus routes, use the relation of a pair of cells 
1.given a pair of cells where the first is the start location and the second is
the end location of a bus route
2.for each cell, it (randomly) selects a point as the start (end) point.
3.use the shortest path connecting the two points as the bus route

*/

void BusRoute::CreateRoute2(int attr,int attr1,int attr2,int attr3)
{
//  cout<<"attr "<<attr<<" attr1 "<<attr1 <<" attr2 "<<attr2<<endl; 
//  cout<<"CreateRoute2()"<<endl; 

  for(int i = 1;i <= rel2->GetNoTuples();i++){
    Tuple* tuple_cell_pair = rel2->GetTuple(i, false);
    int from_cell_id = 
        ((CcInt*)tuple_cell_pair->GetAttribute(attr1))->GetIntval();
    int end_cell_id =
        ((CcInt*)tuple_cell_pair->GetAttribute(attr2))->GetIntval();
        
    int route_type =
        ((CcInt*)tuple_cell_pair->GetAttribute(attr3))->GetIntval();
//    cout<<"from_cell "<<from_cell_id<<" end_cell "<<end_cell_id<<endl; 

//    cout<<"route_type "<<route_type<<endl; 
    
    ConnectCell(attr,from_cell_id,end_cell_id,route_type);
    tuple_cell_pair->DeleteIfAllowed(); 
  
  }
}

/*
randomly choose two positions in two cells and find the shortest path
connecting them. use the shortest path as the bus route 

*/
void BusRoute::ConnectCell(int attr,int from_cell_id,
                           int end_cell_id, int route_type)
{
     
    ////use btree to get the sections that this cell interesects////////
    CcInt* search_cell_id_from = new CcInt(true, from_cell_id);
    BTreeIterator* btree_iter1 = btree->ExactMatch(search_cell_id_from);
    vector<int> sec_id_list_from; 
    while(btree_iter1->Next()){
        Tuple* tuple_cell = rel1->GetTuple(btree_iter1->GetId(), false);
        CcInt* sec_id = (CcInt*)tuple_cell->GetAttribute(attr);
        sec_id_list_from.push_back(sec_id->GetIntval());
//        cout<<"sec id1 "<<sec_id->GetIntval()<<endl;
        tuple_cell->DeleteIfAllowed();
    }
    delete btree_iter1;
    delete search_cell_id_from;
/*    cout<<"from_cell_id "<<from_cell_id
        <<" sec no "<<sec_id_list_from.size()<<" "; */
        
    ////////////////////////////////////////////////////////////////////////
    CcInt* search_cell_id_end = new CcInt(true, end_cell_id);
    BTreeIterator* btree_iter2 = btree->ExactMatch(search_cell_id_end);
    vector<int> sec_id_list_end; 
    while(btree_iter2->Next()){
        Tuple* tuple_cell = rel1->GetTuple(btree_iter2->GetId(), false);
        CcInt* sec_id = (CcInt*)tuple_cell->GetAttribute(attr);
        sec_id_list_end.push_back(sec_id->GetIntval());
//        cout<<"sec id2 "<<sec_id->GetIntval()<<endl;
        tuple_cell->DeleteIfAllowed();
    }
    delete btree_iter2;
    delete search_cell_id_end;
/*    cout<<"end_cell_id "<<end_cell_id
        <<" sec no "<<sec_id_list_end.size()<<endl; */
    ///////// create two gpoints from selected two sections///////////////
    /////////choose the first road section for each cell/////////////////
    int index1 = 0;
    int index2 = 0; 
    ////////////////////create first gpoint////////////////////////
    int sec_id_1 = sec_id_list_from[index1];
    
//    cout<<"sec_id_1 "<<sec_id_1<<" "; 

    Tuple* tuple_sec_1 = n->GetSection(sec_id_1);  
    int rid1 = ((CcInt*)tuple_sec_1->GetAttribute(SECTION_RID))->GetIntval();
//    cout<<"section from "<<*tuple_sec_1<<endl; 
//    cout<<"rid1 "<<rid1<<endl;
    double loc1 = 
      ((CcReal*)tuple_sec_1->GetAttribute(SECTION_MEAS1))->GetRealval();

    GPoint* gp1 = new GPoint(true,n->GetId(),rid1,loc1,None);
    Point* location1 = new Point();
    gp1->ToPoint(location1);
    start_gp.push_back(*location1); 
    
    
    ///////////////////get the selected road section////////////////////
/*  SimpleLine* sl1 = (SimpleLine*)tuple_sec_1->GetAttribute(SECTION_CURVE);
    Line* busline1 = new Line(0);
    sl1->toLine(*busline1);
    bus_sections1.push_back(*busline1);
    delete busline1; */
    ///////////////////create second gpoint/////////////////////////////
    int sec_id_2 = sec_id_list_end[index2];
    
//    cout<<"sec_id_2 "<<sec_id_2<<endl; 
    
    Tuple* tuple_sec_2 = n->GetSection(sec_id_2);
    int rid2 = ((CcInt*)tuple_sec_2->GetAttribute(SECTION_RID))->GetIntval();
    double loc2 = 
        ((CcReal*)tuple_sec_2->GetAttribute(SECTION_MEAS1))->GetRealval();
    GPoint* gp2 = new GPoint(true,n->GetId(),rid2,loc2,None); 
    Point* location2 = new Point();
    gp2->ToPoint(location2); 
    end_gp.push_back(*location2); 
    
//    cout<<"section end "<<*tuple_sec_2<<endl; 

    ////////////////////////get the selected road section////////////////////
/*    SimpleLine* sl2 = (SimpleLine*)tuple_sec_2->GetAttribute(SECTION_CURVE);
    Line* busline2 = new Line(0);
    sl2->toLine(*busline2);
    bus_sections2.push_back(*busline2);
    delete busline2; */
    //////////////////////////////////////////////////////////////////////////
    //    cout<<"rid2 "<<rid2<<endl; 

    GLine* gl = new GLine(0);
    gp1->ShortestPath(gp2, gl);
    
    delete location1; 
    delete gp1;
    
    delete location2; 
    delete gp2; 
    
    tuple_sec_1->DeleteIfAllowed();
    tuple_sec_2->DeleteIfAllowed(); 
    //////////////////////////////////////////////////////////////////////////
    bus_lines1.push_back(*gl);
    
    Line* l = new Line(0);
    gl->Gline2line(l);
    bus_lines2.push_back(*l);
    delete l; 
    delete gl; 
    
    bus_route_type.push_back(route_type);
}

/*
for each bus route, it creates a sequence of points on it as bus stops 

*/
void BusRoute::CreateBusStop1(int attr1, int attr2, int attr3, int attr4)
{
    for(int i = 1;i <= rel1->GetNoTuples();i++){
      
//      if(i != 20) continue; 
      
      Tuple* tuple_bus_route = rel1->GetTuple(i, false);
      int br_id = ((CcInt*)tuple_bus_route->GetAttribute(attr1))->GetIntval();
      GLine* gl = (GLine*)tuple_bus_route->GetAttribute(attr2); 
      Line* l = (Line*)tuple_bus_route->GetAttribute(attr3);
      int route_type = 
          ((CcInt*)tuple_bus_route->GetAttribute(attr4))->GetIntval();
      
//      cout<<"br_id "<<br_id<<endl; 
//      cout<<"route_type "<<route_type<<endl; 
      
      CreateStops(br_id, gl,l, route_type); 
      tuple_bus_route->DeleteIfAllowed(); 
      
    }
}

/*
for such a gline, create a set of points on it 

*/

void BusRoute::CreateStops(int br_id, GLine* gl, Line* l, int route_type)
{
  vector<double> dist_for_stops1;//for type 1
  dist_for_stops1.push_back(1000.0);
  dist_for_stops1.push_back(900.0);
  dist_for_stops1.push_back(1100.0);
  dist_for_stops1.push_back(1050.0);
  dist_for_stops1.push_back(1500.0);
  dist_for_stops1.push_back(1300.0);
  dist_for_stops1.push_back(950.0);
  dist_for_stops1.push_back(1200.0);
  dist_for_stops1.push_back(850.0);
  dist_for_stops1.push_back(1400.0);
  dist_for_stops1.push_back(1300.0);
  
  ////////////////////////////////////////////////////////////////////
  vector<double> dist_for_stops2;//for type 2
  dist_for_stops2.push_back(900.0);
  dist_for_stops2.push_back(1100.0);
  dist_for_stops2.push_back(1150.0); 
  dist_for_stops2.push_back(1250.0); 
  dist_for_stops2.push_back(850.0); 
  dist_for_stops2.push_back(1050.0); 
  dist_for_stops2.push_back(1000.0);
  dist_for_stops2.push_back(1200.0);
  ///////////////////////////////////////////////////////////////////
  vector<double> dist_for_stops3;//for type 3
  dist_for_stops3.push_back(900.0);
  dist_for_stops3.push_back(1100.0); 
  dist_for_stops3.push_back(950.0); 
  dist_for_stops3.push_back(1300.0); 
  dist_for_stops3.push_back(1000.0);  
  dist_for_stops3.push_back(1200.0); 
  dist_for_stops3.push_back(1150.0); 
  //////////////////////////////////////////////////////////////////////
  
 
////we have to know it goes from start to end or the other direction/////
  vector<bool> start_from; //or end_from 
  
  vector<SectTreeEntry> sec_list; 
  for(int i = 0; i < gl->Size();i++){
    RouteInterval* ri = new RouteInterval();
    gl->Get(i, *ri); 
    
/*    cout<<"rid "<<ri->GetRouteId()
        <<"start "<<ri->GetStartPos()
        <<"end "<<ri->GetEndPos()<<endl;*/
    
    if(!AlmostEqual(ri->GetStartPos(), ri->GetEndPos())){
      
      DbArray<SectTreeEntry>* actSections = new DbArray<SectTreeEntry>(0);
      n->GetSectionsOfRouteInterval(ri,actSections);

      SectTreeEntry nEntry;
      
      vector<SectTreeEntry> sec_list_temp; 
      
      for(int j = 0;j < actSections->Size();j++){
          actSections->Get(j, nEntry);

          ////////////////////////////////////////////////////
/*          cout<<"entry "<<nEntry.rid
              <<" start "<<nEntry.start
              <<" end "<<nEntry.end<<endl; */

          if(AlmostEqual(nEntry.start,nEntry.end)) continue; 

          sec_list_temp.push_back(nEntry); 
          
          if(ri->GetStartPos() < ri->GetEndPos()) start_from.push_back(true);
          else
            start_from.push_back(false); 
          
      }  
      
      if(ri->GetStartPos() < ri->GetEndPos()){
        for(unsigned int k = 0;k < sec_list_temp.size();k++)
          sec_list.push_back(sec_list_temp[k]);
      }else{
        for(int k = sec_list_temp.size() - 1;k >= 0;k--)
          sec_list.push_back(sec_list_temp[k]);
      }
      
      
      delete actSections;
    }  
      delete ri; 
  }
  
/*  for(unsigned int i = 0;i < sec_list.size();i++){
    SectTreeEntry nEntry = sec_list[i]; 
    cout<<"i "<<i<<" rid "<<nEntry.rid<<" secid "<<nEntry.secttid
        <<"start "<<nEntry.start<<" end "<<nEntry.end<<endl; 
  }*/
  
  if(sec_list.size() < 2) return; 
  
  const double dist_to_jun = 15.0; //minimum distance between bus stop and jun 
  /////////////////create the first bus stop///////////////////////////
  int stop_count = 0; 
  unsigned int last_sec_index = 0; 
  double last_sec_start;
  double last_sec_end; 
  double last_sec_gp_pos; 
  
    
  assert(start_from.size() == sec_list.size()); 
  
/*  for(int i = 0;i < sec_list.size();i++){
    cout<<"rid "<<sec_list[i].rid
        <<"secid "<<sec_list[i].secttid
        <<"start pos "<<sec_list[i].start
        <<"end pos "<<sec_list[i].end<<endl; 
    cout<<start_from[i]<<endl; 
  }*/
  
 
  //////////////////////////////////////////////////////////////////////////
  while(true){
    SectTreeEntry entry = sec_list[last_sec_index]; 
    last_sec_start = entry.start;
    last_sec_end = entry.end; 
    
    if(fabs(last_sec_start - last_sec_end) < 2*dist_to_jun){
      last_sec_index++;
      continue; 
    }
    

    if(start_from[last_sec_index]){ //start from small
      last_sec_gp_pos = last_sec_start + dist_to_jun;  
    }else{ //start from big 
      last_sec_gp_pos = last_sec_end - dist_to_jun; 
    }
      
      
      GPoint* gp = 
        new GPoint(true,n->GetId(),entry.rid,last_sec_gp_pos,None);
      bus_stop_loc_1.push_back(*gp);
      Point* p = new Point();
      gp->ToPoint(p);
      bus_stop_loc_2.push_back(*p);
      
//      cout<<"gp start "<<*gp<<endl; 
      
      delete p;
      delete gp;
      
      br_id_list.push_back(br_id);
      br_stop_id.push_back(stop_count + 1);

      stop_count++;
      break; 
  }
  //////////////create more bus stops////////////////////////////////////////
  while(true){
    double next_stop_dist;
    if(route_type == 1)
      next_stop_dist = dist_for_stops1[(stop_count-1)% dist_for_stops1.size()];
    else if(route_type == 2)
      next_stop_dist = dist_for_stops2[(stop_count-1)% dist_for_stops2.size()];
    else if(route_type == 3)
      next_stop_dist = dist_for_stops3[(stop_count-1)% dist_for_stops3.size()];
    else assert(false);
      
/*    cout<<"stop_id "<<stop_count
        <<" current dist stop "<<next_stop_dist<<endl; */
    //////////////////////////////////////////////////////////////////////
    if(FindNextStop(sec_list,last_sec_index,last_sec_start,
                 last_sec_end, last_sec_gp_pos,next_stop_dist,
                 dist_to_jun,start_from)){
    
      br_id_list.push_back(br_id);
      br_stop_id.push_back(stop_count + 1);
      stop_count++;
    }else
      break; 
    //////////////////////////////////////////////////////////////////////
  }
  
}

/*
create more bus stops along the given gline
use nextstopdist to control the distance between two consecutive bus stops  

*/
bool BusRoute::FindNextStop(vector<SectTreeEntry> sec_list,
                    unsigned int& last_sec_index,double& last_sec_start,
                    double& last_sec_end, double& last_sec_gp_pos,
                    double next_stop_dist, 
                    double dist_to_jun, vector<bool> start_from)
{
  
  if(start_from[last_sec_index]){ //from small to big 
    
//    cout<<"from small to big "<<endl; 
    
    int rid; 
    //still in the same section, very long road section 
    if(fabs(last_sec_gp_pos - last_sec_end) > (next_stop_dist + dist_to_jun)){
      last_sec_gp_pos += next_stop_dist;
      rid = sec_list[last_sec_index].rid; 
  
    }else{//move to the next road section 
      next_stop_dist -= fabs(last_sec_gp_pos - last_sec_end); 
      for(last_sec_index++;last_sec_index < sec_list.size();last_sec_index++){
      
      double cur_start = sec_list[last_sec_index].start;
      double cur_end = sec_list[last_sec_index].end; 
      //////////////////////////////////////////////////////
/*      cout<<"rid "<<sec_list[last_sec_index].rid
        <<" secid "<<sec_list[last_sec_index].secttid
        <<" start pos "<<sec_list[last_sec_index].start
        <<" end pos "<<sec_list[last_sec_index].end<<endl; */
        
      /////////////////////////////////////////////////////////
      if(fabs(cur_end - cur_start) < 2*dist_to_jun){
        next_stop_dist -= fabs(cur_end - cur_start);
        continue; 
      }
        
      if(next_stop_dist - fabs(cur_end - cur_start) < 0){
        
         if(start_from[last_sec_index]){ //from small to big 
          double temp_pos = cur_start + next_stop_dist;
          if(fabs(temp_pos - cur_start) < dist_to_jun)
              temp_pos = cur_start + dist_to_jun;
          if(fabs(cur_end - temp_pos) < dist_to_jun)
              temp_pos = cur_end - dist_to_jun;
        
          last_sec_gp_pos = temp_pos; 
        }else{ //from big to small 
          double temp_pos = cur_end - next_stop_dist;
          if(fabs(temp_pos - cur_end) < dist_to_jun)
              temp_pos = cur_end - dist_to_jun;
          if(fabs(temp_pos - cur_start) < dist_to_jun)
              temp_pos = cur_start + dist_to_jun;
        
          last_sec_gp_pos = temp_pos;   
        }
        
        last_sec_start = cur_start;
        last_sec_end = cur_end; 
        rid = sec_list[last_sec_index].rid;      
        break; 
      }else
        next_stop_dist -= fabs(cur_end - cur_start); 
    
    }
    if(last_sec_index == sec_list.size()) return false; 
  }
    
//   cout<<"last_sec_index "<<last_sec_index<<" rid " <<rid<<endl; 
    
   GPoint* gp = 
   new GPoint(true,n->GetId(),rid,last_sec_gp_pos,None);
   bus_stop_loc_1.push_back(*gp);
   Point* p = new Point();
   gp->ToPoint(p);
   bus_stop_loc_2.push_back(*p);   
   
//   cout<<"stop "<<*gp<<endl; 
   
   delete p;
   delete gp;
         
   return true; 
   
  }else{///from big to small 
  
//    cout<<"from big to small"<<endl;
    
    int rid; 
    //still in the same section, very long road section 
    if(fabs(last_sec_gp_pos - last_sec_start) > (next_stop_dist + dist_to_jun)){
      last_sec_gp_pos -= next_stop_dist;
      rid = sec_list[last_sec_index].rid; 
      
    }else{
    
     next_stop_dist -= fabs(last_sec_gp_pos - last_sec_start); 
      for(last_sec_index++;last_sec_index < sec_list.size();last_sec_index++){
      
      double cur_start = sec_list[last_sec_index].start;
      double cur_end = sec_list[last_sec_index].end; 
      ////////////////////////////////////////////////////////////////
/*      cout<<"cur_start "<<cur_start<<" cur_end "<<cur_end<<endl; 
      
      cout<<"rid "<<sec_list[last_sec_index].rid
        <<" secid "<<sec_list[last_sec_index].secttid
        <<" start pos "<<sec_list[last_sec_index].start
        <<" end pos "<<sec_list[last_sec_index].end<<endl; */

      ////////////////////////////////////////////////////////////////
      if(fabs(cur_end - cur_start) < 2*dist_to_jun){
        next_stop_dist -= fabs(cur_end - cur_start);
        continue; 
      }
        
      if(next_stop_dist - fabs(cur_end - cur_start) < 0){
        
         if(start_from[last_sec_index]){ //from small to big 
          double temp_pos = cur_start + next_stop_dist;
          if(fabs(temp_pos - cur_start) < dist_to_jun)
              temp_pos = cur_start + dist_to_jun;
          if(fabs(cur_end - temp_pos) < dist_to_jun)
              temp_pos = cur_end - dist_to_jun;
        
          last_sec_gp_pos = temp_pos; 
        }else{ //from big to small 
//          cout<<"next_stop_dist "<<next_stop_dist<<endl; 

          double temp_pos = cur_end - next_stop_dist;
          if(fabs(temp_pos - cur_end) < dist_to_jun)
              temp_pos = cur_end - dist_to_jun;
          if(fabs(temp_pos - cur_start) < dist_to_jun)
              temp_pos = cur_start + dist_to_jun;

          last_sec_gp_pos = temp_pos;   
        }
        
        last_sec_start = cur_start;
        last_sec_end = cur_end; 
        rid = sec_list[last_sec_index].rid;      
        break; 
      }else
        next_stop_dist -= fabs(cur_end - cur_start); 
    
    }
      if(last_sec_index == sec_list.size()) return false; 
    
    }
    
    GPoint* gp = 
    new GPoint(true,n->GetId(),rid,last_sec_gp_pos,None);
    bus_stop_loc_1.push_back(*gp);
    Point* p = new Point();
    gp->ToPoint(p);
    bus_stop_loc_2.push_back(*p);   
  
//    cout<<" stop "<<*gp<<endl; 
    
    delete p;
    delete gp;

    return true; 
  }
   
}

/*
merge bus stops 
for several bus stops on the same road section, it might be merge them into one
 the minimum length of road section is 0.055
 the maximum length of road section is 5046.577  

we merge bus stops that locate on the same road section 

*/


void BusRoute::CreateBusStop2(int attr1,int attr2,int attr3)
{
//  cout<<"attr1 "<<attr1<<" attr2 "<<attr2<<" attr3 "<<attr3<<endl; 
//  cout<<"CreateBusStop2()"<<endl; 
  
  vector<BusStop> bus_stop_list; 
  
  for(int i = 1;i <= rel1->GetNoTuples();i++){
    Tuple* tuple_bus_stop = rel1->GetTuple(i, false);
        
    int br_id = ((CcInt*)tuple_bus_stop->GetAttribute(attr1))->GetIntval();
    
/*    if(br_id != 108){ 
      tuple_bus_stop->DeleteIfAllowed();
      continue; 
    }*/
    
    int stop_id = ((CcInt*)tuple_bus_stop->GetAttribute(attr2))->GetIntval();
    GPoint* gp = (GPoint*)tuple_bus_stop->GetAttribute(attr3); 

    int sid = n->GetSectionOnRoute(gp)->GetTupleId(); 
    
//    cout<<*tuple_bus_stop<<endl; 
//    cout<<*n->GetSectionOnRoute(gp)<<endl;
    
    BusStop* bs = new BusStop(br_id,stop_id,gp->GetRouteId(),
                              gp->GetPosition(), sid, true);
    bus_stop_list.push_back(*bs);
    delete bs; 

    tuple_bus_stop->DeleteIfAllowed();
  }
  sort(bus_stop_list.begin(),bus_stop_list.end());
  
/*  for(int i = 0;i < bus_stop_list.size();i++){
      bus_stop_list[i].Print(); 
  }*/
  ////////////////take all stops in one route////////////////////////
  int count = 0;
  for(unsigned int i = 0;i < bus_stop_list.size();i++){
    int sid = bus_stop_list[i].sid; 
    
    vector<BusStop> temp_list; 
    temp_list.push_back(bus_stop_list[i]); 
    
    while((i + 1)< bus_stop_list.size() && 
          bus_stop_list[i + 1].sid == sid){
        i++;
        temp_list.push_back(bus_stop_list[i]); 
    }    
    
    MergeBusStop1(temp_list); 
    
    count += temp_list.size(); 
  }

//  cout<<bus_stop_list.size()<<" count "<<count<<endl; 

}

/*
take all bus stops in the same section and merge some of them if they are close
to each other. there is one condition:
after merging, all bus stops should still locate on their original bus routes.
they can't move to another different route

*/
void BusRoute::MergeBusStop1(vector<BusStop>& stop_list)
{
//  cout<<"stop_list size "<<stop_list.size()<<endl; 
//  cout<<stop_list.size()<<endl; 
  if(stop_list.size() == 1){
    br_id_list.push_back(stop_list[0].br_id);
    br_stop_id.push_back(stop_list[0].br_stop_id); 
    GPoint* gp = new GPoint(true,n->GetId(),stop_list[0].rid,stop_list[0].pos);
    bus_stop_loc_1.push_back(*gp); 
  
    Point* p = new Point();
    gp->ToPoint(p);
    bus_stop_loc_2.push_back(*p);      
    delete p; 
    delete gp;   
    
//    point_id_list.push_back(point_id);
    sec_id_list.push_back(stop_list[0].sid);

  }else{//there are several bus stops on the same road section 
    
    //to avoid that two bus stops coming from the same bus route are mereged
    //into one position 

    const double dist_val = 800.0; //the minimum distance between stops
    const double dist_to_jun = 15.0;//minimum distance between bus stop and jun 
    
    int sec_id = stop_list[0].sid; 
    Tuple* tuple_sec = n->GetSection(sec_id);
    SimpleLine* curve = (SimpleLine*)tuple_sec->GetAttribute(SECTION_CURVE);
    
    if(curve->Length() < dist_val){//merge all stops in one position
      double final_pos = 0.0; 
      for(unsigned int i = 0;i < stop_list.size();i++){
//        cout<<"stop_list pos "<<stop_list[i].pos<<" "; 
        final_pos += stop_list[i].pos;
      }
      
      final_pos = final_pos / stop_list.size(); 
//      cout<<endl<<" final pos "<<final_pos<<endl; 
      
      int count1 = 0; 
      for(unsigned int i = 0;i < stop_list.size();i++){
      
        br_id_list.push_back(stop_list[i].br_id);
        br_stop_id.push_back(stop_list[i].br_stop_id); 
        GPoint* gp = new GPoint(true,n->GetId(),stop_list[i].rid,final_pos);
        bus_stop_loc_1.push_back(*gp); 
  
        Point* p = new Point();
        gp->ToPoint(p);
        bus_stop_loc_2.push_back(*p);      
        delete p; 
        delete gp;   
        count1++;
        
        sec_id_list.push_back(stop_list[i].sid);
      }
      
//      cout<<"count1 "<<count1<<endl; 
      
    }else{//partition a road section into severl parts and there is a bus stop
          //for each part 
      
      double sec_pos1 = 
          ((CcReal*)tuple_sec->GetAttribute(SECTION_MEAS1))->GetRealval();
      double sec_pos2 =
          ((CcReal*)tuple_sec->GetAttribute(SECTION_MEAS2))->GetRealval();
      if(sec_pos1 > sec_pos2){
        double temp = sec_pos1;
        sec_pos1 = sec_pos2;
        sec_pos2 = temp; 
      }
      
      
      int interval = curve->Length() / dist_val;
      
/*      cout<<"interval "<<interval
          <<"curve length "<<curve->Length()<<endl; */
          
      for(unsigned int i = 0;i < stop_list.size();i++){
        double stop_pos = stop_list[i].pos;
//        int interval_pos = stop_pos / dist_val;
        int interval_pos = (stop_pos - sec_pos1)/ dist_val;
        
/*        stop_list[i].Print();
        cout<<"stop_pos "<<stop_pos
            <<" interval_pos "<<interval_pos<<endl; */
        
        assert(interval_pos >= 0 && interval_pos <= interval);
        double new_pos = dist_val / 4 + interval_pos*dist_val; 
        if(new_pos > curve->Length()){
//          new_pos = interval_pos*dist_val; 
            new_pos = curve->Length() - dist_to_jun;
        }
        /////////////////////////////////
        new_pos += sec_pos1; 
        /////////////////////////////////
        
        br_id_list.push_back(stop_list[i].br_id);
        br_stop_id.push_back(stop_list[i].br_stop_id); 
        GPoint* gp = new GPoint(true,n->GetId(),stop_list[i].rid,new_pos);
        bus_stop_loc_1.push_back(*gp); 
  
        Point* p = new Point();
        gp->ToPoint(p);
        bus_stop_loc_2.push_back(*p);      
        delete p; 
        delete gp; 

        sec_id_list.push_back(stop_list[i].sid);
      }
      
    }
  
    tuple_sec->DeleteIfAllowed();
  }


}

/*
Merge bus stops locating on different road sections (adjacent)
use a distance threshold value to expand the bus stop (gpoint) to a gline value 

after changing the position of some bus stops, they should still locate on 
their original bus route

Note: each bus route covers a complete road section or none of it.
it can't only cover part of a road section 

*/
void BusRoute::CreateBusStop3(int attr,int attr1,int attr2,int attr3)
{
//  cout<<"attr "<<attr<<endl;
//  cout<<"attr1 "<<attr1<<" attr2 "<<attr2<<" attr3 "<<attr3<<endl; 
  
  /////////////////  collect all the bus stops ////////////////////////
  vector<BusStop> bus_stop_list;
  for(int i = 1;i <= rel2->GetNoTuples();i++){
    Tuple* tuple_bus_stop = rel2->GetTuple(i, false);
    int br_id = ((CcInt*)tuple_bus_stop->GetAttribute(attr1))->GetIntval();
    int stop_id = ((CcInt*)tuple_bus_stop->GetAttribute(attr2))->GetIntval();
    GPoint* gp = (GPoint*)tuple_bus_stop->GetAttribute(attr3);
           

    int sid = n->GetSectionOnRoute(gp)->GetTupleId(); 
    
    BusStop* bs = 
      new BusStop(br_id,stop_id,gp->GetRouteId(),gp->GetPosition(),sid,true);
    
    bus_stop_list.push_back(*bs);
    delete bs;
    tuple_bus_stop->DeleteIfAllowed();
  }

//  cout<<"bus_stop_list size "<<bus_stop_list.size()<<endl; 

  //////////////////////////////////////////////////////////////////////////
  const double dist_val = 500.0; //distance used to expand road section 
  int last_br_id = 0; 
  vector<SectTreeEntry> sec_list; 
  vector<bool> start_from; 
  for(unsigned int i = 0;i < bus_stop_list.size();i++){
      
//      if(bus_stop_list[i].br_id > 10) continue;

//      if(i != 500) continue; 
        
      if(bus_stop_list[i].def == false) continue; 
      
    ///////////////////////////////////////////////////////////////////////////
    //collect road sections that are with the distance distval to the bus stop/
    ///////////////////////////////////////////////////////////////////////////
      //if it is false, it means the section starts from the big value
      //instead of the small value 
      
      if(bus_stop_list[i].br_id != last_br_id){
        sec_list.clear();
        start_from.clear();
        Tuple* tuple_bus_route = rel1->GetTuple(bus_stop_list[i].br_id,false);
        GLine* gl = (GLine*)tuple_bus_route->GetAttribute(attr);
        GetSectionList(gl, sec_list, start_from);
        tuple_bus_route->DeleteIfAllowed();
        last_br_id = bus_stop_list[i].br_id; 
        
        ///////////////////////////////////////////////////////
        assert(sec_list.size() == start_from.size());
/*        for(unsigned int j = 0;j < sec_list.size();j++){
          cout<<" rid "<<sec_list[j].rid<<" secid "<<sec_list[j].secttid
          <<" start "<<sec_list[j].start<<" end "<<sec_list[j].end
          <<" start from small "<<start_from[j]<<endl;       
        }*/
        /////////////////////////////////////////////////////////
      }
      /////////find the position of this bus stop in section/////////////
      int sec_index = -1;
      for(unsigned int j = 0;j < sec_list.size();j++){
        if(sec_list[j].secttid == bus_stop_list[i].sid){
          sec_index = j;
          break; 
        } 
      }
      assert(sec_index != -1); 
      
//      bus_stop_list[i].Print();
      
      /////////////// find the up adjacent sections //////////////////////////
      vector<SectTreeEntry> sec_down;
      vector<SectTreeEntry> sec_up;
      //we only consider adjacent section.
      //the same section has been processed before 
      if(start_from[sec_index])
      {
        if(fabs(bus_stop_list[i].pos - sec_list[sec_index].start) < dist_val){
          FindDownSection(bus_stop_list[i].pos,sec_list,sec_index,
                        dist_val,sec_down,start_from);
        }
        /////////////// find the down adjacent sections ///////////////////////
        
        if(fabs(bus_stop_list[i].pos - sec_list[sec_index].end) < dist_val){
          FindUpSection(bus_stop_list[i].pos,sec_list,sec_index,
                        dist_val,sec_up,start_from);
        }
        
      }else{
      
        if(fabs(bus_stop_list[i].pos - sec_list[sec_index].end) < dist_val){
          FindDownSection(bus_stop_list[i].pos,sec_list,sec_index,
                        dist_val,sec_down,start_from);
        }
        /////////////// find the down adjacent sections ///////////////////////
        if(fabs(bus_stop_list[i].pos - sec_list[sec_index].start) < dist_val){
          FindUpSection(bus_stop_list[i].pos,sec_list,sec_index,
                        dist_val,sec_up,start_from);
        }      
      }
      //////////////////////////////////////////////////////////////////////
/*      cout<<"bus route down direction"<<endl;
      for(unsigned int j = 0;j < sec_down.size();j++){
          cout<<" rid "<<sec_down[j].rid<<" secid "<<sec_down[j].secttid
          <<" start "<<sec_down[j].start<<" end "<<sec_down[j].end<<endl;       
      }
      
      cout<<"bus route up direction"<<endl;
      for(unsigned int j = 0;j < sec_up.size();j++){
          cout<<" rid "<<sec_up[j].rid<<" secid "<<sec_up[j].secttid
          <<" start "<<sec_up[j].start<<" end "<<sec_up[j].end<<endl;       
      }*/
      /////////////////////////////////////////////////////////////////////
      ////////collect all bus stops on these sections/////////////////////
      ////////////////////////////////////////////////////////////////////
      MergeBusStop2(sec_down,sec_up,bus_stop_list,i,attr);
  }

   ///////////////////////get the result///////////////////////////////
   for(unsigned int i = 0;i < bus_stop_list.size();i++){
      br_id_list.push_back(bus_stop_list[i].br_id);
      br_stop_id.push_back(bus_stop_list[i].br_stop_id);
      sec_id_list.push_back(bus_stop_list[i].sid); 
      int rid = bus_stop_list[i].rid;
      double pos = bus_stop_list[i].pos;
      GPoint* gp = new GPoint(true,n->GetId(),rid,pos,None);
      bus_stop_loc_1.push_back(*gp);
      
      Point* p = new Point();
      gp->ToPoint(p);
      bus_stop_loc_2.push_back(*p);      
      delete p;
      delete gp;
   }

}

/*
Merge bus stops that locate on adjacent road sections

*/
void BusRoute::MergeBusStop2(vector<SectTreeEntry> sec_down,
                             vector<SectTreeEntry> sec_up,
                             vector<BusStop>& bus_stop_list, 
                             int cur_index, int attr)
{
  vector<SectTreeEntry> sec_all;
  for(unsigned int i = 0;i < sec_down.size();i++)
    sec_all.push_back(sec_down[i]);
  
  for(unsigned int i = 0;i < sec_up.size();i++){
    if(sec_down.size() > 0 &&
      sec_up[i].secttid == sec_down[0].secttid)continue; //the same section 
      
    sec_all.push_back(sec_up[i]);
  }  

  if(sec_all.size() > 0){
      vector<int> bus_stop_id_list; 
      for(unsigned int i = 0;i < sec_all.size();i++){
          int sec_id = sec_all[i].secttid; 
          double start = sec_all[i].start;
          double end = sec_all[i].end; 
          /////////collect all bus stops on these sections/////////////
        
        CcInt* search_cell_id = new CcInt(true, sec_id);
        BTreeIterator* btree_iter = btree->ExactMatch(search_cell_id);
        while(btree_iter->Next()){
          
          int tuple_id = btree_iter->GetId();
          if(tuple_id - 1 != cur_index){//do not include itself 
            if(start <= bus_stop_list[tuple_id - 1].pos &&
              bus_stop_list[tuple_id - 1].pos <= end){//bus stop locate on it 
            

              bus_stop_id_list.push_back(tuple_id - 1);
            
/*              if(bus_stop_list[tuple_id - 1].rid != 
                 bus_stop_list[cur_index].rid && 
                 !AlmostEqual(bus_stop_list[tuple_id - 1].pos,
                              bus_stop_list[cur_index].pos)){
                cout<<"tuple_id "<<tuple_id<<endl; 
                bus_stop_list[cur_index].Print();
                bus_stop_list[tuple_id - 1].Print();

              }*/
              ///////////////////////////////////////////////////////
/*              Tuple* tuple_bus_stop = 
                    rel2->GetTuple(btree_iter->GetId(), false);
//              cout<<*tuple_bus_stop<<endl; 
              tuple_bus_stop->DeleteIfAllowed();*/
              ///////////////////////////////////////////////////////    
            }    
          }    
        }
        delete btree_iter;
        delete search_cell_id;
          
      }
      //check whether the route of the bus stop changed position /////////////
      /////////////////has the section id //////////
      for(unsigned int i = 0;i < bus_stop_id_list.size();i++){
          int index = bus_stop_id_list[i];
          BusStop bs = bus_stop_list[index]; 
          int br_id = bs.br_id;
          Tuple* tuple_bus_route = rel1->GetTuple(br_id,false);
          GLine* gl = (GLine*)tuple_bus_route->GetAttribute(attr);
          
          vector<SectTreeEntry> sec_list; 
          vector<bool> start_from;
          GetSectionList(gl, sec_list, start_from);
          for(unsigned int j = 0;j < sec_list.size();j++){
            if(sec_list[j].secttid == bus_stop_list[cur_index].sid){

              bus_stop_list[index].rid = bus_stop_list[cur_index].rid;
              bus_stop_list[index].pos = bus_stop_list[cur_index].pos;
              bus_stop_list[index].sid = bus_stop_list[cur_index].sid;
              
          
              bus_stop_list[cur_index].def = false;
              bus_stop_list[index].def = false; 
              break; 
            }    
          }
          tuple_bus_route->DeleteIfAllowed();
      }
  }
  
}

/*
find the section to down direction. bus route goes down 

*/

void BusRoute::FindDownSection(double cur_bus_stop_pos,
                     vector<SectTreeEntry> sec_list,int sec_index,
                     const double dist_val,
                     vector<SectTreeEntry>& sec_down,vector<bool> start_from)
                       
{
  double temp_dist; 
  if(start_from[sec_index]){
    int secttid = sec_list[sec_index].secttid;
    int rid = sec_list[sec_index].rid;
    double start = sec_list[sec_index].start;
    double end = cur_bus_stop_pos; 
    SectTreeEntry* tmp_entry = 
        new SectTreeEntry(secttid,rid,start,end,true,true);
    sec_down.push_back(*tmp_entry);
    delete tmp_entry;
    temp_dist = dist_val - fabs(end - start);
  }else{
    int secttid = sec_list[sec_index].secttid;
    int rid = sec_list[sec_index].rid;
    double start = cur_bus_stop_pos; 
    double end = sec_list[sec_index].end;
    SectTreeEntry* tmp_entry = 
        new SectTreeEntry(secttid,rid,start,end,true,true);
    sec_down.push_back(*tmp_entry);
    delete tmp_entry; 
    temp_dist = dist_val - fabs(end - start);
  }    
        
  int temp_index = sec_index;
  while(temp_dist > 0.0){
      temp_index--;
      if(temp_index >= 0){
            SectTreeEntry entry = sec_list[temp_index];
            
            if(temp_dist - fabs(entry.start - entry.end) > 0){
              sec_down.push_back(entry);
              temp_dist -= fabs(entry.start - entry.end);
            }else{
              if(start_from[temp_index] == false){
                
                int secttid = entry.secttid;
                int rid = entry.rid;
                double start = entry.start;
                double end = start + temp_dist; 
                SectTreeEntry* temp_entry = 
                  new SectTreeEntry(secttid,rid,start,end,true,true);
                sec_down.push_back(*temp_entry);
                delete temp_entry; 
                temp_dist -= fabs(entry.start - entry.end);
                
              }else{ 
                int secttid = entry.secttid;
                int rid = entry.rid;
                double start = entry.end - temp_dist;
                double end = entry.end; 
                SectTreeEntry* temp_entry = 
                  new SectTreeEntry(secttid,rid,start,end,true,true);
                sec_down.push_back(*temp_entry);
                delete temp_entry; 
                temp_dist -= fabs(entry.start - entry.end);
              }
            }
      }else break;
  }
  
}                       
  
/*
find the section to up direction. bus route goes up

*/

void BusRoute::FindUpSection(double cur_bus_stop_pos,
                     vector<SectTreeEntry> sec_list,int sec_index,
                     const double dist_val,
                     vector<SectTreeEntry>& sec_up,vector<bool> start_from)
                       
{
  double temp_dist;
  if(start_from[sec_index]){
    int secttid = sec_list[sec_index].secttid;
    int rid = sec_list[sec_index].rid;
    double start = cur_bus_stop_pos;
    double end = sec_list[sec_index].end;
    SectTreeEntry* tmp_entry = 
        new SectTreeEntry(secttid,rid,start,end,true,true);
    sec_up.push_back(*tmp_entry);
    delete tmp_entry; 
    
    temp_dist = dist_val - fabs(end - start);
    
  }else{
    int secttid = sec_list[sec_index].secttid;
    int rid = sec_list[sec_index].rid;
    double start = sec_list[sec_index].start;
    double end = cur_bus_stop_pos;
    SectTreeEntry* tmp_entry = 
        new SectTreeEntry(secttid,rid,start,end,true,true);
    sec_up.push_back(*tmp_entry);
    delete tmp_entry; 
    
    temp_dist = dist_val - fabs(end - start);
  
  }  
        
  unsigned int temp_index = sec_index;
  while(temp_dist > 0.0){
      temp_index++;
      if(temp_index < sec_list.size()){
        
            SectTreeEntry entry = sec_list[temp_index];
            
            if(temp_dist - fabs(entry.start - entry.end) > 0){
              sec_up.push_back(entry);
              temp_dist -= fabs(entry.start - entry.end);
            }else{
              if(start_from[temp_index]){
                
                int secttid = entry.secttid;
                int rid = entry.rid;
                double start = entry.start;
                double end = start + temp_dist; 
                SectTreeEntry* temp_entry = 
                  new SectTreeEntry(secttid,rid,start,end,true,true);
                sec_up.push_back(*temp_entry);
                delete temp_entry; 
                temp_dist -= fabs(entry.start - entry.end);
                
              }else{ 
                int secttid = entry.secttid;
                int rid = entry.rid;
                double start = entry.end - temp_dist;
                double end = entry.end; 
                SectTreeEntry* temp_entry = 
                  new SectTreeEntry(secttid,rid,start,end,true,true);
                sec_up.push_back(*temp_entry);
                delete temp_entry; 
                temp_dist -= fabs(entry.start - entry.end);
              }
            }
      }else break;
  }
  
}                       

/*
convert a gline object to a list of section representations 

*/

void BusRoute::GetSectionList(GLine* gl,vector<SectTreeEntry>& sec_list, 
                              vector<bool>& start_from)
{
   for(int i = 0; i < gl->Size();i++){
    RouteInterval* ri = new RouteInterval();
    gl->Get(i, *ri); 
    
/*    cout<<"rid "<<ri->GetRouteId()
        <<"start "<<ri->GetStartPos()
        <<"end "<<ri->GetEndPos()<<endl;*/
    
    if(!AlmostEqual(ri->GetStartPos(), ri->GetEndPos())){
      
      DbArray<SectTreeEntry>* actSections = new DbArray<SectTreeEntry>(0);
      n->GetSectionsOfRouteInterval(ri,actSections);

      SectTreeEntry nEntry;
      
      vector<SectTreeEntry> sec_list_temp; 
      
      for(int j = 0;j < actSections->Size();j++){
          actSections->Get(j, nEntry);

          ////////////////////////////////////////////////////
/*          cout<<"entry "<<nEntry.rid
              <<" start "<<nEntry.start
              <<" end "<<nEntry.end<<endl; */
              
          if(AlmostEqual(nEntry.start,nEntry.end))continue; 

          sec_list_temp.push_back(nEntry); 
          
          if(ri->GetStartPos() < ri->GetEndPos()) start_from.push_back(true);
          else
            start_from.push_back(false); 
          
      }  
      
      if(ri->GetStartPos() < ri->GetEndPos()){
        for(unsigned int k = 0;k < sec_list_temp.size();k++)
          sec_list.push_back(sec_list_temp[k]);
      }else{
        for(int k = sec_list_temp.size() - 1;k >= 0;k--)
          sec_list.push_back(sec_list_temp[k]);
      }
      
      
      delete actSections;
    }  
      delete ri; 
  }

}
