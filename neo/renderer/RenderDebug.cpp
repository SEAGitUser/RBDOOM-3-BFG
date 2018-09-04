

#pragma hdrstop
#include "precompiled.h"

#include "tr_local.h"

#include "RenderDebug.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

struct debugLine_t {
	idVec3		start;
	idVec3		end;
	uint32		lifeTime;
	uint32		color;
	bool		depthTest;
};

struct debugBounds_t {
	idBounds	bounds;
	idVec3		origin;
	idMat3		axis;
	uint32		lifeTime;
	uint32		color;
	bool		depthTest;
};

struct debugText_t {
	idStr		text;
	idVec3		origin;
	float		scale;
	idMat3		viewAxis;
	int			align;
	float		lineWidth;
	uint32		lifeTime;
	uint32		color;
	bool		orientToRenderViewAxis;
	bool		depthTest;
};

struct debugPolygon_t
{
	idWinding	winding;
	uint32		lifeTime;
	uint32		color;
	bool		depthTest;
};

class idCylinder {
public:
	void			Set();
	void			SetOrigin( const idVec3 &_origin ) { origin = _origin; }
	void			ShrinkToHoldBounds( const idBounds & );
	const idVec3 &	GetOrigin() const { return origin; }
	float			GetRadius() const { return radius; }
	float			GetHalfHeight() const { return halfHeight; }
	float			GetHeight() const { return halfHeight * 2.0f; }
	bool			IsValid() const;
	bool			ContainsPoint( const idVec3 & ) const;
	bool			ContainsCylinder( const idCylinder & ) const;
	bool			IntersectsCylinder( const idCylinder & ) const;
	bool			LineIntersection() const;
	bool			RayIntersection( const idVec3 & rayStart, const idVec3 & rayDir, float scale1, float scale2 ) const;

	idVec3			origin;
	float			halfHeight;
	float			radius;
};


class idRenderDebugLocal : public idRenderDebug
{
	idList<debugLine_t, TAG_RENDER_DEBUG> debugLines;
	uint32 debugLineTime;

	idList<debugBounds_t, TAG_RENDER_DEBUG>	debugBounds;
	uint32 debugBoundsTime;

	idList<debugText_t, TAG_RENDER_DEBUG> debugText;
	uint32 debugTextTime;

	idList<debugPolygon_t, TAG_RENDER_DEBUG> debugPolygons;
	uint32 debugPolygonTime;

	void AddDebugBounds( const idColor & color, const idBounds & bounds, const idVec3& origin, const idMat3& axis, const uint32 lifetime, const bool depthTest );

public:
	virtual void DebugClear( uint32 deltaTime );

	virtual void DebugLine( const idColor & color, const idVec3 & start, const idVec3 & end, uint32 lifetime, bool depthTest );
	virtual void DebugArrow( const idColor & color, const idVec3 & start, const idVec3 & end, int size, uint32 lifetime, bool depthTest );
	//virtual void DebugArrow2( const idColor & color, const idVec3 & start, const idVec3 & end, int size, uint32 lifetime, int camPos, int w );
	//virtual void DebugArrow3( const idColor & color, const idVec3 & start, const idVec3 & end, int size, uint32 lifetime, bool depthTest, int right, int forward );
	//virtual void DebugPyramid( const idColor & color, const idVec3 & base, const idVec3 & up, const idVec3 & forward, int height, int width, uint32 lifetime, bool depthTest, const idVec3 & corner3, const idVec3 & corner2, const idVec3 & corner4, const idVec3 & peak, const idVec3 & corner1 );
	virtual void DebugWinding( const idColor & color, const idWinding &, const idVec3 & origin, const idMat3 & axis, uint32 lifetime, bool depthTest );
	virtual void DebugCircle( const idColor & color, const idVec3 & origin, const idVec3 & dir, float radius, int numSteps, uint32 lifetime, bool depthTest );
	//virtual void DebugCylinder( const idColor & color, const idCylinder & cyl, const idVec3 & dir, int numSteps, uint32 lifetime, bool depthTest, const idVec3 & left, const idVec3 & up );
	//virtual void DebugArc( const idColor & color, const idVec3 & origin, const idVec3 & dir, int radius, int startAngle, int endAngle, int anglesPerSegment, uint32 lifetime, bool depthTest, int drawSectorEdges, int left, int lastPoint, int up, int point );
	//virtual void DebugShadedArc( const idColor & color, const idVec3 & origin, const idVec3 & dir, const idVec3 & up, int radius, int startAngle, int endAngle, int anglesPerSegment, uint32 lifetime, bool depthTest, int fwd, int winding, int drawReversed );
	//virtual void DebugSphere( const idColor & color, const idSphere & sphere, int sectors, uint32 lifetime, bool depthTest, int lastp, int lastArray );
	virtual void DebugSphere( const idColor & color, const idSphere& sphere, uint32 lifetime, bool depthTest );
	virtual void DebugBounds( const idColor & color, const idBounds & bounds, const idVec3 & org, uint32 lifetime, bool depthTest );
	//virtual void DebugOrientedBounds( const idColor & color, const idBounds & bounds, const idVec3 & org, const idMat3 & axis, uint32 lifetime, bool depthTest );
	virtual void DebugBox( const idColor & color, const idBox & box, uint32 lifetime, bool depthTest );
	virtual void DebugCone( const idColor & color, const idVec3 & apex, const idVec3 & dir, float radius1, float radius2, uint32 lifetime, bool depthTest );
	virtual void DebugAxis( const idVec3 & origin, const idMat3 & axis, uint32 lifeTime, bool depthTest );
	//virtual void DebugAxisScaled( const idVec3 & origin, const idMat3 & axis, int scale, int lifeTime, bool depthTest );
	//virtual void DebugPoint( const idColor & color, const idVec3 & origin, int lifeTime, bool depthTest );
	virtual void DebugPolygon( const idColor & color, const idWinding & winding, uint32 lifetime, bool depthTest );
	//virtual void DebugFilledBounds( const idColor & color, const idBounds & bounds, const idVec3 & org, uint32 lifetime, bool depthTest );
	virtual void DebugText( const char *text, const idVec3 & origin, float scale, const idColor & color, const idMat3 & viewAxis, int align, uint32 lifetime, bool depthTest, bool bold = false );
	//virtual void DebugSpline( const idColor & color, const idCurve_Spline<idVec3> & spline, int step, int drawKnots, int lifeTime, bool depthTest );

