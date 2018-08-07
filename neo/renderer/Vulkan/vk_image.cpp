
#include "precompiled.h"
#pragma hdrstop

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#include "../../sys/win32/win_local.h"

#elif defined( __ANDROID__ )
#define VK_USE_PLATFORM_ANDROID_KHR

#else
#define VK_USE_PLATFORM_XCB_KHR
#endif

#include "vk_header.h"

#include <vector>
#include <string>

#include "..\tr_local.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

VkFormat VK_GetFormat( idImage * img )
{
	const bool sRGB = glConfig.sRGBFramebufferAvailable && ( r_useSRGB.GetInteger() == 1 || r_useSRGB.GetInteger() == 3 );

	switch( img->GetOpts().format )
	{
		case FMT_RGBA8:
			return sRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;

		case FMT_XRGB8:
			return sRGB ? VK_FORMAT_R8G8B8_SRGB : VK_FORMAT_R8G8B8_UNORM; // RGBx ?

		case FMT_RGB565: // light textures typically
			return VK_FORMAT_R5G6B5_UNORM_PACK16;

		case FMT_ALPHA:
			return VK_FORMAT_R8_UNORM;

		case FMT_L8A8:
			return VK_FORMAT_R8G8_UNORM;

		case FMT_LUM8:
			return VK_FORMAT_R8_UNORM;

		case FMT_INT8:
			return VK_FORMAT_R8_SINT;

		case FMT_DXT1:
			return sRGB ? VK_FORMAT_BC1_RGB_SRGB_BLOCK : VK_FORMAT_BC1_RGB_UNORM_BLOCK;

		case FMT_DXT5:
			return ( sRGB && img->GetOpts().colorFormat != CFM_YCOCG_DXT5 && img->GetOpts().colorFormat != CFM_NORMAL_DXT5 )? VK_FORMAT_BC3_SRGB_BLOCK : VK_FORMAT_BC3_UNORM_BLOCK;

		case FMT_DEPTH:
			return VK_FORMAT_D32_SFLOAT;
		case FMT_DEPTH_STENCIL:
			return VK_FORMAT_D24_UNORM_S8_UINT;

		case FMT_RGB10_A2://SEA
			return VK_FORMAT_A2R10G10B10_UINT_PACK32; // VK_FORMAT_A2R10G10B10_USCALED_PACK32 / VK_FORMAT_A2R10G10B10_UNORM_PACK32
		case FMT_RG11B10F://SEA
			return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
		case FMT_RG16F://SEA
			return VK_FORMAT_R16G16_SFLOAT;
		case FMT_RGBA16F:
			return VK_FORMAT_R16G16B16A16_SFLOAT;
		case FMT_RGBA32F:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
		case FMT_R32F:
			return VK_FORMAT_R32_SFLOAT;

		case FMT_X16:
			return VK_FORMAT_R16_UNORM;
		case FMT_Y16_X16:
			return VK_FORMAT_R16G16_UNORM;
	}

	idLib::Error( "Unhandled image format %d in %s\n", img->GetOpts().format, img->GetName() );
	return VK_FORMAT_R8G8B8A8_UNORM;
}

VkSampleCountFlagBits VK_GetSampleCount( idImage * img )
{
	int numSamples = idMath::ClampInt( 1, 16, img->GetOpts().numSamples );

	/*/*/if( numSamples < 64 && numSamples > 16 )
	{
		return VK_SAMPLE_COUNT_32_BIT;
	}
	else if( numSamples < 32 && numSamples > 8 )
	{
		return VK_SAMPLE_COUNT_16_BIT;
	}
	else if( numSamples < 16 && numSamples > 4 )
	{
		return VK_SAMPLE_COUNT_8_BIT;
	}
	else if( numSamples < 8 && numSamples > 2 )
	{
		return VK_SAMPLE_COUNT_4_BIT;
	}
	else if( numSamples < 4 && numSamples > 1 )
	{
		return VK_SAMPLE_COUNT_2_BIT;
	}

	assert( numSamples < 2 );
	return VK_SAMPLE_COUNT_1_BIT;
}

VkImageViewType VK_GetImgViewType( idImage * img )
{
	if( img->GetOpts().textureType == TT_2D )
	{
		return ( img->GetOpts().IsArray() )? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
	}
	else if( img->GetOpts().textureType == TT_CUBIC )
	{
		return ( img->GetOpts().IsArray() )? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
	}
	else if( img->GetOpts().textureType == TT_3D )
	{
		return VK_IMAGE_VIEW_TYPE_3D;
	}

	assert( !"invalid opts.textureType" );
	return VK_IMAGE_VIEW_TYPE_2D;;
}

