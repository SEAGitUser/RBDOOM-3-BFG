#pragma hdrstop
#include "precompiled.h"

#include "tr_local.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// using namespace idRenderStateParse;

/*
===============
 idRenderStateParse::NameToSrcBlendMode
===============
*/
int NameToSrcBlendMode( const idStr & name )
{
	/*/*/if( !name.Icmp( "GL_ONE" ) ) return GLS_SRCBLEND_ONE;
	else if( !name.Icmp( "GL_ZERO" ) ) return GLS_SRCBLEND_ZERO;
	else if( !name.Icmp( "GL_DST_COLOR" ) ) return GLS_SRCBLEND_DST_COLOR;
	else if( !name.Icmp( "GL_ONE_MINUS_DST_COLOR" ) ) return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
	else if( !name.Icmp( "GL_SRC_ALPHA" ) ) return GLS_SRCBLEND_SRC_ALPHA;
	else if( !name.Icmp( "GL_ONE_MINUS_SRC_ALPHA" ) ) return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	else if( !name.Icmp( "GL_DST_ALPHA" ) ) return GLS_SRCBLEND_DST_ALPHA;
	else if( !name.Icmp( "GL_ONE_MINUS_DST_ALPHA" ) ) return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
	else if( !name.Icmp( "GL_SRC_ALPHA_SATURATE" ) ) { assert( 0 ); return GLS_SRCBLEND_SRC_ALPHA; } // FIX ME

	//common->Warning( "unknown blend mode '%s' in material '%s'", name.c_str(), GetName() );
	//SetMaterialFlag( MF_DEFAULTED );
	return GLS_SRCBLEND_ONE;
}

/*
===============
 idRenderStateParse::NameToDstBlendMode
===============
*/
int NameToDstBlendMode( const idStr & name )
{
	/*/*/if( !name.Icmp( "GL_ONE" ) ) return GLS_DSTBLEND_ONE;
	else if( !name.Icmp( "GL_ZERO" ) ) return GLS_DSTBLEND_ZERO;
	else if( !name.Icmp( "GL_SRC_ALPHA" ) ) return GLS_DSTBLEND_SRC_ALPHA;
	else if( !name.Icmp( "GL_ONE_MINUS_SRC_ALPHA" ) ) return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	else if( !name.Icmp( "GL_DST_ALPHA" ) ) return GLS_DSTBLEND_DST_ALPHA;
	else if( !name.Icmp( "GL_ONE_MINUS_DST_ALPHA" ) ) return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
	else if( !name.Icmp( "GL_SRC_COLOR" ) ) return GLS_DSTBLEND_SRC_COLOR;
	else if( !name.Icmp( "GL_ONE_MINUS_SRC_COLOR" ) ) return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;

	//common->Warning( "unknown blend mode '%s' in material '%s'", name.c_str(), GetName() );
	//SetMaterialFlag( MF_DEFAULTED );
	return GLS_DSTBLEND_ONE;
}

