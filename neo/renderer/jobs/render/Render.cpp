
#include "precompiled.h"
#pragma hdrstop

#include "Render.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

idCVar r_drawEyeColor( "r_drawEyeColor", "0", CVAR_RENDERER | CVAR_BOOL, "Draw a colored box, red = left eye, blue = right eye, grey = non-stereo" );
idCVar r_skipTransMaterialPasses( "r_skipTransMaterialPasses", "0", CVAR_RENDERER | CVAR_BOOL, "" );

idCVar r_useHDR( "r_useHDR", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "use high dynamic range rendering" );

idCVar r_useFilmicPostProcessEffects( "r_useFilmicPostProcessEffects", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "apply several post process effects to mimic a filmic look" );
idCVar r_forceAmbient( "r_forceAmbient", "0.08", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "render additional ambient pass to make the game less dark", 0.0f, 0.5f );

idCVar r_exposure( "r_exposure", "0.5", CVAR_ARCHIVE | CVAR_RENDERER | CVAR_FLOAT, "HDR exposure or LDR brightness [0.0 .. 1.0]", 0.0f, 1.0f );
// RB end

idCVar r_useDeferredRender( "r_useDeferredRender", "0", CVAR_RENDERER | CVAR_BOOL, "" );

backEndState_t	backEnd;

/////////////////////////////////////////////////////////////////////////////////////////////////////

