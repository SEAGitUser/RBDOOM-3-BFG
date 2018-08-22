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
#include "Model_local.h"

#include "../../../idlib/geometry/DrawVert_intrinsics.h"

/*
==========================================================================================

								DEFORM SURFACES

==========================================================================================
*/

/*
=================
R_FinishDeform
=================
*/
static drawSurf_t* R_FinishDeform( drawSurf_t* surf, idTriangles* newTri, const idDrawVert* newVerts, const triIndex_t* newIndexes )
{
	newTri->vertexCache = vertexCache.AllocVertex( newVerts, ALIGN( newTri->numVerts * sizeof( idDrawVert ), VERTEX_CACHE_ALIGN ) );
	newTri->indexCache = vertexCache.AllocIndex( newIndexes, ALIGN( newTri->numIndexes * sizeof( triIndex_t ), INDEX_CACHE_ALIGN ) );

	surf->frontEndGeo = newTri;

	surf->numIndexes = newTri->numIndexes;
	surf->vertexCache = newTri->vertexCache;
	surf->indexCache = newTri->indexCache;
	surf->shadowCache = 0;
	surf->jointCache = 0;

	surf->nextOnLight = NULL;

	return surf;
}

#if USE_INTRINSICS

	ID_INLINE __m128 _loadV3( const idVec3 & vec )
	{
		// load x, y with a 64 bit integer load (00YX)
		__m128i xy = _mm_loadl_epi64( ( const __m128i* )&vec );

		// now load the z element using a 32 bit float load (000Z)
		__m128 z = _mm_load_ss( &vec.z );

		// we now need to cast the __m128i register into a __m128 one (0ZYX)
		return _mm_movelh_ps( _mm_castsi128_ps( xy ), z );
	}

	// sqrt(x^2 + y^2 + z^2)
	ID_INLINE float _length3( __m128 v ) //SSE4
	{
		return _mm_cvtss_f32( _mm_sqrt_ss( _mm_dp_ps( v, v, 0x71 ) ) ); // (0x71 = 0111 0001)
	}
	ID_INLINE __m128 _length3v( __m128 v ) //SSE4
	{
		return _mm_sqrt_ss( _mm_dp_ps( v, v, 0x71 ) ); // (0x71 = 0111 0001)
	}

	// (x,y,z) / sqrt(x^2 + y^2 + z^2)
	ID_INLINE __m128 _normalize3( __m128 v ) //SSE4
	{
		__m128 inverse_norm = _mm_rsqrt_ps( _mm_dp_ps( v, v, 0x77 ) ); // (0x77 = 0111 0111)
		return _mm_mul_ps( v, inverse_norm );
	}

	// (x,y,z) / sqrt(x^2 + y^2 + z^2)
	ID_INLINE __m128 _normalize3_accurate( __m128 v ) //SSE4
	{
		__m128 norm = _mm_sqrt_ps( _mm_dp_ps( v, v, 0x7F ) ); // (0x77 = 0111 1111)
		return _mm_div_ps( v, norm );
	}

	/*ID_INLINE __m128 _cross( __m128 a, __m128 b )
	{
		return _mm_sub_ps(
			_mm_mul_ps( _mm_shuffle_ps( a, a, _MM_SHUFFLE( 3, 0, 2, 1 ) ), _mm_shuffle_ps( b, b, _MM_SHUFFLE( 3, 1, 0, 2 ) ) ),
			_mm_mul_ps( _mm_shuffle_ps( a, a, _MM_SHUFFLE( 3, 1, 0, 2 ) ), _mm_shuffle_ps( b, b, _MM_SHUFFLE( 3, 0, 2, 1 ) ) )
		);
	}*/
	ID_INLINE __m128 _cross( __m128 a, __m128 b ) // the same but with 3x _mm_shuffle_ps
	{
		__m128 result = _mm_sub_ps(
			_mm_mul_ps( a, _mm_shuffle_ps( b, b, _MM_SHUFFLE( 3, 0, 2, 1 ) ) ),
			_mm_mul_ps( b, _mm_shuffle_ps( a, a, _MM_SHUFFLE( 3, 0, 2, 1 ) ) )
		);
		return _mm_shuffle_ps( result, result, _MM_SHUFFLE( 3, 0, 2, 1 ) );
	}

	const __m128 _mm_mid4 = { 0.0f, 0.25f, 0.25f, 0.25f };
	const __m128 _mm_SQRT_1OVER2 = { idMath::SQRT_1OVER2 };

#endif

