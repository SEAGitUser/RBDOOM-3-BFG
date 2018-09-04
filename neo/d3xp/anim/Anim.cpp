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

#pragma hdrstop
#include "precompiled.h"

#include "../Game_local.h"

idCVar binaryLoadAnim( "binaryLoadAnim", "1", 0, "enable binary load/write of idMD5Anim" );

static const byte B_ANIM_MD5_VERSION = 101;
static const unsigned int B_ANIM_MD5_MAGIC = ( 'B' << 24 ) | ( 'M' << 16 ) | ( 'D' << 8 ) | B_ANIM_MD5_VERSION;

static const int JOINT_FRAME_PAD = 1;	// one extra to be able to read one more float than is necessary

bool idAnimManager::forceExport = false;

/***********************************************************************

	idMD5Anim

***********************************************************************/

/*
====================
idMD5Anim::idMD5Anim
====================
*/
idMD5Anim::idMD5Anim()
{
	ref_count = 0;
	numFrames = 0;
	numJoints = 0;
	frameRate = 24;
	animLength = 0;
	numAnimatedComponents = 0;
	totaldelta.Zero();
}

/*
====================
idMD5Anim::idMD5Anim
====================
*/
idMD5Anim::~idMD5Anim()
{
	Free();
}

/*
====================
idMD5Anim::Free
====================
*/
void idMD5Anim::Free()
{
	numFrames = 0;
	numJoints = 0;
	frameRate = 24;
	animLength = 0;
	numAnimatedComponents = 0;
	//name		= "";

	totaldelta.Zero();

	jointInfo.Clear();
	bounds.Clear();
	componentFrames.Clear();
}

/*
====================
idMD5Anim::Reload
====================
*/
bool idMD5Anim::Reload()
{
	idStr filename = name;
	Free();

	return LoadAnim( filename );
}

/*
====================
idMD5Anim::Allocated
====================
*/
size_t idMD5Anim::Allocated() const
{
	size_t	size = bounds.Allocated() + jointInfo.Allocated() + componentFrames.Allocated() + name.Allocated();
	return size;
}

/*
====================
idMD5Anim::LoadAnim
====================
*/
ID_INLINE short AssertShortRange( int value )
{
	assert( value >= -( 1 << ( sizeof( short ) * 8 - 1 ) ) );
	assert( value < ( 1 << ( sizeof( short ) * 8 - 1 ) ) );
	return ( short ) value;
}

bool idMD5Anim::LoadAnim( const char* filename )
{
	idLexer	parser( LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT );
	idToken	token;

	idStr generatedFileName = "generated/anim/";
	generatedFileName.AppendPath( filename );
	generatedFileName.SetFileExtension( ".bMD5anim" );

	// Get the timestamp on the original file, if it's newer than what is stored in binary model, regenerate it
	ID_TIME_T sourceTimeStamp = fileSystem->GetTimestamp( filename );

	idFileLocal file( fileSystem->OpenFileReadMemory( generatedFileName ) );
	if( binaryLoadAnim.GetBool() && LoadBinary( file, sourceTimeStamp ) )
	{
		name = filename;
		if( cvarSystem->GetCVarBool( "fs_buildresources" ) )
		{
			// for resource gathering write this anim to the preload file for this map
			fileSystem->AddAnimPreload( name );
		}
		return true;
	}

	if( !parser.LoadFile( filename ) )
	{
		return false;
	}

	name = filename;

	Free();

	parser.ExpectTokenString( MD5_VERSION_STRING );
	int version = parser.ParseInt();
	if( version != MD5_VERSION )
	{
		parser.Error( "Invalid version %i. Should be version %i\n", version, MD5_VERSION );
	}

	// skip the commandline
	parser.ExpectTokenString( "commandline" );
	parser.ReadToken( &token );

	// parse num frames
	parser.ExpectTokenString( "numFrames" );
	numFrames = parser.ParseInt();
	if( numFrames <= 0 )
	{
		parser.Error( "Invalid number of frames: %i", numFrames );
	}

	// parse num joints
	parser.ExpectTokenString( "numJoints" );
	numJoints = parser.ParseInt();
	if( numJoints <= 0 )
	{
		parser.Error( "Invalid number of joints: %i", numJoints );
	}

	// parse frame rate
	parser.ExpectTokenString( "frameRate" );
	frameRate = parser.ParseInt();
	if( frameRate < 0 )
	{
		parser.Error( "Invalid frame rate: %d", frameRate );
	}

	// parse number of animated components
	parser.ExpectTokenString( "numAnimatedComponents" );
	numAnimatedComponents = parser.ParseInt();
	if( ( numAnimatedComponents < 0 ) || ( numAnimatedComponents > numJoints * 6 ) )
	{
		parser.Error( "Invalid number of animated components: %i", numAnimatedComponents );
	}

	// parse the hierarchy
	jointInfo.SetGranularity( 1 );
	jointInfo.SetNum( numJoints );
	parser.ExpectTokenString( "hierarchy" );
	parser.ExpectTokenString( "{" );
	for( int i = 0; i < numJoints; i++ )
	{
		parser.ReadToken( &token );
		jointInfo[ i ].nameIndex = AssertShortRange( animationLib.JointIndex( token ) );

		// parse parent num
		jointInfo[ i ].parentNum = AssertShortRange( parser.ParseInt() );
		if( jointInfo[ i ].parentNum >= i )
		{
			parser.Error( "Invalid parent num: %i", jointInfo[ i ].parentNum );
		}

		if( ( i != 0 ) && ( jointInfo[ i ].parentNum < 0 ) )
		{
			parser.Error( "Animations may have only one root joint" );
		}

		// parse anim bits
		jointInfo[ i ].animBits = AssertShortRange( parser.ParseInt() );
		if( jointInfo[ i ].animBits & ~63 )
		{
			parser.Error( "Invalid anim bits: %i", jointInfo[ i ].animBits );
		}

		// parse first component
		jointInfo[ i ].firstComponent = AssertShortRange( parser.ParseInt() );
		if( ( numAnimatedComponents > 0 ) && ( ( jointInfo[ i ].firstComponent < 0 ) || ( jointInfo[ i ].firstComponent >= numAnimatedComponents ) ) )
		{
			parser.Error( "Invalid first component: %i", jointInfo[ i ].firstComponent );
		}
	}

	parser.ExpectTokenString( "}" );

	// parse bounds
	parser.ExpectTokenString( "bounds" );
	parser.ExpectTokenString( "{" );
	bounds.SetGranularity( 1 );
	bounds.SetNum( numFrames );
	for( int i = 0; i < numFrames; i++ )
	{
		parser.Parse1DMatrix( 3, bounds[ i ][ 0 ].ToFloatPtr() );
		parser.Parse1DMatrix( 3, bounds[ i ][ 1 ].ToFloatPtr() );
	}
	parser.ExpectTokenString( "}" );

	// parse base frame
	baseFrame.SetGranularity( 1 );
	baseFrame.SetNum( numJoints );
	parser.ExpectTokenString( "baseframe" );
	parser.ExpectTokenString( "{" );
	for( int i = 0; i < numJoints; i++ )
	{
		idCQuat q;
		parser.Parse1DMatrix( 3, baseFrame[ i ].t.ToFloatPtr() );
		parser.Parse1DMatrix( 3, q.ToFloatPtr() );//baseFrame[ i ].q.ToFloatPtr() );
		baseFrame[ i ].q = q.ToQuat();//.w = baseFrame[ i ].q.CalcW();
		baseFrame[ i ].w = 0.0f;
	}
	parser.ExpectTokenString( "}" );

	// parse frames
	componentFrames.SetGranularity( 1 );
	componentFrames.SetNum( numAnimatedComponents * numFrames + JOINT_FRAME_PAD );
	componentFrames[ numAnimatedComponents * numFrames + JOINT_FRAME_PAD - 1 ] = 0.0f;

	float* componentPtr = componentFrames.Ptr();
	for( int i = 0; i < numFrames; i++ )
	{
		parser.ExpectTokenString( "frame" );
		int num = parser.ParseInt();
		if( num != i )
		{
			parser.Error( "Expected frame number %i", i );
		}
		parser.ExpectTokenString( "{" );

		for( int j = 0; j < numAnimatedComponents; j++, componentPtr++ )
		{
			*componentPtr = parser.ParseFloat();
		}

		parser.ExpectTokenString( "}" );
	}

	// get total move delta
	if( !numAnimatedComponents )
	{
		totaldelta.Zero();
	}
	else
	{
		componentPtr = &componentFrames[ jointInfo[ 0 ].firstComponent ];
		if( jointInfo[ 0 ].animBits & ANIM_TX )
		{
			for( int i = 0; i < numFrames; i++ )
			{
				componentPtr[ numAnimatedComponents * i ] -= baseFrame[ 0 ].t.x;
			}
			totaldelta.x = componentPtr[ numAnimatedComponents * ( numFrames - 1 ) ];
			componentPtr++;
		}
		else
		{
			totaldelta.x = 0.0f;
		}
		if( jointInfo[ 0 ].animBits & ANIM_TY )
		{
			for( int i = 0; i < numFrames; i++ )
			{
				componentPtr[ numAnimatedComponents * i ] -= baseFrame[ 0 ].t.y;
			}
			totaldelta.y = componentPtr[ numAnimatedComponents * ( numFrames - 1 ) ];
			componentPtr++;
		}
		else
		{
			totaldelta.y = 0.0f;
		}
		if( jointInfo[ 0 ].animBits & ANIM_TZ )
		{
			for( int i = 0; i < numFrames; i++ )
			{
				componentPtr[ numAnimatedComponents * i ] -= baseFrame[ 0 ].t.z;
			}
			totaldelta.z = componentPtr[ numAnimatedComponents * ( numFrames - 1 ) ];
		}
		else
		{
			totaldelta.z = 0.0f;
		}
	}
	baseFrame[ 0 ].t.Zero();

	// we don't count last frame because it would cause a 1 frame pause at the end
	animLength = ( ( numFrames - 1 ) * 1000 + frameRate - 1 ) / frameRate;

	if( binaryLoadAnim.GetBool() )
	{
		idLib::Printf( "Writing %s\n", generatedFileName.c_str() );
		idFileLocal outputFile( fileSystem->OpenFileWrite( generatedFileName, "fs_basepath" ) );
		WriteBinary( outputFile, sourceTimeStamp );
	}

	// done
	return true;
}

