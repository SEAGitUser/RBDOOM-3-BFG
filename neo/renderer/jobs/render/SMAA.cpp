
#include "precompiled.h"
#pragma hdrstop

#include "Render.h"

/*
==============================================================================================

	SMAA RENDERPASS

==============================================================================================
*/
#if 0
idCVar r_smaa_usePredication( "r_smaa_usePredication", "0", CVAR_RENDERER | CVAR_BOOL, "Enable predicated thresholding" );
idCVar r_smaa_mode( "r_smaa_mode", "0", CVAR_RENDERER | CVAR_INTEGER, "SMAA mode preset 0-SMAA_1X, 1-SMAA_T2X, 2-SMAA_S2X, 3-SMAA_4X", 0.0, 3.0 );
idCVar r_smaa_preset( "r_smaa_preset", "0", CVAR_RENDERER | CVAR_INTEGER, "SMAA quality preset 0-LOW, 1-MEDIUM, 2-HIGH, 3-ULTRA, 4-CUSTOM", 0.0, 4.0 );
idCVar r_smaa_input( "r_smaa_input", "0", CVAR_RENDERER | CVAR_INTEGER, "SMAA edge detection input type 0-LUMA, 1-COLOR, 2-DEPTH", 0.0, 2.0 );
idCVar r_smaa_useReprojection( "r_smaa_useReprojection", "0", CVAR_RENDERER | CVAR_BOOL, "Enable temporal supersampling" );
idCVar r_smaa_threshold( "r_smaa_threshold", "0.1", CVAR_RENDERER | CVAR_FLOAT, "Threshold for the edge detection. Only has effect if PRESET_CUSTOM is selected" );
idCVar r_smaa_cornerRounding( "r_smaa_cornerRounding", "0.25", CVAR_RENDERER | CVAR_FLOAT, "Desired corner rounding, from 0.0 (no rounding) to 100.0 (full rounding). Only has effect if PRESET_CUSTOM is selected" );
idCVar r_smaa_maxSearchSteps( "r_smaa_maxSearchSteps", "16", CVAR_RENDERER | CVAR_INTEGER, "Maximum length to search for horizontal/vertical patterns. Each step is two pixels wide. Only has effect if PRESET_CUSTOM is selected" );
idCVar r_smaa_maxSearchStepsDiag( "r_smaa_maxSearchStepsDiag", "8", CVAR_RENDERER | CVAR_INTEGER, "Maximum length to search for diagonal patterns. Only has effect if PRESET_CUSTOM is selected" );

namespace SMAA
{
	enum Mode_e {
		MODE_SMAA_1X, MODE_SMAA_T2X, MODE_SMAA_S2X, MODE_SMAA_4X, MODE_SMAA_COUNT
	};
	enum Preset_e {
		PRESET_LOW, PRESET_MEDIUM, PRESET_HIGH, PRESET_ULTRA, PRESET_CUSTOM, PRESET_COUNT
	};
	enum Input_e {
		INPUT_LUMA, INPUT_COLOR, INPUT_DEPTH, INPUT_COUNT
	};

	static uint frameIndex = 0;
	void NextFrame()
	{
		frameIndex = ( frameIndex + 1U ) % 2U;
	}

	static const idDeclRenderProg *	progEdgeDetect[ PRESET_COUNT ][ INPUT_COUNT ][ 2 ] = { nullptr };
	static const idDeclRenderProg *	progBlendingWeightCalculation[ PRESET_COUNT ] = { nullptr };
	static const idDeclRenderProg *	progNeighborhoodBlending[ 2 ] = { nullptr };
	static const idDeclRenderProg *	progResolve[ 2 ] = { nullptr };
	static const idDeclRenderProg *	progSeparate[ 2 ] = { nullptr };

	static float blendFactorVariable = 0.0f;

