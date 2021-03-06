/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Robert Beckebans
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

#ifndef __MATERIAL_H__
#define __MATERIAL_H__

/*
===============================================================================

	Material

===============================================================================
*/

class idImage;
class idCinematic;
class idUserInterface;
enum textureRepeat_t;
class idDeclRenderParm;
class idDeclRenderProg;

struct decalInfo_t {
	int		stayTime;		// msec for no change
	int		fadeTime;		// msec to fade vertex colors over
	float	start[ 4 ];		// vertex color at spawn (possibly out of 0.0 - 1.0 range, will clamp after calc)
	float	end[ 4 ];		// vertex color at fade-out (possibly out of 0.0 - 1.0 range, will clamp after calc)
};

enum deform_t {
	DFRM_NONE,
	DFRM_SPRITE,
	DFRM_TUBE,
	DFRM_FLARE,
	DFRM_EXPAND,
	DFRM_MOVE,
	DFRM_EYEBALL,
	DFRM_PARTICLE,
	DFRM_PARTICLE2,
	DFRM_TURB
};

enum dynamicidImage_t {
	DI_STATIC,
	DI_SCRATCH,		// video, screen wipe, etc
	DI_CUBE_RENDER,
	DI_MIRROR_RENDER,
	DI_XRAY_RENDER,
	DI_REMOTE_RENDER
};

// note: keep opNames[] in sync with changes
enum expOpType_t {
	OP_TYPE_ADD,
	OP_TYPE_SUBTRACT,
	OP_TYPE_MULTIPLY,
	OP_TYPE_DIVIDE,
	OP_TYPE_MOD,
	OP_TYPE_TABLE,
	OP_TYPE_GT,
	OP_TYPE_GE,
	OP_TYPE_LT,
	OP_TYPE_LE,
	OP_TYPE_EQ,
	OP_TYPE_NE,
	OP_TYPE_AND,
	OP_TYPE_OR,
	OP_TYPE_SOUND
};

enum expRegister_t {
	EXP_REG_TIME,

	EXP_REG_PARM0,
	EXP_REG_PARM1,
	EXP_REG_PARM2,
	EXP_REG_PARM3,
	EXP_REG_PARM4,
	EXP_REG_PARM5,
	EXP_REG_PARM6,
	EXP_REG_PARM7,
	EXP_REG_PARM8,
	EXP_REG_PARM9,
	EXP_REG_PARM10,
	EXP_REG_PARM11,

	EXP_REG_GLOBAL0,
	EXP_REG_GLOBAL1,
	EXP_REG_GLOBAL2,
	EXP_REG_GLOBAL3,
	EXP_REG_GLOBAL4,
	EXP_REG_GLOBAL5,
	EXP_REG_GLOBAL6,
	EXP_REG_GLOBAL7,

	EXP_REG_NUM_PREDEFINED
};

struct expOp_t {
	expOpType_t		opType;
	int				a, b, c;
};

struct colorStage_t {
	int				registers[ 4 ];
};

enum texgen_t {
	TG_EXPLICIT,
	TG_DIFFUSE_CUBE,
	TG_REFLECT_CUBE,
	TG_SKYBOX_CUBE,
	TG_WOBBLESKY_CUBE,
	TG_SCREEN,			// screen aligned, for mirrorRenders and screen space temporaries
	TG_SCREEN2,
	TG_GLASSWARP
};

struct textureStage_t {
	idCinematic* 		cinematic;
	idImage* 			image;
	texgen_t			texgen;
	bool				hasMatrix;
	int					matrix[ 2 ][ 3 ];	// we only allow a subset of the full projection matrix

	// dynamic image variables
	dynamicidImage_t	dynamic;
	int					width, height;
	int					dynamicFrameCount;
};

// the order BUMP / DIFFUSE / SPECULAR is necessary for interactions to draw correctly on low end cards
enum stageLighting_t {
	SL_AMBIENT,						// execute after lighting
	SL_BUMP,
	SL_DIFFUSE,
	SL_SPECULAR,
	SL_COVERAGE,
};

// cross-blended terrain textures need to modulate the color by
// the vertex color to smoothly blend between two textures
enum stageVertexColor_t {
	SVC_IGNORE,
	SVC_MODULATE,
	//SVC_MODULATE_ALPHA,
	SVC_INVERSE_MODULATE
};

