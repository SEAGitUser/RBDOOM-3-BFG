// Copyright (C) 2007 Id Software, Inc.
//

#ifndef _MATH_RADIANS_H_INC_
#define _MATH_RADIANS_H_INC_

/*
===============================================================================
	Euler angles

	This is basically a duplicate of idAngles, but used radians rather than
	degrees (to avoid the conversion before trig calls)

	All trig calls use float precision

	ToMat3 passes in workspace to avoid a memcpy

===============================================================================
*/

#ifndef M_PI
#  define M_PI (3.1415926536f)
#endif

class idVec3;
class idMat3;

class idRadAngles {
public:
	float			pitch;
	float			yaw;
	float			roll;

	idRadAngles() {}
	idRadAngles( float pitch, float yaw, float roll );
	explicit		idRadAngles( const idVec3 &v );

	void 			Set( float pitch, float yaw, float roll );
	idRadAngles &	Zero();

	float			operator[]( int index ) const;
	float &			operator[]( int index );
	idRadAngles		operator-() const;
	idRadAngles &	operator=( const idRadAngles &a );
	idRadAngles &	operator=( const idVec3 &a );
	idRadAngles		operator+( const idRadAngles &a ) const;
	idRadAngles		operator+( const idVec3 &a ) const;
	idRadAngles &	operator+=( const idRadAngles &a );
	idRadAngles &	operator+=( const idVec3 &a );
	idRadAngles		operator-( const idRadAngles &a ) const;
	idRadAngles		operator-( const idVec3 &a ) const;
	idRadAngles &	operator-=( const idRadAngles &a );
	idRadAngles &	operator-=( const idVec3 &a );
	idRadAngles		operator*( const float a ) const;
	idRadAngles &	operator*=( const float a );

	friend idRadAngles	operator+( const idVec3 &a, const idRadAngles &b );
	friend idRadAngles	operator-( const idVec3 &a, const idRadAngles &b );
	friend idRadAngles	operator*( const float a, const idRadAngles &b );