VkComponentMapping VK_GetComponentMappingFromTextureFormat( idImage * img )
{
	VkComponentMapping componentMapping = { VK_COMPONENT_SWIZZLE_IDENTITY };

	if( img->GetOpts().colorFormat == CFM_GREEN_ALPHA )
	{
		componentMapping.r = VK_COMPONENT_SWIZZLE_ONE;
		componentMapping.g = VK_COMPONENT_SWIZZLE_ONE;
		componentMapping.b = VK_COMPONENT_SWIZZLE_ONE;
		componentMapping.a = VK_COMPONENT_SWIZZLE_G;
		return componentMapping;
	}

	switch( img->GetOpts().format )
	{
		case FMT_LUM8:
			componentMapping.r = VK_COMPONENT_SWIZZLE_R;
			componentMapping.g = VK_COMPONENT_SWIZZLE_R;
			componentMapping.b = VK_COMPONENT_SWIZZLE_R;
			componentMapping.a = VK_COMPONENT_SWIZZLE_ONE;
			break;

		case FMT_L8A8:
			componentMapping.r = VK_COMPONENT_SWIZZLE_R;
			componentMapping.g = VK_COMPONENT_SWIZZLE_R;
			componentMapping.b = VK_COMPONENT_SWIZZLE_R;
			componentMapping.a = VK_COMPONENT_SWIZZLE_G;
			break;

		case FMT_ALPHA:
			componentMapping.r = VK_COMPONENT_SWIZZLE_ONE;
			componentMapping.g = VK_COMPONENT_SWIZZLE_ONE;
			componentMapping.b = VK_COMPONENT_SWIZZLE_ONE;
			componentMapping.a = VK_COMPONENT_SWIZZLE_R;
			break;

		case FMT_INT8:
			componentMapping.r = VK_COMPONENT_SWIZZLE_R;
			componentMapping.g = VK_COMPONENT_SWIZZLE_R;
			componentMapping.b = VK_COMPONENT_SWIZZLE_R;
			componentMapping.a = VK_COMPONENT_SWIZZLE_R;
			break;

		default:
			componentMapping.r = VK_COMPONENT_SWIZZLE_R;
			componentMapping.g = VK_COMPONENT_SWIZZLE_G;
			componentMapping.b = VK_COMPONENT_SWIZZLE_B;
			componentMapping.a = VK_COMPONENT_SWIZZLE_A;
			break;
	}

	return componentMapping;
}


/*
========================
idImage::SubImageUpload
========================
*/
void idImage::SubImageUpload( int mipLevel, int x, int y, int z, int width, int height, const void* pic, int pixelPitch ) const
{
	assert( x >= 0 && y >= 0 && mipLevel >= 0 && width >= 0 && height >= 0 && mipLevel < opts.numLevels );

	auto tex = ( vkTextureObject_t *) GetAPIObject();

	if( IsCompressed() )
	{
		width = ( width + 3 ) & ~3;
		height = ( height + 3 ) & ~3;
	}

	const int size = width * height * BitsForFormat( GetOpts().format ) / 8;

	VkBuffer buffer;
	VkCommandBuffer commandBuffer;

	int offset = 0;
	byte * data = stagingManager.Stage( size, 16, commandBuffer, buffer, offset );
	if( GetOpts().format == FMT_RGB565 )
	{
		byte * imgData = ( byte * )pic;
		for( int i = 0; i < size; i += 2 )
		{
			data[ i ] = imgData[ i + 1 ];
			data[ i + 1 ] = imgData[ i ];
		}
	}
	else {
		memcpy( data, pic, size );
	}

	VkBufferImageCopy imgCopy = {};
	imgCopy.bufferOffset = offset;
	imgCopy.bufferRowLength = pixelPitch;
	imgCopy.bufferImageHeight = height;
	imgCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imgCopy.imageSubresource.layerCount = 1;
	imgCopy.imageSubresource.mipLevel = mipLevel;
	imgCopy.imageSubresource.baseArrayLayer = z;
	imgCopy.imageOffset.x = x;
	imgCopy.imageOffset.y = y;
	imgCopy.imageOffset.z = 0;
	imgCopy.imageExtent.width = width;
	imgCopy.imageExtent.height = height;
	imgCopy.imageExtent.depth = 1;

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = tex->imageHandle;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = GetOpts().numLevels;
	barrier.subresourceRange.baseArrayLayer = z;
	barrier.subresourceRange.layerCount = 1;

	barrier.oldLayout = tex->currentLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	vkCmdPipelineBarrier( commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier );

	vkCmdCopyBufferToImage( commandBuffer, buffer, tex->imageHandle, barrier.newLayout, 1, &imgCopy );

	barrier.oldLayout = barrier.newLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	vkCmdPipelineBarrier( commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, NULL, 0, NULL, 1, &barrier );

	tex->currentLayout = barrier.newLayout;
}

/*
========================
idImage::SetPixel
========================
*/
void idImage::SetPixel( int mipLevel, int x, int y, const void* data, int dataSize )
{
	SubImageUpload( mipLevel, x, y, 0, 1, 1, data );
}

