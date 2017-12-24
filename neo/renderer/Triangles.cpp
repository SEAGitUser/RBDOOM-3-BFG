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
==============================================================================

	TRIANGLE MESH PROCESSING

	The functions in this file have no vertex / index count limits.

	Truly identical vertexes that match in position, normal, and texcoord can
	be merged away.

	Vertexes that match in position and texcoord, but have distinct normals will
	remain distinct for all purposes.  This is usually a poor choice for models,
	as adding a bevel face will not add any more vertexes, and will tend to
	look better.

	Match in position and normal, but differ in texcoords are referenced together
	for calculating tangent vectors for bump mapping.
	Artists should take care to have identical texels in all maps (bump/diffuse/specular)
	in this case

	Vertexes that only match in position are merged for shadow edge finding.

	Degenerate triangles.

	Overlapped triangles, even if normals or texcoords differ, must be removed.
	for the silhoette based stencil shadow algorithm to function properly.
	Is this true???
	Is the overlapped triangle problem just an example of the trippled edge problem?

	Interpenetrating triangles are not currently clipped to surfaces.
	Do they effect the shadows?

	if vertexes are intended to deform apart, make sure that no vertexes
	are on top of each other in the base frame, or the sil edges may be
	calculated incorrectly.

	We might be able to identify this from topology.

	Dangling edges are acceptable, but three way edges are not.

	Are any combinations of two way edges unacceptable, like one facing
	the backside of the other?

	Topology is determined by a collection of triangle indexes.

	The edge list can be built up from this, and stays valid even under
	deformations.

	Somewhat non-intuitively, concave edges cannot be optimized away, or the
	stencil shadow algorithm miscounts.

	Face normals are needed for generating shadow volumes and for calculating
	the silhouette, but they will change with any deformation.

	Vertex normals and vertex tangents will change with each deformation,
	but they may be able to be transformed instead of recalculated.

	bounding volume, both box and sphere will change with deformation.

	silhouette indexes
	shade indexes
	texture indexes

	  shade indexes will only be > silhouette indexes if there is facet shading present

		lookups from texture to sil and texture to shade?

	The normal and tangent vector smoothing is simple averaging, no attempt is
	made to better handle the cases where the distribution around the shared vertex
	is highly uneven.

	  we may get degenerate triangles even with the uniquing and removal
	  if the vertexes have different texcoords.

==============================================================================
*/

// this shouldn't change anything, but previously renderbumped models seem to need it
#define USE_INVA

// instead of using the texture T vector, cross the normal and S vector for an orthogonal axis
#define DERIVE_UNSMOOTHED_BITANGENT

const triIndex_t idTriangles::quadIndexes[ 6 ] = { 0, 1, 2, 2, 3, 0 }; //{ 3, 0, 2, 2, 0, 1 };

/*
=================
R_TriSurfMemory

For memory profiling
=================
*/
size_t idTriangles::CPUMemoryUsed() const
{
	size_t total = 0;
	
	if( this->preLightShadowVertexes != NULL )
	{
		total +=this->numVerts * 2 * sizeof(this->preLightShadowVertexes[0] );
	}
	if(this->staticShadowVertexes != NULL )
	{
		total +=this->numVerts * 2 * sizeof( this->staticShadowVertexes[0] );
	}
	if( this->verts != NULL )
	{
		if( this->baseTriangles == NULL || this->verts != this->baseTriangles->verts )
		{
			total += this->numVerts * sizeof( this->verts[0] );
		}
	}
	if( this->indexes != NULL )
	{
		if( this->baseTriangles == NULL || this->indexes != this->baseTriangles->indexes )
		{
			total += this->numIndexes * sizeof( this->indexes[0] );
		}
	}
	if( this->silIndexes != NULL )
	{
		total += this->numIndexes * sizeof( this->silIndexes[0] );
	}
	if( this->silEdges != NULL )
	{
		total += this->numSilEdges * sizeof( this->silEdges[0] );
	}
	if( this->dominantTris != NULL )
	{
		total += this->numVerts * sizeof( this->dominantTris[0] );
	}
	if( this->mirroredVerts != NULL )
	{
		total += this->numMirroredVerts * sizeof( this->mirroredVerts[0] );
	}
	if( this->dupVerts != NULL )
	{
		total += this->numDupVerts * sizeof( this->dupVerts[0] );
	}
	
	total += sizeof( idTriangles );
	
	return total;
}

/*
==============
R_FreeStaticTriSurfVertexCaches
==============
*/
void idTriangles::FreeStaticVertexCaches()
{
	// we don't support reclaiming static geometry memory
	// without a level change
	this->ambientCache = 0;
	this->indexCache = 0;
	this->shadowCache = 0;
}

/*
==============
R_FreeStaticTriSurf
==============
*/
void idTriangles::FreeStatic( idTriangles* tri )
{
	if( !tri )
	{
		return;
	}
	
	tri->FreeStaticVertexCaches();
	
	if( !tri->referencedVerts )
	{
		if( tri->verts != NULL )
		{
			// R_CreateLightTris points tri->verts at the verts of the ambient surface
			if( tri->baseTriangles == NULL || tri->verts != tri->baseTriangles->verts )
			{
				Mem_Free( tri->verts );
			}
		}
	}
	
	if( !tri->referencedIndexes )
	{
		if( tri->indexes != NULL )
		{
			// if a surface is completely inside a light volume R_CreateLightTris points tri->indexes at the indexes of the ambient surface
			if( tri->baseTriangles == NULL || tri->indexes != tri->baseTriangles->indexes )
			{
				Mem_Free( tri->indexes );
			}
		}
		if( tri->silIndexes != NULL )
		{
			Mem_Free( tri->silIndexes );
		}
		if( tri->silEdges != NULL )
		{
			Mem_Free( tri->silEdges );
		}
		if( tri->dominantTris != NULL )
		{
			Mem_Free( tri->dominantTris );
		}
		if( tri->mirroredVerts != NULL )
		{
			Mem_Free( tri->mirroredVerts );
		}
		if( tri->dupVerts != NULL )
		{
			Mem_Free( tri->dupVerts );
		}
	}
	
	if( tri->preLightShadowVertexes != NULL )
	{
		Mem_Free( tri->preLightShadowVertexes );
	}
	if( tri->staticShadowVertexes != NULL )
	{
		Mem_Free( tri->staticShadowVertexes );
	}
	
	// clear the tri out so we don't retain stale data
	memset( tri, 0, sizeof( idTriangles ) );
	
	allocManager.StaticFree( tri );
}

/*
==============
R_FreeStaticTriSurfVerts
==============
*/
void idTriangles::FreeStaticVerts()
{
	// we don't support reclaiming static geometry memory
	// without a level change
	this->ambientCache = 0;
	
	if( this->verts != NULL )
	{
		// R_CreateLightTris points tri->verts at the verts of the ambient surface
		if( this->baseTriangles == NULL || this->verts != this->baseTriangles->verts )
		{
			Mem_Free( this->verts );
		}
	}
}

/*
==============
R_AllocStaticTriSurf
==============
*/
idTriangles * idTriangles::AllocStatic()
{
	return allocManager.StaticAlloc<idTriangles, TAG_SRFTRIS, true>();
}

/*
=================
R_CopyStaticTriSurf

	This only duplicates the indexes and verts, not any of the derived data.
=================
*/
idTriangles * idTriangles::CopyStatic( const idTriangles* tri )
{
	auto newTri = idTriangles::AllocStatic();

	newTri->AllocStaticVerts( tri->numVerts );
	newTri->AllocStaticIndexes( tri->numIndexes );

	newTri->numVerts = tri->numVerts;
	newTri->numIndexes = tri->numIndexes;

	memcpy( newTri->verts, tri->verts, tri->numVerts * sizeof( newTri->verts[0] ) );
	memcpy( newTri->indexes, tri->indexes, tri->numIndexes * sizeof( newTri->indexes[0] ) );
	
	return newTri;
}

/*
=================
R_AllocStaticTriSurfVerts
=================
*/
void idTriangles::AllocStaticVerts( int numVerts )
{
	assert( this->verts == NULL );
	this->verts = ( idDrawVert* )Mem_Alloc16( numVerts * sizeof( idDrawVert ), TAG_TRI_VERTS );
}

/*
=================
R_AllocStaticTriSurfIndexes
=================
*/
void idTriangles::AllocStaticIndexes( int numIndexes )
{
	assert( this->indexes == NULL );
	this->indexes = ( triIndex_t* )Mem_Alloc16( numIndexes * sizeof( triIndex_t ), TAG_TRI_INDEXES );
}

/*
=================
R_AllocStaticTriSurfSilIndexes
=================
*/
void idTriangles::AllocStaticSilIndexes( int numIndexes )
{
	assert( this->silIndexes == NULL );
	this->silIndexes = ( triIndex_t* )Mem_Alloc16( numIndexes * sizeof( triIndex_t ), TAG_TRI_SIL_INDEXES );
}

/*
=================
R_AllocStaticTriSurfDominantTris
=================
*/
void idTriangles::AllocStaticDominantTris( int numVerts )
{
	assert( this->dominantTris == NULL );
	this->dominantTris = ( dominantTri_t* )Mem_Alloc16( numVerts * sizeof( dominantTri_t ), TAG_TRI_DOMINANT_TRIS );
}

/*
=================
R_AllocStaticTriSurfMirroredVerts
=================
*/
void idTriangles::AllocStaticMirroredVerts( int numMirroredVerts )
{
	assert( this->mirroredVerts == NULL );
	this->mirroredVerts = ( int* )Mem_Alloc16( numMirroredVerts * sizeof( *this->mirroredVerts ), TAG_TRI_MIR_VERT );
}

/*
=================
R_AllocStaticTriSurfDupVerts
=================
*/
void idTriangles::AllocStaticDupVerts( int numDupVerts )
{
	assert( this->dupVerts == NULL );
	this->dupVerts = ( int* )Mem_Alloc16( numDupVerts * 2 * sizeof( *this->dupVerts ), TAG_TRI_DUP_VERT );
}

