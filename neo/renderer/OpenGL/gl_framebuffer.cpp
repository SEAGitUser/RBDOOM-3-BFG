
#include "precompiled.h"
#pragma hdrstop

#include "../tr_local.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

idRenderDestinationManager renderDestManager;

static ID_INLINE GLuint GetGLObject( void * apiObject )
{
#pragma warning( suppress: 4311 4302 )
	return reinterpret_cast<GLuint>( apiObject );
}

static void GL_CheckFramebuffer( GLuint fbo, const char * fboName )
{
	GLenum status = glCheckFramebufferStatus( GL_FRAMEBUFFER );
	if( status == GL_FRAMEBUFFER_COMPLETE )
	{
		return;
	}

	switch( status )
	{
		case GL_FRAMEBUFFER_UNDEFINED:
			idLib::Error( "CheckFramebuffer( %s.%u ): GL_FRAMEBUFFER_UNDEFINED", fboName, fbo );
			// is returned if the specified framebuffer is the default read or draw framebuffer,
			// but the default framebuffer does not exist.
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			idLib::Error( "CheckFramebuffer( %s.%u ): GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT", fboName, fbo );
			// is returned if any of the framebuffer attachment points are framebuffer incomplete.
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			idLib::Error( "CheckFramebuffer( %s.%u ): GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT", fboName, fbo );
			// is returned if the framebuffer does not have at least one image attached to it.
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			idLib::Error( "CheckFramebuffer( %s.%u ): GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER", fboName, fbo );
			// is returned if the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for any color
			// attachment point(s) named by GL_DRAW_BUFFERi.
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			idLib::Error( "CheckFramebuffer( %s.%u ): GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER", fboName, fbo );
			// is returned if GL_READ_BUFFER is not GL_NONE and the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE
			// for the color attachment point named by GL_READ_BUFFER.
			break;

		case GL_FRAMEBUFFER_UNSUPPORTED:
			idLib::Error( "CheckFramebuffer( %s.%u ): GL_FRAMEBUFFER_UNSUPPORTED", fboName, fbo );
			// is returned if the combination of internal formats of the attached images
			// violates an implementation-dependent set of restrictions.
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			idLib::Error( "CheckFramebuffer( %s.%u ): GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE", fboName, fbo );
			// is returned if the value of GL_RENDERBUFFER_SAMPLES is not the same for all attached renderbuffers;
			// if the value of GL_TEXTURE_SAMPLES is the not same for all attached textures; or, if the attached images
			// are a mix of renderbuffers and textures, the value of GL_RENDERBUFFER_SAMPLES does not match the value of GL_TEXTURE_SAMPLES.
			//   Also returned if the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not the same for all attached textures; or,
			// if the attached images are a mix of renderbuffers and textures, the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is
			// not GL_TRUE for all attached textures.
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			idLib::Error( "CheckFramebuffer( %s.%u ): GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS", fboName, fbo );
			// is returned if any framebuffer attachment is layered, and any populated attachment is not layered,
			// or if all populated color attachments are not from textures of the same target.
			break;

		default:
			idLib::Error( "CheckFramebuffer( %s.%u ): Unknown error 0x%X", fboName, fbo, status );
			break;
	};
}

// idODSObject<idRenderDestination>

/*
=============================================
 idRenderDestination()
=============================================
*/
idRenderDestination::idRenderDestination( const char *_name )
	: name( _name ), apiObject( nullptr )
{
	Clear();
}
/*
=============================================
 ~idRenderDestination()
=============================================
*/
idRenderDestination::~idRenderDestination()
{
	assert( tr.IsRenderDeviceRunning() );
	if( IsValid() )
	{
		GLuint fbo = GetGLObject( apiObject );
		glDeleteFramebuffers( 1, &fbo );
		apiObject = nullptr;
	}
	name.Clear();
	Clear();
}

/*
=============================================
 Clear
=============================================
*/
void idRenderDestination::Clear()
{
	targetWidth = targetHeight = 0;
	targetList.Clear();
	depthStencilImage = nullptr;
	depthStencilLod = 0;
	isDefault = false;
	isExplicit = false;
}

