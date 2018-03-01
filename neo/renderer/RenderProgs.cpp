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
	renderProgManager.LoadAllShaders();
}

/*
================================================================================================
idRenderProgManager::Init()
================================================================================================
*/
void idRenderProgManager::Init()
{
	common->Printf( "----- Initializing Render Programs -----\n" );
		
	for( int i = 0; i < MAX_BUILTINS; i++ )
	{
		builtinShaders[i] = -1;
	}
	
	// RB: added checks for GPU skinning
	struct builtinShaders_t
	{
		int			index;
		const char* name;
		const char* nameOutSuffix;
		uint32		shaderFeatures;
		bool		requireGPUSkinningSupport;
	} 
	builtins[] =
	{
		{ BUILTIN_GUI, "gui.vfp", "", 0, false },
		{ BUILTIN_COLOR, "color.vfp", "", 0, false },
		// RB begin
		{ BUILTIN_COLOR_SKINNED, "color", "_skinned", BIT( USE_GPU_SKINNING ), true },
		{ BUILTIN_VERTEX_COLOR, "vertex_color.vfp", "", 0, false },
		{ BUILTIN_AMBIENT_LIGHTING, "ambient_lighting", "", 0, false },
		{ BUILTIN_AMBIENT_LIGHTING_SKINNED, "ambient_lighting", "_skinned", BIT( USE_GPU_SKINNING ), true },
		{ BUILTIN_SMALL_GEOMETRY_BUFFER, "gbuffer", "", 0, false },
		{ BUILTIN_SMALL_GEOMETRY_BUFFER_SKINNED, "gbuffer", "_skinned", BIT( USE_GPU_SKINNING ), true },
//SEA ->
		{ BUILTIN_SMALL_GBUFFER_SML, "gbuffer_sml", "", 0, false },
		{ BUILTIN_SMALL_GBUFFER_SML_SKINNED, "gbuffer_sml", "_skinned", BIT( USE_GPU_SKINNING ), true },
		{ BUILTIN_DEPTH_WORLD, "depth_world", "", 0, false },
//SEA <-
		// RB end
//		{ BUILTIN_SIMPLESHADE, "simpleshade.vfp", 0, false },
		{ BUILTIN_TEXTURED, "texture.vfp", "", 0, false },
		{ BUILTIN_TEXTURE_VERTEXCOLOR, "texture_color.vfp", "", 0, false },
		{ BUILTIN_TEXTURE_VERTEXCOLOR_SRGB, "texture_color.vfp", "", BIT( USE_SRGB ), false },
		{ BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED, "texture_color_skinned.vfp", "", 0, true },
		{ BUILTIN_TEXTURE_TEXGEN_VERTEXCOLOR, "texture_color_texgen.vfp", "",  0, false },
		// RB begin
		{ BUILTIN_INTERACTION, "interaction.vfp", "", 0, false },
		{ BUILTIN_INTERACTION_SKINNED, "interaction", "_skinned", BIT( USE_GPU_SKINNING ), true },
		{ BUILTIN_INTERACTION_AMBIENT, "interactionAmbient.vfp", 0, false },
		{ BUILTIN_INTERACTION_AMBIENT_SKINNED, "interactionAmbient_skinned.vfp", 0, true },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT, "interactionSM", "_spot", 0, false },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT_SKINNED, "interactionSM", "_spot_skinned", BIT( USE_GPU_SKINNING ), true },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_POINT, "interactionSM", "_point", BIT( LIGHT_POINT ), false },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_POINT_SKINNED, "interactionSM", "_point_skinned", BIT( USE_GPU_SKINNING ) | BIT( LIGHT_POINT ), true },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL, "interactionSM", "_parallel", BIT( LIGHT_PARALLEL ), false },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL_SKINNED, "interactionSM", "_parallel_skinned", BIT( USE_GPU_SKINNING ) | BIT( LIGHT_PARALLEL ), true },
		// RB end
		{ BUILTIN_ENVIRONMENT, "environment.vfp", "", 0, false },
		{ BUILTIN_ENVIRONMENT_SKINNED, "environment_skinned.vfp", "",  0, true },
		{ BUILTIN_BUMPY_ENVIRONMENT, "bumpyenvironment.vfp", "", 0, false },
		{ BUILTIN_BUMPY_ENVIRONMENT_SKINNED, "bumpyenvironment_skinned.vfp", "", 0, true },
		
		{ BUILTIN_DEPTH, "depth.vfp", "", 0, false },
		{ BUILTIN_DEPTH_SKINNED, "depth_skinned.vfp", "", 0, true },
		
		{ BUILTIN_SHADOW, "shadow.vfp", "", 0, false },
		{ BUILTIN_SHADOW_SKINNED, "shadow_skinned.vfp", "", 0, true },
		
		{ BUILTIN_SHADOW_DEBUG, "shadowDebug.vfp", "", 0, false },
		{ BUILTIN_SHADOW_DEBUG_SKINNED, "shadowDebug_skinned.vfp", "", 0, true },
		
		{ BUILTIN_BLENDLIGHT, "blendlight.vfp", "", 0, false },
		{ BUILTIN_BLENDLIGHT_SCREENSPACE, "blendlight2", "", 0, false },
		{ BUILTIN_FOG, "fog.vfp", "", 0, false },
		{ BUILTIN_FOG_SKINNED, "fog_skinned.vfp", "", 0, true },
		{ BUILTIN_SKYBOX, "skybox.vfp", "",  0, false },
		{ BUILTIN_WOBBLESKY, "wobblesky.vfp", "", 0, false },
		{ BUILTIN_POSTPROCESS, "postprocess.vfp", "", 0, false },
		// RB begin
		{ BUILTIN_SCREEN, "screen", "", 0, false },
		{ BUILTIN_TONEMAP, "tonemap", "", 0, false },
		{ BUILTIN_BRIGHTPASS, "tonemap", "_brightpass", BIT( BRIGHTPASS ), false },
		{ BUILTIN_HDR_GLARE_CHROMATIC, "hdr_glare_chromatic", "", 0, false },
		{ BUILTIN_HDR_DEBUG, "tonemap", "_debug", BIT( HDR_DEBUG ), false },
		
		{ BUILTIN_SMAA_EDGE_DETECTION, "SMAA_edge_detection", "", 0, false },
		{ BUILTIN_SMAA_BLENDING_WEIGHT_CALCULATION, "SMAA_blending_weight_calc", "", 0, false },
		{ BUILTIN_SMAA_NEIGHBORHOOD_BLENDING, "SMAA_final", "", 0, false },
		
		{ BUILTIN_AMBIENT_OCCLUSION, "AmbientOcclusion_AO", "", 0, false },
		{ BUILTIN_AMBIENT_OCCLUSION_AND_OUTPUT, "AmbientOcclusion_AO", "_write", BIT( BRIGHTPASS ), false },
		{ BUILTIN_AMBIENT_OCCLUSION_BLUR, "AmbientOcclusion_blur", "", 0, false },
		{ BUILTIN_AMBIENT_OCCLUSION_BLUR_AND_OUTPUT, "AmbientOcclusion_blur", "_write", BIT( BRIGHTPASS ), false },
		{ BUILTIN_AMBIENT_OCCLUSION_MINIFY, "AmbientOcclusion_minify", "", 0, false },
		{ BUILTIN_AMBIENT_OCCLUSION_RECONSTRUCT_CSZ, "AmbientOcclusion_minify", "_mip0", BIT( BRIGHTPASS ), false },
		{ BUILTIN_DEEP_GBUFFER_RADIOSITY_SSGI, "DeepGBufferRadiosity_radiosity", "", 0, false },
		{ BUILTIN_DEEP_GBUFFER_RADIOSITY_BLUR, "DeepGBufferRadiosity_blur", "", 0, false },
		{ BUILTIN_DEEP_GBUFFER_RADIOSITY_BLUR_AND_OUTPUT, "DeepGBufferRadiosity_blur", "_write", BIT( BRIGHTPASS ), false },
		// RB end
		{ BUILTIN_STEREO_DEGHOST, "stereoDeGhost.vfp", "", 0, false },
		{ BUILTIN_STEREO_WARP, "stereoWarp.vfp", "", 0, false },
