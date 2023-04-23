/*
 * main3d.c
 *
 *  Created on: 4 de mar de 2017
 *      Author: Alexandre
 */

#define _GNU_SOURCE

#define PTW32_STATIC_LIB

#include "main3d.h"
#include <GL/GL.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include "utils.h"
#include "engine.h"
#include "plot3d.h"
#include "simulation.h"
#include "tuple.h"
#include "keyboard.h"
#include "mouse.h"

// Global variables

BITMAPINFO bmInfo;
HDC myCompatibleDC;
HBITMAP myBitmap;
HDC hdc;
pthread_t loop, display;
pthread_barrier_t barrier;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_barrierattr_t attr;

LRESULT CALLBACK MyWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(msg)
	{
		case WM_CREATE:
    		hdc = GetDC(hwnd);
    		pixels = (DWORD*) malloc(WIDTH * HEIGHT * sizeof(DWORD));
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
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
		case WM_RBUTTONDOWN:
		case WM_MOUSEWHEEL:
			mouse(msg, wparam, lparam);
			break;

		case WM_KEYDOWN:
			keyboard(msg, wparam, lparam);
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
 	char *title = NULL;
 	asprintf(&title, "Toy universe");
 	//
	RegisterClass(&wc);
	hwnd = CreateWindow("MYWNDCLASSNAME", title,
		WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
		500, 20, WIDTH + 4, HEIGHT + 4, NULL, NULL, hInstance, NULL);
 	free(title);
 	setvbuf(stdout, NULL, _IONBF, 0);
 	//
 	pthread_barrier_init(&barrier, &attr, 2);
 	pthread_create(&display, NULL, &DisplayLoop, NULL);
 	Sleep(2);
 	pthread_create(&loop, NULL, &AutomatonLoop, NULL);
 	Sleep(2);
	srand(time(NULL));
//	SetWindowTextW(hwnd_test, L"Label:");
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
}
