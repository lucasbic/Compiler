/****************************************************/
/* File: object.c                                	*/
/* Autor: Lucas Bicalho Oliveira                    */
/****************************************************/

#include "object.h"
#include "globals.h"
#include "cgen.h"
#include "code.h"
#include "symtab.h"
#include "util.h"

/* Macros para aumentar e diminuir o espacamento da identacao */
#define INDENT indent+=4
#define UNINDENT indent-=4

/* Cabeca da lista de instrucoes objeto */
Objeto objHead = NULL;

/* Cabeca da lista de labels */
Label labelHead = NULL;

/* Escopo atual */
EscopoGerador escopoHead = NULL;

/* Variavel usada para configurar a identacao do codigo impresso */
static int indent = 0;

/* Variaveis auxiliares na geracao de codigo objeto */
static char temp[100];
static int linha = 0;

 /* Registradores especiais */
InstOperand rtnAddrReg;
InstOperand rtnValReg;
InstOperand stackReg;
InstOperand outputReg;
InstOperand rzero;
InstOperand globalReg;

RegisterName tempReg[10] = {
    $t0, $t1, $t2, $t3, $t4, $t5, $t6, $t7, $t8, $t9
};

RegisterName savedReg[10] = {
    $s0, $s1, $s2, $s3, $s4, $s5, $s6, $s7, $s8, $s9
};

RegisterName argReg[4] = {
    $a0, $a1, $a2, $a3
};

const char * toStringOpcode(Opcode op) {
    const char * strings[] = {
        "add", "sub", "mult", "div", "inc", "dec", "addi", "subi",
		"and", "or", "not", "xor", "shl", "shr",
		"beq", "bne", "ble", "bgr", "slt", "jump", "jr", "jal",
		"nop", "halt",  "load", "str", "li", "move",
		"in", "out", "setso", "setpr", "trgpc"
    };
    return strings[op];
}

const char * toStringRegName(RegisterName rn) {
    const char * strings[] = {
    "$rz", "$v0", "$out", "$sp", "$inv", "$gp", "$fp", "$a0",
	"$a1", "$a2", "$a3", "$s0", "$s1", "$s2", "$s3", "$s4",
	"$s5", "$s6", "$s7", "$s8", "$s9", "$t0", "$t1", "$t2",
	"$t3", "$t4", "$t5", "$t6", "$t7", "$t8", "$t9", "$ra"
    };
    return strings[rn];
}

InstOperand getTempReg(int i) {
    /* Se ja tiver usado todos registradores temporarios volta a usar do inicio sem fazer nenhuma
     * verificacao adicional, pois os registradores temporarios nao garantem persistencia dos dados
     */
    if(i > 7) {
        escopoHead->tempRegCount = 0;
        i = 0;
    }
    InstOperand operando = (InstOperand) malloc(sizeof(struct instOperand));
    operando->tipoEnderecamento = REGISTRADOR;
    operando->enderecamento.registrador = tempReg[i];
    return operando;
}

InstOperand getLastTempReg(int i) {
    /* Usa o reg temporario $t9 para armazenar o contexto
     */
    InstOperand operando = (InstOperand) malloc(sizeof(struct instOperand));
    operando->tipoEnderecamento = REGISTRADOR;
    operando->enderecamento.registrador = tempReg[i];
    return operando;
}

InstOperand getSavedReg(int i) {
    /* Se ja tiver usado todos registradores temporarios volta a usar do inicio, mas antes de usar o registrador
     * deve salvar seu valor antigo na memoria
     */
    if(i > 9) {
        escopoHead->savedRegCount = 0;
        i = 0;
    }
    InstOperand operando = (InstOperand) malloc(sizeof(struct instOperand));
    operando->tipoEnderecamento = REGISTRADOR;
    operando->enderecamento.registrador = savedReg[i];
    return operando;
}

InstOperand getArgReg(int i) {
    InstOperand operando = (InstOperand) malloc(sizeof(struct instOperand));
    operando->tipoEnderecamento = REGISTRADOR;
    operando->enderecamento.registrador = argReg[i];
    if(i > 3) {
        operando->enderecamento.registrador = $inv;
    }
    return operando;
}

InstOperand getMemLocation(RegisterName registrador) {
    // Operando que representa o modo de enderecamento indexado
    InstOperand operando = (InstOperand) malloc(sizeof(struct instOperand));
    operando->tipoEnderecamento = INDEXADO;
    operando->enderecamento.indexado.offset = 0;
    operando->enderecamento.indexado.registrador = registrador;
    return operando;
}

InstOperand getMemIndexedLocation(RegisterName registrador, int offset) {
    // Operando que representa o modo de enderecamento indexado
    InstOperand operando = (InstOperand) malloc(sizeof(struct instOperand));
    operando->tipoEnderecamento = INDEXADO;
    operando->enderecamento.indexado.offset = offset;
    operando->enderecamento.indexado.registrador = registrador;
    return operando;
}

