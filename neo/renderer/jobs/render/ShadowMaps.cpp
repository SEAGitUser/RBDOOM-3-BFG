
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

	Doom4 tech:	Shadow Map Atlas

	For each light casting a shadow, a unique depth map is generated and saved into one tile
	of a giant 8k x 8k texture atlas. However not every single depth map is calculated at every frame:
	DOOM heavily re-uses the result of the previous frame and regenerates only the depth maps which need to be updated.

	When a light is static and casts shadows only on static objects it makes sense to simply keep its depth map as-is
	instead of doing unnecessary re-calculation. If some enemy is moving under the light though, the depth map must be generated again.
	Depth map sizes can vary depending on the light distance from the camera, also re-generated depth maps don’t necessarily
	stay inside the same tile within the atlas.
	DOOM has specific optimizations like caching the static portion of a depth map, computing then only the
	dynamic meshes projection and compositing the results.


==============================================================================================
*/

struct renderPassShadowBufferFillParms_t
{
	idRenderMatrix				shadowVPMatrices[ 6 ];

	const idDeclRenderProg *	shadowBuffer_point;
	const idDeclRenderProg *	shadowBuffer_perforated_point;
	const idDeclRenderProg *	shadowBuffer_spot;
	const idDeclRenderProg *	shadowBuffer_perforated_spot;
	const idDeclRenderProg *	shadowBuffer_parallel;
	const idDeclRenderProg *	shadowBuffer_perforated_parallel;

};

void Init( renderPassShadowBufferFillParms_t * pass )
{
	pass->shadowBuffer_point				= renderProgManager.FindRenderProgram( "shadowBuffer_point" );
	pass->shadowBuffer_perforated_point		= renderProgManager.FindRenderProgram( "shadowBuffer_perforated_point" );
	pass->shadowBuffer_spot					= renderProgManager.FindRenderProgram( "shadowBuffer_spot" );
	pass->shadowBuffer_perforated_spot		= renderProgManager.FindRenderProgram( "shadowBuffer_perforated_spot" );
	pass->shadowBuffer_parallel				= renderProgManager.FindRenderProgram( "shadowBuffer_parallel" );
	pass->shadowBuffer_perforated_parallel	= renderProgManager.FindRenderProgram( "shadowBuffer_perforated_parallel" );



}

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

