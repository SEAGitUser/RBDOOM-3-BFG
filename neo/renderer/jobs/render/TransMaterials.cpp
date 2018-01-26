
#include "precompiled.h"
#pragma hdrstop

#include "Render.h"

/*
=============================================================================================

	NON-INTERACTION SHADER PASSES

=============================================================================================
*/

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
			///renderImageManager->defaultImage->Bind();
			GL_BindTexture( 0, renderImageManager->defaultImage );
			return;
		}

		// offset time by shaderParm[7] (FIXME: make the time offset a parameter of the shader?)
		// We make no attempt to optimize for multiple identical cinematics being in view, or
		// for cinematics going at a lower framerate than the renderer.
		cin = texture->cinematic->ImageForTime( backEnd.viewDef->GetGameTimeMS() + SEC2MS( backEnd.viewDef->GetGlobalMaterialParms()[ 11 ] ) );
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
			else {
			renderProgManager.BindShader_TextureVertexColor();
			}
			*/
		}
		else
		{
			///renderImageManager->blackImage->Bind();
			GL_BindTexture( 0, renderImageManager->blackImage );
			// because the shaders may have already been set - we need to make sure we are not using a bink shader which would
			// display incorrectly.  We may want to get rid of RB_BindVariableStageImage and inline the code so that the
			// SWF GUI case is handled better, too
			renderProgManager.BindShader_TextureVertexColor( false );
		}
	}
	else
	{
		// FIXME: see why image is invalid
		if( texture->image != NULL )
		{
			///texture->image->Bind();
			GL_BindTexture( 0, texture->image );
		}
	}
}

