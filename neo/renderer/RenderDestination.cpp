/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2014-2016 Robert Beckebans

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

#include "tr_local.h"
//#include "Framebuffer.h"

/*
=================================================
 idRenderDestinationManager()
=================================================
*/
idRenderDestinationManager::idRenderDestinationManager()
{
	bIsInitialized = false;
	renderDestList.Clear();
	renderTargetImgList.Clear();
}

/*
=================================================
 GetByName
=================================================
*/
idRenderDestination * idRenderDestinationManager::FinedByName( const char * name ) const
{
	for( int i = 0; i < renderDestList.Num(); i++ )
	{
		if( !idStr::Icmp( renderDestList[ i ]->GetName(), name ) )
		{
			return renderDestList[ i ];
		}
	}
	return nullptr;
}
/*
=================================================
 GetByName
=================================================
*/
idImage * idRenderDestinationManager::FinedRenderTargetImageByName( const char * name ) const
{
	for( int i = 0; i < renderTargetImgList.Num(); i++ )
	{
		if( !idStr::Icmp( renderTargetImgList[ i ]->GetName(), name ) )
		{
			return renderTargetImgList[ i ];
		}
	}
	return nullptr;
}

/*
=================================================
 CreateRenderTargetImage
=================================================
*/
idImage * idRenderDestinationManager::CreateRenderTargetImage( const char * Name,
	textureType_t Type, int Width, int Height, int Layers, int Levels, int Samples,
	textureFormat_t Format, textureFilter_t Filter, textureWrap_t WrapMode, bool CmpMode )
{
	idImage * image = nullptr;
	for( int i = 0; i < renderTargetImgList.Num(); i++ )
	{
		if( !idStr::Icmp( renderTargetImgList[ i ]->GetName(), Name ) )
		{
			image = renderTargetImgList[ i ];
			break;
		}
	}
	if( !image )
	{
		image = renderImageManager->CreateStandaloneImage( Name );
		renderTargetImgList.Append( image );
	}

	idImageOpts opts;
	opts.textureType = Type;
	opts.format = Format;
	opts.colorFormat = CFM_DEFAULT;
	opts.width = Width;
	opts.height = Height;
	opts.depth = Layers;
	opts.numLevels = Levels;
	opts.numSamples = Samples;
	opts.gammaMips = false;
	opts.readback = false;

	idSamplerOpts smp;
	smp.filter = Filter;
	smp.wrap = WrapMode;
	smp.depthCompareMode = CmpMode;

	image->AllocImage( opts, smp );

	return image;
}

