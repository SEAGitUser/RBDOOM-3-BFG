/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 Robert Beckebans

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#pragma hdrstop
#include "precompiled.h"

#include "win_local.h"
#include "../../renderer/tr_local.h"

#include <windowsx.h>

LONG WINAPI MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

static bool s_alttab_disabled;

extern idCVar r_windowX;
extern idCVar r_windowY;
extern idCVar r_windowWidth;
extern idCVar r_windowHeight;

static void WIN_DisableAltTab()
{
	if( s_alttab_disabled || win32.win_allowAltTab.GetBool() )
	{
		return;
	}
	if( !idStr::Icmp( cvarSystem->GetCVarString( "sys_arch" ), "winnt" ) )
	{
		RegisterHotKey( 0, 0, MOD_ALT, VK_TAB );
	}
	else
	{
		BOOL old;
		
		// RB begin
#if defined(__MINGW32__)
		SystemParametersInfo( SPI_GETSCREENSAVEACTIVE, 1, &old, 0 );
#else
		SystemParametersInfo( SPI_SCREENSAVERRUNNING, 1, &old, 0 );
#endif
		// RB end
	}
	s_alttab_disabled = true;
}

static void WIN_EnableAltTab()
{
	if( !s_alttab_disabled || win32.win_allowAltTab.GetBool() )
	{
		return;
	}
	if( !idStr::Icmp( cvarSystem->GetCVarString( "sys_arch" ), "winnt" ) )
	{
		UnregisterHotKey( 0, 0 );
	}
	else
	{
		BOOL old;
		
		// RB begin
#if defined(__MINGW32__)
		SystemParametersInfo( SPI_GETSCREENSAVEACTIVE, 1, &old, 0 );
#else
		SystemParametersInfo( SPI_SCREENSAVERRUNNING, 1, &old, 0 );
#endif
		// RB end
	}
	
	s_alttab_disabled = false;
}

void WIN_Sizing( WORD side, RECT* rect )
{
	if( !renderSystem->IsRenderDeviceRunning() || renderSystem->GetWidth() <= 0 || renderSystem->GetHeight() <= 0 )
	{
		return;
	}
	
	// restrict to a standard aspect ratio
	int width = rect->right - rect->left;
	int height = rect->bottom - rect->top;
	
	// Adjust width/height for window decoration
	RECT decoRect = { 0, 0, 0, 0 };
	::AdjustWindowRect( &decoRect, WINDOW_STYLE | WS_SYSMENU, FALSE );
	int decoWidth = decoRect.right - decoRect.left;
	int decoHeight = decoRect.bottom - decoRect.top;
	
	width -= decoWidth;
	height -= decoHeight;
	
	// Clamp to a minimum size
	if( width < SCREEN_WIDTH / 4 ) width = SCREEN_WIDTH / 4;
	if( height < SCREEN_HEIGHT / 4 ) height = SCREEN_HEIGHT / 4;
	
	const int minWidth = height * 4 / 3;
	const int maxHeight = width * 3 / 4;
	
	const int maxWidth = height * 16 / 9;
	const int minHeight = width * 9 / 16;
	
	// Set the new size
	switch( side )
	{
		case WMSZ_LEFT:
			rect->left = rect->right - width - decoWidth;
			rect->bottom = rect->top + idMath::ClampInt( minHeight, maxHeight, height ) + decoHeight;
			break;
		case WMSZ_RIGHT:
			rect->right = rect->left + width + decoWidth;
			rect->bottom = rect->top + idMath::ClampInt( minHeight, maxHeight, height ) + decoHeight;
			break;
		case WMSZ_BOTTOM:
		case WMSZ_BOTTOMRIGHT:
			rect->bottom = rect->top + height + decoHeight;
			rect->right = rect->left + idMath::ClampInt( minWidth, maxWidth, width ) + decoWidth;
			break;
		case WMSZ_TOP:
		case WMSZ_TOPRIGHT:
			rect->top = rect->bottom - height - decoHeight;
			rect->right = rect->left + idMath::ClampInt( minWidth, maxWidth, width ) + decoWidth;
			break;
		case WMSZ_BOTTOMLEFT:
			rect->bottom = rect->top + height + decoHeight;
			rect->left = rect->right - idMath::ClampInt( minWidth, maxWidth, width ) - decoWidth;
			break;
		case WMSZ_TOPLEFT:
			rect->top = rect->bottom - height - decoHeight;
			rect->left = rect->right - idMath::ClampInt( minWidth, maxWidth, width ) - decoWidth;
			break;
	}
}

