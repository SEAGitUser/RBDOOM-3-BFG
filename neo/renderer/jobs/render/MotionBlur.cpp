
#include "precompiled.h"
#pragma hdrstop

#include "Render.h"

/*
=========================================================================================================

	MOTION BLUR RENDER PASSES

=========================================================================================================
*/

idCVar r_motionBlur( "r_motionBlur", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "1 - 5, log2 of the number of motion blur samples" );
idCVar r_useCopyColorProg( "r_useCopyColorProg", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "use shaders to copy texture" );

void RB_MotionBlur()
{
	RENDERLOG_OPEN_BLOCK( "RB_MotionBlur" );

	RB_ResetBaseRenderDest( r_useHDR.GetBool() );
	RB_ResetViewportAndScissorToDefaultCamera( backEnd.viewDef );

	// clear the alpha buffer and draw only the hands + weapon into it so
	// we can avoid blurring them
	GL_State( GLS_COLORMASK | GLS_DEPTHMASK | GLS_STENCILMASK );
	GL_Clear( true, false, false, 0, 0.0, 0.0, 0.0, 1.0 );

	// skinned color-only prog
	GL_SetRenderProgram( renderProgManager.prog_motionBlurMask );

	backEnd.ClearCurrentSpace();
	for( int surfNum = 0; surfNum < backEnd.viewDef->numDrawSurfs; ++surfNum )
	{
		const auto * const surf = backEnd.viewDef->drawSurfs[ surfNum ];

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
			renderProgManager.SetMVPMatrixParms( surf->space->mvp );
			backEnd.currentSpace = surf->space;
		}

		// draw it solid
		GL_DrawIndexed( surf );
	}

	//GL_State( GLS_DEPTHFUNC_ALWAYS );

	// copy off the color buffer and the depth buffer for the motion blur prog
	// we use the viewport dimensions for copying the buffers in case resolution scaling is enabled.
	auto & viewport = backEnd.viewDef->GetViewport();
	if( !r_useCopyColorProg.GetBool() )
	{
		//GL_CopyCurrentColorToTexture( renderImageManager->currentRenderImage, viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );
		GL_BlitRenderBuffer(
			GL_GetCurrentRenderDestination(),
			viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight(),
			renderDestManager.renderDestStagingColor,
			0, 0, viewport.GetWidth(), viewport.GetHeight(),
			true, false );
	}
	else {
		GL_SetRenderDestination( renderDestManager.renderDestStagingColor );
		//GL_ViewportAndScissor( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );
		GL_State( GLS_DISABLE_BLENDING | GLS_DISABLE_DEPTHTEST | GLS_TWOSIDED );
		GL_SetRenderProgram( renderProgManager.prog_copyColor );
		renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderDestManager.viewColorHDRImage );
		GL_DrawScreenTriangleAuto();

		RB_ResetBaseRenderDest( r_useHDR.GetBool() );
	}

	// in stereo rendering, each eye needs to get a separate previous frame mvp
	const int mvpIndex = ( backEnd.viewDef->GetStereoEye() == 1 )? 1 : 0;

	// derive the matrix to go from current pixels to previous frame pixels
	idRenderMatrix::Multiply( backEnd.prevMVP[ mvpIndex ], backEnd.viewDef->GetInverseVPMatrix(),
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_X )->GetVec4(),
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_Y )->GetVec4(),
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_Z )->GetVec4(),
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_W )->GetVec4() );

	backEnd.prevMVP[ mvpIndex ] = backEnd.viewDef->GetMVPMatrix();

	GL_State( GLS_DISABLE_DEPTHTEST | GLS_TWOSIDED );

	GL_SetRenderProgram( renderProgManager.prog_motionBlur );

	// let the fragment program know how many samples we are going to use
	idRenderVector samples( ( float )( 1 << r_motionBlur.GetInteger() ) );
	renderProgManager.SetRenderParm( RENDERPARM_USERVEC0, samples );

	renderProgManager.SetRenderParm( RENDERPARM_CURRENTRENDERMAP, renderImageManager->currentRenderImage );
	renderProgManager.SetRenderParm( RENDERPARM_VIEWDEPTHSTENCILMAP, renderDestManager.viewDepthStencilImage );

	///GL_DrawUnitSquare();
	GL_DrawScreenTriangleAuto();

	RENDERLOG_CLOSE_BLOCK();
}




