#include "dxstuff.h"

static bool errExit = false;
static LPCSTR className = "Name";
static WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, 0, 0L, 0L, 
		0, NULL, NULL, NULL, NULL,
		className, NULL };
	
static char windowTitle[]="tengine";
static POINT wsize={1280, 960};
//static POINT wsize={1024, 768};
RECT sr={0, 0, wsize.x, wsize.y};
POINT dp={0, 0};
				
HWND hwnd;
HINSTANCE hinst;

static BOOL active=FALSE;

void initApp();
void preInit();
bool main();
void closeApp();
void catchAction(const string &s);

LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_DESTROY:
            PostQuitMessage( 0 );
            return 0;
		case WM_ACTIVATE:	
			if (WA_INACTIVE!=wParam)
			{
				if (pdiKeyb)
					pdiKeyb->Acquire();
				if (pdiMouse)
					pdiMouse->Acquire();
				for (UINT i=0;i<numJoys;i++)
					joy[i].device->Acquire();
				active=TRUE;
			}
			else
				active=FALSE;

	}

    return DefWindowProc( hWnd, msg, wParam, lParam );
}

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
    //hinst=GetModuleHandle(NULL);
	hinst = hInst;
	
	// Register the window class.
	wc.lpfnWndProc = MsgProc;
	wc.hInstance = hInst;
	if (!RegisterClassEx( &wc ))
		return 0;

    // Create the application's window.
		hwnd = CreateWindow( className, windowTitle, 
		WS_POPUP, 0, 0, wsize.x, wsize.y,
		GetDesktopWindow(), NULL, wc.hInstance, NULL );
	try
	{ 
		/*if (!SetPriorityClass(hinst, HIGH_PRIORITY_CLASS))
			d_file<<"Couldn't set priority.\n";
		DWORD err=GetLastError();
		d_file<<err<<endl;*/
				
		ShowWindow( hwnd, SW_SHOWDEFAULT );
		UpdateWindow( hwnd );
		int stgdr=ShowCursor(FALSE);				
		initApp();
		// The message loop.
		MSG msg; 
		msg.message = 0;
				
		while( msg.message!=WM_QUIT )
		{
			if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
			else if (active)
			{
				updateDevices();
				if (keypress(DIK_ESCAPE))
					PostQuitMessage(0);
				main();
				if (D3DERR_DEVICELOST == pd3dDevice->Present(0,0,0,0))
					PostQuitMessage(0);
				frames++;
				//ShowCursor(1);
				//MessageBox(hwnd, "Click Ok to exit.", 0, MB_OK);
			}
		}
		int dummy = ShowCursor(true);
	}
	catch (string s){
		catchAction(s);
	}
	catch (char s[]){
		catchAction(s);
	}
	closeApp();
	UnregisterClass( className, wc.hInstance );
	
    return 0;
}

void catchAction(const string &s)
{
	toggleFullscreen(false);
	int dummy = ShowCursor(true);
	//static char s2[200];
	//strcpy(s2, s.c_str());
	string s2 = s + g_errMsg;
	//if (FAILED(hr=DXTRACE_ERR(s2, hr)))
	//	d_file << "DXTrace failed:\n"<<DXGetErrorString9(hr);
	
	//const char *errString = DXGetErrorString(hr);
	MessageBox(hwnd, s2.c_str(), NULL, MB_OK|MB_ICONWARNING);
	
//	errExit = true;

}
