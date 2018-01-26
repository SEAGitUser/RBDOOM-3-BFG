
#include "precompiled.h"
#pragma hdrstop

#include "Render.h"

/*
==============================================================================================

	STENCIL SHADOW RENDERING

==============================================================================================
*/

idCVar r_forceZPassStencilShadows( "r_forceZPassStencilShadows", "0", CVAR_RENDERER | CVAR_BOOL, "force Z-pass rendering for performance testing" );
idCVar r_useStencilShadowPreload( "r_useStencilShadowPreload", "1", CVAR_RENDERER | CVAR_BOOL, "use stencil shadow preload algorithm instead of Z-fail" );

/*
=====================
 RB_StencilShadowPass

	The stencil buffer should have been set to 128 on any surfaces that might receive shadows.
=====================
*/
void RB_StencilShadowPass( const drawSurf_t* const drawSurfs, const viewLight_t* const vLight )
{
	if( r_skipShadows.GetBool() ) {
		return;
	}

	if( !drawSurfs ) {
		return;
	}

	RENDERLOG_OPEN_BLOCK( "RB_StencilShadowPass" );

	renderProgManager.BindShader_Shadow( false );

	GL_ResetTexturesState();

	uint64 glState = 0;

	// for visualizing the shadows
	if( r_showShadows.GetInteger() )
	{
		// set the debug shadow color
		renderProgManager.SetRenderParm( RENDERPARM_COLOR, colorMagenta.ToFloatPtr() );
		if( r_showShadows.GetInteger() == 2 )
		{
			// draw filled in
			glState = GLS_DEPTHMASK | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_LESS;
		}
		else {
			// draw as lines, filling the depth buffer
			glState = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_POLYMODE_LINE | GLS_DEPTHFUNC_ALWAYS;
		}
	}
	else {
		// don't write to the color or depth buffer, just the stencil buffer
		glState = GLS_DEPTHMASK | GLS_COLORMASK | GLS_ALPHAMASK | GLS_DEPTHFUNC_LESS;
	}

	GL_PolygonOffset( r_shadowPolygonFactor.GetFloat(), -r_shadowPolygonOffset.GetFloat() );

	// the actual stencil func will be set in the draw code, but we need to make sure it isn't
	// disabled here, and that the value will get reset for the interactions without looking
	// like a no-change-required
	GL_State( glState | GLS_STENCIL_OP_FAIL_KEEP | GLS_STENCIL_OP_ZFAIL_KEEP | GLS_STENCIL_OP_PASS_INCR |
		GLS_STENCIL_MAKE_REF( STENCIL_SHADOW_TEST_VALUE ) | GLS_STENCIL_MAKE_MASK( STENCIL_SHADOW_MASK_VALUE ) | GLS_POLYGON_OFFSET );

	// Two Sided Stencil reduces two draw calls to one for slightly faster shadows
	GL_Cull( CT_TWO_SIDED );


	// process the chain of shadows with the current rendering state
	backEnd.currentSpace = NULL;

	for( const drawSurf_t* drawSurf = drawSurfs; drawSurf != NULL; drawSurf = drawSurf->nextOnLight )
	{
		if( drawSurf->scissorRect.IsEmpty() )
		{
			continue; // !@# FIXME: find out why this is sometimes being hit!
			// temporarily jump over the scissor and draw so the gl error callback doesn't get hit
		}

		// make sure the shadow volume is done
		if( drawSurf->shadowVolumeState != SHADOWVOLUME_DONE )
		{
			assert( drawSurf->shadowVolumeState == SHADOWVOLUME_UNFINISHED || drawSurf->shadowVolumeState == SHADOWVOLUME_DONE );

			uint64 start = Sys_Microseconds();
			while( drawSurf->shadowVolumeState == SHADOWVOLUME_UNFINISHED )
			{
				Sys_Yield();
			}
			uint64 end = Sys_Microseconds();

			backEnd.pc.shadowMicroSec += end - start;
		}

		if( drawSurf->numIndexes == 0 ) {
			continue;	// a job may have created an empty shadow volume
		}

		RB_SetScissor( drawSurf->scissorRect );

		if( drawSurf->space != backEnd.currentSpace )
		{
			// change the matrix
			RB_SetMVP( drawSurf->space->mvp );

			// set the local light position to allow the vertex program to project the shadow volume end cap to infinity
			idRenderVector localLight( 0.0f );
			drawSurf->space->modelMatrix.InverseTransformPoint( vLight->globalLightOrigin, localLight.ToVec3() );
			renderProgManager.SetRenderParm( RENDERPARM_LOCALLIGHTORIGIN, localLight.ToFloatPtr() );

			backEnd.currentSpace = drawSurf->space;
		}

		if( r_showShadows.GetInteger() == 0 )
		{
			renderProgManager.BindShader_Shadow( drawSurf->jointCache );
		}
		else {
			renderProgManager.BindShader_ShadowDebug( drawSurf->jointCache );
		}

		// set depth bounds per shadow
		if( r_useShadowDepthBounds.GetBool() )
		{
			GL_DepthBoundsTest( drawSurf->scissorRect.zmin, drawSurf->scissorRect.zmax );
		}

		// Determine whether or not the shadow volume needs to be rendered with Z-pass or
		// Z-fail. It is worthwhile to spend significant resources to reduce the number of
		// cases where shadow volumes need to be rendered with Z-fail because Z-fail
		// rendering can be significantly slower even on today's hardware. For instance,
		// on NVIDIA hardware Z-fail rendering causes the Z-Cull to be used in reverse:
		// Z-near becomes Z-far (trivial accept becomes trivial reject). Using the Z-Cull
		// in reverse is far less efficient because the Z-Cull only stores Z-near per 16x16
		// pixels while the Z-far is stored per 4x2 pixels. (The Z-near coallesce buffer
		// which has 4x4 granularity is only used when updating the depth which is not the
		// case for shadow volumes.) Note that it is also important to NOT use a Z-Cull
		// reconstruct because that would clear the Z-near of the Z-Cull which results in
		// no trivial rejection for Z-fail stencil shadow rendering.

		const bool renderZPass = ( drawSurf->renderZFail == 0 ) || r_forceZPassStencilShadows.GetBool();

		if( renderZPass )
		{
			glStencilOpSeparate( GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR );
			glStencilOpSeparate( GL_BACK, GL_KEEP, GL_KEEP, GL_DECR );
			///glStencilOpSeparateATI( backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK, GL_KEEP, GL_KEEP, tr.stencilIncr );
			///qglStencilOpSeparateATI( backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT, GL_KEEP, GL_KEEP, tr.stencilDecr );
		}
		else if( r_useStencilShadowPreload.GetBool() )
		{
			// preload + Z-pass
			glStencilOpSeparate( GL_FRONT, GL_KEEP, GL_DECR, GL_DECR );
			glStencilOpSeparate( GL_BACK, GL_KEEP, GL_INCR, GL_INCR );
		}
		else
		{	// Z-fail
			glStencilOpSeparate( GL_FRONT, GL_KEEP, GL_DECR, GL_KEEP );
			glStencilOpSeparate( GL_BACK, GL_KEEP, GL_INCR, GL_KEEP );
			///glStencilOpSeparate( backEnd.viewDef->isMirror ? GL_FRONT : GL_BACK, GL_KEEP, GL_DECR_WRAP, GL_KEEP );
			///glStencilOpSeparate( backEnd.viewDef->isMirror ? GL_BACK : GL_FRONT, GL_KEEP, GL_INCR_WRAP, GL_KEEP );
		}

		// get vertex buffer -------------------------------------------------------

		idVertexBuffer vertexBuffer;
		vertexCache.GetVertexBuffer( drawSurf->shadowCache, &vertexBuffer );
		const GLint vertOffset = vertexBuffer.GetOffset();
		const GLuint vbo = reinterpret_cast<GLuint>( vertexBuffer.GetAPIObject() );

		// get index buffer --------------------------------------------------------

		idIndexBuffer indexBuffer;
		vertexCache.GetIndexBuffer( drawSurf->indexCache, &indexBuffer );
		const GLintptr indexOffset = indexBuffer.GetOffset();
		const GLuint ibo = reinterpret_cast<GLuint>( indexBuffer.GetAPIObject() );

		// -------------------------------------------------------------------------

		const GLsizei indexCount = r_singleTriangle.GetBool() ? 3 : drawSurf->numIndexes;
		const vertexLayoutType_t vertexLayout = drawSurf->jointCache ? LAYOUT_DRAW_SHADOW_VERT_SKINNED : LAYOUT_DRAW_SHADOW_VERT;
		const GLsizei vertexSize = drawSurf->jointCache ? sizeof( idShadowVertSkinned ) : sizeof( idShadowVert );
		const GLint baseVertexOffset = vertOffset / vertexSize;

		if( drawSurf->jointCache )
		{
			assert( renderProgManager.ShaderUsesJoints() );

			idJointBuffer jointBuffer;
			if( !vertexCache.GetJointBuffer( drawSurf->jointCache, &jointBuffer ) )
			{
				idLib::Warning( "GL_DrawIndexed, jointBuffer == NULL" );
				continue;
			}
			assert( ( jointBuffer.GetOffset() & ( glConfig.uniformBufferOffsetAlignment - 1 ) ) == 0 );

			const GLintptr ubo = reinterpret_cast< GLintptr >( jointBuffer.GetAPIObject() );
			glBindBufferRange( GL_UNIFORM_BUFFER, BINDING_MATRICES_UBO, ubo, jointBuffer.GetOffset(), jointBuffer.GetNumJoints() * sizeof( idJointMat ) );
		}

		renderProgManager.CommitUniforms();

		// RB: 64 bit fixes, changed GLuint to GLintptr
		if( backEnd.glState.currentIndexBuffer != ( GLintptr )ibo || !r_useStateCaching.GetBool() )
		{
			glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo );
			backEnd.glState.currentIndexBuffer = ( GLintptr )ibo;
		}

		if( ( backEnd.glState.vertexLayout != vertexLayout ) || ( backEnd.glState.currentVertexBuffer != ( GLintptr )vbo ) || !r_useStateCaching.GetBool() )
		{
			backEnd.glState.currentVertexBuffer = ( GLintptr )vbo;

			if( glConfig.ARB_vertex_attrib_binding && r_useVertexAttribFormat.GetBool() ) 
			{
				const GLintptr baseOffset = 0;
				const GLuint bindingIndex = 0;
				glBindVertexBuffer( bindingIndex, vbo, baseOffset, vertexSize );
			}
			else {
				glBindBuffer( GL_ARRAY_BUFFER, vbo );
			}

			GL_SetVertexLayout( vertexLayout );
			backEnd.glState.vertexLayout = vertexLayout;
		}

		glDrawElementsBaseVertex( GL_TRIANGLES, indexCount, GL_INDEX_TYPE, ( triIndex_t* )indexOffset, baseVertexOffset );

		backEnd.pc.c_shadowElements++;
		backEnd.pc.c_shadowIndexes += drawSurf->numIndexes;

		RENDERLOG_PRINT( "GL_DrawIndexed( VBO %u:%i, IBO %u:%i )\n", vbo, vertOffset, ibo, indexOffset );

		if( !renderZPass && r_useStencilShadowPreload.GetBool() )
		{
			// render again with Z-pass
			glStencilOpSeparate( GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR );
			glStencilOpSeparate( GL_BACK, GL_KEEP, GL_KEEP, GL_DECR );

			glDrawElementsBaseVertex( GL_TRIANGLES, indexCount, GL_INDEX_TYPE, ( triIndex_t* )indexOffset, baseVertexOffset );

			backEnd.pc.c_shadowElements++;
			backEnd.pc.c_shadowIndexes += drawSurf->numIndexes;

			RENDERLOG_PRINT( "GL_DrawIndexed( VBO %u:%i, IBO %u:%i ) Z-pass\n", vbo, vertOffset, ibo, indexOffset );
		}
	}

	// cleanup the shadow specific rendering state

	GL_Cull( CT_FRONT_SIDED );

	// reset depth bounds
	if( r_useShadowDepthBounds.GetBool() )
	{
		if( r_useLightDepthBounds.GetBool() )
		{
			GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
		}
		else {
			GL_DepthBoundsTest( 0.0f, 0.0f );
		}
	}

	RENDERLOG_CLOSE_BLOCK();
}

