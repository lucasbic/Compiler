/****************************************************/
/* File: code.h                                  	*/
/* Autor: Lucas Bicalho Oliveira                    */
/****************************************************/

# ifndef _CODE_H_
# define _CODE_H_

# include "cgen.h"

/* code emitting utilities */
void emitSpaces (int indent );

void emitComment(const char *c, int indent);

void emitCode(const char *c);

/* Procedimento toString retorna o mnemonico do enum da instrucao
* passado como parametro
*/
const char * toString ( enum instrucao i);
# endif