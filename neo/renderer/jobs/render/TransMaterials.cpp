
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

	bool	skipDynamicTextures;
	bool	skipNewTransMaterials;
	bool	skipTransMaterials;
	bool	stereoRender_swapEyes;

	float	guiStereoScreenOffset;
	int		stereoEye;
};

/*
================
RB_SetMVPWithStereoOffset
================
*/
ID_INLINE void SetMVPWithStereoOffset( const idRenderMatrix& mvp, const float stereoOffset )
{
	idRenderMatrix offset = mvp;
	offset[ 0 ][ 3 ] += stereoOffset;
	renderProgManager.SetMVPMatrixParms( offset );
}

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
		if( r_skipDynamicTextures.GetBool() )
		{
			renderProgManager.SetRenderParm( RENDERPARM_MAP, renderImageManager->defaultImage );
			return;
		}

		// offset time by shaderParm[7] (FIXME: make the time offset a parameter of the shader?)
		// We make no attempt to optimize for multiple identical cinematics being in view, or
		// for cinematics going at a lower framerate than the renderer.
		auto cin = texture->cinematic->ImageForTime( backEnd.viewDef->GetGameTimeMS() + SEC2MS( backEnd.viewDef->GetGlobalMaterialParms()[ 11 ] ) );
		if( cin.imageY != NULL )
		{
			renderProgManager.SetRenderParm( RENDERPARM_MAPY,  cin.imageY );
			renderProgManager.SetRenderParm( RENDERPARM_MAPCR, cin.imageCr );
			renderProgManager.SetRenderParm( RENDERPARM_MAPCB, cin.imageCb );
		}
		else if( cin.image != NULL )
		{
			// Carl: A single RGB image works better with the FFMPEG BINK codec.
			renderProgManager.SetRenderParm( RENDERPARM_MAP, cin.image );
		/*
			if( backEnd.viewDef->is2Dgui )
			{
			GL_SetRenderProgram( renderProgManager.prog_texture_color_srgb );
			}
			else {
			GL_SetRenderProgram( renderProgManager.prog_texture_color );
			}
		*/
		}
		else {
			renderProgManager.SetRenderParm( RENDERPARM_MAP, renderImageManager->blackImage );
			// because the shaders may have already been set - we need to make sure we are not using a bink shader which would
			// display incorrectly.  We may want to get rid of RB_BindVariableStageImage and inline the code so that the
			// SWF GUI case is handled better, too
			GL_SetRenderProgram( renderProgManager.prog_texture_color );
		}
	}
	else {
		// FIXME: see why image is invalid
		if( texture->image != NULL )
		{
			renderProgManager.SetRenderParm( RENDERPARM_MAP, texture->image );
		}
	}
}

ID_INLINE uint64 GetCullState( cullType_t cull )
{
	return ( cull == cullType_t::CT_TWO_SIDED )? GLS_TWOSIDED : ( cull == cullType_t::CT_BACK_SIDED )? GLS_BACKSIDED : GLS_FRONTSIDED;
}

idCVar r_useWobble2( "r_useWobble2", "1", CVAR_RENDERER | CVAR_BOOL, "use new wobble render program" );

