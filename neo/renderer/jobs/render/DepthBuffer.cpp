
#include "precompiled.h"
#pragma hdrstop

#include "Render.h"

/*
=========================================================================================

	DEPTH BUFFER RENDERING

=========================================================================================
*/
//idCVar r_drawWorldModelsOnly( "r_drawWorldModelsOnly", "0", CVAR_RENDERER | CVAR_BOOL, "" );
idCVar r_useMultiDraw( "r_useMultiDraw", "1", CVAR_RENDERER | CVAR_BOOL, "use batched draw calls" );

/*
==================
 RB_FillDepthBufferGeneric
==================
*/
static void RB_FillDepthBufferGeneric( const drawSurf_t* const* drawSurfs, int numDrawSurfs )
{
	for( int i = 0; i < numDrawSurfs; ++i )
	{
		auto const drawSurf = drawSurfs[ i ];
		auto const shader = drawSurf->material;

		// translucent surfaces don't put anything in the depth buffer and don't
		// test against it, which makes them fail the mirror clip plane operation
		if( shader->Coverage() == MC_TRANSLUCENT )
		{
			continue;
		}

		// get the expressions for conditionals / color / texcoords
		const float* regs = drawSurf->shaderRegisters;

		// if all stages of a material have been conditioned off, don't do anything
		if( shader->ConditionedOff( regs ) )
		{
			continue;
		}

		// change the matrix if needed
		if( drawSurf->space != backEnd.currentSpace )
		{
			backEnd.currentSpace = drawSurf->space;

			RB_SetMVP( drawSurf->space->mvp );
		}

		uint64 surfGLState = 0;

		// set polygon offset if necessary
		if( shader->TestMaterialFlag( MF_POLYGONOFFSET ) )
		{
			surfGLState |= GLS_POLYGON_OFFSET;
			GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
		}

		// subviews will just down-modulate the color buffer
		idRenderVector color;
		if( shader->GetSort() == SS_SUBVIEW )
		{
			surfGLState |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_DEPTHFUNC_LESS;
			color = colorWhite;
		}
		else { // others just draw black
			color = colorBlack;
		}

		RENDERLOG_OPEN_BLOCK( shader->GetName() );

		bool drawSolid = false;
		if( shader->Coverage() == MC_OPAQUE )
		{
			drawSolid = true;
		}
		else if( shader->Coverage() == MC_PERFORATED )
		{
			// we may have multiple alpha tested stages
			// if the only alpha tested stages are condition register omitted,
			// draw a normal opaque surface
			bool didDraw = false;

			// perforated surfaces may have multiple alpha tested stages
			for( int stage = 0; stage < shader->GetNumStages(); stage++ )
			{
				auto const pStage = shader->GetStage( stage );

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

				uint64 stageGLState = surfGLState;

				// set privatePolygonOffset if necessary
				if( pStage->privatePolygonOffset )
				{
					GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * pStage->privatePolygonOffset );
					stageGLState |= GLS_POLYGON_OFFSET;
				}

				GL_Color( color );

				GL_State( stageGLState );

				idRenderVector alphaTestValue( regs[ pStage->alphaTestRegister ] );
				renderProgManager.SetRenderParm( RENDERPARM_ALPHA_TEST, alphaTestValue.ToFloatPtr() );

				renderProgManager.BindShader_TextureVertexColor( drawSurf->jointCache );

				RB_SetVertexColorParms( SVC_IGNORE );

				// bind the texture
				GL_BindTexture( 0, pStage->texture.image );

				// set texture matrix and texGens
				RB_PrepareStageTexturing( pStage, drawSurf );

				// must render with less-equal for Z-Cull to work properly
				assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );

				// draw it
				GL_DrawIndexed( drawSurf );

				// clean up
				RB_FinishStageTexturing( pStage, drawSurf );

				// unset privatePolygonOffset if necessary
				if( pStage->privatePolygonOffset )
				{
					GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
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
			if( shader->GetSort() == SS_SUBVIEW )
			{
				renderProgManager.BindShader_Color( false );

				GL_Color( color );
				GL_State( surfGLState );
			}
			else {
				renderProgManager.BindShader_Depth( drawSurf->jointCache );

				GL_State( surfGLState | GLS_ALPHAMASK );
			}

			// must render with less-equal for Z-Cull to work properly
			assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );

			// draw it
			GL_DrawIndexed( drawSurf );
		}

		RENDERLOG_CLOSE_BLOCK();
	}

	renderProgManager.SetRenderParm( RENDERPARM_ALPHA_TEST, vec4_zero.ToFloatPtr() );
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
	if( !numDrawSurfs ) {
		return;
	}
	// if we are just doing 2D rendering, no need to fill the depth buffer
	if( backEnd.viewDef->Is2DView() ) {
		return;
	}

	RENDERLOG_OPEN_MAINBLOCK( MRB_FILL_DEPTH_BUFFER );
	RENDERLOG_OPEN_BLOCK( "RB_FillDepthBufferFast" );

	GL_StartDepthPass( backEnd.viewDef->GetScissor() );

	// force MVP change on first surface
	backEnd.ClearCurrentSpace();

	// draw all the subview surfaces, which will already be at the start of the sorted list,
	// with the general purpose path
	GL_State( GLS_DEFAULT );

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

	const drawSurf_t** opaqueSurfaces = ( const drawSurf_t** )_alloca( numDrawSurfs * sizeof( drawSurf_t* ) );
	int numOpaqueSurfaces = 0;

	// draw all the opaque surfaces and build up a list of perforated surfaces that
	// we will defer drawing until all opaque surfaces are done
	//GL_State( GLS_DEFAULT | GLS_COLORMASK | GLS_ALPHAMASK );

