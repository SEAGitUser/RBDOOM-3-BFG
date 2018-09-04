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

#include "gl_header.h"

#include "../tr_local.h"

idCVar r_useVertexAttribFormat( "r_useVertexAttribFormat", "0", CVAR_RENDERER | CVAR_BOOL, "use GL_ARB_vertex_attrib_binding extension" );

float _minss( float a, float b )
{
	// Branchless SSE min.
	_mm_store_ss( &a, _mm_min_ss( _mm_set_ss( a ), _mm_set_ss( b ) ) );
	return a;
}
float _maxss( float a, float b )
{
	// Branchless SSE max.
	_mm_store_ss( &a, _mm_max_ss( _mm_set_ss( a ), _mm_set_ss( b ) ) );
	return a;
}
float _clamp( float val, float minval, float maxval )
{
	// Branchless SSE clamp.
	// return minss( maxss(val,minval), maxval );

	_mm_store_ss( &val, _mm_min_ss( _mm_max_ss( _mm_set_ss( val ), _mm_set_ss( minval ) ), _mm_set_ss( maxval ) ) );
	return val;
}

/*
====================
 GL_BeginRenderView
====================
*/
void GL_BeginRenderView( const idRenderView * view )
{
	glGetIntegerv( GL_FRONT_FACE, &backEnd.glState.initial_facing );
	glFrontFace( view->isMirror ? GL_CCW : GL_CW );
	RENDERLOG_PRINT( "FrontFace( %s )\n", view->isMirror ? "CCW" : "CW" );
}
/*
====================
 GL_EndRenderView
====================
*/
void GL_EndRenderView()
{
	glFrontFace( backEnd.glState.initial_facing );
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
			} else
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
			else {
				glBindTexObject( target_2DMS, texUnit, GL_TEXTURE_2D_MULTISAMPLE, texnum );
			}
		}
		else {
			if( img->GetOpts().IsArray() )
			{
				glBindTexObject( target_2DArray, texUnit, GL_TEXTURE_2D_ARRAY, texnum );
			}
			else {
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
		else {
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

#if 0
	if( GLEW_ARB_polygon_offset_clamp )
	{
		// This extension adds a new parameter to the polygon offset function
		// that clamps the calculated offset to a minimum or maximum value.
		// The clamping functionality is useful when polygons are nearly
		// parallel to the view direction because their high slopes can result
		// in arbitrarily large polygon offsets.In the particular case of
		// shadow mapping, the lack of clamping can produce the appearance of
		// unwanted holes when the shadow casting polygons are offset beyond
		// the shadow receiving polygons, and this problem can be alleviated by
		// enforcing a maximum offset value.
		glPolygonOffsetClamp( float factor, float units, float clamp );
	}
	else
	{
		GL_PolygonOffset
	}
#endif
}

/*
========================
GL_DepthBoundsTest
========================
*/
void GL_DepthBoundsTest( const float zmin, const float zmax )
{
	if( !glConfig.depthBoundsTestAvailable || zmin > zmax )
		return;

	if( zmin == 0.0f && zmax == 0.0f )
	{
		glDisable( GL_DEPTH_BOUNDS_TEST_EXT );

		RENDERLOG_PRINT( "GL_DepthBoundsTest( Disable )\n" );
	}
	else {
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
========================
GL_Clear
  The pixel ownership test, the scissor test, dithering, and the buffer writemasks affect the operation of glClear.
  The scissor box bounds the cleared region. Alpha function, blend function, logical operation, stenciling,
  texture mapping, and depth-buffering are ignored by glClear.
========================
*/
void GL_Clear( bool color, bool depth, bool stencil, byte stencilValue, float r, float g, float b, float a )
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
}
//
// The same per-fragment and masking operations defined for glClear are applied.
//
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
	GLfloat colorClearValue[ 4 ] = { r,  g,  b,  a };
	glClearBufferfv( GL_COLOR, /*GL_DRAW_BUFFER0 +*/ iColorBuffer, colorClearValue );

	RENDERLOG_PRINT( "GL_ClearColor( target:%i, %f,%f,%f,%f )\n", iColorBuffer, r, g, b, a );
}


/*
========================
 GL_SetRenderDestination
========================
*/
void GL_SetRenderDestination( const idRenderDestination * dest )
{
	assert( dest != nullptr );
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
GL_ResetRenderDestination
========================
*/
void GL_ResetRenderDestination()
{
	glBindFramebuffer( GL_FRAMEBUFFER, GL_NONE );

	backEnd.glState.currentFramebufferObject = GL_NONE;
	backEnd.glState.currentRenderDestination = nullptr;

	RENDERLOG_PRINT( "GL_ResetRenderDestination()\n" );
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
		GL_ResetRenderDestination();

		glDrawBuffer( GL_BACK );
		glReadBuffer( GL_BACK );

		RENDERLOG_PRINT( "GL_SetNativeRenderDestination()\n" );
	}
}
/*
========================
 GL_GetCurrentRenderDestination
========================
*/
const idRenderDestination * GL_GetCurrentRenderDestination()
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
	const idRenderDestination * src, int src_x, int src_y, int src_w, int src_h,
	const idRenderDestination * dst, int dst_x, int dst_y, int dst_w, int dst_h,
	bool color, bool linearFiltering )
{
	const GLuint srcFBO = src ? GetGLObject( src->GetAPIObject() ) : GL_ZERO;
	const GLuint dstFBO = dst ? GetGLObject( dst->GetAPIObject() ) : GL_ZERO;

	if( GLEW_ARB_direct_state_access )
	{
		glBlitNamedFramebuffer( srcFBO, dstFBO,
			src_x, src_y, src_w, src_h,
			dst_x, dst_y, dst_w, dst_h,
			color ? GL_COLOR_BUFFER_BIT : GL_DEPTH_BUFFER_BIT,
			linearFiltering ? GL_LINEAR : GL_NEAREST );
	}
	else {
		glBindFramebuffer( GL_READ_FRAMEBUFFER, srcFBO );
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, dstFBO );

		glBlitFramebuffer(
			src_x, src_y, src_w, src_h,
			dst_x, dst_y, dst_w, dst_h,
			color ? GL_COLOR_BUFFER_BIT : GL_DEPTH_BUFFER_BIT,
			linearFiltering ? GL_LINEAR : GL_NEAREST );

		GL_ResetRenderDestination();
	}

	RENDERLOG_OPEN_BLOCK("GL_Blit");
	RENDERLOG_PRINT( "src: %s, %u, rect:%i,%i,%i,%i\n", src ? src->GetName() : "NATIVE", srcFBO, src_x, src_y, src_w, src_h );
	RENDERLOG_PRINT( "dst: %s, %u, rect:%i,%i,%i,%i\n", dst ? dst->GetName() : "NATIVE", dstFBO, dst_x, dst_y, dst_w, dst_h );
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
	else {
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
		else {
			// gl will allocate memory for this new texture
			glCopyTexImage2D( GL_TEXTURE_2D, 0,
								r_useHDR.GetBool() ? GL_RGBA16F : GL_RGBA8,
								x, y, imageWidth, imageHeight, 0 );
		}
		GL_CheckErrors();
	}

	backEnd.pc.c_copyFrameBuffer++;

	RENDERLOG_PRINT( "GL_CopyCurrentColorToTexture( dst: %s %ix%i, %i,%i,%i,%i )\n",
		img->GetName(), img->GetUploadWidth(), img->GetUploadHeight(), x, y, imageWidth, imageHeight );
}

