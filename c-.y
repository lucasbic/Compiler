/****************************************************/
/* File: c-.y                                     */
/* The C- Yacc/Bison specification file           */
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
static int yylex(void);


%}

%token IF ELSE INT RETURN VOID WHILE
%token ID NUM 
%token PLUS MINUS TIMES OVER LT LTEQ GT GTEQ EQ NEQ ASSIGN SEMI COMMA LPAREN RPAREN LBRACKET RBRACKET LBRACE RBRACE
%token ERROR 

%% /* Grammar for C- */

programa            : declaracao_lista
                        { savedTree = $1;}
                    ;
declaracao_lista    : declaracao_lista declaracao
                        { YYSTYPE t = $1;
                          if (t != NULL)
                          { while (t->sibling != NULL)
                            t = t->sibling;
                            t->sibling = $2;
                            $$ = $1; }
                          else $$ = $2;
                        }
                    | declaracao  { $$ = $1; }
                    ;
declaracao          : var_declaracao { $$ = $1; }
                    | fun_declaracao { $$ = $1; }
                    ;
var_declaracao      : tipo_especificador id SEMI
                         {  $$ = $1;
                            $$->child[0] = $2;
                    				$$->child[0]->kind.exp = VariableK;
                    			   $$->child[0]->tipoId = "Variável";
                    			   $$->child[0]->tipoDado = "INT";
                             $$->child[0]->type = $$->type;
                         }
                    | error { yyerrok; }
                    | tipo_especificador id LBRACKET num RBRACKET SEMI
                         {  $$ = $1;
                            $$->child[0] = $2;
                    				$$->child[0]->kind.exp = VectorK;
                    			   $$->child[0]->tipoId = "Vetor";
                    			   $$->child[0]->tipoDado = "INT";
                            $$->child[0]->child[0] = $4;
                            $$->child[0]->type = $$->type;
                         }
                    ;
tipo_especificador  : INT
                         { $$ = newExpNode(TypeK);
                           $$->type = Int;
                           $$->attr.name = copyString(tokenString);
                         }
                    | VOID
                         { $$ = newExpNode(TypeK);
                           $$->type = Void;
                           $$->attr.name = copyString(tokenString);
                         }
                    ;
fun_declaracao      : tipo_especificador id LPAREN params RPAREN composto_decl 
                         { $$ = $1;
                           $$->child[0] = $2;
                      		if($$->type == Int)
                      	   $$->child[0]->tipoDado = copyString("INT");
                      		else
                        		$$->child[0]->tipoDado = copyString("VOID");
                        	$$->child[0]->kind.exp = FunctionK;
                          $$->child[0]->type = $$->type;
              	          $$->child[0]->tipoId = "Função";
                          $$->child[0]->child[0] = $4;
                          $$->child[0]->child[1] = $6;
                         }
                    ;
params              : param_lista { $$ = $1; }
                    | tipo_especificador { $$ = $1; }
                    ;
param_lista         : param_lista COMMA param
                         { YYSTYPE t = $1;
                           if (t != NULL)
                           { while (t->sibling != NULL)
                                t = t->sibling;
                                t->sibling = $3;
                                $$ = $1; }
                            else $$ = $3;
                         }
                    | param { $$ = $1; }
                    ;
param               : tipo_especificador id
                         { $$ = $1;
                           $$->child[0] = $2;
                  				$$->child[0]->kind.exp = VariableK;
                          $$->child[0]->type = $$->type;
                  			   $$->child[0]->tipoId = "Variável";
                  			   $$->child[0]->tipoDado = "INT";
                         }
                    | tipo_especificador id LBRACKET RBRACKET
                         { $$ = $1;
                           $$->child[0] = $2;
                           $$->child[0]->kind.exp = VectorK;
                           $$->child[0]->type = $$->type;
                  			   $$->child[0]->tipoId = "Vetor";
                  			   $$->child[0]->tipoDado = "INT";
                         }
                    ;
composto_decl       : LBRACE local_declaracoes statement_lista RBRACE 
                         {  YYSTYPE t = $2;
                           if (t != NULL)
                           { while (t->sibling != NULL)
                                t = t->sibling;
                                t->sibling = $3;
                                $$ = $2; }
                            else $$ = $3;
                         }
                    | LBRACE local_declaracoes RBRACE { $$ = $2; }
                    | LBRACE statement_lista RBRACE { $$ = $2; }
                    | LBRACE RBRACE
                    ;
local_declaracoes   : local_declaracoes var_declaracao 
                         {  YYSTYPE t = $1;
                           if (t != NULL)
                           { while (t->sibling != NULL)
                                t = t->sibling;
                                t->sibling = $2;
                                $$ = $1; }
                            else $$ = $2;
                         }
                    | var_declaracao { $$ = $1; }
                    ;
statement_lista     : statement_lista statement 
                        {  YYSTYPE t = $1;
                           if (t != NULL)
                           { while (t->sibling != NULL)
                                t = t->sibling;
                                t->sibling = $2;
                                $$ = $1; }
                            else $$ = $2;
                        }
                    | statement { $$ = $1; }
                    ;