/*
========================
idImage::AllocImage

	Every image will pass through this function. Allocates all the necessary MipMap levels for the
	Image, but doesn't put anything in them.

	This should not be done during normal game-play, if you can avoid it.
========================
*/
void idImage::AllocImage() // imgOpts
{
	PurgeImage();

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before OpenGL starts would miss
	// the generated texture
	if( !tr.IsRenderDeviceRunning() )
	{
		return;
	}

#if 0
	// generate the texture number
	{
		GLuint texnum = GL_NONE;
		glGenTextures( 1, ( GLuint* )&texnum );
		assert( texnum != GL_NONE );
		apiObject = reinterpret_cast< void* >( texnum );
	}

	glTextureObject_t tex( this );
	glBindTexture( tex.target, tex.texnum );

	if( GLEW_KHR_debug )
	{
		idStrStatic<MAX_IMAGE_NAME / 2> name;
		name.Format( "idImage(%p.%u)", this, tex.texnum );
		glObjectLabel( GL_TEXTURE, tex.texnum, name.Length(), name.c_str() );
	}

	//----------------------------------------------------
	// allocate all the mip levels with NULL data
	//----------------------------------------------------

	GLsizei w = idMath::Max( opts.width, 1 );
	GLsizei h = idMath::Max( opts.height, 1 );
	GLsizei depth = idMath::Max( opts.depth, 1 );
	const GLint numLevels = idMath::Max( opts.numLevels, 1 );
	const GLint numSamples = idMath::Max( opts.numSamples, 1 );

	if( glConfig.ARB_texture_storage )
	{
		if( tex.target == GL_TEXTURE_2D || tex.target == GL_TEXTURE_CUBE_MAP )
		{
			// 2D, CUBE
			glTexStorage2D( tex.target, numLevels, tex.internalFormat, w, ( tex.target != GL_TEXTURE_CUBE_MAP ) ? h : w );
		}
		else if( tex.target == GL_TEXTURE_2D_MULTISAMPLE )
		{
			// 2DMS
			glTexStorage2DMultisample( GL_TEXTURE_2D_MULTISAMPLE, numSamples, tex.internalFormat, w, h, GL_FALSE );
		}
		else if( tex.target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY )
		{
			// 2DMSArray
			glTexStorage3DMultisample( GL_TEXTURE_2D_MULTISAMPLE_ARRAY, numSamples, tex.internalFormat, w, h, depth, GL_FALSE );
		}
		else// if( tex.target == GL_TEXTURE_3D || tex.target == GL_TEXTURE_2D_ARRAY || tex.target == GL_TEXTURE_CUBE_MAP_ARRAY )
		{
			// 3D, 2DArray, CUBEArray
			glTexStorage3D( tex.target, numLevels, tex.internalFormat, w, h, depth );
		}
	}
	else
	{
		if( opts.IsMultisampled() && opts.textureType == TT_2D )
		{
			assert( numLevels < 1 && opts.textureType == TT_2D );
			if( opts.IsArray() )
			{
				glTexImage3DMultisample( GL_TEXTURE_2D_MULTISAMPLE_ARRAY, numSamples, tex.internalFormat, w, h, depth, GL_FALSE );
			}
			else
			{
				glTexImage2DMultisample( GL_TEXTURE_2D_MULTISAMPLE, numSamples, tex.internalFormat, w, h, GL_FALSE );
			}
		}
		else
		{
			assert( !opts.IsMultisampled() );

			GLenum numSides = 1;
			if( opts.textureType == TT_CUBIC )
			{
				h = w;
				numSides = 6;
				depth *= 6;
			}

			if( IsCompressed() )
			{
				const GLsizei formatBits = 16 * BitsForFormat( opts.format );

				if( opts.IsArray() )
				{
					for( GLint level = 0; level < numLevels; level++ )
					{
						const GLsizei compressedSize = ( ( ( w + 3 ) / 4 ) * ( ( h + 3 ) / 4 ) * formatBits ) / 8;
						glCompressedTexImage3D( tex.target, level, tex.internalFormat, w, h, depth, 0, compressedSize, NULL );
						w = idMath::Max( 1, w >> 1 );
						h = idMath::Max( 1, h >> 1 );
					}
				}
				else
				{
					for( GLint level = 0; level < numLevels; level++ )
					{
						const GLsizei compressedSize = ( ( ( w + 3 ) / 4 ) * ( ( h + 3 ) / 4 ) * formatBits ) / 8;
						for( GLenum side = 0; side < numSides; side++ )
						{
							glCompressedTexImage2D( tex.uploadTarget + side, level, tex.internalFormat, w, h, 0, compressedSize, NULL );
						}
						w = idMath::Max( 1, w >> 1 );
						h = idMath::Max( 1, h >> 1 );
					}
				}
			}
			else
			{
				if( opts.IsArray() )
				{
					for( GLint level = 0; level < numLevels; level++ )
					{
						glTexImage3D( tex.target, level, tex.internalFormat, w, h, depth, 0, tex.dataFormat, tex.dataType, NULL );
						w = idMath::Max( 1, w >> 1 );
						h = idMath::Max( 1, h >> 1 );
					}
				}
				else
				{
					for( GLint level = 0; level < numLevels; level++ )
					{
						for( GLenum side = 0; side < numSides; side++ )
						{
							glTexImage2D( tex.uploadTarget + side, level, tex.internalFormat, w, h, 0, tex.dataFormat, tex.dataType, NULL );
						}
						w = idMath::Max( 1, w >> 1 );
						h = idMath::Max( 1, h >> 1 );
					}
				}
			}
		}

		GL_CheckErrors();
	}

	// Non sampler parameters

	glTexParameteri( tex.target, GL_TEXTURE_BASE_LEVEL, 0 );
	glTexParameteri( tex.target, GL_TEXTURE_MAX_LEVEL, numLevels - 1 );

	// ALPHA, LUMINANCE, LUMINANCE_ALPHA, and INTENSITY have been removed
	// in OpenGL 3.2. In order to mimic those modes, we use the swizzle operators

	if( GetOpts().colorFormat == CFM_GREEN_ALPHA )
	{
		GLint swizzleMask[] = { GL_ONE, GL_ONE, GL_ONE, GL_GREEN };
		glTexParameteriv( tex.target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask );
	}
	else if( GetOpts().format == FMT_LUM8 )
	{
		GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
		glTexParameteriv( tex.target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask );
	}
	else if( GetOpts().format == FMT_L8A8 )
	{
		GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_GREEN };
		glTexParameteriv( tex.target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask );
	}
	else if( GetOpts().format == FMT_ALPHA )
	{
		GLint swizzleMask[] = { GL_ONE, GL_ONE, GL_ONE, GL_RED };
		glTexParameteriv( tex.target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask );
	}
	else if( GetOpts().format == FMT_INT8 )
	{
		GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_RED };
		glTexParameteriv( tex.target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask );
	}
	else
	{
		GLint swizzleMask[] = { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA };
		glTexParameteriv( tex.target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask );
	}

	// see if we messed anything up
	GL_CheckErrors();

	// Sampler parameters
	//SEA: TODO real sampler
	SetTexParameters();

	GL_CheckErrors();
