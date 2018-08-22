#pragma hdrstop
#include "precompiled.h"

#include "../tr_local.h"

#include "dx_header.h"

#pragma comment( lib, "d3dcompiler.lib" )

#include <d3d11shader.h>
#include <d3dcompiler.h>

#include <string>
#include <vector>

/*
=====================================================================================================
				Contains the program object implementation for DirecX
=====================================================================================================
*/

idCVar r_dxShaderCompileOptLevel("r_dxShaderCompileOptLevel", "3", CVAR_INTEGER, "Sets the shader compile opt level(1-low,2-mid,3-high)", 1.0, 3.0 );

const char * shaderProfile[][ SHT_MAX + 1 ] = {
	{ "vs_5_0", "hs_5_0", "ds_5_0", "gs_5_0", "ps_5_0", "cs_5_0" }, // Direct3D 11.0 and 11.1 feature levels
	{ "vs_4_1", "......", "......", "gs_4_1", "ps_4_1", "cs_4_1" }, // Direct3D 10.1 feature level
	{ "vs_4_0", "......", "......", "gs_4_0", "ps_4_0", "cs_4_0" }, // Direct3D 10.0 feature level
};
const char * shaderEntry[ SHT_MAX + 1 ] = { "VSMain", "HSMain", "DSMain", "GSMain", "PSMain", "CSMain" };

ID3DBlob * DXCompileShader( const char shaderName[], const char shaderSrcBytes[], shaderType_e sht )
{
	bool bUseRowMajorMatrices = false;
	bool bUseHalfPrecision = false;

	UINT flags1 = D3DCOMPILE_OPTIMIZATION_LEVEL1;
	if( r_dxShaderCompileOptLevel.GetInteger() == 2 ) {
		flags1 = D3DCOMPILE_OPTIMIZATION_LEVEL2;
	} else if( r_dxShaderCompileOptLevel.GetInteger() == 3 ) {
		flags1 = D3DCOMPILE_OPTIMIZATION_LEVEL3;
	}

	if( r_debugContext.GetInteger() > 0 ) {
		flags1 |= D3DCOMPILE_DEBUG;
	}
	flags1 |= ( bUseRowMajorMatrices ? D3DCOMPILE_PACK_MATRIX_ROW_MAJOR : D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR );

	if( bUseHalfPrecision ) {
		flags1 |= D3DCOMPILE_PARTIAL_PRECISION;
	}

	auto device = GetCurrentDevice();

	UINT profile = 2;
	auto fl = device->GetFeatureLevel();
	if( fl == D3D_FEATURE_LEVEL_11_1 || fl == D3D_FEATURE_LEVEL_11_0 )
	{
		profile = 0;
	}
	else if( fl == D3D_FEATURE_LEVEL_10_1 )
	{
		profile = 1;
	}

	ID3DBlob * blob = nullptr;
	ID3DBlob * msgs = nullptr;

	auto res = ::D3DCompile( shaderSrcBytes, strlen( shaderSrcBytes ), shaderName, nullptr, nullptr,
					 shaderEntry[ sht ], shaderProfile[ profile ][ sht ], flags1, 0, &blob, &msgs );

	if( SUCCEEDED( res ) )
	{
		if( msgs )
		{
			// The message is a 'warning' from the HLSL compiler.
			const char * message = static_cast<const char*>( msgs->GetBufferPointer() );

			// Write the warning to the output window when the program is
			// executing through the Microsoft Visual Studio IDE.
			/*const size_t length = strlen( message );
			std::wstring output = L"";
			for( size_t i = 0; i < length; ++i )
			{
			output += static_cast<wchar_t>( message[ i ] );
			}
			output += L'\n';
			::OutputDebugStringW( output.c_str() );*/
		#if defined( _WIN32 )
			::OutputDebugString( message );
			::OutputDebugString( "\n" );
		#else
			printf( "%s\n", message );
		#endif
			idLib::Warning( "D3DCompile warning:\n%s", message );
		}
	}
	else // FAILED( res )
	{
		if( msgs )
		{
			// The message is an 'error' from the HLSL compiler.
			const char * message = static_cast<const char*>( msgs->GetBufferPointer() );
			const size_t length = strlen( message );

			// Write the error to the output window when the program is
			// executing through the Microsoft Visual Studio IDE.
			/*std::wstring output = L"";
			for( size_t i = 0; i < length; ++i )
			{
			output += static_cast<wchar_t>( message[ i ] );
			}
			output += L'\n';
			::OutputDebugString( output.c_str() );*/
		#if defined( _WIN32 )
			::OutputDebugString( message );
			::OutputDebugString( "\n" );
		#else
			printf( "%s\n", message );
		#endif
			idLib::Warning( "D3DCompile error:\n%s", message );
		}
		else // Unknown reason for error.
		{
			idLib::Warning( "D3DCompile error: %s", WinErrStr( res ) );
		}
	}

	if( msgs ) {
		msgs->Release();
	}

	return blob;
}

