/*
\def\CC{C\raise.22ex\hbox{{\footnotesize +}}\raise.22ex\hbox{\footnotesize +}\xs
pace}
\centerline{\LARGE \bf  DisplayTTY}

\centerline{Friedhelm Becker , Mai1998}

\begin{center}
\footnotesize
\tableofcontents
\end{center}

1 Overview

There must be exactly one TTY display function of type DisplayFunction
for every type constructor provided by any of the algebra modules which
are loaded by *AlgebraManager2*. The first parameter is the type
expression in numeric nested list format describing the value which is
going to be displayed by the display function. The second parameter is
this value in nested list format.

1.2 Includes and defines

maxLineLength gives the maximum length of an input line (an command)
which will be read.

*/

using namespace std;

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "DisplayTTY.h"
#include "NestedList.h"
#include "SecondoInterface.h"
#include "AlgebraTypes.h"
#include "Base64.h"

#define LINELENGTH 80
/*1.3 Managing display functions

The map *displayFunctions* holds all existing display functions. It
is indexed by a string created from the algebraId and the typeId of
the corresponding type constructor.

*/

SecondoInterface* DisplayTTY::si = 0;
NestedList*       DisplayTTY::nl = 0;
map<string,DisplayFunction> DisplayTTY::displayFunctions;

/*
The function *CallDisplayFunction* uses its first argument *idPair*
--- consisting of the two-elem-list $<$algebraId, typeId$>$ --- to
find the right display function in the array *displayFunctions*. The
arguments *typeArg* and *valueArg* are simply passed to this display
function.

*/

void
DisplayTTY::CallDisplayFunction( const ListExpr idPair,
                                 ListExpr type,
                                 ListExpr numType,
                                 ListExpr value )
{
  ostringstream osId;
  osId << "[" << nl->IntValue( nl->First( idPair ) )
       << "|" << nl->IntValue( nl->Second( idPair ) ) << "]";
  map<string,DisplayFunction>::iterator dfPos =
    displayFunctions.find( osId.str() );
  if ( dfPos != displayFunctions.end() )
  {
    (*(dfPos->second))( type, numType, value );
  }
  else
  {
    DisplayGeneric( type, numType, value );
  }
}

/*
The procedure *InsertDisplayFunction* inserts the procedure given as
second argument into the array displayFunctions at the index which is
determined by the type constructor name given as first argument.

*/

void
DisplayTTY::InsertDisplayFunction( const string& name,
                                   DisplayFunction df )
{
  int algebraId, typeId;
  si->GetTypeId( ExecutableLevel, name, algebraId, typeId );
  ostringstream osId;
  osId << "[" << algebraId << "|" << typeId << "]";
  displayFunctions[osId.str()] = df;
}

/*
1.4 Display functions

Display functions of the DisplayTTY module are used to  transform a
nested list value into a pretty printed output in text format. Display
functions which are called with a value of compound type usually call
recursively the display functions of the subtypes, passing the subtype
and subvalue, respectively.

*/

void
DisplayTTY::DisplayGeneric( ListExpr type, ListExpr numType, ListExpr value )
{
  cout << "No specific display function defined. Generic function used." << endl;
  cout << "Type: ";
  nl->WriteListExpr( type, cout );
  cout << endl << "Value: ";
  nl->WriteListExpr( value, cout );
}

void
DisplayTTY::DisplayRelation( ListExpr type, ListExpr numType, ListExpr value )
{
  type = nl->Second( type );
  numType = nl->Second( numType );
  CallDisplayFunction( nl->First( numType ), type, numType, value );
}

int
DisplayTTY::MaxHeaderLength( ListExpr type )
{
  int max, len;
  string s;
  max = 0;
  while (!nl->IsEmpty( type ))
  {
    s = nl->StringValue( nl->First( type ) );
    len = s.length();
    if ( len > max )
    {
      max = len;
    }
    type = nl->Rest( type );
  }
  return (max);
}


int
DisplayTTY::MaxAttributLength( ListExpr type )
{
  int max, len;
  string s;
  max = 0;
  while (!nl->IsEmpty( type ))
  {
    s = nl->SymbolValue( nl->First( nl->First( type ) ) );
    len = s.length();
    if ( len > max )
    {
      max = len;
    }
    type = nl->Rest( type );
  }
  return (max);
}

