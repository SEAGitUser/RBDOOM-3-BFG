/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

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

#undef min			// windef.h macros
#undef max

#include "BoundsTrack.h"

/*

We want to do one SIMD compare on 8 short components and know that the bounds
overlap if all 8 tests pass

*/

// shortBounds_t is used to track the reference bounds of all entities in a
// cache-friendly and easy to compare way.
//
// To allow all elements to be compared with a single comparison sense, the maxs
// are stored as negated values.
//
// We may need to add a global scale factor to this if there are intersections
// completely outside +/-32k
struct shortBounds_t {
	shortBounds_t()
	{
		SetToEmpty();
	}

	shortBounds_t( const idBounds& b )
	{
		SetFromReferenceBounds( b );
	}

	short	b[ 2 ][ 4 ];		// fourth element is just for padding

	idBounds ToFloatBounds() const
	{
		idBounds	f;
		for( int i = 0; i < 3; i++ )
		{
			f[ 0 ][ i ] = b[ 0 ][ i ];
			f[ 1 ][ i ] = -b[ 1 ][ i ];
		}
		return f;
	}

	bool	IntersectsShortBounds( shortBounds_t& comp ) const
	{
		shortBounds_t test;
		comp.MakeComparisonBounds( test );
		return IntersectsComparisonBounds( test );
	}

	bool	IntersectsComparisonBounds( shortBounds_t& test ) const
	{
		// this can be a single ALTIVEC vcmpgtshR instruction
		return test.b[ 0 ][ 0 ] > b[ 0 ][ 0 ]
			&& test.b[ 0 ][ 1 ] > b[ 0 ][ 1 ]
			&& test.b[ 0 ][ 2 ] > b[ 0 ][ 2 ]
			&& test.b[ 0 ][ 3 ] > b[ 0 ][ 3 ]
			&& test.b[ 1 ][ 0 ] > b[ 1 ][ 0 ]
			&& test.b[ 1 ][ 1 ] > b[ 1 ][ 1 ]
			&& test.b[ 1 ][ 2 ] > b[ 1 ][ 2 ]
			&& test.b[ 1 ][ 3 ] > b[ 1 ][ 3 ];
	}

	void MakeComparisonBounds( shortBounds_t& comp ) const
	{
		comp.b[ 0 ][ 0 ] = -b[ 1 ][ 0 ];
		comp.b[ 1 ][ 0 ] = -b[ 0 ][ 0 ];
		comp.b[ 0 ][ 1 ] = -b[ 1 ][ 1 ];
		comp.b[ 1 ][ 1 ] = -b[ 0 ][ 1 ];
		comp.b[ 0 ][ 2 ] = -b[ 1 ][ 2 ];
		comp.b[ 1 ][ 2 ] = -b[ 0 ][ 2 ];
		comp.b[ 0 ][ 3 ] = 0x7fff;
		comp.b[ 1 ][ 3 ] = 0x7fff;
	}

	void SetFromReferenceBounds( const idBounds& set )
	{
		// the maxs are stored negated
		for( int i = 0; i < 3; i++ )
		{
			// RB: replaced std::min, max
			int minv = idMath::Floor( set[ 0 ][ i ] );
			b[ 0 ][ i ] = idMath::Max( -32768, minv );
			int maxv = -idMath::Ceil( set[ 1 ][ i ] );
			b[ 1 ][ i ] = idMath::Min( 32767, maxv );
			// RB end
		}
		b[ 0 ][ 3 ] = b[ 1 ][ 3 ] = 0;
	}

	void SetToEmpty()
	{
		// this will always fail the comparison
		for( int i = 0; i < 2; i++ )
		{
			for( int j = 0; j < 4; j++ )
			{
				b[ i ][ j ] = 0x7fff;
			}
		}
	}
};

// pure function
int FindBoundsIntersectionsTEST(
	const shortBounds_t			testBounds,
	const shortBounds_t* const	boundsList,
	const int					numBounds,
	int* const					returnedList )
{
	int hits = 0;
	idBounds	testF = testBounds.ToFloatBounds();
	for( int i = 0; i < numBounds; i++ )
	{
		idBounds listF = boundsList[ i ].ToFloatBounds();
		if( testF.IntersectsBounds( listF ) )
		{
			returnedList[ hits++ ] = i;
		}
	}
	return hits;
}