/*
=================================================
 Init
=================================================
*/
void idRenderDestinationManager::Init()
{
	GL_ResetRenderDestination();

	const int RenderWidth = renderSystem->GetWidth();
	const int RenderHeight = renderSystem->GetHeight();
	const int RenderWidthQuarter = RenderWidth >> 2;
	const int RenderHeightQuarter = RenderHeight >> 2;

	if( 1 )
	{
		viewDepthStencilImage = CreateRenderTargetImage( "_viewDepthStencilImage", TT_2D, RenderWidth, RenderHeight, 1, GetMipCountForResolution( RenderWidth, RenderHeight ), 1, FMT_DEPTH_STENCIL, TF_NEAREST, TR_CLAMP );
		viewColorHDRImage = CreateRenderTargetImage( "_viewColorHDRImage", TT_2D, RenderWidth, RenderHeight, 1, 1, 1, FMT_RGBA16F, TF_NEAREST, TR_CLAMP );
		viewColorLDRImage = CreateRenderTargetImage( "_viewColorLDRImage", TT_2D, RenderWidth, RenderHeight, 1, 1, 1, FMT_RGBA8, TF_NEAREST, TR_CLAMP );

		///viewVelocityImage = CreateRenderTargetImage( "_viewVelocityImage", TT_2D, RenderWidth, RenderHeight, 1, 1, 1, FMT_RGB10_A2, TF_NEAREST, TR_CLAMP );

		for( uint32 i = 0; i < MAX_SHADOWMAP_RESOLUTIONS; i++ )
		{
			int smSize = shadowMapResolutions[ i ];
			shadowBufferImage[ i ] = CreateRenderTargetImage( va( "_shadowBuffer_%i", smSize ), TT_2D, smSize, smSize, 6, 1, 1, FMT_DEPTH, TF_LINEAR, TR_CLAMP, true );
		}
		///viewShadowMaskImage = CreateRenderTargetImage( "_viewShadowMaskImage", TT_2D, SHADOWMASK_RESOLUTION, SHADOWMASK_RESOLUTION, 1, 1, 1, FMT_RGBA8, TF_NEAREST, TR_MIRROR );
		///viewShadowMaskImagePrev = CreateRenderTargetImage( "_viewShadowMaskImagePrev", TT_2D, SHADOWMASK_RESOLUTION, SHADOWMASK_RESOLUTION, 1, 1, 1, FMT_RGBA8, TF_NEAREST, TR_MIRROR );

		viewColorsImage = CreateRenderTargetImage( "_viewColorsImage", TT_2D, RenderWidth, RenderHeight, 1, 1, 1, FMT_RGBA8, TF_NEAREST, TR_MIRROR );
		viewNormalImage = CreateRenderTargetImage( "_viewNormalImage", TT_2D, RenderWidth, RenderHeight, 1, 1, 1, FMT_RGB10_A2, TF_NEAREST, TR_CLAMP );

		viewColorHDRQuarterImage = CreateRenderTargetImage( "_viewColorHDRQuarterImage", TT_2D, RenderWidthQuarter, RenderHeightQuarter, 1, 1, 1, FMT_RGBA16F, TF_NEAREST, TR_CLAMP );
		viewColorHDRImageSml = CreateRenderTargetImage( "_viewColorHDRSml", TT_2D, 64, 64, 1, 1, 1, FMT_RGBA16F, TF_NEAREST, TR_CLAMP );
		//viewColorHDRImageSmlPrev = CreateRenderTargetImage( "_viewColorHDRSmlPrev", TT_2D, 64, 64, 1, 1, 1, FMT_RGBA16F, TF_NEAREST, TR_CLAMP );

		hiZbufferImage = CreateRenderTargetImage( "_cszBuffer", TT_2D, RenderWidth, RenderHeight, 1, MAX_HIERARCHICAL_ZBUFFERS, 1, FMT_R32F, TF_NEAREST_MIPMAP, TR_CLAMP );
		ambientOcclusionImage[ 0 ] = CreateRenderTargetImage( "_ao0", TT_2D, RenderWidth, RenderHeight, 1, 1, 1, FMT_RGBA8, TF_LINEAR, TR_CLAMP );
		ambientOcclusionImage[ 1 ] = CreateRenderTargetImage( "_ao1", TT_2D, RenderWidth, RenderHeight, 1, 1, 1, FMT_RGBA8, TF_LINEAR, TR_CLAMP );

		bloomRenderImage[ 0 ] = CreateRenderTargetImage( "_bloomRender0", TT_2D, RenderWidthQuarter, RenderHeightQuarter, 1, 1, 1, FMT_RGBA8, TF_LINEAR, TR_CLAMP );
		bloomRenderImage[ 1 ] = CreateRenderTargetImage( "_bloomRender1", TT_2D, RenderWidthQuarter, RenderHeightQuarter, 1, 1, 1, FMT_RGBA8, TF_LINEAR, TR_CLAMP );

		//viewColorLDRImagePrev = CreateRenderTargetImage( "_viewColorLDRImagePrev", TT_2D, RenderWidth, RenderHeight, 1, 1, 1, FMT_RGBA8, TF_NEAREST, TR_MIRROR );

		///smaaInputImage = CreateRenderTargetImage( "_smaaInput", TT_2D, RenderWidth, RenderHeight, 1, 1, 1, FMT_RGBA8, TF_NEAREST, TR_MIRROR );
		smaaEdgesImage = CreateRenderTargetImage( "_smaaEdges", TT_2D, RenderWidth, RenderHeight, 1, 1, 1, FMT_RGBA8, TF_LINEAR, TR_CLAMP );
		smaaBlendImage = CreateRenderTargetImage( "_smaaBlend", TT_2D, RenderWidth, RenderHeight, 1, 1, 1, FMT_RGBA8, TF_LINEAR, TR_CLAMP );

		//lumaImage = CreateRenderTargetImage( "_luma", TT_2D, 2, 2, 1, 1, 1, FMT_R16F, TF_NEAREST, TR_CLAMP );
		//autoExposureImage[ 0 ] = CreateRenderTargetImage( "_autoExp0", TT_2D, 2, 2, 1, 1, 1, FMT_R16F, TF_NEAREST, TR_CLAMP );
		//autoExposureImage[ 1 ] = CreateRenderTargetImage( "_autoExp1", TT_2D, 2, 2, 1, 1, 1, FMT_R16F, TF_NEAREST, TR_CLAMP );

		//bloomRenderMipsImage = CreateRenderTargetImage( "_bloomRenderMips", TT_2D, RenderWidth, RenderHeight, 1, MAX_BLOOM_BUFFER_MIPS, 1, FMT_RGBA16F, TF_LINEAR, TR_CLAMP );
		bloomRenderMipsImage[ 0 ] = CreateRenderTargetImage( "_bloomRenderMip0", TT_2D, RenderWidth, RenderHeight, 1, 1, 1, FMT_RGBA16F, TF_LINEAR, TR_CLAMP );
		bloomRenderMipsImage[ 1 ] = CreateRenderTargetImage( "_bloomRenderMip1", TT_2D, RenderWidth / 2, RenderHeight / 2, 1, 1, 1, FMT_RGBA16F, TF_LINEAR, TR_CLAMP );
		bloomRenderMipsImage[ 2 ] = CreateRenderTargetImage( "_bloomRenderMip2", TT_2D, RenderWidth / 4, RenderHeight / 4, 1, 1, 1, FMT_RGBA16F, TF_LINEAR, TR_CLAMP );
		bloomRenderMipsImage[ 3 ] = CreateRenderTargetImage( "_bloomRenderMip3", TT_2D, RenderWidth / 8, RenderHeight / 8, 1, 1, 1, FMT_RGBA16F, TF_LINEAR, TR_CLAMP );
		bloomRenderMipsImage[ 4 ] = CreateRenderTargetImage( "_bloomRenderMip4", TT_2D, RenderWidth / 16, RenderHeight / 16, 1, 1, 1, FMT_RGBA16F, TF_LINEAR, TR_CLAMP );
		bloomRenderMipsImage[ 5 ] = CreateRenderTargetImage( "_bloomRenderMip5", TT_2D, RenderWidth / 32, RenderHeight / 32, 1, 1, 1, FMT_RGBA16F, TF_LINEAR, TR_CLAMP );
	}

	// Init render destinations

	// MAIN RENDER
	{
		idRenderDestination::targetList_t targets;
		targets.Append( viewColorHDRImage );
		renderDestBaseHDR = CreateRenderDestination( "_rd_baseHDRrender" );
		renderDestBaseHDR->CreateFromImages( &targets, viewDepthStencilImage );
	}
	{
		idRenderDestination::targetList_t targets;
		targets.Append( viewColorLDRImage );
		renderDestBaseLDR = CreateRenderDestination( "_rd_baseLDRrender" );
		renderDestBaseLDR->CreateFromImages( &targets, viewDepthStencilImage );
	}
	{
		//renderDestDepthStencil = CreateRenderDestination( "_rd_currentDepthStencil" );
		//renderDestDepthStencil->CreateFromImages( nullptr, viewDepthStencilImage );

		// Will be used for CMD_SetBuffer
		//idRenderDestination::targetList_t targets;
		//targets.Append( renderImageManager->currentRenderImage );
		//renderDestRenderToTexture = CreateRenderDestination( "_rd_renderToTexture" );
		//renderDestRenderToTexture->CreateFromImages( &targets, viewDepthStencilImage );

		///idRenderDestination::targetList_t targets;
		///targets.Append( renderImageManager->currentRenderImage );
		///renderDestCopyCurrentRender = CreateRenderDestination( "_rd_copyCurrentRender" );
		///renderDestDepthStencil->CreateFromImages( &targets, nullptr, nullptr );
	}

	// SHADOWMAPS
	{
		for( uint32 i = 0; i < MAX_SHADOWMAP_RESOLUTIONS; i++ )
		{
			renderDestShadowOnePass[ i ] = CreateRenderDestination( va( "_rd_shadowBuffer_%i", shadowMapResolutions[ i ] ) );
			renderDestShadowOnePass[ i ]->CreateFromImages( nullptr, shadowBufferImage[ i ] );
		}
		///renderDestShadow = CreateRenderDestination( "_rd_shadowBuffer" );
		///renderDestShadow->CreateFromImages( nullptr, renderImageManager->shadowMapArray );

		/*idRenderDestination::targetList_t targets;
		targets.Append( renderImageManager->shadowMaskImage[ 0 ] );
		targets.Append( renderImageManager->shadowMaskImage[ 1 ] );
		renderDestShadowMask = CreateRenderDestination( "_rd_shadowMask" );
		renderDestShadowMask->CreateFromImages( &targets, nullptr );*/
	}

	// GBUFFERS
	{
		idRenderDestination::targetList_t targets;
		targets.Append( viewColorHDRImage );
		targets.Append( viewNormalImage );
		renderDestGBufferSml = CreateRenderDestination( "_rd_gBufferSml" );
		renderDestGBufferSml->CreateFromImages( &targets, viewDepthStencilImage );

		targets.Clear();
		targets.Append( viewColorsImage ); // rt0
		targets.Append( viewNormalImage ); // rt1
		renderDestGBuffer = CreateRenderDestination( "_rd_gbuffer" );
		renderDestGBuffer->CreateFromImages( &targets, viewDepthStencilImage );
	}

	// HDR DOWNSCALE
	{
		idRenderDestination::targetList_t targets;
		targets.Append( viewColorHDRImageSml );
		renderDestBaseHDRsml64 = CreateRenderDestination( "_rd_baseHDRsml64" );
		renderDestBaseHDRsml64->CreateFromImages( &targets, nullptr );
	}

	// HDR AUTOEXPOSURE
	/*{
		idRenderDestination::targetList_t targets;
		targets.Append( lumaImage );
		renderDestLuminance = CreateRenderDestination( "_rd_luma" );
		renderDestLuminance->CreateFromImages( &targets, nullptr );
	}
	for( int i = 0; i < 2; i++ )
	{
		idRenderDestination::targetList_t targets;
		targets.Append( autoExposureImage[ i ] );
		renderDestAutoExposure[ i ] = CreateRenderDestination( va( "_rd_autoExp%i", i ) );
		renderDestAutoExposure[ i ]->CreateFromImages( &targets, nullptr );
	}*/

	// BLOOM RB
	for( int i = 0; i < MAX_BLOOM_BUFFERS; i++ )
	{
		idRenderDestination::targetList_t targets;
		targets.Append( bloomRenderImage[ i ] );
		renderDestBloomRender[ i ] = CreateRenderDestination( va( "_rd_bloomRender%i", i ) );
		renderDestBloomRender[ i ]->CreateFromImages( &targets, nullptr );
	}
	// BLOOM NATURAL
	for( int i = 0; i < MAX_BLOOM_BUFFER_MIPS; i++ )
	{
		idRenderDestination::targetList_t targets;
		targets.Append( bloomRenderMipsImage[ i ] );
		renderDestBloomMip[ i ] = CreateRenderDestination( va( "_rd_bloomMip%i", i ) );
		renderDestBloomMip[ i ]->CreateFromImages( &targets, nullptr );
	}

	// AMBIENT OCCLUSION
	for( int i = 0; i < MAX_SSAO_BUFFERS; i++ )
	{
		idRenderDestination::targetList_t targets;
		targets.Append( ambientOcclusionImage[ i ] );
		renderDestAmbientOcclusion[ i ] = CreateRenderDestination( va( "_rd_aoRender%i", i ) );
		renderDestAmbientOcclusion[ i ]->CreateFromImages( &targets, nullptr );
	}

	// HIERARCHICAL Z BUFFER
	for( int i = 0; i < MAX_HIERARCHICAL_ZBUFFERS; i++ )
	{
		idRenderDestination::targetList_t targets;
		targets.Append( hiZbufferImage, i );
		renderDestCSDepth[ i ] = CreateRenderDestination( va( "_rd_csz%i", i ) );
		renderDestCSDepth[ i ]->CreateFromImages( &targets, nullptr );
	}

	// SMAA
	{
		idRenderDestination::targetList_t targets;

		targets.Append( smaaEdgesImage );
		renderDestSMAAEdges = CreateRenderDestination( "_rd_smaaEdges" );
		renderDestSMAAEdges->CreateFromImages( &targets, viewDepthStencilImage );

		targets.Clear(); targets.Append( smaaBlendImage );
		renderDestSMAABlend = CreateRenderDestination( "_rd_smaaBlend" );
		renderDestSMAABlend->CreateFromImages( &targets, viewDepthStencilImage );
	}

	// Miscellaneous.
	// mostly for copying
	{
		idRenderDestination::targetList_t targets;
		targets.Append( renderImageManager->currentRenderImage );
		renderDestStagingColor = CreateRenderDestination( "_rd_stagingColor" );
		renderDestStagingColor->CreateFromImages( &targets, nullptr );

		renderDestStagingDepth = CreateRenderDestination( "_rd_stagingDepth" );
		renderDestStagingDepth->CreateFromImages( nullptr, viewDepthStencilImage );
	}

	GL_ResetRenderDestination();

	cmdSystem->AddCommand( "listRenderDest", ListRenderDestinations_f, CMD_FL_RENDERER, "lists render destinations" );

	bIsInitialized = true;
}