	void Init()
	{
		progEdgeDetect[ PRESET_LOW ][ INPUT_LUMA ][ 0 ]  = declManager->FindRenderProg( "SMAA_EdgeDetection_low_luma" );
		progEdgeDetect[ PRESET_LOW ][ INPUT_LUMA ][ 1 ]  = declManager->FindRenderProg( "SMAA_EdgeDetection_low_luma_predicate" );
		progEdgeDetect[ PRESET_LOW ][ INPUT_COLOR ][ 0 ] = declManager->FindRenderProg( "SMAA_EdgeDetection_low_color" );
		progEdgeDetect[ PRESET_LOW ][ INPUT_COLOR ][ 1 ] = declManager->FindRenderProg( "SMAA_EdgeDetection_low_color_predicate" );
		progEdgeDetect[ PRESET_LOW ][ INPUT_DEPTH ][ 0 ] = declManager->FindRenderProg( "SMAA_EdgeDetection_low_depth" );

		progEdgeDetect[ PRESET_MEDIUM ][ INPUT_LUMA ][ 0 ]  = declManager->FindRenderProg( "SMAA_EdgeDetection_medium_luma" );
		progEdgeDetect[ PRESET_MEDIUM ][ INPUT_LUMA ][ 1 ]  = declManager->FindRenderProg( "SMAA_EdgeDetection_medium_luma_predicate" );
		progEdgeDetect[ PRESET_MEDIUM ][ INPUT_COLOR ][ 0 ] = declManager->FindRenderProg( "SMAA_EdgeDetection_medium_color" );
		progEdgeDetect[ PRESET_MEDIUM ][ INPUT_COLOR ][ 1 ] = declManager->FindRenderProg( "SMAA_EdgeDetection_medium_color_predicate" );
		progEdgeDetect[ PRESET_MEDIUM ][ INPUT_DEPTH ][ 0 ] = declManager->FindRenderProg( "SMAA_EdgeDetection_medium_depth" );

		progEdgeDetect[ PRESET_HIGH ][ INPUT_LUMA ][ 0 ]  = declManager->FindRenderProg( "SMAA_EdgeDetection_high_luma" );
		progEdgeDetect[ PRESET_HIGH ][ INPUT_LUMA ][ 1 ]  = declManager->FindRenderProg( "SMAA_EdgeDetection_high_luma_predicate" );
		progEdgeDetect[ PRESET_HIGH ][ INPUT_COLOR ][ 0 ] = declManager->FindRenderProg( "SMAA_EdgeDetection_high_color" );
		progEdgeDetect[ PRESET_HIGH ][ INPUT_COLOR ][ 1 ] = declManager->FindRenderProg( "SMAA_EdgeDetection_high_color_predicate" );
		progEdgeDetect[ PRESET_HIGH ][ INPUT_DEPTH ][ 0 ] = declManager->FindRenderProg( "SMAA_EdgeDetection_high_depth" );

		progEdgeDetect[ PRESET_ULTRA ][ INPUT_LUMA ][ 0 ]  = declManager->FindRenderProg( "SMAA_EdgeDetection_ultra_luma" );
		progEdgeDetect[ PRESET_ULTRA ][ INPUT_LUMA ][ 1 ]  = declManager->FindRenderProg( "SMAA_EdgeDetection_ultra_luma_predicate" );
		progEdgeDetect[ PRESET_ULTRA ][ INPUT_COLOR ][ 0 ] = declManager->FindRenderProg( "SMAA_EdgeDetection_ultra_color" );
		progEdgeDetect[ PRESET_ULTRA ][ INPUT_COLOR ][ 1 ] = declManager->FindRenderProg( "SMAA_EdgeDetection_ultra_color_predicate" );
		progEdgeDetect[ PRESET_ULTRA ][ INPUT_DEPTH ][ 0 ] = declManager->FindRenderProg( "SMAA_EdgeDetection_ultra_depth" );

		progEdgeDetect[ PRESET_CUSTOM ][ INPUT_LUMA ][ 0 ]  = declManager->FindRenderProg( "SMAA_EdgeDetection_custom_luma" );
		progEdgeDetect[ PRESET_CUSTOM ][ INPUT_LUMA ][ 1 ]  = declManager->FindRenderProg( "SMAA_EdgeDetection_custom_luma_predicate" );
		progEdgeDetect[ PRESET_CUSTOM ][ INPUT_COLOR ][ 0 ] = declManager->FindRenderProg( "SMAA_EdgeDetection_custom_color" );
		progEdgeDetect[ PRESET_CUSTOM ][ INPUT_COLOR ][ 1 ] = declManager->FindRenderProg( "SMAA_EdgeDetection_custom_color_predicate" );
		progEdgeDetect[ PRESET_CUSTOM ][ INPUT_DEPTH ][ 0 ] = declManager->FindRenderProg( "SMAA_EdgeDetection_custom_depth" );

		progBlendingWeightCalculation[ PRESET_LOW ]    = declManager->FindRenderProg( "SMAA_BlendingWeightCalculation_low" );
		progBlendingWeightCalculation[ PRESET_MEDIUM ] = declManager->FindRenderProg( "SMAA_BlendingWeightCalculation_medium" );
		progBlendingWeightCalculation[ PRESET_HIGH ]   = declManager->FindRenderProg( "SMAA_BlendingWeightCalculation_high" );
		progBlendingWeightCalculation[ PRESET_ULTRA ]  = declManager->FindRenderProg( "SMAA_BlendingWeightCalculation_ultra" );
		progBlendingWeightCalculation[ PRESET_CUSTOM ] = declManager->FindRenderProg( "SMAA_BlendingWeightCalculation_custom" );

		progNeighborhoodBlending[ 0 ] = declManager->FindRenderProg( "SMAA_NeighborhoodBlending" );
		progNeighborhoodBlending[ 1 ] = declManager->FindRenderProg( "SMAA_NeighborhoodBlending_reproject" );

		progResolve[ 0 ] = declManager->FindRenderProg( "SMAA_Resolve" );
		progSeparate[ 1 ] = declManager->FindRenderProg( "SMAA_Resolve_reproject" );
	}

