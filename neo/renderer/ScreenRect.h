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

#ifndef __SCREENRECT_H__
#define __SCREENRECT_H__
#if 0
class idRect {
public:
	short	x1 = 32000;
	short	y1 = 32000;
	short	x2 = -32000;
	short	y2 = -32000;

	bool IsEmpty() const { return( x1 > x2 || y1 > y2 ); }
	short GetWidth() const { return x2 - x1 + 1; }
	short GetHeight() const { return y2 - y1 + 1; }
	int	GetArea() const { return ( x2 - x1 + 1 ) * ( y2 - y1 + 1 ); }

	// expand by one pixel each way to fix roundoffs
	void Expand() { x1--; y1--; x2++; y2++; }

	// adds a point
	void AddPoint( float x, float y )
	{
		short ix = idMath::Ftoi16( x );
		short iy = idMath::Ftoi16( y );

		if( ix < x1 ) x1 = ix;
		if( ix > x2 ) x2 = ix;
		if( iy < y1 ) y1 = iy;
		if( iy > y2 ) y2 = iy;
	}

	void Intersect( const idRect& rect )
	{
		if( rect.x1 > x1 ) x1 = rect.x1;
		if( rect.x2 < x2 ) x2 = rect.x2;
		if( rect.y1 > y1 ) y1 = rect.y1;
		if( rect.y2 < y2 ) y2 = rect.y2;
	}

	void Union( const idRect& rect )
	{
		if( rect.x1 < x1 ) x1 = rect.x1;
		if( rect.x2 > x2 ) x2 = rect.x2;
		if( rect.y1 < y1 ) y1 = rect.y1;
		if( rect.y2 > y2 ) y2 = rect.y2;
	}

	bool Equals( const idRect& rect ) const
	{
		return( x1 == rect.x1 && x2 == rect.x2 && y1 == rect.y1 && y2 == rect.y2 );
	}
};
#endif
/*
================================================================================================

idScreenRect

idScreenRect gets carried around with each drawSurf, so it makes sense
to keep it compact, instead of just using the idBounds class
================================================================================================
*/

class idScreenRect {
public:
	// Inclusive pixel bounds inside viewport
	short		x1;
	short		y1;
	short		x2;
	short		y2;

	// for depth bounds test
	float       zmin;
	float		zmax;

public:
	idScreenRect() { Clear(); }
	idScreenRect( int x1, int y1, int x2, int y2 )
	{
		this->x1 = idMath::ClampShort( x1 );
		this->y1 = idMath::ClampShort( y1 );
		this->x2 = idMath::ClampShort( x2 );
		this->y2 = idMath::ClampShort( y2 );
	}

	// clear to backwards values
	ID_INLINE void Clear()
	{
		x1 = y1 = 32000;
		x2 = y2 = -32000;
		zmin = 0.0f;
		zmax = 1.0f;
	}
	ID_INLINE bool IsEmpty() const { return ( x1 > x2 || y1 > y2 ); }
	ID_INLINE short GetWidth( int border = 1 ) const { return x2 - x1 + border; }
	ID_INLINE short GetHeight( int border = 1 ) const { return y2 - y1 + border; }
	ID_INLINE int GetArea( int border = 1 ) const { return ( x2 - x1 + border ) * ( y2 - y1 + border ); }
	ID_INLINE bool IsValid() const { return( GetWidth() > 0 && GetHeight() > 0 ); }

	// expand by one pixel each way to fix roundoffs
	ID_INLINE void Expand() { x1--; y1--; x2++; y2++; }

	// adds a point
	ID_INLINE void AddPoint( float x, float y )
	{
		int	ix = idMath::Ftoi( x );
		int iy = idMath::Ftoi( y );

		if( ix < x1 ) x1 = ix;
		if( ix > x2 ) x2 = ix;
		if( iy < y1 ) y1 = iy;
		if( iy > y2 ) y2 = iy;
	}

	ID_INLINE void Intersect( const idScreenRect& rect )
	{
		if( rect.x1 > x1 ) x1 = rect.x1;
		if( rect.x2 < x2 ) x2 = rect.x2;
		if( rect.y1 > y1 ) y1 = rect.y1;
		if( rect.y2 < y2 ) y2 = rect.y2;
	}

	ID_INLINE void Union( const idScreenRect& rect )
	{
		if( rect.x1 < x1 ) x1 = rect.x1;
		if( rect.x2 > x2 ) x2 = rect.x2;
		if( rect.y1 < y1 ) y1 = rect.y1;
		if( rect.y2 > y2 ) y2 = rect.y2;
	}

	ID_INLINE bool Equals( const idScreenRect& rect ) const
	{
		return ( x1 == rect.x1 && x2 == rect.x2 && y1 == rect.y1 && y2 == rect.y2 );
	}
};

void R_ShowColoredScreenRect( const idScreenRect& rect, int colorIndex );

#endif /* !__SCREENRECT_H__ */
