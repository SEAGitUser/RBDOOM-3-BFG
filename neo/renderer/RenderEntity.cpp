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

idRenderEntityLocal::idRenderEntityLocal()
{
	parms.Clear();
	world					= NULL;
	index					= 0;
	lastModifiedFrameNum	= 0;
	archived				= false;
	dynamicModel			= NULL;
	dynamicModelFrameCount	= 0;
	cachedDynamicModel		= NULL;
	localReferenceBounds	= bounds_zero;
	globalReferenceBounds	= bounds_zero;
	viewCount				= 0;
	viewModel				= NULL;
	decals					= NULL;
	overlays				= NULL;
	entityRefs				= NULL;
	firstInteraction		= NULL;
	lastInteraction			= NULL;
	needsPortalSky			= false;
}

/*
============================
 IsDirectlyVisible()
============================
*/
bool idRenderEntityLocal::IsDirectlyVisible() const
{
	if( viewCount != tr.GetViewCount() )
	{
		return false;
	}
	if( viewModel->scissorRect.IsEmpty() )
	{
		// a viewModel was created for shadow generation, but the
		// model global reference bounds isn't directly visible
		return false;
	}
	return true;
}

/*
=================
 R_DeriveEntityData
=================
*/
void idRenderEntityLocal::DeriveData()
{
	//prevModelRenderMatrix.Copy( modelRenderMatrix );

	idRenderMatrix::CreateFromOriginAxis( parms.origin, parms.axis, modelRenderMatrix );

	// calculate the matrix that transforms the unit cube to exactly cover the model in world space
	idRenderMatrix::OffsetScaleForBounds( modelRenderMatrix, localReferenceBounds, inverseBaseModelProject );

	// calculate the global model bounds by inverse projecting the unit cube with the 'inverseBaseModelProject'
	idRenderMatrix::ProjectedBounds( globalReferenceBounds, inverseBaseModelProject, bounds_unitCube, false );
}

/*
===================
 R_FreeEntityDefDerivedData

Used by both FreeEntityDef and UpdateEntityDef
Does not actually free the entityDef.
===================
*/
void idRenderEntityLocal::FreeDerivedData( bool keepDecals, bool keepCachedDynamicModel )
{
	// demo playback needs to free the joints, while normal play
	// leaves them in the control of the game
	if( common->ReadDemo() )
	{
		if( this->parms.joints )
		{
			Mem_Free16( this->parms.joints );
			this->parms.joints = NULL;
		}
		if( this->parms.callbackData )
		{
			Mem_Free( this->parms.callbackData );
			this->parms.callbackData = NULL;
		}
		for( int i = 0; i < MAX_RENDERENTITY_GUI; i++ )
		{
			if( this->parms.gui[ i ] )
			{
				delete this->parms.gui[ i ];
				this->parms.gui[ i ] = NULL;
			}
		}
	}

	// free all the interactions
	while( this->firstInteraction != NULL )
	{
		this->firstInteraction->UnlinkAndFree();
	}
	this->dynamicModelFrameCount = 0;

	// clear the dynamic model if present
	if( this->dynamicModel )
	{
		this->dynamicModel = NULL;
	}

	if( !keepDecals )
	{
		this->FreeDecals();
		this->FreeOverlay();
	}

	if( !keepCachedDynamicModel )
	{
		delete this->cachedDynamicModel;
		this->cachedDynamicModel = NULL;
	}

	// free the entityRefs from the areas
	areaReference_t* next = NULL;
	for( areaReference_t* ref = this->entityRefs; ref != NULL; ref = next )
	{
		next = ref->ownerNext;

		// unlink from the area
		ref->areaNext->areaPrev = ref->areaPrev;
		ref->areaPrev->areaNext = ref->areaNext;

		// put it back on the free list for reuse
		this->world->areaReferenceAllocator.Free( ref );
	}
	this->entityRefs = NULL;
}

/*
===================
 R_FreeEntityDefDecals
===================
*/
void idRenderEntityLocal::FreeDecals()
{
	decals = NULL;
}

/*
===================
 R_FreeEntityDefFadedDecals
===================
*/
void idRenderEntityLocal::FreeFadedDecals( int time )
{
	if( decals != NULL )
	{
		decals->RemoveFadedDecals( time );
	}
}

/*
===================
 R_FreeEntityDefOverlay
===================
*/
void idRenderEntityLocal::FreeOverlay()
{
	overlays = NULL;
}

