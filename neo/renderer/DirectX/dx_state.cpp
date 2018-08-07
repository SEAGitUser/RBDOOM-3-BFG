#pragma hdrstop
#include "precompiled.h"

#include "../tr_local.h"

#include "dx_header.h"

/*
=====================================================================================================
	Contains the RenderState implementation for DirecX
=====================================================================================================
*/

static const uint64 BlendStateBits =
						( GLS_REDMASK | GLS_GREENMASK | GLS_BLUEMASK | GLS_ALPHAMASK ) |
						( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) |
						GLS_BLENDOP_BITS |
						GLS_ALPHA_COVERAGE;

static const uint64 DepthStencilBits =
						GLS_DEPTHFUNC_BITS |
						GLS_DISABLE_DEPTHTEST |
						GLS_DEPTHMASK |
						GLS_STENCILMASK |
						( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS | GLS_STENCIL_FUNC_REF_BITS | GLS_STENCIL_FUNC_MASK_BITS );

static const uint64 RasterizerStateBits =
						GLS_POLYMODE_LINE | GLS_POLYGON_OFFSET |
						GLS_TWOSIDED | GLS_BACKSIDED | GLS_FRONTSIDED;

class DXRenderStateManager
{
	struct dxState_t {
		ID3D11BlendState * pBS = nullptr;
		ID3D11DepthStencilState* pDSS = nullptr;
		ID3D11RasterizerState* pRS = nullptr;
	};

	typedef idPair<uint64, ID3D11BlendState *> dxBlendStatePair_t;
	typedef idPair<uint64, ID3D11DepthStencilState *> dxDSStatePair_t;
	typedef idPair<uint64, ID3D11RasterizerState *>	dxRasterStatePair_t;

	idList<dxBlendStatePair_t> blendStateList;
	idList<dxDSStatePair_t> dsStateList;
	idList<dxRasterStatePair_t> rasterStateList;

	dxState_t dx_state_disable_rasterizer;

public:

	void Init();
	void Shutdown();

	ID_INLINE ID3D11BlendState * GetBlendState( uint64 stateBits );
	ID_INLINE ID3D11DepthStencilState * GetDepthStencilState( uint64 stateBits );
	ID_INLINE ID3D11RasterizerState * GetRasterizerState( uint64 stateBits );

	void DisableRasterizer( DXDeviceContext * iContext )
	{
		iContext->OMSetBlendState( dx_state_disable_rasterizer.pBS, NULL, 0 );
		iContext->OMSetDepthStencilState( dx_state_disable_rasterizer.pDSS, 0 );
		iContext->RSSetState( dx_state_disable_rasterizer.pRS );

		auto & dxState = GetCurrentRenderState();

		dxState.currentBSObject = dx_state_disable_rasterizer.pBS;
		dxState.currentDSSObject = dx_state_disable_rasterizer.pDSS;
		dxState.currentRSObject = dx_state_disable_rasterizer.pRS;
	}

} dxRenderStateManager;

void DXRenderStateManager::Init()
{
	HRESULT res;
	auto device = GetCurrentDevice();

	// Disable rasterizer states
	{
		DX_BLEND_DESC blend_desc = {};
		DX_DEPTH_STENCIL_DESC depth_stencil_desc = {};
		DX_RASTERIZER_DESC rasterizer_desc = {};

		res = device->CreateBlendState( &blend_desc, &dx_state_disable_rasterizer.pBS ); assert( res == S_OK );
		res = device->CreateDepthStencilState( &depth_stencil_desc, &dx_state_disable_rasterizer.pDSS ); assert( res == S_OK );
		res = device->CreateRasterizerState( &rasterizer_desc, &dx_state_disable_rasterizer.pRS ); assert( res == S_OK );
	}


}

void DXRenderStateManager::Shutdown()
{
	for( int i = 0; i < blendStateList.Num(); i++ )
	{
		blendStateList[ i ].first = 0ull;
		assert( blendStateList[ i ].second != nullptr );
		blendStateList[ i ].second->Release();
		blendStateList[ i ].second = nullptr;
	}
	blendStateList.Clear();

	for( int i = 0; i < dsStateList.Num(); i++ )
	{
		dsStateList[ i ].first = 0ull;
		assert( dsStateList[ i ].second != nullptr );
		dsStateList[ i ].second->Release();
		dsStateList[ i ].second = nullptr;
	}
	dsStateList.Clear();

	for( int i = 0; i < rasterStateList.Num(); i++ )
	{
		rasterStateList[ i ].first = 0ull;
		assert( rasterStateList[ i ].second != nullptr );
		rasterStateList[ i ].second->Release();
		rasterStateList[ i ].second = nullptr;
	}
	rasterStateList.Clear();
}

