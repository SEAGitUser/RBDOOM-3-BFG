/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013 Robert Beckebans

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

idCVar r_showBuffers( "r_showBuffers", "1", CVAR_INTEGER, "print creation info" );
// r_amdWorkaroundMapBuffersOnCreate
// r_useUnsyncBufferObjects

// glVBHeap
// glIBHeap
// glCBHeap
// glJBHeap
// glPPHeap
// glPUHeap
// glPMHeap

// bo_useUnsynchronizedPBO
// bo_useInvalidateRangePBO

//static const GLenum bufferUsage = GL_STATIC_DRAW;
static const GLenum bufferUsage = GL_DYNAMIC_DRAW;

// RB begin
#if defined(_WIN32)
/*
==================
IsWriteCombined
==================
*/
bool IsWriteCombined( void* base )
{
	MEMORY_BASIC_INFORMATION info;
	SIZE_T size = VirtualQueryEx( GetCurrentProcess(), base, &info, sizeof( info ) );
	if( size == 0 )
	{
		DWORD error = GetLastError();
		error = error;
		return false;
	}
	bool isWriteCombined = ( ( info.AllocationProtect & PAGE_WRITECOMBINE ) != 0 );
	return isWriteCombined;
}
#endif
// RB end

/*
===================================================================================
First, the frequency of access (modification and usage), and second, the nature of that access. 
	The frequency of access may be one of these:

STREAM
The data store contents will be modified once and used at most a few times.

STATIC
The data store contents will be modified once and used many times.

DYNAMIC
The data store contents will be modified repeatedly and used many times.

	The nature of access may be one of these:

DRAW
The data store contents are modified by the application, and used as the source for GL drawing and image specification commands.

READ
The data store contents are modified by reading data from the GL, and used to return that data when queried by the application.

COPY
The data store contents are modified by reading data from the GL, and used as the source for GL drawing and image specification commands.

===================================================================================
*/

static void * GL_CreateBuffer( const char *class_name, const void *class_ptr, GLenum target, bufferUsageType_t allocUsage, GLsizeiptr numBytes, const void * data )
{
	GLuint bufferObject = MAX_UNSIGNED_TYPE( GLuint );

	glGenBuffers( 1, &bufferObject );
	if( bufferObject == MAX_UNSIGNED_TYPE( GLuint ) )
	{
		idLib::FatalError( "GL_CreateBuffer: failed" );
	}
	glBindBuffer( target, bufferObject );

	if( GLEW_KHR_debug )
	{
		idStrStatic<128> name;
		name.Format<128>( "%s(%p.%u)", class_name, class_ptr, bufferObject );
		glObjectLabel( GL_BUFFER, bufferObject, name.Length(), name.c_str() );
	}

	if( GLEW_ARB_buffer_storage )
	{
		if( allocUsage == BU_STATIC )
		{
			glBufferStorage( target, numBytes, data, 0 );
		}
		else if( allocUsage == BU_DYNAMIC )
		{
			glBufferStorage( target, numBytes, GL_NONE, GL_DYNAMIC_STORAGE_BIT );
		}
		else if( allocUsage == BU_STAGING )
		{
			glBufferStorage( target, numBytes, GL_NONE, 0 );
		}
		// BU_DEFAULT
		glBufferStorage( target, numBytes, GL_NONE, GL_MAP_WRITE_BIT );
	}
	else
	{
		if( allocUsage == BU_STATIC )
		{
			glBufferData( target, numBytes, data, GL_STATIC_DRAW );
		}
		else if( allocUsage == BU_DYNAMIC )
		{
			glBufferData( target, numBytes, GL_NONE, GL_STREAM_DRAW );
		}
		else if( allocUsage == BU_STAGING )
		{
			glBufferData( target, numBytes, GL_NONE, GL_DYNAMIC_COPY ); // GL_STREAM_COPY // ???
		}
		// BU_DEFAULT
		glBufferData( target, numBytes, GL_NONE, GL_DYNAMIC_DRAW );
	}

	GLenum err = glGetError();
	if( err == GL_OUT_OF_MEMORY )
	{
		idLib::Warning( "GL_CreateBuffer(): allocation failed" );
		glBindBuffer( target, GL_NONE );
		glDeleteBuffers( 1, &bufferObject );
		return nullptr;
	}

	glBindBuffer( target, GL_NONE );

	return reinterpret_cast< void* >( bufferObject );
}

