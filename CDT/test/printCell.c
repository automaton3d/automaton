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
	if(cell->off == 0)
	{
		if(first)
		{
			puts("\nn\tch\toff\ta\tpos\tk\tsyn\tnoise\to\tp");
			puts("---------------------------------------------------------------------------------");
		}
		printf("%d\t", cell->n);
		printf("%d,%d,%d\t", cell->pos[0], cell->pos[1], cell->pos[2]);
		printf("0x%02x\t", cell->ch);
		printf("%d\t", cell->off);
		printf("%d\t", cell->a);
		printf("%d\t", cell->k);
		printf("%d\t", cell->syn);
		printf("%d\t", cell->r);
		printf("%d,%d,%d\t", cell->o[0], cell->o[1], cell->o[2]);
		printf("%d,%d,%d\n", cell->p[0], cell->p[1], cell->p[2]);
		first = false;
	}
}

