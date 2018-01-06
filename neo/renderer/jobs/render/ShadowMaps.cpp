
#include "precompiled.h"
#pragma hdrstop

#include "Render.h"

/*
==============================================================================================

	SHADOW MAPS RENDERING

	idRenderMatrix modelToShadowMatrix;
	idRenderMatrix::Multiply( backEnd.shadowV[0], surf->space->modelMatrix, modelToShadowMatrix );

	idRenderMatrix shadowClipMVP;
	idRenderMatrix::Multiply( backEnd.shadowP[0], modelToShadowMatrix, shadowClipMVP );

	idRenderMatrix shadowWindowMVP;
	idRenderMatrix::Multiply( renderMatrix_clipSpaceToWindowSpace, shadowClipMVP, shadowWindowMVP );

	idBounds tmp;
	idRenderMatrix::ProjectedBounds( tmp, vLight->inverseBaseLightProject, bounds_zeroOneCube, false );
	idRenderMatrix::ProjectedBounds( lightBounds, lightViewRenderMatrix, tmp, false );

	idBounds tmp;
	idRenderMatrix::ProjectedBounds( tmp, splitFrustumInverse, bounds_unitCube, false );
	idRenderMatrix::ProjectedBounds( cropBounds, lightViewRenderMatrix, tmp, false );


	The y-axis of clip space points upwards while the y axis in texture space points downwards: 
	top left in clip space is (-1,1) whereas top left in texture coordinates is (0,0). Luckily we 
	can extend the combined shadow view and projection matrix to do the remapping:

	// compute block index into shadow atlas
	int tileX = i % 2;
	int tileY = i / 2;

	// tile matrix: maps from clip space to shadow atlas block
	var tileMatrix = Matrix.Identity;
	tileMatrix.M11 = 0.25f;
	tileMatrix.M22 = -0.25f;
	tileMatrix.Translation = new Vector3(0.25f + tileX * 0.5f, 0.25f + tileY * 0.5f, 0);

	// now combine with shadow view and projection
	var ShadowTransform = ShadowView * ShadowProjection * tileMatrix;

==============================================================================================
*/

idCVar r_useSinglePassShadowMaps( "r_useSinglePassShadowMaps", "0", CVAR_RENDERER | CVAR_BOOL, "" );

