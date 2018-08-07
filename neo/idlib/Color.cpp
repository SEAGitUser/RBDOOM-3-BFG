// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

const ALIGNTYPE16 idColor	idColor::black		= idColor( 0.00f, 0.00f, 0.00f, 1.00f );
const ALIGNTYPE16 idColor	idColor::white		= idColor( 1.00f, 1.00f, 1.00f, 1.00f );
const ALIGNTYPE16 idColor	idColor::red		= idColor( 1.00f, 0.00f, 0.00f, 1.00f );
const ALIGNTYPE16 idColor	idColor::green		= idColor( 0.00f, 1.00f, 0.00f, 1.00f );
const ALIGNTYPE16 idColor	idColor::blue		= idColor( 0.00f, 0.00f, 1.00f, 1.00f );
const ALIGNTYPE16 idColor	idColor::yellow		= idColor( 1.00f, 1.00f, 0.00f, 1.00f );
const ALIGNTYPE16 idColor	idColor::magenta	= idColor( 1.00f, 0.00f, 1.00f, 1.00f );
const ALIGNTYPE16 idColor	idColor::cyan		= idColor( 0.00f, 1.00f, 1.00f, 1.00f );
const ALIGNTYPE16 idColor	idColor::orange		= idColor( 1.00f, 0.50f, 0.00f, 1.00f );
const ALIGNTYPE16 idColor	idColor::purple		= idColor( 0.60f, 0.00f, 0.60f, 1.00f );
const ALIGNTYPE16 idColor	idColor::pink		= idColor( 0.73f, 0.40f, 0.48f, 1.00f );
const ALIGNTYPE16 idColor	idColor::brown		= idColor( 0.40f, 0.35f, 0.08f, 1.00f );
const ALIGNTYPE16 idColor	idColor::ltGrey		= idColor( 0.75f, 0.75f, 0.75f, 1.00f );
const ALIGNTYPE16 idColor	idColor::mdGrey		= idColor( 0.50f, 0.50f, 0.50f, 1.00f );
const ALIGNTYPE16 idColor	idColor::dkGrey		= idColor( 0.25f, 0.25f, 0.25f, 1.00f );
const ALIGNTYPE16 idColor	idColor::ltBlue		= idColor( 0.40f, 0.70f, 1.00f, 1.00f );
const ALIGNTYPE16 idColor	idColor::dkRed		= idColor( 0.70f, 0.00f, 0.00f, 1.00f );

/*
=============
idColor::ToString
=============
*/
const char* idColor::ToString( int precision ) const {
	return idStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
================
ColorFloatToByte
================
*/
ID_INLINE static byte ColorFloatToByte( float c ) {
	return idMath::Ftob( c * 255.0f );
}

/*
================
idColor::PackColor
================
*/
dword idColor::PackColor( const idVec3& color, float alpha ) {
	dword dw, dx, dy, dz;

	dx = ColorFloatToByte( color.x );
	dy = ColorFloatToByte( color.y );
	dz = ColorFloatToByte( color.z );
	dw = ColorFloatToByte( alpha );

#if defined( _XENON )
	return ( dx << 24 ) | ( dy << 16 ) | ( dz << 8 ) | ( dw << 0 );
#elif defined( _WIN32 ) || defined( __linux__ )
	return ( dx << 0 ) | ( dy << 8 ) | ( dz << 16 ) | ( dw << 24 );
#elif defined( MACOS_X )
#if defined( __ppc__ )
	return ( dx << 24 ) | ( dy << 16 ) | ( dz << 8 ) | ( dw << 0 );
#else
	return ( dx << 0 ) | ( dy << 8 ) | ( dz << 16 ) | ( dw << 24 );
#endif
#else
#error OS define is required!
#endif
}

/*
================
idColor::PackColor
================
*/
dword idColor::PackColor( const idVec4& color )
{
	dword dx = ColorFloatToByte( color.x );
	dword dy = ColorFloatToByte( color.y );
	dword dz = ColorFloatToByte( color.z );
	dword dw = ColorFloatToByte( color.w );

#if defined( _XENON )
	return ( dx << 24 ) | ( dy << 16 ) | ( dz << 8 ) | ( dw << 0 );
#elif defined( _WIN32 ) || defined( __linux__ )
	return ( dx << 0 ) | ( dy << 8 ) | ( dz << 16 ) | ( dw << 24 );
#elif defined( MACOS_X )
	#if defined( __ppc__ )
	return ( dx << 24 ) | ( dy << 16 ) | ( dz << 8 ) | ( dw << 0 );
	#else
	return ( dx << 0 ) | ( dy << 8 ) | ( dz << 16 ) | ( dw << 24 );
	#endif
#else
#error OS define is required!
#endif
}

/*
================
idColor::UnpackColor
================
*/
void idColor::UnpackColor( const dword color, idVec4& unpackedColor ) {
#if defined( _XENON )
	unpackedColor.Set(
		( ( color >> 24 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ) );
#elif defined( _WIN32 ) || defined( __linux__ )
	unpackedColor.Set(
		( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 24 ) & 255 ) * ( 1.0f / 255.0f ) );
#elif defined(MACOS_X)
	#if defined ( __ppc__ )
	unpackedColor.Set(
		( ( color >> 24 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ) );
	#else
	unpackedColor.Set(
		( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 24 ) & 255 ) * ( 1.0f / 255.0f ) );
	#endif
#else
#error OS define is required!
#endif
}
