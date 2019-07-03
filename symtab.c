/****************************************************/
/* File: symtab.c                                   */
/* Symbol table implementation for the C- compiler  */
/* (allows only one symbol table)                   */
/* Symbol table is implemented as a chained         */
/* hash table                                       */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"
#include "globals.h"

/* SHIFT is the power of two used as multiplier
   in hash function  */
#define SHIFT 4

#define MAX_SCOPE 20

#define ESCOPO_GLOBAL 0
#define ESCOPO_NAO_GLOBAL 1

static Scope scopes[MAX_SCOPE];
static int nScope = 0;
static Scope scopeStack[MAX_SCOPE];
static int nScopeStack = 0;

/* the hash function */
static int hash ( char * key, char * escopo )
{ int temp = 0;
  int i = 0;
  while (key[i] != '\0')
  { temp = ((temp << SHIFT) + key[i]) % SIZE;
    ++i;
  }
 if(escopo != NULL){
   i = 0;
   while (escopo[i] != '\0')
   { 
      temp = ((temp << SHIFT) + escopo[i]) % SIZE;
      ++i;
   }
 }
  //printf("\ntemp: %d\n", temp);
  return temp;
}

const char * dataTypeToString(ExpKind k) {
    printf("\nteste2\n");
    if(k == IdK) {
        return "Variavel";
    } else if(k == VectorK) {
        return "Vetor";
    } else {
        return "Funcao";
    }
}

Scope sc_top(void) {
    return scopeStack[nScopeStack - 1];
}

void sc_pop(void) {
    --nScopeStack;
}

void incScope() {
    ++nScopeStack;
}

void sc_push(Scope scope) {
    scopeStack[nScopeStack++] = scope;
}

Scope sc_create(char * funcName) {
    Scope newScope = (Scope) malloc(sizeof(struct ScopeRec));
    newScope->funcName = funcName;
    newScope->tamanhoBlocoMemoria = 0;
    if(!strcmp(funcName, "ESCOPO_GLOBAL")) {
        newScope->parent = NULL;
    } else {
        newScope->parent->funcName = globalScope;
    }
    scopes[nScope++] = newScope;
    return newScope;
}

Scope st_scopeVar(char * name) {
    int h = hash(name, '\0');
    Scope sc = sc_top();
    while(sc != NULL) {
        BucketList l = sc->hashTable[h];
        while ((l != NULL) && (strcmp(name, l->name))) {
            l = l->next;
        }
        if (l != NULL) return sc;
        sc = sc->parent;
    }
    return NULL;
}

BucketList st_bucket(char * name) {
    int h = hash(name, '\0');
    Scope sc = sc_top();
    while(sc != NULL) {
        BucketList l = sc->hashTable[h];
        while ((l != NULL) && (strcmp(name,l->name))) {
            l = l->next;
        }
        if (l != NULL) return l;
        sc = sc->parent;
    }
    return NULL;
}


// Procura por uma funcao no escopo global
BucketList st_bucket_func (char * name) {
  int h = hash(name, '\0');
  //printf("\nh: %d\n", h);
  Scope sc;
  sc->funcName  = globalScope;
  BucketList l = sc->hashTable[h];
  printf("\nh: %d\n", h);
  printf("\nname: %s\n", l->name);
  while ((l != NULL) && (strcmp(name,l->name))) {
    printf("\nwhile\n");
    l = l->next;
  }
  if (l != NULL) {
    printf("\nif\n");
    return l;
  } else {
    printf("\nelse\n");
    return NULL;
  }
}

/* the hash table */
static BucketList hashTable[SIZE];

BucketList getVarFromSymtab(char * nome, Scope escopo) {
    int h = hash(nome, '\0');
    BucketList l = escopo->hashTable[h];
    while (l != NULL) {
        /* !strcmp(const char *s1, const char *s2) verifica se as duas
         * strings passadas como parametro sao iguais. Retorna 0 em caso
         * verdadeiro, por isso o simbolo '!' antes da funcao
         */
        if(!strcmp(l->name, nome)) {
            return l;
        }
        l = l->next;
    }
    return NULL;
}

