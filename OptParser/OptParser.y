%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "OptSecUtils.h"

#define YYDEBUG 1
#define YYERROR_VERBOSE 1


#ifdef __cplusplus
extern "C"{
 int optlex();
 int opterror (const char *error);
 void opt_scan_string(const char* argument);
}
#endif



char* err_message;
bool success;


%}


%union {
 char* strval;
 int numval;
}


%name-prefix="opt"

%token SELECT FROM STAR ERROR
%token<strval> ID VARIABLE


%%

simplequery :  SELECT STAR FROM sources {
          err_message = 0;      
          success = true;
   }
;

sources:  ID {
     string dbname;
     if(!optutils::isDatabaseOpen(dbname)){
        opterror("no Database open, querying inpossible.");
        return false;
      } 
      char* relname = $1; 
      ListExpr type;
      string realname;
      if(!optutils::isObject(relname, realname, type)){
         string err = "Object " + string(relname) + " not known in the database " + dbname;
         opterror(err.c_str());
         return false;
      } 
      if(!optutils::isRelationDescription(type)){
         string err = "The object " + realname + " is not a relation.";
         return false;
       }
     }

  | VARIABLE {
     opterror("The name of a relation must start with a lower case letter");
     return false;
  }

;

%%


int opterror (const char *error)
{
  success=false;
  err_message = (char*)malloc(strlen(error)+1);
  strcpy(err_message, error);
  return 0;
}

extern "C"{void lexDestroy();}


bool checkOptimizerQuery(const char* argument, char*& errmsg){
    lexDestroy();
 
    opt_scan_string(argument);
    yyparse();
 
    if(success && err_message){
         free(err_message);
         err_message=0;
     }
     if(!success){
        errmsg = err_message;
        err_message = 0;
     }
     return success;
}