/* Busca a posicao de memoria do operando op, e retorna sua posicao com offset na stack */
InstOperand getStackOperandLocation(Operand op) {
    int memloc = getMemoryLocation(op.contents.variable.name, op.contents.variable.scope);
    //printf("\nteste\n");
    int offset = memloc - (escopoHead->tamanhoBlocoMemoria - 1);
    // Operando que representa o modo de enderecamento indexado
    InstOperand operando = (InstOperand) malloc(sizeof(struct instOperand));
    operando->tipoEnderecamento = INDEXADO;
    operando->enderecamento.indexado.offset = offset;
    operando->enderecamento.indexado.registrador = $sp;
    return operando;
}

InstOperand getGlobalOperandLocation(Operand op) {
    int offset = getMemoryLocation(op.contents.variable.name, op.contents.variable.scope);
    // Operando que representa o modo de enderecamento indexado
    InstOperand operando = (InstOperand) malloc(sizeof(struct instOperand));
    operando->tipoEnderecamento = INDEXADO;
    operando->enderecamento.indexado.offset = offset;
    operando->enderecamento.indexado.registrador = $gp;
    return operando;
}

InstOperand getProgOperandLocation(Operand op) {
    int offset = getMemoryLocation(op.contents.variable.name, op.contents.variable.scope);
    // Operando que representa o modo de enderecamento indexado
    InstOperand operando = (InstOperand) malloc(sizeof(struct instOperand));
    operando->tipoEnderecamento = INDEXADO;
    operando->enderecamento.indexado.offset = offset;
    operando->enderecamento.indexado.registrador = $t9;
    return operando;
}

/* Retorna a posicao da stack de acordo com o offset passado por parametro */
InstOperand getStackLocation(int offset) {
    // Operando que representa o modo de enderecamento indexado
    InstOperand operando = (InstOperand) malloc(sizeof(struct instOperand));
    operando->tipoEnderecamento = INDEXADO;
    operando->enderecamento.indexado.offset = offset;
    operando->enderecamento.indexado.registrador = $sp;
    return operando;
}

void pushStackSpace(int n) {
    printCode(insertObjInst(createObjInst(_ADDI, _ADDIF, TYPE_I, stackReg, stackReg, getImediato(n))));
}

void popStackSpace(int n) {
    printCode(insertObjInst(createObjInst(_SUBI, _SUBIF, TYPE_I, stackReg, stackReg, getImediato(n))));
}

InstOperand getOperandRegName(Operand op) {
    InstOperand rs, rt;
    char * regName;
    Operand operand;

    /* OPERANDO */
    if(op.kind == String) { /* Registrador */
        rs = getRegByName(op.contents.variable.name);
        if(rs == NULL) {
            if(op.contents.variable.scope == NULL) { // Scope e nulo, entao e um temporario
                rs = getTempReg(escopoHead->tempRegCount++);
                insertRegistrador(createRegistrador(op, rs->enderecamento.registrador));
                printCode(insertObjInst(createObjInst(_LOAD, _LOADF, TYPE_I, rs, getRegByName(op.contents.variable.name), NULL)));
            } else if(op.contents.variable.scope == globalScope) {
                rs = getSavedReg(escopoHead->savedRegCount++);
                insertRegistrador(createRegistrador(op, rs->enderecamento.registrador));
                printCode(insertObjInst(createObjInst(_LOAD, _LOADF, TYPE_I, rs, getGlobalOperandLocation(op), NULL)));
            } else { // Scope nao e nulo, entao e uma variavel e deve ser lida da memoria
                rs = getSavedReg(escopoHead->savedRegCount++);
                insertRegistrador(createRegistrador(op, rs->enderecamento.registrador));

                printCode(insertObjInst(createObjInst(_LOAD, _LOADF, TYPE_I, rs, getStackOperandLocation(op), NULL)));
            }
        }
    } else { /* Valor Imediato */
        // Le um valor imediato em um registrador
        rt = getImediato(op.contents.val);
        rs = getTempReg(escopoHead->tempRegCount++);
        regName = (char *) toStringRegName(rs->enderecamento.registrador);
        operand.kind = String;
        operand.contents.variable.name = regName;
        operand.contents.variable.scope = NULL;
        insertRegistrador(createRegistrador(operand, rs->enderecamento.registrador));
        printCode(insertObjInst(createObjInst(_LI, _LIF, TYPE_I, rs, rt, NULL)));
    }
    return rs;
}

InstOperand getVectorRegName(Operand op) {
    InstOperand reg = getRegByName(op.contents.variable.name);
    if(reg == NULL) {
        reg = getSavedReg(escopoHead->savedRegCount++);
        insertRegistrador(createRegistrador(op, reg->enderecamento.registrador));
        if(op.contents.variable.scope == globalScope) {
            // Le o endereco de memoria do inicio do vetor
            printCode(insertObjInst(createObjInst(_LI, _LIF, TYPE_I, reg, getGlobalOperandLocation(op), NULL)));
        } else {
            // Le o endereco de memoria do inicio do vetor
            printCode(insertObjInst(createObjInst(_LI, _LIF, TYPE_I, reg, getStackOperandLocation(op), NULL)));
        }
    }
    return reg;
}

InstOperand getTempRegName(Operand op) {
    InstOperand reg;
    // Cria registrador temporario para armazenar o resultado da expressao
    reg = getTempReg(escopoHead->tempRegCount++);
    insertRegistrador(createRegistrador(op, reg->enderecamento.registrador));
    return reg;
}