static GLbitfield GetMapFlags( bufferMapType_t mapType, bool bInvalidateRange )
{
	if( mapType == BM_READ )
	{
		return GL_MAP_READ_BIT;
	}
	else if( mapType == BM_READ_NOSYNC )
	{
		return GL_MAP_READ_BIT /*| GL_MAP_UNSYNCHRONIZED_BIT*/;
	}
	else if( mapType == BM_WRITE )
	{
		return GL_MAP_WRITE_BIT | ( bInvalidateRange ? GL_MAP_INVALIDATE_RANGE_BIT : 0 );
	}
	else if( mapType == BM_WRITE_NOSYNC )
	{
		return GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | ( bInvalidateRange ? GL_MAP_INVALIDATE_RANGE_BIT : 0 );
	}
	assert( false );
	return 0;
}

static void * GL_TryMapBufferRange( GLenum target, GLintptr bufferObject, GLintptr offset, GLsizeiptr size, GLbitfield flags )
{
	if( GLEW_EXT_direct_state_access )
	{
		return glMapNamedBufferRangeEXT( bufferObject, offset, size, flags );
	}

	glBindBuffer( target, bufferObject );
	return glMapBufferRange( target, offset, size, flags );
}

static bool GL_TryUnmap( GLenum target, GLintptr bufferObject )
{
	if( GLEW_EXT_direct_state_access )
	{
		return glUnmapNamedBufferEXT( bufferObject );
	}

	glBindBuffer( target, bufferObject );
	return glUnmapBuffer( target );
}

static void GL_UpdateSubData( GLenum target, GLintptr bufferObject, GLintptr offset, GLsizeiptr updateSize, const void *data )
{
	const GLsizeiptr numBytes = ALIGN( updateSize, 16 ); // common minimal alignment

	if( GLEW_EXT_direct_state_access )
	{
		glNamedBufferSubDataEXT( bufferObject, offset, numBytes, data );
	}
	else {
		glBindBuffer( target, bufferObject );
		glBufferSubData( target, offset, numBytes, data );
	}
}

class idBuffer {
public:
	idBuffer();
	~idBuffer();

	// Allocate or free the buffer.
	virtual bool		AllocBufferObject( int32 allocSize, bufferUsageType_t allocUsage, const void* data = nullptr );
	virtual void		FreeBufferObject();

	// Make this buffer a reference to another buffer.
	virtual void		Reference( const idBuffer& other );
	virtual void		Reference( const idBuffer& other, int32 refOffset, int32 refSize );

	// Copies data to the buffer. 'size' may be less than the originally allocated size.
	virtual void		Update( int32 updateSize, const void* data ) const;

	virtual void * 		MapBuffer( bufferMapType_t mapType ) const;
	///void * TryMapBuffer( bufferMapType_t mapType );
	void				UnmapBuffer() const;
	ID_INLINE bool		IsMapped() const		{ return ( size & MAPPED_FLAG ) != 0; }

	ID_INLINE int32		GetSize() const			{ return ( size & ~MAPPED_FLAG ); }
	ID_INLINE int32		GetAllocedSize() const	{ return ALIGN( GetSize(), 16 ); }
	ID_INLINE void * 	GetAPIObject() const	{ return apiObject; }
	ID_INLINE int32		GetOffset() const		{ return ( offsetInOtherBuffer & ~OWNS_BUFFER_FLAG ); }

protected:

	int32				size;					// size in bytes
	int32				offsetInOtherBuffer;	// offset in bytes
	void * 				apiObject;

	bufferUsageType_t	usage;

	static const int32	MAPPED_FLAG				= MAX_TYPE( int32 );
	static const int32	OWNS_BUFFER_FLAG		= MAX_TYPE( int32 );

private:

	void				ClearWithoutFreeing();
	ID_INLINE void		SetMapped() const		{ const_cast< int32& >( size ) |= MAPPED_FLAG; }
	ID_INLINE void		SetUnmapped() const		{ const_cast< int32& >( size ) &= ~MAPPED_FLAG; }
	ID_INLINE bool		OwnsBuffer() const		{ return ( ( offsetInOtherBuffer & OWNS_BUFFER_FLAG ) != 0 ); }

	DISALLOW_COPY_AND_ASSIGN( idBuffer );
};