/*
=================================================
 Update
=================================================
*/
void idRenderDestinationManager::Update()
{
	GL_ResetRenderDestination();

	int screenWidth = renderSystem->GetWidth();
	int screenHeight = renderSystem->GetHeight();

	if( renderDestBaseHDR->GetTargetWidth() != screenWidth || renderDestBaseHDR->GetTargetHeight() != screenHeight )
	{
		// MAIN RENDER
		viewDepthStencilImage->Resize( screenWidth, screenHeight );
		viewColorHDRImage->Resize( screenWidth, screenHeight );
		viewColorLDRImage->Resize( screenWidth, screenHeight );
		viewColorsImage->Resize( screenWidth, screenHeight );
		viewNormalImage->Resize( screenWidth, screenHeight );

		{
			//renderDestDepthStencil->CreateFromImages( nullptr, renderImageManager->viewDepthStencilImage );

			//idRenderDestination::targetList_t targets;
			//targets.Append( renderImageManager->currentRenderImage );
			//renderDestRenderToTexture->CreateFromImages( &targets, renderImageManager->viewDepthStencilImage );
		}
		{
			idRenderDestination::targetList_t targets;
			targets.Append( viewColorHDRImage );
			renderDestBaseHDR->CreateFromImages( &targets, viewDepthStencilImage );
		}

		// HDR quarter
		viewColorHDRQuarterImage->Resize( screenWidth >> 2, screenHeight >> 2 );
		/*
		renderImageManager->currentRenderHDRImageQuarter->Resize( screenWidth / 4, screenHeight / 4 );

		globalFramebuffers.hdrQuarterFBO->Bind();
		globalFramebuffers.hdrQuarterFBO->AttachImage2D( GL_TEXTURE_2D, renderImageManager->currentRenderHDRImageQuarter, 0 );
		globalFramebuffers.hdrQuarterFBO->Check();
		*/

		// BLOOM

		for( int i = 0; i < MAX_BLOOM_BUFFERS; i++ )
		{
			bloomRenderImage[ i ]->Resize( screenWidth >> 2, screenHeight >> 2 );

			idRenderDestination::targetList_t targets;
			targets.Append( bloomRenderImage[ i ] );
			renderDestBloomRender[ i ]->CreateFromImages( &targets, nullptr );
		}

		//bloomRenderMipsImage->Resize( screenWidth, screenHeight );
		bloomRenderMipsImage[ 0 ]->Resize( screenWidth, screenHeight );
		bloomRenderMipsImage[ 1 ]->Resize( screenWidth / 2, screenHeight / 2 );
		bloomRenderMipsImage[ 2 ]->Resize( screenWidth / 4, screenHeight / 4 );
		bloomRenderMipsImage[ 3 ]->Resize( screenWidth / 8, screenHeight / 8 );
		bloomRenderMipsImage[ 4 ]->Resize( screenWidth / 16, screenHeight / 16 );
		bloomRenderMipsImage[ 5 ]->Resize( screenWidth / 32, screenHeight / 32 );

		for( int i = 0; i < MAX_BLOOM_BUFFER_MIPS; i++ )
		{
			idRenderDestination::targetList_t targets;
			targets.Append( bloomRenderMipsImage[ i ], 0 );
			renderDestBloomMip[ i ]->CreateFromImages( &targets, nullptr );
		}

		// AMBIENT OCCLUSION

		for( int i = 0; i < MAX_SSAO_BUFFERS; i++ )
		{
			ambientOcclusionImage[ i ]->Resize( screenWidth, screenHeight );

			idRenderDestination::targetList_t targets;
			targets.Append( ambientOcclusionImage[ i ] );
			renderDestAmbientOcclusion[ i ]->CreateFromImages( &targets, nullptr );
		}

		// HIERARCHICAL Z BUFFER

		hiZbufferImage->Resize( screenWidth, screenHeight );

		for( int i = 0; i < MAX_HIERARCHICAL_ZBUFFERS; i++ )
		{
			idRenderDestination::targetList_t targets;
			targets.Append( hiZbufferImage, i );
			renderDestCSDepth[ i ]->CreateFromImages( &targets, nullptr );
		}

		// GEOMETRY BUFFER
		{
			idRenderDestination::targetList_t targets;
			targets.Append( viewColorHDRImage );
			targets.Append( viewNormalImage );
			renderDestGBufferSml->CreateFromImages( &targets, viewDepthStencilImage );

			targets.Clear();
			targets.Append( viewColorsImage ); // rt0
			targets.Append( viewNormalImage ); // rt1
			renderDestGBuffer = CreateRenderDestination( "_rd_gbuffer" );
			renderDestGBuffer->CreateFromImages( &targets, viewDepthStencilImage );
		}

		// SMAA
		{
			idRenderDestination::targetList_t targets;

			smaaEdgesImage->Resize( screenWidth, screenHeight );

			targets.Append( smaaEdgesImage );
			renderDestSMAAEdges->CreateFromImages( &targets, nullptr );

			smaaBlendImage->Resize( screenWidth, screenHeight );

			targets.Clear(); targets.Append( smaaBlendImage );
			renderDestSMAABlend->CreateFromImages( &targets, nullptr );
		}
	}

	GL_ResetRenderDestination();
}

