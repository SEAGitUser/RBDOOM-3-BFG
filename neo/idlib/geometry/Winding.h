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

#ifndef __WINDING_H__
#define __WINDING_H__

/*
===============================================================================

	A winding is an arbitrary convex polygon defined by an array of points.

===============================================================================
*/

class idWinding {
public:
	idWinding();
	explicit idWinding( const int n );								// allocate for n points
	explicit idWinding( const idVec3* verts, const int n );			// winding from points
	explicit idWinding( const idVec3& normal, const float dist );	// base winding for plane
	explicit idWinding( const idPlane& plane );						// base winding for plane
	explicit idWinding( const idWinding& winding );
	virtual	~idWinding();

	idWinding & 	operator=( const idWinding& winding );
	const idVec5 & 	operator[]( const int index ) const;
	idVec5 & 		operator[]( const int index );

	// add a point to the end of the winding point array
	idWinding & 	operator+=( const idVec3& v );
	idWinding & 	operator+=( const idVec5& v );
	void			AddPoint( const idVec3& v );
	void			AddPoint( const idVec5& v );

	// number of points on winding
	int				GetNumPoints() const;
	void			SetNumPoints( int n );
	virtual void	Clear();

	void			Rotate( const idVec3& origin, const idMat3& axis );

	// huge winding for plane, the points go counter clockwise when facing the front of the plane
	void			BaseForPlane( const idVec3& normal, const float dist );
	void			BaseForPlane( const idPlane& plane );

	// splits the winding into a front and back winding, the winding itself stays unchanged,
	// returns a SIDE_?
	int				Split( const idPlane& plane, const float epsilon, idWinding** front, idWinding** back ) const;
	// identical to the above version, but avoids heap allocations
	int				Split( const idPlane &plane, const float epsilon, idWinding& front, idWinding& back ) const;
	// chops of the part of the winding at the back of the plane
	// if there is nothing at the front the number of points is set to zero, returns a SIDE_?
	int				SplitInPlace( const idPlane &plane, const float epsilon, idWinding **back );

	// returns the winding fragment at the front of the clipping plane,
	// if there is nothing at the front the winding itself is destroyed and NULL is returned
	idWinding * 	Clip( const idPlane& plane, const float epsilon = ON_EPSILON, const bool keepOn = false );
	// cuts off the part at the back side of the plane, returns true if some part was at the front
	// if there is nothing at the front the number of points is set to zero
	bool			ClipInPlace( const idPlane& plane, const float epsilon = ON_EPSILON, const bool keepOn = false );

	// splits the winding into a front and back winding, the winding itself stays unchanged
	// returns a SIDE_?
	int				SplitWithEdgeNums( const int *edgeNums, const idPlane &plane, const int edgeNum, const float epsilon,
									   idWinding **front, idWinding **back, int **frontEdgePlanes, int **backEdgePlanes ) const;
	// cuts off the part at the back side of the plane, returns true if some part was at the front
	// if there is nothing at the front the number of points is set to zero
	int				ClipInPlaceWithEdgeNums( idList<int> &edgeNums, const idPlane &plane, const int edgeNum,
											 const float epsilon = ON_EPSILON, const bool keepOn = false );
	// returns a copy of the winding
	idWinding * 	Copy() const;
	idWinding * 	Reverse() const;
	void			ReverseSelf();
	void			RemoveEqualPoints( const float epsilon = ON_EPSILON );
	void			RemoveColinearPoints( const idVec3& normal, const float epsilon = ON_EPSILON );
	void			RemovePoint( int point );
	void			InsertPoint( const idVec5& point, int spot );
	bool			InsertPointIfOnEdge( const idVec5& point, const idPlane& plane, const float epsilon = ON_EPSILON );
	bool			InsertPointIfOnEdge( const idVec3& point, const idPlane& plane, const float epsilon = ON_EPSILON );
	// add a winding to the convex hull
	void			AddToConvexHull( const idWinding* winding, const idVec3& normal, const float epsilon = ON_EPSILON );
	// add a point to the convex hull
	void			AddToConvexHull( const idVec3& point, const idVec3& normal, const float epsilon = ON_EPSILON );
	// tries to merge 'this' with the given winding, returns NULL if merge fails, both 'this' and 'w' stay intact
	// 'keep' tells if the contacting points should stay even if they create colinear edges
	idWinding * 	TryMerge( const idWinding& w, const idVec3& normal, int keep = false ) const;
	// The parameter indices should point to an array with at least GetNumPoints() * 3 integers.
	// If no triangle fan could be created from one of the corners an assumed center of the
	// winding is used to create a fan and the indices referencing this additional vertex will
	// equal GetNumPoints() and indices[0] will always be equal to GetNumPoints().
	// Returns the number of indices written out.
	int				CreateTriangles( int *indices, const float epsilon ) const;
	// check whether the winding is valid or not
	bool			Check( bool print = true ) const;

