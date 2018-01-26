/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2016 Robert Beckebans

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
#ifndef __RENDERPROGS_H__
#define __RENDERPROGS_H__

static const int PC_ATTRIB_INDEX_POSITION	= 0;
static const int PC_ATTRIB_INDEX_NORMAL		= 1;
static const int PC_ATTRIB_INDEX_COLOR		= 2;
static const int PC_ATTRIB_INDEX_COLOR2		= 3;
static const int PC_ATTRIB_INDEX_ST			= 4;
static const int PC_ATTRIB_INDEX_TANGENT	= 5;

/*
================================================
	vertexMask_t

	NOTE: There is a PS3 dependency between the bit flag specified here and the vertex
	attribute index and attribute semantic specified in DeclRenderProg.cpp because the
	stored render prog vertexMask is initialized with cellCgbGetVertexConfiguration().
	The ATTRIB_INDEX_ defines are used to make sure the vertexMask_t and attrib assignment
	in DeclRenderProg.cpp are in sync.

	Even though VERTEX_MASK_XYZ_SHORT and VERTEX_MASK_ST_SHORT are not real attributes,
	they come before the VERTEX_MASK_MORPH to reduce the range of vertex program
	permutations defined by the vertexMask_t bits on the Xbox 360 (see MAX_VERTEX_DECLARATIONS).
================================================
*/
enum vertexMask_t
{
	VERTEX_MASK_POSITION	= BIT( PC_ATTRIB_INDEX_POSITION ),
	VERTEX_MASK_ST			= BIT( PC_ATTRIB_INDEX_ST ),
	VERTEX_MASK_NORMAL		= BIT( PC_ATTRIB_INDEX_NORMAL ),
	VERTEX_MASK_COLOR		= BIT( PC_ATTRIB_INDEX_COLOR ),
	VERTEX_MASK_TANGENT		= BIT( PC_ATTRIB_INDEX_TANGENT ),
	VERTEX_MASK_COLOR2		= BIT( PC_ATTRIB_INDEX_COLOR2 ),
};

enum vertexAttributeIndex_e
{
	VS_IN_POSITION	,
	VS_IN_NORMAL	,
	VS_IN_COLOR		,
	VS_IN_COLOR2	,
	VS_IN_ST		,
	VS_IN_TANGENT	,
	VS_IN_MAX
};

enum shaderType_e {
	SHT_VERTEX,
	SHT_TESS_CTRL,		// GL_TESS_CONTROL / DX_HULL
	SHT_TESS_EVAL,		// GL_TESS_EVALUATION / DX_DOMAIN
	SHT_GEOMETRY,
	SHT_FRAGMENT,
	SHT_COMPUTE,
	SHT_MAX
};

// This enum list corresponds to the global constant register indecies as defined in global.inc for all
// shaders.  We used a shared pool to keeps things simple.  If something changes here then it also
// needs to change in global.inc and vice versa
enum renderParm_t
{
	// For backwards compatibility, do not change the order of the first 17 items
	RENDERPARM_SCREENCORRECTIONFACTOR = 0,
	RENDERPARM_WINDOWCOORD,
	RENDERPARM_DIFFUSEMODIFIER,
	RENDERPARM_SPECULARMODIFIER,
	
	RENDERPARM_LOCALLIGHTORIGIN,
	RENDERPARM_LOCALVIEWORIGIN,
	
	RENDERPARM_LIGHTPROJECTION_S,
	RENDERPARM_LIGHTPROJECTION_T,
	RENDERPARM_LIGHTPROJECTION_Q,
	RENDERPARM_LIGHTFALLOFF_S,
	
	RENDERPARM_BUMPMATRIX_S,
	RENDERPARM_BUMPMATRIX_T,
	
	RENDERPARM_DIFFUSEMATRIX_S,
	RENDERPARM_DIFFUSEMATRIX_T,
	
	RENDERPARM_SPECULARMATRIX_S,
	RENDERPARM_SPECULARMATRIX_T,
	