#endif

	vkDeviceContext_t * dc = VK_GetCurrDC();

	apiObject = new vkTextureObject_t();
	auto tex = ( vkTextureObject_t * )apiObject;

	const int w = idMath::Max( opts.width, 1 );
	const int h = idMath::Max( opts.height, 1 );
	const int layers = idMath::Max( opts.depth, 1 );
	const int numLevels = idMath::Max( opts.numLevels, 1 );
	const int numSamples = idMath::Max( opts.numSamples, 1 );

	{
		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

		imageCreateInfo.format = VK_GetFormat( this );

		// query supported format features
		VkFormatProperties formatProps;
		vkGetPhysicalDeviceFormatProperties( dc->GetPDHandle(), imageCreateInfo.format, &formatProps );

		// See if we can use a linear tiled image for a texture, if not, we will
		// need a staging image for the texture data.
		///bool needStaging = ( !( formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT ) ) ? true : false;

		void * data = nullptr;

		if( GetOpts().textureType == TT_3D )
		{
			imageCreateInfo.imageType = VK_IMAGE_TYPE_3D;
			imageCreateInfo.flags = VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
			imageCreateInfo.extent = { w, h, layers };
			imageCreateInfo.mipLevels = numLevels;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
			if( GetOpts().renderTarget )
			{
				assert( !data );
				imageCreateInfo.usage |= ( GetOpts().IsDepthStencil() || GetOpts().IsDepth() ) ?
					VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT :
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			}
			else if( data )
			{
				assert( !GetOpts().renderTarget );
				imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			}
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		}
		else if( GetOpts().textureType == TT_CUBIC )
		{
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			imageCreateInfo.extent = { w, w, 1 };
			imageCreateInfo.mipLevels = numLevels;
			imageCreateInfo.arrayLayers = 6 * layers;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			if( GetOpts().renderTarget )
			{
				assert( !data );
				imageCreateInfo.usage |= ( GetOpts().IsDepthStencil() || GetOpts().IsDepth() ) ?
					VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT :
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			}
			else if( data )
			{
				assert( !GetOpts().renderTarget );
				imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			}
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		}
		else // TT_2D
		{
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.flags = 0;
			imageCreateInfo.extent = { w, h, 1 };

			if( GetOpts().readback )
			{
				// format is not a depth/stencil format.
				assert( !GetOpts().IsDepthStencil() && !GetOpts().IsDepth() );

				imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
				imageCreateInfo.mipLevels = 1;
				imageCreateInfo.arrayLayers = 1;
				imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
				imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // and/or VK_IMAGE_USAGE_TRANSFER_DST_BIT
				imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
				// TODO
			}
			else if( GetOpts().IsMultisampled() )
			{
				imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				imageCreateInfo.mipLevels = 1;
				imageCreateInfo.arrayLayers = layers;
				imageCreateInfo.samples = VK_GetSampleCount( this );
				imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
				if( GetOpts().renderTarget )
				{
					assert( !data );
					imageCreateInfo.usage |= ( GetOpts().IsDepthStencil() || GetOpts().IsDepth() ) ?
						VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT :
						VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
				}
				imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			}
			else {
				imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				imageCreateInfo.mipLevels = numLevels;
				imageCreateInfo.arrayLayers = layers;
				imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
				if( GetOpts().renderTarget )
				{
					assert( !data );
					imageCreateInfo.usage |= ( GetOpts().IsDepthStencil() || GetOpts().IsDepth() ) ?
						VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT :
						VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
				}
				else if( data )
				{
					assert( !GetOpts().renderTarget );
					imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
				}
				imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			}
		}

		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// ?
		// If sharingMode is VK_SHARING_MODE_CONCURRENT, pQueueFamilyIndices must be a valid pointer to an array of queueFamilyIndexCount uint32_t values
		// If sharingMode is VK_SHARING_MODE_CONCURRENT, queueFamilyIndexCount must be greater than 1
		// If sharingMode is VK_SHARING_MODE_CONCURRENT, each element of pQueueFamilyIndices must be unique and must be less than pQueueFamilyPropertyCount
		//  returned by either vkGetPhysicalDeviceQueueFamilyProperties or vkGetPhysicalDeviceQueueFamilyProperties2 for the physicalDevice that was used to create device

		///imageCreateInfo.queueFamilyIndexCount;		// ?
		///imageCreateInfo.pQueueFamilyIndices;		// ?

		tex->currentLayout = imageCreateInfo.initialLayout;

	#if 0
		{
			VkImageFormatProperties imageFormatProps;
			vkGetPhysicalDeviceImageFormatProperties( dc->GetPDHandle(),
				imageCreateInfo.format, imageCreateInfo.imageType, imageCreateInfo.tiling, imageCreateInfo.usage, imageCreateInfo.flags,
				&imageFormatProps );

			// query additional capabilities specific to image type
			VkImageFormatProperties2 formatProps2;
			VkPhysicalDeviceImageFormatInfo2 imageFormatInfo = {
				VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2, nullptr,
				imageCreateInfo.format,
				imageCreateInfo.imageType,
				imageCreateInfo.tiling,
				imageCreateInfo.usage,
				imageCreateInfo.flags
			};
			vkGetPhysicalDeviceImageFormatProperties2( dc->GetPDHandle(), &imageFormatInfo, &formatProps2 );
		}
	#endif

		vkCreateImage( dc->GetHandle(), &imageCreateInfo, nullptr, &tex->imageHandle );
	}

	// 2. Query for memory requirements of this image.
	VkMemoryRequirements memReq = {};
	bool dedicatedAllocation = false;
