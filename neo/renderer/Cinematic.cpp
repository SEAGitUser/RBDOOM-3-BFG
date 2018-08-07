/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2014 Robert Beckebans
Copyright (C) 2014 Carl Kenner

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

/*
=====================================================================================================

	idRenderCinematicManager

=====================================================================================================
*/

extern void InitCinematicBIK();
extern void ShutdownCinematicBIK();
extern idCinematic * InitROQFromFile( const char * fileName, bool looping );

extern void InitCinematicROQ();
extern void ShutdownCinematicROQ();
extern idCinematic * InitBIKFromFile( const char * fileName, bool looping );

class idRenderCinematicManagerLocal : public idRenderCinematicManager {
public:
	virtual void			Init() final;
	virtual void			Shutdown() final;
	virtual idCinematic *	CreateFromFile( const char* path, bool looping ) final;
};

idRenderCinematicManagerLocal cinMgrLocal;
idRenderCinematicManager * renderCinematicManager = &cinMgrLocal;

/*
==============
 idRenderCinematicManagerLocal::InitCinematic
==============
*/
void idRenderCinematicManagerLocal::Init()
{
	InitCinematicBIK();
	InitCinematicROQ();
}

/*
==============
 idRenderCinematicManagerLocal::ShutdownCinematic
==============
*/
void idRenderCinematicManagerLocal::Shutdown()
{
	ShutdownCinematicROQ();
	ShutdownCinematicBIK();
}

/*
==============
 idRenderCinematicManagerLocal::CreateFromFile
==============
*/
idCinematic * idRenderCinematicManagerLocal::CreateFromFile( const char* qpath, bool amilooping )
{
	idCinematic * cin = nullptr;
	idStrStatic< MAX_PATH > fileName;

	// if no folder is specified, look in the video folder
	if( idStr::FindChar( qpath, '/' ) == idStr::INVALID_POSITION &&
		idStr::FindString( qpath, "\\" ) == NULL )
	{
		fileName.Format( "video/%s", qpath );
	}
	else {
		fileName = qpath;
	}

	// Carl: Look for original Doom 3 RoQ files first:
	fileName.SetFileExtension( ".roq" );

	//if( fileName == "video\\loadvideo.roq" ) {
	//	fileName = "video\\idlogo.roq";
	//}

	cin = InitROQFromFile( fileName, amilooping );
	if( !cin ) // Carl: If the RoQ file doesn't exist, try using ffmpeg instead:
	{
		common->DPrintf( "RoQ file not found: '%s'\n", fileName.c_str() );

		fileName.SetFileExtension( ".bik" );
		common->DPrintf( "Trying BIK filename: '%s'\n", fileName.c_str() );

		cin = InitBIKFromFile( fileName, amilooping );
		if( !cin ) // Carl: If the RoQ file doesn't exist, try using ffmpeg instead:
		{
			common->DPrintf( "BIK file not found: '%s'\n", fileName.c_str() );
		}
	}

	return cin;
}

/*
=====================================================================================================

	idSndWindow

=====================================================================================================
*/

/*
==============
idSndWindow::InitFromFile
==============
*/
bool idSndWindow::InitFromFile( const char* qpath, bool looping )
{
	idStr fname = qpath;

	fname.ToLower();
	if( !fname.Icmp( "waveform" ) )
	{
		showWaveform = true;
	}
	else {
		showWaveform = false;
	}
	return true;
}

/*
==============
idSndWindow::ImageForTime
==============
*/
cinData_t idSndWindow::ImageForTime( int milliseconds )
{
	return soundSystem->ImageForTime( milliseconds, showWaveform );
}