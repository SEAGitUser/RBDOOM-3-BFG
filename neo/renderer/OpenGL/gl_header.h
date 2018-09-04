#ifndef __OGL_COMMON_H__
#define __OGL_COMMON_H__


ID_INLINE GLuint GetGLObject( void * apiObject )
{
#pragma warning( suppress: 4311 4302 )
	return reinterpret_cast<GLuint>( apiObject );
}

struct glslProgram_t
{
	//idArray<uint32, SHT_MAX> shaderChecksum;
	idArray<GLuint, SHT_MAX> shaderObjects;
	//idArray<int16, SHT_MAX>	 uniformArrays;
	//id2DArray<int16, SHT_MAX, 64>::type	uniforms;
	GLuint	programObject;
	int32   parmBlockIndex;

	GLint	binaryLength;
	GLenum	binaryFormat;
	void *	binaryData;

	glslProgram_t() :
		programObject( GL_ZERO ), parmBlockIndex( -1 ),
		binaryLength( 0 ), binaryFormat( 0 ), binaryData( nullptr )
	{
		for( int i = 0; i < shaderObjects.Num(); i++ )
		{
			//shaderChecksum[ i ] = 0;
			shaderObjects[ i ] = GL_NONE;
			//uniformArrays[ i ] = -1;
			//for( int j = 0; j < uniforms[ i ].Num(); j++ ) {
			//	uniforms[ i ][ j ] = -1;
			//}
		}
	}

	void FreeProgObject()
	{
		if( renderSystem->IsRenderDeviceRunning() )
		{
			for( int i = 0; i < shaderObjects.Num(); i++ )
			{
				if( shaderObjects[ i ] )
				{
					glDeleteShader( shaderObjects[ i ] );
					shaderObjects[ i ] = GL_NONE;
				}
			}
			if( programObject )
			{
				glDeleteProgram( programObject );
				programObject = GL_NONE;
			}
		}
		parmBlockIndex = -1;
	}

	size_t Size()
	{
		size_t size = sizeof( glslProgram_t );
		size += ( size_t ) binaryLength;
		return size;
	}

	void operator = ( const glslProgram_t & other )
	{
		for( int i = 0; i < shaderObjects.Num(); i++ )
		{
			shaderObjects[ i ] = other.shaderObjects[ i ];
		}
		programObject = other.programObject;
		parmBlockIndex = other.parmBlockIndex;

		binaryLength = other.binaryLength;
		binaryFormat = other.binaryFormat;
		binaryData = other.binaryData;
	}
};

void GL_CommitProgUniforms( const idDeclRenderProg * );

#endif /*__OGL_COMMON_H__*/
