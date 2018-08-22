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
void R_ReloadProgs( const idCmdArgs& args )
{
	bool force;

	if( !idStr::Icmp( args.Argv( 1 ), "all" ) )
	{
		force = true;
		common->Printf( "reloading all prog files:\n" );
	}
	else {
		force = false;
		common->Printf( "reloading changed prog files:\n" );
	}

	renderProgManager.Reload( force );

	///renderProgManager.KillAllShaders();
	///renderProgManager.LoadAllShaders( false );
}

void idRenderProgManager::Reload( bool all )
{
	GL_ResetProgramState();

	/*for( int i = 0; i < declManager->GetNumDecls( DECL_RENDERPROG ); ++i )
	{
		auto prog = const_cast<idDeclRenderProg*>( declManager->RenderProgByIndex( i, false ) );
		assert( prog != NULL );

		if( prog->SourceFileChanged() || all )
		{
			prog->Invalidate();
			prog->FreeData();

			declManager->ReloadFile( prog->GetFileName(), true );
		}
	}*/

	declManager->ReloadType( DECL_RENDERPROG, true );
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
		{ RENDERPARM_INVERSEMODELVIEWMATRIX_X,	"InverseModelViewMatrixX" },
		{ RENDERPARM_INVERSEMODELVIEWMATRIX_Y,	"InverseModelViewMatrixY" },
		{ RENDERPARM_INVERSEMODELVIEWMATRIX_Z,	"InverseModelViewMatrixZ" },
		{ RENDERPARM_INVERSEMODELVIEWMATRIX_W,	"InverseModelViewMatrixW" },
		{ RENDERPARM_AMBIENT_COLOR,				"AmbientColor" },
		{ RENDERPARM_ENABLE_SKINNING,			"SkinningParms" },
		{ RENDERPARM_ALPHA_TEST,				"AlphaTest" },
		{ RENDERPARM_SCREENCORRECTIONFACTOR,	"ScreenCorrectionFactor" },
		//{ RENDERPARM_WINDOWCOORD,				"WindowCoord" },
		//{ RENDERPARM_OVERBRIGHT,				"Overbright" },
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
		{ RENDERPARM_VIEWDEPTHSTENCILMAP,		"viewDepthStencilMap" },
		{ RENDERPARM_JITTERMAP,				"jitterMap" },
		{ RENDERPARM_RANDOMIMAGE256,		"randomImage256" },
		{ RENDERPARM_GRAINMAP,				"grainMap" },
		{ RENDERPARM_VIEWCOLORMAP,			"viewColorMap" },
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
		assert( parm != NULL );
		if( com_developer.GetBool() ) {
			parm->Print();
		}
		builtinParms[ builtin_parms[ i ].index ] = parm;
	}

	common->Printf( "----- Initializing Render Programs -----\n" );

