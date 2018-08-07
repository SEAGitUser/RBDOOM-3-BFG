
#pragma hdrstop
#include "precompiled.h"

#include "../tr_local.h"

#include "dx_header.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

D3D11_PRIMITIVE_TOPOLOGY GetPrimType()
{
	return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

struct renderStateDX_t
{
	//uint64		frameNumber;
	//uint64		currentOcclusionQueryBatch;
	//uint64		currentOcclusionQuery;
	//uint32		totalQueriesInBatch;
	//uint32		cachedOcclusionResult;
	//uint64		lastCachedOcclusionBatch;
	//uint64		timeQueryId;
	//uint64		timeQueryPending;
	//uint64		startGPUTimeMicroSec;
	//uint64		endGPUTimeMicroSec;
	//uintptr_t	renderSync;

	dxFramebuffer_t				backBuffer;

	float						polyOfsScale;
	float						polyOfsBias;

	const dxTextureObject_t *	tmu[ MAX_MULTITEXTURE_UNITS ];
	UINT						currenttmu;

	DXQuery *					frameQuery;
	DXQuery *					frameQueryDisjoint;
	DXQuery *					frameStartTimerQuery;
	DXQuery *					frameFinishTimerQuery;

	const idRenderDestination *	currentRenderDest;
	const idDeclRenderProg *	currentRenderProg;
	uint64						currentStateBits;
	DXBlendState *				currentBSObject;
	DXDepthStencilState *		currentDSSObject;
	DXRasterizerState *			currentRSObject;
	DXInputLayout *				currentVAO;
	DXBuffer *					currentIBO;
	DXBuffer *					currentVBO;
} renderStateDX;
renderStateDX_t & GetCurrentRenderState()
{
	return renderStateDX;
}

void Shutdown()
{
	auto iContext = GetCurrentImContext();
	auto dxState = GetCurrentRenderState();

	SAFE_RELEASE( dxState.frameQuery );
	SAFE_RELEASE( dxState.frameQueryDisjoint );
	SAFE_RELEASE( dxState.frameStartTimerQuery );
	SAFE_RELEASE( dxState.frameFinishTimerQuery );
}

/*
============================================================
 Inserts a timing mark for the start of the GPU frame.
============================================================
*/
void GL_StartFrame( int frame )
{
	auto iContext = GetCurrentImContext();
	auto dxState = GetCurrentRenderState();

	if( dxState.frameQueryDisjoint == NULL )
	{
		auto device = GetCurrentDevice();

		D3D11_QUERY_DESC desc = {};

		desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
		device->CreateQuery( &desc, &dxState.frameQueryDisjoint );

		desc.Query = D3D11_QUERY_TIMESTAMP;
		device->CreateQuery( &desc, &dxState.frameStartTimerQuery );
		device->CreateQuery( &desc, &dxState.frameFinishTimerQuery );
	}

	// Begin disjoint query, and timestamp the beginning of the frame
	iContext->Begin( dxState.frameQueryDisjoint );
	iContext->End( dxState.frameStartTimerQuery );
}
/*
============================================================
 Inserts a timing mark for the end of the GPU frame.
============================================================
*/
void GL_EndFrame()
{
	auto iContext = GetCurrentImContext();
	auto dxState = GetCurrentRenderState();

	iContext->End( dxState.frameFinishTimerQuery );
	iContext->End( dxState.frameQueryDisjoint );

	GL_Flush();
}
/*
========================================
 Read back the start and end timer queries from the previous frame
========================================
*/
void GL_GetLastFrameTime( uint64& GPUTimeMicroSec )
{
	GPUTimeMicroSec = 0;

	auto iContext = GetCurrentImContext();
	auto dxState = GetCurrentRenderState();

	if( !dxState.frameQueryDisjoint )
		return;

	// Wait for data to be available
	while( iContext->GetData( dxState.frameQueryDisjoint, NULL, 0, 0 ) == S_FALSE ) {
		sys->Sleep( 1 );   // Wait a bit, but give other threads a chance to run
	}

	// Check whether timestamps were disjoint during the last frame
	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;
	iContext->GetData( dxState.frameQueryDisjoint, &tsDisjoint, sizeof( tsDisjoint ), 0 );
	if( tsDisjoint.Disjoint )
	{
		assert("Disjoint");
		return;
	}

	// Get all the timestamps
	UINT64 tsStartFrame = 0, tsFinishFrame = 0;
	iContext->GetData( dxState.frameStartTimerQuery, &tsStartFrame, sizeof( UINT64 ), 0 );
	iContext->GetData( dxState.frameFinishTimerQuery, &tsFinishFrame, sizeof( UINT64 ), 0 );

	// Convert to real time ( milliseconds )
	float msFrame = float( tsFinishFrame - tsStartFrame ) / float( tsDisjoint.Frequency ) * 1000.0f;

	GPUTimeMicroSec = msFrame * 1000ull;
}

/*
============================================================
 GL_SetRenderProgram
============================================================
*/
void GL_BeginRenderView()
{
	auto iContext = GetCurrentImContext();

	const auto bo = static_cast<DXBuffer *>( backEnd.viewUniformBuffer.GetAPIObject() );

	iContext->VSSetConstantBuffers( BINDING_GLOBAL_UBO, 1, &bo );
	iContext->HSSetConstantBuffers( BINDING_GLOBAL_UBO, 1, &bo );
	iContext->DSSetConstantBuffers( BINDING_GLOBAL_UBO, 1, &bo );
	iContext->GSSetConstantBuffers( BINDING_GLOBAL_UBO, 1, &bo );
	iContext->PSSetConstantBuffers( BINDING_GLOBAL_UBO, 1, &bo );

	//iContext->CSSetConstantBuffers( BINDING_GLOBAL_UBO, 1, &bo );
}
/*
============================================================
 GL_EndRenderView
============================================================
*/
void GL_EndRenderView()
{

}


/*
============================================================
 GL_SelectTexture
============================================================
*/
void GL_SelectTexture( int unit )
{
	auto dxState = GetCurrentRenderState();

	if( dxState.currenttmu == unit )
		return;

	assert( unit >= 0 );
	assert( unit < MAX_PROG_TEXTURE_PARMS );

	RENDERLOG_PRINT( "DX_SelectTexture( %i );\n", unit );

	dxState.currenttmu = unit;
}
/*
============================================================
 GL_GetCurrentTextureUnit
============================================================
*/
int GL_GetCurrentTextureUnit()
{
	return GetCurrentRenderState().currenttmu;
}
/*
============================================================
GL_ResetTextureState
============================================================
*/
void GL_ResetTextureState()
{
	RENDERLOG_OPEN_BLOCK( "DX_ResetTextureState()" );

	auto dxState = GetCurrentRenderState();

	/*for( int i = 0; i < MAX_MULTITEXTURE_UNITS - 1; i++ )
	{
	GL_SelectTexture( i );
	renderImageManager->BindNull();
	}*/
	int oldTMU = dxState.currenttmu;
	for( int i = 0; i < MAX_PROG_TEXTURE_PARMS; ++i )
	{
		dxState.currenttmu = i;
	}
	dxState.currenttmu = oldTMU;

	GL_SelectTexture( 0 );
	RENDERLOG_CLOSE_BLOCK();
}
static const GLenum t1 = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
static const GLenum t2 = GL_TEXTURE_2D_MULTISAMPLE;
static const GLenum t3 = GL_TEXTURE_2D_ARRAY;
static const GLenum t4 = GL_TEXTURE_2D;
static const GLenum t5 = GL_TEXTURE_CUBE_MAP_ARRAY;
static const GLenum t6 = GL_TEXTURE_CUBE_MAP;
static const GLenum t7 = GL_TEXTURE_3D;
/*
============================================================
GL_BindTexture
  Automatically enables 2D mapping or cube mapping if needed
  Pixel shader only!
============================================================
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

	auto iContext = GetCurrentImContext();
	auto & dxState = GetCurrentRenderState();

	const UINT texUnit = dxState.currenttmu;
	const auto tex = GetDXTexture( img );

	if( dxState.tmu[ texUnit ] != tex )
	{
		dxState.tmu[ texUnit ] = tex;

		DXShaderResourceView* view[ 1 ] = { ( DXShaderResourceView* )tex->view };

		iContext->PSSetSamplers( texUnit, 1, &tex->smp );
		iContext->PSSetShaderResources( texUnit, 1, view );
	}

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
}


/*
============================================================
 GL_SetViewport
============================================================
*/
void GL_Viewport( int left, int top, int width, int height )
{
	auto iContext = GetCurrentImContext();

	LONG g_BufferState_backBufferDesc_Width = renderSystem->GetWidth();
	LONG g_BufferState_backBufferDesc_Height = renderSystem->GetHeight();

	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = __max( 0, left );
	viewport.TopLeftY = __max( 0, ( int ) g_BufferState_backBufferDesc_Height - top - height );
	viewport.Width = __min( ( int ) g_BufferState_backBufferDesc_Width - viewport.TopLeftX, width );
	viewport.Height = __min( ( int ) g_BufferState_backBufferDesc_Height - viewport.TopLeftY, height );
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;

	iContext->RSSetViewports( 1, &viewport );
}
/*
============================================================
 GL_SetScissor
============================================================
*/
void GL_Scissor( int left, int top, int width, int height )
{
	auto iContext = GetCurrentImContext();

	///LONG g_BufferState_backBufferDesc_Width = renderSystem->GetWidth();
	LONG g_BufferState_backBufferDesc_Height = renderSystem->GetHeight();

	const RECT rect = {
		left,
		( LONG ) g_BufferState_backBufferDesc_Height - top - height,
		left + width,
		( LONG ) g_BufferState_backBufferDesc_Height - top
	};

	iContext->RSSetScissorRects( 1, &rect );
}


/*
============================================================
 GL_PolygonOffset
============================================================
*/
void GL_PolygonOffset( float scale, float bias )
{
	///auto iContext = GetCurrentImContext();
	auto dxState = GetCurrentRenderState();

	dxState.polyOfsScale = scale;
	dxState.polyOfsBias = bias;

	if( dxState.currentStateBits & GLS_POLYGON_OFFSET )
	{
		///glPolygonOffset( scale, bias );

		RENDERLOG_PRINT( "DX_PolygonOffset( scale %f, bias %f )\n", scale, bias );
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
============================================================
 GL_Clear
	the full extent of the resource view is always cleared. Viewport and scissor settings are not applied.
============================================================
*/
void GL_Clear( bool color, bool depth, bool stencil, byte stencilValue, float r, float g, float b, float a )
{
	auto iContext = GetCurrentImContext();

	auto renderDest = GetCurrentRenderDestination();
	assert( renderDest != nullptr );

	auto fbo = GetDXFramebuffer( renderDest );

	if( color )
	{
		const FLOAT colorClearValue[ 4 ] = { r, g, b, a };
		for( int i = 0; i < renderDest->GetTargetCount(); i++ )
		{
			assert( renderDest->GetTargetImage( i ) != nullptr );
			assert( fbo->colorViews[ i ] != nullptr );

			iContext->ClearRenderTargetView( fbo->colorViews[ i ], colorClearValue );
		}

		RENDERLOG_PRINT( "DX_Clear( color %f,%f,%f,%f )\n", r, g, b, a );
	}

	if( depth || stencil )
	{
		UINT clearFlags = 0U;

		if( depth )
		{
			clearFlags |= D3D11_CLEAR_DEPTH;
			RENDERLOG_PRINT( "DX_Clear( depth 1.0 )\n" );
		}
		if( stencil )
		{
			assert( renderDest->GetDepthStencilImage()->IsDepthStencil() != false );
			clearFlags |= D3D11_CLEAR_STENCIL;
			RENDERLOG_PRINT( "DX_Clear( stencil %i )\n", stencilValue );
		}

		assert( renderDest->GetDepthStencilImage() != nullptr );
		iContext->ClearDepthStencilView( fbo->dsView, clearFlags, 1.0f, stencilValue );
	}
}
/*
========================================
 GL_ClearDepth
========================================
*/
void GL_ClearDepth( const float value )
{
	auto iContext = GetCurrentImContext();

	auto renderDest = GetCurrentRenderDestination();
	assert( renderDest != nullptr );

	assert( renderDest->GetDepthStencilImage() != nullptr );
	auto fbo = GetDXFramebuffer( renderDest );

	iContext->ClearDepthStencilView( fbo->dsView, D3D11_CLEAR_DEPTH, value, 0 );

	RENDERLOG_PRINT( "DX_ClearDepth( %f )\n", value );
}
/*
========================================
 GL_ClearDepthStencil
========================================
*/
void GL_ClearDepthStencil( const float depthValue, const int stencilValue )
{
	auto iContext = GetCurrentImContext();

	auto renderDest = GetCurrentRenderDestination();
	assert( renderDest != nullptr );

	assert( renderDest->GetDepthStencilImage() != nullptr );
	auto fbo = GetDXFramebuffer( renderDest );

	iContext->ClearDepthStencilView( fbo->dsView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depthValue, stencilValue );

	RENDERLOG_PRINT( "DX_ClearDepthStencil( depth:%f, stencil:%i )\n", depthValue, stencilValue );
}
/*
========================================
 GL_ClearColor
========================================
*/
void GL_ClearColor( const float r, const float g, const float b, const float a, const int iColorBuffer )
{
	auto iContext = GetCurrentImContext();

	auto renderDest = GetCurrentRenderDestination();
	assert( renderDest != nullptr );

	assert( renderDest->GetTargetImage( iColorBuffer ) != nullptr );
	auto fbo = GetDXFramebuffer( renderDest );

	FLOAT colorClearValue[ 4 ] = { r, g, b, a };
	iContext->ClearRenderTargetView( fbo->colorViews[ iColorBuffer ], colorClearValue );

	RENDERLOG_PRINT( "DX_ClearColor( target:%i, %f,%f,%f,%f )\n", iColorBuffer, r, g, b, a );
}



/*
============================================================
 GetCurrentRenderDestination
============================================================
*/
const idRenderDestination * GetCurrentRenderDestination()
{
	return renderStateDX.currentRenderDest;
}
/*
============================================================
 GL_SetRenderDestination
============================================================
*/
void GL_SetRenderDestination( idRenderDestination * dest )
{
	assert( dest != nullptr );

	auto & dxState = GetCurrentRenderState();
	if( dxState.currentRenderDest != dest )
	{
		dxState.currentRenderDest = dest;

		auto fbo = static_cast< const dxFramebuffer_t *>( dest->GetAPIObject() );

		auto iContext = GetCurrentImContext();
		iContext->OMSetRenderTargets( ( UINT )dest->GetTargetCount(), fbo->colorViews, fbo->dsView );

		RENDERLOG_PRINT( "DX_SetRenderDestination( %s )\n", dest->GetName() );
	}
}
/*
============================================================
 GL_ResetRenderDestination
============================================================
*/
void GL_ResetRenderDestination()
{
	auto iContext = GetCurrentImContext();
	auto & dxState = GetCurrentRenderState();

	iContext->OMSetRenderTargets( 0, NULL, NULL );

	dxState.currentRenderDest = nullptr;

	RENDERLOG_PRINT( "DX_ResetRenderDestination()\n" );
}
/*
============================================================
 GL_SetNativeRenderDestination
============================================================
*/
void GL_SetNativeRenderDestination()
{
	auto & dxState = GetCurrentRenderState();
	if( dxState.currentRenderDest != nullptr )
	{
		dxState.currentRenderDest = nullptr;

		auto iContext = GetCurrentImContext();
		iContext->OMSetRenderTargets( 1, dxState.backBuffer.colorViews, dxState.backBuffer.dsView );

		RENDERLOG_PRINT( "DX_SetNativeRenderDestination()\n" );
	}
}
/*
============================================================
 GL_IsNativeFramebufferActive
============================================================
*/
bool GL_IsNativeFramebufferActive()
{
	return( GetCurrentRenderDestination() == nullptr );
}
/*
============================================================
 GL_IsBound
============================================================
*/
bool GL_IsBound( const idRenderDestination * dest )
{
	return( GetCurrentRenderDestination() == dest );
}


/*
========================
GL_BlitRenderBuffer
========================
*/
void GL_BlitRenderBuffer(
	const idRenderDestination * src, int src_x, int src_y, int src_w, int src_h,
	const idRenderDestination * dst, int dst_x, int dst_y, int dst_w, int dst_h,
	bool color, bool linearFiltering )
{
	auto iContext = GetCurrentImContext();

	const auto srcTex = color ? GetDXTexture( src->GetColorImage() ) : GetDXTexture( src->GetDepthStencilImage() );
	const auto dstTex = color ? GetDXTexture( dst->GetColorImage() ) : GetDXTexture( dst->GetDepthStencilImage() );

	//iContext->CopyResource( dstTex->res, srcTex->res );

	//D3D11_BOX box = {};
	iContext->CopySubresourceRegion1( dstTex->res, 0, 0, 0, 0, srcTex->res, 0, NULL, 0 ); // D3D11_COPY_NO_OVERWRITE / D3D11_COPY_DISCARD

	RENDERLOG_OPEN_BLOCK( "DX_Blit" );
	RENDERLOG_PRINT( "src: %s, rect:%i,%i,%i,%i\n", src->GetName(), src_x, src_y, src_w, src_h );
	RENDERLOG_PRINT( "dst: %s, rect:%i,%i,%i,%i\n", dst->GetName(), dst_x, dst_y, dst_w, dst_h );
	RENDERLOG_CLOSE_BLOCK();
}
/*
====================
idImage::CopyFramebuffer
====================
*/
void GL_CopyCurrentColorToTexture( idImage * img, int x, int y, int imageWidth, int imageHeight )
{
	assert( img != nullptr );
	assert( img->GetOpts().textureType == TT_2D );
	assert( !img->GetOpts().IsMultisampled() );
	assert( !img->GetOpts().IsArray() );

	auto iContext = GetCurrentImContext();

	D3D11_BOX box;
	box.back = 1;
	box.bottom = y + imageHeight;
	box.front = 0;
	box.left = x;
	box.right = x + imageWidth;
	box.top = y;

	iContext->CopySubresourceRegion1( GetDXTexture( img )->res, 0, 0, 0, 0,
		GetDXTexture( GL_GetCurrentRenderDestination()->GetColorImage() )->res, 0, &box, D3D11_COPY_DISCARD );


	// NO operations on native backbuffer!

	const idImage const * srcImg = GL_GetCurrentRenderDestination()->GetColorImage();
	const bool bFormatMatch = ( srcImg->GetOpts().format == img->GetOpts().format );

	if( GLEW_NV_copy_image && bFormatMatch )
	{
		img->Resize( imageWidth, imageHeight, 1 );

		const GLuint srcName = GetGLObject( srcImg->GetAPIObject() );
		const GLuint dstName = GetGLObject( img->GetAPIObject() );

		glCopyImageSubDataNV(
			srcName, GL_TEXTURE_2D, 0, x, y, 0,
			dstName, GL_TEXTURE_2D, 0, 0, 0, 0,
			imageWidth, imageHeight, 1 );

		GL_CheckErrors();
	}
	else if( GLEW_ARB_copy_image && bFormatMatch )
	{
		img->Resize( imageWidth, imageHeight, 1 );

		const GLuint srcName = GetGLObject( srcImg->GetAPIObject() );
		const GLuint dstName = GetGLObject( img->GetAPIObject() );

		glCopyImageSubData(
			srcName, GL_TEXTURE_2D, 0, x, y, 0,
			dstName, GL_TEXTURE_2D, 0, 0, 0, 0,
			imageWidth, imageHeight, 1 );

		GL_CheckErrors();
	}
	else
	{
		///const GLuint srcFBO = GetGLObject( GL_GetCurrentRenderDestination()->GetAPIObject() );
		///const GLuint dstFBO = GetGLObject( renderDestManager.renderDestCopyCurrentRender->GetAPIObject() );
		///
		///glBindFramebuffer( GL_READ_FRAMEBUFFER, srcFBO );
		///
		///glBindFramebuffer( GL_DRAW_FRAMEBUFFER, dstFBO );
		///glFramebufferTexture( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GetGLObject( img->GetAPIObject() ), 0 );
		///
		///glBlitFramebuffer(
		///	x, y, imageWidth, imageHeight,
		///	0, 0, imageWidth, imageHeight,
		///	GL_COLOR_BUFFER_BIT, GL_NEAREST );

		///glTextureObject_t tex;
		///tex.DeriveTargetInfo( this );
		///GLuint texnum = GetGLObject( img->GetAPIObject() );
		///glBindTexture( GL_TEXTURE_2D, texnum );
		GL_BindTexture( 0, img );

		if( GLEW_ARB_texture_storage )
		{
			assert( bFormatMatch == true );
			img->Resize( imageWidth, imageHeight, 1 );
			// FBO ?

			glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, x, y, imageWidth, imageHeight );
		}
		else
		{
			// gl will allocate memory for this new texture
			glCopyTexImage2D( GL_TEXTURE_2D, 0,
							  r_useHDR.GetBool() ? GL_RGBA16F : GL_RGBA8,
							  x, y, imageWidth, imageHeight, 0 );
		}
		GL_CheckErrors();
	}

	backEnd.pc.c_copyFrameBuffer++;

	RENDERLOG_PRINT( "DX_CopyCurrentColorToTexture( dst: %s %ix%i, %i,%i,%i,%i )\n",
					 img->GetName(), img->GetUploadWidth(), img->GetUploadHeight(), x, y, imageWidth, imageHeight );
}

/*
=============================================================================================

									RENDER STATE

=============================================================================================
*/

/*struct dxRasterStates_t {
	ID3D11RasterizerState* states[ POLY_MODE_COUNT ][ CULLMODE_COUNT ][POLYGON_OFFSET_COUNT][ RASTERIZERSTATE_COUNT ];
};
struct dxDepthStates_t {
	ID3D11DepthStencilState* states[ STENCILPACKAGE_COUNT ][ DEPTHSTATE_COUNT ];
};
struct dxBlendStates_t {
	ID3D11BlendState* states[ COLORMASK_COUNT ][ BLENDSTATE_SRC_COUNT ][ BLENDSTATE_DST_COUNT ];
};*/

static const uint64 BlendStateBits =
	( GLS_REDMASK | GLS_GREENMASK | GLS_BLUEMASK | GLS_ALPHAMASK ) |
	( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) |
	GLS_BLENDOP_BITS |
	GLS_ALPHA_COVERAGE;

static const uint64 DepthStencilBits =
	GLS_DEPTHFUNC_BITS |
	GLS_DISABLE_DEPTHTEST |
	GLS_DEPTHMASK |
	GLS_STENCILMASK |
	( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS | GLS_STENCIL_FUNC_REF_BITS | GLS_STENCIL_FUNC_MASK_BITS );

static const uint64 RasterizerStateBits = GLS_POLYMODE_LINE | GLS_POLYGON_OFFSET |
	GLS_TWOSIDED | GLS_BACKSIDED | GLS_FRONTSIDED;

class DXRenderStateManager
{
	struct dxState_t {
		ID3D11BlendState * pBS = nullptr;
		ID3D11DepthStencilState* pDSS = nullptr;
		ID3D11RasterizerState* pRS = nullptr;
	};

	typedef idPair<uint64, ID3D11BlendState *> dxBlendStatePair_t;
	typedef idPair<uint64, ID3D11DepthStencilState *> dxDSStatePair_t;
	//typedef idPair<uint64, ID3D11RasterizerState *>	dxRasterStatePair_t;
	struct dxRasterStatePair_t {
		dxRasterStatePair_t( ID3D11RasterizerState * _obj, uint64 _stateBits, float _polyOfsScale, float _polyOfsBias )
			: obj( _obj ), stateBits( _stateBits ), polyOfsScale( _polyOfsScale ), polyOfsBias( _polyOfsBias ) {
		}
		ID3D11RasterizerState * obj = nullptr;
		uint64 stateBits = 0ull;
		float polyOfsScale = 0.0f;
		float polyOfsBias = 0.0f;
	};

	idList<dxBlendStatePair_t> blendStateList;
	idList<dxDSStatePair_t> dsStateList;
	idList<dxRasterStatePair_t> rasterStateList;

	dxState_t dx_state_disable_rasterizer;

public:

	void Init();
	void Shutdown();

	ID_INLINE ID3D11BlendState * GetBlendState( uint64 stateBits  );
	ID_INLINE ID3D11DepthStencilState * GetDepthStencilState( uint64 stateBits );
	ID_INLINE ID3D11RasterizerState * GetRasterizerState( uint64 stateBits );

	void DisableRasterizer( DXDeviceContext * iContext )
	{
		iContext->OMSetBlendState( dx_state_disable_rasterizer.pBS, NULL, 0 );
		iContext->OMSetDepthStencilState( dx_state_disable_rasterizer.pDSS, 0 );
		iContext->RSSetState( dx_state_disable_rasterizer.pRS );

		auto & dxState = GetCurrentRenderState();

		dxState.currentBSObject = dx_state_disable_rasterizer.pBS;
		dxState.currentDSSObject = dx_state_disable_rasterizer.pDSS;
		dxState.currentRSObject = dx_state_disable_rasterizer.pRS;
	}

} dxRenderStateManager;

void DXRenderStateManager::Init()
{
	HRESULT res;
	auto device = GetCurrentDevice();

	// Disable rasterizer states
	{
		DX_BLEND_DESC blend_desc = {};
		DX_DEPTH_STENCIL_DESC depth_stencil_desc = {};
		DX_RASTERIZER_DESC rasterizer_desc = {};

		res = device->CreateBlendState( &blend_desc, &dx_state_disable_rasterizer.pBS ); assert( res == S_OK );
		res = device->CreateDepthStencilState( &depth_stencil_desc, &dx_state_disable_rasterizer.pDSS ); assert( res == S_OK );
		res = device->CreateRasterizerState( &rasterizer_desc, &dx_state_disable_rasterizer.pRS ); assert( res == S_OK );
	}


}

void DXRenderStateManager::Shutdown()
{
	for( int i = 0; i < blendStateList.Num(); i++ )
	{
		blendStateList[ i ].first = 0ull;
		assert( blendStateList[ i ].second != nullptr );
		blendStateList[ i ].second->Release();
		blendStateList[ i ].second = nullptr;
	}
	blendStateList.Clear();

	for( int i = 0; i < dsStateList.Num(); i++ )
	{
		dsStateList[ i ].first = 0ull;
		assert( dsStateList[ i ].second != nullptr );
		dsStateList[ i ].second->Release();
		dsStateList[ i ].second = nullptr;
	}
	dsStateList.Clear();

	for( int i = 0; i < rasterStateList.Num(); i++ )
	{
		assert( rasterStateList[ i ].obj != nullptr );
		rasterStateList[ i ].obj->Release();
		rasterStateList[ i ].obj = nullptr;

		rasterStateList[ i ].stateBits = 0ull;
		rasterStateList[ i ].polyOfsBias = 0.0f;
		rasterStateList[ i ].polyOfsScale = 0.0f;
	}
	rasterStateList.Clear();
}

/*
===============================
	BLEND STATE
===============================
*/
ID_INLINE ID3D11BlendState * DXRenderStateManager::GetBlendState( uint64 stateBits )
{
	for( uint32 i = 0; i < blendStateList.Num(); i++ )
	{
		if( blendStateList[ i ].first == stateBits )
		{
			return blendStateList[ i ].second;
		}
	}

	// else create a new one and store

	auto device = GetCurrentDevice();

	DX_BLEND_DESC desc = {};

	desc.AlphaToCoverageEnable = FALSE;
	desc.IndependentBlendEnable = FALSE;

	switch( stateBits & GLS_SRCBLEND_BITS )
	{
		default:
		case GLS_SRCBLEND_ONE:					desc.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_ONE;				break;
		case GLS_SRCBLEND_ZERO:					desc.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_ZERO;				break;
		case GLS_SRCBLEND_DST_COLOR:			desc.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_DEST_COLOR;		break;
		case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:	desc.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_INV_DEST_COLOR;	break;
		case GLS_SRCBLEND_SRC_ALPHA:			desc.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_SRC_ALPHA;		break;
		case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:	desc.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_INV_SRC_ALPHA;	break;
		case GLS_SRCBLEND_DST_ALPHA:			desc.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_DEST_ALPHA;		break;
		case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:	desc.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_INV_DEST_ALPHA;	break;
	}
	switch( stateBits & GLS_DSTBLEND_BITS )
	{
		default:
		case GLS_DSTBLEND_ZERO:					desc.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_ZERO;			break;
		case GLS_DSTBLEND_ONE:					desc.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_ONE;				break;
		case GLS_DSTBLEND_SRC_COLOR:			desc.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_SRC_COLOR;		break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:	desc.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_INV_SRC_COLOR;	break;
		case GLS_DSTBLEND_SRC_ALPHA:			desc.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_SRC_ALPHA;		break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:	desc.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;	break;
		case GLS_DSTBLEND_DST_ALPHA:			desc.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_DEST_ALPHA;		break;
		case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:	desc.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_INV_DEST_ALPHA;	break;
	}
	desc.RenderTarget[ 0 ].BlendEnable = ( desc.RenderTarget[ 0 ].SrcBlend == D3D11_BLEND_ONE && desc.RenderTarget[ 0 ].DestBlend == D3D11_BLEND_ZERO )? FALSE : TRUE;

	//SEA: swf has some issue with main menu :(
	/*switch( stateBits & GLS_BLENDOP_BITS ) {
		default:
		case GLS_BLENDOP_ADD:		desc.RenderTarget[ 0 ].BlendOp = D3D11_BLEND_OP_ADD;			break;
		case GLS_BLENDOP_SUB:		desc.RenderTarget[ 0 ].BlendOp = D3D11_BLEND_OP_SUBTRACT;		break;
		case GLS_BLENDOP_MIN:		desc.RenderTarget[ 0 ].BlendOp = D3D11_BLEND_OP_MIN;			break;
		case GLS_BLENDOP_MAX:		desc.RenderTarget[ 0 ].BlendOp = D3D11_BLEND_OP_MAX;			break;
		case GLS_BLENDOP_INVSUB:	desc.RenderTarget[ 0 ].BlendOp = D3D11_BLEND_OP_REV_SUBTRACT;	break;
	}*/
	desc.RenderTarget[ 0 ].BlendOp = D3D11_BLEND_OP_ADD;

	desc.RenderTarget[ 0 ].SrcBlendAlpha = desc.RenderTarget[ 0 ].SrcBlend;
	desc.RenderTarget[ 0 ].DestBlendAlpha = desc.RenderTarget[ 0 ].DestBlend;
	desc.RenderTarget[ 0 ].BlendOpAlpha = desc.RenderTarget[ 0 ].BlendOp;

	desc.RenderTarget[ 0 ].RenderTargetWriteMask = 0;
	desc.RenderTarget[ 0 ].RenderTargetWriteMask |= ( stateBits & GLS_REDMASK )? 0 : D3D11_COLOR_WRITE_ENABLE_RED;
	desc.RenderTarget[ 0 ].RenderTargetWriteMask |= ( stateBits & GLS_GREENMASK )? 0 : D3D11_COLOR_WRITE_ENABLE_GREEN;
	desc.RenderTarget[ 0 ].RenderTargetWriteMask |= ( stateBits & GLS_BLUEMASK )? 0 : D3D11_COLOR_WRITE_ENABLE_BLUE;
	desc.RenderTarget[ 0 ].RenderTargetWriteMask |= ( stateBits & GLS_ALPHAMASK )? 0 : D3D11_COLOR_WRITE_ENABLE_ALPHA;

	for( UINT32 i = 1; i < _countof( desc.RenderTarget ); i++ ) {
		desc.RenderTarget[ i ] = desc.RenderTarget[ 0 ];
	}

	ID3D11BlendState * pBS = nullptr;
	auto res = device->CreateBlendState( &desc, &pBS );
	assert( res == S_OK );

	blendStateList.Append( dxBlendStatePair_t( stateBits, pBS ) );
	return pBS;
}

/*
===============================
	DEPTH STENCIL STATE
===============================
*/
ID_INLINE ID3D11DepthStencilState * DXRenderStateManager::GetDepthStencilState( uint64 stateBits )
{
	for( uint32 i = 0; i < dsStateList.Num(); i++ )
	{
		if( dsStateList[ i ].first == stateBits )
		{
			return dsStateList[ i ].second;
		}
	}

	// else create a new one and store

	auto device = GetCurrentDevice();

	DX_DEPTH_STENCIL_DESC desc = {};

	desc.DepthEnable = ( stateBits & GLS_DISABLE_DEPTHTEST )? FALSE : TRUE;
	desc.DepthWriteMask = ( stateBits & GLS_DEPTHMASK )? D3D11_DEPTH_WRITE_MASK_ZERO : D3D11_DEPTH_WRITE_MASK_ALL;

	switch( stateBits & GLS_DEPTHFUNC_BITS )
	{
		default:
		case GLS_DEPTHFUNC_LEQUAL:   desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;	  break;
		case GLS_DEPTHFUNC_EQUAL:    desc.DepthFunc = D3D11_COMPARISON_EQUAL;		  break;
		case GLS_DEPTHFUNC_ALWAYS:   desc.DepthFunc = D3D11_COMPARISON_ALWAYS;		  break;
		case GLS_DEPTHFUNC_GEQUAL:   desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL; break;
		case GLS_DEPTHFUNC_NOTEQUAL: desc.DepthFunc = D3D11_COMPARISON_NOT_EQUAL;	  break;
		case GLS_DEPTHFUNC_GREATER:  desc.DepthFunc = D3D11_COMPARISON_GREATER;		  break;
		case GLS_DEPTHFUNC_LESS:	 desc.DepthFunc = D3D11_COMPARISON_LESS;		  break;
		case GLS_DEPTHFUNC_NEVER:	 desc.DepthFunc = D3D11_COMPARISON_NEVER;		  break;
	}

	desc.StencilEnable = BOOL( ( stateBits & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS ) ) != 0 );
	desc.StencilReadMask = GLS_STENCIL_GET_MASK( stateBits );
	desc.StencilWriteMask = ( stateBits & GLS_STENCILMASK )? 0x00u : 0xFFu;
	if( desc.StencilEnable )
	{
		switch( stateBits & GLS_STENCIL_OP_FAIL_BITS )
		{
			default:
			case GLS_STENCIL_OP_FAIL_KEEP:		desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;	  break;
			case GLS_STENCIL_OP_FAIL_ZERO:		desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_ZERO;	  break;
			case GLS_STENCIL_OP_FAIL_REPLACE:	desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;  break;
			case GLS_STENCIL_OP_FAIL_INCR:		desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_INCR;	  break;
			case GLS_STENCIL_OP_FAIL_DECR:		desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_DECR;	  break;
			case GLS_STENCIL_OP_FAIL_INVERT:	desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_INVERT;	  break;
			case GLS_STENCIL_OP_FAIL_INCR_WRAP: desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_INCR_SAT; break;
			case GLS_STENCIL_OP_FAIL_DECR_WRAP: desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_DECR_SAT; break;
		}
		switch( stateBits & GLS_STENCIL_OP_ZFAIL_BITS )
		{
			default:
			case GLS_STENCIL_OP_ZFAIL_KEEP:		 desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;		break;
			case GLS_STENCIL_OP_ZFAIL_ZERO:		 desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_ZERO;		break;
			case GLS_STENCIL_OP_ZFAIL_REPLACE:	 desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;	break;
			case GLS_STENCIL_OP_ZFAIL_INCR:		 desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;		break;
			case GLS_STENCIL_OP_ZFAIL_DECR:		 desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;		break;
			case GLS_STENCIL_OP_ZFAIL_INVERT:	 desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INVERT;   break;
			case GLS_STENCIL_OP_ZFAIL_INCR_WRAP: desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR_SAT; break;
			case GLS_STENCIL_OP_ZFAIL_DECR_WRAP: desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR_SAT; break;
		}
		switch( stateBits & GLS_STENCIL_OP_PASS_BITS )
		{
			default:
			case GLS_STENCIL_OP_PASS_KEEP:		desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;	  break;
			case GLS_STENCIL_OP_PASS_ZERO:		desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_ZERO;	  break;
			case GLS_STENCIL_OP_PASS_REPLACE:	desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;  break;
			case GLS_STENCIL_OP_PASS_INCR:		desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;	  break;
			case GLS_STENCIL_OP_PASS_DECR:		desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_DECR;	  break;
			case GLS_STENCIL_OP_PASS_INVERT:	desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INVERT;	  break;
			case GLS_STENCIL_OP_PASS_INCR_WRAP: desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR_SAT; break;
			case GLS_STENCIL_OP_PASS_DECR_WRAP: desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_DECR_SAT; break;
		}

		switch( stateBits & GLS_STENCIL_FUNC_BITS )
		{
			default:
			case GLS_STENCIL_FUNC_ALWAYS:	desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;		 break;
			case GLS_STENCIL_FUNC_NEVER:	desc.FrontFace.StencilFunc = D3D11_COMPARISON_NEVER;		 break;
			case GLS_STENCIL_FUNC_LESS:		desc.FrontFace.StencilFunc = D3D11_COMPARISON_LESS;			 break;
			case GLS_STENCIL_FUNC_EQUAL:	desc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;		 break;
			case GLS_STENCIL_FUNC_LEQUAL:	desc.FrontFace.StencilFunc = D3D11_COMPARISON_LESS_EQUAL;	 break;
			case GLS_STENCIL_FUNC_GREATER:	desc.FrontFace.StencilFunc = D3D11_COMPARISON_GREATER;		 break;
			case GLS_STENCIL_FUNC_NOTEQUAL: desc.FrontFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;	 break;
			case GLS_STENCIL_FUNC_GEQUAL:	desc.FrontFace.StencilFunc = D3D11_COMPARISON_GREATER_EQUAL; break;
		}
	}
	desc.BackFace = desc.FrontFace;

	ID3D11DepthStencilState* pDSS = nullptr;
	auto res = device->CreateDepthStencilState( &desc, &pDSS );
	assert( res == S_OK );

	dsStateList.Append( dxDSStatePair_t( stateBits, pDSS ) );
	return pDSS;
}

/*
===============================
	RASTERIZER STATE
===============================
*/
ID_INLINE ID3D11RasterizerState* DXRenderStateManager::GetRasterizerState( uint64 stateBits )
{
	auto & dxState = GetCurrentRenderState();

	for( uint32 i = 0; i < rasterStateList.Num(); i++ )
	{
		if( rasterStateList[ i ].stateBits == stateBits &&
			rasterStateList[ i ].polyOfsBias == dxState.polyOfsBias &&
			rasterStateList[ i ].polyOfsScale == dxState.polyOfsScale )
		{
			return rasterStateList[ i ].obj;
		}
	}

	// else create a new one and store

	auto device = GetCurrentDevice();

	DX_RASTERIZER_DESC desc = {};

	desc.FillMode = ( stateBits & GLS_POLYMODE_LINE )? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
	desc.CullMode = ( stateBits & GLS_TWOSIDED )? D3D11_CULL_NONE : (( stateBits & GLS_BACKSIDED )? D3D11_CULL_FRONT : D3D11_CULL_BACK );
	desc.FrontCounterClockwise = FALSE;

	desc.DepthBias = dxState.polyOfsBias;
	desc.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
	desc.SlopeScaledDepthBias = dxState.polyOfsScale;

	desc.DepthClipEnable = TRUE;
	desc.ScissorEnable = TRUE;
	desc.MultisampleEnable = FALSE;
	desc.AntialiasedLineEnable = FALSE;

	ID3D11RasterizerState* pRS = nullptr;
	auto res = device->CreateRasterizerState( &desc, &pRS );
	assert( res == S_OK );

	rasterStateList.Append( dxRasterStatePair_t( pRS, stateBits, dxState.polyOfsScale, dxState.polyOfsBias ) );
	return pRS;
}

/*
========================================
 GL_State
========================================
*/
void GL_State( uint64 stateBits, bool forceChange )
{
	auto & dxState = GetCurrentRenderState();

	uint64 diff = stateBits ^ dxState.currentStateBits;

	if( forceChange )
	{
		// make sure everything is set all the time, so we
		// can see if our delta checking is screwing up
		diff = 0xFFFFFFFFFFFFFFFF;
	}
	else if( diff == 0 )
	{
		return;
	}

	auto iContext = GetCurrentImContext();

	if( diff & BlendStateBits )
	{
		auto pBlendState = dxRenderStateManager.GetBlendState( stateBits );
		iContext->OMSetBlendState( pBlendState, NULL, D3D11_DEFAULT_SAMPLE_MASK );
		dxState.currentBSObject = pBlendState;
	}

	if( diff & DepthStencilBits )
	{
		auto pDepthStencilState = dxRenderStateManager.GetDepthStencilState( stateBits );
		iContext->OMSetDepthStencilState( pDepthStencilState, GLS_STENCIL_GET_REF( stateBits ) );
		dxState.currentDSSObject = pDepthStencilState;
	}

	if( diff & RasterizerStateBits )
	{
		auto pRasterState = dxRenderStateManager.GetRasterizerState( stateBits );
		iContext->RSSetState( pRasterState );
		dxState.currentRSObject = pRasterState;
	}

	dxState.currentStateBits = stateBits;
}

/*
========================
 GL_SetDefaultState
========================
*/
void GL_SetDefaultState()
{
	auto iContext = GetCurrentImContext();

	GetCurrentRenderState().currentRenderDest = nullptr;
	DXRenderTargetView * const mrt[ D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT ] = { NULL };
	iContext->OMSetRenderTargets( _countof( mrt ), mrt, NULL );

	GetCurrentRenderState().currentRenderProg = nullptr;
	iContext->VSSetShader( NULL, NULL, 0 );
	iContext->HSSetShader( NULL, NULL, 0 );
	iContext->DSSetShader( NULL, NULL, 0 );
	iContext->GSSetShader( NULL, NULL, 0 );
	iContext->PSSetShader( NULL, NULL, 0 );

	DXSamplerState * const smp[ D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT ] = { NULL };
	iContext->VSSetSamplers( 0, _countof( smp ), smp );
	iContext->HSSetSamplers( 0, _countof( smp ), smp );
	iContext->DSSetSamplers( 0, _countof( smp ), smp );
	iContext->GSSetSamplers( 0, _countof( smp ), smp );
	iContext->PSSetSamplers( 0, _countof( smp ), smp );

	DXShaderResourceView * const srv[ D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT ] = { NULL };
	iContext->VSSetShaderResources( 0, _countof( srv ), srv );
	iContext->HSSetShaderResources( 0, _countof( srv ), srv );
	iContext->DSSetShaderResources( 0, _countof( srv ), srv );
	iContext->GSSetShaderResources( 0, _countof( srv ), srv );
	iContext->PSSetShaderResources( 0, _countof( srv ), srv );

	DXBuffer * const cb[ D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT ] = { NULL };
	iContext->VSSetConstantBuffers( 0, _countof( cb ), cb );
	iContext->HSSetConstantBuffers( 0, _countof( cb ), cb );
	iContext->DSSetConstantBuffers( 0, _countof( cb ), cb );
	iContext->GSSetConstantBuffers( 0, _countof( cb ), cb );
	iContext->PSSetConstantBuffers( 0, _countof( cb ), cb );

	GL_ResetTextureState();

	RENDERLOG_PRINT( "DX_SetDefaultState()\n" );
}

/*
========================
 GL_DisableRasterizer

  You may disable rasterization by telling the pipeline there is no pixel shader( set the pixel shader stage to
  NULL with DXDeviceContext::PSSetShader ), and disabling depth and stencil testing( set DepthEnable and
  StencilEnable to FALSE in DX_DEPTH_STENCIL_DESC ).
  While disabled, rasterization-related pipeline counters will not update.
========================
*/
void GL_DisableRasterizer()
{
	auto iContext = GetCurrentImContext();

	dxRenderStateManager.DisableRasterizer( iContext );

	GetCurrentRenderState().currentRenderProg = nullptr;
	iContext->VSSetShader( NULL, NULL, 0 );
	iContext->HSSetShader( NULL, NULL, 0 );
	iContext->DSSetShader( NULL, NULL, 0 );
	iContext->GSSetShader( NULL, NULL, 0 );
	iContext->PSSetShader( NULL, NULL, 0 );

	/*DXSamplerState * const samp[ D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT ] = { NULL };
	iContext->VSSetSamplers( 0, _countof( samp ), samp );
	iContext->HSSetSamplers( 0, _countof( samp ), samp );
	iContext->DSSetSamplers( 0, _countof( samp ), samp );
	iContext->GSSetSamplers( 0, _countof( samp ), samp );
	iContext->PSSetSamplers( 0, _countof( samp ), samp );

	DXShaderResourceView * const srv[ D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT ] = { NULL };
	iContext->VSSetShaderResources( 0, _countof( srv ), srv );
	iContext->HSSetShaderResources( 0, _countof( srv ), srv );
	iContext->DSSetShaderResources( 0, _countof( srv ), srv );
	iContext->GSSetShaderResources( 0, _countof( srv ), srv );
	iContext->PSSetShaderResources( 0, _countof( srv ), srv );

	DXBuffer * const cb[ D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT ] = { NULL };
	iContext->VSSetConstantBuffers( 0, _countof( cb ), cb );
	iContext->HSSetConstantBuffers( 0, _countof( cb ), cb );
	iContext->DSSetConstantBuffers( 0, _countof( cb ), cb );
	iContext->GSSetConstantBuffers( 0, _countof( cb ), cb );
	iContext->PSSetConstantBuffers( 0, _countof( cb ), cb );*/

	RENDERLOG_PRINT( "DX_DisableRasterizer()\n" );
}

/*
=====================================================================================================

											PROGRAM

=====================================================================================================
*/

/*
============================================================
GL_GetCurrentRenderProg
============================================================
*/
const idDeclRenderProg * GL_GetCurrentRenderProg()
{
	return renderStateDX.currentRenderProg;
}
/*
============================================================
GL_SetRenderProgram
============================================================
*/
void GL_SetRenderProgram( const idDeclRenderProg * prog )
{
	auto iContext = GetCurrentImContext();
	auto & dxState = GetCurrentRenderState();

	const auto dxProg = GetDXProgram( prog );
	if( dxState.currentRenderProg != prog )
	{
		dxState.currentRenderProg = prog;

		iContext->VSSetShader( ( DXVertexShader * ) dxProg->shaders[ SHT_VERTEX ], NULL, 0 );
		iContext->HSSetShader( ( DXHullShader * ) dxProg->shaders[ SHT_TESS_CTRL ], NULL, 0 );
		iContext->DSSetShader( ( DXDomainShader * ) dxProg->shaders[ SHT_TESS_EVAL ], NULL, 0 );
		iContext->GSSetShader( ( DXGeometryShader * ) dxProg->shaders[ SHT_GEOMETRY ], NULL, 0 );
		iContext->PSSetShader( ( DXPixelShader * ) dxProg->shaders[ SHT_FRAGMENT ], NULL, 0 );

		RENDERLOG_PRINT( "DX_SetRenderProgram( %s ) %s\n", prog->GetName(), prog->HasHardwareSkinning() ? ( prog->HasOptionalSkinning() ? "opt_skinned" : "skinned" ) : "" );

		// Bind buffers

		const auto & ubo = backEnd.progParmsUniformBuffer;
		const auto bo = static_cast<DXBuffer *>( ubo.GetAPIObject() );

		iContext->VSSetConstantBuffers( BINDING_PROG_PARMS_UBO, 1, dxProg->VSHasVecParms() ? &bo : NULL );
		iContext->PSSetConstantBuffers( BINDING_PROG_PARMS_UBO, 1, dxProg->PSHasVecParms() ? &bo : NULL );

		if( dxProg->HasHS() )
		{
			iContext->HSSetConstantBuffers( BINDING_PROG_PARMS_UBO, 1, dxProg->HSHasVecParms() ? &bo : NULL );
		}
		if( dxProg->HasDS() )
		{
			iContext->DSSetConstantBuffers( BINDING_PROG_PARMS_UBO, 1, dxProg->DSHasVecParms() ? &bo : NULL );
		}
		if( dxProg->HasGS() )
		{
			iContext->GSSetConstantBuffers( BINDING_PROG_PARMS_UBO, 1, dxProg->GSHasVecParms() ? &bo : NULL );
		}
	}
}
/*
============================================================
GL_ResetProgramState
============================================================
*/
void GL_ResetProgramState()
{
	auto iContext = GetCurrentImContext();

	auto & dxState = GetCurrentRenderState();
	dxState.currentRenderProg = nullptr;

	iContext->VSSetShader( NULL, NULL, 0 );
	iContext->HSSetShader( NULL, NULL, 0 );
	iContext->DSSetShader( NULL, NULL, 0 );
	iContext->GSSetShader( NULL, NULL, 0 );
	iContext->PSSetShader( NULL, NULL, 0 );

	DXSamplerState * const smp[ MAX_MULTITEXTURE_UNITS ] = { NULL };
	iContext->VSSetSamplers( 0, _countof( smp ), smp );
	iContext->HSSetSamplers( 0, _countof( smp ), smp );
	iContext->DSSetSamplers( 0, _countof( smp ), smp );
	iContext->GSSetSamplers( 0, _countof( smp ), smp );
	iContext->PSSetSamplers( 0, _countof( smp ), smp );

	DXShaderResourceView * const srv[ MAX_MULTITEXTURE_UNITS ] = { NULL };
	iContext->VSSetShaderResources( 0, _countof( srv ), srv );
	iContext->HSSetShaderResources( 0, _countof( srv ), srv );
	iContext->DSSetShaderResources( 0, _countof( srv ), srv );
	iContext->GSSetShaderResources( 0, _countof( srv ), srv );
	iContext->PSSetShaderResources( 0, _countof( srv ), srv );

	DXBuffer * const cbs[ BINDING_UBO_MAX ] = { NULL };
	iContext->VSSetConstantBuffers( 0, BINDING_UBO_MAX - 1, cbs );
	iContext->HSSetConstantBuffers( 0, BINDING_UBO_MAX - 1, cbs );
	iContext->DSSetConstantBuffers( 0, BINDING_UBO_MAX - 1, cbs );
	iContext->GSSetConstantBuffers( 0, BINDING_UBO_MAX - 1, cbs );
	iContext->PSSetConstantBuffers( 0, BINDING_UBO_MAX - 1, cbs );

	RENDERLOG_PRINT( "DX_ResetProgramState()\n" );
}
/*
============================================================
DX_CommitUniforms
============================================================
*/
static void DX_CommitUniforms( DXDeviceContext * iContext, renderStateDX_t & dxState, const idDeclRenderProg * prog )
{
	const auto dxProg = GetDXProgram( prog );

	// Commit used textures and samplers:
	{
		auto CommitTextures = []( shaderType_e sht, const idDeclRenderProg *prog, const dxProgramObject_t *dxProg,
								  idStaticList<DXSamplerState *, 16> &samps, idStaticList<DXShaderResourceView *, 16> &texs ) {
			samps.Clear();
			texs.Clear();

			for( int j = 0; j < dxProg->texParmsUsed[ sht ].Num(); ++j )
			{
				auto dxTex = GetDXTexture( prog->GetTexParms()[ dxProg->texParmsUsed[ sht ][ j ] ]->GetImage() );
				samps.Append( dxTex->smp );
				texs.Append( ( DXShaderResourceView* ) dxTex->view );
			}
		};

		const UINT StartSlot = 0;
		idStaticList<DXSamplerState *, 16> samps;
		idStaticList<DXShaderResourceView *, 16> texs;

		if( dxProg->texParmsUsed[ SHT_VERTEX ].Num() > 0 )
		{
			CommitTextures( SHT_VERTEX, prog, dxProg, samps, texs );

			iContext->VSSetSamplers( StartSlot, ( UINT ) samps.Num(), samps.Ptr() );
			iContext->VSSetShaderResources( StartSlot, ( UINT ) texs.Num(), texs.Ptr() );
		}
		if( dxProg->texParmsUsed[ SHT_TESS_CTRL ].Num() > 0 )
		{
			CommitTextures( SHT_TESS_CTRL, prog, dxProg, samps, texs );

			iContext->HSSetSamplers( StartSlot, ( UINT ) samps.Num(), samps.Ptr() );
			iContext->HSSetShaderResources( StartSlot, ( UINT ) texs.Num(), texs.Ptr() );
		}
		if( dxProg->texParmsUsed[ SHT_TESS_EVAL ].Num() > 0 )
		{
			CommitTextures( SHT_TESS_EVAL, prog, dxProg, samps, texs );

			iContext->DSSetSamplers( StartSlot, ( UINT ) samps.Num(), samps.Ptr() );
			iContext->DSSetShaderResources( StartSlot, ( UINT ) texs.Num(), texs.Ptr() );
		}
		if( dxProg->texParmsUsed[ SHT_GEOMETRY ].Num() > 0 )
		{
			CommitTextures( SHT_GEOMETRY, prog, dxProg, samps, texs );

			iContext->GSSetSamplers( StartSlot, ( UINT ) samps.Num(), samps.Ptr() );
			iContext->GSSetShaderResources( StartSlot, ( UINT ) texs.Num(), texs.Ptr() );
		}
		if( dxProg->texParmsUsed[ SHT_FRAGMENT ].Num() > 0 )
		{
			CommitTextures( SHT_FRAGMENT, prog, dxProg, samps, texs );

			iContext->PSSetSamplers( StartSlot, ( UINT ) samps.Num(), samps.Ptr() );
			iContext->PSSetShaderResources( StartSlot, ( UINT ) texs.Num(), texs.Ptr() );
		}
	}

	// Commit used vectors:
	{
		///DXBuffer * cBuffers[ D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT ] = { nullptr };
		///cBuffers[ BINDING_GLOBAL_UBO ] = static_cast<DXBuffer *>( backEnd.viewUniformBuffer.GetAPIObject() ); // global ubo

		if( prog->GetVecParms().Num() > 0 ) // Update local vector parms if any.
		{
			const auto & ubo = backEnd.progParmsUniformBuffer;
			const auto bo = static_cast<DXBuffer *>( ubo.GetAPIObject() );

			auto & parms = prog->GetVecParms();
			const int updateSize = parms.Num() * sizeof( idRenderVector );
			//ubo.Update( updateSize, localVectors->ToFloatPtr() );

			{
				D3D11_MAPPED_SUBRESOURCE map = {};
				auto res = iContext->Map( bo, 0, D3D11_MAP_WRITE_DISCARD, 0, &map );
				assert( res == S_OK );

				idRenderVector * dst = ( idRenderVector* ) map.pData;

			#if defined( USE_INTRINSICS )
				for( int i = 0; i < parms.Num(); ++i )
				{
					assert_16_byte_aligned( parms[ i ]->GetVecPtr() );
					_mm_stream_ps( ( float* ) dst++, _mm_load_ps( parms[ i ]->GetVecPtr() ) );
				}
				_mm_sfence();
			#else
				for( int i = 0; i < parms.Num(); ++i )
				{
					*( dst++ ) = parms[ i ]->GetVec4();
				}
			#endif

				iContext->Unmap( bo, 0 );
			}

			RENDERLOG_PRINT( "UBO( binding:%i, count:%i, size:%i, offset:%i, bsize:%i )\n", BINDING_PROG_PARMS_UBO, parms.Num(), updateSize, ubo.GetOffset(), ubo.GetSize() );
		}
	}
}


/*
=====================================================================================================

										DRAWING

=====================================================================================================
*/

/*
============================================================
GL_DrawIndexed
============================================================
*/
void GL_DrawIndexed( const drawSurf_t * surf, vertexLayoutType_t vertexLayout, const int globalInstanceCount )
{
	// Get vertex buffer --------------------------------

	idVertexBuffer vertexBuffer;
	assert( surf->vertexCache != 0 );
	vertexCache.GetVertexBuffer( surf->vertexCache, vertexBuffer );
	const UINT vertOffset = vertexBuffer.GetOffset();
	const auto VBO = static_cast<DXBuffer *>( vertexBuffer.GetAPIObject() );

	// Get index buffer ---------------------------------

	idIndexBuffer indexBuffer;
	assert( surf->indexCache != 0 );
	vertexCache.GetIndexBuffer( surf->indexCache, indexBuffer );
	const UINT indexOffset = indexBuffer.GetOffset();
	const auto IBO = static_cast<DXBuffer *>( indexBuffer.GetAPIObject() );

	// --------------------------------------------------

	auto dxContext = GetCurrentImContext();
	auto & dxState = GetCurrentRenderState();

	// --------------------------------------------------

	if( GL_GetCurrentRenderProgram()->HasOptionalSkinning() )
	{
		assert( GL_GetCurrentRenderProgram()->HasHardwareSkinning() != false );

	#if USE_INTRINSICS
		_mm_store_ps( renderProgManager.GetRenderParm( RENDERPARM_ENABLE_SKINNING )->GetVecPtr(), surf->HasSkinning() ? idMath::SIMD_SP_one : _mm_setzero_ps() );
	#else
		renderProgManager.GetRenderParm( RENDERPARM_ENABLE_SKINNING )->GetVec4().Fill( surf->HasSkinning() ? 1.0 : 0.0 );
	#endif
	}
	if( surf->HasSkinning() )
	{
		assert( GL_GetCurrentRenderProgram()->HasHardwareSkinning() != false );

		idJointBuffer jointBuffer;
		if( !vertexCache.GetJointBuffer( surf->jointCache, jointBuffer ) )
		{
			idLib::Warning( "GL_DrawIndexed, jointBuffer == NULL" );
			return;
		}

		assert( ( jointBuffer.GetOffset() & (/*glConfig.uniformBufferOffsetAlignment*/256 - 1 ) ) == 0 );

		auto const bo = static_cast<DXBuffer *>( jointBuffer.GetAPIObject() );

		const UINT firstConstant = jointBuffer.GetOffset() / sizeof( idRenderVector );
		const UINT numConstants = ( jointBuffer.GetNumJoints() * sizeof( idJointMat ) ) / sizeof( idRenderVector );
		dxContext->VSSetConstantBuffers1( BINDING_MATRICES_UBO, 1, &bo, &firstConstant, &numConstants );
	}

	DX_CommitUniforms( dxContext, dxState, GL_GetCurrentRenderProgram() );

	// --------------------------------------------------

	if( dxState.currentIBO != IBO )
	{
		dxContext->IASetIndexBuffer( IBO, DXGI_FORMAT_R16_UINT, 0 ); // DXGI_FORMAT_R32_UINT
		dxState.currentIBO == IBO;
	}

	if( dxState.currentVBO != VBO )
	{
		const UINT VBStride = sizeof( idDrawVert );
		const UINT VBOffset = 0;

		dxContext->IASetVertexBuffers( 0, 1, &VBO, &VBStride, &VBOffset );
		dxState.currentVBO == VBO;

		dxContext->IASetPrimitiveTopology( GetPrimType() );
	}

	auto vao = GetVertexLayout( vertexLayout );
	if( dxState.currentVAO != vao )
	{
		dxContext->IASetInputLayout( vao );
		dxState.currentVAO = vao;
	}

	if( 0 )
	{
		UINT IndexCountPerInstance = 0;
		UINT InstanceCount = 0;
		UINT StartIndexLocation = 0;
		INT  BaseVertexLocation = 0;
		UINT StartInstanceLocation = 0;

		// glDrawElementsInstancedBaseVertexBaseInstance
		dxContext->DrawIndexedInstanced( IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation );
	}
	else {
		const UINT IndexCount = r_singleTriangle.GetBool() ? 3 : surf->numIndexes;
		const UINT StartIndexLocation = indexOffset / sizeof( triIndex_t );
		const INT  BaseVertexLocation = vertOffset / sizeof( idDrawVert );

		// glDrawElementsInstancedBaseVertex
		dxContext->DrawIndexed( IndexCount, StartIndexLocation, BaseVertexLocation );
	}

	RENDERLOG_PRINT( ">>>>> DX_DrawIndexed( VBO:%p offset:%i, IBO:%p offset:%i count:%i )\n", VBO, vertOffset, IBO, indexOffset, surf->numIndexes );
}
/*
============================================================
GL_DrawZeroOneCube
============================================================
*/
void GL_DrawZeroOneCube( vertexLayoutType_t vertexLayout, int instanceCount )
{
	const drawSurf_t* const surf = &backEnd.zeroOneCubeSurface;

	// Get vertex buffer --------------------------------

	idVertexBuffer vertexBuffer;
	assert( surf->vertexCache != 0 );
	vertexCache.GetVertexBuffer( surf->vertexCache, vertexBuffer );
	const UINT vertOffset = vertexBuffer.GetOffset();
	const auto VBO = static_cast<DXBuffer *>( vertexBuffer.GetAPIObject() );

	// Get index buffer ---------------------------------

	idIndexBuffer indexBuffer;
	assert( surf->indexCache != 0 );
	vertexCache.GetIndexBuffer( surf->indexCache, indexBuffer );
	const UINT indexOffset = indexBuffer.GetOffset();
	const auto IBO = static_cast<DXBuffer *>( indexBuffer.GetAPIObject() );

	// --------------------------------------------------

	auto iContext = GetCurrentImContext();
	auto & dxState = GetCurrentRenderState();

	DX_CommitUniforms( iContext, dxState, GL_GetCurrentRenderProgram() );

	// --------------------------------------------------

	if( dxState.currentIBO != IBO )
	{
		iContext->IASetIndexBuffer( IBO, DXGI_FORMAT_R16_UINT, 0 ); // DXGI_FORMAT_R32_UINT
		dxState.currentIBO == IBO;
	}

	if( dxState.currentVBO != VBO )
	{
		const UINT VBStride = sizeof( idDrawVert );
		const UINT VBOffset = 0;

		iContext->IASetVertexBuffers( 0, 1, &VBO, &VBStride, &VBOffset );
		dxState.currentVBO == VBO;

		iContext->IASetPrimitiveTopology( GetPrimType() );
	}

	auto vao = GetVertexLayout( vertexLayout );
	if( dxState.currentVAO != vao )
	{
		iContext->IASetInputLayout( vao );
		dxState.currentVAO = vao;
	}

	const UINT IndexCount = r_singleTriangle.GetBool() ? 3 : surf->numIndexes;
	const UINT StartIndexLocation = indexOffset / sizeof( triIndex_t );
	const INT  BaseVertexLocation = vertOffset / sizeof( idDrawVert );

	iContext->DrawIndexed( IndexCount, StartIndexLocation, BaseVertexLocation );

	backEnd.pc.c_drawElements++;
	backEnd.pc.c_drawIndexes += surf->numIndexes;

	RENDERLOG_PRINT( ">>>>> GL_DrawZeroOneCube( VBO %p:%u, IBO %p:%u, numIndexes:%i )\n", VBO, vertOffset, IBO, indexOffset, surf->numIndexes );
}
/*
============================================================
GL_DrawUnitSquare
============================================================
*/
void GL_DrawUnitSquare( vertexLayoutType_t vertexLayout, int instanceCount )
{
	const auto * const surf = &backEnd.unitSquareSurface;

	// Get vertex buffer --------------------------------

	idVertexBuffer vertexBuffer;
	assert( surf->vertexCache != 0 );
	vertexCache.GetVertexBuffer( surf->vertexCache, vertexBuffer );
	const UINT vertOffset = vertexBuffer.GetOffset();
	const auto VBO = static_cast<DXBuffer *>( vertexBuffer.GetAPIObject() );

	// Get index buffer ---------------------------------

	idIndexBuffer indexBuffer;
	assert( surf->indexCache != 0 );
	vertexCache.GetIndexBuffer( surf->indexCache, indexBuffer );
	const UINT indexOffset = indexBuffer.GetOffset();
	const auto IBO = static_cast<DXBuffer *>( indexBuffer.GetAPIObject() );

	// --------------------------------------------------

	auto iContext = GetCurrentImContext();
	auto & dxState = GetCurrentRenderState();

	DX_CommitUniforms( iContext, dxState, GL_GetCurrentRenderProgram() );

	// --------------------------------------------------

	if( dxState.currentIBO != IBO )
	{
		iContext->IASetIndexBuffer( IBO, DXGI_FORMAT_R16_UINT, 0 ); // DXGI_FORMAT_R32_UINT
		dxState.currentIBO == IBO;
	}

	if( dxState.currentVBO != VBO )
	{
		const UINT VBStride = sizeof( idDrawVert );
		const UINT VBOffset = 0;

		iContext->IASetVertexBuffers( 0, 1, &VBO, &VBStride, &VBOffset );
		dxState.currentVBO == VBO;

		iContext->IASetPrimitiveTopology( GetPrimType() );
	}

	auto vao = GetVertexLayout( vertexLayout );
	if( dxState.currentVAO != vao )
	{
		iContext->IASetInputLayout( vao );
		dxState.currentVAO = vao;
	}

	const UINT IndexCount = r_singleTriangle.GetBool() ? 3 : surf->numIndexes;
	const UINT StartIndexLocation = indexOffset / sizeof( triIndex_t );
	const INT  BaseVertexLocation = vertOffset / sizeof( idDrawVert );

	iContext->DrawIndexed( IndexCount, StartIndexLocation, BaseVertexLocation );

	backEnd.pc.c_drawElements++;
	backEnd.pc.c_drawIndexes += surf->numIndexes;

	RENDERLOG_PRINT( ">>>>> GL_DrawUnitSquare( VBO %p:%u, IBO %p:%u, numIndexes:%i )\n", VBO, vertOffset, IBO, indexOffset, surf->numIndexes );
}
/*
============================================================
GL_DrawUnitSquareAuto
============================================================
*/
void GL_DrawZeroOneCubeAuto()
{
	auto iContext = GetCurrentImContext();
	auto & dxState = GetCurrentRenderState();

	DX_CommitUniforms( iContext, dxState, GL_GetCurrentRenderProgram() );

	if( dxState.currentVAO != nullptr )
	{
		dxState.currentVAO = nullptr;
		dxState.currentIBO == nullptr;
		dxState.currentVBO == nullptr;

		iContext->IASetVertexBuffers( 0, 0, NULL, NULL, NULL );
		iContext->IASetIndexBuffer( NULL, ( DXGI_FORMAT ) 0, 0 );
		iContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );
	}

	iContext->Draw( 14, 0 );

	RENDERLOG_PRINT( ">>>>> DX_DrawZeroOneCubeAuto()\n" );
}
/*
============================================================
GL_DrawScreenTriangleAuto
============================================================
*/
void GL_DrawScreenTriangleAuto()
{
	auto iContext = GetCurrentImContext();
	auto & dxState = GetCurrentRenderState();

	DX_CommitUniforms( iContext, dxState, GL_GetCurrentRenderProgram() );

	if( dxState.currentVAO != nullptr )
	{
		dxState.currentVAO = nullptr;
		dxState.currentIBO == nullptr;
		dxState.currentVBO == nullptr;

		iContext->IASetVertexBuffers( 0, 0, NULL, NULL, NULL );
		iContext->IASetIndexBuffer( NULL, ( DXGI_FORMAT ) 0, 0 );
		iContext->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	}

	iContext->Draw( 3, 0 );

	RENDERLOG_PRINT( ">>>>> DX_DrawScreenTriangleAuto()\n" );
}











