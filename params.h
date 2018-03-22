/*
 * params.h
 *
 *  Created on: 26/04/2016
 *      Author: Alexandre
 */

#ifndef PARAMS_H_
#define PARAMS_H_

// Automaton dimensions

#define ORDER 		5
#define SIDE 		(1<<ORDER)				// size of the universal cube
#define NPREONS		(16)
#define DIAMETER 	((SIDE-1)*2)
#define NDIR 		6						// spatial directions (von Neumann convention)

// Derived parameters

#define BURST		(3*SIDE/2+1)
#define SYNCH		(2*DIAMETER+BURST)

// PWM parameters

#define STEP		((int)log2(SIDE))
#define NSTEPS		(SIDE/STEP)

// Powers

#define SIDE2		(SIDE*SIDE)
#define SIDE3		(SIDE2*SIDE)
#define SIDE4		(SIDE3*NPREONS)

// Wrapping constants

#define WRAP0       ((SIDE3-SIDE2)*NPREONS)
#define WRAP1  		(SIDE2*NPREONS)
#define WRAP2      	((SIDE2-SIDE)*NPREONS)
#define WRAP3     	(SIDE*NPREONS)
#define WRAP4      	((SIDE-1)*NPREONS)
#define WRAP5      	(NPREONS)

#endif /* PARAMS_H_ */