/*
===============
 R_CreateEntityRefs		SEA: this shuold be part of the renderworld probably ?

	Creates all needed model references in portal areas,
	chaining them to both the area and the entityDef.

	Bumps tr.viewCount, which means viewCount can change many times each frame.
===============
*/
void idRenderEntityLocal::CreateReferences()
{
	idRenderEntityLocal* entity = this;

	if( entity->parms.hModel == NULL )
	{
		entity->parms.hModel = renderModelManager->DefaultModel();
	}

	// if the entity hasn't been fully specified due to expensive animation calcs
	// for md5 and particles, use the provided conservative bounds.
	if( entity->parms.callback != NULL )
	{
		entity->localReferenceBounds = entity->parms.bounds;
	}
	else {
		entity->localReferenceBounds = entity->parms.hModel->Bounds( &entity->parms );
	}

	// some models, like empty particles, may not need to be added at all
	if( entity->localReferenceBounds.IsCleared() )
	{
		return;
	}

	if( r_showUpdates.GetBool() &&
		( entity->localReferenceBounds[ 1 ][ 0 ] - entity->localReferenceBounds[ 0 ][ 0 ] > 1024.0f ||
		  entity->localReferenceBounds[ 1 ][ 1 ] - entity->localReferenceBounds[ 0 ][ 1 ] > 1024.0f ) )
	{
		common->Printf( "big entityRef: %f,%f\n",
			entity->localReferenceBounds[ 1 ][ 0 ] - entity->localReferenceBounds[ 0 ][ 0 ],
			entity->localReferenceBounds[ 1 ][ 1 ] - entity->localReferenceBounds[ 0 ][ 1 ] );
	}

	// derive entity data
	entity->DeriveData();

	// bump the view count so we can tell if an
	// area already has a reference
	tr.viewCount++;

	// push the model frustum down the BSP tree into areas
	entity->world->PushFrustumIntoTree( entity, NULL, entity->inverseBaseModelProject, bounds_unitCube );
}

/*
==================
 R_ClearEntityDefDynamicModel

	If we know the reference bounds stays the same, we
	only need to do this on entity update, not the full
	R_FreeEntityDefDerivedData
==================
*/
void idRenderEntityLocal::ClearDynamicModel()
{
	// free all the interaction surfaces
	for( idInteraction* inter = this->firstInteraction; inter != NULL && !inter->IsEmpty(); inter = inter->entityNext )
	{
		inter->FreeSurfaces();
	}

	// clear the dynamic model if present
	if( this->dynamicModel )
	{
		// this is copied from cachedDynamicModel, so it doesn't need to be freed
		this->dynamicModel = NULL;
	}
	this->dynamicModelFrameCount = 0;
}

/*
==================
 R_IssueEntityDefCallback
==================
*/
bool idRenderEntityLocal::IssueCallback()
{
	idBounds oldBounds = this->localReferenceBounds;

	this->archived = false;		// will need to be written to the demo file

	bool update;
	if( tr.viewDef != NULL )
	{
		update = this->parms.callback( &this->parms, &tr.viewDef->GetParms() );
	}
	else {
		update = this->parms.callback( &this->parms, NULL );
	}
	tr.pc.c_entityDefCallbacks++;

	if( this->parms.hModel == NULL )
	{
		common->Error( "R_IssueEntityDefCallback: dynamic entity callback didn't set model" );
	}

	if( r_checkBounds.GetBool() )
	{
		if( oldBounds[ 0 ][ 0 ] > this->localReferenceBounds[ 0 ][ 0 ] + CHECK_BOUNDS_EPSILON ||
			oldBounds[ 0 ][ 1 ] > this->localReferenceBounds[ 0 ][ 1 ] + CHECK_BOUNDS_EPSILON ||
			oldBounds[ 0 ][ 2 ] > this->localReferenceBounds[ 0 ][ 2 ] + CHECK_BOUNDS_EPSILON ||
			oldBounds[ 1 ][ 0 ] < this->localReferenceBounds[ 1 ][ 0 ] - CHECK_BOUNDS_EPSILON ||
			oldBounds[ 1 ][ 1 ] < this->localReferenceBounds[ 1 ][ 1 ] - CHECK_BOUNDS_EPSILON ||
			oldBounds[ 1 ][ 2 ] < this->localReferenceBounds[ 1 ][ 2 ] - CHECK_BOUNDS_EPSILON )
		{
			common->Printf( "entity %i callback extended reference bounds\n", this->GetIndex() );
		}
	}

	return update;
}