#if 0
void CalcCascadeSplits( idRenderView * view, const viewLight_t * const vLight, idVec2 reductionDepth )
{
	bool PartitionMode_Manual;
	bool PartitionMode_Logarithmic;
	bool PartitionMode_PSSM;
	float PSSMLambda;
	bool AutoComputeDepthBounds;
	bool StabilizeCascades;
	const uint32 ShadowMapSize = AppSettings::ShadowMapResolution();
	float FixedFilterKernelSize;
	float MinCascadeDistance;
	float MaxCascadeDistance;
	float SplitDistance0;
	float SplitDistance1;
	float SplitDistance2;
	float SplitDistance3;
	float SplitDistance4;

	const float cameraNearClip = 0;
	const float cameraFarClip = 0;

	const float MinDistance = AutoComputeDepthBounds ? reductionDepth.x : MinCascadeDistance;
	const float MaxDistance = AutoComputeDepthBounds ? reductionDepth.y : MaxCascadeDistance;

	// Compute the split distances based on the partitioning mode
	float CascadeSplits[ 5 ] = { 0.0f };
	uint32 NumCascades;

	if( PartitionMode_Manual )
	{
		CascadeSplits[ 0 ] = MinDistance + SplitDistance0 * MaxDistance;
		CascadeSplits[ 1 ] = MinDistance + SplitDistance1 * MaxDistance;
		CascadeSplits[ 2 ] = MinDistance + SplitDistance2 * MaxDistance;
		CascadeSplits[ 3 ] = MinDistance + SplitDistance3 * MaxDistance;
		CascadeSplits[ 4 ] = MinDistance + SplitDistance4 * MaxDistance;
	}
	else if( PartitionMode_Logarithmic || PartitionMode_PSSM )
	{
		float lambda = 1.0f;
		if( PartitionMode_PSSM )
			lambda = PSSMLambda;

		float nearClip = view->GetZNear();
		float farClip = 10000.0f;
		float clipRange = farClip - nearClip;

		float minZ = nearClip + MinDistance * clipRange;
		float maxZ = nearClip + MaxDistance * clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		for( uint32 i = 0; i < NumCascades; ++i )
		{
			const float p = ( i + 1 ) / static_cast<float>( NumCascades );
			const float log = minZ * idMath::Pow( ratio, p );
			const float uniform = minZ + range * p;
			const float d = lambda * ( log - uniform ) + uniform;
			CascadeSplits[ i ] = ( d - nearClip ) / clipRange;
		}
	}

	idRenderMatrix lightViewRenderMatrix, lightProjectionRenderMatrix;

	// original light direction is from surface to light origin
	idVec3 lightDir = -vLight->lightCenter;
	if( lightDir.Normalize() == 0.0f )
	{
		lightDir[ 2 ] = -1.0f;
	}
	idRenderMatrix::CreateViewMatrix( view->GetOrigin(), lightDir.ToMat3(), lightViewRenderMatrix );

	for( uint32 cascadeIdx = 0; cascadeIdx < NumCascades; ++cascadeIdx )
	{
		// Get the 8 points of the view frustum in world space
		idVec3 frustumCornersWS[ 8 ] =
		{
			idVec3( -1.0f,  1.0f, 0.0f ),
			idVec3( 1.0f,  1.0f, 0.0f ),
			idVec3( 1.0f, -1.0f, 0.0f ),
			idVec3( -1.0f, -1.0f, 0.0f ),
			idVec3( -1.0f,  1.0f, 1.0f ),
			idVec3( 1.0f,  1.0f, 1.0f ),
			idVec3( 1.0f, -1.0f, 1.0f ),
			idVec3( -1.0f, -1.0f, 1.0f ),
		};

		float prevSplitDist = cascadeIdx == 0 ? MinDistance : CascadeSplits[ cascadeIdx - 1 ];
		float splitDist = CascadeSplits[ cascadeIdx ];

		for( uint32 i = 0; i < 8; ++i )
		{
			//frustumCornersWS[ i ] = Float3::Transform( frustumCornersWS[ i ], view->GetInverseVPMatrix() );
			view->GetInverseVPMatrix().TransformPoint( frustumCornersWS[ i ], frustumCornersWS[ i ] );
		}

		// Get the corners of the current cascade slice of the view frustum
		for( uint32 i = 0; i < 4; ++i )
		{
			idVec3 cornerRay = frustumCornersWS[ i + 4 ] - frustumCornersWS[ i ];
			idVec3 nearCornerRay = cornerRay * prevSplitDist;
			idVec3 farCornerRay = cornerRay * splitDist;
			frustumCornersWS[ i + 4 ] = frustumCornersWS[ i ] + farCornerRay;
			frustumCornersWS[ i ] = frustumCornersWS[ i ] + nearCornerRay;
		}

		// Calculate the centroid of the view frustum slice
		idVec3 frustumCenter( 0.0f );
		for( uint32 i = 0; i < 8; ++i )
			frustumCenter = frustumCenter + frustumCornersWS[ i ];
		frustumCenter *= 1.0f / 8.0f;

		// Pick the up vector to use for the light camera.
		// axis vectors are [0] = forward, [1] = left, [2] = up
		idVec3 upDir = view->GetAxis()[2];//.Right();	?????

		idVec3 minExtents;
		idVec3 maxExtents;
		if( StabilizeCascades )
		{
			// This needs to be constant for it to be stable
			upDir = idVec3( 0.0f, 1.0f, 0.0f );

			// Calculate the radius of a bounding sphere surrounding the frustum corners
			float sphereRadius = 0.0f;
			for( uint32 i = 0; i < 8; ++i )
			{
				float dist = ( frustumCornersWS[ i ] - frustumCenter ).Length();
				sphereRadius = std::max( sphereRadius, dist );
			}

			sphereRadius = std::ceil( sphereRadius * 16.0f ) / 16.0f;

			maxExtents = idVec3( sphereRadius, sphereRadius, sphereRadius );
			minExtents = -maxExtents;
		}
		else
		{
			// Create a temporary view matrix for the light
			idVec3 lightCameraPos = frustumCenter;
			idVec3 lookAt = frustumCenter - lightDir;
			idMat4 lightView = XMMatrixLookAtLH( lightCameraPos.ToSIMD(), lookAt.ToSIMD(), upDir.ToSIMD() );

			// Calculate an AABB around the frustum corners
			idVec3 mins( FLT_MAX );
			idVec3 maxes(-FLT_MAX );
			for( uint32 i = 0; i < 8; ++i )
			{
				idVec3 corner = Float3::Transform( frustumCornersWS[ i ], lightView );
				mins = XMVectorMin( mins.ToSIMD(), corner.ToSIMD() );
				maxes = XMVectorMax( maxes.ToSIMD(), corner.ToSIMD() );
			}

			minExtents = mins;
			maxExtents = maxes;

			// Adjust the min/max to accommodate the filtering size
			float scale = ( ShadowMapSize + FixedFilterKernelSize ) / static_cast<float>( ShadowMapSize );
			minExtents.x *= scale;
			minExtents.y *= scale;
			maxExtents.x *= scale;
			maxExtents.y *= scale;
		}

		idVec3 cascadeExtents = maxExtents - minExtents;

		// Get position of the shadow camera
		idVec3 shadowCameraPos = frustumCenter + AppSettings::LightDirection.Value() * -minExtents.z;

		if( StabilizeCascades )
		{
			// Create the rounding matrix, by projecting the world-space origin and determining
			// the fractional offset in texel space
			XMMATRIX shadowMatrix = shadowCamera.ViewProjectionMatrix().ToSIMD();
			XMVECTOR shadowOrigin = XMVectorSet( 0.0f, 0.0f, 0.0f, 1.0f );
			shadowOrigin = XMVector4Transform( shadowOrigin, shadowMatrix );
			shadowOrigin = XMVectorScale( shadowOrigin, sMapSize / 2.0f );

			XMVECTOR roundedOrigin = XMVectorRound( shadowOrigin );
			XMVECTOR roundOffset = XMVectorSubtract( roundedOrigin, shadowOrigin );
			roundOffset = XMVectorScale( roundOffset, 2.0f / sMapSize );
			roundOffset = XMVectorSetZ( roundOffset, 0.0f );
			roundOffset = XMVectorSetW( roundOffset, 0.0f );

			XMMATRIX shadowProj = shadowCamera.ProjectionMatrix().ToSIMD();
			shadowProj.r[ 3 ] = XMVectorAdd( shadowProj.r[ 3 ], roundOffset );
			shadowCamera.SetProjection( shadowProj );
		}

	}
}
#endif