// pure function
int FindBoundsIntersectionsSimSIMD(
	const shortBounds_t			testBounds,
	const shortBounds_t* const	boundsList,
	const int					numBounds,
	int* const					returnedList )
{
	shortBounds_t	compareBounds;
	testBounds.MakeComparisonBounds( compareBounds );

	int hits = 0;
	for( int i = 0; i < numBounds; i++ )
	{
		const shortBounds_t& listBounds = boundsList[ i ];
		bool	compare[ 8 ];
		int		count = 0;
		for( int j = 0; j < 8; j++ )
		{
			if( ( ( short* )&compareBounds )[ j ] >= ( ( short* )&listBounds )[ j ] )
			{
				compare[ j ] = true;
				count++;
			}
			else
			{
				compare[ j ] = false;
			}
		}
		if( count == 8 )
		{
			returnedList[ hits++ ] = i;
		}
	}
	return hits;
}

idBoundsTrack::idBoundsTrack()
{
	boundsList = ( shortBounds_t* )Mem_Alloc( MAX_BOUNDS_TRACK_INDEXES * sizeof( *boundsList ), TAG_RENDER );
	ClearAll();
}

idBoundsTrack::~idBoundsTrack()
{
	Mem_Free( boundsList );
}

void idBoundsTrack::ClearAll()
{
	maxIndex = 0;
	for( int i = 0; i < MAX_BOUNDS_TRACK_INDEXES; i++ )
	{
		ClearIndex( i );
	}
}

void	idBoundsTrack::SetIndex( const int index, const idBounds& bounds )
{
	assert( ( unsigned )index < MAX_BOUNDS_TRACK_INDEXES );
	// RB: replaced std::max
	maxIndex = idMath::Max( maxIndex, index + 1 );
	// RB end
	boundsList[ index ].SetFromReferenceBounds( bounds );
}

void	idBoundsTrack::ClearIndex( const int index )
{
	assert( ( unsigned )index < MAX_BOUNDS_TRACK_INDEXES );
	boundsList[ index ].SetToEmpty();
}

int		idBoundsTrack::FindIntersections( const idBounds& testBounds, int intersectedIndexes[ MAX_BOUNDS_TRACK_INDEXES ] ) const
{
	const shortBounds_t	shortTestBounds( testBounds );
	return FindBoundsIntersectionsTEST( shortTestBounds, boundsList, maxIndex, intersectedIndexes );
}

void	idBoundsTrack::Test()
{
	ClearAll();
	idRandom	r;

	for( int i = 0; i < 1800; i++ )
	{
		idBounds b;
		for( int j = 0; j < 3; j++ )
		{
			b[ 0 ][ j ] = r.RandomInt( 20000 ) - 10000;
			b[ 1 ][ j ] = b[ 0 ][ j ] + r.RandomInt( 1000 );
		}
		SetIndex( i, b );
	}

	const idBounds testBounds( idVec3( -1000, 2000, -3000 ), idVec3( 1500, 4500, -500 ) );
	SetIndex( 1800, testBounds );
	SetIndex( 0, testBounds );

	const shortBounds_t	shortTestBounds( testBounds );

	int intersectedIndexes1[ MAX_BOUNDS_TRACK_INDEXES ];
	const int numHits1 = FindBoundsIntersectionsTEST( shortTestBounds, boundsList, maxIndex, intersectedIndexes1 );

	int intersectedIndexes2[ MAX_BOUNDS_TRACK_INDEXES ];
	const int numHits2 = FindBoundsIntersectionsSimSIMD( shortTestBounds, boundsList, maxIndex, intersectedIndexes2 );
	idLib::Printf( "%i intersections\n", numHits1 );
	if( numHits1 != numHits2 )
	{
		idLib::Printf( "different results\n" );
	}
	else
	{
		for( int i = 0; i < numHits1; i++ )
		{
			if( intersectedIndexes1[ i ] != intersectedIndexes2[ i ] )
			{
				idLib::Printf( "different results\n" );
				break;
			}
		}
	}

	// run again for debugging failure
	FindBoundsIntersectionsTEST( shortTestBounds, boundsList, maxIndex, intersectedIndexes1 );
	FindBoundsIntersectionsSimSIMD( shortTestBounds, boundsList, maxIndex, intersectedIndexes2 );

	// timing
	const int64 start = sys->Microseconds();
	for( int i = 0; i < 40; i++ )
	{
		FindBoundsIntersectionsSimSIMD( shortTestBounds, boundsList, maxIndex, intersectedIndexes2 );
	}
	const int64 stop = sys->Microseconds();
	idLib::Printf( "%lli microseconds for 40 itterations\n", stop - start );
}

