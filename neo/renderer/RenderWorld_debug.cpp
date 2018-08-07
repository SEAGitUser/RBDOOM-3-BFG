/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2014 Robert Beckebans

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
=================================================================================

	WORLD DEBUG INFO PROCESSING

=================================================================================
*/

/*
====================
idRenderWorldLocal::DebugClearLines
====================
*/
void idRenderWorldLocal::DebugClearLines( int time )
{
	RB_ClearDebugLines( time );
	RB_ClearDebugText( time );
}

/*
====================
idRenderWorldLocal::DebugLine
====================
*/
void idRenderWorldLocal::DebugLine( const idVec4& color, const idVec3& start, const idVec3& end, const int lifetime, const bool depthTest )
{
	RB_AddDebugLine( color, start, end, lifetime, depthTest );
}

/*
================
idRenderWorldLocal::DebugArrow
================
*/
void idRenderWorldLocal::DebugArrow( const idVec4& color, const idVec3& start, const idVec3& end, int size, const int lifetime )
{
	idVec3 forward, right, up, v1, v2;
	float a, s;
	int i;
	static float arrowCos[ 40 ];
	static float arrowSin[ 40 ];
	static int arrowStep;

	DebugLine( color, start, end, lifetime );

	if( r_debugArrowStep.GetInteger() <= 10 )
	{
		return;
	}
	// calculate sine and cosine when step size changes
	if( arrowStep != r_debugArrowStep.GetInteger() )
	{
		arrowStep = r_debugArrowStep.GetInteger();
		for( i = 0, a = 0; a < 360.0f; a += arrowStep, i++ )
		{
			arrowCos[ i ] = idMath::Cos16( DEG2RAD( a ) );
			arrowSin[ i ] = idMath::Sin16( DEG2RAD( a ) );
		}
		arrowCos[ i ] = arrowCos[ 0 ];
		arrowSin[ i ] = arrowSin[ 0 ];
	}
	// draw a nice arrow
	forward = end - start;
	forward.Normalize();
	forward.NormalVectors( right, up );
	for( i = 0, a = 0; a < 360.0f; a += arrowStep, i++ )
	{
		s = 0.5f * size * arrowCos[ i ];
		v1 = end - size * forward;
		v1 = v1 + s * right;
		s = 0.5f * size * arrowSin[ i ];
		v1 = v1 + s * up;

		s = 0.5f * size * arrowCos[ i + 1 ];
		v2 = end - size * forward;
		v2 = v2 + s * right;
		s = 0.5f * size * arrowSin[ i + 1 ];
		v2 = v2 + s * up;

		DebugLine( color, v1, end, lifetime );
		DebugLine( color, v1, v2, lifetime );
	}
}

/*
====================
idRenderWorldLocal::DebugWinding
====================
*/
void idRenderWorldLocal::DebugWinding( const idVec4& color, const idWinding& w, const idVec3& origin, const idMat3& axis, const int lifetime, const bool depthTest )
{
	int i;
	idVec3 point, lastPoint;

	if( w.GetNumPoints() < 2 )
	{
		return;
	}

	lastPoint = origin + w[ w.GetNumPoints() - 1 ].ToVec3() * axis;
	for( i = 0; i < w.GetNumPoints(); i++ )
	{
		point = origin + w[ i ].ToVec3() * axis;
		DebugLine( color, lastPoint, point, lifetime, depthTest );
		lastPoint = point;
	}
}

/*
====================
idRenderWorldLocal::DebugCircle
====================
*/
void idRenderWorldLocal::DebugCircle( const idVec4& color, const idVec3& origin, const idVec3& dir, const float radius, const int numSteps, const int lifetime, const bool depthTest )
{
	int i;
	float a;
	idVec3 left, up, point, lastPoint;

	dir.OrthogonalBasis( left, up );
	left *= radius;
	up *= radius;
	lastPoint = origin + up;
	for( i = 1; i <= numSteps; i++ )
	{
		a = idMath::TWO_PI * i / numSteps;
		point = origin + idMath::Sin16( a ) * left + idMath::Cos16( a ) * up;
		DebugLine( color, lastPoint, point, lifetime, depthTest );
		lastPoint = point;
	}
}

