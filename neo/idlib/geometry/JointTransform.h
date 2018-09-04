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

#ifndef __JOINTTRANSFORM_H__
#define __JOINTTRANSFORM_H__

/*
===============================================================================

  Joint Quaternion

===============================================================================
*/

ALIGNTYPE16 class idJointQuat {
public:
	const float* 	ToFloatPtr() const { return q.ToFloatPtr(); }
	float* 			ToFloatPtr() { return q.ToFloatPtr(); }

	idQuat			q;
	idVec3			t;
	float			w;
};

// offsets for SIMD code
#define JOINTQUAT_SIZE				(8*4)		// sizeof( idJointQuat )
#define JOINTQUAT_SIZE_SHIFT		5			// log2( sizeof( idJointQuat ) )
#define JOINTQUAT_Q_OFFSET			(0*4)		// offsetof( idJointQuat, q )
#define JOINTQUAT_T_OFFSET			(4*4)		// offsetof( idJointQuat, t )

assert_sizeof( idJointQuat, JOINTQUAT_SIZE );
assert_sizeof( idJointQuat, ( 1 << JOINTQUAT_SIZE_SHIFT ) );
assert_offsetof( idJointQuat, q, JOINTQUAT_Q_OFFSET );
assert_offsetof( idJointQuat, t, JOINTQUAT_T_OFFSET );

/*
===============================================================================

  Compressed Joint Quaternion

===============================================================================
*/

class idCompressedJointQuat {
public:
	static const int	MAX_BONE_LENGTH = 256;

	short				q[3];
	short				t[3];

	static short		QuatToShort( const float c );
	static short		OffsetToShort( const float o );
	static float		ShortToQuat( const short c );
	static float		ShortToOffset( const short o );

	void				ClearQuat() { q[0] = q[1] = q[2] = 0; }
	void				ClearOffset() { t[0] = t[1] = t[2] = 0; }

	idQuat				ToQuat() const;
	idVec3				ToOffset() const;
};

ID_INLINE short idCompressedJointQuat::QuatToShort( const float x )
{
	return idMath::ClampShort( idMath::Ftoi( ( x < 0 ) ? ( x * 32767.0f - 0.5f ) : ( x * 32767.0f + 0.5f ) ) );
}

ID_INLINE short idCompressedJointQuat::OffsetToShort( const float x )
{
	//assert( x > -MAX_BONE_LENGTH && x < MAX_BONE_LENGTH );
	return idMath::ClampShort( idMath::Ftoi( ( x < 0 ) ? ( x * ( 32767.0f / MAX_BONE_LENGTH ) - 0.5f ) : ( x * ( 32767.0f / MAX_BONE_LENGTH ) + 0.5f ) ) );
}

ID_INLINE float idCompressedJointQuat::ShortToQuat( const short x )
{
	return x * ( 1.0f / 32767.0f );
}

ID_INLINE float idCompressedJointQuat::ShortToOffset( const short x )
{
	return x * ( 1.0f / ( 32767.0f / MAX_BONE_LENGTH ) );
}

ID_INLINE idQuat idCompressedJointQuat::ToQuat() const
{
	idQuat quat;
	quat.x = ShortToQuat( q[0] );
	quat.y = ShortToQuat( q[1] );
	quat.z = ShortToQuat( q[2] );
	// take the absolute value because floating point rounding may cause the dot of x,y,z to be larger than 1
	quat.w = idMath::Sqrt( idMath::Fabs( 1.0f - ( quat.x * quat.x + quat.y * quat.y + quat.z * quat.z ) ) );
	return quat;
}

ID_INLINE idVec3 idCompressedJointQuat::ToOffset() const
{
	return idVec3( ShortToOffset( t[0] ), ShortToOffset( t[1] ), ShortToOffset( t[2] ) );
}

/*
===============================================================================

	Joint Matrix

===============================================================================
*/

/*
================================================
idJointMat has the following structure:

	idMat3 m;
	idVec3 t;

	m[0][0], m[1][0], m[2][0], t[0]
	m[0][1], m[1][1], m[2][1], t[1]
	m[0][2], m[1][2], m[2][2], t[2]

================================================
*/
class idJointMat {
public:

	void			SetRotation( const idMat3& m );
	void			GetRotation( idMat3 & ) const;
	idMat3			GetRotation() const;

	void			SetTranslation( const idVec3& t );
	void			GetTranslation( idVec3 & ) const;
	idVec3			GetTranslation() const;

	idVec3			operator*( const idVec3& v ) const;							// only rotate
	idVec3			operator*( const idVec4& v ) const;							// rotate and translate
	void			Mul( idVec3 &out, const idVec3 &v ) const;					// rotate and translate

