
#pragma hdrstop
#include "precompiled.h"

#include "tr_local.h"
#include "Framebuffer.h"

#include "../sys/win32/win_local.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// renderer\jobs\render\RenderShadowBuffer.cpp

/*
RenderView.cpp

idList<idDeclRenderParm const * __ptr64,5>
idList<parmValue_t,5>

auto prepareFullScreePass = []()
{
	GL_ModelViewMatrix.LoadIdentity();
	GL_State( GLS_DEFAULT );
	GL_ProjectionMatrix.Push();
	GL_ProjectionMatrix.LoadIdentity();
	GL_ProjectionMatrix.Ortho( 0, 1, 0, 1, -1, 1 );
};


	\engine\renderer\jobs\render\SetupModelMVP.obj
	\engine\renderer\jobs\render\SetupModelMVP.obj
	\engine\renderer\jobs\render\SetupColorGrading.obj
	\engine\renderer\jobs\render\SetupColorGrading.obj
	\engine\renderer\jobs\render\RenderWalkBSP.obj
	\engine\renderer\jobs\render\RenderWalkBSP.obj
	\engine\renderer\jobs\render\RenderState.obj
	\engine\renderer\jobs\render\RenderState.obj
	\engine\renderer\jobs\render\RenderSortDepth.obj
	\engine\renderer\jobs\render\RenderSortDepth.obj
	\engine\renderer\jobs\render\RenderSort.obj
	\engine\renderer\jobs\render\RenderSort.obj
	\engine\renderer\jobs\render\RenderShadowBuffer.obj
	\engine\renderer\jobs\render\RenderShadowBuffer.obj
	\engine\renderer\jobs\render\RenderSelfShadows.obj
	\engine\renderer\jobs\render\RenderSelfShadows.obj
	\engine\renderer\jobs\render\RenderScreenSpaceReflections.obj
	\engine\renderer\jobs\render\RenderScreenSpaceReflections.obj
	\engine\renderer\jobs\render\RenderPostProcess.obj
	\engine\renderer\jobs\render\RenderPostProcess.obj
	\engine\renderer\jobs\render\RenderPasses.obj
	\engine\renderer\jobs\render\RenderPasses.obj
	\engine\renderer\jobs\render\RenderOcclusion.obj
	\engine\renderer\jobs\render\RenderOcclusion.obj
	\engine\renderer\jobs\render\RenderLights.obj
	\engine\renderer\jobs\render\RenderLights.obj
	\engine\renderer\jobs\render\RenderHazeFlare.obj
	\engine\renderer\jobs\render\RenderHazeFlare.obj
	\engine\renderer\jobs\render\RenderGodRays.obj
	\engine\renderer\jobs\render\RenderGodRays.obj
	\engine\renderer\jobs\render\RenderGlare.obj
	\engine\renderer\jobs\render\RenderGlare.obj
	\engine\renderer\jobs\render\RenderGather.obj
	\engine\renderer\jobs\render\RenderGather.obj
	\engine\renderer\jobs\render\RenderFog.obj
	\engine\renderer\jobs\render\RenderFog.obj
	\engine\renderer\jobs\render\RenderDynamicEnv.obj
	\engine\renderer\jobs\render\RenderDynamicEnv.obj
	\engine\renderer\jobs\render\RenderDynamicColorCorrection.obj
	\engine\renderer\jobs\render\RenderDynamicColorCorrection.obj
	\engine\renderer\jobs\render\RenderDrawSurf.obj
	\engine\renderer\jobs\render\RenderDrawSurf.obj
	\engine\renderer\jobs\render\RenderDimShadows.obj
	\engine\renderer\jobs\render\RenderDimShadows.obj
	\engine\renderer\jobs\render\RenderDepth.obj
	\engine\renderer\jobs\render\RenderDepth.obj
	\engine\renderer\jobs\render\RenderCull.obj
	\engine\renderer\jobs\render\RenderCull.obj
	\engine\renderer\jobs\render\RenderCapture_OGL_PC.obj
	\engine\renderer\jobs\render\RenderCapture_OGL_PC.obj
	\engine\renderer\jobs\render\RenderCapture_D3D_PC.obj
	\engine\renderer\jobs\render\RenderCapture_D3D_PC.obj
	\engine\renderer\jobs\render\RenderCapture.obj
	\engine\renderer\jobs\render\RenderCapture.obj
	\engine\renderer\jobs\render\ParmState_alloc.obj
	\engine\renderer\jobs\render\ParmState_alloc.obj
	\engine\renderer\jobs\render\ParmState.obj
	\engine\renderer\jobs\render\ParmState.obj
	\engine\renderer\jobs\render\GraphicsAPIWrapper_OGL_PC.obj
	\engine\renderer\jobs\render\GraphicsAPIWrapper_OGL_PC.obj
	\engine\renderer\jobs\render\GraphicsAPIWrapper_D3D_PC.obj
	\engine\renderer\jobs\render\GraphicsAPIWrapper_D3D_PC.obj












*/
#if 0

enum parmType_t
{
	PT_VECTOR,
	PT_TEXTURE_2D,
	PT_TEXTURE_3D,
	PT_TEXTURE_CUBE,
	PT_TEXTURE_SHADOW_2D,
	PT_TEXTURE_SHADOW_3D,
	PT_TEXTURE_SHADOW_CUBE,
	PT_TEXTURE_DIM_SHADOW_2D,
	PT_TEXTURE_MULTISAMPLE_2D,
	PT_SAMPLER,
	PT_PROGRAM,
	PT_STRING,
	PT_MAX
};

struct expOp_t 
{
	parmType_t	type;
	int16		parmIndexDest;
	int16		parmIndexA;
	int16		parmIndexB;

	void Clear();
	void SetOpType();
	void SetConstantType();
	void SetDestWriteMask();
	void GetOpType();
	void GetConstantType();
	void GetDestWriteMask();
	void GetDest();
	void GetA();
	void GetB();

	void operator == ( const expOp_t& );
	void operator != ( const expOp_t& );
};

union parmValue_t 
{
	idVec4				value;
	idImage				*image;
	class idSampler		*sampler;
	idDeclRenderProg	*program;
	idStr				string;
	int32				swizzle;

	bool operator== ( const parmValue_t& );
	bool operator!= ( const parmValue_t& );
};

// renderer\ParmBlock.cpp
class idParmBlock 
{
	void Clear();
	void CopyFrom( other );
	void Num();
	void GetOp();
	void GetConstant();
	void AddOp( op constant );
	void Append( block );
	void SetParm( parm other );
	void SetParm( parm other parmValue );
	void SetFloat();
	void SetVec2();
	void SetVec3();
	void SetVec4();
	void SetFloat4();
	void SetImage( parm image );
	void SetSampler();
	void SetProgram();
	void SetString();
	void ClearParm( parm );
	void SetsRenderParm( parm );
	void RemoveRenderParmsNotNeededInProduction();
	void IsEqualSimple();
	void GetFloat( parm parentBlock );
	void GetInteger( parm parentBlock );
	void GetVector( parm parentBlock );
	void GetImage( parm parentBlock );
	void GetSampler( parm parentBlock );
	void GetProgram( parm parentBlock );
	void GetString( parm parentBlock );
	void Print();
	void WriteString( suffix );
	void Save( fp );
	void Load( fp num usingTempOps str temp atom );
	void MapUpdate();
	void MapMakeDirty();

	void operator= ( idParmBlock & other );
	void operator== ( idParmBlock & other );

	idList<expOp_t, TAG_CRAP>		ops;
	idList<parmValue_t, TAG_CRAP>	constants;

	void RemoveRedundantOperations( opIsRedundant );
	void MapRebuild();
	void GetRenderParmFromOps_r( parmValue numOps parmIndex parentBlock b a );
	void WriteConstant( parmType parmValue );
	void WriteTerm( str term );
};

class idRenderParm {
	void operator == ( idRenderParm );
	uint32		parm;
	parmValue_t	value;
};

// renderer\ParmBlockParser.cpp
class idParmBlockParser 
{
	idParmBlockParser( src block_ token name temp constantValue atom );
	~idParmBlockParser();

	void GetExpressionOp( src, constant );//
	void GetConstantScalar();
	void GetConstantVector( src );//
	void GetConstant();
	void GetExpressionTemporaryParm();//
	void EmitOp( src table opType );//
	void ParseEmitOp( src opType priority );//
	void ParseIntrinsic( src opType );//
	void ParseTerm( src fakeToken token vec constantValue tempToken );//
	void ParseExpressionPriority( src priority token );//
	void ParseExpression();
	void FindFreeTemporary();
	void CleanupTemporaries( opIsRedundant );//

	static const int MAX_STATIC_PARM_OPS	= 0;
	static const int MAX_EXPRESSION_TEMPS	= 0;

	int tempParms;
	int usedTempParms;
	int ops;
	int constants;
	int defaultConstant;
};
// renderer\ParmBlockOptimizer.cpp
// idParmBlockOptimizer::OptimizeParmBlockForUsedParms( parmBlock )

class idDeclRenderParm {
public:
	idDeclRenderParm();
	~idDeclRenderParm();

	void DefaultDefinition();
	void Parse();
	void Print();
	void List();

	void ParseStringToValue();
	void ParseVectorConstant();
	void ParseImageLine();

	void SetTemporary();
	void GetParmIndex();
	void GetType();
	void GetCreator();
	void IsCubeFilterTexture();
	void GetDefaultFloat();
	void GetDefaultVec2();
	void GetDefaultVec3();
	void GetDefaultVec4();
	void GetDefaultImage();
	void GetDefaultSampler();
	void GetDefaultProgram();
	void GetDefaultString();
	void SetDeclaredFloat();
	void GetDeclaredFloat();


private:
	int parmIndex;
	int parmType;
	int creator;
	int cubeFilterTexture;
	int declaredValue;
	int edit;
	int editRange;

	void ParseValue();

};

