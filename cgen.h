/****************************************************/
/*File: cgen.h                                  	*/
/*Autor: Lucas Bicalho Oliveira                    */
/****************************************************/

#include "globals.h"

#ifndef _CGEN_H_
#define _CGEN_H_

typedef enum{Empty, IntConst, String} OperandKind;

/*typedef enum instrucao{ADD, SUB, MULT, DIV, VEC, INC,
						BEQ, BNE, BLE, BGR, SLT, FUNC,
						JUMP, LOAD, STR, LI, MOVE, RET,
						IN, OUT, HALT, PRM, CALL, JPF, LBL, GOTO, ARGS, PARAM, ASNG} InstructionKind;*/

typedef enum instrucao {ADD, SUB, MULT, DIV, VEC, 
						EQL, NE, LTN, LET, GTN, GET, ASNG,
						FUNC, RET, PARAM, CALL, ARGS,
						JPF, GOTO, LBL, HALT, VEC_ADDR, GET_PARAM, SET_PARAM, IN, OUT, SETSO, SETPR} InstructionKind;

typedef struct{
	OperandKind kind;
	union{
		int val;
		struct{
			char *name;
			char *scope;
		} variable;
	} contents;
} Operand;

/*Estrutura Quadrupla que armazena os dados do codigo de tres
enderecos */
typedef struct Quad{
	InstructionKind instruction;
	Operand op1, op2, op3;
	int display;
	struct Quad *next;
} *Quadruple;

typedef struct Location{
	Quadruple *quad;
	struct Location *next;
} *LocationStack;

typedef struct Param{
	int *count;
	struct Param *next;
} *ParamStack;

Quadruple *insertQuad (Quadruple q);

Quadruple createQuad (	InstructionKind instruction, Operand op1,
						Operand op2, Operand op3);

void pushLocation (LocationStack ls);

void popLocation ();

LocationStack createLocation (Quadruple *quad);

void updateLocation (Operand op);

void pushParam (int *count);

void popParam ();

void preparaVazio ();

void printIntermediateCode ();

void codeGen (TreeNode *syntaxTree, char *codefile);

# endif