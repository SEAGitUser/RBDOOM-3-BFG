/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2016 Robert Beckebans

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

/*
================================================================================================
Contains the Image implementation for OpenGL.
================================================================================================
*/

#include "../tr_local.h"

struct glTextureObject_t 
{
	glTextureObject_t() : target( GL_TEXTURE_2D ), uploadTarget( GL_TEXTURE_2D ), texnum( GL_NONE ),
		internalFormat( GL_RGBA8 ), dataFormat( GL_RGBA ), dataType( GL_UNSIGNED_BYTE ) {
	}
	glTextureObject_t( const idImage *img )
	{
		DeriveTargetInfo( img );
		DeriveFormatInfo( img );
	}
	GLuint	texnum;
	GLenum	target;
	GLenum	uploadTarget;
	GLenum	internalFormat;
	GLenum	dataFormat;
	GLenum	dataType;

	void DeriveTargetInfo( const idImage *img  )
	{
			if( img->GetOpts().textureType == TT_2D )
			{
				if( img->GetOpts().IsMultisampled() )
				{
					if( img->GetOpts().IsArray() )
					{
						target = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
						uploadTarget = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
					}
					else
					{
						target = GL_TEXTURE_2D_MULTISAMPLE;
						uploadTarget = GL_TEXTURE_2D_MULTISAMPLE;
					}
				}
				else
				{
					if( img->GetOpts().IsArray() )
					{
						target = GL_TEXTURE_2D_ARRAY;
						uploadTarget = GL_TEXTURE_2D_ARRAY;
					}
					else
					{
						target = GL_TEXTURE_2D;
						uploadTarget = GL_TEXTURE_2D;
					}
				}
			}
			else if( img->GetOpts().textureType == TT_CUBIC )
			{
				if( img->GetOpts().IsArray() )
				{
					target = GL_TEXTURE_CUBE_MAP_ARRAY;
					uploadTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
				}
				else
				{
					target = GL_TEXTURE_CUBE_MAP;
					uploadTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
				}
			}
			else if( img->GetOpts().textureType == TT_3D )
			{
				target = GL_TEXTURE_3D;
				uploadTarget = GL_TEXTURE_3D;
			}
			else
			{
				assert( !"invalid opts.textureType" );
				target = GL_TEXTURE_2D;
				uploadTarget = GL_TEXTURE_2D;
			}

		texnum = reinterpret_cast< GLuint >( img->GetAPIObject() );
	}

	// target
	// 
	/*void * PackTextureInfo()
	{
		uint64 apiObject = texnum;

		apiObject |= ;

		return reinterpret_cast<void*>( apiObject );
	}*/

