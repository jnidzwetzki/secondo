/*
//paragraph	[23]	table3columns:	[\begin{quote}\begin{tabular}{lll}]	[\end{tabular}\end{quote}]
//[--------]	[\hline]
//characters	[1]	verbatim:	[\verb|]	[|]
//[ae] [\"a]
//[oe] [\"o]
//[ue] [\"u]
//[ss] [{\ss}]
//[<=] [$\leq$]
//[#]  [\neq]
//[tilde] [\verb|~|]

1 Header File Counter.h 

December 2003 M. Spiekermann 

August 2004 M. Spiekermann, Function ~resetAll~ introduced.

Nove 2004 M. Spiekermann. The counter values will additonally be
saved in CSV format in a file called "cmd-counters.csv".

1.1 Overview

The class ~Counter~ provides a mechanism for registering counters with an
identifier. It was implemented as dynamic data structure using "std::map"[1].
Counters are created with the first call of "Counter::getRef" which returns a
reference to a variable of type "long" initialized to 0.  A counter can be
incremented or modified with 

----Counter::getRef("MyCounterName")++;
    Counter::getRef("MyCounterName")+=500;
----

All counters used are reported when the ~Application~ object is destructed or
after every query command if the RTFlag "SI:PrintCounters"[1] was set up in
"SecondoConfig.ini". Please choose long unique identifier strings to avoid that
two different programmers use the same identifier for a counter. For reasons of
simplicity there is no check for uniqueness otherwise a ~create~ function must
be introduced and called at a suitable position. 

*/


#ifndef CLASS_COUNTER_H
#define CLASS_COUNTER_H

#include <map>
#include <string>
#include <iostream>

#include "LogMsg.h"

using namespace std;

class Counter {

public:

  Counter(){};
  ~Counter(){};

  static long& getRef( const string& identifier ) { 
    
    if ( (it=CounterMap.find( identifier )) != CounterMap.end() ) { 
      return it->second; 
  
    } else { 
      // initialize new Counter;
      long& CounterRef = CounterMap[identifier];
      CounterRef = 0;
      return CounterRef;
    }
  };
	
	
	static void resetAll() {
	 
	   for ( it = CounterMap.begin(); it != CounterMap.end(); it++ ) {
		   it->second = 0;
		}
	}

  static void reportValues() {

    static int nr = 0;
    static size_t mapsize = 0; 
    const string sep(60,'_');
    const string csvsep="|";
    ostream& clog = cmsg.file("cmd-counters.csv");  

    if ( mapsize != CounterMap.size() ) { // this implies CounterMap > 0
      
      it=CounterMap.begin();
      clog << it->first;
      for ( it++; it!=CounterMap.end(); it++) {
       clog << csvsep << it->first;
      }
      clog << endl;
      cmsg.send();
      mapsize = CounterMap.size();
    }

    cout << sep << endl << "Counter Values ...  ";
    clog << ++nr;
    for (it=CounterMap.begin(); it!=CounterMap.end(); it++) {

       cout << it->first << "=" << it->second << ", ";
       clog << csvsep << it->second;
    }
    cout << endl << sep << endl << endl;
    clog << endl;
    cmsg.send();

  };

private:

  static map<string,long> CounterMap;
  static map<string,long>::iterator it;

};

#endif
