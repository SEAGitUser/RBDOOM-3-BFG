
#include "precompiled.h"
#pragma hdrstop

#include "Render.h"

#define __GFSDK_GL__
#include <libs\NVTxaa\include\GFSDK_TXAA.h>

/*
==============================================================================================

	TXAA RENDERPASS

==============================================================================================
*/

enum sharpenMode_e
{
	SHARPENMODE_NONE,
	SHARPENMODE_GRADIENT,
	SHARPENMODE_SECONDORDER,
	SHARPENMODE_COUNT
};

struct renderPassTXAAParms_t
{
	NvTxaaContextGL			txaaCtx;	// TXAA context.
	NvTxaaResolveParametersGL resolveParameters;

	uint32					screenWidth, screenHeight;
	idRenderDestination *	renderDestDefault;
	idImage *				imgVelocity;		// view motion vectors
	idImage *				imgResolveTarget;
	idImage *				imgPreviousResolveTarget;
	idImage *				imgViewColorMS;
	idImage *				imgEdges;

	sharpenMode_e			sharpenMode;
	gfsdk_U32				frameCounter;       // Increment every frame, notice the ping pong of dst.
	bool					bInitialized;
};


void RB_TXAA_Init( renderPassTXAAParms_t * parms )
{
	if( NV_TXAA_STATUS_OK != GFSDK_TXAA_GL_InitializeContext( &parms->txaaCtx ) ) // No TXAA.
	{
		parms->bInitialized = false;
		common->Warning("GFSDK_TXAA_GL_InitializeContext() Fail.\n");
		return;
	}
}

void RB_TXAA_Shutdown( renderPassTXAAParms_t * parms )
{
	GFSDK_TXAA_GL_ReleaseContext( &parms->txaaCtx );
}

/*
==================
 RB_TXAA
==================
*/
static void RB_TXAA( renderPassTXAAParms_t * parms )
{
	if( !parms->bInitialized )
		return;

	NvTxaaResolveParametersGL resolveParameters = {};
	resolveParameters.txaaContext = &parms->txaaCtx;
	resolveParameters.resolveTarget = ( GLuint ) parms->imgResolveTarget->GetAPIObject();
	resolveParameters.msaaSource = ( GLuint )parms->imgViewColorMS->GetAPIObject();;
	resolveParameters.feedbackSource = ( GLuint )parms->imgPreviousResolveTarget->GetAPIObject();

	resolveParameters.controlSource = ( SHARPENMODE_NONE != parms->sharpenMode )? ( GLuint )parms->imgEdges->GetAPIObject() : GL_NONE;

	resolveParameters.alphaResolveMode = NV_TXAA_ALPHARESOLVEMODE_RESOLVESRCALPHA;
	resolveParameters.feedback = nullptr; //&m_feedback;

	resolveParameters.debug = nullptr; //&m_debug;

	NvTxaaMotionGL motion = {};
	motion.motionVectors = ( GLuint )parms->imgVelocity->GetAPIObject();

	if( NV_TXAA_STATUS_OK != GFSDK_TXAA_GL_ResolveFromMotionVectors( &resolveParameters, &motion ) )
	{
		common->Warning( "GFSDK_TXAA_GL_ResolveFromMotionVectors() Fail.\n" );
		return;
	}
}