/*
=====================
 R_AutospriteDeform

	Assuming all the triangles for this material are independant
	quads, rebuild them as forward facing sprites.
=====================
*/
static drawSurf_t* R_AutospriteDeform( drawSurf_t* surf )
{
	const idTriangles* srcTri = surf->frontEndGeo;

	if( srcTri->numVerts & 3 )
	{
		common->Warning( "R_AutospriteDeform: shader had odd vertex count" );
		return NULL;
	}
	if( srcTri->numIndexes != ( srcTri->numVerts >> 2 ) * 6 )
	{
		common->Warning( "R_AutospriteDeform: autosprite had odd index count" );
		return NULL;
	}

	const idJointMat* joints = ( srcTri->staticModelWithJoints != NULL )? srcTri->staticModelWithJoints->jointsInverted : NULL;

	idVec3 leftDir, upDir;
	surf->space->modelMatrix.InverseTransformDir( tr.viewDef->GetAxis()[ 1 ], leftDir );
	surf->space->modelMatrix.InverseTransformDir( tr.viewDef->GetAxis()[ 2 ], upDir );

	if( tr.viewDef->isMirror )
	{
		leftDir = vec3_origin - leftDir;
	}

	// the idTriangles are in frame memory and will be automatically disposed of
	auto newTri = allocManager.FrameAlloc<idTriangles, FRAME_ALLOC_SURFACE_TRIANGLES, true>();

	newTri->numVerts = srcTri->numVerts;
	newTri->numIndexes = srcTri->numIndexes;

	auto newVerts = ( idDrawVert* ) _alloca16( ALIGN( srcTri->numVerts * sizeof( idDrawVert ), 16 ) );
	auto newIndexes = ( triIndex_t* ) _alloca16( ALIGN( srcTri->numIndexes * sizeof( triIndex_t ), 16 ) );
#if 0//USE_INTRINSICS
	__m128 _leftDir = _loadV3( leftDir );
	__m128 _upDir = _loadV3( upDir );
#endif
	for( int i = 0; i < srcTri->numVerts; i += 4 )
	{
	#if 0//USE_INTRINSICS
		const __m128 v0 = LoadSkinnedDrawVertPosition( srcTri->GetVertex( i + 0 ), joints );
		const __m128 v1 = LoadSkinnedDrawVertPosition( srcTri->GetVertex( i + 1 ), joints );
		const __m128 v2 = LoadSkinnedDrawVertPosition( srcTri->GetVertex( i + 2 ), joints );
		const __m128 v3 = LoadSkinnedDrawVertPosition( srcTri->GetVertex( i + 3 ), joints );

		__m128 _mid = _mm_mul_ps( _mm_mid4, _mm_add_ps( v3, _mm_add_ps( v2, _mm_add_ps( v1, v0 ) ) ) );
		__m128 _delta = _mm_sub_ps( v0, _mid );
		__m128 _radius = _mm_mul_ps( _length3v( _delta ), _mm_SQRT_1OVER2 );

		__m128 _left = _mm_mul_ps( _leftDir, _radius );
		__m128 _up = _mm_mul_ps( _upDir, _radius );

		__m128 p0 = _mm_add_ps( _mid, _mm_add_ps( _left, _up ) );
		__m128 p1 = _mm_sub_ps( _mid, _mm_add_ps( _left, _up ) );
		__m128 p2 = _mm_sub_ps( _mid, _mm_sub_ps( _left, _up ) );
		__m128 p3 = _mm_add_ps( _mid, _mm_sub_ps( _left, _up ) );

		_mm_store_ps( ( float * ) &newVerts[ i + 0 ], _mm_and_ps( p0, vector_float_mask_clear_last ) );	//xyz00
		_mm_store_ps( ( float * ) &newVerts[ i + 1 ], _mm_or_ps( _mm_and_ps( p1, vector_float_mask_clear_last ), vector_float_last_one ) );	//xyz10
		_mm_store_ps( ( float * ) &newVerts[ i + 2 ], p2 );//SEA: todo
		_mm_store_ps( ( float * ) &newVerts[ i + 3 ], p3 );//SEA: todo
	#else
		// find the midpoint
		newVerts[ i + 0 ] = idDrawVert::GetSkinnedDrawVert( srcTri->GetVertex( i + 0 ), joints );
		newVerts[ i + 1 ] = idDrawVert::GetSkinnedDrawVert( srcTri->GetVertex( i + 1 ), joints );
		newVerts[ i + 2 ] = idDrawVert::GetSkinnedDrawVert( srcTri->GetVertex( i + 2 ), joints );
		newVerts[ i + 3 ] = idDrawVert::GetSkinnedDrawVert( srcTri->GetVertex( i + 3 ), joints );

		idVec3 mid = 0.25f * ( newVerts[ i + 0 ].GetPosition() +
							   newVerts[ i + 1 ].GetPosition() +
							   newVerts[ i + 2 ].GetPosition() +
							   newVerts[ i + 3 ].GetPosition() );

		const idVec3 delta = newVerts[ i + 0 ].GetPosition() - mid;
		const float radius = delta.Length() * idMath::SQRT_1OVER2;

		const idVec3 left = leftDir * radius;
		const idVec3 up = upDir * radius;

		newVerts[ i + 0 ].SetPosition( mid + left + up );
		newVerts[ i + 0 ].SetTexCoord( 0, 0 );

		newVerts[ i + 1 ].SetPosition( mid - left + up );
		newVerts[ i + 1 ].SetTexCoord( 1, 0 );

		newVerts[ i + 2 ].SetPosition( mid - left - up );
		newVerts[ i + 2 ].SetTexCoord( 1, 1 );

		newVerts[ i + 3 ].SetPosition( mid + left - up );
		newVerts[ i + 3 ].SetTexCoord( 0, 1 );
	#endif
		newIndexes[ 6 * ( i >> 2 ) + 0 ] = i + 0;
		newIndexes[ 6 * ( i >> 2 ) + 1 ] = i + 1;
		newIndexes[ 6 * ( i >> 2 ) + 2 ] = i + 2;

		newIndexes[ 6 * ( i >> 2 ) + 3 ] = i + 0;
		newIndexes[ 6 * ( i >> 2 ) + 4 ] = i + 2;
		newIndexes[ 6 * ( i >> 2 ) + 5 ] = i + 3;
	}

	return R_FinishDeform( surf, newTri, newVerts, newIndexes );
}

