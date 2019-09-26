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
#define NPREONS		5//(SIDE*SIDE)
#define DIAMETER 	((unsigned)floor((SIDE-1)*1.732))
#define NDIR 		6						// spatial directions (von Neumann convention)

// Derived parameters

#define SHIFT		(ORDER-3)
#define ROOT		(1<<(ORDER/2))
#define BURST		(3*SIDE/2+1)
#define SYNCH		(BURST+2*DIAMETER)

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

// debug

#define LIMIT2		(3*SIDE2/4)

#endif /* PARAMS_H_ */

