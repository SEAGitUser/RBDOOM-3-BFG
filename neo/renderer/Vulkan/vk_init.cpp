
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

/////////////////////////////////////////////////////////////////////////////////////////////////////

idVulkanInterface	vkSys;

//#include <vector>
//#include <string>

#include "..\tr_local.h"

idCVar r_vk_allowPresentOnComputeQueue( "r_vk_allowPresentOnComputeQueue", "1", CVAR_BOOL, "" );
idCVar r_vk_allowAsyncCompute( "r_vk_allowAsyncCompute", "1", CVAR_BOOL, "" );


#define GET_IPROC_LOCAL( NAME ) \
PFN_##NAME NAME = VK_NULL_HANDLE; \
NAME = reinterpret_cast<PFN_##NAME>( vkGetInstanceProcAddr( vkSys.GetInstance(), #NAME ) ); \
ID_VK_VALIDATE( NAME != VK_NULL_HANDLE, #NAME"== NULL" )

/////////////////////////////////////////////////////////////////////////////////////////////////////

static bool CheckExtensionAvailability( const char *extension_name, const idList<VkExtensionProperties> & available_extensions )
{
	for( size_t i = 0; i < available_extensions.Num(); ++i )
	{
		if( idStr::Cmp( available_extensions[ i ].extensionName, extension_name ) == 0 )
			return true;
	}
	return false;
}

// Return 1 (true) if all layer names specified in check_names can be found in given layer properties.
static VkBool32 CheckInstanceLayers( uint32_t check_count, const char ** check_names, uint32_t layer_count, VkLayerProperties *layers )
{
	for( uint32_t i = 0; i < check_count; i++ )
	{
		VkBool32 found = 0;
		for( uint32_t j = 0; j < layer_count; j++ )
		{
			if( !idStr::Cmp( check_names[ i ], layers[ j ].layerName ) )
			{
				found = 1;
				break;
			}
		}
		if( !found )
		{
			common->DWarning( "Cannot find layer: %s\n", check_names[ i ] );
			return 0;
		}
	}
	return 1;
}

#ifdef _DEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback( VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType,
	uint64 obj, size_t location, int32 code, const char * layerPrefix, const char * msg, void * userData )
{
	common->DPrintf( "VK_DEBUG::%s: %s flags=%d, objType=%d, obj=%llu, location=%lld, code=%d\n", layerPrefix, msg, flags, objType, obj, location, code );
	return VK_FALSE;
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessengerCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData )
{
#if 0
	char prefix[ 64 ] = "";
	char *message = ( char * )malloc( strlen( pCallbackData->pMessage ) + 5000 );
	assert( message );
	struct demo *demo = ( struct demo * )pUserData;

	const bool use_break = false;
	if( use_break )
	{
	#ifndef WIN32
		::raise( SIGTRAP );
	#else
		::DebugBreak();
	#endif
	}

	if( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT )
	{
		strcat( prefix, "VERBOSE: " );
	}
	else if( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT )
	{
		strcat( prefix, "INFO: " );
	}
	else if( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
	{
		strcat( prefix, "WARNING: " );
	}
	else if( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
	{
		strcat( prefix, "ERROR: " );
	}

	if( messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT )
	{
		strcat( prefix, "GENERAL" );
	}
	else {
		if( messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT )
		{
			strcat( prefix, "VALIDATION" );
			validation_error = 1;
		}
		if( messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT )
		{
			if( messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT )
			{
				strcat( prefix, "|" );
			}
			strcat( prefix, "PERFORMANCE" );
		}
	}

	sprintf( message, "%s - Message Id Number: %d | Message Id Name: %s\n\t%s\n", prefix, pCallbackData->messageIdNumber,
		pCallbackData->pMessageIdName, pCallbackData->pMessage );
	if( pCallbackData->objectCount > 0 )
	{
		char tmp_message[ 500 ];
		sprintf( tmp_message, "\n\tObjects - %d\n", pCallbackData->objectCount );
		strcat( message, tmp_message );
		for( uint32_t object = 0; object < pCallbackData->objectCount; ++object )
		{
			if( NULL != pCallbackData->pObjects[ object ].pObjectName && strlen( pCallbackData->pObjects[ object ].pObjectName ) > 0 )
			{
				sprintf( tmp_message, "\t\tObject[%d] - %s, Handle %p, Name \"%s\"\n", object,
					string_VkObjectType( pCallbackData->pObjects[ object ].objectType ),
					( void * )( pCallbackData->pObjects[ object ].objectHandle ), pCallbackData->pObjects[ object ].pObjectName );
			}
			else
			{
				sprintf( tmp_message, "\t\tObject[%d] - %s, Handle %p\n", object,
					string_VkObjectType( pCallbackData->pObjects[ object ].objectType ),
					( void * )( pCallbackData->pObjects[ object ].objectHandle ) );
			}
			strcat( message, tmp_message );
		}
	}

	if( pCallbackData->cmdBufLabelCount > 0 )
	{
		char tmp_message[ 500 ];
		sprintf( tmp_message, "\n\tCommand Buffer Labels - %d\n", pCallbackData->cmdBufLabelCount );
		strcat( message, tmp_message );
		for( uint32_t cmd_buf_label = 0; cmd_buf_label < pCallbackData->cmdBufLabelCount; ++cmd_buf_label )
		{
			sprintf( tmp_message, "\t\tLabel[%d] - %s { %f, %f, %f, %f}\n", cmd_buf_label,
				pCallbackData->pCmdBufLabels[ cmd_buf_label ].pLabelName, pCallbackData->pCmdBufLabels[ cmd_buf_label ].color[ 0 ],
				pCallbackData->pCmdBufLabels[ cmd_buf_label ].color[ 1 ], pCallbackData->pCmdBufLabels[ cmd_buf_label ].color[ 2 ],
				pCallbackData->pCmdBufLabels[ cmd_buf_label ].color[ 3 ] );
			strcat( message, tmp_message );
		}
	}

#if defined( VK_USE_PLATFORM_WIN32_KHR )

	in_callback = true;
	if( !demo->suppress_popups )
	{
		MessageBox( NULL, message, "Alert", MB_OK );
	}
	in_callback = false;

#elif defined( VK_USE_PLATFORM_ANDROID_KHR )

	if( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT )
	{
		__android_log_print( ANDROID_LOG_INFO, APP_SHORT_NAME, "%s", message );
	}
	else if( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
	{
		__android_log_print( ANDROID_LOG_WARN, APP_SHORT_NAME, "%s", message );
	}
	else if( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
	{
		__android_log_print( ANDROID_LOG_ERROR, APP_SHORT_NAME, "%s", message );
	}
	else if( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT )
	{
		__android_log_print( ANDROID_LOG_VERBOSE, APP_SHORT_NAME, "%s", message );
	}
	else {
		__android_log_print( ANDROID_LOG_INFO, APP_SHORT_NAME, "%s", message );
	}

#else

	printf( "%s\n", message );
	fflush( stdout );

#endif

	free( message );
#endif

	// Don't bail out, but keep going.
	return false;
}
#endif

/*
========================================
 CreateInstance
========================================
*/
void idVulkanInterface::CreateInstance()
{
	common->DPrintf( "VK: CreateInstance\n" );

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = GAME_NAME;
	appInfo.applicationVersion = VK_MAKE_VERSION( BUILD_NUMBER, BUILD_NUMBER_MINOR, 0 );
	appInfo.pEngineName = ENGINE_NAME;
	appInfo.engineVersion = VK_MAKE_VERSION( ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_PATCH );
	appInfo.apiVersion = VK_MAKE_VERSION( 1, 0, VK_HEADER_VERSION );

	const char * core_requested_extensions[] = {
		VK_KHR_SURFACE_EXTENSION_NAME,

	#if defined( VK_USE_PLATFORM_WIN32_KHR )
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,

	#elif defined( VK_USE_PLATFORM_ANDROID_KHR )
		VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,

	#elif defined( VK_USE_PLATFORM_IOS_MVK )
		VK_MVK_IOS_SURFACE_EXTENSION_NAME,

	#elif defined( VK_USE_PLATFORM_MACOS_MVK )
		VK_MVK_MACOS_SURFACE_EXTENSION_NAME,

	#elif defined( VK_USE_PLATFORM_WAYLAND_KHR )
		VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,

	#elif defined( VK_USE_PLATFORM_XLIB_KHR )
		VK_KHR_XLIB_SURFACE_EXTENSION_NAME,

	#elif defined( VK_USE_PLATFORM_XCB_KHR )
		VK_KHR_XCB_SURFACE_EXTENSION_NAME,

	#endif
	};
	idStaticList<const char *, 32> requested_extensions;
	{
		idList<VkExtensionProperties> available_extensions;
		{
			uint32_t extCount = 0;
			ID_VK_CHECK( vkEnumerateInstanceExtensionProperties( NULL, &extCount, NULL ) );
			available_extensions.SetNum( extCount );
			ID_VK_CHECK( vkEnumerateInstanceExtensionProperties( NULL, &extCount, available_extensions.Ptr() ) );
			for( uint32_t i = 0; i < extCount; i++ )
			{
				common->DPrintf( "Available extension %u - %s\n", i, available_extensions[ i ].extensionName );
			}
		}

		for( uint32_t i = 0; i < _countof( core_requested_extensions ); ++i )
		{
			if( !CheckExtensionAvailability( core_requested_extensions[ i ], available_extensions ) )
			{
				common->FatalError( "Could't find instance extension '%s'", core_requested_extensions[ i ] );
			}
			requested_extensions.Append( core_requested_extensions[ i ] );
		}

	#define CheckExtension( EXTENSION_NAME, EXTENSION_NAME_BOOL ) \
		if( CheckExtensionAvailability( EXTENSION_NAME, available_extensions ) ) { \
			common->DPrintf( "Using: %s\n", EXTENSION_NAME ); \
			requested_extensions.Append( EXTENSION_NAME ); \
			EXTENSION_NAME_BOOL = true; \
		} else common->DWarning( "Could't find instance extension '%s'", EXTENSION_NAME )
	#ifdef _DEBUG
		// this extension combines the functionality of both VK_EXT_debug_report and VK_EXT_debug_marker by allowing object
		// name and debug markers( now called labels ) to be returned to the application’s callback function.
		CheckExtension( VK_EXT_DEBUG_UTILS_EXTENSION_NAME, EXT_debug_utils );
		if( !EXT_debug_utils ) {
			CheckExtension( VK_EXT_DEBUG_REPORT_EXTENSION_NAME, EXT_debug_report );
			CheckExtension( VK_EXT_DEBUG_MARKER_EXTENSION_NAME, EXT_debug_marker );
		}
	#endif
		CheckExtension( VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, KHR_get_physical_device_properties2 );
		CheckExtension( VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME, KHR_get_surface_capabilities2 );
		CheckExtension( VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME, EXT_direct_mode_display );
		CheckExtension( VK_EXT_DISPLAY_SURFACE_COUNTER_EXTENSION_NAME, EXT_display_surface_counter );
		CheckExtension( VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME, EXT_swapchain_color_space );
		CheckExtension( VK_EXT_VALIDATION_FLAGS_EXTENSION_NAME, EXT_validation_flags );

	#undef CheckExtension
	}

	VkInstanceCreateInfo instInfo = {};
	instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instInfo.pApplicationInfo = &appInfo;
#ifdef _DEBUG
	const char * requested_layers[] = {
		"VK_LAYER_LUNARG_standard_validation"
	};
	const char * requested_layers2[] = {
		"VK_LAYER_GOOGLE_threading",
		"VK_LAYER_LUNARG_parameter_validation",
		"VK_LAYER_LUNARG_object_tracker",
		"VK_LAYER_LUNARG_core_validation",
		"VK_LAYER_GOOGLE_unique_objects"
	};
	{
		/* Look for validation layers */
		VkBool32 validation_found = VK_FALSE;

		uint32_t layerCount;
		auto res = vkEnumerateInstanceLayerProperties( &layerCount, NULL );
		assert( !res );

		if( layerCount > 0 )
		{
			idList<VkLayerProperties> instance_layers;
			instance_layers.SetNum( layerCount );

			res = vkEnumerateInstanceLayerProperties( &layerCount, instance_layers.Ptr() );
			assert( !res );

			validation_found = CheckInstanceLayers( _countof( requested_layers ), requested_layers, layerCount, instance_layers.Ptr() );
			if( validation_found )
			{
				instInfo.enabledLayerCount = _countof( requested_layers );
				instInfo.ppEnabledLayerNames = requested_layers;
			}
			else {
				// use alternative set of validation layers
				validation_found = CheckInstanceLayers( _countof( requested_layers2 ), requested_layers2, layerCount, instance_layers.Ptr() );
				instInfo.enabledLayerCount = _countof( requested_layers2 );
				instInfo.ppEnabledLayerNames = requested_layers2;
			}
		}

		ID_VK_VALIDATE( validation_found, "vkEnumerateInstanceLayerProperties failed to find required validation layer" );
	}
#endif
	instInfo.enabledExtensionCount = requested_extensions.Num();
	instInfo.ppEnabledExtensionNames = requested_extensions.Ptr();

#ifdef _DEBUG
	VkDebugUtilsMessengerCreateInfoEXT dbg_messenger_create_info = {};
	if( EXT_debug_utils ) {
		// This is info for a temp callback to use during CreateInstance.
		// After the instance is created, we use the instance-based
		// function to register the final callback.
		dbg_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		dbg_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		dbg_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		dbg_messenger_create_info.pfnUserCallback = DebugMessengerCallback;
		dbg_messenger_create_info.pUserData = &vkSys;

		instInfo.pNext = &dbg_messenger_create_info;
	}
#endif

	ID_VK_CHECK( vkCreateInstance( &instInfo, NULL, &m_instance ) );

#ifdef _DEBUG
	if( EXT_debug_utils )
	{
		vkSubmitDebugUtilsMessageEXT = ( PFN_vkSubmitDebugUtilsMessageEXT )vkGetInstanceProcAddr( GetInstance(), "vkSubmitDebugUtilsMessageEXT" );
		ID_VK_VALIDATE( vkSubmitDebugUtilsMessageEXT, "vkGetInstanceProcAddr( vkSubmitDebugUtilsMessageEXT ) Fail" );

		vkCmdBeginDebugUtilsLabelEXT = ( PFN_vkCmdBeginDebugUtilsLabelEXT )vkGetInstanceProcAddr( GetInstance(), "vkCmdBeginDebugUtilsLabelEXT" );
		ID_VK_VALIDATE( vkCmdBeginDebugUtilsLabelEXT, "vkGetInstanceProcAddr( vkCmdBeginDebugUtilsLabelEXT ) Fail" );

		vkCmdEndDebugUtilsLabelEXT = ( PFN_vkCmdEndDebugUtilsLabelEXT )vkGetInstanceProcAddr( GetInstance(), "vkCmdEndDebugUtilsLabelEXT" );
		ID_VK_VALIDATE( vkCmdEndDebugUtilsLabelEXT, "vkGetInstanceProcAddr( vkCmdEndDebugUtilsLabelEXT ) Fail" );

		vkCmdInsertDebugUtilsLabelEXT = ( PFN_vkCmdInsertDebugUtilsLabelEXT )vkGetInstanceProcAddr( GetInstance(), "vkCmdInsertDebugUtilsLabelEXT" );
		ID_VK_VALIDATE( vkCmdInsertDebugUtilsLabelEXT, "vkGetInstanceProcAddr( vkCmdInsertDebugUtilsLabelEXT ) Fail" );

		vkSetDebugUtilsObjectNameEXT = ( PFN_vkSetDebugUtilsObjectNameEXT )vkGetInstanceProcAddr( GetInstance(), "vkSetDebugUtilsObjectNameEXT" );
		ID_VK_VALIDATE( vkSetDebugUtilsObjectNameEXT, "vkGetInstanceProcAddr( vkSetDebugUtilsObjectNameEXT ) Fail" );

		GET_IPROC_LOCAL( vkCreateDebugUtilsMessengerEXT );
		ID_VK_CHECK( vkCreateDebugUtilsMessengerEXT( GetInstance(), &dbg_messenger_create_info, NULL, &m_dbgMessenger ) );
	}
	else {
		vkDebugReportMessageEXT = ( PFN_vkDebugReportMessageEXT )vkGetInstanceProcAddr( GetInstance(), "vkDebugReportMessageEXT" );
		ID_VK_VALIDATE( vkDebugReportMessageEXT, "vkGetInstanceProcAddr( vkDebugReportMessageEXT ) Fail" );

		GET_IPROC_LOCAL( vkCreateDebugReportCallbackEXT );

		VkDebugReportCallbackCreateInfoEXT callbackCreateInfo = {};
		callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
		callbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		callbackCreateInfo.pfnCallback = ( PFN_vkDebugReportCallbackEXT )DebugCallback;
		callbackCreateInfo.pUserData = &vkSys;

		ID_VK_CHECK( vkCreateDebugReportCallbackEXT( GetInstance(), &callbackCreateInfo, NULL, &m_dbgReportCallback ) );
	}
#endif
}

/*
========================================
 EnumeratePhysicalDevices
========================================
*/
void idVulkanInterface::EnumeratePhysicalDevices( vkPhysicalDeviceList_t & gpus ) const
{
	common->DPrintf("VK: EnumeratePhysicalDevices\n");

	uint32_t numDevices = 0;
	ID_VK_CHECK( vkEnumeratePhysicalDevices( GetInstance(), &numDevices, NULL ) );
	ID_VK_VALIDATE( numDevices > 0, "vkEnumeratePhysicalDevices returned zero devices." );

	idStaticList< VkPhysicalDevice, 4 > devices; devices.SetNum( numDevices );
	ID_VK_CHECK( vkEnumeratePhysicalDevices( GetInstance(), &numDevices, devices.Ptr() ) );

	gpus.SetNum( numDevices );
	for( uint32_t i = 0; i < numDevices; ++i )
	{
		gpus[ i ] = new( TAG_RENDER ) vkPhysicalDevice_t( devices[ i ] );
		auto & gpu = *gpus[ i ];

		vkGetPhysicalDeviceProperties( gpu.GetHandle(), &gpu.m_props );
		vkGetPhysicalDeviceMemoryProperties( gpu.GetHandle(), &gpu.m_memProps );
		vkGetPhysicalDeviceFeatures( gpu.GetHandle(), &gpu.m_features );

		common->DPrintf( "Device_%u:\n", i );
		common->DPrintf( "\tDriver Version: %d\n", gpu.m_props.driverVersion );
		common->DPrintf( "\tDevice Name:    %s\n", gpu.m_props.deviceName );
		common->DPrintf( "\tDevice Type:    %d\n", gpu.m_props.deviceType );
		common->DPrintf( "\tAPI Version:    %d.%d.%d\n", VK_VERSION_MAJOR( gpu.m_props.apiVersion ), VK_VERSION_MINOR( gpu.m_props.apiVersion ), VK_VERSION_PATCH( gpu.m_props.apiVersion ) );

		{
			uint32_t numQueues = 0;
			vkGetPhysicalDeviceQueueFamilyProperties( gpu.GetHandle(), &numQueues, NULL );
			ID_VK_VALIDATE( numQueues > 0, "vkGetPhysicalDeviceQueueFamilyProperties returned zero queues." );

			gpu.m_queueFamilyProps.SetNum( numQueues );
			vkGetPhysicalDeviceQueueFamilyProperties( gpu.GetHandle(), &numQueues, gpu.m_queueFamilyProps.Ptr() );

			// Print the families
			for( uint32_t j = 0; j < numQueues; j++ )
			{
				common->DPrintf( "\t( %u )Count of Queues: %u\n", j, gpu.m_queueFamilyProps[ j ].queueCount );
				common->DPrintf( "\tSupported operationg on this queue:\n" );

				if( gpu.m_queueFamilyProps[ j ].queueFlags & VK_QUEUE_GRAPHICS_BIT )
					common->DPrintf( "\t\t Graphics\n" );

				if( gpu.m_queueFamilyProps[ j ].queueFlags & VK_QUEUE_COMPUTE_BIT )
					common->DPrintf( "\t\t Compute\n" );

				if( gpu.m_queueFamilyProps[ j ].queueFlags & VK_QUEUE_TRANSFER_BIT )
					common->DPrintf( "\t\t Transfer\n" );

				if( gpu.m_queueFamilyProps[ j ].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT )
					common->DPrintf( "\t\t Sparse Binding\n" );
			}
		}

		{
			idList< VkExtensionProperties >	extensionProps;
			gpu.GetExtensions( extensionProps );
			common->DPrintf( "\tAvailable extensions:\n" );
			for( int i = 0; i < extensionProps.Num(); ++i )
			{
				common->DPrintf( "\t\t%s\n", extensionProps[ i ].extensionName );
			}
		}
	}
}


static uint32_t ChooseRenderDevice( vkPhysicalDeviceList_t & gpus )
{
	return 0;
}

static void GetRequestedExtensions( vkPhysicalDevice_t * pd, idStaticList< const char *, 64> & requested_extensions, bool bPresentable )
{
/*
	// Must have !!!
	const char * core_requested_extensions[] =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	// Not strictly necessary.
	const char * nv_requested_extensions[] =
	{
		VK_NV_DEDICATED_ALLOCATION_EXTENSION_NAME,
		//VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
		//VK_NV_EXTERNAL_MEMORY_EXTENSION_NAME,
		//VK_NV_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
		VK_NV_FILL_RECTANGLE_EXTENSION_NAME,
		//VK_NV_FRAGMENT_COVERAGE_TO_COLOR_EXTENSION_NAME,
		VK_NV_FRAMEBUFFER_MIXED_SAMPLES_EXTENSION_NAME,
		VK_NV_GEOMETRY_SHADER_PASSTHROUGH_EXTENSION_NAME,
		VK_NV_GLSL_SHADER_EXTENSION_NAME,
		VK_NV_SAMPLE_MASK_OVERRIDE_COVERAGE_EXTENSION_NAME,
		//VK_NV_VIEWPORT_ARRAY2_EXTENSION_NAME,
		//VK_NV_VIEWPORT_SWIZZLE_EXTENSION_NAME,
		//VK_NVX_MULTIVIEW_PER_VIEW_ATTRIBUTES_EXTENSION_NAME,
	};
	// Not strictly necessary.
	const char * amd_requested_extensions[] =
	{
		//VK_AMD_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
		VK_AMD_GCN_SHADER_EXTENSION_NAME,
		VK_AMD_GPU_SHADER_HALF_FLOAT_EXTENSION_NAME,
		VK_AMD_GPU_SHADER_INT16_EXTENSION_NAME,
		VK_AMD_MIXED_ATTACHMENT_SAMPLES_EXTENSION_NAME,
		VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_EXTENSION_NAME,
		//VK_AMD_RASTERIZATION_ORDER_EXTENSION_NAME,
		VK_AMD_SHADER_FRAGMENT_MASK_EXTENSION_NAME,
		VK_AMD_SHADER_INFO_EXTENSION_NAME,
		VK_AMD_TEXTURE_GATHER_BIAS_LOD_EXTENSION_NAME,
	};
	// Not strictly necessary.
	const char * misc_requested_extensions[] =
	{
		VK_IMG_FILTER_CUBIC_EXTENSION_NAME,
		VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
		VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME,
		VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME,
		VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,

		VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,

		VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
		VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,

		VK_KHR_MAINTENANCE1_EXTENSION_NAME,
		VK_KHR_MAINTENANCE2_EXTENSION_NAME,
		VK_KHR_MAINTENANCE3_EXTENSION_NAME,

		VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME,

		VK_KHR_DEVICE_GROUP_EXTENSION_NAME,
		VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME,

		VK_KHR_DISPLAY_EXTENSION_NAME,
		VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME,

		VK_KHR_MULTIVIEW_EXTENSION_NAME,
		VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
		VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME,
		VK_KHR_SHARED_PRESENTABLE_IMAGE_EXTENSION_NAME,
		VK_EXT_BLEND_OPERATION_ADVANCED_EXTENSION_NAME,
		VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME,
		VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
		VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME,
		VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME,
		VK_EXT_SHADER_STENCIL_EXPORT_EXTENSION_NAME,
		VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME,
		VK_EXT_VALIDATION_CACHE_EXTENSION_NAME,
		VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME,
	};

	idList< VkExtensionProperties > supportedExtensions;
	pd->GetExtensions( supportedExtensions );

	for( size_t i = 0; i < _countof( core_requested_extensions ); ++i )
	{
		if( !CheckExtensionAvailability( core_requested_extensions[ i ], supportedExtensions ) )
		{
			common->FatalError( "Not supported extension: '%s'", core_requested_extensions[ i ] );
		}
		requested_extensions.Append( core_requested_extensions[ i ] );
	}
*/
	idList< VkExtensionProperties > supportedExtensions;
	pd->GetExtensions( supportedExtensions );

	auto & exts = pd->m_extensions;

	if( bPresentable )
	{
		exts.KHR_swapchain.name = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
		exts.KHR_swapchain.available = CheckExtensionAvailability( VK_KHR_SWAPCHAIN_EXTENSION_NAME, supportedExtensions );
		if( exts.KHR_swapchain.available )
			common->FatalError( "Not supported extension: %s", VK_KHR_SWAPCHAIN_EXTENSION_NAME );
	}
	// Not strictly necessary extensions:

	exts.NV_dedicated_allocation.name = VK_NV_DEDICATED_ALLOCATION_EXTENSION_NAME;
	//exts.NV_external_memory_capabilities.name = VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME;
	//exts.NV_external_memory.name = VK_NV_EXTERNAL_MEMORY_EXTENSION_NAME;
	//exts.NV_external_memory_win32.name = VK_NV_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME;
	exts.NV_fill_rectangle.name = VK_NV_FILL_RECTANGLE_EXTENSION_NAME;
	//exts.NV_fragment_coverage_to_color.name = VK_NV_FRAGMENT_COVERAGE_TO_COLOR_EXTENSION_NAME;
	exts.NV_framebuffer_mixed_samples.name = VK_NV_FRAMEBUFFER_MIXED_SAMPLES_EXTENSION_NAME;
	exts.NV_geometry_shader_passthrough.name = VK_NV_GEOMETRY_SHADER_PASSTHROUGH_EXTENSION_NAME;
	exts.NV_glsl_shader.name = VK_NV_GLSL_SHADER_EXTENSION_NAME;
	exts.NV_sample_mask_override_coverage.name = VK_NV_SAMPLE_MASK_OVERRIDE_COVERAGE_EXTENSION_NAME;
	//exts.NV_viewport_array2.name = VK_NV_VIEWPORT_ARRAY2_EXTENSION_NAME;
	//exts.NV_viewport_swizzle.name = VK_NV_VIEWPORT_SWIZZLE_EXTENSION_NAME;
	//exts.NVX_multiview_per_view_attributes.name = VK_NVX_MULTIVIEW_PER_VIEW_ATTRIBUTES_EXTENSION_NAME;

	exts.AMD_draw_indirect_count.name = VK_AMD_DRAW_INDIRECT_COUNT_EXTENSION_NAME;
	exts.AMD_gcn_shader.name = VK_AMD_GCN_SHADER_EXTENSION_NAME;
	exts.AMD_gpu_shader_half_float.name = VK_AMD_GPU_SHADER_HALF_FLOAT_EXTENSION_NAME;
	exts.AMD_gpu_shader_int16.name = VK_AMD_GPU_SHADER_INT16_EXTENSION_NAME;
	exts.AMD_mixed_attachment_samples.name = VK_AMD_MIXED_ATTACHMENT_SAMPLES_EXTENSION_NAME;
	exts.AMD_negative_viewport_height.name = VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_EXTENSION_NAME;
	//exts.AMD_rasterization_order.name = VK_AMD_RASTERIZATION_ORDER_EXTENSION_NAME;
	exts.AMD_shader_fragment_mask.name = VK_AMD_SHADER_FRAGMENT_MASK_EXTENSION_NAME;
	exts.AMD_shader_info.name = VK_AMD_SHADER_INFO_EXTENSION_NAME;
	exts.AMD_texture_gather_bias_lod.name = VK_AMD_TEXTURE_GATHER_BIAS_LOD_EXTENSION_NAME;

	exts.IMG_filter_cubic.name = VK_IMG_FILTER_CUBIC_EXTENSION_NAME;
	exts.KHR_shader_draw_parameters.name = VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME;
	exts.KHR_sampler_mirror_clamp_to_edge.name = VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME;
	exts.KHR_incremental_present.name = VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME;
	exts.KHR_image_format_list.name = VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME;
	exts.KHR_dedicated_allocation.name = VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME;
	exts.KHR_bind_memory2.name = VK_KHR_BIND_MEMORY_2_EXTENSION_NAME;
	exts.KHR_get_memory_requirements2.name = VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME;
	exts.KHR_maintenance1.name = VK_KHR_MAINTENANCE1_EXTENSION_NAME;
	exts.KHR_maintenance2.name = VK_KHR_MAINTENANCE2_EXTENSION_NAME;
	exts.KHR_maintenance3.name = VK_KHR_MAINTENANCE3_EXTENSION_NAME;
	exts.KHR_descriptor_update_template.name = VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME;
	//exts.KHR_device_group.name = VK_KHR_DEVICE_GROUP_EXTENSION_NAME;
	//exts.KHR_device_group_creation.name = VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME;
	//exts.KHR_display.name = VK_KHR_DISPLAY_EXTENSION_NAME;
	//exts.KHR_display_swapchain.name = VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME;
	//exts.KHR_multiview.name = VK_KHR_MULTIVIEW_EXTENSION_NAME;
	exts.KHR_push_descriptor_extension.name = VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME;
	exts.KHR_sampler_ycbcr_conversion.name = VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME;
	//exts.KHR_shared_presentable_image.name = VK_KHR_SHARED_PRESENTABLE_IMAGE_EXTENSION_NAME;
	exts.EXT_blend_operation_advanced.name = VK_EXT_BLEND_OPERATION_ADVANCED_EXTENSION_NAME;
	exts.EXT_conservative_rasterization.name = VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME;
	exts.EXT_depth_range_unrestricted.name = VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME;
	exts.EXT_descriptor_indexing.name = VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME;
	exts.EXT_sample_locations.name = VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME;
	exts.EXT_sampler_filter_minmax.name = VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME;
	exts.EXT_shader_stencil_export.name = VK_EXT_SHADER_STENCIL_EXPORT_EXTENSION_NAME;
	exts.EXT_shader_viewport_index_layer.name = VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME;
	exts.EXT_validation_cache.name = VK_EXT_VALIDATION_CACHE_EXTENSION_NAME;
	exts.EXT_vertex_attribute_divisor.name = VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME;

	const uint32_t exts_count = ( sizeof( exts ) / sizeof( vkExtension_t ) );
	for( size_t i = 0; i < exts_count; ++i )
	{
		exts.array[ i ].available = CheckExtensionAvailability( exts.array[ i ].name, supportedExtensions );
		if( !exts.array[ i ].available )
		{
			common->DPrintf( "Using extension: %s\n", exts.array[ i ].name );
			requested_extensions.Append( exts.array[ i ].name );
		}
	}
}

// Device-only layers are now deprecated!
static void GetRequestedLayers( const vkPhysicalDevice_t * pd, idStaticList< const char *, 16> & requested_layers ) {
#if _DEBUG
	const char* deviceLayers[] = {
		"VK_LAYER_GOOGLE_threading",
		//"VK_LAYER_GOOGLE_unique_objects",
		"VK_LAYER_LUNARG_device_limits",
		//"VK_LAYER_LUNARG_standard_validation",
		"VK_LAYER_LUNARG_image",
		"VK_LAYER_LUNARG_object_tracker",
		"VK_LAYER_LUNARG_parameter_validation",
		"VK_LAYER_LUNARG_swapchain",
		//"VK_LAYER_LUNARG_vktrace",
		//"VK_LAYER_RENDERDOC_Capture",
	};
	for( uint32_t i = 0; i < _countof( deviceLayers ); ++i )
	{
		requested_layers.Append( deviceLayers[ i ] );
	}

	VkResult res;
	uint32_t propCount = 0;
	res = vkEnumerateDeviceLayerProperties( pd->GetHandle(), &propCount, NULL );
	auto availableProps = ( VkLayerProperties *)_alloca( sizeof( VkLayerProperties ) * propCount );
	res = vkEnumerateDeviceLayerProperties( pd->GetHandle(), &propCount, availableProps );

	for( uint32_t i = 0; i < _countof( deviceLayers ); i++ )
	{
		VkBool32 found = 0;
		for( uint32_t j = 0; j < propCount; j++ )
		{
			if( !idStr::Cmp( availableProps[ j ].layerName, deviceLayers[ i ] ) )
			{
				found = 1;
				requested_layers.Append( deviceLayers[ i ] );
				common->DPrintf( "Using %s\n  Description: %s\n", availableProps[ j ].layerName, availableProps[ j ].description );
				break;
			}
		}
		if( !found ) {
			common->DWarning( "Cannot find layer: %s\n", deviceLayers[ i ] );
		}
	}
#endif
}

static void GetRequestedFeatures( const vkPhysicalDevice_t * gpu, VkPhysicalDeviceFeatures & parms )
{
	VkPhysicalDeviceFeatures available_features;
	vkGetPhysicalDeviceFeatures( gpu->GetHandle(), &available_features );

	memcpy( &parms, &available_features, sizeof( VkPhysicalDeviceFeatures ) );

	parms.robustBufferAccess = true;
	parms.fullDrawIndexUint32 = true;
	parms.imageCubeArray = true;
	parms.independentBlend = false;
	parms.geometryShader = true;
	parms.tessellationShader = false;
	parms.sampleRateShading = true;
	parms.dualSrcBlend = false;
	parms.logicOp = false;
	parms.multiDrawIndirect = false;
	parms.drawIndirectFirstInstance = false;
	parms.depthClamp = false;
	parms.depthBiasClamp = true;
	parms.fillModeNonSolid = true;
	parms.depthBounds = true;
	parms.wideLines = true;
	parms.largePoints = true;
	parms.alphaToOne = false;
	parms.multiViewport = false;
	parms.samplerAnisotropy = true;
	parms.textureCompressionETC2 = false;
	parms.textureCompressionASTC_LDR = false;
	parms.textureCompressionBC = true;
	parms.occlusionQueryPrecise = true; // count of passed samples instead of boolean
	parms.pipelineStatisticsQuery = false;
	parms.vertexPipelineStoresAndAtomics = false;
	parms.fragmentStoresAndAtomics = false;
	parms.shaderTessellationAndGeometryPointSize = false;
	parms.shaderImageGatherExtended = true;
	parms.shaderStorageImageExtendedFormats = false;
	parms.shaderStorageImageMultisample = false;
	parms.shaderStorageImageReadWithoutFormat = false;
	parms.shaderStorageImageWriteWithoutFormat = false;
	parms.shaderUniformBufferArrayDynamicIndexing = false;
	parms.shaderSampledImageArrayDynamicIndexing = false;
	parms.shaderStorageBufferArrayDynamicIndexing = false;
	parms.shaderStorageImageArrayDynamicIndexing = false;
	parms.shaderClipDistance = true;
	parms.shaderCullDistance = true;
	parms.shaderFloat64 = false;
	parms.shaderInt64 = false;
	parms.shaderInt16 = true;
	parms.shaderResourceResidency = false;
	parms.shaderResourceMinLod = true;

	parms.sparseBinding = false;
	parms.sparseResidencyBuffer = false;
	parms.sparseResidencyImage2D = false;
	parms.sparseResidencyImage3D = false;
	parms.sparseResidency2Samples = false;
	parms.sparseResidency4Samples = false;
	parms.sparseResidency8Samples = false;
	parms.sparseResidency16Samples = false;
	parms.sparseResidencyAliased = false;

	parms.variableMultisampleRate = true;
	parms.inheritedQueries = true;
}

/*
========================================
 CreateDeviceContext
========================================
*/
vkDeviceContext_t * idVulkanInterface::CreateDeviceContext( uint32_t gpuIndex, bool bPresentable )
{
	VkDeviceCreateInfo deviceInfo = {};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	auto pd = GetGPU( gpuIndex );

	idStaticList< const char *, 64> requested_extensions;
	GetRequestedExtensions( pd, requested_extensions, bPresentable );
	deviceInfo.enabledExtensionCount = requested_extensions.Num();
	deviceInfo.ppEnabledExtensionNames = requested_extensions.Ptr();

	idStaticList< const char *, 16> requested_layers;
	GetRequestedLayers( pd, requested_layers );
	deviceInfo.enabledLayerCount = requested_layers.Num();
	deviceInfo.ppEnabledLayerNames = requested_layers.Ptr();

	VkPhysicalDeviceFeatures features;
	GetRequestedFeatures( pd, features );
	deviceInfo.pEnabledFeatures = &features;

	// Setup Queue info ------------------------------------------

	const bool bAllowPresentOnComputeQueue = r_vk_allowPresentOnComputeQueue.GetBool();
	const bool bAllowAsyncCompute = r_vk_allowAsyncCompute.GetBool();

	idList<VkDeviceQueueCreateInfo> requested_deviceQueueInfos( 1 );

	int32 GraphicsQueueFamilyIndex = -1;
	int32 ComputeQueueFamilyIndex = -1;
	int32 TransferQueueFamilyIndex = -1;

	uint32 NumPriorities = 0;
	for( int32 FamilyIndex = 0; FamilyIndex < pd->m_queueFamilyProps.Num(); ++FamilyIndex )
	{
		const auto & CurrProps = pd->m_queueFamilyProps[ FamilyIndex ];

		bool bIsValidQueue = false;
		if( ( CurrProps.queueFlags & VK_QUEUE_GRAPHICS_BIT ) == VK_QUEUE_GRAPHICS_BIT )
		{
			if( GraphicsQueueFamilyIndex == -1 )
			{
				GraphicsQueueFamilyIndex = FamilyIndex;
				bIsValidQueue = true;
			}
		}

		if( ( CurrProps.queueFlags & VK_QUEUE_COMPUTE_BIT ) == VK_QUEUE_COMPUTE_BIT )
		{
			if( ComputeQueueFamilyIndex == -1 && ( bAllowAsyncCompute || bAllowPresentOnComputeQueue ) && GraphicsQueueFamilyIndex != FamilyIndex )
			{
				ComputeQueueFamilyIndex = FamilyIndex;
				bIsValidQueue = true;
			}
		}

		if( ( CurrProps.queueFlags & VK_QUEUE_TRANSFER_BIT ) == VK_QUEUE_TRANSFER_BIT )
		{
			// Prefer a non-gfx transfer queue
			if( TransferQueueFamilyIndex == -1 && ( CurrProps.queueFlags & VK_QUEUE_GRAPHICS_BIT ) != VK_QUEUE_GRAPHICS_BIT && ( CurrProps.queueFlags & VK_QUEUE_COMPUTE_BIT ) != VK_QUEUE_COMPUTE_BIT )
			{
				TransferQueueFamilyIndex = FamilyIndex;
				bIsValidQueue = true;
			}
		}

		auto GetQueueInfoString = []( const VkQueueFamilyProperties& Props, idStr & Info )
		{
			if( ( Props.queueFlags & VK_QUEUE_GRAPHICS_BIT ) == VK_QUEUE_GRAPHICS_BIT )
			{
				Info += " GRAPHICS";
			}
			if( ( Props.queueFlags & VK_QUEUE_COMPUTE_BIT ) == VK_QUEUE_COMPUTE_BIT )
			{
				Info += " COMPUTE";
			}
			if( ( Props.queueFlags & VK_QUEUE_TRANSFER_BIT ) == VK_QUEUE_TRANSFER_BIT )
			{
				Info += " TRANSFER";
			}
			if( ( Props.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT ) == VK_QUEUE_SPARSE_BINDING_BIT )
			{
				Info += " SPARSE_BINDING";
			}
		};

		if( !bIsValidQueue )
		{
			idStr info;
			GetQueueInfoString( CurrProps, info );
			common->DPrintf( "Skipping unnecessary Queue Family %d: %d queues:%s", FamilyIndex, CurrProps.queueCount, info.c_str() );
			continue;
		}

		auto & CurrQueue = requested_deviceQueueInfos.Alloc();
		CurrQueue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		CurrQueue.queueFamilyIndex = FamilyIndex;
		CurrQueue.queueCount = CurrProps.queueCount;
		NumPriorities += CurrProps.queueCount;

		idStr info;
		GetQueueInfoString( CurrProps, info );
		common->DPrintf( "Initializing Queue Family %d: %d queues%s", FamilyIndex, CurrProps.queueCount, info.c_str() );
	}

	idList<float> QueuePriorities;
	QueuePriorities.SetNum( NumPriorities );
	float* CurrentPriority = QueuePriorities.Ptr();
	for( int32 Index = 0; Index < requested_deviceQueueInfos.Num(); ++Index )
	{
		VkDeviceQueueCreateInfo & CurrQueue = requested_deviceQueueInfos[ Index ];
		CurrQueue.pQueuePriorities = CurrentPriority;

		const VkQueueFamilyProperties & CurrProps = pd->m_queueFamilyProps[ CurrQueue.queueFamilyIndex ];
		for( int32 QueueIndex = 0; QueueIndex < ( int32 )CurrProps.queueCount; ++QueueIndex )
		{
			*CurrentPriority++ = 1.0f;
		}
	}

	deviceInfo.queueCreateInfoCount = requested_deviceQueueInfos.Num();
	deviceInfo.pQueueCreateInfos = requested_deviceQueueInfos.Ptr();

	// -----------------------------------------------------------

	VkDevice device;
	ID_VK_CHECK( vkCreateDevice( pd->GetHandle(), &deviceInfo, NULL, &device ) );
	auto dc = new ( TAG_RENDER ) vkDeviceContext_t( device, pd );

	// -----------------------------------------------------------

	// Create Graphics Queue, here we submit command buffers for execution
	dc->m_graphicsQueue = new vkQueue_t( dc, GraphicsQueueFamilyIndex, 0 );
	if( ComputeQueueFamilyIndex == -1 )
	{
		// If we didn't find a dedicated Queue, use the default one
		ComputeQueueFamilyIndex = GraphicsQueueFamilyIndex;
	}
	dc->m_computeQueue = new vkQueue_t( dc, ComputeQueueFamilyIndex, 0 );
	if( TransferQueueFamilyIndex == -1 )
	{
		// If we didn't find a dedicated Queue, use the default one
		TransferQueueFamilyIndex = ComputeQueueFamilyIndex;
	}
	dc->m_transferQueue = new vkQueue_t( dc, TransferQueueFamilyIndex, 0 );




#ifdef _DEBUG
	if( vkSys.EXT_debug_utils )
	{


	}
	else if( vkSys.EXT_debug_marker )
	{
		dc->vkDebugMarkerSetObjectTagEXT = ( PFN_vkDebugMarkerSetObjectTagEXT )vkGetDeviceProcAddr( device, "vkDebugMarkerSetObjectNameEXT" );
		dc->vkDebugMarkerSetObjectNameEXT = ( PFN_vkDebugMarkerSetObjectNameEXT )vkGetDeviceProcAddr( device, "vkDebugMarkerSetObjectNameEXT" );
		dc->vkCmdDebugMarkerBeginEXT = ( PFN_vkCmdDebugMarkerBeginEXT )vkGetDeviceProcAddr( device, "vkCmdDebugMarkerBeginEXT" );
		dc->vkCmdDebugMarkerEndEXT = ( PFN_vkCmdDebugMarkerEndEXT )vkGetDeviceProcAddr( device, "vkCmdDebugMarkerEndEXT" );
	}
#endif

	m_deviceContexts.Append( dc );
	return dc;
}

/*
========================================
 DestroyDeviceContext
========================================
*/
void idVulkanInterface::DestroyDeviceContext( vkDeviceContext_t * dc )
{
	m_deviceContexts.Remove( dc );
	delete dc;
}


static void GetPhysicalDeviceSurfaceCaps( VkInstance instance, VkSurfaceKHR surface, vkPhysicalDevice_t * gpu )
{
	{
		GET_IPROC_LOCAL( vkGetPhysicalDeviceSurfaceFormatsKHR );

		uint32 numFormats;
		ID_VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR( gpu->GetHandle(), surface, &numFormats, NULL ) );
		ID_VK_VALIDATE( numFormats > 0, "vkGetPhysicalDeviceSurfaceFormatsKHR returned zero surface formats." );

		gpu->m_surfaceFormats.SetNum( numFormats );
		ID_VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR( gpu->GetHandle(), surface, &numFormats, gpu->m_surfaceFormats.Ptr() ) );
	}

	{
		GET_IPROC_LOCAL( vkGetPhysicalDeviceSurfaceCapabilitiesKHR );

		ID_VK_CHECK( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( gpu->GetHandle(), surface, &gpu->m_surfaceCaps ) );
	}

	{
		GET_IPROC_LOCAL( vkGetPhysicalDeviceSurfacePresentModesKHR );

		uint32 numPresentModes;
		ID_VK_CHECK( vkGetPhysicalDeviceSurfacePresentModesKHR( gpu->GetHandle(), surface, &numPresentModes, NULL ) );
		ID_VK_VALIDATE( numPresentModes > 0, "vkGetPhysicalDeviceSurfacePresentModesKHR returned zero present modes." );

		gpu->m_presentModes.SetNum( numPresentModes );
		ID_VK_CHECK( vkGetPhysicalDeviceSurfacePresentModesKHR( gpu->GetHandle(), surface, &numPresentModes, gpu->m_presentModes.Ptr() ) );
	}
}

// Create a WSI surface for the window:
static VkSurfaceKHR CreateRenderSurface( VkInstance instance, const window_t & window )
{
	VkSurfaceKHR surface;

	// Construct the surface description
#if defined( VK_USE_PLATFORM_WIN32_KHR )
	GET_IPROC_LOCAL( vkCreateWin32SurfaceKHR );
	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hinstance = ( HINSTANCE )::GetModuleHandle( NULL );
	createInfo.hwnd = ( HWND )window.handle;
	ID_VK_CHECK( vkCreateWin32SurfaceKHR( instance, &createInfo, nullptr, &surface ) );

#elif defined( VK_USE_PLATFORM_ANDROID_KHR )
	GET_IPROC_LOCAL( vkCreateAndroidSurfaceKHR );
	VkAndroidSurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	createInfo.window = window.handle; //AndroidGetApplicationWindow();
	ID_VK_CHECK( vkCreateAndroidSurfaceKHR( instance, &createInfo, nullptr, &surface ) );

#elif defined( VK_USE_PLATFORM_WAYLAND_KHR )
	GET_IPROC_LOCAL( vkCreateWaylandSurfaceKHR );
	VkWaylandSurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
	createInfo.display = display;
	createInfo.surface = window;
	ID_VK_CHECK( vkCreateWaylandSurfaceKHR( instance, &createInfo, nullptr, &surface ) );

#elif defined( VK_USE_PLATFORM_XLIB_KHR )
	GET_IPROC_LOCAL( vkCreateXlibSurfaceKHR );
	VkXlibSurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
	createInfo.dpy = Window.DisplayPtr;
	createInfo.window = Window.Handle;
	ID_VK_CHECK( vkCreateXlibSurfaceKHR( instance, &createInfo, nullptr, &surface ) );

#elif defined( VK_USE_PLATFORM_XCB_KHR )
	GET_IPROC_LOCAL( vkCreateXcbSurfaceKHR );
	VkXcbSurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	createInfo.connection = connection;
	createInfo.window = window;
	ID_VK_CHECK( vkCreateXcbSurfaceKHR( instance, &createInfo, nullptr, &surface ) );

#elif defined( VK_USE_PLATFORM_IOS_MVK )
	GET_IPROC_LOCAL( vkCreateIOSSurfaceMVK );
	VkIOSSurfaceCreateInfoMVK createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK;
	createInfo.pView = window.handle;
	ID_VK_CHECK( vkCreateIOSSurfaceMVK( instance, &createInfo, nullptr, &surface ) );

#elif defined( VK_USE_PLATFORM_MACOS_MVK )
	GET_IPROC_LOCAL( vkCreateMacOSSurfaceMVK );
	VkMacOSSurfaceCreateInfoMVK createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
	createInfo.pView = window.handle;
	ID_VK_CHECK( vkCreateMacOSSurfaceMVK( instance, &createInfo, nullptr, &surface ) );
#endif

	return surface;
}

static window_t CreateRenderWindow()
{
/*
#if defined(VK_USE_PLATFORM_WIN32_KHR)

#elif defined(VK_USE_PLATFORM_XLIB_KHR)
	Display *display;
	Window xlib_window;
	Atom xlib_wm_delete_window;

#elif defined(VK_USE_PLATFORM_XCB_KHR)
	Display *display;
	xcb_connection_t *connection;
	xcb_screen_t *screen;
	xcb_window_t xcb_window;
	xcb_intern_atom_reply_t *atom_wm_delete_window;

#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct wl_surface *window;
	struct wl_shell *shell;
	struct wl_shell_surface *shell_surface;
	struct wl_seat *seat;
	struct wl_pointer *pointer;
	struct wl_keyboard *keyboard;

#elif defined(VK_USE_PLATFORM_MIR_KHR)
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
	struct ANativeWindow *window;
#elif (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
	void *window;
#endif
*/
	window_t wnd;
	wnd.handle = win32.hWnd;
	return wnd;
}

static void SetupPresentQueue( vkDeviceContext_t * dc, VkSurfaceKHR Surface )
{
	struct Local_t {
		VkSurfaceKHR Surface;
		PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR = nullptr;
		Local_t( VkSurfaceKHR _Surface ) : Surface( _Surface ) {
			vkGetPhysicalDeviceSurfaceSupportKHR = ( PFN_vkGetPhysicalDeviceSurfaceSupportKHR )vkGetInstanceProcAddr( vkSys.GetInstance(), "vkGetPhysicalDeviceSurfaceSupportKHR" );
			ID_VK_VALIDATE( vkGetPhysicalDeviceSurfaceSupportKHR != VK_NULL_HANDLE, "vkGetPhysicalDeviceSurfaceSupportKHR == NULL" )
		}
		auto SupportsPresent( VkPhysicalDevice PhysicalDevice, vkQueue_t* Queue ) const
		{
			VkBool32 bSupportsPresent = VK_FALSE;
			const uint32 FamilyIndex = Queue->GetFamilyIndex();
			ID_VK_CHECK( vkGetPhysicalDeviceSurfaceSupportKHR( PhysicalDevice, FamilyIndex, Surface, &bSupportsPresent ) );
			if( bSupportsPresent ) {
				common->DPrintf( "Queue Family %d: Supports Present", FamilyIndex );
			}
			return( bSupportsPresent == VK_TRUE );
		}
	};

	if( dc->m_presentQueue )
		return;

	Local_t local( Surface );
	bool bGfx = local.SupportsPresent( dc->GetPDHandle(), dc->m_graphicsQueue );
	ID_VK_VALIDATE( bGfx, "Graphics Queue doesn't support present!" );

	bool bCompute = local.SupportsPresent( dc->GetPDHandle(), dc->m_computeQueue );
	if( dc->m_transferQueue->GetFamilyIndex() != dc->m_graphicsQueue->GetFamilyIndex() &&
		dc->m_transferQueue->GetFamilyIndex() != dc->m_computeQueue->GetFamilyIndex() )
	{
		local.SupportsPresent( dc->GetPDHandle(), dc->m_transferQueue );
	}

	if( dc->m_computeQueue->GetFamilyIndex() != dc->m_graphicsQueue->GetFamilyIndex() && bCompute )
	{
		dc->m_presentQueue = dc->m_computeQueue;
	}
	else {
		dc->m_presentQueue = dc->m_graphicsQueue;
	}
}

static vkSwapChain_t * CreateSwapChain( VkInstance inst, window_t & wnd, vkDeviceContext_t * dc, uint32 & width, uint32 & height, vkSwapChain_t * oldSwapChain )
{
	GET_IPROC_LOCAL( vkGetPhysicalDeviceSurfaceSupportKHR );
	GET_IPROC_LOCAL( vkGetPhysicalDeviceSurfaceCapabilitiesKHR );
	GET_IPROC_LOCAL( vkGetPhysicalDeviceSurfaceFormatsKHR );
	GET_IPROC_LOCAL( vkGetPhysicalDeviceSurfacePresentModesKHR );

	GET_IPROC_LOCAL( vkCreateSwapchainKHR );
	GET_IPROC_LOCAL( vkDestroySwapchainKHR );
	GET_IPROC_LOCAL( vkGetSwapchainImagesKHR );
	GET_IPROC_LOCAL( vkAcquireNextImageKHR );
	GET_IPROC_LOCAL( vkQueuePresentKHR );


	auto surf = CreateRenderSurface( inst, wnd );

#if 0
	auto & queueProperties = dc->GetPD()->m_queueFamilyProps;
	uint32_t queueCount = dc->GetPD()->m_queueFamilyProps.Num();

	// The problem is not all queues support presenting. Here
	// we make use of vkGetPhysicalDeviceSurfaceSupportKHR to find queues
	// with present support.
	idList<VkBool32> supportsPresent;
	supportsPresent.SetNum( queueCount );
	for( uint32_t i = 0; i < queueCount; i++ )
	{
		vkGetPhysicalDeviceSurfaceSupportKHR( dc->GetPhysicalHandle(), i, surf, &supportsPresent[ i ] );
	}
	// Now we have a list of booleans for which queues support presenting.
	// We now must walk the queue to find one which supports
	// VK_QUEUE_GRAPHICS_BIT and has supportsPresent[index] == VK_TRUE
	// (indicating it supports both.)
	uint32_t graphicIndex = UINT32_MAX;
	uint32_t presentIndex = UINT32_MAX;
	for( uint32_t i = 0; i < queueCount; i++ )
	{
		if( ( queueProperties[ i ].queueFlags & VK_QUEUE_GRAPHICS_BIT ) )
		{
			if( graphicIndex == UINT32_MAX )
				graphicIndex = i;

			if( supportsPresent[ i ] == VK_TRUE )
			{
				graphicIndex = i;
				presentIndex = i;
				break;
			}
		}
	}
	if( presentIndex == UINT32_MAX )
	{
		// If there is no queue that supports both present and graphics;
		// try and find a separate present queue. They don't necessarily
		// need to have the same index.
		for( uint32_t i = 0; i < queueCount; i++ )
		{
			if( supportsPresent[ i ] != VK_TRUE )
				continue;

			presentIndex = i;
			break;
		}
	}
	// If neither a graphics or presenting queue was found then we cannot
	// render
	if( graphicIndex == UINT32_MAX || presentIndex == UINT32_MAX )
		return nullptr;

	// In a future tutorial we'll look at supporting a separate graphics
	// and presenting queue
	if( graphicIndex != presentIndex )
	{
		fprintf( stderr, "Not supported\n" );
		return nullptr;
	}
#endif
	SetupPresentQueue( dc, surf );

	// --------------------------------------------------------------

	// Now get a list of supported surface formats
	idList<VkSurfaceFormatKHR> surfaceFormats;

	uint32_t formatCount;
	ID_VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR( dc->GetPDHandle(), surf, &formatCount, NULL ) );
	ID_VK_VALIDATE( formatCount != 0, "vkGetPhysicalDeviceSurfaceFormatsKHR returned 0 formatCount" );

	surfaceFormats.SetNum( formatCount );
	ID_VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR( dc->GetPDHandle(), surf, &formatCount, surfaceFormats.Ptr() ) );

	VkSurfaceFormatKHR fmtInfo;

	// If the format list includes just one entry of VK_FORMAT_UNDEFINED,
	// the surface has no preferred format. Otherwise, at least one
	// supported format will be returned
	if( formatCount == 1 && surfaceFormats[ 0 ].format == VK_FORMAT_UNDEFINED )
	{
		fmtInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
		fmtInfo.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	}
	// Check if list contains most widely used R8 G8 B8 A8 format
	// with nonlinear color space
	/*for( VkSurfaceFormatKHR & fmt : surface_formats )
	{
		if( fmt.format == VK_FORMAT_R8G8B8A8_UNORM )
		{
			return fmt;
		}
	}*/
	else {
		fmtInfo.format = surfaceFormats[ 0 ].format;
		fmtInfo.colorSpace = surfaceFormats[ 0 ].colorSpace;
	}


	/*bool bFound = false;
	VkFormat Requested;
	VkSurfaceFormatKHR CurrFormat;
	for( int32 Index = 0; Index < surfaceFormats.Num(); ++Index )
	{
		if( surfaceFormats[ Index ].format == Requested )
		{
			CurrFormat = surfaceFormats[ Index ];
			bFound = true;
			break;
		}
	}
	if( !bFound ) {
		common->Warning( "Requested PixelFormat %d not supported by this swapchain! Falling back to supported swapchain formats...", ( uint32 )Requested );
		Requested = VK_FORMAT_UNDEFINED;
	}*/

	// Create our swap chain data structure.
	vkSwapChain_t swapChain;
	swapChain.images.SetGranularity( 1 );
	swapChain.buffers.SetGranularity( 1 );

	swapChain.presentQueueFamilyIndex = dc->GetPresentQueue()->GetFamilyIndex(); //graphicIndex;
	swapChain.fmtInfo = fmtInfo;

	///////////////////////////////////////////////////////////////

	// Get physical device surface properties and formats.
	// We will be using the result of this to determine the number of
	// images we should use for our swap chain and set the appropriate
	// sizes for them.
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	ID_VK_CHECK( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( dc->GetPDHandle(), surf, &surfaceCapabilities ) );

	// Get the available modes for presentation.
	uint32_t presentModeCount;
	ID_VK_CHECK( vkGetPhysicalDeviceSurfacePresentModesKHR( dc->GetPDHandle(), surf, &presentModeCount, NULL ) );
	ID_VK_VALIDATE( presentModeCount > 0, "vkGetPhysicalDeviceSurfacePresentModesKHR returned 0 presentModeCount" )

	idList<VkPresentModeKHR> presentModes; presentModes.SetNum( presentModeCount );
	ID_VK_CHECK( vkGetPhysicalDeviceSurfacePresentModesKHR( dc->GetPDHandle(), surf, &presentModeCount, presentModes.Ptr() ) );

	// When constructing a swap chain we must supply our surface resolution.
	// Like all things in Vulkan there is a structure for representing this:
	swapChain.swapChainExtent = {};

	// The way surface capabilities work is rather confusing but width
	// and height are either both -1 or both not -1. A size of -1 indicates
	// that the surface size is undefined, which means you can set it to
	// effectively any size. If the size however is defined, the swap chain
	// size *MUST* match.
	if( surfaceCapabilities.currentExtent.width == 0xFFFFFFFF )
	{
		swapChain.swapChainExtent.width = idMath::Clamp( surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width, width );
		swapChain.swapChainExtent.height = idMath::Clamp( surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height, height );

		width = swapChain.swapChainExtent.width;
		height = swapChain.swapChainExtent.height;
	}
	else {
		swapChain.swapChainExtent = surfaceCapabilities.currentExtent;
		width = surfaceCapabilities.currentExtent.width;
		height = surfaceCapabilities.currentExtent.height;
	}

	// Acquiring Supported Present Modes.
	// Prefer mailbox mode if present.
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // FIFO is like glSwapInterval = 1
	for( uint32_t i = 0; i < presentModeCount; i++ )
	{
		if( presentModes[ i ] == VK_PRESENT_MODE_MAILBOX_KHR ) // Mailbox/Immediate is like glSwapInterval 0
		{
			presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
		if( presentModes[ i ] == VK_PRESENT_MODE_FIFO_RELAXED_KHR ) //  EXT_swap_control_tear
		{
			presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
			break;
		}

		if( presentMode != VK_PRESENT_MODE_MAILBOX_KHR &&
			presentMode != VK_PRESENT_MODE_FIFO_RELAXED_KHR &&
			presentMode != VK_PRESENT_MODE_IMMEDIATE_KHR )
		{
			// The horrible fallback
			presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
		}
	}

	// Determine the number of images for our swap chain:
	uint32_t desiredImageCount = surfaceCapabilities.minImageCount + 1;
	if( surfaceCapabilities.maxImageCount > 0 && desiredImageCount > surfaceCapabilities.maxImageCount )
	{
		desiredImageCount = surfaceCapabilities.maxImageCount;
	}

	VkSurfaceTransformFlagsKHR preTransform = surfaceCapabilities.currentTransform;
	if( surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR )
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

	// Alpha on the window surface should be opaque:
	// If it was not we could create transparent regions of our window which
	// would require support from the Window compositor.
	VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	if( surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR )
		compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;


	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

	swapChainCreateInfo.surface = surf;
	swapChainCreateInfo.minImageCount = desiredImageCount;
	swapChainCreateInfo.imageFormat = fmtInfo.format;
	swapChainCreateInfo.imageColorSpace = fmtInfo.colorSpace;
	swapChainCreateInfo.imageExtent = { swapChain.swapChainExtent.width, swapChain.swapChainExtent.height };
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // This is literally the same as GL_COLOR_ATTACHMENT0
	// No other flags necessary, we won't sample and clear it!
#if 0
		///swapChainCreateInfo.imageUsage |= ( VK_IMAGE_USAGE_TRANSFER_DST_BIT /*| VK_IMAGE_USAGE_SAMPLED_BIT*/ );

		// Color attachment flag must always be supported
		// We can define other usage flags but we always need to check if they are supported
		if( surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT )
		{
			// define additional “transfer destination” usage which is required for image clear operation.
			swapChainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		common->Warning( "VK_IMAGE_USAGE_TRANSFER_DST image usage is not supported by the swap chain!" );
		common->DPrintf( "Supported swap chain's image usages include:\n" );
		if( surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT )
			common->DPrintf( "\tVK_IMAGE_USAGE_TRANSFER_SRC\n" );
		if( surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT )
			common->DPrintf( "\tVK_IMAGE_USAGE_TRANSFER_DST\n" );
		if( surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT )
			common->DPrintf( "\tVK_IMAGE_USAGE_SAMPLED\n" );
		if( surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT )
			common->DPrintf( "\tVK_IMAGE_USAGE_STORAGE\n" );
		if( surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT )
			common->DPrintf( "\tVK_IMAGE_USAGE_COLOR_ATTACHMENT\n" );
		if( surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT )
			common->DPrintf( "\tVK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT\n" );
		if( surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT )
			common->DPrintf( "\tVK_IMAGE_USAGE_TRANSIENT_ATTACHMENT\n" );
		if( surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT )
			common->DPrintf( "\tVK_IMAGE_USAGE_INPUT_ATTACHMENT\n" );
#endif
	swapChainCreateInfo.preTransform = ( VkSurfaceTransformFlagBitsKHR )preTransform;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapChainCreateInfo.presentMode = presentMode;
	swapChainCreateInfo.clipped = VK_TRUE; // If we want clipping outside the extents.
	swapChainCreateInfo.compositeAlpha = compositeAlpha;

	///swapChainCreateInfo.queueFamilyIndexCount = 0;
	///swapChainCreateInfo.pQueueFamilyIndices = NULL;
	if( oldSwapChain ) {
		swapChainCreateInfo.oldSwapchain = oldSwapChain->GetHandle();
		// delete oldSwapChain?
	}

	ID_VK_CHECK( vkCreateSwapchainKHR( dc->GetHandle(), &swapChainCreateInfo, NULL, &swapChain.swapChainHandle ) );

	// ---------------------------------------------------

	// Now get the presentable images from the swap chain
	uint32_t imageCount;
	if( vkGetSwapchainImagesKHR( dc->GetHandle(), swapChain.GetHandle(), &imageCount, NULL ) != VK_SUCCESS )
		goto failed;

	swapChain.images.SetNum( imageCount );
	if( vkGetSwapchainImagesKHR( dc->GetHandle(), swapChain.GetHandle(), &imageCount, swapChain.images.Ptr() ) != VK_SUCCESS )
	{
failed:
		vkDestroySwapchainKHR( dc->GetHandle(), swapChain.GetHandle(), NULL );
		common->FatalError("Can't create swap chain :(");
		return nullptr;
	}

	// Create the image views for the swap chain. They will all be single
	// layer, 2D images, with no mipmaps.
	// Check the VkImageViewCreateInfo structure to see other views you
	// can potentially create.
	swapChain.buffers.SetNum( imageCount );
	for( uint32_t i = 0; i < imageCount; i++ )
	{
		VkImageViewCreateInfo colorAttachmentView = {};
		colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

		colorAttachmentView.format = swapChain.fmtInfo.format;
		colorAttachmentView.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorAttachmentView.subresourceRange.baseMipLevel = 0;
		colorAttachmentView.subresourceRange.levelCount = 1;
		colorAttachmentView.subresourceRange.baseArrayLayer = 0;
		colorAttachmentView.subresourceRange.layerCount = 1;
		colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorAttachmentView.flags = 0; // mandatory

		// Wire them up
		swapChain.buffers[ i ].image = swapChain.images[ i ];

		// Transform images from the initial (undefined) layer to
		// present layout
		SetImageLayout( commandBuffer,
			swapChain.buffers[ i ].image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR );

		colorAttachmentView.image = swapChain.buffers[ i ].image;


		// Clear the swapchain to avoid a validation warning, and transition to ColorAttachment
		{
			VkImageSubresourceRange range = {};
			range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			range.baseMipLevel = 0;
			range.levelCount = 1;
			range.baseArrayLayer = 0;
			range.layerCount = 1;

			VkClearColorValue clearColor = {};
			ImagePipelineBarrier( commandBuffer, swapChain.images[ i ], imgLayoutBarrier_e::Undefined, imgLayoutBarrier_e::TransferDest, range );
			vkCmdClearColorImage( commandBuffer, swapChain.images[ i ], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range );
			ImagePipelineBarrier( commandBuffer, swapChain.images[ i ], imgLayoutBarrier_e::TransferDest, imgLayoutBarrier_e::ColorAttachment, range );
		}




		// Create the view
		if( vkCreateImageView( dc->GetHandle(), &colorAttachmentView, NULL, &swapChain.buffers[ i ].view ) != VK_SUCCESS )
			goto failed;
	}

	auto sc = new vkSwapChain_t();
	memcpy( sc, &swapChain, sizeof( vkSwapChain_t ) );
	return sc;



	VkAttachmentDescription attachment_desc = {};
	attachment_desc.flags = 0;
	attachment_desc.format = swapChain.fmtInfo.format;
	attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_desc.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachment_desc.finalLayout	= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


/*
#if USE_IMAGE_ACQUIRE_FENCES
	ImageAcquiredFences.AddUninitialized( NumSwapChainImages );
	VulkanRHI::FFenceManager& FenceMgr = Device.GetFenceManager();
	for( uint32 BufferIndex = 0; BufferIndex < NumSwapChainImages; ++BufferIndex )
	{
		ImageAcquiredFences[ BufferIndex ] = Device.GetFenceManager().AllocateFence( true );
	}
#endif
	ImageAcquiredSemaphore.AddUninitialized( DesiredNumBuffers );
	for( uint32 BufferIndex = 0; BufferIndex < DesiredNumBuffers; ++BufferIndex )
	{
		ImageAcquiredSemaphore[ BufferIndex ] = new FVulkanSemaphore( Device );
	}

	#if USE_IMAGE_ACQUIRE_FENCES
	VulkanRHI::FFenceManager& FenceMgr = Device.GetFenceManager();
	for( int32 Index = 0; Index < ImageAcquiredFences.Num(); ++Index )
	{
	FenceMgr.ReleaseFence( ImageAcquiredFences[ Index ] );
	}
	#endif

	//#todo-rco: Enqueue for deletion as we first need to destroy the cmd buffers and queues otherwise validation fails
	for( int BufferIndex = 0; BufferIndex < ImageAcquiredSemaphore.Num(); ++BufferIndex )
	{
	delete ImageAcquiredSemaphore[ BufferIndex ];
	}
*/
	VkSemaphoreCreateInfo semaphore_create_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };

	if( ( vkCreateSemaphore( dc->GetHandle(), &semaphore_create_info, nullptr, &dc->m_imageAvailableSemaphore ) != VK_SUCCESS ) ||
		( vkCreateSemaphore( dc->GetHandle(), &semaphore_create_info, nullptr, &dc->m_renderingFinishedSemaphore ) != VK_SUCCESS ) )
	{
		common->FatalError( "Could not create semaphores!" );
	}

}

struct vkFence_t {
	VkFence fence;
	VkDevice device;
	bool Signaled() const
	{
		auto res = vkGetFenceStatus( device, fence ); // VK_NOT_READY
		assert( res != VK_ERROR_DEVICE_LOST );
		return( res == VK_SUCCESS );
	}
	void Reset()
	{
		vkResetFences( device, 1, &fence );
	}
	vkFence_t( VkDevice master, bool signaled ) : device( master )
	{
		VkFenceCreateInfo info = {
			VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL,
			signaled ? VK_FENCE_CREATE_SIGNALED_BIT : VK_FLAGS_NONE
		};
		vkCreateFence( device, &info, nullptr, &fence );
	}
	~vkFence_t()
	{
		vkDestroyFence( device, fence, nullptr );
	}
};

void vkDeviceContext_t::SubmitWorkToGPU()
{
	// Host access to queue must be externally synchronized.
	const VkQueue executionQueue = IsPresentable() ? m_presentQueue->GetHandle() : m_graphicsQueue->GetHandle();

	// Host access to fence must be externally synchronized.
	///const VkFence fence_allDone = VK_NULL_HANDLE;
	///vkGetFenceStatus( GetHandle(), fence_allDone );

	idList<VkSemaphore> toBeWaitForSemaphores;
	idList<VkPipelineStageFlags> dstStageMasks;

	idList<VkCommandBuffer> commandBuffers;
	const VkSemaphore toBeSignaledSemaphores[ 1 ] = { m_cmdCompleteSemaphores[ m_currentFrameData ] };



	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	// the number of semaphores upon which to wait before
	// executing the command buffers for the batch.
	submitInfo.waitSemaphoreCount = toBeWaitForSemaphores.Num();
	submitInfo.pWaitSemaphores = toBeWaitForSemaphores.Ptr();	// Host access to pSubmits[].pWaitSemaphores[] must be externally synchronized.

	// Pointer to an array of pipeline stages at which each
	// corresponding semaphore wait will occur.
	submitInfo.pWaitDstStageMask = dstStageMasks.Ptr;

	// The number of command buffers to execute in the batch.
	submitInfo.commandBufferCount = commandBuffers.Num();
	submitInfo.pCommandBuffers = commandBuffers.Ptr();

	// The number of semaphores to be signaled once the commands specified
	// in pCommandBuffers have completed execution.
	submitInfo.signalSemaphoreCount = _countof( toBeSignaledSemaphores );
	submitInfo.pSignalSemaphores = toBeSignaledSemaphores;
	// Host access to pSubmits[].pSignalSemaphores[] must be externally synchronized.

	ID_VK_CHECK( vkQueueSubmit( executionQueue,
								1, &submitInfo,
								m_cmdCompleteFences[ m_currentFrameData ] ) );
								// Fence to wait for all command buffers to finish before
								// presenting to the swap chain.
}



VkResult SwapChainAcquireNext( vkSwapChain_t *swapChain, VkSemaphore presentCompleteSemaphore, uint32_t *currentBuffer )
{
	return vkAcquireNextImageKHR( swapChain->m_dc->GetHandle(), swapChain->swapChainHandle, UINT64_MAX, presentCompleteSemaphore, ( VkFence )0, currentBuffer );
/*
	uint32_t image_index;
	VkResult result = vkAcquireNextImageKHR( swapChain->m_dc->GetHandle(), swapChain->swapChainHandle, UINT64_MAX, swapChain->m_imageAvailableSemaphore, VK_NULL_HANDLE, &image_index );
	switch( result )
	{
		case VK_SUCCESS:
		case VK_SUBOPTIMAL_KHR:
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			OnWindowSizeChanged();
			return;
		default:
			std::cout << "Problem occurred during swap chain image acquisition!" << std::endl;
			return;
*/
}

VkResult SwapChainQueuePresent( vkSwapChain_t *swapChain, VkQueue queue, uint32_t currentBuffer )
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = NULL;

	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapChain->swapChainHandle;
	presentInfo.pImageIndices = &currentBuffer;

	return swapChain->fpQueuePresentKHR( queue, &presentInfo );
/*
	VkPresentInfoKHR present_info = {
		VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,           // VkStructureType              sType
		nullptr,                                      // const void                  *pNext
		1,                                            // uint32_t                     waitSemaphoreCount
		&Vulkan.RenderingFinishedSemaphore,           // const VkSemaphore           *pWaitSemaphores
		1,                                            // uint32_t                     swapchainCount
		&Vulkan.SwapChain,                            // const VkSwapchainKHR        *pSwapchains
		&image_index,                                 // const uint32_t              *pImageIndices
		nullptr                                       // VkResult                    *pResults
	};
	auto result = vkQueuePresentKHR( Vulkan.PresentQueue, &present_info );
	switch( result )
	{
		case VK_SUCCESS:
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
		case VK_SUBOPTIMAL_KHR:
			return OnWindowSizeChanged();
		default:
			std::cout << "Problem occurred during image presentation!" << std::endl;
			return false;
	}
*/


/*
	VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	VkSubmitInfo submit_info = {
		VK_STRUCTURE_TYPE_SUBMIT_INFO,                // VkStructureType              sType
		nullptr,                                      // const void                  *pNext
		1,                                            // uint32_t                     waitSemaphoreCount
		&Vulkan.ImageAvailableSemaphore,              // const VkSemaphore           *pWaitSemaphores
		&wait_dst_stage_mask,                         // const VkPipelineStageFlags  *pWaitDstStageMask;
		1,                                            // uint32_t                     commandBufferCount
		&Vulkan.PresentQueueCmdBuffers[ image_index ],  // const VkCommandBuffer       *pCommandBuffers
		1,                                            // uint32_t                     signalSemaphoreCount
		&Vulkan.RenderingFinishedSemaphore            // const VkSemaphore           *pSignalSemaphores
	};

	if( vkQueueSubmit( Vulkan.PresentQueue, 1, &submit_info, VK_NULL_HANDLE ) != VK_SUCCESS )
	{
		return false;
	}
*/
}

