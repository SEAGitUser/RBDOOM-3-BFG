
#include "precompiled.h"
#pragma hdrstop

#include "Render.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

idCVar r_skipFogLights( "r_skipFogLights", "0", CVAR_RENDERER | CVAR_BOOL, "skip fog lights" );
idCVar r_skipBlendLights( "r_skipBlendLights", "0", CVAR_RENDERER | CVAR_BOOL, "skip blend lights" );
idCVar r_useScreenSpaceBlendLights( "r_useScreenSpaceBlendLights", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "use screen space blend lights" );
idCVar r_useScreenSpaceFogLights( "r_useScreenSpaceFogLights", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "use screen space fog lights" );
/*
=============================================================================================

	BLEND LIGHT PROJECTION

=============================================================================================
*/

static ID_INLINE void _GlobalPlaneToLocal( const idRenderMatrix & modelMatrix, const idPlane& in, idPlane& out )
{
	out[ 0 ] = in[ 0 ] * modelMatrix[ 0 ][ 0 ] + in[ 1 ] * modelMatrix[ 1 ][ 0 ] + in[ 2 ] * modelMatrix[ 2 ][ 0 ];
	out[ 1 ] = in[ 0 ] * modelMatrix[ 0 ][ 1 ] + in[ 1 ] * modelMatrix[ 1 ][ 1 ] + in[ 2 ] * modelMatrix[ 2 ][ 1 ];
	out[ 2 ] = in[ 0 ] * modelMatrix[ 0 ][ 2 ] + in[ 1 ] * modelMatrix[ 1 ][ 2 ] + in[ 2 ] * modelMatrix[ 2 ][ 2 ];
	out[ 3 ] = in[ 0 ] * modelMatrix[ 0 ][ 3 ] + in[ 1 ] * modelMatrix[ 1 ][ 3 ] + in[ 2 ] * modelMatrix[ 2 ][ 3 ] + in[ 3 ];
}

/*
=====================
 RB_T_BlendLight
=====================
*/
static void RB_T_BlendLight( const drawSurf_t* drawSurfs, const viewLight_t* vLight )
{
	backEnd.ClearCurrentSpace();

	for( const drawSurf_t* drawSurf = drawSurfs; drawSurf != NULL; drawSurf = drawSurf->nextOnLight )
	{
		if( drawSurf->scissorRect.IsEmpty() )
		{
			continue;	// !@# FIXME: find out why this is sometimes being hit!
			// temporarily jump over the scissor and draw so the gl error callback doesn't get hit
		}

		RENDERLOG_OPEN_BLOCK( drawSurf->material->GetName() );

		RB_SetScissor( drawSurf->scissorRect );

		if( drawSurf->space != backEnd.currentSpace )
		{
			// change the matrix
			renderProgManager.SetMVPMatrixParms( drawSurf->space->mvp );

			// change the light projection matrix
			idRenderPlane lightProjectInCurrentSpace[ 4 ];
			for( int i = 0; i < 4; i++ )
			{
				_GlobalPlaneToLocal( drawSurf->space->modelMatrix, vLight->lightProject[ i ], lightProjectInCurrentSpace[ i ] );
			}
			/*for( int i = 0; i < 4; i++ )
			{
				drawSurf->space->modelMatrix.InverseTransformPlane( vLight->lightProject[ i ], lightProjectInCurrentSpace[ i ] );
			}*/

			renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_S, lightProjectInCurrentSpace[ 0 ].ToFloatPtr() );
			renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_T, lightProjectInCurrentSpace[ 1 ].ToFloatPtr() );
			renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_Q, lightProjectInCurrentSpace[ 2 ].ToFloatPtr() );
			renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_1_S, lightProjectInCurrentSpace[ 3 ].ToFloatPtr() );	// falloff

			backEnd.currentSpace = drawSurf->space;
		}

		GL_DrawIndexed( drawSurf );

		RENDERLOG_CLOSE_BLOCK();
	}
}