	idJointMat& 	operator*=( const idJointMat& a );							// transform
	idJointMat& 	operator/=( const idJointMat& a );							// untransform

	bool			Compare( const idJointMat& a ) const;						// exact compare, no epsilon
	bool			Compare( const idJointMat& a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const idJointMat& a ) const;					// exact compare, no epsilon
	bool			operator!=(	const idJointMat& a ) const;					// exact compare, no epsilon

	void			Identity();
	void			Invert();

	void			FromMat4( const idMat4& m );

	idMat3			ToMat3() const;
	idMat4			ToMat4() const;
	idVec3			ToVec3() const;
	const float* 	ToFloatPtr() const { return mat; }
	float* 			ToFloatPtr() { return mat; }
	void			ToJointQuat( idJointQuat& ) const;
	ID_INLINE idJointQuat ToJointQuat() const { idJointQuat jc; ToJointQuat( jc ); return jc; }
	void			ToDualQuat( float dq[ 2 ][ 4 ] ) const;

	void			Transform( idVec3& result, const idVec3& v ) const;
	void			Rotate( idVec3& result, const idVec3& v ) const;

	static void		Mul( idJointMat& result, const idJointMat& mat, const float s );
	static void		Mad( idJointMat& result, const idJointMat& mat, const float s );
	static void		Multiply( idJointMat& result, const idJointMat& m1, const idJointMat& m2 );
	static void		InverseMultiply( idJointMat& result, const idJointMat& m1, const idJointMat& m2 );

	ALIGN16( float	mat[3 * 4] );
};

// offsets for SIMD code
#define JOINTMAT_SIZE				( 4 * 3 * 4)		// sizeof( idJointMat )
assert_sizeof( idJointMat,			JOINTMAT_SIZE );

#define JOINTMAT_TYPESIZE			( 4 * 3 )

/*
========================
idJointMat::SetRotation
========================
*/
ID_INLINE void idJointMat::SetRotation( const idMat3& m )
{
	// NOTE: idMat3 is transposed because it is column-major
	mat[0 * 4 + 0] = m[0][0];
	mat[0 * 4 + 1] = m[1][0];
	mat[0 * 4 + 2] = m[2][0];
	mat[1 * 4 + 0] = m[0][1];
	mat[1 * 4 + 1] = m[1][1];
	mat[1 * 4 + 2] = m[2][1];
	mat[2 * 4 + 0] = m[0][2];
	mat[2 * 4 + 1] = m[1][2];
	mat[2 * 4 + 2] = m[2][2];
}
/*
========================
idJointMat::GetRotation
========================
*/
ID_INLINE void idJointMat::GetRotation( idMat3 & m ) const
{
	m[0][0] = mat[0 * 4 + 0];
	m[1][0] = mat[0 * 4 + 1];
	m[2][0] = mat[0 * 4 + 2];
	m[0][1] = mat[1 * 4 + 0];
	m[1][1] = mat[1 * 4 + 1];
	m[2][1] = mat[1 * 4 + 2];
	m[0][2] = mat[2 * 4 + 0];
	m[1][2] = mat[2 * 4 + 1];
	m[2][2] = mat[2 * 4 + 2];
}
ID_INLINE idMat3 idJointMat::GetRotation() const
{
	idMat3 r;
	GetRotation( r );
	return r;
}

/*
========================
idJointMat::SetTranslation
========================
*/
ID_INLINE void idJointMat::SetTranslation( const idVec3& t )
{
	mat[0 * 4 + 3] = t[0];
	mat[1 * 4 + 3] = t[1];
	mat[2 * 4 + 3] = t[2];
}
/*
========================
idJointMat::GetTranslation
========================
*/
ID_INLINE void idJointMat::GetTranslation( idVec3 & t ) const
{
	t[0] = mat[0 * 4 + 3];
	t[1] = mat[1 * 4 + 3];
	t[2] = mat[2 * 4 + 3];
}
ID_INLINE idVec3 idJointMat::GetTranslation() const
{
	idVec3 t;
	GetTranslation( t );
	return t;
}

