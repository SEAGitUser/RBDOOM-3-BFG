/*
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
*/

#pragma hdrstop
#include "precompiled.h"

#include "tr_local.h"

/*
===================
idRenderWorldLocal::idRenderWorldLocal
===================
*/
idRenderWorldLocal::idRenderWorldLocal()
{
	mapName.Clear();
	mapTimeStamp = FILE_NOT_FOUND_TIMESTAMP;

	generateAllInteractionsCalled = false;

	areaNodes = NULL;
	numAreaNodes = 0;

	portalAreas = NULL;
	numPortalAreas = 0;

	doublePortals = NULL;
	numInterAreaPortals = 0;

	interactionTable = 0;
	interactionTableWidth = 0;
	interactionTableHeight = 0;

	for( int i = 0; i < decals.Num(); ++i )
	{
		decals[i].entityHandle = -1;
		decals[i].lastStartTime = 0;
		decals[i].decals = new ( TAG_MODEL ) idRenderModelDecal();
		decals[i].decals->index = i;
	}

	for( int i = 0; i < overlays.Num(); ++i )
	{
		overlays[i].entityHandle = -1;
		overlays[i].lastStartTime = 0;
		overlays[i].overlays = new ( TAG_MODEL ) idRenderModelOverlay();
		overlays[i].overlays->index = i;
	}
}

/*
===================
idRenderWorldLocal::~idRenderWorldLocal
===================
*/
idRenderWorldLocal::~idRenderWorldLocal()
{
	// free all the entityDefs, lightDefs, portals, etc
	FreeWorld();

	for( int i = 0; i < decals.Num(); ++i )
	{
		delete decals[i].decals;
	}

	for( int i = 0; i < overlays.Num(); ++i )
	{
		delete overlays[i].overlays;
	}

	// free up the debug lines, polys, and text
	RB_ClearDebugPolygons( 0 );
	RB_ClearDebugLines( 0 );
	RB_ClearDebugText( 0 );
}

/*
===================
ResizeInteractionTable
===================
*/
void idRenderWorldLocal::ResizeInteractionTable()
{
	// we overflowed the interaction table, so make it larger
	common->Printf( "idRenderWorldLocal::ResizeInteractionTable: overflowed interactionTable, resizing\n" );

	const int oldInteractionTableWidth = interactionTableWidth;
	const int oldIinteractionTableHeight = interactionTableHeight;
	idInteraction** oldInteractionTable = interactionTable;

	// build the interaction table
	// this will be dynamically resized if the entity / light counts grow too much
	interactionTableWidth = entityDefs.Num() + 100;
	interactionTableHeight = lightDefs.Num() + 100;

	interactionTable = allocManager.StaticAlloc<idInteraction*, TAG_RENDER_INTERACTION, true>( interactionTableWidth * interactionTableHeight );

	for( int l = 0; l < oldIinteractionTableHeight; ++l )
	{
		for( int e = 0; e < oldInteractionTableWidth; ++e )
		{
			interactionTable[ l * interactionTableWidth + e ] = oldInteractionTable[ l * oldInteractionTableWidth + e ];
		}
	}

	allocManager.StaticFree( oldInteractionTable );
}

/*
===================
AddEntityDef
===================
*/
qhandle_t idRenderWorldLocal::AddEntityDef( const renderEntityParms_t* re )
{
	// try and reuse a free spot
	int entityHandle = entityDefs.FindNull();
	if( entityHandle == -1 )
	{
		entityHandle = entityDefs.Append( NULL );

		if( interactionTable && entityDefs.Num() > interactionTableWidth )
		{
			ResizeInteractionTable();
		}
	}

	UpdateEntityDef( entityHandle, re );

	return entityHandle;
}

/*
==============
UpdateEntityDef

	Does not write to the demo file, which will only be updated for
	visible entities
==============
*/
///int c_callbackUpdate;
void idRenderWorldLocal::UpdateEntityDef( qhandle_t entityHandle, const renderEntityParms_t* re )
{
	if( r_skipUpdates.GetBool() )
	{
		return;
	}

	tr.pc.c_entityUpdates++;

	if( !re->hModel && !re->callback )
	{
		common->Error( "idRenderWorld::UpdateEntityDef: NULL hModel" );
	}

	// create new slots if needed
	if( entityHandle < 0 || entityHandle > LUDICROUS_INDEX )
	{
		common->Error( "idRenderWorld::UpdateEntityDef: index = %i", entityHandle );
	}
	while( entityHandle >= entityDefs.Num() )
	{
		entityDefs.Append( NULL );
	}

	auto def = entityDefs[ entityHandle ];
	if( def != NULL )
	{
		if( !re->forceUpdate )
		{
			// check for exact match (OPTIMIZE: check through pointers more)
			if( !re->joints && !re->callbackData && !def->dynamicModel && !memcmp( re, &def->parms, sizeof( *re ) ) )
				return;

			// if the only thing that changed was shaderparms, we can just leave things as they are
			// after updating parms

			// if we have a callback function and the bounds, origin, axis and model match,
			// then we can leave the references as they are
			if( re->callback )
			{
				bool axisMatch = ( re->axis == def->parms.axis );
				bool originMatch = ( re->origin == def->parms.origin );
				bool boundsMatch = ( re->bounds == def->localReferenceBounds );
				bool modelMatch = ( re->hModel == def->GetModel() );

				if( boundsMatch && originMatch && axisMatch && modelMatch )
				{
					// only clear the dynamic model and interaction surfaces if they exist
					///++c_callbackUpdate;
					def->ClearDynamicModel();
					def->parms = *re; // copy parms
					return;
				}
			}
		}

		// save any decals if the model is the same, allowing marks to move with entities
		if( def->GetModel() == re->hModel )
		{
			def->FreeDerivedData( true, true );
		}
		else {
			def->FreeDerivedData( false, false );
		}
	}
	else {
		// creating a new one
		def = new( TAG_RENDER_ENTITY ) idRenderEntityLocal;
		entityDefs[ entityHandle ] = def;

		def->world = this;
		def->index = entityHandle;
	}

	def->parms = *re; // copy parms

	def->lastModifiedFrameNum = tr.GetFrameCount();
	def->archived = false;

	// optionally immediately issue any callbacks
	if( !r_useEntityCallbacks.GetBool() && def->HasCallback() )
	{
		def->IssueCallback();
	}

	// trigger entities don't need to get linked in and processed,
	// they only exist for editor use
	if( def->GetModel() != NULL && !def->GetModel()->ModelHasDrawingSurfaces() )
		return;

	// based on the model bounds, add references in each area
	// that may contain the updated surface
	def->CreateReferences();
}