template< typename Type >
class idHeapArray 
{
	void Alloc();
	void Clear();
	void SetBuffer();
	void operator[];
	void Ptr();
	void Size();
	void Num();
	void Zero();
	void MemcpyInto();

private:
	Type *	buffer;
	uint32	num;
	bool	bufferOwnedByObject;
};

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
	int		numViews;
	bool	generateMipMaps;
	int		guiFrameCount;
	int		queryThreshold;
	int		doPreZMask;
	int		dimShadowUseQuery;
	int		dimShadowDepthBoundsTest;
	int		dimShadowResolution;
	int		dimShadowResolutionCap;
	int		dimShadowForceHighQuality;
	int		showDimShadows;
	int		dimShadowMaxLixelsPerUnit;
	int		dimShadowLixelScale;
	int		dimShadowDensity;
	int		dimShadowPolyOfsUnits;
	int		dimShadowPolyOfsFactor;
	int		dimShadowMaxVisibleRange;
	int		dimShadowFadeVisibilityRange;
	int		dimShadowSkipRangeCulling;
	int		selfShadowResolution;
	int		selfShadowResolutionCap;
	int		selfShadowDensity;
	int		selfShadowPolyOfsUnits;
	int		selfShadowPolyOfsFactor;
	int		selfShadowMaxVisibleRange;
	int		selfShadowFadeVisibilityRange;
	int		selfShadowLixelScale;
	int		selfShadowSkipRangeCulling;
	int		subSurfaceScatteringMaxVisibilityRange;
	int		subSurfaceScatteringFadeVisibilityRange;
	int		subSurfaceScatteringSkipRangeCulling;
	int		sortBlendedSurfacesBackToFront;
	int		useLightScissors;
	int		useLightDepthBoundsTest;
	int		useDeferredSlowMapLighting;
	int		showLightScissors;
	int		showBlendedLights;
	int		postProcessDofMode;
	int		sortCoverage;
	int		sortSSS;
	int		sortSkybox;
	int		sortBackgroundFar;
	int		sortBackground;
	int		sortBackgroundEmit;
	int		sortEmit;
	int		sortEmitOnly;
	int		sortLight;
	int		sortDecal;
	int		sortTransSort;
	int		sortTrans;
	int		sortWater;
	int		sortPerturber;
	int		sortLast;
	int		viewNearZ;
	int		viewFarZ;
	int		guiOriginOffset;
	int		progShowGuiOverdraw;
	int		clearColor;
	int		viewOrigin;
	int		viewBounds;
	int		detailBounds;
};

struct renderSortDepthParms_t
{
	int	settings;
	int	renderView;
	int	viewModels;
	int	viewModelSortKeys;
	int	numViewModels;
	int	sortedViewModels;
	int	renderViewModels;

};

struct renderDepthParms_t 
{
	int	settings;
	int	renderView;
	int	viewWorldAreas;
	int	numViewWorldAreas;
	int	viewWorldSurfaces;
	int	numViewWorldSurfaces;
	int	viewModels;
	int	renderViewModels;
	int	binaryModelState;
	int	renderDestDepth;
	int	renderDestDefault;
	int	occlusionBaseTriangles;
	idDeclRenderProg *	progOccluderDepthOnly;
	idDeclRenderProg *	progPreZ;
	idDeclRenderProg *	progPreZAlpha;
	idDeclRenderProg *	progPreZSecondary;
	idDeclRenderProg *	progPreZWound;
	int	imgBlack;
	int	rpPostSpecularEnabled;
	int	extraClears;
	int	skipDepthViewStencil;
	int	skipFirstPersonModels;
	int	renderExtraWorldPassDebug;
	int	splitPlaneDistance;
	int	occlusionState;
	int	drawSurfs;
	int	numDrawSurfs;
};

struct shadowCaster_t {
	int model;
	int bounds;
	int surfaces;
	int numSurfaces;
};

struct lightShadow_t 
{
	int shadowProjection;
	int worldPosToShadowMap;
};

// idThinVector<shadowDrawEntry_t *,64,1>
struct shadowDrawEntry_t {
	idMaterial *material;
	idDeclRenderProg *prog;
};
//  idArray<shadowDrawEntry_t,128>

struct shadowBufferOptions_t
{
	int	shadowBufferSize;
	int	occluderFacing;
	int	singleSide;
	int	sliceStep;
	int	polyOfsFactor;
	int	polyOfsUnits;
	int	parallelPolyOfsFactor;
	int	parallelPolyOfsUnits;
	bool	skipShadows;
	bool	skipShadowModelSort;
	bool	skipShadowOccluders;
	bool	skipShadowModelCPUCulling;
	bool	skipShadowSurfaceCPUCulling;
};

struct renderShadowBufferParms_t
{
	const renderSettings_t *	settings;
	const idRenderView *		renderView;
	int							worldAreaSubspaceBounds;
	int							worldAreaGeometryBounds;
	int							worldAreaSubspaceBoundsCulled;
	int							worldModel;
	int							binaryModelState;
	int							committedRenderModels;
	uint32						numCommittedRenderModels;
	const idDeclRenderParm *	rpShadowBufferResolution;
	const idRenderDestination *	renderDestDefault;
	const idRenderDestination *	renderDestShadow;
	shadowBufferOptions_t		options;
	int							modelConsideredAsOccluder;

};

struct captureParms_t 
{
	int							capture;
	const renderSettings_t *	settings;
	uint32						renderWidth;
	uint32						renderHeight;
	uint32						windowWidth;
	uint32						windowHeight;
	bool						forceFullVirtualTextureLoad;
	int							viewIndex;
	const idRenderDestination *	renderDestDefault;
	idImage *					imgViewDepth;
	const idRenderDestination *	renderDestViewColor;
	const idRenderDestination *	renderDestViewDepth;
	const idRenderDestination *	renderDestViewNormal;
	const idRenderDestination *	renderDestFeedback;
	int	feedbackBuffer;
	int	feedbackBufferObject;
	const idDeclRenderParm * rpFeedbackColor;
	const idRenderDestination *	renderDestGui;
	bool	createMipMaps;
	const idRenderDestination *	renderDestMip;
	const idRenderDestination *	renderDestMip3;
	const idDeclRenderParm * rpGlareMap;
	const idDeclRenderProg *	progGlareScale;
	const idDeclRenderProg *	progOverlappedDownSample;
	const idDeclRenderProg *	progFeedbackDownSample;
	const idDeclRenderProg *	progCaptureDepth;
	const idDeclRenderProg *	progCopyToMip01;
	const idDeclRenderProg *	progCopyToMip012;
	const idDeclRenderParm * rpViewColorMap;
	const idDeclRenderParm * rpMipMapSampler;
	int	mipMapSamplers;
	const idDeclRenderParm * rpTargetDepth;
	const idDeclRenderParm * rpMipGenSrc;
	const idDeclRenderParm * rpMipGenDst0;
	const idDeclRenderParm * rpMipGenDst1;
	const idDeclRenderParm * rpMipGenDst2;
	int	unitSquareTris;
	int	imgBlack;
};

struct renderSelfShadowsParms_t 
{
	const renderSettings_t *	settings;
	const idRenderView *		renderView;
	int selfShadowModelSlaves;
	int committedRenderModels;
	uint32						numCommittedRenderModels;
	int dimShadowModels;
	int extrudeBox;
	const idRenderDestination *	renderDestSelfShadow;
	const idRenderDestination *	renderDestDefault;
	captureParms_t *			captureParms;

	const idDeclRenderParm *	rpLightDepthCompareMap;
	const idDeclRenderParm *	rpShadowBufferResolution;
	const idDeclRenderParm *	rpSelfShadowMap;
	const idDeclRenderParm *	rpSelfShadowFade;
	const idDeclRenderParm *	rpSelfShadowEnable;
	const idDeclRenderParm *	rpModelPosToShadowMapS;
	const idDeclRenderParm *	rpModelPosToShadowMapT;
	const idDeclRenderParm *	rpModelPosToShadowMapR;
	const idDeclRenderParm *	rpModelPosToShadowMapQ;
	const idDeclRenderParm *	rpInvShadowProjX;
	const idDeclRenderParm *	rpInvShadowProjY;
	const idDeclRenderParm *	rpInvShadowProjZ;
	const idDeclRenderParm *	rpInvShadowProjW;
	const idDeclRenderParm *	rpShadowProjectionDepth;

	const idDeclRenderProg *	progSelfShadowCreate;
	const idDeclRenderProg *	progSelfShadowCreateAlpha;
	const idDeclRenderProg *	progSelfShadowCreateWound;
	const idDeclRenderProg *	progSelfShadowCreateSecondary;
	const idDeclRenderProg *	progSelfShadowSample;
	const idDeclRenderProg *	progSelfShadowSampleAlpha;
	const idDeclRenderProg *	progSelfShadowSampleSecondary;
	const idDeclRenderProg *	progSelfShadowSampleWound;
	idImage *					imgBlack;
	uint32						numSelfShadows;
};

