/***************************************************	*/
/*File: cgen.c 											*/
/*Autor: Lucas Bicalho Oliveira 						*/
/***************************************************	*/

# include "globals.h"
# include "symtab.h"
# include "code.h"
# include "cgen.h"

/*Cabeca da Pilha de parametros de funcao */
ParamStack paramHead = NULL;

/*Cabeca da Lista de representacoes quadruplas */
Quadruple head = NULL;

/*Cabeca da Lista de representacoes quadruplas */
LocationStack locationHead = NULL;

/*Variable indentno is used by printTree to
*store current number of spaces to indent
*/
static int indent = 0;

/*macros to increase / decrease indentation */
# define INDENT indent +=4
# define UNINDENT indent -=4

/*main function location
*in the intermediate code instruction list
*/
static int mainLoc;

/*location number for current
*intermediate code instruction emission
*/
static int emitLoc = 0;

/*Numero para geracao de nomes de variaveis temporarias */
static int temporario = 1;

/*Numero para geracao de labels */
static int label = 1;

Operand operandoAtual;
/*Operando para representar vazio */
Operand vazio;
InstructionKind instrucaoAtual;

/*temporary string to help printing text */
static char tempString [20];

/*Prototipo para o gerador de codigo interno recursivo */
static void cGen( TreeNode *tree);

static char *createLabel() {
	char *temp =( char *) malloc(5);
	sprintf (temp , "L%d", label );
	//printf("%s", temp);
    //printf("L%d", label);
	++ label;
	return temp;
}

static char *createTemporaryOperandName() {
	char *temp =( char *) malloc(5);
	sprintf (temp , "t%d", temporario );
	//printf("%s", temp);
    //printf("t%d", temporario);
	++ temporario;
	return temp;
}

static Operand createTemporaryOperand(TreeNode *tree) {
	Operand temp;
	char *scope = tree->escopo;
	char *tempName = createTemporaryOperandName();
	temp.kind = String;
	temp.contents.variable.scope = scope;
	temp.contents.variable.name = tempName;
	return temp;
}

/*Procedure genStmt generates code at a statement node */
static void genStmt( TreeNode *tree) {
	INDENT;
	TreeNode *p1, *p2, *p3;
	Operand op1, op2, op3;
	Quadruple *q;
	//printf("\nentrou genStmt\n");
	//printf("\nkind stmt: %d\n", tree->kind.stmt);
	switch(tree->kind.stmt) {
		/*case CompK:
			p1 = tree->child [0];
			if(p1) { // Parametros 
			cGen(p1);
			}
			p2 = tree->child [1];
			if(p2) { // Expressoes
			cGen(p2);
			}
			break;*/

		case 0:	//IfK

			p1 = tree->child[0];
			p2 = tree->child[1];
			p3 = tree->child[2];

			//Gera codigo para a expressao de teste
			cGen(p1);

			//Atribui como o primeiro operando
			op1 = operandoAtual;

			//Atribui o tipo de instrucao
			instrucaoAtual = JPF;

			//Cria e insere uma nova representacao em codigo intermediario
			q = insertQuad( createQuad( instrucaoAtual, op1, vazio, vazio));

			/*Salva a IR do if para atualizar com o label que representa o fim do bloco then */
			pushLocation( createLocation(q));

			/*Atualiza emitLoc */
			++ emitLoc;

			/*Gera codigo para o bloco then */
			cGen(p2);

			op2.kind = String;
			op2.contents.variable.name = createLabel();
			op2.contents.variable.scope = NULL;
			updateLocation(op2);
			popLocation();

			if(p3 != NULL) {
				q = insertQuad( createQuad(GOTO, vazio, vazio, vazio));
				pushLocation( createLocation(q));
			}

			/*Label usado para marcar o fim do bloco then */
			insertQuad( createQuad(LBL, op2, vazio, vazio));
			/*Incrementa emitLoc */
			++ emitLoc;

			cGen(p3);

			if(p3 != NULL) {
				op1.kind = String;
				op1.contents.variable.name = createLabel();
				op1.contents.variable.scope = NULL;
				updateLocation(op1);
				popLocation();
				/*Label usado para marcar o fim do bloco then */
				insertQuad( createQuad(LBL, op1, vazio, vazio));
			}

			/*Label usado para marcar o fim do bloco else */
			/*Incrementa emitLoc */
			++ emitLoc;
			//printf("<- if: else / end block ");
			//printf("<- if");
			break;

		case 2:	//WhileK
			p1 = tree->child[0];
			p2 = tree->child[1];

			op1.kind = String;
			op1.contents.variable.name = createLabel();
			op1.contents.variable.scope = NULL;

			insertQuad( createQuad(LBL, op1, vazio, vazio));

			//Gera codigo para a expressao de teste
			cGen(p1);

			//Primeiro operando
			op2 = operandoAtual;
			instrucaoAtual = JPF;

			//Cria e insere uma nova representacao em codigo intermediario
			q = insertQuad( createQuad( instrucaoAtual, op2, vazio, vazio));

			//Salva a IR do if para atualizar com o label que representa o fim do bloco then
			pushLocation( createLocation(q));
			//Atualiza emitLoc
			++ emitLoc;

			//build code for while block
			cGen(p2);
			instrucaoAtual = GOTO;

			//Cria e insere uma nova representacao em codigo intermediario
			insertQuad( createQuad( instrucaoAtual, op1, vazio, vazio));

			//Incrementa emitLoc
			++ emitLoc;

			op3.kind = String;
			op3.contents.variable.name = createLabel();
			op3.contents.variable.scope = NULL;

			insertQuad( createQuad(LBL, op3, vazio, vazio));
			updateLocation(op3);
			popLocation();

			break;

		case 1:	//ReturnK
			//printf("->return ");
			p1 = tree->child[0];
			//printf("->return: expression ");
			/*Gera codigo para a expressao */
			cGen(p1);
			if(p1) {
				/*Atribui como o primeiro operando */
				op1 = operandoAtual;
			} else {
				/*Nao retorna nada */
				op1 = vazio;
			}
			/*Atribui o tipo de instrucao */
			instrucaoAtual = RET;
			/*Cria e insere uma nova representacao em codigo intermediario */
			insertQuad( createQuad( instrucaoAtual, op1, vazio, vazio));
			/*Incrementa emitLoc */
			++ emitLoc;
			//printf("<- return: expression ");
			//printf("<- return ");
			break;
		default:
			break;
	}
	UNINDENT;
} /*genStmt */