#if 0
/*
====================
idMD5Anim::Resample
====================
*/
void idMD5Anim::Resample()
{
	if( reduced ) return;

	int idealFrames = numFrames / 2;
	idList<short> resampledFrames;
	resampledFrames.SetGranularity( 1 );
	resampledFrames.SetNum( numAnimatedComponents * idealFrames );

	idCompressedJointQuat *compressedJoints = ( idCompressedJointQuat * ) _alloca16( numJoints * sizeof( compressedJoints[ 0 ] ) );
	idCompressedJointQuat *compressedBlendJoints = ( idCompressedJointQuat * ) _alloca16( numJoints * sizeof( compressedBlendJoints[ 0 ] ) );
	idJointQuat *joints = ( idJointQuat * ) _alloca16( numJoints * sizeof( joints[ 0 ] ) );
	idJointQuat *blendJoints = ( idJointQuat * ) _alloca16( numJoints * sizeof( blendJoints[ 0 ] ) );
	int *baseIndex = ( int* ) _alloca16( numJoints * sizeof( baseIndex[ 0 ] ) );
	for( int i = 0; i < numJoints; ++i )
	{
		baseIndex[ i ] = i;
	}

	for( int i = 0; i < idealFrames; ++i )
	{
		float srcf = ( i*( numFrames - 1 ) ) / ( idealFrames - 1 );
		int srci = ( int ) idMath::Floor( srcf );
		float blend = srcf - srci;
		if( i != srci )
		{
			bounds[ i ] = bounds[ srci ];
		}
		{
			short *destPtr = &resampledFrames[ i * numAnimatedComponents ];
			short *srcPtr = &componentFrames[ srci * numAnimatedComponents ];
			short *nextSrcPtr;
			if( ( srci + 1 ) < numFrames )
			{
				nextSrcPtr = &componentFrames[ ( srci + 1 ) * numAnimatedComponents ];
			}
			else
			{
				nextSrcPtr = srcPtr;
			}
			int numBaseIndex = 0;
			for( int j = 0; j < numJoints; ++j )
			{
				const jointAnimInfo_t *	infoPtr = &jointInfo[ j ];
				int animBits = infoPtr->animBits;
				if( animBits == 0 )
					continue;

				baseIndex[ numBaseIndex ] = numBaseIndex;
				idCompressedJointQuat *jointPtr = &compressedJoints[ numBaseIndex ];
				idCompressedJointQuat *blendPtr = &compressedBlendJoints[ numBaseIndex ];
				const short *jointframe1 = srcPtr + infoPtr->firstComponent;
				const short *jointframe2 = nextSrcPtr + infoPtr->firstComponent;

				*jointPtr = baseFrame[ j ];

				switch( animBits & ( ANIM_TX | ANIM_TY | ANIM_TZ ) )
				{
					case 0:
						blendPtr->t[ 0 ] = jointPtr->t[ 0 ];
						blendPtr->t[ 1 ] = jointPtr->t[ 1 ];
						blendPtr->t[ 2 ] = jointPtr->t[ 2 ];
						break;
					case ANIM_TX:
						jointPtr->t[ 0 ] = jointframe1[ 0 ];
						blendPtr->t[ 0 ] = jointframe2[ 0 ];
						blendPtr->t[ 1 ] = jointPtr->t[ 1 ];
						blendPtr->t[ 2 ] = jointPtr->t[ 2 ];
						jointframe1++;
						jointframe2++;
						break;
					case ANIM_TY:
						jointPtr->t[ 1 ] = jointframe1[ 0 ];
						blendPtr->t[ 1 ] = jointframe2[ 0 ];
						blendPtr->t[ 0 ] = jointPtr->t[ 0 ];
						blendPtr->t[ 2 ] = jointPtr->t[ 2 ];
						jointframe1++;
						jointframe2++;
						break;
					case ANIM_TZ:
						jointPtr->t[ 2 ] = jointframe1[ 0 ];
						blendPtr->t[ 2 ] = jointframe2[ 0 ];
						blendPtr->t[ 0 ] = jointPtr->t[ 0 ];
						blendPtr->t[ 1 ] = jointPtr->t[ 1 ];
						jointframe1++;
						jointframe2++;
						break;
					case ANIM_TX | ANIM_TY:
						jointPtr->t[ 0 ] = jointframe1[ 0 ];
						jointPtr->t[ 1 ] = jointframe1[ 1 ];
						blendPtr->t[ 0 ] = jointframe2[ 0 ];
						blendPtr->t[ 1 ] = jointframe2[ 1 ];
						blendPtr->t[ 2 ] = jointPtr->t[ 2 ];
						jointframe1 += 2;
						jointframe2 += 2;
						break;
					case ANIM_TX | ANIM_TZ:
						jointPtr->t[ 0 ] = jointframe1[ 0 ];
						jointPtr->t[ 2 ] = jointframe1[ 1 ];
						blendPtr->t[ 0 ] = jointframe2[ 0 ];
						blendPtr->t[ 2 ] = jointframe2[ 1 ];
						blendPtr->t[ 1 ] = jointPtr->t[ 1 ];
						jointframe1 += 2;
						jointframe2 += 2;
						break;
					case ANIM_TY | ANIM_TZ:
						jointPtr->t[ 1 ] = jointframe1[ 0 ];
						jointPtr->t[ 2 ] = jointframe1[ 1 ];
						blendPtr->t[ 1 ] = jointframe2[ 0 ];
						blendPtr->t[ 2 ] = jointframe2[ 1 ];
						blendPtr->t[ 0 ] = jointPtr->t[ 0 ];
						jointframe1 += 2;
						jointframe2 += 2;
						break;
					case ANIM_TX | ANIM_TY | ANIM_TZ:
						jointPtr->t[ 0 ] = jointframe1[ 0 ];
						jointPtr->t[ 1 ] = jointframe1[ 1 ];
						jointPtr->t[ 2 ] = jointframe1[ 2 ];
						blendPtr->t[ 0 ] = jointframe2[ 0 ];
						blendPtr->t[ 1 ] = jointframe2[ 1 ];
						blendPtr->t[ 2 ] = jointframe2[ 2 ];
						jointframe1 += 3;
						jointframe2 += 3;
						break;
				}

				switch( animBits & ( ANIM_QX | ANIM_QY | ANIM_QZ ) )
				{
					case 0:
						blendPtr->q[ 0 ] = jointPtr->q[ 0 ];
						blendPtr->q[ 1 ] = jointPtr->q[ 1 ];
						blendPtr->q[ 2 ] = jointPtr->q[ 2 ];
						break;
					case ANIM_QX:
						jointPtr->q[ 0 ] = jointframe1[ 0 ];
						blendPtr->q[ 0 ] = jointframe2[ 0 ];
						blendPtr->q[ 1 ] = jointPtr->q[ 1 ];
						blendPtr->q[ 2 ] = jointPtr->q[ 2 ];
						break;
					case ANIM_QY:
						jointPtr->q[ 1 ] = jointframe1[ 0 ];
						blendPtr->q[ 1 ] = jointframe2[ 0 ];
						blendPtr->q[ 0 ] = jointPtr->q[ 0 ];
						blendPtr->q[ 2 ] = jointPtr->q[ 2 ];
						break;
					case ANIM_QZ:
						jointPtr->q[ 2 ] = jointframe1[ 0 ];
						blendPtr->q[ 2 ] = jointframe2[ 0 ];
						blendPtr->q[ 0 ] = jointPtr->q[ 0 ];
						blendPtr->q[ 1 ] = jointPtr->q[ 1 ];
						break;
					case ANIM_QX | ANIM_QY:
						jointPtr->q[ 0 ] = jointframe1[ 0 ];
						jointPtr->q[ 1 ] = jointframe1[ 1 ];
						blendPtr->q[ 0 ] = jointframe2[ 0 ];
						blendPtr->q[ 1 ] = jointframe2[ 1 ];
						blendPtr->q[ 2 ] = jointPtr->q[ 2 ];
						break;
					case ANIM_QX | ANIM_QZ:
						jointPtr->q[ 0 ] = jointframe1[ 0 ];
						jointPtr->q[ 2 ] = jointframe1[ 1 ];
						blendPtr->q[ 0 ] = jointframe2[ 0 ];
						blendPtr->q[ 2 ] = jointframe2[ 1 ];
						blendPtr->q[ 1 ] = jointPtr->q[ 1 ];
						break;
					case ANIM_QY | ANIM_QZ:
						jointPtr->q[ 1 ] = jointframe1[ 0 ];
						jointPtr->q[ 2 ] = jointframe1[ 1 ];
						blendPtr->q[ 1 ] = jointframe2[ 0 ];
						blendPtr->q[ 2 ] = jointframe2[ 1 ];
						blendPtr->q[ 0 ] = jointPtr->q[ 0 ];
						break;
					case ANIM_QX | ANIM_QY | ANIM_QZ:
						jointPtr->q[ 0 ] = jointframe1[ 0 ];
						jointPtr->q[ 1 ] = jointframe1[ 1 ];
						jointPtr->q[ 2 ] = jointframe1[ 2 ];
						blendPtr->q[ 0 ] = jointframe2[ 0 ];
						blendPtr->q[ 1 ] = jointframe2[ 1 ];
						blendPtr->q[ 2 ] = jointframe2[ 2 ];
						break;
				}
				numBaseIndex++;
			}
			blendJoints = ( idJointQuat * ) _alloca16( baseFrame.Num() * sizeof( blendJoints[ 0 ] ) );

			SIMDProcessor->DecompressJoints( joints, compressedJoints, baseIndex, numBaseIndex );
			SIMDProcessor->DecompressJoints( blendJoints, compressedBlendJoints, baseIndex, numBaseIndex );

			SIMDProcessor->BlendJoints( joints, blendJoints, 1.f - blend, baseIndex, numBaseIndex );
			numBaseIndex = 0;
			for( int j = 0; j < numJoints; j++ )
			{
				const jointAnimInfo_t *	infoPtr = &jointInfo[ j ];
				int animBits = infoPtr->animBits;
				if( animBits == 0 )
				{
					continue;
				}

				idJointQuat const &curjoint = joints[ numBaseIndex ];
				idCQuat cq = curjoint.q.ToCQuat();
				idCompressedJointQuat cj;
				cj.t[ 0 ] = idCompressedJointQuat::OffsetToShort( curjoint.t.x );
				cj.t[ 1 ] = idCompressedJointQuat::OffsetToShort( curjoint.t.y );
				cj.t[ 2 ] = idCompressedJointQuat::OffsetToShort( curjoint.t.z );
				cj.q[ 0 ] = idCompressedJointQuat::QuatToShort( cq.x );
				cj.q[ 1 ] = idCompressedJointQuat::QuatToShort( cq.y );
				cj.q[ 2 ] = idCompressedJointQuat::QuatToShort( cq.z );

				short *output = &destPtr[ infoPtr->firstComponent ];
				if( animBits & ( ANIM_TX ) )
				{
					*output++ = cj.t[ 0 ];
				}
				if( animBits & ( ANIM_TY ) )
				{
					*output++ = cj.t[ 1 ];
				}
				if( animBits & ( ANIM_TZ ) )
				{
					*output++ = cj.t[ 2 ];
				}
				if( animBits & ( ANIM_QX ) )
				{
					*output++ = cj.q[ 0 ];
				}
				if( animBits & ( ANIM_QY ) )
				{
					*output++ = cj.q[ 1 ];
				}
				if( animBits & ( ANIM_QZ ) )
				{
					*output++ = cj.q[ 2 ];
				}

				numBaseIndex++;
			}
		}
	}
	int nb = numFrames;
	int fr = frameRate;
	frameRate = ( frameRate * idealFrames ) / numFrames;//(((numFrames - 1) * 1000) + animLength - 1) / (animLength);
	numFrames = idealFrames;
	animLength = ( ( numFrames - 1 ) * 1000 + frameRate - 1 ) / frameRate;

	bounds.SetGranularity( 1 );
	bounds.SetNum( numFrames );
	componentFrames = resampledFrames;

	reduced = true;
}
#endif