/*
================
 RB_PrepareStageTexturing
================
*/
void RB_PrepareStageTexturing( const materialStage_t* pStage, const drawSurf_t* surf )
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

			RENDERLOG_PRINT( "TexGen: TG_REFLECT_CUBE: Bumpy Environment\n" );
			renderProgManager.BindShader_BumpyEnvironment( surf->jointCache );
		}
		else
		{
			RENDERLOG_PRINT( "TexGen: TG_REFLECT_CUBE: Environment\n" );
			renderProgManager.BindShader_Environment( surf->jointCache );
		}

	}
	else if( pStage->texture.texgen == TG_SKYBOX_CUBE )
	{

		renderProgManager.BindShader_SkyBox();

	}
	else if( pStage->texture.texgen == TG_WOBBLESKY_CUBE )
	{

		const int* parms = surf->material->GetTexGenRegisters();

		float wobbleDegrees = surf->shaderRegisters[ parms[ 0 ] ] * ( idMath::PI / 180.0f );
		float wobbleSpeed = surf->shaderRegisters[ parms[ 1 ] ] * ( 2.0f * idMath::PI / 60.0f );
		float rotateSpeed = surf->shaderRegisters[ parms[ 2 ] ] * ( 2.0f * idMath::PI / 60.0f );

		idVec3 axis[ 3 ];
		{
			// very ad-hoc "wobble" transform
			float s, c;
			idMath::SinCos( wobbleSpeed * backEnd.viewDef->GetGameTimeSec(), s, c );

			float ws, wc;
			idMath::SinCos( wobbleDegrees, ws, wc );

			axis[ 2 ][ 0 ] = ws * c;
			axis[ 2 ][ 1 ] = ws * s;
			axis[ 2 ][ 2 ] = wc;

			axis[ 1 ][ 0 ] = -s * s * ws;
			axis[ 1 ][ 2 ] = -s * ws * ws;
			axis[ 1 ][ 1 ] = idMath::Sqrt( idMath::Fabs( 1.0f - ( axis[ 1 ][ 0 ] * axis[ 1 ][ 0 ] + axis[ 1 ][ 2 ] * axis[ 1 ][ 2 ] ) ) );

			// make the second vector exactly perpendicular to the first
			axis[ 1 ] -= ( axis[ 2 ] * axis[ 1 ] ) * axis[ 2 ];
			axis[ 1 ].Normalize();

			// construct the third with a cross
			axis[ 0 ].Cross( axis[ 1 ], axis[ 2 ] );
		}

		// add the rotate
		float rs, rc;
		idMath::SinCos( rotateSpeed * backEnd.viewDef->GetGameTimeSec(), rs, rc );

		float transform[ 12 ];
		transform[ 0 * 4 + 0 ] = axis[ 0 ][ 0 ] * rc + axis[ 1 ][ 0 ] * rs;
		transform[ 0 * 4 + 1 ] = axis[ 0 ][ 1 ] * rc + axis[ 1 ][ 1 ] * rs;
		transform[ 0 * 4 + 2 ] = axis[ 0 ][ 2 ] * rc + axis[ 1 ][ 2 ] * rs;
		transform[ 0 * 4 + 3 ] = 0.0f;

		transform[ 1 * 4 + 0 ] = axis[ 1 ][ 0 ] * rc - axis[ 0 ][ 0 ] * rs;
		transform[ 1 * 4 + 1 ] = axis[ 1 ][ 1 ] * rc - axis[ 0 ][ 1 ] * rs;
		transform[ 1 * 4 + 2 ] = axis[ 1 ][ 2 ] * rc - axis[ 0 ][ 2 ] * rs;
		transform[ 1 * 4 + 3 ] = 0.0f;

		transform[ 2 * 4 + 0 ] = axis[ 2 ][ 0 ];
		transform[ 2 * 4 + 1 ] = axis[ 2 ][ 1 ];
		transform[ 2 * 4 + 2 ] = axis[ 2 ][ 2 ];
		transform[ 2 * 4 + 3 ] = 0.0f;

		renderProgManager.SetRenderParms( RENDERPARM_WOBBLESKY_X, transform, 3 );
		renderProgManager.BindShader_WobbleSky();

	}
	else if( ( pStage->texture.texgen == TG_SCREEN ) || ( pStage->texture.texgen == TG_SCREEN2 ) )
	{

		useTexGenParm[ 0 ] = 1.0f;
		useTexGenParm[ 1 ] = 1.0f;
		useTexGenParm[ 2 ] = 1.0f;
		useTexGenParm[ 3 ] = 1.0f;

		idRenderMatrix mat;
		idRenderMatrix::Multiply( backEnd.viewDef->GetProjectionMatrix(), surf->space->modelViewMatrix, mat );

		RENDERLOG_PRINT( "TexGen : %s\n", ( pStage->texture.texgen == TG_SCREEN ) ? "TG_SCREEN" : "TG_SCREEN2" );
		RENDERLOG_INDENT();

		renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_S, mat[ 0 ] );
		renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_T, mat[ 1 ] );
		renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_Q, mat[ 3 ] );

		RENDERLOG_PRINT( "TEXGEN_S = %4.3f, %4.3f, %4.3f, %4.3f\n", mat[ 0 ][ 0 ], mat[ 0 ][ 1 ], mat[ 0 ][ 2 ], mat[ 0 ][ 3 ] );
		RENDERLOG_PRINT( "TEXGEN_T = %4.3f, %4.3f, %4.3f, %4.3f\n", mat[ 1 ][ 0 ], mat[ 1 ][ 1 ], mat[ 1 ][ 2 ], mat[ 1 ][ 3 ] );
		RENDERLOG_PRINT( "TEXGEN_Q = %4.3f, %4.3f, %4.3f, %4.3f\n", mat[ 3 ][ 0 ], mat[ 3 ][ 1 ], mat[ 3 ][ 2 ], mat[ 3 ][ 3 ] );

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

	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_ENABLED, useTexGenParm.ToFloatPtr() );
}

