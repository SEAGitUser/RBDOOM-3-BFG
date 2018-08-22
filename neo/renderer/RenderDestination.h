/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2014-2016 Robert Beckebans

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

#ifndef __RENDERDESTINATION_H__
#define __RENDERDESTINATION_H__

static const int MAX_SHADOWMAP_RESOLUTIONS = 5;
static const int MAX_BLOOM_BUFFERS = 2;
static const int MAX_BLOOM_BUFFER_MIPS = 6;  // native resolution + 5 MIP LEVELS
static const int MAX_SSAO_BUFFERS = 2;
static const int MAX_HIERARCHICAL_ZBUFFERS = 6; // native resolution + 5 MIP LEVELS

#if 1
static	int shadowMapResolutions[ MAX_SHADOWMAP_RESOLUTIONS ] = { 2048, 1024, 512, 256, 128 };
#else
static	int shadowMapResolutions[ MAX_SHADOWMAP_RESOLUTIONS ] = { 1024, 1024, 1024, 1024, 1024 };
#endif

/*
=============================================

	idRenderDestination

	Does not support stencil-only render targets

=============================================
*/

// RenderDestination_OGL_PC
// GL_SetRenderDestination( idODSObject<idRenderDestination> )
// GL_ResolveTarget( idODSObject<idRenderDestination>, resolveTarget_t )
/*
enum renderClass_t {
	TC_DEPTH,
	TC_RGBA,
	TC_RGBA_DEPTH,
	TC_FLOAT_RGBA,
	TC_FLOAT_RGBA_DEPTH,
	TC_FLOAT16_RGBA,
	TC_FLOAT16_RGBA_DEPTH,
	TC_NUM_RENDER_CLASS
};

enum resolveTarget_t {
	RESOLVE_TARGET_COLOR0,
	RESOLVE_TARGET_COLOR1,
	RESOLVE_TARGET_COLOR2,
	RESOLVE_TARGET_COLOR3,
	RESOLVE_TARGET_DEPTH
};
*/

class idRenderDestination {
public:
	static const int	MAX_RENDERDEST_NAME = 64;
	static const int	MAX_TARGETS = 8;
	class targetList_t {
		friend class idRenderDestination;
		struct target_t {
			idImage *	image = nullptr;
			uint16		layer = 0;
			uint16		mipLevel = 0;
			uint16		prevWidth = 0;
			uint16		prevHeight = 0;
		};
		idStaticList<target_t, MAX_TARGETS> list;
	public:
		ID_INLINE targetList_t() { Clear(); }
		ID_INLINE ~targetList_t() { Clear(); }
		ID_INLINE int Count() const { return list.Num(); }
		ID_INLINE void Clear() { list.Clear(); list.FillZero(); }
		ID_INLINE void Append( idImage * image, int mipLevel = 0 )
		{
			target_t target;
			target.image = image;
			target.mipLevel = mipLevel;
			target.prevWidth = image->GetUploadWidth();
			target.prevHeight = image->GetUploadHeight();
			list.Append( target );
		}
	};

	idRenderDestination( const char *name );
	~idRenderDestination();

	/// Create( const idStr & name, int width, int height, renderClass_t texClass, const idImageOpts & opts );

	//Note: For 3D/2DArray/Cube/CubeArray only layered rendering configuration for now.
	void CreateFromImages( const targetList_t *, idImage * depthStencil = nullptr, int depthStencilLod = 0 );

	// Change existing depth or depth_stencil target ( opengl specific ). To remove!
	void SetDepthStencilGL( idImage * target, int layer = 0, int level = 0 );

	///void Resize( int width, int height ); // for implicit only
	///void ResolveTarget();

	ID_INLINE void		SetDefault() { isDefault = true; }
	ID_INLINE bool		IsDefault() const { return isDefault; };

