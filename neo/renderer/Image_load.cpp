/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2016 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

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

#include "framework/Common_local.h"
#include "tr_local.h"

/*
================
BitsForFormat
================
*/
int idImage::BitsForFormat( textureFormat_t format )
{
	switch( format )
	{
		case FMT_NONE:
			return 0;
		case FMT_RGBA8:
			return 32;
		case FMT_XRGB8:
			return 32;
		case FMT_RGB565:
			return 16;
		case FMT_L8A8:
			return 16;
		case FMT_ALPHA:
			return 8;
		case FMT_LUM8:
			return 8;
		case FMT_INT8:
			return 8;
		case FMT_DXT1:
			return 4;
		case FMT_DXT5:
			return 8;
		// RB: added ETC compression
		case FMT_ETC1_RGB8_OES:
			return 4;
		case FMT_RG11F_B10F://SEA
			return 32;
		case FMT_RG16F://SEA
			return 32;
		case FMT_RGBA16F:
			return 64;
		case FMT_RGBA32F:
			return 128;
		case FMT_R32F:
			return 32;
		// RB end
		case FMT_DEPTH:
			return 24;
		case FMT_DEPTH_STENCIL://SEA
			return 32;
		case FMT_X16:
			return 16;
		case FMT_Y16_X16:
			return 32;
		default:
			assert( 0 );
			return 0;
	}
}

/*
========================
idImage::DeriveOpts
========================
*/
ID_INLINE void idImage::DeriveOpts()
{
	if( opts.format == FMT_NONE )
	{
		opts.colorFormat = CFM_DEFAULT;
		
		switch( usage )
		{
			case TD_COVERAGE:
				opts.format = FMT_DXT1;
				opts.colorFormat = CFM_GREEN_ALPHA;
				break;
			case TD_DEPTH:
				opts.format = FMT_DEPTH_STENCIL;
				break;
				
			case TD_SHADOWMAP:
				opts.format = FMT_DEPTH;
				break;
				
			case TD_RGBA16F:
				opts.format = FMT_RGBA16F;
				break;
				
			case TD_RGBA32F:
				opts.format = FMT_RGBA32F;
				break;
				
			case TD_R32F:
				opts.format = FMT_R32F;
				break;
				
			case TD_DIFFUSE:
				// TD_DIFFUSE gets only set to when its a diffuse texture for an interaction
				opts.gammaMips = true;
				opts.format = FMT_DXT5;
				opts.colorFormat = CFM_YCOCG_DXT5;
				break;
			case TD_SPECULAR:
				opts.gammaMips = true;
				opts.format = FMT_DXT1;
				opts.colorFormat = CFM_DEFAULT;
				break;
			case TD_DEFAULT:
				opts.gammaMips = true;
				opts.format = FMT_DXT5;
				opts.colorFormat = CFM_DEFAULT;
				break;
			case TD_BUMP:
				opts.format = FMT_DXT5;
				opts.colorFormat = CFM_NORMAL_DXT5;
				break;
			case TD_FONT:
				opts.format = FMT_DXT1;
				opts.colorFormat = CFM_GREEN_ALPHA;
				opts.numLevels = 4; // We only support 4 levels because we align to 16 in the exporter
				opts.gammaMips = true;
				break;
			case TD_LIGHT:
				// RB: don't destroy lighting
				opts.format = FMT_RGB565; //FMT_RGBA8;
				opts.gammaMips = true;
				break;
			case TD_LOOKUP_TABLE_MONO:
				opts.format = FMT_INT8;
				break;
			case TD_LOOKUP_TABLE_ALPHA:
				opts.format = FMT_ALPHA;
				break;
			case TD_LOOKUP_TABLE_RGB1:
			case TD_LOOKUP_TABLE_RGBA:
				opts.format = FMT_RGBA8;
				break;
			default:
				assert( false );
				opts.format = FMT_RGBA8;
		}
	}
	
	if( opts.numLevels == 0 )
	{
		opts.numLevels = 1;
		
		if( filter == TF_LINEAR || filter == TF_NEAREST )
		{
			// don't create mip maps if we aren't going to be using them
		}
		else
		{
			int	temp_width = opts.width;
			int	temp_height = opts.height;
			while( temp_width > 1 || temp_height > 1 )
			{
				temp_width >>= 1;
				temp_height >>= 1;
				if( ( opts.format == FMT_DXT1 || opts.format == FMT_DXT5 || opts.format == FMT_ETC1_RGB8_OES ) &&
					( ( temp_width & 0x3 ) != 0 || ( temp_height & 0x3 ) != 0 ) 
				) {
					break;
				}
				opts.numLevels++;
			}
		}
	}
}

