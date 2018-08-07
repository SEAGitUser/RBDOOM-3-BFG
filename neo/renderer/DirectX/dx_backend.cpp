#pragma hdrstop
#include "precompiled.h"

#include "../tr_local.h"

#include "../../sys/win32/rc/doom_resource.h"
#include "../../sys/win32/win_local.h"

#include "dx_header.h"

#include <dxgi1_2.h>

// Link library dependencies
#pragma comment( lib, "d3d11.lib")
#pragma comment( lib, "dxgi.lib")
//#pragma comment( lib, "d3dcompiler.lib")
//#pragma comment( lib, "winmm.lib")

/*
=====================================================================================================
	Contains the backbuffer implementation for DirecX
=====================================================================================================
*/

/** Returns true if desktop composition is enabled. */
static bool IsCompositionEnabled()
{
	BOOL bDwmEnabled = false;
#if D3D11_WITH_DWMAPI
	DwmIsCompositionEnabled( &bDwmEnabled );
#endif	//D3D11_WITH_DWMAPI
	return !!bDwmEnabled;
}
/*
=======================================
	idDirectXInterface
=======================================
*/
class idDirectXInterface
{
	DXFactory *	factoryHandle = nullptr;
	DXGI_GAMMA_CONTROL initialGammaSettings;

	//idStaticList<dxRenderContext_t*, 5> rcList;
	//
	//bool CreateDeviceInternal( bool bDoDebug, DXDevice*&, DXDeviceContext*& ) const;
	//bool CreateSwapchainInternal( DXDevice*, dxBackBuffer_t &,
	//	HWND hWnd, UINT width, UINT height,
	//	bool bFullscreen, bool bFloatFormat, bool bTransparency ) const;

public:

	void Init()
	{
		auto hr = ::CreateDXGIFactory1( __uuidof( DXFactory ), ( void** )&factoryHandle );
		if( FAILED( hr ) )
		{
			common->FatalError( "Failed to CreateDXGIFactory1( IDXGIFactory2 )");
		}

		IDXGIOutput * output = nullptr;

		// Get the output (the display monitor) that contains the majority of the client area of the target window.
		IDXGISwapChain1 * sc;
		sc->GetContainingOutput( &output );

		// save the hardware gamma so it can be
		// restored on exit.
		output->GetGammaControl( &initialGammaSettings );
	}

	void Shutdown()
	{
		IDXGIOutput * output = nullptr;

		// Get the output (the display monitor) that contains the majority of the client area of the target window.
		IDXGISwapChain1 * sc;
		sc->GetContainingOutput( &output );

		// restore gamma.
		output->SetGammaControl( &initialGammaSettings );

		//rcList.DeleteContents( true );
		SAFE_RELEASE( factoryHandle );
	}

	DXFactory * GetFactoryHandle() const { return factoryHandle; }

	DXFactory * DeriveFactoryFromDeviceHandle( DXDevice * hDevice ) const
	{
		IDXGIDevice2 * pDXGIDevice2 = nullptr;
		auto res = hDevice->QueryInterface( __uuidof( IDXGIDevice2 ), ( void ** )&pDXGIDevice2 );
		if( FAILED( res ) || !pDXGIDevice2 || res == E_POINTER )
		{
			common->Warning( "Failed to QueryInterface( IDXGIDevice2 )\n" );
			return nullptr;
		}
		IDXGIAdapter2 * pDXGIAdapter2 = nullptr;
		res = pDXGIDevice2->GetParent( __uuidof( IDXGIAdapter2 ), ( void ** )&pDXGIAdapter2 );
		pDXGIDevice2->Release();
		if( FAILED( res ) || !pDXGIAdapter2 || res == E_POINTER )
		{
			common->Warning( "Failed to QueryInterface( IDXGIAdapter2 )\n" );
			return nullptr;
		}
		DXFactory * pIDXGIFactory2 = nullptr;
		res = pDXGIAdapter2->GetParent( __uuidof( DXFactory ), ( void ** )&pIDXGIFactory2 );
		pDXGIAdapter2->Release();
		if( FAILED( res ) || !pIDXGIFactory2 || res == E_POINTER )
		{
			common->Warning( "Failed to QueryInterface( IDXGIFactory2 )\n" );
			return nullptr;
		}
		return pIDXGIFactory2;
	}