/*
================================================================================================

	Buffer Objects

================================================================================================
*/

/*
========================
UnbindBufferObjects
========================
*/
void UnbindBufferObjects()
{
	glBindBuffer( GL_UNIFORM_BUFFER, 0 );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
}

#if defined(USE_INTRINSICS)

void CopyBuffer( byte* dst, const byte* src, int numBytes )
{
	assert_16_byte_aligned( dst );
	assert_16_byte_aligned( src );
	
	int i = 0;
	for( ; i + 128 <= numBytes; i += 128 )
	{
		__m128i d0 = _mm_load_si128( ( __m128i* )&src[i + 0 * 16] );
		__m128i d1 = _mm_load_si128( ( __m128i* )&src[i + 1 * 16] );
		__m128i d2 = _mm_load_si128( ( __m128i* )&src[i + 2 * 16] );
		__m128i d3 = _mm_load_si128( ( __m128i* )&src[i + 3 * 16] );
		__m128i d4 = _mm_load_si128( ( __m128i* )&src[i + 4 * 16] );
		__m128i d5 = _mm_load_si128( ( __m128i* )&src[i + 5 * 16] );
		__m128i d6 = _mm_load_si128( ( __m128i* )&src[i + 6 * 16] );
		__m128i d7 = _mm_load_si128( ( __m128i* )&src[i + 7 * 16] );
		_mm_stream_si128( ( __m128i* )&dst[i + 0 * 16], d0 );
		_mm_stream_si128( ( __m128i* )&dst[i + 1 * 16], d1 );
		_mm_stream_si128( ( __m128i* )&dst[i + 2 * 16], d2 );
		_mm_stream_si128( ( __m128i* )&dst[i + 3 * 16], d3 );
		_mm_stream_si128( ( __m128i* )&dst[i + 4 * 16], d4 );
		_mm_stream_si128( ( __m128i* )&dst[i + 5 * 16], d5 );
		_mm_stream_si128( ( __m128i* )&dst[i + 6 * 16], d6 );
		_mm_stream_si128( ( __m128i* )&dst[i + 7 * 16], d7 );
	}
	for( ; i + 16 <= numBytes; i += 16 )
	{
		__m128i d = _mm_load_si128( ( __m128i* )&src[i] );
		_mm_stream_si128( ( __m128i* )&dst[i], d );
	}
	for( ; i + 4 <= numBytes; i += 4 )
	{
		*( uint32* )&dst[i] = *( const uint32* )&src[i];
	}
	for( ; i < numBytes; i++ )
	{
		dst[i] = src[i];
	}
	_mm_sfence();
}

#else

void CopyBuffer( byte* dst, const byte* src, int numBytes )
{
	//assert_16_byte_aligned( dst );
	//assert_16_byte_aligned( src );
	memcpy( dst, src, numBytes );
}

#endif

/*
================================================================================================

	idVertexBuffer

================================================================================================
*/

/*
========================
idVertexBuffer::idVertexBuffer
========================
*/
idVertexBuffer::idVertexBuffer()
{
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
	SetUnmapped();
}

/*
========================
idVertexBuffer::~idVertexBuffer
========================
*/
idVertexBuffer::~idVertexBuffer()
{
	FreeBufferObject();
}

/*
========================
idVertexBuffer::AllocBufferObject
========================
*/
bool idVertexBuffer::AllocBufferObject( const void* data, int allocSize )
{
	assert( apiObject == NULL );
	assert_16_byte_aligned( data );
	
	if( allocSize <= 0 )
	{
		idLib::Error( "idVertexBuffer::AllocBufferObject: allocSize = %i", allocSize );
	}
	
	size = allocSize;

	int numBytes = GetAllocedSize();

	// clear out any previous error
	glGetError();

	// these are rewritten every frame
	apiObject = GL_CreateBuffer( "idVertexBuffer", this, GL_ARRAY_BUFFER, BU_DEFAULT, numBytes, NULL );

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "vertex buffer %p api %p, size %i ( alloced %i ) bytes\n", this, GetAPIObject(), GetSize(), GetAllocedSize() );
	}
	
	// copy the data
	if( data != NULL )
	{
		Update( data, allocSize );
	}
	
	return( apiObject != nullptr );
}

