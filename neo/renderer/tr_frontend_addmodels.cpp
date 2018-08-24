/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#pragma hdrstop
#include "precompiled.h"

#include "tr_local.h"
#include "Model_local.h"

idCVar r_skipStaticShadows( "r_skipStaticShadows", "0", CVAR_RENDERER | CVAR_BOOL, "skip static shadows" );
idCVar r_skipDynamicShadows( "r_skipDynamicShadows", "0", CVAR_RENDERER | CVAR_BOOL, "skip dynamic shadows" );
idCVar r_useParallelAddModels( "r_useParallelAddModels", "1", CVAR_RENDERER | CVAR_BOOL, "add all models in parallel with jobs" );
idCVar r_useParallelAddShadows( "r_useParallelAddShadows", "1", CVAR_RENDERER | CVAR_INTEGER, "0 = off, 1 = threaded", 0, 1 );
idCVar r_useShadowPreciseInsideTest( "r_useShadowPreciseInsideTest", "1", CVAR_RENDERER | CVAR_BOOL, "use a precise and more expensive test to determine whether the view is inside a shadow volume" );
idCVar r_cullDynamicShadowTriangles( "r_cullDynamicShadowTriangles", "1", CVAR_RENDERER | CVAR_BOOL, "cull occluder triangles that are outside the light frustum so they do not contribute to the dynamic shadow volume" );
idCVar r_cullDynamicLightTriangles( "r_cullDynamicLightTriangles", "1", CVAR_RENDERER | CVAR_BOOL, "cull surface triangles that are outside the light frustum so they do not get rendered for interactions" );
idCVar r_forceShadowCaps( "r_forceShadowCaps", "0", CVAR_RENDERER | CVAR_BOOL, "0 = skip rendering shadow caps if view is outside shadow volume, 1 = always render shadow caps" );
// RB begin
idCVar r_forceShadowMapsOnAlphaTestedSurfaces( "r_forceShadowMapsOnAlphaTestedSurfaces", "1", CVAR_RENDERER | CVAR_BOOL, "0 = same shadowing as with stencil shadows, 1 = ignore noshadows for alpha tested materials" );
// RB end
// foresthale 2014-11-24: cvar to control the material lod flags - this is the distance at which a mesh switches from lod1 to lod2,
// where lod3 will appear at this distance *2, lod4 at *4, and persistentLOD keyword will disable the max distance check (thus extending
// this LOD to all further distances, rather than disappearing)
idCVar r_lodMaterialDistance( "r_lodMaterialDistance", "500", CVAR_RENDERER | CVAR_FLOAT, "surfaces further than this distance will use lower quality versions (if their material uses the lod1-4 keywords, persistentLOD disables the max distance checks)" );

idCVar r_useWeaponDepthHack( "r_useWeaponDepthHack", "1", CVAR_RENDERER | CVAR_BOOL, "apply depth hacks to a projection matrix, so view weapons don't poke into walls" );
idCVar r_useModelDepthHack( "r_useModelDepthHack", "1", CVAR_RENDERER | CVAR_BOOL, "apply depth hacks to a projection matrix, so particle effects don't clip into walls" );

/*
===================
 R_SetupDrawSurfShader
===================
*/
void R_SetupDrawSurfShader( drawSurf_t * drawSurf, const idMaterial * material, const renderEntityParms_t * renderEntityParms, const idRenderView * viewDef )
{
	drawSurf->material = material;
	drawSurf->sort = material->GetSort();

	// process the shader expressions for conditionals / color / texcoords
	const float* constRegs = material->ConstantRegisters();
	if( likely( constRegs != NULL ) )
	{
		// shader only uses constant values
		drawSurf->shaderRegisters = constRegs;
	}
	else {
		// by default evaluate with the entityDef's shader parms
		const float * gameParms = renderEntityParms->shaderParms;

		// a reference shader will take the calculated stage color value from another shader
		// and use that for the parm0-parm3 of the current shader, which allows a stage of
		// a light model and light flares to pick up different flashing tables from
		// different light shaders
		float generatedShaderParms[MAX_ENTITY_SHADER_PARMS];
		if( unlikely( renderEntityParms->referenceMaterial != NULL ) )
		{
			// evaluate the reference shader to find our shader parms
			float refRegs[MAX_EXPRESSION_REGISTERS];
			renderEntityParms->referenceMaterial->EvaluateRegisters( refRegs, renderEntityParms->shaderParms, viewDef->GetGlobalMaterialParms(),
													viewDef->GetGameTimeSec( renderEntityParms->timeGroup ), renderEntityParms->referenceSound );

			auto pStage = renderEntityParms->referenceMaterial->GetStage( 0 );

			memcpy( generatedShaderParms, renderEntityParms->shaderParms, sizeof( generatedShaderParms ) );
			generatedShaderParms[ SHADERPARM_RED ]   = refRegs[ pStage->color.registers[ SHADERPARM_RED ] ];
			generatedShaderParms[ SHADERPARM_GREEN ] = refRegs[ pStage->color.registers[ SHADERPARM_GREEN ] ];
			generatedShaderParms[ SHADERPARM_BLUE ]  = refRegs[ pStage->color.registers[ SHADERPARM_BLUE ] ];

			gameParms = generatedShaderParms;
		}

		// allocate frame memory for the shader register values
		auto regs = allocManager.FrameAlloc<float, FRAME_ALLOC_SHADER_REGISTER>( material->GetNumRegisters() );
		drawSurf->shaderRegisters = regs;

		// process the shader expressions for conditionals / color / texcoords
		material->EvaluateRegisters( regs, gameParms, viewDef->GetGlobalMaterialParms(),
									 viewDef->GetGameTimeSec( renderEntityParms->timeGroup ),
									 renderEntityParms->referenceSound );
	}
}

//SEA: D3
/*
==================
R_CreateAmbientCache
Create it if needed
==================
*/
void R_CreateAmbientCache( idTriangles *tri, const idMaterial* material, bool bDeriveTangents )
{
	// make sure we have an ambient cache and all necessary normals / tangents
	if( !vertexCache.CacheIsCurrent( tri->indexCache ) )
	{
		tri->indexCache = vertexCache.AllocIndex( tri->indexes, ALIGN( tri->numIndexes * sizeof( triIndex_t ), INDEX_CACHE_ALIGN ) );
	}

	if( !vertexCache.CacheIsCurrent( tri->vertexCache ) )
	{
		// we are going to use it for drawing, so make sure we have the tangents and normals
		if( bDeriveTangents && !tri->tangentsCalculated )
		{
			assert( tri->staticModelWithJoints == NULL );
			tri->DeriveTangents();

			// RB: this was hit by parametric particle models ..
			//assert( false );	// this should no longer be hit
			// RB end
		}
		tri->vertexCache = vertexCache.AllocVertex( tri->verts, ALIGN( tri->numVerts * sizeof( idDrawVert ), VERTEX_CACHE_ALIGN ) );
	}
}

