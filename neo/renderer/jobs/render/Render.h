
#include "../../tr_local.h"

static ID_INLINE GLuint GetGLObject( void * apiObject ) {
#pragma warning( suppress: 4311 4302 )
	return reinterpret_cast<GLuint>( apiObject );
}

extern idCVar stereoRender_swapEyes;
extern idCVar r_skipInteractionFastPath;

static const ALIGNTYPE16 float zero[ 4 ] = { 0, 0, 0, 0 };
static const ALIGNTYPE16 float one[ 4 ] = { 1, 1, 1, 1 };
static const ALIGNTYPE16 float negOne[ 4 ] = { -1, -1, -1, -1 };

const int INTERACTION_TEXUNIT_BUMP = 0;
const int INTERACTION_TEXUNIT_FALLOFF = 1;
const int INTERACTION_TEXUNIT_PROJECTION = 2;
const int INTERACTION_TEXUNIT_DIFFUSE = 3;
const int INTERACTION_TEXUNIT_SPECULAR = 4;
const int INTERACTION_TEXUNIT_SHADOWMAPS = 5;
const int INTERACTION_TEXUNIT_JITTER = 6;

// complex light / surface interactions are broken up into multiple passes of a
// simple interaction shader
struct drawInteraction_t {
	const drawSurf_t * 	surf;

	idImage * 			bumpImage;
	idImage * 			diffuseImage;
	idImage * 			specularImage;

	idRenderVector		diffuseColor;	// may have a light color baked into it
	idRenderVector		specularColor;	// may have a light color baked into it
	stageVertexColor_t	vertexColor;	// applies to both diffuse and specular

	int					ambientLight;	// use tr.ambientNormalMap instead of normalization cube map

										// these are loaded into the vertex program
	idRenderVector		bumpMatrix[ 2 ];
	idRenderVector		diffuseMatrix[ 2 ];
	idRenderVector		specularMatrix[ 2 ];

	void Clear()
	{
		bumpImage = nullptr;
		diffuseImage = nullptr;
		specularImage = nullptr;

		diffuseColor[ 0 ] = diffuseColor[ 1 ] = diffuseColor[ 2 ] = diffuseColor[ 3 ] = 1.0f;
		specularColor[ 0 ] = specularColor[ 1 ] = specularColor[ 2 ] = specularColor[ 3 ] = 0.0f;
		vertexColor = SVC_IGNORE;

		ambientLight = 0;
	}
};
void RB_ResetViewportAndScissorToDefaultCamera( const idRenderView * const view );
void RB_SetMVP( const idRenderMatrix& mvp );
void RB_SetSurfaceSpaceMatrices( const drawSurf_t *const );
void RB_SetMVPWithStereoOffset( const idRenderMatrix& mvp, const float stereoOffset );
void RB_SetVertexColorParms( stageVertexColor_t );
void RB_GetShaderTextureMatrix( const float* shaderRegisters, const textureStage_t*, float matrix[ 16 ] );
void RB_LoadShaderTextureMatrix( const float* shaderRegisters, const textureStage_t* );
void RB_BakeTextureMatrixIntoTexgen( idPlane lightProject[ 3 ], const float* textureMatrix );
void RB_ResetViewportAndScissorToDefaultCamera( const idRenderView * const );
void RB_SetScissor( const idScreenRect & );
void RB_SetBaseRenderDestination( const idRenderView * const, const viewLight_t * const = nullptr );
void RB_ResetRenderDest( const bool hdrIsActive );

void RB_SetupForFastPathInteractions( const idVec4& diffuseColor, const idVec4& specularColor );
void RB_DrawSingleInteraction( drawInteraction_t* );
void RB_SetupInteractionStage( const materialStage_t*, const float* surfaceRegs, const float lightColor[ 4 ], idVec4 matrix[ 2 ], float color[ 4 ] );
void RB_DrawComplexMaterialInteraction( drawInteraction_t & inter,
	const float* surfaceRegs, const idMaterial* const surfaceMaterial,
	const idRenderVector & diffuseColorMul, const idRenderVector & specularColorMul );

void RB_PrepareStageTexturing( const materialStage_t*, const drawSurf_t* );
void RB_FinishStageTexturing( const materialStage_t*, const drawSurf_t* );

