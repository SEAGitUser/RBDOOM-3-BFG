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

/*
====================================================================

IMAGE

idImage have a one to one correspondance with GL/DX/GCM textures.

No texture is ever used that does not have a corresponding idImage.

IMAGE_SPECULAR,
IMAGE_DIFFUSE,
IMAGE_NORMAL,
IMAGE_TEST_FBR,
IMAGE_TEST_VBR,
imageFlags_t

====================================================================
*/

static const int	MAX_TEXTURE_LEVELS = 14;

// How is this texture used?  Determines the storage and color format
enum textureUsage_t
{
	TD_SPECULAR,			// may be compressed, and always zeros the alpha channel
	TD_DIFFUSE,				// may be compressed
	TD_DEFAULT,				// generic RGBA texture (particles, etc...)
	TD_BUMP,				// may be compressed with 8 bit lookup
	TD_FONT,				// Font image
	TD_LIGHT,				// Light image
	TD_LOOKUP_TABLE_MONO,	// Mono lookup table (including alpha)
	TD_LOOKUP_TABLE_ALPHA,	// Alpha lookup table with a white color channel
	TD_LOOKUP_TABLE_RGB1,	// RGB lookup table with a solid white alpha
	TD_LOOKUP_TABLE_RGBA,	// RGBA lookup table
	TD_COVERAGE,			// coverage map for fill depth pass when YCoCG is used
	TD_DEPTH,				// depth buffer copy for motion blur
	// RB begin
	TD_SHADOWMAP,			// depth buffer for shadow mapping

	TD_RGBA16F,
	TD_RGBA32F,
	TD_R32F,
	// RB end
};

enum textureLayout_t
{
	IMG_LAYOUT_2D,				// not a cube map
	IMG_LAYOUT_CUBE_NATIVE,		// cube map _px, _nx, _py, etc, directly sent to GL
	IMG_LAYOUT_CUBE_CAMERA,		// cube map _forward, _back, etc, rotated and flipped as needed before sending to GL
};

// moved from image.h for default parm
enum textureFilter_t {
	TF_LINEAR,
	TF_NEAREST,
	TF_NEAREST_MIPMAP,		// RB: no linear interpolation but explicit mip-map levels for hierarchical depth buffer
	TF_DEFAULT,				// use the user-specified r_textureFilter
//SEA
	TF_NEAREST_MIPMAP_NEAREST,
	TF_LINEAR_MIPMAP_NEAREST,
	//TF_TRILINEAR,
};

typedef enum textureRepeat_t {
	TR_REPEAT,
	TR_CLAMP,
	TR_CLAMP_TO_ZERO,		// guarantee 0,0,0,255 edge for projected textures
	TR_CLAMP_TO_ZERO_ALPHA,	// guarantee 0 alpha edge for projected textures
//SEA
	//TR_CLAMP_S,
	//TR_CLAMP_T,
	//TR_CLAMP_TO_BORDER,
	TR_MIRROR,
} textureWrap_t;

typedef struct samplerOptions_t
{
	textureFilter_t filter				= TF_DEFAULT;
	textureWrap_t	wrap				= TR_REPEAT;
	//int32			border				= 0.0;
	//float			lodBias				= 0.0;
	//int32			lodMinClamp			= 0;
	//int32			lodMaxClamp			= 0;
	//int32			aniso				= 1;
	//bool			forceBias			= false;
	bool			depthCompareMode	= false;

	bool operator == ( const samplerOptions_t & other ) const
	{
		return( memcmp( this, &other, sizeof( samplerOptions_t ) ) == 0 );
	}

} idSamplerOpts;

enum imageState_t {
	IS_PURGED,
	IS_BACKGROUND_READING,
	IS_READY
};

enum imageFileType_t {
	TGA,
	PNG,
	JPG
};

#if 0
struct idFileProps
{
	void	Clear();
	int64 	timestamp;
	bool 	writable;
};

