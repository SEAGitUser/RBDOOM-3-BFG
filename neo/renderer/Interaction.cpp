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

/*
===========================================================================

idInteraction implementation

===========================================================================
*/

class idRenderFrustum {
public:
	idRenderPlane planes[6];
};

/*
================
 DetermineFacing

	Determines which triangles of the surface are facing towards the referenced origin.

	The facing array should be allocated with one extra index than
	the number of surface triangles, which will be used to handle dangling
	edge silhouettes.
================
*/
void idTriangles::DetermineFacing( const idVec3 & referencedOrigin, cullInfo_t & cullInfo ) const
{
	assert( cullInfo.facing == nullptr );

	const int numFaces = this->GetNumFaces();
	cullInfo.facing = allocManager.StaticAlloc<byte, TAG_RENDER_INTERACTION>( numFaces + 1 );

	// exact geometric cull against face
	for( int i = 0, face = 0; i < this->numIndexes; i += 3, ++face )
	{
		const idDrawVert & v0 = this->GetIVertex( i + 0 );
		const idDrawVert & v1 = this->GetIVertex( i + 1 );
		const idDrawVert & v2 = this->GetIVertex( i + 2 );

		const idPlane plane( v0.GetPosition(), v1.GetPosition(), v2.GetPosition() );
		const float d = plane.Distance( referencedOrigin );

		cullInfo.facing[ face ] = ( d >= 0.0f );
	}
	cullInfo.facing[ numFaces ] = 1;	// for dangling edges to reference
}
/*
=====================
 DetermineCullBits

	We want to cull a little on the sloppy side, because the pre-clipping
	of geometry to the lights in dmap will give many cases that are right
	at the border. We throw things out on the border, because if any one
	vertex is clearly inside, the entire triangle will be accepted.
=====================
*/
void idTriangles::DetermineCullBits( const idPlane clipPlanes[6], cullInfo_t& cullInfo ) const
{
	assert( cullInfo.cullBits == nullptr );
	assert( clipPlanes != nullptr );

	int frontBits = 0;

	// cull the triangle surface bounding box
	for( int i = 0; i < 6; i++ )
	{
		// get front bits for the whole surface
		if( this->GetBounds().PlaneDistance( clipPlanes[ i ] ) >= LIGHT_CLIP_EPSILON )
		{
			frontBits |= 1 << i;
		}
	}

	// if the surface is completely inside the light frustum
	if( frontBits == ( ( 1 << 6 ) - 1 ) )
	{
		cullInfo.cullBits = LIGHT_CULL_ALL_FRONT;
		return;
	}

	cullInfo.cullBits = allocManager.StaticAlloc<byte, TAG_RENDER_INTERACTION, true>( this->numVerts );

	for( int i = 0; i < 6; i++ )
	{
		// if completely infront of this clipping plane
		if( frontBits & ( 1 << i ) )
		{
			continue;
		}
		for( int j = 0; j < this->numVerts; ++j )
		{
			float d = clipPlanes[ i ].Distance( this->verts[ j ].GetPosition() );
			cullInfo.cullBits[ j ] |= ( d < LIGHT_CLIP_EPSILON ) << i;
		}
	}
}
/*
================
 R_FreeInteractionCullInfo
================
*/
void idTriangles::cullInfo_t::Free()
{
	if( facing != NULL )
	{
		allocManager.StaticFree( facing );
		facing = NULL;
	}
	if( cullBits != NULL )
	{
		if( cullBits != LIGHT_CULL_ALL_FRONT )
		{
			allocManager.StaticFree( cullBits );
		}
		cullBits = NULL;
	}
}