/*
=================
R_AllocStaticTriSurfSilEdges
=================
*/
void idTriangles::AllocStaticSilEdges( int numSilEdges )
{
	assert( this->silEdges == NULL );
	this->silEdges = ( silEdge_t* )Mem_Alloc16( numSilEdges * sizeof( silEdge_t ), TAG_TRI_SIL_EDGE );
}

/*
=================
R_AllocStaticTriSurfPreLightShadowVerts
=================
*/
void idTriangles::AllocStaticPreLightShadowVerts( int numVerts )
{
	assert( this->preLightShadowVertexes == NULL );
	this->preLightShadowVertexes = ( idShadowVert* )Mem_Alloc16( numVerts * sizeof( idShadowVert ), TAG_TRI_SHADOW );
}

/*
=================
R_ResizeStaticTriSurfVerts
=================
*/
void idTriangles::ResizeStaticVerts( int numVerts )
{
	idDrawVert* newVerts = ( idDrawVert* )Mem_Alloc16( numVerts * sizeof( idDrawVert ), TAG_TRI_VERTS );
	const int copy = std::min( numVerts, this->numVerts );
	memcpy( newVerts, this->verts, copy * sizeof( idDrawVert ) );
	Mem_Free( this->verts );
	this->verts = newVerts;
}

/*
=================
R_ResizeStaticTriSurfIndexes
=================
*/
void idTriangles::ResizeStaticIndexes( int numIndexes )
{
	triIndex_t* newIndexes = ( triIndex_t* )Mem_Alloc16( numIndexes * sizeof( triIndex_t ), TAG_TRI_INDEXES );
	const int copy = std::min( numIndexes, this->numIndexes );
	memcpy( newIndexes, this->indexes, copy * sizeof( triIndex_t ) );
	Mem_Free( this->indexes );
	this->indexes = newIndexes;
}

/*
=================
R_ReferenceStaticTriSurfVerts
=================
*/
void idTriangles::ReferenceStaticVerts( const idTriangles* reference )
{
	this->verts = reference->verts;
}

/*
=================
R_ReferenceStaticTriSurfIndexes
=================
*/
void idTriangles::ReferenceStaticIndexes( const idTriangles* reference )
{
	this->indexes = reference->indexes;
}

/*
=================
R_FreeStaticTriSurfSilIndexes
=================
*/
void idTriangles::FreeStaticSilIndexes()
{
	Mem_Free( this->silIndexes );
	this->silIndexes = NULL;
}

/*
===============
R_RangeCheckIndexes

	Check for syntactically incorrect indexes, like out of range values.
	Does not check for semantics, like degenerate triangles.

	No vertexes is acceptable if no indexes.
	No indexes is acceptable.
	More vertexes than are referenced by indexes are acceptable.
===============
*/
void idTriangles::RangeCheckIndexes() const
{
	if( this->numIndexes < 0 )
	{
		common->Error( "R_RangeCheckIndexes: numIndexes < 0" );
	}
	if( this->numVerts < 0 )
	{
		common->Error( "R_RangeCheckIndexes: numVerts < 0" );
	}
	
	// must specify an integral number of triangles
	if( this->numIndexes % 3 != 0 )
	{
		common->Error( "R_RangeCheckIndexes: numIndexes %% 3" );
	}
	
	for( int i = 0; i < this->numIndexes; i++ )
	{
		if( this->indexes[i] >= this->numVerts )
		{
			common->Error( "R_RangeCheckIndexes: index out of range" );
		}
	}
	
	// this should not be possible unless there are unused verts
	if( this->numVerts > this->numIndexes )
	{
		// FIXME: find the causes of these
		// common->Printf( "R_RangeCheckIndexes: this->numVerts > this->numIndexes\n" );
	}
}

/*
=================
R_BoundTriSurf
=================
*/
void idTriangles::DeriveBounds()
{
	SIMDProcessor->MinMax( this->bounds[0], this->bounds[1], this->verts, this->numVerts );
}

/*
=================
R_CreateSilRemap
=================
*/
static void R_CreateSilRemap( const idTriangles* tri, int *remap )
{
	int	i, j, hashKey;
	const idDrawVert* v1, *v2;

	if( !r_useSilRemap.GetBool() )
	{
		for( i = 0; i < tri->numVerts; ++i )
		{
			remap[i] = i;
		}
		return;
	}
	
	idHashIndex	hash( 1024, tri->numVerts );
	
	int c_removed = 0;
	int c_unique = 0;

	for( i = 0; i < tri->numVerts; ++i )
	{
		v1 = &tri->verts[i];
		
		// see if there is an earlier vert that it can map to
		hashKey = hash.GenerateKey( v1->GetPosition() );
		for( j = hash.First( hashKey ); j >= 0; j = hash.Next( j ) )
		{
			v2 = &tri->verts[j];
			if( v2->GetPosition()[0] == v1->GetPosition()[0] &&
				v2->GetPosition()[1] == v1->GetPosition()[1] && 
				v2->GetPosition()[2] == v1->GetPosition()[2] )
			{
				c_removed++;
				remap[i] = j;
				break;
			}
		}
		if( j < 0 )
		{
			c_unique++;
			remap[i] = i;
			hash.Add( hashKey, i );
		}
	}
}

/*
=================
R_CreateSilIndexes

Uniquing vertexes only on xyz before creating sil edges reduces
the edge count by about 20% on Q3 models
=================
*/
void idTriangles::CreateSilIndexes()
{
	if( this->silIndexes )
	{
		Mem_Free( this->silIndexes );
		this->silIndexes = NULL;
	}

	idTempArray<int> remap( this->numVerts );
	remap.Zero();

	R_CreateSilRemap( this, remap.Ptr() );
	
	// remap indexes to the first one
	this->AllocStaticSilIndexes( this->numIndexes );
	assert( this->silIndexes != NULL );
	for( int i = 0; i < this->numIndexes; ++i )
	{
		this->silIndexes[ i ] = remap[ this->indexes[ i ] ];
	}
}

/*
=====================
R_CreateDupVerts
=====================
*/
void idTriangles::CreateDupVerts()
{
	idTempArray<int> remap( this->numVerts );
	
	// initialize vertex remap in case there are unused verts
	for( int i = 0; i < this->numVerts; ++i )
	{
		remap[i] = i;
	}
	
	// set the remap based on how the silhouette indexes are remapped
	for( int i = 0; i < this->numIndexes; ++i )
	{
		remap[ this->indexes[i]] = this->silIndexes[i];
	}
	
	// create duplicate vertex index based on the vertex remap
	idTempArray<int> tempDupVerts( this->numVerts * 2 );
	this->numDupVerts = 0;
	for( int i = 0; i < this->numVerts; ++i )
	{
		if( remap[i] != i )
		{
			tempDupVerts[ this->numDupVerts * 2 + 0] = i;
			tempDupVerts[ this->numDupVerts * 2 + 1] = remap[i];
			this->numDupVerts++;
		}
	}
	
	this->AllocStaticDupVerts( this->numDupVerts );
	memcpy( this->dupVerts, tempDupVerts.Ptr(), this->numDupVerts * 2 * sizeof( this->dupVerts[0] ) );
}

/*
===============
R_DefineEdge
===============
*/
static int c_duplicatedEdges, c_tripledEdges;
static const int MAX_SIL_EDGES = 0x7ffff;

static void R_DefineEdge( const int v1, const int v2, const int planeNum, const int numPlanes, idList<silEdge_t>& silEdges, idHashIndex&	 silEdgeHash )
{
	int	i, hashKey;
	
	// check for degenerate edge
	if( v1 == v2 )
	{
		return;
	}
	hashKey = silEdgeHash.GenerateKey( v1, v2 );
	// search for a matching other side
	for( i = silEdgeHash.First( hashKey ); i >= 0 && i < MAX_SIL_EDGES; i = silEdgeHash.Next( i ) )
	{
		if( silEdges[i].v1 == v1 && silEdges[i].v2 == v2 )
		{
			c_duplicatedEdges++;
			// allow it to still create a new edge
			continue;
		}
		if( silEdges[i].v2 == v1 && silEdges[i].v1 == v2 )
		{
			if( silEdges[i].p2 != numPlanes )
			{
				c_tripledEdges++;
				// allow it to still create a new edge
				continue;
			}
			// this is a matching back side
			silEdges[i].p2 = planeNum;
			return;
		}
		
	}
	
	// define the new edge
	silEdgeHash.Add( hashKey, silEdges.Num() );
	
	silEdge_t silEdge;
	
	silEdge.p1 = planeNum;
	silEdge.p2 = numPlanes;
	silEdge.v1 = v1;
	silEdge.v2 = v2;
	
	silEdges.Append( silEdge );
}

/*
=================
SilEdgeSort
=================
*/
static int SilEdgeSort( const void* a, const void* b )
{
	if( ( ( silEdge_t* )a )->p1 < ( ( silEdge_t* )b )->p1 )
	{
		return -1;
	}
	if( ( ( silEdge_t* )a )->p1 > ( ( silEdge_t* )b )->p1 )
	{
		return 1;
	}
	if( ( ( silEdge_t* )a )->p2 < ( ( silEdge_t* )b )->p2 )
	{
		return -1;
	}
	if( ( ( silEdge_t* )a )->p2 > ( ( silEdge_t* )b )->p2 )
	{
		return 1;
	}
	return 0;
}

