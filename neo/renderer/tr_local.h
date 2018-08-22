/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012-2016 Robert Beckebans
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


w:\build\tech5\engine\renderer\RenderThread.cpp(88) : TAG_RENDERER
RenderGui
RenderFrameInternal
Frame - RenderFrameInternal
renderlogs/eventlog_.txt        renderMode == RENDER_MODE_SYNC

===========================================================================
*/

#ifndef __TR_LOCAL_H__
#define __TR_LOCAL_H__

#include "precompiled.h"

#include "RenderState.h"
#include "ScreenRect.h"
#include "ImageOpts.h"
#include "Image.h"
#include "Font.h"
#include "RenderDestination.h"
//#include "DeclRenderParm.h"
//#include "DeclRenderProg.h"

// maximum texture units
const int MAX_PROG_TEXTURE_PARMS	= 16;
const int MAX_PROG_VECTOR_PARMS		= 64;

const int FALLOFF_TEXTURE_SIZE		= 64;

const float	DEFAULT_FOG_DISTANCE	= 500.0f;

// picky to get the bilerp correct at terminator
const int FOG_ENTER_SIZE			= 64;
const float FOG_ENTER				= ( FOG_ENTER_SIZE + 1.0f ) / ( FOG_ENTER_SIZE * 2 );

const float CHECK_BOUNDS_EPSILON	= 1.0f;

const int SHADOWMASK_RESOLUTION		= 1024;

typedef idHandle<qhandle_t, -1>	idRenderIndex;

typedef ALIGNTYPE16 idVec4 idRenderVector;
typedef ALIGNTYPE16 idPlane idRenderPlane;

enum lightType_e
{
	LIGHT_TYPE_POINT,		// otherwise a projection light (should probably invert the sense of this, because points are way more common)
	LIGHT_TYPE_SPOT,
	LIGHT_TYPE_PARALLEL,	// lightCenter gives the direction to the light at infinity.
	LIGHT_TYPE_AREA,
	LIGHT_TYPE_MAX
};

enum demoCommand_t
{
	DC_BAD,
	DC_RENDERVIEW,
	DC_UPDATE_ENTITYDEF,
	DC_DELETE_ENTITYDEF,
	DC_UPDATE_LIGHTDEF,
	DC_DELETE_LIGHTDEF,
	DC_LOADMAP,
	DC_CROP_RENDER,
	DC_UNCROP_RENDER,
	DC_CAPTURE_RENDER,
	DC_END_FRAME,
	DC_DEFINE_MODEL,
	DC_SET_PORTAL_STATE,
	DC_UPDATE_SOUNDOCCLUSION,
	DC_GUI_MODEL,
	DC_UPDATE_DECAL,
	DC_DELETE_DECAL,
	DC_UPDATE_OVERLAY,
	DC_DELETE_OVERLAY,
	DC_CACHE_SKINS,
	DC_CACHE_PARTICLES,
	DC_CACHE_MATERIALS,
};

template<typename _T_>
class idRenderStat {
/*
	GetValueString();
	GetAverage
	Update
	AppendValueString

	STAT_AVERAGE_FRAMES
	STAT_TEXT_BUFFER_SIZE

	maxValue;
	values;
	deltaFromValue;
	valueIndex;
	valueString;
*/
};

/*
==============================================================================

SURFACES

==============================================================================
*/

#include "ModelDecal.h"
#include "ModelOverlay.h"
#include "Interaction.h"

class idRenderWorldLocal;
struct viewModel_t;
struct viewLight_t;

// drawSurf_t structures command the back end to render surfaces
// a given idTriangles may be used with multiple viewModel_t,
// as when viewed in a subview or multiple viewport render, or
// with multiple shaders when skinned, or, possibly with multiple
// lights, although currently each lighting interaction creates
// unique idTriangles
// drawSurf_t are always allocated and freed every frame, they are never cached

struct drawSurfBase_t
{
	uint32					numIndexes;
	vertCacheHandle_t		indexCache;			// triIndex_t
	vertCacheHandle_t		vertexCache;		// idDrawVert
	vertCacheHandle_t		jointCache;			// idJointMat
	vertCacheHandle_t		shadowCache;		// idShadowVert / idShadowVertSkinned

	const viewModel_t * 	space;
	const idMaterial * 		material;			// may be NULL for shadow volumes
	const float * 			materialRegisters;	// evaluated and adjusted for referenceShaders
	uint64					extraGLState;		// Extra GL state |'d with material->stage[].drawStateBits
};

typedef struct drawSurf_t
{
	const idTriangles* 		frontEndGeo;		// don't use on the back end, it may be updated by the front end!
	int						numIndexes;
	vertCacheHandle_t		indexCache;			// triIndex_t
	vertCacheHandle_t		vertexCache;		// idDrawVert
	vertCacheHandle_t		shadowCache;		// idShadowVert / idShadowVertSkinned
	vertCacheHandle_t		jointCache;			// idJointMat
	const viewModel_t * 	space;
	const idMaterial * 		material;			// may be NULL for shadow volumes
	uint64					extraGLState;		// Extra GL state |'d with material->stage[].drawStateBits
	float					sort;				// material->sort, modified by gui / entity sort offsets
	const float * 			shaderRegisters;	// evaluated and adjusted for referenceShaders, expressions for conditionals / color / texcoords
	drawSurf_t * 			nextOnLight;		// viewLight chains
	drawSurf_t ** 			linkChain;			// defer linking to lights to a serial section to avoid a mutex
	idScreenRect			scissorRect;		// for scissor clipping, local inside renderView viewport
	int						renderZFail;
	volatile shadowVolumeState_t shadowVolumeState;

	ID_INLINE bool HasSkinning() const { return !!jointCache; }

} idDrawSurface;

// areas have references to hold all the lights and entities in them
struct areaReference_t
{
	areaReference_t * 		areaNext;				// chain in the area
	areaReference_t * 		areaPrev;
	areaReference_t * 		ownerNext;				// chain on either the entityDef or lightDef
	idRenderEntityLocal * 	entity;					// only one of entity / light will be non-NULL
	idRenderLightLocal * 	light;					// only one of entity / light will be non-NULL
	struct portalArea_t *	area;					// so owners can find all the areas they are in
};


class idRenderLightLocal
{
	friend class idRenderWorldLocal;
	friend void CreateMapLight( const idMapEntity* ); //SEA: temp temp
private:
	renderLightParms_t		parms;					// specification

	// first added, so the prelight model is not valid
	idRenderWorldLocal* 	world;
	int						index;					// in world lightdefs

	int						lastModifiedFrameNum;	// to determine if it is constantly changing,
													// and should go in the dynamic frame memory, or kept
													// in the cached memory.
	bool					archived;			// for demo writing
public:
	// derived information
	idRenderPlane			lightProject[4];		// old style light projection where Z and W are flipped and projected lights lightProject[3] is divided by ( zNear + zFar )
	idRenderMatrix			baseLightProject;		// global xyz1 to projected light strq
	idRenderMatrix			inverseBaseLightProject;// transforms the zero-to-one cube to exactly cover the light in world space

	const idMaterial* 		lightShader;			// guaranteed to be valid, even if parms.shader isn't
	idImage* 				falloffImage;

	idVec3					globalLightOrigin;		// accounting for lightCenter and parallel
	idBounds				globalLightBounds;

	int						viewCount;				// if == tr.viewCount, the light is on the viewDef->viewLights list
	viewLight_t* 			viewLight;

	areaReference_t* 		references;				// each area the light is present in will have a lightRef
	idInteraction* 			firstInteraction;		// doubly linked list
	idInteraction* 			lastInteraction;

	struct doublePortal_t *	foggedPortals;

	int						areaNum;				// if not -1, we may be able to cull all the light's
													// interactions if !viewDef->connectedAreas[areaNum]

	bool					lightHasMoved;			// the light has changed its position since it was

public:
	idRenderLightLocal();

	ID_INLINE int						GetIndex() const { return index; }
	ID_INLINE idRenderWorldLocal *		GetOwner() const { return world; }

	ID_INLINE const renderLightParms_t &		GetParms() const { return parms; }
	ID_INLINE const idVec3 &			GetOrigin() const { return parms.origin; }
	ID_INLINE const idMat3 &			GetAxis() const { return parms.axis; }
	ID_INLINE bool						LightCastsShadows() const { return parms.forceShadows || ( !parms.noShadows && lightShader->LightCastsShadows() ); }
	ID_INLINE int						GetID() const { return parms.lightId; }
	ID_INLINE bool						HasPreLightModel() const { return( parms.prelightModel != nullptr ); }

