/*
 * printCell.c
 *
 *  Created on: 9 de fev. de 2023
 *      Author: Alexandre
 */

#include <stdio.h>
#include "test.h"


int jump = 0;

extern Cell* latt1;

void printCell(Cell *cell)
{
  char *cmpl = "";

  if(cell->off == 0)
  {
    if(jump % 64 == 0)
    {
	  puts("\nn\tch\toff\ta1\ta2\tk\tsyn\to\tp\tpo");
	  puts("---------------------------------------------------------------------------------------");
	}
	printf("%d\t", cell->n);
	printf("0x%02x\t", cell->ch);
	printf("%d\t", cell->off);
	printf("%d\t", cell->a1);
	printf("%d\t", cell->a2);
	printf("%d\t", cell->k);
	printf("%d\t", cell->syn);
	printf("%d,%d,%d\t", cell->o[0], cell->o[1], cell->o[2]);
	printf("%d,%d,%d\t", cell->p[0], cell->p[1], cell->p[2]);
	printf("%d,%d,%d\t", cell->po[0], cell->po[1], cell->po[2]);
	if(!ISMILD(cell->po))
		cmpl = "C";
	printf("%s\n", cmpl);
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

int main___()
{
  initAutomaton();
  Cell *drf = latt1;
  for(int i = 0; i < SIDE3 * SIDE3; i++, drf++)
   	printCell(drf);
  return 0;
}