void RB_SetupShadowCommonParms( const lightType_e lightType, const uint32 shadowLOD )
{
	static const int JITTER_SIZE = 128;

	// default high quality
	float jitterSampleScale = 1.0f;
	float shadowMapSamples = r_shadowMapSamples.GetInteger();

	// screen power of two correction factor / rpScreenCorrectionFactor;
	auto & screenCorrectionParm = renderProgManager.GetRenderParm( RENDERPARM_SCREENCORRECTIONFACTOR )->GetVec4();
	screenCorrectionParm[ 0 ] = 1.0f / ( JITTER_SIZE * shadowMapSamples );
	screenCorrectionParm[ 1 ] = 1.0f / JITTER_SIZE;
	screenCorrectionParm[ 2 ] = 1.0f / shadowMapResolutions[ shadowLOD ];
	screenCorrectionParm[ 3 ] = ( lightType == LIGHT_TYPE_PARALLEL ) ? r_shadowMapSunDepthBiasScale.GetFloat() : r_shadowMapRegularDepthBiasScale.GetFloat();

	// rpJitterTexScale
	auto & jitterTexScale = renderProgManager.GetRenderParm( RENDERPARM_JITTERTEXSCALE )->GetVec4();
	jitterTexScale[ 0 ] = r_shadowMapJitterScale.GetFloat() * jitterSampleScale;	// TODO shadow buffer size fraction shadowMapSize / maxShadowMapSize
	jitterTexScale[ 1 ] = r_shadowMapJitterScale.GetFloat() * jitterSampleScale;
	jitterTexScale[ 2 ] = -r_shadowMapBiasScale.GetFloat();
	jitterTexScale[ 3 ] = 0.0f;

	// rpJitterTexOffset
	auto & jitterTexOffset = renderProgManager.GetRenderParm( RENDERPARM_JITTERTEXOFFSET )->GetVec4();
	if( r_shadowMapRandomizeJitter.GetBool() )
	{
		jitterTexOffset[ 0 ] = ( rand() & 255 ) / 255.0;
		jitterTexOffset[ 1 ] = ( rand() & 255 ) / 255.0;
	}
	else {
		jitterTexOffset[ 0 ] = 0;
		jitterTexOffset[ 1 ] = 0;
	}
	jitterTexOffset[ 2 ] = 0.0f;
	jitterTexOffset[ 3 ] = 0.0f;

	/*if( vLight->parallel ) { // rpCascadeDistances
	auto & cascadeDistances = renderProgManager.GetRenderParm( RENDERPARM_CASCADEDISTANCES )->GetVec4();
	cascadeDistances[0] = backEnd.viewDef->GetSplitFrustumDistances()[0];
	cascadeDistances[1] = backEnd.viewDef->GetSplitFrustumDistances()[1];
	cascadeDistances[2] = backEnd.viewDef->GetSplitFrustumDistances()[2];
	cascadeDistances[3] = backEnd.viewDef->GetSplitFrustumDistances()[3];
	}*/

	//auto & parm = renderProgManager.GetRenderParm( RENDERPARM_USERVEC0 )->GetVec4();
	//parm[ 0 ] = parm[ 1 ] = parm[ 2 ] = parm[ 3 ] = 1.0f;
	//if( r_useSinglePassShadowMaps.GetBool() )
	//{
	//	parm[ 0 ] = ( float )shadowMapResolutions[ shadowLOD ] / shadowMapResolutions[ 0 ];
	//}

	// texture 5 will be the shadow maps array
	renderProgManager.SetRenderParm( RENDERPARM_SHADOWBUFFERMAP, renderDestManager.shadowBufferImage[ shadowLOD ] );

	// texture 6 will be the jitter texture for soft shadowing
	if( r_shadowMapSamples.GetInteger() == 16 )
	{
		renderProgManager.SetRenderParm( RENDERPARM_JITTERMAP, renderImageManager->jitterImage16 );
	}
	else if( r_shadowMapSamples.GetInteger() == 4 )
	{
		renderProgManager.SetRenderParm( RENDERPARM_JITTERMAP, renderImageManager->jitterImage4 );
	}
	else {
		renderProgManager.SetRenderParm( RENDERPARM_JITTERMAP, renderImageManager->jitterImage1 );
	}
}

