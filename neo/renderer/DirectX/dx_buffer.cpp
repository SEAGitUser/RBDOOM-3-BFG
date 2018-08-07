#pragma hdrstop
#include "precompiled.h"

#include "../tr_local.h"

#include "dx_header.h"

/*
=====================================================================================================
					Contains the buffer object implementation for DirecX
=====================================================================================================
*/

idCVar r_showBuffers( "r_showBuffers", "1", CVAR_INTEGER, "print buffers creation info" );

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

	{
		auto device = GetCurrentDevice();

		DX_BUFFER_DESC desc = {};
		desc.ByteWidth = ( UINT )numBytes;

		if( data )
		{
			desc.Usage = D3D11_USAGE_IMMUTABLE;
			desc.CPUAccessFlags = 0;
		}
		else { // these are rewritten every frame
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}

		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		//if( streamout )
		//	desc.BindFlags |= D3D11_BIND_STREAM_OUTPUT;

		DX_SUBRESOURCE_DATA intiData = { data, 0, 0 };

		DXBuffer * pBuffer = nullptr;
		CHECK_FE( device->CreateBuffer( &desc, data ? &intiData : nullptr, &pBuffer ) );

		D3DSetDebugObjectName( pBuffer, "idVertexBuffer" );

		apiObject = pBuffer;
	}

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "vertex buffer %p api %p, size %i ( alloced %i ) bytes\n", this, GetAPIObject(), GetSize(), GetAllocedSize() );
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

	auto pBuffer = ( DXBuffer * )GetAPIObject();
	pBuffer->Release();

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
	/*
	void * buffer = MapBuffer( BM_WRITE );
	CopyBuffer( (byte *)buffer + GetOffset(), (byte *)data, numBytes );
	UnmapBuffer();
	*/
}