InstOperand getTempRegNameOut(Operand op) {
    InstOperand reg;
    // Cria registrador temporario para armazenar o resultado da expressao
    reg = getTempReg(escopoHead->tempRegCount--);
    insertRegistrador(createRegistrador(op, reg->enderecamento.registrador));
    return reg;
}

void geraCodigoInstrucaoAritmetica(Quadruple q, Opcode op, Funct fct) {
    InstOperand op1, op2, op3;

    /* Busca ou atribui o registrador do operando 1 */
    op1 = getOperandRegName(q->op1);
    /* Atribui um registrador para o resultado da expressao */
    op3 = getTempRegName(q->op3);

    /* OPERANDO 2 */
    if(q->op2.kind == String) { /* Registrador */
        /* Busca ou atribui o registrador do operando 2 */
        op2 = getOperandRegName(q->op2);
        /* Imprime a instrucao aritmetica */
        printCode(insertObjInst(createObjInst(op, fct, TYPE_R, op3, op1, op2)));
    } else { /* Valor Imediato */
        // Valor imediato
        op2 = getImediato(q->op2.contents.val);
        // nextInstruction e a versao imediato da instrucao atual
        int nextInstruction = op + 1;
        /* Imprime a instrucao aritmetica, versao imediato */
        printCode(insertObjInst(createObjInst(opcode_map[nextInstruction], funct_map[nextInstruction], TYPE_I, op3, op1, op2)));
    }
}

void geraCodigoInstrucaoLogica(Quadruple q, Opcode op, Funct fct, Operand label) {
    InstOperand op1, op2, op3;

    /* Busca ou atribui o registrador do operando 1 */
    op1 = getOperandRegName(q->op1);
    /* Label para o qual ira ser feito o desvio */
    op3 = getOperandLabel(label.contents.variable.name);
    /* OPERANDO 2 */
    if(q->op2.kind == String) { /* Registrador */
        /* Busca ou atribui o registrador do operando 2 */
        op2 = getOperandRegName(q->op2);
        /* Imprime a instrucao aritmetica */
        printCode(insertObjInst(createObjInst(op, fct, TYPE_I, op1, op2, op3)));
    } else { /* Valor Imediato */
        // Le o valor imediato
        op2 = getOperandRegName(q->op2);
        /* Imprime a instrucao logica */
        printCode(insertObjInst(createObjInst(op, fct, TYPE_I, op1, op2, op3)));
    }
}

void geraCodigoInstrucaoAtribuicao(Quadruple q) {
    InstOperand reg = getOperandRegName(q->op2);
    if(q->op1.contents.variable.scope == NULL) {
        // Vetor com indice do acesso igual a uma variavel
        InstOperand r = getRegByName(q->op1.contents.variable.name);
        printCode(insertObjInst(createObjInst(_STR, _STRF, TYPE_I, reg, getMemLocation(r->enderecamento.registrador), NULL)));
    } else if(q->op1.contents.variable.scope == globalScope) {
        if(q->op3.kind != Empty) {
            // Vetor com indice de acesso igual a uma constante
            InstOperand r = getOperandRegName(q->op1);
            printCode(insertObjInst(createObjInst(_STR, _STRF, TYPE_I, reg, getMemIndexedLocation(r->enderecamento.registrador, q->op3.contents.val), NULL)));
        } else {
            // Variavel comum
            printCode(insertObjInst(createObjInst(_STR, _STRF, TYPE_I, reg, getGlobalOperandLocation(q->op1), NULL)));
            /* Remove o registrador da lista, para forcar um novo LOAD ao usar a variavel que foi recentemente alterada na memoria */
            InstOperand regAux = getRegByName(q->op1.contents.variable.name);
            if(regAux != NULL) {
                removeRegistrador(regAux->enderecamento.registrador);
            }
        }
    } else {
        if(q->op3.kind != Empty) {
            // Vetor com indice de acesso igual a uma constante
            InstOperand r = getOperandRegName(q->op1);
            printCode(insertObjInst(createObjInst(_STR, _STRF, TYPE_I, reg, getMemIndexedLocation(r->enderecamento.registrador, q->op3.contents.val), NULL)));
        } else {
            // Variavel comum
            printCode(insertObjInst(createObjInst(_STR, _STRF, TYPE_I, reg, getStackOperandLocation(q->op1), NULL)));
            /* Remove o registrador da lista, para forcar um novo LOAD ao usar a variavel que foi recentemente alterada na memoria */
            InstOperand regAux = getRegByName(q->op1.contents.variable.name);
            if(regAux != NULL) {
                removeRegistrador(regAux->enderecamento.registrador);
            }
        }
    }
}

