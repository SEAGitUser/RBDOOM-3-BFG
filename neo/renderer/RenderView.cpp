
#pragma hdrstop
#include "precompiled.h"

#include "tr_local.h"

/*
=====================================================================================================
	
	idRenderView	

	A view may be either the actual camera view,
	a mirror / remote location, or a 3D view on a gui surface.

=====================================================================================================
*/

idCVar r_centerX( "r_centerX", "0", CVAR_FLOAT, "projection matrix center adjust" );
idCVar r_centerY( "r_centerY", "0", CVAR_FLOAT, "projection matrix center adjust" );
idCVar r_znear( "r_znear", "3", CVAR_RENDERER | CVAR_FLOAT, "near Z clip plane distance", 0.001f, 200.0f );
idCVar r_zfar( "r_zfar", "131072", CVAR_RENDERER | CVAR_FLOAT, "far Z clip plane distance", 0.001f, 999999.0f );
idCVar r_useInfiniteFarZ( "r_useInfiniteFarZ", "1", CVAR_RENDERER | CVAR_BOOL, "use the no-far-clip-plane trick, ignoring r_zfar value" );
idCVar r_jitter( "r_jitter", "0", CVAR_RENDERER | CVAR_BOOL, "randomly subpixel jitter the projection matrix", 0, 1 );

/*
======================
======================
*/
float idRenderView::GetZNear() const
{
	return ( parms.cramZNear )? ( r_znear.GetFloat() * 0.25f ) : r_znear.GetFloat();
}