class interactionPair_t {
	int		entityIndex;
	int		lightIndex;
};

/*

keep a sorted list of static interactions and interactions already generated this frame?

determine if the light needs more exact culling because it is rotated or a spot light
for each entity on the bounds intersection list
	if entity is not directly visible, determine if it can cast a shadow into the view
		if the light center is in-frustum
			and the entity bounds is out-of-frustum, it can't contribue
		else the light center is off-frustum
			if any of the view frustum planes can be moved out to the light center and the entity bounds is still outside it, it can't contribute
	if a static interaction exists
		continue
	possibly perform more exact refernce bounds to rotated or spot light

	create an interaction pair and add it to the list

all models will have an interaction with light -1 for ambient surface
sort the interaction list by model
do
	if the model is dynamic, create it
	add the ambient surface and skip interaction -1
	for all interactions
		check for static interaction
		check for current-frame interaction
		else create shadow for this light

*/

#if 0

/*
=====================================================================================================
 SSE Сфера-фрустум.

	Алгоритм кулинга сфер и AABB в целом идентичен SphereInFrustum. За исключением того, 
	что производим операции одновременно над 4мя объектами. Что очень удобно ложиться на архитектуру.
=====================================================================================================
*/
void sse_culling_spheres( idSphere *sphere_data, int num_objects, int *culling_res, idVec4 *frustum_planes )
{
	float *sphere_data_ptr = reinterpret_cast<float*>( &sphere_data[ 0 ] );
	int *culling_res_sse = &culling_res[ 0 ];

	//для оптимизации вычислений, собираем xyzw компоненты в отдельные вектора
	__m128 zero_v = _mm_setzero_ps();
	__m128 frustum_planes_x[ 6 ];
	__m128 frustum_planes_y[ 6 ];
	__m128 frustum_planes_z[ 6 ];
	__m128 frustum_planes_d[ 6 ];
	int i, j;
	for( i = 0; i < 6; i++ )
	{
		frustum_planes_x[ i ] = _mm_set1_ps( frustum_planes[ i ].x );
		frustum_planes_y[ i ] = _mm_set1_ps( frustum_planes[ i ].y );
		frustum_planes_z[ i ] = _mm_set1_ps( frustum_planes[ i ].z );
		frustum_planes_d[ i ] = _mm_set1_ps( frustum_planes[ i ].w );
	}

	//обрабатываем сразу 4 объекта за раз
	for( i = 0; i < num_objects; i += 4 )
	{
		//загружаем данные окружающих сфер
		__m128 spheres_pos_x = _mm_load_ps( sphere_data_ptr );
		__m128 spheres_pos_y = _mm_load_ps( sphere_data_ptr + 4 );
		__m128 spheres_pos_z = _mm_load_ps( sphere_data_ptr + 8 );
		__m128 spheres_radius = _mm_load_ps( sphere_data_ptr + 12 );
		sphere_data_ptr += 16;

		//но для вычислений нам нужно транспонировать данные в векторах,
		//чтобы собрать x, y, z и w координаты в отдельные вектора
		_MM_TRANSPOSE4_PS( spheres_pos_x, spheres_pos_y, spheres_pos_z, spheres_radius );

		//spheres_neg_radius = -spheres_radius
		__m128 spheres_neg_radius = _mm_sub_ps( zero_v, spheres_radius );

		__m128 intersection_res = _mm_setzero_ps(); // = 0

		for( j = 0; j < 6; j++ ) //нужно протестировать 6 плоскостей фрустума
		{
			//1. считаем расстояние от центра сферы дл плоскости. dot(sphere_pos.xyz, plane.xyz) + plane.w
			//2. если центр сферы находится за плоскостью и расстояние по модулю больше радиуса сферы,
			//то объект находится вне фрустума
			__m128 dot_x = _mm_mul_ps( spheres_pos_x, frustum_planes_x[ j ] ); //sphere_pos_x * plane_x
			__m128 dot_y = _mm_mul_ps( spheres_pos_y, frustum_planes_y[ j ] ); //sphere_pos_y * plane_y
			__m128 dot_z = _mm_mul_ps( spheres_pos_z, frustum_planes_z[ j ] ); //sphere_pos_z * plane_z

			__m128 sum_xy = _mm_add_ps( dot_x, dot_y ); //x+y
			__m128 sum_zw = _mm_add_ps( dot_z, frustum_planes_d[ j ] ); //z+w

			//в distance_to_plane - расстояние от центров сфер до плоскости, dot(sphere_pos.xyz, plane.xyz) + plane.w
			//посчитанное для 4 сфер одновременно
			__m128 distance_to_plane = _mm_add_ps( sum_xy, sum_zw );

			//если расстояние < -sphere_r (т.е. сфера находится за плоскостью, на расстоянии большим чем радиус сферы)
			__m128 plane_res = _mm_cmple_ps( distance_to_plane, spheres_neg_radius );

			//если да - объект не попадает во фрустум
			intersection_res = _mm_or_ps( intersection_res, plane_res );
		}

		//сохраняем результат
		__m128i intersection_res_i = _mm_cvtps_epi32( intersection_res );
		_mm_store_si128( ( __m128i * )&culling_res_sse[ i ], intersection_res_i );
	}
}