/*Procedure genExp generates code at an expression node */
static void genExp( TreeNode *tree) {
	INDENT;
	Quadruple q;
	TreeNode *p1, *p2;
	Operand op1, op2, op3;
	int qtdParams, display = -1;
	//printf("\nentrou genExp\n");
	//printf("\nkind exp: %d\n", tree->kind.exp);
	switch(tree->kind.exp) {
		case 1:	//ConstK
			//printf("->constant ");
			sprintf(tempString , "%d", tree->attr.val);
			emitComment ( tempString , indent );
			//printf("%s", tempString);
			//printf("%d", tree->attr.val);
			/*Atribui o operando atual */
			operandoAtual.kind = IntConst;
			operandoAtual.contents.val = tree->attr.val;
			//printf("<- constant ");
			break; /*ConstK */
		case 2:	//IdK
			//printf("->identifier ");
			emitComment (tree -> attr .name , indent );
			//sprintf (tempString, "%s", tree->attr.name);
			//printf("%s", tree->attr.name);
			/*Atribui o operando atual */
			operandoAtual.kind = String;
			operandoAtual.contents.variable.name = tree->attr.name;
			operandoAtual.contents.variable.scope = tree->escopo;
			//EMITIR UM LOAD PARA CARREGAR O REG DA MEM
			//printf("<- identifier ");
			break; /*IdK */
		case 7:	//VectorK
			if( strcmp(tree->tipoId, "Vetor ") != 0) {
				//printf("->vector ");
				//printf("%s", tree->attr.name);
				p1 = tree->child [0];
				/*Atualiza o operando atual como o id do vetor e seta como op1 */
				operandoAtual.kind = String;
				operandoAtual.contents.variable.name = tree->attr.name;
				operandoAtual.contents.variable.scope = tree->escopo;
				op1 = operandoAtual;
				/*Gera codigo para a posicao do vetor */
				//printf("->vector: position ");
				cGen(p1);
				/*Indice do vetor */
				op2 = operandoAtual;
				//printf("<- vector: position ");
				/*Atribui a instrucao atual */
				instrucaoAtual = VEC;
				/*Temporario */
				op3 = createTemporaryOperand(tree);
				/*Atualiza o operando atual */
				operandoAtual = op3;
				/*Cria e insere uma nova representacao em codigo intermediario */
				insertQuad( createQuad( instrucaoAtual, op1, op2, op3))
				;
				/*Atualiza emitLoc */
				++ emitLoc;
				//printf("<- vector ");
			}
			break; /*VectorK */
		case 4:	//FunctionK
			//printf("->function declaration ");
			//printf("%s", tree->attr.name);
			/*save location of function main */
			if((! strcmp(tree->attr.name, "main ")) &&(! strcmp(tree->escopo, "ESCOPO_GLOBAL "))) {
				mainLoc = emitLoc;
			}
			op1.kind = String;
			op1.contents.variable.name = tree->attr.name;
			op1.contents.variable.scope = tree->escopo;
			insertQuad( createQuad(FUNC, op1, vazio, vazio));
			p1 = tree;
			p2 = tree->child[1];
			cGen(p2);
			//printf("<- function declaration ");
			break;
		case 8:	//IdFunctionK
			//if (strcmp(tree->child[0]->attr.name, "void") != 0)
			//{
				//printf("->function call ");
				//printf("%s", tree->attr.name);
			//}
			/*if(strcmp(tree->attr.name, "input") == 0){

			}
			else if(strcmp(tree->attr.name, "output") == 0){
				
			}
			else{*/
				/*Argumentos */
				p1 = tree->child[0];
				/*Atribui o primeiro operando */
				op1.kind = String;
				op1.contents.variable.name = tree->attr.name;
				op1.contents.variable.scope = tree->escopo;
				/*Atribui o segundo operando */
				qtdParams = getQuantidadeArgumentos(tree);
				pushParam(& qtdParams);
				if( qtdParams > 0) {
				  op2.kind = IntConst;
				  op2.contents.val = qtdParams;
				} else {
				  op2.kind = IntConst;
				  op2.contents.val = 0;
				}
				/*build code for function call */
				instrucaoAtual = ARGS;
				insertQuad( createQuad( instrucaoAtual, vazio, vazio, vazio));
				/*Incrementa emitLoc */
				++ emitLoc;
				//printf("->function call: arguments ");
				while(p1 != NULL) {
				  cGen(p1);
				  /*Atribui o tipo de instrucao */
				  instrucaoAtual = PARAM;
				  /*Cria e insere uma nova representacao em codigo intermediario */
				  insertQuad( createQuad( instrucaoAtual, operandoAtual, vazio, vazio));
				  /*Incrementa emitLoc */
				  ++ emitLoc;
				  /*Decrementa qtdParams */
				  -- qtdParams;
				  /* Se for um chamado de OUTPUT, verifica o display de exibição */
				  if(strcmp(tree->attr.name, "output") == 0 && p1->sibling == NULL){
					display = p1->attr.val;
				  }
				  p1 = p1->sibling;
				}
				popParam();
				//printf("<- function call: arguments ");
				/*Atribui o tipo de instrucao */
				instrucaoAtual = CALL;
				/*Atualiza o operando atual */
				operandoAtual = createTemporaryOperand(tree);
				q = createQuad( instrucaoAtual, op1, op2, operandoAtual);
				if(display != -1) {
                	q->display = display;
            	}
				/*Cria e insere uma nova representacao em codigo
				  intermediario */
				insertQuad(q);
				/*Incrementa emitLoc */
				++ emitLoc;
			//}
			//printf("<- function call ");
			break;
		case 0:	//OpK
			if(tree->attr.op == ASSIGN) {
			  //printf("->assign ");
			  p1 = tree->child[0];
			  p2 = tree->child[1];
			  /*Gera codigo para o operando da esquerda */
			  //printf("->assign: left argument ");
			  cGen(p1);
			  /*Atribui como o primeiro operando */
			  op1 = operandoAtual;
			  //printf("<- assign: left argument ");
			  /*Gera codigo para o operando da direita */
			  //printf("->assign: right argument ");
			  cGen(p2);
			  /*Atribui como o segundo operando */
			  op2 = operandoAtual;
			  //printf("<- assign: right argument ");
			  /*Atribui o tipo de instrucao */
			  instrucaoAtual = ASNG;
			  /*Cria e insere uma nova representacao em codigo intermediario */
			  insertQuad( createQuad( instrucaoAtual, op1, op2, vazio));
			  /*Incrementa emitLoc */
			  ++ emitLoc;
			  //printf("<- assign ");
			} else {
			  //printf("->arithmetic operator ");
			  p1 = tree->child [0];
			  p2 = tree->child [1];
			  /*Gera codigo para o operando da esquerda */
			  //printf("->arithmetic operator: left argument ");
			  cGen(p1);
			  /*Atribui como o primeiro operando */
			  op1 = operandoAtual;
			  //printf("<- arithmetic operator: left argument ");
			  /*Gera codigo para o operando da direita */
			  //printf("->arithmetic operator: right argument ");
			  cGen(p2);
			  /*Atribui como o segundo operando */
			  op2 = operandoAtual;
			  //printf("<- arithmetic operator: right argument ");
			  switch(tree->attr.op) {
			    case PLUS:
			      //printf("arithmetic operator: +");
			      /*Atribui o tipo de instrucao */
			      instrucaoAtual = ADD;
			      break;
			    case MINUS:
			      //printf("arithmetic operator: -");
			      /*Atribui o tipo de instrucao */
			      instrucaoAtual = SUB;
			      break;
			    case TIMES:
			      //printf("arithmetic operator: *");
			      /*Atribui o tipo de instrucao */
			      instrucaoAtual = MULT;
			      break;
			    case OVER:
			      //printf("arithmetic operator: /");
			      /*Atribui o tipo de instrucao */
			      instrucaoAtual = DIV;
			      break;
			    case LT:
			      //printf("relational operator: <");
			      /*Atribui o tipo de instrucao */
			      instrucaoAtual = LTN;
			      break;
			    case LTEQ:
			      //printf("relational operator: <=");
			      /*Atribui o tipo de instrucao */
			      instrucaoAtual = LET;
			      break;
			    case GT:
			      //printf("relational operator: >");
			      /*Atribui o tipo de instrucao */
			      instrucaoAtual = GTN;
			      break;
			    case GTEQ:
			      //printf("relational operator: >=");
			      /*Atribui o tipo de instrucao */
			      instrucaoAtual = GET;
			      break;
			    case EQ:
			      //printf("relational operator: ==");
			      /*Atribui o tipo de instrucao */
			      instrucaoAtual = EQL;
			      break;
			    case NEQ:
			      //printf("relational operator: !=");
			      /*Atribui o tipo de instrucao */
			      instrucaoAtual = NE;
			      break;
			    default:
			      //printf("BUG: Unknown operator ");
			      break;
			  } /*case op */
			  /*Atualiza o operando atual */
			  operandoAtual = createTemporaryOperand(tree);
			  insertQuad( createQuad( instrucaoAtual, op1, op2, operandoAtual));
			  /*Incrementa emitLoc */
			  ++ emitLoc;
			  //printf("<- Operator ");
			}
			break; /*OpK */
		/*case 5:	//VariableK
			break;
		case IdVectorK:
			break;*/
		case 3:	//TypeK
			if(tree->type == Void){
				//printf("->type void");
				p1 = tree->child[0];
				cGen(p1);
				//printf("<- type void\n");
			}
			if(tree->type == Int){
				//printf("->type int");
				p1 = tree->child[0];
				cGen(p1);
				//printf("<- type int\n");
			}
			break;
		default:
			break;
	}
	UNINDENT;
} /*genExp */