/*
=====================
 RB_BakeTextureMatrixIntoTexgen
=====================
*/
void RB_BakeTextureMatrixIntoTexgen( idPlane lightProject[ 3 ], const float* textureMatrix )
{
#if USE_INTRINSICS

	__m128 a0 = _mm_load_ps( lightProject[ 0 ].ToFloatPtr() );
	__m128 a1 = _mm_load_ps( lightProject[ 1 ].ToFloatPtr() );
	__m128 a2 = _mm_setzero_ps();
	__m128 a3 = _mm_load_ps( lightProject[ 2 ].ToFloatPtr() );

	__m128 r0 = _mm_unpacklo_ps( a0, a2 );
	__m128 r1 = _mm_unpackhi_ps( a0, a2 );
	__m128 r2 = _mm_unpacklo_ps( a1, a3 );
	__m128 r3 = _mm_unpackhi_ps( a1, a3 );

		   a0 = _mm_unpacklo_ps( r0, r2 );
		   a1 = _mm_unpackhi_ps( r0, r2 );
		   a2 = _mm_unpacklo_ps( r1, r3 );
		   a3 = _mm_unpackhi_ps( r1, r3 );

	__m128 b0 = _mm_load_ps( textureMatrix + 0 * 4 );
	__m128 b1 = _mm_load_ps( textureMatrix + 1 * 4 );
	__m128 b2 = _mm_load_ps( textureMatrix + 2 * 4 );
	__m128 b3 = _mm_load_ps( textureMatrix + 3 * 4 );

	__m128 t0 = _mm_mul_ps( _mm_splat_ps( a0, 0 ), b0 );
	__m128 t1 = _mm_mul_ps( _mm_splat_ps( a1, 0 ), b0 );
	__m128 t2 = _mm_mul_ps( _mm_splat_ps( a2, 0 ), b0 );
	__m128 t3 = _mm_mul_ps( _mm_splat_ps( a3, 0 ), b0 );

		   t0 = _mm_madd_ps( _mm_splat_ps( a0, 1 ), b1, t0 );
		   t1 = _mm_madd_ps( _mm_splat_ps( a1, 1 ), b1, t1 );
		   t2 = _mm_madd_ps( _mm_splat_ps( a2, 1 ), b1, t2 );
		   t3 = _mm_madd_ps( _mm_splat_ps( a3, 1 ), b1, t3 );

		   t0 = _mm_madd_ps( _mm_splat_ps( a0, 2 ), b2, t0 );
		   t1 = _mm_madd_ps( _mm_splat_ps( a1, 2 ), b2, t1 );
		   t2 = _mm_madd_ps( _mm_splat_ps( a2, 2 ), b2, t2 );
		   t3 = _mm_madd_ps( _mm_splat_ps( a3, 2 ), b2, t3 );

		   t0 = _mm_madd_ps( _mm_splat_ps( a0, 3 ), b3, t0 );
		   t1 = _mm_madd_ps( _mm_splat_ps( a1, 3 ), b3, t1 );
		   t2 = _mm_madd_ps( _mm_splat_ps( a2, 3 ), b3, t2 );
		   t3 = _mm_madd_ps( _mm_splat_ps( a3, 3 ), b3, t3 );

		   r0 = _mm_unpacklo_ps( t0, t2 );
		   r2 = _mm_unpacklo_ps( t1, t3 );

		   t0 = _mm_unpacklo_ps( r0, r2 );
		   t1 = _mm_unpackhi_ps( r0, r2 );

	_mm_store_ps( lightProject[ 0 ].ToFloatPtr(), t0 );
	_mm_store_ps( lightProject[ 1 ].ToFloatPtr(), t1 );

#else
	ALIGN16( float genMatrix[ 16 ] );
	ALIGN16( float final[ 16 ] );

	genMatrix[ 0 * 4 + 0 ] = lightProject[ 0 ][ 0 ];
	genMatrix[ 1 * 4 + 0 ] = lightProject[ 0 ][ 1 ];
	genMatrix[ 2 * 4 + 0 ] = lightProject[ 0 ][ 2 ];
	genMatrix[ 3 * 4 + 0 ] = lightProject[ 0 ][ 3 ];

	genMatrix[ 0 * 4 + 1 ] = lightProject[ 1 ][ 0 ];
	genMatrix[ 1 * 4 + 1 ] = lightProject[ 1 ][ 1 ];
	genMatrix[ 2 * 4 + 1 ] = lightProject[ 1 ][ 2 ];
	genMatrix[ 3 * 4 + 1 ] = lightProject[ 1 ][ 3 ];

	genMatrix[ 0 * 4 + 2 ] = 0.0f;
	genMatrix[ 1 * 4 + 2 ] = 0.0f;
	genMatrix[ 2 * 4 + 2 ] = 0.0f;
	genMatrix[ 3 * 4 + 2 ] = 0.0f;

	genMatrix[ 0 * 4 + 3 ] = lightProject[ 2 ][ 0 ];
	genMatrix[ 1 * 4 + 3 ] = lightProject[ 2 ][ 1 ];
	genMatrix[ 2 * 4 + 3 ] = lightProject[ 2 ][ 2 ];
	genMatrix[ 3 * 4 + 3 ] = lightProject[ 2 ][ 3 ];

	idRenderMatrix::Multiply( *( idRenderMatrix* )genMatrix, *( idRenderMatrix* )textureMatrix, *( idRenderMatrix* )final );

	lightProject[ 0 ][ 0 ] = final[ 0 * 4 + 0 ];
	lightProject[ 0 ][ 1 ] = final[ 1 * 4 + 0 ];
	lightProject[ 0 ][ 2 ] = final[ 2 * 4 + 0 ];
	lightProject[ 0 ][ 3 ] = final[ 3 * 4 + 0 ];

	lightProject[ 1 ][ 0 ] = final[ 0 * 4 + 1 ];
	lightProject[ 1 ][ 1 ] = final[ 1 * 4 + 1 ];
	lightProject[ 1 ][ 2 ] = final[ 2 * 4 + 1 ];
	lightProject[ 1 ][ 3 ] = final[ 3 * 4 + 1 ];
#endif
}

#if 0
/*
==================
 RB_EXP_SetRenderBuffer

	This may be to a hdr buffer, and scissor is set to cover only the light
==================
*/
void RB_SetBaseRenderDestination( const idRenderView * const view, const viewLight_t * const light )
{
	const idVec2i viewportOffset( 0, 0 );

	if( r_useHDR.GetBool() )
	{
		GLuint fbo = GetGLObject( renderDestManager.renderDestBaseHDR->GetAPIObject() );
		glBindFramebuffer( GL_DRAW_FRAMEBUFFER, fbo );
		backEnd.glState.currentFramebufferObject = fbo;
		backEnd.glState.currentRenderDestination = renderDestManager.renderDestBaseHDR;
	}
	else {
		glBindFramebuffer( GL_FRAMEBUFFER, GL_NONE );
		backEnd.glState.currentFramebufferObject = GL_NONE;
		backEnd.glState.currentRenderDestination = nullptr;
		///GL_SetNativeRenderDestination();
	}

	GL_Viewport(
		viewportOffset.x + view->GetViewport().x1,
		viewportOffset.y + view->GetViewport().y1,
		view->GetViewport().GetWidth(),
		view->GetViewport().GetHeight() );

	backEnd.currentScissor = ( light == nullptr )? view->GetViewport() : light->scissorRect;

	GL_Scissor(
		view->GetViewport().x1 + backEnd.currentScissor.x1,
		view->GetViewport().y1 + backEnd.currentScissor.y1,
		backEnd.currentScissor.GetWidth(),
		backEnd.currentScissor.GetHeight() );
}
#endif