void
DisplayTTY::DisplayTuple( ListExpr type, ListExpr numType,
                          ListExpr value, const int maxNameLen )
{
  string s, blanks;
  while (!nl->IsEmpty( value ))
  {
    cout << endl;
    s = nl->SymbolValue( nl->First( nl->First( numType ) ) );
    blanks.assign( maxNameLen-s.length() , ' ' );
    cout << blanks << s << ": ";

    if( nl->IsAtom( nl->First( nl->Second( nl->First( numType ) ) ) ) )
    {
      CallDisplayFunction( nl->Second( nl->First( numType ) ),
                           nl->Second( nl->First( type ) ),
                           nl->Second( nl->First( numType ) ),
                           nl->First( value ) );
    }
    else
    {
      CallDisplayFunction( nl->First( nl->Second( nl->First( numType ) ) ),
                           nl->Second( nl->First( type ) ),
                           nl->Second( nl->First( numType ) ),
                           nl->First( value ) );
    }

    type    = nl->Rest( type );
    numType = nl->Rest( numType );
    value   = nl->Rest( value );
  }
}

void
DisplayTTY::DisplayTuples( ListExpr type, ListExpr numType, ListExpr value )
{
  int maxAttribNameLen = MaxAttributLength( nl->Second( numType ) );
  while (!nl->IsEmpty( value ))
  {
    DisplayTuple( nl->Second( type ), nl->Second( numType ),
                  nl->First( value ), maxAttribNameLen );
    value = nl->Rest( value );
    cout << endl;
  }
}

void
DisplayTTY::DisplayInt( ListExpr type, ListExpr numType, ListExpr value )
{
  if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType && nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else
  {
    cout << nl->IntValue( value );
  }
}

void
DisplayTTY::DisplayReal( ListExpr type, ListExpr numType, ListExpr value )
{
  if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType && nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else
  {
    cout << nl->RealValue( value );
  }
}

void
DisplayTTY::DisplayBoolean( ListExpr list, ListExpr numType, ListExpr value )
{
  if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType && nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else
  {
    if ( nl->BoolValue( value ) )
    {
      cout << "TRUE";
    }
    else
    {
      cout << "FALSE";
    }
  }
}

void
DisplayTTY::DisplayString( ListExpr type, ListExpr numType, ListExpr value )
{
  if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType && nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else
  {
    cout << nl->StringValue( value );
  }
}

void
DisplayTTY::DisplayText( ListExpr type, ListExpr numType, ListExpr value )
{
  string printstr, line, restline;
  int lastblank, position;
  bool lastline;

  if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
    nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else
  {
    TextScan txtscan = nl->CreateTextScan(nl->First(value));
    nl->GetText(txtscan, nl->TextLength(nl->First(value)),printstr);
    position = 0;
    lastblank = -1;
    line = "";
    for (unsigned i = 1; i <= printstr.length(); i++)
    {
      line += printstr[i-1];
      if (printstr[i-1] == ' ') lastblank = position;
      position++;
      lastline = ( i == printstr.length() );
      if ( position == LINELENGTH || lastline || (printstr[i-1] == '\n') )
      {
        if ( lastline || (printstr[i-1] == '\n') )
	{
	  cout << line;
	  line = "";
	  lastblank = -1;
	  position = 0;
	}
        else
	{
          if ( lastblank > 0 )
	  {
	    cout << line.substr(0, lastblank) << endl;
	    restline = line.substr(lastblank+1, position);
	    line = "";
	    line += restline;
	    lastblank = -1;
	    position = line.length();
	  }
	  else
	  {
	    cout << line << endl;
	    line = "";
	    lastblank = -1;
	    position = 0;
	  }
	}
      }
    }
  }
}

void
DisplayTTY::DisplayFun( ListExpr type, ListExpr numType, ListExpr value )
{
  cout << "Function type: ";
  nl->WriteListExpr( type, cout );
  cout << endl << "Function value: ";
  nl->WriteListExpr( value, cout );
  cout << endl;
}

void
DisplayTTY::DisplayDate( ListExpr type, ListExpr numType, ListExpr value)
{
  if (nl->IsAtom(value) && nl->AtomType(value)==StringType)
      cout <<nl->StringValue(value);
  else
      cout <<"Incorrect Data Format!";
}


void
DisplayTTY::DisplayBinfile( ListExpr type, ListExpr numType, ListExpr value)
{
   cout <<"binary file";
}



