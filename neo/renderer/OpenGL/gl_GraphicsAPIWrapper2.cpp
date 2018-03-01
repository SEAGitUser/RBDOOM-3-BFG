
#pragma hdrstop
#include "precompiled.h"

#include "../tr_local.h"

// Experimental staff //////////////////////////////////////////////////////////////

static ID_INLINE GLuint GetGLObject( void * apiObject )
{
#pragma warning( suppress: 4311 4302 )
	return reinterpret_cast<GLuint>( apiObject );
}


/*

GL_GetCurrentQueryNumber();
GL_CacheOcclusionQueryBatches( uint32 upToIncludingBatchNum, uint32 & available );
GL_BeginQueryBatch();
GL_BeginQuery( uint32 queryNumber );
GL_EndQuery();
GL_GetDeferredQueryResult( uint32 queryNumber, uint32 & result );

GL_BindProgram( prog, extraState, triVertexMask, blockParms );
GL_SetupSurfaceParms prog, idRenderModelSurface surf __glewBindBufferRange
GL_ClearSurfaceParms prog, idRenderModelSurface surf

GL_DeleteVertexBufferVAO apiObject
GL_DeleteIndexBufferVAO apiObject
GL_BuildVAO( vertexBuffer indexBuffer morphMap stMap vertexMask vao );

GL_DrawElementsInternal( tri, vao, progVertexMask, skipDetailTriangles );
GL_DrawElements( idODSObject<idDeclRenderProg> prog, idODSObject<idRenderModelSurface> sur, extraState, skipDetailTriangles );
GL_DrawElements( idODSObject<idDeclRenderProg> prog, idODSObject<idTriangles> tri, extraState, skipDetailTriangles );

GLEW_NVX_gpu_memory_info
GLEW_ATI_meminfo

*/

/////////////////////////////////////////////////////////////////////////////////////////////////////

static const uint32 MAX_MULTIDRAW_COUNT = 4096;
struct multiDrawParms_t {
	int		count[ MAX_MULTIDRAW_COUNT ];
	void *	indices[ MAX_MULTIDRAW_COUNT ];
	int		basevertexes[ MAX_MULTIDRAW_COUNT ];
	int		primcount;
	GLuint	vbo, ibo;
	//vertexLayoutType_t	vertexLayout;
} drawCalls;
//static const size_t multiDrawParmsSize = SIZE_KB( sizeof( multiDrawParms_t ) );

/*
============================================================
GL_StartMDBatch
============================================================
*/
void GL_StartMDBatch()
{
	drawCalls.vbo = 0;
	drawCalls.ibo = 0;
	drawCalls.primcount = 0;
}

/*
============================================================
GL_SetupDrawCall
============================================================
*/
void GL_SetupMDParm( const drawSurf_t * const surf )
{
	idVertexBuffer vertexBuffer;
	vertexCache.GetVertexBuffer( surf->vertexCache, &vertexBuffer );
	const GLint vertOffset = vertexBuffer.GetOffset();
	const GLuint vbo = GetGLObject( vertexBuffer.GetAPIObject() );

	idIndexBuffer indexBuffer;
	vertexCache.GetIndexBuffer( surf->indexCache, &indexBuffer );
	const GLintptr indexOffset = indexBuffer.GetOffset();
	const GLuint ibo = GetGLObject( indexBuffer.GetAPIObject() );

	//const bool nextBatch((( drawCalls.vbo != vbo ) || ( drawCalls.ibo != ibo )) && drawCalls.primcount != 1 );
	//release_assert( nextBatch == false );

	drawCalls.count[ drawCalls.primcount ] = r_singleTriangle.GetBool() ? 3 : surf->numIndexes;
	drawCalls.indices[ drawCalls.primcount ] = ( triIndex_t* )indexOffset;
	drawCalls.basevertexes[ drawCalls.primcount ] = vertOffset / sizeof( idDrawVert );

	drawCalls.primcount++;

	drawCalls.vbo = vbo;
	drawCalls.ibo = ibo;

	backEnd.pc.c_drawElements++;
	backEnd.pc.c_drawIndexes += surf->numIndexes;

	RENDERLOG_PRINT( "SetDrawCall( VBO %u:%i, IBO %u:%i, numIndexes:%i )\n", vbo, vertOffset, ibo, indexOffset, surf->numIndexes );
}

