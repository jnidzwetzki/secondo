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

April, 2010 Jianqiu Xu

[TOC]

1 Overview

This source file essentially contains the necessary implementations for
creating the graph model for walk planning.


*/
#include "Partition.h"
/*
Decompose the pavement on one side of the road into a set of subregions

*/

void SpacePartition::DecomposePave(Region* reg1, Region* reg2,
                     vector<Region>& result)
{
    vector<Region> result1;
    int no_faces = reg1->NoComponents();
    for(int i = 0;i < no_faces;i++){
        Region* temp = new Region(0);

        result1.push_back(*temp);
        delete temp;
        result1[i].StartBulkLoad();
    }
    for(int i = 0;i < reg1->Size();i++){
      HalfSegment hs;
      reg1->Get(i,hs);
      int face = hs.attr.faceno;
      result1[face] += hs;
    }

    for(int i = 0;i < no_faces;i++){
        result1[i].SetNoComponents(1);
        result1[i].EndBulkLoad(false,false,false,false);
        if(result1[i].Size() >= 6)
          result.push_back(result1[i]);
    }


    vector<Region> result2;
    no_faces = reg2->NoComponents();
    for(int i = 0;i < no_faces;i++){
        Region* temp = new Region(0);

        result2.push_back(*temp);
        delete temp;
        result2[i].StartBulkLoad();
    }
    for(int i = 0;i < reg2->Size();i++){
      HalfSegment hs;
      reg2->Get(i,hs);
      int face = hs.attr.faceno;
      result2[face] += hs;
    }
    for(int i = 0;i < no_faces;i++){
        result2[i].SetNoComponents(1);
        result2[i].EndBulkLoad(false,false,false,false);
        if(result2[i].Size() >= 6)
          result.push_back(result2[i]);
    }

}

/*
get the closest point in hs to p, and return it in cp
it also returns the distance between hs and p

*/
double SpacePartition::GetClosestPoint(HalfSegment& hs, Point& p, Point& cp)
{

  assert( p.IsDefined() );
  Coord xl = hs.GetLeftPoint().GetX(),
        yl = hs.GetLeftPoint().GetY(),
        xr = hs.GetRightPoint().GetX(),
        yr = hs.GetRightPoint().GetY(),
        X = p.GetX(),
        Y = p.GetY();

  double result, auxresult;

  if( xl == xr || yl == yr ){
    if( xl == xr){ //hs is vertical
      if( (yl <= Y && Y <= yr) || (yr <= Y && Y <= yl) ){
        result = fabs( X - xl );
        cp.Set(xl, Y); //store the closest point
      }
      else{
        result = p.Distance(hs.GetLeftPoint());
        auxresult = p.Distance(hs.GetRightPoint());
        if( result > auxresult ){
          result = auxresult;
          cp = hs.GetRightPoint(); //store the closest point
        }else{
          cp = hs.GetLeftPoint();  //store the closest point
        }
      }
    }else{         //hs is horizontal line: (yl==yr)
      if( xl <= X && X <= xr ){
        result = fabs( Y - yl );
        cp.Set(X,yl);//store the closest point
      }else{
        result = p.Distance(hs.GetLeftPoint());
        auxresult = p.Distance(hs.GetRightPoint());
        if( result > auxresult ){
          result = auxresult;
          cp = hs.GetRightPoint();//store the closest point
        }else{
          cp = hs.GetLeftPoint();//store the closest point
        }
      }
    }
  }else
  {
    double k = (yr - yl) / (xr - xl),
           a = yl - k * xl,
           xx = (k * (Y - a) + X) / (k * k + 1),
           yy = k * xx + a;
    Coord XX = xx,
          YY = yy;
    Point PP( true, XX, YY );
    if( xl <= XX && XX <= xr ){
      result = p.Distance( PP );
      cp = PP; //store the closest point
    }
    else
    {
      result = p.Distance( hs.GetLeftPoint() );
      auxresult = p.Distance( hs.GetRightPoint());
      if( result > auxresult ){
        result = auxresult;
        cp = hs.GetRightPoint();
      }else{
        cp = hs.GetLeftPoint();
      }
    }
  }
  return result;

}


/*
Decompose the pavement of one road into a set of subregions

*/

void SpacePartition::DecomposePavement1(Network* n, Relation* rel,
                                 int attr_pos1, int attr_pos2, int attr_pos3)
{
    vector<Region> paves1;
    vector<Region> paves2;
    vector<bool> route_flag;
    for(int i = 1;i <=  rel->GetNoTuples();i++){
      Tuple* pave_tuple = rel->GetTuple(i);
      Region* reg1 = (Region*)pave_tuple->GetAttribute(attr_pos2);
      Region* reg2 = (Region*)pave_tuple->GetAttribute(attr_pos3);
      paves1.push_back(*reg1);
      paves2.push_back(*reg2);
      pave_tuple->DeleteIfAllowed();
      route_flag.push_back(false);
    }


    vector<Region> pavements1;
    vector<Region> pavements2;

    int oid = 1;//object identifier

    assert(paves1.size() == paves2.size());
    for(int i = 1;i <=  rel->GetNoTuples();i++){
      Tuple* pave_tuple = rel->GetTuple(i);
      int rid = ((CcInt*)pave_tuple->GetAttribute(attr_pos1))->GetIntval();

/*      if(!(rid == 1306 || rid == 1626)){
          pave_tuple->DeleteIfAllowed();
          continue;
      }*/

      DecomposePave(&paves1[rid - 1], &paves2[rid - 1], pavements1);
      for(unsigned int j = 0;j < pavements1.size();j++){
          junid1.push_back(oid);
          junid2.push_back(rid);
          outer_regions1.push_back(pavements1[j]);
          oid++;
      }
      pave_tuple->DeleteIfAllowed();
      pavements1.clear();
    }
}


/*
Decompose the zebra crossings into a set of subregions

*/

void SpacePartition::DecomposePavement2(int start_oid, Relation* rel,
                                 int attr_pos1, int attr_pos2)
{
    cout<<"start_oid "<<start_oid<<endl;
    int oid = start_oid + 1;//object identifier
    vector<Region> zc_regs;
    for(int i = 1;i <= rel->GetNoTuples();i++){
      Tuple* zc_tuple = rel->GetTuple(i);
      int rid = ((CcInt*)zc_tuple->GetAttribute(attr_pos1))->GetIntval();
      Region* zc_reg = (Region*)zc_tuple->GetAttribute(attr_pos2);
      Region* temp = new Region(0);
//      cout<<"rid "<<rid<<endl;
      DecomposePave(zc_reg, temp, zc_regs);
//      assert(zc_regs.size() > 0);
//      cout<<zc_regs.size()<<endl;
      for(unsigned int j = 0;j < zc_regs.size();j++){
          junid1.push_back(oid);
          junid2.push_back(rid);
          outer_regions1.push_back(zc_regs[j]);
          oid++;
      }
      delete temp;
      zc_regs.clear();
      zc_tuple->DeleteIfAllowed();
    }
}

/*
get the commone line between two pavements (node in the graph model)

*/
void SpacePartition::GetPavementNode1(Network* n, Relation* rel,
                                    BTree* btree_pave,
                                    int attr1, int attr2, int attr3)
{

    Relation* juns = n->GetJunctions();

    vector<Region_Oid> regs1;
    vector<Region_Oid> regs2;

    for(int i = 1;i <= n->GetNoJunctions();i++){
//    for(int i = 1;i <= 2000;i++){
      Tuple* jun_tuple = juns->GetTuple(i);
      CcInt* rid1 = (CcInt*)jun_tuple->GetAttribute(JUNCTION_ROUTE1_ID);
      CcInt* rid2 = (CcInt*)jun_tuple->GetAttribute(JUNCTION_ROUTE2_ID);
      int id1 = rid1->GetIntval();
      int id2 = rid2->GetIntval();

/*      if(!(id1 == 3 && id2 == 6)){
          jun_tuple->DeleteIfAllowed();
          continue;
      }*/

      cout<<"rid1 "<<id1<<" rid2 "<<id2<<endl;

      BTreeIterator* btreeiter1 = btree_pave->ExactMatch(rid1);
      while(btreeiter1->Next()){
        Tuple* pave_tup = rel->GetTuple(btreeiter1->GetId());
        int oid = ((CcInt*)pave_tup->GetAttribute(attr1))->GetIntval();
        Region* pave = (Region*)pave_tup->GetAttribute(attr3);
        Region_Oid* ro = new Region_Oid(oid, *pave);
        regs1.push_back(*ro);
        delete ro;
        pave_tup->DeleteIfAllowed();
      }
      delete btreeiter1;

      BTreeIterator* btreeiter2 = btree_pave->ExactMatch(rid2);
      while(btreeiter2->Next()){
        Tuple* pave_tup = rel->GetTuple(btreeiter2->GetId());
        int oid = ((CcInt*)pave_tup->GetAttribute(attr1))->GetIntval();
        Region* pave = (Region*)pave_tup->GetAttribute(attr3);
        Region_Oid* ro = new Region_Oid(oid, *pave);
        regs2.push_back(*ro);
        delete ro;
        pave_tup->DeleteIfAllowed();
      }
      delete btreeiter2;
//      cout<<regs1.size()<<" "<<regs2.size()<<endl;

      GetCommPave1(regs1, regs2, id1, id2);

      regs1.clear();
      regs2.clear();
      jun_tuple->DeleteIfAllowed();


    }

    juns->Delete();

}


