/****************************************************/
/* File: symtab.c                                   */
/* Symbol table implementation for the TINY compiler*/
/* (allows only one symbol table)                   */
/* Symbol table is implemented as a chained         */
/* hash table                                       */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"
#include "symtab.h"


/* SHIFT is the power of two used as multiplier
   in hash function  */
#define SHIFT 4


const int ONLY_FUNC_SYMBOL = FuncSymbol;
const int ONLY_VAR_SYMBOL = VarSymbol;
const int ALL_SYMBOL = FuncSymbol | VarSymbol;

/* the hash function */
static int hash ( char * key )
{ int temp = 0;
  int i = 0;
  while (key[i] != '\0')
  { temp = ((temp << SHIFT) + key[i]) % HASH_TABLE_SIZE;
    ++i;
  }
  return temp;
}

/**
 * @brief Create a Parameter object
 * 
 * @param type 파라미터의 타입
 * @return ParameterList
 */
static ParameterList createParameter (ExpType type) {
  ParameterList p = malloc(sizeof(struct Parameter));
  p->type = type;
  p->next = NULL;

  return p;
}


/* the list of line numbers of the source 
 * code in which a variable is referenced
 */

static LineList createLine (int lineno) {
  LineList line = malloc(sizeof(struct LineListRec));
  line->lineno = lineno;
  line->next = NULL;

  return line;
}


/* The record in the bucket lists for
 * each variable, including name, 
 * assigned memory location, and
 * the list of line numbers in which
 * it appears in the source code
 */



static BucketList createBucket (char* name, int lineno) {
  BucketList bucket = malloc(sizeof(struct BucketListRec));

  bucket->name = strdup(name);
  bucket->lines = createLine(lineno);
  bucket->next = NULL;

  return bucket;
}

static ScopeList createScope (void) {
  ScopeList scope = malloc(sizeof(struct ScopeListRec));
  
  for (int i = 0; i < HASH_TABLE_SIZE; i++) {
    scope->bucket[i] = NULL;
  }

  scope->name = NULL;
  scope->parent = NULL;
  scope->location_count = 0;

  return scope;
}

static void insertBucket (ScopeList scope, BucketList bucket) {
  int hashed = hash(bucket->name);

  bucket->next = scope->bucket[hashed];
  scope->bucket[hashed] = bucket;
}

static BucketList getBucket (ScopeList scope, char* name) {
  int hashed = hash(name);
  BucketList iter = scope->bucket[hashed];

  while (iter != NULL && strcpy(iter->name, name) != 0) {
    iter = iter->next;
  }

  assert(iter != NULL);

  return iter;
}

ScopeList createGlobalScope (void) {
  ScopeList scope = createScope();
  
  scope->name = strdup("global");

  {
    BucketList bucket = createBucket("input", 0);
    bucket->kind = FuncSymbol;
    bucket->type.funType.returnType = Integer;
    bucket->type.funType.params = createParameter(Void);

    bucket->memloc = scope->location_count++;
    
    insertBucket(scope, bucket);
  }

  {
    BucketList bucket = createBucket("output", 0);
    bucket->kind = FuncSymbol;
    bucket->type.funType.returnType = Void;
    bucket->type.funType.params = createParameter(Integer);
    
    bucket->memloc = scope->location_count++;

    insertBucket(scope, bucket);
  }

  return scope;
}

ScopeList createLocalScope (char* name, ScopeList parent) {
  ScopeList scope = createScope();
  scope->name = name;
  scope->parent = parent;

  return scope;
}

/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
void insertSymbol(ScopeList scope, char* name, SymbolKind kind, ExpType type, int lineno) {
  int h = hash(name);
  BucketList l = scope->bucket[h];

  while ((l != NULL) && (strcmp(name, l->name) != 0))
    l = l->next;
  
  if (l == NULL) {
    /* variable not yet in table */ 
    l = createBucket(name, lineno);
    l->memloc = scope->location_count++;
    l->next = scope->bucket[h];
    l->kind = kind;

    if (kind == VarSymbol) {
      l->type.varType = type;
    } else {
      l->type.funType.returnType = type;
    }
  
    scope->bucket[h] = l;
  } else {
    /* found in table, so just add line number */
    LineList t = l->lines;

    while (t->next != NULL) t = t->next;
    t->next = createLine(lineno);
  }
}


void addParameterType (ScopeList scope, ExpType type) {
  assert(scope->parent != NULL);
  
  BucketList func = lookupScope(scope->parent, scope->name, ONLY_FUNC_SYMBOL);

  assert(func != NULL);

  ParameterList iter = func->type.funType.params;
  
  if (iter == NULL) {
    func->type.funType.params = createParameter(type);
  } else {
    while (iter->next != NULL) {
      iter = iter->next;
    }

    iter->next = createParameter(type);
  }
}



BucketList lookupScopeRecursive (ScopeList scope) {

}

BucketList lookupScope (ScopeList scope, char *name, int kindFlag) {
  int h = hash(name);
  BucketList b = scope->bucket[h];

  while (b != NULL) {
    if (strcmp(name, b->name) == 0 && (b->kind & kindFlag) != 0) {
      break;
    }
    
    b = b->next;
  }
  
  return b;
}

/* Procedure printSymTab prints a formatted 
 * listing of the symbol table contents 
 * to the listing file
 */
void printScope(FILE * listing, ScopeList scope)
{ int i;

  fprintf(listing, "Scope: %s\n", scope->name ? scope->name : "");
  fprintf(listing,"Symbol Name    Location  Type        Line Numbers\n");
  fprintf(listing,"-------------  --------  ---------   ------------\n");

  for (i=0;i<HASH_TABLE_SIZE;++i) {
    if (scope->bucket[i] == NULL) {
      continue;
    }

    BucketList l = scope->bucket[i];

    while (l != NULL) {
      LineList t = l->lines;
      fprintf(listing,"%-14s ",l->name);
      fprintf(listing,"%-8d  ",l->memloc);
      {
        ExpType type;
        const char* typeString = NULL;

        if (l->kind == VarSymbol) {
          type = l->type.varType;
        } else {
          type = l->type.funType.returnType;
        }
        
        switch (type) {
          case Integer: typeString = "int"; break;
          case IntegerArray: typeString = "int[]"; break;
          case Void: typeString = "void"; break;
          case VoidArray: typeString = "void[]"; break;
        }

        if (l->kind == FuncSymbol) {
          fprintf(listing, "-> %-6s", typeString);
        } else {
          fprintf(listing,"%-9s", typeString);
        }
      }
      while (t != NULL)
      { fprintf(listing,"%4d ",t->lineno);
        t = t->next;
      }
      fprintf(listing,"\n");
      l = l->next;
    }
  }
  
} /* printSymTab */
