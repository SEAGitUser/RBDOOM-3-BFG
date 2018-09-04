#ifndef __VK_H__
#define __VK_H__
/*
=====================================================================================================

	VULKAN API

=====================================================================================================
*/

#include "libs\vulkan\vulkan.h"
#include "libs\vulkan\vk_sdk_platform.h"

#define NUM_FRAME_DATA 2

/*
=============
VK_ErrorToString
=============
*/
#define ID_VK_ERROR_STRING( x ) case static_cast< int >( x ): return #x
const char * VK_ErrorToString( VkResult result )
{
	switch( result )
	{
		ID_VK_ERROR_STRING( VK_SUCCESS );
		ID_VK_ERROR_STRING( VK_NOT_READY );
		ID_VK_ERROR_STRING( VK_TIMEOUT );
		ID_VK_ERROR_STRING( VK_EVENT_SET );
		ID_VK_ERROR_STRING( VK_EVENT_RESET );
		ID_VK_ERROR_STRING( VK_INCOMPLETE );
		ID_VK_ERROR_STRING( VK_ERROR_OUT_OF_HOST_MEMORY );
		ID_VK_ERROR_STRING( VK_ERROR_OUT_OF_DEVICE_MEMORY );
		ID_VK_ERROR_STRING( VK_ERROR_INITIALIZATION_FAILED );
		ID_VK_ERROR_STRING( VK_ERROR_DEVICE_LOST );
		ID_VK_ERROR_STRING( VK_ERROR_MEMORY_MAP_FAILED );
		ID_VK_ERROR_STRING( VK_ERROR_LAYER_NOT_PRESENT );
		ID_VK_ERROR_STRING( VK_ERROR_EXTENSION_NOT_PRESENT );
		ID_VK_ERROR_STRING( VK_ERROR_FEATURE_NOT_PRESENT );
		ID_VK_ERROR_STRING( VK_ERROR_INCOMPATIBLE_DRIVER );
		ID_VK_ERROR_STRING( VK_ERROR_TOO_MANY_OBJECTS );
		ID_VK_ERROR_STRING( VK_ERROR_FORMAT_NOT_SUPPORTED );
		ID_VK_ERROR_STRING( VK_ERROR_FRAGMENTED_POOL );
		ID_VK_ERROR_STRING( VK_ERROR_OUT_OF_POOL_MEMORY );
		ID_VK_ERROR_STRING( VK_ERROR_INVALID_EXTERNAL_HANDLE );
		ID_VK_ERROR_STRING( VK_ERROR_SURFACE_LOST_KHR );
		ID_VK_ERROR_STRING( VK_ERROR_NATIVE_WINDOW_IN_USE_KHR );
		ID_VK_ERROR_STRING( VK_SUBOPTIMAL_KHR );
		ID_VK_ERROR_STRING( VK_ERROR_OUT_OF_DATE_KHR );
		ID_VK_ERROR_STRING( VK_ERROR_INCOMPATIBLE_DISPLAY_KHR );
		ID_VK_ERROR_STRING( VK_ERROR_VALIDATION_FAILED_EXT );
		ID_VK_ERROR_STRING( VK_ERROR_INVALID_SHADER_NV );
		ID_VK_ERROR_STRING( VK_ERROR_FRAGMENTATION_EXT );
		ID_VK_ERROR_STRING( VK_ERROR_NOT_PERMITTED_EXT );
		///ID_VK_ERROR_STRING( VK_ERROR_OUT_OF_POOL_MEMORY_KHR );
		///ID_VK_ERROR_STRING( VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR );
		default: return "UNKNOWN";
	};
}
#undef ID_VK_ERROR_STRING

#define ID_VK_CHECK( x ) { \
	VkResult ret = x; if( ret != VK_SUCCESS ) common->FatalError( "VK: %s - %s", VK_ErrorToString( ret ), #x ); \
}

#define ID_VK_VALIDATE( x, msg ) { \
	if ( !( x ) ) common->FatalError( "VK: %s - %s", msg, #x ); \
}

struct window_t
{
	void *	handle;
};

enum class imgLayoutBarrier_e
{
	Undefined,
	TransferDest,
	ColorAttachment,
	DepthStencilAttachment,
	TransferSource,
	Present,
	PixelShaderRead,
	PixelDepthStencilRead,
	ComputeGeneralRW,
};

