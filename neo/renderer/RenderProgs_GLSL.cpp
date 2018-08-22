/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2015 Robert Beckebans

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

idCVar r_skipStripDeadCode( "r_skipStripDeadCode", "0", CVAR_BOOL, "Skip stripping dead code" );

// DG: the AMD drivers output a lot of useless warnings which are fscking annoying, added this CVar to suppress them
idCVar r_displayGLSLCompilerMessages( "r_displayGLSLCompilerMessages", "1", CVAR_BOOL | CVAR_ARCHIVE, "Show info messages the GPU driver outputs when compiling the shaders" );
// DG end

// RB begin
idCVar r_alwaysExportGLSL( "r_alwaysExportGLSL", "0", CVAR_BOOL, "allways write glsl source files" );
// RB end

#define CHECK_NEW_LINE_AND_SPACES() ( token.linesCrossed > 0 )? newline : (( token.WhiteSpaceBeforeToken() > 0 )? " " : "" )

#define VERTEX_UNIFORM_ARRAY_NAME			"_va_"
#define TESS_CONTROL_UNIFORM_ARRAY_NAME		"_tca_"
#define TESS_EVALUATION_UNIFORM_ARRAY_NAME	"_tea_"
#define GEOMETRY_UNIFORM_ARRAY_NAME			"_ga_"
#define FRAGMENT_UNIFORM_ARRAY_NAME			"_fa_"
#define COMPUTE_UNIFORM_ARRAY_NAME			"_ca_"

static const int AT_VS_IN = BIT( 1 );
static const int AT_VS_OUT = BIT( 2 );

static const int AT_TCS_IN = BIT( 3 );
static const int AT_TCS_OUT = BIT( 4 );

static const int AT_TES_IN = BIT( 5 );
static const int AT_TES_OUT = BIT( 6 );

static const int AT_GS_IN = BIT( 7 );
static const int AT_GS_OUT = BIT( 8 );

static const int AT_PS_IN = BIT( 9 );
static const int AT_PS_OUT = BIT( 10 );

static const int AT_CS_IN = BIT( 11 );
static const int AT_CS_OUT = BIT( 12 );

static const int AT_VS_IN_RESERVED = BIT( 13 );
static const int AT_VS_OUT_RESERVED = BIT( 14 );

static const int AT_TCS_IN_RESERVED = BIT( 15 );
static const int AT_TCS_OUT_RESERVED = BIT( 16 );

static const int AT_TES_IN_RESERVED = BIT( 17 );
static const int AT_TES_OUT_RESERVED = BIT( 18 );

static const int AT_GS_IN_RESERVED = BIT( 19 );
static const int AT_GS_OUT_RESERVED = BIT( 20 );

static const int AT_PS_IN_RESERVED = BIT( 21 );
static const int AT_PS_OUT_RESERVED = BIT( 22 );

static const int AT_CS_IN_RESERVED = BIT( 23 );
static const int AT_CS_OUT_RESERVED = BIT( 24 );

static const int AT_PREFIX = BIT( 31 );

#define VInR ( AT_VS_IN | AT_VS_IN_RESERVED )
#define VOutR ( AT_VS_OUT | AT_VS_OUT_RESERVED )

#define TCInR ( AT_TCS_IN | AT_TCS_IN_RESERVED )
#define TCOutR ( AT_TCS_OUT | AT_TCS_OUT_RESERVED )

#define TEInR ( AT_TES_IN | AT_TES_IN_RESERVED )
#define TEOutR ( AT_TES_OUT | AT_TES_OUT_RESERVED )

#define GInR ( AT_GS_IN | AT_GS_IN_RESERVED )
#define GOutR ( AT_GS_OUT | AT_GS_OUT_RESERVED )

#define FInR ( AT_PS_IN | AT_PS_IN_RESERVED )
#define FOutR ( AT_PS_OUT | AT_PS_OUT_RESERVED )

#define InOutVar ( AT_VS_OUT | AT_TCS_IN|AT_TCS_OUT|AT_TES_IN|AT_TES_OUT | AT_GS_IN|AT_GS_OUT | AT_PS_IN )

struct glslShaderInfo_t
{
	GLenum	glTargetType;
	const char * shaderTypeDeclName;
	const char * shaderTypeName;
	const char * uniformArrayName;
}
glslShaderInfo[ SHT_MAX+1 ] =
{
	{ GL_VERTEX_SHADER, "vertexShader", "vertex", VERTEX_UNIFORM_ARRAY_NAME },
	{ GL_TESS_CONTROL_SHADER, "tessCtrlShader", "tess_ctrl", TESS_CONTROL_UNIFORM_ARRAY_NAME },
	{ GL_TESS_EVALUATION_SHADER, "tessEvalShader", "tess_eval", TESS_EVALUATION_UNIFORM_ARRAY_NAME },
	{ GL_GEOMETRY_SHADER, "geometryShader", "geometry", GEOMETRY_UNIFORM_ARRAY_NAME },
	{ GL_FRAGMENT_SHADER, "fragmentShader", "fragment", FRAGMENT_UNIFORM_ARRAY_NAME },
	{ GL_COMPUTE_SHADER, "computeShader", "compute", COMPUTE_UNIFORM_ARRAY_NAME },
};