/*
=================
R_IdentifySilEdges

	If the surface will not deform, coplanar edges (polygon interiors)
	can never create silhouette plains, and can be omited
=================
*/
int	c_coplanarSilEdges;
int	c_totalSilEdges;
void idTriangles::IdentifySilEdges( bool omitCoplanarEdges )
{
	int	i;
	int	shared, single;
	
	omitCoplanarEdges = false;	// optimization doesn't work for some reason
	
	static const int SILEDGE_HASH_SIZE		= 1024;
	
	const int numTris = this->numIndexes / 3;
	
	idList<silEdge_t>	silEdges( MAX_SIL_EDGES );
	idHashIndex	silEdgeHash( SILEDGE_HASH_SIZE, MAX_SIL_EDGES );
	int	numPlanes = numTris;
		
	silEdgeHash.Clear();
	
	c_duplicatedEdges = 0;
	c_tripledEdges = 0;
	
	for( i = 0; i < numTris; i++ )
	{
		int i1 = this->silIndexes[ i * 3 + 0 ];
		int i2 = this->silIndexes[ i * 3 + 1 ];
		int i3 = this->silIndexes[ i * 3 + 2 ];
		
		// create the edges
		R_DefineEdge( i1, i2, i, numPlanes, silEdges, silEdgeHash );
		R_DefineEdge( i2, i3, i, numPlanes, silEdges, silEdgeHash );
		R_DefineEdge( i3, i1, i, numPlanes, silEdges, silEdgeHash );
	}
	
	if( c_duplicatedEdges || c_tripledEdges )
	{
		common->DWarning( "%i duplicated edge directions, %i tripled edges", c_duplicatedEdges, c_tripledEdges );
	}
	
	// if we know that the vertexes aren't going
	// to deform, we can remove interior triangulation edges
	// on otherwise planar polygons.
	// I earlier believed that I could also remove concave
	// edges, because they are never silhouettes in the conventional sense,
	// but they are still needed to balance out all the true sil edges
	// for the shadow algorithm to function
	int c_coplanarCulled = 0;
	if( omitCoplanarEdges )
	{
		for( i = 0; i < silEdges.Num(); i++ )
		{
			int			i1, i2, i3;
			idPlane		plane;
			int			base;
			int			j;
			float		d;
			
			if( silEdges[i].p2 == numPlanes )  	// the fake dangling edge
			{
				continue;
			}
			
			base = silEdges[i].p1 * 3;
			i1 = this->silIndexes[ base + 0 ];
			i2 = this->silIndexes[ base + 1 ];
			i3 = this->silIndexes[ base + 2 ];
			
			plane.FromPoints( this->verts[i1].GetPosition(), this->verts[i2].GetPosition(), this->verts[i3].GetPosition() );
			
			// check to see if points of second triangle are not coplanar
			base = silEdges[i].p2 * 3;
			for( j = 0; j < 3; j++ )
			{
				i1 = this->silIndexes[ base + j ];
				d = plane.Distance( this->verts[i1].GetPosition() );
				if( d != 0 )  		// even a small epsilon causes problems
				{
					break;
				}
			}
			
			if( j == 3 )
			{
				// we can cull this sil edge
				memmove( &silEdges[i], &silEdges[i + 1], ( silEdges.Num() - i - 1 ) * sizeof( silEdges[i] ) );
				c_coplanarCulled++;
				silEdges.SetNum( silEdges.Num() - 1 );
				i--;
			}
		}
		if( c_coplanarCulled )
		{
			c_coplanarSilEdges += c_coplanarCulled;
//			common->Printf( "%i of %i sil edges coplanar culled\n", c_coplanarCulled,
//				c_coplanarCulled + numSilEdges );
		}
	}
	c_totalSilEdges += silEdges.Num();
	
	// sort the sil edges based on plane number
	qsort( silEdges.Ptr(), silEdges.Num(), sizeof( silEdges[0] ), SilEdgeSort );
	
	// count up the distribution.
	// a perfectly built model should only have shared
	// edges, but most models will have some interpenetration
	// and dangling edges
	shared = 0;
	single = 0;
	for( i = 0; i < silEdges.Num(); i++ )
	{
		if( silEdges[i].p2 == numPlanes )
		{
			single++;
		}
		else
		{
			shared++;
		}
	}
	
	if( !single )
	{
		this->perfectHull = true;
	}
	else
	{
		this->perfectHull = false;
	}
	
	this->numSilEdges = silEdges.Num();
	this->AllocStaticSilEdges( silEdges.Num() );
	memcpy( this->silEdges, silEdges.Ptr(), silEdges.Num() * sizeof( this->silEdges[0] ) );
}

/*
===============
R_FaceNegativePolarity

	Returns true if the texture polarity of the face is negative, false if it is positive or zero
===============
*/
static bool R_FaceNegativePolarity( const idTriangles* tri, int firstIndex )
{
	const idDrawVert* a = tri->verts + tri->indexes[firstIndex + 0];
	const idDrawVert* b = tri->verts + tri->indexes[firstIndex + 1];
	const idDrawVert* c = tri->verts + tri->indexes[firstIndex + 2];
	
	const idVec2 aST = a->GetTexCoord();
	const idVec2 bST = b->GetTexCoord();
	const idVec2 cST = c->GetTexCoord();
	
	float d0[5];
	d0[3] = bST[0] - aST[0];
	d0[4] = bST[1] - aST[1];
	
	float d1[5];
	d1[3] = cST[0] - aST[0];
	d1[4] = cST[1] - aST[1];
	
	const float area = d0[3] * d1[4] - d0[4] * d1[3];
	if( area >= 0 )
	{
		return false;
	}
	return true;
}

/*
===================
R_DuplicateMirroredVertexes

	Modifies the surface to bust apart any verts that are shared by both positive and
	negative texture polarities, so tangent space smoothing at the vertex doesn't
	degenerate.

	This will create some identical vertexes (which will eventually get different tangent
	vectors), so never optimize the resulting mesh, or it will get the mirrored edges back.

	Reallocates tri->verts and changes tri->indexes in place
	Silindexes are unchanged by this.

	sets mirroredVerts and mirroredVerts[]
===================
*/
struct tangentVert_t
{
	bool	polarityUsed[2];
	int		negativeRemap;
};

void idTriangles::DuplicateMirroredVertexes()
{
	tangentVert_t*	vert;
	int				i, j;
	int				totalVerts;
	int				numMirror;

	idTempArray<tangentVert_t> tverts( this->numVerts );
	tverts.Zero();

	// determine texture polarity of each surface

	// mark each vert with the polarities it uses
	for( i = 0; i < this->numIndexes; i += 3 )
	{
		int	polarity = R_FaceNegativePolarity( this, i );
		for( j = 0; j < 3; j++ )
		{
			tverts[ this->indexes[ i + j ] ].polarityUsed[ polarity ] = true;
		}
	}

	// now create new vertex indices as needed
	totalVerts = this->numVerts;
	for( i = 0; i < this->numVerts; i++ )
	{
		vert = &tverts[ i ];
		if( vert->polarityUsed[ 0 ] && vert->polarityUsed[ 1 ] )
		{
			vert->negativeRemap = totalVerts;
			totalVerts++;
		}
	}

	this->numMirroredVerts = totalVerts - this->numVerts;

	if( this->numMirroredVerts == 0 )
	{
		this->mirroredVerts = NULL;
		return;
	}

	// now create the new list
	this->AllocStaticMirroredVerts( this->numMirroredVerts );
	this->ResizeStaticVerts( totalVerts );

	// create the duplicates
	numMirror = 0;
	for( i = 0; i < this->numVerts; ++i )
	{
		j = tverts[ i ].negativeRemap;
		if( j )
		{
			this->verts[ j ] = this->verts[ i ];
			this->mirroredVerts[ numMirror ] = i;
			numMirror++;
		}
	}
	this->numVerts = totalVerts;

	// change the indexes
	for( i = 0; i < this->numIndexes; ++i )
	{
		if( tverts[ this->indexes[ i ] ].negativeRemap && R_FaceNegativePolarity( this, 3 * ( i / 3 ) ) )
		{
			this->indexes[ i ] = tverts[ this->indexes[ i ] ].negativeRemap;
		}
	}
}