/*
===============================
BLEND STATE
===============================
*/
ID3D11BlendState * DXRenderStateManager::GetBlendState( uint64 stateBits )
{
	for( uint32 i = 0; i < blendStateList.Num(); i++ )
	{
		if( blendStateList[ i ].first == stateBits )
		{
			return blendStateList[ i ].second;
		}
	}

	// else create a new one and store

	auto device = GetCurrentDevice();

	DX_BLEND_DESC desc = {};

	desc.AlphaToCoverageEnable = FALSE;
	desc.IndependentBlendEnable = FALSE;

	switch( stateBits & GLS_SRCBLEND_BITS )
	{
		default:
		case GLS_SRCBLEND_ONE:					desc.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_ONE;				break;
		case GLS_SRCBLEND_ZERO:					desc.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_ZERO;				break;
		case GLS_SRCBLEND_DST_COLOR:			desc.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_DEST_COLOR;		break;
		case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:	desc.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_INV_DEST_COLOR;	break;
		case GLS_SRCBLEND_SRC_ALPHA:			desc.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_SRC_ALPHA;		break;
		case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:	desc.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_INV_SRC_ALPHA;	break;
		case GLS_SRCBLEND_DST_ALPHA:			desc.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_DEST_ALPHA;		break;
		case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:	desc.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_INV_DEST_ALPHA;	break;
	}
	switch( stateBits & GLS_DSTBLEND_BITS )
	{
		default:
		case GLS_DSTBLEND_ZERO:					desc.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_ZERO;			break;
		case GLS_DSTBLEND_ONE:					desc.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_ONE;				break;
		case GLS_DSTBLEND_SRC_COLOR:			desc.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_SRC_COLOR;		break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:	desc.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_INV_SRC_COLOR;	break;
		case GLS_DSTBLEND_SRC_ALPHA:			desc.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_SRC_ALPHA;		break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:	desc.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;	break;
		case GLS_DSTBLEND_DST_ALPHA:			desc.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_DEST_ALPHA;		break;
		case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:	desc.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_INV_DEST_ALPHA;	break;
	}
	desc.RenderTarget[ 0 ].BlendEnable = ( desc.RenderTarget[ 0 ].SrcBlend == D3D11_BLEND_ONE && desc.RenderTarget[ 0 ].DestBlend == D3D11_BLEND_ZERO ) ? FALSE : TRUE;

	//SEA: swf has some issue with main menu :(
	/*switch( stateBits & GLS_BLENDOP_BITS ) {
	default:
	case GLS_BLENDOP_ADD:		desc.RenderTarget[ 0 ].BlendOp = D3D11_BLEND_OP_ADD;			break;
	case GLS_BLENDOP_SUB:		desc.RenderTarget[ 0 ].BlendOp = D3D11_BLEND_OP_SUBTRACT;		break;
	case GLS_BLENDOP_MIN:		desc.RenderTarget[ 0 ].BlendOp = D3D11_BLEND_OP_MIN;			break;
	case GLS_BLENDOP_MAX:		desc.RenderTarget[ 0 ].BlendOp = D3D11_BLEND_OP_MAX;			break;
	case GLS_BLENDOP_INVSUB:	desc.RenderTarget[ 0 ].BlendOp = D3D11_BLEND_OP_REV_SUBTRACT;	break;
	}*/
	desc.RenderTarget[ 0 ].BlendOp = D3D11_BLEND_OP_ADD;

	desc.RenderTarget[ 0 ].SrcBlendAlpha = desc.RenderTarget[ 0 ].SrcBlend;
	desc.RenderTarget[ 0 ].DestBlendAlpha = desc.RenderTarget[ 0 ].DestBlend;
	desc.RenderTarget[ 0 ].BlendOpAlpha = desc.RenderTarget[ 0 ].BlendOp;

	desc.RenderTarget[ 0 ].RenderTargetWriteMask = 0;
	desc.RenderTarget[ 0 ].RenderTargetWriteMask |= ( stateBits & GLS_REDMASK ) ? 0 : D3D11_COLOR_WRITE_ENABLE_RED;
	desc.RenderTarget[ 0 ].RenderTargetWriteMask |= ( stateBits & GLS_GREENMASK ) ? 0 : D3D11_COLOR_WRITE_ENABLE_GREEN;
	desc.RenderTarget[ 0 ].RenderTargetWriteMask |= ( stateBits & GLS_BLUEMASK ) ? 0 : D3D11_COLOR_WRITE_ENABLE_BLUE;
	desc.RenderTarget[ 0 ].RenderTargetWriteMask |= ( stateBits & GLS_ALPHAMASK ) ? 0 : D3D11_COLOR_WRITE_ENABLE_ALPHA;

	for( UINT32 i = 1; i < _countof( desc.RenderTarget ); i++ )
	{
		desc.RenderTarget[ i ] = desc.RenderTarget[ 0 ];
	}

	ID3D11BlendState * pBS = nullptr;
	auto res = device->CreateBlendState( &desc, &pBS );
	assert( res == S_OK );

	blendStateList.Append( dxBlendStatePair_t( stateBits, pBS ) );
	return pBS;
}