/*
GL_ResetTextureState

GetPointLightViewMatrix( idRenderMatrix lightViewMatrix, idRenderMatrix baseMatrix );

GetSpotLightViewMatrix  idVec3 lightTarget, idVec3 lightRight, idVec3 lightUp, idRenderMatrix lightViewMatrix, idRenderMatrix baseMatrix );

GetSpotShadowProjectionMatrix(  zNear,  zFar,  tanFovX,  tanFovY,  idRenderMatrix projectionMatrix )

GetParallelLightViewMatrix(  idVec3, idRenderMatrix );

SetupWorldPosToShadowMap(   idRenderMatrix shadowProjection,  scale,  sBias,  tBias,   idRenderMatrix worldPosToShadowMap )

SetupShadowBufferMatrices  lightParms lightOrigin lightBounds viewOrigin  options shadowViewProjection  lightShadow  lightTransformMatrix
lightCenterMatrix  baseLightTransform  inverseLightTransformMatrix   shadowProjectionMatrix lightMatrix  lightViewMatrix
depthBoundsMax depthBoundsMin lightDirection

ShadowCasterQuickSort casters distances num hi lo

MAX_SHADOW_CASTERS

RenderShadowBuffer( renderShadowBufferParms_t parms, idRenderLightCommitted light, lightShadow_t lightShadow ) {

RESOLVE_TARGET_DEPTH

GetPointLightViewMatrix
GetSpotLightViewMatrix
GetParallelLightViewMatrix
GetSpotShadowProjectionMatrix
SetupWorldPosToShadowMap

SetupShadowBufferMatrices

ShadowCasterQuickSort


SetupShadowProjection
DistanceToBox idBounds idVec3
ComputeShadowLODDistance idBounds idVec3

lightShadow shadowViewProjection shadowCasters lightOriginCullBits distances
modelCullBits  unitCubeMVPMatrix unitCube inverseShadowMVP shadowMVP




RegenerateColorMips@@YAXPEBUrenderPostProcessParms_t@@PEBVidRenderDestination
CreateColorMips@@YAXPEBUcaptureParms_t@@PEBVidRenderDestination
RenderPostProcess_SlowDepthBlur@@YAXPEBUrenderPostProcessParms_t
RenderPostProcess_SeparatedDepthBlur@@YAXPEBUrenderPostProcessParms_t

RenderClearPass@@YAXPEBU renderPassParms_t
RenderSSSPass@@YAXPEBU renderPassParms_t

GL_ResetTextureState

RenderDrawSurf@idRenderDrawSurf@@QEAAXPEBU renderDrawSurfParms_t @@PEBUdrawSurf_t

RenderDrawSurfProgram@idRenderDrawSurf@@QEAAXPEBU renderDrawSurfParms_t @@PEBUdrawSurf_t@@PEBVidDeclRenderProg@@@Z
RenderBackgroundPass@@YAXPEBU renderPassParms_t
RenderBackgroundEmissivePass@@YAXPEBU renderPassParms_t
RenderEmissivePass@@YAXPEBU renderPassParms_t@@@Z
RenderEmissiveOnlyPass@@YAXPEBU renderPassParms_t@@@Z
RenderBlendPass@@YAXPEBU renderPassParms_t@@@Z
RenderDepthSortedPass@@YAXPEBU renderPassParms_t
RenderDistortionPass@@YAXPEBU renderPassParms_t


*/

#endif


idCVar r_sb_lightResolution( "r_sb_lightResolution", "1024", CVAR_RENDERER | CVAR_INTEGER, "Pixel dimensions for each shadow buffer, 64 - 2048" );
idCVar r_sb_viewResolution( "r_sb_viewResolution", "1024", CVAR_RENDERER | CVAR_INTEGER, "Width of screen space shadow sampling" );
idCVar r_sb_noShadows( "r_sb_noShadows", "0", CVAR_RENDERER | CVAR_BOOL, "don't draw any occluders" );
idCVar r_sb_usePbuffer( "r_sb_usePbuffer", "1", CVAR_RENDERER | CVAR_BOOL, "draw offscreen" );
idCVar r_sb_jitterScale( "r_sb_jitterScale", "0.006", CVAR_RENDERER | CVAR_FLOAT, "scale factor for jitter offset" );
idCVar r_sb_biasScale( "r_sb_biasScale", "0.0001", CVAR_RENDERER | CVAR_FLOAT, "scale factor for jitter bias" );
idCVar r_sb_samples( "r_sb_samples", "4", CVAR_RENDERER | CVAR_INTEGER, "0, 1, 4, or 16" );
idCVar r_sb_randomize( "r_sb_randomize", "1", CVAR_RENDERER | CVAR_BOOL, "randomly offset jitter texture each draw" );
// polyOfsFactor causes holes in low res images
idCVar r_sb_polyOfsFactor( "r_sb_polyOfsFactor", "2", CVAR_RENDERER | CVAR_FLOAT, "polygonOffset factor for drawing shadow buffer" );
idCVar r_sb_polyOfsUnits( "r_sb_polyOfsUnits", "3000", CVAR_RENDERER | CVAR_FLOAT, "polygonOffset units for drawing shadow buffer" );
idCVar r_sb_occluderFacing( "r_sb_occluderFacing", "0", CVAR_RENDERER | CVAR_INTEGER, "0 = front faces, 1 = back faces, 2 = midway between" );
// r_sb_randomizeBufferOrientation?

idCVar r_sb_frustomFOV( "r_sb_frustomFOV", "92", CVAR_RENDERER | CVAR_FLOAT, "oversize FOV for point light side matching" );
idCVar r_sb_showFrustumPixels( "r_sb_showFrustumPixels", "0", CVAR_RENDERER | CVAR_BOOL, "color the pixels contained in the frustum" );
idCVar r_sb_singleSide( "r_sb_singleSide", "-1", CVAR_RENDERER | CVAR_INTEGER, "only draw a single side (0-5) of point lights" );
idCVar r_sb_useCulling( "r_sb_useCulling", "1", CVAR_RENDERER | CVAR_BOOL, "cull geometry to individual side frustums" );
idCVar r_sb_linearFilter( "r_sb_linearFilter", "1", CVAR_RENDERER | CVAR_BOOL, "use GL_LINEAR instead of GL_NEAREST on shadow maps" );

idCVar r_sb_screenSpaceShadow( "r_sb_screenSpaceShadow", "1", CVAR_RENDERER | CVAR_BOOL, "build shadows in screen space instead of on surfaces" );

idCVar r_hdr_useFloats( "r_hdr_useFloats", "0", CVAR_RENDERER | CVAR_BOOL, "use a floating point rendering buffer" );
idCVar r_hdr_exposure( "r_hdr_exposure", "1.0", CVAR_RENDERER | CVAR_FLOAT, "maximum light scale" );
idCVar r_hdr_bloomFraction( "r_hdr_bloomFraction", "0.1", CVAR_RENDERER | CVAR_FLOAT, "fraction to smear across neighbors" );
idCVar r_hdr_gamma( "r_hdr_gamma", "1", CVAR_RENDERER | CVAR_FLOAT, "monitor gamma power" );
idCVar r_hdr_monitorDither( "r_hdr_monitorDither", "0.01", CVAR_RENDERER | CVAR_FLOAT, "random dither in monitor space" );


static float minScreenRadiusForShadowCaster = 0.01f;
idCVar r_smScreenRadiusForShadowCaster( "r_smScreenRadiusForShadowCaster", idStr( minScreenRadiusForShadowCaster ).c_str(), CVAR_RENDERER | CVAR_FLOAT, "cull shadow casters if they are too small, value is the minimal screen space bounding sphere radius" );

static float minScreenRadiusForShadowCasterRSM = 0.06f;
idCVar r_smScreenRadiusForShadowCasterRSM( "r_smScreenRadiusForShadowCasterRSM", idStr( minScreenRadiusForShadowCasterRSM ).c_str(), CVAR_RENDERER | CVAR_FLOAT, "cull shadow casters in the RSM if they are too small, values is the minimal screen space bounding sphere radius" );

idCVar r_smShadowFadeExponent( "r_smShadowFadeExponent", "0.25", CVAR_RENDERER | CVAR_FLOAT, "controls the rate at which shadows are faded out" );

/**
* Helper function to determine fade alpha value for shadows based on resolution. In the below ASCII art (1) is
* the MinShadowResolution and (2) is the ShadowFadeResolution. Alpha will be 0 below the min resolution and 1
* above the fade resolution. In between it is going to be an exponential curve with the values between (1) and (2)
* being normalized in the 0..1 range.
*
*
*  |    /-------
*  |  /
*  |/
*  1-----2-------
*
* @param	MaxUnclampedResolution		Requested resolution, unclamped so it can be below min
* @param	ShadowFadeResolution		Resolution at which fade begins
* @param	MinShadowResolution			Minimum resolution of shadow
*
* @return	fade value between 0 and 1
*/
float CalculateShadowFadeAlpha( const float MaxUnclampedResolution, const uint32 ShadowFadeResolution, const uint32 MinShadowResolution )
{
	// NB: MaxUnclampedResolution < 0 will return FadeAlpha = 0.0f. 

	float FadeAlpha = 0.0f;
	// Shadow size is above fading resolution.
	if( MaxUnclampedResolution > ShadowFadeResolution )
	{
		FadeAlpha = 1.0f;
	}
	// Shadow size is below fading resolution but above min resolution.
	else if( MaxUnclampedResolution > MinShadowResolution )
	{
		const float Exponent = r_smShadowFadeExponent.GetFloat();

		// Use the limit case ShadowFadeResolution = MinShadowResolution
		// to gracefully handle this case.
		if( MinShadowResolution >= ShadowFadeResolution )
		{
			const float SizeRatio = ( float )( MaxUnclampedResolution - MinShadowResolution );
			FadeAlpha = 1.0f - idMath::Pow( SizeRatio, Exponent );
		}
		else {
			const float InverseRange = 1.0f / ( ShadowFadeResolution - MinShadowResolution );
			const float FirstFadeValue = idMath::Pow16( InverseRange, Exponent );
			const float SizeRatio = ( float )( MaxUnclampedResolution - MinShadowResolution ) * InverseRange;
			// Rescale the fade alpha to reduce the change between no fading and the first value, which reduces popping with small ShadowFadeExponent's
			FadeAlpha = ( idMath::Pow16( SizeRatio, Exponent ) - FirstFadeValue ) / ( 1.0f - FirstFadeValue );
		}
	}
	return FadeAlpha;
}

// cull object if it's too small to be considered as shadow caster
bool CullShadowCaster( const idBounds & globalBounds )
{
	/** FOV based multiplier for cull distance on objects */
	float LODDistanceFactor;
	/** Square of the FOV based multiplier for cull distance on objects */
	float LODDistanceFactorSquared;

	float CurrentView_LODDistanceFactorSquared = LODDistanceFactorSquared;

	const float DistanceSquared = ( globalBounds.GetCenter() - backEnd.viewDef->GetOrigin() ).LengthSqr(); //NOTE view origin in world space!
	const bool bDrawShadowDepth = idMath::Square( globalBounds.GetRadius() ) > idMath::Square( minScreenRadiusForShadowCaster ) * DistanceSquared * CurrentView_LODDistanceFactorSquared;
	return !bDrawShadowDepth;
}

