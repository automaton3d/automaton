/*
 * printCell.c
 *
 *  Created on: 9 de fev. de 2023
 *      Author: Alexandre
 */

#include <stdio.h>
#include "test.h"

void printCell(Cell *cell)
{
	printf("ch\t=0x%02x\n", cell->ch);
	printf("a\t=%d\n", cell->a);
	printf("t\t=%d\n", cell->t);
	printf("f\t=%d\n", cell->f);
	printf("syn\t=%d\n", cell->syn);
	printf("noise\t=%d\n", cell->seed);
	printf("o\t=%d,%d,%d\n\n", cell->o[0], cell->o[1], cell->o[2]);
}