/*
===================
 R_EntityDefDynamicModel

	This is also called by the game code for idRenderWorldLocal::ModelTrace(),
	and idRenderWorldLocal::Trace() which is bad for performance...

	Issues a deferred entity callback if necessary.
	If the model isn't dynamic, it returns the original.
	Returns the cached dynamic model if present, otherwise creates it.
===================
*/
idRenderModel* idRenderEntityLocal::EmitDynamicModel()
{
	if( this->dynamicModelFrameCount == tr.GetFrameCount() )
	{
		return this->dynamicModel;
	}

	// allow deferred entities to construct themselves
	bool callbackUpdate;
	if( this->parms.callback != NULL )
	{
		SCOPED_PROFILE_EVENT( "R_IssueEntityDefCallback" );
		callbackUpdate = this->IssueCallback();
	}
	else {
		callbackUpdate = false;
	}

	idRenderModel* model = this->parms.hModel;

	if( model == NULL )
	{
		common->Error( "R_EntityDefDynamicModel: NULL model" );
		return NULL;
	}

	if( model->IsDynamicModel() == DM_STATIC )
	{
		this->dynamicModel = NULL;
		this->dynamicModelFrameCount = 0;
		return model;
	}

	// continously animating models (particle systems, etc) will have their snapshot updated every single view
	if( callbackUpdate || ( model->IsDynamicModel() == DM_CONTINUOUS && this->dynamicModelFrameCount != tr.GetFrameCount() ) )
	{
		this->ClearDynamicModel();
	}

	// if we don't have a snapshot of the dynamic model, generate it now
	if( this->dynamicModel == NULL )
	{
		SCOPED_PROFILE_EVENT( "InstantiateDynamicModel" );

		// instantiate the snapshot of the dynamic model, possibly reusing memory from the cached snapshot
		this->cachedDynamicModel = model->InstantiateDynamicModel( &this->parms, tr.viewDef, this->cachedDynamicModel );

		if( this->cachedDynamicModel != NULL && r_checkBounds.GetBool() )
		{
			idBounds b = this->cachedDynamicModel->Bounds();
			if( b[ 0 ][ 0 ] < this->localReferenceBounds[ 0 ][ 0 ] - CHECK_BOUNDS_EPSILON ||
				b[ 0 ][ 1 ] < this->localReferenceBounds[ 0 ][ 1 ] - CHECK_BOUNDS_EPSILON ||
				b[ 0 ][ 2 ] < this->localReferenceBounds[ 0 ][ 2 ] - CHECK_BOUNDS_EPSILON ||
				b[ 1 ][ 0 ] > this->localReferenceBounds[ 1 ][ 0 ] + CHECK_BOUNDS_EPSILON ||
				b[ 1 ][ 1 ] > this->localReferenceBounds[ 1 ][ 1 ] + CHECK_BOUNDS_EPSILON ||
				b[ 1 ][ 2 ] > this->localReferenceBounds[ 1 ][ 2 ] + CHECK_BOUNDS_EPSILON )
			{
				common->Printf( "entity %i dynamic model exceeded reference bounds\n", this->index );
			}
		}

		this->dynamicModel = this->cachedDynamicModel;
		this->dynamicModelFrameCount = tr.GetFrameCount();
	}

	// set model depth hack value
	this->parms.modelDepthHack = 0.0f;
	if( this->dynamicModel != NULL && model->DepthHack() != 0.0f && tr.viewDef != NULL )
	{
		idVec3 ndc;

		idVec4 eye, clip;
		idRenderMatrix::TransformModelToClip( this->parms.origin, tr.viewDef->GetViewMatrix(), tr.viewDef->GetProjectionMatrix(), eye, clip );
		idRenderMatrix::TransformClipToDevice( clip, ndc );

		this->parms.modelDepthHack = model->DepthHack() * ( 1.0f - ndc.z );
	}

	return this->dynamicModel;
}


