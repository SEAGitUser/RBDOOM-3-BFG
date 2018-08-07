/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2014 Robert Beckebans

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
/*
** WIN_GLIMP.C
**
** This file contains ALL Win32 specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_SwapBuffers
** GLimp_Init
** GLimp_Shutdown
** GLimp_SetGamma
**
** Note that the GLW_xxx functions are Windows specific GL-subsystem
** related functions that are relevant ONLY to win_glimp.c
*/
#pragma hdrstop
#include "precompiled.h"

#include "win_local.h"
#include "rc/doom_resource.h"
#include "../../renderer/tr_local.h"

void GLimp_SaveGamma();
void GLimp_RestoreGamma();
bool GLW_GetWindowDimensions( const glimpParms_t &, int& x, int& y, int& w, int& h );
bool GLW_ChangeDislaySettingsIfNeeded( const glimpParms_t & );
idStr GetDeviceName( const int deviceNum );

idCVar r_useOpenGL32( "r_useOpenGL32", "2", CVAR_INTEGER, "0 = OpenGL 2.0, 1 = OpenGL 3.2 compatibility profile, 2 = OpenGL 3.2 core profile", 0, 2 );

idStr GetErrorString( DWORD errorCode )
{
	TCHAR errBuff[ 256 ];
	::FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, errorCode, 0, errBuff, sizeof( errBuff ), NULL );
	return idStr( errBuff );
}

/*
========================
GLimp_TestSwapBuffers
========================
*/
void GLimp_TestSwapBuffers( const idCmdArgs& args )
{
	idLib::Printf( "GLimp_TimeSwapBuffers\n" );
	static const int MAX_FRAMES = 5;
	uint64	timestamps[MAX_FRAMES];
	glDisable( GL_SCISSOR_TEST );

	int frameMilliseconds = 16;
	for( int swapInterval = 2 ; swapInterval >= -1 ; swapInterval-- )
	{
		wglSwapIntervalEXT( swapInterval );
		for( int i = 0 ; i < MAX_FRAMES ; i++ )
		{
			if( swapInterval == -1 )
			{
				sys->Sleep( frameMilliseconds );
			}
			if( i & 1 )
			{
				glClearColor( 0, 1, 0, 1 );
			}
			else
			{
				glClearColor( 1, 0, 0, 1 );
			}
			glClear( GL_COLOR_BUFFER_BIT );
			SwapBuffers( win32.hDC );
			glFinish();
			timestamps[i] = sys->Microseconds();
		}

		idLib::Printf( "\nswapinterval %i\n", swapInterval );
		for( int i = 1 ; i < MAX_FRAMES ; i++ )
		{
			idLib::Printf( "%i microseconds\n", ( int )( timestamps[i] - timestamps[i - 1] ) );
		}
	}
}

/*
=============================================================================

WglExtension Grabbing

This is gross -- creating a window just to get a context to get the wgl extensions

=============================================================================
*/

/*
====================
FakeWndProc

Only used to get wglExtensions
====================
*/
LONG WINAPI FakeWndProc(
	HWND    hWnd,
	UINT    uMsg,
	WPARAM  wParam,
	LPARAM  lParam )
{

	if( uMsg == WM_DESTROY )
	{
		PostQuitMessage( 0 );
	}

	if( uMsg != WM_CREATE )
	{
		return DefWindowProc( hWnd, uMsg, wParam, lParam );
	}

	const static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof( PIXELFORMATDESCRIPTOR ),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		24,
		0, 0, 0, 0, 0, 0,
		8, 0,
		0, 0, 0, 0,
		24, 8,
		0,
		PFD_MAIN_PLANE,
		0,
		0,
		0,
		0,
	};
	int		pixelFormat;
	HDC hDC;
	HGLRC hGLRC;

	hDC = GetDC( hWnd );

	// Set up OpenGL
	pixelFormat = ChoosePixelFormat( hDC, &pfd );
	SetPixelFormat( hDC, pixelFormat, &pfd );
	hGLRC = wglCreateContext( hDC );
	wglMakeCurrent( hDC, hGLRC );

	// free things
	wglMakeCurrent( NULL, NULL );
	wglDeleteContext( hGLRC );
	ReleaseDC( hWnd, hDC );

	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}


