/*
 * keyboard.c
 *
 *  Created on: 24 de jul de 2017
 *      Author: Alexandre
 */

/*
		case 'k':
			switch(wparam)
			{
				case 'A':
					showAxes = !showAxes;
					break;
				case 'B':
					background ^= 0x00ffffff;
					gridcolor = (background == BLACKBG) ? 0x00333333 : 0x00aaaaaa;
					break;
				case 'G':
					showGrid = !showGrid;
					break;
				case 'P':
					flipMode();
					break;
				case 'S':
					stop = !stop;
					break;
				case 'X':
					flipBox();
					break;
				case '0':
					position.x = sqrt(3) *1.5 * GRID;
					position.y = sqrt(3) *1.5 * GRID;
					position.z = sqrt(3) *1.5 * GRID;
					direction.x = -position.x;		// camera axis
					direction.y = -position.y;
					direction.z = -position.z;
					norm3d(&direction);
					break;
				case '1':
					direction.x = -1;
					direction.y = 0;
					direction.z = 0;
					attitude.x = 0;
					attitude.y = 0;
					attitude.z = 1;
					position.x = 1.5 * GRID;
					position.y = 0;
					position.z = 0;
					break;
				case '2':
					direction.x = 0;
					direction.y = -1;
					direction.z = 0;
					attitude.x = 0;
					attitude.y = 0;
					attitude.z = 1;
					position.x = 0;
					position.y = 1.5 * GRID;
					position.z = 0;
					break;
				case '3':
					direction.x = 0;
					direction.y = 0;
					direction.z = -1;
					attitude.x = 1;
					attitude.y = 0;
					attitude.z = 0;
					position.x = 0;
					position.y = 0;
					position.z = 1.5 * GRID;
					break;
			}
			break;

*/