/*
========================
idJointMat::operator*
========================
*/
ID_INLINE idVec3 idJointMat::operator*( const idVec3& v ) const
{
	return idVec3(	mat[0 * 4 + 0] * v[0] + mat[0 * 4 + 1] * v[1] + mat[0 * 4 + 2] * v[2],
					mat[1 * 4 + 0] * v[0] + mat[1 * 4 + 1] * v[1] + mat[1 * 4 + 2] * v[2],
					mat[2 * 4 + 0] * v[0] + mat[2 * 4 + 1] * v[1] + mat[2 * 4 + 2] * v[2] );
}
ID_INLINE idVec3 idJointMat::operator*( const idVec4& v ) const
{
	return idVec3(	mat[0 * 4 + 0] * v[0] + mat[0 * 4 + 1] * v[1] + mat[0 * 4 + 2] * v[2] + mat[0 * 4 + 3] * v[3],
					mat[1 * 4 + 0] * v[0] + mat[1 * 4 + 1] * v[1] + mat[1 * 4 + 2] * v[2] + mat[1 * 4 + 3] * v[3],
					mat[2 * 4 + 0] * v[0] + mat[2 * 4 + 1] * v[1] + mat[2 * 4 + 2] * v[2] + mat[2 * 4 + 3] * v[3] );
}
ID_INLINE void	idJointMat::Mul( idVec3 &out, const idVec3 &v ) const
{
	out.x = mat[ 0 * 4 + 0 ] * v[ 0 ] + mat[ 0 * 4 + 1 ] * v[ 1 ] + mat[ 0 * 4 + 2 ] * v[ 2 ] + mat[ 0 * 4 + 3 ];
	out.y = mat[ 1 * 4 + 0 ] * v[ 0 ] + mat[ 1 * 4 + 1 ] * v[ 1 ] + mat[ 1 * 4 + 2 ] * v[ 2 ] + mat[ 1 * 4 + 3 ];
	out.z = mat[ 2 * 4 + 0 ] * v[ 0 ] + mat[ 2 * 4 + 1 ] * v[ 1 ] + mat[ 2 * 4 + 2 ] * v[ 2 ] + mat[ 2 * 4 + 3 ];
}

/*
========================
idJointMat::operator*=
========================
*/
ID_INLINE idJointMat& idJointMat::operator*=( const idJointMat& a )
{
	float tmp[3];

	tmp[0] = mat[0 * 4 + 0] * a.mat[0 * 4 + 0] + mat[1 * 4 + 0] * a.mat[0 * 4 + 1] + mat[2 * 4 + 0] * a.mat[0 * 4 + 2];
	tmp[1] = mat[0 * 4 + 0] * a.mat[1 * 4 + 0] + mat[1 * 4 + 0] * a.mat[1 * 4 + 1] + mat[2 * 4 + 0] * a.mat[1 * 4 + 2];
	tmp[2] = mat[0 * 4 + 0] * a.mat[2 * 4 + 0] + mat[1 * 4 + 0] * a.mat[2 * 4 + 1] + mat[2 * 4 + 0] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 0] = tmp[0];
	mat[1 * 4 + 0] = tmp[1];
	mat[2 * 4 + 0] = tmp[2];

	tmp[0] = mat[0 * 4 + 1] * a.mat[0 * 4 + 0] + mat[1 * 4 + 1] * a.mat[0 * 4 + 1] + mat[2 * 4 + 1] * a.mat[0 * 4 + 2];
	tmp[1] = mat[0 * 4 + 1] * a.mat[1 * 4 + 0] + mat[1 * 4 + 1] * a.mat[1 * 4 + 1] + mat[2 * 4 + 1] * a.mat[1 * 4 + 2];
	tmp[2] = mat[0 * 4 + 1] * a.mat[2 * 4 + 0] + mat[1 * 4 + 1] * a.mat[2 * 4 + 1] + mat[2 * 4 + 1] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 1] = tmp[0];
	mat[1 * 4 + 1] = tmp[1];
	mat[2 * 4 + 1] = tmp[2];

	tmp[0] = mat[0 * 4 + 2] * a.mat[0 * 4 + 0] + mat[1 * 4 + 2] * a.mat[0 * 4 + 1] + mat[2 * 4 + 2] * a.mat[0 * 4 + 2];
	tmp[1] = mat[0 * 4 + 2] * a.mat[1 * 4 + 0] + mat[1 * 4 + 2] * a.mat[1 * 4 + 1] + mat[2 * 4 + 2] * a.mat[1 * 4 + 2];
	tmp[2] = mat[0 * 4 + 2] * a.mat[2 * 4 + 0] + mat[1 * 4 + 2] * a.mat[2 * 4 + 1] + mat[2 * 4 + 2] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 2] = tmp[0];
	mat[1 * 4 + 2] = tmp[1];
	mat[2 * 4 + 2] = tmp[2];

	tmp[0] = mat[0 * 4 + 3] * a.mat[0 * 4 + 0] + mat[1 * 4 + 3] * a.mat[0 * 4 + 1] + mat[2 * 4 + 3] * a.mat[0 * 4 + 2];
	tmp[1] = mat[0 * 4 + 3] * a.mat[1 * 4 + 0] + mat[1 * 4 + 3] * a.mat[1 * 4 + 1] + mat[2 * 4 + 3] * a.mat[1 * 4 + 2];
	tmp[2] = mat[0 * 4 + 3] * a.mat[2 * 4 + 0] + mat[1 * 4 + 3] * a.mat[2 * 4 + 1] + mat[2 * 4 + 3] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 3] = tmp[0];
	mat[1 * 4 + 3] = tmp[1];
	mat[2 * 4 + 3] = tmp[2];

	mat[0 * 4 + 3] += a.mat[0 * 4 + 3];
	mat[1 * 4 + 3] += a.mat[1 * 4 + 3];
	mat[2 * 4 + 3] += a.mat[2 * 4 + 3];

	return *this;
}

