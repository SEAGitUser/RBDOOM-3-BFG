/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014 Robert Beckebans

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

#pragma hdrstop
#include "precompiled.h"

#include "tr_local.h"
//#include "Model_local.h"

/*
==========================================================================================

SUBVIEW SURFACES

==========================================================================================
*/

struct orientation_t
{
	idVec3		origin;
	idMat3		axis;

	void MirrorPoint( orientation_t* camera, idVec3& inout_point )
	{
		idVec3 local = inout_point - this->origin;
		idVec3 transformed( 0.0f );
		for( int i = 0; i < 3; i++ )
		{
			float d = local * this->axis[ i ];
			transformed += d * camera->axis[ i ];
		}

		inout_point = transformed + camera->origin;
	}

	void MirrorPoint( const idVec3& in_point, orientation_t* camera, idVec3& out_point )
	{
		idVec3 local = in_point - this->origin;
		idVec3 transformed( 0.0f );
		for( int i = 0; i < 3; i++ )
		{
			float d = local * this->axis[ i ];
			transformed += d * camera->axis[ i ];
		}

		out_point = transformed + camera->origin;
	}

	void MirrorVector( const idVec3& in, orientation_t* camera, idVec3& out )
	{
		out.Zero();
		for( int i = 0; i < 3; i++ )
		{
			float d = in * this->axis[ i ];
			out += d * camera->axis[ i ];
		}
	}
};

/*
=================
R_MirrorPoint
=================
*/
static void R_MirrorPoint( const idVec3& in, orientation_t* surface, orientation_t* camera, idVec3& out )
{
	const idVec3 local = in - surface->origin;
	
	idVec3 transformed( 0.0f );
	for( int i = 0; i < 3; i++ )
	{
		const float d = local * surface->axis[i];
		transformed += d * camera->axis[i];
	}
	
	out = transformed + camera->origin;
}

/*
=================
R_MirrorVector
=================
*/
static void R_MirrorVector( const idVec3& in, orientation_t* surface, orientation_t* camera, idVec3& out )
{
	out.Zero();
	for( int i = 0; i < 3; i++ )
	{
		const float d = in * surface->axis[i];
		out += d * camera->axis[i];
	}
}

/*
=============
R_PlaneForSurface

Returns the plane for the first triangle in the surface
FIXME: check for degenerate triangle?
=============
*/
static void R_PlaneForSurface( const idTriangles* tri, idPlane& plane )
{
	auto & v1 = tri->GetIVertex( 0 );
	auto & v2 = tri->GetIVertex( 1 );
	auto & v3 = tri->GetIVertex( 2 );

	plane.FromPoints( v1.GetPosition(), v2.GetPosition(), v3.GetPosition() );
}


/*
========================
 R_AllocSubView
========================
*/
static idRenderView * R_AllocSubView( const idRenderView * baseView )
{
	auto view = allocManager.FrameAlloc<idRenderView, FRAME_ALLOC_VIEW_DEF>();
	assert( view );

	// copy the viewport size from the original
	assert( baseView );
	memcpy( view, baseView, sizeof( idRenderView ) );

	view->parms.viewID = 0;	// clear to allow player bodies to show up, and suppress view weapons
	view->isSubview = true;

	//view->superView = baseView;
	//view->subviewSurface = surf;

	return view;
}


/*
========================
R_MirrorViewBySurface
========================
*/
static idRenderView* R_MirrorViewBySurface( const idRenderView * const baseView, const drawSurf_t * const drawSurf )
{
	auto view = R_AllocSubView( baseView );
	view->isMirror = true;

	// create plane axis for the portal we are seeing
	idPlane originalPlane, plane;
	R_PlaneForSurface( drawSurf->frontEndGeo, originalPlane );
	drawSurf->space->modelMatrix.TransformPlane( originalPlane, plane );
	
	orientation_t surface;
	surface.origin = plane.Normal() * -plane[3];
	surface.axis[0] = plane.Normal();
	surface.axis[0].NormalVectors( surface.axis[1], surface.axis[2] );
	surface.axis[2] = -surface.axis[2];
	
	orientation_t camera;
	camera.origin = surface.origin;
	camera.axis[0] = -surface.axis[0];
	camera.axis[1] = surface.axis[1];
	camera.axis[2] = surface.axis[2];
	
	// set the mirrored origin and axis
	R_MirrorPoint( baseView->GetOrigin(), &surface, &camera, view->parms.vieworg );
	R_MirrorVector( baseView->GetAxis()[0], &surface, &camera, view->parms.viewaxis[0] );
	R_MirrorVector( baseView->GetAxis()[1], &surface, &camera, view->parms.viewaxis[1] );
	R_MirrorVector( baseView->GetAxis()[2], &surface, &camera, view->parms.viewaxis[2] );
	
	// make the view origin 16 units away from the center of the surface
	const idVec3 center = ( drawSurf->frontEndGeo->GetBounds()[0] + drawSurf->frontEndGeo->GetBounds()[1] ) * 0.5f;
	const idVec3 viewOrigin = center + ( originalPlane.Normal() * 16.0f );
	
	drawSurf->space->modelMatrix.TransformPoint( viewOrigin, view->initialViewAreaOrigin );

	// set the mirror clip plane
	view->numClipPlanes = 1;
	view->clipPlanes[0] = -camera.axis[0];	
	view->clipPlanes[0][3] = -( camera.origin * view->clipPlanes[0].Normal() );
	
	return view;
}

