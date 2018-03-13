
#include "precompiled.h"
#pragma hdrstop

#include "Render.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

idCVar r_drawEyeColor( "r_drawEyeColor", "0", CVAR_RENDERER | CVAR_BOOL, "Draw a colored box, red = left eye, blue = right eye, grey = non-stereo" );
idCVar r_motionBlur( "r_motionBlur", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "1 - 5, log2 of the number of motion blur samples" );
idCVar r_skipTransMaterialPasses( "r_skipTransMaterialPasses", "0", CVAR_RENDERER | CVAR_BOOL, "" );

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
idCVar r_forceAmbient( "r_forceAmbient", "0.08", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "render additional ambient pass to make the game less dark", 0.0f, 0.5f );

idCVar r_useSSGI( "r_useSSGI", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "use screen space global illumination and reflections" );
idCVar r_ssgiDebug( "r_ssgiDebug", "0", CVAR_RENDERER | CVAR_INTEGER, "" );
idCVar r_ssgiFiltering( "r_ssgiFiltering", "1", CVAR_RENDERER | CVAR_BOOL, "" );

idCVar r_useSSAO( "r_useSSAO", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "use screen space ambient occlusion to darken corners" );
idCVar r_ssaoDebug( "r_ssaoDebug", "0", CVAR_RENDERER | CVAR_INTEGER, "" );
idCVar r_ssaoFiltering( "r_ssaoFiltering", "1", CVAR_RENDERER | CVAR_BOOL, "" );
idCVar r_useHierarchicalDepthBuffer( "r_useHierarchicalDepthBuffer", "1", CVAR_RENDERER | CVAR_BOOL, "" );

idCVar r_exposure( "r_exposure", "0.5", CVAR_ARCHIVE | CVAR_RENDERER | CVAR_FLOAT, "HDR exposure or LDR brightness [0.0 .. 1.0]", 0.0f, 1.0f );
// RB end

backEndState_t	backEnd;

/////////////////////////////////////////////////////////////////////////////////////////////////////

/*
================
SetFragmentParm
================
*/
void RB_SetMVP( const idRenderMatrix& mvp )
{
	//renderProgManager.SetRenderParms( RENDERPARM_MVPMATRIX_X, mvp.Ptr(), 4 );
	idRenderMatrix::CopyMatrix( mvp,
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_X ),
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_Y ),
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_Z ),
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_W ) );
}

/*
================
 RB_SetMVPWithStereoOffset
================
*/
void RB_SetMVPWithStereoOffset( const idRenderMatrix& mvp, const float stereoOffset )
{
	idRenderMatrix offset = mvp;
	offset[ 0 ][ 3 ] += stereoOffset;
	renderProgManager.SetRenderParms( RENDERPARM_MVPMATRIX_X, offset[ 0 ], 4 );
}

void RB_SetSurfaceSpaceMatrices( const drawSurf_t *const drawSurf )
{
#if 1
	idRenderMatrix::CopyMatrix( drawSurf->space->mvp,
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_X ),
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_Y ),
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_Z ),
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_W ) );
	
	idRenderMatrix::CopyMatrix( drawSurf->space->modelMatrix,
		renderProgManager.GetRenderParm( RENDERPARM_MODELMATRIX_X ),
		renderProgManager.GetRenderParm( RENDERPARM_MODELMATRIX_Y ),
		renderProgManager.GetRenderParm( RENDERPARM_MODELMATRIX_Z ),
		renderProgManager.GetRenderParm( RENDERPARM_MODELMATRIX_W ) );

	idRenderMatrix::CopyMatrix( drawSurf->space->modelViewMatrix,
		renderProgManager.GetRenderParm( RENDERPARM_MODELVIEWMATRIX_X ),
		renderProgManager.GetRenderParm( RENDERPARM_MODELVIEWMATRIX_Y ),
		renderProgManager.GetRenderParm( RENDERPARM_MODELVIEWMATRIX_Z ),
		renderProgManager.GetRenderParm( RENDERPARM_MODELVIEWMATRIX_W ) );
#else
	renderProgManager.SetRenderParms( RENDERPARM_MVPMATRIX_X, drawSurf->space->mvp.Ptr(), 4 );
	renderProgManager.SetRenderParms( RENDERPARM_MODELMATRIX_X, drawSurf->space->modelMatrix.Ptr(), 4 );
	renderProgManager.SetRenderParms( RENDERPARM_MODELVIEWMATRIX_X, drawSurf->space->modelViewMatrix.Ptr(), 4 );
#endif
}

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
			renderProgManager.SetRenderParm( RENDERPARM_VERTEXCOLOR_MODULATE, zero );
			renderProgManager.SetRenderParm( RENDERPARM_VERTEXCOLOR_ADD, one );
			break;
		case SVC_MODULATE:
			renderProgManager.SetRenderParm( RENDERPARM_VERTEXCOLOR_MODULATE, one );
			renderProgManager.SetRenderParm( RENDERPARM_VERTEXCOLOR_ADD, zero );
			break;
		case SVC_INVERSE_MODULATE:
			renderProgManager.SetRenderParm( RENDERPARM_VERTEXCOLOR_MODULATE, negOne );
			renderProgManager.SetRenderParm( RENDERPARM_VERTEXCOLOR_ADD, one );
			break;
	}
}
void RB_SetVertexColorParm( stageVertexColor_t svc )
{
	switch( svc )
	{
		case SVC_IGNORE:
			renderProgManager.GetRenderParm( RENDERPARM_VERTEXCOLOR_MAD ).Set( 0, 1, 0, 0 );
			break;
		case SVC_MODULATE:
			renderProgManager.GetRenderParm( RENDERPARM_VERTEXCOLOR_MAD ).Set( 1, 0, 0, 0 );
			break;
		case SVC_INVERSE_MODULATE:
			renderProgManager.GetRenderParm( RENDERPARM_VERTEXCOLOR_MAD ).Set( -1, 1, 0, 0 );
			break;
	}
}

/*
======================
 RB_GetShaderTextureMatrix
======================
*/
void RB_GetShaderTextureMatrix( const float* shaderRegisters, const textureStage_t* texture, float matrix[ 16 ] )
{
	matrix[ 0 * 4 + 0 ] = shaderRegisters[ texture->matrix[ 0 ][ 0 ] ];
	matrix[ 1 * 4 + 0 ] = shaderRegisters[ texture->matrix[ 0 ][ 1 ] ];
	matrix[ 2 * 4 + 0 ] = 0.0f;
	matrix[ 3 * 4 + 0 ] = shaderRegisters[ texture->matrix[ 0 ][ 2 ] ];

	matrix[ 0 * 4 + 1 ] = shaderRegisters[ texture->matrix[ 1 ][ 0 ] ];
	matrix[ 1 * 4 + 1 ] = shaderRegisters[ texture->matrix[ 1 ][ 1 ] ];
	matrix[ 2 * 4 + 1 ] = 0.0f;
	matrix[ 3 * 4 + 1 ] = shaderRegisters[ texture->matrix[ 1 ][ 2 ] ];

	// we attempt to keep scrolls from generating incredibly large texture values, but
	// center rotations and center scales can still generate offsets that need to be > 1
	if( matrix[ 3 * 4 + 0 ] < -40.0f || matrix[ 12 ] > 40.0f )
	{
		matrix[ 3 * 4 + 0 ] -= ( int )matrix[ 3 * 4 + 0 ];
	}
	if( matrix[ 13 ] < -40.0f || matrix[ 13 ] > 40.0f )
	{
		matrix[ 13 ] -= ( int )matrix[ 13 ];
	}

	matrix[ 0 * 4 + 2 ] = 0.0f;
	matrix[ 1 * 4 + 2 ] = 0.0f;
	matrix[ 2 * 4 + 2 ] = 1.0f;
	matrix[ 3 * 4 + 2 ] = 0.0f;

	matrix[ 0 * 4 + 3 ] = 0.0f;
	matrix[ 1 * 4 + 3 ] = 0.0f;
	matrix[ 2 * 4 + 3 ] = 0.0f;
	matrix[ 3 * 4 + 3 ] = 1.0f;
}

/*
======================
 RB_LoadShaderTextureMatrix
======================
*/
void RB_LoadShaderTextureMatrix( const float* shaderRegisters, const textureStage_t* texture )
{
	auto & texS = renderProgManager.GetRenderParm( RENDERPARM_TEXTUREMATRIX_S );
	auto & texT = renderProgManager.GetRenderParm( RENDERPARM_TEXTUREMATRIX_T );
	
	texS.Set( 1.0f, 0.0f, 0.0f, 0.0f );
	texT.Set( 0.0f, 1.0f, 0.0f, 0.0f );

	if( texture->hasMatrix )
	{
		ALIGNTYPE16 float matrix[ 16 ];
		RB_GetShaderTextureMatrix( shaderRegisters, texture, matrix );

		texS[ 0 ] = matrix[ 0 * 4 + 0 ];
		texS[ 1 ] = matrix[ 1 * 4 + 0 ];
		texS[ 2 ] = matrix[ 2 * 4 + 0 ];
		texS[ 3 ] = matrix[ 3 * 4 + 0 ];

		texT[ 0 ] = matrix[ 0 * 4 + 1 ];
		texT[ 1 ] = matrix[ 1 * 4 + 1 ];
		texT[ 2 ] = matrix[ 2 * 4 + 1 ];
		texT[ 3 ] = matrix[ 3 * 4 + 1 ];

		RENDERLOG_PRINT( "Setting Texture Matrix\n" );
		RENDERLOG_INDENT();
		RENDERLOG_PRINT( "Texture Matrix S : %4.3f, %4.3f, %4.3f, %4.3f\n", texS[ 0 ], texS[ 1 ], texS[ 2 ], texS[ 3 ] );
		RENDERLOG_PRINT( "Texture Matrix T : %4.3f, %4.3f, %4.3f, %4.3f\n", texT[ 0 ], texT[ 1 ], texT[ 2 ], texT[ 3 ] );
		RENDERLOG_OUTDENT();
	}
}

