/*
 * preon.c
 *
 *  Created on: 23/01/2016
 *      Author: Alexandre
 */

#include <stdio.h>
#include "params.h"
#include "tile.h"
#include "vector3d.h"

char preonBuf[4][50];

void copy(Tile *dst, Tile *org)
{
	dst->p1    = org->p1;
	dst->p2    = org->p2;
	dst->p3    = org->p3;
	dst->p4    = org->p4;
	dst->p5    = org->p5;
	//
	dst->p6.x  = org->p6.x;
	dst->p6.y  = org->p6.y;
	dst->p6.z  = org->p6.z;
	//
	dst->p7.x  = org->p7.x;
	dst->p7.y  = org->p7.y;
	dst->p7.z  = org->p7.z;
	//
	dst->p8    = org->p8;
	dst->p9    = org->p9;
	dst->p10   = org->p10;
	dst->p11   = org->p11;
	//
	dst->p12.x = org->p12.x;
	dst->p12.y = org->p12.y;
	dst->p12.z = org->p12.z;
	//
	dst->p13   = org->p13;
	//
	dst->p141  = org->p141;
	dst->p142  = org->p142;
	dst->p143  = org->p143;
	dst->p144  = org->p144;
	//
	dst->p15E  = org->p15E;
	dst->p15M  = org->p15M;
	dst->p16   = org->p16;
	dst->p17   = org->p17;
	dst->p18   = org->p18;
	//
	dst->p19.x = org->p19.x;
	dst->p19.y = org->p19.y;
	dst->p19.z = org->p19.z;
	//
	dst->p201  = org->p201;
	dst->p202  = org->p202;
	dst->p203  = org->p203;
	//
	dst->p21   = org->p21;
	dst->p22   = org->p22;
}

/*
 * Erases a tile
 */
void cleanCell(Tile *dst)
{
	dst->p1    = 0;
	dst->p2    = 0;
	dst->p3    = 0;
	dst->p4    = 0;
	dst->p5    = 0;
	//
	dst->p6.x  = 0;
	dst->p6.y  = 0;
	dst->p6.z  = 0;
	//
	dst->p7.x  = 0;
	dst->p7.y  = 0;
	dst->p7.z  = 0;
	//
	dst->p8    = 0;
	dst->p9    = 0;
	dst->p10   = 0;
	dst->p11   = 0;
	//
	dst->p12.x = 0;
	dst->p12.y = 0;
	dst->p12.z = 0;
	//
	dst->p13   = 0;
	//
	dst->p141   = 0;
	dst->p142   = 0;
	dst->p143   = 0;
	dst->p144   = 0;
	//
	dst->p15E   = 0;
	dst->p15M   = 0;
	dst->p16   = 0;
	dst->p17   = 0;
	dst->p18   = 0;
	//
	dst->p19.x = 0;
	dst->p19.y = 0;
	dst->p19.z = 0;
	//
	dst->p201  = 0;
	dst->p202  = 0;
	dst->p203  = 0;
	//
	dst->p21   = 0;
	dst->p22   = 0;
}

void entangle(Tile *p1, Tile *p2)
{
	// Axiom: entangling
	//
	if(p1->p13 != 0 && p2->p13 != 0 && p1->p13 != p2->p13)
		p1->p13 = p2->p13 = p1->p13 ^ p2->p13;
}

char *preon2str(Tile *preon)
{
	static int index = 0;
	char *ptr = preonBuf[index];
	sprintf(ptr, "[t=%ld p=%s org=%s]", preon->p1, tuple2str(&preon->p12), tuple2str(&preon->p6));
	index++;
	index &= 3;
	return ptr;
}