/*
============================================================
GL_MultiDrawElementsWithCounters
============================================================
*/
void GL_FinishMDBatch()
{
	if( !drawCalls.primcount )
		return;

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, drawCalls.ibo );
	backEnd.glState.currentIndexBuffer = drawCalls.ibo;

	if( glConfig.ARB_vertex_attrib_binding && r_useVertexAttribFormat.GetBool() )
	{
		const GLintptr baseOffset = 0;
		const GLuint bindingIndex = 0;
		glBindVertexBuffer( bindingIndex, drawCalls.vbo, baseOffset, sizeof( idDrawVert ) );
	}
	else {
		glBindBuffer( GL_ARRAY_BUFFER, drawCalls.vbo );
	}
	backEnd.glState.currentVertexBuffer = ( GLintptr )drawCalls.vbo;

	GL_SetVertexLayout( LAYOUT_DRAW_VERT_POSITION_ONLY );
	backEnd.glState.vertexLayout = LAYOUT_DRAW_VERT_POSITION_ONLY;

	glMultiDrawElementsBaseVertex( GL_TRIANGLES,
		drawCalls.count,
		GL_INDEX_TYPE,
		drawCalls.indices,
		drawCalls.primcount,
		drawCalls.basevertexes );

	RENDERLOG_PRINT( "GL_FinishMDBatch( Count:%i ) -----------------------------\n", drawCalls.primcount );
	RENDERLOG_PRINT( "----------------------------------------------------------\n" );
}


#if 0
void GL_DrawScreenTexture( idImage *img, float img_s0, float img_t0, float img_s1, float img_t1, float dest_x0, float dest_y0, float dest_x1, float dest_y1 )
{
	if( GLEW_NV_draw_texture )
	{
		/*
		glDrawTexture( texture, sampler,
		0, 0, 640, 480,  // destination
		0, 0, 640, 480   // texture
		);

		For textures with a TEXTURE_2D target, we use normalized coordinates.
		The same example as above with a 640x480 2D texture would use :

		glDrawTexture( texture, sampler,
		0, 0, 640, 480,  // destination
		0, 0, 1, 1       // texture
		);
		*/
		const GLuint srcTexture = GetGLObject( img->GetAPIObject() );
		const GLuint srcSampler = 0;

		glDrawTextureNV( srcTexture, srcSampler,
			dest_x0, dest_y0, dest_x1, dest_y1,
			0, // z
			img_s0, img_t0, img_s1, img_t1 );
	}
}
#endif