/*
========================================
 GL_Flush
	Flush the command buffer
========================================
*/
void GL_Flush()
{
	auto iContext = GetCurrentImContext();
	iContext->Flush();
	RENDERLOG_PRINT("DX_Flush()\n");
}

/*
========================================
 GL_Finish
	Make sure there's no remaining work on the GPU
========================================
*/
void GL_Finish()
{
	auto iContext = GetCurrentImContext();
	auto dxState = GetCurrentRenderState();

	iContext->End( dxState.frameQuery );

	BOOL finished = FALSE;
	HRESULT hr;
	do {
		YieldProcessor();
		hr = iContext->GetData( dxState.frameQuery, &finished, sizeof( finished ), 0 );
	} while( ( hr == S_OK || hr == S_FALSE ) && finished == FALSE );

	//assert( SUCCEEDED( hr ) );

	RENDERLOG_PRINT( "DX_Finish()\n" );

#if 0
	HRESULT result;

	auto device = GetCurrentDevice();
	auto dxState = GetCurrentRenderState();

	if( !dxState.frameQuery )
	{
		// Determines whether or not the GPU is finished processing commands.
		DX_QUERY_DESC queryDesc;
		queryDesc.Query = D3D11_QUERY_EVENT;
		queryDesc.MiscFlags = 0;
		result = device->CreateQuery( &queryDesc, &dxState.frameQuery );
		assert( SUCCEEDED( result ) );
		if( FAILED( result ) )
		{
			common->FatalError( "Failed to create event query, result: 0x%X.", result );
		}
	}

	iContext->End( dxState.frameQuery );
	iContext->Flush();

	do {
		result = iContext->GetData( dxState.frameQuery, NULL, 0, D3D11_ASYNC_GETDATA_DONOTFLUSH );
		if( FAILED( result ) )
		{
			common->FatalError( "Failed to get event query data, result: 0x%X.", result );
		}

		// Keep polling, but allow other threads to do something useful first
		///ScheduleYield();
		YieldProcessor();

		/*if( testDeviceLost() )
		{
			mDisplay->notifyDeviceLost();
			return gl::Error( GL_OUT_OF_MEMORY, "Device was lost while waiting for sync." );
		}*/
	} while( result == S_FALSE );
#endif
}