/*
============
R_DeriveNormalsAndTangents

	Derives the normal and orthogonal tangent vectors for the triangle vertices.
	For each vertex the normal and tangent vectors are derived from all triangles
	using the vertex which results in smooth tangents across the mesh.
============
*/
void idTriangles::DeriveNormalsAndTangents()
{
	idTempArray< idVec3 > vertexNormals( this->numVerts );
	idTempArray< idVec3 > vertexTangents( this->numVerts );
	idTempArray< idVec3 > vertexBitangents( this->numVerts );
	
	vertexNormals.Zero();
	vertexTangents.Zero();
	vertexBitangents.Zero();
	
	for( int i = 0; i < this->numIndexes; i += 3 )
	{
		const int v0 = this->indexes[i + 0];
		const int v1 = this->indexes[i + 1];
		const int v2 = this->indexes[i + 2];
		
		const idDrawVert* a = this->verts + v0;
		const idDrawVert* b = this->verts + v1;
		const idDrawVert* c = this->verts + v2;
		
		const idVec2 aST = a->GetTexCoord();
		const idVec2 bST = b->GetTexCoord();
		const idVec2 cST = c->GetTexCoord();

		const idVec3 & a_pos = a->GetPosition();
		const idVec3 & b_pos = b->GetPosition();
		const idVec3 & c_pos = c->GetPosition();
		
		float d0[5];
		d0[0] = b_pos[0] - a_pos[0];
		d0[1] = b_pos[1] - a_pos[1];
		d0[2] = b_pos[2] - a_pos[2];
		d0[3] = bST[0] - aST[0];
		d0[4] = bST[1] - aST[1];
		
		float d1[5];
		d1[0] = c_pos[0] - a_pos[0];
		d1[1] = c_pos[1] - a_pos[1];
		d1[2] = c_pos[2] - a_pos[2];
		d1[3] = cST[0] - aST[0];
		d1[4] = cST[1] - aST[1];
		
		idVec3 normal;
		normal[0] = d1[1] * d0[2] - d1[2] * d0[1];
		normal[1] = d1[2] * d0[0] - d1[0] * d0[2];
		normal[2] = d1[0] * d0[1] - d1[1] * d0[0];
		
		const float f0 = idMath::InvSqrt( normal.x * normal.x + normal.y * normal.y + normal.z * normal.z );
		
		normal.x *= f0;
		normal.y *= f0;
		normal.z *= f0;
		
		// area sign bit
		const float area = d0[3] * d1[4] - d0[4] * d1[3];
		unsigned int signBit = ( *( unsigned int* )&area ) & ( 1 << 31 );
		
		idVec3 tangent;
		tangent[0] = d0[0] * d1[4] - d0[4] * d1[0];
		tangent[1] = d0[1] * d1[4] - d0[4] * d1[1];
		tangent[2] = d0[2] * d1[4] - d0[4] * d1[2];
		
		const float f1 = idMath::InvSqrt( tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z );
		*( unsigned int* )&f1 ^= signBit;
		
		tangent.x *= f1;
		tangent.y *= f1;
		tangent.z *= f1;
		
		idVec3 bitangent;
		bitangent[0] = d0[3] * d1[0] - d0[0] * d1[3];
		bitangent[1] = d0[3] * d1[1] - d0[1] * d1[3];
		bitangent[2] = d0[3] * d1[2] - d0[2] * d1[3];
		
		const float f2 = idMath::InvSqrt( bitangent.x * bitangent.x + bitangent.y * bitangent.y + bitangent.z * bitangent.z );
		*( unsigned int* )&f2 ^= signBit;
		
		bitangent.x *= f2;
		bitangent.y *= f2;
		bitangent.z *= f2;
		
		vertexNormals[v0] += normal;
		vertexTangents[v0] += tangent;
		vertexBitangents[v0] += bitangent;
		
		vertexNormals[v1] += normal;
		vertexTangents[v1] += tangent;
		vertexBitangents[v1] += bitangent;
		
		vertexNormals[v2] += normal;
		vertexTangents[v2] += tangent;
		vertexBitangents[v2] += bitangent;
	}
	
	// add the normal of a duplicated vertex to the normal of the first vertex with the same XYZ
	for( int i = 0; i < this->numDupVerts; ++i )
	{
		vertexNormals[this->dupVerts[i * 2 + 0]] += vertexNormals[this->dupVerts[i * 2 + 1]];
	}
	
	// copy vertex normals to duplicated vertices
	for( int i = 0; i < this->numDupVerts; ++i )
	{
		vertexNormals[this->dupVerts[i * 2 + 1]] = vertexNormals[this->dupVerts[i * 2 + 0]];
	}
	
	// Project the summed vectors onto the normal plane and normalize.
	// The tangent vectors will not necessarily be orthogonal to each
	// other, but they will be orthogonal to the surface normal.
	for( int i = 0; i < this->numVerts; ++i )
	{
		const float normalScale = idMath::InvSqrt( vertexNormals[i].x * vertexNormals[i].x + vertexNormals[i].y * vertexNormals[i].y + vertexNormals[i].z * vertexNormals[i].z );
		vertexNormals[i].x *= normalScale;
		vertexNormals[i].y *= normalScale;
		vertexNormals[i].z *= normalScale;
		
		vertexTangents[i] -= ( vertexTangents[i] * vertexNormals[i] ) * vertexNormals[i];
		vertexBitangents[i] -= ( vertexBitangents[i] * vertexNormals[i] ) * vertexNormals[i];
		
		const float tangentScale = idMath::InvSqrt( vertexTangents[i].x * vertexTangents[i].x + vertexTangents[i].y * vertexTangents[i].y + vertexTangents[i].z * vertexTangents[i].z );
		vertexTangents[i].x *= tangentScale;
		vertexTangents[i].y *= tangentScale;
		vertexTangents[i].z *= tangentScale;
		
		const float bitangentScale = idMath::InvSqrt( vertexBitangents[i].x * vertexBitangents[i].x + vertexBitangents[i].y * vertexBitangents[i].y + vertexBitangents[i].z * vertexBitangents[i].z );
		vertexBitangents[i].x *= bitangentScale;
		vertexBitangents[i].y *= bitangentScale;
		vertexBitangents[i].z *= bitangentScale;
	}
	
	// compress the normals and tangents
	for( int i = 0; i < this->numVerts; ++i )
	{
		this->verts[ i ].SetNormal( vertexNormals[ i ] );
		this->verts[ i ].SetTangent( vertexTangents[ i ] );
		this->verts[ i ].SetBiTangent( vertexBitangents[ i ] );
	}
}

/*
============
R_DeriveUnsmoothedNormalsAndTangents
============
*/
void idTriangles::DeriveUnsmoothedNormalsAndTangents()
{
	for( int i = 0; i < this->numVerts; i++ )
	{
		float d0, d1, d2, d3, d4;
		float d5, d6, d7, d8, d9;
		float s0, s1, s2;
		float n0, n1, n2;
		float t0, t1, t2;
		float t3, t4, t5;
		
		const dominantTri_t& dt = this->dominantTris[i];
		
		idDrawVert* a = this->verts + i;
		idDrawVert* b = this->verts + dt.v2;
		idDrawVert* c = this->verts + dt.v3;
		
		const idVec2 aST = a->GetTexCoord();
		const idVec2 bST = b->GetTexCoord();
		const idVec2 cST = c->GetTexCoord();

		const idVec3 & a_pos = a->GetPosition();
		const idVec3 & b_pos = b->GetPosition();
		const idVec3 & c_pos = c->GetPosition();
		
		d0 = b_pos[0] - a_pos[0];
		d1 = b_pos[1] - a_pos[1];
		d2 = b_pos[2] - a_pos[2];
		d3 = bST[0] - aST[0];
		d4 = bST[1] - aST[1];
		
		d5 = c_pos[0] - a_pos[0];
		d6 = c_pos[1] - a_pos[1];
		d7 = c_pos[2] - a_pos[2];
		d8 = cST[0] - aST[0];
		d9 = cST[1] - aST[1];
		
		s0 = dt.normalizationScale[0];
		s1 = dt.normalizationScale[1];
		s2 = dt.normalizationScale[2];
		
		n0 = s2 * ( d6 * d2 - d7 * d1 );
		n1 = s2 * ( d7 * d0 - d5 * d2 );
		n2 = s2 * ( d5 * d1 - d6 * d0 );
		
		t0 = s0 * ( d0 * d9 - d4 * d5 );
		t1 = s0 * ( d1 * d9 - d4 * d6 );
		t2 = s0 * ( d2 * d9 - d4 * d7 );
		
#ifndef DERIVE_UNSMOOTHED_BITANGENT
		t3 = s1 * ( d3 * d5 - d0 * d8 );
		t4 = s1 * ( d3 * d6 - d1 * d8 );
		t5 = s1 * ( d3 * d7 - d2 * d8 );
#else
		t3 = s1 * ( n2 * t1 - n1 * t2 );
		t4 = s1 * ( n0 * t2 - n2 * t0 );
		t5 = s1 * ( n1 * t0 - n0 * t1 );
#endif
		
		a->SetNormal( n0, n1, n2 );
		a->SetTangent( t0, t1, t2 );
		a->SetBiTangent( t3, t4, t5 );
	}
}

/*
=====================
R_CreateVertexNormals

	Averages together the contributions of all faces that are
	used by a vertex, creating drawVert->normal
=====================
*/
void idTriangles::CreateVertexNormals()
{
	if( this->silIndexes == NULL )
	{
		this->CreateSilIndexes();
	}
	
	idTempArray< idVec3 > vertexNormals( this->numVerts );
	vertexNormals.Zero();
	
	assert( this->silIndexes != NULL );
	for( int i = 0; i < this->numIndexes; i += 3 )
	{
		const int i0 = this->silIndexes[ i + 0 ];
		const int i1 = this->silIndexes[ i + 1 ];
		const int i2 = this->silIndexes[ i + 2 ];

		const idDrawVert& v0 = this->verts[ i0 ];
		const idDrawVert& v1 = this->verts[ i1 ];
		const idDrawVert& v2 = this->verts[ i2 ];

		const idPlane plane( v0.GetPosition(), v1.GetPosition(), v2.GetPosition() );

		vertexNormals[ i0 ] += plane.Normal();
		vertexNormals[ i1 ] += plane.Normal();
		vertexNormals[ i2 ] += plane.Normal();
	}
	
	// replicate from silIndexes to all indexes
	for( int i = 0; i < this->numIndexes; i++ )
	{
		vertexNormals[this->indexes[i]] = vertexNormals[this->silIndexes[i]];
	}
	
	// normalize
	for( int i = 0; i < this->numVerts; i++ )
	{
		vertexNormals[i].Normalize();
	}
	
	// compress the normals
	for( int i = 0; i < this->numVerts; i++ )
	{
		this->verts[i].SetNormal( vertexNormals[i] );
	}
}

