/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2015 Robert Beckebans
Copyright (C) 2014 Carl Kenner

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

#include "tr_local.h"
#include "Framebuffer.h"

idCVar r_drawEyeColor( "r_drawEyeColor", "0", CVAR_RENDERER | CVAR_BOOL, "Draw a colored box, red = left eye, blue = right eye, grey = non-stereo" );
idCVar r_motionBlur( "r_motionBlur", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "1 - 5, log2 of the number of motion blur samples" );
idCVar r_forceZPassStencilShadows( "r_forceZPassStencilShadows", "0", CVAR_RENDERER | CVAR_BOOL, "force Z-pass rendering for performance testing" );
idCVar r_useStencilShadowPreload( "r_useStencilShadowPreload", "1", CVAR_RENDERER | CVAR_BOOL, "use stencil shadow preload algorithm instead of Z-fail" );
idCVar r_skipShaderPasses( "r_skipShaderPasses", "0", CVAR_RENDERER | CVAR_BOOL, "" );
idCVar r_skipInteractionFastPath( "r_skipInteractionFastPath", "1", CVAR_RENDERER | CVAR_BOOL, "" );
idCVar r_useLightStencilSelect( "r_useLightStencilSelect", "0", CVAR_RENDERER | CVAR_BOOL, "use stencil select pass" );

// RB: HDR parameters
idCVar r_useHDR( "r_useHDR", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "use high dynamic range rendering" );
idCVar r_hdrAutoExposure( "r_hdrAutoExposure", "1", CVAR_RENDERER | CVAR_BOOL, "EXPENSIVE: enables adapative HDR tone mapping otherwise the exposure is derived by r_exposure" );
idCVar r_hdrMinLuminance( "r_hdrMinLuminance", "0.005", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_hdrMaxLuminance( "r_hdrMaxLuminance", "300", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_hdrKey( "r_hdrKey", "0.015", CVAR_RENDERER | CVAR_FLOAT, "magic exposure key that works well with Doom 3 maps" );
idCVar r_hdrContrastDynamicThreshold( "r_hdrContrastDynamicThreshold", "2", CVAR_RENDERER | CVAR_FLOAT, "if auto exposure is on, all pixels brighter than this cause HDR bloom glares" );
idCVar r_hdrContrastStaticThreshold( "r_hdrContrastStaticThreshold", "3", CVAR_RENDERER | CVAR_FLOAT, "if auto exposure is off, all pixels brighter than this cause HDR bloom glares" );
idCVar r_hdrContrastOffset( "r_hdrContrastOffset", "100", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_hdrGlarePasses( "r_hdrGlarePasses", "8", CVAR_RENDERER | CVAR_INTEGER, "how many times the bloom blur is rendered offscreen. number should be even" );
idCVar r_hdrDebug( "r_hdrDebug", "0", CVAR_RENDERER | CVAR_FLOAT, "show scene luminance as heat map" );

idCVar r_ldrContrastThreshold( "r_ldrContrastThreshold", "1.1", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_ldrContrastOffset( "r_ldrContrastOffset", "3", CVAR_RENDERER | CVAR_FLOAT, "" );

idCVar r_useFilmicPostProcessEffects( "r_useFilmicPostProcessEffects", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "apply several post process effects to mimic a filmic look" );
idCVar r_forceAmbient( "r_forceAmbient", "0.2", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "render additional ambient pass to make the game less dark", 0.0f, 0.5f );

idCVar r_useSSGI( "r_useSSGI", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "use screen space global illumination and reflections" );
idCVar r_ssgiDebug( "r_ssgiDebug", "0", CVAR_RENDERER | CVAR_INTEGER, "" );
idCVar r_ssgiFiltering( "r_ssgiFiltering", "1", CVAR_RENDERER | CVAR_BOOL, "" );

idCVar r_useSSAO( "r_useSSAO", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "use screen space ambient occlusion to darken corners" );
idCVar r_ssaoDebug( "r_ssaoDebug", "0", CVAR_RENDERER | CVAR_INTEGER, "" );
idCVar r_ssaoFiltering( "r_ssaoFiltering", "1", CVAR_RENDERER | CVAR_BOOL, "" );
idCVar r_useHierarchicalDepthBuffer( "r_useHierarchicalDepthBuffer", "1", CVAR_RENDERER | CVAR_BOOL, "" );

idCVar r_exposure( "r_exposure", "0.5", CVAR_ARCHIVE | CVAR_RENDERER | CVAR_FLOAT, "HDR exposure or LDR brightness [0.0 .. 1.0]", 0.0f, 1.0f );
// RB end

extern idCVar stereoRender_swapEyes;

backEndState_t	backEnd;

/*
================
SetVertexParm
================
*/
static ID_INLINE void SetVertexParm( renderParm_t rp, const float* value )
{
	renderProgManager.SetUniformValue( rp, value );
}

/*
================
SetVertexParms
================
*/
static ID_INLINE void SetVertexParms( renderParm_t rp, const float* value, int num )
{
	for( int i = 0; i < num; i++ )
	{
		renderProgManager.SetUniformValue( ( renderParm_t )( rp + i ), value + ( i * 4 ) );
	}
}

/*
================
SetFragmentParm
================
*/
static ID_INLINE void SetFragmentParm( renderParm_t rp, const float* value )
{
	renderProgManager.SetUniformValue( rp, value );
}

/*
================
RB_SetMVP
================
*/
void RB_SetMVP( const idRenderMatrix& mvp )
{
	SetVertexParms( RENDERPARM_MVPMATRIX_X, mvp[0], 4 );
}

/*
================
RB_SetMVPWithStereoOffset
================
*/
static void RB_SetMVPWithStereoOffset( const idRenderMatrix& mvp, const float stereoOffset )
{
	idRenderMatrix offset = mvp;
	offset[0][3] += stereoOffset;
	
	SetVertexParms( RENDERPARM_MVPMATRIX_X, offset[0], 4 );
}

static const ALIGNTYPE16 float zero[4] = { 0, 0, 0, 0 };
static const ALIGNTYPE16 float one[4] = { 1, 1, 1, 1 };
static const ALIGNTYPE16 float negOne[4] = { -1, -1, -1, -1 };

/*
================
RB_SetVertexColorParms
================
*/
void RB_SetVertexColorParms( stageVertexColor_t svc )
{
	switch( svc )
	{
		case SVC_IGNORE:
			SetVertexParm( RENDERPARM_VERTEXCOLOR_MODULATE, zero );
			SetVertexParm( RENDERPARM_VERTEXCOLOR_ADD, one );
			break;
		case SVC_MODULATE:
			SetVertexParm( RENDERPARM_VERTEXCOLOR_MODULATE, one );
			SetVertexParm( RENDERPARM_VERTEXCOLOR_ADD, zero );
			break;
		case SVC_INVERSE_MODULATE:
			SetVertexParm( RENDERPARM_VERTEXCOLOR_MODULATE, negOne );
			SetVertexParm( RENDERPARM_VERTEXCOLOR_ADD, one );
			break;
	}
}

/*
======================
RB_GetShaderTextureMatrix
======================
*/
static void RB_GetShaderTextureMatrix( const float* shaderRegisters, const textureStage_t* texture, float matrix[16] )
{
	matrix[0 * 4 + 0] = shaderRegisters[ texture->matrix[0][0] ];
	matrix[1 * 4 + 0] = shaderRegisters[ texture->matrix[0][1] ];
	matrix[2 * 4 + 0] = 0.0f;
	matrix[3 * 4 + 0] = shaderRegisters[ texture->matrix[0][2] ];
	
	matrix[0 * 4 + 1] = shaderRegisters[ texture->matrix[1][0] ];
	matrix[1 * 4 + 1] = shaderRegisters[ texture->matrix[1][1] ];
	matrix[2 * 4 + 1] = 0.0f;
	matrix[3 * 4 + 1] = shaderRegisters[ texture->matrix[1][2] ];
	
	// we attempt to keep scrolls from generating incredibly large texture values, but
	// center rotations and center scales can still generate offsets that need to be > 1
	if( matrix[3 * 4 + 0] < -40.0f || matrix[12] > 40.0f )
	{
		matrix[3 * 4 + 0] -= ( int )matrix[3 * 4 + 0];
	}
	if( matrix[13] < -40.0f || matrix[13] > 40.0f )
	{
		matrix[13] -= ( int )matrix[13];
	}
	
	matrix[0 * 4 + 2] = 0.0f;
	matrix[1 * 4 + 2] = 0.0f;
	matrix[2 * 4 + 2] = 1.0f;
	matrix[3 * 4 + 2] = 0.0f;
	
	matrix[0 * 4 + 3] = 0.0f;
	matrix[1 * 4 + 3] = 0.0f;
	matrix[2 * 4 + 3] = 0.0f;
	matrix[3 * 4 + 3] = 1.0f;
}

/*
======================
RB_LoadShaderTextureMatrix
======================
*/
static void RB_LoadShaderTextureMatrix( const float* shaderRegisters, const textureStage_t* texture )
{
	idRenderVector texS( 1.0f, 0.0f, 0.0f, 0.0f );
	idRenderVector texT( 0.0f, 1.0f, 0.0f, 0.0f );
	
	if( texture->hasMatrix )
	{
		ALIGNTYPE16 float matrix[16];
		RB_GetShaderTextureMatrix( shaderRegisters, texture, matrix );

		texS[0] = matrix[0 * 4 + 0];
		texS[1] = matrix[1 * 4 + 0];
		texS[2] = matrix[2 * 4 + 0];
		texS[3] = matrix[3 * 4 + 0];
		
		texT[0] = matrix[0 * 4 + 1];
		texT[1] = matrix[1 * 4 + 1];
		texT[2] = matrix[2 * 4 + 1];
		texT[3] = matrix[3 * 4 + 1];
		
		RENDERLOG_PRINTF( "Setting Texture Matrix\n" );
		RENDERLOG_INDENT();
		RENDERLOG_PRINTF( "Texture Matrix S : %4.3f, %4.3f, %4.3f, %4.3f\n", texS[0], texS[1], texS[2], texS[3] );
		RENDERLOG_PRINTF( "Texture Matrix T : %4.3f, %4.3f, %4.3f, %4.3f\n", texT[0], texT[1], texT[2], texT[3] );
		RENDERLOG_OUTDENT();
	}
	
	SetVertexParm( RENDERPARM_TEXTUREMATRIX_S, texS.ToFloatPtr() );
	SetVertexParm( RENDERPARM_TEXTUREMATRIX_T, texT.ToFloatPtr() );
}

/*
=====================
RB_BakeTextureMatrixIntoTexgen
=====================
*/
static void RB_BakeTextureMatrixIntoTexgen( idPlane lightProject[3], const float* textureMatrix )
{
	ALIGN16( float genMatrix[16] );
	ALIGN16( float final[16] );
	
	genMatrix[0 * 4 + 0] = lightProject[0][0];
	genMatrix[1 * 4 + 0] = lightProject[0][1];
	genMatrix[2 * 4 + 0] = lightProject[0][2];
	genMatrix[3 * 4 + 0] = lightProject[0][3];
	
	genMatrix[0 * 4 + 1] = lightProject[1][0];
	genMatrix[1 * 4 + 1] = lightProject[1][1];
	genMatrix[2 * 4 + 1] = lightProject[1][2];
	genMatrix[3 * 4 + 1] = lightProject[1][3];
	
	genMatrix[0 * 4 + 2] = 0.0f;
	genMatrix[1 * 4 + 2] = 0.0f;
	genMatrix[2 * 4 + 2] = 0.0f;
	genMatrix[3 * 4 + 2] = 0.0f;
	
	genMatrix[0 * 4 + 3] = lightProject[2][0];
	genMatrix[1 * 4 + 3] = lightProject[2][1];
	genMatrix[2 * 4 + 3] = lightProject[2][2];
	genMatrix[3 * 4 + 3] = lightProject[2][3];
	
	idRenderMatrix::Multiply( *( idRenderMatrix* )genMatrix, *( idRenderMatrix* )textureMatrix, *( idRenderMatrix* )final );
	
	lightProject[0][0] = final[0 * 4 + 0];
	lightProject[0][1] = final[1 * 4 + 0];
	lightProject[0][2] = final[2 * 4 + 0];
	lightProject[0][3] = final[3 * 4 + 0];
	
	lightProject[1][0] = final[0 * 4 + 1];
	lightProject[1][1] = final[1 * 4 + 1];
	lightProject[1][2] = final[2 * 4 + 1];
	lightProject[1][3] = final[3 * 4 + 1];
}

/*
======================
RB_BindVariableStageImage

Handles generating a cinematic frame if needed
======================
*/
static void RB_BindVariableStageImage( const textureStage_t* texture, const float* shaderRegisters )
{
	if( texture->cinematic )
	{
		cinData_t cin;
		
		if( r_skipDynamicTextures.GetBool() )
		{
			globalImages->defaultImage->Bind();
			return;
		}
		
		// offset time by shaderParm[7] (FIXME: make the time offset a parameter of the shader?)
		// We make no attempt to optimize for multiple identical cinematics being in view, or
		// for cinematics going at a lower framerate than the renderer.
		cin = texture->cinematic->ImageForTime( backEnd.viewDef->GetGameTimeMS() + SEC2MS( backEnd.viewDef->GetMaterialParms()[ 11 ] ) );
		if( cin.imageY != NULL )
		{
			GL_BindTexture( 0, cin.imageY );
			GL_BindTexture( 1, cin.imageCr );
			GL_BindTexture( 2, cin.imageCb );			
		}
		else if( cin.image != NULL )
		{
			// Carl: A single RGB image works better with the FFMPEG BINK codec.
			GL_BindTexture( 0, cin.image );
			
			/*
			if( backEnd.viewDef->is2Dgui )
			{
				renderProgManager.BindShader_TextureVertexColor_sRGB();
			}
			else
			{
				renderProgManager.BindShader_TextureVertexColor();
			}
			*/
		}
		else
		{
			globalImages->blackImage->Bind();
			// because the shaders may have already been set - we need to make sure we are not using a bink shader which would
			// display incorrectly.  We may want to get rid of RB_BindVariableStageImage and inline the code so that the
			// SWF GUI case is handled better, too
			renderProgManager.BindShader_TextureVertexColor();
		}
	}
	else
	{
		// FIXME: see why image is invalid
		if( texture->image != NULL )
		{
			texture->image->Bind();
		}
	}
}

/*
================
RB_PrepareStageTexturing
================
*/
static void RB_PrepareStageTexturing( const shaderStage_t* pStage,  const drawSurf_t* surf )
{
	idRenderVector useTexGenParm( 0.0f, 0.0f, 0.0f, 0.0f );
	
	// set the texture matrix if needed
	RB_LoadShaderTextureMatrix( surf->shaderRegisters, &pStage->texture );
	
	// texgens
	if( pStage->texture.texgen == TG_REFLECT_CUBE )
	{
	
		// see if there is also a bump map specified
		auto bumpStage = surf->material->GetBumpStage();
		if( bumpStage != NULL )
		{
			// per-pixel reflection mapping with bump mapping
			GL_BindTexture( 1, bumpStage->texture.image );

			GL_SelectTexture( 0 );
			
			RENDERLOG_PRINTF( "TexGen: TG_REFLECT_CUBE: Bumpy Environment\n" );
			if( surf->jointCache )
			{
				renderProgManager.BindShader_BumpyEnvironmentSkinned();
			}
			else
			{
				renderProgManager.BindShader_BumpyEnvironment();
			}
		}
		else
		{
			RENDERLOG_PRINTF( "TexGen: TG_REFLECT_CUBE: Environment\n" );
			if( surf->jointCache )
			{
				renderProgManager.BindShader_EnvironmentSkinned();
			}
			else
			{
				renderProgManager.BindShader_Environment();
			}
		}
		
	}
	else if( pStage->texture.texgen == TG_SKYBOX_CUBE )
	{
	
		renderProgManager.BindShader_SkyBox();
		
	}
	else if( pStage->texture.texgen == TG_WOBBLESKY_CUBE )
	{
	
		const int* parms = surf->material->GetTexGenRegisters();
		
		float wobbleDegrees = surf->shaderRegisters[ parms[0] ] * ( idMath::PI / 180.0f );
		float wobbleSpeed   = surf->shaderRegisters[ parms[1] ] * ( 2.0f * idMath::PI / 60.0f );
		float rotateSpeed   = surf->shaderRegisters[ parms[2] ] * ( 2.0f * idMath::PI / 60.0f );
		
		idVec3 axis[3];
		{
			// very ad-hoc "wobble" transform
			float s, c;
			idMath::SinCos( wobbleSpeed * backEnd.viewDef->GetGameTimeSec(), s, c );
			
			float ws, wc;
			idMath::SinCos( wobbleDegrees, ws, wc );
			
			axis[2][0] = ws * c;
			axis[2][1] = ws * s;
			axis[2][2] = wc;
			
			axis[1][0] = -s * s * ws;
			axis[1][2] = -s * ws * ws;
			axis[1][1] = idMath::Sqrt( idMath::Fabs( 1.0f - ( axis[1][0] * axis[1][0] + axis[1][2] * axis[1][2] ) ) );
			
			// make the second vector exactly perpendicular to the first
			axis[1] -= ( axis[2] * axis[1] ) * axis[2];
			axis[1].Normalize();
			
			// construct the third with a cross
			axis[0].Cross( axis[1], axis[2] );
		}
		
		// add the rotate
		float rs, rc;
		idMath::SinCos( rotateSpeed * backEnd.viewDef->GetGameTimeSec(), rs, rc );
		
		float transform[12];
		transform[0 * 4 + 0] = axis[0][0] * rc + axis[1][0] * rs;
		transform[0 * 4 + 1] = axis[0][1] * rc + axis[1][1] * rs;
		transform[0 * 4 + 2] = axis[0][2] * rc + axis[1][2] * rs;
		transform[0 * 4 + 3] = 0.0f;
		
		transform[1 * 4 + 0] = axis[1][0] * rc - axis[0][0] * rs;
		transform[1 * 4 + 1] = axis[1][1] * rc - axis[0][1] * rs;
		transform[1 * 4 + 2] = axis[1][2] * rc - axis[0][2] * rs;
		transform[1 * 4 + 3] = 0.0f;
		
		transform[2 * 4 + 0] = axis[2][0];
		transform[2 * 4 + 1] = axis[2][1];
		transform[2 * 4 + 2] = axis[2][2];
		transform[2 * 4 + 3] = 0.0f;
		
		SetVertexParms( RENDERPARM_WOBBLESKY_X, transform, 3 );
		renderProgManager.BindShader_WobbleSky();
		
	}
	else if( ( pStage->texture.texgen == TG_SCREEN ) || ( pStage->texture.texgen == TG_SCREEN2 ) )
	{

		useTexGenParm[0] = 1.0f;
		useTexGenParm[1] = 1.0f;
		useTexGenParm[2] = 1.0f;
		useTexGenParm[3] = 1.0f;

		idRenderMatrix mat;
		idRenderMatrix::Multiply( backEnd.viewDef->GetProjectionMatrix(), surf->space->modelViewMatrix, mat );

		RENDERLOG_PRINTF( "TexGen : %s\n", ( pStage->texture.texgen == TG_SCREEN ) ? "TG_SCREEN" : "TG_SCREEN2" );
		RENDERLOG_INDENT();

		SetVertexParm( RENDERPARM_TEXGEN_0_S, mat[ 0 ] );
		SetVertexParm( RENDERPARM_TEXGEN_0_T, mat[ 1 ] );
		SetVertexParm( RENDERPARM_TEXGEN_0_Q, mat[ 3 ] );

		RENDERLOG_PRINTF( "TEXGEN_S = %4.3f, %4.3f, %4.3f, %4.3f\n", mat[ 0 ][ 0 ], mat[ 0 ][ 1 ], mat[ 0 ][ 2 ], mat[ 0 ][ 3 ] );
		RENDERLOG_PRINTF( "TEXGEN_T = %4.3f, %4.3f, %4.3f, %4.3f\n", mat[ 1 ][ 0 ], mat[ 1 ][ 1 ], mat[ 1 ][ 2 ], mat[ 1 ][ 3 ] );
		RENDERLOG_PRINTF( "TEXGEN_Q = %4.3f, %4.3f, %4.3f, %4.3f\n", mat[ 3 ][ 0 ], mat[ 3 ][ 1 ], mat[ 3 ][ 2 ], mat[ 3 ][ 3 ] );

		RENDERLOG_OUTDENT();
		
	}
	else if( pStage->texture.texgen == TG_DIFFUSE_CUBE )
	{	
		// As far as I can tell, this is never used
		idLib::Warning( "Using Diffuse Cube! Please contact Brian!" );		
	}
	else if( pStage->texture.texgen == TG_GLASSWARP )
	{	
		// As far as I can tell, this is never used
		idLib::Warning( "Using GlassWarp! Please contact Brian!" );
	}
	
	SetVertexParm( RENDERPARM_TEXGEN_0_ENABLED, useTexGenParm.ToFloatPtr() );
}

/*
================
RB_FinishStageTexturing
================
*/
static void RB_FinishStageTexturing( const shaderStage_t* pStage, const drawSurf_t* surf )
{

	if( pStage->texture.cinematic )
	{
		// unbind the extra bink textures
		GL_SelectTexture( 1 );
		globalImages->BindNull();
		GL_SelectTexture( 2 );
		globalImages->BindNull();
		GL_SelectTexture( 0 );
	}
	
	if( pStage->texture.texgen == TG_REFLECT_CUBE )
	{
		// see if there is also a bump map specified
		auto bumpStage = surf->material->GetBumpStage();
		if( bumpStage != NULL )
		{
			// per-pixel reflection mapping with bump mapping
			GL_SelectTexture( 1 );
			globalImages->BindNull();
			GL_SelectTexture( 0 );
		}
		else
		{
			// per-pixel reflection mapping without bump mapping
		}
		renderProgManager.Unbind();
	}
}

// RB: moved this up because we need to call this several times for shadow mapping
static void RB_ResetViewportAndScissorToDefaultCamera( const idRenderView* viewDef )
{
	// set the window clipping
	GL_Viewport( viewDef->GetViewport().x1,
				 viewDef->GetViewport().y1,
				 viewDef->GetViewport().GetWidth(),
				 viewDef->GetViewport().GetHeight() );
				 
	// the scissor may be smaller than the viewport for subviews
	GL_Scissor( viewDef->GetViewport().x1 + viewDef->GetScissor().x1,
				viewDef->GetViewport().y1 + viewDef->GetScissor().y1,
				viewDef->GetScissor().GetWidth(),
				viewDef->GetScissor().GetHeight() );

	backEnd.currentScissor = viewDef->scissor;
}
// RB end

static void RB_SetScissor( const idScreenRect & scissorRect )
{
	if( !backEnd.currentScissor.Equals( scissorRect ) && r_useScissor.GetBool() )
	{
		GL_Scissor( 
			backEnd.viewDef->GetViewport().x1 + scissorRect.x1,
			backEnd.viewDef->GetViewport().y1 + scissorRect.y1,
			scissorRect.GetWidth(),
			scissorRect.GetHeight() );

		backEnd.currentScissor = scissorRect;
	}
}

/*
=========================================================================================

DEPTH BUFFER RENDERING

=========================================================================================
*/

/*
==================
RB_FillDepthBufferGeneric
==================
*/
static void RB_FillDepthBufferGeneric( const drawSurf_t* const* drawSurfs, int numDrawSurfs )
{
	for( int i = 0; i < numDrawSurfs; i++ )
	{
		const drawSurf_t* const drawSurf = drawSurfs[i];
		const idMaterial* const shader = drawSurf->material;
		
		// translucent surfaces don't put anything in the depth buffer and don't
		// test against it, which makes them fail the mirror clip plane operation
		if( shader->Coverage() == MC_TRANSLUCENT )
		{
			continue;
		}
		
		// get the expressions for conditionals / color / texcoords
		const float* regs = drawSurf->shaderRegisters;
		
		// if all stages of a material have been conditioned off, don't do anything
		int stage = 0;
		for( ; stage < shader->GetNumStages(); stage++ )
		{
			const shaderStage_t* const pStage = shader->GetStage( stage );
			// check the stage enable condition
			if( regs[ pStage->conditionRegister ] != 0 )
			{
				break;
			}
		}
		if( stage == shader->GetNumStages() )
		{
			continue;
		}
		
		// change the matrix if needed
		if( drawSurf->space != backEnd.currentSpace )
		{
			RB_SetMVP( drawSurf->space->mvp );
			
			backEnd.currentSpace = drawSurf->space;
		}
		
		uint64 surfGLState = 0;
		
		// set polygon offset if necessary
		if( shader->TestMaterialFlag( MF_POLYGONOFFSET ) )
		{
			surfGLState |= GLS_POLYGON_OFFSET;
			GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
		}
		
		// subviews will just down-modulate the color buffer
		idRenderVector color;
		if( shader->GetSort() == SS_SUBVIEW )
		{
			surfGLState |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_DEPTHFUNC_LESS;
			color[0] = 1.0f;
			color[1] = 1.0f;
			color[2] = 1.0f;
			color[3] = 1.0f;
		}
		else
		{
			// others just draw black
			color[0] = 0.0f;
			color[1] = 0.0f;
			color[2] = 0.0f;
			color[3] = 1.0f;
		}
		
		RENDERLOG_OPEN_BLOCK( shader->GetName() );
		
		bool drawSolid = false;
		if( shader->Coverage() == MC_OPAQUE )
		{
			drawSolid = true;
		}
		else if( shader->Coverage() == MC_PERFORATED )
		{
			// we may have multiple alpha tested stages
			// if the only alpha tested stages are condition register omitted,
			// draw a normal opaque surface
			bool didDraw = false;
			
			// perforated surfaces may have multiple alpha tested stages
			for( stage = 0; stage < shader->GetNumStages(); stage++ )
			{
				const shaderStage_t* const pStage = shader->GetStage( stage );
				
				if( !pStage->hasAlphaTest )
				{
					continue;
				}
				
				// check the stage enable condition
				if( regs[ pStage->conditionRegister ] == 0 )
				{
					continue;
				}
				
				// if we at least tried to draw an alpha tested stage,
				// we won't draw the opaque surface
				didDraw = true;
				
				// set the alpha modulate
				color[3] = regs[ pStage->color.registers[3] ];
				
				// skip the entire stage if alpha would be black
				if( color[3] <= 0.0f )
				{
					continue;
				}
				
				uint64 stageGLState = surfGLState;
				
				// set privatePolygonOffset if necessary
				if( pStage->privatePolygonOffset )
				{
					GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * pStage->privatePolygonOffset );
					stageGLState |= GLS_POLYGON_OFFSET;
				}
				
				GL_Color( color );
				
				GL_State( stageGLState );
				idRenderVector alphaTestValue( regs[ pStage->alphaTestRegister ] );
				SetFragmentParm( RENDERPARM_ALPHA_TEST, alphaTestValue.ToFloatPtr() );
				
				if( drawSurf->jointCache )
				{
					renderProgManager.BindShader_TextureVertexColorSkinned();
				}
				else
				{
					renderProgManager.BindShader_TextureVertexColor();
				}
				
				RB_SetVertexColorParms( SVC_IGNORE );
				
				// bind the texture
				GL_BindTexture( 0, pStage->texture.image );
				
				// set texture matrix and texGens
				RB_PrepareStageTexturing( pStage, drawSurf );
				
				// must render with less-equal for Z-Cull to work properly
				assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );
				
				// draw it
				GL_DrawElementsWithCounters( drawSurf );
				
				// clean up
				RB_FinishStageTexturing( pStage, drawSurf );
				
				// unset privatePolygonOffset if necessary
				if( pStage->privatePolygonOffset )
				{
					GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
				}
			}
			
			if( !didDraw )
			{
				drawSolid = true;
			}
		}
		
		// draw the entire surface solid
		if( drawSolid )
		{
			if( shader->GetSort() == SS_SUBVIEW )
			{
				renderProgManager.BindShader_Color();
				GL_Color( color );
				GL_State( surfGLState );
			}
			else
			{
				if( drawSurf->jointCache )
				{
					renderProgManager.BindShader_DepthSkinned();
				}
				else
				{
					renderProgManager.BindShader_Depth();
				}
				GL_State( surfGLState | GLS_ALPHAMASK );
			}
			
			// must render with less-equal for Z-Cull to work properly
			assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );
			
			// draw it
			GL_DrawElementsWithCounters( drawSurf );
		}
		
		RENDERLOG_CLOSE_BLOCK();
	}
	
	SetFragmentParm( RENDERPARM_ALPHA_TEST, vec4_zero.ToFloatPtr() );
}

/*
=====================
RB_FillDepthBufferFast

	Optimized fast path code.

	If there are subview surfaces, they must be guarded in the depth buffer to allow
	the mirror / subview to show through underneath the current view rendering.

	Surfaces with perforated shaders need the full shader setup done, but should be
	drawn after the opaque surfaces.

	The bulk of the surfaces should be simple opaque geometry that can be drawn very rapidly.

	If there are no subview surfaces, we could clear to black and use fast-Z rendering
	on the 360.
=====================
*/
static void RB_FillDepthBufferFast( const drawSurf_t * const * drawSurfs, int numDrawSurfs )
{
	if( numDrawSurfs == 0 )
	{
		return;
	}
	
	// if we are just doing 2D rendering, no need to fill the depth buffer
	if( backEnd.viewDef->viewEntitys == NULL )
	{
		return;
	}
	
	RENDERLOG_OPEN_MAINBLOCK( MRB_FILL_DEPTH_BUFFER );
	RENDERLOG_OPEN_BLOCK( "RB_FillDepthBufferFast" );
	
	GL_StartDepthPass( backEnd.viewDef->GetScissor() );
	
	// force MVP change on first surface
	backEnd.currentSpace = NULL;
	
	// draw all the subview surfaces, which will already be at the start of the sorted list,
	// with the general purpose path
	GL_State( GLS_DEFAULT );
	
	int	surfNum;
	for( surfNum = 0; surfNum < numDrawSurfs; ++surfNum )
	{
		if( drawSurfs[surfNum]->material->GetSort() != SS_SUBVIEW )
		{
			break;
		}
		RB_FillDepthBufferGeneric( &drawSurfs[surfNum], 1 );
	}
	
	const drawSurf_t** perforatedSurfaces = ( const drawSurf_t** )_alloca( numDrawSurfs * sizeof( drawSurf_t* ) );
	int numPerforatedSurfaces = 0;
	
	// draw all the opaque surfaces and build up a list of perforated surfaces that
	// we will defer drawing until all opaque surfaces are done
	GL_State( GLS_DEFAULT );
	
	// continue checking past the subview surfaces
	for( ; surfNum < numDrawSurfs; ++surfNum )
	{
		const drawSurf_t* const surf = drawSurfs[ surfNum ];
		const idMaterial* const shader = surf->material;
		
		// translucent surfaces don't put anything in the depth buffer
		if( shader->Coverage() == MC_TRANSLUCENT )
		{
			continue;
		}
		if( shader->Coverage() == MC_PERFORATED )
		{
			// save for later drawing
			perforatedSurfaces[ numPerforatedSurfaces ] = surf;
			++numPerforatedSurfaces;
			continue;
		}
		
		// set polygon offset?
		
		// set mvp matrix
		if( surf->space != backEnd.currentSpace )
		{
			RB_SetMVP( surf->space->mvp );
			backEnd.currentSpace = surf->space;
		}
		
		RENDERLOG_OPEN_BLOCK( shader->GetName() );
		
		if( surf->jointCache )
		{
			renderProgManager.BindShader_DepthSkinned();
		}
		else
		{
			renderProgManager.BindShader_Depth();
		}
		
		// must render with less-equal for Z-Cull to work properly
		assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );
		
		// draw it solid
		GL_DrawElementsWithCounters( surf );
		
		RENDERLOG_CLOSE_BLOCK();
	}
	
	// draw all perforated surfaces with the general code path
	if( numPerforatedSurfaces > 0 )
	{
		RB_FillDepthBufferGeneric( perforatedSurfaces, numPerforatedSurfaces );
	}
	
	// Allow platform specific data to be collected after the depth pass.
	GL_FinishDepthPass();
	
	RENDERLOG_CLOSE_BLOCK();
	RENDERLOG_CLOSE_MAINBLOCK();
}