	float								GetAxialSize() const;
	void								DeriveData();
	void								FreeDerivedData();
	void								CreateReferences();

	// Creates viewLight_t with an empty scissor rect and links it to the RenderView.
	// Returns created viewLight_t pointer.
	viewLight_t *						EmitToView( idRenderView* );

	ID_INLINE bool CullModelBoundsToLight( const idBounds& localBounds, const idRenderMatrix& modelRenderMatrix ) const
	{
		idRenderMatrix modelLightProject;
		idRenderMatrix::Multiply( baseLightProject, modelRenderMatrix, modelLightProject );
		return idRenderMatrix::CullBoundsToMVP( modelLightProject, localBounds, true );
	}
};

class idRenderEntityLocal
{
	friend class idRenderWorldLocal;
private:
	renderEntityParms_t		parms;

	idRenderWorldLocal* 	world;
	int						index;					// in world entityDefs

	int						lastModifiedFrameNum;	// to determine if it is constantly changing,
													// and should go in the dynamic frame memory, or kept
													// in the cached memory.
	bool					archived;			// for demo writing

public:
	idRenderModel* 			dynamicModel;			// if parms.model->IsDynamicModel(), this is the generated data
	int						dynamicModelFrameCount;	// continuously animating dynamic models will recreate
													// dynamicModel if this doesn't == tr.viewCount
	idRenderModel* 			cachedDynamicModel;

	// this is just a rearrangement of parms.axis and parms.origin
	idRenderMatrix			modelRenderMatrix;
	//idRenderMatrix			prevModelRenderMatrix;
	idRenderMatrix			inverseBaseModelProject;// transforms the unit cube to exactly cover the model in world space

	// the local bounds used to place entityRefs, either from parms for dynamic entities, or a model bounds
	idBounds				localReferenceBounds;

	// axis aligned bounding box in world space, derived from refernceBounds and
	// modelMatrix in R_CreateEntityRefs()
	idBounds				globalReferenceBounds;

	// a viewModel_t is created whenever a idRenderEntityLocal is considered for inclusion
	// in a given view, even if it turns out to not be visible
	int						viewCount;				// if tr.viewCount == viewCount, viewModel is valid,
	// but the entity may still be off screen
	viewModel_t* 			viewModel;				// in frame temporary memory

	idRenderModelDecal* 	decals;					// decals that have been projected on this model
	idRenderModelOverlay* 	overlays;				// blood overlays on animated models

	areaReference_t* 		entityRefs;				// chain of all references

	idInteraction* 			firstInteraction;		// doubly linked list
	idInteraction* 			lastInteraction;

	bool					needsPortalSky;

public:
	idRenderEntityLocal();

	ID_INLINE int						GetIndex() const { return index; }
	ID_INLINE idRenderWorldLocal *		GetOwner() const { return world; }

	ID_INLINE const renderEntityParms_t & GetParms() const { return parms; }
	ID_INLINE const idVec3 &			GetOrigin() const { return parms.origin; }
	ID_INLINE const idMat3 &			GetAxis() const { return parms.axis; }
	ID_INLINE const idRenderModel *		GetModel() const { return parms.hModel; }

	void								ReadFromDemoFile( class idDemoFile* );
	void								WriteToDemoFile( class idDemoFile* ) const;

	bool								IsDirectlyVisible() const;

	void								DeriveData();
	void								FreeDerivedData( bool keepDecals, bool keepCachedDynamicModel );
	void								FreeDecals();
	void								FreeFadedDecals( int time );
	void								FreeOverlay();

	void								CreateReferences();

	void								ClearDynamicModel();
	bool								IssueCallback();
	idRenderModel *						EmitDynamicModel();
	ID_INLINE bool						HasCallback() const { return( parms.callback != nullptr ); }


	// Creates viewModel_t with an empty scissor rect and links it to the RenderView.
	// Returns created viewModel_t pointer.
	viewModel_t *						EmitToView( idRenderView* );
};

struct shadowOnlyEntity_t
{
	shadowOnlyEntity_t * 			next;
	idRenderEntityLocal *			edef;
};

// viewLights are allocated on the frame temporary stack memory
// a viewLight contains everything that the back end needs out of an idRenderLightLocal,
// which the front end may be modifying simultaniously if running in SMP mode.
// a viewLight may exist even without any surfaces, and may be relevent for fogging,
// but should never exist if its volume does not intersect the view frustum
struct viewLight_t
{
	viewLight_t * 					next;

	// back end should NOT reference the lightDef, because it can change when running SMP
	idRenderLightLocal * 			lightDef;

	// for scissor clipping, local inside renderView viewport
	// scissorRect.Empty() is true if the viewModel_t was never actually
	// seen through any portals
	idScreenRect					scissorRect;

	// R_AddSingleLight() determined that the light isn't actually needed
	bool							removeFromList;

	// R_AddSingleLight builds this list of entities that need to be added
	// to the viewEntities list because they potentially cast shadows into
	// the view, even though the aren't directly visible
	shadowOnlyEntity_t * 			shadowOnlyViewEntities;

	enum interactionState_t
	{
		INTERACTION_UNCHECKED,
		INTERACTION_NO,
		INTERACTION_YES
	};
	byte* 							entityInteractionState;		// [numEntities]

	idVec3							globalLightOrigin;			// global light origin used by backend
	idRenderPlane					lightProject[4];			// light project used by backend
	// RB: added for shadow mapping
	idRenderMatrix					baseLightProject;			// global xyz1 to projected light strq
	//bool							pointLight;					// otherwise a projection light (should probably invert the sense of this, because points are way more common)
	//bool							parallel;					// lightCenter gives the direction to the light at infinity
	lightType_e						lightType;
	idVec3							lightCenter;				// offset the lighting direction for shading and
	int								shadowLOD;					// level of detail for shadowmap selection
	// RB end
	idRenderMatrix					inverseBaseLightProject;	// the matrix for deforming the 'zeroOneCubeModel' to exactly cover the light volume in world space
	const idMaterial* 				lightShader;				// light shader used by backend
	const float*					shaderRegisters;			// shader registers used by backend
	idImage* 						falloffImage;				// falloff image used by backend

	drawSurf_t* 					globalShadows;				// shadow everything
	drawSurf_t* 					localShadows;				// don't shadow local surfaces
	drawSurf_t* 					localInteractions;			// don't get local shadows
	drawSurf_t* 					globalInteractions;			// get shadows from everything
	drawSurf_t* 					translucentInteractions;	// translucent interactions don't get shadows

	// R_AddSingleLight will build a chain of parameters here to setup shadow volumes
	preLightShadowVolumeParms_t* 	preLightShadowVolumes;

	ID_INLINE bool NoInteractions() const { return( !localInteractions && !globalInteractions && !translucentInteractions ); }
	ID_INLINE auto GetLightType() const { return lightType; }
	ID_INLINE bool IsParallelType() const { return( lightType == LIGHT_TYPE_PARALLEL ); }
	ID_INLINE bool IsPointType() const { return( lightType == LIGHT_TYPE_POINT ); }
	ID_INLINE bool IsSpotType() const { return( lightType == LIGHT_TYPE_SPOT ); }
	ID_INLINE uint32 GetShadowLOD() const { return ( uint32 ) shadowLOD; }
};

// a viewModel is created whenever a idRenderEntityLocal is considered for inclusion
// in the current view, but it may still turn out to be culled.
// viewModel are allocated on the frame temporary stack memory
// a viewModel contains everything that the back end needs out of a idRenderEntityLocal,
// which the front end may be modifying simultaneously if running in SMP mode.
// A single entityDef can generate multiple viewModel_t in a single frame, as when seen in a mirror
struct viewModel_t
{
	viewModel_t* 					next;

	// back end should NOT reference the entityDef, because it can change when running SMP
	idRenderEntityLocal*			entityDef;

	// for scissor clipping, local inside renderView viewport
	// scissorRect.Empty() is true if the viewModel_t was never actually
	// seen through any portals, but was created for shadow casting.
	// a viewModel can have a non-empty scissorRect, meaning that an area
	// that it is in is visible, and still not be visible.
	idScreenRect					scissorRect;