#if 0

/*
=====================================================================================================

							NV DX DEPTH BOUNDS TEST IMPLEMENTATION

=====================================================================================================
*/


typedef enum _NvAPI_Status {
	NVAPI_OK = 0,		//!< Success. Request is completed.
	NVAPI_ERROR = -1,   //!< Generic error
} NvAPI_Status;

#define NVAPI_INTERFACE extern __success( return == NVAPI_OK ) NvAPI_Status __cdecl

#ifndef __unix
// mac os 32-bit still needs this
#if ( (defined(macintosh) && defined(__LP64__) && (__NVAPI_RESERVED0__)) || \
	  (!defined(macintosh) && defined(__NVAPI_RESERVED0__)) )
typedef unsigned int       NvU32; /* 0 to 4294967295                         */
#else
typedef unsigned long      NvU32; /* 0 to 4294967295                         */
#endif
#else
typedef unsigned int       NvU32; /* 0 to 4294967295                         */
#endif

typedef unsigned long    temp_NvU32; /* 0 to 4294967295                         */
typedef signed   short   NvS16;
typedef unsigned short   NvU16;
typedef unsigned char    NvU8;
typedef signed   char    NvS8;
typedef float            NvF32;


NVAPI_INTERFACE NvAPI_D3D11_SetDepthBoundsTest( IUnknown* pDeviceOrContext, NvU32 bEnable, float fMinDepth, float fMaxDepth );