/*
=================
idRenderView::DeriveData
=================
*/
void idRenderView::DeriveData()
{
	if( this->is2Dgui )
	{
		this->worldSpace.modelMatrix.Identity();

		this->worldSpace.modelViewMatrix.Identity();

		idRenderMatrix::CreateOrthogonalProjection(
			renderSystem->GetVirtualWidth(), renderSystem->GetVirtualHeight(), 0.0f, 1.0f, this->projectionMatrix, true );

		// no other data is needed in 2d rendering ?
		return;
	}

	// setup the matrix for world space to eye space
	{		
		memset( &this->worldSpace, 0, sizeof( this->worldSpace ) );

		// the model matrix is an identity
		this->worldSpace.modelMatrix.Identity();

		// transform by the camera placement,
		// convert from our coordinate system (looking down X)
		// to OpenGL's coordinate system (looking down -Z)
		idRenderMatrix::CreateViewMatrix( this->GetOrigin(), this->GetAxis(), this->worldSpace.modelViewMatrix );
	}

	// we need to set the projection matrix before doing portal-to-screen scissor calculations
	{
		// random jittering is usefull when multiple frames are going to be blended together
		// for motion blurred anti-aliasing
		float jitterx = 0.0f; float jittery = 0.0f;
		if( r_jitter.GetBool() && !this->isSubview )
		{
			static idRandom random;
			jitterx = random.RandomFloat();
			jittery = random.RandomFloat();
		}

		//
		// set up projection matrix
		//
		const float zNear = GetZNear();
		const float zFar = r_useInfiniteFarZ.GetBool() ? 0.0 : r_zfar.GetFloat();

		float xmax = zNear * idMath::Tan( DEG2RAD( this->parms.fov_x ) * 0.5f );
		float ymax = zNear * idMath::Tan( DEG2RAD( this->parms.fov_y ) * 0.5f );

		float ymin = -ymax;
		float xmin = -xmax;

		const float width = xmax - xmin;
		const float height = ymax - ymin;

		const int viewWidth = this->viewport.x2 - this->viewport.x1 + 1;
		const int viewHeight = this->viewport.y2 - this->viewport.y1 + 1;

		jitterx = jitterx * width / viewWidth;
		jitterx += r_centerX.GetFloat();
		jitterx += this->parms.stereoScreenSeparation;

		jittery = jittery * height / viewHeight;
		jittery += r_centerY.GetFloat();

		xmin += jitterx * width;  ymin += jittery * height;
		xmax += jitterx * width;  ymax += jittery * height;

		idRenderMatrix::CreateProjectionMatrix( xmin, xmax, ymin, ymax, zNear, zFar, this->projectionMatrix );
	}

	// Lots of usefull stuff here for postproc, deferred passes, faster culling etc.

	idRenderMatrix::Multiply( this->GetProjectionMatrix(), this->GetViewMatrix(), this->worldSpace.mvp );

	if( !idRenderMatrix::Inverse( this->GetViewMatrix(), this->inverseViewMatrix ) )
	{
		idLib::Warning( "SetupUtilityMatrices() ViewMatrix invert failed" );
	}
	if( !idRenderMatrix::Inverse( this->GetProjectionMatrix(), this->inverseProjectionMatrix ) )
	{
		idLib::Warning( "SetupUtilityMatrices() ProjectionMatrix invert failed" );
	}
	if( !idRenderMatrix::Inverse( this->GetMVPMatrix(), this->inverseViewProjectionMatrix ) )
	{
		idLib::Warning( "SetupUtilityMatrices() MVPMatrix invert failed" );
	}

	// the planes of the view frustum are needed for portal visibility culling
	{
		idRenderMatrix::GetFrustumPlanes( &this->frustums[ FRUSTUM_PRIMARY ][ 0 ], this->GetMVPMatrix(), false, true );

		// the DOOM 3 frustum planes point outside the frustum
		for( int i = 0; i < 6; i++ )
		{
			this->frustums[ FRUSTUM_PRIMARY ][ i ] = -this->frustums[ FRUSTUM_PRIMARY ][ i ];
		}
		// remove the Z-near to avoid portals from being near clipped
		this->frustums[ FRUSTUM_PRIMARY ][ 4 ][ 3 ] -= r_znear.GetFloat();
	}

	// RB Shadow maps split frustums 
	if( r_useShadowMapping.GetBool() )
	{
		const float zNearStart = this->GetZNear();
		const float zFarEnd = 10000;

		const float lambda = r_shadowMapSplitWeight.GetFloat();
		const float ratio = zFarEnd / zNearStart;

		float zNear = zNearStart;
		float zFar = zFarEnd;

		for( int i = 0; i < 6; i++ )
		{
			this->frustumSplitDistances[ i ] = idMath::INFINITY;
		}

		for( int i = 1; i <= ( r_shadowMapSplits.GetInteger() + 1 ) && i < MAX_FRUSTUMS; i++ )
		{
			float si = i / ( float )( r_shadowMapSplits.GetInteger() + 1 );

			if( i > FRUSTUM_CASCADE1 )
			{
				zNear = zFar - ( zFar * 0.005f );
			}

			zFar = 1.005f * lambda * ( zNearStart * idMath::Pow( ratio, si ) ) + ( 1 - lambda ) * ( zNearStart + ( zFarEnd - zNearStart ) * si );

			if( i <= r_shadowMapSplits.GetInteger() )
			{
				this->frustumSplitDistances[ i - 1 ] = zFar;
			}

			idRenderMatrix projectionRenderMatrix;
			idRenderMatrix::CreateProjectionMatrixFov( this->GetFOVx(), this->GetFOVy(), zNear, zFar, 0.0f, 0.0f, projectionRenderMatrix );

			idRenderMatrix frustumMVP;
			idRenderMatrix::Multiply( projectionRenderMatrix, this->GetViewMatrix(), frustumMVP );

			idRenderMatrix::GetFrustumPlanes( &this->frustums[ i ][ 0 ], frustumMVP, false, true );

			// the DOOM 3 frustum planes point outside the frustum
			for( int j = 0; j < 6; j++ )
			{
				this->frustums[ i ][ j ] = -this->frustums[ i ][ j ];
			}
			// remove the Z-near to avoid portals from being near clipped
			if( i == FRUSTUM_CASCADE1 )
			{
				this->frustums[ i ][ 4 ][ 3 ] -= r_znear.GetFloat();
			}

			if( !idRenderMatrix::Inverse( frustumMVP, this->frustumInvMVPs[ i ] ) )
			{
				idLib::Warning( "SetupFrustums() frustumMVPs invert failed" );
			}
		}
	}
}

/*
==========================
 R_GlobalToNormalizedDeviceCoordinates

	-1 to 1 range in x, y, and z
==========================
*/
void idRenderView::GlobalToNormalizedDeviceCoordinates( const idVec3& global, idVec3& ndc ) const
{
	idVec4 view, clip;
	idRenderMatrix::TransformModelToClip( global, GetViewMatrix(), GetProjectionMatrix(), view, clip );
	idRenderMatrix::TransformClipToDevice( clip, ndc );
}

