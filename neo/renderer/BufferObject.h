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
#ifndef __BUFFEROBJECT_H__
#define __BUFFEROBJECT_H__

/*
================================================================================================

	Buffer Objects

================================================================================================
*/

class idIndexBuffer;

enum bufferMapType_t
{
	BM_READ,			// map for reading
	BM_READ_NOSYNC,
	BM_WRITE,			// map for writing
	BM_WRITE_NOSYNC
};

enum bufferUsageType_t 
{
	BU_DEFAULT,			// mappable, no dinamic update, draw
	BU_STATIC,			// no map, no dinamic update, draw
	BU_DYNAMIC,			// no map, dinamic update, draw
	BU_STAGING			// internal
};

// Returns all targets to virtual memory use instead of buffer object use.
// Call this before doing any conventional buffer reads, like screenshots.
void UnbindBufferObjects();

/*
================================================
idVertexBuffer
================================================
*/
class idVertexBuffer
{
public:
	idVertexBuffer();
	~idVertexBuffer();
	
	// Allocate or free the buffer.
	bool				AllocBufferObject( const void* data, int allocSize /*vertexMask, allocUsage, bufferObject*/ ); 
	void				FreeBufferObject();
	
	// Make this buffer a reference to another buffer.
	void				Reference( const idVertexBuffer& other );
	void				Reference( const idVertexBuffer& other, int refOffset, int refSize );
	
	// Copies data to the buffer. 'size' may be less than the originally allocated size.
	void				Update( const void* data, int updateSize ) const;
	
	void* 				MapBuffer( bufferMapType_t mapType ) const;
	idDrawVert* 		MapVertexBuffer( bufferMapType_t mapType ) const
	{
		return static_cast< idDrawVert* >( MapBuffer( mapType ) );
	}
	void				UnmapBuffer() const;
	bool				IsMapped() const
	{
		return ( size & MAPPED_FLAG ) != 0;
	}
	
	int					GetSize() const
	{
		return ( size & ~MAPPED_FLAG );
	}
	int					GetAllocedSize() const
	{
		return ( ( size & ~MAPPED_FLAG ) + 15 ) & ~15;
	}
	void* 				GetAPIObject() const
	{
		return apiObject;
	}
	int					GetOffset() const
	{
		return ( offsetInOtherBuffer & ~OWNS_BUFFER_FLAG );
	}
	
private:
	int					size;					// size in bytes
	int					offsetInOtherBuffer;	// offset in bytes
	void* 				apiObject;
	
	// sizeof() confuses typeinfo...
	static const int	MAPPED_FLAG			= 1 << ( 4 /* sizeof( int ) */ * 8 - 1 );
	static const int	OWNS_BUFFER_FLAG	= 1 << ( 4 /* sizeof( int ) */ * 8 - 1 );
	
private:
	void				ClearWithoutFreeing();
	void				SetMapped() const
	{
		const_cast< int& >( size ) |= MAPPED_FLAG;
	}
	void				SetUnmapped() const
	{
		const_cast< int& >( size ) &= ~MAPPED_FLAG;
	}
	bool				OwnsBuffer() const
	{
		return ( ( offsetInOtherBuffer & OWNS_BUFFER_FLAG ) != 0 );
	}
	
	DISALLOW_COPY_AND_ASSIGN( idVertexBuffer );
};

/*
================================================
idIndexBuffer
================================================
*/
class idIndexBuffer
{
public:
	idIndexBuffer();
	~idIndexBuffer();
	
	// Allocate or free the buffer.
	bool				AllocBufferObject( const void* data, int allocSize /*allocUsage, bufferObject*/ );
	void				FreeBufferObject();
	
	// Make this buffer a reference to another buffer.
	void				Reference( const idIndexBuffer& other );
	void				Reference( const idIndexBuffer& other, int refOffset, int refSize );
	
	// Copies data to the buffer. 'size' may be less than the originally allocated size.
	void				Update( const void* data, int updateSize ) const;
	
	void* 				MapBuffer( bufferMapType_t mapType ) const;
	triIndex_t* 		MapIndexBuffer( bufferMapType_t mapType ) const
	{
		return static_cast< triIndex_t* >( MapBuffer( mapType ) );
	}
	void				UnmapBuffer() const;
	bool				IsMapped() const
	{
		return ( size & MAPPED_FLAG ) != 0;
	}
	
	int					GetSize() const
	{
		return ( size & ~MAPPED_FLAG );
	}
	int					GetAllocedSize() const
	{
		return ( ( size & ~MAPPED_FLAG ) + 15 ) & ~15;
	}
	void* 				GetAPIObject() const
	{
		return apiObject;
	}
	int					GetOffset() const
	{
		return ( offsetInOtherBuffer & ~OWNS_BUFFER_FLAG );
	}
	
private:
	int					size;					// size in bytes
	int					offsetInOtherBuffer;	// offset in bytes
	void* 				apiObject;
	
	// sizeof() confuses typeinfo...
	static const int	MAPPED_FLAG			= 1 << ( 4 /* sizeof( int ) */ * 8 - 1 );
	static const int	OWNS_BUFFER_FLAG	= 1 << ( 4 /* sizeof( int ) */ * 8 - 1 );
	
private:
	void				ClearWithoutFreeing();
	void				SetMapped() const
	{
		const_cast< int& >( size ) |= MAPPED_FLAG;
	}
	void				SetUnmapped() const
	{
		const_cast< int& >( size ) &= ~MAPPED_FLAG;
	}
	bool				OwnsBuffer() const
	{
		return ( ( offsetInOtherBuffer & OWNS_BUFFER_FLAG ) != 0 );
	}
	