static DXDeviceChild * DXCreateShader( DXDevice * device, shaderType_e sht, ID3DBlob * blob, const char * name )
{
	switch( sht )
	{
		case SHT_VERTEX:
		{
			DXVertexShader * pVertexShader = nullptr;
			auto res = device->CreateVertexShader( blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &pVertexShader );
			if( FAILED( res ) ) {
				blob->Release();
				common->Error( "Failed CreateVertexShader( %s ): %s", name, WinErrStr( res ) );
			}
			return pVertexShader;
		}

		case SHT_TESS_CTRL:
		{
			DXHullShader * pHullShader = nullptr;
			auto res = device->CreateHullShader( blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &pHullShader );
			blob->Release();
			if( FAILED( res ) ) {
				blob->Release();
				common->Error( "Failed CreateHullShader( %s ): %s", name, WinErrStr( res ) );
			}
			return pHullShader;
		}

		case SHT_TESS_EVAL:
		{
			DXDomainShader * pDomainShader = nullptr;
			auto res = device->CreateDomainShader( blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &pDomainShader );
			blob->Release();
			if( FAILED( res ) ) {
				blob->Release();
				common->Error( "Failed CreateDomainShader( %s ): %s", name, WinErrStr( res ) );
			}
			return pDomainShader;
		}

		case SHT_GEOMETRY:
		{
			DXGeometryShader * pGeometryShader = nullptr;
			auto res = device->CreateGeometryShader( blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &pGeometryShader );
			blob->Release();
			if( FAILED( res ) ) {
				blob->Release();
				common->Error( "Failed CreateGeometryShader( %s ): %s", name, WinErrStr( res ) );
			}
			return pGeometryShader;
		}

		case SHT_FRAGMENT:
		{
			DXPixelShader * pPixelShader = nullptr;
			auto res = device->CreatePixelShader( blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &pPixelShader );
			blob->Release();
			if( FAILED( res ) ) {
				blob->Release();
				common->Error( "Failed CreatePixelShader( %s ): %s", name, WinErrStr( res ) );
			}
			return pPixelShader;
		}

		case SHT_COMPUTE:
		{
			DXComputeShader * pComputeShader = nullptr;
			auto res = device->CreateComputeShader( blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &pComputeShader );
			blob->Release();
			if( FAILED( res ) ) {
				blob->Release();
				common->Error( "Failed CreateComputeShader( %s ): %s", name, WinErrStr( res ) );
			}
			return pComputeShader;
		}
	}

	assert( "Unknown SHT" );
	return nullptr;
}

dxProgramObject_t * DX_CreateProgram()
{
	//auto iContext = GetCurrentImContext();
	auto device = GetCurrentDevice();

	dxProgramObject_t dxProg = {};
	bool bTessellationShadersAvailable;
	bool bComputeShaderAvailable;

	const char * shaderNames[ SHT_MAX + 1 ]; // "shader.hlsl"
	const char * shaderSrcBytes[ SHT_MAX + 1 ];

	for( int i = 0; i < SHT_MAX; i++ )
	{
		ID3DBlob * blob = DXCompileShader( shaderNames[ i ], shaderSrcBytes[ i ], ( shaderType_e )i );
		if( !blob ) {
			common->Error( "Failed DXCompileShader( %s ): %s", shaderNames[ i ] );
		}

		dxProg.shaders[ i ] = DXCreateShader( device, ( shaderType_e )i, blob, shaderNames[ i ] );

		SAFE_RELEASE( blob );
	}

	return new dxProgramObject_t( dxProg );
}

//#include <D3dx11effect.h>
#ifndef D3DX11_BYTES_FROM_BITS
  #define D3DX11_BYTES_FROM_BITS(x) (((x) + 7) / 8)
