
#include "precompiled.h"
#pragma hdrstop

#include "Render.h"

/*
=========================================================================================================

	SCREEN SPACE AMBIENT OCLUSION RENDER PASSES

=========================================================================================================
*/

idCVar r_useSSAO( "r_useSSAO", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "use screen space ambient occlusion to darken corners" );
idCVar r_ssaoDebug( "r_ssaoDebug", "0", CVAR_RENDERER | CVAR_INTEGER, "" );
idCVar r_ssaoFiltering( "r_ssaoFiltering", "1", CVAR_RENDERER | CVAR_BOOL, "" );
idCVar r_useHierarchicalDepthBuffer( "r_useHierarchicalDepthBuffer", "1", CVAR_RENDERER | CVAR_BOOL, "" );

void RB_SSAO()
{
	RENDERLOG_OPEN_BLOCK( "RB_SSAO" );

#if 0
	GL_CheckErrors();

	// clear the alpha buffer and draw only the hands + weapon into it so
	// we can avoid blurring them
	glClearColor( 0, 0, 0, 1 );
	GL_State( GLS_COLORMASK | GLS_DEPTHMASK );
	glClear( GL_COLOR_BUFFER_BIT );
	renderProgManager.SetColorParm( 0, 0, 0, 0 );

	GL_SelectTexture( 0, renderImageManager->blackImage );

	backEnd.ClearCurrentSpace();

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
			renderProgManager.SetMVPMatrixParms( surf->space->mvp );
			backEnd.currentSpace = surf->space;
		}

		// this could just be a color, but we don't have a skinned color-only prog
		GL_SetRenderProgram( surf->jointCache ? renderProgManager.prog_texture_color_skinned : renderProgManager.prog_texture_color );

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

	renderProgManager.SetMVPMatrixParms( motionMatrix );
