/****************************************************/
/* File: analyze.c                                  */
/* Semantic analyzer implementation                 */
/* for the TINY compiler                            */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "analyze.h"


typedef void (* TraverseInvokeFun) (TreeNode *, ScopeList);

static void printRedefineError (char* name, int line) {
  fprintf(listing, "Error: Symbol \"%s\" is redefined at line %d\n", name, line);
}

static void printVoidVariableError (char* name, int line) {
  fprintf(listing, "Error: The void-type variable is declared at line %d (name : \"%s\")\n", line, name);
}

/* Procedure traverse is a generic recursive 
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc 
 * in postorder to tree pointed to by t
 */
static void traverse(TreeNode * t,
               ScopeList currentScope,
               TraverseInvokeFun preProc,
               TraverseInvokeFun postProc)
{
  if (t == NULL) { 
    return ;
  }

  preProc(t, currentScope);

  ScopeList prevScope = NULL;
  if (t->scope != NULL) {
    prevScope = currentScope;
    currentScope = t->scope;
  }

  for (int i=0; i < MAXCHILDREN; i++)
    traverse(t->child[i], currentScope, preProc,postProc);

  if (prevScope != NULL) {
    currentScope = prevScope;
  }

  postProc(t, currentScope);
  traverse(t->sibling, currentScope, preProc,postProc);
}

/* nullProc is a do-nothing procedure to 
 * generate preorder-only or postorder-only
 * traversals from traverse
 */
static void nullProc(TreeNode * t)
{ if (t==NULL) return;
  else return;
}


static void insertVariableSymbol (TreeNode *t, ScopeList scope) {
  BucketList sameNameSymbol = lookupScope(scope, t->attr.name, ALL_SYMBOL);

  if (sameNameSymbol != NULL) {
    // 현재 스코프에 같은 이름의 심볼이 있는 경우, redefine error가 발생. 
    printRedefineError(t->attr.name, t->lineno);
  } else if (t->type == Void || t->type == VoidArray) {
    // redefine이 아니면서 void 타입 변수를 사용하는 경우, void-type variable error가 발생.
    printVoidVariableError(t->attr.name, t->lineno);
  }

  // 이후 타입 체크를 위해 중복 정의더라도 Fun / Var 각 하나씩은 저장
  sameNameSymbol = lookupScope(scope, t->attr.name, ONLY_VAR_SYMBOL);
  if (sameNameSymbol == NULL) {
    insertSymbol(scope, t->attr.name, VarSymbol, t->type, t->lineno);
  }
}


static void insertFunctionSymbol (TreeNode *t, ScopeList scope) {
  BucketList sameNameSymbol = lookupScope(scope, t->attr.name, ALL_SYMBOL);

  if (sameNameSymbol != NULL) {
    // 현재 스코프에 같은 이름의 심볼이 있는 경우, redeclare error가 발생. 
    printRedefineError(t->attr.name, t->lineno);
  }

  // 이후 타입 체크를 위해 중복 정의더라도 Fun / Var 각 하나씩은 저장
  sameNameSymbol = lookupScope(scope, t->attr.name, ONLY_FUNC_SYMBOL);
  if (sameNameSymbol == NULL) {
    insertSymbol(scope, t->attr.name, FuncSymbol, t->type, t->lineno);
    // 파라미터 정보는 ParamK 노드에서 처리
  }
}


static void insertParamSymbol (TreeNode *t, ScopeList scope) {
  if (t->type == Void && t->attr.name == NULL) {
    // 파라미터가 없는 (void) 형태
    return ;
  }

  BucketList sameNameSymbol = lookupScope(scope, t->attr.name, ONLY_VAR_SYMBOL);

  if (sameNameSymbol != NULL) {
    // 현재 스코프에 같은 이름의 심볼이 있는 경우 = 같은 이름의 파라미터가 앞에 존재하는 경우
    printRedefineError(t->attr.name, t->lineno);
    return ;
  } else if (t->type == Void || t->type == VoidArray) {
    // 재정의가 아니면서, void 타입 변수를 쓰는 경우
    printVoidVariableError(t->attr.name, t->lineno);
  }

  // 함수 스코프 안에 변수 추가
  insertSymbol(scope, t->attr.name, VarSymbol, t->type, t->lineno);

  // 상위 스코프의 함수 심볼에 파라미터 타입 추가
  addParameterType(scope, t->type);
}

/* Procedure insertNode inserts 
 * identifiers stored in t into 
 * the symbol table 
 */
static void insertNode (TreeNode * t, ScopeList scope) {
  static int isNextCompoundFunctionBody = FALSE;

  // 선언으로부터 심볼 추가
  if (t->nodekind == DeclK) {
      switch (t->kind.decl) {
        case VarK: // 변수 선언
          insertVariableSymbol(t, scope);
          break;
        case FunK: // 함수 선언
          insertFunctionSymbol(t, scope);
          isNextCompoundFunctionBody = TRUE;
          break;
        case ParamK: // 파라미터
          insertParamSymbol(t, scope);
          break;
      }
  }

  // Function Declaration 노드인 경우, 새로운 스코프를 생성
  if (t->nodekind == DeclK && t->kind.decl == FunK) {
    t->scope = createLocalScope(t->attr.name, scope);
  }

  // Compound Statement면서, 부모가 Function Declaration 노드가 아닌 경우, 새로운 스코프를 생성
  if (t->nodekind == StmtK && t->kind.stmt == CompoundK) {
    if (!isNextCompoundFunctionBody) {
      t->scope = createLocalScope(NULL, scope);
    } else {
      isNextCompoundFunctionBody = FALSE;
    }
  }
}

static void printScopeOfNode (TreeNode *t, ScopeList scope) {
  if (t->scope != NULL) {
    printScope(listing, t->scope);
  }
}

/* Function buildSymtab constructs the symbol 
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode * syntaxTree) {
  syntaxTree->scope = createGlobalScope();

  traverse(syntaxTree, NULL, insertNode, nullProc);

  if (TraceAnalyze) {
    fprintf(listing,"\nSymbol table:\n\n");
    traverse(syntaxTree, NULL, printScopeOfNode, nullProc);
  }
}


static void typeError(TreeNode * t, char * message)
{ fprintf(listing,"Type error at line %d: %s\n",t->lineno,message);
  Error = TRUE;
}

/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode * t)
{/*  switch (t->nodekind)
  { case ExpK:
      switch (t->kind.exp)
      { case OpK:
          if ((t->child[0]->type != Integer) ||
              (t->child[1]->type != Integer))
            typeError(t,"Op applied to non-integer");
          if ((t->attr.op == EQ) || (t->attr.op == LT))
            t->type = Boolean;
          else
            t->type = Integer;
          break;
        case ConstK:
        case IdK:
          t->type = Integer;
          break;
        default:
          break;
      }
      break;
    case StmtK:
      switch (t->kind.stmt)
      { case IfK:
          if (t->child[0]->type == Integer)
            typeError(t->child[0],"if test is not Boolean");
          break;
        case AssignK:
          if (t->child[0]->type != Integer)
            typeError(t->child[0],"assignment of non-integer value");
          break;
        case WriteK:
          if (t->child[0]->type != Integer)
            typeError(t->child[0],"write of non-integer value");
          break;
        case RepeatK:
          if (t->child[1]->type == Integer)
            typeError(t->child[1],"repeat test is not Boolean");
          break;
        default:
          break;
      }
      break;
    default:
      break;

  } */
}

/* Procedure typeCheck performs type checking 
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode * syntaxTree)
{
  // traverse(syntaxTree,nullProc,checkNode);
}