double DisplayTTY::getNumeric(ListExpr value, bool &err){
   if(nl->AtomType(value)==IntType){
      err=false;
      return nl->IntValue(value);
   }
   if(nl->AtomType(value)==RealType){
      err=false;
      return nl->RealValue(value);
   }
   if(nl->AtomType(value)==NoAtom){
      int len = nl->ListLength(value);
      if(len!=5 & len!=6){
        err=true;
	return 0;
      }
      ListExpr F = nl->First(value);
      if(nl->AtomType(F)!=SymbolType){
         err=true;
	 return 0;
      }
      if(nl->SymbolValue(F)!="rat"){
        err=true;
        return 0;
      }
      value = nl->Rest(value);
      double sign = 1.0;
      if(nl->ListLength(value)==5){  // with sign
        ListExpr SignList = nl->First(value);
        if(nl->AtomType(SignList)!=SymbolType){
	   err=true;
           return 0;
	}
        string SignString = nl->SymbolValue(SignList);
	if(SignString=="-")
	   sign = -1.0;
	else if(SignString=="+")
	   sign = 1.0;
	else{
	  err=true;
	  return 0;
	}
        value= nl->Rest(value);
      }
      if(nl->AtomType(nl->First(value))==IntType && nl->AtomType(nl->Second(value))==IntType &&
         nl->AtomType(nl->Third(value))==SymbolType && nl->SymbolValue(nl->Third(value))=="/" &&
	 nl->AtomType(nl->Fourth(value))==IntType){
	    err=false;
	    double intpart = nl->IntValue(nl->First(value));
	    double numDecimal = nl->IntValue(nl->Second(value));
	    double denomDecimal = nl->IntValue(nl->Fourth(value));
	    if(denomDecimal==0){
	       err=true;
	       return 0;
	    }
	    double res1 = intpart*denomDecimal + numDecimal/denomDecimal;
	    return sign*res1;
	 } else{
	err = true;
	return 0;
     }
   }
   err=true;
   return 0;
}

void
DisplayTTY::DisplayXPoint( ListExpr type, ListExpr numType, ListExpr value)
{
  if(nl->ListLength(value)!=2)
     cout << "Incorrect Data Format";
  else{
     bool err;
     double x = getNumeric(nl->First(value),err);
     if(err){
       cout << "Incorrect Data Format";
       return;
     }
     double y = getNumeric(nl->Second(value),err);
     if(err){
       cout << "Incorrect Data Format";
       return;
     }
     cout << "xpoint (" << x << "," << y << ")";
  }
}

void
DisplayTTY::DisplayPoint( ListExpr type, ListExpr numType, ListExpr value)
{
  if(nl->ListLength(value)!=2)
     cout << "Incorrect Data Format";
  else{
     bool err;
     double x = getNumeric(nl->First(value),err);
     if(err){
       cout << "Incorrect Data Format";
       return;
     }
     double y = getNumeric(nl->Second(value),err);
     if(err){
       cout << "Incorrect Data Format";
       return;
     }
     cout << "point: (" << x << "," << y << ")";
  }
}

void
DisplayTTY::DisplayRect( ListExpr type, ListExpr numType, ListExpr value)
{
  if(nl->ListLength(value)!=4)
     cout << "Incorrect Data Format";
  else{
     bool err;
     double x1 = getNumeric(nl->First(value),err);
     if(err){
       cout << "Incorrect Data Format";
       return;
     }
     double y1 = getNumeric(nl->Second(value),err);
     if(err){
       cout << "Incorrect Data Format";
       return;
     }
     double x2 = getNumeric(nl->Third(value),err);
     if(err){
       cout << "Incorrect Data Format";
       return;
     }
     double y2 = getNumeric(nl->Fourth(value),err);
     if(err){
       cout << "Incorrect Data Format";
       return;
     }
     cout << "rect: ( (" << x1 << "," << y1 << ")->(" << x2 << "," << y2 <<"))";
  }
}

void
DisplayTTY::DisplayMP3( ListExpr type, ListExpr numType, ListExpr value)
{
    if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType && nl->SymbolValue( value ) == "undef" )
	cout << "UNDEFINED";
    else
	cout << "mp3 file";
}

