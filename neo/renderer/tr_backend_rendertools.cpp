﻿/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

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

	ALIGN16( float scale[16] ) = { 0 };
	scale[0] = w; // scale
	scale[5] = h; // scale
	scale[12] = ( halfScreenWidth * w * 2.1f * i ); // translate
	scale[13] = halfScreenHeight + ( halfScreenHeight * h ); // translate
	scale[10] = 1.0f;
	scale[15] = 1.0f;

	ALIGN16( float ortho[16] ) = { 0 };
	ortho[0] = 2.0f / screenWidth;
	ortho[5] = -2.0f / screenHeight;
	ortho[10] = -2.0f;
	ortho[12] = -1.0f;
	ortho[13] = 1.0f;
	ortho[14] = -1.0f;
	ortho[15] = 1.0f;

	ALIGN16( float finalOrtho[16] );
	R_MatrixMultiply( scale, ortho, finalOrtho );

	idRenderMatrix projMatrixTranspose;
	idRenderMatrix::Transpose( *( idRenderMatrix* )finalOrtho, projMatrixTranspose );
	renderProgManager.SetRenderParms( RENDERPARM_MVPMATRIX_X, projMatrixTranspose.Ptr(), 4 );
*/

#pragma hdrstop
#include "precompiled.h"

#include "OpenGL\gl_header.h"

#include "tr_local.h"
#include "simplex.h"	// line font definition

idCVar r_showCenterOfProjection( "r_showCenterOfProjection", "0", CVAR_RENDERER | CVAR_BOOL, "Draw a cross to show the center of projection" );
idCVar r_showLines( "r_showLines", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = draw alternate horizontal lines, 2 = draw alternate vertical lines" );

extern void RB_ResetViewportAndScissorToDefaultCamera( const idRenderView * const );

#define MAX_DEBUG_LINES			16384

struct debugLine_t
{
	idVec3		start;
	idVec3		end;
	uint32		color;
	int			lifeTime;
	bool		depthTest;
};

debugLine_t		rb_debugLines[ MAX_DEBUG_LINES ];
int				rb_numDebugLines = 0;
int				rb_debugLineTime = 0;

#define MAX_DEBUG_TEXT			512

struct debugText_t
{
	idStr		text;
	idVec3		origin;
	float		scale;
	uint32		color;
	idMat3		viewAxis;
	int			align;
	int			lifeTime;
	bool		depthTest;
};

debugText_t		rb_debugText[ MAX_DEBUG_TEXT ];
int				rb_numDebugText = 0;
int				rb_debugTextTime = 0;

#define MAX_DEBUG_POLYGONS		8192

struct debugPolygon_t
{
	idWinding	winding;
	uint32		color;
	int			lifeTime;
	bool		depthTest;
};

debugPolygon_t	rb_debugPolygons[ MAX_DEBUG_POLYGONS ];
int				rb_numDebugPolygons = 0;
int				rb_debugPolygonTime = 0;

static void RB_DrawText( const char* text, const idVec3& origin, float scale, const idVec4& color, const idMat3& viewAxis, const int align );

/*
================
RB_DrawBounds
================
*/
static void RB_DrawBounds( const idBounds& bounds, const idRenderMatrix & modelRenderMatrix ) // local bounds
{
	if( bounds.IsCleared() )
		return;
#if 1
	// calculate the matrix that transforms the unit cube to exactly cover the model in world space
	idRenderMatrix inverseBaseModelProject;
	idRenderMatrix::OffsetScaleForBounds( modelRenderMatrix, bounds, inverseBaseModelProject );

	// set the matrix for deforming the 'zeroOneCubeModel' into the frustum to exactly cover the light volume
	idRenderMatrix::Multiply( backEnd.viewDef->GetMVPMatrix(), inverseBaseModelProject,
							  renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_X )->GetVec4(),
							  renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_Y )->GetVec4(),
							  renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_Z )->GetVec4(),
							  renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_W )->GetVec4() );
	GL_DrawZeroOneCube();

#else
	glBegin( GL_LINE_LOOP );
		glVertex3f( bounds[0][0], bounds[0][1], bounds[0][2] );
		glVertex3f( bounds[0][0], bounds[1][1], bounds[0][2] );
		glVertex3f( bounds[1][0], bounds[1][1], bounds[0][2] );
		glVertex3f( bounds[1][0], bounds[0][1], bounds[0][2] );
	glEnd();
	glBegin( GL_LINE_LOOP );
		glVertex3f( bounds[0][0], bounds[0][1], bounds[1][2] );
		glVertex3f( bounds[0][0], bounds[1][1], bounds[1][2] );
		glVertex3f( bounds[1][0], bounds[1][1], bounds[1][2] );
		glVertex3f( bounds[1][0], bounds[0][1], bounds[1][2] );
	glEnd();

	glBegin( GL_LINES );
		glVertex3f( bounds[0][0], bounds[0][1], bounds[0][2] );
		glVertex3f( bounds[0][0], bounds[0][1], bounds[1][2] );

		glVertex3f( bounds[0][0], bounds[1][1], bounds[0][2] );
		glVertex3f( bounds[0][0], bounds[1][1], bounds[1][2] );

		glVertex3f( bounds[1][0], bounds[0][1], bounds[0][2] );
		glVertex3f( bounds[1][0], bounds[0][1], bounds[1][2] );

		glVertex3f( bounds[1][0], bounds[1][1], bounds[0][2] );
		glVertex3f( bounds[1][0], bounds[1][1], bounds[1][2] );
	glEnd();
#endif
}


/*
================
RB_SimpleSurfaceSetup
================
*/
static void RB_SimpleSurfaceSetup( const drawSurf_t* drawSurf )
{
	// change the matrix if needed
	if( drawSurf->space != backEnd.currentSpace )
	{
		backEnd.currentSpace = drawSurf->space;
		renderProgManager.SetSurfaceSpaceMatrices( drawSurf );
	}

	// change the scissor if needed
	if( !backEnd.currentScissor.Equals( drawSurf->scissorRect ) )
	{
		backEnd.currentScissor = drawSurf->scissorRect;
		GL_Scissor( backEnd.viewDef->GetViewport().x1 + drawSurf->scissorRect.x1,
					backEnd.viewDef->GetViewport().y1 + drawSurf->scissorRect.y1,
					drawSurf->scissorRect.x2 + 1 - drawSurf->scissorRect.x1,
					drawSurf->scissorRect.y2 + 1 - drawSurf->scissorRect.y1 );
	}
}

/*
================
RB_SimpleWorldSetup
================
*/
static void RB_SimpleWorldSetup()
{
	backEnd.currentSpace = &backEnd.viewDef->GetWorldSpace();

	//renderProgManager.SetRenderParms( RENDERPARM_PROJMATRIX_X, backEnd.viewDef->GetProjectionMatrix().Ptr(), 4 );

	//qglLoadMatrixf( backEnd.viewDef->worldSpace.modelViewMatrix );
	renderProgManager.SetMVPMatrixParms( backEnd.viewDef->GetMVPMatrix() );

	GL_Scissor( backEnd.viewDef->GetViewport().x1 + backEnd.viewDef->GetScissor().x1,
				backEnd.viewDef->GetViewport().y1 + backEnd.viewDef->GetScissor().y1,
				backEnd.viewDef->GetScissor().GetWidth(),
				backEnd.viewDef->GetScissor().GetHeight() );

	backEnd.currentScissor = backEnd.viewDef->GetScissor();
}

/*
=================
RB_PolygonClear

	This will cover the entire screen with normal rasterization.
	Texturing is disabled, but the existing glColor, glDepthMask,
	glColorMask, and the enabled state of depth buffering and
	stenciling will matter.
=================
*/
void RB_PolygonClear()
{
	glPushMatrix();
	glPushAttrib( GL_ALL_ATTRIB_BITS );
	glLoadIdentity();
	glDisable( GL_TEXTURE_2D );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );
	glDisable( GL_SCISSOR_TEST );

	glBegin( GL_POLYGON );
		glVertex3f( -20, -20, -10 );
		glVertex3f( 20, -20, -10 );
		glVertex3f( 20, 20, -10 );
		glVertex3f( -20, 20, -10 );
	glEnd();

	glPopAttrib();
	glPopMatrix();
}