/*
================
 RB_FinishStageTexturing
================
*/
void RB_FinishStageTexturing( const materialStage_t* pStage, const drawSurf_t* surf )
{

	if( pStage->texture.cinematic )
	{
		// unbind the extra bink textures
		GL_ResetTexturesState();
	}

	if( pStage->texture.texgen == TG_REFLECT_CUBE )
	{
		// see if there is also a bump map specified
		auto bumpStage = surf->material->GetBumpStage();
		if( bumpStage != NULL )
		{
			// per-pixel reflection mapping with bump mapping
			GL_ResetTexturesState();
		}
		else
		{
			// per-pixel reflection mapping without bump mapping
		}
		renderProgManager.Unbind();
	}
}

/*
=====================
 RB_DrawShaderPasses

	Draw non-light dependent passes

	If we are rendering Guis, the drawSurf_t::sort value is a depth offset that can
	be multiplied by guiEye for polarity and screenSeparation for scale.
=====================
*/
int RB_DrawTransMaterialPasses( const drawSurf_t* const* const drawSurfs, const int numDrawSurfs, const float guiStereoScreenOffset, const int stereoEye )
{
	// only obey skipAmbient if we are rendering a view
	if( backEnd.viewDef->viewEntitys && r_skipAmbient.GetBool() )
	{
		return numDrawSurfs;
	}

	RENDERLOG_OPEN_BLOCK( "RB_DrawTransMaterialPasses" );

	GL_ResetTexturesState();

	backEnd.ClearCurrentSpace();
	float currentGuiStereoOffset = 0.0f;

	int i = 0;
	for(/**/; i < numDrawSurfs; ++i )
	{
		auto const surf = drawSurfs[ i ];
		auto const shader = surf->material;

		if( !shader->HasAmbient() ) {
			continue;
		}

		if( shader->IsPortalSky() ) {
			continue;
		}

		// some deforms may disable themselves by setting numIndexes = 0
		if( surf->numIndexes == 0 ) {
			continue;
		}

		if( shader->SuppressInSubview() ) {
			continue;
		}

		if( backEnd.viewDef->isXraySubview && surf->space->entityDef )
		{
			if( surf->space->entityDef->GetParms().xrayIndex != 2 ) { //SEA: calling entityDef here is very bad !!!
				continue;
			}
		}

		// we need to draw the post process shaders after we have drawn the fog lights
		if( shader->GetSort() >= SS_POST_PROCESS && !backEnd.currentRenderCopied ) {
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
			else {
				RB_SetMVP( space->mvp );
			}

			// set model Matrix
			renderProgManager.SetRenderParms( RENDERPARM_MODELMATRIX_X, space->modelMatrix.Ptr(), 4 );

			// Set ModelView Matrix
			renderProgManager.SetRenderParms( RENDERPARM_MODELVIEWMATRIX_X, space->modelViewMatrix.Ptr(), 4 );

			// set eye position in local space
			idRenderVector localViewOrigin( 1.0f );
			space->modelMatrix.InverseTransformPoint( backEnd.viewDef->GetOrigin(), localViewOrigin.ToVec3() );
			renderProgManager.SetRenderParm( RENDERPARM_LOCALVIEWORIGIN, localViewOrigin.ToFloatPtr() );
		}

		// change the scissor if needed
		RB_SetScissor( surf->scissorRect );

		// get the expressions for conditionals / color / texcoords
		const float* regs = surf->shaderRegisters;

		// set face culling appropriately
		GL_Cull( surf->space->isGuiSurface ? CT_TWO_SIDED : shader->GetCullType() );

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
			if( pStage->SkipStage( regs ) )
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
				// otherwise it will result in an out of bounds crash in GL_DrawIndexed
				renderProgManager.BindShader( newStage->glslProgram, newStage->vertexProgram, -1, newStage->fragmentProgram, false );
				// RB end

				for( int j = 0; j < newStage->numVertexParms; j++ )
				{
					///idRenderVector parm;
					///parm[ 0 ] = regs[ newStage->vertexParms[ j ][ 0 ] ];
					///parm[ 1 ] = regs[ newStage->vertexParms[ j ][ 1 ] ];
					///parm[ 2 ] = regs[ newStage->vertexParms[ j ][ 2 ] ];
					///parm[ 3 ] = regs[ newStage->vertexParms[ j ][ 3 ] ];
					///renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USER + j ), parm.ToFloatPtr() );

					auto & parm = renderProgManager.GetRenderParm( ( renderParm_t )( RENDERPARM_USER + j ) );

					parm[ 0 ] = regs[ newStage->vertexParms[ j ][ 0 ] ];
					parm[ 1 ] = regs[ newStage->vertexParms[ j ][ 1 ] ];
					parm[ 2 ] = regs[ newStage->vertexParms[ j ][ 2 ] ];
					parm[ 3 ] = regs[ newStage->vertexParms[ j ][ 3 ] ];
				}

				// set rpEnableSkinning if the shader has optional support for skinning
				if( surf->jointCache && renderProgManager.ShaderHasOptionalSkinning() )
				{
					const idRenderVector skinningParm( 1.0f );
					renderProgManager.SetRenderParm( RENDERPARM_ENABLE_SKINNING, skinningParm.ToFloatPtr() );
				}

				// bind texture units
				for( int j = 0; j < newStage->numFragmentProgramImages; j++ )
				{
					idImage* image = newStage->fragmentProgramImages[ j ];
					if( image != NULL )
					{
						GL_BindTexture( j, image );
					}
				}

				// draw it
				GL_DrawIndexed( surf );

				// unbind texture units
				/*for( int j = 0; j < newStage->numFragmentProgramImages; j++ )
				{
					idImage* image = newStage->fragmentProgramImages[ j ];
					if( image != NULL )
					{
						GL_SelectTexture( j );
						renderImageManager->BindNull();
					}
				}*/
				GL_ResetTexturesState();

				// clear rpEnableSkinning if it was set
				if( surf->jointCache && renderProgManager.ShaderHasOptionalSkinning() )
				{
					const idRenderVector skinningParm( 0.0f );
					renderProgManager.SetRenderParm( RENDERPARM_ENABLE_SKINNING, skinningParm.ToFloatPtr() );
				}

				GL_SelectTexture( 0 );
				//GL_ResetProgramState();

				RENDERLOG_CLOSE_BLOCK();
				continue;
			}

			//--------------------------
			//
			// old style stages
			//
			//--------------------------

			// set the color
			idRenderVector color;
			pStage->GetColorParm( regs, color );

			// skip the entire stage if an add would be black
			if( ( stageGLState & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) == ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE )
				&& color[ 0 ] <= 0 && color[ 1 ] <= 0 && color[ 2 ] <= 0 )
			{
				continue;
			}

			// skip the entire stage if a blend would be completely transparent
			if( ( stageGLState & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) == ( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA )
				&& color[ 3 ] <= 0 )
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
					else {
						renderProgManager.BindShader_TextureVertexColor( false );
					}
				}
				else {
					if( ( stageGLState & GLS_OVERRIDE ) != 0 )
					{
						// This is a hack... Only SWF Guis set GLS_OVERRIDE
						// Old style guis do not, and we don't want them to use the new GUI renderProg
						renderProgManager.BindShader_GUI();
					}
					else {
						if( backEnd.viewDef->is2Dgui )
						{
							// RB: 2D fullscreen drawing like warp or damage blend effects
							renderProgManager.BindShader_TextureVertexColor_sRGB();
						}
						else {
							renderProgManager.BindShader_TextureVertexColor( surf->jointCache );
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
			else {
				renderProgManager.BindShader_TextureVertexColor( surf->jointCache );
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
			GL_DrawIndexed( surf );

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

	GL_ResetTexturesState();
	GL_ResetProgramState();

	RENDERLOG_CLOSE_BLOCK();
	return i;
}