	bool							isGuiSurface;			// force two sided and vertex colors regardless of material setting
	//bool							isWorldModel;

	bool							skipMotionBlur;

	bool							weaponDepthHack;
	float							modelDepthHack;

	idRenderMatrix					modelMatrix;			// local coords to global coords
	idRenderMatrix					modelViewMatrix;		// local coords to eye coords
	idRenderMatrix					mvp;

	// parallelAddModels will build a chain of surfaces here that will need to
	// be linked to the lights or added to the drawsurf list in a serial code section
	drawSurf_t * 					drawSurfs;

	// R_AddSingleModel will build a chain of parameters here to setup shadow volumes
	staticShadowVolumeParms_t * 	staticShadowVolumes;
	dynamicShadowVolumeParms_t * 	dynamicShadowVolumes;
};


const int	MAX_CLIP_PLANES	= 1;				// we may expand this to six for some subview issues

// RB: added multiple subfrustums for cascaded shadow mapping
enum frustumPlanes_t
{
	FRUSTUM_PLANE_LEFT,
	FRUSTUM_PLANE_RIGHT,
	FRUSTUM_PLANE_BOTTOM,
	FRUSTUM_PLANE_TOP,
	FRUSTUM_PLANE_NEAR,
	FRUSTUM_PLANE_FAR,
	FRUSTUM_PLANES = 6,
	FRUSTUM_CLIPALL = 1 | 2 | 4 | 8 | 16 | 32
};

enum
{
	FRUSTUM_PRIMARY,
	FRUSTUM_CASCADE1,
	FRUSTUM_CASCADE2,
	FRUSTUM_CASCADE3,
	FRUSTUM_CASCADE4,
	FRUSTUM_CASCADE5,
	MAX_FRUSTUMS,
};

struct frustum_t {
	ALIGNTYPE16 idPlane planes[ FRUSTUM_PLANES ];

	const idPlane & operator[]( int index ) const
	{
		assert( index >= 0 && index < FRUSTUM_PLANES );
		return planes[ index ];
	}
	idPlane & operator[]( int index )
	{
		assert( index >= 0 && index < FRUSTUM_PLANES );
		return planes[ index ];
	}
};
// RB end

/*
=================================================================

 idRenderView

	viewDefs are allocated on the frame temporary stack memory

=================================================================
*/
struct idRenderView {
private: // derived data is private

	//idRenderMatrix			viewMatrix;
	idRenderMatrix			projectionMatrix;
	//idRenderMatrix			viewProjectionMatrix;
	idRenderMatrix			inverseViewMatrix;
	idRenderMatrix			inverseProjectionMatrix;
	idRenderMatrix			inverseViewProjectionMatrix;

	//TODO: get rid of this wordspace crap!
	viewModel_t				worldSpace;

// RB begin
	frustum_t				frustums[ MAX_FRUSTUMS ];					// positive sides face outward, [4] is the front clip plane
	ALIGNTYPE16 float		frustumSplitDistances[ MAX_FRUSTUMS ];
	idRenderMatrix			frustumInvMVPs[ MAX_FRUSTUMS ];
// RB end

public:
	// specified in the call to DrawScene()
	renderViewParms_t		parms;

	idRenderWorldLocal *	renderWorld;

	// Used to find the portalArea that view flooding will take place from.
	// for a normal view, the initialViewOrigin will be renderView.viewOrg,
	// but a mirror may put the projection origin outside
	// of any valid area, or in an unconnected area of the map, so the view
	// area must be based on a point just off the surface of the mirror / subview.
	// It may be possible to get a failed portal pass if the plane of the
	// mirror intersects a portal, and the initialViewAreaOrigin is on
	// a different side than the renderView.viewOrg is.
	idVec3					initialViewAreaOrigin;

	// true if this view is not the main view
	bool					isSubview;
	bool					/**/isMirror;			// the portal is a mirror, invert the face culling
	bool					/**/isXraySubview;

	bool					isEditor;
	bool					is2Dgui;

	// mirrors will often use a single clip plane
	int						numClipPlanes;
	// in world space, the positive side of the plane is the visible side.
	idRenderPlane			clipPlanes[MAX_CLIP_PLANES];

	// in real pixels and proper Y flip
	idScreenRect			viewport;

	// for scissor clipping, local inside renderView viewport
	// subviews may only be rendering part of the main view
	// these are real physical pixel values, possibly scaled and offset from the
	// renderView x/y/width/height
	idScreenRect			scissor;

	idRenderView * 			baseView;				// never go into an infinite subview loop
	const drawSurf_t * 		subviewSurface;
	idImage *				subviewRenderTexture;

	// drawSurfs are the visible surfaces of the viewEntities, sorted
	// by the material sort parameter
	drawSurf_t ** 			drawSurfs;				// we don't use an idList for this, because
	int32					numDrawSurfs;			// it is allocated in frame temporary memory
	int32					maxDrawSurfs;			// may be resized

	viewLight_t *			viewLights;				// chain of all viewLights effecting view
	viewModel_t * 			viewEntitys;			// chain of all viewEntities effecting view, including off screen ones casting shadows
	// we use viewEntities as a check to see if a given view consists solely
	// of 2D rendering, which we can optimize in certain ways.  A 2D view will
	// not have any viewEntities

	int						areaNum;				// -1 = not in a valid area

	// An array in frame temporary memory that lists if an area can be reached without
	// crossing a closed door.  This is used to avoid drawing interactions
	// when the light is behind a closed door.
	bool * 					connectedAreas;
public:

	ID_INLINE void					Clear() { memset( this, 0, sizeof( *this ) ); }

	//void Init3DView();
	//void Init2DView();
	//void Init( int stereoEye, bool 2DView );

	bool							Is2DView() const { return( !viewEntitys ); }

	const idRenderMatrix &			GetViewMatrix() const { return worldSpace.modelViewMatrix; }
	const idRenderMatrix &			GetProjectionMatrix() const { return projectionMatrix; }
	const idRenderMatrix &			GetMVPMatrix() const { return worldSpace.mvp; }	// modelmat is identity
	const idRenderMatrix &			GetInverseVPMatrix() const { return inverseViewProjectionMatrix; }	// modelmat is identity
	const idRenderMatrix &			GetInverseViewMatrix() const { return inverseViewMatrix; }
	const idRenderMatrix &			GetInverseProjMatrix() const { return inverseProjectionMatrix; }

	const viewModel_t &				GetWorldSpace() const { return worldSpace; }

	float							GetZNear() const;
	ID_INLINE int					GetStereoEye() const { return parms.viewEyeBuffer; }
	ID_INLINE float					GetStereoScreenSeparation() const { return parms.stereoScreenSeparation; }
	ID_INLINE const renderViewParms_t &	GetParms() const { return parms; }
	ID_INLINE int					GetID() const { return parms.viewID; }
	ID_INLINE float					GetFOVx() const { return parms.fov_x; }
	ID_INLINE float					GetFOVy() const { return parms.fov_y; }
	ID_INLINE const idVec3 &		GetOrigin() const { return parms.vieworg; }
	ID_INLINE const idMat3 &		GetAxis() const { return parms.viewaxis; }
	ID_INLINE int					GetGameTimeMS( int timeGroup = 0 ) const { return parms.time[ timeGroup ]; }
	ID_INLINE float					GetGameTimeSec( int timeGroup = 0 ) const { return MS2SEC( parms.time[ timeGroup ] ); }
	ID_INLINE const float *			GetGlobalMaterialParms() const { return parms.shaderParms; }
	ID_INLINE const idScreenRect &	GetViewport() const { return viewport; }
	ID_INLINE const idScreenRect &	GetScissor() const { return scissor; } // renderWorld.scissorRect

	ID_INLINE const frustum_t &  	GetBaseFrustum() const { return frustums[ FRUSTUM_PRIMARY ]; }
	ID_INLINE const float *			GetSplitFrustumDistances() const { return frustumSplitDistances; }
	ID_INLINE const idRenderMatrix* GetSplitFrustumInvMVPMatrices() const { return frustumInvMVPs; }

	void							DeriveData();

	// UTILITIES

