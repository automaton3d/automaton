/*
 * brick.h
 *
 *  Created on: 23/01/2016
 *      Author: Alexandre
 */

#ifndef BRICK_H_
#define BRICK_H_

#include "tuple.h"

// General code

#define UNDEF		0

// Pair type

#define WBOSON		0x01
#define ZBOSON		0x02
#define ZONE		0x04
#define UNIV		0x08
#define EDEN		0x10
#define GLUON		0x20	// TODO

// Interaction type

#define UXU			6
#define UXP			7
#define PXP			8
#define UXG			9
#define REISSUE		10

// Burst type

#define DESTROY		1
#define CANCEL		2

// Footprint time constant

#define TAU			10			// TODO

/*
 * Preon structure
 */
typedef struct
{
	/// Lattice ///

	Tuple			a;			// xyz position [IMMUTABLE]
	Tuple			o;			// origin vector
	short			t;			// tick counter
	//
	unsigned short	w;			// w address [IMMUTABLE]

	/// Physics ///

	Tuple			p;			// LM direction
	Tuple			s;			// spin direction
	//
	unsigned char	q;			// electric charge
	unsigned char	c;			// chirality
	unsigned char	R;			// red
	unsigned char	G;			// green
	unsigned char	B;			// blue
	unsigned char	g;			// gravity charge
	unsigned char	d;			// duality
	//
	unsigned char	m;			// kinetic messenger
	unsigned		e;			// entanglement
	unsigned char	phi_sin;	// quadrature phase
	unsigned char	phi_cos;	// quadrature phase
	unsigned short	f;			// frequency
	short			xi;			// interference

	/// Burst dynamics ///

	Tuple			burst;		// burst origin vector;
	unsigned char	bcode;		// burst code
	unsigned char	bdir;		// burst direction

	/// Preon dynamics ///

	Tuple			seed;		// re-emission address
	unsigned char	type;		// interaction type
	unsigned char	dir;		// wavefront direction
	unsigned		synch;		// wavefront synch
	unsigned char	used;		// a detection flag
	short			offset;		// relative position in w dimension
	unsigned char	dirs[6];	// von Neumann directions [IMMUTABLE]

} Brick;

// Functions declarations

char *brick2str(Brick *b);
void copyBrick(Brick *dst, Brick *org);
void cleanBrick(Brick *b);
boolean neutralized(Brick *b1, Brick *b2);

#endif /* PREON_H_ */
