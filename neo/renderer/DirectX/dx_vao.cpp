#pragma hdrstop
#include "precompiled.h"

#include "../tr_local.h"

#include "dx_header.h"

//#include <d3d11shader.h>
//#include <d3dcompiler.h>

//#include <string>
//#include <vector>

/*
=====================================================================================================
	Contains the VAO implementation for DirecX
=====================================================================================================
*/

extern ID3DBlob * DXCompileShader( const char shaderName[], const char shaderSrcBytes[], shaderType_e );


DXInputLayout * g_layouts[ LAYOUT_MAX - 1 ] = { nullptr };

void DXShutdownVertexLayouts()
{
	for( uint32 i = 0; i < _countof( g_layouts ); i++ )
	{
		SAFE_RELEASE( g_layouts[ i ] );
	}
}

void DXInitVertexLayouts()
{
	auto device = GetCurrentDevice();

	// idDrawVert
	{
		const char * shaderSrcBytes = {
			"struct VS_IN {\n"
			"	float4 position : VS_IN_POSITION;\n"
			"	float2 texcoord : VS_IN_TEXCOORD;\n"
			"	float4 normal : VS_IN_NORMAL;\n"
			"	float4 tangent : VS_IN_TANGENT;\n"
			"	float4 color : VS_IN_COLOR0;\n"
			"	float4 color2 : VS_IN_COLOR1;\n"
			"};\n"
			"struct VS_OUT {\n"
			"	float4 position : SV_Position;\n"
			"	float2 texcoord0 : TEXCOORD0;\n"
			"	float4 normal : NORMAL;\n"
			"	float4 tangent : TANGENT;\n"
			"	float4 color : COLOR;\n"
			"	float4 color2 : COLOR2;\n"
			"};\n"
			"void VSMain( in VS_IN input, out VS_OUT output )\n"
			"{\n"
			"	output.position = input.position;\n"
			"	output.texcoord0 = input.texcoord;\n"
			"	output.normal = input.normal;\n"
			"	output.tangent = input.tangent;\n"
			"	output.color = input.color;\n"
			"	output.color2 = input.color2;\n"
			"}\n"
		};
		ID3DBlob * blob = DXCompileShader( "DummyVS_LAYOUT_DRAW_VERT_FULL", shaderSrcBytes, SHT_VERTEX );
		if( !blob ) {
			common->FatalError( "Failed DXCompileShader( DummyVS_LAYOUT_DRAW_VERT_FULL )" );
		}
		D3D11_INPUT_ELEMENT_DESC desc[] =
		{
			{ "VS_IN_POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, PC_ATTRIB_INDEX_POSITION,	D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "VS_IN_TEXCOORD",	 0, DXGI_FORMAT_R16G16_FLOAT,	 PC_ATTRIB_INDEX_ST,		D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "VS_IN_NORMAL",	 0, DXGI_FORMAT_R8G8B8A8_UNORM,  PC_ATTRIB_INDEX_NORMAL,	D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "VS_IN_TANGENT",	 0, DXGI_FORMAT_R8G8B8A8_UNORM,  PC_ATTRIB_INDEX_TANGENT,	D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "VS_IN_COLOR0",	 0, DXGI_FORMAT_R8G8B8A8_UNORM,  PC_ATTRIB_INDEX_COLOR,		D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "VS_IN_COLOR1",	 0, DXGI_FORMAT_R8G8B8A8_UNORM,  PC_ATTRIB_INDEX_COLOR2,	D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		auto hr = device->CreateInputLayout( desc, _countof( desc ), blob->GetBufferPointer(), blob->GetBufferSize(), &g_layouts[ LAYOUT_DRAW_VERT_FULL - 1 ] );

		SAFE_RELEASE( blob );

		if( FAILED( hr ) ) {
			common->FatalError( "Failed CreateInputLayout( LAYOUT_DRAW_VERT_FULL ): %s", WinErrStr( hr ) );
		}
	}
	// idDrawVert
	{
		const char * shaderSrcBytes = {
			"struct VS_IN {\n"
			"	float4 position : VS_IN_POSITION;\n"
			"};\n"
			"struct VS_OUT {\n"
			"	float4 position : SV_Position;\n"
			"};\n"
			"void VSMain( in VS_IN input, out VS_OUT output )\n"
			"{\n"
			"	output.position = input.position;\n"
			"}\n"
		};
		ID3DBlob * blob = DXCompileShader( "DummyVS_LAYOUT_DRAW_VERT_POSITION_ONLY", shaderSrcBytes, SHT_VERTEX );
		if( !blob ) {
			common->FatalError( "Failed DXCompileShader( DummyVS_LAYOUT_DRAW_VERT_POSITION_ONLY )" );
		}
		D3D11_INPUT_ELEMENT_DESC desc[] =
		{
			{ "VS_IN_POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, PC_ATTRIB_INDEX_POSITION,	D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		auto hr = device->CreateInputLayout( desc, _countof( desc ), blob->GetBufferPointer(), blob->GetBufferSize(), &g_layouts[ LAYOUT_DRAW_VERT_POSITION_ONLY - 1 ] );

		SAFE_RELEASE( blob );

		if( FAILED( hr ) ) {
			common->FatalError( "Failed CreateInputLayout( LAYOUT_DRAW_VERT_POSITION_ONLY ): %s", WinErrStr( hr ) );
		}
	}
	// idDrawVert
	{
		const char * shaderSrcBytes = {
			"struct VS_IN {\n"
			"	float4 position : VS_IN_POSITION;\n"
			"	float2 texcoord : VS_IN_TEXCOORD;\n"
			"};\n"
			"struct VS_OUT {\n"
			"	float4 position : SV_Position;\n"
			"	float2 texcoord0 : TEXCOORD0;\n"
			"};\n"
			"void VSMain( in VS_IN input, out VS_OUT output )\n"
			"{\n"
			"	output.position = input.position;\n"
			"	output.texcoord0 = input.texcoord;\n"
			"}\n"
		};
		ID3DBlob * blob = DXCompileShader( "DummyVS_LAYOUT_DRAW_VERT_POSITION_TEXCOORD", shaderSrcBytes, SHT_VERTEX );
		if( !blob ) {
			common->FatalError( "Failed DXCompileShader( DummyVS_LAYOUT_DRAW_VERT_POSITION_TEXCOORD )" );
		}
		D3D11_INPUT_ELEMENT_DESC desc[] =
		{
			{ "VS_IN_POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, PC_ATTRIB_INDEX_POSITION,	D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "VS_IN_TEXCOORD",	 0, DXGI_FORMAT_R16G16_FLOAT,	 PC_ATTRIB_INDEX_ST,		D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		auto hr = device->CreateInputLayout( desc, _countof( desc ), blob->GetBufferPointer(), blob->GetBufferSize(), &g_layouts[ LAYOUT_DRAW_VERT_POSITION_TEXCOORD - 1 ] );

		SAFE_RELEASE( blob );

		if( FAILED( hr ) ) {
			common->FatalError( "Failed CreateInputLayout( LAYOUT_DRAW_VERT_POSITION_TEXCOORD ): %s", WinErrStr( hr ) );
		}
	}

	// idShadowVert
	{
		const char * shaderSrcBytes = {
			"struct VS_IN {\n"
			"	float4 position : VS_IN_POSITION;\n"
			"};\n"
			"struct VS_OUT {\n"
			"	float4 position : SV_Position;\n"
			"};\n"
			"void VSMain( in VS_IN input, out VS_OUT output ) {\n"
			"	output.position = input.position;\n"
			"}\n"
		};
		ID3DBlob * blob = DXCompileShader( "DummyVS_LAYOUT_DRAW_SHADOW_VERT", shaderSrcBytes, SHT_VERTEX );
		if( !blob ) {
			common->FatalError( "Failed DXCompileShader( DummyVS_LAYOUT_DRAW_SHADOW_VERT )" );
		}
		D3D11_INPUT_ELEMENT_DESC desc[] =
		{
			{ "VS_IN_POSITION",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, PC_ATTRIB_INDEX_POSITION,	D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		auto hr = device->CreateInputLayout( desc, _countof( desc ), blob->GetBufferPointer(), blob->GetBufferSize(), &g_layouts[ LAYOUT_DRAW_SHADOW_VERT - 1 ] );

		SAFE_RELEASE( blob );

		if( FAILED( hr ) ) {
			common->FatalError( "Failed CreateInputLayout( LAYOUT_DRAW_SHADOW_VERT ): %s", WinErrStr( hr ) );
		}
	}

	// idShadowVertSkinned
	{
		const char * shaderSrcBytes = {
			"struct VS_IN {\n"
			"	float4 position : VS_IN_POSITION;\n"
			"	float4 color 	: VS_IN_COLOR0;\n"
			"	float4 color2 	: VS_IN_COLOR1;\n"
			"};\n"
			"struct VS_OUT {\n"
			"	float4 position : SV_Position;\n"
			"	float4 color 	: COLOR;\n"
			"	float4 color2 	: COLOR2;\n"
			"};\n"
			"void VSMain( in VS_IN input, out VS_OUT output ) {\n"
			"	output.position = input.position;\n"
			"	output.color 	= input.color;\n"
			"	output.color2 	= input.color2;\n"
			"}\n"
		};
		ID3DBlob * blob = DXCompileShader( "DummyVS_LAYOUT_DRAW_SHADOW_VERT", shaderSrcBytes, SHT_VERTEX );
		if( !blob ) {
			common->FatalError( "Failed DXCompileShader( DummyVS_LAYOUT_DRAW_SHADOW_VERT )" );
		}
		D3D11_INPUT_ELEMENT_DESC desc[] =
		{
			{ "VS_IN_POSITION",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, PC_ATTRIB_INDEX_POSITION,	D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "VS_IN_COLOR0",	 0, DXGI_FORMAT_R8G8B8A8_UNORM,		PC_ATTRIB_INDEX_COLOR,		D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "VS_IN_COLOR1",	 0, DXGI_FORMAT_R8G8B8A8_UNORM,		PC_ATTRIB_INDEX_COLOR2,		D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		auto hr = device->CreateInputLayout( desc, _countof( desc ), blob->GetBufferPointer(), blob->GetBufferSize(), &g_layouts[ LAYOUT_DRAW_SHADOW_VERT_SKINNED - 1 ] );

		SAFE_RELEASE( blob );

		if( FAILED( hr ) ) {
			common->FatalError( "Failed CreateInputLayout( LAYOUT_DRAW_SHADOW_VERT_SKINNED ): %s", WinErrStr( hr ) );
		}
	}

}

DXInputLayout * GetVertexLayout( vertexLayoutType_t vertexLayout )
{
	return g_layouts[ vertexLayout - 1 ];
}











#if 0
// Creates an input layout from the vertex shader, after compilation.
// Input layout can be reused with any vertex shaders that use the same input layout.
HRESULT CreateInputLayoutDescFromVertexShaderSignature( ID3DBlob* pShaderBlob, ID3D11Device* pD3DDevice, ID3D11InputLayout** pInputLayout, int* inputLayoutByteLength )
{
	// Reflect shader info
	ID3D11ShaderReflection* pVertexShaderReflection = nullptr;
	if( FAILED( ::D3DReflect( pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), IID_ID3D11ShaderReflection, ( void** )&pVertexShaderReflection ) ) )
	{
		return S_FALSE;
	}

	// get shader description
	D3D11_SHADER_DESC shaderDesc = {};
	pVertexShaderReflection->GetDesc( &shaderDesc );

	// Read input layout description from shader info
	UINT byteOffset = 0;
	idStaticList<D3D11_INPUT_ELEMENT_DESC, 32> inputLayoutDesc;
	for( UINT i = 0; i< shaderDesc.InputParameters; ++i )
	{
		D3D11_SIGNATURE_PARAMETER_DESC paramDesc = {};
		pVertexShaderReflection->GetInputParameterDesc( i, &paramDesc );

		// create input element desc
		D3D11_INPUT_ELEMENT_DESC elementDesc;
		elementDesc.SemanticName = paramDesc.SemanticName;
		elementDesc.SemanticIndex = paramDesc.SemanticIndex;
		elementDesc.InputSlot = 0;
		elementDesc.AlignedByteOffset = byteOffset;
		elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate = 0;

		// determine DXGI format
		/*/*/if( paramDesc.Mask == 1 )
		{
			/*/*/if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 ) elementDesc.Format = DXGI_FORMAT_R32_UINT;
			else if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 ) elementDesc.Format = DXGI_FORMAT_R32_SINT;
			else if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 ) elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
			byteOffset += 4;
		}
		else if( paramDesc.Mask <= 3 )
		{
			/*/*/if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 ) elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
			else if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 ) elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
			else if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 ) elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
			byteOffset += 8;
		}
		else if( paramDesc.Mask <= 7 )
		{
			/*/*/if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 ) elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
			else if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 ) elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
			else if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 ) elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
			byteOffset += 12;
		}
		else if( paramDesc.Mask <= 15 )
		{
			/*/*/if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 ) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
			else if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 ) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
			else if( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 ) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			byteOffset += 16;
		}

		//save element desc
		inputLayoutDesc.Append( elementDesc );
	}

	// Try to create Input Layout
	auto hr = pD3DDevice->CreateInputLayout( inputLayoutDesc.Ptr(), inputLayoutDesc.Num(), pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), pInputLayout );

	//Free allocation shader reflection memory
	pVertexShaderReflection->Release();

	//record byte length
	*inputLayoutByteLength = byteOffset;

	return hr;
}
#endif
