/*
 * printCell.c
 *
 *  Created on: 9 de fev. de 2023
 *      Author: Alexandre
 */

#include <stdio.h>
#include "test.h"

boolean first = true;
void printCell(Cell *cell)
{
	if(cell->offE == 0)
	{
		if(first)
		{
			puts("\nn\tch\tpos\toff\ta\tk\tsyn\tnoise\to\tp\tpo");
			puts("---------------------------------------------------------------------------------");
		}
		printf("%d\t", cell->n);
		printf("0x%02x\t", cell->ch);
		printf("%d\t", cell->offL);
		printf("%d\t", cell->offE);
		printf("%d\t", cell->a);
		printf("%d\t", cell->k);
		printf("%d\t", cell->syn);
		printf("%d\t", cell->r);
		printf("%d,%d,%d\t", cell->o[0], cell->o[1], cell->o[2]);
		printf("%d,%d,%d\t", cell->p[0], cell->p[1], cell->p[2]);
		printf("%d,%d,%d\n", cell->po[0], cell->po[1], cell->po[2]);
		first = false;
	}
}