	RENDERPARM_VERTEXCOLOR_MODULATE,
	RENDERPARM_VERTEXCOLOR_ADD,
	
	// The following are new and can be in any order
	
	RENDERPARM_COLOR,
	RENDERPARM_VIEWORIGIN,
	RENDERPARM_GLOBALEYEPOS,
	
	RENDERPARM_MVPMATRIX_X,
	RENDERPARM_MVPMATRIX_Y,
	RENDERPARM_MVPMATRIX_Z,
	RENDERPARM_MVPMATRIX_W,
	
	RENDERPARM_MODELMATRIX_X,
	RENDERPARM_MODELMATRIX_Y,
	RENDERPARM_MODELMATRIX_Z,
	RENDERPARM_MODELMATRIX_W,
	
	RENDERPARM_PROJMATRIX_X,
	RENDERPARM_PROJMATRIX_Y,
	RENDERPARM_PROJMATRIX_Z,
	RENDERPARM_PROJMATRIX_W,
	
	RENDERPARM_MODELVIEWMATRIX_X,
	RENDERPARM_MODELVIEWMATRIX_Y,
	RENDERPARM_MODELVIEWMATRIX_Z,
	RENDERPARM_MODELVIEWMATRIX_W,
	
	RENDERPARM_TEXTUREMATRIX_S,
	RENDERPARM_TEXTUREMATRIX_T,
	
	RENDERPARM_TEXGEN_0_S,
	RENDERPARM_TEXGEN_0_T,
	RENDERPARM_TEXGEN_0_Q,
	RENDERPARM_TEXGEN_0_ENABLED,
	
	RENDERPARM_TEXGEN_1_S,
	RENDERPARM_TEXGEN_1_T,
	RENDERPARM_TEXGEN_1_Q,
	RENDERPARM_TEXGEN_1_ENABLED,
	
	RENDERPARM_WOBBLESKY_X,
	RENDERPARM_WOBBLESKY_Y,
	RENDERPARM_WOBBLESKY_Z,
	
	RENDERPARM_OVERBRIGHT,
	RENDERPARM_ENABLE_SKINNING,
	RENDERPARM_ALPHA_TEST,
	
	// RB begin
	RENDERPARM_AMBIENT_COLOR,
	
	RENDERPARM_GLOBALLIGHTORIGIN,
	RENDERPARM_JITTERTEXSCALE,
	RENDERPARM_JITTERTEXOFFSET,
	RENDERPARM_CASCADEDISTANCES,

	RENDERPARM_VERTEXCOLOR_MAD,		//SEA

	RENDERPARM_BASELIGHTPROJECT_S,	//SEA
	RENDERPARM_BASELIGHTPROJECT_T,	//SEA
	RENDERPARM_BASELIGHTPROJECT_R,	//SEA
	RENDERPARM_BASELIGHTPROJECT_Q,	//SEA
	
	RENDERPARM_SHADOW_MATRIX_0_X,	// rpShadowMatrices[6 * 4]
	RENDERPARM_SHADOW_MATRIX_0_Y,
	RENDERPARM_SHADOW_MATRIX_0_Z,
	RENDERPARM_SHADOW_MATRIX_0_W,
	
	RENDERPARM_SHADOW_MATRIX_1_X,
	RENDERPARM_SHADOW_MATRIX_1_Y,
	RENDERPARM_SHADOW_MATRIX_1_Z,
	RENDERPARM_SHADOW_MATRIX_1_W,
	
	RENDERPARM_SHADOW_MATRIX_2_X,
	RENDERPARM_SHADOW_MATRIX_2_Y,
	RENDERPARM_SHADOW_MATRIX_2_Z,
	RENDERPARM_SHADOW_MATRIX_2_W,
	
	RENDERPARM_SHADOW_MATRIX_3_X,
	RENDERPARM_SHADOW_MATRIX_3_Y,
	RENDERPARM_SHADOW_MATRIX_3_Z,
	RENDERPARM_SHADOW_MATRIX_3_W,
	