/*
====================
MainWndProc

main window procedure
====================
*/
LONG WINAPI MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	int key;
	switch( uMsg )
	{
		case WM_WINDOWPOSCHANGED:
			if( renderSystem->IsRenderDeviceRunning() )
			{
				RECT rect;
				if( ::GetClientRect( win32.hWnd, &rect ) )
				{			
					if( rect.right > rect.left && rect.bottom > rect.top )
					{
						glConfig.nativeScreenWidth = rect.right - rect.left;
						glConfig.nativeScreenHeight = rect.bottom - rect.top;
						
						// save the window size in cvars if we aren't fullscreen
						int style = ::GetWindowLong( hWnd, GWL_STYLE );
						if( ( style & WS_POPUP ) == 0 )
						{
							r_windowWidth.SetInteger( glConfig.nativeScreenWidth );
							r_windowHeight.SetInteger( glConfig.nativeScreenHeight );
						}
					}
				}
			}
			break;
		case WM_MOVE:
		{
			int	xPos, yPos;
			RECT r;
			
			// save the window origin in cvars if we aren't fullscreen
			int style = ::GetWindowLong( hWnd, GWL_STYLE );
			if( ( style & WS_POPUP ) == 0 )
			{
				xPos = ( short ) LOWORD( lParam ); // horizontal position
				yPos = ( short ) HIWORD( lParam ); // vertical position
				
				r.left   = 0;
				r.top    = 0;
				r.right  = 1;
				r.bottom = 1;
				
				::AdjustWindowRect( &r, style, FALSE );
				
				r_windowX.SetInteger( xPos + r.left );
				r_windowY.SetInteger( yPos + r.top );
			}
			break;
		}
		case WM_CREATE:
		
			win32.hWnd = hWnd;
			
			if( win32.cdsFullscreen )
			{
				WIN_DisableAltTab();
			}
			else
			{
				WIN_EnableAltTab();
			}
			
			// do the OpenGL setup
			void GLW_WM_CREATE( HWND hWnd );
			GLW_WM_CREATE( hWnd );
			
			break;
			
		case WM_DESTROY:
			// let sound and input know about this?
			win32.hWnd = NULL;
			if( win32.cdsFullscreen )
			{
				WIN_EnableAltTab();
			}
			break;
			
		case WM_CLOSE:
			soundSystem->SetMute( true );
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
			break;
			
		case WM_ACTIVATE:
			// if we got here because of an alt-tab or maximize,
			// we should activate immediately.  If we are here because
			// the mouse was clicked on a title bar or drag control,
			// don't activate until the mouse button is released
			{
				int	fActive, fMinimized;
			
				fActive = LOWORD( wParam );
				fMinimized = ( BOOL ) HIWORD( wParam );
			
				win32.activeApp = ( fActive != WA_INACTIVE );
				if( win32.activeApp )
				{
					idKeyInput::ClearStates();
					Sys_GrabMouseCursor( true );
					if( common->IsInitialized() )
					{
						SetCursor( NULL );
					}
				}
			
				if( fActive == WA_INACTIVE )
				{
					win32.movingWindow = false;
					if( common->IsInitialized() )
					{
						SetCursor( LoadCursor( 0, IDC_ARROW ) );
					}
				}
			
				// start playing the game sound world
				soundSystem->SetMute( !win32.activeApp );
				// DG: set com_pause so game pauses when focus is lost
				// and continues when focus is regained
				cvarSystem->SetCVarBool( "com_pause", !win32.activeApp );
				// DG end
			
				// we do not actually grab or release the mouse here,
				// that will be done next time through the main loop
			} break;
		case WM_TIMER:
		{
			if( win32.win_timerUpdate.GetBool() )
			{
				common->Frame();
			}
			break;
		}
		case WM_SYSCOMMAND:
			if( wParam == SC_SCREENSAVE || wParam == SC_KEYMENU )
			{
				return 0;
			}
			break;
			
		case WM_SYSKEYDOWN:
			if( wParam == 13 )  	// alt-enter toggles full-screen
			{
				cvarSystem->SetCVarBool( "r_fullscreen", !renderSystem->IsFullScreen() );
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "vid_restart\n" );
				return 0;
			}
		// fall through for other keys
		case WM_KEYDOWN:
			key = ( ( lParam >> 16 ) & 0xFF ) | ( ( ( lParam >> 24 ) & 1 ) << 7 );
			if( key == K_LCTRL || key == K_LALT || key == K_RCTRL || key == K_RALT )
			{
				// let direct-input handle this because windows sends Alt-Gr
				// as two events (ctrl then alt)
				break;
			}
			// D
			if( key == K_NUMLOCK )
			{
				key = K_PAUSE;
			}
			else if( key == K_PAUSE )
			{
				key = K_NUMLOCK;
			}
			sys->QueEvent( SE_KEY, key, true, 0, NULL, 0 );
			
			break;
			
		case WM_SYSKEYUP:
		case WM_KEYUP:
			key = ( ( lParam >> 16 ) & 0xFF ) | ( ( ( lParam >> 24 ) & 1 ) << 7 );
			if( key == K_PRINTSCREEN )
			{
				// don't queue printscreen keys.  Since windows doesn't send us key
				// down events for this, we handle queueing them with DirectInput
				break;
			}
			else if( key == K_LCTRL || key == K_LALT || key == K_RCTRL || key == K_RALT )
			{
				// let direct-input handle this because windows sends Alt-Gr
				// as two events (ctrl then alt)
				break;
			}
			sys->QueEvent( SE_KEY, key, false, 0, NULL, 0 );
			break;
			
		case WM_CHAR:
			// DG: make sure it's an utf-16 non-surrogate character (and thus a valid utf-32 character as well)
			// TODO: will there ever be two messages with surrogate characters that should be combined?
			//       (probably not, some people claim it's actually UCS-2, not UTF-16)
			if( wParam < 0xD800 || wParam > 0xDFFF )
			{
				sys->QueEvent( SE_CHAR, wParam, 0, 0, NULL, 0 );
			}
			break;
			
		// DG: support utf-32 input via WM_UNICHAR
		case WM_UNICHAR:
			sys->QueEvent( SE_CHAR, wParam, 0, 0, NULL, 0 );
			break;
		// DG end
		
		case WM_NCLBUTTONDOWN:
