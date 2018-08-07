
#include "precompiled.h"
#pragma hdrstop

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#include "../../sys/win32/win_local.h"

#elif defined( __ANDROID__ )
#define VK_USE_PLATFORM_ANDROID_KHR

#else
#define VK_USE_PLATFORM_XCB_KHR
#endif

#include "vk_header.h"

#include "..\tr_local.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

static const uint64 GLS_DEPTH_TEST_MASK = 1ull << 60;
static const uint64 GLS_MIRROR_VIEW = 1ull << 62;

struct vkCommandBuffer_t
{
	VkCommandBuffer m_handle = VK_NULL_HANDLE;

	uint64			m_stateBits = 0;

	VkRect2D		m_currentScissor;
	VkViewport		m_currentViewport;

	VkBuffer		m_currentIBO;
	VkDeviceSize	m_currentIBOffset;

	VkBuffer		m_currentVBO;
	VkDeviceSize	m_currentVBOffset;

	float			m_polyOfsScale;
	float			m_polyOfsBias;
	float			m_polyOfsClamp;

	struct boundAttachment_t {
		uint32_t baseArrayLayer;
		uint32_t layerCount;
	};

	////////////////////////////////////

	VkCommandBuffer GetHandle() const { return m_handle; }

	////////////////////////////////////

