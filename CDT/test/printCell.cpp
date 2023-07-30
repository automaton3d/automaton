/*
 * printCell.c
 *
 *  Created on: 9 de fev. de 2023
 *      Author: Alexandre
 */

#include "tests.h"

namespace framework
{
  extern unsigned long timer;
}

static int jump = 0;

namespace automaton
{

void printCell(Cell *cell)
{
    if(jump % 64 == 0)
    {
    puts("\nn\tch\toff\ta1\ta2\tk\tsyn\tocc\tobj\tu\to\tp\ts\tpo\tpP\tm\trole");
    puts("----------------------------------------------------------------------------------------------------------------------------------");
  }
  printf("%d:%ld\t", cell->n, framework::timer);
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
  printf("\n");
  printf("%d,%d,%d\t", cell->po[0], cell->po[1], cell->po[2]);
  printf("%d,%d,%d\t", cell->pP[0], cell->pP[1], cell->pP[2]);
  printf("%d,%d,%d\t", cell->m[0], cell->m[1], cell->m[2]);
  printf("%d\n\n", GET_ROLE(cell));
  jump++;
}

void printConfig(Cell *cell)
{
  printf("\tcell->n=%d;\n",     cell->n);
  printf("\tcell->ch=%d;\n",    cell->ch);
  printf("\tcell->off=%d;\n",   cell->off);
  printf("\tcell->a1=%d;\n",    cell->a1);
  printf("\tcell->a2=%d;\n",    cell->a2);
  printf("\tcell->k=%d;\n",     cell->k);
  printf("\tcell->syn=%d;\n",   cell->syn);
  printf("\tcell->occ=%d;\n",   cell->occ);
  printf("\tcell->obj=%d;\n",   cell->obj);
  printf("\tcell->u=%d;\n",     cell->u);
  printf("\tcell->o[0]=%d;\n",  cell->o[0]);
    printf("\tcell->o[1]=%d;\n",  cell->o[1]);
  printf("\tcell->o[2]=%d;\n",  cell->o[2]);
  printf("\tcell->p[0]=%d;\n",  cell->p[0]);
  printf("\tcell->p[1]=%d;\n",  cell->p[1]);
  printf("\tcell->p[2]=%d;\n",  cell->p[2]);
  printf("\tcell->s[0]=%d;\n",  cell->s[0]);
  printf("\tcell->s[1]=%d;\n",  cell->s[1]);
  printf("\tcell->s[2]=%d;\n",  cell->s[2]);
  printf("\tcell->po[0]=%d;\n", cell->po[0]);
  printf("\tcell->po[1]=%d;\n", cell->po[1]);
  printf("\tcell->po[2]=%d;\n", cell->po[2]);
  printf("\tcell->pP[0]=%d;\n", cell->pP[0]);
  printf("\tcell->pP[1]=%d;\n", cell->pP[1]);
  printf("\tcell->pP[2]=%d;\n", cell->pP[2]);
  printf("\tcell->m[0]=%d;\n",  cell->m[0]);
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

}