void vkDeviceContext_t::Present()
{
	// Wait for fence to signal that all command buffers are ready
	ID_VK_CHECK( vkWaitForFences( GetHandle(), 1, &m_cmdCompleteFences[ m_currentFrameData ], VK_TRUE, UINT64_MAX ) );
	ID_VK_CHECK( vkResetFences( GetHandle(), 1, &m_cmdCompleteFences[ m_currentFrameData ] ) );
	m_commandBufferRecorded[ m_currentFrameData ] = false;

	if( m_swapChain && IsPresentable() )
	{
		const VkSwapchainKHR swapChainHandles[ 1 ] = { m_swapChain->GetHandle() };
		const uint32 imgIndexes[ 1 ] = { m_swapChain->GetLastAcquiredImageIndex() };

		const VkPresentInfoKHR presentInfo = {
			VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, nullptr,

			// specifies the semaphores to wait for before issuing the present request
			1, &m_cmdCompleteSemaphores[ m_currentFrameData ],

			_countof( swapChainHandles ), swapChainHandles, imgIndexes,

			VK_NULL_HANDLE
		};
		ID_VK_CHECK( vkQueuePresentKHR( GetPresentQueue()->GetHandle(), &presentInfo ) );
	}
	// else
	// copy framebuffer output to presentable context

	m_counter++;
	m_currentFrameData = m_counter % NUM_FRAME_DATA;
}






