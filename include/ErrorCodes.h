/*

15.09.2004, M. Spiekermann. Creation 

This file was created to start with a better handling of error codes.
Functions returning just an integer which is interpreted as an special
case of error should be replaced by returning a typed error constant.

The error codes of the SMI are also defined here. The file SmiCodes.h only
includes this file. In the future there should be only one file (this file)
which defines all error codes.  

*/

#ifndef SECONDO_ERRORCODES_H
#define SECONDO_ERRORCODES_H

/*
1 SecondoInterface Error Codes

The following codes reflect error condition which can occur
during a call of the SecondoInterface.

*/
typedef int SI_Error;

const SI_Error ERR_NO_ERROR = 0;
const SI_Error ERR_CMD_NOT_RECOGNIZED = 1;
const SI_Error ERR_IN_QUERY_EXPR = 2;
const SI_Error ERR_EXPR_NOT_EVALUABLE = 3;
const SI_Error ERR_NO_OBJ_CREATED = 4;
const SI_Error ERR_NO_TYPE_DEFINED = 5;
const SI_Error ERR_NO_DATABASE_OPEN = 6;
const SI_Error ERR_DATABASE_OPEN = 7;
const SI_Error ERR_UNDEF_OBJ_VALUE = 8;
const SI_Error ERR_SYNTAX_ERROR = 9;

const SI_Error ERR_IDENT_USED = 10;
const SI_Error ERR_IDENT_UNKNOWN_TYPE = 11;
const SI_Error ERR_IDENT_UNKNOWN_OBJ = 12;
const SI_Error ERR_EXPR_TYPE_NEQ_OBJ_TYPE = 13;
const SI_Error ERR_TYPE_NAME_USED_BY_OBJ = 14;
const SI_Error ERR_IDENT_RESERVED = 15;
const SI_Error ERR_UPDATE_FOR_DERIVED_OBJ_UNSUPPORTED = 16;

const SI_Error ERR_TRANSACTION_ACTIVE = 20; 
const SI_Error ERR_NO_TRANSACTION_ACTIVE = 21; 
const SI_Error ERR_BEGIN_TRANSACTION_FAILED = 22; 
const SI_Error ERR_COMMIT_OR_ABORT_FAILED = 23;
const SI_Error ERR_IN_DEFINITIONS_FILE = 24;
const SI_Error ERR_IDENT_UNKNOWN_DB_NAME = 25;
const SI_Error ERR_PROBLEM_IN_WRITING_TO_FILE = 26;
const SI_Error ERR_DB_NAME_NEQ_IDENT = 27;
const SI_Error ERR_PROBLEM_IN_READING_FILE = 28;
const SI_Error ERR_IN_LIST_STRUCTURE_IN_FILE = 29;

const SI_Error ERR_CMD_NOT_YET_IMPL = 30;
const SI_Error ERR_CMD_LEVEL_NOT_YET_IMPL = 31;
const SI_Error ERR_CMD_NOT_IMPL_AT_THIS_LEVEL = 32;

const SI_Error ERR_IN_TYPE_DEFINITION = 40;  
const SI_Error ERR_NAME_DOUBLY_DEFINED = 41;
const SI_Error ERR_IN_TYPE_EXPRESSION = 42;
 
const SI_Error ERR_IN_OBJ_DEFINITION = 50;
const SI_Error ERR_OBJ_NAME_DOUBLY_DEFINED = 51;
const SI_Error ERR_WRONG_TYPE_EXPR_FOR_OBJ = 52;
const SI_Error ERR_WRONG_LIST_REP_FOR_OBJ = 53;

const SI_Error ERR_KIND_DOES_NOT_MATCH_TYPE_EXPR = 60;
const SI_Error ERR_SPECIFIC_KIND_CHECKING_ERROR = 61;

const SI_Error ERR_IN_VALUELIST_TC_V = 70;
const SI_Error ERR_SPECIFIC_FOR_TYPE_CONSTRUCTOR = 71;
const SI_Error ERR_IN_VALUELIST_TC = 72;
const SI_Error ERR_AT_POS_IN_VALUELIST = 73;