/*
===================
FreeEntityDef

	Frees all references and lit surfaces from the model, and
	NULL's out it's entry in the world list
===================
*/
void idRenderWorldLocal::FreeEntityDef( qhandle_t entityHandle )
{
	if( entityHandle < 0 || entityHandle >= entityDefs.Num() )
	{
		common->Printf( "idRenderWorld::FreeEntityDef: handle %i > %i\n", entityHandle, entityDefs.Num() );
		return;
	}

	auto def = entityDefs[ entityHandle ];
	if( !def )
	{
		common->Printf( "idRenderWorld::FreeEntityDef: handle %i is NULL\n", entityHandle );
		return;
	}

	def->FreeDerivedData( false, false );

	if( common->WriteDemo() && def->archived )
	{
		WriteFreeEntity( entityHandle );
	}

	// if we are playing a demo, these will have been freed
	// in R_FreeEntityDefDerivedData(), otherwise the gui
	// object still exists in the game

	def->parms.gui[ 0 ] = nullptr;
	def->parms.gui[ 1 ] = nullptr;
	def->parms.gui[ 2 ] = nullptr;

	delete def;
	entityDefs[ entityHandle ] = nullptr;
}

/*
==================
 GetRenderEntity
==================
*/
const renderEntityParms_t * idRenderWorldLocal::GetRenderEntityParms( qhandle_t entityHandle ) const
{
	if( entityHandle < 0 || entityHandle >= entityDefs.Num() )
	{
		common->Printf( "idRenderWorld::GetRenderEntityParms: invalid handle %i [0, %i]\n", entityHandle, entityDefs.Num() );
		return nullptr;
	}

	auto def = entityDefs[ entityHandle ];
	if( !def ) {
		common->Printf( "idRenderWorld::GetRenderEntityParms: handle %i is NULL\n", entityHandle );
		return nullptr;
	}

	return &def->parms;
}

/*
==================
AddLightDef
==================
*/
qhandle_t idRenderWorldLocal::AddLightDef( const renderLightParms_t* rlight )
{
	// try and reuse a free spot
	int lightHandle = lightDefs.FindNull();
	if( lightHandle == -1 )
	{
		lightHandle = lightDefs.Append( nullptr );
		if( interactionTable && lightDefs.Num() > interactionTableHeight )
		{
			ResizeInteractionTable();
		}
	}
	UpdateLightDef( lightHandle, rlight );
	return lightHandle;
}

/*
=================
UpdateLightDef

	The generation of all the derived interaction data will
	usually be deferred until it is visible in a scene.

	Does not write to the demo file, which will only be done for visible lights.
=================
*/
void idRenderWorldLocal::UpdateLightDef( qhandle_t lightHandle, const renderLightParms_t* rlight )
{
	if( r_skipUpdates.GetBool() )
		return;

	tr.pc.c_lightUpdates++;

	// create new slots if needed
	if( lightHandle < 0 || lightHandle > LUDICROUS_INDEX )
	{
		common->Error( "idRenderWorld::UpdateLightDef: index = %i", lightHandle );
	}
	while( lightHandle >= lightDefs.Num() )
	{
		lightDefs.Append( nullptr );
	}

	bool justUpdate = false;
	idRenderLightLocal* light = lightDefs[ lightHandle ];
	if( light )
	{
		// if the shape of the light stays the same, we don't need to dump
		// any of our derived data, because shader parms are calculated every frame
		if( rlight->axis			== light->parms.axis &&
			rlight->end				== light->parms.end &&
			rlight->lightCenter		== light->parms.lightCenter &&
			rlight->lightRadius		== light->parms.lightRadius &&
			rlight->noShadows		== light->parms.noShadows &&
			rlight->origin			== light->parms.origin &&
			rlight->parallel		== light->parms.parallel &&
			rlight->pointLight		== light->parms.pointLight &&
			rlight->right			== light->parms.right &&
			rlight->start			== light->parms.start &&
			rlight->target			== light->parms.target &&
			rlight->up				== light->parms.up &&
			rlight->material		== light->lightShader &&
			rlight->prelightModel	== light->parms.prelightModel )
		{
			justUpdate = true;
		}
		else {
			// if we are updating shadows, the prelight model is no longer valid
			light->lightHasMoved = true;
			light->FreeDerivedData();
		}
	}
	else {
		// create a new one
		light = new( TAG_RENDER_LIGHT ) idRenderLightLocal;
		lightDefs[ lightHandle ] = light;

		light->world = this;
		light->index = lightHandle;
	}

	light->parms = *rlight; // copy parms

	light->lastModifiedFrameNum = tr.GetFrameCount();
	if( common->WriteDemo() && light->archived )
	{
		WriteFreeLight( lightHandle );
		light->archived = false;
	}

	// new for BFG edition: force noShadows on spectrum lights so teleport spawns
	// don't cause such a slowdown.  Hell writing shouldn't be shadowed anyway...
	if( light->parms.material && light->parms.material->Spectrum() )
	{
		light->parms.noShadows = true;
	}

	if( light->lightHasMoved )
	{
		light->parms.prelightModel = nullptr;
	}

	if( !justUpdate )
	{
		light->CreateReferences();
	}
}

/*
====================
FreeLightDef

	Frees all references and lit surfaces from the light, and
	NULL's out it's entry in the world list.
====================
*/
void idRenderWorldLocal::FreeLightDef( qhandle_t lightHandle )
{
	if( lightHandle < 0 || lightHandle >= lightDefs.Num() )
	{
		common->Printf( "idRenderWorld::FreeLightDef: invalid handle %i [0, %i]\n", lightHandle, lightDefs.Num() );
		return;
	}

	auto light = lightDefs[ lightHandle ];
	if( !light )
	{
		common->Printf( "idRenderWorld::FreeLightDef: handle %i is NULL\n", lightHandle );
		return;
	}

	light->FreeDerivedData();


	if( common->WriteDemo() && light->archived )
	{
		WriteFreeLight( lightHandle );
	}

	delete light;
	lightDefs[ lightHandle ] = nullptr;
}

/*
==================
GetRenderLight
==================
*/
const renderLightParms_t* idRenderWorldLocal::GetRenderLightParms( qhandle_t lightHandle ) const
{
	if( lightHandle < 0 || lightHandle >= lightDefs.Num() )
	{
		common->Printf( "idRenderWorld::GetRenderLightParms: handle %i > %i\n", lightHandle, lightDefs.Num() );
		return nullptr;
	}

	auto def = lightDefs[ lightHandle ];
	if( !def ) {
		common->Printf( "idRenderWorld::GetRenderLightParms: handle %i is NULL\n", lightHandle );
		return nullptr;
	}

	return &def->parms;
}

