
#include "../../tr_local.h"
//#include "renderer\RenderContext.h"

extern idCVar stereoRender_swapEyes;
extern idCVar r_skipInteractionFastPath;

// complex light / surface interactions are broken up into multiple passes of a
// simple interaction shader
struct drawInteraction_t {
	const drawSurf_t * 			surf;

	const idDeclRenderParm *	bumpImage;
	const idDeclRenderParm *	diffuseImage;
	const idDeclRenderParm *	specularImage;

	const idDeclRenderParm *	diffuseColor;	// may have a light color baked into it
	const idDeclRenderParm *	specularColor;	// may have a light color baked into it
	const idDeclRenderParm *	vertexColor;	// applies to both diffuse and specular

												// these are loaded into the vertex program
	const idDeclRenderParm *	bumpMatrix[ 2 ];
	const idDeclRenderParm *	diffuseMatrix[ 2 ];
	const idDeclRenderParm *	specularMatrix[ 2 ];

	int ambientLight;	// use tr.ambientNormalMap instead of normalization cube map

	drawInteraction_t()
	{
		bumpImage = renderProgManager.GetRenderParm( RENDERPARM_BUMPMAP );
		diffuseImage = renderProgManager.GetRenderParm( RENDERPARM_DIFFUSEMAP );
		specularImage = renderProgManager.GetRenderParm( RENDERPARM_SPECULARMAP );

		// may have a light color baked into it

		diffuseColor = renderProgManager.GetRenderParm( RENDERPARM_DIFFUSEMODIFIER );
		specularColor = renderProgManager.GetRenderParm( RENDERPARM_SPECULARMODIFIER );

		// applies to both diffuse and specular
		vertexColor = renderProgManager.GetRenderParm( RENDERPARM_VERTEXCOLOR_MAD );

		// these are loaded into the vertex program

		bumpMatrix[ 0 ] = renderProgManager.GetRenderParm( RENDERPARM_BUMPMATRIX_S );
		bumpMatrix[ 1 ] = renderProgManager.GetRenderParm( RENDERPARM_BUMPMATRIX_T );

		diffuseMatrix[ 0 ] = renderProgManager.GetRenderParm( RENDERPARM_DIFFUSEMATRIX_S );
		diffuseMatrix[ 1 ] = renderProgManager.GetRenderParm( RENDERPARM_DIFFUSEMATRIX_T );

		specularMatrix[ 0 ] = renderProgManager.GetRenderParm( RENDERPARM_SPECULARMATRIX_S );
		specularMatrix[ 1 ] = renderProgManager.GetRenderParm( RENDERPARM_SPECULARMATRIX_T );
	}

	void Clear()
	{
		bumpImage->Set( ( idImage * )NULL );
		diffuseImage->Set( ( idImage * )NULL );
		specularImage->Set( ( idImage * )NULL );

		diffuseColor->Set( 1.0f );
		specularColor->Set( 0.0f );

		vertexColor->Set( 0, 1, 0, 0 ); //  SVC_IGNORE;

		ambientLight = 0;
	}
};
void RB_DrawComplexMaterialInteraction( drawInteraction_t &, const float* surfaceRegs, const idMaterial* const );

ID_INLINE void RB_ResetViewportAndScissorToDefaultCamera( const idRenderView * const viewDef )
{
	// set the window clipping
	GL_Viewport(
		viewDef->GetViewport().x1,
		viewDef->GetViewport().y1,
		viewDef->GetViewport().GetWidth(),
		viewDef->GetViewport().GetHeight() );

	// the scissor may be smaller than the viewport for subviews
	GL_Scissor(
		viewDef->GetViewport().x1 + viewDef->GetScissor().x1,
		viewDef->GetViewport().y1 + viewDef->GetScissor().y1,
		viewDef->GetScissor().GetWidth(),
		viewDef->GetScissor().GetHeight() );

	backEnd.currentScissor = viewDef->GetScissor();
}

void RB_BakeTextureMatrixIntoTexgen( idPlane lightProject[ 3 ], const float* textureMatrix );
void RB_ResetViewportAndScissorToDefaultCamera( const idRenderView * const );
ID_INLINE void RB_SetScissor( const idScreenRect & scissorRect )
{
	if( !backEnd.currentScissor.Equals( scissorRect ) )
	{
		GL_Scissor(
			backEnd.viewDef->GetViewport().x1 + scissorRect.x1,
			backEnd.viewDef->GetViewport().y1 + scissorRect.y1,
			scissorRect.GetWidth(),
			scissorRect.GetHeight() );

		backEnd.currentScissor = scissorRect;

		RENDERLOG_PRINT( "GL_Scissor( %i, %i, %i, %i )\n", backEnd.viewDef->GetViewport().x1 + scissorRect.x1,
														   backEnd.viewDef->GetViewport().y1 + scissorRect.y1,
														   scissorRect.GetWidth(), scissorRect.GetHeight() );
	}
}
void RB_SetBaseRenderDestination( const idRenderView * const, const viewLight_t * const = nullptr );
ID_INLINE void RB_ResetBaseRenderDest( const bool hdrIsActive )
{
	if( hdrIsActive ) {
		GL_SetRenderDestination( renderDestManager.renderDestBaseHDR );
	} else {
		GL_SetRenderDestination( renderDestManager.renderDestBaseLDR );
	}
}

void RB_SetupForFastPathInteractions();

void RB_PrepareStageTexturing( const materialStage_t*, const drawSurf_t* );
void RB_FinishStageTexturing( const materialStage_t*, const drawSurf_t* );

////////////////////////////////////////////////////////////////////////
// RenderPasses

void RB_FillDepthBufferFast( const drawSurf_t * const * drawSurfs, int numDrawSurfs );

void RB_FillGBuffer( const idRenderView * const, const drawSurf_t * const *, int );
void RB_DrawInteractionsDeferred( const idRenderView * const );

void RB_AmbientPass( const idRenderView *, const drawSurf_t* const* drawSurfs, int numDrawSurfs );
void RB_DrawInteractionsForward( const idRenderView * const );

void RB_FillShadowBuffer( const drawSurf_t * const drawSurfs, const viewLight_t * const );
void RB_SetupShadowCommonParms( const lightType_e, const uint32 shadowLOD );
void RB_SetupShadowDrawMatrices( const lightType_e, const drawSurf_t * const );

void RB_StencilShadowPass( const drawSurf_t* const drawSurfs, const viewLight_t * const );
void RB_StencilSelectLight( const viewLight_t* const );

int  RB_DrawTransMaterialPasses( const drawSurf_t* const* const drawSurfs, const int numDrawSurfs, const float guiStereoScreenOffset, const int stereoEye );

void RB_FogAllLights();

void RB_CalculateAdaptation();
void RB_Tonemap();

extern idCVar r_useBloom;
void RB_Bloom();
void RB_BloomNatural();
void RB_BloomNatural2();

extern idCVar r_useSSAO;
extern idCVar r_ssaoDebug;
void RB_SSAO();

extern idCVar r_useSSGI;
extern idCVar r_ssgiDebug;
void RB_SSGI();

extern idCVar r_motionBlur;
void RB_MotionBlur();

void RB_SMAA( const idRenderView * );




/*
template< typename T >
class idPresentablePtr {
	// operator=
	// operator==
	// operator->
	// operator class idPresentable *
	IsValid();
	GetPresentable();
	GetIndex();
	Serialize();
	GetSpawnId();
	SetPresentable();
	uint32 spawnId;

};

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