/*
=====================
 RB_BlendLight

	Dual texture together the falloff and projection texture with a blend
	mode to the framebuffer, instead of interacting with the surface texture
=====================
*/
static void RB_BlendLight( const drawSurf_t * const drawSurfs, const drawSurf_t * const drawSurfs2, const viewLight_t * const vLight )
{
	if( drawSurfs == NULL )
		return;

	RENDERLOG_OPEN_BLOCK( vLight->lightShader->GetName() );

	auto const material = vLight->lightShader;
	auto const regs = vLight->shaderRegisters;

	GL_SetRenderProgram( renderProgManager.prog_blendlight );

	// texture 1 will get the falloff texture
	renderProgManager.SetRenderParm( RENDERPARM_LIGHTFALLOFFMAP, vLight->falloffImage );

	for( int i = 0; i < material->GetNumStages(); ++i )
	{
		auto const stage = material->GetStage( i );

		if( stage->SkipStage( regs ) )
		{
			continue;
		}

		GL_State( GLS_DEPTHMASK | stage->drawStateBits | GLS_DEPTHFUNC_EQUAL );

		// texture 0 will get the projected texture
		renderProgManager.SetRenderParm( RENDERPARM_LIGHTPROJECTMAP, stage->texture.image );

		renderProgManager.SetupTextureMatrixParms( regs, &stage->texture );

		// get the modulate values from the light, including alpha, unlike normal lights
		idRenderVector lightColor;
		stage->GetColorParm( regs, lightColor );
		renderProgManager.SetColorParm( lightColor );

		RB_T_BlendLight( drawSurfs, vLight );
		RB_T_BlendLight( drawSurfs2, vLight );
	}

	GL_ResetTextureState();
	GL_ResetProgramState();

	RENDERLOG_CLOSE_BLOCK();
}