/*
================
idRenderWorldLocal::ProjectDecalOntoWorld
================
*/
void idRenderWorldLocal::ProjectDecalOntoWorld( const idFixedWinding& winding, const idVec3& projectionOrigin,
												const bool parallel, const float fadeDepth, const idMaterial* material, const int startTime )
{
	decalProjectionParms_t globalParms;
	if( !idRenderModelDecal::CreateProjectionParms( globalParms, winding, projectionOrigin, parallel, fadeDepth, material, startTime ) )
		return;

	// get the world areas touched by the projection volume
	int areas[10];
	int numAreas = BoundsInAreas( globalParms.projectionBounds, areas, 10 );

	// check all areas for models
	for( int i = 0; i < numAreas; ++i )
	{
		const auto * const area = &portalAreas[ areas[i] ];

		// check all models in this area
		for( const auto * ref = area->entityRefs.areaNext; ref != &area->entityRefs; ref = ref->areaNext )
		{
			auto def = ref->entity;

			if( def->parms.noOverlays )
				continue;

			if( def->parms.customMaterial != NULL && !def->parms.customMaterial->AllowOverlays() )
				continue;

			// completely ignore any dynamic or callback models
			const idRenderModel* model = def->GetModel();
			if( def->parms.callback != NULL || model == NULL || model->IsDynamicModel() != DM_STATIC )
				continue;

			idBounds bounds;
			bounds.FromTransformedBounds( model->Bounds( &def->parms ), def->GetOrigin(), def->GetAxis() );

			// if the model bounds do not overlap with the projection bounds
			decalProjectionParms_t localParms;
			if( !globalParms.projectionBounds.IntersectsBounds( bounds ) )
				continue;

			// transform the bounding planes, fade planes and texture axis into local space
			idRenderModelDecal::GlobalProjectionParmsToLocal( localParms, globalParms, def->GetOrigin(), def->GetAxis() );
			localParms.force = ( def->parms.customMaterial != NULL );

			if( def->decals == NULL ) {
				def->decals = AllocDecal( def->GetIndex(), startTime );
			}
			def->decals->AddDeferredDecal( localParms );
			def->archived = false;
		}
	}
}

/*
====================
idRenderWorldLocal::ProjectDecal
====================
*/
void idRenderWorldLocal::ProjectDecal( qhandle_t entityHandle, const idFixedWinding& winding,
	const idVec3& projectionOrigin, const bool parallel, const float fadeDepth, const idMaterial* material, const int startTime )
{
	if( entityHandle < 0 || entityHandle >= entityDefs.Num() )
	{
		common->Error( "idRenderWorld::ProjectOverlay: index = %i", entityHandle );
		return;
	}

	auto def = entityDefs[ entityHandle ];
	if( def == NULL )
		return;

	const idRenderModel* model = def->GetModel();

	if( model == NULL || model->IsDynamicModel() != DM_STATIC || def->parms.callback != NULL )
		return;

	decalProjectionParms_t globalParms;
	if( !idRenderModelDecal::CreateProjectionParms( globalParms, winding, projectionOrigin, parallel, fadeDepth, material, startTime ) )
		return;

	idBounds bounds;
	bounds.FromTransformedBounds( model->Bounds( &def->parms ), def->GetOrigin(), def->GetAxis() );

	// if the model bounds do not overlap with the projection bounds
	if( !globalParms.projectionBounds.IntersectsBounds( bounds ) )
		return;

	// transform the bounding planes, fade planes and texture axis into local space
	decalProjectionParms_t localParms;
	idRenderModelDecal::GlobalProjectionParmsToLocal( localParms, globalParms, def->GetOrigin(), def->GetAxis() );
	localParms.force = ( def->parms.customMaterial != NULL );

	if( def->decals == NULL )
	{
		def->decals = AllocDecal( def->GetIndex(), startTime );
	}
	def->decals->AddDeferredDecal( localParms );
	def->archived = false;
}

/*
====================
idRenderWorldLocal::ProjectOverlay
====================
*/
void idRenderWorldLocal::ProjectOverlay( qhandle_t entityHandle, const idPlane localTextureAxis[2], const idMaterial* material, const int startTime )
{
	if( entityHandle < 0 || entityHandle >= entityDefs.Num() )
	{
		common->Error( "idRenderWorld::ProjectOverlay: index = %i", entityHandle );
		return;
	}

	auto def = entityDefs[ entityHandle ];
	if( def == NULL )
		return;

	if( def->GetModel()->IsDynamicModel() != DM_CACHED )  	// FIXME: probably should be MD5 only
		return;

	overlayProjectionParms_t localParms;
	localParms.localTextureAxis[0] = localTextureAxis[0];
	localParms.localTextureAxis[1] = localTextureAxis[1];
	localParms.material = material;
	localParms.startTime = startTime;

	if( def->overlays == NULL )
	{
		def->overlays = AllocOverlay( def->GetIndex(), startTime );
	}
	def->overlays->AddDeferredOverlay( localParms );
	def->archived = false;
}

/*
====================
idRenderWorldLocal::AllocDecal
====================
*/
idRenderModelDecal* idRenderWorldLocal::AllocDecal( qhandle_t newEntityHandle, int startTime )
{
	int oldest = 0;
	int oldestTime = MAX_TYPE( oldestTime );
	for( int i = 0; i < decals.Num(); ++i )
	{
		if( decals[i].lastStartTime < oldestTime )
		{
			oldestTime = decals[i].lastStartTime;
			oldest = i;
		}
	}

	// remove any reference another model may still have to this decal
	if( decals[oldest].entityHandle >= 0 && decals[oldest].entityHandle < entityDefs.Num() )
	{
		auto def = entityDefs[decals[oldest].entityHandle];
		if( def != NULL && def->decals == decals[oldest].decals )
		{
			def->decals = NULL;
		}
	}

	decals[ oldest ].entityHandle = newEntityHandle;
	decals[ oldest ].lastStartTime = startTime;
	decals[ oldest ].decals->ReUse();
	if( common->WriteDemo() )
	{
		WriteFreeDecal( common->WriteDemo(), oldest );
	}
	return decals[oldest].decals;
}

