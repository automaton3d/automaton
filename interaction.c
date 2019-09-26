/*
 * interaction.c
 *
 *  Created on: 20 de set de 2019
 *      Author: Alexandre
 */

#include "interaction.h"
#include "params.h"
#include "utils.h"
#include "brick.h"
#include "automaton.h"

void axiomUxU(Brick *U1, Brick *U2)
{
	unsigned char chg1 = (U1->d<<6) | (U1->g<<5) | (U1->R<<4) | (U1->G<<3) | (U1->B<<2) | (U1->c<<1) | U1->q;
	unsigned char chg2 = (U2->d<<6) | (U2->g<<5) | (U2->R<<4) | (U2->G<<3) | (U2->B<<2) | (U2->c<<1) | U2->q;
	Tuple prod;
	tupleCross(U1->s, U2->s, &prod);
	if((chg1^chg2)==0x7f && isOpposite(U1->p, U2->p))
	{
		// Reissue
	}
	else if(!isNull(prod))
	{
		// Entanglement spreading
		//
		U1->e = max(U1->e, U2->e);
		U2->e = U1->e;
		//
		// Spin realignment
		//
		U1->s = prod;
		U2->s = prod;
		invertTuple(&U2->s);
	}
}

void axiomUxP(Brick *U, Brick *P)
{
	unsigned char c1 = U->R + U->G + U->B;
	unsigned char c2 = P->R + P->G + P->B;
	if(P->type == ZONE)
	{
		if(U->d != P->d)		// different universes?
		{
			// Split the pair: P_Z -> U_1, U_2
			//
			Tuple p = U->p;
			U->p = P->p;
			P->p = p;			// Pair formation: p^U <-> p^U1
		}
		else					// rules valid inside each universe
		{
			if(P->d == P->g && U->d == U->g)
			{
				if(U->m == 1)	// the U is in heat?
				{
					resetTuple(&P->s);
					resetTuple(&(P+P->offset)->s);
					U->m = 0;
					P->p = U->p;
					(P+P->offset)->p = U->p;	// grav. accel., p symm. is broken
				}
				else
				{
					P->s = U->s;
					(P+P->offset)->s = U->s;
					P->q = U->q;
					(P+P->offset)->q = U->q;	// P_m formation, q symm. is broken
				}
			}
			else if(P->m == 1 && U->g==P->g)	// real x real or virt x virt?
			{
				if(pwm(U->phi_sin*P->phi_sin))
				{
					Tuple p = P->o;
					subTuples(&p, U->o);
					scaleTuple(&p, 2*(U->q ^ P->q) - 1);
					resetTuple(&U->s);
					resetTuple(&P->s);
					P->p = p;
					(P+P->offset)->p = p;
				}
				else if(pwm(U->phi_cos*P->phi_cos) && pwm(dot(U->s, P->s)))
				{
					Tuple dif = P->o;
					subTuples(&dif, U->o);
					Tuple p;
					tupleCross(P->s, dif, &p);
					scaleTuple(&p, 2*U->d - 1);
					resetTuple(&P->s);
					resetTuple(&(P+P->offset)->s);
					P->q = (P+P->offset)->q;
					P->p = p;
					(P+P->offset)->p = p;
				}
			}
			else if(P->q != (P+P->offset)->q && P->c != (P+P->offset)->c && U->g == P->g && P->g == (P+P->offset)->g && neutralized(P, P+P->offset))
			{
				int e = 0;
				Brick *b = dual;
				for(int i; i<SIDE2; i++, b++)
					if(b->e == e)
						resetTuple(&b->o);
			}
			else if(U->d==P->d && virt(U)==virt(P) && !leptonic(U))
			{
				resetTuple(&P->s);
				resetTuple(&(P+P->offset)->s);
				subTuples(&P->o, U->o);
				unsigned char color;
				if(U->t % 2)
				{
					color = U->R;
					U->R = P->R;
					P->R = color;
					//
					color = U->G;
					U->G = P->G;
					P->G = color;
					//
					color = U->G;
					U->G = P->G;
					P->G = color;
				}
				else
				{
					color = U->R;
					U->R = (P+P->offset)->R;
					(P+P->offset)->R = color;
					//
					color = U->G;
					U->G = (P+P->offset)->G;
					(P+P->offset)->G = color;
					//
					color = U->G;
					U->G = (P+P->offset)->G;
					(P+P->offset)->G = color;
				}
			}
			else if(P->type == WBOSON || P->type == ZBOSON)
			{
				int e = 0;
				Brick *b = dual;
				for(int i; i<SIDE2; i++, b++)
					if(b->e == e)
						resetTuple(&b->o);
			}
			else if(U->d == P->d && virt(U) == virt(P) && neutralized(P, P+P->offset) && leptonic(U))
			{

			}
			else if(isEqual(P->p, (P+P->offset)->p))
			{
				unsigned s = imod(P->o);
				Tuple p = P->p;
				scaleTuple(&p, s);
				p.x >>= (ORDER-1);
				p.y >>= (ORDER-1);
				p.z >>= (ORDER-1);
				subTuples(&P->a, P->o);
				addRectify(&P->a, p);
				resetTuple(&P->o);
				resetTuple(&(P+P->offset)->o);
				(P+P->offset)->a = P->a;
				//
				s = imod(U->o);
				p = U->p;
				scaleTuple(&p, s);
				p.x >>= (ORDER-1);
				p.y >>= (ORDER-1);
				p.z >>= (ORDER-1);
				subTuples(&U->a, U->o);
				addRectify(&U->a, p);
				resetTuple(&U->o);
			}
		}
	}
	else if(c1!=0 && c1!=3 && c2!=0 && c2!=3)
	{
		unsigned char color;
		if(U->t % 2)
		{
			color = U->R;
			U->R = P->R;
			P->R = color;
			color = U->G;
			U->G = P->G;
			P->G = color;
			color = U->B;
			U->B = P->B;
			P->B = color;
		}
		else
		{
			color = (U+U->offset)->R;
			(U+U->offset)->R = (P+P->offset)->R;
			(P+P->offset)->R = color;
			color = (U+U->offset)->G;
			(U+U->offset)->G = (P+P->offset)->G;
			(P+P->offset)->G = color;
			color = (U+U->offset)->B;
			(U+U->offset)->B = (P+P->offset)->B;
			(P+P->offset)->B = color;
		}
	}
	else if(P->type == UNIV)
	{
		unsigned char tmp = P->g;
		P->g = U->g;
		U->g = tmp;
	}
	else if(P->type == EDEN)
	{
		unsigned char tmp = P->d;
		P->d = U->d;
		U->d = tmp;
	}
}