////////////////////////////////////////////////////////////////////////
// RenderPasses

void RB_FillDepthBufferFast( const drawSurf_t * const * drawSurfs, int numDrawSurfs );

void RB_DrawInteractionsForward( const idRenderView * const );

void RB_ShadowMapPass( const drawSurf_t * const drawSurfs, const viewLight_t * const vLight );

void RB_StencilShadowPass( const drawSurf_t* const drawSurfs, const viewLight_t* const vLight );
void RB_StencilSelectLight( const viewLight_t* const vLight );

int  RB_DrawTransMaterialPasses( const drawSurf_t* const* const drawSurfs, const int numDrawSurfs, const float guiStereoScreenOffset, const int stereoEye );

void RB_FogAllLights();


/*

idTokenStatic<256>
	buffer

idImageByteArray
	Copy(  );
	Extrude(  );
	data;
	width;
	height;
idImageByteArray

struct stateTable_t 
{
	stateFlag 
	stateName
};

enum queryState_t 
{
	QUERY_INVALID
	QUERY_STARTED
	QUERY_STOPPED
};

QUERY_INDEX_BITS
QUERY_INDEX_MASK
QUERY_BATCH_BITS
QUERY_BATCH_MASK
QUERY_BATCH_SHIFT

struct renderStateGL_t 
{
	uint64	frameNumber;
	uint64	currentState;
	currentOcclusionQueryBatch;
	currentOcclusionQuery;
	totalQueriesInBatch;
	cachedOcclusionResult;
	lastCachedOcclusionBatch;
	timeQueryId;
	timeQueryPending;
	startGPUTimeMicroSec;
	endGPUTimeMicroSec;
	renderSync;
	currentNumTargets;
	defaultViewPort;
	float	polyOfsScale;
	float	polyOfsBias;
	float	polyOfsFill;
	GLuint	boundProgram;
	GLuint	boundVertexProgramChecksum;
	GLuint	boundFragmentProgramChecksum;
	GLuint	boundVertexParmVersion;
	GLuint	boundFragmentParmVersion;
	GLuint	boundTextureParmVersion;
	GLuint	boundSamplerParmVersion;
	GLuint	boundTextureUnits;
	GLuint	boundTextures;
	GLuint	boundSamplers;
	GLuint	boundTexturesChanged;
	GLuint	boundVertexBuffer;
	GLuint	boundIndexBuffer;
	GLuint	boundVertexAttribArrays;
	GLuint	boundVertexAttribPointers;
	GLuint	boundVAO;
	bool	hardwareSkinning;
} renderStateGL;

struct vaoInfo_t 
{
	vao;
	vb;
	ib;
	mb;
	stb;
	vertexMask;
};

{
	idVaoContainer()
	AddVaoInfo()
	RemoveVaoInfo() 
	FindVaoInfo()
	RemoveVertexBuffer()
	RemoveIndexBuffer()
	GenerateKey()
	VAO_DEFAULT_HASH_SIZE;
	VAO_DEFAULT_INDEX_SIZE;
	VAO_DEFAULT_INFO_SIZE;	vaoInfo_t	vaoInfo;
	idList<vaoInfo_t,5>	freeList;
	vaoHash;
	vbVaoHash;
	ibVaoHash;
	mbVaoHash;
	stbVaoHash;
};


renderPass_t {
	RENDERPASS_CLEAR,
	RENDERPASS_SORTED,
	RENDERPASS_BACKGROUND,
	RENDERPASS_BACKGROUND_EMISSIVE,
	RENDERPASS_SSS,
	RENDERPASS_EMISSIVE,
	RENDERPASS_EMISSIVE_ONLY,
	RENDERPASS_BLEND,
	RENDERPASS_DISTORTION,
	NUM_RENDER_PASSES
};

renderDrawSurfParms_t {
	settings;
	renderView;
	allowInGameGUIs;
	captureParms_t captureParms;
	renderDestDefault;
	renderDestGui;
};

renderPassParms_t {
								pass;
	renderSettings_t			settings;
	const idRenderView *		renderView;
	renderDrawSurfParms_t		drawSurfParms;
	const idRenderDestination *	renderDestDefault;
	const idRenderDestination *	renderDestTemp;
	const idRenderDestination *	renderDestDistortion;
	const idDeclRenderParm *	rpViewTarget;
	const idDeclRenderParm *	rpViewTargetMS;
	const idDeclRenderParm *	rpSSSDirection;
	renderPasses;
	renderPassSurfaces_t		sortedDrawSurfs;
	renderPassSurfaces_t		depthSortedDrawSurfs;
	uint32						numDepthSortedDrawSurfs;
								binaryModelState;
};

struct expOp_t {
	GetA
	GetB
	GetDest

	GetA@expOp_t@@QEBAPEBVidDeclRenderParm@@XZ  a>    ð¾D  ?
	GetDest@expOp_t@@QEBAPEBVidDeclRenderParm@@XZ   @6     š?  ?
}

enum {
	REFERENCE_MASK_GEOMETRY,
	REFERENCE_MASK_JOINTS,
	REFERENCE_MASK_MORPH,
	REFERENCE_MASK_ST,
	REFERENCE_MASK_CONSTANTS,
	REFERENCE_MASK_PARMS,
	REFERENCE_MASK_ALL
};

MC_BAD,
MC_INVISIBLE,
MC_PERFORATED,
MC_TRANSLUCENT,
MC_OPAQUE,
MC_PERFORATED_FOR_MODELFADE,
materialCoverage_t

INPUT_TYPE_KEYBOARD,
INPUT_TYPE_MOUSE,
INPUT_TYPE_GAMEPAD,
inputType_t

*/