#endif
	auto const renderView = backEnd.viewDef;
	auto const renderDestDefault = renderDestManager.renderDestBaseHDR;

	backEnd.currentSpace = &renderView->GetWorldSpace();
	renderProgManager.SetMVPMatrixParms( renderView->GetMVPMatrix() );

	assert( renderDestDefault != NUL );
	const bool hdrIsActive( r_useHDR.GetBool() && GL_IsBound( renderDestDefault ) );

	int screenWidth = renderSystem->GetWidth();
	int screenHeight = renderSystem->GetHeight();

	// build hierarchical depth buffer
	if( r_useHierarchicalDepthBuffer.GetBool() )
	{
		GL_ResetTextureState();

		for( int i = 0; i < MAX_HIERARCHICAL_ZBUFFERS; i++ )
		{
			const int width  = renderDestManager.renderDestCSDepth[ i ]->GetTargetWidth();
			const int height = renderDestManager.renderDestCSDepth[ i ]->GetTargetHeight();

			GL_SetRenderDestination( renderDestManager.renderDestCSDepth[ i ] );
			GL_ViewportAndScissor( 0, 0, width, height );

			GL_State( GLS_DEFAULT | GLS_FRONTSIDED );//?

			GL_Clear( true, false, false, 0, 0.0, 0.0, 0.0, 1.0 );

			if( i == 0 )
			{
				// Ambient Occlusion Reconstruct CSZ
				GL_SetRenderProgram( renderProgManager.prog_AmbientOcclusion_minify_mip0 );
				renderProgManager.SetRenderParm( RENDERPARM_VIEWDEPTHSTENCILMAP, renderDestDefault->GetDepthStencilImage() );
			}
			else {
				GL_SetRenderProgram( renderProgManager.prog_AmbientOcclusion_minify );
				renderProgManager.SetRenderParm( RENDERPARM_VIEWDEPTHSTENCILMAP, renderDestManager.hiZbufferImage );
			}

			auto jitterTexScale = renderProgManager.GetRenderParm( RENDERPARM_JITTERTEXSCALE )->GetVecPtr();
			jitterTexScale[ 0 ] = i - 1;
			jitterTexScale[ 1 ] = 0;
			jitterTexScale[ 2 ] = 0;
			jitterTexScale[ 3 ] = 0;

			auto screenCorrectionParm = renderProgManager.GetRenderParm( RENDERPARM_SCREENCORRECTIONFACTOR )->GetVecPtr();
			screenCorrectionParm[ 0 ] = 1.0f / width;
			screenCorrectionParm[ 1 ] = 1.0f / height;
			screenCorrectionParm[ 2 ] = width;
			screenCorrectionParm[ 3 ] = height;

			GL_DrawUnitSquare();
		}
	}

	// set the window clipping
	GL_ViewportAndScissor( 0, 0, screenWidth, screenHeight );

	GL_State( GLS_DISABLE_BLENDING | GLS_DISABLE_DEPTHTEST | GLS_TWOSIDED );// GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );

	if( r_ssaoFiltering.GetBool() )
	{
		GL_SetRenderDestination( renderDestManager.renderDestAmbientOcclusion[ 0 ] );

		GL_Clear( true, false, false, 0, 0.0, 0.0, 0.0, 0.0 );

		GL_SetRenderProgram( renderProgManager.prog_AmbientOcclusion_AO );
	}
	else {
		RB_ResetBaseRenderDest( hdrIsActive );

		if( r_ssaoDebug.GetInteger() <= 0 )
		{
			GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_ALPHAMASK | GLS_DISABLE_DEPTHTEST | GLS_TWOSIDED );// GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
		}

		GL_SetRenderProgram( renderProgManager.prog_AmbientOcclusion_AO_write );
	}

	auto screenCorrectionParm = renderProgManager.GetRenderParm( RENDERPARM_SCREENCORRECTIONFACTOR )->GetVecPtr();
	screenCorrectionParm[ 0 ] = 1.0f / screenWidth;
	screenCorrectionParm[ 1 ] = 1.0f / screenHeight;
	screenCorrectionParm[ 2 ] = screenWidth;
	screenCorrectionParm[ 3 ] = screenHeight;

	auto jitterTexOffset = renderProgManager.GetRenderParm( RENDERPARM_JITTERTEXOFFSET )->GetVecPtr();
	if( r_shadowMapRandomizeJitter.GetBool() )
	{
		jitterTexOffset[ 0 ] = ( rand() & 255 ) / 255.0;
		jitterTexOffset[ 1 ] = ( rand() & 255 ) / 255.0;
	}
	else {
		jitterTexOffset[ 0 ] = 0;
		jitterTexOffset[ 1 ] = 0;
	}
	jitterTexOffset[ 2 ] = renderView->GetGameTimeSec();
	jitterTexOffset[ 3 ] = 0.0f;

	renderProgManager.SetRenderParm( RENDERPARM_VIEWNORMALMAP, renderDestManager.viewNormalImage );
	renderProgManager.SetRenderParm( RENDERPARM_VIEWDEPTHSTENCILMAP, r_useHierarchicalDepthBuffer.GetBool() ? renderDestManager.hiZbufferImage : renderDestDefault->GetDepthStencilImage() );

	//GL_DrawUnitSquare();
	GL_DrawScreenTriangleAuto();
	// --------------------------------------------------------------------

	if( r_ssaoFiltering.GetBool() )
	{
		idRenderVector jitterTexScale;

		// AO blur X
		GL_SetRenderDestination( renderDestManager.renderDestAmbientOcclusion[ 1 ] );

		GL_SetRenderProgram( renderProgManager.prog_AmbientOcclusion_blur );

		// set axis parameter
		jitterTexScale[ 0 ] = 1;
		jitterTexScale[ 1 ] = 0;
		jitterTexScale[ 2 ] = 0;
		jitterTexScale[ 3 ] = 0;
		renderProgManager.SetRenderParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale

		renderProgManager.SetRenderParm( RENDERPARM_MAP, renderDestManager.ambientOcclusionImage[ 0 ] );

		//GL_DrawUnitSquare();
		GL_DrawScreenTriangleAuto();
		// --------------------------------------------------------------------

		// AO blur Y
		RB_ResetBaseRenderDest( hdrIsActive );

		if( r_ssaoDebug.GetInteger() <= 0 )
		{
			GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_DISABLE_DEPTHTEST | GLS_TWOSIDED );// GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
		}

		GL_SetRenderProgram( renderProgManager.prog_AmbientOcclusion_blur_write );

		// set axis parameter
		jitterTexScale[ 0 ] = 0;
		jitterTexScale[ 1 ] = 1;
		jitterTexScale[ 2 ] = 0;
		jitterTexScale[ 3 ] = 0;
		renderProgManager.SetRenderParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale

		renderProgManager.SetRenderParm( RENDERPARM_MAP, renderDestManager.ambientOcclusionImage[ 1 ] );

		//GL_DrawUnitSquare();
		GL_DrawScreenTriangleAuto();
		// --------------------------------------------------------------------
	}

	renderProgManager.SetRenderParm( RENDERPARM_VIEWDEPTHSTENCILMAP, renderDestDefault->GetDepthStencilImage() );

	GL_ResetProgramState();
	GL_ResetTextureState();

	RENDERLOG_CLOSE_BLOCK();
}