//		{ BUILTIN_ZCULL_RECONSTRUCT, "zcullReconstruct.vfp", 0, false },
		{ BUILTIN_BINK, "bink.vfp", "",  0, false },
		{ BUILTIN_BINK_GUI, "bink_gui.vfp", "", 0, false },
		{ BUILTIN_STEREO_INTERLACE, "stereoInterlace.vfp", "", 0, false },
		{ BUILTIN_MOTION_BLUR, "motionBlur.vfp", "", 0, false },
		
		// RB begin
		{ BUILTIN_DEBUG_SHADOWMAP, "debug_shadowmap.vfp", "", 0, false },
		// RB end
	};
	const int numBuiltins = _countof( builtins );
	vertShaders.SetNum( numBuiltins );
	geomShaders.SetNum( numBuiltins );
	fragShaders.SetNum( numBuiltins );
	glslPrograms.SetNum( numBuiltins );
	
	for( int i = 0; i < numBuiltins; i++ )
	{
		vertShaders[ i ].name = builtins[ i ].name;
		vertShaders[ i ].nameOutSuffix = builtins[ i ].nameOutSuffix;
		vertShaders[ i ].shaderFeatures = builtins[ i ].shaderFeatures;
		vertShaders[ i ].builtin = true;

		geomShaders[ i ].name = builtins[ i ].name;
		geomShaders[ i ].nameOutSuffix = builtins[ i ].nameOutSuffix;
		geomShaders[ i ].shaderFeatures = builtins[ i ].shaderFeatures;
		geomShaders[ i ].builtin = true;

		fragShaders[ i ].name = builtins[ i ].name;
		fragShaders[ i ].nameOutSuffix = builtins[ i ].nameOutSuffix;
		fragShaders[ i ].shaderFeatures = builtins[ i ].shaderFeatures;
		fragShaders[ i ].builtin = true;
		
		builtinShaders[builtins[i].index] = i;
		
		if( builtins[i].requireGPUSkinningSupport && !glConfig.gpuSkinningAvailable )
		{
			// RB: don't try to load shaders that would break the GLSL compiler in the OpenGL driver
			continue;
		}
		
		LoadShader( i, SHT_VERTEX );
		LoadShader( i, SHT_GEOMETRY );
		LoadShader( i, SHT_FRAGMENT );
		LoadGLSLProgram( i, i, i, i );
	}

	r_useHalfLambertLighting.ClearModified();
	r_useHDR.ClearModified();

	glslUniforms.SetNum( RENDERPARM_USER + MAX_GLSL_USER_PARMS, vec4_zero );
	
	if( glConfig.gpuSkinningAvailable )
	{
		vertShaders[ builtinShaders[ BUILTIN_COLOR_SKINNED ] ].usesJoints = true;
		vertShaders[ builtinShaders[ BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED ] ].usesJoints = true;
		vertShaders[ builtinShaders[ BUILTIN_INTERACTION_SKINNED ] ].usesJoints = true;
		vertShaders[ builtinShaders[ BUILTIN_INTERACTION_AMBIENT_SKINNED ] ].usesJoints = true;
		vertShaders[ builtinShaders[ BUILTIN_ENVIRONMENT_SKINNED ] ].usesJoints = true;
		vertShaders[ builtinShaders[ BUILTIN_BUMPY_ENVIRONMENT_SKINNED ] ].usesJoints = true;
		vertShaders[ builtinShaders[ BUILTIN_DEPTH_SKINNED ] ].usesJoints = true;
		vertShaders[ builtinShaders[ BUILTIN_SHADOW_SKINNED ] ].usesJoints = true;
		vertShaders[ builtinShaders[ BUILTIN_SHADOW_DEBUG_SKINNED ] ].usesJoints = true;
		vertShaders[ builtinShaders[ BUILTIN_FOG_SKINNED ] ].usesJoints = true;
		// RB begin
		vertShaders[ builtinShaders[ BUILTIN_AMBIENT_LIGHTING_SKINNED ] ].usesJoints = true;
		vertShaders[ builtinShaders[ BUILTIN_SMALL_GEOMETRY_BUFFER_SKINNED ] ].usesJoints = true;
		vertShaders[ builtinShaders[ BUILTIN_SMALL_GBUFFER_SML_SKINNED ] ].usesJoints = true;

		vertShaders[ builtinShaders[ BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT_SKINNED ] ].usesJoints = true;
		vertShaders[ builtinShaders[ BUILTIN_INTERACTION_SHADOW_MAPPING_POINT_SKINNED ] ].usesJoints = true;
		vertShaders[ builtinShaders[ BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL_SKINNED ] ].usesJoints = true;
		// RB end
	}
	
	cmdSystem->AddCommand( "reloadShaders", R_ReloadShaders, CMD_FL_RENDERER, "reloads shaders" );

	/////////////////////////////////////////////////////////////////////////////////////////////////