/*
get the commone line between zc and pavement (node in the graph model)

*/
void SpacePartition::GetPavementNode2(Relation* rel1,
                                    Relation* rel2, BTree* btree_pave,
                                    int attr1, int attr2, int attr3)
{
  vector<Region_Oid> reg_pave;
  for(int i = 1;i <= rel1->GetNoTuples();i++){
    Tuple* zc_tuple = rel1->GetTuple(i);
    CcInt* zc_oid = (CcInt*)zc_tuple->GetAttribute(attr1);
    CcInt* rid = (CcInt*)zc_tuple->GetAttribute(attr2);
    Region* reg = (Region*)zc_tuple->GetAttribute(attr3);


/*    if(!(rid->GetIntval() == 476)){
        zc_tuple->DeleteIfAllowed();
        continue;
    }*/

//    cout<<"rid "<<rid->GetIntval()<<endl;
//    assert(reg->GetCycleDirection());

    BTreeIterator* btreeiter = btree_pave->ExactMatch(rid);
    while(btreeiter->Next()){
        Tuple* pave_tuple = rel2->GetTuple(btreeiter->GetId());
        int oid = ((CcInt*)pave_tuple->GetAttribute(attr1))->GetIntval();
        Region* pave = (Region*)pave_tuple->GetAttribute(attr3);
        Region_Oid* ro = new Region_Oid(oid, *pave);
        reg_pave.push_back(*ro);
        delete ro;
        pave_tuple->DeleteIfAllowed();
    }
    delete btreeiter;


    GetCommPave2(reg, zc_oid->GetIntval(),reg_pave);
    reg_pave.clear();
    zc_tuple->DeleteIfAllowed();
  }

}

/*
calculate the common border line of two pavements

*/
void SpacePartition::GetCommPave1(vector<Region_Oid>& pave1,
                                 vector<Region_Oid>& pave2, int rid1, int rid2)
{
//  const double delta_dist = 0.1;
  for(unsigned int i = 0;i < pave1.size();i++){
      for(unsigned int j = 0;j < pave2.size();j++){

          if(pave1[i].reg.Inside(pave2[j].reg)){
              continue;
          }
          if(pave2[j].reg.Inside(pave1[i].reg)){
              continue;
          }

          if(MyRegIntersects(&pave1[i].reg, &pave2[j].reg)){
//            cout<<"rid1 "<<rid1<<" oid "<<pave1[i].oid<<endl;
//            cout<<"rid2 "<<rid2<<" oid "<<pave2[j].oid<<endl;

            Region* comm = new Region(0);
            MyIntersection(pave1[i].reg, pave2[j].reg, *comm);

//            pave1[i].reg.Intersection(pave2[j].reg, *comm);

            if(comm->Size() == 0){ //the commone part is a line

              Line* boundary1 = new Line(0);
              pave1[i].reg.Boundary(boundary1);
              Line* result = new Line(0);
//              pave2[j].reg.Intersection(*boundary1, *result);

              MyIntersection(*boundary1, pave2[j].reg, *result);

              if(result->Size() == 0){
                  cout<<comm->Area()<<endl;
                  cout<<"rid1 "<<rid1<<" rid2 "<<rid2<<endl;
                  cout<<"should not here1"<<endl;
              }
              if(result->Size() > 0){
                  junid1.push_back(pave1[i].oid);
                  junid2.push_back(pave2[j].oid);
                  pave_line1.push_back(*result);

                //////////////////////////////////////
                  Line* l1 = new Line(0);
                  Line* l2 = new Line(0);
                  GetCommonLine(&pave1[i].reg, result, l1);
                  GetCommonLine(&pave2[j].reg, result, l2);
                  assert(result->Inside(pave1[i].reg));
                  assert(result->Inside(pave2[j].reg));
                  delete l1;
                  delete l2;
                ///////////////////////////////////////
              }
              delete result;
              delete boundary1;
            }else{    //a commone area

              Line* boundary1 = new Line(0);
              comm->Boundary(boundary1);
              Region* cut_pave1 = new Region(0);
              MyMinus(pave1[i].reg, *comm, *cut_pave1);
//              pave1[i].reg.Minus(*comm, *cut_pave1);

              Line* result = new Line(0);
//              cut_pave1->Intersection(*boundary1, *result);
              MyIntersection(*boundary1, *cut_pave1, *result);


              if(result->Size() == 0){
                  cout<<comm->Area()<<endl;
                  cout<<"rid1 "<<rid1<<" rid2 "<<rid2<<endl;
                  cout<<"should not here2"<<endl;
              }
              if(result->Size() > 0){
                  junid1.push_back(pave1[i].oid);
                  junid2.push_back(pave2[j].oid);
                  pave_line1.push_back(*result);

              /////////////////////////////////////////////////////
                  Line* l1 = new Line(0);
                  Line* l2 = new Line(0);
//                  GetCommonLine(&pave1[i].reg, result, l1);
                  GetCommonLine(cut_pave1, result, l1);
                  GetCommonLine(&pave2[j].reg, result, l2);
                  assert(l1->Inside(pave1[i].reg));
                  assert(l2->Inside(pave2[j].reg));
                  delete l1;
                  delete l2;
              /////////////////////////////////////////////////////

              }
              delete result;
              delete cut_pave1;
              delete boundary1;
            }
            delete comm;
        }
      }
  }

}

/*
get the common border line between zebra crossing and the pavement

*/
void SpacePartition::GetCommPave2(Region* reg, int oid,
                                  vector<Region_Oid>& pave2)
{
    for(unsigned int i = 0;i < pave2.size();i++){
        if(MyRegIntersects(reg, &pave2[i].reg)){

          Region* comm = new Region(0);
          MyIntersection(*reg, pave2[i].reg, *comm);
//          cout<<comm->Size()<<endl;
          assert(comm->Size() > 0);

          Region* temp = new Region(0);
          MyMinus(*reg, *comm, *temp);

          Line* boundary = new Line(0);
          comm->Boundary(boundary);
          Line* result = new Line(0);
//          pave2[i].reg.Intersection(*boundary, *result);
          MyIntersection(*boundary, *temp, *result);

          if(result->Size() == 0){
            cout<<"zc oid1 "<<oid<<endl;
            cout<<"pave oid2 "<<pave2[i].oid<<endl;
          }
          if(result->Size() > 0){
              junid1.push_back(oid);
              junid2.push_back(pave2[i].oid);
              pave_line1.push_back(*result);
          }
          delete result;
          delete boundary;
          delete comm;
          delete temp;
        }
    }

}

/*
find the matching line in reg by input line

*/
void SpacePartition::GetCommonLine(Region* reg, Line* line, Line* res)
{
  cout<<"GetCommonLine() "<<endl;
  Line* boundary = new Line(0);
  reg->Boundary(boundary);
  SimpleLine* sboundary = new SimpleLine(0);
  sboundary->fromLine(*boundary);


  Line* result = new Line(0);
  for(int i = 0;i < line->Size();i++){
      HalfSegment hs1;
      line->Get(i, hs1);
      if(!hs1.IsLeftDomPoint()) continue;

      Point sp, ep;
      sp = hs1.GetLeftPoint();
      ep = hs1.GetRightPoint();

      vector<MyPoint> mps1;
      vector<MyPoint> mps2;

      for(int j = 0;j < sboundary->Size();j++){
        HalfSegment hs2;
        sboundary->Get(j, hs2);
        if(hs2.IsLeftDomPoint()){
          Point cp1, cp2;
  //      printf("dist1 %.8f\n",GetClosestPoint(hs, sp, cp1));

          double dist1 = GetClosestPoint(hs2, sp, cp1);
          double dist2 = GetClosestPoint(hs2, ep, cp2);
          MyPoint* mp1 = new MyPoint(cp1, dist1);
          MyPoint* mp2 = new MyPoint(cp2, dist2);
          mps1.push_back(*mp1);
          mps2.push_back(*mp2);
          delete mp1;
          delete mp2;
        }
      }
      sort(mps1.begin(), mps1.end());
      sort(mps2.begin(), mps2.end());
  //  mps1[0].Print();
  //  mps2[0].Print();

      cout<<"hs1 length "<<hs1.Length()<<endl;
      cout<<"original "<<hs1<<endl;
      Line* subline = new Line(0);
      GetSubLine(sboundary, mps1[0].loc, mps2[0].loc, subline, hs1.Length());
      cout<<"subline length "<<subline->Length()<<endl;
      cout<<"matching "<<*subline<<endl;
      Line* temp = new Line(0);
      result->Union(*subline, *temp);
      *result = *temp;

      delete temp;
      delete subline;
  }
  cout<<"comm line length "<<line->Length()<<endl;
  cout<<"result length "<<result->Length()<<endl;
  const double delta_dist = 0.1;
  assert(fabs(line->Length() - result->Length()) < delta_dist);


  *res = *result;

  delete result;
  delete boundary;
  delete sboundary;
}


