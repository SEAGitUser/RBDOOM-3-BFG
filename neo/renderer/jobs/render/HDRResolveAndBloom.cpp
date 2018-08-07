#include "precompiled.h"
#pragma hdrstop

#include "Render.h"

/*
==============================================================================================

	HDR RESOLVE AND BLOOM RENDER PASSES

	width = max( 1, floor(width / 2) );
	height = max( 1, floor(height / 2) );

==============================================================================================
*/

idCVar r_useBloom( "r_useBloom", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "0-None, 1-BloomRB, 2-Natural", 0.0, 3.0 );

idCVar r_hdrAutoExposure( "r_hdrAutoExposure", "1", CVAR_RENDERER | CVAR_BOOL, "EXPENSIVE: enables adapative HDR tone mapping otherwise the exposure is derived by r_exposure" );
idCVar r_hdrMinLuminance( "r_hdrMinLuminance", "0.005", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_hdrMaxLuminance( "r_hdrMaxLuminance", "300", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_hdrKey( "r_hdrKey", "0.015", CVAR_RENDERER | CVAR_FLOAT, "magic exposure key that works well with Doom 3 maps" );
idCVar r_hdrContrastDynamicThreshold( "r_hdrContrastDynamicThreshold", "2", CVAR_RENDERER | CVAR_FLOAT, "if auto exposure is on, all pixels brighter than this cause HDR bloom glares" );
idCVar r_hdrContrastStaticThreshold( "r_hdrContrastStaticThreshold", "3", CVAR_RENDERER | CVAR_FLOAT, "if auto exposure is off, all pixels brighter than this cause HDR bloom glares" );
idCVar r_hdrContrastOffset( "r_hdrContrastOffset", "100", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_hdrGlarePasses( "r_hdrGlarePasses", "8", CVAR_RENDERER | CVAR_INTEGER, "how many times the bloom blur is rendered offscreen. number should be even" );
idCVar r_hdrDebug( "r_hdrDebug", "0", CVAR_RENDERER | CVAR_FLOAT, "show scene luminance as heat map" );

extern idCVar r_exposure;

/*
=========================================================================================================
 HDR PROC

 The best approach is to evaluate if what you are doing can be done without needing to read back at all.
 For example, you can calculate the average luminance of a scene for tonemapping by generating mipmaps
 for your texture; the 1x1 miplevel will contain the average and the end result is an operation
 that happens entirely on the GPU.
=========================================================================================================
*/
void RB_CalculateAdaptation()
{
	/*
	int x = backEnd.viewDef->GetViewport().x1;
	int y = backEnd.viewDef->GetViewport().y1;
	int	w = backEnd.viewDef->GetViewport().GetWidth();
	int	h = backEnd.viewDef->GetViewport().GetHeight();

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
	/*
	glBindFramebuffer( GL_READ_FRAMEBUFFER_EXT, globalFramebuffers.hdrFBO->GetFramebuffer() );
	glBindFramebuffer( GL_DRAW_FRAMEBUFFER_EXT, globalFramebuffers.hdr64FBO->GetFramebuffer() );
	glBlitFramebuffer( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight(),
	0, 0, 64, 64,
	GL_COLOR_BUFFER_BIT,
	GL_LINEAR );
	*/

	const int texSize = 64;
	const idVec3 LUMINANCE_SRGB( 0.2125f, 0.7154f, 0.0721f ); // be careful wether this should be linear RGB or sRGB

	GL_BlitRenderBuffer(
		renderDestManager.renderDestBaseHDR,
		backEnd.viewDef->GetViewport().x1, backEnd.viewDef->GetViewport().y1,
		backEnd.viewDef->GetViewport().GetWidth(), backEnd.viewDef->GetViewport().GetHeight(),
		renderDestManager.renderDestBaseHDRsml64, 0, 0, texSize, texSize,
		true, true );

	/////////////////////////////////////////////////////

	RENDERLOG_OPEN_BLOCK( "RB_CalculateAdaptation" );

	static ALIGNTYPE16 idArray< float, texSize * texSize * 4 > image;

	if( !r_hdrAutoExposure.GetBool() )
	{
		// no dynamic exposure

		backEnd.hdrKey = r_hdrKey.GetFloat();
		backEnd.hdrAverageLuminance = r_hdrMinLuminance.GetFloat();
		backEnd.hdrMaxLuminance = 1;
	}
	else {
		idRenderVector color;
		float curTime = MS2SEC( sys->Milliseconds() );

		//
		// Calculate the average scene luminance
		//

		GL_SetRenderDestination( renderDestManager.renderDestBaseHDRsml64 );

		// read back the contents
		//glFinish();

		glReadPixels( 0, 0, texSize, texSize, GL_BGRA, GL_FLOAT, image.Ptr() );
		RENDERLOG_PRINT( "glReadPixels( 0, 0, %i, %i, GL_BGRA, GL_FLOAT, image.Ptr())\n", texSize );

		double sum = 0.0f;
		float maxLuminance = 0.0f;
		for( int i = 0; i < ( texSize * texSize ); i += 4 )
		{
			color[ 0 ] = image[ i * 4 + 0 ];
			color[ 1 ] = image[ i * 4 + 1 ];
			color[ 2 ] = image[ i * 4 + 2 ];
			color[ 3 ] = image[ i * 4 + 3 ];

			///float luminance = ( color.x * LUMINANCE_SRGB.x + color.y * LUMINANCE_SRGB.y + color.z * LUMINANCE_SRGB.z ) + 0.0001f;
			float luminance = ( color.x * LUMINANCE_SRGB.y + color.y * LUMINANCE_SRGB.x + color.z * LUMINANCE_SRGB.z ) + 0.0001f; // BGRA
			if( luminance > maxLuminance )
			{
				maxLuminance = luminance;
			}

			float logLuminance = idMath::Log( luminance + 1 );
			//if( logLuminance > 0 )
			{
				sum += luminance;
			}
		}

	#if 0
		sum /= float( texSize * texSize );
		float avgLuminance = exp( sum );
	#else
		float avgLuminance = sum / float( texSize * texSize );
	#endif

		// the user's adapted luminance level is simulated by closing the gap between
		// adapted luminance and current luminance by 2% every frame, based on a
		// 30 fps rate. This is not an accurate model of human adaptation, which can
		// take longer than half an hour.
		if( backEnd.hdrTime > curTime )
		{
			backEnd.hdrTime = curTime;
		}

		float deltaTime = curTime - backEnd.hdrTime;

		//if( r_hdrMaxLuminance.GetFloat() )
		{
			backEnd.hdrAverageLuminance = idMath::ClampFloat( r_hdrMinLuminance.GetFloat(), r_hdrMaxLuminance.GetFloat(), backEnd.hdrAverageLuminance );
			avgLuminance = idMath::ClampFloat( r_hdrMinLuminance.GetFloat(), r_hdrMaxLuminance.GetFloat(), avgLuminance );

			backEnd.hdrMaxLuminance = idMath::ClampFloat( r_hdrMinLuminance.GetFloat(), r_hdrMaxLuminance.GetFloat(), backEnd.hdrMaxLuminance );
			maxLuminance = idMath::ClampFloat( r_hdrMinLuminance.GetFloat(), r_hdrMaxLuminance.GetFloat(), maxLuminance );
		}

		float newAdaptation = backEnd.hdrAverageLuminance + ( avgLuminance - backEnd.hdrAverageLuminance ) * ( 1.0f - idMath::Pow( 0.98f, 30.0f * deltaTime ) );
		float newMaximum = backEnd.hdrMaxLuminance + ( maxLuminance - backEnd.hdrMaxLuminance ) * ( 1.0f - idMath::Pow( 0.98f, 30.0f * deltaTime ) );

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

	// Then we multiply all pixels by exposure, add tone mapping, color grading and gamma.

	// Compute automatic exposure basing on scene luminance just after the main opaque pass.
	// This way we can skip all translucents, emissive and debug meshes and they won’t influence automatic exposure.
	// Additionally, this fixes feedback loop issues( at least if you don’t want to hack lights ).
	// The downside is that it won’t adapt to things like big emissive panels, but we can easily fix it
	// by marking such surfaces in G-buffer or stencil buffer and adding emissive to automatic exposure input only for the marked surfaces.

	// Naty Hoffman in his talk proposed to adapt to illuminance instead of final pixel luminance [Hof13]. This way material reflectance
	// won’t influence automatic exposure–dark corridors will remain dark and snow will be pure white as expected. Additionally it will
	// remove specular from automatic exposure input and further stabilize the automatic exposure.

	// Having the illuminance, we just need to add skybox and (lit) fog to create the final buffer for automatic exposure calculation.
	// It’s not obvious how to treat skybox. One constant color per skybox? Convert it to illuminance? Just sample skybox texture (luminance)?
	// We settled on luminance, as we want exposure to change depending on camera direction (bright white clouds near sun should have different
	// exposure than darker parts of the skybox at the opposite side). In any case we additionally need a manual exposure compensation for the skybox,
	// so lighting artists are able to manually set the optimal skybox brightness on screen, as the nighttime skybox
	// should be much darker on screen than a daytime one.

	// The idea here is to compute separately light diffuse and specular. Light diffuse is multiplied by material’s
	// diffuseColor in a final GBuffer composition pass – after computing exposure. This way, from PoV of automatic exposure,
	// every object is treated as a white diffuse surface and material’s diffuseColor or specularColor doesn’t influence automatic exposure.
}

void RB_Tonemap()
{
	RENDERLOG_OPEN_BLOCK( "RB_Tonemap" );
	RENDERLOG_PRINT( "( avg = %f, max = %f, key = %f, is2Dgui = %i )\n", backEnd.hdrAverageLuminance, backEnd.hdrMaxLuminance, backEnd.hdrKey, ( int )backEnd.viewDef->is2Dgui );

	//postProcessCommand_t* cmd = ( postProcessCommand_t* )data;
	//const idScreenRect& viewport = cmd->viewDef->viewport;
	//renderImageManager->currentRenderImage->CopyFramebuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );

	///GL_SetNativeRenderDestination();
	GL_SetRenderDestination( renderDestManager.renderDestBaseLDR );
	//GL_ViewportAndScissor( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() );
	RB_ResetViewportAndScissorToDefaultCamera( backEnd.viewDef );

	GL_State( GLS_DISABLE_BLENDING | GLS_DISABLE_DEPTHTEST | GLS_TWOSIDED );

	renderProgManager.SetRenderParm( RENDERPARM_CURRENTRENDERMAP, renderDestManager.viewColorHDRImage );

	if( r_hdrDebug.GetBool() )
	{
		GL_SetRenderProgram( renderProgManager.prog_tonemap_debug );
		renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderImageManager->heatmap7Image );
	}
	else {
		GL_SetRenderProgram( renderProgManager.prog_tonemap );
	}

	auto parm0 = renderProgManager.GetRenderParm( RENDERPARM_USERVEC0 )->GetVecPtr();

	if( backEnd.viewDef->is2Dgui )
	{
		parm0[ 0 ] = 2.0f;
		parm0[ 1 ] = 1.0f;
		parm0[ 2 ] = 1.0f;
	}
	else
	{
		if( r_hdrAutoExposure.GetBool() )
		{
			float exposureOffset = Lerp( -0.01f, 0.02f, idMath::ClampFloat( 0.0, 1.0, r_exposure.GetFloat() ) );

			parm0[ 0 ] = backEnd.hdrKey + exposureOffset;
			parm0[ 1 ] = backEnd.hdrAverageLuminance;
			parm0[ 2 ] = backEnd.hdrMaxLuminance;
			parm0[ 3 ] = exposureOffset;
			//screenCorrectionParm[3] = Lerp( -1, 5, idMath::ClampFloat( 0.0, 1.0, r_exposure.GetFloat() ) );
		}
		else
		{
			//float exposureOffset = ( idMath::ClampFloat( 0.0, 1.0, r_exposure.GetFloat() ) * 2.0f - 1.0f ) * 0.01f;

			float exposureOffset = Lerp( -0.01f, 0.01f, idMath::ClampFloat( 0.0, 1.0, r_exposure.GetFloat() ) );

			parm0[ 0 ] = 0.015f + exposureOffset;
			parm0[ 1 ] = 0.005f;
			parm0[ 2 ] = 1;

			// RB: this gives a nice exposure curve in Scilab when using
			// log2( max( 3 + 0..10, 0.001 ) ) as input for exp2
			//float exposureOffset = r_exposure.GetFloat() * 10.0f;
			//parm0[3] = exposureOffset;
		}
	}

	// Draw
	///GL_DrawUnitSquare();
	GL_DrawScreenTriangleAuto();

	// -----------------------------------------
	// reset states

	GL_ResetTextureState();
	GL_ResetProgramState();

	RENDERLOG_CLOSE_BLOCK();
}


/*
=========================================================================================================
	BLOOM RENDERPASSES
=========================================================================================================
*/
struct bloomRenderPassParms_t
{
	const idRenderDestination * renderDest_bloomRender[2];
	uint32						width, height;
	bool						hdrAutoExposure;
	float						hdrContrastDynamicThreshold;
	float						hdrContrastStaticThreshold;
	float						hdrContrastOffset;
	uint32						hdrGlarePasses;
};

void RB_Bloom()
{
	RENDERLOG_OPEN_BLOCK( "RB_Bloom" );
	RENDERLOG_PRINT( "( avg = %f, max = %f, key = %f )\n", backEnd.hdrAverageLuminance, backEnd.hdrMaxLuminance, backEnd.hdrKey );

	// BRIGHTPASS

	GL_SetRenderDestination( renderDestManager.renderDestBloomRender[ 0 ] );
	GL_ViewportAndScissor( 0, 0, renderSystem->GetWidth() / 4, renderSystem->GetHeight() / 4 );

	///	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	//	glClear( GL_COLOR_BUFFER_BIT );

	GL_State( /*GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO |*/ GLS_DISABLE_DEPTHTEST | GLS_TWOSIDED );

	// ---------------------------------------------------------------

	GL_SetRenderProgram( renderProgManager.prog_tonemap_brightpass );

	renderProgManager.SetRenderParm( RENDERPARM_CURRENTRENDERMAP, renderDestManager.viewColorHDRImage );

	auto Parm0 = renderProgManager.GetRenderParm( RENDERPARM_USERVEC0 )->GetVecPtr();
	Parm0[ 0 ] = backEnd.hdrKey;
	Parm0[ 1 ] = backEnd.hdrAverageLuminance;
	Parm0[ 2 ] = backEnd.hdrMaxLuminance;
	Parm0[ 3 ] = 1.0f;

	auto Parm1 = renderProgManager.GetRenderParm( RENDERPARM_USERVEC1 )->GetVecPtr();
	Parm1[ 0 ] = r_hdrAutoExposure.GetBool() ? r_hdrContrastDynamicThreshold.GetFloat() : r_hdrContrastStaticThreshold.GetFloat();
	Parm1[ 1 ] = r_hdrContrastOffset.GetFloat();
	Parm1[ 2 ] = 0.0f;
	Parm1[ 3 ] = 0.0f;

	///GL_DrawUnitSquare();
	GL_DrawScreenTriangleAuto();
	// ---------------------------------------------------------------

	// BLOOM PING PONG rendering
	GL_SetRenderProgram( renderProgManager.prog_hdr_glare_chromatic );

	int j;
	for( j = 0; j < r_hdrGlarePasses.GetInteger(); j++ )
	{
		GL_SetRenderDestination( renderDestManager.renderDestBloomRender[ ( j + 1 ) % 2 ] );
		GL_Clear( true, false, false, 0, 0.0f, 0.0f, 0.0f, 1.0f );

		renderProgManager.SetRenderParm( RENDERPARM_BLOOMRENDERMAP, renderDestManager.bloomRenderImage[ j % 2 ] );

		///GL_DrawUnitSquare();
		GL_DrawScreenTriangleAuto();
		// ---------------------------------------------------------------
	}

	// add filtered glare back to main context
	RB_ResetBaseRenderDest( r_useHDR.GetBool() );
	RB_ResetViewportAndScissorToDefaultCamera( backEnd.viewDef );
	GL_State( GLS_BLEND_ADD | GLS_DISABLE_DEPTHTEST | GLS_TWOSIDED );

	GL_SetRenderProgram( renderProgManager.prog_screen );

	renderProgManager.SetRenderParm( RENDERPARM_MAP, renderDestManager.bloomRenderImage[ ( j + 1 ) % 2 ] );

	GL_DrawUnitSquare();
	//GL_DrawScreenTriangleAuto();
	// ---------------------------------------------------------------

	GL_ResetTextureState();
	GL_ResetProgramState();

	RENDERLOG_CLOSE_BLOCK();
}

/*
=========================================================================================================

								NATURAL BLOOM IMPLEMENTATION

=========================================================================================================
*/
idCVar r_bloomUseLuminance( "r_bloomUseLuminance", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "use luminance to extract the bright spots from the render instead of the average RGB value." );
idCVar r_bloomPreset( "r_bloomPreset", "3", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "0-Wide, 1-Focussed, 2-Small, 3-SuperWide, 4-Cheap, 5-UserCvars", 0.0, 5.0 );
idCVar r_bloomThreshold( "r_bloomThreshold", "2", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "bright spots extraction filter threshold, range 0.0 - 4.0", 0.0, 4.0 );

//Preset variables for different mip levels

idCVar r_bloomRadius1( "r_bloomRadius1", "1.0f", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "mip0 radius, range 0.0 - 8.0, def 1.0", 0.0, 8.0 );
idCVar r_bloomRadius2( "r_bloomRadius2", "1.0f", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "mip1 radius, range 0.0 - 8.0", 0.0, 8.0 );
idCVar r_bloomRadius3( "r_bloomRadius3", "1.0f", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "mip2 radius, range 0.0 - 8.0", 0.0, 8.0 );
idCVar r_bloomRadius4( "r_bloomRadius4", "1.0f", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "mip3 radius, range 0.0 - 8.0", 0.0, 8.0 );
idCVar r_bloomRadius5( "r_bloomRadius5", "1.0f", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "mip4 radius, range 0.0 - 8.0", 0.0, 8.0 );

idCVar r_bloomStrength1( "r_bloomStrength1", "1.0f", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "mip0 strength, range 0.0 - 8.0", 0.0, 8.0 );
idCVar r_bloomStrength2( "r_bloomStrength2", "1.0f", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "mip1 strength, range 0.0 - 8.0", 0.0, 8.0 );
idCVar r_bloomStrength3( "r_bloomStrength3", "1.0f", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "mip2 strength, range 0.0 - 8.0", 0.0, 8.0 );
idCVar r_bloomStrength4( "r_bloomStrength4", "1.0f", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "mip3 strength, range 0.0 - 8.0", 0.0, 8.0 );
idCVar r_bloomStrength5( "r_bloomStrength5", "1.0f", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "mip4 strength, range 0.0 - 8.0", 0.0, 8.0 );

enum class bloomPresets_e {
	Wide = 0,
	Focussed,
	Small,
	SuperWide,
	Cheap
};

void UpdateResolution( int width, int height )
{
/*
	_width = width;
	_height = height;

	if( _bloomRenderTarget2DMip0 != null )
	{
		Dispose();
	}

	renderDestBloomMip0 = new RenderTarget2D( _graphicsDevice, ( int )( width ), ( int )( height ), false, SurfaceFormat.HalfVector4, DepthFormat.None, 0, RenderTargetUsage.PreserveContents );
	renderDestBloomMip1 = new RenderTarget2D( _graphicsDevice, ( int )( width / 2 ), ( int )( height / 2 ), false, SurfaceFormat.HalfVector4, DepthFormat.None, 0, RenderTargetUsage.PreserveContents );
	renderDestBloomMip2 = new RenderTarget2D( _graphicsDevice, ( int )( width / 4 ), ( int )( height / 4 ), false, SurfaceFormat.HalfVector4, DepthFormat.None, 0, RenderTargetUsage.PreserveContents );
	renderDestBloomMip3 = new RenderTarget2D( _graphicsDevice, ( int )( width / 8 ), ( int )( height / 8 ), false, SurfaceFormat.HalfVector4, DepthFormat.None, 0, RenderTargetUsage.PreserveContents );
	renderDestBloomMip4 = new RenderTarget2D( _graphicsDevice, ( int )( width / 16 ), ( int )( height / 16 ), false, SurfaceFormat.HalfVector4, DepthFormat.None, 0, RenderTargetUsage.PreserveContents );
	renderDestBloomMip5 = new RenderTarget2D( _graphicsDevice, ( int )( width / 32 ), ( int )( height / 32 ), false, SurfaceFormat.HalfVector4, DepthFormat.None, 0, RenderTargetUsage.PreserveContents );
*/
}

void Downsample( const idRenderDestination * dest )
{
	GL_SetRenderDestination( dest );
	GL_ViewportAndScissor( 0, 0, dest->GetTargetWidth(), dest->GetTargetHeight() );
	GL_State( GLS_TWOSIDED | GLS_DISABLE_BLENDING | GLS_DISABLE_DEPTHTEST );
	GL_SetRenderProgram( renderProgManager.prog_bloomDownscaleBlur );
	GL_DrawScreenTriangleAuto();
}

void Upsample( const idRenderDestination * dest )
{
	GL_SetRenderDestination( dest );
	GL_ViewportAndScissor( 0, 0, dest->GetTargetWidth(), dest->GetTargetHeight() );
	//GL_State( GLS_TWOSIDED | GLS_BLEND_ADD | GLS_DISABLE_DEPTHTEST );
	GL_State( GLS_TWOSIDED | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DISABLE_DEPTHTEST );
	GL_SetRenderProgram( renderProgManager.prog_bloomUpscaleBlur );
	GL_DrawScreenTriangleAuto();
}

/*
====================================
 RB_Bloom2
====================================
*/
void RB_BloomNatural()
{
	RENDERLOG_SCOPED_BLOCK("RB_BloomNatural");

	int _width = renderSystem->GetWidth();
	int _height = renderSystem->GetHeight();

	const bool bBloomUseLuminance = r_bloomUseLuminance.GetBool();
	const auto inputTexture = GL_GetCurrentRenderDestination()->GetColorImage();

	float _bloomRadius1, _bloomRadius2, _bloomRadius3, _bloomRadius4, _bloomRadius5;
	float _bloomStrength1, _bloomStrength2, _bloomStrength3, _bloomStrength4, _bloomStrength5;

	// Default threshold is 0.8
	float _bloomThreshold = r_bloomThreshold.GetFloat() * 0.1f;

	// Adjust the blur so it looks consistent across diferrent scalings
	float _radiusMultiplier = ( float )_width / inputTexture->GetUploadWidth();

	float _bloomStreakLength = 1.0;
	int _bloomDownsamplePasses = 5;

	// Update our variables with the multiplier.
	// A few presets with different values for the different mip levels of our bloom.
	switch( ( bloomPresets_e )r_bloomPreset.GetInteger() )
	{
		case bloomPresets_e::Wide:
		{
			_bloomStrength1 = 0.5f;
			_bloomStrength2 = 1.0f;
			_bloomStrength3 = 2.0f;
			_bloomStrength4 = 1.0f;
			_bloomStrength5 = 2.0f;
			_bloomRadius5 = 4.0f;
			_bloomRadius4 = 4.0f;
			_bloomRadius3 = 2.0f;
			_bloomRadius2 = 2.0f;
			_bloomRadius1 = 1.0f;
			_bloomStreakLength = 1.0f;
			_bloomDownsamplePasses = 5;
			break;
		}
		case bloomPresets_e::SuperWide:
		{
			_bloomStrength1 = 0.9f;
			_bloomStrength2 = 1.0f;
			_bloomStrength3 = 1.0f;
			_bloomStrength4 = 2.0f;
			_bloomStrength5 = 6.0f;
			_bloomRadius5 = 4.0f;
			_bloomRadius4 = 2.0f;
			_bloomRadius3 = 2.0f;
			_bloomRadius2 = 2.0f;
			_bloomRadius1 = 2.0f;
			_bloomStreakLength = 1.0f;
			_bloomDownsamplePasses = 5;
			break;
		}
		case bloomPresets_e::Focussed:
		{
			_bloomStrength1 = 0.8f;
			_bloomStrength2 = 1.0f;
			_bloomStrength3 = 1.0f;
			_bloomStrength4 = 1.0f;
			_bloomStrength5 = 2.0f;
			_bloomRadius5 = 4.0f;
			_bloomRadius4 = 2.0f;
			_bloomRadius3 = 2.0f;
			_bloomRadius2 = 2.0f;
			_bloomRadius1 = 2.0f;
			_bloomStreakLength = 1.0f;
			_bloomDownsamplePasses = 5;
			break;
		}
		case bloomPresets_e::Small:
		{
			_bloomStrength1 = 0.8f;
			_bloomStrength2 = 1.0f;
			_bloomStrength3 = 1.0f;
			_bloomStrength4 = 1.0f;
			_bloomStrength5 = 1.0f;
			_bloomRadius5 = 1.0f;
			_bloomRadius4 = 1.0f;
			_bloomRadius3 = 1.0f;
			_bloomRadius2 = 1.0f;
			_bloomRadius1 = 1.0f;
			_bloomStreakLength = 1.0f;
			_bloomDownsamplePasses = 5;
			break;
		}
		case bloomPresets_e::Cheap:
		{
			_bloomStrength1 = 0.8f;
			_bloomStrength2 = 2.0f;
			_bloomRadius2 = 2.0f;
			_bloomRadius1 = 2.0f;
			_bloomStreakLength = 1.0f;
			_bloomDownsamplePasses = 2;
			break;
		}
		default:
		{
			_bloomRadius1 = r_bloomRadius1.GetFloat();
			_bloomRadius2 = r_bloomRadius2.GetFloat();
			_bloomRadius3 = r_bloomRadius3.GetFloat();
			_bloomRadius4 = r_bloomRadius4.GetFloat();
			_bloomRadius5 = r_bloomRadius5.GetFloat();
			_bloomStrength1 = r_bloomStrength1.GetFloat();
			_bloomStrength2 = r_bloomStrength2.GetFloat();
			_bloomStrength3 = r_bloomStrength3.GetFloat();
			_bloomStrength4 = r_bloomStrength4.GetFloat();
			_bloomStrength5 = r_bloomStrength5.GetFloat();
			_bloomStreakLength = 1.0f;
			_bloomDownsamplePasses = 5;
			break;
		}
	}

	////////////////////////////////////////////

	auto & bloomInverseResolution = renderProgManager.GetRenderParm( RENDERPARM_USERVEC0 )->GetVec4();
	auto & parms = renderProgManager.GetRenderParm( RENDERPARM_USERVEC1 )->GetVec4();

	// Extract the bright values which are above the Threshold and save them to Mip0
	{
		GL_SetRenderDestination( renderDestManager.renderDestBloomMip[0] );
		GL_ViewportAndScissor( 0, 0, renderDestManager.renderDestBloomMip[0]->GetTargetWidth(), renderDestManager.renderDestBloomMip[0]->GetTargetHeight() );

		// BloomScreenTexture
		renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, inputTexture );

		bloomInverseResolution = idRenderVector( 1.0f / _width, 1.0f / _height, 0.0f, 0.0f );
		bloomInverseResolution[ 3 ] = 0.0f; // sampled mip level

		parms[ 0 ] = _bloomThreshold;
		parms[ 1 ] = 0.0;
		parms[ 2 ] = 0.0;
		parms[ 3 ] = 0.0f;

		GL_SetRenderProgram( bBloomUseLuminance ? renderProgManager.prog_bloomExtractLuminance : renderProgManager.prog_bloomExtractAvarage );
		GL_DrawScreenTriangleAuto();
	}

	// Now downsample to the next lower mip texture
	if( _bloomDownsamplePasses > 0 ) // DOWNSAMPLE TO MIP1
	{
		bloomInverseResolution.x *= 2.0f;
		bloomInverseResolution.y *= 2.0f;
		bloomInverseResolution.z = 0.0f;
		bloomInverseResolution.w = 0.0f; // sampled mip level

		//BloomScreenTexture = renderDestBloomMip0;
		renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderDestManager.renderDestBloomMip[0]->GetColorImage() );

		parms[ 0 ] = 0.0;
		parms[ 1 ] = 0.0;
		parms[ 2 ] = 0.0;
		parms[ 3 ] = _bloomStreakLength;

		Downsample( renderDestManager.renderDestBloomMip[ 1 ] );

		if( _bloomDownsamplePasses > 1 ) // DOWNSAMPLE TO MIP2
		{
			// Our input resolution is halfed, so our inverse 1/res. must be doubled
			bloomInverseResolution.x *= 2.0f;
			bloomInverseResolution.y *= 2.0f;
			bloomInverseResolution.z = 0.0f;
			bloomInverseResolution.w = 1.0f; // sampled mip level

			//BloomScreenTexture = renderDestBloomMip1;
			renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderDestManager.renderDestBloomMip[1]->GetColorImage() );

			parms[ 0 ] = 0.0;
			parms[ 1 ] = 0.0;
			parms[ 2 ] = 0.0;
			parms[ 3 ] = _bloomStreakLength;

			Downsample( renderDestManager.renderDestBloomMip[ 2 ] );

			if( _bloomDownsamplePasses > 2 ) // DOWNSAMPLE TO MIP3
			{
				bloomInverseResolution.x *= 2.0f;
				bloomInverseResolution.y *= 2.0f;
				bloomInverseResolution.z = 0.0f;
				bloomInverseResolution.w = 2.0f; // sampled mip level

				//BloomScreenTexture = renderDestBloomMip2;
				renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderDestManager.renderDestBloomMip[2]->GetColorImage() );

				parms[ 0 ] = 0.0;
				parms[ 1 ] = 0.0;
				parms[ 2 ] = 0.0;
				parms[ 3 ] = _bloomStreakLength;

				Downsample( renderDestManager.renderDestBloomMip[ 3 ] );

				if( _bloomDownsamplePasses > 3 ) // DOWNSAMPLE TO MIP4
				{
					bloomInverseResolution.x *= 2.0f;
					bloomInverseResolution.y *= 2.0f;
					bloomInverseResolution.z = 0.0f;
					bloomInverseResolution.w = 3.0f; // sampled mip level

					//BloomScreenTexture = renderDestBloomMip3;
					renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderDestManager.renderDestBloomMip[3]->GetColorImage() );

					parms[ 0 ] = 0.0;
					parms[ 1 ] = 0.0;
					parms[ 2 ] = 0.0;
					parms[ 3 ] = _bloomStreakLength;

					Downsample( renderDestManager.renderDestBloomMip[ 4 ] );

					if( _bloomDownsamplePasses > 4 ) // DOWNSAMPLE TO MIP5
					{
						bloomInverseResolution.x *= 2.0f;
						bloomInverseResolution.y *= 2.0f;
						bloomInverseResolution.z = 0.0f;
						bloomInverseResolution.w = 4.0f; // sampled mip level

						//BloomScreenTexture = renderDestBloomMip4;
						renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderDestManager.renderDestBloomMip[4]->GetColorImage() );

						parms[ 0 ] = 0.0;
						parms[ 1 ] = 0.0;
						parms[ 2 ] = 0.0;
						parms[ 3 ] = _bloomStreakLength;

						Downsample( renderDestManager.renderDestBloomMip[ 5 ] );

						/////////////////////////////
						// UPSAMPLE TO MIP4

						bloomInverseResolution.w = 5.0f; // sampled mip level

						//BloomScreenTexture = renderDestBloomMip5;
						renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderDestManager.renderDestBloomMip[5]->GetColorImage() );

						parms[ 0 ] = 0.0;
						parms[ 1 ] = _bloomRadius5 * _radiusMultiplier;
						parms[ 2 ] = _bloomStrength5;
						parms[ 3 ] = _bloomStreakLength;

						Upsample( renderDestManager.renderDestBloomMip[ 4 ] );

						bloomInverseResolution.x /= 2.0;
						bloomInverseResolution.y /= 2.0;
					}

					// UPSAMPLE TO MIP3

					bloomInverseResolution.w = 4.0f; // sampled mip level

					//BloomScreenTexture = renderDestBloomMip4;
					renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderDestManager.renderDestBloomMip[4]->GetColorImage() );

					parms[ 0 ] = 0.0;
					parms[ 1 ] = _bloomRadius4 * _radiusMultiplier;
					parms[ 2 ] = _bloomStrength4;
					parms[ 3 ] = _bloomStreakLength;

					Upsample( renderDestManager.renderDestBloomMip[ 3 ] );

					bloomInverseResolution.x /= 2.0;
					bloomInverseResolution.y /= 2.0;
					bloomInverseResolution.w = 2.0f; // sampled mip level
				}

				// UPSAMPLE TO MIP2

				bloomInverseResolution.w = 3.0f; // sampled mip level

				//BloomScreenTexture = renderDestBloomMip3;
				renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderDestManager.renderDestBloomMip[3]->GetColorImage() );

				parms[ 0 ] = 0.0;
				parms[ 1 ] = _bloomRadius3 * _radiusMultiplier;
				parms[ 2 ] = _bloomStrength3;
				parms[ 3 ] = _bloomStreakLength;

				Upsample( renderDestManager.renderDestBloomMip[ 2 ] );

				bloomInverseResolution.x /= 2.0;
				bloomInverseResolution.y /= 2.0;
			}

			// UPSAMPLE TO MIP1

			bloomInverseResolution.w = 2.0f; // sampled mip level

			//BloomScreenTexture = renderDestBloomMip2;
			renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderDestManager.renderDestBloomMip[2]->GetColorImage() );

			parms[ 0 ] = 0.0;
			parms[ 1 ] = _bloomRadius2 * _radiusMultiplier;
			parms[ 2 ] = _bloomStrength2;
			parms[ 3 ] = _bloomStreakLength;

			Upsample( renderDestManager.renderDestBloomMip[ 1 ] );

			bloomInverseResolution.x /= 2.0;
			bloomInverseResolution.y /= 2.0;
		}

		// UPSAMPLE TO MIP0

		bloomInverseResolution.w = 1.0f; // sampled mip level

		//BloomScreenTexture = renderDestBloomMip1;
		renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderDestManager.renderDestBloomMip[1]->GetColorImage() );

		parms[ 0 ] = 0.0;
		parms[ 1 ] = _bloomRadius1 * _radiusMultiplier;
		parms[ 2 ] = _bloomStrength1;
		parms[ 3 ] = _bloomStreakLength;

		Upsample( renderDestManager.renderDestBloomMip[ 0 ] );

		// Note the final step could be done as a blend to the final texture.
	}

	// add filtered glare back to main context

	RB_ResetBaseRenderDest( r_useHDR.GetBool() );
	GL_State( GLS_BLEND_ADD | GLS_DISABLE_DEPTHTEST | GLS_TWOSIDED );
	RB_ResetViewportAndScissorToDefaultCamera( backEnd.viewDef );

	//GL_SetNativeRenderDestination();
	//RB_ResetViewportAndScissorToDefaultCamera( backEnd.viewDef );

	renderProgManager.SetRenderParm( RENDERPARM_MAP, renderDestManager.renderDestBloomMip[ 0 ]->GetColorImage() );

	GL_SetRenderProgram( renderProgManager.prog_screen );

	GL_DrawUnitSquare();
	//GL_DrawScreenTriangleAuto();
	// ---------------------------------------------------------------

	GL_ResetTextureState();
	GL_ResetProgramState();
}
/*
====================================
 RB_BloomNatural2
====================================
*/
void RB_BloomNatural2()
{
	RENDERLOG_SCOPED_BLOCK( "RB_BloomNatural" );

	int _width = renderSystem->GetWidth();
	int _height = renderSystem->GetHeight();

	const bool bBloomUseLuminance = r_bloomUseLuminance.GetBool();
	const auto inputTexture = GL_GetCurrentRenderDestination()->GetColorImage();

	float _bloomRadius1, _bloomRadius2, _bloomRadius3, _bloomRadius4, _bloomRadius5;
	float _bloomStrength1, _bloomStrength2, _bloomStrength3, _bloomStrength4, _bloomStrength5;

	// Default threshold is 0.8
	float _bloomThreshold = r_bloomThreshold.GetFloat() * 0.1f;

	// Adjust the blur so it looks consistent across diferrent scalings
	float _radiusMultiplier = ( float )_width / inputTexture->GetUploadWidth();

	float _bloomStreakLength = 1.0;
	int _bloomDownsamplePasses = 5;

	// Update our variables with the multiplier.
	// A few presets with different values for the different mip levels of our bloom.
	switch( ( bloomPresets_e )r_bloomPreset.GetInteger() )
	{
		case bloomPresets_e::Wide:
		{
			_bloomStrength1 = 0.5f; _bloomStrength2 = 1.0f; _bloomStrength3 = 2.0f; _bloomStrength4 = 1.0f; _bloomStrength5 = 2.0f;
			_bloomRadius5 = 4.0f;	_bloomRadius4 = 4.0f;	_bloomRadius3 = 2.0f;	_bloomRadius2 = 2.0f;	_bloomRadius1 = 1.0f;
			_bloomStreakLength = 1.0f;
			_bloomDownsamplePasses = 5;
			break;
		}
		case bloomPresets_e::SuperWide:
		{
			_bloomStrength1 = 0.9f;	_bloomStrength2 = 1.0f; _bloomStrength3 = 1.0f; _bloomStrength4 = 2.0f; _bloomStrength5 = 6.0f;
			_bloomRadius5 = 4.0f;	_bloomRadius4 = 2.0f;	_bloomRadius3 = 2.0f;	_bloomRadius2 = 2.0f;	_bloomRadius1 = 2.0f;
			_bloomStreakLength = 1.0f;
			_bloomDownsamplePasses = 5;
			break;
		}
		case bloomPresets_e::Focussed:
		{
			_bloomStrength1 = 0.8f; _bloomStrength2 = 1.0f; _bloomStrength3 = 1.0f; _bloomStrength4 = 1.0f; _bloomStrength5 = 2.0f;
			_bloomRadius5 = 4.0f;	_bloomRadius4 = 2.0f;	_bloomRadius3 = 2.0f;	_bloomRadius2 = 2.0f;	_bloomRadius1 = 2.0f;
			_bloomStreakLength = 1.0f;
			_bloomDownsamplePasses = 5;
			break;
		}
		case bloomPresets_e::Small:
		{
			_bloomStrength1 = 0.8f; _bloomStrength2 = 1.0f; _bloomStrength3 = 1.0f; _bloomStrength4 = 1.0f; _bloomStrength5 = 1.0f;
			_bloomRadius5 = 1.0f;	_bloomRadius4 = 1.0f;	_bloomRadius3 = 1.0f;	_bloomRadius2 = 1.0f;	_bloomRadius1 = 1.0f;
			_bloomStreakLength = 1.0f;
			_bloomDownsamplePasses = 5;
			break;
		}
		case bloomPresets_e::Cheap:
		{
			_bloomStrength1 = 0.8f; _bloomStrength2 = 2.0f; _bloomRadius2 = 2.0f;
			_bloomRadius1 = 2.0f;
			_bloomStreakLength = 1.0f;
			_bloomDownsamplePasses = 2;
			break;
		}
		default:
		{
			_bloomRadius1 = r_bloomRadius1.GetFloat();
			_bloomRadius2 = r_bloomRadius2.GetFloat();
			_bloomRadius3 = r_bloomRadius3.GetFloat();
			_bloomRadius4 = r_bloomRadius4.GetFloat();
			_bloomRadius5 = r_bloomRadius5.GetFloat();
			_bloomStrength1 = r_bloomStrength1.GetFloat();
			_bloomStrength2 = r_bloomStrength2.GetFloat();
			_bloomStrength3 = r_bloomStrength3.GetFloat();
			_bloomStrength4 = r_bloomStrength4.GetFloat();
			_bloomStrength5 = r_bloomStrength5.GetFloat();
			_bloomStreakLength = 1.0f;
			_bloomDownsamplePasses = 5;
			break;
		}
	}

	////////////////////////////////////////////

	auto & bloomInverseResolution = renderProgManager.GetRenderParm( RENDERPARM_USERVEC0 )->GetVec4();
	auto & parms = renderProgManager.GetRenderParm( RENDERPARM_USERVEC1 )->GetVec4();

	bloomInverseResolution = idRenderVector( 1.0f / _width, 1.0f / _height, 0.0f, 0.0f );
	bloomInverseResolution[ 3 ] = 0.0f; // sampled mip level

	// Extract the bright values which are above the Threshold and save them to Mip0

	// Now downsample to the next lower mip texture
	if( _bloomDownsamplePasses > 0 ) // DOWNSAMPLE TO MIP1
	{
		bloomInverseResolution.x *= 2.0f;
		bloomInverseResolution.y *= 2.0f;
		bloomInverseResolution.z = 1.0f; // Extract the bright values which are above the Threshold
		bloomInverseResolution.w = 0.0f; // sampled mip level

		//BloomScreenTexture = renderDestBloomMip0;
		renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, inputTexture );

		parms[ 0 ] = _bloomThreshold;
		parms[ 1 ] = 0.0;
		parms[ 2 ] = 0.0;
		parms[ 3 ] = _bloomStreakLength;

		Downsample( renderDestManager.renderDestBloomMip[ 1 ] );

		if( _bloomDownsamplePasses > 1 ) // DOWNSAMPLE TO MIP2
		{
			// Our input resolution is halfed, so our inverse 1/res. must be doubled
			bloomInverseResolution.x *= 2.0f;
			bloomInverseResolution.y *= 2.0f;
			bloomInverseResolution.z = 0.0f;
			bloomInverseResolution.w = 1.0f; // sampled mip level

			//BloomScreenTexture = renderDestBloomMip1;
			renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderDestManager.renderDestBloomMip[ 1 ]->GetColorImage() );

			parms[ 0 ] = _bloomThreshold;
			parms[ 1 ] = 0.0;
			parms[ 2 ] = 0.0;
			parms[ 3 ] = _bloomStreakLength;

			Downsample( renderDestManager.renderDestBloomMip[ 2 ] );

			if( _bloomDownsamplePasses > 2 ) // DOWNSAMPLE TO MIP3
			{
				bloomInverseResolution.x *= 2.0f;
				bloomInverseResolution.y *= 2.0f;
				bloomInverseResolution.z = 0.0f;
				bloomInverseResolution.w = 2.0f; // sampled mip level

				 //BloomScreenTexture = renderDestBloomMip2;
				renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderDestManager.renderDestBloomMip[ 2 ]->GetColorImage() );

				parms[ 0 ] = _bloomThreshold;
				parms[ 1 ] = 0.0;
				parms[ 2 ] = 0.0;
				parms[ 3 ] = _bloomStreakLength;

				Downsample( renderDestManager.renderDestBloomMip[ 3 ] );

				if( _bloomDownsamplePasses > 3 ) // DOWNSAMPLE TO MIP4
				{
					bloomInverseResolution.x *= 2.0f;
					bloomInverseResolution.y *= 2.0f;
					bloomInverseResolution.z = 0.0f;
					bloomInverseResolution.w = 3.0f; // sampled mip level

					//BloomScreenTexture = renderDestBloomMip3;
					renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderDestManager.renderDestBloomMip[ 3 ]->GetColorImage() );

					parms[ 0 ] = _bloomThreshold;
					parms[ 1 ] = 0.0;
					parms[ 2 ] = 0.0;
					parms[ 3 ] = _bloomStreakLength;

					Downsample( renderDestManager.renderDestBloomMip[ 4 ] );

					if( _bloomDownsamplePasses > 4 ) // DOWNSAMPLE TO MIP5
					{
						bloomInverseResolution.x *= 2.0f;
						bloomInverseResolution.y *= 2.0f;
						bloomInverseResolution.z = 0.0f;
						bloomInverseResolution.w = 4.0f; // sampled mip level

						 //BloomScreenTexture = renderDestBloomMip4;
						renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderDestManager.renderDestBloomMip[ 4 ]->GetColorImage() );

						parms[ 0 ] = _bloomThreshold;
						parms[ 1 ] = 0.0;
						parms[ 2 ] = 0.0;
						parms[ 3 ] = _bloomStreakLength;

						Downsample( renderDestManager.renderDestBloomMip[ 5 ] );

						/////////////////////////////
						// UPSAMPLE TO MIP4

						bloomInverseResolution.w = 5.0f; // sampled mip level

						//BloomScreenTexture = renderDestBloomMip5;
						renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderDestManager.renderDestBloomMip[ 5 ]->GetColorImage() );

						parms[ 0 ] = 0.0;
						parms[ 1 ] = _bloomRadius5 * _radiusMultiplier;
						parms[ 2 ] = _bloomStrength5;
						parms[ 3 ] = _bloomStreakLength;

						Upsample( renderDestManager.renderDestBloomMip[ 4 ] );

						bloomInverseResolution.x /= 2.0;
						bloomInverseResolution.y /= 2.0;
					}

					// UPSAMPLE TO MIP3

					bloomInverseResolution.w = 4.0f; // sampled mip level

					//BloomScreenTexture = renderDestBloomMip4;
					renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderDestManager.renderDestBloomMip[ 4 ]->GetColorImage() );

					parms[ 0 ] = 0.0;
					parms[ 1 ] = _bloomRadius4 * _radiusMultiplier;
					parms[ 2 ] = _bloomStrength4;
					parms[ 3 ] = _bloomStreakLength;

					Upsample( renderDestManager.renderDestBloomMip[ 3 ] );

					bloomInverseResolution.x /= 2.0;
					bloomInverseResolution.y /= 2.0;
					bloomInverseResolution.w = 2.0f; // sampled mip level
				}

				// UPSAMPLE TO MIP2

				bloomInverseResolution.w = 3.0f; // sampled mip level

				 //BloomScreenTexture = renderDestBloomMip3;
				renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderDestManager.renderDestBloomMip[ 3 ]->GetColorImage() );

				parms[ 0 ] = 0.0;
				parms[ 1 ] = _bloomRadius3 * _radiusMultiplier;
				parms[ 2 ] = _bloomStrength3;
				parms[ 3 ] = _bloomStreakLength;

				Upsample( renderDestManager.renderDestBloomMip[ 2 ] );

				bloomInverseResolution.x /= 2.0;
				bloomInverseResolution.y /= 2.0;
			}

			// UPSAMPLE TO MIP1

			bloomInverseResolution.w = 2.0f; // sampled mip level

			//BloomScreenTexture = renderDestBloomMip2;
			renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderDestManager.renderDestBloomMip[ 2 ]->GetColorImage() );

			parms[ 0 ] = 0.0;
			parms[ 1 ] = _bloomRadius2 * _radiusMultiplier;
			parms[ 2 ] = _bloomStrength2;
			parms[ 3 ] = _bloomStreakLength;

			Upsample( renderDestManager.renderDestBloomMip[ 1 ] );

			bloomInverseResolution.x /= 2.0;
			bloomInverseResolution.y /= 2.0;
		}

		// UPSAMPLE TO MIP0 and
		// add filtered glare back to main context

		///GL_SetNativeRenderDestination();
		RB_ResetBaseRenderDest( r_useHDR.GetBool() );
		GL_State( GLS_BLEND_ADD | GLS_DISABLE_DEPTHTEST | GLS_TWOSIDED );
		RB_ResetViewportAndScissorToDefaultCamera( backEnd.viewDef );

		bloomInverseResolution.w = 1.0f; // sampled mip level

		//BloomScreenTexture = renderDestBloomMip1;
		renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderDestManager.renderDestBloomMip[ 1 ]->GetColorImage() );

		parms[ 0 ] = 0.0;
		parms[ 1 ] = _bloomRadius1 * _radiusMultiplier;
		parms[ 2 ] = _bloomStrength1;
		parms[ 3 ] = _bloomStreakLength;

		GL_SetRenderProgram( renderProgManager.prog_bloomUpscaleBlur );
		GL_DrawScreenTriangleAuto();

		///Upsample( renderDestManager.renderDestBloomMip[ 0 ] );
		// Note the final step could be done as a blend to the final texture.
	}

	GL_ResetTextureState();
	GL_ResetProgramState();
}








