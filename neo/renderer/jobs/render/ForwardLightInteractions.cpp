
#include "precompiled.h"
#pragma hdrstop

#include "Render.h"

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
==================
 RB_SetupInteractionStage
==================
*/
void RB_SetupInteractionStage( const materialStage_t* surfaceStage, const float* surfaceRegs, 
	const float lightColor[ 4 ], idVec4 matrix[ 2 ], float color[ 4 ] )
{
	if( surfaceStage->texture.hasMatrix )
	{
		matrix[ 0 ][ 0 ] = surfaceRegs[ surfaceStage->texture.matrix[ 0 ][ 0 ] ];
		matrix[ 0 ][ 1 ] = surfaceRegs[ surfaceStage->texture.matrix[ 0 ][ 1 ] ];
		matrix[ 0 ][ 2 ] = 0.0f;
		matrix[ 0 ][ 3 ] = surfaceRegs[ surfaceStage->texture.matrix[ 0 ][ 2 ] ];

		matrix[ 1 ][ 0 ] = surfaceRegs[ surfaceStage->texture.matrix[ 1 ][ 0 ] ];
		matrix[ 1 ][ 1 ] = surfaceRegs[ surfaceStage->texture.matrix[ 1 ][ 1 ] ];
		matrix[ 1 ][ 2 ] = 0.0f;
		matrix[ 1 ][ 3 ] = surfaceRegs[ surfaceStage->texture.matrix[ 1 ][ 2 ] ];

		// we attempt to keep scrolls from generating incredibly large texture values, but
		// center rotations and center scales can still generate offsets that need to be > 1
		if( matrix[ 0 ][ 3 ] < -40.0f || matrix[ 0 ][ 3 ] > 40.0f )
		{
			matrix[ 0 ][ 3 ] -= idMath::Ftoi( matrix[ 0 ][ 3 ] );
		}
		if( matrix[ 1 ][ 3 ] < -40.0f || matrix[ 1 ][ 3 ] > 40.0f )
		{
			matrix[ 1 ][ 3 ] -= idMath::Ftoi( matrix[ 1 ][ 3 ] );
		}
	}
	else
	{
		matrix[ 0 ][ 0 ] = 1.0f;
		matrix[ 0 ][ 1 ] = 0.0f;
		matrix[ 0 ][ 2 ] = 0.0f;
		matrix[ 0 ][ 3 ] = 0.0f;

		matrix[ 1 ][ 0 ] = 0.0f;
		matrix[ 1 ][ 1 ] = 1.0f;
		matrix[ 1 ][ 2 ] = 0.0f;
		matrix[ 1 ][ 3 ] = 0.0f;
	}

	if( color != NULL )
	{
		for( int i = 0; i < 4; i++ )
		{
			// clamp here, so cards with a greater range don't look different.
			// we could perform overbrighting like we do for lights, but
			// it doesn't currently look worth it.
			color[ i ] = idMath::ClampFloat( 0.0f, 1.0f, surfaceRegs[ surfaceStage->color.registers[ i ] ] ) * lightColor[ i ];
		}
	}
}