//			win32.movingWindow = true;
			break;
			
		case WM_ENTERSIZEMOVE:
			win32.movingWindow = true;
			break;
			
		case WM_EXITSIZEMOVE:
			win32.movingWindow = false;
			break;
			
		case WM_SIZING:
			WIN_Sizing( wParam, ( RECT* )lParam );
			break;
		case WM_MOUSEMOVE:
		{
			if( !common->IsInitialized() )
			{
				break;
			}
			
			const bool isShellActive = ( game && ( game->Shell_IsActive() || game->IsPDAOpen() ) );
			const bool isConsoleActive = console->Active();
			
			if( win32.activeApp )
			{
				if( isShellActive )
				{
					// If the shell is active, it will display its own cursor.
					SetCursor( NULL );
				}
				else if( isConsoleActive )
				{
					// The console is active but the shell is not.
					// Show the Windows cursor.
					SetCursor( LoadCursor( 0, IDC_ARROW ) );
				}
				else
				{
					// The shell not active and neither is the console.
					// This is normal gameplay, hide the cursor.
					SetCursor( NULL );
				}
			}
			else
			{
				if( !isShellActive )
				{
					// Always show the cursor when the window is in the background
					SetCursor( LoadCursor( 0, IDC_ARROW ) );
				}
				else
				{
					SetCursor( NULL );
				}
			}
			
			const int x = GET_X_LPARAM( lParam );
			const int y = GET_Y_LPARAM( lParam );
			
			// Generate an event
			sys->QueEvent( SE_MOUSE_ABSOLUTE, x, y, 0, NULL, 0 );
			
			// Get a mouse leave message
			TRACKMOUSEEVENT tme = {
				sizeof( TRACKMOUSEEVENT ),
				TME_LEAVE,
				hWnd,
				0
			};		
			TrackMouseEvent( &tme );
			
			return 0;
		}
		case WM_MOUSELEAVE:
		{
			sys->QueEvent( SE_MOUSE_LEAVE, 0, 0, 0, NULL, 0 );
			return 0;
		}
		case WM_LBUTTONDOWN:
		{
			sys->QueEvent( SE_KEY, K_MOUSE1, 1, 0, NULL, 0 );
			return 0;
		}
		case WM_LBUTTONUP:
		{
			sys->QueEvent( SE_KEY, K_MOUSE1, 0, 0, NULL, 0 );
			return 0;
		}
		case WM_RBUTTONDOWN:
		{
			sys->QueEvent( SE_KEY, K_MOUSE2, 1, 0, NULL, 0 );
			return 0;
		}
		case WM_RBUTTONUP:
		{
			sys->QueEvent( SE_KEY, K_MOUSE2, 0, 0, NULL, 0 );
			return 0;
		}
		case WM_MBUTTONDOWN:
		{
			sys->QueEvent( SE_KEY, K_MOUSE3, 1, 0, NULL, 0 );
			return 0;
		}
		case WM_MBUTTONUP:
		{
			sys->QueEvent( SE_KEY, K_MOUSE3, 0, 0, NULL, 0 );
			return 0;
		}
		case WM_XBUTTONDOWN:
		{
			// RB begin
		#if defined(__MINGW32__)
			sys->QueEvent( SE_KEY, K_MOUSE4, 1, 0, NULL, 0 );
		#else
			int button = GET_XBUTTON_WPARAM( wParam );
			if( button == 1 )
			{
				sys->QueEvent( SE_KEY, K_MOUSE4, 1, 0, NULL, 0 );
			}
			else if( button == 2 )
			{
				sys->QueEvent( SE_KEY, K_MOUSE5, 1, 0, NULL, 0 );
			}
		#endif
			// RB end
			return 0;
		}
		case WM_XBUTTONUP:
		{
			// RB begin
		#if defined(__MINGW32__)
			sys->QueEvent( SE_KEY, K_MOUSE4, 0, 0, NULL, 0 );
		#else
			int button = GET_XBUTTON_WPARAM( wParam );
			if( button == 1 )
			{
				sys->QueEvent( SE_KEY, K_MOUSE4, 0, 0, NULL, 0 );
			}
			else if( button == 2 )
			{
				sys->QueEvent( SE_KEY, K_MOUSE5, 0, 0, NULL, 0 );
			}
		#endif
			// RB end
			return 0;
		}
		case WM_MOUSEWHEEL:
		{
			int delta = GET_WHEEL_DELTA_WPARAM( wParam ) / WHEEL_DELTA;
			int key = ( delta < 0 )? K_MWHEELDOWN : K_MWHEELUP;
			delta = idMath::Abs( delta );
			while( delta-- > 0 )
			{
				sys->QueEvent( SE_KEY, key, true, 0, NULL, 0 );
				sys->QueEvent( SE_KEY, key, false, 0, NULL, 0 );
			}
			break;
		}
	}
	
	return ::DefWindowProc( hWnd, uMsg, wParam, lParam );
}





