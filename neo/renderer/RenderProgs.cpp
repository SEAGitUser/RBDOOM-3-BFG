/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2014 Robert Beckebans

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

#pragma hdrstop
#include "precompiled.h"

#include "tr_local.h"

idRenderProgManager renderProgManager;

/*
================================================================================================
idRenderProgManager::idRenderProgManager()
================================================================================================
*/
idRenderProgManager::idRenderProgManager()
{
}

/*
================================================================================================
idRenderProgManager::~idRenderProgManager()
================================================================================================
*/
idRenderProgManager::~idRenderProgManager()
{
}

/*
================================================================================================
R_ReloadShaders
================================================================================================
*/
static void R_ReloadShaders( const idCmdArgs& args )
{
	renderProgManager.KillAllShaders();
	renderProgManager.LoadAllShaders( false );
}

/*
================================================================================================
idRenderProgManager::Init()
================================================================================================
*/
void idRenderProgManager::Init()
{
	common->Printf( "------ Initializing Render Parms -------\n" );

	struct builtins_t {
		int			index;
		const char * name;
	};

	builtins_t builtin_parms[] =
	{
		{ RENDERPARM_MVPMATRIX_X,				"MVPmatrixX" },
		{ RENDERPARM_MVPMATRIX_Y,				"MVPmatrixY" },
		{ RENDERPARM_MVPMATRIX_Z,				"MVPmatrixZ" },
		{ RENDERPARM_MVPMATRIX_W,				"MVPmatrixW" },
		{ RENDERPARM_MODELMATRIX_X,				"ModelMatrixX" },
		{ RENDERPARM_MODELMATRIX_Y,				"ModelMatrixY" },
		{ RENDERPARM_MODELMATRIX_Z,				"ModelMatrixZ" },
		{ RENDERPARM_MODELMATRIX_W,				"ModelMatrixW" },
		{ RENDERPARM_MODELVIEWMATRIX_X,			"ModelViewMatrixX" },
		{ RENDERPARM_MODELVIEWMATRIX_Y,			"ModelViewMatrixY" },
		{ RENDERPARM_MODELVIEWMATRIX_Z,			"ModelViewMatrixZ" },
		{ RENDERPARM_MODELVIEWMATRIX_W,			"ModelViewMatrixW" },
		{ RENDERPARM_INVERSEMODELVIEWMATRIX_X,	"InverseModelMatrixX" },
		{ RENDERPARM_INVERSEMODELVIEWMATRIX_Y,	"InverseModelMatrixY" },
		{ RENDERPARM_INVERSEMODELVIEWMATRIX_Z,	"InverseModelMatrixZ" },
		{ RENDERPARM_INVERSEMODELVIEWMATRIX_W,	"InverseModelMatrixW" },
		{ RENDERPARM_AMBIENT_COLOR,				"AmbientColor" },
		{ RENDERPARM_ENABLE_SKINNING,			"SkinningParms" },
		{ RENDERPARM_ALPHA_TEST,				"AlphaTest" },
		{ RENDERPARM_SCREENCORRECTIONFACTOR,	"ScreenCorrectionFactor" },
		//{ RENDERPARM_WINDOWCOORD,				"WindowCoord" },
		{ RENDERPARM_OVERBRIGHT,				"Overbright" },
		{ RENDERPARM_GLOBALLIGHTORIGIN,			"GlobalLightOrigin" },
		{ RENDERPARM_LOCALLIGHTORIGIN,			"LocalLightOrigin" },
		{ RENDERPARM_LOCALVIEWORIGIN,			"LocalViewOrigin" },
		{ RENDERPARM_LIGHTPROJECTION_S,			"LightProjectionS" },
		{ RENDERPARM_LIGHTPROJECTION_T,			"LightProjectionT" },
		{ RENDERPARM_LIGHTPROJECTION_P,			"LightProjectionP" },
		{ RENDERPARM_LIGHTPROJECTION_Q,			"LightProjectionQ" },
		{ RENDERPARM_LIGHTFALLOFF_S,			"LightFalloffS" },
		{ RENDERPARM_BASELIGHTPROJECT_S,		"BaseLightProjectS" },
		{ RENDERPARM_BASELIGHTPROJECT_T,		"BaseLightProjectT" },
		{ RENDERPARM_BASELIGHTPROJECT_R,		"BaseLightProjectR" },
		{ RENDERPARM_BASELIGHTPROJECT_Q,		"BaseLightProjectQ" },
		{ RENDERPARM_DIFFUSEMODIFIER,			"DiffuseModifier" },
		{ RENDERPARM_SPECULARMODIFIER,			"SpecularModifier" },
		{ RENDERPARM_COLORMODIFIER,				"ColorModifier" },
		{ RENDERPARM_BUMPMATRIX_S,				"BumpMatrixS" },
		{ RENDERPARM_BUMPMATRIX_T,				"BumpMatrixT" },
		{ RENDERPARM_DIFFUSEMATRIX_S,			"DiffuseMatrixS" },
		{ RENDERPARM_DIFFUSEMATRIX_T,			"DiffuseMatrixT" },
		{ RENDERPARM_SPECULARMATRIX_S,			"SpecularMatrixS" },
		{ RENDERPARM_SPECULARMATRIX_T,			"SpecularMatrixT" },
		{ RENDERPARM_VERTEXCOLOR_MODULATE,		"VertexColorModulate" },
		{ RENDERPARM_VERTEXCOLOR_ADD,			"VertexColorAdd" },
		{ RENDERPARM_VERTEXCOLOR_MAD,			"VertexColorMAD" },
		{ RENDERPARM_COLOR,						"Color" },
		{ RENDERPARM_TEXTUREMATRIX_S,			"TextureMatrixS" },
		{ RENDERPARM_TEXTUREMATRIX_T,			"TextureMatrixT" },
		{ RENDERPARM_TEXGEN_0_S,				"TexGen0S" },
		{ RENDERPARM_TEXGEN_0_T,				"TexGen0T" },
		{ RENDERPARM_TEXGEN_0_Q,				"TexGen0Q" },
		{ RENDERPARM_TEXGEN_0_ENABLED,			"TexGen0Enabled" },
		{ RENDERPARM_TEXGEN_1_S,				"TexGen1S" },
		{ RENDERPARM_TEXGEN_1_T,				"TexGen1T" },
		{ RENDERPARM_TEXGEN_1_Q,				"TexGen1Q" },
		{ RENDERPARM_TEXGEN_1_ENABLED,			"TexGen1Enabled" },
		{ RENDERPARM_WOBBLESKY_X,				"WobbleSkyX" },
		{ RENDERPARM_WOBBLESKY_Y,				"WobbleSkyY" },
		{ RENDERPARM_WOBBLESKY_Z,				"WobbleSkyZ" },
		{ RENDERPARM_JITTERTEXSCALE,			"JitterTexScale" },
		{ RENDERPARM_JITTERTEXOFFSET,			"JitterTexOffset" },

		{ RENDERPARM_USERVEC0,					"UserVec0" },
		{ RENDERPARM_USERVEC1,					"UserVec1" },
		{ RENDERPARM_USERVEC2,					"UserVec2" },
		{ RENDERPARM_USERVEC3,					"UserVec3" },
		{ RENDERPARM_USERVEC4,					"UserVec4" },
		{ RENDERPARM_USERVEC5,					"UserVec5" },
		{ RENDERPARM_USERVEC6,					"UserVec6" },
		{ RENDERPARM_USERVEC7,					"UserVec7" },
		{ RENDERPARM_USERVEC8,					"UserVec8" },
		{ RENDERPARM_USERVEC9,					"UserVec9" },
		{ RENDERPARM_USERVEC10,					"UserVec10" },
		{ RENDERPARM_USERVEC11,					"UserVec11" },
		{ RENDERPARM_USERVEC12,					"UserVec12" },
		{ RENDERPARM_USERVEC13,					"UserVec13" },
		{ RENDERPARM_USERVEC14,					"UserVec14" },
		{ RENDERPARM_USERVEC15,					"UserVec15" },
		{ RENDERPARM_USERVEC16,					"UserVec16" },
		{ RENDERPARM_USERVEC17,					"UserVec17" },
		{ RENDERPARM_USERVEC18,					"UserVec18" },
		{ RENDERPARM_USERVEC19,					"UserVec19" },
		{ RENDERPARM_USERVEC20,					"UserVec20" },
		{ RENDERPARM_USERVEC21,					"UserVec21" },
		{ RENDERPARM_USERVEC22,					"UserVec22" },
		{ RENDERPARM_USERVEC23,					"UserVec23" },
		{ RENDERPARM_USERVEC24,					"UserVec24" },
		{ RENDERPARM_USERVEC25,					"UserVec25" },
		{ RENDERPARM_USERVEC26,					"UserVec26" },
		{ RENDERPARM_USERVEC27,					"UserVec27" },
		{ RENDERPARM_USERVEC28,					"UserVec28" },
		{ RENDERPARM_USERVEC29,					"UserVec29" },
		{ RENDERPARM_USERVEC30,					"UserVec30" },
		{ RENDERPARM_USERVEC31,					"UserVec31" },

		{ RENDERPARM_BUMPMAP,				"bumpMap" },
		{ RENDERPARM_DIFFUSEMAP,			"diffuseMap" },
		{ RENDERPARM_SPECULARMAP,			"specularMap" },
		{ RENDERPARM_LIGHTPROJECTMAP,		"lightProjectMap" },
		{ RENDERPARM_LIGHTFALLOFFMAP,		"lightFalloffMap" },
		{ RENDERPARM_MAP,					"map" },
		{ RENDERPARM_CUBEMAP,				"cubeMap" },
		{ RENDERPARM_ENVIROCUBEMAP,			"enviroCubeMap" },
		{ RENDERPARM_MASKMAP,				"maskMap" },
		{ RENDERPARM_COVERAGEMAP,			"coverageMap" },
		{ RENDERPARM_MAPY,					"mapY" },
		{ RENDERPARM_MAPCR,					"mapCr" },
		{ RENDERPARM_MAPCB,					"mapCb" },
		{ RENDERPARM_FOGMAP,				"fogMap" },
		{ RENDERPARM_FOGENTERMAP,			"fogEnterMap" },
		{ RENDERPARM_NOFALLOFFMAP,			"noFalloffMap" },
		{ RENDERPARM_SCRATCHIMAGE,			"scratchImage" },
		{ RENDERPARM_SCRATCHIMAGE2,			"scratchImage2" },
		{ RENDERPARM_ACCUMMAP,				"accumMap" },
		{ RENDERPARM_CURRENTRENDERMAP,		"currentRenderMap" },
		{ RENDERPARM_CURRENTDEPTHMAP,		"currentDepthMap" },
		{ RENDERPARM_JITTERMAP,				"jitterMap" },
		{ RENDERPARM_RANDOMIMAGE256,		"randomImage256" },
		{ RENDERPARM_GRAINMAP,				"grainMap" },
		{ RENDERPARM_VIEWNORMALMAP,			"viewNormalMap" },
		{ RENDERPARM_SHADOWBUFFERMAP,		"shadowBufferMap" },
		{ RENDERPARM_SHADOWBUFFERDEBUGMAP,	"shadowBufferDebugMap" },
		{ RENDERPARM_BLOOMRENDERMAP,		"bloomRenderMap" },
		{ RENDERPARM_SMAAINPUTIMAGE,		"smaaInputImage" },
		{ RENDERPARM_SMAAAREAIMAGE,			"smaaAreaImage" },
		{ RENDERPARM_SMAASEARCHIMAGE,		"smaaSearchImage" },
		{ RENDERPARM_SMAAEDGESIMAGE,		"smaaEdgesImage" },
		{ RENDERPARM_SMAABLENDIMAGE,		"smaaBlendImage" },
		{ RENDERPARM_EDITORMAP,				"editorMap" },

		{ RENDERPARM_USERMAP0,				"UserMap0" },
		{ RENDERPARM_USERMAP1,				"UserMap1" },
		{ RENDERPARM_USERMAP2,				"UserMap2" },
		{ RENDERPARM_USERMAP3,				"UserMap3" },
		{ RENDERPARM_USERMAP4,				"UserMap4" },
		{ RENDERPARM_USERMAP5,				"UserMap5" },
		{ RENDERPARM_USERMAP6,				"UserMap6" },
		{ RENDERPARM_USERMAP7,				"UserMap7" },
		{ RENDERPARM_USERMAP8,				"UserMap8" },
		{ RENDERPARM_USERMAP9,				"UserMap9" },
		{ RENDERPARM_USERMAP10,				"UserMap10" },
		{ RENDERPARM_USERMAP11,				"UserMap11" },
		{ RENDERPARM_USERMAP12,				"UserMap12" },
		{ RENDERPARM_USERMAP13,				"UserMap13" },
		{ RENDERPARM_USERMAP14,				"UserMap14" },
		{ RENDERPARM_USERMAP15,				"UserMap15" },
	};
	builtinParms.Zero();
	for( int i = 0; i < _countof( builtin_parms ); ++i )
	{
		auto parm = declManager->FindRenderParm( builtin_parms[ i ].name );
		release_assert( parm != NULL );

		parm->Print();

		builtinParms[ builtin_parms[ i ].index ] = parm;
	}

	common->Printf( "----- Initializing Render Programs -----\n" );

	builtins_t builtin_progs[] =
	{
		{ BUILTIN_GUI, "gui" },
		{ BUILTIN_COLOR, "color" },
		// RB begin
		{ BUILTIN_COLOR_SKINNED, "color_skinned" },
		{ BUILTIN_VERTEX_COLOR, "vertex_color" },
		{ BUILTIN_AMBIENT_LIGHTING, "ambient_lighting" },
		{ BUILTIN_AMBIENT_LIGHTING_SKINNED, "ambient_lighting_skinned" },
		{ BUILTIN_SMALL_GEOMETRY_BUFFER, "gbuffer" },
		{ BUILTIN_SMALL_GEOMETRY_BUFFER_SKINNED, "gbuffer_skinned" },
//SEA ->
		{ BUILTIN_SMALL_GBUFFER_SML, "gbuffer_sml" },
		{ BUILTIN_SMALL_GBUFFER_SML_SKINNED, "gbuffer_sml_skinned" },
		{ BUILTIN_DEPTH_WORLD, "depth_world" },
//SEA <-
		// RB end
//		{ BUILTIN_SIMPLESHADE, "simpleshade", 0, false },
		{ BUILTIN_TEXTURED, "texture" },
		{ BUILTIN_TEXTURE_VERTEXCOLOR, "texture_color" },
		{ BUILTIN_TEXTURE_VERTEXCOLOR_SRGB, "texture_color_srgb" },
		{ BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED, "texture_color_skinned" },
		{ BUILTIN_TEXTURE_TEXGEN_VERTEXCOLOR, "texture_color_texgen" },
		// RB begin
		{ BUILTIN_INTERACTION, "interaction" },
		{ BUILTIN_INTERACTION_SKINNED, "interaction_skinned" },
		{ BUILTIN_INTERACTION_AMBIENT, "interactionAmbient" },
		{ BUILTIN_INTERACTION_AMBIENT_SKINNED, "interactionAmbient_skinned" },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT, "interactionSM_spot"},
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT_SKINNED, "interactionSM_spot_skinned" },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_POINT, "interactionSM_point" },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_POINT_SKINNED, "interactionSM_point_skinned" },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL, "interactionSM_parallel" },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL_SKINNED, "interactionSM_parallel_skinned" },
		// RB end
		{ BUILTIN_ENVIRONMENT, "environment" },
		{ BUILTIN_ENVIRONMENT_SKINNED, "environment_skinned" },
		{ BUILTIN_BUMPY_ENVIRONMENT, "bumpyenvironment" },
		{ BUILTIN_BUMPY_ENVIRONMENT_SKINNED, "bumpyenvironment_skinned" },
		
		{ BUILTIN_DEPTH, "depth" },
		{ BUILTIN_DEPTH_SKINNED, "depth_skinned" },
		
		{ BUILTIN_SHADOW, "shadow" },
		{ BUILTIN_SHADOW_SKINNED, "shadow_skinned" },
		
		{ BUILTIN_SHADOW_DEBUG, "shadowDebug" },
		{ BUILTIN_SHADOW_DEBUG_SKINNED, "shadowDebug_skinned" },
		
		{ BUILTIN_BLENDLIGHT, "blendlight" },
		{ BUILTIN_BLENDLIGHT_SCREENSPACE, "blendlight2" },
		{ BUILTIN_FOG, "fog" },
		{ BUILTIN_FOG_SKINNED, "fog_skinned" },
		{ BUILTIN_SKYBOX, "skybox" },
		{ BUILTIN_WOBBLESKY, "wobblesky" },
		{ BUILTIN_POSTPROCESS, "postprocess"},
		// RB begin
		{ BUILTIN_SCREEN, "screen" },
		{ BUILTIN_TONEMAP, "tonemap" },
		{ BUILTIN_BRIGHTPASS, "tonemap_brightpass" },
		{ BUILTIN_HDR_GLARE_CHROMATIC, "hdr_glare_chromatic" },
		{ BUILTIN_HDR_DEBUG, "tonemap_debug" },
		
		{ BUILTIN_SMAA_EDGE_DETECTION, "SMAA_edge_detection" },
		{ BUILTIN_SMAA_BLENDING_WEIGHT_CALCULATION, "SMAA_blending_weight_calc" },
		{ BUILTIN_SMAA_NEIGHBORHOOD_BLENDING, "SMAA_final" },
		
		{ BUILTIN_AMBIENT_OCCLUSION, "AmbientOcclusion_AO" },
		{ BUILTIN_AMBIENT_OCCLUSION_AND_OUTPUT, "AmbientOcclusion_AO_write" },
		{ BUILTIN_AMBIENT_OCCLUSION_BLUR, "AmbientOcclusion_blur", },
		{ BUILTIN_AMBIENT_OCCLUSION_BLUR_AND_OUTPUT, "AmbientOcclusion_blur_write" },
		{ BUILTIN_AMBIENT_OCCLUSION_MINIFY, "AmbientOcclusion_minify" },
		{ BUILTIN_AMBIENT_OCCLUSION_RECONSTRUCT_CSZ, "AmbientOcclusion_minify_mip0" },
		{ BUILTIN_DEEP_GBUFFER_RADIOSITY_SSGI, "DeepGBufferRadiosity_radiosity" },
		{ BUILTIN_DEEP_GBUFFER_RADIOSITY_BLUR, "DeepGBufferRadiosity_blur" },
		{ BUILTIN_DEEP_GBUFFER_RADIOSITY_BLUR_AND_OUTPUT, "DeepGBufferRadiosity_blur_write" },
		// RB end
		{ BUILTIN_STEREO_DEGHOST, "stereoDeGhost" },
		{ BUILTIN_STEREO_WARP, "stereoWarp" },
