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

#include "../precompiled.h"
#pragma hdrstop

#include "Winding2D.h"

/*
============
GetAxialBevel
============
*/
static bool GetAxialBevel( const idVec3& plane1, const idVec3& plane2, const idVec2& point, idVec3& bevel )
{
	if( IEEE_FLT_SIGNBITSET( plane1.x ) ^ IEEE_FLT_SIGNBITSET( plane2.x ) )
	{
		if( idMath::Fabs( plane1.x ) > 0.1f && idMath::Fabs( plane2.x ) > 0.1f )
		{
			bevel.x = 0.0f;
			if( IEEE_FLT_SIGNBITSET( plane1.y ) )
			{
				bevel.y = -1.0f;
			}
			else {
				bevel.y = 1.0f;
			}
			bevel.z = -( point.x * bevel.x + point.y * bevel.y );
			return true;
		}
	}
	if( IEEE_FLT_SIGNBITSET( plane1.y ) ^ IEEE_FLT_SIGNBITSET( plane2.y ) )
	{
		if( idMath::Fabs( plane1.y ) > 0.1f && idMath::Fabs( plane2.y ) > 0.1f )
		{
			bevel.y = 0.0f;
			if( IEEE_FLT_SIGNBITSET( plane1.x ) )
			{
				bevel.x = -1.0f;
			}
			else {
				bevel.x = 1.0f;
			}
			bevel.z = -( point.x * bevel.x + point.y * bevel.y );
			return true;
		}
	}
	return false;
}

/*
============
idWinding2D::ExpandForAxialBox
============
*/
void idWinding2D::ExpandForAxialBox( const idVec2 bounds[ 2 ] )
{
	int i, j, numPlanes;
	idVec2 v;
	idVec3 planes[ MAX_POINTS ], plane, bevel;

	// get planes for the edges and add bevels
	for( numPlanes = i = 0; i < numPoints; i++ )
	{
		j = ( i + 1 ) % numPoints;
		if( ( p[ j ] - p[ i ] ).LengthSqr() < 0.01f )
		{
			continue;
		}
		Plane2DFromPoints( plane, p[ i ], p[ j ], true );
		if( i )
		{
			if( GetAxialBevel( planes[ numPlanes - 1 ], plane, p[ i ], bevel ) )
			{
				planes[ numPlanes++ ] = bevel;
			}
		}
		assert( numPlanes < MAX_POINTS );
		planes[ numPlanes++ ] = plane;
	}
	assert( numPlanes < MAX_POINTS && numPlanes > 0 );
	if( GetAxialBevel( planes[ numPlanes - 1 ], planes[ 0 ], p[ 0 ], bevel ) )
	{
		planes[ numPlanes++ ] = bevel;
	}

	// expand the planes
	for( i = 0; i < numPlanes; i++ )
	{
		v.x = bounds[ IEEE_FLT_SIGNBITSET( planes[ i ].x ) ].x;
		v.y = bounds[ IEEE_FLT_SIGNBITSET( planes[ i ].y ) ].y;
		planes[ i ].z += v.x * planes[ i ].x + v.y * planes[ i ].y;
	}

	// get intersection points of the planes
	for( numPoints = i = 0; i < numPlanes; i++ )
	{
		if( Plane2DIntersection( planes[ ( i + numPlanes - 1 ) % numPlanes ], planes[ i ], p[ numPoints ] ) )
		{
			numPoints++;
		}
	}
}

/*
============
idWinding2D::Expand
============
*/
void idWinding2D::Expand( const float d )
{
	idVec2 edgeNormals[ MAX_POINTS ];

	for( int i = 0; i < numPoints; i++ )
	{
		idVec2& start = p[ i ];
		idVec2& end = p[ ( i + 1 ) % numPoints ];
		edgeNormals[ i ].x = start.y - end.y;
		edgeNormals[ i ].y = end.x - start.x;
		edgeNormals[ i ].Normalize();
		edgeNormals[ i ] *= d;
	}

	for( int i = 0; i < numPoints; i++ )
	{
		p[ i ] += edgeNormals[ i ] + edgeNormals[ ( i + numPoints - 1 ) % numPoints ];
	}
}