	virtual void DebugScreenRect( const idColor & color, const idScreenRect&, const idRenderView*, uint32 lifetime = 0, bool depthTest = false );

}; // committedDebugData;

/*
==============================================
 DebugClear
==============================================
*/
void idRenderDebugLocal::DebugClear( uint32 deltaTime )
{
	if( !deltaTime )
	{
		debugLines.Clear();
		debugBounds.Clear();
		debugText.Clear();
		debugPolygons.Clear();

		debugLineTime = deltaTime;
		debugPolygonTime = deltaTime;
		debugTextTime = deltaTime;
		debugBoundsTime = deltaTime;

		return;
	}
	///////////////////////////////////////////////////
	{
		debugLineTime = deltaTime;

		// copy any lines that still need to be drawn
		int num = 0;
		auto line = debugLines.Ptr();
		for( int i = 0; i < debugLines.Num(); i++, line++ )
		{
			if( line->lifeTime > deltaTime )
			{
				if( num != i )
				{
					debugLines[ num ] = *line;
				}
				num++;
			}
		}
		//rb_numDebugLines = num;
		debugLines.SetNum( num ); //?
	}
	///////////////////////////////////////////////////
	{
		debugPolygonTime = deltaTime;

		// copy any polygons that still need to be drawn
		int num = 0;
		auto poly = debugPolygons.Ptr();
		for( int i = 0; i < debugPolygons.Num(); i++, poly++ )
		{
			if( poly->lifeTime > deltaTime )
			{
				if( num != i )
				{
					debugPolygons[ num ] = *poly;
				}
				num++;
			}
		}
		//rb_numDebugPolygons = num;
		debugPolygons.SetNum( num ); //?
	}
	///////////////////////////////////////////////////
	{
		debugTextTime = deltaTime;

		// copy any text that still needs to be drawn
		int num = 0;
		auto text = debugText.Ptr();
		for( int i = 0; i < debugText.Num(); i++, text++ )
		{
			if( text->lifeTime > deltaTime )
			{
				if( num != i )
				{
					debugText[ num ] = *text;
				}
				num++;
			}
		}
		//rb_numDebugText = num;
		debugText.SetNum( num ); //?
	}
	///////////////////////////////////////////////////
	{
		debugBoundsTime = deltaTime;

		int num = 0;
		auto text = debugBounds.Ptr();
		for( int i = 0; i < debugBounds.Num(); i++, text++ )
		{
			if( text->lifeTime > deltaTime )
			{
				if( num != i )
				{
					debugBounds[ num ] = *text;
				}
				num++;
			}
		}
		//rb_numDebugText = num;
		debugBounds.SetNum( num ); //?
	}
}

