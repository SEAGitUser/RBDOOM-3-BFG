/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
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

#ifndef __CINEMATIC_H__
#define __CINEMATIC_H__

/*
===============================================================================

	cinematic

	Multiple idCinematics can run simultaniously.
	A single idCinematic can be reused for multiple files if desired.

===============================================================================
*/

// cinematic states
enum cinStatus_t {
	FMV_IDLE,
	FMV_PLAY,			// play
	FMV_EOF,			// all other conditions, i.e. stop/EOF/abort
	FMV_ID_BLT,
	FMV_ID_IDLE,
	FMV_LOOPED,
	FMV_ID_WAIT
};

class idImage;

// a cinematic stream generates an image buffer, which the caller will upload to a texture
struct cinData_t {
	int					imageWidth, imageHeight;	// will be a power of 2
	idImage				*imageY, *imageCr, *imageCb;
	idImage				*image;
	int					status;
};

class idCinematic {
public:
	static const int	DEFAULT_CIN_WIDTH = 512;
	static const int	DEFAULT_CIN_HEIGHT = 512;

	// frees all allocated memory
	virtual				~idCinematic() {};

	// returns false if it failed to load
	virtual bool		InitFromFile( const char* qpath, bool looping ) = 0;

	// returns the length of the animation in milliseconds
	virtual int			AnimationLength() const = 0;

	// the pointers in cinData_t will remain valid until the next UpdateForTime() call
	virtual cinData_t	ImageForTime( int milliseconds ) = 0;

	// closes the file and frees all allocated memory
	virtual void		Close() = 0;

	// sets the cinematic to start at that time (can be in the past)
	virtual void		ResetTime( int time ) = 0;


	// gets the time the cinematic started
	virtual int			GetStartTime() const = 0;

	virtual void		ExportToTGA( bool skipExisting = true ) = 0;

	virtual float		GetFrameRate() const = 0;

	// let us know wether this video went EOF or is still active
	virtual bool        IsPlaying() const = 0;
};

/*
===============================================

	Sound meter.

===============================================
*/
class idSndWindow : public idCinematic {
public:

	idSndWindow() : showWaveform( false ) {}
	virtual ~idSndWindow() {}

	virtual bool		InitFromFile( const char* qpath, bool looping );
	virtual cinData_t	ImageForTime( int milliseconds );
	virtual int			AnimationLength() const { return -1; }

	virtual void		Close() {}
	virtual void		ResetTime( int milliseconds ) {}

	virtual int			GetStartTime() const { return -1; }
	virtual void		ExportToTGA( bool skipExisting ) {}
	virtual float		GetFrameRate() const { return 30.0f; }
	virtual bool		IsPlaying() const { return false; }

private:

	bool				showWaveform;
};

/*
===============================================

	idRenderCinematicManager

===============================================
*/
class idRenderCinematicManager {
public:
	// initialize cinematic play back data
	virtual void		Init() = 0;

	// shutdown cinematic play back data
	virtual void		Shutdown() = 0;

	// Allocates and returns a private subclass that implements the methods.
	// Returns nullptr if it failed to load.
	virtual idCinematic * CreateFromFile( const char* path, bool looping ) = 0;
};

extern idRenderCinematicManager * renderCinematicManager;

#endif /* !__CINEMATIC_H__ */