	RENDERPARM_SHADOW_MATRIX_4_X,
	RENDERPARM_SHADOW_MATRIX_4_Y,
	RENDERPARM_SHADOW_MATRIX_4_Z,
	RENDERPARM_SHADOW_MATRIX_4_W,
	
	RENDERPARM_SHADOW_MATRIX_5_X,
	RENDERPARM_SHADOW_MATRIX_5_Y,
	RENDERPARM_SHADOW_MATRIX_5_Z,
	RENDERPARM_SHADOW_MATRIX_5_W,
	// RB end
	
	RENDERPARM_TOTAL,
	RENDERPARM_USER = 128,
};


struct glslUniformLocation_t
{
	int		parmIndex;
	GLint	uniformIndex;
};



/*
================================================================================================
idRenderProgManager
================================================================================================
*/
class idRenderProgManager
{
public:
	idRenderProgManager();
	virtual ~idRenderProgManager();
	
	void	Init();
	void	Shutdown();
	
	idRenderVector & GetRenderParm( renderParm_t rp );
	void	SetRenderParm( renderParm_t rp, const float* value );
	void	SetRenderParms( renderParm_t rp, const float* values, int numValues );
	
	int		FindVertexShader( const char* name );
	int		FindGeometryShader( const char* name );
	int		FindFragmentShader( const char* name );
	
	// RB: added progIndex to handle many custom renderprogs
	void BindShader( int progIndex, int vIndex, int gIndex, int fIndex, bool builtin );
	// RB end
	
	void BindShader_GUI( ) {
		BindShader_Builtin( BUILTIN_GUI );
	}
	
	void BindShader_Color( const bool skinning ) {
		BindShader_Builtin( skinning ? BUILTIN_COLOR_SKINNED : BUILTIN_COLOR );
	}

	// RB begin
	void BindShader_VertexColor() {
		BindShader_Builtin( BUILTIN_VERTEX_COLOR );
	}

	void BindShader_AmbientLighting( const bool skinning ) {
		BindShader_Builtin( skinning ? BUILTIN_AMBIENT_LIGHTING_SKINNED : BUILTIN_AMBIENT_LIGHTING );
	}

	void BindShader_SmallGeometryBuffer( const bool skinning ) {
		BindShader_Builtin( skinning ? BUILTIN_SMALL_GEOMETRY_BUFFER_SKINNED : BUILTIN_SMALL_GEOMETRY_BUFFER );
	}
	// RB end

//SEA ->
	void BindShader_GBufferSml( const bool skinning ) {
		BindShader_Builtin( skinning ? BUILTIN_SMALL_GBUFFER_SML_SKINNED : BUILTIN_SMALL_GBUFFER_SML );
	}
	void BindShader_DepthWorld() {
		BindShader_Builtin( BUILTIN_DEPTH_WORLD );
	}
	void BindShader_FillShadowDepthBufferOnePass() {
		//BindShader_Builtin( BUILTIN_DEPTH_WORLD );
	}
	void BindShader_ScreenSpaceBlendLight()
	{
		BindShader_Builtin( BUILTIN_BLENDLIGHT_SCREENSPACE );
	}
//SEA <-

	void BindShader_Texture() {
		BindShader_Builtin( BUILTIN_TEXTURED );
	}

	void BindShader_TextureVertexColor( const bool skinning ) {
		BindShader_Builtin( skinning ? BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED : BUILTIN_TEXTURE_VERTEXCOLOR );
	};

	void BindShader_TextureVertexColor_sRGB() {
		BindShader_Builtin( BUILTIN_TEXTURE_VERTEXCOLOR_SRGB );
	};

	void BindShader_TextureTexGenVertexColor() {
		BindShader_Builtin( BUILTIN_TEXTURE_TEXGEN_VERTEXCOLOR );
	};

	void BindShader_Interaction( const bool skinning ) {
		BindShader_Builtin( skinning ? BUILTIN_INTERACTION_SKINNED : BUILTIN_INTERACTION );
	}
	void BindShader_InteractionAmbient( const bool skinning ) {
		BindShader_Builtin( skinning ? BUILTIN_INTERACTION_AMBIENT_SKINNED : BUILTIN_INTERACTION_AMBIENT );
	}