idTriangles * idTriangles::CreateTriangleSubsetFromCulling( const idPlane localClipPlanes[ 6 ], bool backFaceCull, const idVec3 & referencedOrigin ) const
{
	int	i, faceNum;

	int c_backfaced = 0;
	int c_distance = 0;

	int numIndexes = 0;
	triIndex_t * indexes = NULL;

	// allocate a new surface for the lit triangles
	auto newTri = idTriangles::AllocStatic();

	// save a reference to the original surface
	newTri->baseTriangles = const_cast<idTriangles*>( this );

	// the light surface references the verts of the base surface
	newTri->numVerts = this->numVerts;
	newTri->ReferenceStaticVerts( this );

	// calculate cull information
	idTriangles::cullInfo_t cullInfo = {};
	if( backFaceCull ) {
		this->DetermineFacing( referencedOrigin, cullInfo );
	}
	this->DetermineCullBits( localClipPlanes, cullInfo );

	// if the surface is completely inside the light frustum
	if( cullInfo.cullBits == LIGHT_CULL_ALL_FRONT )
	{
		// if we aren't self shadowing, let back facing triangles get
		// through so the smooth shaded bump maps light all the way around
		if( !backFaceCull )
		{
			// the whole surface is lit so the light surface just references the indexes of the ambient surface
			newTri->indexCache = this->indexCache;
			newTri->ReferenceStaticIndexes( this );

			numIndexes = this->numIndexes;
			newTri->CopyBounds( this );
		}
		else
		{
			// the light tris indexes are going to be a subset of the original indexes so we generally
			// allocate too much memory here but we decrease the memory block when the number of indexes is known
			newTri->AllocStaticIndexes( this->numIndexes );

			// back face cull the individual triangles
			indexes = newTri->indexes;
			const byte* facing = cullInfo.facing;
			for( faceNum = i = 0; i < this->numIndexes; i += 3, ++faceNum )
			{
				if( !facing[ faceNum ] )
				{
					c_backfaced++;
					continue;
				}
				indexes[ numIndexes + 0 ] = this->indexes[ i + 0 ];
				indexes[ numIndexes + 1 ] = this->indexes[ i + 1 ];
				indexes[ numIndexes + 2 ] = this->indexes[ i + 2 ];
				numIndexes += 3;
			}

			// get bounds for the surface
			//SIMDProcessor->MinMax( bounds[0], bounds[1], this->verts, indexes, numIndexes );

			// decrease the size of the memory block to the size of the number of used indexes
			newTri->numIndexes = numIndexes;
			newTri->ResizeStaticIndexes( numIndexes );

			// get bounds for the surface
			newTri->DeriveBounds( true );
		}
	}
	else {
		// the light tris indexes are going to be a subset of the original indexes so we generally
		// allocate too much memory here but we decrease the memory block when the number of indexes is known
		newTri->AllocStaticIndexes( this->numIndexes );

		// cull individual triangles
		indexes = newTri->indexes;
		const byte* facing = cullInfo.facing;
		const byte* cullBits = cullInfo.cullBits;
		for( faceNum = i = 0; i < this->numIndexes; i += 3, ++faceNum )
		{
			// if we aren't self shadowing, let back facing triangles get
			// through so the smooth shaded bump maps light all the way around
			if( backFaceCull )
			{
				// back face cull
				if( !facing[ faceNum ] )
				{
					c_backfaced++;
					continue;
				}
			}

			int i1 = this->indexes[ i + 0 ];
			int i2 = this->indexes[ i + 1 ];
			int i3 = this->indexes[ i + 2 ];

			// fast cull outside the frustum
			// if all three points are off one plane side, it definately isn't visible
			if( cullBits[ i1 ] & cullBits[ i2 ] & cullBits[ i3 ] )
			{
				c_distance++;
				continue;
			}

			// add to the list
			indexes[ numIndexes + 0 ] = i1;
			indexes[ numIndexes + 1 ] = i2;
			indexes[ numIndexes + 2 ] = i3;
			numIndexes += 3;
		}

		// get bounds for the surface
		//SIMDProcessor->MinMax( bounds[0], bounds[1], this->verts, indexes, numIndexes );

		// decrease the size of the memory block to the size of the number of used indexes
		newTri->numIndexes = numIndexes;
		newTri->ResizeStaticIndexes( numIndexes );

		// get bounds for the surface
		newTri->DeriveBounds( true );
	}

	// free the cull information when it's no longer needed
	cullInfo.Free();

	if( !numIndexes )
	{
		assert( "numIndexes == 0" );
		idTriangles::FreeStatic( newTri );
		return NULL;
	}

	newTri->numIndexes = numIndexes;

	//newTri->bounds = bounds;
	///newTri->DeriveBounds( true );

	return newTri;
}



