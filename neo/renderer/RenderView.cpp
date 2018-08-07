
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

#if 0
/*
=========================================================

 ModifyProjectionMatrix

 The following code modifies the OpenGL projection matrix so that the near plane of the standard
 view frustum is moved so that it coincides with a given arbitrary plane. The far plane is adjusted
 so that the resulting view frustum has the best shape possible. This code assumes that the original
 projection matrix is a perspective projection (standard or infinite). The clipPlane parameter must
 be in camera-space coordinates, and its w-coordinate must be negative (corresponding to the camera
 being on the negative side of the plane).

 For algorithmic details, see “Oblique View Frustum Depth Projection and Clipping”,
 by Eric Lengyel. This technique is also described in Game Programming Gems 5, Section 2.6.

=========================================================
*/
inline float sgn( float a )
{
	if( a > 0.0F ) return ( 1.0F );
	if( a < 0.0F ) return ( -1.0F );
	return ( 0.0F );
}
void ModifyProjectionMatrixGL( const idVec4 & clipPlane )
{
	float     matrix[ 16 ];
	idVec4    q;

	// Grab the current projection matrix from OpenGL
	glGetFloatv( GL_PROJECTION_MATRIX, matrix );

	// Calculate the clip-space corner point opposite the clipping plane
	// as (sgn(clipPlane.x), sgn(clipPlane.y), 1, 1) and
	// transform it into camera space by multiplying it
	// by the inverse of the projection matrix

	q.x = ( sgn( clipPlane.x ) + matrix[ 8 ] ) / matrix[ 0 ];
	q.y = ( sgn( clipPlane.y ) + matrix[ 9 ] ) / matrix[ 5 ];
	q.z = -1.0F;
	q.w = ( 1.0F + matrix[ 10 ] ) / matrix[ 14 ];

	// Calculate the scaled plane vector
	idVec4 c = clipPlane * ( 2.0F / ( clipPlane * q ) ); // Dot( clipPlane, q )

	// Replace the third row of the projection matrix
	matrix[ 2 ] = c.x;
	matrix[ 6 ] = c.y;
	matrix[ 10 ] = c.z + 1.0F;
	matrix[ 14 ] = c.w;

	// Load it back into OpenGL
	glMatrixMode( GL_PROJECTION );
	glLoadMatrixf( matrix );
}
void ModifyProjectionMatrixDX( const idVec4 & clipPlane )
{
	D3DXMatrix	matrix;
	Vector4D	q;

	// Grab the current projection matrix from Direct3D
	D3DDevice.GetTransform( D3DTS_PROJECTION, &matrix );

	// Transform the clip-space corner point opposite the clipping plane
	// into camera space by multiplying it by the inverse of
	// the projection matrix
	q.x = ( sgn( clipPlane.x ) - matrix._31 ) / matrix._11;
	q.y = ( sgn( clipPlane.y ) - matrix._32 ) / matrix._22;
	q.z = 1.0F;
	q.w = ( 1.0F - matrix._33 ) / matrix._43;

	// Calculate the scaled plane vector
	Vector4D c = clipPlane * ( 1.0F / ( clipPlane * q ) );

	// Replace the third row of the projection matrix
	matrix._13 = c.x;
	matrix._23 = c.y;
	matrix._33 = c.z;
	matrix._43 = c.w;

	// Load it back into Direct3D
	D3DDevice.SetTransform( D3DTS_PROJECTION, &matrix );
}

// PolygonOffset
void LoadOffsetMatrix( GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f, GLfloat delta, GLfloat pz )
{
	GLfloat	matrix[ 16 ];

	// Set up standard perspective projection
	glMatrixMode( GL_PROJECTION );
	glFrustum( l, r, b, t, n, f );

	// Retrieve the projection matrix
	glGetFloatv( GL_PROJECTION_MATRIX, matrix );

	// Calculate epsilon with equation (7)
	GLfloat epsilon = -2.0F * f * n * delta / ( ( f + n ) * pz * ( pz + delta ) );

	// Modify entry (3,3) of the projection matrix
	matrix[ 10 ] *= 1.0F + epsilon;

	// Send the projection matrix back to OpenGL
	glLoadMatrixf( matrix );
}

// Frustum Cilinder culling

class Frustum {
private:

	// Near and far plane distances
	float		nearDistance;
	float		farDistance;

