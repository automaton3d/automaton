/*
 * text.c
 */
#include "text.h"
#include <stdio.h>
#include <assert.h>
#include "params.h"
#include "plot3d.h"
#include "automaton.h"
#include "jpeg.h"

char asciiset[] =
{
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x3E, 0x41, 0x55, 0x41, 0x55, 0x49, 0x3E,
        0x00, 0x3E, 0x7F, 0x6B, 0x7F, 0x6B, 0x77, 0x3E,
        0x00, 0x22, 0x77, 0x7F, 0x7F, 0x3E, 0x1C, 0x08,
        0x00, 0x08, 0x1C, 0x3E, 0x7F, 0x3E, 0x1C, 0x08,
        0x00, 0x08, 0x1C, 0x2A, 0x7F, 0x2A, 0x08, 0x1C,
        0x00, 0x08, 0x1C, 0x3E, 0x7F, 0x3E, 0x08, 0x1C,
        0x00, 0x00, 0x1C, 0x3E, 0x3E, 0x3E, 0x1C, 0x00,	//
        0xFF, 0xFF, 0xE3, 0xC1, 0xC1, 0xC1, 0xE3, 0xFF,
        0x00, 0x00, 0x1C, 0x22, 0x22, 0x22, 0x1C, 0x00,
        0xFF, 0xFF, 0xE3, 0xDD, 0xDD, 0xDD, 0xE3, 0xFF,
        0x00, 0x0F, 0x03, 0x05, 0x39, 0x48, 0x48, 0x30,//
        0x00, 0x08, 0x3E, 0x08, 0x1C, 0x22, 0x22, 0x1C,
        0x00, 0x18, 0x14, 0x10, 0x10, 0x30, 0x70, 0x60,
        0x00, 0x0F, 0x19, 0x11, 0x13, 0x37, 0x76, 0x60,
        0x00, 0x08, 0x2A, 0x1C, 0x77, 0x1C, 0x2A, 0x08,
        0x00, 0x60, 0x78, 0x7E, 0x7F, 0x7E, 0x78, 0x60,
        0x00, 0x03, 0x0F, 0x3F, 0x7F, 0x3F, 0x0F, 0x03,
        0x00, 0x08, 0x1C, 0x2A, 0x08, 0x2A, 0x1C, 0x08,
        0x00, 0x66, 0x66, 0x66, 0x66, 0x00, 0x66, 0x66,
        0x00, 0x3F, 0x65, 0x65, 0x3D, 0x05, 0x05, 0x05,
        0x00, 0x0C, 0x32, 0x48, 0x24, 0x12, 0x4C, 0x30,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x7F, 0x7F,
        0x00, 0x08, 0x1C, 0x2A, 0x08, 0x2A, 0x1C, 0x3E,
        0x00, 0x08, 0x1C, 0x3E, 0x7F, 0x1C, 0x1C, 0x1C,
        0x00, 0x1C, 0x1C, 0x1C, 0x7F, 0x3E, 0x1C, 0x08,
		0x00, 0x08, 0x0C, 0x7E, 0x7F, 0x7E, 0x0C, 0x08,
		0x00, 0x08, 0x18, 0x3F, 0x7F, 0x3F, 0x18, 0x08,	//
		0x00, 0x00, 0x00, 0x70, 0x70, 0x70, 0x7F, 0x7F,
		0x00, 0x00, 0x14, 0x22, 0x7F, 0x22, 0x14, 0x00,
		0x00, 0x08, 0x1C, 0x1C, 0x3E, 0x3E, 0x7F, 0x7F,
		0x00, 0x7F, 0x7F, 0x3E, 0x3E, 0x1C, 0x1C, 0x08,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18,
		0x00, 0x36, 0x36, 0x14, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36,
		0x00, 0x08, 0x1E, 0x20, 0x1C, 0x02, 0x3C, 0x08,
		0x00, 0x60, 0x66, 0x0C, 0x18, 0x30, 0x66, 0x06,
		0x00, 0x3C, 0x66, 0x3C, 0x28, 0x65, 0x66, 0x3F,
		0x00, 0x18, 0x18, 0x18, 0x30, 0x00, 0x00, 0x00,	//
		0x00, 0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06,	// (
		0x00, 0x60, 0x30, 0x18, 0x18, 0x18, 0x30, 0x60,	// )
		0x00, 0x00, 0x36, 0x1C, 0x7F, 0x1C, 0x36, 0x00,
		0x00, 0x00, 0x08, 0x08, 0x3E, 0x08, 0x08, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x30, 0x60,
		0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x60,
		0x00, 0x00, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x00,	//
		0x00, 0x3C, 0x66, 0x6E, 0x76, 0x66, 0x66, 0x3C,
		0x00, 0x18, 0x18, 0x38, 0x18, 0x18, 0x18, 0x7E,
		0x00, 0x3C, 0x66, 0x06, 0x0C, 0x30, 0x60, 0x7E,
		0x00, 0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C,
		0x00, 0x0C, 0x1C, 0x2C, 0x4C, 0x7E, 0x0C, 0x0C,
		0x00, 0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C,
		0x00, 0x3C, 0x66, 0x60, 0x7C, 0x66, 0x66, 0x3C,
        0x00, 0x7E, 0x66, 0x0C, 0x0C, 0x18, 0x18, 0x18,
		0x00, 0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C,
		0x00, 0x3C, 0x66, 0x66, 0x3E, 0x06, 0x66, 0x3C,
		0x00, 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00,
		0x00, 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x30,
		0x00, 0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06,
		0x00, 0x00, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x00,
		0x00, 0x60, 0x30, 0x18, 0x0C, 0x18, 0x30, 0x60,
		0x00, 0x3C, 0x66, 0x06, 0x1C, 0x18, 0x00, 0x18,
		0x00, 0x38, 0x44, 0x5C, 0x58, 0x42, 0x3C, 0x00,
		0x00, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66,
		0x00, 0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C,
		0x00, 0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C,
		0x00, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x7C,
		0x00, 0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E,
		0x00, 0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60,
		0x00, 0x3C, 0x66, 0x60, 0x60, 0x6E, 0x66, 0x3C,
		0x00, 0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66,
		0x00, 0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C,
		0x00, 0x1E, 0x0C, 0x0C, 0x0C, 0x6C, 0x6C, 0x38,
		0x00, 0x66, 0x6C, 0x78, 0x70, 0x78, 0x6C, 0x66,
		0x00, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E,
		0x00, 0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x63,
		0x00, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x63, 0x63,
		0x00, 0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C,
		0x00, 0x7C, 0x66, 0x66, 0x66, 0x7C, 0x60, 0x60,
		0x00, 0x3C, 0x66, 0x66, 0x66, 0x6E, 0x3C, 0x06,
		0x00, 0x7C, 0x66, 0x66, 0x7C, 0x78, 0x6C, 0x66,
		0x00, 0x3C, 0x66, 0x60, 0x3C, 0x06, 0x66, 0x3C,
		0x00, 0x7E, 0x5A, 0x18, 0x18, 0x18, 0x18, 0x18,
		0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3E,
		0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18,
		0x00, 0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63,
		0x00, 0x63, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x63,
		0x00, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18,
		0x00, 0x7E, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x7E,
		0x00, 0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E,
		0x00, 0x00, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x00,
		0x00, 0x78, 0x18, 0x18, 0x18, 0x18, 0x18, 0x78,
		0x00, 0x08, 0x14, 0x22, 0x41, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F,
		0x00, 0x0C, 0x0C, 0x06, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x3C, 0x06, 0x3E, 0x66, 0x3E,
		0x00, 0x60, 0x60, 0x60, 0x7C, 0x66, 0x66, 0x7C,
		0x00, 0x00, 0x00, 0x3C, 0x66, 0x60, 0x66, 0x3C,
		0x00, 0x06, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3E,
		0x00, 0x00, 0x00, 0x3C, 0x66, 0x7E, 0x60, 0x3C,
		0x00, 0x1C, 0x36, 0x30, 0x30, 0x7C, 0x30, 0x30,
		0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x3C,
		0x00, 0x60, 0x60, 0x60, 0x7C, 0x66, 0x66, 0x66,
		0x00, 0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3C,	// i
		0x00, 0x0C, 0x00, 0x0C, 0x0C, 0x6C, 0x6C, 0x38,	// j
		0x00, 0x60, 0x60, 0x66, 0x6C, 0x78, 0x6C, 0x66,	// k
		0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,	// l
		0x00, 0x00, 0x00, 0x63, 0x77, 0x7F, 0x6B, 0x6B,	// m
		0x00, 0x00, 0x00, 0x7C, 0x7E, 0x66, 0x66, 0x66,	// n
		0x00, 0x00, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C,	// o
		0x00, 0x00, 0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60,	// p
		0x00, 0x00, 0x3C, 0x6C, 0x6C, 0x3C, 0x0D, 0x0F,	// q
		0x00, 0x00, 0x00, 0x7C, 0x66, 0x66, 0x60, 0x60,	// r
		0x00, 0x00, 0x00, 0x3E, 0x40, 0x3C, 0x02, 0x7C,	// s
		0x00, 0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x0e,	// t
		0x00, 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3E,
		0x00, 0x00, 0x00, 0x00, 0x66, 0x66, 0x3C, 0x18,
		0x00, 0x00, 0x00, 0x63, 0x6B, 0x6B, 0x6B, 0x3E,	//
		0x00, 0x00, 0x00, 0x66, 0x3C, 0x18, 0x3C, 0x66,	//
		0x00, 0x00, 0x00, 0x66, 0x66, 0x3E, 0x06, 0x3C,	//
		0x00, 0x00, 0x00, 0x3C, 0x0C, 0x18, 0x30, 0x3C,	//
		0x00, 0x0E, 0x18, 0x18, 0x30, 0x18, 0x18, 0x0E,
		0x00, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18,
		0x00, 0x70, 0x18, 0x18, 0x0C, 0x18, 0x18, 0x70,
		0x00, 0x00, 0x00, 0x3A, 0x6C, 0x00, 0x00, 0x00,
        0x00, 0x08, 0x1C, 0x36, 0x63, 0x41, 0x41, 0x7F,
};