/*
====================
RB_ShowDestinationAlpha
====================
*/
void RB_ShowDestinationAlpha()
{
	GL_State( GLS_SRCBLEND_DST_ALPHA | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
	renderProgManager.SetColorParm( 1, 1, 1 );
	RB_PolygonClear();
}

/*
===================
RB_ScanStencilBuffer

	Debugging tool to see what values are in the stencil buffer
===================
*/
void RB_ScanStencilBuffer()
{
	int	counts[ 256 ] = { 0 };
	{
		idTempArray<byte> stencilReadback( renderSystem->GetWidth() * renderSystem->GetHeight() );

		glReadPixels( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight(), GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback.Ptr() );

		for( int i = 0; i < renderSystem->GetWidth() * renderSystem->GetHeight(); ++i )
		{
			counts[ stencilReadback[ i ] ]++;
		}
	}
	// print some stats (not supposed to do from back end in SMP...)
	common->Printf( "stencil values:\n" );
	for( int i = 0; i < 255; i++ )
	{
		if( counts[i] )
		{
			common->Printf( "%i: %i\n", i, counts[i] );
		}
	}
}

/*
===================
RB_CountStencilBuffer

	Print an overdraw count based on stencil index values
===================
*/
static void RB_CountStencilBuffer()
{
	int count = 0;
	{
		idTempArray<byte> stencilReadback( renderSystem->GetWidth() * renderSystem->GetHeight() );

		glReadPixels( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight(), GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback.Ptr() );

		for( int i = 0; i < renderSystem->GetWidth() * renderSystem->GetHeight(); ++i )
		{
			count += stencilReadback[ i ];
		}
	}
	// print some stats (not supposed to do from back end in SMP...)
	common->Printf( "overdraw: %5.1f\n", ( float )count / ( renderSystem->GetWidth() * renderSystem->GetHeight() ) );
}

/*
===================
R_ColorByStencilBuffer

	Sets the screen colors based on the contents of the
	stencil buffer.  Stencil of 0 = black, 1 = red, 2 = green,
	3 = blue, ..., 7+ = white
===================
*/
static void R_ColorByStencilBuffer()
{
	static const idVec3 colors[8] = {
		idVec3( 0, 0, 0 ),
		idVec3( 1, 0, 0 ),
		idVec3( 0, 1, 0 ),
		idVec3( 0, 0, 1 ),
		idVec3( 0, 1, 1 ),
		idVec3( 1, 0, 1 ),
		idVec3( 1, 1, 0 ),
		idVec3( 1, 1, 1 ),
	};

	GL_SetRenderProgram( renderProgManager.prog_color );

	// clear color buffer to white (>6 passes)
	GL_Clear( true, false, false, 0, 1.0f, 1.0f, 1.0f, 1.0f );

	// now draw color for each stencil value
	glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
	for( int i = 0; i < 6; i++ )
	{
		renderProgManager.SetColorParm( colors[i] );
		glStencilFunc( GL_EQUAL, i, 255 );
		RB_PolygonClear();
	}

	glStencilFunc( GL_ALWAYS, 0, 255 );
}

//======================================================================

/*
==================
 RB_ShowOverdraw
==================
*/
void RB_ShowOverdraw()
{
	const drawSurf_t * surf;
	viewLight_t * vLight;
	int i;

	if( r_showOverDraw.GetInteger() == 0 )
	{
		return;
	}

	const idMaterial * material = declManager->FindMaterial( "textures/common/overdrawtest", false );
	if( material == NULL )
	{
		return;
	}

	drawSurf_t** drawSurfs = backEnd.viewDef->drawSurfs;
	int numDrawSurfs = backEnd.viewDef->numDrawSurfs;

	int interactions = 0;
	for( vLight = backEnd.viewDef->viewLights; vLight; vLight = vLight->next )
	{
		for( surf = vLight->localInteractions; surf; surf = surf->nextOnLight )
		{
			interactions++;
		}
		for( surf = vLight->globalInteractions; surf; surf = surf->nextOnLight )
		{
			interactions++;
		}
	}

	// FIXME: can't frame alloc from the renderer back-end
	auto newDrawSurfs = allocManager.FrameAlloc<drawSurf_t*, FRAME_ALLOC_DRAW_SURFACE_POINTER>( numDrawSurfs + interactions );

	for( i = 0; i < numDrawSurfs; i++ )
	{
		surf = drawSurfs[i];
		if( surf->material )
		{
			const_cast<drawSurf_t*>( surf )->material = material;
		}
		newDrawSurfs[i] = const_cast<drawSurf_t*>( surf );
	}

	for( vLight = backEnd.viewDef->viewLights; vLight; vLight = vLight->next )
	{
		for( surf = vLight->localInteractions; surf; surf = surf->nextOnLight )
		{
			const_cast<drawSurf_t*>( surf )->material = material;
			newDrawSurfs[i++] = const_cast<drawSurf_t*>( surf );
		}
		for( surf = vLight->globalInteractions; surf; surf = surf->nextOnLight )
		{
			const_cast<drawSurf_t*>( surf )->material = material;
			newDrawSurfs[i++] = const_cast<drawSurf_t*>( surf );
		}
		vLight->localInteractions = NULL;
		vLight->globalInteractions = NULL;
	}

	switch( r_showOverDraw.GetInteger() )
	{
		case 1: // geometry overdraw
			const_cast<idRenderView*>( backEnd.viewDef )->drawSurfs = newDrawSurfs;
			const_cast<idRenderView*>( backEnd.viewDef )->numDrawSurfs = numDrawSurfs;
			break;
		case 2: // light interaction overdraw
			const_cast<idRenderView*>( backEnd.viewDef )->drawSurfs = &newDrawSurfs[numDrawSurfs];
			const_cast<idRenderView*>( backEnd.viewDef )->numDrawSurfs = interactions;
			break;
		case 3: // geometry + light interaction overdraw
			const_cast<idRenderView*>( backEnd.viewDef )->drawSurfs = newDrawSurfs;
			const_cast<idRenderView*>( backEnd.viewDef )->numDrawSurfs += interactions;
			break;
	}
}

/*
===================
 RB_ShowIntensity

	Debugging tool to see how much dynamic range a scene is using.
	The greatest of the rgb values at each pixel will be used, with
	the resulting color shading from red at 0 to green at 128 to blue at 255
===================
*/
static void RB_ShowIntensity()
{
	if( !r_showIntensity.GetBool() )
	{
		return;
	}

	idTempArray<byte> colorReadback( renderSystem->GetWidth() * renderSystem->GetHeight() * 4 );

	glReadPixels( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight(), GL_RGBA, GL_UNSIGNED_BYTE, colorReadback.Ptr() );

	const int c = renderSystem->GetWidth() * renderSystem->GetHeight() * 4;
	for( int i = 0; i < c; i += 4 )
	{
		int j = colorReadback[i];
		if( colorReadback[i + 1] > j )
		{
			j = colorReadback[i + 1];
		}
		if( colorReadback[i + 2] > j )
		{
			j = colorReadback[i + 2];
		}
		if( j < 128 )
		{
			colorReadback[i + 0] = 2 * ( 128 - j );
			colorReadback[i + 1] = 2 * j;
			colorReadback[i + 2] = 0;
		}
		else
		{
			colorReadback[i + 0] = 0;
			colorReadback[i + 1] = 2 * ( 255 - j );
			colorReadback[i + 2] = 2 * ( j - 128 );
		}
	}

	// draw it back to the screen
	glLoadIdentity();
	glMatrixMode( GL_PROJECTION );
	GL_State( GLS_DEPTHFUNC_ALWAYS );
	glPushMatrix();
	glLoadIdentity();
	glOrtho( 0, 1, 0, 1, -1, 1 );
	glRasterPos2f( 0, 0 );
	glPopMatrix();
	renderProgManager.SetColorParm( 1, 1, 1 );
	GL_ResetTextureState();
	glMatrixMode( GL_MODELVIEW );

	glDrawPixels( renderSystem->GetWidth(), renderSystem->GetHeight(), GL_RGBA , GL_UNSIGNED_BYTE, colorReadback.Ptr() );
}


/*
===================
RB_ShowDepthBuffer

Draw the depth buffer as colors
===================
*/
static void RB_ShowDepthBuffer()
{
	if( !r_showDepth.GetBool() )
	{
		return;
	}

	glPushMatrix();
	glLoadIdentity();
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();
	glOrtho( 0, 1, 0, 1, -1, 1 );
	glRasterPos2f( 0, 0 );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();

	GL_State( GLS_DEPTHFUNC_ALWAYS );
	renderProgManager.SetColorParm( 1, 1, 1 );
	GL_ResetTextureState();

	idTempArray<byte> depthReadback( renderSystem->GetWidth() * renderSystem->GetHeight() * 4 );
	depthReadback.Zero();

	glReadPixels( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight(), GL_DEPTH_COMPONENT , GL_FLOAT, depthReadback.Ptr() );
#if 0
	for( i = 0; i < renderSystem->GetWidth() * renderSystem->GetHeight(); i++ )
	{
		( ( byte* ) depthReadback )[ i * 4 + 0 ] =
		( ( byte* ) depthReadback )[ i * 4 + 1 ] =
		( ( byte* ) depthReadback )[ i * 4 + 2 ] = 255 * ( ( float* ) depthReadback )[ i ];
		( ( byte* ) depthReadback )[ i * 4 + 3 ] = 1;
	}
#endif
	glDrawPixels( renderSystem->GetWidth(), renderSystem->GetHeight(), GL_RGBA , GL_UNSIGNED_BYTE, depthReadback.Ptr() );
}

/*
=================
RB_ShowLightCount

This is a debugging tool that will draw each surface with a color
based on how many lights are effecting it
=================
*/
static void RB_ShowLightCount()
{
	const drawSurf_t*	surf;
	const viewLight_t*	vLight;

	if( !r_showLightCount.GetBool() )
	{
		return;
	}

	RB_SimpleWorldSetup();

	GL_Clear( false, false, true, 0, 0.0f, 0.0f, 0.0f, 0.0f );

	// optionally count everything through walls
	if( r_showLightCount.GetInteger() >= 2 )
	{
		GL_State( GLS_DEPTHFUNC_EQUAL | GLS_STENCIL_OP_FAIL_KEEP | GLS_STENCIL_OP_ZFAIL_INCR | GLS_STENCIL_OP_PASS_INCR );
	}
	else {
		GL_State( GLS_DEPTHFUNC_EQUAL | GLS_STENCIL_OP_FAIL_KEEP | GLS_STENCIL_OP_ZFAIL_KEEP | GLS_STENCIL_OP_PASS_INCR );
	}

	renderProgManager.SetRenderParm( RENDERPARM_MAP, renderImageManager->defaultImage );

	for( vLight = backEnd.viewDef->viewLights; vLight; vLight = vLight->next )
	{
		for( int i = 0; i < 2; i++ )
		{
			for( surf = i ? vLight->localInteractions : vLight->globalInteractions; surf; surf = ( drawSurf_t* )surf->nextOnLight )
			{
				RB_SimpleSurfaceSetup( surf );
				GL_DrawIndexed( surf );
			}
		}
	}

	// display the results
	R_ColorByStencilBuffer();

	if( r_showLightCount.GetInteger() > 2 )
	{
		RB_CountStencilBuffer();
	}
}

#if 0
/*
===============
RB_SetWeaponDepthHack
===============
*/
static void RB_SetWeaponDepthHack()
{
}

/*
===============
RB_SetModelDepthHack
===============
*/
static void RB_SetModelDepthHack( float depth )
{
}

/*
===============
RB_EnterWeaponDepthHack
===============
*/
static void RB_EnterWeaponDepthHack()
{
	float	matrix[16];

	memcpy( matrix, backEnd.viewDef->projectionMatrix, sizeof( matrix ) );

	const float modelDepthHack = 0.25f;
	matrix[2] *= modelDepthHack;
	matrix[6] *= modelDepthHack;
	matrix[10] *= modelDepthHack;
	matrix[14] *= modelDepthHack;

	glMatrixMode( GL_PROJECTION );
	glLoadMatrixf( matrix );
	glMatrixMode( GL_MODELVIEW );
}

/*
===============
RB_EnterModelDepthHack
===============
*/
static void RB_EnterModelDepthHack( float depth )
{
	float matrix[16];

	memcpy( matrix, backEnd.viewDef->projectionMatrix, sizeof( matrix ) );

	matrix[14] -= depth;

	glMatrixMode( GL_PROJECTION );
	glLoadMatrixf( matrix );
	glMatrixMode( GL_MODELVIEW );
}

/*
===============
RB_LeaveDepthHack
===============
*/
static void RB_LeaveDepthHack()
{
	glMatrixMode( GL_PROJECTION );
	glLoadMatrixf( backEnd.viewDef->projectionMatrix );
	glMatrixMode( GL_MODELVIEW );
}

/*
=============
RB_LoadMatrixWithBypass

does a glLoadMatrixf after optionally applying the low-latency bypass matrix
=============
*/
static void RB_LoadMatrixWithBypass( const float m[16] )
{
	glLoadMatrixf( m );
}

#endif
/*
====================
RB_RenderDrawSurfListWithFunction

	The triangle functions can check backEnd.currentSpace != surf->space
	to see if they need to perform any new matrix setup.  The modelview
	matrix will already have been loaded, and backEnd.currentSpace will
	be updated after the triangle function completes.
====================
*/
static void RB_RenderDrawSurfListWithFunction( const drawSurf_t * const * drawSurfs, int numDrawSurfs, void ( *triFunc_ )( const drawSurf_t*, vertexLayoutType_t, int ) )
{
	backEnd.ClearCurrentSpace();

	for( int i = 0; i < numDrawSurfs; ++i )
	{
		auto const drawSurf = drawSurfs[ i ];
		if( drawSurf == NULL )
		{
			continue;
		}

		assert( drawSurf->space != NULL );

		// RB begin
	#if 1
		if( drawSurf->space != backEnd.currentSpace )
		{
			backEnd.currentSpace = drawSurf->space;

			renderProgManager.SetSurfaceSpaceMatrices( drawSurf );
		}
	#else

		if( drawSurf->space != NULL )  	// is it ever NULL?  Do we need to check?
		{
			// Set these values ahead of time so we don't have to reconstruct the matrices on the consoles
			if( drawSurf->space->weaponDepthHack )
			{
				RB_SetWeaponDepthHack();
			}

			if( drawSurf->space->modelDepthHack != 0.0f )
			{
				RB_SetModelDepthHack( drawSurf->space->modelDepthHack );
			}

			// change the matrix if needed
			if( drawSurf->space != backEnd.currentSpace )
			{
				RB_LoadMatrixWithBypass( drawSurf->space->modelViewMatrix );
			}

			if( drawSurf->space->weaponDepthHack )
			{
				RB_EnterWeaponDepthHack();
			}

			if( drawSurf->space->modelDepthHack != 0.0f )
			{
				RB_EnterModelDepthHack( drawSurf->space->modelDepthHack );
			}
		}
	#endif

		GL_SetRenderProgram( drawSurf->HasSkinning() ? renderProgManager.prog_color_skinned : renderProgManager.prog_color );

		// RB end

		// change the scissor if needed
		if( !backEnd.currentScissor.Equals( drawSurf->scissorRect ) )
		{
			backEnd.currentScissor = drawSurf->scissorRect;
			GL_Scissor( backEnd.viewDef->GetViewport().x1 + backEnd.currentScissor.x1,
						backEnd.viewDef->GetViewport().y1 + backEnd.currentScissor.y1,
						backEnd.currentScissor.GetWidth(),
						backEnd.currentScissor.GetHeight() );
		}

		// render it
		triFunc_( drawSurf, LAYOUT_DRAW_VERT_FULL, 1 );

		// RB begin
		/*if( drawSurf->space != NULL && ( drawSurf->space->weaponDepthHack || drawSurf->space->modelDepthHack != 0.0f ) )
		{
			RB_LeaveDepthHack();
		}*/
		// RB end

		backEnd.currentSpace = drawSurf->space;
	}
}

/*
=================
RB_ShowSilhouette

Blacks out all edges, then adds color for each edge that a shadow
plane extends from, allowing you to see doubled edges

FIXME: not thread safe!
=================
*/
static void RB_ShowSilhouette()
{
	int		i;
	const drawSurf_t*	surf;
	const viewLight_t*	vLight;

	if( !r_showSilhouette.GetBool() )
	{
		return;
	}

	// clear all triangle edges to black
	GL_ResetTextureState();

	// RB begin
	GL_SetRenderProgram( renderProgManager.prog_color );
	// RB end

	renderProgManager.SetColorParm( 0, 0, 0 );

	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_POLYMODE_LINE | GLS_TWOSIDED );

	RB_RenderDrawSurfListWithFunction( backEnd.viewDef->drawSurfs, backEnd.viewDef->numDrawSurfs, GL_DrawIndexed );

	// now blend in edges that cast silhouettes
	RB_SimpleWorldSetup();
	renderProgManager.SetColorParm( 0.5, 0, 0 );
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

	for( vLight = backEnd.viewDef->viewLights; vLight; vLight = vLight->next )
	{
		for( i = 0; i < 2; i++ )
		{
			for( surf = i ? vLight->localShadows : vLight->globalShadows; surf; surf = ( drawSurf_t* )surf->nextOnLight )
			{
				RB_SimpleSurfaceSetup( surf );

				auto const tri = surf->frontEndGeo;

				idVertexBuffer vertexBuffer;
				if( !vertexCache.GetVertexBuffer( tri->shadowCache, vertexBuffer ) )
				{
					continue;
				}

				// RB: 64 bit fixes, changed GLuint to GLintptr
				glBindBuffer( GL_ARRAY_BUFFER, ( GLintptr )vertexBuffer.GetAPIObject() );
				GLintptr vertOffset = vertexBuffer.GetOffset();
				// RB end

				glVertexPointer( 3, GL_FLOAT, sizeof( idShadowVert ), ( void* )vertOffset );
				glBegin( GL_LINES );

				for( int j = 0; j < tri->numIndexes; j += 3 )
				{
					const GLint i1 = tri->indexes[ j + 0 ];
					const GLint i2 = tri->indexes[ j + 1 ];
					const GLint i3 = tri->indexes[ j + 2 ];

					if( ( i1 & 1 ) + ( i2 & 1 ) + ( i3 & 1 ) == 1 )
					{
						if( ( i1 & 1 ) + ( i2 & 1 ) == 0 )
						{
							glArrayElement( i1 );
							glArrayElement( i2 );
						}
						else if( ( i1 & 1 ) + ( i3 & 1 ) == 0 )
						{
							glArrayElement( i1 );
							glArrayElement( i3 );
						}
					}
				}

				glEnd();
			}
		}
	}

	GL_State( GLS_DEFAULT | GLS_FRONTSIDED );

	renderProgManager.SetColorParm( 1.0, 1.0, 1.0 );
}