/*
===================
 ReadFromDemoFile
===================
*/
void idRenderEntityLocal::ReadFromDemoFile( class idDemoFile* f )
{
	int i;
	renderEntityParms_t ent;
	/* Initialize Pointers */
	decals = NULL;
	overlays = NULL;
	dynamicModel = NULL;
	ent.referenceMaterial = NULL;
	ent.referenceSound = NULL;
	ent.hModel = NULL;
	ent.customSkin = NULL;
	ent.customMaterial = NULL;
	ent.joints = NULL;
	ent.callback = NULL;
	ent.callbackData = NULL;
	ent.remoteRenderView = NULL;

	f->ReadInt( index );
	f->ReadInt( dynamicModelFrameCount );
	f->ReadInt( ent.entityNum );
	f->ReadInt( ent.bodyId );
	f->ReadVec3( ent.bounds[ 0 ] );
	f->ReadVec3( ent.bounds[ 1 ] );
	f->ReadInt( ent.suppressSurfaceInViewID );
	f->ReadInt( ent.suppressShadowInViewID );
	f->ReadInt( ent.suppressShadowInLightID );
	f->ReadInt( ent.allowSurfaceInViewID );
	f->ReadVec3( ent.origin );
	f->ReadMat3( ent.axis );
	for( i = 0; i < MAX_ENTITY_SHADER_PARMS; i++ )
	{
		f->ReadFloat( ent.shaderParms[ i ] );
	}
	for( i = 0; i < MAX_RENDERENTITY_GUI; i++ )
	{
		f->ReadInt( ( int& )ent.gui[ i ] );
		ent.gui[ i ] = NULL;
	}
	f->ReadInt( i ); //( int& )ent.remoteRenderView
	f->ReadInt( ent.numJoints );
	f->ReadFloat( ent.modelDepthHack );
	f->ReadBool( ent.noSelfShadow );
	f->ReadBool( ent.noShadow );
	f->ReadBool( ent.noOverlays );
	f->ReadBool( ent.noDynamicInteractions );
	f->ReadBool( ent.weaponDepthHack );
	f->ReadInt( ent.forceUpdate );

	const char* declName = NULL;

	declName = f->ReadHashString();
	ent.customMaterial = ( declName[ 0 ] != 0 ) ? declManager->FindMaterial( declName ) : NULL;

	declName = f->ReadHashString();
	ent.referenceMaterial = ( declName[ 0 ] != 0 ) ? declManager->FindMaterial( declName ) : NULL;

	declName = f->ReadHashString();
	ent.customSkin = ( declName[ 0 ] != 0 ) ? declManager->FindSkin( declName ) : NULL;

	int soundIndex = -1;
	f->ReadInt( soundIndex );
	ent.referenceSound = soundIndex != -1 ? common->SW()->EmitterForIndex( soundIndex ) : NULL;

	const char* mdlName = f->ReadHashString();
	ent.hModel = ( mdlName[ 0 ] != 0 ) ? renderModelManager->FindModel( mdlName ) : NULL;

	/*if( ent.hModel != NULL )
	{
	bool dynamicModel = false;
	f->ReadBool( dynamicModel );
	if( dynamicModel )
	{
	ent.hModel->ReadFromDemoFile( f );
	}
	}*/

	if( ent.numJoints > 0 )
	{
		ent.joints = ( idJointMat* )Mem_Alloc16( SIMD_ROUND_JOINTS( ent.numJoints ) * sizeof( ent.joints[ 0 ] ), TAG_JOINTMAT );
		for( int i = 0; i < ent.numJoints; i++ )
		{
			float* data = ent.joints[ i ].ToFloatPtr();
			for( int j = 0; j < 12; ++j )
			{
				f->ReadFloat( data[ j ] );
			}
		}
		SIMD_INIT_LAST_JOINT( ent.joints, ent.numJoints );
	}

	f->ReadInt( ent.timeGroup );
	f->ReadInt( ent.xrayIndex );

	/*
	f->ReadInt( i );
	if( i ) {
	ent.overlays = idRenderModelOverlay::Alloc();
	ent.overlays->ReadFromDemoFile( f->Read );
	}
	*/

	world->UpdateEntityDef( index, &ent );
	for( i = 0; i < MAX_RENDERENTITY_GUI; i++ )
	{
		if( parms.gui[ i ] )
		{
			parms.gui[ i ] = uiManager->Alloc();
		#ifdef WRITE_GUIS
			parms.gui[ i ]->ReadFromDemoFile( f->Read );
		#endif
		}
	}
	if( r_showDemo.GetBool() )
	{
		common->Printf( "DC_UPDATE_ENTITYDEF: %i = %s\n", index, parms.hModel ? parms.hModel->Name() : "NULL" );
	}
}
/*
===================
 WriteToDemoFile
===================
*/
void idRenderEntityLocal::WriteToDemoFile( class idDemoFile* f ) const
{
	f->WriteInt( index );
	f->WriteInt( dynamicModelFrameCount );
	f->WriteInt( parms.entityNum );
	f->WriteInt( parms.bodyId );
	f->WriteVec3( parms.bounds[ 0 ] );
	f->WriteVec3( parms.bounds[ 1 ] );
	f->WriteInt( parms.suppressSurfaceInViewID );
	f->WriteInt( parms.suppressShadowInViewID );
	f->WriteInt( parms.suppressShadowInLightID );
	f->WriteInt( parms.allowSurfaceInViewID );
	f->WriteVec3( parms.origin );
	f->WriteMat3( parms.axis );
	for( int i = 0; i < MAX_ENTITY_SHADER_PARMS; i++ )
		f->WriteFloat( parms.shaderParms[ i ] );
	for( int i = 0; i < MAX_RENDERENTITY_GUI; i++ )
		f->WriteInt( ( int& )parms.gui[ i ] );
	f->WriteInt( ( int& )parms.remoteRenderView );
	f->WriteInt( parms.numJoints );
	f->WriteFloat( parms.modelDepthHack );
	f->WriteBool( parms.noSelfShadow );
	f->WriteBool( parms.noShadow );
	f->WriteBool( parms.noOverlays );
	f->WriteBool( parms.noDynamicInteractions );
	f->WriteBool( parms.weaponDepthHack );
	f->WriteInt( parms.forceUpdate );

	f->WriteHashString( parms.customMaterial ? parms.customMaterial->GetName() : "" );
	f->WriteHashString( parms.referenceMaterial ? parms.referenceMaterial->GetName() : "" );
	f->WriteHashString( parms.customSkin ? parms.customSkin->GetName() : "" );
	f->WriteInt( parms.referenceSound ? parms.referenceSound->Index() : -1 );

	f->WriteHashString( parms.hModel ? parms.hModel->Name() : "" );
	/*if( parms.hModel )
	{
	f->WriteBool( parms.hModel->ModelDynamicallyGenerated() ? true : false );
	if( parms.hModel->ModelDynamicallyGenerated() )
	{
	parms.hModel->WriteToDemoFile( f );
	}
	}*/

	for( int i = 0; i < parms.numJoints; i++ )
	{
		float* data = parms.joints[ i ].ToFloatPtr();
		for( int j = 0; j < 12; ++j )
			f->WriteFloat( data[ j ] );
	}

	// RENDERDEMO_VERSION >= 2 ( Doom3 1.2 )
	f->WriteInt( parms.timeGroup );
	f->WriteInt( parms.xrayIndex );

#ifdef WRITE_GUIS
	for( i = 0; i < MAX_RENDERENTITY_GUI; i++ )
	{
		if( parms.gui[ i ] )
		{
			f->WriteInt( 1 );
			parms.gui[ i ]->WriteToDemoFile( f );
		}
		else
		{
			f->WriteInt( 0 );
		}
	}
#endif

	if( r_showDemo.GetBool() )
	{
		common->Printf( "write DC_UPDATE_ENTITYDEF: %i = %s\n", index, parms.hModel ? parms.hModel->Name() : "NULL" );
	}
}