/*
====================
idRenderWorldLocal::AllocOverlay
====================
*/
idRenderModelOverlay* idRenderWorldLocal::AllocOverlay( qhandle_t newEntityHandle, int startTime )
{
	int oldest = 0;
	int oldestTime = MAX_TYPE( oldestTime );
	for( int i = 0; i < overlays.Num(); ++i )
	{
		if( overlays[ i ].lastStartTime < oldestTime )
		{
			oldestTime = overlays[ i ].lastStartTime;
			oldest = i;
		}
	}

	// remove any reference another model may still have to this overlay
	if( overlays[ oldest ].entityHandle >= 0 && overlays[ oldest ].entityHandle < entityDefs.Num() )
	{
		auto def = entityDefs[ overlays[ oldest ].entityHandle ];
		if( def != NULL && def->overlays == overlays[ oldest ].overlays )
		{
			def->overlays = NULL;
		}
	}

	overlays[ oldest ].entityHandle = newEntityHandle;
	overlays[ oldest ].lastStartTime = startTime;
	overlays[ oldest ].overlays->ReUse();

	return overlays[ oldest ].overlays;
}

/*
====================
idRenderWorldLocal::RemoveDecals
====================
*/
void idRenderWorldLocal::RemoveDecals( qhandle_t entityHandle )
{
	if( entityHandle < 0 || entityHandle >= entityDefs.Num() )
	{
		common->Error( "idRenderWorld::ProjectOverlay: index = %i", entityHandle );
		return;
	}

	auto def = entityDefs[ entityHandle ];
	if( !def )
		return;

	def->FreeDecals();
	def->FreeOverlay();
}

/*
====================
idRenderWorldLocal::SetRenderView

	Sets the current view so any calls to the render world will use the correct parms.
====================
*/
void idRenderWorldLocal::SetRenderView( const renderViewParms_t* renderView )
{
	tr.primaryRenderView = *renderView;
}

/*
====================
idRenderWorldLocal::RenderScene

	Draw a 3D view into a part of the window, then return
	to 2D drawing.

	Rendering a scene may require multiple views to be rendered
	to handle mirrors,
====================
*/
void idRenderWorldLocal::RenderScene( const renderViewParms_t* renderViewParms )
{
	if( !tr.IsRenderDeviceRunning() )
		return;

	// skip front end rendering work, which will result
	// in only gui drawing
	if( r_skipFrontEnd.GetBool() )
		return;

	SCOPED_PROFILE_EVENT( "RenderWorld::RenderScene" );

	if( renderViewParms->fov_x <= 0 || renderViewParms->fov_y <= 0 )
	{
		common->Error( "idRenderWorld::RenderScene: bad FOVs: %f, %f", renderViewParms->fov_x, renderViewParms->fov_y );
	}

	// close any gui drawing
	tr.guiModel->EmitFullScreen();
	tr.guiModel->Clear();

	int startTime = sys->Microseconds();

	// setup view parms for the initial view
	auto view = allocManager.FrameAlloc<idRenderView, FRAME_ALLOC_VIEW_DEF, true>();

	view->parms = *renderViewParms;

	view->parms.forceUpdate = tr.takingScreenshot;

	int windowWidth = tr.GetWidth();
	int windowHeight = tr.GetHeight();
	tr.PerformResolutionScaling( windowWidth, windowHeight );

	// screenFraction is just for quickly testing fill rate limitations
	if( r_screenFraction.GetInteger() != 100 )
	{
		windowWidth = ( windowWidth * r_screenFraction.GetInteger() ) / 100;
		windowHeight = ( windowHeight * r_screenFraction.GetInteger() ) / 100;
	}
	tr.CropRenderSize( windowWidth, windowHeight );

	tr.GetCroppedViewport( &view->viewport );

	// the scissor bounds may be shrunk in subviews even if
	// the viewport stays the same
	// this scissor range is local inside the viewport
	view->scissor.x1 = 0;
	view->scissor.y1 = 0;
	view->scissor.x2 = view->viewport.x2 - view->viewport.x1;
	view->scissor.y2 = view->viewport.y2 - view->viewport.y1;

	view->isSubview = false;
	view->initialViewAreaOrigin = renderViewParms->vieworg;
	view->renderWorld = this;

	// see if the view needs to reverse the culling sense in mirrors
	// or environment cube sides
	idVec3 cross = view->GetAxis()[1].Cross( view->GetAxis()[2] );
	view->isMirror = !( cross * view->GetAxis()[ 0 ] > 0 );

	// save this world for use by some console commands
	tr.primaryWorld = this;
	tr.primaryRenderView = *renderViewParms;
	tr.primaryView = view;

	// rendering this view may cause other views to be rendered
	// for mirrors / portals / shadows / environment maps
	// this will also cause any necessary entities and lights to be
	// updated to the demo file
	R_RenderView( view );

	// render any post processing after the view and all its subviews has been draw
	R_RenderPostProcess( view );

	// now write delete commands for any modified-but-not-visible entities, and
	// add the renderView command to the demo
	if( common->WriteDemo() )
	{
		WriteRenderView( renderViewParms );
	}

	tr.UnCrop();

	int endTime = sys->Microseconds();

	tr.pc.frontEndMicroSec += endTime - startTime;

	// prepare for any 2D drawing after this
	tr.guiModel->Clear();
}

/*void CreateRenderView( idRenderWorldLocal *world, const renderViewParms_t & viewParms,
	const idScreenRect & renderViewport, const idScreenRect & renderScissor, bool forceUpdate )
{
	idRenderView* view = ( idRenderView* )R_ClearedFrameAlloc( sizeof( *view ), FRAME_ALLOC_VIEW_DEF );

	view->parms = viewParms;
	view->parms.forceUpdate = forceUpdate;

	view->viewport = renderViewport;
	view->scissor = renderScissor;

	view->isSubview = false;
	view->initialViewAreaOrigin = viewParms.vieworg;
	view->renderWorld = world;

	// see if the view needs to reverse the culling sense in mirrors
	// or environment cube sides
	idVec3 cross = view->GetAxis()[ 1 ].Cross( view->GetAxis()[ 2 ] );
	view->isMirror = !( cross * view->GetAxis()[ 0 ] > 0 );
}*/

/*
===================
idRenderWorldLocal::NumAreas
===================
*/
int idRenderWorldLocal::NumAreas() const
{
	return numPortalAreas;
}

/*
===================
idRenderWorldLocal::NumPortalsInArea
===================
*/
int idRenderWorldLocal::NumPortalsInArea( int areaNum )
{
	if( areaNum >= numPortalAreas || areaNum < 0 )
	{
		common->Error( "idRenderWorld::NumPortalsInArea: bad areanum %i", areaNum );
	}
	auto area = &portalAreas[areaNum];

	int count = 0;
	for( auto portal = area->portals; portal; portal = portal->next )
	{
		++count;
	}
	return count;
}