struct imageProperties_t {
	mtr;
	x;
	y;
	width;
	height;
	area;
};

class idMaterialMapping
{
	materialName;
	materialChecksum;
	materialTimestamp;
	x;
	y;
	width;
	height;
	scaleBias;
	specularFile;
	coverFile;
	diffuseFile;
	bumpFile;
	powerFile;
};

class idMaterialMap
{
	GetImageDimensions()
	GetSpecularImageData()
	GetCoverImageData()
	GetDiffuseImageData()
	GetBumpImageData()
	GetPowerImageData()
	GetSpecularImageFileData()
	GetCoverImageFileData()
	GetDiffuseImageFileData()
	GetBumpImageFileData()
	GetPowerImageFileData()
private:
	int name;
	int width;
	int height;
	bool skinFileTimeStamp;
	idList< idMaterialMapping > mappings;
	GetRenderParmImageFileProps();
};

class idImageData {
	idImageData();
	~idImageData();

	int width;
	int height;
	union {
		byte * data;
		float * floatData;
	};
	void LoadImageProgram();
	void WritePNG( const char *filename );
	void WriteTGA( const char *filename );
	void Resample();
	void MipMapOneStep();
	void Subset();
	void NewSize();
	void PointSample();
	void BilinearSample();
	void ReplaceRect();
};

class idAtlasResource {
	idAtlasResource();
	~idAtlasResource();

	void LoadResource();
	void ReloadIfStale();
	void Print();
	void List();
	void resourceList;
	void GetResourceList();
	void GetScaleBias();
	void GetWidth();
	void GetHeight();
	void FreeData();

	int binaryTimestamp;
	int scaleBias;
	int img_tiles_wide;
	int img_tiles_high;
	int img_x;
	int img_y;
	int img_w;
	int img_h;
};

class idImageAtlas {
	void ClearAtlas();
	void GetAtlasUsage();
	int ATLAS_WIDTH_BITS
	int ATLAS_HEIGHT_BITS
	int ATLAS_MIP_LEVELS
	int ATLAS_TILE_WIDTH_BITS
	int ATLAS_TILE_HEIGHT_BITS
	int ATLAS_WIDTH
	int ATLAS_HEIGHT
	int ATLAS_TILE_WIDTH
	int ATLAS_TILE_HEIGHT
	int ATLAS_TILES_WIDE
	int ATLAS_TILES_HIGH
	int ATLAS_TILES

	tileAlloc;
	atlas;
};

textureBindType_t{
	TB_SHADER,
	TB_RENDER_TARGET,
	TB_DEPTH_STENCIL_TARGET
}

textureUsage_t{
	TU_DEFAULT,
	TU_STATIC,
	TU_DYNAMIC,
	TU_STAGING
}

textureOptions_t{
	TO_NONE,
	TO_GENERATE_MIPS
}

#endif

// calculate the number of mipmap levels for NPOT texture
ID_INLINE int GetMipCountForResolution( float screen_width, float screen_height )
{
	return 1 + ( int )floorf( log2f( fmaxf( screen_width, screen_height ) ) );
}

#include "ImageOpts.h"
#include "BinaryImage.h"

#define	MAX_IMAGE_NAME	256

class idImage {
	friend class idRenderDestination;
	friend class idDeclRenderParm;
public:
	idImage( const char* name );

	ID_INLINE const char * 	GetName() const { return imgName; }

	// Should be called at least once
	void		SetSamplerState( textureFilter_t, textureRepeat_t );

	// used by callback functions to specify the actual data
	// data goes from the bottom to the top line of the image, as OpenGL expects it
	// These perform an implicit Bind() on the current texture unit
	// FIXME: should we implement cinematics this way, instead of with explicit calls?
	void		GenerateImage( const byte* pic, int width, int height, int depth, textureFilter_t, textureRepeat_t, textureUsage_t );
	void		GenerateCubeImage( const byte* pic[6], int size, int depth, textureFilter_t, textureUsage_t );
	void		Generate3DImage( const byte* pic, int width, int height, int depth, textureFilter_t, textureRepeat_t, textureUsage_t );

