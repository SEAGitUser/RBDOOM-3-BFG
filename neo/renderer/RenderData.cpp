
#pragma hdrstop
#include "precompiled.h"

#include "tr_local.h"

/*
==========================================================================================

	FRAME MEMORY ALLOCATION

==========================================================================================
*/

idRenderAllocManager allocManager;

/*
====================
R_ToggleSmpFrame
====================
*/
void idRenderAllocManager::ToggleFrame()
{
	// update the highwater mark
	if( currentFrameData->frameMemoryAllocated.GetValue() > currentFrameData->highWaterAllocated )
	{
		currentFrameData->highWaterAllocated = currentFrameData->frameMemoryAllocated.GetValue();
	#if defined( TRACK_FRAME_ALLOCS )
		currentFrameData->highWaterUsed = currentFrameData->frameMemoryUsed.GetValue();
		for( int i = 0; i < FRAME_ALLOC_MAX; i++ )
		{
			frameHighWaterTypeCount[ i ] = frameAllocTypeCount[ i ].GetValue();
		}
	#endif
	}

	// switch to the next frame
	currentFrame++;
	currentFrameData = &frameData[ currentFrame % NUM_FRAME_DATA ];

	// reset the memory allocation

	// RB: 64 bit fixes, changed unsigned int to uintptr_t
	const uintptr_t bytesNeededForAlignment = FRAME_ALLOC_ALIGNMENT - ( ( uintptr_t )currentFrameData->frameMemory & ( FRAME_ALLOC_ALIGNMENT - 1 ) );
	// RB end

	currentFrameData->frameMemoryAllocated.SetValue( bytesNeededForAlignment );
	currentFrameData->frameMemoryUsed.SetValue( 0 );

#if defined( TRACK_FRAME_ALLOCS )
	for( int i = 0; i < FRAME_ALLOC_MAX; i++ )
	{
		frameAllocTypeCount[ i ].SetValue( 0 );
	}
#endif

	// clear the command chain and make a RC_NOP command the only thing on the list
	currentFrameData->cmdHead = currentFrameData->cmdTail = ( emptyCommand_t* )_FrameAlloc( sizeof( *currentFrameData->cmdHead ), FRAME_ALLOC_DRAW_COMMAND );
	currentFrameData->cmdHead->commandId = RC_NOP;
	currentFrameData->cmdHead->next = NULL;
}

/*
=====================
R_ShutdownFrameData
=====================
*/
void idRenderAllocManager::ShutdownFrameData()
{
	currentFrameData = NULL;
	for( int i = 0; i < NUM_FRAME_DATA; i++ )
	{
		Mem_Free16( frameData[ i ].frameMemory );
		frameData[ i ].frameMemory = NULL;
	}
}

/*
=====================
R_InitFrameData
=====================
*/
void idRenderAllocManager::InitFrameData()
{
	ShutdownFrameData();

	for( int i = 0; i < NUM_FRAME_DATA; i++ )
	{
		frameData[ i ].frameMemory = ( byte* )Mem_Alloc16( MAX_FRAME_MEMORY, TAG_RENDER );
	}

	// must be set before calling R_ToggleSmpFrame()
	currentFrameData = &frameData[ 0 ];

	ToggleFrame();
}

/*
================
R_FrameAlloc

	This data will be automatically freed when the
	current frame's back end completes.

	This should only be called by the front end.  The
	back end shouldn't need to allocate memory.

	All temporary data, like dynamic tesselations
	and local spaces are allocated here.

	All memory is cache-line-cleared for the best performance.
================
*/
void * idRenderAllocManager::_FrameAlloc( int bytes, const frameAllocType_t type )
{
#if defined( TRACK_FRAME_ALLOCS )
	currentFrameData->frameMemoryUsed.Add( bytes );
	frameAllocTypeCount[ type ].Add( bytes );
#endif
	bytes = ALIGN( bytes, FRAME_ALLOC_ALIGNMENT );

	// thread safe add
	int	end = currentFrameData->frameMemoryAllocated.Add( bytes );
	if( end > MAX_FRAME_MEMORY )
	{
		idLib::Error( "R_FrameAlloc ran out of memory. bytes = %d, end = %d, highWaterAllocated = %d\n", bytes, end, currentFrameData->highWaterAllocated );
	}

	byte* ptr = currentFrameData->frameMemory + end - bytes;

	// cache line clear the memory
	for( int offset = 0; offset < bytes; offset += CACHE_LINE_SIZE )
	{
		ZeroCacheLine( ptr, offset );
	}

	return ptr;
}

/*
==================
R_ClearedFrameAlloc
==================
*/
void * idRenderAllocManager::_ClearedFrameAlloc( int bytes, const frameAllocType_t type )
{
	// NOTE: every allocation is cache line cleared
	return _FrameAlloc( bytes, type );
}

/*
==========================================================================================

	FONT-END STATIC MEMORY ALLOCATION

==========================================================================================
*/

/*
=================
R_StaticAlloc
=================
*/
void* idRenderAllocManager::_StaticAlloc( int bytes, const memTag_t tag )
{
	tr.pc.c_alloc++;

	void* buf = Mem_Alloc( bytes, tag );

	// don't exit on failure on zero length allocations since the old code didn't
	if( buf == NULL && bytes != 0 )
	{
		common->FatalError( "R_StaticAlloc failed on %i bytes", bytes );
	}
	return buf;
}

/*
=================
R_ClearedStaticAlloc
=================
*/
void* idRenderAllocManager::_ClearedStaticAlloc( int bytes, const memTag_t tag )
{
	void* buf = _StaticAlloc( bytes, tag );
	memset( buf, 0, bytes );
	return buf;
}

/*
=================
R_StaticFree
=================
*/
void idRenderAllocManager::_StaticFree( void* data )
{
	tr.pc.c_free++;
	Mem_Free( data );
}


/*
============
 R_GetCommandBuffer

	Returns memory for a command buffer (stretchPicCommand_t,
	drawSurfsCommand_t, etc) and links it to the end of the
	current command chain.
============
*/
void * idRenderAllocManager::_GetCommandBuffer( int bytes )
{
	emptyCommand_t* cmd = ( emptyCommand_t* )_FrameAlloc( bytes, FRAME_ALLOC_DRAW_COMMAND );

	cmd->next = nullptr;
	currentFrameData->cmdTail->next = &cmd->commandId;
	currentFrameData->cmdTail = cmd;

	return ( void* )cmd;
}