/*
=====================================================================
=====================================================================
*/

struct stageVector_t {
	int							registers[ 4 ];
	const idDeclRenderParm *	parmDecl;
};
const int MAX_STAGE_VECTORS = 32;

struct stageTexture_t {
	idImage *					image;
	const idDeclRenderParm *	parmDecl;
};
const int MAX_STAGE_TEXTURES = 16;

struct stageTextureMatrix_t {
	int							matrix[ 2 ][ 3 ];
	const idDeclRenderParm *	parmDecl_s;
	const idDeclRenderParm *	parmDecl_t;
};
const int MAX_STAGE_TEXTUREMATRICES = 3;

struct stageParseData_t {
	stageParseData_t()
	{
		memset( this, 0, sizeof( stageParseData_t ) );
	}

	uint32					numVectors;
	stageVector_t			vectors[ MAX_STAGE_VECTORS ];

	uint32					numTextures;
	stageTexture_t			textures[ MAX_STAGE_TEXTURES ];

	uint32					numTextureMatrices;
	stageTextureMatrix_t	textureMatrices[ MAX_STAGE_TEXTUREMATRICES ];
};

struct newShaderStage_t
{
	stageVector_t *			vectors;
	stageTexture_t *		textures;
	//stageTextureMatrix_t *	textureMatrices;

	int32		program;
	uint16		numVectors;
	uint16		numTextures;
	//uint16		numTextureMatrices;

	//int16				numVertexParms;
	//int16				numFragmentProgramImages;

	//int					vertexParms[ MAX_VERTEX_PARMS ][ 4 ];	// evaluated register indexes
	//idImage * 			fragmentProgramImages[ MAX_FRAGMENT_IMAGES ];

	void Clear();
};
static const uint32 newShaderStageSize = sizeof( newShaderStage_t );

/*
=====================================================================
	idMaterialStage
=====================================================================
*/
typedef struct materialStage_t
{
	int					conditionRegister;	// if registers[conditionRegister] == 0, skip stage
	stageLighting_t		lighting;			// determines which passes interact with lights
	uint64				drawStateBits;
	colorStage_t		color;
	bool				hasAlphaTest;
	int					alphaTestRegister;
	textureStage_t		texture;
	stageVertexColor_t	vertexColor;
	bool				ignoreAlphaTest;	// this stage should act as translucent, even
											// if the surface is alpha tested
	float				privatePolygonOffset;	// a per-stage polygon offset

	newShaderStage_t *	newStage;			// vertex / fragment program based stage

	// ------------------------------------
#if 0
	uint64						drawStateBits;
	int							conditionRegister;	// if registers[conditionRegister] == 0, skip stage

	stageLighting_t				lighting;			// determines which passes interact with lights

	const stageTexture_t *		rpStageTex;
	const stageTextureMatrix_t * rpTextureMatrix;

	const stageVector_t *		rpAlphaTest;
	const stageVector_t *		rpVertexColorMAD;
	const stageVector_t *		rpColor;

	float						privatePolygonOffset;	// a per-stage polygon offset
	bool						ignoreAlphaTest;	// this stage should act as translucent, even if the surface is alpha tested
	bool						hasAlphaTest;
#endif
	// ------------------------------------

	bool SkipStage( const float * registers ) const
	{
		return( !registers[ conditionRegister ] );
	}

	void GetColorParm( const float * registers, idVec4 & out ) const
	{
		out[ 0 ] = registers[ color.registers[ 0/*SHADERPARM_RED*/ ] ];
		out[ 1 ] = registers[ color.registers[ 1/*SHADERPARM_GREEN*/ ] ];
		out[ 2 ] = registers[ color.registers[ 2/*SHADERPARM_BLUE*/ ] ];
		out[ 3 ] = registers[ color.registers[ 3/*SHADERPARM_ALPHA*/ ] ];
	}
	float GetColorParmAlpha( const float * registers ) const
	{
		return registers[ color.registers[ 3/*SHADERPARM_ALPHA*/ ] ];
	}
	float GetAlphaTestValue( const float * registers ) const
	{
		return registers[ alphaTestRegister ];
	}

} idMaterialStage;
static const uint32 idMaterialStageSize = sizeof( idMaterialStage );