statement           : expressao_decl { $$ = $1; }
                    | composto_decl { $$ = $1; }
                    | selecao_decl { $$ = $1; }
                    | iteracao_decl { $$ = $1; }
                    | retorno_decl { $$ = $1; }
                    ;
expressao_decl      : expressao SEMI { $$ = $1; }
                    | SEMI
                    | error { yyerrok; }
                    ;
selecao_decl        : IF LPAREN expressao RPAREN statement
                        { $$ = newStmtNode(IfK);
                          $$->child[0] = $3;
                          $$->child[1] = $5;
                        }
                    | IF LPAREN expressao RPAREN statement ELSE statement
                        { $$ = newStmtNode(IfK);
                          $$->child[0] = $3;
                          $$->child[1] = $5;
                          $$->child[2] = $7;
                        }
                    ;
iteracao_decl       : WHILE LPAREN expressao RPAREN statement
                        { $$ = newStmtNode(WhileK);
                        $$->attr.name = copyString(tokenString);
                          $$->child[0] = $3;
                          $$->child[1] = $5;
                        }
                    ;
retorno_decl        : RETURN SEMI
                        {
                          $$ = newStmtNode(ReturnK);
                        $$->attr.name = "";
                        }
                    | RETURN expressao SEMI
                        {
                          $$ = newStmtNode(ReturnK);
                        $$->attr.name = "";
                          $$->child[0] = $2;
                        }
                    ;
expressao           : var relacional expressao 
                        {
                          $$ = $2;
                          $$->child[0] = $1;
                          $$->child[1] = $3;
                        }
                    | simples_expressao { $$ = $1; }
                    ;
var                 : id { $$ = $1;
                  			   $$->tipoId = "Variável";
                  			   $$->tipoDado = "INT";
                          $$->kind.exp = IdK;
			                   }
                    | id LBRACKET expressao RBRACKET
                        { $$ = $1;
                  			   $$->tipoId = "Vetor";
                  			   $$->tipoDado = "INT";
                          $$->child[0] = $3;
                          $$->kind.exp = IdVectorK;
                        }
                    ;
simples_expressao   : soma_expressao relacional soma_expressao
                        {
                            $$ = $2;
                            $$->child[0] = $1;
                            $$->child[1] = $3;
                        }
                    | soma_expressao { $$ = $1; }
                    ;
relacional          : LTEQ 
                        { $$ = newExpNode(OpK);
                          $$->attr.op = LTEQ;
                        }
                    | LT 
                        { $$ = newExpNode(OpK);
                          $$->attr.op = LT;
                        }
                    | GT
                        { $$ = newExpNode(OpK);
                          $$->attr.op = GT;
                        }
                    | GTEQ 
                        { $$ = newExpNode(OpK);
                          $$->attr.op = GTEQ;
                        }
                    | EQ 
                        { $$ = newExpNode(OpK);
                          $$->attr.op = EQ;
                        }
                    | NEQ
                        { $$ = newExpNode(OpK);
                          $$->attr.op = NEQ;
                        }
                    | ASSIGN
                        { $$ = newExpNode(OpK);
                          $$->attr.op = ASSIGN;
                        }
                    ;
soma_expressao      : soma_expressao soma termo 
                        { $$ = $2;
                            $$->child[0] = $1;
                            $$->child[1] = $3;
                        }
                    | termo { $$ = $1; }
                        
                    ;
soma                : PLUS
                        { $$ = newExpNode(OpK);
                          $$->attr.op = PLUS;
                        }
                    | MINUS
                        { $$ = newExpNode(OpK);
                          $$->attr.op = MINUS;
                        }
                    ;
termo               : termo mult fator
                        { $$ = $2;
                            $$->child[0] = $1;
                            $$->child[1] = $3;
                        }
                    | fator { $$ = $1; }
                    ;
mult                : TIMES 
                        { $$ = newExpNode(OpK);
                          $$->attr.op = TIMES;
                        }
                    | OVER
                        { $$ = newExpNode(OpK);
                          $$->attr.op = OVER;
                        }
                    ;
fator               : LPAREN expressao RPAREN
                        { $$ = $2; }
                    | var 
                        { $$ = $1; }
                    | ativacao 
                        { $$ = $1; }
                    | num
                        { $$ = $1;
                        }
                    ;
ativacao            : id LPAREN arg_lista RPAREN
                        { $$ = $1;
                        $$->kind.exp = IdFunctionK;
			                   $$->tipoId = "Função";
                          $$->child[0] = $3;
                        }
                    | id LPAREN RPAREN
                        { $$ = $1;
                        $$->kind.exp = IdFunctionK;
			                   $$->tipoId = "Função";
                        }
                    ;
arg_lista           : arg_lista COMMA expressao
                        {  YYSTYPE t = $1;
                           if (t != NULL)
                           { while (t->sibling != NULL)
                                t = t->sibling;
                                t->sibling = $3;
                                $$ = $1; }
                            else $$ = $3;
                        }
                    | expressao { $$ = $1; }
                    ;
id                  : ID
                        { $$ = newExpNode(IdK);
                          $$->attr.name = copyString(tokenString);
                        }
                    ;
num                 : NUM
                        { $$ = newExpNode(ConstK);
                          $$->attr.val = atoi(tokenString);
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