	// Transform global point to normalized device coordinates.
	void							GlobalToNormalizedDeviceCoordinates( const idVec3& in_global, idVec3& out_ndc ) const;
	// Find the screen pixel bounding box of the given winding( global winding-> model space-> screen space ).
	void							ScreenRectFromWinding( const idWinding &, const idRenderMatrix & modelMatrix, idScreenRect &/*OUT*/ ) const;

	void							ScreenRectFromViewFrustumBounds( const idBounds &bounds, idScreenRect & screenRect ) const;

	bool							PreciseCullSurface( const drawSurf_t* const drawSurf, idBounds& ndcBounds ) const;

	bool							ViewInsideLightVolume( const idBounds & LightBounds ) const;
};

/*
=============================================================

RENDERER BACK END COMMAND QUEUE

TR_CMDS

=============================================================
*/

enum renderCommand_t
{
	RC_NOP,
	RC_DRAW_VIEW_3D,	// may be at a reduced resolution, will be upsampled before 2D GUIs
	RC_DRAW_VIEW_GUI,	// not resolution scaled
	RC_SET_BUFFER,
	RC_COPY_RENDER,
	RC_POST_PROCESS,
};

struct emptyCommand_t
{
	//idQueueNode<emptyCommand_t> queueNode;

	renderCommand_t		commandId;
	renderCommand_t* 	next;
};

struct setBufferCommand_t
{
	renderCommand_t		commandId;
	renderCommand_t* 	next;

	GLenum				buffer;

	///int				frameCount;
	///idScreenRect	rect;
	///int				x, y, imageWidth, imageHeight;
	///idImage*		image;
	///int				cubeFace;	// when copying to a cubeMap
	///bool			clear;
};

struct drawSurfsCommand_t
{
	renderCommand_t		commandId;
	renderCommand_t* 	next;

	idRenderView * 		viewDef;
};

struct copyRenderCommand_t
{
	renderCommand_t		commandId;
	renderCommand_t* 	next;

	int					x;
	int					y;
	int					imageWidth;
	int					imageHeight;
	idImage*			image;
	int					cubeFace;					// when copying to a cubeMap
	bool				clearColorAfterCopy;
};

struct postProcessCommand_t
{
	renderCommand_t		commandId;
	renderCommand_t* 	next;

	idRenderView* 		viewDef;
};

//=======================================================================

// this is the inital allocation for max number of drawsurfs
// in a given view, but it will automatically grow if needed
const int INITIAL_DRAWSURFS =		2048;

enum frameAllocType_t
{
	FRAME_ALLOC_VIEW_DEF,
	FRAME_ALLOC_VIEW_ENTITY,
	FRAME_ALLOC_VIEW_LIGHT,
	FRAME_ALLOC_SURFACE_TRIANGLES,
	FRAME_ALLOC_DRAW_SURFACE,
	FRAME_ALLOC_INTERACTION_STATE,
	FRAME_ALLOC_SHADOW_ONLY_ENTITY,
	FRAME_ALLOC_SHADOW_VOLUME_PARMS,
	FRAME_ALLOC_SHADER_REGISTER,
	FRAME_ALLOC_DRAW_SURFACE_POINTER,
	FRAME_ALLOC_DRAW_COMMAND,
	FRAME_ALLOC_UNKNOWN,
	FRAME_ALLOC_MAX
};

// all of the information needed by the back end must be
// contained in a idFrameData.  This entire structure is
// duplicated so the front and back end can run in parallel
// on an SMP machine.
class idFrameData
{
	friend class idRenderAllocManager;
	friend class idRenderSystemLocal;

	idSysInterlockedInteger	frameMemoryAllocated;
	idSysInterlockedInteger	frameMemoryUsed;
	byte* 					frameMemory;

	int						highWaterAllocated;	// max used on any frame
	int						highWaterUsed;

	// the currently building command list commands can be inserted
	// at the front if needed, as required for dynamically generated textures
	emptyCommand_t* 		cmdHead;	// may be of other command type based on commandId
	emptyCommand_t* 		cmdTail;

	//idQueue<emptyCommand_t, &emptyCommand_t::queueNode> cmdQueue;
public:

	ID_INLINE int GetFrameMemoryAllocated() const { return frameMemoryAllocated.GetValue(); }
	ID_INLINE int GetFrameMemoryUsed() const { return frameMemoryUsed.GetValue(); }
	ID_INLINE int GetHighWaterAllocated() const { return highWaterAllocated; }
	ID_INLINE int GetHighWaterUsed() const { return highWaterUsed; }
};

//=======================================================================

void R_AddDrawViewCmd( idRenderView*, bool guiOnly );
void R_AddDrawPostProcessCmd( idRenderView* );

// this allows a global override of all materials
bool R_GlobalShaderOverride( const idMaterial** );

// this does various checks before calling the idDeclSkin
const idMaterial* R_RemapShaderBySkin( const idMaterial*, const idDeclSkin* customSkin, const idMaterial* customMaterial );


//====================================================

enum uboBindings_e
{
	BINDING_GLOBAL_UBO	= 0,
	BINDING_MATRICES_UBO,			// Skinning
	BINDING_PREVIOUS_MATRICES_UBO,	// Skinning prev mats
	BINDING_SHADOW_UBO,
	BINDING_PROG_PARMS_UBO,

	BINDING_UBO_MAX
};
#if 0
struct uniformBuffersInfo_t {
	const char * name;
	uint32	bindingSlot;
}
	uniformBuffersInfo[] =
{
	{ "global_ubo", BINDING_GLOBAL_UBO },
	{ "matrices_ubo", BINDING_MATRICES_UBO }
	{ "shadow_ubo", BINDING_SHADOW_UBO }
};
#endif
/*
** performanceCounters_t
*/
struct performanceCounters_t
{
	int		c_box_cull_in;
	int		c_box_cull_out;
	int		c_createInteractions;	// number of calls to idInteraction::CreateInteraction
	int		c_createShadowVolumes;
	int		c_generateMd5;
	int		c_entityDefCallbacks;
	int		c_alloc;			// counts for R_StaticAllc/R_StaticFree
	int		c_free;
	int		c_visibleViewEntities;
	int		c_shadowViewEntities;
	int		c_viewLights;
	int		c_numViews;			// number of total views rendered
	int		c_deformedSurfaces;	// idMD5Mesh::GenerateSurface
	int		c_deformedVerts;	// idMD5Mesh::GenerateSurface
	int		c_deformedIndexes;	// idMD5Mesh::GenerateSurface
	int		c_tangentIndexes;	// R_DeriveTangents()
	int		c_entityUpdates;
	int		c_lightUpdates;
	int		c_entityReferences;
	int		c_lightReferences;
	int		c_guiSurfs;
	int		frontEndMicroSec;	// sum of time in all RE_RenderScene's in a frame
};

//SEA: will also be useful for idDeclRenderProg and idDeclRenderParm
enum tmuTarget_t
{
	target_2D = 0,
	target_2DMS,
	target_2DArray,
	target_2DMSArray,
	target_CubeMap,
	target_CubeMapArray,
	target_3D,

	target_Max
};

struct tmu_t //SEA: Must stay this way!
{
	union {
		struct {
			uint32	current2D;
			uint32	current2DMS;
			uint32	current2DArray;
			uint32	current2DMSArray;
			uint32	currentCubeMap;
			uint32	currentCubeMapArray;
			uint32	current3D;
		};
		uint32	currentTarget[7];
	};
};

const int MAX_MULTITEXTURE_UNITS = 16;

enum vertexLayoutType_t
{
	LAYOUT_UNKNOWN = 0,

	LAYOUT_DRAW_VERT_FULL,
	LAYOUT_DRAW_VERT_POSITION_ONLY,
	LAYOUT_DRAW_VERT_POSITION_TEXCOORD,

	LAYOUT_DRAW_SHADOW_VERT,

	LAYOUT_DRAW_SHADOW_VERT_SKINNED,

	LAYOUT_MAX
};

struct glstate_t
{
	GLint				initial_facing;

	tmu_t				tmu[MAX_MULTITEXTURE_UNITS];
	int					currenttmu;

	vertexLayoutType_t	vertexLayout;
	GLuint				currentVAO;

	// RB: 64 bit fixes, changed unsigned int to uintptr_t
	uintptr_t			currentVertexBuffer;
	uintptr_t			currentIndexBuffer;

	GLuint				currentFramebufferObject;
	const idRenderDestination * currentRenderDestination;

	GLuint				currentProgramObject;
	const idDeclRenderProg * currentRenderProg;

