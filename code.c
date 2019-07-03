/****************************************************/
/* File: code.c                                 	*/
/* Autor: Lucas Bicalho Oliveira                    */
/****************************************************/

# include "globals.h"
# include "code.h"

/* Funcao toString que retorna o mnemonico da instrucao
* conforme o enum de parametro
*/
const char * toString ( enum instrucao i) {
	const char * strings [] = { "addition", "subtraction", "multiplication", "division", "vector",
								"equal", "not_equal", "less_than", "less_than_equal_to",
								"greater_than", "greater_than_equal_to", "assign",
								"function", "return", "arg", "call", "begin_args",
								"jump_if_false", "goto", "label", "halt"};
	return strings [i];
}

void emitSpaces (int indent ){
	int i;
	for (i = 0; i < indent ; ++i) {
		fprintf (code , " ");
	}
}

void emitComment ( const char * c, int indent ) {
	if ( TraceCode ) {
		emitSpaces ( indent );
		fprintf (code , "# %s\n", c);
	}
}

void emitCode ( const char * c) {
	printf ("\033[1m\033[32m%s\033[m\n", c);
}

void emitObjectCode(const char * c, int indent) {
    emitSpaces(indent);
    printf("%s\n", c);
}