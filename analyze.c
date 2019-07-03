/****************************************************/
/* File: analyze.c                                  */
/* Semantic analyzer implementation                 */
/* for the C- compiler                              */
/* Autor: Lucas Bicalho Oliveira                    */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "analyze.h"

/* counter for variable memory locations */
static int location = 0;
static char *current_scope = "global";

/* Procedure traverse is a generic recursive 
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc 
 * in postorder to tree pointed to by t
 */
static void traverse(TreeNode * t, void (* preProc) (TreeNode *), void (* postProc) (TreeNode *)){
    if (t != NULL){
        preProc(t);
        {   int i;
            for (i=0; i < MAXCHILDREN; i++)
                traverse(t->child[i],preProc,postProc);
        }
        postProc(t);
        traverse(t->sibling,preProc,postProc);
    }
}

/* nullProc is a do-nothing procedure to 
 * generate preorder-only or postorder-only
 * traversals from traverse
 */
static void nullProc(TreeNode * t){
    if (t==NULL) return;
    else return;
}

void typeError(TreeNode * t, char * message){
    fprintf(listing,"Type error at line %d: (Variavel: %s) %s\n",t->lineno, t->attr.name, message);
    Error = TRUE;
}

void typeError2(TreeNode * t, char * message){
    fprintf(listing,"Type error at line %d: %s\n",t->lineno, message);
    Error = TRUE;
}

/* Procedure insertNode inserts 
 * identifiers stored in t into 
 * the symbol table 
 */
static void insertNode( TreeNode * t){

    switch (t->nodekind){

        case ExpK:
            switch (t->kind.exp){

                case VariableK:
                    if (st_lookup(t->attr.name, current_scope) == -1 && st_lookup(t->attr.name, "global") == -1 && 
                        st_lookup(t->attr.name, "") == -1){
                        /* not yet in table, so treat as new definition */
                        if(t->type == Void)
                            typeError(t,"Erro 3: declaracao invalida de variavel, void so pode ser usado para declaracao de funcao");
                        else{
                            t->escopo = current_scope;
                            st_insert(t->attr.name, t->tipoId, t->escopo, t->tipoDado, t->lineno, location++);
                        }
                    }
                    else{
                        if(st_lookup(t->attr.name, "global") == -1){
                            typeError(t,"Erro 4 ou 7: Variavel ou funcao ja declarada");
                        }
                        else
                            typeError(t,"Erro 7: Declaracao invalida (variavel declarada com nome de funcao)");
                    }
                    break;

                case FunctionK:
                    if (st_lookup(t->attr.name, current_scope) == -1 && st_lookup(t->attr.name, "global") == -1 && 
                        st_lookup(t->attr.name, "") == -1){
                        /* not yet in table, so treat as new definition */
                        current_scope = t->attr.name;
                        st_insert(t->attr.name, t->tipoId, "", t->tipoDado, t->lineno, location++);
                    }
                    else{
                        typeError(t,"Erro 4 ou 7: Funcao ja declarada");
                        //if(t->tipoId == "Funcao")
                        //  t->attr.name = "";
            		}
                    break;

                case VectorK:
                    if (st_lookup(t->attr.name, current_scope) == -1 && st_lookup(t->attr.name, "global") == -1 && 
                        st_lookup(t->attr.name, "") == -1){
                        /* not yet in table, so treat as new definition */
                        t->escopo = current_scope;
                        st_insert(t->attr.name, t->tipoId, t->escopo, t->tipoDado, t->lineno, location++);
                    }
                    else
                        typeError(t,"Erro 4 ou 7: Vetor ja declarado");
                    break;

    		    case IdK:
                    if (st_lookup(t->attr.name, current_scope) == -1 && st_lookup(t->attr.name, "global") == -1){
                        /* not yet in table, so treat as new definition */
          			   typeError(t,"Erro 1: Variavel nao declarada");
                    }
                    else{
                        //already in table, so ignore location, add line number of use only
                        t->escopo = current_scope;
                        //t->tipoDado = st_lookup_type(t->attr.name, t->escopo);
                        st_insert(t->attr.name, t->tipoId, current_scope, t->tipoDado, t->lineno, 0);
          			   //if(t->tipoId == "Funcao")
          			   //	t->attr.name = "";
                    }
                    break;

                case IdFunctionK:
                    t->escopo = current_scope;
                    if (st_lookup(t->attr.name, "global") == -1 && st_lookup(t->attr.name, "") == -1 &&
                        strcmp(t->attr.name, "input") != 0 && strcmp(t->attr.name, "output") != 0){
                        /* not yet in table, so treat as new definition */
                        typeError(t,"Error 5: Chamada de Funcao nao declarada");
                    }
                    else{
                        //already in table, so ignore location, add line number of use only
                        t->tipoDado = st_lookup_type(t->attr.name, "", "Função");
                        st_insert(t->attr.name, t->tipoId, current_scope, t->tipoDado, t->lineno, 0);
                        if(t->tipoId == "Funcao")
                            t->attr.name = "";
                    }
                    break;

                case IdVectorK:
                    if (st_lookup(t->attr.name, current_scope) == -1 && st_lookup(t->attr.name, "global") == -1){
                        /* not yet in table, so treat as new definition */
                        typeError(t,"Error 1: Vetor nao declarado");
                    }
                    else{
                        //already in table, so ignore location, add line number of use only
                        //t->escopo = current_scope;
                        //t->tipoDado = st_lookup_type(t->attr.name, t->escopo, "Vetor");
                        st_insert(t->attr.name, t->tipoId, current_scope, t->tipoDado, t->lineno, 0);
                        //if(t->tipoId == "Vetor")
                        //  t->attr.name = "";
                    }
                    break;

                default:
                    break;
            }
            break;

        default:
            break;
    }
}

/* Function buildSymtab constructs the symbol 
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode * syntaxTree){

    traverse(syntaxTree,insertNode,nullProc);

    if(st_lookup("main", "") == -1)
        printf("Erro 6: Funcao main() nao declarada:\n");

    if (TraceAnalyze){ 
        fprintf(listing,"\nSymbol table:\n\n");
        printSymTab(listing);
    }
}


/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode * t){
    switch (t->nodekind){

        case ExpK:
            switch (t->kind.exp){

                case OpK:
                    if (t->attr.op == ASSIGN){
                        if ((t->child[2] != NULL) && ( t->child[1]->kind.exp == IdFunctionK) && strcmp(t->child[1]->tipoDado, "VOID") == 0)
                            typeError2(t, "Erro 2: Atribuicao invalida");
                    }
                    break;

                case ConstK:

                case IdK:
                    t->type = Int;
                    break;

                default:
                    break;
            }
            break;

        case StmtK:
            switch (t->kind.stmt){

                case IfK:
                    if (t->child[0]->type == Integer)
                        typeError(t->child[0],"if test is not Boolean");
                    break;

                case WhileK:
                    if (t->child[0]->type == Integer)
                        typeError(t->child[0],"while test is not Boolean");
                    break;

                default:
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
void typeCheck(TreeNode * syntaxTree){
    traverse(syntaxTree,nullProc,checkNode);
}