#if 0
/*
====================
 idImage::CopyDepthbuffer	depricated!
====================
*/
void GL_CopyCurrentDepthToTexture( idImage *img, int x, int y, int newImageWidth, int newImageHeight )
{
	assert( img != nullptr );
	assert( img->GetOpts().textureType == TT_2D );
	assert( !img->GetOpts().IsMultisampled() );
	assert( !img->GetOpts().IsArray() );
	///glTextureObject_t tex;
	///tex.DeriveTargetInfo( this );

	if( img->Resize( newImageWidth, newImageHeight, 1 ) ) // makes bind call if resized
	{
		// x, y Specify the window coordinates of the lower left corner of the rectangular region of pixels to be copied.

		if( GLEW_ARB_texture_storage )
		{
			glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, x, y, newImageWidth, newImageHeight );
		}
		else {
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
			else {
				glCopyTextureImage2DEXT( texnum, GL_TEXTURE_2D, 0, GL_DEPTH_STENCIL, x, y, newImageWidth, newImageHeight, 0 );
			}
		}
		else {
			glBindTexture( GL_TEXTURE_2D, texnum );

			if( GLEW_ARB_texture_storage )
			{
				glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, x, y, newImageWidth, newImageHeight );
			}
			else {
				glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_STENCIL, x, y, newImageWidth, newImageHeight, 0 );
			}
		}
	}

	GL_CheckErrors();

	backEnd.pc.c_copyFrameBuffer++;

	RENDERLOG_PRINT("GL_CopyCurrentDepthToTexture( dst: %s %ix%i, %i,%i,%i,%i )\n",
		img->GetName(), img->GetUploadWidth(), img->GetUploadHeight(), x, y, newImageWidth, newImageHeight );
}
#endif

/*
=====================================================================================================

										RENDER STATE

=====================================================================================================
*/