struct renderSettings_t 
{
	bool	isComboMap;
	bool	isToolsWorld;
	bool	skipWorld;
	bool	skipModels;
	bool	skipLights;
	bool	skipBlendLightShadows;
	bool	skipSuppress;
	bool	skipFeedback;
	bool	skipDynamic;
	bool	skipAddAlways;
	bool	skipOpaqueSurfaces;
	bool	skipSubSurfaceScattering;
	bool	skipAutosprites;
	bool	skipOcclusionBaseModel;
	bool	skipBlendedSurfaces;
	bool	skipDistortionSurfaces;
	bool	skipEmissiveSurfaces;
	bool	skipOcclusionSurfaces;
	bool	skipGuis;
	bool	skipInGameGuis;
	bool	skipAdaptiveGlare;
	bool	skipPostProcess;
	bool	skipEmissiveGlare;
	bool	skipBlendLights;
	bool	skipSlowLights;
	bool	skipSelfShadows;
	bool	skipAmbientOcclusion;
	bool	skipFog;
	bool	skipGodRays;
	bool	skipHazeFlare;
	bool	skipPostSpecular;
	bool	skipNodeCPUCulling;
	bool	skipNodeGPUCulling;
	bool	skipAreaCPUCulling;
	bool	skipAreaGPUCulling;
	bool	skipModelCPUCulling;
	bool	skipModelGPUCulling;
	bool	skipLightCPUCulling;
	bool	skipLightGPUCulling;
	bool	skipSurfaceCPUCulling;
	bool	skipModelRangeCulling;
	bool	skipLightRangeCulling;
	bool	screenSpaceReflections;
	bool	singleWorldArea;
	bool	singleLight;
	bool	singleModel;
	bool	singleSurface;
	bool	singleGuiSurface;
	bool	singleDimShadow;
	bool	singleSelfShadow;
	bool	lockView;
	bool	forceTwoSidedDepth;
	bool	useAmbientEnv;
	bool	feedbackBGRA;
	bool	showOcclusionBoxes;
	bool	forceOcclusionBoxQueries;
	uint32	numViews;
	bool	generateMipMaps;
	uint64	guiFrameCount;
	uint32	queryThreshold;
	bool	doPreZMask;
	bool	dimShadowUseQuery;
	bool	dimShadowDepthBoundsTest;
	uint32	dimShadowResolution;
	uint32	dimShadowResolutionCap;
	bool	dimShadowForceHighQuality;
	bool	showDimShadows;
	float	dimShadowMaxLixelsPerUnit;
	float	dimShadowLixelScale;
	float	dimShadowDensity;
	float	dimShadowPolyOfsUnits;
	float	dimShadowPolyOfsFactor;
	float	dimShadowMaxVisibleRange;
	float	dimShadowFadeVisibilityRange;
	float	dimShadowSkipRangeCulling;
	uint32	selfShadowResolution;
	uint32	selfShadowResolutionCap;
	float	selfShadowDensity;
	float	selfShadowPolyOfsUnits;
	float	selfShadowPolyOfsFactor;
	float	selfShadowMaxVisibleRange;
	float	selfShadowFadeVisibilityRange;
	float	selfShadowLixelScale;
	float	selfShadowSkipRangeCulling;
	float	subSurfaceScatteringMaxVisibilityRange;
	float	subSurfaceScatteringFadeVisibilityRange;
	float	subSurfaceScatteringSkipRangeCulling;
	bool	sortBlendedSurfacesBackToFront;
	bool	useLightScissors;
	bool	useLightDepthBoundsTest;
	bool	useDeferredSlowMapLighting;
	bool	showLightScissors;
	bool	showBlendedLights;
	int		postProcessDofMode;
	bool	sortCoverage;
	bool	sortSSS;
	bool	sortSkybox;
	bool	sortBackgroundFar;
	bool	sortBackground;
	bool	sortBackgroundEmit;
	bool	sortEmit;
	bool	sortEmitOnly;
	bool	sortLight;
	bool	sortDecal;
	bool	sortTransSort;
	bool	sortTrans;
	bool	sortWater;
	bool	sortPerturber;
	bool	sortLast;
	float	viewNearZ;
	float	viewFarZ;
	idVec2	guiOriginOffset;
	const idDeclRenderProg * progShowGuiOverdraw;
	idVec4		clearColor;
	idVec3		viewOrigin;
	idBounds	viewBounds;
	idBounds	detailBounds;
};

