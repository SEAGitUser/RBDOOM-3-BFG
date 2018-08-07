#ifndef __RENDERPROGS_H__
#define __RENDERPROGS_H__

enum renderParm_t
{
	// For backwards compatibility, do not change the order of items

	RENDERPARM_MVPMATRIX_X,
	RENDERPARM_MVPMATRIX_Y,
	RENDERPARM_MVPMATRIX_Z,
	RENDERPARM_MVPMATRIX_W,

	RENDERPARM_MODELMATRIX_X,
	RENDERPARM_MODELMATRIX_Y,
	RENDERPARM_MODELMATRIX_Z,
	RENDERPARM_MODELMATRIX_W,

	RENDERPARM_MODELVIEWMATRIX_X,
	RENDERPARM_MODELVIEWMATRIX_Y,
	RENDERPARM_MODELVIEWMATRIX_Z,
	RENDERPARM_MODELVIEWMATRIX_W,

	RENDERPARM_INVERSEMODELVIEWMATRIX_X,
	RENDERPARM_INVERSEMODELVIEWMATRIX_Y,
	RENDERPARM_INVERSEMODELVIEWMATRIX_Z,
	RENDERPARM_INVERSEMODELVIEWMATRIX_W,

	//RENDERPARM_PROJMATRIX_X,
	//RENDERPARM_PROJMATRIX_Y,
	//RENDERPARM_PROJMATRIX_Z,
	//RENDERPARM_PROJMATRIX_W,

	RENDERPARM_AMBIENT_COLOR,

	RENDERPARM_ENABLE_SKINNING,
	RENDERPARM_ALPHA_TEST,

	RENDERPARM_SCREENCORRECTIONFACTOR,
	//RENDERPARM_WINDOWCOORD,
	//RENDERPARM_OVERBRIGHT,

	RENDERPARM_GLOBALLIGHTORIGIN,

	RENDERPARM_LOCALLIGHTORIGIN,
	RENDERPARM_LOCALVIEWORIGIN,

	RENDERPARM_LIGHTPROJECTION_S,
	RENDERPARM_LIGHTPROJECTION_T,
	RENDERPARM_LIGHTPROJECTION_P,
	RENDERPARM_LIGHTPROJECTION_Q,
	RENDERPARM_LIGHTFALLOFF_S,

	RENDERPARM_BASELIGHTPROJECT_S,
	RENDERPARM_BASELIGHTPROJECT_T,
	RENDERPARM_BASELIGHTPROJECT_R,
	RENDERPARM_BASELIGHTPROJECT_Q,

	RENDERPARM_DIFFUSEMODIFIER,
	RENDERPARM_SPECULARMODIFIER,
	RENDERPARM_COLORMODIFIER,

	RENDERPARM_BUMPMATRIX_S,
	RENDERPARM_BUMPMATRIX_T,

	RENDERPARM_DIFFUSEMATRIX_S,
	RENDERPARM_DIFFUSEMATRIX_T,

	RENDERPARM_SPECULARMATRIX_S,
	RENDERPARM_SPECULARMATRIX_T,

	RENDERPARM_VERTEXCOLOR_MAD,
	RENDERPARM_COLOR,

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

	RENDERPARM_JITTERTEXSCALE,
	RENDERPARM_JITTERTEXOFFSET,
	//RENDERPARM_CASCADEDISTANCES,

	//RENDERPARM_VIEWORIGIN,
	//RENDERPARM_GLOBALEYEPOS,

