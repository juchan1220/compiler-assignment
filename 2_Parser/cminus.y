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

#define YYSTYPE TreeNode *
static char * savedName; /* for use in assignments */
static int savedLineNo;  /* ditto */
static TreeNode * savedTree; /* stores syntax tree for later return */
static int yylex(void); // added 11/2/11 to ensure no conflict with lex

%}

%token IF ELSE WHILE RETURN INT VOID
%token ID NUM 
%token ASSIGN EQ NE LE LT GE GT PLUS MINUS TIMES OVER LPAREN RPAREN LBRACE RBRACE LCURLY RCURLY SEMI COMMA
%token ERROR 

%% /* Grammar for C-Minus */

program     : decl_list
                 { savedTree = $1;} 
            ;
decl_list   : decl_list decl
                { }
            | decl
                { }
            ;
decl        : var_decl { }
            | fun_decl { }
            ;
var_decl    : type_spec ID SEMI
                { }
            | type_spec ID LBRACE NUM RBRACE SEMI
                { }
            ;
type_spec   : INT { }
            | VOID { }
            ;
fun_decl    : type_spec ID LPAREN params RPAREN cmpnd_stmt
                { }
            ;
params      : param_list { }
            | VOID { }
            ;
param_list  : param_list COMMA param { }
            | param { }
            ;
param       : type_spec ID { }
            | type_spec ID LBRACE RBRACE { }
            ;
cmpnd_stmt  : LCURLY local_decls stmt_list RCURLY { }
            ;
local_decls : local_decls var_decl { }
            | { }
            ;
stmt_list   : stmt_list stmt { }
            | { }
            ;
stmt        : expr_stmt { }
            | cmpnd_stmt { }
            | select_stmt { }
            | iter_stmt { }
            | ret_stmt { }
            ;
expr_stmt   : expr SEMI { }
            | SEMI { }
            ;
select_stmt : IF LPAREN expr RPAREN stmt { }
            | IF LPAREN expr RPAREN ELSE stmt { }
            ;
iter_stmt   : WHILE LPAREN expr RPAREN stmt { }
            ;
ret_stmt    : RETURN SEMI { }
            | RETURN expr SEMI { }
            ;
expr        : var ASSIGN expr { }
            | simple_expr { }
            ;
var         : ID { }
            | ID LBRACE expr RBRACE { }
            ;
simple_expr : addtv_expr  relop addtv_expr { }
            | addtv_expr { }
            ;
relop       : LE { }
            | LT { }
            | GT { }
            | GE { }
            | EQ { }
            | NE { }
            ;
addtv_expr  : addtv_expr addop term { }
            | term { }
            ;
addop       : PLUS { }
            | MINUS { }
            ;
term        : term mulop factor { }
            | factor { }
            ;
mulop       : TIMES { }
            | OVER { }
            ;
factor      : LCURLY expr RCURLY { }
            | var { }
            | call { }
            | NUM { }
            ;
call        : ID LPAREN args RPAREN { }
            ;
args        : arg_list { }
            | { }
            ;
arg_list    : arg_list COMMA expr { }
            | expr { }
            ;




stmt_seq    : stmt_seq SEMI stmt
                 { YYSTYPE t = $1;
                   if (t != NULL)
                   { while (t->sibling != NULL)
                        t = t->sibling;
                     t->sibling = $3;
                     $$ = $1; }
                     else $$ = $3;
                 }
            | stmt  { $$ = $1; }
            ;
stmt        : if_stmt { $$ = $1; }
            | repeat_stmt { $$ = $1; }
            | assign_stmt { $$ = $1; }
            | read_stmt { $$ = $1; }
            | write_stmt { $$ = $1; }
            | error  { $$ = NULL; }
            ;
if_stmt     : IF exp THEN stmt_seq END
                 { $$ = newStmtNode(IfK);
                   $$->child[0] = $2;
                   $$->child[1] = $4;
                 }
            | IF exp THEN stmt_seq ELSE stmt_seq END
                 { $$ = newStmtNode(IfK);
                   $$->child[0] = $2;
                   $$->child[1] = $4;
                   $$->child[2] = $6;
                 }
            ;
repeat_stmt : REPEAT stmt_seq UNTIL exp
                 { $$ = newStmtNode(RepeatK);
                   $$->child[0] = $2;
                   $$->child[1] = $4;
                 }
            ;
assign_stmt : ID { savedName = copyString(tokenString);
                   savedLineNo = lineno; }
              ASSIGN exp
                 { $$ = newStmtNode(AssignK);
                   $$->child[0] = $4;
                   $$->attr.name = savedName;
                   $$->lineno = savedLineNo;
                 }
            ;
read_stmt   : READ ID
                 { $$ = newStmtNode(ReadK);
                   $$->attr.name =
                     copyString(tokenString);
                 }
            ;
write_stmt  : WRITE exp
                 { $$ = newStmtNode(WriteK);
                   $$->child[0] = $2;
                 }
            ;
exp         : simple_exp LT simple_exp 
                 { $$ = newExpNode(OpK);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->attr.op = LT;
                 }
            | simple_exp EQ simple_exp
                 { $$ = newExpNode(OpK);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->attr.op = EQ;
                 }
            | simple_exp { $$ = $1; }
            ;
simple_exp  : simple_exp PLUS term 
                 { $$ = newExpNode(OpK);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->attr.op = PLUS;
                 }
            | simple_exp MINUS term
                 { $$ = newExpNode(OpK);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->attr.op = MINUS;
                 } 
            | term { $$ = $1; }
            ;
term        : term TIMES factor 
                 { $$ = newExpNode(OpK);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->attr.op = TIMES;
                 }
            | term OVER factor
                 { $$ = newExpNode(OpK);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->attr.op = OVER;
                 }
            | factor { $$ = $1; }
            ;
factor      : LPAREN exp RPAREN
                 { $$ = $2; }
            | NUM
                 { $$ = newExpNode(ConstK);
                   $$->attr.val = atoi(tokenString);
                 }
            | ID { $$ = newExpNode(IdK);
                   $$->attr.name =
                         copyString(tokenString);
                 }
            | error { $$ = NULL; }
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