/*
=====================
 R_TubeDeform

	Will pivot a rectangular quad along the center of its long axis.

	Note that a geometric tube with even quite a few sides will almost certainly render
	much faster than this, so this should only be for faked volumetric tubes.
	Make sure this is used with twosided translucent shaders, because the exact side
	order may not be correct.
=====================
*/
static drawSurf_t* R_TubeDeform( drawSurf_t* surf )
{
	static int edgeVerts[ 6 ][ 2 ] = {
		{ 0, 1 }, { 1, 2 }, { 2, 0 }, { 3, 4 }, { 4, 5 }, { 5, 3 }
	};

	const idTriangles* srcTri = surf->frontEndGeo;

	if( srcTri->numVerts & 3 )
	{
		common->Error( "R_TubeDeform: shader had odd vertex count" );
	}
	if( srcTri->numIndexes != ( srcTri->numVerts >> 2 ) * 6 )
	{
		common->Error( "R_TubeDeform: autosprite had odd index count" );
	}

	const idJointMat* joints = ( srcTri->staticModelWithJoints != NULL )? srcTri->staticModelWithJoints->jointsInverted : NULL;

	// we need the view direction to project the minor axis of the tube
	// as the view changes
	idVec3 localView;
	surf->space->modelMatrix.InverseTransformPoint( tr.viewDef->GetOrigin(), localView );

	// the idTriangles are in frame memory and will be automatically disposed of
	auto newTri = allocManager.FrameAlloc<idTriangles, FRAME_ALLOC_SURFACE_TRIANGLES, true>();

	newTri->numVerts = srcTri->numVerts;
	newTri->numIndexes = srcTri->numIndexes;

	auto newVerts = ( idDrawVert* ) _alloca16( ALIGN( srcTri->numVerts * sizeof( idDrawVert ), 16 ) );
	for( int i = 0; i < srcTri->numVerts; ++i )
	{
		newVerts[ i ].Clear();
	}

	// this is a lot of work for two triangles...
	// we could precalculate a lot if it is an issue, but it would mess up the shader abstraction
	for( int i = 0, indexes = 0; i < srcTri->numVerts; i += 4, indexes += 6 )
	{
		// identify the two shortest edges out of the six defined by the indexes
		int nums[ 2 ] = { 0, 0 };
		float lengths[ 2 ] = { 999999.0f, 999999.0f };

		for( int j = 0; j < 6; j++ )
		{
			const idVec3 v1 = idDrawVert::GetSkinnedDrawVertPosition( srcTri->GetIVertex( i + edgeVerts[ j ][ 0 ] ), joints );
			const idVec3 v2 = idDrawVert::GetSkinnedDrawVertPosition( srcTri->GetIVertex( i + edgeVerts[ j ][ 1 ] ), joints );

			const float l = ( v1 - v2 ).Length();
			if( l < lengths[ 0 ] )
			{
				nums[ 1 ] = nums[ 0 ];
				lengths[ 1 ] = lengths[ 0 ];
				nums[ 0 ] = j;
				lengths[ 0 ] = l;
			}
			else if( l < lengths[ 1 ] )
			{
				nums[ 1 ] = j;
				lengths[ 1 ] = l;
			}
		}

		// find the midpoints of the two short edges, which
		// will give us the major axis in object coordinates
		idVec3 mid[ 2 ];
		for( int j = 0; j < 2; j++ )
		{
			const idVec3 v1 = idDrawVert::GetSkinnedDrawVertPosition( srcTri->GetIVertex( i + edgeVerts[ nums[ j ] ][ 0 ] ), joints );
			const idVec3 v2 = idDrawVert::GetSkinnedDrawVertPosition( srcTri->GetIVertex( i + edgeVerts[ nums[ j ] ][ 1 ] ), joints );

			mid[ j ][ 0 ] = 0.5f * ( v1[ 0 ] + v2[ 0 ] );
			mid[ j ][ 1 ] = 0.5f * ( v1[ 1 ] + v2[ 1 ] );
			mid[ j ][ 2 ] = 0.5f * ( v1[ 2 ] + v2[ 2 ] );
		}

		// find the vector of the major axis
		const idVec3 major = mid[ 1 ] - mid[ 0 ];

		// re-project the points
		for( int j = 0; j < 2; j++ )
		{
			const int i1 = srcTri->indexes[ i + edgeVerts[ nums[ j ] ][ 0 ] ];
			const int i2 = srcTri->indexes[ i + edgeVerts[ nums[ j ] ][ 1 ] ];

			newVerts[ i1 ] = idDrawVert::GetSkinnedDrawVert( srcTri->verts[ i1 ], joints );
			newVerts[ i2 ] = idDrawVert::GetSkinnedDrawVert( srcTri->verts[ i2 ], joints );

			const float l = 0.5f * lengths[ j ];

			// cross this with the view direction to get minor axis
			idVec3 dir = mid[ j ] - localView;
			idVec3 minor;
			minor.Cross( major, dir );
			minor.Normalize();

			if( j )
			{
				newVerts[ i1 ].SetPosition( mid[ j ] - l * minor );
				newVerts[ i2 ].SetPosition( mid[ j ] + l * minor );
			}
			else {
				newVerts[ i1 ].SetPosition( mid[ j ] + l * minor );
				newVerts[ i2 ].SetPosition( mid[ j ] - l * minor );
			}
		}
	}

	return R_FinishDeform( surf, newTri, newVerts, srcTri->indexes );
}

/*
=====================
R_WindingFromTriangles
=====================
*/
#define	MAX_TRI_WINDING_INDEXES		16
static int	R_WindingFromTriangles( const idTriangles* tri, triIndex_t indexes[ MAX_TRI_WINDING_INDEXES ] )
{
	int i, j, k, l;

	indexes[ 0 ] = tri->indexes[ 0 ];
	int numIndexes = 1;
	int	numTris = tri->numIndexes / 3;

	do {
		// find an edge that goes from the current index to another
		// index that isn't already used, and isn't an internal edge
		for( i = 0; i < numTris; ++i )
		{
			for( j = 0; j < 3; j++ )
			{
				if( tri->indexes[ i * 3 + j ] != indexes[ numIndexes - 1 ] )
				{
					continue;
				}
				int next = tri->indexes[ i * 3 + ( j + 1 ) % 3 ];

				// make sure it isn't already used
				if( numIndexes == 1 )
				{
					if( next == indexes[ 0 ] )
					{
						continue;
					}
				}
				else {
					for( k = 1; k < numIndexes; k++ )
					{
						if( indexes[ k ] == next )
						{
							break;
						}
					}
					if( k != numIndexes )
					{
						continue;
					}
				}

				// make sure it isn't an interior edge
				for( k = 0; k < numTris; ++k )
				{
					if( k == i )
					{
						continue;
					}
					for( l = 0; l < 3; l++ )
					{
						int	a, b;

						a = tri->indexes[ k * 3 + l ];
						if( a != next )
						{
							continue;
						}
						b = tri->indexes[ k * 3 + ( l + 1 ) % 3 ];
						if( b != indexes[ numIndexes - 1 ] )
						{
							continue;
						}

						// this is an interior edge
						break;
					}
					if( l != 3 )
					{
						break;
					}
				}
				if( k != numTris )
				{
					continue;
				}

				// add this to the list
				indexes[ numIndexes ] = next;
				numIndexes++;
				break;
			}
			if( j != 3 )
			{
				break;
			}
		}
		if( numIndexes == tri->numVerts )
		{
			break;
		}

	} while( i != numTris );

	return numIndexes;
}