inline imgLayoutBarrier_e GetImageLayoutFromVulkanLayout( VkImageLayout Layout )
{
	switch( Layout )
	{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			return imgLayoutBarrier_e::Undefined;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			return imgLayoutBarrier_e::TransferDest;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			return imgLayoutBarrier_e::ColorAttachment;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			return imgLayoutBarrier_e::DepthStencilAttachment;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			return imgLayoutBarrier_e::TransferSource;

		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			return imgLayoutBarrier_e::Present;

			//case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			//	return imgLayoutBarrier_e::PixelShaderRead;

			//case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
			//	return imgLayoutBarrier_e::PixelDepthStencilRead;

			//case VK_IMAGE_LAYOUT_GENERAL:
			//	return imgLayoutBarrier_e::ComputeGeneral;

		default:
			assert( "Unknown VkImageLayout " );
			break;
	}

	return imgLayoutBarrier_e::Undefined;
}

inline VkPipelineStageFlags GetImageBarrierFlags( imgLayoutBarrier_e Target, VkAccessFlags& AccessFlags, VkImageLayout& Layout )
{
	VkPipelineStageFlags StageFlags = ( VkPipelineStageFlags )0;
	switch( Target )
	{
		case imgLayoutBarrier_e::Undefined:
			AccessFlags = 0;
			StageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			Layout = VK_IMAGE_LAYOUT_UNDEFINED;
			break;

		case imgLayoutBarrier_e::TransferDest:
			AccessFlags = VK_ACCESS_TRANSFER_WRITE_BIT;
			StageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
			Layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			break;

		case imgLayoutBarrier_e::ColorAttachment:
			AccessFlags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			StageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			Layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			break;

		case imgLayoutBarrier_e::DepthStencilAttachment:
			AccessFlags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			StageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			break;

		case imgLayoutBarrier_e::TransferSource:
			AccessFlags = VK_ACCESS_TRANSFER_READ_BIT;
			StageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
			Layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			break;

		case imgLayoutBarrier_e::Present:
			AccessFlags = 0;
			StageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			Layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			break;

		case imgLayoutBarrier_e::PixelShaderRead:
			AccessFlags = VK_ACCESS_SHADER_READ_BIT;
			StageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			break;

		case imgLayoutBarrier_e::PixelDepthStencilRead:
			AccessFlags = VK_ACCESS_SHADER_READ_BIT;
			StageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			break;

		case imgLayoutBarrier_e::ComputeGeneralRW:
			AccessFlags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			StageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			Layout = VK_IMAGE_LAYOUT_GENERAL;
			break;

		default:
			assert( "Unknown ImageLayoutBarrier" );
			break;
	}

	return StageFlags;
}

ID_INLINE VkImageLayout GetImageLayout( imgLayoutBarrier_e Target )
{
	VkAccessFlags Flags;
	VkImageLayout Layout;
	GetImageBarrierFlags( Target, Flags, Layout );
	return Layout;
}

ID_INLINE void SetImageBarrierInfo( imgLayoutBarrier_e Source, imgLayoutBarrier_e Dest, VkImageMemoryBarrier& InOutBarrier, VkPipelineStageFlags& InOutSourceStage, VkPipelineStageFlags& InOutDestStage )
{
	InOutSourceStage |= GetImageBarrierFlags( Source, InOutBarrier.srcAccessMask, InOutBarrier.oldLayout );
	InOutDestStage |= GetImageBarrierFlags( Dest, InOutBarrier.dstAccessMask, InOutBarrier.newLayout );
}