/*
===================
 R_SetupDrawSurfJoints
===================
*/
void R_SetupDrawSurfJoints( drawSurf_t* drawSurf, const idTriangles* tri, const idMaterial* shader )
{
	if( tri->staticModelWithJoints == NULL )
	{
		drawSurf->jointCache = 0;
		return;
	}

	idRenderModelStatic* model = tri->staticModelWithJoints;
	assert( model->jointsInverted != NULL );

	if( !vertexCache.CacheIsCurrent( model->jointsInvertedBuffer ) )
	{
		model->jointsInvertedBuffer = vertexCache.AllocJoint( model->jointsInverted, ALIGN( model->numInvertedJoints * sizeof( idJointMat ), glConfig.uniformBufferOffsetAlignment ) );
	}
	drawSurf->jointCache = model->jointsInvertedBuffer;
}

struct localInteractions_t
{
	static const int MAX_CONTACTED_LIGHTS = 128;
	int	numContactedLights = 0;
	viewLight_t* contactedLights[ MAX_CONTACTED_LIGHTS ];
	idInteraction* staticInteractions[ MAX_CONTACTED_LIGHTS ];
};

/*
===================
 R_AddSingleModel

	May be run in parallel.

	Here is where dynamic models actually get instantiated, and necessary
	interaction surfaces get created. This is all done on a sort-by-model
	basis to keep source data in cache (most likely L2) as any interactions
	and shadows are generated, since dynamic models will typically be lit by
	two or more lights.
===================
*/
void R_AddSingleModel( viewModel_t * vEntity )
{
	// we will add all interaction surfs here, to be chained to the lights in later serial code
	vEntity->drawSurfs = NULL;
	vEntity->staticShadowVolumes = NULL;
	vEntity->dynamicShadowVolumes = NULL;

	// globals we really should pass in...
	const idRenderView * const viewDef = tr.viewDef;
	idRenderEntityLocal * const entityDef = vEntity->entityDef;
	const idRenderWorldLocal * const world = tr.viewDef->renderWorld; //vEntity->entityDef->world;

	if( viewDef->isXraySubview && entityDef->GetParms().xrayIndex == 1 )
	{
		return;
	}
	else if( !viewDef->isXraySubview && entityDef->GetParms().xrayIndex == 2 )
	{
		return;
	}

	SCOPED_PROFILE_EVENT( entityDef->GetModel() == NULL ? "Unknown Model" : entityDef->GetModel()->Name() );

	// calculate the znear for testing whether or not the view is inside a shadow projection
	const float znear = viewDef->GetZNear();

	// if the entity wasn't seen through a portal chain, it was added just for light shadows
	const bool modelIsVisible = !vEntity->scissorRect.IsEmpty();
	const bool addInteractions = modelIsVisible && ( !viewDef->isXraySubview || entityDef->GetParms().xrayIndex == 2 );

	//---------------------------
	// Find which of the visible lights contact this entity
	//
	// If the entity doesn't accept light or cast shadows from any surface,
	// this can be skipped.
	//
	// OPTIMIZE: world areas can assume all referenced lights are used
	//---------------------------

	localInteractions_t inters;

	if( entityDef->GetModel() == NULL ||
		entityDef->GetModel()->ModelHasInteractingSurfaces() ||
		entityDef->GetModel()->ModelHasShadowCastingSurfaces() )
	{
		SCOPED_PROFILE_EVENT( "Find lights" );
		for( viewLight_t* vLight = viewDef->viewLights; vLight != NULL; vLight = vLight->next )
		{
			if( vLight->scissorRect.IsEmpty() )
				continue;

			if( vLight->entityInteractionState != NULL )
			{
				// new code path, everything was done in AddLight
				if( vLight->entityInteractionState[ entityDef->GetIndex() ] == viewLight_t::INTERACTION_YES )
				{
					inters.contactedLights[ inters.numContactedLights ] = vLight;
					inters.staticInteractions[ inters.numContactedLights ] = world->GetInteractionForPair( entityDef->GetIndex(), vLight->lightDef->GetIndex() );
					inters.numContactedLights++;

					if( inters.numContactedLights == localInteractions_t::MAX_CONTACTED_LIGHTS )
					{
						break;
					}
				}
				continue;
			}

			const idRenderLightLocal* lightDef = vLight->lightDef;

			if( !lightDef->globalLightBounds.IntersectsBounds( entityDef->globalReferenceBounds ) )
			{
				continue;
			}

			if( lightDef->CullModelBoundsToLight( entityDef->localReferenceBounds, entityDef->modelRenderMatrix ) )
			{
				continue;
			}

			if( !modelIsVisible )
			{
				// some lights have their center of projection outside the world
				if( lightDef->areaNum != -1 )
				{
					// if no part of the model is in an area that is connected to
					// the light center (it is behind a solid, closed door), we can ignore it
					bool areasConnected = false;
					for( areaReference_t* ref = entityDef->entityRefs; ref != NULL; ref = ref->ownerNext )
					{
						if( world->AreasAreConnected( lightDef->areaNum, ref->area->areaNum, PS_BLOCK_VIEW ) )
						{
							areasConnected = true;
							break;
						}
					}
					if( areasConnected == false )
					{
						// can't possibly be seen or shadowed
						continue;
					}
				}

				// check more precisely for shadow visibility
				idBounds shadowBounds;
				R_ShadowBounds( entityDef->globalReferenceBounds, lightDef->globalLightBounds, lightDef->globalLightOrigin, shadowBounds );

				// this doesn't say that the shadow can't effect anything, only that it can't
				// effect anything in the view
				if( idRenderMatrix::CullBoundsToMVP( viewDef->GetMVPMatrix(), shadowBounds ) )
				{
					continue;
				}
			}

			inters.contactedLights[ inters.numContactedLights ] = vLight;
			inters.staticInteractions[ inters.numContactedLights ] = world->GetInteractionForPair( entityDef->GetIndex(), vLight->lightDef->GetIndex() );
			inters.numContactedLights++;

			if( inters.numContactedLights == localInteractions_t::MAX_CONTACTED_LIGHTS )
			{
				break;
			}
		}
	}

	// if we aren't visible and none of the shadows stretch into the view,
	// we don't need to do anything else
	if( !modelIsVisible && inters.numContactedLights == 0 )
	{
		return;
	}

	//---------------------------
	// create a dynamic model if the geometry isn't static
	//---------------------------
	auto const model = entityDef->EmitDynamicModel();
	if( !model || !model->NumSurfaces() )
	{
		return;
	}

	// add the lightweight blood decal surfaces if the model is directly visible
	if( modelIsVisible )
	{
		assert( !vEntity->scissorRect.IsEmpty() );

		if( entityDef->decals != NULL && !r_skipDecals.GetBool() )
		{
			entityDef->decals->CreateDeferredDecals( model );

			unsigned int numDrawSurfs = entityDef->decals->GetNumDecalDrawSurfs();
			for( unsigned int i = 0; i < numDrawSurfs; ++i )
			{
				auto decalDrawSurf = entityDef->decals->CreateDecalDrawSurf( vEntity, i );
				if( decalDrawSurf != NULL )
				{
					decalDrawSurf->linkChain = NULL;

					decalDrawSurf->nextOnLight = vEntity->drawSurfs;
					vEntity->drawSurfs = decalDrawSurf;
				}
			}
		}

		if( entityDef->overlays != NULL && !r_skipOverlays.GetBool() )
		{
			entityDef->overlays->CreateDeferredOverlays( model );

			unsigned int numDrawSurfs = entityDef->overlays->GetNumOverlayDrawSurfs();
			for( unsigned int i = 0; i < numDrawSurfs; ++i )
			{
				auto overlayDrawSurf = entityDef->overlays->CreateOverlayDrawSurf( vEntity, model, i );
				if( overlayDrawSurf != NULL )
				{
					overlayDrawSurf->linkChain = NULL;

					overlayDrawSurf->nextOnLight = vEntity->drawSurfs;
					vEntity->drawSurfs = overlayDrawSurf;
				}
			}
		}
	}

	//---------------------------
	// copy matrix related stuff for back-end use
	// and setup a render matrix for faster culling
	//---------------------------
	vEntity->modelDepthHack  = entityDef->GetParms().modelDepthHack;
	vEntity->weaponDepthHack = entityDef->GetParms().weaponDepthHack;
	vEntity->skipMotionBlur  = entityDef->GetParms().skipMotionBlur;

	vEntity->modelMatrix.Copy( entityDef->modelRenderMatrix );

	idRenderMatrix::Multiply( viewDef->GetViewMatrix(), vEntity->modelMatrix, vEntity->modelViewMatrix );
	idRenderMatrix::Multiply( viewDef->GetProjectionMatrix(), vEntity->modelViewMatrix, vEntity->mvp );

	if( entityDef->GetParms().weaponDepthHack && r_useWeaponDepthHack.GetBool() )
	{
		idRenderMatrix::ApplyDepthHack( vEntity->mvp );
	}
	if( entityDef->GetParms().modelDepthHack != 0.0f && r_useModelDepthHack.GetBool() )
	{
		idRenderMatrix::ApplyModelDepthHack( vEntity->mvp, entityDef->GetParms().modelDepthHack );
	}

	// local light and view origins are used to determine if the view is definitely outside
	// an extruded shadow volume, which means we can skip drawing the end caps
	idVec3 localViewOrigin;
	vEntity->modelMatrix.InverseTransformPoint( viewDef->GetOrigin(), localViewOrigin );

	//---------------------------
	// add all the model surfaces
	//---------------------------
	for( int surfaceNum = 0; surfaceNum < model->NumSurfaces(); ++surfaceNum )
	{
		auto surf = model->Surface( surfaceNum );

		// for debugging, only show a single surface at a time
		if( r_singleSurface.GetInteger() >= 0 && surfaceNum != r_singleSurface.GetInteger() )
		{
			continue;
		}

		idTriangles * tri = surf->GetTriangles();
		if( tri == NULL )
		{
			continue;
		}
		if( tri->numIndexes == 0 )
		{
			continue;		// happens for particles
		}
		const idMaterial * material = surf->GetMaterial();
		if( material == NULL )
		{
			continue;
		}

		// motorsep 11-24-2014; checking for LOD surface for LOD1 iteration
		if( material->IsLOD() )
		{
			// foresthale 2014-11-24: calculate the bounds and get the distance from camera to bounds
			idBounds& localBounds = tri->bounds;
			if( tri->staticModelWithJoints )
			{
				// skeletal models have difficult to compute bounds for surfaces, so use the whole entity
				localBounds = vEntity->entityDef->localReferenceBounds;
			}
			const float* bounds = localBounds.ToFloatPtr();
			idVec3 nearestPointOnBounds = localViewOrigin;
			nearestPointOnBounds.x = idMath::Max( nearestPointOnBounds.x, bounds[0] );
			nearestPointOnBounds.x = idMath::Min( nearestPointOnBounds.x, bounds[3] );
			nearestPointOnBounds.y = idMath::Max( nearestPointOnBounds.y, bounds[1] );
			nearestPointOnBounds.y = idMath::Min( nearestPointOnBounds.y, bounds[4] );
			nearestPointOnBounds.z = idMath::Max( nearestPointOnBounds.z, bounds[2] );
			nearestPointOnBounds.z = idMath::Min( nearestPointOnBounds.z, bounds[5] );
			idVec3 delta = nearestPointOnBounds - localViewOrigin;
			float distance = delta.LengthFast();

			if( !material->IsLODVisibleForDistance( distance, r_lodMaterialDistance.GetFloat() ) )
			{
				continue;
			}
		}

		// foresthale 2014-09-01: don't skip surfaces that use the "forceShadows" flag
		if( !material->IsDrawn() && !material->SurfaceCastsShadow() )
		{
			continue;		// collision hulls, etc
		}

		// RemapShaderBySkin
		if( entityDef->GetParms().customMaterial != NULL )
		{
			// this is sort of a hack, but causes deformed surfaces to map to empty surfaces,
			// so the item highlight overlay doesn't highlight the autosprite surface
			if( material->GetDeformType() )
			{
				continue;
			}
			material = entityDef->GetParms().customMaterial;
		}
		else if( entityDef->GetParms().customSkin )
		{
			material = entityDef->GetParms().customSkin->RemapShaderBySkin( material );
			if( material == NULL )
			{
				continue;
			}
			// foresthale 2014-09-01: don't skip surfaces that use the "forceShadows" flag
			if( !material->IsDrawn() && !material->SurfaceCastsShadow() )
			{
				continue;
			}
		}

		// optionally override with the renderView->globalMaterial
		if( tr.primaryRenderView.globalMaterial != NULL )
		{
			material = tr.primaryRenderView.globalMaterial;
		}

		SCOPED_PROFILE_EVENT( material->GetName() );

		// debugging tool to make sure we have the correct pre-calculated bounds
		if( r_checkBounds.GetBool() )
		{
			for( int j = 0; j < tri->numVerts; ++j )
			{
				auto pos = tri->verts[ j ].GetPosition();

				int k;
				for( k = 0; k < 3; k++ )
				{
					if( pos[k] > tri->GetBounds()[1][k] + CHECK_BOUNDS_EPSILON ||
						pos[k] < tri->GetBounds()[0][k] - CHECK_BOUNDS_EPSILON )
					{
						common->Printf( "bad tri->bounds on %s:%s\n", entityDef->GetModel()->Name(), material->GetName() );
						break;
					}
					if( pos[k] > entityDef->localReferenceBounds[1][k] + CHECK_BOUNDS_EPSILON ||
						pos[k] < entityDef->localReferenceBounds[0][k] - CHECK_BOUNDS_EPSILON )
					{
						common->Printf( "bad referenceBounds on %s:%s\n", entityDef->GetModel()->Name(), material->GetName() );
						break;
					}
				}
				if( k != 3 )
				{
					break;
				}
			}
		}

		// view frustum culling for the precise surface bounds, which is tighter
		// than the entire entity reference bounds
		// If the entire model wasn't visible, there is no need to check the
		// individual surfaces.
		const bool surfaceDirectlyVisible = modelIsVisible && !idRenderMatrix::CullBoundsToMVP( vEntity->mvp, tri->GetBounds() );

		const bool gpuSkinned = ( tri->staticModelWithJoints != NULL );

		//--------------------------
		// base drawing surface
		//--------------------------
		const float* shaderRegisters = NULL;
		if( material->IsDrawn() )
		{
			drawSurf_t* baseDrawSurf = NULL;
			if( surfaceDirectlyVisible )
			{
				// make sure we have an ambient cache and all necessary normals / tangents
				if( !vertexCache.CacheIsCurrent( tri->indexCache ) )
				{
					tri->indexCache = vertexCache.AllocIndex( tri->indexes, ALIGN( tri->numIndexes * sizeof( triIndex_t ), INDEX_CACHE_ALIGN ) );
				}

				if( !vertexCache.CacheIsCurrent( tri->vertexCache ) )
				{
					// we are going to use it for drawing, so make sure we have the tangents and normals
					if( material->ReceivesLighting() && !tri->tangentsCalculated )
					{
						assert( tri->staticModelWithJoints == NULL );
						tri->DeriveTangents();

						// RB: this was hit by parametric particle models ..
						//assert( false );	// this should no longer be hit
						// RB end
					}
					tri->vertexCache = vertexCache.AllocVertex( tri->verts, ALIGN( tri->numVerts * sizeof( idDrawVert ), VERTEX_CACHE_ALIGN ) );
				}

				// add the surface for drawing
				// we can re-use some of the values for light interaction surfaces
				baseDrawSurf = allocManager.FrameAlloc< drawSurf_t, FRAME_ALLOC_DRAW_SURFACE >();
				baseDrawSurf->frontEndGeo = tri;
				baseDrawSurf->space = vEntity;
				baseDrawSurf->scissorRect = vEntity->scissorRect;
				baseDrawSurf->extraGLState = 0;
				baseDrawSurf->renderZFail = 0;

				R_SetupDrawSurfShader( baseDrawSurf, material, &entityDef->GetParms(), viewDef );

				shaderRegisters = baseDrawSurf->shaderRegisters;

				// Check for deformations (eyeballs, flares, etc)
				const auto shaderDeform = material->GetDeformType();
				if( shaderDeform != DFRM_NONE )
				{
					drawSurf_t* deformDrawSurf = R_DeformDrawSurf( baseDrawSurf );
					if( deformDrawSurf != NULL )
					{
						// any deforms may have created multiple draw surfaces
						for( drawSurf_t* surf = deformDrawSurf, * next = NULL; surf != NULL; surf = next )
						{
							next = surf->nextOnLight;

							surf->linkChain = NULL;

							surf->nextOnLight = vEntity->drawSurfs;
							vEntity->drawSurfs = surf;
						}
					}
				}

				// Most deform source surfaces do not need to be rendered.
				// However, particles are rendered in conjunction with the source surface.
				if( shaderDeform == DFRM_NONE || shaderDeform == DFRM_PARTICLE || shaderDeform == DFRM_PARTICLE2 )
				{
					// copy verts and indexes to this frame's hardware memory if they aren't already there
					if( !vertexCache.CacheIsCurrent( tri->vertexCache ) )
					{
						tri->vertexCache = vertexCache.AllocVertex( tri->verts, ALIGN( tri->numVerts * sizeof( tri->verts[0] ), VERTEX_CACHE_ALIGN ) );
					}
					if( !vertexCache.CacheIsCurrent( tri->indexCache ) )
					{
						tri->indexCache = vertexCache.AllocIndex( tri->indexes, ALIGN( tri->numIndexes * sizeof( tri->indexes[0] ), INDEX_CACHE_ALIGN ) );
					}

					R_SetupDrawSurfJoints( baseDrawSurf, tri, material );

					baseDrawSurf->numIndexes = tri->numIndexes;
					baseDrawSurf->vertexCache = tri->vertexCache;
					baseDrawSurf->indexCache = tri->indexCache;
					baseDrawSurf->shadowCache = 0;

					baseDrawSurf->linkChain = NULL;	// link to the view

					baseDrawSurf->nextOnLight = vEntity->drawSurfs;
					vEntity->drawSurfs = baseDrawSurf;
				}
			}
		}

		//----------------------------------------
		// add all light interactions
		//----------------------------------------
		for( int contactedLight = 0; contactedLight < inters.numContactedLights; ++contactedLight )
		{
			viewLight_t* vLight = inters.contactedLights[ contactedLight ];
			const idInteraction* interaction = inters.staticInteractions[ contactedLight ];

			const idRenderLightLocal* lightDef = vLight->lightDef;

			// check for a static interaction
			surfaceInteraction_t* surfInter = NULL;
			if( interaction > INTERACTION_EMPTY && interaction->isStaticInteraction )
			{
				// we have a static interaction that was calculated accurately
				assert( model->NumSurfaces() == interaction->numSurfaces );
				surfInter = &interaction->surfaces[surfaceNum];
			}
			else {
				// try to do a more precise cull of this model surface to the light
				if( lightDef->CullModelBoundsToLight( tri->GetBounds(), vEntity->modelMatrix ) )
				{
					continue;
				}
			}

			// "invisible ink" lights and shaders (imp spawn drawing on walls, etc)
			if( material->Spectrum() != lightDef->lightShader->Spectrum() )
			{
				continue;
			}

			// Calculate the local light origin to determine if the view is inside the shadow
			// projection and to calculate the triangle facing for dynamic shadow volumes.
			idVec3 localLightOrigin;
			vEntity->modelMatrix.InverseTransformPoint( lightDef->globalLightOrigin, localLightOrigin );

			//--------------------------
			// surface light interactions
			//--------------------------

			dynamicShadowVolumeParms_t* dynamicShadowParms = NULL;

			if( addInteractions && surfaceDirectlyVisible && material->ReceivesLighting() )
			{
				// static interactions can commonly find that no triangles from a surface
				// contact the light, even when the total model does
				if( surfInter == NULL || surfInter->lightTrisIndexCache > 0 )
				{
					// make sure we have a valid shader register even if we didn't generate a drawn mesh above
					if( shaderRegisters == NULL )
					{
						drawSurf_t scratchSurf;
						R_SetupDrawSurfShader( &scratchSurf, material, &entityDef->GetParms(), viewDef );
						shaderRegisters = scratchSurf.shaderRegisters;
					}

					if( shaderRegisters != NULL )
					{
						// create a drawSurf for this interaction
						auto lightDrawSurf = allocManager.FrameAlloc<drawSurf_t, FRAME_ALLOC_DRAW_SURFACE>();

						if( surfInter != NULL )
						{
							// optimized static interaction
							lightDrawSurf->numIndexes = surfInter->numLightTrisIndexes;
							lightDrawSurf->indexCache = surfInter->lightTrisIndexCache;
						}
						else {
							// throw the entire source surface at it without any per-triangle culling
							lightDrawSurf->numIndexes = tri->numIndexes;
							lightDrawSurf->indexCache = tri->indexCache;

							// optionally cull the triangles to the light volume
							// motorsep 11-09-2014; added && material->SurfaceCastsShadow() per Lordhavoc's recommendation; should skip shadows calculation for surfaces with noShadows material flag
							// when using shadow volumes
							if( r_cullDynamicLightTriangles.GetBool() && !r_skipDynamicShadows.GetBool() && !r_useShadowMapping.GetBool() && material->SurfaceCastsShadow() )
							{
								vertCacheHandle_t lightIndexCache = vertexCache.AllocIndex( NULL, ALIGN( lightDrawSurf->numIndexes * sizeof( triIndex_t ), INDEX_CACHE_ALIGN ) );
								if( vertexCache.CacheIsCurrent( lightIndexCache ) )
								{
									lightDrawSurf->indexCache = lightIndexCache;

									dynamicShadowParms = allocManager.FrameAlloc<dynamicShadowVolumeParms_t, FRAME_ALLOC_SHADOW_VOLUME_PARMS>();

									dynamicShadowParms->verts = tri->verts;
									dynamicShadowParms->numVerts = tri->numVerts;
									dynamicShadowParms->indexes = tri->indexes;
									dynamicShadowParms->numIndexes = tri->numIndexes;
									dynamicShadowParms->silEdges = tri->silEdges;
									dynamicShadowParms->numSilEdges = tri->numSilEdges;
									dynamicShadowParms->joints = gpuSkinned ? tri->staticModelWithJoints->jointsInverted : NULL;
									dynamicShadowParms->numJoints = gpuSkinned ? tri->staticModelWithJoints->numInvertedJoints : 0;
									dynamicShadowParms->triangleBounds = tri->GetBounds();
									dynamicShadowParms->triangleMVP = vEntity->mvp;
									dynamicShadowParms->localLightOrigin = localLightOrigin;
									dynamicShadowParms->localViewOrigin = localViewOrigin;
									idRenderMatrix::Multiply( vLight->lightDef->baseLightProject, entityDef->modelRenderMatrix, dynamicShadowParms->localLightProject );
									dynamicShadowParms->zNear = znear;
									dynamicShadowParms->lightZMin = vLight->scissorRect.zmin;
									dynamicShadowParms->lightZMax = vLight->scissorRect.zmax;
									dynamicShadowParms->cullShadowTrianglesToLight = false;
									dynamicShadowParms->forceShadowCaps = false;
									dynamicShadowParms->useShadowPreciseInsideTest = false;
									dynamicShadowParms->useShadowDepthBounds = false;
									dynamicShadowParms->tempFacing = NULL;
									dynamicShadowParms->tempCulled = NULL;
									dynamicShadowParms->tempVerts = NULL;
									dynamicShadowParms->indexBuffer = NULL;
									dynamicShadowParms->shadowIndices = NULL;
									dynamicShadowParms->maxShadowIndices = 0;
									dynamicShadowParms->numShadowIndices = NULL;
									dynamicShadowParms->lightIndices = ( triIndex_t* )vertexCache.MappedIndexBuffer( lightIndexCache );
									dynamicShadowParms->maxLightIndices = lightDrawSurf->numIndexes;
									dynamicShadowParms->numLightIndices = &lightDrawSurf->numIndexes;
									dynamicShadowParms->renderZFail = NULL;
									dynamicShadowParms->shadowZMin = NULL;
									dynamicShadowParms->shadowZMax = NULL;
									dynamicShadowParms->shadowVolumeState = & lightDrawSurf->shadowVolumeState;

									lightDrawSurf->shadowVolumeState = SHADOWVOLUME_UNFINISHED;

									dynamicShadowParms->next = vEntity->dynamicShadowVolumes;
									vEntity->dynamicShadowVolumes = dynamicShadowParms;
								}
							}
						}

						lightDrawSurf->vertexCache = tri->vertexCache;
						lightDrawSurf->shadowCache = 0;
						lightDrawSurf->frontEndGeo = tri;
						lightDrawSurf->space = vEntity;
						lightDrawSurf->material = material;
						lightDrawSurf->extraGLState = 0;
						lightDrawSurf->scissorRect = vLight->scissorRect; // interactionScissor;
						lightDrawSurf->sort = 0.0f;
						lightDrawSurf->renderZFail = 0;
						lightDrawSurf->shaderRegisters = shaderRegisters;

						R_SetupDrawSurfJoints( lightDrawSurf, tri, material );

						// Determine which linked list to add the light surface to.
						// There will only be localSurfaces if the light casts shadows and
						// there are surfaces with NOSELFSHADOW.
						if( material->Coverage() == MC_TRANSLUCENT )
						{
							lightDrawSurf->linkChain = &vLight->translucentInteractions;
						}
						else if( !lightDef->GetParms().noShadows && material->TestMaterialFlag( MF_NOSELFSHADOW ) )
						{
							lightDrawSurf->linkChain = &vLight->localInteractions;
						}
						else {
							lightDrawSurf->linkChain = &vLight->globalInteractions;
						}

						lightDrawSurf->nextOnLight = vEntity->drawSurfs;
						vEntity->drawSurfs = lightDrawSurf;
					}
				}
			}

			//--------------------------
			// surface shadows
			//--------------------------

		#if 1
			if( !material->SurfaceCastsShadow() && !( r_useShadowMapping.GetBool() && r_forceShadowMapsOnAlphaTestedSurfaces.GetBool() && material->Coverage() == MC_PERFORATED ) )
			{
				continue;
			}
		#else
			// Steel Storm 2 behaviour - this destroys many alpha tested shadows in vanilla BFG

			// motorsep 11-08-2014; if r_forceShadowMapsOnAlphaTestedSurfaces is 0 when shadow mapping is on,
			// don't render shadows from all alphaTest surfaces.
			// Useful as global performance booster for old GPUs to disable shadows from grass/foliage/etc.
			if( r_useShadowMapping.GetBool() )
			{
				if( material->Coverage() == MC_PERFORATED )
				{
					if( !r_forceShadowMapsOnAlphaTestedSurfaces.GetBool() )
						continue;
				}
			}

			// if material has "noShadows" global key
			if( !material->SurfaceCastsShadow() )
			{
				// motorsep 11-08-2014; if r_forceShadowMapsOnAlphaTestedSurfaces is 1 when shadow mapping is on,
				// check if a surface IS NOT alphaTested and has "noShadows" global key;
				// or if a surface IS alphaTested and has "noShadows" global key;
				// if either is true, don't make surfaces cast shadow maps.
				if( r_useShadowMapping.GetBool() )
				{
					if( material->Coverage() != MC_PERFORATED && material->TestMaterialFlag( MF_NOSHADOWS ) )
					{
						continue;
					}
					else if( material->Coverage() == MC_PERFORATED && material->TestMaterialFlag( MF_NOSHADOWS ) )
					{
						continue;
					}
				}
				else
				{
					continue;
				}
			}
		#endif

			if( !lightDef->LightCastsShadows() )
			{
				continue;
			}

			if( tri->silEdges == NULL ) //SEA: Is this a problem for shadow mapping ?
			{
				continue;		// can happen for beam models (shouldn't use a shadow casting material, though...)
			}

			// if the static shadow does not have any shadows
			if( surfInter != NULL && surfInter->numShadowIndexes == 0 )
			{
				if( !r_useShadowMapping.GetBool() )
					continue;
			}

			// some entities, like view weapons, don't cast any shadows
			if( entityDef->GetParms().noShadow )
			{
				continue;
			}

			// No shadow if it's suppressed for this light.
			if( entityDef->GetParms().suppressShadowInLightID &&
				entityDef->GetParms().suppressShadowInLightID == lightDef->GetID() )
			{
				continue;
			}

			// RB begin
			if( r_useShadowMapping.GetBool() )
			{
				//if( addInteractions && surfaceDirectlyVisible && shader->ReceivesLighting() )
				{
					// static interactions can commonly find that no triangles from a surface
					// contact the light, even when the total model does
					if( surfInter == NULL || surfInter->lightTrisIndexCache > 0 )
					{
						// create a drawSurf for this interaction
						auto shadowDrawSurf = allocManager.FrameAlloc<drawSurf_t, FRAME_ALLOC_DRAW_SURFACE>();

						if( surfInter != NULL )
						{
							// optimized static interaction
							shadowDrawSurf->numIndexes = surfInter->numLightTrisIndexes;
							shadowDrawSurf->indexCache = surfInter->lightTrisIndexCache;
						}
						else
						{
							// make sure we have an ambient cache and all necessary normals / tangents
							if( !vertexCache.CacheIsCurrent( tri->indexCache ) )
							{
								tri->indexCache = vertexCache.AllocIndex( tri->indexes, ALIGN( tri->numIndexes * sizeof( triIndex_t ), INDEX_CACHE_ALIGN ) );
							}

							// throw the entire source surface at it without any per-triangle culling
							shadowDrawSurf->numIndexes = tri->numIndexes;
							shadowDrawSurf->indexCache = tri->indexCache;
						}

						if( !vertexCache.CacheIsCurrent( tri->vertexCache ) )
						{
							// we are going to use it for drawing, so make sure we have the tangents and normals
							if( material->ReceivesLighting() && !tri->tangentsCalculated )
							{
								assert( tri->staticModelWithJoints == NULL );
								tri->DeriveTangents();
								//SEA: do we need it for shadow mapping ?
								// could we use more simple format here ?

								// RB: this was hit by parametric particle models ..
								//assert( false );	// this should no longer be hit
								// RB end
							}

							tri->vertexCache = vertexCache.AllocVertex( tri->verts, ALIGN( tri->numVerts * sizeof( idDrawVert ), VERTEX_CACHE_ALIGN ) );
						}

						shadowDrawSurf->vertexCache = tri->vertexCache;
						shadowDrawSurf->shadowCache = 0;
						shadowDrawSurf->frontEndGeo = tri;
						shadowDrawSurf->space = vEntity;
						shadowDrawSurf->material = material;
						shadowDrawSurf->extraGLState = 0;
						shadowDrawSurf->scissorRect = vLight->scissorRect; // interactionScissor;
						shadowDrawSurf->sort = 0.0f;
						shadowDrawSurf->renderZFail = 0;
						//shadowDrawSurf->shaderRegisters = baseDrawSurf->shaderRegisters;

						if( material->Coverage() == MC_PERFORATED )
						{
							R_SetupDrawSurfShader( shadowDrawSurf, material, &entityDef->GetParms(), viewDef );
						}

						R_SetupDrawSurfJoints( shadowDrawSurf, tri, material );

						// determine which linked list to add the shadow surface to

						//shadowDrawSurf->linkChain = material->TestMaterialFlag( MF_NOSELFSHADOW ) ? &vLight->localShadows : &vLight->globalShadows;
						shadowDrawSurf->linkChain = &vLight->globalShadows;

						shadowDrawSurf->nextOnLight = vEntity->drawSurfs;
						vEntity->drawSurfs = shadowDrawSurf;

					}
				}

				continue;
			}
			// RB end

			if( lightDef->HasPreLightModel() && lightDef->lightHasMoved == false &&
				entityDef->GetModel()->IsStaticWorldModel() && !r_skipPrelightShadows.GetBool() )
			{
				// static light / world model shadow interacitons
				// are always captured in the prelight shadow volume
				continue;
			}

			// If the shadow is drawn (or translucent), but the model isn't, we must include the shadow caps
			// because we may be able to see into the shadow volume even though the view is outside it.
			// This happens for the player world weapon and possibly some animations in multiplayer.
			const bool forceShadowCaps = !addInteractions || r_forceShadowCaps.GetBool();

			auto shadowDrawSurf = allocManager.FrameAlloc<drawSurf_t, FRAME_ALLOC_DRAW_SURFACE>();

			if( surfInter != NULL )
			{
				shadowDrawSurf->numIndexes = 0;
				shadowDrawSurf->indexCache = surfInter->shadowIndexCache;
				shadowDrawSurf->shadowCache = tri->shadowCache;
				shadowDrawSurf->scissorRect = vLight->scissorRect;		// default to the light scissor and light depth bounds
				shadowDrawSurf->shadowVolumeState = SHADOWVOLUME_DONE;	// assume the shadow volume is done in case r_skipStaticShadows is set

				if( !r_skipStaticShadows.GetBool() && !r_useShadowMapping.GetBool() )
				{
					auto staticShadowParms = allocManager.FrameAlloc<staticShadowVolumeParms_t, FRAME_ALLOC_SHADOW_VOLUME_PARMS>();

					staticShadowParms->verts = tri->staticShadowVertexes;
					staticShadowParms->numVerts = tri->numVerts * 2;
					staticShadowParms->indexes = surfInter->shadowIndexes;
					staticShadowParms->numIndexes = surfInter->numShadowIndexes;
					staticShadowParms->numShadowIndicesWithCaps = surfInter->numShadowIndexes;
					staticShadowParms->numShadowIndicesNoCaps = surfInter->numShadowIndexesNoCaps;
					staticShadowParms->triangleBounds = tri->GetBounds();
					staticShadowParms->triangleMVP = vEntity->mvp;
					staticShadowParms->localLightOrigin = localLightOrigin;
					staticShadowParms->localViewOrigin = localViewOrigin;
					staticShadowParms->zNear = znear;
					staticShadowParms->lightZMin = vLight->scissorRect.zmin;
					staticShadowParms->lightZMax = vLight->scissorRect.zmax;
					staticShadowParms->forceShadowCaps = forceShadowCaps;
					staticShadowParms->useShadowPreciseInsideTest = r_useShadowPreciseInsideTest.GetBool();
					staticShadowParms->useShadowDepthBounds = r_useShadowDepthBounds.GetBool();
					staticShadowParms->numShadowIndices = & shadowDrawSurf->numIndexes;
					staticShadowParms->renderZFail = & shadowDrawSurf->renderZFail;
					staticShadowParms->shadowZMin = & shadowDrawSurf->scissorRect.zmin;
					staticShadowParms->shadowZMax = & shadowDrawSurf->scissorRect.zmax;
					staticShadowParms->shadowVolumeState = & shadowDrawSurf->shadowVolumeState;

					shadowDrawSurf->shadowVolumeState = SHADOWVOLUME_UNFINISHED;

					staticShadowParms->next = vEntity->staticShadowVolumes;
					vEntity->staticShadowVolumes = staticShadowParms;
				}
			}
			else
			{
				// When CPU skinning the dynamic shadow verts of a dynamic model may not have been copied to buffer memory yet.
				if( !vertexCache.CacheIsCurrent( tri->shadowCache ) )
				{
					assert( !gpuSkinned );	// the shadow cache should be static when using GPU skinning
					// Extracts just the xyz values from a set of full size drawverts, and
					// duplicates them with w set to 0 and 1 for the vertex program to project.
					// This is constant for any number of lights, the vertex program takes care
					// of projecting the verts to infinity for a particular light.
					tri->shadowCache = vertexCache.AllocVertex( NULL, ALIGN( tri->numVerts * 2 * sizeof( idShadowVert ), VERTEX_CACHE_ALIGN ) );
					idShadowVert* shadowVerts = ( idShadowVert* )vertexCache.MappedVertexBuffer( tri->shadowCache );
					idShadowVert::CreateShadowCache( shadowVerts, tri->verts, tri->numVerts );
				}

				const int maxShadowVolumeIndexes = tri->numSilEdges * 6 + tri->numIndexes * 2;

				shadowDrawSurf->numIndexes = 0;
				shadowDrawSurf->indexCache = vertexCache.AllocIndex( NULL, ALIGN( maxShadowVolumeIndexes * sizeof( triIndex_t ), INDEX_CACHE_ALIGN ) );
				shadowDrawSurf->shadowCache = tri->shadowCache;
				shadowDrawSurf->scissorRect = vLight->scissorRect;		// default to the light scissor and light depth bounds
				shadowDrawSurf->shadowVolumeState = SHADOWVOLUME_DONE;	// assume the shadow volume is done in case the index cache allocation failed

				// if the index cache was successfully allocated then setup the parms to create a shadow volume in parallel
				if( vertexCache.CacheIsCurrent( shadowDrawSurf->indexCache ) && !r_skipDynamicShadows.GetBool() && !r_useShadowMapping.GetBool() )
				{
					// if the parms were not already allocated for culling interaction triangles to the light frustum
					if( dynamicShadowParms == NULL )
					{
						dynamicShadowParms = allocManager.FrameAlloc<dynamicShadowVolumeParms_t, FRAME_ALLOC_SHADOW_VOLUME_PARMS>();
					}
					else
					{ // the shadow volume will be rendered first so when the interaction surface is drawn the triangles have been culled for sure
						*dynamicShadowParms->shadowVolumeState = SHADOWVOLUME_DONE;
					}

					dynamicShadowParms->verts = tri->verts;
					dynamicShadowParms->numVerts = tri->numVerts;
					dynamicShadowParms->indexes = tri->indexes;
					dynamicShadowParms->numIndexes = tri->numIndexes;
					dynamicShadowParms->silEdges = tri->silEdges;
					dynamicShadowParms->numSilEdges = tri->numSilEdges;
					dynamicShadowParms->joints = gpuSkinned ? tri->staticModelWithJoints->jointsInverted : NULL;
					dynamicShadowParms->numJoints = gpuSkinned ? tri->staticModelWithJoints->numInvertedJoints : 0;
					dynamicShadowParms->triangleBounds = tri->GetBounds();
					dynamicShadowParms->triangleMVP = vEntity->mvp;
					dynamicShadowParms->localLightOrigin = localLightOrigin;
					dynamicShadowParms->localViewOrigin = localViewOrigin;
					idRenderMatrix::Multiply( vLight->lightDef->baseLightProject, entityDef->modelRenderMatrix, dynamicShadowParms->localLightProject );
					dynamicShadowParms->zNear = znear;
					dynamicShadowParms->lightZMin = vLight->scissorRect.zmin;
					dynamicShadowParms->lightZMax = vLight->scissorRect.zmax;
					dynamicShadowParms->cullShadowTrianglesToLight = r_cullDynamicShadowTriangles.GetBool();
					dynamicShadowParms->forceShadowCaps = forceShadowCaps;
					dynamicShadowParms->useShadowPreciseInsideTest = r_useShadowPreciseInsideTest.GetBool();
					dynamicShadowParms->useShadowDepthBounds = r_useShadowDepthBounds.GetBool();
					dynamicShadowParms->tempFacing = NULL;
					dynamicShadowParms->tempCulled = NULL;
					dynamicShadowParms->tempVerts = NULL;
					dynamicShadowParms->indexBuffer = NULL;
					dynamicShadowParms->shadowIndices = ( triIndex_t* )vertexCache.MappedIndexBuffer( shadowDrawSurf->indexCache );
					dynamicShadowParms->maxShadowIndices = maxShadowVolumeIndexes;
					dynamicShadowParms->numShadowIndices = & shadowDrawSurf->numIndexes;
					// dynamicShadowParms->lightIndices may have already been set for the interaction surface
					// dynamicShadowParms->maxLightIndices may have already been set for the interaction surface
					// dynamicShadowParms->numLightIndices may have already been set for the interaction surface
					dynamicShadowParms->renderZFail = & shadowDrawSurf->renderZFail;
					dynamicShadowParms->shadowZMin = & shadowDrawSurf->scissorRect.zmin;
					dynamicShadowParms->shadowZMax = & shadowDrawSurf->scissorRect.zmax;
					dynamicShadowParms->shadowVolumeState = & shadowDrawSurf->shadowVolumeState;

					shadowDrawSurf->shadowVolumeState = SHADOWVOLUME_UNFINISHED;

					// if the parms we not already linked for culling interaction triangles to the light frustum
					if( dynamicShadowParms->lightIndices == NULL )
					{
						dynamicShadowParms->next = vEntity->dynamicShadowVolumes;
						vEntity->dynamicShadowVolumes = dynamicShadowParms;
					}

					tr.pc.c_createShadowVolumes++;
				}
			}

			assert( vertexCache.CacheIsCurrent( shadowDrawSurf->shadowCache ) );
			assert( vertexCache.CacheIsCurrent( shadowDrawSurf->indexCache ) );

			shadowDrawSurf->vertexCache = 0;
			shadowDrawSurf->frontEndGeo = NULL;
			shadowDrawSurf->space = vEntity;
			shadowDrawSurf->material = NULL;
			shadowDrawSurf->extraGLState = 0;
			shadowDrawSurf->sort = 0.0f;
			shadowDrawSurf->shaderRegisters = NULL;

			R_SetupDrawSurfJoints( shadowDrawSurf, tri, NULL );

			// determine which linked list to add the shadow surface to
			shadowDrawSurf->linkChain = material->TestMaterialFlag( MF_NOSELFSHADOW )? &vLight->localShadows : &vLight->globalShadows;

			shadowDrawSurf->nextOnLight = vEntity->drawSurfs;
			vEntity->drawSurfs = shadowDrawSurf;
		}
	}
}