#define LOAD_PROG( Name ) prog_##Name = FindRenderProgram( #Name )

	LOAD_PROG( gui );

	LOAD_PROG( color );
	LOAD_PROG( color_skinned );
	LOAD_PROG( vertex_color );

	LOAD_PROG( ambient_lighting );
	LOAD_PROG( ambient_lighting_skinned );

	LOAD_PROG( gbuffer );
	LOAD_PROG( gbuffer_skinned );
	LOAD_PROG( gbuffer_sml );
	LOAD_PROG( gbuffer_sml_skinned );
	LOAD_PROG( gbuffer_clear );
	LOAD_PROG( fill_gbuffer_trivial );
	LOAD_PROG( fill_gbuffer );

	LOAD_PROG( texture );
	LOAD_PROG( texture_color );
	LOAD_PROG( texture_color_srgb );
	LOAD_PROG( texture_color_skinned );
	LOAD_PROG( texture_color_texgen );

	LOAD_PROG( interaction_ambient );
	LOAD_PROG( interaction_ambient_skinned );
	LOAD_PROG( interaction );
	LOAD_PROG( interaction_skinned );
	LOAD_PROG( interaction_sm_spot );
	LOAD_PROG( interaction_sm_spot_skinned );
	LOAD_PROG( interaction_sm_point );
	LOAD_PROG( interaction_sm_point_skinned );
	LOAD_PROG( interaction_sm_parallel );
	LOAD_PROG( interaction_sm_parallel_skinned );
	LOAD_PROG( interaction_sm_deferred_light );
	LOAD_PROG( interaction_deferred_light );

	LOAD_PROG( shadowBuffer_clear );
	LOAD_PROG( shadowBuffer_point );
	LOAD_PROG( shadowBuffer_perforated_point );
	LOAD_PROG( shadowBuffer_static_point );
	LOAD_PROG( shadowBuffer_spot );
	LOAD_PROG( shadowBuffer_perforated_spot );
	LOAD_PROG( shadowbuffer_static_spot );
	LOAD_PROG( shadowBuffer_parallel );
	LOAD_PROG( shadowBuffer_perforated_parallel );
	LOAD_PROG( shadowBuffer_static_parallel );
	LOAD_PROG( debug_shadowmap );

	LOAD_PROG( stencil_shadow );
	LOAD_PROG( stencil_shadow_skinned );
	LOAD_PROG( stencil_shadowDebug );
	LOAD_PROG( stencil_shadowDebug_skinned );

	LOAD_PROG( environment );
	LOAD_PROG( environment_skinned );
	LOAD_PROG( bumpyenvironment );
	LOAD_PROG( bumpyenvironment_skinned );

	LOAD_PROG( depth );
	LOAD_PROG( depth_skinned );
	LOAD_PROG( depth_world );
	LOAD_PROG( HiZmap_construction );

	LOAD_PROG( blendlight );
	LOAD_PROG( blendlight2 );
	LOAD_PROG( fog );
	LOAD_PROG( fog_skinned );
	LOAD_PROG( fog2 );

	LOAD_PROG( skybox );
	LOAD_PROG( wobblesky );
	LOAD_PROG( wobblesky2 );

	LOAD_PROG( postprocess );

	LOAD_PROG( screen );

	LOAD_PROG( tonemap );
	LOAD_PROG( tonemap_brightpass );
	LOAD_PROG( hdr_glare_chromatic );
	LOAD_PROG( tonemap_debug );

	LOAD_PROG( SMAA_edge_detection );
	LOAD_PROG( SMAA_blending_weight_calc );
	LOAD_PROG( SMAA_final );

	LOAD_PROG( AmbientOcclusion_AO );
	LOAD_PROG( AmbientOcclusion_AO_write );
	LOAD_PROG( AmbientOcclusion_blur );
	LOAD_PROG( AmbientOcclusion_blur_write );
	LOAD_PROG( AmbientOcclusion_minify );
	LOAD_PROG( AmbientOcclusion_minify_mip0 );

	LOAD_PROG( DeepGBufferRadiosity_radiosity );
	LOAD_PROG( DeepGBufferRadiosity_blur );
	LOAD_PROG( DeepGBufferRadiosity_blur_write );

	LOAD_PROG( stereoDeGhost );
	LOAD_PROG( stereoWarp );
	LOAD_PROG( stereoInterlace );

//	LOAD_PROG( zcullReconstruct );

	LOAD_PROG( bink );
	LOAD_PROG( bink_gui );

	LOAD_PROG( motionBlurMask );
	LOAD_PROG( motionBlur );

	LOAD_PROG( copyColor );
	LOAD_PROG( copyColorAndPack );

	//LOAD_PROG( autoExposure );

	LOAD_PROG( bloomExtractLuminance );
	LOAD_PROG( bloomExtractAvarage );
	LOAD_PROG( bloomDownscaleBlur );
	LOAD_PROG( bloomUpscaleBlur );

	///////////////////////////////////////////////////////////////////////////

	r_useHalfLambertLighting.ClearModified();
	r_useHDR.ClearModified();
	r_useProgUBO.ClearModified();

	cmdSystem->AddCommand( "reloadProgs", R_ReloadProgs, CMD_FL_RENDERER, "reloads render programs" );

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

