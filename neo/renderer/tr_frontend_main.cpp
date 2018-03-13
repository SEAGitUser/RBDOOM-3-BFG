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

/*
==========================================================================================

FONT-END RENDERING

==========================================================================================
*/

extern void R_AddInGameGuis( idRenderView * const );
extern void R_OptimizeViewLightsList( idRenderView * const );
extern bool R_GenerateSubViews( idRenderView * const );
extern void R_AddLights( idRenderView * const );
extern void R_AddModels( idRenderView * const );

/*
=================
R_SortDrawSurfs
=================
*/
static void R_SortDrawSurfs( idRenderView * renderView )
{
	drawSurf_t ** drawSurfs = renderView->drawSurfs;
	const int numDrawSurfs = renderView->numDrawSurfs;

#if 1
	uint64* indices = ( uint64* ) _alloca16( numDrawSurfs * sizeof( indices[0] ) );
	
	// sort the draw surfs based on:
	// 1. sort value (largest first)
	// 2. depth (smallest first)
	// 3. index (largest first)
	assert( numDrawSurfs <= 0xFFFF );
	for( int i = 0; i < numDrawSurfs; i++ )
	{
		auto const surf = drawSurfs[ i ];
		auto const material = drawSurfs[ i ]->material;

		float sort = SS_POST_PROCESS - surf->sort;
		assert( sort >= 0.0f );
		
		uint64 dist = 0;
		if( surf->frontEndGeo != NULL )
		{
			float min = 0.0f;
			float max = 1.0f;
			idRenderMatrix::DepthBoundsForBounds( min, max, surf->space->mvp, surf->frontEndGeo->GetBounds() );
			dist = idMath::Ftoui16( min * 0xFFFF );
		}
		
		indices[i] = ( ( numDrawSurfs - i ) & 0xFFFF ) | ( dist << 16 ) | ( ( uint64 )( *( uint32* )&sort ) << 32 );
	}
	
	const int64 MAX_LEVELS = 128;
	int64 lo[MAX_LEVELS];
	int64 hi[MAX_LEVELS];
	
	// Keep the top of the stack in registers to avoid load-hit-stores.
	register int64 st_lo = 0;
	register int64 st_hi = numDrawSurfs - 1;
	register int64 level = 0;
	
	for( ; ; )
	{
		register int64 i = st_lo;
		register int64 j = st_hi;
		if( j - i >= 4 && level < MAX_LEVELS - 1 )
		{
			register uint64 pivot = indices[( i + j ) / 2];
			do
			{
				while( indices[i] > pivot ) i++;
				while( indices[j] < pivot ) j--;
				if( i > j ) break;
				uint64 h = indices[i];
				indices[i] = indices[j];
				indices[j] = h;
			}
			while( ++i <= --j );
			
			// No need for these iterations because we are always sorting unique values.
			//while ( indices[j] == pivot && st_lo < j ) j--;
			//while ( indices[i] == pivot && i < st_hi ) i++;
			
			assert( level < MAX_LEVELS - 1 );
			lo[level] = i;
			hi[level] = st_hi;
			st_hi = j;
			level++;
		}
		else
		{
			for( ; i < j; j-- )
			{
				register int64 m = i;
				for( int64 k = i + 1; k <= j; k++ )
				{
					if( indices[k] < indices[m] )
					{
						m = k;
					}
				}
				uint64 h = indices[m];
				indices[m] = indices[j];
				indices[j] = h;
			}
			if( --level < 0 )
			{
				break;
			}
			st_lo = lo[level];
			st_hi = hi[level];
		}
	}
	
	drawSurf_t** newDrawSurfs = ( drawSurf_t** ) indices;
	for( int i = 0; i < numDrawSurfs; i++ )
	{
		newDrawSurfs[i] = drawSurfs[numDrawSurfs - ( indices[i] & 0xFFFF )];
	}
	memcpy( drawSurfs, newDrawSurfs, numDrawSurfs * sizeof( drawSurfs[0] ) );
	
#else
	
	struct local_t
	{
		static int R_QsortSurfaces( const void* a, const void* b )
		{
			const drawSurf_t* ea = *( drawSurf_t** )a;
			const drawSurf_t* eb = *( drawSurf_t** )b;
			if( ea->sort < eb->sort )
			{
				return -1;
			}
			if( ea->sort > eb->sort )
			{
				return 1;
			}
			return 0;
		}
	};
	
	// Add a sort offset so surfaces with equal sort orders still deterministically
	// draw in the order they were added, at least within a given model.
	float sorfOffset = 0.0f;
	for( int i = 0; i < numDrawSurfs; i++ )
	{
		drawSurf[i]->sort += sorfOffset;
		sorfOffset += 0.000001f;
	}
	
	// sort the drawsurfs
	qsort( drawSurfs, numDrawSurfs, sizeof( drawSurfs[0] ), local_t::R_QsortSurfaces );
	
#endif
}

/*
================
 R_RenderView

	A view may be either the actual camera view,
	a mirror / remote location, or a 3D view on a gui surface.

	renderView will typically be allocated with R_FrameAlloc
================
*/
void R_RenderView( idRenderView * renderView )
{
	// save view in case we are a subview
	idRenderView* oldView = tr.viewDef;
	tr.viewDef = renderView;
	
	renderView->DeriveData();
	
	// identify all the visible portal areas, and create view lights and view entities
	// for all the the entityDefs and lightDefs that are in the visible portal areas
	renderView->renderWorld->FindViewLightsAndEntities();
	
	// wait for any shadow volume jobs from the previous frame to finish
	tr.frontEndJobList->Wait();
	
	// make sure that interactions exist for all light / entity combinations that are visible
	// add any pre-generated light shadows, and calculate the light shader values
	R_AddLights( renderView );
	
	// adds ambient surfaces and create any necessary interaction surfaces to add to the light lists
	R_AddModels( renderView );
	
	// build up the GUIs on world surfaces
	R_AddInGameGuis( renderView );
	
	// any viewLight that didn't have visible surfaces can have it's shadows removed
	R_OptimizeViewLightsList( renderView );
	
	// sort all the ambient surfaces for translucency ordering
	R_SortDrawSurfs( renderView );
	
	// generate any subviews (mirrors, cameras, etc) before adding this view
	if( R_GenerateSubViews( renderView ) )
	{
		// if we are debugging subviews, allow the skipping of the main view draw
		if( r_subviewOnly.GetBool() )
		{
			return;
		}
	}
	
	// write everything needed to the demo file
	if( common->WriteDemo() )
	{
		 renderView->renderWorld->WriteVisibleDefs( renderView );
	}
	
	// add the rendering commands for this viewDef
	R_AddDrawViewCmd( renderView, false );
	
	// restore view in case we are a subview
	tr.viewDef = oldView;
}

/*
================
R_RenderPostProcess

Because R_RenderView may be called by subviews we have to make sure the post process
pass happens after the active view and its subviews is done rendering.
================
*/
void R_RenderPostProcess( idRenderView * renderView )
{
	idRenderView* oldView = tr.viewDef;
	
	R_AddDrawPostProcess( renderView );
	
	tr.viewDef = oldView;
}