/*
================================
 CalcInteractionCullInfo
================================
*/
static void CalcInteractionCullInfo( const idRenderEntityLocal* ent, const idRenderLightLocal* light,
	const idTriangles* tri, idTriangles::cullInfo_t & cullInfo, bool facing )
{
	if( facing )
	{
		// Calculate Interaction Facing
		// Determines which triangles of the surface are facing towards the light origin.
		// The facing array should be allocated with one extra index than
		// the number of surface triangles, which will be used to handle dangling
		// edge silhouettes.

		if( cullInfo.facing == nullptr )
		{
			SCOPED_PROFILE_EVENT( "R_CalcInteractionFacing" );

			idVec3 localLightOrigin;
			ent->modelRenderMatrix.InverseTransformPoint( light->globalLightOrigin, localLightOrigin );

			tri->DetermineFacing( localLightOrigin, cullInfo );
		}
	}

	// Calculate Interaction CullBits.
	// We want to cull a little on the sloppy side, because the pre - clipping
	// of geometry to the lights in dmap will give many cases that are right
	// at the border.We throw things out on the border, because if any one
	// vertex is clearly inside, the entire triangle will be accepted.

	if( cullInfo.cullBits == nullptr )
	{
		SCOPED_PROFILE_EVENT( "R_CalcInteractionCullBits" );

		// Clip planes in surface space used to calculate the cull bits.
		idRenderPlane localClipPlanes[ 6 ];
		{
			idRenderPlane frustumPlanes[ 6 ];
			idRenderMatrix::GetFrustumPlanes( frustumPlanes, light->baseLightProject, true, true );
			for( int i = 0; i < 6; i++ )
			{
				ent->modelRenderMatrix.InverseTransformPlane( frustumPlanes[ i ], localClipPlanes[ i ] );
			}
		}
		tri->DetermineCullBits( localClipPlanes, cullInfo );
	}
}

/*
====================
R_CreateInteractionLightTris

	This is only used for the static interaction case, dynamic interactions
	just draw everything and let the GPU deal with it.

	The resulting surface will be a subset of the original triangles,
	it will never clip triangles, but it may cull on a per-triangle basis.
====================
*/
static idTriangles* R_CreateInteractionLightTris( const idRenderEntityLocal* entity, const idRenderLightLocal* light, bool includeBackFaces, const idTriangles* tri )
{
	SCOPED_PROFILE_EVENT( "R_CreateInteractionLightTris" );
#if 1
	
	idVec3 localLightOrigin( vec3_origin );

	if( !includeBackFaces )
	{
		// Calculate Interaction Facing

		// Determines which triangles of the surface are facing towards the light origin.
		// The facing array should be allocated with one extra index than
		// the number of surface triangles, which will be used to handle dangling
		// edge silhouettes.

		entity->modelRenderMatrix.InverseTransformPoint( light->globalLightOrigin, localLightOrigin );
	}

	// Calculate Interaction CullBits.

	// We want to cull a little on the sloppy side, because the pre - clipping
	// of geometry to the lights in dmap will give many cases that are right
	// at the border.We throw things out on the border, because if any one
	// vertex is clearly inside, the entire triangle will be accepted.

	// Clip planes in surface space used to calculate the cull bits.
	idRenderPlane localClipPlanes[ 6 ];
	{
		idRenderPlane frustumPlanes[ 6 ];
		idRenderMatrix::GetFrustumPlanes( frustumPlanes, light->baseLightProject, true, true );
		for( int i = 0; i < 6; i++ )
		{
			entity->modelRenderMatrix.InverseTransformPlane( frustumPlanes[ i ], localClipPlanes[ i ] );
		}
	}

	return tri->CreateTriangleSubsetFromCulling( localClipPlanes, !includeBackFaces, localLightOrigin );

#else
	idBounds bounds;
	int	i, faceNum;
	
	int c_backfaced = 0;
	int c_distance = 0;
	
	int numIndexes = 0;
	triIndex_t * indexes = NULL;
	
	// allocate a new surface for the lit triangles
	auto newTri = idTriangles::AllocStatic();
	
	// save a reference to the original surface
	newTri->baseTriangles = const_cast<idTriangles*>( tri );
	
	// the light surface references the verts of the base surface
	newTri->numVerts = tri->numVerts;
	newTri->ReferenceStaticVerts( tri );
	
	// calculate cull information
	idTriangles::cullInfo_t cullInfo = {};	
	CalcInteractionCullInfo( ent, light, tri, cullInfo, !includeBackFaces );
	
	// if the surface is completely inside the light frustum
	if( cullInfo.cullBits == LIGHT_CULL_ALL_FRONT )
	{	
		// if we aren't self shadowing, let back facing triangles get
		// through so the smooth shaded bump maps light all the way around
		if( includeBackFaces )
		{		
			// the whole surface is lit so the light surface just references the indexes of the ambient surface
			newTri->indexCache = tri->indexCache;
			newTri->ReferenceStaticIndexes( tri );

			numIndexes = tri->numIndexes;
			newTri->CopyBounds( tri );
		}
		else {		
			// the light tris indexes are going to be a subset of the original indexes so we generally
			// allocate too much memory here but we decrease the memory block when the number of indexes is known
			newTri->AllocStaticIndexes( tri->numIndexes );
			
			// back face cull the individual triangles
			indexes = newTri->indexes;
			const byte* facing = cullInfo.facing;
			for( faceNum = i = 0; i < tri->numIndexes; i += 3, ++faceNum )
			{
				if( !facing[ faceNum ] )
				{
					c_backfaced++;
					continue;
				}
				indexes[numIndexes + 0] = tri->indexes[i + 0];
				indexes[numIndexes + 1] = tri->indexes[i + 1];
				indexes[numIndexes + 2] = tri->indexes[i + 2];
				numIndexes += 3;
			}
			
			// get bounds for the surface
			//SIMDProcessor->MinMax( bounds[0], bounds[1], tri->verts, indexes, numIndexes );
			
			// decrease the size of the memory block to the size of the number of used indexes
			newTri->numIndexes = numIndexes;
			newTri->ResizeStaticIndexes( numIndexes );

			// get bounds for the surface
			newTri->DeriveBounds( true );
		}		
	}
	else {	
		// the light tris indexes are going to be a subset of the original indexes so we generally
		// allocate too much memory here but we decrease the memory block when the number of indexes is known
		newTri->AllocStaticIndexes( tri->numIndexes );
		
		// cull individual triangles
		indexes = newTri->indexes;
		const byte* facing = cullInfo.facing;
		const byte* cullBits = cullInfo.cullBits;
		for( faceNum = i = 0; i < tri->numIndexes; i += 3, ++faceNum )
		{
			// if we aren't self shadowing, let back facing triangles get
			// through so the smooth shaded bump maps light all the way around
			if( !includeBackFaces )
			{
				// back face cull
				if( !facing[ faceNum ] )
				{
					c_backfaced++;
					continue;
				}
			}
			
			int i1 = tri->indexes[i + 0];
			int i2 = tri->indexes[i + 1];
			int i3 = tri->indexes[i + 2];
			
			// fast cull outside the frustum
			// if all three points are off one plane side, it definately isn't visible
			if( cullBits[i1] & cullBits[i2] & cullBits[i3] )
			{
				c_distance++;
				continue;
			}
			
			// add to the list
			indexes[numIndexes + 0] = i1;
			indexes[numIndexes + 1] = i2;
			indexes[numIndexes + 2] = i3;
			numIndexes += 3;
		}
		
		// get bounds for the surface
		//SIMDProcessor->MinMax( bounds[0], bounds[1], tri->verts, indexes, numIndexes );
		
		// decrease the size of the memory block to the size of the number of used indexes
		newTri->numIndexes = numIndexes;
		newTri->ResizeStaticIndexes( numIndexes );

		// get bounds for the surface
		newTri->DeriveBounds( true );
	}
	
	// free the cull information when it's no longer needed
	cullInfo.Free();
	
	if( !numIndexes )
	{
		assert("numIndexes == 0");
		idTriangles::FreeStatic( newTri );
		return NULL;
	}
	
	newTri->numIndexes = numIndexes;
	
	//newTri->bounds = bounds;
	//newTri->DeriveBounds( true );
	
	return newTri;
#endif
}

