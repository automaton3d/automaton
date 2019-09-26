/*
 * main3d.c
 *
 *  Created on: 4 de mar de 2017
 *      Author: Alexandre
 */

#include "main3d.h"
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include "utils.h"
#include "plot3d.h"
#include "automaton.h"
#include "mouse.h"

DWORD* pixels;
boolean stop = FALSE;

LRESULT CALLBACK MyWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(msg)
	{
		case WM_CREATE:
    		hdc = GetDC(hwnd);
    		pixels = (DWORD*) malloc(WIDTH * HEIGHT * sizeof(DWORD));
    		initPlot(pixels);
			ZeroMemory(&bmInfo, sizeof(BITMAPINFO));
			bmInfo.bmiHeader.biBitCount    = 32;
			bmInfo.bmiHeader.biHeight      = HEIGHT;
			bmInfo.bmiHeader.biPlanes      = 1;
			bmInfo.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
			bmInfo.bmiHeader.biWidth       = WIDTH;
			bmInfo.bmiHeader.biCompression = BI_RGB;
			//
			myBitmap = CreateCompatibleBitmap(GetDC(hwnd), WIDTH, HEIGHT);
			myCompatibleDC = CreateCompatibleDC(GetDC(hwnd));
			SelectObject(myCompatibleDC, myBitmap);
			break;

    	case WM_ERASEBKGND:
    		return 1;

		case WM_DESTROY:
			DeleteAutomaton();
    		DeleteObject(myBitmap) ;
    	    DeleteDC(myCompatibleDC);
    	    DeleteDC(hdc);
			PostQuitMessage(0);
			exit(0);
			break;

		case WM_LBUTTONDOWN:
		{
			mouse('d', HIWORD(lparam), LOWORD(lparam));
			break;
		}

		case WM_LBUTTONUP:
			mouse('u', HIWORD(lparam), LOWORD(lparam));
			break;

		case  WM_MOUSEMOVE:
			mouse('m', HIWORD(lparam), LOWORD(lparam));
			break;

		case WM_RBUTTONDOWN:
			mouse('t', HIWORD(lparam), LOWORD(lparam));
			break;

		case WM_KEYDOWN:
			switch(wparam)
			{
				case 'A':
					showAxes = !showAxes;
					break;
				case 'B':
					if(background == BLK)
					{
						background = WHT;
						gridcolor = BB;
					}
					else
					{
						background = BLK;
						gridcolor = GG;
					}
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

		default:
			return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	return lparam;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	CreateEvent(NULL, FALSE, FALSE, "Launching...");
	if(GetLastError() == ERROR_ALREADY_EXISTS)
	{
	    puts("double instance not allowed");
	    return FALSE;
	}
	WNDCLASS wc;
	HWND hwnd;
	MSG msg;
	//
	ZeroMemory(&wc, sizeof(WNDCLASS));
	//
	wc.hInstance     = hInstance;
	wc.lpfnWndProc   = MyWndProc;
	wc.lpszClassName = "MYWNDCLASSNAME";
	wc.hbrBackground = NULL;
	wc.hCursor       = LoadCursor(0, IDC_ARROW);
	//
 	char *title;
 	asprintf(&title, "Automaton %dx%dx%dx%d", SIDE, SIDE, SIDE, NPREONS);
 	//
	RegisterClass(&wc);
	hwnd = CreateWindow("MYWNDCLASSNAME", title,
		WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
		500, 20, WIDTH + 4, HEIGHT + 4, NULL, NULL, hInstance, NULL);
 	free(title);
	//
 	pthread_create(&loop, NULL, &AutomatonLoop, NULL);
 	sleep(2);
 	pthread_create(&display, NULL, &DisplayLoop, NULL);
 	sleep(2);
 	//
	while(GetMessage(&msg, hwnd, 0,0))
		DispatchMessage(&msg);
	return 0;
}

/*
 *
 */
void DeleteAutomaton()
{
	puts("done.");
	free(pri0);
	free(dual0);
}