/*
=================
R_DeriveTangentsWithoutNormals

	Build texture space tangents for bump mapping
	If a surface is deformed, this must be recalculated

	This assumes that any mirrored vertexes have already been duplicated, so
	any shared vertexes will have the tangent spaces smoothed across.

	Texture wrapping slightly complicates this, but as long as the normals
	are shared, and the tangent vectors are projected onto the normals, the
	separate vertexes should wind up with identical tangent spaces.

	mirroring a normalmap WILL cause a slightly visible seam unless the normals
	are completely flat around the edge's full bilerp support.

	Vertexes which are smooth shaded must have their tangent vectors
	in the same plane, which will allow a seamless
	rendering as long as the normal map is even on both sides of the
	seam.

	A smooth shaded surface may have multiple tangent vectors at a vertex
	due to texture seams or mirroring, but it should only have a single
	normal vector.

	Each triangle has a pair of tangent vectors in it's plane

	Should we consider having vertexes point at shared tangent spaces
	to save space or speed transforms?

	this version only handles bilateral symetry
=================
*/
void idTriangles::DeriveTangentsWithoutNormals()
{
	idTempArray< idVec3 > triangleTangents( this->numIndexes / 3 );
	idTempArray< idVec3 > triangleBitangents( this->numIndexes / 3 );
	
	//
	// calculate tangent vectors for each face in isolation
	//
	int c_positive = 0;
	int c_negative = 0;
	int c_textureDegenerateFaces = 0;
	for( int i = 0; i < this->numIndexes; i += 3 )
	{
		idVec3	temp;
		
		idDrawVert* a = this->verts + this->indexes[i + 0];
		idDrawVert* b = this->verts + this->indexes[i + 1];
		idDrawVert* c = this->verts + this->indexes[i + 2];
		
		const idVec2 aST = a->GetTexCoord();
		const idVec2 bST = b->GetTexCoord();
		const idVec2 cST = c->GetTexCoord();

		const idVec3 & a_pos = a->GetPosition();
		const idVec3 & b_pos = b->GetPosition();
		const idVec3 & c_pos = c->GetPosition();
		
		float d0[5];
		d0[0] = b_pos[0] - a_pos[0];
		d0[1] = b_pos[1] - a_pos[1];
		d0[2] = b_pos[2] - a_pos[2];
		d0[3] = bST[0] - aST[0];
		d0[4] = bST[1] - aST[1];
		
		float d1[5];
		d1[0] = c_pos[0] - a_pos[0];
		d1[1] = c_pos[1] - a_pos[1];
		d1[2] = c_pos[2] - a_pos[2];
		d1[3] = cST[0] - aST[0];
		d1[4] = cST[1] - aST[1];
		
		const float area = d0[3] * d1[4] - d0[4] * d1[3];
		if( idMath::Fabs( area ) < 1e-20f )
		{
			triangleTangents[i / 3].Zero();
			triangleBitangents[i / 3].Zero();
			c_textureDegenerateFaces++;
			continue;
		}
		if( area > 0.0f )
		{
			c_positive++;
		}
		else
		{
			c_negative++;
		}
		
#ifdef USE_INVA
		float inva = ( area < 0.0f ) ? -1.0f : 1.0f;		// was = 1.0f / area;
		
		temp[0] = ( d0[0] * d1[4] - d0[4] * d1[0] ) * inva;
		temp[1] = ( d0[1] * d1[4] - d0[4] * d1[1] ) * inva;
		temp[2] = ( d0[2] * d1[4] - d0[4] * d1[2] ) * inva;
		temp.Normalize();
		triangleTangents[i / 3] = temp;
		
		temp[0] = ( d0[3] * d1[0] - d0[0] * d1[3] ) * inva;
		temp[1] = ( d0[3] * d1[1] - d0[1] * d1[3] ) * inva;
		temp[2] = ( d0[3] * d1[2] - d0[2] * d1[3] ) * inva;
		temp.Normalize();
		triangleBitangents[i / 3] = temp;
#else
		temp[0] = ( d0[0] * d1[4] - d0[4] * d1[0] );
		temp[1] = ( d0[1] * d1[4] - d0[4] * d1[1] );
		temp[2] = ( d0[2] * d1[4] - d0[4] * d1[2] );
		temp.Normalize();
		triangleTangents[i / 3] = temp;
		
		temp[0] = ( d0[3] * d1[0] - d0[0] * d1[3] );
		temp[1] = ( d0[3] * d1[1] - d0[1] * d1[3] );
		temp[2] = ( d0[3] * d1[2] - d0[2] * d1[3] );
		temp.Normalize();
		triangleBitangents[i / 3] = temp;
#endif
	}
	
	idTempArray< idVec3 > vertexTangents( this->numVerts );
	idTempArray< idVec3 > vertexBitangents( this->numVerts );
	
	// clear the tangents
	for( int i = 0; i < this->numVerts; ++i )
	{
		vertexTangents[i].Zero();
		vertexBitangents[i].Zero();
	}
	
	// sum up the neighbors
	for( int i = 0; i < this->numIndexes; i += 3 )
	{
		// for each vertex on this face
		for( int j = 0; j < 3; j++ )
		{
			vertexTangents[this->indexes[i + j]] += triangleTangents[i / 3];
			vertexBitangents[this->indexes[i + j]] += triangleBitangents[i / 3];
		}
	}
	
	// Project the summed vectors onto the normal plane and normalize.
	// The tangent vectors will not necessarily be orthogonal to each
	// other, but they will be orthogonal to the surface normal.
	for( int i = 0; i < this->numVerts; ++i )
	{
		idVec3 normal = this->verts[i].GetNormal();
		normal.Normalize();
		
		vertexTangents[i] -= ( vertexTangents[i] * normal ) * normal;
		vertexTangents[i].Normalize();
		
		vertexBitangents[i] -= ( vertexBitangents[i] * normal ) * normal;
		vertexBitangents[i].Normalize();
	}
	
	for( int i = 0; i < this->numVerts; ++i )
	{
		this->verts[i].SetTangent( vertexTangents[i] );
		this->verts[i].SetBiTangent( vertexBitangents[i] );
	}
	
	this->tangentsCalculated = true;
}

/*
===================
R_BuildDominantTris

	Find the largest triangle that uses each vertex
===================
*/
struct indexSort_t
{
	int		vertexNum;
	int		faceNum;
};

static int IndexSort( const void* a, const void* b )
{
	if( ( ( indexSort_t* )a )->vertexNum < ( ( indexSort_t* )b )->vertexNum )
	{
		return -1;
	}
	if( ( ( indexSort_t* )a )->vertexNum > ( ( indexSort_t* )b )->vertexNum )
	{
		return 1;
	}
	return 0;
}

void idTriangles::BuildDominantTris()
{
	int i, j;

	idTempArray<indexSort_t> ind( this->numIndexes );
	if( ind.Ptr() == NULL )
	{
		idLib::Error( "Couldn't allocate index sort array" );
		return;
	}
	
	for( i = 0; i < this->numIndexes; ++i )
	{
		ind[i].vertexNum = this->indexes[i];
		ind[i].faceNum = i / 3;
	}
	qsort( ind.Ptr(), this->numIndexes, sizeof( indexSort_t ), IndexSort );
	
	this->AllocStaticDominantTris( this->numVerts );
	dominantTri_t * dt = this->dominantTris;
	memset( dt, 0, this->numVerts * sizeof( dt[0] ) );
	
	for( i = 0; i < this->numIndexes; i += j )
	{
		float	maxArea = 0;
#pragma warning( disable: 6385 ) // This is simply to get pass a false defect for /analyze -- if you can figure out a better way, please let Shawn know...
		int		vertNum = ind[i].vertexNum;
#pragma warning( default: 6385 )
		for( j = 0; i + j < this->numIndexes && ind[i + j].vertexNum == vertNum; ++j )
		{
			float	d0[5], d1[5];
			idDrawVert*	a, *b, *c;
			idVec3	normal, tangent, bitangent;
			
			int	i1 = this->indexes[ ind[ i + j ].faceNum * 3 + 0 ];
			int	i2 = this->indexes[ ind[ i + j ].faceNum * 3 + 1 ];
			int	i3 = this->indexes[ ind[ i + j ].faceNum * 3 + 2 ];
			
			a = this->verts + i1;
			b = this->verts + i2;
			c = this->verts + i3;
			
			const idVec2 aST = a->GetTexCoord();
			const idVec2 bST = b->GetTexCoord();
			const idVec2 cST = c->GetTexCoord();

			const idVec3 & a_pos = a->GetPosition();
			const idVec3 & b_pos = b->GetPosition();
			const idVec3 & c_pos = c->GetPosition();
			
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
			
			normal[0] = ( d1[1] * d0[2] - d1[2] * d0[1] );
			normal[1] = ( d1[2] * d0[0] - d1[0] * d0[2] );
			normal[2] = ( d1[0] * d0[1] - d1[1] * d0[0] );
			
			float area = normal.Length();
			
			// if this is smaller than what we already have, skip it
			if( area < maxArea )
			{
				continue;
			}
			maxArea = area;
			
			if( i1 == vertNum )
			{
				dt[vertNum].v2 = i2;
				dt[vertNum].v3 = i3;
			}
			else if( i2 == vertNum )
			{
				dt[vertNum].v2 = i3;
				dt[vertNum].v3 = i1;
			}
			else
			{
				dt[vertNum].v2 = i1;
				dt[vertNum].v3 = i2;
			}
			
			float	len = area;
			if( len < 0.001f )
			{
				len = 0.001f;
			}
			dt[vertNum].normalizationScale[2] = 1.0f / len;		// normal
			
			// texture area
			area = d0[3] * d1[4] - d0[4] * d1[3];
			
			tangent[0] = ( d0[0] * d1[4] - d0[4] * d1[0] );
			tangent[1] = ( d0[1] * d1[4] - d0[4] * d1[1] );
			tangent[2] = ( d0[2] * d1[4] - d0[4] * d1[2] );
			len = tangent.Length();
			if( len < 0.001f )
			{
				len = 0.001f;
			}
			dt[vertNum].normalizationScale[0] = ( area > 0 ? 1 : -1 ) / len;	// tangents[0]
			
			bitangent[0] = ( d0[3] * d1[0] - d0[0] * d1[3] );
			bitangent[1] = ( d0[3] * d1[1] - d0[1] * d1[3] );
			bitangent[2] = ( d0[3] * d1[2] - d0[2] * d1[3] );
			len = bitangent.Length();
			if( len < 0.001f )
			{
				len = 0.001f;
			}
		#ifdef DERIVE_UNSMOOTHED_BITANGENT
			dt[vertNum].normalizationScale[1] = ( area > 0 ? 1 : -1 );
		#else
			dt[vertNum].normalizationScale[1] = ( area > 0 ? 1 : -1 ) / len;	// tangents[1]
		#endif
		}
	}
}

/*
==================
 R_DeriveTangents

	This is called once for static surfaces, and every frame for deforming surfaces

	Builds tangents, normals, and face planes
==================
*/
void idTriangles::DeriveTangents()
{
	if( this->tangentsCalculated )
	{
		return;
	}
	
	tr.pc.c_tangentIndexes += this->numIndexes;
	
	if( this->dominantTris != NULL )
	{
		DeriveUnsmoothedNormalsAndTangents();
	}
	else
	{
		DeriveNormalsAndTangents();
	}
	this->tangentsCalculated = true;
}