/*
=====================
 R_CreateInteractionShadowVolume

	Note that dangling edges outside the light frustum don't make silhouette planes because
	a triangle outside the light frustum is considered facing and the "fake triangle" on
	the outside of the dangling edge is also set to facing: cullInfo.facing[numFaces] = 1;
=====================
*/
static idTriangles* R_CreateInteractionShadowVolume( const idRenderEntityLocal* ent, const idRenderLightLocal* light, const idTriangles* tri )
{
	SCOPED_PROFILE_EVENT( "R_CreateInteractionShadowVolume" );
	
	idTriangles::cullInfo_t cullInfo = {};
	CalcInteractionCullInfo( ent, light, tri, cullInfo, true );
	
	int numFaces = tri->GetNumFaces();
	int	numShadowingFaces = 0;
	const byte* facing = cullInfo.facing;
	
	// if all the triangles are inside the light frustum
	if( cullInfo.cullBits == LIGHT_CULL_ALL_FRONT )
	{	
		// count the number of shadowing faces
		for( int i = 0; i < numFaces; i++ )
		{
			numShadowingFaces += facing[i];
		}
		numShadowingFaces = numFaces - numShadowingFaces;		
	}
	else {
		// make all triangles that are outside the light frustum "facing", so they won't cast shadows
		const triIndex_t* indexes = tri->indexes;
		byte* modifyFacing = cullInfo.facing;
		const byte* cullBits = cullInfo.cullBits;
		for( int i = 0, j = 0; i < tri->numIndexes; i += 3, j++ )
		{
			if( !modifyFacing[j] )
			{
				int	i1 = indexes[i + 0];
				int	i2 = indexes[i + 1];
				int	i3 = indexes[i + 2];
				if( cullBits[i1] & cullBits[i2] & cullBits[i3] )
				{
					modifyFacing[j] = 1;
				}
				else {
					numShadowingFaces++;
				}
			}
		}
	}
	
	if( !numShadowingFaces )
	{
		// no faces are inside the light frustum and still facing the right way
		cullInfo.Free();
		return NULL;
	}
	
	// shadowVerts will be NULL on these surfaces, so the shadowVerts will be taken from the ambient surface
	auto newTri = idTriangles::AllocStatic();
	
	newTri->numVerts = tri->numVerts * 2;
	
	// alloc the max possible size
	newTri->AllocStaticIndexes( ( numShadowingFaces + tri->numSilEdges ) * 6 );
	triIndex_t* tempIndexes = newTri->indexes;
	triIndex_t* shadowIndexes = newTri->indexes;
	
	// create new triangles along sil planes
	const silEdge_t* sil = tri->silEdges;
	for( int i = tri->numSilEdges; i > 0; i--, sil++ )
	{	
		int f1 = facing[sil->p1];
		int f2 = facing[sil->p2];
		
		if( !( f1 ^ f2 ) )
		{
			continue;
		}
		
		int v1 = sil->v1 << 1;
		int v2 = sil->v2 << 1;
		
		// set the two triangle winding orders based on facing
		// without using a poorly-predictable branch
		
		shadowIndexes[0] = v1;
		shadowIndexes[1] = v2 ^ f1;
		shadowIndexes[2] = v2 ^ f2;
		shadowIndexes[3] = v1 ^ f2;
		shadowIndexes[4] = v1 ^ f1;
		shadowIndexes[5] = v2 ^ 1;
		
		shadowIndexes += 6;
	}
	
	int	numShadowIndexes = shadowIndexes - tempIndexes;
	
	// we aren't bothering to separate front and back caps on these
	newTri->numIndexes = newTri->numShadowIndexesNoFrontCaps = numShadowIndexes + numShadowingFaces * 6;
	newTri->numShadowIndexesNoCaps = numShadowIndexes;
	newTri->shadowCapPlaneBits = SHADOW_CAP_INFINITE;
	
	// decrease the size of the memory block to only store the used indexes
	// newTri->ResizeStaticIndexes( newTri->numIndexes );
	
	// these have no effect, because they extend to infinity
	newTri->ClearBounds();
	
	// put some faces on the model and some on the distant projection
	const triIndex_t* indexes = tri->indexes;
	shadowIndexes = newTri->indexes + numShadowIndexes;
	for( int i = 0, j = 0; i < tri->numIndexes; i += 3, j++ )
	{
		if( facing[j] )
		{
			continue;
		}
		
		int i0 = indexes[i + 0] << 1;
		int i1 = indexes[i + 1] << 1;
		int i2 = indexes[i + 2] << 1;
		
		shadowIndexes[0] = i2;
		shadowIndexes[1] = i1;
		shadowIndexes[2] = i0;
		shadowIndexes[3] = i0 ^ 1;
		shadowIndexes[4] = i1 ^ 1;
		shadowIndexes[5] = i2 ^ 1;
		
		shadowIndexes += 6;
	}
	
	cullInfo.Free();
	
	return newTri;
}