/*
=============
idWinding2D::Split
=============
*/
int idWinding2D::Split( const idVec3& plane, const float epsilon, idWinding2D** front, idWinding2D** back ) const
{
	float			dists[ MAX_POINTS ];
	byte			sides[ MAX_POINTS ];
	int				counts[ 3 ];
	float			dot;
	int				i, j;
	const idVec2* 	p1, *p2;
	idVec2			mid;
	idWinding2D* 	f;
	idWinding2D* 	b;
	int				maxpts;

	counts[ 0 ] = counts[ 1 ] = counts[ 2 ] = 0;

	// determine sides for each point
	for( i = 0; i < numPoints; i++ )
	{
		dists[ i ] = dot = plane.x * p[ i ].x + plane.y * p[ i ].y + plane.z;
		if( dot > epsilon )
		{
			sides[ i ] = SIDE_FRONT;
		}
		else if( dot < -epsilon )
		{
			sides[ i ] = SIDE_BACK;
		}
		else {
			sides[ i ] = SIDE_ON;
		}
		counts[ sides[ i ] ]++;
	}
	sides[ i ] = sides[ 0 ];
	dists[ i ] = dists[ 0 ];

	*front = *back = NULL;

	// if nothing at the front of the clipping plane
	if( !counts[ SIDE_FRONT ] )
	{
		*back = Copy();
		return SIDE_BACK;
	}
	// if nothing at the back of the clipping plane
	if( !counts[ SIDE_BACK ] )
	{
		*front = Copy();
		return SIDE_FRONT;
	}

	maxpts = numPoints + 4;	// cant use counts[0]+2 because of fp grouping errors

	*front = f = new( TAG_IDLIB_WINDING ) idWinding2D;
	*back = b = new( TAG_IDLIB_WINDING ) idWinding2D;

	for( i = 0; i < numPoints; i++ )
	{
		p1 = &p[ i ];

		if( sides[ i ] == SIDE_ON )
		{
			f->p[ f->numPoints ] = *p1;
			f->numPoints++;
			b->p[ b->numPoints ] = *p1;
			b->numPoints++;
			continue;
		}

		if( sides[ i ] == SIDE_FRONT )
		{
			f->p[ f->numPoints ] = *p1;
			f->numPoints++;
		}

		if( sides[ i ] == SIDE_BACK )
		{
			b->p[ b->numPoints ] = *p1;
			b->numPoints++;
		}

		if( sides[ i + 1 ] == SIDE_ON || sides[ i + 1 ] == sides[ i ] )
		{
			continue;
		}

		// generate a split point
		p2 = &p[ ( i + 1 ) % numPoints ];

		// always calculate the split going from the same side
		// or minor epsilon issues can happen
		if( sides[ i ] == SIDE_FRONT )
		{
			dot = dists[ i ] / ( dists[ i ] - dists[ i + 1 ] );
			for( j = 0; j < 2; j++ )
			{
				// avoid round off error when possible
				if( plane[ j ] == 1.0f )
				{
					mid[ j ] = plane.z;
				}
				else if( plane[ j ] == -1.0f )
				{
					mid[ j ] = -plane.z;
				}
				else {
					mid[ j ] = ( *p1 )[ j ] + dot * ( ( *p2 )[ j ] - ( *p1 )[ j ] );
				}
			}
		}
		else {
			dot = dists[ i + 1 ] / ( dists[ i + 1 ] - dists[ i ] );
			for( j = 0; j < 2; j++ )
			{
				// avoid round off error when possible
				if( plane[ j ] == 1.0f )
				{
					mid[ j ] = plane.z;
				}
				else if( plane[ j ] == -1.0f )
				{
					mid[ j ] = -plane.z;
				}
				else
				{
					mid[ j ] = ( *p2 )[ j ] + dot * ( ( *p1 )[ j ] - ( *p2 )[ j ] );
				}
			}
		}

		f->p[ f->numPoints ] = mid;
		f->numPoints++;
		b->p[ b->numPoints ] = mid;
		b->numPoints++;
	}

	return SIDE_CROSS;
}

