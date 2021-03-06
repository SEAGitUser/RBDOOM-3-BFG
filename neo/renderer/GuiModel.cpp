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

#pragma hdrstop
#include "precompiled.h"

#include "tr_local.h"

const float idRenderModelGui::STEREO_DEPTH_NEAR = 0.0f;
const float idRenderModelGui::STEREO_DEPTH_MID = 0.5f;
const float idRenderModelGui::STEREO_DEPTH_FAR = 1.0f;

/*
================
idRenderModelGui::idRenderModelGui
================
*/
idRenderModelGui::idRenderModelGui()
{
	// identity color for drawsurf register evaluation
	for( int i = 0; i < MAX_ENTITY_SHADER_PARMS; i++ )
	{
		shaderParms[ i ] = 1.0f;
	}
}

/*
================
idRenderModelGui::Clear

Begins collecting draw commands into surfaces
================
*/
void idRenderModelGui::Clear()
{
	surfaces.SetNum( 0 );
	AdvanceSurf();
}

/*
================
idRenderModelGui::WriteToDemo
================
*/
void idRenderModelGui::WriteToDemo( idDemoFile* demo )
{}

/*
================
idRenderModelGui::ReadFromDemo
================
*/
void idRenderModelGui::ReadFromDemo( idDemoFile* demo )
{}

/*
================
idRenderModelGui::BeginFrame
================
*/
void idRenderModelGui::BeginFrame()
{
	vertexBlock = vertexCache.AllocVertex( NULL, ALIGN( MAX_VERTS * sizeof( idDrawVert ), VERTEX_CACHE_ALIGN ) );
	indexBlock = vertexCache.AllocIndex( NULL, ALIGN( MAX_INDEXES * sizeof( triIndex_t ), INDEX_CACHE_ALIGN ) );
	vertexPointer = ( idDrawVert* ) vertexCache.MappedVertexBuffer( vertexBlock );
	indexPointer = ( triIndex_t* ) vertexCache.MappedIndexBuffer( indexBlock );
	numVerts = 0;
	numIndexes = 0;
	Clear();
}