/*
=====================
 RB_SetupShadowDrawMatrices
=====================
*/
void RB_SetupShadowDrawMatrices( const lightType_e lightType, const drawSurf_t * const surf )
{
	if( lightType == LIGHT_TYPE_PARALLEL )
	{
		idRenderMatrix shadowClipMVPs[ 5 ];
		const uint32 count = ( r_shadowMapSplits.GetInteger() + 1 );
		for( uint32 i = 0; i < count; i++ )
		{
			///idRenderMatrix shadowClipMVP;
			idRenderMatrix::Multiply( backEnd.shadowVP[ i ], surf->space->modelMatrix, shadowClipMVPs[ i ] );
			///renderProgManager.SetRenderParms( ( renderParm_t )( RENDERPARM_SHADOW_MATRIX_0_X + i * 4 ), shadowClipMVP.Ptr(), 4 );
		}

		backEnd.shadowUniformBuffer.Update( sizeof( idRenderMatrix ) * count, shadowClipMVPs );
	}
	else if( lightType == LIGHT_TYPE_POINT )
	{
		idRenderMatrix shadowClipMVPs[ 6 ];
		for( uint32 i = 0; i < 6; i++ )
		{
			///idRenderMatrix shadowClipMVP;
			idRenderMatrix::Multiply( backEnd.shadowVP[ i ], surf->space->modelMatrix, shadowClipMVPs[ i ] );
			///renderProgManager.SetRenderParms( ( renderParm_t )( RENDERPARM_SHADOW_MATRIX_0_X + i * 4 ), shadowClipMVP.Ptr(), 4 );
		}

		backEnd.shadowUniformBuffer.Update( sizeof( idRenderMatrix ) * 6, shadowClipMVPs );
	}
	else if( lightType == LIGHT_TYPE_SPOT )
	{
		idRenderMatrix shadowClipMVP;
		idRenderMatrix::Multiply( backEnd.shadowVP[ 0 ], surf->space->modelMatrix, shadowClipMVP );

		///renderProgManager.SetRenderParms( ( renderParm_t )( RENDERPARM_SHADOW_MATRIX_0_X ), shadowClipMVP.Ptr(), 4 );
		backEnd.shadowUniformBuffer.Update( sizeof( idRenderMatrix ), shadowClipMVP.Ptr() );
	}

	//backEnd.shadowUniformBuffer.Bind( BINDING_SHADOW_UBO, backEnd.shadowUniformBuffer.GetOffset(), backEnd.shadowUniformBuffer.GetSize() );
}
/*
=====================
RB_SetupShadowDrawMatricesGlobal
=====================
*/
void RB_SetupShadowDrawMatricesGlobal( const lightType_e lightType )
{
	if( lightType == LIGHT_TYPE_PARALLEL )
	{
		const uint32 count = ( r_shadowMapSplits.GetInteger() + 1 );
		backEnd.shadowUniformBuffer.Update( sizeof( idRenderMatrix ) * count, backEnd.shadowVP );
	}
	else if( lightType == LIGHT_TYPE_POINT )
	{
		backEnd.shadowUniformBuffer.Update( sizeof( idRenderMatrix ) * 6, backEnd.shadowVP );
	}
	else if( lightType == LIGHT_TYPE_SPOT )
	{
		backEnd.shadowUniformBuffer.Update( sizeof( idRenderMatrix ), &backEnd.shadowVP[ 0 ] );
	}

	//backEnd.shadowUniformBuffer.Bind( BINDING_SHADOW_UBO, backEnd.shadowUniformBuffer.GetOffset(), backEnd.shadowUniformBuffer.GetSize() );
}

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
		cropBounds.Intersect2DSelf( lightBounds );

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
 DrawSurfaceGeneric
