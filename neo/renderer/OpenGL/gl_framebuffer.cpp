
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

static void _CheckFramebuffer( GLuint fbo, const char * fboName )
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
	assert( tr.IsOpenGLRunning() );
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
	depthImage = nullptr;
	stencilImage = nullptr;
	boundSide = 0;
	boundLevel = 0;
	isDefault = false;
	isExplicit = false;
}


/*
=============================================
 AddTarget
=============================================
*/
void idRenderDestination::SetTargetOGL( idImage * target, int iTarget, int layer, int level )
{
	GLuint texnum = GetGLObject( target->GetAPIObject() );

	if( target->IsDepth() )
	{
		glFramebufferTextureLayer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texnum, level, layer );
		depthImage = target;
	}
	else if( target->IsDepthStencil() )
	{
		glFramebufferTextureLayer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, texnum, level, layer );
		depthImage = stencilImage = target;
	}
	else {
		glFramebufferTextureLayer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + iTarget, texnum, level, layer );
		targetList.list[ iTarget ].image = target;
		targetList.list[ iTarget ].layer = layer;
		targetList.list[ iTarget ].mipLevel = level;
		release_assert( targetList.Count() > iTarget );
	}
}

/*
=============================================
 CreateFromImages	
	could be called on allready created one to change targets
=============================================
*/
void idRenderDestination::CreateFromImages( const targetList_t *_targetList, idImage* depth, idImage* stencil )
{
	release_assert( tr.IsOpenGLRunning() );
	///release_assert( apiObject == nullptr );
	release_assert( !name.IsEmpty() );

	Clear();

	GLuint fbo = GL_NONE;
	isExplicit = true;

	if( apiObject == nullptr )
	{
		glGenFramebuffers( 1, &fbo );
	#pragma warning( suppress: 4312 )
		apiObject = reinterpret_cast< void* >( fbo );
		release_assert( fbo != GL_NONE );
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
		release_assert( targetList.Count() < glConfig.maxColorAttachments );
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

		targetBuffers[ buffersCount ] = ( GL_COLOR_ATTACHMENT0 + i );
		glFramebufferTexture( GL_FRAMEBUFFER, targetBuffers[ buffersCount ], texnum, targ.mipLevel );			
		buffersCount++;

		targetWidth = idMath::Max( targetWidth, targ.image->GetUploadWidth() ) / ( 1 << targ.mipLevel );
		targetHeight = idMath::Max( targetHeight, targ.image->GetUploadHeight() ) / ( 1 << targ.mipLevel );
	}

	if( depth == stencil )
	{
		if( depth != nullptr )
		{
			release_assert( depth->GetOpts().IsDepthStencil() );
			GLuint texnum = GetGLObject( depth->GetAPIObject() );
			glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, texnum, 0/*mip*/ );
			depthImage = stencilImage = depth;
		}
	}
	else {
		if( depth != nullptr )
		{
			GLuint texnum = GetGLObject( depth->GetAPIObject() );
			glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texnum, 0/*mip*/ );
			depthImage = depth;
		}
		if( stencil != nullptr )
		{
			GLuint texnum = GetGLObject( stencil->GetAPIObject() );
			glFramebufferTexture( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, texnum, 0/*mip*/ );
			stencilImage = stencil;
		}
	}

	if( buffersCount )
	{
		glDrawBuffers( buffersCount, targetBuffers );
	}
	else {
		glDrawBuffer( GL_NONE );
		glReadBuffer( GL_NONE );

		if( depth != nullptr )
		{
			targetWidth = depth->GetUploadWidth();
			targetHeight = depth->GetUploadHeight();
		}
		else if( stencil != nullptr )
		{
			targetWidth = stencil->GetUploadWidth();
			targetHeight = stencil->GetUploadHeight();
		} 
		else release_assert("no colors, no depth, no stencil - wtf?");
	}

	_CheckFramebuffer( fbo, GetName() );
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
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
 Print
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
	if( depthImage != nullptr )
	{
		common->Printf( S_COLOR_GRAY " - depthImage: %s" S_COLOR_DEFAULT "\n", depthImage->GetName() );
	}
	if( stencilImage != nullptr )
	{
		common->Printf( S_COLOR_GRAY " - stencilImage: %s" S_COLOR_DEFAULT "\n", stencilImage->GetName() );
	}
}