/*
========================
idMD5Anim::LoadBinary
========================
*/
bool idMD5Anim::LoadBinary( idFile* file, ID_TIME_T sourceTimeStamp )
{
	if( file == NULL )
	{
		return false;
	}

	unsigned int magic = 0;
	file->ReadBig( magic );
	if( magic != B_ANIM_MD5_MAGIC )
	{
		return false;
	}

	ID_TIME_T loadedTimeStamp;
	file->ReadBig( loadedTimeStamp );

	// RB: source might be from .resources, so we ignore the time stamp and assume a release build
	if( !fileSystem->InProductionMode() && ( sourceTimeStamp != FILE_NOT_FOUND_TIMESTAMP ) && ( sourceTimeStamp != 0 ) && ( sourceTimeStamp != loadedTimeStamp ) )
	{
		return false;
	}
	// RB end

	file->ReadBig( numFrames );
	file->ReadBig( frameRate );
	file->ReadBig( animLength );
	file->ReadBig( numJoints );
	file->ReadBig( numAnimatedComponents );

	int num;
	file->ReadBig( num );
	bounds.SetNum( num );
	for( int i = 0; i < num; i++ )
	{
		file->ReadBig( bounds[ i ][ 0 ] );
		file->ReadBig( bounds[ i ][ 1 ] );
	}

	file->ReadBig( num );
	jointInfo.SetNum( num );
	for( int i = 0; i < num; i++ )
	{
		idStr jointName;
		file->ReadString( jointName );
		jointInfo[ i ].nameIndex = jointName.IsEmpty() ? -1 : animationLib.JointIndex( jointName.c_str() );
		int temp;
		file->ReadBig( temp ); jointInfo[ i ].parentNum = AssertShortRange( temp );
		file->ReadBig( temp ); jointInfo[ i ].animBits = AssertShortRange( temp );
		file->ReadBig( temp ); jointInfo[ i ].firstComponent = AssertShortRange( temp );
	}

	file->ReadBig( num );
	baseFrame.SetNum( num );
	for( int i = 0; i < num; i++ )
	{
		file->ReadBig( baseFrame[ i ].q.x );
		file->ReadBig( baseFrame[ i ].q.y );
		file->ReadBig( baseFrame[ i ].q.z );
		file->ReadBig( baseFrame[ i ].q.w );
		file->ReadVec3( baseFrame[ i ].t );
		baseFrame[ i ].w = 0.0f;
	}

	file->ReadBig( num );
	componentFrames.SetNum( num + JOINT_FRAME_PAD );
	for( int i = 0; i < componentFrames.Num(); i++ )
	{
		file->ReadFloat( componentFrames[ i ] );
	}

	//file->ReadString( name );
	file->ReadVec3( totaldelta );
	//file->ReadBig( ref_count );

	return true;
}

