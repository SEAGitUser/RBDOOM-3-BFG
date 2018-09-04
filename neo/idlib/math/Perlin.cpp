// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

idPerlin::idPerlin( float _persistence, int _octaves, float _frequency )
{
	persistence = _persistence;
	octaves = _octaves;
	frequency = _frequency;

	// "no" tiling
	tilex = 0x0FFFFFFF;
	tiley = 0x0FFFFFFF;
	tilez = 0x0FFFFFFF;
}

void idPerlin::SetTileX( int tile )
{
	tilex = ( int ) ( tile * frequency );
}

void idPerlin::SetTileY( int tile )
{
	tiley = ( int ) ( tile * frequency );
}

void idPerlin::SetTileZ( int tile )
{
	tilez = ( int ) ( tile * frequency );
}

float idPerlin::RawNoise( int x, int y, int z )
{
	int n = x + y * 57 + z * 227;
	n = ( n << 13 ) ^ n;
	//if (alternative)
	//	return (1 - ( ( n * ( n * n * 19417 + 189851) + 4967243) & 4945007) / 3354521.0);
	//else
	return   1 - ( ( n *  ( n * n * 15731 + 789221 ) + 1376312589 ) & 0x7fffffff ) / 1073741824.0;
}

/*
float idPerlin::SmoothedNoise (int x, int y) {
	float corners = ( Noise(x-1, y-1)+Noise(x+1, y-1)+Noise(x-1, y+1)+Noise(x+1, y+1) ) / 16;
	float sides   = ( Noise(x-1, y)  +Noise(x+1, y)  +Noise(x, y-1)  +Noise(x, y+1) ) /  8;
	float center  =  Noise(x, y) / 4;
	return corners + sides + center;
}
*/

/**
	Interpolated 3d noize
*/
float idPerlin::InterpolatedNoise( float x, float y, float z )
{
	int ix = static_cast< int >( x );
	int iy = static_cast< int >( y );
	int iz = static_cast< int >( z );

	float fx = x - ix;
	float fy = y - iy;
	float fz = z - iz;

	float v1, v2, v3, v4, v5, v6, v7, v8;
	v1 = RawNoise( ix, iy, iz );
	v2 = RawNoise( ix + 1, iy, iz );
	v3 = RawNoise( ix, iy + 1, iz );
	v4 = RawNoise( ix + 1, iy + 1, iz );

	v5 = RawNoise( ix, iy, iz + 1 );
	v6 = RawNoise( ix + 1, iy, iz + 1 );
	v7 = RawNoise( ix, iy + 1, iz + 1 );
	v8 = RawNoise( ix + 1, iy + 1, iz + 1 );

	float i1, i2, i3, i4;
	i1 = CosineInterp( v1, v2, fx );
	i2 = CosineInterp( v3, v4, fx );

	i3 = CosineInterp( v5, v6, fx );
	i4 = CosineInterp( v7, v8, fx );

	float j1, j2;
	j1 = CosineInterp( i1, i2, fy );
	j2 = CosineInterp( i3, i4, fy );

	return CosineInterp( j1, j2, fz );
}

float idPerlin::NoiseFloat( const idVec3 &pos )
{
	float rez = 0.0f;
	float frequencyl = frequency;
	float amplitude = 1.0f;

	int i;
	for( i = 0; i < octaves; i++ )
	{
		rez += InterpolatedNoise( pos.x * frequencyl, pos.y * frequencyl, pos.z * frequencyl ) * amplitude;
		frequencyl *= 2;
		amplitude *= persistence;
	}
	if( rez > 1.f )
	{
		rez = 1.f;
	}
	else if( rez < -1.f )
	{
		rez = -1.f;
	}
	return rez;
}

// TODO: change these preprocessor macros into inline functions

// S curve is (3x^2 - 2x^3) because it's quick to calculate
// though -cos(x * PI) * 0.5 + 0.5 would work too

ID_INLINE float EaseCurve( float t )
{
	return t * t * ( 3.0 - 2.0 * t );
}

ID_INLINE float LinearInterp( float t, float a, float b )
{
	return a + t * ( b - a );
}

ID_INLINE float Dot2( float rx, float ry, float* q )
{
	return rx * q[ 0 ] + ry * q[ 1 ];
}

ID_INLINE float Dot3( float rx, float ry, float rz, float* q )
{
	return rx * q[ 0 ] + ry * q[ 1 ] + rz * q[ 2 ];
}