/*
===================
idRenderWorldLocal::GetPortal
===================
*/
exitPortal_t idRenderWorldLocal::GetPortal( int areaNum, int portalNum )
{
	exitPortal_t ret;

	if( areaNum > numPortalAreas )
	{
		common->Error( "idRenderWorld::GetPortal: areaNum > numAreas" );
	}
	auto area = &portalAreas[ areaNum ];

	int count = 0;
	for( auto portal = area->portals; portal; portal = portal->next )
	{
		if( count == portalNum )
		{
			ret.areas[0] = areaNum;
			ret.areas[1] = portal->intoArea;
			ret.w = portal->w;
			ret.blockingBits = portal->doublePortal->blockingBits;
			ret.portalHandle = portal->doublePortal - doublePortals + 1;
			return ret;
		}
		++count;
	}

	common->Error( "idRenderWorld::GetPortal: portalNum > numPortals" );

	memset( &ret, 0, sizeof( ret ) );
	return ret;
}

/*
===============
idRenderWorldLocal::PointInAreaNum

Will return -1 if the point is not in an area, otherwise
it will return 0 <= value < tr.world->numPortalAreas
===============
*/
int idRenderWorldLocal::PointInArea( const idVec3& point ) const
{
	areaNode_t* node = areaNodes;
	if( !node )
	{
		return -1;
	}

	while( 1 )
	{
		float d = point * node->plane.Normal() + node->plane[ 3 ];
		int nodeNum = ( d > 0 )? node->children[ 0 ] : node->children[ 1 ];
		if( nodeNum == 0 )
		{
			return -1;		// in solid
		}
		if( nodeNum < 0 )
		{
			nodeNum = -1 - nodeNum;
			if( nodeNum >= numPortalAreas )
			{
				common->Error( "idRenderWorld::PointInArea: area out of range" );
			}
			return nodeNum;
		}
		node = areaNodes + nodeNum;
	}

	return -1;
}

/*
===================
idRenderWorldLocal::BoundsInAreas_r
===================
*/
void idRenderWorldLocal::BoundsInAreas_r( int nodeNum, const idBounds& bounds, int* areas, int* numAreas, int maxAreas ) const
{
	do {
		if( nodeNum < 0 )
		{
			nodeNum = -1 - nodeNum;

			int i;
			for( i = 0; i < ( *numAreas ); ++i )
			{
				if( areas[i] == nodeNum )
				{
					break;
				}
			}
			if( i >= ( *numAreas ) && ( *numAreas ) < maxAreas )
			{
				areas[( *numAreas )++] = nodeNum;
			}
			return;
		}

		auto node = areaNodes + nodeNum;

		int side = bounds.PlaneSide( node->plane );
		if( side == PLANESIDE_FRONT )
		{
			nodeNum = node->children[0];
		}
		else if( side == PLANESIDE_BACK )
		{
			nodeNum = node->children[1];
		}
		else {
			if( node->children[1] != 0 )
			{
				BoundsInAreas_r( node->children[1], bounds, areas, numAreas, maxAreas );
				if( ( *numAreas ) >= maxAreas )
				{
					return;
				}
			}
			nodeNum = node->children[0];
		}
	}
	while( nodeNum != 0 );

	return;
}

/*
===================
idRenderWorldLocal::BoundsInAreas

  fills the *areas array with the number of the areas the bounds are in
  returns the total number of areas the bounds are in
===================
*/
int idRenderWorldLocal::BoundsInAreas( const idBounds& bounds, int* areas, int maxAreas ) const
{
	int numAreas = 0;

	assert( areas );
	assert( bounds[0][0] <= bounds[1][0] && bounds[0][1] <= bounds[1][1] && bounds[0][2] <= bounds[1][2] );
	assert( bounds[1][0] - bounds[0][0] < 1e4f && bounds[1][1] - bounds[0][1] < 1e4f && bounds[1][2] - bounds[0][2] < 1e4f );

	if( !areaNodes )
	{
		return numAreas;
	}
	BoundsInAreas_r( 0, bounds, areas, &numAreas, maxAreas );
	return numAreas;
}

/*
================
idRenderWorldLocal::GuiTrace

checks a ray trace against any gui surfaces in an entity, returning the
fraction location of the trace on the gui surface, or -1,-1 if no hit.
this doesn't do any occlusion testing, simply ignoring non-gui surfaces.
start / end are in global world coordinates.
================
*/
guiPoint_t idRenderWorldLocal::GuiTrace( qhandle_t entityHandle, const idVec3 start, const idVec3 end ) const
{
	guiPoint_t	pt;
	pt.x = pt.y = -1;
	pt.guiId = 0;

	if( ( entityHandle < 0 ) || ( entityHandle >= entityDefs.Num() ) )
	{
		common->Printf( "idRenderWorld::GuiTrace: invalid handle %i\n", entityHandle );
		return pt;
	}

	auto def = entityDefs[ entityHandle ];
	if( !def )
	{
		common->Printf( "idRenderWorld::GuiTrace: handle %i is NULL\n", entityHandle );
		return pt;
	}

	auto model = def->GetModel();
	if( !model || model->IsDynamicModel() != DM_STATIC || def->parms.callback != NULL )
		return pt;

	// transform the points into local space
	idVec3 localStart, localEnd;
	def->modelRenderMatrix.InverseTransformPoint( start, localStart );
	def->modelRenderMatrix.InverseTransformPoint( end, localEnd );

	for( int i = 0; i < model->NumSurfaces(); ++i )
	{
		auto surf = model->Surface( i );

		if( !surf->GetTriangles() )
			continue;

		auto material = R_RemapShaderBySkin( surf->GetMaterial(), def->parms.customSkin, def->parms.customMaterial );
		if( !material )
			continue;

		// only trace against gui surfaces
		if( !material->HasGui() )
			continue;

		auto local = surf->GetTriangles()->LocalTrace( localStart, localEnd, 0.0f );
		if( local.fraction < 1.0f )
		{
			idVec3 origin;
			idMat3 axis;

			R_SurfaceToTextureAxis( surf->GetTriangles(), origin, axis );
			const idVec3 cursor = local.point - origin;

			float axisLen[2];
			axisLen[0] = axis[0].Length();
			axisLen[1] = axis[1].Length();

			pt.x = ( cursor * axis[0] ) / ( axisLen[0] * axisLen[0] );
			pt.y = ( cursor * axis[1] ) / ( axisLen[1] * axisLen[1] );
			pt.guiId = material->GetEntityGui();

			return pt;
		}
	}

	return pt;
}