/*
========================
idMD5Anim::WriteBinary
========================
*/
void idMD5Anim::WriteBinary( idFile* file, ID_TIME_T sourceTimeStamp )
{
	if( file == NULL )
	{
		return;
	}

	file->WriteBig( B_ANIM_MD5_MAGIC );
	file->WriteBig( sourceTimeStamp );

	file->WriteBig( numFrames );
	file->WriteBig( frameRate );
	file->WriteBig( animLength );
	file->WriteBig( numJoints );
	file->WriteBig( numAnimatedComponents );

	file->WriteBig( bounds.Num() );
	for( int i = 0; i < bounds.Num(); i++ )
	{
		idBounds& b = bounds[ i ];
		file->WriteBig( b[ 0 ] );
		file->WriteBig( b[ 1 ] );
	}

	file->WriteBig( jointInfo.Num() );
	for( int i = 0; i < jointInfo.Num(); i++ )
	{
		jointAnimInfo_t & animInfo = jointInfo[ i ];

		file->WriteString( animationLib.JointName( animInfo.nameIndex ) );
		file->WriteInt( animInfo.parentNum );
		file->WriteInt( animInfo.animBits );
		file->WriteInt( animInfo.firstComponent );
	}

	file->WriteBig( baseFrame.Num() );
	for( int i = 0; i < baseFrame.Num(); i++ )
	{
		idJointQuat& jointQuat = baseFrame[ i ];

		file->WriteBig( jointQuat.q.x );
		file->WriteBig( jointQuat.q.y );
		file->WriteBig( jointQuat.q.z );
		file->WriteBig( jointQuat.q.w );
		file->WriteVec3( jointQuat.t );
	}

	file->WriteBig( componentFrames.Num() - JOINT_FRAME_PAD );
	for( int i = 0; i < componentFrames.Num(); i++ )
	{
		file->WriteFloat( componentFrames[ i ] );
	}

	//file->WriteString( name );
	file->WriteVec3( totaldelta );
	//file->WriteBig( ref_count );
}

/*
====================
idMD5Anim::GetFrameBlend
====================
*/
void idMD5Anim::GetFrameBlend( int framenum, frameBlend_t& frame ) const
{
	frame.cycleCount = 0;
	frame.backlerp = 0.0f;
	frame.frontlerp = 1.0f;

	// frame 1 is first frame
	framenum--;
	if( framenum < 0 )
	{
		framenum = 0;
	}
	else if( framenum >= numFrames )
	{
		framenum = numFrames - 1;
	}

	frame.frame1 = framenum;
	frame.frame2 = framenum;
}