#define SetupValues( t, axis, g0, g1, d0, d1, pos ) \
	t = pos[axis] + NOISE_LARGE_PWR2; \
	g0 = ((int)t) & NOISE_MOD_MASK; \
	g1 = (g0 + 1) & NOISE_MOD_MASK; \
	d0 = t - (int)t; \
	d1 = d0 - 1.0;

/////////////////////////////////////////////////////////////////////
// return a random float in [-1,1]

ID_INLINE float idPerlin2::RandNoiseFloat()
{
	return ( float ) ( ( rand() % ( NOISE_WRAP_INDEX + NOISE_WRAP_INDEX ) ) - NOISE_WRAP_INDEX ) / NOISE_WRAP_INDEX;
};

/////////////////////////////////////////////////////////////////////
// convert a 2D vector into unit length

void idPerlin2::Normalize2d( float vector[ 2 ] )
{
	float length = sqrt( ( vector[ 0 ] * vector[ 0 ] ) + ( vector[ 1 ] * vector[ 1 ] ) );
	vector[ 0 ] /= length;
	vector[ 1 ] /= length;
}

/////////////////////////////////////////////////////////////////////
// convert a 3D vector into unit length

void idPerlin2::Normalize3d( float vector[ 3 ] )
{
	float length = sqrt( ( vector[ 0 ] * vector[ 0 ] ) +
		( vector[ 1 ] * vector[ 1 ] ) +
						 ( vector[ 2 ] * vector[ 2 ] ) );
	vector[ 0 ] /= length;
	vector[ 1 ] /= length;
	vector[ 2 ] /= length;
}

/////////////////////////////////////////////////////////////////////
//
// Mnemonics used in the following 3 functions:
//   L = left		(-X direction)
//   R = right  	(+X direction)
//   D = down   	(-Y direction)
//   U = up     	(+Y direction)
//   B = backwards	(-Z direction)
//   F = forwards       (+Z direction)
//
// Not that it matters to the math, but a reader might want to know.
//
// noise1d - create 1-dimensional coherent noise
//   if you want to learn about how noise works, look at this
//   and then look at noise2d.

float idPerlin2::Noise1d( float pos[ 1 ] )
{
	int   gridPointL, gridPointR;
	float distFromL, distFromR, sX, t, u, v;

	if( !initialized )
	{
		Reseed();
	}

	// find out neighboring grid points to pos and signed distances from pos to them.
	SetupValues( t, 0, gridPointL, gridPointR, distFromL, distFromR, pos );

	sX = EaseCurve( distFromL );

	// u, v, are the vectors from the grid pts. times the random gradients for the grid points
	// they are actually dot products, but this looks like scalar multiplication
	u = distFromL * gradientTable1d[ permutationTable[ gridPointL ] ];
	v = distFromR * gradientTable1d[ permutationTable[ gridPointR ] ];

	// return the linear interpretation between u and v (0 = u, 1 = v) at sX.
	return LinearInterp( sX, u, v );
}

/////////////////////////////////////////////////////////////////////
// create 2d coherent noise

float idPerlin2::Noise2d( float pos[ 2 ] )
{
	int gridPointL, gridPointR, gridPointD, gridPointU;
	int indexLD, indexRD, indexLU, indexRU;
	float distFromL, distFromR, distFromD, distFromU;
	float *q, sX, sY, a, b, t, u, v;
	register int indexL, indexR;

	if( !initialized )
	{
		Reseed();
	}

	// find out neighboring grid points to pos and signed distances from pos to them.
	SetupValues( t, 0, gridPointL, gridPointR, distFromL, distFromR, pos );
	SetupValues( t, 1, gridPointD, gridPointU, distFromD, distFromU, pos );

	// Generate some temporary indexes associated with the left and right grid values
	indexL = permutationTable[ gridPointL ];
	indexR = permutationTable[ gridPointR ];

	// Generate indexes in the permutation table for all 4 corners
	indexLD = permutationTable[ indexL + gridPointD ];
	indexRD = permutationTable[ indexR + gridPointD ];
	indexLU = permutationTable[ indexL + gridPointU ];
	indexRU = permutationTable[ indexR + gridPointU ];

	// Get the s curves at the proper values
	sX = EaseCurve( distFromL );
	sY = EaseCurve( distFromD );

	// Do the dot products for the lower left corner and lower right corners.
	// Interpolate between those dot products value sX to get a.
	q = gradientTable2d[ indexLD ]; u = Dot2( distFromL, distFromD, q );
	q = gradientTable2d[ indexRD ]; v = Dot2( distFromR, distFromD, q );
	a = LinearInterp( sX, u, v );

	// Do the dot products for the upper left corner and upper right corners.
	// Interpolate between those dot products at value sX to get b.
	q = gradientTable2d[ indexLU ]; u = Dot2( distFromL, distFromU, q );
	q = gradientTable2d[ indexRU ]; v = Dot2( distFromR, distFromU, q );
	b = LinearInterp( sX, u, v );

	// Interpolate between a and b at value sY to get the noise return value.
	return LinearInterp( sY, a, b );
}