/*Procedimento cGen gera o codigo recursivamente
*pela arvore sintatica
*/
static void cGen( TreeNode *tree) {
	if( tree != NULL) {
		switch(tree->nodekind) {
			case StmtK:
				genStmt(tree);
				break;
			case ExpK:
				genExp(tree);
				break;
			default:
				break;
		}
		/*Se a quantidade de parametros for maior que 0, cGen() sera
		chamado automaticamente */
		if( paramHead == NULL) {
			cGen(tree->sibling);
		}
		else {
			if( paramHead->count == 0) {
				cGen(tree->sibling);
			}
		}
	}
}

void verificaFimInstrucaoAnterior(void) {
    if(head != NULL) {
        Quadruple temp = head;
        while(temp->next != NULL) {
            temp = temp->next;
        }
        // Insere um return forcadamente caso nao haja no codigo de alto nivel
        if(temp->instruction != RET) {
            insertQuad(createQuad(RET, vazio, vazio, vazio));
        }
    }
}

/**********************************************/
/*the primary function of the code generator */
/**********************************************/
/*Procedure codeGen generates code to a code
*file by traversal of the syntax tree.The
*second parameter( codefile) is the file name
*of the code file, and is used to print the
*file name as a comment in the code file
*/
void codeGen( TreeNode *syntaxTree, char *codefile) {
	char *s =( char *) malloc( strlen( codefile) + 7);
	strcpy(s,"Arquivo: ");
	strcat(s, codefile);
	printf("Compilacao C- para codigo intermediario \n");
	printf("%s", s);
	/*Antes de iniciar, prepara o operando vazio */
	preparaVazio();
	cGen(syntaxTree);
	/*finish */
	printf("\nFim da execucao");
	insertQuad( createQuad(HALT, vazio, vazio, vazio));
	// Lista de instrucoes
	printf("\n\n**********Codigo intermediario**********\n\n");
	printIntermediateCode();
}

