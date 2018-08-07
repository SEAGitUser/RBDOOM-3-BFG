#pragma hdrstop
#include "precompiled.h"

#include "win_local.h"
//#include "rc/doom_resource.h"

//#include "../../renderer/RenderSystem.h"
#include "../../renderer/tr_local.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

/*
========================
GLimp_GetOldGammaRamp
========================
*/
void GLimp_SaveGamma()
{
	HDC hDC = ::GetDC( ::GetDesktopWindow() );
	BOOL success = ::GetDeviceGammaRamp( hDC, win32.oldHardwareGamma );
	common->DPrintf( "...getting default gamma ramp: %s\n", success ? "success" : "failed" );
	::ReleaseDC( ::GetDesktopWindow(), hDC );
}

/*
========================
GLimp_RestoreGamma
========================
*/
void GLimp_RestoreGamma()
{
	// if we never read in a reasonable looking
	// table, don't write it out
	if( win32.oldHardwareGamma[ 0 ][ 255 ] == 0 )
		return;

	HDC hDC = ::GetDC( ::GetDesktopWindow() );
	BOOL success = ::SetDeviceGammaRamp( hDC, win32.oldHardwareGamma );
	common->DPrintf( "...restoring hardware gamma: %s\n", success ? "success" : "failed" );
	::ReleaseDC( GetDesktopWindow(), hDC );
}