/*
=====================================================================================================
 SSE AABB-фрустум.

	Алгоритм для AABB также идентичен исходному на с++. Аналогично sse кулинuу сфер производим 
	операции одновременно над 4мя объектами.
=====================================================================================================
*/
void sse_culling_aabb( idBounds *aabb_data, int num_objects, int *culling_res, idVec4 *frustum_planes )
{
	float *aabb_data_ptr = reinterpret_cast<float*>( &aabb_data[ 0 ] );
	int *culling_res_sse = &culling_res[ 0 ];

	//для оптимизации вычислений, собираем xyzw компоненты в отдельные вектора
	__m128 zero_v = _mm_setzero_ps();
	__m128 frustum_planes_x[ 6 ];
	__m128 frustum_planes_y[ 6 ];
	__m128 frustum_planes_z[ 6 ];
	__m128 frustum_planes_d[ 6 ];
	int i, j;
	for( i = 0; i < 6; i++ )
	{
		frustum_planes_x[ i ] = _mm_set1_ps( frustum_planes[ i ].x );
		frustum_planes_y[ i ] = _mm_set1_ps( frustum_planes[ i ].y );
		frustum_planes_z[ i ] = _mm_set1_ps( frustum_planes[ i ].z );
		frustum_planes_d[ i ] = _mm_set1_ps( frustum_planes[ i ].w );
	}

	const __m128 zero = _mm_setzero_ps();
	//обрабатываем сразу 4 объекта за раз
	for( i = 0; i < num_objects; i += 4 )
	{
		//загружаем данные объектов
		//загружаем в регистры aabb min
		__m128 aabb_min_x = _mm_load_ps( aabb_data_ptr );
		__m128 aabb_min_y = _mm_load_ps( aabb_data_ptr + 8 );
		__m128 aabb_min_z = _mm_load_ps( aabb_data_ptr + 16 );
		__m128 aabb_min_w = _mm_load_ps( aabb_data_ptr + 24 );

		//загружаем в регистры aabb max
		__m128 aabb_max_x = _mm_load_ps( aabb_data_ptr + 4 );
		__m128 aabb_max_y = _mm_load_ps( aabb_data_ptr + 12 );
		__m128 aabb_max_z = _mm_load_ps( aabb_data_ptr + 20 );
		__m128 aabb_max_w = _mm_load_ps( aabb_data_ptr + 28 );

		aabb_data_ptr += 32;

		//в массиве aabb_data расположены вершины AABB (сразу в мировом пространстве!)
		//но для вычислений нам нужно иметь в векторах xxxx yyyy zzzz представление, просто транспонируем данные
		_MM_TRANSPOSE4_PS( aabb_min_x, aabb_min_y, aabb_min_z, aabb_min_w );
		_MM_TRANSPOSE4_PS( aabb_max_x, aabb_max_y, aabb_max_z, aabb_max_w );

		__m128 intersection_res = _mm_setzero_ps(); //=0
		for( j = 0; j < 6; j++ ) //plane index
		{
			//этот код идентичен тому что мы делали в с++ реализации
			//находим ближайшую вершину AABB к плоскости и проверяем находится ли она за плоскостью.
			//Если да - объект вне фрустума

			//dot product, отдельно для каждой координаты, для двух (min и max) вершин
			__m128 aabbMin_frustumPlane_x = _mm_mul_ps( aabb_min_x, frustum_planes_x[ j ] );
			__m128 aabbMin_frustumPlane_y = _mm_mul_ps( aabb_min_y, frustum_planes_y[ j ] );
			__m128 aabbMin_frustumPlane_z = _mm_mul_ps( aabb_min_z, frustum_planes_z[ j ] );

			__m128 aabbMax_frustumPlane_x = _mm_mul_ps( aabb_max_x, frustum_planes_x[ j ] );
			__m128 aabbMax_frustumPlane_y = _mm_mul_ps( aabb_max_y, frustum_planes_y[ j ] );
			__m128 aabbMax_frustumPlane_z = _mm_mul_ps( aabb_max_z, frustum_planes_z[ j ] );

			//мы имеем 8 вершин в боксе, но нам нужно выбрать одну ближайшуй к плоскости.
			//Берем максимальное значение по каждой компоненте, это и даст нам ближайшую точку
			__m128 res_x = _mm_max_ps( aabbMin_frustumPlane_x, aabbMax_frustumPlane_x );
			__m128 res_y = _mm_max_ps( aabbMin_frustumPlane_y, aabbMax_frustumPlane_y );
			__m128 res_z = _mm_max_ps( aabbMin_frustumPlane_z, aabbMax_frustumPlane_z );

			//считаем расстояние дл плоскости = dot(aabb_point.xyz, plane.xyz) + plane.w
			__m128 sum_xy = _mm_add_ps( res_x, res_y );
			__m128 sum_zw = _mm_add_ps( res_z, frustum_planes_d[ j ] );
			__m128 distance_to_plane = _mm_add_ps( sum_xy, sum_zw );

			__m128 plane_res = _mm_cmple_ps( distance_to_plane, zero ); //расстояние до плоскости < 0 ?
			intersection_res = _mm_or_ps( intersection_res, plane_res ); //если да, то объект вне врустума
		}

		//сохраняем результат
		__m128i intersection_res_i = _mm_cvtps_epi32( intersection_res );
		_mm_store_si128( ( __m128i * )&culling_res_sse[ i ], intersection_res_i );
	}
}