	float				polyOfsScale;
	float				polyOfsBias;

	uint64				glStateBits;
};

struct backEndCounters_t
{
	int		c_surfaces;
	int		c_shaders;

	int		c_drawElements;
	int		c_drawIndexes;

	int		c_shadowElements;
	int		c_shadowIndexes;

	int		c_copyFrameBuffer;

	float	c_overDraw;

	uint64	totalMicroSec;			// total microseconds for backend run
	uint64	shadowMicroSec;
};

// all state modified by the back end is separated
// from the front end state
struct backEndState_t
{
	const idRenderView * viewDef;
	backEndCounters_t	pc;

	const viewModel_t * currentSpace;			// for detecting when a matrix must change
	idScreenRect		currentScissor;			// for scissor clipping, local inside renderView viewport
	glstate_t			glState;				// for OpenGL state deltas

	bool				currentRenderCopied;	// true if any material has already referenced _currentRender

	idRenderMatrix		prevMVP[2];				// world MVP from previous frame for motion blur, per-eye

	// RB begin
	///idRenderMatrix		shadowV[6];				// shadow depth view matrix
	///idRenderMatrix		shadowP[6];				// shadow depth projection matrix
	idRenderMatrix		shadowVP[ 6 ];

	idUniformBuffer		viewUniformBuffer;
	idUniformBuffer		shadowUniformBuffer;
	idUniformBuffer		progParmsUniformBuffer;

	float				hdrAverageLuminance;
	float				hdrMaxLuminance;
	float				hdrTime;
	float				hdrKey;
	// RB end

	// surfaces used for code-based drawing
	//drawSurf_t			zeroOneSquareSurface;
	drawSurf_t			unitSquareSurface;
	//drawSurf_t			unitCubeSurface;
	drawSurf_t			zeroOneCubeSurface;
	drawSurf_t			testImageSurface;

	bool hasFogging;	// blend and fog lights

	ID_INLINE void ClearCurrentSpace()
	{
		currentSpace = nullptr;
	}

	ID_INLINE void Clear()
	{
		memset( this, 0, sizeof( *this ) );
	}

	idVec2i	GetRenderViewportOffset() const { return idVec2i( viewDef->GetViewport().x1, viewDef->GetViewport().y1 ); }
	int32 GetRenderViewportWidth() const { return viewDef->GetViewport().GetWidth(); }
	int32 GetRenderViewportHeight() const { return viewDef->GetViewport().GetHeight(); }
};

class idParallelJobList;

const int MAX_GUI_SURFACES	= 1024;		// default size of the drawSurfs list for guis, will
// be automatically expanded as needed

static const int MAX_RENDER_CROPS = 8;

/*
** Most renderer globals are defined here.
** backend functions should never modify any of these fields,
** but may read fields that aren't dynamically modified
** by the frontend.
*/
class idRenderSystemLocal : public idRenderSystem
{
public:
	// external functions
	virtual void			Init();
	virtual void			Shutdown();

	virtual void			ResetGuiModels();

	virtual void			InitRenderDevice();
	virtual void			ShutdownRenderDevice();
	virtual bool			IsRenderDeviceRunning() const;
	virtual bool			IsFullScreen() const;
	virtual stereo3DMode_t	GetStereo3DMode() const;
	virtual bool			HasQuadBufferSupport() const;
	virtual bool			IsStereoScopicRenderingSupported() const;
	virtual stereo3DMode_t	GetStereoScopicRenderingMode() const;
	virtual void			EnableStereoScopicRendering( const stereo3DMode_t mode ) const;
//SEA: moved here from game code:
	virtual stereoDistances_t CaclulateStereoDistances(
		const float	interOcularCentimeters,
		const float screenWidthCentimeters,
		const float convergenceWorldUnits,
		const float	fov_x_degrees ) const;

	virtual int				GetWidth() const;
	virtual int				GetHeight() const;
	virtual int				GetVirtualWidth() const;
	virtual int				GetVirtualHeight() const;
	virtual float			GetPixelAspect() const;
	virtual float			GetPhysicalScreenWidthInCentimeters() const;

	virtual bool			GetModeListForDisplay( const int displayNum, vidModes_t & modeList ) const;

	virtual idRenderWorld* 	AllocRenderWorld();
	virtual void			FreeRenderWorld( idRenderWorld* rw );
	virtual void			BeginLevelLoad();
	virtual void			EndLevelLoad();
	virtual void			LoadLevelImages();
	virtual void			Preload( const idPreloadManifest& manifest, const char* mapName );
	virtual void			BeginAutomaticBackgroundSwaps( autoRenderIconType_t icon = AUTORENDER_DEFAULTICON );
	virtual void			EndAutomaticBackgroundSwaps();
	virtual bool			AreAutomaticBackgroundSwapsRunning( autoRenderIconType_t* usingAlternateIcon = NULL ) const;

	virtual idFont* 		RegisterFont( const char* fontName );
	virtual void			ResetFonts();
	virtual void			PrintMemInfo( MemInfo_t* mi );

	virtual void			SetColor( const idVec4& color );
	virtual uint32			GetColor();
	virtual void			SetGLState( const uint64 glState ) ;
	virtual void			DrawFilled( const idVec4& color, float x, float y, float w, float h );
	virtual void			DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial* material );
	virtual void			DrawStretchPic( const idVec4& topLeft, const idVec4& topRight, const idVec4& bottomRight, const idVec4& bottomLeft, const idMaterial* material );
	virtual void			DrawStretchTri( const idVec2& p1, const idVec2& p2, const idVec2& p3, const idVec2& t1, const idVec2& t2, const idVec2& t3, const idMaterial* material );
	virtual idDrawVert* 	AllocTris( int numVerts, const triIndex_t* indexes, int numIndexes, const idMaterial* material, const stereoDepthType_t stereoType = STEREO_DEPTH_TYPE_NONE );
	virtual void			DrawSmallChar( int x, int y, int ch );
	virtual void			DrawSmallStringExt( int x, int y, const char* string, const idVec4& setColor, bool forceColor );
	virtual void			DrawBigChar( int x, int y, int ch );
	virtual void			DrawBigStringExt( int x, int y, const char* string, const idVec4& setColor, bool forceColor );

	virtual void			WriteDemoPics();
	virtual void			WriteEndFrame();
	virtual void			DrawDemoPics();
	virtual const emptyCommand_t* 	SwapCommandBuffers( uint64* frontEndMicroSec, uint64* backEndMicroSec, uint64* shadowMicroSec, uint64* gpuMicroSec );

	virtual void			SwapCommandBuffers_FinishRendering( uint64* frontEndMicroSec, uint64* backEndMicroSec, uint64* shadowMicroSec, uint64* gpuMicroSec );
	virtual const emptyCommand_t* 	SwapCommandBuffers_FinishCommandBuffers();

	virtual void			RenderCommandBuffers( const emptyCommand_t* commandBuffers );
	virtual void			TakeScreenshot( int width, int height, const char* fileName, int downSample, renderViewParms_t* ref, int exten );
	virtual void			CropRenderSize( int width, int height );
	virtual void			CaptureRenderToImage( const char* imageName, bool clearColorAfterCopy = false );
	virtual void			CaptureRenderToFile( const char* fileName, bool fixAlpha );
	virtual void			UnCrop();
	virtual bool			UploadImage( const char* imageName, const byte* data, int width, int height );

public:
	// internal functions
	idRenderSystemLocal();
	~idRenderSystemLocal();

	void					UpdateStereo3DMode();

	void					Clear();
	void					GetCroppedViewport( idScreenRect* viewport );
	void					PerformResolutionScaling( int& newWidth, int& newHeight );
	int						GetFrameCount() const { return frameCount; };
	int						GetViewCount() const { return viewCount; };

	void					OnFrame();