/*
=========================================================================================

GENERAL INTERACTION RENDERING

=========================================================================================
*/

const int INTERACTION_TEXUNIT_BUMP			= 0;
const int INTERACTION_TEXUNIT_FALLOFF		= 1;
const int INTERACTION_TEXUNIT_PROJECTION	= 2;
const int INTERACTION_TEXUNIT_DIFFUSE		= 3;
const int INTERACTION_TEXUNIT_SPECULAR		= 4;
const int INTERACTION_TEXUNIT_SHADOWMAPS	= 5;
const int INTERACTION_TEXUNIT_JITTER		= 6;

/*
==================
RB_SetupInteractionStage
==================
*/
static void RB_SetupInteractionStage( const shaderStage_t* surfaceStage, 
	const float* surfaceRegs, const float lightColor[4], idVec4 matrix[2], float color[4] )
{
	if( surfaceStage->texture.hasMatrix )
	{
		matrix[0][0] = surfaceRegs[surfaceStage->texture.matrix[0][0]];
		matrix[0][1] = surfaceRegs[surfaceStage->texture.matrix[0][1]];
		matrix[0][2] = 0.0f;
		matrix[0][3] = surfaceRegs[surfaceStage->texture.matrix[0][2]];
		
		matrix[1][0] = surfaceRegs[surfaceStage->texture.matrix[1][0]];
		matrix[1][1] = surfaceRegs[surfaceStage->texture.matrix[1][1]];
		matrix[1][2] = 0.0f;
		matrix[1][3] = surfaceRegs[surfaceStage->texture.matrix[1][2]];
		
		// we attempt to keep scrolls from generating incredibly large texture values, but
		// center rotations and center scales can still generate offsets that need to be > 1
		if( matrix[0][3] < -40.0f || matrix[0][3] > 40.0f )
		{
			matrix[0][3] -= idMath::Ftoi( matrix[0][3] );
		}
		if( matrix[1][3] < -40.0f || matrix[1][3] > 40.0f )
		{
			matrix[1][3] -= idMath::Ftoi( matrix[1][3] );
		}
	}
	else
	{
		matrix[0][0] = 1.0f;
		matrix[0][1] = 0.0f;
		matrix[0][2] = 0.0f;
		matrix[0][3] = 0.0f;
		
		matrix[1][0] = 0.0f;
		matrix[1][1] = 1.0f;
		matrix[1][2] = 0.0f;
		matrix[1][3] = 0.0f;
	}
	
	if( color != NULL )
	{
		for( int i = 0; i < 4; i++ )
		{
			// clamp here, so cards with a greater range don't look different.
			// we could perform overbrighting like we do for lights, but
			// it doesn't currently look worth it.
			color[i] = idMath::ClampFloat( 0.0f, 1.0f, surfaceRegs[surfaceStage->color.registers[i]] ) * lightColor[i];
		}
	}
}

/*
=================
RB_DrawSingleInteraction
=================
*/
static void RB_DrawSingleInteraction( drawInteraction_t* din )
{
	if( din->bumpImage == NULL )
	{
		// stage wasn't actually an interaction
		return;
	}
	
	if( din->diffuseImage == NULL || r_skipDiffuse.GetBool() )
	{
		// this isn't a YCoCg black, but it doesn't matter, because
		// the diffuseColor will also be 0
		din->diffuseImage = globalImages->blackImage;
	}
	if( din->specularImage == NULL || r_skipSpecular.GetBool() || din->ambientLight )
	{
		din->specularImage = globalImages->blackImage;
	}
	if( r_skipBump.GetBool() )
	{
		din->bumpImage = globalImages->flatNormalMap;
	}
	
	// if we wouldn't draw anything, don't call the Draw function
	const bool diffuseIsBlack = ( din->diffuseImage == globalImages->blackImage )
								|| ( ( din->diffuseColor[0] <= 0 ) && ( din->diffuseColor[1] <= 0 ) && ( din->diffuseColor[2] <= 0 ) );
	const bool specularIsBlack = ( din->specularImage == globalImages->blackImage )
								 || ( ( din->specularColor[0] <= 0 ) && ( din->specularColor[1] <= 0 ) && ( din->specularColor[2] <= 0 ) );
	if( diffuseIsBlack && specularIsBlack )
	{
		return;
	}
	
	// bump matrix
	SetVertexParm( RENDERPARM_BUMPMATRIX_S, din->bumpMatrix[0].ToFloatPtr() );
	SetVertexParm( RENDERPARM_BUMPMATRIX_T, din->bumpMatrix[1].ToFloatPtr() );
	
	// diffuse matrix
	SetVertexParm( RENDERPARM_DIFFUSEMATRIX_S, din->diffuseMatrix[0].ToFloatPtr() );
	SetVertexParm( RENDERPARM_DIFFUSEMATRIX_T, din->diffuseMatrix[1].ToFloatPtr() );
	
	// specular matrix
	SetVertexParm( RENDERPARM_SPECULARMATRIX_S, din->specularMatrix[0].ToFloatPtr() );
	SetVertexParm( RENDERPARM_SPECULARMATRIX_T, din->specularMatrix[1].ToFloatPtr() );
	
	RB_SetVertexColorParms( din->vertexColor );
	
	SetFragmentParm( RENDERPARM_DIFFUSEMODIFIER, din->diffuseColor.ToFloatPtr() );
	SetFragmentParm( RENDERPARM_SPECULARMODIFIER, din->specularColor.ToFloatPtr() );
	
	// texture 0 will be the per-surface bump map
	GL_BindTexture( INTERACTION_TEXUNIT_BUMP, din->bumpImage );

	// texture 3 is the per-surface diffuse map
	GL_BindTexture( INTERACTION_TEXUNIT_DIFFUSE, din->diffuseImage );

	// texture 4 is the per-surface specular map
	GL_BindTexture( INTERACTION_TEXUNIT_SPECULAR, din->specularImage );
	
	GL_DrawElementsWithCounters( din->surf );
}

/*
=================
RB_SetupForFastPathInteractions

These are common for all fast path surfaces
=================
*/
static void RB_SetupForFastPathInteractions( const idVec4& diffuseColor, const idVec4& specularColor )
{
	const idRenderVector sMatrix( 1, 0, 0, 0 );
	const idRenderVector tMatrix( 0, 1, 0, 0 );
	
	// bump matrix
	SetVertexParm( RENDERPARM_BUMPMATRIX_S, sMatrix.ToFloatPtr() );
	SetVertexParm( RENDERPARM_BUMPMATRIX_T, tMatrix.ToFloatPtr() );
	
	// diffuse matrix
	SetVertexParm( RENDERPARM_DIFFUSEMATRIX_S, sMatrix.ToFloatPtr() );
	SetVertexParm( RENDERPARM_DIFFUSEMATRIX_T, tMatrix.ToFloatPtr() );
	
	// specular matrix
	SetVertexParm( RENDERPARM_SPECULARMATRIX_S, sMatrix.ToFloatPtr() );
	SetVertexParm( RENDERPARM_SPECULARMATRIX_T, tMatrix.ToFloatPtr() );
	
	RB_SetVertexColorParms( SVC_IGNORE );
	
	SetFragmentParm( RENDERPARM_DIFFUSEMODIFIER, diffuseColor.ToFloatPtr() );
	SetFragmentParm( RENDERPARM_SPECULARMODIFIER, specularColor.ToFloatPtr() );
}

/*
=============
RB_RenderInteractions

With added sorting and trivial path work.
=============
*/
static void RB_RenderInteractions( const drawSurf_t* const surfList, const viewLight_t* const vLight, 
	int depthFunc, bool performStencilTest, bool useLightDepthBounds )
{
	if( surfList == NULL )
	{
		return;
	}
	
	// change the scissor if needed, it will be constant across all the surfaces lit by the light
	RB_SetScissor( vLight->scissorRect );
	
	// perform setup here that will be constant for all interactions
	if( performStencilTest )
	{
		GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | depthFunc | GLS_STENCIL_FUNC_EQUAL | 
			GLS_STENCIL_MAKE_REF( STENCIL_SHADOW_TEST_VALUE ) | GLS_STENCIL_MAKE_MASK( STENCIL_SHADOW_MASK_VALUE ) );		
	} else
	{
		GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | depthFunc | GLS_STENCIL_FUNC_ALWAYS );
	}
	
	// some rare lights have multiple animating stages, loop over them outside the surface list
	const idMaterial* const lightShader = vLight->lightShader;
	const float* lightRegs = vLight->shaderRegisters;
	
	drawInteraction_t inter = {};
	inter.ambientLight = lightShader->IsAmbientLight();
	
	//---------------------------------
	// Split out the complex surfaces from the fast-path surfaces
	// so we can do the fast path ones all in a row.
	// The surfaces should already be sorted by space because they
	// are added single-threaded, and there is only a negligable amount
	// of benefit to trying to sort by materials.
	//---------------------------------
	static const int MAX_INTERACTIONS_PER_LIGHT = 1024;
	static const int MAX_COMPLEX_INTERACTIONS_PER_LIGHT = 256;
	idStaticList< const drawSurf_t*, MAX_INTERACTIONS_PER_LIGHT > allSurfaces;
	idStaticList< const drawSurf_t*, MAX_COMPLEX_INTERACTIONS_PER_LIGHT > complexSurfaces;
	for( const drawSurf_t* walk = surfList; walk != NULL; walk = walk->nextOnLight )
	{	
		// make sure the triangle culling is done
		if( walk->shadowVolumeState != SHADOWVOLUME_DONE )
		{
			assert( walk->shadowVolumeState == SHADOWVOLUME_UNFINISHED || walk->shadowVolumeState == SHADOWVOLUME_DONE );
			
			uint64 start = Sys_Microseconds();
			while( walk->shadowVolumeState == SHADOWVOLUME_UNFINISHED )
			{
				Sys_Yield();
			}
			uint64 end = Sys_Microseconds();
			
			backEnd.pc.shadowMicroSec += end - start;
		}
		
		const idMaterial* surfaceShader = walk->material;
		if( surfaceShader->GetFastPathBumpImage() )
		{
			allSurfaces.Append( walk );
		}
		else
		{
			complexSurfaces.Append( walk );
		}
	}
	for( int i = 0; i < complexSurfaces.Num(); ++i )
	{
		allSurfaces.Append( complexSurfaces[i] );
	}
	
	bool lightDepthBoundsDisabled = false;

	///const int32 shadowLOD = backEnd.viewDef->isSubview ? ( Max( vLight->shadowLOD + 1, MAX_SHADOWMAP_RESOLUTIONS-1 ) ) : vLight->shadowLOD ;
	const int32 shadowLOD = backEnd.viewDef->isSubview ? ( MAX_SHADOWMAP_RESOLUTIONS - 1 ) : vLight->shadowLOD;
	
	// RB begin
	if( r_useShadowMapping.GetBool() )
	{
		const static int JITTER_SIZE = 128;
		
		// default high quality
		float jitterSampleScale = 1.0f;
		float shadowMapSamples = r_shadowMapSamples.GetInteger();
		
		// screen power of two correction factor
		float screenCorrectionParm[4];
		screenCorrectionParm[0] = 1.0f / ( JITTER_SIZE * shadowMapSamples ) ;
		screenCorrectionParm[1] = 1.0f / JITTER_SIZE;
		screenCorrectionParm[2] = 1.0f / shadowMapResolutions[ shadowLOD ];
		screenCorrectionParm[3] = vLight->parallel ? r_shadowMapSunDepthBiasScale.GetFloat() : r_shadowMapRegularDepthBiasScale.GetFloat();
		SetFragmentParm( RENDERPARM_SCREENCORRECTIONFACTOR, screenCorrectionParm ); // rpScreenCorrectionFactor
		
		float jitterTexScale[4];
		jitterTexScale[0] = r_shadowMapJitterScale.GetFloat() * jitterSampleScale;	// TODO shadow buffer size fraction shadowMapSize / maxShadowMapSize
		jitterTexScale[1] = r_shadowMapJitterScale.GetFloat() * jitterSampleScale;
		jitterTexScale[2] = -r_shadowMapBiasScale.GetFloat();
		jitterTexScale[3] = 0.0f;
		SetFragmentParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale
		
		float jitterTexOffset[4];
		if( r_shadowMapRandomizeJitter.GetBool() )
		{
			jitterTexOffset[0] = ( rand() & 255 ) / 255.0;
			jitterTexOffset[1] = ( rand() & 255 ) / 255.0;
		}
		else
		{
			jitterTexOffset[0] = 0;
			jitterTexOffset[1] = 0;
		}
		jitterTexOffset[2] = 0.0f;
		jitterTexOffset[3] = 0.0f;
		SetFragmentParm( RENDERPARM_JITTERTEXOFFSET, jitterTexOffset ); // rpJitterTexOffset
		
		if( vLight->parallel )
		{
			//float cascadeDistances[4];
			//cascadeDistances[0] = backEnd.viewDef->GetSplitFrustumDistances()[0];
			//cascadeDistances[1] = backEnd.viewDef->GetSplitFrustumDistances()[1];
			//cascadeDistances[2] = backEnd.viewDef->GetSplitFrustumDistances()[2];
			//cascadeDistances[3] = backEnd.viewDef->GetSplitFrustumDistances()[3];
			SetFragmentParm( RENDERPARM_CASCADEDISTANCES, backEnd.viewDef->GetSplitFrustumDistances() ); // rpCascadeDistances
		}
		
	}
	// RB end
	
	const float lightScale = r_useHDR.GetBool() ? 3.0f : r_lightScale.GetFloat();
	
	for( int lightStageNum = 0; lightStageNum < lightShader->GetNumStages(); lightStageNum++ )
	{
		auto lightStage = lightShader->GetStage( lightStageNum );
		
		// ignore stages that fail the condition
		if( !lightRegs[ lightStage->conditionRegister ] )
		{
			continue;
		}
		
		const idRenderVector lightColor(
			lightScale * lightRegs[ lightStage->color.registers[0] ],
			lightScale * lightRegs[ lightStage->color.registers[1] ],
			lightScale * lightRegs[ lightStage->color.registers[2] ],
						 lightRegs[ lightStage->color.registers[3] ] );
		// apply the world-global overbright and the 2x factor for specular
		const idRenderVector diffuseColor = lightColor;
		const idRenderVector specularColor = lightColor * 2.0f;
		
		ALIGN16( float lightTextureMatrix[16] );
		if( lightStage->texture.hasMatrix )
		{
			RB_GetShaderTextureMatrix( lightRegs, &lightStage->texture, lightTextureMatrix );
		}
		
		// texture 1 will be the light falloff texture
		GL_BindTexture( INTERACTION_TEXUNIT_FALLOFF, vLight->falloffImage );
		
		// texture 2 will be the light projection texture
		GL_BindTexture( INTERACTION_TEXUNIT_PROJECTION, lightStage->texture.image );
		
		if( r_useShadowMapping.GetBool() )
		{
			// texture 5 will be the shadow maps array
			GL_BindTexture( INTERACTION_TEXUNIT_SHADOWMAPS, globalImages->shadowImage[ shadowLOD ] );
			
			// texture 6 will be the jitter texture for soft shadowing
			if( r_shadowMapSamples.GetInteger() == 16 )
			{
				GL_BindTexture( INTERACTION_TEXUNIT_JITTER, globalImages->jitterImage16 );
			}
			else if( r_shadowMapSamples.GetInteger() == 4 )
			{
				GL_BindTexture( INTERACTION_TEXUNIT_JITTER, globalImages->jitterImage4 );
			}
			else
			{
				GL_BindTexture( INTERACTION_TEXUNIT_JITTER, globalImages->jitterImage1 );
			}
		}
		
		// force the light textures to not use anisotropic filtering, which is wasted on them
		// all of the texture sampler parms should be constant for all interactions, only
		// the actual texture image bindings will change
		
		//----------------------------------
		// For all surfaces on this light list, generate an interaction for this light stage
		//----------------------------------
		
		// setup renderparms assuming we will be drawing trivial surfaces first
		RB_SetupForFastPathInteractions( diffuseColor, specularColor );
		
		// even if the space does not change between light stages, each light stage may need a different lightTextureMatrix baked in
		backEnd.currentSpace = NULL;
		
		for( int sortedSurfNum = 0; sortedSurfNum < allSurfaces.Num(); sortedSurfNum++ )
		{
			const drawSurf_t* const surf = allSurfaces[ sortedSurfNum ];
			
			// select the render prog
			if( lightShader->IsAmbientLight() )
			{
				if( surf->jointCache )
				{
					renderProgManager.BindShader_InteractionAmbientSkinned();
				}
				else
				{
					renderProgManager.BindShader_InteractionAmbient();
				}
			}
			else
			{
				if( r_useShadowMapping.GetBool() && vLight->globalShadows )
				{
					// RB: we have shadow mapping enabled and shadow maps so do a shadow compare
					
					if( vLight->parallel )
					{
						if( surf->jointCache )
						{
							renderProgManager.BindShader_Interaction_ShadowMapping_Parallel_Skinned();
						}
						else
						{
							renderProgManager.BindShader_Interaction_ShadowMapping_Parallel();
						}
					}
					else if( vLight->pointLight )
					{
						if( surf->jointCache )
						{
							renderProgManager.BindShader_Interaction_ShadowMapping_Point_Skinned();
						}
						else
						{
							renderProgManager.BindShader_Interaction_ShadowMapping_Point();
						}
					}
					else
					{
						if( surf->jointCache )
						{
							renderProgManager.BindShader_Interaction_ShadowMapping_Spot_Skinned();
						}
						else
						{
							renderProgManager.BindShader_Interaction_ShadowMapping_Spot();
						}
					}
				}
				else
				{
					if( surf->jointCache )
					{
						renderProgManager.BindShader_InteractionSkinned();
					}
					else
					{
						renderProgManager.BindShader_Interaction();
					}
				}
			}
			
			const idMaterial* const surfaceShader = surf->material;
			const float* surfaceRegs = surf->shaderRegisters;
			
			inter.surf = surf;
			
			// change the MVP matrix, view/light origin and light projection vectors if needed
			if( surf->space != backEnd.currentSpace )
			{
				backEnd.currentSpace = surf->space;
				
				// turn off the light depth bounds test if this model is rendered with a depth hack
				if( useLightDepthBounds )
				{
					if( !surf->space->weaponDepthHack && surf->space->modelDepthHack == 0.0f )
					{
						if( lightDepthBoundsDisabled )
						{
							GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
							lightDepthBoundsDisabled = false;
						}
					}
					else
					{
						if( !lightDepthBoundsDisabled )
						{
							GL_DepthBoundsTest( 0.0f, 0.0f );
							lightDepthBoundsDisabled = true;
						}
					}
				}
				
				// model-view-projection
				RB_SetMVP( surf->space->mvp );
				
				// RB begin		
				SetVertexParms( RENDERPARM_MODELMATRIX_X, surf->space->modelMatrix.Ptr(), 4 );
				
				// for determining the shadow mapping cascades
				SetVertexParms( RENDERPARM_MODELVIEWMATRIX_X, surf->space->modelViewMatrix.Ptr(), 4 );
				
				idVec4 globalLightOrigin( vLight->globalLightOrigin.x, vLight->globalLightOrigin.y, vLight->globalLightOrigin.z, 1.0f );
				SetVertexParm( RENDERPARM_GLOBALLIGHTORIGIN, globalLightOrigin.ToFloatPtr() );
				// RB end
				
				// tranform the light/view origin into model local space
				idVec4 localLightOrigin( 0.0f );
				idVec4 localViewOrigin( 1.0f );
				surf->space->modelMatrix.InverseTransformPoint( vLight->globalLightOrigin, localLightOrigin.ToVec3() );
				surf->space->modelMatrix.InverseTransformPoint( backEnd.viewDef->GetOrigin(), localViewOrigin.ToVec3() );
				
				// set the local light/view origin
				SetVertexParm( RENDERPARM_LOCALLIGHTORIGIN, localLightOrigin.ToFloatPtr() );
				SetVertexParm( RENDERPARM_LOCALVIEWORIGIN, localViewOrigin.ToFloatPtr() );
				
				// transform the light project into model local space
				ALIGNTYPE16 idPlane lightProjection[4];
				for( int i = 0; i < 4; i++ )
				{
					surf->space->modelMatrix.InverseTransformPlane( vLight->lightProject[ i ], lightProjection[ i ] );
				}
				
				// optionally multiply the local light projection by the light texture matrix
				if( lightStage->texture.hasMatrix )
				{
					RB_BakeTextureMatrixIntoTexgen( lightProjection, lightTextureMatrix );
				}
				
				// set the light projection
				SetVertexParm( RENDERPARM_LIGHTPROJECTION_S, lightProjection[0].ToFloatPtr() );
				SetVertexParm( RENDERPARM_LIGHTPROJECTION_T, lightProjection[1].ToFloatPtr() );
				SetVertexParm( RENDERPARM_LIGHTPROJECTION_Q, lightProjection[2].ToFloatPtr() );
				SetVertexParm( RENDERPARM_LIGHTFALLOFF_S,    lightProjection[3].ToFloatPtr() );
				
				// RB begin
				if( r_useShadowMapping.GetBool() )
				{
					if( vLight->parallel )
					{
						for( int i = 0; i < ( r_shadowMapSplits.GetInteger() + 1 ); i++ )
						{
							idRenderMatrix shadowClipMVP;
							idRenderMatrix::Multiply( backEnd.shadowVP[ i ], surf->space->modelMatrix, shadowClipMVP );

							SetVertexParms( ( renderParm_t )( RENDERPARM_SHADOW_MATRIX_0_X + i * 4 ), shadowClipMVP.Ptr(), 4 );
						}
					}
					else if( vLight->pointLight )
					{
						for( int i = 0; i < 6; i++ )
						{
							idRenderMatrix shadowClipMVP;
							idRenderMatrix::Multiply( backEnd.shadowVP[ i ], surf->space->modelMatrix, shadowClipMVP );

							SetVertexParms( ( renderParm_t )( RENDERPARM_SHADOW_MATRIX_0_X + i * 4 ), shadowClipMVP.Ptr(), 4 );
						}
					}
					else // spot light
					{
						idRenderMatrix shadowClipMVP;
						idRenderMatrix::Multiply( backEnd.shadowVP[ 0 ], surf->space->modelMatrix, shadowClipMVP );

						SetVertexParms( ( renderParm_t )( RENDERPARM_SHADOW_MATRIX_0_X ), shadowClipMVP.Ptr(), 4 );
						
					}
				}
				// RB end
			}
			
			// check for the fast path
			if( surfaceShader->GetFastPathBumpImage() && !r_skipInteractionFastPath.GetBool() )
			{
				RENDERLOG_OPEN_BLOCK( surf->material->GetName() );
				
				// texture 0 will be the per-surface bump map
				GL_BindTexture( INTERACTION_TEXUNIT_BUMP, surfaceShader->GetFastPathBumpImage() );
				
				// texture 3 is the per-surface diffuse map
				GL_BindTexture( INTERACTION_TEXUNIT_DIFFUSE, surfaceShader->GetFastPathDiffuseImage() );
				
				// texture 4 is the per-surface specular map
				GL_BindTexture( INTERACTION_TEXUNIT_SPECULAR, surfaceShader->GetFastPathSpecularImage() );
				
				GL_DrawElementsWithCounters( surf );
				
				RENDERLOG_CLOSE_BLOCK();
				continue;
			}
			
			RENDERLOG_OPEN_BLOCK( surf->material->GetName() );
			
			inter.bumpImage = NULL;
			inter.specularImage = NULL;
			inter.diffuseImage = NULL;
			inter.diffuseColor[0] = inter.diffuseColor[1] = inter.diffuseColor[2] = inter.diffuseColor[3] = 0;
			inter.specularColor[0] = inter.specularColor[1] = inter.specularColor[2] = inter.specularColor[3] = 0;
			
			// go through the individual surface stages
			//
			// This is somewhat arcane because of the old support for video cards that had to render
			// interactions in multiple passes.
			//
			// We also have the very rare case of some materials that have conditional interactions
			// for the "hell writing" that can be shined on them.
			for( int surfaceStageNum = 0; surfaceStageNum < surfaceShader->GetNumStages(); surfaceStageNum++ )
			{
				auto const surfaceStage = surfaceShader->GetStage( surfaceStageNum );
				
				switch( surfaceStage->lighting )
				{
					case SL_COVERAGE:
					{
						// ignore any coverage stages since they should only be used for the depth fill pass
						// for diffuse stages that use alpha test.
						break;
					}
					case SL_AMBIENT:
					{
						// ignore ambient stages while drawing interactions
						break;
					}
					case SL_BUMP:
					{
						// ignore stage that fails the condition
						if( !surfaceRegs[ surfaceStage->conditionRegister ] )
						{
							break;
						}
						// draw any previous interaction
						if( inter.bumpImage != NULL )
						{
							RB_DrawSingleInteraction( &inter );
						}
						inter.bumpImage = surfaceStage->texture.image;
						inter.diffuseImage = NULL;
						inter.specularImage = NULL;
						RB_SetupInteractionStage( surfaceStage, surfaceRegs, NULL,  inter.bumpMatrix, NULL );
						break;
					}
					case SL_DIFFUSE:
					{
						// ignore stage that fails the condition
						if( !surfaceRegs[ surfaceStage->conditionRegister ] )
						{
							break;
						}
						// draw any previous interaction
						if( inter.diffuseImage != NULL )
						{
							RB_DrawSingleInteraction( &inter );
						}
						inter.diffuseImage = surfaceStage->texture.image;
						inter.vertexColor = surfaceStage->vertexColor;
						RB_SetupInteractionStage( surfaceStage, surfaceRegs, diffuseColor.ToFloatPtr(), inter.diffuseMatrix, inter.diffuseColor.ToFloatPtr() );
						break;
					}
					case SL_SPECULAR:
					{
						// ignore stage that fails the condition
						if( !surfaceRegs[ surfaceStage->conditionRegister ] )
						{
							break;
						}
						// draw any previous interaction
						if( inter.specularImage != NULL )
						{
							RB_DrawSingleInteraction( &inter );
						}
						inter.specularImage = surfaceStage->texture.image;
						inter.vertexColor = surfaceStage->vertexColor;
						RB_SetupInteractionStage( surfaceStage, surfaceRegs, specularColor.ToFloatPtr(), inter.specularMatrix, inter.specularColor.ToFloatPtr() );
						break;
					}
				}
			}
			
			// draw the final interaction
			RB_DrawSingleInteraction( &inter );
			
			RENDERLOG_CLOSE_BLOCK();
		}
	}
	
	if( useLightDepthBounds && lightDepthBoundsDisabled )
	{
		GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
	}
	
	renderProgManager.Unbind();
}