enum materialCoverage_t {
	MC_BAD,
	MC_OPAQUE,			// completely fills the triangle, will have black drawn on fillDepthBuffer
	MC_PERFORATED,		// may have alpha tested holes
	MC_TRANSLUCENT		// blended with background
/*
	MC_BAD,
	MC_INVISIBLE,
	MC_PERFORATED,
	MC_TRANSLUCENT,
	MC_OPAQUE,
	MC_PERFORATED_FOR_MODELFADE,
*/
};

enum materialSort_t {
	SS_SUBVIEW = -3,	// mirrors, viewscreens, etc
	SS_GUI = -2,	// guis
	SS_BAD = -1,
	SS_OPAQUE,			// opaque

	SS_PORTAL_SKY,

	SS_DECAL,			// scorch marks, etc.

	SS_FAR,
	SS_MEDIUM,			// normal translucent
	SS_CLOSE,

	SS_ALMOST_NEAREST,	// gun smoke puffs

	SS_NEAREST,			// screen blood blobs

	SS_POST_PROCESS = 100	// after a screen copy to texture
};

enum cullType_t {
	CT_FRONT_SIDED,
	CT_BACK_SIDED,
	CT_TWO_SIDED
};

// these don't effect per-material storage, so they can be very large
const int MAX_SHADER_STAGES = 256;

const int MAX_TEXGEN_REGISTERS = 4;

const int MAX_ENTITY_SHADER_PARMS = 12;
const int MAX_GLOBAL_SHADER_PARMS = 12;	// ? this looks like it should only be 8

// material flags
enum materialFlags_t {
	MF_DEFAULTED		= BIT( 0 ),
	MF_POLYGONOFFSET	= BIT( 1 ),
	MF_NOSHADOWS		= BIT( 2 ),
	MF_FORCESHADOWS		= BIT( 3 ),
	MF_NOSELFSHADOW		= BIT( 4 ),
	MF_NOPORTALFOG		= BIT( 5 ),	 // this fog volume won't ever consider a portal fogged out
	MF_EDITOR_VISIBLE	= BIT( 6 ),	 // in use (visible) per editor
	// motorsep 11-23-2014; material LOD keys that define what LOD iteration the surface falls into
	MF_LOD1_SHIFT		= 7,
	MF_LOD1				= BIT( 7 ),	 // motorsep 11-24-2014; material flag for LOD1 iteration
	MF_LOD2				= BIT( 8 ),	 // motorsep 11-24-2014; material flag for LOD2 iteration
	MF_LOD3				= BIT( 9 ),	 // motorsep 11-24-2014; material flag for LOD3 iteration
	MF_LOD4				= BIT( 10 ), // motorsep 11-24-2014; material flag for LOD4 iteration
	MF_LOD_PERSISTENT	= BIT( 11 )	 // motorsep 11-24-2014; material flag for persistent LOD iteration
};

// contents flags, NOTE: make sure to keep the defines in doom_defs.script up to date with these!
enum contentsFlags_t {
	CONTENTS_SOLID				= BIT( 0 ),	// an eye is never valid in a solid
	CONTENTS_OPAQUE				= BIT( 1 ),	// blocks visibility (for ai)
	CONTENTS_WATER				= BIT( 2 ),	// used for water
	CONTENTS_PLAYERCLIP			= BIT( 3 ),	// solid to players
	CONTENTS_MONSTERCLIP		= BIT( 4 ),	// solid to monsters
	CONTENTS_MOVEABLECLIP		= BIT( 5 ),	// solid to moveable entities
	CONTENTS_IKCLIP				= BIT( 6 ),	// solid to IK
	CONTENTS_BLOOD				= BIT( 7 ),	// used to detect blood decals
	CONTENTS_BODY				= BIT( 8 ),	// used for actors
	CONTENTS_PROJECTILE			= BIT( 9 ),	// used for projectiles
	CONTENTS_CORPSE				= BIT( 10 ),	// used for dead bodies
	CONTENTS_RENDERMODEL		= BIT( 11 ),	// used for render models for collision detection
	CONTENTS_TRIGGER			= BIT( 12 ),	// used for triggers
	CONTENTS_AAS_SOLID			= BIT( 13 ),	// solid for AAS
	CONTENTS_AAS_OBSTACLE		= BIT( 14 ),	// used to compile an obstacle into AAS that can be enabled/disabled
	CONTENTS_FLASHLIGHT_TRIGGER = BIT( 15 ),	// used for triggers that are activated by the flashlight