/*
===================
 ScreenRectForWinding
===================
*/
void idRenderView::ScreenRectFromWinding( const idWinding & w, const idRenderMatrix & modelMatrix, idScreenRect & screenRect ) const
{
	const float viewWidth  = ( float )this->GetViewport().x2 - ( float )this->GetViewport().x1;
	const float viewHeight = ( float )this->GetViewport().y2 - ( float )this->GetViewport().y1;

	screenRect.Clear();
	for( int i = 0; i < w.GetNumPoints(); ++i )
	{
		idVec3 v, ndc;

		modelMatrix.TransformPoint( w[ i ].ToVec3(), v );

		GlobalToNormalizedDeviceCoordinates( v, ndc );

		screenRect.AddPoint( 
			( ndc[ 0 ] * 0.5f + 0.5f ) * viewWidth,		// windowX
			( ndc[ 1 ] * 0.5f + 0.5f ) * viewHeight		// windowY
		);
	}

	screenRect.Expand();
}

// transform Z in eye coordinates to window coordinates
static void R_TransformEyeZToWin( float src_z, const idRenderMatrix & projectionMatrix, float &dst_z )
{
	// projection
	//float clip_z = src_z * projectionMatrix[ 2 + 2 * 4 ] + projectionMatrix[ 2 + 3 * 4 ];
	//float clip_w = src_z * projectionMatrix[ 3 + 2 * 4 ] + projectionMatrix[ 3 + 3 * 4 ];

	//float clip_z = src_z * projectionMatrix[ 10 ] + projectionMatrix[ 14 ];
	//float clip_w = src_z * projectionMatrix[ 11 ] + projectionMatrix[ 15 ];

	float clip_z = src_z * projectionMatrix[ 2 ][ 2 ] + projectionMatrix[ 2 ][ 3 ];
	float clip_w = src_z * projectionMatrix[ 3 ][ 2 ] + projectionMatrix[ 3 ][ 3 ];

	if( clip_w <= 0.0f )
	{
		dst_z = 0.0f;	// clamp to near plane
	}
	else {
		dst_z = clip_z / clip_w;
		dst_z = dst_z * 0.5f + 0.5f;	// convert to window coords
	}
}

/*
======================
 R_ScreenRectFromViewFrustumBounds
======================
*/
void idRenderView::ScreenRectFromViewFrustumBounds( const idBounds &bounds, idScreenRect & screenRect ) const
{
	float screenWidth  = ( float )this->GetViewport().x2 - ( float )this->GetViewport().x1;
	float screenHeight = ( float )this->GetViewport().y2 - ( float )this->GetViewport().y1;

	screenRect.x1 = idMath::Ftoi16( 0.5f * ( 1.0f - bounds[ 1 ].y ) * screenWidth  );
	screenRect.x2 = idMath::Ftoi16( 0.5f * ( 1.0f - bounds[ 0 ].y ) * screenWidth  );
	screenRect.y1 = idMath::Ftoi16( 0.5f * ( 1.0f + bounds[ 0 ].z ) * screenHeight );
	screenRect.y2 = idMath::Ftoi16( 0.5f * ( 1.0f + bounds[ 1 ].z ) * screenHeight );

	//if( r_useDepthBoundsTest.GetInteger() )
	//{
		R_TransformEyeZToWin( -bounds[ 0 ].x, this->projectionMatrix, screenRect.zmin );
		R_TransformEyeZToWin( -bounds[ 1 ].x, this->projectionMatrix, screenRect.zmax );
	//}
}

#include "Model_local.h"