	// This is the biggest color target width if present, or depthstencil width.
	ID_INLINE int		GetTargetWidth() const { return targetWidth; };
	ID_INLINE float		GetTargetInvWidth() const { return( 1.0f / targetWidth ); };
	// This is the biggest color target height if present, or depthstencil height.
	ID_INLINE int		GetTargetHeight() const { return targetHeight; };
	ID_INLINE float		GetTargetInvHeight() const { return( 1.0f / targetHeight ); };
	// Vec4( w, h, 1/w, 1/h )
	ID_INLINE idVec4	GetSizes() const { return idVec4( targetWidth, targetHeight, 1.0 / targetWidth, 1.0 / targetHeight ); }

	ID_INLINE idImage *	GetColorImage() const { return targetList.list[ 0 ].image; };
	ID_INLINE idImage *	GetTargetImage( int iTarget ) const { return targetList.list[ iTarget ].image; };
	ID_INLINE int		GetTargetCount() const { return targetList.Count(); }
	ID_INLINE idImage *	GetDepthStencilImage() const { return depthStencilImage; };

	ID_INLINE const char * GetName() const { return name.c_str(); }
	ID_INLINE bool		IsValid() const { return( apiObject != nullptr ); }
	ID_INLINE void *	GetAPIObject() const { return apiObject; }

	// print resources info
	void				Print() const;

private:

	idStrStatic<64>		name;
	void *				apiObject;

	void				Clear();

	int					targetWidth;
	int					targetHeight;
	targetList_t		targetList;
	idImage * 			depthStencilImage;
	int					depthStencilLod;
	bool				isDefault;
	bool				isExplicit;
};

/*
=============================================

	idRenderDestinationManager

	_shadowAtlas_d
	_shadowAtlas_vsm
	_ambient_atlas
	_ambient_sh_coef
	_particlesLightAtlas
	_viewDepth
	_gui
	_guiDepth
	_viewColor%d
	_distortion%d
	_viewColorR11G11B10F%d
	_velocityDepth
	_velocityTileMax
	_accumulationBuffer%d
	_accumulationBufferOpaque
	_ssr
	_viewColorR8
	_viewColorScaled%d%d
	_autoExposure%d
	_autoExposureLum%d
	_viewDepthScaledR16F_0
	_viewDepthScaledR16F_1
	_AO
	_AmbOcclAccBuffer0
	_AmbOcclAccBuffer1
	_dofLayer01
	_dofAccBuffer0
	_dofAccBuffer1
	_dofLayer00
	_transparencyAccumulationBufferHalf
	_transparencyDepth
	_augmentDepth
	_augmentLayer%d
	_augment
	_envProbeArray

	_frontColor
	_specularBuffer
	_motionVector
	_frontNormal


	gpuParticleRender.
	gpuParticleSimulate.
	guiRender.

	r_renderThreadStackSizeKB( "r_renderThreadStackSizeKB", "4194304", , "" )

	Enable color blind modes. 1: Protanopia, 2: Deuteranopia, 3: Tritanopia. ( tbd: Protanomaly, Deuteranomaly, Tritanomaly ). Disabled by default = 0

=============================================
*/
class idRenderDestinationManager {
public:
	idRenderDestinationManager();

	void							Init();
	void							Shutdown();
	void							Update();

	idRenderDestination *			CreateRenderDestination( const char* name );
	void							DestroyRenderDestination( idRenderDestination * );

	idRenderDestination *			FinedByName( const char * name ) const;
	idImage *						FinedRenderTargetImageByName( const char * name ) const;

	static void						ListRenderDestinations_f( const idCmdArgs& );

public:

	idImage *						viewColorHDRImage = nullptr;		// RGBA16F
	idImage *						viewColorLDRImage = nullptr;		// RGBA8
	idImage *						viewDepthStencilImage = nullptr;	// D24_S8
	idRenderDestination *			renderDestBaseHDR = nullptr;		// HDRColor + DepthStencil
	idRenderDestination *			renderDestBaseLDR = nullptr;		// LDRColor + DepthStencil