/*
============
idWinding2D::ClipInPlace
============
*/
bool idWinding2D::ClipInPlace( const idVec3& plane, const float epsilon, const bool keepOn )
{
	int i, j, maxpts, newNumPoints;
	int sides[ MAX_POINTS + 1 ], counts[ 3 ];
	float dot, dists[ MAX_POINTS + 1 ];
	idVec2* p1, *p2, mid, newPoints[ MAX_POINTS + 4 ];

	counts[ SIDE_FRONT ] = counts[ SIDE_BACK ] = counts[ SIDE_ON ] = 0;

	for( i = 0; i < numPoints; i++ )
	{
		dists[ i ] = dot = plane.x * p[ i ].x + plane.y * p[ i ].y + plane.z;
		if( dot > epsilon )
		{
			sides[ i ] = SIDE_FRONT;
		}
		else if( dot < -epsilon )
		{
			sides[ i ] = SIDE_BACK;
		}
		else {
			sides[ i ] = SIDE_ON;
		}
		counts[ sides[ i ] ]++;
	}
	sides[ i ] = sides[ 0 ];
	dists[ i ] = dists[ 0 ];

	// if the winding is on the plane and we should keep it
	if( keepOn && !counts[ SIDE_FRONT ] && !counts[ SIDE_BACK ] )
	{
		return true;
	}
	if( !counts[ SIDE_FRONT ] )
	{
		numPoints = 0;
		return false;
	}
	if( !counts[ SIDE_BACK ] )
	{
		return true;
	}

	maxpts = numPoints + 4;		// cant use counts[0]+2 because of fp grouping errors
	newNumPoints = 0;

	for( i = 0; i < numPoints; i++ )
	{
		p1 = &p[ i ];

		if( newNumPoints + 1 > maxpts )
		{
			return true;		// can't split -- fall back to original
		}

		if( sides[ i ] == SIDE_ON )
		{
			newPoints[ newNumPoints ] = *p1;
			newNumPoints++;
			continue;
		}

		if( sides[ i ] == SIDE_FRONT )
		{
			newPoints[ newNumPoints ] = *p1;
			newNumPoints++;
		}

		if( sides[ i + 1 ] == SIDE_ON || sides[ i + 1 ] == sides[ i ] )
		{
			continue;
		}

		if( newNumPoints + 1 > maxpts )
		{
			return true;		// can't split -- fall back to original
		}

		// generate a split point
		p2 = &p[ ( i + 1 ) % numPoints ];

		dot = dists[ i ] / ( dists[ i ] - dists[ i + 1 ] );
		for( j = 0; j < 2; j++ )
		{
			// avoid round off error when possible
			if( plane[ j ] == 1.0f )
			{
				mid[ j ] = plane.z;
			}
			else if( plane[ j ] == -1.0f )
			{
				mid[ j ] = -plane.z;
			}
			else
			{
				mid[ j ] = ( *p1 )[ j ] + dot * ( ( *p2 )[ j ] - ( *p1 )[ j ] );
			}
		}

		newPoints[ newNumPoints ] = mid;
		newNumPoints++;
	}

	if( newNumPoints >= MAX_POINTS )
	{
		return true;
	}

	numPoints = newNumPoints;
	memcpy( p, newPoints, newNumPoints * sizeof( idVec2 ) );

	return true;
}

