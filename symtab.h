/****************************************************/
/* File: symtab.h                                   */
/* Symbol table interface for the C- compiler     */
/* (allows only one symbol table)                   */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#include "globals.h"

/* SIZE is the size of the hash table */
#define SIZE 211

/* the list of line numbers of the source 
 * code in which a variable is referenced
 */
typedef struct LineListRec
   { int lineno;
     struct LineListRec * next;
   } * LineList;

/* The record in the bucket lists for
 * each variable, including name, 
 * assigned memory location, and
 * the list of line numbers in which
 * it appears in the source code
 */
typedef struct BucketListRec
   { char * name, * tipoId, * tipoDado, *escopo;
     LineList lines;
     TreeNode * treeNode;
     int memloc ; /* memory location for variable */
     int tamanho; /* Tamanho da variavel (util para vetores) */
     struct BucketListRec * next;
   } * BucketList;

typedef struct ScopeRec {
    char * funcName;
    struct ScopeRec * parent;
    int tamanhoBlocoMemoria; /* Tamanho do bloco de memoria alocado */
    BucketList hashTable[SIZE]; /* the hash table */
} * Scope;

static char * globalScope = "global";


/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
void st_insert( char * name,  char * tipoId, char *escopo, char * tipoDado,int lineno, int loc );

BucketList st_create(char * name, int lineno, int loc, TreeNode * treeNode, int tamanho);

void st_insert_func(char * name, int lineno, TreeNode * treeNode);


/* Function st_lookup returns the memory 
 * location of a variable or -1 if not found
 */
int st_lookup ( char * name, char * escopo );

char* st_lookup_type (char* name, char* escopo, char* tipoId);

BucketList st_bucket(char * name);

BucketList st_bucket_func (char * name);

void st_add_lineno(TreeNode * treeNode);

/* Procedure printSymTab prints a formatted 
 * listing of the symbol table contents 
 * to the listing file
 */
void printSymTab(FILE * listing);

#endif