/*
=====================
R_FlareDeform
=====================
*/
static drawSurf_t* R_FlareDeform( drawSurf_t* surf )
{
	const idTriangles* srcTri = surf->frontEndGeo;

	assert( srcTri->staticModelWithJoints == NULL );

	if( srcTri->numVerts != 4 || srcTri->numIndexes != 6 )
	{
		// FIXME: temp hack for flares on tripleted models
		common->Warning( "R_FlareDeform: not a single quad" );
		return NULL;
	}

	// find the plane
	idPlane	plane(
		srcTri->GetIVertex( 0 ).GetPosition(),
		srcTri->GetIVertex( 1 ).GetPosition(),
		srcTri->GetIVertex( 2 ).GetPosition(), true );

	// if viewer is behind the plane, draw nothing
	idVec3 localViewer;
	surf->space->modelMatrix.InverseTransformPoint( tr.viewDef->GetOrigin(), localViewer );
	float distFromPlane = localViewer * plane.Normal() + plane[ 3 ];
	if( distFromPlane <= 0 )
	{
		return NULL;
	}

	idVec3 center = srcTri->verts[ 0 ].GetPosition();
	for( int i = 1; i < srcTri->numVerts; ++i )
	{
		center += srcTri->verts[ i ].GetPosition();
	}
	center *= 1.0f / srcTri->numVerts;

	idVec3 dir = localViewer - center;
	dir.Normalize();

	const float dot = dir * plane.Normal();

	// set vertex colors based on plane angle
	int color = idMath::Ftoi( dot * 8 * 256 );
	if( color > 255 )
	{
		color = 255;
	}

	triIndex_t indexes[ MAX_TRI_WINDING_INDEXES ];
	int numIndexes = R_WindingFromTriangles( srcTri, indexes );

	// only deal with quads
	if( numIndexes != 4 )
	{
		return NULL;
	}

	const int maxVerts = 16;
	const int maxIndexes = 18 * 3;

	// the idTriangles are in frame memory and will be automatically disposed of
	auto newTri = allocManager.FrameAlloc<idTriangles, FRAME_ALLOC_SURFACE_TRIANGLES, true>();

	newTri->numVerts = maxVerts;
	newTri->numIndexes = maxIndexes;

	idDrawVert* newVerts = ( idDrawVert* ) _alloca16( ALIGN( maxVerts * sizeof( idDrawVert ), 16 ) );

	idVec3 edgeDir[ 4 ][ 3 ];

	// calculate vector directions
	for( int i = 0; i < 4; i++ )
	{
		newVerts[ i ].Clear();
		newVerts[ i ].SetPosition( srcTri->verts[ indexes[ i ] ].GetPosition() );
		newVerts[ i ].SetTexCoord( 0.5f, 0.5f );
		newVerts[ i ].color[ 0 ] = color;
		newVerts[ i ].color[ 1 ] = color;
		newVerts[ i ].color[ 2 ] = color;
		newVerts[ i ].color[ 3 ] = 255;

		idVec3 toEye = srcTri->verts[ indexes[ i ] ].GetPosition() - localViewer;
		toEye.Normalize();

		idVec3 d1 = srcTri->verts[ indexes[ ( i + 1 ) % 4 ] ].GetPosition() - localViewer;
		d1.Normalize();
		edgeDir[ i ][ 2 ].Cross( toEye, d1 );
		edgeDir[ i ][ 2 ].Normalize();
		edgeDir[ i ][ 2 ] = vec3_origin - edgeDir[ i ][ 2 ];

		idVec3 d2 = srcTri->verts[ indexes[ ( i + 3 ) % 4 ] ].GetPosition() - localViewer;
		d2.Normalize();
		edgeDir[ i ][ 0 ].Cross( toEye, d2 );
		edgeDir[ i ][ 0 ].Normalize();

		edgeDir[ i ][ 1 ] = edgeDir[ i ][ 0 ] + edgeDir[ i ][ 2 ];
		edgeDir[ i ][ 1 ].Normalize();
	}

	const float spread = surf->shaderRegisters[ surf->material->GetDeformRegister( 0 ) ] * r_flareSize.GetFloat();

	for( int i = 4; i < 16; i++ )
	{
		const int index = ( i - 4 ) / 3;
		idVec3 v = srcTri->verts[ indexes[ index ] ].GetPosition() + spread * edgeDir[ index ][ ( i - 4 ) % 3 ];

		idVec3 dir = v - localViewer;
		const float len = dir.Normalize();
		const float ang = dir * plane.Normal();
		const float newLen = -( distFromPlane / ang );
		if( newLen > 0.0f && newLen < len )
		{
			v = localViewer + dir * newLen;
		}

		newVerts[ i ].Clear();
		newVerts[ i ].SetPosition( v );
		newVerts[ i ].SetTexCoord( 0.0f, 0.5f );
		newVerts[ i ].color[ 0 ] = color;
		newVerts[ i ].color[ 1 ] = color;
		newVerts[ i ].color[ 2 ] = color;
		newVerts[ i ].color[ 3 ] = 255;
	}

	ALIGNTYPE16 static const triIndex_t triIndexes[ 18 * 3 + 10 ] =
	{
		0, 4, 5,  0, 5, 6,  0, 6, 7,  0, 7, 1,  1, 7, 8,  1, 8, 9,
		15, 4, 0, 15, 0, 3,  3, 0, 1,  3, 1, 2,  2, 1, 9,  2, 9, 10,
		14, 15, 3, 14, 3, 13, 13, 3, 2, 13, 2, 12, 12, 2, 11, 11, 2, 10,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0	// to make this a multiple of 16 bytes
	};

	return R_FinishDeform( surf, newTri, newVerts, triIndexes );
}