// RB begin

/*
=========================================================================================

AMBIENT PASS RENDERING

=========================================================================================
*/

/*
==================
RB_AmbientPass
==================
*/
static void RB_AmbientPass( const drawSurf_t* const* drawSurfs, int numDrawSurfs, bool fillGbuffer )
{
	if( fillGbuffer )
	{
		if( !r_useSSGI.GetBool() && !r_useSSAO.GetBool() )
		{
			return;
		}
	}
	else
	{
		if( r_forceAmbient.GetFloat() <= 0 || r_skipAmbient.GetBool() )
		{
			return;
		}
	}
	
	if( numDrawSurfs == 0 )
	{
		return;
	}
	
	// if we are just doing 2D rendering, no need to fill the depth buffer
	if( backEnd.viewDef->viewEntitys == NULL )
	{
		return;
	}
	
	const bool hdrIsActive = ( r_useHDR.GetBool() && globalFramebuffers.hdrFBO != NULL && globalFramebuffers.hdrFBO->IsBound() );
	
	/*
	if( fillGbuffer )
	{
		globalFramebuffers.geometryBufferFBO->Bind();
	
		glClearColor( 0, 0, 0, 0 );
		glClear( GL_COLOR_BUFFER_BIT );
	}
	*/
	
	RENDERLOG_OPEN_MAINBLOCK( MRB_AMBIENT_PASS );
	RENDERLOG_OPEN_BLOCK( "RB_AmbientPass" );
	
	// RB: not needed
	// GL_StartDepthPass( backEnd.viewDef->scissor );
	
	// force MVP change on first surface
	backEnd.currentSpace = NULL;
	
	// draw all the subview surfaces, which will already be at the start of the sorted list,
	// with the general purpose path
	//GL_State( GLS_DEFAULT );
	
	//if( fillGbuffer )
	{
		GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );
	}
	//else
	//{
	//	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );
	//}
	
	GL_Color( colorWhite );
	
	const float lightScale = 1.0f; //r_lightScale.GetFloat();
	const idRenderVector lightColor = colorWhite * lightScale;
	// apply the world-global overbright and the 2x factor for specular
	const idRenderVector diffuseColor = lightColor;
	const idRenderVector specularColor = lightColor * 2.0f;
	
	idRenderVector ambientColor;
	float ambientBoost = 1.0f;
	ambientBoost += r_useSSAO.GetBool() ? 0.2f : 0.0f;
	ambientBoost *= r_useHDR.GetBool() ? 1.1f : 1.0f;
	ambientColor.x = r_forceAmbient.GetFloat() * ambientBoost;
	ambientColor.y = r_forceAmbient.GetFloat() * ambientBoost;
	ambientColor.z = r_forceAmbient.GetFloat() * ambientBoost;
	ambientColor.w = 1;
	
	renderProgManager.SetRenderParm( RENDERPARM_AMBIENT_COLOR, ambientColor.ToFloatPtr() );
	
	// setup renderparms assuming we will be drawing trivial surfaces first
	RB_SetupForFastPathInteractions( diffuseColor, specularColor );
	
	for( int i = 0; i < numDrawSurfs; ++i )
	{
		auto const drawSurf = drawSurfs[i];
		auto const surfaceMaterial = drawSurf->material;
		
		// translucent surfaces don't put anything in the depth buffer and don't
		// test against it, which makes them fail the mirror clip plane operation
		if( surfaceMaterial->Coverage() == MC_TRANSLUCENT )
		{
			continue;
		}
		
		// get the expressions for conditionals / color / texcoords
		const float* surfaceRegs = drawSurf->shaderRegisters;
		
		// if all stages of a material have been conditioned off, don't do anything
		int stage = 0;
		for( ; stage < surfaceMaterial->GetNumStages(); stage++ )
		{
			auto pStage = surfaceMaterial->GetStage( stage );
			// check the stage enable condition
			if( surfaceRegs[ pStage->conditionRegister ] != 0 )
			{
				break;
			}
		}
		if( stage == surfaceMaterial->GetNumStages() )
		{
			continue;
		}
		
		//bool isWorldModel = ( drawSurf->space->entityDef->parms.origin == vec3_origin );
		
		//if( isWorldModel )
		//{
		//	renderProgManager.BindShader_VertexLighting();
		//}
		//else
		{
#if 1
			if( fillGbuffer )
			{
				// fill geometry buffer with normal/roughness information
				if( drawSurf->jointCache )
				{
					renderProgManager.BindShader_SmallGeometryBufferSkinned();
				}
				else
				{
					renderProgManager.BindShader_SmallGeometryBuffer();
				}
			}
			else
#endif
			{
				// draw Quake 4 style ambient
				if( drawSurf->jointCache )
				{
					renderProgManager.BindShader_AmbientLightingSkinned();
				}
				else
				{
					renderProgManager.BindShader_AmbientLighting();
				}
			}
		}
		
		// change the matrix if needed
		if( drawSurf->space != backEnd.currentSpace )
		{
			backEnd.currentSpace = drawSurf->space;
			
			RB_SetMVP( drawSurf->space->mvp );

			// RB: if we want to store the normals in world space so we need the model -> world matrix
			SetVertexParms( RENDERPARM_MODELMATRIX_X, drawSurf->space->modelMatrix.Ptr(), 4 );

			// RB: if we want to store the normals in camera space so we need the model -> camera matrix
			SetVertexParms( RENDERPARM_MODELVIEWMATRIX_X, drawSurf->space->modelViewMatrix.Ptr(), 4 );
			
			// tranform the view origin into model local space
			idRenderVector localViewOrigin( 1.0f );
			drawSurf->space->modelMatrix.InverseTransformPoint( backEnd.viewDef->GetOrigin(), localViewOrigin.ToVec3() );
			SetVertexParm( RENDERPARM_LOCALVIEWORIGIN, localViewOrigin.ToFloatPtr() );
			
			//if( !isWorldModel )
			//{
			//	// tranform the light direction into model local space
			//	idVec3 globalLightDirection( 0.0f, 0.0f, -1.0f ); // HACK
			//	idVec4 localLightDirection( 0.0f );
			//	R_GlobalVectorToLocal( drawSurf->space->modelMatrix, globalLightDirection, localLightDirection.ToVec3() );
			//
			//	SetVertexParm( RENDERPARM_LOCALLIGHTORIGIN, localLightDirection.ToFloatPtr() );
			//}
		}
		
#if 0
		if( !isWorldModel )
		{
			idVec4 directedColor;
			directedColor.x = drawSurf->space->gridDirectedLight.x;
			directedColor.y = drawSurf->space->gridDirectedLight.y;
			directedColor.z = drawSurf->space->gridDirectedLight.z;
			directedColor.w = 1;
			
			idVec4 ambientColor;
			ambientColor.x = drawSurf->space->gridAmbientLight.x;
			ambientColor.y = drawSurf->space->gridAmbientLight.y;
			ambientColor.z = drawSurf->space->gridAmbientLight.z;
			ambientColor.w = 1;
			
			renderProgManager.SetRenderParm( RENDERPARM_COLOR, directedColor.ToFloatPtr() );
			renderProgManager.SetRenderParm( RENDERPARM_AMBIENT_COLOR, ambientColor.ToFloatPtr() );
		}
#endif
		
		/*
		uint64 surfGLState = 0;
		
		// set polygon offset if necessary
		if( surfaceMaterial->TestMaterialFlag( MF_POLYGONOFFSET ) )
		{
			surfGLState |= GLS_POLYGON_OFFSET;
			GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * surfaceMaterial->GetPolygonOffset() );
		}
		
		// subviews will just down-modulate the color buffer
		idVec4 color;
		if( surfaceMaterial->GetSort() == SS_SUBVIEW )
		{
			surfGLState |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_DEPTHFUNC_LESS;
			color[0] = 1.0f;
			color[1] = 1.0f;
			color[2] = 1.0f;
			color[3] = 1.0f;
		}
		else
		{
			// others just draw black
		#if 0
			color[0] = 0.0f;
			color[1] = 0.0f;
			color[2] = 0.0f;
			color[3] = 1.0f;
		#else
			color = colorWhite;
		#endif
		}
		*/
		
		// check for the fast path
		if( surfaceMaterial->GetFastPathBumpImage() && !r_skipInteractionFastPath.GetBool() )
		{
			RENDERLOG_OPEN_BLOCK( surfaceMaterial->GetName() );
			
			// texture 0 will be the per-surface bump map
			GL_BindTexture( INTERACTION_TEXUNIT_BUMP, surfaceMaterial->GetFastPathBumpImage() );
			
			// texture 3 is the per-surface diffuse map
			GL_BindTexture( INTERACTION_TEXUNIT_DIFFUSE, surfaceMaterial->GetFastPathDiffuseImage() );
			
			// texture 4 is the per-surface specular map
			GL_BindTexture( INTERACTION_TEXUNIT_SPECULAR, surfaceMaterial->GetFastPathSpecularImage() );
			
			GL_DrawElementsWithCounters( drawSurf );
			
			RENDERLOG_CLOSE_BLOCK();
			continue;
		}
		
		RENDERLOG_OPEN_BLOCK( surfaceMaterial->GetName() );
		
		//bool drawSolid = false;
		
		
		// we may have multiple alpha tested stages
		// if the only alpha tested stages are condition register omitted,
		// draw a normal opaque surface
		bool didDraw = false;
		
		drawInteraction_t inter = {};
		inter.surf = drawSurf;
		inter.bumpImage = NULL;
		inter.specularImage = NULL;
		inter.diffuseImage = NULL;
		
		inter.diffuseColor[0] = inter.diffuseColor[1] = inter.diffuseColor[2] = inter.diffuseColor[3] = 1;
		inter.specularColor[0] = inter.specularColor[1] = inter.specularColor[2] = inter.specularColor[3] = 0;
		
		// perforated surfaces may have multiple alpha tested stages
		for( stage = 0; stage < surfaceMaterial->GetNumStages(); stage++ )
		{
			auto surfaceStage = surfaceMaterial->GetStage( stage );
			
			switch( surfaceStage->lighting )
			{
				case SL_COVERAGE:
				{
					// ignore any coverage stages since they should only be used for the depth fill pass
					// for diffuse stages that use alpha test.
					break;
				}
				
				case SL_AMBIENT:
				{
					// ignore ambient stages while drawing interactions
					break;
				}
				
				case SL_BUMP:
				{
					// ignore stage that fails the condition
					if( !surfaceRegs[ surfaceStage->conditionRegister ] )
					{
						break;
					}
					// draw any previous interaction
					if( inter.bumpImage != NULL )
					{
						RB_DrawSingleInteraction( &inter );
					}
					inter.bumpImage = surfaceStage->texture.image;
					inter.diffuseImage = NULL;
					inter.specularImage = NULL;
					RB_SetupInteractionStage( surfaceStage, surfaceRegs, NULL,
											  inter.bumpMatrix, NULL );
					break;
				}
				
				case SL_DIFFUSE:
				{
					// ignore stage that fails the condition
					if( !surfaceRegs[ surfaceStage->conditionRegister ] )
					{
						break;
					}
					
					// draw any previous interaction
					if( inter.diffuseImage != NULL )
					{
						RB_DrawSingleInteraction( &inter );
					}
					
					inter.diffuseImage = surfaceStage->texture.image;
					inter.vertexColor = surfaceStage->vertexColor;
					RB_SetupInteractionStage( surfaceStage, surfaceRegs, diffuseColor.ToFloatPtr(),
											  inter.diffuseMatrix, inter.diffuseColor.ToFloatPtr() );
					break;
				}
				
				case SL_SPECULAR:
				{
					// ignore stage that fails the condition
					if( !surfaceRegs[ surfaceStage->conditionRegister ] )
					{
						break;
					}
					// draw any previous interaction
					if( inter.specularImage != NULL )
					{
						RB_DrawSingleInteraction( &inter );
					}
					inter.specularImage = surfaceStage->texture.image;
					inter.vertexColor = surfaceStage->vertexColor;
					RB_SetupInteractionStage( surfaceStage, surfaceRegs, specularColor.ToFloatPtr(),
											  inter.specularMatrix, inter.specularColor.ToFloatPtr() );
					break;
				}
			}
		}
		
		// draw the final interaction
		RB_DrawSingleInteraction( &inter );
		
		RENDERLOG_CLOSE_BLOCK();
	}
	
	SetFragmentParm( RENDERPARM_ALPHA_TEST, vec4_zero.ToFloatPtr() );
	
	RENDERLOG_CLOSE_BLOCK();
	RENDERLOG_CLOSE_MAINBLOCK();
	
	if( fillGbuffer )
	{
		GL_SelectTexture( 0 );
		
		// FIXME: this copies RGBA16F into _currentNormals if HDR is enabled
		const idScreenRect& viewport = backEnd.viewDef->GetViewport();
		globalImages->currentNormalsImage->CopyFramebuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );
		
		//GL_Clear( true, false, false, STENCIL_SHADOW_TEST_VALUE, 0.0f, 0.0f, 0.0f, 1.0f, false );
		
		
		/*
		if( hdrIsActive )
		{
			globalFramebuffers.hdrFBO->Bind();
		}
		else
		{
			Framebuffer::Unbind();
		}
		*/
	}
	
	// unbind texture units
	GL_ResetTexturesState();
	
	renderProgManager.Unbind();
}

// RB end

/*
==============================================================================================

STENCIL SHADOW RENDERING

==============================================================================================
*/