class idRender {
/*
SetupRenderPassParms( renderPassParms_t, renderPass_t, idScreenRect );
UpdateShadowBufferOptions();
SetRenderSize@idRender@@AEAAXPEAVidRenderView@@AEBVidScreenRect@@@Z 42    PA0  ?
PrintStats@idRender@@AEBAXXZ r    àE  ?
SetupAdditiveEnvironmentParms@idRender@@AEAAXAEAUenvBlend_t@@PEBVidEnvBlendParms@@PEBVidParmBlock@@PEAV4@@Z eV    ÌD  ?
SetupDynamicEnvironment@idRender@@AEAAXAEBVidVec3@@AEAUenvBlend_t@@PEAVidParmBlock@@2PEBV4@@Z   k*    PÆD  ?
RenderSingleView( hdc, world_, renderView_, screenRect );
SetupCaptureParms@idRender@@AEAAXAEAUcaptureParms_t@@W4renderCapture_t@@AEBVidScreenRect@@H_N@Z u*     Pã¯  ?
GetLastUsedRenderHeight@idRender@@QEAAHXZ   E6     E  ?
GetLastUsedRenderWidth@idRender@@QEAAHXZ >    °ªF  ?
CaptureToViewColor@idRender@@QEAAXAEBVidScreenRect@@H@Z e~     E  ?
RenderGuiModels@idRender@@QEAAXPEBQEAVidRenderModelCommitted@@HPEBVidRenderDestination@@AEBVidScreenRect@@M@Z   ñv    E 
RenderSingleView@idRender@@QEAAXPEAXPEBVidRenderWorldCommitted@@PEAVidRenderView@@AEBVidScreenRect@@@Z  @^    Ð E  ?
InitSettings@idRender@@QEAAXPEBVidRenderWorldCommitted@@PEBVidRenderView@@H@Z   @.      b«  ?
WaitAndSwap@idRender@@QEAAXPEAX_N@Z A2      G  ?

*/

