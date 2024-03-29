/****************************************************/
/* File: cminus.y                                   */
/* The C-Minus Yacc/Bison specification file        */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/
%{
#define YYPARSER /* distinguishes Yacc output from other code files */

#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parse.h"

static char * savedName; /* for use in assignments */
static int savedLineNo;  /* ditto */
static TreeNode * savedTree; /* stores syntax tree for later return */
static int yylex(void); // added 11/2/11 to ensure no conflict with lex

%}

%union {
 TreeNode *node;
 char* id_name;
 ExpType type_spec;
 int num_value;
 TokenType op_type;
}

%token<id_name> ID
%token<num_value> NUM
%nonassoc ASSIGN
%nonassoc EQ NE
%nonassoc LE LT GE GT
%left PLUS MINUS
%left TIMES OVER
%token LPAREN RPAREN LBRACE RBRACE LCURLY RCURLY SEMI COMMA
%token IF ELSE WHILE RETURN INT VOID
%token ERROR

%type<type_spec> type_spec
%type<node> decl_list decl var_decl var fun_decl params param_list param cmpnd_stmt local_decls stmt_list stmt expr_stmt compl_stmt incompl_stmt ret_stmt expr simple_expr addtv_expr term factor call args arg_list
%type<op_type> relop addop mulop

%% /* Grammar for C-Minus */

program     : decl_list { savedTree = $1; }
            ;
decl_list   : decl_list decl {
              $$ = $1;
              $$->attr.lastChildOfList->sibling = $2;
              $$->attr.lastChildOfList = $2;
            }
            | decl {
              $$ = newListNode(DeclListK); 
              $$->child[0] = $1;
              $$->attr.lastChildOfList = $1;
            }
            ;
decl        : var_decl { $$ = $1; }
            | fun_decl { $$ = $1; }
            ;
var_decl    : type_spec ID LBRACE NUM RBRACE SEMI {
              $$ = newDeclNode(VarK);
              switch ($1) {
                case Void:
                  $$->type = VoidArray;
                  break;
                case Integer:
                  $$->type = IntegerArray;
                  break;
                default:
                  /* !! this should not happen !! */
                  yyerror("token type_spec has invalid semantic value.");
                  // YYABORT;
              }
              $$->attr.name = copyString($2);

              $$->child[0] = newExpNode(ConstK);
              $$->child[0]->attr.val = $4;
            }
            | type_spec ID SEMI {
              $$ = newDeclNode(VarK);
              $$->type = $1;
              $$->attr.name = copyString($2);
            }
            ;
type_spec   : INT { $$ = Integer; }
            | VOID { $$ = Void; }
            ;
fun_decl    : type_spec ID LPAREN params RPAREN cmpnd_stmt {
              $$ = newDeclNode(FunK);
              $$->type = $1;
              $$->attr.name = $2;
              $$->child[0] = $4;
              $$->child[1] = $6;
            }
            ;
params      : param_list { $$ = $1; }
            | VOID {
              $$ = newListNode(ParamListK);
              $$->child[0] = $$->child[1] = newDeclNode(ParamK);
              $$->attr.name = NULL;
            }
            ;
param_list  : param_list COMMA param { 
              $$ = $1;
              $$->attr.lastChildOfList->sibling = $3;
              $$->attr.lastChildOfList = $3;
            }
            | param {
              $$ = newListNode(ParamListK);
              $$->child[0] = $1;
              $$->attr.lastChildOfList = $1;
            }
            ;
param       : type_spec ID LBRACE RBRACE {
              $$ = newDeclNode(ParamK);
              switch ($1) {
                case Void:
                  $$->type = VoidArray;
                  break;
                case Integer:
                  $$->type = IntegerArray;
                  break;
                default:
                  /* !! this should not happen !! */
                  yyerror("token type_spec has invalid semantic value.");
                  // YYABORT;
              }

              $$->attr.name = copyString($2);
            }
            | type_spec ID {
              $$ = newDeclNode(ParamK);
              $$->type = $1;
              $$->attr.name = copyString($2);
            }
            ;
cmpnd_stmt  : LCURLY local_decls stmt_list RCURLY {
              $$ = newStmtNode(CompoundK);
              $$->child[0] = $2;
              $$->child[1] = $3;
            }
            ;
local_decls : local_decls var_decl {
              if ($1 == NULL) {
                $$ = newListNode(LocalDeclListK);
                $$->child[0] = $2;
                $$->attr.lastChildOfList = $2;
              } else {
                $$ = $1;
                $$->attr.lastChildOfList->sibling = $2;
                $$->attr.lastChildOfList = $2;
              }
            }
            | %empty { $$ = NULL; }
            ;
stmt_list   : stmt_list stmt { 
              if ($1 == NULL) {
                $$ = newListNode(StmtListK);
                $$->child[0] = $2;
                $$->attr.lastChildOfList = $2;
              } else {
                $$ = $1;
                $$->attr.lastChildOfList->sibling = $2;
                $$->attr.lastChildOfList = $2;
              }
            }
            | %empty { $$ = NULL; }
            ;