	RENDERPARM_USERVEC0,
	RENDERPARM_USERVEC1,
	RENDERPARM_USERVEC2,
	RENDERPARM_USERVEC3,
	RENDERPARM_USERVEC4,
	RENDERPARM_USERVEC5,
	RENDERPARM_USERVEC6,
	RENDERPARM_USERVEC7,
	RENDERPARM_USERVEC8,
	RENDERPARM_USERVEC9,
	RENDERPARM_USERVEC10,
	RENDERPARM_USERVEC11,
	RENDERPARM_USERVEC12,
	RENDERPARM_USERVEC13,
	RENDERPARM_USERVEC14,
	RENDERPARM_USERVEC15,
	RENDERPARM_USERVEC16,
	RENDERPARM_USERVEC17,
	RENDERPARM_USERVEC18,
	RENDERPARM_USERVEC19,
	RENDERPARM_USERVEC20,
	RENDERPARM_USERVEC21,
	RENDERPARM_USERVEC22,
	RENDERPARM_USERVEC23,
	RENDERPARM_USERVEC24,
	RENDERPARM_USERVEC25,
	RENDERPARM_USERVEC26,
	RENDERPARM_USERVEC27,
	RENDERPARM_USERVEC28,
	RENDERPARM_USERVEC29,
	RENDERPARM_USERVEC30,
	RENDERPARM_USERVEC31,

	// Textures -------------------------------------

	RENDERPARM_BUMPMAP,
	RENDERPARM_DIFFUSEMAP,
	RENDERPARM_SPECULARMAP,

	RENDERPARM_LIGHTPROJECTMAP,
	RENDERPARM_LIGHTFALLOFFMAP,

	RENDERPARM_MAP,
	RENDERPARM_CUBEMAP,
	RENDERPARM_ENVIROCUBEMAP,
	RENDERPARM_MASKMAP,
	RENDERPARM_COVERAGEMAP,

	RENDERPARM_MAPY,
	RENDERPARM_MAPCR,
	RENDERPARM_MAPCB,

	RENDERPARM_FOGMAP,
	RENDERPARM_FOGENTERMAP,

	RENDERPARM_NOFALLOFFMAP,
	RENDERPARM_SCRATCHIMAGE,
	RENDERPARM_SCRATCHIMAGE2,
	RENDERPARM_ACCUMMAP,

	RENDERPARM_CURRENTRENDERMAP,
	RENDERPARM_VIEWDEPTHSTENCILMAP,

	RENDERPARM_JITTERMAP,

	RENDERPARM_RANDOMIMAGE256,
	RENDERPARM_GRAINMAP,

	RENDERPARM_VIEWCOLORMAP,
	RENDERPARM_VIEWNORMALMAP,

	RENDERPARM_SHADOWBUFFERMAP,
	RENDERPARM_SHADOWBUFFERDEBUGMAP,

	RENDERPARM_BLOOMRENDERMAP,

	RENDERPARM_SMAAINPUTIMAGE,
	RENDERPARM_SMAAAREAIMAGE,
	RENDERPARM_SMAASEARCHIMAGE,
	RENDERPARM_SMAAEDGESIMAGE,
	RENDERPARM_SMAABLENDIMAGE,

	RENDERPARM_EDITORMAP,

	RENDERPARM_USERMAP0,
	RENDERPARM_USERMAP1,
	RENDERPARM_USERMAP2,
	RENDERPARM_USERMAP3,
	RENDERPARM_USERMAP4,
	RENDERPARM_USERMAP5,
	RENDERPARM_USERMAP6,
	RENDERPARM_USERMAP7,
	RENDERPARM_USERMAP8,
	RENDERPARM_USERMAP9,
	RENDERPARM_USERMAP10,
	RENDERPARM_USERMAP11,
	RENDERPARM_USERMAP12,
	RENDERPARM_USERMAP13,
	RENDERPARM_USERMAP14,
	RENDERPARM_USERMAP15,
	//RENDERPARM_USERMAP16,
	//RENDERPARM_USERMAP17,
	//RENDERPARM_USERMAP18,
	//RENDERPARM_USERMAP19,
	//RENDERPARM_USERMAP20,
	//RENDERPARM_USERMAP21,
	//RENDERPARM_USERMAP22,
	//RENDERPARM_USERMAP23,
	//RENDERPARM_USERMAP24,
	//RENDERPARM_USERMAP25,
	//RENDERPARM_USERMAP26,
	//RENDERPARM_USERMAP27,
	//RENDERPARM_USERMAP28,
	//RENDERPARM_USERMAP29,
	//RENDERPARM_USERMAP30,
	//RENDERPARM_USERMAP31,

	RENDERPARM_TOTAL,
};

