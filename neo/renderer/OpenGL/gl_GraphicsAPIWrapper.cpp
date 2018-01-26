/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2014 Robert Beckebans

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

#include "../tr_local.h"

idCVar r_useVertexAttribFormat( "r_useVertexAttribFormat", "0", CVAR_RENDERER | CVAR_BOOL, "use GL_ARB_vertex_attrib_binding extension" );

static ID_INLINE GLuint GetGLObject( void * apiObject )
{
#pragma warning( suppress: 4311 4302 )
	return reinterpret_cast<GLuint>( apiObject );
}

/*
====================
 GL_SelectTexture
====================
*/
void GL_SelectTexture( int unit )
{
	if( backEnd.glState.currenttmu == unit )
		return;
	
	if( unit < 0 || unit >= glConfig.maxTextureImageUnits )
	{
		common->Warning( "GL_SelectTexture: unit = %i", unit );
		return;
	}
	
	RENDERLOG_PRINT( "GL_SelectTexture( %i );\n", unit );
	
	backEnd.glState.currenttmu = unit;
}
/*
====================
 GL_GetCurrentTextureUnit
====================
*/
int GL_GetCurrentTextureUnit()
{
	return backEnd.glState.currenttmu;
}
/*
====================
 GL_ResetTexturesState
====================
*/
void GL_ResetTexturesState()
{
	RENDERLOG_OPEN_BLOCK("GL_ResetTexturesState()");
	/*for( int i = 0; i < MAX_MULTITEXTURE_UNITS - 1; i++ )
	{
		GL_SelectTexture( i );
		renderImageManager->BindNull();
	}*/
	renderImageManager->UnbindAll();

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

	const GLenum texUnit = backEnd.glState.currenttmu;
	const GLuint texnum = GetGLObject( img->GetAPIObject() );

	auto glBindTexObject = []( uint32 currentType, GLenum _unit, GLenum _target, GLuint _name )
	{
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
====================
 GL_BindTexture
====================
*/
void GL_BindTexture( int unit, idImage *img, idTextureSampler *sampler )
{
	GL_BindTexture( unit, img );

	glBindSampler( unit, sampler->oglSamplerState );
}

/*
====================
 GL_ResetProgramState
====================
*/
void GL_ResetProgramState()
{
	renderProgManager.Unbind();
}

/*
====================
 GL_Cull

	This handles the flipping needed when the view being
	rendered is a mirored view.
====================
*/
void GL_Cull( int cullType )
{
	if( backEnd.glState.faceCulling == cullType )
	{
		return;
	}
	
	if( cullType == CT_TWO_SIDED )
	{
		glDisable( GL_CULL_FACE );

		RENDERLOG_PRINT( "GL_Cull( Disable )\n" );
	}
	else
	{
		if( backEnd.glState.faceCulling == CT_TWO_SIDED )
		{
			glEnable( GL_CULL_FACE );
		}

		if( cullType == CT_BACK_SIDED )
		{
			glCullFace( backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK );

			RENDERLOG_PRINT( "GL_Cull( %s )\n", backEnd.viewDef->isMirror ? "GL_FRONT" : "GL_BACK" );
		}
		else {
			glCullFace( backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT );

			RENDERLOG_PRINT( "GL_Cull( %s )\n", backEnd.viewDef->isMirror ? "GL_BACK" : "GL_FRONT" );
		}
	}
	
	backEnd.glState.faceCulling = cullType;
}

/*
====================
GL_Scissor
====================
*/
void GL_Scissor( int x /* left*/, int y /* bottom */, int w, int h, int scissorIndex )
{
	glScissor( x, y, w, h );

	RENDERLOG_PRINT( "GL_Scissor( %i, %i, %i, %i )\n", x, y, w, h );
}

/*
====================
GL_Viewport
====================
*/
void GL_Viewport( int x /* left */, int y /* bottom */, int w, int h, int viewportIndex )
{
	glViewport( x, y, w, h );

	RENDERLOG_PRINT( "GL_Viewport( %i, %i, %i, %i )\n", x, y, w, h );
}

/*
====================
GL_PolygonOffset
====================
*/
void GL_PolygonOffset( float scale, float bias )
{
	backEnd.glState.polyOfsScale = scale;
	backEnd.glState.polyOfsBias = bias;
	if( backEnd.glState.glStateBits & GLS_POLYGON_OFFSET )
	{
		glPolygonOffset( scale, bias );

		RENDERLOG_PRINT( "GL_PolygonOffset( scale %f, bias %f )\n", scale, bias );
	}
}

/*
========================
GL_DepthBoundsTest
========================
*/
void GL_DepthBoundsTest( const float zmin, const float zmax )
{
	if( !glConfig.depthBoundsTestAvailable || zmin > zmax )
	{
		return;
	}
	
	if( zmin == 0.0f && zmax == 0.0f )
	{
		glDisable( GL_DEPTH_BOUNDS_TEST_EXT );

		RENDERLOG_PRINT( "GL_DepthBoundsTest( Disable )\n" );
	}
	else
	{
		glEnable( GL_DEPTH_BOUNDS_TEST_EXT );
		glDepthBoundsEXT( zmin, zmax );

		RENDERLOG_PRINT( "GL_DepthBoundsTest( zmin %f, zmax %f )\n", zmin, zmax );
	}
}

/*
========================
GL_StartDepthPass
========================
*/
void GL_StartDepthPass( const idScreenRect& rect )
{
}

/*
========================
GL_FinishDepthPass
========================
*/
void GL_FinishDepthPass()
{
}

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
====================
GL_Color
====================
*/
/*
void GL_Color( float* color )
{
	if( color == NULL )
	{
		return;
	}
	GL_Color( color[0], color[1], color[2], color[3] );
}
*/

// RB begin
void GL_Color( const idVec3& color )
{
	GL_Color( color[0], color[1], color[2], 1.0f );
}

void GL_Color( const idVec4& color )
{
	GL_Color( color[0], color[1], color[2], color[3] );
}
// RB end

/*
====================
GL_Color
====================
*/
void GL_Color( float r, float g, float b )
{
	GL_Color( r, g, b, 1.0f );
}

/*
====================
GL_Color
====================
*/
void GL_Color( float r, float g, float b, float a )
{
	float parm[4];
	parm[0] = idMath::ClampFloat( 0.0f, 1.0f, r );
	parm[1] = idMath::ClampFloat( 0.0f, 1.0f, g );
	parm[2] = idMath::ClampFloat( 0.0f, 1.0f, b );
	parm[3] = idMath::ClampFloat( 0.0f, 1.0f, a );
	renderProgManager.SetRenderParm( RENDERPARM_COLOR, parm );
}

/*
========================
GL_Clear
========================
*/
void GL_Clear( bool color, bool depth, bool stencil, byte stencilValue, float r, float g, float b, float a, bool clearHDR )
{
	int clearFlags = 0;
	if( color )
	{
		glClearColor( r, g, b, a );
		clearFlags |= GL_COLOR_BUFFER_BIT;
		RENDERLOG_PRINT( "GL_Clear( color %f,%f,%f,%f )\n", r, g, b, a );
	}
	if( depth )
	{
		clearFlags |= GL_DEPTH_BUFFER_BIT;
		RENDERLOG_PRINT( "GL_Clear( depth 1.0 )\n" );
	}
	if( stencil )
	{
		glClearStencil( stencilValue );
		clearFlags |= GL_STENCIL_BUFFER_BIT;
		RENDERLOG_PRINT( "GL_Clear( stencil %i )\n", stencilValue );
	}
	glClear( clearFlags );
	
	// RB begin
	if( r_useHDR.GetBool() && clearHDR && renderDestManager.renderDestBaseHDR != NULL )
	{
		GL_SetRenderDestination( renderDestManager.renderDestBaseHDR );
		
		glClear( clearFlags );
		RENDERLOG_PRINT( "---- GL_Clear( same values )\n" );
		
		if( GL_IsNativeFramebufferActive() )
		{
			GL_SetNativeRenderDestination();
		}
	}
	// RB end
}
void GL_ClearDepth( const float value )
{
	const GLfloat depthClearValue[ 1 ] = { value };
	glClearBufferfv( GL_DEPTH, 0, depthClearValue );

	RENDERLOG_PRINT( "GL_ClearDepth( %f )\n", depthClearValue[ 0 ] );
}
void GL_ClearDepthStencil( const float depthValue, const int stencilValue )
{
	glClearBufferfi( GL_DEPTH_STENCIL, 0, depthValue, stencilValue );

	RENDERLOG_PRINT( "GL_ClearDepthStencil( %f, %i )\n", depthValue, stencilValue );
}
void GL_ClearColor( const float r, const float g, const float b, const float a, const int iColorBuffer )
{
	const GLfloat colorClearValue[ 4 ] = { r,  g,  b,  a };
	glClearBufferfv( GL_COLOR, /*GL_DRAW_BUFFER0 +*/ iColorBuffer, colorClearValue );
	
	RENDERLOG_PRINT( "GL_ClearColor( target:%i, %f,%f,%f,%f )\n", iColorBuffer, colorClearValue[0], colorClearValue[1], colorClearValue[2], colorClearValue[3] );
}

/*
========================
 GL_SetRenderDestination
========================
*/
void GL_SetRenderDestination( const idRenderDestination * dest )
{
	const GLuint fbo = GetGLObject( dest->GetAPIObject() );

	if( backEnd.glState.currentFramebufferObject != fbo )
	{
		glBindFramebuffer( GL_FRAMEBUFFER, fbo );

		backEnd.glState.currentFramebufferObject = fbo;
		backEnd.glState.currentRenderDestination = dest;

		RENDERLOG_PRINT( "GL_SetRenderDestination( %s, %u )\n", dest->GetName(), fbo );
	}
}
/*
========================
 GL_SetNativeRenderDestination
========================
*/
void GL_SetNativeRenderDestination()
{
	if( backEnd.glState.currentFramebufferObject != GL_NONE )
	{
		glBindFramebuffer( GL_FRAMEBUFFER, GL_NONE );

		backEnd.glState.currentFramebufferObject = GL_NONE;
		backEnd.glState.currentRenderDestination = nullptr;

		RENDERLOG_PRINT( "GL_SetNativeRenderDestination()\n" );
	}
}
/*
========================
 GetCurrentRenderDestination
========================
*/
const idRenderDestination * GetCurrentRenderDestination()
{
	return backEnd.glState.currentRenderDestination;
}

/*
========================
 GL_IsNativeFramebufferActive
========================
*/
bool GL_IsNativeFramebufferActive()
{
	return( backEnd.glState.currentFramebufferObject == GL_NONE );
}
/*
========================
 GL_IsBound
========================
*/
bool GL_IsBound( const idRenderDestination * dest )
{
	GLuint fbo = GetGLObject( dest->GetAPIObject() );
	return( backEnd.glState.currentFramebufferObject == fbo );
}

/*
========================
 GL_BlitRenderBuffer
========================
*/
void GL_BlitRenderBuffer(
	const idRenderDestination * src, const idScreenRect & srcRect,
	const idRenderDestination * dst, const idScreenRect & dstRect,
	bool color, bool linearFiltering )
{
	const GLuint srcFBO = src ? GetGLObject( src->GetAPIObject() ) : GL_ZERO;
	const GLuint dstFBO = dst ? GetGLObject( dst->GetAPIObject() ) : GL_ZERO;

	if( GLEW_ARB_direct_state_access )
	{
		glBlitNamedFramebuffer( srcFBO, dstFBO,
			srcRect.x1, srcRect.y1, srcRect.x2, srcRect.y2,
			dstRect.x1, dstRect.y1, dstRect.x2, dstRect.y2,
			color ? GL_COLOR_BUFFER_BIT : GL_DEPTH_BUFFER_BIT,
			linearFiltering ? GL_LINEAR : GL_NEAREST );
	}
	else {
		glBindFramebuffer( GL_READ_FRAMEBUFFER, srcFBO );
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, dstFBO );

		backEnd.glState.currentFramebufferObject = MAX_UNSIGNED_TYPE( GLuint );

		glBlitFramebuffer(
			srcRect.x1, srcRect.y1, srcRect.x2, srcRect.y2,
			dstRect.x1, dstRect.y1, dstRect.x2, dstRect.y2,
			color ? GL_COLOR_BUFFER_BIT : GL_DEPTH_BUFFER_BIT,
			linearFiltering ? GL_LINEAR : GL_NEAREST );
	}

	RENDERLOG_OPEN_BLOCK("GL_Blit");
	RENDERLOG_PRINT( "src: %s, %u, rect:%i,%i,%i,%i\n", src ? src->GetName() : "NATIVE", srcFBO, srcRect.x1, srcRect.y1, srcRect.x2, srcRect.y2 );
	RENDERLOG_PRINT( "dst: %s, %u, rect:%i,%i,%i,%i\n", dst ? dst->GetName() : "NATIVE", dstFBO, dstRect.x1, dstRect.y1, dstRect.x2, dstRect.y2 );
	RENDERLOG_CLOSE_BLOCK();
}

/*
====================
 idImage::CopyFramebuffer	depricated!
====================
*/
void GL_CopyCurrentColorToTexture( idImage * img, int x, int y, int imageWidth, int imageHeight )
{
	release_assert( img != nullptr );
	release_assert( img->GetOpts().textureType == TT_2D );
	release_assert( !img->GetOpts().IsMultisampled() );
	release_assert( !img->GetOpts().IsArray() );

#if !defined( USE_GLES3 )
	if( GL_IsNativeFramebufferActive() )
	{
		glReadBuffer( GL_BACK );
	}
#endif
#if 0
	if( GLEW_NV_copy_image && !GL_IsNativeFramebufferActive() )
	{
		img->Resize( imageWidth, imageHeight, 1 );

		const GLuint srcName = GetGLObject( GetCurrentRenderDestination()->GetColorImage()->GetAPIObject() );
		const GLuint dstName = GetGLObject( img->GetAPIObject() );

		glCopyImageSubDataNV(
			srcName, GL_TEXTURE_2D, 0, x, y, 0,
			dstName, GL_TEXTURE_2D, 0, 0, 0, 0,
			imageWidth, imageHeight, 1 );

		GL_CheckErrors();

		backEnd.pc.c_copyFrameBuffer++;

		RENDERLOG_PRINT( "GL_CopyCurrentColorToTextureNV( dst: %s %ix%i, %i,%i,%i,%i )\n",
			img->GetName(), img->GetUploadWidth(), img->GetUploadHeight(), x, y, imageWidth, imageHeight );

		return;
	}
	else if( GLEW_ARB_copy_image && !GL_IsNativeFramebufferActive() )
	{
		img->Resize( imageWidth, imageHeight, 1 );

		const GLuint srcName = GetGLObject( GetCurrentRenderDestination()->GetColorImage()->GetAPIObject() );
		const GLuint dstName = GetGLObject( img->GetAPIObject() );

		glCopyImageSubData(
			srcName, GL_TEXTURE_2D, 0, x, y, 0,
			dstName, GL_TEXTURE_2D, 0, 0, 0, 0,
			imageWidth, imageHeight, 1 );

		GL_CheckErrors();

		backEnd.pc.c_copyFrameBuffer++;

		RENDERLOG_PRINT( "GL_CopyCurrentColorToTextureARB( dst: %s %ix%i, %i,%i,%i,%i )\n",
			img->GetName(), img->GetUploadWidth(), img->GetUploadHeight(), x, y, imageWidth, imageHeight );

		return;
	}
#endif

	///glTextureObject_t tex;
	///tex.DeriveTargetInfo( this );
	GLuint texnum = GetGLObject( img->GetAPIObject() );
	glBindTexture( GL_TEXTURE_2D, texnum );

	img->Resize( imageWidth, imageHeight, 1 );

	if( r_useHDR.GetBool() && GL_IsBound( renderDestManager.renderDestBaseHDR ) )
	{
		//if( backEnd.glState.currentFramebuffer != NULL && backEnd.glState.currentFramebuffer->IsMultiSampled() )

	#if defined( USE_HDR_MSAA )
		if( globalFramebuffers.hdrFBO->IsMultiSampled() )
		{
			glBindFramebuffer( GL_READ_FRAMEBUFFER, globalFramebuffers.hdrFBO->GetFramebuffer() );
			glBindFramebuffer( GL_DRAW_FRAMEBUFFER, globalFramebuffers.hdrNonMSAAFBO->GetFramebuffer() );
			glBlitFramebuffer( 0, 0, glConfig.nativeScreenWidth, glConfig.nativeScreenHeight,
				0, 0, glConfig.nativeScreenWidth, glConfig.nativeScreenHeight,
				GL_COLOR_BUFFER_BIT, GL_LINEAR );

			globalFramebuffers.hdrNonMSAAFBO->Bind();

			glCopyTexImage2D( tex.target, 0, GL_RGBA16F, x, y, imageWidth, imageHeight, 0 );

			globalFramebuffers.hdrFBO->Bind();
		}
		else
		#endif
		{
			if( GLEW_ARB_texture_storage ) {
				glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, x, y, imageWidth, imageHeight );
			} else {
				glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA16F, x, y, imageWidth, imageHeight, 0 );
			}
		}
	}
	else {
		if( GLEW_ARB_texture_storage )
		{
			glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, x, y, imageWidth, imageHeight );
		}
		else {
			glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, x, y, imageWidth, imageHeight, 0 );
		}
	}

	GL_CheckErrors();

	// these shouldn't be necessary if the image was initialized properly
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	GL_CheckErrors();

	backEnd.pc.c_copyFrameBuffer++;

	RENDERLOG_PRINT( "GL_CopyCurrentColorToTexture( dst: %s %ix%i, %i,%i,%i,%i )\n",
		img->GetName(), img->GetUploadWidth(), img->GetUploadHeight(), x, y, imageWidth, imageHeight );
}

