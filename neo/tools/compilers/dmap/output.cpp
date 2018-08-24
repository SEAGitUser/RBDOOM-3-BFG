/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2015 Robert Beckebans

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#include "dmap.h"

//=================================================================================


#if 0

should we try and snap values very close to 0.5, 0.25, 0.125, etc ?

	do we write out normals, or just a "smooth shade" flag ?
		resolved : normals.  otherwise adjacent facet shaded surfaces get their
		vertexes merged, and they would have to be split apart before drawing

		do we save out "wings" for shadow silhouette info ?


#endif

#define	AREANUM_DIFFERENT	-2
/*
=============
PruneNodes_r

Any nodes that have all children with the same
area can be combined into a single leaf node

Returns the area number of all children, or
AREANUM_DIFFERENT if not the same.
=============
*/
int	PruneNodes_r( node_t* node )
{
	int	a1, a2;

	if( node->planenum == PLANENUM_LEAF )
	{
		return node->area;
	}

	a1 = PruneNodes_r( node->children[0] );
	a2 = PruneNodes_r( node->children[1] );

	if( a1 != a2 || a1 == AREANUM_DIFFERENT )
	{
		return AREANUM_DIFFERENT;
	}

	// free all the nodes below this point
	FreeTreePortals_r( node->children[0] );
	FreeTreePortals_r( node->children[1] );
	FreeTree_r( node->children[0] );
	FreeTree_r( node->children[1] );

	// change this node to a leaf
	node->planenum = PLANENUM_LEAF;
	node->area = a1;

	return a1;
}

static void WriteFloat( idFile* f, float v )
{
	if( idMath::Fabs( v - idMath::Rint( v ) ) < 0.001 )
	{
		f->WriteFloatString( "%i ", ( int )idMath::Rint( v ) );
	}
	else {
		f->WriteFloatString( "%f ", v );
	}
}

void Write1DMatrix( idFile* f, int x, float* m )
{
	f->WriteFloatString( "( " );

	for( int i = 0; i < x; i++ )
	{
		WriteFloat( f, m[i] );
	}

	f->WriteFloatString( ") " );
}

static int CountUniqueShaders( optimizeGroup_t* groups )
{
	optimizeGroup_t*		a, *b;
	int					count;

	count = 0;

	for( a = groups ; a ; a = a->nextGroup )
	{
		if( !a->triList )  	// ignore groups with no tris
		{
			continue;
		}
		for( b = groups ; b != a ; b = b->nextGroup )
		{
			if( !b->triList )
			{
				continue;
			}
			if( a->material != b->material )
			{
				continue;
			}
			if( a->mergeGroup != b->mergeGroup )
			{
				continue;
			}
			break;
		}
		if( a == b )
		{
			count++;
		}
	}

	return count;
}


/*
==============
MatchVert
==============
*/
#define	XYZ_EPSILON	0.01
#define	ST_EPSILON	0.001
#define	COSINE_EPSILON	0.999

static bool MatchVert( const idDrawVert* a, const idDrawVert* b )
{
	if( idMath::Fabs( a->GetPosition()[0] - b->GetPosition()[0] ) > XYZ_EPSILON )
	{
		return false;
	}
	if( idMath::Fabs( a->GetPosition()[1] - b->GetPosition()[1] ) > XYZ_EPSILON )
	{
		return false;
	}
	if( idMath::Fabs( a->GetPosition()[2] - b->GetPosition()[2] ) > XYZ_EPSILON )
	{
		return false;
	}

	if( idMath::Fabs( a->GetTexCoordS() - b->GetTexCoordS() ) > ST_EPSILON )
	{
		return false;
	}
	if( idMath::Fabs( a->GetTexCoordT() - b->GetTexCoordT() ) > ST_EPSILON )
	{
		return false;
	}

	// RB begin
	// if the normal is 0 (smoothed normals), consider it a match
	if( a->GetNormal().Length() == 0 && b->GetNormal().Length() == 0 )
	{
		return true;
	}

	// otherwise do a dot-product cosine check
	if( ( a->GetNormal() * b->GetNormal() ) < COSINE_EPSILON )
	{
		return false;
	}
	// RB end

	return true;
}