/*
=====================
 RB_BakeTextureMatrixIntoTexgen
=====================
*/
void RB_BakeTextureMatrixIntoTexgen( idPlane lightProject[ 3 ], const float* textureMatrix )
{
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
}

/*
=====================
 RB_ResetViewportAndScissorToDefaultCamera

	RB: moved this up because we need to call this several times for shadow mapping
=====================
*/
void RB_ResetViewportAndScissorToDefaultCamera( const idRenderView * const viewDef )
{
	// set the window clipping
	GL_Viewport( 
		viewDef->GetViewport().x1, 
		viewDef->GetViewport().y1,
		viewDef->GetViewport().GetWidth(),
		viewDef->GetViewport().GetHeight() );

	// the scissor may be smaller than the viewport for subviews
	GL_Scissor( 
		viewDef->GetViewport().x1 + viewDef->GetScissor().x1,
		viewDef->GetViewport().y1 + viewDef->GetScissor().y1,
		viewDef->GetScissor().GetWidth(),
		viewDef->GetScissor().GetHeight() );

	backEnd.currentScissor = viewDef->GetScissor();
}

/*
=====================
 RB_SetScissor
=====================
*/
void RB_SetScissor( const idScreenRect & scissorRect )
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
==================
 RB_EXP_SetRenderBuffer

	This may be to a float buffer, and scissor is set to cover only the light
==================
*/
void RB_SetBaseRenderDestination( const idRenderView * const view, const viewLight_t * const light )
{
	const idVec2i viewportOffset( 0, 0 );

	if( r_useHDR.GetBool() )
	{
		GLuint fbo = GetGLObject( renderDestManager.renderDestBaseHDR->GetAPIObject() );
		glBindFramebuffer( GL_FRAMEBUFFER, fbo );
		backEnd.glState.currentFramebufferObject = fbo;
	}
	else {
		glBindFramebuffer( GL_FRAMEBUFFER, GL_NONE );
		backEnd.glState.currentFramebufferObject = GL_NONE;
	}

	GL_Viewport(
		viewportOffset.x + view->GetViewport().x1,
		viewportOffset.y + view->GetViewport().y1,
		view->GetViewport().GetWidth(),
		view->GetViewport().GetHeight() );

	backEnd.currentScissor = ( light == nullptr )? view->GetViewport() : light->scissorRect;

	if( r_useScissor.GetBool() )
	{
		GL_Scissor(
			view->GetViewport().x1 + backEnd.currentScissor.x1,
			view->GetViewport().y1 + backEnd.currentScissor.y1,
			backEnd.currentScissor.GetWidth(),
			backEnd.currentScissor.GetHeight() );
	}
}

