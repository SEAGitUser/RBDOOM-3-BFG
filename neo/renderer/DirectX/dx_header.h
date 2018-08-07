#ifndef __D3D_COMMON_H__
#define __D3D_COMMON_H__

#ifndef __cplusplus
	#error This must only be inluded from C++ source files.
#endif

#include <d3d11_1.h>

typedef IDXGIFactory2				DXFactory;
typedef IDXGIAdapter2				DXAdapter;
typedef ID3D11Device1				DXDevice;
typedef ID3D11DeviceChild			DXDeviceChild;
typedef ID3D11DeviceContext1		DXDeviceContext;
typedef ID3D11Texture2D				DXTexture2D;
typedef ID3D11Texture3D				DXTexture3D;
typedef ID3D11Resource				DXResource;
typedef ID3D11View					DXView;
typedef ID3D11ShaderResourceView	DXShaderResourceView;
typedef ID3D11RenderTargetView		DXRenderTargetView;
typedef ID3D11DepthStencilView		DXDepthStencilView;
typedef ID3D11SamplerState			DXSamplerState;
typedef ID3D11InputLayout			DXInputLayout;
typedef ID3D11VertexShader 			DXVertexShader;
typedef ID3D11HullShader 			DXHullShader;
typedef ID3D11DomainShader 			DXDomainShader;
typedef ID3D11GeometryShader		DXGeometryShader;
typedef ID3D11PixelShader 			DXPixelShader;
typedef ID3D11ComputeShader			DXComputeShader;
typedef ID3D11Query					DXQuery;
typedef ID3D11Buffer				DXBuffer;
typedef ID3D11BlendState			DXBlendState;
typedef ID3D11DepthStencilState		DXDepthStencilState;
typedef ID3D11RasterizerState		DXRasterizerState;

typedef D3D11_BOX						DX_BOX;
typedef D3D11_SAMPLER_DESC				DX_SAMPLER_DESC;
typedef D3D11_SHADER_RESOURCE_VIEW_DESC	DX_SHADER_RESOURCE_VIEW_DESC;
typedef D3D11_RENDER_TARGET_VIEW_DESC	DX_RENDER_TARGET_VIEW_DESC;
typedef D3D11_DEPTH_STENCIL_VIEW_DESC	DX_DEPTH_STENCIL_VIEW_DESC;
typedef D3D11_TEXTURE2D_DESC			DX_TEXTURE2D_DESC;
typedef D3D11_TEXTURE3D_DESC			DX_TEXTURE3D_DESC;
typedef D3D11_SUBRESOURCE_DATA			DX_SUBRESOURCE_DATA;
typedef D3D11_RASTERIZER_DESC			DX_RASTERIZER_DESC;
typedef D3D11_BLEND_DESC				DX_BLEND_DESC;
typedef D3D11_DEPTH_STENCIL_DESC		DX_DEPTH_STENCIL_DESC;
typedef D3D11_BUFFER_DESC				DX_BUFFER_DESC;
typedef D3D11_MAPPED_SUBRESOURCE		DX_MAPPED_SUBRESOURCE;
typedef D3D11_INPUT_ELEMENT_DESC		DX_INPUT_ELEMENT_DESC;
typedef D3D11_QUERY_DESC				DX_QUERY_DESC;


inline UINT DXCalcSubresource( UINT MipSlice, UINT ArraySlice, UINT MipLevels )
{
	return MipSlice + ArraySlice * MipLevels;
}

//----------------------------------------------------------------------------
// Returns true if the debug layers are available
//----------------------------------------------------------------------------
BOOL IsSdkDebugLayerAvailable() {
#ifdef D3D_FORCE_DEBUG_LAYER
	return TRUE;
#else
	// Test by trying to create a null device with debug enabled.
	auto hr = D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_NULL, 0, D3D11_CREATE_DEVICE_DEBUG, nullptr, 0, D3D11_SDK_VERSION, nullptr, nullptr, nullptr );
	return SUCCEEDED( hr );
#endif
}

#ifndef SAFE_RELEASE
	#define SAFE_RELEASE( x ) if( x ) { x->Release(); x = nullptr; }
#endif

//#include <comdef.h>
//_com_error err( hr );
//LPCTSTR errMsg = err.ErrorMessage();

inline DWORD Win32FromHResult( HRESULT hr )
{
	if( ( hr & 0xFFFF0000 ) == MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, 0 ) )
		return HRESULT_CODE( hr );

	if( hr == S_OK )
		return ERROR_SUCCESS;

	// Not a Win32 HRESULT so return a generic error code.
	return ERROR_CAN_NOT_COMPLETE;
}
LPTSTR Win_GetErrorStr( HRESULT nError, LPTSTR pStr, WORD wLength )
{
	LPTSTR szBuffer = pStr;
	DWORD nBufferSize = wLength;
	DWORD nErrorCode = Win32FromHResult( nError );

	// prime buffer with error code
	wsprintf( szBuffer, TEXT( "ECode %i" ), nErrorCode );

	// if we have a message, replace default with msg.
	::FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, nErrorCode,
		MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), // Default language
		( LPTSTR )szBuffer, nBufferSize, NULL );