void GL_Qube( int instanceCount )
{
#if 0
	/*
	// Cube vertex shader
	#version 420

	uniform mat4 projection_matrix;
	uniform mat4 model_matrix;

	void main()
	{
	int tri = gl_VertexID / 3;
	int idx = gl_VertexID % 3;
	int face = tri / 2;
	int top = tri % 2;

	int dir = face % 3;
	int pos = face / 3;

	int nz = dir >> 1;
	int ny = dir & 1;
	int nx = 1 ^ (ny | nz);

	vec3 d = vec3(nx, ny, nz);
	float flip = 1 - 2 * pos;

	vec3 n = flip * d;
	vec3 u = -d.yzx;
	vec3 v = flip * d.zxy;

	float mirror = -1 + 2 * top;
	vec3 xyz = n + mirror*(1-2*(idx&1))*u + mirror*(1-2*(idx>>1))*v;

	gl_Position = projection_matrix * model_matrix * vec4( xyz, 1.0 );
	}

	// Wireframe cube
	#version 420

	uniform mat4 projection_matrix;
	uniform mat4 model_matrix;

	void main() {
	vec4 pos = vec4(0.0, 0.0, 0.0, 1.0);
	int axis = gl_VertexID >> 3;
	pos[(axis+0)%3] = (gl_VertexID & 1) >> 0;
	pos[(axis+1)%3] = (gl_VertexID & 2) >> 1;
	pos[(axis+2)%3] = (gl_VertexID & 4) >> 2;

	gl_Position = projection_matrix * model_matrix * (vec4(-1.0) + 2.0*pos);
	}
	glDrawArrays( 0, 24 );
	*/
	backEnd.glState.currentVertexBuffer = ( GLintptr )0;
	backEnd.glState.currentIndexBuffer = ( GLintptr )0;
	backEnd.glState.vertexLayout = LAYOUT_UNKNOWN;

	// disable all vertex arrays
	glDisableVertexAttribArray( PC_ATTRIB_INDEX_POSITION );
	glDisableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
	glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
	glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
	glDisableVertexAttribArray( PC_ATTRIB_INDEX_ST );
	glDisableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );

	glDrawArrays( GL_TRIANGLES, 0, 36 );

#elif 0
	// draw a unit sized cube with a single draw call (a triangle strip with 14 vertices) without any vertex or index buffers.
	/*
	#version 330

	uniform mat4 uModelViewProjection;

	out vec3 vsTexCoord;
	#define oTexCoord vsTexCoord

	void main()
	{
	// extract vertices
	int r = int( gl_VertexID > 6 );
	int i = r == 1 ? 13 - gl_VertexID : gl_VertexID;
	int x = int( i<3 || i == 4 );
	int y = r ^ int( i>0 && i<4 );
	int z = r ^ int( i<2 || i>5 );

	// compute world pos and project
	const float SKY_SIZE = 100.0;
	oTexCoord = vec3( x, y, z )*2.0 - 1.0;
	gl_Position = uModelViewProjection * vec4( oTexCoord * SKY_SIZE, 1 );
	}

	#version 330

	uniform samplerCube sSky;

	in vec3 vsTexCoord;
	#define iTexCoord vsTexCoord
	layout(location=0) out vec4 oColour;

	void main() {
	oColour = texture(sSky, normalize(iTexCoord));
	}
	*/
	//glUseProgram( skyboxProgram );

	// init
	//GLuint empty_vao;
	//glBindVertexArray( empty_vao );
	//glBindVertexArray( GL_NONE );

	// render
	glBindVertexArray( glConfig.empty_vao );
	glDrawArrays( GL_TRIANGLE_STRIP, 0, 14 );

	/*
	// 14 tristrip cube in the vertex shader using only vertex ID (no UVs or normals):
	b = 1 << i;
	x = ( 0x287a & b ) != 0;
	y = ( 0x02af & b ) != 0;
	z = ( 0x31e3 & b ) != 0;

	*/