/*
==================
 RB_ResetRenderDest
==================
*/
void RB_ResetRenderDest( const bool hdrIsActive )
{
	//if( backEnd.viewDef->isSubview && backEnd.viewDef->subviewRenderTexture )
	//{
	//	GL_SetRenderDestination( renderDestManager.renderDestRenderToTexture );
	//	renderDestManager.renderDestRenderToTexture->SetTargetOGL( backEnd.viewDef->subviewRenderTexture );
	//	return;
	//}

	if( hdrIsActive )
	{
		GL_SetRenderDestination( renderDestManager.renderDestBaseHDR );
	}
	else {
		GL_SetNativeRenderDestination();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

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
static void RB_AmbientPass( const drawSurf_t* const* drawSurfs, int numDrawSurfs )
{
	if( r_forceAmbient.GetFloat() <= 0.0f || r_skipAmbient.GetBool() )
	{
		return;
	}

	if( !numDrawSurfs )
	{
		return;
	}

	// if we are just doing 2D rendering, no need to fill the gbuffer
	if( !backEnd.viewDef->viewEntitys )
	{
		return;
	}

	const bool hdrIsActive( r_useHDR.GetBool() && !!renderDestManager.renderDestBaseHDR && GL_IsBound( renderDestManager.renderDestBaseHDR ) );

	RENDERLOG_OPEN_MAINBLOCK( MRB_AMBIENT_PASS );
	RENDERLOG_OPEN_BLOCK( "RB_AmbientPass" );

	// force MVP change on first surface
	backEnd.currentSpace = NULL;

	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );

	// fill geometry buffer with normal/roughness information and ambient color
	GL_SetRenderDestination( renderDestManager.renderDestGBufferSml );
	GL_ClearColor( 0.0f, 0.0f, 0.0f, 0.0f, 0 );
	GL_ClearColor( 0.0f, 0.0f, 0.0f, 0.0f, 1 );

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
	ambientColor.w = 1.0;

	renderProgManager.SetRenderParm( RENDERPARM_AMBIENT_COLOR, ambientColor.ToFloatPtr() );

	// setup renderparms assuming we will be drawing trivial surfaces first
	RB_SetupForFastPathInteractions( diffuseColor, specularColor );

	for( int i = 0; i < numDrawSurfs; ++i )
	{
		auto const drawSurf = drawSurfs[ i ];
		auto const surfaceMaterial = drawSurf->material;

		// translucent surfaces don't put anything in the gbuffer.
		if( surfaceMaterial->Coverage() == MC_TRANSLUCENT )
		{
			continue;
		}

		// get the expressions for conditionals / color / texcoords
		const float* surfaceRegs = drawSurf->shaderRegisters;

		// if all stages of a material have been conditioned off, don't do anything
		if( surfaceMaterial->ConditionedOff( surfaceRegs ) )
		{
			continue;
		}

		///bool isWorldModel = ( drawSurf->space->entityDef->parms.origin == vec3_origin );

		renderProgManager.BindShader_GBufferSml( drawSurf->jointCache );

		// change the matrix if needed
		if( drawSurf->space != backEnd.currentSpace )
		{
			backEnd.currentSpace = drawSurf->space;
			RB_SetSurfaceSpaceMatrices( drawSurf );

			// tranform the view origin into model local space
			idRenderVector localViewOrigin( 1.0f );
			drawSurf->space->modelMatrix.InverseTransformPoint( backEnd.viewDef->GetOrigin(), localViewOrigin.ToVec3() );
			renderProgManager.SetRenderParm( RENDERPARM_LOCALVIEWORIGIN, localViewOrigin );
		}

		// check for the fast path
		if( surfaceMaterial->GetFastPathBumpImage() && !r_skipInteractionFastPath.GetBool() )
		{
			RENDERLOG_OPEN_BLOCK( surfaceMaterial->GetName() );

			///GL_BindTexture( INTERACTION_TEXUNIT_BUMP, surfaceMaterial->GetFastPathBumpImage() );
			///GL_BindTexture( INTERACTION_TEXUNIT_DIFFUSE, surfaceMaterial->GetFastPathDiffuseImage() );
			///GL_BindTexture( INTERACTION_TEXUNIT_SPECULAR, surfaceMaterial->GetFastPathSpecularImage() );
			renderProgManager.SetRenderParm( RENDERPARM_BUMPMAP, surfaceMaterial->GetFastPathBumpImage() );
			renderProgManager.SetRenderParm( RENDERPARM_DIFFUSEMAP, surfaceMaterial->GetFastPathDiffuseImage() );
			renderProgManager.SetRenderParm( RENDERPARM_SPECULARMAP, surfaceMaterial->GetFastPathSpecularImage() );

			GL_DrawIndexed( drawSurf );

			RENDERLOG_CLOSE_BLOCK();
			continue;
		}

		drawInteraction_t inter = {};
		inter.surf = drawSurf;

		inter.bumpImage = inter.specularImage = inter.diffuseImage = NULL;
		inter.diffuseColor.Fill( 1.0 );
		inter.specularColor.Fill( 0.0 );

		RB_DrawComplexMaterialInteraction( inter, surfaceRegs, surfaceMaterial, diffuseColor, specularColor );
	}

	renderProgManager.SetRenderParm( RENDERPARM_ALPHA_TEST, vec4_zero );

	RENDERLOG_CLOSE_BLOCK();
	RENDERLOG_CLOSE_MAINBLOCK();

	/*if( fillGbuffer )
	{
		GL_SelectTexture( 0 );

		// FIXME: this copies RGBA16F into _currentNormals if HDR is enabled
		const idScreenRect& viewport = backEnd.viewDef->GetViewport();
		GL_CopyCurrentColorToTexture( renderImageManager->currentNormalsImage, viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );
	}*/

	RB_ResetRenderDest( hdrIsActive );

	GL_ResetTexturesState();
	GL_ResetProgramState();
}
// RB end



static void RB_DrawInteractionsWithScreenSpaceShadowMap( const idRenderView * const renderView )
{
	if( r_skipInteractions.GetBool() )
		return;

	RENDERLOG_OPEN_MAINBLOCK( MRB_DRAW_INTERACTIONS );
	RENDERLOG_OPEN_BLOCK( "RB_DrawInteractionsWithScreenSpaceShadows" );

	GL_SelectTexture( 0 );

	const bool useLightDepthBounds = r_useLightDepthBounds.GetBool() && !r_useShadowMapping.GetBool();

	//
	// For each light, perform shadowing to screen sized ( or less ) texture, blur it, upscale if needed.
	//
	for( const viewLight_t* vLight = renderView->viewLights; vLight != NULL; vLight = vLight->next )
	{
		// do fogging later
		if( vLight->lightShader->IsFogLight() || vLight->lightShader->IsBlendLight() )
			continue;

		if( vLight->NoInteractions() )
			continue;

		RENDERLOG_OPEN_BLOCK( vLight->lightShader->GetName() );

		// set the depth bounds for the whole light
		if( useLightDepthBounds )
		{
			GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
		}

		// RB_FillShadowDepthBuffer( vLight->globalShadows, vLight );

		// render the shadows into destination alpha on the included pixels
		// RB_FillScreenSpaceShadowMap( vLight );

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
=========================================================================================================
	HDR PROC

	The best approach is to evaluate if what you are doing can be done without needing to read back at all. 
	For example, you can calculate the average luminance of a scene for tonemapping by generating mipmaps 
	for your texture; the 1x1 miplevel will contain the average and the end result is an operation 
	that happens entirely on the GPU.
=========================================================================================================
*/
static void RB_CalculateAdaptation()
{
	int				i;
	static ALIGNTYPE16 idArray<float, 64 * 64 * 4> image;
	float           curTime;
	float			deltaTime;
	float           luminance;
	float			avgLuminance;
	float			maxLuminance;
	double			sum;
	const idVec3    LUMINANCE_SRGB( 0.2125f, 0.7154f, 0.0721f ); // be careful wether this should be linear RGB or sRGB	
	float			newAdaptation;
	float			newMaximum;

	RENDERLOG_OPEN_BLOCK("RB_CalculateAdaptation");

	if( !r_hdrAutoExposure.GetBool() )
	{
		// no dynamic exposure

		backEnd.hdrKey = r_hdrKey.GetFloat();
		backEnd.hdrAverageLuminance = r_hdrMinLuminance.GetFloat();
		backEnd.hdrMaxLuminance = 1;
	}
	else {
		idRenderVector color;

		curTime = sys->Milliseconds() / 1000.0f;

		// calculate the average scene luminance
		GL_SetRenderDestination( renderDestManager.renderDestBaseHDRsml64 );

		// read back the contents
		//	glFinish();

		glReadPixels( 0, 0, 64, 64, GL_BGRA, GL_FLOAT, image.Ptr() );
		RENDERLOG_PRINT( "glReadPixels( 0, 0, 64, 64, GL_BGRA, GL_FLOAT, image.Ptr())\n" );

		sum = 0.0f;
		maxLuminance = 0.0f;
		for( i = 0; i < ( 64 * 64 ); i += 4 )
		{
			color[ 0 ] = image[ i * 4 + 0 ];
			color[ 1 ] = image[ i * 4 + 1 ];
			color[ 2 ] = image[ i * 4 + 2 ];
			color[ 3 ] = image[ i * 4 + 3 ];

			///color[ 0 ] = pboMappedPtr[ i * 4 + 0 ];
			///color[ 1 ] = pboMappedPtr[ i * 4 + 1 ];
			///color[ 2 ] = pboMappedPtr[ i * 4 + 2 ];
			///color[ 3 ] = pboMappedPtr[ i * 4 + 3 ];

			///luminance = ( color.x * LUMINANCE_SRGB.x + color.y * LUMINANCE_SRGB.y + color.z * LUMINANCE_SRGB.z ) + 0.0001f;
			luminance = ( color.x * LUMINANCE_SRGB.y + color.y * LUMINANCE_SRGB.x + color.z * LUMINANCE_SRGB.z ) + 0.0001f; // BGRA
			if( luminance > maxLuminance )
			{
				maxLuminance = luminance;
			}

			float logLuminance = idMath::Log16( luminance + 1 );
			//if( logLuminance > 0 )
			{
				sum += luminance;
			}

		#if 0 //SEA: todo!
			__m128 luminance_srgb_xxxx = _mm_set1_ps( LUMINANCE_SRGB.x );
			__m128 luminance_srgb_yyyy = _mm_set1_ps( LUMINANCE_SRGB.y );
			__m128 luminance_srgb_zzzz = _mm_set1_ps( LUMINANCE_SRGB.z );
			__m128 tail = _mm_set1_ps( 0.0001f );
			//
			//__m128 rgba1 = _mm_load_ps( &image[ i * 4 + ( 0 * 4 ) ] );
			//__m128 rgba2 = _mm_load_ps( &image[ i * 4 + ( 1 * 4 ) ] );
			//__m128 rgba3 = _mm_load_ps( &image[ i * 4 + ( 2 * 4 ) ] );
			//__m128 rgba4 = _mm_load_ps( &image[ i * 4 + ( 3 * 4 ) ] );

			__m128 color_xxxx, color_yyyy, color_zzzz;
			__m128 luminance0123;

			luminance0123 = _mm_add_ps( _mm_add_ps( _mm_mul_ps( color_xxxx, luminance_srgb_xxxx ), _mm_mul_ps( color_zzzz, luminance_srgb_zzzz ) ), );


			luminance = ( color.x * LUMINANCE_SRGB.x + color.y * LUMINANCE_SRGB.y + color.z * LUMINANCE_SRGB.z ) + 0.0001f;
			luminance = ( color.x * LUMINANCE_SRGB.x + color.y * LUMINANCE_SRGB.y + color.z * LUMINANCE_SRGB.z ) + 0.0001f;
			luminance = ( color.x * LUMINANCE_SRGB.x + color.y * LUMINANCE_SRGB.y + color.z * LUMINANCE_SRGB.z ) + 0.0001f;
			luminance = ( color.x * LUMINANCE_SRGB.x + color.y * LUMINANCE_SRGB.y + color.z * LUMINANCE_SRGB.z ) + 0.0001f;
		#endif
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

		newAdaptation = backEnd.hdrAverageLuminance + ( avgLuminance - backEnd.hdrAverageLuminance ) * ( 1.0f - idMath::Pow16( 0.98f, 30.0f * deltaTime ) );
		newMaximum = backEnd.hdrMaxLuminance + ( maxLuminance - backEnd.hdrMaxLuminance ) * ( 1.0f - idMath::Pow16( 0.98f, 30.0f * deltaTime ) );

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

	RENDERLOG_PRINT( "HDR luminance avg = %f, max = %f, key = %f\n", backEnd.hdrAverageLuminance, backEnd.hdrMaxLuminance, backEnd.hdrKey );
	RENDERLOG_CLOSE_BLOCK();
}

static void RB_Tonemap( const idRenderView* viewDef )
{
	RENDERLOG_OPEN_BLOCK("RB_Tonemap");
	RENDERLOG_PRINT( "( avg = %f, max = %f, key = %f, is2Dgui = %i )\n", backEnd.hdrAverageLuminance, backEnd.hdrMaxLuminance, backEnd.hdrKey, ( int )viewDef->is2Dgui );

	//postProcessCommand_t* cmd = ( postProcessCommand_t* )data;
	//const idScreenRect& viewport = cmd->viewDef->viewport;
	//renderImageManager->currentRenderImage->CopyFramebuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );

	GL_SetNativeRenderDestination();

	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
	GL_Cull( CT_TWO_SIDED );

	int screenWidth = renderSystem->GetWidth();
	int screenHeight = renderSystem->GetHeight();

	// set the window clipping
	GL_ViewportAndScissor( 0, 0, screenWidth, screenHeight );

#if defined( USE_HDR_MSAA )
	if( glConfig.multisamples > 0 )
	{
		GL_BindTexture( 0, renderImageManager->currentRenderHDRImageNoMSAA );
	} else
	#endif
	{
		///GL_BindTexture( 0, renderImageManager->currentRenderHDRImage );
		renderProgManager.SetRenderParm( RENDERPARM_CURRENTRENDERMAP, renderImageManager->currentRenderHDRImage );
	}

	///GL_BindTexture( 1, renderImageManager->heatmap7Image );
	renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderImageManager->heatmap7Image );

	if( r_hdrDebug.GetBool() )
	{
		renderProgManager.BindShader_HDRDebug();
	}
	else {
		renderProgManager.BindShader_Tonemap();
	}

	auto & screenCorrectionParm = renderProgManager.GetRenderParm( RENDERPARM_SCREENCORRECTIONFACTOR ); 
	// rpScreenCorrectionFactor

	if( viewDef->is2Dgui )
	{
		screenCorrectionParm[ 0 ] = 2.0f;
		screenCorrectionParm[ 1 ] = 1.0f;
		screenCorrectionParm[ 2 ] = 1.0f;
	}
	else {
		if( r_hdrAutoExposure.GetBool() )
		{
			float exposureOffset = Lerp( -0.01f, 0.02f, idMath::ClampFloat( 0.0, 1.0, r_exposure.GetFloat() ) );

			screenCorrectionParm[ 0 ] = backEnd.hdrKey + exposureOffset;
			screenCorrectionParm[ 1 ] = backEnd.hdrAverageLuminance;
			screenCorrectionParm[ 2 ] = backEnd.hdrMaxLuminance;
			screenCorrectionParm[ 3 ] = exposureOffset;
			//screenCorrectionParm[3] = Lerp( -1, 5, idMath::ClampFloat( 0.0, 1.0, r_exposure.GetFloat() ) );
		}
		else {
			//float exposureOffset = ( idMath::ClampFloat( 0.0, 1.0, r_exposure.GetFloat() ) * 2.0f - 1.0f ) * 0.01f;

			float exposureOffset = Lerp( -0.01f, 0.01f, idMath::ClampFloat( 0.0, 1.0, r_exposure.GetFloat() ) );

			screenCorrectionParm[ 0 ] = 0.015f + exposureOffset;
			screenCorrectionParm[ 1 ] = 0.005f;
			screenCorrectionParm[ 2 ] = 1;

			// RB: this gives a nice exposure curve in Scilab when using
			// log2( max( 3 + 0..10, 0.001 ) ) as input for exp2
			//float exposureOffset = r_exposure.GetFloat() * 10.0f;
			//screenCorrectionParm[3] = exposureOffset;
		}
	}

	// Draw
	GL_DrawUnitSquare();

	// -----------------------------------------
	// reset states

	GL_ResetTexturesState();
	GL_ResetProgramState();

	GL_State( GLS_DEFAULT );
	GL_Cull( CT_FRONT_SIDED );

	RENDERLOG_CLOSE_BLOCK();
}

static void RB_Bloom( const idRenderView* viewDef )
{
	if( viewDef->is2Dgui || viewDef->isSubview || !r_useHDR.GetBool() )
	{
		return;
	}

	RENDERLOG_OPEN_BLOCK("RB_Bloom");
	RENDERLOG_PRINT( "( avg = %f, max = %f, key = %f )\n", backEnd.hdrAverageLuminance, backEnd.hdrMaxLuminance, backEnd.hdrKey );

	// BRIGHTPASS

	//GL_CheckErrors();

	//Framebuffer::Unbind();
	//globalFramebuffers.hdrQuarterFBO->Bind();

	///	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	//	glClear( GL_COLOR_BUFFER_BIT );

	GL_State( /*GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO |*/ GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
	GL_Cull( CT_TWO_SIDED );

	int screenWidth = renderSystem->GetWidth();
	int screenHeight = renderSystem->GetHeight();

	// set the window clipping
	GL_ViewportAndScissor( 0, 0, screenWidth / 4, screenHeight / 4 );

	GL_SetRenderDestination( renderDestManager.renderDestBloomRender[ 0 ] );

	if( r_useHDR.GetBool() )
	{
		///GL_BindTexture( 0, renderImageManager->currentRenderHDRImage );
		renderProgManager.BindShader_Brightpass();

		renderProgManager.SetRenderParm( RENDERPARM_CURRENTRENDERMAP, renderImageManager->currentRenderHDRImage );
	}
	else {
		renderProgManager.BindShader_Brightpass();

		int x = backEnd.viewDef->viewport.x1;
		int y = backEnd.viewDef->viewport.y1;
		int	w = backEnd.viewDef->viewport.GetWidth();
		int	h = backEnd.viewDef->viewport.GetHeight();

		RENDERLOG_PRINT( "Resolve to %i x %i buffer\n", w, h );

		// resolve the screen
		GL_SelectTexture( 0 );
		GL_CopyCurrentColorToTexture( renderImageManager->currentRenderImage, x, y, w, h );
	}

	auto & screenCorrectionParm = renderProgManager.GetRenderParm( RENDERPARM_SCREENCORRECTIONFACTOR ); // rpScreenCorrectionFactor;
	screenCorrectionParm[ 0 ] = backEnd.hdrKey;
	screenCorrectionParm[ 1 ] = backEnd.hdrAverageLuminance;
	screenCorrectionParm[ 2 ] = backEnd.hdrMaxLuminance;
	screenCorrectionParm[ 3 ] = 1.0f;

	auto & overbright = renderProgManager.GetRenderParm( RENDERPARM_OVERBRIGHT ); // rpOverbright;
	if( r_useHDR.GetBool() )
	{
		if( r_hdrAutoExposure.GetBool() )
		{
			overbright[ 0 ] = r_hdrContrastDynamicThreshold.GetFloat();
		}
		else {
			overbright[ 0 ] = r_hdrContrastStaticThreshold.GetFloat();
		}
		overbright[ 1 ] = r_hdrContrastOffset.GetFloat();
		overbright[ 2 ] = 0;
		overbright[ 3 ] = 0;
	}
	else {
		overbright[ 0 ] = r_ldrContrastThreshold.GetFloat();
		overbright[ 1 ] = r_ldrContrastOffset.GetFloat();
		overbright[ 2 ] = 0;
		overbright[ 3 ] = 0;
	}

	GL_DrawUnitSquare();
	// ---------------------------------------------------------------

	// BLOOM PING PONG rendering
	renderProgManager.BindShader_HDRGlareChromatic();

	int j;
	for( j = 0; j < r_hdrGlarePasses.GetInteger(); j++ )
	{
		///globalFramebuffers.bloomRenderFBO[( j + 1 ) % 2 ]->Bind();
		GL_SetRenderDestination( renderDestManager.renderDestBloomRender[ ( j + 1 ) % 2 ] );

		GL_Clear( true, false, false, 0, 0.0f, 0.0f, 0.0f, 1.0f, false );

		///GL_BindTexture( 0, renderImageManager->bloomRenderImage[ j % 2 ] );
		renderProgManager.SetRenderParm( RENDERPARM_BLOOMRENDERMAP, renderImageManager->bloomRenderImage[ j % 2 ] );

		GL_DrawUnitSquare();
		// ---------------------------------------------------------------
	}

	// add filtered glare back to main context
	GL_SetNativeRenderDestination();

	RB_ResetViewportAndScissorToDefaultCamera( viewDef );

	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );

	renderProgManager.BindShader_Screen();

	///GL_BindTexture( 0, renderImageManager->bloomRenderImage[ ( j + 1 ) % 2 ] );
	renderProgManager.SetRenderParm( RENDERPARM_MAP, renderImageManager->bloomRenderImage[ ( j + 1 ) % 2 ] );

	GL_DrawUnitSquare();
	// ---------------------------------------------------------------

	GL_SetNativeRenderDestination();

	GL_ResetTexturesState();
	GL_ResetProgramState();
	GL_State( GLS_DEFAULT );
	GL_Cull( CT_FRONT_SIDED );

	RENDERLOG_CLOSE_BLOCK();
}

/*
=========================================================================================================
	SSAO
=========================================================================================================
*/
#if 1
static void RB_SSAO( const idRenderView* viewDef )
{
	if( r_useSSAO.GetInteger() <= 0 )
	{
		return;
	}

	if( !viewDef->viewEntitys || viewDef->is2Dgui )
	{
		// 3D views only
		return;
	}

	// FIXME very expensive to enable this in subviews
	if( viewDef->isSubview )
	{
		return;
	}

	RENDERLOG_OPEN_BLOCK( "RB_SSAO" );

#if 0
	GL_CheckErrors();

	// clear the alpha buffer and draw only the hands + weapon into it so
	// we can avoid blurring them
	glClearColor( 0, 0, 0, 1 );
	GL_State( GLS_COLORMASK | GLS_DEPTHMASK );
	glClear( GL_COLOR_BUFFER_BIT );
	GL_Color( 0, 0, 0, 0 );


	GL_SelectTexture( 0 );
	renderImageManager->blackImage->Bind();
	backEnd.currentSpace = NULL;

	drawSurf_t** drawSurfs = ( drawSurf_t** )&backEnd.viewDef->drawSurfs[ 0 ];
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
		GL_DrawIndexed( surf );
	}
	GL_State( GLS_DEPTHFUNC_ALWAYS );

	// copy off the color buffer and the depth buffer for the motion blur prog
	// we use the viewport dimensions for copying the buffers in case resolution scaling is enabled.
	const idScreenRect& viewport = backEnd.viewDef->viewport;
	renderImageManager->currentRenderImage->CopyFramebuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );

	// in stereo rendering, each eye needs to get a separate previous frame mvp
	int mvpIndex = ( backEnd.viewDef->parms.viewEyeBuffer == 1 ) ? 1 : 0;

	// derive the matrix to go from current pixels to previous frame pixels
	idRenderMatrix	inverseMVP;
	idRenderMatrix::Inverse( backEnd.viewDef->GetMVPMatrix(), inverseMVP );

	idRenderMatrix	motionMatrix;
	idRenderMatrix::Multiply( backEnd.prevMVP[ mvpIndex ], inverseMVP, motionMatrix );

	backEnd.prevMVP[ mvpIndex ] = backEnd.viewDef->GetMVPMatrix();

	RB_SetMVP( motionMatrix );
#endif

	backEnd.currentSpace = &backEnd.viewDef->GetWorldSpace();
	RB_SetMVP( backEnd.viewDef->GetMVPMatrix() );

	const bool hdrIsActive( r_useHDR.GetBool() && renderDestManager.renderDestBaseHDR != NULL && GL_IsBound( renderDestManager.renderDestBaseHDR ) );

	int screenWidth = renderSystem->GetWidth();
	int screenHeight = renderSystem->GetHeight();

	// build hierarchical depth buffer
	if( r_useHierarchicalDepthBuffer.GetBool() )
	{
		renderProgManager.BindShader_AmbientOcclusionMinify();

		GL_SelectTexture( 0 );
		//renderImageManager->currentDepthImage->Bind();

		for( int i = 0; i < MAX_HIERARCHICAL_ZBUFFERS; i++ )
		{
			int width  = renderDestManager.renderDestCSDepth[ i ]->GetTargetWidth();
			int height = renderDestManager.renderDestCSDepth[ i ]->GetTargetHeight();

			GL_ViewportAndScissor( 0, 0, width, height );

			///globalFramebuffers.csDepthFBO[ i ]->Bind();
			GL_SetRenderDestination( renderDestManager.renderDestCSDepth[ i ] );

			GL_Clear( true, false, false, 0, 0.0, 0.0, 0.0, 1.0, false );

			if( i == 0 )
			{
				renderProgManager.BindShader_AmbientOcclusionReconstructCSZ();

				///GL_BindTexture( 0, renderImageManager->currentDepthImage );
				renderProgManager.SetRenderParm( RENDERPARM_CURRENTDEPTHMAP, renderImageManager->currentDepthImage );
			}
			else {
				renderProgManager.BindShader_AmbientOcclusionMinify();

				///GL_BindTexture( 0, renderImageManager->hierarchicalZbufferImage );
				renderProgManager.SetRenderParm( RENDERPARM_CURRENTDEPTHMAP, renderImageManager->hierarchicalZbufferImage );
			}

			auto & jitterTexScale = renderProgManager.GetRenderParm( RENDERPARM_JITTERTEXSCALE ); // rpJitterTexScale;
			jitterTexScale[ 0 ] = i - 1;
			jitterTexScale[ 1 ] = 0;
			jitterTexScale[ 2 ] = 0;
			jitterTexScale[ 3 ] = 0;
		#if 1
			auto & screenCorrectionParm = renderProgManager.GetRenderParm( RENDERPARM_SCREENCORRECTIONFACTOR ); // rpScreenCorrectionFactor
			screenCorrectionParm[ 0 ] = 1.0f / width;
			screenCorrectionParm[ 1 ] = 1.0f / height;
			screenCorrectionParm[ 2 ] = width;
			screenCorrectionParm[ 3 ] = height;
		#endif

			GL_DrawUnitSquare();
			// -----------------------------------------------------------------------
		}
	}

	// set the window clipping
	GL_ViewportAndScissor( 0, 0, screenWidth, screenHeight );

	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
	GL_Cull( CT_TWO_SIDED );

	if( r_ssaoFiltering.GetBool() )
	{
		///renderDestManager.renderDestAmbientOcclusion[ 0 ]->Bind();
		GL_SetRenderDestination( renderDestManager.renderDestAmbientOcclusion[ 0 ] );

		GL_Clear( true, false, false, 0, 0.0, 0.0, 0.0, 0.0, false );

		renderProgManager.BindShader_AmbientOcclusion();
	}
	else {
		if( r_ssaoDebug.GetInteger() <= 0 )
		{
			GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_ALPHAMASK | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
		}

		RB_ResetRenderDest( hdrIsActive );

		renderProgManager.BindShader_AmbientOcclusionAndOutput();
	}

	auto & screenCorrectionParm = renderProgManager.GetRenderParm( RENDERPARM_SCREENCORRECTIONFACTOR ); // rpScreenCorrectionFactor;
	screenCorrectionParm[ 0 ] = 1.0f / screenWidth;
	screenCorrectionParm[ 1 ] = 1.0f / screenHeight;
	screenCorrectionParm[ 2 ] = screenWidth;
	screenCorrectionParm[ 3 ] = screenHeight;

#if 0
	// RB: set unprojection matrices so we can convert zbuffer values back to camera and world spaces
	idRenderMatrix cameraToWorldMatrix;
	if( !idRenderMatrix::Inverse( backEnd.viewDef->GetViewMatrix(), cameraToWorldMatrix ) )
	{
		idLib::Warning( "cameraToWorldMatrix invert failed" );
	}

	renderProgManager.SetRenderParms( RENDERPARM_MODELMATRIX_X, cameraToWorldMatrix[ 0 ], 4 );
	//renderProgManager.SetRenderParms( RENDERPARM_MODELMATRIX_X, viewDef->unprojectionToWorldRenderMatrix[0], 4 );
#endif
	renderProgManager.SetRenderParms( RENDERPARM_MODELMATRIX_X, viewDef->GetInverseProjMatrix().Ptr(), 4 );


	auto & jitterTexOffset = renderProgManager.GetRenderParm( RENDERPARM_JITTERTEXOFFSET ); // rpJitterTexOffset;
	if( r_shadowMapRandomizeJitter.GetBool() )
	{
		jitterTexOffset[ 0 ] = ( rand() & 255 ) / 255.0;
		jitterTexOffset[ 1 ] = ( rand() & 255 ) / 255.0;
	}
	else {
		jitterTexOffset[ 0 ] = 0;
		jitterTexOffset[ 1 ] = 0;
	}
	jitterTexOffset[ 2 ] = viewDef->GetGameTimeSec();
	jitterTexOffset[ 3 ] = 0.0f;

	///GL_BindTexture( 0, renderImageManager->currentNormalsImage ); // viewNormalMap
	///GL_BindTexture( 1, ( r_useHierarchicalDepthBuffer.GetBool() ) ? renderImageManager->hierarchicalZbufferImage : renderImageManager->currentDepthImage );
	renderProgManager.SetRenderParm( RENDERPARM_VIEWNORMALMAP, renderImageManager->currentNormalsImage );
	renderProgManager.SetRenderParm( RENDERPARM_CURRENTDEPTHMAP, r_useHierarchicalDepthBuffer.GetBool() ? renderImageManager->hierarchicalZbufferImage : renderImageManager->currentDepthImage );

	GL_DrawUnitSquare(); 
	// --------------------------------------------------------------------

	if( r_ssaoFiltering.GetBool() )
	{
		idRenderVector jitterTexScale;

		// AO blur X
	#if 1
		///renderDestManager.renderDestAmbientOcclusion[ 1 ]->Bind();
		GL_SetRenderDestination( renderDestManager.renderDestAmbientOcclusion[ 1 ] );

		renderProgManager.BindShader_AmbientOcclusionBlur();

		// set axis parameter
		jitterTexScale[ 0 ] = 1;
		jitterTexScale[ 1 ] = 0;
		jitterTexScale[ 2 ] = 0;
		jitterTexScale[ 3 ] = 0;
		renderProgManager.SetRenderParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale

		///GL_BindTexture( 2, renderImageManager->ambientOcclusionImage[ 0 ] );
		renderProgManager.SetRenderParm( RENDERPARM_MAP, renderImageManager->ambientOcclusionImage[ 0 ] );

		GL_DrawUnitSquare();
		// --------------------------------------------------------------------
	#endif

		// AO blur Y
		RB_ResetRenderDest( hdrIsActive );

		if( r_ssaoDebug.GetInteger() <= 0 )
		{
			GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
		}

		renderProgManager.BindShader_AmbientOcclusionBlurAndOutput();

		// set axis parameter
		jitterTexScale[ 0 ] = 0;
		jitterTexScale[ 1 ] = 1;
		jitterTexScale[ 2 ] = 0;
		jitterTexScale[ 3 ] = 0;
		renderProgManager.SetRenderParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale

		///GL_BindTexture( 2, renderImageManager->ambientOcclusionImage[ 1 ] );
		renderProgManager.SetRenderParm( RENDERPARM_MAP, renderImageManager->ambientOcclusionImage[ 1 ] );

		GL_DrawUnitSquare();
		// --------------------------------------------------------------------
	}

	GL_ResetProgramState();

	GL_State( GLS_DEFAULT );
	GL_Cull( CT_FRONT_SIDED );

	//GL_CheckErrors();

	RENDERLOG_CLOSE_BLOCK();
}
#endif

#if 0
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

	RENDERLOG_PRINT( "---------- RB_SSGI() ----------\n" );

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
		renderImageManager->currentRenderImage->CopyFramebuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );
	}

	// build hierarchical depth buffer
	if( r_useHierarchicalDepthBuffer.GetBool() )
	{
		renderProgManager.BindShader_AmbientOcclusionMinify();

		///glClearColor( 0, 0, 0, 1 );

		GL_SelectTexture( 0 );
		//renderImageManager->currentDepthImage->Bind();

		for( int i = 0; i < MAX_HIERARCHICAL_ZBUFFERS; i++ )
		{
			int width = globalFramebuffers.csDepthFBO[ i ]->GetWidth();
			int height = globalFramebuffers.csDepthFBO[ i ]->GetHeight();

			GL_ViewportAndScissor( 0, 0, width, height );

			globalFramebuffers.csDepthFBO[ i ]->Bind();

			///glClear( GL_COLOR_BUFFER_BIT );
			GL_Clear( true, false, false, 0, 0.0, 0.0, 0.0, 1.0, false );

			if( i == 0 )
			{
				renderProgManager.BindShader_AmbientOcclusionReconstructCSZ();

				renderImageManager->currentDepthImage->Bind();
			}
			else
			{
				renderProgManager.BindShader_AmbientOcclusionMinify();

				GL_BindTexture( 0, renderImageManager->hierarchicalZbufferImage );
			}

			idRenderVector jitterTexScale;
			jitterTexScale[ 0 ] = i - 1;
			jitterTexScale[ 1 ] = 0;
			jitterTexScale[ 2 ] = 0;
			jitterTexScale[ 3 ] = 0;
			renderProgManager.SetRenderParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale
		#if 1
			idRenderVector screenCorrectionParm;
			screenCorrectionParm[ 0 ] = 1.0f / width;
			screenCorrectionParm[ 1 ] = 1.0f / height;
			screenCorrectionParm[ 2 ] = width;
			screenCorrectionParm[ 3 ] = height;
			renderProgManager.SetRenderParm( RENDERPARM_SCREENCORRECTIONFACTOR, screenCorrectionParm ); // rpScreenCorrectionFactor
		#endif

			GL_DrawIndexed( &backEnd.unitSquareSurface );
		}
	}

	// set the window clipping
	GL_ViewportAndScissor( 0, 0, screenWidth, screenHeight );

	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
	GL_Cull( CT_TWO_SIDED );

	if( r_ssgiFiltering.GetBool() )
	{
		globalFramebuffers.ambientOcclusionFBO[ 0 ]->Bind();

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

	idRenderVector screenCorrectionParm;
	screenCorrectionParm[ 0 ] = 1.0f / screenWidth;
	screenCorrectionParm[ 1 ] = 1.0f / screenHeight;
	screenCorrectionParm[ 2 ] = screenWidth;
	screenCorrectionParm[ 3 ] = screenHeight;
	renderProgManager.SetRenderParm( RENDERPARM_SCREENCORRECTIONFACTOR, screenCorrectionParm ); // rpScreenCorrectionFactor

#if 0
																				// RB: set unprojection matrices so we can convert zbuffer values back to camera and world spaces
	idRenderMatrix cameraToWorldMatrix;
	if( !idRenderMatrix::Inverse( backEnd.viewDef->GetViewMatrix(), cameraToWorldMatrix ) )
	{
		idLib::Warning( "cameraToWorldMatrix invert failed" );
	}

	renderProgManager.SetRenderParms( RENDERPARM_MODELMATRIX_X, cameraToWorldMatrix[ 0 ], 4 );
	//renderProgManager.SetRenderParms( RENDERPARM_MODELMATRIX_X, viewDef->unprojectionToWorldRenderMatrix[0], 4 );
#endif
	renderProgManager.SetRenderParms( RENDERPARM_MODELMATRIX_X, viewDef->GetInverseProjMatrix().Ptr(), 4 );


	idRenderVector jitterTexOffset;
	if( r_shadowMapRandomizeJitter.GetBool() )
	{
		jitterTexOffset[ 0 ] = ( rand() & 255 ) / 255.0;
		jitterTexOffset[ 1 ] = ( rand() & 255 ) / 255.0;
	}
	else
	{
		jitterTexOffset[ 0 ] = 0;
		jitterTexOffset[ 1 ] = 0;
	}
	jitterTexOffset[ 2 ] = viewDef->GetGameTimeSec();
	jitterTexOffset[ 3 ] = 0.0f;
	renderProgManager.SetRenderParm( RENDERPARM_JITTERTEXOFFSET, jitterTexOffset ); // rpJitterTexOffset

	GL_BindTexture( 0, renderImageManager->currentNormalsImage );
	GL_BindTexture( 1, ( r_useHierarchicalDepthBuffer.GetBool() ) ? renderImageManager->hierarchicalZbufferImage : renderImageManager->currentDepthImage );
	GL_BindTexture( 2, ( hdrIsActive ) ? renderImageManager->currentRenderHDRImage : renderImageManager->currentRenderImage );

	GL_DrawIndexed( &backEnd.unitSquareSurface );

	if( r_ssgiFiltering.GetBool() )
	{
		idRenderVector jitterTexScale;

		// AO blur X
	#if 1
		globalFramebuffers.ambientOcclusionFBO[ 1 ]->Bind();

		renderProgManager.BindShader_DeepGBufferRadiosityBlur();

		// set axis parameter
		jitterTexScale[ 0 ] = 1;
		jitterTexScale[ 1 ] = 0;
		jitterTexScale[ 2 ] = 0;
		jitterTexScale[ 3 ] = 0;
		renderProgManager.SetRenderParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale

		GL_BindTexture( 2, renderImageManager->ambientOcclusionImage[ 0 ] );

		GL_DrawIndexed( &backEnd.unitSquareSurface );
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
		jitterTexScale[ 0 ] = 0;
		jitterTexScale[ 1 ] = 1;
		jitterTexScale[ 2 ] = 0;
		jitterTexScale[ 3 ] = 0;
		renderProgManager.SetRenderParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale

		GL_BindTexture( 2, renderImageManager->ambientOcclusionImage[ 1 ] );

		GL_DrawIndexed( &backEnd.unitSquareSurface );
	}

	renderProgManager.Unbind();

	GL_State( GLS_DEFAULT );
	GL_Cull( CT_FRONT_SIDED );

	//GL_CheckErrors();
}
// RB end
#endif
/*
=========================================================================================================

	BUFFERS

=========================================================================================================
*/