/*
====================
idMD5Anim::ConvertTimeToFrame
====================
*/
void idMD5Anim::ConvertTimeToFrame( int time, int cyclecount, frameBlend_t& frame ) const
{
	int frameTime;
	int frameNum;

	if( numFrames <= 1 )
	{
		frame.frame1 = 0;
		frame.frame2 = 0;
		frame.backlerp = 0.0f;
		frame.frontlerp = 1.0f;
		frame.cycleCount = 0;
		return;
	}

	if( time <= 0 )
	{
		frame.frame1 = 0;
		frame.frame2 = 1;
		frame.backlerp = 0.0f;
		frame.frontlerp = 1.0f;
		frame.cycleCount = 0;
		return;
	}

	frameTime = time * frameRate;
	frameNum = frameTime / 1000;
	frame.cycleCount = frameNum / ( numFrames - 1 );

	if( ( cyclecount > 0 ) && ( frame.cycleCount >= cyclecount ) )
	{
		frame.cycleCount = cyclecount - 1;
		frame.frame1 = numFrames - 1;
		frame.frame2 = frame.frame1;
		frame.backlerp = 0.0f;
		frame.frontlerp = 1.0f;
		return;
	}

	frame.frame1 = frameNum % ( numFrames - 1 );
	frame.frame2 = frame.frame1 + 1;
	if( frame.frame2 >= numFrames )
	{
		frame.frame2 = 0;
	}

	frame.backlerp = ( frameTime % 1000 ) * 0.001f;
	frame.frontlerp = 1.0f - frame.backlerp;
}

/*
====================
idMD5Anim::GetOrigin
====================
*/
void idMD5Anim::GetOrigin( idVec3& offset, int time, int cyclecount ) const
{
	offset = baseFrame[ 0 ].t;
	if( !( jointInfo[ 0 ].animBits & ( ANIM_TX | ANIM_TY | ANIM_TZ ) ) )
	{
		// just use the baseframe
		return;
	}

	frameBlend_t frame;
	ConvertTimeToFrame( time, cyclecount, frame );

	const float* componentPtr1 = &componentFrames[ numAnimatedComponents * frame.frame1 + jointInfo[ 0 ].firstComponent ];
	const float* componentPtr2 = &componentFrames[ numAnimatedComponents * frame.frame2 + jointInfo[ 0 ].firstComponent ];

	if( jointInfo[ 0 ].animBits & ANIM_TX )
	{
		offset.x = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
		componentPtr1++;
		componentPtr2++;
	}

	if( jointInfo[ 0 ].animBits & ANIM_TY )
	{
		offset.y = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
		componentPtr1++;
		componentPtr2++;
	}

	if( jointInfo[ 0 ].animBits & ANIM_TZ )
	{
		offset.z = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
	}

	if( frame.cycleCount )
	{
		offset += totaldelta * ( float ) frame.cycleCount;
	}
}

/*
====================
idMD5Anim::GetOriginRotation
====================
*/
void idMD5Anim::GetOriginRotation( idQuat& rotation, int time, int cyclecount ) const
{
	int animBits = jointInfo[ 0 ].animBits;
	if( !( animBits & ( ANIM_QX | ANIM_QY | ANIM_QZ ) ) )
	{
		// just use the baseframe
		rotation = baseFrame[ 0 ].q;
		return;
	}

	frameBlend_t frame;
	ConvertTimeToFrame( time, cyclecount, frame );

	const float*	jointframe1 = &componentFrames[ numAnimatedComponents * frame.frame1 + jointInfo[ 0 ].firstComponent ];
	const float*	jointframe2 = &componentFrames[ numAnimatedComponents * frame.frame2 + jointInfo[ 0 ].firstComponent ];

	if( animBits & ANIM_TX )
	{
		jointframe1++;
		jointframe2++;
	}

	if( animBits & ANIM_TY )
	{
		jointframe1++;
		jointframe2++;
	}

	if( animBits & ANIM_TZ )
	{
		jointframe1++;
		jointframe2++;
	}

	idQuat q1;
	idQuat q2;

	switch( animBits & ( ANIM_QX | ANIM_QY | ANIM_QZ ) )
	{
		case ANIM_QX:
			q1.x = jointframe1[ 0 ];
			q2.x = jointframe2[ 0 ];
			q1.y = baseFrame[ 0 ].q.y;
			q2.y = q1.y;
			q1.z = baseFrame[ 0 ].q.z;
			q2.z = q1.z;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QY:
			q1.y = jointframe1[ 0 ];
			q2.y = jointframe2[ 0 ];
			q1.x = baseFrame[ 0 ].q.x;
			q2.x = q1.x;
			q1.z = baseFrame[ 0 ].q.z;
			q2.z = q1.z;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QZ:
			q1.z = jointframe1[ 0 ];
			q2.z = jointframe2[ 0 ];
			q1.x = baseFrame[ 0 ].q.x;
			q2.x = q1.x;
			q1.y = baseFrame[ 0 ].q.y;
			q2.y = q1.y;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QX | ANIM_QY:
			q1.x = jointframe1[ 0 ];
			q1.y = jointframe1[ 1 ];
			q2.x = jointframe2[ 0 ];
			q2.y = jointframe2[ 1 ];
			q1.z = baseFrame[ 0 ].q.z;
			q2.z = q1.z;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QX | ANIM_QZ:
			q1.x = jointframe1[ 0 ];
			q1.z = jointframe1[ 1 ];
			q2.x = jointframe2[ 0 ];
			q2.z = jointframe2[ 1 ];
			q1.y = baseFrame[ 0 ].q.y;
			q2.y = q1.y;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QY | ANIM_QZ:
			q1.y = jointframe1[ 0 ];
			q1.z = jointframe1[ 1 ];
			q2.y = jointframe2[ 0 ];
			q2.z = jointframe2[ 1 ];
			q1.x = baseFrame[ 0 ].q.x;
			q2.x = q1.x;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QX | ANIM_QY | ANIM_QZ:
			q1.x = jointframe1[ 0 ];
			q1.y = jointframe1[ 1 ];
			q1.z = jointframe1[ 2 ];
			q2.x = jointframe2[ 0 ];
			q2.y = jointframe2[ 1 ];
			q2.z = jointframe2[ 2 ];
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
	}

	rotation.Slerp( q1, q2, frame.backlerp );
}

/*
====================
idMD5Anim::GetBounds
====================
*/
void idMD5Anim::GetBounds( idBounds& bnds, int time, int cyclecount ) const
{
	frameBlend_t frame;
	ConvertTimeToFrame( time, cyclecount, frame );

	bnds = bounds[ frame.frame1 ];
	bnds.AddBounds( bounds[ frame.frame2 ] );

	// origin position
	idVec3 offset = baseFrame[ 0 ].t;
	if( jointInfo[ 0 ].animBits & ( ANIM_TX | ANIM_TY | ANIM_TZ ) )
	{
		const float* componentPtr1 = &componentFrames[ numAnimatedComponents * frame.frame1 + jointInfo[ 0 ].firstComponent ];
		const float* componentPtr2 = &componentFrames[ numAnimatedComponents * frame.frame2 + jointInfo[ 0 ].firstComponent ];

		if( jointInfo[ 0 ].animBits & ANIM_TX )
		{
			offset.x = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
			componentPtr1++;
			componentPtr2++;
		}

		if( jointInfo[ 0 ].animBits & ANIM_TY )
		{
			offset.y = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
			componentPtr1++;
			componentPtr2++;
		}

		if( jointInfo[ 0 ].animBits & ANIM_TZ )
		{
			offset.z = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
		}
	}

	bnds[ 0 ] -= offset;
	bnds[ 1 ] -= offset;
}