	// RB begin
	void BindShader_Interaction_ShadowMapping_Spot( const bool skinning ) {
		BindShader_Builtin( skinning ? BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT_SKINNED : BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT );
	}
	void BindShader_Interaction_ShadowMapping_Point( const bool skinning ) {
		BindShader_Builtin( skinning ? BUILTIN_INTERACTION_SHADOW_MAPPING_POINT_SKINNED : BUILTIN_INTERACTION_SHADOW_MAPPING_POINT );
	}
	void BindShader_Interaction_ShadowMapping_Parallel( const bool skinning ) {
		BindShader_Builtin( skinning ? BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL_SKINNED : BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL );
	}
	// RB end

	void BindShader_SimpleShade() {
		BindShader_Builtin( BUILTIN_SIMPLESHADE );
	}

	void BindShader_Environment( const bool skinning ) {
		BindShader_Builtin( skinning ? BUILTIN_ENVIRONMENT_SKINNED : BUILTIN_ENVIRONMENT );
	}

	void BindShader_BumpyEnvironment( const bool skinning ) {
		BindShader_Builtin( skinning ? BUILTIN_BUMPY_ENVIRONMENT_SKINNED : BUILTIN_BUMPY_ENVIRONMENT );
	}

	void BindShader_Depth( const bool skinning ) {
		BindShader_Builtin( skinning ? BUILTIN_DEPTH_SKINNED : BUILTIN_DEPTH );
	}

	void BindShader_Shadow( const bool skinning ) {
		// RB: no FFP fragment rendering anymore
		//BindShader( -1, builtinShaders[BUILTIN_SHADOW], -1, true );
		BindShader_Builtin( skinning ? BUILTIN_SHADOW_SKINNED : BUILTIN_SHADOW );
		// RB end
	}

	void BindShader_ShadowDebug( const bool skinning ) {
		BindShader_Builtin( skinning ? BUILTIN_SHADOW_DEBUG_SKINNED : BUILTIN_SHADOW_DEBUG );
	}

	void BindShader_BlendLight() {
		BindShader_Builtin( BUILTIN_BLENDLIGHT );
	}

	void BindShader_Fog( const bool skinning ) {
		BindShader_Builtin( skinning ? BUILTIN_FOG_SKINNED : BUILTIN_FOG );
	}

	void BindShader_SkyBox() {
		BindShader_Builtin( BUILTIN_SKYBOX );
	}
	
	void BindShader_WobbleSky() {
		BindShader_Builtin( BUILTIN_WOBBLESKY );
	}
	
	void BindShader_StereoDeGhost() {
		BindShader_Builtin( BUILTIN_STEREO_DEGHOST );
	}
	
	void BindShader_StereoWarp() {
		BindShader_Builtin( BUILTIN_STEREO_WARP );
	}
	
	void BindShader_StereoInterlace() {
		BindShader_Builtin( BUILTIN_STEREO_INTERLACE );
	}
	
	void BindShader_PostProcess() {
		BindShader_Builtin( BUILTIN_POSTPROCESS );
	}
	
	void BindShader_Screen() {
		BindShader_Builtin( BUILTIN_SCREEN );
	}
	
	void BindShader_Tonemap() {
		BindShader_Builtin( BUILTIN_TONEMAP );
	}
	
	void BindShader_Brightpass() {
		BindShader_Builtin( BUILTIN_BRIGHTPASS );
	}
	
	void BindShader_HDRGlareChromatic() {
		BindShader_Builtin( BUILTIN_HDR_GLARE_CHROMATIC );
	}
	
	void BindShader_HDRDebug() {
		BindShader_Builtin( BUILTIN_HDR_DEBUG );
	}
	
	void BindShader_SMAA_EdgeDetection() {
		BindShader_Builtin( BUILTIN_SMAA_EDGE_DETECTION );
	}
	
