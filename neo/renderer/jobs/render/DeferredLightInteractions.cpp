#include "precompiled.h"
#pragma hdrstop

#include "Render.h"

/*
=========================================================================================

	GENERAL INTERACTION RENDERING

=========================================================================================
*/
extern struct drawInteraction_rp_t;
extern void RB_DrawComplexMaterialInteraction_RP( drawInteraction_rp_t & inter,
	const float* surfaceRegs, const idMaterial* const material,
	const idRenderVector & diffuseColorMul, const idRenderVector & specularColorMul );