/*
===============================
DEPTH STENCIL STATE
===============================
*/
ID3D11DepthStencilState * DXRenderStateManager::GetDepthStencilState( uint64 stateBits )
{
	for( uint32 i = 0; i < dsStateList.Num(); i++ )
	{
		if( dsStateList[ i ].first == stateBits )
		{
			return dsStateList[ i ].second;
		}
	}

	// else create a new one and store

	auto device = GetCurrentDevice();

	DX_DEPTH_STENCIL_DESC desc = {};

	desc.DepthEnable = ( stateBits & GLS_DISABLE_DEPTHTEST ) ? FALSE : TRUE;
	desc.DepthWriteMask = ( stateBits & GLS_DEPTHMASK ) ? D3D11_DEPTH_WRITE_MASK_ZERO : D3D11_DEPTH_WRITE_MASK_ALL;

	switch( stateBits & GLS_DEPTHFUNC_BITS )
	{
		default:
		case GLS_DEPTHFUNC_LEQUAL:   desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;	  break;
		case GLS_DEPTHFUNC_EQUAL:    desc.DepthFunc = D3D11_COMPARISON_EQUAL;		  break;
		case GLS_DEPTHFUNC_ALWAYS:   desc.DepthFunc = D3D11_COMPARISON_ALWAYS;		  break;
		case GLS_DEPTHFUNC_GEQUAL:   desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL; break;
		case GLS_DEPTHFUNC_NOTEQUAL: desc.DepthFunc = D3D11_COMPARISON_NOT_EQUAL;	  break;
		case GLS_DEPTHFUNC_GREATER:  desc.DepthFunc = D3D11_COMPARISON_GREATER;		  break;
		case GLS_DEPTHFUNC_LESS:	 desc.DepthFunc = D3D11_COMPARISON_LESS;		  break;
		case GLS_DEPTHFUNC_NEVER:	 desc.DepthFunc = D3D11_COMPARISON_NEVER;		  break;
	}

	desc.StencilEnable = BOOL( ( stateBits & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS ) ) != 0 );
	desc.StencilReadMask = GLS_STENCIL_GET_MASK( stateBits );
	desc.StencilWriteMask = ( stateBits & GLS_STENCILMASK ) ? 0x00u : 0xFFu;
	if( desc.StencilEnable )
	{
		switch( stateBits & GLS_STENCIL_OP_FAIL_BITS )
		{
			default:
			case GLS_STENCIL_OP_FAIL_KEEP:		desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;	  break;
			case GLS_STENCIL_OP_FAIL_ZERO:		desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_ZERO;	  break;
			case GLS_STENCIL_OP_FAIL_REPLACE:	desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_REPLACE;  break;
			case GLS_STENCIL_OP_FAIL_INCR:		desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_INCR;	  break;
			case GLS_STENCIL_OP_FAIL_DECR:		desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_DECR;	  break;
			case GLS_STENCIL_OP_FAIL_INVERT:	desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_INVERT;	  break;
			case GLS_STENCIL_OP_FAIL_INCR_WRAP: desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_INCR_SAT; break;
			case GLS_STENCIL_OP_FAIL_DECR_WRAP: desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_DECR_SAT; break;
		}
		switch( stateBits & GLS_STENCIL_OP_ZFAIL_BITS )
		{
			default:
			case GLS_STENCIL_OP_ZFAIL_KEEP:		 desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;		break;
			case GLS_STENCIL_OP_ZFAIL_ZERO:		 desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_ZERO;		break;
			case GLS_STENCIL_OP_ZFAIL_REPLACE:	 desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;	break;
			case GLS_STENCIL_OP_ZFAIL_INCR:		 desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;		break;
			case GLS_STENCIL_OP_ZFAIL_DECR:		 desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;		break;
			case GLS_STENCIL_OP_ZFAIL_INVERT:	 desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INVERT;   break;
			case GLS_STENCIL_OP_ZFAIL_INCR_WRAP: desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR_SAT; break;
			case GLS_STENCIL_OP_ZFAIL_DECR_WRAP: desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR_SAT; break;
		}
		switch( stateBits & GLS_STENCIL_OP_PASS_BITS )
		{
			default:
			case GLS_STENCIL_OP_PASS_KEEP:		desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;	  break;
			case GLS_STENCIL_OP_PASS_ZERO:		desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_ZERO;	  break;
			case GLS_STENCIL_OP_PASS_REPLACE:	desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;  break;
			case GLS_STENCIL_OP_PASS_INCR:		desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;	  break;
			case GLS_STENCIL_OP_PASS_DECR:		desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_DECR;	  break;
			case GLS_STENCIL_OP_PASS_INVERT:	desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INVERT;	  break;
			case GLS_STENCIL_OP_PASS_INCR_WRAP: desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR_SAT; break;
			case GLS_STENCIL_OP_PASS_DECR_WRAP: desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_DECR_SAT; break;
		}

		switch( stateBits & GLS_STENCIL_FUNC_BITS )
		{
			default:
			case GLS_STENCIL_FUNC_ALWAYS:	desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;		 break;
			case GLS_STENCIL_FUNC_NEVER:	desc.FrontFace.StencilFunc = D3D11_COMPARISON_NEVER;		 break;
			case GLS_STENCIL_FUNC_LESS:		desc.FrontFace.StencilFunc = D3D11_COMPARISON_LESS;			 break;
			case GLS_STENCIL_FUNC_EQUAL:	desc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;		 break;
			case GLS_STENCIL_FUNC_LEQUAL:	desc.FrontFace.StencilFunc = D3D11_COMPARISON_LESS_EQUAL;	 break;
			case GLS_STENCIL_FUNC_GREATER:	desc.FrontFace.StencilFunc = D3D11_COMPARISON_GREATER;		 break;
			case GLS_STENCIL_FUNC_NOTEQUAL: desc.FrontFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;	 break;
			case GLS_STENCIL_FUNC_GEQUAL:	desc.FrontFace.StencilFunc = D3D11_COMPARISON_GREATER_EQUAL; break;
		}
	}
	desc.BackFace = desc.FrontFace;

	ID3D11DepthStencilState* pDSS = nullptr;
	auto res = device->CreateDepthStencilState( &desc, &pDSS );
	assert( res == S_OK );

	dsStateList.Append( dxDSStatePair_t( stateBits, pDSS ) );
	return pDSS;
}

