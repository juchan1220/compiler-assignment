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

static void printUndeclaredVariableError (char *name, int line) {
  fprintf(listing, "Error: Undeclared variable \"%s\" is used at line %d\n", name, line);
}

static void printUndeclaredFunctionError (char *name, int line) {
  fprintf(listing, "Error: Undeclared function \"%s\" is called at line %d\n", name, line);
}

static void printNonIntegerIndexError (char *name, int line) {
  fprintf(listing, "Error: Invalid array indexing at line %d (name : \"%s\"). Indicies should be integer\n", line, name);
}

static void printNonArrayIndexingError (char *name, int line) {
  fprintf(listing, "Error: Invalid array indexing at line %d (name : \"%s\"). Indexing can only be allowed for int[] variables\n", line, name);
}

static void printInvalidFunctionCall (char *name, int line) {
  fprintf(listing, "Error: Invalid function call at line %d (name : \"%s\")\n", line, name);
}

static void printInvalidOperation (int line) {
  fprintf(listing, "Error: Invalid operation at line %d\n", line);
}

static void printInvalidAssignment (int line) {
  fprintf(listing, "Error: Invalid assignment at line %d\n", line);
}

static void printInvalidCondition (int line) {
  fprintf(listing, "Error: Invalid condition at line %d\n", line); 
}

static void printInvalidReturn (int line) {
  fprintf(listing, "Error: Invalid return at line %d\n", line);
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
    traverse(t->child[i], currentScope, preProc, postProc);

  if (prevScope != NULL) {
    currentScope = prevScope;
  }

  postProc(t, currentScope);
  traverse(t->sibling, currentScope, preProc, postProc);
}

/* nullProc is a do-nothing procedure to 
 * generate preorder-only or postorder-only
 * traversals from traverse
 */
static void nullProc(TreeNode * t, ScopeList scope)
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