#endif // D3DX11_BYTES_FROM_BITS

ID_INLINE void SetupResources( const dxProgramObject_t * dxProg, const idDeclRenderProg * prog, shaderType_e shaderType,
	idStaticList<DXShaderResourceView *, 16> & ResourceViews, idStaticList<DXSamplerState *, 16> & Samplers )
{
	for( int i = 0; i < dxProg->texParmsUsed[ shaderType ].Num(); ++i )
	{
		auto parm = prog->GetTexParms()[ dxProg->texParmsUsed[ shaderType ][ i ] ];
		auto tex = static_cast<const dxTextureObject_t*>( parm->GetImage()->GetAPIObject() );

		ResourceViews.Append( ( DXShaderResourceView * )tex->view );
		Samplers.Append( tex->smp );
	}
}

void DX_CommitParms( const idDeclRenderProg * prog )
{
	// D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT
	// D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT
	// D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT

	auto iContext = GetCurrentImContext();
	auto dxProg = static_cast<const dxProgramObject_t *>( prog->GetAPIObject() );


	{
		if( dxProg->texParmsUsed[ SHT_VERTEX    ].Num() > 0 )
		{
			idStaticList<DXShaderResourceView *, 16 > ResourceViews;
			idStaticList<DXSamplerState *, 16 > Samplers;

			SetupResources( dxProg, prog, SHT_VERTEX, ResourceViews, Samplers );

			iContext->VSSetShaderResources( 0, ResourceViews.Num(), ResourceViews.Ptr() );
			iContext->VSSetSamplers( 0, Samplers.Num(), Samplers.Ptr() );
		}
		if( dxProg->texParmsUsed[ SHT_TESS_CTRL ].Num() > 0 )
		{
			idStaticList<DXShaderResourceView *, 16 > ResourceViews;
			idStaticList<DXSamplerState *, 16 > Samplers;

			SetupResources( dxProg, prog, SHT_TESS_CTRL, ResourceViews, Samplers );

			iContext->HSSetShaderResources( 0, ResourceViews.Num(), ResourceViews.Ptr() );
			iContext->VSSetSamplers( 0, Samplers.Num(), Samplers.Ptr() );
		}
		if( dxProg->texParmsUsed[ SHT_TESS_EVAL ].Num() > 0 )
		{
			idStaticList<DXShaderResourceView *, 16 > ResourceViews;
			idStaticList<DXSamplerState *, 16 > Samplers;

			SetupResources( dxProg, prog, SHT_TESS_EVAL, ResourceViews, Samplers );

			iContext->DSSetShaderResources( 0, ResourceViews.Num(), ResourceViews.Ptr() );
			iContext->VSSetSamplers( 0, Samplers.Num(), Samplers.Ptr() );
		}
		if( dxProg->texParmsUsed[ SHT_GEOMETRY  ].Num() > 0 )
		{
			idStaticList<DXShaderResourceView *, 16 > ResourceViews;
			idStaticList<DXSamplerState *, 16 > Samplers;

			SetupResources( dxProg, prog, SHT_GEOMETRY, ResourceViews, Samplers );

			iContext->GSSetShaderResources( 0, ResourceViews.Num(), ResourceViews.Ptr() );
			iContext->VSSetSamplers( 0, Samplers.Num(), Samplers.Ptr() );
		}
		if( dxProg->texParmsUsed[ SHT_FRAGMENT  ].Num() > 0 )
		{
			idStaticList<DXShaderResourceView *, 16 > ResourceViews;
			idStaticList<DXSamplerState *, 16 > Samplers;

			SetupResources( dxProg, prog, SHT_FRAGMENT, ResourceViews, Samplers );

			iContext->PSSetShaderResources( 0, ResourceViews.Num(), ResourceViews.Ptr() );
			iContext->VSSetSamplers( 0, Samplers.Num(), Samplers.Ptr() );
		}



	}

}








#if 0