/*
========================
idImage::AllocImage
========================
*/
void idImage::AllocImage( const idImageOpts& imgOpts, textureFilter_t tf, textureRepeat_t tr )
{
	filter = tf;
	repeat = tr;
	opts = imgOpts;
	DeriveOpts();
	AllocImage();
}

/*
================
GenerateImage
================
*/
void idImage::GenerateImage( const byte* pic, int width, int height, int depth, int msaaSamples, 
	textureFilter_t filterParm, textureRepeat_t repeatParm, textureUsage_t usageParm )
{
	PurgeImage();
	
	filter = filterParm;
	repeat = repeatParm;
	usage = usageParm;
	layout = IMG_LAYOUT_2D;
	
	opts.textureType = TT_2D;
	opts.width = Max( width, 1 );
	opts.height = Max( height, 1 );
	opts.depth = Max( depth, 1 );
	opts.numLevels = 0;
	opts.numSamples = msaaSamples;
	DeriveOpts();
	
	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before the render starts would miss
	// the generated texture
	if( !R_IsInitialized() )
	{
		return;
	}
	
	// RB: allow pic == NULL for internal framebuffer images
	if( pic == NULL || GetOpts().IsMultisampled() )
	{
		AllocImage();
		return;
	}

	idBinaryImage im( GetName() );
				
	// foresthale 2014-05-30: give a nice progress display when binarizing
	commonLocal.LoadPacifierBinarizeFilename( GetName() , "generated image" );

	if( GetOpts().numLevels > 1 )
	{
		commonLocal.LoadPacifierBinarizeProgressTotal( GetOpts().width * GetOpts().height * 4 / 3 );
	}
	else {
		commonLocal.LoadPacifierBinarizeProgressTotal( GetOpts().width * GetOpts().height );
	}
		
	im.Load2DFromMemory( width, height, pic, GetOpts().numLevels, opts.format, opts.colorFormat, GetOpts().gammaMips );
		
	commonLocal.LoadPacifierBinarizeEnd();
		
	AllocImage();
		
	for( int i = 0; i < im.NumImages(); ++i )
	{
		const bimageImage_t& img = im.GetImageHeader( i );
		const byte* data = im.GetImageData( i );
		SubImageUpload( img.level, 0, 0, img.destZ, img.width, img.height, data );
	}
}