/*
========================
idJointMat::operator/=
========================
*/
ID_INLINE idJointMat& idJointMat::operator/=( const idJointMat& a )
{
	float tmp[3];

	mat[0 * 4 + 3] -= a.mat[0 * 4 + 3];
	mat[1 * 4 + 3] -= a.mat[1 * 4 + 3];
	mat[2 * 4 + 3] -= a.mat[2 * 4 + 3];

	tmp[0] = mat[0 * 4 + 0] * a.mat[0 * 4 + 0] + mat[1 * 4 + 0] * a.mat[1 * 4 + 0] + mat[2 * 4 + 0] * a.mat[2 * 4 + 0];
	tmp[1] = mat[0 * 4 + 0] * a.mat[0 * 4 + 1] + mat[1 * 4 + 0] * a.mat[1 * 4 + 1] + mat[2 * 4 + 0] * a.mat[2 * 4 + 1];
	tmp[2] = mat[0 * 4 + 0] * a.mat[0 * 4 + 2] + mat[1 * 4 + 0] * a.mat[1 * 4 + 2] + mat[2 * 4 + 0] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 0] = tmp[0];
	mat[1 * 4 + 0] = tmp[1];
	mat[2 * 4 + 0] = tmp[2];

	tmp[0] = mat[0 * 4 + 1] * a.mat[0 * 4 + 0] + mat[1 * 4 + 1] * a.mat[1 * 4 + 0] + mat[2 * 4 + 1] * a.mat[2 * 4 + 0];
	tmp[1] = mat[0 * 4 + 1] * a.mat[0 * 4 + 1] + mat[1 * 4 + 1] * a.mat[1 * 4 + 1] + mat[2 * 4 + 1] * a.mat[2 * 4 + 1];
	tmp[2] = mat[0 * 4 + 1] * a.mat[0 * 4 + 2] + mat[1 * 4 + 1] * a.mat[1 * 4 + 2] + mat[2 * 4 + 1] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 1] = tmp[0];
	mat[1 * 4 + 1] = tmp[1];
	mat[2 * 4 + 1] = tmp[2];

	tmp[0] = mat[0 * 4 + 2] * a.mat[0 * 4 + 0] + mat[1 * 4 + 2] * a.mat[1 * 4 + 0] + mat[2 * 4 + 2] * a.mat[2 * 4 + 0];
	tmp[1] = mat[0 * 4 + 2] * a.mat[0 * 4 + 1] + mat[1 * 4 + 2] * a.mat[1 * 4 + 1] + mat[2 * 4 + 2] * a.mat[2 * 4 + 1];
	tmp[2] = mat[0 * 4 + 2] * a.mat[0 * 4 + 2] + mat[1 * 4 + 2] * a.mat[1 * 4 + 2] + mat[2 * 4 + 2] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 2] = tmp[0];
	mat[1 * 4 + 2] = tmp[1];
	mat[2 * 4 + 2] = tmp[2];

	tmp[0] = mat[0 * 4 + 3] * a.mat[0 * 4 + 0] + mat[1 * 4 + 3] * a.mat[1 * 4 + 0] + mat[2 * 4 + 3] * a.mat[2 * 4 + 0];
	tmp[1] = mat[0 * 4 + 3] * a.mat[0 * 4 + 1] + mat[1 * 4 + 3] * a.mat[1 * 4 + 1] + mat[2 * 4 + 3] * a.mat[2 * 4 + 1];
	tmp[2] = mat[0 * 4 + 3] * a.mat[0 * 4 + 2] + mat[1 * 4 + 3] * a.mat[1 * 4 + 2] + mat[2 * 4 + 3] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 3] = tmp[0];
	mat[1 * 4 + 3] = tmp[1];
	mat[2 * 4 + 3] = tmp[2];

	return *this;
}

