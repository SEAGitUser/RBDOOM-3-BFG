
#include "precompiled.h"
#pragma hdrstop

#include "Render.h"

/*
=========================================================================================================

	SCREEN SPACE GLOBAL ILLUMINATION RENDER PASSES

=========================================================================================================
*/

idCVar r_useSSGI( "r_useSSGI", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "use screen space global illumination and reflections" );
idCVar r_ssgiDebug( "r_ssgiDebug", "0", CVAR_RENDERER | CVAR_INTEGER, "" );
idCVar r_ssgiFiltering( "r_ssgiFiltering", "1", CVAR_RENDERER | CVAR_BOOL, "" );

extern idCVar r_useHierarchicalDepthBuffer;

void RB_SSGI()
{
	RENDERLOG_OPEN_BLOCK( "RB_SSGI" );

	auto const renderView = backEnd.viewDef;
	auto const renderDestDefault = renderDestManager.renderDestBaseHDR;

	backEnd.currentSpace = &renderView->GetWorldSpace();
	renderProgManager.SetMVPMatrixParms( renderView->GetMVPMatrix() );

	assert( renderDestDefault != NULL );
	const bool hdrIsActive = ( r_useHDR.GetBool() && GL_IsBound( renderDestDefault ) );

	int screenWidth = renderSystem->GetWidth();
	int screenHeight = renderSystem->GetHeight();

	// set the window clipping
	GL_ViewportAndScissor( 0, 0, screenWidth, screenHeight );

	if( !hdrIsActive )
	{
		auto & viewport = renderView->GetViewport();
		GL_CopyCurrentColorToTexture( renderImageManager->currentRenderImage, viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );
	}

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
	//GL_Cull( CT_TWO_SIDED );

	if( r_ssgiFiltering.GetBool() )
	{
		GL_SetRenderDestination( renderDestManager.renderDestAmbientOcclusion[ 0 ] );

		// FIXME remove and mix with color from previous frame
		GL_Clear( true, false, false, 0, 0.0, 0.0, 0.0, 0.0 );

		GL_SetRenderProgram( renderProgManager.prog_DeepGBufferRadiosity_radiosity );
	}
	else {
		RB_ResetBaseRenderDest( hdrIsActive );

		if( r_ssgiDebug.GetInteger() > 0 )
		{
			// replace current
			GL_State( GLS_DISABLE_BLENDING | GLS_DISABLE_DEPTHTEST );// GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
		}
		else {
			// add result to main color
			GL_State( GLS_BLEND_ADD | GLS_DISABLE_DEPTHTEST );// GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
		}

		GL_SetRenderProgram( renderProgManager.prog_DeepGBufferRadiosity_radiosity );
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
	renderProgManager.SetRenderParm( RENDERPARM_MAP, hdrIsActive ? renderDestManager.viewColorHDRImage : renderImageManager->currentRenderImage );

	///GL_DrawUnitSquare();
	GL_DrawScreenTriangleAuto();

	if( r_ssgiFiltering.GetBool() )
	{
		idRenderVector jitterTexScale;

		// AO blur X

		GL_SetRenderDestination( renderDestManager.renderDestAmbientOcclusion[ 1 ] );

		GL_SetRenderProgram( renderProgManager.prog_DeepGBufferRadiosity_blur );

		// set axis parameter
		jitterTexScale[ 0 ] = 1;
		jitterTexScale[ 1 ] = 0;
		jitterTexScale[ 2 ] = 0;
		jitterTexScale[ 3 ] = 0;
		renderProgManager.SetRenderParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale

		renderProgManager.SetRenderParm( RENDERPARM_MAP, renderDestManager.ambientOcclusionImage[ 0 ] );

		///GL_DrawUnitSquare();
		GL_DrawScreenTriangleAuto();

		// AO blur Y

		RB_ResetBaseRenderDest( hdrIsActive );

		if( r_ssgiDebug.GetInteger() > 0 )
		{
			// replace current
			GL_State( GLS_DISABLE_BLENDING | GLS_DISABLE_DEPTHTEST );// GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
		}
		else {
			// add result to main color
			GL_State( GLS_BLEND_ADD | GLS_DISABLE_DEPTHTEST );// GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
		}

		GL_SetRenderProgram( renderProgManager.prog_DeepGBufferRadiosity_blur_write );

		// set axis parameter
		jitterTexScale[ 0 ] = 0;
		jitterTexScale[ 1 ] = 1;
		jitterTexScale[ 2 ] = 0;
		jitterTexScale[ 3 ] = 0;
		renderProgManager.SetRenderParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale

		renderProgManager.SetRenderParm( RENDERPARM_MAP, renderDestManager.ambientOcclusionImage[ 1 ] );

		///GL_DrawUnitSquare();
		GL_DrawScreenTriangleAuto();
	}

	GL_ResetProgramState();
	GL_ResetTextureState();

	GL_State( GLS_DEFAULT | GLS_FRONTSIDED );
	//GL_Cull( CT_FRONT_SIDED );

	RENDERLOG_CLOSE_BLOCK();
}
