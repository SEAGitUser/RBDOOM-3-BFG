/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2015 Robert Beckebans

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
#include "../../framework/Common_local.h"

idCVar r_drawFlickerBox( "r_drawFlickerBox", "0", CVAR_RENDERER | CVAR_BOOL, "visual test for dropping frames" );
idCVar stereoRender_warp( "stereoRender_warp", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "use the optical warping renderprog instead of stereoDeGhost" );
idCVar stereoRender_warpStrength( "stereoRender_warpStrength", "1.45", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "amount of pre-distortion" );

idCVar stereoRender_warpCenterX( "stereoRender_warpCenterX", "0.5", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "center for left eye, right eye will be 1.0 - this" );
idCVar stereoRender_warpCenterY( "stereoRender_warpCenterY", "0.5", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "center for both eyes" );
idCVar stereoRender_warpParmZ( "stereoRender_warpParmZ", "0", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "development parm" );
idCVar stereoRender_warpParmW( "stereoRender_warpParmW", "0", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "development parm" );
idCVar stereoRender_warpTargetFraction( "stereoRender_warpTargetFraction", "1.0", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "fraction of half-width the through-lens view covers" );

/*
============================================================================

					RENDER BACK END THREAD FUNCTIONS

============================================================================
*/

/*
=============
RB_SetBuffer
=============
*/
static void	RB_SetBuffer( const void* data )
{
	// see which draw buffer we want to render the frame to

	auto cmd = ( const setBufferCommand_t* )data;

	RENDERLOG_PRINT( "---------- RB_SetBuffer ---------- to buffer # %d\n", cmd->buffer );

	GL_Scissor( 0, 0, tr.GetWidth(), tr.GetHeight() );

	// clear screen for debugging
	// automatically enable this with several other debug tools
	// that might leave unrendered portions of the screen
	if( r_clear.GetFloat() || idStr::Length( r_clear.GetString() ) != 1 || r_singleArea.GetBool() || r_showOverDraw.GetBool() )
	{
		float c[ 3 ];
		if( sscanf( r_clear.GetString(), "%f %f %f", &c[ 0 ], &c[ 1 ], &c[ 2 ] ) == 3 )
		{
			GL_Clear( true, false, false, 0, c[ 0 ], c[ 1 ], c[ 2 ], 1.0f );
		}
		else if( r_clear.GetInteger() == 2 )
		{
			GL_Clear( true, false, false, 0, 0.0f, 0.0f, 0.0f, 1.0f );
		}
		else if( r_showOverDraw.GetBool() )
		{
			GL_Clear( true, false, false, 0, 1.0f, 1.0f, 1.0f, 1.0f );
		}
		else {
			GL_Clear( true, false, false, 0, 0.4f, 0.0f, 0.25f, 1.0f );
		}
	}
}

/*
====================
R_MakeStereoRenderImage
====================
*/
static void R_MakeStereoRenderImage( idImage* image )
{
	idImageOpts	opts;
	opts.width = renderSystem->GetWidth();
	opts.height = renderSystem->GetHeight();
	opts.numLevels = 1;
	opts.format = FMT_RGBA8;

	idSamplerOpts smp;
	smp.filter = TF_LINEAR;
	smp.wrap = TR_CLAMP;

	image->AllocImage( opts, smp );
}