bool CullScreenSpaceSize( const idBounds & PrimitiveBounds )
{
	// Distance culling for RSMs
	const float MinScreenRadiusForShadowCaster = false/*bReflectiveShadowmap*/ ? minScreenRadiusForShadowCasterRSM : minScreenRadiusForShadowCaster;

	/** Square of the FOV based multiplier for cull distance on objects */
	float LODDistanceFactorSquared ; // ProjectedShadowInfo->DependentView->LODDistanceFactorSquared;

	//const float DistanceSquared = ( PrimitiveBounds.GetCenter() - ProjectedShadowInfo->DependentView->ShadowViewMatrices.GetViewOrigin() ).SizeSquared();
	const float DistanceSquared = ( PrimitiveBounds.GetCenter() - backEnd.viewDef->GetOrigin() ).LengthSqr();
	const bool bScreenSpaceSizeCulled = idMath::Square( PrimitiveBounds.GetRadius() ) < idMath::Square( MinScreenRadiusForShadowCaster ) * DistanceSquared * LODDistanceFactorSquared;
}

void DrawLightVolume( const viewLight_t * const vLight )
{
	idBounds globalLightBounds;
	const bool bCameraInsideLightGeometry = backEnd.viewDef->ViewInsideLightVolume( globalLightBounds );
	idRenderMatrix;

	// Render backfaces with depth tests disabled since the camera is inside (or close to inside) the light geometry
	if( bCameraInsideLightGeometry )
	{
		//GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
		//GraphicsPSOInit.RasterizerState = View.bReverseCulling ? TStaticRasterizerState<FM_Solid, CM_CW>::GetRHI() : TStaticRasterizerState<FM_Solid, CM_CCW>::GetRHI();

		GL_State( 0 );
	}
	else // Render frontfaces with depth tests on to get the speedup from HiZ since the camera is outside the light geometry
	{		
		//GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_DepthNearOrEqual>::GetRHI();
		//GraphicsPSOInit.RasterizerState = View.bReverseCulling ? TStaticRasterizerState<FM_Solid, CM_CCW>::GetRHI() : TStaticRasterizerState<FM_Solid, CM_CW>::GetRHI();
		
		GL_State( 0 );
	}
}

struct shadowBufferParms_t
{
	idVec2i		viewportOffset;

	int			lightBufferSize				= 1024;
	int			maxLightBufferSize			= 1024;
	float		lightBufferSizeFraction		= 0.5;

	int			viewBufferSize				= 1024;
	int			viewBufferHeight			= 768;
	int			maxViewBufferSize			= 1024;
	float		viewBufferSizeFraction		= 0.5;
	float		viewBufferHeightFraction	= 0.5;
	bool		nativeViewBuffer			= false;		// true if viewBufferSize is the viewport width

	idImage *	shadowImage[ 3 ];

	idImage *	viewDepthImage;
	idImage *	viewAlphaImage;

	idImage *	viewShadowImage;

	idImage *	jitterImage16;
	idImage *	jitterImage4;
	idImage *	jitterImage1;

	idImage *	random256Image;

	int			shadowVertexProgram;
	int			shadowFragmentProgram16;
	int			shadowFragmentProgram4;
	int			shadowFragmentProgram1;
	int			shadowFragmentProgram0;

	int			screenSpaceShadowVertexProgram;
	int			screenSpaceShadowFragmentProgram16;
	int			screenSpaceShadowFragmentProgram4;
	int			screenSpaceShadowFragmentProgram1;
	int			screenSpaceShadowFragmentProgram0;

	int			depthMidpointVertexProgram;
	int			depthMidpointFragmentProgram;

	int			shadowResampleVertexProgram;
	int			shadowResampleFragmentProgram;

	int			gammaDitherVertexProgram;
	int			gammaDitherFragmentProgram;

	int			downSampleVertexProgram;
	int			downSampleFragmentProgram;

	int			bloomVertexProgram;
	int			bloomFragmentProgram;

	float		viewLightAxialSize;

	const idRenderDestination *	floatPbuffer;
	idImage	*	floatPbufferImage;

	const idRenderDestination *	floatPbuffer2;
	idImage	*	floatPbuffer2Image;

	const idRenderDestination *	floatPbufferQuarter;
	idImage	*	floatPbufferQuarterImage;

	const idRenderDestination *	shadowPbuffer;
	const idRenderDestination *	viewPbuffer;
};

const static int JITTER_SIZE = 128;

void ValidateShadowMapSettings( shadowBufferParms_t & parms )
{
	// validate the samples
	if( r_sb_samples.GetInteger() != 16 && r_sb_samples.GetInteger() != 4 && r_sb_samples.GetInteger() != 1 )
	{
		r_sb_samples.SetInteger( 0 );
	}

	// validate the light resolution
	if( r_sb_lightResolution.GetInteger() < 64 )
	{
		r_sb_lightResolution.SetInteger( 64 );
	}
	else if( r_sb_lightResolution.GetInteger() > parms.maxLightBufferSize )
	{
		r_sb_lightResolution.SetInteger( parms.maxLightBufferSize );
	}
	parms.lightBufferSize = r_sb_lightResolution.GetInteger();
	parms.lightBufferSizeFraction = ( float )parms.lightBufferSize / parms.maxLightBufferSize;

	// validate the view resolution
	if( r_sb_viewResolution.GetInteger() < 64 )
	{
		r_sb_viewResolution.SetInteger( 64 );
	}
	else if( r_sb_viewResolution.GetInteger() > parms.maxViewBufferSize )
	{
		r_sb_viewResolution.SetInteger( parms.maxViewBufferSize );
	}
	parms.viewBufferSize = r_sb_viewResolution.GetInteger();
	parms.viewBufferHeight = parms.viewBufferSize * 3 / 4;
	parms.viewBufferSizeFraction = ( float )parms.viewBufferSize / parms.maxViewBufferSize;
	parms.viewBufferHeightFraction = ( float )parms.viewBufferHeight / parms.maxViewBufferSize;
	parms.nativeViewBuffer = bool( parms.viewBufferSize == backEnd.viewDef->GetViewport().GetWidth() );

	// set up for either point sampled or percentage-closer filtering for the shadow sampling
	parms.shadowImage[ 0 ]->Bind();
	if( r_sb_linearFilter.GetBool() )
	{
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}
	else {
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE );
	renderImageManager->BindNull();

	// the jitter image will be used to offset sample centers
	if( r_sb_samples.GetInteger() == 16 )
	{
		GL_BindTexture( 8, parms.jitterImage16 );
	}
	else if( r_sb_samples.GetInteger() == 4 )
	{
		GL_BindTexture( 8, parms.jitterImage4 );
	}
	else {
		GL_BindTexture( 8, parms.jitterImage1 );
	}
}

/*
==================
RB_EXP_CullInteractions

Sets surfaceInteraction_t->cullBits
==================
*/
void RB_EXP_CullInteractions( const viewLight_t * const vLight, idPlane frustumPlanes[ 6 ] )
{
	for( auto inter = vLight->lightDef->firstInteraction; inter; inter = inter->lightNext )
	{
		const auto * const entityDef = inter->entityDef;
		if( !entityDef )
		{
			continue;
		}
		if( inter->numSurfaces < 1 )
		{
			continue;
		}

		int	culled = 0;

		if( r_sb_useCulling.GetBool() )
		{
			// transform light frustum into object space, positive side points outside the light
			idRenderPlane localPlanes[ 6 ];
			int	plane;
			for( plane = 0; plane < 6; plane++ )
			{
				//R_GlobalPlaneToLocal( entityDef->modelMatrix, frustumPlanes[ plane ], localPlanes[ plane ] );
				entityDef->modelRenderMatrix.InverseTransformPlane( frustumPlanes[ plane ], localPlanes[ plane ] );
			}

			// cull the entire entity bounding box
			// has referenceBounds been tightened to the actual model bounds?
			const idBounds & referenceBounds = entityDef->globalReferenceBounds; //entityDef->localReferenceBounds;

			idVec3 corners[ 8 ];
			referenceBounds.ToPoints( corners );

			for( plane = 0; plane < 6; plane++ )
			{
				int	j;
				for( j = 0; j < 8; j++ )
				{
					// if a corner is on the negative side (inside) of the frustum, the surface is not culled
					// by this plane
					if( corners[ j ] * localPlanes[ plane ].ToVec4().ToVec3() + localPlanes[ plane ][ 3 ] < 0 )
					{
						break;
					}
				}
				if( j == 8 )
				{
					break;			// all points outside the light
				}
			}
			if( plane < 6 )
			{
				culled = CULL_OCCLUDER_AND_RECEIVER;
			}
		}

		for( int i = 0; i < inter->numSurfaces; i++ )
		{
			auto & surfInt = inter->surfaces[ i ];

			if( !surfInt.ambientTris )
			{
				continue;
			}
			surfInt.expCulled = culled;
		}

	}
}
/*
==================
RB_EXP_RenderOccluders
==================
*/
void RB_EXP_RenderOccluders( const viewLight_t * const vLight )
{
	for( auto inter = vLight->lightDef->firstInteraction; inter; inter = inter->lightNext )
	{
		const auto * const entityDef = inter->entityDef;
		if( !entityDef )
		{
			continue;
		}
		if( inter->numSurfaces < 1 )
		{
			continue;
		}

		// no need to check for current on this, because each interaction is always
		// a different space
		//float matrix[ 16 ];
		//myGlMultMatrix( inter->entityDef->modelMatrix, lightMatrix, matrix );
		//qglLoadMatrixf( matrix );

		// draw each surface
		for( int i = 0; i < inter->numSurfaces; i++ )
		{
			const auto & surfInt = inter->surfaces[ i ];

			if( !surfInt.ambientTris )
			{
				continue;
			}
			if( surfInt.shader && !surfInt.shader->SurfaceCastsShadow() )
			{
				continue;
			}

			// cull it
			if( surfInt.expCulled == CULL_OCCLUDER_AND_RECEIVER )
			{
				continue;
			}

			// render it
			const idTriangles *tri = surfInt.ambientTris;
			if( !tri->vertexCache )
			{
				R_CreateAmbientCache( const_cast<idTriangles *>( tri ), false );
			}
			idDrawVert *ac = ( idDrawVert * )vertexCache.Position( tri->ambientCache );
			qglVertexPointer( 3, GL_FLOAT, sizeof( idDrawVert ), ac->xyz.ToFloatPtr() );
			qglTexCoordPointer( 2, GL_FLOAT, sizeof( idDrawVert ), ac->st.ToFloatPtr() );
			
			if( surfInt.shader ) {
				surfInt.shader->GetEditorImage()->Bind();
			}

			RB_DrawElementsWithCounters( tri );
		}
	}
}