/*
=============
idWinding2D::Copy
=============
*/
idWinding2D* idWinding2D::Copy() const
{
	auto w = new( TAG_IDLIB_WINDING ) idWinding2D;
	w->numPoints = numPoints;
	memcpy( w->p, p, numPoints * sizeof( p[ 0 ] ) );
	return w;
}

/*
=============
idWinding2D::Reverse
=============
*/
idWinding2D* idWinding2D::Reverse() const
{
	auto w = new( TAG_IDLIB_WINDING ) idWinding2D;
	w->numPoints = numPoints;
	for( int i = 0; i < numPoints; i++ )
	{
		w->p[ numPoints - i - 1 ] = p[ i ];
	}
	return w;
}

/*
=============
idWinding2D::Reverse
=============
*/
idWinding2D& idWinding2D::ReverseSelf( void )
{
	for( int i = 0; i < ( numPoints >> 1 ); i++ )
	{
		SwapValues( p[i], p[numPoints - i - 1] );
		///SwapValues( st[i], st[numPoints - i - 1] );
	}
	return *this;
}

/*
============
idWinding2D::GetArea
============
*/
float idWinding2D::GetArea() const
{
	idVec2 d1, d2;
	float total = 0.0f;
	for( int i = 2; i < numPoints; i++ )
	{
		d1 = p[ i - 1 ] - p[ 0 ];
		d2 = p[ i ] - p[ 0 ];
		total += d1.x * d2.y - d1.y * d2.x;
	}
	return total * 0.5f;
}

/*
============
idWinding2D::GetCenter
============
*/
idVec2 idWinding2D::GetCenter() const
{
	idVec2 center;
	center.Zero();
	for( int i = 0; i < numPoints; i++ )
	{
		center += p[ i ];
	}
	center *= ( 1.0f / numPoints );
	return center;
}

/*
============
idWinding2D::GetRadius
============
*/
float idWinding2D::GetRadius( const idVec2& center ) const
{
	idVec2 dir;
	float radius = 0.0f;
	for( int i = 0; i < numPoints; i++ )
	{
		dir = p[ i ] - center;
		float r = dir * dir;
		if( r > radius )
		{
			radius = r;
		}
	}
	return idMath::Sqrt( radius );
}

/*
============
idWinding2D::GetBounds
============
*/
void idWinding2D::GetBounds( idVec2 bounds[ 2 ] ) const
{
	if( !numPoints )
	{
		bounds[ 0 ].x = bounds[ 0 ].y = idMath::INFINITY;
		bounds[ 1 ].x = bounds[ 1 ].y = -idMath::INFINITY;
		return;
	}
	bounds[ 0 ] = bounds[ 1 ] = p[ 0 ];
	for( int i = 1; i < numPoints; i++ )
	{
		if( p[ i ].x < bounds[ 0 ].x )
		{
			bounds[ 0 ].x = p[ i ].x;
		}
		else if( p[ i ].x > bounds[ 1 ].x )
		{
			bounds[ 1 ].x = p[ i ].x;
		}
		if( p[ i ].y < bounds[ 0 ].y )
		{
			bounds[ 0 ].y = p[ i ].y;
		}
		else if( p[ i ].y > bounds[ 1 ].y )
		{
			bounds[ 1 ].y = p[ i ].y;
		}
	}
}

/*
=============
idWinding2D::IsTiny
=============
*/
#define	EDGE_LENGTH		0.2f

bool idWinding2D::IsTiny() const
{
	idVec2	delta;
	int edges = 0;
	for( int i = 0; i < numPoints; i++ )
	{
		delta = p[ ( i + 1 ) % numPoints ] - p[ i ];
		float len = delta.Length();
		if( len > EDGE_LENGTH )
		{
			if( ++edges == 3 )
			{
				return false;
			}
		}
	}
	return true;
}

/*
=============
idWinding2D::IsHuge
=============
*/
bool idWinding2D::IsHuge() const
{
	for( int i = 0; i < numPoints; i++ )
	{
		for( int j = 0; j < 2; j++ )
		{
			if( p[ i ][ j ] <= MIN_WORLD_COORD || p[ i ][ j ] >= MAX_WORLD_COORD )
			{
				return true;
			}
		}
	}
	return false;
}