/*
=========================
 R_PreciseCullSurface

	Check the surface for visibility on a per-triangle basis
	for cases when it is going to be VERY expensive to draw (subviews)

	If not culled, also returns the bounding box of the surface in
	Normalized Device Coordinates, so it can be used to crop the scissor rect.

	OPTIMIZE: we could also take exact portal passing into consideration
=========================
*/
bool idRenderView::PreciseCullSurface( const drawSurf_t * const drawSurf, idBounds & ndcBounds ) const
{
	const idTriangles* tri = drawSurf->frontEndGeo;

	unsigned int pointOr = 0;
	unsigned int pointAnd = ( unsigned int )~0;

	// get an exact bounds of the triangles for scissor cropping
	ndcBounds.Clear();

	// RB: added check wether GPU skinning is available at all
	const idJointMat* joints = ( tri->staticModelWithJoints != NULL && r_useGPUSkinning.GetBool() && glConfig.gpuSkinningAvailable ) ? tri->staticModelWithJoints->jointsInverted : NULL;
	// RB end

	for( int i = 0; i < tri->numVerts; i++ )
	{
		const idVec3 vXYZ = idDrawVert::GetSkinnedDrawVertPosition( tri->GetVertex( i ), joints );

		idVec4 eye, clip;
		idRenderMatrix::TransformModelToClip( vXYZ, drawSurf->space->modelViewMatrix, this->GetProjectionMatrix(), eye, clip );

		unsigned int pointFlags = 0;
		for( int j = 0; j < 3; j++ )
		{
			if( clip[ j ] >= clip[ 3 ] )
			{
				pointFlags |= ( 1 << ( j * 2 + 0 ) );
			}
			else if( clip[ j ] <= -clip[ 3 ] )  	// FIXME: the D3D near clip plane is at zero instead of -1
			{
				pointFlags |= ( 1 << ( j * 2 + 1 ) );
			}
		}

		pointAnd &= pointFlags;
		pointOr |= pointFlags;
	}

	// trivially reject
	if( pointAnd != 0 )
	{
		return true;
	}

	// backface and frustum cull
	idVec3 localViewOrigin;
	drawSurf->space->modelMatrix.InverseTransformPoint( this->GetOrigin(), localViewOrigin );

	for( int i = 0; i < tri->numIndexes; i += 3 )
	{
		const idVec3 v1 = idDrawVert::GetSkinnedDrawVertPosition( tri->GetIVertex( i + 0 ), joints );
		const idVec3 v2 = idDrawVert::GetSkinnedDrawVertPosition( tri->GetIVertex( i + 1 ), joints );
		const idVec3 v3 = idDrawVert::GetSkinnedDrawVertPosition( tri->GetIVertex( i + 2 ), joints );

		// this is a hack, because R_GlobalPointToLocal doesn't work with the non-normalized
		// axis that we get from the gui view transform.  It doesn't hurt anything, because
		// we know that all gui generated surfaces are front facing
		if( tr.guiRecursionLevel == 0 )
		{
			// we don't care that it isn't normalized,
			// all we want is the sign
			const idVec3 d1 = v2 - v1;
			const idVec3 d2 = v3 - v1;
			const idVec3 normal = d2.Cross( d1 );

			const idVec3 dir = v1 - localViewOrigin;

			const float dot = normal * dir;
			if( dot >= 0.0f )
			{
				return true;
			}
		}

		// now find the exact screen bounds of the clipped triangle
		idFixedWinding w;
		w.SetNumPoints( 3 );
		drawSurf->space->modelMatrix.TransformPoint( v1, w[ 0 ].ToVec3() );
		drawSurf->space->modelMatrix.TransformPoint( v2, w[ 1 ].ToVec3() );
		drawSurf->space->modelMatrix.TransformPoint( v3, w[ 2 ].ToVec3() );
		w[ 0 ].s = w[ 0 ].t = w[ 1 ].s = w[ 1 ].t = w[ 2 ].s = w[ 2 ].t = 0.0f;

		for( int j = 0; j < 4; j++ )
		{
			if( !w.ClipInPlace( -this->GetBaseFrustum()[ j ], 0.1f ) )
			{
				break;
			}
		}
		for( int j = 0; j < w.GetNumPoints(); j++ )
		{
			idVec3 screen;
			this->GlobalToNormalizedDeviceCoordinates( w[ j ].ToVec3(), screen );
			ndcBounds.AddPoint( screen );
		}
	}

	// if we don't enclose any area, return
	if( ndcBounds.IsCleared() )
	{
		return true;
	}

	return false;
}


/*
======================
R_ScreenRectFromViewFrustumBounds
======================
*/
bool CullLightToView( const idRenderLightLocal* light, idBounds *_projected )
{
	// Calculate the matrix that projects the zero-to-one cube to exactly cover the
	// light frustum in clip space.
	idRenderMatrix invProjectMVPMatrix;
	idRenderMatrix::Multiply( tr.viewDef->GetMVPMatrix(), light->inverseBaseLightProject, invProjectMVPMatrix );

	// Calculate the projected bounds, either not clipped at all, near clipped, or fully clipped.
	idBounds projected;
	if( r_useLightScissors.GetInteger() == 1 )
	{
		idRenderMatrix::ProjectedBounds( projected, invProjectMVPMatrix, bounds_zeroOneCube );
	}
	else if( r_useLightScissors.GetInteger() == 2 )
	{
		idRenderMatrix::ProjectedNearClippedBounds( projected, invProjectMVPMatrix, bounds_zeroOneCube );
	}
	else {
		idRenderMatrix::ProjectedFullyClippedBounds( projected, invProjectMVPMatrix, bounds_zeroOneCube );
	}

	if( _projected ) 
	{
		*_projected = projected;
	}

	// 'true' if the light was culled to the view frustum
	return( projected[ 0 ][ 2 ] >= projected[ 1 ][ 2 ] );
}