/*
=================
 RB_DrawSingleInteraction
=================
*/
void RB_DrawSingleInteraction( drawInteraction_t* din )
{
	if( din->bumpImage == NULL )
	{
		// stage wasn't actually an interaction
		return;
	}

	if( din->diffuseImage == NULL || r_skipDiffuse.GetBool() )
	{
		// this isn't a YCoCg black, but it doesn't matter, because
		// the diffuseColor will also be 0
		din->diffuseImage = renderImageManager->blackImage;
	}
	if( din->specularImage == NULL || r_skipSpecular.GetBool() || din->ambientLight )
	{
		din->specularImage = renderImageManager->blackImage;
	}
	if( r_skipBump.GetBool() )
	{
		din->bumpImage = renderImageManager->flatNormalMap;
	}

	// if we wouldn't draw anything, don't call the Draw function
	const bool diffuseIsBlack = ( din->diffuseImage == renderImageManager->blackImage )
		|| ( ( din->diffuseColor[ 0 ] <= 0 ) && ( din->diffuseColor[ 1 ] <= 0 ) && ( din->diffuseColor[ 2 ] <= 0 ) );
	
	const bool specularIsBlack = ( din->specularImage == renderImageManager->blackImage )
		|| ( ( din->specularColor[ 0 ] <= 0 ) && ( din->specularColor[ 1 ] <= 0 ) && ( din->specularColor[ 2 ] <= 0 ) );
	
	if( diffuseIsBlack && specularIsBlack )
	{
		return;
	}

	// bump matrix
	renderProgManager.SetRenderParm( RENDERPARM_BUMPMATRIX_S, din->bumpMatrix[ 0 ] );
	renderProgManager.SetRenderParm( RENDERPARM_BUMPMATRIX_T, din->bumpMatrix[ 1 ] );

	// diffuse matrix
	renderProgManager.SetRenderParm( RENDERPARM_DIFFUSEMATRIX_S, din->diffuseMatrix[ 0 ] );
	renderProgManager.SetRenderParm( RENDERPARM_DIFFUSEMATRIX_T, din->diffuseMatrix[ 1 ] );

	// specular matrix
	renderProgManager.SetRenderParm( RENDERPARM_SPECULARMATRIX_S, din->specularMatrix[ 0 ] );
	renderProgManager.SetRenderParm( RENDERPARM_SPECULARMATRIX_T, din->specularMatrix[ 1 ] );

	RB_SetVertexColorParms( din->vertexColor );

	renderProgManager.SetRenderParm( RENDERPARM_DIFFUSEMODIFIER, din->diffuseColor );
	renderProgManager.SetRenderParm( RENDERPARM_SPECULARMODIFIER, din->specularColor );

	
	///GL_BindTexture( INTERACTION_TEXUNIT_BUMP, din->bumpImage ); // texture 0 will be the per-surface bump map	
	///GL_BindTexture( INTERACTION_TEXUNIT_DIFFUSE, din->diffuseImage ); // texture 3 is the per-surface diffuse map	
	///GL_BindTexture( INTERACTION_TEXUNIT_SPECULAR, din->specularImage ); // texture 4 is the per-surface specular map
	renderProgManager.SetRenderParm( RENDERPARM_BUMPMAP, din->bumpImage );
	renderProgManager.SetRenderParm( RENDERPARM_DIFFUSEMAP, din->diffuseImage );
	renderProgManager.SetRenderParm( RENDERPARM_SPECULARMAP, din->specularImage );

	GL_DrawIndexed( din->surf );
}

