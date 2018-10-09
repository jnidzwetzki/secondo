/*
----
This file is part of SECONDO.

Copyright (C) 2015,
Faculty of Mathematics and Computer Science,
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


//[$][\$]


1 Implementation of the secondo operators drelsortmergejoin, drelitHashJoin, 
and drelitSpatialJoin

*/
//#define DRELDEBUG

#include "NestedList.h"
#include "ListUtils.h"
#include "QueryProcessor.h"
#include "StandardTypes.h"
#include "SecParser.h"

#include "Algebras/Stream/Stream.h"
#include "Algebras/FText/FTextAlgebra.h"
#include "Algebras/Distributed2/CommandLogger.h"
#include "Algebras/Distributed2/Distributed2Algebra.h"

#include "DRelHelpers.h"
#include "DRel.h"
#include "Partitioner.hpp"

extern NestedList* nl;
extern QueryProcessor* qp;

using namespace distributed2;

namespace distributed2 {

    extern Distributed2Algebra* algInstance;

    int dmapXVM(Word* args, Word& result, int message,
            Word& local, Supplier s );

    template<int x>
    ListExpr dmapXTMT(ListExpr args);
};

namespace extrelationalg {

    template<bool OptionalIntAllowed, int defaultValue>
    ListExpr JoinTypeMap (ListExpr args);
};

