
#include "precompiled.h"
#pragma hdrstop

#include "vk_header.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

void SetupImageMemoryBarrier( VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout,
							  VkImageLayout newImageLayout, VkImageMemoryBarrier & imageMemoryBarrier )
{
	memset( &imageMemoryBarrier, 0, sizeof( VkImageMemoryBarrier ) );

	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

	imageMemoryBarrier.image = image;

	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	imageMemoryBarrier.oldLayout = oldImageLayout;
	imageMemoryBarrier.newLayout = newImageLayout;

	imageMemoryBarrier.subresourceRange.aspectMask = aspectMask;
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.layerCount = 1;

	// oldImageLayout --------------------------------

	// Undefined layout:
	//   Note: Only allowed as initial layout!
	//   Note: Make sure any writes to the image have been finished
	if( oldImageLayout == VK_IMAGE_LAYOUT_UNDEFINED )
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
	if( oldImageLayout == VK_IMAGE_LAYOUT_PREINITIALIZED )
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;

	// Old layout is color attachment:
	//   Note: Make sure any writes to the color buffer have been finished
	if( oldImageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	if( oldImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	// Old layout is transfer source:
	//   Note: Make sure any reads from the image have been finished
	if( oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL )
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

	// Old layout is shader read (sampler, input attachment):
	//   Note: Make sure any shader reads from the image have been finished
	if( oldImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;

	// newImageLayout --------------------------------
	// newLayout cannot use VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED
	assert( newImageLayout != VK_IMAGE_LAYOUT_UNDEFINED && newImageLayout != VK_IMAGE_LAYOUT_PREINITIALIZED );

	// New layout is transfer destination (copy, blit):
	//   Note: Make sure any copyies to the image have been finished
	if( newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	// New layout is transfer source (copy, blit):
	//   Note: Make sure any reads from and writes to the image have been finished
	if( newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL )
	{
		imageMemoryBarrier.srcAccessMask |= VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}

	// New layout is color attachment:
	//   Note: Make sure any writes to the color buffer hav been finished
	if( newImageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
	{
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}

	// New layout is depth attachment:
	//   Note: Make sure any writes to depth/stencil buffer have been finished
	if( newImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
		imageMemoryBarrier.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	// New layout is shader read (sampler, input attachment):
	//   Note: Make sure any writes to the image have been finished
	if( newImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
}

// This lovely piece of code is from SaschaWillems, check out his Vulkan repositories on
// GitHub if you're feeling adventurous.
void SetImageLayout( VkCommandBuffer commandBuffer,
					 VkImage image,
					 VkImageAspectFlags aspectMask,
					 VkImageLayout oldImageLayout,
					 VkImageLayout newImageLayout )
{
	VkImageMemoryBarrier imageMemoryBarrier;
	SetupImageMemoryBarrier( image, aspectMask, oldImageLayout, newImageLayout, imageMemoryBarrier );

	// Put barrier inside the setup command buffer
	vkCmdPipelineBarrier( commandBuffer,
						  // Put the barriers for source and destination on
						  // top of the command buffer
						  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,	// VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
						  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,	// VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
						  0,
						  0, NULL,
						  0, NULL,
						  1, &imageMemoryBarrier );
}








VkDescriptorPool CreateDescriptorPool( vkDeviceContext_t * dc )
{
	VkDescriptorPoolSize pool_sizes[] =
	{
		//{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		//{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		//{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		//{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		//{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		//{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		//{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		//{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};
	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; //VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT
	pool_info.maxSets = 2 * _countof( pool_sizes );
	pool_info.poolSizeCount = _countof( pool_sizes );
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	ID_VK_CHECK( vkCreateDescriptorPool( dc->GetHandle(), &pool_info, nullptr, &descriptorPool ) );
	return descriptorPool;
}