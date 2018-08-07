
#include "precompiled.h"
#pragma hdrstop

#include "Render.h"

/*
=========================================================================================

	DEPTH BUFFER RENDERING

=========================================================================================
*/


/*
=====================================================================================================
  http://rastergrid.com/blog/2010/10/hierarchical-z-map-based-occlusion-culling/

  The Hi-Z map can be constructed using OpenGL framebuffer objects by rendering a full-screen quad pass for each mip
  level where the previous mip level is bound as the input texture and the current mip level is bound as render target.
  As OpenGL does allow rendering from and to the same texture object as far as we don’t access the same mip level for
  both reading and writing, the algorithm simply looks like the following:
=====================================================================================================
*/
void RB_BuildHiZTexture()
{
	RENDERLOG_OPEN_BLOCK( "RB_BuildHiZTexture" );

	const auto viewDepthImg = renderDestManager.viewDepthStencilImage;

	int currentWidth = viewDepthImg->GetUploadWidth();
	int currentHeight = viewDepthImg->GetUploadHeight();
	//int currentWidth = backEnd.viewDef->GetViewport().GetWidth();
	//int currentHeight = backEnd.viewDef->GetViewport().GetHeight();

	GL_SetRenderDestination( renderDestManager.renderDestStagingDepth );
	GL_SetRenderProgram( renderProgManager.prog_HiZmap_construction );
	GL_State( GLS_DISABLE_BLENDING | GLS_COLORMASK | GLS_ALPHAMASK | GLS_STENCILMASK | GLS_DEPTHFUNC_ALWAYS );

	renderProgManager.GetRenderParm( RENDERPARM_USERMAP0 )->Set( viewDepthImg );
	auto parm = renderProgManager.GetRenderParm( RENDERPARM_USERVEC0 )->GetVecPtr();

	// calculate the number of mipmap levels for NPOT texture
	int numLevels = GetMipCountForResolution( currentWidth, currentHeight );
	RENDERLOG_PRINT( "Num levels for resolution %ix%i: %i\n", currentWidth, currentHeight, numLevels );
	for( int i = 1; i < numLevels; i++ )
	{
		parm[ 0 ] = currentWidth;
		parm[ 1 ] = currentHeight;

		// calculate next viewport size
		currentWidth = currentWidth >> 1;
		currentHeight = currentHeight >> 1;

		// ensure that the viewport size is always at least 1x1
		currentWidth = ( currentWidth > 0 )? currentWidth : 1;
		currentHeight = ( currentHeight > 0 )? currentHeight : 1;

		RENDERLOG_PRINT( "Build Mip%i: %ix%i\n", i, currentWidth, currentHeight );

		// restrict fetches only to previous level
		parm[ 2 ] = ( float )( i - 1 );
		parm[ 3 ] = 0.0f;

		// bind next level for rendering
		renderDestManager.renderDestStagingDepth->SetDepthStencilGL( viewDepthImg, 0, i );

		// draw full-screen triangle

		GL_ViewportAndScissor( 0, 0, currentWidth, currentHeight );

		//GL_DrawScreenTriangleAuto();
		GL_DrawUnitSquare();
	}

	GL_ResetProgramState();
	GL_ResetTextureState();

	RENDERLOG_CLOSE_BLOCK();
}


//idCVar r_drawWorldModelsOnly( "r_drawWorldModelsOnly", "0", CVAR_RENDERER | CVAR_BOOL, "" );
idCVar r_useMultiDraw( "r_useMultiDraw", "1", CVAR_RENDERER | CVAR_BOOL, "use batched draw calls" );