/*
==================================================================
 Init
==================================================================
*/
void idVulkanInterface::Init()
{
/*
#if defined( VK_USE_PLATFORM_WIN32_KHR )
	VulkanLibrary = ::LoadLibrary( "vulkan-1.dll" );
#elif defined( VK_USE_PLATFORM_XCB_KHR ) || defined( VK_USE_PLATFORM_XLIB_KHR )
	VulkanLibrary = ::dlopen( "libvulkan.so", RTLD_NOW );
#endif
	ID_VK_VALIDATE( VulkanLibrary != nullptr, "Could not load Vulkan library!" )
*/
	CreateInstance();
	EnumeratePhysicalDevices( m_gpus );

	auto igpu = ChooseRenderDevice( m_gpus );

	auto dc = CreateDeviceContext( igpu, true );

	auto wnd = CreateRenderWindow();
	uint32 width = renderSystem->GetWidth();
	uint32 height = renderSystem->GetHeight();
	auto swapChain = CreateSwapChain( GetInstance(), wnd, dc, width, height, nullptr );
	ID_VK_VALIDATE( swapChain != nullptr, "CreateSwapChain Fail" );

}

/*
==================================================================
 Shutdown
==================================================================
*/
void idVulkanInterface::Shutdown()
{
	m_deviceContexts.DeleteContents( true );
	m_gpus.DeleteContents( true );

	if( GetInstance() != VK_NULL_HANDLE )
	{
	#ifdef _DEBUG
		if( EXT_debug_utils )
		{
			GET_IPROC_LOCAL( vkDestroyDebugUtilsMessengerEXT );
			vkDestroyDebugUtilsMessengerEXT( GetInstance(), m_dbgMessenger, NULL );
			vkDestroyDebugUtilsMessengerEXT = nullptr;
		}
		else if( EXT_debug_report )
		{
			GET_IPROC_LOCAL( vkDestroyDebugReportCallbackEXT );
			vkDestroyDebugReportCallbackEXT( GetInstance(), m_dbgReportCallback, NULL );
			vkDestroyDebugReportCallbackEXT = nullptr;
		}
	#endif
		vkDestroyInstance( GetInstance(), nullptr );
		m_instance = VK_NULL_HANDLE;
	}

/*
	if( VulkanLibrary )
	{
	#if defined( VK_USE_PLATFORM_WIN32_KHR )
		::FreeLibrary( VulkanLibrary );
	#elif defined( VK_USE_PLATFORM_XCB_KHR ) || defined( VK_USE_PLATFORM_XLIB_KHR )
		::dlclose( VulkanLibrary );
	#endif
	}
*/
}