/*
=====================
 idRenderMatrix is left-handed !?

	D3DXMatrixOrthoOffCenterLH
	https://msdn.microsoft.com/en-us/library/bb205347(v=vs.85).aspx

	Builds a customized, left-handed orthographic projection matrix.
=====================
*/
static void idRenderMatrix_CreateOrthogonalOffCenterProjection(
	float left, 	// Minimum x-value of view volume.	
	float right, 	// Maximum x-value of view volume.	
	float bottom,	// Minimum y-value of view volume.		
	float top,		// Maximum y-value of view volume. 		
	float zNear, float zFar,
	idRenderMatrix & out )
{
	out[ 0 ][ 0 ] = 2.0f / ( right - left );
	out[ 0 ][ 1 ] = 0.0f;
	out[ 0 ][ 2 ] = 0.0f;
	//out[ 0 ][ 3 ] = ( left + right ) / ( left - right );
	out[ 0 ][ 3 ] = -( left + right ) / ( right - left );

	out[ 1 ][ 0 ] = 0.0f;
	out[ 1 ][ 1 ] = 2.0f / ( top - bottom );
	out[ 1 ][ 2 ] = 0.0f;
	//out[ 1 ][ 3 ] = ( top + bottom ) / ( bottom - top );
	out[ 1 ][ 3 ] = -( top + bottom ) / ( top - bottom );

	out[ 2 ][ 0 ] = 0.0f;
	out[ 2 ][ 1 ] = 0.0f;
	out[ 2 ][ 2 ] = 1.0f / ( zFar - zNear );
	out[ 2 ][ 3 ] = zNear / ( zNear - zFar );

	out[ 3 ][ 0 ] = 0.0f;
	out[ 3 ][ 1 ] = 0.0f;
	out[ 3 ][ 2 ] = 0.0f;
	out[ 3 ][ 3 ] = 1.0f;
}