idCVar r_blmul( "r_blmul", "2.4", CVAR_RENDERER | CVAR_FLOAT, "", 0.0, 5.0 );
/*
=====================
 RB_ScreenSpaceBlendLight

	single light for now :(

	TODO: all light texture atlas and draw in one pass!
=====================
*/
static void RB_ScreenSpaceBlendLight( const viewLight_t * const vLight )
{
	RENDERLOG_OPEN_BLOCK( vLight->lightShader->GetName() );

	const auto * const material = vLight->lightShader;
	const float* regs = vLight->shaderRegisters;

	backEnd.ClearCurrentSpace();

	GL_SetRenderProgram( renderProgManager.prog_blendlight2 );

	// set the matrix for deforming the 'zeroOneCubeModel' into the frustum to exactly cover the light volume
	idRenderMatrix::Multiply( backEnd.viewDef->GetMVPMatrix(), vLight->inverseBaseLightProject,
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_X )->GetVec4(),
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_Y )->GetVec4(),
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_Z )->GetVec4(),
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_W )->GetVec4() );

	// calculate the global light bounds by inverse projecting the zero to one cube with the 'inverseBaseLightProject'
	idBounds globalLightBounds;
	idRenderMatrix::ProjectedBounds( globalLightBounds, vLight->inverseBaseLightProject, bounds_zeroOneCube, false );
	//const bool bCameraInsideLightGeometry = backEnd.viewDef->ViewInsideLightVolume( globalLightBounds );
	globalLightBounds.ExpandSelf( r_blmul.GetFloat() );
	const bool bCameraInsideLightGeometry = globalLightBounds.ContainsPoint( backEnd.viewDef->GetOrigin() );
	//const bool bCameraInsideLightGeometry = !idRenderMatrix::CullPointToMVP( vLight->baseLightProject, backEnd.viewDef->GetOrigin(), true );

	///renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_S, vLight->lightProject[ 0 ].ToFloatPtr() );
	///renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_T, vLight->lightProject[ 1 ].ToFloatPtr() );
	///renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_Q, vLight->lightProject[ 2 ].ToFloatPtr() );
	///renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_1_S, vLight->lightProject[ 3 ].ToFloatPtr() );	// falloff

	renderProgManager.SetRenderParm( RENDERPARM_LIGHTPROJECTION_S, vLight->lightProject[ 0 ].ToFloatPtr() );
	renderProgManager.SetRenderParm( RENDERPARM_LIGHTPROJECTION_T, vLight->lightProject[ 1 ].ToFloatPtr() );
	renderProgManager.SetRenderParm( RENDERPARM_LIGHTPROJECTION_Q, vLight->lightProject[ 2 ].ToFloatPtr() );
	renderProgManager.SetRenderParm( RENDERPARM_LIGHTFALLOFF_S, vLight->lightProject[ 3 ].ToFloatPtr() );

	idRenderMatrix::CopyMatrix( vLight->baseLightProject,
		renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_S )->GetVec4(),
		renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_T )->GetVec4(),
		renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_R )->GetVec4(),
		renderProgManager.GetRenderParm( RENDERPARM_BASELIGHTPROJECT_Q )->GetVec4() );

	RB_SetScissor( vLight->scissorRect );
	GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );

	renderProgManager.SetRenderParm( RENDERPARM_VIEWDEPTHSTENCILMAP, renderDestManager.viewDepthStencilImage );
	renderProgManager.SetRenderParm( RENDERPARM_LIGHTFALLOFFMAP, vLight->falloffImage );

	for( int i = 0; i < material->GetNumStages(); ++i )
	{
		auto const stage = material->GetStage( i );

		if( stage->SkipStage( regs ) )
		{
			continue;
		}

		//GL_State( GLS_DEPTHMASK | stage->drawStateBits | GLS_DEPTHFUNC_EQUAL );

		// texture 0 will get the projected texture
		renderProgManager.SetRenderParm( RENDERPARM_LIGHTPROJECTMAP, stage->texture.image );

		renderProgManager.SetupTextureMatrixParms( regs, &stage->texture );

		// get the modulate values from the light, including alpha, unlike normal lights
		idRenderVector lightColor;
		stage->GetColorParm( regs, lightColor );
		renderProgManager.SetColorParm( lightColor );

		if( bCameraInsideLightGeometry )
		{
			// Render backfaces with depth tests disabled since the camera is inside (or close to inside) the light geometry
			GL_State( GLS_DEPTHMASK | stage->drawStateBits | GLS_DEPTHFUNC_ALWAYS | GLS_BACKSIDED );
			//renderProgManager.SetColorParm( idColor::white.ToVec4() );
		}
		else {
			// Render frontfaces with depth tests on to get the speedup from HiZ since the camera is outside the light geometry
			GL_State( GLS_DEPTHMASK | stage->drawStateBits | GLS_DEPTHFUNC_LESS | GLS_FRONTSIDED );
			//renderProgManager.SetColorParm( idColor::purple.ToVec4() );
		}

		GL_DrawZeroOneCube();
	}

	GL_DepthBoundsTest( 0.0, 0.0 );

	GL_ResetTextureState();
	GL_ResetProgramState();

	RENDERLOG_CLOSE_BLOCK();
}

/*
=========================================================================================================

	FOG LIGHTS

=========================================================================================================
*/

/*
=====================
 RB_T_BasicFog
=====================
*/
static void RB_T_BasicFog( const drawSurf_t* drawSurfs, const idPlane fogPlanes[ 4 ] )
{
	backEnd.ClearCurrentSpace();

	for( const drawSurf_t* drawSurf = drawSurfs; drawSurf != nullptr; drawSurf = drawSurf->nextOnLight )
	{
		if( drawSurf->scissorRect.IsEmpty() )
		{
			continue;	// !@# FIXME: find out why this is sometimes being hit!
						// temporarily jump over the scissor and draw so the gl error callback doesn't get hit
		}

		RENDERLOG_OPEN_BLOCK( drawSurf->material->GetName() );

		RB_SetScissor( drawSurf->scissorRect );

		if( drawSurf->space != backEnd.currentSpace )
		{
			idRenderPlane localFogPlanes[ 4 ];

			renderProgManager.SetMVPMatrixParms( drawSurf->space->mvp );

			for( int i = 0; i < 4; i++ )
			{
				_GlobalPlaneToLocal( drawSurf->space->modelMatrix, fogPlanes[ i ], localFogPlanes[ i ] );
				//modelMatrix.InverseTransformPlane( fogPlanes[ i ], localFogPlanes[ i ] );
			}

			renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_S, localFogPlanes[ 0 ].ToFloatPtr() );
			renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_T, localFogPlanes[ 1 ].ToFloatPtr() );

			renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_1_T, localFogPlanes[ 2 ].ToFloatPtr() );
			renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_1_S, localFogPlanes[ 3 ].ToFloatPtr() );

			backEnd.currentSpace = drawSurf->space;
		}

		GL_SetRenderProgram( drawSurf->jointCache ? renderProgManager.prog_fog_skinned : renderProgManager.prog_fog );

		GL_DrawIndexed( drawSurf );

		RENDERLOG_CLOSE_BLOCK();
	}
}