/*
=====================
RB_ShowTris

Debugging tool
=====================
*/
static void RB_ShowTris( const drawSurf_t * const * drawSurfs, int numDrawSurfs )
{
	modelTrace_t mt;
	idVec3 end;

	if( r_showTris.GetInteger() == 0 )
	{
		return;
	}

	backEnd.ClearCurrentSpace();

	idVec4 color( 1.0, 1.0, 1.0, 1.0 );

	GL_PolygonOffset( -1.0f, -2.0f );

	switch( r_showTris.GetInteger() )
	{
		case 1: // only draw visible ones
			GL_State( GLS_DEPTHMASK | GLS_ALPHAMASK | GLS_POLYMODE_LINE | GLS_POLYGON_OFFSET );
			break;
		case 2:	// draw all front facing
		case 3: // draw all
			GL_State( GLS_DEPTHMASK | GLS_ALPHAMASK | GLS_POLYMODE_LINE | GLS_POLYGON_OFFSET | GLS_DEPTHFUNC_ALWAYS );
			break;
		case 4: // only draw visible ones with blended lines
			GL_State( GLS_DEPTHMASK | GLS_ALPHAMASK | GLS_POLYMODE_LINE | GLS_POLYGON_OFFSET | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
			color[3] = 0.4f;
			break;
	}

	if( r_showTris.GetInteger() == 3 )
	{
		//GL_Cull( CT_TWO_SIDED );?
	}

	renderProgManager.SetColorParm( color );

	RB_RenderDrawSurfListWithFunction( drawSurfs, numDrawSurfs, GL_DrawIndexed );

	if( r_showTris.GetInteger() == 3 )
	{
		//GL_Cull( CT_FRONT_SIDED );?
	}
}

/*
=====================
RB_ShowSurfaceInfo

Debugging tool
=====================
*/


static idStr surfModelName, surfMatName;
static idVec3 surfPoint;
static bool surfTraced = false;

// Do tracing at a safe time to avoid threading issues.
void idRenderSystemLocal::OnFrame()
{
	surfTraced = false;

	if( !r_showSurfaceInfo.GetBool() )
	{
		return;
	}

	if( tr.primaryView == NULL )
	{
		return;
	}

	// start far enough away that we don't hit the player model
	idVec3 start = tr.primaryView->GetOrigin() + tr.primaryView->GetAxis()[0] * 32.0f;
	idVec3 end = start + tr.primaryView->GetAxis()[0] * 1000.0f;

	modelTrace_t mt;
	if( !tr.primaryWorld->Trace( mt, start, end, 0.0f, false ) )
	{
		return;
	}

	surfPoint = mt.point;
	surfModelName = mt.entity->hModel->Name();
	surfMatName = mt.material->GetName();
	surfTraced = true;
}


static void RB_ShowSurfaceInfo( const drawSurf_t * const * drawSurfs, int numDrawSurfs )
{
	if( !r_showSurfaceInfo.GetBool() || !surfTraced )
		return;

	backEnd.ClearCurrentSpace();
	GL_ResetTextureState();

	RB_SimpleWorldSetup();

	// foresthale 2014-05-02: don't use a shader for tools
	//GL_SetRenderProgram( renderProgManager.prog_texture_color );
	renderProgManager.SetRenderParm( RENDERPARM_MAP, renderImageManager->whiteImage );

	renderProgManager.SetVertexColorParm( SVC_MODULATE );
	// foresthale 2014-05-02: don't use a shader for tools
	//renderProgManager.CommitUniforms();

	renderProgManager.SetColorParm( 1, 1, 1 );

	static float scale = -1;
	static float bias = -2;

	GL_PolygonOffset( scale, bias );
	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_POLYMODE_LINE | GLS_POLYGON_OFFSET );

	// idVec3 trans[3];
	// float matrix[16];
	// transform the object verts into global space
	// R_AxisToModelMatrix( mt.entity->axis, mt.entity->origin, matrix );

	tr.primaryWorld->DrawText( surfModelName, surfPoint + tr.primaryView->GetAxis()[2] * 12.0f, 0.35f, idColor::red, tr.primaryView->GetAxis() );
	tr.primaryWorld->DrawText( surfMatName, surfPoint,  0.35f, idColor::blue, tr.primaryView->GetAxis() );
}

static void RB_ShowSurfaceInfo2( const drawSurf_t * const * drawSurfs, int numDrawSurfs )
{
	if( !r_showSurfaceInfo.GetBool() )
		return;

	modelTrace_t mt;

	// start far enough away that we don't hit the player model
	idVec3 start = tr.primaryView->GetOrigin() + tr.primaryView->GetAxis()[ 0 ] * 16.0f;
	idVec3 end = start + tr.primaryView->GetAxis()[ 0 ] * 1000.0f;
	if( tr.primaryWorld->Trace( mt, start, end, 0.0f, false ) )
	{
		GL_PolygonOffset( -1, -2 );
		GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_POLYMODE_LINE | GLS_POLYGON_OFFSET );

		tr.primaryWorld->DrawText( mt.entity->hModel->Name(), mt.point + tr.primaryView->GetAxis()[ 2 ] * 12.0f, 0.35f, idColor::red, tr.primaryView->GetAxis() );
		tr.primaryWorld->DrawText( mt.material->GetName(), mt.point, 0.35f, idColor::blue, tr.primaryView->GetAxis() );
	}
}

/*
=====================
RB_ShowViewEntitys

Debugging tool
=====================
*/
static void RB_ShowViewEntitys( viewModel_t* vModels )
{
	if( r_showViewEntitys.GetInteger() > 0 )
		return;

	if( r_showViewEntitys.GetInteger() >= 2 )
	{
		common->Printf( "view entities: " );
		for( const viewModel_t* vModel = vModels; vModel; vModel = vModel->next )
		{
			if( vModel->entityDef->IsDirectlyVisible() )
			{
				common->Printf( "<%i> ", vModel->entityDef->GetIndex() );
			}
			else {
				common->Printf( "%i ", vModel->entityDef->GetIndex() );
			}
		}
		common->Printf( "\n" );
	}

	backEnd.ClearCurrentSpace();
	GL_ResetTextureState();

	GL_SetRenderProgram( renderProgManager.prog_color );

	renderProgManager.SetColorParm( 1.0f, 1.0f, 1.0f );

	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_POLYMODE_LINE | GLS_TWOSIDED );

	for( const viewModel_t* vModel = vModels; vModel; vModel = vModel->next )
	{
		///glLoadMatrixf( vModel->modelViewMatrix );

		const idRenderEntityLocal* edef = vModel->entityDef;
		if( !edef )
		{
			continue;
		}

		///renderProgManager.SetMVPMatrixParms( vModel->modelViewMatrix );

		// draw the model bounds in white if directly visible,
		// or, blue if it is only-for-sahdow
		idColor	color;
		if( edef->IsDirectlyVisible() )
		{
			color.Set( 1, 1, 1, 1 );
		}
		else {
			color.Set( 0, 0, 1, 1 );
		}
		renderProgManager.SetColorParm( color.r, color.g, color.b );

		RB_DrawBounds( edef->localReferenceBounds, vModel->modelMatrix );

		// transform the upper bounds corner into global space
		if( r_showViewEntitys.GetInteger() >= 2 )
		{
			idVec3 corner;
			vModel->modelMatrix.TransformPoint( edef->localReferenceBounds.GetMaxs(), corner );

			tr.primaryWorld->DrawText(
				va( "%i:%s", edef->GetIndex(), edef->GetModel()->Name() ),
				corner,
				0.25f, color,
				tr.primaryView->GetAxis() );
		}

		// draw the actual bounds in yellow if different
		if( r_showViewEntitys.GetInteger() >= 3 )
		{
			renderProgManager.SetColorParm( 1, 1, 0 );
			// FIXME: cannot instantiate a dynamic model from the renderer back-end
			idRenderModel* model = vModel->entityDef->EmitDynamicModel();
			if( !model )
			{
				continue;	// particles won't instantiate without a current view
			}
			idBounds b = model->Bounds( &edef->GetParms() );
			if( b != edef->localReferenceBounds )
			{
				RB_DrawBounds( b, vModel->modelMatrix );
			}
		}
	}
}

/*
=====================
RB_ShowTexturePolarity

	Shade triangle red if they have a positive texture area
	green if they have a negative texture area, or blue if degenerate area.
=====================
*/
static void RB_ShowTexturePolarity( const drawSurf_t * const * drawSurfs, int numDrawSurfs )
{
	if( !r_showTexturePolarity.GetBool() )
	{
		return;
	}

	backEnd.ClearCurrentSpace();
	GL_ResetTextureState();

	GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	renderProgManager.SetColorParm( 1, 1, 1 );

	for( int i = 0; i < numDrawSurfs; ++i )
	{
		auto const drawSurf = drawSurfs[ i ];
		auto const tri = drawSurf->frontEndGeo;

		if( !tri || !tri->verts )
		{
			continue;
		}

		RB_SimpleSurfaceSetup( drawSurf );

		glBegin( GL_TRIANGLES );
		for( int j = 0; j < tri->numIndexes; j += 3 )
		{
			float d0[5], d1[5];

			auto & a = tri->GetIVertex( j + 0 );
			auto & b = tri->GetIVertex( j + 1 );
			auto & c = tri->GetIVertex( j + 2 );

			const idVec2 aST = a.GetTexCoord();
			const idVec2 bST = b.GetTexCoord();
			const idVec2 cST = c.GetTexCoord();

			d0[3] = bST[0] - aST[0];
			d0[4] = bST[1] - aST[1];

			d1[3] = cST[0] - aST[0];
			d1[4] = cST[1] - aST[1];

			float area = d0[3] * d1[4] - d0[4] * d1[3];

			if( idMath::Fabs( area ) < 0.0001f ) {
				renderProgManager.SetColorParm( 0, 0, 1, 0.5 );
			}
			else if( area < 0 ) {
				renderProgManager.SetColorParm( 1, 0, 0, 0.5 );
			}
			else {
				renderProgManager.SetColorParm( 0, 1, 0, 0.5 );
			}
			glVertex3fv( a.GetPosition().ToFloatPtr() );
			glVertex3fv( b.GetPosition().ToFloatPtr() );
			glVertex3fv( c.GetPosition().ToFloatPtr() );
		}
		glEnd();
	}

	GL_State( GLS_DEFAULT );
}