/*
====================
 idImage::CopyDepthbuffer	depricated!
====================
*/
void GL_CopyCurrentDepthToTexture( idImage *img, int x, int y, int newImageWidth, int newImageHeight )
{
	release_assert( img != nullptr );
	release_assert( img->GetOpts().textureType == TT_2D );
	release_assert( !img->GetOpts().IsMultisampled() );
	release_assert( !img->GetOpts().IsArray() );
	///glTextureObject_t tex;
	///tex.DeriveTargetInfo( this );

	if( img->Resize( newImageWidth, newImageHeight, 1 ) ) // makes bind call if resized
	{
		// x, y Specify the window coordinates of the lower left corner of the rectangular region of pixels to be copied.

		if( GLEW_ARB_texture_storage )
		{
			glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, x, y, newImageWidth, newImageHeight );
		}
		else
		{
			glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_STENCIL, x, y, newImageWidth, newImageHeight, 0 );
		}
	}
	else
	{
		GLuint texnum = GetGLObject( img->GetAPIObject() );

		if( GLEW_EXT_direct_state_access )
		{
			if( GLEW_ARB_texture_storage )
			{
				glCopyTextureSubImage2DEXT( texnum, GL_TEXTURE_2D, 0, 0, 0, x, y, newImageWidth, newImageHeight );
			}
			else
			{
				glCopyTextureImage2DEXT( texnum, GL_TEXTURE_2D, 0, GL_DEPTH_STENCIL, x, y, newImageWidth, newImageHeight, 0 );
			}
		}
		else
		{
			glBindTexture( GL_TEXTURE_2D, texnum );

			if( GLEW_ARB_texture_storage )
			{
				glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, x, y, newImageWidth, newImageHeight );
			}
			else
			{
				glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_STENCIL, x, y, newImageWidth, newImageHeight, 0 );
			}
		}
	}

	GL_CheckErrors();

	backEnd.pc.c_copyFrameBuffer++;

	RENDERLOG_PRINT("GL_CopyCurrentDepthToTexture( dst: %s %ix%i, %i,%i,%i,%i )\n", 
		img->GetName(), img->GetUploadWidth(), img->GetUploadHeight(), x, y, newImageWidth, newImageHeight );
}