/*
====================
GenerateCubeImage

Non-square cube sides are not allowed
====================
*/
void idImage::GenerateCubeImage( const byte* pic[6], int size, int depth, textureFilter_t filterParm, textureUsage_t usageParm )
{
	PurgeImage();
	
	filter = filterParm;
	repeat = TR_CLAMP;
	usage = usageParm;
	layout = IMG_LAYOUT_CUBE_NATIVE;
	
	opts.textureType = TT_CUBIC;
	opts.width = opts.height = Max( size, 1 );
	opts.depth = Max( depth, 1 );
	opts.numLevels = 0;
	opts.numSamples = 1;
	DeriveOpts();
	
	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before the render starts would miss
	// the generated texture
	if( !R_IsInitialized() )
	{
		return;
	}

	if( pic == NULL )
	{
		AllocImage();
		return;
	}
	
	idBinaryImage im( GetName() );
	
	// foresthale 2014-05-30: give a nice progress display when binarizing
	commonLocal.LoadPacifierBinarizeFilename( GetName(), "generated cube image" );
	if( opts.numLevels > 1 )
	{
		commonLocal.LoadPacifierBinarizeProgressTotal( opts.width * opts.width * 6 * 4 / 3 );
	}
	else {
		commonLocal.LoadPacifierBinarizeProgressTotal( opts.width * opts.width * 6 );
	}
	
	im.LoadCubeFromMemory( size, pic, opts.numLevels, opts.format, opts.gammaMips );
	
	commonLocal.LoadPacifierBinarizeEnd();
	
	AllocImage();
	
	for( int i = 0; i < im.NumImages(); ++i )
	{
		const bimageImage_t& img = im.GetImageHeader( i );
		const byte* data = im.GetImageData( i );
		SubImageUpload( img.level, 0, 0, img.destZ, img.width, img.height, data );
	}
}

/*
===============
 Generate3DImage
===============
*/
void idImage::Generate3DImage( const byte* pic, int width, int height, int depth, 
	textureFilter_t filterParm, textureRepeat_t repeatParm, textureUsage_t usageParm )
{
	PurgeImage();

	filter = filterParm;
	repeat = repeatParm;
	usage = usageParm;
	layout = IMG_LAYOUT_2D;

	opts.textureType = TT_3D;
	opts.width = Max( width, 1 );
	opts.height = Max( height, 1 );
	opts.depth = Max( depth, 1 );
	opts.numLevels = 0;
	opts.numSamples = 1;
	DeriveOpts();

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before the render starts would miss
	// the generated texture
	if( !R_IsInitialized() )
	{
		return;
	}

	if( pic == NULL )
	{
		AllocImage();
		return;
	}

	idBinaryImage im( GetName() );

	// foresthale 2014-05-30: give a nice progress display when binarizing
	commonLocal.LoadPacifierBinarizeFilename( GetName(), "generated image" );

	if( GetOpts().numLevels > 1 )
	{
		commonLocal.LoadPacifierBinarizeProgressTotal( GetOpts().width * GetOpts().height * 4 / 3 );
	}
	else {
		commonLocal.LoadPacifierBinarizeProgressTotal( GetOpts().width * GetOpts().height );
	}

	im.Load2DFromMemory( width, height, pic, GetOpts().numLevels, opts.format, opts.colorFormat, GetOpts().gammaMips );

	commonLocal.LoadPacifierBinarizeEnd();

	AllocImage();

	for( int i = 0; i < im.NumImages(); ++i )
	{
		const bimageImage_t& img = im.GetImageHeader( i );
		const byte* data = im.GetImageData( i );
		SubImageUpload( img.level, 0, 0, img.destZ, img.width, img.height, data );
	}
}

/*
===============
 GetGeneratedName

	name contains GetName() upon entry
===============
*/
void idImage::GetGeneratedName( idStr& _name, const textureUsage_t& _usage, const textureLayout_t& _cube )
{
	idStrStatic< 64 > extension;
	
	_name.ExtractFileExtension( extension );
	_name.StripFileExtension();
	
	_name += va( "#__%02d%02d", ( int )_usage, ( int )_cube );
	if( extension.Length() > 0 )
	{
		_name.SetFileExtension( extension );
	}
}