/*
=============================================
 SetDepthStencil
=============================================
*/
void idRenderDestination::SetDepthStencilGL( idImage * target, int layer, int level )
{
	GLuint texnum = GetGLObject( target->GetAPIObject() );
	GLenum attachment = target->IsDepth() ? GL_DEPTH_ATTACHMENT : GL_DEPTH_STENCIL_ATTACHMENT;

	if( target->IsArray() )
	{
		glFramebufferTextureLayer( GL_FRAMEBUFFER, attachment, texnum, level, layer );
	}
	else {
		assert( layer < 1 );
		glFramebufferTexture( GL_FRAMEBUFFER, attachment, texnum, level );
	}

	depthStencilImage = target;
	depthStencilLod = level;
}



/*
=============================================
 CreateFromImages
	could be called on allready created one to change targets
=============================================
*/
void idRenderDestination::CreateFromImages( const targetList_t *_targetList, idImage * _depthStencil, const int _depthStencilLod )
{
	assert( tr.IsRenderDeviceRunning() );
	assert( !name.IsEmpty() );

	Clear();

	GLuint fbo = GL_NONE;
	isExplicit = true;

	if( apiObject == nullptr )
	{
		glGenFramebuffers( 1, &fbo );
	#pragma warning( suppress: 4312 )
		apiObject = reinterpret_cast< void* >( fbo );
		assert( fbo != GL_NONE );
	}
	else {
		fbo = GetGLObject( apiObject );
	}

	glBindFramebuffer( GL_FRAMEBUFFER, fbo );

	if( GLEW_ARB_framebuffer_no_attachments )
	{
		glFramebufferParameteri( GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, SCREEN_WIDTH );
		glFramebufferParameteri( GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, SCREEN_HEIGHT );
		glFramebufferParameteri( GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_LAYERS, 1 );
		glFramebufferParameteri( GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_SAMPLES, 1 );
		glFramebufferParameteri( GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS, GL_FALSE );
	}

	if( GLEW_KHR_debug )
	{
		idStrStatic<128> name;
		name.Format( "idRenderDestination(\"%s\")(%u)", GetName(), fbo );
		glObjectLabel( GL_FRAMEBUFFER, fbo, name.Length(), name.c_str() );
	}

	if( _targetList != nullptr )
	{
		targetList = *_targetList;
		assert( targetList.Count() < glConfig.maxColorAttachments );
	}

	GLsizei buffersCount = 0;
	GLenum targetBuffers[ 8 ] = { GL_NONE };

	for( int i = 0; i < targetList.Count(); i++ )
	{
		auto & targ = targetList.list[ i ];

		if( targ.image == nullptr ) {
			idLib::Error( "idRenderDestination( %s ): empty target %i", GetName(), i );
		}

		GLuint texnum = GetGLObject( targ.image->GetAPIObject() );

		// if texture is the name of a three-dimensional, cube map array, cube map, two-dimensional array,
		// or two-dimensional multisample array texture, the specified texture level is an array of images,
		// and the framebuffer attachment is considered to be layered.

		targetBuffers[ buffersCount ] = ( GL_COLOR_ATTACHMENT0 + i );
		glFramebufferTexture( GL_FRAMEBUFFER, targetBuffers[ buffersCount ], texnum, targ.mipLevel );
		++buffersCount;

		targetWidth = idMath::Max( targetWidth, targ.image->GetUploadWidth() ) / ( 1 << targ.mipLevel );
		targetHeight = idMath::Max( targetHeight, targ.image->GetUploadHeight() ) / ( 1 << targ.mipLevel );

		/*{
			if( targ.image->GetOpts().textureType == TT_CUBIC )
			{
				glFramebufferTexture2D( GL_FRAMEBUFFER, GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP, texnum, targ.mipLevel );
				glFramebufferTexture2D( GL_FRAMEBUFFER, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP, texnum, targ.mipLevel );
				glFramebufferTexture2D( GL_FRAMEBUFFER, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP, texnum, targ.mipLevel );
				glFramebufferTexture2D( GL_FRAMEBUFFER, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP, texnum, targ.mipLevel );
				glFramebufferTexture2D( GL_FRAMEBUFFER, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP, texnum, targ.mipLevel );
				glFramebufferTexture2D( GL_FRAMEBUFFER, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, GL_TEXTURE_CUBE_MAP, texnum, targ.mipLevel );
			}
		}*/
	}


	if( _depthStencil != nullptr )
	{
		GLenum attachment = _depthStencil->IsDepthStencil() ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
		GLuint texnum = GetGLObject( _depthStencil->GetAPIObject() );
		glFramebufferTexture( GL_FRAMEBUFFER, attachment, texnum, _depthStencilLod );
		depthStencilImage = _depthStencil;
		depthStencilLod = _depthStencilLod;
	}


	if( buffersCount )
	{
		glDrawBuffers( buffersCount, targetBuffers );
	}
	else {
		glDrawBuffer( GL_NONE );
		glReadBuffer( GL_NONE );

		if( _depthStencil != nullptr )
		{
			targetWidth = _depthStencil->GetUploadWidth();
			targetHeight = _depthStencil->GetUploadHeight();
		}
		else assert("no colors, no depth, no stencil - wtf?");
	}

	GL_CheckFramebuffer( fbo, GetName() );
	glBindFramebuffer( GL_FRAMEBUFFER, GL_NONE );
}