#if 1
	// continue checking past the subview surfaces
	//for( ; surfNum < numDrawSurfs; ++surfNum ) { //SEA: back to front!
	//	auto const surf = drawSurfs[ surfNum ];

	while( numDrawSurfs > surfNum ) //SEA: front to back!
	{
		auto const surf = drawSurfs[ --numDrawSurfs ];
		auto const shader = surf->material;

		/*if( !surf->space->modelMatrix.IsIdentity( MATRIX_EPSILON ) && r_drawWorldModelsOnly.GetBool() )
		{
			// lots of it !!!
			continue;
		}*/

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

		// set polygon offset?

		// set mvp matrix
		if( surf->space != backEnd.currentSpace ) {
			backEnd.currentSpace = surf->space;

			RB_SetMVP( surf->space->mvp );
		}

		RENDERLOG_OPEN_BLOCK( shader->GetName() );

		//SEA: TODO! optimize with multidraw call
		//if( surf->space->modelMatrix.IsIdentity( MATRIX_EPSILON ) ) { 
		//	RENDERLOG_PRINT( "ModelMatrix is Identity\n" );
		//}

		renderProgManager.BindShader_Depth( surf->jointCache );

		// must render with less-equal for Z-Cull to work properly
		assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );

		// draw it solid
		GL_DrawIndexed( surf );

		RENDERLOG_CLOSE_BLOCK();
	}
#else

	GL_StartMDBatch();

	renderProgManager.BindShader_DepthWorld();

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
		assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );

		if( !r_useMultiDraw.GetBool() ) 
		{
			idVertexBuffer vertexBuffer;
			vertexCache.GetVertexBuffer( surf->vertexCache, &vertexBuffer );
			const GLint vertOffset = vertexBuffer.GetOffset();
			const GLuint vbo = reinterpret_cast< GLuint >( vertexBuffer.GetAPIObject() );

			idIndexBuffer indexBuffer;
			vertexCache.GetIndexBuffer( surf->indexCache, &indexBuffer );
			const GLintptr indexOffset = indexBuffer.GetOffset();
			const GLuint ibo = reinterpret_cast< GLuint >( indexBuffer.GetAPIObject() );

			//renderProgManager.CommitUniforms();

			// RB: 64 bit fixes, changed GLuint to GLintptr
			if( backEnd.glState.currentIndexBuffer != ( GLintptr )ibo || !r_useStateCaching.GetBool() )
			{
				glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo );
				backEnd.glState.currentIndexBuffer = ibo;
			}

			if( ( backEnd.glState.vertexLayout != LAYOUT_DRAW_VERT_POSITION_ONLY ) || ( backEnd.glState.currentVertexBuffer != ( GLintptr )vbo ) || !r_useStateCaching.GetBool() )
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

			RB_SetMVP( surf->space->mvp );
		}

		RENDERLOG_OPEN_BLOCK( shader->GetName() );

		renderProgManager.BindShader_Depth( surf->jointCache );

		// must render with less-equal for Z-Cull to work properly
		assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );

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

	RENDERLOG_CLOSE_BLOCK();
	RENDERLOG_CLOSE_MAINBLOCK();
}








#if 0

struct depthBufferPassParms_t
{
	const idRenderView * renderView;

	const drawSurf_t** subviewSurfaces = nullptr;
	uint32 numSubviewSurfaces = 0;

	const drawSurf_t** worldSurfaces = nullptr;
	uint32 numWorldSurfaces = 0;

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

	renderProgManager.BindShader_DepthWorld();

	for( uint32 surfNum; surfNum < parms->numWorldSurfaces; ++surfNum )
	{
		auto const surf = parms->worldSurfaces[ surfNum ];
		auto const shader = surf->material;

		RENDERLOG_OPEN_BLOCK( shader->GetName() );

		// must render with less-equal for Z-Cull to work properly
		assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );

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
			RB_SetMVP( surf->space->mvp );
		}

		RENDERLOG_OPEN_BLOCK( shader->GetName() );

		renderProgManager.BindShader_Depth( surf->jointCache );

		// must render with less-equal for Z-Cull to work properly
		assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );

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