#if 0
	if( dc->GetPD()->m_extensions.NV_dedicated_allocation.available )
	{
		// Create an image with
		// VkDedicatedAllocationImageCreateInfoNV::dedicatedAllocation
		// set to VK_TRUE

		VkDedicatedAllocationImageCreateInfoNV dedicatedImageInfo =
		{
			VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV,	// sType
			NULL,															// pNext
			VK_TRUE,														// dedicatedAllocation
		};
		VkImageCreateInfo imageCreateInfo =
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,    // sType
			&dedicatedImageInfo                     // pNext
													// Other members set as usual
		};
		VkImage image;
		VkResult result = vkCreateImage( dc->GetHandle(), &imageCreateInfo, NULL, &image );

		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements( dc->GetHandle(), image, &memoryRequirements );

		// Allocate memory with VkDedicatedAllocationMemoryAllocateInfoNV::image
		// pointing to the image we are allocating the memory for

		VkDedicatedAllocationMemoryAllocateInfoNV dedicatedInfo =
		{
			VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV,             // sType
			NULL,                                                                       // pNext
			image,                                                                      // image
			VK_NULL_HANDLE,                                                             // buffer
		};
		VkMemoryAllocateInfo memoryAllocateInfo =
		{
			VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,                 // sType
			&dedicatedInfo,                                         // pNext
			memoryRequirements.size,                                // allocationSize
			FindMemoryTypeIndex( memoryRequirements.memoryTypeBits ), // memoryTypeIndex
		};

		VkDeviceMemory memory;
		vkAllocateMemory( dc->GetHandle(), &memoryAllocateInfo, NULL, &memory );

	}
	else
