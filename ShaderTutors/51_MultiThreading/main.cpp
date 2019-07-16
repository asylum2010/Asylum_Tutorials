
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "GdiPlus.lib")

#include <iostream>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "../Framework/win32window.h"
#include "../Framework/renderingcore.h"

extern void MainWindow_Created(Win32Window*);
extern void MainWindow_Closing(Win32Window*);
extern void MainWindow_KeyPress(Win32Window*, WPARAM);
extern void MainWindow_Render(Win32Window*, float, float);

extern void Window1_Created(Win32Window*);
extern void Window1_Closing(Win32Window*);
extern void Window1_Render(Win32Window*, float, float);

extern void Window2_Created(Win32Window*);
extern void Window2_Closing(Win32Window*);
extern void Window2_Render(Win32Window*, float, float);

// in mainwindow.cpp
extern void Window3_Created(Win32Window*);
extern void Window3_Closing(Win32Window*);
extern void Window3_Render(Win32Window*, float, float);

RECT			workarea;
LONG			wawidth;
LONG			waheight;
Win32Window*	window1 = nullptr;
Win32Window*	window2 = nullptr;
Win32Window*	window3 = nullptr;

void THREAD1_Run()
{
	window1 = new Win32Window(
		workarea.left + wawidth / 2, workarea.top,
		wawidth / 2, waheight / 2);

	window1->CreateCallback	= &Window1_Created;
	window1->CloseCallback	= &Window1_Closing;
	window1->RenderCallback	= &Window1_Render;

	window1->SetTitle("2D window with feedback (one thread)");
	window1->MessageHook();

	delete window1;
	window1 = 0;
}

void THREAD2_Run()
{
	window2 = new Win32Window(
		workarea.left, workarea.top + waheight / 2,
		wawidth / 2, waheight / 2);

	window2->CreateCallback	= &Window2_Created;
	window2->CloseCallback	= &Window2_Closing;
	window2->RenderCallback	= &Window2_Render;

	window2->SetTitle("2D window with feedback (two threads)");
	window2->MessageHook();

	delete window2;
	window2 = 0;
}

void THREAD3_Run()
{
	window3 = new Win32Window(
		workarea.left + wawidth / 2, workarea.top + waheight / 2,
		wawidth / 2, waheight / 2);

	window3->CreateCallback	= &Window3_Created;
	window3->CloseCallback	= &Window3_Closing;
	window3->RenderCallback	= &Window3_Render;

	window3->SetTitle("3D window with feedback");
	window3->MessageHook();

	delete window3;
	window3 = 0;
}

int main(int argc, char* argv[])
{
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workarea, 0);

	wawidth = workarea.right - workarea.left;
	waheight = workarea.bottom - workarea.top;

	// other windows
	std::thread worker1(&THREAD1_Run);
	std::thread worker2(&THREAD2_Run);
	std::thread worker3(&THREAD3_Run);

	// main window
	{
		Win32Window mainwindow(
			workarea.left, workarea.top,
			wawidth / 2, waheight / 2);

		mainwindow.CreateCallback	= &MainWindow_Created;
		mainwindow.CloseCallback	= &MainWindow_Closing;
		mainwindow.KeyUpCallback	= &MainWindow_KeyPress;
		mainwindow.RenderCallback	= &MainWindow_Render;

		mainwindow.SetTitle("3D window");
		mainwindow.MessageHook();
	}

	// close other windows
	if (window1 != nullptr)
		window1->Close();

	if (window2 != nullptr)
		window2->Close();

	if (window3 != nullptr)
		window3->Close();

	worker1.join();
	worker2.join();
	worker3.join();

	GetRenderingCore()->Shutdown();
	_CrtDumpMemoryLeaks();

#ifdef _DEBUG
	system("pause");
#endif

	return 0;
}
