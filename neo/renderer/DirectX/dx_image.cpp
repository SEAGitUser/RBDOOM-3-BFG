#pragma hdrstop
#include "precompiled.h"

#include "framework/Common_local.h"
#include "../tr_local.h"

#include "dx_header.h"

/*
=====================================================================================================
						Contains the Image implementation for DirecX
=====================================================================================================
*/

ID_INLINE DXGI_FORMAT DeriveFormat( const idImage * img )
{
	const bool sRGB = glConfig.sRGBFramebufferAvailable && ( r_useSRGB.GetInteger() == 1 || r_useSRGB.GetInteger() == 3 );

	switch( img->GetOpts().format )
	{
		case FMT_RGBA8:
			return sRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;

		case FMT_XRGB8:
			return sRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;

		case FMT_RGB565: // light textures typically
			return DXGI_FORMAT_B5G6R5_UNORM;

		case FMT_ALPHA:
			return DXGI_FORMAT_A8_UNORM; // GL_R8

		case FMT_L8A8:
			return DXGI_FORMAT_R8G8_UNORM;

		case FMT_LUM8:
			return DXGI_FORMAT_R8_UNORM;

		case FMT_INT8:
			return DXGI_FORMAT_R8_SINT; // GL_R8 - GL_UNSIGNED_BYTE

		case FMT_DXT1:
			return sRGB ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM;
		case FMT_DXT5:
			return ( sRGB && 
				img->GetOpts().colorFormat != CFM_YCOCG_DXT5 && 
				img->GetOpts().colorFormat != CFM_NORMAL_DXT5 )? DXGI_FORMAT_BC3_UNORM_SRGB : DXGI_FORMAT_BC3_UNORM;

		case FMT_DEPTH:
			return DXGI_FORMAT_D32_FLOAT;
		case FMT_DEPTH_STENCIL:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;

		case FMT_RGB10_A2:
			return DXGI_FORMAT_R10G10B10A2_UINT; // GL_UNSIGNED_INT_2_10_10_10_REV
		case FMT_RG11B10F:
			return DXGI_FORMAT_R11G11B10_FLOAT; // GL_R11F_G11F_B10F
		case FMT_RG16F:
			return DXGI_FORMAT_R16G16_FLOAT;
		case FMT_RGBA16F:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case FMT_RGBA32F:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case FMT_R32F:
			return DXGI_FORMAT_R32_FLOAT;

		case FMT_X16:
			return DXGI_FORMAT_R16_UNORM; // GL_UNSIGNED_SHORT
		case FMT_Y16_X16:
			return DXGI_FORMAT_R16G16_UNORM; // GL_UNSIGNED_SHORT	
	}

	common->Error( "Unhandled image format %d in %s\n", img->GetOpts().format, img->GetName() );
}

ID_INLINE void GetMultisampleCount( int img_numSamples, DXGI_FORMAT format, DXGI_SAMPLE_DESC & ms )
{
	auto device = GetCurrentDevice();

	int numSamples = idMath::ClampInt( 1, 32, img_numSamples );

	ms.Count = 1;

	/*/*/if( numSamples < 64 && numSamples > 16 )
	{
		ms.Count = 32;
	}
	else if( numSamples < 32 && numSamples > 8 )
	{
		ms.Count = 16;
	}
	else if( numSamples < 16 && numSamples > 4 )
	{
		ms.Count = 8;
	}
	else if( numSamples < 8 && numSamples > 2 )
	{
		ms.Count = 4;
	}
	else if( numSamples < 4 && numSamples > 1 )
	{
		ms.Count = 2;
	}

	UINT qualityLevels = 0;
	auto hr = device->CheckMultisampleQualityLevels( format, ms.Count, &qualityLevels );
	assert( hr == S_OK );
	assert( qualityLevels > 0 );

	ms.Quality = qualityLevels - 1;

	return;
	///////////////////////////////////

	ms.Count = 1;
	ms.Quality = 0;

	UINT msCount[] = { 32, 16, 8, 4, 2 };
	for( uint32 i = 0; i < _countof( msCount ); i++ )
	{
		UINT qualityLevels = 0;
		auto res = device->CheckMultisampleQualityLevels( format, msCount[ i ], &qualityLevels );
		if( res == S_OK && qualityLevels > 0 )
		{
			ms.Count = msCount[ i ];
			ms.Quality = qualityLevels - 1;
			break;
		}
	}
}