void
DisplayTTY::DisplayID3( ListExpr type, ListExpr numType, ListExpr value)
{
    if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType && nl->SymbolValue( value ) == "undef" )
    {
	cout << "UNDEFINED";
    }
    else
    {
	cout << "ID3-Tag"<<endl << endl;
	cout << "Title   : " << nl->StringValue (nl->First (value)) <<endl;
	cout << "Author  : " << nl->StringValue (nl->Second (value)) << endl;
	cout << "Album   : " << nl->StringValue (nl->Third (value)) << endl;
	cout << "Year    : " << nl->IntValue (nl->Fourth (value)) << endl;
	cout << "Comment : " << nl->StringValue (nl->Fifth (value)) << endl;

	if (nl->ListLength(value) == 6)
	{
	    cout << "Genre   : " << nl->StringValue (nl->Sixth (value)) << endl;
	}
	else
	{
	    cout << "Track   : " << nl->IntValue (nl->Sixth (value)) << endl;
	    cout << "Genre   : " << nl->StringValue (nl->Sixth (nl->Rest (value))) << endl;
	}
    }
}

void
DisplayTTY::DisplayLyrics( ListExpr type, ListExpr numType, ListExpr value)
{
    if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType && nl->SymbolValue( value ) == "undef" )
    {
	cout << "UNDEFINED";
    }
    else
    {
	cout << "Lyrics"<<endl<<endl;
	int no = nl->ListLength (value) / 2;
	for (int i=1; i<=no; i++)
	{
	    cout << "[" << nl->IntValue ( nl->First (value)) / 60 << ":";
	    cout << nl->IntValue ( nl->First (value)) % 60 << "] ";
	    cout << nl->StringValue (nl->Second (value));
	    value = nl->Rest (nl->Rest (value));
	}
    }
}

void
DisplayTTY::DisplayMidi (ListExpr type, ListExpr numType, ListExpr value)
{
  int size = nl->IntValue(nl->Second(value));
  int noOfTracks = nl->IntValue(nl->Third(value));
  cout << "Midi: " << size << "bytes, ";
  cout << noOfTracks << " tracks";
}


void
DisplayTTY::DisplayArray( ListExpr type, ListExpr numType, ListExpr value)
{

  if(nl->ListLength(value)==0)
     cout << "an empty array";
  else{
     ListExpr AType = nl->Second(type);
     ListExpr ANumType = nl->Second(numType);
     // find the idpair
     ListExpr idpair = ANumType;
     while(nl->AtomType(nl->First(idpair))!=IntType)
        idpair = nl->First(idpair);

     int No = 1;
     cout << "*************** BEGIN ARRAY ***************" << endl;
     while( !nl->IsEmpty(value)){
        cout << "--------------- Field No: " << No++ << " ---------------" << endl;
        CallDisplayFunction(idpair,AType,ANumType,nl->First(value));
	cout << endl;
	value = nl->Rest(value);
     }
     cout << "***************  END ARRAY  ***************";

  }
}

void
DisplayTTY::DisplayResult( ListExpr type, ListExpr value )
{
  int algebraId, typeId;
  string  name;
  si->LookUpTypeExpr( ExecutableLevel, type, name, algebraId, typeId );
  ListExpr numType = si->NumericTypeExpr( ExecutableLevel, type );
  if ( !nl->IsAtom( type ) )
  {
    CallDisplayFunction( nl->First( numType ), type, numType, value );
  }
  else
  {
    CallDisplayFunction( numType, type, numType, value );
  }
  nl->Destroy( numType );
  cout << endl;
}