void SpacePartition::GetSubLine(SimpleLine* sl, Point& p1,
                                Point& p2, Line* res, double len)
{
//    cout<<"boundary length "<<sl->Length()<<endl;
    vector<MyHalfSegment> mhs;
    ReorderLine(sl, mhs);

    printf("p1 (%.8f,%.8f)  p2 (%.8f, %.8f)\n", p1.GetX(), p1.GetY(),
                                                p2.GetX(), p2.GetY());
//    printf("p1-p2 dist %.8f\n",p1.Distance(p2));
    const double delta_dist = 0.1;

/*    for(unsigned int i = 0;i < mhs.size();i++)
      mhs[i].Print();*/

    int pos1, pos2;
    pos1 = pos2 = -1;
    MyPoint mp1, mp2;
    bool f1, f2;
    f1 = false;
    f2 = false;
    for(unsigned int i = 0;i < mhs.size();i++){
        HalfSegment hs;
        hs.Set(true, mhs[i].from, mhs[i].to);

        Point cp1, cp2;
//        mhs[i].Print();
//        printf("dist1 %.8f dist2 %.8f\n",GetClosestPoint(hs, p1, cp1),
//                                         GetClosestPoint(hs, p2, cp2));
        double dist1 = GetClosestPoint(hs, p1, cp1);
        double dist2 = GetClosestPoint(hs, p2, cp2);
        if(dist1 < delta_dist){
            if(f1 == false){
              mp1.loc = cp1;
              mp1.dist = dist1;
              f1 = true;
              pos1 = i;
            }else{
              if(dist1 < mp1.dist){
                mp1.loc = cp1;
                mp1.dist = dist1;
                pos1 = i;
              }
            }
        }
        if(dist2 < delta_dist){
            if(f2 == false){
              mp2.loc = cp2;
              mp2.dist = dist2;
              f2 = true;
              pos2 = i;
            }else{
              if(dist2 < mp2.dist){
                mp2.loc = cp2;
                mp2.dist = dist2;
                pos2 = i;
              }
            }
        }

    }
//    cout<<"pos1 "<<pos1<<" pos2 "<<pos2<<endl;
    assert(pos1 != -1 && pos2 != -1);
    int index1, index2;
    if(pos1 < pos2){
      index1 = pos1;
      index2 = pos2;
    }else{
      index1 = pos2;
      index2 = pos1;
      Point p;
      p = p1;
      p1 = p2;
      p2 = p;
    }
//    cout<<"index1 "<<index1<<" index2 "<<index2<<endl;
    assert(index1 <= index2);


    int edgeno1 = 0;
    int edgeno2 = 0;
    Line* l1 = new Line(0);
    Line* l2 = new Line(0);
    l1->StartBulkLoad();
    l2->StartBulkLoad();
    if(index1 != index2){
        for(int i = 0;i < index1;i++){
          HalfSegment hs;
          hs.Set(true, mhs[i].from, mhs[i].to);
          hs.attr.edgeno = edgeno1++;
          *l1 += hs;
          hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
          *l1 += hs;
        }
        if(p1.Distance(mhs[index1].from) < delta_dist){
//          cout<<"l2 1"<<endl;
          HalfSegment hs;
          hs.Set(true, mhs[index1].from, mhs[index1].to);
          hs.attr.edgeno = edgeno2++;
          *l2 += hs;
          hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
          *l2 += hs;
        }else if(p1.Distance(mhs[index1].to) < delta_dist){
          HalfSegment hs;
          hs.Set(true, mhs[index1].from, mhs[index1].to);
          hs.attr.edgeno = edgeno1++;
          *l1 += hs;
          hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
          *l1 += hs;
        }else{
          HalfSegment hs1;
          hs1.Set(true, mhs[index1].from, p1);
          hs1.attr.edgeno = edgeno1++;
          *l1 += hs1;
          hs1.SetLeftDomPoint(!hs1.IsLeftDomPoint());
          *l1 += hs1;

//          cout<<"l2 2"<<endl;
          HalfSegment hs2;
          hs2.Set(true, p1, mhs[index1].to);
          hs2.attr.edgeno = edgeno2++;
          *l2 += hs2;
          hs2.SetLeftDomPoint(!hs2.IsLeftDomPoint());
          *l2 += hs2;
      }


        for(int i = index1 + 1; i < index2;i++){
//          cout<<"l2 3"<<endl;
          HalfSegment hs;
          hs.Set(true, mhs[i].from, mhs[i].to);
          hs.attr.edgeno = edgeno2++;
          *l2 += hs;
          hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
          *l2 += hs;
        }


        if(p2.Distance(mhs[index2].from) < delta_dist){
          HalfSegment hs;
          hs.Set(true, mhs[index2].from, mhs[index2].to);
          hs.attr.edgeno = edgeno1++;
          *l1 += hs;
          hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
          *l1 += hs;
        }else if(p2.Distance(mhs[index2].to) < delta_dist){
//          cout<<"l2 4"<<endl;
          HalfSegment hs;
          hs.Set(true, mhs[index2].from, mhs[index2].to);
          hs.attr.edgeno = edgeno2++;
          *l2 += hs;
          hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
          *l2 += hs;
        }else{
//          cout<<"l2 5"<<endl;
          HalfSegment hs1;
          hs1.Set(true, mhs[index2].from, p2);
          hs1.attr.edgeno = edgeno2++;
          *l2 += hs1;
          hs1.SetLeftDomPoint(!hs1.IsLeftDomPoint());
          *l2 += hs1;

          HalfSegment hs2;
          hs2.Set(true, p2, mhs[index2].to);
          hs2.attr.edgeno = edgeno1++;
          *l1 += hs2;
          hs2.SetLeftDomPoint(!hs2.IsLeftDomPoint());
          *l1 += hs2;
        }


      for(unsigned int i = index2 + 1;i < mhs.size();i++){
          HalfSegment hs;
          hs.Set(true, mhs[i].from, mhs[i].to);
          hs.attr.edgeno = edgeno1++;
          *l1 += hs;
          hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
          *l1 += hs;
      }
    }else{

        for(int i = 0;i < index1;i++){
          HalfSegment hs;
          hs.Set(true, mhs[i].from, mhs[i].to);
          hs.attr.edgeno = edgeno1++;
          *l1 += hs;
          hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
          *l1 += hs;
        }
//          cout<<"l2 6"<<endl;
        HalfSegment hs;
        hs.Set(true, p1, p2);
        hs.attr.edgeno = edgeno2++;
        *l2 += hs;
        hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
        *l2 += hs;


        for(unsigned int i = index2 + 1;i < mhs.size();i++){
          HalfSegment hs;
          hs.Set(true, mhs[i].from, mhs[i].to);
          hs.attr.edgeno = edgeno1++;
          *l1 += hs;
          hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
          *l1 += hs;
        }
    }




    l1->EndBulkLoad();
    l2->EndBulkLoad();

//    cout<<"len "<<len<<endl;
    cout<<"l1 length "<<l1->Length()<<endl;
    cout<<"l2 length "<<l2->Length()<<endl;
    assert(fabs(len - l1->Length() < delta_dist) ||
           fabs(len - l2->Length() < delta_dist));
    double d1 = fabs(len - l1->Length());
    double d2 = fabs(len - l2->Length());
    if(d1 < d2) *res = *l1;
    else *res = *l2;


    delete l1;
    delete l2;
}

/*
doing triangulation for a polygon

*/

CompTriangle::CompTriangle()
{
  count = 0;
  path = NULL;
  resulttype = NULL;
}
CompTriangle::CompTriangle(Region* r):reg(r),count(0),
path(NULL),resulttype(NULL)
{

}

CompTriangle:: ~CompTriangle()
{
  if(path != NULL) delete path;
  if(resulttype != NULL) delete resulttype;
}
/*
Compute the area of a polygon

*/

float CompTriangle::Area(const vector<Point>& contour)
{
  int n = contour.size();

  float A=0.0f;

  for(int p=n-1,q=0; q<n; p=q++)
  {
    A+=
    contour[p].GetX()*contour[q].GetY() - contour[q].GetX()*contour[p].GetY();
  }
  return A*0.5f;

}

/*
  InsideTriangle decides if a point P is Inside of the triangle
  defined by A, B, C.

*/
bool CompTriangle::InsideTriangle(float Ax, float Ay,
                      float Bx, float By,
                      float Cx, float Cy,
                      float Px, float Py)

{
  float ax, ay, bx, by, cx, cy, apx, apy, bpx, bpy, cpx, cpy;
  float cCROSSap, bCROSScp, aCROSSbp;

  ax = Cx - Bx;  ay = Cy - By;
  bx = Ax - Cx;  by = Ay - Cy;
  cx = Bx - Ax;  cy = By - Ay;
  apx= Px - Ax;  apy= Py - Ay;
  bpx= Px - Bx;  bpy= Py - By;
  cpx= Px - Cx;  cpy= Py - Cy;

  aCROSSbp = ax*bpy - ay*bpx;
  cCROSSap = cx*apy - cy*apx;
  bCROSScp = bx*cpy - by*cpx;

  return ((aCROSSbp >= 0.0f) && (bCROSScp >= 0.0f) && (cCROSSap >= 0.0f));
};