/*
========================
 GL_DisableRasterizer
========================
*/
void GL_DisableRasterizer()
{
	glEnable( GL_RASTERIZER_DISCARD );
}

/*
========================
 GL_SetDefaultState

	This should initialize all GL state that any part of the entire program
	may touch, including the editor.
========================
*/
void GL_SetDefaultState()
{
	RENDERLOG_PRINT( "--- GL_SetDefaultState ---\n" );

	//glClipControl( GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE );
	//glClipControl( GL_LOWER_LEFT, GL_ZERO_TO_ONE );
	
	glClearDepth( 1.0f );
	
	// make sure our GL state vector is set correctly
	memset( &backEnd.glState, 0, sizeof( backEnd.glState ) );
	GL_State( 0, true );
	
	backEnd.glState.currentFramebufferObject = MAX_UNSIGNED_TYPE( GLuint );
	backEnd.glState.currentRenderDestination = nullptr;
	GL_SetNativeRenderDestination();
	
	// These are changed by GL_Cull
	glCullFace( GL_FRONT_AND_BACK );
	glEnable( GL_CULL_FACE );
	
	// These are changed by GL_State
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
	glBlendFunc( GL_ONE, GL_ZERO );
	glDepthMask( GL_TRUE );
	glDepthFunc( GL_LESS );
	glDisable( GL_STENCIL_TEST );
	glDisable( GL_POLYGON_OFFSET_FILL );
	glDisable( GL_POLYGON_OFFSET_LINE );
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	//glEnable( GL_SAMPLE_COVERAGE );
	//glEnable( GL_SAMPLE_MASK );
	//glEnable( GL_TEXTURE_CUBE_MAP_SEAMLESS );
	//glFrontFace( GL_CCW );
	//glBlendEquation( GL_FUNC_ADD );
	//glDisable( GL_SAMPLE_ALPHA_TO_COVERAGE );
	//glSampleMaski( 0, 0xffffffff );
	
	// These should never be changed
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glEnable( GL_SCISSOR_TEST );
	glDrawBuffer( GL_BACK );
	glReadBuffer( GL_BACK );
	
	if( r_useScissor.GetBool() )
	{
		glScissor( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() );
	}
	
	// RB: don't keep renderprogs that were enabled during level load
	renderProgManager.Unbind();
	// RB end
}