//======================================================================

idRenderLightLocal::idRenderLightLocal()
{
	parms.Clear();
	memset( lightProject, 0, sizeof( lightProject ) );

	lightHasMoved			= false;
	world					= NULL;
	index					= 0;
	areaNum					= 0;
	lastModifiedFrameNum	= 0;
	archived				= false;
	lightShader				= NULL;
	falloffImage			= NULL;
	globalLightOrigin		= vec3_zero;
	viewCount				= 0;
	viewLight				= NULL;
	references				= NULL;
	foggedPortals			= NULL;
	firstInteraction		= NULL;
	lastInteraction			= NULL;

	baseLightProject.Zero();
	inverseBaseLightProject.Zero();
}

/*
==================
 R_EXP_CalcLightAxialSize

	All light side projections must currently match for shadow maps, so non-centered
	and non-cubic lights must take the largest length
==================
*/
float idRenderLightLocal::GetAxialSize() const
{
	float max = 0;

	if( !parms.pointLight )
	{
		idVec3 dir = parms.target - parms.origin;
		max = dir.Length();
		return max;
	}
	for( int i = 0; i < 3; i++ )
	{
		float dist = idMath::Fabs( parms.lightCenter[ i ] );
		dist += parms.lightRadius[ i ];
		if( dist > max )
		{
			max = dist;
		}
	}
	return max;
}

/*
======================
 R_LocalPlaneToGlobal

	NOTE: assumes no skewing or scaling transforms
======================
*/
static void LocalPlaneToGlobal( const idRenderMatrix & modelMatrix, const idPlane& in, idPlane& out )
{
	out[ 0 ] = in[ 0 ] * modelMatrix[ 0 ][ 0 ] + in[ 1 ] * modelMatrix[ 0 ][ 1 ] + in[ 2 ] * modelMatrix[ 0 ][ 2 ];
	out[ 1 ] = in[ 0 ] * modelMatrix[ 1 ][ 0 ] + in[ 1 ] * modelMatrix[ 1 ][ 1 ] + in[ 2 ] * modelMatrix[ 1 ][ 2 ];
	out[ 2 ] = in[ 0 ] * modelMatrix[ 2 ][ 0 ] + in[ 1 ] * modelMatrix[ 2 ][ 1 ] + in[ 2 ] * modelMatrix[ 2 ][ 2 ];
	out[ 3 ] = in[ 3 ] - modelMatrix[ 0 ][ 3 ] * out[ 0 ] - modelMatrix[ 1 ][ 3 ] * out[ 1 ] - modelMatrix[ 2 ][ 3 ] * out[ 2 ];
}

/*
========================
 R_ComputePointLightProjectionMatrix

	Computes the light projection matrix for a point light.
========================
*/
static float R_ComputePointLightProjectionMatrix( const renderLightParms_t& parms, idRenderMatrix& localProject )
{
	assert( light->parms.pointLight );

	// A point light uses a box projection.
	// This projects into the 0.0 - 1.0 texture range instead of -1.0 to 1.0 clip space range.
	localProject.Zero();
	localProject[ 0 ][ 0 ] = 0.5f / parms.lightRadius[ 0 ];
	localProject[ 1 ][ 1 ] = 0.5f / parms.lightRadius[ 1 ];
	localProject[ 2 ][ 2 ] = 0.5f / parms.lightRadius[ 2 ];
	localProject[ 0 ][ 3 ] = 0.5f;
	localProject[ 1 ][ 3 ] = 0.5f;
	localProject[ 2 ][ 3 ] = 0.5f;
	localProject[ 3 ][ 3 ] = 1.0f;	// identity perspective

	return 1.0f;
}

static const float SPOT_LIGHT_MIN_Z_NEAR = 8.0f;
static const float SPOT_LIGHT_MIN_Z_FAR = 16.0f;

