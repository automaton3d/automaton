/*
 * main3d.c
 *
 *  Created on: 4 de mar de 2017
 *      Author: Alexandre
 */

#define _GNU_SOURCE

#include "main3d.h"

// UID

BITMAPINFO bmInfo;
HDC myCompatibleDC;
HBITMAP myBitmap;
HWND hwnd;
HWND front_chk, track_chk, p_chk, plane_chk, cube_chk, latt_chk, axes_chk;
HWND single_rad, partial_rad, full_rad;
COLORREF background;
boolean momentum, wavefront, mode0, mode1, mode2, track, cube, plane, lattice, axes;
HPEN xPen, yPen, zPen, boxPen;

// Trackball

boolean drag;
float r;
float lastQ[4];
float currQ[4];
int startx, starty;

// Simulation

pthread_t loop;
unsigned long begin;
boolean stop;
unsigned long timer = 0;

LRESULT CALLBACK MyWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    int x = GET_X_LPARAM(lparam);
    int y = GET_Y_LPARAM(lparam);
	switch(msg)
	{
    	case WM_PAINT:
        {
        	PAINTSTRUCT ps;
        	HDC hdc = BeginPaint(hwnd, &ps);
        	RECT clirect;

        	// Draw title.

        	GetClientRect(hwnd, &clirect);
        	RECT rect;
        	rect.left = clirect.right / 2 - 200;
        	rect.right = clirect.right / 2 + 200;
        	rect.bottom = 60;
        	rect.top = 10;
        	LOGFONT lf;
        	ZeroMemory(&lf, sizeof(LOGFONT));
        	lf.lfHeight = 32;
        	lf.lfWeight = FW_NORMAL;
        	lf.lfCharSet = DEFAULT_CHARSET;
        	lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
        	lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
        	lf.lfQuality = DEFAULT_QUALITY;
        	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
        	lstrcpy(lf.lfFaceName, "Arial");  // set the font face name
        	HFONT hFont = CreateFontIndirect(&lf);
        	HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        	DrawText(hdc, "Simulation", -1, &rect, DT_CENTER | DT_TOP | DT_SINGLELINE);
        	SelectObject(hdc, hOldFont);
        	DeleteObject(hFont);

        	// Real time display

        	unsigned long millis = GetTickCount64() - begin;
        	char *s = NULL;
        	asprintf(&s, "Elapsed %.1fs ", millis / 1000.0);
        	rect.left   = 300;
        	rect.right  = 450;
        	rect.bottom = 90;
        	rect.top    = 72;
        	DrawText(hdc, s, -1, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        	// Draw the 3d bitmap

        	HDC hdcMem = CreateCompatibleDC(hdc);
        	SelectObject(hdcMem, myBitmap);
        	rect.left = 0;
        	rect.bottom = 0;
        	rect.right = WIDTH;
        	rect.top = HEIGHT;
        	HBRUSH hbrBkGnd = CreateSolidBrush(RGB(0, 0, 0));
        	FillRect(hdcMem, &rect, hbrBkGnd);
        	DeleteObject(hbrBkGnd);
        	SetBkMode(hdcMem, TRANSPARENT);
        	drawModel(hdcMem);
	        update2d(hdcMem);
        	BitBlt(hdc, BMAPX, BMAPY, WIDTH, HEIGHT, hdcMem, 0, 0, SRCCOPY);
        	DeleteDC(hdcMem);
        	EndPaint(hwnd, &ps);
        	break;
        }
	    case WM_CREATE:
    	{
			ZeroMemory(&bmInfo, sizeof(BITMAPINFO));
			bmInfo.bmiHeader.biBitCount    = 32;
			bmInfo.bmiHeader.biHeight      = HEIGHT;
			bmInfo.bmiHeader.biPlanes      = 1;
			bmInfo.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
			bmInfo.bmiHeader.biWidth       = WIDTH;
			bmInfo.bmiHeader.biCompression = BI_RGB;
			single_rad = CreateWindow(
                "BUTTON", "Single",
                WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP,
                50, 350, 100, 20,
                hwnd, (HMENU)MODE0,
				(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
			partial_rad = CreateWindow(
                "BUTTON", "Partial",
                WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
                50, 400, 100, 20,
                hwnd, (HMENU)MODE1,
				(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            full_rad = CreateWindow(
                "BUTTON", "Full",
                WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
                50, 450, 100, 20,
                hwnd, (HMENU)MODE2,
				(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            SendMessage(full_rad, BM_SETCHECK, BST_CHECKED, 0);
         	front_chk = CreateCheckBox(hwnd, 50, 50, 150, 30, FRONT, (LPCWSTR)"Wavefront");
         	track_chk = CreateCheckBox(hwnd, 50, 90, 150, 30, TRACK, (LPCWSTR)"Track");
         	p_chk     = CreateCheckBox(hwnd, 50, 130, 150, 30, MOMENTUM, (LPCWSTR)"Momentum");
         	plane_chk = CreateCheckBox(hwnd, 50, 170, 150, 30, PLANE, (LPCWSTR)"Plane");
         	cube_chk  = CreateCheckBox(hwnd, 50, 210, 150, 30, CUBE, (LPCWSTR)"Cube");
         	latt_chk  = CreateCheckBox(hwnd, 50, 250, 150, 30, LATTICE, (LPCWSTR)"Lattice");
         	axes_chk  = CreateCheckBox(hwnd, 50, 290, 150, 30, AXES, (LPCWSTR)"Axes");
         	SendMessage(front_chk, BM_SETCHECK, BST_CHECKED, 0);
         	SendMessage(p_chk, BM_SETCHECK, BST_CHECKED, 0);
         	SendMessage(axes_chk, BM_SETCHECK, BST_CHECKED, 0);
            // Initial orientation
        	lastQ[0] = rand() / (double)INT_MAX;
        	lastQ[1] = rand() / (double)INT_MAX;
        	lastQ[2] = rand() / (double)INT_MAX;
        	lastQ[3] = rand() / (double)INT_MAX;
        	normalize_quat(lastQ);
        	// Identity
        	currQ[0] = 1;
        	currQ[1] = 0;
        	currQ[2] = 0;
        	currQ[3] = 0;
        	drag = false;
            r = min(WIDTH, HEIGHT) / 3.0;
            SetTimer(hwnd, 1, 32, NULL);
            background = RGB(255,255,255);
            begin = GetTickCount64();
        	xPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
        	yPen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
        	zPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 255));
        	boxPen = CreatePen(PS_SOLID, 1, RGB(10, 100, 15));
			break;
    	}

    	case WM_ERASEBKGND: // avoid flicker
    		return 1;

		case WM_DESTROY:
            ReleaseCapture();
			DeleteAutomaton();
    		DeleteObject(myBitmap) ;
    	    DeleteDC(myCompatibleDC);
			DeleteObject(xPen);
			DeleteObject(yPen);
			DeleteObject(zPen);
			DeleteObject(boxPen);
			PostQuitMessage(0);
			exit(0);
			break;

		case WM_LBUTTONDOWN:
			startx = x;
			starty = y;
			drag = true;
			break;

		case WM_LBUTTONUP:
			if (!drag)
				break;
			mul(lastQ, currQ, lastQ);
		    currQ[0] = 1;
		    currQ[1] = 0;
		    currQ[2] = 0;
		    currQ[3] = 0;
		    drag = false;
			break;

		case WM_MOUSEMOVE:
		{
			if (!drag)
				break;
			float dquat[4];
			trackball (dquat,
				 (2.0*startx - WIDTH) / WIDTH,
				 (HEIGHT - 2.0*starty) / HEIGHT,
				 (2.0*x - WIDTH) / WIDTH,
				 (HEIGHT - 2.0*y) / HEIGHT);
			add_quats (lastQ, dquat, currQ);
            break;
		}
		case WM_KEYDOWN:
			keyboard(msg, wparam, lparam);
			break;

		case WM_TIMER:
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case WM_SIZE:
            //GetClientRect(hwnd, &(tb->box));
            return 0;

		default:
			return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	return lparam;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{ 	setvbuf(stdout, NULL, _IONBF, 0);

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
	HWND g_hBitmap = CreateWindow("STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_BITMAP, BMAPX, BMAPY, 0, 0, hwnd, NULL, hInstance, NULL);
    SendMessage(g_hBitmap, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)myBitmap);
    ReleaseDC(hwnd, hdc);
 	//
 	pthread_create(&loop, NULL, &SimulationLoop, NULL);
 	Sleep(2);
	srand(time(NULL));
    InvalidateRect(hwnd, NULL, TRUE);
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