bool CompTriangle::Snip(const vector<Point>& contour,int u,int v,int w,
                       int n,int *V)
{
  int p;
  float Ax, Ay, Bx, By, Cx, Cy, Px, Py;

  Ax = contour[V[u]].GetX();
  Ay = contour[V[u]].GetY();

  Bx = contour[V[v]].GetX();
  By = contour[V[v]].GetY();

  Cx = contour[V[w]].GetX();
  Cy = contour[V[w]].GetY();

  if ( EPSILON > (((Bx-Ax)*(Cy-Ay)) - ((By-Ay)*(Cx-Ax))) ) return false;

  for (p=0;p<n;p++)
  {
    if( (p == u) || (p == v) || (p == w) ) continue;
    Px = contour[V[p]].GetX();
    Py = contour[V[p]].GetY();
    if (InsideTriangle(Ax,Ay,Bx,By,Cx,Cy,Px,Py)) return false;
  }

  return true;
}

/*
decompose a polygon into a set of triangles

*/

bool CompTriangle::GetTriangles(const vector<Point>& contour,
                                vector<Point>& result)
{
    /* allocate and initialize list of Vertices in polygon */

  int n = contour.size();
  if ( n < 3 ) return false;

  int *V = new int[n];

  /* we want a counter-clockwise polygon in V */
//  cout<<"Area "<<Area(contour)<<endl;

  if ( 0.0f < Area(contour) )
    for (int v=0; v<n; v++) V[v] = v;
  else
    for(int v=0; v<n; v++) V[v] = (n-1)-v;

  int nv = n;

  /*  remove nv-2 Vertices, creating 1 triangle every time */
  int count = 2*nv;   /* error detection */

  for(int m=0, v=nv-1; nv>2; )
  {
    /* if we loop, it is probably a non-simple polygon */
    if (0 >= (count--))
    {
      //** Triangulate: ERROR - probable bad polygon!
      return false;
    }

    /* three consecutive vertices in current polygon, <u,v,w> */
    int u = v  ; if (nv <= u) u = 0;     /* previous */
    v = u+1; if (nv <= v) v = 0;     /* new v    */
    int w = v+1; if (nv <= w) w = 0;     /* next     */

    if ( Snip(contour,u,v,w,nv,V) )
    {
      int a,b,c,s,t;

      /* true names of the vertices */
      a = V[u]; b = V[v]; c = V[w];

      /* output Triangle */
      result.push_back( contour[a] );
      result.push_back( contour[b] );
      result.push_back( contour[c] );

      m++;

      /* remove v from remaining polygon */
      for(s=v,t=v+1;t<nv;s++,t++) V[s] = V[t]; nv--;

      /* resest error detection counter */
      count = 2*nv;
    }
  }

  delete []V;

  return true;
}

/*
Decompose the region into a set of triangles

*/
void CompTriangle::Triangulation()
{
  if(reg->NoComponents() == 0){
      cout<<"this is not a region"<<endl;
      return;
  }
  if(reg->NoComponents() > 1){
      cout<<"there is hole inside or several subregions"<<endl;
      return;
  }
  Line* boundary = new Line(0);
  reg->Boundary(boundary);
//  cout<<"boundary "<<*boundary<<endl;
  SimpleLine* sboundary = new SimpleLine(0);
  sboundary->fromLine(*boundary);
//  cout<<"sboundary size "<<sboundary->Size()<<endl;
  vector<MyHalfSegment> mhs;
  //get all the points of the region
  SpacePartition* sp = new SpacePartition();
  if(sboundary->Size() > 0)
    sp->ReorderLine(sboundary, mhs);
  else{
    cout<<"can't covert the boundary to a sline"<<endl;
    delete boundary;
    delete sboundary;
    return;
  }
  delete boundary;
  delete sboundary;

/*  for(unsigned int i = 0;i < mhs.size();i++)
        mhs[i].Print();*/

  vector<Point> ps;
  for(unsigned int i = 0;i < mhs.size();i++)
    ps.push_back(mhs[i].from);


/*  for(unsigned int i = 0;i < ps.size();i++){
    printf("%.8f %.8f\n",ps[i].GetX(), ps[i].GetY());
  }*/

  vector<Point> result;
  GetTriangles(ps, result);

  unsigned int tcount = result.size()/3;

  for (unsigned int i=0; i<tcount; i++)
  {
     Point p1 = result[i*3+0];
     Point p2 = result[i*3+1];
     Point p3 = result[i*3+2];
/*     printf("Triangle %d => (%.5f,%.5f) (%.5f,%.5f) (%.5f,%.5f)\n", i + 1,
          p1.GetX(), p1.GetY(),
          p2.GetX(), p2.GetY(),
          p3.GetX(), p3.GetY());*/

    vector<Point> reg_ps;

    reg_ps.push_back(p1);
    reg_ps.push_back(p2);
    reg_ps.push_back(p3);

    sp->ComputeRegion(reg_ps, triangles);
  }

  delete sp;

}

/*
if the polygon is convex, returns true, otherwise (concave) returns false

*/
bool CompTriangle::PolygonConvex()
{
  if(reg->NoComponents() == 0){
      cout<<"error: this is not a region"<<endl;
      return false;
  }
  if(reg->NoComponents() > 1){
      cout<<"error: there is hole inside or several subregions"<<endl;
      return false;
  }

  Line* boundary = new Line(0);
  reg->Boundary(boundary);
//  cout<<"boundary "<<*boundary<<endl;
  SimpleLine* sboundary = new SimpleLine(0);
  sboundary->fromLine(*boundary);
//  cout<<"sboundary size "<<sboundary->Size()<<endl;
  vector<MyHalfSegment> mhs;
  //get all the points of the region
  SpacePartition* sp = new SpacePartition();
  if(sboundary->Size() > 0)
    sp->ReorderLine(sboundary, mhs);
  else{
    cout<<"can't covert the boundary to a sline"<<endl;
    delete boundary;
    delete sboundary;
    return false;
  }
  delete boundary;
  delete sboundary;

/*  for(unsigned int i = 0;i < mhs.size();i++)
        mhs[i].Print();*/

  vector<Point> ps;
  for(unsigned int i = 0;i < mhs.size();i++)
    ps.push_back(mhs[i].from);
  ///////////////      convex/concave        /////////////////////////////
   int n = ps.size();
   int i,j,k;
   int flag = 0;
   double z;

   if (ps.size() < 3){
      cout<<"less than 3 points, it is not a region"<<endl;
      return false;
   }
   for (i=0;i<n;i++) {
      j = (i + 1) % n;
      k = (i + 2) % n;
//      z  = (p[j].x - p[i].x) * (p[k].y - p[j].y);
//      z -= (p[j].y - p[i].y) * (p[k].x - p[j].x);
      z  = (ps[j].GetX() - ps[i].GetX()) * (ps[k].GetY() - ps[j].GetY());
      z -= (ps[j].GetY() - ps[i].GetY()) * (ps[k].GetX() - ps[j].GetX());

      if (z < 0)
         flag |= 1;
      else if (z > 0)
         flag |= 2;
      if (flag == 3)
         return false;
   }
   if (flag != 0)
      return true;
   else
      return false;

  /////////////////////////////////////////////////////////////////////////

}
/*
structure for shortest path searching in a polygon
the graph is build the decomposed triangles.
it finds the path with minimum number of triangles connecting the start point
and the end point

*/

struct SPath_elem{
  int prev_index;
  int cur_index;
  unsigned int tri_index;
  unsigned int weight;
  SPath_elem(){}
  SPath_elem(int p, int c, int t, int w):prev_index(p), cur_index(c),
                   tri_index(t),weight(w){}
  SPath_elem(const SPath_elem& se):prev_index(se.prev_index),
                       cur_index(se.cur_index), tri_index(se.tri_index),
                       weight(se.weight){}
  SPath_elem& operator=(const SPath_elem& se)
  {
    prev_index = se.prev_index;
    cur_index = se.cur_index;
    tri_index = se.tri_index;
    weight = se.weight;
    return *this;
  }
  bool operator<(const SPath_elem& se) const
  {
    return weight > se.weight;
  }

  void Print()
  {
    cout<<"prev "<<prev_index<<" cur "<<cur_index
        <<"tri_index" <<tri_index<<"weight "<<weight<<endl;
  }
};
/*
get a sequence of triangles where the shoretest path should pass through

*/
void CompTriangle::FindAdj(unsigned int index, vector<bool>& flag,
                           vector<int>& adj_list)
{
  vector<HalfSegment> cur_triangle;
  for(int i = 0;i < triangles[index].Size();i++){
    HalfSegment hs;
    triangles[index].Get(i, hs);
    if(!hs.IsLeftDomPoint())continue;
    cur_triangle.push_back(hs);
  }
  assert(cur_triangle.size() == 3);

  const double delta_dist = 0.00001;
  for(unsigned int i = 0;i < triangles.size();i++){
    if(flag[i] == true) continue;
    ///////////////////get the edges////////////////////////
    vector<HalfSegment> triangle;
    for(int j = 0;j < triangles[i].Size();j++){
      HalfSegment hs;
      triangles[i].Get(j, hs);
      if(!hs.IsLeftDomPoint())continue;
      triangle.push_back(hs);
    }
    assert(triangle.size() == 3);
    ////////////////////////////////////////////////////////////
    for(unsigned int k1 = 0;k1 < cur_triangle.size();k1++){
        Point p1 = cur_triangle[k1].GetLeftPoint();
        Point p2 = cur_triangle[k1].GetRightPoint();
        unsigned int k2 = 0;
      for(;k2 < triangle.size();k2++){
        Point p3 = triangle[k2].GetLeftPoint();
        Point p4 = triangle[k2].GetRightPoint();
        if(p1.Distance(p3) < delta_dist && p2.Distance(p4) < delta_dist){
          adj_list.push_back(i);
          flag[i] = true;
          break;
        }
        if(p1.Distance(p4) < delta_dist && p2.Distance(p3) < delta_dist){
          adj_list.push_back(i);
          flag[i] = true;
          break;
        }
      }
      if(k2 != triangle.size())break;
    }

  }

}