/*
========================
 GL_SetDefaultState

	This should initialize all GL state that any part of the entire program
	may touch, including the editor.
========================
*/
void GL_SetDefaultState()
{
	RENDERLOG_PRINT( "GL_SetDefaultState()\n" );

	// make sure our GL state vector is set correctly
	memset( &backEnd.glState, 0, sizeof( backEnd.glState ) );

	GL_SetRenderDestination( renderDestManager.renderDestBaseLDR );

	GL_State( GLS_DEFAULT, true );

	//glClipControl( GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE );
	//glClipControl( GL_LOWER_LEFT, GL_ZERO_TO_ONE );

	glClearDepth( 1.0f );

	// These are changed by GL_Cull
	//glCullFace( GL_FRONT_AND_BACK );
	//glEnable( GL_CULL_FACE );

	// These are changed by GL_State
	//glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
	//glBlendFunc( GL_ONE, GL_ZERO );
	//glDepthMask( GL_TRUE );
	//glDepthFunc( GL_LESS );
	//glDisable( GL_STENCIL_TEST );
	//glDisable( GL_POLYGON_OFFSET_FILL );
	//glDisable( GL_POLYGON_OFFSET_LINE );
	//glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	//glEnable( GL_SAMPLE_COVERAGE );
	//glEnable( GL_SAMPLE_MASK );
	//glEnable( GL_TEXTURE_CUBE_MAP_SEAMLESS );
	//glFrontFace( GL_CCW );
	glBlendEquation( GL_FUNC_ADD );
	//glDisable( GL_SAMPLE_ALPHA_TO_COVERAGE );
	//glSampleMaski( 0, 0xffffffff );

	// These should never be changed
	//glEnable( GL_DEPTH_TEST );
	//glEnable( GL_BLEND );
	glEnable( GL_SCISSOR_TEST );
	//glDrawBuffer( GL_BACK );
	//glReadBuffer( GL_BACK );

	//GL_Scissor( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() );

	GL_ResetProgramState();
	GL_ResetTextureState();

	/*for( uint32 i = 0; i < MAX_MULTITEXTURE_UNITS-1; i++ )
	{
		int oldTMU = backEnd.glState.currenttmu;
		for( int i = 0; i < MAX_PROG_TEXTURE_PARMS; ++i )
		{
			glBindSampler( i, GL_NONE );
			glBindMultiTextureEXT( GL_TEXTURE0 + i, GL_TEXTURE_2D, GL_NONE );

			backEnd.glState.currenttmu = i;
		}
		backEnd.glState.currenttmu = oldTMU;
	}*/
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

	if( forceGlState )
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
	// Check depthFunc bits:
	//
	if( diff & GLS_DEPTHFUNC_BITS )
	{
		switch( stateBits & GLS_DEPTHFUNC_BITS )
		{
			case GLS_DEPTHFUNC_LEQUAL:   glDepthFunc( GL_LEQUAL );	break;
			case GLS_DEPTHFUNC_ALWAYS:   glDepthFunc( GL_ALWAYS );	break;
			case GLS_DEPTHFUNC_GEQUAL:   glDepthFunc( GL_GEQUAL );	break;
			case GLS_DEPTHFUNC_EQUAL:    glDepthFunc( GL_EQUAL );	break;
			case GLS_DEPTHFUNC_NOTEQUAL: glDepthFunc( GL_NOTEQUAL ); break;
			case GLS_DEPTHFUNC_GREATER:  glDepthFunc( GL_GREATER ); break;
			case GLS_DEPTHFUNC_LESS:	 glDepthFunc( GL_LESS );	break;
			case GLS_DEPTHFUNC_NEVER:	 glDepthFunc( GL_NEVER );	break;
			default:
				assert( !"GL_State: invalid depthfunc state bits\n" );
				break;
		}
	}
	if( diff & GLS_DISABLE_DEPTHTEST ) //SEA
	{
		if( stateBits & GLS_DISABLE_DEPTHTEST )
		{
			glDisable( GL_DEPTH_TEST );
		}
		else {
			glEnable( GL_DEPTH_TEST );
		}
	}

	//
	// Check blend bits:
	//
	/*if( diff & GLS_BLENDOP_BITS )		//SEA: swf has some issue with main menu :(
	{
	switch( stateBits & GLS_BLENDOP_BITS )
	{
	case GLS_BLENDOP_ADD:		glBlendEquation( GL_FUNC_ADD );					break;
	case GLS_BLENDOP_SUB:		glBlendEquation( GL_FUNC_SUBTRACT );			break;
	case GLS_BLENDOP_MIN:		glBlendEquation( GL_MIN );						break;
	case GLS_BLENDOP_MAX:		glBlendEquation( GL_MAX );						break;
	case GLS_BLENDOP_INVSUB:	glBlendEquation( GL_FUNC_REVERSE_SUBTRACT );	break;
	default:
	assert( !"GL_State: invalid blendOp state bits\n" );
	break;
	}
	}*/
	if( diff & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
	{
		GLenum srcFactor = GL_ONE;
		GLenum dstFactor = GL_ZERO;

		switch( stateBits & GLS_SRCBLEND_BITS )
		{
			case GLS_SRCBLEND_ZERO:					srcFactor = GL_ZERO;				break;
			case GLS_SRCBLEND_ONE:					srcFactor = GL_ONE;					break;
			case GLS_SRCBLEND_DST_COLOR:			srcFactor = GL_DST_COLOR;			break;
			case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:	srcFactor = GL_ONE_MINUS_DST_COLOR; break;
			case GLS_SRCBLEND_SRC_ALPHA:			srcFactor = GL_SRC_ALPHA;			break;
			case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:	srcFactor = GL_ONE_MINUS_SRC_ALPHA; break;
			case GLS_SRCBLEND_DST_ALPHA:			srcFactor = GL_DST_ALPHA;			break;
			case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:	srcFactor = GL_ONE_MINUS_DST_ALPHA; break;
			default:
				assert( !"GL_State: invalid src blend state bits\n" );
				break;
		}

		switch( stateBits & GLS_DSTBLEND_BITS )
		{
			case GLS_DSTBLEND_ZERO:					dstFactor = GL_ZERO;				break;
			case GLS_DSTBLEND_ONE:					dstFactor = GL_ONE;					break;
			case GLS_DSTBLEND_SRC_COLOR:			dstFactor = GL_SRC_COLOR;			break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:	dstFactor = GL_ONE_MINUS_SRC_COLOR; break;
			case GLS_DSTBLEND_SRC_ALPHA:			dstFactor = GL_SRC_ALPHA;			break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:	dstFactor = GL_ONE_MINUS_SRC_ALPHA; break;
			case GLS_DSTBLEND_DST_ALPHA:			dstFactor = GL_DST_ALPHA;			break;
			case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:	dstFactor = GL_ONE_MINUS_DST_ALPHA; break;
			default:
				assert( !"GL_State: invalid dst blend state bits\n" );
				break;
		}

		// Only actually update GL's blend func if blending is enabled.
		if( srcFactor == GL_ONE && dstFactor == GL_ZERO )
		{
			glDisable( GL_BLEND );
		}
		else {
			glEnable( GL_BLEND );
			glBlendFunc( srcFactor, dstFactor );
		}
	}

	//
	// Check depthmask:
	//
	if( diff & GLS_DEPTHMASK )
	{
		if( stateBits & GLS_DEPTHMASK )
		{
			glDepthMask( GL_FALSE );
		}
		else {
			glDepthMask( GL_TRUE );
		}
	}

	//
	// Check stencil mask:
	//
	if( diff & GLS_STENCILMASK )  //SEA
	{
		if( stateBits & GLS_STENCILMASK )
		{
			glStencilMask( 0x00u );
		}
		else {
			glStencilMask( 0xFFu );
		}
	}
	//
	// Check stencil mask:
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
	// Check colormask:
	//
	if( diff & ( GLS_REDMASK | GLS_GREENMASK | GLS_BLUEMASK | GLS_ALPHAMASK ) )
	{
		GLboolean r = ( stateBits & GLS_REDMASK )? GL_FALSE : GL_TRUE;
		GLboolean g = ( stateBits & GLS_GREENMASK )? GL_FALSE : GL_TRUE;
		GLboolean b = ( stateBits & GLS_BLUEMASK )? GL_FALSE : GL_TRUE;
		GLboolean a = ( stateBits & GLS_ALPHAMASK )? GL_FALSE : GL_TRUE;
		glColorMask( r, g, b, a );
	}

	//
	// Fill/Line mode:
	//
	if( diff & GLS_POLYMODE_LINE )
	{
		glPolygonMode( GL_FRONT_AND_BACK, ( stateBits & GLS_POLYMODE_LINE )? GL_LINE : GL_FILL );
	}
	//
	// Polygon offset:
	//
	if( diff & GLS_POLYGON_OFFSET )
	{
		if( stateBits & GLS_POLYGON_OFFSET )
		{
			glPolygonOffset( backEnd.glState.polyOfsScale, backEnd.glState.polyOfsBias );

			//glEnable( GL_POLYGON_OFFSET_FILL );
			//glEnable( GL_POLYGON_OFFSET_LINE );

			if( stateBits & GLS_POLYMODE_LINE )
			{
				glDisable( GL_POLYGON_OFFSET_FILL );
				glEnable( GL_POLYGON_OFFSET_LINE );
			}
			else {
				glEnable( GL_POLYGON_OFFSET_FILL );
				glDisable( GL_POLYGON_OFFSET_LINE );
			}
		}
		else {
			glDisable( GL_POLYGON_OFFSET_FILL );
			glDisable( GL_POLYGON_OFFSET_LINE );
		}
	}
	//
	// Face culling:
	// This handles the flipping needed when the view being rendered is a mirored view.
	//
	if( diff & ( GLS_BACKSIDED | GLS_TWOSIDED ) )
	{
		/*/*/if( stateBits & GLS_TWOSIDED )
		{
			glDisable( GL_CULL_FACE );
			RENDERLOG_PRINT( "GL_Cull( Disable )\n" );
		}
		else if( stateBits & GLS_BACKSIDED )
		{
			glEnable( GL_CULL_FACE );
			//glCullFace( backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK );
			//RENDERLOG_PRINT( "GL_Cull( %s )\n", backEnd.viewDef->isMirror ? "GL_FRONT" : "GL_BACK" );
			glCullFace( GL_FRONT );
			RENDERLOG_PRINT( "GL_Cull( GL_FRONT )\n" );
		}
		else {
			glEnable( GL_CULL_FACE );
			//glCullFace( backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT );
			//RENDERLOG_PRINT( "GL_Cull( %s )\n", backEnd.viewDef->isMirror ? "GL_BACK" : "GL_FRONT" );
			glCullFace( GL_BACK );
			RENDERLOG_PRINT( "GL_Cull( GL_BACK )\n" );
		}
	}

/*#if !defined( USE_CORE_PROFILE )
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
#endif*/

	//
	// Stencil mode:
	//
	if( diff & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS ) )
	{
		if( ( stateBits & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS ) ) != 0 )
		{
			glEnable( GL_STENCIL_TEST );
		}
		else {
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
			case GLS_STENCIL_FUNC_NEVER:	func = GL_NEVER; break;
			case GLS_STENCIL_FUNC_LESS:		func = GL_LESS; break;
			case GLS_STENCIL_FUNC_EQUAL:	func = GL_EQUAL; break;
			case GLS_STENCIL_FUNC_LEQUAL:	func = GL_LEQUAL; break;
			case GLS_STENCIL_FUNC_GREATER:	func = GL_GREATER; break;
			case GLS_STENCIL_FUNC_NOTEQUAL: func = GL_NOTEQUAL; break;
			case GLS_STENCIL_FUNC_GEQUAL:	func = GL_GEQUAL; break;
			case GLS_STENCIL_FUNC_ALWAYS:	func = GL_ALWAYS; break;
		}
		glStencilFunc( func, ref, mask );
	}
	if( diff & ( GLS_STENCIL_OP_FAIL_BITS | GLS_STENCIL_OP_ZFAIL_BITS | GLS_STENCIL_OP_PASS_BITS ) )
	{
		GLenum sFail = 0;
		GLenum zFail = 0;
		GLenum zPass = 0;

		switch( stateBits & GLS_STENCIL_OP_FAIL_BITS )
		{
			case GLS_STENCIL_OP_FAIL_KEEP:		sFail = GL_KEEP; break;
			case GLS_STENCIL_OP_FAIL_ZERO:		sFail = GL_ZERO; break;
			case GLS_STENCIL_OP_FAIL_REPLACE:	sFail = GL_REPLACE; break;
			case GLS_STENCIL_OP_FAIL_INCR:		sFail = GL_INCR; break;
			case GLS_STENCIL_OP_FAIL_DECR:		sFail = GL_DECR; break;
			case GLS_STENCIL_OP_FAIL_INVERT:	sFail = GL_INVERT; break;
			case GLS_STENCIL_OP_FAIL_INCR_WRAP: sFail = GL_INCR_WRAP; break;
			case GLS_STENCIL_OP_FAIL_DECR_WRAP: sFail = GL_DECR_WRAP; break;
		}
		switch( stateBits & GLS_STENCIL_OP_ZFAIL_BITS )
		{
			case GLS_STENCIL_OP_ZFAIL_KEEP:		 zFail = GL_KEEP; break;
			case GLS_STENCIL_OP_ZFAIL_ZERO:		 zFail = GL_ZERO; break;
			case GLS_STENCIL_OP_ZFAIL_REPLACE:	 zFail = GL_REPLACE; break;
			case GLS_STENCIL_OP_ZFAIL_INCR:		 zFail = GL_INCR; break;
			case GLS_STENCIL_OP_ZFAIL_DECR:		 zFail = GL_DECR; break;
			case GLS_STENCIL_OP_ZFAIL_INVERT:	 zFail = GL_INVERT; break;
			case GLS_STENCIL_OP_ZFAIL_INCR_WRAP: zFail = GL_INCR_WRAP; break;
			case GLS_STENCIL_OP_ZFAIL_DECR_WRAP: zFail = GL_DECR_WRAP; break;
		}
		switch( stateBits & GLS_STENCIL_OP_PASS_BITS )
		{
			case GLS_STENCIL_OP_PASS_KEEP:		zPass = GL_KEEP; break;
			case GLS_STENCIL_OP_PASS_ZERO:		zPass = GL_ZERO; break;
			case GLS_STENCIL_OP_PASS_REPLACE:	zPass = GL_REPLACE; break;
			case GLS_STENCIL_OP_PASS_INCR:		zPass = GL_INCR; break;
			case GLS_STENCIL_OP_PASS_DECR:		zPass = GL_DECR; break;
			case GLS_STENCIL_OP_PASS_INVERT:	zPass = GL_INVERT; break;
			case GLS_STENCIL_OP_PASS_INCR_WRAP: zPass = GL_INCR_WRAP; break;
			case GLS_STENCIL_OP_PASS_DECR_WRAP: zPass = GL_DECR_WRAP; break;
		}
		glStencilOp( sFail, zFail, zPass );
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

/*
========================
GL_DisableRasterizer
========================
*/
void GL_DisableRasterizer()
{
	glEnable( GL_RASTERIZER_DISCARD );
	RENDERLOG_PRINT( "GL_DisableRasterizer()\n" );
}

/*
=====================================================================================================

											PROGRAM

=====================================================================================================
*/

/*
========================
 GL_SetRenderProgram
========================
*/
void GL_SetRenderProgram( const idDeclRenderProg * prog )
{
	if( backEnd.glState.currentRenderProg != prog )
	{
		auto glslprog = ( const glslProgram_t * ) prog->GetAPIObject();
		glUseProgram( glslprog->programObject );

		backEnd.glState.currentRenderProg = prog;
		backEnd.glState.currentProgramObject = glslprog->programObject;

		RENDERLOG_PRINT( "GL_SetRenderProgram( %s ) %s\n", prog->GetName(), prog->HasHardwareSkinning() ? ( prog->HasOptionalSkinning() ? "opt_skinned" : "skinned" ) : "" );
	}
}
/*
========================
 GL_GetCurrentRenderProgram
========================
*/
const idDeclRenderProg * GL_GetCurrentRenderProgram()
{
	return backEnd.glState.currentRenderProg;
}
/*
========================
 GL_ResetProgramState
========================
*/
void GL_ResetProgramState()
{
	glUseProgram( GL_NONE );

	backEnd.glState.currentProgramObject = GL_NONE;
	backEnd.glState.currentRenderProg = nullptr;

	RENDERLOG_PRINT("GL_ResetProgramState()\n");
}

/*
===========================================
 GL_CommitUniforms
===========================================
*/
void GL_CommitProgUniforms( const idDeclRenderProg * decl )
{
	///const char * SHT[ SHT_MAX ] = { "VS", "TCS", "TES", "GS", "FS" };
	auto const prog = ( const glslProgram_t * ) decl->GetAPIObject();

	/*{
	auto & parms = GetSmpParms();
	for( int i = 0; i < parms.Num(); ++i )
	{
	GLuint sampler;
	glBindSampler( i, sampler );
	}
	}*/
	{
		auto & parms = decl->GetTexParms();
		for( int i = 0; i < parms.Num(); ++i )
		{
			GL_BindTexture( i, parms[ i ]->GetImage() );
		}
	}

	if( r_useProgUBO.GetBool() && decl->GetVecParms().Num() )
	{
		const auto & ubo = backEnd.progParmsUniformBuffer; //GetParmsUBO();
		const GLuint bo = GetGLObject( ubo.GetAPIObject() );
		const GLuint bindIndex = BINDING_PROG_PARMS_UBO;

		idRenderVector localVectors[ 128 ];
		auto & parms = decl->GetVecParms();
		for( int i = 0; i < parms.Num(); ++i )
		{
		#if defined( USE_INTRINSICS )
			_mm_store_ps( localVectors[ i ].ToFloatPtr(), _mm_load_ps( parms[ i ]->GetVecPtr() ) );
		#else
			localVectors[ i ] = parms[ i ]->GetVec4();
		#endif
		}
		const int updateSize = parms.Num() * sizeof( idRenderVector );
		ubo.Update( updateSize, localVectors->ToFloatPtr() );

		//ubo.Bind( bindIndex, ubo.GetOffset(), updateSize );
		///glBindBufferBase( GL_UNIFORM_BUFFER, bindIndex, bo );

		RENDERLOG_PRINT( "UBO( binding:%u, count:%i, size:%i, offset:%i, bsize:%i )\n", bindIndex, parms.Num(), updateSize, ubo.GetOffset(), ubo.GetSize() );
	}
	else // Uniform Arrays
	{
		if( prog->parmBlockIndex != -1 )
		{
			idRenderVector localVectors[ 128 ];

			auto & parms = decl->GetVecParms();
			for( int i = 0; i < parms.Num(); ++i )
			{
				const float * v = parms[ i ]->GetVecPtr();
			#if defined( USE_INTRINSICS )
				_mm_store_ps( localVectors[ i ].ToFloatPtr(), _mm_load_ps( v ) );
			#else
				localVectors[ i ] = parms[ i ]->GetVec4();
			#endif
				RENDERLOG_PRINT( "_rp.%s = { %f, %f, %f, %f }\n", parms[ i ]->GetName(), v[ 0 ], v[ 1 ], v[ 2 ], v[ 3 ] );
			}
			glUniform4fv( prog->parmBlockIndex, parms.Num(), localVectors->ToFloatPtr() );

			//RENDERLOG_PRINT( "Uniforms( Binding:%i, Count:%i )\n", prog->parmBlockIndex, parms.Num() );
		}
	}
}

/*
=====================================================================================================

											VAO

=====================================================================================================
*/
/*
========================
========================
*/
void GL_SetVertexMask( vertexMask_t vertexMask )
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
	if( 0 )//glConfig.ARB_vertex_attrib_binding && r_useVertexAttribFormat.GetBool() )
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

				RENDERLOG_PRINT( "GL_SetVertexLayout( LAYOUT_DRAW_SHADOW_VERT )\n" );
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
	if( backEnd.glState.currentIndexBuffer != ( GLintptr )ibo )
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
	if( backEnd.glState.currentVAO != glConfig.global_vao )
	{
		glBindVertexArray( glConfig.global_vao );
		backEnd.glState.currentVAO = glConfig.global_vao;
	}

	if( ( backEnd.glState.vertexLayout != vertexLayout ) || ( backEnd.glState.currentVertexBuffer != ( GLintptr )vbo ) )
	{
		backEnd.glState.currentVertexBuffer = ( GLintptr )vbo;
		if( 0 )//glConfig.ARB_vertex_attrib_binding && r_useVertexAttribFormat.GetBool() )
		{
			const GLintptr baseOffset = 0;
			const GLuint bindingIndex = 0;
			glBindVertexBuffer( bindingIndex, vbo, baseOffset, sizeof( idDrawVert ) );
		}
		else {
			glBindBuffer( GL_ARRAY_BUFFER, vbo );
		}

		// -----------------------------------------

		backEnd.glState.vertexLayout = vertexLayout;
		GL_SetVertexLayout( vertexLayout );
	}
}