/////////////////////////////////////////////////////////////////////
// you guessed it -- 3D coherent noise

float idPerlin2::Noise3d( float pos[ 2 ] )
{
	int gridPointL, gridPointR, gridPointD, gridPointU, gridPointB, gridPointF;
	int indexLD, indexLU, indexRD, indexRU;
	float distFromL, distFromR, distFromD, distFromU, distFromB, distFromF;
	float *q, sX, sY, sZ, a, b, c, d, t, u, v;
	register int indexL, indexR;

	if( !initialized )
	{
		Reseed();
	}

	// find out neighboring grid points to pos and signed distances from pos to them.
	SetupValues( t, 0, gridPointL, gridPointR, distFromL, distFromR, pos );
	SetupValues( t, 1, gridPointD, gridPointU, distFromD, distFromU, pos );
	SetupValues( t, 2, gridPointB, gridPointF, distFromB, distFromF, pos );

	indexL = permutationTable[ gridPointL ];
	indexR = permutationTable[ gridPointR ];

	indexLD = permutationTable[ indexL + gridPointD ];
	indexRD = permutationTable[ indexR + gridPointD ];
	indexLU = permutationTable[ indexL + gridPointU ];
	indexRU = permutationTable[ indexR + gridPointU ];

	sX = EaseCurve( distFromL );
	sY = EaseCurve( distFromD );
	sZ = EaseCurve( distFromB );

	q = gradientTable3d[ indexLD + gridPointB ]; u = Dot3( distFromL, distFromD, distFromB, q );
	q = gradientTable3d[ indexRD + gridPointB ]; v = Dot3( distFromR, distFromD, distFromB, q );
	a = LinearInterp( sX, u, v );

	q = gradientTable3d[ indexLU + gridPointB ]; u = Dot3( distFromL, distFromU, distFromB, q );
	q = gradientTable3d[ indexRU + gridPointB ]; v = Dot3( distFromR, distFromU, distFromB, q );
	b = LinearInterp( sX, u, v );

	c = LinearInterp( sY, a, b );

	q = gradientTable3d[ indexLD + gridPointF ]; u = Dot3( distFromL, distFromD, distFromF, q );
	q = gradientTable3d[ indexRD + gridPointF ]; v = Dot3( distFromR, distFromD, distFromF, q );
	a = LinearInterp( sX, u, v );

	q = gradientTable3d[ indexLU + gridPointF ]; u = Dot3( distFromL, distFromU, distFromF, q );
	q = gradientTable3d[ indexRU + gridPointF ]; v = Dot3( distFromR, distFromU, distFromF, q );
	b = LinearInterp( sX, u, v );

	d = LinearInterp( sY, a, b );

	return LinearInterp( sZ, c, d );
}

/////////////////////////////////////////////////////////////////////
// you can call noise component-wise, too.

float idPerlin2::Noise( float x )
{
	return Noise1d( &x );
}

float idPerlin2::Noise( float x, float y )
{
	float p[ 2 ] = { x, y };
	return Noise2d( p );
}

float idPerlin2::Noise( float x, float y, float z )
{
	float p[ 3 ] = { x, y, z };
	return Noise3d( p );
}

/////////////////////////////////////////////////////////////////////
// reinitialize with new, random values.

void idPerlin2::Reseed()
{
	srand( ( unsigned int ) ( time( NULL ) + rand() ) );
	GenerateLookupTables();
}

/////////////////////////////////////////////////////////////////////
// reinitialize using a user-specified random seed.

void idPerlin2::Reseed( unsigned int rSeed )
{
	srand( rSeed );
	GenerateLookupTables();
}