const SI_Error ERR_IN_SECONDO_PROTOCOL = 80;
const SI_Error ERR_CONNECTION_TO_SERVER_LOST = 81;
const SI_Error ERR_IDENT_UNKNOWN_DB_OBJECT = 82;
const SI_Error ERR_OBJ_NAME_IN_FILE_NEQ_IDENT = 83;
const SI_Error ERR_IDENT_ALREADY_KNOWN_IN_DB = 84;
const SI_Error ERR_ALGEBRA_UNKNOWN = 85;

const SI_Error ERR_UNKNOWN_RETURN_CODE = -1;


/*
2 SMI Error Codes

*/

typedef long SmiError;

const SmiError E_SMI_OK                = 0;
const SmiError E_SMI_STARTUP           = -1001;
const SmiError E_SMI_SHUTDOWN          = -1002;
const SmiError E_SMI_DB_CREATE         = -1003;
const SmiError E_SMI_DB_OPEN           = -1004;
const SmiError E_SMI_DB_CLOSE          = -1005;
const SmiError E_SMI_DB_ERASE          = -1006;
const SmiError E_SMI_DB_NOTOPEN        = -1007;
const SmiError E_SMI_DB_NOTCLOSED      = -1008;
const SmiError E_SMI_DB_INVALIDNAME    = -1009;
const SmiError E_SMI_DB_EXISTING       = -1010;
const SmiError E_SMI_DB_NOTEXISTING    = -1011;
const SmiError E_SMI_DB_LOOKUP         = -1012;
const SmiError E_SMI_DB_INSERT_CATALOG = -1013;
const SmiError E_SMI_DB_DELETE_CATALOG = -1014;
const SmiError E_SMI_DB_UPDATE_CATALOG = -1015;
const SmiError E_SMI_DB_NOTFOUND       = -1016;
const SmiError E_SMI_DB_NOTLOCKED      = -1017;
const SmiError E_SMI_DB_REGISTER       = -1018;
const SmiError E_SMI_DB_UNREGISTER     = -1019;
const SmiError E_SMI_DB_LOCK           = -1020;
const SmiError E_SMI_DB_UNLOCK         = -1021;

const SmiError E_SMI_TXN_BEGIN         = -1051;
const SmiError E_SMI_TXN_COMMIT        = -1052;
const SmiError E_SMI_TXN_ABORT         = -1053;
const SmiError E_SMI_TXN_RUNNING       = -1054;
const SmiError E_SMI_TXN_NOTRUNNING    = -1055;

const SmiError E_SMI_CATALOG_LOOKUP    = -1101;
const SmiError E_SMI_CATALOG_INSERT    = -1102;
const SmiError E_SMI_CATALOG_DELETE    = -1103;
const SmiError E_SMI_CATALOG_KEYEXIST  = -1104;
const SmiError E_SMI_CATALOG_NOTFOUND  = -1105;

const SmiError E_SMI_FILE_INVALIDNAME  = -1201;
const SmiError E_SMI_FILE_NOFILEID     = -1202;
const SmiError E_SMI_FILE_BADCONTEXT   = -1203;
const SmiError E_SMI_FILE_CREATE       = -1204;
const SmiError E_SMI_FILE_OPEN         = -1205;
const SmiError E_SMI_FILE_CLOSE        = -1206;
const SmiError E_SMI_FILE_KEYEXIST     = -1207;
const SmiError E_SMI_FILE_ISTEMP       = -1208;

const SmiError E_SMI_RECORD_NOTINIT    = -1251;
const SmiError E_SMI_RECORD_READ       = -1252;
const SmiError E_SMI_RECORD_WRITE      = -1253;
const SmiError E_SMI_RECORD_SELECT     = -1254;
const SmiError E_SMI_RECORD_APPEND     = -1255;
const SmiError E_SMI_RECORD_DELETE     = -1256;
const SmiError E_SMI_RECORD_SELECTALL  = -1257;
const SmiError E_SMI_RECORD_INSERT     = -1258;
const SmiError E_SMI_RECORD_TRUNCATE   = -1259;
const SmiError E_SMI_RECORD_READONLY   = -1260;
const SmiError E_SMI_RECORD_FINISH     = -1261;

const SmiError E_SMI_CURSOR_NOTOPEN    = -1271;
const SmiError E_SMI_CURSOR_NEXT       = -1272;
const SmiError E_SMI_CURSOR_ENDOFSCAN  = -1273;
const SmiError E_SMI_CURSOR_DELETE     = -1274;
const SmiError E_SMI_CURSOR_FINISH     = -1275;

#endif