	void DeriveFormatInfo( const idImage *img )
	{
		const bool sRGB = glConfig.sRGBFramebufferAvailable && ( r_useSRGB.GetInteger() == 1 || r_useSRGB.GetInteger() == 3 );

		switch( img->GetOpts().format )
		{
			case FMT_RGBA8:
				//internalFormat = GL_RGBA8;
				internalFormat = sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
				dataFormat = GL_RGBA;
				dataType = GL_UNSIGNED_BYTE;
				break;

			case FMT_XRGB8:
				internalFormat = sRGB ? GL_SRGB8 : GL_RGB;
				dataFormat = GL_RGBA;
				dataType = GL_UNSIGNED_BYTE;
				break;

			case FMT_RGB565: // light textures typically
				//internalFormat = sRGB ? GL_SRGB : GL_RGB;
				internalFormat = GL_RGB565;
				dataFormat = GL_RGB;
				dataType = GL_UNSIGNED_SHORT_5_6_5;
				break;

			case FMT_ALPHA:
				internalFormat = sRGB ? GL_SRGB : GL_R8; //SEA ???
				//internalFormat = GL_R8;
				dataFormat = GL_RED;
				dataType = GL_UNSIGNED_BYTE;
				break;

			case FMT_L8A8:
				internalFormat = GL_RG8;
				dataFormat = GL_RG;
				dataType = GL_UNSIGNED_BYTE;
				break;

			case FMT_LUM8:
				internalFormat = GL_R8;
				dataFormat = GL_RED;
				dataType = GL_UNSIGNED_BYTE;
				break;

			case FMT_INT8:
				internalFormat = GL_R8;
				dataFormat = GL_RED;
				dataType = GL_UNSIGNED_BYTE;
				break;

			case FMT_DXT1:
				internalFormat = sRGB ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT : GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
				//internalFormat =  GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
				dataFormat = GL_RGBA;
				dataType = GL_UNSIGNED_BYTE;
				break;

			case FMT_DXT5:
				internalFormat = ( sRGB && img->GetOpts().colorFormat != CFM_YCOCG_DXT5 && img->GetOpts().colorFormat != CFM_NORMAL_DXT5 )? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				//internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				dataFormat = GL_RGBA;
				dataType = GL_UNSIGNED_BYTE;
				break;

			case FMT_DEPTH: //SEA GL_DEPTH_COMPONENT16  GL_DEPTH_COMPONENT24  GL_DEPTH_COMPONENT32  GL_DEPTH_COMPONENT32F.
				internalFormat = GL_DEPTH_COMPONENT24;
				dataFormat = GL_DEPTH_COMPONENT;
				dataType = GL_UNSIGNED_INT; // GL_UNSIGNED_BYTE
				break;
			case FMT_DEPTH_STENCIL: //SEA GL_DEPTH24_STENCIL8  GL_DEPTH32F_STENCIL8
				internalFormat = GL_DEPTH24_STENCIL8;
				dataFormat = GL_DEPTH_STENCIL;
				dataType = GL_UNSIGNED_INT_24_8;
				break;

			case FMT_RG11F_B10F://SEA
				internalFormat = GL_R11F_G11F_B10F;
				dataFormat = GL_RGB;
				dataType = GL_FLOAT; // GL_UNSIGNED_INT_10F_11F_11F_REV
				break;
			case FMT_RG16F://SEA
				internalFormat = GL_RG16F;
				dataFormat = GL_RG;
				dataType = GL_HALF_FLOAT;
				break;
			case FMT_RGBA16F:
				internalFormat = GL_RGBA16F;
				dataFormat = GL_RGBA;
				dataType = GL_HALF_FLOAT;
				break;
			case FMT_RGBA32F:
				internalFormat = GL_RGBA32F;
				dataFormat = GL_RGBA;
				dataType = GL_FLOAT;
				break;
			case FMT_R32F:
				internalFormat = GL_R32F;
				dataFormat = GL_RED;
				dataType = GL_FLOAT;
				break;

			case FMT_X16:
				internalFormat = GL_INTENSITY16;
				dataFormat = GL_LUMINANCE;
				dataType = GL_UNSIGNED_SHORT;
				break;
			case FMT_Y16_X16:
				internalFormat = GL_LUMINANCE16_ALPHA16;
				dataFormat = GL_LUMINANCE_ALPHA;
				dataType = GL_UNSIGNED_SHORT;
				break;
			default:
				idLib::Error( "Unhandled image format %d in %s\n", img->GetOpts().format, img->GetName() );
		}
	}
};