/*
===============
idInteraction::idInteraction
===============
*/
idInteraction::idInteraction()
{
	numSurfaces			= 0;
	surfaces			= NULL;
	entityDef			= NULL;
	lightDef			= NULL;
	lightNext			= NULL;
	lightPrev			= NULL;
	entityNext			= NULL;
	entityPrev			= NULL;
	isStaticInteraction = false;
}

/*
===============
idInteraction::AllocAndLink
===============
*/
idInteraction* idInteraction::AllocAndLink( idRenderEntityLocal* edef, idRenderLightLocal* ldef )
{
	if( edef == NULL || ldef == NULL )
	{
		common->Error( "idInteraction::AllocAndLink: NULL parm" );
		return NULL;
	}
	
	idRenderWorldLocal* renderWorld = edef->GetOwner();
	
	idInteraction* interaction = renderWorld->interactionAllocator.Alloc();
	
	// link and initialize
	interaction->lightDef = ldef;
	interaction->entityDef = edef;
	
	interaction->numSurfaces = -1;		// not checked yet
	interaction->surfaces = NULL;
	
	// link at the start of the entity's list
	interaction->lightNext = ldef->firstInteraction;
	interaction->lightPrev = NULL;
	ldef->firstInteraction = interaction;
	if( interaction->lightNext != NULL )
	{
		interaction->lightNext->lightPrev = interaction;
	}
	else
	{
		ldef->lastInteraction = interaction;
	}
	
	// link at the start of the light's list
	interaction->entityNext = edef->firstInteraction;
	interaction->entityPrev = NULL;
	edef->firstInteraction = interaction;
	if( interaction->entityNext != NULL )
	{
		interaction->entityNext->entityPrev = interaction;
	}
	else
	{
		edef->lastInteraction = interaction;
	}
	
	// update the interaction table
	if( renderWorld->interactionTable != NULL )
	{
		int index = ldef->GetIndex() * renderWorld->interactionTableWidth + edef->GetIndex();
		if( renderWorld->interactionTable[index] != NULL )
		{
			common->Error( "idInteraction::AllocAndLink: non NULL table entry" );
		}
		renderWorld->interactionTable[ index ] = interaction;
	}
	
	return interaction;
}