/*
=================
 R_RemoveDuplicatedTriangles

	silIndexes must have already been calculated

	silIndexes are used instead of indexes, because duplicated
	triangles could have different texture coordinates.
=================
*/
void idTriangles::RemoveDuplicatedTriangles()
{
	int	i, j, r;
	int	a, b, c;
	
	int c_removed = 0;
	
	// check for completely duplicated triangles
	// any rotation of the triangle is still the same, but a mirroring
	// is considered different
	for( i = 0; i < this->numIndexes; i += 3 )
	{
		for( r = 0; r < 3; r++ )
		{
			a = this->silIndexes[i + r];
			b = this->silIndexes[i + ( r + 1 ) % 3];
			c = this->silIndexes[i + ( r + 2 ) % 3];
			for( j = i + 3; j < this->numIndexes; j += 3 )
			{
				if( this->silIndexes[j] == a && this->silIndexes[j + 1] == b && this->silIndexes[j + 2] == c )
				{
					c_removed++;
					memmove( this->indexes + j, this->indexes + j + 3, ( this->numIndexes - j - 3 ) * sizeof( this->indexes[0] ) );
					memmove( this->silIndexes + j, this->silIndexes + j + 3, ( this->numIndexes - j - 3 ) * sizeof( this->silIndexes[0] ) );
					this->numIndexes -= 3;
					j -= 3;
				}
			}
		}
	}
	
	if( c_removed )
	{
		common->Printf( "removed %i duplicated triangles\n", c_removed );
	}
}

/*
=================
R_RemoveDegenerateTriangles

silIndexes must have already been calculated
=================
*/
void idTriangles::RemoveDegenerateTriangles()
{
	int c_removed = 0;
	int i;
	int a, b, c;
	
	assert( this->silIndexes != NULL );
	
	// check for completely degenerate triangles
	for( i = 0; i < this->numIndexes; i += 3 )
	{
		a = this->silIndexes[i];
		b = this->silIndexes[i + 1];
		c = this->silIndexes[i + 2];
		if( a == b || a == c || b == c )
		{
			c_removed++;
			memmove( this->indexes + i, this->indexes + i + 3, ( this->numIndexes - i - 3 ) * sizeof( this->indexes[0] ) );
			memmove( this->silIndexes + i, this->silIndexes + i + 3, ( this->numIndexes - i - 3 ) * sizeof( this->silIndexes[0] ) );
			this->numIndexes -= 3;
			i -= 3;
		}
	}
	
	// this doesn't free the memory used by the unused verts
	
	if( c_removed )
	{
		common->Printf( "removed %i degenerate triangles\n", c_removed );
	}
}

/*
=================
R_TestDegenerateTextureSpace
=================
*/
static void R_TestDegenerateTextureSpace( idTriangles* tri )
{
	// check for triangles with a degenerate texture space
	int c_degenerate = 0;
	for( int i = 0; i < tri->numIndexes; i += 3 )
	{
		const idDrawVert& a = tri->verts[tri->indexes[i + 0]];
		const idDrawVert& b = tri->verts[tri->indexes[i + 1]];
		const idDrawVert& c = tri->verts[tri->indexes[i + 2]];
		
		// RB: compare texcoords instead of pointers
		if( a.GetTexCoord() == b.GetTexCoord() || b.GetTexCoord() == c.GetTexCoord() || c.GetTexCoord() == a.GetTexCoord() )
		{
			c_degenerate++;
		}
		// RB end
	}
	
	if( c_degenerate )
	{
//		common->Printf( "%d triangles with a degenerate texture space\n", c_degenerate );
	}
}

/*
=================
R_RemoveUnusedVerts
=================
*/
void idTriangles::RemoveUnusedVerts()
{
	int	i, index, used;
	
	idTempArray<int> mark( this->numVerts );
	mark.Zero();
	
	for( i = 0; i < this->numIndexes; ++i )
	{
		index = this->indexes[i];
		if( index < 0 || index >= this->numVerts )
		{
			common->Error( "R_RemoveUnusedVerts: bad index" );
		}
		mark[ index ] = 1;
		
		if( this->silIndexes )
		{
			index = this->silIndexes[i];
			if( index < 0 || index >= this->numVerts )
			{
				common->Error( "R_RemoveUnusedVerts: bad index" );
			}
			mark[ index ] = 1;
		}
	}
	
	used = 0;
	for( i = 0; i < this->numVerts; ++i )
	{
		if( !mark[i] )
		{
			continue;
		}
		mark[i] = used + 1;
		used++;
	}
	
	if( used != this->numVerts )
	{
		for( i = 0; i < this->numIndexes; ++i )
		{
			this->indexes[i] = mark[ this->indexes[i] ] - 1;
			if( this->silIndexes )
			{
				this->silIndexes[i] = mark[ this->silIndexes[i] ] - 1;
			}
		}
		this->numVerts = used;
		
		for( i = 0; i < this->numVerts; ++i )
		{
			index = mark[ i ];
			if( !index )
			{
				continue;
			}
			this->verts[ index - 1 ] = this->verts[i];
		}
		
		// this doesn't realloc the arrays to save the memory used by the unused verts
	}
}

/*
=================
R_MergeSurfaceList

Only deals with vertexes and indexes, not silhouettes, planes, etc.
Does NOT perform a cleanup triangles, so there may be duplicated verts in the result.
=================
*/
idTriangles* idTriangles::MergeSurfaceList( const idTriangles** surfaces, int numSurfaces )
{
	int totalVerts = 0;
	int totalIndexes = 0;

	for( int i = 0; i < numSurfaces; i++ )
	{
		totalVerts += surfaces[i]->numVerts;
		totalIndexes += surfaces[i]->numIndexes;
	}
	
	auto newTri = idTriangles::AllocStatic();
	newTri->numVerts = totalVerts;
	newTri->numIndexes = totalIndexes;
	newTri->AllocStaticVerts( newTri->numVerts );
	newTri->AllocStaticIndexes( newTri->numIndexes );
	
	totalVerts = 0;
	totalIndexes = 0;
	for( int i = 0; i < numSurfaces; i++ )
	{
		auto tri = surfaces[i];
		memcpy( newTri->verts + totalVerts, tri->verts, tri->numVerts * sizeof( *tri->verts ) );
		for( int j = 0; j < tri->numIndexes; j++ )
		{
			newTri->indexes[ totalIndexes + j ] = totalVerts + tri->indexes[j];
		}
		totalVerts += tri->numVerts;
		totalIndexes += tri->numIndexes;
	}
	
	return newTri;
}

/*
=================
R_MergeTriangles

Only deals with vertexes and indexes, not silhouettes, planes, etc.
Does NOT perform a cleanup triangles, so there may be duplicated verts in the result.
=================
*/
idTriangles* idTriangles::MergeTriangles( const idTriangles* tri1, const idTriangles* tri2 )
{
	const idTriangles *	tris[2];
	
	tris[0] = tri1;
	tris[1] = tri2;
	
	return MergeSurfaceList( tris, 2 );
}

/*
=================
R_ReverseTriangles

	Lit two sided surfaces need to have the triangles actually duplicated,
	they can't just turn on two sided lighting, because the normal and tangents
	are wrong on the other sides.

	This should be called before R_CleanupTriangles
=================
*/
void idTriangles::ReverseTriangles()
{
	// flip the normal on each vertex
	// If the surface is going to have generated normals, this won't matter,
	// but if it has explicit normals, this will keep it on the correct side
	for( int i = 0; i < this->numVerts; i++ )
	{
		this->verts[i].SetNormal( vec3_origin - this->verts[i].GetNormal() );
	}
	
	// flip the index order to make them back sided
	for( int i = 0; i < this->numIndexes; i += 3 )
	{
		SwapValues( this->indexes[ i + 0 ], this->indexes[ i + 1 ] );
	}
}

/*
=================
R_CleanupTriangles

	FIXME: allow createFlat and createSmooth normals, as well as explicit
=================
*/
void idTriangles::CleanupTriangles( bool createNormals, bool identifySilEdges, bool useUnsmoothedTangents )
{
	this->RangeCheckIndexes();
	
	this->CreateSilIndexes();
	
//	this->RemoveDuplicatedTriangles();	// this may remove valid overlapped transparent triangles

	this->RemoveDegenerateTriangles();
	
	R_TestDegenerateTextureSpace( this );
	
//	this->RemoveUnusedVerts();

	if( identifySilEdges )
	{
		IdentifySilEdges( true );	// assume it is non-deformable, and omit coplanar edges
	}
	
	// bust vertexes that share a mirrored edge into separate vertexes
	DuplicateMirroredVertexes();
	
	CreateDupVerts();
	
	this->DeriveBounds();
	
	if( useUnsmoothedTangents )
	{
		BuildDominantTris();
		this->DeriveTangents();
	}
	else if( !createNormals )
	{
		DeriveTangentsWithoutNormals();
	}
	else
	{
		this->DeriveTangents();
	}
}

/*
===================================================================================

	DEFORMED SURFACES

===================================================================================
*/