	void BindShader_SMAA_BlendingWeightCalculation() {
		BindShader_Builtin( BUILTIN_SMAA_BLENDING_WEIGHT_CALCULATION );
	}
	
	void BindShader_SMAA_NeighborhoodBlending() {
		BindShader_Builtin( BUILTIN_SMAA_NEIGHBORHOOD_BLENDING );
	}
	
	void BindShader_AmbientOcclusion() {
		BindShader_Builtin( BUILTIN_AMBIENT_OCCLUSION );
	}
	
	void BindShader_AmbientOcclusionAndOutput() {
		BindShader_Builtin( BUILTIN_AMBIENT_OCCLUSION_AND_OUTPUT );
	}
	
	void BindShader_AmbientOcclusionBlur() {
		BindShader_Builtin( BUILTIN_AMBIENT_OCCLUSION_BLUR );
	}
	
	void BindShader_AmbientOcclusionBlurAndOutput() {
		BindShader_Builtin( BUILTIN_AMBIENT_OCCLUSION_BLUR_AND_OUTPUT );
	}
	
	void BindShader_AmbientOcclusionMinify() {
		BindShader_Builtin( BUILTIN_AMBIENT_OCCLUSION_MINIFY );
	}
	
	void BindShader_AmbientOcclusionReconstructCSZ() {
		BindShader_Builtin( BUILTIN_AMBIENT_OCCLUSION_RECONSTRUCT_CSZ );
	}
	
	void BindShader_DeepGBufferRadiosity() {
		BindShader_Builtin( BUILTIN_DEEP_GBUFFER_RADIOSITY_SSGI );
	}
	
	void BindShader_DeepGBufferRadiosityBlur() {
		BindShader_Builtin( BUILTIN_DEEP_GBUFFER_RADIOSITY_BLUR );
	}
	
	void BindShader_DeepGBufferRadiosityBlurAndOutput() {
		BindShader_Builtin( BUILTIN_DEEP_GBUFFER_RADIOSITY_BLUR_AND_OUTPUT );
	}
	
#if 0
	void BindShader_ZCullReconstruct() {
		BindShader_Builtin( BUILTIN_ZCULL_RECONSTRUCT );
	}
#endif
	
	void BindShader_Bink() {
		BindShader_Builtin( BUILTIN_BINK );
	}
	
	void BindShader_BinkGUI() {
		BindShader_Builtin( BUILTIN_BINK_GUI );
	}
	
	void BindShader_MotionBlur() {
		BindShader_Builtin( BUILTIN_MOTION_BLUR );
	}
	
	void BindShader_DebugShadowMap() {
		BindShader_Builtin( BUILTIN_DEBUG_SHADOWMAP );
	}
	// RB end
	
	// the joints buffer should only be bound for vertex programs that use joints
	bool		ShaderUsesJoints() const { return vertShaders[ currentVertShader ].usesJoints; }
	// the rpEnableSkinning render parm should only be set for vertex programs that use it
	bool		ShaderHasOptionalSkinning() const { return vertShaders[ currentVertShader ].optionalSkinning; }
	
	// unbind the currently bound render program
	void		Unbind();
	
	// RB begin
	bool		IsShaderBound() const;
	// RB end
	
	// this should only be called via the reload shader console command
	void		LoadAllShaders();
	void		KillAllShaders();

	void		CommitUniforms();
	void		ZeroUniforms();
	void		SetUniformValue( const renderParm_t rp, const float* value );
	
	static const int MAX_GLSL_USER_PARMS = 8;
	const char*	GetGLSLParmName( int rp ) const;
	int			GetGLSLCurrentProgram() const { return currentRenderProgram; }
	int			FindGLSLProgram( const char* name, int vIndex, int fIndex );
	
protected:

	void	LoadShader( int index, shaderType_e shaderType );
	