void stColor(int color, JSAMPLE *ptr)
{
	*ptr++ = colors[color];
	*ptr++ = colors[color + 1];
	*ptr   = colors[color + 2];
}

void vprint(int x, int y, char ascii)
{
	JSAMPLE *ptr = image_buffer + 3 * (y * WIDTH + x);
	char *pchar = asciiset + 8 * ascii;
	int i;
	for(i = 0; i < 8; i++)
	{
		char row = *pchar++;
		int j;
		for(j = 0; j < 8; j++)
		{
			if((row & 0x80) == 0)
			{
				stColor(BLK, ptr++);
				stColor(BLK, ptr++);
				stColor(BLK, ptr++);
				stColor(BLK, ptr++);
			}
			else
			{
				stColor(R, ptr++);
				stColor(R, ptr++);
				stColor(R, ptr++);
				stColor(R, ptr++);
			}
			row = row << 1;
		}	
		ptr += 6 * WIDTH - 32;
	}
	ptr = image_buffer + 3 * (y * WIDTH + x) + 3 * WIDTH;
        pchar = asciiset + 8 * ascii;
        for(i = 0; i < 8; i++)
        {
                char row = *pchar++;
                int j;
                for(j = 0; j < 8; j++)
                {
                        if((row & 0x80) == 0)
                        {
                                stColor(BLK, ptr++);
                                stColor(BLK, ptr++);
                                stColor(BLK, ptr++);
                                stColor(BLK, ptr++);
                        }
                        else
                        {
                                stColor(R, ptr++);
                                stColor(R, ptr++);
                                stColor(R, ptr++);
                                stColor(R, ptr++);
                        }
                        row = row << 1;
                }
                ptr += 6 * WIDTH - 32;
        }
}

void vprints(int x, int y, JSAMPLE *str)
{
	while(*str != 0)
	{
		if(*str != ' ')
			vprint(x, y, *str);
		x += 9;
		str++;
	}
}