/*
===================
 R_BuildDeformInfo
===================
*/
deformInfo_t* idTriangles::BuildDeformInfo( int numVerts, const idDrawVert* verts, int numIndexes, const int* indexes, bool useUnsmoothedTangents )
{
	idTriangles	tri;
	tri.Clear();
	
	tri.numVerts = numVerts;
	tri.numIndexes = numIndexes;

	tri.AllocStaticVerts( tri.numVerts );
	SIMDProcessor->Memcpy( tri.verts, verts, tri.numVerts * sizeof( tri.verts[0] ) );
	
	tri.AllocStaticIndexes( tri.numIndexes );	
	// don't memcpy, so we can change the index type from int to short without changing the interface
	for( int i = 0; i < tri.numIndexes; ++i )
	{
		tri.indexes[i] = indexes[i];
	}
	
	tri.RangeCheckIndexes();
	tri.CreateSilIndexes();
	tri.IdentifySilEdges( false );			// we cannot remove coplanar edges, because they can deform to silhouettes
	tri.DuplicateMirroredVertexes();		// split mirror points into multiple points
	tri.CreateDupVerts();
	if( useUnsmoothedTangents )
	{
		tri.BuildDominantTris();
	}
	tri.DeriveTangents();
	
	auto deform = allocManager.StaticAlloc<deformInfo_t, TAG_MODEL, true>();
	
	deform->numSourceVerts = numVerts;
	deform->numOutputVerts = tri.numVerts;
	deform->verts = tri.verts;
	
	deform->numIndexes = numIndexes;
	deform->indexes = tri.indexes;	
	
	deform->silIndexes = tri.silIndexes;		
	deform->numMirroredVerts = tri.numMirroredVerts;
	deform->mirroredVerts = tri.mirroredVerts;	
	deform->numDupVerts = tri.numDupVerts;
	deform->dupVerts = tri.dupVerts;
	deform->numSilEdges = tri.numSilEdges;
	deform->silEdges = tri.silEdges;
	
	if( tri.dominantTris != NULL )
	{
		Mem_Free( tri.dominantTris );
		tri.dominantTris = NULL;
	}

	deform->CreateStaticCache();

	return deform;
}

/*
===================
 R_CreateDeformInfoStaticCache
===================
*/
void deformInfo_t::CreateStaticCache()
{
	idShadowVertSkinned* shadowVerts = ( idShadowVertSkinned* )Mem_Alloc16( ALIGN( numOutputVerts * 2 * sizeof( idShadowVertSkinned ), 16 ), TAG_MODEL );
	idShadowVertSkinned::CreateShadowCache( shadowVerts, verts, numOutputVerts );

	staticAmbientCache = vertexCache.AllocStaticVertex( verts, ALIGN( numOutputVerts * sizeof( idDrawVert ), VERTEX_CACHE_ALIGN ) );
	staticIndexCache = vertexCache.AllocStaticIndex( indexes, ALIGN( numIndexes * sizeof( triIndex_t ), INDEX_CACHE_ALIGN ) );
	staticShadowCache = vertexCache.AllocStaticVertex( shadowVerts, ALIGN( numOutputVerts * 2 * sizeof( idShadowVertSkinned ), VERTEX_CACHE_ALIGN ) );

	Mem_Free( shadowVerts );
}

/*
===================
 R_FreeDeformInfo
===================
*/
void deformInfo_t::Free()
{
	if( verts != NULL )
	{
		Mem_Free( verts );
		verts = NULL;
	}
	if( indexes != NULL )
	{
		Mem_Free( indexes );
		indexes = NULL;
	}
	if( silIndexes != NULL )
	{
		Mem_Free( silIndexes );
		silIndexes = NULL;
	}
	if( silEdges != NULL )
	{
		Mem_Free( silEdges );
		silEdges = NULL;
	}
	if( mirroredVerts != NULL )
	{
		Mem_Free( mirroredVerts );
		mirroredVerts = NULL;
	}
	if( dupVerts != NULL )
	{
		Mem_Free( dupVerts );
		dupVerts = NULL;
	}
}

/*
===================
 R_DeformInfoMemoryUsed
===================
*/
size_t deformInfo_t::CPUMemoryUsed()
{
	size_t total = 0;
	
	if( verts != NULL )
	{
		total += numOutputVerts * sizeof( verts[0] );
	}
	if( indexes != NULL )
	{
		total += numIndexes * sizeof( indexes[0] );
	}
	if( mirroredVerts != NULL )
	{
		total += numMirroredVerts * sizeof( mirroredVerts[0] );
	}
	if( dupVerts != NULL )
	{
		total += numDupVerts * sizeof( dupVerts[0] );
	}
	if( silIndexes != NULL )
	{
		total += numIndexes * sizeof( silIndexes[0] );
	}
	if( silEdges != NULL )
	{
		total += numSilEdges * sizeof( silEdges[0] );
	}
	
	total += sizeof( *this );
	return total;
}

/*
===================================================================================

	VERTEX / INDEX CACHING

===================================================================================
*/

/*
===================
 R_InitDrawSurfFromTri
===================
*/
void idTriangles::InitDrawSurfFromTriangles( drawSurf_t& ds )
{
	if( this->numIndexes == 0 )
	{
		ds.numIndexes = 0;
		return;
	}
	
	// copy verts and indexes to this frame's hardware memory if they aren't already there
	//
	// deformed surfaces will not have any vertices but the ambient cache will have already
	// been created for them.
	if( ( this->verts == NULL ) && !this->referencedIndexes )
	{
		// pre-generated shadow models will not have any verts, just shadowVerts
		this->ambientCache = 0;
	}
	else if( !vertexCache.CacheIsCurrent( this->ambientCache ) )
	{
		this->ambientCache = vertexCache.AllocVertex( this->verts, ALIGN( this->numVerts * sizeof( this->verts[0] ), VERTEX_CACHE_ALIGN ) );
	}
	if( !vertexCache.CacheIsCurrent( this->indexCache ) )
	{
		this->indexCache = vertexCache.AllocIndex( this->indexes, ALIGN( this->numIndexes * sizeof( this->indexes[0] ), INDEX_CACHE_ALIGN ) );
	}
	
	ds.numIndexes = this->numIndexes;
	ds.ambientCache = this->ambientCache;
	ds.indexCache = this->indexCache;
	ds.shadowCache = this->shadowCache;
	ds.jointCache = 0;
}

/*
===================
 R_CreateStaticBuffersForTri

	For static surfaces, the indexes, ambient, and shadow buffers can be pre-created at load
	time, rather than being re-created each frame in the frame temporary buffers.
===================
*/
void idTriangles::CreateStaticBuffers()
{
	this->indexCache = 0;
	this->ambientCache = 0;
	this->shadowCache = 0;
	
	// index cache
	if( this->indexes != NULL )
	{
		this->indexCache = vertexCache.AllocStaticIndex( this->indexes, ALIGN( this->numIndexes * sizeof( this->indexes[0] ), INDEX_CACHE_ALIGN ) );
	}
	
	// vertex cache
	if( this->verts != NULL )
	{
		this->ambientCache = vertexCache.AllocStaticVertex( this->verts, ALIGN( this->numVerts * sizeof( this->verts[0] ), VERTEX_CACHE_ALIGN ) );
	}
	
	// shadow cache
	if( this->preLightShadowVertexes != NULL )
	{
		// this should only be true for the _prelight<NAME> pre-calculated shadow volumes
		assert( this->verts == NULL );	// pre-light shadow volume surfaces don't have ambient vertices
		const int shadowSize = ALIGN( this->numVerts * 2 * sizeof( idShadowVert ), VERTEX_CACHE_ALIGN );
		this->shadowCache = vertexCache.AllocStaticVertex( this->preLightShadowVertexes, shadowSize );
	}
	else if( this->verts != NULL )
	{
		// the shadowVerts for normal models include all the xyz values duplicated
		// for a W of 1 (near cap) and a W of 0 (end cap, projected to infinity)
		const int shadowSize = ALIGN( this->numVerts * 2 * sizeof( idShadowVert ), VERTEX_CACHE_ALIGN );
		if( this->staticShadowVertexes == NULL )
		{
			this->staticShadowVertexes = ( idShadowVert* ) Mem_Alloc16( shadowSize, TAG_TEMP );
			idShadowVert::CreateShadowCache( this->staticShadowVertexes, this->verts, this->numVerts );
		}
		this->shadowCache = vertexCache.AllocStaticVertex( this->staticShadowVertexes, shadowSize );
		
	#if !defined( KEEP_INTERACTION_CPU_DATA )
		Mem_Free( this->staticShadowVertexes );
		this->staticShadowVertexes = NULL;
	#endif
	}
}

/*
===================================================================================

	Utilities

===================================================================================
*/