void ImagePipelineBarrier( VkCommandBuffer CmdBuffer, VkImage Image, imgLayoutBarrier_e Source, imgLayoutBarrier_e Dest, const VkImageSubresourceRange& SubresourceRange )
{
	VkImageMemoryBarrier ImageBarrier = {};
	ImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	ImageBarrier.image = Image;
	ImageBarrier.subresourceRange = SubresourceRange;
	ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	VkPipelineStageFlags SourceStages = ( VkPipelineStageFlags )0;
	VkPipelineStageFlags DestStages = ( VkPipelineStageFlags )0;
	SetImageBarrierInfo( Source, Dest, ImageBarrier, SourceStages, DestStages );

	// special handling for VK_IMAGE_LAYOUT_PRESENT_SRC_KHR (otherwise Mali devices flicker)
	if( Source == imgLayoutBarrier_e::Present )
	{
		SourceStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		DestStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if( Dest == imgLayoutBarrier_e::Present )
	{
		SourceStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		DestStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}

	vkCmdPipelineBarrier( CmdBuffer, SourceStages, DestStages, 0, 0, nullptr, 0, nullptr, 1, &ImageBarrier );
}

inline VkImageSubresourceRange SetupImageSubresourceRange( VkImageAspectFlags Aspect = VK_IMAGE_ASPECT_COLOR_BIT, uint32 StartMip = 0 )
{
	VkImageSubresourceRange Range = {};
	Range.aspectMask = Aspect;
	Range.baseMipLevel = StartMip;
	Range.levelCount = 1;
	Range.baseArrayLayer = 0;
	Range.layerCount = 1;
	return Range;
}

inline VkImageMemoryBarrier SetupImageMemoryBarrier( VkImage Image, VkImageAspectFlags Aspect, uint32 NumMips = 1 )
{
	VkImageMemoryBarrier Barrier = {};
	Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	Barrier.image = Image;
	Barrier.subresourceRange.aspectMask = Aspect;
	Barrier.subresourceRange.levelCount = NumMips;
	Barrier.subresourceRange.layerCount = 1;
	Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	return Barrier;
}

struct vkTextureObject_t
{
	VkImage			imageHandle = VK_NULL_HANDLE;
	VkImageView		imageViewHandle = VK_NULL_HANDLE;
	VkDeviceMemory	deviceImageMemory = VK_NULL_HANDLE;
	VkImageLayout	currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkSampler		sampler;

	void FreeMemory( VkDevice _vkDevice )
	{
		assert( deviceImageMemory != VK_NULL_HANDLE );
		vkFreeMemory( _vkDevice, deviceImageMemory, NULL );
		deviceImageMemory = VK_NULL_HANDLE;
	}

	void Destroy( VkDevice _vkDevice )
	{
		assert( imageViewHandle != VK_NULL_HANDLE );
		vkDestroyImageView( _vkDevice, imageViewHandle, NULL );
		imageViewHandle = VK_NULL_HANDLE;

		assert( imageHandle != VK_NULL_HANDLE );
		vkDestroyImage( _vkDevice, imageHandle, NULL );
		imageHandle = VK_NULL_HANDLE;
	}
};


struct vkSwapChainBuffer_t
{
	VkImage image = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
};
struct vkSwapChain_t
{
	const vkDeviceContext_t * m_dc = nullptr; // master dc

	VkSurfaceKHR surface = VK_NULL_HANDLE;
	size_t presentQueueFamilyIndex = 0;

	VkSurfaceFormatKHR fmtInfo = {};
	VkExtent2D swapChainExtent = {};
	VkSwapchainKHR swapChainHandle = VK_NULL_HANDLE;

	idList<VkImage> images;
	idList<vkSwapChainBuffer_t> buffers;

	VkFence m_imageAvailableFence = VK_NULL_HANDLE;
	uint32_t m_lastAcquiredImageIndex = 0;

	PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR = nullptr;
	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR = nullptr;
	PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR = nullptr;
	PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR = nullptr;
	PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR = nullptr;
	PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR = nullptr;
	PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR = nullptr;
	PFN_vkQueuePresentKHR fpQueuePresentKHR = nullptr;

	VkSwapchainKHR GetHandle() const { return swapChainHandle; }
	uint32_t GetLastAcquiredImageIndex() const { return m_lastAcquiredImageIndex; }

	/*
	=====================
	DESTRUCTOR
	=====================
	*/
	~vkSwapChain_t()
	{
		for( int i = 0; i < buffers.Num(); i++ )
		{
			vkDestroyImageView( m_dc->GetHandle(), buffers[ i ].view, NULL );
			vkDestroyImage( m_dc->GetHandle(), buffers[ i ].image, NULL );
		}
		buffers.Clear();
		images.Clear();

		fpDestroySwapchainKHR( m_dc->GetHandle(), swapChainHandle, NULL );


		//vkDestroySurfaceKHR( instance, surface, NULL );

		memset( this, 0, sizeof( *this ) );
	}

	uint32_t GetNextImageIndex( VkSemaphore semaphore_handle, bool in_should_block = false )
	{
		VkFence fence_handle = VK_NULL_HANDLE;

		if( in_should_block )
		{
			assert( m_imageAvailableFence != VK_NULL_HANDLE );
			vkResetFences( m_dc->GetHandle(), 1, &m_imageAvailableFence );
			fence_handle = m_imageAvailableFence;
		}

		uint32_t index;
		auto res = vkAcquireNextImageKHR( m_dc->GetHandle(), swapChainHandle, UINT64_MAX, semaphore_handle, fence_handle, &index );
		assert( res == VK_SUCCESS );

		if( fence_handle != VK_NULL_HANDLE )
		{
			const VkBool32 waitAll = VK_TRUE;
			res = vkWaitForFences( m_dc->GetHandle(), 1, &fence_handle, waitAll, UINT64_MAX );
			assert( res == VK_SUCCESS );
		}

		return index;
	}
};

enum MemoryProperties_e {
	MT_DeviceLocal = 1,
	MT_HostVisible = 2
};

struct MemoryTypeInfo_t {
	bool deviceLocal = false;
	bool hostVisible = false;
	bool hostCoherent = false;
	bool hostCached = false;
	bool lazilyAllocated = false;

	struct Heap_t {
		uint64_t size = 0;
		bool deviceLocal = false;
	};

	Heap_t heap;
	int index;
};

typedef idList<MemoryTypeInfo_t>	vkHeapList_t;


struct vkBuffer_t
{
	VkBuffer handle;
	VkBuffer GetHandle() const { return handle; }
};




struct vkExtension_t {
	const char *  name;
	VkBool32 available;
};

union vkDeviceExtensionList_t {
	struct {
		vkExtension_t	KHR_swapchain;
		//vkExtension_t	KHR_display_swapchain;

		vkExtension_t	NV_dedicated_allocation;
		//vkExtension_t	NV_external_memory_capabilities;
		//vkExtension_t	NV_external_memory;
		//vkExtension_t	NV_external_memory_win32;
		vkExtension_t	NV_fill_rectangle;
		//vkExtension_t	NV_fragment_coverage_to_color;
		vkExtension_t	NV_framebuffer_mixed_samples;
		vkExtension_t	NV_geometry_shader_passthrough;
		vkExtension_t	NV_glsl_shader;
		vkExtension_t	NV_sample_mask_override_coverage;
		//vkExtension_t	NV_viewport_array2;
		//vkExtension_t	NV_viewport_swizzle;
		//vkExtension_t	NVX_multiview_per_view_attributes;

		vkExtension_t	AMD_draw_indirect_count;
		vkExtension_t	AMD_gcn_shader;
		vkExtension_t	AMD_gpu_shader_half_float;
		vkExtension_t	AMD_gpu_shader_int16;
		vkExtension_t	AMD_mixed_attachment_samples;
		vkExtension_t	AMD_negative_viewport_height;
		//vkExtension_t	AMD_rasterization_order;
		vkExtension_t	AMD_shader_fragment_mask;
		vkExtension_t	AMD_shader_info;
		vkExtension_t	AMD_texture_gather_bias_lod;

		vkExtension_t	IMG_filter_cubic;
		vkExtension_t	KHR_shader_draw_parameters;
		vkExtension_t	KHR_sampler_mirror_clamp_to_edge;
		vkExtension_t	KHR_incremental_present;
		vkExtension_t	KHR_image_format_list;
		vkExtension_t	KHR_dedicated_allocation;
		vkExtension_t	KHR_bind_memory2;
		vkExtension_t	KHR_get_memory_requirements2;
		vkExtension_t	KHR_maintenance1;
		vkExtension_t	KHR_maintenance2;
		vkExtension_t	KHR_maintenance3;
		vkExtension_t	KHR_descriptor_update_template;
		//vkExtension_t	KHR_device_group;
		//vkExtension_t	KHR_device_group_creation;
		//vkExtension_t	KHR_display;
		//vkExtension_t	KHR_display_swapchain;
		//vkExtension_t	KHR_multiview;
		vkExtension_t	KHR_push_descriptor_extension;
		vkExtension_t	KHR_sampler_ycbcr_conversion;
		//vkExtension_t	KHR_shared_presentable_image;
		vkExtension_t	EXT_blend_operation_advanced;
		vkExtension_t	EXT_conservative_rasterization;
		vkExtension_t	EXT_depth_range_unrestricted;
		vkExtension_t	EXT_descriptor_indexing;
		vkExtension_t	EXT_sample_locations;
		vkExtension_t	EXT_sampler_filter_minmax;
		vkExtension_t	EXT_shader_stencil_export;
		vkExtension_t	EXT_shader_viewport_index_layer;
		vkExtension_t	EXT_validation_cache;
		vkExtension_t	EXT_vertex_attribute_divisor;
	};
	vkExtension_t *		array;
};


/*
=====================================================================================================
	vkPhysicalDevice_t
=====================================================================================================
*/
class vkPhysicalDevice_t {
public:

	// Queue family.

	// Memmory heap.
	//		mem type


	VkPhysicalDevice					m_handle;
	VkPhysicalDeviceProperties			m_props;
	VkPhysicalDeviceMemoryProperties	m_memProps;
	VkPhysicalDeviceFeatures			m_features;
	idList< VkQueueFamilyProperties >	m_queueFamilyProps;

	vkDeviceExtensionList_t				m_extensions;
	//idList< VkExtensionProperties >		m_extensionProps;

	VkSurfaceCapabilitiesKHR			m_surfaceCaps;
	idList< VkSurfaceFormatKHR >		m_surfaceFormats;
	idList< VkPresentModeKHR >	  		m_presentModes;

	/*
	=====================
	 CONSTRUCTOR
	=====================
	*/
	vkPhysicalDevice_t( VkPhysicalDevice physicalDevice ) : m_handle( physicalDevice )
	{
		memset( &m_extensions, 0, sizeof( vkDeviceExtensionList_t ) );
	}

	/*
	=====================
	 DESTRUCTOR
	=====================
	*/
	~vkPhysicalDevice_t()
	{
		if( m_handle == VK_NULL_HANDLE )
			return;

		// ... todo

		memset( this, 0, sizeof( *this ) );
	}

	VkPhysicalDevice GetHandle() const { return m_handle; }

	const char * GetVendorString() const
	{
		switch( m_props.vendorID )
		{
			case 0x1002: return "AMD";
			case 0x10DE: return "NVIDIA";
			case 0x8086: return "Intel";

			case 0x1010: return "ImgTec";
			case 0x13B5: return "ARM";
			case 0x5143: return "Qualcomm";
			default: return "Unknown";
		}
	}
	graphicsVendor_t GetVendor() const
	{
		switch( m_props.vendorID )
		{
			// AMD's drivers tested on July 11 2013 have hitching problems with async resource streaming, setting single threaded for now until fixed.
			case 0x1002: return VENDOR_AMD;
			// NVIDIA GPUs are discrete and use DedicatedVideoMemory only.
			case 0x10DE: return VENDOR_NVIDIA;
			// Intel GPUs are integrated and use both DedicatedVideoMemory and SharedSystemMemory.
			case 0x8086: return VENDOR_INTEL;
			//case 0x13B5: return "ARM";
			//case 0x5143: return "Qualcomm";
			//case 0x1010: return "ImgTec";
			//default: return "Unknown";
		}
	}

	// Get list of supported extensions
	void GetExtensions( idList< VkExtensionProperties > & supportedExtensions ) const
	{
		uint32_t extCount = 0;
		auto res = vkEnumerateDeviceExtensionProperties( GetHandle(), nullptr, &extCount, nullptr );
		assert( res == VK_SUCCESS );
		if( extCount > 0 )
		{
			supportedExtensions.SetNum( extCount );
			res = vkEnumerateDeviceExtensionProperties( GetHandle(), nullptr, &extCount, supportedExtensions.Ptr() );
			assert( res == VK_SUCCESS );
		}
	}

	void EnumerateHeaps( vkHeapList_t & ) const;

	/**
	* Get the index of a memory type that has all the requested property bits set
	*
	* @param typeBits Bitmask with bits set for each memory type supported by the resource to request for (from VkMemoryRequirements)
	* @param requirements_mask Bitmask of properties for the memory type to request
	* @param typeIndex is returned memory type index
	* @return true / false
	*/
	bool GetMemoryTypeFromProps( uint32_t typeBits, VkMemoryPropertyFlags requirements_mask, uint32_t *typeIndex ) const
	{
		// Search memtypes to find first index with those properties
		for( uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++ )
		{
			if( ( typeBits & 1 ) == 1 )
			{
				// Type is available, does it match user properties?
				if( ( m_memProps.memoryTypes[ i ].propertyFlags & requirements_mask ) == requirements_mask )
				{
					*typeIndex = i;
					return true;
				}
			}
			typeBits >>= 1;
		}
		// No memory types matched, return failure
		return false;
	}

	/**
	* Get the index of a queue family that supports the requested queue flags
	*
	* @param queueFlags Queue flags to find a queue family index for
	*
	* @return Index of the queue family index that matches the flags
	*
	* @throw Throws an exception if no queue family index could be found that supports the requested flags
	*/
	uint32_t GetQueueFamilyIndex( VkQueueFlagBits queueFlags ) const
	{
		// Dedicated queue for compute
		// Try to find a queue family index that supports compute but not graphics
		if( queueFlags & VK_QUEUE_COMPUTE_BIT )
		{
			for( uint32_t i = 0; i < static_cast<uint32_t>( m_queueFamilyProps.Num() ); i++ )
			{
				if( ( m_queueFamilyProps[ i ].queueFlags & queueFlags ) && ( ( m_queueFamilyProps[ i ].queueFlags & VK_QUEUE_GRAPHICS_BIT ) == 0 ) )
				{
					return i;
					break;
				}
			}
		}

		// Dedicated queue for transfer
		// Try to find a queue family index that supports transfer but not graphics and compute
		if( queueFlags & VK_QUEUE_TRANSFER_BIT )
		{
			for( uint32_t i = 0; i < static_cast<uint32_t>( m_queueFamilyProps.Num() ); i++ )
			{
				if( ( m_queueFamilyProps[ i ].queueFlags & queueFlags ) && ( ( m_queueFamilyProps[ i ].queueFlags & VK_QUEUE_GRAPHICS_BIT ) == 0 ) && ( ( m_queueFamilyProps[ i ].queueFlags & VK_QUEUE_COMPUTE_BIT ) == 0 ) )
				{
					return i;
					break;
				}
			}
		}

		// For other queue types or if no separate compute queue is present, return the first one to support the requested flags
		for( uint32_t i = 0; i < static_cast<uint32_t>( m_queueFamilyProps.Num() ); i++ )
		{
			if( m_queueFamilyProps[ i ].queueFlags & queueFlags )
			{
				return i;
				break;
			}
		}

		throw std::runtime_error( "Could not find a matching queue family index" );
	}

	/**
	* Get supported format for given parameters
	*/
	VkFormat ChooseSupportedFormat( VkFormat * formats, int numFormats, VkImageTiling tiling, VkFormatFeatureFlags features ) const
	{
		for( int i = 0; i < numFormats; ++i )
		{
			VkFormat format = formats[ i ];

			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties( GetHandle(), format, &props );

			if( tiling == VK_IMAGE_TILING_LINEAR && ( props.linearTilingFeatures & features ) == features )
			{
				return format;
			}
			else if( tiling == VK_IMAGE_TILING_OPTIMAL && ( props.optimalTilingFeatures & features ) == features )
			{
				return format;
			}
		}
		///idLib::FatalError( "Failed to find a supported format." );
		return VK_FORMAT_UNDEFINED;
	}


	void GetFormatProperties( VkFormat format, VkFormatProperties * formatProps ) const
	{
		vkGetPhysicalDeviceFormatProperties( GetHandle(), format, formatProps );
	}
};

// Custom define for better code readability
#define VK_FLAGS_NONE 0

// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000

struct vkQueue_t {
	VkQueue						m_handle = VK_NULL_HANDLE;
	class vkDeviceContext_t *	m_dc = nullptr;
	uint32						m_familyIndex = 0;
	uint32						m_queueIndex = 0;
	/*
	=====================
	 CONSTRUCTOR
	=====================
	*/
	vkQueue_t( class vkDeviceContext_t * dc, uint32 InFamilyIndex, uint32 InQueueIndex )
		: m_handle( VK_NULL_HANDLE ), m_familyIndex( InFamilyIndex ), m_queueIndex( InQueueIndex ), m_dc( dc )
	{
		vkGetDeviceQueue( dc->GetHandle(), InFamilyIndex, InQueueIndex, &m_handle );
	}
	/*
	=====================
	 DESTRUCTOR
	=====================
	*/
	~vkQueue_t()
	{
		memset( this, 0, sizeof( *this ) );
	}

	ID_INLINE VkQueue GetHandle() const { return m_handle; }
	ID_INLINE uint32 GetFamilyIndex() const { return m_familyIndex; }
};

/*
=====================================================================================================
	vkDeviceContext_t
=====================================================================================================
*/
class vkDeviceContext_t {
public:
	const vkPhysicalDevice_t *	m_pd = nullptr;

	// Logical device representation ( application's view of the device )
	VkDevice					m_handle = VK_NULL_HANDLE;
	// Default command pool for the graphics queue family index
	VkCommandPool				m_commandPool = VK_NULL_HANDLE;
	VkCommandPool				m_present_commandPool = VK_NULL_HANDLE;

	vkQueue_t *					m_graphicsQueue = nullptr;
	vkQueue_t *					m_computeQueue = nullptr;
	vkQueue_t *					m_transferQueue = nullptr;
	vkQueue_t *					m_presentQueue = nullptr;

	vkSwapChain_t *				m_swapChain = nullptr;

#ifdef _DEBUG
	PFN_vkDebugMarkerSetObjectTagEXT    vkDebugMarkerSetObjectTagEXT = VK_NULL_HANDLE;
	PFN_vkDebugMarkerSetObjectNameEXT   vkDebugMarkerSetObjectNameEXT = VK_NULL_HANDLE;
	PFN_vkCmdDebugMarkerBeginEXT		vkCmdDebugMarkerBeginEXT = VK_NULL_HANDLE;
	PFN_vkCmdDebugMarkerEndEXT			vkCmdDebugMarkerEndEXT = VK_NULL_HANDLE;
#endif

	VkDevice					GetHandle() const { return m_handle; }
	VkPhysicalDevice			GetPDHandle() const { return m_pd->GetHandle(); }
	const vkPhysicalDevice_t *	GetPD() const { return m_pd; }

	bool						IsPresentable() const { return( m_presentQueue != nullptr ); }
	vkQueue_t *					GetPresentQueue() const { return m_presentQueue; }

	////////////////////////////////////////////////////

	VkSemaphore m_cmdAcquireSemaphores[ NUM_FRAME_DATA ] = { VK_NULL_HANDLE };
	VkSemaphore m_cmdCompleteSemaphores[ NUM_FRAME_DATA ] = { VK_NULL_HANDLE };
	VkFence		m_cmdCompleteFences[ NUM_FRAME_DATA ] = { VK_NULL_HANDLE };
	bool		m_commandBufferRecorded[ NUM_FRAME_DATA ] = { false };
	uint64_t	m_counter = 0;
	uint32_t	m_currentFrameData = 0;

	////////////////////////////////////////////////////

	/*
	=====================
	 CONSTRUCTOR
	=====================
	*/
	vkDeviceContext_t( VkDevice device, const vkPhysicalDevice_t * pd ) : m_handle( device ), m_pd( pd )
	{
	}

	/*
	=====================
	 DESTRUCTOR
	=====================
	*/
	~vkDeviceContext_t()
	{
		if( m_handle == VK_NULL_HANDLE )
			return;

		if( m_swapChain )
		{
			delete m_swapChain;
			m_swapChain = nullptr;
		}

		vkDestroyCommandPool( m_handle, m_commandPool, nullptr );

		m_pd = nullptr;

		delete m_graphicsQueue; m_graphicsQueue = nullptr;
		delete m_computeQueue; m_computeQueue = nullptr;
		delete m_transferQueue; m_transferQueue = nullptr;
		delete m_presentQueue; m_presentQueue = nullptr;

		WaitIdle();
		vkDestroyDevice( m_handle, nullptr );
		m_handle = VK_NULL_HANDLE;
	}

	auto GetFuncPtr( const char * FuncName ) const
	{
		return vkGetDeviceProcAddr( GetHandle(), FuncName );
	}

	/**
	* Create a command pool for allocation command buffers from
	*
	* @param queueFamilyIndex Family index of the queue to create the command pool for
	* @param createFlags (Optional) Command pool creation flags (Defaults to VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
	*
	* @note Command buffers allocated from the created pool can only be submitted to a queue with the same family index
	*
	* @return A handle to the created command buffer
	*/
	VkCommandPool CreateCommandPool( uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT ) const
	{
		VkCommandPoolCreateInfo cmdPoolInfo = {
			VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr,
			createFlags, queueFamilyIndex
		};
		VkCommandPool cmdPool;
		auto res = vkCreateCommandPool( GetHandle(), &cmdPoolInfo, nullptr, &cmdPool );
		assert( res == VK_SUCCESS );
		return cmdPool;
	}

	/**
	* Allocate a command buffer from the command pool
	*
	* @param level Level of the new command buffer (primary or secondary)
	* @param (Optional) begin If true, recording on the new command buffer will be started (vkBeginCommandBuffer) (Defaults to false)
	*
	* @return A handle to the allocated command buffer
	*/
	VkCommandBuffer CreateCommandBuffer( VkCommandBufferLevel level, bool begin = false ) const
	{
		VkCommandBufferAllocateInfo cmdBufAllocateInfo = {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr,
			m_commandPool, level, 1
		};
		VkCommandBuffer cmdBuffer;
		auto res = vkAllocateCommandBuffers( GetHandle(), &cmdBufAllocateInfo, &cmdBuffer );
		assert( res == VK_SUCCESS );

		// If requested, also start recording for the new command buffer
		if( begin )
		{
			VkCommandBufferBeginInfo cmdBufInfo = {
				VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
				VK_FLAGS_NONE, nullptr
			};
			vkBeginCommandBuffer( cmdBuffer, &cmdBufInfo );
		}

		return cmdBuffer;
	}

	/**
	* Finish command buffer recording and submit it to a queue
	*
	* @param commandBuffer Command buffer to flush
	* @param queue Queue to submit the command buffer to
	* @param free (Optional) Free the command buffer once it has been submitted (Defaults to true)
	*
	* @note The queue that the command buffer is submitted to must be from the same family index as the pool it was allocated from
	* @note Uses a fence to ensure command buffer has finished executing
	*/
	void FlushCommandBuffer( VkCommandBuffer commandBuffer, VkQueue queue, bool free = true ) const
	{
		assert( commandBuffer != VK_NULL_HANDLE );

		vkEndCommandBuffer( commandBuffer );

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		// Create fence to ensure that the command buffer has finished executing
		VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 0 };
		VkFence fence;
		vkCreateFence( GetHandle(), &fenceInfo, nullptr, &fence );

		// Submit to the queue
		vkQueueSubmit( queue, 1, &submitInfo, fence );
		// Wait for the fence to signal that command buffer has finished executing
		vkWaitForFences( GetHandle(), 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT );

		vkDestroyFence( GetHandle(), fence, nullptr );

		if( free ) {
			vkFreeCommandBuffers( GetHandle(), m_commandPool, 1, &commandBuffer );
		}
	}

	ID_INLINE void WaitIdle() const
	{
		vkDeviceWaitIdle( GetHandle() );
	}


	void SubmitWorkToGPU();

	void Present();
};