	// contents used by utils
	CONTENTS_AREAPORTAL			= BIT( 20 ),	// portal separating renderer areas
	CONTENTS_NOCSG				= BIT( 21 ),	// don't cut this brush with CSG operations in the editor

	CONTENTS_REMOVE_UTIL = ~( CONTENTS_AREAPORTAL | CONTENTS_NOCSG )
};

// surface types
const int NUM_SURFACE_BITS = 4;
const int MAX_SURFACE_TYPES = 1 << NUM_SURFACE_BITS;

enum surfTypes_t {
	SURFTYPE_NONE,	// default type
	SURFTYPE_METAL,
	SURFTYPE_STONE,
	SURFTYPE_FLESH,
	SURFTYPE_WOOD,
	SURFTYPE_CARDBOARD,
	SURFTYPE_LIQUID,
	SURFTYPE_GLASS,
	SURFTYPE_PLASTIC,
	SURFTYPE_RICOCHET,
	SURFTYPE_10,
	SURFTYPE_11,
	SURFTYPE_12,
	SURFTYPE_13,
	SURFTYPE_14,
	SURFTYPE_15
};

// surface flags
enum surfaceFlags_t {
	SURF_TYPE_BIT0		= BIT( 0 ),	// encodes the material type (metal, flesh, concrete, etc.)
	SURF_TYPE_BIT1		= BIT( 1 ),	// "
	SURF_TYPE_BIT2		= BIT( 2 ),	// "
	SURF_TYPE_BIT3		= BIT( 3 ),	// "
	SURF_TYPE_MASK		= ( 1 << NUM_SURFACE_BITS ) - 1,

	SURF_NODAMAGE		= BIT( 4 ),	// never give falling damage
	SURF_SLICK			= BIT( 5 ),	// effects game physics
	SURF_COLLISION		= BIT( 6 ),	// collision surface
	SURF_LADDER			= BIT( 7 ),	// player can climb up this surface
	SURF_NOIMPACT		= BIT( 8 ),	// don't make missile explosions
	SURF_NOSTEPS		= BIT( 9 ),	// no footstep sounds
	SURF_DISCRETE		= BIT( 10 ), // not clipped or merged by utilities
	SURF_NOFRAGMENT		= BIT( 11 ), // dmap won't cut surface at each bsp boundary
	SURF_NULLNORMAL		= BIT( 12 )	// renderbump will draw this surface as 0x80 0x80 0x80, which
									// won't collect light from any angle
};

/*
================================================================================================

	idDeclRenderMaterial

================================================================================================
*/
class idMaterial : public idDecl {
public:
	idMaterial();
	virtual				~idMaterial();

	virtual size_t		Size() const;
	virtual bool		SetDefaultText();
	virtual const char* DefaultDefinition() const;
	virtual bool		Parse( const char* text, const int textLength, bool allowBinaryVersion );
	virtual void		FreeData();
	virtual void		Print() const;

	//BSM Nerve: Added for material editor
	bool				Save( const char* fileName = NULL );

	// returns the internal image name for stage 0, which can be used
	// for the renderer CaptureRenderToImage() call.
	const char *		ImageName() const;

	void				ReloadImages( bool force ) const;

	// returns number of stages this material contains
	const int			GetNumStages() const { return numStages; }

	// if the material is simple, all that needs to be known are
	// the images for drawing.
	// These will either all return valid images, or all return NULL
	idImage * 			GetFastPathBumpImage() const { return fastPathBumpImage; };
	idImage * 			GetFastPathDiffuseImage() const { return fastPathDiffuseImage; };
	idImage * 			GetFastPathSpecularImage() const { return fastPathSpecularImage; };