/*
==================
GLW_GetWGLExtensionsWithFakeWindow
==================
*/
// RB: replaced WGL with GLEW WGL
void GLW_CheckWGLExtensions( HDC hDC )
{
	GLenum glewResult = glewInit();
	if( GLEW_OK != glewResult )
	{
		// glewInit failed, something is seriously wrong
		common->Printf( "^GLW_CheckWGLExtensions() - GLEW could not load OpenGL subsystem: %s", glewGetErrorString( glewResult ) );
	}
	else
	{
		common->Printf( "Using GLEW %s\n", glewGetString( GLEW_VERSION ) );
	}

	if( WGLEW_ARB_extensions_string )
	{
		glConfig.wgl_extensions_string = ( const char* ) wglGetExtensionsStringARB( hDC );
	}
	else
	{
		glConfig.wgl_extensions_string = "";
	}

	// WGL_EXT_swap_control
	//wglSwapIntervalEXT = ( PFNWGLSWAPINTERVALEXTPROC ) GLimp_ExtensionPointer( "wglSwapIntervalEXT" );
	r_swapInterval.SetModified();	// force a set next frame

	// WGL_EXT_swap_control_tear
	glConfig.swapControlTearAvailable = WGLEW_EXT_swap_control_tear != 0;

	// WGL_ARB_pixel_format
	//wglGetPixelFormatAttribivARB = ( PFNWGLGETPIXELFORMATATTRIBIVARBPROC )GLimp_ExtensionPointer( "wglGetPixelFormatAttribivARB" );
	//wglGetPixelFormatAttribfvARB = ( PFNWGLGETPIXELFORMATATTRIBFVARBPROC )GLimp_ExtensionPointer( "wglGetPixelFormatAttribfvARB" );
	//wglChoosePixelFormatARB = ( PFNWGLCHOOSEPIXELFORMATARBPROC )GLimp_ExtensionPointer( "wglChoosePixelFormatARB" );

	// wglCreateContextAttribsARB
	//wglCreateContextAttribsARB = ( PFNWGLCREATECONTEXTATTRIBSARBPROC )wglGetProcAddress( "wglCreateContextAttribsARB" );
}
// RB end

/*
==================
GLW_GetWGLExtensionsWithFakeWindow
==================
*/
static void GLW_GetWGLExtensionsWithFakeWindow()
{
	// Create a window for the sole purpose of getting
	// a valid context to get the wglextensions
	HWND hWnd = ::CreateWindow( WIN32_FAKE_WINDOW_CLASS_NAME, GAME_NAME,
								WS_OVERLAPPEDWINDOW,
								40, 40, 40, 40,
								NULL, NULL, win32.hInstance, NULL );
	if( !hWnd ) {
		common->FatalError( "GLW_GetWGLExtensionsWithFakeWindow: Couldn't create fake window" );
	}

	HDC hDC = ::GetDC( hWnd );
	HGLRC gRC = wglCreateContext( hDC );
	wglMakeCurrent( hDC, gRC );
	GLW_CheckWGLExtensions( hDC );
	wglDeleteContext( gRC );
	::ReleaseDC( hWnd, hDC );

	::DestroyWindow( hWnd );

	MSG	msg;
	while( ::GetMessage( &msg, NULL, 0, 0 ) ) {
		::TranslateMessage( &msg );
		::DispatchMessage( &msg );
	}
}

//=============================================================================

/*
====================
GLW_WM_CREATE
====================
*/
void GLW_WM_CREATE( HWND hWnd )
{
}