public:
	// renderer globals
	bool					registered;		// cleared at shutdown, set at InitRenderDevice

	bool					takingScreenshot;

	int						frameCount;		// incremented every frame
	int						viewCount;		// incremented every view (twice a scene if subviewed)
											// and every R_MarkFragments call

	float					frameShaderTime;	// shader time for all non-world 2D rendering

	idVec4					ambientLightVector;	// used for "ambient bump mapping"

	idStaticList< idRenderWorldLocal*, 4 >	worlds;

	idRenderWorldLocal* 	primaryWorld;
	renderViewParms_t		primaryRenderView;
	idRenderView* 			primaryView;
	// many console commands need to know which world they should operate on

	const idMaterial* 		whiteMaterial;
	const idMaterial* 		charSetMaterial;
	const idMaterial* 		defaultPointLight;
	const idMaterial* 		defaultProjectedLight;
	const idMaterial* 		defaultMaterial;
	idImage* 				testImage;
	idCinematic* 			testVideo;
	int						testVideoStartTime;

	idImage* 				ambientCubeImage;	// hack for testing dependent ambient lighting

	idRenderView* 			viewDef;

	performanceCounters_t	pc;					// performance counters

	viewModel_t				identitySpace;		// can use if we don't know viewDef->worldSpace is valid

	idScreenRect			renderCrops[MAX_RENDER_CROPS];
	int						currentRenderCrop;

	// GUI drawing variables for surface creation
	int						guiRecursionLevel;		// to prevent infinite overruns
	uint32					currentColorNativeBytesOrder;
	uint64					currentGLState;
	class idRenderModelGui* guiModel;

	idStaticList<idFont*, 16> fonts;

	unsigned short			gammaTable[256];	// brightness / gamma modify this

	idTriangles* 			unitSquareTriangles;
	idTriangles* 			zeroOneCubeTriangles;
	idTriangles* 			testImageTriangles;

	// these are allocated at buffer swap time, but
	// the back end should only use the ones in the backEnd stucture,
	// which are copied over from the frame that was just swapped.
	drawSurf_t				unitSquareSurface_;
	drawSurf_t				zeroOneCubeSurface_;
	drawSurf_t				testImageSurface_;

	idParallelJobList * 	frontEndJobList;
};

extern backEndState_t		backEnd;
extern idRenderSystemLocal	tr;
extern glconfig_t			glConfig;		// outside of TR since it shouldn't be cleared during ref re-init

//
// cvars
//
extern idCVar r_debugContext;				// enable various levels of context debug
extern idCVar r_glDriver;					// "opengl32", etc
extern idCVar r_skipIntelWorkarounds;		// skip work arounds for Intel driver bugs
extern idCVar r_vidMode;					// video mode number
extern idCVar r_displayRefresh;				// optional display refresh rate option for vid mode
extern idCVar r_fullscreen;					// 0 = windowed, 1 = full screen
extern idCVar r_antiAliasing;				// anti aliasing mode, SMAA, TXAA, MSAA etc.

extern idCVar r_znear;						// near Z clip plane

extern idCVar r_swapInterval;				// changes wglSwapIntarval
extern idCVar r_offsetFactor;				// polygon offset parameter
extern idCVar r_offsetUnits;				// polygon offset parameter
extern idCVar r_singleTriangle;				// only draw a single triangle per primitive
extern idCVar r_logFile;					// number of frames to emit GL logs
extern idCVar r_clear;						// force screen clear every frame
extern idCVar r_subviewOnly;				// 1 = don't render main view, allowing subviews to be debugged
extern idCVar r_lightScale;					// all light intensities are multiplied by this, which is normally 3
extern idCVar r_flareSize;					// scale the flare deforms from the material def

extern idCVar r_gamma;						// changes gamma tables
extern idCVar r_brightness;					// changes gamma tables

extern idCVar r_checkBounds;				// compare all surface bounds with precalculated ones
extern idCVar r_maxAnisotropicFiltering;	// texture filtering parameter
extern idCVar r_useTrilinearFiltering;		// Extra quality filtering
extern idCVar r_lodBias;					// lod bias

extern idCVar r_useLightPortalFlow;			// 1 = do a more precise area reference determination
extern idCVar r_useShadowSurfaceScissor;	// 1 = scissor shadows by the scissor rect of the interaction surfaces
extern idCVar r_useConstantMaterials;		// 1 = use pre-calculated material registers if possible
extern idCVar r_useNodeCommonChildren;		// stop pushing reference bounds early when possible
extern idCVar r_useSilRemap;				// 1 = consider verts with the same XYZ, but different ST the same for shadows
extern idCVar r_useLightPortalCulling;		// 0 = none, 1 = box, 2 = exact clip of polyhedron faces, 3 MVP to plane culling
extern idCVar r_useLightAreaCulling;		// 0 = off, 1 = on
extern idCVar r_useLightScissors;			// 1 = use custom scissor rectangle for each light
extern idCVar r_useEntityPortalCulling;		// 0 = none, 1 = box
extern idCVar r_skipPrelightShadows;		// 1 = skip the dmap generated static shadow volumes
extern idCVar r_useCachedDynamicModels;		// 1 = cache snapshots of dynamic models
extern idCVar r_usePortals;					// 1 = use portals to perform area culling, otherwise draw everything
extern idCVar r_useEntityCallbacks;			// if 0, issue the callback immediately at update time, rather than defering
extern idCVar r_lightAllBackFaces;			// light all the back faces, even when they would be shadowed
extern idCVar r_useLightDepthBounds;		// use depth bounds test on lights to reduce both shadow and interaction fill
extern idCVar r_useShadowDepthBounds;		// use depth bounds test on individual shadows to reduce shadow fill
// RB begin
extern idCVar r_useShadowMapping;			// use shadow mapping instead of stencil shadows
extern idCVar r_useHalfLambertLighting;		// use Half-Lambert lighting instead of classic Lambert
extern idCVar r_useHDR;
extern idCVar r_useSRGB;
extern idCVar r_useProgUBO;
// RB end

extern idCVar r_skipStaticInteractions;		// skip interactions created at level load
extern idCVar r_skipDynamicInteractions;	// skip interactions created after level load
extern idCVar r_skipPostProcess;			// skip all post-process renderings
extern idCVar r_skipSuppress;				// ignore the per-view suppressions
extern idCVar r_skipInteractions;			// skip all light/surface interaction drawing
extern idCVar r_skipFrontEnd;				// bypasses all front end work, but 2D gui rendering still draws
extern idCVar r_skipBackEnd;				// don't draw anything
extern idCVar r_skipCopyTexture;			// do all rendering, but don't actually copyTexSubImage2D
extern idCVar r_skipRender;					// skip 3D rendering, but pass 2D
extern idCVar r_skipRenderContext;			// NULL the rendering context during backend 3D rendering
extern idCVar r_skipTranslucent;			// skip the translucent interaction rendering
extern idCVar r_skipAmbient;				// bypasses all non-interaction drawing
extern idCVar r_skipNewAmbient;				// bypasses all vertex/fragment program ambients
extern idCVar r_skipSubviews;				// 1 = don't render any mirrors / cameras / etc
extern idCVar r_skipGuiShaders;				// 1 = don't render any gui elements on surfaces
extern idCVar r_skipParticles;				// 1 = don't render any particles
extern idCVar r_skipUpdates;				// 1 = don't accept any entity or light updates, making everything static
extern idCVar r_skipDeforms;				// leave all deform materials in their original state
extern idCVar r_skipDynamicTextures;		// don't dynamically create textures
extern idCVar r_skipBump;					// uses a flat surface instead of the bump map
extern idCVar r_skipSpecular;				// use black for specular
extern idCVar r_skipDiffuse;				// use black for diffuse
extern idCVar r_skipDecals;					// skip decal surfaces
extern idCVar r_skipOverlays;				// skip overlay surfaces
extern idCVar r_skipShadows;				// disable shadows

extern idCVar r_ignoreGLErrors;