void
DisplayTTY::DisplayDescriptionLines( ListExpr value, int  maxNameLen)
{
  string s, blanks, printstr, line, restline, descrstr;
  int position, lastblank;
  ListExpr valueheader, valuedescr;
  bool firstline, lastline;

  valueheader = nl->Second(value);
  valuedescr  = nl->Third(value);

  cout << endl;

  blanks.assign( maxNameLen-4 , ' ' );
  cout << blanks << "Name: " << nl->SymbolValue(nl->First(value)) << endl;

  while (!nl->IsEmpty( valueheader ))
  {
    s = nl->StringValue( nl->First( valueheader ));
    blanks.assign( maxNameLen-s.length() , ' ' );
    //cout << blanks << s << ": ";
    printstr = blanks + s + ": ";

    if( nl->IsAtom(nl->First( valuedescr ))) //&&
      //nl->AtomType(nl->First(valuedescr))==StringType)
    {
      if ( nl->AtomType(nl->First(valuedescr))==StringType )
      //DisplayString(nl->TheEmptyList(), nl->TheEmptyList(),
        //nl->First(valuedescr));
      printstr += nl->StringValue( nl->First(valuedescr) );
      else
      {
        if ( nl->AtomType(nl->First(valuedescr))==TextType )
	{
	  TextScan txtscan = nl->CreateTextScan(nl->First(valuedescr));
	  descrstr = "";
	  nl->GetText(txtscan, nl->TextLength(nl->First(valuedescr)),descrstr);
	  printstr += descrstr;
	  //cout << printstr << endl;
	  nl->DestroyTextScan(txtscan);
	}
      }
      //check whether line break is necessary
      if (printstr.length() <= LINELENGTH) cout << printstr << endl;
      //cout << endl;
      else
      {
        firstline = true;
        position = 0;
	lastblank = -1;
	line = "";
        for (unsigned i = 1; i <= printstr.length(); i++)
        {
	  line += printstr[i-1];
	  //cout << line << endl;
	  if (printstr[i-1] == ' ') lastblank = position;
	  position++;
	  lastline = (i == printstr.length());
	  if ( (firstline && (position == LINELENGTH)) || (!firstline &&
	  (position == (LINELENGTH-maxNameLen-2))) || lastline )
	  //if ((position == LINELENGTH) || lastline)
	  {
	    if (lastblank > 0)
	    {
	      if (firstline)
	      {
	        if (lastline && (line.length() <= LINELENGTH))
		{
		  cout << line << endl;
		}
	        else cout << line.substr(0, lastblank) << endl;
		firstline = false;
	      }
	      else
	      {
	        blanks.assign( maxNameLen+2 , ' ' );
		if (lastline && (line.length() <= LINELENGTH))
		{
		  cout << blanks << line << endl;
		}
	        else cout << blanks << line.substr(0, lastblank) << endl;
	      }
	      restline = line.substr(lastblank+1, position);
	      line = "";
	      line += restline;
	      lastblank = -1;
	      position = line.length();
	    }
	    else
	    {
	      if (firstline)
	      {
	        cout << line << endl;
		firstline = false;
	      }
	      else
	      {
	        blanks.assign( maxNameLen+2 , ' ' );
	        cout << blanks << line << endl;
	      }
	      line = "";
	      lastblank = -1;
	      position = 0;
	    }
	  }
	}
      }
    }
    valueheader   = nl->Rest( valueheader );
    valuedescr    = nl->Rest( valuedescr );
  }
  nl->Destroy( valueheader );
  nl->Destroy( valuedescr );
}


ListExpr
DisplayTTY::ConcatLists( ListExpr list1, ListExpr list2)
{
  if (nl->IsEmpty(list1))
  {
    return list2;
  }
  else
  {
    ListExpr first = nl->First(list1);
    ListExpr rest = nl->Rest(list1);

    ListExpr second =  ConcatLists(rest, list2);

    ListExpr newnode = nl->Cons(first,second);
    return newnode;
  }
}