=====================
*/
static void DrawSurfaceGeneric( const drawSurf_t * const drawSurf, const idDeclRenderProg * progSolid, const idDeclRenderProg * progPerforated )
{
	auto const material = drawSurf->material;
	assert( material != nullptr );

	auto const regs = drawSurf->shaderRegisters;
	//release_assert( regs != nullptr );  empty for statics ?

	RENDERLOG_OPEN_BLOCK( material->GetName() );

	if( drawSurf->space != backEnd.currentSpace )
	{
		backEnd.currentSpace = drawSurf->space;
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

			renderProgManager.SetMVPMatrixParms( clipMVP );
		}
		else if( side < 0 ) // spot
		{
			idRenderMatrix clipMVP;
			idRenderMatrix::Multiply( backEnd.shadowVP[ 0 ], drawSurf->space->modelMatrix, clipMVP );

			/// from OpenGL view space to OpenGL NDC ( -1 : 1 in XYZ )
			///idRenderMatrix MVP;
			///idRenderMatrix::Multiply( renderMatrix_windowSpaceToClipSpace, clipMVP, MVP );
			///renderProgManager.SetMVPMatrixParms( MVP );

			renderProgManager.SetMVPMatrixParms( clipMVP );
		}
		else
		{
			idRenderMatrix clipMVP;
			idRenderMatrix::Multiply( backEnd.shadowVP[ side ], drawSurf->space->modelMatrix, clipMVP );

			renderProgManager.SetMVPMatrixParms( clipMVP );
		}
	#endif
		//idRenderMatrix clipMVP;
		///const idRenderMatrix & smVPMatrix = backEnd.shadowVP[ ( side < 0 ) ? 0 : side ];
		//idRenderMatrix::Multiply( smVPMatrix, drawSurf->space->modelMatrix, clipMVP );
		//renderProgManager.SetMVPMatrixParms( clipMVP );

		renderProgManager.SetModelMatrixParms( drawSurf->space->modelMatrix );
	}
	if( drawSurf->space->modelMatrix.IsIdentity( MATRIX_EPSILON ) )
	{
		RENDERLOG_PRINT( "-- modelMatrix.IsIdentity() --\n" );
	}

	uint64 surfGLState = GLS_COLORMASK | GLS_ALPHAMASK | GLS_STENCILMASK | GLS_DEPTHFUNC_LEQUAL;
	// facing ?? front facing for now.

	// set polygon offset if necessary
	if( material->TestMaterialFlag( MF_POLYGONOFFSET ) )
	{
		surfGLState |= GLS_POLYGON_OFFSET;
		GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * material->GetPolygonOffset() );
	}

	bool didDraw = false;
	if( material->Coverage() == MC_PERFORATED )
	{
		// perforated surfaces may have multiple alpha tested stages
		for( int stage = 0; stage < material->GetNumStages(); stage++ )
		{
			auto pStage = material->GetStage( stage );

			if( !pStage->hasAlphaTest )
				continue;

			// check the stage enable condition
			assert( regs != nullptr );
			if( pStage->SkipStage( regs ) )
				continue;

			// if we at least tried to draw an alpha tested stage,
			// we won't draw the opaque surface
			didDraw = true;

			// skip the entire stage if alpha would be black
			assert( regs != nullptr );
			if( pStage->GetColorParmAlpha( regs ) <= 0.0f )
				continue;

			///renderProgManager.BindProg_FillShadowBufferOnePass( lightType, true );
			GL_SetRenderProgram( progPerforated );

			// set rpEnableSkinning if the shader has optional support for skinning
			/*if( drawSurf->jointCache && renderProgManager.ShaderHasOptionalSkinning() )
			{
				GL_EnableSkinning();
			}*/

			uint64 stageGLState = surfGLState;

			// set privatePolygonOffset if necessary
			if( pStage->privatePolygonOffset )
			{
				GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * pStage->privatePolygonOffset );
				stageGLState |= GLS_POLYGON_OFFSET;
			}

			GL_State( stageGLState );

			assert( regs != nullptr );
			renderProgManager.EnableAlphaTestParm( pStage->GetAlphaTestValue( regs ) );

			// bind the texture
			renderProgManager.SetRenderParm( RENDERPARM_MAP, pStage->texture.image );

			// set the texture matrix if needed
			renderProgManager.SetupTextureMatrixParms( drawSurf->shaderRegisters, &pStage->texture );

			// must render with less-equal for Z-Cull to work properly
			assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LEQUAL );

			// draw it
			GL_DrawIndexed( drawSurf );

			// unset privatePolygonOffset if necessary
			if( pStage->privatePolygonOffset )
			{
				GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * material->GetPolygonOffset() );
			}

			// clear rpEnableSkinning if it was set
			/*if( drawSurf->jointCache && renderProgManager.ShaderHasOptionalSkinning() )
			{
				GL_DisableSkinning();
			}*/
		}
	}

	if( !didDraw )
	{
		///renderProgManager.BindProg_FillShadowBufferOnePass( lightType, false );
		GL_SetRenderProgram( progSolid );

		// set rpEnableSkinning if the shader has optional support for skinning
		/*if( drawSurf->jointCache && renderProgManager.ShaderHasOptionalSkinning() )
		{
			GL_EnableSkinning();
		}*/

		GL_DrawIndexed( drawSurf );

		// clear rpEnableSkinning if it was set
		/*if( drawSurf->jointCache && renderProgManager.ShaderHasOptionalSkinning() )
		{
			GL_DisableSkinning();
		}*/
	}

	RENDERLOG_CLOSE_BLOCK();
}