idPlane	pointLightFrustums[ 6 ][ 6 ] = {
	{ idPlane( 1,0,0,0 ), idPlane( 1,1,0,0 ), idPlane( 1,-1,0,0 ), idPlane( 1,0,1,0 ), idPlane( 1,0,-1,0 ), idPlane( -1,0,0,0 ), },
	{ idPlane( -1,0,0,0 ), idPlane( -1,1,0,0 ), idPlane( -1,-1,0,0 ), idPlane( -1,0,1,0 ), idPlane( -1,0,-1,0 ), idPlane( 1,0,0,0 ), },

	{ idPlane( 0,1,0,0 ), idPlane( 0,1,1,0 ), idPlane( 0,1,-1,0 ), idPlane( 1,1,0,0 ), idPlane( -1,1,0,0 ),idPlane( 0,-1,0,0 ), },
	{ idPlane( 0,-1,0,0 ), idPlane( 0,-1,1,0 ), idPlane( 0,-1,-1,0 ), idPlane( 1,-1,0,0 ), idPlane( -1,-1,0,0 ),idPlane( 0,1,0,0 ), },

	{ idPlane( 0,0,1,0 ), idPlane( 1,0,1,0 ), idPlane( -1,0,1,0 ), idPlane( 0,1,1,0 ), idPlane( 0,-1,1,0 ), idPlane( 0,0,-1,0 ), },
	{ idPlane( 0,0,-1,0 ), idPlane( 1,0,-1,0 ), idPlane( -1,0,-1,0 ), idPlane( 0,1,-1,0 ), idPlane( 0,-1,-1,0 ), idPlane( 0,0,1,0 ), },
};

struct shadowFrustum_t  
{
	int		numPlanes;		// this is always 6 for now
	idPlane	planes[ 6 ];
	// positive sides facing inward
	// plane 5 is always the plane the projection is going to, the
	// other planes are just clip planes
	// all planes are in global coordinates

	bool	makeClippedPlanes;
	// a projected light with a single frustum needs to make sil planes
	// from triangles that clip against side planes, but a point light
	// that has adjacent frustums doesn't need to
};

/*
===================
R_MakeShadowFrustums

Called at definition derivation time
===================
*/
void R_MakeShadowFrustums( idRenderLightLocal *light, shadowFrustum_t shadowFrustums[ 6 ], int & numShadowFrustums )
{
	int	i, j;

	if( light->GetParms().pointLight )
	{
		// exact projection,taking into account asymetric frustums when
		// globalLightOrigin isn't centered

		static const int faceCorners[ 6 ][ 4 ] = {
			{ 7, 5, 1, 3 },		// positive X side
			{ 4, 6, 2, 0 },		// negative X side
			{ 6, 7, 3, 2 },		// positive Y side
			{ 5, 4, 0, 1 },		// negative Y side
			{ 6, 4, 5, 7 },		// positive Z side
			{ 3, 1, 0, 2 }		// negative Z side
		};
		static const int faceEdgeAdjacent[ 6 ][ 4 ] = {
			{ 4, 4, 2, 2 },		// positive X side
			{ 7, 7, 1, 1 },		// negative X side
			{ 5, 5, 0, 0 },		// positive Y side
			{ 6, 6, 3, 3 },		// negative Y side
			{ 0, 0, 3, 3 },		// positive Z side
			{ 5, 5, 6, 6 }		// negative Z side
		};

		bool centerOutside = false;

		// if the light center of projection is outside the light bounds,
		// we will need to build the planes a little differently
		if( idMath::Fabs( light->GetParms().lightCenter[ 0 ] ) > light->GetParms().lightRadius[ 0 ] || 
			idMath::Fabs( light->GetParms().lightCenter[ 1 ] ) > light->GetParms().lightRadius[ 1 ] || 
			idMath::Fabs( light->GetParms().lightCenter[ 2 ] ) > light->GetParms().lightRadius[ 2 ] )
		{
			centerOutside = true;
		}

		// make the corners
		idVec3 corners[ 8 ];

		for( i = 0; i < 8; i++ )
		{
			idVec3	temp;
			for( j = 0; j < 3; j++ )
			{
				if( i & ( 1 << j ) )
				{
					temp[ j ] = light->GetParms().lightRadius[ j ];
				}
				else {
					temp[ j ] = -light->GetParms().lightRadius[ j ];
				}
			}

			// transform to global space
			corners[ i ] = light->GetOrigin() + light->GetAxis() * temp;
		}

		numShadowFrustums = 0;
		for( int side = 0; side < 6; side++ )
		{
			shadowFrustum_t	*frust = &shadowFrustums[ numShadowFrustums ];
			idVec3 &p1 = corners[ faceCorners[ side ][ 0 ] ];
			idVec3 &p2 = corners[ faceCorners[ side ][ 1 ] ];
			idVec3 &p3 = corners[ faceCorners[ side ][ 2 ] ];
			idPlane backPlane;

			// plane will have positive side inward
			backPlane.FromPoints( p1, p2, p3 );

			// if center of projection is on the wrong side, skip
			float d = backPlane.Distance( light->globalLightOrigin );
			if( d < 0 )
			{
				continue;
			}

			frust->numPlanes = 6;
			frust->planes[ 5 ] = backPlane;
			frust->planes[ 4 ] = backPlane;	// we don't really need the extra plane

											// make planes with positive side facing inwards in light local coordinates
			for( int edge = 0; edge < 4; edge++ )
			{
				idVec3 &p1 = corners[ faceCorners[ side ][ edge ] ];
				idVec3 &p2 = corners[ faceCorners[ side ][ ( edge + 1 ) & 3 ] ];

				// create a plane that goes through the center of projection
				frust->planes[ edge ].FromPoints( p2, p1, light->globalLightOrigin );

				// see if we should use an adjacent plane instead
				if( centerOutside )
				{
					idVec3 &p3 = corners[ faceEdgeAdjacent[ side ][ edge ] ];
					idPlane sidePlane;

					sidePlane.FromPoints( p2, p1, p3 );
					d = sidePlane.Distance( light->globalLightOrigin );
					if( d < 0 )
					{
						// use this plane instead of the edged plane
						frust->planes[ edge ] = sidePlane;
					}
					// we can't guarantee a neighbor, so add sill planes at edge
					shadowFrustums[ numShadowFrustums ].makeClippedPlanes = true;
				}
			}
			numShadowFrustums++;
		}

		return;
	}

	// projected light

	numShadowFrustums = 1;
	shadowFrustum_t	*frust = &shadowFrustums[ 0 ];

	// flip and transform the frustum planes so the positive side faces
	// inward in local coordinates

	// it is important to clip against even the near clip plane, because
	// many projected lights that are faking area lights will have their
	// origin behind solid surfaces.
	for( i = 0; i < 6; i++ )
	{
		idPlane &plane = frust->planes[ i ];

		plane.SetNormal( -light->frustum[ i ].Normal() );
		plane.SetDist( -light->frustum[ i ].Dist() );
	}

	frust->numPlanes = 6;

	frust->makeClippedPlanes = true;
	// projected lights don't have shared frustums, so any clipped edges
	// right on the planes must have a sil plane created for them
}