/*
===================
idRenderWorldLocal::ModelTrace
===================
*/
bool idRenderWorldLocal::ModelTrace( modelTrace_t& trace, qhandle_t entityHandle, const idVec3& start, const idVec3& end, const float radius ) const
{
	memset( &trace, 0, sizeof( trace ) );
	trace.fraction = 1.0f;
	trace.point = end;

	if( entityHandle < 0 || entityHandle >= entityDefs.Num() )
		return false;

	auto def = entityDefs[ entityHandle ];
	if( !def )
		return false;

	auto model = def->EmitDynamicModel();
	if( !model )
		return false;

	// transform the points into local space
	idVec3 localStart, localEnd;

	idRenderMatrix modelMatrix;
	idRenderMatrix::CreateFromOriginAxis( def->GetOrigin(), def->GetAxis(), modelMatrix );

	modelMatrix.InverseTransformPoint( start, localStart );
	modelMatrix.InverseTransformPoint( end, localEnd );

	// if we have explicit collision surfaces, only collide against them
	// (FIXME, should probably have a parm to control this)
	bool collisionSurface = false;
	for( int i = 0; i < model->NumBaseSurfaces(); ++i )
	{
		auto surf = model->Surface( i );

		auto material = R_RemapShaderBySkin( surf->GetMaterial(), def->parms.customSkin, def->parms.customMaterial );

		if( material->GetSurfaceFlags() & SURF_COLLISION )
		{
			collisionSurface = true;
			break;
		}
	}

	// only use baseSurfaces, not any overlays
	for( int i = 0; i < model->NumBaseSurfaces(); i++ )
	{
		auto surf = model->Surface( i );

		auto material = R_RemapShaderBySkin( surf->GetMaterial(), def->parms.customSkin, def->parms.customMaterial );

		if( !surf->GetMaterial() || !material )
			continue;

		if( collisionSurface )
		{
			// only trace vs collision surfaces
			if( ( material->GetSurfaceFlags() & SURF_COLLISION ) == 0 )
				continue;
		}
		else {
			// skip if not drawn or translucent
			if( !material->IsDrawn() || ( material->Coverage() != MC_OPAQUE && material->Coverage() != MC_PERFORATED ) )
				continue;
		}

		auto localTrace = surf->GetTriangles()->LocalTrace( localStart, localEnd, radius );
		if( localTrace.fraction < trace.fraction )
		{
			trace.fraction = localTrace.fraction;
			modelMatrix.TransformPoint( localTrace.point, trace.point );
			trace.normal = localTrace.normal * def->GetAxis();
			trace.material = material;
			trace.entity = &def->parms;
			trace.jointNumber = def->GetModel()->NearestJoint( i, localTrace.indexes[0], localTrace.indexes[1], localTrace.indexes[2] );
		}
	}

	return ( trace.fraction < 1.0f );
}

/*
===================
idRenderWorldLocal::Trace
===================
*/
// FIXME: _D3XP added those.
const char* playerModelExcludeList[] =
{
	"models/md5/characters/player/d3xp_spplayer.md5mesh",
	"models/md5/characters/player/head/d3xp_head.md5mesh",
	"models/md5/weapons/pistol_world/worldpistol.md5mesh",
	NULL
};

const char* playerMaterialExcludeList[] =
{
	"muzzlesmokepuff",
	NULL
};

bool idRenderWorldLocal::Trace( modelTrace_t& trace, const idVec3& start, const idVec3& end, const float radius, bool skipDynamic, bool skipPlayer /*_D3XP*/ ) const
{
	trace.fraction = 1.0f;
	trace.point = end;

	// bounds for the whole trace
	idBounds traceBounds;
	traceBounds.Clear();
	traceBounds.AddPoint( start );
	traceBounds.AddPoint( end );

	// get the world areas the trace is in
	int areas[128];
	int numAreas = BoundsInAreas( traceBounds, areas, 128 );

	int numSurfaces = 0;

	// check all areas for models
	for( int i = 0; i < numAreas; ++i )
	{
		auto area = &portalAreas[ areas[ i ] ];

		// check all models in this area
		for( auto ref = area->entityRefs.areaNext; ref != &area->entityRefs; ref = ref->areaNext )
		{
			auto def = ref->entity;

			auto model = def->GetModel();
			if( !model )
				continue;

			if( model->IsDynamicModel() != DM_STATIC )
			{
				if( skipDynamic )
					continue;

			#if 1	/* _D3XP addition. could use a cleaner approach */
				if( skipPlayer )
				{
					bool exclude = false;
					for( int k = 0; playerModelExcludeList[k] != NULL; k++ )
					{
						if( idStr::Cmp( model->Name(), playerModelExcludeList[k] ) == 0 )
						{
							exclude = true;
							break;
						}
					}
					if( exclude )
						continue;
				}
			#endif
				model = def->EmitDynamicModel();
				if( !model )
					continue;	// can happen with particle systems, which don't instantiate without a valid view
			}

			idBounds bounds;
			bounds.FromTransformedBounds( model->Bounds( &def->parms ), def->GetOrigin(), def->GetAxis() );

			// if the model bounds do not overlap with the trace bounds
			if( !traceBounds.IntersectsBounds( bounds ) || !bounds.LineIntersection( start, trace.point ) )
				continue;

			// check all model surfaces
			for( int j = 0; j < model->NumSurfaces(); ++j )
			{
				auto surf = model->Surface( j );

				auto material = R_RemapShaderBySkin( surf->GetMaterial(), def->parms.customSkin, def->parms.customMaterial );

				// if no geometry or no shader
				if( !surf->GetTriangles() || !material )
					continue;

			#if 1 /* _D3XP addition. could use a cleaner approach */
				if( skipPlayer )
				{
					bool exclude = false;
					for( int k = 0; playerMaterialExcludeList[k] != NULL; ++k )
					{
						if( idStr::Cmp( material->GetName(), playerMaterialExcludeList[k] ) == 0 )
						{
							exclude = true;
							break;
						}
					}
					if( exclude )
						continue;
				}
			#endif
				bounds.FromTransformedBounds( surf->GetTriangles()->GetBounds(), def->GetOrigin(), def->GetAxis() );

				// if triangle bounds do not overlap with the trace bounds
				if( !traceBounds.IntersectsBounds( bounds ) || !bounds.LineIntersection( start, trace.point ) )
				{
					continue;
				}

				numSurfaces++;

				// transform the points into local space
				idVec3 localStart, localEnd;

				idRenderMatrix modelMatrix;
				idRenderMatrix::CreateFromOriginAxis( def->GetOrigin(), def->GetAxis(), modelMatrix );

				modelMatrix.InverseTransformPoint( start, localStart );
				modelMatrix.InverseTransformPoint( end, localEnd );

				auto localTrace = surf->GetTriangles()->LocalTrace( localStart, localEnd, radius );
				if( localTrace.fraction < trace.fraction )
				{
					trace.fraction = localTrace.fraction;
					modelMatrix.TransformPoint( localTrace.point, trace.point );
					trace.normal = localTrace.normal * def->GetAxis();
					trace.material = material;
					trace.entity = &def->parms;
					trace.jointNumber = model->NearestJoint( j, localTrace.indexes[0], localTrace.indexes[1], localTrace.indexes[2] );

					traceBounds.Clear();
					traceBounds.AddPoint( start );
					traceBounds.AddPoint( start + trace.fraction * ( end - start ) );
				}
			}
		}
	}
	return( trace.fraction < 1.0f );
}

