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
#pragma hdrstop
#include "precompiled.h"

#include "../tr_local.h"

/*
================================================================================================
	Contains the Program implementation for OpenGL.
================================================================================================
*/

class idRenderProgManagerGLSL {
public:
	idRenderProgManagerGLSL();
	virtual ~idRenderProgManagerGLSL();


	static const int MAX_GLSL_USER_PARMS = 8;
	const char*	GetGLSLParmName( int rp ) const;
	int			GetGLSLCurrentProgram() const { return currentRenderProgram; }
	int			FindGLSLProgram( const char* name, int vIndex, int fIndex );

private:

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
	
	bool	CompileGLSL( GLenum target, const char* name );
	GLuint	LoadGLSLShader( GLenum target, const char* name, const char* nameOutSuffix, uint32 shaderFeatures, bool builtin, idList<int>& uniforms );
	void	LoadGLSLProgram( const int programIndex, const int vertexShaderIndex, const int fragmentShaderIndex );
	
	static const GLuint INVALID_PROGID = MAX_UNSIGNED_TYPE( GLuint );
		
	struct vertexShader_t
	{
		vertexShader_t() : progId( INVALID_PROGID ), usesJoints( false ), optionalSkinning( false ), shaderFeatures( 0 ), builtin( false ) {
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
	struct geometryShader_t 
	{
		geometryShader_t() : progId( INVALID_PROGID ), shaderFeatures( 0 ), builtin( false ) {
		}
		idStr		name;
		idStr		nameOutSuffix;
		GLuint		progId;
		uint32		shaderFeatures;		// RB: Cg compile macros
		bool		builtin;			// RB: part of the core shaders built into the executable
		idList<int>	uniforms;
	};
	struct fragmentShader_t
	{
		fragmentShader_t() : progId( INVALID_PROGID ), shaderFeatures( 0 ), builtin( false ) {
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
			vertexShaderIndex( -1 ),
			geometryShaderIndex( -1 ),
			fragmentShaderIndex( -1 ),
				vertexUniformArray( -1 ),
				geometryUniformArray( -1 ),
				fragmentUniformArray( -1 ) {
		}
		idStr	name;
		GLuint	progId;
		int			vertexShaderIndex;
		int			geometryShaderIndex;
		int			fragmentShaderIndex;
		GLint			vertexUniformArray;
		GLint			geometryUniformArray;
		GLint			fragmentUniformArray;
		idList<glslUniformLocation_t> uniformLocations;
	};
	int	currentRenderProgram;
	idList<glslProgram_t, TAG_RENDER> glslPrograms;
	idStaticList < idVec4, RENDERPARM_USER + MAX_GLSL_USER_PARMS > glslUniforms;
	
	int	currentVertexShader;
	int	currentGeometryShader;
	int	currentFragmentShader;
	idList<vertexShader_t, TAG_RENDER> vertexShaders;
	idList<geometryShader_t, TAG_RENDER> geometryShaders;
	idList<fragmentShader_t, TAG_RENDER> fragmentShaders;
};