/*
============
idRenderWorldLocal::DebugSphere
============
*/
void idRenderWorldLocal::DebugSphere( const idVec4& color, const idSphere& sphere, const int lifetime, const bool depthTest /*_D3XP*/ )
{
	int i, j, n, num;
	float s, c;
	idVec3 p, lastp, *lastArray;

	num = 360 / 15;
	lastArray = ( idVec3* )_alloca16( num * sizeof( idVec3 ) );
	lastArray[ 0 ] = sphere.GetOrigin() + idVec3( 0, 0, sphere.GetRadius() );
	for( n = 1; n < num; n++ )
	{
		lastArray[ n ] = lastArray[ 0 ];
	}

	for( i = 15; i <= 360; i += 15 )
	{
		s = idMath::Sin16( DEG2RAD( i ) );
		c = idMath::Cos16( DEG2RAD( i ) );
		lastp[ 0 ] = sphere.GetOrigin()[ 0 ];
		lastp[ 1 ] = sphere.GetOrigin()[ 1 ] + sphere.GetRadius() * s;
		lastp[ 2 ] = sphere.GetOrigin()[ 2 ] + sphere.GetRadius() * c;
		for( n = 0, j = 15; j <= 360; j += 15, n++ )
		{
			p[ 0 ] = sphere.GetOrigin()[ 0 ] + idMath::Sin16( DEG2RAD( j ) ) * sphere.GetRadius() * s;
			p[ 1 ] = sphere.GetOrigin()[ 1 ] + idMath::Cos16( DEG2RAD( j ) ) * sphere.GetRadius() * s;
			p[ 2 ] = lastp[ 2 ];

			DebugLine( color, lastp, p, lifetime, depthTest );
			DebugLine( color, lastp, lastArray[ n ], lifetime, depthTest );

			lastArray[ n ] = lastp;
			lastp = p;
		}
	}
}

/*
====================
idRenderWorldLocal::DebugBounds
====================
*/
void idRenderWorldLocal::DebugBounds( const idVec4& color, const idBounds& bounds, const idVec3& org, const int lifetime )
{
	int i;
	idVec3 v[ 8 ];

	if( bounds.IsCleared() )
	{
		return;
	}

	for( i = 0; i < 8; i++ )
	{
		v[ i ][ 0 ] = org[ 0 ] + bounds[ ( i ^ ( i >> 1 ) ) & 1 ][ 0 ];
		v[ i ][ 1 ] = org[ 1 ] + bounds[ ( i >> 1 ) & 1 ][ 1 ];
		v[ i ][ 2 ] = org[ 2 ] + bounds[ ( i >> 2 ) & 1 ][ 2 ];
	}
	for( i = 0; i < 4; i++ )
	{
		DebugLine( color, v[ i ], v[ ( i + 1 ) & 3 ], lifetime );
		DebugLine( color, v[ 4 + i ], v[ 4 + ( ( i + 1 ) & 3 ) ], lifetime );
		DebugLine( color, v[ i ], v[ 4 + i ], lifetime );
	}
}

/*
====================
idRenderWorldLocal::DebugBox
====================
*/
void idRenderWorldLocal::DebugBox( const idVec4& color, const idBox& box, const int lifetime )
{
	int i;
	idVec3 v[ 8 ];

	box.ToPoints( v );
	for( i = 0; i < 4; i++ )
	{
		DebugLine( color, v[ i ], v[ ( i + 1 ) & 3 ], lifetime );
		DebugLine( color, v[ 4 + i ], v[ 4 + ( ( i + 1 ) & 3 ) ], lifetime );
		DebugLine( color, v[ i ], v[ 4 + i ], lifetime );
	}
}

