/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013 Robert Beckebans

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#ifndef __GRAPHICSAPIWRAPPER_H__
#define __GRAPHICSAPIWRAPPER_H__

/*
================================================================================================

	Graphics API wrapper/helper functions

	This wraps platform specific graphics API functionality that is used at run-time. This
	functionality is wrapped to avoid excessive conditional compilation and/or code duplication
	throughout the run-time rendering code that is shared on all platforms.

	Most other graphics API functions are called for initialization purposes and are called
	directly from platform specific code implemented in files in the platform specific folders:

	renderer/OpenGL/
	renderer/DirectX/
	renderer/GCM/

================================================================================================
*/

class idImage;
class idDeclRenderParm;
class idDeclRenderProg;

static const int MAX_OCCLUSION_QUERIES = 4096;
// returned by GL_GetDeferredQueryResult() when the query is from too long ago and the result is no longer available
static const int OCCLUSION_QUERY_TOO_OLD				= -1;

/*
================================================================================================

	Platform Specific Context

	GL_SetWrapperContext( wrapperContext_t )

================================================================================================
*/

#define USE_CORE_PROFILE

struct wrapperContext_t
{
	bool			disableDepthBoundsTest;
	bool			useTimerQuery;
	bool			useUniformArrays;
	unsigned int	queries[1];
};


/*
================================================
wrapperConfig_t
	GL_SetWrapperConfig( wrapperConfig_t )
		r_defaultPolyOfsFactor
		r_defaultPolyOfsUnits
		r_singleTriangle
================================================
*/
struct wrapperConfig_t
{
	// rendering options and settings
	bool			waitForOcclusionQuery;
	bool			forceTwoSided;
	bool			disableStateCaching;
	bool			lazyBindPrograms;
	bool			lazyBindParms;
	bool			lazyBindTextures;
	bool			stripFragmentBranches;
	bool			skipDetailTris;
	bool			singleTriangle;
	bool			finishEveryDraw;
	// values for polygon offset
	float			defaultPolyOfsFactor;
	float			defaultPolyOfsUnits;
	// global texture filter settings
	int				textureMinFilter;
	int				textureMaxFilter;
	int				textureMipFilter;
	float			textureAnisotropy;
	float			textureLODBias;

	// ----------------------------

	bool syncAtEndFrame;
	bool useGLSync;
	bool useGLVAO;

	bool skipGpuSyncAtEndOfFrame;
	bool printSyncGpuStatsThreshold;
	bool useMultiQuerry;

};

/*
================================================
wrapperStats_t
	GL_GetCurrentStats( wrapperStats_t & )
================================================
*/
struct wrapperStats_t
{
	int				c_queriesIssued;
	int				c_queriesPassed;
	int				c_queriesWaitTime;
	int				c_queriesTooOld;
	int				c_programsBound;
	int				c_drawElements;
	int				c_drawIndices;
	int				c_drawVertices;
};

/*
================================================================================================

	API

================================================================================================
*/

extern idCVar	r_useVertexAttribFormat;

void			GL_SetWrapperContext( const wrapperContext_t& context );
void			GL_SetWrapperConfig( const wrapperConfig_t& config );

void			GL_SetTimeDelta( uint64 delta );	// delta from GPU to CPU microseconds
void			GL_StartFrame( int frame );			// inserts a timing mark for the start of the GPU frame
void			GL_EndFrame();						// inserts a timing mark for the end of the GPU frame
void			GL_WaitForEndFrame( uint64 gpuFrameNanoseconds );	// wait for the GPU to reach the last end frame marker
void			GL_GetLastFrameTime( uint64& startGPUTimeMicroSec, uint64& endGPUTimeMicroSec );	// GPU time between GL_StartFrame() and GL_EndFrame()
void			GL_StartDepthPass( const idScreenRect& rect );
void			GL_FinishDepthPass();
void			GL_GetDepthPassRect( idScreenRect& rect );

void			GL_SetDefaultState();
void			GL_State( uint64 stateVector, bool forceGlState = false );
uint64			GL_GetCurrentState();
uint64			GL_GetCurrentStateMinusStencil();
void			GL_Cull( int cullType );
void			GL_Scissor( int x /* left*/, int y /* bottom */, int w, int h, int scissorIndex = 0 );
void			GL_Viewport( int x /* left */, int y /* bottom */, int w, int h, int viewportIndex = 0 );
ID_INLINE void	GL_Scissor( const idScreenRect& rect )
{
	GL_Scissor( rect.x1, rect.y1, rect.x2 - rect.x1 + 1, rect.y2 - rect.y1 + 1 );
}
ID_INLINE void	GL_Viewport( const idScreenRect& rect )
{
	GL_Viewport( rect.x1, rect.y1, rect.x2 - rect.x1 + 1, rect.y2 - rect.y1 + 1 );
}
ID_INLINE void	GL_ViewportAndScissor( int x, int y, int w, int h )
{
	GL_Viewport( x, y, w, h );
	GL_Scissor( x, y, w, h );
}
ID_INLINE void	GL_ViewportAndScissor( const idScreenRect& rect )
{
	GL_Viewport( rect );
	GL_Scissor( rect );
}

