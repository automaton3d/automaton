/*
 * brick.h
 */

#ifndef BRICK_H_
#define BRICK_H_

#include "tuple.h"

// General code

#define UNDEF		0

// p8 gravity

#define REAL		+1
#define VIRTUAL		-1

// p9 color

#define RED    		0x20
#define GREEN  		0x10
#define BLUE   		0x08
#define ANTIRED    	0x04
#define ANTIGREEN  	0x02
#define ANTIBLUE   	0x01
#define WHITE		(RED | GREEN | BLUE)
#define ANTIWHITE	(ANTIRED | ANTIGREEN | ANTIBLUE)
#define LEPT		0x38
#define ANTILEPT	0x07

// p13 preon interactions

#define U			1		// unpaired
#define P			2		// pair

#define PXP			4
#define UXU			8
#define UXP			16
#define UXG			32
#define REISSUE		64
#define VACUUM		128

// p16

#define FREE		0
#define BOUND		1

// p21 status

#define	PREON		0X01
#define GRAV		0X04
#define ORG			0x08

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
	char			p6s;	// static force
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
	char			p29;	// gravity excitation
	unsigned		p30;	// directionality
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
	unsigned		p18;	// disambiguation

} Brick;

// Functions declarations

char *brick2str(Brick *t);
void copyBrick(Brick *dst, Brick *org);
void cleanBrick(Brick *t);
unsigned signature(Brick *b);
boolean isColored(Brick *b);
Tuple getOrg(Brick *b);

#endif /* PREON_H_ */