	// Precalculated normal components
	float		leftRightX;
	float		leftRightZ;
	float		topBottomY;
	float		topBottomZ;

public:

	// Constructor defines the frustum
	Frustum( float l, float a, float n, float f );

	// Intersection test returns true or false
	bool CylinderVisible( vector3 p1, vector3 p2, float radius ) const;
};

Frustum::Frustum( float l, float a, float n, float f )
{
	// Save off near plane and far plane distances
	nearDistance = n;
	farDistance = f;

	// Precalculate side plane normal components
	float d = 1.0F / sqrt( l * l + 1.0F );
	leftRightX = l * d;
	leftRightZ = d;

	d = 1.0F / sqrt( l * l + a * a );
	topBottomY = l * d;
	topBottomZ = a * d;
}

bool Frustum::CylinderVisible( vector3 p1, vector3 p2, float radius ) const
{
	// Calculate unit vector representing cylinder's axis
	vector3 dp = p2 - p1;
	dp.normalize();

	// Visit near plane first, N = (0,0,-1)
	float dot1 = -p1.z;
	float dot2 = -p2.z;

	// Calculate effective radius for near and far planes
	float effectiveRadius = radius * sqrt( 1.0F - dp.z * dp.z );

	// Test endpoints against adjusted near plane
	float d = nearDistance - effectiveRadius;
	bool interior1 = ( dot1 > d );
	bool interior2 = ( dot2 > d );

	if( !interior1 )
	{
		// If neither endpoint is interior, cylinder is not visible
		if( !interior2 ) return ( false );

		// p1 was outside, so move it to the near plane
		float t = ( d + p1.z ) / dp.z;
		p1.x -= t * dp.x;
		p1.y -= t * dp.y;
		p1.z = -d;
	}
	else if( !interior2 )
	{
		// p2 was outside, so move it to the near plane
		float t = ( d + p1.z ) / dp.z;
		p2.x = p1.x - t * dp.x;
		p2.y = p1.y - t * dp.y;
		p2.z = -d;
	}

	// Test endpoints against adjusted far plane
	d = farDistance + effectiveRadius;
	interior1 = ( dot1 < d );
	interior2 = ( dot2 < d );

	if( !interior1 )
	{
		// If neither endpoint is interior, cylinder is not visible
		if( !interior2 ) return ( false );

		// p1 was outside, so move it to the far plane
		float t = ( d + p1.z ) / ( p2.z - p1.z );
		p1.x -= t * ( p2.x - p1.x );
		p1.y -= t * ( p2.y - p1.y );
		p1.z = -d;
	}
	else if( !interior2 )
	{
		// p2 was outside, so move it to the far plane
		float t = ( d + p1.z ) / ( p2.z - p1.z );
		p2.x = p1.x - t * ( p2.x - p1.x );
		p2.y = p1.y - t * ( p2.y - p1.y );
		p2.z = -d;
	}

	// Visit left side plane next
	// The normal components have been precalculated
	float nx = leftRightX;
	float nz = leftRightZ;

	// Compute p1 * N and p2 * N
	dot1 = nx * p1.x - nz * p1.z;
	dot2 = nx * p2.x - nz * p2.z;

	// Calculate effective radius for this plane
	float s = nx * dp.x - nz * dp.z;
	effectiveRadius = -radius * sqrt( 1.0F - s * s );

	// Test endpoints against adjusted plane
	interior1 = ( dot1 > effectiveRadius );
	interior2 = ( dot2 > effectiveRadius );

	if( !interior1 )
	{
		// If neither endpoint is interior, cylinder is not visible
		if( !interior2 ) return ( false );

		// p1 was outside, so move it to the plane
		float t = ( effectiveRadius - dot1 ) / ( dot2 - dot1 );
		p1.x += t * ( p2.x - p1.x );
		p1.y += t * ( p2.y - p1.y );
		p1.z += t * ( p2.z - p1.z );
	}
	else if( !interior2 )
	{
		// p2 was outside, so move it to the plane
		float t = ( effectiveRadius - dot1 ) / ( dot2 - dot1 );
		p2.x = p1.x + t * ( p2.x - p1.x );
		p2.y = p1.y + t * ( p2.y - p1.y );
		p2.z = p1.z + t * ( p2.z - p1.z );
	}

	// Visit right side plane next
	dot1 = -nx * p1.x - nz * p1.z;
	dot2 = -nx * p2.x - nz * p2.z;

	s = -nx * dp.x - nz * dp.z;
	effectiveRadius = -radius * sqrt( 1.0F - s * s );

	interior1 = ( dot1 > effectiveRadius );
	interior2 = ( dot2 > effectiveRadius );

	if( !interior1 )
	{
		if( !interior2 ) return ( false );

		float t = ( effectiveRadius - dot1 ) / ( dot2 - dot1 );
		p1.x += t * ( p2.x - p1.x );
		p1.y += t * ( p2.y - p1.y );
		p1.z += t * ( p2.z - p1.z );
	}
	else if( !interior2 )
	{
		float t = ( effectiveRadius - dot1 ) / ( dot2 - dot1 );
		p2.x = p1.x + t * ( p2.x - p1.x );
		p2.y = p1.y + t * ( p2.y - p1.y );
		p2.z = p1.z + t * ( p2.z - p1.z );
	}

	// Visit top side plane next
	// The normal components have been precalculated
	float ny = topBottomY;
	nz = topBottomZ;

	dot1 = -ny * p1.y - nz * p1.z;
	dot2 = -ny * p2.y - nz * p2.z;

	s = -ny * dp.y - nz * dp.z;
	effectiveRadius = -radius * sqrt( 1.0F - s * s );

	interior1 = ( dot1 > effectiveRadius );
	interior2 = ( dot2 > effectiveRadius );

	if( !interior1 )
	{
		if( !interior2 ) return ( false );

		float t = ( effectiveRadius - dot1 ) / ( dot2 - dot1 );
		p1.x += t * ( p2.x - p1.x );
		p1.y += t * ( p2.y - p1.y );
		p1.z += t * ( p2.z - p1.z );
	}
	else if( !interior2 )
	{
		float t = ( effectiveRadius - dot1 ) / ( dot2 - dot1 );
		p2.x = p1.x + t * ( p2.x - p1.x );
		p2.y = p1.y + t * ( p2.y - p1.y );
		p2.z = p1.z + t * ( p2.z - p1.z );
	}

	// Finally, visit bottom side plane
	dot1 = ny * p1.y - nz * p1.z;
	dot2 = ny * p2.y - nz * p2.z;

	s = ny * dp.y - nz * dp.z;
	effectiveRadius = -radius * sqrt( 1.0F - s * s );

	interior1 = ( dot1 > effectiveRadius );
	interior2 = ( dot2 > effectiveRadius );

	// At least one endpoint must be interior or cylinder is not visible
	return ( interior1 | interior2 );
}