typedef const idDeclRenderProg * idRenderProg;
typedef const idDeclRenderParm * idRenderParm;

/*
================================================================================================

	idRenderProgManager

================================================================================================
*/
class idRenderProgManager {
	friend class idDeclRenderProg;
public:
	idRenderProgManager();
	virtual ~idRenderProgManager();

	void				Init();
	void				Shutdown();
	// this should only be called via the reload shader console command
	void				Reload( bool all );
	idRenderProg		FindRenderProgram( const char * progName ) const;

	ID_INLINE idRenderParm GetRenderParm( renderParm_t rp ) const { return builtinParms[ rp ]; }
	void				SetRenderParm( renderParm_t, idImage * );
	void				SetRenderParm( renderParm_t, const idRenderVector & );
	void				SetRenderParm( renderParm_t, const float * value );
	ID_INLINE void		SetRenderParms( renderParm_t rp, const float * values, int numValues )
	{
		for( int i = 0; i < numValues; i++ ) {
			SetRenderParm( ( renderParm_t )( rp + i ), values + ( i * 4 ) );
		}
	}

	void				SetupTextureMatrixParms( const float* registers, const textureStage_t* ) const;

	/*
	===========================
	VertexColorMAD
	===========================
	*/
	ID_INLINE void		SetVertexColorParm( stageVertexColor_t svc )const
	{
	#if USE_INTRINSICS
		static const __m128 vc[ 3 ] = {
			_mm_set_ps( 0.0, 0.0, 1.0, 0.0 ),
			_mm_set_ps( 0.0, 0.0, 0.0, 1.0 ),
			_mm_set_ps( 0.0, 0.0, 1.0,-1.0 ) };
		_mm_store_ps( GetRenderParm( RENDERPARM_VERTEXCOLOR_MAD )->GetVecPtr(), vc[ svc ] );
	#else
		switch( svc ) {
			case SVC_IGNORE: GetRenderParm( RENDERPARM_VERTEXCOLOR_MAD )->GetVec4().Set( 0, 1, 0, 0 ); break;
			case SVC_MODULATE: GetRenderParm( RENDERPARM_VERTEXCOLOR_MAD )->GetVec4().Set( 1, 0, 0, 0 ); break;
			case SVC_INVERSE_MODULATE: GetRenderParm( RENDERPARM_VERTEXCOLOR_MAD )->GetVec4().Set( -1, 1, 0, 0 ); break;
		}
	#endif
	}
	/*
	===========================
	SkinningParms
	===========================
	*/
	ID_INLINE void		EnableSkinningParm() const
	{
	#if USE_INTRINSICS
		_mm_store_ps( GetRenderParm( RENDERPARM_ENABLE_SKINNING )->GetVecPtr(), idMath::SIMD_SP_one );
	#else
		renderProgManager.GetRenderParm( RENDERPARM_ENABLE_SKINNING )->GetVec4().Fill( 1.0 );
	#endif
	}
	ID_INLINE void		DisableSkinningParm() const
	{
	#if USE_INTRINSICS
		_mm_store_ps( GetRenderParm( RENDERPARM_ENABLE_SKINNING )->GetVecPtr(), _mm_setzero_ps() );
	#else
		renderProgManager.GetRenderParm( RENDERPARM_ENABLE_SKINNING )->GetVec4().Fill( 0.0 );
	#endif
	}
	ID_INLINE void		SetSkinningParm( const bool hasJoints ) const
	{
	#if USE_INTRINSICS
		_mm_store_ps( GetRenderParm( RENDERPARM_ENABLE_SKINNING )->GetVecPtr(), hasJoints ? idMath::SIMD_SP_one : _mm_setzero_ps() );
	#else
		renderProgManager.GetRenderParm( RENDERPARM_ENABLE_SKINNING )->GetVec4().Fill( hasJoints ? 1.0 : 0.0 );
	#endif
	}
	/*
	===========================
	AlphaTest
	===========================
	*/
	ID_INLINE void		EnableAlphaTestParm( const float cmpValue ) const
	{
	#if USE_INTRINSICS
		_mm_store_ps( GetRenderParm( RENDERPARM_ALPHA_TEST )->GetVecPtr(), _mm_load1_ps( &cmpValue ) );
	#else
		renderProgManager.GetRenderParm( RENDERPARM_ALPHA_TEST )->GetVec4().Fill( cmpValue );
	#endif
	}
	ID_INLINE void		DisableAlphaTestParm() const
	{
	#if USE_INTRINSICS
		_mm_store_ps( GetRenderParm( RENDERPARM_ALPHA_TEST )->GetVecPtr(), _mm_setzero_ps() );
	#else
		renderProgManager.GetRenderParm( RENDERPARM_ALPHA_TEST )->GetVec4().Fill( 0.0 );
	#endif
	}
	/*
	===========================
	TexGen0Enabled
	===========================
	*/
	ID_INLINE void		EnableTexgen0Parm() const
	{
	#if USE_INTRINSICS
		_mm_store_ps( GetRenderParm( RENDERPARM_TEXGEN_0_ENABLED )->GetVecPtr(), idMath::SIMD_SP_one );
	#else
		renderProgManager.GetRenderParm( RENDERPARM_ENABLE_SKINNING )->GetVec4().Fill( 1.0 );
	#endif
	}
	ID_INLINE void		DisableTexgen0Parm() const
	{
	#if USE_INTRINSICS
		_mm_store_ps( GetRenderParm( RENDERPARM_TEXGEN_0_ENABLED )->GetVecPtr(), _mm_setzero_ps() );
	#else
		renderProgManager.GetRenderParm( RENDERPARM_ENABLE_SKINNING )->GetVec4().Fill( 0.0 );
	#endif
	}
	/*
	====================
	GL_Color
	====================
	*/
	ID_INLINE void		SetColorParm( float r, float g, float b, float a ) const
	{
	#if USE_INTRINSICS
		_mm_store_ps( GetRenderParm( RENDERPARM_COLOR )->GetVecPtr(), _mm_min_ps( _mm_max_ps( _mm_set_ps( a, b, g, r ), idMath::SIMD_SP_zero ), idMath::SIMD_SP_one ) );
	#else
		auto parm = GetRenderParm( RENDERPARM_COLOR )->GetVecPtr();
		parm[ 0 ] = idMath::ClampFloat( 0.0f, 1.0f, r );
		parm[ 1 ] = idMath::ClampFloat( 0.0f, 1.0f, g );
		parm[ 2 ] = idMath::ClampFloat( 0.0f, 1.0f, b );
		parm[ 3 ] = idMath::ClampFloat( 0.0f, 1.0f, a );
	#endif
	}
	ID_INLINE void		SetColorParm( const idVec3& color ) const
	{
		SetColorParm( color[ 0 ], color[ 1 ], color[ 2 ], 1.0f );
	}
	ID_INLINE void		SetColorParm( const idVec4& color ) const
	{
		SetColorParm( color[ 0 ], color[ 1 ], color[ 2 ], color[ 3 ] );
	}
	ID_INLINE void		SetColorParm( float r, float g, float b ) const
	{
		SetColorParm( r, g, b, 1.0f );
	}