void GL_SetAttributeMask( idBitFlags<int> vertMask )
{
	if( vertMask.HasFlag( VERTEX_MASK_POSITION ) ) {
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_POSITION );
	} else {
		glDisableVertexAttribArray( PC_ATTRIB_INDEX_POSITION );
	}

	if( vertMask.HasFlag( VERTEX_MASK_ST ) ) {
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_ST );
	} else {
		glDisableVertexAttribArray( PC_ATTRIB_INDEX_ST );
	}

	if( vertMask.HasFlag( VERTEX_MASK_NORMAL ) ) {
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
	} else {
		glDisableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
	}

	if( vertMask.HasFlag( VERTEX_MASK_TANGENT ) ) {
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );
	} else {
		glDisableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );
	}

	if( vertMask.HasFlag( VERTEX_MASK_COLOR ) ) {
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
	} else {
		glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
	}

	if( vertMask.HasFlag( VERTEX_MASK_COLOR2 ) ) {
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
	} else {
		glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
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
	assert( surf->vertexCache != 0 );
	vertexCache.GetVertexBuffer( surf->vertexCache, vertexBuffer );
	const GLint vertOffset = vertexBuffer.GetOffset();
	const GLuint vbo = GetGLObject( vertexBuffer.GetAPIObject() );

	// get index buffer --------------------------------

	idIndexBuffer indexBuffer;
	assert( surf->indexCache != 0 );
	vertexCache.GetIndexBuffer( surf->indexCache, indexBuffer );
	const GLintptr indexOffset = indexBuffer.GetOffset();
	const GLuint ibo = GetGLObject( indexBuffer.GetAPIObject() );

	// get joint buffer --------------------------------

	/*if( surf->HasSkinning() )
	{
		// DG: this happens all the time in the erebus1 map with blendlight.vfp,
		// so don't call assert (through verify) here until it's fixed (if fixable)
		// else the game crashes on linux when using debug builds

		// FIXME: fix this properly if possible?
		// RB: yes but it would require an additional blend light skinned shader
		//if( !verify( GL_GetCurrentRenderProgram()->HasHardwareSkinning() ) )
		if( !GL_GetCurrentRenderProgram()->HasHardwareSkinning() )
			// DG end
		{
			return;
		}
	}
	else {
		if( !verify( !GL_GetCurrentRenderProgram()->HasHardwareSkinning() || GL_GetCurrentRenderProgram()->HasOptionalSkinning() ) )
		{
			return;
		}
	}*/

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

		assert( ( jointBuffer.GetOffset() & ( glConfig.uniformBufferOffsetAlignment - 1 ) ) == 0 );

		const GLintptr ubo = GetGLObject( jointBuffer.GetAPIObject() );

		glBindBufferRange( GL_UNIFORM_BUFFER, BINDING_MATRICES_UBO, ubo, jointBuffer.GetOffset(), jointBuffer.GetNumJoints() * sizeof( idJointMat ) );
	}

	GL_CommitProgUniforms( GL_GetCurrentRenderProgram() );

	GL_SetVertexArray( vbo, vertexLayout );
	GL_SetIndexBuffer( ibo );

	//SEA: wtf?
	//release_assert( NULL != ( triIndex_t* )indexOffset );
	//release_assert( NULL != ( vertOffset / sizeof( idDrawVert )) );

	const GLsizei primcount = globalInstanceCount;
	glDrawElementsInstancedBaseVertex( GL_TRIANGLES,
		r_singleTriangle.GetBool() ? 3 : surf->numIndexes,
		GL_INDEX_TYPE,
		( triIndex_t* )indexOffset,
		primcount,
		vertOffset / sizeof( idDrawVert ) );

	backEnd.pc.c_drawElements++;
	backEnd.pc.c_drawIndexes += surf->numIndexes;

	RENDERLOG_PRINT( ">>>>> GL_DrawIndexed( VBO:%u offset:%i, IBO:%u offset:%i count:%i )\n", vbo, vertOffset, ibo, indexOffset, surf->numIndexes );
}