/*
===============
 ActuallyLoadImage

	Absolutely every image goes through this path
	On exit, the idImage will have a valid OpenGL texture number that can be bound
===============
*/
void idImage::ActuallyLoadImage( bool fromBackEnd )
{
	// if we don't have a rendering context yet, just return
	if( !R_IsInitialized() )
	{
		return;
	}
	
	// this is the ONLY place generatorFunction will ever be called
	if( generatorFunction )
	{
		generatorFunction( this );
		return;
	}
	
	if( com_productionMode.GetInteger() != 0 )
	{
		sourceFileTime = FILE_NOT_FOUND_TIMESTAMP;
		if( layout != IMG_LAYOUT_2D )
		{
			opts.textureType = TT_CUBIC;
			repeat = TR_CLAMP;
		}
	}
	else {
		if( layout != IMG_LAYOUT_2D )
		{
			opts.textureType = TT_CUBIC;
			repeat = TR_CLAMP;
			R_LoadCubeImages( GetName(), layout, NULL, NULL, &sourceFileTime );
		}
		else {
			opts.textureType = TT_2D;
			R_LoadImageProgram( GetName(), NULL, NULL, NULL, &sourceFileTime, &usage );
		}
	}
	
	// Figure out opts.colorFormat and opts.format so we can make sure the binary image is up to date
	DeriveOpts();
	
	idStrStatic< MAX_OSPATH > generatedName = GetName();
	GetGeneratedName( generatedName, usage, layout );
	
	idBinaryImage im( generatedName );
	binaryFileTime = im.LoadFromGeneratedFile( sourceFileTime );
	
	// BFHACK, do not want to tweak on buildgame so catch these images here
	if( binaryFileTime == FILE_NOT_FOUND_TIMESTAMP && fileSystem->UsingResourceFiles() )
	{
		int c = 1;
		while( c-- > 0 )
		{
			if( generatedName.Find( "guis/assets/white#__0000", false ) >= 0 )
			{
				generatedName.Replace( "white#__0000", "white#__0200" );
				im.SetName( generatedName );
				binaryFileTime = im.LoadFromGeneratedFile( sourceFileTime );
				break;
			}
			if( generatedName.Find( "guis/assets/white#__0100", false ) >= 0 )
			{
				generatedName.Replace( "white#__0100", "white#__0200" );
				im.SetName( generatedName );
				binaryFileTime = im.LoadFromGeneratedFile( sourceFileTime );
				break;
			}
			if( generatedName.Find( "textures/black#__0100", false ) >= 0 )
			{
				generatedName.Replace( "black#__0100", "black#__0200" );
				im.SetName( generatedName );
				binaryFileTime = im.LoadFromGeneratedFile( sourceFileTime );
				break;
			}
			if( generatedName.Find( "textures/decals/bulletglass1_d#__0100", false ) >= 0 )
			{
				generatedName.Replace( "bulletglass1_d#__0100", "bulletglass1_d#__0200" );
				im.SetName( generatedName );
				binaryFileTime = im.LoadFromGeneratedFile( sourceFileTime );
				break;
			}
			if( generatedName.Find( "models/monsters/skeleton/skeleton01_d#__1000", false ) >= 0 )
			{
				generatedName.Replace( "skeleton01_d#__1000", "skeleton01_d#__0100" );
				im.SetName( generatedName );
				binaryFileTime = im.LoadFromGeneratedFile( sourceFileTime );
				break;
			}
		}
	}
	const bimageFile_t& header = im.GetFileHeader();
	
	if( ( fileSystem->InProductionMode() && binaryFileTime != FILE_NOT_FOUND_TIMESTAMP ) || 
		( ( binaryFileTime != FILE_NOT_FOUND_TIMESTAMP )
			&& ( header.colorFormat == opts.colorFormat )
			&& ( header.format == opts.format )
			&& ( header.textureType == opts.textureType ) ) 
	) {
		opts.width = header.width;
		opts.height = header.height;
		opts.depth = 1 ; //header.depth; //SEA tofix!
		opts.numLevels = header.numLevels;
		opts.colorFormat = ( textureColor_t )header.colorFormat;
		opts.format = ( textureFormat_t )header.format;
		opts.textureType = ( textureType_t )header.textureType;
		if( cvarSystem->GetCVarBool( "fs_buildresources" ) )
		{
			// for resource gathering write this image to the preload file for this map
			fileSystem->AddImagePreload( GetName(), filter, repeat, usage, layout );
		}
	}
	else
	{
		idStr binarizeReason = "binarize: unknown reason";
		if( binaryFileTime == FILE_NOT_FOUND_TIMESTAMP )
		{
			binarizeReason = va( "binarize: binary file not found '%s'", generatedName.c_str() );
		}
		else if( header.colorFormat != opts.colorFormat )
		{
			binarizeReason = va( "binarize: mismatch color format '%s'", generatedName.c_str() );
		}
		else if( header.colorFormat != opts.colorFormat )
		{
			binarizeReason = va( "binarize: mismatched color format '%s'", generatedName.c_str() );
		}
		else if( header.textureType != opts.textureType )
		{
			binarizeReason = va( "binarize: mismatched texture type '%s'", generatedName.c_str() );
		}
		//else if( toolUsage )
		//	binarizeReason = va( "binarize: tool usage '%s'", generatedName.c_str() );
		
		if( layout != IMG_LAYOUT_2D )
		{
			int size;
			byte* pics[6];
			
			if( !R_LoadCubeImages( GetName(), layout, pics, &size, &sourceFileTime ) || size == 0 )
			{
				idLib::Warning( "Couldn't load cube image: %s", GetName() );
				return;
			}
			
			repeat = TR_CLAMP;
			
			opts.textureType = TT_CUBIC;
			opts.width = size;
			opts.height = size;
			opts.depth = 1; //SEA tofix!
			opts.numLevels = 0;
			
			DeriveOpts();
			
			// foresthale 2014-05-30: give a nice progress display when binarizing
			commonLocal.LoadPacifierBinarizeFilename( generatedName.c_str(), binarizeReason.c_str() );
			if( opts.numLevels > 1 )
			{
				commonLocal.LoadPacifierBinarizeProgressTotal( opts.width * opts.width * 6 * 4 / 3 );
			}
			else
			{
				commonLocal.LoadPacifierBinarizeProgressTotal( opts.width * opts.width * 6 );
			}
			
			im.LoadCubeFromMemory( size, ( const byte** )pics, opts.numLevels, opts.format, opts.gammaMips );
			
			commonLocal.LoadPacifierBinarizeEnd();
			
			repeat = TR_CLAMP;
			
			for( int i = 0; i < 6; i++ )
			{
				if( pics[i] )
				{
					Mem_Free( pics[i] );
				}
			}
		}
		else
		{
			int width, height;
			byte* pic;
			
			// load the full specification, and perform any image program calculations
			R_LoadImageProgram( GetName(), &pic, &width, &height, &sourceFileTime, &usage );
			
			if( pic == NULL )
			{
				idLib::Warning( "Couldn't load image: %s : %s", GetName(), generatedName.c_str() );
				// create a default so it doesn't get continuously reloaded
				opts.width = 8;
				opts.height = 8;
				opts.depth = 1;
				opts.numLevels = 1;
				DeriveOpts();
				AllocImage();
				
				// clear the data so it's not left uninitialized
				idTempArray<byte> clear( opts.width * opts.height * 4 );
				memset( clear.Ptr(), 0, clear.Size() );
				for( int level = 0; level < opts.numLevels; level++ )
				{
					SubImageUpload( level, 0, 0, 0, opts.width >> level, opts.height >> level, clear.Ptr() );
				}
				
				return;
			}
			
			opts.width = width;
			opts.height = height;
			opts.depth = 1; //SEA tofix!
			opts.numLevels = 0;
			DeriveOpts();
			
			// foresthale 2014-05-30: give a nice progress display when binarizing
			commonLocal.LoadPacifierBinarizeFilename( generatedName.c_str(), binarizeReason.c_str() );
			if( opts.numLevels > 1 )
			{
				commonLocal.LoadPacifierBinarizeProgressTotal( opts.width * opts.width * 6 * 4 / 3 );
			}
			else
			{
				commonLocal.LoadPacifierBinarizeProgressTotal( opts.width * opts.width * 6 );
			}
			
			im.Load2DFromMemory( opts.width, opts.height, pic, opts.numLevels, opts.format, opts.colorFormat, opts.gammaMips );
			commonLocal.LoadPacifierBinarizeEnd();
			
			Mem_Free( pic );
		}
		binaryFileTime = im.WriteGeneratedFile( sourceFileTime );
	}
	
	AllocImage();
	
	
	for( int i = 0; i < im.NumImages(); i++ )
	{
		const bimageImage_t& img = im.GetImageHeader( i );
		const byte* data = im.GetImageData( i );
		SubImageUpload( img.level, 0, 0, img.destZ, img.width, img.height, data );
	}
}