	idVec2 GetJitter( Mode_e mode )
	{
		switch( mode )
		{
			case MODE_SMAA_1X:
			case MODE_SMAA_S2X:
				return idVec2( 0.0f, 0.0f );

			case MODE_SMAA_T2X:
			{
				idVec2 jitters[] = {
					idVec2(-0.25f,  0.25f ),
					idVec2( 0.25f, -0.25f )
				};
				return jitters[ frameIndex ];
			}

			case MODE_SMAA_4X:
			{
				idVec2 jitters[] = {
					idVec2(-0.125f, -0.125f ),
					idVec2( 0.125f,  0.125f )
				};
				return jitters[ frameIndex ];
			}

			default:
				//throw logic_error( "unexpected problem" );
		}
	}

	int msaaReorder( int pass );

	int GetSubsampleIndex( Mode_e mode, int pass )
	{
		switch( mode )
		{
			case MODE_SMAA_1X: return 0;
			case MODE_SMAA_T2X: return frameIndex;
			case MODE_SMAA_S2X: return msaaReorder( pass );
			case MODE_SMAA_4X: return 2 * frameIndex + msaaReorder( pass );

			default:
				//throw logic_error( "unexpected problem" );
		}
	}

};

void DoSMAA(
	//idImage * srcGammaSRV,	// Non-SRGB version of the input color texture.
	//idImage * srcSRV, // SRGB version of the input color texture.
	//idImage * depthSRV, // Input depth texture.
	//idImage * velocitySRV, // Input velocity texture, if reproject is going to be called later on, nullptr otherwise.
	//idImage * dsv, // Depth-stencil buffer for optimizations.
	const idRenderDestination * dstRTV, // Output render target.
	SMAA::Input_e input, // Selects the input for edge detection.
	SMAA::Mode_e mode, // Selects the SMAA mode.
	int pass = -1 ) // Selects the S2x or 4x pass (either 0 or 1).
{
	auto preset = ( SMAA::Preset_e ) r_smaa_preset.GetInteger();

	// Get the subsample index:
	int subsampleIndex = SMAA::GetSubsampleIndex( mode, pass );

	// Setup variables:
	if( preset == SMAA::PRESET_CUSTOM )
	{
		renderProgManager.GetRenderParm( RENDERPARM_USERVEC2 )->Set(
			r_smaa_threshold.GetFloat(),
			r_smaa_cornerRounding.GetFloat(),
			float( r_smaa_maxSearchSteps.GetInteger() ),
			float( r_smaa_maxSearchStepsDiag.GetInteger() ) );
	}

	if( pass >= 0 ) SMAA::blendFactorVariable = ( pass == 0 )? 1.0f : 0.5f;

	renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderImageManager->smaaAreaImage );
	renderProgManager.SetRenderParm( RENDERPARM_USERMAP1, renderImageManager->smaaSearchImage );

	renderProgManager.SetRenderParm( RENDERPARM_USERMAP2, renderImageManager->smaaEdgesImage );
	renderProgManager.SetRenderParm( RENDERPARM_USERMAP3, renderImageManager->smaaBlendImage );

	renderProgManager.SetRenderParm( RENDERPARM_USERMAP4, renderImageManager->smaaInputImage );
	renderProgManager.SetRenderParm( RENDERPARM_USERMAP5, renderImageManager->smaaInputImage );
	//renderProgManager.SetRenderParm( RENDERPARM_USERMAP6, renderImageManager->smaaInputPreviousImage );
	//renderProgManager.SetRenderParm( RENDERPARM_USERMAP7, renderImageManager->smaaInputMSImage );
	renderProgManager.SetRenderParm( RENDERPARM_USERMAP8, renderImageManager->viewDepthStencilImage );
	//renderProgManager.SetRenderParm( RENDERPARM_USERMAP9, renderImageManager->velocityImage );

	{
		RENDERLOG_OPEN_BLOCK( "Edge Detection Pass" );

		GL_SetRenderDestination( renderDestManager.renderDestSMAAEdges );
		GL_ClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

		GL_State( GLS_DISABLE_BLENDING | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS |
			GLS_STENCIL_OP_PASS_REPLACE | GLS_STENCIL_MAKE_REF( 1 ) | GLS_STENCIL_MAKE_MASK( 255 ) );

		renderProgManager.BindRenderProgram( SMAA::progEdgeDetect[ preset ][ input ][ r_smaa_usePredication.GetBool() ] );

		renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderImageManager->smaaInputImage );
		if( r_smaa_usePredication.GetBool() ) {
			renderProgManager.SetRenderParm( RENDERPARM_USERMAP1, renderImageManager->viewDepthStencilImage );
		}
		GL_DrawScreenTriangleAuto();

		// IMPORTANT: The stencil component of 'dsv' is used to mask zones to
		// be processed.It is assumed to be already cleared to zero when this
		// function is called.It is not done here because it is usually
		// cleared together with the depth.

		RENDERLOG_CLOSE_BLOCK();
	}

	{
		RENDERLOG_OPEN_BLOCK( "Blending Weights Calculation Pass" );
		/**
		* Orthogonal indices:
		*     [0]:  0.0
		*     [1]: -0.25
		*     [2]:  0.25
		*     [3]: -0.125
		*     [4]:  0.125
		*     [5]: -0.375
		*     [6]:  0.375
		*
		* Diagonal indices:
		*     [0]:  0.00,   0.00
		*     [1]:  0.25,  -0.25
		*     [2]: -0.25,   0.25
		*     [3]:  0.125, -0.125
		*     [4]: -0.125,  0.125
		*
		* Indices layout: indices[4] = { |, --,  /, \ }
		*/
		switch( mode )
		{
			case SMAA::MODE_SMAA_1X:
			{
				idRenderVector indices( 0.0f, 0.0f, 0.0f, 0.0f );
				renderProgManager.GetRenderParm( RENDERPARM_USERVEC1 )->Set( indices );
				// subsampleIndicesVariable
				break;
			}
			case SMAA::MODE_SMAA_T2X:
			case SMAA::MODE_SMAA_S2X:
			{
				/***
				* Sample positions (bottom-to-top y axis):
				*   _______
				*  | S1    |  S0:  0.25    -0.25
				*  |       |  S1: -0.25     0.25
				*  |____S0_|
				*/
				ALIGNTYPE16 float indices[][ 4 ] = {
					{ 1.0f, 1.0f, 1.0f, 0.0f }, // S0
					{ 2.0f, 2.0f, 2.0f, 0.0f }  // S1
												// (it's 1 for the horizontal slot of S0 because horizontal
												//  blending is reversed: positive numbers point to the right)
				};
				renderProgManager.GetRenderParm( RENDERPARM_USERVEC1 )->Set( indices[ subsampleIndex ] );
				// subsampleIndicesVariable
				break;
			}
			case SMAA::MODE_SMAA_4X:
			{
				/***
				* Sample positions (bottom-to-top y axis):
				*   ________
				*  |  S1    |  S0:  0.3750   -0.1250
				*  |      S0|  S1: -0.1250    0.3750
				*  |S3      |  S2:  0.1250   -0.3750
				*  |____S2__|  S3: -0.3750    0.1250
				*/
				ALIGNTYPE16 float indices[][ 4 ] = {
					{ 5.0f, 3.0f, 1.0f, 3.0f }, // S0
					{ 4.0f, 6.0f, 2.0f, 3.0f }, // S1
					{ 3.0f, 5.0f, 1.0f, 4.0f }, // S2
					{ 6.0f, 4.0f, 2.0f, 4.0f }  // S3
				};
				renderProgManager.GetRenderParm( RENDERPARM_USERVEC1 )->Set( indices[ subsampleIndex ] );
				// subsampleIndicesVariable
				break;
			}
		}

		GL_SetRenderDestination( renderDestManager.renderDestSMAABlend );
		GL_ClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

		GL_State( GLS_DISABLE_BLENDING | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS |
			GLS_STENCIL_FUNC_EQUAL | GLS_STENCIL_MAKE_REF( 1 ) | GLS_STENCIL_MAKE_MASK( 255 ) );

		renderProgManager.BindRenderProgram( SMAA::progBlendingWeightCalculation[ preset ] );

		renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderDestManager.renderDestSMAAEdges->GetColorImage() );
		renderProgManager.SetRenderParm( RENDERPARM_USERMAP1, renderImageManager->smaaAreaImage );
		renderProgManager.SetRenderParm( RENDERPARM_USERMAP2, renderImageManager->smaaSearchImage );

		GL_DrawScreenTriangleAuto();

		RENDERLOG_CLOSE_BLOCK();
	}

	{
		idScopedRenderLogBlock log( "Neighborhood Blending Pass" );

		//if( dstRTV ) {
		//	GL_SetRenderDestination( dstRTV );
		//}
		//else
			GL_SetNativeRenderDestination();

		if( r_smaa_useReprojection.GetBool() )
		{
			GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHMASK | GLS_DISABLE_DEPTHTEST );
			glBlendColor( SMAA::blendFactorVariable, SMAA::blendFactorVariable, SMAA::blendFactorVariable, SMAA::blendFactorVariable );

			//renderProgManager.SetRenderParm( RENDERPARM_USERMAP2, renderImageManager->velocityImage );
		}
		else {
			GL_State( GLS_DISABLE_BLENDING | GLS_DEPTHMASK | GLS_DISABLE_DEPTHTEST );
		}

		renderProgManager.BindRenderProgram( SMAA::progNeighborhoodBlending[ r_smaa_useReprojection.GetBool() ] );

		renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderImageManager->smaaInputImage );
		renderProgManager.SetRenderParm( RENDERPARM_USERMAP1, renderDestManager.renderDestSMAABlend->GetColorImage() );

		GL_DrawScreenTriangleAuto();

		// Initially the GL_BLEND_COLOR is set to (0, 0, 0, 0), so restore it.
		glBlendColor( 0.0f, 0.0f, 0.0f, 0.0f );
		SMAA::blendFactorVariable = 0.0f;
	}
};