/*
========================
idVertexBuffer::FreeBufferObject
========================
*/
void idVertexBuffer::FreeBufferObject()
{
	if( IsMapped() )
	{
		UnmapBuffer();
	}
	
	// if this is a sub-allocation inside a larger buffer, don't actually free anything.
	if( OwnsBuffer() == false )
	{
		ClearWithoutFreeing();
		return;
	}
	
	if( apiObject == NULL )
	{
		return;
	}
	
	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "vertex buffer free %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize() );
	}
	
	// RB: 64 bit fixes, changed GLuint to GLintptrARB
	GLintptr bufferObject = reinterpret_cast< GLintptr >( apiObject );
	glDeleteBuffers( 1, ( const unsigned int* ) & bufferObject );
	// RB end
	
	ClearWithoutFreeing();
}

/*
========================
idVertexBuffer::Reference
========================
*/
void idVertexBuffer::Reference( const idVertexBuffer& other )
{
	assert( IsMapped() == false );
	//assert( other.IsMapped() == false );	// this happens when building idTriangles while at the same time setting up idDrawVerts
	assert( other.GetAPIObject() != NULL );
	assert( other.GetSize() > 0 );
	
	FreeBufferObject();
	size = other.GetSize();						// this strips the MAPPED_FLAG
	offsetInOtherBuffer = other.GetOffset();	// this strips the OWNS_BUFFER_FLAG
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
idVertexBuffer::Reference
========================
*/
void idVertexBuffer::Reference( const idVertexBuffer& other, int refOffset, int refSize )
{
	assert( IsMapped() == false );
	//assert( other.IsMapped() == false );	// this happens when building idTriangles while at the same time setting up idDrawVerts
	assert( other.GetAPIObject() != NULL );
	assert( refOffset >= 0 );
	assert( refSize >= 0 );
	assert( refOffset + refSize <= other.GetSize() );
	
	FreeBufferObject();
	size = refSize;
	offsetInOtherBuffer = other.GetOffset() + refOffset;
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
idVertexBuffer::Update
========================
*/
void idVertexBuffer::Update( const void* data, int updateSize ) const
{
	assert( apiObject != NULL );
	assert( IsMapped() == false );
	assert_16_byte_aligned( data );
	assert( ( GetOffset() & 15 ) == 0 );
	
	if( updateSize > size )
	{
		idLib::FatalError( "idVertexBuffer::Update: size overrun, %i > %i\n", updateSize, GetSize() );
	}
	
	GLsizeiptr numBytes = ALIGN( updateSize, 16 );
	
	// RB: 64 bit fixes, changed GLuint to GLintptrARB
	///GLintptr bufferObject = reinterpret_cast< GLintptr >( apiObject );
	// RB end
	
	GL_UpdateSubData( GL_ARRAY_BUFFER, reinterpret_cast< GLintptr >( apiObject ), GetOffset(), numBytes, data );

	
	void * buffer = MapBuffer( BM_WRITE );
	CopyBuffer( (byte *)buffer + GetOffset(), (byte *)data, numBytes );
	UnmapBuffer();
	
}

/*
========================
idVertexBuffer::MapBuffer
========================
*/
void* idVertexBuffer::MapBuffer( bufferMapType_t mapType ) const
{
	assert( apiObject != NULL );
	assert( IsMapped() == false );

	void* buffer = NULL;
	buffer = GL_TryMapBufferRange( GL_ARRAY_BUFFER, reinterpret_cast< GLintptr >( apiObject ), GetOffset(), GetAllocedSize(), GetMapFlags( mapType, false ) );
	
	SetMapped();

	if( buffer == NULL )
	{
		idLib::FatalError( "idVertexBuffer::MapBuffer: failed" );
	}
	return buffer;
}

/*
========================
idVertexBuffer::UnmapBuffer
========================
*/
void idVertexBuffer::UnmapBuffer() const
{
	assert( apiObject != NULL );
	assert( IsMapped() );
	
	if( !GL_TryUnmap( GL_ARRAY_BUFFER, reinterpret_cast< GLintptr >( apiObject ) ) )
	{
		idLib::Printf( "idVertexBuffer::UnmapBuffer failed\n" );
	}
	
	SetUnmapped();
}

/*
========================
idVertexBuffer::ClearWithoutFreeing
========================
*/
void idVertexBuffer::ClearWithoutFreeing()
{
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
}

/*
================================================================================================

	idIndexBuffer

================================================================================================
*/

/*
========================
idIndexBuffer::idIndexBuffer
========================
*/
idIndexBuffer::idIndexBuffer()
{
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
	SetUnmapped();
}

/*
========================
idIndexBuffer::~idIndexBuffer
========================
*/
idIndexBuffer::~idIndexBuffer()
{
	FreeBufferObject();
}

/*
========================
idIndexBuffer::AllocBufferObject
========================
*/
bool idIndexBuffer::AllocBufferObject( const void* data, int allocSize )
{
	assert( apiObject == NULL );
	assert_16_byte_aligned( data );
	
	if( allocSize <= 0 )
	{
		idLib::Error( "idIndexBuffer::AllocBufferObject: allocSize = %i", allocSize );
	}
	
	size = allocSize;

	int numBytes = GetAllocedSize();
	
	// clear out any previous error
	glGetError();

	// these are rewritten every frame
	apiObject = GL_CreateBuffer( "idIndexBuffer", this, GL_ELEMENT_ARRAY_BUFFER, BU_DEFAULT, numBytes, NULL );

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "index buffer alloc %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize() );
	}
	
	// copy the data
	if( data != NULL )
	{
		Update( data, allocSize );
	}
	
	return( apiObject != nullptr );
}

/*
========================
idIndexBuffer::FreeBufferObject
========================
*/
void idIndexBuffer::FreeBufferObject()
{
	if( IsMapped() )
	{
		UnmapBuffer();
	}
	
	// if this is a sub-allocation inside a larger buffer, don't actually free anything.
	if( OwnsBuffer() == false )
	{
		ClearWithoutFreeing();
		return;
	}
	
	if( apiObject == NULL )
	{
		return;
	}
	
	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "index buffer free %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize() );
	}
	
	// RB: 64 bit fixes, changed GLuint to GLintptrARB
	GLintptr bufferObject = reinterpret_cast< GLintptr >( apiObject );
	glDeleteBuffers( 1, ( const unsigned int* )& bufferObject );
	// RB end
	
	ClearWithoutFreeing();
}