idCVar	stereoRender_defaultGuiDepth( "stereoRender_defaultGuiDepth", "0", CVAR_RENDERER, "Fraction of separation when not specified" );
/*
================
EmitSurfaces

	For full screen GUIs, we can add in per-surface stereoscopic depth effects
================
*/
void idRenderModelGui::EmitSurfaces( idRenderView * view,
									 const idRenderMatrix & modelMatrix, const idRenderMatrix & modelViewMatrix,
									 bool depthHack, bool allowFullScreenStereoDepth, bool linkAsEntity )
{
	auto guiSpace = allocManager.FrameAlloc<viewModel_t, FRAME_ALLOC_VIEW_ENTITY, true>();

	guiSpace->modelMatrix.Copy( modelMatrix );
	guiSpace->modelViewMatrix.Copy( modelViewMatrix );
	idRenderMatrix::Multiply( view->GetProjectionMatrix(), modelViewMatrix, guiSpace->mvp );
	if( depthHack ) idRenderMatrix::ApplyDepthHack( guiSpace->mvp );

	guiSpace->weaponDepthHack = depthHack;
	guiSpace->isGuiSurface = true;

	// If this is an in-game gui, we need to be able to find the matrix again for head mounted
	// display bypass matrix fixup.
	if( linkAsEntity )
	{
		guiSpace->next = view->viewEntitys;
		view->viewEntitys = guiSpace;
	}

	// to allow 3D-TV effects in the menu system, we define surface flags to set
	// depth fractions between 0=screen and 1=infinity, which directly modulate the
	// screenSeparation parameter for an X offset.
	// The value is stored in the drawSurf sort value, which adjusts the matrix in the
	// backend.
	float defaultStereoDepth = stereoRender_defaultGuiDepth.GetFloat();	// default to at-screen

	// add the surfaces to this view
	for( int i = 0; i < surfaces.Num(); ++i )
	{
		const auto & guiSurf = surfaces[ i ];
		if( guiSurf.numIndexes == 0 )
		{
			continue;
		}

		auto drawSurf = allocManager.FrameAlloc<drawSurf_t, FRAME_ALLOC_DRAW_SURFACE>();
		drawSurf->numIndexes = guiSurf.numIndexes;
		drawSurf->vertexCache = vertexBlock;
		// build a vertCacheHandle_t that points inside the allocated block
		drawSurf->indexCache = indexBlock + ( ( int64 ) ( guiSurf.firstIndex * sizeof( triIndex_t ) ) << VERTCACHE_OFFSET_SHIFT );
		drawSurf->shadowCache = 0;
		drawSurf->jointCache = 0;
		drawSurf->frontEndGeo = NULL;
		drawSurf->space = guiSpace;
		drawSurf->material = guiSurf.material;
		drawSurf->extraGLState = guiSurf.glState;
		drawSurf->scissorRect = view->GetScissor();
		drawSurf->sort = guiSurf.material->GetSort();
		drawSurf->renderZFail = 0;
		// process the shader expressions for conditionals / color / texcoords
		const float* constRegs = guiSurf.material->ConstantRegisters();
		if( constRegs )
		{
			// shader only uses constant values
			drawSurf->shaderRegisters = constRegs;
		}
		else {
			auto regs = allocManager.FrameAlloc<float, FRAME_ALLOC_SHADER_REGISTER>( guiSurf.material->GetNumRegisters() );
			drawSurf->shaderRegisters = regs;
			guiSurf.material->EvaluateRegisters( regs, shaderParms, view->GetGlobalMaterialParms(), view->GetGameTimeSec( 1 ), NULL );
		}

		R_LinkDrawSurfToView( drawSurf, view );

		if( allowFullScreenStereoDepth )
		{
			// override sort with the stereoDepth
			//drawSurf->sort = stereoDepth;

			switch( guiSurf.stereoType )
			{
				case STEREO_DEPTH_TYPE_NEAR:
					drawSurf->sort = STEREO_DEPTH_NEAR;
					break;
				case STEREO_DEPTH_TYPE_MID:
					drawSurf->sort = STEREO_DEPTH_MID;
					break;
				case STEREO_DEPTH_TYPE_FAR:
					drawSurf->sort = STEREO_DEPTH_FAR;
					break;
				case STEREO_DEPTH_TYPE_NONE:
				default:
					drawSurf->sort = defaultStereoDepth;
					break;
			}
		}
	}
}

/*
====================
 EmitToCurrentView
====================
*/
void idRenderModelGui::EmitToView( idRenderView * view, const idRenderMatrix & modelMatrix, bool depthHack )
{
	idRenderMatrix modelViewMatrix;
	idRenderMatrix::Multiply( view->GetViewMatrix(), modelMatrix, modelViewMatrix );

	EmitSurfaces( view, modelMatrix, modelViewMatrix, depthHack, false /* stereoDepthSort */, true /* link as entity */ );
}

extern idCVar stereoRender_interOccularCentimeters;
extern idCVar stereoRender_convergence;
extern idCVar stereoRender_screenSeparation;	// screen units from center to eyes
extern idCVar stereoRender_swapEyes;