/*
=====================
 RenderShadowBufferFast
=====================
*/
static void RenderShadowBufferFast( const drawSurf_t * const drawSurfs,
	const idDeclRenderProg * progStaticGeometry, const idDeclRenderProg * progSolid, const idDeclRenderProg * progPerforated )
{
	auto deferredList = ( const drawSurf_t** )_alloca( 256 * sizeof( drawSurf_t* ) );
	uint32 numDeferredSurfaces = 0;

	GL_StartDrawBatch();

	GL_SetRenderProgram( progStaticGeometry );

	for( const drawSurf_t* surf = drawSurfs; surf != NULL; surf = surf->nextOnLight )
	{
		if( surf->numIndexes == 0 ) {
			continue;	// a job may have created an empty shadow geometry
		}

		if( surf->material->Coverage() == MC_PERFORATED ||
			surf->jointCache ||
			!surf->space->modelMatrix.IsIdentity( MATRIX_EPSILON ) )
		{
			assert( numDeferredSurfaces <= 256 );

			// save for later drawing
			deferredList[ numDeferredSurfaces++ ] = surf;
			continue;
		}

		RENDERLOG_OPEN_BLOCK( surf->material->GetName() );

		/*if( surf->space->modelMatrix.IsIdentity( MATRIX_EPSILON ) ) {
			RENDERLOG_PRINT( "-- modelMatrix.IsIdentity() --\n" );
		}*/

		// must render with less-equal for Z-Cull to work properly
		assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LEQUAL );

		// draw it solid
		//GL_DrawIndexed( surf, LAYOUT_DRAW_VERT_POSITION_ONLY );
		GL_SetupDrawBatchParms( surf );

		RENDERLOG_CLOSE_BLOCK();
	}

	GL_FinishDrawBatch();
	///RENDERLOG_PRINT( "---( counted perfs %u, precounted %u, trans %u )---\n", numPerforatedSurfaces, backEnd.viewDef->numPerforatedSurfaces, numTransSurfaces );

	// draw all perforated surfaces with the general code path
	if( numDeferredSurfaces > 0 )
	{
		backEnd.ClearCurrentSpace();

		for( uint32 i = 0; i < numDeferredSurfaces; ++i )
		{
			DrawSurfaceGeneric( deferredList[ i ], progSolid, progPerforated );
		}
	}
}

/*
=====================
 RenderShadowBufferOnePass
=====================
*/
static void RenderShadowBuffer( const drawSurf_t * const drawSurfs, const lightType_e lightType,
	const idDeclRenderProg * progSBSolid, const idDeclRenderProg * progSBPerforated )
{
	backEnd.ClearCurrentSpace();

	for( const drawSurf_t* drawSurf = drawSurfs; drawSurf != NULL; drawSurf = drawSurf->nextOnLight )
	{
		if( drawSurf->numIndexes == 0 )
			continue;	// a job may have created an empty shadow geometry

		DrawSurfaceGeneric( drawSurf, progSBSolid, progSBPerforated );
	}
}

ID_INLINE void ClearShadowBuffer( const bool bUseProgram )
{
	if( bUseProgram )
	{
		GL_SetRenderProgram( renderProgManager.prog_shadowBuffer_clear );
		GL_DrawUnitSquare( LAYOUT_DRAW_VERT_POSITION_ONLY, 6 );
	}
	else {
		///GL_ClearDepth( 1.0f );
		GL_Clear( false, true, false, 0, 0, 0, 0, 0 );
	}
}

ID_INLINE auto GetProg_FillShadowBufferOnePass( lightType_e lightType, const bool bPerforated, const bool bStatic )
{
	switch( lightType )
	{
		case LIGHT_TYPE_POINT:
			return bStatic ? renderProgManager.prog_shadowBuffer_static_point : ( bPerforated ? renderProgManager.prog_shadowBuffer_perforated_point : renderProgManager.prog_shadowBuffer_point );

		case LIGHT_TYPE_SPOT:
			return bStatic ? renderProgManager.prog_shadowbuffer_static_spot : ( bPerforated ? renderProgManager.prog_shadowBuffer_perforated_spot : renderProgManager.prog_shadowBuffer_spot );

		case LIGHT_TYPE_PARALLEL:
			return bStatic ? renderProgManager.prog_shadowBuffer_static_parallel : ( bPerforated ? renderProgManager.prog_shadowBuffer_perforated_parallel : renderProgManager.prog_shadowBuffer_parallel );

			//case LIGHT_TYPE_AREA:
			//return perforated ? prog_shadowBuffer_perforated_area : prog_shadowBuffer_area;
	}
	assert( "GetProg_FillShadowBufferOnePass returns NULL" );
	return ( const idDeclRenderProg * )NULL;
}