extern idCVar r_screenFraction;				// for testing fill rate, the resolution of the entire screen can be changed
extern idCVar r_showUnsmoothedTangents;		// highlight geometry rendered with unsmoothed tangents
extern idCVar r_showSilhouette;				// highlight edges that are casting shadow planes
extern idCVar r_showVertexColor;			// draws all triangles with the solid vertex color
extern idCVar r_showUpdates;				// report entity and light updates and ref counts
extern idCVar r_showDemo;					// report reads and writes to the demo file
extern idCVar r_showDynamic;				// report stats on dynamic surface generation
extern idCVar r_showIntensity;				// draw the screen colors based on intensity, red = 0, green = 128, blue = 255
extern idCVar r_showTrace;					// show the intersection of an eye trace with the world
extern idCVar r_showDepth;					// display the contents of the depth buffer and the depth range
extern idCVar r_showTris;					// enables wireframe rendering of the world
extern idCVar r_showSurfaceInfo;			// show surface material name under crosshair
extern idCVar r_showNormals;				// draws wireframe normals
extern idCVar r_showEdges;					// draw the sil edges
extern idCVar r_showViewEntitys;			// displays the bounding boxes of all view models and optionally the index
extern idCVar r_showTexturePolarity;		// shade triangles by texture area polarity
extern idCVar r_showTangentSpace;			// shade triangles by tangent space
extern idCVar r_showDominantTri;			// draw lines from vertexes to center of dominant triangles
extern idCVar r_showTextureVectors;			// draw each triangles texture (tangent) vectors
extern idCVar r_showLights;					// 1 = print light info, 2 = also draw volumes
extern idCVar r_showLightCount;				// colors surfaces based on light count
extern idCVar r_showShadows;				// visualize the stencil shadow volumes
extern idCVar r_showLightScissors;			// show light scissor rectangles
extern idCVar r_showMemory;					// print frame memory utilization
extern idCVar r_showCull;					// report sphere and box culling stats
extern idCVar r_showAddModel;				// report stats from tr_addModel
extern idCVar r_showSurfaces;				// report surface/light/shadow counts
extern idCVar r_showPrimitives;				// report vertex/index/draw counts
extern idCVar r_showPortals;				// draw portal outlines in color based on passed / not passed
extern idCVar r_showSkel;					// draw the skeleton when model animates
extern idCVar r_showOverDraw;				// show overdraw
// RB begin
extern idCVar r_showShadowMaps;
extern idCVar r_showShadowMapLODs;
// RB end
extern idCVar r_jointNameScale;				// size of joint names when r_showskel is set to 1
extern idCVar r_jointNameOffset;			// offset of joint names when r_showskel is set to 1

extern idCVar r_testGamma;					// draw a grid pattern to test gamma levels
extern idCVar r_testGammaBias;				// draw a grid pattern to test gamma levels

extern idCVar r_singleLight;				// suppress all but one light
extern idCVar r_singleEntity;				// suppress all but one entity
extern idCVar r_singleArea;					// only draw the portal area the view is actually in
extern idCVar r_singleSurface;				// suppress all but one surface on each entity
extern idCVar r_shadowPolygonOffset;		// bias value added to depth test for stencil shadow drawing
extern idCVar r_shadowPolygonFactor;		// scale value for stencil shadow drawing

extern idCVar r_jitter;						// randomly subpixel jitter the projection matrix
extern idCVar r_orderIndexes;				// perform index reorganization to optimize vertex use

extern idCVar r_debugLineDepthTest;			// perform depth test on debug lines
extern idCVar r_debugLineWidth;				// width of debug lines
extern idCVar r_debugArrowStep;				// step size of arrow cone line rotation in degrees
extern idCVar r_debugPolygonFilled;

extern idCVar r_materialOverride;			// override all materials

extern idCVar r_debugRenderToTexture;

extern idCVar stereoRender_deGhost;			// subtract from opposite eye to reduce ghosting

// RB begin
extern idCVar r_shadowMapFrustumFOV;
extern idCVar r_shadowMapSingleSide;
extern idCVar r_shadowMapImageSize;
extern idCVar r_shadowMapJitterScale;
extern idCVar r_shadowMapBiasScale;
extern idCVar r_shadowMapRandomizeJitter;
extern idCVar r_shadowMapSamples;
extern idCVar r_shadowMapSplits;
extern idCVar r_shadowMapSplitWeight;
extern idCVar r_shadowMapLodScale;
extern idCVar r_shadowMapLodBias;
extern idCVar r_shadowMapPolygonFactor;
extern idCVar r_shadowMapPolygonOffset;
extern idCVar r_shadowMapOccluderFacing;
extern idCVar r_shadowMapRegularDepthBiasScale;
extern idCVar r_shadowMapSunDepthBiasScale;
// RB end

/*
====================================================================

INITIALIZATION

====================================================================
*/

void R_Init();
void R_InitOpenGL();

void R_SetColorMappings();

void R_ScreenShot_f( const idCmdArgs& args );
void R_StencilShot();

/*
====================================================================

	IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

struct glimpParms_t
{
	int			x;				// ignored in fullscreen
	int			y;				// ignored in fullscreen
	int			width;
	int			height;

	// 0 = windowed, otherwise 1 based monitor number to go full screen on,
	// -1 = borderless window for spanning multiple displays.
	int			fullScreen;

	bool		stereo;
	int			displayHz;
	int			multiSamples;

	int32		GetOutputIndex() const { return( fullScreen - 1 ); }
	bool		IsBorderLessWindow() const { return( fullScreen == -1 ); }
	bool		IsFullscreen() const { return( fullScreen != 0 ); }
};

// DG: R_GetModeListForDisplay is called before GLimp_Init(), but SDL needs SDL_Init() first.
// So add PreInit for platforms that need it, others can just stub it.
void		GLimp_PreInit();

// If the desired mode can't be set satisfactorily, false will be returned.
// If succesful, sets glConfig.nativeScreenWidth, glConfig.nativeScreenHeight, and glConfig.pixelAspect
// The renderer will then reset the glimpParms to "safe mode" of 640x480
// fullscreen and try again.  If that also fails, the error will be fatal.
bool		GLimp_Init( const glimpParms_t & );

// will set up gl up with the new parms
bool		GLimp_SetScreenParms( const glimpParms_t & );

// Destroys the rendering context, closes the window, resets the resolution,
// and resets the gamma ramps.
void		GLimp_Shutdown();

// Sets the hardware gamma ramps for gamma and brightness adjustment.
// These are now taken as 16 bit values, so we can take full advantage
// of dacs with >8 bits of precision
void		GLimp_SetGamma( unsigned short red[256],
							unsigned short green[256],
							unsigned short blue[256] );



/*
============================================================

RENDERWORLD_DEFS

============================================================
*/

void R_FreeDerivedData();
void R_ReCreateWorldReferences();
void R_CheckForEntityDefsUsingModel( idRenderModel* model );
void R_ModulateLights_f( const idCmdArgs& args );

/*
====================================================================

TR_FRONTEND_MAIN

====================================================================
*/

void R_RenderView( idRenderView* );
void R_RenderPostProcess( idRenderView* );

//#define TRACK_FRAME_ALLOCS

class idRenderAllocManager
{
	// Everything that is needed by the backend needs
	// to be double buffered to allow it to run in parallel.
	static const unsigned int NUM_FRAME_DATA = 2;
	static const unsigned int FRAME_ALLOC_ALIGNMENT = 128;
	static const unsigned int MAX_FRAME_MEMORY = 64 * 1024 * 1024;	// larger so that we can noclip on PC for dev purposes

	idFrameData		frameData[ NUM_FRAME_DATA ];
	idFrameData* 	currentFrameData;
	unsigned int	currentFrame;

#if defined( TRACK_FRAME_ALLOCS )
	idSysInterlockedInteger frameAllocTypeCount[ FRAME_ALLOC_MAX ];
	int frameHighWaterTypeCount[ FRAME_ALLOC_MAX ];
#endif

	void *	_FrameAlloc( int bytes, frameAllocType_t type = FRAME_ALLOC_UNKNOWN );
	void *	_ClearedFrameAlloc( int bytes, frameAllocType_t type = FRAME_ALLOC_UNKNOWN );

	void *	_StaticAlloc( int bytes, const memTag_t tag = TAG_RENDER_STATIC );		// just malloc with error checking
	void *	_ClearedStaticAlloc( int bytes, const memTag_t tag = TAG_RENDER_STATIC );	// with memset
	void	_StaticFree( void* data );

	void *	_GetCommandBuffer( int bytes );

public:

	void	InitFrameData();
	void	ShutdownFrameData();
	void	ToggleFrame();

	ID_INLINE const idFrameData * GetCurrentFrame() const { return currentFrameData; }

	template< typename _T_, frameAllocType_t frameAllocType = FRAME_ALLOC_UNKNOWN, bool bCleared = false >
	_T_ * FrameAlloc( uint32 count = 1 )
	{
		const int bytes = (int)sizeof( _T_ ) * count;
		if( bCleared )
		{
			return reinterpret_cast< _T_* >( _ClearedFrameAlloc( bytes, frameAllocType ) );
		}
		else
		{
			return reinterpret_cast< _T_* >( _FrameAlloc( bytes, frameAllocType ) );
		}
	}