/*
===============
idInteraction::FreeSurfaces

Frees the surfaces, but leaves the interaction linked in, so it
will be regenerated automatically
===============
*/
void idInteraction::FreeSurfaces()
{
	// anything regenerated is no longer an optimized static version
	this->isStaticInteraction = false;
	
	if( this->surfaces != NULL )
	{
		for( int i = 0; i < this->numSurfaces; ++i )
		{
			surfaceInteraction_t& srf = this->surfaces[i];
			Mem_Free( srf.shadowIndexes );
			srf.shadowIndexes = NULL;
		}
		allocManager.StaticFree( this->surfaces );
		this->surfaces = NULL;
	}
	this->numSurfaces = -1;
}

/*
===============
idInteraction::Unlink
===============
*/
void idInteraction::Unlink()
{

	// unlink from the entity's list
	if( this->entityPrev )
	{
		this->entityPrev->entityNext = this->entityNext;
	}
	else
	{
		this->entityDef->firstInteraction = this->entityNext;
	}
	if( this->entityNext )
	{
		this->entityNext->entityPrev = this->entityPrev;
	}
	else
	{
		this->entityDef->lastInteraction = this->entityPrev;
	}
	this->entityNext = this->entityPrev = NULL;
	
	// unlink from the light's list
	if( this->lightPrev )
	{
		this->lightPrev->lightNext = this->lightNext;
	}
	else
	{
		this->lightDef->firstInteraction = this->lightNext;
	}
	if( this->lightNext )
	{
		this->lightNext->lightPrev = this->lightPrev;
	}
	else
	{
		this->lightDef->lastInteraction = this->lightPrev;
	}
	this->lightNext = this->lightPrev = NULL;
}

/*
===============
idInteraction::UnlinkAndFree

Removes links and puts it back on the free list.
===============
*/
void idInteraction::UnlinkAndFree()
{
	// clear the table pointer
	idRenderWorldLocal* renderWorld = lightDef->GetOwner();
	// RB: added check for NULL
	if( renderWorld->interactionTable != NULL )
	{
		int index = lightDef->GetIndex() * renderWorld->interactionTableWidth + this->entityDef->GetIndex();
		if( renderWorld->interactionTable[index] != this && renderWorld->interactionTable[index] != INTERACTION_EMPTY )
		{
			common->Error( "idInteraction::UnlinkAndFree: interactionTable wasn't set" );
		}
		renderWorld->interactionTable[index] = NULL;
	}
	// RB end
	
	Unlink();
	
	FreeSurfaces();
	
	// put it back on the free list
	renderWorld->interactionAllocator.Free( this );
}