/*
=====================
RB_ShowUnsmoothedTangents

Shade materials that are using unsmoothed tangents
=====================
*/
static void RB_ShowUnsmoothedTangents( const drawSurf_t * const * drawSurfs, int numDrawSurfs )
{
	if( !r_showUnsmoothedTangents.GetBool() )
	{
		return;
	}

	backEnd.ClearCurrentSpace();
	GL_ResetTextureState();

	GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	renderProgManager.SetColorParm( 0, 1, 0, 0.5 );

	for( int i = 0; i < numDrawSurfs; ++i )
	{
		auto const drawSurf = drawSurfs[ i ];

		if( !drawSurf->material->UseUnsmoothedTangents() )
		{
			continue;
		}

		RB_SimpleSurfaceSetup( drawSurf );

		auto const tri = drawSurf->frontEndGeo;
		if( tri == NULL || tri->verts == NULL )
		{
			continue;
		}

		//glVertexPointer( 3, GL_FLOAT, sizeof( idDrawVert ), ( const void* ) tri->verts );
		glBegin( GL_TRIANGLES );
		for( int j = 0; j < tri->numIndexes; j += 3 )
		{
			//glArrayElement( j + 0 );
			//glArrayElement( j + 1 );
			//glArrayElement( j + 2 );

			glVertex3fv( tri->GetIVertex( j + 0 ).GetPosition().ToFloatPtr() );
			glVertex3fv( tri->GetIVertex( j + 1 ).GetPosition().ToFloatPtr() );
			glVertex3fv( tri->GetIVertex( j + 2 ).GetPosition().ToFloatPtr() );
		}
		glEnd();
	}

	GL_State( GLS_DEFAULT );
}

/*
=====================
RB_ShowTangentSpace

Shade a triangle by the RGB colors of its tangent space
1 = tangents[0]
2 = tangents[1]
3 = normal
=====================
*/
static void RB_ShowTangentSpace( const drawSurf_t * const * drawSurfs, int numDrawSurfs )
{
	if( !r_showTangentSpace.GetInteger() )
	{
		return;
	}

	backEnd.ClearCurrentSpace();
	GL_ResetTextureState();

	GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	for( int i = 0; i < numDrawSurfs; ++i )
	{
		auto const drawSurf = drawSurfs[ i ];

		RB_SimpleSurfaceSetup( drawSurf );

		auto const tri = drawSurf->frontEndGeo;
		if( tri == NULL || tri->verts == NULL )
		{
			continue;
		}

		glBegin( GL_TRIANGLES );
		for( int j = 0; j < tri->numIndexes; ++j )
		{
			auto & v = tri->GetIVertex( j );

			if( r_showTangentSpace.GetInteger() == 1 )
			{
				const idVec3 vertexTangent = v.GetTangent();
				renderProgManager.SetColorParm( 0.5 + 0.5 * vertexTangent[0], 0.5 + 0.5 * vertexTangent[1], 0.5 + 0.5 * vertexTangent[2], 0.5 );
			}
			else if( r_showTangentSpace.GetInteger() == 2 )
			{
				const idVec3 vertexBiTangent = v.GetBiTangent();
				renderProgManager.SetColorParm( 0.5 + 0.5 * vertexBiTangent[0], 0.5 + 0.5 * vertexBiTangent[1], 0.5 + 0.5 * vertexBiTangent[2], 0.5 );
			}
			else {
				const idVec3 vertexNormal = v.GetNormal();
				renderProgManager.SetColorParm( 0.5 + 0.5 * vertexNormal[0], 0.5 + 0.5 * vertexNormal[1], 0.5 + 0.5 * vertexNormal[2], 0.5 );
			}
			glVertex3fv( v.GetPosition().ToFloatPtr() );
		}
		glEnd();
	}

	GL_State( GLS_DEFAULT );
}

/*
=====================
RB_ShowVertexColor

Draw each triangle with the solid vertex colors
=====================
*/
static void RB_ShowVertexColor( const drawSurf_t * const * drawSurfs, int numDrawSurfs )
{
	if( !r_showVertexColor.GetBool() )
	{
		return;
	}

	backEnd.ClearCurrentSpace();
	GL_ResetTextureState();

	GL_SetRenderProgram( renderProgManager.prog_vertex_color );

	GL_State( GLS_DEPTHFUNC_LEQUAL );

	for( int i = 0; i < numDrawSurfs; ++i )
	{
		auto const drawSurf = drawSurfs[ i ];

		RB_SimpleSurfaceSetup( drawSurf );

		auto const tri = drawSurf->frontEndGeo;
		if( !tri || !tri->verts )
		{
			continue;
		}

		GL_CommitProgUniforms( GL_GetCurrentRenderProgram() );

		glBegin( GL_TRIANGLES );
		for( int j = 0; j < tri->numIndexes; ++j )
		{
			auto & v = tri->GetIVertex( j );
			glColor4ubv( v.color );
			glVertex3fv( v.GetPosition().ToFloatPtr() );
		}
		glEnd();
	}

	// RB end

	GL_State( GLS_DEFAULT );
}

/*
=====================
RB_ShowNormals

Debugging tool
=====================
*/
static void RB_ShowNormals( const drawSurf_t * const * drawSurfs, int numDrawSurfs )
{
	int			i, j;
	idVec3		end;
	float		size;
	bool		showNumbers;
	idVec3		pos;

	if( r_showNormals.GetFloat() == 0.0f )
	{
		return;
	}

	backEnd.ClearCurrentSpace();
	GL_ResetTextureState();

	if( !r_debugLineDepthTest.GetBool() )
	{
		GL_State( GLS_POLYMODE_LINE | GLS_DEPTHFUNC_ALWAYS );
	}
	else {
		GL_State( GLS_POLYMODE_LINE );
	}

	size = r_showNormals.GetFloat();
	if( size < 0.0f )
	{
		size = -size;
		showNumbers = true;
	}
	else {
		showNumbers = false;
	}

	for( i = 0; i < numDrawSurfs; ++i )
	{
		auto const drawSurf = drawSurfs[ i ];

		RB_SimpleSurfaceSetup( drawSurf );

		auto const tri = drawSurf->frontEndGeo;
		if( tri == NULL || tri->verts == NULL )
		{
			continue;
		}

		// RB begin
		GL_SetRenderProgram( renderProgManager.prog_vertex_color );

		glBegin( GL_LINES );
		for( j = 0; j < tri->numVerts; ++j )
		{
			const idVec3 position = tri->verts[ j ].GetPosition();
			const idVec3 normal = tri->verts[ j ].GetNormal();
			const idVec3 tangent = tri->verts[ j ].GetTangent();
			const idVec3 bitangent = tri->verts[ j ].GetBiTangent();

			glColor3f( 0, 0, 1 );
			glVertex3fv( position.ToFloatPtr() );
			VectorMA( position, size, normal, end );
			glVertex3fv( end.ToFloatPtr() );

			glColor3f( 1, 0, 0 );
			glVertex3fv( position.ToFloatPtr() );
			VectorMA( position, size, tangent, end );
			glVertex3fv( end.ToFloatPtr() );

			glColor3f( 0, 1, 0 );
			glVertex3fv( position.ToFloatPtr() );
			VectorMA( position, size, bitangent, end );
			glVertex3fv( end.ToFloatPtr() );
		}
		glEnd();

		// RB end
	}

	if( showNumbers )
	{
		RB_SimpleWorldSetup();
		for( i = 0; i < numDrawSurfs; ++i )
		{
			auto const drawSurf = drawSurfs[i];
			auto const tri = drawSurf->frontEndGeo;
			if( tri == NULL || tri->verts == NULL )
			{
				continue;
			}

			for( j = 0; j < tri->numVerts; ++j )
			{
				const idVec3 normal = tri->verts[j].GetNormal();
				const idVec3 tangent = tri->verts[j].GetTangent();
				drawSurf->space->modelMatrix.TransformPoint( tri->verts[ j ].GetPosition() + tangent + normal * 0.2f, pos );
				RB_DrawText( va( "%d", j ), pos, 0.01f, idColor::white.ToVec4(), backEnd.viewDef->GetAxis(), 1 );
			}

			for( j = 0; j < tri->numIndexes; j += 3 )
			{
				const idVec3 normal = tri->verts[ tri->indexes[ j + 0 ] ].GetNormal();
				drawSurf->space->modelMatrix.TransformPoint(
					( tri->GetIVertex( j + 0 ).GetPosition() +
					  tri->GetIVertex( j + 1 ).GetPosition() +
					  tri->GetIVertex( j + 2 ).GetPosition() )
					* ( 1.0f / 3.0f ) + normal * 0.2f, pos
				);
				RB_DrawText( va( "%d", j / 3 ), pos, 0.01f, idColor::cyan.ToVec4(), backEnd.viewDef->GetAxis(), 1 );
			}
		}
	}
}

#if 0 // compiler warning

/*
=====================
RB_ShowNormals

Debugging tool
=====================
*/
static void RB_AltShowNormals( drawSurf_t** drawSurfs, int numDrawSurfs )
{
	if( r_showNormals.GetFloat() == 0.0f )
	{
		return;
	}

	GL_ResetTextureState();

	GL_State( GLS_DEPTHFUNC_ALWAYS );

	for( int i = 0; i < numDrawSurfs; i++ )
	{
		drawSurf_t* drawSurf = drawSurfs[i];

		RB_SimpleSurfaceSetup( drawSurf );

		const idTriangles* tri = drawSurf->geo;

		glBegin( GL_LINES );
		for( int j = 0; j < tri->numIndexes; j += 3 )
		{
			const idDrawVert* v[3] =
			{
				&tri->verts[tri->indexes[j + 0]],
				&tri->verts[tri->indexes[j + 1]],
				&tri->verts[tri->indexes[j + 2]]
			}

			const idPlane plane( v[0]->xyz, v[1]->xyz, v[2]->xyz );

			// make the midpoint slightly above the triangle
			const idVec3 mid = ( v[0]->xyz + v[1]->xyz + v[2]->xyz ) * ( 1.0f / 3.0f ) + 0.1f * plane.Normal();

			for( int k = 0; k < 3; k++ )
			{
				const idVec3 pos = ( mid + v[k]->xyz * 3.0f ) * 0.25f;
				idVec3 end;

				renderProgManager.SetColorParm( 0, 0, 1 );
				glVertex3fv( pos.ToFloatPtr() );
				VectorMA( pos, r_showNormals.GetFloat(), v[k]->normal, end );
				glVertex3fv( end.ToFloatPtr() );

				renderProgManager.SetColorParm( 1, 0, 0 );
				glVertex3fv( pos.ToFloatPtr() );
				VectorMA( pos, r_showNormals.GetFloat(), v[k]->tangents[0], end );
				glVertex3fv( end.ToFloatPtr() );

				renderProgManager.SetColorParm( 0, 1, 0 );
				glVertex3fv( pos.ToFloatPtr() );
				VectorMA( pos, r_showNormals.GetFloat(), v[k]->tangents[1], end );
				glVertex3fv( end.ToFloatPtr() );

				renderProgManager.SetColorParm( 1, 1, 1 );
				glVertex3fv( pos.ToFloatPtr() );
				glVertex3fv( v[k]->xyz.ToFloatPtr() );
			}
		}
		glEnd();
	}
}

#endif