/*
=============
idWinding2D::Print
=============
*/
void idWinding2D::Print() const
{
	for( int i = 0; i < numPoints; i++ )
	{
		idLib::common->Printf( "(%5.1f, %5.1f)\n", p[ i ][ 0 ], p[ i ][ 1 ] );
	}
}

/*
=============
idWinding2D::PlaneDistance
=============
*/
float idWinding2D::PlaneDistance( const idVec3& plane ) const
{
	float min = idMath::INFINITY;
	float max = -min;
	for( int i = 0; i < numPoints; i++ )
	{
		float d = plane.x * p[ i ].x + plane.y * p[ i ].y + plane.z;
		if( d < min )
		{
			min = d;
			if( IEEE_FLT_SIGNBITSET( min ) & IEEE_FLT_SIGNBITNOTSET( max ) )
			{
				return 0.0f;
			}
		}
		if( d > max )
		{
			max = d;
			if( IEEE_FLT_SIGNBITSET( min ) & IEEE_FLT_SIGNBITNOTSET( max ) )
			{
				return 0.0f;
			}
		}
	}
	if( IEEE_FLT_SIGNBITNOTSET( min ) )
	{
		return min;
	}
	if( IEEE_FLT_SIGNBITSET( max ) )
	{
		return max;
	}
	return 0.0f;
}

/*
=============
idWinding2D::PlaneSide
=============
*/
int idWinding2D::PlaneSide( const idVec3& plane, const float epsilon ) const
{
	bool front = false;
	bool back = false;
	for( int i = 0; i < numPoints; i++ )
	{
		float d = plane.x * p[ i ].x + plane.y * p[ i ].y + plane.z;
		if( d < -epsilon )
		{
			if( front )
			{
				return SIDE_CROSS;
			}
			back = true;
			continue;
		}
		else if( d > epsilon )
		{
			if( back )
			{
				return SIDE_CROSS;
			}
			front = true;
			continue;
		}
	}
	if( back )
	{
		return SIDE_BACK;
	}
	if( front )
	{
		return SIDE_FRONT;
	}
	return SIDE_ON;
}

/*
============
idWinding2D::PointInside
============
*/
bool idWinding2D::PointInside( const idVec2& point, const float epsilon ) const
{
	idVec3 plane;
	for( int i = 0; i < numPoints; i++ )
	{
		Plane2DFromPoints( plane, p[ i ], p[ ( i + 1 ) % numPoints ] );
		float d = plane.x * point.x + plane.y * point.y + plane.z;
		if( d > epsilon )
		{
			return false;
		}
	}
	return true;
}