/*
==================
 DrawSurfaceGeneric
==================
*/
static void DrawSurfaceGeneric( const drawSurf_t * const drawSurf )
{
	auto const material = drawSurf->material;
	assert( material != nullptr );

	// translucent surfaces don't put anything in the depth buffer and don't
	// test against it, which makes them fail the mirror clip plane operation
	if( material->Coverage() == MC_TRANSLUCENT )
	{
		return;
	}

	// get the expressions for conditionals / color / texcoords
	const float * regs = drawSurf->shaderRegisters;
	assert( regs != nullptr );

	// if all stages of a material have been conditioned off, don't do anything
	if( material->ConditionedOff( regs ) )
	{
		return;
	}

	// change the matrix if needed
	if( drawSurf->space != backEnd.currentSpace )
	{
		backEnd.currentSpace = drawSurf->space;
		renderProgManager.SetMVPMatrixParms( drawSurf->space->mvp );
	}

	uint64 surfGLState = 0;

	// set polygon offset if necessary
	if( material->TestMaterialFlag( MF_POLYGONOFFSET ) )
	{
		surfGLState |= GLS_POLYGON_OFFSET;
		GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * material->GetPolygonOffset() );
	}

	// subviews will just down-modulate the color buffer
	idRenderVector color;
	if( material->GetSort() == SS_SUBVIEW )
	{
		surfGLState |= ( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_DEPTHFUNC_LEQUAL );
		color = idColor::white.ToVec4();
	}
	else { // others just draw black
		color = idColor::black.ToVec4();
	}

	RENDERLOG_OPEN_BLOCK( material->GetName() );

	bool drawSolid = false;
	if( material->Coverage() == MC_OPAQUE )
	{
		drawSolid = true;
	}
	else if( material->Coverage() == MC_PERFORATED )
	{
		// we may have multiple alpha tested stages
		// if the only alpha tested stages are condition register omitted,
		// draw a normal opaque surface
		bool didDraw = false;

		// perforated surfaces may have multiple alpha tested stages
		for( int stage = 0; stage < material->GetNumStages(); stage++ )
		{
			auto const pStage = material->GetStage( stage );

			if( !pStage->hasAlphaTest )
			{
				continue;
			}

			// check the stage enable condition
			if( pStage->SkipStage( regs ) )
			{
				continue;
			}

			// if we at least tried to draw an alpha tested stage,
			// we won't draw the opaque surface
			didDraw = true;

			// set the alpha modulate
			color[ 3 ] = pStage->GetColorParmAlpha( regs );

			// skip the entire stage if alpha would be black
			if( color[ 3 ] <= 0.0f )
			{
				continue;
			}

			GL_SetRenderProgram( drawSurf->HasSkinning() ? renderProgManager.prog_texture_color_skinned : renderProgManager.prog_texture_color );

			uint64 stageGLState = surfGLState;

			// set privatePolygonOffset if necessary
			if( pStage->privatePolygonOffset )
			{
				GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * pStage->privatePolygonOffset );
				stageGLState |= GLS_POLYGON_OFFSET;
			}

			renderProgManager.SetColorParm( color );

			GL_State( stageGLState );

			renderProgManager.EnableAlphaTestParm( pStage->GetAlphaTestValue( regs ) );

			renderProgManager.SetVertexColorParm( SVC_IGNORE );

			// bind the texture
			renderProgManager.SetRenderParm( RENDERPARM_MAP, pStage->texture.image );

			// set texture matrix and texGens
			RB_PrepareStageTexturing( pStage, drawSurf );

			// must render with less-equal for Z-Cull to work properly
			assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LEQUAL );

			// draw it
			GL_DrawIndexed( drawSurf );

			// clean up
			RB_FinishStageTexturing( pStage, drawSurf );

			// unset privatePolygonOffset if necessary
			if( pStage->privatePolygonOffset )
			{
				GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * material->GetPolygonOffset() );
			}
		}

		if( !didDraw )
		{
			drawSolid = true;
		}
	}

	// draw the entire surface solid
	if( drawSolid )
	{
		if( material->GetSort() == SS_SUBVIEW )
		{
			GL_SetRenderProgram( renderProgManager.prog_color );

			renderProgManager.SetColorParm( color );

			GL_State( surfGLState );
		}
		else {
			GL_SetRenderProgram( drawSurf->HasSkinning() ? renderProgManager.prog_depth_skinned : renderProgManager.prog_depth );

			GL_State( surfGLState | GLS_COLORMASK | GLS_ALPHAMASK );
		}

		// must render with less-equal for Z-Cull to work properly
		assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LEQUAL );

		// draw it
		GL_DrawIndexed( drawSurf );
	}

	RENDERLOG_CLOSE_BLOCK();
}

/*
==================
 RB_FillDepthBufferGeneric
==================
*/
ID_INLINE static void RB_FillDepthBufferGeneric( const drawSurf_t* const* drawSurfs, int numDrawSurfs )
{
	// force MVP change on first surface
	backEnd.ClearCurrentSpace();

	for( int i = 0; i < numDrawSurfs; ++i )
	{
		DrawSurfaceGeneric( drawSurfs[ i ] );
	}

	renderProgManager.DisableAlphaTestParm();
}