/*
==============================================
DebugClear
==============================================
*/
void idRenderDebugLocal::AddDebugBounds( const idColor& color, const idBounds & bounds, const idVec3& origin, const idMat3& axis, const uint32 lifetime, const bool depthTest )
{
	auto const dBounds = &debugBounds.Alloc();

	dBounds->bounds = bounds;
	dBounds->origin = origin;
	dBounds->axis = axis;
	dBounds->lifeTime = debugBoundsTime + lifetime;
	dBounds->depthTest = depthTest;
	dBounds->color = idColor::PackColor( color.ToVec4() );
}

/*
==============================================
 DebugLine
==============================================
*/
void idRenderDebugLocal::DebugLine( const idColor & color, const idVec3 & start, const idVec3 & end, uint32 lifetime, bool depthTest )
{
	auto dLine = &debugLines.Alloc();

	dLine->start = start;
	dLine->end = end;
	dLine->depthTest = depthTest;
	dLine->lifeTime = debugLineTime + lifetime;
	dLine->color = idColor::PackColor( color.ToVec4() );
}

/*
==============================================
 DebugArrow
==============================================
*/
void idRenderDebugLocal::DebugArrow( const idColor & color, const idVec3 & start, const idVec3 & end, int size, uint32 lifetime, bool depthTest )
{
	idVec3 forward, right, up, v1, v2;
	float a, s;
	int i;
	static float arrowCos[ 40 ];
	static float arrowSin[ 40 ];
	static int arrowStep;

	DebugLine( color, start, end, lifetime, depthTest );

	if( r_debugArrowStep.GetInteger() <= 10 ) {
		return;
	}

	// calculate sine and cosine when step size changes
	if( arrowStep != r_debugArrowStep.GetInteger() )
	{
		arrowStep = r_debugArrowStep.GetInteger();
		for( i = 0, a = 0; a < 360.0f; a += arrowStep, ++i )
		{
			idMath::SinCos16( DEG2RAD( a ), arrowSin[ i ], arrowCos[ i ] );
		}
		arrowCos[ i ] = arrowCos[ 0 ];
		arrowSin[ i ] = arrowSin[ 0 ];
	}

	// draw a nice arrow
	forward = end - start;
	forward.Normalize();
	forward.NormalVectors( right, up );
	for( i = 0, a = 0; a < 360.0f; a += arrowStep, ++i )
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

		DebugLine( color, v1, end, lifetime, depthTest );
		DebugLine( color, v1, v2, lifetime, depthTest );
	}
}

/*
==============================================
 DebugWinding
==============================================
*/
void idRenderDebugLocal::DebugWinding( const idColor& color, const idWinding& winding, const idVec3& origin, const idMat3& axis, uint32 lifetime, bool depthTest )
{
	if( winding.GetNumPoints() < 2 ) {
		return;
	}

	idVec3 lastPoint = origin + winding[ winding.GetNumPoints() - 1 ].ToVec3() * axis;
	for( int i = 0; i < winding.GetNumPoints(); ++i )
	{
		idVec3 point = origin + winding[ i ].ToVec3() * axis;
		DebugLine( color, lastPoint, point, lifetime, depthTest );
		lastPoint = point;
	}
}

/*
==============================================
 DebugCircle
==============================================
*/
void idRenderDebugLocal::DebugCircle( const idColor& color, const idVec3& origin, const idVec3& dir, float radius, int numSteps, uint32 lifetime, bool depthTest )
{
	idVec3 left, up, point, lastPoint;
	float s, c;

	dir.OrthogonalBasis( left, up );
	left *= radius;
	up *= radius;
	lastPoint = origin + up;
	for( int i = 1; i <= numSteps; ++i )
	{
		idMath::SinCos16( idMath::TWO_PI * i / numSteps, s, c );
		point = origin + s * left + c * up;
		DebugLine( color, lastPoint, point, lifetime, depthTest );
		lastPoint = point;
	}
}

