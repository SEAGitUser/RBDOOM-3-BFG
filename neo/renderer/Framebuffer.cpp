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
#include "Framebuffer.h"

idList<Framebuffer*>	Framebuffer::framebuffers;

globalFramebuffers_t	globalFramebuffers;

static void R_ListFramebuffers_f( const idCmdArgs& args )
{
	if( !glConfig.framebufferObjectAvailable )
	{
		common->Printf( "GL_EXT_framebuffer_object is not available.\n" );
		return;
	}
}

Framebuffer::Framebuffer( const char* name, int w, int h )
{
	fboName = name;
	
	frameBuffer = 0;
	
	memset( colorBuffers, 0, sizeof( colorBuffers ) );
	colorFormat = 0;
	
	depthBuffer = 0;
	depthFormat = 0;
	
	stencilBuffer = 0;
	stencilFormat = 0;
	
	width = w;
	height = h;
	
	msaaSamples = false;
	
	glGenFramebuffers( 1, &frameBuffer );
	
	framebuffers.Append( this );
}

Framebuffer::~Framebuffer()
{
	glDeleteFramebuffers( 1, &frameBuffer );
}

void Framebuffer::Init()
{
	cmdSystem->AddCommand( "listFramebuffers", R_ListFramebuffers_f, CMD_FL_RENDERER, "lists framebuffers" );
	
	backEnd.glState.currentFramebuffer = NULL;
	
	// SHADOWMAPS
	
	int width, height;
	width = height = r_shadowMapImageSize.GetInteger();
	
	for( int i = 0; i < MAX_SHADOWMAP_RESOLUTIONS; i++ )
	{
		width = height = shadowMapResolutions[i];
		
		globalFramebuffers.shadowFBO[i] = new Framebuffer( va( "_shadowMap%i", i ) , width, height );
		globalFramebuffers.shadowFBO[i]->Bind();
		glDrawBuffers( 0, NULL );
	}
	
	// HDR

	int screenWidth = renderSystem->GetWidth();
	int screenHeight = renderSystem->GetHeight();
	
	globalFramebuffers.hdrFBO = new Framebuffer( "_hdr", screenWidth, screenHeight );
	globalFramebuffers.hdrFBO->Bind();
	
#if defined(USE_HDR_MSAA)
	if( glConfig.multisamples )
	{
		globalFramebuffers.hdrFBO->AttachImage2D( GL_TEXTURE_2D_MULTISAMPLE, globalImages->currentRenderHDRImage, 0 );
		globalFramebuffers.hdrFBO->AttachImageDepth( GL_TEXTURE_2D_MULTISAMPLE, globalImages->currentDepthImage );
	}
	else
#endif
	{
		globalFramebuffers.hdrFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->currentRenderHDRImage, 0 );
		globalFramebuffers.hdrFBO->AttachImageDepth( GL_TEXTURE_2D, globalImages->currentDepthImage );
	}
	
	globalFramebuffers.hdrFBO->Check();
	
	// HDR no MSAA
#if defined(USE_HDR_MSAA)
	globalFramebuffers.hdrNonMSAAFBO = new Framebuffer( "_hdrNoMSAA", screenWidth, screenHeight );
	globalFramebuffers.hdrNonMSAAFBO->Bind();
	globalFramebuffers.hdrNonMSAAFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->currentRenderHDRImageNoMSAA, 0 );	
	globalFramebuffers.hdrNonMSAAFBO->Check();