/*
=====================
 RB_FillDepthBufferFast

	Optimized fast path code.

	If there are subview surfaces, they must be guarded in the depth buffer to allow
	the mirror / subview to show through underneath the current view rendering.

	Surfaces with perforated shaders need the full shader setup done, but should be
	drawn after the opaque surfaces.

	The bulk of the surfaces should be simple opaque geometry that can be drawn very rapidly.

	If there are no subview surfaces, we could clear to black and use fast-Z rendering
	on the 360.
=====================
*/
void RB_FillDepthBufferFast( const drawSurf_t * const * drawSurfs, int numDrawSurfs )
{
	assert( numDrawSurfs < 0 );

	RENDERLOG_OPEN_MAINBLOCK( MRB_FILL_DEPTH_BUFFER );
	RENDERLOG_OPEN_BLOCK( "RB_FillDepthBufferFast" );

	GL_StartDepthPass( backEnd.viewDef->GetScissor() );

	// force MVP change on first surface
	backEnd.ClearCurrentSpace();

	RB_ResetBaseRenderDest( r_useHDR.GetBool() );
	RB_ResetViewportAndScissorToDefaultCamera( backEnd.viewDef );

	GL_State( GLS_DEFAULT | GLS_FRONTSIDED );

	// Clear the depth buffer and clear the stencil to 128 for stencil shadows as well as gui masking
	GL_Clear( false, true, true, STENCIL_SHADOW_TEST_VALUE, 0.0f, 0.0f, 0.0f, 0.0f );

	// draw all the subview surfaces, which will already be at the start of the sorted list,
	// with the general purpose path
	int	surfNum;
	for( surfNum = 0; surfNum < numDrawSurfs; ++surfNum )
	{
		if( drawSurfs[ surfNum ]->material->GetSort() != SS_SUBVIEW ) {
			break;
		}
		RB_FillDepthBufferGeneric( &drawSurfs[ surfNum ], 1 );
	}

	const drawSurf_t** perforatedSurfaces = ( const drawSurf_t** )_alloca( numDrawSurfs * sizeof( drawSurf_t* ) );
	int numPerforatedSurfaces = 0;

	// draw all the opaque surfaces and build up a list of perforated surfaces that
	// we will defer drawing until all opaque surfaces are done
	//GL_State( GLS_DEFAULT | GLS_COLORMASK | GLS_ALPHAMASK );

#if 1
	// continue checking past the subview surfaces
  #if 0
	for( ; surfNum < numDrawSurfs; ++surfNum ) //SEA: back to front!
	{
		auto const surf = drawSurfs[ surfNum ];
		auto const material = surf->material;
  #else
	while( numDrawSurfs > surfNum ) //SEA: front to back!
	{
		auto const surf = drawSurfs[ --numDrawSurfs ];
		auto const material = surf->material;
  #endif
		/*if( !surf->space->modelMatrix.IsIdentity( MATRIX_EPSILON ) && r_drawWorldModelsOnly.GetBool() ) {
			// lots of it !!!
			continue;
		}*/

		// translucent surfaces don't put anything in the depth buffer
		if( material->Coverage() == MC_TRANSLUCENT )
		{
			continue;
		}
		if( material->Coverage() == MC_PERFORATED )
		{
			// save for later drawing
			perforatedSurfaces[ numPerforatedSurfaces++ ] = surf;
			continue;
		}

		// set polygon offset?

		// set mvp matrix
		if( surf->space != backEnd.currentSpace ) {
			backEnd.currentSpace = surf->space;

			renderProgManager.SetMVPMatrixParms( surf->space->mvp );
		}

		RENDERLOG_OPEN_BLOCK( material->GetName() );

		//SEA: TODO! optimize with multidraw call
		//if( surf->space->modelMatrix.IsIdentity( MATRIX_EPSILON ) ) {
		//	RENDERLOG_PRINT( "ModelMatrix is Identity\n" );
		//}

		GL_SetRenderProgram( surf->jointCache ? renderProgManager.prog_depth_skinned : renderProgManager.prog_depth );

		// must render with less-equal for Z-Cull to work properly
		assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LEQUAL );

		// draw it solid
		GL_DrawIndexed( surf );

		RENDERLOG_CLOSE_BLOCK();
	}
#else

	const drawSurf_t** opaqueSurfaces = ( const drawSurf_t** )_alloca( numDrawSurfs * sizeof( drawSurf_t* ) );
	int numOpaqueSurfaces = 0;

	GL_StartMDBatch();

	GL_SetRenderProgram( renderProgManager.prog_depth_world );

	while( numDrawSurfs > surfNum ) //SEA: front to back!
	{
		auto const surf = drawSurfs[ --numDrawSurfs ];
		auto const shader = surf->material;

		// translucent surfaces don't put anything in the depth buffer
		if( shader->Coverage() == MC_TRANSLUCENT ) {
			continue;
		}

		if( shader->Coverage() == MC_PERFORATED )
		{
			// save for later drawing
			perforatedSurfaces[ numPerforatedSurfaces ] = surf;
			++numPerforatedSurfaces;
			continue;
		}

		if( !surf->space->modelMatrix.IsIdentity( MATRIX_EPSILON ) )
		{
			opaqueSurfaces[ numOpaqueSurfaces ] = surf;
			++numOpaqueSurfaces;
			continue;
		}

		// set polygon offset?

		RENDERLOG_PRINT( "%s\n", shader->GetName() );

		// must render with less-equal for Z-Cull to work properly
		assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LEQUAL );

		if( !r_useMultiDraw.GetBool() )
		{
			idVertexBuffer vertexBuffer;
			vertexCache.GetVertexBuffer( surf->vertexCache, vertexBuffer );
			const GLint vertOffset = vertexBuffer.GetOffset();
			const GLuint vbo = reinterpret_cast< GLuint >( vertexBuffer.GetAPIObject() );

			idIndexBuffer indexBuffer;
			vertexCache.GetIndexBuffer( surf->indexCache, indexBuffer );
			const GLintptr indexOffset = indexBuffer.GetOffset();
			const GLuint ibo = reinterpret_cast< GLuint >( indexBuffer.GetAPIObject() );

			//renderProgManager.CommitUniforms();

			// RB: 64 bit fixes, changed GLuint to GLintptr
			if( backEnd.glState.currentIndexBuffer != ( GLintptr )ibo )
			{
				glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo );
				backEnd.glState.currentIndexBuffer = ibo;
			}

			if( ( backEnd.glState.vertexLayout != LAYOUT_DRAW_VERT_POSITION_ONLY ) || ( backEnd.glState.currentVertexBuffer != ( GLintptr )vbo ) )
			{
				if( glConfig.ARB_vertex_attrib_binding && r_useVertexAttribFormat.GetBool() )
				{
					const GLintptr baseOffset = 0;
					const GLuint bindingIndex = 0;
					glBindVertexBuffer( bindingIndex, vbo, baseOffset, sizeof( idDrawVert ) );
				}
				else {
					glBindBuffer( GL_ARRAY_BUFFER, vbo );
				}
				backEnd.glState.currentVertexBuffer = ( GLintptr )vbo;

				GL_SetVertexLayout( LAYOUT_DRAW_VERT_POSITION_ONLY );
				backEnd.glState.vertexLayout = LAYOUT_DRAW_VERT_POSITION_ONLY;
			}

			const GLsizei primcount = 1;
			glDrawElementsInstancedBaseVertex( GL_TRIANGLES,
				r_singleTriangle.GetBool() ? 3 : surf->numIndexes,
				GL_INDEX_TYPE,
				( triIndex_t* )indexOffset,
				primcount,
				vertOffset / sizeof( idDrawVert ) );

			backEnd.pc.c_drawElements++;
			backEnd.pc.c_drawIndexes += surf->numIndexes;

			RENDERLOG_PRINT( "GL_DrawIndexed( VBO %u:%i, IBO %u:%i )\n", vbo, vertOffset, ibo, indexOffset );
		}
		else
		{
			GL_SetupMDParm( surf );
		}
	}

	if( r_useMultiDraw.GetBool() )
	{
		GL_FinishMDBatch();
	}

	//GL_State( GLS_DEFAULT );

	for( int i = 0; i < numOpaqueSurfaces; ++i )
	{
		auto const surf = opaqueSurfaces[ i ];
		auto const shader = surf->material;

		// set polygon offset?

		// set mvp matrix
		if( surf->space != backEnd.currentSpace ) {
			backEnd.currentSpace = surf->space;

			renderProgManager.SetMVPMatrixParms( surf->space->mvp );
		}

		RENDERLOG_OPEN_BLOCK( shader->GetName() );

		GL_SetRenderProgram( surf->jointCache ? renderProgManager.prog_depth_skinned : renderProgManager.prog_depth );

		// must render with less-equal for Z-Cull to work properly
		assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LEQUAL );

		// draw it solid
		GL_DrawIndexed( surf );

		RENDERLOG_CLOSE_BLOCK();
	}