/*
=========================================================================================================

	BUFFERS

=========================================================================================================
*/

void RB_InitBuffers()
{
	backEnd.viewUniformBuffer.AllocBufferObject( ( sizeof( idRenderMatrix ) * 6 ) + ( sizeof( idRenderVector ) * 4 ), BU_DEFAULT );
	backEnd.shadowUniformBuffer.AllocBufferObject( sizeof( idRenderMatrix ) * 6, BU_DYNAMIC );
	backEnd.progParmsUniformBuffer.AllocBufferObject( sizeof( idRenderVector ) * 32, BU_DYNAMIC );
}

void RB_ShutdownBuffers()
{
	backEnd.viewUniformBuffer.FreeBufferObject();
	backEnd.shadowUniformBuffer.FreeBufferObject();
	backEnd.progParmsUniformBuffer.FreeBufferObject();
}

void RB_UpdateBuffers( const idRenderView * const view )
{
	void * dest = backEnd.viewUniformBuffer.MapBuffer( BM_WRITE_NOSYNC );

	auto mats = ( idRenderMatrix * )dest;

	( mats++ )->Copy( view->GetViewMatrix() );
	( mats++ )->Copy( view->GetProjectionMatrix() );
	( mats++ )->Copy( view->GetMVPMatrix() );
	( mats++ )->Copy( view->GetInverseViewMatrix() );
	( mats++ )->Copy( view->GetInverseProjMatrix() );
	( mats++ )->Copy( view->GetInverseVPMatrix() );

	auto vecs = ( idRenderVector * )mats;

	// set eye position in global space
	*( vecs++ ) = idRenderVector( view->GetOrigin(), 1.0f );

	*( vecs++ ) = *( idRenderVector * )view->GetSplitFrustumDistances();

	const float x = view->GetViewport().x1;
	const float y = view->GetViewport().y1;
	const float w = view->GetViewport().GetWidth();
	const float h = view->GetViewport().GetHeight();

	// window coord to 0.0 to 1.0 conversion
	*( vecs++ ) = idRenderVector( 1.0f / w, 1.0f / h, w, h ); // g_WindowCoord

	 // sets overbright to make world brighter
	 // This value is baked into the specularScale and diffuseScale values so
	 // the interaction programs don't need to perform the extra multiply,
	 // but any other renderprogs that want to obey the brightness value
	 // can reference this.
	*( vecs++ ) = idRenderVector( r_lightScale.GetFloat() * 0.5f ); // g_Overbright

	// screen power of two correction factor (no longer relevant now)
	///*( vecs++ ) = idRenderVector( 1.0f, 1.0f, 0.0f, 1.0f ); // g_ScreenCorrectionFactor

	backEnd.viewUniformBuffer.UnmapBuffer();
	/*
	// color gamma correction
	float parm[ 3 ];
	for ( int i = 0; i < 3; i++ ) {
		if ( parm[ i ] <= 0.0f ) parm[ i ] = 0;
		else if ( parm[ i ] <= 0.04045f ) parm[ i ] /= 12.92f;
		else parm[ i ] = pow( ( parm[ i ] + 0.055f ) / 1.055f, 2.4f );
	}

	*/
}

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
	const drawSurf_t * const * drawSurfs = ( drawSurf_t** )&viewDef->drawSurfs[ 0 ];
	const int numDrawSurfs = viewDef->numDrawSurfs;
	const bool useHDR = r_useHDR.GetBool() && !viewDef->is2Dgui;

	RB_UpdateBuffers( viewDef );

	GL_BeginRenderView( viewDef );

	//------------------------------------
	// sets variables that can be used by all programs
	//------------------------------------
	{
		renderProgManager.GetRenderParm( RENDERPARM_VIEWCOLORMAP )->Set( renderDestManager.viewColorsImage );
		renderProgManager.GetRenderParm( RENDERPARM_VIEWNORMALMAP )->Set( renderDestManager.viewNormalImage );
		renderProgManager.GetRenderParm( RENDERPARM_VIEWDEPTHSTENCILMAP )->Set( renderDestManager.viewDepthStencilImage );

		backEnd.viewUniformBuffer.Bind( BINDING_GLOBAL_UBO );

		if( r_useProgUBO.GetBool() )
		{
			//backEnd.progParmsUniformBuffer.Bind( BINDING_PROG_PARMS_UBO, backEnd.progParmsUniformBuffer.GetOffset(), backEnd.progParmsUniformBuffer.GetSize() );
			backEnd.progParmsUniformBuffer.Bind( BINDING_PROG_PARMS_UBO );
		}
	}

	// If we are just doing 2D rendering, no need to fill the depth buffer.
	if( !viewDef->Is2DView() )
	{
		//-------------------------------------------------
		// fill the depth buffer and clear color buffer to black except on subviews
		//-------------------------------------------------
		RB_FillDepthBufferFast( drawSurfs, numDrawSurfs );

		if( r_useDeferredRender.GetBool() )
		{
			RB_FillGBuffer( viewDef, drawSurfs, numDrawSurfs );
			RB_DrawInteractionsDeferred( viewDef );
		}
		else {
			//-------------------------------------------------
			// FIXME, OPTIMIZE: merge this with FillDepthBufferFast like in a light prepass deferred renderer
			//
			// fill the geometric buffer with normals and roughness,
			// fill the depth buffer and the color buffer with precomputed Q3A style lighting
			//-------------------------------------------------
			if( !viewDef->isXraySubview )
			{
				RB_AmbientPass( viewDef, drawSurfs, numDrawSurfs );
			}
			//-------------------------------------------------
			// main light renderer
			//-------------------------------------------------
			RB_DrawInteractionsForward( viewDef );
		}

		//-------------------------------------------------
		// darken the scene using the screen space ambient occlusion
		//-------------------------------------------------
		if( r_useSSAO.GetBool() && !viewDef->isSubview )
		{
			// 3D views only.
			RB_SSAO();
		}

		///RB_SSGI( viewDef );
	}

	//-------------------------------------------------
	// now draw any non-light dependent shading passes
	//-------------------------------------------------
	int processed = 0;
	if( !r_skipTransMaterialPasses.GetBool() )
	{
		RENDERLOG_OPEN_MAINBLOCK( MRB_DRAW_SHADER_PASSES );

		// guiScreenOffset will be 0 in non-gui views
		const float guiScreenOffset = viewDef->Is2DView() ? ( stereoEye * viewDef->GetStereoScreenSeparation() ) : 0.0f;

		processed = RB_DrawTransMaterialPasses( drawSurfs, numDrawSurfs, guiScreenOffset, stereoEye );
		RENDERLOG_CLOSE_MAINBLOCK();
	}

	//-------------------------------------------------
	// use direct light and emissive light contributions to add indirect screen space light
	//-------------------------------------------------
	if( r_useSSGI.GetBool() && !viewDef->Is2DView() && !viewDef->is2Dgui && !viewDef->isSubview )
	{
		RB_SSGI();
	}

	//-------------------------------------------------
	// fog and blend lights, drawn after emissive surfaces
	// so they are properly dimmed down
	//-------------------------------------------------
	if( !viewDef->is2Dgui || !viewDef->Is2DView() )
	{
		if( backEnd.hasFogging )
		{
			RB_FogAllLights();
		}
	}

	//-------------------------------------------------
	// now draw any screen warping post-process effects using _currentRender
	//-------------------------------------------------
	if( processed < numDrawSurfs && !r_skipPostProcess.GetBool() )
	{
		const int x = viewDef->GetViewport().x1;
		const int y = viewDef->GetViewport().y1;
		const int w = viewDef->GetViewport().GetWidth();
		const int h = viewDef->GetViewport().GetHeight();

		RENDERLOG_PRINT( "Resolve current render to %i x %i buffer\n", w, h );

		//SEA: we can basicaly swap textures here with no copy?

		// resolve the screen
		//GL_CopyCurrentColorToTexture( renderImageManager->currentRenderImage, x, y, w, h );
		GL_BlitRenderBuffer(
			GL_GetCurrentRenderDestination(),
			x, y, w, h,
			renderDestManager.renderDestStagingColor,
			0, 0, w, h,
			true, false );

		backEnd.currentRenderCopied = true;

		// RENDERPARM_SCREENCORRECTIONFACTOR amd RENDERPARM_WINDOWCOORD overlap
		// diffuseScale and specularScale

		// screen power of two correction factor (no longer relevant now)
		auto screenCorrectionParm = renderProgManager.GetRenderParm( RENDERPARM_SCREENCORRECTIONFACTOR )->GetVecPtr();
		screenCorrectionParm[ 0 ] = 1.0f;
		screenCorrectionParm[ 1 ] = 1.0f;
		screenCorrectionParm[ 2 ] = 0.0f;
		screenCorrectionParm[ 3 ] = 1.0f;

	#if 0
		// window coord to 0.0 to 1.0 conversion
		float windowCoordParm[ 4 ];
		windowCoordParm[ 0 ] = 1.0f / w;
		windowCoordParm[ 1 ] = 1.0f / h;
		windowCoordParm[ 2 ] = 0.0f;
		windowCoordParm[ 3 ] = 1.0f;
		renderProgManager.SetRenderParm( RENDERPARM_WINDOWCOORD, windowCoordParm ); // rpWindowCoord / PP_INVERSE_RES

		// ---> sikk - Included non-power-of-two/frag position conversion
		// screen power of two correction factor, assuming the copy to _currentRender
		// also copied an extra row and column for the bilerp
		idRenderVector parm;
		parm[ 0 ] = ( float )w / renderImageManager->currentRenderImage->GetUploadWidth();
		parm[ 1 ] = ( float )h / renderImageManager->currentRenderImage->GetUploadHeight();
		parm[ 2 ] = parm[ 0 ] / w;
		parm[ 3 ] = parm[ 1 ] / h;
		renderProgManager.SetRenderParm( RENDERPARM_WINDOWCOORD, parm ); // rpCurrentRenderParms / PP_NPOT_ADJUST
		RENDERLOG_PRINT("-------rpCurrentRenderParms { %f, %f, %f, %f }\n", parm[ 0 ], parm[ 1 ], parm[ 2 ], parm[ 3 ] );
	#endif

		// render the remaining surfaces
		RENDERLOG_OPEN_MAINBLOCK( MRB_DRAW_SHADER_PASSES_POST );
		RB_DrawTransMaterialPasses( drawSurfs + processed, numDrawSurfs - processed, 0.0f /* definitely not a gui */, stereoEye );
		RENDERLOG_CLOSE_MAINBLOCK();
	}

	if( !viewDef->Is2DView() && !viewDef->isSubview )
	{
		if( ( r_motionBlur.GetInteger() > 0 ) && !viewDef->isSubview )
		{
			// 3D views only
			RB_MotionBlur();
		}

		if( r_antiAliasing.GetInteger() == ANTI_ALIASING_SMAA_1X )
		{
			RB_SMAA( viewDef );
			//RB_TXAA();
		}

		if( !viewDef->is2Dgui && !viewDef->isSubview )
		{
			if( r_useBloom.GetInteger() == 1 )
			{
				RB_Bloom();
			}
			else if( r_useBloom.GetInteger() == 2 )
			{
				RB_BloomNatural();
				//SEA: looks awesome! but wery bright specular problem need to be solved first :/
			}
			else if( r_useBloom.GetInteger() == 3 )
			{
				RB_BloomNatural2();
				//SEA: looks awesome! but wery bright specular problem need to be solved first :/
			}
		}

		// RB: convert back from HDR to LDR range
		if( useHDR /*&& !viewDef->isSubview*/ )
		{
			RB_CalculateAdaptation();
			RB_Tonemap();
		}
	}

	//-------------------------------------------------
	// render debug tools
	//-------------------------------------------------
	RB_RenderDebugTools( drawSurfs, numDrawSurfs );

	GL_EndRenderView();

	if( viewDef->is2Dgui )
	{
		int x = viewDef->GetViewport().x1;
		int y = viewDef->GetViewport().y1;
		int w = viewDef->GetViewport().GetWidth();
		int h = viewDef->GetViewport().GetHeight();

		RENDERLOG_PRINT( "Resolve current 2DRender to native %i x %i buffer\n", w, h );
		GL_BlitRenderBuffer(
			GL_GetCurrentRenderDestination(),
			x, y, w, h,
			NULL,
			0, 0, w, h,
			true, false );
	}

	RENDERLOG_CLOSE_BLOCK();
}