//		{ BUILTIN_ZCULL_RECONSTRUCT, "zcullReconstruct" },
		{ BUILTIN_BINK, "bink" },
		{ BUILTIN_BINK_GUI, "bink_gui" },
		{ BUILTIN_STEREO_INTERLACE, "stereoInterlace" },
		{ BUILTIN_MOTION_BLUR, "motionBlur" },
		
		// RB begin
		{ BUILTIN_DEBUG_SHADOWMAP, "debug_shadowmap" },
		// RB end
	};
	builtinProgs.Zero();
	for( int i = 0; i < _countof( builtin_progs ); ++i )
	{
		auto prog = declManager->FindRenderProg( builtin_progs[ i ].name );
		release_assert( prog != NULL );

		prog->Print();

		builtinProgs[ builtin_progs[ i ].index ] = prog;
	}

	r_useHalfLambertLighting.ClearModified();
	r_useHDR.ClearModified();
	r_useProgUBO.ClearModified();

	cmdSystem->AddCommand( "reloadShaders", R_ReloadShaders, CMD_FL_RENDERER, "reloads render programs" );

/*#ifdef ID_PC
	idParser::AddGlobalDefine( "PC" );

	if( glConfig.driverType == GLDRV_OPENGL_ES3 ) {
		idParser::AddGlobalDefine( "GLES" );
	} else {
		idParser::AddGlobalDefine( "GLSL" );
	}

	if( glConfig.vendor == VENDOR_NVIDIA ) {
		idParser::AddGlobalDefine( "_NVIDIA_" );
	}
	else if( glConfig.vendor == VENDOR_AMD ) {
		idParser::AddGlobalDefine( "_AMD_" );
	}
	else if( glConfig.vendor == VENDOR_INTEL ) {
		idParser::AddGlobalDefine( "_INTEL_" );
	}

	extern idCVar r_useProgUBO;
	if( r_useProgUBO.GetBool() ) {
		idParser::AddGlobalDefine( "USE_UBO_PARMS" );
	}

	if( r_useHalfLambertLighting.GetBool() ) {
		idParser::AddGlobalDefine( "USE_HALF_LAMBERT" );
	}
	if( r_useHDR.GetBool() ) {
		idParser::AddGlobalDefine( "USE_LINEAR_RGB" );
	}

	if( GLEW_ARB_gpu_shader5 ) {
		idParser::AddGlobalDefine( "GS_INSTANCED" );
		idParser::AddGlobalDefine( "TEXTURE_GATHER" );
	}
	if( GLEW_ARB_cull_distance ) {
		idParser::AddGlobalDefine( "CULL_DISTANCE" );
	}

	// SMAA configuration
	idParser::AddGlobalDefine( "SMAA_GLSL_3" );
	idParser::AddGlobalDefine( "SMAA_RT_METRICS $rpScreenCorrectionFactor" );
	idParser::AddGlobalDefine( "SMAA_PRESET_HIGH" );
#else
	#error Non PC world. Implement Me!
#endif*/

	//LoadAllShaders( true );

	//TestParseProg();

	//idParser::RemoveAllGlobalDefines();
}

