/*
 * params.h
 *
 *  Created on: 26/04/2016
 *      Author: Alexandre
 */

#ifndef PARAMS_H_
#define PARAMS_H_

// Automaton dimensions

#define ORDER 	4
#define SIDE 	(1 << ORDER)			// Size of the universal cube
#define DIAMETER ((SIDE - 1) * 2)
#define NDIR 	6						// spatial directions (von Neumann convention)
#define LIGHT (p1 % (3 * SIDE) == 0)	// light step

// Other input parameters

#define HBAR		2
#define LOST		(log2(SIDE))
#define RAMP		4				// universal case: 10^10

// Powers

#define SIDE2	(SIDE*SIDE)
#define SIDE3	(SIDE*SIDE*SIDE)
#define SIDE4	(SIDE*SIDE*SIDE*SIDE)

// PWM

#define STEP	((int)log2(SIDE))
#define NSTEPS	(SIDE / LIGHT)

#endif /* PARAMS_H_ */