/*
========================
idJointMat::Compare
========================
*/
ID_INLINE bool idJointMat::Compare( const idJointMat& a ) const
{
	for( int i = 0; i < 12; i++ )
	{
		if( mat[i] != a.mat[i] ) {
			return false;
		}
	}
	return true;
}

/*
========================
idJointMat::Compare
========================
*/
ID_INLINE bool idJointMat::Compare( const idJointMat& a, const float epsilon ) const
{
	for( int i = 0; i < 12; i++ )
	{
		if( idMath::Fabs( mat[i] - a.mat[i] ) > epsilon ) {
			return false;
		}
	}
	return true;
}

/*
========================
idJointMat::operator==
========================
*/
ID_INLINE bool idJointMat::operator==( const idJointMat& a ) const
{
	return Compare( a );
}

/*
========================
idJointMat::operator!=
========================
*/
ID_INLINE bool idJointMat::operator!=( const idJointMat& a ) const
{
	return !Compare( a );
}

/*
========================
idJointMat::Identity
========================
*/
ID_INLINE void idJointMat::Identity()
{
	mat[0 * 4 + 0] = 1.0f; mat[0 * 4 + 1] = 0.0f; mat[0 * 4 + 2] = 0.0f; mat[0 * 4 + 3] = 0.0f;
	mat[1 * 4 + 0] = 0.0f; mat[1 * 4 + 1] = 1.0f; mat[1 * 4 + 2] = 0.0f; mat[1 * 4 + 3] = 0.0f;
	mat[2 * 4 + 0] = 0.0f; mat[2 * 4 + 1] = 0.0f; mat[2 * 4 + 2] = 1.0f; mat[2 * 4 + 3] = 0.0f;
}

/*
========================
idJointMat::Invert
========================
*/
ID_INLINE void idJointMat::Invert()
{
	float tmp[3];

	// negate inverse rotated translation part
	tmp[0] = mat[0 * 4 + 0] * mat[0 * 4 + 3] + mat[1 * 4 + 0] * mat[1 * 4 + 3] + mat[2 * 4 + 0] * mat[2 * 4 + 3];
	tmp[1] = mat[0 * 4 + 1] * mat[0 * 4 + 3] + mat[1 * 4 + 1] * mat[1 * 4 + 3] + mat[2 * 4 + 1] * mat[2 * 4 + 3];
	tmp[2] = mat[0 * 4 + 2] * mat[0 * 4 + 3] + mat[1 * 4 + 2] * mat[1 * 4 + 3] + mat[2 * 4 + 2] * mat[2 * 4 + 3];
	mat[0 * 4 + 3] = -tmp[0];
	mat[1 * 4 + 3] = -tmp[1];
	mat[2 * 4 + 3] = -tmp[2];

	// transpose rotation part
	tmp[0] = mat[0 * 4 + 1];
	mat[0 * 4 + 1] = mat[1 * 4 + 0];
	mat[1 * 4 + 0] = tmp[0];
	tmp[1] = mat[0 * 4 + 2];
	mat[0 * 4 + 2] = mat[2 * 4 + 0];
	mat[2 * 4 + 0] = tmp[1];
	tmp[2] = mat[1 * 4 + 2];
	mat[1 * 4 + 2] = mat[2 * 4 + 1];
	mat[2 * 4 + 1] = tmp[2];
}

/*
========================
idJointMat::ToMat3
========================
*/
ID_INLINE idMat3 idJointMat::ToMat3() const
{
	return idMat3(	mat[0 * 4 + 0], mat[1 * 4 + 0], mat[2 * 4 + 0],
					mat[0 * 4 + 1], mat[1 * 4 + 1], mat[2 * 4 + 1],
					mat[0 * 4 + 2], mat[1 * 4 + 2], mat[2 * 4 + 2] );
}

/*
========================
idJointMat::ToMat4
========================
*/
ID_INLINE idMat4 idJointMat::ToMat4() const
{
	return idMat4( mat[0 * 4 + 0], mat[0 * 4 + 1], mat[0 * 4 + 2], mat[0 * 4 + 3],
				   mat[1 * 4 + 0], mat[1 * 4 + 1], mat[1 * 4 + 2], mat[1 * 4 + 3],
			       mat[2 * 4 + 0], mat[2 * 4 + 1], mat[2 * 4 + 2], mat[2 * 4 + 3],
			       0.0f, 0.0f, 0.0f, 1.0f );
}