/*
==================
RB_Exp_TrianglesForFrustum
==================
*/
const idTriangles *	RB_Exp_TrianglesForFrustum( shadowBufferParms_t & parms, const viewLight_t * const vLight, int side )
{
	static idTriangles	frustumTri;
	static idDrawVert	verts[ 5 ];
	static triIndex_t	indexes[ 18 ] = { 0, 1, 2,	0, 2, 3,  0, 3, 4,	0, 4, 1,  2, 1, 4,	2, 4, 3 };

	const idTriangles *tri;

	if( side == -1 )
	{
		tri = vLight->frustumTris;
	}
	else {
		memset( verts, 0, sizeof( verts ) );

		for( int i = 0; i < 5; i++ )
		{
			verts[ i ].SetPosition( vLight->globalLightOrigin );
		}

		frustumTri.Clear();
		frustumTri.indexes = indexes;
		frustumTri.verts = verts;
		frustumTri.numIndexes = 18;
		frustumTri.numVerts = 5;

		tri = &frustumTri;

		const float size = parms.viewLightAxialSize;

		switch( side )
		{
			case 0:
				verts[ 1 ].xyz[ 0 ] += size;
				verts[ 2 ].xyz[ 0 ] += size;
				verts[ 3 ].xyz[ 0 ] += size;
				verts[ 4 ].xyz[ 0 ] += size;
				verts[ 1 ].xyz[ 1 ] += size;
				verts[ 1 ].xyz[ 2 ] += size;
				verts[ 2 ].xyz[ 1 ] -= size;
				verts[ 2 ].xyz[ 2 ] += size;
				verts[ 3 ].xyz[ 1 ] -= size;
				verts[ 3 ].xyz[ 2 ] -= size;
				verts[ 4 ].xyz[ 1 ] += size;
				verts[ 4 ].xyz[ 2 ] -= size;
				break;
			case 1:
				verts[ 1 ].xyz[ 0 ] -= size;
				verts[ 2 ].xyz[ 0 ] -= size;
				verts[ 3 ].xyz[ 0 ] -= size;
				verts[ 4 ].xyz[ 0 ] -= size;
				verts[ 1 ].xyz[ 1 ] -= size;
				verts[ 1 ].xyz[ 2 ] += size;
				verts[ 2 ].xyz[ 1 ] += size;
				verts[ 2 ].xyz[ 2 ] += size;
				verts[ 3 ].xyz[ 1 ] += size;
				verts[ 3 ].xyz[ 2 ] -= size;
				verts[ 4 ].xyz[ 1 ] -= size;
				verts[ 4 ].xyz[ 2 ] -= size;
				break;
			case 2:
				verts[ 1 ].xyz[ 1 ] += size;
				verts[ 2 ].xyz[ 1 ] += size;
				verts[ 3 ].xyz[ 1 ] += size;
				verts[ 4 ].xyz[ 1 ] += size;
				verts[ 1 ].xyz[ 0 ] -= size;
				verts[ 1 ].xyz[ 2 ] += size;
				verts[ 2 ].xyz[ 0 ] += size;
				verts[ 2 ].xyz[ 2 ] += size;
				verts[ 3 ].xyz[ 0 ] += size;
				verts[ 3 ].xyz[ 2 ] -= size;
				verts[ 4 ].xyz[ 0 ] -= size;
				verts[ 4 ].xyz[ 2 ] -= size;
				break;
			case 3:
				verts[ 1 ].xyz[ 1 ] -= size;
				verts[ 2 ].xyz[ 1 ] -= size;
				verts[ 3 ].xyz[ 1 ] -= size;
				verts[ 4 ].xyz[ 1 ] -= size;
				verts[ 1 ].xyz[ 0 ] += size;
				verts[ 1 ].xyz[ 2 ] += size;
				verts[ 2 ].xyz[ 0 ] -= size;
				verts[ 2 ].xyz[ 2 ] += size;
				verts[ 3 ].xyz[ 0 ] -= size;
				verts[ 3 ].xyz[ 2 ] -= size;
				verts[ 4 ].xyz[ 0 ] += size;
				verts[ 4 ].xyz[ 2 ] -= size;
				break;
			case 4:
				verts[ 1 ].xyz[ 2 ] += size;
				verts[ 2 ].xyz[ 2 ] += size;
				verts[ 3 ].xyz[ 2 ] += size;
				verts[ 4 ].xyz[ 2 ] += size;
				verts[ 1 ].xyz[ 0 ] += size;
				verts[ 1 ].xyz[ 1 ] += size;
				verts[ 2 ].xyz[ 0 ] -= size;
				verts[ 2 ].xyz[ 1 ] += size;
				verts[ 3 ].xyz[ 0 ] -= size;
				verts[ 3 ].xyz[ 1 ] -= size;
				verts[ 4 ].xyz[ 0 ] += size;
				verts[ 4 ].xyz[ 1 ] -= size;
				break;
			case 5:
				verts[ 1 ].xyz[ 2 ] -= size;
				verts[ 2 ].xyz[ 2 ] -= size;
				verts[ 3 ].xyz[ 2 ] -= size;
				verts[ 4 ].xyz[ 2 ] -= size;
				verts[ 1 ].xyz[ 0 ] -= size;
				verts[ 1 ].xyz[ 1 ] += size;
				verts[ 2 ].xyz[ 0 ] += size;
				verts[ 2 ].xyz[ 1 ] += size;
				verts[ 3 ].xyz[ 0 ] += size;
				verts[ 3 ].xyz[ 1 ] -= size;
				verts[ 4 ].xyz[ 0 ] -= size;
				verts[ 4 ].xyz[ 1 ] -= size;
				break;
		}

		frustumTri.vertexCache = vertexCache.AllocVertex( verts, sizeof( verts ) );
	}

	return tri;
}

/*
==================
RB_Exp_SelectFrustum
==================
*/
void RB_Exp_SelectFrustum( shadowBufferParms_t & parms, const viewLight_t * const vLight, int side )
{
	renderProgManager.SetRenderParms( RENDERPARM_MODELVIEWMATRIX_X, backEnd.viewDef->GetViewMatrix().Ptr(), 4 );

	const idTriangles *tri = RB_Exp_TrianglesForFrustum( parms, vLight, side );

	idDrawVert *ac = ( idDrawVert * )vertexCache.Position( tri->vertexCache );
	//glVertexAttribPointer( 3, GL_FLOAT, sizeof( idDrawVert ), ac->GetPosition().ToFloatPtr() );
	glVertexAttribPointer( PC_ATTRIB_INDEX_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof( idDrawVert ), idDrawVert::positionOffset );

	///glDisable( GL_TEXTURE_2D );
	///glDisableClientState( GL_TEXTURE_COORD_ARRAY );

	// clear stencil buffer
	glEnable( GL_SCISSOR_TEST );
	glEnable( GL_STENCIL_TEST );
	const GLint clearStencilValue[ 1 ] = { 1 };
	glClearBufferiv( GL_STENCIL, 0, clearStencilValue );

	// draw front faces of the light frustum, incrementing the stencil buffer on depth fail
	// so we can't draw on those pixels
	GL_State( GLS_COLORMASK | GLS_ALPHAMASK | GLS_DEPTHMASK | GLS_DEPTHFUNC_LESS );
	glStencilFunc( GL_ALWAYS, 0, 255 );
	glStencilOp( GL_KEEP, GL_INCR, GL_KEEP );
	GL_Cull( CT_FRONT_SIDED );

	GL_DrawIndexed( tri );

	// draw back faces of the light frustum with 
	// depth test greater
	// stencil test of equal 1
	// zero stencil stencil when depth test passes, so subsequent surface drawing
	// can occur on those pixels

	// this pass does all the shadow filtering
	glStencilFunc( GL_EQUAL, 1, 255 );
	glStencilOp( GL_KEEP, GL_KEEP, GL_ZERO );

	GL_Cull( CT_BACK_SIDED );
	glDepthFunc( GL_GREATER );

	// write to destination alpha
	if( r_sb_showFrustumPixels.GetBool() )
	{
		GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_LESS );
		///glDisable( GL_TEXTURE_2D );
		idRenderVector color( 0.0f, 0.25f, 0.0f, 1.0f );
		renderProgManager.SetRenderParm( RENDERPARM_COLOR, color.ToFloatPtr() );
	}
	else {
		GL_State( GLS_COLORMASK | GLS_DEPTHMASK | GLS_DEPTHFUNC_LESS );

		glBindProgramARB( GL_VERTEX_PROGRAM_ARB, parms.screenSpaceShadowVertexProgram );
		switch( r_sb_samples.GetInteger() )
		{
		case 0:
			glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, parms.screenSpaceShadowFragmentProgram0 );
			break;
		case 1:
			glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, parms.screenSpaceShadowFragmentProgram1 );
			break;
		case 4:
			glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, parms.screenSpaceShadowFragmentProgram4 );
			break;
		case 16:
			glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, parms.screenSpaceShadowFragmentProgram16 );
			break;
		}
	}

	/*
	texture[0] = view depth texture
	texture[1] = jitter texture
	texture[2] = light depth texture
	*/

	GL_BindTexture( 2, parms.shadowImage[ 0 ] );

	if( r_sb_samples.GetInteger() == 16 )
	{
		GL_BindTexture( 1, parms.jitterImage16 );
	}
	else if( r_sb_samples.GetInteger() == 4 )
	{
		GL_BindTexture( 1, parms.jitterImage4 );
	}
	else {
		GL_BindTexture( 1, parms.jitterImage1 );
	}

	GL_BindTexture( 0, parms.viewDepthImage );

	/*
	PARAM	positionToDepthTexScale		= program.local[0];	# fragment.position to screen depth texture transformation
	PARAM	zProject					= program.local[1];	# projection[10], projection[14], 0, 0
	PARAM	positionToViewSpace			= program.local[2];	# X add, Y add, X mul, Y mul
	PARAM	viewToLightS				= program.local[3];
	PARAM	viewToLightT				= program.local[4];
	PARAM	viewToLightR				= program.local[5];
	PARAM	viewToLightQ				= program.local[6];
	PARAM	positionToJitterTexScale	= program.local[7];	# fragment.position to jitter texture
	PARAM	jitterTexScale				= program.local[8];
	PARAM	jitterTexOffset				= program.local[9];
	*/
	idRenderVector	parm;
	int		pot;

	// calculate depth projection for shadow buffer
	idRenderVector	sRow, tRow, rRow, qRow;
	float	invertedView[ 16 ];
	///float	invertedProjection[ 16 ];
	float	matrix[ 16 ];
	float	matrix2[ 16 ];

	// we need the inverse of the projection matrix to go from NDC to view
	///FullInvert( backEnd.viewDef->projectionMatrix, invertedProjection );

	/*
	from window to NDC:
	( x - xMid ) * 1.0 / xMid
	( y - yMid ) * 1.0 / yMid
	( z - 0.5 ) * 2

	from NDC to clip coordinates:
	rcp(1/w)

	*/

	// we need the inverse of the viewMatrix to go from view (looking down negative Z) to world
	//InvertByTranspose( backEnd.viewDef->worldSpace.modelViewMatrix, invertedView );

	// then we go from world to light view space (looking down negative Z)
	//myGlMultMatrix( invertedView, lightMatrix, matrix );

	// then to light projection, giving X/w, Y/w, Z/w in the -1 : 1 range
	//myGlMultMatrix( matrix, lightProjectionMatrix, matrix2 );

	// the final values need to be in 0.0 : 1.0 range instead of -1 : 1
	sRow[ 0 ] = 0.5 * ( matrix2[ 0 ] + matrix2[ 3 ] ) * parms.lightBufferSizeFraction;
	sRow[ 1 ] = 0.5 * ( matrix2[ 4 ] + matrix2[ 7 ] ) * parms.lightBufferSizeFraction;
	sRow[ 2 ] = 0.5 * ( matrix2[ 8 ] + matrix2[ 11 ] ) * parms.lightBufferSizeFraction;
	sRow[ 3 ] = 0.5 * ( matrix2[ 12 ] + matrix2[ 15 ] ) * parms.lightBufferSizeFraction;

	tRow[ 0 ] = 0.5 * ( matrix2[ 1 ] + matrix2[ 3 ] ) * parms.lightBufferSizeFraction;
	tRow[ 1 ] = 0.5 * ( matrix2[ 5 ] + matrix2[ 7 ] ) * parms.lightBufferSizeFraction;
	tRow[ 2 ] = 0.5 * ( matrix2[ 9 ] + matrix2[ 11 ] ) * parms.lightBufferSizeFraction;
	tRow[ 3 ] = 0.5 * ( matrix2[ 13 ] + matrix2[ 15 ] ) * parms.lightBufferSizeFraction;
	
	rRow[ 0 ] = 0.5 * ( matrix2[ 2 ] + matrix2[ 3 ] );
	rRow[ 1 ] = 0.5 * ( matrix2[ 6 ] + matrix2[ 7 ] );
	rRow[ 2 ] = 0.5 * ( matrix2[ 10 ] + matrix2[ 11 ] );
	rRow[ 3 ] = 0.5 * ( matrix2[ 14 ] + matrix2[ 15 ] );
	
	qRow[ 0 ] = matrix2[ 3 ];
	qRow[ 1 ] = matrix2[ 7 ];
	qRow[ 2 ] = matrix2[ 11 ];
	qRow[ 3 ] = matrix2[ 15 ];

	renderProgManager.SetRenderParm( RENDERPARM_SHADOW_MATRIX_0_X, sRow.ToFloatPtr() );
	renderProgManager.SetRenderParm( RENDERPARM_SHADOW_MATRIX_0_Y, tRow.ToFloatPtr() );
	renderProgManager.SetRenderParm( RENDERPARM_SHADOW_MATRIX_0_Z, rRow.ToFloatPtr() );
	renderProgManager.SetRenderParm( RENDERPARM_SHADOW_MATRIX_0_W, qRow.ToFloatPtr() );

	//-----------------------------------------------------
	// these should be constant for the entire frame

	// convert 0..viewport-1 sizes to fractions inside the POT screen depth texture
	int	w = parms.viewBufferSize;
	pot = MakePowerOfTwo( w );
	parm[ 0 ] = 1.0 / parms.maxViewBufferSize;	//  * ( (float)viewBufferSize / w );
	int	 h = parms.viewBufferHeight;
	pot = MakePowerOfTwo( h );
	parm[ 1 ] = parm[ 0 ]; // 1.0 / pot;
	parm[ 2 ] = 0;
	parm[ 3 ] = 1;
	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_S, parm.ToFloatPtr() );

	// zProject values
	parm[ 0 ] = backEnd.viewDef->GetProjectionMatrix()[2][2];
	parm[ 1 ] = backEnd.viewDef->GetProjectionMatrix()[2][3];
	parm[ 2 ] = 0.0f;
	parm[ 3 ] = 0.0f;
	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_T, parm.ToFloatPtr() );

	// positionToViewSpace
	parm[ 0 ] = -1.0 / backEnd.viewDef->GetProjectionMatrix()[0][0];
	parm[ 1 ] = -1.0 / backEnd.viewDef->GetProjectionMatrix()[1][1];
	parm[ 2 ] = 2.0 / parms.viewBufferSize;
	parm[ 3 ] = 2.0 / parms.viewBufferSize;
	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_S, parm.ToFloatPtr() );

	// positionToJitterTexScale
	parm[ 0 ] = 1.0 / ( JITTER_SIZE * r_sb_samples.GetInteger() );
	parm[ 1 ] = 1.0 / JITTER_SIZE;
	parm[ 2 ] = 0;
	parm[ 3 ] = 1;
	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_1_S, parm.ToFloatPtr() );

	// jitter tex scale
	parm[ 0 ] =
	parm[ 1 ] = r_sb_jitterScale.GetFloat() * parms.lightBufferSizeFraction;
	parm[ 2 ] = -r_sb_biasScale.GetFloat();
	parm[ 3 ] = 0;
	renderProgManager.SetRenderParm( RENDERPARM_JITTERTEXSCALE, parm.ToFloatPtr() );

	// jitter tex offset
	if( r_sb_randomize.GetBool() )
	{
		parm[ 0 ] = ( rand() & 255 ) / 255.0;
		parm[ 1 ] = ( rand() & 255 ) / 255.0;
	}
	else {
		parm[ 0 ] = parm[ 1 ] = 0;
	}
	parm[ 2 ] = 0;
	parm[ 3 ] = 0;
	renderProgManager.SetRenderParm( RENDERPARM_JITTERTEXOFFSET, parm.ToFloatPtr() );
	//-----------------------------------------------------



	GL_DrawIndexed( tri );

	renderProgManager.Unbind();

	GL_Cull( CT_FRONT_SIDED );
	//	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

	glDepthFunc( GL_LEQUAL );
	if( r_sb_showFrustumPixels.GetBool() )
	{
		///glEnable( GL_TEXTURE_2D );
		idRenderVector color( 1.0f, 1.0f, 1.0f, 1.0f );
		renderProgManager.SetRenderParm( RENDERPARM_COLOR, color.ToFloatPtr() );
	}

	// after all the frustums have been drawn, the surfaces that have been drawn on will get interactions
	// scissor may still be a win even with the stencil test for very fast rejects
	glStencilFunc( GL_EQUAL, 0, 255 );
	glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );

	// we can avoid clearing the stencil buffer by changing the hasLight value for each light
}