/*
=====================
RB_StencilShadowPass

The stencil buffer should have been set to 128 on any surfaces that might receive shadows.
=====================
*/
static void RB_StencilShadowPass( const drawSurf_t* const drawSurfs, const viewLight_t* const vLight )
{
	if( r_skipShadows.GetBool() )
	{
		return;
	}
	
	if( drawSurfs == NULL )
	{
		return;
	}
	
	RENDERLOG_PRINTF( "---------- RB_StencilShadowPass ----------\n" );
	
	renderProgManager.BindShader_Shadow();
	
	GL_SelectTexture( 0 );
	globalImages->BindNull();
	
	uint64 glState = 0;
	
	// for visualizing the shadows
	if( r_showShadows.GetInteger() )
	{
		// set the debug shadow color
		SetFragmentParm( RENDERPARM_COLOR, colorMagenta.ToFloatPtr() );
		if( r_showShadows.GetInteger() == 2 )
		{
			// draw filled in
			glState = GLS_DEPTHMASK | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_LESS;
		}
		else {
			// draw as lines, filling the depth buffer
			glState = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_POLYMODE_LINE | GLS_DEPTHFUNC_ALWAYS;
		}
	}
	else {
		// don't write to the color or depth buffer, just the stencil buffer
		glState = GLS_DEPTHMASK | GLS_COLORMASK | GLS_ALPHAMASK | GLS_DEPTHFUNC_LESS;
	}
	
	GL_PolygonOffset( r_shadowPolygonFactor.GetFloat(), -r_shadowPolygonOffset.GetFloat() );
	
	// the actual stencil func will be set in the draw code, but we need to make sure it isn't
	// disabled here, and that the value will get reset for the interactions without looking
	// like a no-change-required
	GL_State( glState | GLS_STENCIL_OP_FAIL_KEEP | GLS_STENCIL_OP_ZFAIL_KEEP | GLS_STENCIL_OP_PASS_INCR |
			  GLS_STENCIL_MAKE_REF( STENCIL_SHADOW_TEST_VALUE ) | GLS_STENCIL_MAKE_MASK( STENCIL_SHADOW_MASK_VALUE ) | GLS_POLYGON_OFFSET );
			  
	// Two Sided Stencil reduces two draw calls to one for slightly faster shadows
	GL_Cull( CT_TWO_SIDED );
	
	
	// process the chain of shadows with the current rendering state
	backEnd.currentSpace = NULL;
	
	for( const drawSurf_t* drawSurf = drawSurfs; drawSurf != NULL; drawSurf = drawSurf->nextOnLight )
	{
		if( drawSurf->scissorRect.IsEmpty() )
		{
			continue;	// !@# FIXME: find out why this is sometimes being hit!
			// temporarily jump over the scissor and draw so the gl error callback doesn't get hit
		}
		
		// make sure the shadow volume is done
		if( drawSurf->shadowVolumeState != SHADOWVOLUME_DONE )
		{
			assert( drawSurf->shadowVolumeState == SHADOWVOLUME_UNFINISHED || drawSurf->shadowVolumeState == SHADOWVOLUME_DONE );
			
			uint64 start = Sys_Microseconds();
			while( drawSurf->shadowVolumeState == SHADOWVOLUME_UNFINISHED )
			{
				Sys_Yield();
			}
			uint64 end = Sys_Microseconds();
			
			backEnd.pc.shadowMicroSec += end - start;
		}
		
		if( drawSurf->numIndexes == 0 )
		{
			continue;	// a job may have created an empty shadow volume
		}
		
		RB_SetScissor( drawSurf->scissorRect );
		
		if( drawSurf->space != backEnd.currentSpace )
		{
			// change the matrix
			RB_SetMVP( drawSurf->space->mvp );

			// set the local light position to allow the vertex program to project the shadow volume end cap to infinity
			idRenderVector localLight( 0.0f );
			drawSurf->space->modelMatrix.InverseTransformPoint( vLight->globalLightOrigin, localLight.ToVec3() );
			SetVertexParm( RENDERPARM_LOCALLIGHTORIGIN, localLight.ToFloatPtr() );
			
			backEnd.currentSpace = drawSurf->space;
		}
		
		if( r_showShadows.GetInteger() == 0 )
		{
			if( drawSurf->jointCache )
			{
				renderProgManager.BindShader_ShadowSkinned();
			}
			else
			{
				renderProgManager.BindShader_Shadow();
			}
		}
		else
		{
			if( drawSurf->jointCache )
			{
				renderProgManager.BindShader_ShadowDebugSkinned();
			}
			else
			{
				renderProgManager.BindShader_ShadowDebug();
			}
		}
		
		// set depth bounds per shadow
		if( r_useShadowDepthBounds.GetBool() )
		{
			GL_DepthBoundsTest( drawSurf->scissorRect.zmin, drawSurf->scissorRect.zmax );
		}
		
		// Determine whether or not the shadow volume needs to be rendered with Z-pass or
		// Z-fail. It is worthwhile to spend significant resources to reduce the number of
		// cases where shadow volumes need to be rendered with Z-fail because Z-fail
		// rendering can be significantly slower even on today's hardware. For instance,
		// on NVIDIA hardware Z-fail rendering causes the Z-Cull to be used in reverse:
		// Z-near becomes Z-far (trivial accept becomes trivial reject). Using the Z-Cull
		// in reverse is far less efficient because the Z-Cull only stores Z-near per 16x16
		// pixels while the Z-far is stored per 4x2 pixels. (The Z-near coallesce buffer
		// which has 4x4 granularity is only used when updating the depth which is not the
		// case for shadow volumes.) Note that it is also important to NOT use a Z-Cull
		// reconstruct because that would clear the Z-near of the Z-Cull which results in
		// no trivial rejection for Z-fail stencil shadow rendering.
		
		const bool renderZPass = ( drawSurf->renderZFail == 0 ) || r_forceZPassStencilShadows.GetBool();
		
		if( renderZPass )
		{
			glStencilOpSeparate( GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR );
			glStencilOpSeparate( GL_BACK, GL_KEEP, GL_KEEP, GL_DECR );
			///glStencilOpSeparateATI( backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK, GL_KEEP, GL_KEEP, tr.stencilIncr );
			///qglStencilOpSeparateATI( backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT, GL_KEEP, GL_KEEP, tr.stencilDecr );
		}
		else if( r_useStencilShadowPreload.GetBool() )
		{
			// preload + Z-pass
			glStencilOpSeparate( GL_FRONT, GL_KEEP, GL_DECR, GL_DECR );
			glStencilOpSeparate( GL_BACK, GL_KEEP, GL_INCR, GL_INCR );
		}
		else
		{	// Z-fail
			glStencilOpSeparate( GL_FRONT, GL_KEEP, GL_DECR, GL_KEEP );
			glStencilOpSeparate( GL_BACK, GL_KEEP, GL_INCR, GL_KEEP );
			///glStencilOpSeparate( backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK, GL_KEEP, GL_DECR_WRAP, GL_KEEP );
			///glStencilOpSeparate( backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT, GL_KEEP, GL_INCR_WRAP, GL_KEEP );
		}

		// get vertex buffer --------------------------------
		
		idVertexBuffer vertexBuffer;
		vertexCache.GetVertexBuffer( drawSurf->shadowCache, &vertexBuffer );
		const GLint vertOffset = vertexBuffer.GetOffset();
		const GLuint vbo = reinterpret_cast<GLuint>( vertexBuffer.GetAPIObject() );

		// get index buffer --------------------------------

		idIndexBuffer indexBuffer;
		vertexCache.GetIndexBuffer( drawSurf->indexCache, &indexBuffer );
		const GLintptr indexOffset = indexBuffer.GetOffset();
		const GLuint ibo = reinterpret_cast<GLuint>( indexBuffer.GetAPIObject() );

		RENDERLOG_PRINTF( "Binding Buffers: %p %p\n", vertexBuffer, indexBuffer );

		const GLsizei indexCount = r_singleTriangle.GetBool() ? 3 : drawSurf->numIndexes;
		
		// RB: 64 bit fixes, changed GLuint to GLintptr
		if( backEnd.glState.currentIndexBuffer != ( GLintptr )ibo || !r_useStateCaching.GetBool() )
		{
			glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo );
			backEnd.glState.currentIndexBuffer = ( GLintptr )ibo;
		}
		
		if( drawSurf->jointCache )
		{
			assert( renderProgManager.ShaderUsesJoints() );
			
			idJointBuffer jointBuffer;
			if( !vertexCache.GetJointBuffer( drawSurf->jointCache, &jointBuffer ) )
			{
				idLib::Warning( "GL_DrawElementsWithCounters, jointBuffer == NULL" );
				continue;
			}
			assert( ( jointBuffer.GetOffset() & ( glConfig.uniformBufferOffsetAlignment - 1 ) ) == 0 );
			
			const GLintptr ubo = reinterpret_cast< GLintptr >( jointBuffer.GetAPIObject() );
			glBindBufferRange( GL_UNIFORM_BUFFER, BINDING_MATRICES_UBO, ubo, jointBuffer.GetOffset(), jointBuffer.GetNumJoints() * sizeof( idJointMat ) );
			
			if( ( backEnd.glState.vertexLayout != LAYOUT_DRAW_SHADOW_VERT_SKINNED ) || ( backEnd.glState.currentVertexBuffer != ( GLintptr )vbo ) || !r_useStateCaching.GetBool() )
			{
				glBindBuffer( GL_ARRAY_BUFFER, vbo );
				backEnd.glState.currentVertexBuffer = ( GLintptr )vbo;
				
				glEnableVertexAttribArray( PC_ATTRIB_INDEX_VERTEX );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
				glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
				glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_ST );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );
				
			#if defined(USE_GLES2) || defined(USE_GLES3)
				glVertexAttribPointer( PC_ATTRIB_INDEX_VERTEX, 4, GL_FLOAT, GL_FALSE, sizeof( idShadowVertSkinned ), ( void* )( vertOffset + SHADOWVERTSKINNED_XYZW_OFFSET ) );
				glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idShadowVertSkinned ), ( void* )( vertOffset + SHADOWVERTSKINNED_COLOR_OFFSET ) );
				glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idShadowVertSkinned ), ( void* )( vertOffset + SHADOWVERTSKINNED_COLOR2_OFFSET ) );
			#else
				glVertexAttribPointer( PC_ATTRIB_INDEX_VERTEX, 4, GL_FLOAT, GL_FALSE, sizeof( idShadowVertSkinned ), idShadowVertSkinned::xyzwOffset );
				glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idShadowVertSkinned ), idShadowVertSkinned::colorOffset );
				glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idShadowVertSkinned ), idShadowVertSkinned::color2Offset );
			#endif
				
				backEnd.glState.vertexLayout = LAYOUT_DRAW_SHADOW_VERT_SKINNED;
			}
			
		}
		else
		{
			if( ( backEnd.glState.vertexLayout != LAYOUT_DRAW_SHADOW_VERT ) || ( backEnd.glState.currentVertexBuffer != ( GLintptr )vbo ) || !r_useStateCaching.GetBool() )
			{
				glBindBuffer( GL_ARRAY_BUFFER, vbo );
				backEnd.glState.currentVertexBuffer = ( GLintptr )vbo;
				
				glEnableVertexAttribArray( PC_ATTRIB_INDEX_VERTEX );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_ST );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );
				
			#if defined(USE_GLES2) || defined(USE_GLES3)
				glVertexAttribPointer( PC_ATTRIB_INDEX_VERTEX, 4, GL_FLOAT, GL_FALSE, sizeof( idShadowVert ), ( void* )( vertOffset + SHADOWVERT_XYZW_OFFSET ) );
			#else
				glVertexAttribPointer( PC_ATTRIB_INDEX_VERTEX, 4, GL_FLOAT, GL_FALSE, sizeof( idShadowVert ), idShadowVert::xyzwOffset );
			#endif
				
				backEnd.glState.vertexLayout = LAYOUT_DRAW_SHADOW_VERT;
			}
		}
		// RB end
		
		renderProgManager.CommitUniforms();
		
		if( drawSurf->jointCache )
		{
		#if defined(USE_GLES3)
			glDrawElements( GL_TRIANGLES, indexCount, GL_INDEX_TYPE, ( triIndex_t* )indexOffset );
		#else
			glDrawElementsBaseVertex( GL_TRIANGLES, indexCount, GL_INDEX_TYPE, ( triIndex_t* )indexOffset, vertOffset / sizeof( idShadowVertSkinned ) );
		#endif
		}
		else
		{
		#if defined(USE_GLES3)
			glDrawElements( GL_TRIANGLES, indexCount, GL_INDEX_TYPE, ( triIndex_t* )indexOffset );
		#else
			glDrawElementsBaseVertex( GL_TRIANGLES, indexCount, GL_INDEX_TYPE, ( triIndex_t* )indexOffset, vertOffset / sizeof( idShadowVert ) );
		#endif
		}
		
		// RB: added stats
		backEnd.pc.c_shadowElements++;
		backEnd.pc.c_shadowIndexes += drawSurf->numIndexes;
		// RB end
		
		if( !renderZPass && r_useStencilShadowPreload.GetBool() )
		{
			// render again with Z-pass
			glStencilOpSeparate( GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR );
			glStencilOpSeparate( GL_BACK, GL_KEEP, GL_KEEP, GL_DECR );
			
			if( drawSurf->jointCache )
			{
			#if defined(USE_GLES3)
				glDrawElements( GL_TRIANGLES, indexCount, GL_INDEX_TYPE, ( triIndex_t* )indexOffset );
			#else
				glDrawElementsBaseVertex( GL_TRIANGLES, indexCount, GL_INDEX_TYPE, ( triIndex_t* )indexOffset, vertOffset / sizeof( idShadowVertSkinned ) );
			#endif
			}
			else
			{
			#if defined(USE_GLES3)
				glDrawElements( GL_TRIANGLES, indexCount, GL_INDEX_TYPE, ( triIndex_t* )indexOffset );
			#else
				glDrawElementsBaseVertex( GL_TRIANGLES, indexCount, GL_INDEX_TYPE, ( triIndex_t* )indexOffset, vertOffset / sizeof( idShadowVert ) );
			#endif
			}
			
			// RB: added stats
			backEnd.pc.c_shadowElements++;
			backEnd.pc.c_shadowIndexes += drawSurf->numIndexes;
			// RB end
		}
	}
	
	// cleanup the shadow specific rendering state
	
	GL_Cull( CT_FRONT_SIDED );
	
	// reset depth bounds
	if( r_useShadowDepthBounds.GetBool() )
	{
		if( r_useLightDepthBounds.GetBool() )
		{
			GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
		}
		else {
			GL_DepthBoundsTest( 0.0f, 0.0f );
		}
	}
}

/*
==================
RB_StencilSelectLight

	Deform the zeroOneCubeModel to exactly cover the light volume. Render the deformed cube model to the stencil buffer in
	such a way that only fragments that are directly visible and contained within the volume will be written creating a
	mask to be used by the following stencil shadow and draw interaction passes.
==================
*/
static void RB_StencilSelectLight( const viewLight_t* const vLight )
{
	RENDERLOG_OPEN_BLOCK( "Stencil Select" );
	
	// enable the light scissor
	RB_SetScissor( vLight->scissorRect );
	
	// clear stencil buffer to 0 (not drawable)
	uint64 glStateMinusStencil = GL_GetCurrentStateMinusStencil();
	GL_State( glStateMinusStencil | GLS_STENCIL_FUNC_ALWAYS | GLS_STENCIL_MAKE_REF( STENCIL_SHADOW_TEST_VALUE ) | GLS_STENCIL_MAKE_MASK( STENCIL_SHADOW_MASK_VALUE ) );	// make sure stencil mask passes for the clear
	GL_Clear( false, false, true, 0, 0.0f, 0.0f, 0.0f, 0.0f, false );	// clear to 0 for stencil select
	
	// set the depthbounds
	GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
	
	
	GL_State( GLS_COLORMASK | GLS_ALPHAMASK | GLS_DEPTHMASK | GLS_DEPTHFUNC_LESS | GLS_STENCIL_FUNC_ALWAYS | GLS_STENCIL_MAKE_REF( STENCIL_SHADOW_TEST_VALUE ) | GLS_STENCIL_MAKE_MASK( STENCIL_SHADOW_MASK_VALUE ) );
	GL_Cull( CT_TWO_SIDED );
	
	renderProgManager.BindShader_Depth();
	
	// set the matrix for deforming the 'zeroOneCubeModel' into the frustum to exactly cover the light volume
	idRenderMatrix invProjectMVPMatrix;
	idRenderMatrix::Multiply( backEnd.viewDef->GetMVPMatrix(), vLight->inverseBaseLightProject, invProjectMVPMatrix );
	RB_SetMVP( invProjectMVPMatrix );
	
	// two-sided stencil test
	glStencilOpSeparate( GL_FRONT, GL_KEEP, GL_REPLACE, GL_ZERO );
	glStencilOpSeparate( GL_BACK, GL_KEEP, GL_ZERO, GL_REPLACE );
	
	GL_DrawElementsWithCounters( &backEnd.zeroOneCubeSurface );
	
	// reset stencil state
	
	GL_Cull( CT_FRONT_SIDED );
	
	renderProgManager.Unbind();
	
	
	// unset the depthbounds
	GL_DepthBoundsTest( 0.0f, 0.0f );
	
	RENDERLOG_CLOSE_BLOCK();
}

/*
==============================================================================================

SHADOW MAPS RENDERING

	idRenderMatrix modelToShadowMatrix;
	idRenderMatrix::Multiply( backEnd.shadowV[0], surf->space->modelMatrix, modelToShadowMatrix );

	idRenderMatrix shadowClipMVP;
	idRenderMatrix::Multiply( backEnd.shadowP[0], modelToShadowMatrix, shadowClipMVP );

	idRenderMatrix shadowWindowMVP;
	idRenderMatrix::Multiply( renderMatrix_clipSpaceToWindowSpace, shadowClipMVP, shadowWindowMVP );

	idBounds tmp;
	idRenderMatrix::ProjectedBounds( tmp, vLight->inverseBaseLightProject, bounds_zeroOneCube, false );
	idRenderMatrix::ProjectedBounds( lightBounds, lightViewRenderMatrix, tmp, false );

	idBounds tmp;
	idRenderMatrix::ProjectedBounds( tmp, splitFrustumInverse, bounds_unitCube, false );
	idRenderMatrix::ProjectedBounds( cropBounds, lightViewRenderMatrix, tmp, false );

==============================================================================================
*/

/*
=====================
 idRenderMatrix is left-handed !
 
 D3DXMatrixOrthoOffCenterLH
 https://msdn.microsoft.com/en-us/library/bb205347(v=vs.85).aspx
 
 Builds a customized, left-handed orthographic projection matrix.
=====================
*/
static void idRenderMatrix_CreateOrthogonalOffCenterProjection(
	float left, 	// Minimum x-value of view volume.	
	float right, 	// Maximum x-value of view volume.	
	float bottom,	// Minimum y-value of view volume.		
	float top,		// Maximum y-value of view volume. 		
	float zNear, float zFar,
	idRenderMatrix & out )
{
	out[ 0 ][ 0 ] = 2.0f / ( right - left );
	out[ 0 ][ 1 ] = 0.0f;
	out[ 0 ][ 2 ] = 0.0f;
	//out[ 0 ][ 3 ] = ( left + right ) / ( left - right );
	out[ 0 ][ 3 ] = -( left + right ) / ( right - left );

	out[ 1 ][ 0 ] = 0.0f;
	out[ 1 ][ 1 ] = 2.0f / ( top - bottom );
	out[ 1 ][ 2 ] = 0.0f;
	//out[ 1 ][ 3 ] = ( top + bottom ) / ( bottom - top );
	out[ 1 ][ 3 ] = -( top + bottom ) / ( top - bottom );

	out[ 2 ][ 0 ] = 0.0f;
	out[ 2 ][ 1 ] = 0.0f;
	out[ 2 ][ 2 ] = 1.0f / ( zFar - zNear );
	out[ 2 ][ 3 ] = zNear / ( zNear - zFar );

	out[ 3 ][ 0 ] = 0.0f;
	out[ 3 ][ 1 ] = 0.0f;
	out[ 3 ][ 2 ] = 0.0f;
	out[ 3 ][ 3 ] = 1.0f;
}

