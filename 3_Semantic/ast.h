#ifndef _AST_H_
#define _AST_H_

typedef int TokenType; 

typedef struct ScopeListRec* ScopeList;

/**************************************************/
/***********   Syntax tree for parsing ************/
/**************************************************/

typedef enum {StmtK,ExpK,DeclK,ListK} NodeKind;
typedef enum {CompoundK,SelectK,IterK,RetK,NopK} StmtKind;
typedef enum {AssignK,BinaryOpK,ConstK,IdK,CallK} ExpKind;
typedef enum {DeclListK,ParamListK,LocalDeclListK,StmtListK,ArgListK} ListKind;
typedef enum {FunK,VarK,ParamK} DeclKind;

/* ExpType is used for type checking */
typedef enum {Void,Integer,VoidArray,IntegerArray,Unknown} ExpType;

#define MAXCHILDREN 3

typedef struct treeNode
   { struct treeNode * child[MAXCHILDREN];
     struct treeNode * sibling;
     int lineno;
     NodeKind nodekind;
     union { StmtKind stmt; ExpKind exp; DeclKind decl; ListKind list; } kind;
     union { TokenType op;
             int val;
             char * name;
             int has_else;
             struct treeNode* lastChildOfList;
             } attr;
     ExpType type; /* for type checking of exps */
     ScopeList scope;
   } TreeNode;

#endif