#endif

	// draw all perforated surfaces with the general code path
	if( numPerforatedSurfaces > 0 )
	{
		RB_FillDepthBufferGeneric( perforatedSurfaces, numPerforatedSurfaces );
	}

	// Allow platform specific data to be collected after the depth pass.
	GL_FinishDepthPass();

	if( !backEnd.viewDef->isSubview )
	{
		RB_BuildHiZTexture();
	}


	RENDERLOG_CLOSE_BLOCK();
	RENDERLOG_CLOSE_MAINBLOCK();
}








#if 0

struct depthBufferPassParms_t
{
	const idRenderView * renderView;

	const drawSurf_t** subviewSurfaces = nullptr;
	uint32 numSubviewSurfaces = 0;

	const drawSurf_t** staticSurfaces = nullptr;
	uint32 numStaticSurfaces = 0;

	const drawSurf_t** opaqueSurfaces = nullptr;
	uint32 numOpaqueSurfaces = 0;

	const drawSurf_t** perforatedSurfaces = nullptr;
	uint32 numPerforatedSurfaces = 0;
};

void RB_RenderDepthBufferPass( const depthBufferPassParms_t * parms )
{
	RENDERLOG_OPEN_MAINBLOCK( MRB_FILL_DEPTH_BUFFER );
	RENDERLOG_OPEN_BLOCK( "RB_RenderDepthBufferFillPass" );

	GL_StartDepthPass( backEnd.viewDef->GetScissor() );

	//
	// Draw all the subview surfaces with the general purpose path
	//

	GL_State( GLS_DEFAULT );

	backEnd.ClearCurrentSpace();

	RB_FillDepthBufferGeneric( parms->subviewSurfaces, parms->numSubviewSurfaces );

	//
	// Draw all the world surfaces
	//

	GL_State( GLS_DEFAULT | GLS_COLORMASK | GLS_ALPHAMASK );

	GL_SetRenderProgram( renderProgManager.prog_depth_world );

	for( uint32 surfNum; surfNum < parms->numWorldSurfaces; ++surfNum )
	{
		auto const surf = parms->worldSurfaces[ surfNum ];
		auto const shader = surf->material;

		RENDERLOG_OPEN_BLOCK( shader->GetName() );

		// must render with less-equal for Z-Cull to work properly
		assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LEQUAL );

		// draw it solid
		GL_DrawIndexed( surf );

		RENDERLOG_CLOSE_BLOCK();
	}

	//
	// Draw all the opaque surfaces
	//

	GL_State( GLS_DEFAULT );
	backEnd.ClearCurrentSpace();

	for( uint32 surfNum = 0; surfNum < parms->numOpaqueSurfaces; ++surfNum )
	{
		auto const surf = parms->opaqueSurfaces[ surfNum ];
		auto const shader = surf->material;

		// set polygon offset?

		// set mvp matrix
		if( surf->space != backEnd.currentSpace )
		{
			backEnd.currentSpace = surf->space;
			renderProgManager.SetMVPMatrixParms( surf->space->mvp );
		}

		RENDERLOG_OPEN_BLOCK( shader->GetName() );

		GL_SetRenderProgram( surf->jointCache ? renderProgManager.prog_depth_skinned : renderProgManager.prog_depth );

		// must render with less-equal for Z-Cull to work properly
		assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LEQUAL );

		// draw it solid
		GL_DrawIndexed( surf );

		RENDERLOG_CLOSE_BLOCK();
	}

	// draw all perforated surfaces with the general code path
	RB_FillDepthBufferGeneric( parms->perforatedSurfaces, parms->numPerforatedSurfaces );

	// Allow platform specific data to be collected after the depth pass.
	GL_FinishDepthPass();

	RENDERLOG_CLOSE_BLOCK();
	RENDERLOG_CLOSE_MAINBLOCK();
}

#endif