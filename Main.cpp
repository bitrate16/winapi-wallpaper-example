
#include "stdafx.h"

#include <windows.h>
#include <WinUser.h>
#include <tchar.h>
#include <math.h>
#include <iostream>
#include <cstdio>
#include <string>
#include <iomanip>
#include <sstream>
#include <io.h>
#include <fcntl.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>

#define DISPLAY_CONSOLE_WINDOW

// #define PRINT_WINDOWS_ENUM
#include "WorkerWEnumerator.h"

// Ling OpenGL
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

// For freopen
#pragma warning(disable : 4996)

using WorkerWEnumerator::enumerateForWorkerW;

HDC hDC;
HPALETTE hPalette = 0;
GLboolean animate = GL_TRUE;

// initialize renderring on OpenGL context
void initSC() {
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0, 0, 0, 0);
}

// Callback for resize event for OpenGL
void resizeSC(int width, int height) {
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

// Callback for rendering a surface
void renderSC() {
	/* rotate a triangle around */
	glClear(GL_COLOR_BUFFER_BIT);
	if (animate)
		glRotatef(1.0f, 0.0f, 0.0f, 1.0f);
	glBegin(GL_TRIANGLES);
	glIndexi(1);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex2i(0, 1);
	glIndexi(2);
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex2i(-1, -1);
	glIndexi(3);
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex2i(1, -1);
	glEnd();
	glFlush();
	SwapBuffers(hDC);			/* nop if singlebuffered */
}

// Event dispatcher
LONG WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static PAINTSTRUCT ps;

	switch (uMsg) {
		case WM_PAINT:
			BeginPaint(hWnd, &ps);
			renderSC();
			EndPaint(hWnd, &ps);
			return 0;

		case WM_SIZE:
			resizeSC(LOWORD(lParam), HIWORD(lParam));
			PostMessage(hWnd, WM_PAINT, 0, 0);
			return 0;

		case WM_CHAR:
			switch (wParam) {
				case 27:			/* ESC key */
					PostQuitMessage(0);
					break;
			}
			return 0;

		case WM_CLOSE:
			PostQuitMessage(0);
			return 0;

		case WM_PALETTECHANGED:
			if (hWnd == (HWND)wParam)
				break;

		case WM_QUERYNEWPALETTE:
			if (hPalette) {
				UnrealizeObject(hPalette);
				SelectPalette(hDC, hPalette, FALSE);
				RealizePalette(hDC);
				return TRUE;
			}
			return FALSE;

		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// Register OpenGL context for window
HWND CreateOpenGLWindow(LPWSTR title, int x, int y, int width, int height, BYTE type, DWORD flags) {
	int         n, pf;
	HWND        hWnd;
	WNDCLASS    wc;
	LOGPALETTE* lpPal;
	PIXELFORMATDESCRIPTOR pfd;
	static HINSTANCE hInstance = 0;

	/* only register the window class once - use hInstance as a flag. */
	if (!hInstance) {
		hInstance = GetModuleHandle(NULL);
		wc.style = CS_OWNDC;
		wc.lpfnWndProc = (WNDPROC)WindowProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance;
		wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = NULL;
		wc.lpszMenuName = NULL;
		wc.lpszClassName = L"OpenGL";

		if (!RegisterClass(&wc)) {
			MessageBox(NULL, L"RegisterClass() failed:  "
				"Cannot register window class.", L"Error", MB_OK);
			return NULL;
		}
	}

	hWnd = CreateWindow(L"OpenGL", title, WS_OVERLAPPEDWINDOW |
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		x, y, width, height, NULL, NULL, hInstance, NULL);

	if (hWnd == NULL) {
		MessageBox(NULL, L"CreateWindow() failed:  Cannot create a window.",
			L"Error", MB_OK);
		return NULL;
	}

	DWORD style = ::GetWindowLong(hWnd, GWL_STYLE);
	style &= ~WS_OVERLAPPEDWINDOW;
	style |= WS_POPUP;
	::SetWindowLong(hWnd, GWL_STYLE, style);

	hDC = GetDC(hWnd);

	memset(&pfd, 0, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | flags;
	pfd.iPixelType = type;
	pfd.cColorBits = 32;

	pf = ChoosePixelFormat(hDC, &pfd);
	if (pf == 0) {
		MessageBox(NULL, L"ChoosePixelFormat() failed:  "
			"Cannot find a suitable pixel format.", L"Error", MB_OK);
		return 0;
	}

	if (SetPixelFormat(hDC, pf, &pfd) == FALSE) {
		MessageBox(NULL, L"SetPixelFormat() failed:  "
			"Cannot set format specified.", L"Error", MB_OK);
		return 0;
	}

	DescribePixelFormat(hDC, pf, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

	if (pfd.dwFlags & PFD_NEED_PALETTE ||
		pfd.iPixelType == PFD_TYPE_COLORINDEX) {

		n = 1 << pfd.cColorBits;
		if (n > 256) n = 256;

		lpPal = (LOGPALETTE*)malloc(sizeof(LOGPALETTE) +
			sizeof(PALETTEENTRY) * n);
		memset(lpPal, 0, sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * n);
		lpPal->palVersion = 0x300;
		lpPal->palNumEntries = n;

		GetSystemPaletteEntries(hDC, 0, n, &lpPal->palPalEntry[0]);

		if (pfd.iPixelType == PFD_TYPE_RGBA) {
			int redMask = (1 << pfd.cRedBits) - 1;
			int greenMask = (1 << pfd.cGreenBits) - 1;
			int blueMask = (1 << pfd.cBlueBits) - 1;
			int i;

			for (i = 0; i < n; ++i) {
				lpPal->palPalEntry[i].peRed =
					(((i >> pfd.cRedShift)   & redMask) * 255) / redMask;
				lpPal->palPalEntry[i].peGreen =
					(((i >> pfd.cGreenShift) & greenMask) * 255) / greenMask;
				lpPal->palPalEntry[i].peBlue =
					(((i >> pfd.cBlueShift)  & blueMask) * 255) / blueMask;
				lpPal->palPalEntry[i].peFlags = 0;
			}
		}
		else {
			lpPal->palPalEntry[0].peRed = 0;
			lpPal->palPalEntry[0].peGreen = 0;
			lpPal->palPalEntry[0].peBlue = 0;
			lpPal->palPalEntry[0].peFlags = PC_NOCOLLAPSE;
			lpPal->palPalEntry[1].peRed = 255;
			lpPal->palPalEntry[1].peGreen = 0;
			lpPal->palPalEntry[1].peBlue = 0;
			lpPal->palPalEntry[1].peFlags = PC_NOCOLLAPSE;
			lpPal->palPalEntry[2].peRed = 0;
			lpPal->palPalEntry[2].peGreen = 255;
			lpPal->palPalEntry[2].peBlue = 0;
			lpPal->palPalEntry[2].peFlags = PC_NOCOLLAPSE;
			lpPal->palPalEntry[3].peRed = 0;
			lpPal->palPalEntry[3].peGreen = 0;
			lpPal->palPalEntry[3].peBlue = 255;
			lpPal->palPalEntry[3].peFlags = PC_NOCOLLAPSE;
		}

		hPalette = CreatePalette(lpPal);
		if (hPalette) {
			SelectPalette(hDC, hPalette, FALSE);
			RealizePalette(hDC);
		}

		free(lpPal);
	}

	ReleaseDC(hWnd, hDC);

	return hWnd;
};

// Entry
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow) {
#ifdef DISPLAY_CONSOLE_WINDOW
	// Open console window
	AllocConsole();
	freopen("CONOUT$", "w+", stdout);
#endif

	// Output unicode text
	_setmode(_fileno(stdout), _O_U16TEXT);

	// Get WorkerW layer
	HWND workerw = enumerateForWorkerW();

	if (workerw == NULL) {
		std::cerr << "WorkerW enumeration fail" << std::endl;
		exit(1);
	}

	HDC hDC;
	HGLRC hRC;
	HWND  hWnd;
	MSG   msg;

	// Get size of WorkerW. Entire desktop background
	RECT windowsize;
	GetWindowRect(workerw, &windowsize);
	hWnd = CreateOpenGLWindow(L"minimal", windowsize.left, windowsize.top, windowsize.right, windowsize.bottom, PFD_TYPE_RGBA, 0);
	if (hWnd == NULL)
		exit(1);

	hDC = GetDC(hWnd);
	hRC = wglCreateContext(hDC);
	wglMakeCurrent(hDC, hRC);

	SetParent(hWnd, workerw);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	while (1) {
		while (PeekMessage(&msg, hWnd, 0, 0, PM_NOREMOVE)) {
			if (GetMessage(&msg, hWnd, 0, 0)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else {
				goto quit;
			}
		}
		renderSC();
	}

quit:

	wglMakeCurrent(NULL, NULL);
	ReleaseDC(hWnd, hDC);
	wglDeleteContext(hRC);
	DestroyWindow(hWnd);
	if (hPalette)
		DeleteObject(hPalette);

	return msg.wParam;
};
