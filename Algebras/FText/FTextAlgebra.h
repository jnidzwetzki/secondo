/*
//paragraph [1] Title: [{\Large \bf \begin {center}] [\end {center}}]

[1] FText Algebra

March - April 2003 Lothar Sowada

The algebra ~FText~ provides the type constructor ~text~ and two operators:

(i) ~contains~, which search text or string in a text.

(ii) ~length~ which give back the length of a text.

*/

#ifndef __F_TEXT_ALGEBRA__
#define __F_TEXT_ALGEBRA__

#include <iostream>

#include "StandardAttribute.h"
#include "FLOB.h"

class FText: public StandardAttribute
{
public:

  FText();
  FText(bool newDefined, const char *newText = NULL);
  FText(FText&);
  ~FText();
  void Destroy();

  bool  SearchString( const char* subString );
  void  Set( const char *newString );
  void  Set( bool newDefined, const char *newString );
  int TextLength() const;
  const char *Get();

/*************************************************************************

  The following virtual functions:
  IsDefined, SetDefined, HashValue, CopyFrom, Compare, Sizeof, Clone, Print, Adjacent
  need to be defined if we want to use ~text~ as an attribute type in tuple definitions.

*************************************************************************/

  bool     IsDefined() const;
  void     SetDefined(bool newDefined);
  size_t   HashValue();
  void     CopyFrom(StandardAttribute* right);
  int      Compare(Attribute * arg);
  int      Sizeof() const;
  FText*   Clone();
  ostream& Print(ostream &os);
  bool     Adjacent(Attribute * arg);

  int NumOfFLOBs();
  FLOB* GetFLOB( const int );

private:
  FLOB theText;
  bool defined;
};

#endif 