	void		UploadScratch( const byte* pic, int width, int height );

	// estimates size of the GL image based on dimensions and storage type
	int			StorageSize() const;

	// print a one line summary of the image
	void		Print() const;

	// check for changed timestamp on disk and reload if necessary
	void		Reload( bool force );

	ID_INLINE void	AddReference() { refCount++; };

	void			MakeDefault( textureType_t = TT_2D );	// fill with a grid pattern

	ID_INLINE const idImageOpts & GetOpts() const { return opts; }
	ID_INLINE int	GetUploadWidth() const { return opts.width; }
	ID_INLINE int	GetUploadHeight() const { return opts.height; }
	ID_INLINE int	GetLayerCount() const { return opts.depth; }

	ID_INLINE void	SetReferencedOutsideLevelLoad() { referencedOutsideLevelLoad = true; }
	ID_INLINE void	SetReferencedInsideLevelLoad() { levelLoadReferenced = true; }

	void		ActuallyLoadImage( bool fromBackEnd );
	//---------------------------------------------
	// Platform specific implementations
	//---------------------------------------------

	void		AllocImage( const idImageOpts&, const samplerOptions_t& );

	// Deletes the texture object, but leaves the structure so it can be reloaded
	// or resized.
	void		PurgeImage();

	// z is 0 for 2D textures, 0 - 5 for cube maps, and 0 - uploadDepth for 3D textures. Only
	// one plane at a time of 3D textures can be uploaded. The data is assumed to be correct for
	// the format, either bytes, halfFloats, floats, or DXT compressed. The data is assumed to
	// be in OpenGL RGBA format, the consoles may have to reorganize. pixelPitch is only needed
	// when updating from a source subrect. Width, height, and dest* are always in pixels, so
	// they must be a multiple of four for dxt data.
	void		SubImageUpload( int mipLevel, int destX, int destY, int destZ,
								int width, int height, const void* data,
								int pixelPitch = 0 ) const;

	// SetPixel is assumed to be a fast memory write on consoles, degenerating to a
	// SubImageUpload on PCs.  Used to update the page mapping images.
	// We could remove this now, because the consoles don't use the intermediate page mapping
	// textures now that they can pack everything into the virtual page table images.
	void		SetPixel( int mipLevel, int x, int y, const void* data, int dataSize );

	// some scratch images are dynamically resized based on the display window size.  This
	// simply purges the image and recreates it if the sizes are different, so it should not be
	// done under any normal circumstances, and probably not at all on consoles.
	// Returns true if resized.
	bool		Resize( int width, int height, int depth = 1 );

	ID_INLINE bool		IsCompressed() const { return opts.IsCompressed(); }
	ID_INLINE bool		IsArray() const { return opts.IsArray(); }
	ID_INLINE bool		IsMultisampled() const { return opts.IsMultisampled(); }
	ID_INLINE bool		IsDepth() const { return opts.IsDepth(); }
	ID_INLINE bool		IsDepthStencil() const { return opts.IsDepthStencil(); }
	ID_INLINE bool		UsageShadowMap() const { return usage == TD_SHADOWMAP; }


	ID_INLINE bool		IsLoaded() const { return apiObject != nullptr; }
	ID_INLINE void *	GetAPIObject() const { return apiObject; }

	void				SetTexParameters();	// update aniso and trilinear
	void				EnableDepthCompareModeOGL();

	static void			GetGeneratedName( idStr& _name, const textureUsage_t&, const textureLayout_t& );

	static int			BitsForFormat( textureFormat_t format );

	//idImageData		GetImageData();
	// void ModifySampler( samplerOptions_t );
	// imageState_t		ImageState

private:
	friend class idImageManager;

	void				AllocImage();
	void				DeriveOpts();