/*
=====================
RB_ShowTextureVectors

Draw texture vectors in the center of each triangle
=====================
*/
static void RB_ShowTextureVectors( const drawSurf_t * const * drawSurfs, int numDrawSurfs )
{
	if( r_showTextureVectors.GetFloat() == 0.0f )
	{
		return;
	}

	GL_State( GLS_DEPTHFUNC_LEQUAL );

	backEnd.ClearCurrentSpace();
	GL_ResetTextureState();

	for( int i = 0; i < numDrawSurfs; ++i )
	{
		auto const drawSurf = drawSurfs[i];
		auto const tri = drawSurf->frontEndGeo;

		if( tri == NULL || tri->verts == NULL )
		{
			continue;
		}

		RB_SimpleSurfaceSetup( drawSurf );

		// draw non-shared edges in yellow
		glBegin( GL_LINES );

		for( int j = 0; j < tri->numIndexes; j += 3 )
		{
			float d0[5], d1[5];
			idVec3 temp;
			idVec3 tangents[2];

			auto & a = tri->GetIVertex( j + 0 );
			auto & b = tri->GetIVertex( j + 1 );
			auto & c = tri->GetIVertex( j + 2 );

			const idVec3& a_pos = a.GetPosition();
			const idVec3& b_pos = b.GetPosition();
			const idVec3& c_pos = c.GetPosition();

			const idPlane plane( a_pos, b_pos, c_pos );

			// make the midpoint slightly above the triangle
			const idVec3 mid = ( a_pos + b_pos + c_pos ) * ( 1.0f / 3.0f ) + 0.1f * plane.Normal();

			// calculate the texture vectors
			const idVec2 aST = a.GetTexCoord();
			const idVec2 bST = b.GetTexCoord();
			const idVec2 cST = c.GetTexCoord();

			d0[0] = b_pos[0] - a_pos[0];
			d0[1] = b_pos[1] - a_pos[1];
			d0[2] = b_pos[2] - a_pos[2];
			d0[3] = bST[0] - aST[0];
			d0[4] = bST[1] - aST[1];

			d1[0] = c_pos[0] - a_pos[0];
			d1[1] = c_pos[1] - a_pos[1];
			d1[2] = c_pos[2] - a_pos[2];
			d1[3] = cST[0] - aST[0];
			d1[4] = cST[1] - aST[1];

			const float area = d0[3] * d1[4] - d0[4] * d1[3];
			if( area == 0 )
			{
				continue;
			}
			const float inva = 1.0f / area;

			temp[0] = ( d0[0] * d1[4] - d0[4] * d1[0] ) * inva;
			temp[1] = ( d0[1] * d1[4] - d0[4] * d1[1] ) * inva;
			temp[2] = ( d0[2] * d1[4] - d0[4] * d1[2] ) * inva;
			temp.Normalize();
			tangents[0] = temp;

			temp[0] = ( d0[3] * d1[0] - d0[0] * d1[3] ) * inva;
			temp[1] = ( d0[3] * d1[1] - d0[1] * d1[3] ) * inva;
			temp[2] = ( d0[3] * d1[2] - d0[2] * d1[3] ) * inva;
			temp.Normalize();
			tangents[1] = temp;

			// draw the tangents
			tangents[0] = mid + tangents[0] * r_showTextureVectors.GetFloat();
			tangents[1] = mid + tangents[1] * r_showTextureVectors.GetFloat();

			renderProgManager.SetColorParm( 1, 0, 0 );
			glVertex3fv( mid.ToFloatPtr() );
			glVertex3fv( tangents[0].ToFloatPtr() );

			///glVertexAttrib3fv( PC_ATTRIB_INDEX_POSITION, mid.ToFloatPtr() );
			///glVertexAttrib3fv( PC_ATTRIB_INDEX_TANGENT, tangents[ 0 ].ToFloatPtr() );

			renderProgManager.SetColorParm( 0, 1, 0 );
			glVertex3fv( mid.ToFloatPtr() );
			glVertex3fv( tangents[1].ToFloatPtr() );
		}

		glEnd();
	}
}

/*
=====================
RB_ShowDominantTris

Draw lines from each vertex to the dominant triangle center
=====================
*/
static void RB_ShowDominantTris( const drawSurf_t * const * drawSurfs, int numDrawSurfs )
{
	if( !r_showDominantTri.GetBool() )
	{
		return;
	}

	GL_State( GLS_DEPTHFUNC_LEQUAL );

	GL_PolygonOffset( -1, -2 );
	glEnable( GL_POLYGON_OFFSET_LINE );

	GL_ResetTextureState();

	for( int i = 0; i < numDrawSurfs; ++i )
	{
		auto const drawSurf = drawSurfs[i];
		auto const tri = drawSurf->frontEndGeo;

		if( tri == NULL || tri->verts == NULL )
		{
			continue;
		}
		if( !tri->dominantTris )
		{
			continue;
		}

		RB_SimpleSurfaceSetup( drawSurf );

		renderProgManager.SetColorParm( 1, 1, 0 );
		glBegin( GL_LINES );

		for( int j = 0; j < tri->numVerts; j++ )
		{
			// find the midpoint of the dominant tri

			auto & a = tri->GetVertex( j );
			auto & b = tri->GetVertex( tri->dominantTris[ j ].v2 );
			auto & c = tri->GetVertex( tri->dominantTris[ j ].v3 );

			idVec3 mid = ( a.GetPosition() + b.GetPosition() + c.GetPosition() ) * ( 1.0f / 3.0f );

			glVertex3fv( mid.ToFloatPtr() );
			glVertex3fv( a.GetPosition().ToFloatPtr() );

			//glVertexAttrib3fv( PC_ATTRIB_INDEX_POSITION, mid.ToFloatPtr() );
			//glVertexAttrib3fv( PC_ATTRIB_INDEX_POSITION, a.GetPosition().ToFloatPtr() );
		}

		glEnd();
	}
	glDisable( GL_POLYGON_OFFSET_LINE );
}

/*
=====================
RB_ShowEdges

Debugging tool
=====================
*/
static void RB_ShowEdges( const drawSurf_t * const * drawSurfs, int numDrawSurfs )
{
	int	i, j, k, m, n, o;

	const silEdge_t* edge;
	int	danglePlane;

	if( !r_showEdges.GetBool() )
	{
		return;
	}

	backEnd.ClearCurrentSpace();
	GL_ResetTextureState();

	GL_State( GLS_DEPTHFUNC_ALWAYS );

	for( i = 0; i < numDrawSurfs; ++i )
	{
		auto const drawSurf = drawSurfs[i];
		auto const tri = drawSurf->frontEndGeo;

		idDrawVert* ac = ( idDrawVert* )tri->verts;
		if( !ac )
		{
			continue;
		}

		RB_SimpleSurfaceSetup( drawSurf );

		// draw non-shared edges in yellow
		renderProgManager.SetColorParm( 1, 1, 0 );
		glBegin( GL_LINES );

		for( j = 0; j < tri->numIndexes; j += 3 )
		{
			for( k = 0; k < 3; k++ )
			{
				int l = ( k == 2 ) ? 0 : k + 1;
				int i1 = tri->indexes[ j + k ];
				int i2 = tri->indexes[ j + l ];

				// if these are used backwards, the edge is shared
				for( m = 0; m < tri->numIndexes; m += 3 )
				{
					for( n = 0; n < 3; n++ )
					{
						o = ( n == 2 ) ? 0 : n + 1;
						if( tri->indexes[m + n] == i2 && tri->indexes[m + o] == i1 )
						{
							break;
						}
					}
					if( n != 3 )
					{
						break;
					}
				}

				// if we didn't find a backwards listing, draw it in yellow
				if( m == tri->numIndexes )
				{
					glVertex3fv( ac[ i1 ].GetPosition().ToFloatPtr() );
					glVertex3fv( ac[ i2 ].GetPosition().ToFloatPtr() );
				}

			}
		}

		glEnd();

		// draw dangling sil edges in red
		if( !tri->silEdges )
		{
			continue;
		}

		// the plane number after all real planes
		// is the dangling edge
		danglePlane = tri->GetNumFaces();

		renderProgManager.SetColorParm( 1, 0, 0 );

		glBegin( GL_LINES );
		for( j = 0; j < tri->numSilEdges; ++j )
		{
			edge = tri->silEdges + j;

			if( edge->p1 != danglePlane && edge->p2 != danglePlane )
			{
				continue;
			}

			glVertex3fv( ac[ edge->v1 ].GetPosition().ToFloatPtr() );
			glVertex3fv( ac[ edge->v2 ].GetPosition().ToFloatPtr() );
		}
		glEnd();
	}
}

/*
==============
RB_ShowLights

Visualize all light volumes used in the current scene
r_showLights 1	: just print volumes numbers, highlighting ones covering the view
r_showLights 2	: also draw planes of each volume
r_showLights 3	: also draw edges of each volume
==============
*/
static void RB_ShowLights()
{
	if( !r_showLights.GetInteger() )
	{
		return;
	}

	GL_State( GLS_DEFAULT | GLS_TWOSIDED );
	//GL_Cull( CT_TWO_SIDED );

	GL_ResetTextureState();

	GL_SetRenderProgram( renderProgManager.prog_color );

	common->Printf( "volumes: " );	// FIXME: not in back end!

	int count = 0;
	for( viewLight_t* vLight = backEnd.viewDef->viewLights; vLight != NULL; vLight = vLight->next )
	{
		count++;

		// depth buffered planes
		if( r_showLights.GetInteger() >= 2 )
		{
			GL_State( GLS_DEPTHFUNC_LEQUAL | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHMASK );

			// RB: show different light types
			if( vLight->lightType == LIGHT_TYPE_PARALLEL )
			{
				renderProgManager.SetColorParm( 1.0f, 0.0f, 0.0f, 0.15f );
			}
			else if( vLight->lightType == LIGHT_TYPE_POINT )
			{
				renderProgManager.SetColorParm( 0.0f, 0.0f, 1.0f, 0.15f );
			}
			else {
				renderProgManager.SetColorParm( 0.0f, 1.0f, 0.0f, 0.15f );
			}
			// RB end

			//glEnable( GL_POLYGON_STIPPLE );
			//glPolygonStipple( stipple_pattern );
			// Specifies a pointer to a 32 × 32 stipple pattern that will be unpacked from memory in the same way that glDrawPixels unpacks pixels.

			idRenderMatrix invProjectMVPMatrix;
			idRenderMatrix::Multiply( backEnd.viewDef->GetMVPMatrix(), vLight->inverseBaseLightProject, invProjectMVPMatrix );
			renderProgManager.SetMVPMatrixParms( invProjectMVPMatrix );
			GL_DrawIndexed( &backEnd.zeroOneCubeSurface );
		}

		// non-hidden lines
		if( r_showLights.GetInteger() >= 3 )
		{
			idRenderMatrix invProjectMVPMatrix;
			idRenderMatrix::Multiply( backEnd.viewDef->GetMVPMatrix(), vLight->inverseBaseLightProject, invProjectMVPMatrix );
			renderProgManager.SetMVPMatrixParms( invProjectMVPMatrix );

			GL_State( GLS_DEPTHFUNC_LEQUAL | GLS_POLYMODE_LINE | GLS_DEPTHMASK );
			renderProgManager.SetColorParm( 1.0f, 1.0f, 1.0f, 0.3f );
			GL_DrawIndexed( &backEnd.zeroOneCubeSurface );

			//glPushAttrib( GL_ENABLE_BIT ); // to return everything to normal after drawing

			glLineStipple( 4, 0xAAAA );
			glEnable( GL_LINE_STIPPLE );

			GL_State( GLS_DEPTHFUNC_GREATER | GLS_POLYMODE_LINE | GLS_DEPTHMASK );
			renderProgManager.SetColorParm( idColor::mdGrey.ToVec4().x, idColor::mdGrey.ToVec4().y, idColor::mdGrey.ToVec4().z, 0.2f );
			GL_DrawIndexed( &backEnd.zeroOneCubeSurface );

			glDisable( GL_LINE_STIPPLE );
			//glPopAttrib();
		}

		common->Printf( "%i %s %f", vLight->lightDef->GetIndex(), vLight->lightShader->GetName(), vLight->lightDef->GetAxialSize() );
	}

	common->Printf( " = %i total\n", count );
}