typedef idStaticList< vkPhysicalDevice_t *, 4 >	vkPhysicalDeviceList_t;
typedef idStaticList< vkDeviceContext_t *, 8 >	vkDeviceContextList_t;

/*
=====================================================================================================
	idVulkanInterface
=====================================================================================================
*/
class idVulkanInterface
{
	friend class vkPhysicalDevice_t;
	friend class vkDeviceContext_t;

	// Instance.
	// SurfaceKHR.

	VkInstance				m_instance = VK_NULL_HANDLE;
	idStrStatic<64>			m_appName;

	vkPhysicalDeviceList_t	m_gpus;
	vkDeviceContextList_t	m_deviceContexts;

	void	CreateInstance();
	void	EnumeratePhysicalDevices( vkPhysicalDeviceList_t & ) const;

public:

	auto GetFuncPtr( const char * FuncName ) const
	{
		return vkGetInstanceProcAddr( m_instance, FuncName );
	}

	//PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
	//PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
	//PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
	//PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
	//PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
#ifdef _DEBUG
	PFN_vkSubmitDebugUtilsMessageEXT vkSubmitDebugUtilsMessageEXT = VK_NULL_HANDLE;
	PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = VK_NULL_HANDLE;
	PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT = VK_NULL_HANDLE;
	PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT = VK_NULL_HANDLE;
	PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT	m_dbgMessenger = VK_NULL_HANDLE;

	PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT = VK_NULL_HANDLE;
	VkDebugReportCallbackEXT	m_dbgReportCallback = VK_NULL_HANDLE;

	bool	EXT_debug_utils = false;
	bool	EXT_debug_report = false;
	bool	EXT_debug_marker = false;
#endif
	bool	KHR_get_physical_device_properties2 = false;
	bool	KHR_get_surface_capabilities2 = false;
	bool	EXT_direct_mode_display = false; // for vr
	bool	EXT_display_surface_counter = false;
	bool	EXT_swapchain_color_space = false;
	bool	EXT_validation_flags = false;

public:
	void	Init();
	void	Shutdown();

	ID_INLINE const char *				GetAppName() const { return m_appName.c_str(); }
	ID_INLINE VkInstance				GetInstance() const { return m_instance; }
	ID_INLINE vkPhysicalDevice_t *		GetGPU( int index ) const { return m_gpus[ index ]; }
	ID_INLINE vkDeviceContext_t *		GetDeviceContext( int index ) const { return m_deviceContexts[ index ]; }

	vkDeviceContext_t *					CreateDeviceContext( uint32_t gpu, bool bPresentable );
	void								DestroyDeviceContext( vkDeviceContext_t * );

	ID_INLINE vkDeviceContext_t *		GetCurrDC()
	{
		const int index = 0;
		return GetDeviceContext( index );
	}

private:
	PFN_vkDestroyInstance				vkDestroyInstance;
	PFN_vkEnumeratePhysicalDevices		vkEnumeratePhysicalDevices;
	PFN_vkGetInstanceProcAddr			vkGetInstanceProcAddr;

	PFN_vkDestroySurfaceKHR				vkDestroySurfaceKHR;
};

extern idVulkanInterface				vkSys;

#endif /*__VK_H__*/