/*
=====================
 GetParallelLightViewProjMatrices
=====================
*/
static void GetParallelLightViewProjMatrices( const viewLight_t * const vLight, idRenderMatrix shadowVP[ MAX_FRUSTUMS-1 ] )
{
	idRenderMatrix lightViewRenderMatrix, lightProjectionRenderMatrix;

	// original light direction is from surface to light origin
	idVec3 lightDir = -vLight->lightCenter;
	if( lightDir.Normalize() == 0.0f ) {
		lightDir[ 2 ] = -1.0f;
	}
	idRenderMatrix::CreateViewMatrix( backEnd.viewDef->GetOrigin(), lightDir.ToMat3(), lightViewRenderMatrix );

	idBounds lightBounds;
	{
		idRenderMatrix transform;
		idRenderMatrix::Multiply( lightViewRenderMatrix, vLight->inverseBaseLightProject, transform );
		idRenderMatrix::ProjectedBounds( lightBounds, transform, bounds_zeroOneCube, false );
	}
	///idRenderMatrix_CreateOrthogonalOffCenterProjection(
	///	lightBounds[ 0 ][ 0 ], lightBounds[ 1 ][ 0 ], lightBounds[ 0 ][ 1 ], lightBounds[ 1 ][ 1 ], -lightBounds[ 1 ][ 2 ], -lightBounds[ 0 ][ 2 ],
	///	lightProjectionRenderMatrix );

	// 'frustumMVP' goes from global space -> camera local space -> camera projective space
	// invert the MVP projection so we can deform zero-to-one cubes into the frustum pyramid shape and calculate global bounds
	for( int i = 0; i < MAX_FRUSTUMS-1; i++ )
	{
		const idRenderMatrix & splitFrustumInverse = backEnd.viewDef->GetSplitFrustumInvMVPMatrices()[ FRUSTUM_CASCADE1 + i ];

		idBounds cropBounds;
		{
			idRenderMatrix transform;
			idRenderMatrix::Multiply( lightViewRenderMatrix, splitFrustumInverse, transform );
			idRenderMatrix::ProjectedBounds( cropBounds, transform, bounds_unitCube, false );
		}
		// don't let the frustum AABB be bigger than the light AABB
		cropBounds.IntersectXYSelf( lightBounds );

		cropBounds[ 0 ].z = lightBounds[ 0 ].z;
		cropBounds[ 1 ].z = lightBounds[ 1 ].z;

	#if 0
		if( 0 ) {

			// idVec3 points[ 8 ];
			// cropBounds.ToPoints( points );

			// float diagonalLength;// As Single = ( frustumCornersWS( 0 ) - frustumCornersWS( 6 ) ).Length()
			// 
			// float worldsUnitsPerTexel = diagonalLength / float( shadowMapResolutions[ i ] );
			// idVec3 vBorderOffset = ( idVec3( diagonalLength, diagonalLength, diagonalLength ) - ( cropBounds.GetMaxs() - cropBounds.GetMins() ) ) * 0.5f;
			// cropBounds[1]/*Maximum*/ += vBorderOffset;
			// cropBounds[0]/*Minimum*/ -= vBorderOffset;
			// 
			// cropBounds[0] /= worldsUnitsPerTexel;
			// cropBounds[0].x = idMath::Floor( cropBounds[0].x );
			// cropBounds[0].y = idMath::Floor( cropBounds[0].y );
			// cropBounds[0].z = idMath::Floor( cropBounds[0].z );
			// cropBounds[0] *= worldsUnitsPerTexel;
			// 
			// cropBounds[1] /= worldsUnitsPerTexel;
			// cropBounds[1].x = idMath::Floor( cropBounds[1].x );
			// cropBounds[1].y = idMath::Floor( cropBounds[1].y );
			// cropBounds[1].z = idMath::Floor( cropBounds[1].z );
			// cropBounds[1] *= worldsUnitsPerTexel;*/

			auto Math_IEEERemainder = []( double dividend, double divisor ) {
				return dividend - ( divisor * idMath::Rint( dividend / divisor ) );
			};

			float shadowMapSize = shadowMapResolutions[ /*MAX_SHADOWMAP_RESOLUTIONS -*/ i ];

			float quantizationStep = 1.0f / shadowMapSize;
			float qx = ( float )Math_IEEERemainder( cropBounds[ 0 ].x, quantizationStep );
			float qy = ( float )Math_IEEERemainder( cropBounds[ 0 ].y, quantizationStep );

			cropBounds[ 0 ].x -= qx;
			cropBounds[ 0 ].y -= qy;

			cropBounds[ 1 ].x += shadowMapSize;
			cropBounds[ 1 ].y += shadowMapSize;
		}
	#endif

	#if 1
		idRenderMatrix_CreateOrthogonalOffCenterProjection(
			cropBounds[ 0 ][ 0 ], cropBounds[ 1 ][ 0 ], cropBounds[ 0 ][ 1 ], cropBounds[ 1 ][ 1 ], cropBounds[ 1 ][ 2 ], cropBounds[ 0 ][ 2 ],
			lightProjectionRenderMatrix );
	#else
		idRenderMatrix::CreateOrthogonalOffCenterProjection(
			cropBounds[ 0 ][ 0 ], cropBounds[ 1 ][ 0 ], cropBounds[ 0 ][ 1 ], cropBounds[ 1 ][ 1 ], cropBounds[ 1 ][ 2 ], cropBounds[ 0 ][ 2 ],
			lightProjectionRenderMatrix );
	#endif
		idRenderMatrix::Multiply( lightProjectionRenderMatrix, lightViewRenderMatrix, shadowVP[ i ] );
	}
}

