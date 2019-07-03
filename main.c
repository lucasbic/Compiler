/****************************************************/
/* File: main.c                                     */
/* Main program for C- compiler                     */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"

/* set NO_PARSE to TRUE to get a scanner-only compiler */
#define NO_PARSE FALSE
/* set NO_ANALYZE to TRUE to get a parser-only compiler */
#define NO_ANALYZE FALSE

/* set NO_CODE to TRUE to get a compiler that does not
 * generate code
 */
#define NO_CODE FALSE

/* NO_OBJECT_CODE igual a TRUE nao gera codigo objeto ,
* caso contrario gera codigo objeto
*/
# define NO_OBJECT_CODE FALSE

/* NO_BINARY_CODE igual a TRUE nao gera codigo binario ,
* caso contrario gera codigo binario
*/
# define NO_BINARY_CODE FALSE

#include "util.h"
#if NO_PARSE
#include "scan.h"
#else
#include "parse.h"
#if !NO_ANALYZE
#include "analyze.h"
#if !NO_CODE
#include "cgen.h"
#if !NO_OBJECT_CODE
#include "object.h"
#if !NO_BINARY_CODE
#include "binary.h"
#endif
#endif
#endif
#endif
#endif

/* allocate global variables */
int lineno = 0;
FILE * source;
FILE * listing;
FILE * code;

/* allocate and set tracing flags */
int EchoSource = FALSE;
int TraceScan = FALSE;
int TraceParse = TRUE;
int TraceAnalyze = TRUE;
int TraceCode = TRUE;
int TraceObject = TRUE;
int TraceBinary = TRUE;

int Error = FALSE;
int codigoIntermediarioGerado = FALSE;
int codigoObjetoGerado = FALSE;

int main( int argc, char * argv[] )
{ TreeNode * syntaxTree;
  char pgm[120]; /* source code file name */
  if (argc != 2)
  { fprintf(stderr,"usage: %s <filename>\n",argv[0]);
      exit(1);
  }
  strcpy(pgm,argv[1]) ;
  if (strchr (pgm, '.') == NULL)
     strcat(pgm,".tny");
  source = fopen(pgm,"r");
  if (source==NULL)
  { fprintf(stderr,"File %s not found\n",pgm);
    exit(1);
  }
  listing = stdout; /* send listing to screen */
  fprintf(listing,"\nC- COMPILATION: %s\n",pgm);
#if NO_PARSE
  while (getToken()!=ENDFILE);
#else
  syntaxTree = parse();
  if (TraceParse) {
    fprintf(listing,"\nSyntax tree:\n");
    printTree(syntaxTree);
  }
#if !NO_ANALYZE
  //if (! Error)
   if (TraceAnalyze) fprintf(listing,"\nErros semanticos identificados:\n\n");
    buildSymtab(syntaxTree);
    if (TraceAnalyze) fprintf(listing,"\nChecking Types...\n");
    typeCheck(syntaxTree);
    if (TraceAnalyze) fprintf(listing,"\nType Checking Finished\n");
  //}
#if !NO_CODE
  if (! Error)
  { char * codefile;
    int fnlen = strcspn(pgm,".");
    codefile = (char *) calloc(fnlen+4, sizeof(char));
    strncpy(codefile,pgm,fnlen);
    strcat(codefile,".tm");
    code = fopen(codefile,"w");
    if (code == NULL)
    { printf("Unable to open %s\n", codefile);
      exit(1);
    }
    if (TraceCode)  fprintf(listing ,"\nGerando codigo intermediario ...\n");
    codeGen(syntaxTree, codefile);
    fclose(code);
    if (TraceCode)  fprintf(listing ,"\nGeracao de codigo intermediario concluida!\n\n");
    // Codigo intermediario gerado com sucesso
    codigoIntermediarioGerado = TRUE;
  }
  #if !NO_OBJECT_CODE
    if(codigoIntermediarioGerado) {
        char * codefile;
        int fnlen = strcspn(pgm, ".");
        codefile = (char *) calloc(fnlen + 4, sizeof(char));
        strncpy(codefile, pgm, fnlen);
        strcat(codefile, ".txt");
        code = fopen(codefile, "a+");
        Quadruple codigoIntermediario = getCodigoIntermediario();
        if (TraceObject) fprintf(listing, "\nGerando codigo objeto...\n");
        geraCodigoObjeto(codigoIntermediario);
        fclose(code);
        if (TraceObject) fprintf(listing, "\nGeracao de codigo objeto concluida!\n");
        // Codigo objeto gerado com sucesso
        codigoObjetoGerado = TRUE;
    }
  #if !NO_BINARY_CODE
    if(codigoObjetoGerado) {
        char * codefile;
        int fnlen = strcspn(pgm, ".");
        codefile = (char *) calloc(fnlen + 4, sizeof(char));
        strncpy(codefile, pgm, fnlen);
        strcat(codefile, ".txt");
        code = fopen(codefile, "a+");
        Objeto codigoObjeto = getCodigoObjeto();
        if (TraceBinary) fprintf(listing, "\nGerando codigo binario...\n");
        geraCodigoBinario(codigoObjeto);
        fclose(code);
        if (TraceBinary) fprintf(listing, "\nGeracao de codigo binario concluida!\n\n");
    }
#endif
#endif    
#endif
#endif
#endif
  fclose(source);
  return 0;
}
