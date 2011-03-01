#This file is part of SECONDO.

#Copyright (C) 2004, University in Hagen, Department of Computer Science,
#Database Systems for New Applications.

#SECONDO is free software; you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation; either version 2 of the License, or
#(at your option) any later version.

#SECONDO is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.

#You should have received a copy of the GNU General Public License
#along with SECONDO; if not, write to the Free Software
#Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

operator checksline alias CHECKSLINE pattern op(_,_)
operator modifyboundary alias MODIFYBOUNDARY pattern op(_,_)
operator segment2region alias SEGMENT2REGION pattern op(_,_)
operator paveregion alias PAVEREGION pattern op(_,_,_,_,_,_,_)
operator junregion alias JUNREGION pattern op(_,_,_,_,_,_,_)
operator decomposeregion alias DECOMPOSEREGION pattern op(_)
operator fillpavement alias FILLPAVEMENT pattern op(_,_,_,_,_)
operator getpavenode1 alias GETPAVENODE1 pattern op(_,_,_,_,_)
operator getpaveedge1 alias GETPAVEEDGE1 pattern op(_,_,_,_,_,_)
operator getpavenode2 alias GETPAVENODE2 pattern op(_,_,_,_)
operator getpaveedge2 alias GETPAVEEDGE2 pattern op(_,_,_,_,_,_)
operator triangulation  alias TRIANGULATION pattern op(_)
operator convex alias CONVEX pattern op(_)
operator geospath alias GEOSPATH pattern op(_,_,_)
operator createdualgraph alias CREATEDUALGRAPH pattern op(_,_,_)
operator nodedualgraph alias NODEDUALGRAPH pattern op(_)
operator walk_sp alias WALK_SP pattern op(_,_,_,_,_)
operator test_walk_sp alias TEST_WALK_SP pattern op(_,_,_,_,_)
operator setpave_rid alias SETPAVE_RID pattern op(_,_,_)
operator pave_loc_togp alias PAVE_LOC_TOGP pattern op(_,_,_,_)
operator generate_wp1 alias GENERATE_WP1 pattern op(_,_)
operator generate_wp2 alias GENERATE_WP2 pattern op(_,_)
operator generate_wp3 alias GENERATE_WP3 pattern op(_,_)
operator zval alias ZVAL pattern op(_)
operator zcurve alias ZCURVE pattern op(_,_)
operator regvertex alias REGVERTEX pattern op(_)
operator triangulation_new  alias TRIANGULATION_NEW pattern op(_)
operator get_dg_edge alias GET_DG_EDGE pattern op(_,_)
operator smcdgte alias SMCDGTE pattern op(_,_)
operator getvnode alias GETVNODE pattern op(_,_,_,_,_,_)
operator getvgedge alias GETVGEDGE pattern op(_,_,_,_,_)
operator myinside alias MYINSIDE pattern _ infixop _
operator getadjnode_dg alias GETADJNODE_DG pattern op(_,_)
operator getadjnode_vg alias GETADJNODE_VG pattern op(_,_)
operator decomposetri alias DECOMPOSETRI pattern op(_)
operator createvgraph alias CREATEVGRAPH pattern op(_,_,_)
operator getcontour alias GETCONTOUR pattern op(_)
operator getpolygon alias GETPOLYGON pattern op(_,_)
operator getallpoints alias GETALLPOINTS pattern op(_)
operator rotationsweep alias ROTATIONSWEEP pattern op(_,_,_,_,_)
operator gethole alias GETHOLE pattern op(_)
operator getsections alias GETSECTIONS pattern op(_,_,_,_,_,_)
operator geninterestp1 alias GENINTERESTP1 pattern op(_,_,_,_,_,_,_)
operator geninterestp2 alias GENINTERESTP2 pattern op(_,_,_,_,_,_)
operator cellbox alias CELLBOX pattern op(_,_)
operator create_bus_route1 alias CREATE_BUS_ROUTE1 pattern op(_,_,_,_,_,_,_)
operator create_bus_route2 alias CREATE_BUS_ROUTE2 pattern op(_,_,_,_,_,_,_,_)
operator refine_bus_route alias REFINE_BUS_ROUTE pattern op(_,_,_,_,_,_,_,_)
operator bus_route_road alias BUS_ROUTE_ROAD pattern op(_,_,_)
operator create_bus_route3 alias CREATE_BUS_ROUTE3 pattern op(_,_,_,_,_)
operator create_bus_route4 alias CREATE_BUS_ROUTE3 pattern op(_,_,_,_,_,_,_,_)
operator create_bus_stop1 alias CREATE_BUS_STOP1 pattern op(_,_,_,_,_,_,_,_)
operator create_bus_stop2 alias CREATE_BUS_STOP2 pattern op(_,_,_,_,_)
operator create_bus_stop3 alias CREATE_BUS_STOP3 pattern op(_,_,_,_,_,_,_,_)
operator create_bus_stop4 alias CREATE_BUS_STOP5 pattern op(_,_,_,_,_,_,_,_)
operator create_bus_stop5 alias CREATE_BUS_STOP5 pattern op(_,_,_,_,_,_,_,_)
operator getbusstops alias GETBUSSTOPS pattern op(_,_,_)
operator getstopid alias GETSTOPID pattern op(_)
operator getbusroutes alias GETBUSROUTES pattern op(_,_,_)
operator brgeodata alias BRGEODATA pattern op(_)
operator bsgeodata alias BSGEODATA pattern op(_,_)
operator up_down alias UP_DOWN pattern op(_)
operator thebusnetwork alias THEBUSNETWORK pattern op(_,_,_)
operator bn_busstops alias BN_BUSSTOPS pattern op(_)
operator bn_busroutes alias BN_BUSROUTES pattern op(_)
operator mapbstopave alias MAPBSTOPAVE pattern op(_,_,_,_)
operator bs_neighbors1 alias BS_NEIGHBORS1 pattern op(_,_,_,_,_)
operator bs_neighbors2 alias BS_NEIGHBORS2 pattern op(_)
operator bs_neighbors3 alias BS_NEIGHBORS3 pattern op(_,_,_)
operator createbgraph alias CREATEBGRAPH pattern op(_,_,_,_,_)
operator getadjnode_bg alias GETADJNODE_BG pattern op(_,_)
operator get_route_density1 alias GET_ROUTE_DENSITY1 pattern op(_,_,_,_,_,_,_,_,_,_)
operator set_ts_nightbus alias SET_TS_NIGHTBUS pattern op(_,_,_,_,_,_)
operator set_ts_daybus alias SET_TS_DAYBUS pattern op(_,_,_,_,_,_)
operator set_br_speed alias SET_BR_SPEED pattern op(_,_,_,_,_,_,_,_)
operator create_bus_segment_speed alias CREATE_BUS_SEGMENT_SPEED pattern op(_,_,_,_,_,_,_,_,_,_,_)
operator create_night_bus_mo alias CREATE_NIGHT_BUS_MO pattern op(_,_,_)
operator create_daytime_bus_mo alias CREATE_DAYTIME_BUS_MO pattern op(_,_,_)
operator create_time_table1 alias CREATE_TIME_TABLE1 pattern op(_,_,_)
operator create_time_table1_new alias CREATE_TIME_TABLE1_NEW pattern op(_,_,_,_,_)
operator createUBTrains alias CREATEUBTRAINS pattern op(_,_,_,_,_)
operator create_train_stop alias CREATE_TRAIN_STOP pattern op(_,_,_,_)
operator create_time_table2 alias CREATE_TIME_TABLE2 pattern op(_,_,_)
operator create_time_table2_new alias CREATE_TIME_TABLE2_NEW pattern op(_,_,_)
operator splitubahn alias SPLITUBAHN pattern op(_,_,_)
operator trainstogenmo alias TRAINSTOGENMO pattern op(_,_,_)
operator thefloor alias THEFLOOR pattern op(_,_)
operator getheight alias GETHEIGHT pattern op(_)
operator getregion alias GETREGION pattern op(_)
operator thedoor alias THEDOOR pattern op(_,_,_,_,_,_)
operator oid_of_door alias TYPE_OF_DOOR pattern op(_)
operator type_of_door alias TYPE_OF_DOOR pattern op(_)
operator loc_of_door alias LOC_OF_DOOR pattern op(_,_)
operator state_of_door alias STATE_OF_DOOR pattern op(_)
operator get_floor alias GET_FLOOR pattern op(_,_)
operator add_height_groom alias ADD_HEIGHT_GROOM pattern op(_,_)
operator translate_groom alias TRANSLATE_GROOM pattern _ op [list]
operator createdoor3d alias CREATEDOOR3D pattern op(_)
operator createdoorbox alias CREATEDOORBOX pattern op(_)
operator createdoor1 alias CREATEDOOR1 pattern op(_,_,_,_,_,_)
operator createdoor2 alias CREATEDOOR2 pattern op(_)
operator createadjdoor1 alias CREATEADJDOOR1 pattern op(_,_,_,_,_,_,_)
operator createadjdoor2 alias CREATEADJDOOR2 pattern op(_,_)
operator path_in_region alias PATH_IN_REGION pattern op(_,_,_)
operator size alias SIZE pattern op(_)
operator bbox3d alias BBOX_D pattern op(_)
operator createigraph alias CREATEIGRAPH pattern op(_,_,_)
operator getadjnode_ig alias GETADJNODE_IG pattern op(_,_)
operator generate_ip1 alias GENERATE_IP1 pattern op(_,_)
operator generate_mo1 alias GENERATE_MO1 pattern op(_,_,_,_,_,_)
operator indoornavigation alias INDOORNAVIGATION pattern op(_,_,_,_,_,_)
operator instant2day alias INSTANT2DAY pattern op(_)
operator outputregion alias OUTPUTREGION pattern op(_)
operator maxrect alias MAXRECT pattern op(_)
operator remove_dirty alias REMOVE_DIRTY pattern op(_,_,_)
operator getrect2 alias GETRECT pattern op(_,_,_,_,_,_,_)
operator getrect1 alias GETRECT1 pattern op(_,_,_)
operator path_to_building alias PATH_TO_BUILDING pattern op(_,_,_)
operator ref_id alias REF_ID pattern op(_)
operator deftime alias DEFTIME pattern op(_)
operator no_components alias NO_COMPONENTS pattern op (_)
operator lowres alias LOWRES pattern op(_)
operator trajectory alias TRAJECTORY pattern op(_)
operator getmode alias GETMODE pattern op(_)
operator thespace alias THESPACE pattern op(_)
operator genmo_tm_list alias GENMO_TM_LIST pattern op(_)
