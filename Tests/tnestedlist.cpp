/*

//paragraph [1] Title: [{\Large \bf ] [}]
//[ae] [\"a]
//[oe] [\"o]
//[ue] [\"u]
//[ss] [{\ss}]
//[->] [$\rightarrow $]

[1] Module TestNL: Tests with Stable Nested Lists


April 1996 Carsten Mund

Februar 2002 M. Spiekermann, Environment for the SMI was added.

August 2003 M. Spiekermann, The test code for the persistent and main-memory implementations were 
                            merged into one code file. 


1 Introduction

In this module the nested list functions from the module NestedList are called to test them and to demonstrate how to use them.

2 Imports

******************************************************************************/

#include <string>
#include "NestedList.h"
#include "SecondoSMI.h"

namespace {

SmiRecordFile* rf = 0;

/******************************************************************************

3 Preliminaries

3.1 Useful helper functions

******************************************************************************/

void pause()
{
   char buf[80];
   cout << "<<< Press return to continue >>>" << endl;
   cin.getline( buf, sizeof(buf) );
}

/******************************************************************************

3.2 Local Procedures

******************************************************************************/

string WriteBool( bool b )
{
   if ( b )
     return "TRUE";
   else
     return "FALSE";
}

int recCounter;
NestedList* nl = 0;

ListExpr
ConcatLists( ListExpr list1, ListExpr list2)
{
  recCounter++;
  if (nl->IsEmpty(list1))
  {
    cerr << endl << "### ConcatLists - terminated." << endl;
    return list2;
  }
  else
  {
    ListExpr first = nl->First(list1);
    ListExpr rest = nl->Rest(list1);

    //cerr << "( rec:" << recCounter << " , first:" << first << ", rest:" << rest << " )"; 

    ListExpr second =  ConcatLists(rest, list2);

    ListExpr newnode = nl->Cons(first,second);
    return newnode;
  }
}




/******************************************************************************

4 Module body


4.1 Copy lists between two C++ NestedList-Objects

******************************************************************************/



int
TestNLCopy()
{
   int testcase = 0;
   NestedList listA;
   NestedList listB;

   cout << endl << "### Copy lists between two C++ NestedList-Objects" << endl;

   cout << "NodeRecord Size: " << sizeof(NodeRecord) << endl
	<< "ConstRecord Size: " << sizeof(Constant) << endl 
	<< endl;
   
   cout << ++testcase << " reportVectorSizes():" << endl
	<< "listA: " << listA.reportVectorSizes() << endl;

   string listStr1("(create fifty : (rel(tuple((n int)))))");
   //string listStr1("(0 (1 2) (3 4))"); 
   //string listStr1("(1 2 (3 5) 7 (9 2 3 4 5) (2 (3 4 (5 6)) 4))");
   //string listStr1("(1 \"jhgjhg\" 6 <text> hallo dies inst ein recht langer und langweiliger Text, hier gibt es nichts zu erfahren. Wir wollen nur testen ob Text-Atome korrekt behandelt werden. </text---> \"anbsdfklsd sksdjf sdksdf sdfj asdkjf sdkjssd\" 7 (9 10))");
   ListExpr listExp1 =0;
   cout << ++testcase << " ReadFromString():" << endl
	<< "Trying String: " << ">>" << listStr1 << "<<" << endl;
   if ( listA.ReadFromString(listStr1, listExp1) ) {
      cout << "Ok!" << endl;
   } else {
      cout << "Failed!" << endl;
   }

   cout << ++testcase << " reportVectorSizes():" << endl
	<< "listA: " << listA.reportVectorSizes() << endl;
   
      
   string listStr2("");
   listA.WriteToString(listStr2, listExp1);
   cout << ++testcase << " WriteToString(): " << endl
	<< "Result: " << ">>" << listStr2 << "<<" << endl;
   

   cout << ++testcase << " reportVectorSizes():" << endl
	<< "listA: " << listA.reportVectorSizes() << endl;
   
   string listStr3("");
   ListExpr newExp1 = listA.CopyList(listExp1, &listB);
   listB.WriteToString(listStr3, newExp1);
   cout << ++testcase << " CopyList(): " << endl
	<< "Result: " << ">>" << listStr3 << "<<" << endl;

   cout << ++testcase << " reportVectorSizes(): " << endl
	<< "listA: " << listA.reportVectorSizes() << endl
	<< "listB: " << listB.reportVectorSizes() << endl;
   
   return 0;
}

/*
The next functions contain code which is extraced from
the secondo system to isolate bugs.
*/

void
StringAtom_bug() {

   //NestedList nl(10,10,10,10);
   NestedList nl(rf,10,10,10,10);

   cout << "Test of String Atoms." << endl << endl;
   cout << "MemoryModel: " << nl.MemoryModel() << endl;

   NestedList* pnl = &nl;

   /* An example of an algebra property */
   ListExpr examplelist = pnl->TextAtom();
   pnl->AppendText(examplelist,"<relation> createbtree [<attrname>] where "
   "<attrname> is the key");

   ListExpr result = pnl->TwoElemList(
            pnl->TwoElemList(pnl->StringAtom("Creation"), 
			     pnl->StringAtom("Example Creation")),    
            pnl->TwoElemList(examplelist, 
			     pnl->StringAtom("(let mybtree = ten "
			     "createbtree [no])")));
   cerr << endl << "### BTreeProp(): " << pnl->ToString(result) << endl;

  exit(0);

}

void
ConcatLists_bug() {

   //NestedList Nl(10,10,10,10);
   NestedList Nl(rf,10,10,10,10);
   nl = &Nl;

   cout << "Test of String Atoms." << endl << endl;
   cout << "MemoryModel: " << nl->MemoryModel() << endl;

   ListExpr headerlist=0;
   ListExpr concatenatedlist = 0;
   nl->ReadFromFile("ListOperators.nl", headerlist);
   headerlist = nl->Second(headerlist);

  concatenatedlist = nl->TheEmptyList();
  concatenatedlist = nl->Second( nl->First(headerlist) );
  headerlist = nl->Rest(headerlist);

  recCounter = 0;
  while (!nl->IsEmpty( headerlist ))
  {
    cout << " header: " << nl->ToString(headerlist) << endl; 
    concatenatedlist = 
      ConcatLists( concatenatedlist, nl->Second(nl->First(headerlist)) );
    headerlist = nl->Rest(headerlist);
    recCounter++;
  }
 
  cerr << endl << "### concatenatedlist: " << nl->ToString(concatenatedlist);

}

int 
TestBasicOperations()
{
   ListExpr  ListExpr1,  ListExpr2,  ListExpr3, 
	     ListExpr4,  ListExpr5,  ListExpr6,
	     ListExpr8,  ListExpr9, ListExpr15,
	     EmptyListVar,
	     IntAtomVar,
	     RealAtomVar,
	     BoolAtomVar,
	     StringAtomVar,
	     SymbolAtomVar,
	     TextAtomVar, TextAtomVar2;

   long IntValue, IntValue2, ErrorVar;
   double RealValue, RealValue2;
   bool BoolValue, BoolValue2;

   string NLStringValue, NLStringValue2, SymbolValue, SymbolValue2;
   string String1, String2, String3, Chars;
   TextScan TextScan1;
   Cardinal Position;

   
   NestedList nl(rf,10,10,10,10);

/******************************************************************************

4.1 Elementary methods 

4.1.1 Empty List 

******************************************************************************/
   EmptyListVar = nl.TheEmptyList(); 
   if ( nl.IsEmpty (EmptyListVar) )
   {
     cout << "EmptyListVar is the empty list." << endl << endl;
   }

/******************************************************************************

4.1.2 Integer atoms

******************************************************************************/
   IntValue = 123;
   IntAtomVar = nl.IntAtom (IntValue);
   if ( nl.AtomType (IntAtomVar) == IntType)
   {
     cout << "IntAtomVar is an INTEGER atom: ";
     IntValue2 = nl.IntValue (IntAtomVar);
     cout << IntValue2 << endl << endl;
   }

/****************************************************************************** 

4.1.3 Real atoms 

******************************************************************************/
   RealValue = 87654.321;
   RealAtomVar = nl.RealAtom (RealValue);
   if ( nl.AtomType(RealAtomVar) == RealType )
   {
     cout << "RealAtomVar is a REAL atom: ";
     RealValue2 = nl.RealValue (RealAtomVar);
     cout << RealValue2 << endl << endl;
   }

/******************************************************************************

4.1.4 Bool atoms 

******************************************************************************/
   BoolValue = true;
   BoolAtomVar = nl.BoolAtom (BoolValue);
   if ( (nl.AtomType (BoolAtomVar) == BoolType) )
   {
     cout << "BoolAtomVar is a bool atom: "; 
     BoolValue2 = nl.BoolValue (BoolAtomVar);
     cout << WriteBool(BoolValue2) << endl << endl;
   }

/******************************************************************************

4.1.5 String atoms

******************************************************************************/
   NLStringValue = "Here I go again and again";
   StringAtomVar = nl.StringAtom (NLStringValue);
   if (nl.AtomType(StringAtomVar) == StringType)
   {
     cout << "StringAtomVar is a string atom: "; 
     NLStringValue2 = nl.StringValue (StringAtomVar);
     cout << """" << NLStringValue2 << """" << endl << endl;
   }

/****************************************************************************** 

4.1.6 Symbol atoms 

******************************************************************************/
   SymbolValue = "<=";
   SymbolAtomVar = nl.SymbolAtom(SymbolValue);

   if ( (nl.AtomType (SymbolAtomVar) == SymbolType) )
   {
     cout << endl;
     cout << "SymbolAtomVar is a symbol atom: ";
     SymbolValue2 = nl.SymbolValue (SymbolAtomVar);
     cout << SymbolValue2 << endl << endl;
   }


/****************************************************************************** 

4.1.6 Text atoms and text scans 

******************************************************************************/

   /* Short text (one fragment only) */
   TextAtomVar = nl.TextAtom();
   Chars = "1__4__7__10__4__7__20__4__7__30__4__7__40__4__7__50__4__7";
   nl.AppendText (TextAtomVar, Chars);

   ListExpr textList = nl.OneElemList(TextAtomVar);
   string s1("");
   nl.WriteToString(s1, textList);
   cout << endl << "A one element list wit text atom: " << s1 << endl;
	   
   TextScan1 = nl.CreateTextScan (TextAtomVar);
   Chars = "";
   Position = 0;
   nl.GetText (TextScan1, 30, Chars);
   cout << "(TextScan1, 30, Chars): " << Chars << endl;
   nl.GetText (TextScan1, 17, Chars);
   cout << "(TextScan1, 17, Chars): " << Chars << endl;
   nl.GetText (TextScan1, 10, Chars);
   cout << "(TextScan1, 10, Chars): " << Chars << endl;

   nl.DestroyTextScan (TextScan1);

   cout << "After Text with one fragment: Memory-Usage: " << nl.reportVectorSizes() << endl;

   /* Text in several fragments */ 
   TextAtomVar2 = nl.TextAtom();
   Chars = 
     "1__4__7__10__4__7__20__4__7__30__4__7__40__4__7__50__4__7__60__4__7__70__4__7_";
   nl.AppendText (TextAtomVar2, Chars);
   
   Chars = 
     "1++4++7++10++4++7++20++4++7++30++4++7++40++4++7++50++4++7++60++4++7++70++4++7+";
   nl.AppendText (TextAtomVar2, Chars);
   textList = nl.OneElemList(TextAtomVar2);

   s1 = "";
   nl.WriteToString(s1, textList);
   cout << endl << "A list with a bigger text atom: " << s1 << endl;
   
   TextScan1 = nl.CreateTextScan (TextAtomVar2);
   Chars = "";
   Position = 0;
   while ( !nl.EndOfText (TextScan1) )
   {   
     nl.GetText (TextScan1, 50, Chars);
     cout << endl <<  "GetText(TextScan1, 50, Chars): " << Chars << endl;
   }
   cout << "After While" << endl;
   nl.DestroyTextScan (TextScan1);

   cout << "After Text with more than one fragment, Memory-Usage: " << nl.reportVectorSizes() << endl;
   
   ListExpr15 = nl.TwoElemList (TextAtomVar, TextAtomVar2);
   cout << "ListExpr15" << endl;


/****************************************************************************** 

4.2 List Construction and Traversal

******************************************************************************/
   ListExpr1 = nl.SixElemList (EmptyListVar, IntAtomVar, 
			       BoolAtomVar,  StringAtomVar,
			       SymbolAtomVar, ListExpr15);
   cout << "ListExpr1" << endl;

   ListExpr2 = nl.Cons (nl.StringAtom(NLStringValue2), ListExpr1);
   cout << "ListExpr2" << endl;

   ListExpr3 = nl.TwoElemList (ListExpr1, ListExpr2);
   cout << "ListExpr3" << endl;

   ListExpr4 = nl.Cons (StringAtomVar, nl.Second (ListExpr3));
   cout << "ListExpr4" << endl;


/******************************************************************************

4.3 In-/Output 

4.3.1 In-/Output of a small list expression

The following steps are executed with a small list expression. 

  1 ListExpr [->] File   [->] ListExpr [->] String ; PrintString

  2 ListExpr [->] String [->] ListExpr [->] String ; PrintString

*/
   cout << "WriteListExpr start" << endl;
   nl.WriteListExpr (ListExpr3);
   cout << "WriteListExpr stop" << endl;
   ErrorVar = nl.WriteToFile ("testout_SmallListFile", ListExpr3);
   cout << "WriteToFile" << endl;
   ErrorVar = nl.WriteToString ( String1, ListExpr3 );
   cout << "WriteToString" << endl;
   cout << endl;
   cout << "Small list - File test. Written to File: SmallList = "; 
   cout << String1 << endl << endl;

   ErrorVar = nl.ReadFromFile ("testout_SmallListFile", ListExpr5);
   ErrorVar = nl.WriteToString (String1, ListExpr5);
   cout << endl;
   cout << "Small list - File test. Found in File: SmallList = "; 
   cout << String1 << endl << endl;

   ErrorVar = nl.WriteToString (String2, ListExpr3);
   ErrorVar = nl.ReadFromString (String2, ListExpr6);
   ErrorVar = nl.WriteToString (String3, ListExpr6);
   cout << endl;
   cout << "Small list- String test. SmallList = ";
   cout << String3 << endl << endl;

/*

4.3.2 In-/Output of a complex list expression

The complex list expression is saved in the file ~testin-simple~.
The following five steps are executed:

File [->] ListExpr [->] File 
File [->] ListExpr [->] String [->] File

*/

   cout << endl << "Reading and writing file geo ..." << endl;
   ErrorVar = nl.ReadFromFile ("geo.nl", ListExpr8);
   ErrorVar = nl.WriteToFile ("geo_write.nl", ListExpr8);

   //ErrorVar = nl.WriteToString (String1, ListExpr8);
   //ErrorVar = nl.ReadFromString (String1, ListExpr9);
   //ErrorVar = nl.WriteToFile ("testout_simple2", ListExpr9);

   cout << endl << "Reading and writing operator lists ..." << endl;
   ListExpr ListConcat = 0;
   nl.ReadFromFile("ListOperators.nl", ListConcat);
   nl.WriteToFile("ListOperators_write.nl", ListConcat);
   
   ListExpr List2list = 0;
   nl.ReadFromFile("2lists.nl", List2list);
   nl.WriteToFile("2lists_write.nl", List2list);
   

/*

4.3.3 Test of nested instructions

*/

   ErrorVar = nl.WriteToFile ("testout_RestExpr5", nl.Rest (ListExpr5));
   cout << endl;

   ErrorVar = nl.WriteToFile ("testout_FirstExpr5", nl.First (ListExpr5));
   cout << endl;

  


   cout << "After String <-> List Conversions, Memory-Usage: " << nl.reportVectorSizes() << endl;
  
/******************************************************************************

4.4 Destruction 

******************************************************************************/

   nl.Destroy(ListExpr3);
   nl.Destroy(ListExpr4);
   nl.Destroy(ListExpr5);
   nl.Destroy(ListExpr6);
   nl.Destroy(ListExpr8);
   nl.Destroy(ListExpr9);

   ErrorVar = nl.ReadFromString("(<text>--------10--------20--------30--------40--------50--------60--------70--------80-------90-------100-------110-------120-------130-------140-------150-------160-------170-------180-------190-------200-------210-------</text--->)", ListExpr1);
   ErrorVar = nl.ReadFromString("(<text>--------10--------20--------30--------40--------50--------60--------70--------80-------90-------100-------110-------120-------130-------140-------150-------160-------170-------180-------190-------200-------210-------</text--->)", ListExpr2);
   ErrorVar = nl.ReadFromString("(<text>--------10--------20--------30--------40--------50--------60--------70--------80-------90-------100-------110-------120-------130-------140-------150-------160-------170-------180-------190-------200-------210-------</text--->)", ListExpr3);
   ErrorVar = nl.WriteToString(String1, ListExpr1);
   ErrorVar = nl.WriteToString(String2, ListExpr2);
   ErrorVar = nl.WriteToString(String3, ListExpr3);
   cout << String1 << endl;
   cout << String2 << endl;
   cout << String3 << endl;

   cout << "After Destruction, Memory-Usage: " << nl.reportVectorSizes() << endl;

   return (0);
}

bool
openDB(string dbname ) {
   
   bool ok=false;   
   
   cout << "OpenDatabase " << dbname;
   if ( (ok=SmiEnvironment::OpenDatabase( dbname )) == true )
   {
      cout << " ok." << endl;
   }
   else
   {
      cout << " failed, try to create." << endl;
   
      cout << "CreateDatabase " << dbname; 
      if ( (ok=SmiEnvironment::CreateDatabase( dbname )) == true )
         cout << " ok." << endl;
      else
         cout << " failed." << endl;
   }
 
   return ok;
   
}

bool
closeDB()
{
    bool ok=false;   

    cout << "CloseDatabase "; 
    if ( (ok=SmiEnvironment::CloseDatabase()) == true )
      cout << " ok." << endl;
    else
      cout << " failed." << endl;

    return ok;
}

void
listDB() {
   
   string dbname;

   cout << "*** Start list of databases ***" << endl;
   while (SmiEnvironment::ListDatabases( dbname ))
   {
     cout << dbname << endl;
   }
   cout << "*** End list of databases ***" << endl;
 
}

int
TestRun_Persistent() {
  
   cout << endl << "Test run persistent" << endl;
 
   SmiError rc = 0;
   
   rc = SmiEnvironment::StartUp( SmiEnvironment::MultiUser,
   "SecondoConfig.ini", cerr );
   listDB();
   pause();
   
   //assert( openDB("PARRAY") ); 
   //cout << "Begin Transaction: " << SmiEnvironment::BeginTransaction() << endl;
   
   /*
   rf = new SmiRecordFile(true, 8000, true);
   if ( !(rf->Create()) ) {
   string errMsg;
   SmiError errCode = SmiEnvironment::GetLastErrorCode(errMsg);
     cerr << "SmiError: " << "SmiErrorCode " << errCode << " Msg: " << errMsg << endl;
   exit(0);
   } else {
     cout << "SmiRecordFile created!" << endl;
   }
   */
   //ConcatLists_bug();  
 
   //assert( closeDB() );
   //assert( openDB("PARRAY2") );   
   
   pause();
   TestBasicOperations();
   
   /*
   assert( rf->Close() );
   delete rf;      
   rf = 0;
   */  

   //exit(0); 
   cout << endl << "Test list copy function" << endl;
   
   pause();
   TestNLCopy();
   
   //cout << "Commit: " << SmiEnvironment::CommitTransaction() << endl;
   
   //assert( closeDB() );
   pause();
   
   rc = SmiEnvironment::ShutDown();
   cout << "ShutDown rc=" << rc << endl;
   
   return rc;
}


int
TestRun_MainMemory() {
   
   bool ok = true;
   
   pause();
   TestBasicOperations();
   
   //StringAtom_bug(); 
   
   pause();
   TestNLCopy();
   
   return ok;
}
   
} /* end of anonymous namespace */

int
main() {

#ifdef NL_PERSISTENT   
   return TestRun_Persistent();
#else
   return TestRun_MainMemory();
#endif

}