/*
========================
idJointMat::FromMat4
========================
*/
ID_INLINE void idJointMat::FromMat4( const idMat4& m )
{
	mat[0 * 4 + 0] = m[0][0], mat[0 * 4 + 1] = m[0][1], mat[0 * 4 + 2] = m[0][2], mat[0 * 4 + 3] = m[0][3];
	mat[1 * 4 + 0] = m[1][0], mat[1 * 4 + 1] = m[1][1], mat[1 * 4 + 2] = m[1][2], mat[1 * 4 + 3] = m[1][3];
	mat[2 * 4 + 0] = m[2][0], mat[2 * 4 + 1] = m[2][1], mat[2 * 4 + 2] = m[2][2], mat[2 * 4 + 3] = m[2][3];
	assert( m[3][0] == 0.0f );
	assert( m[3][1] == 0.0f );
	assert( m[3][2] == 0.0f );
	assert( m[3][3] == 1.0f );
}

/*
========================
idJointMat::ToVec3
========================
*/
ID_INLINE idVec3 idJointMat::ToVec3() const
{
	return idVec3( mat[0 * 4 + 3], mat[1 * 4 + 3], mat[2 * 4 + 3] );
}

/*
========================
idJointMat::Transform
========================
*/
ID_INLINE void idJointMat::Transform( idVec3& result, const idVec3& v ) const
{
	result.x = mat[0 * 4 + 0] * v.x + mat[0 * 4 + 1] * v.y + mat[0 * 4 + 2] * v.z + mat[0 * 4 + 3];
	result.y = mat[1 * 4 + 0] * v.x + mat[1 * 4 + 1] * v.y + mat[1 * 4 + 2] * v.z + mat[1 * 4 + 3];
	result.z = mat[2 * 4 + 0] * v.x + mat[2 * 4 + 1] * v.y + mat[2 * 4 + 2] * v.z + mat[2 * 4 + 3];
}

/*
========================
idJointMat::Rotate
========================
*/
ID_INLINE void idJointMat::Rotate( idVec3& result, const idVec3& v ) const
{
	result.x = mat[0 * 4 + 0] * v.x + mat[0 * 4 + 1] * v.y + mat[0 * 4 + 2] * v.z;
	result.y = mat[1 * 4 + 0] * v.x + mat[1 * 4 + 1] * v.y + mat[1 * 4 + 2] * v.z;
	result.z = mat[2 * 4 + 0] * v.x + mat[2 * 4 + 1] * v.y + mat[2 * 4 + 2] * v.z;
}

/*
========================
idJointMat::Mul
========================
*/
ID_INLINE void idJointMat::Mul( idJointMat& result, const idJointMat& mat, const float s )
{
	result.mat[0 * 4 + 0] = s * mat.mat[0 * 4 + 0];
	result.mat[0 * 4 + 1] = s * mat.mat[0 * 4 + 1];
	result.mat[0 * 4 + 2] = s * mat.mat[0 * 4 + 2];
	result.mat[0 * 4 + 3] = s * mat.mat[0 * 4 + 3];
	result.mat[1 * 4 + 0] = s * mat.mat[1 * 4 + 0];
	result.mat[1 * 4 + 1] = s * mat.mat[1 * 4 + 1];
	result.mat[1 * 4 + 2] = s * mat.mat[1 * 4 + 2];
	result.mat[1 * 4 + 3] = s * mat.mat[1 * 4 + 3];
	result.mat[2 * 4 + 0] = s * mat.mat[2 * 4 + 0];
	result.mat[2 * 4 + 1] = s * mat.mat[2 * 4 + 1];
	result.mat[2 * 4 + 2] = s * mat.mat[2 * 4 + 2];
	result.mat[2 * 4 + 3] = s * mat.mat[2 * 4 + 3];
}

/*
========================
idJointMat::Mad
========================
*/
ID_INLINE void idJointMat::Mad( idJointMat& result, const idJointMat& mat, const float s )
{
	result.mat[0 * 4 + 0] += s * mat.mat[0 * 4 + 0];
	result.mat[0 * 4 + 1] += s * mat.mat[0 * 4 + 1];
	result.mat[0 * 4 + 2] += s * mat.mat[0 * 4 + 2];
	result.mat[0 * 4 + 3] += s * mat.mat[0 * 4 + 3];
	result.mat[1 * 4 + 0] += s * mat.mat[1 * 4 + 0];
	result.mat[1 * 4 + 1] += s * mat.mat[1 * 4 + 1];
	result.mat[1 * 4 + 2] += s * mat.mat[1 * 4 + 2];
	result.mat[1 * 4 + 3] += s * mat.mat[1 * 4 + 3];
	result.mat[2 * 4 + 0] += s * mat.mat[2 * 4 + 0];
	result.mat[2 * 4 + 1] += s * mat.mat[2 * 4 + 1];
	result.mat[2 * 4 + 2] += s * mat.mat[2 * 4 + 2];
	result.mat[2 * 4 + 3] += s * mat.mat[2 * 4 + 3];
}