void
DisplayTTY::DisplayResult2( ListExpr value )
{
  ListExpr InquiryType = nl->First(value);
  string TypeName = nl->SymbolValue(InquiryType);
  ListExpr v = nl->Second(value);
  if(TypeName=="databases"){
      cout << endl << "--------------------" << endl;
      cout << "Database(s)" << endl;
      cout << "--------------------" << endl;
      if(nl->ListLength(v)==0)
         cout << "none" << endl;
      while(!nl->IsEmpty(v)){
        cout << "  " << nl->SymbolValue(nl->First(v)) << endl;
	v = nl->Rest(v);
      }
      return;
  }else if(TypeName=="algebras"){
   cout << endl << "--------------------" << endl;
      cout << "Algebra(s) " << endl;
      cout << "--------------------" << endl;
      if(nl->ListLength(v)==0)
         cout << "none" << endl;
      while(!nl->IsEmpty(v)){
        cout << "  " << nl->SymbolValue(nl->First(v)) << endl;
	v = nl->Rest(v);
      }
      return;
  }else if(TypeName=="types"){
      nl->WriteListExpr(v,cout);
      return;
  }else if(TypeName=="objects"){
      nl->WriteListExpr(v,cout);
      return;
  } else if(TypeName=="constructors" || TypeName=="operators"){
      cout << endl << "--------------------" << endl;
      if(TypeName=="constructors")
         cout <<"Type Constructor(s)\n";
      else cout << "Operator(s)" << endl;
      cout << "--------------------" << endl;
      if(nl->IsEmpty(v)){
         cout <<"  none " << endl;
      } else{
         ListExpr headerlist = v;
	 int MaxLength = 0;
	 int currentlength;
	 while(!nl->IsEmpty(headerlist)){
        ListExpr tmp = (nl->Second(nl->First(headerlist)));
	    while(!nl->IsEmpty(tmp)){
	       currentlength = (nl->StringValue(nl->First(tmp))).length();
	       tmp = nl->Rest(tmp);
 	       if(currentlength>MaxLength)
	          MaxLength = currentlength;
	    }
	    headerlist = nl->Rest(headerlist);
	 }

         while (!nl->IsEmpty( v ))
         {
            DisplayDescriptionLines( nl->First(v), MaxLength );
            v   = nl->Rest( v );
         }
     }
  }else if(TypeName=="algebra"){
      string AlgebraName = nl->SymbolValue(nl->First(v));
      cout << endl << "-----------------------------------" << endl;
      cout << "Algebra : " << AlgebraName << endl;
      cout << "-----------------------------------" << endl;
      ListExpr Cs = nl->First(nl->Second(v));
      ListExpr Ops = nl->Second(nl->Second(v));
      // determine the headerlength
      ListExpr tmp1 = Cs;
      int maxLength=0;
      int len;
      while(!nl->IsEmpty(tmp1)){
        ListExpr tmp2 = nl->Second(nl->First(tmp1));
	while(!nl->IsEmpty(tmp2)){
          len = (nl->StringValue(nl->First(tmp2))).length();
	  if(len>maxLength)
	     maxLength=len;
	  tmp2 = nl->Rest(tmp2);
	}
        tmp1 = nl->Rest(tmp1);
      }
      tmp1 = Ops;
      while(!nl->IsEmpty(tmp1)){
        ListExpr tmp2 = nl->Second(nl->First(tmp1));
	while(!nl->IsEmpty(tmp2)){
          len = (nl->StringValue(nl->First(tmp2))).length();
	  if(len>maxLength)
	     maxLength=len;
	  tmp2 = nl->Rest(tmp2);
	}
        tmp1 = nl->Rest(tmp1);
      }

      cout << endl << "-------------------------" << endl;
      cout << "  "<< "Type Constructor(s)" << endl;
      cout << "-------------------------" << endl;
      if(nl->ListLength(Cs)==0)
         cout << "  none" << endl;
      while(!nl->IsEmpty(Cs)){
         DisplayDescriptionLines(nl->First(Cs),maxLength);
	 Cs = nl->Rest(Cs);
      }

      cout << endl << "-------------------------" << endl;
      cout << "  " << "Operator(s)" << endl;
      cout << "-------------------------" << endl;
      if(nl->ListLength(Ops)==0)
         cout << "  none" << endl;
      while(!nl->IsEmpty(Ops)){
         DisplayDescriptionLines(nl->First(Ops),maxLength);
	 Ops = nl->Rest(Ops);
      }
  }else{
    cout << "unknow inquiry type" << endl;
    nl->WriteListExpr(value,cout);
  }
}

void
DisplayTTY::Initialize( SecondoInterface* secondoInterface )
{
  si = secondoInterface;
  nl = si->GetNestedList();
  InsertDisplayFunction( "int",     &DisplayInt );
  InsertDisplayFunction( "real",    &DisplayReal );
  InsertDisplayFunction( "bool",    &DisplayBoolean );
  InsertDisplayFunction( "string",  &DisplayString );
  InsertDisplayFunction( "rel",     &DisplayRelation );
  InsertDisplayFunction( "mrel",    &DisplayRelation );
  InsertDisplayFunction( "tuple",   &DisplayTuples );
  InsertDisplayFunction( "mtuple",  &DisplayTuples );
  InsertDisplayFunction( "map",     &DisplayFun );
  InsertDisplayFunction( "date",    &DisplayDate );
  InsertDisplayFunction( "text",    &DisplayText );
  InsertDisplayFunction( "xpoint",  &DisplayXPoint);
  InsertDisplayFunction( "rect",    &DisplayRect);
  InsertDisplayFunction( "array",   &DisplayArray);
  InsertDisplayFunction( "point",   &DisplayPoint);
  InsertDisplayFunction( "binfile", &DisplayBinfile);
  InsertDisplayFunction( "mp3",     &DisplayMP3);
  InsertDisplayFunction( "id3",     &DisplayID3);
  InsertDisplayFunction( "lyrics",  &DisplayLyrics);
  InsertDisplayFunction( "midi",    &DisplayMidi);
}