struct renderSSAOParms_t
{
	const idRenderView *		renderView;
	uint32						renderWidth, renderHeight;

	const idRenderDestination * renderDestDefault;
	const idRenderDestination * renderDestCSDepth[ MAX_HIERARCHICAL_ZBUFFERS ];
	const idRenderDestination *	renderDestAmbientOcclusion[ MAX_SSAO_BUFFERS ];

	const idDeclRenderProg *	progAmbientOcclusionMinify;
	const idDeclRenderProg *	progAmbientOcclusionReconstructCSZ;
	const idDeclRenderProg *	progAmbientOcclusion;
	const idDeclRenderProg *	progAmbientOcclusionAndOutput;
	const idDeclRenderProg *	progAmbientOcclusionBlur;
	const idDeclRenderProg *	progAmbientOcclusionBlurAndOutput;

	const idDeclRenderParm *	rpViewDepthStencilMap;

	bool						useHierarchicalDepthBuffer;
};



void RenderSSAO( const renderSSAOParms_t * parms )
{
	RENDERLOG_OPEN_BLOCK( "RenderSSAO" );

#if 0
	GL_CheckErrors();

	// clear the alpha buffer and draw only the hands + weapon into it so
	// we can avoid blurring them
	glClearColor( 0, 0, 0, 1 );
	GL_State( GLS_COLORMASK | GLS_DEPTHMASK );
	glClear( GL_COLOR_BUFFER_BIT );
	renderProgManager.SetColorParm( 0, 0, 0, 0 );

	GL_SelectTexture( 0, renderImageManager->blackImage );

	backEnd.ClearCurrentSpace();

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
			renderProgManager.SetMVPMatrixParms( surf->space->mvp );
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

	renderProgManager.SetMVPMatrixParms( motionMatrix );
#endif

	auto const currentSpace = &parms->renderView->GetWorldSpace();
	renderProgManager.SetMVPMatrixParms( parms->renderView->GetMVPMatrix() );

	assert( parms->renderDestDefault != NUL );
	const bool hdrIsActive( r_useHDR.GetBool() && GL_IsBound( parms->renderDestDefault ) );

	int screenWidth = renderSystem->GetWidth();
	int screenHeight = renderSystem->GetHeight();

	// build hierarchical depth buffer
	if( parms->useHierarchicalDepthBuffer )
	{
		GL_SetRenderProgram( parms->progAmbientOcclusionMinify );

		GL_ResetTextureState();

		for( int i = 0; i < MAX_HIERARCHICAL_ZBUFFERS; i++ )
		{
			int width = parms->renderDestCSDepth[ i ]->GetTargetWidth();
			int height = parms->renderDestCSDepth[ i ]->GetTargetHeight();

			GL_SetRenderDestination( parms->renderDestCSDepth[ i ] );
			GL_ViewportAndScissor( 0, 0, width, height );
			GL_Clear( true, false, false, 0, 0.0, 0.0, 0.0, 1.0 );

			if( i == 0 )
			{
				GL_SetRenderProgram( parms->progAmbientOcclusionReconstructCSZ );
				renderProgManager.SetRenderParm( RENDERPARM_VIEWDEPTHSTENCILMAP, parms->renderDestDefault->GetDepthStencilImage() );
			}
			else {
				GL_SetRenderProgram( parms->progAmbientOcclusionMinify );
				renderProgManager.SetRenderParm( RENDERPARM_VIEWDEPTHSTENCILMAP, renderDestManager.hiZbufferImage );
				parms->rpViewDepthStencilMap->Set( renderDestManager.hiZbufferImage );
			}

			auto jitterTexScale = renderProgManager.GetRenderParm( RENDERPARM_JITTERTEXSCALE )->GetVecPtr(); // rpJitterTexScale;
			jitterTexScale[ 0 ] = i - 1;
			jitterTexScale[ 1 ] = 0;
			jitterTexScale[ 2 ] = 0;
			jitterTexScale[ 3 ] = 0;

			auto screenCorrectionParm = renderProgManager.GetRenderParm( RENDERPARM_SCREENCORRECTIONFACTOR )->GetVecPtr(); // rpScreenCorrectionFactor
			screenCorrectionParm[ 0 ] = 1.0f / width;
			screenCorrectionParm[ 1 ] = 1.0f / height;
			screenCorrectionParm[ 2 ] = width;
			screenCorrectionParm[ 3 ] = height;

			GL_DrawUnitSquare();
			//GL_DrawScreenTriangleAuto();
			// -----------------------------------------------------------------------
		}
	}

	// set the window clipping
	GL_ViewportAndScissor( 0, 0, screenWidth, screenHeight );

	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS | GLS_TWOSIDED );
	//GL_Cull( CT_TWO_SIDED );

	if( r_ssaoFiltering.GetBool() )
	{
		GL_SetRenderDestination( parms->renderDestAmbientOcclusion[ 0 ] );

		GL_Clear( true, false, false, 0, 0.0, 0.0, 0.0, 0.0 );

		GL_SetRenderProgram( parms->progAmbientOcclusion );
	}
	else {
		if( r_ssaoDebug.GetInteger() <= 0 )
		{
			GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_ALPHAMASK | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
		}

		RB_ResetBaseRenderDest( hdrIsActive );

		GL_SetRenderProgram( parms->progAmbientOcclusionAndOutput );
	}

	auto screenCorrectionParm = renderProgManager.GetRenderParm( RENDERPARM_SCREENCORRECTIONFACTOR )->GetVecPtr(); // rpScreenCorrectionFactor;
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
	///renderProgManager.SetRenderParms( RENDERPARM_MODELMATRIX_X, parms->renderView->GetInverseProjMatrix().Ptr(), 4 );

	auto jitterTexOffset = renderProgManager.GetRenderParm( RENDERPARM_JITTERTEXOFFSET )->GetVecPtr(); // rpJitterTexOffset;
	if( r_shadowMapRandomizeJitter.GetBool() )
	{
		jitterTexOffset[ 0 ] = ( rand() & 255 ) / 255.0;
		jitterTexOffset[ 1 ] = ( rand() & 255 ) / 255.0;
	}
	else {
		jitterTexOffset[ 0 ] = 0;
		jitterTexOffset[ 1 ] = 0;
	}
	jitterTexOffset[ 2 ] = parms->renderView->GetGameTimeSec();
	jitterTexOffset[ 3 ] = 0.0f;

	renderProgManager.SetRenderParm( RENDERPARM_VIEWNORMALMAP, renderDestManager.viewNormalImage );
	renderProgManager.SetRenderParm( RENDERPARM_VIEWDEPTHSTENCILMAP,
		r_useHierarchicalDepthBuffer.GetBool() ? renderDestManager.hiZbufferImage : renderDestManager.viewDepthStencilImage );

	//GL_DrawUnitSquare();
	GL_DrawScreenTriangleAuto();
	// --------------------------------------------------------------------

	if( r_ssaoFiltering.GetBool() )
	{
		idRenderVector jitterTexScale;

		// AO blur X
		GL_SetRenderDestination( parms->renderDestAmbientOcclusion[ 1 ] );

		GL_SetRenderProgram( parms->progAmbientOcclusionBlur );

		// set axis parameter
		jitterTexScale[ 0 ] = 1;
		jitterTexScale[ 1 ] = 0;
		jitterTexScale[ 2 ] = 0;
		jitterTexScale[ 3 ] = 0;
		renderProgManager.SetRenderParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale

		renderProgManager.SetRenderParm( RENDERPARM_MAP, renderDestManager.ambientOcclusionImage[ 0 ] );

		//GL_DrawUnitSquare();
		GL_DrawScreenTriangleAuto();
		// --------------------------------------------------------------------

		// AO blur Y
		RB_ResetBaseRenderDest( hdrIsActive );

		if( r_ssaoDebug.GetInteger() <= 0 )
		{
			GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
		}

		GL_SetRenderProgram( renderProgManager.prog_AmbientOcclusion_blur_write );

		// set axis parameter
		jitterTexScale[ 0 ] = 0;
		jitterTexScale[ 1 ] = 1;
		jitterTexScale[ 2 ] = 0;
		jitterTexScale[ 3 ] = 0;
		renderProgManager.SetRenderParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale

		renderProgManager.SetRenderParm( RENDERPARM_MAP, renderDestManager.ambientOcclusionImage[ 1 ] );

		//GL_DrawUnitSquare();
		GL_DrawScreenTriangleAuto();
		// --------------------------------------------------------------------
	}

	renderProgManager.SetRenderParm( RENDERPARM_VIEWDEPTHSTENCILMAP, renderDestManager.viewDepthStencilImage );

	GL_ResetProgramState();
	GL_ResetTextureState();

	GL_State( GLS_DEFAULT | GLS_FRONTSIDED );
	//GL_Cull( CT_FRONT_SIDED );

	RENDERLOG_CLOSE_BLOCK();
}