	// EnumOutputs first returns the output on which the desktop primary is displayed.
	// This output corresponds with an index of zero.
	// EnumOutputs then returns other outputs.
	uint32 CountAdapterOutputs( DXAdapter * Adapter ) const
	{
		uint32 count = 0;
		IDXGIOutput * pOutput;
		while( Adapter->EnumOutputs( count, &pOutput ) != DXGI_ERROR_NOT_FOUND )
		{
			DXGI_OUTPUT_DESC Desc = {};
			pOutput->GetDesc( &Desc );
			pOutput->Release();

			if( !Desc.AttachedToDesktop )
				continue;

			++count;
		}
		return count;
	}



	//dxRenderContext_t * CreateRenderContext( bool bPresentable );


} DXInterface;






static uint32 FindClosestMatchingMode( DXAdapter * Adapter, int Width, int Height )
{
	DXGI_MODE_DESC1 closestMatch = {};

	uint32 count = 0;
	IDXGIOutput * pOutput;
	while( Adapter->EnumOutputs( count, &pOutput ) != DXGI_ERROR_NOT_FOUND )
	{
		IDXGIOutput1 * pOutput1 = nullptr;
		pOutput->QueryInterface( __uuidof( IDXGIOutput1 ), ( void ** )&pOutput1 );
		pOutput->Release();

		DXGI_MODE_DESC1 modeToMatch = {};
		modeToMatch.Width = Width;
		modeToMatch.Height = Height;
		modeToMatch.RefreshRate;
		modeToMatch.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

		auto hr = pOutput1->FindClosestMatchingMode1( &modeToMatch, &closestMatch, NULL );
		pOutput1->Release();

		++count;
	}
	return count;
}



struct dxBackBuffer_t
{
	DXRenderTargetView * colorView = nullptr;
	IDXGISwapChain1 * hSwapChain1 = nullptr;

	void Release()
	{
		SAFE_RELEASE( colorView );
		SAFE_RELEASE( hSwapChain1 );
	}

	// Get buffer and create a render-target-view.
	void DeriveResources( DXDevice * hDevice )
	{
		DXTexture2D * pBuffer = nullptr;
		auto res = hSwapChain1->GetBuffer( 0, __uuidof( DXTexture2D ), ( void** )&pBuffer );
		if( FAILED( res ) )
		{
			common->FatalError( "Failed to hSwapChain1->GetBuffer()" );
		}
		res = hDevice->CreateRenderTargetView( pBuffer, NULL, &colorView );
		if( FAILED( res ) )
		{
			common->FatalError( "Failed to CreateRenderTargetView( colorView )" );
		}
		pBuffer->Release();
	}

	bool IsFullscreen() const
	{
		BOOL bFullscreenState;
		hSwapChain1->GetFullscreenState( &bFullscreenState, NULL );
		return !!bFullscreenState;
	}
};


struct dxRenderContext_t
{
	DXDevice * hDevice = nullptr;
	DXDeviceContext * hIContext = nullptr;
	D3D_FEATURE_LEVEL featureLevelsSupported = D3D_FEATURE_LEVEL_10_0;

	dxBackBuffer_t	backBuffer;

	void Release()
	{
		hIContext->Flush();
		hIContext->ClearState();

		backBuffer.Release();

		SAFE_RELEASE( hIContext );
		SAFE_RELEASE( hDevice );

		featureLevelsSupported = D3D_FEATURE_LEVEL_10_0;


	}
	// handler for WM_SIZE messages
	void ResizeBackbuffer()
	{
		hIContext->OMSetRenderTargets( 0, 0, 0 );

		//hIContext->ClearState();

		// Release all outstanding references to the swap chain's buffers.
		UINT refs = backBuffer.colorView->Release();
		assert( refs <= 0 );

		// Preserve the existing buffer count and format.
		// Automatically choose the width and height to match the client rect for HWNDs.
		auto res = backBuffer.hSwapChain1->ResizeBuffers( 0, 0, 0, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH );
		if( FAILED( res ) )
		{
			common->FatalError( "Failed to hSwapChain1->ResizeBuffers()" );
		}

		backBuffer.DeriveResources( hDevice );
	}
};