/*
================================================================================================
idRenderProgManager::LoadAllShaders()
================================================================================================
*/
void idRenderProgManager::LoadAllShaders( bool bPrint ) // ???
{
	for( int index = 0; index < declManager->GetNumDecls( DECL_RENDERPROG ); ++index )
	{
		auto decl = declManager->RenderProgByIndex( index, true );
		if( decl )
		{
			if( bPrint )
			{
				decl->Print();
			}
		}
	}
}

/*
================================================================================================
idRenderProgManager::KillAllShaders()
================================================================================================
*/
void idRenderProgManager::KillAllShaders()
{
	Unbind();

	for( int index = 0; index < declManager->GetNumDecls( DECL_RENDERPROG ); ++index )
	{
		auto prog = declManager->RenderProgByIndex( index, false );
		release_assert( prog != NULL );

		const_cast<idDeclRenderProg*>( prog )->FreeData();
	}

}

/*
================================================================================================
idRenderProgManager::Shutdown()
================================================================================================
*/
void idRenderProgManager::Shutdown()
{
	//KillAllShaders();
	//SEA: decl manager will handle this.	
}

/*
================================================================================================
idRenderProgManager::Unbind
================================================================================================
*/
void idRenderProgManager::Unbind()
{
	mCurrentDeclRenderProg = nullptr;
	glUseProgram( GL_NONE );
}

