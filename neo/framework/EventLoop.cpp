/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

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

#include "precompiled.h"
#pragma hdrstop

idCVar idEventLoop::com_journal( "com_journal", "0", CVAR_INIT | CVAR_SYSTEM, "1 = record journal, 2 = play back journal", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );

idEventLoop eventLoopLocal;
idEventLoop* eventLoop = &eventLoopLocal;


/*
=================
idEventLoop::idEventLoop
=================
*/
idEventLoop::idEventLoop()
{
	com_journalFile = NULL;
	com_journalDataFile = NULL;
	initialTimeOffset = 0;
}

/*
=================
idEventLoop::~idEventLoop
=================
*/
idEventLoop::~idEventLoop()
{
}

/*
=================
idEventLoop::GetRealEvent
=================
*/
idSysEvent	idEventLoop::GetRealEvent()
{
	idSysEvent	ev;
	
	// either get an event from the system or the journal file
	if( com_journal.GetInteger() == 2 )
	{
		if( !ev.Read( com_journalFile ) )
		{
			common->FatalError( "Error reading from journal file" );
		}
	}
	else {
		ev = sys->GetEvent();
		
		// write the journal value out if needed
		if( com_journal.GetInteger() == 1 )
		{
			if( !ev.Write( com_journalFile ) )
			{
				common->FatalError( "Error writing to journal file" );
			}
		}
	}
	
	return ev;
}

/*
=================
idEventLoop::PushEvent
=================
*/
void idEventLoop::PushEvent( idSysEvent* event )
{
	static bool printedWarning;
	
	auto ev = &com_pushedEvents[ com_pushedEventsHead & ( MAX_PUSHED_EVENTS - 1 ) ];
	
	if( com_pushedEventsHead - com_pushedEventsTail >= MAX_PUSHED_EVENTS )
	{	
		// don't print the warning constantly, or it can give time for more...
		if( !printedWarning )
		{
			printedWarning = true;
			common->Printf( "WARNING: Com_PushEvent overflow\n" );
		}
		ev->FreeData();

		com_pushedEventsTail++;
	}
	else {
		printedWarning = false;
	}
	
	*ev = *event;
	com_pushedEventsHead++;
}

/*
=================
idEventLoop::GetEvent
=================
*/
idSysEvent idEventLoop::GetEvent()
{
	if( com_pushedEventsHead > com_pushedEventsTail )
	{
		com_pushedEventsTail++;
		return com_pushedEvents[( com_pushedEventsTail - 1 ) & ( MAX_PUSHED_EVENTS - 1 ) ];
	}
	return GetRealEvent();
}

/*
=================
idEventLoop::ProcessEvent
=================
*/
void idEventLoop::ProcessEvent( idSysEvent & ev )
{
	// track key up / down states
	if( ev.IsKeyEvent() )
	{
		idKeyInput::PreliminaryKeyEvent( ev.GetKey(), ev.IsKeyDown() );
	}
	
	if( ev.IsConsoleEvent() )
	{
		// from a text console outside the game window
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, ev.GetCommand() );
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "\n" );
	}
	else {
		common->ProcessEvent( &ev );
	}
	
	ev.FreeData();
}

/*
===============
idEventLoop::RunEventLoop
===============
*/
int idEventLoop::RunEventLoop( bool commandExecution )
{
	idSysEvent	ev;
	
	while( 1 )
	{	
		if( commandExecution )
		{
			// execute any bound commands before processing another event
			cmdSystem->ExecuteCommandBuffer();
		}
		
		ev = GetEvent();
		
		// if no more events are available
		if( ev.IsEmptyEvent() )
		{
			return 0;
		}

		ProcessEvent( ev );
	}
	
	return 0;	// never reached
}

/*
=============
idEventLoop::Init
=============
*/
void idEventLoop::Init()
{
	initialTimeOffset = sys->Milliseconds();
	
	common->StartupVariable( "journal" );
	
	if( com_journal.GetInteger() == 1 )
	{
		common->Printf( "Journaling events\n" );
		com_journalFile = fileSystem->OpenFileWrite( "journal.dat" );
		com_journalDataFile = fileSystem->OpenFileWrite( "journaldata.dat" );
	}
	else if( com_journal.GetInteger() == 2 )
	{
		common->Printf( "Replaying journaled events\n" );
		com_journalFile = fileSystem->OpenFileRead( "journal.dat" );
		com_journalDataFile = fileSystem->OpenFileRead( "journaldata.dat" );
	}
	
	if( !com_journalFile || !com_journalDataFile )
	{
		com_journal.SetInteger( 0 );
		com_journalFile = 0;
		com_journalDataFile = 0;
		common->Printf( "Couldn't open journal files\n" );
	}
}

/*
=============
idEventLoop::Shutdown
=============
*/
void idEventLoop::Shutdown()
{
	if( com_journalFile )
	{
		fileSystem->CloseFile( com_journalFile );
		com_journalFile = NULL;
	}
	if( com_journalDataFile )
	{
		fileSystem->CloseFile( com_journalDataFile );
		com_journalDataFile = NULL;
	}
}

/*
================
idEventLoop::Milliseconds

Can be used for profiling, but will be journaled accurately
================
*/
int idEventLoop::Milliseconds()
{
#if 1	// FIXME!
	return sys->Milliseconds() - initialTimeOffset;
#else
	idSysEvent	ev;
	
	// get events and push them until we get a null event with the current time
	do {	
		ev = Com_GetRealEvent();
		if( ev.evType != SE_NONE )
		{
			Com_PushEvent( &ev );
		}
	}
	while( ev.evType != SE_NONE );
	
	return ev.evTime;
#endif
}

/*
================
idEventLoop::JournalLevel
================
*/
int idEventLoop::JournalLevel() const
{
	return com_journal.GetInteger();
}