	void Clear( bool color, bool depth, bool stencil, byte stencilValue = 0, float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 0.0f )
	{
		uint32 numAttachments = 0;
		VkClearAttachment attachments[ 9 ] = {};

		if( color )
		{
			VkClearAttachment & attachment = attachments[ numAttachments++ ];
			attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			attachment.colorAttachment = 0;
			attachment.clearValue.color.float32[ 0 ] = r;
			attachment.clearValue.color.float32[ 1 ] = g;
			attachment.clearValue.color.float32[ 2 ] = b;
			attachment.clearValue.color.float32[ 3 ] = a;
		}

		if( depth || stencil )
		{
			VkClearAttachment & attachment = attachments[ numAttachments++ ];
			if( depth )
			{
				attachment.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
			}
			if( stencil )
			{
				attachment.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
			attachment.clearValue.depthStencil.depth = 1.0f;
			attachment.clearValue.depthStencil.stencil = stencilValue;
		}

		VkClearRect clearRect[ 1 ] = {};
		clearRect[ 0 ].baseArrayLayer = 0;
		clearRect[ 0 ].layerCount = 1;
		clearRect[ 0 ].rect = m_currentScissor;

		vkCmdClearAttachments( GetHandle(), numAttachments, attachments, _countof( clearRect ), clearRect );
	}
	void ClearColors( uint32 count, const idColor clearColors[], bool allLayers = false )
	{
		idImage * imgColors[ 8 ] = {};
		VkClearAttachment attachments[ 8 ] = {};

		for( uint32 i = 0; i < count; i++ )
		{
			VkClearAttachment & attachment = attachments[ i ];
			attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			attachment.colorAttachment = i;
			attachment.clearValue.color.float32[ 0 ] = clearColors[ i ].r;
			attachment.clearValue.color.float32[ 1 ] = clearColors[ i ].g;
			attachment.clearValue.color.float32[ 2 ] = clearColors[ i ].b;
			attachment.clearValue.color.float32[ 3 ] = clearColors[ i ].a;

			RENDERLOG_PRINT( "VK_ClearColor( attachment.%u, r.%f, g.%f, b.%f, a.%f )\n",
							 i, clearColors[ i ].r, clearColors[ i ].g, clearColors[ i ].b, clearColors[ i ].a );
		}

		//SEA: arrays? mips?

		VkClearRect clearRect[ 1 ] = {};

		clearRect[ 0 ].baseArrayLayer = 0;
		clearRect[ 0 ].layerCount = imgColors[ 0 ]->GetLayerCount(); // Cube ?
		clearRect[ 0 ].rect.extent = { imgColors[ 0 ]->GetUploadWidth(), imgColors[ 0 ]->GetUploadHeight() };

		vkCmdClearAttachments( GetHandle(), count, attachments, _countof( clearRect ), clearRect );
	}
	void ClearDepthStencil( bool depth, bool stencil, float depthValue = 1.0f, byte stencilValue = 0 )
	{
		idImage * imgDepthStencil;
		VkClearAttachment attachment = {};

		if( depth )
		{
			attachment.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		if( stencil )
		{
			attachment.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		assert( !!depth || !!stencil );

		attachment.clearValue.depthStencil.depth = depthValue;
		attachment.clearValue.depthStencil.stencil = stencilValue;

		//SEA: arrays? mips?

		VkClearRect clearRect[ 1 ] = {};

		clearRect[ 0 ].baseArrayLayer = 0;
		clearRect[ 0 ].layerCount = imgDepthStencil->GetLayerCount(); // Cube ?
		clearRect[ 0 ].rect.extent = { imgDepthStencil->GetUploadWidth(), imgDepthStencil->GetUploadHeight() };

		vkCmdClearAttachments( GetHandle(), 1, &attachment, _countof( clearRect ), clearRect );

		RENDERLOG_PRINT( "VK_ClearDepthStencil( depth.%i, stencil.%i, depthVal.%f, stencilVal.%i )\n", ( int ) depth, ( int ) stencil, depthValue, ( int ) stencilValue );
	}

	void DepthBoundsTest( const float zmin, const float zmax )
	{
		assert( zmin < zmax );

		if( zmin == 0.0f && zmax == 0.0f )
		{
			m_stateBits = m_stateBits & ~GLS_DEPTH_TEST_MASK;
		}
		else {
			m_stateBits |= GLS_DEPTH_TEST_MASK;
			vkCmdSetDepthBounds( GetHandle(), zmin, zmax );
		}

		RENDERLOG_PRINT( "GL_DepthBoundsTest( zmin.%f, zmax.%f )\n", zmin, zmax );
	}
	void PolygonOffset( float scale, float bias, float clamp = 0.0f )
	{
		vkCmdSetDepthBias( GetHandle(), bias, clamp, scale );
	}
	void SetScissor( int x /* left*/, int y /* bottom */, int w, int h )
	{
		m_currentScissor.offset.x = x;
		m_currentScissor.offset.y = y;
		m_currentScissor.extent.width = w;
		m_currentScissor.extent.height = h;

		vkCmdSetScissor( GetHandle(), 0, 1, &m_currentScissor );
	}
	void SetViewport( int x/* left */, int y/* bottom */, int w, int h )
	{
		m_currentViewport.x = x;
		m_currentViewport.y = y;
		m_currentViewport.width = w;
		m_currentViewport.height = h;
		m_currentViewport.minDepth = 0.0f;
		m_currentViewport.maxDepth = 1.0f;

		vkCmdSetViewport( GetHandle(), 0, 1, &m_currentViewport );
	}

	void BindVBO( const vkBuffer_t * vbuffer, VkDeviceSize offset )
	{
		if( m_currentVBOffset != offset || m_currentVBO != vbuffer->GetHandle() )
		{
			m_currentVBO = vbuffer->GetHandle();
			m_currentVBOffset = offset;

			vkCmdBindVertexBuffers( GetHandle(), 0, 1, &m_currentVBO, &m_currentVBOffset );
		}
	}

	void BindIBO( const vkBuffer_t * ibuffer, VkDeviceSize offset, VkIndexType indexType = VK_INDEX_TYPE_UINT16 )
	{
		if( m_currentIBOffset != offset || m_currentIBO != ibuffer->GetHandle() )
		{
			m_currentIBO = ibuffer->GetHandle();
			m_currentIBOffset = offset;

			vkCmdBindIndexBuffer( GetHandle(), m_currentIBO, m_currentIBOffset, indexType );
		}
	}
};

vkCommandBuffer_t * VK_GetCurrCMD() {
	return nullptr;
}



/*
====================
GL_BeginRenderView
====================
*/
void GL_BeginRenderView( const idRenderView * view )
{
	//glGetIntegerv( GL_FRONT_FACE, &backEnd.glState.initial_facing );
	//glFrontFace( view->isMirror ? GL_CCW : GL_CW );
	//RENDERLOG_PRINT( "FrontFace( %s )\n", view->isMirror ? "CCW" : "CW" );
}
/*
====================
GL_EndRenderView
====================
*/
void GL_EndRenderView()
{
	//glFrontFace( backEnd.glState.initial_facing );
}

/*
====================
GL_SelectTexture
====================
*/
void GL_SelectTexture( int unit )
{
	/*if( backEnd.glState.currenttmu == unit )
		return;

	if( unit < 0 || unit >= glConfig.maxTextureImageUnits )
	{
		common->Warning( "GL_SelectTexture: unit = %i", unit );
		return;
	}

	RENDERLOG_PRINT( "GL_SelectTexture( %i );\n", unit );

	backEnd.glState.currenttmu = unit;*/
}
/*
====================
GL_GetCurrentTextureUnit
====================
*/
int GL_GetCurrentTextureUnit()
{
	//return backEnd.glState.currenttmu;
}
/*
====================
GL_BindTexture
Automatically enables 2D mapping or cube mapping if needed
====================
*/
void GL_BindTexture( int unit, idImage *img )
{
	GL_SelectTexture( unit );

	RENDERLOG_PRINT( "GL_BindTexture( %i, %s, %ix%i )\n", unit, img->GetName(), img->GetUploadWidth(), img->GetUploadHeight() );

	// load the image if necessary (FIXME: not SMP safe!)
	if( !img->IsLoaded() )
	{
		// load the image on demand here, which isn't our normal game operating mode
		img->ActuallyLoadImage( true );
	}
#if 0
	const GLenum texUnit = backEnd.glState.currenttmu;
	const GLuint texnum = GetGLObject( img->GetAPIObject() );

	auto glBindTexObject = []( uint32 currentType, GLenum _unit, GLenum _target, GLuint _name ) {
		if( backEnd.glState.tmu[ _unit ].currentTarget[ currentType ] != _name )
		{
			backEnd.glState.tmu[ _unit ].currentTarget[ currentType ] = _name;

		#if !defined( USE_GLES3 )
			if( glConfig.directStateAccess )
			{
				glBindMultiTextureEXT( GL_TEXTURE0 + _unit, _target, _name );
			}
			else
			#endif
			{
				glActiveTexture( GL_TEXTURE0 + _unit );
				glBindTexture( _target, _name );
			}
		}
	};

	if( img->GetOpts().textureType == TT_2D )
	{
		if( img->GetOpts().IsMultisampled() )
		{
			if( img->GetOpts().IsArray() )
			{
				glBindTexObject( target_2DMSArray, texUnit, GL_TEXTURE_2D_MULTISAMPLE_ARRAY, texnum );
			}
			else
			{
				glBindTexObject( target_2DMS, texUnit, GL_TEXTURE_2D_MULTISAMPLE, texnum );
			}
		}
		else
		{
			if( img->GetOpts().IsArray() )
			{
				glBindTexObject( target_2DArray, texUnit, GL_TEXTURE_2D_ARRAY, texnum );
			}
			else
			{
				glBindTexObject( target_2D, texUnit, GL_TEXTURE_2D, texnum );
			}
		}
	}
	else if( img->GetOpts().textureType == TT_CUBIC )
	{
		if( img->GetOpts().IsArray() )
		{
			glBindTexObject( target_CubeMapArray, texUnit, GL_TEXTURE_CUBE_MAP_ARRAY, texnum );
		}
		else
		{
			glBindTexObject( target_CubeMap, texUnit, GL_TEXTURE_CUBE_MAP, texnum );
		}
	}
	else if( img->GetOpts().textureType == TT_3D )
	{
		glBindTexObject( target_3D, texUnit, GL_TEXTURE_3D, texnum );
	}

	/*if( GLEW_ARB_direct_state_access )
	{
	glBindTextureUnit( texUnit, texnum );
	}*/

	/*if( GLEW_ARB_multi_bind )
	{
	GLuint textures[ MAX_MULTITEXTURE_UNITS ] = { GL_ZERO };
	glBindTextures( 0, _countof( textures ), textures );

	GLuint samplers[ MAX_MULTITEXTURE_UNITS ] = { GL_ZERO };
	glBindSamplers( 0, _countof( samplers ), samplers );
	}*/

#if 0 //SEA: later ;)
	const struct glTypeInfo_t {
		GLenum target;
		//uint32 tmuIndex;
	} glInfo[ 7 ] = {
		GL_TEXTURE_2D,
		GL_TEXTURE_2D_MULTISAMPLE,
		GL_TEXTURE_2D_ARRAY,
		GL_TEXTURE_2D_MULTISAMPLE_ARRAY,
		GL_TEXTURE_CUBE_MAP,
		GL_TEXTURE_CUBE_MAP_ARRAY,
		GL_TEXTURE_3D
	};
	if( backEnd.glState.tmu[ texUnit ].currentTarget[ currentType ] != this->texnum )
	{
		backEnd.glState.tmu[ texUnit ].currentTarget[ currentType ] = this->texnum;

	#if !defined( USE_GLES2 ) && !defined( USE_GLES3 )
		if( glConfig.directStateAccess )
		{
			glBindMultiTextureEXT( GL_TEXTURE0 + texUnit, glInfo[].target, this->texnum );
		}
		else
		#endif
		{
			glActiveTexture( GL_TEXTURE0 + texUnit );
			glBindTexture( glInfo[].target, this->texnum );
		}
	}
#endif
#endif
}
/*
====================
GL_ResetTextureState
====================
*/
void GL_ResetTextureState()
{
	RENDERLOG_OPEN_BLOCK( "GL_ResetTextureState()" );
	/*for( int i = 0; i < MAX_MULTITEXTURE_UNITS - 1; i++ )
	{
	GL_SelectTexture( i );
	renderImageManager->BindNull();
	}*/
	renderImageManager->UnbindAll();

	GL_SelectTexture( 0 );
	RENDERLOG_CLOSE_BLOCK();
}

/*
====================
GL_Scissor
====================
*/
void GL_Scissor( int x /* left*/, int y /* bottom */, int w, int h, int scissorIndex )
{
	auto cmd = VK_GetCurrCMD();

	cmd->SetScissor( x, y, w, h );

	RENDERLOG_PRINT( "VK_Scissor( %i, %i, %i, %i )\n", x, y, w, h );
}
/*
====================
GL_Viewport
====================
*/
void GL_Viewport( int x /* left */, int y /* bottom */, int w, int h, int viewportIndex )
{
	auto cmd = VK_GetCurrCMD();

	cmd->SetViewport( x, y, w, h );

	RENDERLOG_PRINT( "VK_Viewport( %i, %i, %i, %i )\n", x, y, w, h );
}

/*
====================
GL_PolygonOffset
====================
*/
void GL_PolygonOffset( float scale, float bias )
{
	auto cmd = VK_GetCurrCMD();

	cmd->m_polyOfsScale = scale;
	cmd->m_polyOfsBias = bias;
	cmd->m_polyOfsClamp = 0.0f;

	if( cmd->m_stateBits & GLS_POLYGON_OFFSET )
	{
		cmd->PolygonOffset( cmd->m_polyOfsScale, cmd->m_polyOfsBias, cmd->m_polyOfsClamp );

		RENDERLOG_PRINT( "VK_PolygonOffset( scale %f, bias %f )\n", scale, bias );
	}
}

/*
========================
GL_DepthBoundsTest
========================
*/
void GL_DepthBoundsTest( const float zmin, const float zmax )
{
	if( /*!glConfig.depthBoundsTestAvailable ||*/ zmin > zmax )
		return;

	auto cmd = VK_GetCurrCMD();

	if( zmin == 0.0f && zmax == 0.0f )
	{
		vkCmdSetDepthBounds( cmd->GetHandle(), 0.0, 1.0 );

		RENDERLOG_PRINT( "VK_DepthBoundsTest( Disable )\n" );
	}
	else {
		vkCmdSetDepthBounds( cmd->GetHandle(), zmin, zmax );

		RENDERLOG_PRINT( "VK_DepthBoundsTest( zmin %f, zmax %f )\n", zmin, zmax );
	}
}


/*
========================
GL_StartDepthPass
========================
*/
void GL_StartDepthPass( const idScreenRect& rect )
{}
/*
========================
GL_FinishDepthPass
========================
*/
void GL_FinishDepthPass()
{}
/*
========================
GL_GetDepthPassRect
========================
*/
void GL_GetDepthPassRect( idScreenRect& rect )
{
	rect.Clear();
}

/*
========================
 GL_Clear
	The pixel ownership test, the scissor test, dithering, and the buffer writemasks affect the operation of glClear.
	The scissor box bounds the cleared region. Alpha function, blend function, logical operation, stenciling,
	texture mapping, and depth-buffering are ignored by glClear.
========================
*/
void GL_Clear( bool color, bool depth, bool stencil, byte stencilValue, float r, float g, float b, float a )
{
	auto cmd = VK_GetCurrCMD();

	uint32 boundCount = 1;

	uint32 numAttachments = 0;
	VkClearAttachment attachments[ 9 ] = {};

	if( color )
	{
		for( uint32 i = 0; i < boundCount; i++ )
		{
			VkClearAttachment & attachment = attachments[ i ];
			attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			attachment.colorAttachment = i;
			attachment.clearValue.color.float32[ 0 ] = r;
			attachment.clearValue.color.float32[ 1 ] = g;
			attachment.clearValue.color.float32[ 2 ] = b;
			attachment.clearValue.color.float32[ 3 ] = a;
		}

		RENDERLOG_PRINT( "Vk_Clear( color %f,%f,%f,%f )\n", r, g, b, a );
	}

	if( depth || stencil )
	{
		VkClearAttachment & attachment = attachments[ numAttachments++ ];
		if( depth )
		{
			attachment.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
			RENDERLOG_PRINT( "VK_Clear( depth 1.0 )\n" );
		}
		if( stencil )
		{
			attachment.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			RENDERLOG_PRINT( "VK_Clear( stencil %i )\n", stencilValue );
		}
		attachment.clearValue.depthStencil.depth = 1.0f;
		attachment.clearValue.depthStencil.stencil = stencilValue;
	}

	VkClearRect clearRect[ 1 ] = {};
	clearRect[ 0 ].rect = cmd->m_currentScissor;
	clearRect[ 0 ].baseArrayLayer = 0;
	clearRect[ 0 ].layerCount = 1;

	vkCmdClearAttachments( cmd->GetHandle(), numAttachments, attachments, _countof( clearRect ), clearRect );
}

/*
=============
 VL_DrawIndexed

	No state is inherited across command buffers!
=============
*/
void GL_DrawIndexed( const drawSurf_t * surf )
{
	// VkCommandBuffers written to in different threads must come from different pools
	vkCommandBuffer_t cmd;


	// get vertex buffer
	const vertCacheHandle_t vbHandle = surf->vertexCache;
	idVertexBuffer * vertexBuffer;
	if( vertexCache.CacheIsStatic( vbHandle ) )
	{
		vertexBuffer = &vertexCache.staticData.vertexBuffer;
	}
	else {
		const uint64 frameNum = ( int )( vbHandle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
		if( frameNum != ( ( vertexCache.currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) )
		{
			idLib::Warning( "idRenderBackend::DrawElementsWithCounters, vertexBuffer == NULL" );
			return;
		}
		vertexBuffer = &vertexCache.frameData[ vertexCache.drawListNum ].vertexBuffer;
	}
	const int32 vertOffset = ( int )( vbHandle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;

	// get index buffer
	const vertCacheHandle_t ibHandle = surf->indexCache;
	idIndexBuffer * indexBuffer;
	if( vertexCache.CacheIsStatic( ibHandle ) )
	{
		indexBuffer = &vertexCache.staticData.indexBuffer;
	}
	else {
		const uint64 frameNum = ( int )( ibHandle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
		if( frameNum != ( ( vertexCache.currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) )
		{
			idLib::Warning( "idRenderBackend::DrawElementsWithCounters, indexBuffer == NULL" );
			return;
		}
		indexBuffer = &vertexCache.frameData[ vertexCache.drawListNum ].indexBuffer;
	}
	const int32 indexOffset = ( int )( ibHandle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;

	RENDERLOG_PRINT( "Binding Buffers(%d): %p:%i %p:%i\n", surf->numIndexes, vertexBuffer, vertOffset, indexBuffer, indexOffset );

	//const renderProg_t & prog = renderProgManager.GetCurrentRenderProg();

	if( surf->HasSkinning() )
	{
		assert( GL_GetCurrentRenderProgram()->HasHardwareSkinning() );
		if( !GL_GetCurrentRenderProgram()->HasHardwareSkinning() )
			return;
	}
	else {
		assert( !GL_GetCurrentRenderProgram()->HasHardwareSkinning() || GL_GetCurrentRenderProgram()->HasOptionalSkinning() );
		if( GL_GetCurrentRenderProgram()->HasHardwareSkinning() && !GL_GetCurrentRenderProgram()->HasOptionalSkinning() )
			return;
	}

	//vkcontext.jointCacheHandle = surf->jointCache;

	//PrintState( m_glStateBits );
	//renderProgManager.CommitCurrent( m_glStateBits );
	{
		VkPipeline pipeline;
		vkCmdBindPipeline( cmd.GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline );
	} // infrequent


	{
		VkPipelineLayout pipelineLayout;
		VkDescriptorSet descriptorSet;

		static unsigned int unsignedint_temp_4[ 3 ] = { 1971968u, 1972224u };
		vkCmdBindDescriptorSets( cmd.GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0u, 1u, &descriptorSet, 3u, unsignedint_temp_4 );
	}


	cmd.BindVBO( reinterpret_cast<const vkBuffer_t *>( vertexBuffer->GetAPIObject() ), vertexBuffer->GetOffset() );

	cmd.BindIBO( reinterpret_cast<const vkBuffer_t *>( indexBuffer->GetAPIObject() ), indexBuffer->GetOffset(), VK_INDEX_TYPE_UINT16 );

	vkCmdDrawIndexed( cmd.GetHandle(), surf->numIndexes, 1, ( indexOffset >> 1 ), vertOffset / sizeof( idDrawVert ), 0 );

	RENDERLOG_PRINT( "VK_DrawIndexed( indexCount.%i, vertOffset.%f )", surf->numIndexes, vertOffset / sizeof( idDrawVert ) );




}


void VK_ExecuteGraphicsRenderPass( VkCommandBuffer commandBuffer, VkQueryPool queryPool )
{
	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = VkCommandBufferUsageFlags( 0 );
	commandBufferBeginInfo.pInheritanceInfo = nullptr;
	vkBeginCommandBuffer( commandBuffer, &commandBufferBeginInfo );

	///vkCmdWriteTimestamp( commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queryPool, 1u );

	VkPipeline currentPipeline;
	VkViewport currentViewport;
	VkRect2D currentScissor;

	{/*for*/
		VkRenderPass renderPass;
		VkFramebuffer framebuffer;
		VkClearValue clearValues[8];
		uint32_t numClearValues;
		uint32_t width, height;

		VkRenderPassBeginInfo rp_begin_info = {};
		rp_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rp_begin_info.renderPass = renderPass;
		rp_begin_info.framebuffer = framebuffer;
		rp_begin_info.renderArea.offset.x = 0;
		rp_begin_info.renderArea.offset.y = 0;
		rp_begin_info.renderArea.extent.width = width;
		rp_begin_info.renderArea.extent.height = height;
		rp_begin_info.clearValueCount = numClearValues;
		rp_begin_info.pClearValues = clearValues;
		vkCmdBeginRenderPass( commandBuffer, &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE );

		{/*for*/
			struct vkProgram_t {
				VkPipeline pipeline;
				VkPipelineLayout pipelineLayout;
				VkDescriptorSet descriptorSet;
			};
			{ // bind render program
				VkPipeline pipeline;
				vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline );
			}
			{ // bind program resources
				VkPipelineLayout pipelineLayout;
				VkDescriptorSet descriptorSet[1];

				unsigned int dynamicOffsets[ 6 ] = { 1961984u, 1962240u, 0u, 0u, 1962496u, 1962752u };
				vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
					0u, _countof( descriptorSet ), descriptorSet,
					_countof( dynamicOffsets ), dynamicOffsets );

				// Any bindings that were previously applied via these sets are no longer valid.
			}

			static VkViewport VkViewport_temp_7[ 1 ] = { VkViewport{
				/* x = */ 0.0f,
				/* y = */ 0.0f,
				/* width = */ HexToFloat( 0x44D20000/*1680.0f*/ ),
				/* height = */ HexToFloat( 0x44834000/*1050.0f*/ ),
				/* minDepth = */ 0.0f,
				/* maxDepth = */ 1.0f } };
			vkCmdSetViewport( commandBuffer, 0u, 1u, VkViewport_temp_7 );

			static VkRect2D VkRect2D_temp_7[ 1 ] = { VkRect2D{
				/* offset = */ VkOffset2D{
				/* >> x = */ 0,
				/* >> y = */ 0 },
				/* extent = */ VkExtent2D{
				/* >> width = */ 1680u,
				/* >> height = */ 1050u } } };
			vkCmdSetScissor( commandBuffer, 0u, 1u, VkRect2D_temp_7 );

			{ // set dynamic render state
				vkCmdSetStencilCompareMask( commandBuffer, VkStencilFaceFlags( VK_STENCIL_FRONT_AND_BACK ), 255u );
				vkCmdSetStencilReference( commandBuffer, VkStencilFaceFlags( VK_STENCIL_FRONT_AND_BACK ), 0u );
			}

			{
				static unsigned __int64 unsigned__int64_temp_43[ 1 ] = { 0ull };
				vkCmdBindVertexBuffers( commandBuffer, 0u, 1u, &VkBuffer_uid_9456, unsigned__int64_temp_43 );

				vkCmdBindIndexBuffer( commandBuffer, VkBuffer_uid_9455, 0ull, VK_INDEX_TYPE_UINT16 );
			}

			vkCmdDrawIndexed( commandBuffer, 492u, 1u, 0u, 0, 0u );
		}

		vkCmdEndRenderPass( commandBuffer );
	}

	///vkCmdWriteTimestamp( commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, 2u );

	vkEndCommandBuffer( commandBuffer );
}

void VK_ExecuteComputeRenderPass( VkCommandBuffer commandBuffer )
{
	vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, VkPipeline_uid_14330 );

	static unsigned int unsignedint_temp_146[ 2 ] = { 1967616u, 1968384u };
	vkCmdBindDescriptorSets( commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, VkPipelineLayout_uid_14284, 0u, 1u, &VkDescriptorSet_uid_33433, 2u, unsignedint_temp_146 );

	vkCmdDispatch( commandBuffer, 26u, 16u, 1u );
}

/*
============================================================
 VK_DrawZeroOneCubeAuto
============================================================
*/
void VK_DrawZeroOneCubeAuto()
{
	VkCommandBuffer commandBuffer;

	GL_GetCurrentRenderProgram()->CommitUniforms();

	if( backEnd.glState.currentVAO != glConfig.empty_vao )
	{
		///glBindVertexArray( glConfig.empty_vao );
		backEnd.glState.currentVAO = glConfig.empty_vao;

		backEnd.glState.vertexLayout = LAYOUT_UNKNOWN;
		backEnd.glState.currentVertexBuffer = 0;
		backEnd.glState.currentIndexBuffer = 0;
	}

	vkCmdDraw( commandBuffer, 14, 1, 0, 0 );

	RENDERLOG_PRINT( "VK_DrawZeroOneCubeAuto()\n" );
}

/*
============================================================
 VK_DrawScreenTriangleAuto
============================================================
*/
void VK_DrawScreenTriangleAuto()
{
	VkCommandBuffer commandBuffer;

	GL_GetCurrentRenderProgram()->CommitUniforms();

	if( backEnd.glState.currentVAO != glConfig.empty_vao )
	{
		///glBindVertexArray( glConfig.empty_vao );
		backEnd.glState.currentVAO = glConfig.empty_vao;

		backEnd.glState.vertexLayout = LAYOUT_UNKNOWN;
		backEnd.glState.currentVertexBuffer = 0;
		backEnd.glState.currentIndexBuffer = 0;
	}

	vkCmdDraw( commandBuffer, 3, 1, 0, 0 );

	RENDERLOG_PRINT( "VK_DrawScreenTriangleAuto()\n" );
}








//-----------------------------------------------------------------------------
// Thread03RunFrame0Part00
//-----------------------------------------------------------------------------
void Thread03RunFrame0Part00( int threadId )
{
	//BEGIN_DATA_SCOPE_FUNCTION();

	ID_VK_CHECK( vkWaitForFences( VkDevice_uid_5, 1u, &VkFence_uid_72, 1u, 18446744073709551615ull ) );

	static unsigned int current_buffer[ 1 ] = { 1u };
	ID_VK_CHECK( vkAcquireNextImageKHR( VkDevice_uid_5, VkSwapchainKHR_uid_32621, 18446744073709551615ull, VkSemaphore_uid_32617, VkFence( VK_NULL_HANDLE ), current_buffer ) );

#if 0
	// NOTE: Command buffer 0x03cf1728 was recorded in frame during the original capture.
	// NOTE: Command buffer 0x03cf4328 was recorded in frame during the original capture.
	// NOTE: Command buffer 0x0eae1f28 was recorded in frame during the original capture.
	// NOTE: Command buffer 0x0eae4b68 was recorded in frame during the original capture.
	// NOTE: Command buffer 0x0eae6fc8 was recorded in frame during the original capture.
	// NOTE: Command buffer 0x0eae9428 was recorded in frame during the original capture.
	// NOTE: Command buffer 0x0eaeb888 was recorded in frame during the original capture.
	// Host update to mapped memory
	NV_THROW_IF( Serialization::UpdateMappedMemory( VkDevice_uid_5, VkDeviceMemory_uid_78, VkDeviceMemory_uid_78_MappedPointer, 35520512ull, 65536ull, NV_GET_RESOURCE( const uint8_t*, 0 ) ) != VK_SUCCESS, "A Vulkan API call was unsuccessful" );
	NV_THROW_IF( Serialization::UpdateMappedMemory( VkDevice_uid_5, VkDeviceMemory_uid_78, VkDeviceMemory_uid_78_MappedPointer, 36044800ull, 262144ull, NV_GET_RESOURCE( const uint8_t*, 1 ) ) != VK_SUCCESS, "A Vulkan API call was unsuccessful" );
	NV_THROW_IF( Serialization::UpdateMappedMemory( VkDevice_uid_5, VkDeviceMemory_uid_78, VkDeviceMemory_uid_78_MappedPointer, 56819712ull, 65536ull, NV_GET_RESOURCE( const uint8_t*, 2 ) ) != VK_SUCCESS, "A Vulkan API call was unsuccessful" );
	NV_THROW_IF( Serialization::UpdateMappedMemory( VkDevice_uid_5, VkDeviceMemory_uid_78, VkDeviceMemory_uid_78_MappedPointer, 56950784ull, 1114112ull, NV_GET_RESOURCE( const uint8_t*, 3 ) ) != VK_SUCCESS, "A Vulkan API call was unsuccessful" );
	NV_THROW_IF( Serialization::UpdateMappedMemory( VkDevice_uid_5, VkDeviceMemory_uid_78, VkDeviceMemory_uid_78_MappedPointer, 58458112ull, 65536ull, NV_GET_RESOURCE( const uint8_t*, 4 ) ) != VK_SUCCESS, "A Vulkan API call was unsuccessful" );
	NV_THROW_IF( Serialization::UpdateMappedMemory( VkDevice_uid_5, VkDeviceMemory_uid_78, VkDeviceMemory_uid_78_MappedPointer, 59179008ull, 65536ull, NV_GET_RESOURCE( const uint8_t*, 5 ) ) != VK_SUCCESS, "A Vulkan API call was unsuccessful" );
	NV_THROW_IF( Serialization::UpdateMappedMemory( VkDevice_uid_5, VkDeviceMemory_uid_78, VkDeviceMemory_uid_78_MappedPointer, 59310080ull, 131072ull, NV_GET_RESOURCE( const uint8_t*, 6 ) ) != VK_SUCCESS, "A Vulkan API call was unsuccessful" );
	NV_THROW_IF( Serialization::UpdateMappedMemory( VkDevice_uid_5, VkDeviceMemory_uid_78, VkDeviceMemory_uid_78_MappedPointer, 64815104ull, 196608ull, NV_GET_RESOURCE( const uint8_t*, 7 ) ) != VK_SUCCESS, "A Vulkan API call was unsuccessful" );

	// Host update to mapped memory
	NV_THROW_IF( Serialization::UpdateMappedMemory( VkDevice_uid_5, VkDeviceMemory_uid_16794, VkDeviceMemory_uid_16794_MappedPointer, 18284544ull, 65536ull, NV_GET_RESOURCE( const uint8_t*, 8 ) ) != VK_SUCCESS, "A Vulkan API call was unsuccessful" );
	NV_THROW_IF( Serialization::UpdateMappedMemory( VkDevice_uid_5, VkDeviceMemory_uid_16794, VkDeviceMemory_uid_16794_MappedPointer, 20840448ull, 65536ull, NV_GET_RESOURCE( const uint8_t*, 9 ) ) != VK_SUCCESS, "A Vulkan API call was unsuccessful" );
	NV_THROW_IF( Serialization::UpdateMappedMemory( VkDevice_uid_5, VkDeviceMemory_uid_16794, VkDeviceMemory_uid_16794_MappedPointer, 37879808ull, 65536ull, NV_GET_RESOURCE( const uint8_t*, 10 ) ) != VK_SUCCESS, "A Vulkan API call was unsuccessful" );
	NV_THROW_IF( Serialization::UpdateMappedMemory( VkDevice_uid_5, VkDeviceMemory_uid_16794, VkDeviceMemory_uid_16794_MappedPointer, 48955392ull, 262144ull, NV_GET_RESOURCE( const uint8_t*, 11 ) ) != VK_SUCCESS, "A Vulkan API call was unsuccessful" );

	static VkMappedMemoryRange VkMappedMemoryRange_temp_1[ 2 ] = { VkMappedMemoryRange{
		/* sType = */ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		/* pNext = */ nullptr,
		/* memory = */ VkDeviceMemory_uid_78,
		/* offset = */ 0ull,
		/* size = */ 67108864ull }, VkMappedMemoryRange{
		/* sType = */ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		/* pNext = */ nullptr,
		/* memory = */ VkDeviceMemory_uid_16794,
		/* offset = */ 0ull,
		/* size = */ 67108864ull } };
	ID_VK_CHECK( vkFlushMappedMemoryRanges( VkDevice_uid_5, 2u, VkMappedMemoryRange_temp_1 ) );

	// NOTE: Command buffer 0x0eafafa8 was recorded in frame during the original capture.
	// Host update to mapped memory
	NV_THROW_IF( Serialization::UpdateMappedMemory( VkDevice_uid_5, VkDeviceMemory_uid_70, VkDeviceMemory_uid_70_MappedPointer, 0ull, 13056ull, NV_GET_RESOURCE( const uint8_t*, 12 ) ) != VK_SUCCESS, "A Vulkan API call was unsuccessful" );

	static VkMappedMemoryRange VkMappedMemoryRange_temp_2[ 1 ] = { VkMappedMemoryRange{
		/* sType = */ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		/* pNext = */ nullptr,
		/* memory = */ VkDeviceMemory_uid_70,
		/* offset = */ 0ull,
		/* size = */ 13056ull } };
	ID_VK_CHECK( vkFlushMappedMemoryRanges( VkDevice_uid_5, 1u, VkMappedMemoryRange_temp_2 ) );

	// Host update to mapped memory
	NV_THROW_IF( Serialization::UpdateMappedMemory( VkDevice_uid_5, VkDeviceMemory_uid_70, VkDeviceMemory_uid_70_MappedPointer, 0ull, 65536ull, NV_GET_RESOURCE( const uint8_t*, 13 ) ) != VK_SUCCESS, "A Vulkan API call was unsuccessful" );

	static VkSubmitInfo VkSubmitInfo_temp_1[ 1 ] = { VkSubmitInfo{
		/* sType = */ VK_STRUCTURE_TYPE_SUBMIT_INFO,
		/* pNext = */ nullptr,
		/* waitSemaphoreCount = */ 0u,
		/* pWaitSemaphores = */ nullptr,
		/* pWaitDstStageMask = */ nullptr,
		/* commandBufferCount = */ 1u,
		/* pCommandBuffers = */ NV_TO_PTR( VkCommandBuffer_uid_71 ),
		/* signalSemaphoreCount = */ 0u,
		/* pSignalSemaphores = */ nullptr } };
	ID_VK_CHECK( vkQueueSubmit( VkQueue_uid_14, 1u, VkSubmitInfo_temp_1, VkFence_uid_72 ) );

	// Host update to mapped memory
	NV_THROW_IF( Serialization::UpdateMappedMemory( VkDevice_uid_5, VkDeviceMemory_uid_78, VkDeviceMemory_uid_78_MappedPointer, 51838976ull, 131072ull, NV_GET_RESOURCE( const uint8_t*, 14 ) ) != VK_SUCCESS, "A Vulkan API call was unsuccessful" );
#endif

	VkFence VkFence_uid_82;
	VkSemaphore VkSemaphore_uid_32617;
	VkSemaphore VkSemaphore_uid_32619;
	VkSwapchainKHR VkSwapchainKHR_uid_32621;
	VkQueue VkQueue_uid_14;
	VkCommandBuffer renderCommandBuffers[7]; // VkCommandBuffer_uid_16, VkCommandBuffer_uid_23, VkCommandBuffer_uid_30, VkCommandBuffer_uid_37, VkCommandBuffer_uid_44, VkCommandBuffer_uid_51, VkCommandBuffer_uid_58

	VkPipelineStageFlags pWaitDstStageMask[] = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
	static VkSubmitInfo submitInfo[ 1 ] = { VkSubmitInfo{
		/* sType = */ VK_STRUCTURE_TYPE_SUBMIT_INFO,
		/* pNext = */ nullptr,
		/* waitSemaphoreCount = */ 1u,
		/* pWaitSemaphores = */ &VkSemaphore_uid_32617,
		/* pWaitDstStageMask = */ pWaitDstStageMask,
		/* commandBufferCount = */ _countof( renderCommandBuffers ),
		/* pCommandBuffers = */ renderCommandBuffers,
		/* signalSemaphoreCount = */ 1u,
		/* pSignalSemaphores = */ &VkSemaphore_uid_32619 }
	};
	ID_VK_CHECK( vkQueueSubmit( VkQueue_uid_14, 1u, submitInfo, VkFence_uid_82 ) );

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.waitSemaphoreCount = 1u;
	presentInfo.pWaitSemaphores = &VkSemaphore_uid_32619;
	presentInfo.swapchainCount = 1u,
	presentInfo.pSwapchains = &VkSwapchainKHR_uid_32621;
	presentInfo.pImageIndices = current_buffer;
	presentInfo.pResults = nullptr;
	ID_VK_CHECK( vkQueuePresentKHR( VkQueue_uid_14, &presentInfo ) );

	//NV_SIGNAL_FRAME_COMPLETE();
}








struct vkFramebufferLayout_t
{
	VkFramebuffer handle;
	VkRenderPass renderPassHandle;
};




void VK_CreateRenderDestination( uint32 width, uint32 height,
	bool bClearColors, bool bClearDepth, bool bClearStencil
) {
	VkDevice device;

	VkFormat colorImageFormat;
	VkFormat depthImageFormat;
	uint32 numAttachments = 0;

	VkAttachmentDescription attachments[ 8 + 1 ] = {};
	VkAttachmentReference colorRefs[ 8 ] = {};
	VkAttachmentReference depthRef = {};

	for( uint32 i = 0; i < numAttachments; i++ )
	{
		colorRefs[ i ].attachment = i;
		colorRefs[ i ].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		attachments[ i ].format = colorImageFormat;
		attachments[ i ].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[ i ].loadOp = bClearColors ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[ i ].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[ i ].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[ i ].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[ i ].flags = 0;
	}

	depthRef.attachment = numAttachments;
	depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	attachments[ depthRef.attachment ].format = depthImageFormat;
	attachments[ depthRef.attachment ].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[ depthRef.attachment ].loadOp = bClearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[ depthRef.attachment ].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[ depthRef.attachment ].stencilLoadOp = bClearStencil ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[ depthRef.attachment ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[ depthRef.attachment ].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[ depthRef.attachment ].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachments[ depthRef.attachment ].flags = 0;



	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.flags = 0;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = NULL;
	subpass.colorAttachmentCount = numAttachments;
	subpass.pColorAttachments = colorRefs;
	subpass.pResolveAttachments = NULL;
	subpass.pDepthStencilAttachment = &depthRef;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = NULL;

	VkRenderPassCreateInfo rp_info = {};
	rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rp_info.attachmentCount = numAttachments;
	rp_info.pAttachments = attachments;
	rp_info.subpassCount = 1;
	rp_info.pSubpasses = &subpass;
	rp_info.dependencyCount = 0;
	rp_info.pDependencies = NULL;

	VkRenderPass renderPass;
	auto res = vkCreateRenderPass( device, &rp_info, NULL, &renderPass );
	assert( res == VK_SUCCESS );


	uint32 numImgAttachments;
	VkImageView imgAttachments[ 8 + 1 ];

	VkFramebufferCreateInfo fb_info = {};
	fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fb_info.pNext = NULL;
	fb_info.renderPass = renderPass;
	fb_info.attachmentCount = numImgAttachments;
	fb_info.pAttachments = imgAttachments;
	fb_info.width = width;
	fb_info.height = height;
	fb_info.layers = 1;

	VkFramebuffer framebuffer;
	res = vkCreateFramebuffer( device, &fb_info, NULL, &framebuffer );
	assert( res == VK_SUCCESS );
}


static void GetStencilOpState( uint64 stencilBits, VkStencilOpState & state )
{
	switch( stencilBits & GLS_STENCIL_OP_FAIL_BITS )
	{
		case GLS_STENCIL_OP_FAIL_KEEP:		state.failOp = VK_STENCIL_OP_KEEP; break;
		case GLS_STENCIL_OP_FAIL_ZERO:		state.failOp = VK_STENCIL_OP_ZERO; break;
		case GLS_STENCIL_OP_FAIL_REPLACE:	state.failOp = VK_STENCIL_OP_REPLACE; break;
		case GLS_STENCIL_OP_FAIL_INCR:		state.failOp = VK_STENCIL_OP_INCREMENT_AND_CLAMP; break;
		case GLS_STENCIL_OP_FAIL_DECR:		state.failOp = VK_STENCIL_OP_DECREMENT_AND_CLAMP; break;
		case GLS_STENCIL_OP_FAIL_INVERT:	state.failOp = VK_STENCIL_OP_INVERT; break;
		case GLS_STENCIL_OP_FAIL_INCR_WRAP: state.failOp = VK_STENCIL_OP_INCREMENT_AND_WRAP; break;
		case GLS_STENCIL_OP_FAIL_DECR_WRAP: state.failOp = VK_STENCIL_OP_DECREMENT_AND_WRAP; break;
	}
	switch( stencilBits & GLS_STENCIL_OP_ZFAIL_BITS )
	{
		case GLS_STENCIL_OP_ZFAIL_KEEP:		state.depthFailOp = VK_STENCIL_OP_KEEP; break;
		case GLS_STENCIL_OP_ZFAIL_ZERO:		state.depthFailOp = VK_STENCIL_OP_ZERO; break;
		case GLS_STENCIL_OP_ZFAIL_REPLACE:	state.depthFailOp = VK_STENCIL_OP_REPLACE; break;
		case GLS_STENCIL_OP_ZFAIL_INCR:		state.depthFailOp = VK_STENCIL_OP_INCREMENT_AND_CLAMP; break;
		case GLS_STENCIL_OP_ZFAIL_DECR:		state.depthFailOp = VK_STENCIL_OP_DECREMENT_AND_CLAMP; break;
		case GLS_STENCIL_OP_ZFAIL_INVERT:	state.depthFailOp = VK_STENCIL_OP_INVERT; break;
		case GLS_STENCIL_OP_ZFAIL_INCR_WRAP:state.depthFailOp = VK_STENCIL_OP_INCREMENT_AND_WRAP; break;
		case GLS_STENCIL_OP_ZFAIL_DECR_WRAP:state.depthFailOp = VK_STENCIL_OP_DECREMENT_AND_WRAP; break;
	}
	switch( stencilBits & GLS_STENCIL_OP_PASS_BITS )
	{
		case GLS_STENCIL_OP_PASS_KEEP:		state.passOp = VK_STENCIL_OP_KEEP; break;
		case GLS_STENCIL_OP_PASS_ZERO:		state.passOp = VK_STENCIL_OP_ZERO; break;
		case GLS_STENCIL_OP_PASS_REPLACE:	state.passOp = VK_STENCIL_OP_REPLACE; break;
		case GLS_STENCIL_OP_PASS_INCR:		state.passOp = VK_STENCIL_OP_INCREMENT_AND_CLAMP; break;
		case GLS_STENCIL_OP_PASS_DECR:		state.passOp = VK_STENCIL_OP_DECREMENT_AND_CLAMP; break;
		case GLS_STENCIL_OP_PASS_INVERT:	state.passOp = VK_STENCIL_OP_INVERT; break;
		case GLS_STENCIL_OP_PASS_INCR_WRAP:	state.passOp = VK_STENCIL_OP_INCREMENT_AND_WRAP; break;
		case GLS_STENCIL_OP_PASS_DECR_WRAP:	state.passOp = VK_STENCIL_OP_DECREMENT_AND_WRAP; break;
	}
}



/*
=============
CreateVertexDescriptions
=============
*/
enum vertexLayoutType_e
{
	LAYOUT_UNKNOWN = -1,

	LAYOUT_DRAW_VERT,
	LAYOUT_DRAW_SHADOW_VERT,
	LAYOUT_DRAW_SHADOW_VERT_SKINNED,

	NUM_VERTEX_LAYOUTS
};
struct vkVertexLayout_t
{
	VkVertexInputBindingDescription binding;
	idStaticList<VkVertexInputAttributeDescription, 16> attributes;
}
vertexLayouts[ NUM_VERTEX_LAYOUTS ];

static void CreateVertexDescriptions()
{
	VkVertexInputBindingDescription binding = {};
	VkVertexInputAttributeDescription attribute = {};

	{
		auto & layout = vertexLayouts[ LAYOUT_DRAW_VERT ];

		uint32 locationNo = 0;

		layout.binding.binding = 0;
		layout.binding.stride = sizeof( idDrawVert );
		layout.binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		// Position
		attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute.location = locationNo++;
		attribute.offset = DRAWVERT_XYZ_OFFSET;
		layout.attributes.Append( attribute );

		// TexCoord
		attribute.format = VK_FORMAT_R16G16_SFLOAT;
		attribute.location = locationNo++;
		attribute.offset = DRAWVERT_ST_OFFSET;
		layout.attributes.Append( attribute );

		// Normal
		attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = DRAWVERT_NORMAL_OFFSET;
		layout.attributes.Append( attribute );

		// Tangent
		attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = DRAWVERT_TANGENT_OFFSET;
		layout.attributes.Append( attribute );

		// Color1
		attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = DRAWVERT_COLOR_OFFSET;
		layout.attributes.Append( attribute );

		// Color2
		attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = DRAWVERT_COLOR2_OFFSET;
		layout.attributes.Append( attribute );
	}

	{
		auto & layout = vertexLayouts[ LAYOUT_DRAW_SHADOW_VERT ];

		layout.binding.binding = 0;
		layout.binding.stride = sizeof( idShadowVert );
		layout.binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		// Position
		attribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attribute.location = 0;
		attribute.offset = 0;
		layout.attributes.Append( attribute );
	}

	{
		auto & layout = vertexLayouts[ LAYOUT_DRAW_SHADOW_VERT_SKINNED ];

		uint32 locationNo = 0;

		layout.binding.stride = sizeof( idShadowVertSkinned );
		layout.binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		// Position
		attribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attribute.location = locationNo++;
		attribute.offset = SHADOWVERTSKINNED_XYZW_OFFSET;
		layout.attributes.Append( attribute );

		// Color1
		attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = SHADOWVERTSKINNED_COLOR_OFFSET;
		layout.attributes.Append( attribute );

		// Color2
		attribute.format = VK_FORMAT_R8G8B8A8_UNORM;
		attribute.location = locationNo++;
		attribute.offset = SHADOWVERTSKINNED_COLOR2_OFFSET;
		layout.attributes.Append( attribute );
	}
}

void VK_GraphicsPipeline( uint64 stateBits, vertexLayoutType_e vertLayout )
{
	VkDevice device;
	VkRenderPass renderPass;

	// Dynamic State

	idStaticList<VkDynamicState, VK_DYNAMIC_STATE_RANGE_SIZE> dynamicStates;

	dynamicStates.Append( VK_DYNAMIC_STATE_VIEWPORT );
	dynamicStates.Append( VK_DYNAMIC_STATE_SCISSOR );
	if( stateBits & GLS_POLYGON_OFFSET )
		dynamicStates.Append( VK_DYNAMIC_STATE_DEPTH_BIAS );
	if( stateBits & GLS_DEPTH_TEST_MASK )
		dynamicStates.Append( VK_DYNAMIC_STATE_DEPTH_BOUNDS );
	//dynamicStates.Append( VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK );
	//dynamicStates.Append( VK_DYNAMIC_STATE_STENCIL_WRITE_MASK );
	//dynamicStates.Append( VK_DYNAMIC_STATE_STENCIL_REFERENCE );

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pDynamicStates = dynamicStates.Ptr();
	dynamicState.dynamicStateCount = dynamicStates.Num();

	// Pipeline Vertex Input State

	VkPipelineVertexInputStateCreateInfo vi = {};
	vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vi.vertexBindingDescriptionCount = 1;
	vi.pVertexBindingDescriptions = &vertexLayouts[ vertLayout ].binding;
	vi.vertexAttributeDescriptionCount = vertexLayouts[ vertLayout ].attributes.Num();
	vi.pVertexAttributeDescriptions = vertexLayouts[ vertLayout ].attributes.Ptr();

	// Pipeline Vertex Input Assembly State

	VkPipelineInputAssemblyStateCreateInfo ia = {};
	ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	ia.primitiveRestartEnable = VK_FALSE;
	ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Pipeline Rasterization State

	VkPipelineRasterizationStateCreateInfo rs = {};
	rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rs.polygonMode = ( stateBits & GLS_POLYMODE_LINE )? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
	if( stateBits & GLS_BACKSIDED ) {
		rs.cullMode = VK_CULL_MODE_FRONT_BIT;
	} else if( stateBits & GLS_TWOSIDED ) {
		rs.cullMode = VK_CULL_MODE_NONE;
	} else {
		rs.cullMode = VK_CULL_MODE_BACK_BIT;
	}
	rs.frontFace = ( stateBits & GLS_MIRROR_VIEW )? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
	rs.rasterizerDiscardEnable = VK_FALSE;
	rs.depthBiasEnable = ( stateBits & GLS_POLYGON_OFFSET ) != 0;
	rs.depthBiasConstantFactor = 0;
	rs.depthBiasClamp = 0;
	rs.depthBiasSlopeFactor = 0;
	rs.lineWidth = 1.0f;

	// Pipeline Color Blend State

	VkPipelineColorBlendAttachmentState att_state = {};
	{
		att_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		switch( stateBits & GLS_SRCBLEND_BITS )
		{
			case GLS_SRCBLEND_ONE:					att_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; break;
			case GLS_SRCBLEND_ZERO:					att_state.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO; break;
			case GLS_SRCBLEND_DST_COLOR:			att_state.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR; break;
			case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:	att_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR; break;
			case GLS_SRCBLEND_SRC_ALPHA:			att_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; break;
			case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:	att_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; break;
			case GLS_SRCBLEND_DST_ALPHA:			att_state.srcColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA; break;
			case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:	att_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA; break;
		}

		att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		switch( stateBits & GLS_DSTBLEND_BITS )
		{
			case GLS_DSTBLEND_ZERO:					att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; break;
			case GLS_DSTBLEND_ONE:					att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE; break;
			case GLS_DSTBLEND_SRC_COLOR:			att_state.dstColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR; break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:	att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR; break;
			case GLS_DSTBLEND_SRC_ALPHA:			att_state.dstColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:	att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; break;
			case GLS_DSTBLEND_DST_ALPHA:			att_state.dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA; break;
			case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:	att_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA; break;
		}

		att_state.colorBlendOp = VK_BLEND_OP_ADD;
		switch( stateBits & GLS_BLENDOP_BITS )
		{
			case GLS_BLENDOP_MIN:    att_state.colorBlendOp = VK_BLEND_OP_MIN; break;
			case GLS_BLENDOP_MAX:    att_state.colorBlendOp = VK_BLEND_OP_MAX; break;
			case GLS_BLENDOP_ADD:    att_state.colorBlendOp = VK_BLEND_OP_ADD; break;
			case GLS_BLENDOP_SUB:    att_state.colorBlendOp = VK_BLEND_OP_SUBTRACT; break;
			case GLS_BLENDOP_INVSUB: att_state.colorBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT; break;
		}

		att_state.blendEnable = ( att_state.srcColorBlendFactor != VK_BLEND_FACTOR_ONE || att_state.dstColorBlendFactor != VK_BLEND_FACTOR_ZERO );

		att_state.alphaBlendOp = att_state.colorBlendOp;
		att_state.srcAlphaBlendFactor = att_state.srcColorBlendFactor;
		att_state.dstAlphaBlendFactor = att_state.dstColorBlendFactor;

		// Color Mask
		att_state.colorWriteMask |= ( stateBits & GLS_REDMASK )? 0 : VK_COLOR_COMPONENT_R_BIT;
		att_state.colorWriteMask |= ( stateBits & GLS_GREENMASK )? 0 : VK_COLOR_COMPONENT_G_BIT;
		att_state.colorWriteMask |= ( stateBits & GLS_BLUEMASK )? 0 : VK_COLOR_COMPONENT_B_BIT;
		att_state.colorWriteMask |= ( stateBits & GLS_ALPHAMASK )? 0 : VK_COLOR_COMPONENT_A_BIT;
	}

	VkPipelineColorBlendStateCreateInfo cb = {};
	cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

	cb.attachmentCount = 1;
	cb.pAttachments = &att_state;

	//cb.logicOpEnable = VK_FALSE;
	//cb.logicOp = VK_LOGIC_OP_NO_OP;

	cb.blendConstants[ 0 ] = 1.0f;
	cb.blendConstants[ 1 ] = 1.0f;
	cb.blendConstants[ 2 ] = 1.0f;
	cb.blendConstants[ 3 ] = 1.0f;

	// Pipeline Depth Stencil State

	VkPipelineDepthStencilStateCreateInfo ds = {};
	ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	{
		ds.depthCompareOp = VK_COMPARE_OP_ALWAYS;
		switch( stateBits & GLS_DEPTHFUNC_BITS )
		{
			case GLS_DEPTHFUNC_LEQUAL:		ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; break;
			case GLS_DEPTHFUNC_ALWAYS:		ds.depthCompareOp = VK_COMPARE_OP_ALWAYS; break;
			case GLS_DEPTHFUNC_GEQUAL:		ds.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL; break;
			case GLS_DEPTHFUNC_EQUAL:		ds.depthCompareOp = VK_COMPARE_OP_EQUAL; break;
			case GLS_DEPTHFUNC_NOTEQUAL:	ds.depthCompareOp = VK_COMPARE_OP_NOT_EQUAL; break;
			case GLS_DEPTHFUNC_GREATER:		ds.depthCompareOp = VK_COMPARE_OP_GREATER; break;
			case GLS_DEPTHFUNC_LESS:		ds.depthCompareOp = VK_COMPARE_OP_LESS; break;
			case GLS_DEPTHFUNC_NEVER:		ds.depthCompareOp = VK_COMPARE_OP_NEVER; break;
		}

		VkCompareOp stencilCompareOp = VK_COMPARE_OP_ALWAYS;
		switch( stateBits & GLS_STENCIL_FUNC_BITS )
		{
			case GLS_STENCIL_FUNC_ALWAYS:	stencilCompareOp = VK_COMPARE_OP_ALWAYS; break;
			case GLS_STENCIL_FUNC_LESS:		stencilCompareOp = VK_COMPARE_OP_LESS; break;
			case GLS_STENCIL_FUNC_LEQUAL:	stencilCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; break;
			case GLS_STENCIL_FUNC_GREATER:	stencilCompareOp = VK_COMPARE_OP_GREATER; break;
			case GLS_STENCIL_FUNC_GEQUAL:	stencilCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL; break;
			case GLS_STENCIL_FUNC_EQUAL:	stencilCompareOp = VK_COMPARE_OP_EQUAL; break;
			case GLS_STENCIL_FUNC_NOTEQUAL: stencilCompareOp = VK_COMPARE_OP_NOT_EQUAL; break;
			case GLS_STENCIL_FUNC_NEVER:	stencilCompareOp = VK_COMPARE_OP_NEVER; break;
		}

		ds.depthTestEnable = ( stateBits & GLS_DISABLE_DEPTHTEST ) == 0;
		ds.depthWriteEnable = ( stateBits & GLS_DEPTHMASK ) == 0;

		ds.depthBoundsTestEnable = ( stateBits & GLS_DEPTH_TEST_MASK ) != 0;
		ds.minDepthBounds = 0.0f;
		ds.maxDepthBounds = 1.0f;

		ds.stencilTestEnable = ( stateBits & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS ) ) != 0;

		uint32 ref = uint32( ( stateBits & GLS_STENCIL_FUNC_REF_BITS ) >> GLS_STENCIL_FUNC_REF_SHIFT );
		uint32 mask = uint32( ( stateBits & GLS_STENCIL_FUNC_MASK_BITS ) >> GLS_STENCIL_FUNC_MASK_SHIFT );

		if( stateBits & GLS_SEPARATE_STENCIL )
		{
			GetStencilOpState( stateBits & GLS_STENCIL_FRONT_OPS, ds.front );
			ds.front.writeMask = 0xFFFFFFFF;
			ds.front.compareOp = stencilCompareOp;
			ds.front.compareMask = mask;
			ds.front.reference = ref;

			GetStencilOpState( ( stateBits & GLS_STENCIL_BACK_OPS ) >> 12, ds.back );
			ds.back.writeMask = 0xFFFFFFFF;
			ds.back.compareOp = stencilCompareOp;
			ds.back.compareMask = mask;
			ds.back.reference = ref;
		}
		else {
			GetStencilOpState( stateBits, ds.front );
			ds.front.writeMask = 0xFFFFFFFF;
			ds.front.compareOp = stencilCompareOp;
			ds.front.compareMask = mask;
			ds.front.reference = ref;
			ds.back = ds.front;
		}
	}

	// Pipeline Multisample State

	VkPipelineMultisampleStateCreateInfo ms = {};
	ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	ms.pSampleMask = NULL;
	ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	ms.sampleShadingEnable = VK_FALSE;
	ms.alphaToCoverageEnable = VK_FALSE; // GLS_ALPHA_COVERAGE
	ms.alphaToOneEnable = VK_FALSE;
	ms.minSampleShading = 0.0;
	/*if( vkcontext.supersampling )
	{
		ms.sampleShadingEnable = VK_TRUE;
		ms.minSampleShading = 1.0f;
	}*/

	// Pipeline Viewport State

	VkPipelineViewportStateCreateInfo vp = {};
	vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vp.viewportCount = 1;
	vp.scissorCount = 1;
	vp.pScissors = NULL;
	vp.pViewports = NULL;


	// Create Graphics Pipeline

	class vkResourceBindings_t
	{
		idList<VkDescriptorSetLayoutBinding> data;

	public:
		uint32 GetCount() const { return data.Num(); }
		const VkDescriptorSetLayoutBinding * GetData() const { return data.Ptr(); }

		void AddUBO( uint32 binding, VkShaderStageFlags stageFlags )
		{
			auto & layout_binding = data.Alloc();
			layout_binding.binding = binding;
			layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			layout_binding.descriptorCount = 1;
			layout_binding.stageFlags = stageFlags; // VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			layout_binding.pImmutableSamplers = NULL;
		}
		void AddTexture( uint32 binding, VkShaderStageFlags stageFlags )
		{
			auto & layout_binding = data.Alloc();
			layout_binding.binding = binding;
			layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			layout_binding.descriptorCount = 1;
			layout_binding.stageFlags = stageFlags; // VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			layout_binding.pImmutableSamplers = NULL;
		}
		void AddSampler( uint32 binding, VkShaderStageFlags stageFlags )
		{
			auto & layout_binding = data.Alloc();
			layout_binding.binding = binding;
			layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			layout_binding.descriptorCount = 1;
			layout_binding.stageFlags = stageFlags; // VK_SHADER_STAGE_FRAGMENT_BIT;
			layout_binding.pImmutableSamplers = NULL;
		}
		void AddTextureSampler( uint32 binding, VkShaderStageFlags stageFlags )
		{
			auto & layout_binding = data.Alloc();
			layout_binding.binding = binding;
			layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			layout_binding.descriptorCount = 1;
			layout_binding.stageFlags = stageFlags; // VK_SHADER_STAGE_FRAGMENT_BIT;
			layout_binding.pImmutableSamplers = NULL;
		}
	};
	vkResourceBindings_t bindings;
	VkDescriptorSetLayout bindingsLayout;
	VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
	descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_layout.pNext = NULL;
	descriptor_layout.bindingCount = bindings.GetCount();
	descriptor_layout.pBindings = bindings.GetData();
	auto res = vkCreateDescriptorSetLayout( device, &descriptor_layout, NULL, &bindingsLayout );

		VkDescriptorPool descriptorPool;
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.pNext = nullptr;
		descriptorSetAllocateInfo.descriptorPool = descriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = 1u;
		descriptorSetAllocateInfo.pSetLayouts = &bindingsLayout;

		VkDescriptorSet descriptorSet;
		res = vkAllocateDescriptorSets( device, &descriptorSetAllocateInfo, &descriptorSet );
		assert( res == VK_SUCCESS );
		{
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = VkBuffer();
			bufferInfo.offset = 0ull;
			bufferInfo.range = 96ull;

			VkWriteDescriptorSet updateInfo = {};
			updateInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			updateInfo.pNext = nullptr;
			updateInfo.dstSet = descriptorSet;

			updateInfo.dstBinding = 0u;
			updateInfo.dstArrayElement = 0u;
			updateInfo.descriptorCount = 1u;
			updateInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			updateInfo.pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets( device, 1u, &updateInfo, 0u, nullptr );
		}
		{
			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			imageInfo.imageView = VkImageView();
			imageInfo.sampler = VkSampler();

			VkWriteDescriptorSet updateInfo = {};
			updateInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			updateInfo.pNext = nullptr;
			updateInfo.dstSet = descriptorSet;

			updateInfo.dstBinding = 1u;
			updateInfo.dstArrayElement = 0u;
			updateInfo.descriptorCount = 1u;
			updateInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			updateInfo.pImageInfo = &imageInfo;

			vkUpdateDescriptorSets( device, 1u, &updateInfo, 0u, nullptr );
		}

	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
	pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pPipelineLayoutCreateInfo.pNext = NULL;
	pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pPipelineLayoutCreateInfo.pPushConstantRanges = NULL;
	pPipelineLayoutCreateInfo.setLayoutCount = 1;
	pPipelineLayoutCreateInfo.pSetLayouts = &bindingsLayout;

	VkPipelineLayout pipeline_layout;
	res = vkCreatePipelineLayout( device, &pPipelineLayoutCreateInfo, NULL, &pipeline_layout );
	assert( res == VK_SUCCESS );

	struct vkProgram_t
	{
		VkShaderModule saders[ SHT_MAX ];

		void GetShaderStages( idStaticList<VkPipelineShaderStageCreateInfo, SHT_MAX> & list ) const
		{
			VkPipelineShaderStageCreateInfo stage = {};
			stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stage.pName = "main";

			if( saders[ SHT_VERTEX ] )
			{
				stage.module = saders[ SHT_VERTEX ];
				stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
				list.Append( stage );
			}
			if( saders[ SHT_TESS_CTRL ] )
			{
				stage.module = saders[ SHT_TESS_CTRL ];
				stage.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
				list.Append( stage );
			}
			if( saders[ SHT_TESS_EVAL ] )
			{
				stage.module = saders[ SHT_TESS_EVAL ];
				stage.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
				list.Append( stage );
			}
			if( saders[ SHT_GEOMETRY ] )
			{
				stage.module = saders[ SHT_GEOMETRY ];
				stage.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
				list.Append( stage );
			}
			if( saders[ SHT_FRAGMENT ] )
			{
				stage.module = saders[ SHT_FRAGMENT ];
				stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				list.Append( stage );
			}
		}
	} prog;

	idStaticList<VkPipelineShaderStageCreateInfo, SHT_MAX> shaders;
	prog.GetShaderStages( shaders );


	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	pipelineInfo.layout = pipeline_layout;
	pipelineInfo.pVertexInputState = &vi;
	pipelineInfo.pInputAssemblyState = &ia;
	pipelineInfo.pRasterizationState = &rs;
	pipelineInfo.pColorBlendState = &cb;
	pipelineInfo.pTessellationState = NULL;
	pipelineInfo.pMultisampleState = &ms;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.pViewportState = &vp;
	pipelineInfo.pDepthStencilState = &ds;
	pipelineInfo.pStages = shaders.Ptr();
	pipelineInfo.stageCount = shaders.Num();
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;

	VkPipeline pipeline;
	res = vkCreateGraphicsPipelines( device, NULL, 1, &pipelineInfo, NULL, &pipeline );
	assert( res == VK_SUCCESS );
}






void VK_() // Translucency and Deferred Shading Setup
{
	VkDevice device;

	enum {
		kAttachment_BACK = 0,
		kAttachment_DEPTH = 1,
		kAttachment_GBUFFER = 2,
		kAttachment_TRANSLUCENCY = 3,
		kAttachment_OPAQUE = 4
	};
	enum {
		kSubpass_DEPTH = 0,
		kSubpass_GBUFFER = 1,
		kSubpass_LIGHTING = 2,
		kSubpass_TRANSLUCENTS = 3,
		kSubpass_COMPOSITE = 4
	};

	static const VkAttachmentDescription attachments[] =
	{
		// Back buffer
		{
			0, // flags
			VK_FORMAT_R8G8B8A8_UNORM, // format
			VK_SAMPLE_COUNT_1_BIT, // samples
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, // loadOp
			VK_ATTACHMENT_STORE_OP_STORE, // storeOp
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, // stencilLoadOp
			VK_ATTACHMENT_STORE_OP_DONT_CARE, // stencilStoreOp
			VK_IMAGE_LAYOUT_UNDEFINED, // initialLayout
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR // finalLayout
		},
		// Depth buffer
		{
			0, // flags
			VK_FORMAT_D32_SFLOAT, // format
			VK_SAMPLE_COUNT_1_BIT, // samples
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, // loadOp
			VK_ATTACHMENT_STORE_OP_DONT_CARE, // storeOp
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, // stencilLoadOp
			VK_ATTACHMENT_STORE_OP_DONT_CARE, // stencilStoreOp
			VK_IMAGE_LAYOUT_UNDEFINED, // initialLayout
			VK_IMAGE_LAYOUT_UNDEFINED // finalLayout
		},
		// G-buffer 1
		{
			0, // flags
			VK_FORMAT_R32G32B32A32_UINT, // format
			VK_SAMPLE_COUNT_1_BIT, // samples
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, // loadOp
			VK_ATTACHMENT_STORE_OP_DONT_CARE, // storeOp
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, // stencilLoadOp
			VK_ATTACHMENT_STORE_OP_DONT_CARE, // stencilStoreOp
			VK_IMAGE_LAYOUT_UNDEFINED, // initialLayout
			VK_IMAGE_LAYOUT_UNDEFINED // finalLayout
		},
		// Translucency buffer
		{
			0, // flags
			VK_FORMAT_R8G8B8A8_UNORM, // format
			VK_SAMPLE_COUNT_1_BIT, // samples
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, // loadOp
			VK_ATTACHMENT_STORE_OP_DONT_CARE, // storeOp
			VK_ATTACHMENT_LOAD_OP_DONT_CARE, // stencilLoadOp
			VK_ATTACHMENT_STORE_OP_DONT_CARE, // stencilStoreOp
			VK_IMAGE_LAYOUT_UNDEFINED, // initialLayout
			VK_IMAGE_LAYOUT_UNDEFINED // finalLayout
		}
	};
	// Depth prepass depth buffer reference (read/write)
	static const VkAttachmentReference depthAttachmentReference =
	{
		kAttachment_DEPTH, // attachment
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL // layout
	};
	// G-buffer attachment references (render)
	static const VkAttachmentReference gBufferOutputs[] =
	{
		{
			kAttachment_GBUFFER, // attachment
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL // layout
		}
	};
	// Lighting input attachment references
	static const VkAttachmentReference gBufferReadRef[] =
	{
		// Read from g-buffer.
		{
			kAttachment_GBUFFER, // attachment
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL // layout
		},
		// Read depth as texture.
		{
			kAttachment_DEPTH, // attachment
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL // layout
		}
	};
	// Lighting pass - write to opaque buffer.
	static const VkAttachmentReference opaqueWrite[] =
	{
		// Write to opaque buffer.
		{
			kAttachment_OPAQUE, // attachment
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL // layout
		}
	};
	// Translucency rendering pass - translucency buffer write
	static const VkAttachmentReference translucentWrite[] =
	{
		// Write to translucency buffer.
		{
			kAttachment_TRANSLUCENCY, // attachment
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL // layout
		}
	};
	static const VkAttachmentReference compositeInputs[] =
	{
		// Read from translucency buffer.
		{
			kAttachment_TRANSLUCENCY, // attachment
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL // layout
		},
		// Read from opaque buffer.
		{
			kAttachment_OPAQUE, // attachment
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL // layout
		}
	};
	// Final pass - back buffer render reference
	static const VkAttachmentReference backBufferRenderRef[] =
	{
		{
			kAttachment_BACK, // attachment
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL // layout
		}
	};
	static const VkSubpassDescription subpasses[] = {
		// Subpass 1 - depth prepass
		{
			0, // flags
			VK_PIPELINE_BIND_POINT_GRAPHICS, // pipelineBindPoint
			0, // inputAttachmentCount
			nullptr, // pInputAttachments
			0, // colorAttachmentCount
			nullptr, // pColorAttachments
			nullptr, // pResolveAttachments
			&depthAttachmentReference, // pDepthStencilAttachment
			0, // preserveAttachmentCount
			nullptr // pPreserveAttachments
		},
		// Subpass 2 - g-buffer generation
		{
			0, // flags
			VK_PIPELINE_BIND_POINT_GRAPHICS, // pipelineBindPoint
			0, // inputAttachmentCount
			nullptr, // pInputAttachments
			_countof( gBufferOutputs ), // colorAttachmentCount
			gBufferOutputs, // pColorAttachments
			nullptr, // pResolveAttachments
			&depthAttachmentReference, // pDepthStencilAttachment
			0, // preserveAttachmentCount
			nullptr // pPreserveAttachments
		},
		// Subpass 3 - lighting
		{
			0, // flags
			VK_PIPELINE_BIND_POINT_GRAPHICS, // pipelineBindPoint
			_countof( gBufferReadRef ), // inputAttachmentCount
			gBufferReadRef, // pInputAttachments
			_countof( opaqueWrite ), // colorAttachmentCount
			opaqueWrite, // pColorAttachments
			nullptr, // pResolveAttachments
			nullptr, // pDepthStencilAttachment
			0, // preserveAttachmentCount
			nullptr // pPreserveAttachments
		},
		// Subpass 4 - translucent objects
		{
			0, // flags
			VK_PIPELINE_BIND_POINT_GRAPHICS, // pipelineBindPoint
			0, // inputAttachmentCount
			nullptr, // pInputAttachments
			_countof( translucentWrite ), // colorAttachmentCount,
			translucentWrite, // pColorAttachments
			nullptr, // pResolveAttachments
			nullptr, // pDepthStencilAttachment
			0, // preserveAttachmentCount
			nullptr // pPreserveAttachments
		},
		// Subpass 5 - composite
		{
			0, // flags
			VK_PIPELINE_BIND_POINT_GRAPHICS, // pipelineBindPoint
			0, // inputAttachmentCount
			nullptr, // pInputAttachments
			_countof( backBufferRenderRef ), // colorAttachmentCount,
			backBufferRenderRef, // pColorAttachments
			nullptr, // pResolveAttachments
			nullptr, // pDepthStencilAttachment
			0, // preserveAttachmentCount
			nullptr // pPreserveAttachments
		}
	};
	static const VkSubpassDependency dependencies[] =
	{
		// G-buffer pass depends on depth prepass.
		{
			kSubpass_DEPTH, // srcSubpass
			kSubpass_GBUFFER, // dstSubpass
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStageMask
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // dstStageMask
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // srcAccessMask
			VK_ACCESS_SHADER_READ_BIT, // dstAccessMask
			VK_DEPENDENCY_BY_REGION_BIT // dependencyFlags
		},
		// Lighting pass depends on g-buffer.
		{
			kSubpass_GBUFFER, // srcSubpass
			kSubpass_LIGHTING, // dstSubpass
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStageMask
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // dstStageMask
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // srcAccessMask
			VK_ACCESS_SHADER_READ_BIT, // dstAccessMask
			VK_DEPENDENCY_BY_REGION_BIT // dependencyFlags
		},
		// Composite pass depends on translucent pass.
		{
			kSubpass_TRANSLUCENTS, // srcSubpass
			kSubpass_COMPOSITE, // dstSubpass
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStageMask
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // dstStageMask
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // srcAccessMask
			VK_ACCESS_SHADER_READ_BIT, // dstAccessMask
			VK_DEPENDENCY_BY_REGION_BIT // dependencyFlags
		},
		// Composite pass also depends on lighting.
		{
			kSubpass_LIGHTING, // srcSubpass
			kSubpass_COMPOSITE, // dstSubpass
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStageMask
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // dstStageMask
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // srcAccessMask
			VK_ACCESS_SHADER_READ_BIT, // dstAccessMask
			VK_DEPENDENCY_BY_REGION_BIT // dependencyFlags
		}
	};

	static const VkRenderPassCreateInfo renderPassCreateInfo =
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, nullptr,
		0, // flags
		_countof( attachments ), attachments,
		_countof( subpasses ), subpasses,
		_countof( dependencies ), dependencies
	};

	VkRenderPass renderPass;
	auto res = vkCreateRenderPass( device, &renderPassCreateInfo, nullptr, &renderPass );
	assert( res == VK_SUCCESS );
}








/*
============================================================
 VK_StartFrame
============================================================
*/
void GL_StartFrame()
{
	auto const dc = VK_GetCurrDC();

	if( dc->IsPresentable() )
	{
		ID_VK_CHECK( vkAcquireNextImageKHR( dc->GetHandle(), m_swapchain, UINT64_MAX, m_acquireSemaphores[ vkcontext.currentFrameData ], VK_NULL_HANDLE, &m_currentSwapIndex ) );
	}
}

/*
============================================================
 VK_EndFrame
============================================================
*/
void GL_EndFrame()
{
	auto const dc = VK_GetCurrDC();


	// Host access to queue must be externally synchronized.
	const VkQueue executionQueue = dc->m_graphicsQueue->GetHandle();

	// Host access to fence must be externally synchronized.
	const VkFence fence_allDone = VK_NULL_HANDLE;

	vkGetFenceStatus( dc->GetHandle(), fence_allDone );

	idStaticList<VkSemaphore, 16> waitSemaphores;
	idStaticList<VkPipelineStageFlags, 16> dstStageMasks;

	idStaticList<VkCommandBuffer, 16> commandBuffers;
	idStaticList<VkSemaphore, 16> commandBuffersToBeSignaledSemaphores;



	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	// the number of semaphores upon which to wait before
	// executing the command buffers for the batch.
	submitInfo.waitSemaphoreCount = waitSemaphores.Num();
	submitInfo.pWaitSemaphores = waitSemaphores.Ptr();	// Host access to pSubmits[].pWaitSemaphores[] must be externally synchronized.

	// Pointer to an array of pipeline stages at which each
	// corresponding semaphore wait will occur.
	submitInfo.pWaitDstStageMask = dstStageMasks.Ptr;

	// The number of command buffers to execute in the batch.
	submitInfo.commandBufferCount = commandBuffers.Num();
	submitInfo.pCommandBuffers = commandBuffers.Ptr();

	// The number of semaphores to be signaled once the commands specified
	// in pCommandBuffers have completed execution.
	submitInfo.signalSemaphoreCount = commandBuffersToBeSignaledSemaphores.Num();
	submitInfo.pSignalSemaphores = commandBuffersToBeSignaledSemaphores.Ptr();		// Host access to pSubmits[].pSignalSemaphores[] must be externally synchronized.

	ID_VK_CHECK( vkQueueSubmit( executionQueue,
								1, &submitInfo,
								fence_allDone ) ); // Fence to wait for all command buffers to finish before
												   // presenting to the swap chain
}

/*
============================================================
 VK_BlockingSwapBuffers
============================================================
*/
void GL_BlockingSwapBuffers()
{
	( "***************** BlockingSwapBuffers *****************\n\n\n" );

	auto dc = VK_GetCurrDC();

	//if( vkcontext.commandBufferRecorded[ vkcontext.currentFrameData ] == false )
	//{
	//	return;
	//}

	dc->Present();
}