/*
========================
idIndexBuffer::Reference
========================
*/
void idIndexBuffer::Reference( const idIndexBuffer& other )
{
	assert( IsMapped() == false );
	//assert( other.IsMapped() == false );	// this happens when building idTriangles while at the same time setting up triIndex_t
	assert( other.GetAPIObject() != NULL );
	assert( other.GetSize() > 0 );
	
	FreeBufferObject();
	size = other.GetSize();						// this strips the MAPPED_FLAG
	offsetInOtherBuffer = other.GetOffset();	// this strips the OWNS_BUFFER_FLAG
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
idIndexBuffer::Reference
========================
*/
void idIndexBuffer::Reference( const idIndexBuffer& other, int refOffset, int refSize )
{
	assert( IsMapped() == false );
	//assert( other.IsMapped() == false );	// this happens when building idTriangles while at the same time setting up triIndex_t
	assert( other.GetAPIObject() != NULL );
	assert( refOffset >= 0 );
	assert( refSize >= 0 );
	assert( refOffset + refSize <= other.GetSize() );
	
	FreeBufferObject();
	size = refSize;
	offsetInOtherBuffer = other.GetOffset() + refOffset;
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
idIndexBuffer::Update
========================
*/
void idIndexBuffer::Update( const void* data, int updateSize ) const
{
	assert( apiObject != NULL );
	assert( IsMapped() == false );
	assert_16_byte_aligned( data );
	assert( ( GetOffset() & 15 ) == 0 );
	
	if( updateSize > size )
	{
		idLib::FatalError( "idIndexBuffer::Update: size overrun, %i > %i\n", updateSize, GetSize() );
	}
	
	int numBytes = ALIGN( updateSize, 16 );

	GL_UpdateSubData( GL_ELEMENT_ARRAY_BUFFER, reinterpret_cast< GLintptr >( apiObject ), GetOffset(), numBytes, data );
	/*
		void * buffer = MapBuffer( BM_WRITE );
		CopyBuffer( (byte *)buffer + GetOffset(), (byte *)data, numBytes );
		UnmapBuffer();
	*/
}

/*
========================
idIndexBuffer::MapBuffer
========================
*/
void* idIndexBuffer::MapBuffer( bufferMapType_t mapType ) const
{
	assert( apiObject != NULL );
	assert( IsMapped() == false );
	
	void* buffer = NULL;
	buffer = GL_TryMapBufferRange( GL_ELEMENT_ARRAY_BUFFER, reinterpret_cast< GLintptr >( apiObject ), GetOffset(), GetAllocedSize(), GetMapFlags( mapType, false ) );

	SetMapped();
	
	if( buffer == NULL )
	{
		idLib::FatalError( "idIndexBuffer::MapBuffer: failed" );
	}
	return buffer;
}

/*
========================
idIndexBuffer::UnmapBuffer
========================
*/
void idIndexBuffer::UnmapBuffer() const
{
	assert( apiObject != NULL );
	assert( IsMapped() );
	
	if( !GL_TryUnmap( GL_ELEMENT_ARRAY_BUFFER, reinterpret_cast< GLintptr >( apiObject ) ) )
	{
		idLib::Printf( "idIndexBuffer::UnmapBuffer failed\n" );
	}
	
	SetUnmapped();
}

/*
========================
idIndexBuffer::ClearWithoutFreeing
========================
*/
void idIndexBuffer::ClearWithoutFreeing()
{
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
}

/*
================================================================================================

	idJointBuffer

================================================================================================
*/

/*
========================
idJointBuffer::idJointBuffer
========================
*/
idJointBuffer::idJointBuffer()
{
	numJoints = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
	SetUnmapped();
}

/*
========================
idJointBuffer::~idJointBuffer
========================
*/
idJointBuffer::~idJointBuffer()
{
	FreeBufferObject();
}

/*
========================
idJointBuffer::AllocBufferObject
========================
*/
bool idJointBuffer::AllocBufferObject( const float* joints, int numAllocJoints )
{
	assert( apiObject == NULL );
	assert_16_byte_aligned( joints );
	
	if( numAllocJoints <= 0 )
	{
		idLib::Error( "idJointBuffer::AllocBufferObject: joints = %i", numAllocJoints );
	}
	
	numJoints = numAllocJoints;

	apiObject = GL_CreateBuffer( "idJointBuffer", this, GL_UNIFORM_BUFFER, BU_DEFAULT, GetAllocedSize(), NULL );
	
	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "joint buffer alloc %p, api %p (%i joints)\n", this, GetAPIObject(), GetNumJoints() );
	}
	
	// copy the data
	if( joints != NULL )
	{
		Update( joints, numAllocJoints );
	}
	
	return( apiObject  != nullptr );
}