/*
=====================
 GetPointLightViewProjMatrices
=====================
*/
static void GetPointLightViewProjMatrices( const viewLight_t * const vLight, idRenderMatrix shadowVP[ 6 ] )
{
	idRenderMatrix lightViewRenderMatrix, lightProjectionRenderMatrix;

	static const idMat3 SM_VAxis[ 6 ] = {
		idMat3( 1, 0, 0, 0, 0, 1, 0,-1, 0 ),
		idMat3(-1, 0, 0, 0, 0,-1, 0,-1, 0 ),
		idMat3( 0, 1, 0,-1, 0, 0, 0, 0, 1 ),
		idMat3( 0,-1, 0,-1, 0, 0, 0, 0,-1 ),
		idMat3( 0, 0, 1,-1, 0, 0, 0,-1, 0 ),
		idMat3( 0, 0,-1, 1, 0, 0, 0,-1, 0 )
	};
	for( int i = 0; i < 6; i++ )
	{
		idRenderMatrix::CreateViewMatrix( vLight->globalLightOrigin, SM_VAxis[ i ], lightViewRenderMatrix );

		// set up 90 degree projection matrix
		const float zNear = 4.0f;
		const float	fov = r_shadowMapFrustumFOV.GetFloat();

		idRenderMatrix::CreateProjectionMatrixFov( fov, fov, zNear, 0.0f, 0.0f, 0.0f, lightProjectionRenderMatrix );
		idRenderMatrix::Multiply( lightProjectionRenderMatrix, lightViewRenderMatrix, shadowVP[ i ] );
	}
}

/*
=====================
 GetSpotLightViewProjMatrices
=====================
*/
static void GetSpotLightViewProjMatrix( const viewLight_t * const vLight, idRenderMatrix & shadowVP )
{
	idRenderMatrix lightViewRenderMatrix, lightProjectionRenderMatrix;

	lightViewRenderMatrix.Identity();
	idRenderMatrix::Multiply( renderMatrix_windowSpaceToClipSpace, vLight->baseLightProject, lightProjectionRenderMatrix );
	idRenderMatrix::Multiply( lightProjectionRenderMatrix, lightViewRenderMatrix, shadowVP );
}