	// get a specific stage
	ID_INLINE const materialStage_t * GetStage( const int index ) const
	{
		assert( index >= 0 && index < numStages );
		return &stages[ index ];
	}
	// Check if all stages of a material have been conditioned off
	ID_INLINE bool ConditionedOff( const float* registers ) const
	{
		int stage = 0;
		for(/**/; stage < GetNumStages(); stage++ )
		{
			// check the stage enable condition
			if( registers[ GetStage( stage )->conditionRegister ] != 0 )
				break;
		}
		return( stage == GetNumStages() );
	}

	// get the first bump map stage, or NULL if not present.
	// used for bumpy-specular
	const materialStage_t * GetBumpStage() const;

	// returns true if the material will draw anything at all.  Triggers, portals,
	// etc, will not have anything to draw.  A not drawn surface can still castShadow,
	// which can be used to make a simplified shadow hull for a complex object set
	// as noShadow
	ID_INLINE bool		IsDrawn() const { return( numStages > 0 || entityGui != 0 || gui != NULL ); }

	// returns true if the material will draw any non light interaction stages
	ID_INLINE bool		HasAmbient() const { return( numAmbientStages > 0 ); }

	// returns true if material has a gui
	ID_INLINE bool		HasGui() const { return( entityGui != 0 || gui != NULL ); }

	// returns true if the material will generate another view, either as
	// a mirror or dynamic rendered image
	ID_INLINE bool		HasSubview() const { return hasSubview; }

	// returns true if the material will generate shadows, not making a
	// distinction between global and no-self shadows
	ID_INLINE bool		SurfaceCastsShadow() const { return TestMaterialFlag( MF_FORCESHADOWS ) || !TestMaterialFlag( MF_NOSHADOWS ); }

	// returns true if the material will generate interactions with fog/blend lights
	// All non-translucent surfaces receive fog unless they are explicitly noFog
	ID_INLINE bool		ReceivesFog() const { return ( IsDrawn() && !noFog && coverage != MC_TRANSLUCENT ); }

	// returns true if the material will generate interactions with normal lights
	// Many special effect surfaces don't have any bump/diffuse/specular
	// stages, and don't interact with lights at all
	ID_INLINE bool		ReceivesLighting() const { return numAmbientStages != numStages; }

	// returns true if the material should generate interactions on sides facing away
	// from light centers, as with noshadow and noselfshadow options
	ID_INLINE bool		ReceivesLightingOnBackSides() const { return ( materialFlags & ( MF_NOSELFSHADOW | MF_NOSHADOWS ) ) != 0; }

	// Standard two-sided triangle rendering won't work with bump map lighting, because
	// the normal and tangent vectors won't be correct for the back sides.  When two
	// sided lighting is desired. typically for alpha tested surfaces, this is
	// addressed by having CleanupModelSurfaces() create duplicates of all the triangles
	// with apropriate order reversal.
	ID_INLINE bool		ShouldCreateBackSides() const { return shouldCreateBackSides; }

	// characters and models that are created by a complete renderbump can use a faster
	// method of tangent and normal vector generation than surfaces which have a flat
	// renderbump wrapped over them.
	ID_INLINE bool		UseUnsmoothedTangents() const { return unsmoothedTangents; }

	// by default, monsters can have blood overlays placed on them, but this can
	// be overrided on a per-material basis with the "noOverlays" material command.
	// This will always return false for translucent surfaces
	ID_INLINE bool		AllowOverlays() const { return allowOverlays; }

	// MC_OPAQUE, MC_PERFORATED, or MC_TRANSLUCENT, for interaction list linking and
	// dmap flood filling
	// The depth buffer will not be filled for MC_TRANSLUCENT surfaces
	// FIXME: what do nodraw surfaces return?
	ID_INLINE materialCoverage_t Coverage() const { return coverage; }

	// returns true if this material takes precedence over other in coplanar cases
	ID_INLINE bool		HasHigherDmapPriority( const idMaterial& other ) const
	{
		return ( IsDrawn() && !other.IsDrawn() ) || ( Coverage() < other.Coverage() );
	}

	// returns a idUserInterface if it has a global gui, or NULL if no gui
	ID_INLINE idUserInterface *	GlobalGui() const { return gui; }

	// a discrete surface will never be merged with other surfaces by dmap, which is
	// necessary to prevent mutliple gui surfaces, mirrors, autosprites, and some other
	// special effects from being combined into a single surface
	// guis, merging sprites or other effects, mirrors and remote views are always discrete
	ID_INLINE bool		IsDiscrete() const
	{
		return( entityGui || gui || deform != DFRM_NONE || sort == SS_SUBVIEW || ( surfaceFlags & SURF_DISCRETE ) != 0 );
	}