/*
=============================================
 Create
=============================================
*//*
void idRenderDestination::Create( const idStr & name, int width, int height, renderClass_t texClass, const idImageOpts & opts )
{
	switch( texClass )
	{
		case( TC_DEPTH ):
		{
			//idImage* depth = renderImageManager->CreateStandaloneImage("_rd_depth");
			//depth->GenerateImage( nullptr, width, height, 6, 1, TF_LINEAR, TR_CLAMP, TD_SHADOWMAP );
			//CreateFromImages( 0, nullptr, depth, nullptr );
		} break;
		case( TC_RGBA ):
		{
			//idImage* color;
			//CreateFromImages( 1, &color, nullptr, nullptr );
		} break;
		case( TC_RGBA_DEPTH ):
		{
			//idImage * color, * depthStecil;
			//CreateFromImages( 1, &color, depthStecil, depthStecil );
		} break;
		case( TC_FLOAT_RGBA ):
		{
			//idImage * color;
			//CreateFromImages( 1, &color, nullptr, nullptr );
		} break;
		case( TC_FLOAT_RGBA_DEPTH ):
		{
			//idImage * color, *depthStecil;
			//CreateFromImages( 1, &color, depthStecil, depthStecil );
		} break;
		case( TC_FLOAT16_RGBA ):
		{
			//idImage * color;
			//CreateFromImages( 1, &color, nullptr, nullptr );
		} break;
		case( TC_FLOAT16_RGBA_DEPTH ):
		{
			//idImage * color, *depthStecil;
			//CreateFromImages( 1, &color, depthStecil, depthStecil );
		} break;

		default:
			release_assert("unknown renderClass")
			break;
	}

	isExplicit = false;
}*/

/*
=================================================
 idRenderDestination::Print()
=================================================
*/
void idRenderDestination::Print() const
{
	for( int i = 0; i < targetList.Count(); i++ )
	{
		auto & trg = targetList.list[ i ];

		if( trg.image != nullptr )
		{
			common->Printf( S_COLOR_GRAY " - target%i mip(%i) leyer(%i) %s" S_COLOR_DEFAULT "\n", i, trg.mipLevel, trg.layer, trg.image->GetName() );
		}
	}
	if( depthStencilImage != nullptr )
	{
		common->Printf( S_COLOR_GRAY " - depthStencilImage lod(%i): %s" S_COLOR_DEFAULT "\n", depthStencilLod, depthStencilImage->GetName() );
	}
}