#endif
	if( dc->GetPD()->m_extensions.KHR_dedicated_allocation.available &&
		dc->GetPD()->m_extensions.KHR_get_memory_requirements2.available &&
		GetOpts().renderTarget ) // special handling for rendertargets
	{
		auto pVkGetImageMemoryRequirements2KHR = ( PFN_vkGetImageMemoryRequirements2KHR ) dc->GetFuncPtr( "vkGetImageMemoryRequirements2KHR" );

		VkImageMemoryRequirementsInfo2KHR memReqInfo2 = {
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2, nullptr,
			tex->imageHandle
		};
		VkMemoryRequirements2KHR memReq2 = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR };
		VkMemoryDedicatedRequirementsKHR memDedicatedReq = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR };
		memReq2.pNext = &memDedicatedReq;

		pVkGetImageMemoryRequirements2KHR( dc->GetHandle(), &memReqInfo2, &memReq2 );

		memReq = memReq2.memoryRequirements;
		dedicatedAllocation = ( memDedicatedReq.requiresDedicatedAllocation != VK_FALSE ) || ( memDedicatedReq.prefersDedicatedAllocation != VK_FALSE );
	}
	else
	{
		vkGetImageMemoryRequirements( dc->GetHandle(), tex->imageHandle, &memReq );
	}

	// 3. Allocate device memory.
	VkDeviceSize memoryOffset = 0;

	if( dedicatedAllocation )
	{
		VkMemoryAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.allocationSize = memReq.size;
		allocateInfo.memoryTypeIndex = /* Some magic to find suitable memory type using memReq.memoryTypeBits. */;

		VkMemoryDedicatedAllocateInfoKHR dedicatedAllocateInfo = {};
		dedicatedAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR;
		dedicatedAllocateInfo.image = tex->imageHandle;
		allocateInfo.pNext = &dedicatedAllocateInfo;

		vkAllocateMemory( dc->GetHandle(), &allocateInfo, nullptr, &tex->deviceImageMemory );
		assert( tex->deviceImageMemory != VK_NULL_HANDLE );
	}
	else
	{
		// Try to find free space in an existing, bigger block of VkDeviceMemory or allocate new one.
		// Respect parameters from memReq.
		memoryOffset = 0;

		int requiredSizeForImage = memReq.size;

		vkHeapList_t memoryHeaps;
		EnumerateHeaps( dc, memoryHeaps );
		tex->deviceImageMemory = AllocateMemory( memoryHeaps, dc->GetHandle(), requiredSizeForImage, memReq.memoryTypeBits, MT_DeviceLocal );
		assert( tex->deviceImageMemory != VK_NULL_HANDLE );
	}

	{
		// 4. Bind image with memory.
		vkBindImageMemory( dc->GetHandle(), tex->imageHandle, tex->deviceImageMemory, memoryOffset );
	}

	// Create Image View
	{
		VkImageViewCreateInfo viewCreateInfo = {};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

		viewCreateInfo.image = tex->imageHandle;
		viewCreateInfo.viewType = VK_GetImgViewType( this );
		viewCreateInfo.format = VK_GetFormat( this );
		viewCreateInfo.components = VK_GetComponentMappingFromTextureFormat( this );
		if( GetOpts().IsDepth() ) {
			viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else if( GetOpts().IsDepthStencil() ) {
			viewCreateInfo.subresourceRange.aspectMask = ( VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT );
		}
		else {
			viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = numLevels;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 6 * layers;

		auto res = vkCreateImageView( dc->GetHandle(), &viewCreateInfo, NULL, &tex->imageViewHandle );
		assert( res == VK_SUCCESS );
	}
}

// Create a host(cpu)-visible staging buffer that contains the raw image data
void VK_CreateStagingBuffer( VkDevice device, size_t src_size, const void * src_data )
{
	VkResult res;
	VkBuffer stagingBuffer;
	{
		VkBufferCreateInfo bufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, nullptr, 0,
			src_size,
			// This buffer is used as a transfer source for the buffer copy
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,	// usage
			VK_SHARING_MODE_EXCLUSIVE,			// sharingMode
			0, nullptr
		};
		res = vkCreateBuffer( device, &bufferCreateInfo, nullptr, &stagingBuffer );
		assert( res == VK_SUCCESS );
	}

	// Get memory requirements for the staging buffer (alignment, memory type bits)
	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements( device, stagingBuffer, &memReqs );

	VkMemoryAllocateInfo memAllocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr };
	memAllocInfo.allocationSize = memReqs.size;
	// Get memory type index for a host visible buffer
	memAllocInfo.memoryTypeIndex = device->GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

	VkDeviceMemory stagingMemory;
	res = vkAllocateMemory( device, &memAllocInfo, nullptr, &stagingMemory );
	assert( res == VK_SUCCESS );

	res = vkBindBufferMemory( device, stagingBuffer, stagingMemory, 0 );
	assert( res == VK_SUCCESS );

#if 0
	{
		// Copy texture data into staging buffer
		uint8_t * data;
		res = vkMapMemory( device, stagingMemory, 0, memReqs.size, 0, ( void ** )&data );
		assert( res == VK_SUCCESS );
		{
			memcpy( data, src_data, src_size );
		}
		vkUnmapMemory( device, stagingMemory );

		// Setup buffer copy regions for each layer including all of it's miplevels
		std::vector<VkBufferImageCopy> bufferCopyRegions;
		size_t offset = 0;

		for( uint32_t layer = 0; layer < layerCount; layer++ )
		{
			for( uint32_t level = 0; level < mipLevels; level++ )
			{
				VkBufferImageCopy bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				bufferCopyRegion.imageSubresource.mipLevel = level;
				bufferCopyRegion.imageSubresource.baseArrayLayer = layer;
				bufferCopyRegion.imageSubresource.layerCount = 1;
				bufferCopyRegion.imageExtent.width = static_cast< uint32_t >( tex2DArray[ layer ][ level ].extent().x );
				bufferCopyRegion.imageExtent.height = static_cast< uint32_t >( tex2DArray[ layer ][ level ].extent().y );
				bufferCopyRegion.imageExtent.depth = 1;
				bufferCopyRegion.bufferOffset = offset;

				bufferCopyRegions.push_back( bufferCopyRegion );

				// Increase offset into staging buffer for next level / face
				offset += tex2DArray[ layer ][ level ].size();
			}
		}

		// Create optimal tiled target image
		VkImageCreateInfo imageCreateInfo = vks::initializers::imageCreateInfo();
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.extent = { width, height, 1 };
		imageCreateInfo.usage = imageUsageFlags;
		// Ensure that the TRANSFER_DST bit is set for staging
		if( !( imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT ) )
		{
			imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		imageCreateInfo.arrayLayers = layerCount;
		imageCreateInfo.mipLevels = mipLevels;

		VK_CHECK_RESULT( vkCreateImage( device, &imageCreateInfo, nullptr, &image ) );

		vkGetImageMemoryRequirements( device, image, &memReqs );

		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = device->GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

		VK_CHECK_RESULT( vkAllocateMemory( device, &memAllocInfo, nullptr, &deviceMemory ) );
		VK_CHECK_RESULT( vkBindImageMemory( device, image, deviceMemory, 0 ) );

		// Use a separate command buffer for texture loading
		VkCommandBuffer copyCmd = device->CreateCommandBuffer( VK_COMMAND_BUFFER_LEVEL_PRIMARY, true );

		// Image barrier for optimal image (target)
		// Set initial layout for all array layers (faces) of the optimal (target) tiled texture
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = mipLevels;
		subresourceRange.layerCount = layerCount;

		vks::tools::setImageLayout(
			copyCmd,
			image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			subresourceRange );

		// Copy the layers and mip levels from the staging buffer to the optimal tiled image
		vkCmdCopyBufferToImage(
			copyCmd,
			stagingBuffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<uint32_t>( bufferCopyRegions.size() ),
			bufferCopyRegions.data() );

		// Change texture image layout to shader read after all faces have been copied
		this->imageLayout = imageLayout;
		vks::tools::setImageLayout(
			copyCmd,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			imageLayout,
			subresourceRange );

		device->FlushCommandBuffer( copyCmd, copyQueue );
	}
#endif

	// Clean up staging resources
	vkFreeMemory( device, stagingMemory, nullptr );
	vkDestroyBuffer( device, stagingBuffer, nullptr );
}