/*
=====================
 RenderShadowBuffer
=====================
*/
//static void RenderShadowBufferGeneric( const drawSurf_t * const * drawSurfs, uint32 numDrawSurfs, const idRenderMatrix & smVPMatrix )
static void RenderShadowBufferGeneric( const drawSurf_t * const drawSurfs, const idRenderMatrix & smVPMatrix )
{
	// process the chain of shadows with the current rendering state
	backEnd.currentSpace = NULL;

	//for( int i = 0; i < numDrawSurfs; i++ ) {
	//	const drawSurf_t* const drawSurf = drawSurfs[ i ];

	for( const drawSurf_t* drawSurf = drawSurfs; drawSurf != NULL; drawSurf = drawSurf->nextOnLight ) {

		if( drawSurf->numIndexes == 0 )
			continue;	// a job may have created an empty shadow geometry

		if( drawSurf->space != backEnd.currentSpace )
		{
		#if 0
			///idRenderMatrix smLightVP;
			///idRenderMatrix::Multiply( lightProjectionRenderMatrix, lightViewRenderMatrix, smLightVP );
			///
			///idRenderMatrix clipMVP;
			///idRenderMatrix::Multiply( smLightVP, drawSurf->space->modelMatrix, clipMVP );

			if( vLight->parallel )
			{
				///idRenderMatrix MVP;
				///idRenderMatrix::Multiply( renderMatrix_clipSpaceToWindowSpace, clipMVP, MVP );

				idRenderMatrix clipMVP;
				idRenderMatrix::Multiply( backEnd.shadowVP[ side ], drawSurf->space->modelMatrix, clipMVP );

				RB_SetMVP( clipMVP );
			}
			else if( side < 0 ) // spot
			{
				idRenderMatrix clipMVP;
				idRenderMatrix::Multiply( backEnd.shadowVP[ 0 ], drawSurf->space->modelMatrix, clipMVP );

				/// from OpenGL view space to OpenGL NDC ( -1 : 1 in XYZ )
				///idRenderMatrix MVP;
				///idRenderMatrix::Multiply( renderMatrix_windowSpaceToClipSpace, clipMVP, MVP );		
				///RB_SetMVP( MVP );

				RB_SetMVP( clipMVP );
			}
			else
			{
				idRenderMatrix clipMVP;
				idRenderMatrix::Multiply( backEnd.shadowVP[ side ], drawSurf->space->modelMatrix, clipMVP );

				RB_SetMVP( clipMVP );
			}
		#endif
			idRenderMatrix clipMVP;

			///const idRenderMatrix & smVPMatrix = backEnd.shadowVP[ ( side < 0 ) ? 0 : side ];
			idRenderMatrix::Multiply( smVPMatrix, drawSurf->space->modelMatrix, clipMVP );

			RB_SetMVP( clipMVP );

			backEnd.currentSpace = drawSurf->space;
		}

		const idMaterial* const material = drawSurf->material;
		release_assert( material != nullptr );

		RENDERLOG_OPEN_BLOCK( material->GetName() );

		bool didDraw = false;

		// get the expressions for conditionals / color / texcoords
		const float* regs = drawSurf->shaderRegisters;
		idRenderVector color( 0, 0, 0, 1 );

		uint64 surfGLState = 0;

		// set polygon offset if necessary
		if( material && material->TestMaterialFlag( MF_POLYGONOFFSET ) )
		{
			surfGLState |= GLS_POLYGON_OFFSET;
			GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * material->GetPolygonOffset() );
		}

		if( material && material->Coverage() == MC_PERFORATED )
		{
			// perforated surfaces may have multiple alpha tested stages
			for( int stage = 0; stage < material->GetNumStages(); stage++ )
			{
				auto pStage = material->GetStage( stage );

				if( !pStage->hasAlphaTest )
					continue;

				// check the stage enable condition
				if( regs[ pStage->conditionRegister ] == 0 )
					continue;

				// if we at least tried to draw an alpha tested stage,
				// we won't draw the opaque surface
				didDraw = true;

				// set the alpha modulate
				color[ 3 ] = regs[ pStage->color.registers[ 3 ] ];

				// skip the entire stage if alpha would be black
				if( color[ 3 ] <= 0.0f )
					continue;

				uint64 stageGLState = surfGLState;

				// set privatePolygonOffset if necessary
				if( pStage->privatePolygonOffset )
				{
					GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * pStage->privatePolygonOffset );
					stageGLState |= GLS_POLYGON_OFFSET;
				}

				GL_Color( color );

				GL_State( stageGLState );
				idRenderVector alphaTestValue( regs[ pStage->alphaTestRegister ] );
				SetFragmentParm( RENDERPARM_ALPHA_TEST, alphaTestValue.ToFloatPtr() );

				renderProgManager.BindShader_TextureVertexColor( drawSurf->jointCache );

				RB_SetVertexColorParms( SVC_IGNORE );

				// bind the texture
				GL_BindTexture( 0, pStage->texture.image );

				// set texture matrix and texGens
				RB_PrepareStageTexturing( pStage, drawSurf );

				// must render with less-equal for Z-Cull to work properly
				assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );

				// draw it
				GL_DrawIndexed( drawSurf );

				// clean up
				RB_FinishStageTexturing( pStage, drawSurf );

				// unset privatePolygonOffset if necessary
				if( pStage->privatePolygonOffset )
				{
					GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * material->GetPolygonOffset() );
				}
			}
		}

		if( !didDraw )
		{
			renderProgManager.BindShader_Depth( drawSurf->jointCache );

			GL_DrawIndexed( drawSurf );
		}

		RENDERLOG_CLOSE_BLOCK();
	}
}
#if 0
/*
=====================
 RenderShadowBufferFast
=====================
*/
static void RenderShadowBufferFast( const drawSurf_t * const drawSurfs, const idRenderMatrix & smVPMatrix )
{
	auto perforatedSurfaces = ( const drawSurf_t** )_alloca( backEnd.viewDef->numPerforatedSurfaces * sizeof( drawSurf_t* ) );
	uint32 numPerforatedSurfaces = 0;
	uint32 numTransSurfaces = 0;

	for( const drawSurf_t* surf = drawSurfs; surf != NULL; surf = surf->nextOnLight )
	{
		if( surf->numIndexes == 0 ) {
			continue;	// a job may have created an empty shadow geometry
		}

		// translucent surfaces don't put anything in the depth buffer
		if( surf->material->Coverage() == MC_TRANSLUCENT )
		{
			++numTransSurfaces;
			continue;
		}

		if( surf->material->Coverage() == MC_PERFORATED )
		{
			release_assert( numPerforatedSurfaces <= backEnd.viewDef->numPerforatedSurfaces );

			// save for later drawing
			perforatedSurfaces[ numPerforatedSurfaces ] = surf;
			++numPerforatedSurfaces;
			continue;
		}

		// set mvp matrix
		if( surf->space != backEnd.currentSpace )
		{
			backEnd.currentSpace = surf->space;

			idRenderMatrix clipMVP;
			idRenderMatrix::Multiply( smVPMatrix, surf->space->modelMatrix, clipMVP );

			RB_SetMVP( clipMVP );
		}

		RENDERLOG_PRINT( surf->material->GetName() );

		if( surf->jointCache )
		{
			renderProgManager.BindShader_DepthSkinned();
		}
		else {
			renderProgManager.BindShader_Depth();
		}

		// must render with less-equal for Z-Cull to work properly
		assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );

		// draw it solid
		GL_DrawIndexed( surf );

		//RENDERLOG_CLOSE_BLOCK();
	}
	RENDERLOG_PRINT( "---( counted perfs %u, precounted %u, trans %u )---\n", numPerforatedSurfaces, backEnd.viewDef->numPerforatedSurfaces, numTransSurfaces );

	// draw all perforated surfaces with the general code path
	if( numPerforatedSurfaces > 0 )
	{
		RenderShadowBufferGeneric( perforatedSurfaces, numPerforatedSurfaces, smVPMatrix );
	}
}
#endif
/*
=====================
RB_ShadowMapPass

	TODO: some world models do not have some faces, not issue for stencils but problem for shadow maps
		
		set separate culling for world models and the rest of the models ?

=====================
*/
void RB_ShadowMapPass( const drawSurf_t * const drawSurfs, const viewLight_t * const vLight )
{
	if( r_skipShadows.GetBool() )
		return;

	if( drawSurfs == NULL )
		return;

	RENDERLOG_OPEN_BLOCK( "RB_ShadowMapPass" );

	if( vLight->parallel )
	{
		GetParallelLightViewProjMatrices( vLight, backEnd.shadowVP );
	}
	else if( vLight->pointLight )
	{
		GetPointLightViewProjMatrices( vLight, backEnd.shadowVP );
	}
	else {
		GetSpotLightViewProjMatrix( vLight, backEnd.shadowVP[ 0 ] );
	}

	GL_ResetTexturesState();

	uint64 glState = 0;

	// the actual stencil func will be set in the draw code, but we need to make sure it isn't
	// disabled here, and that the value will get reset for the interactions without looking
	// like a no-change-required
	GL_State( glState | GLS_POLYGON_OFFSET );

	switch( r_shadowMapOccluderFacing.GetInteger() )
	{
		case 0:
			GL_Cull( CT_FRONT_SIDED );
			GL_PolygonOffset( r_shadowMapPolygonFactor.GetFloat(), r_shadowMapPolygonOffset.GetFloat() );
			break;

		case 1:
			GL_Cull( CT_BACK_SIDED );
			GL_PolygonOffset( -r_shadowMapPolygonFactor.GetFloat(), -r_shadowMapPolygonOffset.GetFloat() );
			break;

		default:
			GL_Cull( CT_TWO_SIDED );
			GL_PolygonOffset( r_shadowMapPolygonFactor.GetFloat(), r_shadowMapPolygonOffset.GetFloat() );
			break;
	}

	if( r_useSinglePassShadowMaps.GetBool() )
	{
		//renderProgManager.BindShader_DepthOnePass();

		GL_SetRenderDestination( renderDestManager.renderDestShadowOnePass );
		GL_ClearDepth( 1.0f );
	}
	else
	{
		renderProgManager.BindShader_Depth( false );

		GL_SetRenderDestination( renderDestManager.renderDestShadow );

		int	side, sideStop;

		if( vLight->parallel )
		{
			side = 0;
			sideStop = r_shadowMapSplits.GetInteger() + 1;
		}
		else if( vLight->pointLight )
		{
			if( r_shadowMapSingleSide.GetInteger() != -1 )
			{
				side = r_shadowMapSingleSide.GetInteger();
				sideStop = side + 1;
			}
			else {
				side = 0;
				sideStop = 6;
			}
		}
		else { // spot light
			side = -1;
			sideStop = 0;
		}

		for(/**/; side < sideStop; side++ )
		{
			///const int32 shadowLOD = backEnd.viewDef->isSubview ? ( Max( vLight->shadowLOD + 1, MAX_SHADOWMAP_RESOLUTIONS-1 ) ) : vLight->shadowLOD ;
			const int32 shadowLOD = backEnd.viewDef->isSubview ? ( MAX_SHADOWMAP_RESOLUTIONS - 1 ) :
				( ( vLight->parallel && side >= 0 ) ? vLight->shadowLOD : Max( vLight->shadowLOD, 1 ) );

			RENDERLOG_PRINT( "----- side = %i, shadowLOD %i----------\n", side, shadowLOD );

			///GL_SetRenderDestination( renderDestManager.renderDestShadow );
			const int32 layer = ( side < 0 )? 0 : side;
			renderDestManager.renderDestShadow->SetTargetOGL( renderImageManager->shadowImage[ shadowLOD ], 0, layer, 0 );

			GL_ViewportAndScissor( 0, 0, shadowMapResolutions[ shadowLOD ], shadowMapResolutions[ shadowLOD ] );
			GL_ClearDepth( 1.0f );

			// process the chain of surfaces with the current rendering state
			//RenderShadowBufferFast( drawSurfs, backEnd.shadowVP[ ( side < 0 )? 0 : side ] );
			RenderShadowBufferGeneric( drawSurfs, backEnd.shadowVP[ ( side < 0 ) ? 0 : side ] );
		}
	}

	//
	// cleanup the shadow specific rendering state
	//

	RB_ResetRenderDest( r_useHDR.GetBool() );

	renderProgManager.Unbind();

	GL_State( GLS_DEFAULT );
	GL_Cull( CT_FRONT_SIDED );

	SetFragmentParm( RENDERPARM_ALPHA_TEST, vec4_zero.ToFloatPtr() );

	RENDERLOG_CLOSE_BLOCK();
}