void geraCodigoChamadaFuncao(Quadruple q) {
    int tamanhoBlocoMemoria;
    /* Verifica o nome da funcao/procedimento que esta sendo chamada, se for input ou output imprime as
     * instrucoes especificas 'in' e 'out'. Depois verifica o escopo de onde vem a chamada, se for do
     * escopo da 'main' nao guarda $ra na memoria, caso contrario guarda $ra na memoria.
     */
    if(!strcmp(q->op1.contents.variable.name, "input")) {
        printCode(insertObjInst(createObjInst(_IN, _INF, TYPE_IO, getTempRegName(q->op3), NULL, NULL)));
        printCode(insertObjInst(createObjInst(_TRGPC, _TRGPCF, TYPE_I, NULL, NULL, NULL)));
    } else if(!strcmp(q->op1.contents.variable.name, "output")) {
        printCode(insertObjInst(createObjInst(_OUT, _OUTF, TYPE_IO, getTempRegNameOut(q->op3), NULL, getImediato(q->display))));
        printCode(insertObjInst(createObjInst(_TRGPC, _TRGPCF, TYPE_I, NULL, NULL, NULL)));
    } else if(!strcmp(q->op1.contents.variable.name, "setSO")) {
        printCode(insertObjInst(createObjInst(_LI, _LIF, TYPE_I, getLastTempReg(9), getImediato(q->display), NULL))); //arrumar imediato
        printCode(insertObjInst(createObjInst(_SETSO, _SETSOF, TYPE_I, NULL, NULL, NULL)));
    } else if(!strcmp(q->op1.contents.variable.name, "setPr")) {
        printCode(insertObjInst(createObjInst(_LI, _LIF, TYPE_I, getLastTempReg(9), getImediato(q->display), NULL))); //arrumar imediato
        printCode(insertObjInst(createObjInst(_SETPR, _SETPRF, TYPE_I, NULL, NULL, NULL)));
    } else if(!strcmp(q->op1.contents.variable.name, "runPr")) {
        printCode(insertObjInst(createObjInst(_JUMP, _JUMPF, TYPE_J, getOperandLabel(q->op1.contents.variable.name), NULL, NULL)));
    } else if(!strcmp(q->op1.contents.variable.name, "retPr")) {
        printCode(insertObjInst(createObjInst(_JR, _JRF, TYPE_I,  NULL, getLastTempReg(8), NULL)));
    } /*else if(!strcmp(q->op1.contents.variable.name, "findRegs")) {
        printCode(insertObjInst(createObjInst(_FREG, _FREGF, TYPE_IO, getTempRegName(q->op3), NULL, getImediato(q->display))));
    } else if(!strcmp(q->op1.contents.variable.name, "saveRegs")) {
        printCode(insertObjInst(createObjInst(_SREG, _SREGF, TYPE_IO, getTempRegName(q->op3), NULL, getImediato(q->display))));
    }*/ else if(!strcmp(escopoHead->nome, "main")) {
        tamanhoBlocoMemoria = getTamanhoBlocoMemoriaEscopo(q->op1.contents.variable.name);
        printCode(insertObjInst(createObjInst(_JAL, _JALF, TYPE_J, getOperandLabel(q->op1.contents.variable.name), NULL, NULL)));
        printCode(insertObjInst(createObjInst(_MOVE, _MOVEF, TYPE_I, getTempRegName(q->op3), rtnValReg, NULL)));
        /* Desaloca o bloco de memoria na stack */
        popStackSpace(tamanhoBlocoMemoria + 1);
    } else {
        tamanhoBlocoMemoria = getTamanhoBlocoMemoriaEscopo(q->op1.contents.variable.name);
        printCode(insertObjInst(createObjInst(_STR, _STRF, TYPE_I, rtnAddrReg, getStackLocation(1), NULL))); // sw $ra
        printCode(insertObjInst(createObjInst(_JAL, _JALF, TYPE_J, getOperandLabel(q->op1.contents.variable.name), NULL, NULL)));
        popStackSpace(tamanhoBlocoMemoria + 1); // +1 devido ao registrador $ra
        printCode(insertObjInst(createObjInst(_LOAD, _LOADF, TYPE_I, rtnAddrReg, getStackLocation(1), NULL))); // lw $ra
        printCode(insertObjInst(createObjInst(_MOVE, _MOVEF, TYPE_I, getTempRegName(q->op3), rtnValReg, NULL)));
    }
}

