
#include "precompiled.h"
#pragma hdrstop

#include "Render.h"

/*
=========================================================================================

									DOF RENDERING

=========================================================================================
*/

struct dofRenderPassParms_t
{
	const idDeclRenderParm * rpResolutionScale;
	const idDeclRenderParm * rpProjectionMatrixZ;

	const idDeclRenderParm * rpRenderPositionToViewTexture;
	const idDeclRenderParm * rpTexSize;
	const idDeclRenderParm * rpDOFParms;
	const idDeclRenderParm * rpTex0;
	const idDeclRenderParm * rpTex1;

	const idDeclRenderProg * progDOFDownsample;
	const idDeclRenderProg * progDOFTemporalAA;
	const idDeclRenderProg * progDOF;
	const idDeclRenderProg * progDOFFloodFill;
	const idDeclRenderProg * progDOFComposit;

	const idRenderDestination *	renderDestDOFDownsample;
	const idRenderDestination *	renderDestDOFTemporalAA[2]; // current/previous
	const idRenderDestination *	renderDestDOF;
	const idRenderDestination *	renderDestDOFFloodFill;
	const idRenderDestination *	renderDestDOFComposit;
};

// idView::dofParms_t
struct dofParms_t
{
	float nearEnd;
	float focusBegin;
	float focusEnd;
	float farBegin;
	float nearAmount;
	float farAmount;
	float skyAmount;
};

void SetMVPmatrixX( const idDeclRenderParm * parm )
{
	parm->Set( backEnd.viewDef->GetMVPMatrix()[ 0 ][ 0 ],
			   backEnd.viewDef->GetMVPMatrix()[ 0 ][ 1 ],
			   backEnd.viewDef->GetMVPMatrix()[ 0 ][ 2 ],
			   backEnd.viewDef->GetMVPMatrix()[ 0 ][ 3 ] );
}
void SetMVPmatrixY( const idDeclRenderParm * parm )
{
	parm->Set( backEnd.viewDef->GetMVPMatrix()[ 1 ][ 0 ],
			   backEnd.viewDef->GetMVPMatrix()[ 1 ][ 1 ],
			   backEnd.viewDef->GetMVPMatrix()[ 1 ][ 2 ],
			   backEnd.viewDef->GetMVPMatrix()[ 1 ][ 3 ] );
}
void SetMVPmatrixZ( const idDeclRenderParm * parm )
{
	parm->Set( backEnd.viewDef->GetMVPMatrix()[ 2 ][ 0 ],
			   backEnd.viewDef->GetMVPMatrix()[ 2 ][ 1 ],
			   backEnd.viewDef->GetMVPMatrix()[ 2 ][ 2 ],
			   backEnd.viewDef->GetMVPMatrix()[ 2 ][ 3 ] );
}
void SetMVPmatrixW( const idDeclRenderParm * parm )
{
	parm->Set( backEnd.viewDef->GetMVPMatrix()[ 3 ][ 0 ],
			   backEnd.viewDef->GetMVPMatrix()[ 3 ][ 1 ],
			   backEnd.viewDef->GetMVPMatrix()[ 3 ][ 2 ],
			   backEnd.viewDef->GetMVPMatrix()[ 3 ][ 3 ] );
}

void Draw( const idRenderDestination * dest, const idDeclRenderProg * prog )
{
	const idDeclRenderParm * parm;
	parm->SetEvaluator( &SetMVPmatrixX );
	parm->SetEvaluator( &SetMVPmatrixY );
	parm->SetEvaluator( &SetMVPmatrixZ );
	parm->SetEvaluator( &SetMVPmatrixW );


	GL_SetRenderDestination( dest );
	GL_ViewportAndScissor( 0, 0, dest->GetTargetWidth(), dest->GetTargetHeight() );
	GL_State( GLS_TWOSIDED | GLS_DISABLE_BLENDING | GLS_DISABLE_DEPTHTEST );
	GL_SetRenderProgram( prog );
	GL_DrawScreenTriangleAuto();
}

