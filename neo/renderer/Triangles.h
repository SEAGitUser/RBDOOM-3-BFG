#ifndef __TR_TRIANGLES_H__
#define __TR_TRIANGLES_H__

/*
====================================================================

	idTriangles		Tech5 style

	our only drawing geometry type

====================================================================
*/
class idTriangles {
public:
	idTriangles()
	{
	} // numVerts_, numIndexes_

	idBounds					bounds;					// for culling

	bool						generateNormals;		// create normals from geometry, instead of using explicit ones
	bool						tangentsCalculated;		// set when the vertex tangents have been calculated
	bool						perfectHull;			// true if there aren't any dangling edges
	bool						referencedVerts;		// if true the 'verts' are referenced and should not be freed
	bool						referencedIndexes;		// if true, indexes, silIndexes, mirrorVerts, and silEdges are
														// pointers into the original surface, and should not be freed

	int							numVerts;				// number of vertices
	idDrawVert* 				verts;					// vertices, allocated with special allocator

	int							numIndexes;				// for shadows, this has both front and rear end caps and silhouette planes
	triIndex_t* 				indexes;				// indexes, allocated with special allocator
//private:
	triIndex_t* 				silIndexes;				// indexes changed to be the first vertex with same XYZ, ignoring normal and texcoords

	int							numMirroredVerts;		// this many verts at the end of the vert list are tangent mirrors
	int* 						mirroredVerts;			// tri->mirroredVerts[0] is the mirror of tri->numVerts - tri->numMirroredVerts + 0

	int							numDupVerts;			// number of duplicate vertexes
	int* 						dupVerts;				// pairs of the number of the first vertex and the number of the duplicate vertex

	int							numSilEdges;			// number of silhouette edges
	silEdge_t* 					silEdges;				// silhouette edges

	dominantTri_t* 				dominantTris;			// [numVerts] for deformed surface fast tangent calculation
														//public:
	int							numShadowIndexesNoFrontCaps;	// shadow volumes with front caps omitted
	int							numShadowIndexesNoCaps;			// shadow volumes with the front and rear caps omitted

	int							shadowCapPlaneBits;		// bits 0-5 are set when that plane of the interacting light has triangles
														// projected on it, which means that if the view is on the outside of that
														// plane, we need to draw the rear caps of the shadow volume
														// dynamic shadows will have SHADOW_CAP_INFINITE

	idShadowVert* 				preLightShadowVertexes;	// shadow vertices in CPU memory for pre-light shadow volumes
	idShadowVert* 				staticShadowVertexes;	// shadow vertices in CPU memory for static shadow volumes

														// for light interactions, point back at the original surface that generated
														// the interaction, which we will get the ambientCache from
	idTriangles* 				baseTriangles;

	// for deferred normal / tangent transformations by joints
	// the jointsInverted list / buffer object on md5WithJoints may be
	// shared by multiple idTriangles
	idRenderModelStatic* 		staticModelWithJoints;

	// data in vertex object space, not directly readable by the CPU
	vertCacheHandle_t			indexCache;				// GL_INDEX_TYPE
	vertCacheHandle_t			vertexCache;			// idDrawVert
	vertCacheHandle_t			shadowCache;			// idVec4

	static const ALIGNTYPE16 triIndex_t	quadIndexes[ 6 ];

private:

	void						CreateDupVerts();
	void						IdentifySilEdges( bool omitCoplanarEdges );
	void						DuplicateMirroredVertexes();
	void						DeriveNormalsAndTangents();
	void						DeriveUnsmoothedNormalsAndTangents();
	void						DeriveTangentsWithoutNormals();
	void						BuildDominantTris();

public:

	ID_INLINE void Clear()
	{
		memset( this, 0, sizeof( *this ) );
	}

	static idTriangles *		AllocStatic();
	static void					FreeStatic( idTriangles* );

	// This only duplicates the indexes and verts, not any of the derived data.
	static idTriangles *		CopyStatic( const idTriangles* source );

