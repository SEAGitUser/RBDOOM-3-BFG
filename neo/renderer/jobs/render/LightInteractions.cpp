
#include "precompiled.h"
#pragma hdrstop

#include "Render.h"
/*
struct drawInteraction_t
{
	const drawSurf_t * 			surf;

	const idDeclRenderParm *	bumpImage;
	const idDeclRenderParm *	diffuseImage;
	const idDeclRenderParm *	specularImage;

	const idDeclRenderParm *	diffuseColor;	// may have a light color baked into it
	const idDeclRenderParm *	specularColor;	// may have a light color baked into it
	const idDeclRenderParm *	vertexColor;	// applies to both diffuse and specular

	// these are loaded into the vertex program
	const idDeclRenderParm *	bumpMatrix[ 2 ];
	const idDeclRenderParm *	diffuseMatrix[ 2 ];
	const idDeclRenderParm *	specularMatrix[ 2 ];

	int ambientLight;	// use tr.ambientNormalMap instead of normalization cube map

	drawInteraction_t()
	{
		bumpImage = renderProgManager.GetRenderParm( RENDERPARM_BUMPMAP );
		diffuseImage = renderProgManager.GetRenderParm( RENDERPARM_DIFFUSEMAP );
		specularImage = renderProgManager.GetRenderParm( RENDERPARM_SPECULARMAP );

		// may have a light color baked into it

		diffuseColor = renderProgManager.GetRenderParm( RENDERPARM_DIFFUSEMODIFIER );
		specularColor = renderProgManager.GetRenderParm( RENDERPARM_SPECULARMODIFIER );

		// applies to both diffuse and specular
		vertexColor = renderProgManager.GetRenderParm( RENDERPARM_VERTEXCOLOR_MAD );

		// these are loaded into the vertex program

		bumpMatrix[ 0 ] = renderProgManager.GetRenderParm( RENDERPARM_BUMPMATRIX_S );
		bumpMatrix[ 1 ] = renderProgManager.GetRenderParm( RENDERPARM_BUMPMATRIX_T );

		diffuseMatrix[ 0 ] = renderProgManager.GetRenderParm( RENDERPARM_DIFFUSEMATRIX_S );
		diffuseMatrix[ 1 ] = renderProgManager.GetRenderParm( RENDERPARM_DIFFUSEMATRIX_T );

		specularMatrix[ 0 ] = renderProgManager.GetRenderParm( RENDERPARM_SPECULARMATRIX_S );
		specularMatrix[ 1 ] = renderProgManager.GetRenderParm( RENDERPARM_SPECULARMATRIX_T );
	}

	void Clear()
	{
		bumpImage->Set( ( idImage * )NULL );
		diffuseImage->Set( ( idImage * )NULL );
		specularImage->Set( ( idImage * )NULL );

		diffuseColor->Set( 1.0f );
		specularColor->Set( 0.0f );

		vertexColor->Set( 0, 1, 0, 0 ); //  SVC_IGNORE;

		ambientLight = 0;
	}
};*/

static void RB_SetupInteractionStage( const materialStage_t* surfaceStage, const float* surfaceRegs, const idDeclRenderParm * matrix[ 2 ], float color[ 4 ] )
{
	auto & matrix0 = matrix[ 0 ]->GetVec4();
	auto & matrix1 = matrix[ 1 ]->GetVec4();

	if( surfaceStage->texture.hasMatrix )
	{
		matrix0[ 0 ] = surfaceRegs[ surfaceStage->texture.matrix[ 0 ][ 0 ] ];
		matrix0[ 1 ] = surfaceRegs[ surfaceStage->texture.matrix[ 0 ][ 1 ] ];
		matrix0[ 2 ] = 0.0f;
		matrix0[ 3 ] = surfaceRegs[ surfaceStage->texture.matrix[ 0 ][ 2 ] ];

		matrix1[ 0 ] = surfaceRegs[ surfaceStage->texture.matrix[ 1 ][ 0 ] ];
		matrix1[ 1 ] = surfaceRegs[ surfaceStage->texture.matrix[ 1 ][ 1 ] ];
		matrix1[ 2 ] = 0.0f;
		matrix1[ 3 ] = surfaceRegs[ surfaceStage->texture.matrix[ 1 ][ 2 ] ];

		// we attempt to keep scrolls from generating incredibly large texture values, but
		// center rotations and center scales can still generate offsets that need to be > 1
		if( matrix0[ 3 ] < -40.0f || matrix0[ 3 ] > 40.0f ) matrix0[ 3 ] -= idMath::Ftoi( matrix0[ 3 ] );
		if( matrix1[ 3 ] < -40.0f || matrix1[ 3 ] > 40.0f ) matrix1[ 3 ] -= idMath::Ftoi( matrix1[ 3 ] );
	}
	else {
		matrix0[ 0 ] = 1.0f;
		matrix0[ 1 ] = 0.0f;
		matrix0[ 2 ] = 0.0f;
		matrix0[ 3 ] = 0.0f;

		matrix1[ 0 ] = 0.0f;
		matrix1[ 1 ] = 1.0f;
		matrix1[ 2 ] = 0.0f;
		matrix1[ 3 ] = 0.0f;
	}

	if( color != NULL )
	{
		// clamp here, so cards with a greater range don't look different.
		// we could perform overbrighting like we do for lights, but
		// it doesn't currently look worth it.
		color[ 0 ] = idMath::ClampFloat( 0.0f, 1.0f, surfaceRegs[ surfaceStage->color.registers[ 0 ] ] );
		color[ 1 ] = idMath::ClampFloat( 0.0f, 1.0f, surfaceRegs[ surfaceStage->color.registers[ 1 ] ] );
		color[ 2 ] = idMath::ClampFloat( 0.0f, 1.0f, surfaceRegs[ surfaceStage->color.registers[ 2 ] ] );
		color[ 3 ] = idMath::ClampFloat( 0.0f, 1.0f, surfaceRegs[ surfaceStage->color.registers[ 3 ] ] );
	}
}