/*
========================
CreateOpenGLContextOnDC
	WGL_ARB_create_context_no_error
	WGL_ARB_create_context_profile
	WGL_ARB_create_context_robustness
========================
*/
static HGLRC CreateOpenGLContextOnDC( const HDC hdc, const bool debugContext )
{
	int useOpenGL32 = r_useOpenGL32.GetInteger();
	HGLRC m_hrc = NULL;

	// RB: for GLintercept 1.2.0 or otherwise we can't diff the framebuffers using the XML log
	if( !WGLEW_ARB_create_context || useOpenGL32 == 0 )
	{
		return wglCreateContext( hdc );
	}
	// RB end

	for( int i = 0; i < 2; i++ )
	{
		const int glMajorVersion = ( useOpenGL32 != 0 ) ? 3 : 2;
		const int glMinorVersion = ( useOpenGL32 != 0 ) ? 2 : 0;
		const int glDebugFlag = debugContext ? WGL_CONTEXT_DEBUG_BIT_ARB : 0;
		const int glProfileMask = ( useOpenGL32 != 0 ) ? WGL_CONTEXT_PROFILE_MASK_ARB : 0;
		const int glProfile = ( useOpenGL32 == 1 ) ? WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB : ( ( useOpenGL32 == 2 ) ? WGL_CONTEXT_CORE_PROFILE_BIT_ARB : 0 );
		const int attribs[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB,	glMajorVersion,
			WGL_CONTEXT_MINOR_VERSION_ARB,	glMinorVersion,
			WGL_CONTEXT_FLAGS_ARB,			glDebugFlag | ( ( useOpenGL32 == 1 ) ? 0 : WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB ),
			glProfileMask,					glProfile,
			0
		};

		m_hrc = wglCreateContextAttribsARB( hdc, 0, attribs );
		if( m_hrc != NULL )
		{
			idLib::Printf( "created OpenGL %d.%d context\n", glMajorVersion, glMinorVersion );

			if( useOpenGL32 == 2 )
			{
				glConfig.driverType = GLDRV_OPENGL32_CORE_PROFILE;
			}
			else if( useOpenGL32 == 1 )
			{
				glConfig.driverType = GLDRV_OPENGL32_COMPATIBILITY_PROFILE;
			}
			else
			{
				glConfig.driverType = GLDRV_OPENGL3X;
			}

			break;
		}

		idLib::Printf( "failed to create OpenGL %d.%d context\n", glMajorVersion, glMinorVersion );
		useOpenGL32 = 0;	// fall back to OpenGL 2.0
	}

	if( m_hrc == NULL )
	{
		int	err = GetLastError();
		switch( err )
		{
			case ERROR_INVALID_VERSION_ARB:
				idLib::Printf( "ERROR_INVALID_VERSION_ARB\n" );
				break;
			case ERROR_INVALID_PROFILE_ARB:
				idLib::Printf( "ERROR_INVALID_PROFILE_ARB\n" );
				break;
			default:
				idLib::Printf( "unknown error: 0x%x\n", err );
				break;
		}
	}

	return m_hrc;
}

