/*
 * main3d.c
 *
 *  Created on: 4 de mar de 2017
 *      Author: Alexandre
 */

#define _GNU_SOURCE

#define PTW32_STATIC_LIB

#include <windows.h>
#include "main3d.h"
#include <GL/GL.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "utils.h"
#include "engine.h"
#include "plot3d.h"
#include "simulation.h"
#include "tuple.h"
#include "keyboard.h"
#include "mouse.h"

extern unsigned long begin;

// Global variables

BITMAPINFO bmInfo;
HDC myCompatibleDC;
HBITMAP myBitmap;
HDC hdc;
pthread_t loop, display;
pthread_barrier_t barrier;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_barrierattr_t attr;
#define FRONT		0
#define TRACK		1
#define MOMENTUM	2
#define PLANE		3
#define CUBE		4
#define MODE0		5
#define MODE1		6
#define MODE2		7

HWND front_chk, track_chk, p_chk, plane_chk, cube_chk;
HWND mode0_rad, mode1_rad, mode2_rad;

LRESULT CALLBACK MyWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(msg)
	{
    	case WM_PAINT:
	        {
	            PAINTSTRUCT ps;
	            HDC hdc = BeginPaint(hwnd, &ps);
	            RECT clirect;
	            GetClientRect(hwnd, &clirect);
	            RECT rect;
	            rect.left = clirect.right / 2 - 200;
	            rect.right = clirect.right / 2 + 200;
	            rect.bottom = 60;
	            rect.top = 10;

	            // Create a LOGFONT structure and set its properties

	            LOGFONT lf;
	            ZeroMemory(&lf, sizeof(LOGFONT));
	            lf.lfHeight = 32;  // set the font height to 20 pixels
	            lf.lfWeight = FW_NORMAL;
	            lf.lfCharSet = DEFAULT_CHARSET;
	            lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
	            lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	            lf.lfQuality = DEFAULT_QUALITY;
	            lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	            lstrcpy(lf.lfFaceName, "Arial");  // set the font face name

	            // Create a font object based on the LOGFONT structure

	            HFONT hFont = CreateFontIndirect(&lf);

	            // Select the font object into the device context

	            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

	            DrawText(hdc, "The universe in an automaton", -1, &rect, DT_CENTER | DT_TOP | DT_SINGLELINE);
	         	char *s = NULL;
	            //
	            // Select the original font object back into the device context

	            SelectObject(hdc, hOldFont);

	            // Delete the font object

	            DeleteObject(hFont);

	            asprintf(&s, "(SIDE = %d)", SIDE);
	            DrawText(hdc, s, -1, &rect, DT_CENTER | DT_BOTTOM | DT_SINGLELINE);

	            rect.left   = 300;
	            rect.right  = 450;
	            rect.bottom = 90;
	            rect.top    = 72;

	            HBRUSH hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	            FillRect(hdc, &rect, hBrush);

	         	unsigned long millis = GetTickCount64() - begin;
	         	asprintf(&s, "Elapsed %.1fs ", millis / 1000.0);
	            DrawText(hdc, s, -1, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
	            EndPaint(hwnd, &ps);
	            break;
	        }
	    case WM_CREATE:
	    	{
    		hdc = GetDC(hwnd);
    		pixels = (DWORD*) malloc(WIDTH * HEIGHT * sizeof(DWORD));
			ZeroMemory(&bmInfo, sizeof(BITMAPINFO));
			bmInfo.bmiHeader.biBitCount    = 32;
			bmInfo.bmiHeader.biHeight      = HEIGHT;
			bmInfo.bmiHeader.biPlanes      = 1;
			bmInfo.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
			bmInfo.bmiHeader.biWidth       = WIDTH;
			bmInfo.bmiHeader.biCompression = BI_RGB;

            // Create the first radio button

			mode0_rad = CreateWindow(
                "BUTTON", "Mode 0",
                WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP,
                50, 350, 100, 20,
                hwnd, (HMENU)MODE0,
				(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

            // Create the second radio button

			mode1_rad = CreateWindow(
                "BUTTON", "Mode 1",
                WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
                50, 400, 100, 20,
                hwnd, (HMENU)MODE1,
				(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

            // Create the third radio button

            mode2_rad = CreateWindow(
                "BUTTON", "Mode 2",
                WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
                50, 450, 100, 20,
                hwnd, (HMENU)MODE2,
				(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

            // Check the first radio button by default

            SendMessage(mode0_rad, BM_SETCHECK, BST_CHECKED, 0);
			break;
	    	}

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

HWND CreateCheckBox(HWND hwndParent, int x, int y, int width, int height, int id, LPCWSTR text)
{
	return CreateWindowA("BUTTON", (char *)text, WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, x, y, width, height, hwndParent, NULL, NULL, NULL);
}

void CreateLabel(HWND hwndParent, int x, int y, int width, int height, LPCWSTR text)
{
    CreateWindow ("STATIC", (LPCSTR)text, WS_VISIBLE | WS_CHILD | SS_LEFT, x, y, width, height, hwndParent, NULL, NULL, NULL);
}

HWND g_hBitmap;


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
 	// Get the dimensions of the screen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    //
	RegisterClass(&wc);
	hwnd = CreateWindow("MYWNDCLASSNAME", title,
		WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		20, 20, screenWidth - 40, screenHeight -80, NULL, NULL, hInstance, NULL);
 	free(title);

    // Create checkboxes

 	front_chk = CreateCheckBox(hwnd, 50, 50, 150, 30, FRONT, (LPCWSTR)"Wavefront");
 	track_chk = CreateCheckBox(hwnd, 50, 100, 150, 30, TRACK, (LPCWSTR)"Track");
 	p_chk     = CreateCheckBox(hwnd, 50, 150, 150, 30, MOMENTUM, (LPCWSTR)"Momentum");
 	plane_chk = CreateCheckBox(hwnd, 50, 200, 150, 30, PLANE, (LPCWSTR)"Plane");
 	cube_chk  = CreateCheckBox(hwnd, 50, 250, 150, 30, CUBE, (LPCWSTR)"Cube");

 	SendMessage(front_chk, BM_SETCHECK, BST_CHECKED, 0);
 	SendMessage(p_chk, BM_SETCHECK, BST_CHECKED, 0);

    // Create label

    //CreateLabel (hwnd, 50, 150, 150, 30, (LPCWSTR)"Label");

    // Show window

    ShowWindow(hwnd, nShowCmd);
    UpdateWindow(hwnd);

    // Create the bitmap child window

    HDC hdc = GetDC(hwnd);
	myBitmap = CreateCompatibleBitmap(hdc, WIDTH, HEIGHT);

    g_hBitmap = CreateWindow("STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_BITMAP, 300, 100, 0, 0, hwnd, NULL, hInstance, NULL);

    SendMessage(g_hBitmap, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)myBitmap);
    ReleaseDC(hwnd, hdc);

 	setvbuf(stdout, NULL, _IONBF, 0);
 	//
 	pthread_barrier_init(&barrier, &attr, 2);
 	pthread_create(&display, NULL, &DisplayLoop, NULL);
 	Sleep(2);
 	pthread_create(&loop, NULL, &AutomatonLoop, NULL);
 	Sleep(2);
	srand(time(NULL));
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