////////////////////////////////////////////////////////////////////////
// Displaying a Splash Screen
////////////////////////////////////////////////////////////////////////
#if 0
// Window Class name
const TCHAR * c_szSplashClass = TEXT( "SplashWindow" );
#define IDI_SPLASHICON 0

// Registers a window class for the splash and splash owner windows.
void RegisterWindowClass()
{
	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = ::DefWindowProc;
	wc.hInstance = win32.hInstance;
	wc.hIcon = ::LoadIcon( win32.hInstance, MAKEINTRESOURCE( IDI_SPLASHICON ) );
	wc.hCursor = ::LoadCursor( NULL, IDC_ARROW );
	wc.lpszClassName = c_szSplashClass;
	::RegisterClass( &wc );
}

// Creates the splash owner window and the splash window.
HWND CreateSplashWindow()
{
	HWND hwndOwner = ::CreateWindow( c_szSplashClass, NULL, WS_POPUP,
									 0, 0, 0, 0, NULL, NULL, win32.hInstance, NULL );
	return ::CreateWindowEx( WS_EX_LAYERED, c_szSplashClass, NULL, WS_POPUP | WS_VISIBLE,
							 0, 0, 0, 0, hwndOwner, NULL, win32.hInstance, NULL );
}

// Calls UpdateLayeredWindow to set a bitmap (with alpha) as the content of the splash window.
void SetSplashImage( HWND hwndSplash, HBITMAP hbmpSplash )
{
	// get the size of the bitmap
	BITMAP bm;
	::GetObject( hbmpSplash, sizeof( bm ), &bm );
	SIZE sizeSplash = { bm.bmWidth, bm.bmHeight };

	// get the primary monitor's info
	POINT ptZero = { 0 };
	HMONITOR hmonPrimary = ::MonitorFromPoint( ptZero, MONITOR_DEFAULTTOPRIMARY );
	MONITORINFO monitorinfo = { 0 };
	monitorinfo.cbSize = sizeof( monitorinfo );
	::GetMonitorInfo( hmonPrimary, &monitorinfo );

	// center the splash screen in the middle of the primary work area
	const RECT & rcWork = monitorinfo.rcWork;
	POINT ptOrigin = {
		rcWork.left + ( rcWork.right - rcWork.left - sizeSplash.cx ) / 2,
		rcWork.top + ( rcWork.bottom - rcWork.top - sizeSplash.cy ) / 2
	};

	// create a memory DC holding the splash bitmap
	HDC hdcScreen = ::GetDC( NULL );
	HDC hdcMem = ::CreateCompatibleDC( hdcScreen );
	HBITMAP hbmpOld = ( HBITMAP )::SelectObject( hdcMem, hbmpSplash );

	// use the source image's alpha channel for blending
	BLENDFUNCTION blend = { 0 };
	blend.BlendOp = AC_SRC_OVER;
	blend.SourceConstantAlpha = 255;
	blend.AlphaFormat = AC_SRC_ALPHA;

	// paint the window (in the right location) with the alpha-blended bitmap
	::UpdateLayeredWindow( hwndSplash, hdcScreen, &ptOrigin, &sizeSplash,
						 hdcMem, &ptZero, RGB( 0, 0, 0 ), &blend, ULW_ALPHA );

	// delete temporary objects
	::SelectObject( hdcMem, hbmpOld );
	::DeleteDC( hdcMem );
	::ReleaseDC( NULL, hdcScreen );

	/////////////////////////////

	// create the named close splash screen event, making sure we're the first process to create it
	SetLastError( ERROR_SUCCESS );
	HANDLE hCloseSplashEvent = CreateEvent( NULL, TRUE, FALSE, TEXT( "CloseSplashScreenEvent" ) );
	if( GetLastError() == ERROR_ALREADY_EXISTS )
		ExitProcess( 0 );
}

