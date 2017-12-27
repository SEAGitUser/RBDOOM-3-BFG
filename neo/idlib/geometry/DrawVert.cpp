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

#define attributeOffset( Class, Name ) reinterpret_cast<const void*>( offsetof( Class , Name ) )

const void * const idDrawVert::positionOffset = attributeOffset( idDrawVert, xyz );
const void * const idDrawVert::texcoordOffset = attributeOffset( idDrawVert, st );
const void * const idDrawVert::normalOffset = attributeOffset( idDrawVert, normal );
const void * const idDrawVert::tangentOffset = attributeOffset( idDrawVert, tangent );
const void * const idDrawVert::colorOffset = attributeOffset( idDrawVert, color );
const void * const idDrawVert::color2Offset = attributeOffset( idDrawVert, color2 );

const void * const idShadowVert::xyzwOffset = attributeOffset( idShadowVert, xyzw );

const void * const idShadowVertSkinned::xyzwOffset = attributeOffset( idShadowVertSkinned, xyzw );
const void * const idShadowVertSkinned::colorOffset = attributeOffset( idShadowVertSkinned, color );
const void * const idShadowVertSkinned::color2Offset = attributeOffset( idShadowVertSkinned, color2 );

/*
============
idDrawVert::ReadFromFile
============
*/
void idDrawVert::ReadFromFile( idFile *file )
{
	file->ReadVec3( xyz );
	file->ReadBigArray( st, 2 );
	file->ReadBigArray( normal, 4 );
	file->ReadBigArray( tangent, 4 );
	file->ReadBigArray( color, sizeof( color ) / sizeof( color[ 0 ] ) );
	file->ReadBigArray( color2, sizeof( color2 ) / sizeof( color2[ 0 ] ) );
}
void idDrawVert::ReadFromFile_NoWeights( idFile *file )
{
	file->ReadVec3( xyz );
	file->ReadBigArray( st, 2 );
	file->ReadBigArray( normal, 4 );
	file->ReadBigArray( tangent, 4 );
	file->ReadUnsignedChar( color[ 0 ] );
	file->ReadUnsignedChar( color[ 1 ] );
	file->ReadUnsignedChar( color[ 2 ] );
	file->ReadUnsignedChar( color[ 3 ] );
}
/*
============
idDrawVert::WriteToFile
============
*/
void idDrawVert::WriteToFile( idFile *file ) const
{
	file->WriteVec3( xyz );
	file->WriteBigArray( st, 2 );
	file->WriteBigArray( normal, 4 );
	file->WriteBigArray( tangent, 4 );
	file->WriteBigArray( color, sizeof( color ) / sizeof( color[ 0 ] ) );
	file->WriteBigArray( color2, sizeof( color2 ) / sizeof( color2[ 0 ] ) );
}
void idDrawVert::WriteToFile_NoWeights( idFile *file ) const
{
	file->WriteVec3( xyz );
	file->WriteBigArray( st, 2 );
	file->WriteBigArray( normal, 4 );
	file->WriteBigArray( tangent, 4 );
	file->WriteUnsignedChar( color[ 0 ] );
	file->WriteUnsignedChar( color[ 1 ] );
	file->WriteUnsignedChar( color[ 2 ] );
	file->WriteUnsignedChar( color[ 3 ] );
}

/*
============
TransformVertsAndTangents
============
*/
void idDrawVert::TransformVertsAndTangents( idDrawVert* targetVerts, const int numVerts, const idDrawVert* baseVerts, const idJointMat* joints )
{
	for( int i = 0; i < numVerts; ++i )
	{
		const idDrawVert & base = baseVerts[ i ];

		const idJointMat & j0 = joints[ base.color[ 0 ] ];
		const idJointMat & j1 = joints[ base.color[ 1 ] ];
		const idJointMat & j2 = joints[ base.color[ 2 ] ];
		const idJointMat & j3 = joints[ base.color[ 3 ] ];

		const float w0 = base.color2[ 0 ] * ( 1.0f / 255.0f );
		const float w1 = base.color2[ 1 ] * ( 1.0f / 255.0f );
		const float w2 = base.color2[ 2 ] * ( 1.0f / 255.0f );
		const float w3 = base.color2[ 3 ] * ( 1.0f / 255.0f );

		idJointMat accum;
		idJointMat::Mul( accum, j0, w0 );
		idJointMat::Mad( accum, j1, w1 );
		idJointMat::Mad( accum, j2, w2 );
		idJointMat::Mad( accum, j3, w3 );

		targetVerts[ i ].SetPosition( accum * idVec4( base.GetPosition(), 1.0f ) );
		targetVerts[ i ].SetNormal( accum * base.GetNormal() );
		targetVerts[ i ].SetTangent( accum * base.GetTangent() );
		targetVerts[ i ].tangent[ 3 ] = base.tangent[ 3 ];
	}
}

/*
============
idShadowVert::CreateShadowCache
============
*/
int idShadowVert::CreateShadowCache( idShadowVert* vertexCache, const idDrawVert* verts, const int numVerts )
{
	for( int i = 0; i < numVerts; ++i )
	{
		auto & pos = verts[ i ].GetPosition();

		vertexCache[i * 2 + 0].xyzw[0] = pos[0];
		vertexCache[i * 2 + 0].xyzw[1] = pos[1];
		vertexCache[i * 2 + 0].xyzw[2] = pos[2];
		vertexCache[i * 2 + 0].xyzw[3] = 1.0f;
		
		vertexCache[i * 2 + 1].xyzw[0] = pos[0];
		vertexCache[i * 2 + 1].xyzw[1] = pos[1];
		vertexCache[i * 2 + 1].xyzw[2] = pos[2];
		vertexCache[i * 2 + 1].xyzw[3] = 0.0f;
	}
	return numVerts * 2;
}

/*
===================
idShadowVertSkinned::CreateShadowCache
===================
*/
int idShadowVertSkinned::CreateShadowCache( idShadowVertSkinned* vertexCache, const idDrawVert* verts, const int numVerts )
{
	for( int i = 0; i < numVerts; ++i )
	{
		auto & pos = verts[ i ].GetPosition();

		vertexCache[0].xyzw[0] = pos[0];
		vertexCache[0].xyzw[1] = pos[1];
		vertexCache[0].xyzw[2] = pos[2];
		vertexCache[0].xyzw[3] = 1.0f;
		*( unsigned int* )vertexCache[0].color = *( unsigned int* )verts[i].color;
		*( unsigned int* )vertexCache[0].color2 = *( unsigned int* )verts[i].color2;
		
		vertexCache[1].xyzw[0] = pos[0];
		vertexCache[1].xyzw[1] = pos[1];
		vertexCache[1].xyzw[2] = pos[2];
		vertexCache[1].xyzw[3] = 0.0f;
		*( unsigned int* )vertexCache[1].color = *( unsigned int* )verts[i].color;
		*( unsigned int* )vertexCache[1].color2 = *( unsigned int* )verts[i].color2;
		
		vertexCache += 2;
	}
	return numVerts * 2;
}