bool IsTextureArray( D3D11_SRV_DIMENSION dim )
{
	return dim == D3D_SRV_DIMENSION_TEXTURE1DARRAY
		|| dim == D3D_SRV_DIMENSION_TEXTURE2DARRAY
		|| dim == D3D_SRV_DIMENSION_TEXTURE2DMSARRAY
		|| dim == D3D_SRV_DIMENSION_TEXTURECUBE
		|| dim == D3D_SRV_DIMENSION_TEXTURECUBEARRAY;
}
void WriteLog( const std::string & message )
{
	std::ofstream logfile( "hlslshaderfactory.log" );
	if( logfile )
	{
		logfile << message.c_str() << std::endl;
		logfile.close();
	}
}
bool GetTypes( ID3D11ShaderReflectionType* rtype, unsigned int numMembers, HLSLShaderType& stype )
{
	for( UINT i = 0; i < numMembers; ++i )
	{
		ID3D11ShaderReflectionType* memType = rtype->GetMemberTypeByIndex( i );
		char const* memTypeName = rtype->GetMemberTypeName( i );
		std::string memName( memTypeName ? memTypeName : "" );
		D3D11_SHADER_TYPE_DESC memTypeDesc;
		HRESULT hr = memType->GetDesc( &memTypeDesc );
		if( FAILED( hr ) )
		{
			WriteLog( "memType->GetDesc error, hr = " + std::to_string( hr ) );
			return false;
		}
		HLSLShaderType& child = stype.GetChild( i );
		child.SetName( memName );
		child.SetDescription( memTypeDesc );
		GetTypes( memType, memTypeDesc.Members, child );
	}
	return true;
}
bool GetVariables( ID3D11ShaderReflectionConstantBuffer* cbuffer,
	unsigned int numVariables,
	std::vector<HLSLBaseBuffer::Member>& members )
{
	for( UINT i = 0; i < numVariables; ++i )
	{
		ID3D11ShaderReflectionVariable* var = cbuffer->GetVariableByIndex( i );
		ID3D11ShaderReflectionType* varType = var->GetType();

		D3D11_SHADER_VARIABLE_DESC varDesc;
		HRESULT hr = var->GetDesc( &varDesc );
		if( FAILED( hr ) )
		{
			WriteLog( "var->GetDesc error, hr = " + std::to_string( hr ) );
			return false;
		}

		D3D11_SHADER_TYPE_DESC varTypeDesc;
		hr = varType->GetDesc( &varTypeDesc );
		if( FAILED( hr ) )
		{
			WriteLog( "varType->GetDesc error, hr = " + std::to_string( hr ) );
			return false;
		}

		// Get the top-level information about the shader variable.
		HLSLShaderVariable svar;
		svar.SetDescription( varDesc );

		// Get the type of the variable.  If this is a struct type, the
		// call recurses to build the type tree implied by the struct.
		HLSLShaderType stype;
		stype.SetName( svar.GetName() );
		stype.SetDescription( varTypeDesc );
		if( !GetTypes( varType, varTypeDesc.Members, stype ) )
		{
			return false;
		}

		members.push_back( std::make_pair( svar, stype ) );
	}
	return true;
}