/**
* This function perform a temporal resolve of two buffers. They must
* contain temporary jittered color subsamples.
*/
void DoReproject( idImage * currentSRV, idImage * previousSRV, idImage * velocitySRV, const idRenderDestination * dstRTV )
{
	RENDERLOG_OPEN_BLOCK( "Temporal Antialiasing" );
/*
	// Save the state:
	SaveViewportsScope saveViewport( device );
	SaveRenderTargetsScope saveRenderTargets( device );
	SaveInputLayoutScope saveInputLayout( device );
	SaveBlendStateScope saveBlendState( device );
	SaveDepthStencilScope saveDepthStencil( device );

	// Setup the viewport and the vertex layout:
	edgesRT->setViewport();
	triangle->setInputLayout();

	// Setup variables:
	V( colorTexVariable->SetResource( currentSRV ) );
	V( colorTexPrevVariable->SetResource( previousSRV ) );
	V( velocityTexVariable->SetResource( velocitySRV ) );

	// Select the technique accordingly:
	V( resolveTechnique->GetPassByIndex( 0 )->Apply( 0 ) );

	// Do it!
	device->OMSetRenderTargets( 1, &dstRTV, nullptr );
	triangle->draw();
	device->OMSetRenderTargets( 0, nullptr, nullptr );

	// Reset external inputs, to avoid warnings:
	V( colorTexVariable->SetResource( nullptr ) );
	V( colorTexPrevVariable->SetResource( nullptr ) );
	V( velocityTexVariable->SetResource( nullptr ) );
	V( resolveTechnique->GetPassByIndex( 0 )->Apply( 0 ) );
*/
	GL_State( GLS_DISABLE_BLENDING | GLS_DEPTHMASK | GLS_DISABLE_DEPTHTEST );

	renderProgManager.BindRenderProgram( SMAA::progResolve[ r_smaa_useReprojection.GetBool() ] );

	GL_DrawScreenTriangleAuto();

	RENDERLOG_CLOSE_BLOCK();
}