	DISALLOW_COPY_AND_ASSIGN( idIndexBuffer );
};

/*
================================================
idJointBuffer

IMPORTANT NOTICE: on the PC, binding to an offset in uniform buffer objects
is limited to GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, which is 256 on current nvidia cards,
so joint offsets, which are multiples of 48 bytes, must be in multiples of 16 = 768 bytes.
================================================
*/
class idJointBuffer
{
public:
	idJointBuffer();
	~idJointBuffer();
	
	// Allocate or free the buffer.
	bool				AllocBufferObject( const float* joints, int numAllocJoints /*buffer*/);
	void				FreeBufferObject();
	
	// Make this buffer a reference to another buffer.
	void				Reference( const idJointBuffer& other );
	void				Reference( const idJointBuffer& other, int jointRefOffset, int numRefJoints );
	
	// Copies data to the buffer. 'numJoints' may be less than the originally allocated size.
	void				Update( const float* joints, int numUpdateJoints ) const;
	
	float* 				MapBuffer( bufferMapType_t mapType ) const;
	void				UnmapBuffer() const;
	bool				IsMapped() const
	{
		return ( numJoints & MAPPED_FLAG ) != 0;
	}
	
	int					GetNumJoints() const
	{
		return ( numJoints & ~MAPPED_FLAG );
	}
	int					GetAllocedSize() const
	{
		return ( numJoints & ~MAPPED_FLAG ) * 3 * 4 * sizeof( float );
	}
	void* 				GetAPIObject() const
	{
		return apiObject;
	}
	int					GetOffset() const
	{
		return ( offsetInOtherBuffer & ~OWNS_BUFFER_FLAG );
	}
	
	void				Swap( idJointBuffer& other );
	
private:
	int					numJoints;
	int					offsetInOtherBuffer;	// offset in bytes
	void* 				apiObject;

	// flags
	
	// sizeof() confuses typeinfo...
	static const int	MAPPED_FLAG			= 1 << ( 4 /* sizeof( int ) */ * 8 - 1 );
	static const int	OWNS_BUFFER_FLAG	= 1 << ( 4 /* sizeof( int ) */ * 8 - 1 );

	// JOINT_MAT_ROWS 
	// JOINT_MAT_SIZE 
	// JOINT_QUAT_ROWS 
	// JOINT_QUAT_SIZE
	
private:
	void				ClearWithoutFreeing();
	void				SetMapped() const
	{
		const_cast< int& >( numJoints ) |= MAPPED_FLAG;
	}
	void				SetUnmapped() const
	{
		const_cast< int& >( numJoints ) &= ~MAPPED_FLAG;
	}
	bool				OwnsBuffer() const
	{
		return ( ( offsetInOtherBuffer & OWNS_BUFFER_FLAG ) != 0 );
	}
	
	DISALLOW_COPY_AND_ASSIGN( idJointBuffer );
};



/*
=====================================================================================================
	idUniformBuffer	idUniformBuffer
=====================================================================================================
*/
class idUniformBuffer {
public:
	idUniformBuffer();
	~idUniformBuffer();

	// Allocate or free the buffer.
	bool				AllocBufferObject( int allocSize, bufferUsageType_t allocUsage, const void* data = nullptr );
	void				FreeBufferObject();

	// Make this buffer a reference to another buffer.
	void				Reference( const idUniformBuffer& other );
	void				Reference( const idUniformBuffer& other, int refOffset, int refSize );

	// Copies data to the buffer. 'size' may be less than the originally allocated size.
	void				Update( int updateSize, const void* data ) const;

	void * 				MapBuffer( bufferMapType_t mapType ) const;
	///void * TryMapBuffer( bufferMapType_t mapType );
	void				UnmapBuffer() const;
	ID_INLINE bool		IsMapped() const { return ( size & MAPPED_FLAG ) != 0; }

	ID_INLINE int		GetSize() const { return ( size & ~MAPPED_FLAG ); }
	ID_INLINE int		GetAllocedSize() const { return ALIGN( GetSize(), 16 ); }
	ID_INLINE void * 	GetAPIObject() const { return apiObject; }
	ID_INLINE int		GetOffset() const { return ( offsetInOtherBuffer & ~OWNS_BUFFER_FLAG ); }

private:
	int					size;					// size in bytes
	int					offsetInOtherBuffer;	// offset in bytes
	void * 				apiObject;

	bufferUsageType_t	usage;

	static const int	MAPPED_FLAG			= MIN_TYPE( int );
	static const int	OWNS_BUFFER_FLAG	= MIN_TYPE( int );

private:
	void				ClearWithoutFreeing();
	ID_INLINE void		SetMapped() const { const_cast< int& >( size ) |= MAPPED_FLAG; }
	ID_INLINE void		SetUnmapped() const { const_cast< int& >( size ) &= ~MAPPED_FLAG; }
	ID_INLINE bool		OwnsBuffer() const { return ( ( offsetInOtherBuffer & OWNS_BUFFER_FLAG ) != 0 ); }

	DISALLOW_COPY_AND_ASSIGN( idUniformBuffer );
};

// idShaderStorageBuffer

class idPixelPackBuffer {
};

class idPixelUnpackBuffer {
};


class idRenderBufferManager {
public:

	void	Init();
	void	Shutdown();

	void	BeginFrame();
	void	FinishFrame();

	void *	CreateBuffer();
	void	DestroyBuffer();
};





#endif // !__BUFFEROBJECT_H__