/*
=====================
RB_FillShadowBuffer

	TODO: some world models do not have some faces, not issue for stencils but problem for shadow maps

		set separate culling for world models and the rest of the models ?

=====================
*/
void RB_FillShadowBuffer( const drawSurf_t * const drawSurfs, const viewLight_t * const vLight )
{
	if( r_skipShadows.GetBool() )
		return;

	if( drawSurfs == NULL )
		return;

	RENDERLOG_OPEN_BLOCK( "RB_FillShadowBuffer" );

	const idDeclRenderProg * progSolid;
	const idDeclRenderProg * progPerforated;
	const idDeclRenderProg * progStaticGeometry;

	/*/*/if( vLight->IsPointType() )
	{
		GetPointLightViewProjMatrices( vLight, backEnd.shadowVP );

		progSolid			= GetProg_FillShadowBufferOnePass( LIGHT_TYPE_POINT, false, false );
		progPerforated		= GetProg_FillShadowBufferOnePass( LIGHT_TYPE_POINT, true, false );
		progStaticGeometry	= GetProg_FillShadowBufferOnePass( LIGHT_TYPE_POINT, false, true );

		// Update shadow view projection matrices
		backEnd.shadowUniformBuffer.Update( sizeof( idRenderMatrix ) * 6, backEnd.shadowVP );
	}
	else if( vLight->IsParallelType() )
	{
		GetParallelLightViewProjMatrices( vLight, backEnd.shadowVP );

		progSolid			= GetProg_FillShadowBufferOnePass( LIGHT_TYPE_PARALLEL, false, false );
		progPerforated		= GetProg_FillShadowBufferOnePass( LIGHT_TYPE_PARALLEL, true, false );
		progStaticGeometry	= GetProg_FillShadowBufferOnePass( LIGHT_TYPE_PARALLEL, false, true );

		// Update shadow view projection matrices
		backEnd.shadowUniformBuffer.Update( sizeof( idRenderMatrix ) * ( r_shadowMapSplits.GetInteger() + 1 ), backEnd.shadowVP );
	}
	else if( vLight->IsSpotType() )
	{
		GetSpotLightViewProjMatrix( vLight, backEnd.shadowVP[0] );

		progSolid			= GetProg_FillShadowBufferOnePass( LIGHT_TYPE_SPOT, false, false );
		progPerforated		= GetProg_FillShadowBufferOnePass( LIGHT_TYPE_SPOT, true, false );
		progStaticGeometry	= GetProg_FillShadowBufferOnePass( LIGHT_TYPE_SPOT, false, true );

		// Update shadow view projection matrices
		backEnd.shadowUniformBuffer.Update( sizeof( idRenderMatrix ), backEnd.shadowVP );
	}

	GL_ResetTextureState();

	uint64 glState = GLS_COLORMASK | GLS_ALPHAMASK | GLS_STENCILMASK | GLS_DEPTHFUNC_LESS;
	float polyScale, polyBias;

	glState |= GLS_POLYGON_OFFSET;
	switch( r_shadowMapOccluderFacing.GetInteger() )
	{
		case 0:
			glState |= GLS_FRONTSIDED;
			polyScale = r_shadowMapPolygonFactor.GetFloat(),
			polyBias = r_shadowMapPolygonOffset.GetFloat();
			break;

		case 1:
			glState |= GLS_BACKSIDED;
			polyScale = -r_shadowMapPolygonFactor.GetFloat(),
			polyBias = -r_shadowMapPolygonOffset.GetFloat();
			break;

		case 2:
		default:
			glState |= GLS_TWOSIDED;
			polyScale = r_shadowMapPolygonFactor.GetFloat(),
			polyBias = r_shadowMapPolygonOffset.GetFloat();
			break;
	}

	{
		const uint32 shadowLOD = vLight->GetShadowLOD();

		GL_SetRenderDestination( renderDestManager.renderDestShadowOnePass[ shadowLOD ] );
		GL_ViewportAndScissor( 0, 0, shadowMapResolutions[ shadowLOD ], shadowMapResolutions[ shadowLOD ] );

		GL_State( glState );
		GL_PolygonOffset( polyScale, polyBias );

		GL_ClearDepth( 1.0f );

		if( r_useLightDepthBounds.GetBool() )
		{
			GL_DepthBoundsTest( 0.0f, 0.0f );
		}

		RENDERLOG_PRINT( "--- %s shadowLOD %ux%i ----------\n", renderDestManager.renderDestShadowOnePass[ shadowLOD ]->GetName(), shadowLOD, shadowMapResolutions[ shadowLOD ] );

		//RenderShadowBuffer( drawSurfs, vLight->GetLightType(), progSolid, progPerforated );
		RenderShadowBufferFast( drawSurfs, progStaticGeometry, progSolid, progPerforated );
	}

	//
	// cleanup the shadow specific rendering state
	//
	renderProgManager.DisableAlphaTestParm();

	GL_ResetProgramState();
	GL_ResetTextureState();

	RENDERLOG_CLOSE_BLOCK();
}