void geraCodigoSetParam(Quadruple q) {
    InstOperand reg;
    BucketList var = NULL;
    /* Verifica se e uma variavel para recuperar o BucketList correspondente */
    if(q->op1.kind == String && q->op1.contents.variable.scope != NULL) {
        // Recupera o BucketList da variavel
        var = getVarFromSymtab(q->op1.contents.variable.name, q->op1.contents.variable.scope);
    }
    /* Se a chamada de funcao tiver ate 4 parametros, utiliza os registradores $a0 - $a3
     * caso contrario, o excedente deve ser armazenado na stack
     */
    if(escopoHead->argRegCount < 4) {
        /* Verifica se e uma constante ou variavel */
        if(q->op1.kind == String) { // Variavel
            if(var != NULL && var->treeNode->kind.exp == VectorK) { // Vetor
                printCode(insertObjInst(createObjInst(_LI, _LIF, TYPE_I, getArgReg(escopoHead->savedRegCount), getStackOperandLocation(q->op1), NULL)));
            } else { // Variavel
                reg = getOperandRegName(q->op1);
                if(getArgReg(escopoHead->argRegCount)->enderecamento.registrador != reg->enderecamento.registrador) { /* So move se os registradores forem diferentes */
                    printCode(insertObjInst(createObjInst(_MOVE, _MOVEF, TYPE_I, getArgReg(escopoHead->argRegCount), reg, NULL)));
                    moveRegistrador(getArgReg(escopoHead->argRegCount)->enderecamento.registrador, reg->enderecamento.registrador);
                }
            }
        } else { // Constante
            printCode(insertObjInst(createObjInst(_LI, _LIF, TYPE_I, getArgReg(escopoHead->argRegCount), getImediato(q->op1.contents.val), NULL)));
        }
        escopoHead->argRegCount++;
    } else { // TODO fazer o resto
        /* Verifica se e uma constante ou variavel */
        if(q->op1.kind == String) { // Variavel
            if(var != NULL && var->treeNode->kind.exp == VectorK) { // Vetor
                printCode(insertObjInst(createObjInst(_LI, _LIF, TYPE_I, getSavedReg(escopoHead->savedRegCount), getStackOperandLocation(q->op1), NULL)));
            } else { // Variavel
                printCode(insertObjInst(createObjInst(_LOAD, _LOADF, TYPE_I, getSavedReg(escopoHead->savedRegCount), getStackOperandLocation(q->op1), NULL)));
            }
        } else { // Constante
            printCode(insertObjInst(createObjInst(_LI, _LIF, TYPE_I, getSavedReg(escopoHead->savedRegCount), getImediato(q->op1.contents.val), NULL)));
        }
        printCode(insertObjInst(createObjInst(_STR, _STRF, TYPE_I, getSavedReg(escopoHead->savedRegCount), getStackOperandLocation(q->op1), NULL)));
        escopoHead->savedRegCount++;
    }
}

void geraCodigoGetParam(Quadruple q) {
    /* Se a chamada de funcao tiver ate 4 parametros, le os registradores $a0 - $a3
     * caso contrario, o excedente deve ser lido da stack
     */
    if(escopoHead->argRegCount < 4) {
        InstOperand arg = getArgReg(escopoHead->argRegCount);
        insertRegistrador(createRegistrador(q->op1, arg->enderecamento.registrador));
        InstOperand saved = getSavedReg(escopoHead->savedRegCount);
        insertRegistrador(createRegistrador(q->op1, saved->enderecamento.registrador));
        printCode(insertObjInst(createObjInst(_MOVE, _MOVEF, TYPE_I, saved, arg, NULL)));
        moveRegistrador(saved->enderecamento.registrador, arg->enderecamento.registrador);
        escopoHead->argRegCount++;
        escopoHead->savedRegCount++;
    } else if(escopoHead->argRegCount >= 4) {
        insertRegistrador(createRegistrador(q->op1, getSavedReg(escopoHead->savedRegCount)->enderecamento.registrador));
        printCode(insertObjInst(createObjInst(_LOAD, _LOADF, TYPE_I, getSavedReg(escopoHead->savedRegCount), getStackOperandLocation(q->op1), NULL)));
        escopoHead->savedRegCount++;
    }
}

void geraCodigoRetorno(Quadruple q) {
    InstOperand reg;
    /* So retorna valor se o escopo atual nao for o escopo da main */
    if(strcmp(escopoHead->nome, "main")) {
        /* Verifica se ha valor para ser retornado */
        if(q->op1.kind != Empty) {
            reg = getOperandRegName(q->op1);
            printCode(insertObjInst(createObjInst(_MOVE, _MOVEF, TYPE_I, rtnValReg, reg, NULL)));
        }
        printCode(insertObjInst(createObjInst(_JR, _JRF, TYPE_I, rtnAddrReg, NULL, NULL)));
    }
}

void geraCodigoVetor(Quadruple q) {
    InstOperand reg = getVectorRegName(q->op1);
    /* Verifica se o indice e constante ou variavel */
    if(q->op2.kind == String) {
        /* Le o valor da posicao do vetor em um registrador temporario */
        InstOperand temp = getTempRegName(q->op3);
        printCode(insertObjInst(createObjInst(_ADD, _ADDF, TYPE_R, temp, reg, getOperandRegName(q->op2))));
        printCode(insertObjInst(createObjInst(_LOAD, _LOADF, TYPE_I, temp, getMemIndexedLocation(temp->enderecamento.registrador, 0), NULL)));
    } else {
        printCode(insertObjInst(createObjInst(_LOAD, _LOADF, TYPE_I, getTempRegName(q->op3), getMemIndexedLocation(reg->enderecamento.registrador, q->op2.contents.val), NULL)));
    }
}

void geraCodigoEnderecoVetor(Quadruple q) {
    /* Le endereco do vetor */
    InstOperand reg = getVectorRegName(q->op1);
    /* Verifica se o indice e constante ou variavel */
    if(q->op2.kind == String) {
        /* Soma o endereco base do vetor com o valor da variavel */
        printCode(insertObjInst(createObjInst(_ADD, _ADDF, TYPE_R, getTempRegName(q->op3), reg, getOperandRegName(q->op2))));
    }
}