ID_INLINE int DXTBlockSizeAligned( int x )
{
	return ( x + 3 ) & ~0x3;
}

/*
========================
idImage::SubImageUpload
========================
*/
void idImage::SubImageUpload( int mipLevel, int x, int y, int z, int width, int height, const void * pic, int pixelPitch ) const
{
	auto iContext = GetCurrentImContext();
	//auto device = GetCurrentDevice();

	auto tex = ( dxTextureObject_t * )GetAPIObject();
	assert( tex != nullptr );

	int index = z * opts.numLevels + mipLevel;

	//assert(x == 0 && y == 0); // @pjb: todo
	//
	//D3D11_MAPPED_SUBRESOURCE map;
	//if ( FAILED( pContext->Map( pTexture, index, D3D11_MAP_WRITE_DISCARD, 0, &map ) ) ) {
	//    return;
	//}
	//
	//memcpy( map.pData, pic, StorageSize() );
	//pContext->Unmap( pTexture, index );

	int surfPitch;
	if( !pixelPitch )
	{
		if( IsCompressed() )
		{
			width = DXTBlockSizeAligned( width );
			height = DXTBlockSizeAligned( height );
			pixelPitch = ( ( width / 4 ) * int64( 16 ) * BitsForFormat( opts.format ) ) / 8;
			surfPitch = pixelPitch * ( height / 4 );
		}
		else {
			pixelPitch = width * BitsForFormat( opts.format ) / 8;
			surfPitch = pixelPitch * height;
		}
	}

	DX_BOX box = { x, y, 0, x + width, y + height, 1 };

	iContext->UpdateSubresource( tex->res, index, &box, pic, pixelPitch, surfPitch );
}
/*
========================
 idImage::SetPixel
========================
*/
void idImage::SetPixel( int mipLevel, int x, int y, const void* data, int dataSize )
{
	SubImageUpload( mipLevel, x, y, 0, 1, 1, data );
}

//idCVar r_defaultFiltering( "r_defaultFiltering", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, 
//	"Set default mipped filtering: 0-Anisotropic, 1-Trilinear, 2-Bilinear, 3-Point", 0.0, 3.0 );

/*
========================
idImage::SetTexParameters
========================
*/
void idImage::SetTexParameters()
{
	//auto iContext = GetCurrentImContext();
	auto device = GetCurrentDevice();

	bool bUseComparisonState = ( usage == TD_SHADOWMAP );

	DX_SAMPLER_DESC desc = {};
	desc.MaxAnisotropy = 1;

	switch( filter )
	{
		case TF_DEFAULT:
		{
			// only do aniso filtering on mip mapped images
			if( glConfig.anisotropicFilterAvailable )
			{
				int aniso = r_maxAnisotropicFiltering.GetInteger();
				if( aniso != 0 )
				{
					if( aniso > glConfig.maxTextureAnisotropy )
					{
						aniso = glConfig.maxTextureAnisotropy;
					}
					if( aniso < 0 )
					{
						aniso = 0;
					}

					desc.MaxAnisotropy = aniso;
					desc.Filter = bUseComparisonState ? D3D11_FILTER_COMPARISON_ANISOTROPIC : D3D11_FILTER_ANISOTROPIC;
				}
			} 
			else if( r_useTrilinearFiltering.GetBool() )
			{
				desc.Filter = bUseComparisonState ? D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			}
			else { // Bilinear
				desc.Filter = bUseComparisonState ? D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT : D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			}
		} break;

		case TF_LINEAR: 
		case TF_LINEAR_MIPMAP_NEAREST:
			desc.Filter = bUseComparisonState ? D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT : D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT; 
			break;

		case TF_NEAREST:
		case TF_NEAREST_MIPMAP:
		case TF_NEAREST_MIPMAP_NEAREST:
			desc.Filter = bUseComparisonState ? D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT : D3D11_FILTER_MIN_MAG_MIP_POINT;
			break;

		default:
			common->FatalError( "%s: bad texture filter %d", GetName(), filter );
	}

	// RB: disabled use of unreliable extension that can make the game look worse
	/*
	if( usage != TD_FONT )
	{
		// use a blurring LOD bias in combination with high anisotropy to fix our aliasing grate textures...
		desc.MipLODBias = r_lodBias.GetFloat();
	}
	*/
	// RB end

	switch( repeat )
	{
		case TR_REPEAT:
			desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			break;
		case TR_CLAMP_TO_ZERO:
			desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
			desc.BorderColor[ 3 ] = 1.0f;
			break;
		case TR_CLAMP_TO_ZERO_ALPHA:
			desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
			break;
		case TR_CLAMP:
			desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			break;
		case TR_MIRROR:
			desc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
			break;

		default:
			common->FatalError( "%s: bad texture repeat %d", GetName(), repeat );
	}
	
	desc.MaxLOD = opts.numLevels - 1;

	if( bUseComparisonState )
		//if( opts.format == FMT_DEPTH || opts.format == FMT_DEPTH_STENCIL )
	{
		desc.ComparisonFunc = D3D11_COMPARISON_LESS; // D3D11_COMPARISON_LESS_EQUAL
	}
	else {
		desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	}

	// Create and store unique sampler object

	auto tex = ( dxTextureObject_t * )GetAPIObject();
	assert( tex != nullptr );

	// D3D11 will return the same pointer if the particular state description was already created.
	auto res = device->CreateSamplerState( &desc, &tex->smp );
	assert( res == S_OK );
}