void DX_EnumGPUs( DXFactory * pIDXGIFactory )
{
	bool bIsAnyAMD = false;
	bool bIsAnyIntel = false;
	bool bIsAnyNVIDIA = false;
	IDXGIAdapter1 * pAdapter1;
	for( uint32 adapterIndex = 0;
		pIDXGIFactory->EnumAdapters1( adapterIndex, &pAdapter1 ) != DXGI_ERROR_NOT_FOUND;
		++adapterIndex )
	{
		IDXGIAdapter2 * pDXGIAdapter2 = nullptr;
		auto res = pAdapter1->QueryInterface( __uuidof( IDXGIAdapter2 ), ( void ** )&pDXGIAdapter2 );
		if( FAILED( res ) || !pDXGIAdapter2 || res == E_POINTER )
		{
			pAdapter1->Release();
			common->Warning( "Failed to QueryInterface( IDXGIAdapter2 )\n" );
			continue;
		}
		pAdapter1->Release();

		DXGI_ADAPTER_DESC2 AdapterDesc = {};
		pDXGIAdapter2->GetDesc2( &AdapterDesc );
		{
			bool bIsAMD = AdapterDesc.VendorId == 0x1002;
			bool bIsIntel = AdapterDesc.VendorId == 0x8086;
			bool bIsNVIDIA = AdapterDesc.VendorId == 0x10DE;
			bool bIsMicrosoft = AdapterDesc.VendorId == 0x1414;

			if( bIsAMD ) bIsAnyAMD = true;
			if( bIsIntel ) bIsAnyIntel = true;
			if( bIsNVIDIA ) bIsAnyNVIDIA = true;

			// Simple heuristic but without profiling it's hard to do better
			const bool bIsIntegrated = bIsIntel;
			// PerfHUD is for performance profiling
			const bool bIsPerfHUD = !idWStr::Icmp( AdapterDesc.Description, L"NVIDIA PerfHUD" );


			IDXGIOutput * pOutput;
			for( uint32 outputIndex = 0;
				pDXGIAdapter2->EnumOutputs( outputIndex, &pOutput ) != DXGI_ERROR_NOT_FOUND;
				++outputIndex )
			{
				IDXGIOutput1 * pOutput1 = nullptr;
				auto res = pOutput->QueryInterface( __uuidof( IDXGIOutput1 ), ( void ** )&pOutput1 );
				if( FAILED( res ) || !pOutput1 || res == E_POINTER )
				{
					pOutput->Release();
					common->Warning( "Failed to QueryInterface( IDXGIOutput1 )\n" );
					continue;
				}
				pOutput->Release();

				DXGI_OUTPUT_DESC Desc = {};
				pOutput1->GetDesc( &Desc );
				{

				}
				pOutput1->Release();
			}
		}
		pDXGIAdapter2->Release();
	}
}