static void RB_DrawSingleInteraction( drawInteraction_t * din )
{
	if( din->bumpImage->GetImage() == NULL )
	{
		// stage wasn't actually an interaction
		return;
	}

	if( din->diffuseImage->GetImage() == NULL || r_skipDiffuse.GetBool() )
	{
		// this isn't a YCoCg black, but it doesn't matter, because
		// the diffuseColor will also be 0
		din->diffuseImage->Set( renderImageManager->blackImage );
	}
	if( din->specularImage->GetImage() == NULL || r_skipSpecular.GetBool() || din->ambientLight )
	{
		din->specularImage->Set( renderImageManager->blackImage );
	}
	if( r_skipBump.GetBool() )
	{
		din->bumpImage->Set( renderImageManager->flatNormalMap );
	}

	// if we wouldn't draw anything, don't call the Draw function
	const bool diffuseIsBlack = ( din->diffuseImage->GetImage() == renderImageManager->blackImage ) ||
		( ( din->diffuseColor->GetVec3()[ 0 ] <= 0 ) &&
		  ( din->diffuseColor->GetVec3()[ 1 ] <= 0 ) &&
		  ( din->diffuseColor->GetVec3()[ 2 ] <= 0 ) );

	const bool specularIsBlack = ( din->specularImage->GetImage() == renderImageManager->blackImage ) ||
		( ( din->specularColor->GetVec3()[ 0 ] <= 0 ) &&
		  ( din->specularColor->GetVec3()[ 1 ] <= 0 ) &&
		  ( din->specularColor->GetVec3()[ 2 ] <= 0 ) );

	if( diffuseIsBlack && specularIsBlack )
	{
		return;
	}

/*
	renderProgManager.SetRenderParm( RENDERPARM_BUMPMATRIX_S, din->bumpMatrix[ 0 ] );
	renderProgManager.SetRenderParm( RENDERPARM_BUMPMATRIX_T, din->bumpMatrix[ 1 ] );

	renderProgManager.SetRenderParm( RENDERPARM_DIFFUSEMATRIX_S, din->diffuseMatrix[ 0 ] );
	renderProgManager.SetRenderParm( RENDERPARM_DIFFUSEMATRIX_T, din->diffuseMatrix[ 1 ] );

	renderProgManager.SetRenderParm( RENDERPARM_SPECULARMATRIX_S, din->specularMatrix[ 0 ] );
	renderProgManager.SetRenderParm( RENDERPARM_SPECULARMATRIX_T, din->specularMatrix[ 1 ] );

	RB_SetVertexColorParm( din->vertexColor );

	renderProgManager.SetRenderParm( RENDERPARM_DIFFUSEMODIFIER, din->diffuseColor );
	renderProgManager.SetRenderParm( RENDERPARM_SPECULARMODIFIER, din->specularColor );

	renderProgManager.SetRenderParm( RENDERPARM_BUMPMAP, din->bumpImage ); // texture 0 will be the per-surface bump map
	renderProgManager.SetRenderParm( RENDERPARM_DIFFUSEMAP, din->diffuseImage ); // texture 3 is the per-surface diffuse map
	renderProgManager.SetRenderParm( RENDERPARM_SPECULARMAP, din->specularImage );  // texture 4 is the per-surface specular map
*/

	GL_DrawIndexed( din->surf );

	din->Clear();
}
/*
====================================================
 RB_DrawComplexMaterialInteraction

	go through the individual surface stages

	This is somewhat arcane because of the old support for video cards that had to render
	interactions in multiple passes.

	We also have the very rare case of some materials that have conditional interactions
	for the "hell writing" that can be shined on them.
====================================================
*/
void RB_DrawComplexMaterialInteraction( drawInteraction_t & inter, const float* surfaceRegs, const idMaterial* const material )
{
	RENDERLOG_OPEN_BLOCK( material->GetName() );

	for( int stageNum = 0; stageNum < material->GetNumStages(); stageNum++ )
	{
		auto const materialStage = material->GetStage( stageNum );

		switch( materialStage->lighting )
		{
			case SL_COVERAGE:
			{
				// ignore any coverage stages since they should only be used for the depth fill pass
				// for diffuse stages that use alpha test.
				break;
			}
			case SL_AMBIENT:
			{
				// ignore ambient stages while drawing interactions
				break;
			}
			case SL_BUMP:
			{
				// ignore stage that fails the condition
				if( materialStage->SkipStage( surfaceRegs ) )
				{
					break;
				}
				// draw any previous interaction
				if( inter.bumpImage->GetImage() != NULL )
				{
					RB_DrawSingleInteraction( &inter );
				}
				inter.bumpImage->Set( materialStage->texture.image );
				inter.diffuseImage->Set( ( idImage * )NULL );
				inter.specularImage->Set( ( idImage * )NULL );
				RB_SetupInteractionStage( materialStage, surfaceRegs, inter.bumpMatrix, NULL );
				break;
			}
			case SL_DIFFUSE:
			{
				// ignore stage that fails the condition
				if( materialStage->SkipStage( surfaceRegs ) )
				{
					break;
				}
				// draw any previous interaction
				if( inter.diffuseImage->GetImage() != NULL )
				{
					RB_DrawSingleInteraction( &inter );
				}
				inter.diffuseImage->Set( materialStage->texture.image );
				renderProgManager.SetVertexColorParm( materialStage->vertexColor );
				RB_SetupInteractionStage( materialStage, surfaceRegs, inter.diffuseMatrix, inter.diffuseColor->GetVecPtr() );
				break;
			}
			case SL_SPECULAR:
			{
				// ignore stage that fails the condition
				if( materialStage->SkipStage( surfaceRegs ) )
				{
					break;
				}
				// draw any previous interaction
				if( inter.specularImage->GetImage() != NULL )
				{
					RB_DrawSingleInteraction( &inter );
				}
				inter.specularImage->Set( materialStage->texture.image );
				renderProgManager.SetVertexColorParm( materialStage->vertexColor );
				RB_SetupInteractionStage( materialStage, surfaceRegs, inter.specularMatrix, inter.specularColor->GetVecPtr() );
				break;
			}
		}
	}

	// draw the final interaction
	RB_DrawSingleInteraction( &inter );

	RENDERLOG_CLOSE_BLOCK();
}