/*
========================
 R_ComputeSpotLightProjectionMatrix

	Computes the light projection matrix for a spot light.
========================
*/
static float R_ComputeSpotLightProjectionMatrix( const renderLightParms_t& parms, idRenderMatrix& localProject )
{
	const float targetDistSqr = parms.target.LengthSqr();
	const float invTargetDist = idMath::InvSqrt( targetDistSqr );
	const float targetDist = invTargetDist * targetDistSqr;

	const idVec3 normalizedTarget = parms.target * invTargetDist;
	const idVec3 normalizedRight = parms.right * ( 0.5f * targetDist / parms.right.LengthSqr() );
	const idVec3 normalizedUp = parms.up * ( -0.5f * targetDist / parms.up.LengthSqr() );

	localProject[ 0 ][ 0 ] = normalizedRight[ 0 ];
	localProject[ 0 ][ 1 ] = normalizedRight[ 1 ];
	localProject[ 0 ][ 2 ] = normalizedRight[ 2 ];
	localProject[ 0 ][ 3 ] = 0.0f;

	localProject[ 1 ][ 0 ] = normalizedUp[ 0 ];
	localProject[ 1 ][ 1 ] = normalizedUp[ 1 ];
	localProject[ 1 ][ 2 ] = normalizedUp[ 2 ];
	localProject[ 1 ][ 3 ] = 0.0f;

	localProject[ 3 ][ 0 ] = normalizedTarget[ 0 ];
	localProject[ 3 ][ 1 ] = normalizedTarget[ 1 ];
	localProject[ 3 ][ 2 ] = normalizedTarget[ 2 ];
	localProject[ 3 ][ 3 ] = 0.0f;

	// Set the falloff vector.
	// This is similar to the Z calculation for depth buffering, which means that the
	// mapped texture is going to be perspective distorted heavily towards the zero end.
	const float zNear = idMath::Max( parms.start * normalizedTarget, SPOT_LIGHT_MIN_Z_NEAR );
	const float zFar = idMath::Max( parms.end * normalizedTarget, SPOT_LIGHT_MIN_Z_FAR );
	const float zScale = ( zNear + zFar ) / zFar;

	localProject[ 2 ][ 0 ] = normalizedTarget[ 0 ] * zScale;
	localProject[ 2 ][ 1 ] = normalizedTarget[ 1 ] * zScale;
	localProject[ 2 ][ 2 ] = normalizedTarget[ 2 ] * zScale;
	localProject[ 2 ][ 3 ] = -zNear * zScale;

	// now offset to the 0.0 - 1.0 texture range instead of -1.0 to 1.0 clip space range
	idVec4 projectedTarget;
	localProject.TransformPoint( parms.target, projectedTarget );

	const float ofs0 = 0.5f - projectedTarget[ 0 ] / projectedTarget[ 3 ];
	localProject[ 0 ][ 0 ] += ofs0 * localProject[ 3 ][ 0 ];
	localProject[ 0 ][ 1 ] += ofs0 * localProject[ 3 ][ 1 ];
	localProject[ 0 ][ 2 ] += ofs0 * localProject[ 3 ][ 2 ];
	localProject[ 0 ][ 3 ] += ofs0 * localProject[ 3 ][ 3 ];

	const float ofs1 = 0.5f - projectedTarget[ 1 ] / projectedTarget[ 3 ];
	localProject[ 1 ][ 0 ] += ofs1 * localProject[ 3 ][ 0 ];
	localProject[ 1 ][ 1 ] += ofs1 * localProject[ 3 ][ 1 ];
	localProject[ 1 ][ 2 ] += ofs1 * localProject[ 3 ][ 2 ];
	localProject[ 1 ][ 3 ] += ofs1 * localProject[ 3 ][ 3 ];

	return 1.0f / ( zNear + zFar );
}

/*
========================
 R_ComputeParallelLightProjectionMatrix

	Computes the light projection matrix for a parallel light.
========================
*/
static float R_ComputeParallelLightProjectionMatrix( const renderLightParms_t& parms, idRenderMatrix& localProject )
{
	assert( light->parms.parallel );

	// A parallel light uses a box projection.
	// This projects into the 0.0 - 1.0 texture range instead of -1.0 to 1.0 clip space range.
	localProject.Zero();
	localProject[ 0 ][ 0 ] = 0.5f / parms.lightRadius[ 0 ];
	localProject[ 1 ][ 1 ] = 0.5f / parms.lightRadius[ 1 ];
	localProject[ 2 ][ 2 ] = 0.5f / parms.lightRadius[ 2 ];
	localProject[ 0 ][ 3 ] = 0.5f;
	localProject[ 1 ][ 3 ] = 0.5f;
	localProject[ 2 ][ 3 ] = 0.5f;
	localProject[ 3 ][ 3 ] = 1.0f;	// identity perspective

	return 1.0f;
}