/*
==================
idRenderWorldLocal::RecurseProcBSP
==================
*/
void idRenderWorldLocal::RecurseProcBSP_r( modelTrace_t* results, int parentNodeNum, int nodeNum, float p1f, float p2f, const idVec3& p1, const idVec3& p2 ) const
{
	float		t1, t2;
	float		frac;
	idVec3		mid;
	int			side;
	float		midf;
	areaNode_t* node;

	if( results->fraction <= p1f )
	{
		return;		// already hit something nearer
	}
	// empty leaf
	if( nodeNum < 0 )
	{
		return;
	}
	// if solid leaf node
	if( nodeNum == 0 )
	{
		if( parentNodeNum != -1 )
		{

			results->fraction = p1f;
			results->point = p1;
			node = &areaNodes[parentNodeNum];
			results->normal = node->plane.Normal();
			return;
		}
	}
	node = &areaNodes[nodeNum];

	// distance from plane for trace start and end
	t1 = node->plane.Normal() * p1 + node->plane[3];
	t2 = node->plane.Normal() * p2 + node->plane[3];

	if( t1 >= 0.0f && t2 >= 0.0f )
	{
		RecurseProcBSP_r( results, nodeNum, node->children[0], p1f, p2f, p1, p2 );
		return;
	}
	if( t1 < 0.0f && t2 < 0.0f )
	{
		RecurseProcBSP_r( results, nodeNum, node->children[1], p1f, p2f, p1, p2 );
		return;
	}
	side = t1 < t2;
	frac = t1 / ( t1 - t2 );
	midf = p1f + frac * ( p2f - p1f );
	mid[0] = p1[0] + frac * ( p2[0] - p1[0] );
	mid[1] = p1[1] + frac * ( p2[1] - p1[1] );
	mid[2] = p1[2] + frac * ( p2[2] - p1[2] );
	RecurseProcBSP_r( results, nodeNum, node->children[side], p1f, midf, p1, mid );
	RecurseProcBSP_r( results, nodeNum, node->children[side ^ 1], midf, p2f, mid, p2 );
}

/*
==================
idRenderWorldLocal::FastWorldTrace
==================
*/
bool idRenderWorldLocal::FastWorldTrace( modelTrace_t& results, const idVec3& start, const idVec3& end ) const
{
	memset( &results, 0, sizeof( modelTrace_t ) );
	results.fraction = 1.0f;
	if( areaNodes != NULL )
	{
		RecurseProcBSP_r( &results, -1, 0, 0.0f, 1.0f, start, end );
		return ( results.fraction < 1.0f );
	}
	return false;
}

/*
=================================================================================

	CREATE MODEL REFS

=================================================================================
*/

/*
=================
idRenderWorldLocal::AddEntityRefToArea

This is called by R_PushVolumeIntoTree and also directly
for the world model references that are precalculated.
=================
*/
void idRenderWorldLocal::AddEntityRefToArea( idRenderEntityLocal* def, portalArea_t* area )
{
	if( def == NULL )
	{
		common->Error( "idRenderWorldLocal::AddEntityRefToArea: NULL def" );
		return;
	}

	for( auto ref = def->entityRefs; ref != NULL; ref = ref->ownerNext )
	{
		if( ref->area == area )
			return;
	}

	auto ref = areaReferenceAllocator.Alloc();
	tr.pc.c_entityReferences++;

	ref->entity = def;

	// link to entityDef
	ref->ownerNext = def->entityRefs;
	def->entityRefs = ref;

	// link to end of area list
	ref->area = area;
	ref->areaNext = &area->entityRefs;
	ref->areaPrev = area->entityRefs.areaPrev;
	ref->areaNext->areaPrev = ref;
	ref->areaPrev->areaNext = ref;
}

/*
===================
idRenderWorldLocal::AddLightRefToArea
===================
*/
void idRenderWorldLocal::AddLightRefToArea( idRenderLightLocal* light, portalArea_t* area )
{
	if( light == NULL )
	{
		common->Error( "idRenderWorldLocal::AddLightRefToArea: NULL light" );
		return;
	}

	for( auto lref = light->references; lref != NULL; lref = lref->ownerNext )
	{
		if( lref->area == area )
			return;
	}

	// add a lightref to this area
	auto lref = areaReferenceAllocator.Alloc();
	tr.pc.c_lightReferences++;

	lref->light = light;

	lref->ownerNext = light->references;
	light->references = lref;

	// doubly linked list so we can free them easily later
	lref->area = area;
	area->lightRefs.areaNext->areaPrev = lref;
	lref->areaNext = area->lightRefs.areaNext;
	lref->areaPrev = &area->lightRefs;
	area->lightRefs.areaNext = lref;
}

/*
===================
idRenderWorldLocal::GenerateAllInteractions

Force the generation of all light / surface interactions at the start of a level
If this isn't called, they will all be dynamically generated
===================
*/
void idRenderWorldLocal::GenerateAllInteractions()
{
	if( !R_IsInitialized() )
		return;

	int start = sys->Milliseconds();

	generateAllInteractionsCalled = false;

	// let the interaction creation code know that it shouldn't
	// try and do any view specific optimizations
	tr.viewDef = NULL;

	// build the interaction table
	// this will be dynamically resized if the entity / light counts grow too much
	interactionTableWidth = entityDefs.Num() + 100;
	interactionTableHeight = lightDefs.Num() + 100;
	int	size =  interactionTableWidth * interactionTableHeight * sizeof( *interactionTable );
	interactionTable = allocManager.StaticAlloc<idInteraction*, TAG_RENDER_INTERACTION, true>( interactionTableWidth * interactionTableHeight );

	// itterate through all lights
	int	count = 0;
	for( int i = 0; i < this->lightDefs.Num(); ++i )
	{
		auto ldef = this->lightDefs[ i ];
		if( !ldef )
			continue;

		// check all areas the light touches
		for( auto lref = ldef->references; lref; lref = lref->ownerNext )
		{
			auto area = lref->area;

			// check all the models in this area
			for( auto eref = area->entityRefs.areaNext; eref != &area->entityRefs; eref = eref->areaNext )
			{
				auto edef = eref->entity;

				// scan the doubly linked lists, which may have several dozen entries
				idInteraction*	inter;

				// we could check either model refs or light refs for matches, but it is
				// assumed that there will be less lights in an area than models
				// so the entity chains should be somewhat shorter (they tend to be fairly close).
				for( inter = edef->firstInteraction; inter != NULL; inter = inter->entityNext )
				{
					if( inter->lightDef == ldef )
					{
						break;
					}
				}

				// if we already have an interaction, we don't need to do anything
				if( inter != NULL )
					continue;

				// make an interaction for this light / entity pair
				// and add a pointer to it in the table
				inter = idInteraction::AllocAndLink( edef, ldef );
				++count;

				// the interaction may create geometry
				inter->CreateStaticInteraction();
			}
		}

		session->Pump();
	}

	int end = sys->Milliseconds();
	int	msec = end - start;

	common->Printf( "idRenderWorld::GenerateAllInteractions, msec = %i\n", msec );
	common->Printf( "interactionTable size: %i bytes\n", size );
	common->Printf( "%i interactions take %i bytes\n", count, count * sizeof( idInteraction ) );

	// entities flagged as noDynamicInteractions will no longer make any
	generateAllInteractionsCalled = true;
}