/*
============================================================
 GL_DrawZeroOneCube
============================================================
*/
void GL_DrawZeroOneCube( vertexLayoutType_t vertexLayout, int instanceCount )
{
	const auto * const surf = &backEnd.zeroOneCubeSurface;

	GL_CommitProgUniforms( GL_GetCurrentRenderProgram() );

	// get vertex buffer --------------------------------

	idVertexBuffer vertexBuffer;
	vertexCache.GetVertexBuffer( surf->vertexCache, vertexBuffer );
	const GLint vertOffset = vertexBuffer.GetOffset();
	const GLuint vbo = GetGLObject( vertexBuffer.GetAPIObject() );

	// get index buffer --------------------------------

	idIndexBuffer indexBuffer;
	vertexCache.GetIndexBuffer( surf->indexCache, indexBuffer );
	const GLintptr indexOffset = indexBuffer.GetOffset();
	const GLuint ibo = GetGLObject( indexBuffer.GetAPIObject() );

	GL_SetVertexArray( vbo, vertexLayout );
	GL_SetIndexBuffer( ibo );

	glDrawElementsInstancedBaseVertex( GL_TRIANGLES,
		r_singleTriangle.GetBool() ? 3 : surf->numIndexes,
		GL_INDEX_TYPE,
		( triIndex_t* )indexOffset,
		instanceCount,
		vertOffset / sizeof( idDrawVert ) );

	backEnd.pc.c_drawElements++;
	backEnd.pc.c_drawIndexes += surf->numIndexes;

	RENDERLOG_PRINT( ">>>>> GL_DrawZeroOneCube( VBO %u:%i, IBO %u:%i, numIndexes:%i )\n", vbo, vertOffset, ibo, indexOffset, surf->numIndexes );
}
/*
============================================================
 GL_DrawZeroOneCube
============================================================
*/
void GL_DrawUnitSquare( vertexLayoutType_t vertexLayout, int instanceCount )
{
	const auto * const surf = &backEnd.unitSquareSurface;

	GL_CommitProgUniforms( GL_GetCurrentRenderProgram() );

	// get vertex buffer --------------------------------

	idVertexBuffer vertexBuffer;
	vertexCache.GetVertexBuffer( surf->vertexCache, vertexBuffer );
	const GLint vertOffset = vertexBuffer.GetOffset();
	const GLuint vbo = GetGLObject( vertexBuffer.GetAPIObject() );

	// get index buffer --------------------------------

	idIndexBuffer indexBuffer;
	vertexCache.GetIndexBuffer( surf->indexCache, indexBuffer );
	const GLintptr indexOffset = indexBuffer.GetOffset();
	const GLuint ibo = GetGLObject( indexBuffer.GetAPIObject() );

	GL_SetVertexArray( vbo, vertexLayout );
	GL_SetIndexBuffer( ibo );

	glDrawElementsInstancedBaseVertex( GL_TRIANGLES,
		r_singleTriangle.GetBool() ? 3 : surf->numIndexes,
		GL_INDEX_TYPE,
		( triIndex_t* )indexOffset,
		instanceCount,
		vertOffset / sizeof( idDrawVert ) );

	backEnd.pc.c_drawElements++;
	backEnd.pc.c_drawIndexes += surf->numIndexes;

	RENDERLOG_PRINT( ">>>>> GL_DrawUnitSquare( VBO %u:%i, IBO %u:%i, numIndexes:%i )\n", vbo, vertOffset, ibo, indexOffset, surf->numIndexes );
}

