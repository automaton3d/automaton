/*
 * printCell.c
 *
 *  Created on: 9 de fev. de 2023
 *      Author: Alexandre
 */

#include <stdio.h>
#include "test.h"

#define true  1
#define false 0

boolean first = true;

extern Cell* latt1;

void printCell(Cell *cell)
{
  //if (!ZERO(cell->p))
  if(cell->oE == 0)
  {
    if(first)
    {
	  puts("\nn\tch\toL\toE\ta\tk\tsyn\tnoise\to\tp\tpo");
	  puts("---------------------------------------------------------------------------------");
	}
	printf("%d\t", cell->n);
	printf("0x%02x\t", cell->ch);
	printf("%d\t", cell->oL);
	printf("%d\t", cell->oE);
	printf("%d\t", cell->a1);
	printf("%d\t", cell->k);
	printf("%d\t", cell->syn);
	printf("%d,%d,%d\t", cell->o[0], cell->o[1], cell->o[2]);
	printf("%d,%d,%d\t", cell->p[0], cell->p[1], cell->p[2]);
	printf("%d,%d,%d\n", cell->po[0], cell->po[1], cell->po[2]);
	first = false;
  }
}

void printLattice(Cell *latt)
{
    for(int i = 0; i < SIDE6; i++, latt++)
    {
    	if(0&&!ZERO(latt->p))
    	{
    		puts("-----------");
    	}
    	if(latt->occ > 0)
    	{
    		printCell(latt);
    	}
   		//delay(100);
    }
}

int main___()
{
  initAutomaton();
  Cell *drf = latt1;
  for(int i = 0; i < SIDE3 * SIDE3; i++, drf++)
   	printCell(drf);
  return 0;
}