/*
=================================================
 Shutdown
=================================================
*/
void idRenderDestinationManager::Shutdown()
{
	//GL_ResetRenderDestination();

	renderDestList.DeleteContents( true );
	for( int i = 0; i < renderTargetImgList.Num(); ++i )
	{
		assert( renderTargetImgList[ i ] != nullptr );
		renderImageManager->DestroyStandaloneImage( renderTargetImgList[ i ] );
		renderTargetImgList[ i ] = nullptr;
	}
	renderTargetImgList.Clear();

	bIsInitialized = false;
}

/*
=================================================
 CreateRenderDestination
=================================================
*/
idRenderDestination * idRenderDestinationManager::CreateRenderDestination( const char* name )
{
	int nameLength = idStr::Length( name );
	if( nameLength >= idRenderDestination::MAX_RENDERDEST_NAME )
	{
		common->Error( "idRenderDestinationManager::CreateRenderDestination: \"%s\" is too long\n", name );
	}
	else if( nameLength < 1 )
	{
		common->Error( "idRenderDestinationManager::CreateRenderDestination: name is empty \"%s\"\n", name );
	}

	auto dest = FinedByName( name );
	if( !dest )
	{
		dest = new( TAG_RENDERDESTINATION ) idRenderDestination( name );
		renderDestList.Append( dest );
	}
	return dest;
}

/*
=================================================
 DestroyRenderDestination
=================================================
*/
void idRenderDestinationManager::DestroyRenderDestination( idRenderDestination * dest )
{
	int index = renderDestList.FindIndex( dest );
	assert( index != -1 );

	delete dest;

	//renderDestList[ index ] = nullptr;
	renderDestList.RemoveIndex( index );
}

/*
=================================================
 ListFramebuffers_f
=================================================
*/
void idRenderDestinationManager::ListRenderDestinations_f( const idCmdArgs& args )
{
	for( int i = 0; i < renderDestManager.renderDestList.Num(); i++ )
	{
		auto dest = renderDestManager.renderDestList[ i ];
		if( dest != nullptr )
		{
			common->Printf( S_COLOR_BLUE "RD%i:" S_COLOR_DEFAULT " %s w:%i, h:%i\n", i, dest->GetName(), dest->GetTargetWidth(), dest->GetTargetHeight() );
			dest->Print();
		}
		else
		{
			common->Printf( S_COLOR_BLUE "RD%i:" S_COLOR_DEFAULT " NULL\n", i );
		}
	}
}