/*
========================
GLimp_SetGamma

The renderer calls this when the user adjusts r_gamma or r_brightness
========================
*/
void GLimp_SetGamma( unsigned short red[ 256 ], unsigned short green[ 256 ], unsigned short blue[ 256 ] )
{
	if( !win32.hDC )
		return;

	unsigned short table[ 3 ][ 256 ];

	for( int i = 0; i < 256; i++ )
	{
		table[ 0 ][ i ] = red[ i ];
		table[ 1 ][ i ] = green[ i ];
		table[ 2 ][ i ] = blue[ i ];
	}

	if( !SetDeviceGammaRamp( win32.hDC, table ) )
	{
		common->Printf( "WARNING: SetDeviceGammaRamp failed.\n" );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

/*
========================
GetDisplayName
========================
*/
const char* GetDisplayName( const int deviceNum )
{
	static DISPLAY_DEVICE	device;
	device.cb = sizeof( device );
	if( !EnumDisplayDevices(
		0,			// lpDevice
		deviceNum,
		&device,
		0 /* dwFlags */ ) )
	{
		return NULL;
	}
	return device.DeviceName;
}

/*
========================
GetDeviceName
========================
*/
idStr GetDeviceName( const int deviceNum )
{
	DISPLAY_DEVICE	device = {};
	device.cb = sizeof( device );
	if( !EnumDisplayDevices(
		0,			// lpDevice
		deviceNum,
		&device,
		0 /* dwFlags */ ) )
	{
		return idStr();
	}

	// get the monitor for this display
	if( !( device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP ) )
	{
		return idStr();
	}

	return( device.DeviceName );
}

/*
========================
GetDisplayCoordinates
========================
*/
bool GetDisplayCoordinates( const int deviceNum, int& x, int& y, int& width, int& height, int& displayHz )
{
	idStr deviceName = GetDeviceName( deviceNum );
	if( deviceName.Length() == 0 )
	{
		return false;
	}

	DISPLAY_DEVICE	device = {};
	device.cb = sizeof( device );
	if( !EnumDisplayDevices(
		0,			// lpDevice
		deviceNum,
		&device,
		0 /* dwFlags */ ) )
	{
		return false;
	}

	DISPLAY_DEVICE	monitor;
	monitor.cb = sizeof( monitor );
	if( !EnumDisplayDevices(
		deviceName.c_str(),
		0,
		&monitor,
		0 /* dwFlags */ ) )
	{
		return false;
	}

	DEVMODE	devmode;
	devmode.dmSize = sizeof( devmode );
	if( !EnumDisplaySettings( deviceName.c_str(), ENUM_CURRENT_SETTINGS, &devmode ) )
	{
		return false;
	}

	common->Printf( "display device: %i\n", deviceNum );
	common->Printf( "  DeviceName  : %s\n", device.DeviceName );
	common->Printf( "  DeviceString: %s\n", device.DeviceString );
	common->Printf( "  StateFlags  : 0x%x\n", device.StateFlags );
	common->Printf( "  DeviceID    : %s\n", device.DeviceID );
	common->Printf( "  DeviceKey   : %s\n", device.DeviceKey );
	common->Printf( "      DeviceName  : %s\n", monitor.DeviceName );
	common->Printf( "      DeviceString: %s\n", monitor.DeviceString );
	common->Printf( "      StateFlags  : 0x%x\n", monitor.StateFlags );
	common->Printf( "      DeviceID    : %s\n", monitor.DeviceID );
	common->Printf( "      DeviceKey   : %s\n", monitor.DeviceKey );
	common->Printf( "          dmPosition.x      : %i\n", devmode.dmPosition.x );
	common->Printf( "          dmPosition.y      : %i\n", devmode.dmPosition.y );
	common->Printf( "          dmBitsPerPel      : %i\n", devmode.dmBitsPerPel );
	common->Printf( "          dmPelsWidth       : %i\n", devmode.dmPelsWidth );
	common->Printf( "          dmPelsHeight      : %i\n", devmode.dmPelsHeight );
	common->Printf( "          dmDisplayFlags    : 0x%x\n", devmode.dmDisplayFlags );
	common->Printf( "          dmDisplayFrequency: %i\n", devmode.dmDisplayFrequency );

	x = devmode.dmPosition.x;
	y = devmode.dmPosition.y;
	width = devmode.dmPelsWidth;
	height = devmode.dmPelsHeight;
	displayHz = devmode.dmDisplayFrequency;

	return true;
}

/*
====================
DMDFO
====================
*/
static const char* DMDFO( int dmDisplayFixedOutput )
{
	switch( dmDisplayFixedOutput )
	{
		case DMDFO_DEFAULT:
			return "DMDFO_DEFAULT";
		case DMDFO_CENTER:
			return "DMDFO_CENTER";
		case DMDFO_STRETCH:
			return "DMDFO_STRETCH";
	}
	return "UNKNOWN";
}

/*
====================
PrintDevMode
====================
*/
static void PrintDevMode( DEVMODE& devmode )
{
	common->Printf( "          dmPosition.x        : %i\n", devmode.dmPosition.x );
	common->Printf( "          dmPosition.y        : %i\n", devmode.dmPosition.y );
	common->Printf( "          dmBitsPerPel        : %i\n", devmode.dmBitsPerPel );
	common->Printf( "          dmPelsWidth         : %i\n", devmode.dmPelsWidth );
	common->Printf( "          dmPelsHeight        : %i\n", devmode.dmPelsHeight );
	common->Printf( "          dmDisplayFixedOutput: %s\n", DMDFO( devmode.dmDisplayFixedOutput ) );
	common->Printf( "          dmDisplayFlags      : 0x%x\n", devmode.dmDisplayFlags );
	common->Printf( "          dmDisplayFrequency  : %i\n", devmode.dmDisplayFrequency );
}

/*
====================
DumpAllDisplayDevices
====================
*/
void DumpAllDisplayDevices()
{
	common->Printf( "\n" );
	for( int deviceNum = 0; ; deviceNum++ )
	{
		DISPLAY_DEVICE	device = {};
		device.cb = sizeof( device );
		if( !EnumDisplayDevices(
			0,			// lpDevice
			deviceNum,
			&device,
			0 /* dwFlags */ ) )
		{
			break;
		}

		common->Printf( "display device: %i\n", deviceNum );
		common->Printf( "  DeviceName  : %s\n", device.DeviceName );
		common->Printf( "  DeviceString: %s\n", device.DeviceString );
		common->Printf( "  StateFlags  : 0x%x\n", device.StateFlags );
		common->Printf( "  DeviceID    : %s\n", device.DeviceID );
		common->Printf( "  DeviceKey   : %s\n", device.DeviceKey );

		for( int monitorNum = 0; ; monitorNum++ )
		{
			DISPLAY_DEVICE	monitor = {};
			monitor.cb = sizeof( monitor );
			if( !EnumDisplayDevices(
				device.DeviceName,
				monitorNum,
				&monitor,
				0 /* dwFlags */ ) )
			{
				break;
			}

			common->Printf( "      DeviceName  : %s\n", monitor.DeviceName );
			common->Printf( "      DeviceString: %s\n", monitor.DeviceString );
			common->Printf( "      StateFlags  : 0x%x\n", monitor.StateFlags );
			common->Printf( "      DeviceID    : %s\n", monitor.DeviceID );
			common->Printf( "      DeviceKey   : %s\n", monitor.DeviceKey );

			DEVMODE	currentDevmode = {};
			if( !EnumDisplaySettings( device.DeviceName, ENUM_CURRENT_SETTINGS, &currentDevmode ) )
			{
				common->Printf( "ERROR:  EnumDisplaySettings(ENUM_CURRENT_SETTINGS) failed!\n" );
			}
			common->Printf( "          -------------------\n" );
			common->Printf( "          ENUM_CURRENT_SETTINGS\n" );
			PrintDevMode( currentDevmode );

			DEVMODE	registryDevmode = {};
			if( !EnumDisplaySettings( device.DeviceName, ENUM_REGISTRY_SETTINGS, &registryDevmode ) )
			{
				common->Printf( "ERROR:  EnumDisplaySettings(ENUM_CURRENT_SETTINGS) failed!\n" );
			}
			common->Printf( "          -------------------\n" );
			common->Printf( "          ENUM_CURRENT_SETTINGS\n" );
			PrintDevMode( registryDevmode );

			for( int modeNum = 0; ; modeNum++ )
			{
				DEVMODE	devmode = {};

				if( !EnumDisplaySettings( device.DeviceName, modeNum, &devmode ) )
				{
					break;
				}

				if( devmode.dmBitsPerPel != 32 )
				{
					continue;
				}
				if( devmode.dmDisplayFrequency < 60 )
				{
					continue;
				}
				if( devmode.dmPelsHeight < 720 )
				{
					continue;
				}
				common->Printf( "          -------------------\n" );
				common->Printf( "          modeNum             : %i\n", modeNum );
				PrintDevMode( devmode );
			}
		}
	}
	common->Printf( "\n" );
}

class idSort_VidMode : public idSort_Quick< vidMode_t, idSort_VidMode > {
public:
	int Compare( const vidMode_t& a, const vidMode_t& b ) const
	{
		int wd = a.width - b.width;
		int hd = a.height - b.height;
		int fd = a.displayHz - b.displayHz;
		return ( hd != 0 ) ? hd : ( wd != 0 ) ? wd : fd;
	}
};

/*
====================
R_GetModeListForDisplay
====================
*/
bool R_GetModeListForDisplay( const int requestedDisplayNum, idList<vidMode_t>& modeList )
{
	modeList.Clear();

	bool verbose = false;

	for( int displayNum = requestedDisplayNum; ; displayNum++ )
	{
		DISPLAY_DEVICE	device;
		device.cb = sizeof( device );
		if( !EnumDisplayDevices(
			0,			// lpDevice
			displayNum,
			&device,
			0 /* dwFlags */ ) )
		{
			return false;
		}

		// get the monitor for this display
		if( !( device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP ) )
		{
			continue;
		}

		DISPLAY_DEVICE	monitor;
		monitor.cb = sizeof( monitor );
		if( !EnumDisplayDevices(
			device.DeviceName,
			0,
			&monitor,
			0 /* dwFlags */ ) )
		{
			continue;
		}

		DEVMODE	devmode;
		devmode.dmSize = sizeof( devmode );

		if( verbose )
		{
			common->Printf( "display device: %i\n", displayNum );
			common->Printf( "  DeviceName  : %s\n", device.DeviceName );
			common->Printf( "  DeviceString: %s\n", device.DeviceString );
			common->Printf( "  StateFlags  : 0x%x\n", device.StateFlags );
			common->Printf( "  DeviceID    : %s\n", device.DeviceID );
			common->Printf( "  DeviceKey   : %s\n", device.DeviceKey );
			common->Printf( "      DeviceName  : %s\n", monitor.DeviceName );
			common->Printf( "      DeviceString: %s\n", monitor.DeviceString );
			common->Printf( "      StateFlags  : 0x%x\n", monitor.StateFlags );
			common->Printf( "      DeviceID    : %s\n", monitor.DeviceID );
			common->Printf( "      DeviceKey   : %s\n", monitor.DeviceKey );
		}

		for( int modeNum = 0; ; modeNum++ )
		{
			if( !EnumDisplaySettings( device.DeviceName, modeNum, &devmode ) )
			{
				break;
			}

			if( devmode.dmBitsPerPel != 32 )
			{
				continue;
			}
			if( ( devmode.dmDisplayFrequency != 60 ) && ( devmode.dmDisplayFrequency != 120 ) )
			{
				continue;
			}
			if( devmode.dmPelsHeight < 720 )
			{
				continue;
			}
			if( verbose )
			{
				common->Printf( "          -------------------\n" );
				common->Printf( "          modeNum             : %i\n", modeNum );
				common->Printf( "          dmPosition.x        : %i\n", devmode.dmPosition.x );
				common->Printf( "          dmPosition.y        : %i\n", devmode.dmPosition.y );
				common->Printf( "          dmBitsPerPel        : %i\n", devmode.dmBitsPerPel );
				common->Printf( "          dmPelsWidth         : %i\n", devmode.dmPelsWidth );
				common->Printf( "          dmPelsHeight        : %i\n", devmode.dmPelsHeight );
				common->Printf( "          dmDisplayFixedOutput: %s\n", DMDFO( devmode.dmDisplayFixedOutput ) );
				common->Printf( "          dmDisplayFlags      : 0x%x\n", devmode.dmDisplayFlags );
				common->Printf( "          dmDisplayFrequency  : %i\n", devmode.dmDisplayFrequency );
			}
			vidMode_t mode;
			mode.width = devmode.dmPelsWidth;
			mode.height = devmode.dmPelsHeight;
			mode.displayHz = devmode.dmDisplayFrequency;
			modeList.AddUnique( mode );
		}

		if( modeList.Num() > 0 )
		{
			// sort with lowest resolution first
			modeList.SortWithTemplate( idSort_VidMode() );

			return true;
		}
	}

	// Never gets here
	return false;
}

/*
====================
GLW_GetWindowDimensions
====================
*/
bool GLW_GetWindowDimensions( const glimpParms_t & parms, int& x, int& y, int& w, int& h )
{
	//
	// compute width and height
	//
	if( parms.fullScreen != 0 )
	{
		if( parms.fullScreen == -1 )
		{
			// borderless window at specific location, as for spanning
			// multiple monitor outputs
			x = parms.x;
			y = parms.y;
			w = parms.width;
			h = parms.height;
		}
		else
		{
			// get the current monitor position and size on the desktop, assuming
			// any required ChangeDisplaySettings has already been done
			int displayHz = 0;
			if( !GetDisplayCoordinates( parms.GetOutputIndex(), x, y, w, h, displayHz ) )
			{
				return false;
			}
		}
	}
	else
	{
		RECT r;
		::SetRect( &r, 0, 0, parms.width, parms.height );

		// adjust width and height for window border
		::AdjustWindowRect( &r, WINDOW_STYLE | WS_SYSMENU, FALSE );

		w = r.right - r.left;
		h = r.bottom - r.top;

		x = parms.x;
		y = parms.y;
	}

	return true;
}

/*
===================
PrintCDSError
===================
*/
static void PrintCDSError( int value )
{
	switch( value )
	{
		case DISP_CHANGE_RESTART:
			common->Printf( "restart required\n" );
			break;
		case DISP_CHANGE_BADPARAM:
			common->Printf( "bad param\n" );
			break;
		case DISP_CHANGE_BADFLAGS:
			common->Printf( "bad flags\n" );
			break;
		case DISP_CHANGE_FAILED:
			common->Printf( "DISP_CHANGE_FAILED\n" );
			break;
		case DISP_CHANGE_BADMODE:
			common->Printf( "bad mode\n" );
			break;
		case DISP_CHANGE_NOTUPDATED:
			common->Printf( "not updated\n" );
			break;
		default:
			common->Printf( "unknown error %d\n", value );
			break;
	}
}

/*
===================
GLW_ChangeDislaySettingsIfNeeded

Optionally ChangeDisplaySettings to get a different fullscreen resolution.
Default uses the full desktop resolution.
===================
*/
bool GLW_ChangeDislaySettingsIfNeeded( const glimpParms_t & parms )
{
	// If we had previously changed the display settings on a different monitor,
	// go back to standard.
	if( win32.cdsFullscreen != 0 && win32.cdsFullscreen != parms.fullScreen )
	{
		win32.cdsFullscreen = 0;
		::ChangeDisplaySettings( 0, 0 );
		sys->Sleep( 1000 ); // Give the driver some time to think about this change
	}

	// 0 is dragable mode on desktop, -1 is borderless window on desktop
	if( parms.fullScreen <= 0 )
	{
		return true;
	}

	// if we are already in the right resolution, don't do a ChangeDisplaySettings
	int x, y, width, height, displayHz;

	if( !GetDisplayCoordinates( parms.fullScreen - 1, x, y, width, height, displayHz ) )
	{
		return false;
	}
	if( width == parms.width && height == parms.height && ( displayHz == parms.displayHz || parms.displayHz == 0 ) )
	{
		return true;
	}

	DEVMODE dm = {};
	dm.dmSize = sizeof( dm );
	dm.dmPelsWidth = parms.width;
	dm.dmPelsHeight = parms.height;
	dm.dmBitsPerPel = 32;
	dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
	if( parms.displayHz != 0 )
	{
		dm.dmDisplayFrequency = parms.displayHz;
		dm.dmFields |= DM_DISPLAYFREQUENCY;
	}

	common->Printf( "...calling CDS: " );

	const char* const deviceName = GetDisplayName( parms.fullScreen - 1 );

	int	cdsRet;
	if( ( cdsRet = ::ChangeDisplaySettingsEx( deviceName, &dm, NULL, CDS_FULLSCREEN, NULL ) ) == DISP_CHANGE_SUCCESSFUL )
	{
		common->Printf( "ok\n" );
		win32.cdsFullscreen = parms.fullScreen;
		return true;
	}

	common->Printf( "^3failed^0, " );
	PrintCDSError( cdsRet );
	return false;
}

