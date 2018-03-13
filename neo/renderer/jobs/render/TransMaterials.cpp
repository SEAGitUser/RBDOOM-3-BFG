
#include "precompiled.h"
#pragma hdrstop

#include "Render.h"

/*
=============================================================================================

	NON-INTERACTION SHADER PASSES

=============================================================================================
*/

struct transMaterialsRenderPassParms_t 
{
	const idDeclRenderProg *	prog_gui;

	const idDeclRenderProg *	prog_bink;
	const idDeclRenderProg *	prog_bink_gui;

	const idDeclRenderProg *	prog_color;
	const idDeclRenderProg *	prog_color_skinned;

	const idDeclRenderProg *	prog_vertex_color;

	const idDeclRenderProg *	prog_texture;
	const idDeclRenderProg *	prog_texture_color;
	const idDeclRenderProg *	prog_texture_color_srgb;
	const idDeclRenderProg *	prog_texture_color_skinned;
	const idDeclRenderProg *	prog_texture_color_texgen;

	const idDeclRenderProg *	prog_environment;
	const idDeclRenderProg *	prog_environment_skinned;
	const idDeclRenderProg *	prog_bumpyenvironment;
	const idDeclRenderProg *	prog_bumpyenvironment_skinned;

	const idDeclRenderProg *	prog_skybox;
	const idDeclRenderProg *	prog_wobblesky;
	const idDeclRenderProg *	prog_screen;
};

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
			///GL_BindTexture( 0, renderImageManager->defaultImage );
			renderProgManager.SetRenderParm( RENDERPARM_MAP, renderImageManager->defaultImage );
			return;
		}

		// offset time by shaderParm[7] (FIXME: make the time offset a parameter of the shader?)
		// We make no attempt to optimize for multiple identical cinematics being in view, or
		// for cinematics going at a lower framerate than the renderer.
		cin = texture->cinematic->ImageForTime( backEnd.viewDef->GetGameTimeMS() + SEC2MS( backEnd.viewDef->GetGlobalMaterialParms()[ 11 ] ) );
		if( cin.imageY != NULL )
		{
			///GL_BindTexture( 0, cin.imageY );
			///GL_BindTexture( 1, cin.imageCr );
			///GL_BindTexture( 2, cin.imageCb );
			renderProgManager.SetRenderParm( RENDERPARM_MAPY,  cin.imageY );
			renderProgManager.SetRenderParm( RENDERPARM_MAPCR, cin.imageCr );
			renderProgManager.SetRenderParm( RENDERPARM_MAPCB, cin.imageCb );
		}
		else if( cin.image != NULL )
		{
			// Carl: A single RGB image works better with the FFMPEG BINK codec.
			///GL_BindTexture( 0, cin.image );
			renderProgManager.SetRenderParm( RENDERPARM_MAP, cin.image );
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
		else {
			///GL_BindTexture( 0, renderImageManager->blackImage );
			renderProgManager.SetRenderParm( RENDERPARM_MAP, renderImageManager->blackImage );
			// because the shaders may have already been set - we need to make sure we are not using a bink shader which would
			// display incorrectly.  We may want to get rid of RB_BindVariableStageImage and inline the code so that the
			// SWF GUI case is handled better, too
			renderProgManager.BindShader_TextureVertexColor( false );
		}
	}
	else {
		// FIXME: see why image is invalid
		if( texture->image != NULL )
		{
			///GL_BindTexture( 0, texture->image );
			renderProgManager.SetRenderParm( RENDERPARM_MAP, texture->image );
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
	auto & useTexGenParm = renderProgManager.GetRenderParm( RENDERPARM_TEXGEN_0_ENABLED );
	useTexGenParm.Set( 0.0f, 0.0f, 0.0f, 0.0f );

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
			///GL_BindTexture( 1, bumpStage->texture.image );
			renderProgManager.SetRenderParm( RENDERPARM_BUMPMAP, bumpStage->texture.image );

			GL_SelectTexture( 0 );

			RENDERLOG_PRINT( "TexGen: TG_REFLECT_CUBE: Bumpy Environment\n" );
			renderProgManager.BindShader_BumpyEnvironment( surf->jointCache );
		}
		else {
			RENDERLOG_PRINT( "TexGen: TG_REFLECT_CUBE: Environment\n" );
			renderProgManager.BindShader_Environment( surf->jointCache );
		}

		renderProgManager.SetRenderParm( RENDERPARM_ENVIROCUBEMAP, pStage->texture.image );
	}
	else if( pStage->texture.texgen == TG_SKYBOX_CUBE )
	{
		renderProgManager.BindShader_SkyBox();

		renderProgManager.SetRenderParm( RENDERPARM_CUBEMAP, pStage->texture.image );
	}
	else if( pStage->texture.texgen == TG_WOBBLESKY_CUBE )
	{
		const int * parms = surf->material->GetTexGenRegisters();

		const float wobbleDegrees = surf->shaderRegisters[ parms[ 0 ] ] * ( idMath::PI / 180.0f );
		const float wobbleSpeed   = surf->shaderRegisters[ parms[ 1 ] ] * ( 2.0f * idMath::PI / 60.0f );
		const float rotateSpeed   = surf->shaderRegisters[ parms[ 2 ] ] * ( 2.0f * idMath::PI / 60.0f );

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

		ALIGN16( float transform[ 12 ] );
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

		renderProgManager.SetRenderParm( RENDERPARM_CUBEMAP, pStage->texture.image );

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

		RENDERLOG_PRINT( "TexGen : %s\n", ( pStage->texture.texgen == TG_SCREEN )? "TG_SCREEN" : "TG_SCREEN2" );
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
		else {
			// per-pixel reflection mapping without bump mapping
		}
		renderProgManager.Unbind();
	}
}