	ID_INLINE void		SetMVPMatrixParms( const idRenderMatrix & mvp ) const
	{
		idRenderMatrix::CopyMatrix( mvp,
									GetRenderParm( RENDERPARM_MVPMATRIX_X )->GetVec4(),
									GetRenderParm( RENDERPARM_MVPMATRIX_Y )->GetVec4(),
									GetRenderParm( RENDERPARM_MVPMATRIX_Z )->GetVec4(),
									GetRenderParm( RENDERPARM_MVPMATRIX_W )->GetVec4() );
	}
	ID_INLINE void		SetModelMatrixParms( const idRenderMatrix & modelMatrix ) const
	{
		idRenderMatrix::CopyMatrix( modelMatrix,
									GetRenderParm( RENDERPARM_MODELMATRIX_X )->GetVec4(),
									GetRenderParm( RENDERPARM_MODELMATRIX_Y )->GetVec4(),
									GetRenderParm( RENDERPARM_MODELMATRIX_Z )->GetVec4(),
									GetRenderParm( RENDERPARM_MODELMATRIX_W )->GetVec4() );
	}
	ID_INLINE void		SetModelViewMatrixParms( const idRenderMatrix & modelViewMatrix ) const
	{
		idRenderMatrix::CopyMatrix( modelViewMatrix,
									GetRenderParm( RENDERPARM_MODELVIEWMATRIX_X )->GetVec4(),
									GetRenderParm( RENDERPARM_MODELVIEWMATRIX_Y )->GetVec4(),
									GetRenderParm( RENDERPARM_MODELVIEWMATRIX_Z )->GetVec4(),
									GetRenderParm( RENDERPARM_MODELVIEWMATRIX_W )->GetVec4() );
	}
	ID_INLINE void		SetSurfaceSpaceMatrices( const drawSurf_t *const drawSurf ) const
	{
		SetModelMatrixParms( drawSurf->space->modelMatrix );
		SetModelViewMatrixParms( drawSurf->space->modelViewMatrix );
		SetMVPMatrixParms( drawSurf->space->mvp );
	}

