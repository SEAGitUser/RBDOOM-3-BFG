// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __COLOR_H__
#define __COLOR_H__
/*
======================================================
	idColor
======================================================
*/
ALIGNTYPE16 class idColor {
public:	
	float			r, g, b, a;

					idColor();
	explicit		idColor( const float r, const float g, const float b, const float a );
	explicit		idColor( const idVec4& rhs );

	void 			Set( const float r, const float g, const float b, const float a );
	ID_INLINE void 	Fill( const float value ) { Set( value, value, value, value ); }
	void			Zero();

	void			Normalize( float scaleFactor = 1.0f );

	int				GetDimension() const;

	void			ToBytes( byte* bytes ) const;
	void			FromBytes( byte* bytes );

	idVec4&			ToVec4();
	const idVec4&	ToVec4() const;

	float			operator[]( const int index ) const;
	float& 			operator[]( const int index );
	idColor			operator-() const;
	idColor			operator*( const float rhs ) const;
	idColor			operator/( const float rhs ) const;
	idColor			operator+( const idColor& rhs ) const;
	idColor			operator-( const idColor& rhs ) const;
	idColor& 		operator+=( const idColor& rhs );
	idColor& 		operator-=( const idColor& rhs );
	idColor& 		operator/=( const idColor& rhs );
	idColor& 		operator/=( const float rhs );
	idColor& 		operator*=( const float rhs );	

	idColor& 		operator=( const idVec4& rhs );

	friend idColor	operator*( const float a1, const idColor b1 );

	bool			Compare( const idColor& rhs ) const;							// exact compare, no epsilon
	bool			Compare( const idColor& rhs, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const idColor& rhs ) const;						// exact compare, no epsilon
	bool			operator!=(	const idColor& rhs ) const;						// exact compare, no epsilon

	const float*	ToFloatPtr() const;
	float*			ToFloatPtr();
	const char*		ToString( int precision = 2 ) const;

	// Linearly inperpolates one vector to another.
	void			Lerp( const idColor& v1, const idColor& v2, const float l );

	static dword	PackColor( const idVec3& color, float alpha );
	static dword	PackColor( const idVec4& color );
	static void		UnpackColor( const dword color, idVec4& unpackedColor );

	//const idColor3&	ToColor3() const;
	//idColor3&		ToColor3();

	static	const ALIGNTYPE16 idColor black;
	static	const ALIGNTYPE16 idColor white;
	static	const ALIGNTYPE16 idColor red;
	static	const ALIGNTYPE16 idColor green;
	static	const ALIGNTYPE16 idColor blue;
	static	const ALIGNTYPE16 idColor yellow;
	static	const ALIGNTYPE16 idColor magenta;
	static	const ALIGNTYPE16 idColor cyan;
	static	const ALIGNTYPE16 idColor orange;
	static	const ALIGNTYPE16 idColor purple;
	static	const ALIGNTYPE16 idColor pink;
	static	const ALIGNTYPE16 idColor brown;
	static	const ALIGNTYPE16 idColor ltGrey;
	static	const ALIGNTYPE16 idColor mdGrey;
	static	const ALIGNTYPE16 idColor dkGrey;
	static	const ALIGNTYPE16 idColor ltBlue;
	static	const ALIGNTYPE16 idColor dkRed;
};

ID_INLINE idColor::idColor()
{
}