#endif
	
	// HDR DOWNSCALE
	
	globalFramebuffers.hdr64FBO = new Framebuffer( "_hdr64", 64, 64 );
	globalFramebuffers.hdr64FBO->Bind();
	globalFramebuffers.hdr64FBO->AttachImage2D( GL_TEXTURE_2D, globalImages->currentRenderHDRImage64, 0 );
	globalFramebuffers.hdr64FBO->Check();
	
	
	// BLOOM
	
	for( int i = 0; i < MAX_BLOOM_BUFFERS; i++ )
	{
		globalFramebuffers.bloomRenderFBO[i] = new Framebuffer( va( "_bloomRender%i", i ), screenWidth, screenHeight );
		globalFramebuffers.bloomRenderFBO[i]->Bind();
		globalFramebuffers.bloomRenderFBO[i]->AttachImage2D( GL_TEXTURE_2D, globalImages->bloomRenderImage[i], 0 );
		globalFramebuffers.bloomRenderFBO[i]->Check();
	}
	
	// AMBIENT OCCLUSION
	
	for( int i = 0; i < MAX_SSAO_BUFFERS; i++ )
	{
		globalFramebuffers.ambientOcclusionFBO[i] = new Framebuffer( va( "_aoRender%i", i ), screenWidth, screenHeight );
		globalFramebuffers.ambientOcclusionFBO[i]->Bind();
		globalFramebuffers.ambientOcclusionFBO[i]->AttachImage2D( GL_TEXTURE_2D, globalImages->ambientOcclusionImage[i], 0 );
		globalFramebuffers.ambientOcclusionFBO[i]->Check();
	}
	
	// HIERARCHICAL Z BUFFER
	
	for( int i = 0; i < MAX_HIERARCHICAL_ZBUFFERS; i++ )
	{
		globalFramebuffers.csDepthFBO[i] = new Framebuffer( va( "_csz%i", i ), screenWidth / ( 1 << i ), screenHeight / ( 1 << i ) );
		globalFramebuffers.csDepthFBO[i]->Bind();
		globalFramebuffers.csDepthFBO[i]->AttachImage2D( GL_TEXTURE_2D, globalImages->hierarchicalZbufferImage, 0, i );
		globalFramebuffers.csDepthFBO[i]->Check();
	}
	
	// GEOMETRY BUFFER
	
	//globalFramebuffers.geometryBufferFBO = new Framebuffer( "_gbuffer", screenWidth, screenHeight );
	//globalFramebuffers.geometryBufferFBO->Bind();
	//globalFramebuffers.geometryBufferFBO->AddColorBuffer( GL_RGBA8, 0 );
	//globalFramebuffers.geometryBufferFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->currentNormalsImage, 0 );
	//globalFramebuffers.geometryBufferFBO->Check();
	
	// SMAA
	
	globalFramebuffers.smaaEdgesFBO = new Framebuffer( "_smaaEdges", screenWidth, screenHeight );
	globalFramebuffers.smaaEdgesFBO->Bind();
	globalFramebuffers.smaaEdgesFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->smaaEdgesImage, 0 );
	globalFramebuffers.smaaEdgesFBO->Check();
	
	globalFramebuffers.smaaBlendFBO = new Framebuffer( "_smaaBlend", screenWidth, screenHeight );
	globalFramebuffers.smaaBlendFBO->Bind();
	globalFramebuffers.smaaBlendFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->smaaBlendImage, 0 );
	globalFramebuffers.smaaBlendFBO->Check();
	
	Unbind();
}

