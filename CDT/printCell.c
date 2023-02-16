/*
 * printCell.c
 *
 *  Created on: 9 de fev. de 2023
 *      Author: Alexandre
 */

#include <stdio.h>
#include "simulation.h"

void printCell(Tensor *cell)
{
	printf("ch\t=0x%02x\n", cell->ch);
	printf("a\t=%d\n", cell->a);
	printf("t\t=%d\n", cell->t);
	printf("f\t=%d\n", cell->f);
	printf("syn\t=%d\n", cell->syn);
	printf("noise\t=%d\n", cell->noise);
	printf("o\t=%d,%d,%d\n\n", cell->o[0], cell->o[1], cell->o[2]);
	/*
	  int p[3], s[3];  // momentum, spin

	  // Wavefront

	  unsigned tt;     // lifetime
	  unsigned syn;    // synchronism
	  int u, v;        // Euler

	  // Footprint

	  int p0[3];       // momentum propensity
	  int prone[3];    // sought for direction

	  // Superluminal variables

	  unsigned char flash; // flash
	  unsigned pole[3];    // pole
	  unsigned target;     // affinity collapsing

	  // Pointers

	  unsigned offset;     // offset inside espacito (constant)
	  struct Tensor *wires[6];  // wires to other cells (constant);

	  // Interaction data

	  unsigned char kind;  // kind of fragment
*/
	fflush(stdout);
}