/*
================
idMaterial::ParseBlend
================
*/
bool ParseBlend( idParser & src, uint64 stateBits )
{
	idToken token;
	if( !src.ReadTokenOnLine( &token ) )
	{
		src.Error( "ParseBlend() no blend option specified" );
		return false;
	}

	// blending combinations

	/*/*/if( token.is( "blend" ) )
	{
		stateBits |= ( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
		return true;
	}
	else if( token.is( "add" ) )
	{
		stateBits |= ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
		return true;
	}
	else if( token.is( "filter" ) || token.is( "modulate" ) )
	{
		stateBits |= ( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO );
		return true;
	}
	else if( token.is( "none" ) || token.is( "replace" ) )
	{
		// none is used when defining an alpha mask that doesn't draw
		stateBits |= ( GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE );
		return true;
	}

	// src blend mode

	/*/*/if( token.is( "ONE" ) ) stateBits |= GLS_SRCBLEND_ONE;
	else if( token.is( "ZERO" ) ) stateBits |= GLS_SRCBLEND_ZERO;
	else if( token.is( "DST_COLOR" ) ) stateBits |= GLS_SRCBLEND_DST_COLOR;
	else if( token.is( "ONE_MINUS_DST_COLOR" ) ) stateBits |= GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
	else if( token.is( "SRC_ALPHA" ) ) stateBits |= GLS_SRCBLEND_SRC_ALPHA;
	else if( token.is( "ONE_MINUS_SRC_ALPHA" ) ) stateBits |= GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	else if( token.is( "DST_ALPHA" ) ) stateBits |= GLS_SRCBLEND_DST_ALPHA;
	else if( token.is( "ONE_MINUS_DST_ALPHA" ) ) stateBits |= GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
	else if( token.is( "SRC_ALPHA_SATURATE" ) )
	{
		assert( 0 ); stateBits |= GLS_SRCBLEND_SRC_ALPHA;
	} // FIX ME ?
	else {
		src.Error( "ParseBlend() bad src token '%s'", token.c_str() );
		return false;
	}

	// dst blend mode

	if( !src.ReadTokenOnLine( &token ) )
	{
		src.Error( "ParseBlend() no dst specified" );
		return false;
	}

	/*/*/if( token.is( "ONE" ) ) stateBits |= GLS_DSTBLEND_ONE;
	else if( token.is( "ZERO" ) ) stateBits |= GLS_DSTBLEND_ZERO;
	else if( token.is( "SRC_ALPHA" ) ) stateBits |= GLS_DSTBLEND_SRC_ALPHA;
	else if( token.is( "ONE_MINUS_SRC_ALPHA" ) ) stateBits |= GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	else if( token.is( "DST_ALPHA" ) ) stateBits |= GLS_DSTBLEND_DST_ALPHA;
	else if( token.is( "ONE_MINUS_DST_ALPHA" ) ) stateBits |= GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
	else if( token.is( "SRC_COLOR" ) ) stateBits |= GLS_DSTBLEND_SRC_COLOR;
	else if( token.is( "ONE_MINUS_SRC_COLOR" ) ) stateBits |= GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
	else {
		src.Error( "ParseBlend() bad dst token '%s'", token.c_str() );
		return false;
	}

	return true;
}

// glBlendEquation — specify the equation used for both the RGB blend equation and the Alpha blend equation
bool ParseBlendOp( idParser & src, uint64 & stateBits )
{
	idToken token;

	if( !src.ReadTokenOnLine( &token ) )
	{
		src.Error( "ParseBlendOp() no Op specified" );
		return false;
	}

	if( token.is( "ADD" ) ) stateBits |= GLS_BLENDOP_ADD;
	if( token.is( "SUB" ) ) stateBits |= GLS_BLENDOP_SUB;
	if( token.is( "INV_SUB" ) ) stateBits |= GLS_BLENDOP_INVSUB;
	if( token.is( "MIN" ) ) stateBits |= GLS_BLENDOP_MIN;
	if( token.is( "MAX" ) ) stateBits |= GLS_BLENDOP_MAX;
	else {
		src.Error( "ParseBlendOp() bad Op token '%s'", token.c_str() );
		return false;
	}

	return true;
}

bool ParseStancilOp( idParser & src, uint64 & stateBits )
{
	idToken token;

	// Read sFail option
	if( !src.ReadTokenOnLine( &token ) )
	{
		src.Error( "ParseStancilOp() no sFail option specified" );
		return false;
	}
	/*/*/if( token.is( "KEEP" ) ) stateBits |= GLS_STENCIL_OP_FAIL_KEEP;
	else if( token.is( "ZERO" ) ) stateBits |= GLS_STENCIL_OP_FAIL_ZERO;
	else if( token.is( "REPLACE" ) ) stateBits |= GLS_STENCIL_OP_FAIL_REPLACE;
	else if( token.is( "INCR" ) ) stateBits |= GLS_STENCIL_OP_FAIL_INCR;
	else if( token.is( "DECR" ) ) stateBits |= GLS_STENCIL_OP_FAIL_DECR;
	else if( token.is( "INVERT" ) ) stateBits |= GLS_STENCIL_OP_FAIL_INVERT;
	else if( token.is( "INCR_WRAP" ) ) stateBits |= GLS_STENCIL_OP_FAIL_INCR_WRAP;
	else if( token.is( "DECR_WRAP" ) ) stateBits |= GLS_STENCIL_OP_FAIL_DECR_WRAP;
	else {
		src.Error( "ParseStancilOp() bad sFail token '%s'", token.c_str() );
		return false;
	}


	// Read zFail option
	if( src.ReadTokenOnLine( &token ) )
	{
		src.Error( "ParseStancilOp() no zFail option specified" );
		return false;
	}
	/*/*/if( token.is( "KEEP" ) ) stateBits |= GLS_STENCIL_OP_ZFAIL_KEEP;
	else if( token.is( "ZERO" ) ) stateBits |= GLS_STENCIL_OP_ZFAIL_ZERO;
	else if( token.is( "REPLACE" ) ) stateBits |= GLS_STENCIL_OP_ZFAIL_REPLACE;
	else if( token.is( "INCR" ) ) stateBits |= GLS_STENCIL_OP_ZFAIL_INCR;
	else if( token.is( "DECR" ) ) stateBits |= GLS_STENCIL_OP_ZFAIL_DECR;
	else if( token.is( "INVERT" ) ) stateBits |= GLS_STENCIL_OP_ZFAIL_INVERT;
	else if( token.is( "INCR_WRAP" ) ) stateBits |= GLS_STENCIL_OP_ZFAIL_INCR_WRAP;
	else if( token.is( "DECR_WRAP" ) ) stateBits |= GLS_STENCIL_OP_ZFAIL_DECR_WRAP;
	else {
		src.Error( "ParseStancilOp() bad zFail token '%s'", token.c_str() );
		return false;
	}

	// Read zPass option
	if( src.ReadTokenOnLine( &token ) )
	{
		src.Error( "ParseStancilOp() no zPass option specified" );
		return false;
	}
	/*/*/if( token.is( "KEEP" ) ) stateBits |= GLS_STENCIL_OP_PASS_KEEP;
	else if( token.is( "ZERO" ) ) stateBits |= GLS_STENCIL_OP_PASS_ZERO;
	else if( token.is( "REPLACE" ) ) stateBits |= GLS_STENCIL_OP_PASS_REPLACE;
	else if( token.is( "INCR" ) ) stateBits |= GLS_STENCIL_OP_PASS_INCR;
	else if( token.is( "DECR" ) ) stateBits |= GLS_STENCIL_OP_PASS_DECR;
	else if( token.is( "INVERT" ) ) stateBits |= GLS_STENCIL_OP_PASS_INVERT;
	else if( token.is( "INCR_WRAP" ) ) stateBits |= GLS_STENCIL_OP_PASS_INCR_WRAP;
	else if( token.is( "DECR_WRAP" ) ) stateBits |= GLS_STENCIL_OP_PASS_DECR_WRAP;
	else {
		src.Error( "ParseStancilOp() bad zPass token '%s'", token.c_str() );
		return false;
	}

	return true;
}

bool ParseStancilFunc( idParser & src, uint64 & stateBits )
{
	idToken token;

	// Read sFunc option
	if( src.ReadTokenOnLine( &token ) )
	{
		/*/*/if( token.is( "NEVER" ) ) stateBits |= GLS_STENCIL_FUNC_NEVER;
		else if( token.is( "LESS" ) ) stateBits |= GLS_STENCIL_FUNC_LESS;
		else if( token.is( "EQUAL" ) ) stateBits |= GLS_STENCIL_FUNC_EQUAL;
		else if( token.is( "LEQUAL" ) ) stateBits |= GLS_STENCIL_FUNC_LEQUAL;
		else if( token.is( "GREATER" ) ) stateBits |= GLS_STENCIL_FUNC_GREATER;
		else if( token.is( "NOTEQUAL" ) ) stateBits |= GLS_STENCIL_FUNC_NOTEQUAL;
		else if( token.is( "GEQUAL" ) ) stateBits |= GLS_STENCIL_FUNC_GEQUAL;
		else if( token.is( "ALWAYS" ) ) stateBits |= GLS_STENCIL_FUNC_ALWAYS;
		else {
			src.Error( "ParseStancilFunc() bad sFunc token '%s'", token.c_str() );
			return false;
		}
	}
	else {
		src.Error( "ParseStancilFunc() no sFunc specified" );
		return false;
	}

	// Read ref value
	if( src.ExpectTokenType( TT_NUMBER, TT_INTEGER, token ) )
	{
		uint64 ref = token.GetUnsignedIntValue();
		stateBits |= GLS_STENCIL_MAKE_REF( ref );
	}
	else {
		src.Error( "ParseStancilFunc() no ref value specified" );
		return false;
	}

	// Read mask value
	if( src.ExpectTokenType( TT_NUMBER, TT_INTEGER, token ) )
	{
		uint64 mask = token.GetUnsignedIntValue();
		stateBits |= GLS_STENCIL_MAKE_MASK( mask );
	}
	else {
		src.Error( "ParseStancilFunc() no mask value specified" );
		return false;
	}

	return true;
}

bool ParseDepthFunc( idParser & src, uint64 & stateBits )
{
	idToken token;

	if( src.ReadTokenOnLine( &token ) )
	{
		/*/*/if( token.is( "EQUAL" ) ) stateBits |= GLS_DEPTHFUNC_EQUAL;
		else if( token.is( "LESS" ) ) stateBits |= GLS_DEPTHFUNC_LESS;
		else if( token.is( "LEQUAL" ) ) stateBits |= GLS_DEPTHFUNC_LEQUAL;
		else if( token.is( "GREATER" ) ) stateBits |= GLS_DEPTHFUNC_GREATER;
		else if( token.is( "NOTEQUAL" ) ) stateBits |= GLS_DEPTHFUNC_NOTEQUAL;
		else if( token.is( "GEQUAL" ) ) stateBits |= GLS_DEPTHFUNC_GEQUAL;
		else if( token.is( "NEVER" ) ) stateBits |= GLS_DEPTHFUNC_NEVER;
		else if( token.is( "ALWAYS" ) ) stateBits |= GLS_DEPTHFUNC_ALWAYS;
		else {
			src.Error( "ParseDepthFunc() bad dFunc token '%s'", token.c_str() );
			return false;
		}

		return true;
	}

	src.Error( "ParseDepthFunc() no dFunc specified" );
	return false;
}

/*
=======================================================
 ParseGLState
=======================================================
*/
bool idRenderStateParse::ParseGLState( idParser & src, uint64 & stateBits )
{
	idToken token;
	// registerCount 32

	if( !src.ExpectAnyToken( &token ) )
		return false;

	if( token.is( "blend" ) )
	{
		return ParseBlend( src, stateBits );
	}
	if( token.is( "blendOp" ) ) // ADD MIN MAX SUB INV_SUB
	{
		return ParseBlendOp( src, stateBits );
	}

	// color mask options
	{
		if( token.is( "maskRed" ) )
		{
			stateBits |= GLS_REDMASK;
			return true;
		}
		if( token.is( "maskGreen" ) )
		{
			stateBits |= GLS_GREENMASK;
			return true;
		}
		if( token.is( "maskBlue" ) )
		{
			stateBits |= GLS_BLUEMASK;
			return true;
		}
		if( token.is( "maskAlpha" ) /*|| token.is( "alphaMask" )*/ )
		{
			stateBits |= GLS_ALPHAMASK;
			return true;
		}
		if( token.is( "maskColor" ) /*|| token.is( "colorMask" )*/ )
		{
			stateBits |= GLS_COLORMASK;
			return true;
		}
	}

	// depth / stencil options
	{
		if( token.is( "maskDepth" ) )
		{
			stateBits |= GLS_DEPTHMASK;
			return true;
		}
		if( token.is( "maskStencil" ) )
		{
			stateBits |= GLS_STENCILMASK;
			return true;
		}

		if( token.is( "disableDepthTest" ) || token.is( "disableDepth" ) )
		{
			stateBits |= GLS_DISABLE_DEPTHTEST;
			return true;
		}

		if( token.is( "depthFunc" ) )
		{
			return ParseDepthFunc( src, stateBits );
		}

		if( token.is( "stencilOp" ) )
		{
			return ParseStancilOp( src, stateBits );
		}
		if( token.is( "stencilFunc" ) )
		{
			return ParseStancilFunc( src, stateBits );
		}
	}

	if( token.is( "wireframe" ) )
	{
		stateBits |= GLS_POLYMODE_LINE;
		return true;
	}

	if( token.is( "alphaCoverage" ) )
	{
		stateBits |= GLS_ALPHA_COVERAGE;
		return true;
	}

	{
		if( token.is( "twoSided" ) )
		{
			stateBits |= GLS_TWOSIDED;
			return true;
		}
		if( token.is( "backSided" ) )
		{
			stateBits |= GLS_BACKSIDED;
			return true;
		}
	}

	if( token.is( "polygonOffset" ) )
	{
		stateBits |= GLS_POLYGON_OFFSET;
		return true;
	}

	/*if( token.is( "earlyDepthStencil" ) )
	{
		// ...
		return true;
	}*/

	src.Error( "Unknown drawing state option '%s'", token.c_str() );
	return false;
}