/*
============
idRenderWorldLocal::DebugCone

dir is the cone axis
radius1 is the radius at the apex
radius2 is the radius at apex+dir
============
*/
void idRenderWorldLocal::DebugCone( const idVec4& color, const idVec3& apex, const idVec3& dir, float radius1, float radius2, const int lifetime )
{
	int i;
	idMat3 axis;
	idVec3 top, p1, p2, lastp1, lastp2, d;

	axis[ 2 ] = dir;
	axis[ 2 ].Normalize();
	axis[ 2 ].NormalVectors( axis[ 0 ], axis[ 1 ] );
	axis[ 1 ] = -axis[ 1 ];

	top = apex + dir;
	lastp2 = top + radius2 * axis[ 1 ];

	if( radius1 == 0.0f )
	{
		for( i = 20; i <= 360; i += 20 )
		{
			d = idMath::Sin16( DEG2RAD( i ) ) * axis[ 0 ] + idMath::Cos16( DEG2RAD( i ) ) * axis[ 1 ];
			p2 = top + d * radius2;
			DebugLine( color, lastp2, p2, lifetime );
			DebugLine( color, p2, apex, lifetime );
			lastp2 = p2;
		}
	}
	else
	{
		lastp1 = apex + radius1 * axis[ 1 ];
		for( i = 20; i <= 360; i += 20 )
		{
			d = idMath::Sin16( DEG2RAD( i ) ) * axis[ 0 ] + idMath::Cos16( DEG2RAD( i ) ) * axis[ 1 ];
			p1 = apex + d * radius1;
			p2 = top + d * radius2;
			DebugLine( color, lastp1, p1, lifetime );
			DebugLine( color, lastp2, p2, lifetime );
			DebugLine( color, p1, p2, lifetime );
			lastp1 = p1;
			lastp2 = p2;
		}
	}
}

/*
================
idRenderWorldLocal::DebugAxis
================
*/
void idRenderWorldLocal::DebugAxis( const idVec3& origin, const idMat3& axis )
{
	idVec3 start = origin;
	idVec3 end = start + axis[ 0 ] * 20.0f;
	DebugArrow( idColor::white.ToVec4(), start, end, 2 );
	end = start + axis[ 0 ] * -20.0f;
	DebugArrow( idColor::white.ToVec4(), start, end, 2 );
	end = start + axis[ 1 ] * +20.0f;
	DebugArrow( idColor::green.ToVec4(), start, end, 2 );
	end = start + axis[ 1 ] * -20.0f;
	DebugArrow( idColor::green.ToVec4(), start, end, 2 );
	end = start + axis[ 2 ] * +20.0f;
	DebugArrow( idColor::blue.ToVec4(), start, end, 2 );
	end = start + axis[ 2 ] * -20.0f;
	DebugArrow( idColor::blue.ToVec4(), start, end, 2 );
}

/*
====================
idRenderWorldLocal::DebugClearPolygons
====================
*/
void idRenderWorldLocal::DebugClearPolygons( int time )
{
	RB_ClearDebugPolygons( time );
}

/*
====================
idRenderWorldLocal::DebugPolygon
====================
*/
void idRenderWorldLocal::DebugPolygon( const idVec4& color, const idWinding& winding, const int lifeTime, const bool depthTest )
{
	RB_AddDebugPolygon( color, winding, lifeTime, depthTest );
}

/*
================
idRenderWorldLocal::DebugScreenRect
================
*/
void idRenderWorldLocal::DebugScreenRect( const idVec4& color, const idScreenRect& rect, const idRenderView* viewDef, const int lifetime )
{
	idBounds bounds;
	idVec3 p[ 4 ];

	float centerx = ( viewDef->GetViewport().x2 - viewDef->GetViewport().x1 ) * 0.5f;
	float centery = ( viewDef->GetViewport().y2 - viewDef->GetViewport().y1 ) * 0.5f;

	float dScale = r_znear.GetFloat() + 1.0f;
	float hScale = dScale * idMath::Tan16( DEG2RAD( viewDef->GetFOVx() * 0.5f ) );
	float vScale = dScale * idMath::Tan16( DEG2RAD( viewDef->GetFOVy() * 0.5f ) );

	bounds[ 0 ][ 0 ] = bounds[ 1 ][ 0 ] = dScale;
	bounds[ 0 ][ 1 ] = -( rect.x1 - centerx ) / centerx * hScale;
	bounds[ 1 ][ 1 ] = -( rect.x2 - centerx ) / centerx * hScale;
	bounds[ 0 ][ 2 ] =  ( rect.y1 - centery ) / centery * vScale;
	bounds[ 1 ][ 2 ] =  ( rect.y2 - centery ) / centery * vScale;

	for( int i = 0; i < 4; i++ )
	{
		p[ i ].x = bounds[ 0 ][ 0 ];
		p[ i ].y = bounds[ ( i ^ ( i >> 1 ) ) & 1 ].y;
		p[ i ].z = bounds[ ( i >> 1 ) & 1 ].z;

		p[ i ] = viewDef->GetOrigin() + p[ i ] * viewDef->GetAxis();
	}
	for( int i = 0; i < 4; i++ )
	{
		DebugLine( color, p[ i ], p[ ( i + 1 ) & 3 ], false );
	}
}

