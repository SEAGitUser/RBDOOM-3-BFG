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

#ifndef __DEVICECONTEXT_H__
#define __DEVICECONTEXT_H__

// device context support for gui stuff
//

#include "Rectangle.h"
#include "../renderer/Font.h"

const int VIRTUAL_WIDTH = 640;
const int VIRTUAL_HEIGHT = 480;
const int BLINK_DIVISOR = 200;

#undef DrawText

class idDeviceContext {
public:
	idDeviceContext();
	~idDeviceContext() { }

	void				Init();
	void				Shutdown();
	bool				Initialized() const { return initialized; }

	void				EnableLocalization();

	void				GetTransformInfo( idVec3& origin, idMat3& mat );

	void				SetTransformInfo( const idVec3& origin, const idMat3& mat );
	void				DrawMaterial( float x, float y, float w, float h, const idMaterial* mat, const idColor &, float scalex = 1.0, float scaley = 1.0 );
	void				DrawRect( float x, float y, float width, float height, float size, const idColor & );
	void				DrawFilledRect( float x, float y, float width, float height, const idColor & );
	int					DrawText( const char* text, float textScale, int textAlign, const idColor &, const idRectangle & rectDraw, bool wrap, int cursor = -1, bool calcOnly = false, idList<int>* breaks = NULL, int limit = 0 );
	void				DrawMaterialRect( float x, float y, float w, float h, float size, const idMaterial*, const idColor & );
	void				DrawStretchPic( float x, float y, float w, float h, float s0, float t0, float s1, float t1, const idMaterial* );
	void				DrawMaterialRotated( float x, float y, float w, float h, const idMaterial*, const idColor &, float scalex = 1.0, float scaley = 1.0, float angle = 0.0f );
	void				DrawStretchPicRotated( float x, float y, float w, float h, float s0, float t0, float s1, float t1, const idMaterial*, float angle = 0.0f );
	void				DrawWinding( idWinding&, const idMaterial* );

	int					CharWidth( const char c, float scale );
	int					TextWidth( const char* text, float scale, int limit );
	int					TextHeight( const char* text, float scale, int limit );
	int					MaxCharHeight( float scale );
	int					MaxCharWidth( float scale );

	idRegion* 			GetTextRegion( const char* text, float textScale, const idRectangle & rectDraw, float xStart, float yStart );

	void				SetSize( float width, float height );
	void				SetOffset( float x, float y );

	const idMaterial* 	GetScrollBarImage( int index );

	void				DrawCursor( float* x, float* y, float size );
	void				SetCursor( int n );

	// clipping rects
	virtual bool		ClippedCoords( float* x, float* y, float* w, float* h, float* s1, float* t1, float* s2, float* t2 ) const;
	virtual void		PushClipRect( const idRectangle& );
	virtual void		PopClipRect();
	virtual void		EnableClipping( bool b );

	void				SetFont( idFont* font ) { activeFont = font; }

	void				SetOverStrike( bool b ) { overStrikeMode = b; }

	bool				GetOverStrike() const { return overStrikeMode; }

	void				DrawEditCursor( float x, float y, float scale );

	enum {
		CURSOR_ARROW,
		CURSOR_HAND,
		CURSOR_HAND_JOY1,
		CURSOR_HAND_JOY2,
		CURSOR_HAND_JOY3,
		CURSOR_HAND_JOY4,
		CURSOR_COUNT
	};

	enum {
		ALIGN_LEFT,
		ALIGN_CENTER,
		ALIGN_RIGHT
	};

	enum {
		SCROLLBAR_HBACK,
		SCROLLBAR_VBACK,
		SCROLLBAR_THUMB,
		SCROLLBAR_RIGHT,
		SCROLLBAR_LEFT,
		SCROLLBAR_UP,
		SCROLLBAR_DOWN,
		SCROLLBAR_COUNT
	};

protected:
	virtual int			DrawText( float x, float y, float scale, const idColor &, const char* text, float adjust, int limit, int style, int cursor = -1 );
	void				PaintChar( float x, float y, const scaledGlyphInfo_t& glyphInfo );
	void				Clear();

	const idMaterial* 	cursorImages[CURSOR_COUNT];
	const idMaterial* 	scrollBarImages[SCROLLBAR_COUNT];
	const idMaterial* 	whiteImage;
	idFont* 			activeFont;

	float				xScale;
	float				yScale;
	float				xOffset;
	float				yOffset;

	int					cursor;

	idList<idRectangle>	clipRects;

	bool				enableClipping;

	bool				overStrikeMode;

	idMat3				mat;
	bool				matIsIdentity;
	idVec3				origin;
	bool				initialized;
};

class idDeviceContextOptimized : public idDeviceContext
{

	virtual bool		ClippedCoords( float* x, float* y, float* w, float* h, float* s1, float* t1, float* s2, float* t2 ) const;
	virtual void		PushClipRect( const idRectangle & );
	virtual void		PopClipRect();
	virtual void		EnableClipping( bool b );

	virtual int			DrawText( float x, float y, float scale, const idColor &, const char* text, float adjust, int limit, int style, int cursor = -1 );

	float				clipX1, clipX2, clipY1, clipY2;
};

#endif /* !__DEVICECONTEXT_H__ */