	// Normally, dmap chops each surface by every BSP boundary, then reoptimizes.
	// For gigantic polygons like sky boxes, this can cause a huge number of planar
	// triangles that make the optimizer take forever to turn back into a single
	// triangle.  The "noFragment" option causes dmap to only break the polygons at
	// area boundaries, instead of every BSP boundary.  This has the negative effect
	// of not automatically fixing up interpenetrations, so when this is used, you
	// should manually make the edges of your sky box exactly meet, instead of poking
	// into each other.
	ID_INLINE bool		NoFragment() const { return ( surfaceFlags & SURF_NOFRAGMENT ) != 0; }

	//------------------------------------------------------------------
	// light shader specific functions, only called for light entities

	// lightshader option to fill with fog from viewer instead of light from center
	ID_INLINE bool		IsFogLight() const { return fogLight; }

	// perform simple blending of the projection, instead of interacting with bumps and textures
	ID_INLINE bool		IsBlendLight() const { return blendLight; }

	// an ambient light has non-directional bump mapping and no specular
	ID_INLINE bool		IsAmbientLight() const { return ambientLight; }

	// implicitly no-shadows lights (ambients, fogs, etc) will never cast shadows
	// but individual light entities can also override this value
	ID_INLINE bool		LightCastsShadows() const
	{
		return TestMaterialFlag( MF_FORCESHADOWS ) || ( !fogLight && !ambientLight && !blendLight && !TestMaterialFlag( MF_NOSHADOWS ) );
	}

	// fog lights, blend lights, ambient lights, etc will all have to have interaction
	// triangles generated for sides facing away from the light as well as those
	// facing towards the light.  It is debatable if noshadow lights should effect back
	// sides, making everything "noSelfShadow", but that would make noshadow lights
	// potentially slower than normal lights, which detracts from their optimization
	// ability, so they currently do not.
	ID_INLINE bool		LightEffectsBackSides() const { return fogLight || ambientLight || blendLight; }

	// NULL unless an image is explicitly specified in the shader with "lightFalloffShader <image>"
	ID_INLINE idImage *	LightFalloffImage() const { return lightFalloffImage; }

	//------------------------------------------------------------------

	// returns the renderbump command line for this shader, or an empty string if not present
	ID_INLINE const char * GetRenderBump() const { return renderBump; };

	// set specific material flag(s)
	ID_INLINE void		SetMaterialFlag( const int flag ) const { materialFlags |= flag; }

	// clear specific material flag(s)
	ID_INLINE void		ClearMaterialFlag( const int flag ) const { materialFlags &= ~flag; }

	// test for existance of specific material flag(s)
	ID_INLINE bool		TestMaterialFlag( const int flag ) const { return ( materialFlags & flag ) != 0; }

	// get content flags
	ID_INLINE const int	GetContentFlags() const { return contentFlags; }

	// get surface flags
	ID_INLINE const int	GetSurfaceFlags() const { return surfaceFlags; }

	// gets name for surface type (stone, metal, flesh, etc.)
	ID_INLINE const surfTypes_t	GetSurfaceType() const { return static_cast< surfTypes_t >( surfaceFlags & SURF_TYPE_MASK ); }

	// get material description
	ID_INLINE const char * GetDescription() const { return desc.c_str(); }

	// get sort order
	ID_INLINE const float GetSort() const { return sort; }

	ID_INLINE const int	GetStereoEye() const { return stereoEye; }

	// this is only used by the gui system to force sorting order
	// on images referenced from tga's instead of materials.
	// this is done this way as there are 2000 tgas the guis use
	ID_INLINE void		SetSort( float s ) const { sort = s; };

	// DFRM_NONE, DFRM_SPRITE, etc
	ID_INLINE deform_t	GetDeformType() const { return deform; }

	// flare size, expansion size, etc
	ID_INLINE const int	GetDeformRegister( int index ) const { return deformRegisters[ index ]; }

	// particle system to emit from surface and table for turbulent
	ID_INLINE const idDecl * GetDeformDecl() const { return deformDecl; }