#if 1

	for( int index = 0; index < declManager->GetNumDecls( DECL_RENDERPARM ); ++index )
	{
		auto decl = declManager->DeclByIndex( DECL_RENDERPARM, index, true )->Cast<idDeclRenderParm>();
		decl->Print();
	}

	extern idCVar r_useProgUBO;

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

	for( int index = 0; index < declManager->GetNumDecls( DECL_RENDERPROG ); ++index )
	{
		auto decl = declManager->DeclByIndex( DECL_RENDERPROG, index, true )->Cast<idDeclRenderProg>();
		decl->Print();
	}
	//TestParseProg();

	//idParser::RemoveAllGlobalDefines();
#endif
}

/*
================================================================================================
idRenderProgManager::LoadAllShaders()
================================================================================================
*/
void idRenderProgManager::LoadAllShaders() 
{
#if 1
	for( int i = 0; i < vertShaders.Num(); ++i )
	{
		LoadShader( i, SHT_VERTEX );
	}
	for( int i = 0; i < geomShaders.Num(); ++i )
	{
		LoadShader( i, SHT_GEOMETRY );
	}
	for( int i = 0; i < fragShaders.Num(); ++i )
	{
		LoadShader( i, SHT_FRAGMENT );
	}
	
	for( int i = 0; i < glslPrograms.Num(); ++i )
	{
		if( glslPrograms[i].vertShaderIndex == -1 || glslPrograms[i].fragShaderIndex == -1 ) {
			// RB: skip reloading because we didn't load it initially
			continue;
		}	
		LoadGLSLProgram( i, glslPrograms[i].vertShaderIndex, glslPrograms[ i ].geomShaderIndex, glslPrograms[i].fragShaderIndex );
	}
#else

	for( int sht = 0; sht < SHT_MAX; sht++ )
	{
		for( int i = 0; i < shaders[ sht ].Num(); ++i )
		{
			LoadShader( i, sht );
		}
	}

#endif
}