/*
===============
R_RemoteRender
===============
*/
static void R_RemoteRender( idRenderView * const baseView, const drawSurf_t* surf, textureStage_t* stage )
{
	// remote views can be reused in a single frame
	if( stage->dynamicFrameCount == tr.GetFrameCount() )
	{
		return;
	}
	
	// if the entity doesn't have a remoteRenderView, do nothing
	if( !surf->space->entityDef->GetParms().remoteRenderView )
	{
		return;
	}

	auto view = R_AllocSubView( baseView );	
	view->isMirror = false;

	view->parms = *surf->space->entityDef->GetParms().remoteRenderView;
	view->initialViewAreaOrigin = view->GetOrigin();

	tr.CropRenderSize( stage->width, stage->height );
	tr.GetCroppedViewport( &view->viewport );
	
	view->scissor.x1 = 0;
	view->scissor.y1 = 0;
	view->scissor.x2 = view->viewport.x2 - view->viewport.x1;
	view->scissor.y2 = view->viewport.y2 - view->viewport.y1;
	
	view->baseView = baseView;
	view->subviewSurface = surf;
#if 0
	stage->dynamicFrameCount = tr.GetFrameCount();
	if( stage->image == NULL )
	{
		stage->image = renderImageManager->scratchImage;
	}
	tr.SetBuffer( stage->image, tr.GetFrameCount() );
#endif
	// generate render commands for it
	R_RenderView( view );
	
	// copy this rendering to the image
	stage->dynamicFrameCount = tr.GetFrameCount();
	if( stage->image == NULL )
	{
		stage->image = renderImageManager->scratchImage;
	}
	//view->subviewRenderTexture = stage->image;
	
	tr.CaptureRenderToImage( stage->image->GetName(), true );
	tr.UnCrop();
}

/*
=================
R_MirrorRender
=================
*/
static void R_MirrorRender( idRenderView * const baseView, const drawSurf_t* const surf, textureStage_t* const stage )
{
	// remote views can be reused in a single frame
	if( stage->dynamicFrameCount == tr.GetFrameCount() )
	{
		return;
	}
	
	// issue a new view command
	idRenderView* view = R_MirrorViewBySurface( baseView, surf );

	tr.CropRenderSize( stage->width, stage->height );	
	tr.GetCroppedViewport( &view->viewport );

	//render->SetRenderSize( renderView, screenRect, fracX, fracY );
	
	view->scissor.x1 = 0;
	view->scissor.y1 = 0;
	view->scissor.x2 = view->viewport.x2 - view->viewport.x1;
	view->scissor.y2 = view->viewport.y2 - view->viewport.y1;
	
	view->baseView = baseView;
	view->subviewSurface = surf;
	
	// triangle culling order changes with mirroring
	view->isMirror = ( ( ( int )view->isMirror ^ ( int )baseView->isMirror ) != 0 );
	
	// generate render commands for it
	R_RenderView( view );

	//render->RenderSingleView( hdc, world_, renderView_, screenRect );
	
	// copy this rendering to the image
	stage->dynamicFrameCount = tr.GetFrameCount();
	stage->image = renderImageManager->scratchImage;
	
	//view->subviewRenderTexture = stage->image;
	
	tr.CaptureRenderToImage( stage->image->GetName() );
	tr.UnCrop();
}

/*
=================
R_XrayRender
=================
*/
static void R_XrayRender( idRenderView * const baseView, const drawSurf_t* const surf, textureStage_t* const stage )
{
	// remote views can be reused in a single frame
	if( stage->dynamicFrameCount == tr.GetFrameCount() )
	{
		return;
	}
	
	// issue a new view command
	auto view = R_AllocSubView( baseView );
	view->isXraySubview = true;

	tr.CropRenderSize( stage->width, stage->height );
	tr.GetCroppedViewport( &view->viewport );
	
	view->scissor.x1 = 0;
	view->scissor.y1 = 0;
	view->scissor.x2 = view->viewport.x2 - view->viewport.x1;
	view->scissor.y2 = view->viewport.y2 - view->viewport.y1;
	
	view->baseView = baseView;
	view->subviewSurface = surf;
	
	// triangle culling order changes with mirroring
	view->isMirror = ( ( ( int )view->isMirror ^ ( int )baseView->isMirror ) != 0 );
	
	// generate render commands for it
	R_RenderView( view );
	
	// copy this rendering to the image
	stage->dynamicFrameCount = tr.GetFrameCount();
	stage->image = renderImageManager->scratchImage2;

	//view->subviewRenderTexture = stage->image;
	
	tr.CaptureRenderToImage( stage->image->GetName(), true );
	tr.UnCrop();
}

