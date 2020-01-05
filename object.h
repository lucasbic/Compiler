/****************************************************/
/*File: object.h                                 	*/
/*Autor: Lucas Bicalho Oliveira                    */
/****************************************************/

#ifndef _OBJECT_H_

#define _OBJECT_H_

#include "cgen.h"

typedef enum opcode { _ADD, _SUB, _MULT, _DIV, _INC, _DEC, _ADDI, _SUBI,
				      _AND, _OR, _NOT, _XOR, _SHL, SHR,
				      _BEQ, _BNE, _BLE, _BGR, _SLT, _JUMP, _JR, _JAL,
				      _NOP, _HALT, _LOAD, _STR, _LI, _MOVE,
				      _IN, _OUT, _SETSO, _SETPR, _TRGPC} Opcode;

typedef enum funct { _ADDF, _SUBF, _MULTF, _DIVF, _INCF, _DECF, _ADDIF, _SUBIF,
                      _ANDF, _ORF, _NOTF, _XORF, _SHLF, SHRF,
                      _BEQF, _BNEF, _BLEF, _BGRF, _SLTF, _JUMPF, _JRF, _JALF,
                      _NOPF, _HALTF, _LOADF, _STRF, _LIF, _MOVEF,
                      _INF, _OUTF, _SETSOF, _SETPRF, _TRGPCF} Funct;

/**
 *opcode_map mapeia todas posicoes dos enums de opcode, e utilizado para
 *encontrar opcodes em tempo de execucao
 */
static const enum opcode opcode_map[] = { _ADD, _SUB, _MULT, _DIV, _INC, _DEC, _ADDI, _SUBI,
									      _AND, _OR, _NOT, _XOR, _SHL, SHR,
									      _BEQ, _BNE, _BLE, _BGR, _SLT, _JUMP, _JR, _JAL,
									      _NOP, _HALT, _LOAD, _STR, _LI, _MOVE,
									      _IN, _OUT, _SETSO, _SETPR, _TRGPC};

/**
 *funct_map mapeia todas posicoes dos enums de funct, e utilizado para
 *encontrar functs em tempo de execucao
 */
static const enum funct funct_map[] = { _ADDF, _SUBF, _MULTF, _DIVF, _INCF, _DECF, _ADDIF, _SUBIF,
                                        _ANDF, _ORF, _NOTF, _XORF, _SHLF, SHRF,
                                        _BEQF, _BNEF, _BLEF, _BGRF, _SLTF, _JUMPF, _JRF, _JALF,
                                        _NOPF, _HALTF, _LOADF, _STRF, _LIF, _MOVEF,
                                        _INF, _OUTF, _SETSOF, _SETPRF, _TRGPCF};

typedef enum type {
    TYPE_R, TYPE_I, TYPE_J, TYPE_IO
} Type;

typedef enum registerName {
    $rz, $v0, $out, $sp, $inv, $gp, $fp, $a0,
    $a1, $a2, $a3, $s0, $s1, $s2, $s3, $s4,
    $s5, $s6, $s7, $s8, $s9, $t0, $t1, $t2,
    $t3, $t4, $t5, $t6, $t7, $t8, $t9, $ra
} RegisterName;

typedef enum addressingType {
    IMEDIATO, REGISTRADOR, INDEXADO, LABEL  //Imed, Dir por reg e Desloc
} AddressingType;

typedef struct instOperand {
    union {
        int imediato;
        RegisterName registrador;
        struct {
            RegisterName registrador;
            int offset;
        } indexado;
        char *label;
    } enderecamento;
    AddressingType tipoEnderecamento;
} *InstOperand;

typedef struct escopoGerador {
    int argRegCount;
    int savedRegCount;
    int tempRegCount;
    int quantidadeParametros;
    int quantidadeVariaveis;
    int tamanhoBlocoMemoria;
    const char *nome;
    struct registrador *regList;
    struct escopoGerador *next;
} *EscopoGerador;

typedef struct registrador {
    Operand op;
    RegisterName regName;
    struct registrador *next;
} *Registrador;

typedef struct objeto {
    Opcode opcode;
    Funct funct;
    Type type;
    InstOperand op1;
    InstOperand op2;
    InstOperand op3;
    struct objeto *next;
} *Objeto;

typedef struct label {
    char *nome;
    int linha;
    struct label *next;
} *Label;

const char *toStringOpcode(enum opcode op);

EscopoGerador createEscopoGerador(const char *);

void pushEscopoGerador(EscopoGerador eg);

void popEscopoGerador(void);

Registrador createRegistrador(Operand op, RegisterName regName);

void insertRegistrador(Registrador r);

void removeRegistrador(RegisterName name);

Registrador getRegistrador(RegisterName name);

void moveRegistrador(RegisterName dest, RegisterName orig);

InstOperand getRegByName(char *name);

void geraCodigoObjeto(Quadruple q);

void printCode(Objeto instrucao);

Objeto createObjInst(Opcode opcode, Funct funct, Type type, InstOperand op1, InstOperand op2, InstOperand op3);

Objeto insertObjInst(Objeto obj);

Objeto getCodigoObjeto(void);

void preparaRegistradoresEspeciais(void);

InstOperand getImediato(int valor);

InstOperand getOperandLabel(char *name);

Label createLabel(char *nome, int linha);

void insertLabel(char *nome, int linha);

int getLinhaLabel(char *nome);

#endif