/*
=====================
RB_ShadowMapPass
=====================
*/
static void RB_ShadowMapPass( const drawSurf_t * const drawSurfs, const viewLight_t * const vLight, int side )
{
	if( r_skipShadows.GetBool() )
	{
		return;
	}
	
	if( drawSurfs == NULL ) 
	{
		return;
	}
	
	RENDERLOG_PRINTF( "---------- RB_ShadowMapPass( side = %i ) ----------\n", side );
	
	renderProgManager.BindShader_Depth();
	
	GL_SelectTexture( 0 );
	globalImages->BindNull();
	
	uint64 glState = 0;
	
	// the actual stencil func will be set in the draw code, but we need to make sure it isn't
	// disabled here, and that the value will get reset for the interactions without looking
	// like a no-change-required
	GL_State( glState | GLS_POLYGON_OFFSET );
	
	switch( r_shadowMapOccluderFacing.GetInteger() )
	{
		case 0:
			GL_Cull( CT_FRONT_SIDED );
			GL_PolygonOffset( r_shadowMapPolygonFactor.GetFloat(), r_shadowMapPolygonOffset.GetFloat() );
			break;
			
		case 1:
			GL_Cull( CT_BACK_SIDED );
			GL_PolygonOffset( -r_shadowMapPolygonFactor.GetFloat(), -r_shadowMapPolygonOffset.GetFloat() );
			break;
			
		default:
			GL_Cull( CT_TWO_SIDED );
			GL_PolygonOffset( r_shadowMapPolygonFactor.GetFloat(), r_shadowMapPolygonOffset.GetFloat() );
			break;
	}
	
	idRenderMatrix lightViewRenderMatrix, lightProjectionRenderMatrix;
		
	if( vLight->parallel && side >= 0 )
	{
		assert( side >= 0 && side < 6 );
		
		// original light direction is from surface to light origin
		idVec3 lightDir = -vLight->lightCenter;
		if( lightDir.Normalize() == 0.0f )
		{
			lightDir[2] = -1.0f;
		}		

		idRenderMatrix::CreateViewMatrix( backEnd.viewDef->GetOrigin(), lightDir.ToMat3(), lightViewRenderMatrix );

		idBounds lightBounds;
		{
			idRenderMatrix transform;
			idRenderMatrix::Multiply( lightViewRenderMatrix, vLight->inverseBaseLightProject, transform );
			idRenderMatrix::ProjectedBounds( lightBounds, transform, bounds_zeroOneCube, false );
		}
		///idRenderMatrix_CreateOrthogonalOffCenterProjection(
		///	lightBounds[ 0 ][ 0 ], lightBounds[ 1 ][ 0 ], lightBounds[ 0 ][ 1 ], lightBounds[ 1 ][ 1 ], -lightBounds[ 1 ][ 2 ], -lightBounds[ 0 ][ 2 ],
		///	lightProjectionRenderMatrix );
		
		// 'frustumMVP' goes from global space -> camera local space -> camera projective space
		// invert the MVP projection so we can deform zero-to-one cubes into the frustum pyramid shape and calculate global bounds
		
		const idRenderMatrix & splitFrustumInverse = backEnd.viewDef->GetSplitFrustumInvMVPMatrices()[ FRUSTUM_CASCADE1 + side ];

		idBounds cropBounds;
		{
			idRenderMatrix transform;
			idRenderMatrix::Multiply( lightViewRenderMatrix, splitFrustumInverse, transform );
			idRenderMatrix::ProjectedBounds( cropBounds, transform, bounds_unitCube, false );
		}
		// don't let the frustum AABB be bigger than the light AABB
		if( cropBounds[0][0] < lightBounds[0][0] ) cropBounds[0][0] = lightBounds[0][0];
		if( cropBounds[0][1] < lightBounds[0][1] ) cropBounds[0][1] = lightBounds[0][1];
		if( cropBounds[1][0] > lightBounds[1][0] ) cropBounds[1][0] = lightBounds[1][0];
		if( cropBounds[1][1] > lightBounds[1][1] ) cropBounds[1][1] = lightBounds[1][1];
		
		cropBounds[0][2] = lightBounds[0][2];
		cropBounds[1][2] = lightBounds[1][2];
	#if 1
		idRenderMatrix_CreateOrthogonalOffCenterProjection(
			cropBounds[ 0 ][ 0 ], cropBounds[ 1 ][ 0 ], cropBounds[ 0 ][ 1 ], cropBounds[ 1 ][ 1 ], cropBounds[ 1 ][ 2 ], cropBounds[ 0 ][ 2 ],
			lightProjectionRenderMatrix );
	#else
		idRenderMatrix::CreateOrthogonalOffCenterProjection(
			cropBounds[ 0 ][ 0 ], cropBounds[ 1 ][ 0 ], cropBounds[ 0 ][ 1 ], cropBounds[ 1 ][ 1 ], cropBounds[ 1 ][ 2 ], cropBounds[ 0 ][ 2 ],
			lightProjectionRenderMatrix );
	#endif
		idRenderMatrix::Multiply( lightProjectionRenderMatrix, lightViewRenderMatrix, backEnd.shadowVP[ side ] );
	}
	else if( vLight->pointLight && side >= 0 )
	{
		assert( side >= 0 && side < 6 );

		static const idMat3 SM_VAxis[ 6 ] = {
			idMat3( 1, 0, 0, 0, 0, 1, 0,-1, 0 ),
			idMat3(-1, 0, 0, 0, 0,-1, 0,-1, 0 ),
			idMat3( 0, 1, 0,-1, 0, 0, 0, 0, 1 ),
			idMat3( 0,-1, 0,-1, 0, 0, 0, 0,-1 ),
			idMat3( 0, 0, 1,-1, 0, 0, 0,-1, 0 ),
			idMat3( 0, 0,-1, 1, 0, 0, 0,-1, 0 )
		};
		idRenderMatrix::CreateViewMatrix( vLight->globalLightOrigin, SM_VAxis[ side ], lightViewRenderMatrix );

		// set up 90 degree projection matrix
		const float zNear = 4.0f;
		const float	fov = r_shadowMapFrustumFOV.GetFloat();

		idRenderMatrix::CreateProjectionMatrixFov( fov, fov, zNear, 0.0f, 0.0f, 0.0f, lightProjectionRenderMatrix );	
		idRenderMatrix::Multiply( lightProjectionRenderMatrix, lightViewRenderMatrix, backEnd.shadowVP[ side ] );
	}
	else
	{
		lightViewRenderMatrix.Identity();
		idRenderMatrix::Multiply( renderMatrix_windowSpaceToClipSpace, vLight->baseLightProject, lightProjectionRenderMatrix );
		idRenderMatrix::Multiply( lightProjectionRenderMatrix, lightViewRenderMatrix, backEnd.shadowVP[ 0 ] );
	}
	
	// ------------------------------------------------------

	///const int32 shadowLOD = backEnd.viewDef->isSubview ? ( Max( vLight->shadowLOD + 1, MAX_SHADOWMAP_RESOLUTIONS-1 ) ) : vLight->shadowLOD ;
	const int32 shadowLOD = backEnd.viewDef->isSubview ? ( MAX_SHADOWMAP_RESOLUTIONS - 1 ) : vLight->shadowLOD;
	
	globalFramebuffers.shadowFBO[ 0 ]->Bind(); //SEA: shadowLOD no need for multiple buffers !

	if( side < 0 ) {
		globalFramebuffers.shadowFBO[ shadowLOD ]->AttachImageDepthLayer( globalImages->shadowImage[ shadowLOD ], 0 );
	} else {
		globalFramebuffers.shadowFBO[ shadowLOD ]->AttachImageDepthLayer( globalImages->shadowImage[ shadowLOD ], side );
	}
	
	///globalFramebuffers.shadowFBO[vLight->shadowLOD]->Check(); //SEA: this stalls the pipe
	
	GL_ViewportAndScissor( 0, 0, shadowMapResolutions[ shadowLOD ], shadowMapResolutions[ shadowLOD ] );
	GL_ClearDepth();
	
	// process the chain of shadows with the current rendering state
	backEnd.currentSpace = NULL;
	
	for( const drawSurf_t* drawSurf = drawSurfs; drawSurf != NULL; drawSurf = drawSurf->nextOnLight )
	{	
#if 1
		// make sure the shadow occluder geometry is done
		if( drawSurf->shadowVolumeState != SHADOWVOLUME_DONE )
		{
			assert( drawSurf->shadowVolumeState == SHADOWVOLUME_UNFINISHED || drawSurf->shadowVolumeState == SHADOWVOLUME_DONE );
			
			uint64 start = Sys_Microseconds();
			while( drawSurf->shadowVolumeState == SHADOWVOLUME_UNFINISHED )
			{
				Sys_Yield();
			}
			uint64 end = Sys_Microseconds();
			
			backEnd.pc.shadowMicroSec += end - start;
		}
#endif		
		if( drawSurf->numIndexes == 0 )
		{
			continue;	// a job may have created an empty shadow geometry
		}
		
		if( drawSurf->space != backEnd.currentSpace )
		{
		#if 0
			///idRenderMatrix smLightVP;
			///idRenderMatrix::Multiply( lightProjectionRenderMatrix, lightViewRenderMatrix, smLightVP );
			///
			///idRenderMatrix clipMVP;
			///idRenderMatrix::Multiply( smLightVP, drawSurf->space->modelMatrix, clipMVP );
		
			if( vLight->parallel )
			{
				///idRenderMatrix MVP;
				///idRenderMatrix::Multiply( renderMatrix_clipSpaceToWindowSpace, clipMVP, MVP );

				idRenderMatrix clipMVP;
				idRenderMatrix::Multiply( backEnd.shadowVP[ side ], drawSurf->space->modelMatrix, clipMVP );

				RB_SetMVP( clipMVP );
			}
			else if( side < 0 ) // spot
			{
				idRenderMatrix clipMVP;
				idRenderMatrix::Multiply( backEnd.shadowVP[ 0 ], drawSurf->space->modelMatrix, clipMVP );

				/// from OpenGL view space to OpenGL NDC ( -1 : 1 in XYZ )
				///idRenderMatrix MVP;
				///idRenderMatrix::Multiply( renderMatrix_windowSpaceToClipSpace, clipMVP, MVP );		
				///RB_SetMVP( MVP );

				RB_SetMVP( clipMVP );
			}
			else
			{
				idRenderMatrix clipMVP;
				idRenderMatrix::Multiply( backEnd.shadowVP[ side ], drawSurf->space->modelMatrix, clipMVP );

				RB_SetMVP( clipMVP );
			}
		#endif
			idRenderMatrix clipMVP;

			const idRenderMatrix & smVPMatrix = backEnd.shadowVP[ ( side < 0 )? 0 : side ];
			idRenderMatrix::Multiply( smVPMatrix, drawSurf->space->modelMatrix, clipMVP );

			RB_SetMVP( clipMVP );
			
			// set the local light position to allow the vertex program to project the shadow volume end cap to infinity
			/*
			idRenderVector localLight( 0.0f );
			R_GlobalPointToLocal( drawSurf->space->modelMatrix, vLight->globalLightOrigin, localLight.ToVec3() );
			SetVertexParm( RENDERPARM_LOCALLIGHTORIGIN, localLight.ToFloatPtr() );
			*/
			
			backEnd.currentSpace = drawSurf->space;
		}
		
		bool didDraw = false;
		
		const idMaterial* shader = drawSurf->material;
		
		// get the expressions for conditionals / color / texcoords
		const float* regs = drawSurf->shaderRegisters;
		idRenderVector color( 0, 0, 0, 1 );
		
		uint64 surfGLState = 0;
		
		// set polygon offset if necessary
		if( shader && shader->TestMaterialFlag( MF_POLYGONOFFSET ) )
		{
			surfGLState |= GLS_POLYGON_OFFSET;
			GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
		}
		
#if 1
		if( shader && shader->Coverage() == MC_PERFORATED )
		{
			// perforated surfaces may have multiple alpha tested stages
			for( int stage = 0; stage < shader->GetNumStages(); stage++ )
			{
				auto pStage = shader->GetStage( stage );
				
				if( !pStage->hasAlphaTest )
				{
					continue;
				}
				
				// check the stage enable condition
				if( regs[ pStage->conditionRegister ] == 0 )
				{
					continue;
				}
				
				// if we at least tried to draw an alpha tested stage,
				// we won't draw the opaque surface
				didDraw = true;
				
				// set the alpha modulate
				color[3] = regs[ pStage->color.registers[3] ];
				
				// skip the entire stage if alpha would be black
				if( color[3] <= 0.0f )
				{
					continue;
				}
				
				uint64 stageGLState = surfGLState;
				
				// set privatePolygonOffset if necessary
				if( pStage->privatePolygonOffset )
				{
					GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * pStage->privatePolygonOffset );
					stageGLState |= GLS_POLYGON_OFFSET;
				}
				
				GL_Color( color );
				
				GL_State( stageGLState );
				idRenderVector alphaTestValue( regs[ pStage->alphaTestRegister ] );
				SetFragmentParm( RENDERPARM_ALPHA_TEST, alphaTestValue.ToFloatPtr() );
				
				if( drawSurf->jointCache )
				{
					renderProgManager.BindShader_TextureVertexColorSkinned();
				}
				else
				{
					renderProgManager.BindShader_TextureVertexColor();
				}
				
				RB_SetVertexColorParms( SVC_IGNORE );
				
				// bind the texture
				GL_BindTexture( 0, pStage->texture.image );
				
				// set texture matrix and texGens
				RB_PrepareStageTexturing( pStage, drawSurf );
				
				// must render with less-equal for Z-Cull to work properly
				assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );
				
				// draw it
				GL_DrawElementsWithCounters( drawSurf );
				
				// clean up
				RB_FinishStageTexturing( pStage, drawSurf );
				
				// unset privatePolygonOffset if necessary
				if( pStage->privatePolygonOffset )
				{
					GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
				}
			}
		}
#endif
		
		if( !didDraw )
		{
			if( drawSurf->jointCache )
			{
				renderProgManager.BindShader_DepthSkinned();
			}
			else
			{
				renderProgManager.BindShader_Depth();
			}
			
			GL_DrawElementsWithCounters( drawSurf );
		}
	}
	
	// cleanup the shadow specific rendering state
	if( r_useHDR.GetBool() ) //&& !backEnd.viewDef->is2Dgui )
	{
		globalFramebuffers.hdrFBO->Bind();
	}
	else
	{
		Framebuffer::Unbind();
	}
	renderProgManager.Unbind();
	
	GL_State( GLS_DEFAULT );
	GL_Cull( CT_FRONT_SIDED );
	
	SetFragmentParm( RENDERPARM_ALPHA_TEST, vec4_zero.ToFloatPtr() );
}

/*
==============================================================================================

DRAW INTERACTIONS

==============================================================================================
*/
/*
==================
RB_DrawInteractions
==================
*/
static void RB_DrawInteractions( const idRenderView * const viewDef )
{
	if( r_skipInteractions.GetBool() )
	{
		return;
	}
	
	RENDERLOG_OPEN_MAINBLOCK( MRB_DRAW_INTERACTIONS );
	RENDERLOG_OPEN_BLOCK( "RB_DrawInteractions" );
	
	GL_SelectTexture( 0 );
		
	const bool useLightDepthBounds = r_useLightDepthBounds.GetBool() && !r_useShadowMapping.GetBool();
	
	//
	// for each light, perform shadowing and adding
	//
	for( const viewLight_t* vLight = backEnd.viewDef->viewLights; vLight != NULL; vLight = vLight->next )
	{
		// do fogging later
		if( vLight->lightShader->IsFogLight() )
		{
			continue;
		}
		if( vLight->lightShader->IsBlendLight() )
		{
			continue;
		}
		
		if( vLight->localInteractions == NULL && vLight->globalInteractions == NULL && vLight->translucentInteractions == NULL )
		{
			continue;
		}
		
		const idMaterial* const lightShader = vLight->lightShader;
		RENDERLOG_OPEN_BLOCK( lightShader->GetName() );
		
		// set the depth bounds for the whole light
		if( useLightDepthBounds )
		{
			GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
		}
		
		// RB: shadow mapping
		if( r_useShadowMapping.GetBool() )
		{
			int	side, sideStop;
			
			if( vLight->parallel )
			{
				side = 0;
				sideStop = r_shadowMapSplits.GetInteger() + 1;
			}
			else if( vLight->pointLight )
			{
				if( r_shadowMapSingleSide.GetInteger() != -1 )
				{
					side = r_shadowMapSingleSide.GetInteger();
					sideStop = side + 1;
				}
				else {
					side = 0;
					sideStop = 6;
				}
			}
			else {
				side = -1;
				sideStop = 0;
			}
			
			for( ; side < sideStop ; side++ )
			{
				RB_ShadowMapPass( vLight->globalShadows, vLight, side );
			}
			
			// go back from light view to default camera view
			RB_ResetViewportAndScissorToDefaultCamera( viewDef );
			
			if( vLight->localInteractions != NULL )
			{
				RENDERLOG_OPEN_BLOCK( "Local Light Interactions" );
				RB_RenderInteractions( vLight->localInteractions, vLight, GLS_DEPTHFUNC_EQUAL, false, useLightDepthBounds );
				RENDERLOG_CLOSE_BLOCK();
			}
			
			if( vLight->globalInteractions != NULL )
			{
				RENDERLOG_OPEN_BLOCK( "Global Light Interactions" );
				RB_RenderInteractions( vLight->globalInteractions, vLight, GLS_DEPTHFUNC_EQUAL, false, useLightDepthBounds );
				RENDERLOG_CLOSE_BLOCK();
			}
		}
		else {
			// only need to clear the stencil buffer and perform stencil testing if there are shadows
			const bool performStencilTest = ( vLight->globalShadows != NULL || vLight->localShadows != NULL ) && !r_useShadowMapping.GetBool();
			
			// mirror flips the sense of the stencil select, and I don't want to risk accidentally breaking it
			// in the normal case, so simply disable the stencil select in the mirror case
			const bool useLightStencilSelect = ( r_useLightStencilSelect.GetBool() && backEnd.viewDef->isMirror == false );
			
			if( performStencilTest )
			{
				if( useLightStencilSelect )
				{
					// write a stencil mask for the visible light bounds to hi-stencil
					RB_StencilSelectLight( vLight );
				}
				else
				{
					// always clear whole S-Cull tiles
					idScreenRect rect;
					rect.x1 = ( vLight->scissorRect.x1 +  0 ) & ~15;
					rect.y1 = ( vLight->scissorRect.y1 +  0 ) & ~15;
					rect.x2 = ( vLight->scissorRect.x2 + 15 ) & ~15;
					rect.y2 = ( vLight->scissorRect.y2 + 15 ) & ~15;

					RB_SetScissor( rect );

					GL_State( GLS_DEFAULT );	// make sure stencil mask passes for the clear
					GL_Clear( false, false, true, STENCIL_SHADOW_TEST_VALUE, 0.0f, 0.0f, 0.0f, 0.0f, false );
				}
			}
			
			if( vLight->globalShadows != NULL )
			{
				RENDERLOG_OPEN_BLOCK( "Global Light Shadows" );
				RB_StencilShadowPass( vLight->globalShadows, vLight );
				RENDERLOG_CLOSE_BLOCK();
			}
			
			if( vLight->localInteractions != NULL )
			{
				RENDERLOG_OPEN_BLOCK( "Local Light Interactions" );
				RB_RenderInteractions( vLight->localInteractions, vLight, GLS_DEPTHFUNC_EQUAL, performStencilTest, useLightDepthBounds );
				RENDERLOG_CLOSE_BLOCK();
			}
			
			if( vLight->localShadows != NULL )
			{
				RENDERLOG_OPEN_BLOCK( "Local Light Shadows" );
				RB_StencilShadowPass( vLight->localShadows, vLight );
				RENDERLOG_CLOSE_BLOCK();
			}
			
			if( vLight->globalInteractions != NULL )
			{
				RENDERLOG_OPEN_BLOCK( "Global Light Interactions" );
				RB_RenderInteractions( vLight->globalInteractions, vLight, GLS_DEPTHFUNC_EQUAL, performStencilTest, useLightDepthBounds );
				RENDERLOG_CLOSE_BLOCK();
			}
		}
		// RB end
		
		if( vLight->translucentInteractions != NULL && !r_skipTranslucent.GetBool() )
		{
			RENDERLOG_OPEN_BLOCK( "Translucent Interactions" );
			
			// Disable the depth bounds test because translucent surfaces don't work with
			// the depth bounds tests since they did not write depth during the depth pass.
			if( useLightDepthBounds )
			{
				GL_DepthBoundsTest( 0.0f, 0.0f );
			}
			
			// The depth buffer wasn't filled in for translucent surfaces, so they
			// can never be constrained to perforated surfaces with the depthfunc equal.
			
			// Translucent surfaces do not receive shadows. This is a case where a
			// shadow buffer solution would work but stencil shadows do not because
			// stencil shadows only affect surfaces that contribute to the view depth
			// buffer and translucent surfaces do not contribute to the view depth buffer.
			
			RB_RenderInteractions( vLight->translucentInteractions, vLight, GLS_DEPTHFUNC_LESS, false, false );
			
			RENDERLOG_CLOSE_BLOCK();
		}
		
		RENDERLOG_CLOSE_BLOCK();
	}
	
	// disable stencil shadow test
	GL_State( GLS_DEFAULT );
	
	// unbind texture units
	GL_ResetTexturesState();
	
	// reset depth bounds
	if( useLightDepthBounds )
	{
		GL_DepthBoundsTest( 0.0f, 0.0f );
	}
	
	RENDERLOG_CLOSE_BLOCK();
	RENDERLOG_CLOSE_MAINBLOCK();
}

/*
=============================================================================================

NON-INTERACTION SHADER PASSES

=============================================================================================
*/

/*
=====================
RB_DrawShaderPasses

Draw non-light dependent passes

If we are rendering Guis, the drawSurf_t::sort value is a depth offset that can
be multiplied by guiEye for polarity and screenSeparation for scale.
=====================
*/
static int RB_DrawShaderPasses( const drawSurf_t* const* const drawSurfs, const int numDrawSurfs,
								const float guiStereoScreenOffset, const int stereoEye )
{
	// only obey skipAmbient if we are rendering a view
	if( backEnd.viewDef->viewEntitys && r_skipAmbient.GetBool() )
	{
		return numDrawSurfs;
	}
	
	RENDERLOG_OPEN_BLOCK( "RB_DrawShaderPasses" );
	
	GL_SelectTexture( 1 );
	globalImages->BindNull();
	
	GL_SelectTexture( 0 );
	
	backEnd.currentSpace = ( const viewModel_t* )1;	// using NULL makes /analyze think surf->space needs to be checked...
	float currentGuiStereoOffset = 0.0f;
	
	int i = 0;
	for( ; i < numDrawSurfs; ++i )
	{
		const drawSurf_t* const surf = drawSurfs[i];
		const idMaterial* const shader = surf->material;
		
		if( !shader->HasAmbient() )
		{
			continue;
		}
		
		if( shader->IsPortalSky() )
		{
			continue;
		}
		
		// some deforms may disable themselves by setting numIndexes = 0
		if( surf->numIndexes == 0 )
		{
			continue;
		}
		
		if( shader->SuppressInSubview() )
		{
			continue;
		}
		
		if( backEnd.viewDef->isXraySubview && surf->space->entityDef )
		{
			if( surf->space->entityDef->GetParms().xrayIndex != 2 ) //SEA: calling entityDef here is very bad !!!
			{
				continue;
			}
		}
		
		// we need to draw the post process shaders after we have drawn the fog lights
		if( shader->GetSort() >= SS_POST_PROCESS && !backEnd.currentRenderCopied )
		{
			break;
		}
		
		// if we are rendering a 3D view and the surface's eye index doesn't match
		// the current view's eye index then we skip the surface
		// if the stereoEye value of a surface is 0 then we need to draw it for both eyes.
		const int shaderStereoEye = shader->GetStereoEye();
		const bool isEyeValid = stereoRender_swapEyes.GetBool() ? ( shaderStereoEye == stereoEye ) : ( shaderStereoEye != stereoEye );
		if( ( stereoEye != 0 ) && ( shaderStereoEye != 0 ) && ( isEyeValid ) )
		{
			continue;
		}
		
		RENDERLOG_OPEN_BLOCK( shader->GetName() );
		
		// determine the stereoDepth offset
		// guiStereoScreenOffset will always be zero for 3D views, so the !=
		// check will never force an update due to the current sort value.
		const float thisGuiStereoOffset = guiStereoScreenOffset * surf->sort;
		
		// change the matrix and other space related vars if needed
		if( surf->space != backEnd.currentSpace || thisGuiStereoOffset != currentGuiStereoOffset )
		{
			backEnd.currentSpace = surf->space;
			currentGuiStereoOffset = thisGuiStereoOffset;
			
			const viewModel_t* space = backEnd.currentSpace;
			
			if( guiStereoScreenOffset != 0.0f )
			{
				RB_SetMVPWithStereoOffset( space->mvp, currentGuiStereoOffset );
			}
			else
			{
				RB_SetMVP( space->mvp );
			}
						
			// set model Matrix
			SetVertexParms( RENDERPARM_MODELMATRIX_X, space->modelMatrix.Ptr(), 4 );
			
			// Set ModelView Matrix
			SetVertexParms( RENDERPARM_MODELVIEWMATRIX_X, space->modelViewMatrix.Ptr(), 4 );

			// set eye position in local space
			idVec4 localViewOrigin( 1.0f );
			space->modelMatrix.InverseTransformPoint( backEnd.viewDef->GetOrigin(), localViewOrigin.ToVec3() );
			SetVertexParm( RENDERPARM_LOCALVIEWORIGIN, localViewOrigin.ToFloatPtr() );
		}
		
		// change the scissor if needed
		RB_SetScissor( surf->scissorRect );
		
		// get the expressions for conditionals / color / texcoords
		const float* regs = surf->shaderRegisters;
		
		// set face culling appropriately
		if( surf->space->isGuiSurface )
		{
			GL_Cull( CT_TWO_SIDED );
		}
		else
		{
			GL_Cull( shader->GetCullType() );
		}
		
		uint64 surfGLState = surf->extraGLState;
		
		// set polygon offset if necessary
		if( shader->TestMaterialFlag( MF_POLYGONOFFSET ) )
		{
			GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
			surfGLState = GLS_POLYGON_OFFSET;
		}
		
		for( int stage = 0; stage < shader->GetNumStages(); stage++ )
		{
			auto pStage = shader->GetStage( stage );
			
			// check the enable condition
			if( regs[ pStage->conditionRegister ] == 0 )
			{
				continue;
			}
			
			// skip the stages involved in lighting
			if( pStage->lighting != SL_AMBIENT )
			{
				continue;
			}
			
			uint64 stageGLState = surfGLState;
			if( ( surfGLState & GLS_OVERRIDE ) == 0 )
			{
				stageGLState |= pStage->drawStateBits;
			}
			
			// skip if the stage is ( GL_ZERO, GL_ONE ), which is used for some alpha masks
			if( ( stageGLState & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) == ( GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE ) )
			{
				continue;
			}
			
			
			// see if we are a new-style stage
			auto newStage = pStage->newStage;
			if( newStage != NULL )
			{
				//--------------------------
				//
				// new style stages
				//
				//--------------------------
				if( r_skipNewAmbient.GetBool() )
				{
					continue;
				}
				RENDERLOG_OPEN_BLOCK( "New Shader Stage" );
				
				GL_State( stageGLState );
				
				// RB: CRITICAL BUGFIX: changed newStage->glslProgram to vertexProgram and fragmentProgram
				// otherwise it will result in an out of bounds crash in GL_DrawElementsWithCounters
				renderProgManager.BindShader( newStage->glslProgram, newStage->vertexProgram, newStage->fragmentProgram, false );
				// RB end
				
				for( int j = 0; j < newStage->numVertexParms; j++ )
				{
					float parm[4];
					parm[0] = regs[ newStage->vertexParms[j][0] ];
					parm[1] = regs[ newStage->vertexParms[j][1] ];
					parm[2] = regs[ newStage->vertexParms[j][2] ];
					parm[3] = regs[ newStage->vertexParms[j][3] ];
					SetVertexParm( ( renderParm_t )( RENDERPARM_USER + j ), parm );
				}
				
				// set rpEnableSkinning if the shader has optional support for skinning
				if( surf->jointCache && renderProgManager.ShaderHasOptionalSkinning() )
				{
					const idVec4 skinningParm( 1.0f );
					SetVertexParm( RENDERPARM_ENABLE_SKINNING, skinningParm.ToFloatPtr() );
				}
				
				// bind texture units
				for( int j = 0; j < newStage->numFragmentProgramImages; j++ )
				{
					idImage* image = newStage->fragmentProgramImages[j];
					if( image != NULL )
					{
						GL_BindTexture( j, image );
					}
				}
				
				// draw it
				GL_DrawElementsWithCounters( surf );
				
				// unbind texture units
				for( int j = 0; j < newStage->numFragmentProgramImages; j++ )
				{
					idImage* image = newStage->fragmentProgramImages[j];
					if( image != NULL )
					{
						GL_SelectTexture( j );
						globalImages->BindNull();
					}
				}
				
				// clear rpEnableSkinning if it was set
				if( surf->jointCache && renderProgManager.ShaderHasOptionalSkinning() )
				{
					const idVec4 skinningParm( 0.0f );
					SetVertexParm( RENDERPARM_ENABLE_SKINNING, skinningParm.ToFloatPtr() );
				}
				
				GL_SelectTexture( 0 );
				renderProgManager.Unbind();
				
				RENDERLOG_CLOSE_BLOCK();
				continue;
			}
			
			//--------------------------
			//
			// old style stages
			//
			//--------------------------
			
			// set the color
			idVec4 color;
			color[0] = regs[ pStage->color.registers[0] ];
			color[1] = regs[ pStage->color.registers[1] ];
			color[2] = regs[ pStage->color.registers[2] ];
			color[3] = regs[ pStage->color.registers[3] ];
			
			// skip the entire stage if an add would be black
			if( ( stageGLState & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) == ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE )
					&& color[0] <= 0 && color[1] <= 0 && color[2] <= 0 )
			{
				continue;
			}
			
			// skip the entire stage if a blend would be completely transparent
			if( ( stageGLState & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) == ( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA )
					&& color[3] <= 0 )
			{
				continue;
			}
			
			stageVertexColor_t svc = pStage->vertexColor;
			
			RENDERLOG_OPEN_BLOCK( "Old Shader Stage" );
			GL_Color( color );
			
			if( surf->space->isGuiSurface )
			{
				// Force gui surfaces to always be SVC_MODULATE
				svc = SVC_MODULATE;
				
				// use special shaders for bink cinematics
				if( pStage->texture.cinematic )
				{
					if( ( stageGLState & GLS_OVERRIDE ) != 0 )
					{
						// This is a hack... Only SWF Guis set GLS_OVERRIDE
						// Old style guis do not, and we don't want them to use the new GUI renederProg
						renderProgManager.BindShader_TextureVertexColor_sRGB();
					}
					else
					{
						renderProgManager.BindShader_TextureVertexColor();
					}
				}
				else
				{
					if( ( stageGLState & GLS_OVERRIDE ) != 0 )
					{
						// This is a hack... Only SWF Guis set GLS_OVERRIDE
						// Old style guis do not, and we don't want them to use the new GUI renderProg
						renderProgManager.BindShader_GUI();
					}
					else
					{
						if( surf->jointCache )
						{
							renderProgManager.BindShader_TextureVertexColorSkinned();
						}
						else
						{
							if( backEnd.viewDef->is2Dgui )
							{
								// RB: 2D fullscreen drawing like warp or damage blend effects
								renderProgManager.BindShader_TextureVertexColor_sRGB();
							}
							else
							{
								renderProgManager.BindShader_TextureVertexColor();
							}
						}
					}
				}
			}
			else if( ( pStage->texture.texgen == TG_SCREEN ) || ( pStage->texture.texgen == TG_SCREEN2 ) )
			{
				renderProgManager.BindShader_TextureTexGenVertexColor();
			}
			else if( pStage->texture.cinematic )
			{
				renderProgManager.BindShader_Bink();
			}
			else
			{
				if( surf->jointCache )
				{
					renderProgManager.BindShader_TextureVertexColorSkinned();
				}
				else
				{
					renderProgManager.BindShader_TextureVertexColor();
				}
			}
			
			RB_SetVertexColorParms( svc );
			
			// bind the texture
			RB_BindVariableStageImage( &pStage->texture, regs );
			
			// set privatePolygonOffset if necessary
			if( pStage->privatePolygonOffset )
			{
				GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * pStage->privatePolygonOffset );
				stageGLState |= GLS_POLYGON_OFFSET;
			}
			
			// set the state
			GL_State( stageGLState );
			
			RB_PrepareStageTexturing( pStage, surf );
			
			// draw it
			GL_DrawElementsWithCounters( surf );
			
			RB_FinishStageTexturing( pStage, surf );
			
			// unset privatePolygonOffset if necessary
			if( pStage->privatePolygonOffset )
			{
				GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
			}
			RENDERLOG_CLOSE_BLOCK();
		}
		
		RENDERLOG_CLOSE_BLOCK();
	}
	
	GL_Cull( CT_FRONT_SIDED );
	GL_Color( 1.0f, 1.0f, 1.0f );
	
	// disable stencil shadow test
	GL_State( GLS_DEFAULT );
	
	// unbind texture units
	GL_ResetTexturesState();
	
	RENDERLOG_CLOSE_BLOCK();
	return i;
}

