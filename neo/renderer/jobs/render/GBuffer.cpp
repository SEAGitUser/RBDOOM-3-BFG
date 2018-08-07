
#include "precompiled.h"
#pragma hdrstop

#include "Render.h"

/*
=========================================================================================

	GBUFFER RENDERING

=========================================================================================
*/

idCVar r_useClearGBufferWithProg( "r_useClearGBufferWithProg", "0", CVAR_RENDERER | CVAR_BOOL, "" );
ID_INLINE void ClearGBuffer( const bool bClearWithProg )
{
	if( bClearWithProg )
	{
		// Also, this handles state changing.
		GL_SetRenderProgram( renderProgManager.prog_gbuffer_clear );
		GL_DrawUnitSquare( LAYOUT_DRAW_VERT_FULL );
	}
	else {
		GL_ClearColor( 0.0f, 0.0f, 0.0f, 0.0f, 0 );
		GL_ClearColor( 0.0f, 0.0f, 0.0f, 0.0f, 1 );
	}
}

/*
==========================================================
RB_FillGBuffer
==========================================================
*/
void RB_FillGBuffer( const idRenderView * const view, const drawSurf_t * const * drawSurfs, int numDrawSurfs )
{
	if( !numDrawSurfs )
		return;

	RENDERLOG_OPEN_MAINBLOCK( MRB_FILL_GBUFFER );
	RENDERLOG_OPEN_BLOCK( "RB_FillGBuffer" );

	GL_SetRenderDestination( renderDestManager.renderDestGBuffer );
	RB_ResetViewportAndScissorToDefaultCamera( view );

	GL_State( GLS_DISABLE_BLENDING | GLS_DEPTHMASK | GLS_DEPTHFUNC_EQUAL );

	//ClearGBuffer( r_useClearGBufferWithProg.GetBool() );
	GL_Clear( true, false, false, 0, 0.0, 0.0, 0.0, 0.0 );

	// We render trivial surfaces first.

	RB_SetupForFastPathInteractions();

	const drawSurf_t** complexSurfaces = ( const drawSurf_t** )_alloca( numDrawSurfs * sizeof( drawSurf_t* ) );
	uint32 numComplexSurfaces = 0;

	// --------------------------------------------------------
	// Draw all non translucent trivial surfaces
	// --------------------------------------------------------

	backEnd.ClearCurrentSpace();
	RENDERLOG_OPEN_BLOCK( "Trivial Materials" );
#if 0
	for( int i = 0; i < numDrawSurfs; ++i )
	{
		auto const drawSurf = drawSurfs[ i ];
		assert( drawSurf != nullptr );
#else
	while( numDrawSurfs > 0 ) //SEA: front to back!
	{
		auto const drawSurf = drawSurfs[ --numDrawSurfs ];
		assert( drawSurf != nullptr );
#endif
		auto const surfaceMaterial = drawSurf->material;
		assert( surfaceMaterial != nullptr );

		auto const surfaceRegisters = drawSurf->shaderRegisters;
		assert( surfaceRegisters != nullptr );

		// translucent surfaces don't put anything in the gbuffer.
		if( surfaceMaterial->Coverage() == MC_TRANSLUCENT )
		{
			continue;
		}

		// if all stages of a material have been conditioned off, don't do anything
		if( surfaceMaterial->ConditionedOff( surfaceRegisters ) )
		{
			continue;
		}

		if( !surfaceMaterial->GetFastPathBumpImage() )
		{
			complexSurfaces[ numComplexSurfaces ] = drawSurf;
			++numComplexSurfaces;
			continue;
		}

		// change the matrix if needed
		if( drawSurf->space != backEnd.currentSpace )
		{
			backEnd.currentSpace = drawSurf->space;

			renderProgManager.SetModelMatrixParms( drawSurf->space->modelMatrix );
			renderProgManager.SetMVPMatrixParms( drawSurf->space->mvp );
		}

		RENDERLOG_OPEN_BLOCK( surfaceMaterial->GetName() );

		GL_SetRenderProgram( drawSurf->HasSkinning() ? renderProgManager.prog_fill_gbuffer : renderProgManager.prog_fill_gbuffer_trivial );

		renderProgManager.SetRenderParm( RENDERPARM_BUMPMAP, surfaceMaterial->GetFastPathBumpImage() );
		renderProgManager.SetRenderParm( RENDERPARM_DIFFUSEMAP, surfaceMaterial->GetFastPathDiffuseImage() );
		renderProgManager.SetRenderParm( RENDERPARM_SPECULARMAP, surfaceMaterial->GetFastPathSpecularImage() );

		GL_DrawIndexed( drawSurf );

		RENDERLOG_CLOSE_BLOCK();
	}
	RENDERLOG_CLOSE_BLOCK();

	// --------------------------------------------------------
	// Draw all non translucent complex surfaces
	// --------------------------------------------------------

	GL_SetRenderProgram( renderProgManager.prog_fill_gbuffer );

	drawInteraction_t inter;
	backEnd.ClearCurrentSpace();
	RENDERLOG_OPEN_BLOCK( "Complex Materials" );
	for( int i = 0; i < numComplexSurfaces; ++i )
	{
		auto const drawSurf = complexSurfaces[ i ];
		assert( drawSurf != nullptr );

		auto const surfaceMaterial = drawSurf->material;
		assert( surfaceMaterial != nullptr );

		auto const surfaceRegisters = drawSurf->shaderRegisters;
		assert( surfaceRegisters != nullptr );

		// change the matrix if needed
		if( drawSurf->space != backEnd.currentSpace )
		{
			backEnd.currentSpace = drawSurf->space;

			renderProgManager.SetModelMatrixParms( drawSurf->space->modelMatrix );
			renderProgManager.SetMVPMatrixParms( drawSurf->space->mvp );
		}

		inter.surf = drawSurf;
		inter.bumpImage->Set( ( idImage * )NULL );
		inter.diffuseImage->Set( ( idImage * )NULL );
		inter.specularImage->Set( ( idImage * )NULL );
		inter.diffuseColor->Set( 0.0f );
		inter.specularColor->Set( 0.0f );

		RB_DrawComplexMaterialInteraction( inter, surfaceRegisters, surfaceMaterial );
	}
	RENDERLOG_CLOSE_BLOCK();

	GL_ResetTextureState();
	GL_ResetProgramState();

	RENDERLOG_CLOSE_BLOCK();
	RENDERLOG_CLOSE_MAINBLOCK();
}





/*
=========================================================================================

	AMBIENT PASS RENDERING

=========================================================================================
*/

/*
==================
 RB_AmbientPass
==================
*/
void RB_AmbientPass( const idRenderView * view, const drawSurf_t* const* drawSurfs, int numDrawSurfs )
{
	extern idCVar r_forceAmbient;
	extern idCVar r_useSSAO;

	if( r_forceAmbient.GetFloat() <= 0.0f || r_skipAmbient.GetBool() )
		return;

	if( !numDrawSurfs )
		return;

	RENDERLOG_OPEN_MAINBLOCK( MRB_AMBIENT_PASS );
	RENDERLOG_OPEN_BLOCK( "RB_AmbientPass" );

	// fill geometry buffer with normal/roughness information and ambient color
	GL_SetRenderDestination( renderDestManager.renderDestGBufferSml );
	RB_ResetViewportAndScissorToDefaultCamera( view );

	GL_State( GLS_DISABLE_BLENDING | GLS_DEPTHMASK | GLS_STENCILMASK | GLS_DEPTHFUNC_EQUAL );

	//SEA: it clears subview rendering results!
	//ClearGBuffer( r_useClearGBufferWithProg.GetBool() );

	renderProgManager.SetColorParm( idColor::white.ToVec4() );

	///const float lightScale = 1.0f; //r_lightScale.GetFloat();
	///const idRenderVector lightColor = idColor::white.ToVec4() * lightScale;
	// apply the world-global overbright and the 2x factor for specular
	///const idRenderVector diffuseColor = lightColor;
	///const idRenderVector specularColor = lightColor * 2.0f;

	auto ambientColor = renderProgManager.GetRenderParm( RENDERPARM_AMBIENT_COLOR )->GetVecPtr();
	float ambientBoost = 1.0f;
	ambientBoost += r_useSSAO.GetBool() ? 0.2f : 0.0f;
	ambientBoost *= 1.1f; //r_useHDR.GetBool() ? 1.1f : 1.0f;
	ambientColor[ 0 ] = r_forceAmbient.GetFloat() * ambientBoost;
	ambientColor[ 1 ] = r_forceAmbient.GetFloat() * ambientBoost;
	ambientColor[ 2 ] = r_forceAmbient.GetFloat() * ambientBoost;
	ambientColor[ 3 ] = 1.0;

	// setup renderparms assuming we will be drawing trivial surfaces first
	RB_SetupForFastPathInteractions();

	const drawSurf_t** complexSurfaces = ( const drawSurf_t** )_alloca( numDrawSurfs * sizeof( drawSurf_t* ) );
	uint32 numComplexSurfaces = 0;

	backEnd.ClearCurrentSpace();
	RENDERLOG_OPEN_BLOCK( "Trivial Materials" );
#if 0
	for( int i = 0; i < numDrawSurfs; ++i )
	{
		auto const drawSurf = drawSurfs[ i ];
		assert( drawSurf != nullptr );
#else
	while( numDrawSurfs > 0 ) //SEA: front to back!
	{
		auto const drawSurf = drawSurfs[ --numDrawSurfs ];
		assert( drawSurf != nullptr );
#endif
		auto const surfaceMaterial = drawSurf->material;
		assert( surfaceMaterial != nullptr );

		auto const surfaceRegisters = drawSurf->shaderRegisters;
		assert( surfaceRegisters != nullptr );

		// translucent surfaces don't put anything in the gbuffer.
		if( surfaceMaterial->Coverage() == MC_TRANSLUCENT )
		{
			continue;
		}

		// if all stages of a material have been conditioned off, don't do anything
		/*if( surfaceMaterial->ConditionedOff( surfaceRegisters ) )
		{
		continue;
		}*/

		if( !surfaceMaterial->GetFastPathBumpImage() )
		{
			complexSurfaces[ numComplexSurfaces ] = drawSurf;
			++numComplexSurfaces;
			continue;
		}

		///bool isWorldModel = ( drawSurf->space->entityDef->parms.origin == vec3_origin );

		GL_SetRenderProgram( drawSurf->HasSkinning() ? renderProgManager.prog_gbuffer_sml_skinned : renderProgManager.prog_gbuffer_sml );

		// change the matrix if needed
		if( drawSurf->space != backEnd.currentSpace )
		{
			backEnd.currentSpace = drawSurf->space;
			renderProgManager.SetSurfaceSpaceMatrices( drawSurf );

			// tranform the view origin into model local space
			idRenderVector localViewOrigin( 1.0f );
			drawSurf->space->modelMatrix.InverseTransformPoint( backEnd.viewDef->GetOrigin(), localViewOrigin.ToVec3() );
			renderProgManager.SetRenderParm( RENDERPARM_LOCALVIEWORIGIN, localViewOrigin );
		}

		RENDERLOG_OPEN_BLOCK( surfaceMaterial->GetName() );

		renderProgManager.SetRenderParm( RENDERPARM_BUMPMAP, surfaceMaterial->GetFastPathBumpImage() );
		renderProgManager.SetRenderParm( RENDERPARM_DIFFUSEMAP, surfaceMaterial->GetFastPathDiffuseImage() );
		renderProgManager.SetRenderParm( RENDERPARM_SPECULARMAP, surfaceMaterial->GetFastPathSpecularImage() );

		GL_DrawIndexed( drawSurf );

		RENDERLOG_CLOSE_BLOCK();
	}
	RENDERLOG_CLOSE_BLOCK();

	drawInteraction_t inter;

	backEnd.ClearCurrentSpace();
	RENDERLOG_OPEN_BLOCK( "Complex Materials" );
	for( int i = 0; i < numComplexSurfaces; ++i )
	{
		auto const drawSurf = complexSurfaces[ i ];
		assert( drawSurf != nullptr );

		auto const surfaceMaterial = drawSurf->material;
		assert( surfaceMaterial != nullptr );

		auto const surfaceRegisters = drawSurf->shaderRegisters;
		assert( surfaceRegisters != nullptr );

		// if all stages of a material have been conditioned off, don't do anything
		if( surfaceMaterial->ConditionedOff( surfaceRegisters ) )
		{
			continue;
		}

		GL_SetRenderProgram( drawSurf->HasSkinning() ? renderProgManager.prog_gbuffer_sml_skinned : renderProgManager.prog_gbuffer_sml );

		// change the matrix if needed
		if( drawSurf->space != backEnd.currentSpace )
		{
			backEnd.currentSpace = drawSurf->space;
			renderProgManager.SetSurfaceSpaceMatrices( drawSurf );

			// tranform the view origin into model local space
			idRenderVector localViewOrigin( 1.0f );
			drawSurf->space->modelMatrix.InverseTransformPoint( view->GetOrigin(), localViewOrigin.ToVec3() );
			renderProgManager.SetRenderParm( RENDERPARM_LOCALVIEWORIGIN, localViewOrigin );
		}

		inter.surf = drawSurf;
		inter.bumpImage->Set( ( idImage * )NULL );
		inter.diffuseImage->Set( ( idImage * )NULL );
		inter.specularImage->Set( ( idImage * )NULL );
		inter.diffuseColor->Set( 0.0f );
		inter.specularColor->Set( 0.0f );

		RB_DrawComplexMaterialInteraction( inter, surfaceRegisters, surfaceMaterial );
	}
	RENDERLOG_CLOSE_BLOCK();

	renderProgManager.DisableAlphaTestParm();

	GL_ResetTextureState();
	GL_ResetProgramState();

	RENDERLOG_CLOSE_BLOCK();
	RENDERLOG_CLOSE_MAINBLOCK();
	}