/*
=========================================================================================

	GENERAL INTERACTION RENDERING

=========================================================================================
*/

idCVar r_skipInteractionFastPath( "r_skipInteractionFastPath", "1", CVAR_RENDERER | CVAR_BOOL, "" );
idCVar r_useLightStencilSelect( "r_useLightStencilSelect", "0", CVAR_RENDERER | CVAR_BOOL, "use stencil select pass" );

struct forwardInteractionsRenderPassParms_t
{
	const idDeclRenderParm *	rpBumpMap;
	const idDeclRenderParm *	rpDiffuseMap;
	const idDeclRenderParm *	rpSpecularMap;
	const idDeclRenderParm *	rpLightProjectMap;
	const idDeclRenderParm *	rpLightFalloffMap;
	const idDeclRenderParm *	rpShadowBufferMap;
	const idDeclRenderParm *	rpJitterMap;

	const idDeclRenderParm *	rpDiffuseModifier;
	const idDeclRenderParm *	rpSpecularModifier;

	const idDeclRenderParm *	rpBumpMatrixS;
	const idDeclRenderParm *	rpBumpMatrixT;
	const idDeclRenderParm *	rpDiffuseMatrixS;
	const idDeclRenderParm *	rpDiffuseMatrixT;
	const idDeclRenderParm *	rpSpecularMatrixS;
	const idDeclRenderParm *	rpSpecularMatrixT;

	const idDeclRenderParm *	rpVertexColorMAD;
};

/*
=================
 BindProg_ForwardInteraction_ShadowMapping
=================
*/
ID_INLINE void BindProg_ForwardInteraction_ShadowMapping( lightType_e lighttype, const bool skinning )
{
	if( lighttype == LIGHT_TYPE_POINT )
	{
		GL_SetRenderProgram( skinning ? renderProgManager.prog_interaction_sm_point_skinned : renderProgManager.prog_interaction_sm_point );
	}
	else if( lighttype == LIGHT_TYPE_PARALLEL )
	{
		GL_SetRenderProgram( skinning ? renderProgManager.prog_interaction_sm_parallel_skinned : renderProgManager.prog_interaction_sm_parallel );
	}
	else if( lighttype == LIGHT_TYPE_SPOT )
	{
		GL_SetRenderProgram( skinning ? renderProgManager.prog_interaction_sm_spot_skinned : renderProgManager.prog_interaction_sm_spot );
	}
}

/*
=================
 RB_SetupForFastPathInteractions

	These are common for all fast path surfaces
=================
*/
void RB_SetupForFastPathInteractions()
{
	const idRenderVector sMatrix( 1.0f, 0.0f, 0.0f, 0.0f );
	const idRenderVector tMatrix( 0.0f, 1.0f, 0.0f, 0.0f );

	renderProgManager.SetRenderParm( RENDERPARM_BUMPMATRIX_S, sMatrix );
	renderProgManager.SetRenderParm( RENDERPARM_BUMPMATRIX_T, tMatrix );

	renderProgManager.SetRenderParm( RENDERPARM_DIFFUSEMATRIX_S, sMatrix );
	renderProgManager.SetRenderParm( RENDERPARM_DIFFUSEMATRIX_T, tMatrix );

	renderProgManager.SetRenderParm( RENDERPARM_SPECULARMATRIX_S, sMatrix );
	renderProgManager.SetRenderParm( RENDERPARM_SPECULARMATRIX_T, tMatrix );

	renderProgManager.SetVertexColorParm( SVC_IGNORE );

	renderProgManager.SetRenderParm( RENDERPARM_DIFFUSEMODIFIER, idColor::white.ToVec4() );
	renderProgManager.SetRenderParm( RENDERPARM_SPECULARMODIFIER, idColor::white.ToVec4() );
}