/*
====================
GL_State

This routine is responsible for setting the most commonly changed state
====================
*/
void GL_State( uint64 stateBits, bool forceGlState )
{
	uint64 diff = stateBits ^ backEnd.glState.glStateBits;
	
	if( !r_useStateCaching.GetBool() || forceGlState )
	{
		// make sure everything is set all the time, so we
		// can see if our delta checking is screwing up
		diff = 0xFFFFFFFFFFFFFFFF;
	}
	else if( diff == 0 )
	{
		return;
	}
	
	//
	// check depthFunc bits
	//
	if( diff & GLS_DEPTHFUNC_BITS )
	{
		switch( stateBits & GLS_DEPTHFUNC_BITS )
		{
			case GLS_DEPTHFUNC_EQUAL:
				glDepthFunc( GL_EQUAL );
				break;
			case GLS_DEPTHFUNC_ALWAYS:
				glDepthFunc( GL_ALWAYS );
				break;
			case GLS_DEPTHFUNC_LESS:
				glDepthFunc( GL_LEQUAL );
				break;
			case GLS_DEPTHFUNC_GREATER:
				glDepthFunc( GL_GEQUAL );
				break;
		}
	}
	
	//
	// check blend bits
	//
	if( diff & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
	{
		GLenum srcFactor = GL_ONE;
		GLenum dstFactor = GL_ZERO;
		
		switch( stateBits & GLS_SRCBLEND_BITS )
		{
			case GLS_SRCBLEND_ZERO:
				srcFactor = GL_ZERO;
				break;
			case GLS_SRCBLEND_ONE:
				srcFactor = GL_ONE;
				break;
			case GLS_SRCBLEND_DST_COLOR:
				srcFactor = GL_DST_COLOR;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
				srcFactor = GL_ONE_MINUS_DST_COLOR;
				break;
			case GLS_SRCBLEND_SRC_ALPHA:
				srcFactor = GL_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
				srcFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_DST_ALPHA:
				srcFactor = GL_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
				srcFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			default:
				assert( !"GL_State: invalid src blend state bits\n" );
				break;
		}
		
		switch( stateBits & GLS_DSTBLEND_BITS )
		{
			case GLS_DSTBLEND_ZERO:
				dstFactor = GL_ZERO;
				break;
			case GLS_DSTBLEND_ONE:
				dstFactor = GL_ONE;
				break;
			case GLS_DSTBLEND_SRC_COLOR:
				dstFactor = GL_SRC_COLOR;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
				dstFactor = GL_ONE_MINUS_SRC_COLOR;
				break;
			case GLS_DSTBLEND_SRC_ALPHA:
				dstFactor = GL_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
				dstFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_DST_ALPHA:
				dstFactor = GL_DST_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
				dstFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			default:
				assert( !"GL_State: invalid dst blend state bits\n" );
				break;
		}
		
		// Only actually update GL's blend func if blending is enabled.
		if( srcFactor == GL_ONE && dstFactor == GL_ZERO )
		{
			glDisable( GL_BLEND );
		}
		else
		{
			glEnable( GL_BLEND );
			glBlendFunc( srcFactor, dstFactor );
		}
	}
	
	//
	// check depthmask
	//
	if( diff & GLS_DEPTHMASK )
	{
		if( stateBits & GLS_DEPTHMASK )
		{
			glDepthMask( GL_FALSE );
		}
		else
		{
			glDepthMask( GL_TRUE );
		}
	}

	//
	// check stencil mask
	//
	// glStencilMask( 0x00 ); // effectively disables stencil writing.
	// glStencilMask( 0xFF ); // write stencil
	// 0xff is 11111111 in binary.That means GL can write to all 8 of the stencil bits.
	// 0x00 is 00000000 in binary, and GL is not allowed to write to any bits when this mask is used.
	/*
		The stencil mask never disables the stencil buffer completely, you'd actually have to call glDisable (GL_STENCIL_TEST) for that.
		It simply enables or disables writes to portions of it.

		On a final note, if you disable GL_STENCIL_TEST or GL_DEPTH_TEST that actually does two things:

		Disables the test
		Disables writing stencil / depth values
		So, if for some reason, you ever wanted to write a constant depth or stencil value and you assumed 
		that disabling the test would accomplish that -- it won't. Use GL_ALWAYS for the test function instead of disabling the test if that is your intention.
	*/
	
	//
	// check colormask
	//
	if( diff & ( GLS_REDMASK | GLS_GREENMASK | GLS_BLUEMASK | GLS_ALPHAMASK ) )
	{
		GLboolean r = ( stateBits & GLS_REDMASK ) ? GL_FALSE : GL_TRUE;
		GLboolean g = ( stateBits & GLS_GREENMASK ) ? GL_FALSE : GL_TRUE;
		GLboolean b = ( stateBits & GLS_BLUEMASK ) ? GL_FALSE : GL_TRUE;
		GLboolean a = ( stateBits & GLS_ALPHAMASK ) ? GL_FALSE : GL_TRUE;
		glColorMask( r, g, b, a );
	}
	
	//
	// fill/line mode
	//
	if( diff & GLS_POLYMODE_LINE )
	{
		if( stateBits & GLS_POLYMODE_LINE )
		{
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		else
		{
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}
	
	//
	// polygon offset
	//
	if( diff & GLS_POLYGON_OFFSET )
	{
		if( stateBits & GLS_POLYGON_OFFSET )
		{
			glPolygonOffset( backEnd.glState.polyOfsScale, backEnd.glState.polyOfsBias );
			glEnable( GL_POLYGON_OFFSET_FILL );
			glEnable( GL_POLYGON_OFFSET_LINE );
		}
		else
		{
			glDisable( GL_POLYGON_OFFSET_FILL );
			glDisable( GL_POLYGON_OFFSET_LINE );
		}
	}
	
#if !defined( USE_CORE_PROFILE )
	//
	// alpha test
	//
	if( diff & ( GLS_ALPHATEST_FUNC_BITS | GLS_ALPHATEST_FUNC_REF_BITS ) )
	{
		if( ( stateBits & GLS_ALPHATEST_FUNC_BITS ) != 0 )
		{
			glEnable( GL_ALPHA_TEST );
			
			GLenum func = GL_ALWAYS;
			switch( stateBits & GLS_ALPHATEST_FUNC_BITS )
			{
				case GLS_ALPHATEST_FUNC_LESS:
					func = GL_LESS;
					break;
				case GLS_ALPHATEST_FUNC_EQUAL:
					func = GL_EQUAL;
					break;
				case GLS_ALPHATEST_FUNC_GREATER:
					func = GL_GEQUAL;
					break;
				default:
					assert( false );
			}
			GLclampf ref = ( ( stateBits & GLS_ALPHATEST_FUNC_REF_BITS ) >> GLS_ALPHATEST_FUNC_REF_SHIFT ) / ( float )0xFF;
			glAlphaFunc( func, ref );
		}
		else
		{
			glDisable( GL_ALPHA_TEST );
		}
	}
#endif
	
	//
	// stencil
	//
	if( diff & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS ) )
	{
		if( ( stateBits & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS ) ) != 0 )
		{
			glEnable( GL_STENCIL_TEST );
		}
		else
		{
			glDisable( GL_STENCIL_TEST );
		}
	}
	if( diff & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_FUNC_REF_BITS | GLS_STENCIL_FUNC_MASK_BITS ) )
	{
		GLuint ref = GLuint( ( stateBits & GLS_STENCIL_FUNC_REF_BITS ) >> GLS_STENCIL_FUNC_REF_SHIFT );
		GLuint mask = GLuint( ( stateBits & GLS_STENCIL_FUNC_MASK_BITS ) >> GLS_STENCIL_FUNC_MASK_SHIFT );
		GLenum func = 0;
		
		switch( stateBits & GLS_STENCIL_FUNC_BITS )
		{
			case GLS_STENCIL_FUNC_NEVER:
				func = GL_NEVER;
				break;
			case GLS_STENCIL_FUNC_LESS:
				func = GL_LESS;
				break;
			case GLS_STENCIL_FUNC_EQUAL:
				func = GL_EQUAL;
				break;
			case GLS_STENCIL_FUNC_LEQUAL:
				func = GL_LEQUAL;
				break;
			case GLS_STENCIL_FUNC_GREATER:
				func = GL_GREATER;
				break;
			case GLS_STENCIL_FUNC_NOTEQUAL:
				func = GL_NOTEQUAL;
				break;
			case GLS_STENCIL_FUNC_GEQUAL:
				func = GL_GEQUAL;
				break;
			case GLS_STENCIL_FUNC_ALWAYS:
				func = GL_ALWAYS;
				break;
		}
		glStencilFunc( func, ref, mask );
	}
	if( diff & ( GLS_STENCIL_OP_FAIL_BITS | GLS_STENCIL_OP_ZFAIL_BITS | GLS_STENCIL_OP_PASS_BITS ) )
	{
		GLenum sFail = 0;
		GLenum zFail = 0;
		GLenum pass = 0;
		
		switch( stateBits & GLS_STENCIL_OP_FAIL_BITS )
		{
			case GLS_STENCIL_OP_FAIL_KEEP:
				sFail = GL_KEEP;
				break;
			case GLS_STENCIL_OP_FAIL_ZERO:
				sFail = GL_ZERO;
				break;
			case GLS_STENCIL_OP_FAIL_REPLACE:
				sFail = GL_REPLACE;
				break;
			case GLS_STENCIL_OP_FAIL_INCR:
				sFail = GL_INCR;
				break;
			case GLS_STENCIL_OP_FAIL_DECR:
				sFail = GL_DECR;
				break;
			case GLS_STENCIL_OP_FAIL_INVERT:
				sFail = GL_INVERT;
				break;
			case GLS_STENCIL_OP_FAIL_INCR_WRAP:
				sFail = GL_INCR_WRAP;
				break;
			case GLS_STENCIL_OP_FAIL_DECR_WRAP:
				sFail = GL_DECR_WRAP;
				break;
		}
		switch( stateBits & GLS_STENCIL_OP_ZFAIL_BITS )
		{
			case GLS_STENCIL_OP_ZFAIL_KEEP:
				zFail = GL_KEEP;
				break;
			case GLS_STENCIL_OP_ZFAIL_ZERO:
				zFail = GL_ZERO;
				break;
			case GLS_STENCIL_OP_ZFAIL_REPLACE:
				zFail = GL_REPLACE;
				break;
			case GLS_STENCIL_OP_ZFAIL_INCR:
				zFail = GL_INCR;
				break;
			case GLS_STENCIL_OP_ZFAIL_DECR:
				zFail = GL_DECR;
				break;
			case GLS_STENCIL_OP_ZFAIL_INVERT:
				zFail = GL_INVERT;
				break;
			case GLS_STENCIL_OP_ZFAIL_INCR_WRAP:
				zFail = GL_INCR_WRAP;
				break;
			case GLS_STENCIL_OP_ZFAIL_DECR_WRAP:
				zFail = GL_DECR_WRAP;
				break;
		}
		switch( stateBits & GLS_STENCIL_OP_PASS_BITS )
		{
			case GLS_STENCIL_OP_PASS_KEEP:
				pass = GL_KEEP;
				break;
			case GLS_STENCIL_OP_PASS_ZERO:
				pass = GL_ZERO;
				break;
			case GLS_STENCIL_OP_PASS_REPLACE:
				pass = GL_REPLACE;
				break;
			case GLS_STENCIL_OP_PASS_INCR:
				pass = GL_INCR;
				break;
			case GLS_STENCIL_OP_PASS_DECR:
				pass = GL_DECR;
				break;
			case GLS_STENCIL_OP_PASS_INVERT:
				pass = GL_INVERT;
				break;
			case GLS_STENCIL_OP_PASS_INCR_WRAP:
				pass = GL_INCR_WRAP;
				break;
			case GLS_STENCIL_OP_PASS_DECR_WRAP:
				pass = GL_DECR_WRAP;
				break;
		}
		glStencilOp( sFail, zFail, pass );
	}
	
	backEnd.glState.glStateBits = stateBits;
}