void RB_InitBuffers()
{
	backEnd.viewUniformBuffer.AllocBufferObject( ( sizeof( idRenderMatrix ) * 6 ) + ( sizeof( idRenderVector ) * 5 ), BU_DEFAULT );
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
	*( vecs++ ) = idRenderVector( 1.0f, 1.0f, 0.0f, 1.0f ); // g_ScreenCorrectionFactor

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
=========================================================================================================
	RB_MotionBlur
=========================================================================================================
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

	RENDERLOG_OPEN_BLOCK( "RB_MotionBlur" );

	GL_CheckErrors();

	// clear the alpha buffer and draw only the hands + weapon into it so
	// we can avoid blurring them
	GL_State( GLS_COLORMASK | GLS_DEPTHMASK );
	GL_Clear( true, false, false, 0, 0.0, 0.0, 0.0, 1.0, false );
	GL_Color( 0, 0, 0, 0 );
	///GL_BindTexture( 0, renderImageManager->blackImage );
	renderProgManager.SetRenderParm( RENDERPARM_MAP, renderImageManager->blackImage );

	backEnd.ClearCurrentSpace();

	for( int surfNum = 0; surfNum < backEnd.viewDef->numDrawSurfs; ++surfNum )
	{
		const drawSurf_t * const surf = backEnd.viewDef->drawSurfs[ surfNum ];

		if( !surf->space->weaponDepthHack && !surf->space->skipMotionBlur && !surf->material->HasSubview() )
		{
			// Apply motion blur to this object
			continue;
		}
		if( surf->material->Coverage() == MC_TRANSLUCENT )
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
		renderProgManager.BindShader_TextureVertexColor( surf->jointCache );

		// draw it solid
		GL_DrawIndexed( surf );
	}

	const bool useHDR = r_useHDR.GetBool() && !backEnd.viewDef->is2Dgui;

	GL_State( GLS_DEPTHFUNC_ALWAYS );

	//if( !useHDR ) {
	// copy off the color buffer and the depth buffer for the motion blur prog
	// we use the viewport dimensions for copying the buffers in case resolution scaling is enabled.
	const idScreenRect& viewport = backEnd.viewDef->GetViewport();
	GL_CopyCurrentColorToTexture( renderImageManager->currentRenderImage, viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );
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
	renderProgManager.SetRenderParm( RENDERPARM_OVERBRIGHT, samples );
	//if( !useHDR ) 
	//{
	///GL_BindTexture( 0, renderImageManager->currentRenderImage );
	///GL_BindTexture( 1, renderImageManager->currentDepthImage );
	renderProgManager.SetRenderParm( RENDERPARM_CURRENTRENDERMAP, renderImageManager->currentRenderImage );
	renderProgManager.SetRenderParm( RENDERPARM_CURRENTDEPTHMAP, renderImageManager->currentDepthImage );
	//} 
	//else {
	//	GL_BindTexture( 0, renderImageManager->currentRenderHDRImage );
	//	GL_BindTexture( 1, renderImageManager->currentDepthImage );
	//}
	GL_DrawUnitSquare();

	GL_CheckErrors();

	RENDERLOG_CLOSE_BLOCK();
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
	const drawSurf_t * const * drawSurfs = ( drawSurf_t** )&viewDef->drawSurfs[ 0 ];
	const int numDrawSurfs = viewDef->numDrawSurfs;

	for( int i = 0; i < numDrawSurfs; ++i )
	{
		const drawSurf_t* ds = viewDef->drawSurfs[ i ];
		if( ds->material != NULL )
		{
			const_cast<idMaterial*>( ds->material )->EnsureNotPurged();
		}
	}

	RB_UpdateBuffers( viewDef );
	
	//-------------------------------------------------
	// RB_BeginDrawingView
	//
	// Any mirrored or portaled views have already been drawn, so prepare
	// to actually render the visible surfaces for this view
	//
	// clear the z buffer, set the projection matrix, etc
	//-------------------------------------------------
	RB_ResetViewportAndScissorToDefaultCamera( viewDef );

	backEnd.glState.faceCulling = -1;		// force face culling to set next time

	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT );

	const bool useHDR = r_useHDR.GetBool() && !viewDef->is2Dgui;

	// Clear the depth buffer and clear the stencil to 128 for stencil shadows as well as gui masking
	GL_Clear( false, true, true, STENCIL_SHADOW_TEST_VALUE, 0.0f, 0.0f, 0.0f, 0.0f, useHDR );

	RB_ResetRenderDest( useHDR );

	//GL_CheckErrors();

	// normal face culling
	GL_Cull( CT_FRONT_SIDED );

	// bind one global Vertex Array Object (VAO)
	{
		glBindVertexArray( glConfig.global_vao );

		if( glConfig.ARB_vertex_attrib_binding && r_useVertexAttribFormat.GetBool() )
		{
			const GLuint bindingIndex = 0;
			glVertexAttribBinding( PC_ATTRIB_INDEX_POSITION, bindingIndex );
			glVertexAttribBinding( PC_ATTRIB_INDEX_NORMAL, bindingIndex );
			glVertexAttribBinding( PC_ATTRIB_INDEX_COLOR, bindingIndex );
			glVertexAttribBinding( PC_ATTRIB_INDEX_COLOR2, bindingIndex );
			glVertexAttribBinding( PC_ATTRIB_INDEX_ST, bindingIndex );
			glVertexAttribBinding( PC_ATTRIB_INDEX_TANGENT, bindingIndex );

			//glVertexAttribFormat( PC_ATTRIB_INDEX_POSITION,  3, GL_FLOAT,			GL_FALSE, ( GLsizei )idDrawVert::positionOffset );
			//glVertexAttribFormat( PC_ATTRIB_INDEX_NORMAL,  4, GL_UNSIGNED_BYTE, GL_TRUE,  ( GLsizei )idDrawVert::normalOffset );
			//glVertexAttribFormat( PC_ATTRIB_INDEX_COLOR,   4, GL_UNSIGNED_BYTE, GL_TRUE,  ( GLsizei )idDrawVert::colorOffset );
			//glVertexAttribFormat( PC_ATTRIB_INDEX_COLOR2,  4, GL_UNSIGNED_BYTE, GL_TRUE,  ( GLsizei )idDrawVert::color2Offset );
			//glVertexAttribFormat( PC_ATTRIB_INDEX_ST,	   2, GL_HALF_FLOAT,	GL_TRUE,  ( GLsizei )idDrawVert::texcoordOffset );
			//glVertexAttribFormat( PC_ATTRIB_INDEX_TANGENT, 4, GL_UNSIGNED_BYTE, GL_TRUE,  ( GLsizei )idDrawVert::tangentOffset );
		}
	}

	//------------------------------------
	// sets variables that can be used by all programs
	//------------------------------------
	{
/*		idRenderVector parm;

		// set eye position in global space
		parm[ 0 ] = viewDef->GetOrigin()[ 0 ];
		parm[ 1 ] = viewDef->GetOrigin()[ 1 ];
		parm[ 2 ] = viewDef->GetOrigin()[ 2 ];
		parm[ 3 ] = 1.0f;
		renderProgManager.SetRenderParm( RENDERPARM_GLOBALEYEPOS, parm.ToFloatPtr() ); // rpGlobalEyePos

		// sets overbright to make world brighter
		// This value is baked into the specularScale and diffuseScale values so
		// the interaction programs don't need to perform the extra multiply,
		// but any other renderprogs that want to obey the brightness value
		// can reference this.
		float overbright = r_lightScale.GetFloat() * 0.5f;
		parm[ 0 ] = overbright;
		parm[ 1 ] = overbright;
		parm[ 2 ] = overbright;
		parm[ 3 ] = overbright;
		renderProgManager.SetRenderParm( RENDERPARM_OVERBRIGHT, parm.ToFloatPtr() );

		// Set Projection Matrix
		renderProgManager.SetRenderParms( RENDERPARM_PROJMATRIX_X, viewDef->GetProjectionMatrix().Ptr(), 4 );
*/
		glBindBufferRange( GL_UNIFORM_BUFFER, BINDING_GLOBAL_UBO,
			GetGLObject( backEnd.viewUniformBuffer.GetAPIObject() ),
			backEnd.viewUniformBuffer.GetOffset(), backEnd.viewUniformBuffer.GetSize() );

		if( r_useProgUBO.GetBool() ) 
		{
			glBindBufferRange( GL_UNIFORM_BUFFER, BINDING_PROG_PARMS_UBO,
				GetGLObject( backEnd.progParmsUniformBuffer.GetAPIObject() ),
				backEnd.progParmsUniformBuffer.GetOffset(), backEnd.progParmsUniformBuffer.GetSize() );
		}
	}

	//-------------------------------------------------
	// fill the depth buffer and clear color buffer to black except on subviews
	//-------------------------------------------------
	RB_FillDepthBufferFast( drawSurfs, numDrawSurfs );

	//-------------------------------------------------
	// capture the depth for the motion blur before rendering any post process surfaces that may contribute to the depth
	//-------------------------------------------------
	if( !r_useHDR.GetBool() )
	{
		auto & viewport = viewDef->GetViewport();
		GL_CopyCurrentDepthToTexture( renderImageManager->currentDepthImage, viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );
	}

	//-------------------------------------------------
	// FIXME, OPTIMIZE: merge this with FillDepthBufferFast like in a light prepass deferred renderer
	//
	// fill the geometric buffer with normals and roughness,
	// fill the depth buffer and the color buffer with precomputed Q3A style lighting
	//-------------------------------------------------
	RB_AmbientPass( drawSurfs, numDrawSurfs );

	//-------------------------------------------------
	// main light renderer
	//-------------------------------------------------
	RB_DrawInteractionsForward( viewDef );

	//-------------------------------------------------
	// darken the scene using the screen space ambient occlusion
	//-------------------------------------------------
	RB_SSAO( viewDef );
	///RB_SSGI( viewDef );

	//-------------------------------------------------
	// now draw any non-light dependent shading passes
	//-------------------------------------------------
	int processed = 0;
	if( !r_skipTransMaterialPasses.GetBool() )
	{
		RENDERLOG_OPEN_MAINBLOCK( MRB_DRAW_SHADER_PASSES );

		// guiScreenOffset will be 0 in non-gui views
		const float guiScreenOffset = viewDef->Is2DView() ? stereoEye * viewDef->GetStereoScreenSeparation() : 0.0f;

		processed = RB_DrawTransMaterialPasses( drawSurfs, numDrawSurfs, guiScreenOffset, stereoEye );
		RENDERLOG_CLOSE_MAINBLOCK();
	}

	//-------------------------------------------------
	// use direct light and emissive light contributions to add indirect screen space light
	//-------------------------------------------------
	///RB_SSGI( viewDef );

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
		const int x = viewDef->GetViewport().x1;
		const int y = viewDef->GetViewport().y1;
		const int w = viewDef->GetViewport().GetWidth();
		const int h = viewDef->GetViewport().GetHeight();

		RENDERLOG_PRINT( "Resolve current render to %i x %i buffer\n", w, h );

		GL_SelectTexture( 0 );

		// resolve the screen
		GL_CopyCurrentColorToTexture( renderImageManager->currentRenderImage, x, y, w, h );
		backEnd.currentRenderCopied = true;

		// RENDERPARM_SCREENCORRECTIONFACTOR amd RENDERPARM_WINDOWCOORD overlap
		// diffuseScale and specularScale

		// screen power of two correction factor (no longer relevant now)
		idRenderVector & screenCorrectionParm = renderProgManager.GetRenderParm( RENDERPARM_SCREENCORRECTIONFACTOR ); // rpScreenCorrectionFactor;
		screenCorrectionParm[ 0 ] = 1.0f;
		screenCorrectionParm[ 1 ] = 1.0f;
		screenCorrectionParm[ 2 ] = 0.0f;
		screenCorrectionParm[ 3 ] = 1.0f;		