/*
===============
idInteraction::MakeEmpty

Relinks the interaction at the end of both the light and entity chains
and adds the INTERACTION_EMPTY marker to the interactionTable.

It is necessary to keep the empty interaction so when entities or lights move
they can set all the interactionTable values to NULL.
===============
*/
void idInteraction::MakeEmpty()
{
	// an empty interaction has no surfaces
	numSurfaces = 0;
	
	Unlink();
	
	// relink at the end of the entity's list
	this->entityNext = NULL;
	this->entityPrev = this->entityDef->lastInteraction;
	this->entityDef->lastInteraction = this;
	if( this->entityPrev )
	{
		this->entityPrev->entityNext = this;
	}
	else
	{
		this->entityDef->firstInteraction = this;
	}
	
	// relink at the end of the light's list
	this->lightNext = NULL;
	this->lightPrev = this->lightDef->lastInteraction;
	this->lightDef->lastInteraction = this;
	if( this->lightPrev )
	{
		this->lightPrev->lightNext = this;
	}
	else
	{
		this->lightDef->firstInteraction = this;
	}
	
	// store the special marker in the interaction table
	auto world = entityDef->GetOwner();
	const int interactionIndex = lightDef->GetIndex() * world->interactionTableWidth + entityDef->GetIndex();
	assert( world->interactionTable[ interactionIndex ] == this );
	world->interactionTable[ interactionIndex ] = INTERACTION_EMPTY;
}

/*
===============
idInteraction::HasShadows
===============
*/
bool idInteraction::HasShadows() const
{
	return !entityDef->GetParms().noShadow && lightDef->LightCastsShadows();
}

/*
======================
CreateStaticInteraction

Called by idRenderWorldLocal::GenerateAllInteractions
======================
*/
void idInteraction::CreateStaticInteraction()
{
	// note that it is a static interaction
	isStaticInteraction = true;
	const idRenderModel* model = entityDef->GetModel();
	if( model == NULL || model->NumSurfaces() <= 0 || model->IsDynamicModel() != DM_STATIC )
	{
		MakeEmpty();
		return;
	}
	
	const idBounds bounds = model->Bounds( &entityDef->GetParms() );
	
	// if it doesn't contact the light frustum, none of the surfaces will
	if( lightDef->CullModelBoundsToLight( bounds, entityDef->modelRenderMatrix ) )
	{
		MakeEmpty();
		return;
	}
	
	//
	// create slots for each of the model's surfaces
	//
	numSurfaces = model->NumSurfaces();
	surfaces = allocManager.StaticAlloc<surfaceInteraction_t, TAG_RENDER_INTERACTION, true>( numSurfaces );
	
	bool interactionGenerated = false;
	
	// check each surface in the model
	for( int c = 0; c < model->NumSurfaces(); ++c )
	{
		auto const surf = model->Surface( c );
		const idTriangles* tri = surf->geometry;
		if( tri == NULL )
		{
			continue;
		}
		
		// determine the shader for this surface, possibly by skinning
		// Note that this will be wrong if customSkin/customShader are
		// changed after map load time without invalidating the interaction!
		auto const shader = R_RemapShaderBySkin( surf->shader, entityDef->GetParms().customSkin, entityDef->GetParms().customShader );
		if( shader == NULL )
		{
			continue;
		}
		
		// try to cull each surface
		if( lightDef->CullModelBoundsToLight( tri->GetBounds(), entityDef->modelRenderMatrix ) )
		{
			continue;
		}
		
		surfaceInteraction_t& sint = surfaces[c];
		
		// generate a set of indexes for the lit surfaces, culling away triangles that are
		// not at least partially inside the light
		if( shader->ReceivesLighting() )
		{
			// it is debatable if non-shadowing lights should light back faces. we aren't at the moment
			// RB: now we do with r_useHalfLambert, so don't cull back faces if we have smooth shadowing enabled
			const bool bIncludeBackFaces( r_lightAllBackFaces.GetBool() || lightDef->lightShader->LightEffectsBackSides()
				|| shader->ReceivesLightingOnBackSides() || entityDef->GetParms().noSelfShadow || entityDef->GetParms().noShadow ||
				( r_useHalfLambertLighting.GetInteger() && r_useShadowMapping.GetBool() ) );

			idTriangles* lightTris = R_CreateInteractionLightTris( entityDef, lightDef, bIncludeBackFaces, tri );
			if( lightTris != NULL )
			{
				// make a static index cache
				sint.numLightTrisIndexes = lightTris->numIndexes;
				sint.lightTrisIndexCache = vertexCache.AllocStaticIndex( lightTris->indexes, ALIGN( lightTris->numIndexes * sizeof( lightTris->indexes[0] ), INDEX_CACHE_ALIGN ) );
				
				interactionGenerated = true;
				idTriangles::FreeStatic( lightTris );
			}
		}
		
		// if the interaction has shadows and this surface casts a shadow
		if( HasShadows() && shader->SurfaceCastsShadow() && tri->silEdges != NULL )
		{
			// if the light has an optimized shadow volume, don't create shadows for any models that are part of the base areas
			if( !lightDef->HasPreLightModel() || !model->IsStaticWorldModel() || r_skipPrelightShadows.GetBool() )
			{
				idTriangles* shadowTris = R_CreateInteractionShadowVolume( entityDef, lightDef, tri );
				if( shadowTris != NULL )
				{
					// make a static index cache
					sint.shadowIndexCache = vertexCache.AllocStaticIndex( shadowTris->indexes, ALIGN( shadowTris->numIndexes * sizeof( shadowTris->indexes[0] ), INDEX_CACHE_ALIGN ) );
					sint.numShadowIndexes = shadowTris->numIndexes;
				#if defined( KEEP_INTERACTION_CPU_DATA )
					sint.shadowIndexes = shadowTris->indexes;
					shadowTris->indexes = NULL;
				#endif
					if( shader->Coverage() != MC_OPAQUE )
					{
						// if any surface is a shadow-casting perforated or translucent surface, or the
						// base surface is suppressed in the view (world weapon shadows) we can't use
						// the external shadow optimizations because we can see through some of the faces
						sint.numShadowIndexesNoCaps = shadowTris->numIndexes;
					}
					else
					{
						sint.numShadowIndexesNoCaps = shadowTris->numShadowIndexesNoCaps;
					}
					idTriangles::FreeStatic( shadowTris );
				}
				interactionGenerated = true;
			}
		}
	}
	
	// if none of the surfaces generated anything, don't even bother checking?
	if( !interactionGenerated )
	{
		MakeEmpty();
	}
}

