/*
 * params.h
 */

#ifndef PARAMS_H_
#define PARAMS_H_

// Automaton dimensions

#define ORDER		5

#define GD_X            ((1<<ORDER)>>2)
#define GD_Y            ((1<<ORDER)>>2)
#define GD_Z            ((1<<ORDER)>>3)
#define BD_X            4
#define BD_Y            4
#define BD_Z            8

#define NDIR 		6					// spatial directions (von Neumann convention)
#define NPREONS		8

// Derived parameters:

#define NTHREADS	(GD_X*GD_Y*GD_Z*BD_X*BD_Y*BD_Z)
#define SIDE		(1<<ORDER)
#define SIDE2		(SIDE*SIDE)
#define SIDE3		(SIDE2*SIDE)
#define NCELLS		(SIDE3*NPREONS)
#define DIAMETER 	((SIDE-1)*2)
#define BURST		(3*SIDE/2)
#define SYNCH		(2*DIAMETER+BURST)

// Wrapping constants

#define WRAP0		((SIDE3-SIDE2)*NPREONS)
#define WRAP1		(SIDE2*NPREONS)
#define WRAP2		((SIDE2-SIDE)*NPREONS)
#define WRAP3		(SIDE*NPREONS)
#define WRAP4		((SIDE-1)*NPREONS)
#define WRAP5		(NPREONS)

// PWM parameters

#define STEP		((int)log2((float)SIDE))
#define NSTEPS		(SIDE/STEP)

#endif /* PARAMS_H_ */