void idImage::GenerateImage( const byte* pic, int width, int height, int depth, textureFilter_t filterParm, textureRepeat_t repeatParm, textureUsage_t usageParm )
{
#if 0
	PurgeImage();

	filter = filterParm;
	repeat = repeatParm;
	usage = usageParm;
	layout = IMG_LAYOUT_2D;

	opts.textureType = TT_2D;
	opts.width = idMath::Max( width, 1 );
	opts.height = idMath::Max( height, 1 );
	opts.depth = idMath::Max( depth, 1 );
	opts.numLevels = 0;
	opts.numSamples = msaaSamples;
	DeriveOpts();

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before the render starts would miss
	// the generated texture
	if( !R_IsInitialized() )
	{
		return;
	}

	// RB: allow pic == NULL for internal framebuffer images
	if( pic == NULL || GetOpts().IsMultisampled() )
	{
		AllocImage();
		return;
	}

	idBinaryImage im( GetName() );

	// foresthale 2014-05-30: give a nice progress display when binarizing
	commonLocal.LoadPacifierBinarizeFilename( GetName(), "generated image" );

	if( GetOpts().numLevels > 1 )
	{
		commonLocal.LoadPacifierBinarizeProgressTotal( GetOpts().width * GetOpts().height * 4 / 3 );
	}
	else {
		commonLocal.LoadPacifierBinarizeProgressTotal( GetOpts().width * GetOpts().height );
	}

	im.Load2DFromMemory( width, height, pic, GetOpts().numLevels, opts.format, opts.colorFormat, GetOpts().gammaMips );

	commonLocal.LoadPacifierBinarizeEnd();

	AllocImage();

	for( int i = 0; i < im.NumImages(); ++i )
	{
		const bimageImage_t& img = im.GetImageHeader( i );
		const byte* data = im.GetImageData( i );
		SubImageUpload( img.level, 0, 0, img.destZ, img.width, img.height, data );
	}


	// query supported format features
	VkFormatProperties formatProps;
	vkGetPhysicalDeviceFormatProperties( _vkPhysicalDevice, imageCreateInfo.format, &formatProps );

	// See if we can use a linear tiled image for a texture, if not, we will
	// need a staging image for the texture data.
	///bool needStaging = ( !( formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT ) ) ? true : false;

	VkMemoryAllocateInfo memAllocInfo = vks::initializers::memoryAllocateInfo();
	VkMemoryRequirements memReqs;

	VkBool32 useStaging = !forceLinear; // // Only use linear tiling if requested (and supported by the device).
	if( useStaging )
	{
		// Create a host-visible staging buffer that contains the raw image data
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;

		VkBufferCreateInfo bufferCreateInfo = vks::initializers::bufferCreateInfo();
		bufferCreateInfo.size = tex2D.size();
		// This buffer is used as a transfer source for the buffer copy
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		vkCreateBuffer( _vkDevice, &bufferCreateInfo, nullptr, &stagingBuffer );

		// Get memory requirements for the staging buffer (alignment, memory type bits)
		vkGetBufferMemoryRequirements( _vkDevice, stagingBuffer, &memReqs );

		memAllocInfo.allocationSize = memReqs.size;
		// Get memory type index for a host visible buffer
		memAllocInfo.memoryTypeIndex = device->GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

		vkAllocateMemory( _vkDevice, &memAllocInfo, nullptr, &stagingMemory );
		vkBindBufferMemory( _vkDevice, stagingBuffer, stagingMemory, 0 );

		// Copy texture data into staging buffer
		uint8_t * data;
		vkMapMemory( _vkDevice, stagingMemory, 0, memReqs.size, 0, ( void ** )&data );
		memcpy( data, tex2D.data(), tex2D.size() );
		vkUnmapMemory( _vkDevice, stagingMemory );

		// Setup buffer copy regions for each mip level
		std::vector<VkBufferImageCopy> bufferCopyRegions;
		uint32_t offset = 0;

		for( uint32_t i = 0; i < numLevels; i++ )
		{
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = i;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = static_cast<uint32_t>( tex2D[ i ].extent().x );
			bufferCopyRegion.imageExtent.height = static_cast<uint32_t>( tex2D[ i ].extent().y );
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = offset;

			bufferCopyRegions.push_back( bufferCopyRegion );

			offset += static_cast<uint32_t>( tex2D[ i ].size() );
		}

		// Create optimal tiled target image
		VkImageCreateInfo imageCreateInfo = vks::initializers::imageCreateInfo();
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.mipLevels = numLevels;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.extent = { width, height, 1 };
		imageCreateInfo.usage = imageUsageFlags;
		// Ensure that the TRANSFER_DST bit is set for staging
		if( !( imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT ) )
		{
			imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		vkCreateImage( _vkDevice, &imageCreateInfo, nullptr, &image );

		vkGetImageMemoryRequirements( _vkDevice, image, &memReqs );

		memAllocInfo.allocationSize = memReqs.size;

		memAllocInfo.memoryTypeIndex = device->getMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
		vkAllocateMemory( _vkDevice, &memAllocInfo, nullptr, &deviceMemory );
		vkBindImageMemory( _vkDevice, image, deviceMemory, 0 );

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = numLevels;
		subresourceRange.layerCount = 1;

		// Image barrier for optimal image (target)
		// Optimal image will be used as destination for the copy
		vks::tools::setImageLayout(
			copyCmd,
			image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			subresourceRange );

		// Copy mip levels from staging buffer
		vkCmdCopyBufferToImage(
			copyCmd,
			stagingBuffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<uint32_t>( bufferCopyRegions.size() ),
			bufferCopyRegions.data()
		);

		// Change texture image layout to shader read after all mip levels have been copied
		this->imageLayout = imageLayout;
		vks::tools::setImageLayout(
			copyCmd,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			imageLayout,
			subresourceRange );

		device->flushCommandBuffer( copyCmd, copyQueue );

		// Clean up staging resources
		vkFreeMemory( _vkDevice, stagingMemory, nullptr );
		vkDestroyBuffer( _vkDevice, stagingBuffer, nullptr );
	}
#endif
}







void EnumerateHeaps( const vkDeviceContext_t * dc, vkHeapList_t & result )
{
	VkPhysicalDeviceMemoryProperties memoryProperties = {};
	vkGetPhysicalDeviceMemoryProperties( dc->GetPDHandle(), &memoryProperties );

	idList<MemoryTypeInfo_t::Heap_t> heaps( memoryProperties.memoryHeapCount );

	for( uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i )
	{
		MemoryTypeInfo_t::Heap_t info;
		info.size = memoryProperties.memoryHeaps[ i ].size;
		info.deviceLocal = ( memoryProperties.memoryHeaps[ i ].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ) != 0;

		heaps.Append( info );
	}

	result.SetGranularity( memoryProperties.memoryTypeCount );
	for( uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i )
	{
		MemoryTypeInfo_t typeInfo;

		typeInfo.deviceLocal = ( memoryProperties.memoryTypes[ i ].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) != 0;
		typeInfo.hostVisible = ( memoryProperties.memoryTypes[ i ].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) != 0;
		typeInfo.hostCoherent = ( memoryProperties.memoryTypes[ i ].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) != 0;
		typeInfo.hostCached = ( memoryProperties.memoryTypes[ i ].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT ) != 0;
		typeInfo.lazilyAllocated = ( memoryProperties.memoryTypes[ i ].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT ) != 0;

		typeInfo.heap = heaps[ memoryProperties.memoryTypes[ i ].heapIndex ];

		typeInfo.index = static_cast<int> ( i );

		result.Append( typeInfo );
	}
}

VkDeviceMemory AllocateMemory( const vkHeapList_t & memoryInfos, VkDevice device, const int size,
	const uint32_t memoryBits, unsigned int memoryProperties, bool * isHostCoherent = nullptr )
{
	for( auto & memoryInfo : memoryInfos )
	{
		if( ( ( 1 << memoryInfo.index ) & memoryBits ) == 0 )
		{
			continue;
		}
		if( ( memoryProperties & MT_DeviceLocal ) && !memoryInfo.deviceLocal )
		{
			continue;
		}
		if( ( memoryProperties & MT_HostVisible ) && !memoryInfo.hostVisible )
		{
			continue;
		}

		if( isHostCoherent )
		{
			*isHostCoherent = memoryInfo.hostCoherent;
		}

		VkMemoryAllocateInfo memoryAllocateInfo = {};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.memoryTypeIndex = memoryInfo.index;
		memoryAllocateInfo.allocationSize = size;

		VkDeviceMemory deviceMemory;
		vkAllocateMemory( device, &memoryAllocateInfo, nullptr, &deviceMemory );
		return deviceMemory;
	}

	return VK_NULL_HANDLE;
}