// RB begin
static void RB_ShowShadowMapLODs()
{
	if( !r_showShadowMapLODs.GetInteger() )
	{
		return;
	}

	GL_State( GLS_DEFAULT | GLS_TWOSIDED );
	//GL_Cull( CT_TWO_SIDED );

	GL_ResetTextureState();

	GL_SetRenderProgram( renderProgManager.prog_color );

	common->Printf( "volumes: " );	// FIXME: not in back end!

	int count = 0;
	for( viewLight_t* vLight = backEnd.viewDef->viewLights; vLight != NULL; vLight = vLight->next )
	{
		if( !vLight->lightDef->LightCastsShadows() )
		{
			continue;
		}

		count++;

		// depth buffered planes
		if( r_showShadowMapLODs.GetInteger() >= 1 )
		{
			GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHMASK );

			idVec4 c;
			if( vLight->shadowLOD == 0 )
			{
				c = idColor::red.ToVec4();
			}
			else if( vLight->shadowLOD == 1 )
			{
				c = idColor::green.ToVec4();
			}
			else if( vLight->shadowLOD == 2 )
			{
				c = idColor::blue.ToVec4();
			}
			else if( vLight->shadowLOD == 3 )
			{
				c = idColor::yellow.ToVec4();
			}
			else if( vLight->shadowLOD == 4 )
			{
				c = idColor::magenta.ToVec4();
			}
			else if( vLight->shadowLOD == 5 )
			{
				c = idColor::cyan.ToVec4();
			}
			else {
				c = idColor::mdGrey.ToVec4();
			}

			c[3] = 0.25f;
			renderProgManager.SetColorParm( c );

			idRenderMatrix invProjectMVPMatrix;
			idRenderMatrix::Multiply( backEnd.viewDef->GetMVPMatrix(), vLight->inverseBaseLightProject, invProjectMVPMatrix );
			renderProgManager.SetMVPMatrixParms( invProjectMVPMatrix );
			GL_DrawIndexed( &backEnd.zeroOneCubeSurface );
		}

		// non-hidden lines
		if( r_showShadowMapLODs.GetInteger() >= 2 )
		{
			GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_POLYMODE_LINE | GLS_DEPTHMASK );
			renderProgManager.SetColorParm( 1.0f, 1.0f, 1.0f );
			idRenderMatrix invProjectMVPMatrix;
			idRenderMatrix::Multiply( backEnd.viewDef->GetMVPMatrix(), vLight->inverseBaseLightProject, invProjectMVPMatrix );
			renderProgManager.SetMVPMatrixParms( invProjectMVPMatrix );
			GL_DrawIndexed( &backEnd.zeroOneCubeSurface );
		}

		common->Printf( "%i ", vLight->lightDef->GetIndex() );
	}

	common->Printf( " = %i total\n", count );
}
// RB end

/*
=====================
RB_ShowPortals

Debugging tool, won't work correctly with SMP or when mirrors are present
=====================
*/
static void RB_ShowPortals()
{
	if( !r_showPortals.GetBool() )
	{
		return;
	}

	// all portals are expressed in world coordinates
	RB_SimpleWorldSetup();

	GL_ResetTextureState();

	GL_SetRenderProgram( renderProgManager.prog_color );
	GL_State( GLS_DEPTHFUNC_ALWAYS );

	GL_CommitProgUniforms( renderProgManager.prog_color );

	( ( idRenderWorldLocal* )backEnd.viewDef->renderWorld )->ShowPortals();
}

/*
================
RB_ClearDebugText
================
*/
void RB_ClearDebugText( int time )
{
	int			i;
	int			num;
	debugText_t*	text;

	rb_debugTextTime = time;

	if( !time )
	{
		// free up our strings
		text = rb_debugText;
		for( i = 0; i < MAX_DEBUG_TEXT; i++, text++ )
		{
			text->text.Clear();
		}
		rb_numDebugText = 0;
		return;
	}

	// copy any text that still needs to be drawn
	num	= 0;
	text = rb_debugText;
	for( i = 0; i < rb_numDebugText; i++, text++ )
	{
		if( text->lifeTime > time )
		{
			if( num != i )
			{
				rb_debugText[ num ] = *text;
			}
			num++;
		}
	}
	rb_numDebugText = num;
}

/*
================
RB_AddDebugText
================
*/
void RB_AddDebugText( const char* text, const idVec3& origin, float scale, const idVec4& color, const idMat3& viewAxis, const int align, const int lifetime, const bool depthTest )
{
	debugText_t* debugText;

	if( rb_numDebugText < MAX_DEBUG_TEXT )
	{
		debugText = &rb_debugText[ rb_numDebugText++ ];
		debugText->text			= text;
		debugText->origin		= origin;
		debugText->scale		= scale;
		debugText->viewAxis		= viewAxis;
		debugText->align		= align;
		debugText->color		= idColor::PackColor( color );
		debugText->lifeTime		= rb_debugTextTime + lifetime;
		debugText->depthTest	= depthTest;
	}
}

/*
================
RB_DrawTextLength

  returns the length of the given text
================
*/
float RB_DrawTextLength( const char* text, float scale, int len )
{
	int num, index, charIndex;
	float spacing, textLen = 0.0f;

	if( text && *text )
	{
		if( !len )
		{
			len = idStr::Length( text );
		}
		for( int i = 0; i < len; i++ )
		{
			charIndex = text[i] - 32;
			if( charIndex < 0 || charIndex > NUM_SIMPLEX_CHARS )
			{
				continue;
			}
			num = simplex[charIndex][0] * 2;
			spacing = simplex[charIndex][1];
			index = 2;

			while( index - 2 < num )
			{
				if( simplex[charIndex][index] < 0 )
				{
					++index;
					continue;
				}
				index += 2;
				if( simplex[charIndex][index] < 0 )
				{
					++index;
					continue;
				}
			}
			textLen += spacing * scale;
		}
	}
	return textLen;
}

/*
================
RB_DrawText

  oriented on the viewaxis
  align can be 0-left, 1-center (default), 2-right
================
*/
static void RB_DrawText( const char* text, const idVec3& origin, float scale, const idVec4& color, const idMat3& viewAxis, const int align )
{
	GL_SetRenderProgram( renderProgManager.prog_color );

	// RB begin
	renderProgManager.SetColorParm( color[0], color[1], color[2], 1 /*color[3]*/ );
	GL_CommitProgUniforms( GL_GetCurrentRenderProgram() );
	// RB end

	int j, num, index, charIndex, line;
	float textLen = 1.0f, spacing = 1.0f;
	idVec3 org, p1, p2;

	if( text && *text )
	{
		glBegin( GL_LINES );

		if( text[0] == '\n' )
		{
			line = 1;
		}
		else {
			line = 0;
		}

		int len = idStr::Length( text );
		for( int i = 0; i < len; i++ )
		{
			if( i == 0 || text[i] == '\n' )
			{
				org = origin - viewAxis[2] * ( line * 36.0f * scale );
				if( align != 0 )
				{
					for( j = 1; i + j <= len; j++ )
					{
						if( i + j == len || text[i + j] == '\n' )
						{
							textLen = RB_DrawTextLength( text + i, scale, j );
							break;
						}
					}
					if( align == 2 ) // right
					{
						org += viewAxis[1] * textLen;
					}
					else { // center
						org += viewAxis[1] * ( textLen * 0.5f );
					}
				}
				++line;
			}

			charIndex = text[i] - 32;
			if( charIndex < 0 || charIndex > NUM_SIMPLEX_CHARS )
				continue;

			num = simplex[charIndex][0] * 2;
			spacing = simplex[charIndex][1];
			index = 2;

			while( index - 2 < num )
			{
				if( simplex[charIndex][index] < 0 )
				{
					++index;
					continue;
				}
				p1 = org + scale * ( float )simplex[charIndex][index] * -viewAxis[1] + scale * ( float )simplex[charIndex][index + 1] * viewAxis[2];
				index += 2;
				if( simplex[charIndex][index] < 0 )
				{
					++index;
					continue;
				}
				p2 = org + scale * ( float )simplex[charIndex][index] * -viewAxis[1] + scale * ( float )simplex[charIndex][index + 1] * viewAxis[2];

				glVertex3fv( p1.ToFloatPtr() );
				glVertex3fv( p2.ToFloatPtr() );
			}
			org -= viewAxis[1] * ( spacing * scale );
		}

		glEnd();
	}
}

/*
================
RB_ShowDebugText
================
*/
void RB_ShowDebugText()
{
	if( !rb_numDebugText )
	{
		return;
	}

	RENDERLOG_SCOPED_BLOCK( "RB_ShowDebugText" );

	// all lines are expressed in world coordinates
	RB_SimpleWorldSetup();

	GL_ResetTextureState();

	int width = r_debugLineWidth.GetInteger();
	if( width < 1 )
	{
		width = 1;
	}
	else if( width > 10 )
	{
		width = 10;
	}

	// draw lines
	glLineWidth( width );

	if( !r_debugLineDepthTest.GetBool() )
	{
		GL_State( GLS_POLYMODE_LINE | GLS_DEPTHFUNC_ALWAYS );
	}
	else {
		GL_State( GLS_POLYMODE_LINE );
	}

	debugText_t* text = nullptr;

	text = rb_debugText;
	for( int i = 0; i < rb_numDebugText; i++, text++ )
	{
		if( !text->depthTest )
		{
			idVec4 color;
			idColor::UnpackColor( text->color, color );
			RB_DrawText( text->text, text->origin, text->scale, color, text->viewAxis, text->align );
		}
	}

	if( !r_debugLineDepthTest.GetBool() )
	{
		GL_State( GLS_POLYMODE_LINE );
	}

	text = rb_debugText;
	for( int i = 0; i < rb_numDebugText; i++, text++ )
	{
		if( text->depthTest )
		{
			idVec4 color;
			idColor::UnpackColor( text->color, color );
			RB_DrawText( text->text, text->origin, text->scale, color, text->viewAxis, text->align );
		}
	}

	glLineWidth( 1.0f );
}

/*
================
RB_ClearDebugLines
================
*/
void RB_ClearDebugLines( int time )
{
	rb_debugLineTime = time;

	if( !time )
	{
		rb_numDebugLines = 0;
		return;
	}

	// copy any lines that still need to be drawn
	int num	= 0;
	auto line = rb_debugLines;
	for( int i = 0; i < rb_numDebugLines; i++, line++ )
	{
		if( line->lifeTime > time )
		{
			if( num != i )
			{
				rb_debugLines[ num ] = *line;
			}
			num++;
		}
	}
	rb_numDebugLines = num;
}

/*
================
RB_AddDebugLine
================
*/
void RB_AddDebugLine( const idVec4& color, const idVec3& start, const idVec3& end, const int lifeTime, const bool depthTest )
{
	if( rb_numDebugLines < MAX_DEBUG_LINES )
	{
		auto line = &rb_debugLines[ rb_numDebugLines++ ];
		line->start		= start;
		line->end		= end;
		line->depthTest = depthTest;
		line->color		= idColor::PackColor( color );
		line->lifeTime	= rb_debugLineTime + lifeTime;
	}
}

/*
================
RB_ShowDebugLines
================
*/
static void RB_ShowDebugLines()
{
	if( !rb_numDebugLines )
	{
		return;
	}

	RENDERLOG_SCOPED_BLOCK( "RB_ShowDebugLines" );

	// all lines are expressed in world coordinates
	RB_SimpleWorldSetup();

	// RB begin
	GL_SetRenderProgram( renderProgManager.prog_vertex_color );
	GL_CommitProgUniforms( GL_GetCurrentRenderProgram() );
	// RB end

	GL_ResetTextureState();

	int width = r_debugLineWidth.GetInteger();
	if( width < 1 )
	{
		width = 1;
	}
	else if( width > 10 )
	{
		width = 10;
	}

	// draw lines
	glLineWidth( width );

	if( !r_debugLineDepthTest.GetBool() )
	{
		GL_State( GLS_POLYMODE_LINE | GLS_DEPTHFUNC_ALWAYS );
	}
	else
	{
		GL_State( GLS_POLYMODE_LINE );
	}

	glBegin( GL_LINES );

	debugLine_t* line = nullptr;

	line = rb_debugLines;
	for( int i = 0; i < rb_numDebugLines; i++, line++ )
	{
		if( !line->depthTest )
		{
			idVec4 color;
			idColor::UnpackColor( line->color, color );
			glColor4fv( color.ToFloatPtr() );
			glVertex3fv( line->start.ToFloatPtr() );
			glVertex3fv( line->end.ToFloatPtr() );
		}
	}
	glEnd();

	if( !r_debugLineDepthTest.GetBool() )
	{
		GL_State( GLS_POLYMODE_LINE );
	}

	glBegin( GL_LINES );

	line = rb_debugLines;
	for( int i = 0; i < rb_numDebugLines; i++, line++ )
	{
		if( line->depthTest )
		{
			idVec4 color;
			idColor::UnpackColor( line->color, color );
			glColor4fv( color.ToFloatPtr() );
			glVertex3fv( line->start.ToFloatPtr() );
			glVertex3fv( line->end.ToFloatPtr() );
		}
	}

	glEnd();

	glLineWidth( 1 );
	GL_State( GLS_DEFAULT );
}