/*
================
 RB_PrepareStageTexturing
================
*/
void RB_PrepareStageTexturing( const materialStage_t* pStage, const drawSurf_t* surf )
{
	renderProgManager.DisableTexgen0Parm();

	// set the texture matrix if needed
	renderProgManager.SetupTextureMatrixParms( surf->shaderRegisters, &pStage->texture );

	// texgens
	if( pStage->texture.texgen == TG_REFLECT_CUBE )
	{
		// see if there is also a bump map specified
		auto bumpStage = surf->material->GetBumpStage();
		if( bumpStage != NULL )
		{
			// per-pixel reflection mapping with bump mapping
			renderProgManager.SetRenderParm( RENDERPARM_BUMPMAP, bumpStage->texture.image );

			//GL_SelectTexture( 0 );

			RENDERLOG_PRINT( "TexGen: TG_REFLECT_CUBE: Bumpy Environment\n" );
			GL_SetRenderProgram( surf->HasSkinning() ? renderProgManager.prog_bumpyenvironment_skinned : renderProgManager.prog_bumpyenvironment );
		}
		else {
			RENDERLOG_PRINT( "TexGen: TG_REFLECT_CUBE: Environment\n" );
			GL_SetRenderProgram( surf->HasSkinning() ? renderProgManager.prog_environment_skinned : renderProgManager.prog_environment );
		}

		renderProgManager.SetRenderParm( RENDERPARM_ENVIROCUBEMAP, pStage->texture.image );
	}
	else if( pStage->texture.texgen == TG_SKYBOX_CUBE )
	{
		GL_SetRenderProgram( renderProgManager.prog_skybox );

		renderProgManager.SetRenderParm( RENDERPARM_CUBEMAP, pStage->texture.image );
	}
	else if( pStage->texture.texgen == TG_WOBBLESKY_CUBE )
	{
		const int * parms = surf->material->GetTexGenRegisters();

		if( r_useWobble2.GetBool() )
		{
			auto WobbleParms = renderProgManager.GetRenderParm( RENDERPARM_USERVEC0 )->GetVecPtr();
			WobbleParms[ 0 ] = surf->shaderRegisters[ parms[ 0 ] ];
			WobbleParms[ 1 ] = surf->shaderRegisters[ parms[ 1 ] ];
			WobbleParms[ 2 ] = surf->shaderRegisters[ parms[ 2 ] ];
			WobbleParms[ 3 ] = backEnd.viewDef->GetGameTimeSec();

			renderProgManager.SetRenderParm( RENDERPARM_CUBEMAP, pStage->texture.image );

			GL_SetRenderProgram( renderProgManager.prog_wobblesky2 );
		}
		else {
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

			GL_SetRenderProgram( renderProgManager.prog_wobblesky );
		}
	}
	else if( ( pStage->texture.texgen == TG_SCREEN ) || ( pStage->texture.texgen == TG_SCREEN2 ) )
	{
		renderProgManager.EnableTexgen0Parm();

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
		GL_ResetTextureState();
	}

	if( pStage->texture.texgen == TG_REFLECT_CUBE )
	{
		// see if there is also a bump map specified
		auto bumpStage = surf->material->GetBumpStage();
		if( bumpStage != NULL )
		{
			// per-pixel reflection mapping with bump mapping
			GL_ResetTextureState();
		}
		else {
			// per-pixel reflection mapping without bump mapping
		}
		GL_ResetProgramState();
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
	if( matrix[ 3 * 4 + 0 ] < -40.0f || matrix[ 12 ] > 40.0f ) matrix[ 3 * 4 + 0 ] -= ( int )matrix[ 3 * 4 + 0 ];
	if( matrix[ 13 ] < -40.0f || matrix[ 13 ] > 40.0f ) matrix[ 13 ] -= ( int )matrix[ 13 ];

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
int RB_DrawTransMaterialPasses( const drawSurf_t* const* const drawSurfs, const int numDrawSurfs,
								const float guiStereoScreenOffset, const int stereoEye )
{
	// only obey skipAmbient if we are rendering a 3d view
	if( !backEnd.viewDef->Is2DView() && r_skipAmbient.GetBool() ) {
		return numDrawSurfs;
	}

	RENDERLOG_OPEN_BLOCK( "RB_DrawTransMaterialPasses" );

	const bool useHDR = r_useHDR.GetBool() && !backEnd.viewDef->is2Dgui;

	RB_ResetBaseRenderDest( useHDR );
	RB_ResetViewportAndScissorToDefaultCamera( backEnd.viewDef );

	// Normal face culling CT_FRONT_SIDED.
	// Ensures that depth writes are enabled for the depth clear.
	GL_State( GLS_DEFAULT | GLS_FRONTSIDED );

	if( backEnd.viewDef->Is2DView() || backEnd.viewDef->is2Dgui )
	{
		// Clear the depth buffer and clear the stencil to 128 for stencil shadows as well as gui masking
		GL_Clear( false, true, true, STENCIL_SHADOW_TEST_VALUE, 0.0f, 0.0f, 0.0f, 0.0f );
	}

	backEnd.ClearCurrentSpace();
	float currentGuiStereoOffset = 0.0f;

	int i = 0;
	for(/**/; i < numDrawSurfs; ++i )
	{
		auto const surf = drawSurfs[ i ];
		auto const material = surf->material;

		if( !material->HasAmbient() ) {
			continue;
		}
		if( material->IsPortalSky() ) {
			continue;
		}
		if( material->SuppressInSubview() ) {
			continue;
		}
		// we need to draw the post process shaders after we have drawn the fog lights
		if( material->GetSort() >= SS_POST_PROCESS && !backEnd.currentRenderCopied ) {
			break;
		}

		// some deforms may disable themselves by setting numIndexes = 0
		if( surf->numIndexes == 0 ) {
			continue;
		}
		if( backEnd.viewDef->isXraySubview && surf->space->entityDef ) {
			if( surf->space->entityDef->GetParms().xrayIndex != 2 )
			{ //SEA: calling entityDef here is very bad !!!
				continue;
			}
		}

		// if we are rendering a 3D view and the surface's eye index doesn't match
		// the current view's eye index then we skip the surface
		// if the stereoEye value of a surface is 0 then we need to draw it for both eyes.
		const int shaderStereoEye = material->GetStereoEye();
		const bool isEyeValid = stereoRender_swapEyes.GetBool() ? ( shaderStereoEye == stereoEye ) : ( shaderStereoEye != stereoEye );
		if( ( stereoEye != 0 ) && ( shaderStereoEye != 0 ) && ( isEyeValid ) )
		{
			continue;
		}

		RENDERLOG_OPEN_BLOCK( material->GetName() );

		// determine the stereoDepth offset
		// guiStereoScreenOffset will always be zero for 3D views, so the !=
		// check will never force an update due to the current sort value.
		const float thisGuiStereoOffset = guiStereoScreenOffset * surf->sort;

		// change the matrix and other space related vars if needed
		if( surf->space != backEnd.currentSpace || thisGuiStereoOffset != currentGuiStereoOffset )
		{
			backEnd.currentSpace = surf->space;
			currentGuiStereoOffset = thisGuiStereoOffset;

			const viewModel_t * const space = backEnd.currentSpace;

			if( guiStereoScreenOffset != 0.0f )
			{
				SetMVPWithStereoOffset( space->mvp, currentGuiStereoOffset );
			}
			else {
				renderProgManager.SetMVPMatrixParms( space->mvp );
			}

			renderProgManager.SetModelMatrixParms( space->modelMatrix );
			renderProgManager.SetModelViewMatrixParms( space->modelViewMatrix );

			// set eye position in local space
			idRenderVector localViewOrigin( 1.0f );
			space->modelMatrix.InverseTransformPoint( backEnd.viewDef->GetOrigin(), localViewOrigin.ToVec3() );
			renderProgManager.SetRenderParm( RENDERPARM_LOCALVIEWORIGIN, localViewOrigin );
		}

		// change the scissor if needed
		RB_SetScissor( surf->scissorRect );

		uint64 surfGLState = surf->extraGLState;

		// set face culling appropriately
		///GL_Cull( surf->space->isGuiSurface ? CT_TWO_SIDED : material->GetCullType() );
		surfGLState |= ( surf->space->isGuiSurface ? GLS_TWOSIDED : GetCullState( material->GetCullType() ) );

		// set polygon offset if necessary
		if( material->TestMaterialFlag( MF_POLYGONOFFSET ) )
		{
			GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * material->GetPolygonOffset() );
			surfGLState = GLS_POLYGON_OFFSET;
		}

		// get the expressions for conditionals / color / texcoords
		const float* regs = surf->shaderRegisters;

		for( int stage = 0; stage < material->GetNumStages(); stage++ )
		{
			auto pStage = material->GetStage( stage );

			// check the enable condition
			assert( regs != NULL );
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
					assert( prog != NULL );

					GL_SetRenderProgram( prog );
				}

				// bind texture units
				for( uint32 j = 0; j < newStage->numTextures; j++ )
				{
					auto & parm = newStage->textures[ j ];
					assert( parm.image != NULL );

					parm.parmDecl->Set( parm.image );
				}

				// set vector parms
				for( uint32 j = 0; j < newStage->numVectors; j++ )
				{
					auto & parm = newStage->vectors[ j ];

				#if USE_INTRINSICS
					assert_16_byte_aligned( parm.parmDecl->GetVecPtr() );
					_mm_store_ps( parm.parmDecl->GetVecPtr(),
						_mm_set_ps(
							regs[ parm.registers[ 3 ] ], regs[ parm.registers[ 2 ] ], regs[ parm.registers[ 1 ] ], regs[ parm.registers[ 0 ] ]
						)
					);
				#else
					parm.parmDecl->GetVecPtr()[ 0 ] = regs[ parm.registers[ 0 ] ];
					parm.parmDecl->GetVecPtr()[ 1 ] = regs[ parm.registers[ 1 ] ];
					parm.parmDecl->GetVecPtr()[ 2 ] = regs[ parm.registers[ 2 ] ];
					parm.parmDecl->GetVecPtr()[ 3 ] = regs[ parm.registers[ 3 ] ];
				#endif
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
				/*if( surf->jointCache && renderProgManager.ShaderHasOptionalSkinning() )
				{
					GL_EnableSkinning();
				}*/

				// draw it
				GL_DrawIndexed( surf );

				GL_ResetTextureState();
				///GL_ResetProgramState();

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

			auto svc = pStage->vertexColor;

			RENDERLOG_OPEN_BLOCK( "Old Shader Stage" );

			renderProgManager.SetColorParm( color );

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
						GL_SetRenderProgram( renderProgManager.prog_texture_color_srgb );
					}
					else {
						GL_SetRenderProgram( renderProgManager.prog_texture_color );
						///GL_EnableAlphaTest( pStage->GetAlphaTestValue( regs ) );//SEA
					}
				}
				else {
					if( ( stageGLState & GLS_OVERRIDE ) != 0 )
					{
						// This is a hack... Only SWF Guis set GLS_OVERRIDE
						// Old style guis do not, and we don't want them to use the new GUI renderProg
						GL_SetRenderProgram( renderProgManager.prog_gui );
					}
					else {
						if( backEnd.viewDef->is2Dgui )
						{
							// RB: 2D fullscreen drawing like warp or damage blend effects
							GL_SetRenderProgram( renderProgManager.prog_texture_color_srgb );
						}
						else {
							GL_SetRenderProgram( surf->HasSkinning() ? renderProgManager.prog_texture_color_skinned : renderProgManager.prog_texture_color );
							///GL_EnableAlphaTest( pStage->GetAlphaTestValue( regs ) );//SEA
						}
					}
				}
			}
			else if( ( pStage->texture.texgen == TG_SCREEN ) || ( pStage->texture.texgen == TG_SCREEN2 ) )
			{
				GL_SetRenderProgram( renderProgManager.prog_texture_color_texgen );
			}
			else if( pStage->texture.cinematic )
			{
				GL_SetRenderProgram( renderProgManager.prog_bink );
			}
			else {
				GL_SetRenderProgram( surf->HasSkinning() ? renderProgManager.prog_texture_color_skinned : renderProgManager.prog_texture_color );
				///GL_EnableAlphaTest( pStage->GetAlphaTestValue( regs ) );//SEA
			}

			renderProgManager.SetVertexColorParm( svc );

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
			//
			GL_DrawIndexed( surf ); //SEA: img?
			//
			// --------------------

			RB_FinishStageTexturing( pStage, surf );

			// unset privatePolygonOffset if necessary
			if( pStage->privatePolygonOffset )
			{
				GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * material->GetPolygonOffset() );
			}

			///GL_DisableAlphaTest();//SEA

			RENDERLOG_CLOSE_BLOCK();
		}

		RENDERLOG_CLOSE_BLOCK();
	}

	renderProgManager.SetColorParm( 1.0f, 1.0f, 1.0f, 1.0f );

	// disable stencil shadow test
	///GL_State( GLS_DEFAULT | GLS_FRONTSIDED );
	///GL_Cull( CT_FRONT_SIDED );

	GL_ResetTextureState();
	GL_ResetProgramState();

	RENDERLOG_CLOSE_BLOCK();
	return i;
}