/*
====================
ShareMapTriVerts

	Converts independent triangles to shared vertex triangles
====================
*/
idTriangles * ShareMapTriVerts( const mapTri_t* tris )
{
	const mapTri_t*	step;
	int			count;
	int			i, j;
	int			numVerts;
	int			numIndexes;
	idTriangles*	uTri;

	// unique the vertexes
	count = CountTriList( tris );

	uTri = idTriangles::AllocStatic();
	uTri->AllocStaticVerts( count * 3 );
	uTri->AllocStaticIndexes( count * 3 );

	numVerts = 0;
	numIndexes = 0;

	for( step = tris ; step ; step = step->next )
	{
		for( i = 0 ; i < 3 ; i++ )
		{
			const idDrawVert*	dv;

			dv = &step->v[i];

			// search for a match
			for( j = 0 ; j < numVerts ; j++ )
			{
				if( MatchVert( &uTri->verts[j], dv ) )
				{
					break;
				}
			}

			if( j == numVerts )
			{
				numVerts++;
				uTri->verts[j].SetPosition( dv->GetPosition() );
				//uTri->verts[j].SetNormal( dv->normal[0], dv->normal[1], dv->normal[2] );
				uTri->verts[j].SetNormal( dv->GetNormal() );
				uTri->verts[j].SetTexCoordS( dv->GetTexCoordS() );
				uTri->verts[j].SetTexCoordT( dv->GetTexCoordT() );
			}

			uTri->indexes[numIndexes++] = j;
		}
	}

	uTri->numVerts = numVerts;
	uTri->numIndexes = numIndexes;

	return uTri;
}

/*
==================
CleanupUTriangles
==================
*/
static void CleanupUTriangles( idTriangles* tri )
{
	// perform cleanup operations

	tri->RangeCheckIndexes();
	tri->CreateSilIndexes();
//	tri->RemoveDuplicatedTriangles();	// this may remove valid overlapped transparent triangles
	tri->RemoveDegenerateTriangles();
//	tri->RemoveUnusedVerts();

	tri->FreeStaticSilIndexes();
}

/*
====================
WriteUTriangles

	Writes text verts and indexes to procfile
====================
*/
static void WriteUTriangles( idFile* file, const idTriangles* uTris )
{
	int			col;
	int			i;

	// emit this chain
	file->WriteFloatString( "/* numVerts = */ %i /* numIndexes = */ %i\n", uTris->numVerts, uTris->numIndexes );

	// verts
	col = 0;
	for( i = 0 ; i < uTris->numVerts ; i++ )
	{
		float	vec[8];
		const idDrawVert* dv;

		dv = &uTris->verts[i];

		vec[0] = dv->GetPosition()[0];
		vec[1] = dv->GetPosition()[1];
		vec[2] = dv->GetPosition()[2];

		idVec2 st = dv->GetTexCoord();
		vec[3] = st.x;
		vec[4] = st.y;

		idVec3 normal = dv->GetNormal();
		vec[5] = normal.x;
		vec[6] = normal.y;
		vec[7] = normal.z;

		Write1DMatrix( file, 8, vec );

		if( ++col == 3 )
		{
			col = 0;
			file->WriteFloatString( "\n" );
		}
	}
	if( col != 0 )
	{
		file->WriteFloatString( "\n" );
	}

	// indexes
	col = 0;
	for( i = 0 ; i < uTris->numIndexes ; i++ )
	{
		file->WriteFloatString( "%i ", uTris->indexes[i] );

		if( ++col == 18 )
		{
			col = 0;
			file->WriteFloatString( "\n" );
		}
	}
	if( col != 0 )
	{
		file->WriteFloatString( "\n" );
	}
}


/*
=======================
GroupsAreSurfaceCompatible

Planes, texcoords, and groupLights can differ,
but the material and mergegroup must match
=======================
*/
static bool GroupsAreSurfaceCompatible( const optimizeGroup_t* a, const optimizeGroup_t* b )
{
	if( a->material != b->material )
	{
		return false;
	}
	if( a->mergeGroup != b->mergeGroup )
	{
		return false;
	}
	return true;
}