/*
get a sequence of triangles from the start point to the end point

*/
void CompTriangle::GetChannel(Point* start, Point* end)
{
  ////////// find the start triangle /////////////////////////
  int index1 = -1;
  int index2 = -1;
  for(unsigned int i = 0;i < triangles.size();i++){
      if(start->Inside(triangles[i])){
        index1 = i;
        break;
      }
  }
  ////////// find the end triangle /////////////////////////
  for(unsigned int i = 0;i < triangles.size();i++){
      if(end->Inside(triangles[i])){
        index2 = i;
        break;
      }
  }
  assert(index1 != -1 && index2 != -1);
  cout<<"index1 "<<index1<<" index2 "<<index2<<endl;
  vector<bool> triangle_flag;
  for(unsigned int i = 0;i < triangles.size();i++)
    triangle_flag.push_back(false);

  triangle_flag[index1] = true;

  ////////////////shortest path algorithm///////////////////////
  priority_queue<SPath_elem> path_queue;
  vector<SPath_elem> expand_path;

  path_queue.push(SPath_elem(-1, 0, index1, 1));
  expand_path.push_back(SPath_elem(-1,0, index1, 1));
  bool find = false;
  SPath_elem dest;//////////destination
  while(path_queue.empty() == false){
    SPath_elem top = path_queue.top();
    path_queue.pop();
//    top.Print();
    if(top.tri_index == index2){
       cout<<"find the path"<<endl;
       find = true;
       dest = top;
       break;
    }
    ////////find its adjacecy element, and push them into queue and path//////
    vector<int> adj_list;
    FindAdj(top.tri_index, triangle_flag, adj_list);
//    cout<<"adjcency_list size "<<adj_list.size()<<endl;
//    cout<<"expand_path_size "<<expand_path.size()<<endl;
    int pos_expand_path = top.cur_index;
    for(unsigned int i = 0;i < adj_list.size();i++){
      int expand_path_size = expand_path.size();
      path_queue.push(SPath_elem(pos_expand_path, expand_path_size,
                                adj_list[i], top.weight+1));
      expand_path.push_back(SPath_elem(pos_expand_path, expand_path_size,
                            adj_list[i], top.weight+1));
    }
  }
  ///////////////construct the path/////////////////////////////
  if(find){
    vector<int> path_record;
    while(dest.prev_index != -1){
//      sleeve.push_back(triangles[dest.tri_index]);
      path_record.push_back(dest.tri_index);
      dest = expand_path[dest.prev_index];
    }
//    sleeve.push_back(triangles[dest.tri_index]);
    path_record.push_back(dest.tri_index);

    for(int i = path_record.size() - 1;i >= 0;i--)
      sleeve.push_back(triangles[path_record[i]]);
  }

}

/*
build the channel/funnel

*/
void CompTriangle::ConstructConvexChannel1(list<MyPoint>& funnel_front,
                              list<MyPoint>& funnel_back,
                              Point& newvertex,
                              vector<Point>& path, bool front_back)
{
  cout<<"ConstructConvexChannel1 "<<endl;
  const double delta_dist = 0.00001;
  //push newvertex into funnel_back
  if(front_back){ //front = newvertex, check funnel_back first,
//    cout<<"front "<<"push into back "<<endl;
    //case1   case2 check another list
    MyPoint mp1;
    MyPoint mp2 = funnel_back.back();
    while(funnel_back.empty() == false){
        MyPoint elem = funnel_back.back();
        elem.Print();
        HalfSegment hs;
        hs.Set(true, elem.loc, newvertex);
        if(reg->Contains(hs)){
          funnel_back.pop_back();
          mp1 = elem;
        }else break;
    }
    if(funnel_back.empty()){//case 1
//      cout<<"funnel_back empty"<<endl;
      funnel_back.push_back(mp1);
      funnel_back.push_back(MyPoint(newvertex,
                            newvertex.Distance(mp1.loc) + mp1.dist));
    }else{
      if(mp1.loc.Distance(mp2.loc) < delta_dist){ //case2
        double l1 = newvertex.Distance(mp2.loc) + mp2.dist;

        //////////////////////////////
        list<MyPoint>::iterator iter = funnel_front.begin();
        for(;iter != funnel_front.end();iter++){
          Point p = iter->loc;
          HalfSegment hs;
          hs.Set(true, p, newvertex);
          if(reg->Contains(hs))break;
        }
        double l2 = newvertex.Distance(iter->loc) + iter->dist;
        ///////////////////////////////
//        cout<<"l1 "<<l1<<" l2 "<<l2<<endl;
        if(l1 < l2 || AlmostEqual(l1, l2)){
          funnel_back.push_back(mp2);
          funnel_back.push_back(MyPoint(newvertex,l1));
        }else{
            while(funnel_front.empty() == false){
              MyPoint elem = funnel_front.front();
              elem.Print();
              HalfSegment hs;
              hs.Set(true, elem.loc, newvertex);
              if(reg->Contains(hs))break;
              funnel_front.pop_front();
              if(path[path.size() - 1].Distance(elem.loc) > delta_dist)
                  path.push_back(elem.loc);
            }
            assert(funnel_front.empty() == false);
            MyPoint top = funnel_front.front();
            path.push_back(top.loc);


            //update funnel_back
            funnel_back.clear();
            funnel_back.push_back(top);
            funnel_back.push_back(MyPoint(newvertex,
                       newvertex.Distance(top.loc) + top.dist));
        }
      }else{//case 1,find a point in funnel_back directly connected to newvertex
        funnel_back.push_back(mp1);
        funnel_back.push_back(MyPoint(newvertex,
                            newvertex.Distance(mp1.loc) + mp1.dist));
      }
    }
  //push newvertex into funnel_front
  }else{ //back = newvertex, check funnel funnel_front first
//  cout<<"back "<<"push into front "<<endl;
 //case1   case2 check another list
    MyPoint mp1;
    MyPoint mp2 = funnel_front.back();
    while(funnel_front.empty() == false){
        MyPoint elem = funnel_front.back();
        elem.Print();
        HalfSegment hs;
        hs.Set(true, elem.loc, newvertex);
        if(reg->Contains(hs)){
          funnel_front.pop_back();
          mp1 = elem;
        }else break;
    }
    if(funnel_front.empty()){//case 1
//      cout<<"funnel_front empty"<<endl;
      funnel_front.push_back(mp1);
      funnel_front.push_back(MyPoint(newvertex,
                            newvertex.Distance(mp1.loc) + mp1.dist));
    }else{
      if(mp1.loc.Distance(mp2.loc) < delta_dist){ //case2
        double l1 = newvertex.Distance(mp2.loc) + mp2.dist;
        ////////////////////////////////////////////////
        list<MyPoint>::iterator iter = funnel_back.begin();
        for(;iter != funnel_back.end();iter++){
          Point p = iter->loc;
          HalfSegment hs;
          hs.Set(true, p, newvertex);
          if(reg->Contains(hs))break;
        }
        double l2 = newvertex.Distance(iter->loc) + iter->dist;
        /////////////////////////////////////////////////

//        cout<<"l1 "<<l1<<" l2 "<<l2<<endl;
        if(l1 < l2 || AlmostEqual(l1, l2)){
          funnel_front.push_back(mp2);
          funnel_front.push_back(MyPoint(newvertex,l1));
        }
        else{
            while(funnel_back.empty() == false){
              MyPoint elem = funnel_back.front();
              HalfSegment hs;
              hs.Set(true, elem.loc, newvertex);
              if(reg->Contains(hs))break;
              funnel_back.pop_front();
              if(path[path.size() - 1].Distance(elem.loc) > delta_dist)
                  path.push_back(elem.loc);
            }
            assert(funnel_back.empty() == false);
            MyPoint top = funnel_back.front();
            path.push_back(top.loc);


            //update funnel_front
            funnel_front.clear();
            funnel_front.push_back(top);
            funnel_front.push_back(MyPoint(newvertex,
                       newvertex.Distance(top.loc) + top.dist));
        }
      }else{//case 1,find a point in funnel_back directly connected to newvertex
        funnel_front.push_back(mp1);
        funnel_front.push_back(MyPoint(newvertex,
                            newvertex.Distance(mp1.loc) + mp1.dist));
      }
    }

  }

/*  list<MyPoint>::iterator iter = funnel_front.begin();
  cout<<"front ";
  for(;iter != funnel_front.end();iter++){
        Point p = iter->loc;
        cout<<p<<" ";
  }
  cout<<endl<<"back ";
  iter = funnel_back.begin();
  for(;iter != funnel_back.end();iter++){
        Point p = iter->loc;
        cout<<p<<" ";
  }
  cout<<endl;
  cout<<"points in path "<<endl;
  for(unsigned int i = 0;i < path.size();i++)
      cout<<path[i]<<" ";
  cout<<endl;*/

}