/////////////////////////////////////////////////////////////////////
// initialize everything during constructor or reseed -- note
// that space was already allocated for the gradientTable
// during the constructor

void idPerlin2::GenerateLookupTables()
{
	unsigned i, j, temp;

	for( i = 0; i < NOISE_WRAP_INDEX; i++ )
	{
		// put index into permutationTable[index], we will shuffle later
		permutationTable[ i ] = i;

		gradientTable1d[ i ] = RandNoiseFloat();

		for( j = 0; j < 2; j++ ) { gradientTable2d[ i ][ j ] = RandNoiseFloat(); }
		Normalize2d( gradientTable2d[ i ] );

		for( j = 0; j < 3; j++ ) { gradientTable3d[ i ][ j ] = RandNoiseFloat(); }
		Normalize3d( gradientTable3d[ i ] );
	}

	// Shuffle permutation table up to NOISE_WRAP_INDEX
	for( i = 0; i < NOISE_WRAP_INDEX; i++ )
	{
		j = rand() & NOISE_MOD_MASK;
		temp = permutationTable[ i ];
		permutationTable[ i ] = permutationTable[ j ];
		permutationTable[ j ] = temp;
	}

	// Add the rest of the table entries in, duplicating
	// indices and entries so that they can effectively be indexed
	// by unsigned chars.  I think.  Ask Perlin what this is really doing.
	//
	// This is the only part of the algorithm that I don't understand 100%.

	for( i = 0; i < NOISE_WRAP_INDEX + 2; i++ )
	{
		permutationTable[ NOISE_WRAP_INDEX + i ] = permutationTable[ i ];

		gradientTable1d[ NOISE_WRAP_INDEX + i ] = gradientTable1d[ i ];

		for( j = 0; j < 2; j++ )
		{
			gradientTable2d[ NOISE_WRAP_INDEX + i ][ j ] = gradientTable2d[ i ][ j ];
		}

		for( j = 0; j < 3; j++ )
		{
			gradientTable3d[ NOISE_WRAP_INDEX + i ][ j ] = gradientTable3d[ i ][ j ];
		}
	}

	// And we're done. Set initialized to true
	initialized = 1;
}

////////////////////////////////////

int idPerlin3::p[ 512 ] = {
	151,160,137,91,90,15,
	131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
	190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
	88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
	77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
	102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
	135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
	5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
	223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
	129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
	251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
	49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
	138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,

	// and again
	151,160,137,91,90,15,
	131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
	190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
	88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
	77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
	102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
	135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
	5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
	223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
	129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
	251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
	49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
	138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

float idPerlin3::Noise( float x )
{
	return 0.f;
}

float idPerlin3::Noise( float x, float y )
{
	return 0.f;
}

float idPerlin3::Noise( float x, float y, float z )
{
	// Find unit cube that contains point
	int ucX = static_cast< int >( idMath::Floor( x ) ) & 255;
	int ucY = static_cast< int >( idMath::Floor( y ) ) & 255;
	int ucZ = static_cast< int >( idMath::Floor( z ) ) & 255;

	// Find relative x, y, z of point in cube
	x -= idMath::Floor( x );
	y -= idMath::Floor( y );
	z -= idMath::Floor( z );

	// Compute fade curves for each of x, y, z
	float u = Fade( x );
	float v = Fade( y );
	float w = Fade( z );

	// Hash coordinates of the 8 cube corners;
	int a = p[ ucX ] + ucY;
	int aa = p[ a ] + ucZ;
	int ab = p[ a + 1 ] + ucZ;
	int b = p[ ucX + 1 ] + ucY;
	int ba = p[ b ] + ucZ;
	int bb = p[ b + 1 ] + ucZ;

	// And add blended results from 8 corners of cube
	return( Lerp( w, Lerp( v, Lerp( u, Grad( p[ aa ], x, y, z ), Grad( p[ ba ], x - 1, y, z ) ),
			Lerp( u, Grad( p[ ab ], x, y - 1, z ), Grad( p[ bb ], x - 1, y - 1, z ) ) ),
			Lerp( v, Lerp( u, Grad( p[ aa + 1 ], x, y, z - 1 ), Grad( p[ ba + 1 ], x - 1, y, z - 1 ) ),
			Lerp( u, Grad( p[ ab + 1 ], x, y - 1, z - 1 ), Grad( p[ bb + 1 ], x - 1, y - 1, z - 1 ) ) ) ) );
}