	enum
	{
		BUILTIN_GUI,
		BUILTIN_COLOR,
		// RB begin
		BUILTIN_COLOR_SKINNED,
		BUILTIN_VERTEX_COLOR,
		BUILTIN_AMBIENT_LIGHTING,
		BUILTIN_AMBIENT_LIGHTING_SKINNED,
		BUILTIN_SMALL_GEOMETRY_BUFFER,
		BUILTIN_SMALL_GEOMETRY_BUFFER_SKINNED,
//SEA ->
		BUILTIN_SMALL_GBUFFER_SML,
		BUILTIN_SMALL_GBUFFER_SML_SKINNED,
		BUILTIN_DEPTH_WORLD,
//SEA <-
		// RB end
		BUILTIN_SIMPLESHADE,
		BUILTIN_TEXTURED,
		BUILTIN_TEXTURE_VERTEXCOLOR,
		BUILTIN_TEXTURE_VERTEXCOLOR_SRGB,
		BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED,
		BUILTIN_TEXTURE_TEXGEN_VERTEXCOLOR,
		BUILTIN_INTERACTION,
		BUILTIN_INTERACTION_SKINNED,
		BUILTIN_INTERACTION_AMBIENT,
		BUILTIN_INTERACTION_AMBIENT_SKINNED,
		// RB begin
		BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT,
		BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT_SKINNED,
		BUILTIN_INTERACTION_SHADOW_MAPPING_POINT,
		BUILTIN_INTERACTION_SHADOW_MAPPING_POINT_SKINNED,
		BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL,
		BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL_SKINNED,
		// RB end
		BUILTIN_ENVIRONMENT,
		BUILTIN_ENVIRONMENT_SKINNED,
		BUILTIN_BUMPY_ENVIRONMENT,
		BUILTIN_BUMPY_ENVIRONMENT_SKINNED,
		
		BUILTIN_DEPTH,
		BUILTIN_DEPTH_SKINNED,
		BUILTIN_SHADOW,
		BUILTIN_SHADOW_SKINNED,
		BUILTIN_SHADOW_DEBUG,
		BUILTIN_SHADOW_DEBUG_SKINNED,
		
		BUILTIN_BLENDLIGHT,
		BUILTIN_BLENDLIGHT_SCREENSPACE,
		BUILTIN_FOG,
		BUILTIN_FOG_SKINNED,
		BUILTIN_SKYBOX,
		BUILTIN_WOBBLESKY,
		BUILTIN_POSTPROCESS,
		// RB begin
		BUILTIN_SCREEN,
		BUILTIN_TONEMAP,
		BUILTIN_BRIGHTPASS,
		BUILTIN_HDR_GLARE_CHROMATIC,
		BUILTIN_HDR_DEBUG,
		
		BUILTIN_SMAA_EDGE_DETECTION,
		BUILTIN_SMAA_BLENDING_WEIGHT_CALCULATION,
		BUILTIN_SMAA_NEIGHBORHOOD_BLENDING,
		
		BUILTIN_AMBIENT_OCCLUSION,
		BUILTIN_AMBIENT_OCCLUSION_AND_OUTPUT,
		BUILTIN_AMBIENT_OCCLUSION_BLUR,
		BUILTIN_AMBIENT_OCCLUSION_BLUR_AND_OUTPUT,
		BUILTIN_AMBIENT_OCCLUSION_MINIFY,
		BUILTIN_AMBIENT_OCCLUSION_RECONSTRUCT_CSZ,
		
		BUILTIN_DEEP_GBUFFER_RADIOSITY_SSGI,
		BUILTIN_DEEP_GBUFFER_RADIOSITY_BLUR,
		BUILTIN_DEEP_GBUFFER_RADIOSITY_BLUR_AND_OUTPUT,
		// RB end
		BUILTIN_STEREO_DEGHOST,
		BUILTIN_STEREO_WARP,
		BUILTIN_ZCULL_RECONSTRUCT,
		BUILTIN_BINK,
		BUILTIN_BINK_GUI,
		BUILTIN_STEREO_INTERLACE,
		BUILTIN_MOTION_BLUR,
		
		BUILTIN_DEBUG_SHADOWMAP,
		