bool DX_CreateDevice( bool bDoDebug, DXDevice* & hDevice1, DXDeviceContext* & hIContext1 )
{
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	bool bDebugAvailable = IsSdkDebugLayerAvailable();
	UINT Flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
	if( bDoDebug && bDebugAvailable )
	{
		Flags |= D3D11_CREATE_DEVICE_DEBUG;
	}

	D3D_FEATURE_LEVEL featureLevelsSupported;
	ID3D11Device * hDevice = nullptr;
	ID3D11DeviceContext * hIContext = nullptr;

	HRESULT res = D3D11CreateDevice( NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, Flags,
		featureLevels, 1, D3D11_SDK_VERSION, &hDevice,
		&featureLevelsSupported, &hIContext );
	if( FAILED( res ) ) // 11_1 not supported
	{
		/*/*/if( res == E_INVALIDARG ) common->DWarning( "Failed to create DXDevice with D3D_FEATURE_LEVEL_11_1: E_INVALIDARG" );
		else if( res == DXGI_ERROR_UNSUPPORTED ) common->DWarning( "Failed to create DXDevice with D3D_FEATURE_LEVEL_11_1: DXGI_ERROR_UNSUPPORTED" );
		else if( res == DXGI_ERROR_INVALID_CALL ) common->DWarning( "Failed to create DXDevice with D3D_FEATURE_LEVEL_11_1: DXGI_ERROR_INVALID_CALL" );
		else if( res == E_NOTIMPL ) common->DWarning( "Failed to create DXDevice with D3D_FEATURE_LEVEL_11_1: E_NOTIMPL" );

		res = D3D11CreateDevice( NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, Flags,
			&featureLevels[1], UINT( _countof( featureLevels ) - 1U ), D3D11_SDK_VERSION, &hDevice,
			&featureLevelsSupported, &hIContext );
	}
	if( FAILED( res ) )
	{
		hIContext->Release();
		hDevice->Release();
		common->Warning( "Failed to create any DXDevice\n" );
		return false;
	}

	common->Printf( "Created DXDevice %s\n", GetFeatureLevelStr( featureLevelsSupported ) );

	res = hDevice->QueryInterface( __uuidof( ID3D11Device1 ), ( void ** )&hDevice1 );
	if( FAILED( res ) || !hDevice1 || res == E_POINTER )
	{
		hIContext->Release();
		hDevice->Release();
		common->Warning( "Failed to QueryInterface( ID3D11Device1 )\n" );
		return false;
	}
	res = hIContext->QueryInterface( __uuidof( ID3D11DeviceContext1 ), ( void ** )&hIContext1 );
	if( FAILED( res ) || !hIContext1 || res == E_POINTER )
	{
		hDevice1->Release();
		hIContext->Release();
		hDevice->Release();
		common->Warning( "Failed to QueryInterface( ID3D11DeviceContext1 )\n" );
		return false;
	}

	{
		D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS features;
		hDevice1->CheckFeatureSupport( D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &features, sizeof( features ) );
		common->Printf( "D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS:\n" );
		common->Printf( "-ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x: %i\n", features.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x );
	}
	{
		D3D11_FEATURE_DATA_D3D11_OPTIONS features;
		hDevice1->CheckFeatureSupport( D3D11_FEATURE_D3D11_OPTIONS, &features, sizeof( features ) );
		common->Printf( "D3D11_FEATURE_DATA_D3D11_OPTIONS:\n" );
		common->Printf( "-OutputMergerLogicOp: %i\n", features.OutputMergerLogicOp );
		common->Printf( "-UAVOnlyRenderingForcedSampleCount: %i\n", features.UAVOnlyRenderingForcedSampleCount );
		common->Printf( "-DiscardAPIsSeenByDriver: %i\n", features.DiscardAPIsSeenByDriver );
		common->Printf( "-FlagsForUpdateAndCopySeenByDriver: %i\n", features.FlagsForUpdateAndCopySeenByDriver );
		common->Printf( "-ClearView: %i\n", features.ClearView );
		common->Printf( "-CopyWithOverlap: %i\n", features.CopyWithOverlap );
		common->Printf( "-ConstantBufferPartialUpdate: %i\n", features.ConstantBufferPartialUpdate );
		common->Printf( "-ConstantBufferOffsetting: %i\n", features.ConstantBufferOffsetting );
		common->Printf( "-MapNoOverwriteOnDynamicConstantBuffer: %i\n", features.MapNoOverwriteOnDynamicConstantBuffer );
		common->Printf( "-MapNoOverwriteOnDynamicBufferSRV: %i\n", features.MapNoOverwriteOnDynamicBufferSRV );
		common->Printf( "-MultisampleRTVWithForcedSampleCountOne: %i\n", features.MultisampleRTVWithForcedSampleCountOne );
		common->Printf( "-SAD4ShaderInstructions: %i\n", features.SAD4ShaderInstructions );
		common->Printf( "-ExtendedDoublesShaderInstructions: %i\n", features.ExtendedDoublesShaderInstructions );
		common->Printf( "-ExtendedResourceSharing: %i\n", features.ExtendedResourceSharing );
	}

	if( bDoDebug && bDebugAvailable )
	{
		ID3D11Debug *d3dDebug = nullptr;
		if( SUCCEEDED( hDevice->QueryInterface( __uuidof( ID3D11Debug ), ( void** )&d3dDebug ) ) )
		{
			ID3D11InfoQueue *d3dInfoQueue = nullptr;
			if( SUCCEEDED( d3dDebug->QueryInterface( __uuidof( ID3D11InfoQueue ), ( void** )&d3dInfoQueue ) ) )
			{
			#ifdef _DEBUG
				d3dInfoQueue->SetBreakOnSeverity( D3D11_MESSAGE_SEVERITY_CORRUPTION, true );
				d3dInfoQueue->SetBreakOnSeverity( D3D11_MESSAGE_SEVERITY_ERROR, true );
			#endif
				D3D11_MESSAGE_SEVERITY severities[] = {
					D3D11_MESSAGE_SEVERITY_INFO,
					D3D11_MESSAGE_SEVERITY_MESSAGE
				};
				D3D11_INFO_QUEUE_FILTER filter = {};
				filter.AllowList.NumSeverities = _countof( severities );
				filter.AllowList.pSeverityList = severities;
				d3dInfoQueue->AddStorageFilterEntries( &filter );

				d3dInfoQueue->Release();
			}
			d3dDebug->Release();
		}
	}

	hIContext->Release();
	hDevice->Release();

	return true;
}