/*
		// window coord to 0.0 to 1.0 conversion
		float windowCoordParm[ 4 ];
		windowCoordParm[ 0 ] = 1.0f / w;
		windowCoordParm[ 1 ] = 1.0f / h;
		windowCoordParm[ 2 ] = 0.0f;
		windowCoordParm[ 3 ] = 1.0f;
		renderProgManager.SetRenderParm( RENDERPARM_WINDOWCOORD, windowCoordParm ); // rpWindowCoord

		// ---> sikk - Included non-power-of-two/frag position conversion
		// screen power of two correction factor, assuming the copy to _currentRender
		// also copied an extra row and column for the bilerp
		idRenderVector parm;
		parm[ 0 ] = ( float )w / renderImageManager->currentRenderImage->GetUploadWidth();
		parm[ 1 ] = ( float )h / renderImageManager->currentRenderImage->GetUploadHeight();
		parm[ 2 ] = parm[ 0 ] / w;	// sikk - added - one less fragment shader instruction
		parm[ 3 ] = parm[ 1 ] / h;	// sikk - added - one less fragment shader instruction
		renderProgManager.SetRenderParm( RENDERPARM_WINDOWCOORD, parm ); // rpCurrentRenderParms
*/
		// render the remaining surfaces
		RENDERLOG_OPEN_MAINBLOCK( MRB_DRAW_SHADER_PASSES_POST );
		RB_DrawTransMaterialPasses( drawSurfs + processed, numDrawSurfs - processed, 0.0f /* definitely not a gui */, stereoEye );
		RENDERLOG_CLOSE_MAINBLOCK();
	}

	// RB: convert back from HDR to LDR range
	if( useHDR /*&& !viewDef->isSubview*/ )
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
			/*glBindFramebuffer( GL_READ_FRAMEBUFFER_EXT, globalFramebuffers.hdrFBO->GetFramebuffer() );
			glBindFramebuffer( GL_DRAW_FRAMEBUFFER_EXT, globalFramebuffers.hdr64FBO->GetFramebuffer() );
			glBlitFramebuffer( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight(),
			0, 0, 64, 64,
			GL_COLOR_BUFFER_BIT,
			GL_LINEAR );*/
			GL_BlitRenderBuffer( //SEA: fix sizes for subviews!
				//renderDestManager.renderDestBaseHDR, idScreenRect( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() ),
				renderDestManager.renderDestBaseHDR, idScreenRect( viewDef->GetViewport().x1, viewDef->GetViewport().y1, viewDef->GetViewport().GetWidth(), viewDef->GetViewport().GetHeight() ),
				renderDestManager.renderDestBaseHDRsml64, idScreenRect( 0, 0, 64, 64 ),
				true, true );
		}

		RB_CalculateAdaptation();

		RB_Tonemap( viewDef );
	}

	RB_Bloom( viewDef );
	// RB end

	RB_MotionBlur();

	//-------------------------------------------------
	// render debug tools
	//-------------------------------------------------
	RB_RenderDebugTools( drawSurfs, numDrawSurfs );

	RENDERLOG_CLOSE_BLOCK();
}

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

	if( backEnd.viewDef->viewEntitys ) 
	{
		RENDERLOG_OPEN_BLOCK( backEnd.viewDef->isSubview ? "RB_CMD_DrawSubview3D" : "RB_CMD_DrawView3D" );
	} 
	else {
		RENDERLOG_OPEN_BLOCK( backEnd.viewDef->isSubview ? "RB_CMD_DrawSubview2D" : "RB_CMD_DrawView2D" );
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

	// restore the context for 2D drawing if we were stubbing it out
	//if( r_skipRenderContext.GetBool() && backEnd.viewDef->viewEntitys )
	//{
	//	GLimp_ActivateContext();
	//	GL_SetDefaultState();
	//}

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
	if( rs_enable.GetInteger() == 0 && !r_useFilmicPostProcessEffects.GetBool() && r_antiAliasing.GetInteger() == 0 )
	{
		return;
	}

	if( ( r_ssaoDebug.GetInteger() > 0 ) || ( r_ssgiDebug.GetInteger() > 0 ) )
	{
		return;
	}

	RENDERLOG_OPEN_BLOCK( "RB_CMD_PostProcess" );

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
		*                           |input|------------------·
		*                              v                     |
		*                    [ SMAA*EdgeDetection ]          |
		*                              v                     |
		*                          |edgesTex|                |
		*                              v                     |
		*              [ SMAABlendingWeightCalculation ]     |
		*                              v                     |
		*                          |blendTex|                |
		*                              v                     |
		*                [ SMAANeighborhoodBlending ] <------·
		*                              v
		*                           |output|
		*/

		GL_CopyCurrentColorToTexture( renderImageManager->smaaInputImage, 0, 0, screenWidth, screenHeight );

		renderProgManager.BindShader_SMAA_EdgeDetection();

		// set SMAA_RT_METRICS = rpScreenCorrectionFactor
		auto & screenCorrectionParm = renderProgManager.GetRenderParm( RENDERPARM_SCREENCORRECTIONFACTOR );
		screenCorrectionParm[ 0 ] = 1.0f / screenWidth;
		screenCorrectionParm[ 1 ] = 1.0f / screenHeight;
		screenCorrectionParm[ 2 ] = screenWidth;
		screenCorrectionParm[ 3 ] = screenHeight;

		///globalFramebuffers.smaaEdgesFBO->Bind();
		GL_SetRenderDestination( renderDestManager.renderDestSMAAEdges );
		GL_ClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
		//GL_Clear( true, false, false, 0, 0.0f, 0.0f, 0.0f, 0.0f, false );

		///GL_BindTexture( 0, renderImageManager->smaaInputImage );
		renderProgManager.SetRenderParm( RENDERPARM_SMAAINPUTIMAGE, renderImageManager->smaaInputImage );

		GL_DrawUnitSquare();
		// ---------------------------------------------------------
	#if 1
		//renderImageManager->smaaEdgesImage->CopyFramebuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );

		renderProgManager.BindShader_SMAA_BlendingWeightCalculation();

		///globalFramebuffers.smaaBlendFBO->Bind();
		GL_SetRenderDestination( renderDestManager.renderDestSMAABlend );
		GL_ClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
		//GL_Clear( true, false, false, 0, 0.0f, 0.0f, 0.0f, 0.0f, false );

		///GL_BindTexture( 0, renderImageManager->smaaEdgesImage );
		///GL_BindTexture( 1, renderImageManager->smaaAreaImage );
		///GL_BindTexture( 2, renderImageManager->smaaSearchImage );
		renderProgManager.SetRenderParm( RENDERPARM_SMAAEDGESIMAGE, renderImageManager->smaaEdgesImage );
		renderProgManager.SetRenderParm( RENDERPARM_SMAAAREAIMAGE, renderImageManager->smaaAreaImage );
		renderProgManager.SetRenderParm( RENDERPARM_SMAASEARCHIMAGE, renderImageManager->smaaSearchImage );

		
		GL_DrawUnitSquare();
		// ---------------------------------------------------------

		renderProgManager.BindShader_SMAA_NeighborhoodBlending();

		GL_SetNativeRenderDestination();
	#endif

	#if 1
		GL_ResetTexturesState();

		//GL_SelectTexture( 0 );
		//renderImageManager->smaaBlendImage->CopyFramebuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );

		///GL_BindTexture( 0, renderImageManager->smaaInputImage );
		///GL_BindTexture( 1, renderImageManager->smaaBlendImage );
		renderProgManager.SetRenderParm( RENDERPARM_SMAAINPUTIMAGE, renderImageManager->smaaInputImage );
		renderProgManager.SetRenderParm( RENDERPARM_SMAABLENDIMAGE, renderImageManager->smaaBlendImage );

		
		GL_DrawUnitSquare();
		// ---------------------------------------------------------
	#endif
	}

