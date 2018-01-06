/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2016 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

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

#include "tr_local.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

extern void R_SetNewMode( const bool fullInit );

/*
==============
 R_ListModes_f
==============
*/
static void R_ListModes_f( const idCmdArgs& args )
{
	for( int displayNum = 0; ; displayNum++ )
	{
		idList<vidMode_t> modeList;
		if( !R_GetModeListForDisplay( displayNum, modeList ) )
		{
			break;
		}
		for( int i = 0; i < modeList.Num(); i++ )
		{
			common->Printf( "Monitor %i, mode %3i: %4i x %4i @ %ihz\n", displayNum + 1, i, modeList[ i ].width, modeList[ i ].height, modeList[ i ].displayHz );
		}
	}
}

/*
=================
 R_SizeUp_f

	Keybinding command
=================
*/
static void R_SizeUp_f( const idCmdArgs& args )
{
	if( r_screenFraction.GetInteger() + 10 > 100 )
	{
		r_screenFraction.SetInteger( 100 );
	}
	else
	{
		r_screenFraction.SetInteger( r_screenFraction.GetInteger() + 10 );
	}
}

/*
=================
 R_SizeDown_f

	Keybinding command
=================
*/
static void R_SizeDown_f( const idCmdArgs& args )
{
	if( r_screenFraction.GetInteger() - 10 < 10 )
	{
		r_screenFraction.SetInteger( 10 );
	}
	else
	{
		r_screenFraction.SetInteger( r_screenFraction.GetInteger() - 10 );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

/*
=============
 R_TestImage_f

	Display the given image centered on the screen.
	testimage <number>
	testimage <filename>
=============
*/
static void R_TestImage_f( const idCmdArgs& args )
{
	int imageNum;

	if( tr.testVideo )
	{
		delete tr.testVideo;
		tr.testVideo = NULL;
	}
	tr.testImage = NULL;

	if( args.Argc() != 2 )
	{
		return;
	}

	if( idStr::IsNumeric( args.Argv( 1 ) ) )
	{
		imageNum = atoi( args.Argv( 1 ) );
		if( imageNum >= 0 && imageNum < renderImageManager->images.Num() )
		{
			tr.testImage = renderImageManager->images[ imageNum ];
		}
	}
	else
	{
		tr.testImage = renderImageManager->ImageFromFile( args.Argv( 1 ), TF_DEFAULT, TR_REPEAT, TD_DEFAULT );
	}
}

/*
=============
R_TestVideo_f

	Plays the cinematic file in a testImage
=============
*/
static void R_TestVideo_f( const idCmdArgs& args )
{
	if( tr.testVideo )
	{
		delete tr.testVideo;
		tr.testVideo = NULL;
	}
	tr.testImage = NULL;

	if( args.Argc() < 2 )
	{
		return;
	}

	tr.testImage = renderImageManager->ImageFromFile( "_scratch", TF_DEFAULT, TR_REPEAT, TD_DEFAULT );
	tr.testVideo = idCinematic::Alloc();
	tr.testVideo->InitFromFile( args.Argv( 1 ), true );

	cinData_t cin = tr.testVideo->ImageForTime( 0 );
	if( cin.imageY == NULL )
	{
		delete tr.testVideo;
		tr.testVideo = NULL;
		tr.testImage = NULL;
		return;
	}

	common->Printf( "%i x %i images\n", cin.imageWidth, cin.imageHeight );

	int	len = tr.testVideo->AnimationLength();
	common->Printf( "%5.1f seconds of video\n", len * 0.001 );

	tr.testVideoStartTime = tr.primaryRenderView.time[ 1 ];

	// try to play the matching wav file
	idStr	wavString = args.Argv( ( args.Argc() == 2 ) ? 1 : 2 );
	wavString.StripFileExtension();
	wavString = wavString + ".wav";
	common->SW()->PlayShaderDirectly( wavString.c_str() );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

/*
================,
 R_ReloadGuis_f

	Reloads any guis that have had their file timestamps changed.
	An optional "all" parameter will cause all models to reload, even
	if they are not out of date.

	Should we also reload the map models?
================
*/
static void R_ReloadGuis_f( const idCmdArgs& args )
{
	bool all;

	if( !idStr::Icmp( args.Argv( 1 ), "all" ) )
	{
		all = true;
		common->Printf( "Reloading all gui files...\n" );
	}
	else
	{
		all = false;
		common->Printf( "Checking for changed gui files...\n" );
	}

	uiManager->Reload( all );
}

/*
================,
 R_ListGuis_f
================
*/
static void R_ListGuis_f( const idCmdArgs& args )
{
	uiManager->ListGuis();
}

/*
===============
 TouchGui_f

	this is called from the main thread
===============
*/
static void R_TouchGui_f( const idCmdArgs& args )
{
	const char*	gui = args.Argv( 1 );
	if( !gui[ 0 ] )
	{
		common->Printf( "USAGE: touchGui <guiName>\n" );
		return;
	}

	common->Printf( "touchGui %s\n", gui );
	const bool captureToImage = false;
	common->UpdateScreen( captureToImage );
	uiManager->Touch( gui );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

/*
=====================
 R_ReloadSurface_f

	Reload the material displayed by r_showSurfaceInfo
=====================
*/
static void R_ReloadSurface_f( const idCmdArgs& args )
{
	// start far enough away that we don't hit the player model
	idVec3 start = tr.primaryView->GetOrigin() + tr.primaryView->GetAxis()[ 0 ] * 16;
	idVec3 end = start + tr.primaryView->GetAxis()[ 0 ] * 1000.0f;

	modelTrace_t mt;
	if( !tr.primaryWorld->Trace( mt, start, end, 0.0f, false ) )
	{
		return;
	}

	common->Printf( "Reloading %s\n", mt.material->GetName() );

	// reload the decl
	mt.material->base->Reload();

	// reload any images used by the decl
	mt.material->ReloadImages( false );
}


/*
===================
 R_ReportSurfaceAreas_f

	Prints a list of the materials sorted by surface area
===================
*/

static int R_QsortSurfaceAreas( const void* a, const void* b )
{
	const idMaterial*	ea, *eb;
	int	ac, bc;

	ea = *( idMaterial** )a;
	if( !ea->EverReferenced() )
	{
		ac = 0;
	}
	else
	{
		ac = ea->GetSurfaceArea();
	}
	eb = *( idMaterial** )b;
	if( !eb->EverReferenced() )
	{
		bc = 0;
	}
	else
	{
		bc = eb->GetSurfaceArea();
	}

	if( ac < bc )
	{
		return -1;
	}
	if( ac > bc )
	{
		return 1;
	}

	return idStr::Icmp( ea->GetName(), eb->GetName() );
}

#pragma warning( disable: 6385 ) // This is simply to get pass a false defect for /analyze -- if you can figure out a better way, please let Shawn know...
static void R_ReportSurfaceAreas_f( const idCmdArgs& args )
{
	unsigned int i;
	idMaterial ** list;

	const unsigned int count = declManager->GetNumDecls( DECL_MATERIAL );
	if( count == 0 )
	{
		return;
	}

	list = ( idMaterial** )_alloca( count * sizeof( *list ) );

	for( i = 0; i < count; ++i )
	{
		list[ i ] = ( idMaterial* )declManager->DeclByIndex( DECL_MATERIAL, i, false );
	}

	qsort( list, count, sizeof( list[ 0 ] ), R_QsortSurfaceAreas );

	// skip over ones with 0 area
	for( i = 0; i < count; ++i )
	{
		if( list[ i ]->GetSurfaceArea() > 0 )
		{
			break;
		}
	}

	for( ; i < count; ++i )
	{
		// report size in "editor blocks"
		int	blocks = list[ i ]->GetSurfaceArea() / 4096.0;
		common->Printf( "%7i %s\n", blocks, list[ i ]->GetName() );
	}
}
#pragma warning( default: 6385 )


/*
==============================================================================

	SCREEN SHOTS

==============================================================================
*/

static const char* fileExten[ 3 ] = { "tga", "png", "jpg" };
static const char* envDirection[ 6 ] = { "_px", "_nx", "_py", "_ny", "_pz", "_nz" };
static const char* skyDirection[ 6 ] = { "_forward", "_back", "_left", "_right", "_up", "_down" };

/*
====================
 R_ReadTiledPixels

	NO LONGER SUPPORTED (FIXME: make standard case work)

	Used to allow the rendering of an image larger than the actual window by
	tiling it into window-sized chunks and rendering each chunk separately

	If ref isn't specified, the full session UpdateScreen will be done.
====================
*/
static void R_ReadTiledPixels( int width, int height, byte* buffer, renderView_t* ref = NULL )
{
	// include extra space for OpenGL padding to word boundaries
	int sysWidth = renderSystem->GetWidth();
	int sysHeight = renderSystem->GetHeight();

	idTempArray<byte> temp( ( sysWidth + 3 ) * sysHeight * 3 );

	// foresthale 2014-03-01: fixed custom screenshot resolution by doing a more direct render path
#ifdef BUGFIXEDSCREENSHOTRESOLUTION
	if( sysWidth > width )
		sysWidth = width;

	if( sysHeight > height )
		sysHeight = height;

	// make sure the game / draw thread has completed
	//commonLocal.WaitGameThread();

	// discard anything currently on the list
	tr.SwapCommandBuffers( NULL, NULL, NULL, NULL );

	int originalNativeWidth = glConfig.nativeScreenWidth;
	int originalNativeHeight = glConfig.nativeScreenHeight;
	glConfig.nativeScreenWidth = sysWidth;
	glConfig.nativeScreenHeight = sysHeight;
#endif

	// disable scissor, so we don't need to adjust all those rects
	r_useScissor.SetBool( false );

	for( int xo = 0; xo < width; xo += sysWidth )
	{
		for( int yo = 0; yo < height; yo += sysHeight )
		{
			// foresthale 2014-03-01: fixed custom screenshot resolution by doing a more direct render path
		#ifdef BUGFIXEDSCREENSHOTRESOLUTION
			// discard anything currently on the list
			tr.SwapCommandBuffers( NULL, NULL, NULL, NULL );
			if( ref )
			{
				// ref is only used by envShot, Event_camShot, etc to grab screenshots of things in the world,
				// so this omits the hud and other effects
				tr.primaryWorld->RenderScene( ref );
			}
			else
			{
				// build all the draw commands without running a new game tic
				commonLocal.Draw();
			}
			// this should exit right after vsync, with the GPU idle and ready to draw
			const emptyCommand_t* cmd = tr.SwapCommandBuffers( NULL, NULL, NULL, NULL );

			// get the GPU busy with new commands
			tr.RenderCommandBuffers( cmd );

			// discard anything currently on the list (this triggers SwapBuffers)
			tr.SwapCommandBuffers( NULL, NULL, NULL, NULL );
		#else
			// foresthale 2014-03-01: note: ref is always NULL in every call path to this function
			if( ref )
			{
				// discard anything currently on the list
				tr.SwapCommandBuffers( NULL, NULL, NULL, NULL );

				// build commands to render the scene
				tr.primaryWorld->RenderScene( ref );

				// finish off these commands
				const emptyCommand_t* cmd = tr.SwapCommandBuffers( NULL, NULL, NULL, NULL );

				// issue the commands to the GPU
				tr.RenderCommandBuffers( cmd );
			}
			else
			{
				const bool captureToImage = false;
				common->UpdateScreen( captureToImage, false );
			}
		#endif

			int w = sysWidth;
			if( xo + w > width )
			{
				w = width - xo;
			}
			int h = sysHeight;
			if( yo + h > height )
			{
				h = height - yo;
			}

			glReadBuffer( GL_FRONT );
			glReadPixels( 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, temp.Ptr() );

			int	row = ( w * 3 + 3 ) & ~3;		// OpenGL pads to dword boundaries

			for( int y = 0; y < h; ++y )
			{
				memcpy( buffer + ( ( yo + y )* width + xo ) * 3, temp.Ptr() + y * row, w * 3 );
			}
		}
	}

	// foresthale 2014-03-01: fixed custom screenshot resolution by doing a more direct render path
#ifdef BUGFIXEDSCREENSHOTRESOLUTION
	// discard anything currently on the list
	tr.SwapCommandBuffers( NULL, NULL, NULL, NULL );

	glConfig.nativeScreenWidth = originalNativeWidth;
	glConfig.nativeScreenHeight = originalNativeHeight;
#endif

	r_useScissor.SetBool( true );
}

/*
==================
 TakeScreenshot

	Move to tr_imagefiles.c...

	Downsample is the number of steps to mipmap the image before saving it
	If ref == NULL, common->UpdateScreen will be used
==================
*/
// RB: changed .tga to .png
void idRenderSystemLocal::TakeScreenshot( int width, int height, const char* fileName, int blends, renderView_t* ref, int exten )
{
	byte* buffer = nullptr;
	int	i, j, c, temp;

	idStr finalFileName;
	finalFileName.Format( "%s.%s", fileName, fileExten[ exten ] );

	takingScreenshot = true;

	int pix = width * height;
	const int bufferSize = pix * 3 + 18;

	if( exten == PNG )
	{
		buffer = ( byte* )Mem_Alloc( pix * 3, TAG_TEMP );
	}
	else if( exten == TGA )
	{
		buffer = ( byte* )Mem_ClearedAlloc( bufferSize, TAG_TEMP );
	}
	assert( buffer );

	if( blends <= 1 )
	{
		if( exten == PNG )
		{
			R_ReadTiledPixels( width, height, buffer, ref );
		}
		else if( exten == TGA )
		{
			R_ReadTiledPixels( width, height, buffer + 18, ref );
		}
	}
	else
	{
		idTempArray<unsigned short> shortBuffer( pix * 2 * 3 );
		shortBuffer.Zero();

		// enable anti-aliasing jitter
		r_jitter.SetBool( true );

		for( i = 0; i < blends; ++i )
		{
			if( exten == PNG )
			{
				R_ReadTiledPixels( width, height, buffer, ref );
			}
			else if( exten == TGA )
			{
				R_ReadTiledPixels( width, height, buffer + 18, ref );
			}

			for( j = 0; j < pix * 3; ++j )
			{
				if( exten == PNG )
				{
					shortBuffer[ j ] += buffer[ j ];
				}
				else if( exten == TGA )
				{
					shortBuffer[ j ] += buffer[ 18 + j ];
				}
			}
		}

		// divide back to bytes
		for( i = 0; i < pix * 3; ++i )
		{
			if( exten == PNG )
			{
				buffer[ i ] = shortBuffer[ i ] / blends;
			}
			else if( exten == TGA )
			{
				buffer[ 18 + i ] = shortBuffer[ i ] / blends;
			}
		}

		r_jitter.SetBool( false );
	}

	if( exten == PNG )
	{
		R_WritePNG( finalFileName, buffer, 3, width, height, false, "fs_basepath" );
	}
	else
	{
		// fill in the header (this is vertically flipped, which qglReadPixels emits)
		buffer[ 2 ] = 2;	// uncompressed type
		buffer[ 12 ] = width & 255;
		buffer[ 13 ] = width >> 8;
		buffer[ 14 ] = height & 255;
		buffer[ 15 ] = height >> 8;
		buffer[ 16 ] = 24;	// pixel size

							// swap rgb to bgr
		c = 18 + width * height * 3;

		for( i = 18; i < c; i += 3 )
		{
			temp = buffer[ i ];
			buffer[ i ] = buffer[ i + 2 ];
			buffer[ i + 2 ] = temp;
		}

		fileSystem->WriteFile( finalFileName, buffer, c, "fs_basepath" );
	}

	Mem_Free( buffer );

	takingScreenshot = false;
}

/*
==================
 R_ScreenshotFilename

	Returns a filename with digits appended
	if we have saved a previous screenshot, don't scan
	from the beginning, because recording demo avis can involve
	thousands of shots
==================
*/
static void R_ScreenshotFilename( int& lastNumber, const char* base, idStr& fileName )
{
	bool restrict = cvarSystem->GetCVarBool( "fs_restrict" );
	cvarSystem->SetCVarBool( "fs_restrict", false );

	lastNumber++;
	if( lastNumber > 99999 )
	{
		lastNumber = 99999;
	}
	for( ; lastNumber < 99999; lastNumber++ )
	{

		// RB: added date to screenshot name
	#if 0
		int	frac = lastNumber;
		int	a, b, c, d, e;

		a = frac / 10000;
		frac -= a * 10000;
		b = frac / 1000;
		frac -= b * 1000;
		c = frac / 100;
		frac -= c * 100;
		d = frac / 10;
		frac -= d * 10;
		e = frac;

		sprintf( fileName, "%s%i%i%i%i%i.png", base, a, b, c, d, e );
	#else
		time_t aclock;
		time( &aclock );
		struct tm* t = localtime( &aclock );

		sprintf( fileName, "%s%s-%04d%02d%02d-%02d%02d%02d-%03d", base, "rbdoom-3-bfg",
			1900 + t->tm_year, 1 + t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, lastNumber );
	#endif
		// RB end
		if( lastNumber == 99999 )
		{
			break;
		}
		int len = fileSystem->ReadFile( fileName, NULL, NULL );
		if( len <= 0 )
		{
			break;
		}
		// check again...
	}
	cvarSystem->SetCVarBool( "fs_restrict", restrict );
}

/*
==================
 R_BlendedScreenShot

	screenshot
	screenshot [filename]
	screenshot [width] [height]
	screenshot [width] [height] [samples]
==================
*/
#define	MAX_BLENDS	256	// to keep the accumulation in shorts
static void R_ScreenShot_f( const idCmdArgs& args )
{
	static int lastNumber = 0;
	idStr checkname;

	int width = renderSystem->GetWidth();
	int height = renderSystem->GetHeight();
	int	blends = 0;

	switch( args.Argc() )
	{
		case 1:
			width = renderSystem->GetWidth();
			height = renderSystem->GetHeight();
			blends = 1;
			R_ScreenshotFilename( lastNumber, "screenshots/", checkname );
			break;
		case 2:
			width = renderSystem->GetWidth();
			height = renderSystem->GetHeight();
			blends = 1;
			checkname = args.Argv( 1 );
			break;
		case 3:
			width = atoi( args.Argv( 1 ) );
			height = atoi( args.Argv( 2 ) );
			blends = 1;
			R_ScreenshotFilename( lastNumber, "screenshots/", checkname );
			break;
		case 4:
			width = atoi( args.Argv( 1 ) );
			height = atoi( args.Argv( 2 ) );
			blends = atoi( args.Argv( 3 ) );
			if( blends < 1 )
			{
				blends = 1;
			}
			if( blends > MAX_BLENDS )
			{
				blends = MAX_BLENDS;
			}
			R_ScreenshotFilename( lastNumber, "screenshots/", checkname );
			break;
		default:
			common->Printf( "usage: screenshot\n       screenshot <filename>\n       screenshot <width> <height>\n       screenshot <width> <height> <blends>\n" );
			return;
	}

	// put the console away
	console->Close();

	tr.TakeScreenshot( width, height, checkname, blends, NULL, PNG );

	common->Printf( "Wrote %s\n", checkname.c_str() );
}

/*
===============
 R_StencilShot
	Save out a screenshot showing the stencil buffer expanded by 16x range
===============
*/
static void R_StencilShot()
{
	int			i, c;

	int	width = tr.GetWidth();
	int	height = tr.GetHeight();

	int	pix = width * height;

	c = pix * 3 + 18;
	idTempArray< byte > buffer( c );
	memset( buffer.Ptr(), 0, 18 );

	idTempArray< byte > byteBuffer( pix );

	glReadPixels( 0, 0, width, height, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, byteBuffer.Ptr() );

	for( i = 0; i < pix; i++ )
	{
		buffer[ 18 + i * 3 ] =
			buffer[ 18 + i * 3 + 1 ] =
			//		buffer[18+i*3+2] = ( byteBuffer[i] & 15 ) * 16;
			buffer[ 18 + i * 3 + 2 ] = byteBuffer[ i ];
	}

	// fill in the header (this is vertically flipped, which glReadPixels emits)
	buffer[ 2 ] = 2;		// uncompressed type
	buffer[ 12 ] = width & 255;
	buffer[ 13 ] = width >> 8;
	buffer[ 14 ] = height & 255;
	buffer[ 15 ] = height >> 8;
	buffer[ 16 ] = 24;	// pixel size

	fileSystem->WriteFile( "screenshots/stencilShot.tga", buffer.Ptr(), c, "fs_savepath" );
}

/*
==================
 R_EnvShot_f

	envshot <basename>

	Saves out env/<basename>_ft.tga, etc
==================
*/
static void R_EnvShot_f( const idCmdArgs& args )
{
	idStr		fullname;
	const char*	baseName;
	int			i;
	idMat3		axis[ 6 ], oldAxis;
	renderView_t	ref;
	idRenderView	primary;
	int			blends;
	const char*  extension;
	int			size;
	int         res_w, res_h, old_fov_x, old_fov_y;

	res_w = renderSystem->GetWidth();
	res_h = renderSystem->GetHeight();

	if( args.Argc() != 2 && args.Argc() != 3 && args.Argc() != 4 )
	{
		common->Printf( "USAGE: envshot <basename> [size] [blends]\n" );
		return;
	}
	baseName = args.Argv( 1 );

	blends = 1;
	if( args.Argc() == 4 )
	{
		size = atoi( args.Argv( 2 ) );
		blends = atoi( args.Argv( 3 ) );
	}
	else if( args.Argc() == 3 )
	{
		size = atoi( args.Argv( 2 ) );
		blends = 1;
	}
	else
	{
		size = 256;
		blends = 1;
	}

	if( !tr.primaryView )
	{
		common->Printf( "No primary view.\n" );
		return;
	}

	primary = *tr.primaryView;

	memset( &axis, 0, sizeof( axis ) );

	// +X
	axis[ 0 ][ 0 ][ 0 ] = 1;
	axis[ 0 ][ 1 ][ 2 ] = 1;
	axis[ 0 ][ 2 ][ 1 ] = 1;

	// -X
	axis[ 1 ][ 0 ][ 0 ] = -1;
	axis[ 1 ][ 1 ][ 2 ] = -1;
	axis[ 1 ][ 2 ][ 1 ] = 1;

	// +Y
	axis[ 2 ][ 0 ][ 1 ] = 1;
	axis[ 2 ][ 1 ][ 0 ] = -1;
	axis[ 2 ][ 2 ][ 2 ] = -1;

	// -Y
	axis[ 3 ][ 0 ][ 1 ] = -1;
	axis[ 3 ][ 1 ][ 0 ] = -1;
	axis[ 3 ][ 2 ][ 2 ] = 1;

	// +Z
	axis[ 4 ][ 0 ][ 2 ] = 1;
	axis[ 4 ][ 1 ][ 0 ] = -1;
	axis[ 4 ][ 2 ][ 1 ] = 1;

	// -Z
	axis[ 5 ][ 0 ][ 2 ] = -1;
	axis[ 5 ][ 1 ][ 0 ] = 1;
	axis[ 5 ][ 2 ][ 1 ] = 1;

	// let's get the game window to a "size" resolution
	if( ( res_w != size ) || ( res_h != size ) )
	{
		cvarSystem->SetCVarInteger( "r_windowWidth", size );
		cvarSystem->SetCVarInteger( "r_windowHeight", size );
		R_SetNewMode( false ); // the same as "vid_restart"
	} // FIXME that's a hack!!

	  // so we return to that axis and fov after the fact.
	oldAxis = primary.GetAxis();
	old_fov_x = primary.GetFOVx();
	old_fov_y = primary.GetFOVy();

	for( i = 0; i < 6; i++ )
	{
		ref = primary.GetParms();

		extension = envDirection[ i ];

		ref.fov_x = ref.fov_y = 90;
		ref.viewaxis = axis[ i ];
		fullname.Format( "env/%s%s", baseName, extension );

		tr.TakeScreenshot( size, size, fullname, blends, &ref, TGA );
	}

	// restore the original resolution, axis and fov
	ref.viewaxis = oldAxis;
	ref.fov_x = old_fov_x;
	ref.fov_y = old_fov_y;
	cvarSystem->SetCVarInteger( "r_windowWidth", res_w );
	cvarSystem->SetCVarInteger( "r_windowHeight", res_h );
	R_SetNewMode( false ); // the same as "vid_restart"

	common->Printf( "Wrote a env set with the name %s\n", baseName );
}


/*
==============================================================================

	AMBIENT SHOTS

==============================================================================
*/

static idMat3	cubeAxis[ 6 ];

/*
==================
 R_SampleCubeMap
==================
*/
static void R_SampleCubeMap( const idVec3& dir, int size, byte* buffers[ 6 ], byte result[ 4 ] )
{
	float	adir[ 3 ];
	int		axis, x, y;

	adir[ 0 ] = fabs( dir[ 0 ] );
	adir[ 1 ] = fabs( dir[ 1 ] );
	adir[ 2 ] = fabs( dir[ 2 ] );

	if( dir[ 0 ] >= adir[ 1 ] && dir[ 0 ] >= adir[ 2 ] )
	{
		axis = 0;
	}
	else if( -dir[ 0 ] >= adir[ 1 ] && -dir[ 0 ] >= adir[ 2 ] )
	{
		axis = 1;
	}
	else if( dir[ 1 ] >= adir[ 0 ] && dir[ 1 ] >= adir[ 2 ] )
	{
		axis = 2;
	}
	else if( -dir[ 1 ] >= adir[ 0 ] && -dir[ 1 ] >= adir[ 2 ] )
	{
		axis = 3;
	}
	else if( dir[ 2 ] >= adir[ 1 ] && dir[ 2 ] >= adir[ 2 ] )
	{
		axis = 4;
	}
	else
	{
		axis = 5;
	}

	float	fx = ( dir * cubeAxis[ axis ][ 1 ] ) / ( dir * cubeAxis[ axis ][ 0 ] );
	float	fy = ( dir * cubeAxis[ axis ][ 2 ] ) / ( dir * cubeAxis[ axis ][ 0 ] );

	fx = -fx;
	fy = -fy;
	x = size * 0.5 * ( fx + 1 );
	y = size * 0.5 * ( fy + 1 );
	if( x < 0 )
	{
		x = 0;
	}
	else if( x >= size )
	{
		x = size - 1;
	}
	if( y < 0 )
	{
		y = 0;
	}
	else if( y >= size )
	{
		y = size - 1;
	}

	result[ 0 ] = buffers[ axis ][ ( y * size + x ) * 4 + 0 ];
	result[ 1 ] = buffers[ axis ][ ( y * size + x ) * 4 + 1 ];
	result[ 2 ] = buffers[ axis ][ ( y * size + x ) * 4 + 2 ];
	result[ 3 ] = buffers[ axis ][ ( y * size + x ) * 4 + 3 ];
}

class CommandlineProgressBar {
private:
	size_t tics = 0;
	size_t nextTicCount = 0;
	int	count = 0;
	int expectedCount = 0;

public:
	CommandlineProgressBar( int _expectedCount )
	{
		expectedCount = _expectedCount;

		common->Printf( "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
		common->Printf( "|----|----|----|----|----|----|----|----|----|----|\n" );

		common->UpdateScreen( false );
	}

	void Increment()
	{
		if( ( count + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( count + 1 ) / expectedCount ) * 50.0 );

			do
			{
				common->Printf( "*" );
			} while( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * expectedCount );
			if( count == ( expectedCount - 1 ) )
			{
				if( tics < 51 )
				{
					common->Printf( "*" );
				}
				common->Printf( "\n" );
			}

			common->UpdateScreen( false );
		}

		count++;
	}
};


// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html

// To implement the Hammersley point set we only need an efficent way to implement the Van der Corput radical inverse phi2(i).
// Since it is in base 2 we can use some basic bit operations to achieve this.
// The brilliant book Hacker's Delight [warren01] provides us a a simple way to reverse the bits in a given 32bit integer. Using this, the following code then implements phi2(i)

/*
GLSL version

float radicalInverse_VdC( uint bits )
{
bits = (bits << 16u) | (bits >> 16u);
bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
*/

// RB: radical inverse implementation from the Mitsuba PBR system

// Van der Corput radical inverse in base 2 with single precision
inline float RadicalInverse_VdC( uint32_t n, uint32_t scramble = 0U )
{
	/* Efficiently reverse the bits in 'n' using binary operations */
#if (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))) || defined(__clang__)
	n = __builtin_bswap32( n );
#else
	n = ( n << 16 ) | ( n >> 16 );
	n = ( ( n & 0x00ff00ff ) << 8 ) | ( ( n & 0xff00ff00 ) >> 8 );
#endif
	n = ( ( n & 0x0f0f0f0f ) << 4 ) | ( ( n & 0xf0f0f0f0 ) >> 4 );
	n = ( ( n & 0x33333333 ) << 2 ) | ( ( n & 0xcccccccc ) >> 2 );
	n = ( ( n & 0x55555555 ) << 1 ) | ( ( n & 0xaaaaaaaa ) >> 1 );

	// Account for the available precision and scramble
	n = ( n >> ( 32 - 24 ) ) ^ ( scramble & ~- ( 1 << 24 ) );

	return ( float )n / ( float )( 1U << 24 );
}

// The ith point xi is then computed by
inline idVec2 Hammersley2D( uint i, uint N )
{
	return idVec2( float( i ) / float( N ), RadicalInverse_VdC( i ) );
}

static idVec3 ImportanceSampleGGX( const idVec2& Xi, float roughness, const idVec3& N )
{
	float a = roughness * roughness;

	// cosinus distributed direction (Z-up or tangent space) from the hammersley point xi
	float Phi = 2 * idMath::PI * Xi.x;
	float cosTheta = sqrt( ( 1 - Xi.y ) / ( 1 + ( a * a - 1 ) * Xi.y ) );
	float sinTheta = sqrt( 1 - cosTheta * cosTheta );

	idVec3 H;
	H.x = sinTheta * cos( Phi );
	H.y = sinTheta * sin( Phi );
	H.z = cosTheta;

	// rotate from tangent space to world space along N
	idVec3 upVector = abs( N.z ) < 0.999f ? idVec3( 0, 0, 1 ) : idVec3( 1, 0, 0 );
	idVec3 tangentX = upVector.Cross( N );
	tangentX.Normalize();
	idVec3 tangentY = N.Cross( tangentX );

	return tangentX * H.x + tangentY * H.y + N * H.z;
}

/*
==================
 R_MakeAmbientMap_f

	R_MakeAmbientMap_f <basename> [size]

	Saves out env/<basename>_amb_ft.tga, etc
==================
*/
static void R_MakeAmbientMap_f( const idCmdArgs& args )
{
	idStr fullname;
	const char*	baseName;
	int			i;
	renderView_t	ref;
	idRenderView	primary;
	int			downSample;
	int			outSize;
	byte*		buffers[ 6 ];
	int			width = 0, height = 0;

	if( args.Argc() != 2 && args.Argc() != 3 )
	{
		common->Printf( "USAGE: ambientshot <basename> [size]\n" );
		return;
	}
	baseName = args.Argv( 1 );

	downSample = 0;
	if( args.Argc() == 3 )
	{
		outSize = atoi( args.Argv( 2 ) );
	}
	else
	{
		outSize = 32;
	}

	memset( &cubeAxis, 0, sizeof( cubeAxis ) );
	cubeAxis[ 0 ][ 0 ][ 0 ] = 1;
	cubeAxis[ 0 ][ 1 ][ 2 ] = 1;
	cubeAxis[ 0 ][ 2 ][ 1 ] = 1;

	cubeAxis[ 1 ][ 0 ][ 0 ] = -1;
	cubeAxis[ 1 ][ 1 ][ 2 ] = -1;
	cubeAxis[ 1 ][ 2 ][ 1 ] = 1;

	cubeAxis[ 2 ][ 0 ][ 1 ] = 1;
	cubeAxis[ 2 ][ 1 ][ 0 ] = -1;
	cubeAxis[ 2 ][ 2 ][ 2 ] = -1;

	cubeAxis[ 3 ][ 0 ][ 1 ] = -1;
	cubeAxis[ 3 ][ 1 ][ 0 ] = -1;
	cubeAxis[ 3 ][ 2 ][ 2 ] = 1;

	cubeAxis[ 4 ][ 0 ][ 2 ] = 1;
	cubeAxis[ 4 ][ 1 ][ 0 ] = -1;
	cubeAxis[ 4 ][ 2 ][ 1 ] = 1;

	cubeAxis[ 5 ][ 0 ][ 2 ] = -1;
	cubeAxis[ 5 ][ 1 ][ 0 ] = 1;
	cubeAxis[ 5 ][ 2 ][ 1 ] = 1;

	// read all of the images
	for( i = 0; i < 6; i++ )
	{
		fullname.Format( "env/%s%s.%s", baseName, envDirection[ i ], fileExten[ TGA ] );
		common->Printf( "loading %s\n", fullname.c_str() );
		const bool captureToImage = false;
		common->UpdateScreen( captureToImage );
		R_LoadImage( fullname, &buffers[ i ], &width, &height, NULL, true );
		if( !buffers[ i ] )
		{
			common->Printf( "failed.\n" );
			for( i--; i >= 0; i-- )
			{
				Mem_Free( buffers[ i ] );
			}
			return;
		}
	}

	bool pacifier = true;

	// resample with hemispherical blending
	int	samples = 1000;

	byte*	outBuffer = ( byte* )_alloca( outSize * outSize * 4 );

	for( int map = 0; map < 2; map++ )
	{
		CommandlineProgressBar progressBar( outSize * outSize * 6 );

		int	start = Sys_Milliseconds();

		for( i = 0; i < 6; i++ )
		{
			for( int x = 0; x < outSize; x++ )
			{
				for( int y = 0; y < outSize; y++ )
				{
					idVec3	dir;
					float	total[ 3 ];

					dir = cubeAxis[ i ][ 0 ] + -( -1 + 2.0 * x / ( outSize - 1 ) ) * cubeAxis[ i ][ 1 ] + -( -1 + 2.0 * y / ( outSize - 1 ) ) * cubeAxis[ i ][ 2 ];
					dir.Normalize();
					total[ 0 ] = total[ 1 ] = total[ 2 ] = 0;

					float roughness = map ? 0.1 : 0.95;		// small for specular, almost hemisphere for ambient

					for( int s = 0; s < samples; s++ )
					{
						idVec2 Xi = Hammersley2D( s, samples );
						idVec3 test = ImportanceSampleGGX( Xi, roughness, dir );

						byte	result[ 4 ];
						//test = dir;
						R_SampleCubeMap( test, width, buffers, result );
						total[ 0 ] += result[ 0 ];
						total[ 1 ] += result[ 1 ];
						total[ 2 ] += result[ 2 ];
					}
					outBuffer[ ( y * outSize + x ) * 4 + 0 ] = total[ 0 ] / samples;
					outBuffer[ ( y * outSize + x ) * 4 + 1 ] = total[ 1 ] / samples;
					outBuffer[ ( y * outSize + x ) * 4 + 2 ] = total[ 2 ] / samples;
					outBuffer[ ( y * outSize + x ) * 4 + 3 ] = 255;

					progressBar.Increment();
				}
			}

			if( map == 0 )
			{
				fullname.Format( "env/%s_amb%s.%s", baseName, envDirection[ i ], fileExten[ PNG ] );
			}
			else
			{
				fullname.Format( "env/%s_spec%s.%s", baseName, envDirection[ i ], fileExten[ PNG ] );
			}
			//common->Printf( "writing %s\n", fullname.c_str() );

			const bool captureToImage = false;
			common->UpdateScreen( captureToImage );

			//R_WriteTGA( fullname, outBuffer, outSize, outSize, false, "fs_basepath" );
			R_WritePNG( fullname, outBuffer, 4, outSize, outSize, true, "fs_basepath" );
		}

		int	end = Sys_Milliseconds();

		if( map == 0 )
		{
			common->Printf( "env/%s_amb convolved  in %5.1f seconds\n\n", baseName, ( end - start ) * 0.001f );
		}
		else
		{
			common->Printf( "env/%s_spec convolved  in %5.1f seconds\n\n", baseName, ( end - start ) * 0.001f );
		}
	}

	for( i = 0; i < 6; i++ )
	{
		if( buffers[ i ] )
		{
			Mem_Free( buffers[ i ] );
		}
	}
}

//============================================================================

static void R_TransformCubemap( const char* orgDirection[ 6 ], const char* orgDir, const char* destDirection[ 6 ], const char* destDir, const char* baseName )
{
	idStr fullname;
	int			i;
	bool        errorInOriginalImages = false;
	byte*		buffers[ 6 ];
	int			width = 0, height = 0;

	for( i = 0; i < 6; i++ )
	{
		// read every image images
		fullname.Format( "%s/%s%s.%s", orgDir, baseName, orgDirection[ i ], fileExten[ TGA ] );
		common->Printf( "loading %s\n", fullname.c_str() );
		const bool captureToImage = false;
		common->UpdateScreen( captureToImage );
		R_LoadImage( fullname, &buffers[ i ], &width, &height, NULL, true );

		//check if the buffer is troublesome
		if( !buffers[ i ] )
		{
			common->Printf( "failed.\n" );
			errorInOriginalImages = true;
		}
		else if( width != height )
		{
			common->Printf( "wrong size pal!\n\n\nget your shit together and set the size according to your images!\n\n\ninept programmers are inept!\n" );
			errorInOriginalImages = true; // yeah, but don't just choke on a joke!
		}
		else
		{
			errorInOriginalImages = false;
		}

		if( errorInOriginalImages )
		{
			errorInOriginalImages = false;
			for( i--; i >= 0; i-- )
			{
				Mem_Free( buffers[ i ] ); // clean up every buffer from this stage down
			}

			return;
		}

		// apply rotations and flips
		R_ApplyCubeMapTransforms( i, buffers[ i ], width );

		//save the images with the appropiate skybox naming convention
		fullname.Format( "%s/%s/%s%s.%s", destDir, baseName, baseName, destDirection[ i ], fileExten[ TGA ] );
		common->Printf( "writing %s\n", fullname.c_str() );
		common->UpdateScreen( false );
		R_WriteTGA( fullname, buffers[ i ], width, width, false, "fs_basepath" );
	}

	for( i = 0; i < 6; i++ )
	{
		if( buffers[ i ] )
		{
			Mem_Free( buffers[ i ] );
		}
	}
}

/*
==================
 R_TransformEnvToSkybox_f

	R_TransformEnvToSkybox_f <basename>

	transforms env textures (of the type px, py, pz, nx, ny, nz)
	to skybox textures ( forward, back, left, right, up, down)
==================
*/
static void R_TransformEnvToSkybox_f( const idCmdArgs& args )
{

	if( args.Argc() != 2 )
	{
		common->Printf( "USAGE: envToSky <basename>\n" );
		return;
	}

	R_TransformCubemap( envDirection, "env", skyDirection, "skybox", args.Argv( 1 ) );
}

/*
==================
 R_TransformSkyboxToEnv_f

	R_TransformSkyboxToEnv_f <basename>

	transforms skybox textures ( forward, back, left, right, up, down)
	to env textures (of the type px, py, pz, nx, ny, nz)
==================
*/

static void R_TransformSkyboxToEnv_f( const idCmdArgs& args )
{

	if( args.Argc() != 2 )
	{
		common->Printf( "USAGE: skyToEnv <basename>\n" );
		return;
	}

	R_TransformCubemap( skyDirection, "skybox", envDirection, "env", args.Argv( 1 ) );
}

/*
=================
R_InitCommands
=================
*/
void R_InitCommands()
{
	extern void GfxInfo_f( const idCmdArgs& );
	extern void R_VidRestart_f( const idCmdArgs& );

	cmdSystem->AddCommand( "gfxInfo", GfxInfo_f, CMD_FL_RENDERER, "show graphics info" );
	cmdSystem->AddCommand( "vid_restart", R_VidRestart_f, CMD_FL_RENDERER, "restarts renderSystem" );
	cmdSystem->AddCommand( "listModes", R_ListModes_f, CMD_FL_RENDERER, "lists all video modes" );
	cmdSystem->AddCommand( "sizeUp", R_SizeUp_f, CMD_FL_RENDERER, "makes the rendered view larger" );
	cmdSystem->AddCommand( "sizeDown", R_SizeDown_f, CMD_FL_RENDERER, "makes the rendered view smaller" );

	cmdSystem->AddCommand( "reloadGuis", R_ReloadGuis_f, CMD_FL_RENDERER, "reloads guis" );
	cmdSystem->AddCommand( "listGuis", R_ListGuis_f, CMD_FL_RENDERER, "lists guis" );
	cmdSystem->AddCommand( "touchGui", R_TouchGui_f, CMD_FL_RENDERER, "touches a gui" );

	cmdSystem->AddCommand( "screenshot", R_ScreenShot_f, CMD_FL_RENDERER, "takes a screenshot" );
	cmdSystem->AddCommand( "envshot", R_EnvShot_f, CMD_FL_RENDERER, "takes an environment shot" );
	cmdSystem->AddCommand( "makeAmbientMap", R_MakeAmbientMap_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "makes an ambient map" );
	cmdSystem->AddCommand( "envToSky", R_TransformEnvToSkybox_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "transforms environment textures to sky box textures" );
	cmdSystem->AddCommand( "skyToEnv", R_TransformSkyboxToEnv_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "transforms sky box textures to environment textures" );
	cmdSystem->AddCommand( "modulateLights", R_ModulateLights_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "modifies shader parms on all lights" );
	cmdSystem->AddCommand( "testImage", R_TestImage_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "displays the given image centered on screen", idCmdSystem::ArgCompletion_ImageName );
	cmdSystem->AddCommand( "testVideo", R_TestVideo_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "displays the given cinematic", idCmdSystem::ArgCompletion_VideoName );
	cmdSystem->AddCommand( "reportSurfaceAreas", R_ReportSurfaceAreas_f, CMD_FL_RENDERER, "lists all used materials sorted by surface area" );
	cmdSystem->AddCommand( "showInteractionMemory", R_ShowInteractionMemory_f, CMD_FL_RENDERER, "shows memory used by interactions" );
	cmdSystem->AddCommand( "listRenderEntityDefs", R_ListRenderEntityDefs_f, CMD_FL_RENDERER, "lists the entity defs" );
	cmdSystem->AddCommand( "listRenderLightDefs", R_ListRenderLightDefs_f, CMD_FL_RENDERER, "lists the light defs" );
	cmdSystem->AddCommand( "reloadSurface", R_ReloadSurface_f, CMD_FL_RENDERER, "reloads the decl and images for selected surface" );
}