/*
================
MakePowerOfTwo
================
*/
int MakePowerOfTwo( int num )
{
	int	pot;
	for( pot = 1; pot < num; pot <<= 1 )
	{
	}
	return pot;
}

/*
========================
idImage::Resize
========================
*/
bool idImage::Resize( int width, int height, int depth )
{
	if( opts.width == width && opts.height == height && opts.depth == depth )
		return false;

	opts.width = width;
	opts.height = height;
	opts.depth = depth;

	AllocImage();

	return true;
}

/*
=============
RB_UploadScratchImage

if rows = cols * 6, assume it is a cube map animation
=============
*/
void idImage::UploadScratch( const byte* data, int cols, int rows )
{
	// if rows = cols * 6, assume it is a cube map animation
	if( rows == ( cols * 6 ) )
	{
		rows /= 6;
		const byte* pic[6];
		for( int i = 0; i < 6; i++ )
		{
			pic[i] = data + cols * rows * 4 * i;
		}
		
		if( opts.textureType != TT_CUBIC || usage != TD_LOOKUP_TABLE_RGBA )
		{
			GenerateCubeImage( pic, cols, 1, TF_LINEAR, TD_LOOKUP_TABLE_RGBA );
			return;
		}

		if( opts.width != cols || opts.height != rows )
		{
			opts.width = cols;
			opts.height = rows;
			AllocImage();
		}
		SetSamplerState( TF_LINEAR, TR_CLAMP );
		for( int i = 0; i < 6; i++ )
		{
			SubImageUpload( 0, 0, 0, i, opts.width, opts.height, pic[i] );
		}		
	}
	else {
		if( opts.textureType != TT_2D || usage != TD_LOOKUP_TABLE_RGBA )
		{
			GenerateImage( data, cols, rows, 1, 1, TF_LINEAR, TR_REPEAT, TD_LOOKUP_TABLE_RGBA );
			return;
		}

		if( opts.width != cols || opts.height != rows )
		{
			opts.width = cols;
			opts.height = rows;
			AllocImage();
		}
		SetSamplerState( TF_LINEAR, TR_REPEAT );
		SubImageUpload( 0, 0, 0, 0, opts.width, opts.height, data );
	}
}