void geraCodigoFuncao(Quadruple q) {
    // Atribui fim de string para todas posicoes de temp, isso e feito pois o Procedimento
    // strcat so insere de forma correta strings inicializadas.
    memset(temp, '\0', sizeof(temp));
    strcat(temp, "\n");
    strcat(temp, q->op1.contents.variable.name);
    strcat(temp, ":");
    // Adiciona o label a proxima linha de instrucao
    insertLabel(q->op1.contents.variable.name, linha);
    emitCode(temp);
    pushEscopoGerador(createEscopoGerador(q->op1.contents.variable.name));
    if(!strcmp(escopoHead->nome, "main")) {
        /* Aloca o bloco de memoria na stack */
        pushStackSpace(escopoHead->tamanhoBlocoMemoria + getTamanhoBlocoMemoriaEscopoGlobal() - 1);
    } else {
        /* Aloca espaco na stack para os parametros + 1 para o registrador de endereco de retorno */
        pushStackSpace(escopoHead->tamanhoBlocoMemoria + 1); // +1 devido ao registrador $ra
    }
}

void geraCodigoLabel(Quadruple q) {
    // Atribui fim de string para todas posicoes de temp, isso e feito pois o Procedimento
    // strcat so insere de forma correta strings inicializadas.
    memset(temp, '\0', sizeof(temp));
    strcat(temp, ".");
    strcat(temp, q->op1.contents.variable.name);
    strcat(temp, "\t");
    // Adiciona o label a proxima linha de instrucao
    insertLabel(q->op1.contents.variable.name, linha);
    emitCode(temp);
}

void printCode(Objeto instrucao) {
    char aux[20];
    bzero(aux, 20);
    memset(temp, '\0', sizeof(temp));
    //printf("%s:\t",temp);
    printf("\t%d:\t", linha++);
    strcat(temp, toStringOpcode(instrucao->opcode));
    strcat(temp, "\t");
    //printf("\nteste\n");
    if(instrucao->op1 != NULL) {
        if(instrucao->op1->tipoEnderecamento == IMEDIATO) {
            sprintf(aux, "%d", instrucao->op1->enderecamento.imediato);
            //printf("%s:\t",aux);
            //printf("%d:\t", instrucao->op1->enderecamento.imediato);
            strcat(temp, aux);
        } else if(instrucao->op1->tipoEnderecamento == REGISTRADOR) {
            strcat(temp, toStringRegName(instrucao->op1->enderecamento.registrador));
        } else if(instrucao->op1->tipoEnderecamento == INDEXADO) {
            sprintf(aux, "%d(%s)", instrucao->op1->enderecamento.indexado.offset, toStringRegName(instrucao->op1->enderecamento.indexado.registrador));
            strcat(temp, aux);
        } else { // Label
            strcat(temp, instrucao->op1->enderecamento.label);
        }
    }
    if(instrucao->op2 != NULL) {
        strcat(temp, ", ");
        if(instrucao->op2->tipoEnderecamento == IMEDIATO) {
            sprintf(aux, "%d", instrucao->op1->enderecamento.imediato);
            //printf("%s:\t",aux);
            //printf("%d:\t", instrucao->op1->enderecamento.imediato);
            strcat(temp, aux);
        } else if(instrucao->op2->tipoEnderecamento == REGISTRADOR) {
            strcat(temp, toStringRegName(instrucao->op2->enderecamento.registrador));
        } else if(instrucao->op2->tipoEnderecamento == INDEXADO) {
            sprintf(aux, "%d(%s)", instrucao->op2->enderecamento.indexado.offset, toStringRegName(instrucao->op2->enderecamento.indexado.registrador));
            strcat(temp, aux);
        } else { // Label
            strcat(temp, instrucao->op2->enderecamento.label);
        }
    }
    if(instrucao->op3 != NULL) {
        strcat(temp, ", ");
        if(instrucao->op3->tipoEnderecamento == IMEDIATO) {
            sprintf(aux, "%d", instrucao->op1->enderecamento.imediato);
            //printf("%s:\t",aux);
            //printf("%d:\t", instrucao->op1->enderecamento.imediato);
            strcat(temp, aux);
        } else if(instrucao->op3->tipoEnderecamento == REGISTRADOR) {
            strcat(temp, toStringRegName(instrucao->op3->enderecamento.registrador));
        } else if(instrucao->op3->tipoEnderecamento == INDEXADO) {
            sprintf(aux, "%d(%s)", instrucao->op3->enderecamento.indexado.offset, toStringRegName(instrucao->op3->enderecamento.indexado.registrador));
            strcat(temp, aux);
        } else { // Label
            strcat(temp, instrucao->op3->enderecamento.label);
        }
    }
    emitObjectCode(temp, indent);
}

