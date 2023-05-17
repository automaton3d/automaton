/*
 * view.h
 *
 *  Created on: 15 de mai. de 2023
 *      Author: Alexandre
 */

#ifndef VIEW_H_
#define VIEW_H_

#include <windows.h>

typedef struct
{
	int width, height;
    float rotation[4];

    // Novo

    float currQ[4], lastQ[4];
//	float perspective;
//  float scalex, scaley;
//	float translate[2];
	boolean dragged;
//	int buttons;
//	int modifiers;
	int beginx, beginy;

} View;

#endif /* VIEW_H_ */