/*
====================
GLW_InitDriver

Set the pixelformat for the window before it is
shown, and create the rendering context
====================
*/
static bool GLW_InitDriver( glimpParms_t parms )
{
	common->Printf( "Initializing OpenGL driver\n" );
	//
	// get a DC for our window if we don't already have one allocated
	//
	if( win32.hDC == NULL )
	{
		common->Printf( "...getting DC: " );

		if( ( win32.hDC = ::GetDC( win32.hWnd ) ) == NULL )
		{
			common->Printf( S_COLOR_YELLOW "failed" S_COLOR_DEFAULT "\n" );
			return false;
		}
		common->Printf( "succeeded\n" );
	}

	if( WGLEW_ARB_pixel_format )
	{
		int	iAttributes[] =
		{
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB,	GL_TRUE,
			WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
			WGL_SWAP_METHOD_ARB,	WGL_SWAP_EXCHANGE_ARB,
			WGL_RED_BITS_ARB,		8,
			WGL_GREEN_BITS_ARB,		8,
			WGL_BLUE_BITS_ARB,		8,
			WGL_ALPHA_BITS_ARB,		8,
			WGL_STENCIL_BITS_ARB,	0,
			WGL_DEPTH_BITS_ARB,		0,
			WGL_STEREO_ARB,			GL_FALSE,
			WGL_SAMPLE_BUFFERS_ARB, GL_FALSE,
			//WGL_SAMPLES_ARB,		1,
			//WGL_ACCUM_BITS_ARB,		0,
			//WGL_AUX_BUFFERS_ARB,	0,
			0,						0
		};
		// Let's check how many formats are supporting our requirements
		int  formats[ 512 ];
		UINT numFormats;
		if( !wglChoosePixelFormatARB( win32.hDC, iAttributes, NULL, _countof( formats ), formats, &numFormats ) )
		{
			common->Printf( "..." S_COLOR_YELLOW "wglChoosePixelFormatARB() failed: %s" S_COLOR_DEFAULT "\n", GetErrorString( ::GetLastError() ).c_str() );
			return false;
		}
		if( !numFormats )
		{
			common->Printf( "..." S_COLOR_YELLOW "wglChoosePixelFormatARB return 0 format count" S_COLOR_DEFAULT "\n" );
			return false;
		}

		win32.pixelformat = -1;

		// Extract the components of the current format
		int outAttributes[] =
		{
			WGL_DRAW_TO_WINDOW_ARB, WGL_SUPPORT_OPENGL_ARB, WGL_DOUBLE_BUFFER_ARB, WGL_PIXEL_TYPE_ARB, WGL_SWAP_METHOD_ARB,
			WGL_RED_BITS_ARB, WGL_GREEN_BITS_ARB, WGL_BLUE_BITS_ARB, WGL_ALPHA_BITS_ARB,
			WGL_STENCIL_BITS_ARB, WGL_DEPTH_BITS_ARB,
			WGL_STEREO_ARB, WGL_SAMPLE_BUFFERS_ARB,
			//WGL_SAMPLES_ARB,
			//WGL_ACCUM_BITS_ARB,
			//WGL_AUX_BUFFERS_ARB,
		};
		int outValues[ _countof( outAttributes ) ] = {};
		for( UINT i = 0; i < numFormats; ++i )
		{
			if( !wglGetPixelFormatAttribivARB( win32.hDC, formats[ i ], PFD_MAIN_PLANE, _countof( outAttributes ), outAttributes, outValues ) )
			{
				common->Printf( "..." S_COLOR_YELLOW "Failed to retrieve pixel format( %i ) info: %s" S_COLOR_DEFAULT "\n",
								formats[ i ], GetErrorString( ::GetLastError() ).c_str() );
				return false;
			}

			int res = 0;
			res |= ( outValues[  0 ] != GL_TRUE );				// WGL_DRAW_TO_WINDOW_ARB
			res |= ( outValues[  1 ] != GL_TRUE );				// WGL_SUPPORT_OPENGL_ARB
			res |= ( outValues[  2 ] != GL_TRUE );				// WGL_DOUBLE_BUFFER_ARB
			res |= ( outValues[  3 ] != WGL_TYPE_RGBA_ARB );	// WGL_PIXEL_TYPE_ARB
			res |= ( outValues[  4 ] != WGL_SWAP_EXCHANGE_ARB );// WGL_SWAP_METHOD_ARB
			res |= ( outValues[  5 ] != 8 );					// WGL_RED_BITS_ARB
			res |= ( outValues[  6 ] != 8 );					// WGL_GREEN_BITS_ARB
			res |= ( outValues[  7 ] != 8 );					// WGL_BLUE_BITS_ARB
			res |= ( outValues[  8 ] != 8 );					// WGL_ALPHA_BITS_ARB
			res |= ( outValues[  9 ] != 0 );					// WGL_STENCIL_BITS_ARB
			res |= ( outValues[ 10 ] != 0 );					// WGL_DEPTH_BITS_ARB
			res |= ( outValues[ 11 ] != GL_FALSE );				// WGL_STEREO_ARB
			res |= ( outValues[ 12 ] != GL_FALSE );				// WGL_SAMPLE_BUFFERS_ARB
			//res |= ( outValues[ 13 ] != 1 );					// WGL_SAMPLES_ARB
			//res |= ( outValues[ 13 ] != 0 );					// WGL_ACCUM_BITS_ARB
			//res |= ( outValues[ 14 ] != 0 );					// WGL_AUX_BUFFERS_ARB
			if( res )
			{
				common->DPrintf( "...Skipping format %i", formats[ i ] );
				continue;
			}

			win32.pixelformat = formats[ i ];
			common->DPrintf( "...PIXELFORMAT %i selected\n", win32.pixelformat );
			common->DPrintf( ".....WGL_RED_BITS_ARB %i\n", outValues[ 5 ] );
			common->DPrintf( ".....WGL_GREEN_BITS_ARB %i\n", outValues[ 6 ] );
			common->DPrintf( ".....WGL_BLUE_BITS_ARB %i\n", outValues[ 7 ] );
			common->DPrintf( ".....WGL_ALPHA_BITS_ARB %i\n", outValues[ 8 ] );
			common->DPrintf( ".....WGL_STENCIL_BITS_ARB %i\n", outValues[ 9 ] );
			common->DPrintf( ".....WGL_DEPTH_BITS_ARB %i\n", outValues[ 10 ] );
			break;
		}

		if( win32.pixelformat == -1 )
		{
			common->Printf( "..." S_COLOR_YELLOW "Failed to retrieve pixel format: win32.pixelformat == -1" S_COLOR_DEFAULT "\n" );
			return false;
		}
	}
	else {
		// Setup a standard pixel format descriptor
		PIXELFORMATDESCRIPTOR desc = { 0 };
		desc.nSize = sizeof( desc );
		desc.nVersion = 1;
		desc.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		desc.iPixelType = PFD_TYPE_RGBA;
		desc.cColorBits = 32;
		desc.cAlphaBits = 8;

		if( ( win32.pixelformat = ::ChoosePixelFormat( win32.hDC, &desc ) ) == 0 )
		{
			common->Printf( "..." S_COLOR_YELLOW "ChoosePixelFormat failed" S_COLOR_DEFAULT "\n" );
			return false;
		}
		common->Printf( "...PIXELFORMAT %i selected\n", win32.pixelformat );
	}

	// the same SetPixelFormat is used either way
	if( ::SetPixelFormat( win32.hDC, win32.pixelformat, &win32.pfd ) == FALSE )
	{
		common->Printf( "..." S_COLOR_YELLOW "SetPixelFormat failed: %s" S_COLOR_DEFAULT "\n", GetErrorString( ::GetLastError() ).c_str() );
		return false;
	}

	// get the full info
	win32.pfd.nSize = sizeof( PIXELFORMATDESCRIPTOR );
	win32.pfd.nVersion = 1;
	::DescribePixelFormat( win32.hDC, win32.pixelformat, sizeof( win32.pfd ), &win32.pfd );
	glConfig.colorBits = win32.pfd.cColorBits;
	glConfig.depthBits = win32.pfd.cDepthBits;
	glConfig.stencilBits = win32.pfd.cStencilBits;

	//
	// startup the OpenGL subsystem by creating a context and making it current
	//
	common->Printf( "...creating GL context: " );
	win32.hGLRC = CreateOpenGLContextOnDC( win32.hDC, r_debugContext.GetBool() );
	if( win32.hGLRC == 0 )
	{
		common->Printf( S_COLOR_YELLOW "failed" S_COLOR_DEFAULT "\n" );
		return false;
	}
	common->Printf( "succeeded\n" );

	common->Printf( "...making context current: " );
	if( !wglMakeCurrent( win32.hDC, win32.hGLRC ) )
	{
		wglDeleteContext( win32.hGLRC );
		win32.hGLRC = NULL;
		common->Printf( S_COLOR_YELLOW "failed" S_COLOR_DEFAULT "\n" );
		return false;
	}
	common->Printf( "succeeded\n" );

	return true;
}

