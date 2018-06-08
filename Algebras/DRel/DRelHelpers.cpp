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


1 ~DRelHelpers~

Class with usefull helper functions for the DRelAlgebra

*/
#include "DRelHelpers.h"

#include "Algebras/Rectangle/RectangleAlgebra.h"
#include "ListUtils.h"
#include "StandardTypes.h"
#include "QueryProcessor.h"

using namespace std;

namespace drel {

    /*
    1.1 ~findAttribute~

    Return true if the attribute name is found in the attribute list. False 
    otherwise.

    */
    bool DRelHelpers::findAttribute( 
        ListExpr attrList, const string& name, ListExpr& type ) {

        return !( listutils::findAttribute( attrList, name, type ) == 0 );
    }

    /*
    1.2 ~countRecords~

    Counts the records in a given relation. If an error occour false is 
    returned. True otherwise. The number of records is records.

    */
    bool DRelHelpers::countRecords( 
        const string attrName, const string relName, int& records ) {

        string query = "( count ( filter( feed " + relName + " ) "
            "( fun( t TUPLE ) ( isdefined (attr t " + attrName + " ) ) ) ) )";
        Word result;
        if( !QueryProcessor::ExecuteQuery( query, result ) ) {
            cout << "ExecuteError of " + query << endl;
            records = -1;
            return false;
        }

        CcInt* number = ( CcInt* )result.addr;
        if( !number->IsDefined( ) ) {
            number->DeleteIfAllowed( );
            records = -1;
            return false;
        }

        records = number->GetIntval( );
        number->DeleteIfAllowed( );
        result.setAddr( 0 );

        return true;
    }

    /*
    1.3 ~createBoundaryQuery~

    Creates a boundary object. This is a factory function for the boundary 
    class.

    */
    Boundary* DRelHelpers::createBoundaryQuery( 
        const std::string relation, 
        const std::string attrName, 
        const int boundarySize ) {

        string query = "( createboundary ( " + relation + " ) ( " + 
            attrName + " ) " + to_string( boundarySize ) + " ) ";
        Word result;
        if( !QueryProcessor::ExecuteQuery( query, result ) ) {
            cout << "ExecuteError of " + query << endl;
            return 0;
        }

        Boundary* boundary = ( Boundary* )result.addr;
        if( !boundary->isDefined( ) ) {
            delete boundary;
            return 0;
        }

        return boundary;
    }

    /*
    1.4 ~randomBoundaryName~

    Genereates a random name for a boundary object.

    */
    string DRelHelpers::randomBoundaryName( ) {

        return "Boundary" + to_string( rand( ) );
    }
    
    /*
    1.5 ~randomGridName~

    Genereates a random name for a grid object.

    */
    string DRelHelpers::randomGridName( ) {

        return "Grid" + to_string( rand( ) );
    }

    /*
    1.6 ~createGrid~

    Creates a grid object. This is a factory function for CellGrid2D.

    */
    temporalalgebra::CellGrid2D* DRelHelpers::createGrid(
        Relation* _rel, int _attr, int _arraySize ) {

        assert( _arraySize > 0 );
        assert( _attr >= 0 );

        string query = "( rect2cellgrid( collect_box( "
            "transformstream( projectextend( feed strassen )( )"
            "( ( Box( fun( tuple1 TUPLE )( bbox( attr tuple1 GeoData )"
            ") ) ) ) ) ) TRUE )" + to_string( _arraySize ) + " )";
        Word result;
        if( !QueryProcessor::ExecuteQuery( query, result ) ) {
            return 0;
        }

        temporalalgebra::CellGrid2D* grid = ( 
            temporalalgebra::CellGrid2D* )result.addr;

        return grid;
    }
    
    /*
    1.7 ~setGrid~

    Uses a Rectangle and munipulates a grid by setting generated values for an 
    arraySize.

    */
    void DRelHelpers::setGrid( 
        temporalalgebra::CellGrid2D* grid, Rectangle<2>* rect, 
        const int _arraySize ) {

        assert( _arraySize > 0 );

        // compute cell height and cell weigth, we use _arraySize*_arraySize 
        // as number of the cells.
        double cellheight = 
            ( rect->MaxD( 1 ) - rect->MinD( 1 ) ) / _arraySize;
        //double cellwidth = 
        //    ( rect->MaxD( 0 ) - rect->MinD( 0 ) ) / _arraySize;

        /*grid->set( rect->MinD( 0 ), rect->MinD( 1 ), 
            cellheight, cellwidth, _arraySize*_arraySize );*/
        grid->set( rect->MinD( 0 ), rect->MinD( 1 ), 
            cellheight, cellheight, _arraySize*_arraySize );
    }

} // end of namespace drel