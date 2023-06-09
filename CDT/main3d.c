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
HWND single_rad, partial_rad, full_rad, rand_rad;
HWND xy_rad, yz_rad, zx_rad, iso_rad;
HPEN xPen, yPen, zPen, boxPen;
HWND g_hButton, suspendButton;
boolean momentum, wavefront, mode0, mode1, mode2, track, cube, plane, lattice, axes, xy, yz, zx, iso, rnd;

// Trackball

boolean drag;
Quaternion currQ, lastQ;
Quaternion rotation;
float radius;
float width, height;
Vector start;

// Simulation

boolean stop;
boolean active = true;
unsigned long begin;
unsigned long timer = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
RECT bitrect = { BMAPX, BMAPY, BMAPX + WIDTH, BMAPY + HEIGHT };

VOID CALLBACK TimerCallback(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	InvalidateRect(hwnd, &bitrect, TRUE);
}

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

        	// Real time display.

        	unsigned long millis = GetTickCount64() - begin;
        	char *s = NULL;
        	asprintf(&s, "Elapsed %.1fs ", millis / 1000.0);
        	rect.left   = 300;
        	rect.right  = 450;
        	rect.bottom = 90;
        	rect.top    = 72;
        	DrawText(hdc, s, -1, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        	rect.left   = 0;
        	rect.right  = WIDTH;
        	rect.bottom = 0;
        	rect.top    = HEIGHT;

        	// Draw the 3d bitmap.

        	HDC hdcMem = CreateCompatibleDC(hdc);
        	SelectObject(hdcMem, myBitmap);
        	HBRUSH hbrBkGnd = CreateSolidBrush(RGB(0, 0, 0));
        	FillRect(hdcMem, &rect, hbrBkGnd);
        	DeleteObject(hbrBkGnd);
        	SetBkMode(hdcMem, TRANSPARENT);
	        pthread_mutex_lock(&mutex);
	        drawGUI(hdcMem);
        	drawModel(hdcMem);
        	BitBlt(hdc, BMAPX, BMAPY, WIDTH, HEIGHT, hdcMem, 0, 0, SRCCOPY);
            pthread_mutex_unlock(&mutex);
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

            full_rad = CreateWindow(
                "BUTTON", "Full",
                WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP,
                50, 400, 100, 20,
                hwnd, (HMENU)MODE2,
				(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
			single_rad = CreateWindow(
                "BUTTON", "Single",
                WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
                50, 420, 100, 20,
                hwnd, (HMENU)MODE0,
				(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
			partial_rad = CreateWindow(
                "BUTTON", "Partial",
                WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
                50, 440, 100, 20,
                hwnd, (HMENU)MODE1,
				(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            rand_rad = CreateWindow(
                "BUTTON", "Random",
                WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
                50, 460, 100, 20,
                hwnd, (HMENU)RAND,
				(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            //
            iso_rad = CreateWindow(
                "BUTTON", "ISO view",
                WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP,
                50, 540, 100, 20,
                hwnd, (HMENU)ISO_VIEW,
				(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            xy_rad = CreateWindow(
                "BUTTON", "XY view",
                WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
                50, 560, 100, 20,
                hwnd, (HMENU)XY_VIEW,
				(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            yz_rad = CreateWindow(
                "BUTTON", "YZ view",
                WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
                50, 580, 100, 20,
                hwnd, (HMENU)YZ_VIEW,
				(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            zx_rad = CreateWindow(
                "BUTTON", "ZX view",
                WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
                50, 600, 100, 20,
                hwnd, (HMENU)ZX_VIEW,
				(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            SendMessage(full_rad, BM_SETCHECK, BST_CHECKED, 0);
            SendMessage(iso_rad, BM_SETCHECK, BST_CHECKED, 0);
            //
         	front_chk = CreateCheckBox(hwnd, 50, 120, 150, 30, FRONT, (LPCWSTR)"Wavefront");
         	track_chk = CreateCheckBox(hwnd, 50, 150, 150, 30, TRACK, (LPCWSTR)"Track");
         	p_chk     = CreateCheckBox(hwnd, 50, 180, 150, 30, MOMENTUM, (LPCWSTR)"Momentum");
         	plane_chk = CreateCheckBox(hwnd, 50, 210, 150, 30, PLANE, (LPCWSTR)"Plane");
         	cube_chk  = CreateCheckBox(hwnd, 50, 240, 150, 30, CUBE, (LPCWSTR)"Cube");
         	latt_chk  = CreateCheckBox(hwnd, 50, 270, 150, 30, LATTICE, (LPCWSTR)"Lattice");
         	axes_chk  = CreateCheckBox(hwnd, 50, 300, 150, 30, AXES, (LPCWSTR)"Axes");
         	SendMessage(cube_chk, BM_SETCHECK, BST_CHECKED, 0);
         	SendMessage(front_chk, BM_SETCHECK, BST_CHECKED, 0);
         	SendMessage(p_chk, BM_SETCHECK, BST_CHECKED, 0);
         	SendMessage(axes_chk, BM_SETCHECK, BST_CHECKED, 0);
         	//
            // Create the stop button
        	RECT clirect;
        	GetClientRect(hwnd, &clirect);
            g_hButton = CreateWindow(
                TEXT("BUTTON"),
                TEXT("Stop"),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                clirect.right - 200,
				clirect.top + BMAPY,
                100,
                30,
                hwnd,
                NULL,
                ((LPCREATESTRUCT)lparam)->hInstance,
                NULL
            );
            suspendButton = CreateWindow(
                TEXT("BUTTON"),
                TEXT("Background"),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                clirect.right - 200,
				clirect.top + BMAPY + 40,
                100,
                30,
                hwnd,
                NULL,
                ((LPCREATESTRUCT)lparam)->hInstance,
                NULL
            );

            // Initial orientation

            setView(ISO_VIEW, &lastQ);
        	// Identity
            radius = 1.0;
            currQ.w = 1.0;
            currQ.x = 0.0;
            currQ.y = 0.0;
            currQ.z = 0.0;
            start.x = 0.0;
            start.y = 0.0;
        	drag = false;

	        // Start the simulate thread

	        DWORD dwThreadId;
	        HANDLE hSimulateThread = CreateThread(NULL, 0, SimulateThread, NULL, 0, &dwThreadId);
	        CloseHandle(hSimulateThread);
            SetTimer(hwnd, 1, 50, TimerCallback);
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
		{
			mousedown(x, y);
		    drag = true;
		    SetCapture(hwnd);
		    SendMessage(xy_rad, BM_SETCHECK, BST_UNCHECKED, 0);
		    SendMessage(yz_rad, BM_SETCHECK, BST_UNCHECKED, 0);
		    SendMessage(zx_rad, BM_SETCHECK, BST_UNCHECKED, 0);
		    SendMessage(iso_rad, BM_SETCHECK, BST_UNCHECKED, 0);
		    break;
		}
		case WM_LBUTTONUP:
			if (drag)
			{
				mouseup(x, y);
				drag = false;
				ReleaseCapture();
			}
			break;

		case WM_MOUSEMOVE:
		{
			if (drag)
			{
				mousemove(x, y);
	            InvalidateRect(hwnd, &bitrect, TRUE);
			}
            break;
		}
        case WM_MOUSEWHEEL:
        {
        	float wheelDelta = GET_WHEEL_DELTA_WPARAM(wparam);
        	zoom(wheelDelta, &currQ, &lastQ);
            InvalidateRect(hwnd, &bitrect, TRUE);
            break;
        }
		case WM_KEYDOWN:
			keyboard(msg, wparam, lparam);
			break;

        case WM_SIZE:
            //GetClientRect(hwnd, &(tb->box));
        	break;

        case WM_COMMAND:
            if (lparam == (LPARAM)g_hButton && active)
            {
            	stop = !stop;
            	if(stop)
            		SetWindowText(g_hButton, TEXT("Continue"));
            	else
            		SetWindowText(g_hButton, TEXT("Stop"));
            	InvalidateRect(hwnd, NULL, TRUE);
            }
            else if (lparam == (LPARAM)suspendButton && !stop)
            {
            	active = !active;
            	if(active)
            	{
        	        SetTimer(NULL, 1, 50, TimerCallback);
            		SetWindowText(suspendButton, TEXT("Background"));
            	}
            	else
            	{
                    KillTimer(NULL, 1);
            		SetWindowText(suspendButton, TEXT("Foreground"));
            		//sound(); TODO
            	}
            	InvalidateRect(hwnd, NULL, TRUE);
            }
            else if (!active)
            {
                SetTimer(NULL, 1, 50, TimerCallback);
                active = TRUE;
            }
            else if (HIWORD(wparam) == BN_CLICKED)
            {
            	int controlID = LOWORD(wparam);
            	setView(controlID, &lastQ);
            }
        	break;

		default:
			return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
 	setvbuf(stdout, NULL, _IONBF, 0);
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

 	// Get the dimensions of the screen

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	RegisterClass(&wc);
	hwnd = CreateWindow("MYWNDCLASSNAME", "Toy universe",
		WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		20, 20, screenWidth - 500, screenHeight - 100, NULL, NULL, hInstance, NULL);

    // Show window

    ShowWindow(hwnd, nShowCmd);
    UpdateWindow(hwnd);

    // Create the bitmap child window

    HDC hdc = GetDC(hwnd);
	myBitmap = CreateCompatibleBitmap(hdc, WIDTH, HEIGHT);
	HWND g_hBitmap = CreateWindow("STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_BITMAP, BMAPX, BMAPY, 0, 0, hwnd, NULL, hInstance, NULL);
    SendMessage(g_hBitmap, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)myBitmap);
	DrawLabel(hdc, 50, BMAPY, "Screen");
	DrawLabel(hdc, 50, 380, "Data selection");
	DrawLabel(hdc, 50, 520, "Views");
    ReleaseDC(hwnd, hdc);
 	//
	srand(time(NULL));
    InvalidateRect(hwnd, NULL, TRUE);
 	//
	Beep(1000, 100);
	while(GetMessage(&msg, NULL, 0,0))
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