/*
=============================================================================================

BLEND LIGHT PROJECTION

=============================================================================================
*/

static void _GlobalPlaneToLocal( const idRenderMatrix & modelMatrix, const idPlane& in, idPlane& out )
{
	out[ 0 ] = in[ 0 ] * modelMatrix[ 0 ][ 0 ] + in[ 1 ] * modelMatrix[ 1 ][ 0 ] + in[ 2 ] * modelMatrix[ 2 ][ 0 ];
	out[ 1 ] = in[ 0 ] * modelMatrix[ 0 ][ 1 ] + in[ 1 ] * modelMatrix[ 1 ][ 1 ] + in[ 2 ] * modelMatrix[ 2 ][ 1 ];
	out[ 2 ] = in[ 0 ] * modelMatrix[ 0 ][ 2 ] + in[ 1 ] * modelMatrix[ 1 ][ 2 ] + in[ 2 ] * modelMatrix[ 2 ][ 2 ];
	out[ 3 ] = in[ 0 ] * modelMatrix[ 0 ][ 3 ] + in[ 1 ] * modelMatrix[ 1 ][ 3 ] + in[ 2 ] * modelMatrix[ 2 ][ 3 ] + in[ 3 ];
}

/*
=====================
RB_T_BlendLight
=====================
*/
static void RB_T_BlendLight( const drawSurf_t* drawSurfs, const viewLight_t* vLight )
{
	backEnd.currentSpace = NULL;
	
	for( const drawSurf_t* drawSurf = drawSurfs; drawSurf != NULL; drawSurf = drawSurf->nextOnLight )
	{
		if( drawSurf->scissorRect.IsEmpty() )
		{
			continue;	// !@# FIXME: find out why this is sometimes being hit!
			// temporarily jump over the scissor and draw so the gl error callback doesn't get hit
		}
		
		RB_SetScissor( drawSurf->scissorRect );
		
		if( drawSurf->space != backEnd.currentSpace )
		{
			// change the matrix
			RB_SetMVP( drawSurf->space->mvp );
			
			// change the light projection matrix
			ALIGNTYPE16 idPlane	lightProjectInCurrentSpace[4];
			for( int i = 0; i < 4; i++ )
			{
				_GlobalPlaneToLocal( drawSurf->space->modelMatrix, vLight->lightProject[i], lightProjectInCurrentSpace[i] );
			}
			//for( int i = 0; i < 4; i++ )
			//{
			//	drawSurf->space->modelMatrix.InverseTransformPlane( vLight->lightProject[ i ], lightProjectInCurrentSpace[ i ] );
			//}
			
			SetVertexParm( RENDERPARM_TEXGEN_0_S, lightProjectInCurrentSpace[0].ToFloatPtr() );
			SetVertexParm( RENDERPARM_TEXGEN_0_T, lightProjectInCurrentSpace[1].ToFloatPtr() );
			SetVertexParm( RENDERPARM_TEXGEN_0_Q, lightProjectInCurrentSpace[2].ToFloatPtr() );
			SetVertexParm( RENDERPARM_TEXGEN_1_S, lightProjectInCurrentSpace[3].ToFloatPtr() );	// falloff
			
			backEnd.currentSpace = drawSurf->space;
		}
		
		GL_DrawElementsWithCounters( drawSurf );
	}
}

/*
=====================
RB_BlendLight

Dual texture together the falloff and projection texture with a blend
mode to the framebuffer, instead of interacting with the surface texture
=====================
*/
static void RB_BlendLight( const drawSurf_t* drawSurfs, const drawSurf_t* drawSurfs2, const viewLight_t* vLight )
{
	if( drawSurfs == NULL )
	{
		return;
	}
	if( r_skipBlendLights.GetBool() )
	{
		return;
	}
	RENDERLOG_OPEN_BLOCK( vLight->lightShader->GetName() );
	
	const idMaterial* lightShader = vLight->lightShader;
	const float* regs = vLight->shaderRegisters;
	
	// texture 1 will get the falloff texture
	GL_BindTexture( 1, vLight->falloffImage );
	
	// texture 0 will get the projected texture
	GL_SelectTexture( 0 );
	
	renderProgManager.BindShader_BlendLight();
	
	for( int i = 0; i < lightShader->GetNumStages(); i++ )
	{
		auto stage = lightShader->GetStage( i );
		
		if( !regs[ stage->conditionRegister ] )
		{
			continue;
		}
		
		GL_State( GLS_DEPTHMASK | stage->drawStateBits | GLS_DEPTHFUNC_EQUAL );
		
		GL_BindTexture( 0, stage->texture.image );
		
		if( stage->texture.hasMatrix )
		{
			RB_LoadShaderTextureMatrix( regs, &stage->texture );
		}
		
		// get the modulate values from the light, including alpha, unlike normal lights
		idVec4 lightColor;
		lightColor[0] = regs[ stage->color.registers[0] ];
		lightColor[1] = regs[ stage->color.registers[1] ];
		lightColor[2] = regs[ stage->color.registers[2] ];
		lightColor[3] = regs[ stage->color.registers[3] ];
		GL_Color( lightColor );
		
		RB_T_BlendLight( drawSurfs, vLight );
		RB_T_BlendLight( drawSurfs2, vLight );
	}
	
	GL_SelectTexture( 1 );
	globalImages->BindNull();
	
	GL_SelectTexture( 0 );
	
	renderProgManager.Unbind();
	RENDERLOG_CLOSE_BLOCK();
}

/*
=========================================================================================================

FOG LIGHTS

=========================================================================================================
*/

/*
=====================
RB_T_BasicFog
=====================
*/
static void RB_T_BasicFog( const drawSurf_t* drawSurfs, const idPlane fogPlanes[4], const idRenderMatrix* inverseBaseLightProject )
{
	backEnd.currentSpace = NULL;
	
	for( const drawSurf_t* drawSurf = drawSurfs; drawSurf != NULL; drawSurf = drawSurf->nextOnLight )
	{
		if( drawSurf->scissorRect.IsEmpty() )
		{
			continue;	// !@# FIXME: find out why this is sometimes being hit!
			// temporarily jump over the scissor and draw so the gl error callback doesn't get hit
		}
		
		RB_SetScissor( drawSurf->scissorRect );
		
		if( drawSurf->space != backEnd.currentSpace )
		{
			idPlane localFogPlanes[4];
			if( inverseBaseLightProject == NULL )
			{
				RB_SetMVP( drawSurf->space->mvp );

				for( int i = 0; i < 4; i++ )
				{
					_GlobalPlaneToLocal( drawSurf->space->modelMatrix, fogPlanes[i], localFogPlanes[i] );
					//modelMatrix.InverseTransformPlane( fogPlanes[ i ], localFogPlanes[ i ] );
				}
			}
			else
			{
				idRenderMatrix invProjectMVPMatrix;
				idRenderMatrix::Multiply( backEnd.viewDef->GetMVPMatrix(), *inverseBaseLightProject, invProjectMVPMatrix );
				RB_SetMVP( invProjectMVPMatrix );
				for( int i = 0; i < 4; i++ )
				{
					inverseBaseLightProject->InverseTransformPlane( fogPlanes[i], localFogPlanes[i], false );
				}
			}
			
			SetVertexParm( RENDERPARM_TEXGEN_0_S, localFogPlanes[0].ToFloatPtr() );
			SetVertexParm( RENDERPARM_TEXGEN_0_T, localFogPlanes[1].ToFloatPtr() );
			SetVertexParm( RENDERPARM_TEXGEN_1_T, localFogPlanes[2].ToFloatPtr() );
			SetVertexParm( RENDERPARM_TEXGEN_1_S, localFogPlanes[3].ToFloatPtr() );
			
			backEnd.currentSpace = ( inverseBaseLightProject == NULL )? drawSurf->space : NULL;
		}
		
		if( drawSurf->jointCache )
		{
			renderProgManager.BindShader_FogSkinned();
		}
		else
		{
			renderProgManager.BindShader_Fog();
		}
		
		GL_DrawElementsWithCounters( drawSurf );
	}
}

/*
==================
RB_FogPass
==================
*/
static void RB_FogPass( const drawSurf_t* drawSurfs,  const drawSurf_t* drawSurfs2, const viewLight_t* vLight )
{
	RENDERLOG_OPEN_BLOCK( vLight->lightShader->GetName() );
	
	// find the current color and density of the fog

	// assume fog shaders have only a single stage
	auto stage = vLight->lightShader->GetStage( 0 );
	
	idVec4 lightColor;
	lightColor[0] = vLight->shaderRegisters[ stage->color.registers[0] ];
	lightColor[1] = vLight->shaderRegisters[ stage->color.registers[1] ];
	lightColor[2] = vLight->shaderRegisters[ stage->color.registers[2] ];
	lightColor[3] = vLight->shaderRegisters[ stage->color.registers[3] ];
	
	GL_Color( lightColor );
	
	// Calculate the falloff planes.
	// If they left the default value on, set a fog distance of 500 otherwise, distance = alpha color
	const float a = ( lightColor[ 3 ] <= 1.0f )? ( -0.5f / DEFAULT_FOG_DISTANCE ) : ( -0.5f / lightColor[ 3 ] );
	
	// texture 0 is the falloff image
	GL_BindTexture( 0, globalImages->fogImage );
	
	// texture 1 is the entering plane fade correction
	GL_BindTexture( 1, globalImages->fogEnterImage );

	idPlane fogPlane;

	// the fog plane is the light far clip plane
	const idPlane plane( 
		vLight->baseLightProject[ 2 ][ 0 ] - vLight->baseLightProject[ 3 ][ 0 ],
		vLight->baseLightProject[ 2 ][ 1 ] - vLight->baseLightProject[ 3 ][ 1 ],
		vLight->baseLightProject[ 2 ][ 2 ] - vLight->baseLightProject[ 3 ][ 2 ],
		vLight->baseLightProject[ 2 ][ 3 ] - vLight->baseLightProject[ 3 ][ 3 ] 
	);
	const float planeScale = idMath::InvSqrt( plane.Normal().LengthSqr() );
	fogPlane[ 0 ] = plane[ 0 ] * planeScale;
	fogPlane[ 1 ] = plane[ 1 ] * planeScale;
	fogPlane[ 2 ] = plane[ 2 ] * planeScale;
	fogPlane[ 3 ] = plane[ 3 ] * planeScale;
	
	// S is based on the view origin
	const float s = fogPlane.Distance( backEnd.viewDef->GetOrigin() );
	
	const float FOG_SCALE = 0.001f;
	
	ALIGN16( idPlane fogPlanes[4] );
	
	// S-0
	fogPlanes[0][0] = a * backEnd.viewDef->GetViewMatrix()[2][0];
	fogPlanes[0][1] = a * backEnd.viewDef->GetViewMatrix()[2][1];
	fogPlanes[0][2] = a * backEnd.viewDef->GetViewMatrix()[2][2];
	fogPlanes[0][3] = a * backEnd.viewDef->GetViewMatrix()[2][3] + 0.5f;
	
	// T-0
	fogPlanes[1][0] = 0.0f;//a * backEnd.viewDef->worldSpace.modelViewMatrix[0*4+0];
	fogPlanes[1][1] = 0.0f;//a * backEnd.viewDef->worldSpace.modelViewMatrix[1*4+0];
	fogPlanes[1][2] = 0.0f;//a * backEnd.viewDef->worldSpace.modelViewMatrix[2*4+0];
	fogPlanes[1][3] = 0.5f;//a * backEnd.viewDef->worldSpace.modelViewMatrix[3*4+0] + 0.5f;
	
	// T-1 will get a texgen for the fade plane, which is always the "top" plane on unrotated lights
	fogPlanes[2][0] = FOG_SCALE * fogPlane[0];
	fogPlanes[2][1] = FOG_SCALE * fogPlane[1];
	fogPlanes[2][2] = FOG_SCALE * fogPlane[2];
	fogPlanes[2][3] = FOG_SCALE * fogPlane[3] + FOG_ENTER;
	
	// S-1
	fogPlanes[3][0] = 0.0f;
	fogPlanes[3][1] = 0.0f;
	fogPlanes[3][2] = 0.0f;
	fogPlanes[3][3] = FOG_SCALE * s + FOG_ENTER;
	
	// draw it
	GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );
	RB_T_BasicFog( drawSurfs, fogPlanes, NULL );
	RB_T_BasicFog( drawSurfs2, fogPlanes, NULL );
	
	// the light frustum bounding planes aren't in the depth buffer, so use depthfunc_less instead
	// of depthfunc_equal
	GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_LESS );
	GL_Cull( CT_BACK_SIDED );
	
	backEnd.zeroOneCubeSurface.space = &backEnd.viewDef->GetWorldSpace();
	backEnd.zeroOneCubeSurface.scissorRect = backEnd.viewDef->GetScissor();
	RB_T_BasicFog( &backEnd.zeroOneCubeSurface, fogPlanes, &vLight->inverseBaseLightProject );
	
	GL_Cull( CT_FRONT_SIDED );
	
	GL_SelectTexture( 1 );
	globalImages->BindNull();
	
	GL_SelectTexture( 0 );
	
	renderProgManager.Unbind();
	
	RENDERLOG_CLOSE_BLOCK();
}

/*
==================
RB_FogAllLights
==================
*/
static void RB_FogAllLights()
{
	if( r_skipFogLights.GetBool() || r_showOverDraw.GetInteger() != 0
		|| backEnd.viewDef->isXraySubview /* don't fog in xray mode*/ )
	{
		return;
	}
	RENDERLOG_OPEN_MAINBLOCK( MRB_FOG_ALL_LIGHTS );
	RENDERLOG_OPEN_BLOCK( "RB_FogAllLights" );
	
	// force fog plane to recalculate
	backEnd.currentSpace = NULL;
	
	for( viewLight_t* vLight = backEnd.viewDef->viewLights; vLight != NULL; vLight = vLight->next )
	{
		if( vLight->lightShader->IsFogLight() )
		{
			RB_FogPass( vLight->globalInteractions, vLight->localInteractions, vLight );
		}
		else if( vLight->lightShader->IsBlendLight() )
		{
			RB_BlendLight( vLight->globalInteractions, vLight->localInteractions, vLight );
		}
	}
	
	RENDERLOG_CLOSE_BLOCK();
	RENDERLOG_CLOSE_MAINBLOCK();
}


// RB begin
static void RB_CalculateAdaptation()
{
	int				i;
	static float	image[64 * 64 * 4];
	float           curTime;
	float			deltaTime;
	float           luminance;
	float			avgLuminance;
	float			maxLuminance;
	double			sum;
	const idVec3    LUMINANCE_SRGB( 0.2125f, 0.7154f, 0.0721f ); // be careful wether this should be linear RGB or sRGB
	idVec4			color;
	float			newAdaptation;
	float			newMaximum;
	
	if( !r_hdrAutoExposure.GetBool() )
	{
		// no dynamic exposure
		
		backEnd.hdrKey = r_hdrKey.GetFloat();
		backEnd.hdrAverageLuminance = r_hdrMinLuminance.GetFloat();
		backEnd.hdrMaxLuminance = 1;
	}
	else
	{
		curTime = Sys_Milliseconds() / 1000.0f;
		
		// calculate the average scene luminance
		globalFramebuffers.hdr64FBO->Bind();
		
		// read back the contents
		//	glFinish();
		glReadPixels( 0, 0, 64, 64, GL_RGBA, GL_FLOAT, image );
		
		sum = 0.0f;
		maxLuminance = 0.0f;
		for( i = 0; i < ( 64 * 64 ); i += 4 )
		{
			color[0] = image[i * 4 + 0];
			color[1] = image[i * 4 + 1];
			color[2] = image[i * 4 + 2];
			color[3] = image[i * 4 + 3];
			
			luminance = ( color.x * LUMINANCE_SRGB.x + color.y * LUMINANCE_SRGB.y + color.z * LUMINANCE_SRGB.z ) + 0.0001f;
			if( luminance > maxLuminance )
			{
				maxLuminance = luminance;
			}
			
			float logLuminance = log( luminance + 1 );
			//if( logLuminance > 0 )
			{
				sum += luminance;
			}
		}
#if 0
		sum /= ( 64.0f * 64.0f );
		avgLuminance = exp( sum );
#else
		avgLuminance = sum / ( 64.0f * 64.0f );
#endif
		
		// the user's adapted luminance level is simulated by closing the gap between
		// adapted luminance and current luminance by 2% every frame, based on a
		// 30 fps rate. This is not an accurate model of human adaptation, which can
		// take longer than half an hour.
		if( backEnd.hdrTime > curTime )
		{
			backEnd.hdrTime = curTime;
		}
		
		deltaTime = curTime - backEnd.hdrTime;
		
		//if(r_hdrMaxLuminance->value)
		{
			backEnd.hdrAverageLuminance = idMath::ClampFloat( r_hdrMinLuminance.GetFloat(), r_hdrMaxLuminance.GetFloat(), backEnd.hdrAverageLuminance );
			avgLuminance = idMath::ClampFloat( r_hdrMinLuminance.GetFloat(), r_hdrMaxLuminance.GetFloat(), avgLuminance );
			
			backEnd.hdrMaxLuminance = idMath::ClampFloat( r_hdrMinLuminance.GetFloat(), r_hdrMaxLuminance.GetFloat(), backEnd.hdrMaxLuminance );
			maxLuminance = idMath::ClampFloat( r_hdrMinLuminance.GetFloat(), r_hdrMaxLuminance.GetFloat(), maxLuminance );
		}
		
		newAdaptation = backEnd.hdrAverageLuminance + ( avgLuminance - backEnd.hdrAverageLuminance ) * ( 1.0f - powf( 0.98f, 30.0f * deltaTime ) );
		newMaximum = backEnd.hdrMaxLuminance + ( maxLuminance - backEnd.hdrMaxLuminance ) * ( 1.0f - powf( 0.98f, 30.0f * deltaTime ) );
		
		if( !IsNAN( newAdaptation ) && !IsNAN( newMaximum ) )
		{
#if 1
			backEnd.hdrAverageLuminance = newAdaptation;
			backEnd.hdrMaxLuminance = newMaximum;
#else
			backEnd.hdrAverageLuminance = avgLuminance;
			backEnd.hdrMaxLuminance = maxLuminance;
#endif
		}
		
		backEnd.hdrTime = curTime;
		
		// calculate HDR image key
#if 0
		// RB: this never worked :/
		if( r_hdrAutoExposure.GetBool() )
		{
			// calculation from: Perceptual Effects in Real-time Tone Mapping - Krawczyk et al.
			backEnd.hdrKey = 1.03 - ( 2.0 / ( 2.0 + ( backEnd.hdrAverageLuminance + 1.0f ) ) );
		}
		else
#endif
		{
			backEnd.hdrKey = r_hdrKey.GetFloat();
		}
	}
	
	if( r_hdrDebug.GetBool() )
	{
		idLib::Printf( "HDR luminance avg = %f, max = %f, key = %f\n", backEnd.hdrAverageLuminance, backEnd.hdrMaxLuminance, backEnd.hdrKey );
	}
	
	//GL_CheckErrors();
}


static void RB_Tonemap( const idRenderView* viewDef )
{
	RENDERLOG_PRINTF( "---------- RB_Tonemap( avg = %f, max = %f, key = %f, is2Dgui = %i ) ----------\n", backEnd.hdrAverageLuminance, backEnd.hdrMaxLuminance, backEnd.hdrKey, ( int )viewDef->is2Dgui );
	
	//postProcessCommand_t* cmd = ( postProcessCommand_t* )data;
	//const idScreenRect& viewport = cmd->viewDef->viewport;
	//globalImages->currentRenderImage->CopyFramebuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );
	
	Framebuffer::Unbind();
	
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
	GL_Cull( CT_TWO_SIDED );
	
	int screenWidth = renderSystem->GetWidth();
	int screenHeight = renderSystem->GetHeight();
	
	// set the window clipping
	GL_ViewportAndScissor( 0, 0, screenWidth, screenHeight );
	
	GL_SelectTexture( 0 );
	
#if defined(USE_HDR_MSAA)
	if( glConfig.multisamples > 0 )
	{
		globalImages->currentRenderHDRImageNoMSAA->Bind();
	}
	else
#endif
	{
		globalImages->currentRenderHDRImage->Bind();
	}
	
	GL_BindTexture( 1, globalImages->heatmap7Image );
	
	if( r_hdrDebug.GetBool() )
	{
		renderProgManager.BindShader_HDRDebug();
	}
	else
	{
		renderProgManager.BindShader_Tonemap();
	}
	
	float screenCorrectionParm[4];
	if( viewDef->is2Dgui )
	{
		screenCorrectionParm[0] = 2.0f;
		screenCorrectionParm[1] = 1.0f;
		screenCorrectionParm[2] = 1.0f;
	}
	else
	{
		if( r_hdrAutoExposure.GetBool() )
		{
			float exposureOffset = Lerp( -0.01f, 0.02f, idMath::ClampFloat( 0.0, 1.0, r_exposure.GetFloat() ) );
			
			screenCorrectionParm[0] = backEnd.hdrKey + exposureOffset;
			screenCorrectionParm[1] = backEnd.hdrAverageLuminance;
			screenCorrectionParm[2] = backEnd.hdrMaxLuminance;
			screenCorrectionParm[3] = exposureOffset;
			//screenCorrectionParm[3] = Lerp( -1, 5, idMath::ClampFloat( 0.0, 1.0, r_exposure.GetFloat() ) );
		}
		else
		{
			//float exposureOffset = ( idMath::ClampFloat( 0.0, 1.0, r_exposure.GetFloat() ) * 2.0f - 1.0f ) * 0.01f;
			
			float exposureOffset = Lerp( -0.01f, 0.01f, idMath::ClampFloat( 0.0, 1.0, r_exposure.GetFloat() ) );
			
			screenCorrectionParm[0] = 0.015f + exposureOffset;
			screenCorrectionParm[1] = 0.005f;
			screenCorrectionParm[2] = 1;
			
			// RB: this gives a nice exposure curve in Scilab when using
			// log2( max( 3 + 0..10, 0.001 ) ) as input for exp2
			//float exposureOffset = r_exposure.GetFloat() * 10.0f;
			//screenCorrectionParm[3] = exposureOffset;
		}
	}
	
	SetFragmentParm( RENDERPARM_SCREENCORRECTIONFACTOR, screenCorrectionParm ); // rpScreenCorrectionFactor
	
	// Draw
	GL_DrawElementsWithCounters( &backEnd.unitSquareSurface );
	
	// unbind heatmap
	globalImages->BindNull();
	
	// unbind _currentRender
	GL_SelectTexture( 0 );
	globalImages->BindNull();
	
	renderProgManager.Unbind();
	
	GL_State( GLS_DEFAULT );
	GL_Cull( CT_FRONT_SIDED );
}


