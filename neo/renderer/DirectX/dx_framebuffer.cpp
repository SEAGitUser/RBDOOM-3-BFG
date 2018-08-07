#pragma hdrstop
#include "precompiled.h"

#include "framework/Common_local.h"
#include "../tr_local.h"

#include "dx_header.h"

/*
=====================================================================================================
						Contains the Framebuffer implementation for DirecX
=====================================================================================================
*/

idRenderDestinationManager renderDestManager;

/*
=============================================
 idRenderDestination()
=============================================
*/
idRenderDestination::idRenderDestination( const char *_name )
	: name( _name ), apiObject( new dxFramebuffer_t() )
{
	Clear();
}
/*
=============================================
 ~idRenderDestination()
=============================================
*/
idRenderDestination::~idRenderDestination()
{
	assert( tr.IsRenderDeviceRunning() );
	if( IsValid() )
	{
		auto fbo = ( dxFramebuffer_t *)GetAPIObject();

		for( uint32 i = 0; i < GetTargetCount(); i++ )
		{
			SAFE_RELEASE( fbo->colorViews[ i ] );
		}
		SAFE_RELEASE( fbo->dsView );

		delete apiObject;
		apiObject = nullptr;
	}
	name.Clear();
	Clear();
}

/*
=============================================
 idRenderDestination::Clear
=============================================
*/
void idRenderDestination::Clear()
{
	targetWidth = targetHeight = 0;
	targetList.Clear();
	depthStencilImage = nullptr;
	boundSide = 0;
	boundLevel = 0;
	isDefault = false;
	isExplicit = false;
}

