/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

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

#include "OpenGL\gl_header.h"

#include "tr_local.h"

#include "DeclRenderProg.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
// DeclRenderProgGenerator.cpp

/*
	// GL_ARB_shading_language_include
	// #include </foo/bar.glsl>
	struct glslInclude_t {
	const char * name;		// defines the name associated with the string.
	GLint nameLength;		// <namelen> is the number of <char>s in <name>.
	GLint stringLength;		// the number of <char>s in <string>.
	const char * string;
	};
	// If <namelen> or <stringlen> are negative, then <name> or <string> respectively are considered to be null - terminated strings
	glNamedStringARB( GL_SHADER_INCLUDE_ARB, int namelen, const char *name, int stringlen, const char *string );
	glDeleteNamedStringARB( int namelen, const char *name );
	glCompileShaderIncludeARB( uint shader, sizei count, const char *const *path, const int *length );
	//boolean IsNamedStringARB( int namelen, const char *name );
	//void GetNamedStringARB( int namelen, const char *name, sizei bufSize, int *stringlen, char *string );
	//void GetNamedStringivARB( int namelen, const char *name, enum pname, int *params );
*/

idCVar r_skipStripDeadProgCode( "r_skipStripDeadProgCode", "0", CVAR_BOOL, "Skip stripping dead code" );
idCVar r_useProgUBO( "r_useProgUBO", "0", CVAR_ARCHIVE | CVAR_BOOL, "use uniform buffer for sending parms to programs" );

class idProgException {
public:
	static const int MAX_ERROR_LEN = 128;

	idProgException( const char* text = "" ) {
		strncpy( error, text, MAX_ERROR_LEN );
	}
	const char * 	GetError() const { return error; }
private:
	static char error[ MAX_ERROR_LEN ];
};
char idProgException::error[ idProgException::MAX_ERROR_LEN ];

GLuint GL_CreateGLSLShaderObject( shaderType_e, const char * sourceGLSL, const char * fileName );
void GL_CreateGLSLProgram( struct progParseData_t &, const char * progName, bool bRetrieveBinary );

typedef idStringBuilder< idStr, idCharAllocator_Heap< 2048, char > > idShaderStringBuilder;

#define _Txt idStrStatic<128>().Format
#define CHECK_NEW_LINE_AND_SPACES() ( token.linesCrossed > 0 )? newline : (( token.WhiteSpaceBeforeToken() > 0 )? " " : "" )

#define VERTEX_UNIFORM_ARRAY_NAME			"_va"
#define TESS_CONTROL_UNIFORM_ARRAY_NAME		"_tca"
#define TESS_EVALUATION_UNIFORM_ARRAY_NAME	"_tea"
#define GEOMETRY_UNIFORM_ARRAY_NAME			"_ga"
#define FRAGMENT_UNIFORM_ARRAY_NAME			"_fa"
#define COMPUTE_UNIFORM_ARRAY_NAME			"_ca"
#define COMMON_PARMBLOCK_NAME				"_rp"

#define PROG_INCLUDE_PATH "renderprogs\\include"

struct attribInfo_t {
	const char * 	type;
	const char * 	name;
	const char * 	glsl;
	uint32			bind;

	bool operator == ( const attribInfo_t & other ) const {
		return( /* type == other.type &&*/ name == other.name );
	}
};

struct glslInOut_t {
	const char *	nameCG;
	const char *	prefGLSL;
	const char *	nameGLSL;
	const char *	typeHLSL;
	const char *	symHLSL;
};

struct glslShaderInfo_t {
	GLenum			glTargetType;
	const char *	shaderTypeDeclName;
	const char *	shaderTypeName;
	const char *	uniformArrayName;
}
_glslShaderInfo[ SHT_MAX + 1 ] =
{
	{ GL_VERTEX_SHADER, "vertexShader", ".vertex", VERTEX_UNIFORM_ARRAY_NAME },
	{ GL_TESS_CONTROL_SHADER, "tessCtrlShader", ".tess_ctrl", TESS_CONTROL_UNIFORM_ARRAY_NAME },
	{ GL_TESS_EVALUATION_SHADER, "tessEvalShader", ".tess_eval", TESS_EVALUATION_UNIFORM_ARRAY_NAME },
	{ GL_GEOMETRY_SHADER, "geometryShader", ".geometry", GEOMETRY_UNIFORM_ARRAY_NAME },
	{ GL_FRAGMENT_SHADER, "fragmentShader", ".fragment", FRAGMENT_UNIFORM_ARRAY_NAME },
	{ GL_COMPUTE_SHADER, "computeShader", ".compute", COMPUTE_UNIFORM_ARRAY_NAME },
};

struct varInOut_t {
	//idStrStatic<32>	type;
	idStrStatic<48>	name;
	//idStrStatic<48>	nameGLSL;
	uint16  typeIndex = 5U;/*vec4*/	//-> _typeConversion[ typeIndex ]

	bool operator == ( const varInOut_t & other ) const {
		return( /* type == other.type &&*/ name == other.name );
	}
};
//typedef idStaticList< varInOut_t, 32 > inOutVarList_t;

#define MAX_STAGE_VEC 128
#define MAX_STAGE_TEX 16

struct progParseData_t
{
	idArray< idStr, SHT_MAX > parseTexts;

	glslProgram_t glslProgram;

	//renderProgData_t data;
	idStaticList< idStrStatic<86>, 8 > dependencies;
	ID_TIME_T dependenciesTimestamp;
	idBitFlags<uint64>	glState;
	idBitFlags<uint32>	flags;
	vertexMask_t	vertexMask;

	// custom variables
	idStaticList< varInOut_t, 64*SHT_MAX > variables;
	idStaticList< int8, 32 > varIn[ SHT_MAX ];
	idStaticList< int8, 32 > varOut[ SHT_MAX ];

	idStaticList< attribInfo_t, 16 > vsInputs;
	idStaticList< attribInfo_t, 8 > fsOutputs;

	idStaticList< uint16, MAX_STAGE_VEC*SHT_MAX> vecParms;
	idStaticList< uint16, MAX_STAGE_TEX*SHT_MAX> texParms;
	idArray< idStaticList< int16, MAX_STAGE_VEC>, SHT_MAX> vecParmsUsed;
	idArray< idStaticList< int16, MAX_STAGE_TEX>, SHT_MAX> texParmsUsed;

	struct gsLayout_t {
		uint8 prim_in_type = 0;
		uint8 prim_out_type = 0;
		uint8 count = 0;
		uint8 invoc = 0;
	} primLayout;

	struct tessLayout_t {
		uint8 prim_type = 0;
		uint8 prim_spacing = 0;
		uint8 prim_topology = 0;
		uint8 count = 0;
	} tessLayout;

	// ----------------------------------------------------------------

	void BeginShader( shaderType_e sht, shaderType_e prev_sht )
	{
	#if 0
		varIn.Clear();
		// Copy previous outputs to current inputs.
		for( int i = 0; i < varOut.Num(); i++ ) {
			varIn.Append( varOut[ i ] );
			// we dont care if it is referenced or not, we need interface matching!
		}
		varOut.Clear();
	#else
		if( sht != shaderType_e::SHT_VERTEX )
		{
			for( int i = 0; i < varOut[ prev_sht ].Num(); i++ )
			{
				varIn[ sht ].Append( varOut[ prev_sht ][ i ] );
			}
		}
	#endif
	}

	int AddRenderParm( const idDeclRenderParm * parm, shaderType_e shaderType )
	{
		if( parm->IsVector() )
		{
			int index = vecParms.AddUnique( parm->Index() );
			vecParmsUsed[ shaderType ].AddUnique( index );
			return index;
		}
		else if( parm->IsTexture() )
		{
			int index = texParms.AddUnique( parm->Index() );
			texParmsUsed[ shaderType ].AddUnique( index );
			return index;
		}

		assert("AddRenderParm() Unknown Parm Type");
		return 0;
	}

	const idDeclRenderParm * GetVecParm( int parmIndex ) const
	{
		return declManager->RenderParmByIndex( vecParms[ parmIndex ], false );
	}
	const idDeclRenderParm * GetTexParm( int parmIndex ) const
	{
		return declManager->RenderParmByIndex( texParms[ parmIndex ], false );
	}
	int GetVecParmCount() const { return vecParms.Num(); }
	int GetTexParmCount() const { return texParms.Num(); }

	void Clear()
	{
		//data.Clear();
		/*for( int i = 0; i < dependencies.Num(); i++ ) {
			dependencies[ i ].Clear();
		} */
		dependencies.Clear();
		dependenciesTimestamp = 0;
		glState.Clear();
		flags.Clear();
		vertexMask.Clear();

		for( int i = 0; i < SHT_MAX; i++ )
		{
			parseTexts[ i ].Clear();

			vecParmsUsed[ i ].Clear();
			texParmsUsed[ i ].Clear();

			varIn[ i ].Clear();
			varOut[ i ].Clear();
		}
		vecParms.Clear();
		texParms.Clear();

		variables.Clear();
		//varIn.Clear();
		//varOut.Clear();

		vsInputs.Clear();
		fsOutputs.Clear();
	}

	progParseData_t()
	{
		Clear();
	}
	~progParseData_t()
	{
		Clear();
	}
};
static const size_t progParseDataSizeKB = SIZE_KB( sizeof( progParseData_t ) );

/*
================================================
	attribInfo_t
================================================
*/

const attribInfo_t _vertexAttribsPC[] = // Vertex input attributes
{
	{ "float4",		"position",		"in_Position",	PC_ATTRIB_INDEX_POSITION },
	{ "float2",		"texcoord",		"in_TexCoord",	PC_ATTRIB_INDEX_ST },
	{ "float4",		"normal",		"in_Normal",	PC_ATTRIB_INDEX_NORMAL },
	{ "float4",		"tangent",		"in_Tangent",	PC_ATTRIB_INDEX_TANGENT },
	{ "float4",		"color",		"in_Color",		PC_ATTRIB_INDEX_COLOR },
	{ "float4",		"color2",		"in_Color2",	PC_ATTRIB_INDEX_COLOR2 },
};
const attribInfo_t _fragmentAttribsPC[] = // Fragment output attributes
{
	//{ "uint",		"stencilRef",	"gl_FragStencilRef",	0 },
	//{ "float",		"depth",		"gl_FragDepth",			0 },

	{ "float4",		"color",		"out_FragColor",		0 },	// SV_Target
	{ "half4",		"hcolor",		"out_FragColor",		0 },	// SV_Target

	{ "float4",		"target0",		"out_FragData0",		0 },	// SV_Target0
	{ "float4",		"target1",		"out_FragData1",		1 },	// SV_Target1
	{ "float4",		"target2",		"out_FragData2",		2 },	// SV_Target2
	{ "float4",		"target3",		"out_FragData3",		3 },	// SV_Target3
	{ "float4",		"target4",		"out_FragData4",		4 },	// SV_Target4
	{ "float4",		"target5",		"out_FragData5",		5 },	// SV_Target5
	{ "float4",		"target6",		"out_FragData6",		6 },	// SV_Target6
	{ "float4",		"target7",		"out_FragData7",		7 },	// SV_Target7

	{ "half4",		"htarget0",		"out_FragData0",		0 },	// SV_Target0
	{ "half4",		"htarget1",		"out_FragData1",		1 },	// SV_Target1
	{ "half4",		"htarget2",		"out_FragData2",		2 },	// SV_Target2
	{ "half4",		"htarget3",		"out_FragData3",		3 },	// SV_Target3
	{ "half4",		"htarget4",		"out_FragData4",		4 },	// SV_Target4
	{ "half4",		"htarget5",		"out_FragData5",		5 },	// SV_Target5
	{ "half4",		"htarget6",		"out_FragData6",		6 },	// SV_Target6
	{ "half4",		"htarget7",		"out_FragData7",		7 },	// SV_Target7

	//{ NULL, NULL, NULL, 0 }
};

const char *_types[] = {
	"int",
	"uint",
	"float",
	"double",
	"matrix",
	"half",
	"fixed",
	"bool",
	"cint",
	"cfloat",
	"void"
};
const char *_typePosts[] = {
	"1",   "2",   "3",   "4",
	"1x1", "1x2", "1x3", "1x4",
	"2x1", "2x2", "2x3", "2x4",
	"3x1", "3x2", "3x3", "3x4",
	"4x1", "4x2", "4x3", "4x4"
};
const char *_prefixes[] = {
	"static",
	"const",
	"uniform",
	"struct",

	"sampler",

	"sampler1D",
	"sampler2D",
	"sampler3D",
	"samplerCUBE",

	"sampler1DShadow",		// GLSL
	"sampler2DShadow",		// GLSL
	"sampler2DArrayShadow",	// GLSL
	"sampler3DShadow",		// GLSL
	"samplerCubeShadow",	// GLSL
	"samplerCubeArrayShadow",	// GLSL

	"sampler2DArray",		// GLSL"
	"samplerCubeArray",

	"sampler2DMS",			// GLSL
	"sampler2DMSArray",		// GLSL
};

const struct typeConversion_t {
	const char* typeCG;
	const char* typeGLSL;
}
_typeConversion[] =
{
	{ "void",			"void" },

	{ "fixed",			"float" },

	{ "float",			"float" },
	{ "float2",			"vec2" },
	{ "float3",			"vec3" },
	{ "float4",			"vec4" },

	{ "half",			"float" }, // mediump
	{ "half2",			"vec2" },
	{ "half3",			"vec3" },
	{ "half4",			"vec4" },

	{ "int",			"int" },
	{ "int2",			"ivec2" },
	{ "int3",			"ivec3" },
	{ "int4",			"ivec4" },

	{ "uint",			"uint" },
	{ "uint2",			"uvec2" },
	{ "uint3",			"uvec3" },
	{ "uint4",			"uvec4" },

	{ "bool",			"bool" },
	{ "bool2",			"bvec2" },
	{ "bool3",			"bvec3" },
	{ "bool4",			"bvec4" },

	{ "float2x2",		"mat2x2" },
	{ "float2x3",		"mat2x3" },
	{ "float2x4",		"mat2x4" },

	{ "float3x2",		"mat3x2" },
	{ "float3x3",		"mat3x3" },
	{ "float3x4",		"mat3x4" },

	{ "float4x2",		"mat4x2" },
	{ "float4x3",		"mat4x3" },
	{ "float4x4",		"mat4x4" },

	{ "matrix",			"mat4x4" },

	{ "floatFlat",		"flat float" },
	{ "float2Flat",		"flat vec2" },
	{ "float3Flat",		"flat vec3" },
	{ "float4Flat",		"flat vec4" },

	{ "intFlat",		"flat int" },
	{ "int2Flat",		"flat ivec2" },
	{ "int3Flat",		"flat ivec3" },
	{ "int4Flat",		"flat ivec4" },

	{ "samplerCUBE",		"samplerCube" },

	{ "sampler2DARRAY",		"sampler2DArray" },
	{ "samplerCUBEARRAY",	"samplerCubeArray" },

	{ "sampler2DMS",		"sampler2DMS" },
	{ "sampler2DMSARRAY",	"sampler2DMSArray" },

	{ NULL, NULL }
};
static bool ConvertTypes( idStr & out, idToken & token, char newline[] )
{
	for( int i = 0; _typeConversion[ i ].typeCG != NULL; i++ )
	{
		if( token.Cmp( _typeConversion[ i ].typeCG ) == 0 )
		{
			out += CHECK_NEW_LINE_AND_SPACES();
			out += _typeConversion[ i ].typeGLSL;
			return true;
		}
	}
	return false;
}

const char* GetHLSLTextureType( const idDeclRenderParm * decl )
{
	assert( decl != nullptr );
	auto img = decl->GetDefaultImage();
	assert( img != nullptr );

	if( img->GetOpts().textureType == TT_2D )
	{
		if( img->GetOpts().IsMultisampled() )
		{
			return img->GetOpts().IsArray() ? "Texture2DMSArray" : "Texture2DMS";
		}
		else {
			return img->GetOpts().IsArray() ? "Texture2DArray" : "Texture2D";
		}
	}
	else if( img->GetOpts().textureType == TT_CUBIC )
	{
		return img->GetOpts().IsArray() ? "TextureCubeArray" : "TextureCube";
	}
	else if( img->GetOpts().textureType == TT_3D )
	{
		return "Texture3D";
	}

	return "TextureType unknown";
	// return "Buffer"; // texture buffer

	// Object1 [<Type>] Name;
	// Texture2DMS/Texture2DMSArray [<Type, Samples>] Name;
	// Type - Optional. Any scalar HLSL type or vector HLSL type, surrounded by angle brackets.The default type is float4.
	// Samples - The number of samples( ranges between 1 and 128 ).
	// TextureCubeArray is available in shader model 4.1 or higher.
	// Shader model 4.1 is available in Direct3D 10.1 or higher.
}

static const char * GetGLSLSamplerType( const idDeclRenderParm * decl )
{
	assert( decl != nullptr );
	auto img = decl->GetDefaultImage();
	assert( img != nullptr );
	const bool bIsShadowSampler = img->UsageShadowMap() && ( decl->GetDefaultTextureUsage() == TD_SHADOWMAP );
	//SEA: fix sampler permutations issue

	if( img->GetOpts().textureType == TT_2D )
	{
		if( img->GetOpts().IsMultisampled() )
		{
			if( img->GetOpts().IsArray() )
			{
				return "sampler2DMSArray";
			}
			else {
				return "sampler2DMS";
			}
		}
		else {
			if( img->GetOpts().IsArray() )
			{
				return !bIsShadowSampler ? "sampler2DArray" : "sampler2DArrayShadow";
			}
			else {
				return !bIsShadowSampler ? "sampler2D" : "sampler2DShadow";
			}
		}
	}
	else if( img->GetOpts().textureType == TT_CUBIC )
	{
		if( img->GetOpts().IsArray() )
		{
			return !bIsShadowSampler ? "samplerCubeArray" : "samplerCubeArrayShadow";
		}
		else {
			return !bIsShadowSampler ? "samplerCube" : "samplerCubeShadow";
		}
	}
	else if( img->GetOpts().textureType == TT_3D )
	{
		return !bIsShadowSampler ? "sampler3D" : "sampler3DShadow";
	}

	return "sampler unknown";
}