static void RB_Bloom( const idRenderView* viewDef )
{
	if( viewDef->is2Dgui || !r_useHDR.GetBool() )
	{
		return;
	}
	
	RENDERLOG_PRINTF( "---------- RB_Bloom( avg = %f, max = %f, key = %f ) ----------\n", backEnd.hdrAverageLuminance, backEnd.hdrMaxLuminance, backEnd.hdrKey );
	
	// BRIGHTPASS
	
	//GL_CheckErrors();
	
	//Framebuffer::Unbind();
	//globalFramebuffers.hdrQuarterFBO->Bind();
	
	glClearColor( 0, 0, 0, 1 );
//	glClear( GL_COLOR_BUFFER_BIT );

	GL_State( /*GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO |*/ GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
	GL_Cull( CT_TWO_SIDED );
	
	int screenWidth = renderSystem->GetWidth();
	int screenHeight = renderSystem->GetHeight();
	
	// set the window clipping
	GL_ViewportAndScissor( 0, 0, screenWidth / 4, screenHeight / 4 );
	
	globalFramebuffers.bloomRenderFBO[ 0 ]->Bind();
	
	GL_SelectTexture( 0 );
	
	if( r_useHDR.GetBool() )
	{
		globalImages->currentRenderHDRImage->Bind();
		
		renderProgManager.BindShader_Brightpass();
	}
	else
	{
		int x = backEnd.viewDef->viewport.x1;
		int y = backEnd.viewDef->viewport.y1;
		int	w = backEnd.viewDef->viewport.x2 - backEnd.viewDef->viewport.x1 + 1;
		int	h = backEnd.viewDef->viewport.y2 - backEnd.viewDef->viewport.y1 + 1;
		
		RENDERLOG_PRINTF( "Resolve to %i x %i buffer\n", w, h );
		
		// resolve the screen
		globalImages->currentRenderImage->CopyFramebuffer( x, y, w, h );
		
		renderProgManager.BindShader_Brightpass();
	}
	
	float screenCorrectionParm[4];
	screenCorrectionParm[0] = backEnd.hdrKey;
	screenCorrectionParm[1] = backEnd.hdrAverageLuminance;
	screenCorrectionParm[2] = backEnd.hdrMaxLuminance;
	screenCorrectionParm[3] = 1.0f;
	SetFragmentParm( RENDERPARM_SCREENCORRECTIONFACTOR, screenCorrectionParm ); // rpScreenCorrectionFactor
	
	float overbright[4];
	if( r_useHDR.GetBool() )
	{
		if( r_hdrAutoExposure.GetBool() )
		{
			overbright[0] = r_hdrContrastDynamicThreshold.GetFloat();
		}
		else
		{
			overbright[0] = r_hdrContrastStaticThreshold.GetFloat();
		}
		overbright[1] = r_hdrContrastOffset.GetFloat();
		overbright[2] = 0;
		overbright[3] = 0;
	}
	else
	{
		overbright[0] = r_ldrContrastThreshold.GetFloat();
		overbright[1] = r_ldrContrastOffset.GetFloat();
		overbright[2] = 0;
		overbright[3] = 0;
	}
	SetFragmentParm( RENDERPARM_OVERBRIGHT, overbright ); // rpOverbright
	
	// Draw
	GL_DrawElementsWithCounters( &backEnd.unitSquareSurface );
	
	
	// BLOOM PING PONG rendering
	renderProgManager.BindShader_HDRGlareChromatic();
	
	int j;
	for( j = 0; j < r_hdrGlarePasses.GetInteger(); j++ )
	{
		globalFramebuffers.bloomRenderFBO[( j + 1 ) % 2 ]->Bind();
		glClear( GL_COLOR_BUFFER_BIT );
		
		globalImages->bloomRenderImage[j % 2]->Bind();
		
		GL_DrawElementsWithCounters( &backEnd.unitSquareSurface );
	}
	
	// add filtered glare back to main context
	Framebuffer::Unbind();
	
	RB_ResetViewportAndScissorToDefaultCamera( viewDef );
	
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
	
	renderProgManager.BindShader_Screen();
	
	globalImages->bloomRenderImage[( j + 1 ) % 2]->Bind();
	
	GL_DrawElementsWithCounters( &backEnd.unitSquareSurface );
	
	globalImages->BindNull();
	
	Framebuffer::Unbind();
	renderProgManager.Unbind();
	
	GL_State( GLS_DEFAULT );
	GL_Cull( CT_FRONT_SIDED );
}


static void RB_SSAO( const idRenderView* viewDef )
{
	if( !viewDef->viewEntitys || viewDef->is2Dgui )
	{
		// 3D views only
		return;
	}
	
	if( r_useSSAO.GetInteger() <= 0 )
	{
		return;
	}
	
	// FIXME very expensive to enable this in subviews
	if( viewDef->isSubview )
	{
		return;
	}
	
	RENDERLOG_PRINTF( "---------- RB_SSAO() ----------\n" );
	
#if 0
	GL_CheckErrors();
	
	// clear the alpha buffer and draw only the hands + weapon into it so
	// we can avoid blurring them
	glClearColor( 0, 0, 0, 1 );
	GL_State( GLS_COLORMASK | GLS_DEPTHMASK );
	glClear( GL_COLOR_BUFFER_BIT );
	GL_Color( 0, 0, 0, 0 );
	
	
	GL_SelectTexture( 0 );
	globalImages->blackImage->Bind();
	backEnd.currentSpace = NULL;
	
	drawSurf_t** drawSurfs = ( drawSurf_t** )&backEnd.viewDef->drawSurfs[0];
	for( int surfNum = 0; surfNum < backEnd.viewDef->numDrawSurfs; surfNum++ )
	{
		const drawSurf_t* surf = drawSurfs[ surfNum ];
		
		if( !surf->space->weaponDepthHack && !surf->space->skipMotionBlur && !surf->material->HasSubview() )
		{
			// Apply motion blur to this object
			continue;
		}
		
		const idMaterial* shader = surf->material;
		if( shader->Coverage() == MC_TRANSLUCENT )
		{
			// muzzle flash, etc
			continue;
		}
		
		// set mvp matrix
		if( surf->space != backEnd.currentSpace )
		{
			RB_SetMVP( surf->space->mvp );
			backEnd.currentSpace = surf->space;
		}
		
		// this could just be a color, but we don't have a skinned color-only prog
		if( surf->jointCache )
		{
			renderProgManager.BindShader_TextureVertexColorSkinned();
		}
		else
		{
			renderProgManager.BindShader_TextureVertexColor();
		}
		
		// draw it solid
		GL_DrawElementsWithCounters( surf );
	}
	GL_State( GLS_DEPTHFUNC_ALWAYS );
	
	// copy off the color buffer and the depth buffer for the motion blur prog
	// we use the viewport dimensions for copying the buffers in case resolution scaling is enabled.
	const idScreenRect& viewport = backEnd.viewDef->viewport;
	globalImages->currentRenderImage->CopyFramebuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );
	
	// in stereo rendering, each eye needs to get a separate previous frame mvp
	int mvpIndex = ( backEnd.viewDef->parms.viewEyeBuffer == 1 ) ? 1 : 0;
	
	// derive the matrix to go from current pixels to previous frame pixels
	idRenderMatrix	inverseMVP;
	idRenderMatrix::Inverse( backEnd.viewDef->GetMVPMatrix(), inverseMVP );
	
	idRenderMatrix	motionMatrix;
	idRenderMatrix::Multiply( backEnd.prevMVP[mvpIndex], inverseMVP, motionMatrix );
	
	backEnd.prevMVP[mvpIndex] = backEnd.viewDef->GetMVPMatrix();
	
	RB_SetMVP( motionMatrix );
#endif
	
	backEnd.currentSpace = &backEnd.viewDef->GetWorldSpace();
	RB_SetMVP( backEnd.viewDef->GetMVPMatrix() );
	
	const bool hdrIsActive = ( r_useHDR.GetBool() && globalFramebuffers.hdrFBO != NULL && globalFramebuffers.hdrFBO->IsBound() );
	
	int screenWidth = renderSystem->GetWidth();
	int screenHeight = renderSystem->GetHeight();
	
	// build hierarchical depth buffer
	if( r_useHierarchicalDepthBuffer.GetBool() )
	{
		renderProgManager.BindShader_AmbientOcclusionMinify();

		GL_SelectTexture( 0 );
		//globalImages->currentDepthImage->Bind();
		
		for( int i = 0; i < MAX_HIERARCHICAL_ZBUFFERS; i++ )
		{
			int width = globalFramebuffers.csDepthFBO[i]->GetWidth();
			int height = globalFramebuffers.csDepthFBO[i]->GetHeight();
			
			GL_ViewportAndScissor( 0, 0, width, height );
			
			globalFramebuffers.csDepthFBO[i]->Bind();
			
			GL_Clear( true, false, false, 0, 0.0, 0.0, 0.0, 1.0, false );
			
			if( i == 0 )
			{
				renderProgManager.BindShader_AmbientOcclusionReconstructCSZ();
				
				globalImages->currentDepthImage->Bind();
			}
			else
			{
				renderProgManager.BindShader_AmbientOcclusionMinify();
				
				GL_BindTexture( 0, globalImages->hierarchicalZbufferImage );
			}
			
			float jitterTexScale[4];
			jitterTexScale[0] = i - 1;
			jitterTexScale[1] = 0;
			jitterTexScale[2] = 0;
			jitterTexScale[3] = 0;
			SetFragmentParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale
#if 1
			float screenCorrectionParm[4];
			screenCorrectionParm[0] = 1.0f / width;
			screenCorrectionParm[1] = 1.0f / height;
			screenCorrectionParm[2] = width;
			screenCorrectionParm[3] = height;
			SetFragmentParm( RENDERPARM_SCREENCORRECTIONFACTOR, screenCorrectionParm ); // rpScreenCorrectionFactor
#endif
			
			GL_DrawElementsWithCounters( &backEnd.unitSquareSurface );
		}
	}
	
	// set the window clipping
	GL_ViewportAndScissor( 0, 0, screenWidth, screenHeight );
	
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
	GL_Cull( CT_TWO_SIDED );
	
	if( r_ssaoFiltering.GetBool() )
	{
		globalFramebuffers.ambientOcclusionFBO[0]->Bind();
		
		GL_Clear( true, false, false, 0, 0.0, 0.0, 0.0, 0.0, false );
		
		renderProgManager.BindShader_AmbientOcclusion();
	}
	else
	{
		if( r_ssaoDebug.GetInteger() <= 0 )
		{
			GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_ALPHAMASK | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
		}
		
		if( hdrIsActive )
		{
			globalFramebuffers.hdrFBO->Bind();
		}
		else
		{
			Framebuffer::Unbind();
		}
		
		renderProgManager.BindShader_AmbientOcclusionAndOutput();
	}
	
	float screenCorrectionParm[4];
	screenCorrectionParm[0] = 1.0f / screenWidth;
	screenCorrectionParm[1] = 1.0f / screenHeight;
	screenCorrectionParm[2] = screenWidth;
	screenCorrectionParm[3] = screenHeight;
	SetFragmentParm( RENDERPARM_SCREENCORRECTIONFACTOR, screenCorrectionParm ); // rpScreenCorrectionFactor
	
#if 0
	// RB: set unprojection matrices so we can convert zbuffer values back to camera and world spaces
	idRenderMatrix cameraToWorldMatrix;
	if( !idRenderMatrix::Inverse( backEnd.viewDef->GetViewMatrix(), cameraToWorldMatrix ) )
	{
		idLib::Warning( "cameraToWorldMatrix invert failed" );
	}
	
	SetVertexParms( RENDERPARM_MODELMATRIX_X, cameraToWorldMatrix[0], 4 );
	//SetVertexParms( RENDERPARM_MODELMATRIX_X, viewDef->unprojectionToWorldRenderMatrix[0], 4 );
#endif
	SetVertexParms( RENDERPARM_MODELMATRIX_X, viewDef->GetInverseProjMatrix().Ptr(), 4 );
	
	
	float jitterTexOffset[4];
	if( r_shadowMapRandomizeJitter.GetBool() )
	{
		jitterTexOffset[0] = ( rand() & 255 ) / 255.0;
		jitterTexOffset[1] = ( rand() & 255 ) / 255.0;
	}
	else
	{
		jitterTexOffset[0] = 0;
		jitterTexOffset[1] = 0;
	}
	jitterTexOffset[2] = viewDef->GetGameTimeSec();
	jitterTexOffset[3] = 0.0f;
	SetFragmentParm( RENDERPARM_JITTERTEXOFFSET, jitterTexOffset ); // rpJitterTexOffset
	
	GL_BindTexture( 0, globalImages->currentNormalsImage );
	GL_BindTexture( 1, ( r_useHierarchicalDepthBuffer.GetBool() )? globalImages->hierarchicalZbufferImage : globalImages->currentDepthImage );
	
	GL_DrawElementsWithCounters( &backEnd.unitSquareSurface );
	
	if( r_ssaoFiltering.GetBool() )
	{
		float jitterTexScale[4];
		
		// AO blur X
#if 1
		globalFramebuffers.ambientOcclusionFBO[1]->Bind();
		
		renderProgManager.BindShader_AmbientOcclusionBlur();
		
		// set axis parameter
		jitterTexScale[0] = 1;
		jitterTexScale[1] = 0;
		jitterTexScale[2] = 0;
		jitterTexScale[3] = 0;
		SetFragmentParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale
		
		GL_BindTexture( 2, globalImages->ambientOcclusionImage[ 0 ] );
		
		GL_DrawElementsWithCounters( &backEnd.unitSquareSurface );
#endif
		
		// AO blur Y
		if( hdrIsActive )
		{
			globalFramebuffers.hdrFBO->Bind();
		}
		else
		{
			Framebuffer::Unbind();
		}
		
		if( r_ssaoDebug.GetInteger() <= 0 )
		{
			GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
		}
		
		renderProgManager.BindShader_AmbientOcclusionBlurAndOutput();
		
		// set axis parameter
		jitterTexScale[0] = 0;
		jitterTexScale[1] = 1;
		jitterTexScale[2] = 0;
		jitterTexScale[3] = 0;
		SetFragmentParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale
		
		GL_BindTexture( 2, globalImages->ambientOcclusionImage[ 1 ] );
		
		GL_DrawElementsWithCounters( &backEnd.unitSquareSurface );
	}
	
	renderProgManager.Unbind();
	
	GL_State( GLS_DEFAULT );
	GL_Cull( CT_FRONT_SIDED );
	
	//GL_CheckErrors();
}

static void RB_SSGI( const idRenderView* viewDef )
{
	if( !viewDef->viewEntitys || viewDef->is2Dgui )
	{
		// 3D views only
		return;
	}
	
	if( r_useSSGI.GetInteger() <= 0 )
	{
		return;
	}
	
	// FIXME very expensive to enable this in subviews
	if( viewDef->isSubview )
	{
		return;
	}
	
	RENDERLOG_PRINTF( "---------- RB_SSGI() ----------\n" );
	
	backEnd.currentSpace = &backEnd.viewDef->GetWorldSpace();
	RB_SetMVP( backEnd.viewDef->GetMVPMatrix() );
	
	const bool hdrIsActive = ( r_useHDR.GetBool() && globalFramebuffers.hdrFBO != NULL && globalFramebuffers.hdrFBO->IsBound() );
	
	int screenWidth = renderSystem->GetWidth();
	int screenHeight = renderSystem->GetHeight();
	
	// set the window clipping
	GL_ViewportAndScissor( 0, 0, screenWidth, screenHeight );
	
	if( !hdrIsActive )
	{
		const idScreenRect& viewport = viewDef->viewport;
		globalImages->currentRenderImage->CopyFramebuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );
	}
	
	// build hierarchical depth buffer
	if( r_useHierarchicalDepthBuffer.GetBool() )
	{
		renderProgManager.BindShader_AmbientOcclusionMinify();
		
		///glClearColor( 0, 0, 0, 1 );
		
		GL_SelectTexture( 0 );
		//globalImages->currentDepthImage->Bind();
		
		for( int i = 0; i < MAX_HIERARCHICAL_ZBUFFERS; i++ )
		{
			int width = globalFramebuffers.csDepthFBO[i]->GetWidth();
			int height = globalFramebuffers.csDepthFBO[i]->GetHeight();
			
			GL_ViewportAndScissor( 0, 0, width, height );
			
			globalFramebuffers.csDepthFBO[i]->Bind();
			
			///glClear( GL_COLOR_BUFFER_BIT );
			GL_Clear( true, false, false, 0, 0.0, 0.0, 0.0, 1.0 , false );
			
			if( i == 0 )
			{
				renderProgManager.BindShader_AmbientOcclusionReconstructCSZ();
				
				globalImages->currentDepthImage->Bind();
			}
			else
			{
				renderProgManager.BindShader_AmbientOcclusionMinify();
				
				GL_BindTexture( 0, globalImages->hierarchicalZbufferImage );
			}
			
			float jitterTexScale[4];
			jitterTexScale[0] = i - 1;
			jitterTexScale[1] = 0;
			jitterTexScale[2] = 0;
			jitterTexScale[3] = 0;
			SetFragmentParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale
#if 1
			float screenCorrectionParm[4];
			screenCorrectionParm[0] = 1.0f / width;
			screenCorrectionParm[1] = 1.0f / height;
			screenCorrectionParm[2] = width;
			screenCorrectionParm[3] = height;
			SetFragmentParm( RENDERPARM_SCREENCORRECTIONFACTOR, screenCorrectionParm ); // rpScreenCorrectionFactor
#endif
			
			GL_DrawElementsWithCounters( &backEnd.unitSquareSurface );
		}
	}
	
	// set the window clipping
	GL_ViewportAndScissor( 0, 0, screenWidth, screenHeight );
	
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
	GL_Cull( CT_TWO_SIDED );
	
	if( r_ssgiFiltering.GetBool() )
	{
		globalFramebuffers.ambientOcclusionFBO[0]->Bind();
		
		// FIXME remove and mix with color from previous frame
		GL_Clear( true, false, false, 0, 0.0, 0.0, 0.0, 0.0, false );
		
		renderProgManager.BindShader_DeepGBufferRadiosity();
	}
	else
	{
		if( r_ssgiDebug.GetInteger() > 0 )
		{
			// replace current
			GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
		}
		else
		{
			// add result to main color
			GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
		}
		
		if( hdrIsActive )
		{
			globalFramebuffers.hdrFBO->Bind();
		}
		else
		{
			Framebuffer::Unbind();
		}
		
		renderProgManager.BindShader_DeepGBufferRadiosity();
	}
	
	float screenCorrectionParm[4];
	screenCorrectionParm[0] = 1.0f / screenWidth;
	screenCorrectionParm[1] = 1.0f / screenHeight;
	screenCorrectionParm[2] = screenWidth;
	screenCorrectionParm[3] = screenHeight;
	SetFragmentParm( RENDERPARM_SCREENCORRECTIONFACTOR, screenCorrectionParm ); // rpScreenCorrectionFactor
	
#if 0
	// RB: set unprojection matrices so we can convert zbuffer values back to camera and world spaces
	idRenderMatrix cameraToWorldMatrix;
	if( !idRenderMatrix::Inverse( backEnd.viewDef->GetViewMatrix(), cameraToWorldMatrix ) )
	{
		idLib::Warning( "cameraToWorldMatrix invert failed" );
	}
	
	SetVertexParms( RENDERPARM_MODELMATRIX_X, cameraToWorldMatrix[0], 4 );
	//SetVertexParms( RENDERPARM_MODELMATRIX_X, viewDef->unprojectionToWorldRenderMatrix[0], 4 );
#endif
	SetVertexParms( RENDERPARM_MODELMATRIX_X, viewDef->GetInverseProjMatrix().Ptr(), 4 );
	
	
	float jitterTexOffset[4];
	if( r_shadowMapRandomizeJitter.GetBool() )
	{
		jitterTexOffset[0] = ( rand() & 255 ) / 255.0;
		jitterTexOffset[1] = ( rand() & 255 ) / 255.0;
	}
	else
	{
		jitterTexOffset[0] = 0;
		jitterTexOffset[1] = 0;
	}
	jitterTexOffset[2] = viewDef->GetGameTimeSec();
	jitterTexOffset[3] = 0.0f;
	SetFragmentParm( RENDERPARM_JITTERTEXOFFSET, jitterTexOffset ); // rpJitterTexOffset
	
	GL_BindTexture( 0, globalImages->currentNormalsImage );
	GL_BindTexture( 1, ( r_useHierarchicalDepthBuffer.GetBool() )? globalImages->hierarchicalZbufferImage : globalImages->currentDepthImage );
	GL_BindTexture( 2, ( hdrIsActive )? globalImages->currentRenderHDRImage : globalImages->currentRenderImage );
	
	GL_DrawElementsWithCounters( &backEnd.unitSquareSurface );
	
	if( r_ssgiFiltering.GetBool() )
	{
		float jitterTexScale[4];
		
		// AO blur X
#if 1
		globalFramebuffers.ambientOcclusionFBO[1]->Bind();
		
		renderProgManager.BindShader_DeepGBufferRadiosityBlur();
		
		// set axis parameter
		jitterTexScale[0] = 1;
		jitterTexScale[1] = 0;
		jitterTexScale[2] = 0;
		jitterTexScale[3] = 0;
		SetFragmentParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale
		
		GL_BindTexture( 2, globalImages->ambientOcclusionImage[ 0 ] );
		
		GL_DrawElementsWithCounters( &backEnd.unitSquareSurface );
#endif
		
		// AO blur Y
		if( hdrIsActive )
		{
			globalFramebuffers.hdrFBO->Bind();
		}
		else
		{
			Framebuffer::Unbind();
		}
		
		if( r_ssgiDebug.GetInteger() > 0 )
		{
			// replace current
			GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
		}
		else
		{
			// add result to main color
			GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
		}
		
		renderProgManager.BindShader_DeepGBufferRadiosityBlurAndOutput();
		
		// set axis parameter
		jitterTexScale[0] = 0;
		jitterTexScale[1] = 1;
		jitterTexScale[2] = 0;
		jitterTexScale[3] = 0;
		SetFragmentParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale
		
		GL_BindTexture( 2, globalImages->ambientOcclusionImage[ 1 ] );
		
		GL_DrawElementsWithCounters( &backEnd.unitSquareSurface );
	}
	
	renderProgManager.Unbind();
	
	GL_State( GLS_DEFAULT );
	GL_Cull( CT_FRONT_SIDED );
	
	//GL_CheckErrors();
}
// RB end

/*
=========================================================================================================

	BUFFERS

=========================================================================================================
*/

typedef ALIGNTYPE16 idVec4	idRenderVector;
//ALIGNTYPE16 class idRenderVector : public idVec4 {
//public:
//};

void RB_InitBuffers()
{
	backEnd.viewCBuffer.AllocBufferObject( ( sizeof( idRenderMatrix ) * 6 ) + ( sizeof( idRenderVector ) * 2 ), BU_DEFAULT );
}

void RB_ShutdownBuffers()
{
	backEnd.viewCBuffer.FreeBufferObject();
}

void RB_UpdateBuffers( const idRenderView * const view )
{
	void * dest = backEnd.viewCBuffer.MapBuffer( BM_WRITE_NOSYNC );

	auto mats = ( idRenderMatrix * )dest;
	( mats++ )->Copy( view->GetViewMatrix() );
	( mats++ )->Copy( view->GetProjectionMatrix() );
	( mats++ )->Copy( view->GetMVPMatrix() );
	( mats++ )->Copy( view->GetInverseViewMatrix() );
	( mats++ )->Copy( view->GetInverseProjMatrix() );
	( mats++ )->Copy( view->GetInverseVPMatrix() );

	auto vecs = ( idRenderVector * )mats;
	*( vecs++ ) = idRenderVector( view->GetOrigin(), 1.0f );
	*( vecs++ ) = *( idRenderVector * )view->GetSplitFrustumDistances();

	backEnd.viewCBuffer.UnmapBuffer();
}

/*
=========================================================================================================

	BACKEND COMMANDS

=========================================================================================================
*/

