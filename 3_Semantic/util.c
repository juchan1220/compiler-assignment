/****************************************************/
/* File: util.c                                     */
/* Utility function implementation                  */
/* for the TINY compiler                            */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "util.h"

/* Procedure printToken prints a token
 * and its lexeme to the listing file
 */
void printToken(TokenType token, const char *tokenString)
{
  switch (token)
  {
  case IF:
  case ELSE:
  case WHILE:
  case RETURN:
  case INT:
  case VOID:
    fprintf(listing,
            "reserved word: %s\n", tokenString);
    break;
  case ASSIGN:
    fprintf(listing, "=\n");
    break;
  case EQ:
    fprintf(listing, "==\n");
    break;
  case NE:
    fprintf(listing, "!=\n");
    break;
  case LT:
    fprintf(listing, "<\n");
    break;
  case LE:
    fprintf(listing, "<=\n");
    break;
  case GT:
    fprintf(listing, ">\n");
    break;
  case GE:
    fprintf(listing, ">=\n");
    break;
  case PLUS:
    fprintf(listing, "+\n");
    break;
  case MINUS:
    fprintf(listing, "-\n");
    break;
  case TIMES:
    fprintf(listing, "*\n");
    break;
  case OVER:
    fprintf(listing, "/\n");
    break;
  case LPAREN:
    fprintf(listing, "(\n");
    break;
  case RPAREN:
    fprintf(listing, ")\n");
    break;
  case LBRACE:
    fprintf(listing, "[\n");
    break;
  case RBRACE:
    fprintf(listing, "]\n");
    break;
  case LCURLY:
    fprintf(listing, "{\n");
    break;
  case RCURLY:
    fprintf(listing, "}\n");
    break;
  case SEMI:
    fprintf(listing, ";\n");
    break;
  case COMMA:
    fprintf(listing, ",\n");
    break;
  case ENDFILE:
    fprintf(listing, "EOF\n");
    break;
  case NUM:
    fprintf(listing,
            "NUM, val= %s\n", tokenString);
    break;
  case ID:
    fprintf(listing,
            "ID, name= %s\n", tokenString);
    break;
  case ERROR:
    fprintf(listing,
            "ERROR: %s\n", tokenString);
    break;
  default: /* should never happen */
    fprintf(listing, "Unknown token: %d\n", token);
  }
}

/* Function newStmtNode creates a new statement
 * node for syntax tree construction
 */
TreeNode *newStmtNode(StmtKind kind)
{
  TreeNode *t = (TreeNode *)malloc(sizeof(TreeNode));
  int i;
  if (t == NULL)
    fprintf(listing, "Out of memory error at line %d\n", lineno);
  else
  {
    for (i = 0; i < MAXCHILDREN; i++)
      t->child[i] = NULL;
    t->sibling = NULL;
    t->nodekind = StmtK;
    t->kind.stmt = kind;
    t->lineno = lineno;
    t->scope = NULL;
  }
  return t;
}

/* Function newExpNode creates a new expression
 * node for syntax tree construction
 */
TreeNode *newExpNode(ExpKind kind)
{
  TreeNode *t = (TreeNode *)malloc(sizeof(TreeNode));
  int i;
  if (t == NULL)
    fprintf(listing, "Out of memory error at line %d\n", lineno);
  else
  {
    for (i = 0; i < MAXCHILDREN; i++)
      t->child[i] = NULL;
    t->sibling = NULL;
    t->nodekind = ExpK;
    t->kind.exp = kind;
    t->lineno = lineno;
    t->type = Void;
    t->scope = NULL;
  }
  return t;
}


TreeNode *newDeclNode(DeclKind kind) {
  TreeNode *t = (TreeNode *)malloc(sizeof(TreeNode));
  int i;

  if (t == NULL) {
    fprintf(listing, "Out of memory error at line %d\n", lineno);
  } else {
    for (i = 0; i < MAXCHILDREN; i++) {
      t->child[i] = NULL;
    }

    t->sibling = NULL;
    t->nodekind = DeclK;
    t->kind.decl = kind;
    t->lineno = lineno;
    t->type = Void;
    t->scope = NULL;
  }

  return t;
}


TreeNode *newListNode(ListKind kind) {
  TreeNode *t = (TreeNode *)malloc(sizeof(TreeNode));
  int i;

  if (t == NULL) {
    fprintf(listing, "Out of memory error at line %d\n", lineno);
  } else {
    for (i = 0; i < MAXCHILDREN; i++) {
      t->child[i] = NULL;
    }

    t->sibling = NULL;
    t->nodekind = ListK;
    t->kind.decl = kind;
    t->lineno = lineno;
    t->scope = NULL;
  }

  return t;
}