/*
complete the final shortest path

*/

void CompTriangle::ConstructConvexChannel2(list<MyPoint> funnel_front,
                              list<MyPoint> funnel_back,
                              Point& newvertex,
                              vector<Point>& path, bool front_back)
{
  cout<<"ConstructConvexChannel2 "<<endl;
  const double delta_dist = 0.00001;
  //push newvertex into funnel_back
  if(front_back){ //front = newvertex, check funnel_back first,
//    cout<<"front "<<"push into back "<<endl;
    //case1   case2 check another list

    MyPoint mp1 = funnel_back.back();
    MyPoint mp2 = funnel_back.front();
    while(funnel_back.empty() == false){
        MyPoint elem = funnel_back.back();
//        elem.Print();
        HalfSegment hs;
        hs.Set(true, elem.loc, newvertex);
        if(reg->Contains(hs)){
          funnel_back.pop_back();
          mp1 = elem;
        }else break;
    }

      double l1 = newvertex.Distance(mp1.loc) + mp1.dist;
        ////////////////////////////////////////////////
        list<MyPoint>::iterator iter = funnel_front.begin();
        for(;iter != funnel_front.end();iter++){
          Point p = iter->loc;
          HalfSegment hs;
          hs.Set(true, p, newvertex);
          if(reg->Contains(hs))break;
        }
        double l2 = newvertex.Distance(iter->loc) + iter->dist;
        /////////////////////////////////////////////////

//      cout<<"l1 "<<l1<<" l2 "<<l2<<endl;
      if(l1 < l2 || AlmostEqual(l1, l2)){

        if(funnel_back.empty() == false)
          funnel_back.pop_front();

        while(funnel_back.empty() == false){
          MyPoint elem = funnel_back.front();
          path.push_back(elem.loc);
          funnel_back.pop_front();
        }
        if(mp1.loc.Distance(mp2.loc) > delta_dist)
          path.push_back(mp1.loc);
      }else{
          MyPoint top = funnel_front.front();

          if(funnel_front.empty() == false)
            funnel_front.pop_front();

          HalfSegment hs;
          hs.Set(true, top.loc, newvertex);
          if(reg->Contains(hs)) return;

          while(funnel_front.empty() == false){
            MyPoint elem = funnel_front.front();
            path.push_back(elem.loc);
            ////////////////////////////////////////
            HalfSegment hs;
            hs.Set(true, elem.loc, newvertex);
            if(reg->Contains(hs))break;
            ///////////////////////////////////////
            funnel_front.pop_front();
          }
      }
    //push newvertex into funnel_front
  }else{ //back = newvertex, check funnel funnel_front first
//  cout<<"back "<<"push into front "<<endl;
 //case1   case2 check another list

    MyPoint mp1 = funnel_front.back();
    MyPoint mp2 = funnel_front.front();
    while(funnel_front.empty() == false){
        MyPoint elem = funnel_front.back();
//        elem.Print();
        HalfSegment hs;
        hs.Set(true, elem.loc, newvertex);
        if(reg->Contains(hs)){
          funnel_front.pop_back();
          mp1 = elem;
        }else break;
    }

        double l1 = newvertex.Distance(mp1.loc) + mp1.dist;
        /////////////////////////////////////////////
        list<MyPoint>::iterator iter = funnel_back.begin();
        for(;iter != funnel_back.end();iter++){
          Point p = iter->loc;
          HalfSegment hs;
          hs.Set(true, p, newvertex);
          if(reg->Contains(hs))break;
        }
        double l2 = newvertex.Distance(iter->loc) + iter->dist;
        ////////////////////////////////////////////

//        cout<<"l1 "<<l1<<" l2 "<<l2<<endl;
        if(l1 < l2 || AlmostEqual(l1, l2)){
            if(funnel_front.empty() == false)
                funnel_front.pop_front();
            while(funnel_front.empty() == false){
              MyPoint elem = funnel_front.front();
              path.push_back(elem.loc);
              funnel_front.pop_front();
            }
            if(mp1.loc.Distance(mp2.loc) > delta_dist)
                path.push_back(mp1.loc);
        }
        else{
            MyPoint top = funnel_back.front();
            if(funnel_back.empty() == false){
                funnel_back.pop_front();
            }

            HalfSegment hs;
            hs.Set(true, top.loc, newvertex);
            if(reg->Contains(hs)) return;

            while(funnel_back.empty() == false){
              MyPoint elem = funnel_back.front();
              path.push_back(elem.loc);
              /////////////////////////////
              HalfSegment hs;
              hs.Set(true, elem.loc, newvertex);
              if(reg->Contains(hs))break;
              //////////////////////////////
              funnel_back.pop_front();
            }
        }
  }

}

/*
geometrical shortest path in a polygon
Apply the Funnel Algorithm, front point---->point

*/
void CompTriangle::GeoShortestPath(Point* start, Point* end)
{
    cout<<"GeoShortestPath point to point"<<endl;
    if(start->Inside(*reg) == false || end->Inside(*reg) == false){
      cout<<"points are not inside the polygon"<<endl;
      return;
    }
    if(AlmostEqual(start->GetX(), end->GetX()) &&
       AlmostEqual(start->GetY(), end->GetY())){
        cout<<"two points are equal"<<endl;
        return;
    }

    if(PolygonConvex()){ //convex, just use euclidean distance
      cout<<"a convex polygon"<<endl;
      int edgeno = 0;
      path = new Line(0);
      path->StartBulkLoad();
      HalfSegment hs;
      hs.Set(true, *start, *end);
      hs.attr.edgeno = edgeno++;
      *path += hs;
      hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
      *path += hs;
      path->EndBulkLoad();
      sleeve.push_back(*reg);
      return;
    }

    Triangulation();//get a set of triangles

    ///////////////  find the channel /////////////////////////////

    GetChannel(start, end);
    /////////////////get a sequence of diagonals////////////////////////
    vector<HalfSegment> diagonals;
    const double delta_dist = 0.00001;
//    cout<<"channel size "<<sleeve.size()<<endl;
    for(unsigned int i = 0;i < sleeve.size() - 1;i++){
        vector<HalfSegment> cur_triangle;
        for(int j = 0;j < sleeve[i].Size();j++){
          HalfSegment hs;
          sleeve[i].Get(j, hs);
            if(!hs.IsLeftDomPoint())continue;
          cur_triangle.push_back(hs);
        }

        vector<HalfSegment> next_triangle;
        for(int j = 0;j < sleeve[i + 1].Size();j++){
          HalfSegment hs;
          sleeve[i + 1].Get(j, hs);
            if(!hs.IsLeftDomPoint())continue;
          next_triangle.push_back(hs);
        }
        for(unsigned int k1 = 0;k1 < cur_triangle.size();k1++){
            Point p1 = cur_triangle[k1].GetLeftPoint();
            Point p2 = cur_triangle[k1].GetRightPoint();

            unsigned int k2 = 0;
            for(;k2 < next_triangle.size();k2++){
              Point p3 = next_triangle[k2].GetLeftPoint();
              Point p4 = next_triangle[k2].GetRightPoint();

              if(p1.Distance(p3) < delta_dist && p2.Distance(p4) < delta_dist){
                HalfSegment hs;
                hs.Set(true,p1, p2);
                diagonals.push_back(hs);
                break;
              }
              if(p1.Distance(p4) < delta_dist && p2.Distance(p3) < delta_dist){
                HalfSegment hs;
                hs.Set(true,p1, p2);
                diagonals.push_back(hs);
                break;
              }
            }
            if(k2 != next_triangle.size())break;
        }
    }
//    cout<<"diagnoals size "<<diagonals.size()<<endl;

    list<MyPoint> funnel_front;
    list<MyPoint> funnel_back;
    Point apex = *start;
    vector<Point> shortest_path;

    shortest_path.push_back(apex);
    funnel_front.push_back(MyPoint(apex, 0.0));
    funnel_back.push_back(MyPoint(apex, 0.0));

    bool funnel_front_flag = true;

    for(unsigned int i = 0;i < diagonals.size();i++){
      Point lp = diagonals[i].GetLeftPoint();
      Point rp = diagonals[i].GetRightPoint();
      if(i == 0){
        funnel_front.push_back(MyPoint(lp, lp.Distance(apex)));
        funnel_back.push_back(MyPoint(rp, rp.Distance(apex)));
        continue;
      }
      Point last_lp = diagonals[i - 1].GetLeftPoint();
      Point last_rp = diagonals[i - 1].GetRightPoint();
      Point newvertex;
      if(lp.Distance(last_lp) < delta_dist ||
         lp.Distance(last_rp) < delta_dist){
          newvertex = rp;

//          cout<<"newvertex rp" <<newvertex<<endl;
          MyPoint front = funnel_front.back();
          MyPoint back = funnel_back.back();


          if(front.loc.Distance(lp) < delta_dist){
            ConstructConvexChannel1(funnel_front, funnel_back,
                                 newvertex, shortest_path, true);
            funnel_front_flag = true;
          }else if(back.loc.Distance(lp) < delta_dist){
            ConstructConvexChannel1(funnel_front, funnel_back,
                                 newvertex, shortest_path, false);
            funnel_front_flag = false;
          }else assert(false);
      }
      else if(rp.Distance(last_lp) < delta_dist ||
              rp.Distance(last_rp) < delta_dist){
              newvertex = lp;

//          cout<<"newvertex lp" <<newvertex<<endl;
          MyPoint front = funnel_front.back();
          MyPoint back = funnel_back.back();


          if(front.loc.Distance(rp) < delta_dist){
            ConstructConvexChannel1(funnel_front, funnel_back,
                                 newvertex, shortest_path, true);
            funnel_front_flag = true;
          }else if(back.loc.Distance(rp) < delta_dist){
            ConstructConvexChannel1(funnel_front, funnel_back,
                                 newvertex, shortest_path, false);
            funnel_front_flag = false;
          }else assert(false);
      }
      else assert(false);
    }

/*    for(unsigned int i = 0;i < shortest_path.size();i++)
            cout<<"point in path "<<shortest_path[i]<<endl;*/

    ////////////////// last point //////////////////////////////
    ConstructConvexChannel2(funnel_front, funnel_back,
                                 *end, shortest_path, funnel_front_flag);
    shortest_path.push_back(*end);
    ////////////////////////////////////////////////////////////
//    cout<<"shortest path segments size "<<shortest_path.size()<<endl;
    path = new Line(0);
    path->StartBulkLoad();
    int edgeno = 0;
    for(unsigned int i = 0;i < shortest_path.size() - 1;i++){
//      cout<<"point1 "<<shortest_path[i]<<endl;
//      cout<<"point2 "<<shortest_path[i + 1]<<endl;
      HalfSegment hs;
      Point p1 = shortest_path[i];
      Point p2 = shortest_path[i + 1];
      hs.Set(true, p1, p2);
      hs.attr.edgeno = edgeno++;
      *path += hs;
      hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
      *path += hs;
    }
    path->EndBulkLoad();
}

