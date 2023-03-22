/*
 * test-update.c
 *
 *  Created on: 12 de mar. de 2023
 *      Author: Alexandre
 */
#include <stdio.h>
#include "test.h"

extern Cell *latt0, *latt1;
extern Cell *stb, *drf;

void test_update()
{
	drf = getAddress(latt1, 3, 3, 3);
	drf->a = 0;
	drf->o[0] = 7;
	drf->o[1] = 7;
	drf->o[2] = 7;
	drf->f = 1;
	drf->t = 325;
	drf->syn = 115248;
	stb = getAddress(latt0, 3, 3, 3);
	stb->a = 0;
	copy();
	puts("expand 1");
	expand();
	copy();
	puts("expand 2");
	expand();
	copy();
	puts("expand 3");
	expand();
	copy();
	puts("expand 4");
	expand();
	copy();
	puts("expand 5");
	expand();
	copy();
	puts("expand 6");
	expand();
	copy();
	puts("expand 7");
	expand();
	copy();
	puts("expand 8");
	expand();
	copy();
	puts("expand 9");
	expand();
	copy();
	puts("expand 10");
	expand();
	copy();
	puts("expand 11");
	expand();
	copy();
	puts("expand 12");
	expand();
	copy();
	puts("expand 13");
	expand();
	copy();
	puts("expand 14");
	expand();
	copy();
	puts("expand 15");
	expand();
}