/*
================================================
	attribInfo_t
================================================
*/
struct attribInfo_t {
	const char* 	type;
	const char* 	name;
	const char* 	semantic;
	const char* 	glsl;
	int32			bind;
	int32			flags;
	int32			vertexMask;
}
	attribsPC[] =
{
	// vertex attributes
	{ "float4",	"position",	"VS_IN_POSITION",	"in_Position",	PC_ATTRIB_INDEX_POSITION,	AT_VS_IN,	VERTEX_MASK_POSITION },
	{ "float2",	"texcoord",	"VS_IN_TEXCOORD0",	"in_TexCoord",	PC_ATTRIB_INDEX_ST,			AT_VS_IN,	VERTEX_MASK_ST },
	{ "float4",	"normal",	"VS_IN_NORMAL",		"in_Normal",	PC_ATTRIB_INDEX_NORMAL,		AT_VS_IN,	VERTEX_MASK_NORMAL },
	{ "float4",	"tangent",	"VS_IN_TANGENT",	"in_Tangent",	PC_ATTRIB_INDEX_TANGENT,	AT_VS_IN,	VERTEX_MASK_TANGENT },
	{ "float4",	"color",	"VS_IN_COLOR0",		"in_Color",		PC_ATTRIB_INDEX_COLOR,		AT_VS_IN,	VERTEX_MASK_COLOR },
	{ "float4",	"color2",	"VS_IN_COLOR1",		"in_Color2",	PC_ATTRIB_INDEX_COLOR2,		AT_VS_IN,	VERTEX_MASK_COLOR2 },

/**/{ "uint",		"vertexID",			"SV_VertexID",				"gl_VertexID",			0,	VInR,	0 },
/**/{ "uint",		"instanceID",		"SV_InstanceID",			"gl_InstanceID",		0,	VInR,	0 },
/**/{ "uint",		"baseVertexID",		"",							"gl_BaseVertexARB",		0,	VInR,	0 }, // DX?	 // Requires GLSL 4.60 or ARB_shader_draw_parameters
/**/{ "uint",		"baseInstanceID",	"",							"gl_BaseInstanceARB",	0,	VInR,	0 }, // DX?	 // Requires GLSL 4.60 or ARB_shader_draw_parameters
/**/{ "uint",		"drawID",			"",							"gl_DrawIDARB",			0,	VInR,	0 }, // DX?	 // Requires GLSL 4.60 or ARB_shader_draw_parameters

/**/{ "uint",		"invocationID",		"SV_OutputControlPointID",	"gl_InvocationID",		0,	TCInR,	0 },
/**/{ "int",		"patchVertices",	"",							"gl_PatchVerticesIn",	0,	TCInR | TEInR, 0 }, // DX? to func GetNumPatchVertices()
/**/{ "float3",		"tessCoord",		"SV_DomainLocation",		"gl_TessCoord",			0,	TEInR,	0 },
/**/{ "float",		"tessLevelOuter",	"SV_TessFactor",			"gl_TessLevelOuter",	0,	TCOutR | TEInR | TEOutR,	0 },	// [4]
/**/{ "float",		"tessLevelInner",	"SV_InsideTessFactor",		"gl_TessLevelInner",	0,	TCOutR | TEInR | TEOutR,	0 },	// [2]

/**/{ "uint",		"primitiveID",		"SV_PrimitiveID",			"gl_PrimitiveIDIn",		0,	GInR,	0 },
/**/{ "uint",		"invocationID",		"SV_GSInstanceID",			"gl_InvocationID",		0,	GInR,	0 },// Requires GLSL 4.0 or ARB_gpu_shader5

/**/{ "float4",		"position",			"SV_Position",				"gl_Position",			0,	VOutR | GInR | GOutR | TCInR | TCOutR | TEInR | TEOutR | AT_PREFIX,	0 },
/**/{ "float",		"clip",				"SV_ClipDistance",			"gl_ClipDistance",		0,	VOutR | GInR | GOutR | TCInR | TCOutR | TEInR | TEOutR | FInR | AT_PREFIX,	0 }, //[]
/**/{ "float",		"cull",				"SV_CullDistance",			"gl_CullDistance",		0,	VOutR | GInR | GOutR | TCInR | TCOutR | TEInR | TEOutR | FInR | AT_PREFIX,	0 }, //[]
/**/{ "int",		"count",			"",							"length()",				0,	GInR | TCInR | TEInR | AT_PREFIX, 0 },


/**/{ "uint",		"primitiveID",		"SV_PrimitiveID",			"gl_PrimitiveID",		0,	GOutR | TCInR | TEInR | FInR,	0 },

// GL_AMD_vertex_shader_layer / GL_AMD_vertex_shader_viewport / GL_ARB_fragment_layer_viewport / GL_ARB_viewport_array
/**/{ "uint",		"layerIndex",		"SV_RenderTargetArrayIndex","gl_Layer",				0,	VOutR | GOutR | FInR,	0 },
/**/{ "uint",		"viewportIndex",	"SV_ViewportArrayIndex",	"gl_ViewportIndex",		0,	VOutR | GOutR | FInR,	0 },

	{ "float4",		"position",			"SV_Position",				"gl_FragCoord",			0,	FInR,	0 },
	{ "half4",		"hposition",		"SV_Position",				"gl_FragCoord",			0,	FInR,	0 },
	{ "float",		"facing",			"FACE",						"gl_FrontFacing",		0,	FInR,	0 },
/**/{ "bool",		"isFrontFace",		"SV_IsFrontFace",			"gl_FrontFacing",		0,	FInR,	0 },

// GL 4.0 or GL_ARB_sample_shading
/**/{ "uint",		"sampleID",			"SV_SampleIndex",			"gl_SampleID",			0,	FInR,	0 },
/**/{ "float2",		"samplePosition",	"",							"gl_SamplePosition",	0,	FInR,	0 }, //DX GetRenderTargetSamplePosition()
/**/{ "int",		"sampleCount",		"",							"gl_NumSamples",		0,	FInR,	0 }, //DX GetRenderTargetSamplePosition()
/**/{ "uint",		"sampleMask",		"SV_Coverage",				"gl_SampleMaskIn",		0,	FInR,	0 }, //[]
/**/{ "uint",		"sampleMask",		"SV_Coverage",				"gl_SampleMask",		0,	FOutR,	0 }, //[]
// uniform int gl_NumSamples; // GLSL 4.20

/**/{ "uint",		"stencilRef",		"SV_StencilRef",			"gl_FragStencilRef",	0,	FOutR,	0 }, //[]
	{ "float",		"depth",			"SV_Depth",					"gl_FragDepth",			0,	FOutR,	0 },

	{ "float4",		"color",		"COLOR",		"out_FragColor",	0,	AT_PS_OUT,	0 },
	{ "half4",		"hcolor",		"COLOR",		"out_FragColor",	0,	AT_PS_OUT,	0 },

	{ "float4",		"target0",		"SV_Target0",	"out_FragColor0",	0,	AT_PS_OUT,	0 },
	{ "float4",		"target1",		"SV_Target1",	"out_FragColor1",	1,	AT_PS_OUT,	0 },
	{ "float4",		"target2",		"SV_Target2",	"out_FragColor2",	2,	AT_PS_OUT,	0 },
	{ "float4",		"target3",		"SV_Target3",	"out_FragColor3",	3,	AT_PS_OUT,	0 },
	{ "float4",		"target4",		"SV_Target4",	"out_FragColor4",	4,	AT_PS_OUT,	0 },
	{ "float4",		"target5",		"SV_Target5",	"out_FragColor5",	5,	AT_PS_OUT,	0 },
	{ "float4",		"target6",		"SV_Target6",	"out_FragColor6",	6,	AT_PS_OUT,	0 },
	{ "float4",		"target7",		"SV_Target7",	"out_FragColor7",	7,	AT_PS_OUT,	0 },

	{ "half4",		"htarget0",		"SV_Target0",	"out_FragColor0",	0,	AT_PS_OUT,	0 },
	{ "half4",		"htarget1",		"SV_Target1",	"out_FragColor1",	1,	AT_PS_OUT,	0 },
	{ "half4",		"htarget2",		"SV_Target2",	"out_FragColor2",	2,	AT_PS_OUT,	0 },
	{ "half4",		"htarget3",		"SV_Target3",	"out_FragColor3",	3,	AT_PS_OUT,	0 },
	{ "half4",		"htarget4",		"SV_Target4",	"out_FragColor4",	4,	AT_PS_OUT,	0 },
	{ "half4",		"htarget5",		"SV_Target5",	"out_FragColor5",	5,	AT_PS_OUT,	0 },
	{ "half4",		"htarget6",		"SV_Target6",	"out_FragColor6",	6,	AT_PS_OUT,	0 },
	{ "half4",		"htarget7",		"SV_Target7",	"out_FragColor7",	7,	AT_PS_OUT,	0 },

	{ "uint3",		"numWorkGroups",	"",						"gl_NumWorkGroups",			0,	AT_CS_IN | AT_CS_IN_RESERVED,	0 }, //? to func
	{ "uint3",		"workGroupSize",	"",						"gl_WorkGroupSize",			0,	AT_CS_IN | AT_CS_IN_RESERVED,	0 }, //? to func
	{ "uint3",		"groupID",			"SV_GroupID",			"gl_WorkGroupID",			0,	AT_CS_IN | AT_CS_IN_RESERVED,	0 },
	{ "uint3",		"groupThreadID",	"SV_GroupThreadID",		"gl_LocalInvocationID",		0,	AT_CS_IN | AT_CS_IN_RESERVED,	0 },
	{ "uint3",		"dispatchThreadID",	"SV_DispatchThreadID",	"gl_GlobalInvocationID",	0,	AT_CS_IN | AT_CS_IN_RESERVED,	0 },
	{ "uint",		"groupIndex",		"SV_GroupIndex",		"gl_LocalInvocationIndex",	0,	AT_CS_IN | AT_CS_IN_RESERVED,	0 },

	// shaders pass through

	{ "float4",		"color",		"COLOR",				"vofi_Color",		0,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"color0",		"COLOR0",				"vofi_Color",		0,	AT_VS_OUT | AT_PS_IN,	0 },

	{ "half4",		"hcolor",		"COLOR",				"vofi_Color",		0,	AT_PS_IN,	0 },
	{ "half4",		"hcolor0",		"COLOR0",				"vofi_Color",		0,	AT_PS_IN,	0 },

	{ "float4",		"texcoord0",	"TEXCOORD0_centroid",	"vofi_TexCoord0",	0,	AT_PS_IN,	0 },
	{ "float4",		"texcoord1",	"TEXCOORD1_centroid",	"vofi_TexCoord1",	1,	AT_PS_IN,	0 },
	{ "float4",		"texcoord2",	"TEXCOORD2_centroid",	"vofi_TexCoord2",	2,	AT_PS_IN,	0 },
	{ "float4",		"texcoord3",	"TEXCOORD3_centroid",	"vofi_TexCoord3",	3,	AT_PS_IN,	0 },
	{ "float4",		"texcoord4",	"TEXCOORD4_centroid",	"vofi_TexCoord4",	4,	AT_PS_IN,	0 },
	{ "float4",		"texcoord5",	"TEXCOORD5_centroid",	"vofi_TexCoord5",	5,	AT_PS_IN,	0 },
	{ "float4",		"texcoord6",	"TEXCOORD6_centroid",	"vofi_TexCoord6",	6,	AT_PS_IN,	0 },
	{ "float4",		"texcoord7",	"TEXCOORD7_centroid",	"vofi_TexCoord7",	7,	AT_PS_IN,	0 },
	{ "float4",		"texcoord8",	"TEXCOORD8_centroid",	"vofi_TexCoord8",	8,	AT_PS_IN,	0 },
	{ "float4",		"texcoord9",	"TEXCOORD9_centroid",	"vofi_TexCoord9",	9,	AT_PS_IN,	0 },
	{ "float4",		"texcoord10",	"TEXCOORD10_centroid",	"vofi_TexCoord10",	10,	AT_PS_IN,	0 },
	{ "float4",		"texcoord11",	"TEXCOORD11_centroid",	"vofi_TexCoord11",	11,	AT_PS_IN,	0 },
	{ "float4",		"texcoord12",	"TEXCOORD12_centroid",	"vofi_TexCoord12",	12,	AT_PS_IN,	0 },
	{ "float4",		"texcoord13",	"TEXCOORD13_centroid",	"vofi_TexCoord13",	13,	AT_PS_IN,	0 },
	{ "float4",		"texcoord14",	"TEXCOORD14_centroid",	"vofi_TexCoord14",	14,	AT_PS_IN,	0 },
	{ "float4",		"texcoord15",	"TEXCOORD15_centroid",	"vofi_TexCoord15",	15,	AT_PS_IN,	0 },

	{ "float4",		"texcoord0",	"TEXCOORD0",	"vofi_TexCoord0",	0,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord1",	"TEXCOORD1",	"vofi_TexCoord1",	1,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord2",	"TEXCOORD2",	"vofi_TexCoord2",	2,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord3",	"TEXCOORD3",	"vofi_TexCoord3",	3,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord4",	"TEXCOORD4",	"vofi_TexCoord4",	4,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord5",	"TEXCOORD5",	"vofi_TexCoord5",	5,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord6",	"TEXCOORD6",	"vofi_TexCoord6",	6,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord7",	"TEXCOORD7",	"vofi_TexCoord7",	7,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord8",	"TEXCOORD8",	"vofi_TexCoord8",	8,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord9",	"TEXCOORD9",	"vofi_TexCoord9",	9,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord10",	"TEXCOORD10",	"vofi_TexCoord10",	10,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord11",	"TEXCOORD11",	"vofi_TexCoord11",	11,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord12",	"TEXCOORD12",	"vofi_TexCoord12",	12,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord13",	"TEXCOORD13",	"vofi_TexCoord13",	13,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord14",	"TEXCOORD14",	"vofi_TexCoord14",	14,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4",		"texcoord15",	"TEXCOORD15",	"vofi_TexCoord15",	15,	AT_VS_OUT | AT_PS_IN,	0 },

	{ "float4x4",	"x4matrix0",	"MATRIX4X4_0",	"vofi_x4Matrix0",	0,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4x4",	"x4matrix1",	"MATRIX4X4_1",	"vofi_x4Matrix1",	0,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4x4",	"x4matrix2",	"MATRIX4X4_2",	"vofi_x4Matrix2",	0,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float4x4",	"x4matrix3",	"MATRIX4X4_3",	"vofi_x4Matrix3",	0,	AT_VS_OUT | AT_PS_IN,	0 },

	{ "float3x3",	"x3matrix0",	"MATRIX3X3_0",	"vofi_x3Matrix0",	0,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float3x3",	"x3matrix1",	"MATRIX3X3_1",	"vofi_x3Matrix1",	0,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float3x3",	"x3matrix2",	"MATRIX3X3_2",	"vofi_x3Matrix2",	0,	AT_VS_OUT | AT_PS_IN,	0 },
	{ "float3x3",	"x3matrix3",	"MATRIX3X3_3",	"vofi_x3Matrix3",	0,	AT_VS_OUT | AT_PS_IN,	0 },

	{ NULL, NULL, NULL, NULL, 0, 0, 0 }
};

const char* types[] =
{
	"int",
	"uint",
	"float",
	"double",
	"half",
	"fixed",
	"bool",
	"cint",
	"cfloat",
	"void"
};
static const int numTypes = _countof( types );

const char* typePosts[] =
{
	"1", "2", "3", "4",
	"1x1", "1x2", "1x3", "1x4",
	"2x1", "2x2", "2x3", "2x4",
	"3x1", "3x2", "3x3", "3x4",
	"4x1", "4x2", "4x3", "4x4"
};
static const int numTypePosts = _countof( typePosts );

const char* prefixes[] =
{
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
static const int numPrefixes = _countof( prefixes );

// For GLSL we need to have the names for the renderparms so we can look up their run time indices within the renderprograms
static const char* GLSLParmNames[ RENDERPARM_TOTAL ] =
{
	"rpScreenCorrectionFactor",
	"rpWindowCoord",
	"rpDiffuseModifier",
	"rpSpecularModifier",

	"rpLocalLightOrigin",
	"rpLocalViewOrigin",

	"rpLightProjectionS",
	"rpLightProjectionT",
	"rpLightProjectionQ",
	"rpLightFalloffS",

	"rpBumpMatrixS",
	"rpBumpMatrixT",

	"rpDiffuseMatrixS",
	"rpDiffuseMatrixT",

	"rpSpecularMatrixS",
	"rpSpecularMatrixT",

	"rpVertexColorModulate",
	"rpVertexColorAdd",

	"rpColor",
	"rpViewOrigin",
	"rpGlobalEyePos",

	"rpMVPmatrixX",
	"rpMVPmatrixY",
	"rpMVPmatrixZ",
	"rpMVPmatrixW",

	"rpModelMatrixX",
	"rpModelMatrixY",
	"rpModelMatrixZ",
	"rpModelMatrixW",

	"rpProjectionMatrixX",
	"rpProjectionMatrixY",
	"rpProjectionMatrixZ",
	"rpProjectionMatrixW",

	"rpModelViewMatrixX",
	"rpModelViewMatrixY",
	"rpModelViewMatrixZ",
	"rpModelViewMatrixW",

	"rpTextureMatrixS",
	"rpTextureMatrixT",

	"rpTexGen0S",
	"rpTexGen0T",
	"rpTexGen0Q",
	"rpTexGen0Enabled",

	"rpTexGen1S",
	"rpTexGen1T",
	"rpTexGen1Q",
	"rpTexGen1Enabled",

	"rpWobbleSkyX",
	"rpWobbleSkyY",
	"rpWobbleSkyZ",

	"rpOverbright",
	"rpEnableSkinning",
	"rpAlphaTest",

	// RB begin
	"rpAmbientColor",

	"rpGlobalLightOrigin",
	"rpJitterTexScale",
	"rpJitterTexOffset",
	"rpCascadeDistances",

	"rpVertexColorMAD",		//SEA

	"rpBaseLightProjectS",	//SEA
	"rpBaseLightProjectT",	//SEA
	"rpBaseLightProjectR",	//SEA
	"rpBaseLightProjectQ",	//SEA

	"rpShadowMatrices",
	"rpShadowMatrix0Y",
	"rpShadowMatrix0Z",
	"rpShadowMatrix0W",

	"rpShadowMatrix1X",
	"rpShadowMatrix1Y",
	"rpShadowMatrix1Z",
	"rpShadowMatrix1W",

	"rpShadowMatrix2X",
	"rpShadowMatrix2Y",
	"rpShadowMatrix2Z",
	"rpShadowMatrix2W",

	"rpShadowMatrix3X",
	"rpShadowMatrix3Y",
	"rpShadowMatrix3Z",
	"rpShadowMatrix3W",

	"rpShadowMatrix4X",
	"rpShadowMatrix4Y",
	"rpShadowMatrix4Z",
	"rpShadowMatrix4W",

	"rpShadowMatrix5X",
	"rpShadowMatrix5Y",
	"rpShadowMatrix5Z",
	"rpShadowMatrix5W",
	// RB end
};

// RB begin
/*const char* idRenderProgManager::GLSLMacroNames[ MAX_SHADER_MACRO_NAMES ] =
{
	"USE_GPU_SKINNING",
	"LIGHT_POINT",
	"LIGHT_PARALLEL",
	"BRIGHTPASS",
	"HDR_DEBUG",
	"USE_SRGB"
};*/
// RB end

/*
========================
StripDeadCode
========================
*/
void StripDeadCode( const char* in, const char* name, const idStrList& compileMacros, bool builtin, idStr & out )
{
	if( r_skipStripDeadCode.GetBool() )
	{
		out = in;
		return;
	}

	idParser src( LEXFL_NOFATALERRORS | LEXFL_NODOLLARPRECOMPILE );
	src.LoadMemory( in, idStr::Length( in ), name );

	idStrStatic<MAX_FILENAME_LENGTH> sourceName = "filename ";
	sourceName += name;

	src.AddDefine( sourceName );
	src.AddDefine( "PC" );
	src.AddDefine( "USE_UNIFORM_ARRAYS" );

	for( int i = 0; i < compileMacros.Num(); i++ )
	{
		src.AddDefine( compileMacros[ i ] );
	}

	if( glConfig.driverType == GLDRV_OPENGL_ES3 )
	{
		src.AddDefine( "GLES" );
	}

	if( !builtin )
	{
		src.AddDefine( "USE_GPU_SKINNING" );
	}

	if( r_useHalfLambertLighting.GetBool() )
	{
		src.AddDefine( "USE_HALF_LAMBERT" );
	}

	if( r_useHDR.GetBool() )
	{
		src.AddDefine( "USE_LINEAR_RGB" );
	}

	// SMAA configuration
	src.AddDefine( "SMAA_GLSL_3" );
	src.AddDefine( "SMAA_RT_METRICS rpScreenCorrectionFactor " );
	src.AddDefine( "SMAA_PRESET_HIGH" );

	struct idCGBlock {
		idStrStatic<64> prefix;	// tokens that comes before the name
		idStrStatic<64> name;	// the name
		idStr postfix;	// tokens that comes after the name
		bool  used;		// whether or not this block is referenced anywhere
	};
	idList< idCGBlock > blocks;
	blocks.SetNum( 100 );

	idToken token;
	while( !src.EndOfFile() )
	{
		idCGBlock& block = blocks.Alloc();

		// read prefix
		while( src.ReadToken( &token ) )
		{
			bool found = false;
			for( int i = 0; i < numPrefixes; i++ )
			{
				if( token == prefixes[ i ] )
				{
					found = true;
					break;
				}
			}
			if( !found )
			{
				for( int i = 0; i < numTypes; i++ )
				{
					if( token == types[ i ] )
					{
						found = true;
						break;
					}
					int typeLen = idStr::Length( types[ i ] );
					if( token.Cmpn( types[ i ], typeLen ) == 0 )
					{
						for( int j = 0; j < numTypePosts; j++ )
						{
							if( idStr::Cmp( token.c_str() + typeLen, typePosts[ j ] ) == 0 )
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
			else
			{
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
					if( token.WhiteSpaceBeforeToken() )
					{
						block.postfix += ' ';
					}
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
		else
		{
			src.Warning( "Could not strip dead code -- unknown token %s\n", token.c_str() );
			out = in;
			return;
		}
	}

	idList<int, TAG_RENDERPROG> stack;
	for( int i = 0; i < blocks.Num(); ++i )
	{
		blocks[ i ].used = ( ( blocks[ i ].name == "main" ) || blocks[ i ].name.Right( 4 ) == "_ubo" );

		if( blocks[ i ].name == "include" )
		{
			blocks[ i ].used = true;
			blocks[ i ].name = ""; // clear out the include tag
		}

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

	idStringBuilder_Heap outstr;

	for( int i = 0; i < blocks.Num(); ++i )
	{
		if( blocks[ i ].used )
		{
			outstr += blocks[ i ].prefix;
			outstr += ' ';
			outstr += blocks[ i ].name;
			outstr += ' ';
			outstr += blocks[ i ].postfix;
			outstr += '\n';
		}
	}
	outstr.ToString( out );
}

struct typeConversion_t {
	const char* typeCG;
	const char* typeGLSL;
}
typeConversion[] =
{
	{ "void",			"void" },

	{ "fixed",			"float" },

	{ "float",			"float" },
	{ "float2",			"vec2" },
	{ "float3",			"vec3" },
	{ "float4",			"vec4" },

	{ "half",			"float" },
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

	{ "sampler1D",		"sampler1D" },
	{ "sampler2D",		"sampler2D" },
	{ "sampler3D",		"sampler3D" },
	{ "samplerCUBE",	"samplerCube" },

	//SEA: is this needed?

	{ "sampler1DShadow",		"sampler1DShadow" },
	{ "sampler2DShadow",		"sampler2DShadow" },
	{ "sampler2DArrayShadow",	"sampler2DArrayShadow" },
	{ "sampler3DShadow",		"sampler3DShadow" },
	{ "samplerCubeShadow",		"samplerCubeShadow" },
	{ "samplerCubeArrayShadow",	"samplerCubeArrayShadow" },

	{ "sampler2DArray",		"sampler2DArray" },
	{ "samplerCubeArray",	"samplerCubeArray" },

	{ "sampler2DMS",		"sampler2DMS" },
	{ "sampler2DMSArray",	"sampler2DMSArray" },

	{ NULL, NULL }

	/*
	#extension GL_NV_gpu_shader5 : <behavior>

	int8_t              i8vec2          i8vec3          i8vec4
	int16_t             i16vec2         i16vec3         i16vec4
	int32_t             i32vec2         i32vec3         i32vec4
	int64_t             i64vec2         i64vec3         i64vec4
	uint8_t             u8vec2          u8vec3          u8vec4
	uint16_t            u16vec2         u16vec3         u16vec4
	uint32_t            u32vec2         u32vec3         u32vec4
	uint64_t            u64vec2         u64vec3         u64vec4
	float16_t           f16vec2         f16vec3         f16vec4
	float32_t           f32vec2         f32vec3         f32vec4
	float64_t           f64vec2         f64vec3         f64vec4

	(note:  the "float64_t" and "f64vec*" types are available if and only if
	ARB_gpu_shader_fp64 is also supported)
	*/
};

// #extension extension_name : require
// Give an error on the #extension if the extension extension_name is not
// supported, or if all is specified.

// #ifdef OES_extension_name
//	 #extension OES_extension_name : enable
//	 code that requires the extension
// #else
//	 alternative code
// #endif

//SEA: TODO! read from file

const char* vertexInsert_GLSL_ES_3_00 =
{
	"#version 300 es\n"
	"#define PC\n"
	"precision mediump float;\n"

	//"#extension GL_ARB_gpu_shader5 : enable\n"
	"\n"
	"float saturate( float v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec2 saturate( vec2 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec3 saturate( vec3 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec4 saturate( vec4 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	//"vec4 tex2Dlod( sampler2D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xy, texcoord.w ); }\n"
	"\n"
};

const char* vertexInsert_GLSL_1_50 =
{
	"#version 150 core\n"
	"#define PC\n"
	"\n"
	"float saturate( float v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec2 saturate( vec2 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec3 saturate( vec3 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec4 saturate( vec4 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec4 tex2Dlod( sampler2D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xy, texcoord.w ); }\n"
	"\n"
};

const char* fragmentInsert_GLSL_ES_3_00 =
{
	"#version 300 es\n"
	"#define PC\n"
	"precision mediump float;\n"
	"precision lowp sampler2D;\n"
	"precision lowp sampler2DShadow;\n"
	"precision lowp sampler2DArray;\n"
	"precision lowp sampler2DArrayShadow;\n"
	"precision lowp samplerCube;\n"
	"precision lowp samplerCubeShadow;\n"
	"precision lowp sampler3D;\n"
	"\n"
	"void clip( float v ) { if ( v < 0.0 ) { discard; } }\n"
	"void clip( vec2 v ) { if ( any( lessThan( v, vec2( 0.0 ) ) ) ) { discard; } }\n"
	"void clip( vec3 v ) { if ( any( lessThan( v, vec3( 0.0 ) ) ) ) { discard; } }\n"
	"void clip( vec4 v ) { if ( any( lessThan( v, vec4( 0.0 ) ) ) ) { discard; } }\n"
	"\n"
	"float saturate( float v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec2 saturate( vec2 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec3 saturate( vec3 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec4 saturate( vec4 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"\n"
	"vec4 tex2D( sampler2D sampler, vec2 texcoord ) { return texture( sampler, texcoord.xy ); }\n"
	"vec4 tex2D( sampler2DShadow sampler, vec3 texcoord ) { return vec4( texture( sampler, texcoord.xyz ) ); }\n"
	"\n"
	"vec4 tex2D( sampler2D sampler, vec2 texcoord, vec2 dx, vec2 dy ) { return textureGrad( sampler, texcoord.xy, dx, dy ); }\n"
	"vec4 tex2D( sampler2DShadow sampler, vec3 texcoord, vec2 dx, vec2 dy ) { return vec4( textureGrad( sampler, texcoord.xyz, dx, dy ) ); }\n"
	"\n"
	"vec4 texCUBE( samplerCube sampler, vec3 texcoord ) { return texture( sampler, texcoord.xyz ); }\n"
	"vec4 texCUBE( samplerCubeShadow sampler, vec4 texcoord ) { return vec4( texture( sampler, texcoord.xyzw ) ); }\n"
	"\n"
	//"vec4 tex1Dproj( sampler1D sampler, vec2 texcoord ) { return textureProj( sampler, texcoord ); }\n"
	"vec4 tex2Dproj( sampler2D sampler, vec3 texcoord ) { return textureProj( sampler, texcoord ); }\n"
	"vec4 tex3Dproj( sampler3D sampler, vec4 texcoord ) { return textureProj( sampler, texcoord ); }\n"
	"\n"
	//"vec4 tex1Dbias( sampler1D sampler, vec4 texcoord ) { return texture( sampler, texcoord.x, texcoord.w ); }\n"
	"vec4 tex2Dbias( sampler2D sampler, vec4 texcoord ) { return texture( sampler, texcoord.xy, texcoord.w ); }\n"
	"vec4 tex3Dbias( sampler3D sampler, vec4 texcoord ) { return texture( sampler, texcoord.xyz, texcoord.w ); }\n"
	"vec4 texCUBEbias( samplerCube sampler, vec4 texcoord ) { return texture( sampler, texcoord.xyz, texcoord.w ); }\n"
	"\n"
	//"vec4 tex1Dlod( sampler1D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.x, texcoord.w ); }\n"
	"vec4 tex2Dlod( sampler2D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xy, texcoord.w ); }\n"
	"vec4 tex3Dlod( sampler3D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xyz, texcoord.w ); }\n"
	"vec4 texCUBElod( samplerCube sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xyz, texcoord.w ); }\n"
	"\n"
};

const char* fragmentInsert_GLSL_1_50 =
{
	"#version 150 core\n"
	"#define PC\n"
	"\n"
	"void clip( float v ) { if ( v < 0.0 ) { discard; } }\n"
	"void clip( vec2 v ) { if ( any( lessThan( v, vec2( 0.0 ) ) ) ) { discard; } }\n"
	"void clip( vec3 v ) { if ( any( lessThan( v, vec3( 0.0 ) ) ) ) { discard; } }\n"
	"void clip( vec4 v ) { if ( any( lessThan( v, vec4( 0.0 ) ) ) ) { discard; } }\n"
	"\n"
	"float saturate( float v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec2 saturate( vec2 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec3 saturate( vec3 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"vec4 saturate( vec4 v ) { return clamp( v, 0.0, 1.0 ); }\n"
	"\n"
	"vec4 tex2D( sampler2D sampler, vec2 texcoord ) { return texture( sampler, texcoord.xy ); }\n"
	"vec4 tex2D( sampler2DShadow sampler, vec3 texcoord ) { return vec4( texture( sampler, texcoord.xyz ) ); }\n"
	"\n"
	"vec4 tex2D( sampler2D sampler, vec2 texcoord, vec2 dx, vec2 dy ) { return textureGrad( sampler, texcoord.xy, dx, dy ); }\n"
	"vec4 tex2D( sampler2DShadow sampler, vec3 texcoord, vec2 dx, vec2 dy ) { return vec4( textureGrad( sampler, texcoord.xyz, dx, dy ) ); }\n"
	"\n"
	"vec4 texCUBE( samplerCube sampler, vec3 texcoord ) { return texture( sampler, texcoord.xyz ); }\n"
	"vec4 texCUBE( samplerCubeShadow sampler, vec4 texcoord ) { return vec4( texture( sampler, texcoord.xyzw ) ); }\n"
	"\n"
	"vec4 tex2Dproj( sampler2D sampler, vec3 texcoord ) { return textureProj( sampler, texcoord ); }\n"
	"vec4 tex3Dproj( sampler3D sampler, vec4 texcoord ) { return textureProj( sampler, texcoord ); }\n"
	"\n"
	"vec4 tex2Dbias( sampler2D sampler, vec4 texcoord ) { return texture( sampler, texcoord.xy, texcoord.w ); }\n"
	"vec4 tex3Dbias( sampler3D sampler, vec4 texcoord ) { return texture( sampler, texcoord.xyz, texcoord.w ); }\n"
	"vec4 texCUBEbias( samplerCube sampler, vec4 texcoord ) { return texture( sampler, texcoord.xyz, texcoord.w ); }\n"
	"\n"
	"vec4 tex2Dlod( sampler2D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xy, texcoord.w ); }\n"
	"vec4 tex3Dlod( sampler3D sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xyz, texcoord.w ); }\n"
	"vec4 texCUBElod( samplerCube sampler, vec4 texcoord ) { return textureLod( sampler, texcoord.xyz, texcoord.w ); }\n"
	"\n"
};
// RB end

void SetupGLSLHeader_Extensions( idStr & out )
{
	// GL_ARB_explicit_attrib_location
	if( GLEW_ARB_explicit_attrib_location )	//EGL 3.0, MESA
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

	// GL_NV_gpu_shader5
	/*
	add to the list of qualifiers for geometry shaders, p. 37)

	  layout-qualifier-id:
		...
		triangles_adjacency
		patches

		Layout          Size of Input Arrays
	  ------------      --------------------
		patches         gl_MaxPatchVertices

		(add to the list of built-ins inputs for geometry shaders) In the geometry
		language, built-in input and output variables are intrinsically declared
		as:

		in int gl_PatchVerticesIn;
		patch in float gl_TessLevelOuter[4];
		patch in float gl_TessLevelInner[2];

		int64_t  packInt2x32(ivec2 v);
		uint64_t packUint2x32(uvec2 v);

		ivec2  unpackInt2x32(int64_t v);
		uvec2  unpackUint2x32(uint64_t v);

		uint      packFloat2x16(f16vec2 v);
		f16vec2   unpackFloat2x16(uint v);

		genI64Type doubleBitsToInt64(genDType value);
		genU64Type doubleBitsToUint64(genDType value);

		genDType int64BitsToDouble(genI64Type value);
		genDType uint64BitsToDouble(genU64Type value);

		bool anyThreadNV(bool value);
		bool allThreadsNV(bool value);
		bool allThreadsEqualNV(bool value);
	*/

	// GL_ARB_shading_language_420pack
	if( GLEW_ARB_shading_language_420pack ) //EGL 3.1, MESA
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
	if( GLEW_ARB_gpu_shader5 )
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
	}
	// GL_ARB_viewport_array	MESA
	if( GLEW_ARB_viewport_array )
	{
	}
	// GL_ARB_sample_shading	MESA
	if( GLEW_ARB_sample_shading )
	{
	}
	// GL_ARB_texture_gather	MESA
	if( GLEW_ARB_texture_gather )
	{
	}

	// GL_ARB_conservative_depth	MESA
	if( GLEW_ARB_conservative_depth )
	{
	}

	// GL_ARB_explicit_uniform_location
	if( GLEW_ARB_explicit_uniform_location )
	{
	}

	// GL_ARB_shader_image_size

	// GL_ARB_texture_cube_map_array
	/*
		samplerCubeArray, samplerCubeArrayShadow,
		isamplerCubeArray,
		usamplerCubeArray
	*/

	// GL_ARB_fragment_layer_viewport
	if( GLEW_ARB_fragment_layer_viewport )
	{
		//	in int gl_Layer;
		//	in int gl_ViewportIndex;
	}
	// GL_AMD_vertex_shader_layer
	if( GLEW_AMD_vertex_shader_layer )
	{
	}
	// GL_AMD_vertex_shader_viewport
	/*if( GLEW_AMD_vertex_shader_viewport )
	{
	}*/

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

	// GL_ARB_shader_image_load_store

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
	if( GLEW_ARB_shader_draw_parameters )
	{
		// gl_BaseVertexARB gl_BaseInstanceARB gl_DrawID
	}

	// GL_ARB_indirect_parameters
	if( GLEW_ARB_indirect_parameters )
	{
		// gl_BaseVertexARB gl_BaseInstanceARB gl_DrawID
	}

	if( GLEW_ARB_cull_distance )
	{
	}

	// GL_ARB_derivative_control
	if( GLEW_ARB_derivative_control )
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
	}

	// GL_ARB_compute_shader

	// GL_ARB_enhanced_layouts

	out += "\n";
}

// check for a function conversion
bool ConvertBuiltin( idToken & token, idStr & out, char newline[] )
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
		//SEA ->
		{ "ddy_coarse",						"dFdyCoarse" },
		{ "ddy_fine",						"dFdyFine" },
		{ "EvaluateAttributeAtCentroid",	"interpolateAtCentroid" },
		{ "EvaluateAttributeAtSample",		"interpolateAtSample" },
		{ "EvaluateAttributeSnapped",		"interpolateAtOffset" },
		{ "mad",							"fma" },
		//SEA <-

		{ NULL, NULL }
	};

	for( int i = 0; builtinConversion[ i ].nameCG != NULL; i++ )
	{
		if( token.Cmp( builtinConversion[ i ].nameCG ) == 0 )
		{
			out += ( token.linesCrossed > 0 ) ? newline : ( token.WhiteSpaceBeforeToken() > 0 ? " " : "" );
			out += builtinConversion[ i ].nameGLSL;
			return true;
		}
	}
	return false;
}

struct inOutVariable_t {
	idStr	type;
	idStr	nameCg;
	idStr	nameGLSL;
	bool	declareInOut;
};

/*
========================
ParseInOutStruct
========================
*/
void ParseInOutStruct( idLexer& src, int attribType, int attribIgnoreType, idList< inOutVariable_t >& inOutVars )
{
	src.ExpectTokenString( "{" );

	while( !src.CheckTokenString( "}" ) )
	{
		inOutVariable_t var;

		idToken token;
		src.ReadToken( &token );
		var.type = token;
		src.ReadToken( &token );
		var.nameCg = token;

		if( !src.CheckTokenString( ":" ) )
		{
			src.SkipUntilString( ";" );
			continue;
		}

		src.ReadToken( &token );
		var.nameGLSL = token;
		src.ExpectTokenString( ";" );

		// convert the type
		for( int i = 0; typeConversion[ i ].typeCG != NULL; i++ )
		{
			if( var.type.Cmp( typeConversion[ i ].typeCG ) == 0 )
			{
				var.type = typeConversion[ i ].typeGLSL;
				break;
			}
		}

		// convert the semantic to a GLSL name
		for( int i = 0; attribsPC[ i ].semantic != NULL; i++ )
		{
			if( ( attribsPC[ i ].flags & attribType ) != 0 )
			{
				if( var.nameGLSL.Cmp( attribsPC[ i ].semantic ) == 0 )
				{
					var.nameGLSL = attribsPC[ i ].glsl;
					break;
				}
			}
		}

		// check if it was defined previously
		var.declareInOut = true;
		for( int i = 0; i < inOutVars.Num(); i++ )
		{
			if( var.nameGLSL == inOutVars[ i ].nameGLSL )
			{
				var.declareInOut = false;
				break;
			}
		}

		for( int i = 0; attribsPC[ i ].semantic != NULL; i++ )
		{
			if( var.nameGLSL.Cmp( attribsPC[ i ].glsl ) == 0 )
			{
				// RB: ignore reserved builtin gl_...
				if( ( attribsPC[ i ].flags & attribIgnoreType ) != 0 )
				{
					var.declareInOut = false;
					break;
				}
			}
		}

		inOutVars.Append( var );
	}

	src.ExpectTokenString( ";" );
}

/*
========================
 ConvertCG2GLSL

 points
 lines
 lines_adjacency
 triangles
 triangles_adjacency

 layout( triangles ) in;

========================
*/
void ConvertCG2GLSL( const idStr & in, idStr & out, const char* name, idStr& uniforms, shaderType_e shaderType )
{
	idStr program;
	program.ReAllocate( in.Length() * 2, false );

	idList< inOutVariable_t, TAG_RENDERPROG > varsIn;
	idList< inOutVariable_t, TAG_RENDERPROG > varsOut;
	idList< idStrStatic<80> > uniformList;

	idLexer src( /*LEXFL_NOFATALERRORS |*/ LEXFL_NODOLLARPRECOMPILE );
	src.LoadMemory( in.c_str(), in.Length(), name );

	bool inMain = false;
	auto uniformArrayName = glslShaderInfo[ shaderType ].uniformArrayName;
	char newline[ 128 ] = { "\n" };

	//idStaticList<const idDeclRenderParm *, 128> parmList;

	idToken token;
	while( src.ReadToken( &token ) )
	{
		// check for uniforms

		// RB: added special case for matrix arrays
		while( token == "uniform" && ( src.CheckTokenString( "float4" ) || src.CheckTokenString( "float4x4" ) ) )
		{
			src.ReadToken( &token );
			idStr uniform = token;

			// strip ': register()' from uniforms
			if( src.CheckTokenString( ":" ) )
			{
				if( src.CheckTokenString( "register" ) )
				{
					src.SkipUntilString( ";" );
				}
			}

			if( src.PeekTokenString( "[" ) )
			{
				while( src.ReadToken( &token ) )
				{
					uniform += token;

					if( token == "]" )
					{
						break;
					}
				}
			}

			uniformList.Append( uniform );

			src.ReadToken( &token );
		}
		// RB end

		// convert the in/out structs
		if( token == "struct" )
		{
			/*/*/if( src.CheckTokenString( "VS_IN" ) )
			{
				ParseInOutStruct( src, AT_VS_IN, AT_VS_IN_RESERVED, varsIn );
				program += "\n\n";
				for( int i = 0; i < varsIn.Num(); i++ )
				{
					if( varsIn[ i ].declareInOut )
					{
						program += "in " + varsIn[ i ].type + " " + varsIn[ i ].nameGLSL + ";\n";
					}
				}
				continue;
			}
			else if( src.CheckTokenString( "VS_OUT" ) )
			{
				ParseInOutStruct( src, AT_VS_OUT, AT_VS_OUT_RESERVED, varsOut );
				program += "\n";
				for( int i = 0; i < varsOut.Num(); i++ )
				{
					if( varsOut[ i ].declareInOut )
					{
						program += "out " + varsOut[ i ].type + " " + varsOut[ i ].nameGLSL + ";\n";
						// ( smooth flat noperspective ) centroid out
					}
				}
				continue;
			}
			else if( src.CheckTokenString( "GS_IN" ) ) //SEA
			{
				/*
				[maxvertexcount(NumVerts)]
				layout(max_vertices=NumVerts) out;
				*/
				ParseInOutStruct( src, AT_GS_IN, AT_GS_IN_RESERVED, varsIn );
				program += "\n";
				for( int i = 0; i < varsIn.Num(); i++ )
				{
					if( varsIn[ i ].declareInOut )
					{
						program += "in " + varsIn[ i ].type + " " + varsIn[ i ].nameGLSL + "[];\n";
						// ( smooth flat noperspective ) centroid out
					}
				}
				continue;
			}
			else if( src.CheckTokenString( "GS_OUT" ) ) //SEA
			{
				ParseInOutStruct( src, AT_GS_OUT, AT_GS_OUT_RESERVED, varsOut );
				program += "\n";
				for( int i = 0; i < varsOut.Num(); i++ )
				{
					if( varsOut[ i ].declareInOut )
					{
						program += "out " + varsOut[ i ].type + " " + varsOut[ i ].nameGLSL + ";\n";
						// ( smooth flat noperspective ) centroid out
					}
				}
				continue;
				// layout( triangle_strip, max_vertices = 60 ) out;   points  line_strip  triangle_strip
			}
			else if( src.CheckTokenString( "PS_IN" ) )
			{
				ParseInOutStruct( src, AT_PS_IN, AT_PS_IN_RESERVED, varsIn );
				program += "\n\n";
				for( int i = 0; i < varsIn.Num(); i++ )
				{
					if( varsIn[ i ].declareInOut )
					{
						program += "in " + varsIn[ i ].type + " " + varsIn[ i ].nameGLSL + ";\n";
						// ( smooth flat noperspective ) centroid out
					}
				}

				inOutVariable_t var;
				var.type = "vec4";
				var.nameCg = "position";
				var.nameGLSL = "gl_FragCoord";
				varsIn.Append( var );
				continue;
				// layout( origin_upper_left, pixel_center_integer ) in vec4 gl_FragCoord;
			}
			else if( src.CheckTokenString( "PS_OUT" ) )
			{
				ParseInOutStruct( src, AT_PS_OUT, AT_PS_OUT_RESERVED, varsOut );
				program += "\n";
				for( int i = 0; i < varsOut.Num(); i++ )
				{
					if( varsOut[ i ].declareInOut )
					{
						program += "out " + varsOut[ i ].type + " " + varsOut[ i ].nameGLSL + ";\n";
						//program += layout[ i ] + varsOut[ i ].type + " " + varsOut[ i ].nameGLSL + ";\n";
					}
				}
				continue;
			}
		}

		// strip 'static'
		if( token == "static" )
		{
			program += CHECK_NEW_LINE_AND_SPACES();
			src.SkipWhiteSpace( true ); // remove white space between 'static' and the next token
			continue;
		}

		// strip ': register()' from uniforms
		if( token == ":" )
		{
			if( src.CheckTokenString( "register" ) )
			{
				src.SkipUntilString( ";" );
				program += ";";
				continue;
			}
		}

		// strip in/program parameters from main
		if( token == "void" && src.CheckTokenString( "main" ) )
		{
			src.ExpectTokenString( "(" );
			while( src.ReadToken( &token ) )
			{
				if( token == ")" )
				{
					break;
				}
			}

			program += "\nvoid main()";
			inMain = true;
			continue;
		}

		// strip 'const' from local variables in main()
		if( token == "const" && inMain && !GLEW_ARB_shading_language_420pack )
		{
			program += CHECK_NEW_LINE_AND_SPACES();
			src.SkipWhiteSpace( true ); // remove white space between 'const' and the next token
			continue;
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

		// check for a type conversion
		bool foundType = false;
		for( int i = 0; typeConversion[ i ].typeCG != NULL; i++ )
		{
			if( token.Cmp( typeConversion[ i ].typeCG ) == 0 )
			{
				program += CHECK_NEW_LINE_AND_SPACES();
				program += typeConversion[ i ].typeGLSL;
				foundType = true;
				break;
			}
		}
		if( foundType )
		{
			continue;
		}

		// check for uniforms that need to be converted to the array
		bool isUniform = false;
		for( int i = 0; i < uniformList.Num(); i++ )
		{
			if( token == uniformList[ i ] )
			{
				program += CHECK_NEW_LINE_AND_SPACES();

				// RB: for some unknown reasons has the Nvidia driver problems with regular uniforms when using glUniform4fv
				// so we need these uniform arrays
				// I added special check rpShadowMatrices so we can still index the uniforms from the shader

				if( idStr::Cmp( uniformList[ i ].c_str(), "rpShadowMatrices" ) == 0 )
				{
					program += idStrStatic<128>().Format( "%s[/* %s */ %d + ", uniformArrayName, uniformList[ i ].c_str(), i );

					if( src.ExpectTokenString( "[" ) )
					{
						idStr uniformIndexing;

						while( src.ReadToken( &token ) )
						{
							if( token == "]" )
							{
								break;
							}

							uniformIndexing += token;
							uniformIndexing += " ";
						}

						program += uniformIndexing + "]";
					}
				}
				else {
					program += va( "%s[%d /* %s */]", uniformArrayName, i, uniformList[ i ].c_str() );
				}
				// RB end
				isUniform = true;
				break;
			}
		}
		if( isUniform )
		{
			continue;
		}

		// check for input/output parameters
		if( src.CheckTokenString( "." ) )
		{
			if( token == "input" )
			{
				idToken member;
				src.ReadToken( &member );

				bool foundInOut = false;
				for( int i = 0; i < varsIn.Num(); i++ )
				{
					if( member.Cmp( varsIn[ i ].nameCg ) == 0 )
					{
						program += CHECK_NEW_LINE_AND_SPACES();
						program += varsIn[ i ].nameGLSL;
						foundInOut = true;
						break;
					}
				}
				if( !foundInOut )
				{
					src.Error( "invalid input parameter %s.%s", token.c_str(), member.c_str() );
					program += CHECK_NEW_LINE_AND_SPACES();
					program += token;
					program += ".";
					program += member;
				}
				continue;
			}

			if( token == "]" && ( shaderType_e::SHT_GEOMETRY || shaderType_e::SHT_TESS_CTRL || shaderType_e::SHT_TESS_EVAL ) )
			{
				// check gs builtins
			}

			if( token == "output" )
			{
				idToken member;
				src.ReadToken( &member );

				bool foundInOut = false;
				for( int i = 0; i < varsOut.Num(); i++ )
				{
					if( member.Cmp( varsOut[ i ].nameCg ) == 0 )
					{
						program += CHECK_NEW_LINE_AND_SPACES();
						program += varsOut[ i ].nameGLSL;
						foundInOut = true;
						break;
					}
				}
				if( !foundInOut )
				{
					src.Error( "invalid output parameter %s.%s", token.c_str(), member.c_str() );
					program += CHECK_NEW_LINE_AND_SPACES();
					program += token;
					program += ".";
					program += member;
				}
				continue;
			}

			program += CHECK_NEW_LINE_AND_SPACES();
			program += token;
			program += ".";
			continue;
		}

		// check for a function conversion
		if( ConvertBuiltin( token, program, newline ) )
		{
			continue;
		}

		program += CHECK_NEW_LINE_AND_SPACES();
		program += token;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////

	idStringBuilder_Heap final;

	// RB: tell shader debuggers what shader we look at
	idStrStatic<128> filenameHint;
	filenameHint.Format("// %s.%s\n", name, glslShaderInfo[ shaderType ].shaderTypeName );
	final += filenameHint;

	// RB: changed to allow multiple versions of GLSL
	if( shaderType == shaderType_e::SHT_VERTEX )
	{
		switch( glConfig.driverType )
		{
			case GLDRV_OPENGL_ES3:
			case GLDRV_OPENGL_MESA:
			{
				///final.ReAllocate( idStr::Length( vertexInsert_GLSL_ES_3_00 ) + in.Length() * 2, false );
				///final += filenameHint;
				///final += vertexInsert_GLSL_ES_3_00;
				final.Append( vertexInsert_GLSL_ES_3_00, idStr::Length( vertexInsert_GLSL_ES_3_00 ) );

				break;
			}

			default:
			{
				///final.ReAllocate( idStr::Length( vertexInsert_GLSL_1_50 ) + in.Length() * 2, false );
				///final += filenameHint;
				///final += vertexInsert_GLSL_1_50;
				final.Append( vertexInsert_GLSL_1_50, idStr::Length( vertexInsert_GLSL_1_50 ) );

				break;
			}
		}
	}
	else if( shaderType == shaderType_e::SHT_GEOMETRY )
	{
		// ...
	}
	else
	{
		switch( glConfig.driverType )
		{
			case GLDRV_OPENGL_ES3:
			case GLDRV_OPENGL_MESA:
			{
				///final.ReAllocate( idStr::Length( fragmentInsert_GLSL_ES_3_00 ) + in.Length() * 2, false );
				///final += filenameHint;
				///final += fragmentInsert_GLSL_ES_3_00;
				final.Append( fragmentInsert_GLSL_ES_3_00, idStr::Length( fragmentInsert_GLSL_ES_3_00 ) );
				break;
			}

			default:
			{
				///final.ReAllocate( idStr::Length( fragmentInsert_GLSL_1_50 ) + in.Length() * 2, false );
				///final += filenameHint;
				///final += fragmentInsert_GLSL_1_50;
				final.Append( fragmentInsert_GLSL_1_50, idStr::Length( fragmentInsert_GLSL_1_50 ) );

				break;
			}
		}
	}

	if( GLEW_ARB_shading_language_420pack )
	{
		final += "\n#extension GL_ARB_shading_language_420pack : require\n";
	}

	if( uniformList.Num() > 0 )
	{
		int extraSize = 0;
		for( int i = 0; i < uniformList.Num(); i++ )
		{
			if( idStr::Cmp( uniformList[ i ].c_str(), "rpShadowMatrices" ) == 0 )
			{
				extraSize += ( 6 * 4 );
			}
		}
		final += idStrStatic<128>().Format( "\nuniform vec4 %s[%d];\n", uniformArrayName, uniformList.Num() + extraSize );
	}

	final += program;

	for( int i = 0; i < uniformList.Num(); i++ )
	{
		uniforms += uniformList[ i ];
		uniforms += "\n";
	}
	uniforms += "\n";

	final.ToString( out );
}

#if 0
/*
================================================================================================
idRenderProgManager::LoadGLSLShader
================================================================================================
*/
GLuint idRenderProgManager::LoadGLSLShader( shaderType_e shaderType, const char* name, const char* nameOutSuffix, uint32 shaderFeatures, bool builtin, idList<int>& uniforms )
{
	idStrStatic<MAX_OSPATH> inFile;
	idStrStatic<MAX_OSPATH> outFileHLSL;
	idStrStatic<MAX_OSPATH> outFileGLSL;
	idStrStatic<MAX_OSPATH> outFileUniforms;

	// RB: replaced backslashes
	inFile.Format( "renderprogs/%s", name ).StripFileExtension();
	outFileHLSL.Format( "renderprogs/hlsl/%s%s", name, nameOutSuffix ).StripFileExtension();

	switch( glConfig.driverType )
	{
		case GLDRV_OPENGL_ES3:
		case GLDRV_OPENGL_MESA:
		{
			outFileGLSL.Format( "renderprogs/glsles-3_00/%s%s", name, nameOutSuffix );
			outFileUniforms.Format( "renderprogs/glsles-3_00/%s%s", name, nameOutSuffix );
			break;
		}
		default:
		{
			outFileGLSL.Format( "renderprogs/glsl-1_50/%s%s", name, nameOutSuffix );
			outFileUniforms.Format( "renderprogs/glsl-1_50/%s%s", name, nameOutSuffix );
		}
	}

	outFileGLSL.StripFileExtension();
	outFileUniforms.StripFileExtension();

	/*/*/if( shaderType == shaderType_e::SHT_FRAGMENT )
	{
		inFile += ".frag";
		outFileHLSL += "_frag.hlsl";
		outFileGLSL += "_frag.glsl";
		outFileUniforms += "_frag.uniforms";
	}
	else if( shaderType == shaderType_e::SHT_GEOMETRY )
	{
		inFile += ".geom";
		outFileHLSL += "_geom.hlsl";
		outFileGLSL += "_geom.glsl";
		outFileUniforms += "_geom.uniforms";
	}
	else if( shaderType == shaderType_e::SHT_TESS_CTRL )
	{
		inFile += ".tess_ctrl";
		outFileHLSL += "_tess_ctrl.hlsl";
		outFileGLSL += "_tess_ctrl.glsl";
		outFileUniforms += "_tess_ctrl.uniforms";
	}
	else if( shaderType == shaderType_e::SHT_TESS_EVAL )
	{
		inFile += ".tess_eval";
		outFileHLSL += "_tess_eval.hlsl";
		outFileGLSL += "_tess_eval.glsl";
		outFileUniforms += "_tess_eval.uniforms";
	}
	else if( shaderType == shaderType_e::SHT_VERTEX )
	{
		inFile += ".vert";
		outFileHLSL += "_vert.hlsl";
		outFileGLSL += "_vert.glsl";
		outFileUniforms += "_vert.uniforms";
	}
	else {
		idLib::Error( "LoadGLSLShader( Unknown shaderType )" );
	}

	// first check whether we already have a valid GLSL file and compare it to the hlsl timestamp;
	ID_TIME_T hlslTimeStamp;
	int hlslFileLength = fileSystem->ReadFile( inFile.c_str(), NULL, &hlslTimeStamp );

	ID_TIME_T glslTimeStamp;
	int glslFileLength = fileSystem->ReadFile( outFileGLSL.c_str(), NULL, &glslTimeStamp );

	idStr programGLSL, programUniforms;

	// if the glsl file doesn't exist or we have a newer HLSL file we need to recreate the glsl file.
	if( ( glslFileLength <= 0 ) ||
		( hlslTimeStamp != FILE_NOT_FOUND_TIMESTAMP && hlslTimeStamp > glslTimeStamp ) ||
		r_alwaysExportGLSL.GetBool() )
	{
		if( hlslFileLength <= 0 )
		{
			// hlsl file doesn't even exist bail out
			return INVALID_PROGID;
		}

		void* hlslFileBuffer = NULL;
		int len = fileSystem->ReadFile( inFile.c_str(), &hlslFileBuffer );
		if( len <= 0 )
		{
			return INVALID_PROGID;
		}

		idStrList compileMacros;
		for( int j = 0; j < MAX_SHADER_MACRO_NAMES; j++ )
		{
			if( BIT( j ) & shaderFeatures )
			{
				const char* macroName = GetGLSLMacroName( ( shaderFeature_t )j );
				compileMacros.Append( idStr( macroName ) );
			}
		}

		idStr programHLSL;
		StripDeadCode( ( const char* )hlslFileBuffer, inFile, compileMacros, builtin, programHLSL );
		Mem_Free( hlslFileBuffer );
		ConvertCG2GLSL( programHLSL, programGLSL, inFile, programUniforms, shaderType );

		fileSystem->WriteFile( outFileHLSL, programHLSL.c_str(), programHLSL.Length(), "fs_savepath" );
		fileSystem->WriteFile( outFileGLSL, programGLSL.c_str(), programGLSL.Length(), "fs_savepath" );
		fileSystem->WriteFile( outFileUniforms, programUniforms.c_str(), programUniforms.Length(), "fs_savepath" );
	}
	else {
		// read in the glsl file
		void* fileBufferGLSL = NULL;
		int lengthGLSL = fileSystem->ReadFile( outFileGLSL.c_str(), &fileBufferGLSL );
		if( lengthGLSL <= 0 )
		{
			idLib::Error( "GLSL file %s could not be loaded and may be corrupt", outFileGLSL.c_str() );
		}
		programGLSL = ( const char* )fileBufferGLSL;
		Mem_Free( fileBufferGLSL );

		// read in the uniform file
		void* fileBufferUniforms = NULL;
		int lengthUniforms = fileSystem->ReadFile( outFileUniforms.c_str(), &fileBufferUniforms );
		if( lengthUniforms <= 0 )
		{
			idLib::Error( "uniform file %s could not be loaded and may be corrupt", outFileUniforms.c_str() );
		}
		programUniforms = ( const char* )fileBufferUniforms;
		Mem_Free( fileBufferUniforms );
	}

	// find the uniforms locations
	uniforms.Clear();

	idLexer src( programUniforms, programUniforms.Length(), "uniforms" );
	idToken token;
	while( src.ReadToken( &token ) )
	{
		int index = -1;
		for( int i = 0; i < RENDERPARM_TOTAL && index == -1; i++ )
		{
			const char* parmName = GetGLSLParmName( i );
			if( token == parmName )
			{
				index = i;
			}
		}
		for( int i = 0; i < MAX_GLSL_USER_PARMS && index == -1; i++ )
		{
			const char* parmName = GetGLSLParmName( RENDERPARM_USER + i );
			if( token == parmName )
			{
				index = RENDERPARM_USER + i;
			}
		}
		if( index == -1 )
		{
			idLib::Error( "couldn't find uniform %s for %s", token.c_str(), outFileGLSL.c_str() );
		}
		uniforms.Append( index );
	}

	return CreateGLSLShaderObject( shaderType, programGLSL.c_str(), inFile.c_str() );
}
#endif



/*
================================================================================================
idRenderProgManager::CommitUnforms
================================================================================================
*/
/*void idRenderProgManager::CommitUniforms()
{
	const auto prog = GL_GetCurrentRenderProgram();

}*/

/*class idSort_QuickUniforms : public idSort_Quick< glslUniformLocation_t, idSort_QuickUniforms > {
public:
	int Compare( const glslUniformLocation_t& a, const glslUniformLocation_t& b ) const
	{
		return a.uniformIndex - b.uniformIndex;
	}
};*/

// Bind vertex input / fragment output locations.
void BindInOutAttributes( GLuint program )
{
	// bind vertex input locations
	for( int i = 0; attribsPC[ i ].glsl != NULL; i++ )
	{
		if( !!( attribsPC[ i ].flags & AT_VS_IN ) && !( attribsPC[ i ].flags & AT_VS_IN_RESERVED ) )
		{
			glBindAttribLocation( program, attribsPC[ i ].bind, attribsPC[ i ].glsl );
		}
	}

	// bind fragment output locations
	for( int i = 0; attribsPC[ i ].glsl != NULL; i++ )
	{
		if( !!( attribsPC[ i ].flags & AT_PS_OUT ) && !( attribsPC[ i ].flags & AT_PS_OUT_RESERVED ) )
		{
			glBindFragDataLocation( program, attribsPC[ i ].bind, attribsPC[ i ].glsl );
		}
	}

	// bind transform feedback attributes
	/*for( size_t i = 0; i < 0; i++ ) {
	const char* varyings[ 3 ] = { "vPosition", "vBirthTime", "vVelocity" };
	glTransformFeedbackVaryings( program, 3, varyings, GL_INTERLEAVED_ATTRIBS ); // GL_SEPARATE_ATTRIBS
	}*/
}

void BindUniformBuffers( GLuint program )
{
	{
	  // get the uniform buffer binding for global parameters
		GLint blockIndex = glGetUniformBlockIndex( program, "global_ubo" );
		if( blockIndex != -1 ) {
			glUniformBlockBinding( program, blockIndex, BINDING_GLOBAL_UBO );
		}
	}

	{
		// get the uniform buffer binding for skinning joint matrices
		GLint blockIndex = glGetUniformBlockIndex( program, "matrices_ubo" );
		if( blockIndex != -1 ) {
			glUniformBlockBinding( program, blockIndex, BINDING_MATRICES_UBO );
		}
		blockIndex = glGetUniformBlockIndex( program, "previous_matrices_ubo" );
		if( blockIndex != -1 ) {
			glUniformBlockBinding( program, blockIndex, BINDING_PREVIOUS_MATRICES_UBO );
		}
	}

	{
		// get the uniform buffer binding for shadow parameters
		GLint blockIndex = glGetUniformBlockIndex( program, "shadow_ubo" );
		if( blockIndex != -1 ) {
			glUniformBlockBinding( program, blockIndex, BINDING_SHADOW_UBO );
		}
	}
}