	idImage *						shadowBufferImage[ MAX_SHADOWMAP_RESOLUTIONS ] = { nullptr };
	idRenderDestination *			renderDestShadowOnePass[ MAX_SHADOWMAP_RESOLUTIONS ] = { nullptr };	// Depth24 x6_2DArray

	//idImage *						shadowMaskImage = nullptr;
	//idRenderDestination *			renderDestShadowMask = nullptr;

	//idImage *						viewVelocityImage = nullptr;

	idImage *						viewColorHDRQuarterImage = nullptr;
	//idRenderDestination *			renderDestBaseHDRQuarter = nullptr;
	idImage *						viewColorHDRImageSml = nullptr;
	idImage *						viewColorHDRImageSmlPrev = nullptr;
	idRenderDestination *			renderDestBaseHDRsml64 = nullptr;

	idImage *						viewColorsImage = nullptr;		// RGBA8 diffuse + specular
	idImage *						viewNormalImage = nullptr;		// RGB10_A2 global normals
	idRenderDestination *			renderDestGBuffer = nullptr;	// Colors + GlobalNormals + DepthStencil
	idRenderDestination *			renderDestGBufferSml = nullptr;	// AmbienColor + GlobalNormals + DepthStencil

	idImage *						hiZbufferImage = nullptr;	// zbuffer with mip maps to accelerate screen space ray tracing
	idImage *						ambientOcclusionImage[ MAX_SSAO_BUFFERS ] = { nullptr }; // contain AO and bilateral filtering keys
	idRenderDestination *			renderDestAmbientOcclusion[ MAX_SSAO_BUFFERS ] = { nullptr };
	idRenderDestination *			renderDestCSDepth[ MAX_HIERARCHICAL_ZBUFFERS ] = { nullptr };
	//idRenderDestination *			renderDestGeometryBuffer;

	//idImage *						lumaImage = nullptr;
	//idRenderDestination *			renderDestLuminance = nullptr;
	//idImage *						autoExposureImage[ 2 ] = { nullptr };
	//idRenderDestination *			renderDestAutoExposure[ 2 ] = { nullptr };

	idImage *						bloomRenderImage[ MAX_BLOOM_BUFFERS ] = { nullptr };
	idRenderDestination *			renderDestBloomRender[ MAX_BLOOM_BUFFERS ] = { nullptr };
	//idImage *						bloomRenderMipsImage = nullptr;
	idImage *						bloomRenderMipsImage[ MAX_BLOOM_BUFFER_MIPS ] = { nullptr };
	idRenderDestination *			renderDestBloomMip[ MAX_BLOOM_BUFFER_MIPS ] = { nullptr };

	//idImage *						viewColorLDRImagePrev = nullptr;

	idImage *						smaaEdgesImage = nullptr;
	idRenderDestination *			renderDestSMAAEdges = nullptr;
	idImage *						smaaBlendImage = nullptr;
	idRenderDestination *			renderDestSMAABlend = nullptr;

	idRenderDestination *			renderDestRenderToTexture = nullptr;	// to replace copytex
	idRenderDestination *			renderDestDepthStencil = nullptr;
	idRenderDestination *			renderDestCopyCurrentRender = nullptr;

	//idRenderDestination *			renderDestShadow = nullptr;	// Depth24 x6_2DArray

	idRenderDestination *			renderDestStagingColor = nullptr;	// for misc usage
	idRenderDestination *			renderDestStagingDepth = nullptr;	// for misc usage

private:
	idStaticList<idImage *, 32> renderTargetImgList;
	idStaticList<idRenderDestination *, TAG_RENDER>	renderDestList;
	bool bIsInitialized = false;

	idImage * CreateRenderTargetImage( const char * Name,
									   textureType_t Type, int Width, int Height, int Layers, int Levels, int Samples,
									   textureFormat_t Format, textureFilter_t Filter, textureWrap_t WrapMode, bool CmpMode = false );
};

extern idRenderDestinationManager renderDestManager;

//typedef idAutoPtr<idRenderDestination> idRenderDest;

#endif // __RENDERDESTINATION_H__