/*
====================
GLW_CreateWindowClasses
====================
*/
static void GLW_CreateWindowClasses()
{
	//
	// register the window class if necessary
	//
	if( win32.windowClassRegistered )
	{
		return;
	}

	WNDCLASS wc = {};

	wc.style         = 0;
	wc.lpfnWndProc   = ( WNDPROC ) MainWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = win32.hInstance;
	wc.hIcon         = LoadIcon( win32.hInstance, MAKEINTRESOURCE( IDI_ICON1 ) );
	wc.hCursor       = NULL;
	wc.hbrBackground = ( struct HBRUSH__* )COLOR_GRAYTEXT;
	wc.lpszMenuName  = 0;
	wc.lpszClassName = WIN32_WINDOW_CLASS_NAME;

	if( !RegisterClass( &wc ) )
	{
		common->FatalError( "GLW_CreateWindow: could not register window class" );
	}
	common->Printf( "...registered window class\n" );

	// now register the fake window class that is only used
	// to get wgl extensions
	wc.style         = 0;
	wc.lpfnWndProc   = ( WNDPROC ) FakeWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = win32.hInstance;
	wc.hIcon         = LoadIcon( win32.hInstance, MAKEINTRESOURCE( IDI_ICON1 ) );
	wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = ( struct HBRUSH__* )COLOR_GRAYTEXT;
	wc.lpszMenuName  = 0;
	wc.lpszClassName = WIN32_FAKE_WINDOW_CLASS_NAME;

	if( !RegisterClass( &wc ) )
	{
		common->FatalError( "GLW_CreateWindow: could not register window class" );
	}
	common->Printf( "...registered fake window class\n" );

	win32.windowClassRegistered = true;
}