/*
=================
 RB_SetupForFastPathInteractions

	These are common for all fast path surfaces
=================
*/
void RB_SetupForFastPathInteractions( const idVec4& diffuseColor, const idVec4& specularColor )
{
	const idRenderVector sMatrix( 1.0f, 0.0f, 0.0f, 0.0f );
	const idRenderVector tMatrix( 0.0f, 1.0f, 0.0f, 0.0f );

	// bump matrix
	renderProgManager.SetRenderParm( RENDERPARM_BUMPMATRIX_S, sMatrix );
	renderProgManager.SetRenderParm( RENDERPARM_BUMPMATRIX_T, tMatrix );

	// diffuse matrix
	renderProgManager.SetRenderParm( RENDERPARM_DIFFUSEMATRIX_S, sMatrix );
	renderProgManager.SetRenderParm( RENDERPARM_DIFFUSEMATRIX_T, tMatrix );

	// specular matrix
	renderProgManager.SetRenderParm( RENDERPARM_SPECULARMATRIX_S, sMatrix );
	renderProgManager.SetRenderParm( RENDERPARM_SPECULARMATRIX_T, tMatrix );

	RB_SetVertexColorParms( SVC_IGNORE );

	renderProgManager.SetRenderParm( RENDERPARM_DIFFUSEMODIFIER, diffuseColor );
	renderProgManager.SetRenderParm( RENDERPARM_SPECULARMODIFIER, specularColor );
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
void RB_DrawComplexMaterialInteraction( drawInteraction_t & inter,
	const float* surfaceRegs, const idMaterial* const surfaceMaterial,
	const idRenderVector & diffuseColorMul, const idRenderVector & specularColorMul )
{
	RENDERLOG_OPEN_BLOCK( surfaceMaterial->GetName() );

	for( int stageNum = 0; stageNum < surfaceMaterial->GetNumStages(); stageNum++ )
	{
		auto const surfaceMaterialStage = surfaceMaterial->GetStage( stageNum );

		switch( surfaceMaterialStage->lighting )
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
				if( !surfaceRegs[ surfaceMaterialStage->conditionRegister ] )
				{
					break;
				}
				// draw any previous interaction
				if( inter.bumpImage != NULL )
				{
					RB_DrawSingleInteraction( &inter );
				}
				inter.bumpImage = surfaceMaterialStage->texture.image;
				inter.diffuseImage = NULL;
				inter.specularImage = NULL;
				RB_SetupInteractionStage( surfaceMaterialStage, surfaceRegs, NULL, inter.bumpMatrix, NULL );
				break;
			}
			case SL_DIFFUSE:
			{
				// ignore stage that fails the condition
				if( !surfaceRegs[ surfaceMaterialStage->conditionRegister ] )
				{
					break;
				}
				// draw any previous interaction
				if( inter.diffuseImage != NULL )
				{
					RB_DrawSingleInteraction( &inter );
				}
				inter.diffuseImage = surfaceMaterialStage->texture.image;
				inter.vertexColor = surfaceMaterialStage->vertexColor;
				RB_SetupInteractionStage( surfaceMaterialStage, surfaceRegs, diffuseColorMul.ToFloatPtr(), inter.diffuseMatrix, inter.diffuseColor.ToFloatPtr() );
				break;
			}
			case SL_SPECULAR:
			{
				// ignore stage that fails the condition
				if( !surfaceRegs[ surfaceMaterialStage->conditionRegister ] )
				{
					break;
				}
				// draw any previous interaction
				if( inter.specularImage != NULL )
				{
					RB_DrawSingleInteraction( &inter );
				}
				inter.specularImage = surfaceMaterialStage->texture.image;
				inter.vertexColor = surfaceMaterialStage->vertexColor;
				RB_SetupInteractionStage( surfaceMaterialStage, surfaceRegs, specularColorMul.ToFloatPtr(), inter.specularMatrix, inter.specularColor.ToFloatPtr() );
				break;
			}
		}
	}

	// draw the final interaction
	RB_DrawSingleInteraction( &inter );

	RENDERLOG_CLOSE_BLOCK();
}