/*
========================
idImage::SubImageUpload
========================
*/
void idImage::SubImageUpload( int mipLevel, int x, int y, int z, int width, int height, const void* pic, int pixelPitch ) const
{
	assert( x >= 0 && y >= 0 && mipLevel >= 0 && width >= 0 && height >= 0 && mipLevel < opts.numLevels );

	int compressedSize = 0;
	
	if( IsCompressed() )
	{
		assert( !( x & 3 ) && !( y & 3 ) );
		
		// compressed size may be larger than the dimensions due to padding to quads
		int quadW = ( width + 3 ) & ~3;
		int quadH = ( height + 3 ) & ~3;
		compressedSize = quadW * quadH * BitsForFormat( opts.format ) / 8;
		
		int padW = ( opts.width + 3 ) & ~3;
		int padH = ( opts.height + 3 ) & ~3;
		
		assert( x + width <= padW && y + height <= padH );
		// upload the non-aligned value, OpenGL understands that there
		// will be padding
		if( x + width > opts.width )
		{
			width = opts.width - x;
		}
		if( y + height > opts.height )
		{
			height = opts.height - x;
		}
	}
	else assert( x + width <= opts.width && y + height <= opts.height );

	glTextureObject_t tex( this );

	if( opts.textureType == TT_CUBIC ) {
		tex.uploadTarget += z;
	}

	glBindTexture( tex.target, tex.texnum );
	
	if( pixelPitch != 0 )
	{
		glPixelStorei( GL_UNPACK_ROW_LENGTH, pixelPitch );
	}
	
	if( opts.format == FMT_RGB565 )
	{
#if !defined(USE_GLES3)
		glPixelStorei( GL_UNPACK_SWAP_BYTES, GL_TRUE );
#endif
	}
	
#if defined(DEBUG) || defined(__ANDROID__)
	GL_CheckErrors();
#endif
	if( IsCompressed() )
	{
		glCompressedTexSubImage2D( tex.uploadTarget, mipLevel, x, y, width, height, tex.internalFormat, compressedSize, pic );
	}
	else
	{	
		// make sure the pixel store alignment is correct so that lower mips get created
		// properly for odd shaped textures - this fixes the mip mapping issues with
		// fonts
		int unpackAlignment = width * BitsForFormat( ( textureFormat_t )opts.format ) / 8;
		if( ( unpackAlignment & 3 ) == 0 )
		{
			glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
		}
		else {
			glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
		}
		
		glTexSubImage2D( tex.uploadTarget, mipLevel, x, y, width, height, tex.dataFormat, tex.dataType, pic );
	}
	
#if defined(DEBUG) || defined(__ANDROID__)
	GL_CheckErrors();
#endif
	
	if( opts.format == FMT_RGB565 )
	{
		glPixelStorei( GL_UNPACK_SWAP_BYTES, GL_FALSE );
	}
	if( pixelPitch != 0 )
	{
		glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
	}
}

/*
========================
idImage::SetPixel
========================
*/
void idImage::SetPixel( int mipLevel, int x, int y, const void* data, int dataSize )
{
	SubImageUpload( mipLevel, x, y, 0, 1, 1, data );
}