/**
* This function separates 2 subsamples in a 2x multisampled buffer
* (srcSRV) into two different render targets.
*/
void DoSeparate( idImage *srcSRV, const idRenderDestination *dst1RTV, const idRenderDestination *dst2RTV )
{
	RENDERLOG_OPEN_BLOCK( "Separate MSAA Samples Pass" );
/*
	// Save the state:
	SaveViewportsScope saveViewport( device );
	SaveRenderTargetsScope saveRenderTargets( device );
	SaveInputLayoutScope saveInputLayout( device );
	SaveBlendStateScope saveBlendState( device );
	SaveDepthStencilScope saveDepthStencil( device );

	// Setup the viewport and the vertex layout:
	edgesRT->setViewport();
	triangle->setInputLayout();

	// Setup variables:
	V( colorTexMSVariable->SetResource( srcSRV ) );

	// Select the technique accordingly:
	V( separateTechnique->GetPassByIndex( 0 )->Apply( 0 ) );

	// Do it!
	ID3D10RenderTargetView *dst[] = { dst1RTV, dst2RTV };
	device->OMSetRenderTargets( 2, dst, nullptr );
	triangle->draw();
	device->OMSetRenderTargets( 0, nullptr, nullptr );
*/
	GL_SetRenderDestination( dstRTV );

	GL_State( GLS_DISABLE_BLENDING | GLS_DEPTHMASK | GLS_DISABLE_DEPTHTEST );

	renderProgManager.BindRenderProgram( SMAA::progSeparate[ r_smaa_useReprojection.GetBool() ] );

	GL_DrawScreenTriangleAuto();

	RENDERLOG_CLOSE_BLOCK();
}