REGISTER_PARALLEL_JOB( R_AddSingleModel, "R_AddSingleModel" );

/*
==================
 R_SortViewEntities
==================
*/
static viewModel_t* R_SortViewEntities( viewModel_t* vEntities )
{
	SCOPED_PROFILE_EVENT( "R_SortViewEntities" );

	// We want to avoid having a single AddModel for something complex be
	// the last thing processed and hurt the parallel occupancy, so
	// sort dynamic models first, _area models second, then everything else.
	viewModel_t* dynamics = NULL;
	viewModel_t* areas = NULL;
	viewModel_t* others = NULL;
	for( viewModel_t* vEntity = vEntities; vEntity != NULL; )
	{
		viewModel_t* next = vEntity->next;
		const idRenderModel* model = vEntity->entityDef->GetModel();
		if( model->IsDynamicModel() != DM_STATIC )
		{
			vEntity->next = dynamics;
			dynamics = vEntity;
		}
		else if( model->IsStaticWorldModel() )
		{
			vEntity->next = areas;
			areas = vEntity;
		}
		else {
			vEntity->next = others;
			others = vEntity;
		}
		vEntity = next;
	}

	// concatenate the lists
	viewModel_t* all = others;

	for( viewModel_t* vEntity = areas; vEntity != NULL; )
	{
		viewModel_t* next = vEntity->next;

		vEntity->next = all;
		all = vEntity;

		vEntity = next;
	}

	for( viewModel_t* vEntity = dynamics; vEntity != NULL; )
	{
		viewModel_t* next = vEntity->next;

		vEntity->next = all;
		all = vEntity;

		vEntity = next;
	}

	return all;
}