/*
==============================================
 DebugSphere
==============================================
*/
void idRenderDebugLocal::DebugSphere( const idColor& color, const idSphere& sphere, uint32 lifetime, bool depthTest )
{
	int i, j, n;
	float s, c, ss, cc;
	idVec3 p, lastp;

	const int num = 360 / 15;
	auto lastArray = ( idVec3* )_alloca16( num * sizeof( idVec3 ) );
	lastArray[ 0 ] = sphere.GetOrigin() + idVec3( 0, 0, sphere.GetRadius() );
	for( n = 1; n < num; n++ )
	{
		lastArray[ n ] = lastArray[ 0 ];
	}

	for( i = 15; i <= 360; i += 15 )
	{
		idMath::SinCos16( DEG2RAD( i ), s, c );
		lastp[ 0 ] = sphere.GetOrigin()[ 0 ];
		lastp[ 1 ] = sphere.GetOrigin()[ 1 ] + sphere.GetRadius() * s;
		lastp[ 2 ] = sphere.GetOrigin()[ 2 ] + sphere.GetRadius() * c;
		for( n = 0, j = 15; j <= 360; j += 15, n++ )
		{
			idMath::SinCos16( DEG2RAD( j ), ss, cc );
			p[ 0 ] = sphere.GetOrigin()[ 0 ] + ss * sphere.GetRadius() * s;
			p[ 1 ] = sphere.GetOrigin()[ 1 ] + cc * sphere.GetRadius() * s;
			p[ 2 ] = lastp[ 2 ];

			DebugLine( color, lastp, p, lifetime, depthTest );
			DebugLine( color, lastp, lastArray[ n ], lifetime, depthTest );

			lastArray[ n ] = lastp;
			lastp = p;
		}
	}
}

/*
==============================================
 DebugBounds
==============================================
*/
void idRenderDebugLocal::DebugBounds( const idColor& color, const idBounds& bounds, const idVec3& org, uint32 lifetime, bool depthTest )
{
	if( bounds.IsCleared() ) {
		return;
	}

	idVec3 v[ 8 ];
	for( int i = 0; i < 8; i++ )
	{
		v[ i ][ 0 ] = org[ 0 ] + bounds[ ( i ^ ( i >> 1 ) ) & 1 ][ 0 ];
		v[ i ][ 1 ] = org[ 1 ] + bounds[ ( i >> 1 ) & 1 ][ 1 ];
		v[ i ][ 2 ] = org[ 2 ] + bounds[ ( i >> 2 ) & 1 ][ 2 ];
	}

	for( int i = 0; i < 4; i++ )
	{
		DebugLine( color, v[ i ], v[ ( i + 1 ) & 3 ], lifetime, depthTest );
		DebugLine( color, v[ 4 + i ], v[ 4 + ( ( i + 1 ) & 3 ) ], lifetime, depthTest );
		DebugLine( color, v[ i ], v[ 4 + i ], lifetime, depthTest );
	}
}

/*
==============================================
 DebugBox
==============================================
*/
void idRenderDebugLocal::DebugBox( const idColor& color, const idBox& box, uint32 lifetime, bool depthTest )
{
	idVec3 v[ 8 ];
	box.ToPoints( v );
	for( int i = 0; i < 4; i++ )
	{
		DebugLine( color, v[ i ], v[ ( i + 1 ) & 3 ], lifetime, depthTest );
		DebugLine( color, v[ 4 + i ], v[ 4 + ( ( i + 1 ) & 3 ) ], lifetime, depthTest );
		DebugLine( color, v[ i ], v[ 4 + i ], lifetime, depthTest );
	}
}

/*
==============================================
 DebugCone
	dir is the cone axis
	radius1 is the radius at the apex
	radius2 is the radius at apex+dir
==============================================
*/
void idRenderDebugLocal::DebugCone( const idColor& color, const idVec3& apex, const idVec3& dir, float radius1, float radius2, uint32 lifetime, bool depthTest )
{
	idMat3 axis;
	axis[ 2 ] = dir;
	axis[ 2 ].Normalize();
	axis[ 2 ].NormalVectors( axis[ 0 ], axis[ 1 ] );
	axis[ 1 ] = -axis[ 1 ];

	idVec3 top = apex + dir;
	idVec3 lastp2 = top + radius2 * axis[ 1 ];

	if( radius1 == 0.0f )
	{
		for( int i = 20; i <= 360; i += 20 )
		{
			float s, c; idMath::SinCos16( DEG2RAD( i ), s, c );

			idVec3 d = s * axis[ 0 ] + c * axis[ 1 ];
			idVec3 p2 = top + d * radius2;
			DebugLine( color, lastp2, p2, lifetime, depthTest );
			DebugLine( color, p2, apex, lifetime, depthTest );
			lastp2 = p2;
		}
	}
	else {
		idVec3 lastp1 = apex + radius1 * axis[ 1 ];
		for( int i = 20; i <= 360; i += 20 )
		{
			float s, c; idMath::SinCos16( DEG2RAD( i ), s, c );

			idVec3 d = s * axis[ 0 ] + c * axis[ 1 ];
			idVec3 p1 = apex + d * radius1;
			idVec3 p2 = top + d * radius2;
			DebugLine( color, lastp1, p1, lifetime, depthTest );
			DebugLine( color, lastp2, p2, lifetime, depthTest );
			DebugLine( color, p1, p2, lifetime, depthTest );
			lastp1 = p1;
			lastp2 = p2;
		}
	}
}