/*
=================
GL_GetCurrentState
=================
*/
uint64 GL_GetCurrentState()
{
	return backEnd.glState.glStateBits;
}

/*
========================
GL_GetCurrentStateMinusStencil
========================
*/
uint64 GL_GetCurrentStateMinusStencil()
{
	return GL_GetCurrentState() & ~( GLS_STENCIL_OP_BITS | GLS_STENCIL_FUNC_BITS | GLS_STENCIL_FUNC_REF_BITS | GLS_STENCIL_FUNC_MASK_BITS );
}

void GL_SetVertexMask( idBitFlags<vertexMask_t> vertexMask )
{
	vertexMask.HasFlag( VERTEX_MASK_POSITION )? glEnableVertexAttribArray( PC_ATTRIB_INDEX_POSITION ) : glDisableVertexAttribArray( PC_ATTRIB_INDEX_POSITION );
	vertexMask.HasFlag( VERTEX_MASK_ST )? glEnableVertexAttribArray( PC_ATTRIB_INDEX_ST ) : glDisableVertexAttribArray( PC_ATTRIB_INDEX_ST );
	vertexMask.HasFlag( VERTEX_MASK_NORMAL )? glEnableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL ) : glDisableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
	vertexMask.HasFlag( VERTEX_MASK_COLOR )? glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR ) : glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
	vertexMask.HasFlag( VERTEX_MASK_TANGENT )? glEnableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT ) : glDisableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );
	vertexMask.HasFlag( VERTEX_MASK_COLOR2 )? glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 ) : glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );

	if( vertexMask.HasFlag( VERTEX_MASK_POSITION ) ) {
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_POSITION );
		glVertexAttribPointer( PC_ATTRIB_INDEX_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof( idDrawVert ), idDrawVert::positionOffset );
	} else {
		glDisableVertexAttribArray( PC_ATTRIB_INDEX_POSITION );
	}

	if( vertexMask.HasFlag( VERTEX_MASK_ST ) ) {
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_ST );
		glVertexAttribPointer( PC_ATTRIB_INDEX_ST, 2, GL_HALF_FLOAT, GL_TRUE, sizeof( idDrawVert ), idDrawVert::texcoordOffset );
	} else {
		glDisableVertexAttribArray( PC_ATTRIB_INDEX_ST );
	}

	if( vertexMask.HasFlag( VERTEX_MASK_NORMAL ) ) {
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
		glVertexAttribPointer( PC_ATTRIB_INDEX_NORMAL, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), idDrawVert::normalOffset );
	} else {
		glDisableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
	}

	if( vertexMask.HasFlag( VERTEX_MASK_COLOR ) ) {
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
		glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), idDrawVert::colorOffset );
	} else {
		glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
	}

	if( vertexMask.HasFlag( VERTEX_MASK_TANGENT ) ) {
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );
		glVertexAttribPointer( PC_ATTRIB_INDEX_TANGENT, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), idDrawVert::tangentOffset );
	} else {
		glDisableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );
	}

	if( vertexMask.HasFlag( VERTEX_MASK_COLOR2 ) ) {
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
		glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), idDrawVert::color2Offset );
	} else {
		glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
	}
}