/*
============================================================
 GL_DrawUnitSquareAuto
============================================================
*/
void GL_DrawZeroOneCubeAuto()
{
	GL_CommitProgUniforms( GL_GetCurrentRenderProgram() );

	if( backEnd.glState.currentVAO != glConfig.empty_vao )
	{
		glBindVertexArray( glConfig.empty_vao );
		backEnd.glState.currentVAO = glConfig.empty_vao;

		backEnd.glState.vertexLayout = LAYOUT_UNKNOWN;
		backEnd.glState.currentVertexBuffer = 0;
		backEnd.glState.currentIndexBuffer = 0;
	}

	glDrawArrays( GL_TRIANGLE_STRIP, 0, 14 );

	RENDERLOG_PRINT( ">>>>> GL_DrawScreenTriangleAuto()\n" );
}
/*
============================================================
 GL_DrawScreenTriangleAuto
============================================================
*/
void GL_DrawScreenTriangleAuto()
{
	GL_CommitProgUniforms( GL_GetCurrentRenderProgram() );

	if( backEnd.glState.currentVAO != glConfig.empty_vao )
	{
		glBindVertexArray( glConfig.empty_vao );
		backEnd.glState.currentVAO = glConfig.empty_vao;

		backEnd.glState.vertexLayout = LAYOUT_UNKNOWN;
		backEnd.glState.currentVertexBuffer = 0;
		backEnd.glState.currentIndexBuffer = 0;
	}

	glDrawArrays( GL_TRIANGLES, 0, 3 );

	RENDERLOG_PRINT( ">>>>> GL_DrawScreenTriangleAuto()\n" );
}