		MAX_BUILTINS
	};
	int builtinShaders[MAX_BUILTINS];
	void BindShader_Builtin( int i )
	{
		BindShader( -1, builtinShaders[i], builtinShaders[i], builtinShaders[i], true );
	}
	
	enum shaderFeature_t
	{
		USE_GPU_SKINNING,
		LIGHT_POINT,
		LIGHT_PARALLEL,
		BRIGHTPASS,
		HDR_DEBUG,
		USE_SRGB,
		
		MAX_SHADER_MACRO_NAMES,
	};
	
	static const char* GLSLMacroNames[MAX_SHADER_MACRO_NAMES];
	const char*	GetGLSLMacroName( shaderFeature_t sf ) const;
	
	GLuint  CreateGLSLShaderObject( shaderType_e shaderType, const char * sourceGLSL, const char * fileName ) const;
	GLuint	LoadGLSLShader( GLenum target, const char* name, const char* nameOutSuffix, uint32 shaderFeatures, bool builtin, idList<int>& uniforms );
	void	LoadGLSLProgram( const int programIndex, const int vertexShaderIndex, const int geometryShaderIndex, const int fragmentShaderIndex );
	
	static const GLuint INVALID_PROGID = MAX_UNSIGNED_TYPE( GLuint );
	typedef idHandle<GLuint, INVALID_PROGID> idGLSLShader;
		
	struct vertShader_t
	{
		vertShader_t() : progId( INVALID_PROGID ), usesJoints( false ), optionalSkinning( false ), shaderFeatures( 0 ), builtin( false ) {
		}
		idStr		name;
		idStr		nameOutSuffix;
		GLuint		progId;
		bool		usesJoints;
		bool		optionalSkinning;
		uint32		shaderFeatures;		// RB: Cg compile macros
		bool		builtin;			// RB: part of the core shaders built into the executable
		idList<int>	uniforms;
	};
	struct geomShader_t 
	{
		geomShader_t() : progId( INVALID_PROGID ), shaderFeatures( 0 ), builtin( false ) {
		}
		idStr		name;
		idStr		nameOutSuffix;
		GLuint		progId;
		uint32		shaderFeatures;		// RB: Cg compile macros
		bool		builtin;			// RB: part of the core shaders built into the executable
		idList<int>	uniforms;
	};
	struct fragShader_t
	{
		fragShader_t() : progId( INVALID_PROGID ), shaderFeatures( 0 ), builtin( false ) {
		}
		idStr		name;
		idStr		nameOutSuffix;
		GLuint		progId;
		uint32		shaderFeatures;
		bool		builtin;
		idList<int>	uniforms;
	};
	
	struct glslProgram_t
	{
		glslProgram_t() : progId( INVALID_PROGID ),
			vertShaderIndex( -1 ),
			geomShaderIndex( -1 ),
			fragShaderIndex( -1 ),
				vertUniformArray( -1 ),
				geomUniformArray( -1 ),
				fragUniformArray( -1 ) {
		}
		idStr	name;
		GLuint	progId;
		int		vertexMask;
		int			vertShaderIndex;
		int			geomShaderIndex;
		int			fragShaderIndex;
		GLint			vertUniformArray;
		GLint			geomUniformArray;
		GLint			fragUniformArray;
		idList<glslUniformLocation_t> uniformLocations;
	};
	int	currentRenderProgram;
	idList<glslProgram_t, TAG_RENDER> glslPrograms;
	idStaticList< idRenderVector, RENDERPARM_USER + MAX_GLSL_USER_PARMS > glslUniforms;
	
	int	currentVertShader = -1;
	int	currentGeomShader = -1;
	int	currentFragShader = -1;
	idList<vertShader_t, TAG_RENDER> vertShaders;
	idList<geomShader_t, TAG_RENDER> geomShaders;
	idList<fragShader_t, TAG_RENDER> fragShaders;
};

class idRenderProgram {
	void *	apiObject;
public:
	void *	GetAPIObject() const
	{
		return apiObject;
	}
};

extern idRenderProgManager renderProgManager;

#endif