/*
========================
idJointMat::Multiply
========================
*/
ID_INLINE void idJointMat::Multiply( idJointMat& result, const idJointMat& m1, const idJointMat& m2 )
{
	result.mat[0 * 4 + 0] = m1.mat[0 * 4 + 0] * m2.mat[0 * 4 + 0] + m1.mat[0 * 4 + 1] * m2.mat[1 * 4 + 0] + m1.mat[0 * 4 + 2] * m2.mat[2 * 4 + 0];
	result.mat[0 * 4 + 1] = m1.mat[0 * 4 + 0] * m2.mat[0 * 4 + 1] + m1.mat[0 * 4 + 1] * m2.mat[1 * 4 + 1] + m1.mat[0 * 4 + 2] * m2.mat[2 * 4 + 1];
	result.mat[0 * 4 + 2] = m1.mat[0 * 4 + 0] * m2.mat[0 * 4 + 2] + m1.mat[0 * 4 + 1] * m2.mat[1 * 4 + 2] + m1.mat[0 * 4 + 2] * m2.mat[2 * 4 + 2];
	result.mat[0 * 4 + 3] = m1.mat[0 * 4 + 0] * m2.mat[0 * 4 + 3] + m1.mat[0 * 4 + 1] * m2.mat[1 * 4 + 3] + m1.mat[0 * 4 + 2] * m2.mat[2 * 4 + 3] + m1.mat[0 * 4 + 3];

	result.mat[1 * 4 + 0] = m1.mat[1 * 4 + 0] * m2.mat[0 * 4 + 0] + m1.mat[1 * 4 + 1] * m2.mat[1 * 4 + 0] + m1.mat[1 * 4 + 2] * m2.mat[2 * 4 + 0];
	result.mat[1 * 4 + 1] = m1.mat[1 * 4 + 0] * m2.mat[0 * 4 + 1] + m1.mat[1 * 4 + 1] * m2.mat[1 * 4 + 1] + m1.mat[1 * 4 + 2] * m2.mat[2 * 4 + 1];
	result.mat[1 * 4 + 2] = m1.mat[1 * 4 + 0] * m2.mat[0 * 4 + 2] + m1.mat[1 * 4 + 1] * m2.mat[1 * 4 + 2] + m1.mat[1 * 4 + 2] * m2.mat[2 * 4 + 2];
	result.mat[1 * 4 + 3] = m1.mat[1 * 4 + 0] * m2.mat[0 * 4 + 3] + m1.mat[1 * 4 + 1] * m2.mat[1 * 4 + 3] + m1.mat[1 * 4 + 2] * m2.mat[2 * 4 + 3] + m1.mat[1 * 4 + 3];

	result.mat[2 * 4 + 0] = m1.mat[2 * 4 + 0] * m2.mat[0 * 4 + 0] + m1.mat[2 * 4 + 1] * m2.mat[1 * 4 + 0] + m1.mat[2 * 4 + 2] * m2.mat[2 * 4 + 0];
	result.mat[2 * 4 + 1] = m1.mat[2 * 4 + 0] * m2.mat[0 * 4 + 1] + m1.mat[2 * 4 + 1] * m2.mat[1 * 4 + 1] + m1.mat[2 * 4 + 2] * m2.mat[2 * 4 + 1];
	result.mat[2 * 4 + 2] = m1.mat[2 * 4 + 0] * m2.mat[0 * 4 + 2] + m1.mat[2 * 4 + 1] * m2.mat[1 * 4 + 2] + m1.mat[2 * 4 + 2] * m2.mat[2 * 4 + 2];
	result.mat[2 * 4 + 3] = m1.mat[2 * 4 + 0] * m2.mat[0 * 4 + 3] + m1.mat[2 * 4 + 1] * m2.mat[1 * 4 + 3] + m1.mat[2 * 4 + 2] * m2.mat[2 * 4 + 3] + m1.mat[2 * 4 + 3];
}