/*
====================
WriteOutputSurfaces
====================
*/
static void WriteOutputSurfaces( idFile* file, int entityNum, int areaNum )
{
	struct interactionTris_t {
		interactionTris_t*	next;
		mapTri_t*	triList;
		mapLight_t*	light;
	};

	auto area = &dmapGlobals.uEntities[entityNum].areas[areaNum];
	auto entity = dmapGlobals.uEntities[entityNum].mapEntity;

	int numSurfaces = CountUniqueShaders( area->groups );

	if( entityNum == 0 )
	{
		file->WriteFloatString( "model { /* name = */ \"_area%i\" /* numSurfaces = */ %i\n\n", areaNum, numSurfaces );
	}
	else {
		const char* name;
		entity->epairs.GetString( "name", "", &name );
		if( !name[0] )
		{
			common->Error( "Entity %i has surfaces, but no name key", entityNum );
		}
		file->WriteFloatString( "model { /* name = */ \"%s\" /* numSurfaces = */ %i\n\n", name, numSurfaces );
	}

	int surfaceNum = 0;
	for( auto group = area->groups ; group ; group = group->nextGroup )
	{
		if( group->surfaceEmited )
		{
			continue;
		}

		// combine all groups compatible with this one
		// usually several optimizeGroup_t can be combined into a single
		// surface, even though they couldn't be merged together to save
		// vertexes because they had different planes, texture coordinates, or lights.
		// Different mergeGroups will stay in separate surfaces.
		mapTri_t* ambient = NULL;

		// each light that illuminates any of the groups in the surface will
		// get its own list of indexes out of the original surface
		interactionTris_t *interactions = NULL;

		for( auto groupStep = group ; groupStep ; groupStep = groupStep->nextGroup )
		{
			if( groupStep->surfaceEmited )
			{
				continue;
			}
			if( !GroupsAreSurfaceCompatible( group, groupStep ) )
			{
				continue;
			}

			// copy it out to the ambient list
			mapTri_t* copy = CopyTriList( groupStep->triList );
			ambient = MergeTriLists( ambient, copy );
			groupStep->surfaceEmited = true;

			// duplicate it into an interaction for each groupLight
			for( int i = 0 ; i < groupStep->numGroupLights ; i++ )
			{
				interactionTris_t* checkInter = nullptr;
				for( checkInter = interactions ; checkInter ; checkInter = checkInter->next )
				{
					if( checkInter->light == groupStep->groupLights[i] )
					{
						break;
					}
				}
				if( !checkInter )
				{
					// create a new interaction
					checkInter = ( interactionTris_t* )Mem_ClearedAlloc( sizeof( *checkInter ), TAG_TOOLS );

					checkInter->light = groupStep->groupLights[i];

					checkInter->next = interactions;
					interactions = checkInter;
				}

				copy = CopyTriList( groupStep->triList );
				checkInter->triList = MergeTriLists( checkInter->triList, copy );
			}
		}

		if( !ambient )
		{
			continue;
		}

		if( surfaceNum >= numSurfaces )
		{
			common->Error( "WriteOutputSurfaces: surfaceNum >= numSurfaces" );
		}

		file->WriteFloatString( "/* surface %i */ { ", surfaceNum );
		surfaceNum++;
		file->WriteFloatString( "\"%s\" ", ambient->material->GetName() );

		auto uTri = ShareMapTriVerts( ambient );
		FreeTriList( ambient );

		CleanupUTriangles( uTri );
		WriteUTriangles( file, uTri );
		idTriangles::FreeStatic( uTri );

		file->WriteFloatString( "}\n\n" );
	}

	file->WriteFloatString( "}\n\n" );
}

/*
===============
WriteNode_r

===============
*/
static void WriteNode_r( idFile* file, node_t* node )
{
	int		child[2];
	int		i;
	idPlane*	plane;

	if( node->planenum == PLANENUM_LEAF )
	{
		// we shouldn't get here unless the entire world
		// was a single leaf
		file->WriteFloatString( "/* node 0 */ ( 0 0 0 0 ) -1 -1\n" );
		return;
	}

	for( i = 0 ; i < 2 ; i++ )
	{
		if( node->children[i]->planenum == PLANENUM_LEAF )
		{
			child[i] = -1 - node->children[i]->area;
		}
		else
		{
			child[i] = node->children[i]->nodeNumber;
		}
	}

	plane = &dmapGlobals.mapPlanes[node->planenum];

	file->WriteFloatString( "/* node %i */ ", node->nodeNumber );
	Write1DMatrix( file, 4, plane->ToFloatPtr() );
	file->WriteFloatString( "%i %i\n", child[0], child[1] );

	if( child[0] > 0 )
	{
		WriteNode_r( file, node->children[0] );
	}
	if( child[1] > 0 )
	{
		WriteNode_r( file, node->children[1] );
	}
}