void geraCodigoObjeto(Quadruple q) {
    INDENT;
    printf("\n********** Codigo objeto **********");
    // Antes de comecar gerar codigo objeto, prepara os registradores especiais
    preparaRegistradoresEspeciais();
    while(q != NULL) {
        //printf("\ninstruction: %d\n", q->instruction);
        switch (q->instruction) {
            case 0: //ADD
                //printf("\nADD\n");
                geraCodigoInstrucaoAritmetica(q, _ADD, _ADDF);
                break; /* ADD */

            case 1: //SUB
                //printf("\nSUB\n");
                geraCodigoInstrucaoAritmetica(q, _SUB, _SUBF);
                break; /* SUB */

            case 2: //MULT
                //printf("\nMULT\n");
                geraCodigoInstrucaoAritmetica(q, _MULT, _MULTF);
                break; /* MULT */

            case 3: //DIV
                //printf("\nDIV\n");
                geraCodigoInstrucaoAritmetica(q, _DIV, _DIVF);
                break; /* DIV */

            case 4: //VEC
                //printf("\nVEC\n");
                geraCodigoVetor(q);
                break; /* VEC */

            case 5: //EQL
                //printf("\nEQL\n");
                geraCodigoInstrucaoLogica(q, _BEQ, _BEQF, q->next->op2);
                break; /* BEQ */

            case 6: //NE
                //printf("\nNE\n");
                geraCodigoInstrucaoLogica(q, _BNE, _BNEF, q->next->op2);
                break; /* BNE */

            case 7: //LT
                //printf("\nLT\n");
                geraCodigoInstrucaoLogica(q, _BLE, _BLEF, q->next->op2);
                break; /* BLT */

            case 9: //GT
                //printf("\nGT\n");
                geraCodigoInstrucaoLogica(q, _BGR, _BGRF, q->next->op2);
                break; /* BGT */

            case 11: //ASNG
                //printf("\nASNG\n");
                geraCodigoInstrucaoAtribuicao(q);
                break; /* ASN */

            case 21: //VEC_ADDR
                //printf("\nVEC_AD\n");
                geraCodigoEnderecoVetor(q);
                break; /* VEC_ADDR */

            case 12: //FUNC
                //printf("\nFUNC\n");
                geraCodigoFuncao(q);
                escopoHead->savedRegCount = 0;
                break; /* FUNC */

            case 13: //RET
                //printf("\nRET\n");
                geraCodigoRetorno(q);
                break; /* RTN */

            case 22: //GET_PARAM
                //printf("\nGET_PARAM\n");
                geraCodigoGetParam(q);
                break; /* GET_PARAM */

            case 23: //SET_PARAM
                //printf("\nSET_PARAM\n");
                geraCodigoSetParam(q);
                break; /* SET_PARAM */

            case 15: //CALL
                //printf("\nCALL\n");
                geraCodigoChamadaFuncao(q);
                escopoHead->argRegCount = 0;
                break; /* CALL */

            case 17: //JPF
                //printf("\nJPF\n");
                //printCode(insertObjInst(createObjInst(_JUMP, _JUMPF, TYPE_J, getOperandLabel(q->op1.contents.variable.name), NULL, NULL)));
                break; /* GOTO */

            case 18: //GOTO
                //printf("\nGOTO\n");
                printCode(insertObjInst(createObjInst(_JUMP, _JUMPF, TYPE_J, getOperandLabel(q->op1.contents.variable.name), NULL, NULL)));
                break; /* GOTO */

            case 19: //LBL
                //printf("\nLBL\n");
                geraCodigoLabel(q);
                break; /* LBL */

            case 20: //HALT
                //printf("\nHALT\n");
                printCode(insertObjInst(createObjInst(_HALT, _HALTF, TYPE_J, NULL, NULL, NULL)));
                break; /* HALT */

            case 14: //PARAM
                //printf("\nPARAM\n");
                escopoHead->argRegCount = 0;
                break; /* PARAM_LIST */

            default:
                //printf("\ndefault\n");
                break;
        }
        q = q->next;
    }
}

EscopoGerador createEscopoGerador(const char * nome) {
    EscopoGerador eg = (EscopoGerador) malloc(sizeof(struct escopoGerador));
    eg->argRegCount = 0;
    eg->savedRegCount = 0;
    eg->tempRegCount = 0;
    // Recupera o BucketList do escopo
    //BucketList bl = st_bucket_func((char *) nome);
    /* Recupera o treeNode que representa o escopo para recuperar a quantidade de
     * parametros e variaveis*/
    //eg->quantidadeParametros = getQuantidadeParametros(bl->treeNode);
    //eg->quantidadeVariaveis = getQuantidadeVariaveis(bl->treeNode);
    eg->tamanhoBlocoMemoria = getTamanhoBlocoMemoriaEscopo((char *) nome);
    eg->nome = nome;
    eg->regList = NULL;
    eg->next = NULL;
    return eg;
}

void pushEscopoGerador(EscopoGerador eg) {
    if(escopoHead == NULL) {
        escopoHead = eg;
    } else {
        eg->next = escopoHead;
        escopoHead = eg;
    }
}

void popEscopoGerador() {
    if(escopoHead != NULL) {
        EscopoGerador eg = escopoHead;
        escopoHead = eg->next;
        free(eg);
    }
}

Registrador createRegistrador(Operand op, RegisterName regName) {
    Registrador r = (Registrador) malloc(sizeof(struct registrador));
    r->op = op;
    r->regName = regName;
    r->next = NULL;
    return r;
}