/*
=====================
R_ExpandDeform

Expands the surface along it's normals by a shader amount
=====================
*/
static drawSurf_t* R_ExpandDeform( drawSurf_t* surf )
{
	const idTriangles* srcTri = surf->frontEndGeo;

	assert( srcTri->staticModelWithJoints == NULL );

	// the idTriangles are in frame memory and will be automatically disposed of
	auto newTri = allocManager.FrameAlloc<idTriangles, FRAME_ALLOC_SURFACE_TRIANGLES, true>();

	newTri->numVerts = srcTri->numVerts;
	newTri->numIndexes = srcTri->numIndexes;

	idDrawVert* newVerts = ( idDrawVert* ) _alloca16( ALIGN( newTri->numVerts * sizeof( idDrawVert ), 16 ) );

	const float dist = surf->shaderRegisters[ surf->material->GetDeformRegister( 0 ) ];
	for( int i = 0; i < srcTri->numVerts; ++i )
	{
		newVerts[ i ] = srcTri->verts[ i ];
		newVerts[ i ].SetPosition( srcTri->verts[ i ].GetPosition() + srcTri->verts[ i ].GetNormal() * dist );
	}

	return R_FinishDeform( surf, newTri, newVerts, srcTri->indexes );
}

/*
=====================
R_MoveDeform

Moves the surface along the X axis, mostly just for demoing the deforms
=====================
*/
static drawSurf_t* R_MoveDeform( drawSurf_t* surf )
{
	const idTriangles* srcTri = surf->frontEndGeo;

	assert( srcTri->staticModelWithJoints == NULL );

	// the idTriangles are in frame memory and will be automatically disposed of
	auto newTri = allocManager.FrameAlloc<idTriangles, FRAME_ALLOC_SURFACE_TRIANGLES, true>();

	newTri->numVerts = srcTri->numVerts;
	newTri->numIndexes = srcTri->numIndexes;

	idDrawVert* newVerts = ( idDrawVert* ) _alloca16( ALIGN( newTri->numVerts * sizeof( idDrawVert ), 16 ) );

	const float dist = surf->shaderRegisters[ surf->material->GetDeformRegister( 0 ) ];
	for( int i = 0; i < srcTri->numVerts; ++i )
	{
		newVerts[ i ] = srcTri->verts[ i ];
		///newVerts[ i ].xyz.x += dist;
		auto pos = newVerts[ i ].GetPosition();
		newVerts[ i ].SetPosition( pos.x + dist, pos.y, pos.z );
	}

	return R_FinishDeform( surf, newTri, newVerts, srcTri->indexes );
}

/*
=====================
R_TurbulentDeform

Turbulently deforms the texture coordinates.
=====================
*/
static drawSurf_t* R_TurbulentDeform( drawSurf_t* surf )
{
	const idTriangles* srcTri = surf->frontEndGeo;

	assert( srcTri->staticModelWithJoints == NULL );

	// the idTriangles are in frame memory and will be automatically disposed of
	auto newTri = allocManager.FrameAlloc<idTriangles, FRAME_ALLOC_SURFACE_TRIANGLES, true>();

	newTri->numVerts = srcTri->numVerts;
	newTri->numIndexes = srcTri->numIndexes;

	idDrawVert* newVerts = ( idDrawVert* ) _alloca16( ALIGN( newTri->numVerts * sizeof( idDrawVert ), 16 ) );

	const idDeclTable* table = ( const idDeclTable* ) surf->material->GetDeformDecl();
	const float range   = surf->shaderRegisters[ surf->material->GetDeformRegister( 0 ) ];
	const float timeOfs = surf->shaderRegisters[ surf->material->GetDeformRegister( 1 ) ];
	const float domain  = surf->shaderRegisters[ surf->material->GetDeformRegister( 2 ) ];
	const float tOfs = 0.5f;

	for( int i = 0; i < srcTri->numVerts; ++i )
	{
		auto & pos = srcTri->verts[ i ].GetPosition();
		float f = pos[ 0 ] * 0.003f +
				  pos[ 1 ] * 0.007f +
				  pos[ 2 ] * 0.011f;

		f = timeOfs + domain * f;
		f += timeOfs;

		idVec2 tempST = srcTri->verts[ i ].GetTexCoord();
		tempST[ 0 ] += range * table->TableLookup( f );
		tempST[ 1 ] += range * table->TableLookup( f + tOfs );

		newVerts[ i ] = srcTri->verts[ i ];
		newVerts[ i ].SetTexCoord( tempST );
	}

	return R_FinishDeform( surf, newTri, newVerts, srcTri->indexes );
}

/*
=====================
AddTriangleToIsland_r
=====================
*/
#define	MAX_EYEBALL_TRIS	10
#define	MAX_EYEBALL_ISLANDS	6

struct eyeIsland_t {
	int			tris[ MAX_EYEBALL_TRIS ];
	int			numTris;
	idBounds	bounds;
	idVec3		mid;
};

