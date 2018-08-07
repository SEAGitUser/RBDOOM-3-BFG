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

#ifndef __GLSTATE_H__
#define __GLSTATE_H__

// one/zero is flipped on src/dest so a gl state of 0 is SRC_ONE,DST_ZERO
static const uint64 GLS_SRCBLEND_ONE					= 0ull << 0;				// default
static const uint64 GLS_SRCBLEND_ZERO					= 1ull << 0;
static const uint64 GLS_SRCBLEND_DST_COLOR				= 2ull << 0;
static const uint64 GLS_SRCBLEND_ONE_MINUS_DST_COLOR	= 3ull << 0;
static const uint64 GLS_SRCBLEND_SRC_ALPHA				= 4ull << 0;
static const uint64 GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA	= 5ull << 0;
static const uint64 GLS_SRCBLEND_DST_ALPHA				= 6ull << 0;
static const uint64 GLS_SRCBLEND_ONE_MINUS_DST_ALPHA	= 7ull << 0;
static const uint64 GLS_SRCBLEND_BITS					= 7ull << 0;

static const uint64 GLS_DSTBLEND_ZERO					= 0ull << 3;				// default
static const uint64 GLS_DSTBLEND_ONE					= 1ull << 3;
static const uint64 GLS_DSTBLEND_SRC_COLOR				= 2ull << 3;
static const uint64 GLS_DSTBLEND_ONE_MINUS_SRC_COLOR	= 3ull << 3;
static const uint64 GLS_DSTBLEND_SRC_ALPHA				= 4ull << 3;
static const uint64 GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA	= 5ull << 3;
static const uint64 GLS_DSTBLEND_DST_ALPHA				= 6ull << 3;
static const uint64 GLS_DSTBLEND_ONE_MINUS_DST_ALPHA	= 7ull << 3;
static const uint64 GLS_DSTBLEND_BITS					= 7ull << 3;

#define GLS_DISABLE_BLENDING	( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO )
#define GLS_BLEND_ADD ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE )

//------------------------
// these masks are the inverse, meaning when set the glColorMask value will be 0,
// preventing that channel from being written
//------------------------
static const uint64 GLS_DEPTHMASK						= 1ull << 6;
static const uint64 GLS_REDMASK							= 1ull << 7;
static const uint64 GLS_GREENMASK						= 1ull << 8;
static const uint64 GLS_BLUEMASK						= 1ull << 9;
static const uint64 GLS_ALPHAMASK						= 1ull << 10;

static const uint64 GLS_COLORMASK						= ( GLS_REDMASK | GLS_GREENMASK | GLS_BLUEMASK );

static const uint64 GLS_POLYMODE_LINE					= 1ull << 11;
static const uint64 GLS_POLYGON_OFFSET					= 1ull << 12;

static const uint64 GLS_DEPTHFUNC_LEQUAL				= 0ull << 13; // 000  		//SEA	// default
static const uint64 GLS_DEPTHFUNC_ALWAYS				= 1ull << 13; // 001
static const uint64 GLS_DEPTHFUNC_GEQUAL				= 2ull << 13; // 010		//SEA
static const uint64 GLS_DEPTHFUNC_EQUAL					= 3ull << 13; // 011
static const uint64 GLS_DEPTHFUNC_NOTEQUAL				= 4ull << 13; // 100		//SEA
static const uint64 GLS_DEPTHFUNC_GREATER				= 5ull << 13; // 101		//SEA
static const uint64 GLS_DEPTHFUNC_LESS					= 6ull << 13; // 110		//SEA
static const uint64 GLS_DEPTHFUNC_NEVER					= 7ull << 13; // 111		//SEA
static const uint64 GLS_DEPTHFUNC_BITS					= 7ull << 13;

static const uint64 GLS_DISABLE_DEPTHTEST				= 1ull << 16; //SEA

static const uint64 GLS_BLENDOP_INVSUB					= 1ull << 17; //SEA
static const uint64 GLS_BLENDOP_ADD						= 0ull << 18;			// default
static const uint64 GLS_BLENDOP_SUB						= 1ull << 18;
static const uint64 GLS_BLENDOP_MIN						= 2ull << 18;
static const uint64 GLS_BLENDOP_MAX						= 3ull << 18;
static const uint64 GLS_BLENDOP_BITS					= 7ull << 17;	// 3 << 18;