void GetTextureMatrix( const float* shaderRegisters, const stageTextureMatrix_t & texture, float matrix[ 16 ] )
{
	matrix[ 0 * 4 + 0 ] = shaderRegisters[ texture.matrix[ 0 ][ 0 ] ];
	matrix[ 1 * 4 + 0 ] = shaderRegisters[ texture.matrix[ 0 ][ 1 ] ];
	matrix[ 2 * 4 + 0 ] = 0.0f;
	matrix[ 3 * 4 + 0 ] = shaderRegisters[ texture.matrix[ 0 ][ 2 ] ];

	matrix[ 0 * 4 + 1 ] = shaderRegisters[ texture.matrix[ 1 ][ 0 ] ];
	matrix[ 1 * 4 + 1 ] = shaderRegisters[ texture.matrix[ 1 ][ 1 ] ];
	matrix[ 2 * 4 + 1 ] = 0.0f;
	matrix[ 3 * 4 + 1 ] = shaderRegisters[ texture.matrix[ 1 ][ 2 ] ];

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
=====================
 RB_DrawShaderPasses

	Draw non-light dependent passes

	If we are rendering Guis, the drawSurf_t::sort value is a depth offset that can
	be multiplied by guiEye for polarity and screenSeparation for scale.
=====================
*/
int RB_DrawTransMaterialPasses( const drawSurf_t* const* const drawSurfs, const int numDrawSurfs, const float guiStereoScreenOffset, const int stereoEye )
{
	// only obey skipAmbient if we are rendering a 3d view
	if( !backEnd.viewDef->Is2DView() && r_skipAmbient.GetBool() )
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

			renderProgManager.SetRenderParms( RENDERPARM_MODELMATRIX_X, space->modelMatrix.Ptr(), 4 );
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

				{
					auto prog = declManager->RenderProgByIndex( newStage->program, false );
					release_assert( prog != NULL );

					renderProgManager.BindRenderProgram( prog );
				}

				// megaTextures bind a lot of images and set a lot of parameters
				/*if( newStage->megaTexture )
				{
					newStage->megaTexture->SetMappingForSurface( tri );
					idVec3	localViewer;
					R_GlobalPointToLocal( surf->space->modelMatrix, backEnd.viewDef->renderView.vieworg, localViewer );
					newStage->megaTexture->BindForViewOrigin( localViewer );
				}*/

				// bind texture units
				for( uint32 j = 0; j < newStage->numTextures; j++ )
				{
					auto & parm = newStage->textures[ j ];
					release_assert( parm.image != NULL )

					///GL_BindTexture( j, image );
					///renderProgManager.SetRenderParm( ( renderParm_t )( RENDERPARM_USERMAP0 + j ), image );
					parm.parmDecl->Set( parm.image );
				}
				// set vector parms
				for( uint32 j = 0; j < newStage->numVectors; j++ )
				{
					auto & parm = newStage->vectors[ j ];
					auto & vec = parm.parmDecl->GetVec4();

					vec[ 0 ] = regs[ parm.registers[ 0 ] ];
					vec[ 1 ] = regs[ parm.registers[ 1 ] ];
					vec[ 2 ] = regs[ parm.registers[ 2 ] ];
					vec[ 3 ] = regs[ parm.registers[ 3 ] ];

					///auto & parm = renderProgManager.GetRenderParm( ( renderParm_t )( RENDERPARM_USERVEC0 + j ) );
					///parm[ 0 ] = regs[ newStage->vertexParms[ j ][ 0 ] ];
					///parm[ 1 ] = regs[ newStage->vertexParms[ j ][ 1 ] ];
					///parm[ 2 ] = regs[ newStage->vertexParms[ j ][ 2 ] ];
					///parm[ 3 ] = regs[ newStage->vertexParms[ j ][ 3 ] ];
				}
				// set texture matrices
				/*for( uint32 j = 0; j < newStage->numTextureMatrices; j++ )
				{
					auto & parm = newStage->textureMatrices[ j ];

					idRenderVector & texS = parm.parmDecl_s->GetVec4();
					idRenderVector & texT = parm.parmDecl_t->GetVec4();

					ALIGNTYPE16 float matrix[ 16 ];
					GetTextureMatrix( regs, parm, matrix );

					texS[ 0 ] = matrix[ 0 * 4 + 0 ];
					texS[ 1 ] = matrix[ 1 * 4 + 0 ];
					texS[ 2 ] = matrix[ 2 * 4 + 0 ];
					texS[ 3 ] = matrix[ 3 * 4 + 0 ];

					texT[ 0 ] = matrix[ 0 * 4 + 1 ];
					texT[ 1 ] = matrix[ 1 * 4 + 1 ];
					texT[ 2 ] = matrix[ 2 * 4 + 1 ];
					texT[ 3 ] = matrix[ 3 * 4 + 1 ];

					RENDERLOG_INDENT();
					RENDERLOG_PRINT( "TexMatS : %4.3f, %4.3f, %4.3f, %4.3f\n", texS[ 0 ], texS[ 1 ], texS[ 2 ], texS[ 3 ] );
					RENDERLOG_PRINT( "TexMatT : %4.3f, %4.3f, %4.3f, %4.3f\n", texT[ 0 ], texT[ 1 ], texT[ 2 ], texT[ 3 ] );
					RENDERLOG_OUTDENT();

					///renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_S, texS.ToFloatPtr() );
					///renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_T, texT.ToFloatPtr() );
				}*/

				// set rpEnableSkinning if the shader has optional support for skinning
				if( surf->jointCache && renderProgManager.ShaderHasOptionalSkinning() )
				{
					auto & skinningParm = renderProgManager.GetRenderParm( RENDERPARM_ENABLE_SKINNING );
					_mm_store_ps( skinningParm.ToFloatPtr(), idMath::SIMD_SP_one );
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
					auto & skinningParm = renderProgManager.GetRenderParm( RENDERPARM_ENABLE_SKINNING );
					_mm_store_ps( skinningParm.ToFloatPtr(), _mm_setzero_ps() );
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

			// draw it ------------
			GL_DrawIndexed( surf ); //SEA: img?
			// --------------------

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