void RB_SMAA( const idRenderDestination * renderDestInput, const idRenderDestination * renderDestOutput )
{
	const idRenderDestination * backbufferRT;
	const idRenderDestination * previousRT[ 2 ];
	const idRenderDestination * velocityRT;

	// ----------------------------------------

	int screenWidth = renderSystem->GetWidth();
	int screenHeight = renderSystem->GetHeight();

	// set the window clipping
	GL_ViewportAndScissor( 0, 0, screenWidth, screenHeight );

	GL_State( GLS_DISABLE_BLENDING | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS | GLS_TWOSIDED );
	//GL_Cull( CT_TWO_SIDED );

	auto SMAA_RT_METRICS = renderProgManager.GetRenderParm( RENDERPARM_USERVEC0 )->GetVecPtr();
	SMAA_RT_METRICS[ 0 ] = 1.0f / screenWidth;
	SMAA_RT_METRICS[ 1 ] = 1.0f / screenHeight;
	SMAA_RT_METRICS[ 2 ] = screenWidth;
	SMAA_RT_METRICS[ 3 ] = screenHeight;

	// Calculate next subpixel index:
	const uint previousIndex = SMAA::frameIndex;
	const uint currentIndex = ( SMAA::frameIndex + 1U ) % 2U;

	const auto mode = ( SMAA::Mode_e ) r_smaa_mode.GetInteger();
	const auto input = ( SMAA::Input_e ) r_smaa_input.GetInteger();

	RENDERLOG_OPEN_BLOCK( "RB_SMAA" );
	switch( mode )
	{
		case SMAA::MODE_SMAA_1X:
			DoSMAA( /**rtc.sceneRT, *rtc.sceneRT_SRGB, *rtc.depthRT, nullptr, *rtc.mainDS, */ backbufferRT, input, mode );
			break;

		case SMAA::MODE_SMAA_T2X:
			DoSMAA( /**rtc.sceneRT, *rtc.sceneRT_SRGB, *rtc.depthRT, *rtc.velocityRT, *rtc.mainDS,*/ previousRT[ currentIndex ], input, mode );
			DoReproject( previousRT[ currentIndex ]->GetColorImage(), previousRT[ previousIndex ]->GetColorImage(), velocityRT->GetColorImage(), backbufferRT );
			break;
	/*
		case SMAA::MODE_SMAA_S2X:
			DoSeparate( *rtc.sceneRT_MS, *rtc.tmpRT[ 0 ], *rtc.tmpRT[ 1 ] );
			DoSMAA( *rtc.tmpRT[ 0 ], *rtc.tmpRT_SRGB[ 0 ], *rtc.depthRT, nullptr, *rtc.mainDS, *backbufferRT, input, mode, 0 );
			DoSMAA( *rtc.tmpRT[ 1 ], *rtc.tmpRT_SRGB[ 1 ], *rtc.depthRT, nullptr, *rtc.mainDS, *backbufferRT, input, mode, 1 );
			break;

		case SMAA::MODE_SMAA_4X:
			DoSeparate( *rtc.sceneRT_MS, *rtc.tmpRT[ 0 ], *rtc.tmpRT[ 1 ] );
			DoSMAA( *rtc.tmpRT[ 0 ], *rtc.tmpRT_SRGB[ 0 ], *rtc.depthRT, *rtc.velocityRT, *rtc.mainDS, *rtc.previousRT[ currentIndex ], input, mode, 0 );
			DoSMAA( *rtc.tmpRT[ 1 ], *rtc.tmpRT_SRGB[ 1 ], *rtc.depthRT, *rtc.velocityRT, *rtc.mainDS, *rtc.previousRT[ currentIndex ], input, mode, 1 );
			DoReproject( *rtc.previousRT[ currentIndex ], *rtc.previousRT[ previousIndex ], *rtc.velocityRT, *backbufferRT );
			break;
	*/
	}
	RENDERLOG_CLOSE_BLOCK();

	// Update subpixel index:
	SMAA::NextFrame();
}