/*
====================
DecodeInterpolatedFrames

====================
*/
int DecodeInterpolatedFrames( idJointQuat* joints, idJointQuat* blendJoints, int* lerpIndex, const float* frame1, const float* frame2,
							  const jointAnimInfo_t* jointInfo, const int* index, const int numIndexes )
{
	int numLerpJoints = 0;
	for( int i = 0; i < numIndexes; ++i )
	{
		const int j = index[ i ];

		const int animBits = jointInfo[ j ].animBits;
		if( animBits == 0 )
			continue;
		
		lerpIndex[ numLerpJoints++ ] = j;

		idJointQuat* jointPtr = &joints[ j ];
		idJointQuat* blendPtr = &blendJoints[ j ];

	#if USE_INTRINSICS
		_mm_store_ps( blendJoints[ j ].ToFloatPtr() + 0, _mm_load_ps( joints[ j ].ToFloatPtr() + 0 ) );
		_mm_store_ps( blendJoints[ j ].ToFloatPtr() + 4, _mm_load_ps( joints[ j ].ToFloatPtr() + 4 ) );
	#else
		*blendPtr = *jointPtr;
	#endif

		const float* jointframe1 = frame1 + jointInfo[ j ].firstComponent;
		const float* jointframe2 = frame2 + jointInfo[ j ].firstComponent;

		/*( animBits & ( ANIM_TX | ANIM_TY | ANIM_TZ ) )
		{
			if( animBits & ANIM_TX )
			{
				jointPtr->t.x = *jointframe1++;
				blendPtr->t.x = *jointframe2++;
			}
			if( animBits & ANIM_TY )
			{
				jointPtr->t.y = *jointframe1++;
				blendPtr->t.y = *jointframe2++;
			}
			if( animBits & ANIM_TZ )
			{
				jointPtr->t.z = *jointframe1++;
				blendPtr->t.z = *jointframe2++;
			}
		}*/
		switch( animBits & ( ANIM_TX | ANIM_TY | ANIM_TZ ) )
		{
			case 0: break;
			case ANIM_TX:
				jointPtr->t.x = jointframe1[ 0 ];
				blendPtr->t.x = jointframe2[ 0 ];
				jointframe1++;
				jointframe2++;
				break;
			case ANIM_TY:
				jointPtr->t.y = jointframe1[ 0 ];
				blendPtr->t.y = jointframe2[ 0 ];
				jointframe1++;
				jointframe2++;
				break;
			case ANIM_TZ:
				jointPtr->t.z = jointframe1[ 0 ];
				blendPtr->t.z = jointframe2[ 0 ];
				jointframe1++;
				jointframe2++;
				break;
			case ANIM_TX | ANIM_TY:
				jointPtr->t.x = jointframe1[ 0 ];
				blendPtr->t.x = jointframe2[ 0 ];
				jointPtr->t.y = jointframe1[ 1 ];
				blendPtr->t.y = jointframe2[ 1 ];
				jointframe1 += 2;
				jointframe2 += 2;
				break;
			case ANIM_TX | ANIM_TZ:
				jointPtr->t.x = jointframe1[ 0 ];
				blendPtr->t.x = jointframe2[ 0 ];
				jointPtr->t.z = jointframe1[ 1 ];
				blendPtr->t.z = jointframe2[ 1 ];
				jointframe1 += 2;
				jointframe2 += 2;
				break;
			case ANIM_TY | ANIM_TZ:
				jointPtr->t.y = jointframe1[ 0 ];
				blendPtr->t.y = jointframe2[ 0 ];
				jointPtr->t.z = jointframe1[ 1 ];
				blendPtr->t.z = jointframe2[ 1 ];
				jointframe1 += 2;
				jointframe2 += 2;
				break;
			case ANIM_TX | ANIM_TY | ANIM_TZ:
				jointPtr->t.x = jointframe1[ 0 ];
				jointPtr->t.y = jointframe1[ 1 ];
				jointPtr->t.z = jointframe1[ 2 ];
				blendPtr->t.x = jointframe2[ 0 ];
				blendPtr->t.y = jointframe2[ 1 ];
				blendPtr->t.z = jointframe2[ 2 ];
				jointframe1 += 3;
				jointframe2 += 3;
				break;
		}

		/*if( animBits & ( ANIM_QX | ANIM_QY | ANIM_QZ ) )
		{
			if( animBits & ANIM_QX )
			{
				jointPtr->q.x = *jointframe1++;
				blendPtr->q.x = *jointframe2++;
			}
			if( animBits & ANIM_QY )
			{
				jointPtr->q.y = *jointframe1++;
				blendPtr->q.y = *jointframe2++;
			}
			if( animBits & ANIM_QZ )
			{
				jointPtr->q.z = *jointframe1++;
				blendPtr->q.z = *jointframe2++;
			}
			jointPtr->q.w = jointPtr->q.CalcW();
			blendPtr->q.w = blendPtr->q.CalcW();
		}*/
		switch( animBits & ( ANIM_QX | ANIM_QY | ANIM_QZ ) )
		{
			case 0: break;
			case ANIM_QX:
				jointPtr->q.x = jointframe1[ 0 ];
				jointPtr->q.w = jointPtr->q.CalcW();
				blendPtr->q.x = jointframe2[ 0 ];				
				blendPtr->q.w = blendPtr->q.CalcW();
				break;
			case ANIM_QY:
				jointPtr->q.y = jointframe1[ 0 ];
				jointPtr->q.w = jointPtr->q.CalcW();
				blendPtr->q.y = jointframe2[ 0 ];		
				blendPtr->q.w = blendPtr->q.CalcW();
				break;
			case ANIM_QZ:
				jointPtr->q.z = jointframe1[ 0 ];
				jointPtr->q.w = jointPtr->q.CalcW();
				blendPtr->q.z = jointframe2[ 0 ];			
				blendPtr->q.w = blendPtr->q.CalcW();
				break;
			case ANIM_QX | ANIM_QY:
				jointPtr->q.x = jointframe1[ 0 ];
				jointPtr->q.y = jointframe1[ 1 ];
				jointPtr->q.w = jointPtr->q.CalcW();
				blendPtr->q.x = jointframe2[ 0 ];			
				blendPtr->q.y = jointframe2[ 1 ];				
				blendPtr->q.w = blendPtr->q.CalcW();
				break;
			case ANIM_QX | ANIM_QZ:
				jointPtr->q.x = jointframe1[ 0 ];
				jointPtr->q.z = jointframe1[ 1 ];
				jointPtr->q.w = jointPtr->q.CalcW();
				blendPtr->q.x = jointframe2[ 0 ];				
				blendPtr->q.z = jointframe2[ 1 ];		
				blendPtr->q.w = blendPtr->q.CalcW();
				break;
			case ANIM_QY | ANIM_QZ:
				jointPtr->q.y = jointframe1[ 0 ];
				jointPtr->q.z = jointframe1[ 1 ];
				jointPtr->q.w = jointPtr->q.CalcW();
				blendPtr->q.y = jointframe2[ 0 ];			
				blendPtr->q.z = jointframe2[ 1 ];			
				blendPtr->q.w = blendPtr->q.CalcW();
				break;
			case ANIM_QX | ANIM_QY | ANIM_QZ:
				jointPtr->q.x = jointframe1[ 0 ];			
				jointPtr->q.y = jointframe1[ 1 ];
				jointPtr->q.z = jointframe1[ 2 ];
				jointPtr->q.w = jointPtr->q.CalcW();
				blendPtr->q.x = jointframe2[ 0 ];
				blendPtr->q.y = jointframe2[ 1 ];			
				blendPtr->q.z = jointframe2[ 2 ];
				blendPtr->q.w = blendPtr->q.CalcW();
				break;
		}
	}
	return numLerpJoints;
}