static void AddTriangleToIsland_r( const idTriangles* tri, int triangleNum, bool* usedList, eyeIsland_t* island )
{
	usedList[ triangleNum ] = true;

	// add to the current island
	if( island->numTris >= MAX_EYEBALL_TRIS )
	{
		common->Error( "MAX_EYEBALL_TRIS" );
		return;
	}
	island->tris[ island->numTris ] = triangleNum;
	island->numTris++;

	const idJointMat* joints = ( tri->staticModelWithJoints != NULL )? tri->staticModelWithJoints->jointsInverted : NULL;

	// recurse into all neighbors
	const int a = tri->indexes[ triangleNum * 3 + 0 ];
	const int b = tri->indexes[ triangleNum * 3 + 1 ];
	const int c = tri->indexes[ triangleNum * 3 + 2 ];

	const idVec3 va = idDrawVert::GetSkinnedDrawVertPosition( tri->verts[ a ], joints );
	const idVec3 vb = idDrawVert::GetSkinnedDrawVertPosition( tri->verts[ b ], joints );
	const idVec3 vc = idDrawVert::GetSkinnedDrawVertPosition( tri->verts[ c ], joints );

	island->bounds.AddPoint( va );
	island->bounds.AddPoint( vb );
	island->bounds.AddPoint( vc );

	const int numTri = tri->numIndexes / 3;
	for( int i = 0; i < numTri; ++i )
	{
		if( usedList[ i ] )
		{
			continue;
		}
		if( tri->indexes[ i * 3 + 0 ] == a ||
			tri->indexes[ i * 3 + 1 ] == a ||
			tri->indexes[ i * 3 + 2 ] == a ||
			tri->indexes[ i * 3 + 0 ] == b ||
			tri->indexes[ i * 3 + 1 ] == b ||
			tri->indexes[ i * 3 + 2 ] == b ||
			tri->indexes[ i * 3 + 0 ] == c ||
			tri->indexes[ i * 3 + 1 ] == c ||
			tri->indexes[ i * 3 + 2 ] == c )
		{
			AddTriangleToIsland_r( tri, i, usedList, island );
		}
	}
}

/*
=====================
R_EyeballDeform

Each eyeball surface should have an separate upright triangle behind it, long end
pointing out the eye, and another single triangle in front of the eye for the focus point.
=====================
*/
static drawSurf_t* R_EyeballDeform( drawSurf_t* surf )
{
	const idTriangles* srcTri = surf->frontEndGeo;

	// separate all the triangles into islands
	const int numTri = srcTri->numIndexes / 3;
	if( numTri > MAX_EYEBALL_ISLANDS * MAX_EYEBALL_TRIS )
	{
		common->Printf( "R_EyeballDeform: too many triangles in surface" );
		return NULL;
	}

	eyeIsland_t islands[ MAX_EYEBALL_ISLANDS ];
	bool triUsed[ MAX_EYEBALL_ISLANDS * MAX_EYEBALL_TRIS ] = { 0 };

	int numIslands = 0;
	for( ; numIslands < MAX_EYEBALL_ISLANDS; numIslands++ )
	{
		islands[ numIslands ].numTris = 0;
		islands[ numIslands ].bounds.Clear();
		int i;
		for( i = 0; i < numTri; i++ )
		{
			if( !triUsed[ i ] )
			{
				AddTriangleToIsland_r( srcTri, i, triUsed, &islands[ numIslands ] );
				break;
			}
		}
		if( i == numTri )
		{
			break;
		}
	}

	// assume we always have two eyes, two origins, and two targets
	if( numIslands != 3 )
	{
		common->Printf( "R_EyeballDeform: %i triangle islands\n", numIslands );
		return NULL;
	}

	const idJointMat* joints = ( srcTri->staticModelWithJoints != NULL )? srcTri->staticModelWithJoints->jointsInverted : NULL;

	// the idTriangles are in frame memory and will be automatically disposed of
	auto newTri = allocManager.FrameAlloc<idTriangles, FRAME_ALLOC_SURFACE_TRIANGLES, true>();

	newTri->numVerts = srcTri->numVerts;
	newTri->numIndexes = srcTri->numIndexes;

	auto newVerts = ( idDrawVert* ) _alloca16( ALIGN( srcTri->numVerts * sizeof( idDrawVert ), 16 ) );
	auto newIndexes = ( triIndex_t* ) _alloca16( ALIGN( srcTri->numIndexes * sizeof( triIndex_t ), 16 ) );

	// decide which islands are the eyes and points
	for( int i = 0; i < numIslands; i++ )
	{
		islands[ i ].mid = islands[ i ].bounds.GetCenter();
	}

	int numIndexes = 0;
	for( int i = 0; i < numIslands; i++ )
	{
		eyeIsland_t* island = &islands[ i ];

		if( island->numTris == 1 )
		{
			continue;
		}

		// the closest single triangle point will be the eye origin
		// and the next-to-farthest will be the focal point
		idVec3 origin;
		idVec3 focus;
		int originIsland = 0;
		float dist[ MAX_EYEBALL_ISLANDS ];
		int sortOrder[ MAX_EYEBALL_ISLANDS ];

		for( int j = 0; j < numIslands; ++j )
		{
			idVec3 dir = islands[ j ].mid - island->mid;
			dist[ j ] = dir.Length();
			sortOrder[ j ] = j;
			for( int k = j - 1; k >= 0; --k )
			{
				if( dist[ k ] > dist[ k + 1 ] )
				{
					SwapValues( sortOrder[ k ], sortOrder[ k + 1 ] );
					SwapValues( dist[ k ], dist[ k + 1 ] );
				}
			}
		}

		originIsland = sortOrder[ 1 ];
		origin = islands[ originIsland ].mid;

		focus = islands[ sortOrder[ 2 ] ].mid;

		// determine the projection directions based on the origin island triangle
		idVec3 dir = focus - origin; dir.Normalize();

		const idVec3 p1 = idDrawVert::GetSkinnedDrawVertPosition( srcTri->GetIVertex( islands[ originIsland ].tris[ 0 ] + 0 ), joints );
		const idVec3 p2 = idDrawVert::GetSkinnedDrawVertPosition( srcTri->GetIVertex( islands[ originIsland ].tris[ 0 ] + 1 ), joints );
		const idVec3 p3 = idDrawVert::GetSkinnedDrawVertPosition( srcTri->GetIVertex( islands[ originIsland ].tris[ 0 ] + 2 ), joints );

		idVec3 v1 = p2 - p1; v1.Normalize();
		idVec3 v2 = p3 - p1; v2.Normalize();

		// texVec[0] will be the normal to the origin triangle
		idVec3 texVec[ 2 ];
		texVec[ 0 ].Cross( v1, v2 );
		texVec[ 1 ].Cross( texVec[ 0 ], dir );

		for( int j = 0; j < 2; j++ )
		{
			texVec[ j ] -= dir * ( texVec[ j ] * dir );
			texVec[ j ].Normalize();
		}

		// emit these triangles, generating the projected texcoords
		for( int j = 0; j < islands[ i ].numTris; ++j )
		{
			for( int k = 0; k < 3; k++ )
			{
				int	index = islands[ i ].tris[ j ] * 3;

				index = srcTri->indexes[ index + k ];
				newIndexes[ numIndexes++ ] = index;

				newVerts[ index ] = idDrawVert::GetSkinnedDrawVert( srcTri->verts[ index ], joints );

				const idVec3 local = newVerts[ index ].GetPosition() - origin;
				newVerts[ index ].SetTexCoord( 0.5f + local * texVec[ 0 ], 0.5f + local * texVec[ 1 ] );
			}
		}
	}

	newTri->numIndexes = numIndexes;

	return R_FinishDeform( surf, newTri, newVerts, newIndexes );
}