	// Read the contents of idTriangles from the given file
	void						ReadFromFile( idFile * /*, buffer*/ );
	// Write the contents of idTriangles to the given file
	void						WriteToFile( idFile * /*, buffer*/ ) const;

	// Only deals with vertexes and indexes, not silhouettes, planes, etc.
	// Does NOT perform a cleanup triangles, so there may be duplicated verts in the result.
	static idTriangles * 		MergeSurfaceList( const idTriangles** surfaces, int numSurfaces );
	// Only deals with vertexes and indexes, not silhouettes, planes, etc.
	// Does NOT perform a cleanup triangles, so there may be duplicated verts in the result.
	static idTriangles * 		MergeTriangles( const idTriangles* tri1, const idTriangles* tri2 );

	void						AllocStaticVerts( int numVerts );
	void						AllocStaticIndexes( int numIndexes );
	void						AllocStaticPreLightShadowVerts( int numVerts );
	void						AllocStaticSilIndexes( int numIndexes );
	void						AllocStaticDominantTris( int numVerts );
	void						AllocStaticSilEdges( int numSilEdges );
	void						AllocStaticMirroredVerts( int numMirroredVerts );
	void						AllocStaticDupVerts( int numDupVerts );

	void						ResizeStaticVerts( int numVerts );
	void						ResizeStaticIndexes( int numIndexes );
	void						ReferenceStaticVerts( const idTriangles* reference );
	void						ReferenceStaticIndexes( const idTriangles* reference );

	void						FreeStaticSilIndexes();
	void						FreeStaticVerts();
	void						FreeStaticVertexCaches();

	void						RemoveDuplicatedTriangles();
	void						CreateSilIndexes();
	void						RemoveDegenerateTriangles();
	void						RemoveUnusedVerts();
	void						RangeCheckIndexes() const;
	void						CreateVertexNormals();		// also called by dmap
	void						CleanupTriangles( bool createNormals, bool identifySilEdges, bool useUnsmoothedTangents );
	// This should be called before CleanupTriangles().
	void						ReverseTriangles();

	// If the deformed verts have significant enough texture coordinate changes to reverse the texture
	// polarity of a triangle, the tangents will be incorrect.
	void						DeriveTangents();

	static void					CreateIndexesForQuads( int numVerts, triIndex_t* indexes, int& numIndexes );
	///void						DeriveIndexesForQuads() { CreateIndexesForQuads( this->numVerts, this->indexes, this->numIndexes ); }

	// Derive bounds from vertex scan or indexed vertex scan.
	void						DeriveBounds( const bool indexedScan = false );
	void						ClearBounds() { bounds.Clear(); }

	// Copy data from a front-end idTriangles to a back-end drawSurf_t.
	void						InitDrawSurfFromTriangles( struct drawSurf_t& );

	// For static surfaces, the indexes, ambient, and shadow buffers can be pre-created at load
	// time, rather than being re-created each frame in the frame temporary buffers.
	void						CreateStaticBuffers();

	void						AddCubeFace( const idVec3& v1, const idVec3& v2, const idVec3& v3, const idVec3& v4 );

	//Note: for non skinned geometry
	ID_INLINE float				GetTriangleArea( int tri ) const
	{
		assert( staticModelWithJoints == NULL );
		return idWinding::TriangleArea(
			GetIVertex( tri + 0 ).GetPosition(),
			GetIVertex( tri + 1 ).GetPosition(),
			GetIVertex( tri + 2 ).GetPosition() );
	};

	// Generate vertexes and indexes for a polytope, and optionally returns the polygon windings.
	// The positive sides of the planes will be visible.
	static idTriangles *		CreateTrianglesForPolytope( int numPlanes, const idPlane* planes, idWinding** windings );

	static idTriangles *		MakeZeroOneQuad();
	static idTriangles *		MakeZeroOneCube();
	static idTriangles *		MakeUnitQuad();