	// currently a surface can only have one unique texgen for all the stages
	texgen_t			Texgen() const;

	// wobble sky parms
	ID_INLINE const int * GetTexGenRegisters() const { return texGenRegisters; }

	// get cull type
	ID_INLINE const cullType_t	GetCullType() const { return cullType; }

	ID_INLINE float		GetEditorAlpha() const { return editorAlpha; }

	ID_INLINE int		GetEntityGui() const { return entityGui; }

	ID_INLINE decalInfo_t GetDecalInfo() const { return decalInfo; }

	// spectrums are used for "invisible writing" that can only be
	// illuminated by a light of matching spectrum
	ID_INLINE int		Spectrum() const { return spectrum; }

	ID_INLINE float		GetPolygonOffset() const { return polygonOffset; }

	ID_INLINE float		GetSurfaceArea() const { return surfaceArea; }
	ID_INLINE void		AddToSurfaceArea( float area ) { surfaceArea += area; }

	//------------------------------------------------------------------

	// returns the length, in milliseconds, of the videoMap on this material,
	// or zero if it doesn't have one
	int					CinematicLength() const;
	void				CloseCinematic() const;
	void				ResetCinematicTime( int time ) const;
	int					GetCinematicStartTime() const;
	void				UpdateCinematic( int time ) const;
	// RB: added because we can't rely on the FFmpeg feedback how long a video really is
	bool                CinematicIsPlaying() const;
	// RB end

	//------------------------------------------------------------------

	// gets an image for the editor to use
	idImage * 			GetEditorImage() const;
	int					GetImageWidth() const;
	int					GetImageHeight() const;

	void				SetGui( const char* _gui ) const;

	//------------------------------------------------------------------

	// returns number of registers this material contains
	ID_INLINE const int	GetNumRegisters() const { return numRegisters; }

	// Regs should point to a float array large enough to hold GetNumRegisters() floats.
	// FloatTime is passed in because different entities, which may be running in parallel,
	// can be in different time groups.
	void				EvaluateRegisters(
		float * 		registers,
		const float		localShaderParms[],
		const float		globalShaderParms[],
		const float		floatTime,
		class idSoundEmitter* ) const;

	// if a material only uses constants (no entityParm or globalparm references), this
	// will return a pointer to an internal table, and EvaluateRegisters will not need
	// to be called.  If NULL is returned, EvaluateRegisters must be used.
	ID_INLINE const float * ConstantRegisters() const { return constantRegisters; };

	ID_INLINE bool		SuppressInSubview() const { return suppressInSubview; };
	ID_INLINE bool		IsPortalSky() const { return portalSky; };
	void				AddReference();

	// motorsep 11-23-2014; material LOD keys that define what LOD iteration the surface falls into
	// lod1 - lod4 defines several levels of LOD
	// persistentLOD specifies the LOD iteration that still being rendered, even after the camera is beyond the distance at which LOD iteration should not be rendered

	ID_INLINE bool		IsLOD() const { return ( materialFlags & ( MF_LOD1 | MF_LOD2 | MF_LOD3 | MF_LOD4 ) ) != 0; }
	// foresthale 2014-11-24: added IsLODVisibleForDistance method
	ID_INLINE bool		IsLODVisibleForDistance( float distance, float lodBase ) const
	{
		int bit = ( materialFlags & ( MF_LOD1 | MF_LOD2 | MF_LOD3 | MF_LOD4 ) ) >> MF_LOD1_SHIFT;
		float m1 = lodBase * ( bit >> 1 );
		float m2 = lodBase * bit;
		return distance >= m1 && ( distance < m2 || ( materialFlags & ( MF_LOD_PERSISTENT ) ) );
	}

	// Utilities