#endif
	/*
	// GS

	struct GSIn
	{
	float4 R0 : XFORM0;
	float4 R1 : XFORM1;
	float4 R2 : XFORM2;
	};
	struct GSOut
	{
	float4 v : SV_Position;
	};

	void emit( inout TriangleStream<GSOut> triStream, float4 v )
	{
	GSOut s;
	s.v = v;
	triStream.Append(s);
	}

	uniform row_major float4x4 g_ViewProj;
	void GenerateTransformedBox( out float4 v[8], float4 R0, float4 R1, float4 R2 )
	{
	float4 center =float4( R0.w,R1.w,R2.w,1);
	float4 X = float4( R0.x,R1.x,R2.x,0);
	float4 Y = float4( R0.y,R1.y,R2.y,0);
	float4 Z = float4( R0.z,R1.z,R2.z,0);
	center = mul( center, g_ViewProj );
	X = mul( X, g_ViewProj );
	Y = mul( Y, g_ViewProj );
	Z = mul( Z, g_ViewProj );

	float4 t1 = center - X - Z ;
	float4 t2 = center + X - Z ;
	float4 t3 = center - X + Z ;
	float4 t4 = center + X + Z ;
	v[0] = t1 + Y;
	v[1] = t2 + Y;
	v[2] = t3 + Y;
	v[3] = t4 + Y;
	v[4] = t1 - Y;
	v[5] = t2 - Y;
	v[6] = t4 - Y;
	v[7] = t3 - Y;
	}
	// http://www.asmcommunity.net/forums/topic/?id=6284
	static const int INDICES[14] =
	{
	4, 3, 7, 8, 5, 3, 1, 4, 2, 7, 6, 5, 2, 1,
	};

	[maxvertexcount(14)]
	void main( point GSIn box[1], inout TriangleStream<GSOut> triStream )
	{
	float4 v[8];
	GenerateTransformedBox( v, box[0].R0, box[0].R1, box[0].R2 );

	//  Indices are off by one, so we just let the optimizer fix it
	[unroll]
	for( int i=0; i<14; i++ )
	emit(triStream, v[INDICES[i]-1] );
	}
	*/


	renderProgManager.CommitUniforms();

	// No buffers, no vertex attributes

	static GLuint currentVAO = 0;
	if( currentVAO != glConfig.empty_vao )
	{
		glBindVertexArray( glConfig.empty_vao );
		currentVAO = glConfig.empty_vao;
	}
	glDrawArraysInstanced( GL_TRIANGLE_STRIP, 0, 14, instanceCount );
}



void GL_Quad()
{
/*
	2-----4
	| \   |
	|   \ |
	1-----3

	//Vertices below are in Clockwise orientation
	//Default setting for glFrontFace is Counter-clockwise
	glFrontFace(GL_CW);

	glBegin(GL_TRIANGLE_STRIP);
		glVertex3f( 0.0f, 1.0f, 0.0f ); //vertex 1
		glVertex3f( 0.0f, 0.0f, 0.0f ); //vertex 2
		glVertex3f( 1.0f, 1.0f, 0.0f ); //vertex 3
		glVertex3f( 1.5f, 0.0f, 0.0f ); //vertex 4
	glEnd();
*/
}

void GL_ScreenQuad()
{
/*
	// A vertex shader for full-screen effects without a vertex buffer.  The
	// intent is to output an over-sized triangle that encompasses the entire
	// screen.  By doing so, we avoid rasterization inefficiency that could
	// result from drawing two triangles with a shared edge.
	//
	// Use null input layout
	// Draw(3)

	#include "PresentRS.hlsli"

	[RootSignature(Present_RootSig)]
	void main(
		in uint VertID : SV_VertexID,
		out float4 Pos : SV_Position,
		out float2 Tex : TexCoord0
	)
	{
		// Texture coordinates range [0, 2], but only [0, 1] appears on screen.
		Tex = float2(uint2(VertID, VertID << 1) & 2);
		Pos = float4(lerp(float2(-1, 1), float2(1, -1), Tex), 0, 1);
	}

	-----------------------------------------------------
	A vertex shader that uses the vertex ID to generate a full-screen quad. Don't bind vertex buffer,
	index buffer or input layout. Just render three vertices!

	struct Output
	{
		float4 position_cs : SV_POSITION;
		float2 texcoord : TEXCOORD;
	};

	Output main(uint id: SV_VertexID)
	{
		Output output;

		output.texcoord = float2((id << 1) & 2, id & 2);
		output.position_cs = float4(output.texcoord * float2(2, -2) + float2(-1, 1), 0, 1);

		return output;
	}
	---------------------------------------------------------
	To output a fullscreen quad geometry shader can be used:

	#version 330 core

	layout(points) in;
	layout(triangle_strip, max_vertices = 4) out;

	out vec2 texcoord;

	void main()
	{
		gl_Position = vec4( 1.0, 1.0, 0.5, 1.0 );
		texcoord = vec2( 1.0, 1.0 );
		EmitVertex();

		gl_Position = vec4(-1.0, 1.0, 0.5, 1.0 );
		texcoord = vec2( 0.0, 1.0 );
		EmitVertex();

		gl_Position = vec4( 1.0,-1.0, 0.5, 1.0 );
		texcoord = vec2( 1.0, 0.0 );
		EmitVertex();

		gl_Position = vec4(-1.0,-1.0, 0.5, 1.0 );
		texcoord = vec2( 0.0, 0.0 );
		EmitVertex();

		EndPrimitive();
	}
	Vertex shader is just empty:

	#version 330 core

	void main()
	{
	}
	To use this shader you can use dummy draw command with empty VBO:

	glDrawArrays(GL_POINTS, 0, 1);

	----------------------------------------------------------
	render a full-screen quad:

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	#version 330 core
	const vec2 quadVertices[4] = { vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(-1.0, 1.0), vec2(1.0, 1.0) };
	void main()
	{
		gl_Position = vec4( quadVertices[ gl_VertexID ], 0.0, 1.0 );
	}
*/
}
/////////////////////////////////////////////////////////////////////////////////////////////////////