void RB_DOF( dofRenderPassParms_t * parms )
{
	auto const renderView = backEnd.viewDef;

	// 1: Downscale
	{
		auto src = GL_GetCurrentRenderDestination();

		parms->rpRenderPositionToViewTexture->Set( 0.0, 0.0, parms->renderDestDOFDownsample->GetTargetInvWidth(), parms->renderDestDOFDownsample->GetTargetInvHeight() );

		parms->rpTexSize->Set( src->GetTargetWidth(), src->GetTargetHeight(), src->GetTargetInvWidth(), src->GetTargetInvHeight() );
		parms->rpTex0->Set( src->GetColorImage() );
		parms->rpTex1->Set( src->GetDepthStencilImage() );

		parms->rpDOFParms->Set( 200.0f, 100.0f, 300.0f, 1.0f );

		parms->rpResolutionScale->Set( 1.0f, 1.0f, 1.0f, 1.0f );
		parms->rpProjectionMatrixZ->Set( renderView->GetProjectionMatrix()[2][0], renderView->GetProjectionMatrix()[2][1], renderView->GetProjectionMatrix()[2][2], renderView->GetProjectionMatrix()[2][3] );

		Draw( parms->renderDestDOFDownsample, parms->progDOFDownsample );
	}

	// 2: Temporal AA
	{
		parms->rpRenderPositionToViewTexture->Set( 0.0, 0.0, parms->renderDestDOFTemporalAA[ 1 ]->GetTargetInvWidth(), parms->renderDestDOFTemporalAA[ 1 ]->GetTargetInvHeight() );

		parms->rpTexSize->Set( parms->renderDestDOFDownsample->GetTargetWidth(), parms->renderDestDOFDownsample->GetTargetHeight(),
							   parms->renderDestDOFDownsample->GetTargetInvWidth(), parms->renderDestDOFDownsample->GetTargetInvHeight() );
		parms->rpTex0->Set( parms->renderDestDOFDownsample->GetColorImage() ); // result of previous pass
		parms->rpTex1->Set( parms->renderDestDOFTemporalAA[ 0 ]->GetColorImage() ); // previous result of this pass

		Draw( parms->renderDestDOFTemporalAA[ 1 ], parms->progDOFTemporalAA );
	}

	// 3: Do DOF
	{
		parms->rpRenderPositionToViewTexture->Set( 0.0, 0.0, parms->renderDestDOF->GetTargetInvWidth(), parms->renderDestDOF->GetTargetInvHeight() );

		parms->rpTexSize->Set( parms->renderDestDOFTemporalAA[ 1 ]->GetTargetWidth(), parms->renderDestDOFTemporalAA[ 1 ]->GetTargetHeight(),
							   parms->renderDestDOFTemporalAA[ 1 ]->GetTargetInvWidth(), parms->renderDestDOFTemporalAA[ 1 ]->GetTargetInvHeight() );
		parms->rpTex0->Set( parms->renderDestDOFTemporalAA[ 1 ]->GetColorImage() );

		parms->rpDOFParms->Set( 200.0f, 100.0f, 300.0f, 1.0f );

		parms->rpResolutionScale->Set( 1.0f, 1.0f, 1.0f, 1.0f );

		Draw( parms->renderDestDOF, parms->progDOF );
	}

	// 4: FloodFill
	{
		parms->rpRenderPositionToViewTexture->Set( 0.0, 0.0, parms->renderDestDOFFloodFill->GetTargetInvWidth(), parms->renderDestDOFFloodFill->GetTargetInvHeight() );

		parms->rpTexSize->Set( parms->renderDestDOF->GetTargetWidth(), parms->renderDestDOF->GetTargetHeight(),
							   parms->renderDestDOF->GetTargetInvWidth(), parms->renderDestDOF->GetTargetInvHeight() );
		parms->rpTex0->Set( parms->renderDestDOF->GetColorImage() );

		parms->rpDOFParms->Set( 200.0f, 100.0f, 300.0f, 1.0f );

		Draw( parms->renderDestDOFFloodFill, parms->progDOFFloodFill );
	}

	// 5: Composition
	{
		auto src = GL_GetCurrentRenderDestination();

		parms->rpRenderPositionToViewTexture->Set( 0.0, 0.0, parms->renderDestDOFComposit->GetTargetInvWidth(), parms->renderDestDOFComposit->GetTargetInvHeight() );

		parms->rpTexSize->Set( parms->renderDestDOFFloodFill->GetTargetWidth(), parms->renderDestDOFFloodFill->GetTargetHeight(),
							   parms->renderDestDOFFloodFill->GetTargetInvWidth(), parms->renderDestDOFFloodFill->GetTargetInvHeight() );
		parms->rpTex0->Set( parms->renderDestDOFFloodFill->GetColorImage() );
		parms->rpTex1->Set( src->GetColorImage() );

		parms->rpDOFParms->Set( 200.0f, 100.0f, 300.0f, 1.0f );

		parms->rpResolutionScale->Set( 1.0f, 1.0f, 1.0f, 1.0f );
		parms->rpProjectionMatrixZ->Set( renderView->GetProjectionMatrix()[ 2 ][ 0 ], renderView->GetProjectionMatrix()[ 2 ][ 1 ], renderView->GetProjectionMatrix()[ 2 ][ 2 ], renderView->GetProjectionMatrix()[ 2 ][ 3 ] );


		Draw( parms->renderDestDOFComposit, parms->progDOFComposit );
	}
}