/*
=======================
GLW_CreateWindow

Responsible for creating the Win32 window.
If fullscreen, it won't have a border
=======================
*/
static bool GLW_CreateWindow( glimpParms_t parms )
{
	int x, y, w, h;
	if( !GLW_GetWindowDimensions( parms, x, y, w, h ) )
	{
		return false;
	}

	int	stylebits;
	int	exstyle;
	if( parms.fullScreen != 0 )
	{
		exstyle = WS_EX_TOPMOST;
		stylebits = WS_POPUP | WS_VISIBLE | WS_SYSMENU;
	}
	else {
		exstyle = 0;
		stylebits = WINDOW_STYLE | WS_SYSMENU;
	}

	win32.hWnd = ::CreateWindowEx( exstyle, WIN32_WINDOW_CLASS_NAME, GAME_NAME,
								   stylebits, x, y, w, h, NULL, NULL, win32.hInstance, NULL );
	if( !win32.hWnd )
	{
		common->Printf( S_COLOR_YELLOW "GLW_CreateWindow() - Couldn't create window" S_COLOR_DEFAULT "\n" );
		return false;
	}

	::SetTimer( win32.hWnd, 0, 100, NULL );

	::ShowWindow( win32.hWnd, SW_SHOW );
	::UpdateWindow( win32.hWnd );
	common->Printf( "...created window @ %d,%d (%dx%d)\n", x, y, w, h );

	// makeCurrent NULL frees the DC, so get another
	win32.hDC = ::GetDC( win32.hWnd );
	if( !win32.hDC )
	{
		common->Printf( S_COLOR_YELLOW "GLW_CreateWindow() - GetDC() failed" S_COLOR_DEFAULT "\n" );
		return false;
	}

	// Check to see if we can get a stereo pixel format, even if we aren't going to use it,
	// so the menu option can be
	glConfig.stereoPixelFormatAvailable = false;
	///if( GLW_ChoosePixelFormat( win32.hDC, parms.multiSamples, true ) != -1 ) {
	///	glConfig.stereoPixelFormatAvailable = true;
	///}

	if( !GLW_InitDriver( parms ) )
	{
		::ShowWindow( win32.hWnd, SW_HIDE );
		::DestroyWindow( win32.hWnd );
		win32.hWnd = NULL;
		return false;
	}

	::SetForegroundWindow( win32.hWnd );
	::SetFocus( win32.hWnd );

	glConfig.isFullscreen = parms.fullScreen;

	return true;
}