ID_INLINE idColor::idColor( const float r, const float g, const float b, const float a )
{
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

ID_INLINE idColor::idColor( const idVec4& rhs ) 
{
	this->r = rhs.x;
	this->g = rhs.y;
	this->b = rhs.z;
	this->a = rhs.w;
}

ID_INLINE void idColor::ToBytes( byte* bytes ) const 
{
	for( int i = 0; i < GetDimension(); i++ ) {
		bytes[ i ] = idMath::Ftob( (*this)[ i ] * 255.0f );
	}
}

ID_INLINE void idColor::FromBytes( byte* bytes )
{
	for( int i = 0; i < GetDimension(); i++ ) {
		(*this)[ i ] = bytes[ i ] / 255.0f;
	}
}

ID_INLINE void idColor::Set( const float r, const float g, const float b, const float a )
{
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

ID_INLINE void idColor::Normalize( float scaleFactor ) 
{
	r = idMath::ClampFloat( 0.0f, 1.0f, r * scaleFactor );
	g = idMath::ClampFloat( 0.0f, 1.0f, g * scaleFactor );
	b = idMath::ClampFloat( 0.0f, 1.0f, b * scaleFactor );
	a = idMath::ClampFloat( 0.0f, 1.0f, a * scaleFactor );
}

ID_INLINE void idColor::Zero()
{
	r = g = b = a = 0.0f;
}

ID_INLINE int idColor::GetDimension() const
{
	return 4;
}

ID_INLINE float idColor::operator[]( int index ) const 
{
	assert( index >= 0 && index < 4 );
	return ( &r )[ index ];
}

ID_INLINE float& idColor::operator[]( int index )
{
	assert( index >= 0 && index < 4 );
	return ( &r )[ index ];
}

ID_INLINE idColor idColor::operator-() const
{
	return idColor( -r, -g, -b, -a );
}

ID_INLINE idColor idColor::operator-( const idColor& rhs ) const 
{
	return idColor( r - rhs.r, g - rhs.g, b - rhs.b, a - rhs.a );
}

ID_INLINE idColor idColor::operator*( const float rhs ) const
{
	return idColor( r * rhs, g * rhs, b * rhs, a * rhs );
}

ID_INLINE idColor idColor::operator/( const float rhs ) const
{
	float inva = 1.0f / rhs;
	return idColor( r * inva, g * inva, b * inva, a * inva );
}

ID_INLINE idColor operator*( const float a1, const idColor b1 )
{
	return idColor( b1.r * a1, b1.g * a1, b1.b * a1, b1.a * a1 );
}

ID_INLINE idColor idColor::operator+( const idColor& rhs ) const
{
	return idColor( r + rhs.r, g + rhs.g, b + rhs.b, a + rhs.a );
}

ID_INLINE idColor& idColor::operator+=( const idColor& rhs )
{
	r += rhs.r;
	g += rhs.g;
	b += rhs.b;
	a += rhs.a;

	return *this;
}

ID_INLINE idColor& idColor::operator/=( const idColor& rhs )
{
	r /= rhs.r;
	g /= rhs.g;
	b /= rhs.b;
	a /= rhs.a;

	return *this;
}

ID_INLINE idColor& idColor::operator/=( const float rhs )
{
	float inva = 1.0f / rhs;
	r *= inva;
	g *= inva;
	b *= inva;
	a *= inva;

	return *this;
}

ID_INLINE idColor& idColor::operator-=( const idColor& rhs ) 
{
	r -= rhs.r;
	g -= rhs.g;
	b -= rhs.b;
	a -= rhs.a;

	return *this;
}

ID_INLINE idColor& idColor::operator*=( const float rhs ) 
{
	r *= rhs;
	g *= rhs;
	b *= rhs;
	a *= rhs;

	return *this;
}

ID_INLINE bool idColor::Compare( const idColor& rhs ) const {
	return ( ( r == rhs.r ) && ( g == rhs.g ) && ( b == rhs.b ) && a == rhs.a );
}

ID_INLINE bool idColor::Compare( const idColor& rhs, const float epsilon ) const 
{
	if ( idMath::Fabs( r - rhs.r ) > epsilon ) {
		return false;
	}
	if ( idMath::Fabs( g - rhs.g ) > epsilon ) {
		return false;
	}
	if ( idMath::Fabs( b - rhs.b ) > epsilon ) {
		return false;
	}
	if ( idMath::Fabs( a - rhs.a ) > epsilon ) {
		return false;
	}
	return true;
}

ID_INLINE bool idColor::operator==( const idColor& rhs ) const 
{
	return Compare( rhs );
}

ID_INLINE bool idColor::operator!=( const idColor& rhs ) const
{
	return !Compare( rhs );
}

ID_INLINE const float* idColor::ToFloatPtr() const 
{
	return &r;
}

ID_INLINE float* idColor::ToFloatPtr()
{
	return &r;
}

ID_INLINE idColor& idColor::operator=( const idVec4& rhs ) 
{
	r = rhs.x;
	g = rhs.y;
	b = rhs.z;
	a = rhs.w;
	return *this;
}

/*
=============
Lerp

Linearly inperpolates one vector to another.
=============
*/
ID_INLINE void idColor::Lerp( const idColor& v1, const idColor& v2, const float l )
{
	if( l <= 0.0f )
	{
		( *this ) = v1;
	}
	else if( l >= 1.0f )
	{
		( *this ) = v2;
	}
	else {
		( *this ) = v1 + l * ( v2 - v1 );
	}
}

ID_INLINE const idVec4&	idColor::ToVec4() const
{
	return *reinterpret_cast< const idVec4 * >( this );
}

ID_INLINE idVec4&	idColor::ToVec4()
{
	return *reinterpret_cast< idVec4 * >( this );
}

#endif // __COLOR_H__
