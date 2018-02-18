/*
 * tile.h
 *
 *  Created on: 23/01/2016
 *      Author: Alexandre
 */

#ifndef TILE_H_
#define TILE_H_

#include "tuple.h"

// General code

#define UNDEF		0

// p5 chirality

#define LHM			-1
#define RHMLHAM		0
#define RHM			+1

// p6 color

#define RED    			0x20
#define GREEN  			0x10
#define BLUE   			0x08
#define ANTIRED    		0x04
#define ANTIGREEN  		0x02
#define ANTIBLUE   		0x01
#define WHITE			(RED | GREEN | BLUE)
#define ANTIWHITE		(ANTIRED | ANTIGREEN | ANTIBLUE)
#define LEPT			0x07
#define ANTILEPT		0xc8

// p13 preon interactions

#define U			1			// unpaired
#define P			2			// pair

#define UXU			6
#define UXP			7
#define PXP			8
#define UXG			9

// p18 status

#define	PREON		0X01
#define SEED		0X02
#define GRAV		0X04

// p22 burst type

#define DESTROY		1
#define CANCEL		2

/*
 * Preon structure
 */
typedef struct
{
	Tuple			p0;		// xyz position
	unsigned		p1;		// clock
	Tuple			p2;		// origin vector

	Tuple			p3;	// LM direction

	char			p4;		// electric charge
	char			p5;		// chirality
	unsigned char	p6;		// color and conjugation
	Tuple			p7;		// spin direction
	unsigned char	p8;		// gravity charge
	unsigned		p9;		// entanglement
	unsigned char	p10;	// sinusoid pwm
	int 			p11;	// frequency
	unsigned char	p12;	// helicity
	unsigned char	p13;	// interaction
	int				p14;	// interference
	Tuple			p15;	// return path
	unsigned char	p16;	// cohesion
	//
	// Auxiliary
	//
	unsigned		p17;	// w address
	unsigned char	p18;	// status
	unsigned char	p19;	// is pair flag
	unsigned char	p20;	// wavefront direction
	unsigned		p21;	// wavefront synch: t1
	unsigned char	p22;	// messenger code
	Tuple			p23;	// burst origin vector;
	unsigned char	p24;	// burst direction
	double 			p25a1;	// a1
	double			p25a2;	// a2
	unsigned		p26;	// w of peer

} Tile;

// Functions declarations

char *tile2str(Tile *t);
void copyTile(Tile *dst, Tile *org);
void cleanTile(Tile *t);

#endif /* PREON_H_ */