bool DX_CreateSwapchain( DXFactory * pIDXGIFactory2, ID3D11Device1 * hDevice1, dxBackBuffer_t & backBuffer,
	HWND hWnd, UINT width, UINT height,
	bool bFullscreen, bool bFloatFormat, bool bTransparency )
{
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	// If the buffer width or the buffer height is zero, the sizes will be inferred
	// from the output window size in the swap-chain description.
	desc.Width = width;
	desc.Height = height;
	desc.Format = bFloatFormat ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM; // DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;//| DXGI_USAGE_SHADER_INPUT;
	desc.BufferCount = 2;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	desc.AlphaMode = bTransparency ? DXGI_ALPHA_MODE_STRAIGHT : DXGI_ALPHA_MODE_IGNORE;
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING

	auto res = pIDXGIFactory2->CreateSwapChainForHwnd( hDevice1, hWnd, &desc, NULL, NULL, &backBuffer.hSwapChain1 );
	if( FAILED( res ) || !backBuffer.hSwapChain1 )
	{
		/*/*/if( res == E_OUTOFMEMORY ) common->DWarning( "Failed to CreateSwapChainForHwnd( IDXGISwapChain1 with DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL ): E_INVALIDARG" );
		else if( res == E_NOTIMPL ) common->DWarning( "Failed to CreateSwapChainForHwnd( IDXGISwapChain1 with DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL ): E_NOTIMPL" );
		else common->DWarning( "Failed to CreateSwapChainForHwnd( IDXGISwapChain1 with DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL ):\n%s", WinErrStr( res ) );

		desc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
		res = pIDXGIFactory2->CreateSwapChainForHwnd( hDevice1, hWnd, &desc, NULL, NULL, &backBuffer.hSwapChain1 );
		if( FAILED( res ) || !backBuffer.hSwapChain1 )
		{
			/*/*/if( res == E_OUTOFMEMORY ) common->DWarning( "Failed to CreateSwapChainForHwnd( IDXGISwapChain1 with DXGI_SWAP_EFFECT_SEQUENTIAL ): E_INVALIDARG" );
			else if( res == E_NOTIMPL ) common->DWarning( "Failed to CreateSwapChainForHwnd( IDXGISwapChain1 with DXGI_SWAP_EFFECT_SEQUENTIAL ): E_NOTIMPL" );
			else common->DWarning( "Failed to CreateSwapChainForHwnd( IDXGISwapChain1 with DXGI_SWAP_EFFECT_SEQUENTIAL ):\n%s", WinErrStr( res ) );

			desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
			res = pIDXGIFactory2->CreateSwapChainForHwnd( hDevice1, hWnd, &desc, NULL, NULL, &backBuffer.hSwapChain1 );
			if( FAILED( res ) || !backBuffer.hSwapChain1 )
			{
				/*/*/if( res == E_OUTOFMEMORY ) common->DWarning( "Failed to CreateSwapChainForHwnd( IDXGISwapChain1 with DXGI_SWAP_EFFECT_DISCARD ): E_INVALIDARG" );
				else if( res == E_NOTIMPL ) common->DWarning( "Failed to CreateSwapChainForHwnd( IDXGISwapChain1 with DXGI_SWAP_EFFECT_DISCARD ): E_NOTIMPL" );
				else common->DWarning( "Failed to CreateSwapChainForHwnd( IDXGISwapChain1 with DXGI_SWAP_EFFECT_DISCARD ):\n%s", WinErrStr( res ) );

				return false;
			}
		}
	}

	DXGI_SWAP_CHAIN_DESC1 Desc = {};
	backBuffer.hSwapChain1->GetDesc1( &Desc );
	common->Printf( "Created SwapChain(%ux%u) with %s.\n", Desc.Width, Desc.Height,
		( Desc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL )? "DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL" :
		(( Desc.SwapEffect == DXGI_SWAP_EFFECT_SEQUENTIAL )? "DXGI_SWAP_EFFECT_SEQUENTIAL" : "DXGI_SWAP_EFFECT_DISCARD" ) );

	if( bFullscreen )
	{
		backBuffer.hSwapChain1->SetFullscreenState( TRUE, NULL );
	}

	backBuffer.DeriveResources( hDevice1 );

	return true;
}