const float AREA_SIZE = 20.f;
const float half_box_size = 0.05f;
const idVec3 box_max = idVec3( half_box_size, half_box_size, half_box_size );
const idVec3 box_min = -idVec3( half_box_size, half_box_size, half_box_size );
const idVec3 box_half_size = idVec3( half_box_size, half_box_size, half_box_size );
const float bounding_radius = sqrtf( 3.0f ) * half_box_size;

ALIGNTYPE16 struct mat4_sse {
	__m128 col0;
	__m128 col1;
	__m128 col2;
	__m128 col3;

	mat4_sse() {}
	mat4_sse( idMat4 &m )
	{
		set( m );
	}
	inline void set( idMat4 &m )
	{
		col0 = _mm_loadu_ps( &m.mat[  0 ] );
		col1 = _mm_loadu_ps( &m.mat[  4 ] );
		col2 = _mm_loadu_ps( &m.mat[  8 ] );
		col3 = _mm_loadu_ps( &m.mat[ 12 ] );
	}
};

//http://stackoverflow.com/questions/38090188/matrix-multiplication-using-sse
//https://gist.github.com/rygorous/4172889

__forceinline __m128 sse_mat4_mul_vec4( mat4_sse &m, __m128 v )
{
	__m128 xxxx = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 0, 0, 0, 0 ) );
	__m128 yyyy = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 1, 1, 1, 1 ) );
	__m128 zzzz = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 2, 2, 2, 2 ) );
	__m128 wwww = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 3, 3, 3, 3 ) );

	return _mm_add_ps(
		_mm_add_ps( _mm_mul_ps( xxxx, m.col0 ), _mm_mul_ps( yyyy, m.col1 ) ),
		_mm_add_ps( _mm_mul_ps( zzzz, m.col2 ), _mm_mul_ps( wwww, m.col3 ) )
	);
}

__forceinline void sse_mat4_mul( mat4_sse &dest, mat4_sse &m1, mat4_sse &m2 )
{
	dest.col0 = sse_mat4_mul_vec4( m1, m2.col0 );
	dest.col1 = sse_mat4_mul_vec4( m1, m2.col1 );
	dest.col2 = sse_mat4_mul_vec4( m1, m2.col2 );
	dest.col3 = sse_mat4_mul_vec4( m1, m2.col3 );
}

