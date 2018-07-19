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

//paragraph [1] Title: [{\Large \bf \begin{center}] [\end{center}}]
//characters [1] Type: [] []
//characters [2] Type: [] []
//[ae] [\"{a}]
//[oe] [\"{o}]
//[ue] [\"{u}]
//[ss] [{\ss}]
//[Ae] [\"{A}]
//[Oe] [\"{O}]
//[Ue] [\"{U}]
//[x] [$\times $]
//[->] [$\rightarrow $]
//[toc] [\tableofcontents]

[1] Implementation of operators.

[toc]

1 Operator implementation using dmap value mapping.

Implementation of the secondo operators drelfilter, drelproject, 
drelprojectextend, drellsortby, drellgroupby, drellsort, drellrdup, 
drelrename, drelhead and drelextend.
This operators use the same value mapping witch calls the dmap operator of 
the Distributed2Algebra.

*/

#include "NestedList.h"
#include "ListUtils.h"
#include "QueryProcessor.h"
#include "StandardTypes.h"
#include "SecParser.h"

#include "Algebras/Stream/Stream.h"
#include "Algebras/Relation-C++/OperatorFilter.h"
#include "Algebras/Relation-C++/OperatorProject.h"

#include "DRelHelpers.h"
#include "DRel.h"

extern NestedList* nl;
extern QueryProcessor* qp;

ListExpr RenameTypeMap( ListExpr args );

namespace distributed2 {
    ListExpr dmapTM( ListExpr args );

    template<class A>
    int dmapVMT( Word* args, Word& result, int message,
        Word& local, Supplier s );
}

namespace extrelationalg {
    ListExpr ExtendTypeMap( ListExpr args );
    ListExpr ExtProjectExtendTypeMap( ListExpr args );
    ListExpr SortByTypeMap( ListExpr args );
    ListExpr GroupByTypeMap( ListExpr args );
}

using namespace distributed2;