/*
====================
idMD5Anim::GetInterpolatedFrame
====================
*/
void idMD5Anim::GetInterpolatedFrame( frameBlend_t& frame, idJointQuat* joints, const int* index, int numIndexes ) const
{
	// copy the baseframe
	SIMDProcessor->Memcpy( joints, baseFrame.Ptr(), baseFrame.Num() * sizeof( baseFrame[ 0 ] ) );

	if( numAnimatedComponents == 0 )
	{
		// just use the base frame
		return;
	}

	idJointQuat* blendJoints = ( idJointQuat* ) _alloca16( baseFrame.Num() * sizeof( blendJoints[ 0 ] ) );
	int* lerpIndex = ( int* ) _alloca16( baseFrame.Num() * sizeof( lerpIndex[ 0 ] ) );

	const float* frame1 = &componentFrames[ frame.frame1 * numAnimatedComponents ];
	const float* frame2 = &componentFrames[ frame.frame2 * numAnimatedComponents ];

	int numLerpJoints = DecodeInterpolatedFrames( joints, blendJoints, lerpIndex, frame1, frame2, jointInfo.Ptr(), index, numIndexes );

	SIMDProcessor->BlendJoints( joints, blendJoints, frame.backlerp, lerpIndex, numLerpJoints );

	if( frame.cycleCount )
	{
		joints[ 0 ].t += totaldelta * ( float ) frame.cycleCount;
	}
}

/*
====================
DecodeSingleFrame

====================
*/
void DecodeSingleFrame( idJointQuat* joints, const float* frame, const jointAnimInfo_t* jointInfo, const int* index, const int numIndexes )
{
	for( int i = 0; i < numIndexes; ++i )
	{
		const int j = index[ i ];

		const int animBits = jointInfo[ j ].animBits;
		if( animBits == 0 ) 
			continue;

		idJointQuat* jointPtr = &joints[ j ];

		const float* jointframe = frame + jointInfo[ j ].firstComponent;

		/*if( animBits & ( ANIM_TX | ANIM_TY | ANIM_TZ ) )
		{
			if( animBits & ANIM_TX )
			{
				jointPtr->t.x = *jointframe++;
			}
			if( animBits & ANIM_TY )
			{
				jointPtr->t.y = *jointframe++;
			}
			if( animBits & ANIM_TZ )
			{
				jointPtr->t.z = *jointframe++;
			}
		}*/
		switch( animBits & ( ANIM_TX | ANIM_TY | ANIM_TZ ) )
		{
			case 0: break;
			case ANIM_TX:
				jointPtr->t[ 0 ] = jointframe[ 0 ];
				jointframe++;
				break;
			case ANIM_TY:
				jointPtr->t[ 1 ] = jointframe[ 0 ];
				jointframe++;
				break;
			case ANIM_TZ:
				jointPtr->t[ 2 ] = jointframe[ 0 ];
				jointframe++;
				break;
			case ANIM_TX | ANIM_TY:
				jointPtr->t[ 0 ] = jointframe[ 0 ];
				jointPtr->t[ 1 ] = jointframe[ 1 ];
				jointframe += 2;
				break;
			case ANIM_TX | ANIM_TZ:
				jointPtr->t[ 0 ] = jointframe[ 0 ];
				jointPtr->t[ 2 ] = jointframe[ 1 ];
				jointframe += 2;
				break;
			case ANIM_TY | ANIM_TZ:
				jointPtr->t[ 1 ] = jointframe[ 0 ];
				jointPtr->t[ 2 ] = jointframe[ 1 ];
				jointframe += 2;
				break;
			case ANIM_TX | ANIM_TY | ANIM_TZ:
				jointPtr->t[ 0 ] = jointframe[ 0 ];
				jointPtr->t[ 1 ] = jointframe[ 1 ];
				jointPtr->t[ 2 ] = jointframe[ 2 ];
				jointframe += 3;
				break;
		}

		/*if( animBits & ( ANIM_QX | ANIM_QY | ANIM_QZ ) )
		{
			if( animBits & ANIM_QX )
			{
				jointPtr->q.x = *jointframe++;
			}
			if( animBits & ANIM_QY )
			{
				jointPtr->q.y = *jointframe++;
			}
			if( animBits & ANIM_QZ )
			{
				jointPtr->q.z = *jointframe++;
			}
			jointPtr->q.w = jointPtr->q.CalcW();
		}*/
		switch( animBits & ( ANIM_QX | ANIM_QY | ANIM_QZ ) )
		{
			case 0: break;
			case ANIM_QX:
				jointPtr->q.x = jointframe[ 0 ];
				jointPtr->q.w = jointPtr->q.CalcW();
				break;
			case ANIM_QY:
				jointPtr->q.y = jointframe[ 0 ];
				jointPtr->q.w = jointPtr->q.CalcW();
				break;
			case ANIM_QZ:
				jointPtr->q.z = jointframe[ 0 ];
				jointPtr->q.w = jointPtr->q.CalcW();
				break;
			case ANIM_QX | ANIM_QY:
				jointPtr->q.x = jointframe[ 0 ];
				jointPtr->q.y = jointframe[ 1 ];
				jointPtr->q.w = jointPtr->q.CalcW();
				break;
			case ANIM_QX | ANIM_QZ:
				jointPtr->q.x = jointframe[ 0 ];
				jointPtr->q.z = jointframe[ 1 ];
				jointPtr->q.w = jointPtr->q.CalcW();
				break;
			case ANIM_QY | ANIM_QZ:
				jointPtr->q.y = jointframe[ 0 ];
				jointPtr->q.z = jointframe[ 1 ];
				jointPtr->q.w = jointPtr->q.CalcW();
				break;
			case ANIM_QX | ANIM_QY | ANIM_QZ:
				jointPtr->q.x = jointframe[ 0 ];
				jointPtr->q.y = jointframe[ 1 ];
				jointPtr->q.z = jointframe[ 2 ];
				jointPtr->q.w = jointPtr->q.CalcW();
				break;
		}

	}
}