/*
=====================================================================================================
 SSE OBB-фрустум.

	Кулинг OBB немного сложнее. Мы производим вычисления над одним объектом. Но при этом проводим 
	вычисления одновременно над тремя осями xyz. Это не самый оптимальный алгоритм, но он передает 
	общую идею алгоритма. К тому же векторная математика (умножение матриц и трансформация векторов)
	с использованием SSE выполняются быстрее.
=====================================================================================================
*/
void sse_culling_obb( int firs_processing_object, int num_objects, int *culling_res, idMat4 &cam_modelview_proj_mat )
{
	mat4_sse sse_camera_mat( cam_modelview_proj_mat );
	mat4_sse sse_clip_space_mat;

	//вершины бокса в локальных координатах
	__m128 obb_points_sse[ 8 ];
	obb_points_sse[ 0 ] = _mm_set_ps( 1.0f, box_min[ 2 ], box_max[ 1 ], box_min[ 0 ] );
	obb_points_sse[ 1 ] = _mm_set_ps( 1.0f, box_max[ 2 ], box_max[ 1 ], box_min[ 0 ] );
	obb_points_sse[ 2 ] = _mm_set_ps( 1.0f, box_max[ 2 ], box_max[ 1 ], box_max[ 0 ] );
	obb_points_sse[ 3 ] = _mm_set_ps( 1.0f, box_min[ 2 ], box_max[ 1 ], box_max[ 0 ] );
	obb_points_sse[ 4 ] = _mm_set_ps( 1.0f, box_min[ 2 ], box_min[ 1 ], box_max[ 0 ] );
	obb_points_sse[ 5 ] = _mm_set_ps( 1.0f, box_max[ 2 ], box_min[ 1 ], box_max[ 0 ] );
	obb_points_sse[ 6 ] = _mm_set_ps( 1.0f, box_max[ 2 ], box_min[ 1 ], box_min[ 0 ] );
	obb_points_sse[ 7 ] = _mm_set_ps( 1.0f, box_min[ 2 ], box_min[ 1 ], box_min[ 0 ] );

	ALIGN16( int obj_culling_res[ 4 ] );

	__m128 zero_v = _mm_setzero_ps();
	int i, j;

	//производим вычисления над одним объектов за раз
	for( i = firs_processing_object; i < firs_processing_object + num_objects; i++ )
	{
		//матрица перевода координат сразу в clip space, space matrix = camera_view_proj * obj_mat
		sse_mat4_mul( sse_clip_space_mat, sse_camera_mat, sse_obj_mat[ i ] );

		//тут внимательно: в _mm_set1_ps() нужно передать именно отрицательное число
		//потому-что _mm_movemask_ps (при сохранении результата) проверяет 'первый значащий бит' (у float'а это знак)
		__m128 outside_positive_plane = _mm_set1_ps( -1.f );
		__m128 outside_negative_plane = _mm_set1_ps( -1.f );

		//для всех 8 точек
		for( j = 0; j < 8; j++ )
		{
			//трансформируем в clip space (умножение вектора на матрицу)
			__m128 obb_transformed_point = sse_mat4_mul_vec4( sse_clip_space_mat, obb_points_sse[ j ] );

			//собираем w и -w в отдельные вектора, чтобы быстро сравнивать
			__m128 wwww = _mm_shuffle_ps( obb_transformed_point, obb_transformed_point, _MM_SHUFFLE( 3, 3, 3, 3 ) );
			__m128 wwww_neg = _mm_sub_ps( zero_v, wwww ); //получаем -w-w-w-w

			//box_point.xyz > box_point.w || box_point.xyz < -box_point.w ?
			//можно нормализовать координаты (point.xyz /= point.w;) Затем сравнить с -1 и 1. point.xyz > 1 && point.xyz < -1
			__m128 outside_pos_plane = _mm_cmpge_ps( obb_transformed_point, wwww );
			__m128 outside_neg_plane = _mm_cmple_ps( obb_transformed_point, wwww_neg );

			//если хотя бы 1 из 8 вершин спереди плоскости, то получим 0 в outside_* flag.
			outside_positive_plane = _mm_and_ps( outside_positive_plane, outside_pos_plane );
			outside_negative_plane = _mm_and_ps( outside_negative_plane, outside_neg_plane );
		}

		//находятся ли все 8 вершин за какой плоскостью, по 3м осям результат хранится отдельно
		__m128 outside = _mm_or_ps( outside_positive_plane, outside_negative_plane );

		//сохраняем результат
		//сейчас у нас результат хранится отдельно по каждой оси
		//нужно скомбинировать результаты. Если хотя бы по одной оси все 8 вершин либо >1 либо < 1
		//то объект находится вне области видимости / фрустума
		//другими словами, если outside[0, 1 или 2] не 0... то по одной из осей наш объект вне фрустума
		culling_res[ i ] = _mm_movemask_ps( outside ) & 0x7; // & 0x7 потому что интересуют только 3 оси
	}
}

#endif