/*
========================
idVertexBuffer::MapBuffer
========================
*/
void * idVertexBuffer::MapBuffer( bufferMapType_t mapType ) const
{
	assert( apiObject != NULL );
	assert( IsMapped() == false );

	//auto device = GetCurrentDevice();
	auto iContext = GetCurrentImContext();

	auto pBuffer = ( DXBuffer * )GetAPIObject();

	DX_MAPPED_SUBRESOURCE mappedResource;

	//UINT flags = D3D11_MAP_FLAG_DO_NOT_WAIT;
	// D3D11_MAP_FLAG_DO_NOT_WAIT cannot be used with D3D11_MAP_WRITE_DISCARD or D3D11_MAP_WRITE_NO_OVERWRITE.

	iContext->Map( pBuffer, 0,
		D3D11_MAP_WRITE_DISCARD, // D3D11_MAP_WRITE_NO_OVERWRITE
		0,
		&mappedResource );

	SetMapped();

	if( !mappedResource.pData )
	{
		idLib::FatalError( "idVertexBuffer::MapBuffer: failed" );
	}
	return mappedResource.pData;
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

	//auto device = GetCurrentDevice();
	auto iContext = GetCurrentImContext();

	auto pBuffer = ( DXBuffer * )GetAPIObject();

	iContext->Unmap( pBuffer, 0 );

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

	{
		auto device = GetCurrentDevice();

		DX_BUFFER_DESC desc = {};
		desc.ByteWidth = ( UINT )numBytes;

		if( data )
		{
			desc.Usage = D3D11_USAGE_IMMUTABLE;
			desc.CPUAccessFlags = 0;
		}
		else
		{ // these are rewritten every frame
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}

		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		//if( streamout )
		//	desc.BindFlags |= D3D11_BIND_STREAM_OUTPUT;

		DX_SUBRESOURCE_DATA intiData = { data, 0, 0 };

		DXBuffer * pBuffer = nullptr;
		CHECK_FE( device->CreateBuffer( &desc, data ? &intiData : nullptr, &pBuffer ) );

		D3DSetDebugObjectName( pBuffer, "idIndexBuffer" );

		apiObject = pBuffer;
	}

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "index buffer alloc %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize() );
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

	auto pBuffer = ( DXBuffer * )GetAPIObject();
	pBuffer->Release();

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

	{
		auto device = GetCurrentDevice();

		DX_BUFFER_DESC desc = {};
		desc.ByteWidth = ALIGN( ( UINT )GetAllocedSize(), 16u );
		//desc.ByteWidth = idMath::Min( (UINT)D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT, desc.ByteWidth );

		// these are rewritten every frame
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		DX_SUBRESOURCE_DATA initData = { joints, 0, 0 };

		DXBuffer * pBuffer = nullptr;
		CHECK_FE( device->CreateBuffer( &desc, joints ? &initData : nullptr, &pBuffer ) );

		D3DSetDebugObjectName( pBuffer, "idJointBuffer" );

		apiObject = pBuffer;
	}

	if( r_showBuffers.GetBool() )
	{
		idLib::Printf( "joint buffer alloc %p, api %p (%i joints)\n", this, GetAPIObject(), GetNumJoints() );
	}

	// copy the data
	/*if( joints != NULL )
	{
		Update( joints, numAllocJoints );
	}*/

	return( apiObject != nullptr );
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

	auto pBuffer = ( DXBuffer * )GetAPIObject();
	pBuffer->Release();

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

	if( glConfig.directStateAccess )
	{
		glNamedBufferSubDataEXT( bufferObject, GetOffset(), numBytes, joints );
		return;
	}

	glBindBuffer( GL_UNIFORM_BUFFER, bufferObject );
	glBufferSubData( GL_UNIFORM_BUFFER, GetOffset(), numBytes, joints );
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

	return ( float* )buffer;
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

	if( allocSize <= 0 )
	{
		idLib::Error( "idUniformBuffer::AllocBufferObject: allocSize = %i", allocSize );
	}

	size = allocSize;
	usage = allocUsage;

	{
		auto device = GetCurrentDevice();

		DX_BUFFER_DESC desc = {};
		desc.ByteWidth = ALIGN( ( UINT )GetAllocedSize(), 16u );
		//desc.ByteWidth = idMath::Min( (UINT)D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT, desc.ByteWidth );

		if( allocUsage == BU_STATIC )
		{
			desc.Usage = D3D11_USAGE_IMMUTABLE;
			desc.CPUAccessFlags = 0;
			assert( data != nullptr );
		}
		else if( allocUsage == BU_DYNAMIC )
		{
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.CPUAccessFlags = 0;
		}
		else if( allocUsage == BU_DEFAULT )
		{
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}

		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		DX_SUBRESOURCE_DATA initData = { data, 0, 0 };

		DXBuffer * pBuffer = nullptr;
		CHECK_FE( device->CreateBuffer( &desc, data ? &initData : nullptr, &pBuffer ) );

		D3DSetDebugObjectName( pBuffer, "idUniformBuffer" );

		apiObject = pBuffer;
	}

	if( r_showBuffers.GetBool() )
	{
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

	if( r_showBuffers.GetBool() )
	{
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
idUniformBuffer::Bind
========================
*/
void idUniformBuffer::Bind( uint slot ) const
{
	// When you use glBindBufferRange on uniform buffers, the `offset` parameter must be aligned to an implementation-dependent
	// value: GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT.

	glBindBufferBase( GL_UNIFORM_BUFFER, slot, reinterpret_cast<GLintptr>( GetAPIObject() ) );
}
void idUniformBuffer::Bind( uint slot, int offset, int size ) const
{
	// When you use glBindBufferRange on uniform buffers, the `offset` parameter must be aligned to an implementation-dependent
	// value: GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT.

	glBindBufferRange( GL_UNIFORM_BUFFER, slot, reinterpret_cast<GLintptr>( GetAPIObject() ), offset, size );
}

/*
========================
idUniformBuffer::Update
	UpdateSubresource and Map with Discard should be about the same speed. Choose between them depending on
	which one copies the least amount of memory. If you already have your data stored in memory in one contiguous block,
	use UpdateSubresource. If you need to accumulate data from other places, use Map with Discard.

========================
*/
void idUniformBuffer::Update( int updateSize, const void * data ) const
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

	//GL_UpdateSubData( GL_UNIFORM_BUFFER, reinterpret_cast< GLintptr >( apiObject ), GetOffset(), updateSize, data );
	/*
	void * buffer = MapBuffer( BM_WRITE ); // Map( MAP_WRITE_DISCARD )
	CopyBuffer( (byte *)buffer + GetOffset(), (byte *)data, numBytes );
	UnmapBuffer();
	*/

	auto device = GetCurrentDevice();
	auto iContext = GetCurrentImContext();

	D3D11_FEATURE_DATA_D3D11_OPTIONS features;
	device->CheckFeatureSupport( D3D11_FEATURE_D3D11_OPTIONS, &features, sizeof( features ) );

	if( features.ConstantBufferPartialUpdate )
	{
		const D3D11_BOX DstBox = { GetOffset(), 0U, 0U, GetOffset() + updateSize, 1U, 1U };

		iContext->UpdateSubresource1( ( DXBuffer * )GetAPIObject(), 0, &DstBox, data, 0, 0, D3D11_COPY_DISCARD ); // D3D11_COPY_NO_OVERWRITE

		// All constant buffer data that is passed to XXSetConstantBuffers1() needs to be aligned to 256 byte boundaries.
		//iContext->PSSetConstantBuffers1( 0, 1, &buffer, &offset, &size );
	}
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

	auto iContext = GetCurrentImContext();

	auto pBuffer = ( DXBuffer * )GetAPIObject();

	DX_MAPPED_SUBRESOURCE mappedResource = {};

	//UINT flags = D3D11_MAP_FLAG_DO_NOT_WAIT;
	// D3D11_MAP_FLAG_DO_NOT_WAIT cannot be used with D3D11_MAP_WRITE_DISCARD or D3D11_MAP_WRITE_NO_OVERWRITE.

	auto res = iContext->Map( pBuffer, 0,
		D3D11_MAP_WRITE_DISCARD, // D3D11_MAP_WRITE_NO_OVERWRITE
		0,
		&mappedResource );

	CHECK_FE( res );

	if( !mappedResource.pData )
	{
		idLib::FatalError( "idUniformBuffer::MapBuffer: failed" );
	}

	SetMapped();

	//return mappedResource.pData;
	return ( byte * )mappedResource.pData + GetOffset();
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

	auto iContext = GetCurrentImContext();

	auto pBuffer = ( DXBuffer * )GetAPIObject();

	m_d3dContext->Unmap( pBuffer, 0 );

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