/*
================================================================================================
idRenderProgManager::GetRenderParm
================================================================================================
*/
idRenderVector & idRenderProgManager::GetRenderParm( renderParm_t rp )
{
	return builtinParms[ rp ]->GetVec4();
}

/*
================================================================================================
idRenderProgManager::SetRenderParm
================================================================================================
*/
void idRenderProgManager::SetRenderParm( renderParm_t rp, const float* vector )
{
	///SetUniformValue( rp, value );
	const idDeclRenderParm * parm = builtinParms[ rp ];
	release_assert( parm != NULL );

#if defined( USE_INTRINSICS )
	assert_16_byte_aligned( vector );
	_mm_store_ps( parm->GetVecPtr(), _mm_load_ps( vector ) );	
#else
	parm->Set( vector );
#endif
}
void idRenderProgManager::SetRenderParm( renderParm_t rp, const idRenderVector & vector )
{
	const idDeclRenderParm * parm = builtinParms[ rp ];
	release_assert( parm != NULL );

#if defined( USE_INTRINSICS )
	assert_16_byte_aligned( vector.ToFloatPtr() );
	_mm_store_ps( parm->GetVecPtr(), _mm_load_ps( vector.ToFloatPtr() ) );
#else
	parm->Set( vector );
#endif
}

/*
================================================================================================
idRenderProgManager::SetRenderParm
================================================================================================
*/
void idRenderProgManager::SetRenderParm( renderParm_t rp, idImage * img )
{
	builtinParms[ rp ]->Set( img );
}