#endif

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
 ViewInsideLightVolume
======================
*/
bool idRenderView::ViewInsideLightVolume( const idBounds & LightBounds ) const
{
	const float viewNearClippingDistance = GetZNear();

	const bool bViewInsideLightVolume = ( GetOrigin() - LightBounds.GetCenter() ).LengthSqr()
		< idMath::Square( LightBounds.GetRadius() * 1.05f + viewNearClippingDistance * 2.0f );

	return bViewInsideLightVolume;
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

/*
======================
 TAA
	idVec3 translation = GetHaltonSequence();
	viewProjection = viewProjection * Matrix.CreateTranslation( translation );
======================
*/
const idVec3 & GetHaltonSequence()
{
	static idVec3 _inverseResolution;

	static idVec3 * _haltonSequence;
	static int _haltonSequenceIndex = -1;
	static const int HaltonSequenceLength = 16;

	//First time? Create the sequence
	if( _haltonSequence == nullptr )
	{
		_haltonSequence = new idVec3[ HaltonSequenceLength ];
		for( int index = 0; index < HaltonSequenceLength; index++ )
		{
			for( int baseValue = 2; baseValue <= 3; baseValue++ )
			{
				float result = 0;
				float f = 1;
				int i = index + 1;

				while( i > 0 )
				{
					f = f / baseValue;
					result = result + f * ( i % baseValue );
					i = i / baseValue; //floor / int()
				}

				if( baseValue == 2 )
					_haltonSequence[ index ].x = ( result - 0.5f ) * 2.0f * _inverseResolution.x;
				else
					_haltonSequence[ index ].y = ( result - 0.5f ) * 2.0f * _inverseResolution.y;
			}
		}
	}

	_haltonSequenceIndex++;
	if( _haltonSequenceIndex >= HaltonSequenceLength ) {
		_haltonSequenceIndex = 0;
	}
	return _haltonSequence[ _haltonSequenceIndex ];
}