namespace drel {

/*
1.1 Type Mappings for all operators using dmapVM

1.1.1 Type Mapping ~drelfilterTM~

Expect a drel or dfrel with a function to filter the tuples. Type mapping 
for the drelfilter operator.

*/
    ListExpr drelfilterTM( ListExpr args ) {

        std::string err = "d[f]rel(X) x fun expected";

        if( !nl->HasLength( args, 2 ) ) {
            return listutils::typeError( err +
                ": two arguments are expected" );
        }

        if( !DRelHelpers::isListOfTwoElemLists( args ) ) {
            return listutils::typeError( "internal Error" );
        }

        ListExpr drelType, relType, distType, drelValue, darrayType;
        if( !DRelHelpers::isDRelDescr( nl->First( args ), drelType, relType, 
            distType, drelValue, darrayType ) ) {
            return listutils::typeError( err +
                ": first argument is not a d[f]rel" );
        }

        ListExpr fun, map;
        if( !DRelHelpers::replaceDRELFUNARG( nl->Second( args ), 
            "STREAMELEM", fun, map ) ) {
            return listutils::typeError( err +
                ": error in the function format" );
        }

        ListExpr result = OperatorFilter::FilterTypeMap(
            nl->TwoElemList(
                nl->TwoElemList(
                    nl->TwoElemList(
                        listutils::basicSymbol<Stream<Tuple> >( ),
                        nl->Second( relType ) ),
                    nl->TwoElemList(
                        nl->SymbolAtom( "feed" ),
                        drelValue ) ), // only dummy for the filter TM
                nl->TwoElemList( map, fun ) ) );

        // filter TM ok?
        if( !listutils::isTupleStream( result ) ) {
            return result;
        }

        if( DFRel::checkType( drelType ) ) {
            relType = nl->TwoElemList(
                listutils::basicSymbol<frel>( ),
                nl->Second( relType ) );
        }

        // create function type to call dmapTM
        ListExpr funList = nl->TwoElemList(
            nl->ThreeElemList(
                nl->SymbolAtom( "map" ),
                relType,
                result ),
            nl->ThreeElemList(
                nl->SymbolAtom( "fun" ),
                nl->TwoElemList(
                    nl->SymbolAtom( "dmapelem1" ),
                    nl->SymbolAtom( "ARRAYFUNARG1" ) ),
                    nl->ThreeElemList(
                        nl->SymbolAtom( "filter" ),
                        nl->TwoElemList(
                            nl->SymbolAtom( "feed" ),
                            nl->SymbolAtom( "dmapelem1" ) ),
                        fun ) ) );

        ListExpr dmapResult = dmapTM(
            nl->ThreeElemList(
                nl->TwoElemList( darrayType, drelValue ),
                nl->TwoElemList( 
                    listutils::basicSymbol<CcString>( ), 
                    nl->StringAtom( "" ) ),
                funList ) );

        if( !nl->HasLength( dmapResult, 3 ) ) {
            return dmapResult;
        }

        ListExpr newDRelType;
        if( DArray::checkType( nl->Third( dmapResult ) ) ) {
            newDRelType = listutils::basicSymbol<DRel>( );
        } else if( DFArray::checkType( nl->Third( dmapResult ) ) ) {
            newDRelType = listutils::basicSymbol<DFRel>( );
        } else {
            return dmapResult;
        }

        ListExpr newRes = nl->ThreeElemList(
            newDRelType,
            nl->Second( nl->Third( dmapResult ) ),
            nl->Third( drelType ) );  // disttype

        ListExpr append = nl->Second( dmapResult );

        return nl->ThreeElemList(
            nl->SymbolAtom( Symbols::APPEND( ) ),
            append,
            newRes );
    }

/*
1.1.2 Type Mapping ~drelprojectTM~

Expect a drel or dfrel an attribute list. Type mapping for the drelproject 
operator.

*/
    ListExpr drelprojectTM( ListExpr args ) {

        std::string err = "d[f]rel(X) x attrlist expected";

        if( !nl->HasLength( args, 2 ) ) {
            return listutils::typeError( err +
                ": two arguments are expected" );
        }

        if( !DRelHelpers::isListOfTwoElemLists( args ) ) {
            return listutils::typeError( "internal Error" );
        }

        ListExpr drelType, relType, distType, drelValue, darrayType;
        if( !DRelHelpers::isDRelDescr( nl->First( args ), drelType, relType, 
            distType, drelValue, darrayType ) ) {
            return listutils::typeError( err +
                ": first argument is not a d[f]rel" );
        }

        ListExpr attrlist = nl->First( nl->Second( args ) );

        ListExpr result = OperatorProject::ProjectTypeMap(
            nl->TwoElemList(
                nl->TwoElemList(
                    listutils::basicSymbol<Stream<Tuple> >( ),
                    nl->Second( relType ) ),
                attrlist ) );

        // project TM ok?
        if( !nl->HasLength( result, 3 ) ) {
            return result;
        }
        if( !listutils::isTupleStream( nl->Third( result ) ) ) {
            return result;
        }

        // distribution by an attribute?
        if( nl->HasMinLength( distType, 2 ) ) {

            int newPos = nl->IntValue( nl->Second( distType ) ) + 1;

            if( DistTypeHash::computeNewAttrPos( 
                nl->Second( nl->Second( result ) ), newPos ) ) {

                switch( nl->ListLength( distType ) ) {
                case 3:
                    distType = nl->ThreeElemList(
                        nl->First( distType ),
                        nl->IntAtom( newPos ),
                        nl->Third( distType ) );
                    break;
                case 4:
                    distType = nl->FourElemList(
                        nl->First( distType ),
                        nl->IntAtom( newPos ),
                        nl->Third( distType ),
                        nl->Fourth( distType ) );
                    break;
                default:
                    distType = nl->TwoElemList(
                        nl->First( distType ),
                        nl->IntAtom( newPos ) );
                }

                drelType = nl->ThreeElemList(
                    nl->First( drelType ),
                    distType,
                    nl->Third( drelType ) );
            }
            else {
                return listutils::typeError( err +
                    ": it is not allowed to project without the "
                    "distribution attribute" );
            }

        }

        if( DFRel::checkType( drelType ) ) {
            relType = nl->TwoElemList(
                listutils::basicSymbol<frel>( ),
                nl->Second( relType ) );
        }

        // create function type to call dmapTM
        ListExpr funList = nl->TwoElemList(
            nl->ThreeElemList(
                nl->SymbolAtom( "map" ),
                relType,
                nl->Third( result ) ),
            nl->ThreeElemList(
                nl->SymbolAtom( "fun" ),
                nl->TwoElemList(
                    nl->SymbolAtom( "dmapelem1" ),
                    nl->SymbolAtom( "ARRAYFUNARG1" ) ),
                nl->ThreeElemList(
                    nl->SymbolAtom( "project" ),
                    nl->TwoElemList(
                        nl->SymbolAtom( "feed" ),
                        nl->SymbolAtom( "dmapelem1" ) ),
                    attrlist ) ) );

        ListExpr dmapResult = dmapTM(
            nl->ThreeElemList(
                nl->TwoElemList( darrayType, drelValue ),
                nl->TwoElemList(
                    listutils::basicSymbol<CcString>( ),
                    nl->StringAtom( "" ) ),
                funList ) );

        if( !nl->HasLength( dmapResult, 3 ) ) {
            return dmapResult;
        }

        ListExpr newDRelType;
        if( DArray::checkType( nl->Third( dmapResult ) ) ) {
            cout << "result darray" << endl;
            newDRelType = listutils::basicSymbol<DRel>( );
        } else if( DFArray::checkType( nl->Third( dmapResult ) ) ) {
            newDRelType = listutils::basicSymbol<DFRel>( );
            cout << "result dfarray" << endl;
        } else {
            return dmapResult;
        }

        ListExpr newRes = nl->ThreeElemList(
            newDRelType,
            nl->Second( nl->Third( dmapResult ) ),
            distType );

        ListExpr append = nl->Second( dmapResult );

        return nl->ThreeElemList(
            nl->SymbolAtom( Symbols::APPEND( ) ),
            append,
            newRes );
    }

/*
1.1.3 Type Mapping ~drelprojectextendTM~

Expect a drel or dfrel with an attribute list and a function list to extend
the tuples. This is a combination of the operators project and extend.

*/
    ListExpr drelprojectextendTM( ListExpr args ) {

        std::string err = "d[f]rel(X) x attrlist x funlist expected";

        if( !nl->HasLength( args, 3 ) ) {
            return listutils::typeError( err +
                ": three arguments are expected" );
        }

        if( !DRelHelpers::isListOfTwoElemLists( args ) ) {
            return listutils::typeError( "internal Error" );
        }

        ListExpr drelType, relType, distType, drelValue, darrayType;
        if( !DRelHelpers::isDRelDescr( nl->First( args ), drelType, relType, 
            distType, drelValue, darrayType ) ) {
            return listutils::typeError( err +
                ": first argument is not a d[f]rel" );
        }

        ListExpr attrlist = nl->First( nl->Second( args ) );

        if( !nl->HasMinLength( nl->First( nl->Third( args ) ), 1 )
         || !nl->HasMinLength( nl->Second( nl->Third( args ) ), 1 ) ) {
            return listutils::typeError( err +
                ": error in the function format" );
        }

        ListExpr map = nl->First( nl->Third( args ) );

        ListExpr tempfun;
        ListExpr fun = nl->TheEmptyList( );
        ListExpr temp = nl->Second( nl->Third( args ) );
        while( !nl->IsEmpty( temp ) ) {

            if( !nl->HasLength( nl->First( temp ), 2 ) ) {
                return listutils::typeError( "internal Error" );
            }

            if( !DRelHelpers::replaceDRELFUNARG( 
                nl->Second( nl->First( temp ) ), 
                "TUPLE", tempfun ) ) {
                return listutils::typeError( err +
                    ": error in the function format" );
            }

            if( nl->IsEmpty( fun ) ) {
                fun = nl->OneElemList( nl->TwoElemList( 
                    nl->First( nl->First( temp ) ), tempfun ) );
            } else {
                fun = listutils::concat( fun, 
                    nl->OneElemList( nl->TwoElemList(
                    nl->First( nl->First( temp ) ), tempfun ) ) );
            }

            temp = nl->Rest( temp );
        }

        ListExpr result = extrelationalg::ExtProjectExtendTypeMap(
            nl->ThreeElemList(
                nl->TwoElemList(
                    listutils::basicSymbol<Stream<Tuple> >( ),
                    nl->Second( relType ) ), 
                attrlist,
                map ) );

        // filter TM ok?
        if( !nl->HasLength( result, 3 ) ) {
            return result;
        }
        if( !listutils::isTupleStream( nl->Third( result ) ) ) {
            return result;
        }

        // distribution by an attribute?
        if( nl->HasMinLength( distType, 2 ) ) {

            int newPos = nl->IntValue( nl->Second( distType ) ) + 1;

            if( DistTypeHash::computeNewAttrPos(
                nl->Second( nl->Second( result ) ), newPos ) ) {

                switch( nl->ListLength( distType ) ) {
                case 3:
                    distType = nl->ThreeElemList(
                        nl->First( distType ),
                        nl->IntAtom( newPos ),
                        nl->Third( distType ) );
                    break;
                case 4:
                    distType = nl->FourElemList(
                        nl->First( distType ),
                        nl->IntAtom( newPos ),
                        nl->Third( distType ),
                        nl->Fourth( distType ) );
                    break;
                default:
                    distType = nl->TwoElemList(
                        nl->First( distType ),
                        nl->IntAtom( newPos ) );
                }

                drelType = nl->ThreeElemList(
                    nl->First( drelType ),
                    distType,
                    nl->Third( drelType ) );
            } else {
                return listutils::typeError( err +
                    ": it is not allowed to project without the "
                    "distribution attribute" );
            }

        }

        if( DFRel::checkType( drelType ) ) {
            relType = nl->TwoElemList(
                listutils::basicSymbol<frel>( ),
                nl->Second( relType ) );
        }

        // create function type to call dmapTM
        ListExpr funList = nl->TwoElemList(
            nl->ThreeElemList(
                nl->SymbolAtom( "map" ),
                relType,
                nl->Third( result ) ),
            nl->ThreeElemList(
                nl->SymbolAtom( "fun" ),
                nl->TwoElemList(
                    nl->SymbolAtom( "dmapelem1" ),
                    nl->SymbolAtom( "ARRAYFUNARG1" ) ),
                nl->FourElemList(
                    nl->SymbolAtom( "projectextend" ),
                    nl->TwoElemList(
                        nl->SymbolAtom( "feed" ),
                        nl->SymbolAtom( "dmapelem1" ) ),
                    attrlist,
                    fun ) ) );

        ListExpr dmapResult = dmapTM(
            nl->ThreeElemList(
                nl->TwoElemList( darrayType, drelValue ),
                nl->TwoElemList(
                    listutils::basicSymbol<CcString>( ),
                    nl->StringAtom( "" ) ),
                funList ) );

        if( !nl->HasLength( dmapResult, 3 ) ) {
            return dmapResult;
        }

        ListExpr newDRelType;
        if( DArray::checkType( nl->Third( dmapResult ) ) ) {
            newDRelType = listutils::basicSymbol<DRel>( );
        } else if( DFArray::checkType( nl->Third( dmapResult ) ) ) {
            newDRelType = listutils::basicSymbol<DFRel>( );
        } else {
            return dmapResult;
        }

        ListExpr newRes = nl->ThreeElemList(
            newDRelType,
            nl->Second( nl->Third( dmapResult ) ),
            distType );

        ListExpr append = nl->Second( dmapResult );

        return nl->ThreeElemList(
            nl->SymbolAtom( Symbols::APPEND( ) ),
            append,
            newRes );
    }

/*
1.1.4 Type Mapping ~drelextendTM~

Expect a drel or dfrel with a function list to extend the tuples. This is 
a combination of the operator extend.

*/
    ListExpr drelextendTM( ListExpr args ) {

        std::string err = "d[f]rel(X) x funlist expected";

        if( !nl->HasLength( args, 2 ) ) {
            return listutils::typeError( err +
                ": three arguments are expected" );
        }

        if( !DRelHelpers::isListOfTwoElemLists( args ) ) {
            return listutils::typeError( "internal Error" );
        }

        ListExpr drelType, relType, distType, drelValue, darrayType;
        if( !DRelHelpers::isDRelDescr( nl->First( args ), drelType, relType, 
            distType, drelValue, darrayType ) ) {
            return listutils::typeError( err +
                ": first argument is not a d[f]rel" );
        }

        if( !nl->HasMinLength( nl->First( nl->Second( args ) ), 1 )
            || !nl->HasMinLength( nl->Second( nl->Second( args ) ), 1 ) ) {
            return listutils::typeError( err +
                ": error in the function format" );
        }

        ListExpr map = nl->First( nl->Second( args ) );

        ListExpr tempfun;
        ListExpr fun = nl->TheEmptyList( );
        ListExpr temp = nl->Second( nl->Second( args ) );
        while( !nl->IsEmpty( temp ) ) {

            if( !nl->HasLength( nl->First( temp ), 2 ) ) {
                return listutils::typeError( "internal Error" );
            }

            if( !DRelHelpers::replaceDRELFUNARG( 
                nl->Second( nl->First( temp ) ), 
                "TUPLE", tempfun ) ) {
                return listutils::typeError( err +
                    ": error in the function format" );
            }

            if( nl->IsEmpty( fun ) ) {
                fun = nl->OneElemList( nl->TwoElemList(
                    nl->First( nl->First( temp ) ), tempfun ) );
            } else {
                fun = listutils::concat( fun, 
                    nl->OneElemList( nl->TwoElemList(
                    nl->First( nl->First( temp ) ), tempfun ) ) );
            }

            temp = nl->Rest( temp );
        }

        ListExpr result = extrelationalg::ExtendTypeMap(
            nl->TwoElemList(
                nl->TwoElemList(
                    listutils::basicSymbol<Stream<Tuple> >( ),
                    nl->Second( relType ) ),
                map ) );

        // filter TM ok?
        if( !nl->HasLength( result, 2 ) ) {
            return result;
        }
        if( !listutils::isTupleStream( result ) ) {
            return result;
        }

        if( DFRel::checkType( drelType ) ) {
            relType = nl->TwoElemList(
                listutils::basicSymbol<frel>( ),
                nl->Second( relType ) );
        }

        // create function type to call dmapTM
        ListExpr funList = nl->TwoElemList(
            nl->ThreeElemList(
                nl->SymbolAtom( "map" ),
                relType,
                result ),
            nl->ThreeElemList(
                nl->SymbolAtom( "fun" ),
                nl->TwoElemList(
                    nl->SymbolAtom( "dmapelem1" ),
                    nl->SymbolAtom( "ARRAYFUNARG1" ) ),
                nl->ThreeElemList(
                    nl->SymbolAtom( "extend" ),
                    nl->TwoElemList(
                        nl->SymbolAtom( "feed" ),
                        nl->SymbolAtom( "dmapelem1" ) ),
                    fun ) ) );

        ListExpr dmapResult = dmapTM(
            nl->ThreeElemList(
                nl->TwoElemList( darrayType, drelValue ),
                nl->TwoElemList(
                    listutils::basicSymbol<CcString>( ),
                    nl->StringAtom( "" ) ),
                funList ) );

        if( !nl->HasLength( dmapResult, 3 ) ) {
            return dmapResult;
        }

        ListExpr newDRelType;
        if( DArray::checkType( nl->Third( dmapResult ) ) ) {
            newDRelType = listutils::basicSymbol<DRel>( );
        } else if( DFArray::checkType( nl->Third( dmapResult ) ) ) {
            newDRelType = listutils::basicSymbol<DFRel>( );
        } else {
            return dmapResult;
        }

        ListExpr newRes = nl->ThreeElemList(
            newDRelType,
            nl->Second( nl->Third( dmapResult ) ),
            distType );

        ListExpr append = nl->Second( dmapResult );

        return nl->ThreeElemList(
            nl->SymbolAtom( Symbols::APPEND( ) ),
            append,
            newRes );
    }

/*
1.1.5 Type Mapping ~drelheadTM~

Expect a drel or dfrel and an int value. Type mapping for the drelhead
operator.

*/
    ListExpr drelheadTM( ListExpr args ) {

        std::string err = "d[f]rel(X) x int expected";

        if( !nl->HasLength( args, 2 ) ) {
            return listutils::typeError( err +
                ": two arguments are expected" );
        }

        if( !DRelHelpers::isListOfTwoElemLists( args ) ) {
            return listutils::typeError( "internal Error" );
        }

        ListExpr drelType, relType, distType, drelValue, darrayType;
        if( !DRelHelpers::isDRelDescr( nl->First( args ), drelType, relType, 
            distType, drelValue, darrayType ) ) {
            return listutils::typeError( err +
                ": first argument is not a d[f]rel" );
        }

        ListExpr secondType = nl->First( nl->Second( args ) );
        ListExpr secondValue = nl->Second( nl->Second( args ) );

        if( !CcInt::checkType( secondType ) ) {
            return listutils::typeError( err +
                ": second argument is not an integer" );
        }

        if( DFRel::checkType( drelType ) ) {
            relType = nl->TwoElemList(
                listutils::basicSymbol<frel>( ),
                nl->Second( relType ) );
        }

        // create function type to call dmapTM
        ListExpr funList = nl->TwoElemList(
            nl->ThreeElemList(
                nl->SymbolAtom( "map" ),
                relType,
                nl->TwoElemList(
                    listutils::basicSymbol<Stream<Tuple>>( ),
                    nl->Second( relType ) ) ),
            nl->ThreeElemList(
                nl->SymbolAtom( "fun" ),
                nl->TwoElemList(
                    nl->SymbolAtom( "dmapelem1" ),
                    nl->SymbolAtom( "ARRAYFUNARG1" ) ),
                nl->ThreeElemList(
                    nl->SymbolAtom( "head" ),
                    nl->TwoElemList(
                        nl->SymbolAtom( "feed" ),
                        nl->SymbolAtom( "dmapelem1" ) ),
                    secondValue ) ) );

        ListExpr dmapResult = dmapTM(
            nl->ThreeElemList(
                nl->TwoElemList( darrayType, drelValue ),
                nl->TwoElemList(
                    listutils::basicSymbol<CcString>( ),
                    nl->StringAtom( "" ) ),
                funList ) );

        if( !nl->HasLength( dmapResult, 3 ) ) {
            return dmapResult;
        }

        ListExpr newDRelType;
        if( DArray::checkType( nl->Third( dmapResult ) ) ) {
            newDRelType = listutils::basicSymbol<DRel>( );
        } else if( DFArray::checkType( nl->Third( dmapResult ) ) ) {
            newDRelType = listutils::basicSymbol<DFRel>( );
        } else {
            return dmapResult;
        }

        ListExpr newRes = nl->ThreeElemList(
            newDRelType,
            nl->Second( nl->Third( dmapResult ) ),
            distType );

        ListExpr append = nl->Second( dmapResult );

        return nl->ThreeElemList(
            nl->SymbolAtom( Symbols::APPEND( ) ),
            append,
            newRes );
    }

/*
1.1.6 Type Mapping ~drelrenameTM~

Expect a drel or dfrel and a symbol. Type mapping for the drelrename 
operator.

*/
    ListExpr drelrenameTM( ListExpr args ) {

        std::string err = "d[f]rel(X) x ar expected";

        if( !nl->HasLength( args, 2 ) ) {
            return listutils::typeError( err +
                ": two arguments are expected" );
        }

        if( !DRelHelpers::isListOfTwoElemLists( args ) ) {
            return listutils::typeError( "internal Error" );
        }

        ListExpr drelType, relType, distType, drelValue, darrayType;
        if( !DRelHelpers::isDRelDescr( nl->First( args ), drelType, relType,
            distType, drelValue, darrayType ) ) {
            return listutils::typeError( err +
                ": first argument is not a d[f]rel" );
        }

        ListExpr secondType = nl->First( nl->Second( args ) );
        ListExpr secondValue = nl->Second( nl->Second( args ) );

        ListExpr result = RenameTypeMap(
            nl->TwoElemList(
                nl->TwoElemList(
                    listutils::basicSymbol<Stream<Tuple> >( ),
                    nl->Second( relType ) ),
                secondType ) );

        // project TM ok?
        if( !nl->HasLength( result, 2 ) ) {
            return result;
        }
        if( !listutils::isTupleStream( result ) ) {
            return result;
        }

        if( DFRel::checkType( drelType ) ) {
            relType = nl->TwoElemList(
                listutils::basicSymbol<frel>( ),
                nl->Second( relType ) );
        }

        // create function type to call dmapTM
        ListExpr funList = nl->TwoElemList(
            nl->ThreeElemList(
                nl->SymbolAtom( "map" ),
                relType,
                result ),
            nl->ThreeElemList(
                nl->SymbolAtom( "fun" ),
                nl->TwoElemList(
                    nl->SymbolAtom( "dmapelem1" ),
                    nl->SymbolAtom( "ARRAYFUNARG1" ) ),
                nl->ThreeElemList(
                    nl->SymbolAtom( "rename" ),
                    nl->TwoElemList(
                        nl->SymbolAtom( "feed" ),
                        nl->SymbolAtom( "dmapelem1" ) ),
                    secondValue ) ) );

        ListExpr dmapResult = dmapTM(
            nl->ThreeElemList(
                nl->TwoElemList( darrayType, drelValue ),
                nl->TwoElemList(
                    listutils::basicSymbol<CcString>( ),
                    nl->StringAtom( "" ) ),
                funList ) );

        if( !nl->HasLength( dmapResult, 3 ) ) {
            return dmapResult;
        }

        ListExpr newDRelType;
        if( DArray::checkType( nl->Third( dmapResult ) ) ) {
            newDRelType = listutils::basicSymbol<DRel>( );
        } else if( DFArray::checkType( nl->Third( dmapResult ) ) ) {
            newDRelType = listutils::basicSymbol<DFRel>( );
        } else {
            return dmapResult;
        }

        ListExpr newRes = nl->ThreeElemList(
            newDRelType,
            nl->Second( nl->Third( dmapResult ) ),
            distType );

        ListExpr append = nl->Second( dmapResult );

        return nl->ThreeElemList(
            nl->SymbolAtom( Symbols::APPEND( ) ),
            append,
            newRes );
    }

/*
1.1.7 Type Mapping ~drellrdupTM~

Expect a drel or dfrel and a symbol. Type mapping for the drellrdup
operator.

*/
    ListExpr drellrdupTM( ListExpr args ) {

        std::string err = "d[f]rel(X) expected";

        if( !nl->HasLength( args, 1 ) ) {
            return listutils::typeError( err +
                ": two arguments are expected" );
        }

        if( !DRelHelpers::isListOfTwoElemLists( args ) ) {
            return listutils::typeError( "internal Error" );
        }

        ListExpr drelType, relType, distType, drelValue, darrayType;
        if( !DRelHelpers::isDRelDescr( nl->First( args ), drelType, relType,
            distType, drelValue, darrayType ) ) {
            return listutils::typeError( err +
                ": first argument is not a d[f]rel" );
        }

        if( DFRel::checkType( drelType ) ) {
            relType = nl->TwoElemList(
                listutils::basicSymbol<frel>( ),
                nl->Second( relType ) );
        }

        // create function type to call dmapTM
        ListExpr funList = nl->TwoElemList(
            nl->ThreeElemList(
                nl->SymbolAtom( "map" ),
                relType,
                nl->TwoElemList(
                    listutils::basicSymbol<Stream<Tuple>>( ),
                    nl->Second( relType ) ) ),
            nl->ThreeElemList(
                nl->SymbolAtom( "fun" ),
                nl->TwoElemList(
                    nl->SymbolAtom( "dmapelem1" ),
                    nl->SymbolAtom( "ARRAYFUNARG1" ) ),
                nl->TwoElemList(
                    nl->SymbolAtom( "rdup" ),
                    nl->TwoElemList(
                        nl->SymbolAtom( "feed" ),
                        nl->SymbolAtom( "dmapelem1" ) ) ) ) );

        ListExpr dmapResult = dmapTM(
            nl->ThreeElemList(
                nl->TwoElemList( darrayType, drelValue ),
                nl->TwoElemList(
                    listutils::basicSymbol<CcString>( ),
                    nl->StringAtom( "" ) ),
                funList ) );

        if( !nl->HasLength( dmapResult, 3 ) ) {
            return dmapResult;
        }

        ListExpr newDRelType;
        if( DArray::checkType( nl->Third( dmapResult ) ) ) {
            newDRelType = listutils::basicSymbol<DRel>( );
        } else if( DFArray::checkType( nl->Third( dmapResult ) ) ) {
            newDRelType = listutils::basicSymbol<DFRel>( );
        } else {
            return dmapResult;
        }

        ListExpr newRes = nl->ThreeElemList(
            newDRelType,
            nl->Second( nl->Third( dmapResult ) ),
            distType );

        ListExpr append = nl->Second( dmapResult );

        return nl->ThreeElemList(
            nl->SymbolAtom( Symbols::APPEND( ) ),
            append,
            newRes );
    }

/*
1.1.8 Type Mapping ~drellsortTM~

Expect a drel or dfrel and a symbol. Type mapping for the drellsort
operator.

*/
    ListExpr drellsortTM( ListExpr args ) {

        std::string err = "d[f]rel(X) expected";

        if( !nl->HasLength( args, 1 ) ) {
            return listutils::typeError( err +
                ": two arguments are expected" );
        }

        if( !DRelHelpers::isListOfTwoElemLists( args ) ) {
            return listutils::typeError( "internal Error" );
        }

        ListExpr drelType, relType, distType, drelValue, darrayType;
        if( !DRelHelpers::isDRelDescr( nl->First( args ), drelType, relType,
            distType, drelValue, darrayType ) ) {
            return listutils::typeError( err +
                ": first argument is not a d[f]rel" );
        }

        if( DFRel::checkType( drelType ) ) {
            relType = nl->TwoElemList(
                listutils::basicSymbol<frel>( ),
                nl->Second( relType ) );
        }

        // create function type to call dmapTM
        ListExpr funList = nl->TwoElemList(
            nl->ThreeElemList(
                nl->SymbolAtom( "map" ),
                relType,
                nl->TwoElemList(
                    listutils::basicSymbol<Stream<Tuple>>( ),
                    nl->Second( relType ) ) ),
            nl->ThreeElemList(
                nl->SymbolAtom( "fun" ),
                nl->TwoElemList(
                    nl->SymbolAtom( "dmapelem1" ),
                    nl->SymbolAtom( "ARRAYFUNARG1" ) ),
                nl->TwoElemList(
                    nl->SymbolAtom( "sort" ),
                    nl->TwoElemList(
                        nl->SymbolAtom( "feed" ),
                        nl->SymbolAtom( "dmapelem1" ) ) ) ) );

        ListExpr dmapResult = dmapTM(
            nl->ThreeElemList(
                nl->TwoElemList( darrayType, drelValue ),
                nl->TwoElemList(
                    listutils::basicSymbol<CcString>( ),
                    nl->StringAtom( "" ) ),
                funList ) );

        if( !nl->HasLength( dmapResult, 3 ) ) {
            return dmapResult;
        }

        ListExpr newDRelType;
        if( DArray::checkType( nl->Third( dmapResult ) ) ) {
            newDRelType = listutils::basicSymbol<DRel>( );
        } else if( DFArray::checkType( nl->Third( dmapResult ) ) ) {
            newDRelType = listutils::basicSymbol<DFRel>( );
        } else {
            return dmapResult;
        }

        ListExpr newRes = nl->ThreeElemList(
            newDRelType,
            nl->Second( nl->Third( dmapResult ) ),
            distType );

        ListExpr append = nl->Second( dmapResult );

        return nl->ThreeElemList(
            nl->SymbolAtom( Symbols::APPEND( ) ),
            append,
            newRes );
    }

/*
1.1.1 Type Mapping ~drellgroupbyTM~

Expect a drel or dfrel, an attribute list to group the tuple and a function list.
Type mapping for the drellgroup operator.

*/
    ListExpr drellgroupbyTM( ListExpr args ) {

        std::string err = "d[f]rel(X) x attrlist x funlist expected";

        if( !nl->HasLength( args, 3 ) ) {
            return listutils::typeError( err +
                ": three arguments are expected" );
        }

        if( !DRelHelpers::isListOfTwoElemLists( args ) ) {
            return listutils::typeError( "internal Error" );
        }

        ListExpr drelType, relType, distType, drelValue, darrayType;
        if( !DRelHelpers::isDRelDescr( nl->First( args ), drelType, relType,
            distType, drelValue, darrayType ) ) {
            return listutils::typeError( err +
                ": first argument is not a d[f]rel" );
        }

        ListExpr attrlist = nl->First( nl->Second( args ) );

        if( !nl->HasMinLength( nl->First( nl->Third( args ) ), 1 )
            || !nl->HasMinLength( nl->Second( nl->Third( args ) ), 1 ) ) {
            return listutils::typeError( err +
                ": error in the function format" );
        }

        ListExpr map = nl->First( nl->Third( args ) );

        ListExpr tempfun;
        ListExpr fun = nl->TheEmptyList( );
        ListExpr temp = nl->Second( nl->Third( args ) );
        while( !nl->IsEmpty( temp ) ) {

            if( !nl->HasLength( nl->First( temp ), 2 ) ) {
                return listutils::typeError( "internal Error" );
            }

            if( !DRelHelpers::replaceDRELFUNARG(
                nl->Second( nl->First( temp ) ),
                "GROUP", tempfun ) ) {
                return listutils::typeError( err +
                    ": error in the function format" );
            }

            if( nl->IsEmpty( fun ) ) {
                fun = nl->OneElemList( nl->TwoElemList(
                    nl->First( nl->First( temp ) ), tempfun ) );
            } else {
                fun = listutils::concat( fun,
                    nl->OneElemList( nl->TwoElemList(
                        nl->First( nl->First( temp ) ), tempfun ) ) );
            }

            temp = nl->Rest( temp );
        }

        ListExpr result = extrelationalg::GroupByTypeMap(
            nl->ThreeElemList(
                nl->TwoElemList(
                    listutils::basicSymbol<Stream<Tuple> >( ),
                    nl->Second( relType ) ),
                attrlist,
                map ) );

        // groupby TM ok?
        if( !nl->HasLength( result, 3 ) ) {
            return result;
        }
        if( !listutils::isTupleStream( nl->Third( result ) ) ) {
            return result;
        }

        // distribution by an attribute?
        if( nl->HasMinLength( distType, 2 ) ) {

            int newPos = nl->IntValue( nl->Second( distType ) ) + 1;

            if( nl->HasMinLength( nl->Second( result ), 2 )
             && DistTypeHash::computeNewAttrPos(
                 nl->Rest( nl->Second( result ) ), newPos ) ) {

                switch( nl->ListLength( distType ) ) {
                case 3:
                    distType = nl->ThreeElemList(
                        nl->First( distType ),
                        nl->IntAtom( newPos ),
                        nl->Third( distType ) );
                    break;
                case 4:
                    distType = nl->FourElemList(
                        nl->First( distType ),
                        nl->IntAtom( newPos ),
                        nl->Third( distType ),
                        nl->Fourth( distType ) );
                    break;
                default:
                    distType = nl->TwoElemList(
                        nl->First( distType ),
                        nl->IntAtom( newPos ) );
                }

                drelType = nl->ThreeElemList(
                    nl->First( drelType ),
                    distType,
                    nl->Third( drelType ) );

            } else {
                return listutils::typeError( err +
                    ": it is not allowed to create a group without the "
                    "distribution attribute" );
            }

        }

        if( DFRel::checkType( drelType ) ) {
            relType = nl->TwoElemList(
                listutils::basicSymbol<frel>( ),
                nl->Second( relType ) );
        }

        // create function type to call dmapTM
        ListExpr funList = nl->TwoElemList(
            nl->ThreeElemList(
                nl->SymbolAtom( "map" ),
                relType,
                nl->Third( result ) ),
            nl->ThreeElemList(
                nl->SymbolAtom( "fun" ),
                nl->TwoElemList(
                    nl->SymbolAtom( "dmapelem1" ),
                    nl->SymbolAtom( "ARRAYFUNARG1" ) ),
                nl->FourElemList(
                    nl->SymbolAtom( "groupby" ),
                    nl->TwoElemList(
                        nl->SymbolAtom( "feed" ),
                        nl->SymbolAtom( "dmapelem1" ) ),
                    attrlist,
                    fun ) ) );

        ListExpr dmapResult = dmapTM(
            nl->ThreeElemList(
                nl->TwoElemList( darrayType, drelValue ),
                nl->TwoElemList(
                    listutils::basicSymbol<CcString>( ),
                    nl->StringAtom( "" ) ),
                funList ) );

        if( !nl->HasLength( dmapResult, 3 ) ) {
            return dmapResult;
        }

        ListExpr newDRelType;
        if( DArray::checkType( nl->Third( dmapResult ) ) ) {
            newDRelType = listutils::basicSymbol<DRel>( );
        } else if( DFArray::checkType( nl->Third( dmapResult ) ) ) {
            newDRelType = listutils::basicSymbol<DFRel>( );
        } else {
            return dmapResult;
        }

        ListExpr newRes = nl->ThreeElemList(
            newDRelType,
            nl->Second( nl->Third( dmapResult ) ),
            distType );

        ListExpr append = nl->Second( dmapResult );

        return nl->ThreeElemList(
            nl->SymbolAtom( Symbols::APPEND( ) ),
            append,
            newRes );
    }

/*
1.1.2 Type Mapping ~drellsortbyTM~

Expect a drel or dfrel and an attribute list. Type mapping for the 
drellsortby operator.

*/
    ListExpr drellsortbyTM( ListExpr args ) {

        std::string err = "d[f]rel(X) x attrlist expected";

        if( !nl->HasLength( args, 2 ) ) {
            return listutils::typeError( err +
                ": two arguments are expected" );
        }

        if( !DRelHelpers::isListOfTwoElemLists( args ) ) {
            return listutils::typeError( "internal Error" );
        }

        ListExpr drelType, relType, distType, drelValue, darrayType;
        if( !DRelHelpers::isDRelDescr( nl->First( args ), drelType, relType,
            distType, drelValue, darrayType ) ) {
            return listutils::typeError( err +
                ": first argument is not a d[f]rel" );
        }

        ListExpr attrlist = nl->First( nl->Second( args ) );

        ListExpr result = extrelationalg::SortByTypeMap(
            nl->TwoElemList(
                nl->TwoElemList(
                    listutils::basicSymbol<Stream<Tuple> >( ),
                    nl->Second( relType ) ),
                attrlist ) );

        // sortby TM ok?
        if( !nl->HasLength( result, 3 ) ) {
            return result;
        }
        if( !listutils::isTupleStream( nl->Third( result ) ) ) {
            return result;
        }

        if( DFRel::checkType( drelType ) ) {
            relType = nl->TwoElemList(
                listutils::basicSymbol<frel>( ),
                nl->Second( relType ) );
        }

        // create function type to call dmapTM
        ListExpr funList = nl->TwoElemList(
            nl->ThreeElemList(
                nl->SymbolAtom( "map" ),
                relType,
                nl->Third( result ) ),
            nl->ThreeElemList(
                nl->SymbolAtom( "fun" ),
                nl->TwoElemList(
                    nl->SymbolAtom( "dmapelem1" ),
                    nl->SymbolAtom( "ARRAYFUNARG1" ) ),
                nl->ThreeElemList(
                    nl->SymbolAtom( "sortby" ),
                    nl->TwoElemList(
                        nl->SymbolAtom( "feed" ),
                        nl->SymbolAtom( "dmapelem1" ) ),
                    attrlist ) ) );

        ListExpr dmapResult = dmapTM(
            nl->ThreeElemList(
                nl->TwoElemList( darrayType, drelValue ),
                nl->TwoElemList(
                    listutils::basicSymbol<CcString>( ),
                    nl->StringAtom( "" ) ),
                funList ) );

        if( !nl->HasLength( dmapResult, 3 ) ) {
            return dmapResult;
        }

        ListExpr newDRelType;
        if( DArray::checkType( nl->Third( dmapResult ) ) ) {
            cout << "result darray" << endl;
            newDRelType = listutils::basicSymbol<DRel>( );
        } else if( DFArray::checkType( nl->Third( dmapResult ) ) ) {
            newDRelType = listutils::basicSymbol<DFRel>( );
            cout << "result dfarray" << endl;
        } else {
            return dmapResult;
        }

        ListExpr newRes = nl->ThreeElemList(
            newDRelType,
            nl->Second( nl->Third( dmapResult ) ),
            distType );

        ListExpr append = nl->Second( dmapResult );

        return nl->ThreeElemList(
            nl->SymbolAtom( Symbols::APPEND( ) ),
            append,
            newRes );
    }

/*
1.2 Value Mapping ~dreldmapVMT~

Uses a drel or dfrel and creates a new drel or dfrel. The drel or dfrel is 
created by calling the dmap value mapping of the Distributed2Algebra.

*/
    template<class R, class T, int parm>
    int dreldmapVMT( Word* args, Word& result, int message,
        Word& local, Supplier s ) {
        
        R* drel = ( R* )args[ 0 ].addr;

        ArgVector argVec = {
            drel,
            new CcString( "" ),
            new CcBool( false, false ), // dummy, ignored by dmapVMT
            args[ 1 + parm ].addr,
            args[ 2 + parm ].addr,
            args[ 3 + parm ].addr };

        dmapVMT<T>( argVec, result, message, local, s );

        R* resultDRel = ( R* )result.addr;
        if( !resultDRel->IsDefined( ) ) {
            return 0;
        }

        resultDRel->setDistType( drel->getDistType( )->copy( ) );

        return 0;
    }

/*
1.3 ValueMapping Array for dmap
    
Used by the operators with only a drel input.

*/
    ValueMapping dreldmapVM[ ] = {
        dreldmapVMT<DRel, DArray, 0>,
        dreldmapVMT<DRel, DArray, 1>,
        dreldmapVMT<DRel, DArray, 2>,
        dreldmapVMT<DFRel, DFArray, 0>,
        dreldmapVMT<DFRel, DFArray, 1>,
        dreldmapVMT<DFRel, DFArray, 2>
    };

/*
1.4 Selection function for dreldmap

Used to select the right position of the parameters. It is necessary, 
because the dmap-Operator ignores the second parameter. So so parameters 
must be moved to the right position for the dmap value mapping.

*/
    int dreldmapSelect( ListExpr args ) {

        int parm = nl->ListLength( args ) - 1;

        return DRel::checkType( nl->First( args ) ) ? 0 + parm : 3 + parm;
    }

/*
1.5 Specification for all operators using dmapVM

1.5.1 Specification of drelproject

Operator specification of the drelporject operator.

*/
    OperatorSpec drelprojectSpec(
        " drel(X) x attrlist "
        "-> drel(Y) ",
        " _ drelproject[attrlist]",
        "Passed only the listed attributes to a stream. It is not allowed to "
        "create a new d[f]rel without the partion attribute.",
        " query drel1 drelproject[PLZ, Ort] consume"
    );

/*
1.5.2 Specification of drelfilter

Operator specification of the drelfilter operator.

*/
    OperatorSpec drelfilterSpec(
        " drel(X) x fun "
        "-> drel(X) ",
        " _ drelfilter[fun]",
        "Only tuples, fulfilling a certain condition are passed to the new "
        "d[f]rel",
        " query drel1 drelfilter[.PLZ=99998]"
    );

/*
1.5.3 Specification of drelprojectextend

Operator specification of the drelprojectextend operator.

*/
    OperatorSpec drelprojectextendSpec(
        " drel(X) x attrlist x funlist "
        "-> drel(X) ",
        " _ drelprojectextend[list; funlist]",
        "First projects the drel and then extends the drel with additional "
        "attributes specified by the funlist",
        " query drel1 drelprojectextend[No; Mult5: .No * 5]"
    );

/*
1.5.4 Specification of drelextend

Operator specification of the drelextend operator.

*/
    OperatorSpec drelextendSpec(
        " drel(X) x funlist "
        "-> drel(X) ",
        " _ drelextend[list]",
        "Extends the drel with additional attributes specified by the funlist",
        " query drel1 drelextend[Mult5: .No * 5]"
    );

/*
1.5.5 Specification of drelhead

Operator specification of the drelhead operator.

*/
    OperatorSpec drelheadSpec(
        " drel(X) x list "
        "-> drel(X) ",
        " _ drelhead[int]",
        "Passed only the first n tuple of each array field ",
        " query drel1 drelhead[5]"
    );

/*
1.5.6 Specification of drelrename

Operator specification of the drelrename operator.

*/
    OperatorSpec drelrenameSpec(
        " drel(X) x ar "
        "-> drel(X) ",
        " _ drelrename[symbol]",
        "Renames all attribute names by adding"
        " them with the postfix passed as parameter. "
        "NOTE: parameter must be of symbol type.",
        " query drel1 drelhead[t1]"
    );

/*
1.5.7 Specification of drellrdup

Operator specification of the drellrdup operator.

*/
    OperatorSpec drellrdupSpec(
        " drel(X) "
        "-> drel(X) ",
        " _ drellrdup",
        "Removes duplicates in a d[f]rel. "
        "NOTE: Duplicates are only removed in a local array field "
        "and not in the global d[f]rel.",
        " query drel1 drellrdup"
    );

/*
1.5.8 Specification of drellsort

Operator specification of the drellsort operator.

*/
    OperatorSpec drellsortSpec(
        " drel(X) "
        "-> drel(X) ",
        " _ drellsort",
        "Sorts a d[f]rel. "
        "NOTE: The operator only sorts the local array fields "
        "and not in global d[f]rel.",
        " query drel1 drellsort"
    );
    
/*
1.5.9 Specification of drellgroupby

Operator specification of the drellgroupby operator.

*/
    OperatorSpec drellgroupbySpec(
        " drel(X) x attrlist x funlist "
        "-> drel(X) ",
        " _ drellgroupby[attrlist, funlist]",
        "Groups a d[f]rel according to attributes "
        "ai1, ..., aik and feeds the groups to other "
        "functions. The results of those functions are "
        "appended to the grouping attributes. The empty "
        "list is allowed for the grouping attributes (this "
        "results in a single group with all input tuples)."
        "NOTE: The operator only groups the local array fields "
        "and not in global d[f]rel.",
        " query drel1 drellgroupby[PLZ; Anz: group feed count]"
    );

/*
1.5.10 Specification of drellsortby

Operator specification of the drellsortby operator.

*/
    OperatorSpec drellsortbySpec(
        " drel(X) "
        "-> drel(X) ",
        " _ drellsortby[attrlist]",
        "Sorts a d[f]rel by a specific attribute list. "
        "NOTE: The operator only sorts the local array fields "
        "and not in global d[f]rel.",
        " query drel1 drellsortby[PLZ]"
    );

/*
1.6 Operator instance of operators using dmapVM

1.6.1 Operator instance of drelfilter operator

*/
    Operator drelfilterOp(
        "drelfilter",
        drelfilterSpec.getStr( ),
        4,
        dreldmapVM,
        dreldmapSelect,
        drelfilterTM
    );

/*
1.6.2 Operator instance of drelproject operator

*/
    Operator drelprojectOp(
        "drelproject",
        drelprojectSpec.getStr( ),
        4,
        dreldmapVM,
        dreldmapSelect,
        drelprojectTM
    );

/*
1.6.3 Operator instance of drelprojectextend operator

*/
    Operator drelprojectextendOp(
        "drelprojectextend",
        drelprojectextendSpec.getStr( ),
        4,
        dreldmapVM,
        dreldmapSelect,
        drelprojectextendTM
    );

/*
1.6.4 Operator instance of drelextend operator

*/
    Operator drelextendOp(
        "drelextend",
        drelextendSpec.getStr( ),
        4,
        dreldmapVM,
        dreldmapSelect,
        drelextendTM
    );

/*
1.6.5 Operator instance of drelhead operator

*/
    Operator drelheadOp(
        "drelhead",
        drelheadSpec.getStr( ),
        4,
        dreldmapVM,
        dreldmapSelect,
        drelheadTM
    );
    
/*
1.6.6 Operator instance of drelrename operator

*/
    Operator drelrenameOp(
        "drelrename",
        drelheadSpec.getStr( ),
        4,
        dreldmapVM,
        dreldmapSelect,
        drelrenameTM
    );
    
/*
1.6.7 Operator instance of drellrdup operator

*/
    Operator drellrdupOp(
        "drellrdup",
        drellrdupSpec.getStr( ),
        4,
        dreldmapVM,
        dreldmapSelect,
        drellrdupTM
    );
    
/*
1.6.8 Operator instance of drellsort operator

*/
    Operator drellsortOp(
        "drellsort",
        drellsortSpec.getStr( ),
        4,
        dreldmapVM,
        dreldmapSelect,
        drellsortTM
    );
    
/*
1.6.9 Operator instance of drellgroupby operator

*/
    Operator drellgroupbyOp(
        "drellgroupby",
        drellgroupbySpec.getStr( ),
        4,
        dreldmapVM,
        dreldmapSelect,
        drellgroupbyTM
    );

/*
1.6.10 Operator instance of drellsortby operator

*/
    Operator drellsortbyOp(
        "drellsortby",
        drellsortbySpec.getStr( ),
        4,
        dreldmapVM,
        dreldmapSelect,
        drellsortbyTM
    );

} // end of namespace drel