static void RB_T_BasicFog_Finish( const drawSurf_t* drawSurf, const idPlane fogPlanes[ 4 ], const idRenderMatrix & inverseBaseLightProject )
{
	RENDERLOG_OPEN_BLOCK( "BasicFog_Finish( zeroOneCubeSurface )" );

	backEnd.ClearCurrentSpace();

	RB_SetScissor( drawSurf->scissorRect );

	idRenderMatrix::Multiply( backEnd.viewDef->GetMVPMatrix(), inverseBaseLightProject,
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_X )->GetVec4(),
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_Y )->GetVec4(),
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_Z )->GetVec4(),
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_W )->GetVec4() );

	idRenderPlane localFogPlanes[ 4 ];

	inverseBaseLightProject.InverseTransformPlane( fogPlanes[ 0 ], localFogPlanes[ 0 ], false );
	inverseBaseLightProject.InverseTransformPlane( fogPlanes[ 1 ], localFogPlanes[ 1 ], false );
	inverseBaseLightProject.InverseTransformPlane( fogPlanes[ 2 ], localFogPlanes[ 2 ], false );
	inverseBaseLightProject.InverseTransformPlane( fogPlanes[ 3 ], localFogPlanes[ 3 ], false );

	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_S, localFogPlanes[ 0 ].ToFloatPtr() );
	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_T, localFogPlanes[ 1 ].ToFloatPtr() );
	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_1_T, localFogPlanes[ 2 ].ToFloatPtr() );
	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_1_S, localFogPlanes[ 3 ].ToFloatPtr() );

	GL_SetRenderProgram( drawSurf->jointCache ? renderProgManager.prog_fog_skinned : renderProgManager.prog_fog );

	GL_DrawIndexed( drawSurf );

	RENDERLOG_CLOSE_BLOCK();
}

static void RB_ScreenSpaceBasicFog( const viewLight_t* vLight, const idPlane fogPlanes[ 4 ] )
{
	RENDERLOG_OPEN_BLOCK( vLight->lightShader->GetName() );

	const auto * const material = vLight->lightShader;
	const float* regs = vLight->shaderRegisters;

	backEnd.ClearCurrentSpace();

	GL_SetRenderProgram( renderProgManager.prog_fog2 );

	// set the matrix for deforming the 'zeroOneCubeModel' into the frustum to exactly cover the light volume
	idRenderMatrix::Multiply( backEnd.viewDef->GetMVPMatrix(), vLight->inverseBaseLightProject,
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_X )->GetVec4(),
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_Y )->GetVec4(),
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_Z )->GetVec4(),
		renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_W )->GetVec4() );

	// calculate the global light bounds by inverse projecting the zero to one cube with the 'inverseBaseLightProject'
	idBounds globalLightBounds;
	idRenderMatrix::ProjectedBounds( globalLightBounds, vLight->inverseBaseLightProject, bounds_zeroOneCube, false );
	globalLightBounds.ExpandSelf( r_blmul.GetFloat() );
	const bool bCameraInsideLightGeometry = globalLightBounds.ContainsPoint( backEnd.viewDef->GetOrigin() );

	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_S, fogPlanes[ 0 ].ToFloatPtr() );
	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_T, fogPlanes[ 1 ].ToFloatPtr() );

	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_1_T, fogPlanes[ 2 ].ToFloatPtr() );
	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_1_S, fogPlanes[ 3 ].ToFloatPtr() );

	RB_SetScissor( vLight->scissorRect );
	GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );

	renderProgManager.SetRenderParm( RENDERPARM_VIEWDEPTHSTENCILMAP, renderDestManager.viewDepthStencilImage );

	if( bCameraInsideLightGeometry )
	{
		// Render backfaces with depth tests disabled since the camera is inside (or close to inside) the light geometry
		GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_GREATER | GLS_BACKSIDED );
		//renderProgManager.SetColorParm( idColor::white.ToVec4() );
	}
	else {
		// Render frontfaces with depth tests on to get the speedup from HiZ since the camera is outside the light geometry
		GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_LESS | GLS_FRONTSIDED );
		//renderProgManager.SetColorParm( idColor::purple.ToVec4() );
	}

	GL_DrawZeroOneCube();

	GL_DepthBoundsTest( 0.0, 0.0 );

	GL_ResetTextureState();
	GL_ResetProgramState();

	RENDERLOG_CLOSE_BLOCK();
}