/*
=============================================
 idRenderDestination::CreateFromImages
	could be called on allready created one to change targets
=============================================
*/
void idRenderDestination::CreateFromImages( const targetList_t *_targetList, idImage * depthStencil, const int depthStencilLod )
{
	assert( tr.IsRenderDeviceRunning() );
	assert( !name.IsEmpty() );

	if( IsValid() ) 
	{
		auto fbo = ( dxFramebuffer_t * )GetAPIObject();

		for( uint32 i = 0; i < GetTargetCount(); i++ ) 
		{
			SAFE_RELEASE( fbo->colorViews[ i ] );
		}
		SAFE_RELEASE( fbo->dsView );
		SAFE_RELEASE( fbo->dsReadOnlyView );
	}

	Clear();

	/*GLuint fbo = GL_NONE;
	isExplicit = true;

	if( apiObject == nullptr )
	{
		glGenFramebuffers( 1, &fbo );
	#pragma warning( suppress: 4312 )
		apiObject = reinterpret_cast< void* >( fbo );
		assert( fbo != GL_NONE );
	}
	else
	{
		fbo = GetGLObject( apiObject );
	}

	glBindFramebuffer( GL_FRAMEBUFFER, fbo );

	if( GLEW_ARB_framebuffer_no_attachments )
	{
		glFramebufferParameteri( GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, SCREEN_WIDTH );
		glFramebufferParameteri( GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, SCREEN_HEIGHT );
		glFramebufferParameteri( GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_LAYERS, 1 );
		glFramebufferParameteri( GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_SAMPLES, 1 );
		glFramebufferParameteri( GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS, GL_FALSE );
	}

	if( GLEW_KHR_debug )
	{
		idStrStatic<128> name;
		name.Format( "idRenderDestination(\"%s\")(%u)", GetName(), fbo );
		glObjectLabel( GL_FRAMEBUFFER, fbo, name.Length(), name.c_str() );
	}
*/

	if( _targetList != nullptr )
	{
		targetList = *_targetList;
		assert( targetList.Count() < MAX_TARGETS );
	}

	auto device = GetCurrentDevice();

	auto fbo = ( dxFramebuffer_t * )GetAPIObject();

	UINT buffersCount = 0;
	memset( fbo->colorViews, 0, sizeof( fbo->colorViews[0] ) * _countof( fbo->colorViews ) );

	for( int i = 0; i < targetList.Count(); i++ )
	{
		auto & targ = targetList.list[ i ];

		if( targ.image == nullptr ) {
			idLib::Error( "idRenderDestination( %s ): empty target %i", GetName(), i );
		}

		auto tex = ( dxTextureObject_t * )targ.image->GetAPIObject();

		DX_RENDER_TARGET_VIEW_DESC desc = {};

		if( targ.image->GetOpts().textureType == TT_3D )
		{
			desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
			desc.Texture3D.MipSlice = targ.mipLevel;
			//desc.Texture3D.FirstWSlice = 0;
			desc.Texture3D.WSize = targ.image->GetOpts().depth;
		} 
		else {
			if( targ.image->IsMultisampled() ) // 2DMS / 2DMSArray
			{
				if( targ.image->IsArray() )
				{
					desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
					//desc.Texture2DMSArray.FirstArraySlice = 0;
					desc.Texture2DMSArray.ArraySize = targ.image->GetOpts().depth;
				}
				else {
					desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
					//desc.Texture2DMS.UnusedField_NothingToDefine = 0;
				}
			}
			else {
				if( targ.image->IsArray() ) // 2DArray / CubeArray
				{
					desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
					desc.Texture2DArray.MipSlice = targ.mipLevel;
					//desc.Texture2DArray.FirstArraySlice = 0;
					desc.Texture2DArray.ArraySize = targ.image->GetOpts().depth;

					if( targ.image->GetOpts().textureType == TT_CUBIC ) {
						desc.Texture2DArray.ArraySize *= 6;
						// cube sides as layers
					}
				}
				else { // Cube / 2D
					if( targ.image->GetOpts().textureType == TT_CUBIC )
					{
						desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
						desc.Texture2DArray.MipSlice = targ.mipLevel;
						//desc.Texture2DArray.FirstArraySlice = 0;
						desc.Texture2DArray.ArraySize = 6;
						// cube sides as layers
					} 
					else {
						desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
						desc.Texture2D.MipSlice = targ.mipLevel;
					}
				}
			}
		}

		auto res = device->CreateRenderTargetView( tex->res, &desc, &fbo->colorViews[ buffersCount++ ] );
		assert( res == S_OK );

		//idStrStatic<128> name;
		//name.Format( "idImage(%s)", GetName() );
		//D3DSetDebugObjectName( pTexture, GetName() );

		targetWidth = idMath::Max( targetWidth, targ.image->GetUploadWidth() ) / ( 1 << targ.mipLevel );
		targetHeight = idMath::Max( targetHeight, targ.image->GetUploadHeight() ) / ( 1 << targ.mipLevel );
	}

	if( depthStencil != nullptr )
	{
		// DXGI_FORMAT_D16_UNORM
		// DXGI_FORMAT_D24_UNORM_S8_UINT
		// DXGI_FORMAT_D32_FLOAT
		// DXGI_FORMAT_D32_FLOAT_S8X24_UINT

		assert( depthStencil->GetOpts().renderTarget == true );
		auto tex = ( dxTextureObject_t * )depthStencil->GetAPIObject();

		DX_DEPTH_STENCIL_VIEW_DESC desc = {};

		// A value that describes whether the texture is read only. Pass 0 to specify that it is not read only; 
		// otherwise, pass one of: D3D11_DSV_READ_ONLY_DEPTH , D3D11_DSV_READ_ONLY_STENCIL    
		//desc.Flags = D3D11_DSV_READ_ONLY_DEPTH;

		if( depthStencil->IsMultisampled() ) // 2DMS / 2DMSArray
		{
			if( depthStencil->IsArray() )
			{
				desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
				//desc.Texture2DMSArray.FirstArraySlice = 0;
				desc.Texture2DMSArray.ArraySize = depthStencil->GetOpts().depth;
			}
			else {
				desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
				//desc.Texture2DMS.UnusedField_NothingToDefine = 0;
			}			
		}
		else {
			if( depthStencil->IsArray() ) // 2DArray / Cube / CubeArray
			{
				desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
				desc.Texture2DArray.MipSlice = depthStencilLod;
				//desc.Texture2DArray.FirstArraySlice = 0;
				desc.Texture2DArray.ArraySize = depthStencil->GetOpts().depth;

				if( depthStencil->GetOpts().textureType == TT_CUBIC ) {
					desc.Texture2DArray.ArraySize *= 6;
					// cube sides as layers
				}
			}
			else {
				if( depthStencil->GetOpts().textureType == TT_CUBIC )
				{
					desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
					desc.Texture2DArray.MipSlice = depthStencilLod;
					//desc.Texture2DArray.FirstArraySlice = 0;
					desc.Texture2DArray.ArraySize = 6;
					// cube sides as layers
				}
				else {
					desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
					desc.Texture2D.MipSlice = depthStencilLod;
				}
			}
		}

		auto res = device->CreateDepthStencilView( tex->res, &desc, &fbo->dsView );
		assert( res == S_OK );

		desc.Flags = D3D11_DSV_READ_ONLY_DEPTH;
		auto res = device->CreateDepthStencilView( tex->res, &desc, &fbo->dsReadOnlyView );
		assert( res == S_OK );

		depthStencilImage = depthStencil;
	}

	if( !buffersCount )
	{
		if( depthStencil != nullptr )
		{
			targetWidth = depthStencil->GetUploadWidth();
			targetHeight = depthStencil->GetUploadHeight();
		}
		else release_assert( "no colors, no depth, no stencil - wtf?" );
	}
}

/*
=================================================
 idRenderDestination::Print()
=================================================
*/
void idRenderDestination::Print() const
{
	for( int i = 0; i < targetList.Count(); i++ )
	{
		auto & trg = targetList.list[ i ];

		if( trg.image != nullptr )
		{
			common->Printf( S_COLOR_GRAY " - target%i mip(%i) leyer(%i) %s" S_COLOR_DEFAULT "\n", i, trg.mipLevel, trg.layer, trg.image->GetName() );
		}
	}
	if( depthStencilImage != nullptr )
	{
		common->Printf( S_COLOR_GRAY " - depthStencilImage: %s" S_COLOR_DEFAULT "\n", depthStencilImage->GetName() );
	}
}