/*
================
idRenderModelGui::EmitFullScreen

Creates a view that covers the screen and emit the surfaces
================
*/
void idRenderModelGui::EmitFullScreen()
{
	if( surfaces[ 0 ].numIndexes == 0 )
	{
		return;
	}

	SCOPED_PROFILE_EVENT( "Gui::EmitFullScreen" );

	auto viewDef = allocManager.FrameAlloc<idRenderView, FRAME_ALLOC_VIEW_DEF, true>();

	viewDef->is2Dgui = true;

	tr.GetCroppedViewport( &viewDef->viewport );

	viewDef->scissor.x1 = 0;
	viewDef->scissor.y1 = 0;
	viewDef->scissor.x2 = viewDef->viewport.x2 - viewDef->viewport.x1;
	viewDef->scissor.y2 = viewDef->viewport.y2 - viewDef->viewport.y1;

	bool stereoEnabled = ( renderSystem->GetStereo3DMode() != STEREO3D_OFF );
	if( stereoEnabled )
	{
		const stereoDistances_t dists = renderSystem->CaclulateStereoDistances(
											stereoRender_interOccularCentimeters.GetFloat(),
											renderSystem->GetPhysicalScreenWidthInCentimeters(),
											stereoRender_convergence.GetFloat(),
											80.0f /* fov */ );

		// this will be negated on the alternate eyes, both rendered each frame
		viewDef->parms.stereoScreenSeparation = dists.screenSeparation;

		extern idCVar stereoRender_swapEyes;
		viewDef->parms.viewEyeBuffer = 0;	// render to both buffers
		if( stereoRender_swapEyes.GetBool() )
		{
			viewDef->parms.stereoScreenSeparation = -dists.screenSeparation;
		}
	}

	viewDef->DeriveData();

	viewDef->maxDrawSurfs = surfaces.Num();
	viewDef->drawSurfs = allocManager.FrameAlloc<drawSurf_t*, FRAME_ALLOC_DRAW_SURFACE_POINTER>( viewDef->maxDrawSurfs );
	viewDef->numDrawSurfs = 0;

	// RB: give renderView the current time to calculate 2D shader effects
	int shaderTime = SEC2MS( tr.frameShaderTime );  //tr.frameShaderTime * 1000; //sys->Milliseconds();
	viewDef->parms.time[ 0 ] = shaderTime;
	viewDef->parms.time[ 1 ] = shaderTime;
	// RB end

	EmitSurfaces( viewDef, renderMatrix_identity, renderMatrix_identity,
				  false /* depthHack */, stereoEnabled /* stereoDepthSort */, false /* link as entity */ );

	// add the command to draw this view
	R_AddDrawViewCmd( viewDef, true );
}

/*
=============
AdvanceSurf
=============
*/
void idRenderModelGui::AdvanceSurf()
{
	guiModelSurface_t s;

	if( surfaces.Num() )
	{
		s.material = surf->material;
		s.glState = surf->glState;
	}
	else {
		s.material = tr.defaultMaterial;
		s.glState = 0;
	}

	// advance indexes so the pointer to each surface will be 16 byte aligned
	numIndexes = ALIGN( numIndexes, 8 );

	s.numIndexes = 0;
	s.firstIndex = numIndexes;

	surfaces.Append( s );
	surf = &surfaces[ surfaces.Num() - 1 ];
}

/*
=============
AllocTris
=============
*/
idDrawVert * idRenderModelGui::AllocTris( int vertCount, const triIndex_t* tempIndexes, int indexCount,
										  const idMaterial* material, const uint64 glState,
										  const stereoDepthType_t stereoType )
{
	if( material == NULL )
	{
		return NULL;
	}
	if( numIndexes + indexCount > MAX_INDEXES )
	{
		static int warningFrame = 0;
		if( warningFrame != tr.GetFrameCount() )
		{
			warningFrame = tr.GetFrameCount();
			idLib::Warning( "idRenderModelGui::AllocTris: MAX_INDEXES exceeded" );
		}
		return NULL;
	}
	if( numVerts + vertCount > MAX_VERTS )
	{
		static int warningFrame = 0;
		if( warningFrame != tr.GetFrameCount() )
		{
			warningFrame = tr.GetFrameCount();
			idLib::Warning( "idRenderModelGui::AllocTris: MAX_VERTS exceeded" );
		}
		return NULL;
	}

	// break the current surface if we are changing to a new material or we can't
	// fit the data into our allocated block
	if( material != surf->material || glState != surf->glState || stereoType != surf->stereoType )
	{
		if( surf->numIndexes )
		{
			AdvanceSurf();
		}
		surf->material = material;
		surf->glState = glState;
		surf->stereoType = stereoType;
	}

	int startVert = numVerts;
	int startIndex = numIndexes;

	numVerts += vertCount;
	numIndexes += indexCount;

	surf->numIndexes += indexCount;

	if( ( startIndex & 1 ) || ( indexCount & 1 ) )
	{
		// slow for write combined memory!
		// this should be very rare, since quads are always an even index count
		for( int i = 0; i < indexCount; i++ )
		{
			indexPointer[ startIndex + i ] = startVert + tempIndexes[ i ];
		}
	}
	else {
		for( int i = 0; i < indexCount; i += 2 )
		{
			WriteIndexPair( indexPointer + startIndex + i, startVert + tempIndexes[ i ], startVert + tempIndexes[ i + 1 ] );
		}
	}

	return vertexPointer + startVert;
}