#if 1
	if( r_useFilmicPostProcessEffects.GetBool() )
	{
		GL_CopyCurrentColorToTexture( renderImageManager->currentRenderImage, viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );

		renderProgManager.BindShader_PostProcess();

		///GL_BindTexture( 0, renderImageManager->currentRenderImage );
		///GL_BindTexture( 1, renderImageManager->grainImage1 );
		renderProgManager.SetRenderParm( RENDERPARM_CURRENTRENDERMAP, renderImageManager->currentRenderImage );
		renderProgManager.SetRenderParm( RENDERPARM_GRAINMAP, renderImageManager->grainImage1 );

		const static int GRAIN_SIZE = 128;

		// screen power of two correction factor	
		// rpScreenCorrectionFactor
		auto & screenCorrectionParm = renderProgManager.GetRenderParm( RENDERPARM_SCREENCORRECTIONFACTOR );
		screenCorrectionParm[ 0 ] = 1.0f / GRAIN_SIZE;
		screenCorrectionParm[ 1 ] = 1.0f / GRAIN_SIZE;
		screenCorrectionParm[ 2 ] = 1.0f;
		screenCorrectionParm[ 3 ] = 1.0f;

		// rpJitterTexOffset
		auto & jitterTexOffset = renderProgManager.GetRenderParm( RENDERPARM_JITTERTEXOFFSET );
		if( r_shadowMapRandomizeJitter.GetBool() )
		{
			jitterTexOffset[ 0 ] = ( rand() & 255 ) / 255.0;
			jitterTexOffset[ 1 ] = ( rand() & 255 ) / 255.0;
		}
		else {
			jitterTexOffset[ 0 ] = 0;
			jitterTexOffset[ 1 ] = 0;
		}
		jitterTexOffset[ 2 ] = 0.0f;
		jitterTexOffset[ 3 ] = 0.0f;

		GL_DrawUnitSquare();
		// ---------------------------------------------------------
	}
#endif

	GL_ResetTexturesState();
	GL_ResetProgramState();

	RENDERLOG_CLOSE_BLOCK();
}