/*
==================
StorageSize
==================
*/
int idImage::StorageSize() const
{
	if( !IsLoaded() )
	{
		return 0;
	}

	int baseSize = opts.width * opts.height;// * opts.depth;
	//if( opts.textureType == TT_CUBIC ) {  SEA ???
	//	baseSize *= 6;
	//}
	if( opts.numLevels > 1 )
	{
		baseSize *= 4;
		baseSize /= 3;
	}
	baseSize *= BitsForFormat( opts.format );
	baseSize /= 8;
	return baseSize;
}

/*
==================
Print
==================
*/
void idImage::Print() const
{
	if( generatorFunction )
	{
		common->Printf( "F" );
	}
	else
	{
		common->Printf( " " );
	}
	
	if( GetOpts().textureType == TT_2D )
	{
		if( GetOpts().IsMultisampled() )
		{
			if( GetOpts().IsArray() )
			{
				common->Printf( "2DMS-A " );
			}
			else
			{
				common->Printf( "2DMS " );
			}
		}
		else
		{
			if( GetOpts().IsArray() )
			{
				common->Printf( "2D-A " );
			}
			else
			{
				common->Printf( "2D " );
			}
		}
	}
	else if( GetOpts().textureType == TT_CUBIC )
	{
		if( GetOpts().IsArray() )
		{
			common->Printf( "6D-A " );
		}
		else
		{
			common->Printf( "6D " );
		}
	}
	else if( GetOpts().textureType == TT_3D )
	{
		common->Printf( "3D " );
	}
	else
	{
		common->Printf( "<BAD TYPE:%i>", opts.textureType );
	}
	
	common->Printf( "%4i %4i %4i ",	opts.width, opts.height, opts.depth );
	
	switch( opts.format )
	{
#define NAME_FORMAT( x ) case FMT_##x: common->Printf( "%-16s ", #x ); break;
		NAME_FORMAT( NONE );
		NAME_FORMAT( RGBA8 );
		NAME_FORMAT( XRGB8 );
		NAME_FORMAT( RGB565 );
		NAME_FORMAT( L8A8 );
		NAME_FORMAT( ALPHA );
		NAME_FORMAT( LUM8 );
		NAME_FORMAT( INT8 );
		NAME_FORMAT( DXT1 );
		NAME_FORMAT( DXT5 );
		NAME_FORMAT( DEPTH );
		NAME_FORMAT( X16 );
		NAME_FORMAT( Y16_X16 );
		NAME_FORMAT( DEPTH_STENCIL ); //SEA
		NAME_FORMAT( RG11F_B10F ); //SEA
		NAME_FORMAT( RG16F ); //SEA
		// RB begin
		NAME_FORMAT( ETC1_RGB8_OES );
		NAME_FORMAT( RGBA16F );
		NAME_FORMAT( RGBA32F );
		NAME_FORMAT( R32F );
		// RB end
		default:
			common->Printf( "<%3i>", opts.format );
			break;
#undef NAME_FORMAT
	}
	
	switch( filter )
	{
		case TF_DEFAULT:
			common->Printf( "mip  " );
			break;
		case TF_LINEAR:
			common->Printf( "linr " );
			break;
		case TF_NEAREST:
			common->Printf( "nrst " );
			break;
		case TF_NEAREST_MIPMAP:
			common->Printf( "nmip " );
			break;
		default:
			common->Printf( "<BAD FILTER:%i>", filter );
			break;
	}
	
	switch( repeat )
	{
		case TR_REPEAT:
			common->Printf( "rept " );
			break;
		case TR_CLAMP_TO_ZERO:
			common->Printf( "zero " );
			break;
		case TR_CLAMP_TO_ZERO_ALPHA:
			common->Printf( "azro " );
			break;
		case TR_CLAMP:
			common->Printf( "clmp " );
			break;
		default:
			common->Printf( "<BAD REPEAT:%i>", repeat );
			break;
	}
	
	common->Printf( "%4ik ", SIZE_MB( StorageSize() ) );
	
	common->Printf( " %s\n", GetName() );
}

/*
===============
idImage::Reload
===============
*/
void idImage::Reload( bool force )
{
	// always regenerate functional images
	if( generatorFunction )
	{
		common->DPrintf( "regenerating %s.\n", GetName() );
		generatorFunction( this );
		return;
	}
	
	// check file times
	if( !force )
	{
		ID_TIME_T current;
		if( layout != IMG_LAYOUT_2D )
		{
			R_LoadCubeImages( imgName, layout, NULL, NULL, &current );
		}
		else {
			// get the current values
			R_LoadImageProgram( imgName, NULL, NULL, NULL, &current );
		}
		if( current <= sourceFileTime )
		{
			return;
		}
	}
	
	common->DPrintf( "reloading %s.\n", GetName() );
	
	PurgeImage();
	
	// Load is from the front end, so the back end must be synced
	ActuallyLoadImage( false );
}