/*
================================================================================================
idRenderProgManager::KillAllShaders()
================================================================================================
*/
void idRenderProgManager::KillAllShaders()
{
	Unbind();
#if 1
	for( int i = 0; i < glslPrograms.Num(); ++i )
	{
		if( glslPrograms[ i ].progId != INVALID_PROGID ) {
			glDeleteProgram( glslPrograms[ i ].progId );
			glslPrograms[ i ].progId = INVALID_PROGID;
		}
	}

	for( int i = 0; i < vertShaders.Num(); ++i )
	{
		if( vertShaders[i].progId != INVALID_PROGID ) {
			glDeleteShader( vertShaders[i].progId );
			vertShaders[i].progId = INVALID_PROGID;
		}
	}
	for( int i = 0; i < geomShaders.Num(); ++i )
	{
		if( geomShaders[ i ].progId != INVALID_PROGID ) {
			glDeleteShader( geomShaders[ i ].progId );
			geomShaders[ i ].progId = INVALID_PROGID;
		}
	}
	for( int i = 0; i < fragShaders.Num(); ++i )
	{
		if( fragShaders[i].progId != INVALID_PROGID ) {
			glDeleteShader( fragShaders[i].progId );
			fragShaders[i].progId = INVALID_PROGID;
		}
	}
#else

	for( int sht = 0; sht < SHT_MAX; sht++ )
	{
		for( int i = 0; i < shaders[ sht ].Num(); ++i )
		{
			if( shaders[ sht ][ i ].objectID != INVALID_PROGID ) {
				glDeleteShader( shaders[ sht ][ i ].objectID );
				shaders[ sht ][ i ].objectID = INVALID_PROGID;
			}
		}
	}

#endif
}

/*
================================================================================================
idRenderProgManager::Shutdown()
================================================================================================
*/
void idRenderProgManager::Shutdown()
{
	KillAllShaders();

	/*for( int index = 0; index < declManager->GetNumDecls( DECL_RENDERPROG ); ++index ) 
	{
		auto prog = declManager->DeclByIndex( DECL_RENDERPROG, index, false )->Cast<idDeclRenderProg>();
		const_cast<idDeclRenderProg*>( prog )->FreeData();
	}*/
}

/*
================================================================================================
idRenderProgManager::FindShader
================================================================================================
*/