/*
========================
 idImage::EnableDepthCompareMode
========================
*/
void idImage::EnableDepthCompareModeOGL()
{
	glTextureObject_t tex;
	tex.DeriveTargetInfo( this );

	// GL_INTENSITY = dddd
	// GL_LUMINANCE = ddd1
	// GL_ALPHA = 000d
	// GL_RED = d001
	glTexParameteri( tex.target, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY );
	glTexParameteri( tex.target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
	glTexParameteri( tex.target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
}

/*
========================
 idImage::AllocImage

	Every image will pass through this function. Allocates all the necessary MipMap levels for the
	Image, but doesn't put anything in them.

	This should not be done during normal game-play, if you can avoid it.
========================
*/
void idImage::AllocImage()
{
	PurgeImage();

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before OpenGL starts would miss
	// the generated texture
	if( !renderSystem->IsRenderDeviceRunning() )
		return;

	if( !apiObject ) apiObject = new dxTextureObject_t();

	//auto iContext = GetCurrentImContext();
	auto device = GetCurrentDevice();

	//----------------------------------------------------
	// allocate all the mip levels with NULL data
	// desc.Usage = D3D11_USAGE_DEFAULT;	// read-write GPU  ->UpdateSubresource   Input / Input 
	// desc.Usage = D3D11_USAGE_IMMUTABLE;	// read GPU
	// desc.Usage = D3D11_USAGE_DYNAMIC;	// read GPU, write CPU  ->Map 
	// desc.Usage = D3D11_USAGE_STAGING;	// transfer(copy) from GPU to CPU  ->CopySubresourceRegion, CopyResource
	//----------------------------------------------------

	if( GetOpts().textureType == TT_3D )
	{
		DX_TEXTURE3D_DESC desc = {};

		desc.Width = idMath::Max( opts.width, 1 );
		desc.Height = idMath::Max( opts.height, 1 );
		desc.Depth = idMath::Max( opts.depth, 1 );
		desc.MipLevels = idMath::Max( opts.numLevels, 1 );
		desc.Format = DeriveFormat( this );

		desc.Usage = D3D11_USAGE_DEFAULT;	// read-write GPU  ->UpdateSubresource   Input / Input 
		//desc.Usage = D3D11_USAGE_IMMUTABLE; // read GPU
		//desc.Usage = D3D11_USAGE_DYNAMIC; // read GPU, write CPU  ->Map 
		//desc.Usage = D3D11_USAGE_STAGING; // transfer(copy) from GPU to CPU  ->CopySubresourceRegion, CopyResource

		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		if( GetOpts().renderTarget )
			desc.BindFlags |= ( IsDepth() || IsDepthStencil() )? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET;

		DXTexture3D * pTexture = nullptr;
		auto res = device->CreateTexture3D( &desc, NULL, &pTexture );
		assert( res == S_OK );

		idStrStatic<128> name;
		name.Format( "idImage(%s)", GetName() );
		D3DSetDebugObjectName( pTexture, GetName() );

		DX_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
		//srvDesc.Texture3D.MostDetailedMip = 0;
		srvDesc.Texture3D.MipLevels = desc.MipLevels;

		DXShaderResourceView* pSRV = nullptr;
		res = device->CreateShaderResourceView( pTexture, &srvDesc, &pSRV );
		assert( res == S_OK );

		auto tex = ( dxTextureObject_t * )GetAPIObject();
		tex->res = pTexture;
		tex->view = pSRV;
	}
	else if( GetOpts().textureType == TT_CUBIC ) 
	{
		DX_TEXTURE2D_DESC desc = {};
		
		desc.Width = idMath::Max( opts.width, 1 );
		desc.Height = desc.Width;
		desc.MipLevels = idMath::Max( opts.numLevels, 1 );
		desc.ArraySize = idMath::Max( opts.depth, 1 ) * 6;
		desc.Format = DeriveFormat( this );
		desc.SampleDesc.Count = 1;

		desc.Usage = D3D11_USAGE_DEFAULT;	// read-write GPU  ->UpdateSubresource   Input / Input 
		//desc.Usage = D3D11_USAGE_IMMUTABLE; // read GPU
		//desc.Usage = D3D11_USAGE_DYNAMIC; // read GPU, write CPU  ->Map 
		//desc.Usage = D3D11_USAGE_STAGING; // transfer(copy) from GPU to CPU  ->CopySubresourceRegion, CopyResource
		
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		if( GetOpts().renderTarget )
			desc.BindFlags |= ( IsDepth() || IsDepthStencil() )? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET;

		desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

		DXTexture2D * pTexture = nullptr;
		auto res = device->CreateTexture2D( &desc, NULL, &pTexture );
		assert( res == S_OK );

		idStrStatic<128> name;
		name.Format( "idImage(%s)", GetName() );
		D3DSetDebugObjectName( pTexture, GetName() );

		DX_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		if( IsArray() )
		{
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
			//srvDesc.TextureCubeArray.MostDetailedMip = 0;
			srvDesc.TextureCubeArray.MipLevels = desc.MipLevels;
			//srvDesc.TextureCubeArray.First2DArrayFace = 0;
			srvDesc.TextureCubeArray.NumCubes = idMath::Max( opts.depth, 1 );
		}
		else {
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			//srvDesc.TextureCube.MostDetailedMip = 0;
			srvDesc.TextureCube.MipLevels = desc.MipLevels;
		}
		DXShaderResourceView* pSRV = nullptr;
		res = device->CreateShaderResourceView( pTexture, &srvDesc, &pSRV );
		assert( res == S_OK );

		auto tex = ( dxTextureObject_t * )GetAPIObject();
		tex->res = pTexture;
		tex->view = pSRV;
	}
	else // TT_2D
	{
		DX_TEXTURE2D_DESC desc = {};

		desc.Width = idMath::Max( opts.width, 1 );
		desc.Height = idMath::Max( opts.height, 1 );
		desc.MipLevels = idMath::Max( opts.numLevels, 1 );
		desc.ArraySize = idMath::Max( opts.depth, 1 );
		desc.Format = DeriveFormat( this );

		int numSamples = idMath::Max( opts.numSamples, 1 );
		GetMultisampleCount( numSamples, desc.Format, desc.SampleDesc );

		desc.Usage = D3D11_USAGE_DEFAULT;	// read-write GPU  ->UpdateSubresource   Input / Input 
		//desc.Usage = D3D11_USAGE_IMMUTABLE; // read GPU
		//desc.Usage = D3D11_USAGE_DYNAMIC; // read GPU, write CPU  ->Map 
		//desc.Usage = D3D11_USAGE_STAGING; // transfer(copy) from GPU to CPU  ->CopySubresourceRegion, CopyResource

		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		if( GetOpts().renderTarget )
			desc.BindFlags |= ( IsDepth() || IsDepthStencil() )? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET;

		DXTexture2D * pTexture = nullptr;
		auto res = device->CreateTexture2D( &desc, nullptr, &pTexture );
		assert( res == S_OK );

		idStrStatic<128> name;
		name.Format( "idImage(%s)", GetName() );
		D3DSetDebugObjectName( pTexture, GetName() );

		DX_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		if( IsMultisampled() )
		{
			if( IsArray() )
			{
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
				srvDesc.Texture2DMSArray.ArraySize = desc.ArraySize;
				//srvDesc.Texture2DMSArray.FirstArraySlice = 0;
			}
			else {
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
				//srvDesc.Texture2DMS.UnusedField_NothingToDefine = 0;
			}
		}
		else {
			if( IsArray() )
			{
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
				//srvDesc.Texture2DArray.MostDetailedMip = 0;
				srvDesc.Texture2DArray.MipLevels = desc.MipLevels;
				//srvDesc.Texture2DArray.FirstArraySlice = 0;
				srvDesc.Texture2DArray.ArraySize = desc.ArraySize;
			}
			else {
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				//srvDesc.Texture2D.MostDetailedMip = 0;
				srvDesc.Texture2D.MipLevels = desc.MipLevels;
			}
		}
		DXShaderResourceView* pSRV = nullptr;
		res = device->CreateShaderResourceView( pTexture, &srvDesc, &pSRV );
		assert( res == S_OK );

		auto tex = ( dxTextureObject_t * )GetAPIObject();
		tex->res = pTexture;
		tex->view = pSRV;
	}

	// Non sampler parameters

	// ALPHA, LUMINANCE, LUMINANCE_ALPHA, and INTENSITY have been removed
	// in OpenGL 3.2. In order to mimic those modes, we use the swizzle operators

	/*if( GetOpts().colorFormat == CFM_GREEN_ALPHA )
	{
		GLint swizzleMask[] = { GL_ONE, GL_ONE, GL_ONE, GL_GREEN };
	}
	else if( GetOpts().format == FMT_LUM8 )
	{
		GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
	}
	else if( GetOpts().format == FMT_L8A8 )
	{
		GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_GREEN };
	}
	else if( GetOpts().format == FMT_ALPHA )
	{
		GLint swizzleMask[] = { GL_ONE, GL_ONE, GL_ONE, GL_RED };
	}
	else if( GetOpts().format == FMT_INT8 )
	{
		GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_RED };
	}
	else {
		GLint swizzleMask[] = { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA };
	}*/

	SetTexParameters();
}

/*
========================
 idImage::PurgeImage
========================
*/
void idImage::PurgeImage()
{
	if( IsLoaded() )
	{
		auto tex = ( dxTextureObject_t * )GetAPIObject();
		SAFE_RELEASE( tex->smp );
		SAFE_RELEASE( tex->view );
		SAFE_RELEASE( tex->res );
	}

	// clear all the current binding caches, so the next bind will do a real one
	for( int i = 0; i < MAX_MULTITEXTURE_UNITS; i++ )
	{
		for( int j = 0; j < target_Max; j++ )
		{
			backEnd.glState.tmu[ i ].currentTarget[ j ] = GL_NONE;
		}
	}
}

/*
========================
idImage::SetSamplerState
========================
*/
void idImage::SetSamplerState( textureFilter_t tf, textureRepeat_t tr )
{
	if( tf == filter && tr == repeat )
		return;

	filter = tf;
	repeat = tr;

	auto tex = ( dxTextureObject_t * )GetAPIObject();
	SAFE_RELEASE( tex->smp );

	SetTexParameters();
}





/*
========================
 idImage::GenerateImage		2D / 2DArray
========================
*/
void idImage::GenerateImage( const byte* pic, int width, int height, int depth, int msaaSamples,
	textureFilter_t filterParm, textureRepeat_t repeatParm, textureUsage_t usageParm )
{
	PurgeImage();

	filter = filterParm;
	repeat = repeatParm;
	usage = usageParm;
	layout = IMG_LAYOUT_2D;

	opts.textureType = TT_2D;
	opts.width = idMath::Max( width, 1 );
	opts.height = idMath::Max( height, 1 );
	opts.depth = idMath::Max( depth, 1 );
	opts.numLevels = 0;
	opts.numSamples = 1;
	DeriveOpts();

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before the render starts would miss
	// the generated texture
	if( !renderSystem->IsRenderDeviceRunning() )
		return;

	// RB: allow pic == NULL for internal framebuffer images
	if( pic == NULL || GetOpts().IsMultisampled() )
	{
		AllocImage();
		return;
	}

	if( !apiObject ) apiObject = new dxTextureObject_t();

	idBinaryImage im( GetName() );

	// foresthale 2014-05-30: give a nice progress display when binarizing
	commonLocal.LoadPacifierBinarizeFilename( GetName(), "generated image" );

	if( GetOpts().numLevels > 1 )
	{
		commonLocal.LoadPacifierBinarizeProgressTotal( GetOpts().width * GetOpts().height * 4 / 3 );
	}
	else {
		commonLocal.LoadPacifierBinarizeProgressTotal( GetOpts().width * GetOpts().height );
	}

	im.Load2DFromMemory( width, height, pic, GetOpts().numLevels, opts.format, opts.colorFormat, GetOpts().gammaMips );

	commonLocal.LoadPacifierBinarizeEnd();

	///AllocImage();
	{
		//auto iContext = GetCurrentImContext();
		auto device = GetCurrentDevice();

		// This is a bit conservative because the hardware could support larger textures than
		// the Feature Level defined minimums.
		int maxsize = 0;
		switch( device->GetFeatureLevel() )
		{
			case D3D_FEATURE_LEVEL_10_0:
			case D3D_FEATURE_LEVEL_10_1:
				maxsize = D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION;
				break;

			default:
				maxsize = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
				break;
		}

		DX_TEXTURE2D_DESC desc = {};

		desc.Width = GetOpts().width;
		desc.Height = GetOpts().height;
		desc.MipLevels = idMath::Max( GetOpts().numLevels, 1 );
		desc.ArraySize = GetOpts().depth;
		desc.Format = DeriveFormat( this );
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		// A subresource is a single mipmap-level surface.
		auto initData = ( D3D11_SUBRESOURCE_DATA * )_alloca( im.NumImages() * sizeof( D3D11_SUBRESOURCE_DATA ) );
		for( int i = 0; i < im.NumImages(); ++i )
		{
			auto & img = im.GetImageHeader( i );
			///const byte* data = im.GetImageData( i );
			///SubImageUpload( img.level, 0, 0, img.destZ, img.width, img.height, data );

			int rowBytes, numBytes;

			if( IsCompressed() )
			{
				int width = DXTBlockSizeAligned( img.width );
				int height = DXTBlockSizeAligned( img.height );
				rowBytes = ( ( width / 4 ) * 16 * BitsForFormat( GetOpts().format ) ) / 8;
				numBytes = rowBytes * ( height / 4 );
			}
			else {
				rowBytes = img.width * BitsForFormat( GetOpts().format ) / 8;
				numBytes = rowBytes * img.height;
			}

			initData[ i ].pSysMem = im.GetImageData( i );
			initData[ i ].SysMemPitch = static_cast<UINT>( rowBytes ); // 2D and 3D textures
			initData[ i ].SysMemSlicePitch = static_cast<UINT>( numBytes ); // for 3D textures
		}

		DXTexture2D * pTexture = nullptr;
		auto res = device->CreateTexture2D( &desc, initData, &pTexture );
		assert( res == S_OK );

		idStrStatic<128> name;
		name.Format( "idImage(%s)", GetName() );
		D3DSetDebugObjectName( pTexture, GetName() );

		DX_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		if( IsMultisampled() )
		{
			if( IsArray() )
			{
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
				srvDesc.Texture2DMSArray.ArraySize = desc.ArraySize;
				//srvDesc.Texture2DMSArray.FirstArraySlice = 0;
			}
			else {
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
				//srvDesc.Texture2DMS.UnusedField_NothingToDefine = 0;
			}
		}
		else {
			if( IsArray() )
			{
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
				//srvDesc.Texture2DArray.MostDetailedMip = 0;
				srvDesc.Texture2DArray.MipLevels = desc.MipLevels;
				//srvDesc.Texture2DArray.FirstArraySlice = 0;
				srvDesc.Texture2DArray.ArraySize = desc.ArraySize;
			}
			else {
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				//srvDesc.Texture2D.MostDetailedMip = 0;
				srvDesc.Texture2D.MipLevels = desc.MipLevels;
			}
		}
		DXShaderResourceView* pSRV = nullptr;
		res = device->CreateShaderResourceView( pTexture, &srvDesc, &pSRV );
		assert( res == S_OK );

		auto tex = ( dxTextureObject_t * )GetAPIObject();
		tex->res = pTexture;
		tex->view = pSRV;
	}
}

/*
====================
 idImage::GenerateCubeImage

	Non-square cube sides are not allowed
====================
*/
void idImage::GenerateCubeImage( const byte* pic[ 6 ], int size, int depth, textureFilter_t filterParm, textureUsage_t usageParm )
{
	PurgeImage();

	filter = filterParm;
	repeat = TR_CLAMP;
	usage = usageParm;
	layout = IMG_LAYOUT_CUBE_NATIVE;

	opts.textureType = TT_CUBIC;
	opts.width = opts.height = idMath::Max( size, 1 );
	opts.depth = idMath::Max( depth, 1 );
	opts.numLevels = 0;
	opts.numSamples = 1;
	DeriveOpts();

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before the render starts would miss
	// the generated texture
	if( !renderSystem->IsRenderDeviceRunning() )
		return;

	if( !apiObject ) apiObject = new dxTextureObject_t();

	idBinaryImage im( GetName() );

	// foresthale 2014-05-30: give a nice progress display when binarizing
	commonLocal.LoadPacifierBinarizeFilename( GetName(), "generated cube image" );
	if( opts.numLevels > 1 )
	{
		commonLocal.LoadPacifierBinarizeProgressTotal( opts.width * opts.width * 6 * 4 / 3 );
	}
	else {
		commonLocal.LoadPacifierBinarizeProgressTotal( opts.width * opts.width * 6 );
	}

	im.LoadCubeFromMemory( size, pic, opts.numLevels, opts.format, opts.gammaMips );

	commonLocal.LoadPacifierBinarizeEnd();

	///AllocImage();
	{
		//auto iContext = GetCurrentImContext();
		auto device = GetCurrentDevice();

		DX_TEXTURE2D_DESC desc = {};
		
		desc.Width = idMath::Max( opts.width, 1 );
		desc.Height = desc.Width;
		desc.MipLevels = idMath::Max( opts.numLevels, 1 );
		desc.ArraySize = idMath::Max( opts.depth, 1 ) * 6;
		desc.Format = DeriveFormat( this );
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_IMMUTABLE;	
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

		// A subresource is a single mipmap-level surface.
		auto initData = ( D3D11_SUBRESOURCE_DATA * )_alloca( im.NumImages() * sizeof( D3D11_SUBRESOURCE_DATA ) );
		for( int i = 0; i < im.NumImages(); ++i )
		{
			auto & img = im.GetImageHeader( i );
			///const byte* data = im.GetImageData( i );
			///SubImageUpload( img.level, 0, 0, img.destZ, img.width, img.height, data );

			int rowBytes, numBytes;

			if( IsCompressed() )
			{
				int width = DXTBlockSizeAligned( img.width );
				int height = DXTBlockSizeAligned( img.height );
				rowBytes = ( ( width / 4 ) * 16 * BitsForFormat( GetOpts().format ) ) / 8;
				numBytes = rowBytes * ( height / 4 );
			}
			else {
				rowBytes = img.width * BitsForFormat( GetOpts().format ) / 8;
				numBytes = rowBytes * img.height;
			}

			initData[ i ].pSysMem = im.GetImageData( i );
			initData[ i ].SysMemPitch = static_cast<UINT>( rowBytes );
			initData[ i ].SysMemSlicePitch = static_cast<UINT>( numBytes );
		}

		DXTexture2D * pTexture = nullptr;
		auto res = device->CreateTexture2D( &desc, NULL, &pTexture );
		assert( res == S_OK );

		idStrStatic<128> name;
		name.Format( "idImage(%s)", GetName() );
		D3DSetDebugObjectName( pTexture, GetName() );

		DX_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		if( IsArray() )
		{
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
			//srvDesc.TextureCubeArray.MostDetailedMip = 0;
			srvDesc.TextureCubeArray.MipLevels = desc.MipLevels;
			//srvDesc.TextureCubeArray.First2DArrayFace = 0;
			srvDesc.TextureCubeArray.NumCubes = idMath::Max( opts.depth, 1 );
		}
		else {
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			//srvDesc.TextureCube.MostDetailedMip = 0;
			srvDesc.TextureCube.MipLevels = desc.MipLevels;
		}
		DXShaderResourceView* pSRV = nullptr;
		res = device->CreateShaderResourceView( pTexture, &srvDesc, &pSRV );
		assert( res == S_OK );

		auto tex = ( dxTextureObject_t * )GetAPIObject();
		tex->res = pTexture;
		tex->view = pSRV;
	}
}