/*
========================
idJointBuffer::FreeBufferObject
========================
*/
void idJointBuffer::FreeBufferObject()
{
	if( IsMapped() )
	{
		UnmapBuffer();
	}
	
	// if this is a sub-allocation inside a larger buffer, don't actually free anything.
	if( OwnsBuffer() == false )
	{
		ClearWithoutFreeing();
		return;
	}
	
	if( apiObject == NULL )
	{
		return;
	}
	
	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "joint buffer free %p, api %p (%i joints)\n", this, GetAPIObject(), GetNumJoints() );
	}
	
	// RB: 64 bit fixes, changed GLuint to GLintptrARB
	GLintptr buffer = reinterpret_cast< GLintptr >( apiObject );
	
	glBindBuffer( GL_UNIFORM_BUFFER, 0 );
	glDeleteBuffers( 1, ( const GLuint* )& buffer );
	// RB end
	
	ClearWithoutFreeing();
}

/*
========================
idJointBuffer::Reference
========================
*/
void idJointBuffer::Reference( const idJointBuffer& other )
{
	assert( IsMapped() == false );
	assert( other.IsMapped() == false );
	assert( other.GetAPIObject() != NULL );
	assert( other.GetNumJoints() > 0 );
	
	FreeBufferObject();
	numJoints = other.GetNumJoints();			// this strips the MAPPED_FLAG
	offsetInOtherBuffer = other.GetOffset();	// this strips the OWNS_BUFFER_FLAG
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
idJointBuffer::Reference
========================
*/
void idJointBuffer::Reference( const idJointBuffer& other, int jointRefOffset, int numRefJoints )
{
	assert( IsMapped() == false );
	assert( other.IsMapped() == false );
	assert( other.GetAPIObject() != NULL );
	assert( jointRefOffset >= 0 );
	assert( numRefJoints >= 0 );
	assert( jointRefOffset + numRefJoints * sizeof( idJointMat ) <= other.GetNumJoints() * sizeof( idJointMat ) );
	assert_16_byte_aligned( numRefJoints * 3 * 4 * sizeof( float ) );
	
	FreeBufferObject();
	numJoints = numRefJoints;
	offsetInOtherBuffer = other.GetOffset() + jointRefOffset;
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
idJointBuffer::Update
========================
*/
void idJointBuffer::Update( const float* joints, int numUpdateJoints ) const
{
	assert( apiObject != NULL );
	assert( IsMapped() == false );
	assert_16_byte_aligned( joints );
	assert( ( GetOffset() & 15 ) == 0 );
	
	if( numUpdateJoints > numJoints )
	{
		idLib::FatalError( "idJointBuffer::Update: size overrun, %i > %i\n", numUpdateJoints, numJoints );
	}
	
	const GLsizeiptr numBytes = numUpdateJoints * 3 * 4 * sizeof( float );

	GLintptr bufferObject = reinterpret_cast< GLintptr >( apiObject );

	if( GLEW_EXT_direct_state_access )
	{
		glNamedBufferSubDataEXT( bufferObject, GetOffset(), numBytes, joints );
	}
	else {
		// RB: 64 bit fixes, changed GLuint to GLintptrARB
		glBindBuffer( GL_UNIFORM_BUFFER, bufferObject );
		// RB end
		glBufferSubData( GL_UNIFORM_BUFFER, GetOffset(), numBytes, joints );
	}
}

/*
========================
idJointBuffer::MapBuffer
========================
*/
float* idJointBuffer::MapBuffer( bufferMapType_t mapType ) const
{
	assert( IsMapped() == false );
	assert( mapType == BM_WRITE || mapType == BM_WRITE_NOSYNC );
	assert( apiObject != NULL );
	
	void* buffer = NULL;
	buffer = GL_TryMapBufferRange( GL_UNIFORM_BUFFER, reinterpret_cast< GLintptr >( apiObject ), GetOffset(), GetAllocedSize(), GetMapFlags( mapType, true ) );
	if( buffer == NULL )
	{
		idLib::FatalError( "idJointBuffer::MapBuffer: failed" );
	}
	assert( GetOffset() == 0 );

	SetMapped();

	return ( float* ) buffer;
}

/*
========================
idJointBuffer::UnmapBuffer
========================
*/
void idJointBuffer::UnmapBuffer() const
{
	assert( apiObject != NULL );
	assert( IsMapped() );
	
	if( !GL_TryUnmap( GL_UNIFORM_BUFFER, reinterpret_cast< GLintptr >( apiObject ) ) )
	{
		idLib::Printf( "idJointBuffer::UnmapBuffer failed\n" );
	}
	
	SetUnmapped();
}

/*
========================
idJointBuffer::ClearWithoutFreeing
========================
*/
void idJointBuffer::ClearWithoutFreeing()
{
	numJoints = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
}

/*
========================
idJointBuffer::Swap
========================
*/
void idJointBuffer::Swap( idJointBuffer& other )
{
	// Make sure the ownership of the buffer is not transferred to an unintended place.
	assert( other.OwnsBuffer() == OwnsBuffer() );
	
	SwapValues( other.numJoints, numJoints );
	SwapValues( other.offsetInOtherBuffer, offsetInOtherBuffer );
	SwapValues( other.apiObject, apiObject );
}

/*
================================================================================================

	idUniformBuffer

================================================================================================
*/

/*
========================
idUniformBuffer::idUniformBuffer
========================
*/
idUniformBuffer::idUniformBuffer()
{
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = nullptr;

	SetUnmapped();
}

/*
========================
idUniformBuffer::~idUniformBuffer
========================
*/
idUniformBuffer::~idUniformBuffer()
{
	FreeBufferObject();
}

/*
========================
idUniformBuffer::idUniformBuffer
========================
*/
bool idUniformBuffer::AllocBufferObject( int allocSize, bufferUsageType_t allocUsage, const void* data )
{
	assert( apiObject == nullptr );
	assert_16_byte_aligned( data );

	if( allocSize <= 0 ) {
		idLib::Error( "idUniformBuffer::AllocBufferObject: allocSize = %i", allocSize );
	}

	size = allocSize;
	usage = allocUsage;

	// clear out any previous error
	glGetError();

	apiObject = GL_CreateBuffer( "idUniformBuffer", this, GL_UNIFORM_BUFFER, allocUsage, GetAllocedSize(), data );

	if( r_showBuffers.GetBool() ) {
		idLib::Printf( "constants buffer %p api %p, size %i ( alloced %i ) bytes\n", this, GetAPIObject(), GetSize(), GetAllocedSize() );
	}

	return( apiObject != nullptr );
}

/*
========================
idUniformBuffer::FreeBufferObject
========================
*/
void idUniformBuffer::FreeBufferObject()
{
	if( IsMapped() )
	{
		UnmapBuffer();
	}

	// if this is a sub-allocation inside a larger buffer, don't actually free anything.
	if( OwnsBuffer() == false )
	{
		ClearWithoutFreeing();
		return;
	}

	if( apiObject == nullptr )
	{
		return;
	}

	if( r_showBuffers.GetBool() ) {
		idLib::Printf( "constants buffer free %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize() );
	}

	GLintptr bufferObject = reinterpret_cast< GLintptr >( apiObject );
	glDeleteBuffers( 1, ( const unsigned int* )& bufferObject );

	ClearWithoutFreeing();
}

/*
========================
idUniformBuffer::ClearWithoutFreeing
========================
*/
void idUniformBuffer::ClearWithoutFreeing()
{
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = nullptr;
}

/*
========================
idUniformBuffer::Update
========================
*/
void idUniformBuffer::Update( int updateSize, const void* data ) const
{
	assert( apiObject != nullptr );
	assert( IsMapped() == false );
	assert_16_byte_aligned( data );
	assert( ( GetOffset() & 15 ) == 0 );
	assert( usage == BU_DYNAMIC );

	if( updateSize > size )
	{
		idLib::FatalError( "idUniformBuffer::Update: size overrun, %i > %i\n", updateSize, GetSize() );
	}
	//glConfig.uniformBufferOffsetAlignment

	GL_UpdateSubData( GL_UNIFORM_BUFFER, reinterpret_cast< GLintptr >( apiObject ), GetOffset(), updateSize, data );

	///void * buffer = MapBuffer( BM_WRITE );
	///CopyBuffer( (byte *)buffer + GetOffset(), (byte *)data, numBytes );
	///UnmapBuffer();
}

/*
========================
idUniformBuffer::MapBuffer
========================
*/
void* idUniformBuffer::MapBuffer( bufferMapType_t mapType ) const
{
	assert( apiObject != NULL );
	assert( IsMapped() == false );

	void* buffer = NULL;
	buffer = GL_TryMapBufferRange( GL_UNIFORM_BUFFER, reinterpret_cast< GLintptr >( apiObject ), GetOffset(), GetAllocedSize(), GetMapFlags( mapType, true ) );
	if( buffer == NULL )
	{
		idLib::FatalError( "idUniformBuffer::MapBuffer: failed" );
	}

	SetMapped();

	return buffer;
}

/*
========================
idIndexBuffer::UnmapBuffer
========================
*/
void idUniformBuffer::UnmapBuffer() const
{
	assert( apiObject != NULL );
	assert( IsMapped() );

	if( !GL_TryUnmap( GL_UNIFORM_BUFFER, reinterpret_cast< GLintptr >( apiObject ) ) )
	{
		idLib::Printf( "idUniformBuffer::UnmapBuffer failed\n" );
	}

	SetUnmapped();
}

/*
========================
idVertexBuffer::Reference
========================
*/
void idUniformBuffer::Reference( const idUniformBuffer& other )
{
	assert( IsMapped() == false );
	assert( other.IsMapped() == false );
	assert( other.GetAPIObject() != NULL );
	assert( other.GetSize() > 0 );

	FreeBufferObject();
	size = other.GetSize();
	offsetInOtherBuffer = other.GetOffset();
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
idVertexBuffer::Reference
========================
*/
void idUniformBuffer::Reference( const idUniformBuffer& other, int refOffset, int refSize )
{
	assert( IsMapped() == false );
	assert( other.IsMapped() == false );
	assert( other.GetAPIObject() != NULL );
	assert( refOffset >= 0 );
	assert( refSize >= 0 );
	assert( refOffset + refSize <= other.GetSize() );

	FreeBufferObject();
	size = refSize;
	offsetInOtherBuffer = other.GetOffset() + refOffset;
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}