/*
==================
 RB_FogPass
==================
*/
static void RB_FogPass( const drawSurf_t* drawSurfs, const drawSurf_t* drawSurfs2, const viewLight_t* vLight )
{
	RENDERLOG_OPEN_BLOCK( vLight->lightShader->GetName() );

	// find the current color and density of the fog

	// assume fog shaders have only a single stage
	auto const stage = vLight->lightShader->GetStage( 0 );

	idRenderVector lightColor;
	stage->GetColorParm( vLight->shaderRegisters, lightColor );
	renderProgManager.SetColorParm( lightColor );

	// Calculate the falloff planes.
	// If they left the default value on, set a fog distance of 500 otherwise, distance = alpha color
	const float a = ( lightColor[ 3 ] <= 1.0f )? ( -0.5f / DEFAULT_FOG_DISTANCE ) : ( -0.5f / lightColor[ 3 ] );

	// texture 0 is the falloff image
	renderProgManager.SetRenderParm( RENDERPARM_FOGMAP, renderImageManager->fogImage );
	// texture 1 is the entering plane fade correction
	renderProgManager.SetRenderParm( RENDERPARM_FOGENTERMAP, renderImageManager->fogEnterImage );

	idRenderPlane fogPlane;

	// the fog plane is the light far clip plane
	const idRenderPlane plane(
		vLight->baseLightProject[ 2 ][ 0 ] - vLight->baseLightProject[ 3 ][ 0 ],
		vLight->baseLightProject[ 2 ][ 1 ] - vLight->baseLightProject[ 3 ][ 1 ],
		vLight->baseLightProject[ 2 ][ 2 ] - vLight->baseLightProject[ 3 ][ 2 ],
		vLight->baseLightProject[ 2 ][ 3 ] - vLight->baseLightProject[ 3 ][ 3 ]
	);
	const float planeScale = idMath::InvSqrt( plane.Normal().LengthSqr() );
	fogPlane[ 0 ] = plane[ 0 ] * planeScale;
	fogPlane[ 1 ] = plane[ 1 ] * planeScale;
	fogPlane[ 2 ] = plane[ 2 ] * planeScale;
	fogPlane[ 3 ] = plane[ 3 ] * planeScale;

	// S is based on the view origin
	const float s = fogPlane.Distance( backEnd.viewDef->GetOrigin() );

	const float FOG_SCALE = 0.001f;

	idRenderPlane fogPlanes[ 4 ];

	// S-0
	fogPlanes[ 0 ][ 0 ] = a * backEnd.viewDef->GetViewMatrix()[ 2 ][ 0 ];
	fogPlanes[ 0 ][ 1 ] = a * backEnd.viewDef->GetViewMatrix()[ 2 ][ 1 ];
	fogPlanes[ 0 ][ 2 ] = a * backEnd.viewDef->GetViewMatrix()[ 2 ][ 2 ];
	fogPlanes[ 0 ][ 3 ] = a * backEnd.viewDef->GetViewMatrix()[ 2 ][ 3 ] + 0.5f;
	// T-0
	fogPlanes[ 1 ][ 0 ] = a * backEnd.viewDef->GetViewMatrix()[ 0 ][ 0 ];		  // 0.0f
	fogPlanes[ 1 ][ 1 ] = a * backEnd.viewDef->GetViewMatrix()[ 0 ][ 1 ];		  // 0.0f
	fogPlanes[ 1 ][ 2 ] = a * backEnd.viewDef->GetViewMatrix()[ 0 ][ 2 ];		  // 0.0f
	fogPlanes[ 1 ][ 3 ] = a * backEnd.viewDef->GetViewMatrix()[ 0 ][ 3 ] + 0.5f;  // 0.5f

	// T-1 will get a texgen for the fade plane, which is always the "top" plane on unrotated lights
	fogPlanes[ 2 ][ 0 ] = FOG_SCALE * fogPlane[ 0 ];
	fogPlanes[ 2 ][ 1 ] = FOG_SCALE * fogPlane[ 1 ];
	fogPlanes[ 2 ][ 2 ] = FOG_SCALE * fogPlane[ 2 ];
	fogPlanes[ 2 ][ 3 ] = FOG_SCALE * fogPlane[ 3 ] + FOG_ENTER;
	// S-1
	fogPlanes[ 3 ][ 0 ] = 0.0f;
	fogPlanes[ 3 ][ 1 ] = 0.0f;
	fogPlanes[ 3 ][ 2 ] = 0.0f;
	fogPlanes[ 3 ][ 3 ] = FOG_SCALE * s + FOG_ENTER;

	// draw it
	if( r_useScreenSpaceFogLights.GetBool() )
	{
		RB_ScreenSpaceBasicFog( vLight, fogPlanes );
	}
	else {
		GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL | GLS_FRONTSIDED );
		RB_T_BasicFog( drawSurfs, fogPlanes );
		RB_T_BasicFog( drawSurfs2, fogPlanes );
	}

	// the light frustum bounding planes aren't in the depth buffer, so use depthfunc_less instead
	// of depthfunc_equal
	GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_LESS | GLS_BACKSIDED );

	auto zoSurf = backEnd.zeroOneCubeSurface;
	zoSurf.space = &backEnd.viewDef->GetWorldSpace();
	zoSurf.scissorRect = backEnd.viewDef->GetScissor();

	RB_T_BasicFog_Finish( &zoSurf, fogPlanes, vLight->inverseBaseLightProject );

	GL_ResetTextureState();
	GL_ResetProgramState();

	RENDERLOG_CLOSE_BLOCK();
}

