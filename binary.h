/****************************************************/
/* File: binary.h                                 	*/
/* Autor: Lucas Bicalho Oliveira                    */
/****************************************************/

#ifndef _BINARY_H_

#define _BINARY_H_

#include "object.h"

void geraCodigoBinario(Objeto codigoObjeto);

const char *toBinaryOpcode(Opcode op);

const char *toBinaryFunct(Funct fct);

#endif