/*
====================
RB_StereoRenderExecuteBackEndCommands

Renders the draw list twice, with slight modifications for left eye / right eye
====================
*/
void RB_StereoRenderExecuteBackEndCommands( const emptyCommand_t* const allCmds )
{
	uint64 backEndStartTime = sys->Microseconds();

	// If we are in a monoscopic context, this draws to the only buffer, and is
	// the same as GL_BACK.  In a quad-buffer stereo context, this is necessary
	// to prevent GL from forcing the rendering to go to both BACK_LEFT and
	// BACK_RIGHT at a performance penalty.
	// To allow stereo deghost processing, the views have to be copied to separate
	// textures anyway, so there isn't any benefit to rendering to BACK_RIGHT for
	// that eye.
	glDrawBuffer( GL_BACK_LEFT );

	// create the stereoRenderImage if we haven't already
	static idImage* stereoRenderImages[2];
	for( int i = 0; i < 2; i++ )
	{
		if( stereoRenderImages[i] == NULL )
		{
			stereoRenderImages[i] = renderImageManager->ImageFromFunction( va( "_stereoRender%i", i ), R_MakeStereoRenderImage );
		}

		// resize the stereo render image if the main window has changed size
		if( stereoRenderImages[i]->GetUploadWidth() != renderSystem->GetWidth() ||
			stereoRenderImages[i]->GetUploadHeight() != renderSystem->GetHeight() )
		{
			stereoRenderImages[i]->Resize( renderSystem->GetWidth(), renderSystem->GetHeight() );
		}
	}

	// In stereoRender mode, the front end has generated two RC_DRAW_VIEW commands
	// with slightly different origins for each eye.

	// TODO: only do the copy after the final view has been rendered, not mirror subviews?

	// Render the 3D draw views from the screen origin so all the screen relative
	// texture mapping works properly, then copy the portion we are going to use
	// off to a texture.
	bool foundEye[2] = { false, false };

	for( int stereoEye = 1; stereoEye >= -1; stereoEye -= 2 )
	{
		// set up the target texture we will draw to
		const int targetEye = ( stereoEye == 1 )? 1 : 0;

		// Set the back end into a known default state to fix any stale render state issues
		GL_SetDefaultState();
		renderProgManager.ZeroUniforms();

		for( const emptyCommand_t* cmds = allCmds; cmds != NULL; cmds = ( const emptyCommand_t* )cmds->next )
		{
			switch( cmds->commandId )
			{
				case RC_NOP:
					break;

				case RC_DRAW_VIEW_GUI:
				case RC_DRAW_VIEW_3D:
				{
					const drawSurfsCommand_t * const dsc = ( const drawSurfsCommand_t* )cmds;
					const idRenderView & eyeViewDef = *dsc->viewDef;

					if( eyeViewDef.GetStereoEye() && eyeViewDef.GetStereoEye() != stereoEye )
					{
						// this is the render view for the other eye
						continue;
					}

					foundEye[ targetEye ] = true;
					RB_CMD_DrawView( dsc, stereoEye );
					if( cmds->commandId == RC_DRAW_VIEW_GUI )
					{
					}
				}
				break;

				case RC_SET_BUFFER:
					RB_SetBuffer( cmds );
					break;

				case RC_COPY_RENDER:
					RB_CMD_CopyRender( cmds );
					break;

				case RC_POST_PROCESS: {
					postProcessCommand_t* cmd = ( postProcessCommand_t* )cmds;
					if( cmd->viewDef->GetStereoEye() != stereoEye )
					{
						break;
					}
					RB_CMD_PostProcess( cmds );
				}
				break;

				default:
					common->Error( "RB_ExecuteBackEndCommands: bad commandId" );
					break;
			}
		}

		// copy to the target
		GL_CopyCurrentColorToTexture( stereoRenderImages[ targetEye ], 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() );
	}

	// perform the final compositing / warping / deghosting to the actual framebuffer(s)
	assert( foundEye[0] && foundEye[1] );

	GL_SetDefaultState();

	renderProgManager.SetMVPMatrixParms( renderMatrix_identity );

	// If we are in quad-buffer pixel format but testing another 3D mode,
	// make sure we draw to both eyes.  This is likely to be sub-optimal
	// performance on most cards and drivers, but it is better than getting
	// a confusing, half-ghosted view.
	if( renderSystem->GetStereo3DMode() != STEREO3D_QUAD_BUFFER )
	{
		glDrawBuffer( GL_BACK );
	}

	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_TWOSIDED );

	// We just want to do a quad pass - so make sure we disable any texgen and
	// set the texture matrix to the identity so we don't get anomalies from
	// any stale uniform data being present from a previous draw call
	const float texS[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
	const float texT[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
	renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_S, texS );
	renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_T, texT );

	// disable any texgen
	const float texGenEnabled[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_ENABLED, texGenEnabled );

	GL_SetRenderProgram( renderProgManager.prog_texture );

	renderProgManager.SetColorParm( 1, 1, 1, 1 );

	switch( renderSystem->GetStereo3DMode() )
	{
		case STEREO3D_QUAD_BUFFER:
			glDrawBuffer( GL_BACK_RIGHT );
			renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USERMAP0 + 0 ), stereoRenderImages[ 1 ] );
			renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USERMAP0 + 1 ), stereoRenderImages[ 0 ] );
			GL_DrawIndexed( &backEnd.unitSquareSurface );

			glDrawBuffer( GL_BACK_LEFT );
			renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USERMAP0 + 0 ), stereoRenderImages[ 0 ] );
			renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USERMAP0 + 1 ), stereoRenderImages[ 1 ] );
			GL_DrawIndexed( &backEnd.unitSquareSurface );

			break;
		case STEREO3D_HDMI_720: // HDMI 720P 3D
			GL_ViewportAndScissor( 0, 0, 1280, 720 );
			renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USERMAP0 + 0 ), stereoRenderImages[ 1 ] );
			renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USERMAP0 + 1 ), stereoRenderImages[ 0 ] );
			GL_DrawIndexed( &backEnd.unitSquareSurface );

			GL_ViewportAndScissor( 0, 750, 1280, 720 );
			renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USERMAP0 + 0 ), stereoRenderImages[ 0 ] );
			renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USERMAP0 + 1 ), stereoRenderImages[ 1 ] );
			GL_DrawIndexed( &backEnd.unitSquareSurface );

			// force the HDMI 720P 3D guard band to a constant color
			GL_Scissor( 0, 720, 1280, 30 );
			glClear( GL_COLOR_BUFFER_BIT );
			break;

		default:
		case STEREO3D_SIDE_BY_SIDE:
			if( stereoRender_warp.GetBool() )
			{
				// this is the Rift warp
				// renderSystem->GetWidth() / GetHeight() have returned equal values (640 for initial Rift)
				// and we are going to warp them onto a symetric square region of each half of the screen

				GL_SetRenderProgram( renderProgManager.prog_stereoWarp );

				// clear the entire screen to black
				// we could be smart and only clear the areas we aren't going to draw on, but
				// clears are fast...
				GL_Scissor( 0, 0, glConfig.nativeScreenWidth, glConfig.nativeScreenHeight );
				GL_Clear( true, false, false, 0, 0.0, 0.0, 0.0, 0.0 );

				// the size of the box that will get the warped pixels
				// With the 7" displays, this will be less than half the screen width
				const int pixelDimensions = ( glConfig.nativeScreenWidth >> 1 ) * stereoRender_warpTargetFraction.GetFloat();

				// Always scissor to the half-screen boundary, but the viewports
				// might cross that boundary if the lenses can be adjusted closer
				// together.
				GL_Viewport( ( glConfig.nativeScreenWidth >> 1 ) - pixelDimensions, ( glConfig.nativeScreenHeight >> 1 ) - ( pixelDimensions >> 1 ), pixelDimensions, pixelDimensions );
				GL_Scissor( 0, 0, glConfig.nativeScreenWidth >> 1, glConfig.nativeScreenHeight );

				idVec4	color( stereoRender_warpCenterX.GetFloat(), stereoRender_warpCenterY.GetFloat(), stereoRender_warpParmZ.GetFloat(), stereoRender_warpParmW.GetFloat() );
				// don't use renderProgManager.SetColorParm(), because we don't want to clamp
				renderProgManager.SetRenderParm( RENDERPARM_COLOR, color.ToFloatPtr() );

				renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USERMAP0 + 0 ), stereoRenderImages[ 0 ] );
				glTextureParameterfEXT( GetGLObject( stereoRenderImages[ 0 ]->GetAPIObject() ), GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
				glTextureParameterfEXT( GetGLObject( stereoRenderImages[ 0 ]->GetAPIObject() ), GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );

				GL_DrawIndexed( &backEnd.unitSquareSurface );

				idVec4	color2( stereoRender_warpCenterX.GetFloat(), stereoRender_warpCenterY.GetFloat(), stereoRender_warpParmZ.GetFloat(), stereoRender_warpParmW.GetFloat() );
				// don't use renderProgManager.SetColorParm(), because we don't want to clamp
				renderProgManager.SetRenderParm( RENDERPARM_COLOR, color2.ToFloatPtr() );

				GL_Viewport( ( glConfig.nativeScreenWidth >> 1 ), ( glConfig.nativeScreenHeight >> 1 ) - ( pixelDimensions >> 1 ), pixelDimensions, pixelDimensions );
				GL_Scissor( glConfig.nativeScreenWidth >> 1, 0, glConfig.nativeScreenWidth >> 1, glConfig.nativeScreenHeight );

				renderProgManager.SetRenderParm( ( renderParm_t ) ( RENDERPARM_USERMAP0 + 0 ), stereoRenderImages[ 1 ] );
				glTextureParameterfEXT( GetGLObject( stereoRenderImages[ 1 ]->GetAPIObject() ), GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
				glTextureParameterfEXT( GetGLObject( stereoRenderImages[ 1 ]->GetAPIObject() ), GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );

				GL_DrawIndexed( &backEnd.unitSquareSurface );
				break;
			}
		// a non-warped side-by-side-uncompressed (dual input cable) is rendered
		// just like STEREO3D_SIDE_BY_SIDE_COMPRESSED, so fall through.
		case STEREO3D_SIDE_BY_SIDE_COMPRESSED:
			GL_ViewportAndScissor( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() );
			renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USERMAP0 + 0 ), stereoRenderImages[ 0 ] );
			renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USERMAP0 + 1 ), stereoRenderImages[ 1 ] );
			GL_DrawIndexed( &backEnd.unitSquareSurface );

			GL_ViewportAndScissor( renderSystem->GetWidth(), 0, renderSystem->GetWidth(), renderSystem->GetHeight() );
			renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USERMAP0 + 0 ), stereoRenderImages[ 1 ] );
			renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USERMAP0 + 1 ), stereoRenderImages[ 0 ] );
			GL_DrawIndexed( &backEnd.unitSquareSurface );
			break;

		case STEREO3D_TOP_AND_BOTTOM_COMPRESSED:
			GL_ViewportAndScissor( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() );
			renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USERMAP0 + 0 ), stereoRenderImages[ 1 ] );
			renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USERMAP0 + 1 ), stereoRenderImages[ 0 ] );
			GL_DrawIndexed( &backEnd.unitSquareSurface );

			GL_ViewportAndScissor( 0, renderSystem->GetHeight(), renderSystem->GetWidth(), renderSystem->GetHeight() );
			renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USERMAP0 + 0 ), stereoRenderImages[ 0 ] );
			renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USERMAP0 + 1 ), stereoRenderImages[ 1 ] );
			GL_DrawIndexed( &backEnd.unitSquareSurface );
			break;

		case STEREO3D_INTERLACED: // every other scanline
			renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USERMAP0 + 0 ), stereoRenderImages[ 0 ] );
			glTextureParameterfEXT( GetGLObject( stereoRenderImages[ 0 ]->GetAPIObject() ), GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTextureParameterfEXT( GetGLObject( stereoRenderImages[ 0 ]->GetAPIObject() ), GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

			renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USERMAP0 + 1 ), stereoRenderImages[ 1 ] );
			glTextureParameterfEXT( GetGLObject( stereoRenderImages[ 1 ]->GetAPIObject() ), GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTextureParameterfEXT( GetGLObject( stereoRenderImages[ 1 ]->GetAPIObject() ), GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

			GL_ViewportAndScissor( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() * 2 );
			GL_SetRenderProgram( renderProgManager.prog_stereoInterlace );
			GL_DrawIndexed( &backEnd.unitSquareSurface );

			glTextureParameterfEXT( GetGLObject( stereoRenderImages[ 0 ]->GetAPIObject() ), GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTextureParameterfEXT( GetGLObject( stereoRenderImages[ 0 ]->GetAPIObject() ), GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

			glTextureParameterfEXT( GetGLObject( stereoRenderImages[ 1 ]->GetAPIObject() ), GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTextureParameterfEXT( GetGLObject( stereoRenderImages[ 1 ]->GetAPIObject() ), GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			break;
	}

	// make sure the drawing is actually started
	GL_Flush();

	// we may choose to sync to the swapbuffers before the next frame

	// stop rendering on this thread
	uint64 backEndFinishTime = sys->Microseconds();
	backEnd.pc.totalMicroSec = backEndFinishTime - backEndStartTime;
}

/*
====================
 RB_ExecuteBackEndCommands

	This function will be called syncronously if running without
	smp extensions, or asyncronously by another thread.
====================
*/
void RB_ExecuteBackEndCommands( const emptyCommand_t* cmds )
{
	if( cmds->commandId == RC_NOP && !cmds->next )
		return;

	// r_debugRenderToTexture
	int c_draw3d = 0;
	int c_draw2d = 0;
	int c_setBuffers = 0;
	int c_copyRenders = 0;

	resolutionScale.SetCurrentGPUFrameTime( commonLocal.GetRendererGPUMicroseconds() );

	RENDERLOG_START_FRAME();
	GL_StartFrame( 0 );

	// upload any image loads that have completed
	//globalImages->CompleteBackgroundImageLoads();

	if( renderSystem->GetStereo3DMode() != STEREO3D_OFF )
	{
		// If we have a stereo pixel format, this will draw to both
		// the back left and back right buffers, which will have a
		// performance penalty.

		RB_StereoRenderExecuteBackEndCommands( cmds );

		GL_EndFrame();
		RENDERLOG_END_FRAME();
		return;
	}

	uint64 backEndStartTime = sys->Microseconds();

	// needed for editor rendering
	GL_SetDefaultState();

	for(/**/; cmds != NULL; cmds = ( const emptyCommand_t* )cmds->next )
	{
		switch( cmds->commandId )
		{
			case RC_NOP:
				break;

			case RC_DRAW_VIEW_3D:
				RB_CMD_DrawView( cmds, 0 );
				++c_draw3d;
				break;

			case RC_DRAW_VIEW_GUI:
				RB_CMD_DrawView( cmds, 0 );
				++c_draw2d;
				break;

			case RC_SET_BUFFER:
				//RB_SetBuffer( cmds );
				++c_setBuffers;
				break;

			case RC_COPY_RENDER:
				RB_CMD_CopyRender( cmds );
				++c_copyRenders;
				break;

			case RC_POST_PROCESS:
				RB_CMD_PostProcess( cmds );
				break;

			//case RC_SWAP_BUFFERS:
			//	RB_SwapBuffers( cmds );
			//	++c_swapBuffers;
			//	break;

			default:
				common->Error( "RB_ExecuteBackEndCommands: bad commandId" );
				break;
		}
	}

	//GL_SetNativeRenderDestination();

	GL_EndFrame();

	// stop rendering on this thread
	uint64 backEndFinishTime = sys->Microseconds();
	backEnd.pc.totalMicroSec = backEndFinishTime - backEndStartTime;

	if( r_debugRenderToTexture.GetInteger() == 1 )
	{
		common->Printf( "3d: %i, 2d: %i, SetBuf: %i, CpyRenders: %i, CpyFrameBuf: %i\n", c_draw3d, c_draw2d, c_setBuffers, c_copyRenders, backEnd.pc.c_copyFrameBuffer );
		backEnd.pc.c_copyFrameBuffer = 0;
	}

	RENDERLOG_END_FRAME();
}