/*
========================================
 Flush the command buffer
========================================
*/
void GL_Flush()
{
	glFlush();
}


/*
========================================
 Inserts a timing mark for the start of the GPU frame.
========================================
*/
void GL_StartFrame( int frame )
{
	if( !glConfig.timerQueryAvailable )
		return;

	if( glConfig.timerQueryId == GL_NONE )
	{
		glGenQueries( 1, &glConfig.timerQueryId );
	}
	glBeginQuery( GL_TIME_ELAPSED_EXT, glConfig.timerQueryId );
}
/*
========================================
 Inserts a timing mark for the end of the GPU frame.
========================================
*/
void GL_EndFrame()
{
	UnbindBufferObjects();

	if( !glConfig.timerQueryAvailable )
		return;

	glEndQuery( GL_TIME_ELAPSED_EXT );

	glFlush();
}
/*
========================================
 Read back the start and end timer queries from the previous frame
========================================
*/
void GL_GetLastFrameTime( uint64& endGPUTimeMicroSec )
{
	endGPUTimeMicroSec = 0ull;

	if( !glConfig.timerQueryAvailable )
		return;

	GLuint64EXT drawingTimeNanoseconds = 0;
	if( glConfig.timerQueryId != GL_NONE )
	{
		glGetQueryObjectui64vEXT( glConfig.timerQueryId, GL_QUERY_RESULT, &drawingTimeNanoseconds );
	}

	endGPUTimeMicroSec = drawingTimeNanoseconds / 1000ull;
}




