/****************************************************/
/* File: c-.l                                       */
/* Lex specification for C-                         */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/* Adaptado de Tiny para C- por Lucas Bicalho       */
/****************************************************/

%{
#include "globals.h"
#include "util.h"
#include "scan.h"
  
/* lexeme of identifier or reserved word */
char tokenString[MAXTOKENLEN+1];
static int yylex(void);
%}

digit       [0-9]
number      {digit}+
letter      [a-zA-Z]
identifier  {letter}+
newline     \n
whitespace  [ \r\t]+

%%

"if"            {return IF;}
"else"          {return ELSE;}
"int"          {return INT;}		/*ADICIONADO*/
"return"        {return RETURN;} 	/*ADICIONADO*/
"void"          {return VOID;} 		/*ADICIONADO*/
"while"         {return WHILE;} 	/*ADICIONADO*/
"+"             {return PLUS;}
"-"             {return MINUS;}
"*"             {return TIMES;}
"/"             {return OVER;}
"<"             {return LT;}
"<="             {return LTEQ;} 	/*ADICIONADO*/
">"             {return GT;} 		/*ADICIONADO*/
">="             {return GTEQ;} 	/*ADICIONADO*/
"=="             {return EQ;} 		/*ADICIONADO*/
"!="             {return NEQ;} 		/*ADICIONADO*/
"="            {return ASSIGN;} 	/*ADICIONADO*/
";"             {return SEMI;}
","             {return	COMMA;}		/*ADICIONADO*/
"("             {return LPAREN;}
")"             {return RPAREN;}
"["             {return LBRACKET;}	/*ADICIONADO*/
"]"             {return RBRACKET;}	/*ADICIONADO*/
"{"             {return LBRACE;}	/*ADICIONADO*/
"}"             {return RBRACE;}	/*ADICIONADO*/
{number}        {return NUM;}
{identifier}    {return ID;}
{newline}       {lineno++;}
{whitespace}    {/* skip whitespace */}
"/*"            {
					char ant, atual;
					atual = input();
					if (atual == EOF) break;
					if (atual == '\n') lineno++;
					do{
						ant = atual;
						atual = input();
						if (atual == EOF) break;
								if (atual == '\n') lineno++;
					} while (ant != '*' || atual != '/');
				}
.               {return ERROR;}

%%

TokenType getToken(void)
{ static int firstTime = TRUE;
  TokenType currentToken;
  if (firstTime)
  { firstTime = FALSE;
    lineno++;
    yyin = source;
    yyout = listing;
  }
  currentToken = yylex();
  strncpy(tokenString,yytext,MAXTOKENLEN);
  if (TraceScan) {
    fprintf(listing,"\t%d: ",lineno);
    printToken(currentToken,tokenString);
  }
  return currentToken;
}

