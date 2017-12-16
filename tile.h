/*
 * preon.h
 *
 *  Created on: 23/01/2016
 *      Author: Alexandre
 */

#ifndef PREON_H_
#define PREON_H_

// p2 role

#define NIL		0
#define REAL		1
#define VIRT		2
#define GRAV		3

// p3 messenger type

#define IDLE		0
#define PLAIN		1
#define FORCING		2
#define COLL		3
#define FORMING		4

// p9 chirality

#define LEFTMATTER	-1
#define RHM_LHAM	0
#define RIGHTMATTER	+1

// p10 color

#define UNDEFINED		0x00
#define RED    			0x20
#define GREEN  			0x10
#define BLUE   			0x08
#define ANTIRED    		0x04
#define ANTIGREEN  		0x02
#define ANTIBLUE   		0x01
#define WHITE			(RED | GREEN | BLUE)
#define ANTIWHITE		(ANTIRED | ANTIGREEN | ANTIBLUE)
#define LEPTONIC		0x07
#define ANTILEPTONIC	0xc8

//#define COLOR		(RED | GREEN | BLUE)				// filter
//#define ANTICOLOR	(ANTIRED | ANTIGREEN | ANTIBLUE)	// filter

// p11 gravity

#define ON			1
#define OFF			0

// p16 interaction

#define ND			0x0000			// empty
#define U			0x0001			// unpaired
#define P			0x0002			// pair
#define B			0x0003			// bread
#define Z			0x0004			// cheese
#define G			0x0005			// graviton

#define UXU			0x0011
#define UXP			0x0012
#define PXP			0x0022
#define UXZ			0x0014
#define UXG			0x0015
#define ZXP			0x0015

#define WZBOSON		0x0016
#define HADRON		0x0017

// Pair types

#define KNP			1			// kinetic
#define MGP			2			// massgen
#define EMP			3			// static
#define VCP			4			// vacuum
#define GLP			5			// gluonic
#define MSP			6			// mesonic
#define PHP			7			// photonic
#define NTP			8			// neutrino

#include "tuple.h"

/*
 * Wavefront synchronization subfields
 */
typedef struct
{
	char d;						// immediate motion direction
	long t0, t1;				// timing

} Wavefront;

/*
 * Preon structure
 */
typedef struct
{
	long			p1;		// clock
	unsigned char	p2;		// role
	unsigned char	p3;		// messenger
	unsigned char	p4;		// helicity
	unsigned		p5;		// level
	Tuple			p6;		// origin vector
	Tuple			p7;		// momentum direction
	char			p8;		// electric charge
	char			p9;		// chirality
	unsigned char	p10;	// color
	unsigned char	p11;	// gravity
	Tuple			p12;	// spin direction
	unsigned		p13;	// entanglement
	//
	double 			p141;	// a1
	double			p142;	// a2
	int 			p143;	// ramp
	boolean 		p144;	// pwm
	//
	boolean			p15E;	// electric polarization
	boolean			p15M;	// magnetic polarization
	//
	unsigned char	p16;	// interaction
	unsigned		p17;	// number of light steps since last visit
	int				p18;	// interference
	Tuple			p19;	// return path
	//
	char			p201;	// wavefront tree: d
	unsigned		p202;	// wavefront tree: depth
	long			p203;	// wavefront synch: t0
	long			p204;	// wavefront synch: t1
	//
	unsigned		p21;	// frequency
	unsigned char	p22;	// pair type
	//
	// Exploration channels
	//
	void *n [7];			// six 3d neighbors and one w neighbor pointers

} Tile;

char *preon2str(Tile *preon);
void copy(Tile *dst, Tile *org);
void cleanCell(Tile *iu);
void entangle(Tile *p1, Tile *p2);

#endif /* PREON_H_ */
