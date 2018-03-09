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
#define REISSUE		10

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
	Tuple			p3;		// LM direction
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
	unsigned		p17;	// virtual decay
	//
	// Auxiliary
	//
	unsigned		p18;	// w address
	unsigned		p19;	// w of peer
	unsigned char	p20;	// status
	unsigned char	p21;	// is pair flag
	unsigned char	p22;	// wavefront direction
	unsigned		p23;	// wavefront synch: t1
	unsigned char	p24;	// messenger code
	Tuple			p25;	// burst origin vector;
	unsigned char	p26;	// burst direction
	double 			p27a1;	// a1
	double			p27a2;	// a2

} Brick;

// Functions declarations

char *tile2str(Brick *t);
void copyTile(Brick *dst, Brick *org);
void cleanTile(Brick *t);

#endif /* PREON_H_ */
