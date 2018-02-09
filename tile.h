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

// p4 chirality

#define LHM			-1
#define RHMLHAM		0
#define RHM			+1

// p5 color

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

// p14 preon classification

#define U			1			// unpaired
#define P			2			// pair

// p14 preon interactions

#define UXU			6
#define UXP			7
#define PXP			8
#define UXG			9

// p17 status

#define	PREON		0X01
#define SEED		0X02
#define GRAV		0X04

/*
 * Preon structure
 */
typedef struct
{
	Tuple			p0;		// xyz position
	unsigned		p1;		// clock
	Tuple			p2;		// origin vector
	char			p3;		// electric charge
	char			p4;		// chirality
	unsigned char	p5;		// color and conjugation
	Tuple			p6;		// spin direction
	unsigned char	p7;		// gravity charge
	unsigned		p8;		// entanglement
	unsigned char	p9;		// sinusoid pwm
	int 			p10;	// frequency
	unsigned char	p11;	// helicity
	unsigned char	p12;	// interaction
	int				p13;	// interference
	Tuple			p14;	// return path
	unsigned char	p15;	// cohesion
	//
	// Auxiliary
	//
	unsigned		p16;	// w address
	unsigned char	p17;	// status
	unsigned char	p18;	// pair classification
	unsigned char	p19;	// messenger
	double 			p21a1;	// a1
	double			p21a2;	// a2
	unsigned char	p22;	// wf direction
	unsigned		p23;	// wavefront synch: t1
	unsigned		p24;	// timeout of virtual pairs
	Tuple			p25;	// burst origin vector;
	unsigned char	p26;	// burst direction
	unsigned		p27;	// w of peer
	unsigned char	p28;	// is pair flag
	Tuple			p29;	// LM direction

} Tile;

// Functions declarations

char *tile2str(Tile *t);
void copyTile(Tile *dst, Tile *org);
void cleanTile(Tile *t);

#endif /* PREON_H_ */