void printIntermediateCode() {
  Quadruple q = head;
  int linha = 0, i;
  char quad[100];
  while(q != NULL) {
  	sprintf (quad , "%d: (", linha ++);
    strcat(quad, toString(q->instruction));
    if(q->op1.kind == String) {
      strcat(quad, ", ");
      strcat(quad, q->op1.contents.variable.name);
    } else if(q->op1.kind == IntConst) {
      sprintf(tempString , ", %d", q->op1.contents.val);
      //printf("%s", tempString);
      //printf(", %d", q->op1.contents.val);
      strcat(quad, tempString);
    } else {
      strcat(quad, ", _");
    }
    if(q->op2.kind == String) {
      strcat(quad, ", ");
      strcat(quad, q->op2.contents.variable.name);
    } else if(q->op2.kind == IntConst) {
      sprintf(tempString , ", %d", q->op2.contents.val);
      //printf("%s", tempString);
      //printf(", %d", q->op2.contents.val);
      strcat(quad, tempString);
    } else {
      strcat(quad, ", _");
    }
    if(q->op3.kind == String) {
      strcat(quad, ", ");
      strcat(quad, q->op3.contents.variable.name);
      strcat(quad, ")");
    } else if(q->op3.kind == IntConst) {
      sprintf(tempString , ", %d", q->op3.contents.val);
      //printf("%s", tempString);
      //printf(", %d", q->op3.contents.val);
      strcat(quad, tempString);
      strcat(quad, ")");
    } else {
      strcat(quad, ", _)");
    }
    printf("%s\n", quad);
    q = q->next;
    for (i = 0; i < 100; i++)
    	quad[i] = '\0';
  }
  printf("\n");
}

