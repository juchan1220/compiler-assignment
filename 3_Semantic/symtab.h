/****************************************************/
/* File: symtab.h                                   */
/* Symbol table interface for the TINY compiler     */
/* (allows only one symbol table)                   */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _SYMTAB_H_
#define _SYMTAB_H_


#define HASH_TABLE_SIZE 211

typedef enum { FuncSymbol = 1, VarSymbol = 2 } SymbolKind;

const int ONLY_FUNC_SYMBOL;
const int ONLY_VAR_SYMBOL;
const int ALL_SYMBOL;

typedef struct LineListRec
   { int lineno;
     struct LineListRec * next;
   } * LineList;


typedef struct Parameter {
  ExpType type;
  struct Parameter* next;
} * ParameterList;


typedef struct FunctionType {
  ExpType returnType;
  ParameterList params;
} ;

typedef struct BucketListRec
   { char * name;
     LineList lines;
     int memloc ; /* memory location for variable */
     struct BucketListRec * next;
     SymbolKind kind;
     union { ExpType varType; struct FunctionType funType; } type;
   } * BucketList;


typedef struct ScopeListRec {
  char *name;
  BucketList bucket[HASH_TABLE_SIZE][2]; // [0] is first, [1] is last
  struct ScopeListRec *parent; // for symbol table hierarchy
  int locationCount;
  int funcSymbolLocOnGlobal;
} * ScopeList;


/**
 * @brief input(), output() 함수를 포함한 기본 global symbol table을 생성합니다.
 * 
 * @return ScopeList 
 */
ScopeList createGlobalScope (void);


/**
 * @brief 함수 스코프, nested 스코프의 symbol table을 생성합니다.
 * 
 * @param name 
 * @param parent 
 * @return ScopeList 
 */
ScopeList createLocalScope (char* name, ScopeList parent);

/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
BucketList insertSymbol(ScopeList scope, char* name, SymbolKind kind, ExpType type, int lineno);


void addParameterType (ScopeList scope, ExpType type);


/**
 * @brief 주어진 scope에서만 주어진 kindFlag, name에 해당하는 symbol을 lookup합니다.
 * 
 * @param scope 
 * @param name 
 * @return BucketList 없는 경우 NULL을 반환합니다.
 */
BucketList lookupScope (ScopeList scope, char *name, int kindFlag);

BucketList lookupScopeRecursive (ScopeList scope, char *name, int kindFlag);

BucketList lookupFunctionOnGlobalWithLocation (ScopeList scope, char *name, int location);

/* Procedure printSymTab prints a formatted 
 * listing of the symbol table contents 
 * to the listing file
 */
void printScope(FILE * listing, ScopeList scope);

#endif