/*
==================
RB_ShadowResampleAlpha
==================
*/
void RB_ShadowResampleAlpha( shadowBufferParms_t & parms, const viewLight_t * const vLight )
{
	parms.viewAlphaImage->Bind();
	// we could make this a subimage, but it isn't relevent once we have render-to-texture
	glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, parms.viewBufferSize, parms.viewBufferHeight );

	RB_EXP_SetRenderBuffer( parms, vLight, parms.viewportOffset );

	//=====================

	renderProgManager.SetRenderParms( RENDERPARM_MODELVIEWMATRIX_X, backEnd.viewDef->GetViewMatrix().Ptr(), 4 );

	// this uses the full light, not side frustums
	const idTriangles *tri = vLight->frustumTris;

	idDrawVert *ac = ( idDrawVert * )vertexCache.Position( tri->vertexCache );
	glVertexPointer( 3, GL_FLOAT, sizeof( idDrawVert ), ac->GetPosition().ToFloatPtr() );

	// clear stencil buffer
	glEnable( GL_SCISSOR_TEST );
	glEnable( GL_STENCIL_TEST );
	glClearStencil( 1 );
	glClear( GL_STENCIL_BUFFER_BIT );

	// draw front faces of the light frustum, incrementing the stencil buffer on depth fail
	// so we can't draw on those pixels
	GL_State( GLS_COLORMASK | GLS_ALPHAMASK | GLS_DEPTHMASK | GLS_DEPTHFUNC_LESS );
	glStencilFunc( GL_ALWAYS, 0, 255 );
	glStencilOp( GL_KEEP, GL_INCR, GL_KEEP );
	GL_Cull( CT_FRONT_SIDED );

	// set fragment / vertex program?

	GL_DrawIndexed( tri );

	// draw back faces of the light frustum with 
	// depth test greater
	// stencil test of equal 1
	// zero stencil stencil when depth test passes, so subsequent interaction drawing
	// can occur on those pixels

	// this pass does all the shadow filtering
	glStencilFunc( GL_EQUAL, 1, 255 );
	glStencilOp( GL_KEEP, GL_KEEP, GL_ZERO );

	// write to destination alpha
	if( r_sb_showFrustumPixels.GetBool() )
	{
		GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | GLS_DEPTHFUNC_LESS );
		glDisable( GL_TEXTURE_2D );
		idRenderVector color( 0, 0.25, 0, 1 );
		renderProgManager.SetRenderParm( RENDERPARM_COLOR, color.ToFloatPtr() );
	}
	else {
		GL_State( GLS_COLORMASK | GLS_DEPTHMASK | GLS_DEPTHFUNC_LESS );
		glEnable( GL_VERTEX_PROGRAM_ARB );
		glEnable( GL_FRAGMENT_PROGRAM_ARB );
		glBindProgramARB( GL_VERTEX_PROGRAM_ARB, parms.shadowResampleVertexProgram );
		glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, parms.shadowResampleFragmentProgram );

		// convert 0..viewport-1 sizes to fractions inside the POT screen depth texture
		// shrink by one unit for bilerp
		idRenderVector parm;
		parm[ 0 ] = 1.0f / ( parms.maxViewBufferSize + 1 ) * parms.viewBufferSize / parms.maxViewBufferSize;
		parm[ 1 ] = parm[ 0 ];
		parm[ 2 ] = 0.0f;
		parm[ 3 ] = 1.0f;
		renderProgManager.SetRenderParm( RENDERPARM_SCREENCORRECTIONFACTOR, parm.ToFloatPtr() ); // frag
	}

	GL_Cull( CT_BACK_SIDED );
	glDepthFunc( GL_GREATER );

	GL_DrawIndexed( tri );

	renderProgManager.Unbind();

	GL_Cull( CT_FRONT_SIDED );

	glDepthFunc( GL_LEQUAL );
	if( r_sb_showFrustumPixels.GetBool() )
	{
		glEnable( GL_TEXTURE_2D );
		idRenderVector color( 1.0f, 1.0f, 1.0f, 1.0f );
		renderProgManager.SetRenderParm( RENDERPARM_COLOR, color.ToFloatPtr() );
	}

	// after all the frustums have been drawn, the surfaces that have been drawn on will get interactions
	// scissor may still be a win even with the stencil test for very fast rejects
	glStencilFunc( GL_EQUAL, 0, 255 );
	glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
}