	float			GetArea() const;
	idVec3			GetCenter() const;
	idVec3			GetNormal() const;
	float			GetRadius( const idVec3 &center ) const;
	void			GetPlane( idVec3 &normal, float &dist ) const;
	void			GetPlane( idPlane &plane ) const;
	void			GetBounds( idBounds &bounds ) const;

	bool			IsTiny() const;
	bool			IsHuge() const;	// base winding for a plane is typically huge

	bool			IsTiny( float epsilon ) const;
	bool			IsHuge( float radius ) const;

	void			Print() const;

	float			PlaneDistance( const idPlane& plane ) const;
	int				PlaneSide( const idPlane& plane, const float epsilon = ON_EPSILON ) const;

	bool			PlanesConcave( const idWinding& w2, const idVec3& normal1, const idVec3& normal2, float dist1, float dist2 ) const;

	bool			PointInside( const idVec3& normal, const idVec3& point, const float epsilon ) const;
	// returns true if the line or ray intersects the winding
	bool			LineIntersection( const idPlane& windingPlane, const idVec3& start, const idVec3& end, bool backFaceCull = false ) const;
	// intersection point is start + dir * scale
	bool			RayIntersection( const idPlane& windingPlane, const idVec3& start, const idVec3& dir, float& scale, bool backFaceCull = false ) const;

	static float	TriangleArea( const idVec3& a, const idVec3& b, const idVec3& c );

protected:
	idVec5 * 		p;						// pointer to point data
	int				numPoints;				// number of points
	int				allocedSize;

	bool			EnsureAlloced( int n, bool keep = false );
	virtual bool	ReAllocate( int n, bool keep = false );
};

ID_INLINE idWinding::idWinding()
{
	numPoints = allocedSize = 0;
	p = NULL;
}

ID_INLINE idWinding::idWinding( int n )
{
	numPoints = allocedSize = 0;
	p = NULL;
	EnsureAlloced( n );
}

ID_INLINE idWinding::idWinding( const idVec3* verts, const int n )
{
	numPoints = allocedSize = 0;
	p = NULL;
	if( !EnsureAlloced( n ) )
	{
		numPoints = 0;
		return;
	}
	for( int i = 0; i < n; i++ )
	{
		p[ i ].ToVec3() = verts[ i ];
		p[ i ].s = p[ i ].t = 0.0f;
	}
	numPoints = n;
}

ID_INLINE idWinding::idWinding( const idVec3& normal, const float dist )
{
	numPoints = allocedSize = 0;
	p = NULL;
	BaseForPlane( normal, dist );
}

ID_INLINE idWinding::idWinding( const idPlane& plane )
{
	numPoints = allocedSize = 0;
	p = NULL;
	BaseForPlane( plane );
}

ID_INLINE idWinding::idWinding( const idWinding& winding )
{
	numPoints = allocedSize = 0;
	p = NULL;
	if( !EnsureAlloced( winding.GetNumPoints() ) )
	{
		numPoints = 0;
		return;
	}
	for( int i = 0; i < winding.GetNumPoints(); i++ )
	{
		p[ i ] = winding[ i ];
	}
	numPoints = winding.GetNumPoints();
}

ID_INLINE idWinding::~idWinding()
{
	delete[] p;
	p = NULL;
}

ID_INLINE idWinding& idWinding::operator=( const idWinding& winding )
{
	if( !EnsureAlloced( winding.numPoints ) )
	{
		numPoints = 0;
		return *this;
	}
	for( int i = 0; i < winding.numPoints; i++ )
	{
		p[ i ] = winding.p[ i ];
	}
	numPoints = winding.numPoints;
	return *this;
}

ID_INLINE const idVec5& idWinding::operator[]( const int index ) const
{
	//assert( index >= 0 && index < numPoints );
	return p[ index ];
}

ID_INLINE idVec5& idWinding::operator[]( const int index )
{
	//assert( index >= 0 && index < numPoints );
	return p[ index ];
}

ID_INLINE idWinding& idWinding::operator+=( const idVec3& v )
{
	AddPoint( v );
	return *this;
}

ID_INLINE idWinding& idWinding::operator+=( const idVec5& v )
{
	AddPoint( v );
	return *this;
}

ID_INLINE void idWinding::AddPoint( const idVec3& v )
{
	if( !EnsureAlloced( numPoints + 1, true ) )
	{
		return;
	}
	p[ numPoints ] = v;
	numPoints++;
}

ID_INLINE void idWinding::AddPoint( const idVec5& v )
{
	if( !EnsureAlloced( numPoints + 1, true ) )
	{
		return;
	}
	p[ numPoints ] = v;
	numPoints++;
}

ID_INLINE int idWinding::GetNumPoints() const
{
	return numPoints;
}

