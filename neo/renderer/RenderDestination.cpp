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
	renderDestList.Clear();
}

/*
=================================================
 Init
=================================================
*/
void idRenderDestinationManager::Init()
{
	// glTexImage2D( GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, 1680, 1050, 0, GL_RGB, GL_HALF_FLOAT, 0000000000000000 );
	{

	}

	// Init render destinations

	// SHADOWMAPS
	{
		renderDestShadowOnePass = CreateRenderDestination( "_rd_shadowBufferOnePass" );
		renderDestShadowOnePass->CreateFromImages( nullptr, renderImageManager->shadowMapArray, nullptr );

		renderDestShadow = CreateRenderDestination( "_rd_shadowBuffer" );
		renderDestShadow->CreateFromImages( nullptr, renderImageManager->shadowMapArray, nullptr );
	}

	// MAIN RENDER
	{
		idRenderDestination::targetList_t targets;
		targets.Append( renderImageManager->currentRenderHDRImage );
		renderDestBaseHDR = CreateRenderDestination( "_rd_baseHDRrender" );
		renderDestBaseHDR->CreateFromImages( &targets, renderImageManager->currentDepthImage, renderImageManager->currentDepthImage );
	}
	{
		//renderDestDepthStencil = CreateRenderDestination( "_rd_currentDepthStencil" );
		//renderDestDepthStencil->CreateFromImages( nullptr, renderImageManager->currentDepthImage, renderImageManager->currentDepthImage );

		// Will be used for CMD_SetBuffer
		//idRenderDestination::targetList_t targets;
		//targets.Append( renderImageManager->currentRenderImage );
		//renderDestRenderToTexture = CreateRenderDestination( "_rd_renderToTexture" );
		//renderDestRenderToTexture->CreateFromImages( &targets, renderImageManager->currentDepthImage, renderImageManager->currentDepthImage );

		///idRenderDestination::targetList_t targets;
		///targets.Append( renderImageManager->currentRenderImage );
		///renderDestCopyCurrentRender = CreateRenderDestination( "_rd_copyCurrentRender" );
		///renderDestDepthStencil->CreateFromImages( &targets, nullptr, nullptr );
	}

	// HDR DOWNSCALE
	{
		idRenderDestination::targetList_t targets;
		targets.Append( renderImageManager->currentRenderHDRImage64 );
		renderDestBaseHDRsml64 = CreateRenderDestination( "_rd_baseHDRsml64" );
		renderDestBaseHDRsml64->CreateFromImages( &targets, nullptr, nullptr );
	}

	// BLOOM
	for( int i = 0; i < MAX_BLOOM_BUFFERS; i++ )
	{
		idRenderDestination::targetList_t targets;
		targets.Append( renderImageManager->bloomRenderImage[ i ] );
		renderDestBloomRender[ i ] = CreateRenderDestination( va( "_rd_bloomRender%i", i ) );
		renderDestBloomRender[ i ]->CreateFromImages( &targets, nullptr, nullptr );
	}

	// AMBIENT OCCLUSION
	for( int i = 0; i < MAX_SSAO_BUFFERS; i++ )
	{
		idRenderDestination::targetList_t targets;
		targets.Append( renderImageManager->ambientOcclusionImage[ i ] );
		renderDestAmbientOcclusion[ i ] = CreateRenderDestination( va( "_rd_aoRender%i", i ) );
		renderDestAmbientOcclusion[ i ]->CreateFromImages( &targets, nullptr, nullptr );
	}
	// GBUFFER SMALL
	{
		idRenderDestination::targetList_t targets;
		targets.Append( renderImageManager->currentRenderHDRImage );
		targets.Append( renderImageManager->currentNormalsImage );
		renderDestGBufferSml = CreateRenderDestination( "_rd_gBufferSml" );
		renderDestGBufferSml->CreateFromImages( &targets, renderImageManager->currentDepthImage, renderImageManager->currentDepthImage );
	}

	// HIERARCHICAL Z BUFFER
	for( int i = 0; i < MAX_HIERARCHICAL_ZBUFFERS; i++ )
	{
		idRenderDestination::targetList_t targets;
		targets.Append( renderImageManager->hierarchicalZbufferImage, i );
		renderDestCSDepth[ i ] = CreateRenderDestination( va( "_rd_csz%i", i ) );
		renderDestCSDepth[ i ]->CreateFromImages( &targets, nullptr, nullptr );
	}

	// SMAA
	{
		idRenderDestination::targetList_t targets;

		targets.Append( renderImageManager->smaaEdgesImage );
		renderDestSMAAEdges = CreateRenderDestination( "_rd_smaaEdges" );
		renderDestSMAAEdges->CreateFromImages( &targets, nullptr, nullptr );

		targets.Clear(); targets.Append( renderImageManager->smaaBlendImage );
		renderDestSMAABlend = CreateRenderDestination( "_rd_smaaBlend" );
		renderDestSMAABlend->CreateFromImages( &targets, nullptr, nullptr );
	}

	backEnd.glState.currentFramebufferObject = NULL;

	cmdSystem->AddCommand( "listRenderDest", ListRenderDestinations_f, CMD_FL_RENDERER, "lists render destinations" );
}