	static void GetTexMatrixFromStage( const float* registers, const textureStage_t*, float matrix[ 16 ] );
	static void GetTexMatrixFromStage( const float* registers, const textureStage_t*, class idRenderMatrix & );

private:
	// parse the entire material
	void				CommonInit();
	void				ParseMaterial( idLexer& src );
	bool				MatchToken( idLexer& src, const char* match );
	void				ParseSort( idLexer& src );
	void				ParseStereoEye( idLexer& src );
	void				ParseBlend( idLexer& src, materialStage_t* stage );
	void				ParseVertexParm( idLexer& src, stageParseData_t* newStage );
	void				ParseVertexParm2( idLexer& src, stageParseData_t* newStage );
	void				ParseFragmentMap( idLexer& src, stageParseData_t* newStage );
	void				ParseStage( idLexer& src, const textureRepeat_t trpDefault ); // TR_REPEAT
	void				ParseDeform( idLexer& src );
	void				ParseDecalInfo( idLexer& src );
	bool				CheckSurfaceParm( idToken* token );
	int					GetExpressionConstant( float f );
	int					GetExpressionTemporary();
	expOp_t *			GetExpressionOp();
	int					EmitOp( int a, int b, expOpType_t opType );
	int					ParseEmitOp( idLexer& src, int a, expOpType_t opType, int priority );
	int					ParseTerm( idLexer& src );
	int					ParseExpressionPriority( idLexer& src, int priority );
	int					ParseExpression( idLexer& src );
	void				ClearStage( materialStage_t* ss );
	int					NameToSrcBlendMode( const idStr& name );
	int					NameToDstBlendMode( const idStr& name );
	void				MultiplyTextureMatrix( textureStage_t* ts, int registers[ 2 ][ 3 ] );	// FIXME: for some reason the const is bad for gcc and Mac
	void				SortInteractionStages();
	void				AddImplicitStages( const textureRepeat_t trpDefault ); // TR_REPEAT
	void				CheckForConstantRegisters();
	void				SetFastPathImages();

private:
	idStr				desc;				// description
	idStr				renderBump;			// renderbump command options, without the "renderbump" at the start

	idImage *			lightFalloffImage;	// only for light shaders

	idImage * 			fastPathBumpImage;	// if any of these are set, they all will be
	idImage * 			fastPathDiffuseImage;
	idImage * 			fastPathSpecularImage;

	int					entityGui;			// draw a gui with the idUserInterface from the renderEntityParms_t
	// non zero will draw gui, gui2, or gui3 from renderEnitty_t
	mutable idUserInterface*	gui;			// non-custom guis are shared by all users of a material

	bool				noFog;				// surface does not create fog interactions

	int					spectrum;			// for invisible writing, used for both lights and surfaces

	float				polygonOffset;

	int					contentFlags;		// content flags
	int					surfaceFlags;		// surface flags
	mutable int			materialFlags;		// material flags

	decalInfo_t			decalInfo;

	mutable	float		sort;				// lower numbered shaders draw before higher numbered
	int					stereoEye;
	deform_t			deform;
	int					deformRegisters[ 4 ];		// numeric parameter for deforms
	const idDecl *		deformDecl;			// for surface emitted particle deforms and tables

	int					texGenRegisters[ MAX_TEXGEN_REGISTERS ];	// for wobbleSky

	materialCoverage_t	coverage;
	cullType_t			cullType;			// CT_FRONT_SIDED, CT_BACK_SIDED, or CT_TWO_SIDED
	bool				shouldCreateBackSides;

	bool				fogLight;
	bool				blendLight;
	bool				ambientLight;
	bool				unsmoothedTangents;
	bool				hasSubview;			// mirror, remote render, etc
	bool				allowOverlays;

	int					numOps;
	expOp_t* 			ops;				// evaluate to make expressionRegisters

	int					numRegisters;
	float* 				expressionRegisters;

	float* 				constantRegisters;	// NULL if ops ever reference globalParms or entityParms

	int					numStages;
	int					numAmbientStages;

	materialStage_t * 	stages;

	struct mtrParsingData_t * pd;			// only used during parsing

	float				surfaceArea;		// only for listSurfaceAreas

	// we defer loading of the editor image until it is asked for, so the game doesn't load up
	// all the invisible and uncompressed images.
	// If editorImage is NULL, it will atempt to load editorImageName, and set editorImage to that or defaultImage
	idStr				editorImageName;
	mutable idImage * 	editorImage;		// image used for non-shaded preview
	float				editorAlpha;

	bool				suppressInSubview;
	bool				portalSky;
	int					refCount;
};

typedef idList<const idMaterial*, TAG_MATERIAL> idMatList;

#endif /* !__MATERIAL_H__ */