/*
	for( int vertexID = 0; vertexID < 14; vertexID++ )
	{
		int b = 1 << vertexID;
		int x = ( 0x287a & b ) != 0;
		int y = ( 0x02af & b ) != 0;
		int z = ( 0x31e3 & b ) != 0;

		common->Printf( "QVert[%i] = X.%i, Y.%i, Z.%i\n", vertexID, x, y, z ); // 0 ... 1
	}
	common->Printf("\n");
	for( int vertexID = 0; vertexID < 14; vertexID++ )
	{
		int r = int( vertexID > 6 );
		int i = ( r == 1 ) ? ( 13 - vertexID ) : vertexID;

		int x = int( i < 3 || i == 4 );
		int y = r ^ int( i > 0 && i < 4 );
		int z = r ^ int( i < 2 || i > 5 );

		common->Printf( "QVert[%i] = X.%i, Y.%i, Z.%i\n", vertexID, x, y, z ); // 0 ... 1
	}

	common->Printf( "\nTriangles\n" );
	idVec4 Pos; idVec2 Tex;
	for( int vertexID = 0; vertexID < 3; vertexID++ )
	{
		float x = -1.0 + float( ( vertexID & 1 ) << 2 );
		float y = -1.0 + float( ( vertexID & 2 ) << 1 );
		Tex.x = ( ( x ) + 1.0 ) * 0.5;
		Tex.y = ( ( y ) + 1.0 ) * 0.5;
		Pos = idVec4( x, y, 0.0, 1.0 );

		common->Printf( "TriVert[%i] = posX.%4.1f, posY.%4.1f\n", vertexID, Pos.x, Pos.y ); // -1 ... 3
		common->Printf( "TriVert[%i] = texX.%4.1f, texY.%4.1f\n", vertexID, Tex.x, Tex.y ); //  0 ... 2
	}
	common->Printf( "\n" );
	for( int vertexID = 0; vertexID < 3; vertexID++ )
	{
		idVec2i t = idVec2i( vertexID, vertexID << 1 ) & 2;
		Tex = idVec2( t.x, t.y );
		//Pos = idVec4( Lerp( idVec2( -1.0, 1.0 ), idVec2( 1.0, -1.0 ), Tex ), 0.0f, 1.0f );

		Pos.x = Lerp( -1.0, 1.0, Tex.x );
		Pos.y = Lerp( 1.0, -1.0, Tex.y );
		Pos.z = 0.0;
		Pos.w = 1.0;

		common->Printf( "TriVert[%i] = posX.%4.1f, posY.%4.1f\n", vertexID, Pos.x, Pos.y ); // -1 ... 3
		common->Printf( "TriVert[%i] = texX.%4.1f, texY.%4.1f\n", vertexID, Tex.x, Tex.y ); //  0 ... 2
	}
*/
#undef LOAD_PROG
}

#if 0
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
		auto prog = const_cast<idDeclRenderProg*>( declManager->RenderProgByIndex( index, false ) );
		assert( prog != NULL );

		prog->Invalidate();
		prog->FreeData();

		//prog->
	}

}
#endif

/*
================================================================================================
idRenderProgManager::Shutdown()
================================================================================================
*/
void idRenderProgManager::Shutdown()
{
	//KillAllShaders();
	//SEA: decl manager will handle this.

	//fileSystem->RemoveDir( "renderprogs\\debug" );
}

/*
================================================================================================
idRenderProgManager::GetRenderParm
================================================================================================
*/
/*const idDeclRenderParm * idRenderProgManager::GetRenderParm( renderParm_t rp ) const
{
	return builtinParms[ rp ];
}*/