static void RB_RenderScreenSpaceShadowMap( const idRenderView * const renderView )
{
	if( r_skipShadows.GetBool() )
		return;

	RENDERLOG_OPEN_MAINBLOCK( MRB_DRAW_INTERACTIONS );
	RENDERLOG_OPEN_BLOCK( "RB_DrawInteractionsWithScreenSpaceShadows" );

	GL_ResetTextureState();

	const bool useLightDepthBounds = r_useLightDepthBounds.GetBool() && !r_useShadowMapping.GetBool();
	const int shadowMaskWidth = 0;
	const int shadowMaskHeight = 0;

	//
	// For each light, perform shadowing to screen sized ( or less ) texture, blur it, upscale if needed.
	//
	for( const viewLight_t* vLight = renderView->viewLights; vLight != NULL; vLight = vLight->next )
	{
		// do fogging later
		if( vLight->lightShader->IsFogLight() || vLight->lightShader->IsBlendLight() )
			continue;

		if( vLight->NoInteractions() )
			continue;

		RENDERLOG_OPEN_BLOCK( vLight->lightShader->GetName() );

		const uint32 shadowLOD = vLight->GetShadowLOD();

		GL_SetRenderDestination( renderDestManager.renderDestShadowOnePass[ shadowLOD ] );
		GL_ViewportAndScissor( 0, 0, shadowMapResolutions[ shadowLOD ], shadowMapResolutions[ shadowLOD ] );
		GL_ClearDepth( 1.0f );

		// set the depth bounds for the whole light
		if( useLightDepthBounds )
		{
			GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
		}

		RENDERLOG_PRINT( "--- %s shadowLOD %ix%i ----------\n", renderDestManager.renderDestShadowOnePass[ shadowLOD ]->GetName(), shadowLOD, shadowMapResolutions[ shadowLOD ] );

		// RB_FillShadowDepthBuffer( vLight->globalShadows, vLight );

		// render the shadows into destination alpha on the included pixels
		// RB_FillScreenSpaceShadowMap( vLight );

		RENDERLOG_CLOSE_BLOCK();

		RENDERLOG_OPEN_BLOCK( "DrawShadowMask" );

		//GL_SetRenderDestination( renderDestManager.renderDestShadowMask );
		GL_Viewport( 0, 0, shadowMaskWidth, shadowMaskHeight );
		RB_SetScissor( vLight->scissorRect );


		RENDERLOG_CLOSE_BLOCK();
	}

	// disable stencil shadow test
	GL_State( GLS_DEFAULT );

	// unbind texture units
	GL_ResetTextureState();

	// reset depth bounds
	if( useLightDepthBounds )
	{
		GL_DepthBoundsTest( 0.0f, 0.0f );
	}

	RENDERLOG_CLOSE_BLOCK();
	RENDERLOG_CLOSE_MAINBLOCK();
}