/*
============
idWinding2D::LineIntersection
============
*/
bool idWinding2D::LineIntersection( const idVec2& start, const idVec2& end ) const
{
	int i, numEdges;
	int sides[ MAX_POINTS + 1 ], counts[ 3 ];
	float d1, d2, epsilon = 0.1f;
	idVec3 plane, edges[ 2 ];

	counts[ SIDE_FRONT ] = counts[ SIDE_BACK ] = counts[ SIDE_ON ] = 0;

	Plane2DFromPoints( plane, start, end );
	for( i = 0; i < numPoints; i++ )
	{
		d1 = plane.x * p[ i ].x + plane.y * p[ i ].y + plane.z;
		if( d1 > epsilon )
		{
			sides[ i ] = SIDE_FRONT;
		}
		else if( d1 < -epsilon )
		{
			sides[ i ] = SIDE_BACK;
		}
		else
		{
			sides[ i ] = SIDE_ON;
		}
		counts[ sides[ i ] ]++;
	}
	sides[ i ] = sides[ 0 ];

	if( !counts[ SIDE_FRONT ] )
	{
		return false;
	}
	if( !counts[ SIDE_BACK ] )
	{
		return false;
	}

	numEdges = 0;
	for( i = 0; i < numPoints; i++ )
	{
		if( sides[ i ] != sides[ i + 1 ] && sides[ i + 1 ] != SIDE_ON )
		{
			Plane2DFromPoints( edges[ numEdges++ ], p[ i ], p[ ( i + 1 ) % numPoints ] );
			if( numEdges >= 2 )
			{
				break;
			}
		}
	}
	if( numEdges < 2 )
	{
		return false;
	}

	d1 = edges[ 0 ].x * start.x + edges[ 0 ].y * start.y + edges[ 0 ].z;
	d2 = edges[ 0 ].x * end.x + edges[ 0 ].y * end.y + edges[ 0 ].z;
	if( IEEE_FLT_SIGNBITNOTSET( d1 ) & IEEE_FLT_SIGNBITNOTSET( d2 ) )
	{
		return false;
	}
	d1 = edges[ 1 ].x * start.x + edges[ 1 ].y * start.y + edges[ 1 ].z;
	d2 = edges[ 1 ].x * end.x + edges[ 1 ].y * end.y + edges[ 1 ].z;
	if( IEEE_FLT_SIGNBITNOTSET( d1 ) & IEEE_FLT_SIGNBITNOTSET( d2 ) )
	{
		return false;
	}
	return true;
}

/*
============
idWinding2D::RayIntersection
============
*/
bool idWinding2D::RayIntersection( const idVec2& start, const idVec2& dir, float& scale1, float& scale2, int* edgeNums ) const
{
	int i, numEdges, localEdgeNums[ 2 ];
	int sides[ MAX_POINTS + 1 ], counts[ 3 ];
	float d1, d2, epsilon = 0.1f;
	idVec3 plane, edges[ 2 ];

	scale1 = scale2 = 0.0f;
	counts[ SIDE_FRONT ] = counts[ SIDE_BACK ] = counts[ SIDE_ON ] = 0;

	Plane2DFromVecs( plane, start, dir );
	for( i = 0; i < numPoints; i++ )
	{
		d1 = plane.x * p[ i ].x + plane.y * p[ i ].y + plane.z;
		if( d1 > epsilon )
		{
			sides[ i ] = SIDE_FRONT;
		}
		else if( d1 < -epsilon )
		{
			sides[ i ] = SIDE_BACK;
		}
		else {
			sides[ i ] = SIDE_ON;
		}
		counts[ sides[ i ] ]++;
	}
	sides[ i ] = sides[ 0 ];

	if( !counts[ SIDE_FRONT ] )
	{
		return false;
	}
	if( !counts[ SIDE_BACK ] )
	{
		return false;
	}

	numEdges = 0;
	for( i = 0; i < numPoints; i++ )
	{
		if( sides[ i ] != sides[ i + 1 ] && sides[ i + 1 ] != SIDE_ON )
		{
			localEdgeNums[ numEdges ] = i;
			Plane2DFromPoints( edges[ numEdges++ ], p[ i ], p[ ( i + 1 ) % numPoints ] );
			if( numEdges >= 2 )
			{
				break;
			}
		}
	}
	if( numEdges < 2 )
	{
		return false;
	}

	d1 = edges[ 0 ].x * start.x + edges[ 0 ].y * start.y + edges[ 0 ].z;
	d2 = -( edges[ 0 ].x * dir.x + edges[ 0 ].y * dir.y );
	if( d2 == 0.0f )
	{
		return false;
	}
	scale1 = d1 / d2;
	d1 = edges[ 1 ].x * start.x + edges[ 1 ].y * start.y + edges[ 1 ].z;
	d2 = -( edges[ 1 ].x * dir.x + edges[ 1 ].y * dir.y );
	if( d2 == 0.0f )
	{
		return false;
	}
	scale2 = d1 / d2;

	if( idMath::Fabs( scale1 ) > idMath::Fabs( scale2 ) )
	{
		SwapValues( scale1, scale2 );
		SwapValues( localEdgeNums[ 0 ], localEdgeNums[ 1 ] );
	}

	if( edgeNums )
	{
		edgeNums[ 0 ] = localEdgeNums[ 0 ];
		edgeNums[ 1 ] = localEdgeNums[ 1 ];
	}
	return true;
}

