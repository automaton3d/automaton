/*
 * ratnum.h
 *
 *  Created on: 17 de out. de 2021
 *      Author: Alexandre
 */

#ifndef RATNUM_H_
#define RATNUM_H_

struct	ratnumber
{
	long	int	num;
	long	int	den;
};

typedef	struct ratnumber ratnum;

ratnum	mkrat(long int, long int);
ratnum	addrat(ratnum, ratnum);
ratnum	addinttorat(ratnum, long int);
void	showrat(ratnum);
int	ratcomp(ratnum*, ratnum*);
ratnum modrat(ratnum a, ratnum b);
ratnum multrat(ratnum, ratnum);
int rat2int(ratnum);
double rat2float(ratnum r);

#endif /* RATNUM_H_ */