	size_t						CPUMemoryUsed() const;
	//size_t						GPUMemoryUsed() const;

	struct localTrace_t {
		float					fraction;
		// only valid if fraction < 1.0
		idVec3					point;
		idVec3					normal;
		int						indexes[ 3 ];
	};
	localTrace_t				LocalTrace( const idVec3& start, const idVec3& end, const float radius ) const;

	// If outputVertexes is not NULL, it will point to a newly allocated set of verts that includes the mirrored ones.
	static struct deformInfo_t * BuildDeformInfo( int numVerts, const idDrawVert* verts, int numIndexes, const int* indexes, bool useUnsmoothedTangents );

	struct cullInfo_t {
		// For each triangle a byte set to 1 if facing the given origin.
		byte* 					facing;

		// For each vertex a byte with the bits [0-5] set if the
		// vertex is at the back side of the corresponding clip plane.
		// If the 'cullBits' pointer equals LIGHT_CULL_ALL_FRONT all
		// vertices are at the front of all the clip planes.
		byte* 					cullBits;

		void Free();

		cullInfo_t() : facing( nullptr ), cullBits( nullptr )
		{
		}

		~cullInfo_t()
		{
			Free();
		}
	};
	void DetermineCullBits( const idPlane localClipPlanes[6], cullInfo_t & ) const;
	void DetermineFacing( const idVec3 & referencedOrigin, cullInfo_t & ) const;
	// The resulting surface will be a subset of the original triangles,
	// it will never clip triangles, but it may cull on a per-triangle basis.
	// clipPlanes in surface space used to calculate the cull bits.
	// If backFaceCull == false then referencedOrigin is ignored.
	idTriangles * CreateTriangleSubsetFromCulling( const idPlane clipPlanes[ 6 ], bool backFaceCull, const idVec3 & referencedOrigin ) const;

	/*
	GetSurfaceArea();
	OptimizeIndexOrder();
	OptimizeVertexOrder( vertRemap );
	TriangleVectors(  tangent, biTangent ); idDrawVert   idVec3
	UpdateVertexBuffer( strippedBytes, strippedVerts, unstrippedBytes )
	UpdateIndexBuffer()
	CreateQuads( quads )
	TurnIntoQuads( quads )
	GetFullSizeDrawVerts();
	CreateStandardTriangles( triIndexes );
	CreateBounds();
	SnapVerts()
	ReadbackVertexBuffer

	idHeapArray<vertexTangents_t>
	idList<skinRemap_t,5>

	DeriveTangents( vertTangents normal tangents hash remap )
	Cleanup( optimize )

	idAutoStandardTriangles {
	standardTris
	freeOnDelete
	operator->
	}
	sourceSurface_t {
	mtr
	mtrChecksum
	renderSurface
	firstVertex
	lastVertex
	}
	vertexTangents_t {
	normal
	tangents
	}
	tangentVert_t {
	polarityUsed
	negativeRemap
	}

	matchVert_t {
	next
	morph
	color
	normal
	tangents
	}

	idDrawVert::CompressWeightsToBytes( weights, compressed );

	*/

	// Get indexed vertex
	ID_INLINE const idDrawVert & GetIVertex( triIndex_t index ) const
	{
		return verts[ indexes[ index ] ];
	}
	ID_INLINE const idDrawVert & GetVertex( int ivert ) const
	{
		return verts[ ivert ];
	}
	ID_INLINE void CopyBounds( const idTriangles *src )
	{
		bounds = src->bounds;
	}
	ID_INLINE const idBounds & GetBounds() const
	{
		return bounds;
	}
	ID_INLINE int GetNumFaces() const
	{
		return( numIndexes / 3 );
	}

	DISALLOW_COPY_AND_ASSIGN( idTriangles );
};

typedef idList<idTriangles*, TAG_IDLIB_LIST_TRIANGLES> idTriList;

#endif /*__TR_TRIANGLES_H__*/