idCVar r_showSwapBuffers( "r_showSwapBuffers", "0", CVAR_BOOL, "Show timings from GL_BlockingSwapBuffers" );
idCVar r_syncEveryFrame( "r_syncEveryFrame", "1", CVAR_BOOL, "Don't let the GPU buffer execution past swapbuffers" );

static int		swapIndex;		// 0 or 1 into renderSync
static GLsync	renderSync[ 2 ];

/*
========================================
 GL_BlockingSwapBuffers
	We want to exit this with the GPU idle, right at vsync
========================================
*/
void GL_BlockingSwapBuffers()
{
	RENDERLOG_PRINT( "***************** GL_BlockingSwapBuffers *****************\n\n\n" );

	const int beforeFinish = sys->Milliseconds();

	if( !glConfig.syncAvailable )
	{
		glFinish();
	}

	const int beforeSwap = sys->Milliseconds();
	if( r_showSwapBuffers.GetBool() && beforeSwap - beforeFinish > 1 )
	{
		common->Printf( "%i msec to glFinish\n", beforeSwap - beforeFinish );
	}

	extern void GLimp_SwapBuffers();
	GLimp_SwapBuffers();

	const int beforeFence = sys->Milliseconds();
	if( r_showSwapBuffers.GetBool() && beforeFence - beforeSwap > 1 )
	{
		common->Printf( "%i msec to swapBuffers\n", beforeFence - beforeSwap );
	}

	if( glConfig.syncAvailable )
	{
		swapIndex ^= 1;

		if( glIsSync( renderSync[ swapIndex ] ) )
		{
			glDeleteSync( renderSync[ swapIndex ] );
		}
		// draw something tiny to ensure the sync is after the swap
		const int start = sys->Milliseconds();
		glScissor( 0, 0, 1, 1 );
		glEnable( GL_SCISSOR_TEST );
		glClear( GL_COLOR_BUFFER_BIT );
		renderSync[ swapIndex ] = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
		const int end = sys->Milliseconds();
		if( r_showSwapBuffers.GetBool() && end - start > 1 )
		{
			common->Printf( "%i msec to start fence\n", end - start );
		}

		GLsync syncToWaitOn = ( r_syncEveryFrame.GetBool() ) ? renderSync[ swapIndex ] : renderSync[ !swapIndex ];
		if( glIsSync( syncToWaitOn ) )
		{
			for( GLenum r = GL_TIMEOUT_EXPIRED; r == GL_TIMEOUT_EXPIRED; )
			{
				r = glClientWaitSync( syncToWaitOn, GL_SYNC_FLUSH_COMMANDS_BIT, 1000 * 1000 );
			}
		}
	}

	const int afterFence = sys->Milliseconds();
	if( r_showSwapBuffers.GetBool() && afterFence - beforeFence > 1 )
	{
		common->Printf( "%i msec to wait on fence\n", afterFence - beforeFence );
	}

	const int64 exitBlockTime = sys->Microseconds();

	static int64 prevBlockTime;
	if( r_showSwapBuffers.GetBool() && prevBlockTime )
	{
		const int delta = ( int )( exitBlockTime - prevBlockTime );
		common->Printf( "blockToBlock: %i\n", delta );
	}
	prevBlockTime = exitBlockTime;
}