/* Function copyString allocates and makes a new
 * copy of an existing string
 */
char *copyString(char *s)
{
  int n;
  char *t;
  if (s == NULL)
    return NULL;
  n = strlen(s) + 1;
  t = malloc(n);
  if (t == NULL)
    fprintf(listing, "Out of memory error at line %d\n", lineno);
  else
    strcpy(t, s);
  return t;
}

/* Variable indentno is used by printTree to
 * store current number of spaces to indent
 */
static indentno = 0;

/* macros to increase/decrease indentation */
#define INDENT indentno += 2
#define UNINDENT indentno -= 2

/* printSpaces indents by printing spaces */
static void printSpaces(void)
{
  int i;
  for (i = 0; i < indentno; i++)
    fprintf(listing, " ");
}

static void printTypes(ExpType t) {
  switch (t) {
    case Integer:
      fprintf(listing, "int");
      break;
    case Void:
      fprintf(listing, "void");
      break;
    case IntegerArray:
      fprintf(listing, "int[]");
      break;
    case VoidArray:
      fprintf(listing, "void[]");
      break;
    default:
      fprintf(listing, "Unknown Type\n");
      break;
  }
}

/* procedure printTree prints a syntax tree to the
 * listing file using indentation to indicate subtrees
 */
void printTree(TreeNode *tree)
{
  int i;
  int indented;

  indented = FALSE;
  if (tree != NULL && tree->nodekind != ListK) {
    INDENT;
    indented = TRUE;
  }
  
  while (tree != NULL)
  {
    if (tree->nodekind != ListK
      && !(tree->nodekind == StmtK && tree->kind.stmt == NopK)
    ) {
      printSpaces();
    }

    if (tree->nodekind == StmtK)
    {
      switch (tree->kind.stmt)
      {
      case CompoundK:
        fprintf(listing, "Compound Statement:\n");
        break;
      case SelectK:
        if (tree->attr.has_else == TRUE) {
          fprintf(listing, "If-Else Statement:\n");
        } else {
          fprintf(listing, "If Statement:\n");
        }
        break;
      case IterK:
        fprintf(listing, "While Statement:\n");
        break;
      case RetK:
        if (tree->child[0] == NULL) {
          fprintf(listing, "Non-value Return Statement\n");
        } else {
          fprintf(listing, "Return Statement:\n");
        }
        break;
      case NopK:
        // Print nothing!
        break;
      default:
        fprintf(listing, "Unknown ExpNode kind\n");
        break;
      }
    } else if (tree->nodekind == ExpK) {
      switch (tree->kind.exp)
      {
      case AssignK:
        fprintf(listing, "Assign:\n");
        break;
      case BinaryOpK:
        fprintf(listing, "Op: ");
        printToken(tree->attr.op, "\0");
        break;
      case ConstK:
        fprintf(listing, "Const: %d\n", tree->attr.val);
        break;
      case IdK:
        fprintf(listing, "Variable: name = %s\n", tree->attr.name);
        break;
      case CallK:
        fprintf(listing, "Call: function name = %s\n", tree->attr.name);
        break;
      default:
        fprintf(listing, "Unknown ExpNode kind\n");
        break;
      }
    } else if (tree->nodekind == DeclK) {
      switch (tree->kind.decl) {
        case FunK:
          fprintf(listing, "Function Declaration: name = %s, return type = ", tree->attr.name);
          printTypes(tree->type);
          fprintf(listing, "\n");
          break;
        case VarK:
          fprintf(listing, "Variable Declaration: name = %s, type = ", tree->attr.name);
          printTypes(tree->type);
          fprintf(listing, "\n");
          break;
        case ParamK:
          if (tree->attr.name != NULL) {
            fprintf(listing, "Parameter: name = %s, type = ", tree->attr.name);
            printTypes(tree->type);
            fprintf(listing, "\n");
          } else {
            fprintf(listing, "Void Parameter\n");
          }

          break;
      }
    } else if (tree->nodekind == ListK) {
      tree->child[1] = NULL;
    } else {
      fprintf(listing, "Unknown node kind\n");
    }
    for (i = 0; i < MAXCHILDREN; i++) {
      printTree(tree->child[i]);
    }
    tree = tree->sibling;
  }

  if (indented == TRUE) {
    UNINDENT;
  }
}