ID_INLINE void idWinding::SetNumPoints( int n )
{
	if( !EnsureAlloced( n, true ) )
	{
		return;
	}
	numPoints = n;
}

ID_INLINE void idWinding::Clear()
{
	numPoints = 0;
	allocedSize = 0;
	delete[] p;
	p = NULL;
}

ID_INLINE void idWinding::BaseForPlane( const idPlane& plane )
{
	BaseForPlane( plane.Normal(), plane.Dist() );
}

ID_INLINE bool idWinding::EnsureAlloced( int n, bool keep )
{
	if( n > allocedSize )
	{
		return ReAllocate( n, keep );
	}
	return true;
}

ID_INLINE void idWinding::Rotate( const idVec3& origin, const idMat3& axis )
{
	for( int i = 0; i < numPoints; i++ )
	{
		p[ i ].ToVec3() -= origin;
		p[ i ].ToVec3() *= axis;
		p[ i ].ToVec3() += origin;
	}
}

ID_INLINE float idWinding::TriangleArea( const idVec3& a, const idVec3& b, const idVec3& c )
{
	idVec3 v1 = b - a;
	idVec3 v2 = c - a;
	idVec3 cross = v1.Cross( v2 );
	return 0.5f * cross.Length();
}

/*
===============================================================================

	idFixedWinding is a fixed buffer size winding not using
	memory allocations.

	When an operation would overflow the fixed buffer a warning
	is printed and the operation is safely cancelled.

===============================================================================
*/

class idFixedWinding : public idWinding {
public:
	static const int MAX_POINTS = 64;

	idFixedWinding();
	explicit idFixedWinding( const int n );
	explicit idFixedWinding( const idVec3* verts, const int n );
	explicit idFixedWinding( const idVec3& normal, const float dist );
	explicit idFixedWinding( const idPlane& plane );
	explicit idFixedWinding( const idWinding& winding );
	explicit idFixedWinding( const idFixedWinding& winding );
	virtual	~idFixedWinding();

	auto & operator=( const idWinding& winding );

	virtual void	Clear();

	// splits the winding in a back and front part, 'this' becomes the front part
	// returns a SIDE_?
	int				Split( idFixedWinding* back, const idPlane& plane, const float epsilon = ON_EPSILON );

protected:
	idVec5			data[ MAX_POINTS ];	// point data

	virtual bool	ReAllocate( int n, bool keep = false );
};

ID_INLINE idFixedWinding::idFixedWinding()
{
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS;
}

ID_INLINE idFixedWinding::idFixedWinding( int n )
{
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS;
}

ID_INLINE idFixedWinding::idFixedWinding( const idVec3* verts, const int n )
{
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS;
	if( !EnsureAlloced( n ) )
	{
		numPoints = 0;
		return;
	}
	for( int i = 0; i < n; i++ )
	{
		p[ i ].ToVec3() = verts[ i ];
		p[ i ].s = p[ i ].t = 0;
	}
	numPoints = n;
}

ID_INLINE idFixedWinding::idFixedWinding( const idVec3& normal, const float dist )
{
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS;
	BaseForPlane( normal, dist );
}

ID_INLINE idFixedWinding::idFixedWinding( const idPlane& plane )
{
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS;
	BaseForPlane( plane );
}

ID_INLINE idFixedWinding::idFixedWinding( const idWinding& winding )
{
	p = data;
	allocedSize = MAX_POINTS;
	if( !EnsureAlloced( winding.GetNumPoints() ) )
	{
		numPoints = 0;
		return;
	}
	for( int i = 0; i < winding.GetNumPoints(); i++ )
	{
		p[ i ] = winding[ i ];
	}
	numPoints = winding.GetNumPoints();
}

ID_INLINE idFixedWinding::idFixedWinding( const idFixedWinding & winding )
{
	p = data;
	allocedSize = MAX_POINTS;
	if( !EnsureAlloced( winding.GetNumPoints() ) )
	{
		numPoints = 0;
		return;
	}
	for( int i = 0; i < winding.GetNumPoints(); i++ )
	{
		p[ i ] = winding[ i ];
	}
	numPoints = winding.GetNumPoints();
}

ID_INLINE idFixedWinding::~idFixedWinding()
{
	p = NULL;	// otherwise it tries to free the fixed buffer
}

ID_INLINE auto & idFixedWinding::operator=( const idWinding& winding )
{
	if( !EnsureAlloced( winding.GetNumPoints() ) )
	{
		numPoints = 0;
		return *this;
	}
	for( int i = 0; i < winding.GetNumPoints(); i++ )
	{
		p[ i ] = winding[ i ];
	}
	numPoints = winding.GetNumPoints();
	return *this;
}

ID_INLINE void idFixedWinding::Clear()
{
	numPoints = 0;
}

#endif	/* !__WINDING_H__ */