/*
====================
idMD5Anim::GetSingleFrame
====================
*/
void idMD5Anim::GetSingleFrame( int framenum, idJointQuat* joints, const int* index, int numIndexes ) const
{
	// copy the baseframe
	SIMDProcessor->Memcpy( joints, baseFrame.Ptr(), baseFrame.Num() * sizeof( baseFrame[ 0 ] ) );

	if( framenum == 0 || numAnimatedComponents == 0 )
	{
		// just use the base frame
		return;
	}

	const float* frame = &componentFrames[ framenum * numAnimatedComponents ];

	DecodeSingleFrame( joints, frame, jointInfo.Ptr(), index, numIndexes );
}

/*
====================
idMD5Anim::CheckModelHierarchy
====================
*/
void idMD5Anim::CheckModelHierarchy( const idRenderModel* model ) const
{
	if( jointInfo.Num() != model->NumJoints() )
	{
		if( !fileSystem->InProductionMode() )
		{
			gameLocal.Warning( "Model '%s' has different # of joints than anim '%s'", model->Name(), name.c_str() );
		}
		else
		{
			//gameLocal.Warning( "Model '%s' has different # of joints than anim '%s'", model->Name(), name.c_str() );
			return;
		}
	}

	const idMD5Joint* modelJoints = model->GetJoints();
	for( int i = 0; i < jointInfo.Num(); i++ )
	{
		int jointNum = jointInfo[ i ].nameIndex;
		if( modelJoints[ i ].name != animationLib.JointName( jointNum ) )
		{
			gameLocal.Error( "Model '%s''s joint names don't match anim '%s''s", model->Name(), name.c_str() );
		}
		int parent;
		if( modelJoints[ i ].parent )
		{
			parent = modelJoints[ i ].parent - modelJoints;
		}
		else
		{
			parent = -1;
		}
		if( parent != jointInfo[ i ].parentNum )
		{
			gameLocal.Error( "Model '%s' has different joint hierarchy than anim '%s'", model->Name(), name.c_str() );
		}
	}
}

/***********************************************************************

	idAnimManager

***********************************************************************/

/*
====================
idAnimManager::idAnimManager
====================
*/
idAnimManager::idAnimManager()
{}

/*
====================
idAnimManager::~idAnimManager
====================
*/
idAnimManager::~idAnimManager()
{
	Shutdown();
}

/*
====================
idAnimManager::Shutdown
====================
*/
void idAnimManager::Shutdown()
{
	animations.DeleteContents();
	jointnames.Clear();
	jointnamesHash.Free();
}

/*
====================
idAnimManager::GetAnim
====================
*/
idMD5Anim* idAnimManager::GetAnim( const char* name )
{
	idMD5Anim** animptrptr;
	idMD5Anim* anim;

	// see if it has been asked for before
	animptrptr = NULL;
	if( animations.Get( name, &animptrptr ) )
	{
		anim = *animptrptr;
	}
	else
	{
		idStr extension;
		idStr filename = name;

		filename.ExtractFileExtension( extension );
		if( extension != MD5_ANIM_EXT )
		{
			return NULL;
		}

		anim = new( TAG_ANIM ) idMD5Anim();
		if( !anim->LoadAnim( filename ) )
		{
			gameLocal.Warning( "Couldn't load anim: '%s'", filename.c_str() );
			delete anim;
			anim = NULL;
		}
		animations.Set( filename, anim );
	}

	return anim;
}

/*
================
idAnimManager::Preload
================
*/
void idAnimManager::Preload( const idPreloadManifest& manifest )
{
	if( manifest.NumResources() >= 0 )
	{
		common->Printf( "Preloading anims...\n" );
		int	start = sys->Milliseconds();
		int numLoaded = 0;
		for( int i = 0; i < manifest.NumResources(); i++ )
		{
			const preloadEntry_s& p = manifest.GetPreloadByIndex( i );
			if( p.resType == PRELOAD_ANIM )
			{
				GetAnim( p.resourceName );
				numLoaded++;
			}
		}
		int	end = sys->Milliseconds();
		common->Printf( "%05d anims preloaded ( or were already loaded ) in %5.1f seconds\n", numLoaded, ( end - start ) * 0.001 );
		common->Printf( "----------------------------------------\n" );
	}
}

/*
================
idAnimManager::ReloadAnims
================
*/
void idAnimManager::ReloadAnims()
{
	int			i;
	idMD5Anim**	animptr;

	for( i = 0; i < animations.Num(); i++ )
	{
		animptr = animations.GetIndex( i );
		if( animptr != NULL && *animptr != NULL )
		{
			( *animptr )->Reload();
		}
	}
}

/*
================
idAnimManager::JointIndex
================
*/
int	idAnimManager::JointIndex( const char* name )
{
	int i, hash;

	hash = jointnamesHash.GenerateKey( name );
	for( i = jointnamesHash.First( hash ); i != -1; i = jointnamesHash.Next( i ) )
	{
		if( jointnames[ i ].Cmp( name ) == 0 )
		{
			return i;
		}
	}

	i = jointnames.Append( name );
	jointnamesHash.Add( hash, i );
	return i;
}

/*
================
idAnimManager::JointName
================
*/
const char* idAnimManager::JointName( int index ) const
{
	return jointnames[ index ];
}

/*
================
idAnimManager::ListAnims
================
*/
void idAnimManager::ListAnims() const
{
	int			i;
	idMD5Anim**	animptr;
	idMD5Anim*	anim;
	size_t		size;
	size_t		s;
	size_t		namesize;
	int			num;

	num = 0;
	size = 0;
	for( i = 0; i < animations.Num(); i++ )
	{
		animptr = animations.GetIndex( i );
		if( animptr != NULL && *animptr != NULL )
		{
			anim = *animptr;
			s = anim->Size();
			gameLocal.Printf( "%8d bytes : %2d refs : %s\n", s, anim->NumRefs(), anim->Name() );
			size += s;
			num++;
		}
	}

	namesize = jointnames.Size() + jointnamesHash.Size();
	for( i = 0; i < jointnames.Num(); i++ )
	{
		namesize += jointnames[ i ].Size();
	}

	gameLocal.Printf( "\n%d memory used in %d anims\n", size, num );
	gameLocal.Printf( "%d memory used in %d joint names\n", namesize, jointnames.Num() );
}

/*
================
idAnimManager::FlushUnusedAnims
================
*/
void idAnimManager::FlushUnusedAnims()
{
	int						i;
	idMD5Anim**				animptr;
	idList<idMD5Anim*>		removeAnims;

	for( i = 0; i < animations.Num(); i++ )
	{
		animptr = animations.GetIndex( i );
		if( animptr != NULL && *animptr != NULL )
		{
			if( ( *animptr )->NumRefs() <= 0 )
			{
				removeAnims.Append( *animptr );
			}
		}
	}

	for( i = 0; i < removeAnims.Num(); i++ )
	{
		animations.Remove( removeAnims[ i ]->Name() );
		delete removeAnims[ i ];
	}
}