int getMemoryLocation(char * nome, char * escopo) {
    int h = hash(nome, escopo);
    BucketList l = hashTable[h];
    while (l != NULL) {
        //printf("\n%s\n", l->escopo);
        if(!strcmp(l->name, nome)) {
            return l->memloc;
        }
        l = l->next;
    }
    return -1;
}

/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
void st_insert( char * name, char * tipoId, char *escopo, char * tipoDado, int lineno, int loc )
{ int h = hash(name, escopo);
  BucketList l =  hashTable[h];
  Scope top = sc_top();

  while ((l != NULL) && (strcmp(name,l->name) != 0))
    l = l->next;

  if (l == NULL) /* variable not yet in table */
  { 
    l = (BucketList) malloc(sizeof(struct BucketListRec));
    l->name = name;
    l->tipoId = tipoId;
	  l->escopo = escopo;
    l->tipoDado = tipoDado;
    l->lines = (LineList) malloc(sizeof(struct LineListRec));
    l->lines->lineno = lineno;
    l->memloc = loc;
    l->lines->next = NULL;
    l->next = hashTable[h];
    hashTable[h] = l; }
  else /* found in table, so just add line number */
  { LineList t = l->lines;
    while (t->next != NULL) t = t->next;
    t->next = (LineList) malloc(sizeof(struct LineListRec));
    t->next->lineno = lineno;
    t->next->next = NULL;
  }
} /* st_insert */

BucketList st_create(char * name, int lineno, int loc, TreeNode * treeNode, int tamanho) {
    BucketList l = (BucketList) malloc(sizeof(struct BucketListRec));
    l->name = name;
    l->lines = (LineList) malloc(sizeof(struct LineListRec));
    l->lines->lineno = lineno;
    l->lines->next = NULL;
    l->memloc = loc;
    l->tamanho = tamanho;
    l->treeNode = treeNode;
    l->next = NULL;
    return l;
} /* st_create */

void st_add_lineno(TreeNode * treeNode) {
    // Adiciona o escopo ao nó da árvore sintática
    treeNode->escopo = st_scopeVar(treeNode->attr.name);
    int lineno = treeNode->lineno;
  BucketList l = st_bucket(treeNode->attr.name);
    LineList ll = l->lines;
    while (ll->next != NULL) {
    ll = ll->next;
    }
  if (ll->lineno != lineno) {
    ll->next = (LineList) malloc(sizeof(struct LineListRec));
      ll->next->lineno = lineno;
      ll->next->next = NULL;
  }
}

void st_insert_func(char * name, int lineno, TreeNode * treeNode) {
    int h = hash(name, '\0');
    Scope top = globalScope;
    BucketList l = top->hashTable[h];

    while ((l != NULL) && (strcmp(name,l->name) != 0)) {
      l = l->next;
    }
    if (l == NULL) { /* Variável ainda não existente na tabela */
        // Adiciona o escopo ao nó da árvore sintática
        treeNode->escopo = top;
        // Adiciona um novo item na tabela de símbolos
      l = (BucketList) malloc(sizeof(struct BucketListRec));
      l->name = name;
      l->lines = (LineList) malloc(sizeof(struct LineListRec));
      l->lines->lineno = lineno;
        l->lines->next = NULL;
        l->memloc = -1;
        l->tamanho = 0;
        l->treeNode = treeNode;
      l->next = top->hashTable[h];
      top->hashTable[h] = l;
  }
}

/* Function st_lookup returns the memory 
 * location of a variable or -1 if not found
 */
int st_lookup ( char * name, char * escopo )
{ int h = hash(name, escopo);
  BucketList l =  hashTable[h];
  while ((l != NULL) && (strcmp(name,l->name) != 0) && (strcmp(escopo,l->escopo) != 0))
    l = l->next;
  if (l == NULL) return -1;
  else return l->memloc;
}