/*
===================
idRenderWorldLocal::FreeInteractions
===================
*/
void idRenderWorldLocal::FreeInteractions()
{
	for( int i = 0; i < entityDefs.Num(); ++i )
	{
		auto def = entityDefs[ i ];
		if( !def )
			continue;

		// free all the interactions
		while( def->firstInteraction != NULL )
		{
			def->firstInteraction->UnlinkAndFree();
		}
	}
}

/*
==================
idRenderWorldLocal::PushFrustumIntoTree_r

Used for both light volumes and model volumes.

This does not clip the points by the planes, so some slop
occurs.

tr.viewCount should be bumped before calling, allowing it
to prevent double checking areas.

We might alternatively choose to do this with an area flow.
==================
*/
void idRenderWorldLocal::PushFrustumIntoTree_r( idRenderEntityLocal* def, idRenderLightLocal* light, const idRenderMatrix::frustumCorners_t& corners, int nodeNum )
{
	if( nodeNum < 0 )
	{
		int areaNum = -1 - nodeNum;
		auto area = &portalAreas[ areaNum ];
		if( area->viewCount == tr.GetViewCount() )
		{
			return;	// already added a reference here
		}
		area->viewCount = tr.GetViewCount();

		if( def != NULL )
		{
			AddEntityRefToArea( def, area );
		}
		if( light != NULL )
		{
			AddLightRefToArea( light, area );
		}

		return;
	}

	areaNode_t* node = areaNodes + nodeNum;

	// if we know that all possible children nodes only touch an area
	// we have already marked, we can early out
	if( node->commonChildrenArea != CHILDREN_HAVE_MULTIPLE_AREAS && r_useNodeCommonChildren.GetBool() )
	{
		// note that we do NOT try to set a reference in this area
		// yet, because the test volume may yet wind up being in the
		// solid part, which would cause bounds slightly poked into
		// a wall to show up in the next room
		if( portalAreas[ node->commonChildrenArea ].viewCount == tr.GetViewCount() )
		{
			return;
		}
	}

	// exact check all the corners against the node plane
	auto cull = idRenderMatrix::CullFrustumCornersToPlane( corners, node->plane );

	if( cull != idRenderMatrix::FRUSTUM_CULL_BACK )
	{
		nodeNum = node->children[0];
		if( nodeNum != 0 )  	// 0 = solid
		{
			PushFrustumIntoTree_r( def, light, corners, nodeNum );
		}
	}

	if( cull != idRenderMatrix::FRUSTUM_CULL_FRONT )
	{
		nodeNum = node->children[1];
		if( nodeNum != 0 )  	// 0 = solid
		{
			PushFrustumIntoTree_r( def, light, corners, nodeNum );
		}
	}
}

/*
==============
idRenderWorldLocal::PushFrustumIntoTree
==============
*/
void idRenderWorldLocal::PushFrustumIntoTree( idRenderEntityLocal* def, idRenderLightLocal* light, const idRenderMatrix& frustumTransform, const idBounds& frustumBounds )
{
	if( areaNodes == NULL )
		return;

	// calculate the corners of the frustum in word space
	idRenderMatrix::frustumCorners_t corners;
	idRenderMatrix::GetFrustumCorners( corners, frustumTransform, frustumBounds );

	PushFrustumIntoTree_r( def, light, corners, 0 );
}

//===================================================================

/*
================
idRenderWorldLocal::DrawTextLength

  returns the length of the given text
================
*/
float idRenderWorldLocal::DrawTextLength( const char* text, float scale, int len )
{
	return RB_DrawTextLength( text, scale, len );
}

/*
================
idRenderWorldLocal::DrawText

  oriented on the viewaxis
  align can be 0-left, 1-center (default), 2-right
================
*/
void idRenderWorldLocal::DrawText( const char* text, const idVec3& origin, float scale, const idColor& color, const idMat3& viewAxis, const int align, const int lifetime, const bool depthTest )
{
	RB_AddDebugText( text, origin, scale, color.ToVec4(), viewAxis, align, lifetime, depthTest );
}

/*
===============
idRenderWorldLocal::RegenerateWorld
===============
*/
void idRenderWorldLocal::RegenerateWorld()
{
	R_FreeDerivedData();
	R_ReCreateWorldReferences();
}

/*
===============
R_GlobalShaderOverride
===============
*/
bool R_GlobalShaderOverride( const idMaterial** shader )
{
	if( !( *shader )->IsDrawn() )
	{
		return false;
	}

	if( tr.primaryRenderView.globalMaterial )
	{
		*shader = tr.primaryRenderView.globalMaterial;
		return true;
	}

	if( r_materialOverride.GetString()[0] != '\0' )
	{
		*shader = declManager->FindMaterial( r_materialOverride.GetString() );
		return true;
	}

	return false;
}

/*
===============
R_RemapShaderBySkin
===============
*/
const idMaterial* R_RemapShaderBySkin( const idMaterial* material, const idDeclSkin* skin, const idMaterial* customMaterial )
{
	if( !material )
	{
		return NULL;
	}

	// never remap surfaces that were originally nodraw, like collision hulls
	if( !material->IsDrawn() )
	{
		return material;
	}

	if( customMaterial )
	{
		// this is sort of a hack, but cause deformed surfaces to map to empty surfaces,
		// so the item highlight overlay doesn't highlight the autosprite surface
		if( material->GetDeformType() )
		{
			return NULL;
		}
		return const_cast<idMaterial*>( customMaterial );
	}

	if( !skin )
	{
		return const_cast<idMaterial*>( material );
	}

	return skin->RemapShaderBySkin( material );
}