void insertRegistrador(Registrador r) {
    Registrador head;
    if(escopoHead != NULL) {
        head = escopoHead->regList;
    }

    if(head == NULL) { /* Escopo ainda nao tem nenhum registrador */
        escopoHead->regList = r;
    } else { /* Escopo ja tem registrador, entao coloca no fim da lista o novo registrador */
        Registrador temp = head;
        while(temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = r;
    }
}

void moveRegistrador(RegisterName dest, RegisterName orig) {
    Registrador origem = getRegistrador(orig);
    origem->regName = dest;
    removeRegistrador(dest);
}

void removeRegistrador(RegisterName name) {
    Registrador atual, anterior;
    if(escopoHead != NULL) {
        atual = escopoHead->regList;
    }
    /* Verifica se o primeiro deve ser removido */
    if(name == atual->regName) {
        escopoHead->regList = atual->next;
        free(atual);
        atual = NULL;
        return;
    }

    anterior = atual;
    atual = atual->next;

    /* Verifica o restante */
    while(atual != NULL && anterior != NULL) {
        if(name == atual->regName) {
            anterior->next = atual->next;
            free(atual);
            return;
        }
        anterior = atual;
        atual = atual->next;
    }
}

Registrador getRegistrador(RegisterName name) {
    Registrador reg;
    if(escopoHead != NULL) {
        reg = escopoHead->regList;
    }
    while(reg != NULL) {
        if(name == reg->regName) {
            return reg;
        }
        reg = reg->next;
    }
    return NULL;
}

InstOperand getRegByName(char * name) {
    Registrador reg;
    if(escopoHead != NULL) {
        reg = escopoHead->regList;
    }
    while(reg != NULL) {
        if(!strcmp(name, reg->op.contents.variable.name)) {
            InstOperand operando = (InstOperand) malloc(sizeof(struct instOperand));
            operando->tipoEnderecamento = REGISTRADOR;
            operando->enderecamento.registrador = reg->regName;
            return operando;
        }
        reg = reg->next;
    }
    return NULL;
}

void freeParamList(Registrador head) {
    Registrador temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}

Objeto createObjInst(Opcode opcode, Funct funct, Type type, InstOperand op1, InstOperand op2, InstOperand op3) {
    Objeto obj = (Objeto) malloc(sizeof(struct objeto));
    obj->opcode = opcode;
    obj->funct = funct;
    obj->type = type;
    obj->op1 = op1;
    obj->op2 = op2;
    obj->op3 = op3;
    obj->next = NULL;
    return obj;
}

Objeto insertObjInst(Objeto obj) {
    if(objHead == NULL) {
        objHead = obj;
    } else {
        Objeto temp = objHead;
        while(temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = obj;
    }
    return obj;
}

void preparaRegistradoresEspeciais(void) {
    // Registrador de endereco de retorno de funcao
    rtnAddrReg = (InstOperand) malloc(sizeof(struct instOperand));
    rtnAddrReg->tipoEnderecamento = REGISTRADOR;
    rtnAddrReg->enderecamento.registrador = $ra;
    // Registrador de valor de retorno
    rtnValReg = (InstOperand) malloc(sizeof(struct instOperand));
    rtnValReg->tipoEnderecamento = REGISTRADOR;
    rtnValReg->enderecamento.registrador = $v0;
    // Registrador da stack
    stackReg = (InstOperand) malloc(sizeof(struct instOperand));
    stackReg->tipoEnderecamento = REGISTRADOR;
    stackReg->enderecamento.registrador = $sp;
    // Registrador para output
    outputReg = (InstOperand) malloc(sizeof(struct instOperand));
    outputReg->tipoEnderecamento = REGISTRADOR;
    outputReg->enderecamento.registrador = $out;
    // Registrador zero
    rzero = (InstOperand) malloc(sizeof(struct instOperand));
    rzero->tipoEnderecamento = REGISTRADOR;
    rzero->enderecamento.registrador = $rz;
    // Registrador para valores globais
    globalReg = (InstOperand) malloc(sizeof(struct instOperand));
    globalReg->tipoEnderecamento = REGISTRADOR;
    globalReg->enderecamento.registrador = $gp;
}

InstOperand getImediato(int valor) {
    InstOperand imediato = (InstOperand) malloc(sizeof(struct instOperand));
    imediato->tipoEnderecamento = IMEDIATO;
    imediato->enderecamento.imediato = valor;
    return imediato;
}

InstOperand getOperandLabel(char * name) {
    InstOperand label = (InstOperand) malloc(sizeof(struct instOperand));
    label->tipoEnderecamento = LABEL;
    label->enderecamento.label = name;
    return label;
}

Label createLabel(char * nome, int linha) {
    Label l = (Label) malloc(sizeof(struct label));
    l->nome = nome;
    l->linha = linha;
    l->next = NULL;
    return l;
}

void insertLabel(char * nome, int linha) {
    Label l = createLabel(nome, linha);
    if(labelHead == NULL) {
        labelHead = l;
    } else {
        Label temp = labelHead;
        while(temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = l;
    }
}

int getLinhaLabel(char * nome) {
    Label l;
    if(labelHead != NULL) {
        l = labelHead;
    }
    while(l != NULL) {
        if(!strcmp(nome, l->nome)) {
            return l->linha + 1; // +1 util no codigo binario
        }
        l = l->next;
    }
    return -1;
}

Objeto getCodigoObjeto(void) {
    return objHead;
}