/*
========================
GL_DepthBoundsTest
========================
*/
void DX_NV_DepthBoundsTest( const float zmin, const float zmax )
{
	//if( !dxConfig.depthBoundsTestAvailable || zmin > zmax )
	//	return;

	auto iContext = GetCurrentImContext();

	if( zmin == 0.0f && zmax == 0.0f )
	{
		NvAPI_D3D11_SetDepthBoundsTest( iContext, FALSE, 0.0f, 0.0f );

		RENDERLOG_PRINT( "GL_DepthBoundsTest( Disable )\n" );
	}
	else {
		NvAPI_D3D11_SetDepthBoundsTest( iContext, TRUE, zmin, zmax );

		RENDERLOG_PRINT( "GL_DepthBoundsTest( zmin %f, zmax %f )\n", zmin, zmax );
	}
}

/*
=====================================================================================================

							AMD DX DEPTH BOUNDS TEST IMPLEMENTATION

=====================================================================================================
*/

#pragma comment( lib, "amd_ags_x64.lib" )

// AMD SDK also sits one directory up
#include "..\\..\\AMD_SDK\\inc\\AMD_SDK.h"
#include "amd_ags.h"

// AGS - AMD's helper library
AGSContext *		g_pAGSContext = nullptr;
unsigned int		g_ExtensionsSupported = 0;