//----------------------------------------------------------------------------
// Present the frame
//----------------------------------------------------------------------------
HRESULT DX_SwapBuffers( int frequency )
{
	IDXGISwapChain1 * g_pSwapChain;

	//int frequency = 0;
	//if ( r_swapInterval->integer > 0 )
	//{
	//	frequency = min( glConfig.displayFrequency, 60 / r_swapInterval.GetInteger() );
	//}

	HRESULT hr = S_OK;
	switch( frequency )
	{
		case 60: hr = g_pSwapChain->Present1( 1, 0, nullptr ); break;
		case 30: hr = g_pSwapChain->Present1( 2, 0, nullptr ); break;
		default: hr = g_pSwapChain->Present1( 0, 0, nullptr ); break;
	}

	if( FAILED( hr ) )
		return hr;

#ifdef WIN8
	auto iContext = GetCurrentImContext();

	// Discard the contents of the render target.
	// This is a valid operation only when the existing contents will be entirely
	// overwritten. If dirty or scroll rects are used, this call should be removed.
	iContext->DiscardView( g_BufferState.backBufferView );

	// Discard the contents of the depth stencil.
	iContext->DiscardView( g_BufferState.depthBufferView );

	// Present unbinds the rendertarget, so let's put it back (FFS)
	iContext->OMSetRenderTargets( 1, &g_BufferState.backBufferView, g_BufferState.depthBufferView );
#endif

	return S_OK;
}





dxRenderContext_t g_MainRenderContext;

/*
====================
GLW_CreateWindowClasses
====================
*/
static void GLW_CreateWindowClasses()
{
	//
	// register the window class if necessary
	//
	if( win32.windowClassRegistered )
	{
		return;
	}

	WNDCLASS wc = {};
	wc.style = 0;
	wc.lpfnWndProc = ( WNDPROC )MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = ( HINSTANCE )::GetModuleHandle( nullptr );
	wc.hIcon = ::LoadIcon( wc.hInstance, MAKEINTRESOURCE( IDI_ICON1 ) );
	wc.hCursor = NULL;
	wc.hbrBackground = ( struct HBRUSH__* )COLOR_GRAYTEXT;
	wc.lpszMenuName = 0;
	wc.lpszClassName = WIN32_WINDOW_CLASS_NAME;

	if( !::RegisterClass( &wc ) )
	{
		common->FatalError( "GLW_CreateWindow: could not register window class" );
	}
	common->Printf( "...registered render window class\n" );

	win32.windowClassRegistered = true;
}

