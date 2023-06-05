/*
 * printCell.c
 *
 *  Created on: 9 de fev. de 2023
 *      Author: Alexandre
 */

#include <stdio.h>
#include "simulation.h"

extern Cell *stb, *drf;
extern Cell *latt0, *latt1;
int jump = 0;

void printCell(Cell *cell)
{
  if(cell->off == 0 || 1)
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

void printConfig(Cell *cell)
{
	printf("\tcell->n=%d;\n",     cell->n);
	printf("\tcell->ch=%d\n",     cell->ch);
	printf("\tcell->off=%d;\n",   cell->off);
	printf("\tcell->a1=%d;\n",    cell->a1);
	printf("\tcell->a2=%d;\n",    cell->a2);
	printf("\tcell->k=%d;\n",     cell->k);
	printf("\tcell->syn=%d;\n",   cell->syn);
	printf("\tcell->occ=%d;\n",   cell->occ);
	printf("\tcell->obj=%d;\n",   cell->obj);
	printf("\tcell->u=%d;\n",     cell->u);
	printf("\tcell->o[0]=%d;\n",   cell->o[0]);
    printf("\tcell->o[1]=%d;\n",  cell->o[1]);
	printf("\tcell->o[2]=%d;\n",  cell->o[2]);
	printf("\tcell->p[0]=%d;\n",  cell->p[0]);
	printf("\tcell->p[1]=%d;\n",  cell->p[1]);
	printf("\tcell->p[2]=%d;\n",  cell->p[2]);
	printf("\tcell->s[0]=%d;\n",   cell->s[0]);
	printf("\tcell->s[1]=%d;\n",  cell->s[1]);
	printf("\tcell->s[2]=%d;\n",  cell->s[2]);
	printf("\tcell->po[0]=%d;\n",  cell->po[0]);
	printf("\tcell->po[1]=%d;\n", cell->po[1]);
	printf("\tcell->po[2]=%d;\n", cell->po[2]);
	printf("\tcell->pP[0]=%d;\n",  cell->pP[0]);
	printf("\tcell->pP[1]=%d;\n", cell->pP[1]);
	printf("\tcell->pP[2]=%d;\n", cell->pP[2]);
	printf("\tcell->m[0]=%d;\n",   cell->m[0]);
	printf("\tcell->m[1]=%d;\n",  cell->m[1]);
	printf("\tcell->m[2]=%d;\n",  cell->m[2]);
}

void printLattice(Cell *latt)
{
    for(int i = 0; i < SIDE6; i++, latt++)
    {
   		printCell(latt);
    }
}

int main___()
{
 	setvbuf(stdout, NULL, _IONBF, 0);
	initSimulation();
	Cell *cell = latt0 + 3585;
	cell->n=196;
	cell->ch=57;
	cell->off=3585;
	cell->a1=2;
	cell->a2=0;
	cell->k=2;
	cell->syn=38416;
	cell->occ=0;
	cell->obj=512;
	cell->u=0;
	cell->o[0]=0;
	cell->o[1]=7;
	cell->o[2]=0;
	cell->p[0]=0;
	cell->p[1]=4;
	cell->p[2]=0;
	cell->s[0]=0;
	cell->s[1]=4;
	cell->s[2]=0;
	cell->po[0]=0;
	cell->po[1]=0;
	cell->po[2]=0;
	cell->pP[0]=0;
	cell->pP[1]=0;
	cell->pP[2]=0;
	cell->m[0]=8;
	cell->m[1]=8;
	cell->m[2]=8;
	stb = cell;
	cell = latt1 + 3585;
	cell->n=196;
	cell->ch=57;
	cell->off=3585;
	cell->a1=2;
	cell->a2=0;
	cell->k=2;
	cell->syn=38416;
	cell->occ=0;
	cell->obj=512;
	cell->u=0;
	cell->o[0]=0;
	cell->o[1]=7;
	cell->o[2]=0;
	cell->p[0]=0;
	cell->p[1]=4;
	cell->p[2]=0;
	cell->s[0]=0;
	cell->s[1]=4;
	cell->s[2]=0;
	cell->po[0]=0;
	cell->po[1]=0;
	cell->po[2]=0;
	cell->pP[0]=0;
	cell->pP[1]=0;
	cell->pP[2]=0;
	cell->m[0]=8;
	cell->m[1]=8;
	cell->m[2]=8;
	drf = cell;
	phase4();
	puts("done.");
	return 0;
}