#if 0
/*
=========================================================================================================
									HDR AUTOEXPOSURE D4
=========================================================================================================
*/

idCVar r_hdrAutoExposureSpeed( "r_hdrAutoExposureSpeed", "1", CVAR_RENDERER | CVAR_INTEGER, "0 = disable, 1.0 = 16.5 ms (default), 2.0 = 8.25 ms, etc", 0.0, 6.0 );
idCVar r_hdrAutoExposureBase( "r_hdrAutoExposureBase", "0.01", CVAR_RENDERER | CVAR_FLOAT, "0.01 (default). Set base exposure adjustment. Helpful for screenshots and fine tunning automated exposure behavior." );
idCVar r_hdrAutoExposureRatio( "r_hdrAutoExposureRatio", "0", CVAR_RENDERER | CVAR_BOOL, "1.0 = use full adjustment. 0 = use base exposure adjustment. Helpful for screenshots and fine tunning automated exposure behavior." );
// r_hdrShutterSpeed  "Set camera shutter speed. Default = 10 (1/10 th of a second)."
// r_hdrDebug "0 = Disabled, 1 = Non - tonemapped gamma corrected output, 2 = debug view( red = nans, green = negative, grayscale = valid ), 3 = range debug( red = above max range, blue = bellow min range ), 4 = Auto Exposure + Bloom debug"
// r_hdrDebugRangeMin  "Min range used for r_hdrDebug mode 3"
// r_hdrDebugRangeMax  "Max range used for r_hdrDebug mode 3"