/*
=============
 RB_RenderInteractions

	With added sorting and trivial path work.
=============
*/
void RB_RenderInteractions( const drawSurf_t* const surfList, const viewLight_t* const vLight, int depthFunc, bool performStencilTest, bool useLightDepthBounds )
{
	if( surfList == NULL )
		return;

	// change the scissor if needed, it will be constant across all the surfaces lit by the light
	RB_SetScissor( vLight->scissorRect );

	// perform setup here that will be constant for all interactions
	if( performStencilTest )
	{
		GL_State( GLS_BLEND_ADD | GLS_DEPTHMASK | depthFunc | GLS_STENCIL_FUNC_EQUAL |
			GLS_STENCIL_MAKE_REF( STENCIL_SHADOW_TEST_VALUE ) | GLS_STENCIL_MAKE_MASK( STENCIL_SHADOW_MASK_VALUE ) );
	}
	else {
		GL_State( GLS_BLEND_ADD | GLS_DEPTHMASK | depthFunc | GLS_STENCIL_FUNC_ALWAYS );
	}

	// some rare lights have multiple animating stages, loop over them outside the surface list
	const idMaterial* const lightMaterial = vLight->lightShader;
	const float* lightRegs = vLight->shaderRegisters;

	drawInteraction_t inter;
	inter.ambientLight = lightMaterial->IsAmbientLight();

	//---------------------------------
	// Split out the complex surfaces from the fast-path surfaces
	// so we can do the fast path ones all in a row.
	// The surfaces should already be sorted by space because they
	// are added single-threaded, and there is only a negligable amount
	// of benefit to trying to sort by materials.
	//---------------------------------
	static const int MAX_INTERACTIONS_PER_LIGHT = 1024;
	static const int MAX_COMPLEX_INTERACTIONS_PER_LIGHT = 256;
	idStaticList< const drawSurf_t*, MAX_INTERACTIONS_PER_LIGHT > allSurfaces;
	idStaticList< const drawSurf_t*, MAX_COMPLEX_INTERACTIONS_PER_LIGHT > complexSurfaces;
	for( const drawSurf_t* walk = surfList; walk != NULL; walk = walk->nextOnLight )
	{
		// make sure the triangle culling is done
		if( walk->shadowVolumeState != SHADOWVOLUME_DONE )
		{
			assert( walk->shadowVolumeState == SHADOWVOLUME_UNFINISHED || walk->shadowVolumeState == SHADOWVOLUME_DONE );

			uint64 start = sys->Microseconds();
			while( walk->shadowVolumeState == SHADOWVOLUME_UNFINISHED )
			{
				Sys_Yield();
			}
			uint64 end = sys->Microseconds();

			backEnd.pc.shadowMicroSec += end - start;
		}

		const idMaterial* surfaceShader = walk->material;
		if( surfaceShader->GetFastPathBumpImage() )
		{
			allSurfaces.Append( walk );
		}
		else {
			complexSurfaces.Append( walk );
		}
	}
	for( int i = 0; i < complexSurfaces.Num(); ++i )
	{
		allSurfaces.Append( complexSurfaces[ i ] );
	}

	bool lightDepthBoundsDisabled = false;

	const uint32 shadowLOD = vLight->GetShadowLOD();
	const auto lightType = vLight->GetLightType();

	if( r_useShadowMapping.GetBool() && vLight->globalShadows )
	{
		RB_SetupShadowCommonParms( lightType, shadowLOD );
	}

	auto globalLightOrigin = renderProgManager.GetRenderParm( RENDERPARM_GLOBALLIGHTORIGIN )->GetVecPtr();
	globalLightOrigin[ 0 ] = vLight->globalLightOrigin[ 0 ];
	globalLightOrigin[ 1 ] = vLight->globalLightOrigin[ 1 ];
	globalLightOrigin[ 2 ] = vLight->globalLightOrigin[ 2 ];
	globalLightOrigin[ 3 ] = ( lightType == LIGHT_TYPE_PARALLEL )? -1.0f : ( lightType == LIGHT_TYPE_POINT ? 0.0f : 1.0f );

	// Set the light falloff texture
	renderProgManager.SetRenderParm( RENDERPARM_LIGHTFALLOFFMAP, vLight->falloffImage );

	const float lightScale = r_useHDR.GetBool() ? 2.6f : r_lightScale.GetFloat();

	for( int lightStageNum = 0; lightStageNum < lightMaterial->GetNumStages(); lightStageNum++ )
	{
		auto lightStage = lightMaterial->GetStage( lightStageNum );

		// ignore stages that fail the condition
		if( lightStage->SkipStage( lightRegs ) )
			continue;

		// Set the light projection texture
		renderProgManager.SetRenderParm( RENDERPARM_LIGHTPROJECTMAP, lightStage->texture.image );

		const idRenderVector lightColor(
			lightScale * lightRegs[ lightStage->color.registers[ SHADERPARM_RED ] ],
			lightScale * lightRegs[ lightStage->color.registers[ SHADERPARM_GREEN ] ],
			lightScale * lightRegs[ lightStage->color.registers[ SHADERPARM_BLUE] ],
						 lightRegs[ lightStage->color.registers[ SHADERPARM_ALPHA ] ] );
		renderProgManager.SetRenderParm( RENDERPARM_COLOR, lightColor );

		// apply the world-global overbright and the 2x factor for specular
		///const idRenderVector diffuseColor = lightColor;
		///const idRenderVector specularColor = lightColor * 2.0f;

		idRenderMatrix lightTextureRenderMatrix;
		ALIGN16( float lightTextureMatrix[ 16 ] );
		if( lightStage->texture.hasMatrix )
		{
			idMaterial::GetTexMatrixFromStage( lightRegs, &lightStage->texture, lightTextureMatrix );
			idMaterial::GetTexMatrixFromStage( lightRegs, &lightStage->texture, lightTextureRenderMatrix );
		}

		// force the light textures to not use anisotropic filtering, which is wasted on them
		// all of the texture sampler parms should be constant for all interactions, only
		// the actual texture image bindings will change

		//--------------------------------------------------------------------
		// For all surfaces on this light list, generate an interaction for this light stage
		//--------------------------------------------------------------------

		// setup renderparms assuming we will be drawing trivial surfaces first
		RB_SetupForFastPathInteractions();

		// even if the space does not change between light stages, each light stage may need a different lightTextureMatrix baked in
		backEnd.ClearCurrentSpace();

		for( int sortedSurfNum = 0; sortedSurfNum < allSurfaces.Num(); sortedSurfNum++ )
		{
			const auto surf = allSurfaces[ sortedSurfNum ];

			// select the render prog
			if( lightMaterial->IsAmbientLight() )
			{
				GL_SetRenderProgram( surf->HasSkinning() ? renderProgManager.prog_interaction_ambient_skinned : renderProgManager.prog_interaction_ambient );
			}
			else {
				if( r_useShadowMapping.GetBool() && vLight->globalShadows )
				{
					// RB: we have shadow mapping enabled and shadow maps so do a shadow compare
					BindProg_ForwardInteraction_ShadowMapping( lightType, surf->jointCache );
				}
				else {
					GL_SetRenderProgram( surf->HasSkinning() ? renderProgManager.prog_interaction_skinned : renderProgManager.prog_interaction );
				}
			}

			auto const surfaceMaterial = surf->material;
			auto const surfaceRegs = surf->shaderRegisters;

			inter.surf = surf;

			// change the MVP matrix, view/light origin and light projection vectors if needed
			if( surf->space != backEnd.currentSpace )
			{
				backEnd.currentSpace = surf->space;
				renderProgManager.SetSurfaceSpaceMatrices( surf );

				// tranform the light/view origin into model local space

				idRenderVector localLightOrigin( 0.0f );
				surf->space->modelMatrix.InverseTransformPoint( vLight->globalLightOrigin, localLightOrigin.ToVec3() );
				renderProgManager.SetRenderParm( RENDERPARM_LOCALLIGHTORIGIN, localLightOrigin );

				idRenderVector localViewOrigin( 1.0f );
				surf->space->modelMatrix.InverseTransformPoint( backEnd.viewDef->GetOrigin(), localViewOrigin.ToVec3() );
				renderProgManager.SetRenderParm( RENDERPARM_LOCALVIEWORIGIN, localViewOrigin );

				// transform the light project into model local space
				idRenderPlane lightProjection[ 4 ];
				surf->space->modelMatrix.InverseTransformPlane( vLight->lightProject[ 0 ], lightProjection[ 0 ] );
				surf->space->modelMatrix.InverseTransformPlane( vLight->lightProject[ 1 ], lightProjection[ 1 ] );
				surf->space->modelMatrix.InverseTransformPlane( vLight->lightProject[ 2 ], lightProjection[ 2 ] );
				surf->space->modelMatrix.InverseTransformPlane( vLight->lightProject[ 3 ], lightProjection[ 3 ] );

				// Set the light projection and optionally multiply the local light projection
				// by the light texture matrix.
				if( lightStage->texture.hasMatrix )
				{
					RB_BakeTextureMatrixIntoTexgen( lightProjection, lightTextureMatrix );

					idRenderMatrix baseLightProject;
					idRenderMatrix::Multiply( lightTextureRenderMatrix, vLight->baseLightProject, baseLightProject );
					idRenderMatrix::CopyMatrix( baseLightProject,
						renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_S )->GetVec4(),
						renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_T )->GetVec4(),
						renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_R )->GetVec4(),
						renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_Q )->GetVec4() );
				}
				else {
					idRenderMatrix::CopyMatrix( vLight->baseLightProject,
						renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_S )->GetVec4(),
						renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_T )->GetVec4(),
						renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_R )->GetVec4(),
						renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_Q )->GetVec4() );
				}

				// set the light projection
				renderProgManager.SetRenderParm( RENDERPARM_LIGHTPROJECTION_S, lightProjection[ 0 ].ToFloatPtr() );
				renderProgManager.SetRenderParm( RENDERPARM_LIGHTPROJECTION_T, lightProjection[ 1 ].ToFloatPtr() );
				renderProgManager.SetRenderParm( RENDERPARM_LIGHTPROJECTION_Q, lightProjection[ 2 ].ToFloatPtr() );
				renderProgManager.SetRenderParm( RENDERPARM_LIGHTFALLOFF_S,    lightProjection[ 3 ].ToFloatPtr() );

				// turn off the light depth bounds test if this model is rendered with a depth hack
				if( useLightDepthBounds )
				{
					if( !surf->space->weaponDepthHack && surf->space->modelDepthHack == 0.0f )
					{
						if( lightDepthBoundsDisabled )
						{
							GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
							lightDepthBoundsDisabled = false;
						}
					}
					else {
						if( !lightDepthBoundsDisabled )
						{
							GL_DepthBoundsTest( 0.0f, 0.0f );
							lightDepthBoundsDisabled = true;
						}
					}
				}
			}

			// check for the fast path
			if( surfaceMaterial->GetFastPathBumpImage() && !r_skipInteractionFastPath.GetBool() )
			{
				RENDERLOG_OPEN_BLOCK( surf->material->GetName() );

				renderProgManager.SetRenderParm( RENDERPARM_BUMPMAP, surfaceMaterial->GetFastPathBumpImage() );
				renderProgManager.SetRenderParm( RENDERPARM_DIFFUSEMAP, surfaceMaterial->GetFastPathDiffuseImage() );
				renderProgManager.SetRenderParm( RENDERPARM_SPECULARMAP, surfaceMaterial->GetFastPathSpecularImage() );

				GL_DrawIndexed( surf );

				RENDERLOG_CLOSE_BLOCK();
				continue;
			}

			//inter.bumpImage = inter.specularImage = inter.diffuseImage = NULL;
			//inter.diffuseColor[ 0 ] = inter.diffuseColor[ 1 ] = inter.diffuseColor[ 2 ] = inter.diffuseColor[ 3 ] = 0;
			//inter.specularColor[ 0 ] = inter.specularColor[ 1 ] = inter.specularColor[ 2 ] = inter.specularColor[ 3 ] = 0;
			inter.bumpImage->Set( ( idImage * )NULL );
			inter.diffuseImage->Set( ( idImage * )NULL );
			inter.specularImage->Set( ( idImage * )NULL );
			inter.diffuseColor->Set( 0.0f );
			inter.specularColor->Set( 0.0f );

			RB_DrawComplexMaterialInteraction( inter, surfaceRegs, surfaceMaterial );
		}
	}

	if( useLightDepthBounds && lightDepthBoundsDisabled )
	{
		GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
	}
}