	//void				LoadAllShaders( bool bPrint );
	//void				KillAllShaders();

	///void				CommitUniforms();
	void				ZeroUniforms();
	///void				SetUniformValue( renderParm_t, const float* value );

public:
	// Builtin programs -----------------------------------------------------

	idRenderProg prog_gui = nullptr;

	idRenderProg prog_color = nullptr;
	idRenderProg prog_color_skinned = nullptr;
	idRenderProg prog_vertex_color = nullptr;
	idRenderProg prog_ambient_lighting = nullptr;
	idRenderProg prog_ambient_lighting_skinned = nullptr;

	idRenderProg prog_gbuffer = nullptr;
	idRenderProg prog_gbuffer_skinned = nullptr;

	idRenderProg prog_gbuffer_sml = nullptr;
	idRenderProg prog_gbuffer_sml_skinned = nullptr;
	idRenderProg prog_gbuffer_clear = nullptr;

	idRenderProg prog_fill_gbuffer_trivial = nullptr;
	idRenderProg prog_fill_gbuffer = nullptr;

	idRenderProg prog_depth_world = nullptr;

	idRenderProg prog_texture = nullptr;
	idRenderProg prog_texture_color = nullptr;
	idRenderProg prog_texture_color_srgb = nullptr;
	idRenderProg prog_texture_color_skinned = nullptr;
	idRenderProg prog_texture_color_texgen = nullptr;

	idRenderProg prog_interaction_ambient = nullptr;
	idRenderProg prog_interaction_ambient_skinned = nullptr;
	idRenderProg prog_interaction = nullptr;
	idRenderProg prog_interaction_skinned = nullptr;
	idRenderProg prog_interaction_sm_spot = nullptr;
	idRenderProg prog_interaction_sm_spot_skinned = nullptr;
	idRenderProg prog_interaction_sm_point = nullptr;
	idRenderProg prog_interaction_sm_point_skinned = nullptr;
	idRenderProg prog_interaction_sm_parallel = nullptr;
	idRenderProg prog_interaction_sm_parallel_skinned = nullptr;
	idRenderProg prog_interaction_sm_deferred_light = nullptr;
	idRenderProg prog_interaction_deferred_light = nullptr;

	idRenderProg prog_shadowBuffer_clear = nullptr;
	idRenderProg prog_shadowBuffer_point = nullptr;
	idRenderProg prog_shadowBuffer_perforated_point = nullptr;
	idRenderProg prog_shadowBuffer_static_point = nullptr;
	idRenderProg prog_shadowBuffer_spot = nullptr;
	idRenderProg prog_shadowBuffer_perforated_spot = nullptr;
	idRenderProg prog_shadowbuffer_static_spot = nullptr;
	idRenderProg prog_shadowBuffer_parallel = nullptr;
	idRenderProg prog_shadowBuffer_perforated_parallel = nullptr;
	idRenderProg prog_shadowBuffer_static_parallel = nullptr;
	idRenderProg prog_debug_shadowmap = nullptr;