void axiomUxG(Brick *U, Brick *G)
{
	U->m = 1;
	G->p = U->o;
	if(U->d == G->d)
		invertTuple(&G->p);
	G->e = 0;
	resetTuple(&U->o);
}

void axiomPxG(Brick *P, Brick *G)
{
	resetTuple(&P->o);
	resetTuple(&(P+P->offset)->o);
}

void axiomPxP(Brick *P1, Brick *P2)
{
	if(P1->type == EDEN && P2->type == EDEN)
	{
		P1->d = P1->t % 2;
		(P1+P1->offset)->d = P1->d;
		P2->d = !P1->t;
		(P2+P2->offset)->d = P2->d;
	}
	else if(P1->d == P2->d)
	{
		unsigned char c1 = P1->R + P1->G + P1->B;
		unsigned char c2 = P2->R + P2->G + P2->B;
		if(P1->g!=(P1+P1->offset)->g && P2->g!=(P2+P2->offset)->g)
		{
			P1->g = P1->t % 2;
			(P1+P1->offset)->g = P1->g;
			P2->g = !P1->t;
			(P2+P2->offset)->g = P2->g;
		}
		else if(isEqual(P1->p, (P1+P1->offset)->p) && isEqual(P2->p, (P2+P2->offset)->p))	// Pk x Pk?
		{
			if(dot(P1->p, P2->p)==-SIDE/2)
			{
				invertTuple(&P1->p);
				invertTuple(&P2->p);
				if(P1->c == (P1+P1->offset)->c)
					P1->c = !P1->c;
				if(P1->q == (P1+P1->offset)->q)
					P1->q = !P1->q;
			}
		}
		else if(c1!=0 && c1!=3 && c2!=0 && c2!=3)
		{
			unsigned char r, g, b;
			if(P1->t % 2)
			{
				r = P1->R;
				g = P1->G;
				b = P1->B;
				P1->R = P2->R;
				P1->G = P2->G;
				P1->B = P2->B;
				P2->R = r;
				P2->G = g;
				P2->B = b;
			}
			else
			{
				r = (P1+P1->offset)->R;
				g = (P1+P1->offset)->G;
				b = (P1+P1->offset)->B;
				(P1+P1->offset)->R = (P2+P2->offset)->R;
				(P1+P1->offset)->G = (P2+P2->offset)->G;
				(P1+P1->offset)->B = (P2+P2->offset)->B;
				(P2+P2->offset)->R = r;
				(P2+P2->offset)->G = g;
				(P2+P2->offset)->B = b;
			}
		}
		else if(P1->q == (P1+P1->offset)->q && P1->c == (P1+P1->offset)->c)
		{
			unsigned s = imod(P1->o);
			Tuple p = P1->p;
			scaleTuple(&p, s);
			p.x >>= (ORDER-1);
			p.y >>= (ORDER-1);
			p.z >>= (ORDER-1);
			subTuples(&P1->a, P1->o);
			addRectify(&P1->a, p);
			resetTuple(&P1->o);
			resetTuple(&(P1+P1->offset)->o);
			(P1+P1->offset)->a = P1->a;
			//
			s = imod(P2->o);
			p = P2->p;
			scaleTuple(&p, s);
			p.x >>= (ORDER-1);
			p.y >>= (ORDER-1);
			p.z >>= (ORDER-1);
			subTuples(&P2->a, P2->o);
			addRectify(&P2->a, p);
			resetTuple(&P2->o);
			resetTuple(&(P2+P2->offset)->o);
			(P2+P2->offset)->a = P2->a;
		}
	}
}