/*
==================
RB_EXP_SetNativeBuffer

	This is always the back buffer, and scissor is set full screen
==================
*/
void RB_EXP_SetNativeBuffer( float tr_viewportOffset[2] )
{	
	///R_MakeCurrent( win32.hDC, win32.hGLRC, NULL );
	RENDERLOG_PRINT("RB_EXP_SetNativeBuffer()\n");
	
	// set the normal screen drawable current
	glBindFramebuffer( GL_FRAMEBUFFER, GL_NONE );
	backEnd.glState.currentFramebufferObject = GL_NONE;
	backEnd.glState.currentRenderDestination = nullptr;

	glViewport( 
		tr_viewportOffset[ 0 ] + backEnd.viewDef->GetViewport().x1,
		tr_viewportOffset[ 1 ] + backEnd.viewDef->GetViewport().y1,
		backEnd.viewDef->GetViewport().GetWidth(),
		backEnd.viewDef->GetViewport().GetHeight() );

	backEnd.currentScissor = backEnd.viewDef->GetViewport();
	if( r_useScissor.GetBool() )
	{
		glScissor( 
			backEnd.viewDef->GetViewport().x1 + backEnd.currentScissor.x1,
			backEnd.viewDef->GetViewport().y1 + backEnd.currentScissor.y1,
			backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
			backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
	}
}

void GL_SetRenderDestination( GLuint fbo )
{
	glBindFramebuffer( GL_FRAMEBUFFER, fbo );
	backEnd.glState.currentFramebufferObject = fbo;

}

/*
==================
RB_EXP_SetRenderBuffer

	This may be to a float pBuffer, and scissor is set to cover only the light
==================
*/
void RB_EXP_SetRenderBuffer( const shadowBufferParms_t & parms, const viewLight_t * const vLight, const idVec2i & viewportOffset )
{
	if( r_hdr_useFloats.GetBool() )
	{
		//R_MakeCurrent( floatPbufferDC, floatContext, floatPbuffer );
		GL_SetRenderDestination( parms.floatPbuffer );
	}
	else {
		GL_SetNativeRenderDestination();
	}

	glViewport(
		viewportOffset.x + backEnd.viewDef->GetViewport().x1,
		viewportOffset.y + backEnd.viewDef->GetViewport().y1,
		backEnd.viewDef->GetViewport().GetWidth(),
		backEnd.viewDef->GetViewport().GetHeight() );

	backEnd.currentScissor = ( !vLight )? backEnd.viewDef->GetViewport() : vLight->scissorRect;

	if( r_useScissor.GetBool() )
	{
		glScissor( 
			backEnd.viewDef->GetViewport().x1 + backEnd.currentScissor.x1,
			backEnd.viewDef->GetViewport().y1 + backEnd.currentScissor.y1,
			backEnd.currentScissor.GetWidth(),
			backEnd.currentScissor.GetHeight() );
	}
}

/*
==================
RB_EXP_ReadFloatBuffer
==================
*/
void RB_EXP_ReadFloatBuffer()
{
	int	pixels = glConfig.nativeScreenWidth * glConfig.nativeScreenHeight;
	idTempArray<float> buf( pixels * 4 );

	glReadPixels( 0, 0, glConfig.nativeScreenWidth, glConfig.nativeScreenHeight, GL_RGBA, GL_FLOAT, buf.Ptr() );

	float mins[ 4 ] = { 9999 };
	float maxs[ 4 ] = {-9999 };
	for( int i = 0; i < pixels; ++i )
	{
		for( int j = 0; j < 4; j++ )
		{
			float v = buf[ i * 4 + j ];
			
			if( v < mins[ j ] )
			{
				mins[ j ] = v;
			}
			if( v > maxs[ j ] )
			{
				maxs[ j ] = v;
			}
		}
	}

	RB_EXP_SetNativeBuffer();

	glLoadIdentity();
	glMatrixMode( GL_PROJECTION );

	GL_State( GLS_DEPTHFUNC_ALWAYS );

	glColor3f( 1, 1, 1 );

	glPushMatrix();
	glLoadIdentity();
	glDisable( GL_TEXTURE_2D );
	glOrtho( 0, 1, 0, 1, -1, 1 );

	glRasterPos2f( 0.01f, 0.01f );
	glDrawPixels( glConfig.nativeScreenWidth, glConfig.nativeScreenHeight, GL_RGBA, GL_FLOAT, buf.Ptr() );

	glPopMatrix();
	glEnable( GL_TEXTURE_2D );
	glMatrixMode( GL_MODELVIEW );
}

/*
==================
RB_Exp_DrawInteractions
==================
*/
void RB_DrawInteractions( const idRenderView * const view , const viewLight_t * const viewLights, shadowBufferParms_t & parms )
{
	///if( !initialized ) {
	///	R_Exp_Allocate();
	///}

	if( !viewLights ) 
		return;

	ValidateShadowMapSettings( parms );

	// copy the current depth buffer to a texture for image-space shadowing,
	// or re-render at a lower resolution
	//  R_EXP_RenderViewDepthImage();

	GL_SelectTexture( 0 );

	// disable stencil shadow test
	glStencilFunc( GL_ALWAYS, 128, 255 );

	// if we are using a float buffer, clear it now
	if( r_hdr_useFloats.GetBool() )
	{
		RB_EXP_SetRenderBuffer( parms, NULL, parms.viewportOffset );
		
		// we need to set a lot of things, because this is a completely different context
		GL_SetDefaultState();
		GL_Clear( true, true, false, 0, 0.001f, 1.0f, 0.01f, 0.1f, false );
		
		// clear the z buffer, set the projection matrix, etc
		//RB_BeginDrawingView();

		//RB_STD_FillDepthBuffer( ( drawSurf_t ** )&backEnd.viewDef->drawSurfs[ 0 ], backEnd.viewDef->numDrawSurfs );
	}


	//
	// For each light, perform adding and shadowing
	//
	for( auto vLight = viewLights; vLight; vLight = vLight->next )
	{
		// do fogging later
		if( vLight->lightShader->IsFogLight() || vLight->lightShader->IsBlendLight() ) {
			continue;
		}
		if( !vLight->localInteractions && !vLight->globalInteractions && !vLight->translucentInteractions ) {
			continue;
		}

		//if( !vLight->frustumTris->vertexCache )
		//{
		//	R_CreateAmbientCache( const_cast<idTrianglse *>( vLight->frustumTris ), false );
		//}

		// all light side projections must currently match, so non-centered
		// and non-cubic lights must take the largest length
		parms.viewLightAxialSize = vLight->lightDef->GetAxialSize();

		int	side, sideStop;

		if( vLight->lightDef->GetParms().pointLight )
		{
			if( r_sb_singleSide.GetInteger() != -1 )
			{
				side = r_sb_singleSide.GetInteger();
				sideStop = side + 1;
			}
			else {
				side = 0;
				sideStop = 6;
			}
		}
		else {
			side = -1;
			sideStop = 0;
		}

		for(/**/; side < sideStop; side++ )
		{
			// FIXME: check for frustums completely off the screen

			// render a shadow buffer
			RB_RenderShadowBuffer( vLight, side );

			// back to view rendering, possibly in the off-screen buffer
			if( parms.nativeViewBuffer || !r_sb_screenSpaceShadow.GetBool() )
			{
				// directly to screen
				RB_EXP_SetRenderBuffer( parms, vLight, parms.viewportOffset );
			}
			else {
				// to off screen buffer
				if( r_sb_usePbuffer.GetBool() )
				{
					GL_CheckErrors();
					// set the current openGL drawable to the shadow buffer
					//R_MakeCurrent( viewPbufferDC, win32.hGLRC, viewPbuffer );
					GL_SetRenderDestination( parms.viewPbuffer );
				}
				// !@# FIXME: scale light scissor
				GL_ViewportAndScissor( 0, 0, parms.viewBufferSize, parms.viewBufferHeight );
			}

			// render the shadows into destination alpha on the included pixels
			RB_Exp_SelectFrustum( parms, vLight, side );

			if( !r_sb_screenSpaceShadow.GetBool() )
			{
				// bind shadow buffer to texture
				GL_BindTexture( 7, parms.shadowImage[ 0 ] );

				RB_EXP_CreateDrawInteractions( vLight->localInteractions );
				RB_EXP_CreateDrawInteractions( vLight->globalInteractions );
				backEnd.depthFunc = GLS_DEPTHFUNC_LESS;
				RB_EXP_CreateDrawInteractions( vLight->translucentInteractions );
				backEnd.depthFunc = GLS_DEPTHFUNC_EQUAL;
			}
		}

		// render the native window coordinates interactions
		if( r_sb_screenSpaceShadow.GetBool() )
		{
			if( !parms.nativeViewBuffer )
			{
				RB_ShadowResampleAlpha( parms, vLight );
				glEnable( GL_STENCIL_TEST );
			}
			else
			{
				RB_EXP_SetRenderBuffer( parms, vLight, parms.viewportOffset );
				if( /*r_ignore.GetBool()*/ 0 )
				{
					glEnable( GL_STENCIL_TEST );	//!@#
				}
				else {
					glDisable( GL_STENCIL_TEST );	//!@#
				}
			}
			RB_EXP_CreateDrawInteractions( vLight->localInteractions );
			RB_EXP_CreateDrawInteractions( vLight->globalInteractions );
			backEnd.depthFunc = GLS_DEPTHFUNC_LESS;
			RB_EXP_CreateDrawInteractions( vLight->translucentInteractions );
			backEnd.depthFunc = GLS_DEPTHFUNC_EQUAL;
		}
	}

	GL_SelectTexture( 0 );
	GL_ResetTexturesState();
	GL_State( 0 );

	//RB_EXP_Bloom();
	//RB_EXP_GammaDither();

	// these haven't been state saved
	for( int i = 0; i < 8; i++ ) {
		backEnd.glState.tmu[ i ].current2DMap = -1;
	}

	// take it out of texture compare mode so I can testImage it for debugging
	parms.shadowImage[ 0 ]->Bind();
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE );


}