/*
========================
idJointMat::InverseMultiply
========================
*/
ID_INLINE void idJointMat::InverseMultiply( idJointMat& result, const idJointMat& m1, const idJointMat& m2 )
{
	float dst[3];

	result.mat[0 * 4 + 0] = m1.mat[0 * 4 + 0] * m2.mat[0 * 4 + 0] + m1.mat[0 * 4 + 1] * m2.mat[0 * 4 + 1] + m1.mat[0 * 4 + 2] * m2.mat[0 * 4 + 2];
	result.mat[0 * 4 + 1] = m1.mat[0 * 4 + 0] * m2.mat[1 * 4 + 0] + m1.mat[0 * 4 + 1] * m2.mat[1 * 4 + 1] + m1.mat[0 * 4 + 2] * m2.mat[1 * 4 + 2];
	result.mat[0 * 4 + 2] = m1.mat[0 * 4 + 0] * m2.mat[2 * 4 + 0] + m1.mat[0 * 4 + 1] * m2.mat[2 * 4 + 1] + m1.mat[0 * 4 + 2] * m2.mat[2 * 4 + 2];

	result.mat[1 * 4 + 0] = m1.mat[1 * 4 + 0] * m2.mat[0 * 4 + 0] + m1.mat[1 * 4 + 1] * m2.mat[0 * 4 + 1] + m1.mat[1 * 4 + 2] * m2.mat[0 * 4 + 2];
	result.mat[1 * 4 + 1] = m1.mat[1 * 4 + 0] * m2.mat[1 * 4 + 0] + m1.mat[1 * 4 + 1] * m2.mat[1 * 4 + 1] + m1.mat[1 * 4 + 2] * m2.mat[1 * 4 + 2];
	result.mat[1 * 4 + 2] = m1.mat[1 * 4 + 0] * m2.mat[2 * 4 + 0] + m1.mat[1 * 4 + 1] * m2.mat[2 * 4 + 1] + m1.mat[1 * 4 + 2] * m2.mat[2 * 4 + 2];

	result.mat[2 * 4 + 0] = m1.mat[2 * 4 + 0] * m2.mat[0 * 4 + 0] + m1.mat[2 * 4 + 1] * m2.mat[0 * 4 + 1] + m1.mat[2 * 4 + 2] * m2.mat[0 * 4 + 2];
	result.mat[2 * 4 + 1] = m1.mat[2 * 4 + 0] * m2.mat[1 * 4 + 0] + m1.mat[2 * 4 + 1] * m2.mat[1 * 4 + 1] + m1.mat[2 * 4 + 2] * m2.mat[1 * 4 + 2];
	result.mat[2 * 4 + 2] = m1.mat[2 * 4 + 0] * m2.mat[2 * 4 + 0] + m1.mat[2 * 4 + 1] * m2.mat[2 * 4 + 1] + m1.mat[2 * 4 + 2] * m2.mat[2 * 4 + 2];

	dst[0] = - ( m2.mat[0 * 4 + 0] * m2.mat[0 * 4 + 3] + m2.mat[1 * 4 + 0] * m2.mat[1 * 4 + 3] + m2.mat[2 * 4 + 0] * m2.mat[2 * 4 + 3] );
	dst[1] = - ( m2.mat[0 * 4 + 1] * m2.mat[0 * 4 + 3] + m2.mat[1 * 4 + 1] * m2.mat[1 * 4 + 3] + m2.mat[2 * 4 + 1] * m2.mat[2 * 4 + 3] );
	dst[2] = - ( m2.mat[0 * 4 + 2] * m2.mat[0 * 4 + 3] + m2.mat[1 * 4 + 2] * m2.mat[1 * 4 + 3] + m2.mat[2 * 4 + 2] * m2.mat[2 * 4 + 3] );

	result.mat[0 * 4 + 3] = m1.mat[0 * 4 + 0] * dst[0] + m1.mat[0 * 4 + 1] * dst[1] + m1.mat[0 * 4 + 2] * dst[2] + m1.mat[0 * 4 + 3];
	result.mat[1 * 4 + 3] = m1.mat[1 * 4 + 0] * dst[0] + m1.mat[1 * 4 + 1] * dst[1] + m1.mat[1 * 4 + 2] * dst[2] + m1.mat[1 * 4 + 3];
	result.mat[2 * 4 + 3] = m1.mat[2 * 4 + 0] * dst[0] + m1.mat[2 * 4 + 1] * dst[1] + m1.mat[2 * 4 + 2] * dst[2] + m1.mat[2 * 4 + 3];
}

/*
===============================================================================

  Joint Frame

===============================================================================
*/

struct jointFrame_t
{
	idCQuat				q;
	idVec3				t;
};

#endif /* !__JOINTTRANSFORM_H__ */