/*
==================
R_GenerateSurfaceSubview
==================
*/
static bool R_GenerateSurfaceSubview( idRenderView * const baseView, const drawSurf_t * const drawSurf )
{
	idBounds ndcBounds;
	if( baseView->PreciseCullSurface( drawSurf, ndcBounds ) )
	{
		return false;
	}
	
	// never recurse through a subview surface that we are
	// already seeing through
	idRenderView* view = NULL;
	for( view = baseView; view != NULL; view = view->baseView )
	{
		if( view->subviewSurface != NULL &&
			view->subviewSurface->frontEndGeo == drawSurf->frontEndGeo &&
			view->subviewSurface->space->entityDef == drawSurf->space->entityDef )
		{
			break;
		}
	}
	if( view )
	{
		return false;
	}
	
	// crop the scissor bounds based on the precise cull
	assert( baseView != NULL );
	auto & v = baseView->GetViewport();
	idScreenRect scissor;
	scissor.x1 = v.x1 + idMath::Ftoi( ( v.x2 - v.x1 + 1 ) * 0.5f * ( ndcBounds[0][0] + 1.0f ) );
	scissor.y1 = v.y1 + idMath::Ftoi( ( v.y2 - v.y1 + 1 ) * 0.5f * ( ndcBounds[0][1] + 1.0f ) );
	scissor.x2 = v.x1 + idMath::Ftoi( ( v.x2 - v.x1 + 1 ) * 0.5f * ( ndcBounds[1][0] + 1.0f ) );
	scissor.y2 = v.y1 + idMath::Ftoi( ( v.y2 - v.y1 + 1 ) * 0.5f * ( ndcBounds[1][1] + 1.0f ) );
	
	// nudge a bit for safety
	scissor.Expand();
	
	scissor.Intersect( baseView->GetScissor() );
	
	if( scissor.IsEmpty() )
	{
		// cropped out
		return false;
	}
	
	// see what kind of subview we are making
	if( drawSurf->material->GetSort() != SS_SUBVIEW )
	{
		for( int i = 0; i < drawSurf->material->GetNumStages(); i++ )
		{
			auto stage = drawSurf->material->GetStage( i );
			switch( stage->texture.dynamic )
			{
				case DI_REMOTE_RENDER:
					R_RemoteRender( baseView, drawSurf, const_cast<textureStage_t*>( &stage->texture ) );
					break;
				case DI_MIRROR_RENDER:
					R_MirrorRender( baseView, drawSurf, const_cast<textureStage_t*>( &stage->texture ) );
					break;
				case DI_XRAY_RENDER:
					R_XrayRender( baseView, drawSurf, const_cast<textureStage_t*>( &stage->texture ) );
					break;
			}
		}
		return true;
	}
	
	// issue a new view command
	view = R_MirrorViewBySurface( baseView, drawSurf );

	view->scissor = scissor;

	view->baseView = baseView;
	view->subviewSurface = drawSurf;
	
	// triangle culling order changes with mirroring
	view->isMirror = ( ( ( int )view->isMirror ^ ( int )baseView->isMirror ) != 0 );
	
	// generate render commands for it
	R_RenderView( view );
	
	return true;
}

/*
================
R_GenerateSubViews

	If we need to render another view to complete the current view,
	generate it first.

	It is important to do this after all drawSurfs for the current
	view have been generated, because it may create a subview which
	would change tr.viewCount.
================
*/
bool R_GenerateSubViews( idRenderView * const renderView )
{
	SCOPED_PROFILE_EVENT( "R_GenerateSubViews" );
	
	// for testing the performance hit
	if( r_skipSubviews.GetBool() )
	{
		return false;
	}
	
	// scan the surfaces until we either find a subview, or determine
	// there are no more subview surfaces.
	bool subviews = false;
	for( int i = 0; i < renderView->numDrawSurfs; ++i )
	{
		const drawSurf_t * const drawSurf = renderView->drawSurfs[ i ];
		
		if( drawSurf->material->HasSubview() )
		{
			if( R_GenerateSurfaceSubview( renderView, drawSurf ) )
			{
				subviews = true;
			}
		}
	}
	
	return subviews;
}