/*
=================
 R_DeriveLightData

	Fills everything in based on light->parms
=================
*/
void idRenderLightLocal::DeriveData()
{
	idRenderLightLocal* light = this;

	/*{ //SEA: FuglyHack to make big point_lights parallel
		if( light->parms.pointLight && GetAxialSize() > 1000.0 )
		{
			light->parms.parallel = true;
		}
	}*/

	// decide which light shader we are going to use
	if( light->parms.shader != NULL )
	{
		light->lightShader = light->parms.shader;
	}
	else if( light->lightShader == NULL )
	{
		if( light->parms.pointLight )
		{
			light->lightShader = tr.defaultPointLight;
		}
		else {
			light->lightShader = tr.defaultProjectedLight;
		}
	}

	// get the falloff image
	light->falloffImage = light->lightShader->LightFalloffImage();

	if( light->falloffImage == NULL )
	{
		// use the falloff from the default shader of the correct type
		const idMaterial* defaultShader;

		if( light->parms.pointLight )
		{
			defaultShader = tr.defaultPointLight;

			// Touch the default shader. to make sure it's decl has been parsed ( it might have been purged ).
			declManager->Touch( static_cast< const idDecl*>( defaultShader ) );

			light->falloffImage = defaultShader->LightFalloffImage();
		}
		else {
			// projected lights by default don't diminish with distance
			defaultShader = tr.defaultProjectedLight;

			// Touch the light shader. to make sure it's decl has been parsed ( it might have been purged ).
			declManager->Touch( static_cast< const idDecl*>( defaultShader ) );

			light->falloffImage = defaultShader->LightFalloffImage();
		}
	}

	// ------------------------------------
	// compute the light projection matrix
	// ------------------------------------

	idRenderMatrix localProject;
	float zScale = 1.0f;
	if( light->parms.parallel )
	{
		zScale = R_ComputeParallelLightProjectionMatrix( light->parms, localProject );
	}
	else if( light->parms.pointLight )
	{
		zScale = R_ComputePointLightProjectionMatrix( light->parms, localProject );
	}
	else {
		zScale = R_ComputeSpotLightProjectionMatrix( light->parms, localProject );
	}

	// set the old style light projection where Z and W are flipped and
	// for projected lights lightProject[3] is divided by ( zNear + zFar )
	light->lightProject[ 0 ][ 0 ] = localProject[ 0 ][ 0 ];
	light->lightProject[ 0 ][ 1 ] = localProject[ 0 ][ 1 ];
	light->lightProject[ 0 ][ 2 ] = localProject[ 0 ][ 2 ];
	light->lightProject[ 0 ][ 3 ] = localProject[ 0 ][ 3 ];

	light->lightProject[ 1 ][ 0 ] = localProject[ 1 ][ 0 ];
	light->lightProject[ 1 ][ 1 ] = localProject[ 1 ][ 1 ];
	light->lightProject[ 1 ][ 2 ] = localProject[ 1 ][ 2 ];
	light->lightProject[ 1 ][ 3 ] = localProject[ 1 ][ 3 ];

	light->lightProject[ 2 ][ 0 ] = localProject[ 3 ][ 0 ];
	light->lightProject[ 2 ][ 1 ] = localProject[ 3 ][ 1 ];
	light->lightProject[ 2 ][ 2 ] = localProject[ 3 ][ 2 ];
	light->lightProject[ 2 ][ 3 ] = localProject[ 3 ][ 3 ];

	light->lightProject[ 3 ][ 0 ] = localProject[ 2 ][ 0 ] * zScale;
	light->lightProject[ 3 ][ 1 ] = localProject[ 2 ][ 1 ] * zScale;
	light->lightProject[ 3 ][ 2 ] = localProject[ 2 ][ 2 ] * zScale;
	light->lightProject[ 3 ][ 3 ] = localProject[ 2 ][ 3 ] * zScale;

	// transform the lightProject
	{
		idRenderMatrix lightTransform;
		idRenderMatrix::CreateFromOriginAxis( light->parms.origin, light->parms.axis, lightTransform );
		for( int i = 0; i < 4; i++ )
		{
			idRenderPlane temp = light->lightProject[ i ];
			//lightTransform.TransformPlane( temp, light->lightProject[ i ] );
			LocalPlaneToGlobal( lightTransform, temp, light->lightProject[ i ] );
		}
	}
	// adjust global light origin for off center projections and parallel projections
	// we are just faking parallel by making it a very far off center for now
	if( light->parms.parallel )
	{
		idVec3 dir = light->parms.lightCenter;
		if( dir.Normalize() == 0.0f )
		{
			// make point straight up if not specified
			dir[ 2 ] = 1.0f;
		}
		light->globalLightOrigin = light->parms.origin + dir * 100000.0f;
	}
	else
	{
		light->globalLightOrigin = light->parms.origin + light->parms.axis * light->parms.lightCenter;
	}
	{
		// Rotate and translate the light projection by the light matrix.
		// 99% of lights remain axis aligned in world space.
		idRenderMatrix lightMatrix;
		idRenderMatrix::CreateFromOriginAxis( light->parms.origin, light->parms.axis, lightMatrix );

		idRenderMatrix inverseLightMatrix;
		if( !idRenderMatrix::Inverse( lightMatrix, inverseLightMatrix ) )
		{
			idLib::Warning( "lightMatrix invert failed" );
		}

		// 'baseLightProject' goes from global space -> light local space -> light projective space
		idRenderMatrix::Multiply( localProject, inverseLightMatrix, light->baseLightProject );
	}

	// Invert the light projection so we can deform zero-to-one cubes into
	// the light model and calculate global bounds.
	if( !idRenderMatrix::Inverse( light->baseLightProject, light->inverseBaseLightProject ) )
	{
		idLib::Warning( "baseLightProject invert failed" );
	}

	// calculate the global light bounds by inverse projecting the zero to one cube with the 'inverseBaseLightProject'
	idRenderMatrix::ProjectedBounds( light->globalLightBounds, light->inverseBaseLightProject, bounds_zeroOneCube, false );
}