/*
================
RB_ClearDebugPolygons
================
*/
void RB_ClearDebugPolygons( int time )
{
	rb_debugPolygonTime = time;

	if( !time )
	{
		rb_numDebugPolygons = 0;
		return;
	}

	// copy any polygons that still need to be drawn
	int num	= 0;

	auto poly = rb_debugPolygons;
	for( int i = 0; i < rb_numDebugPolygons; i++, poly++ )
	{
		if( poly->lifeTime > time )
		{
			if( num != i )
			{
				rb_debugPolygons[ num ] = *poly;
			}
			num++;
		}
	}
	rb_numDebugPolygons = num;
}

/*
================
RB_AddDebugPolygon
================
*/
void RB_AddDebugPolygon( const idVec4& color, const idWinding& winding, const int lifeTime, const bool depthTest )
{
	if( rb_numDebugPolygons < MAX_DEBUG_POLYGONS )
	{
		auto poly = &rb_debugPolygons[ rb_numDebugPolygons++ ];
		poly->winding	= winding;
		poly->color = idColor::PackColor( color );
		poly->lifeTime	= rb_debugPolygonTime + lifeTime;
		poly->depthTest = depthTest;
	}
}

/*
================
RB_ShowDebugPolygons
================
*/
static void RB_ShowDebugPolygons()
{
	if( !rb_numDebugPolygons )
	{
		return;
	}

	RENDERLOG_SCOPED_BLOCK( "RB_ShowDebugPolygons" );

	// all lines are expressed in world coordinates
	RB_SimpleWorldSetup();

	// RB begin
	GL_SetRenderProgram( renderProgManager.prog_vertex_color );
	GL_CommitProgUniforms( GL_GetCurrentRenderProgram() );
	// RB end

	GL_ResetTextureState();

	if( r_debugPolygonFilled.GetBool() )
	{
		GL_State( GLS_POLYGON_OFFSET | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHMASK );
		GL_PolygonOffset( -1, -2 );
	}
	else {
		GL_State( GLS_POLYGON_OFFSET | GLS_POLYMODE_LINE );
		GL_PolygonOffset( -1, -2 );
	}

	auto poly = rb_debugPolygons;
	for( int i = 0; i < rb_numDebugPolygons; i++, poly++ )
	{
//		if ( !poly->depthTest ) {

		idVec4 color;
		idColor::UnpackColor( poly->color, color );
		glColor4fv( color.ToFloatPtr() );

		glBegin( GL_POLYGON );

		for( int j = 0; j < poly->winding.GetNumPoints(); ++j )
		{
			glVertex3fv( poly->winding[j].ToFloatPtr() );
		}

		glEnd();
//		}
	}

	GL_State( GLS_DEFAULT );

	if( r_debugPolygonFilled.GetBool() )
	{
		glDisable( GL_POLYGON_OFFSET_FILL );
	}
	else {
		glDisable( GL_POLYGON_OFFSET_LINE );
	}

	GL_State( GLS_DEFAULT );
}

/*
================
RB_ShowCenterOfProjection
================
*/
static void RB_ShowCenterOfProjection()
{
	if( !r_showCenterOfProjection.GetBool() )
	{
		return;
	}

	const int w = backEnd.viewDef->scissor.GetWidth();
	const int h = backEnd.viewDef->scissor.GetHeight();
	glClearColor( 1, 0, 0, 1 );
	for( float f = 0.0f ; f <= 1.0f ; f += 0.125f )
	{
		glScissor( w * f - 1 , 0, 3, h );
		glClear( GL_COLOR_BUFFER_BIT );
		glScissor( 0, h * f - 1 , w, 3 );
		glClear( GL_COLOR_BUFFER_BIT );
	}
	glClearColor( 0, 1, 0, 1 );
	float f = 0.5f;
	glScissor( w * f - 1 , 0, 3, h );
	glClear( GL_COLOR_BUFFER_BIT );
	glScissor( 0, h * f - 1 , w, 3 );
	glClear( GL_COLOR_BUFFER_BIT );

	glScissor( 0, 0, w, h );
}

/*
================
RB_ShowLines

Draw exact pixel lines to check pixel center sampling
================
*/
static void RB_ShowLines()
{
	if( !r_showLines.GetBool() )
	{
		return;
	}

	RENDERLOG_SCOPED_BLOCK( "RB_ShowLines" );

	glEnable( GL_SCISSOR_TEST );
	if( backEnd.viewDef->GetStereoEye() == 0 )
	{
		glClearColor( 1, 0, 0, 1 );
	}
	else if( backEnd.viewDef->GetStereoEye() == 1 )
	{
		glClearColor( 0, 1, 0, 1 );
	}
	else {
		glClearColor( 0, 0, 1, 1 );
	}

	const int start = ( r_showLines.GetInteger() > 2 );	// 1,3 = horizontal, 2,4 = vertical
	if( r_showLines.GetInteger() == 1 || r_showLines.GetInteger() == 3 )
	{
		for( int i = start ; i < tr.GetHeight() ; i += 2 )
		{
			glScissor( 0, i, tr.GetWidth(), 1 );
			glClear( GL_COLOR_BUFFER_BIT );
		}
	}
	else {
		for( int i = start ; i < tr.GetWidth() ; i += 2 )
		{
			glScissor( i, 0, 1, tr.GetHeight() );
			glClear( GL_COLOR_BUFFER_BIT );
		}
	}
}


/*
================
RB_TestGamma
================
*/
#define	G_WIDTH		512
#define	G_HEIGHT	512
#define	BAR_HEIGHT	64

static void RB_TestGamma()
{
	byte	image[G_HEIGHT][G_WIDTH][4];
	int		i, j;
	int		c, comp;
	int		v, dither;
	int		mask, y;

	if( r_testGamma.GetInteger() <= 0 )
	{
		return;
	}

	v = r_testGamma.GetInteger();
	if( v <= 1 || v >= 196 )
	{
		v = 128;
	}

	memset( image, 0, sizeof( image ) );

	for( mask = 0; mask < 8; mask++ )
	{
		y = mask * BAR_HEIGHT;
		for( c = 0; c < 4; c++ )
		{
			v = c * 64 + 32;
			// solid color
			for( i = 0; i < BAR_HEIGHT / 2; i++ )
			{
				for( j = 0; j < G_WIDTH / 4; j++ )
				{
					for( comp = 0; comp < 3; comp++ )
					{
						if( mask & ( 1 << comp ) )
						{
							image[y + i][c * G_WIDTH / 4 + j][comp] = v;
						}
					}
				}
				// dithered color
				for( j = 0; j < G_WIDTH / 4; j++ )
				{
					if( ( i ^ j ) & 1 )
					{
						dither = c * 64;
					}
					else {
						dither = c * 64 + 63;
					}
					for( comp = 0; comp < 3; comp++ )
					{
						if( mask & ( 1 << comp ) )
						{
							image[y + BAR_HEIGHT / 2 + i][c * G_WIDTH / 4 + j][comp] = dither;
						}
					}
				}
			}
		}
	}

	// draw geometrically increasing steps in the bottom row
	y = 0 * BAR_HEIGHT;
	float	scale = 1;
	for( c = 0; c < 4; c++ )
	{
		v = ( int )( 64 * scale );
		if( v < 0 )
		{
			v = 0;
		}
		else if( v > 255 )
		{
			v = 255;
		}
		scale = scale * 1.5;
		for( i = 0; i < BAR_HEIGHT; i++ )
		{
			for( j = 0; j < G_WIDTH / 4; j++ )
			{
				image[y + i][c * G_WIDTH / 4 + j][0] = v;
				image[y + i][c * G_WIDTH / 4 + j][1] = v;
				image[y + i][c * G_WIDTH / 4 + j][2] = v;
			}
		}
	}

	glLoadIdentity();

	glMatrixMode( GL_PROJECTION );
	GL_State( GLS_DEPTHFUNC_ALWAYS );
	renderProgManager.SetColorParm( 1, 1, 1 );
	glPushMatrix();
	glLoadIdentity();
	glDisable( GL_TEXTURE_2D );
	glOrtho( 0, 1, 0, 1, -1, 1 );
	glRasterPos2f( 0.01f, 0.01f );
	glDrawPixels( G_WIDTH, G_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, image );
	glPopMatrix();
	glEnable( GL_TEXTURE_2D );
	glMatrixMode( GL_MODELVIEW );
}


/*
==================
RB_TestGammaBias
==================
*/
static void RB_TestGammaBias()
{
	byte	image[G_HEIGHT][G_WIDTH][4];

	if( r_testGammaBias.GetInteger() <= 0 )
	{
		return;
	}

	int y = 0;
	for( int bias = -40; bias < 40; bias += 10, y += BAR_HEIGHT )
	{
		float	scale = 1;
		for( int c = 0; c < 4; c++ )
		{
			int v = ( int )( 64 * scale + bias );
			scale = scale * 1.5;
			if( v < 0 )
			{
				v = 0;
			}
			else if( v > 255 )
			{
				v = 255;
			}
			for( int i = 0; i < BAR_HEIGHT; i++ )
			{
				for( int j = 0; j < G_WIDTH / 4; j++ )
				{
					image[y + i][c * G_WIDTH / 4 + j][0] = v;
					image[y + i][c * G_WIDTH / 4 + j][1] = v;
					image[y + i][c * G_WIDTH / 4 + j][2] = v;
				}
			}
		}
	}

	glLoadIdentity();
	glMatrixMode( GL_PROJECTION );
	GL_State( GLS_DEPTHFUNC_ALWAYS );
	renderProgManager.SetColorParm( 1, 1, 1 );
	glPushMatrix();
	glLoadIdentity();
	glDisable( GL_TEXTURE_2D );
	glOrtho( 0, 1, 0, 1, -1, 1 );
	glRasterPos2f( 0.01f, 0.01f );
	glDrawPixels( G_WIDTH, G_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, image );
	glPopMatrix();
	glEnable( GL_TEXTURE_2D );
	glMatrixMode( GL_MODELVIEW );
}

/*
================
RB_TestImage

Display a single image over most of the screen
================
*/
static void RB_TestImage()
{
	idImage* image = NULL;
	idImage* imageCr = NULL;
	idImage* imageCb = NULL;
	int		max;
	float	w, h;

	image = tr.testImage;
	if( !image )
	{
		return;
	}

	if( tr.testVideo )
	{
		cinData_t cin = tr.testVideo->ImageForTime( backEnd.viewDef->GetGameTimeMS( 1 ) - tr.testVideoStartTime );
		if( cin.imageY != NULL )
		{
			image = cin.imageY;
			imageCr = cin.imageCr;
			imageCb = cin.imageCb;
		}
		else
		{
			tr.testImage = NULL;
			return;
		}
		w = 0.25;
		h = 0.25;
	}
	else {
		max = image->GetUploadWidth() > image->GetUploadHeight() ? image->GetUploadWidth() : image->GetUploadHeight();

		w = 0.25 * image->GetUploadWidth() / max;
		h = 0.25 * image->GetUploadHeight() / max;

		w *= ( float )renderSystem->GetHeight() / renderSystem->GetWidth();
	}

	// Set State
	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );

	// Set Parms
	ALIGN16( float texS[4] ) = { 1.0f, 0.0f, 0.0f, 0.0f };
	ALIGN16( float texT[4] ) = { 0.0f, 1.0f, 0.0f, 0.0f };
	renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_S, texS );
	renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_T, texT );

	ALIGN16( float texGenEnabled[4] ) = { 0, 0, 0, 0 };
	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_ENABLED, texGenEnabled );

	// not really necessary but just for clarity
	const float screenWidth = 1.0f;
	const float screenHeight = 1.0f;
	const float halfScreenWidth = screenWidth * 0.5f;
	const float halfScreenHeight = screenHeight * 0.5f;

	idRenderMatrix scale;
	scale.Zero();
	scale[ 0 ][ 0 ] = w; // scale
	scale[ 1 ][ 1 ] = h; // scale
	scale[ 0 ][ 3 ] = halfScreenWidth - ( halfScreenWidth * w ); // translate
	scale[ 1 ][ 3 ] = halfScreenHeight - ( halfScreenHeight * h ); // translate
	scale[ 2 ][ 2 ] = 1.0f;
	scale[ 3 ][ 3 ] = 1.0f;

	idRenderMatrix ortho;
	idRenderMatrix::CreateOrthogonalProjection( screenWidth, screenHeight, 0.0, 1.0, ortho, true );

	idRenderMatrix projMatrixTranspose;
	idRenderMatrix::Multiply( ortho, scale, projMatrixTranspose );

	renderProgManager.SetRenderParms( RENDERPARM_MVPMATRIX_X, projMatrixTranspose.Ptr(), 4 );