// stencil bits
static const uint64 GLS_STENCIL_FUNC_REF_SHIFT			= 20;
static const uint64 GLS_STENCIL_FUNC_REF_BITS			= 0xFFll << GLS_STENCIL_FUNC_REF_SHIFT;

static const uint64 GLS_STENCIL_FUNC_MASK_SHIFT			= 28;
static const uint64 GLS_STENCIL_FUNC_MASK_BITS			= 0xFFll << GLS_STENCIL_FUNC_MASK_SHIFT;

#define GLS_STENCIL_MAKE_REF( x ) ( ( (uint64)(x) << GLS_STENCIL_FUNC_REF_SHIFT ) & GLS_STENCIL_FUNC_REF_BITS )
#define GLS_STENCIL_MAKE_MASK( x ) ( ( (uint64)(x) << GLS_STENCIL_FUNC_MASK_SHIFT ) & GLS_STENCIL_FUNC_MASK_BITS )

#define GLS_STENCIL_GET_REF( x ) ( (uint8)( ( x & GLS_STENCIL_FUNC_REF_BITS ) >> GLS_STENCIL_FUNC_REF_SHIFT ) )
#define GLS_STENCIL_GET_MASK( x ) ( (uint8)( ( x & GLS_STENCIL_FUNC_MASK_BITS ) >> GLS_STENCIL_FUNC_MASK_SHIFT ) )

static const uint64 GLS_STENCIL_FUNC_ALWAYS				= 0ull << 36;
static const uint64 GLS_STENCIL_FUNC_LESS				= 1ull << 36;
static const uint64 GLS_STENCIL_FUNC_LEQUAL				= 2ull << 36;
static const uint64 GLS_STENCIL_FUNC_GREATER			= 3ull << 36;
static const uint64 GLS_STENCIL_FUNC_GEQUAL				= 4ull << 36;
static const uint64 GLS_STENCIL_FUNC_EQUAL				= 5ull << 36;
static const uint64 GLS_STENCIL_FUNC_NOTEQUAL			= 6ull << 36;
static const uint64 GLS_STENCIL_FUNC_NEVER				= 7ull << 36;
static const uint64 GLS_STENCIL_FUNC_BITS				= 7ull << 36;

static const uint64 GLS_STENCIL_OP_FAIL_KEEP			= 0ull << 39;
static const uint64 GLS_STENCIL_OP_FAIL_ZERO			= 1ull << 39;
static const uint64 GLS_STENCIL_OP_FAIL_REPLACE			= 2ull << 39;
static const uint64 GLS_STENCIL_OP_FAIL_INCR			= 3ull << 39;
static const uint64 GLS_STENCIL_OP_FAIL_DECR			= 4ull << 39;
static const uint64 GLS_STENCIL_OP_FAIL_INVERT			= 5ull << 39;
static const uint64 GLS_STENCIL_OP_FAIL_INCR_WRAP		= 6ull << 39;
static const uint64 GLS_STENCIL_OP_FAIL_DECR_WRAP		= 7ull << 39;
static const uint64 GLS_STENCIL_OP_FAIL_BITS			= 7ull << 39;

static const uint64 GLS_STENCIL_OP_ZFAIL_KEEP			= 0ull << 42;
static const uint64 GLS_STENCIL_OP_ZFAIL_ZERO			= 1ull << 42;
static const uint64 GLS_STENCIL_OP_ZFAIL_REPLACE		= 2ull << 42;
static const uint64 GLS_STENCIL_OP_ZFAIL_INCR			= 3ull << 42;
static const uint64 GLS_STENCIL_OP_ZFAIL_DECR			= 4ull << 42;
static const uint64 GLS_STENCIL_OP_ZFAIL_INVERT			= 5ull << 42;
static const uint64 GLS_STENCIL_OP_ZFAIL_INCR_WRAP		= 6ull << 42;
static const uint64 GLS_STENCIL_OP_ZFAIL_DECR_WRAP		= 7ull << 42;
static const uint64 GLS_STENCIL_OP_ZFAIL_BITS			= 7ull << 42;