/*
=================================================
 Update
=================================================
*/
void idRenderDestinationManager::Update()
{
	int screenWidth = renderSystem->GetWidth();
	int screenHeight = renderSystem->GetHeight();

	if( renderDestBaseHDR->GetTargetWidth() != screenWidth || renderDestBaseHDR->GetTargetHeight() != screenHeight )
	{
		// MAIN RENDER
		renderImageManager->currentRenderHDRImage->Resize( screenWidth, screenHeight );
		renderImageManager->currentDepthImage->Resize( screenWidth, screenHeight );
		renderImageManager->currentNormalsImage->Resize( screenWidth, screenHeight );

		{
			//renderDestDepthStencil->CreateFromImages( nullptr, renderImageManager->currentDepthImage, renderImageManager->currentDepthImage );

			//idRenderDestination::targetList_t targets;
			//targets.Append( renderImageManager->currentRenderImage );
			//renderDestRenderToTexture->CreateFromImages( &targets, renderImageManager->currentDepthImage, renderImageManager->currentDepthImage );
		}

	#if defined(USE_HDR_MSAA)
		if( glConfig.multisamples )
		{
			renderImageManager->currentRenderHDRImageNoMSAA->Resize( screenWidth, screenHeight );

			hdrNonMSAAFBO->Bind();
			hdrNonMSAAFBO->AttachImage2D( GL_TEXTURE_2D, renderImageManager->currentRenderHDRImageNoMSAA, 0 );
			hdrNonMSAAFBO->Check();

			hdrNonMSAAFBO->width = screenWidth;
			hdrNonMSAAFBO->height = screenHeight;

			hdrFBO->Bind();
			hdrFBO->AttachImage2D( GL_TEXTURE_2D_MULTISAMPLE, renderImageManager->currentRenderHDRImage, 0 );
			hdrFBO->AttachImageDepth( GL_TEXTURE_2D_MULTISAMPLE, renderImageManager->currentDepthImage );
			hdrFBO->Check();
		}
		else
		#endif
		{
			idRenderDestination::targetList_t targets;
			targets.Append( renderImageManager->currentRenderHDRImage );
			renderDestBaseHDR->CreateFromImages( &targets, renderImageManager->currentDepthImage, renderImageManager->currentDepthImage );
		}

		// HDR quarter
		/*
		renderImageManager->currentRenderHDRImageQuarter->Resize( screenWidth / 4, screenHeight / 4 );

		globalFramebuffers.hdrQuarterFBO->Bind();
		globalFramebuffers.hdrQuarterFBO->AttachImage2D( GL_TEXTURE_2D, renderImageManager->currentRenderHDRImageQuarter, 0 );
		globalFramebuffers.hdrQuarterFBO->Check();
		*/

		// BLOOM

		for( int i = 0; i < MAX_BLOOM_BUFFERS; i++ )
		{
			renderImageManager->bloomRenderImage[ i ]->Resize( screenWidth / 4, screenHeight / 4 );

			idRenderDestination::targetList_t targets;
			targets.Append( renderImageManager->bloomRenderImage[ i ] );
			renderDestBloomRender[ i ]->CreateFromImages( &targets, nullptr, nullptr );
		}

		// AMBIENT OCCLUSION

		for( int i = 0; i < MAX_SSAO_BUFFERS; i++ )
		{
			renderImageManager->ambientOcclusionImage[ i ]->Resize( screenWidth, screenHeight );

			idRenderDestination::targetList_t targets;
			targets.Append( renderImageManager->ambientOcclusionImage[ i ] );
			renderDestAmbientOcclusion[ i ]->CreateFromImages( &targets, nullptr, nullptr );
		}

		// HIERARCHICAL Z BUFFER

		renderImageManager->hierarchicalZbufferImage->Resize( screenWidth, screenHeight );

		for( int i = 0; i < MAX_HIERARCHICAL_ZBUFFERS; i++ )
		{
			idRenderDestination::targetList_t targets;
			targets.Append( renderImageManager->hierarchicalZbufferImage, i );
			renderDestCSDepth[ i ]->CreateFromImages( &targets, nullptr, nullptr );
		}

		// GEOMETRY BUFFER
		{
			idRenderDestination::targetList_t targets;
			targets.Append( renderImageManager->currentRenderHDRImage );
			targets.Append( renderImageManager->currentNormalsImage );
			renderDestGBufferSml->CreateFromImages( &targets, renderImageManager->currentDepthImage, renderImageManager->currentDepthImage );
		}

		// SMAA
		{
			idRenderDestination::targetList_t targets;

			renderImageManager->smaaEdgesImage->Resize( screenWidth, screenHeight );

			targets.Append( renderImageManager->smaaEdgesImage );
			renderDestSMAAEdges->CreateFromImages( &targets, nullptr, nullptr );

			renderImageManager->smaaBlendImage->Resize( screenWidth, screenHeight );

			targets.Clear(); targets.Append( renderImageManager->smaaBlendImage );
			renderDestSMAABlend->CreateFromImages( &targets, nullptr, nullptr );
		}
	}
}

