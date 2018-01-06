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
static const int MAX_SSAO_BUFFERS = 2;
static const int MAX_HIERARCHICAL_ZBUFFERS = 6; // native resolution + 5 MIP LEVELS

#if 1
static	int shadowMapResolutions[MAX_SHADOWMAP_RESOLUTIONS] = { 2048, 1024, 512, 256, 128 };
#else
static	int shadowMapResolutions[MAX_SHADOWMAP_RESOLUTIONS] = { 1024, 1024, 1024, 1024, 1024 };
#endif

/*
=============================================

	idRenderDestination

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
			uint16		layer	= 0;
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
	void CreateFromImages( const targetList_t *, idImage* depth = nullptr, idImage* stencil = nullptr );

	// change existing target ( not normal operation, openGL specific )
	void SetTargetOGL( idImage * target, int iTarget = 0, int layer = 0, int level = 0 ); //SEA: temp
	
	///void Resize( int width, int height ); // for implicit only
	///void ResolveTarget();

	ID_INLINE void		SetDefault() { isDefault = true; }
	ID_INLINE bool		IsDefault() const { return isDefault; };
	ID_INLINE int		GetTargetWidth() const { return targetWidth; };
	ID_INLINE int		GetTargetHeight() const { return targetHeight; };
	ID_INLINE idImage *	GetColorImage() const { return targetList.list[ 0 ].image; };
	ID_INLINE idImage *	GetTargetImage( int iTarget ) const { return targetList.list[ iTarget ].image; };
	ID_INLINE int		GetTargetCount() const { return targetList.Count(); }
	ID_INLINE idImage *	GetDepthImage() const { return depthImage; };

	ID_INLINE const char * GetName() const { return name.c_str(); }
	ID_INLINE bool		IsValid() const { return( apiObject != nullptr ); }
	ID_INLINE void *	GetAPIObject() const { return apiObject; }

	// print resources info
	void				Print() const;

private:

	idStrStatic<64>		name;
	void *				apiObject;

	void				Clear();

	int					targetWidth;	// not wery useful for multitargets
	int					targetHeight;	// not wery useful for multitargets
	targetList_t		targetList;
	idImage * 			depthImage;
	idImage * 			stencilImage;
	int					boundSide;
	int					boundLevel;
	bool				isDefault;
	bool				isExplicit;

	/*
	The initial value is GL_FRONT for single-buffered contexts, and GL_BACK for double-buffered contexts.
	*/
};

/*
=============================================

	idRenderDestinationManager

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

	static void						ListRenderDestinations_f( const idCmdArgs& args );

public:

	idRenderDestination *			renderDestRenderToTexture;	// to replace copytex
	idRenderDestination *			renderDestDepthStencil;
	idRenderDestination *			renderDestCopyCurrentRender;
	idRenderDestination *			renderDestShadowOnePass;	// Depth24 x6_2DArray
	idRenderDestination *			renderDestShadow;	// Depth24 x6_2DArray
	idRenderDestination *			renderDestBaseHDR;	// HDRColor, DepthStencil
	//idRenderDestination *			renderDestBaseHDRQuarter;
	idRenderDestination *			renderDestBaseHDRsml64;
	idRenderDestination *			renderDestGBufferSml;	// ambien pass + global normals
	idRenderDestination *			renderDestBloomRender[ MAX_BLOOM_BUFFERS ];
	idRenderDestination *			renderDestAmbientOcclusion[ MAX_SSAO_BUFFERS ];
	idRenderDestination *			renderDestCSDepth[ MAX_HIERARCHICAL_ZBUFFERS ];
	//idRenderDestination *			renderDestGeometryBuffer;
	idRenderDestination *			renderDestSMAAEdges;
	idRenderDestination *			renderDestSMAABlend;


private:
	idStaticList<idRenderDestination *, TAG_RENDER>	renderDestList;

};

extern idRenderDestinationManager renderDestManager;



#endif // __RENDERDESTINATION_H__