#if 0
void DrawSyrface( const drawSurf_t * const drawSurf, const float guiStereoScreenOffset, float & currentGuiStereoOffset )
{
	auto const material = drawSurf->material;
	const float * const materialRegisters = drawSurf->shaderRegisters;

	RENDERLOG_OPEN_BLOCK( material->GetName() );

	// determine the stereoDepth offset
	// guiStereoScreenOffset will always be zero for 3D views, so the !=
	// check will never force an update due to the current sort value.
	const float thisGuiStereoOffset = guiStereoScreenOffset * drawSurf->sort;

	// change the matrix and other space related vars if needed
	if( drawSurf->space != backEnd.currentSpace || thisGuiStereoOffset != currentGuiStereoOffset )
	{
		backEnd.currentSpace = drawSurf->space;
		currentGuiStereoOffset = thisGuiStereoOffset;

		const viewModel_t* space = backEnd.currentSpace;

		if( guiStereoScreenOffset != 0.0f )
		{
			SetMVPWithStereoOffset( space->mvp, currentGuiStereoOffset );
		}
		else {
			renderProgManager.SetMVPMatrixParms( space->mvp );
		}

		renderProgManager.SetModelMatrixParms( space->modelMatrix );
		renderProgManager.SetModelViewMatrixParms( space->modelViewMatrix );

		// set eye position in local space
		idRenderVector localViewOrigin( 1.0f );
		space->modelMatrix.InverseTransformPoint( backEnd.viewDef->GetOrigin(), localViewOrigin.ToVec3() );
		renderProgManager.SetRenderParm( RENDERPARM_LOCALVIEWORIGIN, localViewOrigin );
	}

	// change the scissor if needed
	RB_SetScissor( drawSurf->scissorRect );

	// set face culling appropriately
	//GL_Cull( drawSurf->space->isGuiSurface ? CT_TWO_SIDED : material->GetCullType() );

	uint64 surfGLState = drawSurf->extraGLState;

	// set polygon offset if necessary
	if( material->TestMaterialFlag( MF_POLYGONOFFSET ) )
	{
		GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * material->GetPolygonOffset() );
		surfGLState = GLS_POLYGON_OFFSET;
	}

	/*
	------------------------------------------------------------------------
	 Iterate through each material stage and draw surface whith it.
	------------------------------------------------------------------------
	*/
	for( int stageNum = 0; stageNum < material->GetNumStages(); stageNum++ )
	{
		auto stage = material->GetStage( stageNum );

		// check the enable condition
		assert( materialRegisters != NULL );
		if( stage->SkipStage( materialRegisters ) )
		{
			continue;
		}

		// skip the stages involved in lighting
		if( stage->lighting != SL_AMBIENT )
		{
			continue;
		}

		uint64 stageGLState = surfGLState;
		if( ( surfGLState & GLS_OVERRIDE ) == 0 )
		{
			stageGLState |= stage->drawStateBits;
		}

		// skip if the stage is ( GL_ZERO, GL_ONE ), which is used for some alpha masks
		if( ( stageGLState & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) == ( GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE ) )
		{
			continue;
		}

		//--------------------------
		//
		// old style stages
		//
		//--------------------------

		// set the color
		idRenderVector color;
		stage->GetColorParm( materialRegisters, color );

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

		RENDERLOG_OPEN_BLOCK( "Old Material Stage" );
		{
			if( drawSurf->space->isGuiSurface )
			{
				// Force gui surfaces to always be SVC_MODULATE

				// use special shaders for bink cinematics
				if( stage->texture.cinematic )
				{
					if( ( stageGLState & GLS_OVERRIDE ) != 0 )
					{
						// This is a hack... Only SWF Guis set GLS_OVERRIDE
						// Old style guis do not, and we don't want them to use the new GUI renederProg
						GL_SetRenderProgram( renderProgManager.prog_texture_color_srgb );
					}
					else {
						GL_SetRenderProgram( renderProgManager.prog_texture_color );
					}

					auto cin = stage->texture.cinematic->ImageForTime( backEnd.viewDef->GetGameTimeMS() + SEC2MS( backEnd.viewDef->GetGlobalMaterialParms()[ 11 ] ) );
					if( cin.image != NULL )
					{
						// Carl: A single RGB image works better with the FFMPEG BINK codec.
						renderProgManager.SetRenderParm( RENDERPARM_MAP, cin.image );
					}

					renderProgManager.SetColorParm( color );
					renderProgManager.DisableTexgen0Parm();
					renderProgManager.EnableAlphaTestParm( stage->GetAlphaTestValue( materialRegisters ) );
					renderProgManager.SetVertexColorParm( SVC_MODULATE );
					renderProgManager.SetupTextureMatrixParms( materialRegisters, &stage->texture );
				}
				else
				{
					if( ( stageGLState & GLS_OVERRIDE ) != 0 )
					{
						// This is a hack... Only SWF Guis set GLS_OVERRIDE
						// Old style guis do not, and we don't want them to use the new GUI renderProg.
						GL_SetRenderProgram( renderProgManager.prog_gui );
						// No parms here.
					}
					else
					{
						if( backEnd.viewDef->is2Dgui )
						{
							// RB: 2D fullscreen drawing like warp or damage blend effects
							GL_SetRenderProgram( renderProgManager.prog_texture_color_srgb );
						}
						else {
							GL_SetRenderProgram( drawSurf->HasSkinning() ? renderProgManager.prog_texture_color_skinned : renderProgManager.prog_texture_color );
						}

						renderProgManager.SetColorParm( color );
						renderProgManager.DisableTexgen0Parm();
						renderProgManager.EnableAlphaTestParm( stage->GetAlphaTestValue( materialRegisters ) );
						renderProgManager.SetVertexColorParm( SVC_MODULATE );
						renderProgManager.SetupTextureMatrixParms( materialRegisters, &stage->texture );
					}

					// FIXME: see why image is invalid
					if( stage->texture.image != NULL )
					{
						renderProgManager.SetRenderParm( RENDERPARM_MAP, stage->texture.image );
					}
				}
			}
			else if( ( stage->texture.texgen == TG_SCREEN ) || ( stage->texture.texgen == TG_SCREEN2 ) )
			{
				GL_SetRenderProgram( renderProgManager.prog_texture_color_texgen );

				renderProgManager.SetColorParm( color );
				renderProgManager.SetVertexColorParm( stage->vertexColor );
				renderProgManager.EnableTexgen0Parm();
				renderProgManager.SetupTextureMatrixParms( materialRegisters, &stage->texture );

				idRenderMatrix mat;
				idRenderMatrix::Multiply( backEnd.viewDef->GetProjectionMatrix(), drawSurf->space->modelViewMatrix, mat );

				renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_S, mat[ 0 ] );
				renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_T, mat[ 1 ] );
				renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_Q, mat[ 3 ] );

				RENDERLOG_PRINT( "TexGen : %s\n", ( stage->texture.texgen == TG_SCREEN ) ? "TG_SCREEN" : "TG_SCREEN2" );
				RENDERLOG_INDENT();
				RENDERLOG_PRINT( "TEXGEN_S = %4.3f, %4.3f, %4.3f, %4.3f\n", mat[ 0 ][ 0 ], mat[ 0 ][ 1 ], mat[ 0 ][ 2 ], mat[ 0 ][ 3 ] );
				RENDERLOG_PRINT( "TEXGEN_T = %4.3f, %4.3f, %4.3f, %4.3f\n", mat[ 1 ][ 0 ], mat[ 1 ][ 1 ], mat[ 1 ][ 2 ], mat[ 1 ][ 3 ] );
				RENDERLOG_PRINT( "TEXGEN_Q = %4.3f, %4.3f, %4.3f, %4.3f\n", mat[ 3 ][ 0 ], mat[ 3 ][ 1 ], mat[ 3 ][ 2 ], mat[ 3 ][ 3 ] );
				RENDERLOG_OUTDENT();

				// FIXME: see why image is invalid
				if( stage->texture.image != NULL )
				{
					renderProgManager.SetRenderParm( RENDERPARM_MAP, stage->texture.image );
				}
			}
			else if( stage->texture.texgen == TG_REFLECT_CUBE )
			{
				// see if there is also a bump map specified
				auto bumpStage = material->GetBumpStage();
				if( bumpStage != NULL )
				{
					// per-pixel reflection mapping with bump mapping
					renderProgManager.SetRenderParm( RENDERPARM_BUMPMAP, bumpStage->texture.image );

					GL_SetRenderProgram( drawSurf->HasSkinning() ? renderProgManager.prog_bumpyenvironment_skinned : renderProgManager.prog_bumpyenvironment );
				}
				else {
					GL_SetRenderProgram( drawSurf->HasSkinning() ? renderProgManager.prog_environment_skinned : renderProgManager.prog_environment );
				}

				renderProgManager.SetRenderParm( RENDERPARM_ENVIROCUBEMAP, stage->texture.image );

				renderProgManager.SetColorParm( color );

			}
			else if( stage->texture.texgen == TG_SKYBOX_CUBE )
			{
				GL_SetRenderProgram( renderProgManager.prog_skybox );

				renderProgManager.SetRenderParm( RENDERPARM_CUBEMAP, stage->texture.image );

				renderProgManager.SetVertexColorParm( stage->vertexColor );
				//?TBD renderProgManager.SetColorParm( color );
			}
			else if( stage->texture.texgen == TG_WOBBLESKY_CUBE )
			{
				const int * parms = material->GetTexGenRegisters();

				const float wobbleDegrees = materialRegisters[ parms[ 0 ] ] * ( idMath::PI / 180.0f );
				const float wobbleSpeed   = materialRegisters[ parms[ 1 ] ] * ( 2.0f * idMath::PI / 60.0f );
				const float rotateSpeed   = materialRegisters[ parms[ 2 ] ] * ( 2.0f * idMath::PI / 60.0f );

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

				renderProgManager.SetRenderParm( RENDERPARM_WOBBLESKY_X, transform + 0 * 4 );
				renderProgManager.SetRenderParm( RENDERPARM_WOBBLESKY_Y, transform + 1 * 4 );
				renderProgManager.SetRenderParm( RENDERPARM_WOBBLESKY_Z, transform + 2 * 4 );

				renderProgManager.SetRenderParm( RENDERPARM_CUBEMAP, stage->texture.image );

				renderProgManager.SetVertexColorParm( stage->vertexColor );
				//?TBD renderProgManager.SetColorParm( color );

				GL_SetRenderProgram( renderProgManager.prog_wobblesky );
			}
			else if( stage->texture.texgen == TG_DIFFUSE_CUBE )
			{
				// As far as I can tell, this is never used
				idLib::Error( "Using Diffuse Cube! Please contact Brian!" );
			}
			else if( stage->texture.texgen == TG_GLASSWARP )
			{
				// As far as I can tell, this is never used
				idLib::Error( "Using GlassWarp! Please contact Brian!" );
			}
			else if( stage->texture.cinematic )
			{
				GL_SetRenderProgram( renderProgManager.prog_bink );

				auto cin = stage->texture.cinematic->ImageForTime( backEnd.viewDef->GetGameTimeMS() + SEC2MS( backEnd.viewDef->GetGlobalMaterialParms()[ 11 ] ) );
				if( cin.imageY != NULL )
				{
					renderProgManager.SetRenderParm( RENDERPARM_MAPY, cin.imageY );
					renderProgManager.SetRenderParm( RENDERPARM_MAPCR, cin.imageCr );
					renderProgManager.SetRenderParm( RENDERPARM_MAPCB, cin.imageCb );
				}

				renderProgManager.SetColorParm( color );
			}
			else {
				GL_SetRenderProgram( drawSurf->HasSkinning() ? renderProgManager.prog_texture_color_skinned : renderProgManager.prog_texture_color );

				renderProgManager.SetColorParm( color );
				renderProgManager.DisableTexgen0Parm();
				renderProgManager.EnableAlphaTestParm( stage->GetAlphaTestValue( materialRegisters ) );
				renderProgManager.SetVertexColorParm( stage->vertexColor );
				renderProgManager.SetupTextureMatrixParms( materialRegisters, &stage->texture );
			}

			// set privatePolygonOffset if necessary
			if( stage->privatePolygonOffset )
			{
				GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * stage->privatePolygonOffset );
				stageGLState |= GLS_POLYGON_OFFSET;
			}

			// set the state
			GL_State( stageGLState );

			// ----------------------------------------

			GL_DrawIndexed( drawSurf );

			// ----------------------------------------

			if( stage->texture.cinematic )
			{
				// unbind the extra bink textures
				GL_ResetTextureState();
			}

			if( stage->texture.texgen == TG_REFLECT_CUBE )
			{
				// see if there is also a bump map specified
				auto bumpStage = material->GetBumpStage();
				if( bumpStage != NULL )
				{
					// per-pixel reflection mapping with bump mapping
					GL_ResetTextureState();
				}
				else
				{
					// per-pixel reflection mapping without bump mapping
				}
				GL_ResetProgramState();
			}

			// unset privatePolygonOffset if necessary
			if( stage->privatePolygonOffset )
			{
				GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * material->GetPolygonOffset() );
			}
		}
		RENDERLOG_CLOSE_BLOCK();
	}
}
#endif