void GLimp_PreInit()
{
	// DG: not needed on this platform, so just do nothing
}

/*
===================
GLimp_Init

This is the platform specific OpenGL initialization function.  It
is responsible for loading OpenGL, initializing it,
creating a window of the appropriate size, doing
fullscreen manipulations, etc.  Its overall responsibility is
to make sure that a functional OpenGL subsystem is operating
when it returns to the ref.

If there is any failure, the renderer will revert back to safe
parameters and try again.
===================
*/
bool GLimp_Init( const glimpParms_t & parms )
{
	cmdSystem->AddCommand( "testSwapBuffers", GLimp_TestSwapBuffers, CMD_FL_SYSTEM, "Times swapbuffer options" );

	common->Printf( "Initializing OpenGL subsystem with multisamples:%i stereo:%i fullscreen:%i\n", parms.multiSamples, parms.stereo, parms.fullScreen );

	// check our desktop attributes
	HDC hDC = ::GetDC( ::GetDesktopWindow() );
	win32.desktopBitsPixel = ::GetDeviceCaps( hDC, BITSPIXEL );
	win32.desktopWidth = ::GetDeviceCaps( hDC, HORZRES );
	win32.desktopHeight = ::GetDeviceCaps( hDC, VERTRES );
	::ReleaseDC( ::GetDesktopWindow(), hDC );

	// we can't run in a window unless it is 32 bpp
	if( win32.desktopBitsPixel < 32 && parms.fullScreen <= 0 )
	{
		common->Printf( S_COLOR_YELLOW "Windowed mode requires 32 bit desktop depth" S_COLOR_DEFAULT "\n" );
		return false;
	}

	// save the hardware gamma so it can be
	// restored on exit
	GLimp_SaveGamma();

	// create our window classes if we haven't already
	GLW_CreateWindowClasses();

	// this will load the dll and set all our gl* function pointers,
	// but doesn't create a window

	// getting the wgl extensions involves creating a fake window to get a context,
	// which is pretty disgusting, and seems to mess with the AGP VAR allocation
	GLW_GetWGLExtensionsWithFakeWindow();

	// Optionally ChangeDisplaySettings to get a different fullscreen resolution.
	if( !GLW_ChangeDislaySettingsIfNeeded( parms ) )
	{
		GLimp_Shutdown();
		return false;
	}

	// try to create a window with the correct pixel format
	// and init the renderer context
	if( !GLW_CreateWindow( parms ) )
	{
		GLimp_Shutdown();
		return false;
	}

	glConfig.isFullscreen = parms.fullScreen;
	glConfig.isStereoPixelFormat = parms.stereo;
	glConfig.nativeScreenWidth = parms.width;
	glConfig.nativeScreenHeight = parms.height;
	glConfig.multisamples = parms.multiSamples;

	glConfig.pixelAspect = 1.0f;	// FIXME: some monitor modes may be distorted
	// should side-by-side stereo modes be consider aspect 0.5?

	// get the screen size, which may not be reliable...
	// If we use the windowDC, I get my 30" monitor, even though the window is
	// on a 27" monitor, so get a dedicated DC for the full screen device name.
	const idStr deviceName = ::GetDeviceName( idMath::Max( 0, parms.fullScreen - 1 ) );
	HDC deviceDC = ::CreateDC( deviceName.c_str(), deviceName.c_str(), NULL, NULL );
	const int mmWide = ::GetDeviceCaps( win32.hDC, HORZSIZE );
	::DeleteDC( deviceDC );

	glConfig.physicalScreenWidthInCentimeters = ( mmWide == 0 )? 100.0f : ( 0.1f * mmWide );

	// RB: we probably have a new OpenGL 3.2 core context so reinitialize GLEW
	GLenum glewResult = glewInit();
	if( GLEW_OK != glewResult )
	{
		// glewInit failed, something is seriously wrong
		common->Printf( S_COLOR_YELLOW "GLimp_Init() - GLEW could not load OpenGL subsystem: %s", glewGetErrorString( glewResult ) );
	}
	else {
		common->Printf( "Using GLEW %s\n", glewGetString( GLEW_VERSION ) );
	}
	// RB end

	// wglSwapinterval, etc
	//GLW_CheckWGLExtensions( win32.hDC );

	return true;
}