/*
==================
 RB_DrawInteractionsForward
==================
*/
void RB_DrawInteractionsForward( const idRenderView * const viewDef )
{
	if( r_skipInteractions.GetBool() )
		return;

	RENDERLOG_OPEN_MAINBLOCK( MRB_DRAW_INTERACTIONS );
	RENDERLOG_OPEN_BLOCK( "RB_DrawInteractions" );

	GL_ResetTextureState();

	const bool useLightDepthBounds = r_useLightDepthBounds.GetBool();

	// Bind shadow uniform buffer range.
	if( r_useShadowMapping.GetBool() )
	{
		backEnd.shadowUniformBuffer.Bind( BINDING_SHADOW_UBO );
	}

	//
	// For each light, perform shadowing and adding.
	//
	for( const auto * vLight = viewDef->viewLights; vLight != NULL; vLight = vLight->next )
	{
		// do fogging later
		if( vLight->lightShader->IsFogLight() || vLight->lightShader->IsBlendLight() )
		{
			backEnd.hasFogging = true;
			continue;
		}
		if( vLight->NoInteractions() )
		{
			continue;
		}

		RENDERLOG_OPEN_BLOCK( vLight->lightShader->GetName() );

		if( r_useShadowMapping.GetBool() )
		{
			RB_FillShadowBuffer( vLight->globalShadows, vLight );

			// go back from light view to default camera view
			RB_ResetBaseRenderDest( r_useHDR.GetBool() );
			RB_ResetViewportAndScissorToDefaultCamera( viewDef );

			// set the depth bounds for the whole light
			if( useLightDepthBounds )
			{
				GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
			}

			if( vLight->localInteractions != NULL )
			{
				RENDERLOG_OPEN_BLOCK( "Local Light Interactions" );
				RB_RenderInteractions( vLight->localInteractions, vLight, GLS_DEPTHFUNC_EQUAL, false, useLightDepthBounds );
				RENDERLOG_CLOSE_BLOCK();
			}

			if( vLight->globalInteractions != NULL )
			{
				RENDERLOG_OPEN_BLOCK( "Global Light Interactions" );
				RB_RenderInteractions( vLight->globalInteractions, vLight, GLS_DEPTHFUNC_EQUAL, false, useLightDepthBounds );
				RENDERLOG_CLOSE_BLOCK();
			}
		}
		else {
			// set the depth bounds for the whole light
			if( useLightDepthBounds )
			{
				GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
			}

			// only need to clear the stencil buffer and perform stencil testing if there are shadows
			const bool performStencilTest = ( vLight->globalShadows != NULL || vLight->localShadows != NULL ) && !r_useShadowMapping.GetBool();

			// mirror flips the sense of the stencil select, and I don't want to risk accidentally breaking it
			// in the normal case, so simply disable the stencil select in the mirror case
			const bool useLightStencilSelect = ( r_useLightStencilSelect.GetBool() && backEnd.viewDef->isMirror == false );

			if( performStencilTest )
			{
				if( useLightStencilSelect )
				{
					// write a stencil mask for the visible light bounds to hi-stencil
					RB_StencilSelectLight( vLight );
				}
				else {
					// always clear whole S-Cull tiles
					idScreenRect rect;
					rect.x1 = ( vLight->scissorRect.x1 + 0 ) & ~15;
					rect.y1 = ( vLight->scissorRect.y1 + 0 ) & ~15;
					rect.x2 = ( vLight->scissorRect.x2 + 15 ) & ~15;
					rect.y2 = ( vLight->scissorRect.y2 + 15 ) & ~15;

					RB_SetScissor( rect );

					GL_State( GLS_DEFAULT );	// make sure stencil mask passes for the clear
					GL_Clear( false, false, true, STENCIL_SHADOW_TEST_VALUE, 0.0f, 0.0f, 0.0f, 0.0f );
				}
			}

			if( vLight->globalShadows != NULL )
			{
				RENDERLOG_OPEN_BLOCK( "Global Light Shadows" );
				RB_StencilShadowPass( vLight->globalShadows, vLight );
				RENDERLOG_CLOSE_BLOCK();
			}

			if( vLight->localInteractions != NULL )
			{
				RENDERLOG_OPEN_BLOCK( "Local Light Interactions" );
				RB_RenderInteractions( vLight->localInteractions, vLight, GLS_DEPTHFUNC_EQUAL, performStencilTest, useLightDepthBounds );
				RENDERLOG_CLOSE_BLOCK();
			}

			if( vLight->localShadows != NULL )
			{
				RENDERLOG_OPEN_BLOCK( "Local Light Shadows" );
				RB_StencilShadowPass( vLight->localShadows, vLight );
				RENDERLOG_CLOSE_BLOCK();
			}

			if( vLight->globalInteractions != NULL )
			{
				RENDERLOG_OPEN_BLOCK( "Global Light Interactions" );
				RB_RenderInteractions( vLight->globalInteractions, vLight, GLS_DEPTHFUNC_EQUAL, performStencilTest, useLightDepthBounds );
				RENDERLOG_CLOSE_BLOCK();
			}
		}

		if( vLight->translucentInteractions != NULL && !r_skipTranslucent.GetBool() )
		{
			RENDERLOG_OPEN_BLOCK( "Translucent Interactions" );

			// Disable the depth bounds test because translucent surfaces don't work with
			// the depth bounds tests since they did not write depth during the depth pass.
			if( useLightDepthBounds )
			{
				GL_DepthBoundsTest( 0.0f, 0.0f );
			}

			// The depth buffer wasn't filled in for translucent surfaces, so they
			// can never be constrained to perforated surfaces with the depthfunc equal.

			// Translucent surfaces do not receive shadows. This is a case where a
			// shadow buffer solution would work but stencil shadows do not because
			// stencil shadows only affect surfaces that contribute to the view depth
			// buffer and translucent surfaces do not contribute to the view depth buffer.

			RB_RenderInteractions( vLight->translucentInteractions, vLight, GLS_DEPTHFUNC_LEQUAL, false, false );

			RENDERLOG_CLOSE_BLOCK();
		}

		RENDERLOG_CLOSE_BLOCK();
	}

	// Unbind shadow uniform buffer
	if( r_useShadowMapping.GetBool() )
	{
		glBindBufferRange( GL_UNIFORM_BUFFER, BINDING_SHADOW_UBO, 0, 0, 0 );
	}

	// reset depth bounds
	if( useLightDepthBounds )
	{
		GL_DepthBoundsTest( 0.0f, 0.0f );
	}

	// unbind texture units
	GL_ResetTextureState();
	GL_ResetProgramState();

	RENDERLOG_CLOSE_BLOCK();
	RENDERLOG_CLOSE_MAINBLOCK();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	DEFERRED INTERACTIONS
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

/*
==========================================================
RB_DrawLight
==========================================================
*/
void RB_DrawLight( const viewLight_t * const vLight )
{
	{ // Set the matrix for deforming the 'zeroOneCubeModel' into the frustum to exactly cover the light volume
	  ///idRenderMatrix invProjectMVPMatrix;
	  ///idRenderMatrix::Multiply( backEnd.viewDef->GetMVPMatrix(), vLight->inverseBaseLightProject, invProjectMVPMatrix );
	  ///renderProgManager.SetMVPMatrixParms( invProjectMVPMatrix );
		bool negativeDeterminant;
		idRenderMatrix::SetMVPForInverseProject( backEnd.viewDef->GetMVPMatrix(), vLight->inverseBaseLightProject,
			renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_X )->GetVec4(),
			renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_Y )->GetVec4(),
			renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_Z )->GetVec4(),
			renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_W )->GetVec4(),
			negativeDeterminant );
		RENDERLOG_PRINT( "-- mvp determinant is %s --\n", negativeDeterminant ? "negative" : "positive" );
	}

	const auto lightMaterial = vLight->lightShader;
	const auto lightRegs = vLight->shaderRegisters;
	const auto lightType = vLight->GetLightType();
	const auto lightScale = r_useHDR.GetBool() ? 2.6f : r_lightScale.GetFloat();
	const auto shadowLOD = vLight->GetShadowLOD();

	if( r_useShadowMapping.GetBool() && vLight->globalShadows )
	{
		RB_SetupShadowCommonParms( lightType, shadowLOD );
	}

	// select the render program
	if( lightMaterial->IsAmbientLight() )
	{
		//renderProgManager.BindProg_Interaction_Deferred_Ambient();
		assert("no ambient prog in defrred pass");
	}
	else {
		GL_SetRenderProgram( ( r_useShadowMapping.GetBool() && vLight->globalShadows )?
			renderProgManager.prog_interaction_sm_deferred_light : renderProgManager.prog_interaction_deferred_light );
	}

	// Setup world space light origin render parameter.
	auto globalLightOrigin = renderProgManager.GetRenderParm( RENDERPARM_GLOBALLIGHTORIGIN )->GetVecPtr();
	globalLightOrigin[ 0 ] = vLight->globalLightOrigin[ 0 ];
	globalLightOrigin[ 1 ] = vLight->globalLightOrigin[ 1 ];
	globalLightOrigin[ 2 ] = vLight->globalLightOrigin[ 2 ];
	globalLightOrigin[ 3 ] = ( lightType == LIGHT_TYPE_PARALLEL ) ? -1.0f : ( ( lightType == LIGHT_TYPE_POINT ) ? 0.0f : 1.0f );

	// The light falloff texture.
	renderProgManager.SetRenderParm( RENDERPARM_LIGHTFALLOFFMAP, vLight->falloffImage );

	// some rare lights have multiple animating stages.
	for( int lightStageNum = 0; lightStageNum < lightMaterial->GetNumStages(); lightStageNum++ )
	{
		auto lightStage = lightMaterial->GetStage( lightStageNum );

		// Ignore stages that fail the condition.
		if( lightStage->SkipStage( lightRegs ) )
			continue;

		// The light projection texture.
		renderProgManager.SetRenderParm( RENDERPARM_LIGHTPROJECTMAP, lightStage->texture.image );

		// Setup scaled light color.
		const idRenderVector lightColor(
			lightScale * lightRegs[ lightStage->color.registers[ SHADERPARM_RED ] ],
			lightScale * lightRegs[ lightStage->color.registers[ SHADERPARM_GREEN ] ],
			lightScale * lightRegs[ lightStage->color.registers[ SHADERPARM_BLUE ] ],
						 lightRegs[ lightStage->color.registers[ SHADERPARM_ALPHA ] ] );
		renderProgManager.SetRenderParm( RENDERPARM_COLOR, lightColor );

		// Get light texture matrix and multiply the light projection by this matrix.
		if( lightStage->texture.hasMatrix )
		{
			idRenderPlane lightProjection[ 4 ];
			memcpy( lightProjection, vLight->lightProject, sizeof( idRenderPlane ) * _countof( lightProjection ) );

			ALIGN16( float lightTextureMatrix[ 16 ] );
			idMaterial::GetTexMatrixFromStage( lightRegs, &lightStage->texture, lightTextureMatrix );
			RB_BakeTextureMatrixIntoTexgen( lightProjection, lightTextureMatrix );

			renderProgManager.SetRenderParm( RENDERPARM_LIGHTPROJECTION_S, lightProjection[ 0 ].ToFloatPtr() );
			renderProgManager.SetRenderParm( RENDERPARM_LIGHTPROJECTION_T, lightProjection[ 1 ].ToFloatPtr() );
			renderProgManager.SetRenderParm( RENDERPARM_LIGHTPROJECTION_Q, lightProjection[ 2 ].ToFloatPtr() );
			renderProgManager.SetRenderParm( RENDERPARM_LIGHTFALLOFF_S, lightProjection[ 3 ].ToFloatPtr() );

			idRenderMatrix lightTextureRenderMatrix;
			idMaterial::GetTexMatrixFromStage( lightRegs, &lightStage->texture, lightTextureRenderMatrix );

			idRenderMatrix baseLightProject;
			idRenderMatrix::Multiply( lightTextureRenderMatrix, vLight->baseLightProject, baseLightProject );
			idRenderMatrix::CopyMatrix( baseLightProject,
				renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_S )->GetVec4(),
				renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_T )->GetVec4(),
				renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_R )->GetVec4(),
				renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_Q )->GetVec4() );
		}
		else // Simply set the light projection.
		{
			renderProgManager.SetRenderParm( RENDERPARM_LIGHTPROJECTION_S, vLight->lightProject[ 0 ].ToFloatPtr() );
			renderProgManager.SetRenderParm( RENDERPARM_LIGHTPROJECTION_T, vLight->lightProject[ 1 ].ToFloatPtr() );
			renderProgManager.SetRenderParm( RENDERPARM_LIGHTPROJECTION_Q, vLight->lightProject[ 2 ].ToFloatPtr() );
			renderProgManager.SetRenderParm( RENDERPARM_LIGHTFALLOFF_S, vLight->lightProject[ 3 ].ToFloatPtr() );

			idRenderMatrix::CopyMatrix( vLight->baseLightProject,
				renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_S )->GetVec4(),
				renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_T )->GetVec4(),
				renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_R )->GetVec4(),
				renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_Q )->GetVec4() );
		}

		///GL_DrawIndexed( &backEnd.zeroOneCubeSurface, LAYOUT_DRAW_VERT_POSITION_TEXCOORD );
		GL_DrawZeroOneCube();
		///GL_DrawZeroOneCubeAuto();
		// Fix MD calls first and implement deferred shadowing!
	}
}