/*
	LPVOID pText = 0;
	::FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, nErrorCode, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), ( LPWSTR )&pText, 0, NULL );
	///MessageBox( NULL, ( LPCWSTR )pText, 0, MB_OK );
	::LocalFree( pText );
*/
	return pStr;
}

#ifndef CHECK_FE
	#define CHECK_FE( x ) { HRESULT res = ( x ); if( FAILED( res ) ) { \
char buffer[512] = {}; \
common->FatalError( "DXError: %s", Win_GetErrorStr( res, buffer, _countof( buffer ) )); \
}
#endif

#ifndef CHECK_E
#define CHECK_E( x ) { HRESULT res = ( x ); if( FAILED( res ) ) { \
char buffer[512] = {}; \
common->Error( "DXError: %s", Win_GetErrorStr( res, buffer, _countof( buffer ) )); \
}
#endif

#include "dx_error.h"

//----------------------------------------------------------------------------
// Set a resource's debug name
//----------------------------------------------------------------------------
inline void D3DSetDebugObjectName( _In_ DXDeviceChild * resource, _In_z_ const char* name )
{
#if !defined( ID_RETAIL )
	if( resource != nullptr ) {
		resource->SetPrivateData( WKPDID_D3DDebugObjectName, strlen( name ), name );
	}
#endif
}

inline const char * GetFeatureLevelStr( D3D_FEATURE_LEVEL featureLevel )
{
	switch( featureLevel )
	{
		case D3D_FEATURE_LEVEL_10_0: return "v10.0 (Compatibility)";
		case D3D_FEATURE_LEVEL_10_1: return "v10.1 (Compatibility)";
		case D3D_FEATURE_LEVEL_11_0: return "v11.0";
		case D3D_FEATURE_LEVEL_11_1: return "v11.1";
	}
	return "Unknown Version";
}


struct dxTextureObject_t
{
	DXResource * res = nullptr; // DXTexture2D / DXTexture3D
	DXView * view = nullptr; // DXShaderResourceView
	DXSamplerState * smp = nullptr; //SEA: todo sampler manager
};
ID_INLINE const dxTextureObject_t * GetDXTexture( idImage * img ) {
	return static_cast< const dxTextureObject_t *>( img->GetAPIObject() );
}

struct dxFramebuffer_t
{
	DXRenderTargetView * colorViews[ 8 ] = { nullptr };
	DXDepthStencilView * dsView = nullptr;
	DXDepthStencilView * dsReadOnlyView = nullptr;
};
ID_INLINE const dxFramebuffer_t * GetDXFramebuffer( const idRenderDestination * dest ) {
	return static_cast<const dxFramebuffer_t *>( dest->GetAPIObject() );
}

struct dxProgramObject_t
{
	DXDeviceChild * shaders[ shaderType_e::SHT_MAX ] = { nullptr };

	DXInputLayout * inputLayout = nullptr;

	idStaticList< int16, 128> vecParmsUsed[ shaderType_e::SHT_MAX ];
	idStaticList< int16, 16> texParmsUsed[ shaderType_e::SHT_MAX ];

	// SHT_TESS_CTRL
	bool HasHS() const { return( shaders[ shaderType_e::SHT_TESS_CTRL ] != nullptr ); }
	// SHT_TESS_EVAL
	bool HasDS() const { return( shaders[ shaderType_e::SHT_TESS_EVAL ] != nullptr ); }

	bool HasGS() const { return( shaders[ shaderType_e::SHT_GEOMETRY ] != nullptr ); }

	bool VSHasTexParms() const { return( texParmsUsed[ shaderType_e::SHT_VERTEX ].Num() > 0 ); }
	bool HSHasTexParms() const { return( texParmsUsed[ shaderType_e::SHT_TESS_CTRL ].Num() > 0 ); }
	bool DSHasTexParms() const { return( texParmsUsed[ shaderType_e::SHT_TESS_EVAL ].Num() > 0 ); }
	bool GSHasTexParms() const { return( texParmsUsed[ shaderType_e::SHT_GEOMETRY ].Num() > 0 ); }
	bool PSHasTexParms() const { return( texParmsUsed[ shaderType_e::SHT_FRAGMENT ].Num() > 0 ); }

	bool VSHasVecParms() const { return( vecParmsUsed[ shaderType_e::SHT_VERTEX ].Num() > 0 ); }
	bool HSHasVecParms() const { return( vecParmsUsed[ shaderType_e::SHT_TESS_CTRL ].Num() > 0 ); }
	bool DSHasVecParms() const { return( vecParmsUsed[ shaderType_e::SHT_TESS_EVAL ].Num() > 0 ); }
	bool GSHasVecParms() const { return( vecParmsUsed[ shaderType_e::SHT_GEOMETRY ].Num() > 0 ); }
	bool PSHasVecParms() const { return( vecParmsUsed[ shaderType_e::SHT_FRAGMENT ].Num() > 0 ); }
};
ID_INLINE dxProgramObject_t * GetDXProgram( const idDeclRenderProg * prog ) {
	return static_cast<dxProgramObject_t *>( prog->GetAPIObject() );
}

DXInputLayout * GetVertexLayout( vertexLayoutType_t );






DXDeviceContext * GetCurrentImContext()
{
	return NULL;
}
DXDevice * GetCurrentDevice()
{
	return NULL;
}




#endif /*__D3D_COMMON_H__*/