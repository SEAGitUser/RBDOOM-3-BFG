/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

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

#ifndef __RENDERCONTEXT_H__
#define __RENDERCONTEXT_H__

// This is for "official" HDMI 3D support with with the left eye above the right and a guard band in the middle
// Some displays which don't support this can still do stereo 3D by packing 2 eyes into a single (mono-sized) buffer
enum hdmi3DState_t
{
	HDMI3D_NOT_SUPPORTED,	// The TV doesn't support it
	HDMI3D_NOT_ENABLED,		// The TV supports it, but the user disabled it
	HDMI3D_NOT_ACTIVE,		// The TV supports it, and the user enabled it, but it's not active
	HDMI3D_ACTIVE
};


//================================================================================================
// class idRenderContext
//================================================================================================
class idRenderContext {
public:
	idRenderContext() : depthHackValue( 0.0f ), weaponDepthHackValue( 1.0f ) {}
	virtual			~idRenderContext() {};

	void			InitContext();
	void			Shutdown();
	void			PrepareSwap();
	void			SwapBuffers( bool forceVsync = false );
	void			SetGamma( unsigned short red[256], unsigned short green[256], unsigned short blue[256] );

	hdmi3DState_t	GetHDMI3DState();
	void			EnableHDMI3D( const bool enable );

	// Back End Rendering
	void			ExecuteBackendCommands( const emptyCommand_t* cmds );
	void			Clear( float r, float g, float b, float a );
	void			InitGraphicsAPIWrapper();

	// Debug Tools
	void			RenderDebugTools( drawSurf_t** drawSurfs, int numDrawSurfs );

	void			SetWrapperContext( const wrapperContext_t& context );
	void			SetWrapperConfig( const wrapperConfig_t& config );

	// Texture Resolves
	void			ResolveTargetColor( idImage* image );
	void			ResolveTargetDepth( idImage* image );
	void			ResolveTargetColor( idImage* image, int srcMinX, int srcMinY, int srcMaxX, int srcMaxY, int dstX, int dstY );
	void			ResolveTargetDepth( idImage* image, int srcMinX, int srcMinY, int srcMaxX, int srcMaxY, int dstX, int dstY );

	void			SetDepthHackValue( float depth ) { depthHackValue = depth; }
	float			GetDepthHackValue() const { return depthHackValue; }
	void			SetWeaponDepthHackValue( float depth ) { weaponDepthHackValue = depth; }
	float			GetWeaponDepthHackValue() const { return weaponDepthHackValue; }

	uint64			GetGPUFrameMicroSec() const { return GPUFrameMicroSec; }

	/*
	============================================================
		RUNTIME CONTEXT
		Graphics Application Programming Interface (API)
	============================================================
	*/

	ID_INLINE void		BeginFrame() { GL_StartFrame( 0 ); }
	ID_INLINE void		EndFrame() { GL_EndFrame(); }

	ID_INLINE void		BeginRenderView( const idRenderView * rv ) { GL_BeginRenderView( rv ); }
	ID_INLINE void		EndRenderView() { GL_EndRenderView(); }

	ID_INLINE void		SetDefaultState() { GL_SetDefaultState(); }
	ID_INLINE void		SetState( uint64 stateVector, bool forceGlState = false ) { GL_State( stateVector, forceGlState ); }
	ID_INLINE uint64	GetCurrentState() const { return GL_GetCurrentState(); }
	ID_INLINE uint64	GetCurrentStateMinusStencil() const { return GL_GetCurrentStateMinusStencil(); }
	ID_INLINE void		SetPolygonOffset( float scale, float bias, float clamp = 0.0f ) { GL_PolygonOffset( scale, bias ); }
	ID_INLINE void		SetDepthBoundsTest( const float zmin, const float zmax ) { GL_DepthBoundsTest( zmin, zmax ); }
	ID_INLINE void		SetScissor( int x /* left*/, int y /* bottom */, int w, int h, int scissorIndex = 0 ) { GL_Scissor( x, y, w, h ); }
	ID_INLINE void		SetViewport( int x /* left */, int y /* bottom */, int w, int h, int viewportIndex = 0 ) { GL_Viewport( x, y, w, h ); }
	ID_INLINE void		SetScissor( const idScreenRect& rect ) { GL_Scissor( rect.x1, rect.y1, rect.x2 - rect.x1 + 1, rect.y2 - rect.y1 + 1 ); }
	ID_INLINE void		SetViewport( const idScreenRect& rect ) { GL_Viewport( rect.x1, rect.y1, rect.x2 - rect.x1 + 1, rect.y2 - rect.y1 + 1 ); }
	ID_INLINE void		SetViewportAndScissor( int x, int y, int w, int h ) { GL_Viewport( x, y, w, h ); GL_Scissor( x, y, w, h ); }
	ID_INLINE void		SetViewportAndScissor( const idScreenRect& rect ) { GL_Viewport( rect ); GL_Scissor( rect ); }

	ID_INLINE void		Clear( bool color, bool depth, bool stencil, byte stencilValue, float r, float g, float b, float a ) { GL_Clear( color, depth, stencil, stencilValue, r, g, b, a ); }
	ID_INLINE void		ClearDepth( const float value = 1.0f ) { GL_ClearDepth( value ); }
	ID_INLINE void		ClearDepthStencil( const float depthValue, const int stencilValue ) { GL_ClearDepthStencil( depthValue, stencilValue ); }
	ID_INLINE void		ClearColor( const float r = 0.0f, const float g = 0.0f, const float b = 0.0f, const float a = 1.0f, const int iColorBuffer = 0 ) { GL_ClearColor( r, g, b, a, iColorBuffer ); }

	ID_INLINE void		SetRenderDestination( const idRenderDestination * dest  ) { GL_SetRenderDestination( dest ); }
	ID_INLINE const idRenderDestination * GetCurrentRenderDestination() const { return GL_GetCurrentRenderDestination(); }

	ID_INLINE void		SetRenderProgram( const idDeclRenderProg * prog ) { GL_SetRenderProgram( prog ); }

	// Flush the GPU command buffer.
	ID_INLINE void		GPUFlush() { GL_Flush(); }
	// Wait for the GPU to have executed all commands.
	ID_INLINE void		GPUFinish() { GL_Finish(); }

private:

	float			depthHackValue;
	float			weaponDepthHackValue;
	uint64			GPUFrameMicroSec;
};

extern idRenderContext rAPIContext;

#endif	// !__RENDERCONTEXT_H__