uint32 GetCapacity( primitiveType_e primitiveType )
{
	switch( primitiveType )
	{
		case PRIM_TYPE_POINTS: 
			return 1;

		case PRIM_TYPE_LINE_STRIP:
		///case PRIM_TYPE_LINE_LOOP:
		case PRIM_TYPE_LINES:
		case PRIM_TYPE_LINES_ADJACENCY:
		case PRIM_TYPE_LINE_STRIP_ADJACENCY:
			return 2;

		case PRIM_TYPE_TRIANGLE_STRIP:
		///case PRIM_TYPE_TRIANGLE_FAN:
		case PRIM_TYPE_TRIANGLES:
		case PRIM_TYPE_TRIANGLES_ADJACENCY:
		case PRIM_TYPE_TRIANGLE_STRIP_ADJACENCY:
			return 3;
	}
	return 0;
}


ID_INLINE static byte ColorFloatToByte( float c )
{
	return idMath::Ftob( c * 255.0f );
}

struct vertex_t 
{
	idVec3	xyz;			// 12 bytes
	idVec2  st;				// 8 bytes
	byte	color[ 4 ];		// 4 bytes

	ID_INLINE void SetColor( float r, float g, float b, float a )
	{
		color[ 0 ] = idMath::Ftob( r * 255.0f );
		color[ 1 ] = idMath::Ftob( g * 255.0f );
		color[ 2 ] = idMath::Ftob( b * 255.0f );
		color[ 3 ] = idMath::Ftob( a * 255.0f );
	}

	ID_INLINE void Clear()
	{
		xyz.Set( 0.0, 0.0, 0.0 );
		st.Set( 0.0, 0.0 );
		SetColor( 1.0f, 1.0f, 1.0f, 1.0f );
	}
};

/////////////////////////////////////////////
//SEA: TODO

class idImmediateRenderOGL : public idImmediateRender
{
	primitiveType_e	primitiveType;
	idVertexBuffer	 vertexBuffer;
	void *	   mappedVertexBuffer;
	uint32		   vertexCapacity;

	vertexMask_t vertexMask;
	uint32		currentVertexNum = 0;
	vertex_t	currentVertex;

	static const int MAX_VERT_BUFFER_SIZE = 0;

public:

	idImmediateRenderOGL()
	{
		vertexBuffer.AllocBufferObject( nullptr, ALIGN( MAX_VERT_BUFFER_SIZE, 16 ) );
	}
	~idImmediateRenderOGL()
	{
		vertexBuffer.FreeBufferObject();
	}

	void Begin( primitiveType_e primType );
	void End();

	void Vertex3f( float x, float y, float z );
	void Color4f( float r, float g, float b, float a );