void Framebuffer::CheckFramebuffers()
{
	int screenWidth = renderSystem->GetWidth();
	int screenHeight = renderSystem->GetHeight();

	if( globalFramebuffers.hdrFBO->GetWidth() != screenWidth || globalFramebuffers.hdrFBO->GetHeight() != screenHeight )
	{
		Unbind();
		
		// HDR
		globalImages->currentRenderHDRImage->Resize( screenWidth, screenHeight );
		globalImages->currentDepthImage->Resize( screenWidth, screenHeight );
		
#if defined(USE_HDR_MSAA)
		if( glConfig.multisamples )
		{
			globalImages->currentRenderHDRImageNoMSAA->Resize( screenWidth, screenHeight );
			
			globalFramebuffers.hdrNonMSAAFBO->Bind();
			globalFramebuffers.hdrNonMSAAFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->currentRenderHDRImageNoMSAA, 0 );
			globalFramebuffers.hdrNonMSAAFBO->Check();

			globalFramebuffers.hdrNonMSAAFBO->width = screenWidth;
			globalFramebuffers.hdrNonMSAAFBO->height = screenHeight;
			
			globalFramebuffers.hdrFBO->Bind();
			globalFramebuffers.hdrFBO->AttachImage2D( GL_TEXTURE_2D_MULTISAMPLE, globalImages->currentRenderHDRImage, 0 );
			globalFramebuffers.hdrFBO->AttachImageDepth( GL_TEXTURE_2D_MULTISAMPLE, globalImages->currentDepthImage );
			globalFramebuffers.hdrFBO->Check();
		}
		else
#endif
		{
			globalFramebuffers.hdrFBO->Bind();
			globalFramebuffers.hdrFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->currentRenderHDRImage, 0 );
			globalFramebuffers.hdrFBO->AttachImageDepth( GL_TEXTURE_2D, globalImages->currentDepthImage );
			globalFramebuffers.hdrFBO->Check();
		}
		
		globalFramebuffers.hdrFBO->width = screenWidth;
		globalFramebuffers.hdrFBO->height = screenHeight;
		
		// HDR quarter
		/*
		globalImages->currentRenderHDRImageQuarter->Resize( screenWidth / 4, screenHeight / 4 );
		
		globalFramebuffers.hdrQuarterFBO->Bind();
		globalFramebuffers.hdrQuarterFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->currentRenderHDRImageQuarter, 0 );
		globalFramebuffers.hdrQuarterFBO->Check();
		*/
		
		// BLOOM
		
		for( int i = 0; i < MAX_BLOOM_BUFFERS; i++ )
		{
			globalImages->bloomRenderImage[i]->Resize( screenWidth / 4, screenHeight / 4 );
			
			globalFramebuffers.bloomRenderFBO[i]->width = screenWidth / 4;
			globalFramebuffers.bloomRenderFBO[i]->height = screenHeight / 4;
			
			globalFramebuffers.bloomRenderFBO[i]->Bind();
			globalFramebuffers.bloomRenderFBO[i]->AttachImage2D( GL_TEXTURE_2D, globalImages->bloomRenderImage[i], 0 );
			globalFramebuffers.bloomRenderFBO[i]->Check();
		}
		
		// AMBIENT OCCLUSION
		
		for( int i = 0; i < MAX_SSAO_BUFFERS; i++ )
		{
			globalImages->ambientOcclusionImage[i]->Resize( screenWidth, screenHeight );
			
			globalFramebuffers.ambientOcclusionFBO[i]->width = screenWidth;
			globalFramebuffers.ambientOcclusionFBO[i]->height = screenHeight;
			
			globalFramebuffers.ambientOcclusionFBO[i]->Bind();
			globalFramebuffers.ambientOcclusionFBO[i]->AttachImage2D( GL_TEXTURE_2D, globalImages->ambientOcclusionImage[i], 0 );
			globalFramebuffers.ambientOcclusionFBO[i]->Check();
		}
		
		// HIERARCHICAL Z BUFFER
		
		globalImages->hierarchicalZbufferImage->Resize( screenWidth, screenHeight );
		
		for( int i = 0; i < MAX_HIERARCHICAL_ZBUFFERS; i++ )
		{
			globalFramebuffers.csDepthFBO[i]->width = screenWidth / ( 1 << i );
			globalFramebuffers.csDepthFBO[i]->height = screenHeight / ( 1 << i );
			
			globalFramebuffers.csDepthFBO[i]->Bind();
			globalFramebuffers.csDepthFBO[i]->AttachImage2D( GL_TEXTURE_2D, globalImages->hierarchicalZbufferImage, 0, i );
			globalFramebuffers.csDepthFBO[i]->Check();
		}
		
		// GEOMETRY BUFFER
		
		//globalImages->currentNormalsImage->Resize( screenWidth, screenHeight );
		
		//globalFramebuffers.geometryBufferFBO->width = screenWidth;
		//globalFramebuffers.geometryBufferFBO->height = screenHeight;
		
		//globalFramebuffers.geometryBufferFBO->Bind();
		//globalFramebuffers.geometryBufferFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->currentNormalsImage, 0 );
		//globalFramebuffers.geometryBufferFBO->Check();
		
		// SMAA
		
		globalImages->smaaEdgesImage->Resize( screenWidth, screenHeight );
		
		globalFramebuffers.smaaEdgesFBO->width = screenWidth;
		globalFramebuffers.smaaEdgesFBO->height = screenHeight;
		
		globalFramebuffers.smaaEdgesFBO->Bind();
		globalFramebuffers.smaaEdgesFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->smaaEdgesImage, 0 );
		globalFramebuffers.smaaEdgesFBO->Check();
		
		globalImages->smaaBlendImage->Resize( screenWidth, screenHeight );
		
		globalFramebuffers.smaaBlendFBO->width = screenWidth;
		globalFramebuffers.smaaBlendFBO->height = screenHeight;
		
		globalFramebuffers.smaaBlendFBO->Bind();
		globalFramebuffers.smaaBlendFBO->AttachImage2D( GL_TEXTURE_2D, globalImages->smaaBlendImage, 0 );
		globalFramebuffers.smaaBlendFBO->Check();
		
		Unbind();
	}
}