struct primLayout_t {
	const char * name;
	const char * glslName;
	const char * hlslName;
	uint32		 count;
};
const primLayout_t tessSpacing[] = {
	{ "EQUAL",		"equal_spacing",			"integer",			0 },
	{ "FRAC_EVEN",	"fractional_even_spacing",	"fractional_even",	0 },
	{ "FRAC_ODD",	"fractional_odd_spacing",	"fractional_odd",	0 },
};
const primLayout_t tessTopology[] = {
	{ "CCW",		"ccw",					"triangle_ccw",	0 },
	{ "CW",			"cw",					"triangle_cw",	0 },
};
const primLayout_t tessPrimType[] = {
	{ "TRIANGLES",	"triangles",			"tri",			3 },
	{ "QUADS",		"quads",				"quad",			4 },
	{ "ISOLINE",	"isolines",				"isoline",		2 },
};
const primLayout_t gs_primIn[] = {
	{ "TRIANGLES",	"triangles",			"triangle",		3 },
	{ "POINTS",		"points",				"point",		1 },
	{ "LINES",		"lines",				"line",			2 },
	{ "TRI_ADJ",	"triangles_adjacency",	"triangleadj",	6 },
	{ "LINE_ADJ",	"lines_adjacency",		"lineadj",		4 },
};
const primLayout_t gs_primOut[] = {
	{ "TRIANGLES",	"triangle_strip",	"TriangleStream",	3 },
	{ "POINTS",		"points",			"PointStream",		1 },
	{ "LINES",		"line_strip",		"LineStream",		2 },
};
static bool ParsePrimLayout( idLexer & src, idToken & token, progParseData_t & parseData, shaderType_e shaderType, bool inMain )
{
	// PRIM_IN( TRIANGLES, 6 );
	// PRIM_OUT( TRIANGLES, 3 );
	// ??? PRIM_INOUT( TRIANGLES, TRIANGLES, 6 ); - type, type, primOutCount

	if( shaderType == shaderType_e::SHT_GEOMETRY && !inMain )
	{
		if( token.is( "PRIM_INOUT" ) )
		{
			if( src.ExpectTokenString( "(" ) )
			{
				if( src.ExpectTokenType( TT_NAME, 0, token ) )
				{
					bool found = false;
					for( uint32 i = 0; i < _countof( gs_primIn ); i++ )
					{
						if( token.is( gs_primIn[ i ].name ) )
						{
							parseData.primLayout.prim_in_type = ( uint8 )i;
							found = true;
							break;
						}
					}
					if( !found )
					{
						src.Error( "Bad gs prim_in type ( %s )", token.c_str() );
						return false;
					}

					if( src.CheckTokenString( "," ) )
					{
						if( src.ExpectTokenType( TT_NAME, 0, token ) )
						{
							bool found = false;
							for( uint32 i = 0; i < _countof( gs_primOut ); i++ )
							{
								if( token.is( gs_primOut[ i ].name ) )
								{
									parseData.primLayout.prim_out_type = ( uint8 )i;
									found = true;
									break;
								}
							}
							if( !found )
							{
								src.Error( "Bad gs prim_out type ( %s )", token.c_str() );
								return false;
							}

							if( src.ExpectTokenString( "," ) )
							{
								if( src.ExpectTokenType( TT_NUMBER, TT_INTEGER, token ) )
								{
									if( GLEW_ARB_gpu_shader5 )
									{
										parseData.primLayout.invoc = ( uint8 )idMath::Min( 32U, token.GetUnsignedIntValue() );
										if( 32U < token.GetUnsignedIntValue() ) {
											src.Warning( "Bad geometryShader instance count %u ( maximum is 32 )\n", token.GetUnsignedIntValue() );
										}
										parseData.primLayout.count = gs_primOut[ parseData.primLayout.prim_out_type ].count;
									}
									else {
										parseData.primLayout.count = ( uint8 )( token.GetUnsignedIntValue() * gs_primOut[ parseData.primLayout.prim_out_type ].count );
										parseData.primLayout.invoc = 0;
									}

									if( src.ExpectTokenString( ")" ) )
									{
										if( src.ExpectTokenString( ";" ) )
										{
											return true;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		else {
			if( token.is( "PRIM_IN" ) )
			{
				if( src.ExpectTokenString( "(" ) )
				{
					if( src.ExpectTokenType( TT_NAME, 0, token ) )
					{
						bool found = false;
						for( uint32 i = 0; i < _countof( gs_primIn ); i++ )
						{
							if( token.is( gs_primIn[ i ].name ) )
							{
								parseData.primLayout.prim_in_type = ( uint8 )i;
								found = true;
								break;
							}
						}
						if( !found )
						{
							src.Error( "Bad prim_in type ( %s )", token.c_str() );
							return false;
						}

						if( src.CheckTokenString( "," ) )
						{
							if( src.ExpectTokenType( TT_NUMBER, TT_INTEGER, token ) )
							{
								parseData.primLayout.invoc = ( uint8 )idMath::Min( 32U, token.GetUnsignedIntValue() );
								if( 32U < token.GetUnsignedIntValue() )
								{
									src.Warning( "Bad geometryShader instance count %u ( maximum is 32 )\n", token.GetUnsignedIntValue() );
								}
							}
						}
						if( src.ExpectTokenString( ")" ) )
						{
							if( src.ExpectTokenString( ";" ) )
							{
								return true;
							}
						}
					}
				}
			}
			if( token.is( "PRIM_OUT" ) )
			{
				if( src.ExpectTokenString( "(" ) )
				{
					if( src.ExpectTokenType( TT_NAME, 0, token ) )
					{
						bool found = false;
						for( uint32 i = 0; i < _countof( gs_primOut ); i++ )
						{
							if( token.is( gs_primOut[ i ].name ) )
							{
								parseData.primLayout.prim_out_type = ( uint8 )i;
								found = true;
								break;
							}
						}
						if( !found )
						{
							src.Error( "Bad prim_out type ( %s )", token.c_str() );
							return false;
						}

						if( src.ExpectTokenString( "," ) )
						{
							if( src.ExpectTokenType( TT_NUMBER, TT_INTEGER, token ) )
							{
								parseData.primLayout.count = ( uint8 )token.GetUnsignedIntValue();

								if( src.ExpectTokenString( ")" ) )
								{
									if( src.ExpectTokenString( ";" ) )
									{
										return true;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if( shaderType == shaderType_e::SHT_TESS_CTRL && !inMain )
	{
		// PATCH( N, TRIANGLES/QUADS/ISOLINE, EQUAL/FRAC_EVEN/FRAC_ODD, CW/CCW ) N - vert count

		if( token.is( "PATCH_LAYOUT" ) )
		{
			if( src.ExpectTokenString( "(" ) )
			{
				if( src.ExpectTokenType( TT_NUMBER, TT_INTEGER, token ) )
				{
					parseData.tessLayout.count = ( uint8 )idMath::Min( 128U, token.GetUnsignedIntValue() );
					if( 128U < token.GetUnsignedIntValue() )
					{
						src.Warning( "Bad tessShader vertex count %u ( maximum is 128 )\n", token.GetUnsignedIntValue() );
					}

					if( src.CheckTokenString( "," ) )
					{
						if( src.ExpectTokenType( TT_NAME, 0, token ) )
						{
							bool found = false;
							for( uint32 i = 0; i < _countof( tessPrimType ); i++ )
							{
								if( token.is( tessPrimType[ i ].name ) )
								{
									parseData.tessLayout.prim_type = ( uint8 )i;
									found = true;
									break;
								}
							}
							if( !found )
							{
								src.Error( "Bad tess prim_type ( %s )", token.c_str() );
								return false;
							}

							if( src.CheckTokenString( "," ) )
							{
								if( src.ExpectTokenType( TT_NAME, 0, token ) )
								{
									found = false;
									for( uint32 i = 0; i < _countof( tessSpacing ); i++ )
									{
										if( token.is( tessSpacing[ i ].name ) )
										{
											parseData.tessLayout.prim_spacing = ( uint8 )i;
											found = true;
											break;
										}
									}
									if( !found )
									{
										src.Error( "Bad tess prim_spacing ( %s )", token.c_str() );
										return false;
									}

									if( src.CheckTokenString( "," ) )
									{
										if( src.ExpectTokenType( TT_NAME, 0, token ) )
										{
											found = false;
											for( uint32 i = 0; i < _countof( tessTopology ); i++ )
											{
												if( token.is( tessTopology[ i ].name ) )
												{
													parseData.tessLayout.prim_topology = ( uint8 )i;
													found = true;
													break;
												}
											}
											if( !found )
											{
												src.Error( "Bad tess prim_topology ( %s )", token.c_str() );
												return false;
											}
										}
									}
								}
							}
						}
					}

					if( src.ExpectTokenString( ")" ) )
					{
						if( src.ExpectTokenString( ";" ) )
						{
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

static void SetupLayouts( idShaderStringBuilder & shaderText, progParseData_t & parseData, shaderType_e shaderType )
{
	if( shaderType == shaderType_e::SHT_TESS_CTRL )
	{
		idStrStatic<128> layout;
		shaderText += "\n";

	#ifdef USE_OPENGL

		shaderText += layout.Format( "layout( vertices = %u ) out;\n", ( uint32 )parseData.tessLayout.count ); layout.Clear();

	#else // HLSL
		///shaderText += layout.Format( "[patchsize(%u)]\n", ( uint32 )parseData.tessLayout.count ); layout.Clear();		shaderText += layout.Format( "[domain(\"%s\")]\n", tessPrimType[ parseData.tessLayout.prim_type ].hlslName ); layout.Clear();		shaderText += layout.Format( "[partitioning(\"%s\")]\n", tessSpacing[ parseData.tessLayout.prim_spacing ].hlslName ); layout.Clear();		shaderText += layout.Format( "[outputtopology(\"%s\")]\n", tessTopology[ parseData.tessLayout.prim_topology ].hlslName ); layout.Clear();		shaderText += layout.Format( "[outputcontrolpoints(%u)]\n", ( uint32 )parseData.tessLayout.count ); layout.Clear();
		shaderText += "[patchconstantfunc(\"PATCH_BLOCK\")]\n";
		shaderText += "[maxtessfactor( 63.0 )]\n";
	#endif
	}
	if( shaderType == shaderType_e::SHT_TESS_EVAL )
	{
		idStrStatic<128> layout;
		shaderText += "\n";

	#ifdef USE_OPENGL

		shaderText += layout.Format( "layout( %s, %s, %s ) in;\n", tessPrimType[ parseData.tessLayout.prim_type ].glslName,
			tessSpacing[ parseData.tessLayout.prim_spacing ].glslName,
			tessTopology[ parseData.tessLayout.prim_topology ].glslName );
	#else // HLSL

		shaderText += layout.Format( "[domain(\"%s\")]\n", tessPrimType[ parseData.tessLayout.prim_type ].hlslName );

		// SV_DomainLocation
		// float2	quad patch
		// float3	tri patch
		// float2	isoline
	#endif
	}

	if( shaderType == shaderType_e::SHT_GEOMETRY )
	{
		idStrStatic<128> layout;
		shaderText += "\n";

	#ifdef USE_OPENGL

		if( parseData.primLayout.invoc )
		{
			shaderText += layout.Format( "layout( %s, invocations = %u ) in;\n", gs_primIn[ parseData.primLayout.prim_in_type ].glslName, ( uint32 )parseData.primLayout.invoc ); layout.Clear();
		}
		else {
			shaderText += layout.Format( "layout( %s ) in;\n", gs_primIn[ parseData.primLayout.prim_in_type ].glslName ); layout.Clear();
		}

		shaderText += layout.Format( "layout( %s, max_vertices = %u ) out;\n", gs_primOut[ parseData.primLayout.prim_out_type ].glslName, ( uint32 )parseData.primLayout.count ); layout.Clear();

	#else // HLSL

		if( parseData.primLayout.invoc ) {
			shaderText += layout.Format( "[instance(%u)]\n", ( uint32 )parseData.primLayout.invoc ); layout.Clear();
		}
		shaderText += layout.Format( "[maxvertexcount(%u)]\n", ( uint32 )parseData.primLayout.count ); layout.Clear();
	#endif
	}

	if( shaderType == shaderType_e::SHT_FRAGMENT )
	{
		/*if( parseData.data.hasDepthOutput || parseData.data.hasFragClip ) // || parseData.data.hasImageStore
		{
			if( GLEW_ARB_shader_image_load_store ) // Early Per-Fragment Tests
			{
				shaderText += "\n";
				shaderText += "layout( early_fragment_tests ) in;\n";
			}
		}*/
		if( parseData.flags.HasFlag( HAS_FORCED_EARLY_FRAG_TESTS ) )
		{
			shaderText += "\n";
		#ifdef USE_OPENGL
			shaderText += "layout( early_fragment_tests ) in;\n";
		#else
			shaderText += "[earlydepthstencil]\n";
		#endif
		}


		// GL_ARB_conservative_depth
		// layout (depth_any/depth_greater/depth_less/depth_unchanged) out float gl_FragDepth;
	}

	if( shaderType == shaderType_e::SHT_COMPUTE )
	{
		idStrStatic<128> layout;
		shaderText += "\n";
	#ifdef USE_OPENGL
		shaderText += layout.Format( "layout( local_size_x = %u, local_size_y = %u, local_size_z = %u ) in;\n" );
	#else // HLSL
		shaderText += layout.Format( "[numthreads( %u, %u, %u )]\n" );
	#endif
	}
}

static bool CheckBuiltins( idStr & shaderText, const idToken & token, const idToken & member, const char newline[], const idStr & index, const bool bInputs, progParseData_t & parseData, shaderType_e shaderType )
{
	//// VERTEX SHADER
	const glslInOut_t vsInputs[] = {
		{ "vertexID",				"",		"gl_VertexID",				"uint",		"SV_VertexID" },
		{ "instanceID",				"",		"gl_InstanceID",			"uint",		"SV_InstanceID" },
		{ "drawID",					"",		"gl_DrawID",				"",			"" },	//DX?
		{ "baseVertexID",			"",		"gl_BaseVertex",			"",			"" },	//DX?
		{ "baseInstanceID",			"",		"gl_BaseInstance",			"",			"" },	//DX?
	};
	const glslInOut_t vsOutputs[] = {
		{ "position",				"",		"gl_Position",				"float4",	"SV_Position" },
		{ "clip",					"",		"gl_ClipDistance",			"float",	"SV_ClipDistance" },
		{ "cull",					"",		"gl_CullDistance",			"float",	"SV_CullDistance" },
		{ "layerIndex",				"",		"gl_Layer",					"uint",		"SV_RenderTargetArrayIndex" },
		{ "viewportIndex",			"",		"gl_ViewportIndex",			"uint",		"SV_ViewportArrayIndex" },
	};
	//////////////////////////////////////////////////////// TESS CONTROL SHADER	- per patch
	const glslInOut_t tcsInputs[] = {
		{ "position",			"gl_in[",	".gl_Position",				"float4",	"SV_Position" },		// gl_in[ gl_MaxPatchVertices ]
		{ "clip",				"gl_in[",	".gl_ClipDistance",			"float",	"SV_ClipDistance" },	// gl_in[ gl_MaxPatchVertices ]
		{ "cull",				"gl_in[",	".gl_CullDistance",			"float",	"SV_CullDistance" },	// gl_in[ gl_MaxPatchVertices ]
		{ "count",				"gl_in.",	"length()",					"",			"" }, //uint .Length()
		{ "patchVertices",		"",			"gl_PatchVerticesIn",		"",			"" },					//? to func GetNumPatchVertices()
		{ "primitiveID",		"",			"gl_PrimitiveID",			"uint",		"SV_PrimitiveID" },
		{ "invocationID",		"",			"gl_InvocationID",			"uint",		"SV_OutputControlPointID" },
	};
	const glslInOut_t tcsOutputs[] = {
		{ "position",			"gl_out[",	".gl_Position",				"float4",	"SV_Position" },		// gl_out[]
		{ "clip",				"gl_out[",	".gl_ClipDistance",			"float",	"SV_ClipDistance" },	// gl_out[]
		{ "cull",				"gl_out[",	".gl_CullDistance",			"float",	"SV_CullDistance" },	// gl_out[]
		{ "tessLevelOuter",		"",			"gl_TessLevelOuter",		"float",	"SV_TessFactor" },		// [4]
		{ "tessLevelInner",		"",			"gl_TessLevelInner",		"float",	"SV_InsideTessFactor" },// [2]
	};
	///////////////////////////// TESS EVALUATION SHADER	- per vertex
	const glslInOut_t tesInputs[] = {
		{ "position",			"gl_in[",	".gl_Position",				"float4",	"SV_Position" },		// gl_in[ gl_MaxPatchVertices ]
		{ "clip",				"gl_in[",	".gl_ClipDistance",			"float",	"SV_ClipDistance" },	// gl_in[ gl_MaxPatchVertices ]
		{ "cull",				"gl_in[",	".gl_CullDistance",			"float",	"SV_CullDistance" },	// gl_in[ gl_MaxPatchVertices ]
		{ "count",				"gl_in.",	"length()",					"",			"" },  //uint .Length()
		{ "patchVertices",		"",			"gl_PatchVerticesIn",		"",			"" },					//? to func GetNumPatchVertices()
		{ "primitiveID",		"",			"gl_PrimitiveID",			"uint",		"SV_PrimitiveID" },
		{ "tessCoord",			"",			"gl_TessCoord",				"float3",	"SV_DomainLocation" },	// float2 quad / float3	tri / float2 isoline
		{ "tessLevelOuter",		"",			"gl_TessLevelOuter",		"float",	"SV_TessFactor" },		// float[4]	quad / float[3]	tri / float[2] isoline
		{ "tessLevelInner",		"",			"gl_TessLevelInner",		"float",	"SV_InsideTessFactor" },// float[2]	quad / float[1]	tri / unused isoline
	};
	const glslInOut_t tesOutputs[] = {
		{ "position",			"gl_out[",	".gl_Position",				"float4",	"SV_Position" },		// gl_out[]
		{ "clip",				"gl_out[",	".gl_ClipDistance",			"float",	"SV_ClipDistance" },	// gl_out[]
		{ "cull",				"gl_out[",	".gl_CullDistance",			"float",	"SV_CullDistance" },	// gl_out[]
		///{ "tessLevelOuter",		"",			"gl_TessLevelOuter",		"float",	"SV_TessFactor" },		// [4]
		///{ "tessLevelInner",		"",			"gl_TessLevelInner",		"float",	"SV_InsideTessFactor" },// [2]
	};
	//////////////////////////////////////////////////////// GEOMETRY SHADER
	const glslInOut_t gsInputs[] = {
		{ "position",			"gl_in[",	".gl_Position",				"float4",	"SV_Position" },		// gl_in[]
		{ "clip",				"gl_in[",	".gl_ClipDistance",			"float",	"SV_ClipDistance" },	// gl_in[]
		{ "cull",				"gl_in[",	".gl_CullDistance",			"float",	"SV_CullDistance" },	// gl_in[]
		{ "count",				"gl_in.",	"length()",					"",			"" },  //uint .Length()
		{ "primitiveID",		"",			"gl_PrimitiveIDIn",			"uint",		"SV_PrimitiveID" },
		{ "invocationID",		"",			"gl_InvocationID",			"uint",		"SV_GSInstanceID" },	// uint InstanceID : SV_GSInstanceID
	};
	const glslInOut_t gsOutputs[] = {
		{ "position",			"",			"gl_Position",				"float4",	"SV_Position" },
		{ "clip",				"",			"gl_ClipDistance",			"float",	"SV_ClipDistance" },
		{ "cull",				"",			"gl_CullDistance",			"float",	"SV_CullDistance" },
		{ "primitiveID",		"",			"gl_PrimitiveID",			"uint",		"SV_PrimitiveID" },
		{ "layerIndex",			"",			"gl_Layer",					"uint",		"SV_RenderTargetArrayIndex" },
		{ "viewportIndex",		"",			"gl_ViewportIndex",			"uint",		"SV_ViewportArrayIndex" },
	};
	//////////////////////////////////////////////////////// FRAGMENT SHADER
	const glslInOut_t fsInputs[] = {
		{ "position",			"",			"gl_FragCoord",				"float4",	"SV_Position" },
		{ "isFrontFacing",		"",			"gl_FrontFacing",			"bool",		"SV_IsFrontFace" },
		{ "primitiveID",		"",			"gl_PrimitiveID",			"uint",		"SV_PrimitiveID" },
		{ "clip",				"",			"gl_ClipDistance",			"float",	"SV_ClipDistance" },
		{ "cull",				"",			"gl_CullDistance",			"float",	"SV_CullDistance" },
		{ "layerIndex",			"",			"gl_Layer",					"uint",		"SV_RenderTargetArrayIndex" },
		{ "viewportIndex",		"",			"gl_ViewportIndex",			"uint",		"SV_ViewportArrayIndex" },
		{ "sampleID",			"",			"gl_SampleID",				"uint",		"SV_SampleIndex" },
		{ "sampleMask",			"",			"gl_SampleMaskIn",			"uint",		"SV_Coverage" },
		{ "samplePosition",		"",			"gl_SamplePosition",		"",			"" }, //? to func /HLSL: float2 GetRenderTargetSamplePosition( input.sampleID )
		{ "sampleCount",		"",			"gl_NumSamples",			"",			"" }, //? to func GetNumSamples() /HLSL: uint GetRenderTargetSampleCount()
	};
	const glslInOut_t fsOutputs[] = {
		{ "depth",				"",			"gl_FragDepth",				"float",	"SV_Depth" }, // SV_DepthGreaterEqual, SV_DepthLessEqual,
		{ "sampleMask",			"",			"gl_SampleMask",			"uint",		"SV_Coverage" },
		{ "stencilRef",			"",			"gl_FragStencilRef",		"uint",		"SV_StencilRef" },
	};
	////////////////////////////////////////////////////////
	const glslInOut_t csInputs[] = {
		{ "numWorkGroups",		"",			"gl_NumWorkGroups",			"uint",		"" }, //? to func GetNumWorkGroups()
		{ "groupID",			"",			"gl_WorkGroupID",			"uint3",	"SV_GroupID" },
		{ "groupThreadID",		"",			"gl_LocalInvocationID",		"uint3",	"SV_GroupThreadID" },
		{ "dispatchThreadID",	"",			"gl_GlobalInvocationID",	"uint3",	"SV_DispatchThreadID" },
		{ "groupIndex",			"",			"gl_LocalInvocationIndex",	"uint",		"SV_GroupIndex" },
	};
	//const glslInOut_t csOutputs[] = {
		// The local size is available to the shader as a compile-time constant variable, so you don't need to define it yourself:
		//{ "WorkGroupSize",			"gl_WorkGroupSize" }, //? to func GetWorkGroupSize()
	//};

	/*/*/if( shaderType == shaderType_e::SHT_VERTEX ) ///////////////////////////////
	{
		if( bInputs )
		{
			for( uint32 i = 0; i < _countof( _vertexAttribsPC ); i++ )
			{
				if( member.is( _vertexAttribsPC[ i ].name ) )
				{
					parseData.vsInputs.AddUnique( _vertexAttribsPC[ i ] );
					parseData.vertexMask.SetFlag( BIT( _vertexAttribsPC[ i ].bind ) );

					shaderText += CHECK_NEW_LINE_AND_SPACES();
					shaderText += _vertexAttribsPC[ i ].glsl;
					return true;
				}
			}
			for( uint32 i = 0; i < _countof( vsInputs ); i++ )
			{
				if( member.is( vsInputs[ i ].nameCG ) )
				{
					shaderText += CHECK_NEW_LINE_AND_SPACES();
					shaderText += vsInputs[ i ].nameGLSL;
					return true;
				}
			}
		}
		else {
			for( uint32 i = 0; i < _countof( vsOutputs ); i++ )
			{
				if( member.is( vsOutputs[ i ].nameCG ) )
				{
					shaderText += CHECK_NEW_LINE_AND_SPACES();
					shaderText += vsOutputs[ i ].nameGLSL;
					return true;
				}
			}
		}
	}
	else if( shaderType == shaderType_e::SHT_TESS_CTRL) /////////////////////////////
	{
		if( bInputs )
		{
			for( uint32 i = 0; i < _countof( tcsInputs ); i++ )
			{
				if( member.is( tcsInputs[ i ].nameCG ) )
				{
					shaderText += CHECK_NEW_LINE_AND_SPACES();
					if( !( idStr::Cmp( tcsInputs[ i ].prefGLSL, "" ) == 0 ) )
					{
						shaderText += tcsInputs[ i ].prefGLSL;
					}
					if( !index.IsEmpty() )
					{
						//shaderText += " ";
						shaderText += index;
					}
					shaderText += tcsInputs[ i ].nameGLSL;
					return true;
				}
			}
		}
		else {
			for( uint32 i = 0; i < _countof( tcsOutputs ); i++ )
			{
				if( member.is( tcsOutputs[ i ].nameCG ) )
				{
					shaderText += CHECK_NEW_LINE_AND_SPACES();
					if( !( idStr::Cmp( tcsOutputs[ i ].prefGLSL, "" ) == 0 ) )
					{
						shaderText += tcsOutputs[ i ].prefGLSL;
					}
					if( !index.IsEmpty() )
					{
						//shaderText += " ";
						shaderText += index;
					}
					shaderText += tcsOutputs[ i ].nameGLSL;
					return true;
				}
			}
		}
	}
	else if( shaderType == shaderType_e::SHT_TESS_EVAL ) ////////////////////////////
	{
		if( bInputs )
		{
			for( uint32 i = 0; i < _countof( tesInputs ); i++ )
			{
				if( member.is( tesInputs[ i ].nameCG ) )
				{
					shaderText += CHECK_NEW_LINE_AND_SPACES();
					if( !( idStr::Cmp( tesInputs[ i ].prefGLSL, "" ) == 0 ) )
					{
						shaderText += tesInputs[ i ].prefGLSL;
					}
					if( !index.IsEmpty() )
					{
						//shaderText += " ";
						shaderText += index;
					}
					shaderText += tesInputs[ i ].nameGLSL;
					return true;
				}
			}
		}
		else {
			for( uint32 i = 0; i < _countof( tesOutputs ); i++ )
			{
				if( member.is( tesOutputs[ i ].nameCG ) )
				{
					shaderText += CHECK_NEW_LINE_AND_SPACES();
					shaderText += tesOutputs[ i ].nameGLSL;
					return true;
				}
			}
		}
	}
	else if( shaderType == shaderType_e::SHT_GEOMETRY ) /////////////////////////////
	{
		if( bInputs )
		{
			for( uint32 i = 0; i < _countof( gsInputs ); i++ )
			{
				if( member.is( gsInputs[ i ].nameCG ) )
				{
					shaderText += CHECK_NEW_LINE_AND_SPACES();
					if( !( idStr::Cmp( gsInputs[ i ].prefGLSL, "" ) == 0 ) )
					{
						shaderText += gsInputs[ i ].prefGLSL;
					}
					if( !index.IsEmpty() )
					{
						//shaderText += " ";
						shaderText += index;
					}
					shaderText += gsInputs[ i ].nameGLSL;
					return true;
				}
			}
		}
		else {
			for( uint32 i = 0; i < _countof( gsOutputs ); i++ )
			{
				if( member.is( gsOutputs[ i ].nameCG ) )
				{
					shaderText += CHECK_NEW_LINE_AND_SPACES();
					shaderText += gsOutputs[ i ].nameGLSL;
					return true;
				}
			}
		}
	}
	else if( shaderType == shaderType_e::SHT_FRAGMENT ) /////////////////////////////
	{
		if( bInputs )
		{
			for( uint32 i = 0; i < _countof( fsInputs ); i++ )
			{
				if( member.is( fsInputs[ i ].nameCG ) )
				{
					shaderText += CHECK_NEW_LINE_AND_SPACES();
					shaderText += fsInputs[ i ].nameGLSL;
					return true;
				}
			}
		}
		else {
			for( uint32 i = 0; i < _countof( _fragmentAttribsPC ); i++ )
			{
				if( member.is( _fragmentAttribsPC[ i ].name ) )
				{
					parseData.fsOutputs.AddUnique( _fragmentAttribsPC[ i ] );

					shaderText += CHECK_NEW_LINE_AND_SPACES();
					shaderText += _fragmentAttribsPC[ i ].glsl;
					return true;
				}
			}
			for( uint32 i = 0; i < _countof( fsOutputs ); i++ )
			{
				if( member.is( fsOutputs[ i ].nameCG ) )
				{
					shaderText += CHECK_NEW_LINE_AND_SPACES();
					shaderText += fsOutputs[ i ].nameGLSL;

					if( i == 0 ) {
						parseData.flags.SetFlag( HAS_DEPTH_OUTPUT );
					}
					else if( i = 1 ) {
						parseData.flags.SetFlag( HAS_COVERAGE_OUTPUT );
					}
					else if( i = 2 ) {
						parseData.flags.SetFlag( HAS_STENCIL_OUTPUT );
					}
					return true;
				}
			}
		}
	}
	else if( shaderType == shaderType_e::SHT_COMPUTE ) //////////////////////////////
	{
		assert( "SHT_COMPUTE" );
	}

	return false;
}

static bool ParseInOutVars( idLexer & lex, idStr & shaderText, idToken & token, const char newline[], progParseData_t & parseData, shaderType_e shaderType )
{
	if( token.IsEmpty() )
		return false;

	if( token == "input" )
	{
		idStrStatic<128> bracedStr; //form:  str]
		if( lex.CheckTokenString( "[" ) )
		{
			idToken token2;
			lex.ExpectAnyToken( &token2 );

			// special case if we use builtins as index ( like: input[ input.count ], input[ input[0].texcoord0 ] etc. )
			if( !ParseInOutVars( lex, bracedStr, token2, newline, parseData, shaderType ) )
			{
				if( /*lex.HadWarning() ||*/ lex.HadError() )
					return false;

				lex.UnreadToken( &token2 );
			}

			idStrStatic<128> bracedStrTemp;
			lex.ParseBracedSection( bracedStrTemp, -1, false, '[', ']' );
			bracedStr += bracedStrTemp;
		}

		if( lex.ExpectTokenString( "." ) )
		{
			idToken member;
			// read input member
			lex.ExpectAnyToken( &member );

			if( CheckBuiltins( shaderText, token, member, newline, bracedStr, true, parseData, shaderType ) )
			{
				return true;
			}

			if( shaderType != shaderType_e::SHT_VERTEX )
			{
				// user's inputs
				for( int i = 0; i < parseData.variables.Num(); i++ )
				{
					// check if it was declared previously
					auto & var = parseData.variables[ i ];
					if( var.name == member )
					{
						// add it if found, else emit Error()
						shaderText += CHECK_NEW_LINE_AND_SPACES();
						shaderText += "_in";
						if( !bracedStr.IsEmpty() && shaderType != SHT_FRAGMENT )
						{
							shaderText += "[";
							shaderText += bracedStr;
							///shaderText += "]";
						}
						shaderText += ".";
						shaderText += member;
						return true;
					}
				}
			}
			if( !bracedStr.IsEmpty() )
				lex.Error( "Unknown input[%s.%s in %s", bracedStr.c_str(), member.c_str(), _glslShaderInfo[ shaderType ].shaderTypeDeclName );
			else
				lex.Error( "Unknown input.%s in %s", member.c_str(), _glslShaderInfo[ shaderType ].shaderTypeDeclName );

			return false;
		}
	}
	else if( token == "output" )
	{
		idStrStatic<128> bracedStr;
		if( lex.CheckTokenString( "[" ) )
		{
			idToken token2;
			lex.ExpectAnyToken( &token2 );

			// special case if we use builtins as index
			if( !ParseInOutVars( lex, bracedStr, token2, newline, parseData, shaderType ) )
			{
				if( /*lex.HadWarning() ||*/ lex.HadError() )
					return false;

				lex.UnreadToken( &token2 );
			}

			idStrStatic<128> bracedStrTemp;
			lex.ParseBracedSection( bracedStrTemp, -1, false, '[', ']' );
			bracedStr += bracedStrTemp;
		}

		if( lex.ExpectTokenString( "." ) )
		{
			idToken member;
			// read output member
			lex.ExpectAnyToken( &member );

			if( shaderType == shaderType_e::SHT_GEOMETRY )
			{
				if( member == "BeginVertex" )
				{
					if( lex.ExpectTokenString( "(" ) )
					{
						if( lex.ExpectTokenString( ")" ) )
						{
							if( lex.ExpectTokenString( ";" ) )
							{
								shaderText += CHECK_NEW_LINE_AND_SPACES();
							#ifdef USE_OPENGL
								///shaderText += "EmitVertex();";
							#else
								shaderText += "_out = (GS_OUT)0;";
							#endif
								return true;
							}
						}
					}
					return false;
				}
				else if( member == "FinishVertex" )
				{
					if( lex.ExpectTokenString("(") ) {
						if( lex.ExpectTokenString( ")" ) ) {
							if( lex.ExpectTokenString( ";" ) )
							{
								shaderText += CHECK_NEW_LINE_AND_SPACES();
							#ifdef USE_OPENGL
								shaderText += "EmitVertex();";
							#else
								shaderText += "_stm.Append( _out );";
							#endif
								return true;
							}
						}
					}
					return false;
				}
				else if( member == "FinishPrimitive" )
				{
					if( lex.ExpectTokenString( "(" ) ) {
						if( lex.ExpectTokenString( ")" ) ) {
							if( lex.ExpectTokenString( ";" ) )
							{
								shaderText += CHECK_NEW_LINE_AND_SPACES();
							#ifdef USE_OPENGL
								shaderText += "EndPrimitive();";
							#else
								shaderText += "_stm.RestartStrip();";
							#endif
								return true;
							}
						}
					}
					return false;
				}
			}

			if( CheckBuiltins( shaderText, token, member, newline, bracedStr, false, parseData, shaderType ) )
			{
				return true;
			}

			// user's outputs
			if( shaderType != shaderType_e::SHT_FRAGMENT )
			{
				bool found = false;
				for( int i = 0; i < parseData.variables.Num(); i++ )
				{
					auto & var = parseData.variables[ i ];
					if( var.name == member )
					{
						parseData.varOut[ shaderType ].AddUnique( i );
						found = true;
						break;
					}
				}
				if( !found )
				{
					varInOut_t var;

					if( member == "tbn" )
					{
						var.typeIndex = 26U;
						var.name = "tbn";
					}
					else {
						//var.type = "vec4";
						var.typeIndex = 5U;
						var.name = member;
					}

					int16 i = parseData.variables.Append( var );
					parseData.varOut[ shaderType ].Append( i );
				}

				shaderText += CHECK_NEW_LINE_AND_SPACES();
				shaderText += "_out";
				if( !bracedStr.IsEmpty() )
				{
					shaderText += "[";
					shaderText += bracedStr;
					///shaderText += "]";
				}
				shaderText += ".";
				shaderText += member;
				return true;
			}
		}
	}

	return false;
/*
	shaderText += idStrStatic<128>().Format( "input[%s].%s" );
	shaderText += idStrStatic<128>().Format( "gl_in[%s].%s" );
	shaderText += idStrStatic<128>().Format( "gl_out[%s].%s" );
	shaderText += idStrStatic<128>().Format( "input.%s" );
	shaderText += idStrStatic<128>().Format( "gl_in.%s" );
	shaderText += idStrStatic<128>().Format( "gl_out.%s" );
	shaderText += idStrStatic<128>().Format( "%s" );
*/
}

static void SetupInOutVars( idShaderStringBuilder & shaderText, progParseData_t & parseData, shaderType_e shaderType )
{
	//SEA: no redeclaring of fs/vs builtin vars for now.

	// Setup vertex shader inputs.
	if( shaderType == shaderType_e::SHT_VERTEX && parseData.vsInputs.Num() )
	{
		shaderText += "\n";

		for( int i = 0; i < parseData.vsInputs.Num(); i++ )
		{
			auto & var = parseData.vsInputs[ i ];

			for( int i = 0; _typeConversion[ i ].typeCG != NULL; i++ ) {
				if( idStr::Cmp( var.type, _typeConversion[ i ].typeCG ) == 0 ) {
					var.type  = _typeConversion[ i ].typeGLSL;
					break;
				}
			}
			if( glConfig.explicit_attrib_location ) {
				shaderText += _Txt( "layout( location = %u ) in %s %s;\n", var.bind, var.type, var.glsl );
			} else {
				shaderText += _Txt( "in %s %s;\n", var.bind, var.type, var.glsl );
			}
		}

		if( glConfig.explicit_attrib_location )
			parseData.vsInputs.Clear();

		//shaderText += "\n";
	#if 0
		switch( shaderType )
		{
			case SHT_VERTEX:
				shaderText += "struct VS_IN {\n";
				break;
			case SHT_TESS_CTRL:
				shaderText += "struct HS_IN {\n";
				break;
			case SHT_TESS_EVAL:
				shaderText += "struct DS_IN {\n";
				break;
			case SHT_GEOMETRY:
				shaderText += "struct GS_IN {\n";
				break;
			case SHT_FRAGMENT:
				shaderText += "struct PS_IN {\n";
				break;
		}
		for( int i = 0; i < parseData.vsInputs.Num(); i++ )
		{
			auto & var = parseData.vsInputs[ i ];

			idStrStatic<64> symantic( var.name );
			symantic.ToUpper();

			shaderText += _Txt( "\t%s %s : _%s;\n", var.type, var.name, symantic );
		}
		shaderText += "};\n";

		parseData.vsInputs.Clear();
	#endif
	}
	else if( parseData.varIn[ shaderType ].Num() )
	// Custom input structure. Interface matching.
	{
		// outputs from previous shader stage are the input
		shaderText += "\n";
		shaderText += "in vars {\n";
		for( int i = 0; i < parseData.varIn[ shaderType ].Num(); i++ )
		{
			auto & var = parseData.variables[ ( int )parseData.varIn[ shaderType ][ i ] ];

			shaderText += "\t";
			///shaderText += var.type + " ";
			shaderText += _typeConversion[ var.typeIndex ].typeGLSL; shaderText += " "; shaderText += var.name + ";\n";
		}
		if( shaderType == shaderType_e::SHT_TESS_CTRL || shaderType == shaderType_e::SHT_TESS_EVAL )
		{
			shaderText += _Txt( "} _in[%u];\n", parseData.tessLayout.count );
		}
		else if( shaderType == shaderType_e::SHT_GEOMETRY )
		{
			///shaderText += "} _in[];\n";
			shaderText += _Txt( "} _in[%u];\n", gs_primIn[ parseData.primLayout.prim_in_type ].count );
		}
		else {
			shaderText += "} _in;\n";
		}
	}



	if( shaderType == shaderType_e::SHT_TESS_CTRL )
	{
		"struct TCS_IN {\n";
		"\t%s %s : %s;\n";
		"\tfloat4 position : SV_Position;\n";
		"};\n";
		"struct TCS_OUT {\n";
		"\t%s %s : %s;\n";
		"\tfloat4 position : SV_Position;\n";
		"};\n";
		"struct TCS_OUT_CONST {\n";
		"\tfloat tessLevelOuter[ 3 ] : SV_TessFactor;\n";
		"\tfloat tessLevelInner[ 1 ] : SV_InsideTessFactor;\n";
		"\t%s %s : %s;\n";
		"};\n";
	}
	if( shaderType == shaderType_e::SHT_TESS_EVAL )
	{
		"struct TES_IN {\n";
		"\t%s %s : %s;\n";
		"\tfloat4 position : SV_Position;\n";
		"};\n";
		"struct TES_OUT {\n";
		"\t%s %s : %s;\n";
		"\tfloat4 position : SV_Position;\n";
		"};\n";
		"struct TES_IN_CONST {\n";
		"\tfloat tessLevelOuter[ 3 ] : SV_TessFactor;\n";
		"\tfloat tessLevelInner[ 1 ] : SV_InsideTessFactor;\n";
		"\t%s %s : %s;\n";
		"};\n";
	}




	// Setup fragment shader outputs.
	if( shaderType == shaderType_e::SHT_FRAGMENT && parseData.fsOutputs.Num() )
	{
		parseData.flags.SetFlag( HAS_FRAG_OUTPUT );
		if( parseData.fsOutputs.Num() > 1 ) {
			parseData.flags.SetFlag( HAS_MRT_OUTPUT );
		}

		shaderText += "\n";
		for( int i = 0; i < parseData.fsOutputs.Num(); i++ )
		{
			auto & var = parseData.fsOutputs[ i ];

			for( int i = 0; _typeConversion[ i ].typeCG != NULL; i++ ) {
				if( idStr::Cmp( var.type, _typeConversion[ i ].typeCG ) == 0 ) {
					var.type  = _typeConversion[ i ].typeGLSL;
					break;
				}
			}
			if( glConfig.explicit_attrib_location ) {
				shaderText += _Txt( "layout( location = %u ) out %s %s;\n", var.bind, var.type, var.glsl );
			} else {
				shaderText += _Txt( "out %s %s;\n", var.bind, var.type, var.glsl );
			}
		}

		if( glConfig.explicit_attrib_location )
			parseData.fsOutputs.Clear();

		//shaderText += "\n";
	#if 0
		switch( shaderType )
		{
			case SHT_VERTEX:
				shaderText += "struct VS_OUT {\n";
				break;
			case SHT_TESS_CTRL:
				shaderText += "struct HS_OUT {\n";
				break;
			case SHT_TESS_EVAL:
				shaderText += "struct DS_OUT {\n";
				break;
			case SHT_GEOMETRY:
				shaderText += "struct GS_OUT {\n";
				break;
			case SHT_FRAGMENT:
				shaderText += "struct PS_OUT {\n";
				break;
		}
		for( int i = 0; i < parseData.fsOutputs.Num(); i++ )
		{
			auto & var = parseData.fsOutputs[ i ];

			idStrStatic<64> symantic( var.name );
			symantic.ToUpper();

			shaderText += _Txt( "\t%s %s : %s;\n", var.type, var.name, symantic );
		}
		shaderText += "};\n";

		parseData.fsOutputs.Clear();
	#endif
	}
	else if( parseData.varOut[ shaderType ].Num() )
	// Custom output structure. Interface matching.
	{
		shaderText += "\n";
		shaderText += "out vars {\n";
		for( int i = 0; i < parseData.varOut[ shaderType ].Num(); i++ )
		{
			auto & var = parseData.variables[ ( int )parseData.varOut[ shaderType ][ i ] ];

			shaderText += "\t";
			//shaderText += var.type + " ";
			shaderText += _typeConversion[ var.typeIndex ].typeGLSL; shaderText += " "; shaderText += var.name + ";\n";

			// ( smooth flat noperspective ) centroid out
		}
		if( shaderType == shaderType_e::SHT_TESS_CTRL )
		{
			///shaderText += "} _out[];\n"; //SEA: TODO!!!
			shaderText += _Txt( "} _out[%u];\n", parseData.tessLayout.count );
		}
		else {
			shaderText += "} _out;\n";
		}
		//shaderText += "\n";
	}
}

static bool ConvertFuncs( idStr & shaderText, const idToken & token, const char newline[] )
{
	struct builtinConversion_t {
		const char* nameCG;
		const char* nameGLSL;
	}
	builtinConversion[] =
	{
		{ "frac",		"fract" },
		{ "lerp",		"mix" },
		{ "rsqrt",		"inversesqrt" },
		{ "ddx",		"dFdx" },
		{ "ddy",		"dFdy" },
		{ NULL, NULL }
	};
	for( int i = 0; builtinConversion[ i ].nameCG != NULL; i++ )
	{
		if( token.Cmp( builtinConversion[ i ].nameCG ) == 0 )
		{
			shaderText += CHECK_NEW_LINE_AND_SPACES();
			shaderText += builtinConversion[ i ].nameGLSL;
			return true;
		}
	}

	if( token.Cmp( "ddx_coarse" ) == 0 )
	{
		shaderText += CHECK_NEW_LINE_AND_SPACES();
		shaderText += glConfig.derivative_control ? "dFdxCoarse" : "dFdx";
		return true;
	}
	if( token.Cmp( "ddx_fine" ) == 0 )
	{
		shaderText += CHECK_NEW_LINE_AND_SPACES();
		shaderText += glConfig.derivative_control ? "dFdxFine" : "dFdx";
		return true;
	}
	if( token.Cmp( "ddy_coarse" ) == 0 )
	{
		shaderText += CHECK_NEW_LINE_AND_SPACES();
		shaderText += glConfig.derivative_control ? "dFdyCoarse" : "dFdy";
		return true;
	}
	if( token.Cmp( "ddy_fine" ) == 0 )
	{
		shaderText += CHECK_NEW_LINE_AND_SPACES();
		shaderText += glConfig.derivative_control ? "dFdyFine" : "dFdy";
		return true;
	}

	if( glConfig.gpu_shader5 )
	{
		if( token.Cmp( "EvaluateAttributeAtCentroid" ) == 0 )
		{
			shaderText += CHECK_NEW_LINE_AND_SPACES();
			shaderText += "interpolateAtCentroid";
			return true;
		}
		if( token.Cmp( "EvaluateAttributeAtSample" ) == 0 )
		{
			shaderText += CHECK_NEW_LINE_AND_SPACES();
			shaderText += "interpolateAtSample";
			return true;
		}
		if( token.Cmp( "EvaluateAttributeSnapped" ) == 0 )
		{
			shaderText += CHECK_NEW_LINE_AND_SPACES();
			shaderText += "interpolateAtOffset";
			return true;
		}
		if( token.Cmp( "mad" ) == 0 )
		{
			shaderText += CHECK_NEW_LINE_AND_SPACES();
			shaderText += "fma";
			return true;
		}
	}

	return false;
}

static bool ParseParms( idLexer & lex, idStr & shaderText, const idToken & token, const char newline[], progParseData_t & parseData, shaderType_e shaderType )
{
	if( token == "$" )
	{
		idToken parmName;
		if( lex.ExpectTokenType( TT_NAME, 0, parmName ) )
		{
			auto parm = declManager->FindType( DECL_RENDERPARM, parmName, false )->Cast<idDeclRenderParm>();
			if( parm ) 	// search idDeclRenderParm by name and add it to the source
			{
				if( parm->IsImplicit() )
					lex.Warning( "Parm '%s' was defaulted", parm->GetName() );

				int index = parseData.AddRenderParm( parm, shaderType );

				if( ( shaderType == SHT_VERTEX || shaderType == SHT_GEOMETRY ) && parmName.is( "SkinningParms" ) )
				{
					parseData.flags.SetFlag( HAS_HARDWARE_SKINNING );
					parseData.flags.SetFlag( HAS_OPTIONAL_SKINNING );
				}

				shaderText += CHECK_NEW_LINE_AND_SPACES();
				if( parm->IsVector() )
				{
					//shaderText += _Txt( "%s[ %i /* %s */]",  _glslShaderInfo[ shaderType ].uniformArrayName, index, parm->GetName() );
					if( r_useProgUBO.GetBool() ) {
						shaderText += _Txt( COMMON_PARMBLOCK_NAME".%s", parm->GetName() );
					} else {
						shaderText += _Txt( COMMON_PARMBLOCK_NAME"[%i/*%s*/]", index, parm->GetName() );
					}
					return true;
				}
				else if( parm->IsTexture() )
				{
					shaderText += parm->GetName();
					return true;
				}
			}
		}

		shaderText += CHECK_NEW_LINE_AND_SPACES();
		shaderText += " _badParm_";
		shaderText += parmName;
		return true; //SEA: compiler will do the error job.
	}

	return false;
}

static void SetupHeaderExtensions( idShaderStringBuilder & out, shaderType_e shaderType )
{
	//out += "\n";

	// GL_ARB_explicit_attrib_location
	if( glConfig.explicit_attrib_location && ( shaderType == SHT_VERTEX || shaderType == SHT_FRAGMENT ) )	//EGL 3.0, MESA
	{
		out += "#extension GL_ARB_explicit_attrib_location : require\n";
	}

	// GL_ARB_shader_bit_encoding
	/*
	genIType floatBitsToInt(genType value);
	genUType floatBitsToUint(genType value);

	genType intBitsToFloat(genIType value);
	genType uintBitsToFloat(genUType value);
	*/
	// GL_ARB_shading_language_420pack
	if( glConfig.shading_language_420pack ) //EGL 3.1, MESA
	{
		/*
		* Add line-continuation using '\', as in C++.

		* The *const* keyword can be used to declare variables within a function
		body with initializer expressions that are not constant expressions.

		* Add layout qualifier identifier "binding" to bind the location of a uniform block.

		* Add layout qualifier identifier "binding" to bind units to sampler and image variable declarations.

		* Allow swizzle operations on scalars.

		* Built-in constants for gl_MinProgramTexelOffset and gl_MaxProgramTexelOffset.

		* Allow ".length()" to be applied to vectors and matrices, returning the number of components or columns.

		* Add C-style curly brace initializer lists syntax for initializers.
		*/

		out += "#extension GL_ARB_shading_language_420pack : require\n";
	}
	// GL_ARB_gpu_shader5
	if( glConfig.gpu_shader5 )
	{
		out += "#extension GL_ARB_gpu_shader5 : require\n";
		/*
		* support for indexing into arrays of samplers using non-constant indices
		* extending the uniform block capability of OpenGL 3.1 and 3.2 to allow shaders to index into an array of uniform blocks;

		// ...

		// in int gl_InvocationID;
		// layout( triangles, invocations = 6 ) in;

		// in int gl_SampleMaskIn[];

		* support for reading a mask of covered samples in a fragment shader;
		and
		* support for interpolating a fragment shader input at a programmable
		offset relative to the pixel center, a programmable sample number, or
		at the centroid.

		genIType bitfieldExtract(genIType value, int offset, int bits);
		genUType bitfieldExtract(genUType value, int offset, int bits);

		genIType bitfieldInsert(genIType base, genIType insert, int offset, int bits);
		genUType bitfieldInsert(genUType base, genUType insert, int offset, int bits);

		genIType bitfieldReverse(genIType value);
		genUType bitfieldReverse(genUType value);

		genIType bitCount(genIType value);
		genIType bitCount(genUType value);

		genIType findLSB(genIType value);
		genIType findLSB(genUType value);

		genIType findMSB(genIType value);
		genIType findMSB(genUType value);

		uint      packUnorm2x16(vec2 v);
		uint      packUnorm4x8(vec4 v);
		uint      packSnorm4x8(vec4 v);

		vec2      unpackUnorm2x16(uint v);
		vec4      unpackUnorm4x8(uint v);
		vec4      unpackSnorm4x8(uint v);

		packUnorm2x16     fixed_val = round(clamp(float_val, 0, +1) * 65535.0);
		packUnorm4x8      fixed_val = round(clamp(float_val, 0, +1) * 255.0);
		packSnorm4x8      fixed_val = round(clamp(float_val, -1, +1) * 127.0);
		unpackUnorm2x16   float_val = fixed_val / 65535.0;
		unpackUnorm4x8    float_val = fixed_val / 255.0;
		unpackSnorm4x8    float_val = clamp(fixed_val / 127.0, -1, +1);

		genIType floatBitsToInt(genType value);
		genUType floatBitsToUint(genType value);

		genType intBitsToFloat(genIType value);
		genType uintBitsToFloat(genUType value);

		gvec4 textureGather(gsampler2D sampler, vec2 coord[, int comp]);
		gvec4 textureGather(gsampler2DArray sampler, vec3 coord[, int comp]);
		gvec4 textureGather(gsamplerCube sampler, vec3 coord[, int comp]);
		gvec4 textureGather(gsamplerCubeArray sampler, vec4 coord[, int comp]);
		gvec4 textureGather(gsampler2DRect sampler, vec2 coord[, int comp]);

		vec4 textureGather(sampler2DShadow sampler, vec2 coord, float refZ);
		vec4 textureGather(sampler2DArrayShadow sampler, vec3 coord, float refZ);
		vec4 textureGather(samplerCubeShadow sampler, vec3 coord, float refZ);
		vec4 textureGather(samplerCubeArrayShadow sampler, vec4 coord, float refZ);
		vec4 textureGather(sampler2DRectShadow sampler, vec2 coord, float refZ);

		gvec4 textureGatherOffset(gsampler2D sampler, vec2 coord, ivec2 offset[, int comp]);
		gvec4 textureGatherOffset(gsampler2DArray sampler, vec3 coord, ivec2 offset[, int comp]);
		gvec4 textureGatherOffset(gsampler2DRect sampler, vec2 coord, ivec2 offset[, int comp]);

		vec4 textureGatherOffset(sampler2DShadow sampler, vec2 coord, float refZ, ivec2 offset);
		vec4 textureGatherOffset(sampler2DArrayShadow sampler, vec3 coord, float refZ, ivec2 offset);
		vec4 textureGatherOffset(sampler2DRectShadow sampler, vec2 coord, float refZ, ivec2 offset);

		gvec4 textureGatherOffsets(gsampler2D sampler, vec2 coord, ivec2 offsets[4][, int comp]);
		gvec4 textureGatherOffsets(gsampler2DArray sampler, vec3 coord, ivec2 offsets[4][, int comp]);
		gvec4 textureGatherOffsets(gsampler2DRect sampler, vec2 coord, ivec2 offsets[4][, int comp]);

		vec4 textureGatherOffsets(sampler2DShadow sampler, vec2 coord, float refZ, ivec2 offsets[4]);
		vec4 textureGatherOffsets(sampler2DArrayShadow sampler, vec3 coord, float refZ, ivec2 offsets[4]);
		vec4 textureGatherOffsets(sampler2DRectShadow sampler, vec2 coord, float refZ, ivec2 offsets[4]);

		void EmitStreamVertex(int stream);      // Geometry-only
		void EndStreamPrimitive(int stream);    // Geometry-only
		*/
	}
	// GL_ARB_arrays_of_arrays
	if( GLEW_ARB_arrays_of_arrays )
	{
		out += "#extension GL_ARB_arrays_of_arrays : require\n";
	}
	// GL_ARB_viewport_array	MESA
	if( GLEW_ARB_viewport_array )
	{
		out += "#extension GL_ARB_viewport_array : require\n";
	}
	// GL_ARB_sample_shading	MESA
	if( glConfig.sample_shading && shaderType == SHT_FRAGMENT )
	{
		out += "#extension GL_ARB_sample_shading : require\n";
	}
	// GL_ARB_texture_gather	MESA
	if( GLEW_ARB_texture_gather )
	{
		out += "#extension GL_ARB_texture_gather : require\n";
	}
	// GL_ARB_texture_cube_map_array
	if( glConfig.texture_cube_map_array )
	{
		out += "#extension GL_ARB_texture_cube_map_array : require\n";
	}
	// GL_ARB_texture_query_lod
	if( GLEW_ARB_texture_query_lod )
	{
		//out += "#extension GL_ARB_texture_query_lod : require\n";
		/*
		vec2 textureQueryLOD( gsampler1D sampler, float coord )
		vec2 textureQueryLOD( gsampler2D sampler, vec2 coord )
		vec2 textureQueryLOD( gsampler3D sampler, vec3 coord )
		vec2 textureQueryLOD( gsamplerCube sampler, vec3 coor d)
		vec2 textureQueryLOD( gsampler1DArray sampler, float coord )
		vec2 textureQueryLOD( gsampler2DArray sampler, vec2 coord )
		vec2 textureQueryLOD( gsamplerCubeArray sampler, vec3 coord )
		vec2 textureQueryLOD( sampler1DShadow sampler, float coord )
		vec2 textureQueryLOD( sampler2DShadow sampler, vec2 coord )
		vec2 textureQueryLOD( samplerCubeShadow sampler, vec3 coord )
		vec2 textureQueryLOD( sampler1DArrayShadow sampler, float coord )
		vec2 textureQueryLOD( sampler2DArrayShadow sampler, vec2 coord )
		vec2 textureQueryLOD( samplerCubeArrayShadow sampler, vec3 coord )
		*/
	}

	// GL_ARB_conservative_depth	MESA
	if( glConfig.conservative_depth && shaderType == SHT_FRAGMENT )
	{
		out += "#extension GL_ARB_conservative_depth : require\n";
	}

	if( glConfig.shader_image_load_store && shaderType == SHT_FRAGMENT )
	{
		out += "#extension GL_ARB_shader_image_load_store : require\n";
	}

	// GL_ARB_explicit_uniform_location
	if( glConfig.explicit_uniform_location )
	{
		out += "#extension GL_ARB_explicit_uniform_location : require\n";
	}

	// GL_ARB_shader_image_size

	// GL_ARB_texture_cube_map_array
	/*
	samplerCubeArray, samplerCubeArrayShadow,
	isamplerCubeArray,
	usamplerCubeArray
	*/

	// GL_AMD_vertex_shader_layer
	if( glConfig.vertex_shader_layer && shaderType == SHT_VERTEX )
	{
		out += "#extension GL_AMD_vertex_shader_layer : require\n";
	}
	// GL_AMD_vertex_shader_viewport_index
	if( glConfig.vertex_shader_viewport_index && shaderType == SHT_VERTEX )
	{
		out += "#extension GL_AMD_vertex_shader_viewport_index : require\n";
	}
	// GL_ARB_fragment_layer_viewport
	if( glConfig.fragment_layer_viewport && shaderType == SHT_FRAGMENT )
	{
		//	in int gl_Layer;
		//	in int gl_ViewportIndex;
		out += "#extension GL_ARB_fragment_layer_viewport : require\n";
	}
	// GL_NV_viewport_array2 - gl_ViewportIndex / gl_ViewportMask[] / gl_Layer
	// The gl_ViewportMask[] output is available in vertex, tessellation
	//	control, tessellation evaluation, and geometry shaders.gl_ViewportIndex
	//	and gl_Layer are also made available in all these shader stages.

	// GL_ARB_draw_indirect
	/*
	typedef struct {
	GLuint count;
	GLuint primCount;
	GLuint first;
	GLuint reservedMustBeZero;
	} DrawArraysIndirectCommand;

	typedef struct {
	GLuint count;
	GLuint primCount;
	GLuint firstIndex;
	GLint  baseVertex;
	GLuint reservedMustBeZero;
	} DrawElementsIndirectCommand;
	*/
	// GL_ARB_multi_draw_indirect

	// GL_ARB_shader_storage_buffer_object

	// GL_ARB_shading_language_packing
	/*
	uint      packUnorm2x16( vec2 v );
	uint      packSnorm2x16( vec2 v );
	uint      packUnorm4x8( vec4 v );
	uint      packSnorm4x8( vec4 v );

	vec2      unpackUnorm2x16( uint v );
	vec2      unpackSnorm2x16( uint v );
	vec4      unpackUnorm4x8( uint v );
	vec4      unpackSnorm4x8( uint v );
	*/

	// GL_ARB_conservative_depth
	/*
	Redeclarations of gl_FragDepth are performed as follows:

	// redeclaration that changes nothing is allowed

	out float gl_FragDepth;

	// assume it may be modified in any way
	layout (depth_any) out float gl_FragDepth;

	// assume it may be modified such that its value will only increase
	layout (depth_greater) out float gl_FragDepth;

	// assume it may be modified such that its value will only decrease
	layout (depth_less) out float gl_FragDepth;

	// assume it will not be modified
	layout (depth_unchanged) out float gl_FragDepth;
	*/

	// GL_ARB_shader_draw_parameters
	if( glConfig.shader_draw_parameters && shaderType == SHT_VERTEX )
	{
		// gl_BaseVertexARB gl_BaseInstanceARB gl_DrawID
		out += "#extension GL_ARB_shader_draw_parameters : require\n";
	}

	if( glConfig.cull_distance )
	{
		out += "#extension GL_ARB_cull_distance : require\n";
	}

	// GL_ARB_derivative_control
	if( glConfig.derivative_control && shaderType == SHT_FRAGMENT )
	{/*
	 To select the coarse derivative, use:

	 dFdxCoarse(p)
	 dFdyCoarse(p)
	 fwidthCoarse(p)

	 To select the fine derivative, use:

	 dFdxFine(p)
	 dFdyFine(p)
	 fwidthFine(p)

	 To select which ever is "better" (based on performance, API hints, or other
	 factors), use:

	 dFdx(p)
	 dFdy(p)
	 fwidth(p)

	 */
		out += "#extension GL_ARB_derivative_control : require\n";
	}

	// GL_ARB_shading_language_packing
	if( glConfig.shading_language_packing )
	{
		//out += "#extension GL_ARB_shading_language_packing : require\n";
	}

	// GL_ARB_compute_shader

	// GL_ARB_enhanced_layouts

	//out += "\n";
}

static void BuiltHeader( idShaderStringBuilder & out, shaderType_e shaderType )
{
	const char* vertexInsert_GLSL_ES_3_00 = {
		"\n"
		"precision mediump float;\n"
		//"#extension GL_ARB_gpu_shader5 : enable\n"
		"\n"
		"float saturate( const in float v ) { return clamp( v, 0.0, 1.0 ); }\n"
		"vec2 saturate( const in vec2 v ) { return clamp( v, 0.0, 1.0 ); }\n"
		"vec3 saturate( const in vec3 v ) { return clamp( v, 0.0, 1.0 ); }\n"
		"vec4 saturate( const in vec4 v ) { return clamp( v, 0.0, 1.0 ); }\n"
		//"vec4 tex2Dlod( sampler2D img, const in vec4 tc ) { return textureLod( img, tc.xy, tc.w ); }\n"
		"\n"
	};
	const char* vertexInsert_GLSL_1_50 = {
		"\n"
		"vec4 tex2Dlod( sampler2D img, const in vec4 tc ) { return textureLod( img, tc.xy, tc.w ); }\n"
		"vec4 tex3Dlod( sampler3D img, const in vec4 tc ) { return textureLod( img, tc.xyz, tc.w ); }\n"
		"vec4 texCUBElod( samplerCube img, const in vec4 tc ) { return textureLod( img, tc.xyz, tc.w ); }\n"
		"vec4 tex2DARRAYlod( sampler2DArray img, const in vec4 tc ) { return textureLod( img, tc.xyz, tc.w ); }\n"
		"vec4 texCUBEARRAYlod( samplerCubeArray img, const in vec4 tc, const in float lod ) { return textureLod( img, tc.xyzw, lod ); }\n"
		"\n"
	};
	const char* geometryInsert_GLSL_1_50 = {
		"\n"
		"vec4 tex2Dlod( sampler2D img, const in vec4 tc ) { return textureLod( img, tc.xy, tc.w ); }\n"
		"vec4 tex3Dlod( sampler3D img, const in vec4 tc ) { return textureLod( img, tc.xyz, tc.w ); }\n"
		"vec4 texCUBElod( samplerCube img, const in vec4 tc ) { return textureLod( img, tc.xyz, tc.w ); }\n"
		"vec4 tex2DARRAYlod( sampler2DArray img, const in vec4 tc ) { return textureLod( img, tc.xyz, tc.w ); }\n"
		"vec4 texCUBEARRAYlod( samplerCubeArray img, const in vec4 tc, const in float lod ) { return textureLod( img, tc.xyzw, lod ); }\n"
		"\n"
	};
	const char* fragmentInsert_GLSL_ES_3_00 = {
		"\n"
		"precision mediump float;\n"
		"precision lowp sampler2D;\n"
		"precision lowp sampler2DShadow;\n"
		"precision lowp sampler2DArray;\n"
		"precision lowp sampler2DArrayShadow;\n"
		"precision lowp samplerCube;\n"
		"precision lowp samplerCubeShadow;\n"
		"precision lowp sampler3D;\n"
		"\n"
		"void clip( const in float v ) { if ( v < 0.0 ) { discard; } }\n"
		"void clip( const in vec2 v ) { if ( any( lessThan( v, vec2( 0.0 ) ) ) ) { discard; } }\n"
		"void clip( const in vec3 v ) { if ( any( lessThan( v, vec3( 0.0 ) ) ) ) { discard; } }\n"
		"void clip( const in vec4 v ) { if ( any( lessThan( v, vec4( 0.0 ) ) ) ) { discard; } }\n"
		"\n"
		"float saturate( const in float v ) { return clamp( v, 0.0, 1.0 ); }\n"
		"vec2 saturate( const in vec2 v ) { return clamp( v, 0.0, 1.0 ); }\n"
		"vec3 saturate( const in vec3 v ) { return clamp( v, 0.0, 1.0 ); }\n"
		"vec4 saturate( const in vec4 v ) { return clamp( v, 0.0, 1.0 ); }\n"
		"\n"
		"vec4 tex2D( sampler2D img, vec2 tc ) { return texture( img, tc.xy ); }\n"
		"vec4 tex2D( sampler2DShadow img, vec3 tc ) { return vec4( texture( img, tc.xyz ) ); }\n"
		"\n"
		"vec4 tex2D( sampler2D img, vec2 tc, vec2 dx, vec2 dy ) { return textureGrad( img, tc.xy, dx, dy ); }\n"
		"vec4 tex2D( sampler2DShadow img, vec3 tc, vec2 dx, vec2 dy ) { return vec4( textureGrad( img, tc.xyz, dx, dy ) ); }\n"
		"\n"
		"vec4 texCUBE( samplerCube img, vec3 tc ) { return texture( img, tc.xyz ); }\n"
		"vec4 texCUBE( samplerCubeShadow img, vec4 tc ) { return vec4( texture( img, tc.xyzw ) ); }\n"
		"\n"
		"vec4 tex2Dproj( sampler2D img, vec3 tc ) { return textureProj( img, tc ); }\n"
		"vec4 tex3Dproj( sampler3D img, vec4 tc ) { return textureProj( img, tc ); }\n"
		"\n"
		"vec4 tex2Dbias( sampler2D img, vec4 tc ) { return texture( img, tc.xy, tc.w ); }\n"
		"vec4 tex3Dbias( sampler3D img, vec4 tc ) { return texture( img, tc.xyz, tc.w ); }\n"
		"vec4 texCUBEbias( samplerCube img, vec4 tc ) { return texture( img, tc.xyz, tc.w ); }\n"
		"\n"
		"vec4 tex2Dlod( sampler2D img, vec4 tc ) { return textureLod( img, tc.xy, tc.w ); }\n"
		"vec4 tex3Dlod( sampler3D img, vec4 tc ) { return textureLod( img, tc.xyz, tc.w ); }\n"
		"vec4 texCUBElod( samplerCube img, vec4 tc ) { return textureLod( img, tc.xyz, tc.w ); }\n"
		"\n"
	};
	const char* fragmentInsert_GLSL_1_50 = {
		"\n"
		"vec4 tex2D( sampler2D img, const in vec2 tc ) { return texture( img, tc.xy ); }\n"
		"vec4 tex2D( sampler2D img, const in vec2 tc, const ivec2 offset ) { return textureOffset( img, tc.xy, offset ); }\n"
		"vec4 tex2D( sampler2DShadow img, const in vec3 tc ) { return vec4( texture( img, tc.xyz ) ); }\n"
		"vec4 tex2DARRAY( sampler2DArray img, const in vec3 tc ) { return texture( img, tc.xyz ); }\n"
		"\n"
		"vec4 tex2D( sampler2D img, const in vec2 tc, const in vec2 dx, const in vec2 dy ) { return textureGrad( img, tc.xy, dx, dy ); }\n"
		"vec4 tex2D( sampler2DShadow img, const in vec3 tc, const in vec2 dx, const in vec2 dy ) { return vec4( textureGrad( img, tc.xyz, dx, dy ) ); }\n"
		"vec4 tex2DARRAY( sampler2DArray img, const in vec3 tc, const in vec2 dx, const in vec2 dy ) { return textureGrad( img, tc.xyz, dx, dy ); }\n"
		"\n"
		"vec4 texCUBE( samplerCube img, const in vec3 tc ) { return texture( img, tc.xyz ); }\n"
		"vec4 texCUBE( samplerCubeShadow img, const in vec4 tc ) { return vec4( texture( img, tc.xyzw ) ); }\n"
		"vec4 texCUBEARRAY( samplerCubeArray img, const in vec4 tc ) { return texture( img, tc.xyzw ); }\n"
		"\n"
		"vec4 tex2Dproj( sampler2D img, const in vec3 tc ) { return textureProj( img, tc ); }\n"
		"vec4 tex3Dproj( sampler3D img, const in vec4 tc ) { return textureProj( img, tc ); }\n"
		"\n"
		"vec4 tex2Dbias( sampler2D img, const in vec4 tc ) { return texture( img, tc.xy, tc.w ); }\n"
		"vec4 tex2Dbias( sampler2D img, const in vec4 tc, const ivec2 offset ) { return textureOffset( img, tc.xy, offset, tc.w ); }\n"
		"vec4 tex2DARRAYbias( sampler2DArray img, const in vec4 tc ) { return texture( img, tc.xyz, tc.w ); }\n"
		"vec4 tex3Dbias( sampler3D img, const in vec4 tc ) { return texture( img, tc.xyz, tc.w ); }\n"
		"vec4 texCUBEbias( samplerCube img, const in vec4 tc ) { return texture( img, tc.xyz, tc.w ); }\n"
		"vec4 texCUBEARRAYbias( samplerCubeArray img, const in vec4 tc, const in float bias ) { return texture( img, tc.xyzw, bias ); }\n"
		"\n"
		"vec4 tex2Dlod( sampler2D img, const in vec4 tc ) { return textureLod( img, tc.xy, tc.w ); }\n"
		"vec4 tex2Dlod( sampler2D img, const in vec4 tc, const ivec2 offset ) { return textureLodOffset( img, tc.xy, tc.w, offset ); }\n"
		"vec4 tex2DARRAYlod( sampler2DArray img, const in vec4 tc ) { return textureLod( img, tc.xyz, tc.w ); }\n"
		"vec4 tex2DARRAYlod( sampler2DArray img, const in vec4 tc, const ivec2 offset ) { return textureLodOffset( img, tc.xyz, tc.w, offset ); }\n"
		"vec4 tex3Dlod( sampler3D img, const in vec4 tc ) { return textureLod( img, tc.xyz, tc.w ); }\n"
		"vec4 texCUBElod( samplerCube img, const in vec4 tc ) { return textureLod( img, tc.xyz, tc.w ); }\n"
		"vec4 texCUBEARRAYlod( samplerCubeArray img, const in vec4 tc, const in float lod ) { return textureLod( img, tc.xyzw, lod ); }\n"
		"\n" //SEA: TODO GL_ARB_gpu_shader5
		"vec4 tex2DGatherR( sampler2D img, const in vec2 tc ) { return textureGather( img, tc, 0 ); }\n"
		"vec4 tex2DGatherG( sampler2D img, const in vec2 tc ) { return textureGather( img, tc, 1 ); }\n"
		"vec4 tex2DGatherB( sampler2D img, const in vec2 tc ) { return textureGather( img, tc, 2 ); }\n"
		"vec4 tex2DGatherA( sampler2D img, const in vec2 tc ) { return textureGather( img, tc, 3 ); }\n"
		"\n"
		"vec4 tex2DGatherR( sampler2D img, const in vec2 tc, const in ivec2 v0 ) { return textureGatherOffset( img, tc, v0, 0 ); }\n"
		"vec4 tex2DGatherG( sampler2D img, const in vec2 tc, const in ivec2 v0 ) { return textureGatherOffset( img, tc, v0, 1 ); }\n"
		"vec4 tex2DGatherB( sampler2D img, const in vec2 tc, const in ivec2 v0 ) { return textureGatherOffset( img, tc, v0, 2 ); }\n"
		"vec4 tex2DGatherA( sampler2D img, const in vec2 tc, const in ivec2 v0 ) { return textureGatherOffset( img, tc, v0, 3 ); }\n"
		"\n"
		"#define tex2DGatherOffsetsR( img, tc, v0, v1, v2, v3 ) textureGatherOffsets( img, tc, ivec2[]( v0, v1, v2, v3 ), 0 )\n"
		"#define tex2DGatherOffsetsG( img, tc, v0, v1, v2, v3 ) textureGatherOffsets( img, tc, ivec2[]( v0, v1, v2, v3 ), 1 )\n"
		"#define tex2DGatherOffsetsB( img, tc, v0, v1, v2, v3 ) textureGatherOffsets( img, tc, ivec2[]( v0, v1, v2, v3 ), 2 )\n"
		"#define tex2DGatherOffsetsA( img, tc, v0, v1, v2, v3 ) textureGatherOffsets( img, tc, ivec2[]( v0, v1, v2, v3 ), 3 )\n"
		"\n"
		"void clip( const in float v ) { if ( v < 0.0 ) { discard; } }\n"
		"void clip( const in vec2 v ) { if ( any( lessThan( v, vec2( 0.0 ) ) ) ) { discard; } }\n"
		"void clip( const in vec3 v ) { if ( any( lessThan( v, vec3( 0.0 ) ) ) ) { discard; } }\n"
		"void clip( const in vec4 v ) { if ( any( lessThan( v, vec4( 0.0 ) ) ) ) { discard; } }\n"
		"\n"
	};

	const char* commonInsert_GLSL_1_50 = {
		"float saturate( const in float v ) { return clamp( v, 0.0, 1.0 ); }\n"
		"vec2 saturate( const in vec2 v ) { return clamp( v, 0.0, 1.0 ); }\n"
		"vec3 saturate( const in vec3 v ) { return clamp( v, 0.0, 1.0 ); }\n"
		"vec4 saturate( const in vec4 v ) { return clamp( v, 0.0, 1.0 ); }\n"
		"\n"
		"float dot2( const in vec2 a, const in vec2 b ) { return dot( a, b ); }\n"
		"float dot3( const in vec3 a, const in vec3 b ) { return dot( a, b ); }\n"
		"float dot3( const in vec3 a, const in vec4 b ) { return dot( a, b.xyz ); }\n"
		"float dot3( const in vec4 a, const in vec3 b ) { return dot( a.xyz, b ); }\n"
		"float dot3( const in vec4 a, const in vec4 b ) { return dot( a.xyz, b.xyz ); }\n"
		"float dot4( const in vec4 a, const in vec4 b ) { return dot( a, b ); }\n"
		"float dot4( const in vec3 a, const in vec4 b ) { return dot( vec4( a, 1.0f ), b ); }\n"
		"float dot4( const in vec2 a, const in vec4 b ) { return dot( vec4( a, 0.0f, 1.0f ), b ); }\n"
		"\n"
		"vec4 tex2Dfetch( sampler2D img, const in ivec4 tc ) { return texelFetch( img, tc.st, tc.q ); }\n"
		"vec4 tex2Dfetch( sampler2D img, const in ivec4 tc, const ivec2 offsets ) { return texelFetchOffset( img, tc.st, tc.q, offsets ); }\n"
		"vec4 tex2DARRAYfetch( sampler2DArray img, const in ivec4 tc ) { return texelFetch( img, tc.stp, tc.q ); }\n"
		"vec4 tex2DARRAYfetch( sampler2DArray img, const in ivec4 tc, const ivec2 offsets ) { return texelFetchOffset( img, tc.stp, tc.q, offsets ); }\n"
		"vec4 tex2DMSfetch( sampler2DMS img, const in ivec4 tc ) { return texelFetch( img, tc.st, tc.p ); }\n"
		"vec4 tex2DMSARRAYfetch( sampler2DMSArray img, const in ivec4 tc ) { return texelFetch( img, tc.stp, tc.p ); }\n"
		"vec4 tex3Dfetch( sampler3D img, const in ivec4 tc ) { return texelFetch( img, tc.stp, tc.q ); }\n"
		"\n"
	};

	//"#pragma debug(on)\n";

	/*/*/if( shaderType == shaderType_e::SHT_VERTEX )
	{
		switch( glConfig.driverType )
		{
			case GLDRV_OPENGL_ES3:
			case GLDRV_OPENGL_MESA:
			{
				out += "#version 300 es\n";
				SetupHeaderExtensions( out, shaderType );
				out.Append( vertexInsert_GLSL_ES_3_00, idStr::Length( vertexInsert_GLSL_ES_3_00 ) );
				break;
			}

			default: {
				out += "#version 150 core\n";
				SetupHeaderExtensions( out, shaderType );
				out.Append( vertexInsert_GLSL_1_50, idStr::Length( vertexInsert_GLSL_1_50 ) );
				break;
			}
		}
	}
	else if( shaderType == shaderType_e::SHT_GEOMETRY )
	{
		switch( glConfig.driverType )
		{
			case GLDRV_OPENGL_ES3:
			case GLDRV_OPENGL_MESA:
			{
				out += "#version 300 es\n";
				//SetupHeaderExtensions( out, shaderType );
				//out.Append( geometryInsert_GLSL_ES_3_00, idStr::Length( geometryInsert_GLSL_ES_3_00 ) );
				assert( "TODO geometryInsert_GLSL_ES_3_00" );
				break;
			}

			default: {
				out += "#version 150 core\n";
				SetupHeaderExtensions( out, shaderType );
				out.Append( geometryInsert_GLSL_1_50, idStr::Length( geometryInsert_GLSL_1_50 ) );
				break;
			}
		}
	}
	else if( shaderType == shaderType_e::SHT_FRAGMENT )
	{
		switch( glConfig.driverType )
		{
			case GLDRV_OPENGL_ES3:
			case GLDRV_OPENGL_MESA:
			{
				out += "#version 300 es\n";
				SetupHeaderExtensions( out, shaderType );
				out.Append( fragmentInsert_GLSL_ES_3_00, idStr::Length( fragmentInsert_GLSL_ES_3_00 ) );
				break;
			}

			default: {
				out += "#version 150 core\n";
				SetupHeaderExtensions( out, shaderType );
				out.Append( fragmentInsert_GLSL_1_50, idStr::Length( fragmentInsert_GLSL_1_50 ) );
				break;
			}
		}
	}

	out.Append( commonInsert_GLSL_1_50, idStr::Length( commonInsert_GLSL_1_50 ) );

	///////////////////////////////////////////////////////////////////////////////
/*
	const GLchar *name = "/commonInsert";
	const GLint namelen = idStr::Length( name );

	const GLchar * string = commonInsert_GLSL_1_50;
	const GLint stringlen = idStr::Length( string );

	glNamedStringARB( GL_SHADER_INCLUDE_ARB, namelen, name, stringlen, string );
*/
}

static void SetupParms( idShaderStringBuilder & out, progParseData_t & parseData, shaderType_e shaderType )
{
	//SEA:GLSL only arrays[] of basic types can be updated with single call :(

	// Setup uniforms
	int vecCount = parseData.GetVecParmCount();
	if( vecCount && parseData.vecParmsUsed[ shaderType ].Num() )
	{
	#ifdef USE_OPENGL

		//out += "\n//Used:\n";
		out += "\n";
	  #if 0
		if( r_useProgUBO.GetBool() ) // cbuffer 'Name' : register( b0 )
		{
			if( parseData.shading_language_420pack ) {
				out += _Txt( "layout( binding = %i ) uniform parms_ubo {\n", BINDING_PROG_PARMS_UBO );
			} else {
				out += "uniform parms_ubo {\n";
			}
		}
		else {
			if( parseData.explicit_uniform_location ) {
				out += "layout( location = 0 ) uniform struct parms_t {\n";
			} else {
				out += "uniform struct parms_t {\n";
			}
		}
		for( int i = 0; i < vecCount; ++i )
		{
			auto parm = parseData.GetVecParm( i );
			out += idStrStatic<96>().Format( "\tvec4 %s;\n", parm->GetName() );
		}
		out += idStrStatic<24>().Format( "} " COMMON_PARMBLOCK_NAME ";\n" );
	  #else
		if( r_useProgUBO.GetBool() ) // cbuffer 'Name' : register( b0 )
		{
			if( glConfig.shading_language_420pack ) {
				out += _Txt( "layout( binding = %i ) uniform parms_ubo {\n", BINDING_PROG_PARMS_UBO );
			} else {
				out += "uniform parms_ubo {\n";
			}
			for( int i = 0; i < vecCount; ++i )
			{
				auto parm = parseData.GetVecParm( i );
				out += idStrStatic<96>().Format( "\tvec4 %s;\n", parm->GetName() );
			}
			out += idStrStatic<24>().Format( "} " COMMON_PARMBLOCK_NAME ";\n" );
		}
		else {
			/*int usedCount = parseData.vecParmsUsed[ shaderType ].Num();
			for( int i = 0; i < usedCount; ++i )
			{
				int index = parseData.vecParmsUsed[ shaderType ][ i ];
				auto parm = parseData.GetVecParm( index );

				out += idStrStatic<96>().Format( "// %i.%s\n", index, parm->GetName() );
			}*/
			if( glConfig.explicit_uniform_location ) {
				out += _Txt( "layout( location = 0 ) uniform vec4 " COMMON_PARMBLOCK_NAME "[%i];\n", vecCount );
			} else {
				out += _Txt( "uniform vec4 " COMMON_PARMBLOCK_NAME "[%i];\n", vecCount );
			}
		}
	  #endif

	#else // HLSL

		out += "\n";
		out += _Txt( "// cbuffer parms_ubo : register( b%i ) {\n", BINDING_PROG_PARMS_UBO );
		for( int i = 0; i < vecCount; ++i )
		{
			auto parm = parseData.GetVecParm( i );
			out += idStrStatic<96>().Format( "// \tfloat4 %s;\n", parm->GetName() );
		}
		out += idStrStatic<24>().Format( "// } " COMMON_PARMBLOCK_NAME ";\n" );

	#endif
	}

	// Setup samplers/textures
	int texCount = parseData.GetTexParmCount();
	if( texCount && parseData.texParmsUsed[ shaderType ].Num() )
	{
		out += "\n";
	#ifdef USE_OPENGL

		for( int i = 0; i < parseData.texParmsUsed[ shaderType ].Num(); ++i )
		{
			int index = parseData.texParmsUsed[ shaderType ][ i ];
			auto parm = parseData.GetTexParm( index );

			if( glConfig.shading_language_420pack ) {
				out += _Txt( "layout( binding = %i ) uniform %s %s;\n", index, GetGLSLSamplerType( parm ), parm->GetName() );
			} else {
				out += _Txt( "uniform %s %s;\n", GetGLSLSamplerType( parm ), parm->GetName() );
			}
		}

	#else // HLSL

		for( int i = 0; i < parseData.texParmsUsed[ shaderType ].Num(); ++i )
		{
			int index = parseData.texParmsUsed[ shaderType ][ i ];
			auto parm = parseData.GetTexParm( index );

			out += _Txt( "// %s _tex_%s : register( t%i );\n", GetHLSLTextureType( parm ), parm->GetName(), index );
			if( parm->GetDefaultTextureUsage() == TD_SHADOWMAP ) {
				out += _Txt( "// SamplerComparisonState _smp_%s : register( s%i );\n", parm->GetName(), index );
			} else {
				out += _Txt( "// SamplerState _smp_%s : register( s%i );\n", parm->GetName(), index );
			}
			out += _Txt( "// #define %s InitSamp( _tex_%s, _smp_%s )\n", parm->GetName(), parm->GetName(), parm->GetName() );
		}

	#endif
	}
}

static bool ParseState( idParser & src, progParseData_t & parseData )
{
	extern bool ParseGLState( idParser &, uint64 & stateBits );

	if( !src.ExpectTokenString( "{" ) )
		return false;

	idToken token;
	uint64 state = 0;
	while( !src.CheckTokenString( "}" ) )
	{
		if( !ParseGLState( src, state ) )
		{
			src.Error( "ParseGLState() fails and defaulted" );
			state = 0;
			break;
		}
	}

	parseData.glState = state;
	return true;
}

static bool ParseVariablesBlock( idParser & src, const idStr & text, progParseData_t & parseData )
{
#if 0
	idLexer lex( LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NODOLLARPRECOMPILE | LEXFL_NOBASEINCLUDES | LEXFL_NOFATALERRORS );
	lex.LoadMemory( text.c_str(), text.Length(), "variables", src.GetLineNum() );

	while( !lex.EndOfFile() )
	{
		varInOut_t var;
		idToken token;

		uint32 i = 0; // read variable type
		for(/**/; i < _countof( _typeConversion ); i++ )
		{
			if( lex.CheckTokenString( _typeConversion[ i ].typeCG ) ) {
				var.type = _typeConversion[ i ].typeGLSL;
				break;
			}
		}
		if( i == _countof( _typeConversion ) ) {
			lex.Warning("Expected variable type not found");
			return false;
		}

		// read variable name
		if( !lex.ExpectTokenType( TT_NAME, 0, token ) ) {
			return false;
		}
		var.name = token;
		if( !lex.ExpectTokenString( ";" ) ) {
			return false;
		}

		parseData.variables.AddUnique( var );
	}
#else
	if( !src.ExpectTokenString( "{" ) )
		return false;

	idToken token;
	while( !src.CheckTokenString( "}" ) )
	{
		varInOut_t var;

		uint32 i = 0; // read variable type
		for(/**/; i < _countof( _typeConversion ); i++ )
		{
			if( src.CheckTokenString( _typeConversion[ i ].typeCG ) ) {
				//var.type = _typeConversion[ i ].typeGLSL;
				var.typeIndex = i;
				break;
			}
		}
		if( i == _countof( _typeConversion ) ) {
			src.Error("Expected variable type not found");
			return false;
		}
		// read variable name
		if( !src.ExpectTokenType( TT_NAME, 0, token ) ) {
			return false;
		}
		var.name = token;
		if( !src.ExpectTokenString( ";" ) )
		{
			return false;
		}

		parseData.variables.AddUnique( var );
	}
#endif

	return true;
}

static const char * texbuff_formats[] = {
	"RGBA32F",	"RGBA16F", "RG32F", "RG16F", "R11F_G11F_B10F", "R32F", "R16F", "RGBA32UI", "RGBA16UI",
	"RGB10_A2UI", "RGBA8UI", "RG32UI", "RG16UI", "RG8UI", "R32UI", "R16UI", "R8UI", "RGBA32I", "RGBA16I",
	"RGBA8I", "RG32I", "RG16I", "RG8I", "R32I", "R16I", "R8I", "RGBA16", "RGB10_A2", "RGBA8", "RG16", "RG8",
	"R16", "R8", "RGBA16_SNORM", "RGBA8_SNORM", "RG16_SNORM", "RG8_SNORM", "R16_SNORM", "R8_SNORM",
};

/*
=====================================================================================================
  ConvertCG2GLSL
=====================================================================================================
*/
static void ConvertCG2GLSL( const idStr & in, const char* name, progParseData_t & parseData, shaderType_e shaderType )
{
	idStr & program = parseData.parseTexts[ shaderType ];
	program.ReAllocate( in.Length() * 2, false );

	idLexer src( /*LEXFL_NOFATALERRORS |*/ LEXFL_NODOLLARPRECOMPILE );
	src.LoadMemory( in.c_str(), in.Length(), name );

	bool inMain = false;
	char newline[ 128 ] = { "\n" };

	idToken token, prev_token;
	while( src.ReadToken( &token ) )
	{
		// strip 'static'
		if( token == "static" )
		{
			program += CHECK_NEW_LINE_AND_SPACES();
			src.SkipWhiteSpace( true ); // remove white space between 'static' and the next token
			continue;
		}

		if( token.is( "MAIN_BLOCK" ) )
		{
			inMain = true;
		#ifdef USE_OPENGL
			program += "\n\nvoid main()";
		#else
			/*/*/if( shaderType == SHT_VERTEX )
			{
				program += "\n\nvoid VSMain( in VS_IN input, out VS_OUT output )";
			}
			else if( shaderType == SHT_TESS_CTRL )
			{
				program += "struct TCS_OUT_CONST {\n";
				program += "\tfloat	tessLevelOuter[ 4 ]	: SV_TessFactor;\n";
				program += "\tfloat	tessLevelInner[ 2 ] : SV_InsideTessFactor;\n";
				program += "\t%s %s : %s;\n"; // custom per patch vars
				program += "};\n";

				// void PATCH_BLOCK( in InputPatch<TCS_IN, INPUT_PATCH_SIZE> input, out TCS_OUT_CONST output, uint primitiveID : SV_PrimitiveID ) {
				program += _Txt( "\n\nTCS_OUT TCSMain( in InputPatch<TCS_IN, %u> input, in uint invocationID : SV_OutputControlPointID, in uint primitiveID : SV_PrimitiveID )", ( uint32 )parseData.tessLayout.count );

				src.ReadToken( &token );
				if( token == "{" )
				{
					program += CHECK_NEW_LINE_AND_SPACES();
					program += "{\n";

					int len = idMath::Min( idStr::Length( newline ) + 1, ( int )sizeof( newline ) - 1 );
					newline[ len - 1 ] = '\t';
					newline[ len - 0 ] = '\0';

					program += CHECK_NEW_LINE_AND_SPACES();
					program += "TCS_OUT output = (TCS_OUT)0;\n";
				}
				else {
					src.Error( "Expected open brace '{' after MAIN_BLOCK not found." );
					return;
				}
			}
			else if( shaderType == SHT_TESS_EVAL )
			{
				program += _Txt( "\n\nvoid TESMain( in TCS_OUT_CONST cinput, in OutputPatch<TES_IN, %u> input, out TES_OUT output, in float3 tessCoord : SV_DomainLocation, in uint primitiveID : SV_PrimitiveID )", ( uint32 )parseData.tessLayout.count );
			}
			else if( shaderType == SHT_GEOMETRY )
			{
				if( ( uint32 )parseData.tessLayout.count > 3 )
				{
					program += _Txt( "\n\nvoid GSMain( InputPatch<GS_IN, %u> input, inout %s<GS_OUT> stream )", ( uint32 )parseData.tessLayout.count, gs_primOut[ parseData.primLayout.prim_out_type ].hlslName )
				}
				else {
					program += _Txt( "\n\nvoid GSMain( %s GS_IN input[%u], inout %s<GS_OUT> stream )",
						gs_primIn[ parseData.primLayout.prim_in_type ].hlslName,
						gs_primIn[ parseData.primLayout.prim_in_type ].count,
						gs_primOut[ parseData.primLayout.prim_out_type ].hlslName );
				}
				src.ReadToken( &token );
				if( token == "{" )
				{
					program += CHECK_NEW_LINE_AND_SPACES();
					program += "{\n";

					int len = idMath::Min( idStr::Length( newline ) + 1, ( int )sizeof( newline ) - 1 );
					newline[ len - 1 ] = '\t';
					newline[ len - 0 ] = '\0';

					program += CHECK_NEW_LINE_AND_SPACES();
					program += "GS_OUT _out = (GS_OUT)0;\n";
				}
				else {
					src.Error( "Expected open brace '{' after MAIN_BLOCK not found." );
					return;
				}
			}
			else if( shaderType == SHT_FRAGMENT )
			{
				program += "\n\nvoid PSMain( in PS_IN input, out PS_OUT output )";
			}
			else if( shaderType == SHT_COMPUTE )
			{
				// [numthreads( %d, %d, %d )]
				// void CSMain( uint3 groupID : SV_GroupID, uint3 threadID : SV_GroupThreadID, uint3 dispatchThreadID : SV_DispatchThreadID )
			}
		#endif
			continue;
		}

		// strip 'const' from local variables in main()
		if( token == "const" && inMain && !glConfig.shading_language_420pack )
		{
			program += CHECK_NEW_LINE_AND_SPACES();
			src.SkipWhiteSpace( true ); // remove white space between 'const' and the next token
			continue;
		}

		if( inMain && shaderType == SHT_TESS_CTRL )
		{
			if( token.is( "BEGIN_PATCH_BLOCK_" ) )
			{
				program += "//BEGIN_PATCH_BLOCK_"; program += "\n";
				continue;
			}
			else if( token.is( "_FINISH_PATCH_BLOCK" ) )
			{
				program += "//_FINISH_PATCH_BLOCK"; program += "\n";
				continue;
			}
		}

		// maintain indentation
		if( token == "{" )
		{
			program += CHECK_NEW_LINE_AND_SPACES();
			program += "{";

			int len = idMath::Min( idStr::Length( newline ) + 1, ( int )sizeof( newline ) - 1 );
			newline[ len - 1 ] = '\t';
			newline[ len - 0 ] = '\0';
			continue;
		}
		if( token == "}" )
		{
			int len = idMath::Max( idStr::Length( newline ) - 1, 0 );
			newline[ len ] = '\0';

			program += CHECK_NEW_LINE_AND_SPACES();
			program += "}";
			continue;
		}

		if( shaderType == SHT_FRAGMENT )
		{
			if( token == "clip" || token == "discard" ) {
				parseData.flags.SetFlag( HAS_FRAG_CLIP );
			}

			/*if( !inMain ) {
				if( token == "EARLY_FRAG_TESTS" ) {
					parseData.flags.SetFlag( HAS_FORCED_EARLY_FRAG_TESTS );
					continue;
				}
			}*/
		}
		if( shaderType == SHT_VERTEX || shaderType == SHT_GEOMETRY )
		{
			if( token.is( "matrices_ubo" ) ) {
				parseData.flags.SetFlag( HAS_HARDWARE_SKINNING );
			}
		}

		// check for a type conversion
		if( ConvertTypes( program, token, newline ) )
		{
			prev_token = token;
			continue;
		}
		// check for input/output parameters
		if( ParseInOutVars( src, program, token, newline, parseData, shaderType ) )
		{
			continue;
		}
		// check for uniforms
		if( ParseParms( src, program, token, newline, parseData, shaderType ) )
		{
			continue;
		}
		// check for a function conversion
		if( ConvertFuncs( program, token, newline ) )
		{
			prev_token = token;
			continue;
		}
		// check gs primitive layouts
		if( ParsePrimLayout( src, token, parseData, shaderType, inMain ) )
		{
			continue;
		}

	//SEA: these could be avoided by fixing Parser's precompilation whitespase issue.
	#if 1
		if( token.type == TT_PUNCTUATION )
		{
			auto pid = src.GetPunctuationId( token.c_str() );

			if( pid == P_PARENTHESESOPEN || pid == P_SQBRACKETOPEN )
			{
				if( prev_token.type == TT_NAME )
				{
					program += ( token.linesCrossed > 0 ) ? newline : "";
					program += token;
					prev_token = token;
					continue;
				}
			}
			if( pid == P_MEMBER_SELECTION_OBJECT || pid == P_COMMA || pid == P_SEMICOLON )
			{
				program += ( token.linesCrossed > 0 )? newline : "";
				program += token;
				prev_token = token;
				continue;
			}
		}
		if( token.type == TT_LITERAL || token.type == TT_NAME )
		{
			if( prev_token.type == TT_PUNCTUATION ) {
				auto pid = src.GetPunctuationId( prev_token.c_str() );
				if( pid == P_MEMBER_SELECTION_OBJECT )
				{
					program += ( token.linesCrossed > 0 ) ? newline : "";
					program += token;
					prev_token = token;
					continue;
				}
			}
		}
	#endif
		program += CHECK_NEW_LINE_AND_SPACES();
		program += token;
		prev_token = token;
	}
}

#undef CHECK_NEW_LINE_AND_SPACES

static void StripDeadCode( const char* in, const char* name, idStr & out, int baseLine, progParseData_t & parseData )
{
	if( r_skipStripDeadProgCode.GetBool() )
	{
		out = in;
		return;
	}

	idLexer src( LEXFL_NOFATALERRORS );
	src.LoadMemory( in, idStr::Length( in ), name, baseLine );

	struct idCGBlock {
		idStrStatic<64> prefix;	// tokens that comes before the name
		idStrStatic<64> name;	// the name
		idStr postfix;	// tokens that comes after the name
		//uint32 postfixChecksum;
		bool  used;		// whether or not this block is referenced anywhere
	};
	idList< idCGBlock > blocks;
	blocks.SetNum( 100 );

	idToken token;
	while( !src.EndOfFile() )
	{
		if( src.CheckTokenString( "EARLY_FRAG_TESTS" ) || src.CheckTokenString( "early_frag_tests" ) ) {
			parseData.flags.SetFlag( HAS_FORCED_EARLY_FRAG_TESTS );
			continue;
		}

		/*if( src.CheckTokenString( "UNIFORM_BLOCK" ) )
		{

		}*/

		auto & block = blocks.Alloc();

		// read prefix
		while( src.ReadToken( &token ) )
		{
			bool found = false;
			for( int i = 0; i < _countof( _prefixes ); i++ )
			{
				if( token == _prefixes[ i ] )
				{
					found = true;
					break;
				}
			}
			if( !found )
			{
				for( int i = 0; i < _countof( _types ); i++ )
				{
					if( token == _types[ i ] )
					{
						found = true;
						break;
					}
					int typeLen = idStr::Length( _types[ i ] );
					if( token.Cmpn( _types[ i ], typeLen ) == 0 )
					{
						for( int j = 0; j < _countof( _typePosts ); j++ )
						{
							if( idStr::Cmp( token.c_str() + typeLen, _typePosts[ j ] ) == 0 )
							{
								found = true;
								break;
							}
						}
						if( found )
						{
							break;
						}
					}
				}
			}
			if( found )
			{
				if( block.prefix.Length() > 0 && token.WhiteSpaceBeforeToken() )
				{
					block.prefix += ' ';
				}
				block.prefix += token;
			}
			else {
				src.UnreadToken( &token );
				break;
			}
		}
		if( !src.ReadToken( &token ) )
		{
			blocks.SetNum( blocks.Num() - 1 );
			break;
		}
		block.name = token;

		if( src.PeekTokenString( "=" ) || src.PeekTokenString( ":" ) || src.PeekTokenString( "[" ) )
		{
			src.ReadToken( &token );
			block.postfix = token;
			while( src.ReadToken( &token ) )
			{
				if( token == ";" )
				{
					block.postfix += ';';
					break;
				}
				else {
					/*if( token.WhiteSpaceBeforeToken() ) {
						block.postfix += ' ';
					}*/
					block.postfix += ( token.linesCrossed > 0 )? "\n" : ( ( token.WhiteSpaceBeforeToken() > 0 )? " " : "" );
					block.postfix += token;
				}
			}
		}
		else if( src.PeekTokenString( "(" ) )
		{
			idStr parms, body;
			src.ParseBracedSection( parms, -1, true, '(', ')' );
			if( src.CheckTokenString( ";" ) )
			{
				block.postfix = parms + ";";
			}
			else {
				src.ParseBracedSection( body, -1, true, '{', '}' );
				block.postfix = parms + " " + body;
				if( src.CheckTokenString( ";" ) )
				{
					block.postfix += ';';
				}
			}
		}
		else if( src.PeekTokenString( "{" ) )
		{
			src.ParseBracedSection( block.postfix, -1, true, '{', '}' );
			if( src.CheckTokenString( ";" ) )
			{
				block.postfix += ';';
			}
		}
		else if( src.CheckTokenString( ";" ) )
		{
			block.postfix = idStr( ';' );
		}
		else {
			src.Warning( "Could not strip dead code: unknown token '%s'\n", token.c_str() );
			out = in;
			return;
		}
	}

	idList<int, TAG_RENDERPROG> stack;
	for( int i = 0; i < blocks.Num(); ++i )
	{
		//blocks[ i ].used = ( ( blocks[ i ].name == "main" ) || blocks[ i ].name.Right( 4 ) == "_ubo" );
		blocks[ i ].used = ( !blocks[ i ].name.Icmp( "main_block" ) || blocks[ i ].name.Right( 4 ) == "_ubo" ||
			!blocks[ i ].name.Icmp( "prim_in" ) || !blocks[ i ].name.Icmp( "prim_out" ) || !blocks[ i ].name.Icmp( "prim_inout" ) ||
			!blocks[ i ].name.Icmp( "UNIFORM_BLOCK" )
		);

		/*if( blocks[ i ].name == "include" )
		{
			blocks[ i ].used = true;
			blocks[ i ].name = ""; // clear out the include tag
		}*/

		if( blocks[ i ].used )
		{
			stack.Append( i );
		}
	}

	while( stack.Num() > 0 )
	{
		int i = stack[ stack.Num() - 1 ];
		stack.SetNum( stack.Num() - 1 );

		idLexer src( LEXFL_NOFATALERRORS );
		src.LoadMemory( blocks[ i ].postfix.c_str(), blocks[ i ].postfix.Length(), name );
		while( src.ReadToken( &token ) )
		{
			for( int j = 0; j < blocks.Num(); ++j )
			{
				if( !blocks[ j ].used )
				{
					if( token == blocks[ j ].name )
					{
						blocks[ j ].used = true;
						stack.Append( j );
					}
				}
			}
		}
	}

	idShaderStringBuilder outstr;

	for( int i = 0; i < blocks.Num(); ++i )
	{
		if( blocks[ i ].used )
		{
			outstr += blocks[ i ].prefix;
			outstr += ' ';
			outstr += blocks[ i ].name;
			outstr += ' ';
			outstr += blocks[ i ].postfix;
			outstr += "\n\n";
		}
	}
	outstr.ToString( out );
}

static bool ParseShaderBlock( idParser & src, progParseData_t & parseData, const shaderType_e shaderType, shaderType_e & prev_shaderType, const char * progName, const char * defScope )
{
	parseData.BeginShader( shaderType, prev_shaderType );
	prev_shaderType = shaderType;

	idStr block;

	src.PushDefineScope( defScope );
	if( !src.AddDefine( defScope ) ) {
		src.Error( "ParseShaderBlock() !src.AddDefine( \"%s\" ) %s", defScope, progName );
		return false;
	}

	src.ExpectTokenString( "{" );
	int baseLine = 1;//src.GetLineNum();
	src.AddInclude( "global.inc" );

	block.ReAllocate( 2048, false );
	src.ParseBracedSection( block, -1, false, '{', '}' ); // Expand precompilation here.
	block.StripLeadingOnce( "{" );
	block.StripTrailingOnce( "}" );

	src.RemoveDefine( defScope );
	src.PopDefineScope();

	if( com_developer.GetBool() )
	{
		idStrStatic<MAX_OSPATH> outFilePass;
		outFilePass.Format( "renderprogs/debug/parsed_hlsl/%s%s.hlsl", progName, _glslShaderInfo[ shaderType ].shaderTypeName );
		fileSystem->WriteFile( outFilePass, block.c_str(), block.Length(), "fs_basepath" );
	}

	if( !block.IsEmpty() )
	{
		idStr shaderHLSL;
		StripDeadCode( block.c_str(), progName, shaderHLSL, baseLine, parseData );
		block.Clear();

		if( com_developer.GetBool() )
		{
			idStrStatic<MAX_OSPATH> outFilePass;
			outFilePass.Format( "renderprogs/debug/parsed_hlsl/%s_striped%s.hlsl", progName, _glslShaderInfo[ shaderType ].shaderTypeName );
			fileSystem->WriteFile( outFilePass, shaderHLSL.c_str(), shaderHLSL.Length(), "fs_basepath" );
		}

		ConvertCG2GLSL( shaderHLSL, progName, parseData, shaderType );
		return true;
	}

	src.Error("Empty %s block", _glslShaderInfo[ shaderType ].shaderTypeDeclName );
	return false;
}

static const int PROG_PARSER_FLAGS =
	//LEXFL_NOERRORS |	// don't print any errors
	//LEXFL_NOWARNINGS |	// don't print any warnings
	LEXFL_NOFATALERRORS |	// errors aren't fatal
	//LEXFL_VCSTYLEREPORTS |	// warnings and errors are reported in M$ VC style
	//LEXFL_NOSTRINGCONCAT |	// multiple strings separated by whitespaces are not concatenated
	LEXFL_NOSTRINGESCAPECHARS |	// no escape characters inside strings
	LEXFL_NODOLLARPRECOMPILE |	// don't use the $ sign for precompilation
	//LEXFL_NOBASEINCLUDES |	// don't include files embraced with < >
	//LEXFL_ALLOWPATHNAMES |	// allow path separators in names
	//LEXFL_ALLOWNUMBERNAMES |	// allow names to start with a number
	//LEXFL_ALLOWIPADDRESSES |	// allow ip addresses to be parsed as numbers
	//LEXFL_ALLOWFLOATEXCEPTIONS | 	// allow float exceptions like 1.#INF or 1.#IND to be parsed
	//LEXFL_ALLOWMULTICHARLITERALS |	// allow multi character literals
	LEXFL_ALLOWBACKSLASHSTRINGCONCAT |	// allow multiple strings seperated by '\' to be concatenated
	//LEXFL_ONLYSTRINGS |	// parse as whitespace deliminated strings (quoted strings keep quotes)
	//LEXFL_NOEMITSTRINGESCAPECHARS |	// no escape characters inside strings
	LEXFL_ALLOWRAWSTRINGBLOCKS;	// allow raw text blocks embraced with <% %>

/*
=====================================================================================================
 idDeclRenderProg::Parse

	#if defined( HLSL ) && defined( _GS_ )
		#define TRIANGLE_INPUT 		triangle GS_IN input[3]
		#define POINT_INPUT 		point GS_IN input[1]
		#define LINE_INPUT 			line GS_IN input[2]
		#define LINEADJ_INPUT 		lineadj GS_IN input[4]
		#define TRIANGLEADJ_INPUT 	triangleadj GS_IN input[6]

		#define TRIANGLE_OUTPUT 	out TriangleStream<GS_OUT> stream
		#define POINT_OUTPUT 		out PointStream<GS_OUT> stream
		#define LINE_OUTPUT 		out LineStream<GS_OUT> stream

		#define FinishVertex( x ) stream.Append( x )
		#define FinishPrimitive() stream.RestartStrip()

		//SEA add this on top of main():
		#define BeginVertex() GS_OUT output = (GS_OUT)0
	#endif
=====================================================================================================
*/
bool idDeclRenderProg::Parse( const char* text, const int textLength, bool allowBinaryVersion )
{
/*
	if( cvarSystem->GetCVarBool( "fs_buildresources" ) ) {
		fileSystem->AddProgPreload( GetName() );
	}

	if( allowBinaryVersion )
	{
		// Try to load the generated version of it
		// If successful,
		// - Create an MD5 of the hash of the source
		// - Load the MD5 of the generated, if they differ, create a new generated
		idStrStatic< MAX_OSPATH > generatedFileName;
		generatedFileName = "generated/renderprogs/glsl/";
		generatedFileName.AppendPath( GetName() );
		generatedFileName.SetFileExtension( ".bprog" );

		idFileLocal file( fileSystem->OpenFileReadMemory( generatedFileName ) );
		uint32 sourceChecksum = idHashing::MD5_BlockChecksum( text, textLength );

		if( binaryLoadPrograms.GetBool() && LoadBinary( file, sourceChecksum ) )
		{
			return true;
		}
	}
*/
	/////////////////////////////////////////////////////////////////////////////
	try
	{
		idParser src( PROG_PARSER_FLAGS );
		src.LoadMemory( text, textLength, GetName(), GetLineNum() );
		if( !src.IsLoaded() )
		{
			///common->Error( "idParser:: couldn't load %s", GetName() );
			throw idProgException( _Txt( "idParser::LoadMemory() fail for %s", GetName() ));
			///return false;
		}
		src.SetIncludePath( PROG_INCLUDE_PATH );

		if( !src.SkipUntilString( "{" ) )
			throw idProgException( "idParser::SkipUntilString(\"{\") Fail" );

		/////////////////////////////////////////////////////////////

	#ifdef ID_PC //SEA: TODO fix global define code!
		src.AddDefine( "PC" );

		if( glConfig.driverType == GLDRV_OPENGL_ES3 ) {
			src.AddDefine( "GLES" );
		} else {
			src.AddDefine( "GLSL" );
		}

		if( glConfig.vendor == VENDOR_NVIDIA ) {
			src.AddDefine( "_NVIDIA_" );
		}
		else if( glConfig.vendor == VENDOR_AMD ) {
			src.AddDefine( "_AMD_" );
		}
		else if( glConfig.vendor == VENDOR_INTEL ) {
			src.AddDefine( "_INTEL_" );
		}

		if( r_useProgUBO.GetBool() ) {
			src.AddDefine( "USE_UBO_PARMS" );
		}

		if( r_useHalfLambertLighting.GetBool() ) {
			src.AddDefine( "USE_HALF_LAMBERT" );
		}
		if( r_useHDR.GetBool() ) {
			src.AddDefine( "USE_LINEAR_RGB" );
		}
		src.AddDefine( "USE_YCOCG" );
		src.AddDefine( "SKINNING_MAT_COUNT 408" );

		// Extensions ------------------------

		if( glConfig.gpu_shader5 ) // HLSL Shader Model 5
		{
			src.AddDefine( "GS_INSTANCED" ); //  Instance a Geometry Shader, Index Multiple Output Streams
			src.AddDefine( "TEXTURE_GATHER" );
			// Tessshaders
			// Coverage as PS Input
			// Programmable Interpolation of Inputs - The pixel shader can evaluate attributes within the pixel, anywhere on the multisample grid
			// Compute shader
			// conservative oDepth
			// Addressable Stream output / Increase Stream output count to 4 / Change all stream output buffers to be multi-element

			// Doubles with denorms
			// Count bits set instruction
			// Find first bit set instruction
			// Carry / Overflow handling
			// Bit reversal instructions for FFTs
			// Conditional Swap intrinsic
			// Resinfo on buffers
			// Reduced - precision reciprocal
			// Shader conversion instructions - fp16 to fp32 and vice versa
			// Structured buffer, which is a new type of buffer containing structured elements.


		}
		if( glConfig.cull_distance ) src.AddDefine( "CULL_DISTANCE" );
		if( glConfig.vertex_shader_layer && glConfig.vertex_shader_viewport_index ) src.AddDefine( "GLSL_VS_LAYER_VIEWPORT" );
		if( glConfig.fragment_layer_viewport ) src.AddDefine( "GLSL_FS_LAYER_VIEWPORT" );
		if( glConfig.shader_draw_parameters ) src.AddDefine( "GLSL_DRAW_PARMS" );
		if( glConfig.texture_cube_map_array ) src.AddDefine( "CUBE_MAP_ARRAY" );
		if( glConfig.sample_shading ) src.AddDefine( "SAMPLE_SHADING" );

	#else
		#error Non PC world. Implement Me!
	#endif

		shaderType_e prev_shaderType = SHT_VERTEX;
		progParseData_t	parseData;

		// support for globals.inc
		//parseData.dependencies.SetGranularity( 1 );
		//parseData.dependencies.SetNum( 1 );

		/*if( parseData.explicit_uniform_location ) {
			src.AddDefine( "EXPLICIT_LOCATIONS" );
		}
		if( parseData.shading_language_420pack ) {
			src.AddDefine( "EXPLICIT_BINDINGS" );
		}*/

		if( src.CheckTokenString( "parms" ) )
		{
			src.SkipBracedSection();
			//src.AddDefine( "_PARM_" );
			//idStrStatic<2048> text;
			//src.ParseBracedSection( text );
			//src.RemoveDefine( "_PARM_" );

			//ParsePreParms( parseData );
		}

		if( src.CheckTokenString( "state" ) )
		{
			//src.SkipBracedSection();

			//src.AddDefine( "_STATE_" );
			//src.PushDefineScope( "_STATE_" );
			//idStrStatic<512> text;
			//src.ParseBracedSection( text );
			//src.RemoveDefine( "_STATE_" );
			//src.PopDefineScope();

			if( !src.ExpectTokenString( "{" ) )
				throw idProgException( _Txt( "Parse general state block error for '%s'", GetName() ) );

			idToken token;
			uint64 state = 0;
			while( !src.CheckTokenString( "}" ) )
			{
				if( !idRenderStateParse::ParseGLState( src, state ) )
				{
					src.Error( "ParseGLState() fails and defaulted" );
					state = 0;
					break;
				}
			}
			parseData.glState = state;
		}

		if( src.CheckTokenString( "variables" ) )
		{
		#if 0
			idStrStatic<1024> block;
			src.AddDefine("_VARS_");
			src.ParseBracedSection( block, -1, '{', '}', true );
			src.RemoveDefine("_VARS_");
			if( !block.IsEmpty() )
			{
				if( !ParseVariablesBlock( src, block, parseData ) )
				{
					src.Error("ParseVariablesBlock() fail");
					return false;
				}
			}
		#elif 1
			idStr block;
			if( !ParseVariablesBlock( src, block, parseData ) )
			{
				throw idProgException( "ParseVariablesBlock() fail" );
				///return false;
			}
		#else
			src.SkipBracedSection();
		#endif
		}

		if( src.CheckTokenString( "vertexShader" ) )
		{
			ParseShaderBlock( src, parseData, SHT_VERTEX, prev_shaderType, GetName(), "_VS_" );
		}

		if( src.CheckTokenString( "tessCtrlShader" ) )
		{
			src.SkipBracedSection();
		}

		if( src.CheckTokenString( "tessEvalShader" ) )
		{
			src.SkipBracedSection();
		}

		if( src.CheckTokenString( "geometryShader" ) )
		{
			ParseShaderBlock( src, parseData, SHT_GEOMETRY, prev_shaderType, GetName(), "_GS_" );
		}

		if( src.CheckTokenString( "fragmentShader" ) )
		{
			ParseShaderBlock( src, parseData, SHT_FRAGMENT, prev_shaderType, GetName(), "_FS_" );
		}

		if( src.CheckTokenString( "computeShader" ) )
		{
			src.SkipBracedSection();
			//src.AddDefine( "_CS_" );
			//src.RemoveDefine( "_CS_" );
			//ParseShader( parseData );
		}

		if( !src.ExpectTokenString( "}" ) )
		{
			//common->Warning( "unknown program's general block '%s' in '%s'", token.c_str(), GetName() );
			//return false;
			throw idProgException(_Txt( "Parse general block error for '%s'", GetName() ));
		}

		////////////////////////////////////////////////
	#if 0
		idStrList dependencies;
		ID_TIME_T dependenciesTimestamp;
		const char * inc_name = nullptr;
		int inc_Count = 0;
		do {
			inc_name = src.GetNextDependency( inc_Count );
			if( inc_name )
			{
				common->DPrintf( "---include< %s >\n", inc_name );

				idStrStatic<128> name( inc_name );

				dependenciesTimestamp += fileSystem->GetTimestamp( inc_name );

				name.StripLeadingOnce( PROG_INCLUDE_PATH );

				auto & str = dependencies.Alloc();
				str = name;
			}
		} while( inc_name );
	#endif
		//////////////////////////////////////////////////////////////////////////
		// Save off dependencies
		parseData.dependencies.Clear();
		const char * inc_name = NULL;
		int inc_Count = 0;
		do {
			inc_name = src.GetNextDependency( inc_Count );
			if( inc_name ) {
				auto str = parseData.dependencies.Alloc(); str->Clear(); *str = inc_name;
				auto ts = fileSystem->GetTimestamp( inc_name );
				parseData.dependenciesTimestamp += ts;
				///common->DPrintf("--- include: \"%s\" %i, %i\n", str->c_str(), ts, inc_Count );
			}
		} while( inc_name != NULL );
		//////////////////////////////////////////////////////////////////////////

		src.FreeSource();

		if( !renderSystem->IsRenderDeviceRunning() )
		{
			idLib::Error( "idDeclRenderProg::Parse() called befor render initialization" );
			return false;
		}

		// Build shader text
		for( int i = 0; i < parseData.parseTexts.Num(); i++ )
		{
			if( parseData.parseTexts[ i ].IsEmpty() )
				continue;

			idShaderStringBuilder gatherGLSL;

			// Format name and type
			gatherGLSL += _Txt( "// %s%s\n", GetName(), _glslShaderInfo[ i ].shaderTypeName );

			BuiltHeader( gatherGLSL, ( shaderType_e )i );

			SetupLayouts( gatherGLSL, parseData, ( shaderType_e )i );
			SetupInOutVars( gatherGLSL, parseData, ( shaderType_e )i );
			SetupParms( gatherGLSL, parseData, ( shaderType_e )i );

			gatherGLSL += "\n";
			gatherGLSL += parseData.parseTexts[ i ];
			parseData.parseTexts[ i ].Clear();

			idStr shaderGLSL;
			gatherGLSL.ToString( shaderGLSL );
			gatherGLSL.Clear();

			if( com_developer.GetBool() )
			{
				idStrStatic<MAX_OSPATH> outFilePass;
				outFilePass.Format( "renderprogs/debug/parsed_glsl/%s%s.glsl", GetName(), _glslShaderInfo[ i ].shaderTypeName );
				fileSystem->WriteFile( outFilePass.c_str(), shaderGLSL.c_str(), shaderGLSL.Length(), "fs_basepath" );
			}

			///uint32 checksum = idHashing::MD5_BlockChecksum( shaderGLSL.c_str(), shaderGLSL.Length() );

			parseData.glslProgram.shaderObjects[ i ] = GL_CreateGLSLShaderObject( ( shaderType_e )i, shaderGLSL.c_str(), GetName() );
			if( !parseData.glslProgram.shaderObjects[ i ] )
			{
				shaderGLSL.Clear();
				///idLib::Error( "GLSL %s compile error in '%s'", _glslShaderInfo[ i ].shaderTypeDeclName, GetName() );
				throw idProgException( _Txt( "GLSL %s compile error in '%s'", _glslShaderInfo[ i ].shaderTypeDeclName, GetName() ));
				///return false;
			}
			shaderGLSL.Clear();
		}

		GL_CreateGLSLProgram( parseData, GetName(), com_developer.GetBool() );

		// copy glsl program data
		*( ( glslProgram_t *)apiObject ) = parseData.glslProgram;

		// copy decl prog data
		{
		#if 0
			data.dependencies.SetNum( src.GetDependenciesCount() );
			const char * inc_name = nullptr;
			int inc_Count = 0;
			do {
				inc_name = src.GetNextDependency( inc_Count );
				if( inc_name )
				{
					data.dependencies[ inc_Count - 1 ] = inc_name;
					data.dependenciesTimestamp += fileSystem->GetTimestamp( inc_name );
				}
			} while( inc_name );
		#else
			data.dependencies.SetNum( parseData.dependencies.Num() );
			for( int i = 0; i < parseData.dependencies.Num(); i++ ) {
				data.dependencies[ i ] = parseData.dependencies[ i ];
			}
			data.dependenciesTimestamp = parseData.dependenciesTimestamp;
		#endif
			data.glState = parseData.glState;
			data.flags = parseData.flags;
			data.vertexMask = parseData.vertexMask;

			data.vecParms.SetNum( parseData.GetVecParmCount() );
			for( int i = 0; i < data.vecParms.Num(); i++ ) { data.vecParms[ i ] = parseData.GetVecParm( i ); }

			data.texParms.SetNum( parseData.GetTexParmCount() );
			for( int i = 0; i < data.texParms.Num(); i++ ) { data.texParms[ i ] = parseData.GetTexParm( i ); }
		}
	}
	catch( idProgException & ex )
	{
		/*if( com_developer.GetBool() )
		{
			common->Error( ex.GetError() );
			return false;
		}*/

		//SEA: tofix! sometimes stack overflow happenes :(

		common->Warning( ex.GetError() );
		MakeDefault();
		return false;
	}
	return true;
}

/*
===========================================
idDeclRenderProg::idDeclRenderProg
===========================================
*/
idDeclRenderProg::idDeclRenderProg()
{
	apiObject = new( TAG_RENDERPROG ) glslProgram_t();

	Clear();
}
/*
===========================================
idDeclRenderProg::~idDeclRenderProg
===========================================
*/
idDeclRenderProg::~idDeclRenderProg()
{
	if( IsValidAPIObject() )
	{
		auto prog = ( glslProgram_t * )GetAPIObject();
		prog->FreeProgObject();

		delete apiObject;
		apiObject = nullptr;
	}

	Clear();
}

/*
===========================================
idDeclRenderProg::Clear
===========================================
*/
void idDeclRenderProg::Clear()
{
	data.Clear();
}

/*
===========================================
idDeclRenderProg::FreeData
===========================================
*/
void idDeclRenderProg::FreeData()
{
	if( IsValidAPIObject() )
	{
		auto prog = ( glslProgram_t * )GetAPIObject();
		prog->FreeProgObject();
	}

	Clear();
}

/*
===========================================
idDeclRenderProg::Size
===========================================
*/
size_t idDeclRenderProg::Size() const
{
	size_t size = idDecl::Size();

	size += data.Size();

	size += sizeof( apiObject );
	if( IsValidAPIObject() ) {
		size += (( glslProgram_t * )GetAPIObject())->Size();
	}

	return size;
}

/*
===========================================
idDeclRenderProg::Print
===========================================
*/
void idDeclRenderProg::Print() const
{
	common->Printf( "Prog: %s, File: %s, Size: %u\n", GetName(), GetFileName(), Size() );

	if( data.dependencies.Num() )
	{
		common->Printf( "includes:\n" );
		for( int i = 0; i < data.dependencies.Num(); i++ )
		{
			common->Printf( "\t%s\n", data.dependencies[ i ].c_str() );
		}
	}

	if( !data.flags.IsEmpty() )
	{
		common->Printf( "options:\n" );
		if( HasFlag( HAS_COVERAGE_OUTPUT ) ) common->Printf( "\thasCoverageOutput\n" );
		if( HasFlag( HAS_STENCIL_OUTPUT ) ) common->Printf( "\thasStencilRefOutput\n" );
		if( HasFlag( HAS_HARDWARE_SKINNING ) ) common->Printf( "\thasHardwareSkinning\n" );
		if( HasFlag( HAS_OPTIONAL_SKINNING ) ) common->Printf( "\thasOptionalSkinning\n" );
		if( HasFlag( HAS_FRAG_CLIP ) ) common->Printf( "\thasFragClip\n" );
		if( HasFlag( HAS_FRAG_OUTPUT ) ) common->Printf( "\thasFragOutput\n" );
		if( HasFlag( HAS_MRT_OUTPUT ) ) common->Printf( "\thasMRT\n" );
		if( HasFlag( HAS_DEPTH_OUTPUT ) ) common->Printf( "\thasDepthOutput\n" );
		if( HasFlag( HAS_FORCED_EARLY_FRAG_TESTS ) ) common->Printf( "\thasForcedEarlyFragmentTests\n" );
	}

	if( !data.vertexMask.IsEmpty() )
	{
		common->Printf( "vertex attributes:\n" );
		for( uint32 i = 0; i < _countof( _vertexAttribsPC ); i++ )
		{
			if( data.vertexMask.HasFlag( BIT( _vertexAttribsPC[ i ].bind ) ) )
			{
				common->Printf( "\t%s\n", _vertexAttribsPC[ i ].glsl );
			}
		}
	}

	if( GetVecParms().Num() )
	{
		common->Printf( "vectors:\n" );
		for( uint32 i = 0; i < GetVecParms().Num(); i++ )
		{
			common->Printf( "\t%s\n", GetVecParms()[ i ]->GetName() );
		}
	}

	if( GetTexParms().Num() )
	{
		common->Printf( "textures:\n" );
		for( uint32 i = 0; i < GetTexParms().Num(); i++ )
		{
			common->Printf( "\t%s\n", GetTexParms()[ i ]->GetName() );
		}
	}
}

/*
===========================================
idDeclRenderProg::List
===========================================
*/
void idDeclRenderProg::List() const
{

}

/*
===========================================
idDeclRenderProg::DefaultDefinition
===========================================
*/
const char * idDeclRenderProg::DefaultDefinition() const {
return
"{ // DefaultDefinition\n\
	state { depthFunc LEQUAL }\n\
	vertexShader { main_block {\n\
		output.position.x = dot4( input.position, $MVPmatrixX );\n\
		output.position.y = dot4( input.position, $MVPmatrixY );\n\
		output.position.z = dot4( input.position, $MVPmatrixZ );\n\
		output.position.w = dot4( input.position, $MVPmatrixW );\n\
	}}\n\
	fragmentShader { main_block {\n\
		output.hcolor = float4( 0.2, 1.0, 0.4, 1.0 );\n\
	}}\n\
}\n";
}

/*
===========================================
idDeclRenderProg::SourceFileChanged
===========================================
*/
bool idDeclRenderProg::SourceFileChanged() const
{
	if( idDecl::SourceFileChanged() )
		return true;

	ID_TIME_T dependenciesTimestamp = 0;
	for( int i = 0; i < data.dependencies.Num(); i++ ) {
		dependenciesTimestamp += fileSystem->GetTimestamp( data.dependencies[ i ] );
		assert( dependenciesTimestamp > 0 );
	}
	if( data.dependenciesTimestamp != dependenciesTimestamp )
		return true;

	return false;
}


/*
===========================================
 ProgUsesParm
===========================================
*/
bool idDeclRenderProg::ProgUsesParm( const idDeclRenderParm * parm ) const
{
	assert( parm != nullptr );

	for( int i = 0; i < GetVecParms().Num(); i++ )
	{
		if( GetVecParms()[ i ] == parm )
		{
			return true;
		}
	}
	for( int i = 0; i < GetTexParms().Num(); i++ )
	{
		if( GetTexParms()[ i ] == parm )
		{
			return true;
		}
	}
	return false;
}


struct IncludeEntry {
	idStr   name;
	idStr   filename;
	idStr   content;
};
typedef idList<IncludeEntry> IncludeRegistry;
IncludeRegistry m_includes;
void GL_RegisterInclude( idStr const & name, idStr const & filename, idStr const & content )
{
	IncludeEntry entry;
	entry.name = name;	// <name> must begin with the character '/'
	entry.filename = filename;
	entry.content = content;

	m_includes.Append( entry );

	glNamedStringARB( GL_SHADER_INCLUDE_ARB, name.Length(), name.c_str(), content.Length(), content.c_str() );
}


/*
===========================================
 GL_CreateGLSLShaderObject
===========================================
*/
static GLuint GL_CreateGLSLShaderObject( shaderType_e shaderType, const char * sourceGLSL, const char * fileName )
{
	// create and compile the shader
	const GLuint shader = glCreateShader( _glslShaderInfo[ shaderType ].glTargetType );
	if( !shader ) {
		idLib::Warning( "Fail to create GLSL shader object for %s", fileName );
		return GL_ZERO;
	}

	const char* source[ 1 ] = { sourceGLSL };
	glShaderSource( shader, 1, source, NULL );

	//GLsizei count;
	//const GLchar * incPaths[1];
	//idList<const GLchar *> incPaths;
	//glCompileShaderIncludeARB( shader, incPaths.Num(), incPaths.Ptr(), NULL );
	glCompileShader( shader );

	GLint compiled = GL_FALSE;
	glGetShaderiv( shader, GL_COMPILE_STATUS, &compiled );
	if( compiled == GL_FALSE )
	{
		int infologLength = 0;
		glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &infologLength );
		if( infologLength > 1 )
		{
			idTempArray<char> infoLog( infologLength );
			glGetShaderInfoLog( shader, infologLength, NULL, infoLog.Ptr() );

			idLib::Printf( S_COLOR_ORANGE "While compiling %s program %s" S_COLOR_DEFAULT "\n", _glslShaderInfo[ shaderType ].shaderTypeName, fileName );

			const char separator = '\n';
			idList<idStr> lines;
			lines.Clear();
			lines.Append( idStr( sourceGLSL ) );
			for( int index = 0, ofs = lines[ index ].Find( separator ); ofs != -1; index++, ofs = lines[ index ].Find( separator ) )
			{
				lines.Append( lines[ index ].c_str() + ofs + 1 );
				lines[ index ].CapLength( ofs );
			}

			idLib::Printf( S_COLOR_ORANGE "---------------------" S_COLOR_DEFAULT "\n" );
			for( int i = 0; i < lines.Num(); ++i )
			{
				idLib::Printf( S_COLOR_LT_GREY "%3d: %s" S_COLOR_DEFAULT "\n", i + 1, lines[ i ].c_str() );
			}
			idLib::Printf( S_COLOR_ORANGE "---------------------" S_COLOR_DEFAULT "\n" );
			idLib::Printf( S_COLOR_ORANGE "%s" S_COLOR_DEFAULT "\n", infoLog.Ptr() );
		}

		if( GLEW_ARB_ES2_compatibility ) {
			glReleaseShaderCompiler();
		}
		glDeleteShader( shader );
		return GL_ZERO;
	}

	if( GLEW_ARB_ES2_compatibility ) {
		glReleaseShaderCompiler();
		//glShaderBinary( sizei count, const uint *shaders, enum binaryformat, const void *binary, sizei length );
	}

	return shader;
}

/*
===========================================
 GL_CreateGLSLProgram
===========================================
*/
void GL_CreateGLSLProgram( progParseData_t & parseData, const char * progName, bool bRetrieveBinary )
{
	assert( progName != nullptr );

	auto & shaderObjects = parseData.glslProgram.shaderObjects;

	if( !shaderObjects[ SHT_VERTEX ] || !shaderObjects[ SHT_FRAGMENT ] )
	{
		///idLib::Error( "Fail to create GLSL program VS:%u, FS:%u ( VS&FS must be valid )", shaderObjects[ SHT_VERTEX ], shaderObjects[ SHT_FRAGMENT ] );
		throw idProgException(_Txt( "Fail to create GLSL program for '%s' VS:%u, FS:%u ( VS&FS must be valid )", progName, shaderObjects[ SHT_VERTEX ], shaderObjects[ SHT_FRAGMENT ] ));
		///return;
	}

	const GLuint program = glCreateProgram();
	if( !program )
	{
		idLib::Error( "Fail to create GLSL program object for %s", progName );
		return;
	}
	if( GLEW_KHR_debug )
	{
		idStrStatic<128> name;
		name.Format( "GLSLProg( %u, %s )", program, progName );
		glObjectLabel( GL_PROGRAM, program, name.Length(), name.c_str() );
	}

	for( int i = 0; i < shaderObjects.Num(); i++ )
	{
		if( shaderObjects[ i ] ) {
			glAttachShader( program, shaderObjects[ i ] );
		}
	}

	// Bind input / output attributes locations.
	if( !glConfig.explicit_attrib_location )
	{
		// bind vertex input locations
		for( int i = 0; i < parseData.vsInputs.Num(); i++ )
		{
			auto & var = parseData.vsInputs[ i ];
			glBindAttribLocation( program, var.bind, var.glsl );
		}
		parseData.vsInputs.Clear();

		// bind fragment output locations
		for( int i = 0; i < parseData.fsOutputs.Num(); i++ )
		{
			auto & var = parseData.fsOutputs[ i ];
			glBindFragDataLocation( program, var.bind, var.glsl );
		}
		parseData.fsOutputs.Clear();

		// bind transform feedback attributes
		/*for( size_t i = 0; i < 0; i++ ) {
			const char* varyings[ 3 ] = { "streamVar0", "streamVar1", "streamVar2" }; // gl_NextBuffer gl_SkipComponents#(1 2 3 4)
			glTransformFeedbackVaryings( program, _countof( varyings ), varyings, GL_INTERLEAVED_ATTRIBS ); // GL_SEPARATE_ATTRIBS
		}*/
	}

	if( bRetrieveBinary )
	{
		glProgramParameteri( program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE );
	}

	GLint linked = GL_FALSE;
	glLinkProgram( program );
	glGetProgramiv( program, GL_LINK_STATUS, &linked );
	if( linked == GL_FALSE )
	{
		int infologLength = 0;
		glGetProgramiv( program, GL_INFO_LOG_LENGTH, &infologLength );
		if( infologLength > 1 )
		{
			idTempArray<char> infoLog( infologLength );
			glGetProgramInfoLog( program, infologLength, NULL, infoLog.Ptr() );

			idLib::Printf( S_COLOR_LT_GREY "Linking GLSL prog:%u with vs:%u, ts1:%u, ts2:%u, gs:%u, fs:%u Fail. InfoLog:" S_COLOR_DEFAULT "\n",
				program,
				shaderObjects[ shaderType_e::SHT_VERTEX ],
				shaderObjects[ shaderType_e::SHT_TESS_CTRL ],
				shaderObjects[ shaderType_e::SHT_TESS_EVAL ],
				shaderObjects[ shaderType_e::SHT_GEOMETRY ],
				shaderObjects[ shaderType_e::SHT_FRAGMENT ]
			);
			idLib::Printf( S_COLOR_LT_GREY "%s" S_COLOR_DEFAULT "\n", infoLog.Ptr() );
		}

		glDeleteProgram( program );
		throw idProgException(_Txt( "Failed linkage ( Prog:%u Name:%s )\n", program, progName ));
		///return;
	}

	GLint valid = GL_FALSE;
	glValidateProgram( program );
	glGetProgramiv( program, GL_VALIDATE_STATUS, &valid );
	if( valid == GL_FALSE )
	{
		GLint infologLength = 0;
		glGetProgramiv( program, GL_INFO_LOG_LENGTH, &infologLength );
		if( infologLength > 1 )
		{
			// The maxLength includes the NULL character
			idTempArray<char> infoLog( infologLength );
			glGetProgramInfoLog( program, infologLength, NULL, infoLog.Ptr() );

			idLib::Printf( S_COLOR_LT_GREY "Validating GLSL prog:%u ( vs:%u, ts1:%u, ts2:%u, gs:%u, fs:%u ) Fail. InfoLog:" S_COLOR_DEFAULT "\n",
				program,
				shaderObjects[ shaderType_e::SHT_VERTEX ],
				shaderObjects[ shaderType_e::SHT_TESS_CTRL ],
				shaderObjects[ shaderType_e::SHT_TESS_EVAL ],
				shaderObjects[ shaderType_e::SHT_GEOMETRY ],
				shaderObjects[ shaderType_e::SHT_FRAGMENT ]
			);
			idLib::Printf( S_COLOR_LT_GREY "%s" S_COLOR_DEFAULT "\n", infoLog.Ptr() );
		}

		glDeleteProgram( program );
		throw idProgException(_Txt( "Failed validation ( Prog:%u Name:%s )\n", program, progName ));
		///return;
	}

	if( bRetrieveBinary )
	{
		GLint binaryLength = 0;
		GLenum binaryFormat = 0;
		glGetProgramiv( program, GL_PROGRAM_BINARY_LENGTH, &binaryLength );
		if( binaryLength )
		{
			void * binaryData = _alloca( binaryLength );
			glGetProgramBinary( program, binaryLength, NULL, &binaryFormat, binaryData );

			idStrStatic<MAX_OSPATH> outFilePass;
			outFilePass.Format( "renderprogs/debug/parsed_glsl_bin/%s.glsl_bin", progName );
			fileSystem->WriteFile( outFilePass.c_str(), binaryData, binaryLength, "fs_basepath" );

			parseData.glslProgram.binaryLength = binaryLength;
			parseData.glslProgram.binaryFormat = binaryFormat;
			//parseData.glslProgram.binaryData = binaryData;
		}
	}

	// Always detach shaders after a successful link.
	for( int i = 0; i < shaderObjects.Num(); i++ )
	{
		if( shaderObjects[ i ] ) {
			glDetachShader( program, shaderObjects[ i ] );
			glDeleteShader( shaderObjects[ i ] );
			shaderObjects[ i ] = GL_NONE;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////

	// !parseData.shading_language_420pack
	extern void BindUniformBuffers( GLuint );
	BindUniformBuffers( program );

	if( parseData.GetVecParmCount() )
	{
		if( r_useProgUBO.GetBool() )
		{
			if( !glConfig.shading_language_420pack )
			{
				// get the uniform buffer binding for prog parameters
				GLint blockIndex = glGetUniformBlockIndex( program, "parms_ubo" );
				if( blockIndex != -1 ) {
					glUniformBlockBinding( program, blockIndex, BINDING_PROG_PARMS_UBO );
				}
			}
		}
		else {
			//idStrStatic<128> name;
			//name.Format( COMMON_PARMBLOCK_NAME ".%s", parseData.GetVecParm( 0 )->GetName() );
			//parseData.glslProgram.parmBlockIndex = glGetUniformLocation( program, name.c_str() );
			parseData.glslProgram.parmBlockIndex = glGetUniformLocation( program, COMMON_PARMBLOCK_NAME );
		}
	}

	// set the texture unit locations once for the render program.
	// We only need to do this once since we only link the program once.
	if( !glConfig.shading_language_420pack )
	{
		glUseProgram( program );
		for( int i = 0; i < parseData.GetTexParmCount(); ++i )
		{
			auto parm = parseData.GetTexParm( i );
			GLint loc = glGetUniformLocation( program, parm->GetName() );
			if( loc != -1 ) {
				glUniform1i( loc, i );
			}
		}
		glUseProgram( GL_NONE );
	}

	parseData.glslProgram.programObject = program;
}

#undef _Txt