/*
================================================================================================
idRenderProgManager::FindRenderProgram
================================================================================================
*/
const idDeclRenderProg * idRenderProgManager::FindRenderProgram( const char * progName ) const
{
	auto decl = declManager->FindType( DECL_RENDERPROG, progName, false );
	release_assert( decl != NULL );

	return decl->Cast<idDeclRenderProg>();
}
/*
================================================================================================
idRenderProgManager::BindRenderProg
================================================================================================
*/
void idRenderProgManager::BindRenderProgram( const idDeclRenderProg * prog )
{
	if( mCurrentDeclRenderProg != prog )
	{
		mCurrentDeclRenderProg = prog;

		RENDERLOG_PRINT( "Binding GLSL Program %s %s\n", prog->GetName(), prog->HasHardwareSkinning() ? "skinned" : "" );

		prog->Bind();
	}
}

/*
================================================================================================
idRenderProgManager::ZeroUniforms
================================================================================================
*/
void idRenderProgManager::ZeroUniforms()
{
	//memset( glslUniforms.Ptr(), 0, glslUniforms.Allocated() );

	for( int index = 0; index < declManager->GetNumDecls( DECL_RENDERPARM ); ++index )
	{
		auto decl = declManager->RenderParmByIndex( index, false );
		if( decl != NULL )
		{
			decl->SetDefaultData();
		}
	}
}

