#ifndef NL_SCANNER_H
#define NL_SCANNER_H

#include "NestedList.h"

#ifndef yyFlexLexer
#define yyFlexLexer nlFlexLexer
#include <FlexLexer.h>
#endif

class NLScanner: public yyFlexLexer
{
 public:         
  enum Error
  {
    invalidInclude,
    circularInclusion,
    nestingTooDeep,
    cantRead,
  };
                
  NLScanner( NestedList* nestedList, istream* yyin = 0, ostream* yyout = 0 );
/*
  string const &lastFile()
  {
    return fileName.back();
  }
  void stackTrace();  // dumps filename stack to cerr
*/
  int yylex();        // overruling yyFlexLexer's yylex()
 private:
  NLScanner( NLScanner const &other );      // no Scanner copy-initialization
  NLScanner &operator=( NLScanner const &other );   // no assignment either

  NestedList* nl;
};

#endif