int NumberNodes_r( node_t* node, int nextNumber )
{
	if( node->planenum == PLANENUM_LEAF )
	{
		return nextNumber;
	}
	node->nodeNumber = nextNumber;
	nextNumber++;
	nextNumber = NumberNodes_r( node->children[0], nextNumber );
	nextNumber = NumberNodes_r( node->children[1], nextNumber );

	return nextNumber;
}

/*
====================
WriteOutputNodes
====================
*/
static void WriteOutputNodes( idFile* file, node_t* node )
{
	// prune unneeded nodes and count
	PruneNodes_r( node );
	int numNodes = NumberNodes_r( node, 0 );

	// output
	file->WriteFloatString( "nodes { /* numNodes = */ %i\n\n", numNodes );
	file->WriteFloatString( "/* node format is: ( planeVector ) positiveChild negativeChild */\n" );
	file->WriteFloatString( "/* a child number of 0 is an opaque, solid area */\n" );
	file->WriteFloatString( "/* negative child numbers are areas: (-1-child) */\n" );

	WriteNode_r( file, node );

	file->WriteFloatString( "}\n\n" );
}

/*
====================
WriteOutputPortals
====================
*/
static void WriteOutputPortals( idFile* file, uEntity_t* e )
{
	file->WriteFloatString( "interAreaPortals { /* numAreas = */ %i /* numIAP = */ %i\n\n", e->numAreas, interAreaPortals.Num() );
	file->WriteFloatString( "/* interAreaPortal format is: numPoints positiveSideArea negativeSideArea ( point) ... */\n" );

	for( int i = 0 ; i < interAreaPortals.Num() ; i++ )
	{
		auto iap = &interAreaPortals[i];

		// RB: support new area portals
		auto w = ( iap->side )? iap->side->winding : &iap->w;
		// RB end

		file->WriteFloatString( "/* iap %i */ %i %i %i ", i, w->GetNumPoints(), iap->area0, iap->area1 );
		for( int j = 0 ; j < w->GetNumPoints() ; j++ )
		{
			Write1DMatrix( file, 3, ( *w )[j].ToFloatPtr() );
		}
		file->WriteFloatString( "\n" );
	}

	file->WriteFloatString( "}\n\n" );
}

/*
====================
WriteOutputEntity
====================
*/
static void WriteOutputEntity( idFile* file, int entityNum )
{
	auto & e = dmapGlobals.uEntities[entityNum];

	if( entityNum != 0 )
	{
		// entities may have enclosed, empty areas that we don't need to write out
		if( e.numAreas > 1 )
		{
			e.numAreas = 1;
		}
	}

	for( int i = 0 ; i < e.numAreas ; i++ )
	{
		WriteOutputSurfaces( file, entityNum, i );
	}

	// we will completely skip the portals and nodes if it is a single area
	if( entityNum == 0 && e.numAreas > 1 )
	{
		// output the area portals
		WriteOutputPortals( file, &e );

		// output the nodes
		WriteOutputNodes( file, e.tree->headnode );
	}
}

/*
====================
WriteOutputFile
====================
*/
void WriteOutputFile()
{
	common->Printf( "----- WriteOutputFile -----\n" );

	idStr qpath;
	///sprintf( qpath, "%s." PROC_FILE_EXT, dmapGlobals.mapFileBase );
	qpath.Format<MAX_PATH>( "%s." PROC_FILE_EXT, dmapGlobals.mapFileBase );

	common->Printf( "writing %s\n", qpath.c_str() );
	// _D3XP used fs_cdpath
	auto file = fileSystem->OpenFileWrite( qpath, "fs_basepath" );
	if( !file )
	{
		common->Error( "Error opening %s", qpath.c_str() );
	}

	file->WriteFloatString( "%s\n\n", PROC_FILE_ID );

	// write the entity models and information, writing entities first
	for( int i = dmapGlobals.num_entities - 1; i >= 0; i-- )
	{
		if( dmapGlobals.uEntities[ i ].primitives )
		{
			WriteOutputEntity( file, i );
		}
	}

	fileSystem->CloseFile( file );
}