bool GetDescription( DXShaderReflection * reflector, HLSLShader& shader )
{
	DX_SHADER_DESC desc;
	HRESULT hr = reflector->GetDesc( &desc );
	if( FAILED( hr ) )
	{
		WriteLog( "reflector->GetDesc error, hr = " + std::to_string( hr ) );
		return false;
	}
	shader.SetDescription( desc );
	return true;
}
bool GetInputs( DXShaderReflection* reflector, HLSLShader& shader )
{
	UINT const numInputs = shader.GetDescription().numInputParameters;
	for( UINT i = 0; i < numInputs; ++i )
	{
		DX_SIGNATURE_PARAMETER_DESC spDesc;
		HRESULT hr = reflector->GetInputParameterDesc( i, &spDesc );
		if( FAILED( hr ) )
		{
			WriteLog( "reflector->GetInputParameterDesc error, hr = " + std::to_string( hr ) );
			return false;
		}
		shader.InsertInput( HLSLParameter( spDesc ) );
	}
	return true;
}
bool GetOutputs( DXShaderReflection* reflector, HLSLShader& shader )
{
	UINT const numOutputs = shader.GetDescription().numOutputParameters;
	for( UINT i = 0; i < numOutputs; ++i )
	{
		DX_SIGNATURE_PARAMETER_DESC spDesc;
		HRESULT hr = reflector->GetOutputParameterDesc( i, &spDesc );
		if( FAILED( hr ) )
		{
			WriteLog( "reflector->GetOutputParameterDesc error, hr = " + std::to_string( hr ) );
			return false;
		}
		shader.InsertOutput( HLSLParameter( spDesc ) );
	}
	return true;
}
bool GetCBuffers( DXShaderReflection* reflector, HLSLShader& shader )
{
	UINT const numCBuffers = shader.GetDescription().numConstantBuffers;
	for( UINT i = 0; i < numCBuffers; ++i )
	{
		ID3D11ShaderReflectionConstantBuffer* cbuffer = reflector->GetConstantBufferByIndex( i );

		D3D11_SHADER_BUFFER_DESC cbDesc;
		HRESULT hr = cbuffer->GetDesc( &cbDesc );
		if( FAILED( hr ) )
		{
			WriteLog( "reflector->GetConstantBufferByIndex error, hr = " + std::to_string( hr ) );
			return false;
		}

		D3D11_SHADER_INPUT_BIND_DESC resDesc;
		hr = reflector->GetResourceBindingDescByName( cbDesc.Name, &resDesc );
		if( FAILED( hr ) )
		{
			WriteLog( "reflector->GetResourceBindingDescByName error, hr = " + std::to_string( hr ) );
			return false;
		}

		if( cbDesc.Type == D3D_CT_CBUFFER )
		{
			std::vector<HLSLBaseBuffer::Member> members;
			if( !GetVariables( cbuffer, cbDesc.Variables, members ) )
			{
				return false;
			}

			if( resDesc.BindCount == 1 )
			{
				shader.Insert( HLSLConstantBuffer( resDesc, cbDesc.Size, members ) );
			}
			else
			{
				for( UINT j = 0; j < resDesc.BindCount; ++j )
				{
					shader.Insert( HLSLConstantBuffer( resDesc, j, cbDesc.Size, members ) );
				}
			}
		}
		else if( cbDesc.Type == D3D_CT_TBUFFER )
		{
			std::vector<HLSLBaseBuffer::Member> members;
			if( !GetVariables( cbuffer, cbDesc.Variables, members ) )
			{
				return false;
			}

			if( resDesc.BindCount == 1 )
			{
				shader.Insert( HLSLTextureBuffer( resDesc, cbDesc.Size, members ) );
			}
			else
			{
				for( UINT j = 0; j < resDesc.BindCount; ++j )
				{
					shader.Insert( HLSLTextureBuffer( resDesc, j, cbDesc.Size, members ) );
				}
			}
		}
		else if( cbDesc.Type == D3D_CT_RESOURCE_BIND_INFO )
		{
			std::vector<HLSLBaseBuffer::Member> members;
			if( !GetVariables( cbuffer, cbDesc.Variables, members ) )
			{
				return false;
			}

			if( resDesc.BindCount == 1 )
			{
				shader.Insert( HLSLResourceBindInfo( resDesc, cbDesc.Size, members ) );
			}
			else
			{
				for( UINT j = 0; j < resDesc.BindCount; ++j )
				{
					shader.Insert( HLSLResourceBindInfo( resDesc, j, cbDesc.Size, members ) );
				}
			}
		}
		else  // cbDesc.Type == D3D_CT_INTERFACE_POINTERS
		{
			WriteLog( "Interface pointers are not yet supported in GTEngine." );
			return false;
		}
	}
	return true;
}
bool GetBoundResources( DXShaderReflection* reflector, HLSLShader& shader )
{
	// TODO: It appears that D3DCompile never produces a resource with a bind
	// count larger than 1.  For example, "Texture2D texture[2];" comes
	// through with names "texture[0]" and "texture[1]", each having a bind
	// count of 1 (with consecutive bind points).  So it appears that passing
	// a bind count in D3D interfaces that is larger than 1 is something a
	// programmer can do only if he manually combines the texture resources
	// into his own data structure.  If the statement about D3DCompiler is
	// true, we do not need the loops with upper bound resDesc.BindCount.

	UINT const numResources = shader.GetDescription().numBoundResources;
	for( UINT i = 0; i < numResources; ++i )
	{
		D3D11_SHADER_INPUT_BIND_DESC resDesc;
		HRESULT hr = reflector->GetResourceBindingDesc( i, &resDesc );
		if( FAILED( hr ) )
		{
			WriteLog( "reflector->GetResourceBindingDesc error, hr = " + std::to_string( hr ) );
			return false;
		}

		if( resDesc.Type == D3D_SIT_CBUFFER     // cbuffer
			|| resDesc.Type == D3D_SIT_TBUFFER )    // tbuffer
		{
			// These were processed in the previous loop.
		}
		else if( resDesc.Type == D3D_SIT_TEXTURE        // Texture*
			|| resDesc.Type == D3D_SIT_UAV_RWTYPED )   // RWTexture*
		{
			if( IsTextureArray( resDesc.Dimension ) )
			{
				if( resDesc.BindCount == 1 )
				{
					shader.Insert( HLSLTextureArray( resDesc ) );
				}
				else
				{
					for( UINT j = 0; j < resDesc.BindCount; ++j )
					{
						shader.Insert( HLSLTextureArray( resDesc, j ) );
					}
				}
			}
			else
			{
				if( resDesc.BindCount == 1 )
				{
					shader.Insert( HLSLTexture( resDesc ) );
				}
				else
				{
					for( UINT j = 0; j < resDesc.BindCount; ++j )
					{
						shader.Insert( HLSLTexture( resDesc, j ) );
					}
				}
			}
		}
		else if( resDesc.Type == D3D_SIT_SAMPLER )   // SamplerState
		{
			if( resDesc.BindCount == 1 )
			{
				shader.Insert( HLSLSamplerState( resDesc ) );
			}
			else
			{
				for( UINT j = 0; j < resDesc.BindCount; ++j )
				{
					shader.Insert( HLSLSamplerState( resDesc, j ) );
				}
			}
		}
		else if( resDesc.Type == D3D_SIT_BYTEADDRESS         // ByteAddressBuffer
			|| resDesc.Type == D3D_SIT_UAV_RWBYTEADDRESS )  // RWByteAddressBuffer
		{
			if( resDesc.BindCount == 1 )
			{
				shader.Insert( HLSLByteAddressBuffer( resDesc ) );
			}
			else
			{
				for( UINT j = 0; j < resDesc.BindCount; ++j )
				{
					shader.Insert( HLSLByteAddressBuffer( resDesc, j ) );
				}
			}
		}
		else
		{
			// D3D_SIT_STRUCTURED:  StructuredBuffer
			// D3D_SIT_UAV_RWSTRUCTURED:  RWStructuredBuffer
			// D3D_SIT_UAV_APPEND_STRUCTURED:  AppendStructuredBuffer
			// D3D_SIT_UAV_CONSUME_STRUCTURED:  ConsumeStructuredBuffer
			// D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:  RWStructuredBuffer

			if( resDesc.BindCount == 1 )
			{
				shader.Insert( HLSLStructuredBuffer( resDesc ) );
			}
			else
			{
				for( UINT j = 0; j < resDesc.BindCount; ++j )
				{
					shader.Insert( HLSLStructuredBuffer( resDesc, j ) );
				}
			}
		}
	}

	return true;
}



