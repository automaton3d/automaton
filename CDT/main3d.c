/*
 * main3d.c
 *
 *  Created on: 4 de mar de 2017
 *      Author: Alexandre
 */

#define _GNU_SOURCE

#include <windowsx.h>
#include "plot3d.h"
#include "simulation.h"
#include "mouse.h"

extern unsigned long begin;
extern DWORD *pixels;

boolean stop;

// Global variables

BITMAPINFO bmInfo;
HDC myCompatibleDC;
HBITMAP myBitmap;
HDC hdc;
pthread_t loop, display;
pthread_barrier_t barrier;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_barrierattr_t attr;

HWND g_hBitmap;
HWND hwnd;

Trackball trackball;
Trackball *tb = &trackball;

float modelTransformationMatrix[16] =
{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

LRESULT CALLBACK MyWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    int x = GET_X_LPARAM(lparam);
    int y = GET_Y_LPARAM(lparam);
    Trackball* tb = &trackball;
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

	            DrawText(hdc, "Simulation", -1, &rect, DT_CENTER | DT_TOP | DT_SINGLELINE);
	         	char *s = NULL;
	            rect.left   = 300;
	            rect.right  = 450;
	            rect.bottom = 90;
	            rect.top    = 72;

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

            UINT_PTR nIDEvent = 1; // Timer identifier
            UINT uElapse = 100; // 1 second interval

            tb->p[0] = 1;
            tb->p[1] = 0;
            tb->p[2] = 0;
            tb->p[3] = 0;
            tb->q[0] = 1;
            tb->q[1] = 0;
            tb->q[2] = 0;
            tb->q[3] = 0;
            tb->drag_startVector = NULL;
            tb->box.left = 0;
            tb->box.top = 0;
            tb->box.right = WIDTH;
            tb->box.bottom = HEIGHT;
            tb->slideID = 0;
            tb->dragged = 0;
            tb->smooth = 0;
            tb->limitAxis = NULL;
            tb->angleChange = 0;
            tb->axis = NULL;
            tb->angle = 0;
            tb->oldTime = 0;
            tb->curTime = 0;
            SetTimer(hwnd, nIDEvent, uElapse, NULL);//TimerProc);

            // JC

            initPlot();
            flipBuffers();  // forces init of curr
            begin = GetTickCount64();

			break;
	    	}

    	case WM_ERASEBKGND:
    		return 1;

		case WM_DESTROY:
            // Release the capture and exit the application
            ReleaseCapture();	// JS
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
		{
            mouse(msg, x, y);
            break;
		}

		case WM_KEYDOWN:
			keyboard(msg, wparam, lparam);
			break;

		case WM_TIMER:
			drawModel();
	        flipBuffers();
	        clearBuffer();
	        setWindow(-WINDOW, WINDOW, -WINDOW, WINDOW);
	        newView2();
	        newProjection();
	        newView3();
	        update3d();
	        SetDIBits(myCompatibleDC, myBitmap, 0, HEIGHT, pixels, &bmInfo, 0);
	        BitBlt(hdc, 0, 0, WIDTH, HEIGHT, myCompatibleDC, 0, 0, SRCCOPY);
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case WM_SIZE:
            GetClientRect(hwnd, &(tb->box));
            return 0;

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
	MSG msg;
	//
	ZeroMemory(&wc, sizeof(WNDCLASS));
	//
	wc.hInstance     = hInstance;
	wc.lpfnWndProc   = MyWndProc;
	wc.lpszClassName = "MYWNDCLASSNAME";
	wc.hbrBackground = NULL;
	wc.hCursor       = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	//
 	char *title = NULL;
 	asprintf(&title, "Toy universe");
 	//
 	// Get the dimensions of the screen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Start the rendering loop
    SetTimer(hwnd, 1, 16, NULL); // 16 ms interval for approximately 60 FPS
    //
	RegisterClass(&wc);
	hwnd = CreateWindow("MYWNDCLASSNAME", title,
		WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		20, 20, screenWidth - 500, screenHeight - 100, NULL, NULL, hInstance, NULL);
 	free(title);

    // Show window

    ShowWindow(hwnd, nShowCmd);
    UpdateWindow(hwnd);

    // Create the bitmap child window

    HDC hdc = GetDC(hwnd);
	myBitmap = CreateCompatibleBitmap(hdc, WIDTH, HEIGHT);

    g_hBitmap = CreateWindow("STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_BITMAP, BMAPX, BMAPY, 0, 0, hwnd, NULL, hInstance, NULL);

    SendMessage(g_hBitmap, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)myBitmap);
    ReleaseDC(hwnd, hdc);

 	setvbuf(stdout, NULL, _IONBF, 0);
 	//
 	pthread_create(&loop, NULL, &SimulationLoop, NULL);
 	Sleep(2);
	srand(time(NULL));
 	//
	Beep(1000, 100);
	while(GetMessage(&msg, hwnd, 0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
    return msg.wParam;
}

/*
 *
 */
void DeleteAutomaton()
{
	puts("done.");
}

void keyboard(UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(wparam) { case 'S': stop = !stop; }
}