/*
===================
R_ShowInteractionMemory_f
===================
*/
void R_ShowInteractionMemory_f( const idCmdArgs& args )
{
	int entities = 0;
	int interactions = 0;
	int deferredInteractions = 0;
	int emptyInteractions = 0;
	int lightTris = 0;
	int lightTriIndexes = 0;
	int shadowTris = 0;
	int shadowTriIndexes = 0;
	int maxInteractionsForEntity = 0;
	int maxInteractionsForLight = 0;
	
	for( int i = 0; i < tr.primaryWorld->lightDefs.Num(); ++i )
	{
		idRenderLightLocal* light = tr.primaryWorld->lightDefs[i];
		if( light == NULL )
		{
			continue;
		}
		int numInteractionsForLight = 0;
		for( idInteraction* inter = light->firstInteraction; inter != NULL; inter = inter->lightNext )
		{
			if( !inter->IsEmpty() )
			{
				numInteractionsForLight++;
			}
		}
		if( numInteractionsForLight > maxInteractionsForLight )
		{
			maxInteractionsForLight = numInteractionsForLight;
		}
	}
	
	for( int i = 0; i < tr.primaryWorld->entityDefs.Num(); ++i )
	{
		auto def = tr.primaryWorld->entityDefs[ i ];
		if( def == NULL )
		{
			continue;
		}
		if( def->firstInteraction == NULL )
		{
			continue;
		}
		entities++;
		
		int numInteractionsForEntity = 0;
		for( idInteraction* inter = def->firstInteraction; inter != NULL; inter = inter->entityNext )
		{
			interactions++;
			
			if( !inter->IsEmpty() )
			{
				numInteractionsForEntity++;
			}
			
			if( inter->IsDeferred() )
			{
				deferredInteractions++;
				continue;
			}
			if( inter->IsEmpty() )
			{
				emptyInteractions++;
				continue;
			}
			
			for( int j = 0; j < inter->numSurfaces; ++j )
			{
				auto srf = &inter->surfaces[ j ];
				
				if( srf->numLightTrisIndexes )
				{
					lightTris++;
					lightTriIndexes += srf->numLightTrisIndexes;
				}
				
				if( srf->numShadowIndexes )
				{
					shadowTris++;
					shadowTriIndexes += srf->numShadowIndexes;
				}
			}
		}
		if( numInteractionsForEntity > maxInteractionsForEntity )
		{
			maxInteractionsForEntity = numInteractionsForEntity;
		}
	}
	
	common->Printf( "%i entities with %i total interactions\n", entities, interactions );
	common->Printf( "%i deferred interactions, %i empty interactions\n", deferredInteractions, emptyInteractions );
	common->Printf( "%5i indexes in %5i light tris\n", lightTriIndexes, lightTris );
	common->Printf( "%5i indexes in %5i shadow tris\n", shadowTriIndexes, shadowTris );
	common->Printf( "%i maxInteractionsForEntity\n", maxInteractionsForEntity );
	common->Printf( "%i maxInteractionsForLight\n", maxInteractionsForLight );
}