	// parameters that define this image
	idStr				imgName;				// game path, including extension (except for cube maps), may be an image program
	void ( *generatorFunction )( idImage* image );	// NULL for files
	textureLayout_t		layout;					// If this is a cube map, and if so, what kind
	textureUsage_t		usage;					// Used to determine the type of compression to use

	idImageOpts			opts;					// Parameters that determine the storage method

	//textureFilter_t		filter;
	//textureRepeat_t		repeat;
	samplerOptions_t	sampOpts;

	bool				referencedOutsideLevelLoad;
	bool				levelLoadReferenced;	// for determining if it needs to be purged
	bool				defaulted;				// true if the default image was generated because a file couldn't be loaded
	ID_TIME_T			sourceFileTime;			// the most recent of all images used in creation, for reloadImages command
	ID_TIME_T			binaryFileTime;			// the time stamp of the binary file

	int					refCount;				// overall ref count

	void *				apiObject;
};

ID_INLINE idImage::idImage( const char* name ) : imgName( name )
{
	apiObject = nullptr;
	generatorFunction = NULL;
	//filter = TF_DEFAULT;
	//repeat = TR_REPEAT;
	sampOpts = samplerOptions_t();
	usage = TD_DEFAULT;
	layout = IMG_LAYOUT_2D;

	referencedOutsideLevelLoad = false;
	levelLoadReferenced = false;
	defaulted = false;
	sourceFileTime = FILE_NOT_FOUND_TIMESTAMP;
	binaryFileTime = FILE_NOT_FOUND_TIMESTAMP;
	refCount = 0;
}


// data is RGBA
void	R_WriteTGA( const char* filename, const byte* data, int width, int height, bool flipVertical = false, const char* basePath = "fs_savepath" );
// data is in top-to-bottom raster order unless flipVertical is set

// RB begin
void	R_WritePNG( const char* filename, const byte* data, int bytesPerPixel, int width, int height, bool flipVertical = false, const char* basePath = "fs_savepath" );
// RB end

class idImageManager {
public:

	idImageManager() {
		insideLevelLoad = false;
		preloadingMapImages = false;
	}

	void				Init();
	void				Shutdown();

	// If the exact combination of parameters has been asked for already, an existing
	// image will be returned, otherwise a new image will be created.
	// Be careful not to use the same image file with different filter / repeat / etc parameters
	// if possible, because it will cause a second copy to be loaded.
	// If the load fails for any reason, the image will be filled in with the default
	// grid pattern.
	// Will automatically execute image programs if needed.
	idImage* 			ImageFromFile( const char* name, textureFilter_t, textureRepeat_t, textureUsage_t, textureLayout_t = IMG_LAYOUT_2D );

	// look for a loaded image, whatever the parameters
	idImage* 			GetImage( const char* name ) const;

	// look for a loaded image, whatever the parameters
	idImage* 			GetImageWithParameters( const char* name, textureFilter_t, textureRepeat_t, textureUsage_t, textureLayout_t ) const;

	// The callback will be issued immediately, and later if images are reloaded or vid_restart
	// The callback function should call one of the idImage::Generate* functions to fill in the data
	idImage* 			ImageFromFunction( const char* name, void ( *generatorFunction )( idImage* image ) );

	// scratch images are for internal renderer use.  ScratchImage names should always begin with an underscore
	idImage* 			ScratchImage( const char* name, idImageOpts*, textureFilter_t, textureRepeat_t, textureUsage_t );

	// purges all the images before a vid_restart
	void				PurgeAllImages();

	// reloads all apropriate images after a vid_restart
	void				ReloadImages( bool all );

	// unbind all textures from all texture units
	void				UnbindAll();

	// Called only by renderSystem::BeginLevelLoad
	void				BeginLevelLoad();

	// Called only by renderSystem::EndLevelLoad
	void				EndLevelLoad();

	void				Preload( const idPreloadManifest& manifest, const bool& mapPreload );

