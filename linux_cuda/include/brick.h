/*
 * brick.h
 */

#ifndef TILE_H_
#define TILE_H_

#include "tuple.h"

// General code

#define UNDEF		0

// p9 color

#define RED    		0x20
#define GREEN  		0x10
#define BLUE   		0x08
#define ANTIRED    	0x04
#define ANTIGREEN  	0x02
#define ANTIBLUE   	0x01
#define WHITE		(RED | GREEN | BLUE)
#define ANTIWHITE	(ANTIRED | ANTIGREEN | ANTIBLUE)
#define LEPT		0xc8
#define ANTILEPT	0x07

// p13 preon interactions

#define U			1			// unpaired
#define P			2			// pair

#define PXP			5
#define UXU			6
#define UXP			7
#define UXG			8
#define REISSUE		9

// p21 status

#define	PREON		0X01
//#define SEED		0X02
#define GRAV		0X04

// p25 burst type

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
	Tuple			p4;		// spin direction
	char			p5;		// helicity
	char			p6;		// electric charge
	char			p7;		// chirality
	char			p8;		// gravity charge
	unsigned char	p9;		// color and conjugation
	unsigned		p10;	// entanglement
	unsigned char	p11;	// sinusoid pwm
	int 			p12;	// frequency
	unsigned char	p13;	// interaction
	int				p14;	// interference
	Tuple			p15;	// return cell
	unsigned char	p16;	// cohesion
	unsigned		p17;	// virtual decay
	unsigned		p18;	// disambiguation
	//
	// Auxiliary
	//
	unsigned		p19;	// w address
	unsigned		p20;	// w of peer
	unsigned char	p21;	// status
	unsigned char	p22;	// is pair flag
	unsigned char	p23;	// wavefront direction
	unsigned		p24;	// wavefront synch: t1
	unsigned char	p25;	// messenger code
	Tuple			p26;	// burst origin vector;
	unsigned char	p27;	// burst direction
	double 			p28a1;	// a1
	double			p28a2;	// a2

} Brick;

// Functions declarations

__device__ void copyBrick(Brick *dst, Brick *org);
__device__ void cleanBrick(Brick *t);
// __device__ unsigned signature(Brick *b);

#endif /* PREON_H_ */