void DoDownSample( const idRenderDestination * dest )
{
	const idDeclRenderProg * progDownSampling;

	GL_SetRenderDestination( dest );
	GL_ViewportAndScissor( 0, 0, dest->GetTargetWidth(), dest->GetTargetHeight() );
	GL_State( GLS_TWOSIDED | GLS_DISABLE_BLENDING | GLS_DISABLE_DEPTHTEST );
	GL_SetRenderProgram( progDownSampling );
	GL_DrawScreenTriangleAuto();
}

void RB_AutoExposurePass() // D4
{
	const idDeclRenderProg * progDownSampling;
	const idRenderDestination * renderDestDownscaled[8];
	idRenderVector resolutionScale( 1.0, 1.0, 1.0, 1.0 );
	idRenderVector renderPositionToViewTexture;
	idRenderVector downsampleType;
	idRenderVector texSize;
	idImage * inputImage;
	static uint64 currLumTarget = 0;

	// Init 1280, 800
	int renderWidth = 1280;
	int renderHeight = 800;

	// Common for auto exposure and bloom passes
	{ // Dowscale 1/2		640, 400
		inputImage = GL_GetCurrentRenderDestination()->GetColorImage();

		float currWidth = renderDestDownscaled[ 0 ]->GetTargetWidth();
		float currHeight = renderDestDownscaled[ 0 ]->GetTargetHeight();
		renderPositionToViewTexture.Set( 0.0, 0.0, 1.0f / currWidth, 1.0f / currHeight );

		downsampleType.Set( 0.0, 1.0, 0.15, 0.0 );

		texSize.Set( inputImage->GetUploadWidth(), inputImage->GetUploadHeight(), 1.0f / inputImage->GetUploadWidth(), 1.0f / inputImage->GetUploadHeight() );
		renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, inputImage );

		DoDownSample( renderDestDownscaled[ 0 ] );
	}

	// Luminance
	for( int i = 1; i < 7; i++ )
	{
		inputImage = renderDestDownscaled[ i - 1 ]->GetColorImage();

		int currWidth = renderDestDownscaled[ i ]->GetTargetWidth();
		int currHeight = renderDestDownscaled[ i ]->GetTargetHeight();
		renderPositionToViewTexture.Set( 0.0, 0.0, 1.0f / currWidth, 1.0f / currHeight );

		if( i == 1 ) // Luminance downscale type. First downsample
		{
			downsampleType.Set( 1.0, 1.0, 0.15, 0.0 );
		}
		else {
			downsampleType.Set( 0.0, 0.0, 0.15, 0.0 );
		}

		texSize.Set( inputImage->GetUploadWidth(), inputImage->GetUploadHeight(), 1.0f / inputImage->GetUploadWidth(), 1.0f / inputImage->GetUploadHeight() );
		renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, inputImage );

		DoDownSample( renderDestDownscaled[ i ] );

		if( currWidth == 2 ) {
			break;
		}
	}

	// Luminance downscale type. First downsample
	{ // Dowscale 1/4		128, 128
		inputImage = renderDestDownscaled[ 0 ]->GetColorImage();

		float currWidth = renderDestDownscaled[ 1 ]->GetTargetWidth();
		float currHeight = renderDestDownscaled[ 1 ]->GetTargetHeight();
		renderPositionToViewTexture.Set( 0.0, 0.0, 1.0f / currWidth, 1.0f / currHeight );

		downsampleType.Set( 1.0, 1.0, 0.15, 0.0 );

		texSize.Set( inputImage->GetUploadWidth(), inputImage->GetUploadHeight(), 1.0f / inputImage->GetUploadWidth(), 1.0f / inputImage->GetUploadHeight() );
		renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, inputImage );

		DoDownSample( renderDestDownscaled[ 1 ] );
	}

	{ // Dowscale 1/8		64, 64
		inputImage = renderDestDownscaled[ 1 ]->GetColorImage();

		float currWidth = renderDestDownscaled[ 2 ]->GetTargetWidth();
		float currHeight = renderDestDownscaled[ 2 ]->GetTargetHeight();
		renderPositionToViewTexture.Set( 0.0, 0.0, 1.0f / currWidth, 1.0f / currHeight );

		downsampleType.Set( 0.0, 0.0, 0.15, 0.0 );

		texSize.Set( inputImage->GetUploadWidth(), inputImage->GetUploadHeight(), 1.0f / inputImage->GetUploadWidth(), 1.0f / inputImage->GetUploadHeight() );
		renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, inputImage );

		DoDownSample( renderDestDownscaled[ 2 ] );
	}

	{ // Dowscale 1/16		32, 32
		inputImage = renderDestDownscaled[ 2 ]->GetColorImage();

		float currWidth = renderDestDownscaled[ 3 ]->GetTargetWidth();
		float currHeight = renderDestDownscaled[ 3 ]->GetTargetHeight();
		renderPositionToViewTexture.Set( 0.0, 0.0, 1.0f / currWidth, 1.0f / currHeight );

		downsampleType.Set( 0.0, 0.0, 0.15, 0.0 );

		texSize.Set( inputImage->GetUploadWidth(), inputImage->GetUploadHeight(), 1.0f / inputImage->GetUploadWidth(), 1.0f / inputImage->GetUploadHeight() );
		renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, inputImage );

		DoDownSample( renderDestDownscaled[ 3 ] );
	}

	{ // Dowscale 1/16		16, 16
		inputImage = renderDestDownscaled[ 3 ]->GetColorImage();

		float currWidth = renderDestDownscaled[ 4 ]->GetTargetWidth();
		float currHeight = renderDestDownscaled[ 4 ]->GetTargetHeight();
		renderPositionToViewTexture.Set( 0.0, 0.0, 1.0f / currWidth, 1.0f / currHeight );

		downsampleType.Set( 0.0, 0.0, 0.15, 0.0 );

		texSize.Set( inputImage->GetUploadWidth(), inputImage->GetUploadHeight(), 1.0f / inputImage->GetUploadWidth(), 1.0f / inputImage->GetUploadHeight() );
		renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, inputImage );

		DoDownSample( renderDestDownscaled[ 4 ] );
	}

	{ // Dowscale 1/32		8, 8
		inputImage = renderDestDownscaled[ 4 ]->GetColorImage();

		float currWidth = renderDestDownscaled[ 5 ]->GetTargetWidth();
		float currHeight = renderDestDownscaled[ 5 ]->GetTargetHeight();
		renderPositionToViewTexture.Set( 0.0, 0.0, 1.0f / currWidth, 1.0f / currHeight );

		downsampleType.Set( 0.0, 0.0, 0.15, 0.0 );

		texSize.Set( inputImage->GetUploadWidth(), inputImage->GetUploadHeight(), 1.0f / inputImage->GetUploadWidth(), 1.0f / inputImage->GetUploadHeight() );
		renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, inputImage );

		DoDownSample( renderDestDownscaled[ 5 ] );
	}

	{ // Dowscale 1/64		4, 4
		inputImage = renderDestDownscaled[ 5 ]->GetColorImage();

		float currWidth = renderDestDownscaled[ 6 ]->GetTargetWidth();
		float currHeight = renderDestDownscaled[ 6 ]->GetTargetHeight();
		renderPositionToViewTexture.Set( 0.0, 0.0, 1.0f / currWidth, 1.0f / currHeight );

		downsampleType.Set( 0.0, 0.0, 0.15, 0.0 );

		texSize.Set( inputImage->GetUploadWidth(), inputImage->GetUploadHeight(), 1.0f / inputImage->GetUploadWidth(), 1.0f / inputImage->GetUploadHeight() );
		renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, inputImage );

		DoDownSample( renderDestDownscaled[ 6 ] );
	}

	{ // Dowscale 1/128		2, 2	luma_curr
		inputImage = renderDestDownscaled[ 6 ]->GetColorImage();

		float currWidth = renderDestDownscaled[ 7 ]->GetTargetWidth();
		float currHeight = renderDestDownscaled[ 7 ]->GetTargetHeight();
		renderPositionToViewTexture.Set( 0.0, 0.0, 1.0f / currWidth, 1.0f / currHeight );

		downsampleType.Set( 0.0, 0.0, 0.15, 0.0 );

		texSize.Set( inputImage->GetUploadWidth(), inputImage->GetUploadHeight(), 1.0f / inputImage->GetUploadWidth(), 1.0f / inputImage->GetUploadHeight() );
		renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, inputImage );

		DoDownSample( renderDestDownscaled[ 7 ] );
	}


	const int lumTexSize = 2;
	//idRenderVector downsampleType( 0.0f, 0.0f, 0.15f, 0.0f );
	idRenderVector autoExposureParms( 0.01f, 1.0f, 0.0f, 0.221199f );
	idRenderVector autoExposureMinMaxParms( 6.0f, 32.0f, 1.0f, 0.4f );

	renderProgManager.SetRenderParm( RENDERPARM_USERVEC0, downsampleType );
	renderProgManager.SetRenderParm( RENDERPARM_USERVEC1, autoExposureParms );
	renderProgManager.SetRenderParm( RENDERPARM_USERVEC2, autoExposureMinMaxParms );

	renderProgManager.SetRenderParm( RENDERPARM_USERMAP0, renderDestManager.renderDestLuminance->GetColorImage() ); // luma_curr
	renderProgManager.SetRenderParm( RENDERPARM_USERMAP1, renderDestManager.renderDestAutoExposure[ 0+1 ]->GetColorImage() ); // prevAutoExposure

	GL_SetRenderDestination( renderDestManager.renderDestAutoExposure[ 0 ] );
	GL_ViewportAndScissor( 0, 0, renderDestManager.renderDestAutoExposure[ 0 ]->GetTargetWidth(), renderDestManager.renderDestAutoExposure[ 0 ]->GetTargetHeight() );

	GL_State( GLS_TWOSIDED | GLS_GREENMASK | GLS_BLUEMASK | GLS_ALPHAMASK | GLS_DISABLE_BLENDING | GLS_DISABLE_DEPTHTEST );

	GL_SetRenderProgram( renderProgManager.progAutoExposure );
	GL_DrawScreenTriangleAuto();

	currLumTarget = !currLumTarget;

	// output will be use in postprocess.prog
}
#endif



















/** Divides two integers and rounds up */
template <class T>
static FORCEINLINE T DivideAndRoundUp( T Dividend, T Divisor )
{
	return ( Dividend + Divisor - 1 ) / Divisor;
}