namespace drel {

/*
1.1 Type Mappings for all join operators

1.1.1 Type Mapping ~drelsortmergejoinTM~

*/
    ListExpr drelsortmergejoinTM( ListExpr args ) {

        cout << "drelsortmergejoinTM" << endl;
        cout << nl->ToString( args ) << endl;

        std::string err = "d[f]rel(X) x d[f]rel(X) x attr x attr expected";

        ListExpr drel1Type = nl->First( nl->First( args ) );
        ListExpr drel1Value = nl->Second( nl->First( args ) );
        ListExpr drel2Type = nl->First( nl->Second( args ) );
        ListExpr drel2Value = nl->Second( nl->Second( args ) );

        ListExpr attr1Name = nl->First( nl->Third( args ) );
        ListExpr attr2Name = nl->First( nl->Fourth( args ) );

        ListExpr darray1Type, darray2Type;

        if( DRel::checkType( drel1Type ) ) {
            darray1Type = nl->TwoElemList(
                listutils::basicSymbol<DArray>( ),
                nl->Second( drel1Type ) );
        }
        else if( DFRel::checkType( drel1Type ) ) {
            darray1Type = nl->TwoElemList(
                listutils::basicSymbol<DFArray>( ),
                nl->Second( drel1Type ) );
        }
        else {
            return listutils::typeError(
                err + ": first argument is not a d[f]rel" );
        }

        if( DRel::checkType( drel2Type ) ) {
            darray2Type = nl->TwoElemList(
                listutils::basicSymbol<DArray>( ),
                nl->Second( drel2Type ) );
        }
        else if( DFRel::checkType( drel2Type ) ) {
            darray2Type = nl->TwoElemList(
                listutils::basicSymbol<DFArray>( ),
                nl->Second( drel2Type ) );
        }
        else {
            return listutils::typeError(
                err + ": second argument is not a d[f]rel" );
        }

        if( !listutils::isRelDescription( nl->Second( drel1Type ) ) ||
            !listutils::isRelDescription( nl->Second( drel2Type ) ) ) {
            
            return listutils::typeError(
                err + ": one of the d[f]rel's does not contain a relation" );
        }

        ListExpr attr1List = 
            nl->Second( nl->Second( nl->Second( drel1Type ) ) );
        ListExpr attr2List = 
            nl->Second( nl->Second( nl->Second( drel2Type ) ) );

        // tpye map check for sortmergejoin
        ListExpr joinTMresult = extrelationalg::JoinTypeMap<false, 0>( 
            nl->FourElemList(
                nl->TwoElemList(
                    listutils::basicSymbol<Stream<Tuple>>( ),
                    nl->TwoElemList(
                        listutils::basicSymbol<Tuple>( ),
                        attr1List ) ),
                nl->TwoElemList(
                    listutils::basicSymbol<Stream<Tuple>>( ),
                    nl->TwoElemList(
                        listutils::basicSymbol<Tuple>( ),
                        attr2List ) ),
                attr1Name,
                attr2Name ) );
        
        if( !nl->HasLength( joinTMresult, 3 ) ) {
            return joinTMresult;
        }

        // type map check for dmap
        ListExpr fun = nl->TwoElemList(
            nl->FourElemList(
                nl->SymbolAtom( "map" ),
                nl->Second( drel1Type ), 
                nl->Second( drel2Type ), 
                nl->TwoElemList(
                    listutils::basicSymbol<Stream<Tuple>>( ),
                    nl->TwoElemList(
                        listutils::basicSymbol<Tuple>( ),
                    ConcatLists( attr1List, attr2List ) ) ) ),
            nl->FourElemList(
                nl->SymbolAtom( "fun" ),
                nl->TwoElemList(
                    nl->SymbolAtom( "elem1_1" ),
                    nl->SymbolAtom( "ARRAYFUNARG1" ) ),
                nl->TwoElemList(
                    nl->SymbolAtom( "elem2_2" ),
                    nl->SymbolAtom( "ARRAYFUNARG2" ) ),
                nl->ThreeElemList(
                    nl->SymbolAtom( "sortmergejoin" ),
                    nl->TwoElemList(
                        nl->SymbolAtom( "feed" ),
                        nl->SymbolAtom( "elem1_1" ) ),
                    nl->TwoElemList(
                        nl->SymbolAtom( "feed" ),
                        nl->SymbolAtom( "elem2_2" ) ) ) ) );

        cout << "fun " << endl;
        cout << nl->ToString( fun ) << endl;

        ListExpr dmapResult = dmapXTMT<2>(
            nl->FiveElemList(
                nl->TwoElemList( darray1Type, drel1Value ),
                nl->TwoElemList( darray2Type, drel2Value ),
                nl->TwoElemList( 
                    listutils::basicSymbol<CcString>( ),
                    nl->StringAtom( "" ) ),
                fun,
                nl->TwoElemList( 
                    listutils::basicSymbol<CcInt>( ),
                    nl->IntAtom( 1238 ) ) ) );

        cout << "dmapResult" << endl;
        cout << nl->ToString( dmapResult ) << endl;

        ListExpr drel1distType = nl->Third( drel1Type );
        ListExpr drel2distType = nl->Third( drel2Type );

        ListExpr attrType;
        int attrPos1 = listutils::findAttribute( attr1List, 
            nl->SymbolValue( attr1Name ), attrType );
        int attrPos2 = listutils::findAttribute( attr2List, 
            nl->SymbolValue( attr2Name ), attrType );

        // repartitioning of drel1 or drel2 required?
        bool drel1reparti = DistTypeBasic::repartiRequired( 
            drel1distType, attrPos1 - 1 );
        bool drel2reparti = DistTypeBasic::repartiRequired( 
            drel2distType, attrPos2 - 1 );

        if( drel1reparti ) {
            cout << "repartitioning of first drel required..." << endl;
        }

        if( drel2reparti ) {
            cout << "repartitioning of second drel required..." << endl;
        }

        // no repartitioning required, check keys
        if( !drel1reparti && !drel2reparti ) {

            // both drels partitioned by range or spartial
            if( nl->HasMinLength( drel1distType, 3 )
             && nl->HasMinLength( drel2distType, 3 ) ) {

                if( nl->IntValue( nl->Third( drel1distType ) ) 
                    != nl->IntValue( nl->Third( drel2distType ) ) ) {

                        cout << "key of the two drels do not match" << endl;
                        drel1reparti = true;
                        drel2reparti = true;
                }
            }
            else {

                //if one of the drels is partitioned by hash this drel 
                //is choosen for repartitioning
                if( nl->HasLength( drel1distType,2 ) ) {
                    drel2reparti = false;
                }

                if( nl->HasLength( drel2distType,2 ) ) {
                    drel1reparti = false;
                }
            }

        }

        int count = -1;
        if( drel1reparti && drel2reparti ) {

            int count1 = DRelHelpers::countDRel( nl->ToString( drel1Value ) );
            int count2 = DRelHelpers::countDRel( nl->ToString( drel2Value ) );

            if( count1 <= count2 ) {
                count = count1;
                drel2reparti = false;
                cout << "repartition of the first drel will be done" << endl;
            }
            else {
                count = count2;
                drel1reparti = false;
                cout << "repartition of the second drel will be done" << endl;
                cout << "drel size" + std::to_string( count ) << endl;
            }
        }

        cout << "made a decision about repartitioning..." << endl;

        ListExpr resultType;
        if( drel1reparti ) {
            // drel1 repartitioning? use distribution type of drel2
            resultType = nl->ThreeElemList(
                listutils::basicSymbol<DFRel>( ),
                nl->Second( nl->Third( dmapResult ) ),
                drel2distType );
        }
        else {
            // drel2 repartitioning? use distribution type of drel1
            resultType = nl->ThreeElemList(
                listutils::basicSymbol<DFRel>( ),
                nl->Second( nl->Third( dmapResult ) ),
                drel1distType );
        }

        cout << "res type" << endl;
        cout << nl->ToString( resultType ) << endl;

        ListExpr append = ConcatLists( 
            nl->Second( dmapResult ), 
            nl->ThreeElemList( 
                nl->BoolAtom( drel1reparti ),
                nl->BoolAtom( drel2reparti ),
                nl->IntAtom( count ) ) );

        /*return nl->ThreeElemList( 
            nl->SymbolAtom( Symbols::APPEND( ) ),
            appendList,
            resultType );*/

        return listutils::typeError(
                err + ": test only" );
    }

/*
1.2 Value Mapping ~dreldmapVMT~

Uses a d[f]rel and creates a new drel. The d[f]rel is created by 
repartitioning the d[f]rel and execute a function on the d[f]rel.

*/
    template<class R, class T>
    int dreljoinVMT( Word* args, Word& result, int message,
        Word& local, Supplier s ) {

        //int parmNum = qp->GetNoSons( s );

        return 0;
    }

/*
1.3 ValueMapping Array for dmap
    
Used by the operators with only a drel input.

*/
    ValueMapping dreljoinVM[ ] = {
        dreljoinVMT<DRel, DArray>,
        dreljoinVMT<DFRel, DFArray>
    };

/*
1.4 Selection function for dreldmap

Used to select the right position of the parameters. It is necessary, 
because the dmap-Operator ignores the second parameter. So so parameters 
must be moved to the right position for the dmap value mapping.

*/
    int dreljoinSelect( ListExpr args ) {

        return 0;
    }

/*
1.5 Specification for all operators using dmapVM

1.5.7 Specification of drelsortmergejoin

Operator specification of the drelsortmergejoin operator.

*/
    OperatorSpec drelsortmergejoinSpec(
        " d[f]rel(X) x d[f]rel(X) x attr x attr "
        "-> d[f]rel(X) ",
        " _ _ drelsortmergejoin[_,_]",
        "Computes the equijoin of two d[f]rels using the new sort "
        "operator implementation. NOT JET WORKING!!",
        " query drel1 {p} drel2 {o} drelsortmergejoin[PLZ_p, PLZ_o]"
    );


/*
1.6 Operator instance of the join operators

1.6.7 Operator instance of drelsortmergejoin operator

*/
    Operator drelsortmergejoinOp(
        "drelsortmergejoin",
        drelsortmergejoinSpec.getStr( ),
        2,
        dreljoinVM,
        dreljoinSelect,
        drelsortmergejoinTM
    );

} // end of namespace drel