/*
=========================================================================================================

	BACKEND COMMANDS

=========================================================================================================
*/

/*
==================
 RB_CMD_DrawView

	StereoEye will always be 0 in mono modes, or -1 / 1 in stereo modes.
	If the view is a GUI view that is repeated for both eyes, the viewDef.stereoEye value
	is 0, so the stereoEye parameter is not always the same as that.
==================
*/
void RB_CMD_DrawView( const void* data, const int stereoEye )
{
	const drawSurfsCommand_t* cmd = ( const drawSurfsCommand_t* )data;

	// if there aren't any drawsurfs, do nothing
	if( !cmd->viewDef->numDrawSurfs )
	{
		return;
	}
	// skip render bypasses everything that has models, assuming
	// them to be 3D views, but leaves 2D rendering visible
	if( r_skipRender.GetBool() && cmd->viewDef->viewEntitys )
	{
		return;
	}

	if( cmd->viewDef->viewEntitys )
	{
		RENDERLOG_OPEN_BLOCK( cmd->viewDef->isSubview ? "RB_CMD_DrawSubview3D" : "RB_CMD_DrawView3D" );
	}
	else {
		RENDERLOG_OPEN_BLOCK( cmd->viewDef->isSubview ? "RB_CMD_DrawSubview2D" : "RB_CMD_DrawView2D" );
	}

	backEnd.viewDef = cmd->viewDef;
	// we will need to do a new copyTexSubImage of the screen
	// when a SS_POST_PROCESS material is used
	backEnd.currentRenderCopied = false;
	backEnd.pc.c_surfaces += backEnd.viewDef->numDrawSurfs;

	RB_ShowOverdraw();

	//-------------------------------------------------
	// guis can wind up referencing purged images that need to be loaded.
	// this used to be in the gui emit code, but now that it can be running
	// in a separate thread, it must not try to load images, so do it here.
	//-------------------------------------------------
	for( int i = 0; i < cmd->viewDef->numDrawSurfs; ++i )
	{
		const drawSurf_t* ds = cmd->viewDef->drawSurfs[ i ];
		if( ds->material != NULL )
		{
			const_cast<idMaterial*>( ds->material )->EnsureNotPurged();
		}
	}

	// render the scene
	RB_DrawViewInternal( cmd->viewDef, stereoEye );

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

	RENDERLOG_CLOSE_BLOCK();
}