//	glMatrixMode( GL_PROJECTION );
//	glLoadMatrixf( finalOrtho );
//	glMatrixMode( GL_MODELVIEW );
//	glLoadIdentity();

	// Set Color
	renderProgManager.SetColorParm( 1.0f, 1.0f, 1.0f, 1.0f );

	// Bind the Texture
	if( ( imageCr != NULL ) && ( imageCb != NULL ) )
	{
		renderProgManager.SetRenderParm( RENDERPARM_MAPY,  image );
		renderProgManager.SetRenderParm( RENDERPARM_MAPCR, imageCr );
		renderProgManager.SetRenderParm( RENDERPARM_MAPCB, imageCb );

		GL_SetRenderProgram( renderProgManager.prog_bink );
	}
	else {
		renderProgManager.SetRenderParm( RENDERPARM_MAP, image );

		GL_SetRenderProgram( renderProgManager.prog_texture );
	}

	// Draw!
	GL_DrawIndexed( &backEnd.testImageSurface );
}

/*
=======================================================
	RB_ShowShadowMaps
=======================================================
*/
static void RB_ShowShadowMaps()
{
	if( !r_showShadowMaps.GetBool() )
		return;

	auto image = renderDestManager.shadowBufferImage[0];
	if( !image )
	{
		return;
	}
	glTextureParameteriEXT( ( intptr_t )image->GetAPIObject(), GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_NONE );

	GL_SetRenderProgram( renderProgManager.prog_debug_shadowmap );

	// Set State
	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );

	// Set Parms
	idRenderVector texS( 1.0f, 0.0f, 0.0f, 0.0f );
	idRenderVector texT( 0.0f, 1.0f, 0.0f, 0.0f );
	renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_S, texS );
	renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_T, texT );

	idRenderVector texGenEnabled( 0.0f );
	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_ENABLED, texGenEnabled );

	for( int i = 0; i < ( r_shadowMapSplits.GetInteger() + 1 ); i++ )
	{
		int max = ( image->GetUploadWidth() > image->GetUploadHeight() )? image->GetUploadWidth() : image->GetUploadHeight();

		float w = 0.25 * image->GetUploadWidth() / max;
		float h = 0.25 * image->GetUploadHeight() / max;

		w *= ( float )renderSystem->GetHeight() / renderSystem->GetWidth();

		// not really necessary but just for clarity
		const float screenWidth = 1.0f;
		const float screenHeight = 1.0f;
		const float halfScreenWidth = screenWidth * 0.5f;
		const float halfScreenHeight = screenHeight * 0.5f;

		idRenderMatrix scale;
		scale.Zero();
		scale[ 0 ][ 0 ] = w; // scale
		scale[ 1 ][ 1 ] = h; // scale
		scale[ 0 ][ 3 ] = ( halfScreenWidth * w * 2.1f * i ); // translate
		scale[ 1 ][ 3 ] = halfScreenHeight + ( halfScreenHeight * h ); // translate
		scale[ 2 ][ 2 ] = 1.0f;
		scale[ 3 ][ 3 ] = 1.0f;

		idRenderMatrix ortho;
		idRenderMatrix::CreateOrthogonalProjection( screenWidth, screenHeight, 0.0, 1.0, ortho, true );
		idRenderMatrix::Multiply( ortho, scale,
			renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_X )->GetVec4(),
			renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_Y )->GetVec4(),
			renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_Z )->GetVec4(),
			renderProgManager.GetRenderParm( RENDERPARM_MVPMATRIX_W )->GetVec4() );

		// rpScreenCorrectionFactor
		auto & screenCorrectionParm = renderProgManager.GetRenderParm( RENDERPARM_SCREENCORRECTIONFACTOR )->GetVec4();
		screenCorrectionParm[0] = i;
		screenCorrectionParm[1] = 0.0f;
		screenCorrectionParm[2] = 0.0f;
		screenCorrectionParm[3] = 1.0f;

		//	glMatrixMode( GL_PROJECTION );
		//	glLoadMatrixf( finalOrtho );
		//	glMatrixMode( GL_MODELVIEW );
		//	glLoadIdentity();

		// Set Color
		renderProgManager.SetColorParm( 1.0f, 1.0f, 1.0f, 1.0f );

		///glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_NONE );
		renderProgManager.SetRenderParm( RENDERPARM_SHADOWBUFFERDEBUGMAP, image );

		GL_DrawIndexed( &backEnd.testImageSurface );
	}

	glTextureParameteriEXT( ( intptr_t )image->GetAPIObject(), GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
}

/*
=================
RB_DrawExpandedTriangles
=================
*/
static void DrawExpandedTriangles( const idTriangles* tri, const float radius, const idVec3& vieworg )
{
	int i, j, k;
	idVec3 dir[6], normal, point;

	for( i = 0; i < tri->numIndexes; i += 3 )
	{
		const idVec3 p[3] = {
			tri->verts[ tri->indexes[ i + 0 ] ].GetPosition(),
			tri->verts[ tri->indexes[ i + 1 ] ].GetPosition(),
			tri->verts[ tri->indexes[ i + 2 ] ].GetPosition()
		};

		dir[0] = p[0] - p[1];
		dir[1] = p[1] - p[2];
		dir[2] = p[2] - p[0];

		normal = dir[0].Cross( dir[1] );

		if( normal * p[0] < normal * vieworg )
		{
			continue;
		}

		dir[0] = normal.Cross( dir[0] );
		dir[1] = normal.Cross( dir[1] );
		dir[2] = normal.Cross( dir[2] );

		dir[0].Normalize();
		dir[1].Normalize();
		dir[2].Normalize();

		glBegin( GL_LINE_LOOP );

		for( j = 0; j < 3; j++ )
		{
			k = ( j + 1 ) % 3;

			dir[4] = ( dir[j] + dir[k] ) * 0.5f;
			dir[4].Normalize();

			dir[3] = ( dir[j] + dir[4] ) * 0.5f;
			dir[3].Normalize();

			dir[5] = ( dir[4] + dir[k] ) * 0.5f;
			dir[5].Normalize();

			point = p[k] + dir[j] * radius;
			glVertex3f( point[0], point[1], point[2] );

			point = p[k] + dir[3] * radius;
			glVertex3f( point[0], point[1], point[2] );

			point = p[k] + dir[4] * radius;
			glVertex3f( point[0], point[1], point[2] );

			point = p[k] + dir[5] * radius;
			glVertex3f( point[0], point[1], point[2] );

			point = p[k] + dir[k] * radius;
			glVertex3f( point[0], point[1], point[2] );
		}

		glEnd();
	}
}

/*
================
RB_ShowTrace

Debug visualization

FIXME: not thread safe!
================
*/
static void RB_ShowTrace( const drawSurf_t* const* drawSurfs, int numDrawSurfs )
{
	if( r_showTrace.GetInteger() == 0 )
		return;

	backEnd.ClearCurrentSpace();

	float radius = ( r_showTrace.GetInteger() == 2 )? 5.0f : 0.0f;

	// determine the points of the trace
	idVec3 start = backEnd.viewDef->GetOrigin();
	idVec3 end = start + 4000 * backEnd.viewDef->GetAxis()[0];

	// check and draw the surfaces
	renderProgManager.SetRenderParm( RENDERPARM_MAP, renderImageManager->whiteImage );

	idVec3 localStart, localEnd;

	for( int i = 0; i < numDrawSurfs; ++i )
	{
		auto const surf = drawSurfs[ i ];
		auto const tri = surf->frontEndGeo;

		if( !tri || !tri->verts )
			continue;

		// transform the points into local space
		surf->space->modelMatrix.InverseTransformPoint( start, localStart );
		surf->space->modelMatrix.InverseTransformPoint( end, localEnd );

		// check the bounding box
		if( tri->GetBounds().Expand( radius ).LineIntersection( localStart, localEnd ) )
		{
			GL_SetRenderProgram( surf->HasSkinning() ? renderProgManager.prog_texture_color_skinned : renderProgManager.prog_texture_color );
			RB_SimpleSurfaceSetup( surf );

			//
			// Highlight the surface
			//

			GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

			renderProgManager.SetColorParm( 1.0, 0.0, 0.0, 0.25 );
			GL_DrawIndexed( surf );

			//
			// Draw the bounding box
			//

			GL_State( GLS_DEPTHFUNC_ALWAYS );

			renderProgManager.SetColorParm( 1.0, 1.0, 1.0, 1.0 );
			RB_DrawBounds( tri->GetBounds(), surf->space->modelMatrix );

			/*if( radius != 0.0f )
			{
				// draw the expanded triangles
				renderProgManager.SetColorParm( 0.5f, 0.5f, 1.0f, 1.0f );
				DrawExpandedTriangles( tri, radius, localStart );
			}*/

			// check the exact surfaces
			auto hit = tri->LocalTrace( localStart, localEnd, radius );
			if( hit.fraction < 1.0 )
			{
				renderProgManager.SetColorParm( 1.0, 1.0, 1.0, 1.0 );
				RB_DrawBounds( idBounds( hit.point ).Expand( 1 ), surf->space->modelMatrix );
			}
		}
	}
}

idCVar r_useRenderDebugTools( "r_useRenderDebugTools", "0", CVAR_RENDERER | CVAR_BOOL, "" );
extern idCVar com_smp;

/*
=================================================================================

	RB_RenderDebugTools

=================================================================================
*/
void RB_RenderDebugTools( const drawSurf_t * const * drawSurfs, int numDrawSurfs )
{
	if( !r_useRenderDebugTools.GetBool() ) {
		return;
	}

	if( com_smp.GetBool() ) {
		com_smp.SetBool( false );
		return;
	}

	RENDERLOG_OPEN_MAINBLOCK( MRB_DRAW_DEBUG_TOOLS );
	RENDERLOG_OPEN_BLOCK( "RB_RenderDebugTools" );

	//GL_SetNativeRenderDestination();
	GL_SetRenderDestination( renderDestManager.renderDestBaseLDR );

	// don't do much if this was a 2D rendering
	if( backEnd.viewDef->Is2DView() )
	{
		RB_TestImage();
		RB_ShowLines();

		RENDERLOG_CLOSE_BLOCK();
		RENDERLOG_CLOSE_MAINBLOCK();
		return;
	}

	RB_ResetViewportAndScissorToDefaultCamera( backEnd.viewDef );
	GL_State( GLS_DEFAULT );

	RB_ShowLightCount();
	RB_ShowTexturePolarity( drawSurfs, numDrawSurfs );
	RB_ShowTangentSpace( drawSurfs, numDrawSurfs );
	RB_ShowVertexColor( drawSurfs, numDrawSurfs );
	RB_ShowTris( drawSurfs, numDrawSurfs );
	RB_ShowUnsmoothedTangents( drawSurfs, numDrawSurfs );
	RB_ShowSurfaceInfo( drawSurfs, numDrawSurfs );
	RB_ShowEdges( drawSurfs, numDrawSurfs );
	RB_ShowNormals( drawSurfs, numDrawSurfs );
	RB_ShowViewEntitys( backEnd.viewDef->viewEntitys );
	RB_ShowLights();
	// RB begin
	RB_ShowShadowMapLODs();
	RB_ShowShadowMaps();
	// RB end

	RB_ShowTextureVectors( drawSurfs, numDrawSurfs );
	RB_ShowDominantTris( drawSurfs, numDrawSurfs );
	if( r_testGamma.GetInteger() > 0 )  	// test here so stack check isn't so damn slow on debug builds
	{
		RB_TestGamma();
	}
	if( r_testGammaBias.GetInteger() > 0 )
	{
		RB_TestGammaBias();
	}
	RB_TestImage();
	RB_ShowPortals();
	RB_ShowSilhouette();
	RB_ShowDepthBuffer();
	RB_ShowIntensity();
	RB_ShowCenterOfProjection();
	RB_ShowLines();
	RB_ShowDebugLines();
	RB_ShowDebugText();
	RB_ShowDebugPolygons();
	RB_ShowTrace( drawSurfs, numDrawSurfs );

	RENDERLOG_CLOSE_BLOCK();
	RENDERLOG_CLOSE_MAINBLOCK();
}

/*
=================
RB_ShutdownDebugTools
=================
*/
void RB_ShutdownDebugTools()
{
	for( int i = 0; i < MAX_DEBUG_POLYGONS; i++ )
	{
		rb_debugPolygons[i].winding.Clear();
	}
}
