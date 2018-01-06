
#include "../../tr_local.h"

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
	const drawSurf_t* 	surf;

	idImage* 			bumpImage;
	idImage* 			diffuseImage;
	idImage* 			specularImage;

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

void SetVertexParm( renderParm_t, const float* value );
void SetVertexParms( renderParm_t, const float* value, int num );
void SetFragmentParm( renderParm_t, const float* value );
void RB_SetMVP( const idRenderMatrix& mvp );
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
void RB_SetupInteractionStage( const shaderStage_t*, const float* surfaceRegs, const float lightColor[ 4 ], idVec4 matrix[ 2 ], float color[ 4 ] );

void RB_PrepareStageTexturing( const shaderStage_t*, const drawSurf_t* );
void RB_FinishStageTexturing( const shaderStage_t*, const drawSurf_t* );

////////////////////////////////////////////////////////////////////////
// RenderPasses

void RB_FillDepthBufferFast( const drawSurf_t * const * drawSurfs, int numDrawSurfs );

void RB_DrawInteractionsForward( const idRenderView * const );

void RB_ShadowMapPass( const drawSurf_t * const drawSurfs, const viewLight_t * const vLight );

void RB_StencilShadowPass( const drawSurf_t* const drawSurfs, const viewLight_t* const vLight );
void RB_StencilSelectLight( const viewLight_t* const vLight );

int RB_DrawTransMaterialPasses( const drawSurf_t* const* const drawSurfs, const int numDrawSurfs, const float guiStereoScreenOffset, const int stereoEye );

void RB_FogAllLights();