/*
==================
 RB_CMD_CopyRender

	Copy part of the current framebuffer to an image
==================
*/
void RB_CMD_CopyRender( const void* data )
{
	const copyRenderCommand_t* cmd = ( const copyRenderCommand_t* )data;

	if( r_skipCopyTexture.GetBool() )
	{
		return;
	}

	RENDERLOG_OPEN_BLOCK( "RB_CMD_CopyRender" );

	if( cmd->image )
	{
		GL_CopyCurrentColorToTexture( cmd->image, cmd->x, cmd->y, cmd->imageWidth, cmd->imageHeight );
	}

	if( cmd->clearColorAfterCopy )
	{
		GL_Clear( true, false, false, STENCIL_SHADOW_TEST_VALUE, 0, 0, 0, 0 );
	}

	RENDERLOG_CLOSE_BLOCK();
}

/*
==================
 RB_CMD_PostProcess
==================
*/
extern idCVar rs_enable;
void RB_CMD_PostProcess( const void* data )
{
	// only do the post process step if resolution scaling is enabled. Prevents the unnecessary copying of the framebuffer and
	// corresponding full screen quad pass.
	if( //rs_enable.GetInteger() == 0 &&
		!r_useFilmicPostProcessEffects.GetBool() && r_antiAliasing.GetInteger() == 0 )
	{
		return;
	}

	if( ( r_ssaoDebug.GetInteger() > 0 ) || ( r_ssgiDebug.GetInteger() > 0 ) )
	{
		return;
	}

	RENDERLOG_OPEN_BLOCK( "RB_CMD_PostProcess" );

	postProcessCommand_t* cmd = ( postProcessCommand_t* )data;
	auto & viewport = cmd->viewDef->GetViewport();

	// SMAA
	/*if( r_antiAliasing.GetInteger() == ANTI_ALIASING_SMAA_1X )
	{
		RB_SMAA( cmd->viewDef );
		//RB_TXAA();
	}*/

	if( r_useFilmicPostProcessEffects.GetBool() )
	{
		// resolve the scaled rendering to a temporary texture
		//GL_CopyCurrentColorToTexture( renderImageManager->currentRenderImage, viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );
		GL_BlitRenderBuffer(
			GL_GetCurrentRenderDestination(),
			viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight(),
			renderDestManager.renderDestStagingColor,
			0, 0, viewport.GetWidth(), viewport.GetHeight(),
			true, false );

		int screenWidth = renderSystem->GetWidth();
		int screenHeight = renderSystem->GetHeight();

		GL_SetRenderDestination( renderDestManager.renderDestBaseLDR );
		GL_ViewportAndScissor( 0, 0, screenWidth, screenHeight );
		GL_State( GLS_DISABLE_BLENDING | GLS_DISABLE_DEPTHTEST | GLS_TWOSIDED );

		GL_SetRenderProgram( renderProgManager.prog_postprocess );

		renderProgManager.SetRenderParm( RENDERPARM_CURRENTRENDERMAP, renderImageManager->currentRenderImage );
		renderProgManager.SetRenderParm( RENDERPARM_GRAINMAP, renderImageManager->grainImage1 );

		const static int GRAIN_SIZE = 128;

		// screen power of two correction factor
		// rpScreenCorrectionFactor
		auto screenCorrectionParm = renderProgManager.GetRenderParm( RENDERPARM_SCREENCORRECTIONFACTOR )->GetVecPtr();
		screenCorrectionParm[ 0 ] = 1.0f / GRAIN_SIZE;
		screenCorrectionParm[ 1 ] = 1.0f / GRAIN_SIZE;
		screenCorrectionParm[ 2 ] = 1.0f;
		screenCorrectionParm[ 3 ] = 1.0f;

		// rpJitterTexOffset
		auto jitterTexOffset = renderProgManager.GetRenderParm( RENDERPARM_JITTERTEXOFFSET )->GetVecPtr();
		if( r_shadowMapRandomizeJitter.GetBool() )
		{
			jitterTexOffset[ 0 ] = ( rand() & 255 ) / 255.0f;
			jitterTexOffset[ 1 ] = ( rand() & 255 ) / 255.0f;
		}
		else {
			jitterTexOffset[ 0 ] = 0.0f;
			jitterTexOffset[ 1 ] = 0.0f;
		}
		jitterTexOffset[ 2 ] = 0.0f;
		jitterTexOffset[ 3 ] = 0.0f;

		///GL_DrawUnitSquare();
		GL_DrawScreenTriangleAuto();
	}

	GL_ResetProgramState();
	GL_ResetTextureState();

	RENDERLOG_CLOSE_BLOCK();
}