/*
============
idWinding2D::Rotation
============
*/
void idWinding2D::Rotation( const idVec2& org, float angle )
{
	idMat2 mat;
	mat.Rotation( angle );

	for( int i = 0; i < numPoints; i++ )
	{
		p[ i ] -= org;
		p[ i ] *= mat;
		p[ i ] += org;
	}
}

/*
============
idWinding2D::Scale
============
*/
void idWinding2D::Scale( const idVec2& scale )
{
	for( int i = 0; i < numPoints; i++ )
	{
		p[ i ].x *= scale.x;
		p[ i ].y *= scale.y;
	}
}

/*
============
idWinding2D::SplitEdgesByLine
============
*/
bool idWinding2D::SplitEdgesByLine( const idVec2& start, const idVec2& end, const float epsilon )
{
	float frac1 = 0.0f;
	float frac2 = 0.0f;
	float denom = 0.0f;
	bool clipped = false;

	idWinding2D tempWinding;

	const idVec2& p1 = start;
	const idVec2& p2 = end;

	const idVec2 dirLine = end - start;

	for( int i = 0; i < numPoints; i++ )
	{
		const idVec2& p3 = p[ i ];
		const idVec2& p4 = p[ ( i + 1 ) % numPoints ];

		const idVec2 dirEdge = p4 - p3;

		//const idVec2& st1 = st[ i ];
		//const idVec2& st2 = st[ ( i + 1 ) % numPoints ];

		if( tempWinding.GetNumPoints() >= idWinding2D::MAX_POINTS )
		{
			continue;
		}

		tempWinding.AddPoint( p3/*, st1*/ );

		if( tempWinding.GetNumPoints() >= idWinding2D::MAX_POINTS )
		{
			continue;
		}

		denom = ( ( p4.y - p3.y ) * ( p2.x - p1.x ) ) - ( ( p4.x - p3.x ) * ( p2.y - p1.y ) );
		if( idMath::Fabs( denom ) < VECTOR_EPSILON )
		{
			continue;	// parallel
		}

		frac1 = ( ( ( p4.x - p3.x ) * ( p1.y - p3.y ) ) - ( ( p4.y - p3.y ) * ( p1.x - p3.x ) ) ) / denom;
		frac2 = ( ( ( p2.x - p1.x ) * ( p1.y - p3.y ) ) - ( ( p2.y - p1.y ) * ( p1.x - p3.x ) ) ) / denom;

		if( ( frac1 < 0.0f || frac1 > 1.0f ) || ( frac2 < 0.0f || frac2 > 1.0f ) )
		{
			continue;	// co-linear
		}

		idVec2 newPoint = p3 + ( ( p4 - p3 ) * frac2 );
		//idVec2 newST = st1 + ( ( st2 - st1 ) * frac2 );

		tempWinding.AddPoint( newPoint/*, newST*/ );
		clipped = true;
	}

	*this = tempWinding;
	return clipped;
}