idCVar r_dlmul( "r_dlmul", "2.7", CVAR_RENDERER | CVAR_FLOAT, "", 0.0, 5.0 );

/*
==========================================================
 RB_DrawInteractionsDeferred
==========================================================
*/
void RB_DrawInteractionsDeferred( const idRenderView * const view )
{
	if( r_skipInteractions.GetBool() )
		return;

	RENDERLOG_OPEN_MAINBLOCK( MRB_DRAW_INTERACTIONS );
	RENDERLOG_OPEN_BLOCK( "RB_DrawInteractionsDeferred" );

	RB_ResetBaseRenderDest( r_useHDR.GetBool() );
	RB_ResetViewportAndScissorToDefaultCamera( view );

	GL_ResetTextureState();

	const bool useLightDepthBounds = r_useLightDepthBounds.GetBool();

	// Bind shadow uniform buffer range.
	if( r_useShadowMapping.GetBool() )
	{
		backEnd.shadowUniformBuffer.Bind( BINDING_SHADOW_UBO );
	}

	renderProgManager.GetRenderParm( RENDERPARM_VIEWDEPTHSTENCILMAP )->Set( renderDestManager.viewDepthStencilImage );
	renderProgManager.GetRenderParm( RENDERPARM_VIEWCOLORMAP )->Set( renderDestManager.viewColorsImage );
	renderProgManager.GetRenderParm( RENDERPARM_VIEWNORMALMAP )->Set( renderDestManager.viewNormalImage );

	//
	// For each light, perform shadowing and adding.
	//
	for( const auto * vLight = view->viewLights; vLight != NULL; vLight = vLight->next )
	{
		// do fogging later
		if( vLight->lightShader->IsFogLight() || vLight->lightShader->IsBlendLight() )
		{
			backEnd.hasFogging = true;
			continue;
		}
		if( vLight->NoInteractions() )
		{
			continue;
		}

		RENDERLOG_OPEN_BLOCK( vLight->lightShader->GetName() );

		{
			RB_FillShadowBuffer( vLight->globalShadows, vLight );

			RB_ResetBaseRenderDest( r_useHDR.GetBool() );
			RB_ResetViewportAndScissorToDefaultCamera( view );
		}

		// set the depth bounds for the whole light
		if( useLightDepthBounds )
		{
			GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
		}

		// change the scissor if needed, it will be constant across all the surfaces lit by the light
		RB_SetScissor( vLight->scissorRect );

		//GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL | GLS_STENCIL_FUNC_ALWAYS );

		// calculate the global light bounds by inverse projecting the zero to one cube with the 'inverseBaseLightProject'
		idBounds globalLightBounds;
		idRenderMatrix::ProjectedBounds( globalLightBounds, vLight->inverseBaseLightProject, bounds_zeroOneCube, false );
		///const bool bCameraInsideLightGeometry = backEnd.viewDef->ViewInsideLightVolume( globalLightBounds );
		globalLightBounds.ExpandSelf( r_dlmul.GetFloat() );
		const bool bCameraInsideLightGeometry = globalLightBounds.ContainsPoint( view->GetOrigin() );
		///const bool bCameraInsideLightGeometry = !idRenderMatrix::CullPointToMVP( vLight->baseLightProject, backEnd.viewDef->GetOrigin(), true );

		if( bCameraInsideLightGeometry )
		{
			// Render backfaces with depth tests disabled since the camera is inside (or close to inside) the light geometry
			GL_State(/*GLS_ALPHAMASK |*/ GLS_DEPTHMASK | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_ALWAYS | GLS_BACKSIDED );
			///renderProgManager.SetColorParm( idColor::white.ToVec4() );
		}
		else {
			// Render frontfaces with depth tests on to get the speedup from HiZ since the camera is outside the light geometry
			GL_State(/*GLS_ALPHAMASK |*/ GLS_DEPTHMASK | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_LESS | GLS_FRONTSIDED );
			///renderProgManager.SetColorParm( idColor::purple.ToVec4() );
		}

		RB_DrawLight( vLight );

		RENDERLOG_CLOSE_BLOCK();
	}

	// Unbind shadow uniform buffer
	if( r_useShadowMapping.GetBool() )
	{
		glBindBufferRange( GL_UNIFORM_BUFFER, BINDING_SHADOW_UBO, 0, 0, 0 );
	}

	// reset depth bounds
	if( useLightDepthBounds )
	{
		GL_DepthBoundsTest( 0.0f, 0.0f );
	}

	renderProgManager.SetColorParm( idColor::white.ToVec4() );

	// unbind texture units
	GL_ResetTextureState();
	GL_ResetProgramState();

	RENDERLOG_CLOSE_BLOCK();
	RENDERLOG_CLOSE_MAINBLOCK();
}