/*
===============================
RASTERIZER STATE
===============================
*/
ID3D11RasterizerState* DXRenderStateManager::GetRasterizerState( uint64 stateBits )
{
	for( uint32 i = 0; i < rasterStateList.Num(); i++ )
	{
		if( rasterStateList[ i ].first == stateBits )
		{
			return rasterStateList[ i ].second;
		}
	}

	// else create a new one and store

	auto device = GetCurrentDevice();

	DX_RASTERIZER_DESC desc = {};

	desc.FillMode = ( stateBits & GLS_POLYMODE_LINE ) ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
	desc.CullMode = ( stateBits & GLS_TWOSIDED ) ? D3D11_CULL_NONE : ( ( stateBits & GLS_BACKSIDED ) ? D3D11_CULL_FRONT : D3D11_CULL_BACK );
	desc.FrontCounterClockwise = FALSE;

	desc.DepthBias = backEnd.glState.polyOfsBias;
	desc.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
	desc.SlopeScaledDepthBias = backEnd.glState.polyOfsScale;

	desc.DepthClipEnable = TRUE;
	desc.ScissorEnable = TRUE;
	desc.MultisampleEnable = FALSE;
	desc.AntialiasedLineEnable = FALSE;

	ID3D11RasterizerState* pRS = nullptr;
	auto res = device->CreateRasterizerState( &desc, &pRS );
	assert( res == S_OK );

	rasterStateList.Append( dxRasterStatePair_t( stateBits, pRS ) );
	return pRS;
}