	idRenderProg prog_stencil_shadow = nullptr;
	idRenderProg prog_stencil_shadow_skinned = nullptr;
	idRenderProg prog_stencil_shadowDebug = nullptr;
	idRenderProg prog_stencil_shadowDebug_skinned = nullptr;


	idRenderProg prog_environment = nullptr;
	idRenderProg prog_environment_skinned = nullptr;
	idRenderProg prog_bumpyenvironment = nullptr;
	idRenderProg prog_bumpyenvironment_skinned = nullptr;

	idRenderProg prog_depth = nullptr;
	idRenderProg prog_depth_skinned = nullptr;
	idRenderProg prog_HiZmap_construction = nullptr;

	idRenderProg prog_blendlight = nullptr;
	idRenderProg prog_blendlight2 = nullptr;
	idRenderProg prog_fog = nullptr;
	idRenderProg prog_fog_skinned = nullptr;
	idRenderProg prog_fog2 = nullptr;

	idRenderProg prog_skybox = nullptr;
	idRenderProg prog_wobblesky = nullptr;
	idRenderProg prog_wobblesky2 = nullptr;

	idRenderProg prog_postprocess = nullptr;

	idRenderProg prog_screen = nullptr;
	idRenderProg prog_tonemap = nullptr;
	idRenderProg prog_tonemap_brightpass = nullptr;
	idRenderProg prog_hdr_glare_chromatic = nullptr;
	idRenderProg prog_tonemap_debug = nullptr;

	idRenderProg prog_SMAA_edge_detection = nullptr;
	idRenderProg prog_SMAA_blending_weight_calc = nullptr;
	idRenderProg prog_SMAA_final = nullptr;

	idRenderProg prog_AmbientOcclusion_AO = nullptr;
	idRenderProg prog_AmbientOcclusion_AO_write = nullptr;
	idRenderProg prog_AmbientOcclusion_blur = nullptr;
	idRenderProg prog_AmbientOcclusion_blur_write = nullptr;
	idRenderProg prog_AmbientOcclusion_minify = nullptr;
	idRenderProg prog_AmbientOcclusion_minify_mip0 = nullptr;
	idRenderProg prog_DeepGBufferRadiosity_radiosity = nullptr;
	idRenderProg prog_DeepGBufferRadiosity_blur = nullptr;
	idRenderProg prog_DeepGBufferRadiosity_blur_write = nullptr;

	idRenderProg prog_stereoDeGhost = nullptr;
	idRenderProg prog_stereoWarp = nullptr;
	idRenderProg prog_stereoInterlace = nullptr;

//	idRenderProg prog_zcullReconstruct = nullptr;

	idRenderProg prog_bink = nullptr;
	idRenderProg prog_bink_gui = nullptr;

	idRenderProg prog_motionBlurMask = nullptr;
	idRenderProg prog_motionBlur = nullptr;

	idRenderProg prog_copyColor = nullptr;
	idRenderProg prog_copyColorAndPack = nullptr;

	// ----------------------

//	idRenderProg prog_autoExposure = nullptr;

	idRenderProg prog_bloomExtractLuminance = nullptr;
	idRenderProg prog_bloomExtractAvarage = nullptr;
	idRenderProg prog_bloomDownscaleBlur = nullptr;
	idRenderProg prog_bloomUpscaleBlur = nullptr;

	// ----------------------------------------

//	void BindShader_AmbientLighting( const bool skinning ) { BindRenderProgram( skinning ? prog_ambient_lighting_skinned : prog_ambient_lighting ); }
//	void BindShader_SmallGeometryBuffer( const bool skinning ) { BindRenderProgram( skinning ? prog_small_geometry_buffer_skinned : prog_small_geometry_buffer ); }
//	void BindShader_SimpleShade() { BindRenderProgram( prog_simpleshade ); }
	///void BindShader_StereoDeGhost() { BindRenderProgram( prog_stereoDeGhost ); }
//	void BindShader_ZCullReconstruct() { BindRenderProgram( prog_zcull_reconstruct ); }

protected:

	idArray<idRenderParm, RENDERPARM_TOTAL> builtinParms;
};

extern idRenderProgManager renderProgManager;

#endif