/*
====================
GLW_GetWindowDimensions
====================
*/
static bool DXW_GetWindowDimensions( const glimpParms_t & parms, int& x, int& y, int& w, int& h )
{
	//
	// compute width and height
	//
	if( parms.fullScreen != 0 )
	{
		if( parms.IsBorderLessWindow() )
		{
			// borderless window at specific location, as for spanning
			// multiple monitor outputs
			x = parms.x;
			y = parms.y;
			w = parms.width;
			h = parms.height;
		}
		else {
			// get the current monitor position and size on the desktop, assuming
			// any required ChangeDisplaySettings has already been done
			int displayHz = 0;
			extern bool GetDisplayCoordinates( const int deviceNum, int& x, int& y, int& width, int& height, int& displayHz );
			if( !GetDisplayCoordinates( parms.GetOutputIndex(), x, y, w, h, displayHz ) )
			{
				return false;
			}
		}
	}
	else {
		RECT r;
		::SetRect( &r, 0, 0, parms.width, parms.height );
		// adjust width and height for window border
		::AdjustWindowRect( &r, WINDOW_STYLE | WS_SYSMENU, FALSE );

		w = r.right - r.left;
		h = r.bottom - r.top;

		x = parms.x;
		y = parms.y;
	}

	return true;
}

/*
=======================
GLW_CreateWindow

Responsible for creating the Win32 window.
If fullscreen, it won't have a border
=======================
*/
static bool DX_CreateRenderWindow( const glimpParms_t & parms )
{
	int x, y, w, h;
	if( !DXW_GetWindowDimensions( parms, x, y, w, h ) )
	{
		return false;
	}

	int	stylebits;
	int	exstyle;
	if( parms.fullScreen != 0 )
	{
		exstyle = WS_EX_TOPMOST;
		stylebits = WS_POPUP | WS_VISIBLE | WS_SYSMENU;
	}
	else {
		exstyle = 0;
		stylebits = WINDOW_STYLE | WS_SYSMENU;
	}

	win32.hWnd = ::CreateWindowEx( exstyle, WIN32_WINDOW_CLASS_NAME, GAME_NAME, stylebits,
						x, y, w, h, NULL, NULL, ( HINSTANCE )::GetModuleHandle( nullptr ), NULL );
	if( !win32.hWnd )
	{
		common->Printf( S_COLOR_YELLOW "DX_CreateRenderWindow() - Couldn't create window" S_COLOR_DEFAULT "\n" );
		return false;
	}

	::SetTimer( win32.hWnd, 0, 100, NULL );

	::ShowWindow( win32.hWnd, SW_SHOW );
	::UpdateWindow( win32.hWnd );
	common->Printf( "...created window %i,%i(%ix%i)\n", x, y, w, h );

	////////////////////////////////////////////////

	if( !DX_CreateDevice( true, g_MainRenderContext.hDevice, g_MainRenderContext.hIContext ) )
	{
		g_MainRenderContext.Release();
		return false;
	}
	if( !DX_CreateSwapchain( DXInterface.GetFactoryHandle(),
		g_MainRenderContext.hDevice, g_MainRenderContext.backBuffer,
		win32.hWnd, 0, 0, false, false, false ) )
	{
		g_MainRenderContext.Release();
		return false;
	}

	glConfig.stereoPixelFormatAvailable = false;
	glConfig.colorBits = 32;
	glConfig.depthBits = 24;
	glConfig.stencilBits = 8;

	//FLOAT clearColor[] = { ( FLOAT )0.0f, ( FLOAT )0.0f, ( FLOAT )0.0f, ( FLOAT )0.0f };
	//g_MainRenderContext.hIContext->ClearRenderTargetView( g_MainRenderContext.backBuffer.colorView, clearColor );

	::SetForegroundWindow( win32.hWnd );
	::SetFocus( win32.hWnd );

	glConfig.isFullscreen = parms.fullScreen;

	return true;
}