/*
=========================================================================================================
	RB_FogAllLights
=========================================================================================================
*/
void RB_FogAllLights()
{
	if( r_showOverDraw.GetInteger() != 0 || backEnd.viewDef->isXraySubview /* don't fog in xray mode*/ )
	{
		return;
	}
	if( r_skipFogLights.GetBool() && r_skipBlendLights.GetBool() )
	{
		return;
	}

	RENDERLOG_OPEN_MAINBLOCK( MRB_FOG_ALL_LIGHTS );
	RENDERLOG_OPEN_BLOCK( "RB_FogAllLights" );

	RB_ResetBaseRenderDest( r_useHDR.GetBool() );
	RB_ResetViewportAndScissorToDefaultCamera( backEnd.viewDef );

	GL_ResetTextureState();

	// force fog plane to recalculate
	backEnd.ClearCurrentSpace();

	for( viewLight_t* vLight = backEnd.viewDef->viewLights; vLight != NULL; vLight = vLight->next )
	{
		if( vLight->lightShader->IsFogLight() && !r_skipFogLights.GetBool() )
		{
			RB_FogPass( vLight->globalInteractions, vLight->localInteractions, vLight );
		}
		else if( vLight->lightShader->IsBlendLight() && !r_skipBlendLights.GetBool() )
		{
			if( r_useScreenSpaceBlendLights.GetBool() )
			{
				if( vLight->scissorRect.IsValid() )
				{
					RB_ScreenSpaceBlendLight( vLight );
				}
			}
			else {
				RB_BlendLight( vLight->globalInteractions, vLight->localInteractions, vLight );
			}
		}
	}

	backEnd.hasFogging = false;

	GL_ResetTextureState();
	GL_ResetProgramState();

	RENDERLOG_CLOSE_BLOCK();
	RENDERLOG_CLOSE_MAINBLOCK();
}