	void	InitSettings( class idRenderWorldCommitted *, idRenderView *);
	void	RenderGuiModels();
	void	RenderSingleView();
	void	CaptureToViewColor();
	void	CaptureToDebugColor();
	void	WaitAndSwap();
	void	CreateVisibleWorldSurfacesModel();
	void	GetLastUsedRenderWidth();
	void	GetLastUsedRenderHeight();
	void	SetupCaptureParms();
	void	SetupRenderDrawSurfParms();
	void	SetupRenderPassParms();
	void	SetupDynamicEnvironment();
	void	SetupAdditiveEnvironmentParms();
	void	UpdateShadowBufferOptions();
	void	SetRenderSize();
	void	PrintStats();
	void	BuildToolViewList();
	void	SimpleWorldSetup();
	void	PolygonClear();
	void	ColorByStencilBuffer();
	void	CountStencilBuffer();
	void	ShowDebugText();
	void	ShowDebugLines();
	void	ShowDebugPolygons();
	void	ShowDebugBounds();
	void	ShowDynamic();
	void	ShowViewModels();
	void	ShowTwoSided();
	void	ShowBlendedSurfaces();
	void	ShowStencilBuffer();
	void	ShowLights();
	void	ShowLightCount();
	void	ShowBlendedSurfaceTransSortOverdraw();
	void	ShowMegaTexture();
	void	ShowSurfaceBounds();
	void	ShowTestVMTR();
	void	ShowLoadedImages();
	void	ShowDestinationAlpha();
	void	ShowIntensity();
	void	ShowDepthBuffer();
	void	ShowCracks();
	void	ShowTris();
	void	ShowOcclusionTestTriangles();
	void	ShowOcclusionBaseModel();
	void	ShowManifolds();
	void	ShowTJunctions();
	void	ShowDuplicateTriangles();
	void	ShowSpecularTriangles();
	void	ShowWorldAreas();
	void	ShowPVS();
	void	ShowTrace();
	void	ShowVmtrChannel();
	void	ShowTangentSpace();
	void	ShowTextureSpace();
	void	ShowTextureDistortion();
	void	ShowVertexColor();
	void	ShowGamma();
	void	ShowGammaBias();
	void	ShowGlobalShadows();
	void	ShowTextureReconstruct();
	void	ShowGammaCalibrationTest();
	void	ShowAutosprit();
	void	ShowModelGroupMasters();
	void	ShowSurfaceInfo();
	void	ShowTraceWorld();
	void	ShowModelLightingInfo();
	void	ShowEnvironments();
	void	ShowTestImage();
	void	RenderDebugTools();
	void	ShowTexturePolarity();
	void	ShowTexelDensity();
	void	ShowNormals();
	void	ShowEdges();
	void	ShowTriangleInfo();
	void	ShowNonTwoManifoldEdge();
	void	ShowTestCubeImage();
	void	DrawTextLength();
	void	InternalDrawText();
	void	RenderDebugTools_NonPortable();

	int		tempConsidered;
	int		areaNodeBoundsCulled;
	int		worldAreaSubspaceBoundsCulled;
	int		worldAreaGeometryBoundsCulled;
	int		modelDimShadowState;
	int		nodeWorldAreas;
	int		viewWorldAreas;
	int		viewWorldSurfaces;
	int		viewModelSortKeys;
	int		viewModels;
	int		viewModelsPreZ;
	int		viewLights;
	int		viewGodrayModels;
	int		occlusionTestNodes;
	int		occlusionTestWorldAreas;
	int		occlusionTestModels;
	int		occlusionTestLights;
	int		drawSurfs;
	int		sortedDrawSurfs;
	int		depthSortedDrawSurfs;
	int		dimShadowModelSlaves;
	uint	numNodeWorldAreas;
	uint	numViewWorldAreas;
	uint	numViewWorldSurfaces;
	uint	numViewModels;
	uint	numViewModelsPreZ;
	uint	numViewSceneModels;
	uint	numViewGodrayModels;
	uint	numViewLights;
	uint	numOcclusionTestNodes;
	uint	numOcclusionTestWorldAreas;
	uint	numOcclusionTestModels;
	uint	numOcclusionTestLights;
	uint	numDrawSurfs;
	uint	numDepthSortedDrawSurfs;
	uint	numShadows;
	uint	numDimShadows;
	uint	numSelfShadows;
	int		renderViewModels;
	int		renderPasses;
	renderSettings_t	settings;
	int		renderViewObject;
	int		world;
	int		worldSpace;
	int		viewSpace;

};