/*
retrieve the channel/sleeve from a point to a line segment

*/
void CompTriangle::GetChannel(Point* start, HalfSegment* end,
                             vector<Region>& temp_sleeve)
{
////////// find the start triangle /////////////////////////
  int index1 = -1;
  int index2 = -1;
  for(unsigned int i = 0;i < triangles.size();i++){
      if(start->Inside(triangles[i])){
        index1 = i;
        break;
      }
  }
  ////////// find the end triangle /////////////////////////
  for(unsigned int i = 0;i < triangles.size();i++){
      if(triangles[i].Contains(*end)){
        index2 = i;
        break;
      }
  }
  assert(index1 != -1 && index2 != -1);
//  cout<<"index1 "<<index1<<" index2 "<<index2<<endl;

      vector<bool> triangle_flag;
      for(unsigned int i = 0;i < triangles.size();i++)
        triangle_flag.push_back(false);

      triangle_flag[index1] = true;

      ////////////////shortest path algorithm///////////////////////
      priority_queue<SPath_elem> path_queue;
      vector<SPath_elem> expand_path;

      path_queue.push(SPath_elem(-1, 0, index1, 1));
      expand_path.push_back(SPath_elem(-1,0, index1, 1));
      bool find = false;
      SPath_elem dest;//////////destination
      while(path_queue.empty() == false){
        SPath_elem top = path_queue.top();
        path_queue.pop();
    //    top.Print();
        if(top.tri_index == index2){
          cout<<"find the path"<<endl;
          find = true;
          dest = top;
          break;
        }
      ////////find its adjacecy element, and push them into queue and path//////
        vector<int> adj_list;
        FindAdj(top.tri_index, triangle_flag, adj_list);
    //    cout<<"adjcency_list size "<<adj_list.size()<<endl;
    //    cout<<"expand_path_size "<<expand_path.size()<<endl;
        int pos_expand_path = top.cur_index;
        for(unsigned int i = 0;i < adj_list.size();i++){
          int expand_path_size = expand_path.size();
          path_queue.push(SPath_elem(pos_expand_path, expand_path_size,
                                adj_list[i], top.weight+1));
          expand_path.push_back(SPath_elem(pos_expand_path, expand_path_size,
                            adj_list[i], top.weight+1));
        }
      }
  ///////////////construct the path/////////////////////////////
      if(find){
        vector<int> path_record;
        while(dest.prev_index != -1){
            path_record.push_back(dest.tri_index);
            dest = expand_path[dest.prev_index];
        }
        path_record.push_back(dest.tri_index);
        for(int i = path_record.size() - 1;i >= 0;i--)
          temp_sleeve.push_back(triangles[path_record[i]]);

      }

}

/*
select the point on segment to compute the shortest path.
it goes through the funnel to check the vertex stored and try to find
sucn an segment which is contained by the polygon and it consists of
the vertex and the closest point to the vertex in the input segement.

*/
void CompTriangle::SelectPointOnSeg(list<MyPoint> funnel_front,
                        list<MyPoint> funnel_back, HalfSegment* end,
                        Point& vertex, Point& cp)

{
  SpacePartition* sp = new SpacePartition();
  const double delta_dist = 0.00001;
  MyPoint elem_front = funnel_front.back();
  MyPoint elem_back = funnel_back.back();
  if(elem_front.loc.Distance(vertex) < delta_dist){
    sp->GetClosestPoint(*end, elem_front.loc, cp);
    funnel_front.pop_back();
    while(funnel_front.empty() == false){
        MyPoint elem = funnel_front.back();
        Point p;
        sp->GetClosestPoint(*end, elem.loc, p);
        HalfSegment hs;
        hs.Set(true, elem.loc, p);
        if(reg->Contains(hs) == false)break;
        cp = p;
        funnel_front.pop_back();
    }
  }else if(elem_back.loc.Distance(vertex) < delta_dist){
    sp->GetClosestPoint(*end, elem_back.loc, cp);
    funnel_back.pop_back();
    while(funnel_back.empty() == false){
        MyPoint elem = funnel_back.back();
        Point p;
        sp->GetClosestPoint(*end, elem.loc, p);
        HalfSegment hs;
        hs.Set(true, elem.loc, p);
        if(reg->Contains(hs) == false)break;
        cp = p;
        funnel_back.pop_back();
    }
  }else assert(false);

  delete sp;
}