/*
========================
 GL_SetVertexLayout
	Use GL_OES_draw_elements_base_vertex for EGL3.0 backend
========================
*/
void GL_SetVertexLayout( vertexLayoutType_t vertexLayout )
{
	if( glConfig.ARB_vertex_attrib_binding && r_useVertexAttribFormat.GetBool() )
	{
		if( vertexLayout == LAYOUT_DRAW_VERT_FULL )
		{
			glEnableVertexAttribArray( PC_ATTRIB_INDEX_POSITION );
			glEnableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
			glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
			glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
			glEnableVertexAttribArray( PC_ATTRIB_INDEX_ST );
			glEnableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );

			glVertexAttribFormat( PC_ATTRIB_INDEX_POSITION, 3, GL_FLOAT, GL_FALSE, DRAWVERT_XYZ_OFFSET );
			glVertexAttribFormat( PC_ATTRIB_INDEX_NORMAL, 4, GL_UNSIGNED_BYTE, GL_TRUE, DRAWVERT_NORMAL_OFFSET );
			glVertexAttribFormat( PC_ATTRIB_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, DRAWVERT_COLOR_OFFSET );
			glVertexAttribFormat( PC_ATTRIB_INDEX_COLOR2, 4, GL_UNSIGNED_BYTE, GL_TRUE, DRAWVERT_COLOR2_OFFSET );
			glVertexAttribFormat( PC_ATTRIB_INDEX_ST, 2, GL_HALF_FLOAT, GL_TRUE, DRAWVERT_ST_OFFSET );
			glVertexAttribFormat( PC_ATTRIB_INDEX_TANGENT, 4, GL_UNSIGNED_BYTE, GL_TRUE, DRAWVERT_TANGENT_OFFSET );
		}
		else if( LAYOUT_DRAW_VERT_POSITION_ONLY )
		{
			glEnableVertexAttribArray( PC_ATTRIB_INDEX_POSITION );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_ST );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );

			glVertexAttribFormat( PC_ATTRIB_INDEX_POSITION, 3, GL_FLOAT, GL_FALSE, DRAWVERT_XYZ_OFFSET );
		}
		else if( LAYOUT_DRAW_VERT_POSITION_TEXCOORD )
		{
			glEnableVertexAttribArray( PC_ATTRIB_INDEX_POSITION );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
			glEnableVertexAttribArray( PC_ATTRIB_INDEX_ST );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );

			glVertexAttribFormat( PC_ATTRIB_INDEX_POSITION, 3, GL_FLOAT, GL_FALSE, DRAWVERT_XYZ_OFFSET );
			glVertexAttribFormat( PC_ATTRIB_INDEX_ST, 2, GL_HALF_FLOAT, GL_TRUE, DRAWVERT_ST_OFFSET );
		}
		else if( LAYOUT_DRAW_SHADOW_VERT_SKINNED )
		{
			glEnableVertexAttribArray( PC_ATTRIB_INDEX_POSITION );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
			glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
			glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_ST );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );

			glVertexAttribFormat( PC_ATTRIB_INDEX_POSITION, 4, GL_FLOAT, GL_FALSE, SHADOWVERTSKINNED_XYZW_OFFSET );
			glVertexAttribFormat( PC_ATTRIB_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, SHADOWVERTSKINNED_COLOR_OFFSET );
			glVertexAttribFormat( PC_ATTRIB_INDEX_COLOR2, 4, GL_UNSIGNED_BYTE, GL_TRUE, SHADOWVERTSKINNED_COLOR2_OFFSET );
		}
		else if( LAYOUT_DRAW_SHADOW_VERT )
		{
			glEnableVertexAttribArray( PC_ATTRIB_INDEX_POSITION );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_ST );
			glDisableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );

			glVertexAttribFormat( PC_ATTRIB_INDEX_POSITION, 4, GL_FLOAT, GL_FALSE, SHADOWVERT_XYZW_OFFSET );
		}
	} 
	else {
		switch( vertexLayout )
		{
			case LAYOUT_DRAW_VERT_POSITION_ONLY:
			{
				glEnableVertexAttribArray( PC_ATTRIB_INDEX_POSITION );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_ST );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );

				glVertexAttribPointer( PC_ATTRIB_INDEX_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof( idDrawVert ), idDrawVert::positionOffset );

				RENDERLOG_PRINT( "GL_SetVertexLayout( LAYOUT_DRAW_VERT_POSITION_ONLY )\n" );
			} break;

			case LAYOUT_DRAW_VERT_POSITION_TEXCOORD:
			{
				glEnableVertexAttribArray( PC_ATTRIB_INDEX_POSITION );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
				glEnableVertexAttribArray( PC_ATTRIB_INDEX_ST );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );

				glVertexAttribPointer( PC_ATTRIB_INDEX_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof( idDrawVert ), idDrawVert::positionOffset );
				glVertexAttribPointer( PC_ATTRIB_INDEX_ST, 2, GL_HALF_FLOAT, GL_TRUE, sizeof( idDrawVert ), idDrawVert::texcoordOffset );

				RENDERLOG_PRINT( "GL_SetVertexLayout( LAYOUT_DRAW_VERT_POSITION_TEXCOORD )\n" );
			} break;

			case LAYOUT_DRAW_VERT_FULL:
			{
				glEnableVertexAttribArray( PC_ATTRIB_INDEX_POSITION );
				glEnableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
				glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
				glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
				glEnableVertexAttribArray( PC_ATTRIB_INDEX_ST );
				glEnableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );

				glVertexAttribPointer( PC_ATTRIB_INDEX_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof( idDrawVert ), idDrawVert::positionOffset );
				glVertexAttribPointer( PC_ATTRIB_INDEX_NORMAL, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), idDrawVert::normalOffset );
				glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), idDrawVert::colorOffset );
				glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), idDrawVert::color2Offset );
				glVertexAttribPointer( PC_ATTRIB_INDEX_ST, 2, GL_HALF_FLOAT, GL_TRUE, sizeof( idDrawVert ), idDrawVert::texcoordOffset );
				glVertexAttribPointer( PC_ATTRIB_INDEX_TANGENT, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), idDrawVert::tangentOffset );

				RENDERLOG_PRINT( "GL_SetVertexLayout( LAYOUT_DRAW_VERT_FULL )\n" );
			} break;

			case LAYOUT_DRAW_SHADOW_VERT_SKINNED:
			{
				glEnableVertexAttribArray( PC_ATTRIB_INDEX_POSITION );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
				glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
				glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_ST );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );

				glVertexAttribPointer( PC_ATTRIB_INDEX_POSITION, 4, GL_FLOAT, GL_FALSE, sizeof( idShadowVertSkinned ), idShadowVertSkinned::xyzwOffset );
				glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idShadowVertSkinned ), idShadowVertSkinned::colorOffset );
				glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idShadowVertSkinned ), idShadowVertSkinned::color2Offset );

				RENDERLOG_PRINT( "GL_SetVertexLayout( LAYOUT_DRAW_SHADOW_VERT_SKINNED )\n" );
			} break;

			case LAYOUT_DRAW_SHADOW_VERT:
			{
				glEnableVertexAttribArray( PC_ATTRIB_INDEX_POSITION );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_ST );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );

				glVertexAttribPointer( PC_ATTRIB_INDEX_POSITION, 4, GL_FLOAT, GL_FALSE, sizeof( idShadowVert ), idShadowVert::xyzwOffset );

				RENDERLOG_PRINT("GL_SetVertexLayout( LAYOUT_DRAW_SHADOW_VERT )\n");
			} break;

			default:{
				release_assert("Unknown vertexLayoutType_t");
			} break;
		}
	}
}

