/*
 * main3d.c
 */

#include "main.h"

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include "vector3d.h"
#include "utils.h"
#include "plot3d.h"
#include "automaton.h"
#include "mouse.h"
#include "quaternion.h"
#include "text.h"

LRESULT CALLBACK MyWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
pthread_t loop, display;

boolean stop = FALSE;

Quaternion q, qstart;
DWORD* pixels;

boolean splash = true;
int item = -1;

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
	printf("Running...\n"); fflush(stdout);
 	char *title;
 	asprintf(&title, "Automaton %dx%dx%dx%d", SIDE, SIDE, SIDE, NPREONS);
 	//
	RegisterClass(&wc);
	hwnd = CreateWindow("MYWNDCLASSNAME", title,
		WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
		500, 20, WIDTH + 4, HEIGHT + 4, NULL, NULL, hInstance, NULL);
 	free(title);
 	//
 	ShowWindow (hwnd, nShowCmd);
 	UpdateWindow (hwnd) ;
 	//
	while(GetMessage(&msg, hwnd, 0,0))
		DispatchMessage(&msg);
	return 0;
}

LRESULT CALLBACK MyWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(msg)
	{
		case WM_CREATE:
    		hdc = GetDC(hwnd);
    		pixels = (DWORD*) malloc(WIDTH * HEIGHT * sizeof(DWORD));
    		//
			ZeroMemory(&bmInfo, sizeof(BITMAPINFO));
			bmInfo.bmiHeader.biBitCount    = 32;
			bmInfo.bmiHeader.biHeight      = HEIGHT;
			bmInfo.bmiHeader.biPlanes      = 1;
			bmInfo.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
			bmInfo.bmiHeader.biWidth       = WIDTH;
			bmInfo.bmiHeader.biCompression = BI_RGB;
			//
			myBitmap = CreateCompatibleBitmap(hdc, WIDTH, HEIGHT);
			dc = CreateCompatibleDC(hdc);
			SelectObject(dc, myBitmap);
			//
		 	pthread_create(&display, NULL, &DisplayLoop, NULL);
			break;

    	case WM_ERASEBKGND:
    		return 1;

		case WM_DESTROY:
			DeleteAutomaton();
    		DeleteObject(myBitmap) ;
    	    DeleteDC(dc);
    	    DeleteDC(hdc);
			PostQuitMessage(0);
			exit(0);
			break;

		case WM_LBUTTONDOWN:
		{
			if(splash)
			{
				scene = mouse('p', LOWORD(lparam), HIWORD(lparam));
				if(scene >= 0)
				{
					item = -1;
					initPlot(pixels);
					pthread_create(&loop, NULL, &AutomatonLoop, NULL);
					sleep(1);
					splash = false;
				}
			}
			else
				mouse('d', LOWORD(lparam), HIWORD(lparam));
			break;
		}

		case WM_LBUTTONUP:
			if(!splash)
				mouse('u', LOWORD(lparam), HIWORD(lparam));
			break;

		case  WM_MOUSEMOVE:
			if(!splash)
				mouse('m', LOWORD(lparam), HIWORD(lparam));
			else
			{
				TRACKMOUSEEVENT tme;
				tme.cbSize = sizeof(tme);
				tme.dwFlags = TME_HOVER;
				tme.dwHoverTime = 5;
				tme.hwndTrack = hwnd;
				TrackMouseEvent(&tme);
			}
			break;

		case WM_MOUSEHOVER:
			if(splash)
				item = mouse('h', LOWORD(lparam), HIWORD(lparam));
			break;
		case WM_KEYDOWN:
			if(splash)
			{
				if(wparam == ESC)
					SendMessage(hwnd, WM_DESTROY, wparam, lparam);
				break;
			}
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
				case 'O':
					showOrgs = !showOrgs;
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
					attitude.x = 0;
					attitude.y = 0;
					attitude.z = -1;
					position.x = sqrt(3) *1.5 * GRID;
					position.y = sqrt(3) *1.5 * GRID;
					position.z = sqrt(3) *1.5 * GRID;
					direction.x = -position.x;		// camera axis
					direction.y = -position.y;
					direction.z = -position.z;
					norm3d(&direction);
					break;
				case '1':
					attitude.x = 1;
					attitude.y = 0;
					attitude.z = 0;
					position.x = 0;
					position.y = 0;
					position.z = 1.5 * GRID;
					direction.x = 0;
					direction.y = 0;
					direction.z = -1;
					break;
				case '2':
					attitude.x = 0;
					attitude.y = 0;
					attitude.z = 1;
					position.x = 1.5 * GRID;
					position.y = 0;
					position.z = 0;
					direction.x = -1;
					direction.y = 0;
					direction.z = 0;
					break;
				case '3':
					attitude.x = 0;
					attitude.y = 0;
					attitude.z = 1;
					position.x = 0;
					position.y = 1.5 * GRID;
					position.z = 0;
					direction.x = 0;
					direction.y = -1;
					direction.z = 0;
					break;
				case ESC:
					SendMessage(hwnd, WM_DESTROY, wparam, lparam);
					break;
			}
			break;

		default:
			return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	return lparam;
}

/*
 * Release memory.
 */
void DeleteAutomaton()
{
	free(pri0);
	free(dual0);
	free(pixels);
	puts("...done.");
}