/*
====================
 R_FreeLightDefDerivedData

	Frees all references and lit surfaces from the light
====================
*/
void idRenderLightLocal::FreeDerivedData()
{
	idRenderLightLocal* ldef = this;

	// remove any portal fog references
	for( doublePortal_t* dp = ldef->foggedPortals; dp != NULL; dp = dp->nextFoggedPortal )
	{
		dp->fogLight = NULL;
	}

	// free all the interactions
	while( ldef->firstInteraction != NULL )
	{
		ldef->firstInteraction->UnlinkAndFree();
	}

	// free all the references to the light
	areaReference_t* nextRef = NULL;
	for( areaReference_t* lref = ldef->references; lref != NULL; lref = nextRef )
	{
		nextRef = lref->ownerNext;

		// unlink from the area
		lref->areaNext->areaPrev = lref->areaPrev;
		lref->areaPrev->areaNext = lref->areaNext;

		// put it back on the free list for reuse
		ldef->world->areaReferenceAllocator.Free( lref );
	}
	ldef->references = NULL;
}

/*
===============
 WindingCompletelyInsideLight
===============
*/
static bool WindingCompletelyInsideLight( const idWinding* w, const idRenderLightLocal* ldef )
{
	for( int i = 0; i < w->GetNumPoints(); ++i )
	{
		if( idRenderMatrix::CullPointToMVP( ldef->baseLightProject, ( *w )[ i ].ToVec3(), true ) )
		{
			return false;
		}
	}
	return true;
}

/*
=================
 R_CreateLightRefs 		SEA: this shuold be part of renderworld probably ?
=================
*/
void idRenderLightLocal::CreateReferences()
{
	idRenderLightLocal* light = this;

	// derive light data
	light->DeriveData();

	// determine the areaNum for the light origin, which may let us
	// cull the light if it is behind a closed door
	// it is debatable if we want to use the entity origin or the center offset origin,
	// but we definitely don't want to use a parallel offset origin
	light->areaNum = light->world->PointInArea( light->globalLightOrigin );
	if( light->areaNum == -1 )
	{
		light->areaNum = light->world->PointInArea( light->parms.origin );
	}

	// bump the view count so we can tell if an
	// area already has a reference
	tr.viewCount++;

	// if we have a prelight model that includes all the shadows for the major world occluders,
	// we can limit the area references to those visible through the portals from the light center.
	// We can't do this in the normal case, because shadows are cast from back facing triangles, which
	// may be in areas not directly visible to the light projection center.
	if( light->parms.prelightModel != nullptr && r_useLightPortalFlow.GetBool() && light->lightShader->LightCastsShadows() )
	{
		light->world->FlowLightThroughPortals( light );
	}
	else {
		// push the light frustum down the BSP tree into areas
		light->world->PushFrustumIntoTree( nullptr, light, light->inverseBaseLightProject, bounds_zeroOneCube );
	}

	// ----------------------------------------------------------------
	// Create light def fogPortals
	// When a fog light is created or moved, see if it completely
	// encloses any portals, which may allow them to be fogged closed.
	// ----------------------------------------------------------------
	{
		light->foggedPortals = nullptr;

		if( !light->lightShader->IsFogLight() )
		{
			return;
		}

		// some fog lights will explicitly disallow portal fogging
		if( light->lightShader->TestMaterialFlag( MF_NOPORTALFOG ) )
		{
			return;
		}

		for( auto lref = light->references; lref != nullptr; lref = lref->ownerNext )
		{
			// check all the models in this area
			auto area = lref->area;

			for( auto prt = area->portals; prt != nullptr; prt = prt->next )
			{
				auto dp = prt->doublePortal;

				// we only handle a single fog volume covering a portal
				// this will never cause incorrect drawing, but it may
				// fail to cull a portal
				if( dp->fogLight )
				{
					continue;
				}

				if( WindingCompletelyInsideLight( prt->w, light ) )
				{
					dp->fogLight = light;

					dp->nextFoggedPortal = light->foggedPortals;
					light->foggedPortals = dp;
				}
			}
		}
	}
}