/*
===================
 GLimp_Init

  This is the platform specific OpenGL initialization function.  It
  is responsible for loading OpenGL, initializing it,
  creating a window of the appropriate size, doing
  fullscreen manipulations, etc.  Its overall responsibility is
  to make sure that a functional OpenGL subsystem is operating
  when it returns to the ref.

  If there is any failure, the renderer will revert back to safe
  parameters and try again.
===================
*/
bool DXimp_Init( const glimpParms_t & parms )
{
	//cmdSystem->AddCommand( "testSwapBuffers", GLimp_TestSwapBuffers, CMD_FL_SYSTEM, "Times swapbuffer options" );

	common->Printf( "Initializing DirectX subsystem with multisamples:%i stereo:%i fullscreen:%i\n", parms.multiSamples, parms.stereo, parms.fullScreen );

	// check our desktop attributes
	{
		HDC hDC = ::GetDC( ::GetDesktopWindow() );
		win32.desktopBitsPixel = ::GetDeviceCaps( hDC, BITSPIXEL );
		win32.desktopWidth = ::GetDeviceCaps( hDC, HORZRES );
		win32.desktopHeight = ::GetDeviceCaps( hDC, VERTRES );
		::ReleaseDC( ::GetDesktopWindow(), hDC );
	}
	// we can't run in a window unless it is 32 bpp
	if( win32.desktopBitsPixel < 32 && parms.fullScreen <= 0 )
	{
		common->Printf( S_COLOR_YELLOW "Windowed mode requires 32 bit desktop depth" S_COLOR_DEFAULT "\n" );
		return false;
	}

	// create our window classes if we haven't already
	GLW_CreateWindowClasses();

	DXInterface.Init();

	// Optionally ChangeDisplaySettings to get a different fullscreen resolution.
	extern bool GLW_ChangeDislaySettingsIfNeeded( const glimpParms_t & );
	if( !GLW_ChangeDislaySettingsIfNeeded( parms ) )
	{
		GLimp_Shutdown();
		return false;
	}

	// try to create a window with the correct pixel format
	// and init the renderer context
	if( !DX_CreateRenderWindow( parms ) )
	{
		GLimp_Shutdown();
		return false;
	}

	glConfig.isFullscreen = parms.fullScreen;
	glConfig.isStereoPixelFormat = parms.stereo;
	glConfig.nativeScreenWidth = parms.width;
	glConfig.nativeScreenHeight = parms.height;
	glConfig.multisamples = parms.multiSamples;

	glConfig.pixelAspect = 1.0f;	// FIXME: some monitor modes may be distorted
									// should side-by-side stereo modes be consider aspect 0.5?

	// get the screen size, which may not be reliable...
	// If we use the windowDC, I get my 30" monitor, even though the window is
	// on a 27" monitor, so get a dedicated DC for the full screen device name.
	extern idStr GetDeviceName( const int );
	idStr deviceName = GetDeviceName( idMath::Max( 0, parms.GetOutputIndex() ) );
	HDC deviceDC = ::CreateDC( deviceName.c_str(), deviceName.c_str(), NULL, NULL );
	const int mmWide = ::GetDeviceCaps( win32.hDC, HORZSIZE );
	::DeleteDC( deviceDC );
	glConfig.physicalScreenWidthInCentimeters = ( mmWide == 0 )? 100.0f : ( 0.1f * mmWide );

	return true;
}
/*
===================
GLimp_Shutdown

This routine does all OS specific shutdown procedures for the OpenGL
subsystem.
===================
*/
void DXimp_Shutdown()
{
	common->Printf( "Shutting down DirecX subsystem\n" );

	g_MainRenderContext.Release();

	// destroy window
	if( win32.hWnd )
	{
		common->Printf( "...destroying window\n" );
		::ShowWindow( win32.hWnd, SW_HIDE );
		::DestroyWindow( win32.hWnd );
		win32.hWnd = NULL;
	}

	// reset display settings
	if( win32.cdsFullscreen )
	{
		common->Printf( "...resetting display\n" );
		::ChangeDisplaySettings( 0, 0 );
		win32.cdsFullscreen = 0;
	}

	DXInterface.Shutdown();
}





void DX_Test()
{
	DXInterface.Init();

	dxRenderContext_t rc;
	if( DX_CreateDevice( true, rc.hDevice, rc.hIContext ) )
	{
		DX_CreateSwapchain( DXInterface.GetFactoryHandle(), rc.hDevice, rc.backBuffer,
			win32.hWnd, renderSystem->GetWidth(), renderSystem->GetHeight(), false, false, false );
	}
	rc.Release();

	DXInterface.Shutdown();
}