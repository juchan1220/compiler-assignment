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

static ParameterList createParameter (ExpType type) {
  ParameterList p = malloc(sizeof(struct Parameter));
  p->type = type;
  p->next = NULL;

  return p;
}


static LineList createLine (int lineno) {
  LineList line = malloc(sizeof(struct LineListRec));
  line->lineno = lineno;
  line->next = NULL;

  return line;
}


static BucketList createBucket (char* name, int lineno) {
  BucketList bucket = malloc(sizeof(struct BucketListRec));

  bucket->name = strdup(name);
  bucket->lines = createLine(lineno);
  bucket->next = NULL;

  return bucket;
}

static void insertBucket (ScopeList scope, BucketList bucket) {
  int hashed = hash(bucket->name);

  if (scope->bucket[hashed][1] == NULL) {
    scope->bucket[hashed][0] = scope->bucket[hashed][1] = bucket;
    bucket->next = NULL;
  } else {
    scope->bucket[hashed][1]->next = bucket;
    scope->bucket[hashed][1] = bucket;
  }
}


static ScopeList createScope (void) {
  ScopeList scope = malloc(sizeof(struct ScopeListRec));
  
  for (int i = 0; i < HASH_TABLE_SIZE; i++) {
    scope->bucket[i][0] = scope->bucket[i][1] = NULL;
  }

  scope->name = NULL;
  scope->parent = NULL;
  scope->locationCount = 0;

  return scope;
}

ScopeList createGlobalScope (void) {
  ScopeList scope = createScope();
  
  scope->name = strdup("global");

  {
    BucketList bucket = createBucket("input", 0);
    bucket->kind = FuncSymbol;
    bucket->type.funType.returnType = Integer;
    bucket->type.funType.params = NULL;

    bucket->memloc = scope->locationCount++;
    
    insertBucket(scope, bucket);
  }

  {
    BucketList bucket = createBucket("output", 0);
    bucket->kind = FuncSymbol;
    bucket->type.funType.returnType = Void;
    bucket->type.funType.params = createParameter(Integer);
    
    bucket->memloc = scope->locationCount++;

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


BucketList lookupFunctionOnGlobalWithLocation (ScopeList scope, char *name, int location) {
  while (scope->parent != NULL) {
    scope = scope->parent;
  }

  int h = hash(name);
  BucketList b = scope->bucket[h][0];

  while (b != NULL) {
    if (b->memloc == location) {
      break;
    }
    
    b = b->next;
  }
  
  return b;
}

BucketList lookupScope (ScopeList scope, char *name, int kindFlag) {
  int h = hash(name);
  BucketList b = scope->bucket[h][0];

  while (b != NULL) {
    if (strcmp(name, b->name) == 0 && (b->kind & kindFlag) != 0) {
      break;
    }
    
    b = b->next;
  }
  
  return b;
}

BucketList lookupScopeRecursive (ScopeList scope, char *name, int kindFlag) {
  while (scope != NULL) {
    BucketList b = lookupScope(scope, name, kindFlag);

    if (b != NULL) {
      return b;
    }

    scope = scope->parent;
  }
  
}

/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
BucketList insertSymbol(ScopeList scope, char* name, SymbolKind kind, ExpType type, int lineno) {
  BucketList l = createBucket(name, lineno);

    l->memloc = scope->locationCount++;
    l->kind = kind;

    if (kind == VarSymbol) {
      l->type.varType = type;
    } else {
      l->type.funType.returnType = type;
    }

    insertBucket(scope, l);

    return l;
}

void addReference(ScopeList scope, char* name, SymbolKind kind, int lineno) {
  // TODO: 
}


void addParameterType (ScopeList scope, ExpType type) {
  BucketList func = lookupFunctionOnGlobalWithLocation(scope, scope->name, scope->funcSymbolLocOnGlobal);
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
    if (scope->bucket[i][0] == NULL) {
      continue;
    }

    BucketList l = scope->bucket[i][0];

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


      if (l->kind == FuncSymbol) {
        ParameterList p = l->type.funType.params;
        fprintf(listing, "params: ");
        if (p == NULL) {
          fprintf(listing, "void\n");
        } else {
          ExpType type;

          while (p != NULL) {
            ExpType type = p->type;
            const char* typeString = NULL;

                    switch (type) {
          case Integer: typeString = "int"; break;
          case IntegerArray: typeString = "int[]"; break;
          case Void: typeString = "void"; break;
          case VoidArray: typeString = "void[]"; break;
        }

                fprintf(listing, "%s, ", typeString);
            p = p->next;
          }
          fprintf(listing, "\n");

        }
      }
      l = l->next;
    }
  }
  
} /* printSymTab */