static const uint64 GLS_STENCIL_OP_PASS_KEEP			= 0ull << 45;
static const uint64 GLS_STENCIL_OP_PASS_ZERO			= 1ull << 45;
static const uint64 GLS_STENCIL_OP_PASS_REPLACE			= 2ull << 45;
static const uint64 GLS_STENCIL_OP_PASS_INCR			= 3ull << 45;
static const uint64 GLS_STENCIL_OP_PASS_DECR			= 4ull << 45;
static const uint64 GLS_STENCIL_OP_PASS_INVERT			= 5ull << 45;
static const uint64 GLS_STENCIL_OP_PASS_INCR_WRAP		= 6ull << 45;
static const uint64 GLS_STENCIL_OP_PASS_DECR_WRAP		= 7ull << 45;
static const uint64 GLS_STENCIL_OP_PASS_BITS			= 7ull << 45;

static const uint64 GLS_STENCIL_OP_BITS					= ( GLS_STENCIL_OP_FAIL_BITS | GLS_STENCIL_OP_ZFAIL_BITS | GLS_STENCIL_OP_PASS_BITS );
/*
static const uint64 GLS_ALPHATEST_FUNC_REF_SHIFT		= 48;
static const uint64 GLS_ALPHATEST_FUNC_REF_BITS			= 0xFFll << GLS_ALPHATEST_FUNC_REF_SHIFT;
#define GLS_ALPHATEST_MAKE_REF( x ) ( ( (uint64)(x) << GLS_ALPHATEST_FUNC_REF_SHIFT ) & GLS_ALPHATEST_FUNC_REF_BITS )
static const uint64 GLS_ALPHATEST_FUNC_ALWAYS			= 0ull << 56;
static const uint64 GLS_ALPHATEST_FUNC_LESS				= 1ull << 56;
static const uint64 GLS_ALPHATEST_FUNC_GREATER			= 2ull << 56;
static const uint64 GLS_ALPHATEST_FUNC_EQUAL			= 3ull << 56;
static const uint64 GLS_ALPHATEST_FUNC_BITS				= 3ull << 56;
*/
static const uint64 GLS_STENCILMASK						= 1ull << 58;	//SEA

static const uint64 GLS_ALPHA_COVERAGE					= 1ull << 59;	//SEA

static const uint64 GLS_BACKSIDED						= 1ull << 60;	//SEA
static const uint64 GLS_TWOSIDED						= 1ull << 61;	//SEA
#define				GLS_FRONTSIDED						0ull


static const uint64 GLS_NEGATIVE_DETERMINANT			= 1ull << 62;	//SEA

static const uint64 GLS_OVERRIDE						= 1ull << 63;		// override the render prog state

static const uint64 GLS_DEFAULT							= 0;

#define STENCIL_SHADOW_TEST_VALUE		128
#define STENCIL_SHADOW_MASK_VALUE		255

enum blendMode_e {
	BM_ADD = 1,
	BM_AVERAGE,
	BM_COLOR_BURN,
	BM_COLOR_DODGE,
	BM_DARKEN,
	BM_DIFFERENCE,
	BM_EXCLUSION,
	BM_GLOW,
	BM_HARD_LIGHT,
	BM_HARD_MIX,
	BM_LIGHTEN,
	BM_LINEAR_BURN,
	BM_LINEAR_DODGE,
	BM_LINEAR_LIGHT,
	BM_MULTIPLY,
	BM_NEGATION,
	BM_NORMAL,
	BM_OVERLAY,
	BM_PHOENIX,
	BM_PIN_LIGHT,
	BM_REFLECT,
	BM_SCREEN,
	BM_SOFT_LIGHT,
	BM_SUBTRACT,
	BM_VIVID_LIGHT
};

namespace idRenderStateParse
{
	// Reads token and parses it.
	bool ParseGLState( idParser &, uint64 & stateBits );
};

#endif /* !__GLSTATE_H__ */
