/*
 * printCell.c
 *
 *  Created on: 9 de fev. de 2023
 *      Author: Alexandre
 */

#include <stdio.h>
#include "simulation.h"

int jump = 0;

void printCell(Cell *cell)
{
  if(cell->off == 0)
  {
    if(jump % 64 == 0)
    {
	  puts("\nn\tch\toff\ta1\ta2\tk\tsyn\tocc\tobj\tu\to\tp\ts\tpo\tpP\tm\trole");
	  puts("----------------------------------------------------------------------------------------------------------------------------------");
	}
	printf("%d\t", cell->n);
	printf("0x%02x\t", cell->ch);
	printf("%d\t", cell->off);
	printf("%d\t", cell->a1);
	printf("%d\t", cell->a2);
	printf("%d\t", cell->k);
	printf("%d\t", cell->syn);
	printf("%d\t", cell->occ);
	printf("%d\t", cell->obj);
	printf("%d\t", cell->u);
	printf("%d,%d,%d\t", cell->o[0], cell->o[1], cell->o[2]);
	printf("%d,%d,%d\t", cell->p[0], cell->p[1], cell->p[2]);
	printf("%d,%d,%d\t", cell->s[0], cell->s[1], cell->s[2]);
	char *ch = ISMILD(cell->po) ? "M" : " ";
	printf("%d,%d,%d%s\t", cell->po[0], cell->po[1], cell->po[2], ch);
	printf("%d,%d,%d\t", cell->pP[0], cell->pP[1], cell->pP[2]);
	printf("%d,%d,%d\n\n", cell->m[0], cell->m[1], cell->m[2]);
	jump++;
  }
}

void printLattice(Cell *latt)
{
    for(int i = 0; i < SIDE6; i++, latt++)
    {
   		printCell(latt);
    }
}