	bool			Compare( const idRadAngles &a ) const;							// exact compare, no epsilon
	bool			Compare( const idRadAngles &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==( const idRadAngles &a ) const;						// exact compare, no epsilon
	bool			operator!=( const idRadAngles &a ) const;						// exact compare, no epsilon

	idRadAngles &	NormalizeFull();	// normalizes 'this'
	idRadAngles &	NormalizeHalf();	// normalizes 'this'

	void			ToVectors( idVec3 *forward, idVec3 *right = NULL, idVec3 *up = NULL ) const;
	idVec3			ToForward() const;
	idMat3			&ToMat3( idMat3 &mat ) const;

	const float *	ToFloatPtr() const { return( &pitch ); }
	float *			ToFloatPtr() { return( &pitch ); }
};

ID_INLINE idRadAngles::idRadAngles( float pitch, float yaw, float roll )
{
	this->pitch = pitch;
	this->yaw = yaw;
	this->roll = roll;
}

ID_INLINE idRadAngles::idRadAngles( const idVec3 &v )
{
	this->pitch = v[ 0 ];
	this->yaw = v[ 1 ];
	this->roll = v[ 2 ];
}

ID_INLINE void idRadAngles::Set( float pitch, float yaw, float roll )
{
	this->pitch = pitch;
	this->yaw = yaw;
	this->roll = roll;
}

ID_INLINE idRadAngles &idRadAngles::Zero()
{
	pitch = 0.0f;
	yaw = 0.0f;
	roll = 0.0f;
	return( *this );
}

ID_INLINE float idRadAngles::operator[]( int index ) const
{
	assert( ( index >= 0 ) && ( index < 3 ) );
	return( ( &pitch )[ index ] );
}

ID_INLINE float &idRadAngles::operator[]( int index )
{
	assert( ( index >= 0 ) && ( index < 3 ) );
	return( ( &pitch )[ index ] );
}

ID_INLINE idRadAngles idRadAngles::operator-() const
{
	return( idRadAngles( -pitch, -yaw, -roll ) );
}

ID_INLINE idRadAngles &idRadAngles::operator=( const idRadAngles &a )
{
	pitch = a.pitch;
	yaw = a.yaw;
	roll = a.roll;
	return( *this );
}

ID_INLINE idRadAngles &idRadAngles::operator=( const idVec3 &a )
{
	pitch = a.x;
	yaw = a.y;
	roll = a.z;
	return( *this );
}

ID_INLINE idRadAngles idRadAngles::operator+( const idRadAngles &a ) const
{
	return( idRadAngles( pitch + a.pitch, yaw + a.yaw, roll + a.roll ) );
}

ID_INLINE idRadAngles idRadAngles::operator+( const idVec3 &a ) const
{
	return( idRadAngles( pitch + a.x, yaw + a.y, roll + a.z ) );
}

ID_INLINE idRadAngles& idRadAngles::operator+=( const idRadAngles &a )
{
	pitch += a.pitch;
	yaw += a.yaw;
	roll += a.roll;
	return( *this );
}

ID_INLINE idRadAngles& idRadAngles::operator+=( const idVec3 &a )
{
	pitch += a.x;
	yaw += a.y;
	roll += a.z;
	return( *this );
}

ID_INLINE idRadAngles idRadAngles::operator-( const idRadAngles &a ) const
{
	return( idRadAngles( pitch - a.pitch, yaw - a.yaw, roll - a.roll ) );
}

ID_INLINE idRadAngles idRadAngles::operator-( const idVec3 &a ) const
{
	return( idRadAngles( pitch - a.x, yaw - a.y, roll - a.z ) );
}

ID_INLINE idRadAngles& idRadAngles::operator-=( const idRadAngles &a )
{
	pitch -= a.pitch;
	yaw -= a.yaw;
	roll -= a.roll;
	return( *this );
}

ID_INLINE idRadAngles& idRadAngles::operator-=( const idVec3 &a )
{
	pitch -= a.x;
	yaw -= a.y;
	roll -= a.z;
	return( *this );
}

ID_INLINE idRadAngles idRadAngles::operator*( const float a ) const
{
	return( idRadAngles( pitch * a, yaw * a, roll * a ) );
}

ID_INLINE idRadAngles& idRadAngles::operator*=( float a )
{
	pitch *= a;
	yaw *= a;
	roll *= a;
	return( *this );
}

ID_INLINE idRadAngles operator+( const idVec3 &a, const idRadAngles &b )
{
	return( idRadAngles( a.x + b.pitch, a.y + b.yaw, a.z + b.roll ) );
}

ID_INLINE idRadAngles operator-( const idVec3 &a, const idRadAngles &b )
{
	return( idRadAngles( a.x - b.pitch, a.y - b.yaw, a.z - b.roll ) );
}

ID_INLINE idRadAngles operator*( const float a, const idRadAngles &b )
{
	return( idRadAngles( a * b.pitch, a * b.yaw, a * b.roll ) );
}

ID_INLINE bool idRadAngles::Compare( const idRadAngles &a ) const
{
	return( ( a.pitch == pitch ) && ( a.yaw == yaw ) && ( a.roll == roll ) );
}

ID_INLINE bool idRadAngles::Compare( const idRadAngles &a, const float epsilon ) const
{
	if( idMath::Fabs( pitch - a.pitch ) > epsilon )
	{
		return( false );
	}

	if( idMath::Fabs( yaw - a.yaw ) > epsilon )
	{
		return( false );
	}

	if( idMath::Fabs( roll - a.roll ) > epsilon )
	{
		return( false );
	}

	return( true );
}

ID_INLINE bool idRadAngles::operator==( const idRadAngles &a ) const
{
	return( Compare( a ) );
}

ID_INLINE bool idRadAngles::operator!=( const idRadAngles &a ) const
{
	return( !Compare( a ) );
}

ID_INLINE idVec3 idRadAngles::ToForward() const
{
	float	sp, sy, cp, cy;

	idMath::SinCos( yaw, sy, cy );
	idMath::SinCos( pitch, sp, cp );

	return( idVec3( cp * cy, cp * sy, -sp ) );
}

ID_INLINE idMat3& idRadAngles::ToMat3( idMat3 &mat ) const
{
	float	sr, sp, sy, cr, cp, cy;

	idMath::SinCos( yaw, sy, cy );
	idMath::SinCos( pitch, sp, cp );
	idMath::SinCos( roll, sr, cr );

	mat[ 0 ].Set( cp * cy, cp * sy, -sp );
	mat[ 1 ].Set( sr * sp * cy + cr * -sy, sr * sp * sy + cr * cy, sr * cp );
	mat[ 2 ].Set( cr * sp * cy + -sr * -sy, cr * sp * sy + -sr * cy, cr * cp );

	return( mat );
}

#endif // _MATH_RADIANS_H_INC_
