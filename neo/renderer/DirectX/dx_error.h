#pragma once

#include <Winerror.h>

const char * WinErrStr( int ErrorCode )
{
	switch( ErrorCode )
	{
		//case S_FALSE:
		//case S_OK:

		case E_FAIL:
			return "Unspecified failure.";
		case E_INVALIDARG:
			return "One or more arguments are not valid.";
		case E_OUTOFMEMORY:
			return "Failed to allocate necessary memory";
		case E_NOTIMPL:
			return "Not implemented.";
		case E_POINTER:
			return "Pointer that is not valid.";
		case E_UNEXPECTED:
			return "Unexpected failure.";
		case E_NOINTERFACE:
			return "No such interface supported.";
		case E_HANDLE:
			return "Handle that is not valid.";
		case E_ACCESSDENIED:
			return "General access denied error.";
		case E_ABORT:
			return "Operation aborted.";


		case UI_E_CREATE_FAILED:
			return "UI_E_CREATE_FAILED:\nThe object could not be created.";

		case UI_E_SHUTDOWN_CALLED:
			return "UI_E_SHUTDOWN_CALLED:\nShutdown was already called on this object or the object that owns it.";

		case UI_E_ILLEGAL_REENTRANCY:
			return "UI_E_ILLEGAL_REENTRANCY:\nThis method cannot be called during this type of callback.";

		case UI_E_OBJECT_SEALED:
			return "UI_E_OBJECT_SEALED:\nThis object has been sealed, so this change is no longer allowed.";

		case UI_E_VALUE_NOT_SET:
			return "UI_E_VALUE_NOT_SET:\nThe requested value was never set.";

		case UI_E_VALUE_NOT_DETERMINED:
			return "UI_E_VALUE_NOT_DETERMINED:\nThe requested value cannot be determined.";

		case UI_E_INVALID_OUTPUT:
			return "UI_E_INVALID_OUTPUT:\nA callback returned an invalid output parameter.";

		case UI_E_BOOLEAN_EXPECTED:
			return "UI_E_BOOLEAN_EXPECTED:\nA callback returned a success code other than S_OK or S_FALSE.";

		case UI_E_DIFFERENT_OWNER:
			return "UI_E_DIFFERENT_OWNER:\nA parameter that should be owned by this object is owned by a different object.";

		case UI_E_AMBIGUOUS_MATCH:
			return "UI_E_AMBIGUOUS_MATCH:\nMore than one item matched the search criteria.";

		case UI_E_FP_OVERFLOW:
			return "UI_E_FP_OVERFLOW:\nA floating - point overflow occurred.";

		case UI_E_WRONG_THREAD:
			return "UI_E_WRONG_THREAD:\nThis method can only be called from the thread that created the object.";

		case UI_E_STORYBOARD_ACTIVE:
			return "UI_E_STORYBOARD_ACTIVE:\nThe storyboard is currently in the schedule.";

		case UI_E_STORYBOARD_NOT_PLAYING:
			return "UI_E_STORYBOARD_NOT_PLAYING:\nThe storyboard is not playing.";

		case UI_E_START_KEYFRAME_AFTER_END:
			return "UI_E_START_KEYFRAME_AFTER_END:\nThe start keyframe might occur after the end keyframe.";

		case UI_E_END_KEYFRAME_NOT_DETERMINED:
			return "UI_E_END_KEYFRAME_NOT_DETERMINED:\nIt might not be possible to determine the end keyframe time when the start keyframe is reached.";

		case UI_E_LOOPS_OVERLAP:
			return "UI_E_LOOPS_OVERLAP:\nTwo repeated portions of a storyboard might overlap.";

		case UI_E_TRANSITION_ALREADY_USED:
			return "UI_E_TRANSITION_ALREADY_USED:\nThe transition has already been added to a storyboard.";

		case UI_E_TRANSITION_NOT_IN_STORYBOARD:
			return "UI_E_TRANSITION_NOT_IN_STORYBOARD:\nThe transition has not been added to a storyboard.";

		case UI_E_TRANSITION_ECLIPSED:
			return "UI_E_TRANSITION_ECLIPSED:\nThe transition might eclipse the beginning of another transition in the storyboard.";

		case UI_E_TIME_BEFORE_LAST_UPDATE:
			return "UI_E_TIME_BEFORE_LAST_UPDATE:\nThe given time is earlier than the time passed to the last update.";

		case UI_E_TIMER_CLIENT_ALREADY_CONNECTED:
			return "UI_E_TIMER_CLIENT_ALREADY_CONNECTED:\nThis client is already connected to a timer.";

		case UI_E_INVALID_DIMENSION:
			return "UI_E_INVALID_DIMENSION:\nThe passed dimension is invalid or does not match the object's dimension.";

		case UI_E_PRIMITIVE_OUT_OF_BOUNDS:
			return "UI_E_PRIMITIVE_OUT_OF_BOUNDS:\nThe added primitive begins at or beyond the duration of the interpolator.";

		case UI_E_WINDOW_CLOSED:
			return "UI_E_WINDOW_CLOSED:\nThe operation cannot be completed because the window is being closed.";

		case E_AUDIO_ENGINE_NODE_NOT_FOUND:
			return "E_AUDIO_ENGINE_NODE_NOT_FOUND:\nPortCls could not find an audio engine node exposed by a miniport driver claiming support for IMiniportAudioEngineNode.";

		case DXGI_STATUS_OCCLUDED:
			return "DXGI_STATUS_OCCLUDED:\nThe Present operation was invisible to the user.";

		case DXGI_STATUS_CLIPPED:
			return "DXGI_STATUS_CLIPPED:\nThe Present operation was partially invisible to the user.";

		case DXGI_STATUS_NO_REDIRECTION:
			return "DXGI_STATUS_NO_REDIRECTION:\nThe driver is requesting that the DXGI runtime not use shared resources to communicate with the Desktop Window Manager.";

		case DXGI_STATUS_NO_DESKTOP_ACCESS:
			return "DXGI_STATUS_NO_DESKTOP_ACCESS:\nThe Present operation was not visible because the Windows session has switched to another desktop( for example, ctrl - alt - del ).";

		case DXGI_STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE:
			return "DXGI_STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE:\nThe Present operation was not visible because the target monitor was being used for some other purpose.";

		case DXGI_STATUS_MODE_CHANGED:
			return "DXGI_STATUS_MODE_CHANGED:\nThe Present operation was not visible because the display mode changed.DXGI will have re-attempted the presentation.";

		case DXGI_STATUS_MODE_CHANGE_IN_PROGRESS:
			return "DXGI_STATUS_MODE_CHANGE_IN_PROGRESS:\nThe Present operation was not visible because another Direct3D device was attempting to take fullscreen mode at the time.";

		case DXGI_ERROR_INVALID_CALL:
			return "DXGI_ERROR_INVALID_CALL:\nThe application made a call that is invalid.Either the parameters of the call or the state of some object was incorrect. Enable the D3D debug layer in order to see details via debug messages.";
			//return "DXGI_ERROR_INVALID_CALL:\nThe application provided invalid parameter data; this must be debugged and fixed before the application is released.";

		case DXGI_ERROR_NOT_FOUND:
			//return "DXGI_ERROR_NOT_FOUND:\nThe object was not found.If calling IDXGIFactory::EnumAdaptes, there is no adapter with the specified ordinal.";
			return "DXGI_ERROR_NOT_FOUND:\nWhen calling IDXGIObject::GetPrivateData, the GUID passed in is not recognized as one previously passed to IDXGIObject::SetPrivateData or IDXGIObject::SetPrivateDataInterface. When calling IDXGIFactory::EnumAdapters or IDXGIAdapter::EnumOutputs, the enumerated ordinal is out of range.";

		case DXGI_ERROR_MORE_DATA:
			//return "DXGI_ERROR_MORE_DATA:\nThe caller did not supply a sufficiently large buffer.";
			return "DXGI_ERROR_MORE_DATA:\nThe buffer supplied by the application is not big enough to hold the requested data.";

		case DXGI_ERROR_UNSUPPORTED:
			return "DXGI_ERROR_UNSUPPORTED:\nThe specified device interface or feature level is not supported on this system.";
			//return "DXGI_ERROR_UNSUPPORTED:\nThe requested functionality is not supported by the device or the driver.";

		case DXGI_ERROR_DEVICE_REMOVED:
			//return "DXGI_ERROR_DEVICE_REMOVED:\nThe GPU device instance has been suspended.Use GetDeviceRemovedReason to determine the appropriate action.";
			return "DXGI_ERROR_DEVICE_REMOVED:\nThe video card has been physically removed from the system, or a driver upgrade for the video card has occurred. The application should destroy and recreate the device. For help debugging the problem, call ID3D10Device::GetDeviceRemovedReason.";

		case DXGI_ERROR_DEVICE_HUNG:
			return "DXGI_ERROR_DEVICE_HUNG:\nThe application's device failed due to badly formed commands sent by the application. This is an design-time issue that should be investigated and fixed.";

		case DXGI_ERROR_DEVICE_RESET:
			//return "DXGI_ERROR_DEVICE_RESET:\nThe GPU will not respond to more commands, most likely because some other application submitted invalid commands. The calling application should re-create the device and continue.";
			return "DXGI_ERROR_DEVICE_RESET:\nThe device failed due to a badly formed command. This is a run-time issue; The application should destroy and recreate the device.";

		case DXGI_ERROR_WAS_STILL_DRAWING:
			//return "DXGI_ERROR_WAS_STILL_DRAWING:\nThe GPU was busy at the moment when the call was made, and the call was neither executed nor scheduled.";
			return "DXGI_ERROR_WAS_STILL_DRAWING:\nThe GPU was busy at the moment when a call was made to perform an operation, and did not execute or schedule the operation.";

		case DXGI_ERROR_FRAME_STATISTICS_DISJOINT:
			return "DXGI_ERROR_FRAME_STATISTICS_DISJOINT:\nAn event( such as power cycle ) interrupted the gathering of presentation statistics. Any previous statistics should be considered invalid.";
			//return "DXGI_ERROR_FRAME_STATISTICS_DISJOINT:\nAn event (for example, a power cycle) interrupted the gathering of presentation statistics.";

		case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE:
			//return "DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE:\nFullscreen mode could not be achieved because the specified output was already in use.";
			return "DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE:\nThe application attempted to acquire exclusive ownership of an output, but failed because some other application (or device within the application) already acquired ownership.";

		case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
			return "DXGI_ERROR_DRIVER_INTERNAL_ERROR:\nAn internal issue prevented the driver from carrying out the specified operation. The driver's state is probably suspect, and the application should not continue.";
			//return "DXGI_ERROR_DRIVER_INTERNAL_ERROR:\nThe driver encountered a problem and was put into the device removed state.";

		case DXGI_ERROR_NAME_ALREADY_EXISTS:
			return "DXGI_ERROR_NAME_ALREADY_EXISTS:\nThe supplied name of a resource in a call to IDXGIResource1::CreateSharedHandle is already associated with some other resource.";

		case DXGI_ERROR_NONEXCLUSIVE:
			return "DXGI_ERROR_NONEXCLUSIVE:\nA global counter resource was in use, and the specified counter cannot be used by this Direct3D device at this time.";
			//return "DXGI_ERROR_NONEXCLUSIVE:\nA global counter resource is in use, and the Direct3D device can't currently use the counter resource.";

		case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:
			return "DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:\nA resource is not available at the time of the call, but may become available later.";
			//return "DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:\nThe resource or request is not currently available, but it might become available later.";

		case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED:
			return "DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED:\nThe application's remote device has been removed due to session disconnect or network disconnect. The application should call IDXGIFactory1::IsCurrent to find out when the remote device becomes available again.";

		case DXGI_ERROR_REMOTE_OUTOFMEMORY:
			return "DXGI_ERROR_REMOTE_OUTOFMEMORY:\nThe device has been removed during a remote session because the remote computer ran out of memory.";

		case DXGI_ERROR_ACCESS_LOST:
			//return "DXGI_ERROR_ACCESS_LOST:\nThe keyed mutex was abandoned.";
			return "DXGI_ERROR_ACCESS_LOST:\nThe desktop duplication interface is invalid. The desktop duplication interface typically becomes invalid when a different type of image is displayed on the desktop.";

		case DXGI_ERROR_WAIT_TIMEOUT:
			return "DXGI_ERROR_WAIT_TIMEOUT:\nThe timeout value has elapsed and the resource is not yet available.";
			//return "DXGI_ERROR_WAIT_TIMEOUT:\nThe time-out interval elapsed before the next desktop frame was available.";

		case DXGI_ERROR_SDK_COMPONENT_MISSING:
			return "DXGI_ERROR_SDK_COMPONENT_MISSING:\nThe operation depends on an SDK component that is missing or mismatched.";

		case DXGI_ERROR_SESSION_DISCONNECTED:
			return "DXGI_ERROR_SESSION_DISCONNECTED:\nThe output duplication has been turned off because the Windows session ended or was disconnected. This happens when a remote user disconnects, or when \"switch user\" is used locally.";
			//return "DXGI_ERROR_SESSION_DISCONNECTED:\nThe Remote Desktop Services session is currently disconnected.";

		case DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE:
			return "DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE:\nThe DXGI output( monitor ) to which the swapchain content was restricted, has been disconnected or changed.";

		case DXGI_ERROR_CANNOT_PROTECT_CONTENT:
			return "DXGI_ERROR_CANNOT_PROTECT_CONTENT:\nDXGI can't provide content protection on the swap chain. This error is typically caused by an older driver, or when you use a swap chain that is incompatible with content protection.";

		case DXGI_ERROR_ACCESS_DENIED:
			return "DXGI_ERROR_ACCESS_DENIED:\nYou tried to use a resource to which you did not have the required access privileges. This error is most typically caused when you write to a shared resource with read-only access.";

		case DXGI_STATUS_UNOCCLUDED:
			return "DXGI_STATUS_UNOCCLUDED:\nThe swapchain has become unoccluded.";

		case DXGI_STATUS_DDA_WAS_STILL_DRAWING:
			return "DXGI_STATUS_DDA_WAS_STILL_DRAWING:\nThe adapter did not have access to the required resources to complete the Desktop Duplication Present() call, the Present() call needs to be made again.";

		case DXGI_ERROR_MODE_CHANGE_IN_PROGRESS:
			return "DXGI_ERROR_MODE_CHANGE_IN_PROGRESS:\nAn on-going mode change prevented completion of the call. The call may succeed if attempted later.";

		case DXGI_DDI_ERR_WASSTILLDRAWING:
			return "DXGI_DDI_ERR_WASSTILLDRAWING:\nThe GPU was busy when the operation was requested.";

		case DXGI_DDI_ERR_UNSUPPORTED:
			return "DXGI_DDI_ERR_UNSUPPORTED:\nThe driver has rejected the creation of this resource.";

		case DXGI_DDI_ERR_NONEXCLUSIVE:
			return "DXGI_DDI_ERR_NONEXCLUSIVE:\nThe GPU counter was in use by another process or d3d device when application requested access to it.";

		//case DXGI_ERROR_ALREADY_EXISTS:
		//	return "DXGI_ERROR_ALREADY_EXISTS:\nThe desired element already exists. This is returned by DXGIDeclareAdapterRemovalSupport if it is not the first time that the function is called.";

		case D3D10_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:
			return "D3D10_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:\nThe application has exceeded the maximum number of unique state objects per Direct3D device. The limit is 4096 for feature levels up to 11.1.";

		case D3D10_ERROR_FILE_NOT_FOUND:
			return "D3D10_ERROR_FILE_NOT_FOUND:\nThe specified file was not found.";

		case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:
			return "D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:\nThe application has exceeded the maximum number of unique state objects per Direct3D device. The limit is 4096 for feature levels up to 11.1.";

		case D3D11_ERROR_FILE_NOT_FOUND:
			return "D3D11_ERROR_FILE_NOT_FOUND:\nThe specified file was not found.";

		case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS:
			return "D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS:\nThe application has exceeded the maximum number of unique view objects per Direct3D device. The limit is 2 ^ 20 for feature levels up to 11.1.";

		case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD:
			return "D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD:\nThe application's first call per command list to Map on a deferred context did not use D3D11_MAP_WRITE_DISCARD.";

		case D2DERR_WRONG_STATE:
			return "D2DERR_WRONG_STATE:\nThe object was not in the correct state to process the method.";

		case D2DERR_NOT_INITIALIZED:
			return "D2DERR_NOT_INITIALIZED:\nThe object has not yet been initialized.";

		case D2DERR_UNSUPPORTED_OPERATION:
			return "D2DERR_UNSUPPORTED_OPERATION:\nThe requested operation is not supported.";

		case D2DERR_SCANNER_FAILED:
			return "D2DERR_SCANNER_FAILED:\nThe geometry scanner failed to process the data.";

		case D2DERR_SCREEN_ACCESS_DENIED:
			return "D2DERR_SCREEN_ACCESS_DENIED:\nDirect2D could not access the screen.";

		case D2DERR_DISPLAY_STATE_INVALID:
			return "D2DERR_DISPLAY_STATE_INVALID:\nA valid display state could not be determined.";

		case D2DERR_ZERO_VECTOR:
			return "D2DERR_ZERO_VECTOR:\nThe supplied vector is zero.";

		case D2DERR_INTERNAL_ERROR:
			return "D2DERR_INTERNAL_ERROR:\nAn internal error( Direct2D bug ) occurred.On checked builds, we would assert. The application should close this instance of Direct2D and should consider restarting its process.";

		case D2DERR_DISPLAY_FORMAT_NOT_SUPPORTED:
			return "D2DERR_DISPLAY_FORMAT_NOT_SUPPORTED:\nThe display format Direct2D needs to render is not supported by the hardware device.";

		case D2DERR_INVALID_CALL:
			return "D2DERR_INVALID_CALL:\nA call to this method is invalid.";

		case D2DERR_NO_HARDWARE_DEVICE:
			return "D2DERR_NO_HARDWARE_DEVICE:\nNo hardware rendering device is available for this operation.";

		case D2DERR_RECREATE_TARGET:
			return "D2DERR_RECREATE_TARGET:\nThere has been a presentation error that may be recoverable. The caller needs to recreate, rerender the entire frame, and reattempt present.";

		case D2DERR_TOO_MANY_SHADER_ELEMENTS:
			return "D2DERR_TOO_MANY_SHADER_ELEMENTS:\nShader construction failed because it was too complex.";

		case D2DERR_SHADER_COMPILE_FAILED:
			return "D2DERR_SHADER_COMPILE_FAILED:\nShader compilation failed.";

		case D2DERR_MAX_TEXTURE_SIZE_EXCEEDED:
			return "D2DERR_MAX_TEXTURE_SIZE_EXCEEDED:\nRequested DirectX surface size exceeded maximum texture size.";

		case D2DERR_UNSUPPORTED_VERSION:
			return "D2DERR_UNSUPPORTED_VERSION:\nThe requested Direct2D version is not supported.";

		case D2DERR_BAD_NUMBER:
			return "D2DERR_BAD_NUMBER:\nInvalid number.";

		case D2DERR_WRONG_FACTORY:
			return "D2DERR_WRONG_FACTORY:\nObjects used together must be created from the same factory instance.";

		case D2DERR_LAYER_ALREADY_IN_USE:
			return "D2DERR_LAYER_ALREADY_IN_USE:\nA layer resource can only be in use once at any point in time.";

		case D2DERR_POP_CALL_DID_NOT_MATCH_PUSH:
			return "D2DERR_POP_CALL_DID_NOT_MATCH_PUSH:\nThe pop call did not match the corresponding push call.";

		case D2DERR_WRONG_RESOURCE_DOMAIN:
			return "D2DERR_WRONG_RESOURCE_DOMAIN:\nThe resource was realized on the wrong render target.";

		case D2DERR_PUSH_POP_UNBALANCED:
			return "D2DERR_PUSH_POP_UNBALANCED:\nThe push and pop calls were unbalanced.";

		case D2DERR_RENDER_TARGET_HAS_LAYER_OR_CLIPRECT:
			return "D2DERR_RENDER_TARGET_HAS_LAYER_OR_CLIPRECT:\nAttempt to copy from a render target while a layer or clip rect is applied.";

		case D2DERR_INCOMPATIBLE_BRUSH_TYPES:
			return "D2DERR_INCOMPATIBLE_BRUSH_TYPES:\nThe brush types are incompatible for the call.";

		case D2DERR_WIN32_ERROR:
			return "D2DERR_WIN32_ERROR:\nAn unknown win32 failure occurred.";

		case D2DERR_TARGET_NOT_GDI_COMPATIBLE:
			return "D2DERR_TARGET_NOT_GDI_COMPATIBLE:\nThe render target is not compatible with GDI.";

		case D2DERR_TEXT_EFFECT_IS_WRONG_TYPE:
			return "D2DERR_TEXT_EFFECT_IS_WRONG_TYPE:\nA text client drawing effect object is of the wrong type.";

		case D2DERR_TEXT_RENDERER_NOT_RELEASED:
			return "D2DERR_TEXT_RENDERER_NOT_RELEASED:\nThe application is holding a reference to the IDWriteTextRenderer interface after the corresponding DrawText or DrawTextLayout call has returned. The IDWriteTextRenderer instance will be invalid.";

		case D2DERR_EXCEEDS_MAX_BITMAP_SIZE:
			return "D2DERR_EXCEEDS_MAX_BITMAP_SIZE:\nThe requested size is larger than the guaranteed supported texture size at the Direct3D device's current feature level.";

		case D2DERR_INVALID_GRAPH_CONFIGURATION:
			return "D2DERR_INVALID_GRAPH_CONFIGURATION:\nThere was a configuration error in the graph.";

		case D2DERR_INVALID_INTERNAL_GRAPH_CONFIGURATION:
			return "D2DERR_INVALID_INTERNAL_GRAPH_CONFIGURATION:\nThere was a internal configuration error in the graph.";

		case D2DERR_CYCLIC_GRAPH:
			return "D2DERR_CYCLIC_GRAPH:\nThere was a cycle in the graph.";

		case D2DERR_BITMAP_CANNOT_DRAW:
			return "D2DERR_BITMAP_CANNOT_DRAW:\nCannot draw with a bitmap that has the D2D1_BITMAP_OPTIONS_CANNOT_DRAW option.";

		case D2DERR_OUTSTANDING_BITMAP_REFERENCES:
			return "D2DERR_OUTSTANDING_BITMAP_REFERENCES:\nThe operation cannot complete while there are outstanding references to the target bitmap.";

		case D2DERR_ORIGINAL_TARGET_NOT_BOUND:
			return "D2DERR_ORIGINAL_TARGET_NOT_BOUND:\nThe operation failed because the original target is not currently bound as a target.";

		case D2DERR_INVALID_TARGET:
			return "D2DERR_INVALID_TARGET:\nCannot set the image as a target because it is either an effect or is a bitmap that does not have the D2D1_BITMAP_OPTIONS_TARGET flag set.";

		case D2DERR_BITMAP_BOUND_AS_TARGET:
			return "D2DERR_BITMAP_BOUND_AS_TARGET:\nCannot draw with a bitmap that is currently bound as the target bitmap.";

		case D2DERR_INSUFFICIENT_DEVICE_CAPABILITIES:
			return "D2DERR_INSUFFICIENT_DEVICE_CAPABILITIES:\nD3D Device does not have sufficient capabilities to perform the requested action.";

		case D2DERR_INTERMEDIATE_TOO_LARGE:
			return "D2DERR_INTERMEDIATE_TOO_LARGE:\nThe graph could not be rendered with the context's current tiling settings.";

		case D2DERR_EFFECT_IS_NOT_REGISTERED:
			return "D2DERR_EFFECT_IS_NOT_REGISTERED:\nThe CLSID provided to Unregister did not correspond to a registered effect.";

		case D2DERR_INVALID_PROPERTY:
			return "D2DERR_INVALID_PROPERTY:\nThe specified property does not exist.";

		case D2DERR_NO_SUBPROPERTIES:
			return "D2DERR_NO_SUBPROPERTIES:\nThe specified sub - property does not exist.";

		case D2DERR_PRINT_JOB_CLOSED:
			return "D2DERR_PRINT_JOB_CLOSED:\nAddPage or Close called after print job is already closed.";

		case D2DERR_PRINT_FORMAT_NOT_SUPPORTED:
			return "D2DERR_PRINT_FORMAT_NOT_SUPPORTED:\nError during print control creation.Indicates that none of the package target types( representing printer formats ) are supported by Direct2D print control.";

		case D2DERR_TOO_MANY_TRANSFORM_INPUTS:
			return "D2DERR_TOO_MANY_TRANSFORM_INPUTS:\nAn effect attempted to use a transform with too many inputs.";

		case DWRITE_E_FILEFORMAT:
			return "DWRITE_E_FILEFORMAT:\nIndicates an error in an input file such as a font file.";

		case DWRITE_E_UNEXPECTED:
			return "DWRITE_E_UNEXPECTED:\nIndicates an error originating in DirectWrite code, which is not expected to occur but is safe to recover from.";

		case DWRITE_E_NOFONT:
			return "DWRITE_E_NOFONT:\nIndicates the specified font does not exist.";

		case DWRITE_E_FILENOTFOUND:
			return "DWRITE_E_FILENOTFOUND:\nA font file could not be opened because the file, directory, network location, drive, or other storage location does not exist or is unavailable.";

		case DWRITE_E_FILEACCESS:
			return "DWRITE_E_FILEACCESS:\nA font file exists but could not be opened due to access denied, sharing violation, or similar error.";

		case DWRITE_E_FONTCOLLECTIONOBSOLETE:
			return "DWRITE_E_FONTCOLLECTIONOBSOLETE:\nA font collection is obsolete due to changes in the system.";

		case DWRITE_E_ALREADYREGISTERED:
			return "DWRITE_E_ALREADYREGISTERED:\nThe given interface is already registered.";

		case DWRITE_E_CACHEFORMAT:
			return "DWRITE_E_CACHEFORMAT:\nThe font cache contains invalid data.";

		case DWRITE_E_CACHEVERSION:
			return "DWRITE_E_CACHEVERSION:\nA font cache file corresponds to a different version of DirectWrite.";

		case DWRITE_E_UNSUPPORTEDOPERATION:
			return "DWRITE_E_UNSUPPORTEDOPERATION:\nThe operation is not supported for this type of font.";

		case WINCODEC_ERR_WRONGSTATE:
			return "WINCODEC_ERR_WRONGSTATE:\nThe codec is in the wrong state.";

		case WINCODEC_ERR_VALUEOUTOFRANGE:
			return "WINCODEC_ERR_VALUEOUTOFRANGE:\nThe value is out of range.";

		case WINCODEC_ERR_UNKNOWNIMAGEFORMAT:
			return "WINCODEC_ERR_UNKNOWNIMAGEFORMAT:\nThe image format is unknown.";

		case WINCODEC_ERR_UNSUPPORTEDVERSION:
			return "WINCODEC_ERR_UNSUPPORTEDVERSION:\nThe SDK version is unsupported.";

		case WINCODEC_ERR_NOTINITIALIZED:
			return "WINCODEC_ERR_NOTINITIALIZED:\nThe component is not initialized.";

		case WINCODEC_ERR_ALREADYLOCKED:
			return "WINCODEC_ERR_ALREADYLOCKED:\nThere is already an outstanding read or write lock.";

		case WINCODEC_ERR_PROPERTYNOTFOUND:
			return "WINCODEC_ERR_PROPERTYNOTFOUND:\nThe specified bitmap property cannot be found.";

		case WINCODEC_ERR_PROPERTYNOTSUPPORTED:
			return "WINCODEC_ERR_PROPERTYNOTSUPPORTED:\nThe bitmap codec does not support the bitmap property.";

		case WINCODEC_ERR_PROPERTYSIZE:
			return "WINCODEC_ERR_PROPERTYSIZE:\nThe bitmap property size is invalid.";

		case WINCODEC_ERR_CODECPRESENT:
			return "WINCODEC_ERR_CODECPRESENT:\nAn unknown error has occurred.";

		case WINCODEC_ERR_CODECNOTHUMBNAIL:
			return "WINCODEC_ERR_CODECNOTHUMBNAIL:\nThe bitmap codec does not support a thumbnail.";

		case WINCODEC_ERR_PALETTEUNAVAILABLE:
			return "WINCODEC_ERR_PALETTEUNAVAILABLE:\nThe bitmap palette is unavailable.";

		case WINCODEC_ERR_CODECTOOMANYSCANLINES:
			return "WINCODEC_ERR_CODECTOOMANYSCANLINES:\nToo many scanlines were requested.";

		case WINCODEC_ERR_INTERNALERROR:
			return "WINCODEC_ERR_INTERNALERROR:\nAn internal error occurred.";

		case WINCODEC_ERR_SOURCERECTDOESNOTMATCHDIMENSIONS:
			return "WINCODEC_ERR_SOURCERECTDOESNOTMATCHDIMENSIONS:\nThe bitmap bounds do not match the bitmap dimensions.";

		case WINCODEC_ERR_COMPONENTNOTFOUND:
			return "WINCODEC_ERR_COMPONENTNOTFOUND:\nThe component cannot be found.";

		case WINCODEC_ERR_IMAGESIZEOUTOFRANGE:
			return "WINCODEC_ERR_IMAGESIZEOUTOFRANGE:\nThe bitmap size is outside the valid range.";

		case WINCODEC_ERR_TOOMUCHMETADATA:
			return "WINCODEC_ERR_TOOMUCHMETADATA:\nThere is too much metadata to be written to the bitmap.";

		case WINCODEC_ERR_BADIMAGE:
			return "WINCODEC_ERR_BADIMAGE:\nThe image is unrecognized.";

		case WINCODEC_ERR_BADHEADER:
			return "WINCODEC_ERR_BADHEADER:\nThe image header is unrecognized.";

		case WINCODEC_ERR_FRAMEMISSING:
			return "WINCODEC_ERR_FRAMEMISSING:\nThe bitmap frame is missing.";

		case WINCODEC_ERR_BADMETADATAHEADER:
			return "WINCODEC_ERR_BADMETADATAHEADER:\nThe image metadata header is unrecognized.";

		case WINCODEC_ERR_BADSTREAMDATA:
			return "WINCODEC_ERR_BADSTREAMDATA:\nThe stream data is unrecognized.";

		case WINCODEC_ERR_STREAMWRITE:
			return "WINCODEC_ERR_STREAMWRITE:\nFailed to write to the stream.";

		case WINCODEC_ERR_STREAMREAD:
			return "WINCODEC_ERR_STREAMREAD:\nFailed to read from the stream.";

		case WINCODEC_ERR_STREAMNOTAVAILABLE:
			return "WINCODEC_ERR_STREAMNOTAVAILABLE:\nThe stream is not available.";

		case WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT:
			return "WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT:\nThe bitmap pixel format is unsupported.";

		case WINCODEC_ERR_UNSUPPORTEDOPERATION:
			return "WINCODEC_ERR_UNSUPPORTEDOPERATION:\nThe operation is unsupported.";

		case WINCODEC_ERR_INVALIDREGISTRATION:
			return "WINCODEC_ERR_INVALIDREGISTRATION:\nThe component registration is invalid.";

		case WINCODEC_ERR_COMPONENTINITIALIZEFAILURE:
			return "WINCODEC_ERR_COMPONENTINITIALIZEFAILURE:\nThe component initialization has failed.";

		case WINCODEC_ERR_INSUFFICIENTBUFFER:
			return "WINCODEC_ERR_INSUFFICIENTBUFFER:\nThe buffer allocated is insufficient.";

		case WINCODEC_ERR_DUPLICATEMETADATAPRESENT:
			return "WINCODEC_ERR_DUPLICATEMETADATAPRESENT:\nDuplicate metadata is present.";

		case WINCODEC_ERR_PROPERTYUNEXPECTEDTYPE:
			return "WINCODEC_ERR_PROPERTYUNEXPECTEDTYPE:\nThe bitmap property type is unexpected.";

		case WINCODEC_ERR_UNEXPECTEDSIZE:
			return "WINCODEC_ERR_UNEXPECTEDSIZE:\nThe size is unexpected.";

		case WINCODEC_ERR_INVALIDQUERYREQUEST:
			return "WINCODEC_ERR_INVALIDQUERYREQUEST:\nThe property query is invalid.";

		case WINCODEC_ERR_UNEXPECTEDMETADATATYPE:
			return "WINCODEC_ERR_UNEXPECTEDMETADATATYPE:\nThe metadata type is unexpected.";

		case WINCODEC_ERR_REQUESTONLYVALIDATMETADATAROOT:
			return "WINCODEC_ERR_REQUESTONLYVALIDATMETADATAROOT:\nThe specified bitmap property is only valid at root level.";

		case WINCODEC_ERR_INVALIDQUERYCHARACTER:
			return "WINCODEC_ERR_INVALIDQUERYCHARACTER:\nThe query string contains an invalid character.";

		case WINCODEC_ERR_WIN32ERROR:
			return "WINCODEC_ERR_WIN32ERROR:\nWindows Codecs received an error from the Win32 system.";

		case WINCODEC_ERR_INVALIDPROGRESSIVELEVEL:	
			return "WINCODEC_ERR_INVALIDPROGRESSIVELEVEL:\nThe requested level of detail is not present.";
	}

	return "Unknown Error Code";
}