bool DXReflectShader( std::string const& name, std::string const& entry, std::string const& target,
	ID3DBlob* compiledCode, HLSLShader & shader )
{
	void const* buffer = compiledCode->GetBufferPointer();
	size_t numBytes = compiledCode->GetBufferSize();
	DXShaderReflection * reflector = nullptr;
	HRESULT hr = D3DReflect( buffer, numBytes, IID_ID3D11ShaderReflection, ( void** )&reflector );
	if( FAILED( hr ) )
	{
		WriteLog( "D3DReflect error, hr = " + std::to_string( hr ) );
		return false;
	}

	bool success =
		GetDescription( reflector, shader ) &&
		GetInputs( reflector, shader ) &&
		GetOutputs( reflector, shader ) &&
		GetCBuffers( reflector, shader ) &&
		GetBoundResources( reflector, shader );

	if( success )
	{
		shader.SetName( std::string( name ) );
		shader.SetEntry( std::string( entry ) );
		shader.SetTarget( std::string( target ) );
		shader.SetCompiledCode( numBytes, buffer );

		if( shader.GetDescription().shaderType == D3D11_SHVER_COMPUTE_SHADER )
		{
			// The return value of GetThreadGroupSize is numX*numY*numZ, so
			// it is safe to ignore.
			UINT numX, numY, numZ;
			reflector->GetThreadGroupSize( &numX, &numY, &numZ );
			shader.SetNumThreads( numX, numY, numZ );
		}
	}

	reflector->Release();
	return success;
}

#endif