/*
compute the shortest path from start point to the end segment

*/
void CompTriangle::PtoSegSPath(Point* start, HalfSegment* end,
                              vector<Region>& temp_sleeve, Line* res_line)
{

  /////////////////get the channel////////////////////////////
  vector<HalfSegment> diagonals;
  const double delta_dist = 0.00001;

    for(unsigned int i = 0;i < temp_sleeve.size() - 1;i++){
        vector<HalfSegment> cur_triangle;
        for(int j = 0;j < temp_sleeve[i].Size();j++){
          HalfSegment hs;
          temp_sleeve[i].Get(j, hs);
            if(!hs.IsLeftDomPoint())continue;
          cur_triangle.push_back(hs);
        }

        vector<HalfSegment> next_triangle;
        for(int j = 0;j < temp_sleeve[i + 1].Size();j++){
          HalfSegment hs;
          temp_sleeve[i + 1].Get(j, hs);
            if(!hs.IsLeftDomPoint())continue;
          next_triangle.push_back(hs);
        }
        for(unsigned int k1 = 0;k1 < cur_triangle.size();k1++){
            Point p1 = cur_triangle[k1].GetLeftPoint();
            Point p2 = cur_triangle[k1].GetRightPoint();

            unsigned int k2 = 0;
            for(;k2 < next_triangle.size();k2++){
              Point p3 = next_triangle[k2].GetLeftPoint();
              Point p4 = next_triangle[k2].GetRightPoint();

              if(p1.Distance(p3) < delta_dist && p2.Distance(p4) < delta_dist){
                HalfSegment hs;
                hs.Set(true,p1, p2);
                diagonals.push_back(hs);
                break;
              }
              if(p1.Distance(p4) < delta_dist && p2.Distance(p3) < delta_dist){
                HalfSegment hs;
                hs.Set(true,p1, p2);
                diagonals.push_back(hs);
                break;
              }
            }
            if(k2 != next_triangle.size())break;
        }
    }
//    cout<<"diagnoals size "<<diagonals.size()<<endl;

 ///////////////////////////////////////////////////////////////

    list<MyPoint> funnel_front;
    list<MyPoint> funnel_back;
    Point apex = *start;
    vector<Point> shortest_path;

    shortest_path.push_back(apex);
    funnel_front.push_back(MyPoint(apex, 0.0));
    funnel_back.push_back(MyPoint(apex, 0.0));

    bool funnel_front_flag = true;
    Point newvertex;
    Point newvertex_pair;
    for(unsigned int i = 0;i < diagonals.size();i++){
      Point lp = diagonals[i].GetLeftPoint();
      Point rp = diagonals[i].GetRightPoint();
      if(i == 0){
        funnel_front.push_back(MyPoint(lp, lp.Distance(apex)));
        funnel_back.push_back(MyPoint(rp, rp.Distance(apex)));
        continue;
      }
      Point last_lp = diagonals[i - 1].GetLeftPoint();
      Point last_rp = diagonals[i - 1].GetRightPoint();

      if(lp.Distance(last_lp) < delta_dist ||
         lp.Distance(last_rp) < delta_dist){
          newvertex = rp;
          newvertex_pair = lp;

//          cout<<"newvertex rp" <<newvertex<<endl;
          MyPoint front = funnel_front.back();
          MyPoint back = funnel_back.back();

          if(front.loc.Distance(lp) < delta_dist){
            ConstructConvexChannel1(funnel_front, funnel_back,
                                 newvertex, shortest_path, true);
            funnel_front_flag = true;
          }else if(back.loc.Distance(lp) < delta_dist){
            ConstructConvexChannel1(funnel_front, funnel_back,
                                 newvertex, shortest_path, false);
            funnel_front_flag = false;
          }else assert(false);
      }
      else if(rp.Distance(last_lp) < delta_dist ||
              rp.Distance(last_rp) < delta_dist){
              newvertex = lp;
              newvertex_pair = rp;

//          cout<<"newvertex lp" <<newvertex<<endl;
          MyPoint front = funnel_front.back();
          MyPoint back = funnel_back.back();


          if(front.loc.Distance(rp) < delta_dist){
            ConstructConvexChannel1(funnel_front, funnel_back,
                                 newvertex, shortest_path, true);
            funnel_front_flag = true;
          }else if(back.loc.Distance(rp) < delta_dist){
            ConstructConvexChannel1(funnel_front, funnel_back,
                                 newvertex, shortest_path, false);
            funnel_front_flag = false;
          }else assert(false);
      }
      else assert(false);
    }
  //////////////////////////////////////////////////////////////
//  cout<<"end halfsegment "<<*end<<endl;
  vector<Point> shortest_path_another;
  for(unsigned int i = 0;i < shortest_path.size();i++)
      shortest_path_another.push_back(shortest_path[i]);

//  cout<<"newvertex "<<newvertex<<" cp1 "<<cp1<<endl;
  ////////////////////////////////////////////
  list<MyPoint> copy_funnel_front;
  list<MyPoint> copy_funnel_back;
  list<MyPoint>::iterator iter;
  for(iter = funnel_front.begin(); iter != funnel_front.end();iter++)
    copy_funnel_front.push_back(*iter);
  for(iter = funnel_back.begin(); iter != funnel_back.end();iter++)
    copy_funnel_back.push_back(*iter);

  Point cp1, cp2;

  //////////////////////////////////////////////////////////////////

  SelectPointOnSeg(funnel_front, funnel_back, end, newvertex, cp1);

  if(newvertex.Distance(cp1) > delta_dist){
    ConstructConvexChannel2(funnel_front, funnel_back,
                                 cp1, shortest_path, funnel_front_flag);
    shortest_path.push_back(cp1);
  }else{
    MyPoint top_front = funnel_front.back();
    MyPoint top_back = funnel_back.back();
    if(top_front.loc.Distance(cp1) < delta_dist){
      while(funnel_front.size() > 1){
        MyPoint elem = funnel_front.back();
        funnel_front.pop_back();
        shortest_path.push_back(elem.loc);
      }
    }else if(top_back.loc.Distance(cp1) < delta_dist){
      while(funnel_back.size() > 1){
        MyPoint elem = funnel_back.back();
        funnel_back.pop_back();
        shortest_path.push_back(elem.loc);
      }
    }else assert(false);
  }
  ///////////////////////////////////////////////////////////////////////
//  cout<<"path1 size "<<shortest_path.size()<<endl;

  SelectPointOnSeg(funnel_front, funnel_back, end, newvertex_pair, cp2);

//  cout<<"newvertex_pair "<<newvertex_pair<<" cp2 "<<cp2<<endl;
  if(newvertex_pair.Distance(cp2) > delta_dist){
    ConstructConvexChannel2(copy_funnel_front, copy_funnel_back,
                                 cp2, shortest_path_another,
                                 !funnel_front_flag);
    shortest_path_another.push_back(cp2);
  }else{
      MyPoint top_front = copy_funnel_front.back();
      MyPoint top_back = copy_funnel_back.back();
      if(top_front.loc.Distance(cp2) < delta_dist){
        while(copy_funnel_front.size() > 1){
          MyPoint elem = copy_funnel_front.back();
          copy_funnel_front.pop_back();
          shortest_path_another.push_back(elem.loc);
        }
      }else if(top_back.loc.Distance(cp2) < delta_dist){
          while(copy_funnel_back.size() > 1){
            MyPoint elem = copy_funnel_back.back();
            copy_funnel_back.pop_back();
            shortest_path_another.push_back(elem.loc);
          }
      }else assert(false);
  }

//  cout<<"path2 size "<<shortest_path_another.size()<<endl;

  //////////////////////////////////////////////////////////////
  Line* l1 = new Line(0);

  l1->StartBulkLoad();


  int edgeno1 = 0;
  for(unsigned int i = 0;i < shortest_path.size() - 1;i++){
      HalfSegment hs;
      Point p1 = shortest_path[i];
      Point p2 = shortest_path[i + 1];
      hs.Set(true, p1, p2);
      hs.attr.edgeno = edgeno1++;
      *l1 += hs;
      hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
      *l1 += hs;
  }
  l1->EndBulkLoad();

  Line* l2 = new Line(0);
  l2->StartBulkLoad();
  int edgeno2 = 0;
  for(unsigned int i = 0;i < shortest_path_another.size() - 1;i++){
      HalfSegment hs;
      Point p1 = shortest_path_another[i];
      Point p2 = shortest_path_another[i + 1];
      hs.Set(true, p1, p2);
      hs.attr.edgeno = edgeno2++;
      *l2 += hs;
      hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
      *l2 += hs;
  }
  l2->EndBulkLoad();
  if(l1->Length() < l2->Length())
    *res_line = *l1;
  else
    *res_line = *l2;

  delete l1;
  delete l2;
}

/*
geometrical shortest path in a polygon
Apply the Funnel Algorithm, find the shortest path from a point to a line

*/
void CompTriangle::GeoShortestPath(Point* start, Line* end)
{
  cout<<"GeoShortestPath point to sline"<<endl;

  if(start->Inside(*reg) == false || end->Intersects(*reg) == false){
      cout<<"point is not inside the polygon"<<endl;
      cout<<"or line does not intersect the region"<<endl;
      return;
  }
  if(PolygonConvex()){ //convex, just use euclidean distance
      cout<<"a convex polygon"<<endl;
      double dist = numeric_limits<float>::max();
      SpacePartition* sp = new SpacePartition();
      Point end_point;
      for(int i = 0;i < end->Size();i++){
        HalfSegment hs;
        end->Get(i, hs);
        if(!hs.IsLeftDomPoint()) continue;
        Point cp;
        sp->GetClosestPoint(hs, *start, cp);
        assert(cp.IsDefined());
        double d = start->Distance(cp);
        if(d < dist){
          end_point = cp;
          dist = d;
        }
      }
      int edgeno = 0;
      path = new Line(0);
      path->StartBulkLoad();
      HalfSegment hs;
      hs.Set(true, *start, end_point);
      hs.attr.edgeno = edgeno++;
      *path += hs;
      hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
      *path += hs;
      path->EndBulkLoad();
      sleeve.push_back(*reg);
      delete sp;
      return;
  }
  ////////////////concave polgyon////////////////////////////////////////
  Line* reg_boundary = new Line(0);
  reg->Boundary(reg_boundary);
  if(end->Inside(*reg_boundary) == false){
    cout<<"the end line should be covered by the border of the polygon"<<endl;
    delete reg_boundary;
    assert(false);
  }
  ///////////////////////////////////////////////////////////////////////
  Triangulation();//get a set of triangles
  vector<Line*> temp_sp; //store all possible shortest path
  double shortest_path_len = numeric_limits<float>::max();
  path = new Line(0);
  for(int i = 0;i < end->Size();i++){
    HalfSegment hs;
    end->Get(i, hs);
    if(!hs.IsLeftDomPoint()) continue;
//    cout<<"i "<<i<<"hs "<<hs<<endl;
    Line* l = new Line(0);
    temp_sp.push_back(l);

    vector<Region> temp_sleeve;
    GetChannel(start, &hs, temp_sleeve);
    PtoSegSPath(start, &hs, temp_sleeve, l);

    if(l->Length() < shortest_path_len){
      shortest_path_len = l->Length();
      sleeve.clear();
      for(unsigned int j = 0;j < temp_sleeve.size();j++)
        sleeve.push_back(temp_sleeve[j]);
      *path = *l;
    }
  }

  for(unsigned int i = 0;i < temp_sp.size();i++)
    delete temp_sp[i];
}