static int insertFunctionSymbol (TreeNode *t, ScopeList scope) {
  BucketList sameNameSymbol = lookupScope(scope, t->attr.name, ALL_SYMBOL);

  if (sameNameSymbol != NULL) {
    // 현재 스코프에 같은 이름의 심볼이 있는 경우, redeclare error가 발생. 
    printRedefineError(t->attr.name, t->lineno);
  }

  // 타입 체크를 위해 재정의라도 무조건 symbol에 추가해야함
  return insertSymbol(scope, t->attr.name, FuncSymbol, t->type, t->lineno)->memloc;
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
  int funcLocation = -1;

  // TODO: 심볼 추가 시에 input, output 이름은 항상 걸러야 하는가?
  // 선언으로부터 심볼 추가
  if (t->nodekind == DeclK) {
      switch (t->kind.decl) {
        case VarK: // 변수 선언
          insertVariableSymbol(t, scope);
          break;
        case FunK: // 함수 선언
          funcLocation = insertFunctionSymbol(t, scope);
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
    t->scope->funcSymbolLocOnGlobal = funcLocation;
  }

  // Compound Statement면서, 부모가 Function Declaration 노드가 아닌 경우, 새로운 스코프를 생성
  if (t->nodekind == StmtK && t->kind.stmt == CompoundK) {
    if (!isNextCompoundFunctionBody) {
      t->scope = createLocalScope(scope->name, scope);
      t->scope->funcSymbolLocOnGlobal = scope->funcSymbolLocOnGlobal;
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

static void typeCheckSingleIdExpr (TreeNode *node, ScopeList scope) {
  BucketList symbol = lookupScopeRecursive(scope, node->attr.name, ONLY_VAR_SYMBOL);

  if (symbol == NULL) {
    printUndeclaredVariableError(node->attr.name, node->lineno);
    node->type = Unknown;
  } else {
    node->type = symbol->type.varType;
  }
}

static void typeCheckArrayRefIdExpr (TreeNode *node, ScopeList scope) {
  BucketList symbol = lookupScopeRecursive(scope, node->attr.name, ONLY_VAR_SYMBOL);
  node->type = Integer;

  // symbol 정의 여부 확인
  if (symbol == NULL) {
    printUndeclaredVariableError(node->attr.name, node->lineno);
    node->type = Unknown;
  } else if (symbol->type.varType != IntegerArray) {
    // symbol 타입 확인
    printNonArrayIndexingError(node->attr.name, node->lineno);
    node->type = Unknown;
  }

  // index 타입 확인
  if (node->child[0]->type != Integer) {
    printNonIntegerIndexError(node->attr.name, node->lineno);
  }

}

static void typeCheckAssignment (TreeNode *node, ScopeList scope) {
  // TODO: Integer 가 아닌 타입 간의 assignment는 허용인가?
  if (node->child[0]->type != Integer || node->child[1]->type != Integer) {
    printInvalidAssignment(node->lineno);
    node->type = Unknown;
  } else {
    node->type = node->child[0]->type;
  }
}

static void typeCheckBinaryOp (TreeNode *node, ScopeList scope) {
  if (node->child[0]->type != Integer || node->child[1]->type != Integer) {
    printInvalidOperation(node->lineno);
    node->type = Unknown;
  } else {
    node->type = Integer;
  }
}

static void typeCheckCall (TreeNode *node, ScopeList scope) {
  BucketList symbol = lookupScopeRecursive(scope, node->attr.name, ONLY_FUNC_SYMBOL);

  if (symbol == NULL) {
    printUndeclaredFunctionError(node->attr.name, node->lineno);
    node->type = Unknown;
    return ;
  }

  ParameterList p = symbol->type.funType.params;
  TreeNode* arg = NULL;

  if (node->child[0] != NULL) {
    arg = node->child[0]->child[0];
  }

  // 파라미터가 아예 없으면 void 타입 -> NULL
  // 리스트가 아예 없으면 void 타입 -> NULL
  while (p != NULL && arg != NULL) {
    if (p->type != arg->type) {
      break;
    }

    p = p->next;
    arg = arg->sibling;
  }

  if (p != NULL || arg != NULL) {
    printInvalidFunctionCall(node->attr.name, node->lineno);
  }

  node->type = symbol->type.funType.returnType;
}

static void typeCheckSelectStmt (TreeNode* node, ScopeList scope) {
  if (node->child[0]->type != Integer) {
    printInvalidCondition(node->child[0]->lineno);
  }
}

static void typeCheckIterStmt (TreeNode *node, ScopeList scope) {
  if (node->child[0]->type != Integer) {
    printInvalidCondition(node->child[0]->lineno);
  }
}

static void typeCheckRetStmt (TreeNode *node, ScopeList scope) {
  BucketList symbol = lookupFunctionOnGlobalWithLocation(scope, scope->name, scope->funcSymbolLocOnGlobal);
  assert(symbol != NULL && symbol->kind == FuncSymbol);

  if (node->child[0] == NULL) {
    if (symbol->type.funType.returnType != Void) {
      printInvalidReturn(node->lineno);
    }
  } else if (node->child[0]->type != symbol->type.funType.returnType) {
    printInvalidReturn(node->lineno);
  }
}

/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode * t, ScopeList scope) {

  switch (t->nodekind) {
    case ExpK:
      switch (t->kind.exp) {
        case IdK:
          if (t->child[0] == NULL) {
            typeCheckSingleIdExpr(t, scope);
          } else {
            typeCheckArrayRefIdExpr(t, scope);
          }
          break;
        case AssignK:
          typeCheckAssignment(t, scope);
          break;
        case ConstK:
          t->type = Integer;
          break;
        case BinaryOpK:
          typeCheckBinaryOp(t, scope);
          break;
        case CallK:
          typeCheckCall(t, scope);
          break;
      }
      break;
    case StmtK:
      switch (t->kind.stmt) {
        case SelectK:
          typeCheckSelectStmt(t, scope);
          break;
        case IterK:
          typeCheckIterStmt(t, scope);
          break;
        case RetK:
          typeCheckRetStmt(t, scope);
          break;
      }
      break;
    default:
      break;
  }
}

/* Procedure typeCheck performs type checking 
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode * syntaxTree)
{
  // traverse(syntaxTree,nullProc,checkNode);
}

/* Function buildSymtab constructs the symbol 
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode * syntaxTree) {
  syntaxTree->scope = createGlobalScope();

  traverse(syntaxTree, NULL, insertNode, checkNode);

  if (TraceAnalyze) {
    fprintf(listing,"\nSymbol table:\n\n");
    traverse(syntaxTree, NULL, printScopeOfNode, nullProc);
  }
}