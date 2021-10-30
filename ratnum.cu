/*
 * ratnum.c
 *
 *  Created on: 17 de out. de 2021
 *      Author: Alexandre
 */


#include	<stdio.h>
#include	<math.h>
#include	"ratnum.cuh"

static	void simplify(ratnum*);
static	long int euclid(long int, long int);


ratnum mkrat(long int a, long int b)
{
	ratnum r;
	r.num = a;
	r.den = b;
	simplify(&r);
	return r;
}

void showrat(ratnum x)
{
	double	rr;
	rr = (double)(x.num) / (double)(x.den);
	fprintf(stderr, "%ld/", x.num);
	fprintf(stderr, "%ld", x.den);
	fprintf(stderr, " (%20.10lf)\n", rr);
}

double rat2float(ratnum r)
{
	return (double)(r.num) / (double)(r.den);
}

ratnum addrat(ratnum a, ratnum b)
{
	ratnum r;
	r.num = a.num * b.den + b.num * a.den;
	r.den = a.den * b.den;
	simplify(&r);
	return r;
}

ratnum addinttorat(ratnum a, long int x)
{
	ratnum sum;
	sum.num = a.num + x * a.den;
	sum.den = a.den;
	simplify(&sum);
	return sum;
}

static void simplify(ratnum* a)
{
	long int gcd;
	gcd = euclid(a->num, a->den);
	a->num /= gcd;
	a->den /= gcd;
}

static long int euclid(long int a, long int b)
{
	if (b == 0)	return a;
	else		return euclid(b, a % b);
}

int ratcomp(ratnum* a, ratnum* b)
{
	return  a->den * b->num - a->num * b->den;
}

ratnum modrat(ratnum a, ratnum b)
{
	ratnum re = a;
	double q = floor((a.num * b.den) / (double)(a.den * b.num));
	ratnum t2 = mkrat(-q * b.den, b.num);
	re = addrat(re, t2);
	return re;
}

ratnum multrat(ratnum a, ratnum b)
{
	ratnum prod = mkrat(a.num * b.num, a.den * b.den);
	simplify(&prod);
	return prod;
}

int rat2int(ratnum r)
{
	return floor(r.num / (double)r.den);
}