/*
============
ClipWindingClassify
============
*/
ID_INLINE bool ClipWindingClassify( float val, float min, float max, bool sign, int index, const idVec2& v1, const idVec2& v2, /*const idVec2& st1, const idVec2& st2,*/ idWinding2D& winding )
{
	if( winding.GetNumPoints() >= idWinding2D::MAX_POINTS )
	{
		return false;
	}

	bool inside[ 2 ];

	inside[ 0 ] = sign ? v1[ index ] < val : v1[ index ] > val;
	inside[ 1 ] = sign ? v2[ index ] < val : v2[ index ] > val;

	if( inside[ 0 ] != inside[ 1 ] )
	{
		idVec2 intersectionXY;
		//idVec2 intersectionST;
		float frac = ( val - v1[ index ] ) / ( v1[ index ] - v2[ index ] );

		intersectionXY = ( ( v1 - v2 ) * frac ) + v1;
		//intersectionST = ( ( st1 - st2 ) * frac ) + st1;

		if( intersectionXY[ 1 - index ] < min )
		{
			intersectionXY[ 1 - index ] = min;
		}
		else if( intersectionXY[ 1 - index ] > max )
		{
			intersectionXY[ 1 - index ] = max;
		}

		winding.AddPoint( intersectionXY/*, intersectionST*/ );
	}

	return inside[ 0 ] != inside[ 1 ];
}
/*
============
idWinding2D::ClipByBounds
============
*/
bool idWinding2D::ClipByBounds( const idBounds2D& bounds, const float epsilon )
{
	idBounds2D boundsLocal;
	GetBounds( boundsLocal );
	if( !boundsLocal.IntersectsBounds( bounds ) )
	{
		Clear();
		return true;
	}

	if( bounds.ContainsBounds( boundsLocal ) )
	{
		return false;
	}

	static idVec2 verts[ MAX_POINTS ];
	//static idVec2 st[ MAX_POINTS ];

	for( int i = 0; i < numPoints; i++ )
	{
		verts[ i ] = p[ i ];
		//st[ i ] = this->st[ i ];
	}

	int count = numPoints;
	Clear();
	for( int i = 0; i < count; i++ )
	{
		int j = ( i + 1 ) % count;

		bool inside0 = verts[ i ].x > bounds.GetMins().x;
		bool inside1 = verts[ j ].x > bounds.GetMins().x;
		bool inside2 = verts[ i ].x < bounds.GetMins().x + bounds.GetWidth();
		bool inside3 = verts[ j ].x < bounds.GetMins().x + bounds.GetWidth();
		bool inside4 = verts[ i ].y > bounds.GetMins().y;
		bool inside5 = verts[ j ].y > bounds.GetMins().y;
		bool inside6 = verts[ i ].y < bounds.GetMins().y + bounds.GetHeight();
		bool inside7 = verts[ j ].y < bounds.GetMins().y + bounds.GetHeight();

		// add current point if inside
		if( inside0 && inside2 && inside4 && inside6 )
		{
			if( numPoints < MAX_POINTS )
			{
				AddPoint( verts[ i ]/*, st[ i ]*/ );
			}
		}

		// if line is intersected, add next point
		if( inside0 != inside1 )
		{
			ClipWindingClassify( bounds.GetMins().x, bounds.GetMins().y, bounds.GetMins().y + bounds.GetHeight(), false, 0, verts[ i ], verts[ j ],/* st[ i ], st[ j ],*/ *this );
		}
		else if( inside2 != inside3 )
		{
			ClipWindingClassify( bounds.GetMins().x + bounds.GetWidth(), bounds.GetMins().y, bounds.GetMins().y + bounds.GetHeight(), true, 0, verts[ i ], verts[ j ],/* st[ i ], st[ j ],*/ *this );
		}
		else if( inside4 != inside5 )
		{
			ClipWindingClassify( bounds.GetMins().y, bounds.GetMins().x, bounds.GetMins().x + bounds.GetWidth(), false, 1, verts[ i ], verts[ j ],/* st[ i ], st[ j ],*/ *this );
		}
		else if( inside6 != inside7 )
		{
			ClipWindingClassify( bounds.GetMins().y + bounds.GetHeight(), bounds.GetMins().x, bounds.GetMins().x + bounds.GetWidth(), true, 1, verts[ i ], verts[ j ],/* st[ i ], st[ j ],*/ *this );
		}
	}

	return numPoints == 0;
}