char* st_lookup_type (char* name, char* escopo, char* tipoId)
{ 
  int h = hash(name, escopo);  
  BucketList l =  hashTable[h];
  while ((l != NULL) && (strcmp(name,l->name) != 0) && (strcmp(escopo,l->escopo) != 0))
        l = l->next;
  if (strcmp(name,"input") == 0)
  {
    return "INT";
  }
  else if (strcmp(name,"output") == 0)
  {
    return "VOID";
  }
  else if (l == NULL)
    return "null";
  else 
    return l->tipoDado;
}

int getQuantidadeArgumentos(TreeNode *treeNode) {
  int tamanho = 0;
  if( treeNode != NULL ) {
    ++ tamanho;
    while ( treeNode -> sibling != NULL ) {
      treeNode = treeNode -> sibling;
      ++ tamanho;
    }
  }
  return tamanho;
}

int getQuantidadeParametros(TreeNode * functionNode) {
    int qtd = 0;
    TreeNode * temp = functionNode->child[0];
    if(temp != NULL) {
        ++qtd;
        while(temp->sibling != NULL) {
            temp = temp->sibling;
            ++qtd;
        }
    }
    return qtd;
}

int getQuantidadeVariaveis(TreeNode * functionNode) {
    int qtd = 0;
    TreeNode * temp = functionNode->child[1]->child[0];
    if(temp != NULL) {
        ++qtd;
        while(temp->sibling != NULL) {
            temp = temp->sibling;
            ++qtd;
        }
    }
    return qtd;
}

int getTamanhoBlocoMemoriaEscopo(char * scopeName) {
    int i, tamanho = 0;
    for (i = 0; i < nScope; ++i) {
        if(!strcmp(scopeName, scopes[i]->funcName)) {
            tamanho = scopes[i]->tamanhoBlocoMemoria;
            break;
        }
    }
    return tamanho;
}

int getTamanhoBlocoMemoriaEscopoGlobal(void) {
    int j, tamanho = 0;
    //Scope global = scopes[0];
    //BucketList * hashTable = global->hashTable;
    for (j = 0; j < SIZE; ++j) {
        //printf("\nht name: %s\n", hashTable[j]->name);
        if (hashTable != NULL &&  hashTable[j] != NULL) {
            BucketList l = hashTable[j];
            while (l != NULL) {
                if(strcmp("FunctionK", l->tipoId)) {
                    tamanho += l->tamanho;
                }
                l = l->next;
            }
        }
    }
    return tamanho;
}

/* Procedure printSymTab prints a formatted 
 * listing of the symbol table contents 
 * to the listing file
 */
void printSymTab(FILE * listing)
{ int i;
  char *g = "global";
  fprintf(listing,"--------------------------------------------------------------------------------------------------------------\n");
  fprintf(listing,"    ID Name      ||     Scope     || Location ||  ID Type    || Data Type ||   Number of the Lines     \n");
  fprintf(listing,"---------------- || ------------- || -------- || ----------- || --------- || ---------------------------------\n");
  for (i=0;i<SIZE;++i)
  { if (hashTable[i] != NULL)
    { BucketList l = hashTable[i];
      while (l != NULL)
      { LineList t = l->lines;
        fprintf(listing,"    %-13s", l->name);
        if(strcmp(l->escopo, "") == 0)
          fprintf(listing,"||    %-10s ||", g);
        else
          fprintf(listing,"||    %-10s ||", l->escopo);
        fprintf(listing,"   %-7d||  ", l->memloc);
        if (strcmp(l->tipoId, "Variável") == 0)
          fprintf(listing,"%-12s||   ", l->tipoId);
        else if(strcmp(l->tipoId, "Função") == 0)
          fprintf(listing,"%-13s||   ", l->tipoId);
        else
          fprintf(listing,"%-11s||   ", l->tipoId);
        fprintf(listing,"%-7s ||", l->tipoDado);
        while (t != NULL)
        { fprintf(listing,"%5d", t->lineno);
          t = t->next;
        }
        fprintf(listing, "\n");
        l = l->next;
      }
    }
  }
  fprintf(listing,"-------------------------------------------------------------------------------------------------------------\n");
} /* printSymTab */