// RB: HDR parm
void			GL_Clear( bool color, bool depth, bool stencil, byte stencilValue, float r, float g, float b, float a, bool clearHDR = true );
// RB end
void			GL_ClearDepth( const float value = 1.0f );
void			GL_ClearColor( const float r = 0.0f, const float g = 0.0f, const float b = 0.0f, const float a = 1.0f, const int iColorBuffer = 0 );

void			GL_PolygonOffset( float scale, float bias ); // scale bias fill //clamp
void			GL_DepthBoundsTest( const float zmin, const float zmax );
//void			GL_Color( float* color );
void			GL_Color( const idVec3& color );
void			GL_Color( const idVec4& color );
void			GL_Color( float r, float g, float b );
void			GL_Color( float r, float g, float b, float a );

void			GL_SelectTexture( int unit );
int				GL_GetCurrentTextureUnit();
// Makes this image active on the given texture unit.
// automatically enables or disables cube mapping
// May perform file loading if the image was not preloaded.
void			GL_BindTexture( int unit, idImage * );
void			GL_ResetTexturesState();

void			GL_ResetProgramState();

void			GL_DisableRasterizer();

void			GL_SetNativeRenderDestination();
void			GL_SetRenderDestination( const idRenderDestination * );
const idRenderDestination * GetCurrentRenderDestination();
void			GL_BlitRenderBuffer(
					const idRenderDestination * src, const idScreenRect & srcRect,
					const idRenderDestination * dst, const idScreenRect & dstRect,
					bool color, bool linearFiltering );
bool			GL_IsNativeFramebufferActive();
bool			GL_IsBound( const idRenderDestination * );

//SEA: depricated!
void			GL_CopyCurrentColorToTexture( idImage *, int x, int y, int newImageWidth, int newImageHeight );
void			GL_CopyCurrentDepthToTexture( idImage *, int x, int y, int newImageWidth, int newImageHeight );

void			GL_Flush();		// flush the GPU command buffer
void			GL_Finish();	// wait for the GPU to have executed all commands

void			GL_SetVertexLayout( vertexLayoutType_t );
void			GL_DrawIndexed( const drawSurf_t*, vertexLayoutType_t = LAYOUT_DRAW_VERT_FULL, int globalInstanceCount = 1 );
void			GL_DrawZeroOneCube( vertexLayoutType_t = LAYOUT_DRAW_VERT_POSITION_ONLY, int instanceCount = 1 );
void			GL_DrawUnitSquare( vertexLayoutType_t = LAYOUT_DRAW_VERT_POSITION_TEXCOORD, int instanceCount = 1 );

void			GL_StartMDBatch();
void			GL_SetupMDParm( const drawSurf_t * const surface );
void			GL_FinishMDBatch();

// RB begin
bool			GL_CheckErrors_( const char* filename, int line );
#if 1 // !defined(RETAIL)
#define         GL_CheckErrors()	GL_CheckErrors_(__FILE__, __LINE__)
#else
#define         GL_CheckErrors()	false
#endif
// RB end

wrapperStats_t	GL_GetCurrentStats();
void			GL_ClearStats();


#if 1
//SEA it does't suppous to be fast its for compatibility

enum primitiveType_e
{
	PRIM_TYPE_POINTS,//dx

	PRIM_TYPE_TRIANGLES,//dx
	PRIM_TYPE_TRIANGLE_STRIP,//dx
	///PRIM_TYPE_TRIANGLE_FAN,
	PRIM_TYPE_TRIANGLES_ADJACENCY,//dx
	PRIM_TYPE_TRIANGLE_STRIP_ADJACENCY,//dx

	PRIM_TYPE_LINES,//dx
	PRIM_TYPE_LINE_STRIP,//dx
	///PRIM_TYPE_LINE_LOOP,
	PRIM_TYPE_LINES_ADJACENCY,//dx
	PRIM_TYPE_LINE_STRIP_ADJACENCY,//dx

	PRIM_TYPE_MAX
};

class idImmediateRender {
public:
	virtual ~idImmediateRender() = 0;

	virtual void Begin( primitiveType_e primType ) = 0;
	virtual void End() = 0;

	virtual void Vertex3f( float x, float y, float z ) = 0;
	virtual void Color4f( float r, float g, float b, float a ) = 0;

	ID_INLINE void Vertex2fv( const float v[2] ) {
		Vertex3f( v[ 0 ], v[ 1 ], 0.0f );
	}
	ID_INLINE void Vertex3fv( const float v[3] ) {
		Vertex3f( v[ 0 ], v[ 1 ], v[ 2 ] );
	}

	//ID_INLINE void Color4ubv( const float v[ 3 ] )
	//{
	//	Vertex3f( v[ 0 ], v[ 1 ], v[ 2 ] );
	//}
	ID_INLINE void glColor3f( float r, float g, float b )
	{
		Color4f( r, g, b, 1.0f );
	}
	ID_INLINE void glColor4fv( const float v[ 4 ] )
	{
		Color4f( v[ 0 ], v[ 1 ], v[ 2 ], 1.0f );
	}
};

#endif


#endif // !__GRAPHICSAPIWRAPPER_H__