#else

/*
====================================
	The shader has three passes,
	chained together as follows:

			|input|-----------------·
				v                   |
	[ SMAA*EdgeDetection ]          |
				v                   |
			|edgesTex|              |
				v                   |
[ SMAABlendingWeightCalculation ]   |
				v                   |
			|blendTex|              |
				v                   |
[ SMAANeighborhoodBlending ] <------·
				v
			|output|
====================================
*/
void RB_SMAA( const idRenderView * renderView )
{
	RENDERLOG_OPEN_BLOCK( "RB_SMAA" );

	//auto const renderDestDefault = renderDestManager.renderDestBaseHDR;

	const int screenWidth = renderSystem->GetWidth();
	const int screenHeight = renderSystem->GetHeight();

	// resolve the scaled rendering to a temporary texture
	auto srcColorImage = renderImageManager->currentRenderImage;
	//auto srcColorImage = renderImageManager->smaaInputImage;
	//GL_CopyCurrentColorToTexture( srcColorImage, 0, 0, screenWidth, screenHeight );
	const int x = renderView->GetViewport().x1;
	const int y = renderView->GetViewport().y1;
	const int w = renderView->GetViewport().GetWidth();
	const int h = renderView->GetViewport().GetHeight();
	GL_CopyCurrentColorToTexture( srcColorImage, x, y, w, h );
	/*GL_BlitRenderBuffer(
		GL_GetCurrentRenderDestination(), x, y, w, h,
		renderDestManager.renderDestStagingColor, 0, 0, w, h,
		true, false );*/

	// set SMAA_RT_METRICS = rpScreenCorrectionFactor
	auto screenCorrectionParm = renderProgManager.GetRenderParm( RENDERPARM_SCREENCORRECTIONFACTOR )->GetVecPtr();
	//screenCorrectionParm[ 0 ] = 1.0f / screenWidth;
	//screenCorrectionParm[ 1 ] = 1.0f / screenHeight;
	//screenCorrectionParm[ 2 ] = screenWidth;
	//screenCorrectionParm[ 3 ] = screenHeight;
	screenCorrectionParm[ 0 ] = 1.0f / w;
	screenCorrectionParm[ 1 ] = 1.0f / h;
	screenCorrectionParm[ 2 ] = w;
	screenCorrectionParm[ 3 ] = h;

	{ // Do full resolution
		GL_SetRenderDestination( renderDestManager.renderDestSMAAEdges );
		GL_ViewportAndScissor( 0, 0, screenWidth, screenHeight );
		GL_State( GLS_DISABLE_BLENDING | GLS_DISABLE_DEPTHTEST | GLS_TWOSIDED | GLS_STENCIL_OP_PASS_REPLACE | GLS_STENCIL_MAKE_REF( 1 ) | GLS_STENCIL_MAKE_MASK( 255 ) );
		GL_ClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

		renderProgManager.SetRenderParm( RENDERPARM_SMAAINPUTIMAGE, srcColorImage );

		GL_SetRenderProgram( renderProgManager.prog_SMAA_edge_detection );

		///GL_DrawUnitSquare();
		GL_DrawScreenTriangleAuto();
		GL_ResetTextureState();
	}

	{ // Do full resolution
		GL_SetRenderDestination( renderDestManager.renderDestSMAABlend );
		GL_ViewportAndScissor( 0, 0, screenWidth, screenHeight );
		GL_State( GLS_DISABLE_BLENDING | GLS_DISABLE_DEPTHTEST | GLS_TWOSIDED | GLS_STENCIL_FUNC_EQUAL | GLS_STENCIL_MAKE_REF( 1 ) | GLS_STENCIL_MAKE_MASK( 255 ) );
		GL_ClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

		renderProgManager.SetRenderParm( RENDERPARM_SMAAEDGESIMAGE, renderDestManager.renderDestSMAAEdges->GetColorImage() );
		renderProgManager.SetRenderParm( RENDERPARM_SMAAAREAIMAGE, renderImageManager->smaaAreaImage );
		renderProgManager.SetRenderParm( RENDERPARM_SMAASEARCHIMAGE, renderImageManager->smaaSearchImage );

		GL_SetRenderProgram( renderProgManager.prog_SMAA_blending_weight_calc );

		///GL_DrawUnitSquare();
		GL_DrawScreenTriangleAuto();
		GL_ResetTextureState();
	}

	RB_ResetBaseRenderDest( r_useHDR.GetBool() );
	//GL_SetNativeRenderDestination();
	GL_State( GLS_DISABLE_BLENDING | GLS_DISABLE_DEPTHTEST | GLS_TWOSIDED );

	//renderImageManager->smaaBlendImage->CopyFramebuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );

	renderProgManager.SetRenderParm( RENDERPARM_SMAAINPUTIMAGE, srcColorImage );
	renderProgManager.SetRenderParm( RENDERPARM_SMAABLENDIMAGE, renderDestManager.renderDestSMAABlend->GetColorImage() );

	GL_SetRenderProgram( renderProgManager.prog_SMAA_final );

	///GL_DrawUnitSquare();
	GL_DrawScreenTriangleAuto();

	GL_ResetTextureState();

	RENDERLOG_CLOSE_BLOCK();
}

#endif