void pushLocation( LocationStack ls) {
  if( locationHead == NULL) {
    locationHead = ls;
    locationHead->next = NULL;
  } else {
    ls->next = locationHead;
    locationHead = ls;
  }
}

void popLocation() {
	if( locationHead != NULL) {
		LocationStack ls = locationHead;
		locationHead = locationHead->next;
		free(ls);
		ls = NULL;
	}
}

LocationStack createLocation( Quadruple *quad) {
	LocationStack ls =( LocationStack) malloc( sizeof( struct Location)
	);
	ls->quad = quad;
	ls->next = NULL;
	return ls;
}

void updateLocation( Operand op) {
	Quadruple q = *locationHead->quad;
	if(q-> instruction != JPF) {
		q-> op1 = op;
	} else {
		q-> op2 = op;
	}
	*locationHead->quad = q;
}

void pushParam(int *count) {
	ParamStack ps =( ParamStack) malloc( sizeof( struct Param));
	ps->count = count;
	ps->next = NULL;
	if( paramHead == NULL) {
		paramHead = ps;
	} else {
		ps->next = paramHead;
		paramHead = ps;
	}
}

void popParam() {
	if( paramHead != NULL) {
		ParamStack ps = paramHead;
		paramHead = paramHead->next;
		free(ps);
		ps = NULL;
	}
}

Quadruple createQuad( InstructionKind instruction, Operand op1,
	Operand op2, Operand op3) {
	Quadruple q =( Quadruple) malloc( sizeof( struct Quad));
	q-> instruction = instruction;
	q-> op1 = op1;
	q-> op2 = op2;
	q-> op3 = op3;
	q-> next = NULL;
	return q;
}

Quadruple *insertQuad( Quadruple q) {
	Quadruple *ptr =( Quadruple *) malloc( sizeof( struct Quad));
	if( head == NULL) {
		head = q;
		head->next = NULL;
		ptr = & head;
	} else {
		Quadruple temp = head;
		while(temp->next != NULL) {
			temp = temp->next;
		}
		temp->next = q;
		temp ->next->next = NULL;
		ptr = &temp->next;
	}
	return ptr;
}

void preparaVazio() {
	vazio.kind = Empty;
	vazio.contents.variable.name = NULL;
	vazio.contents.variable.scope = NULL;
}

Quadruple getCodigoIntermediario(void) {
    return head;
}