/*
========================
========================
*/
static void GL_SetIndexBuffer( GLuint ibo )
{
	if( backEnd.glState.currentIndexBuffer != ( GLintptr )ibo || !r_useStateCaching.GetBool() )
	{
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo );
		backEnd.glState.currentIndexBuffer = ibo;
	}
}
/*
========================
========================
*/
static void GL_SetVertexArray( GLuint vbo, vertexLayoutType_t vertexLayout )
{
	if( ( backEnd.glState.vertexLayout != vertexLayout ) || ( backEnd.glState.currentVertexBuffer != ( GLintptr )vbo ) || !r_useStateCaching.GetBool() )
	{
		if( glConfig.ARB_vertex_attrib_binding && r_useVertexAttribFormat.GetBool() )
		{
			const GLintptr baseOffset = 0;
			const GLuint bindingIndex = 0;
			glBindVertexBuffer( bindingIndex, vbo, baseOffset, sizeof( idDrawVert ) );
		}
		else {
			glBindBuffer( GL_ARRAY_BUFFER, vbo );
		}
		backEnd.glState.currentVertexBuffer = ( GLintptr )vbo;

		GL_SetVertexLayout( vertexLayout );
		backEnd.glState.vertexLayout = vertexLayout;
	}
}

/*
============================================================
 GL_DrawElementsWithCounters
============================================================
*/
void GL_DrawIndexed( const drawSurf_t* surf, vertexLayoutType_t vertexLayout, int globalInstanceCount )
{
	// get vertex buffer --------------------------------

	idVertexBuffer vertexBuffer;
	vertexCache.GetVertexBuffer( surf->vertexCache, &vertexBuffer );
	const GLint vertOffset = vertexBuffer.GetOffset();
	const GLuint vbo = GetGLObject( vertexBuffer.GetAPIObject() );

	// get index buffer --------------------------------

	idIndexBuffer indexBuffer;
	vertexCache.GetIndexBuffer( surf->indexCache, &indexBuffer );
	const GLintptr indexOffset = indexBuffer.GetOffset();
	const GLuint ibo = GetGLObject( indexBuffer.GetAPIObject() );

	// get joint buffer --------------------------------

	if( surf->jointCache )
	{
		// DG: this happens all the time in the erebus1 map with blendlight.vfp,
		// so don't call assert (through verify) here until it's fixed (if fixable)
		// else the game crashes on linux when using debug builds

		// FIXME: fix this properly if possible?
		// RB: yes but it would require an additional blend light skinned shader
		//if( !verify( renderProgManager.ShaderUsesJoints() ) )
		if( !renderProgManager.ShaderUsesJoints() )
			// DG end
		{
			return;
		}
	}
	else {
		if( !verify( !renderProgManager.ShaderUsesJoints() || renderProgManager.ShaderHasOptionalSkinning() ) )
		{
			return;
		}
	}

	if( surf->jointCache )
	{
		idJointBuffer jointBuffer;
		if( !vertexCache.GetJointBuffer( surf->jointCache, &jointBuffer ) )
		{
			idLib::Warning( "GL_DrawIndexed, jointBuffer == NULL" );
			return;
		}
		assert( ( jointBuffer.GetOffset() & ( glConfig.uniformBufferOffsetAlignment - 1 ) ) == 0 );

		// RB: 64 bit fixes, changed GLuint to GLintptr
		const GLintptr ubo = GetGLObject( jointBuffer.GetAPIObject() );
		// RB end

		glBindBufferRange( GL_UNIFORM_BUFFER, BINDING_MATRICES_UBO, ubo, jointBuffer.GetOffset(), jointBuffer.GetNumJoints() * sizeof( idJointMat ) );
	}

	renderProgManager.CommitUniforms();

	GL_SetIndexBuffer( ibo );
	GL_SetVertexArray( vbo, vertexLayout );

	const GLsizei primcount = globalInstanceCount;
	glDrawElementsInstancedBaseVertex( GL_TRIANGLES,
		r_singleTriangle.GetBool() ? 3 : surf->numIndexes,
		GL_INDEX_TYPE,
		( triIndex_t* )indexOffset,
		primcount,
		vertOffset / sizeof( idDrawVert ) );

	backEnd.pc.c_drawElements++;
	backEnd.pc.c_drawIndexes += surf->numIndexes;

	RENDERLOG_PRINT( "GL_DrawIndexed( VBO %u:%i, IBO %u:%i, numIndexes:%i )\n", vbo, vertOffset, ibo, indexOffset, surf->numIndexes );
}
/*
============================================================
 GL_DrawZeroOneCube
============================================================
*/
void GL_DrawZeroOneCube( vertexLayoutType_t vertexLayout, int instanceCount )
{
	const drawSurf_t* const surf = &backEnd.zeroOneCubeSurface;

	renderProgManager.CommitUniforms();

	// get vertex buffer --------------------------------

	idVertexBuffer vertexBuffer;
	vertexCache.GetVertexBuffer( surf->vertexCache, &vertexBuffer );
	const GLint vertOffset = vertexBuffer.GetOffset();
	const GLuint vbo = GetGLObject( vertexBuffer.GetAPIObject() );

	// get index buffer --------------------------------

	idIndexBuffer indexBuffer;
	vertexCache.GetIndexBuffer( surf->indexCache, &indexBuffer );
	const GLintptr indexOffset = indexBuffer.GetOffset();
	const GLuint ibo = GetGLObject( indexBuffer.GetAPIObject() );

	GL_SetIndexBuffer( ibo );
	GL_SetVertexArray( vbo, vertexLayout );

	glDrawElementsInstancedBaseVertex( GL_TRIANGLES,
		r_singleTriangle.GetBool() ? 3 : surf->numIndexes,
		GL_INDEX_TYPE,
		( triIndex_t* )indexOffset,
		instanceCount,
		vertOffset / sizeof( idDrawVert ) );

	backEnd.pc.c_drawElements++;
	backEnd.pc.c_drawIndexes += surf->numIndexes;

	RENDERLOG_PRINT( "GL_DrawZeroOneCube( VBO %u:%i, IBO %u:%i, numIndexes:%i )\n", vbo, vertOffset, ibo, indexOffset, surf->numIndexes );
}
/*
============================================================
 GL_DrawZeroOneCube
============================================================
*/
void GL_DrawUnitSquare( vertexLayoutType_t vertexLayout, int instanceCount )
{
	const drawSurf_t* const surf = &backEnd.unitSquareSurface;

	renderProgManager.CommitUniforms();

	// get vertex buffer --------------------------------

	idVertexBuffer vertexBuffer;
	vertexCache.GetVertexBuffer( surf->vertexCache, &vertexBuffer );
	const GLint vertOffset = vertexBuffer.GetOffset();
	const GLuint vbo = GetGLObject( vertexBuffer.GetAPIObject() );

	// get index buffer --------------------------------

	idIndexBuffer indexBuffer;
	vertexCache.GetIndexBuffer( surf->indexCache, &indexBuffer );
	const GLintptr indexOffset = indexBuffer.GetOffset();
	const GLuint ibo = GetGLObject( indexBuffer.GetAPIObject() );

	GL_SetIndexBuffer( ibo );
	GL_SetVertexArray( vbo, vertexLayout );

	glDrawElementsInstancedBaseVertex( GL_TRIANGLES,
		r_singleTriangle.GetBool() ? 3 : surf->numIndexes,
		GL_INDEX_TYPE,
		( triIndex_t* )indexOffset,
		instanceCount,
		vertOffset / sizeof( idDrawVert ) );

	backEnd.pc.c_drawElements++;
	backEnd.pc.c_drawIndexes += surf->numIndexes;

	RENDERLOG_PRINT( "GL_DrawUnitSquare( VBO %u:%i, IBO %u:%i, numIndexes:%i )\n", vbo, vertOffset, ibo, indexOffset, surf->numIndexes );
}