/*
==================
 RB_StencilSelectLight

	Deform the zeroOneCubeModel to exactly cover the light volume. Render the deformed cube model to the stencil buffer in
	such a way that only fragments that are directly visible and contained within the volume will be written creating a
	mask to be used by the following stencil shadow and draw interaction passes.
==================
*/
void RB_StencilSelectLight( const viewLight_t* const vLight )
{
	RENDERLOG_OPEN_BLOCK( "Stencil Select" );

	// enable the light scissor
	RB_SetScissor( vLight->scissorRect );

	// clear stencil buffer to 0 (not drawable)
	uint64 glStateMinusStencil = GL_GetCurrentStateMinusStencil();
	GL_State( glStateMinusStencil | GLS_STENCIL_FUNC_ALWAYS | GLS_STENCIL_MAKE_REF( STENCIL_SHADOW_TEST_VALUE ) | GLS_STENCIL_MAKE_MASK( STENCIL_SHADOW_MASK_VALUE ) );	// make sure stencil mask passes for the clear
	GL_Clear( false, false, true, 0, 0.0f, 0.0f, 0.0f, 0.0f, false );	// clear to 0 for stencil select

	GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );

	GL_State( GLS_COLORMASK | GLS_ALPHAMASK | GLS_DEPTHMASK | GLS_DEPTHFUNC_LESS | GLS_STENCIL_FUNC_ALWAYS | GLS_STENCIL_MAKE_REF( STENCIL_SHADOW_TEST_VALUE ) | GLS_STENCIL_MAKE_MASK( STENCIL_SHADOW_MASK_VALUE ) );
	GL_Cull( CT_TWO_SIDED );

	renderProgManager.BindShader_Depth( false );

	// set the matrix for deforming the 'zeroOneCubeModel' into the frustum to exactly cover the light volume
	idRenderMatrix invProjectMVPMatrix;
	idRenderMatrix::Multiply( backEnd.viewDef->GetMVPMatrix(), vLight->inverseBaseLightProject, invProjectMVPMatrix );
	RB_SetMVP( invProjectMVPMatrix );

	// two-sided stencil test
	glStencilOpSeparate( GL_FRONT, GL_KEEP, GL_REPLACE, GL_ZERO );
	glStencilOpSeparate( GL_BACK, GL_KEEP, GL_ZERO, GL_REPLACE );

	GL_DrawIndexed( &backEnd.zeroOneCubeSurface );

	// reset stencil state

	GL_Cull( CT_FRONT_SIDED );

	renderProgManager.Unbind();

	// unset the depthbounds
	GL_DepthBoundsTest( 0.0f, 0.0f );

	RENDERLOG_CLOSE_BLOCK();
}