/*
=================================================
 Shutdown
=================================================
*/
void idRenderDestinationManager::Shutdown()
{
	renderDestList.DeleteContents( true );
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

	auto dest = new( TAG_RENDERDESTINATION ) idRenderDestination( name );

	return renderDestList[ renderDestList.AddUnique( dest ) ];
}

/*
=================================================
 DestroyRenderDestination
=================================================
*/
void idRenderDestinationManager::DestroyRenderDestination( idRenderDestination * dest )
{
	int index = renderDestList.FindIndex( dest );
	release_assert( index != -1 );

	///dest->~idRenderDestination();
	delete dest;

	renderDestList[ index ] = nullptr;
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













//SEA TODO!!!

class idRenderClass {
public:
	static const int MAX_RENDERCLASS_NAME = 64;

	idRenderClass( const char *name )
	{
		int nameLength = idStr::Length( name );
		if( nameLength >= MAX_RENDERCLASS_NAME )
		{
			common->Error( "idRenderClass: \"%s\" is too long\n", name );
		}
		else if( nameLength < 1 )
		{
			common->Error( "idRenderClass: name is empty \"%s\"\n", name );
		}
		this->name = name;
	}
	~idRenderClass()
	{
		name.Clear();
	}
	const char * GetName() const
	{
		return name.c_str();
	}

	virtual void Print();

private:
	idStrStatic<MAX_RENDERCLASS_NAME>	name;
};