/*
=====================
R_ParticleDeform

Emit particles from the surface.
=====================
*/
static drawSurf_t* R_ParticleDeform( drawSurf_t* surf, bool useArea )
{
	const renderEntityParms_t* renderEntity = &surf->space->entityDef->GetParms();
	const idRenderView* viewDef = tr.viewDef;
	const idDeclParticle* particleSystem = ( const idDeclParticle* ) surf->material->GetDeformDecl();
	const idTriangles* srcTri = surf->frontEndGeo;

	if( r_skipParticles.GetBool() )
	{
		return NULL;
	}

	//
	// calculate the area of all the triangles
	//
	int numSourceTris = surf->frontEndGeo->GetNumFaces();
	float totalArea = 0.0f;
	float* sourceTriAreas = NULL;

	const idJointMat* joints = ( srcTri->staticModelWithJoints != NULL )? srcTri->staticModelWithJoints->jointsInverted : NULL;

	if( useArea )
	{
		sourceTriAreas = ( float* ) _alloca( sizeof( *sourceTriAreas ) * numSourceTris );
		int	triNum = 0;
		for( int i = 0; i < srcTri->numIndexes; i += 3, triNum++ )
		{
			float area = idWinding::TriangleArea( idDrawVert::GetSkinnedDrawVertPosition( srcTri->GetIVertex( i + 0 ), joints ),
												  idDrawVert::GetSkinnedDrawVertPosition( srcTri->GetIVertex( i + 1 ), joints ),
												  idDrawVert::GetSkinnedDrawVertPosition( srcTri->GetIVertex( i + 2 ), joints ) );
			sourceTriAreas[ triNum ] = totalArea;
			totalArea += area;
		}
	}

	//
	// create the particles almost exactly the way idRenderModelPrt does
	//
	particleGen_t g;

	g.renderEnt = renderEntity;
	g.renderView = &viewDef->GetParms();
	g.origin.Zero();
	g.axis = mat3_identity;

	int maxStageParticles[ MAX_PARTICLE_STAGES ] = { 0 };
	int maxStageQuads[ MAX_PARTICLE_STAGES ] = { 0 };
	int maxQuads = 0;

	for( int stageNum = 0; stageNum < particleSystem->stages.Num(); ++stageNum )
	{
		idParticleStage* stage = particleSystem->stages[ stageNum ];

		if( stage->material == NULL )
		{
			continue;
		}
		if( stage->cycleMsec == 0 )
		{
			continue;
		}
		if( stage->hidden )  		// just for gui particle editor use
		{
			continue;
		}

		// we interpret stage->totalParticles as "particles per map square area"
		// so the systems look the same on different size surfaces
		const int totalParticles = ( useArea ) ? idMath::Ftoi( stage->totalParticles * totalArea * ( 1.0f / 4096.0f ) ) : ( stage->totalParticles );
		const int numQuads = totalParticles * stage->NumQuadsPerParticle() * ( ( useArea ) ? 1 : numSourceTris );

		maxStageParticles[ stageNum ] = totalParticles;
		maxStageQuads[ stageNum ] = numQuads;
		maxQuads = idMath::Max( maxQuads, numQuads );
	}

	if( maxQuads == 0 )
	{
		return NULL;
	}

	idTempArray<byte> tempVerts( ALIGN( maxQuads * 4 * sizeof( idDrawVert ), 16 ) );
	idDrawVert* newVerts = ( idDrawVert* ) tempVerts.Ptr();
	idTempArray<byte> tempIndex( ALIGN( maxQuads * 6 * sizeof( triIndex_t ), 16 ) );
	triIndex_t* newIndexes = ( triIndex_t* ) tempIndex.Ptr();

	drawSurf_t* drawSurfList = NULL;

	for( int stageNum = 0; stageNum < particleSystem->stages.Num(); stageNum++ )
	{
		if( maxStageQuads[ stageNum ] == 0 )
		{
			continue;
		}

		idParticleStage* stage = particleSystem->stages[ stageNum ];

		int numVerts = 0;
		for( int currentTri = 0; currentTri < ( ( useArea ) ? 1 : numSourceTris ); currentTri++ )
		{
			idRandom steppingRandom;
			idRandom steppingRandom2;

			int stageAge = g.renderView->time[ renderEntity->timeGroup ] + idMath::Ftoi( renderEntity->shaderParms[ SHADERPARM_TIMEOFFSET ] * 1000.0f - stage->timeOffset * 1000.0f );
			int stageCycle = stageAge / stage->cycleMsec;

			// some particles will be in this cycle, some will be in the previous cycle
			steppingRandom.SetSeed( ( ( stageCycle << 10 ) & idRandom::MAX_RAND ) ^ idMath::Ftoi( renderEntity->shaderParms[ SHADERPARM_DIVERSITY ] * idRandom::MAX_RAND ) );
			steppingRandom2.SetSeed( ( ( ( stageCycle - 1 ) << 10 ) & idRandom::MAX_RAND ) ^ idMath::Ftoi( renderEntity->shaderParms[ SHADERPARM_DIVERSITY ] * idRandom::MAX_RAND ) );

			for( int index = 0; index < maxStageParticles[ stageNum ]; index++ )
			{
				g.index = index;

				// bump the random
				steppingRandom.RandomInt();
				steppingRandom2.RandomInt();

				// calculate local age for this index
				int bunchOffset = idMath::Ftoi( stage->particleLife * 1000 * stage->spawnBunching * index / maxStageParticles[ stageNum ] );

				int particleAge = stageAge - bunchOffset;
				int particleCycle = particleAge / stage->cycleMsec;
				if( particleCycle < 0 )
				{
					// before the particleSystem spawned
					continue;
				}
				if( stage->cycles != 0.0f && particleCycle >= stage->cycles )
				{
					// cycled systems will only run cycle times
					continue;
				}

				int inCycleTime = particleAge - particleCycle * stage->cycleMsec;

				if( renderEntity->shaderParms[ SHADERPARM_PARTICLE_STOPTIME ] != 0.0f &&
					g.renderView->time[ renderEntity->timeGroup ] - inCycleTime >= SEC2MS( renderEntity->shaderParms[ SHADERPARM_PARTICLE_STOPTIME ] ) )
				{
					// don't fire any more particles
					continue;
				}

				// supress particles before or after the age clamp
				g.frac = ( float ) inCycleTime / SEC2MS( stage->particleLife );
				if( g.frac < 0.0f )
				{
					// yet to be spawned
					continue;
				}
				if( g.frac > 1.0f )
				{
					// this particle is in the deadTime band
					continue;
				}

				if( particleCycle == stageCycle )
				{
					g.random = steppingRandom;
				}
				else
				{
					g.random = steppingRandom2;
				}

				//---------------
				// locate the particle origin and axis somewhere on the surface
				//---------------

				int pointTri = currentTri;

				if( useArea )
				{
					// select a triangle based on an even area distribution
					pointTri = idBinSearch::LessEqual<float>( sourceTriAreas, numSourceTris, g.random.RandomFloat() * totalArea );
				}

				// now pick a random point inside pointTri
				const idDrawVert v1 = idDrawVert::GetSkinnedDrawVert( srcTri->GetIVertex( pointTri * 3 + 0 ), joints );
				const idDrawVert v2 = idDrawVert::GetSkinnedDrawVert( srcTri->GetIVertex( pointTri * 3 + 1 ), joints );
				const idDrawVert v3 = idDrawVert::GetSkinnedDrawVert( srcTri->GetIVertex( pointTri * 3 + 2 ), joints );

				float f1 = g.random.RandomFloat();
				float f2 = g.random.RandomFloat();
				float f3 = g.random.RandomFloat();

				float ft = 1.0f / ( f1 + f2 + f3 + 0.0001f );

				f1 *= ft;
				f2 *= ft;
				f3 *= ft;

				g.origin = v1.GetPosition()  * f1 + v2.GetPosition()  * f2 + v3.GetPosition()  * f3;
				g.axis[ 0 ] = v1.GetTangent()   * f1 + v2.GetTangent()   * f2 + v3.GetTangent()   * f3;
				g.axis[ 1 ] = v1.GetBiTangent() * f1 + v2.GetBiTangent() * f2 + v3.GetBiTangent() * f3;
				g.axis[ 2 ] = v1.GetNormal()    * f1 + v2.GetNormal()    * f2 + v3.GetNormal()    * f3;

				// this is needed so aimed particles can calculate origins at different times
				g.originalRandom = g.random;

				g.age = g.frac * stage->particleLife;

				// if the particle doesn't get drawn because it is faded out or beyond a kill region,
				// don't increment the verts
				numVerts += stage->CreateParticle( &g, newVerts + numVerts );
			}
		}

		if( numVerts == 0 )
		{
			continue;
		}

		// build the index list
		int numIndexes = 0;
		idTriangles::CreateIndexesForQuads( numVerts, newIndexes, numIndexes );

		// allocate a srfTriangles in temp memory that can hold all the particles
		auto newTri = allocManager.FrameAlloc<idTriangles, FRAME_ALLOC_SURFACE_TRIANGLES, true>();

		newTri->numVerts = numVerts;
		newTri->numIndexes = numIndexes;

		///newTri->verts = newVerts;
		///newTri->indexes = newIndexes;

		newTri->bounds = stage->bounds;	// just always draw the particles

		auto drawSurf = allocManager.FrameAlloc<drawSurf_t, FRAME_ALLOC_DRAW_SURFACE>();

		///newTri->InitDrawSurfFromTriangles( *drawSurf );
		R_FinishDeform( drawSurf, newTri, newVerts, newIndexes );

		drawSurf->space = surf->space;
		drawSurf->scissorRect = surf->scissorRect;
		drawSurf->extraGLState = 0;
		drawSurf->renderZFail = 0;

		R_SetupDrawSurfShader( drawSurf, stage->material, renderEntity, viewDef );

		//drawSurf->linkChain = NULL;
		drawSurf->nextOnLight = drawSurfList;
		drawSurfList = drawSurf;
	}

	return drawSurfList;
}

class idRenderDrawSurfaceFactory {
public:
};

/*
=================
R_DeformDrawSurf
=================
*/
drawSurf_t* R_DeformDrawSurf( drawSurf_t* drawSurf )
{
	if( drawSurf->material == NULL )
	{
		return NULL;
	}

	if( r_skipDeforms.GetBool() )
	{
		return drawSurf;
	}
	switch( drawSurf->material->GetDeformType() )
	{
		case DFRM_SPRITE: return R_AutospriteDeform( drawSurf );
		case DFRM_TUBE: return R_TubeDeform( drawSurf );
		case DFRM_FLARE: return R_FlareDeform( drawSurf );
		case DFRM_EXPAND: return R_ExpandDeform( drawSurf );
		case DFRM_MOVE: return R_MoveDeform( drawSurf );
		case DFRM_TURB: return R_TurbulentDeform( drawSurf );
		case DFRM_EYEBALL: return R_EyeballDeform( drawSurf );
		case DFRM_PARTICLE: return R_ParticleDeform( drawSurf, true );
		case DFRM_PARTICLE2: return R_ParticleDeform( drawSurf, false );

		default: return NULL;
	}
}