// agsInit( &g_pAGSContext, nullptr, nullptr );
//
// agsDeInit( g_pAGSContext );
// g_pAGSContext = nullptr;

void AMD_AGS_Init()
{
	agsInit( &g_pAGSContext, nullptr, nullptr );


	// Initialize AGS's driver extensions - these are dependent on the D3D11 device
	g_ExtensionsSupported = 0;
	agsDriverExtensionsDX11_Init( g_pAGSContext, pd3dDevice, 7, &g_ExtensionsSupported );
	if( g_ExtensionsSupported & AGS_DX11_EXTENSION_DEPTH_BOUNDS_TEST )
	{
	}
}

/*
========================
GL_DepthBoundsTest
========================
*/
void DX_AMD_DepthBoundsTest( const float zmin, const float zmax )
{
	//if( !dxConfig.depthBoundsTestAvailable || zmin > zmax )
	//	return;

	auto iContext = GetCurrentImContext();

	if( zmin == 0.0f && zmax == 0.0f )
	{
		agsDriverExtensionsDX11_SetDepthBounds( g_pAGSContext, false, 0.0f, 1.0f );

		RENDERLOG_PRINT( "GL_DepthBoundsTest( Disable )\n" );
	}
	else {
		agsDriverExtensionsDX11_SetDepthBounds( g_pAGSContext, true, zmin, zmax );

		RENDERLOG_PRINT( "GL_DepthBoundsTest( zmin %f, zmax %f )\n", zmin, zmax );
	}
}

#endif

/*
========================
GL_DepthBoundsTest
========================
*/
void GL_DepthBoundsTest( const float zmin, const float zmax )
{
	//if( !dxConfig.depthBoundsTestAvailable || zmin > zmax )
	//	return;

	auto iContext = GetCurrentImContext();

}