/*
===================
GLimp_SetScreenParms

Sets up the screen based on passed parms..
===================
*/
bool GLimp_SetScreenParms( const glimpParms_t & parms )
{
	// Optionally ChangeDisplaySettings to get a different fullscreen resolution.
	if( !GLW_ChangeDislaySettingsIfNeeded( parms ) )
	{
		return false;
	}

	int x, y, w, h;
	if( !GLW_GetWindowDimensions( parms, x, y, w, h ) )
	{
		return false;
	}

	int exstyle;
	int stylebits;

	if( parms.fullScreen )
	{
		exstyle = WS_EX_TOPMOST;
		stylebits = WS_POPUP | WS_VISIBLE | WS_SYSMENU;
	}
	else
	{
		exstyle = 0;
		stylebits = WINDOW_STYLE | WS_SYSMENU;
	}

	SetWindowLong( win32.hWnd, GWL_STYLE, stylebits );
	SetWindowLong( win32.hWnd, GWL_EXSTYLE, exstyle );
	SetWindowPos( win32.hWnd, parms.fullScreen ? HWND_TOPMOST : HWND_NOTOPMOST, x, y, w, h, SWP_SHOWWINDOW );

	glConfig.isFullscreen = parms.fullScreen;
	glConfig.pixelAspect = 1.0f;	// FIXME: some monitor modes may be distorted

	glConfig.isFullscreen = parms.fullScreen;
	glConfig.nativeScreenWidth = parms.width;
	glConfig.nativeScreenHeight = parms.height;

	return true;
}

/*
===================
GLimp_Shutdown

This routine does all OS specific shutdown procedures for the OpenGL
subsystem.
===================
*/
void GLimp_Shutdown()
{
	const char* success[] = { "failed", "success" };
	int retVal;

	common->Printf( "Shutting down OpenGL subsystem\n" );

	// set current context to NULL
	//if( wglMakeCurrent )
	{
		retVal = wglMakeCurrent( NULL, NULL ) != 0;
		common->Printf( "...wglMakeCurrent( NULL, NULL ): %s\n", success[retVal] );
	}

	// delete HGLRC
	if( win32.hGLRC )
	{
		retVal = wglDeleteContext( win32.hGLRC ) != 0;
		common->Printf( "...deleting GL context: %s\n", success[retVal] );
		win32.hGLRC = NULL;
	}

	// release DC
	if( win32.hDC )
	{
		retVal = ReleaseDC( win32.hWnd, win32.hDC ) != 0;
		common->Printf( "...releasing DC: %s\n", success[retVal] );
		win32.hDC   = NULL;
	}

	// destroy window
	if( win32.hWnd )
	{
		common->Printf( "...destroying window\n" );
		ShowWindow( win32.hWnd, SW_HIDE );
		DestroyWindow( win32.hWnd );
		win32.hWnd = NULL;
	}

	// reset display settings
	if( win32.cdsFullscreen )
	{
		common->Printf( "...resetting display\n" );
		ChangeDisplaySettings( 0, 0 );
		win32.cdsFullscreen = 0;
	}

	// restore gamma
	GLimp_RestoreGamma();
}

/*
=====================
GLimp_SwapBuffers
=====================
*/
// RB: use GLEW for V-Sync
void GLimp_SwapBuffers()
{
	if( r_swapInterval.IsModified() )
	{
		r_swapInterval.ClearModified();

		int interval = 0;
		if( r_swapInterval.GetInteger() == 1 )
		{
			interval = ( glConfig.swapControlTearAvailable ) ? -1 : 1;
		}
		else if( r_swapInterval.GetInteger() == 2 )
		{
			interval = 1;
		}

		if( WGLEW_EXT_swap_control )
		{
			wglSwapIntervalEXT( interval );
		}
	}

	::SwapBuffers( win32.hDC );
}
// RB end