/*
================================================================================================
idRenderProgManager::SetRenderParm
================================================================================================
*/
void idRenderProgManager::SetRenderParm( renderParm_t rp, const float* vector )
{
	///SetUniformValue( rp, value );
	const idDeclRenderParm * parm = builtinParms[ rp ];
	assert( parm != NULL );

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
	assert( parm != NULL );

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
RB_LoadShaderTextureMatrix
================================================================================================
*/
void idRenderProgManager::SetupTextureMatrixParms( const float* shaderRegisters, const textureStage_t* texture ) const
{
	auto texS = GetRenderParm( RENDERPARM_TEXTUREMATRIX_S );
	auto texT = GetRenderParm( RENDERPARM_TEXTUREMATRIX_T );
#if USE_INTRINSICS
	_mm_store_ps( texS->GetVecPtr(), _mm_set_ps( 0.0, 0.0, 0.0, 1.0 ) );
	_mm_store_ps( texT->GetVecPtr(), _mm_set_ps( 0.0, 0.0, 1.0, 0.0 ) );
#else
	texS->GetVec4().Set( 1.0f, 0.0f, 0.0f, 0.0f );
	texT->GetVec4().Set( 0.0f, 1.0f, 0.0f, 0.0f );
#endif
	if( texture->hasMatrix )
	{
		ALIGNTYPE16 float matrix[ 16 ];
		idMaterial::GetTexMatrixFromStage( shaderRegisters, texture, matrix );
	#if USE_INTRINSICS
		_mm_store_ps( texS->GetVecPtr(), _mm_set_ps( matrix[ 3 * 4 + 0 ], matrix[ 2 * 4 + 0 ], matrix[ 1 * 4 + 0 ], matrix[ 0 * 4 + 0 ] ) );
		_mm_store_ps( texT->GetVecPtr(), _mm_set_ps( matrix[ 3 * 4 + 1 ], matrix[ 2 * 4 + 1 ], matrix[ 1 * 4 + 1 ], matrix[ 0 * 4 + 1 ] ) );
	#else
		texS->GetVecPtr()[ 0 ] = matrix[ 0 * 4 + 0 ];
		texS->GetVecPtr()[ 1 ] = matrix[ 1 * 4 + 0 ];
		texS->GetVecPtr()[ 2 ] = matrix[ 2 * 4 + 0 ];
		texS->GetVecPtr()[ 3 ] = matrix[ 3 * 4 + 0 ];

		texT->GetVecPtr()[ 0 ] = matrix[ 0 * 4 + 1 ];
		texT->GetVecPtr()[ 1 ] = matrix[ 1 * 4 + 1 ];
		texT->GetVecPtr()[ 2 ] = matrix[ 2 * 4 + 1 ];
		texT->GetVecPtr()[ 3 ] = matrix[ 3 * 4 + 1 ];
	#endif
		RENDERLOG_PRINT( "Setting Texture Matrix\n" );
		RENDERLOG_INDENT();
		RENDERLOG_PRINT( "Texture Matrix S : %4.3f, %4.3f, %4.3f, %4.3f\n", texS->GetVecPtr()[ 0 ], texS->GetVecPtr()[ 1 ], texS->GetVecPtr()[ 2 ], texS->GetVecPtr()[ 3 ] );
		RENDERLOG_PRINT( "Texture Matrix T : %4.3f, %4.3f, %4.3f, %4.3f\n", texT->GetVecPtr()[ 0 ], texT->GetVecPtr()[ 1 ], texT->GetVecPtr()[ 2 ], texT->GetVecPtr()[ 3 ] );
		RENDERLOG_OUTDENT();
	}
}

/*
================================================================================================
idRenderProgManager::FindRenderProgram
================================================================================================
*/
const idDeclRenderProg * idRenderProgManager::FindRenderProgram( const char * progName ) const
{
	auto decl = declManager->FindType( DECL_RENDERPROG, progName, false );
	assert( decl != NULL );

	return decl->Cast<idDeclRenderProg>();
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