	// Loads unloaded level images
	int					LoadLevelImages( bool pacifier );

	// used to clear and then write the dds conversion batch file
	void				StartBuild();
	void				FinishBuild( bool removeDups = false );

	void				PrintMemInfo( MemInfo_t* mi );

	// built-in images
	void				CreateIntrinsicImages();
	idImage * 			defaultImage;
	idImage * 			defaultImageCube;
	idImage * 			flatNormalMap;				// 128 128 255 in all pixels
	idImage * 			alphaNotchImage;			// 2x1 texture with just 1110 and 1111 with point sampling
	idImage * 			whiteImage;					// full of 0xff
	idImage * 			blackImage;					// full of 0x00
	idImage * 			grayImage;
	idImage * 			noFalloffImage;				// all 255, but zero clamped
	idImage * 			fogImage;					// increasing alpha is denser fog
	idImage * 			fogEnterImage;				// adjust fogImage alpha based on terminator plane

	idImage *			jitterImage1;				// shadow jitter
	idImage *			jitterImage4;
	idImage *			jitterImage16;
	idImage *			grainImage1;
	idImage *			randomImage256;
	idImage *			heatmap5Image;
	idImage *			heatmap7Image;

	idImage *			smaaAreaImage;
	idImage *			smaaSearchImage;

	idImage * 			scratchImage;
	idImage * 			scratchImage2;
	idImage * 			accumImage;
	idImage * 			currentRenderImage;				// for SS_POST_PROCESS shaders
	idImage * 			originalCurrentRenderImage;		// currentRenderImage before any changes for stereo rendering
	idImage * 			loadingIconImage;				// loading icon must exist always
	idImage * 			hellLoadingIconImage;			// loading icon must exist always

	//--------------------------------------------------------

	idImage * 			CreateImage( const char* name );
	idImage * 			CreateStandaloneImage( const char* name );
	void				DestroyStandaloneImage( idImage* ) const;

	bool				ExcludePreloadImage( const char* name );

	idList<idImage *, TAG_IDLIB_LIST_IMAGE>	images;
	idHashIndex			imageHash;

	bool				insideLevelLoad;			// don't actually load images now
	bool				preloadingMapImages;		// unless this is set
};

extern idImageManager *	renderImageManager;		// pointer to global list for the rest of the system

extern int MakePowerOfTwo( int num );

/*
====================================================================

IMAGEPROCESS

FIXME: make an "imageBlock" type to hold byte*,width,height?
====================================================================
*/

byte* R_Dropsample( const byte* in, int inwidth, int inheight, int outwidth, int outheight );
byte* R_ResampleTexture( const byte* in, int inwidth, int inheight, int outwidth, int outheight );
byte* R_MipMapWithAlphaSpecularity( const byte* in, int width, int height );
byte* R_MipMapWithGamma( const byte* in, int width, int height );
byte* R_MipMap( const byte* in, int width, int height );

// these operate in-place on the provided pixels
void R_BlendOverTexture( byte* data, int pixelCount, const byte blend[4] );
void R_HorizontalFlip( byte* data, int width, int height );
void R_VerticalFlip( byte* data, int width, int height );
void R_RotatePic( byte* data, int width );
void R_ApplyCubeMapTransforms( int i, byte* data, int size );

/*
====================================================================

IMAGEFILES

====================================================================
*/

void R_LoadImage( const char* name, byte** pic, int* width, int* height, ID_TIME_T* timestamp, bool makePowerOf2 );
// pic is in top to bottom raster format
bool R_LoadCubeImages( const char* cname, textureLayout_t extensions, byte* pic[6], int* size, ID_TIME_T* timestamp );

/*
====================================================================

IMAGEPROGRAM

====================================================================
*/

void R_LoadImageProgram( const char* name, byte** pic, int* width, int* height, ID_TIME_T* timestamp, textureUsage_t* usage = NULL );
const char* R_ParsePastImageProgram( idLexer& src );