/*
========================
idImage::SetTexParameters
========================
*/
void idImage::SetTexParameters()
{
	glTextureObject_t tex;
	tex.DeriveTargetInfo( this );
		
	switch( filter )
	{
		case TF_DEFAULT:
			if( r_useTrilinearFiltering.GetBool() ) {
				glTexParameterf( tex.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
			} else {
				glTexParameterf( tex.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
			}
			glTexParameterf( tex.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			break;
		case TF_LINEAR:
			glTexParameterf( tex.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameterf( tex.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			break;
		case TF_NEAREST:
		case TF_NEAREST_MIPMAP:
			glTexParameterf( tex.target, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameterf( tex.target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			break;

		case TF_NEAREST_MIPMAP_NEAREST:
			glTexParameterf( tex.target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST );
			glTexParameterf( tex.target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			break;
		case TF_LINEAR_MIPMAP_NEAREST:
			glTexParameterf( tex.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
			glTexParameterf( tex.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			break;

		default:
			common->FatalError( "%s: bad texture filter %d", GetName(), filter );
	}
	
	if( glConfig.anisotropicFilterAvailable )
	{
		// only do aniso filtering on mip mapped images
		if( filter == TF_DEFAULT )
		{
			int aniso = r_maxAnisotropicFiltering.GetInteger();
			if( aniso > glConfig.maxTextureAnisotropy )
			{
				aniso = glConfig.maxTextureAnisotropy;
			}
			if( aniso < 0 )
			{
				aniso = 0;
			}
			glTexParameterf( tex.target, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso );
		}
		else
		{
			glTexParameterf( tex.target, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1 );
		}
	}
	
	// RB: disabled use of unreliable extension that can make the game look worse
	/*
	if( glConfig.textureLODBiasAvailable && ( usage != TD_FONT ) )
	{
		// use a blurring LOD bias in combination with high anisotropy to fix our aliasing grate textures...
		glTexParameterf( target, GL_TEXTURE_LOD_BIAS_EXT, 0.5 ); //r_lodBias.GetFloat() );
	}
	*/
	// RB end
	
	// set the wrap/clamp modes		GL_MIRRORED_REPEAT  GL_MIRROR_CLAMP_TO_EDGE(4.4)
	// Initially, glALL is set to GL_REPEAT.
	switch( repeat )
	{
		case TR_REPEAT:
		{
			glTexParameterf( tex.target, GL_TEXTURE_WRAP_S, GL_REPEAT );
			glTexParameterf( tex.target, GL_TEXTURE_WRAP_T, GL_REPEAT );
			if( opts.textureType == TT_3D ) {
				glTexParameterf( tex.target, GL_TEXTURE_WRAP_R, GL_REPEAT );
			}
		} 
		break;
		case TR_MIRROR:
		{
			glTexParameterf( tex.target, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT );
			glTexParameterf( tex.target, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT );
			if( opts.textureType == TT_3D ) {
				glTexParameterf( tex.target, GL_TEXTURE_WRAP_R, GL_MIRRORED_REPEAT );
			}
		}
		break;
		case TR_CLAMP_TO_ZERO:
		{
			float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			glTexParameterfv( tex.target, GL_TEXTURE_BORDER_COLOR, color );
			glTexParameterf( tex.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
			glTexParameterf( tex.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
			if( opts.textureType == TT_3D ) {
				glTexParameterf( tex.target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER );
			}
		} 
		break;
		case TR_CLAMP_TO_ZERO_ALPHA:
		{
			float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			glTexParameterfv( tex.target, GL_TEXTURE_BORDER_COLOR, color );
			glTexParameterf( tex.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
			glTexParameterf( tex.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
			if( opts.textureType == TT_3D ) {
				glTexParameterf( tex.target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER );
			}
		} 
		break;
		case TR_CLAMP:
		{
			glTexParameterf( tex.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameterf( tex.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			if( opts.textureType == TT_3D ) {
				glTexParameterf( tex.target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
			}
		} 
		break;
		default:
			common->FatalError( "%s: bad texture repeat %d", GetName(), repeat );
	}
	
	// RB: added shadow compare parameters for shadow map textures
	if( usage == TD_SHADOWMAP )
	//if( opts.format == FMT_DEPTH || opts.format == FMT_DEPTH_STENCIL )
	{
		glTexParameteri( tex.target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
		glTexParameteri( tex.target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
	}
}

/*
========================
idImage::EnableDepthCompareMode
========================
*/
void idImage::EnableDepthCompareModeOGL()
{
	glTextureObject_t tex;
	tex.DeriveTargetInfo( this );

	// GL_INTENSITY = dddd
	// GL_LUMINANCE = ddd1
	// GL_ALPHA = 000d
	// GL_RED = d001
	glTexParameteri( tex.target, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY );
	glTexParameteri( tex.target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
	glTexParameteri( tex.target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
}

/*
========================
idImage::AllocImage

	Every image will pass through this function. Allocates all the necessary MipMap levels for the
	Image, but doesn't put anything in them.

	This should not be done during normal game-play, if you can avoid it.
========================
*/
void idImage::AllocImage()
{
	GL_CheckErrors();
	PurgeImage();

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before OpenGL starts would miss
	// the generated texture
	if( !tr.IsOpenGLRunning() )
	{
		return;
	}

	// generate the texture number
	{
		GLuint texnum = GL_NONE;
		glGenTextures( 1, ( GLuint* )&texnum );
		assert( texnum != GL_NONE );
		apiObject = reinterpret_cast< void* >( texnum );
	}

	glTextureObject_t tex( this );
	glBindTexture( tex.target, tex.texnum );

	if( GLEW_KHR_debug )
	{
		idStrStatic<MAX_IMAGE_NAME/2> name;
		name.Format( "idImage(%p.%u)", this, tex.texnum );
		glObjectLabel( GL_TEXTURE, tex.texnum, name.Length(), name.c_str() );
	}

	//----------------------------------------------------
	// allocate all the mip levels with NULL data
	//----------------------------------------------------

	GLsizei w = idMath::Max( opts.width, 1 );
	GLsizei h = idMath::Max( opts.height, 1 );
	GLsizei depth = idMath::Max( opts.depth, 1 );
	const GLint numLevels = idMath::Max( opts.numLevels, 1 );
	const GLint numSamples = idMath::Max( opts.numSamples, 1 );

	if( glConfig.ARB_texture_storage )
	{
		if( tex.target == GL_TEXTURE_2D || tex.target == GL_TEXTURE_CUBE_MAP )
		{
			// 2D, CUBE
			glTexStorage2D( tex.target, numLevels, tex.internalFormat, w, ( tex.target != GL_TEXTURE_CUBE_MAP )? h : w );
		}
		else if( tex.target == GL_TEXTURE_2D_MULTISAMPLE )
		{
			// 2DMS
			glTexStorage2DMultisample( GL_TEXTURE_2D_MULTISAMPLE, numSamples, tex.internalFormat, w, h, GL_FALSE );
		}
		else if( tex.target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY )
		{
			// 2DMSArray
			glTexStorage3DMultisample( GL_TEXTURE_2D_MULTISAMPLE_ARRAY, numSamples, tex.internalFormat, w, h, depth, GL_FALSE );
		}
		else// if( tex.target == GL_TEXTURE_3D || tex.target == GL_TEXTURE_2D_ARRAY || tex.target == GL_TEXTURE_CUBE_MAP_ARRAY )
		{
			// 3D, 2DArray, CUBEArray
			glTexStorage3D( tex.target, numLevels, tex.internalFormat, w, h, depth );
		}
	} 
	else {
		if( opts.IsMultisampled() && opts.textureType == TT_2D )
		{
			assert( numLevels < 1 && opts.textureType == TT_2D );
			if( opts.IsArray() ) 
			{
				glTexImage3DMultisample( GL_TEXTURE_2D_MULTISAMPLE_ARRAY, numSamples, tex.internalFormat, w, h, depth, GL_FALSE );
			} 
			else {
				glTexImage2DMultisample( GL_TEXTURE_2D_MULTISAMPLE, numSamples, tex.internalFormat, w, h, GL_FALSE );
			}
		}
		else {
			assert( !opts.IsMultisampled() );

			GLenum numSides = 1;
			if( opts.textureType == TT_CUBIC )
			{
				h = w;
				numSides = 6;
				depth *= 6;
			}

			if( IsCompressed() )
			{
				const GLsizei formatBits = 16 * BitsForFormat( opts.format );

				if( opts.IsArray() )
				{
					for( GLint level = 0; level < numLevels; level++ )
					{
						const GLsizei compressedSize = ( ( ( w + 3 ) / 4 ) * ( ( h + 3 ) / 4 ) * formatBits ) / 8;
						glCompressedTexImage3D( tex.target, level, tex.internalFormat, w, h, depth, 0, compressedSize, NULL );
						w = idMath::Max( 1, w >> 1 );
						h = idMath::Max( 1, h >> 1 );
					}
				}
				else
				{
					for( GLint level = 0; level < numLevels; level++ )
					{
						const GLsizei compressedSize = ( ( ( w + 3 ) / 4 ) * ( ( h + 3 ) / 4 ) * formatBits ) / 8;
						for( GLenum side = 0; side < numSides; side++ ) {
							glCompressedTexImage2D( tex.uploadTarget + side, level, tex.internalFormat, w, h, 0, compressedSize, NULL );
						}
						w = idMath::Max( 1, w >> 1 );
						h = idMath::Max( 1, h >> 1 );
					}
				}
			}
			else {
				if( opts.IsArray() )
				{
					for( GLint level = 0; level < numLevels; level++ )
					{
						glTexImage3D( tex.target, level, tex.internalFormat, w, h, depth, 0, tex.dataFormat, tex.dataType, NULL );
						w = idMath::Max( 1, w >> 1 );
						h = idMath::Max( 1, h >> 1 );
					}
				}
				else
				{
					for( GLint level = 0; level < numLevels; level++ )
					{
						for( GLenum side = 0; side < numSides; side++ ) {
							glTexImage2D( tex.uploadTarget + side, level, tex.internalFormat, w, h, 0, tex.dataFormat, tex.dataType, NULL );
						}
						w = idMath::Max( 1, w >> 1 );
						h = idMath::Max( 1, h >> 1 );
					}
				}
			}
		}

		GL_CheckErrors();
	}

	// Non sampler parameters

	glTexParameteri( tex.target, GL_TEXTURE_BASE_LEVEL, 0 );
	glTexParameteri( tex.target, GL_TEXTURE_MAX_LEVEL, numLevels - 1 );

	// ALPHA, LUMINANCE, LUMINANCE_ALPHA, and INTENSITY have been removed
	// in OpenGL 3.2. In order to mimic those modes, we use the swizzle operators

	if( GetOpts().colorFormat == CFM_GREEN_ALPHA )
	{
		GLint swizzleMask[] = { GL_ONE, GL_ONE, GL_ONE, GL_GREEN };
		glTexParameteriv( tex.target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask );
	}
	else if( GetOpts().format == FMT_LUM8 )
	{
		GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
		glTexParameteriv( tex.target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask );
	}
	else if( GetOpts().format == FMT_L8A8 )
	{
		GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_GREEN };
		glTexParameteriv( tex.target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask );
	}
	else if( GetOpts().format == FMT_ALPHA )
	{
		GLint swizzleMask[] = { GL_ONE, GL_ONE, GL_ONE, GL_RED };
		glTexParameteriv( tex.target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask );
	}
	else if( GetOpts().format == FMT_INT8 )
	{
		GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_RED };
		glTexParameteriv( tex.target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask );
	}
	else {
		GLint swizzleMask[] = { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA };
		glTexParameteriv( tex.target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask );
	}
	
	// see if we messed anything up
	GL_CheckErrors();
	
	// Sampler parameters 
	//SEA: TODO real sampler
	SetTexParameters();
	
	GL_CheckErrors();
}

/*
========================
idImage::PurgeImage
========================
*/
void idImage::PurgeImage()
{
	if( IsLoaded() )
	{
		GLuint texnum[ 1 ] = { reinterpret_cast<GLuint>( apiObject ) };
		glDeleteTextures( 1, texnum );	// this should be the ONLY place it is ever called!
		apiObject = nullptr;
	}

	// clear all the current binding caches, so the next bind will do a real one
	for( int i = 0; i < MAX_MULTITEXTURE_UNITS; i++ )
	{
		for( int j = 0; j < target_Max; j++ )
		{
			backEnd.glState.tmu[ i ].currentTarget[ j ] = GL_NONE;
		}
	}
}

/*
========================
idImage::SetSamplerState
========================
*/
void idImage::SetSamplerState( textureFilter_t tf, textureRepeat_t tr )
{
	if( tf == filter && tr == repeat )
		return;

	filter = tf;
	repeat = tr;

	glTextureObject_t tex;
	tex.DeriveTargetInfo( this );
	glBindTexture( tex.target, tex.texnum );

	SetTexParameters();
}