/*
==============================================
 DebugAxis
==============================================
*/
void idRenderDebugLocal::DebugAxis( const idVec3& origin, const idMat3& axis, uint32 lifeTime, bool depthTest )
{
	idVec3 start = origin;
	idVec3 end = start + axis[ 0 ] * 20.0f;
	DebugArrow( idColor::white, start, end, 2, lifeTime, depthTest );
	end = start + axis[ 0 ] * -20.0f;
	DebugArrow( idColor::white, start, end, 2, lifeTime, depthTest );
	end = start + axis[ 1 ] * +20.0f;
	DebugArrow( idColor::green, start, end, 2, lifeTime, depthTest );
	end = start + axis[ 1 ] * -20.0f;
	DebugArrow( idColor::green, start, end, 2, lifeTime, depthTest );
	end = start + axis[ 2 ] * +20.0f;
	DebugArrow( idColor::blue, start, end, 2, lifeTime, depthTest );
	end = start + axis[ 2 ] * -20.0f;
	DebugArrow( idColor::blue, start, end, 2, lifeTime, depthTest );
}

/*
==============================================
 DebugPolygon
==============================================
*/
void idRenderDebugLocal::DebugPolygon( const idColor& color, const idWinding& winding, uint32 lifeTime, bool depthTest )
{
	auto dPoly = &debugPolygons.Alloc();

	dPoly->color = idColor::PackColor( color.ToVec4() );
	dPoly->winding = winding;
	dPoly->depthTest = depthTest;
	dPoly->lifeTime = debugPolygonTime + lifeTime;
}

/*
==============================================
DebugClear
==============================================
*/
void idRenderDebugLocal::DebugText( const char *text, const idVec3 & origin, float scale, const idColor & color, const idMat3 & viewAxis, int align, uint32 lifetime, bool depthTest, bool bold )
{
	auto dText = &debugText.Alloc();

	dText->text = text;
	dText->origin = origin;
	dText->scale = scale;
	dText->viewAxis = viewAxis;
	dText->align = align;
	dText->lineWidth;
	dText->lifeTime = debugTextTime + lifetime;
	dText->color = idColor::PackColor( color.ToVec4() );
	dText->orientToRenderViewAxis;
	dText->depthTest = depthTest;
}


/*
==============================================
DebugClear
==============================================
*/

/*
==============================================
DebugClear
==============================================
*/

/*
==============================================
DebugClear
==============================================
*/

/*
==============================================
DebugClear
==============================================
*/

/*
==============================================
DebugClear
==============================================
*/
void idRenderDebugLocal::DebugScreenRect( const idColor & color, const idScreenRect& rect, const idRenderView* viewDef, uint32 lifetime, bool depthTest )
{
	idBounds bounds;
	idVec3 p[ 4 ];

	idVec2 center = viewDef->GetViewport().GetCenter();

	float dScale = viewDef->GetZNear() + 1.0f; // r_znear.GetFloat() + 1.0f
	float hScale = dScale * idMath::Tan16( DEG2RAD( viewDef->GetFOVx() * 0.5f ) );
	float vScale = dScale * idMath::Tan16( DEG2RAD( viewDef->GetFOVy() * 0.5f ) );

	bounds[ 0 ][ 0 ] = bounds[ 1 ][ 0 ] = dScale;
	bounds[ 0 ][ 1 ] = -( rect.x1 - center.x ) / center.x * hScale;
	bounds[ 1 ][ 1 ] = -( rect.x2 - center.x ) / center.x * hScale;
	bounds[ 0 ][ 2 ] =  ( rect.y1 - center.y ) / center.y * vScale;
	bounds[ 1 ][ 2 ] =  ( rect.y2 - center.y ) / center.y * vScale;

	for( int i = 0; i < 4; i++ )
	{
		p[ i ].x = bounds[ 0 ][ 0 ];
		p[ i ].y = bounds[ ( i ^ ( i >> 1 ) ) & 1 ].y;
		p[ i ].z = bounds[ ( i >> 1 ) & 1 ].z;

		p[ i ] = viewDef->GetOrigin() + p[ i ] * viewDef->GetAxis();
	}
	for( int i = 0; i < 4; i++ )
	{
		DebugLine( color, p[ i ], p[ ( i + 1 ) & 3 ], lifetime, false );
	}
}