	ID_INLINE void Vertex2fv( const float v[2] ) {
		Vertex3f( v[ 0 ], v[ 1 ], 0.0f );
	}
	ID_INLINE void Vertex3fv( const float v[3] ) {
		Vertex3f( v[ 0 ], v[ 1 ], v[ 2 ] );
	}

	ID_INLINE void Color4ubv( const float v[ 3 ] )
	{
		Vertex3f( v[ 0 ], v[ 1 ], v[ 2 ] );
	}
	ID_INLINE void glColor3f( const float v[ 3 ] )
	{
		Color4f( v[ 0 ], v[ 1 ], v[ 2 ], 1.0f );
	}
	ID_INLINE void glColor4fv( const float v[ 4 ] )
	{
		Color4f( v[ 0 ], v[ 1 ], v[ 2 ], 1.0f );
	}
};

void idImmediateRenderOGL::Begin( primitiveType_e primType )
{
	primitiveType = primType;
	mappedVertexBuffer = vertexBuffer.MapBuffer( BM_WRITE_NOSYNC );
}

void idImmediateRenderOGL::End()
{
	if( vertexBuffer.IsMapped() ) 
	{
		vertexBuffer.UnmapBuffer();
		mappedVertexBuffer = nullptr;
	}

	if( vertexMask.HasFlag( VERTEX_MASK_POSITION ) ) {
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_POSITION );
		glVertexAttribPointer( PC_ATTRIB_INDEX_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof( vertex_t ), ( const void * )0 );
	} else {
		glDisableVertexAttribArray( PC_ATTRIB_INDEX_POSITION );
	}

	if( vertexMask.HasFlag( VERTEX_MASK_ST ) ) {
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_ST );
		glVertexAttribPointer( PC_ATTRIB_INDEX_ST, 2, GL_FLOAT, GL_FALSE, sizeof( vertex_t ), ( const void * )12 );
	} else {
		glDisableVertexAttribArray( PC_ATTRIB_INDEX_ST );
	}

	if( vertexMask.HasFlag( VERTEX_MASK_COLOR ) ) {
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
		glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( vertex_t ), ( const void * )20 );
	} else {
		glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
	}

	{
		struct MultiDrawElements {
			GLenum			primType = GL_TRIANGLES;
			const GLint *	first = nullptr; 
			const GLsizei * vertCount = 0;
			GLsizei			drawCount = 0;
		} draw;

		glMultiDrawArrays( draw.primType, draw.first, draw.vertCount, draw.drawCount );
	}
	{
		struct MultiDrawElements {
			GLenum		primType = GL_TRIANGLES;
			GLsizei		primcount = 0;
			GLsizei *	indexCount = nullptr;
			GLenum		indexType = GL_INDEX_TYPE;
			void **		indices = nullptr;
		} draw;

		glMultiDrawElements( draw.primType, draw.indexCount, draw.indexType, draw.indices, draw.primcount );
	}
	{
		struct MultiDrawElementsBaseVertex {
			GLenum		primType = GL_TRIANGLES;
			GLsizei		primcount = 0;
			GLsizei *	indexCount = nullptr;
			GLenum		indexType = GL_INDEX_TYPE;
			void **		indices = nullptr;
			int *		baseVertex = nullptr;
		} draw;

		glMultiDrawElementsBaseVertex( draw.primType, draw.indexCount, draw.indexType, draw.indices, draw.primcount, draw.baseVertex );
	}
	currentVertex.Clear();
	currentVertexNum = 0;
}


// The current color, normal, and texture coordinates are associated with the vertex 
// when glVertex is called.
void idImmediateRenderOGL::Vertex3f( float x, float y, float z )
{
	assert( mappedVertexBuffer != nullptr );

	currentVertex.xyz[0] = x;
	currentVertex.xyz[1] = y;
	currentVertex.xyz[2] = z;

	vertex_t * vert = (( vertex_t * )mappedVertexBuffer ) + currentVertexNum;
	*vert = currentVertex;

	currentVertex.Clear();
	++currentVertexNum;
}