/*
=================
 R_LinkDrawSurfToView

	Also called directly by GuiModel
=================
*/
void R_LinkDrawSurfToView( drawSurf_t* drawSurf, idRenderView* viewDef )
{
	// if it doesn't fit, resize the list
	if( viewDef->numDrawSurfs == viewDef->maxDrawSurfs )
	{
		drawSurf_t** old = viewDef->drawSurfs;
		int count;

		if( viewDef->maxDrawSurfs == 0 )
		{
			viewDef->maxDrawSurfs = INITIAL_DRAWSURFS;
			count = 0;
		}
		else {
			count = viewDef->maxDrawSurfs * sizeof( viewDef->drawSurfs[0] );
			viewDef->maxDrawSurfs *= 2;
		}

		viewDef->drawSurfs = allocManager.FrameAlloc<drawSurf_t*, FRAME_ALLOC_DRAW_SURFACE_POINTER>( viewDef->maxDrawSurfs );
		memcpy( viewDef->drawSurfs, old, count );
	}

	viewDef->drawSurfs[ viewDef->numDrawSurfs ] = drawSurf;
	viewDef->numDrawSurfs++;
}

/*
===================
 R_AddModels

	The end result of running this is the addition of drawSurf_t to the
	tr.viewDef->drawSurfs[] array and light link chains, along with
	frameData and vertexCache allocations to support the drawSurfs.
===================
*/
void R_AddModels( idRenderView * const view )
{
	SCOPED_PROFILE_EVENT( "R_AddModels" );

	view->viewEntitys = R_SortViewEntities( view->viewEntitys );

	//-------------------------------------------------
	// Go through each view entity that is either visible to the view, or to
	// any light that intersects the view (for shadows).
	//-------------------------------------------------

	if( r_useParallelAddModels.GetBool() )
	{
		for( viewModel_t* vEntity = view->viewEntitys; vEntity != NULL; vEntity = vEntity->next )
		{
			tr.frontEndJobList->AddJob( ( jobRun_t )R_AddSingleModel, vEntity );
		}
		tr.frontEndJobList->Submit();
		tr.frontEndJobList->Wait();
	}
	else
	{
		for( viewModel_t* vEntity = view->viewEntitys; vEntity != NULL; vEntity = vEntity->next )
		{
			R_AddSingleModel( vEntity );
		}
	}

	//-------------------------------------------------
	// Kick off jobs to setup static and dynamic shadow volumes.
	//-------------------------------------------------
	if( ( r_skipStaticShadows.GetBool() && r_skipDynamicShadows.GetBool() ) || r_useShadowMapping.GetBool() )
	{
		// no shadow volumes were chained to any entity, all are in DONE state, we don't need to Submit() or Wait()
	}
	else
	{
		if( r_useParallelAddShadows.GetInteger() == 1 )
		{
			for( viewModel_t* vEntity = view->viewEntitys; vEntity != NULL; vEntity = vEntity->next )
			{
				for( staticShadowVolumeParms_t* shadowParms = vEntity->staticShadowVolumes; shadowParms != NULL; shadowParms = shadowParms->next )
				{
					tr.frontEndJobList->AddJob( ( jobRun_t )StaticShadowVolumeJob, shadowParms );
				}
				for( dynamicShadowVolumeParms_t* shadowParms = vEntity->dynamicShadowVolumes; shadowParms != NULL; shadowParms = shadowParms->next )
				{
					tr.frontEndJobList->AddJob( ( jobRun_t )DynamicShadowVolumeJob, shadowParms );
				}
				vEntity->staticShadowVolumes = NULL;
				vEntity->dynamicShadowVolumes = NULL;
			}
			tr.frontEndJobList->Submit();
			// wait here otherwise the shadow volume index buffer may be unmapped before all shadow volumes have been constructed
			tr.frontEndJobList->Wait();
		}
		else
		{
			int start = sys->Microseconds();

			for( viewModel_t* vEntity = view->viewEntitys; vEntity != NULL; vEntity = vEntity->next )
			{
				for( staticShadowVolumeParms_t* shadowParms = vEntity->staticShadowVolumes; shadowParms != NULL; shadowParms = shadowParms->next )
				{
					StaticShadowVolumeJob( shadowParms );
				}
				for( dynamicShadowVolumeParms_t* shadowParms = vEntity->dynamicShadowVolumes; shadowParms != NULL; shadowParms = shadowParms->next )
				{
					DynamicShadowVolumeJob( shadowParms );
				}
				vEntity->staticShadowVolumes = NULL;
				vEntity->dynamicShadowVolumes = NULL;
			}

			int end = sys->Microseconds();
			backEnd.pc.shadowMicroSec += end - start;
		}
	}

	//-------------------------------------------------
	// Move the draw surfs to the view.
	//-------------------------------------------------

	view->numDrawSurfs = 0;	// clear the ambient surface list
	view->maxDrawSurfs = 0;	// will be set to INITIAL_DRAWSURFS on R_LinkDrawSurfToView

	for( viewModel_t* vEntity = view->viewEntitys; vEntity != NULL; vEntity = vEntity->next )
	{
		for( drawSurf_t* ds = vEntity->drawSurfs; ds != NULL; )
		{
			drawSurf_t* next = ds->nextOnLight;
			if( ds->linkChain == NULL )
			{
				R_LinkDrawSurfToView( ds, view );
			}
			else {
				ds->nextOnLight = *ds->linkChain;
				*ds->linkChain = ds;
			}
			ds = next;
		}
		vEntity->drawSurfs = NULL;
	}
}