/*
==================
RB_DrawViewInternal
==================
*/
void RB_DrawViewInternal( const idRenderView* viewDef, const int stereoEye )
{
	RENDERLOG_OPEN_BLOCK( "RB_DrawViewInternal" );
	
	//-------------------------------------------------
	// guis can wind up referencing purged images that need to be loaded.
	// this used to be in the gui emit code, but now that it can be running
	// in a separate thread, it must not try to load images, so do it here.
	//-------------------------------------------------
	const drawSurf_t * const * drawSurfs = ( drawSurf_t** )&viewDef->drawSurfs[0];
	const int numDrawSurfs = viewDef->numDrawSurfs;
	
	for( int i = 0; i < numDrawSurfs; ++i )
	{
		const drawSurf_t* ds = viewDef->drawSurfs[ i ];
		if( ds->material != NULL )
		{
			const_cast<idMaterial*>( ds->material )->EnsureNotPurged();
		}
	}
	
	//-------------------------------------------------
	// RB_BeginDrawingView
	//
	// Any mirrored or portaled views have already been drawn, so prepare
	// to actually render the visible surfaces for this view
	//
	// clear the z buffer, set the projection matrix, etc
	//-------------------------------------------------
	RB_ResetViewportAndScissorToDefaultCamera( viewDef );

	RB_UpdateBuffers( viewDef );
	
	backEnd.glState.faceCulling = -1;		// force face culling to set next time
	
	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT );
	
	//GL_CheckErrors();
	
	// RB begin
	const bool useHDR = r_useHDR.GetBool() && !viewDef->is2Dgui;
	
	// Clear the depth buffer and clear the stencil to 128 for stencil shadows as well as gui masking
	GL_Clear( false, true, true, STENCIL_SHADOW_TEST_VALUE, 0.0f, 0.0f, 0.0f, 0.0f, useHDR );
	
	if( useHDR )
	{
		globalFramebuffers.hdrFBO->Bind();
	}
	else
	{
		Framebuffer::Unbind();
	}
	// RB end
	
	//GL_CheckErrors();
	
	// normal face culling
	GL_Cull( CT_FRONT_SIDED );
	
#if !defined( USE_GLES3 )
	// bind one global Vertex Array Object (VAO)
	glBindVertexArray( glConfig.global_vao );
#endif
	
	//------------------------------------
	// sets variables that can be used by all programs
	//------------------------------------
	{
		//
		// set eye position in global space
		//
		float parm[4];
		parm[0] = backEnd.viewDef->GetOrigin()[0];
		parm[1] = backEnd.viewDef->GetOrigin()[1];
		parm[2] = backEnd.viewDef->GetOrigin()[2];
		parm[3] = 1.0f;
		
		SetVertexParm( RENDERPARM_GLOBALEYEPOS, parm ); // rpGlobalEyePos
		
		// sets overbright to make world brighter
		// This value is baked into the specularScale and diffuseScale values so
		// the interaction programs don't need to perform the extra multiply,
		// but any other renderprogs that want to obey the brightness value
		// can reference this.
		float overbright = r_lightScale.GetFloat() * 0.5f;
		parm[0] = overbright;
		parm[1] = overbright;
		parm[2] = overbright;
		parm[3] = overbright;
		SetFragmentParm( RENDERPARM_OVERBRIGHT, parm );
		
		// Set Projection Matrix
		SetVertexParms( RENDERPARM_PROJMATRIX_X, backEnd.viewDef->GetProjectionMatrix().Ptr(), 4 );

		const GLintptr ubo = reinterpret_cast< GLintptr >( backEnd.viewCBuffer.GetAPIObject() );
		glBindBufferRange( GL_UNIFORM_BUFFER, BINDING_GLOBAL_UBO, ubo, backEnd.viewCBuffer.GetOffset(), backEnd.viewCBuffer.GetSize() );
	}
	
	//-------------------------------------------------
	// fill the depth buffer and clear color buffer to black except on subviews
	//-------------------------------------------------
	RB_FillDepthBufferFast( drawSurfs, numDrawSurfs );
	
	//-------------------------------------------------
	// FIXME, OPTIMIZE: merge this with FillDepthBufferFast like in a light prepass deferred renderer
	//
	// fill the geometric buffer with normals and roughness
	//-------------------------------------------------
	RB_AmbientPass( drawSurfs, numDrawSurfs, true );
	
	//-------------------------------------------------
	// fill the depth buffer and the color buffer with precomputed Q3A style lighting
	//-------------------------------------------------
	RB_AmbientPass( drawSurfs, numDrawSurfs, false );
	
	//-------------------------------------------------
	// main light renderer
	//-------------------------------------------------
	RB_DrawInteractions( viewDef );
	
	//-------------------------------------------------
	// capture the depth for the motion blur before rendering any post process surfaces that may contribute to the depth
	//-------------------------------------------------
	if( ( r_motionBlur.GetInteger() > 0 ||  r_useSSAO.GetBool() || r_useSSGI.GetBool() ) && !r_useHDR.GetBool() )
	{
		const auto & viewport = backEnd.viewDef->GetViewport();
		globalImages->currentDepthImage->CopyDepthbuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );
	}
	
	//-------------------------------------------------
	// darken the scene using the screen space ambient occlusion
	//-------------------------------------------------
	RB_SSAO( viewDef );
	//RB_SSGI( viewDef );
	
	//-------------------------------------------------
	// now draw any non-light dependent shading passes
	//-------------------------------------------------
	int processed = 0;
	if( !r_skipShaderPasses.GetBool() )
	{
		RENDERLOG_OPEN_MAINBLOCK( MRB_DRAW_SHADER_PASSES );
		float guiScreenOffset;
		if( viewDef->viewEntitys != NULL )
		{
			// guiScreenOffset will be 0 in non-gui views
			guiScreenOffset = 0.0f;
		}
		else
		{
			guiScreenOffset = stereoEye * viewDef->GetStereoScreenSeparation();
		}
		processed = RB_DrawShaderPasses( drawSurfs, numDrawSurfs, guiScreenOffset, stereoEye );
		RENDERLOG_CLOSE_MAINBLOCK();
	}
	
	//-------------------------------------------------
	// use direct light and emissive light contributions to add indirect screen space light
	//-------------------------------------------------
	RB_SSGI( viewDef );
	
	//-------------------------------------------------
	// fog and blend lights, drawn after emissive surfaces
	// so they are properly dimmed down
	//-------------------------------------------------
	RB_FogAllLights();
	
	//-------------------------------------------------
	// now draw any screen warping post-process effects using _currentRender
	//-------------------------------------------------
	if( processed < numDrawSurfs && !r_skipPostProcess.GetBool() )
	{
		int x = backEnd.viewDef->GetViewport().x1;
		int y = backEnd.viewDef->GetViewport().y1;
		int	w = backEnd.viewDef->GetViewport().GetWidth();
		int	h = backEnd.viewDef->GetViewport().GetHeight();
		
		RENDERLOG_PRINTF( "Resolve to %i x %i buffer\n", w, h );
		
		GL_SelectTexture( 0 );
		
		// resolve the screen
		globalImages->currentRenderImage->CopyFramebuffer( x, y, w, h );
		backEnd.currentRenderCopied = true;
		
		// RENDERPARM_SCREENCORRECTIONFACTOR amd RENDERPARM_WINDOWCOORD overlap
		// diffuseScale and specularScale
		
		// screen power of two correction factor (no longer relevant now)
		float screenCorrectionParm[4];
		screenCorrectionParm[0] = 1.0f;
		screenCorrectionParm[1] = 1.0f;
		screenCorrectionParm[2] = 0.0f;
		screenCorrectionParm[3] = 1.0f;
		SetFragmentParm( RENDERPARM_SCREENCORRECTIONFACTOR, screenCorrectionParm ); // rpScreenCorrectionFactor
		
		// window coord to 0.0 to 1.0 conversion
		float windowCoordParm[4];
		windowCoordParm[0] = 1.0f / w;
		windowCoordParm[1] = 1.0f / h;
		windowCoordParm[2] = 0.0f;
		windowCoordParm[3] = 1.0f;
		SetFragmentParm( RENDERPARM_WINDOWCOORD, windowCoordParm ); // rpWindowCoord
		
		// render the remaining surfaces
		RENDERLOG_OPEN_MAINBLOCK( MRB_DRAW_SHADER_PASSES_POST );
		RB_DrawShaderPasses( drawSurfs + processed, numDrawSurfs - processed, 0.0f /* definitely not a gui */, stereoEye );
		RENDERLOG_CLOSE_MAINBLOCK();
	}
	
	//-------------------------------------------------
	// render debug tools
	//-------------------------------------------------
	RB_RenderDebugTools( drawSurfs, numDrawSurfs );
	
	// RB: convert back from HDR to LDR range
	if( useHDR )
	{
		/*
		int x = backEnd.viewDef->viewport.x1;
		int y = backEnd.viewDef->viewport.y1;
		int	w = backEnd.viewDef->viewport.x2 - backEnd.viewDef->viewport.x1 + 1;
		int	h = backEnd.viewDef->viewport.y2 - backEnd.viewDef->viewport.y1 + 1;
		
		GL_Viewport( viewDef->viewport.x1,
				viewDef->viewport.y1,
				viewDef->viewport.x2 + 1 - viewDef->viewport.x1,
				viewDef->viewport.y2 + 1 - viewDef->viewport.y1 );
		*/
		
		/*
		glBindFramebuffer( GL_READ_FRAMEBUFFER, globalFramebuffers.hdrFBO->GetFramebuffer() );
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, globalFramebuffers.hdrQuarterFBO->GetFramebuffer() );
		glBlitFramebuffer( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight(),
						   0, 0, renderSystem->GetWidth() * 0.25f, renderSystem->GetHeight() * 0.25f,
						   GL_COLOR_BUFFER_BIT,
						   GL_LINEAR );
		*/
		
#if defined(USE_HDR_MSAA)
		if( glConfig.multisamples > 0 )
		{
			glBindFramebuffer( GL_READ_FRAMEBUFFER, globalFramebuffers.hdrFBO->GetFramebuffer() );
			glBindFramebuffer( GL_DRAW_FRAMEBUFFER, globalFramebuffers.hdrNonMSAAFBO->GetFramebuffer() );
			glBlitFramebuffer( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight(),
							   0, 0, renderSystem->GetWidth(), renderSystem->GetHeight(),
							   GL_COLOR_BUFFER_BIT,
							   GL_LINEAR );
							   
			// TODO resolve to 1x1
			glBindFramebuffer( GL_READ_FRAMEBUFFER_EXT, globalFramebuffers.hdrNonMSAAFBO->GetFramebuffer() );
			glBindFramebuffer( GL_DRAW_FRAMEBUFFER_EXT, globalFramebuffers.hdr64FBO->GetFramebuffer() );
			glBlitFramebuffer( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight(),
							   0, 0, 64, 64,
							   GL_COLOR_BUFFER_BIT,
							   GL_LINEAR );
		}
		else
#endif
		{
			glBindFramebuffer( GL_READ_FRAMEBUFFER_EXT, globalFramebuffers.hdrFBO->GetFramebuffer() );
			glBindFramebuffer( GL_DRAW_FRAMEBUFFER_EXT, globalFramebuffers.hdr64FBO->GetFramebuffer() );
			glBlitFramebuffer( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight(),
							   0, 0, 64, 64,
							   GL_COLOR_BUFFER_BIT,
							   GL_LINEAR );
		}
		
		RB_CalculateAdaptation();
		
		RB_Tonemap( viewDef );
	}
	
	RB_Bloom( viewDef );
	// RB end
	
	RENDERLOG_CLOSE_BLOCK();
}

/*
==================
RB_MotionBlur

Experimental feature
==================
*/
void RB_MotionBlur()
{
	if( !backEnd.viewDef->viewEntitys )
	{
		// 3D views only
		return;
	}
	if( r_motionBlur.GetInteger() <= 0 )
	{
		return;
	}
	if( backEnd.viewDef->isSubview )
	{
		return;
	}
	
	GL_CheckErrors();
	
	// clear the alpha buffer and draw only the hands + weapon into it so
	// we can avoid blurring them
	GL_State( GLS_COLORMASK | GLS_DEPTHMASK );
	GL_Clear( true, false, false, 0, 0.0, 0.0, 0.0, 1.0, false );
	GL_Color( 0, 0, 0, 0 );
	GL_BindTexture( 0, globalImages->blackImage );
	
	backEnd.currentSpace = NULL;
	
	const drawSurf_t * const * drawSurfs = ( drawSurf_t** )&backEnd.viewDef->drawSurfs[0];
	for( int surfNum = 0; surfNum < backEnd.viewDef->numDrawSurfs; ++surfNum )
	{
		const drawSurf_t * const surf = drawSurfs[ surfNum ];
		
		if( !surf->space->weaponDepthHack && !surf->space->skipMotionBlur && !surf->material->HasSubview() )
		{
			// Apply motion blur to this object
			continue;
		}
		
		auto const material = surf->material;
		if( material->Coverage() == MC_TRANSLUCENT )
		{
			// muzzle flash, etc
			continue;
		}
		
		// set mvp matrix
		if( surf->space != backEnd.currentSpace )
		{
			RB_SetMVP( surf->space->mvp );
			backEnd.currentSpace = surf->space;
		}
		
		// this could just be a color, but we don't have a skinned color-only prog
		if( surf->jointCache )
		{
			renderProgManager.BindShader_TextureVertexColorSkinned();
		}
		else {
			renderProgManager.BindShader_TextureVertexColor();
		}
		
		// draw it solid
		GL_DrawElementsWithCounters( surf );
	}

	const bool useHDR = r_useHDR.GetBool() && !backEnd.viewDef->is2Dgui;

	GL_State( GLS_DEPTHFUNC_ALWAYS );

	//if( !useHDR ) {
		// copy off the color buffer and the depth buffer for the motion blur prog
		// we use the viewport dimensions for copying the buffers in case resolution scaling is enabled.
		const idScreenRect& viewport = backEnd.viewDef->GetViewport();
		globalImages->currentRenderImage->CopyFramebuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );
	//}
	// in stereo rendering, each eye needs to get a separate previous frame mvp
	const int mvpIndex = ( backEnd.viewDef->GetStereoEye() == 1 ) ? 1 : 0;
	
	// derive the matrix to go from current pixels to previous frame pixels	
	idRenderMatrix motionMatrix;
	idRenderMatrix::Multiply( backEnd.prevMVP[ mvpIndex ], backEnd.viewDef->GetInverseVPMatrix(), motionMatrix );

	backEnd.prevMVP[ mvpIndex ] = backEnd.viewDef->GetMVPMatrix();
	
	RB_SetMVP( motionMatrix );
	
	GL_State( GLS_DEPTHFUNC_ALWAYS );
	GL_Cull( CT_TWO_SIDED );
	
	renderProgManager.BindShader_MotionBlur();
	
	// let the fragment program know how many samples we are going to use
	const idRenderVector samples( ( float )( 1 << r_motionBlur.GetInteger() ) );
	SetFragmentParm( RENDERPARM_OVERBRIGHT, samples.ToFloatPtr() );	
	//if( !useHDR ) 
	//{
		GL_BindTexture( 0, globalImages->currentRenderImage );
		GL_BindTexture( 1, globalImages->currentDepthImage );
	//} 
	//else {
	//	GL_BindTexture( 0, globalImages->currentRenderHDRImage );
	//	GL_BindTexture( 1, globalImages->currentDepthImage );
	//}
	GL_DrawElementsWithCounters( &backEnd.unitSquareSurface );
	GL_CheckErrors();
}

/*
==================
RB_DrawView

StereoEye will always be 0 in mono modes, or -1 / 1 in stereo modes.
If the view is a GUI view that is repeated for both eyes, the viewDef.stereoEye value
is 0, so the stereoEye parameter is not always the same as that.
==================
*/
void RB_DrawView( const void* data, const int stereoEye )
{
	const drawSurfsCommand_t* cmd = ( const drawSurfsCommand_t* )data;
	
	backEnd.viewDef = cmd->viewDef;
	
	// we will need to do a new copyTexSubImage of the screen
	// when a SS_POST_PROCESS material is used
	backEnd.currentRenderCopied = false;
	
	// if there aren't any drawsurfs, do nothing
	if( !backEnd.viewDef->numDrawSurfs )
	{
		return;
	}
	
	// skip render bypasses everything that has models, assuming
	// them to be 3D views, but leaves 2D rendering visible
	if( r_skipRender.GetBool() && backEnd.viewDef->viewEntitys )
	{
		return;
	}
	
	// skip render context sets the wgl context to NULL,
	// which should factor out the API cost, under the assumption
	// that all gl calls just return if the context isn't valid
	
	// RB: not really needed
	//if( r_skipRenderContext.GetBool() && backEnd.viewDef->viewEntitys )
	//{
	//	GLimp_DeactivateContext();
	//}
	// RB end
	
	backEnd.pc.c_surfaces += backEnd.viewDef->numDrawSurfs;
	
	RB_ShowOverdraw();
	
	// render the scene
	RB_DrawViewInternal( cmd->viewDef, stereoEye );
	
	RB_MotionBlur();
	
	// restore the context for 2D drawing if we were stubbing it out
	// RB: not really needed
	//if( r_skipRenderContext.GetBool() && backEnd.viewDef->viewEntitys )
	//{
	//	GLimp_ActivateContext();
	//	GL_SetDefaultState();
	//}
	// RB end
	
	// optionally draw a box colored based on the eye number
	if( r_drawEyeColor.GetBool() )
	{
		const idScreenRect& r = backEnd.viewDef->GetViewport();
		GL_Scissor( ( r.x1 + r.x2 ) / 2, ( r.y1 + r.y2 ) / 2, 32, 32 );
		switch( stereoEye )
		{
			case -1:
				GL_Clear( true, false, false, 0, 1.0f, 0.0f, 0.0f, 1.0f );
				break;
			case 1:
				GL_Clear( true, false, false, 0, 0.0f, 1.0f, 0.0f, 1.0f );
				break;
			default:
				GL_Clear( true, false, false, 0, 0.5f, 0.5f, 0.5f, 1.0f );
				break;
		}
	}
}

/*
==================
RB_CopyRender

Copy part of the current framebuffer to an image
==================
*/
void RB_CopyRender( const void* data )
{
	const copyRenderCommand_t* cmd = ( const copyRenderCommand_t* )data;
	
	if( r_skipCopyTexture.GetBool() )
	{
		return;
	}
	
	RENDERLOG_PRINTF( "***************** RB_CopyRender *****************\n" );
	
	if( cmd->image )
	{
		cmd->image->CopyFramebuffer( cmd->x, cmd->y, cmd->imageWidth, cmd->imageHeight );
	}
	
	if( cmd->clearColorAfterCopy )
	{
		GL_Clear( true, false, false, STENCIL_SHADOW_TEST_VALUE, 0, 0, 0, 0 );
	}
}

/*
==================
RB_PostProcess

==================
*/
extern idCVar rs_enable;
void RB_PostProcess( const void* data )
{
	// only do the post process step if resolution scaling is enabled. Prevents the unnecessary copying of the framebuffer and
	// corresponding full screen quad pass.
	if( rs_enable.GetInteger() == 0 && !r_useFilmicPostProcessEffects.GetBool() && r_antiAliasing.GetInteger() == 0 )
	{
		return;
	}
	
	if( ( r_ssaoDebug.GetInteger() > 0 ) || ( r_ssgiDebug.GetInteger() > 0 ) )
	{
		return;
	}
	
	RENDERLOG_PRINTF( "---------- RB_PostProcess() ----------\n" );
	
	// resolve the scaled rendering to a temporary texture
	postProcessCommand_t* cmd = ( postProcessCommand_t* )data;
	const idScreenRect& viewport = cmd->viewDef->GetViewport();
	
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
	GL_Cull( CT_TWO_SIDED );
	
	int screenWidth = renderSystem->GetWidth();
	int screenHeight = renderSystem->GetHeight();
	
	// set the window clipping
	GL_ViewportAndScissor( 0, 0, screenWidth, screenHeight );
	
	// SMAA
	int aaMode = r_antiAliasing.GetInteger();
	if( aaMode == ANTI_ALIASING_SMAA_1X )
	{
		/*
		 * The shader has three passes, chained together as follows:
		 *
		 *                           |input|------------------�
		 *                              v                     |
		 *                    [ SMAA*EdgeDetection ]          |
		 *                              v                     |
		 *                          |edgesTex|                |
		 *                              v                     |
		 *              [ SMAABlendingWeightCalculation ]     |
		 *                              v                     |
		 *                          |blendTex|                |
		 *                              v                     |
		 *                [ SMAANeighborhoodBlending ] <------�
		 *                              v
		 *                           |output|
		*/
		
		globalImages->smaaInputImage->CopyFramebuffer( 0, 0, screenWidth, screenHeight );
		
		// set SMAA_RT_METRICS = rpScreenCorrectionFactor
		float screenCorrectionParm[4];
		screenCorrectionParm[0] = 1.0f / screenWidth;
		screenCorrectionParm[1] = 1.0f / screenHeight;
		screenCorrectionParm[2] = screenWidth;
		screenCorrectionParm[3] = screenHeight;
		SetFragmentParm( RENDERPARM_SCREENCORRECTIONFACTOR, screenCorrectionParm ); // rpScreenCorrectionFactor
		
		globalFramebuffers.smaaEdgesFBO->Bind();
		
		glClearColor( 0, 0, 0, 0 );
		glClear( GL_COLOR_BUFFER_BIT );
		///GL_Clear( true, false, false, 0, 0.0f, 0.0f, 0.0f, 0.0f );
		
		GL_BindTexture( 0, globalImages->smaaInputImage );
		
		renderProgManager.BindShader_SMAA_EdgeDetection();
		GL_DrawElementsWithCounters( &backEnd.unitSquareSurface );
		
#if 1
		//globalImages->smaaEdgesImage->CopyFramebuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );
		
		globalFramebuffers.smaaBlendFBO->Bind();
		//Framebuffer::Unbind();
		
		glClear( GL_COLOR_BUFFER_BIT );
		
		GL_BindTexture( 0, globalImages->smaaEdgesImage );
		GL_BindTexture( 1, globalImages->smaaAreaImage );
		GL_BindTexture( 2, globalImages->smaaSearchImage );
		
		renderProgManager.BindShader_SMAA_BlendingWeightCalculation();
		GL_DrawElementsWithCounters( &backEnd.unitSquareSurface );
		
		Framebuffer::Unbind();
#endif
		
#if 1
		globalImages->BindNull();
		
		//GL_SelectTexture( 0 );
		//globalImages->smaaBlendImage->CopyFramebuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );
		
		GL_BindTexture( 0, globalImages->smaaInputImage );
		GL_BindTexture( 1, globalImages->smaaBlendImage );
		
		renderProgManager.BindShader_SMAA_NeighborhoodBlending();
		GL_DrawElementsWithCounters( &backEnd.unitSquareSurface );
#endif
	}
	
#if 1
	if( r_useFilmicPostProcessEffects.GetBool() )
	{
		globalImages->currentRenderImage->CopyFramebuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );
		
		GL_BindTexture( 0, globalImages->currentRenderImage );
		GL_BindTexture( 1, globalImages->grainImage1 );
		
		renderProgManager.BindShader_PostProcess();
		
		const static int GRAIN_SIZE = 128;
		
		// screen power of two correction factor
		float screenCorrectionParm[4];
		screenCorrectionParm[0] = 1.0f / GRAIN_SIZE;
		screenCorrectionParm[1] = 1.0f / GRAIN_SIZE;
		screenCorrectionParm[2] = 1.0f;
		screenCorrectionParm[3] = 1.0f;
		SetFragmentParm( RENDERPARM_SCREENCORRECTIONFACTOR, screenCorrectionParm ); // rpScreenCorrectionFactor
		
		float jitterTexOffset[4];
		if( r_shadowMapRandomizeJitter.GetBool() )
		{
			jitterTexOffset[0] = ( rand() & 255 ) / 255.0;
			jitterTexOffset[1] = ( rand() & 255 ) / 255.0;
		}
		else {
			jitterTexOffset[0] = 0;
			jitterTexOffset[1] = 0;
		}
		jitterTexOffset[2] = 0.0f;
		jitterTexOffset[3] = 0.0f;
		SetFragmentParm( RENDERPARM_JITTERTEXOFFSET, jitterTexOffset ); // rpJitterTexOffset
		
		// Draw
		GL_DrawElementsWithCounters( &backEnd.unitSquareSurface );
	}
#endif
	
	GL_SelectTexture( 2 );
	globalImages->BindNull();
	
	GL_SelectTexture( 1 );
	globalImages->BindNull();
	
	GL_SelectTexture( 0 );
	globalImages->BindNull();
	
	renderProgManager.Unbind();
	
	RENDERLOG_CLOSE_BLOCK();
}