/*
=============
 RB_RenderInteractions

	With added sorting and trivial path work.
=============
*/
void RB_RenderInteractions( const drawSurf_t* const surfList, const viewLight_t* const vLight,int depthFunc, bool performStencilTest, bool useLightDepthBounds )
{
	if( surfList == NULL )
		return;

	// change the scissor if needed, it will be constant across all the surfaces lit by the light
	RB_SetScissor( vLight->scissorRect );

	// perform setup here that will be constant for all interactions
	if( performStencilTest )
	{
		GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | depthFunc | GLS_STENCIL_FUNC_EQUAL |
			GLS_STENCIL_MAKE_REF( STENCIL_SHADOW_TEST_VALUE ) | GLS_STENCIL_MAKE_MASK( STENCIL_SHADOW_MASK_VALUE ) );
	}
	else {
		GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | depthFunc | GLS_STENCIL_FUNC_ALWAYS );
	}

	// some rare lights have multiple animating stages, loop over them outside the surface list
	const idMaterial* const lightShader = vLight->lightShader;
	const float* lightRegs = vLight->shaderRegisters;

	drawInteraction_t inter = {};
	inter.ambientLight = lightShader->IsAmbientLight();

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

	///const int32 shadowLOD = backEnd.viewDef->isSubview ? ( Max( vLight->shadowLOD + 1, MAX_SHADOWMAP_RESOLUTIONS-1 ) ) : vLight->shadowLOD ;
	///const int32 shadowLOD = backEnd.viewDef->isSubview ? ( MAX_SHADOWMAP_RESOLUTIONS - 1 ) : vLight->shadowLOD;
	const int32 shadowLOD = backEnd.viewDef->isSubview ? ( MAX_SHADOWMAP_RESOLUTIONS - 1 ) : ( vLight->parallel ? vLight->shadowLOD : idMath::Max( vLight->shadowLOD, 1 ) );

	// RB begin
	if( r_useShadowMapping.GetBool() )
	{
		const static int JITTER_SIZE = 128;

		// default high quality
		float jitterSampleScale = 1.0f;
		float shadowMapSamples = r_shadowMapSamples.GetInteger();

		// screen power of two correction factor / rpScreenCorrectionFactor;
		auto & screenCorrectionParm = renderProgManager.GetRenderParm( RENDERPARM_SCREENCORRECTIONFACTOR );
		screenCorrectionParm[ 0 ] = 1.0f / ( JITTER_SIZE * shadowMapSamples );
		screenCorrectionParm[ 1 ] = 1.0f / JITTER_SIZE;
		screenCorrectionParm[ 2 ] = 1.0f / shadowMapResolutions[ shadowLOD ];
		screenCorrectionParm[ 3 ] = vLight->parallel ? r_shadowMapSunDepthBiasScale.GetFloat() : r_shadowMapRegularDepthBiasScale.GetFloat();

		// rpJitterTexScale
		auto & jitterTexScale = renderProgManager.GetRenderParm( RENDERPARM_JITTERTEXSCALE );
		jitterTexScale[ 0 ] = r_shadowMapJitterScale.GetFloat() * jitterSampleScale;	// TODO shadow buffer size fraction shadowMapSize / maxShadowMapSize
		jitterTexScale[ 1 ] = r_shadowMapJitterScale.GetFloat() * jitterSampleScale;
		jitterTexScale[ 2 ] = -r_shadowMapBiasScale.GetFloat();
		jitterTexScale[ 3 ] = 0.0f;

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

		/*if( vLight->parallel ) { // rpCascadeDistances
			auto & cascadeDistances = renderProgManager.GetRenderParm( RENDERPARM_CASCADEDISTANCES );
			cascadeDistances[0] = backEnd.viewDef->GetSplitFrustumDistances()[0];
			cascadeDistances[1] = backEnd.viewDef->GetSplitFrustumDistances()[1];
			cascadeDistances[2] = backEnd.viewDef->GetSplitFrustumDistances()[2];
			cascadeDistances[3] = backEnd.viewDef->GetSplitFrustumDistances()[3];
		}*/

	}
	// RB end

	const float lightScale = r_useHDR.GetBool() ? 2.6f : r_lightScale.GetFloat();

	for( int lightStageNum = 0; lightStageNum < lightShader->GetNumStages(); lightStageNum++ )
	{
		auto lightStage = lightShader->GetStage( lightStageNum );

		// ignore stages that fail the condition
		if( !lightRegs[ lightStage->conditionRegister ] )
			continue;

		const idRenderVector lightColor(
			lightScale * lightRegs[ lightStage->color.registers[ SHADERPARM_RED ] ],
			lightScale * lightRegs[ lightStage->color.registers[ SHADERPARM_GREEN ] ],
			lightScale * lightRegs[ lightStage->color.registers[ SHADERPARM_BLUE] ],
						 lightRegs[ lightStage->color.registers[ SHADERPARM_ALPHA ] ] );

		// apply the world-global overbright and the 2x factor for specular
		const idRenderVector diffuseColor = lightColor;
		const idRenderVector specularColor = lightColor * 2.0f;

		ALIGN16( float lightTextureMatrix[ 16 ] );
		if( lightStage->texture.hasMatrix )
		{
			RB_GetShaderTextureMatrix( lightRegs, &lightStage->texture, lightTextureMatrix );
		}

		// texture 1 will be the light falloff texture
		///GL_BindTexture( INTERACTION_TEXUNIT_FALLOFF, vLight->falloffImage );
		renderProgManager.SetRenderParm( RENDERPARM_LIGHTFALLOFFMAP, vLight->falloffImage );

		// texture 2 will be the light projection texture
		///GL_BindTexture( INTERACTION_TEXUNIT_PROJECTION, lightStage->texture.image );
		renderProgManager.SetRenderParm( RENDERPARM_LIGHTPROJECTMAP, lightStage->texture.image );

		if( r_useShadowMapping.GetBool() )
		{
			// texture 5 will be the shadow maps array
			///GL_BindTexture( INTERACTION_TEXUNIT_SHADOWMAPS, renderImageManager->shadowImage[ shadowLOD ] );
			renderProgManager.SetRenderParm( RENDERPARM_SHADOWBUFFERMAP, renderImageManager->shadowImage[ shadowLOD ] );

			// texture 6 will be the jitter texture for soft shadowing
			if( r_shadowMapSamples.GetInteger() == 16 )
			{
				///GL_BindTexture( INTERACTION_TEXUNIT_JITTER, renderImageManager->jitterImage16 );
				renderProgManager.SetRenderParm( RENDERPARM_JITTERMAP, renderImageManager->jitterImage16 );
			}
			else if( r_shadowMapSamples.GetInteger() == 4 )
			{
				///GL_BindTexture( INTERACTION_TEXUNIT_JITTER, renderImageManager->jitterImage4 );
				renderProgManager.SetRenderParm( RENDERPARM_JITTERMAP, renderImageManager->jitterImage4 );
			}
			else {
				///GL_BindTexture( INTERACTION_TEXUNIT_JITTER, renderImageManager->jitterImage1 );
				renderProgManager.SetRenderParm( RENDERPARM_JITTERMAP, renderImageManager->jitterImage1 );
			}
		}

		// force the light textures to not use anisotropic filtering, which is wasted on them
		// all of the texture sampler parms should be constant for all interactions, only
		// the actual texture image bindings will change

		//----------------------------------
		// For all surfaces on this light list, generate an interaction for this light stage
		//----------------------------------

		// setup renderparms assuming we will be drawing trivial surfaces first
		RB_SetupForFastPathInteractions( diffuseColor, specularColor );

		// even if the space does not change between light stages, each light stage may need a different lightTextureMatrix baked in
		backEnd.ClearCurrentSpace();

		for( int sortedSurfNum = 0; sortedSurfNum < allSurfaces.Num(); sortedSurfNum++ )
		{
			const drawSurf_t* const surf = allSurfaces[ sortedSurfNum ];

			// select the render prog
			if( lightShader->IsAmbientLight() )
			{
				renderProgManager.BindShader_InteractionAmbient( surf->jointCache );
			}
			else
			{
				if( r_useShadowMapping.GetBool() && vLight->globalShadows )
				{
					// RB: we have shadow mapping enabled and shadow maps so do a shadow compare

					if( vLight->parallel )
					{
						renderProgManager.BindShader_Interaction_ShadowMapping_Parallel( surf->jointCache );
					}
					else if( vLight->pointLight )
					{
						renderProgManager.BindShader_Interaction_ShadowMapping_Point( surf->jointCache );
					}
					else
					{
						renderProgManager.BindShader_Interaction_ShadowMapping_Spot( surf->jointCache );
					}
				}
				else
				{
					renderProgManager.BindShader_Interaction( surf->jointCache );
				}
			}

			const idMaterial* const surfaceMaterial = surf->material;
			const float* surfaceRegs = surf->shaderRegisters;

			inter.surf = surf;

			// change the MVP matrix, view/light origin and light projection vectors if needed
			if( surf->space != backEnd.currentSpace )
			{
				backEnd.currentSpace = surf->space;
				RB_SetSurfaceSpaceMatrices( surf );

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

				auto & globalLightOrigin = renderProgManager.GetRenderParm( RENDERPARM_GLOBALLIGHTORIGIN );
				globalLightOrigin.Set( vLight->globalLightOrigin, vLight->parallel ? ( -1.0f ) : ( vLight->pointLight ? 0.0f : 1.0f ) );
				// RB end

				// tranform the light/view origin into model local space

				idRenderVector localLightOrigin( 0.0f );
				surf->space->modelMatrix.InverseTransformPoint( vLight->globalLightOrigin, localLightOrigin.ToVec3() );
				renderProgManager.SetRenderParm( RENDERPARM_LOCALLIGHTORIGIN, localLightOrigin );
				
				idRenderVector localViewOrigin( 1.0f );
				surf->space->modelMatrix.InverseTransformPoint( backEnd.viewDef->GetOrigin(), localViewOrigin.ToVec3() );
				renderProgManager.SetRenderParm( RENDERPARM_LOCALVIEWORIGIN, localViewOrigin );

				// transform the light project into model local space
				idRenderPlane lightProjection[ 4 ];
				for( int i = 0; i < 4; i++ ) {
					surf->space->modelMatrix.InverseTransformPlane( vLight->lightProject[ i ], lightProjection[ i ] );
				}
				// optionally multiply the local light projection by the light texture matrix
				if( lightStage->texture.hasMatrix )
				{
					RB_BakeTextureMatrixIntoTexgen( lightProjection, lightTextureMatrix );
				}

				// set the light projection
				renderProgManager.SetRenderParm( RENDERPARM_LIGHTPROJECTION_S, lightProjection[ 0 ].ToFloatPtr() );
				renderProgManager.SetRenderParm( RENDERPARM_LIGHTPROJECTION_T, lightProjection[ 1 ].ToFloatPtr() );
				renderProgManager.SetRenderParm( RENDERPARM_LIGHTPROJECTION_Q, lightProjection[ 2 ].ToFloatPtr() );
				renderProgManager.SetRenderParm( RENDERPARM_LIGHTFALLOFF_S,    lightProjection[ 3 ].ToFloatPtr() );
			/*
				idRenderMatrix localBaseLightProject;
				idRenderMatrix::Multiply( vLight->baseLightProject, surf->space->modelMatrix, localBaseLightProject );

				idRenderMatrix localBaseLightProjectWithTexMat;;
				idRenderMatrix::Multiply( localBaseLightProject, *( idRenderMatrix* )lightTextureMatrix, localBaseLightProjectWithTexMat );
			*/
				idRenderMatrix::CopyMatrix( vLight->baseLightProject, //localBaseLightProjectWithTexMat,
					renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_S ),
					renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_T ),
					renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_R ),
					renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_Q ) );
				
				// RB begin
				if( r_useShadowMapping.GetBool() )
				{
					if( vLight->parallel )
					{
						idRenderMatrix shadowClipMVPs[ 6 ];
						uint32 count = ( r_shadowMapSplits.GetInteger() + 1 );
						for( uint32 i = 0; i < count; i++ )
						{
							///idRenderMatrix shadowClipMVP;
							idRenderMatrix::Multiply( backEnd.shadowVP[ i ], surf->space->modelMatrix, shadowClipMVPs[ i ] );
							///renderProgManager.SetRenderParms( ( renderParm_t )( RENDERPARM_SHADOW_MATRIX_0_X + i * 4 ), shadowClipMVP.Ptr(), 4 );
						}
						backEnd.shadowUniformBuffer.Update( sizeof( idRenderMatrix ) * count, shadowClipMVPs->Ptr() );
					}
					else if( vLight->pointLight )
					{
						idRenderMatrix shadowClipMVPs[ 6 ];
						for( uint32 i = 0; i < 6; i++ )
						{
							///idRenderMatrix shadowClipMVP;
							idRenderMatrix::Multiply( backEnd.shadowVP[ i ], surf->space->modelMatrix, shadowClipMVPs[ i ] );
							///renderProgManager.SetRenderParms( ( renderParm_t )( RENDERPARM_SHADOW_MATRIX_0_X + i * 4 ), shadowClipMVP.Ptr(), 4 );
						}
						backEnd.shadowUniformBuffer.Update( sizeof( idRenderMatrix ) * 6, shadowClipMVPs->Ptr() );
					}
					else // spot light
					{
						idRenderMatrix shadowClipMVP;
						idRenderMatrix::Multiply( backEnd.shadowVP[ 0 ], surf->space->modelMatrix, shadowClipMVP );
						///renderProgManager.SetRenderParms( ( renderParm_t )( RENDERPARM_SHADOW_MATRIX_0_X ), shadowClipMVP.Ptr(), 4 );
						backEnd.shadowUniformBuffer.Update( sizeof( idRenderMatrix ), shadowClipMVP.Ptr() );
					}

					const GLintptr ubo = GetGLObject( backEnd.shadowUniformBuffer.GetAPIObject() );
					glBindBufferRange( GL_UNIFORM_BUFFER, BINDING_SHADOW_UBO, ubo, backEnd.shadowUniformBuffer.GetOffset(), backEnd.shadowUniformBuffer.GetSize() );
				}
				// RB end
			}

			// check for the fast path
			if( surfaceMaterial->GetFastPathBumpImage() && !r_skipInteractionFastPath.GetBool() )
			{
				RENDERLOG_OPEN_BLOCK( surf->material->GetName() );

				//GL_BindTexture( INTERACTION_TEXUNIT_BUMP, surfaceShader->GetFastPathBumpImage() );
				//GL_BindTexture( INTERACTION_TEXUNIT_DIFFUSE, surfaceShader->GetFastPathDiffuseImage() );
				//GL_BindTexture( INTERACTION_TEXUNIT_SPECULAR, surfaceShader->GetFastPathSpecularImage() );
				renderProgManager.SetRenderParm( RENDERPARM_BUMPMAP, surfaceMaterial->GetFastPathBumpImage() );
				renderProgManager.SetRenderParm( RENDERPARM_DIFFUSEMAP, surfaceMaterial->GetFastPathDiffuseImage() );
				renderProgManager.SetRenderParm( RENDERPARM_SPECULARMAP, surfaceMaterial->GetFastPathSpecularImage() );

				GL_DrawIndexed( surf );

				RENDERLOG_CLOSE_BLOCK();
				continue;
			}

			inter.bumpImage = inter.specularImage = inter.diffuseImage = NULL;
			inter.diffuseColor[ 0 ] = inter.diffuseColor[ 1 ] = inter.diffuseColor[ 2 ] = inter.diffuseColor[ 3 ] = 0;
			inter.specularColor[ 0 ] = inter.specularColor[ 1 ] = inter.specularColor[ 2 ] = inter.specularColor[ 3 ] = 0;

			RB_DrawComplexMaterialInteraction( inter, surfaceRegs, surfaceMaterial, diffuseColor, specularColor );
		}
	}

	if( useLightDepthBounds && lightDepthBoundsDisabled )
	{
		GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
	}

	renderProgManager.Unbind();
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

	GL_SelectTexture( 0 );

	const bool useLightDepthBounds = r_useLightDepthBounds.GetBool() && !r_useShadowMapping.GetBool();

	//
	// for each light, perform shadowing and adding
	//
	for( const auto * vLight = viewDef->viewLights; vLight != NULL; vLight = vLight->next )
	{
		// do fogging later
		if( vLight->lightShader->IsFogLight() || vLight->lightShader->IsBlendLight() )
		{
			continue;
		}
		if( vLight->NoInteractions() )
		{
			continue;
		}

		RENDERLOG_OPEN_BLOCK( vLight->lightShader->GetName() );

		// set the depth bounds for the whole light
		if( useLightDepthBounds )
		{
			GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
		}

		if( r_useShadowMapping.GetBool() )
		{
			RB_ShadowMapPass( vLight->globalShadows, vLight );

			// go back from light view to default camera view
			RB_ResetViewportAndScissorToDefaultCamera( viewDef );

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
					GL_Clear( false, false, true, STENCIL_SHADOW_TEST_VALUE, 0.0f, 0.0f, 0.0f, 0.0f, false );
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

			RB_RenderInteractions( vLight->translucentInteractions, vLight, GLS_DEPTHFUNC_LESS, false, false );

			RENDERLOG_CLOSE_BLOCK();
		}

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