void Test()
{
	HANDLE hCloseSplashEvent;

	// call LoadSplashImage() etc.
	// call SetSplashImage() etc.

	// launch the WPF application
	HANDLE hProcess ;//= LaunchWpfApplication();
	AllowSetForegroundWindow( GetProcessId( hProcess ) );

	// display the splash screen for as long as it's needed
	HANDLE aHandles[ 2 ] = { hProcess, hCloseSplashEvent };
	PumpMsgWaitForMultipleObjects( 2, &aHandles[ 0 ], INFINITE );
}

/* 
The PumpMsgWaitForMultipleObjects method has not yet been defined. It�s similar (in API) to the Win32 WaitForMultipleObjects function, 
but it also dispatches window messages as they arrive. Since we have created a window on this thread, running a message pump is essential. 
We use MsgWaitForMultipleObjects to wait for either of two HANDLEs while also being woken up when a window message arrives. 
(Note that this implementation is more generic than is necessary in this example: the timeout is always INFINITE, 
and there�s no outer message loop that would need to reprocess the WM_QUIT message.)
*/
ID_INLINE DWORD PumpMsgWaitForMultipleObjects( DWORD nCount, LPHANDLE pHandles, DWORD dwMilliseconds )
{
	// useful variables
	const DWORD dwStartTickCount = ::GetTickCount();
	for(;;) // loop until done
	{
		// calculate timeout
		const DWORD dwElapsed = ::GetTickCount() - dwStartTickCount;
		const DWORD dwTimeout = dwMilliseconds == INFINITE ? INFINITE : dwElapsed < dwMilliseconds ? dwMilliseconds - dwElapsed : 0;

		// wait for a handle to be signaled or a message
		const DWORD dwWaitResult = ::MsgWaitForMultipleObjects( nCount, pHandles, FALSE, dwTimeout, QS_ALLINPUT );
		if( dwWaitResult == WAIT_OBJECT_0 + nCount )
		{
			// pump messages
			MSG msg;
			while( ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) != FALSE )
			{
				// check for WM_QUIT
				if( msg.message == WM_QUIT )
				{
					// repost quit message and return
					::PostQuitMessage( ( int ) msg.wParam );
					return WAIT_OBJECT_0 + nCount;
				}

				// dispatch thread message
				::TranslateMessage( &msg );
				::DispatchMessage( &msg );
			}
		}
		else {
			// timeout on actual wait or any other object
			return dwWaitResult;
		}
	}
}
#endif