stmt        : compl_stmt { $$ = $1; }
            | incompl_stmt { $$ = $1; }
            ;
compl_stmt  : IF LPAREN expr RPAREN compl_stmt ELSE compl_stmt {
              $$ = newStmtNode(SelectK);
              $$->child[0] = $3;
              $$->child[1] = $5;
              $$->child[2] = $7;
              $$->attr.has_else = TRUE;
            }
            | WHILE LPAREN expr RPAREN compl_stmt {
              $$ = newStmtNode(IterK);
              $$->child[0] = $3;
              $$->child[1] = $5;
            }
            | expr_stmt { $$ = $1; }
            | cmpnd_stmt { $$ = $1; }
            | ret_stmt { $$ = $1; }
            ;
incompl_stmt: IF LPAREN expr RPAREN stmt {
              $$ = newStmtNode(SelectK);
              $$->child[0] = $3;
              $$->child[1] = $5;
              $$->attr.has_else = FALSE;
            }
            | IF LPAREN expr RPAREN compl_stmt ELSE incompl_stmt {
              $$ = newStmtNode(SelectK);
              $$->child[0] = $3;
              $$->child[1] = $5;
              $$->child[2] = $7;
              $$->attr.has_else = TRUE;
            }
            | WHILE LPAREN expr RPAREN incompl_stmt {
              $$ = newStmtNode(IterK);
              $$->child[0] = $3;
              $$->child[1] = $5;
            }
            ;
expr_stmt   : expr SEMI { $$ = $1; }
            | SEMI { $$ = newStmtNode(NopK); }
            ;
ret_stmt    : RETURN SEMI { $$ = newStmtNode(RetK); }
            | RETURN expr SEMI {
              $$ = newStmtNode(RetK);
              $$->child[0] = $2;
            }
            ;
expr        : var ASSIGN expr { 
              $$ = newExpNode(AssignK);
              $$->child[0] = $1;
              $$->child[1] = $3;
            }
            | simple_expr { $$ = $1; }
            ;
var         : ID LBRACE expr RBRACE {
              $$ = newExpNode(IdK);
              $$->attr.name = copyString($1);
              $$->child[0] = $3;
            }
            | ID { 
              $$ = newExpNode(IdK);
              $$->attr.name = copyString($1);
            }
            ;
simple_expr : addtv_expr relop addtv_expr {
              $$ = newExpNode(BinaryOpK);
              $$->child[0] = $1;
              $$->attr.op = $2;
              $$->child[1] = $3;
            }
            | addtv_expr { $$ = $1; }
            ;
relop       : LE { $$ = LE; }
            | LT { $$ = LT; }
            | GT { $$ = GT; }
            | GE { $$ = GE; }
            | EQ { $$ = EQ; }
            | NE { $$ = NE; }
            ;
addtv_expr  : addtv_expr addop term { 
              $$ = newExpNode(BinaryOpK);
              $$->child[0] = $1;
              $$->attr.op = $2;
              $$->child[1] = $3;
            }
            | term { $$ = $1; }
            ;
addop       : PLUS { $$ = PLUS; }
            | MINUS { $$ = MINUS; }
            ;
term        : term mulop factor {
              $$ = newExpNode(BinaryOpK);
              $$->child[0] = $1;
              $$->attr.op = $2;
              $$->child[1] = $3;
            }
            | factor { $$ = $1; }
            ;
mulop       : TIMES { $$ = TIMES; }
            | OVER { $$ = OVER; }
            ;
factor      : LPAREN expr RPAREN { $$ = $2; }
            | var { $$ = $1; }
            | call { $$ = $1; }
            | NUM {
                $$ = newExpNode(ConstK);
                $$->attr.val = $1;
            }
            ;
call        : ID LPAREN args RPAREN {
              $$ = newExpNode(CallK);
              $$->attr.name = copyString($1);
              $$->child[0] = $3;
            }
            ;
args        : arg_list { $$ = $1; }
            | %empty { $$ = NULL; }
            ;
arg_list    : arg_list COMMA expr {
              $$->attr.lastChildOfList->sibling = $3;
              $$->attr.lastChildOfList = $3;
            }
            | expr {
              $$ = newListNode(ArgListK);
              $$->child[0] = $1;
              $$->attr.lastChildOfList = $1;
             }
            ;

%%

int yyerror(char * message)
{ fprintf(listing,"Syntax error at line %d: %s\n",lineno,message);
  fprintf(listing,"Current token: ");
  printToken(yychar,tokenString);
  Error = TRUE;
  return 0;
}

/* yylex calls getToken to make Yacc/Bison output
 * compatible with ealier versions of the TINY scanner
 */
static int yylex(void)
{ return getToken(); }

TreeNode * parse(void)
{ yyparse();
  return savedTree;
}

