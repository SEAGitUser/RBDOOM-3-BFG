/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2014 Robert Beckebans

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

#ifndef __IMAGEOPTS_H__
#define __IMAGEOPTS_H__

enum textureType_t
{
	TT_DISABLED,
	TT_2D,
	TT_CUBIC,
	TT_3D, //SEA
};

/*
================================================
The internal *Texture Format Types*, ::textureFormat_t, are:
================================================
*/
enum textureFormat_t
{
	FMT_NONE,
	
	//------------------------
	// Standard color image formats
	//------------------------
	
	FMT_RGBA8,			// 32 bpp
	FMT_XRGB8,			// 32 bpp
	// FMT_XRGB8<-FMT_ARGB8
	//------------------------
	// Alpha channel only
	//------------------------
	
	// Alpha ends up being the same as L8A8 in our current implementation, because straight
	// alpha gives 0 for color, but we want 1.
	FMT_ALPHA,
	
	//------------------------
	// Luminance replicates the value across RGB with a constant A of 255
	// Intensity replicates the value across RGBA
	//------------------------
	
	FMT_L8A8,			// 16 bpp
	// FMT_RG8
	FMT_LUM8,			//  8 bpp
	FMT_INT8,			//  8 bpp
	
	//------------------------
	// Compressed texture formats
	//------------------------
	
	FMT_DXT1,			// 4 bpp
	FMT_DXT5,			// 8 bpp
	
	//------------------------
	// Depth buffer formats
	//------------------------
	
	FMT_DEPTH,			// 24 bpp
	///FMT_DEPTH_STENCIL,	// 24+8 bpp
	//------------------------
	//
	//------------------------
	
	// FMT_X32F
	// FMT_Y16F_X16F

	FMT_X16,			// 16 bpp
	FMT_Y16_X16,		// 32 bpp
	FMT_RGB565,			// 16 bpp

	FMT_DEPTH_STENCIL,	//SEA 24+8 bpp
	FMT_RG11F_B10F,		//SEA 32 bpp
	FMT_RG16F,			//SEA 32 bpp
	/*
GL_RGB5_A1:		5 bits each for RGB, 1 for Alpha. This format is generally trumped by compressed formats (see below), which give greater than 16-bit quality in much less storage than 16-bits of color.
GL_RGB10_A2:	10 bits each for RGB, 2 for Alpha. This can be a useful format for framebuffers, if you do not need a high-precision destination alpha value. It carries more color depth, thus preserving subtle gradations. They can also be used for normals, though there is no signed-normalized version, so you have to do the conversion manually. It is also a required format (see below), so you can count on it being present.
GL_RGB10_A2UI:	10 bits each for RGB, 2 for Alpha, as unsigned integers. There is no signed integral version.
	
	*/
	
	// RB: don't change above for legacy .bimage compatibility
	FMT_ETC1_RGB8_OES,	// 4 bpp
	FMT_RGBA16F,		// 64 bpp
	FMT_RGBA32F,		// 128 bpp
	FMT_R32F,			// 32 bpp
	// RB end
};

/*
================================================
DXT5 color formats
================================================
*/
enum textureColor_t
{
	CFM_DEFAULT,			// RGBA
	// CFM_RGBA_HQ_DXT5,
	// CFM_NORMAL_HQ_DXT5,
	// CFM_YCOCG_DXT5,
	// CFM_YCOCGA_DXT5,
	// CFM_YCOCGA_HQ_DXT5,
	// CFM_RGB_HQ_DXT1

	CFM_NORMAL_DXT5,		// XY format and use the fast DXT5 compressor
	CFM_YCOCG_DXT5,			// convert RGBA to CoCg_Y format
	CFM_GREEN_ALPHA,		// Copy the alpha channel to green
	
	// RB: don't change above for legacy .bimage compatibility
	CFM_YCOCG_RGBA8,
	// RB end
};
#if 0
struct textureUsage_t //SEA
{
	TU_DEFAULT,
	TU_STATIC,
	TU_DYNAMIC,
	TU_STAGING
};

struct textureOptions_t //SEA
{
	TO_NONE,
	TO_GENERATE_MIPS
};

enum textureBindType_t //SEA
{
	TB_SHADER,
	TB_RENDER_TARGET,
	TB_DEPTH_STENCIL_TARGET
};
#endif
/*
================================================
idImageOpts hold parameters for texture operations.
================================================
*/
class idImageOpts
{
public:
	idImageOpts();
	
	bool	operator==( const idImageOpts& opts );
	
	//---------------------------------------------------
	// these determine the physical memory size and layout
	//---------------------------------------------------
	
	textureType_t		textureType;
	textureFormat_t		format;
	textureColor_t		colorFormat;
	int					width;
	int					height;			// not needed for cube maps
	int					depth; //SEA
	int					numLevels;		// if 0, will be 1 for NEAREST / LINEAR filters, otherwise based on size
	int					numSamples; //SEA
	//int					packedTail; //SEA
	//bool					readback; //SEA
	//bool					linear; //SEA
	//bool					partiallyResident; //SEA
	//bool					cubeFilter; //SEA
	//bool					overlayMemory; //SEA
	//bool					startPurged; //SEA
	//int					struct_pad; //SEA
	//textureBindType_t		textureBindType; //SEA
	//textureUsage_t		textureUsage; //SEA
	//textureOptions_t		textureOptions; //SEA
	//idTextureSampler *	sampler; //SEA
	//int					struct_pad2; //SEA
	bool				gammaMips;		// if true, mips will be generated with gamma correction
	bool				readback;		// 360 specific - cpu reads back from this texture, so allocate with cached memory

	bool IsArray() const { return( depth > 1 ); }
	bool IsMultisampled() const { return( numSamples > 1 ); }

	void Init2D( uint32 width, uint32 height );
	void Init2DArray( uint32 width, uint32 height, uint32 depth );
	void InitCube( uint32 width );
	void InitCubeArray( uint32 width, uint32 depth );
};

/*
========================
idImageOpts::idImageOpts
========================
*/
ID_INLINE idImageOpts::idImageOpts()
{
	format			= FMT_NONE;
	colorFormat		= CFM_DEFAULT;
	width			= 0;
	height			= 0;
	depth			= 1;
	numLevels		= 0;
	numSamples		= 1;
	textureType		= TT_2D;
	gammaMips		= false;
	readback		= false;
};

/*
========================
idImageOpts::operator==
========================
*/
ID_INLINE bool idImageOpts::operator==( const idImageOpts& opts )
{
	return ( memcmp( this, &opts, sizeof( *this ) ) == 0 );
}

#endif