int idRenderProgManager::FindVertexShader( const char* name )
{
	for( int i = 0; i < vertShaders.Num(); ++i )
	{
		if( vertShaders[i].name.Icmp( name ) == 0 )
		{
			LoadShader( i, SHT_VERTEX );
			return i;
		}
	}
	vertShader_t shader;
	shader.name = name;
	int index = vertShaders.Append( shader );
	LoadShader( index, SHT_VERTEX );
	currentVertShader = index;
	
	// RB: removed idStr::Icmp( name, "heatHaze.vfp" ) == 0  hack
	// this requires r_useUniformArrays 1
	for( int i = 0; i < vertShaders[ index ].uniforms.Num(); i++ )
	{
		if( vertShaders[ index ].uniforms[ i ] == RENDERPARM_ENABLE_SKINNING )
		{
			vertShaders[ index ].usesJoints = true;
			vertShaders[ index ].optionalSkinning = true;
		}
	}
	// RB end
	
	return index;
}
int idRenderProgManager::FindGeometryShader( const char* name )
{
	for( int i = 0; i < geomShaders.Num(); ++i )
	{
		if( geomShaders[ i ].name.Icmp( name ) == 0 )
		{
			LoadShader( i, SHT_GEOMETRY );
			return i;
		}
	}
	geomShader_t shader;
	shader.name = name;
	int index = geomShaders.Append( shader );
	LoadShader( index, SHT_GEOMETRY );
	currentGeomShader = index;
	return index;
}
int idRenderProgManager::FindFragmentShader( const char* name )
{
	for( int i = 0; i < fragShaders.Num(); ++i )
	{
		if( fragShaders[i].name.Icmp( name ) == 0 )
		{
			LoadShader( i, SHT_FRAGMENT );
			return i;
		}
	}
	fragShader_t shader;
	shader.name = name;
	int index = fragShaders.Append( shader );
	LoadShader( index, SHT_FRAGMENT );
	currentFragShader = index;
	return index;
}

int idRenderProgManager::FindShader( const char* name, shaderType_e shaderType )
{
#if 1
	/*/*/if( shaderType == shaderType_e::SHT_VERTEX )
	{
		return FindVertexShader( name );
	}
	else if( shaderType == shaderType_e::SHT_GEOMETRY )
	{
		return FindGeometryShader( name );
	}
	else if( shaderType == shaderType_e::SHT_FRAGMENT )
	{
		return FindFragmentShader( name );
	}

#else

	for( int i = 0; i < shaders[ shaderType ].Num(); ++i )
	{
		if( shaders[ shaderType ][ i ].name.Icmp( name ) == 0 )
		{
			LoadShader( i, shaderType );
			return i;
		}
	}

	glslShader_t shader;
	shader.name = name;
	int index = shaders[ shaderType ].Append( shader );
	LoadShader( index, shaderType );
	currentShaders[ shaderType ] = index;

	return index;

#endif

	return 0;
}

/*
================================================================================================
idRenderProgManager::LoadShader
================================================================================================
*/
void idRenderProgManager::LoadShader( int index, shaderType_e shaderType )
{
#if 1
	/*/*/if( shaderType == shaderType_e::SHT_VERTEX )/////////////////////////////////////////////////////////////
	{
		if( vertShaders[ index ].progId != INVALID_PROGID ) {
			return; // Already loaded
		}
		vertShader_t& vs = vertShaders[ index ];
		vs.progId = LoadGLSLShader( shaderType_e::SHT_VERTEX, vs.name, vs.nameOutSuffix, vs.shaderFeatures, vs.builtin, vs.uniforms );
		return;
	} 
	else if( shaderType == shaderType_e::SHT_TESS_CTRL )//////////////////////////////////////////////////////////
	{
		release_assert( "Loading SHT_TESS_CTRL Not Implemented!" );
		return;
	}
	else if( shaderType == shaderType_e::SHT_TESS_EVAL )
	{
		release_assert( "Loading SHT_TESS_EVAL Not Implemented!" );
		return;
	}
	else if( shaderType == shaderType_e::SHT_GEOMETRY )///////////////////////////////////////////////////////////
	{
		if( geomShaders[ index ].progId != INVALID_PROGID ) {
			return; // Already loaded
		}
		geomShader_t& gs = geomShaders[ index ];
		gs.progId = LoadGLSLShader( shaderType_e::SHT_GEOMETRY, gs.name, gs.nameOutSuffix, gs.shaderFeatures, gs.builtin, gs.uniforms );
		return;
	}
	else if( shaderType == shaderType_e::SHT_FRAGMENT )///////////////////////////////////////////////////////////
	{
		if( fragShaders[ index ].progId != INVALID_PROGID ) {
			return; // Already loaded
		}
		fragShader_t& fs = fragShaders[ index ];
		fs.progId = LoadGLSLShader( shaderType_e::SHT_FRAGMENT, fs.name, fs.nameOutSuffix, fs.shaderFeatures, fs.builtin, fs.uniforms );
		return;
	}
	else if( shaderType == shaderType_e::SHT_COMPUTE )////////////////////////////////////////////////////////////
	{
		release_assert( "Loading SHT_COMPUTE Not Implemented!" );
		return;
	}

	release_assert("Unknown ShaderType Loading");
#else

	if( shaders[ shaderType ][ index ].objectID != INVALID_PROGID ) {
		return; // Already loaded
	}

	auto & sho = shaders[ shaderType ][ index ];
	sho.objectID = LoadGLSLShader( shaderType, sho.name, sho.nameOutSuffix, sho.flags, sho.builtin, sho.uniforms );

#endif
}