/*
void RB_SurfaceSkip( void *surf )
{}
const int SF_NUM_SURFACE_TYPES = 1;
extern void( *rb_surfaceTable[ SF_NUM_SURFACE_TYPES ] ) ( void * );
void( *rb_surfaceTable[ SF_NUM_SURFACE_TYPES ] ) ( void * ) = {
	( void( *) ( void* ) )RB_SurfaceBad,          // SF_BAD,
	( void( *) ( void* ) )RB_SurfaceSkip,         // SF_SKIP,
	( void( *) ( void* ) )RB_SurfaceFace,         // SF_FACE,
	( void( *) ( void* ) )RB_SurfaceGrid,         // SF_GRID,
	( void( *) ( void* ) )RB_SurfaceTriangles,    // SF_TRIANGLES,
	( void( *) ( void* ) )RB_SurfacePolychain,    // SF_POLY,
	( void( *) ( void* ) )RB_SurfaceMesh,         // SF_MD3,
	( void( *) ( void* ) )RB_SurfaceCMesh,        // SF_MDC,
	( void( *) ( void* ) )RB_SurfaceAnim,         // SF_MDS,
	( void( *) ( void* ) )RB_SurfaceFlare,        // SF_FLARE,
	( void( *) ( void* ) )RB_SurfaceEntity,       // SF_ENTITY
	( void( *) ( void* ) )RB_SurfaceDisplayList   // SF_DISPLAY_LIST
};
*/