	template< typename _T_, memTag_t memTag = TAG_RENDER_STATIC, bool bCleared = false >
	_T_ * StaticAlloc( uint32 count = 1 )
	{
		const int bytes = (int)sizeof( _T_ ) * count;
		if( bCleared )
		{
			return reinterpret_cast< _T_* >( _ClearedStaticAlloc( bytes, memTag ) );
		}
		else
		{
			return reinterpret_cast< _T_* >( _StaticAlloc( bytes, memTag ) );
		}
	}

	void StaticFree( void* data )
	{
		_StaticFree( data );
	}

	template< typename _T_ >
	_T_ * GetEmptyFrameCmd()
	{
		return reinterpret_cast< _T_* >( _GetCommandBuffer( sizeof( _T_ ) ) );
	}
};
extern idRenderAllocManager allocManager;

template idTriangles* idRenderAllocManager::FrameAlloc<idTriangles>( uint32 count );
template drawSurf_t* idRenderAllocManager::FrameAlloc<drawSurf_t>( uint32 count );

#if 0
class idRenderFrameInfo
{
	void Clear() {
		memset( this, 0, sizeof(*this) );
	}
};

class idScreenView
{
	int screenRect;
	int viewIndex;
	int usePreviousRendering;
	int world;
	int bypassUserCmd;
	int viewGuis;
	int guiOriginOffset;
};

/*
idCVar stereoRender_separation;
idCVar stereoRender_screenSeparation;
idCVar stereoRender_guiOffset;
idCVar stereoRender_swapEyes;
idCVar multiView_fullRender;
idCVar r_loadingIconSize;
idCVar r_loadingIconSpeed;
idCVar r_dialogIconX;
idCVar r_dialogIconY;
com_skipGameRenderView

screenViewDef_t	screenViewDefs;
	name;
	playerNum;
	x;
	y;
	width;
	height;
	getsGlobalGuis;
	getsLocalGuis;
	eyeSeparation;

idStaticList<idScreenView,3>
idList<idRenderModel * __ptr64,5>
idList<idScreenView,5>
*/
class idRenderManager {

	idRenderFrameInfo	renderFrameInfo;
	//? currentViewDef;

	void Clear(  );
	void RenderFrame();
	void BuildGameFrame( gameReturn_t& gameReturn, idGame* game, uint64 frameNumber, uint32 viewIndexToPlayerIndex );
	void UpdateConsole();
	void AddGlobalGui( idRenderModelGui *gui );
	void BuildGuiFrame( baseGui, loading );
	void ClearAllGuiModels();
	void RenderFrameAndBeginAutomaticBackgroundSwaps( loadingIcon, oldFrameInAuto );
};
#endif

/*
============================================================

TR_FRONTEND_ADDLIGHTS

============================================================
*/

void R_ShadowBounds( const idBounds& modelBounds, const idBounds& lightBounds, const idVec3& lightOrigin, idBounds& shadowBounds );

/*
============================================================

TR_FRONTEND_ADDMODELS


	modelDepthSort_t {
		MODEL_DEPTH_SORT_PORTAL_INSIDE,
		MODEL_DEPTH_SORT_PORTAL_PLANE,
		MODEL_DEPTH_SORT_VIEW_HEAD,
		MODEL_DEPTH_SORT_VIEW_BODY,
		MODEL_DEPTH_SORT_SCENE_MODEL,
		MODEL_DEPTH_SORT_NORMAL,
		MODEL_DEPTH_SORT_AFTER_WORLD,
		MODEL_DEPTH_SORT_OCCLUSION_TEST,
		MODEL_DEPTH_SORT_SKY,
		MODEL_DEPTH_SORT_TRANSPARENT,
		MODEL_DEPTH_SORT_LAST,
		MODEL_DEPTH_SORT_NUM
	};


============================================================
*/

void R_SetupDrawSurfShader( drawSurf_t*, const idMaterial*, const renderEntityParms_t*, const idRenderView* );
void R_SetupDrawSurfJoints( drawSurf_t*, const idTriangles*, const idMaterial* );
void R_LinkDrawSurfToView( drawSurf_t*, idRenderView* );


/*
=============================================================

TR_FRONTEND_DEFORM

=============================================================
*/

drawSurf_t* R_DeformDrawSurf( drawSurf_t* );

/*
=============================================================

TR_FRONTEND_GUISURF

=============================================================
*/

void R_SurfaceToTextureAxis( const idTriangles*, idVec3& origin, idMat3& axis );

/*
============================================================

TR_FRONTEND_SUBVIEW

============================================================
*/

bool R_PreciseCullSurface( const idRenderView *, const drawSurf_t*, idBounds& ndcBounds );

/*
============================================================

TR_TRISURF

============================================================
*/

// deformable meshes precalculate as much as possible from a base frame, then generate
// complete idTriangles from just a new set of vertexes
struct deformInfo_t
{
	int					numSourceVerts;

	// numOutputVerts may be smaller if the input had duplicated or degenerate triangles
	// it will often be larger if the input had mirrored texture seams that needed
	// to be busted for proper tangent spaces
	int					numOutputVerts;
	idDrawVert* 		verts;

	int					numIndexes;
	triIndex_t* 		indexes;

	triIndex_t* 		silIndexes;				// indexes changed to be the first vertex with same XYZ, ignoring normal and texcoords

	int					numMirroredVerts;		// this many verts at the end of the vert list are tangent mirrors
	int* 				mirroredVerts;			// tri->mirroredVerts[0] is the mirror of tri->numVerts - tri->numMirroredVerts + 0

	int					numDupVerts;			// number of duplicate vertexes
	int* 				dupVerts;				// pairs of the number of the first vertex and the number of the duplicate vertex

	int					numSilEdges;			// number of silhouette edges
	silEdge_t* 			silEdges;				// silhouette edges

	vertCacheHandle_t	staticIndexCache;		// GL_INDEX_TYPE
	vertCacheHandle_t	staticAmbientCache;		// idDrawVert
	vertCacheHandle_t	staticShadowCache;		// idShadowCacheSkinned

	void				Free();
	void				CreateStaticCache();
	size_t				CPUMemoryUsed();
};

/*
=============================================================

BACKEND

=============================================================
*/

void RB_ExecuteBackEndCommands( const emptyCommand_t* cmds );

/*
============================================================

TR_BACKEND_DRAW

============================================================
*/

void RB_CMD_DrawView( const void* data, const int stereoEye );
void RB_CMD_CopyRender( const void* data );
void RB_CMD_PostProcess( const void* data );

/*
=============================================================

TR_BACKEND_RENDERTOOLS

=============================================================
*/

float RB_DrawTextLength( const char* text, float scale, int len );
void RB_AddDebugText( const char* text, const idVec3& origin, float scale, const idVec4& color, const idMat3& viewAxis, const int align, const int lifetime, const bool depthTest );
void RB_ClearDebugText( int time );
void RB_AddDebugLine( const idVec4& color, const idVec3& start, const idVec3& end, const int lifeTime, const bool depthTest );
void RB_ClearDebugLines( int time );
void RB_AddDebugPolygon( const idVec4& color, const idWinding& winding, const int lifeTime, const bool depthTest );
void RB_ClearDebugPolygons( int time );
void RB_ShowLights( const drawSurf_t * const * drawSurfs, int numDrawSurfs );
void RB_ShowLightCount( const drawSurf_t * const * drawSurfs, int numDrawSurfs );
void RB_PolygonClear();
void RB_ScanStencilBuffer();
void RB_ShowDestinationAlpha();
void RB_ShowOverdraw();
void RB_RenderDebugTools( const drawSurf_t * const * drawSurfs, int numDrawSurfs );
void RB_ShutdownDebugTools();

//=============================================

#include "ResolutionScale.h"
#include "RenderLog.h"
#include "jobs/ShadowShared.h"
#include "jobs/prelightshadowvolume/PreLightShadowVolume.h"
#include "jobs/staticshadowvolume/StaticShadowVolume.h"
#include "jobs/dynamicshadowvolume/DynamicShadowVolume.h"
#include "GraphicsAPIWrapper.h"

#include "BufferObject.h"
#include "RenderProgs.h"
#include "RenderWorld_local.h"
#include "GuiModel.h"
#include "VertexCache.h"


#endif /* !__TR_LOCAL_H__ */