/*
================================================================================================
idRenderProgManager::BindShader
================================================================================================
*/
// RB begin
void idRenderProgManager::BindShader( int progIndex, int vIndex, int gIndex, int fIndex, bool builtin )
{
	if( currentVertShader == vIndex && currentGeomShader == gIndex && currentFragShader == fIndex ) {
		return;
	}
	
	if( builtin )
	{
		currentVertShader = vIndex;
		currentGeomShader = gIndex;
		currentFragShader = fIndex;
		
		// vIndex denotes the GLSL program
		if( vIndex >= 0 && vIndex < glslPrograms.Num() )
		{
			currentRenderProgram = vIndex;
			RENDERLOG_PRINT( "Binding GLSL Program %s %s\n", glslPrograms[vIndex].name.c_str(), ShaderUsesJoints() ? "skinned" : "" );
			glUseProgram( glslPrograms[vIndex].progId );
		}
	}
	else
	{
		if( progIndex == -1 )
		{
			// RB: FIXME linear search
			for( int i = 0; i < glslPrograms.Num(); ++i )
			{
				if( ( glslPrograms[i].vertShaderIndex == vIndex ) && 
					( glslPrograms[i].geomShaderIndex == fIndex ) && 
					( glslPrograms[i].fragShaderIndex == fIndex ) )
				{
					progIndex = i;
					break;
				}
			}
		}
		
		currentVertShader = vIndex;
		currentGeomShader = gIndex;
		currentFragShader = fIndex;
		
		if( progIndex >= 0 && progIndex < glslPrograms.Num() )
		{
			currentRenderProgram = progIndex;
			RENDERLOG_PRINT( "Binding GLSL Program %s %s\n", glslPrograms[progIndex].name.c_str(), ShaderUsesJoints() ? "skinned" : "" );
			glUseProgram( glslPrograms[progIndex].progId );
		}
	}
}
// RB end

/*
================================================================================================
idRenderProgManager::Unbind
================================================================================================
*/
void idRenderProgManager::Unbind()
{
	currentVertShader = -1;
	currentGeomShader = -1;
	currentFragShader = -1;

	for( int i = 0; i < m_currentShaders.Num(); i++ ) {
		m_currentShaders[ i ] = -1;
	}
	currentRenderProgram = -1;
	glUseProgram( GL_NONE );
}

// RB begin
bool idRenderProgManager::IsShaderBound() const
{
	//return( currentShaders[ SHT_VERTEX ] != -1 );
	return( currentVertShader != -1 );
}
// RB end

/*
================================================================================================
idRenderProgManager::SetRenderParms
================================================================================================
*/
void idRenderProgManager::SetRenderParms( renderParm_t rp, const float* value, int num )
{
	for( int i = 0; i < num; i++ )
	{
		SetRenderParm( ( renderParm_t )( rp + i ), value + ( i * 4 ) );
	}
}

/*
================================================================================================
idRenderProgManager::SetRenderParm
================================================================================================
*/
void idRenderProgManager::SetRenderParm( renderParm_t rp, const float* value )
{
	SetUniformValue( rp, value );
}

/*
================================================================================================
idRenderProgManager::GetRenderParm
================================================================================================
*/
idRenderVector & idRenderProgManager::GetRenderParm( renderParm_t rp )
{
	return glslUniforms[ rp ];
}