void RB_MotionBlur2()
{
	RENDERLOG_OPEN_BLOCK( "RB_MotionBlur2" );

	RB_ResetBaseRenderDest( r_useHDR.GetBool() );
	RB_ResetViewportAndScissorToDefaultCamera( backEnd.viewDef );

	// clear the alpha buffer and draw only the hands + weapon into it so
	// we can avoid blurring them
	GL_State( GLS_DISABLE_BLENDING | GLS_COLORMASK | GLS_ALPHAMASK |
		GLS_DEPTHMASK |
		GLS_STENCIL_OP_PASS_REPLACE | GLS_STENCIL_MAKE_REF( 1 ) | GLS_STENCIL_MAKE_MASK( 255 ) );
	//GL_Clear( true, false, false, 0, 0.0, 0.0, 0.0, 1.0 );

	// skinned color-only prog
	GL_SetRenderProgram( renderProgManager.prog_motionBlurMask );

	backEnd.ClearCurrentSpace();
	for( int surfNum = 0; surfNum < backEnd.viewDef->numDrawSurfs; ++surfNum )
	{
		const auto * const surf = backEnd.viewDef->drawSurfs[ surfNum ];

		if( !surf->space->weaponDepthHack && !surf->space->skipMotionBlur && !surf->material->HasSubview() )
		{ // Apply motion blur to this object
			continue;
		}
		if( surf->material->Coverage() == MC_TRANSLUCENT )
		{ // muzzle flash, etc
			continue;
		}

		// set mvp matrix
		if( surf->space != backEnd.currentSpace )
		{
			renderProgManager.SetMVPMatrixParms( surf->space->mvp );
			backEnd.currentSpace = surf->space;
		}

		// draw it solid
		GL_DrawIndexed( surf );
	}

	GL_State( GLS_DEPTHFUNC_ALWAYS );

	// copy off the color buffer and the depth buffer for the motion blur prog
	// we use the viewport dimensions for copying the buffers in case resolution scaling is enabled.
	auto & viewport = backEnd.viewDef->GetViewport();
	GL_CopyCurrentColorToTexture( renderImageManager->currentRenderImage, viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );


	// in stereo rendering, each eye needs to get a separate previous frame mvp
	const int mvpIndex = ( backEnd.viewDef->GetStereoEye() == 1 ) ? 1 : 0;

	// derive the matrix to go from current pixels to previous frame pixels
	idRenderMatrix motionMatrix;
	idRenderMatrix::Multiply( backEnd.prevMVP[ mvpIndex ], backEnd.viewDef->GetInverseVPMatrix(), motionMatrix );

	backEnd.prevMVP[ mvpIndex ] = backEnd.viewDef->GetMVPMatrix();

	renderProgManager.SetMVPMatrixParms( motionMatrix );

	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_TWOSIDED );

	GL_SetRenderProgram( renderProgManager.prog_motionBlur );

	// let the fragment program know how many samples we are going to use
	idRenderVector samples( ( float )( 1 << r_motionBlur.GetInteger() ) );
	renderProgManager.SetRenderParm( RENDERPARM_USERVEC0, samples );

	renderProgManager.SetRenderParm( RENDERPARM_CURRENTRENDERMAP, renderImageManager->currentRenderImage );
	renderProgManager.SetRenderParm( RENDERPARM_VIEWDEPTHSTENCILMAP, renderDestManager.viewDepthStencilImage );

	///GL_DrawUnitSquare();
	GL_DrawScreenTriangleAuto();

	RENDERLOG_CLOSE_BLOCK();
}

struct renderGlobalMotionBlurParms_t
{
	const idRenderView *		renderView;
	idImage *					blackImage;
	idImage *					currentRenderImage;
	idImage *					viewDepthStencilImage;
	uint32						numDrawSurfs;
	drawSurf_t **				drawSurfs;
	idRenderMatrix *			prevMVP;
	uint32						sampleCount;
	const idRenderDestination *	renderDestDefault;
	const idDeclRenderProg *	progBlurMask; // TextureVertexColor
	const idDeclRenderProg *	progBlurMaskSkinned; // TextureVertexColor
	const idDeclRenderProg *	progGlobalMotionBlur;
};

void RenderGlobalMotionBlur( const renderGlobalMotionBlurParms_t * parms )
{
	RENDERLOG_OPEN_BLOCK( "RenderGlobalMotionBlur" );

	// clear the alpha buffer and draw only the hands + weapon into it so
	// we can avoid blurring them
	GL_State( GLS_COLORMASK | GLS_DEPTHMASK | GLS_STENCILMASK );
	GL_Clear( true, false, false, 0, 0.0, 0.0, 0.0, 1.0 );

	renderProgManager.SetColorParm( 0, 0, 0, 0 );
	renderProgManager.SetRenderParm( RENDERPARM_MAP, parms->blackImage );

	const viewModel_t * currentSpace = nullptr;
	for( int surfNum = 0; surfNum < parms->numDrawSurfs; ++surfNum )
	{
		const auto * const surf = parms->drawSurfs[ surfNum ];

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
		if( surf->space != currentSpace )
		{
			renderProgManager.SetMVPMatrixParms( surf->space->mvp );
			currentSpace = surf->space;
		}

		// this could just be a color, but we don't have a skinned color-only prog
		GL_SetRenderProgram( surf->HasSkinning() ? parms->progBlurMaskSkinned : parms->progBlurMask );

		// draw it solid
		GL_DrawIndexed( surf );
	}

	GL_State( GLS_DEPTHFUNC_ALWAYS );

	// copy off the color buffer and the depth buffer for the motion blur prog
	// we use the viewport dimensions for copying the buffers in case resolution scaling is enabled.
	auto & viewport = parms->renderView->GetViewport();
	GL_CopyCurrentColorToTexture( parms->currentRenderImage, viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );

	// in stereo rendering, each eye needs to get a separate previous frame mvp
	const int mvpIndex = ( parms->renderView->GetStereoEye() == 1 ) ? 1 : 0;

	// derive the matrix to go from current pixels to previous frame pixels
	idRenderMatrix motionMatrix;
	idRenderMatrix::Multiply( parms->prevMVP[ mvpIndex ], parms->renderView->GetInverseVPMatrix(), motionMatrix );

	parms->prevMVP[ mvpIndex ] = parms->renderView->GetMVPMatrix();

	renderProgManager.SetMVPMatrixParms( motionMatrix );

	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_TWOSIDED );
	//GL_Cull( CT_TWO_SIDED );

	GL_SetRenderProgram( parms->progGlobalMotionBlur );

	// let the fragment program know how many samples we are going to use
	idRenderVector samples( ( float )( 1U << parms->sampleCount ) );
	renderProgManager.SetRenderParm( RENDERPARM_USERVEC0, samples );

	renderProgManager.SetRenderParm( RENDERPARM_CURRENTRENDERMAP, parms->currentRenderImage );
	renderProgManager.SetRenderParm( RENDERPARM_VIEWDEPTHSTENCILMAP, parms->viewDepthStencilImage );

	///GL_DrawUnitSquare();
	GL_DrawScreenTriangleAuto();

	RENDERLOG_CLOSE_BLOCK();
}