void Framebuffer::Shutdown()
{
	framebuffers.DeleteContents( true );
}

void Framebuffer::Bind()
{
	RENDERLOG_PRINTF( "Framebuffer::Bind( %s )\n", fboName.c_str() );
	
	if( backEnd.glState.currentFramebuffer != this )
	{
		glBindFramebuffer( GL_FRAMEBUFFER, frameBuffer );
		backEnd.glState.currentFramebuffer = this;
	}
}

bool Framebuffer::IsBound()
{
	return ( backEnd.glState.currentFramebuffer == this );
}

void Framebuffer::Unbind()
{
	RENDERLOG_PRINTF( "Framebuffer::Unbind()\n" );
	
	//if(backEnd.glState.framebuffer != NULL)
	{
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
		backEnd.glState.currentFramebuffer = NULL;
	}
}

bool Framebuffer::IsDefaultFramebufferActive()
{
	return ( backEnd.glState.currentFramebuffer == NULL );
}

void Framebuffer::AttachImage2D( int target, const idImage* image, int index, int mipmapLod )
{
	if( ( target != GL_TEXTURE_2D ) && ( target != GL_TEXTURE_2D_MULTISAMPLE ) && ( target < GL_TEXTURE_CUBE_MAP_POSITIVE_X || target > GL_TEXTURE_CUBE_MAP_NEGATIVE_Z ) )
	{
		common->Warning( "Framebuffer::AttachImage2D( %s ): invalid target", fboName.c_str() );
		return;
	}
	
	if( index < 0 || index >= glConfig.maxColorAttachments )
	{
		common->Warning( "Framebuffer::AttachImage2D( %s ): bad index = %i", fboName.c_str(), index );
		return;
	}
	
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, target, image->texnum, mipmapLod );
}

void Framebuffer::AttachImageDepth( int target, const idImage* image )
{
	if( ( target != GL_TEXTURE_2D ) && ( target != GL_TEXTURE_2D_MULTISAMPLE ) )
	{
		common->Warning( "Framebuffer::AttachImageDepth( %s ): invalid target", fboName.c_str() );
		return;
	}
	
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, image->texnum, 0 );
}

void Framebuffer::AttachImageDepthLayer( const idImage* image, int layer )
{
	glFramebufferTextureLayer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, image->texnum, 0, layer );
}

void Framebuffer::Check()
{
	int prev;
	glGetIntegerv( GL_FRAMEBUFFER_BINDING, &prev );
	
	glBindFramebuffer( GL_FRAMEBUFFER, frameBuffer );
	
	int status = glCheckFramebufferStatus( GL_FRAMEBUFFER );
	if( status == GL_FRAMEBUFFER_COMPLETE )
	{
		glBindFramebuffer( GL_FRAMEBUFFER, prev );
		return;
	}
	
	// something went wrong
	switch( status )
	{
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, incomplete attachment", fboName.c_str() );
			break;
			
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, missing attachment", fboName.c_str() );
			break;
			
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, missing draw buffer", fboName.c_str() );
			break;
			
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, missing read buffer", fboName.c_str() );
			break;
			
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, missing layer targets", fboName.c_str() );
			break;
			
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, missing multisample", fboName.c_str() );
			break;
			
		case GL_FRAMEBUFFER_UNSUPPORTED:
			common->Error( "Framebuffer::Check( %s ): Unsupported framebuffer format", fboName.c_str() );
			break;
			
		default:
			common->Error( "Framebuffer::Check( %s ): Unknown error 0x%X", fboName.c_str(), status );
			break;
	};
	
	glBindFramebuffer( GL_FRAMEBUFFER, prev );
}