/*
================
 ReadFromFile
================
*/
void idTriangles::ReadFromFile( idFile *file )
{
	bool temp;

	// Read the contents of idTriangles
	auto & tri = *this;

	file->ReadVec3( tri.bounds[ 0 ] );
	file->ReadVec3( tri.bounds[ 1 ] );

	int ambientViewCount = 0;	// FIXME: remove
	file->ReadBig( ambientViewCount );
	file->ReadBig( tri.generateNormals );
	file->ReadBig( tri.tangentsCalculated );
	file->ReadBig( tri.perfectHull );
	file->ReadBig( tri.referencedIndexes );

	file->ReadBig( tri.numVerts );
	tri.verts = NULL;
	int numInFile = 0;
	file->ReadBig( numInFile );
	if( numInFile > 0 )
	{
		tri.AllocStaticVerts( tri.numVerts );
		assert( tri.verts != NULL );
		for( int j = 0; j < tri.numVerts; ++j )
		{
			tri.verts[ j ].ReadFromFile( file );
		}
	}

	file->ReadBig( numInFile );
	if( numInFile == 0 )
	{
		tri.preLightShadowVertexes = NULL;
	}
	else
	{
		tri.AllocStaticPreLightShadowVerts( numInFile );
		for( int j = 0; j < numInFile; ++j )
		{
			file->ReadVec4( tri.preLightShadowVertexes[ j ].xyzw );
		}
	}

	file->ReadBig( tri.numIndexes );
	tri.indexes = NULL;
	tri.silIndexes = NULL;
	if( tri.numIndexes > 0 )
	{
		tri.AllocStaticIndexes( tri.numIndexes );
		file->ReadBigArray( tri.indexes, tri.numIndexes );
	}
	file->ReadBig( numInFile );
	if( numInFile > 0 )
	{
		tri.AllocStaticSilIndexes( tri.numIndexes );
		file->ReadBigArray( tri.silIndexes, tri.numIndexes );
	}

	file->ReadBig( tri.numMirroredVerts );
	tri.mirroredVerts = NULL;
	if( tri.numMirroredVerts > 0 )
	{
		tri.AllocStaticMirroredVerts( tri.numMirroredVerts );
		file->ReadBigArray( tri.mirroredVerts, tri.numMirroredVerts );
	}

	file->ReadBig( tri.numDupVerts );
	tri.dupVerts = NULL;
	if( tri.numDupVerts > 0 )
	{
		tri.AllocStaticDupVerts( tri.numDupVerts );
		file->ReadBigArray( tri.dupVerts, tri.numDupVerts * 2 );
	}

	file->ReadBig( tri.numSilEdges );
	tri.silEdges = NULL;
	if( tri.numSilEdges > 0 )
	{
		tri.AllocStaticSilEdges( tri.numSilEdges );
		assert( tri.silEdges != NULL );
		for( int j = 0; j < tri.numSilEdges; ++j )
		{
			file->ReadBig( tri.silEdges[ j ].p1 );
			file->ReadBig( tri.silEdges[ j ].p2 );
			file->ReadBig( tri.silEdges[ j ].v1 );
			file->ReadBig( tri.silEdges[ j ].v2 );
		}
	}

	file->ReadBig( temp );
	tri.dominantTris = NULL;
	if( temp )
	{
		tri.AllocStaticDominantTris( tri.numVerts );
		assert( tri.dominantTris != NULL );
		for( int j = 0; j < tri.numVerts; ++j )
		{
			file->ReadBig( tri.dominantTris[ j ].v2 );
			file->ReadBig( tri.dominantTris[ j ].v3 );
			file->ReadFloat( tri.dominantTris[ j ].normalizationScale[ 0 ] );
			file->ReadFloat( tri.dominantTris[ j ].normalizationScale[ 1 ] );
			file->ReadFloat( tri.dominantTris[ j ].normalizationScale[ 2 ] );
		}
	}

	file->ReadBig( tri.numShadowIndexesNoFrontCaps );
	file->ReadBig( tri.numShadowIndexesNoCaps );
	file->ReadBig( tri.shadowCapPlaneBits );

	tri.baseTriangles = NULL;
	tri.nextDeferredFree = NULL;
	tri.indexCache = 0;
	tri.ambientCache = 0;
	tri.shadowCache = 0;
}
/*
================
 WriteToFile
================
*/
void idTriangles::WriteToFile( idFile *file ) const
{
	const auto & tri = *this;

	file->WriteVec3( tri.bounds[ 0 ] );
	file->WriteVec3( tri.bounds[ 1 ] );

	int ambientViewCount = 0;	// FIXME: remove
	file->WriteBig( ambientViewCount );
	file->WriteBig( tri.generateNormals );
	file->WriteBig( tri.tangentsCalculated );
	file->WriteBig( tri.perfectHull );
	file->WriteBig( tri.referencedIndexes );

	// shadow models use numVerts but have no verts
	file->WriteBig( tri.numVerts );
	if( tri.verts != NULL )
	{
		file->WriteBig( tri.numVerts );
	}
	else
	{
		file->WriteBig( ( int )0 );
	}

	if( tri.numVerts > 0 && tri.verts != NULL )
	{
		for( int j = 0; j < tri.numVerts; j++ )
		{
			tri.verts[ j ].WriteToFile( file );
		}
	}

	if( tri.preLightShadowVertexes != NULL )
	{
		file->WriteBig( tri.numVerts * 2 );
		for( int j = 0; j < tri.numVerts * 2; j++ )
		{
			file->WriteVec4( tri.preLightShadowVertexes[ j ].xyzw );
		}
	}
	else
	{
		file->WriteBig( ( int )0 );
	}

	file->WriteBig( tri.numIndexes );

	if( tri.numIndexes > 0 )
	{
		file->WriteBigArray( tri.indexes, tri.numIndexes );
	}

	if( tri.silIndexes != NULL )
	{
		file->WriteBig( tri.numIndexes );
	}
	else
	{
		file->WriteBig( ( int )0 );
	}

	if( tri.numIndexes > 0 && tri.silIndexes != NULL )
	{
		file->WriteBigArray( tri.silIndexes, tri.numIndexes );
	}

	file->WriteBig( tri.numMirroredVerts );
	if( tri.numMirroredVerts > 0 )
	{
		file->WriteBigArray( tri.mirroredVerts, tri.numMirroredVerts );
	}

	file->WriteBig( tri.numDupVerts );
	if( tri.numDupVerts > 0 )
	{
		file->WriteBigArray( tri.dupVerts, tri.numDupVerts * 2 );
	}

	file->WriteBig( tri.numSilEdges );
	if( tri.numSilEdges > 0 )
	{
		for( int j = 0; j < tri.numSilEdges; j++ )
		{
			file->WriteBig( tri.silEdges[ j ].p1 );
			file->WriteBig( tri.silEdges[ j ].p2 );
			file->WriteBig( tri.silEdges[ j ].v1 );
			file->WriteBig( tri.silEdges[ j ].v2 );
		}
	}

	file->WriteBig( tri.dominantTris != NULL );
	if( tri.dominantTris != NULL )
	{
		for( int j = 0; j < tri.numVerts; j++ )
		{
			file->WriteBig( tri.dominantTris[ j ].v2 );
			file->WriteBig( tri.dominantTris[ j ].v3 );
			file->WriteFloat( tri.dominantTris[ j ].normalizationScale[ 0 ] );
			file->WriteFloat( tri.dominantTris[ j ].normalizationScale[ 1 ] );
			file->WriteFloat( tri.dominantTris[ j ].normalizationScale[ 2 ] );
		}
	}

	file->WriteBig( tri.numShadowIndexesNoFrontCaps );
	file->WriteBig( tri.numShadowIndexesNoCaps );
	file->WriteBig( tri.shadowCapPlaneBits );
}

/*
================
 R_AddCubeFace
================
*/
void idTriangles::AddCubeFace( const idVec3& v1, const idVec3& v2, const idVec3& v3, const idVec3& v4 )
{
	this->verts[ this->numVerts + 0 ].Clear();
	this->verts[ this->numVerts + 0 ].SetPosition( v1 * 8 );
	this->verts[ this->numVerts + 0 ].SetTexCoord( 0, 0 );

	this->verts[ this->numVerts + 1 ].Clear();
	this->verts[ this->numVerts + 1 ].SetPosition( v2 * 8 );
	this->verts[ this->numVerts + 1 ].SetTexCoord( 1, 0 );

	this->verts[ this->numVerts + 2 ].Clear();
	this->verts[ this->numVerts + 2 ].SetPosition( v3 * 8 );
	this->verts[ this->numVerts + 2 ].SetTexCoord( 1, 1 );

	this->verts[ this->numVerts + 3 ].Clear();
	this->verts[ this->numVerts + 3 ].SetPosition( v4 * 8 );
	this->verts[ this->numVerts + 3 ].SetTexCoord( 0, 1 );

	this->indexes[ this->numIndexes + 0 ] = this->numVerts + 0;
	this->indexes[ this->numIndexes + 1 ] = this->numVerts + 1;
	this->indexes[ this->numIndexes + 2 ] = this->numVerts + 2;
	this->indexes[ this->numIndexes + 3 ] = this->numVerts + 0;
	this->indexes[ this->numIndexes + 4 ] = this->numVerts + 2;
	this->indexes[ this->numIndexes + 5 ] = this->numVerts + 3;

	this->numVerts += 4;
	this->numIndexes += 6;
}

/*
=====================
 R_PolytopeSurface
	Generate vertexes and indexes for a polytope, and optionally returns the polygon windings.
	The positive sides of the planes will be visible.
=====================
*/
idTriangles * idTriangles::CreateTrianglesForPolytope( int numPlanes, const idPlane* planes, idWinding** windings )
{
	const int MAX_POLYTOPE_PLANES = 6;
	idFixedWinding planeWindings[ MAX_POLYTOPE_PLANES ];

	if( numPlanes > MAX_POLYTOPE_PLANES )
	{
		common->Error( "R_PolytopeSurface: more than %d planes", MAX_POLYTOPE_PLANES );
	}

	int numVerts = 0;
	int numIndexes = 0;
	for( int i = 0; i < numPlanes; ++i )
	{
		const idPlane& plane = planes[ i ];
		idFixedWinding& w = planeWindings[ i ];

		w.BaseForPlane( plane );
		for( int j = 0; j < numPlanes; ++j )
		{
			const idPlane& plane2 = planes[ j ];
			if( j == i )
			{
				continue;
			}
			if( !w.ClipInPlace( -plane2, ON_EPSILON ) )
			{
				break;
			}
		}
		if( w.GetNumPoints() <= 2 )
		{
			continue;
		}
		numVerts += w.GetNumPoints();
		numIndexes += ( w.GetNumPoints() - 2 ) * 3;
	}

	// allocate the surface
	auto tri = idTriangles::AllocStatic();
	tri->AllocStaticVerts( numVerts );
	tri->AllocStaticIndexes( numIndexes );

	// copy the data from the windings
	for( int i = 0; i < numPlanes; ++i )
	{
		idFixedWinding& w = planeWindings[ i ];
		if( !w.GetNumPoints() )
		{
			continue;
		}
		for( int j = 0; j < w.GetNumPoints(); ++j )
		{
			tri->verts[ tri->numVerts + j ].Clear();
			tri->verts[ tri->numVerts + j ].SetPosition( w[ j ].ToVec3() );
		}

		for( int j = 1; j < w.GetNumPoints() - 1; ++j )
		{
			tri->indexes[ tri->numIndexes + 0 ] = tri->numVerts;
			tri->indexes[ tri->numIndexes + 1 ] = tri->numVerts + j;
			tri->indexes[ tri->numIndexes + 2 ] = tri->numVerts + j + 1;
			tri->numIndexes += 3;
		}
		tri->numVerts += w.GetNumPoints();

		// optionally save the winding
		if( windings )
		{
			windings[ i ] = new idWinding( w.GetNumPoints() );
			*windings[ i ] = w;
		}
	}

	tri->DeriveBounds();

	return tri;
}