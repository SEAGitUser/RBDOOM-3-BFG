/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2015 Robert Beckebans

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

/*
===============================================================================

	Trace model vs. polygonal model collision detection.

===============================================================================
*/

#include "CollisionModel.h"

#define MIN_NODE_SIZE						64.0f
#define MAX_NODE_POLYGONS					128
#define CM_MAX_POLYGON_EDGES				64
#define CIRCLE_APPROXIMATION_LENGTH			64.0f

#define	MAX_SUBMODELS						2048
#define	TRACE_MODEL_HANDLE					MAX_SUBMODELS

#define VERTEX_HASH_BOXSIZE					(1<<6)	// must be power of 2
#define VERTEX_HASH_SIZE					(VERTEX_HASH_BOXSIZE*VERTEX_HASH_BOXSIZE)
#define EDGE_HASH_SIZE						(1<<14)

#define NODE_BLOCK_SIZE_SMALL				8
#define NODE_BLOCK_SIZE_LARGE				256
#define REFERENCE_BLOCK_SIZE_SMALL			8
#define REFERENCE_BLOCK_SIZE_LARGE			256

#define MAX_WINDING_LIST					128		// quite a few are generated at times
#define INTEGRAL_EPSILON					0.01f
#define VERTEX_EPSILON						0.1f
#define CHOP_EPSILON						0.1f

struct cm_windingList_t
{
	int					numWindings;			// number of windings
	idFixedWinding		w[MAX_WINDING_LIST];	// windings
	idVec3				normal;					// normal for all windings
	idBounds			bounds;					// bounds of all windings in list
	idVec3				origin;					// origin for radius
	float				radius;					// radius relative to origin for all windings
	int					contents;				// winding surface contents
	int					primitiveNum;			// number of primitive the windings came from
};

/*
===============================================================================

Collision model

===============================================================================
*/

struct cm_vertex_t
{
	idVec3					p;					// vertex point
	int						checkcount;			// for multi-check avoidance
	// DG: use int instead of long for 64bit compatibility
	unsigned int			side;				// each bit tells at which side this vertex passes one of the trace model edges
	unsigned int			sideSet;			// each bit tells if sidedness for the trace model edge has been calculated yet
	// DG end
};

struct cm_edge_t
{
	int						checkcount;			// for multi-check avoidance
	unsigned short			internal;			// a trace model can never collide with internal edges
	unsigned short			numUsers;			// number of polygons using this edge
	// DG: use int instead of long for 64bit compatibility
	unsigned int			side;				// each bit tells at which side of this edge one of the trace model vertices passes
	unsigned int			sideSet;			// each bit tells if sidedness for the trace model vertex has been calculated yet
	// DG end
	int						vertexNum[2];		// start and end point of edge
	idVec3					normal;				// edge normal
};

struct cm_polygonBlock_t
{
	int						bytesRemaining;
	byte* 					next;
};

struct cm_polygon_t
{
	idBounds				bounds;				// polygon bounds
	int						checkcount;			// for multi-check avoidance
	int						contents;			// contents behind polygon
	const idMaterial* 		material;			// material
	idPlane					plane;				// polygon plane
	int						numEdges;			// number of edges
	int						edges[1];			// variable sized, indexes into cm_edge_t list
};

struct cm_polygonRef_t
{
	cm_polygon_t * 			p;					// pointer to polygon
	cm_polygonRef_t *		next;				// next polygon in chain
};

struct cm_polygonRefBlock_t
{
	cm_polygonRef_t * 		nextRef;			// next polygon reference in block
	cm_polygonRefBlock_t *	next;			// next block with polygon references
};

struct cm_brushBlock_t
{
	int						bytesRemaining;
	byte* 					next;
};

struct cm_brush_t
{
	cm_brush_t()
	{
		checkcount = 0;
		contents = 0;
		material = NULL;
		primitiveNum = 0;
		numPlanes = 0;
	}
	int						checkcount;			// for multi-check avoidance
	idBounds				bounds;				// brush bounds
	int						contents;			// contents of brush
	const idMaterial* 		material;			// material
	int						primitiveNum;		// number of brush primitive
	int						numPlanes;			// number of bounding planes
	idPlane					planes[1];			// variable sized
};

struct cm_brushRef_t
{
	cm_brush_t * 			b;					// pointer to brush
	cm_brushRef_t * 		next;				// next brush in chain
};

struct cm_brushRefBlock_t
{
	cm_brushRef_t * 		nextRef;			// next brush reference in block
	cm_brushRefBlock_t *	next;			// next block with brush references
};

struct cm_node_t
{
	int						planeType;			// node axial plane type
	float					planeDist;			// node plane distance
	cm_polygonRef_t * 		polygons;			// polygons in node
	cm_brushRef_t * 		brushes;			// brushes in node
	cm_node_t * 			parent;				// parent of this node
	cm_node_t * 			children[2];		// node children
};

struct cm_nodeBlock_t
{
	cm_node_t * 			nextNode;			// next node in block
	cm_nodeBlock_t *		next;				// next block with nodes
};

struct cm_model_t
{
	idStr					name;				// model name
	idBounds				bounds;				// model bounds
	int						contents;			// all contents of the model ored together
	bool					isConvex;			// set if model is convex
	// model geometry
	int						maxVertices;		// size of vertex array
	int						numVertices;		// number of vertices
	cm_vertex_t * 			vertices;			// array with all vertices used by the model
	int						maxEdges;			// size of edge array
	int						numEdges;			// number of edges
	cm_edge_t * 			edges;				// array with all edges used by the model
	cm_node_t * 			node;				// first node of spatial subdivision
	// blocks with allocated memory
	cm_nodeBlock_t * 		nodeBlocks;			// list with blocks of nodes
	cm_polygonRefBlock_t * 	polygonRefBlocks;	// list with blocks of polygon references
	cm_brushRefBlock_t * 	brushRefBlocks;		// list with blocks of brush references
	cm_polygonBlock_t * 	polygonBlock;		// memory block with all polygons
	cm_brushBlock_t * 		brushBlock;			// memory block with all brushes
	// statistics
	int						numPolygons;
	int						polygonMemory;
	int						numBrushes;
	int						brushMemory;
	int						numNodes;
	int						numBrushRefs;
	int						numPolygonRefs;
	int						numInternalEdges;
	int						numSharpEdges;
	int						numRemovedPolys;
	int						numMergedPolys;
	int						usedMemory;
};

#if 0
class idCollisionModelLocal : public idCollisionModel {
private:
	idStr					name;				// model name
	idBounds				bounds;				// model bounds
	int						contents;			// all contents of the model ored together
	bool					isConvex;			// set if model is convex
	bool					isTraceModel;		// set if model is idTraceModel

	// model geometry
	int						maxVertices;		// size of vertex array
	int						numVertices;		// number of vertices
	cm_vertex_t * 			vertices;			// array with all vertices used by the model
	int						maxEdges;			// size of edge array
	int						numEdges;			// number of edges
	cm_edge_t * 			edges;				// array with all edges used by the model
	cm_node_t * 			node;				// first node of spatial subdivision

	// blocks with allocated memory
	cm_nodeBlock_t * 		nodeBlocks;			// list with blocks of nodes
	cm_polygonRefBlock_t * 	polygonRefBlocks;	// list with blocks of polygon references
	cm_brushRefBlock_t * 	brushRefBlocks;		// list with blocks of brush references
	cm_polygonBlock_t * 	polygonBlock;		// memory block with all polygons
	cm_brushBlock_t * 		brushBlock;			// memory block with all brushes

	// statistics
	uint32					numPolygons;
	uint32					polygonMemory;
	uint32					numBrushes;
	uint32					brushMemory;
	uint32					numNodes;
	uint32					numBrushRefs;
	uint32					numPolygonRefs;
	uint32					numInternalEdges;
	uint32					numSharpEdges;
	uint32					numRemovedPolys;
	uint32					numMergedPolys;
	uint32					usedMemory;

public:
	virtual const char *		GetName() const { return name.c_str(); }
	virtual const idBounds &	GetBounds() const { return bounds; }
	//virtual void				GetBounds( idBounds& bounds, int surfaceMask, bool inclusive ) const { bounds = this->bounds; };
	virtual int					GetContents() const { return contents; }
	virtual const idVec3 &		GetVertex( int vertexNum ) const { return vertices[ vertexNum ].p; }
	virtual void				GetEdge( int edgeNum, idVec3& start, idVec3& end ) const;
	virtual void				GetPolygon( int polygonNum, idFixedWinding &winding ) const;

	virtual void				Draw( int surfaceMask, bool inclusive ) const;

	virtual int					GetNumBrushPlanes() const;
	virtual const idPlane &		GetBrushPlane( int planeNum ) const;

	virtual const idMaterial *	GetPolygonMaterial( int polygonNum ) const { auto poly = *reinterpret_cast< cm_polygon_t** >( &polygonNum ); return poly->material; }
	virtual const idPlane &		GetPolygonPlane( int polygonNum ) const { auto poly = *reinterpret_cast< cm_polygon_t** >( &polygonNum ); return poly->plane; }
	virtual int					GetNumPolygons() const { return numPolygons; }

	virtual bool				IsTraceModel() const { return isTraceModel; }
	virtual bool				IsConvex() const { return isConvex; }

	virtual bool				IsWorld() const;
	virtual void				SetWorld( bool tf );

private:
	cm_node_t * 		AllocNode( int blockSize );
	cm_polygonRef_t *	AllocPolygonReference( int blockSize );
	cm_brushRef_t * 	AllocBrushReference( int blockSize );
	cm_polygon_t * 		AllocPolygon( int numEdges );
	cm_brush_t * 		AllocBrush( int numPlanes );
	void				AddPolygonToNode( cm_node_t* node, cm_polygon_t* p );
	void				AddBrushToNode( cm_node_t* node, cm_brush_t* b );
};
void idCollisionModelLocal::GetEdge( int edgeNum, idVec3& start, idVec3& end ) const
{
	edgeNum = idMath::Abs( edgeNum );
	if( edgeNum >= numEdges ) {
		idLib::Printf( "idCollisionModelLocal::GetEdge: invalid edge number\n" );
		return;
	}

	start = vertices[ edges[ edgeNum ].vertexNum[ 0 ] ].p;
	end = vertices[ edges[ edgeNum ].vertexNum[ 1 ] ].p;
}
void idCollisionModelLocal::GetPolygon( int polygonNum, idFixedWinding& winding ) const
{
	auto poly = *reinterpret_cast< cm_polygon_t** >( &polygonNum ); //SEA: ptr size ?
	winding.Clear();
	for( int i = 0; i < poly->numEdges; i++ )
	{
		int edgeNum = poly->edges[ i ];
		winding += vertices[ edges[ idMath::Abs( edgeNum ) ].vertexNum[ INT32_SIGNBITSET( edgeNum ) ] ].p;
	}
}
#endif

/*
===============================================================================

	Data used during collision detection calculations

===============================================================================
*/

struct cm_trmVertex_t
{
	int used;										// true if this vertex is used for collision detection
	idVec3 p;										// vertex position
	idVec3 endp;									// end point of vertex after movement
	int polygonSide;								// side of polygon this vertex is on (rotational collision)
	idPluecker pl;									// pluecker coordinate for vertex movement
	idVec3 rotationOrigin;							// rotation origin for this vertex
	idBounds rotationBounds;						// rotation bounds for this vertex
};

struct cm_trmEdge_t
{
	int used;										// true when vertex is used for collision detection
	idVec3 start;									// start of edge
	idVec3 end;										// end of edge
	int vertexNum[2];								// indexes into cm_traceWork_t->vertices
	idPluecker pl;									// pluecker coordinate for edge
	idVec3 cross;									// (z,-y,x) of cross product between edge dir and movement dir
	idBounds rotationBounds;						// rotation bounds for this edge
	idPluecker plzaxis;								// pluecker coordinate for rotation about the z-axis
	unsigned short bitNum;							// vertex bit number
};

struct cm_trmPolygon_t
{
	int used;
	idPlane plane;									// polygon plane
	int numEdges;									// number of edges
	int edges[MAX_TRACEMODEL_POLYEDGES];			// index into cm_traceWork_t->edges
	idBounds rotationBounds;						// rotation bounds for this polygon
};

struct cm_traceWork_t
{
	int numVerts;
	cm_trmVertex_t vertices[MAX_TRACEMODEL_VERTS];	// trm vertices
	int numEdges;
	cm_trmEdge_t edges[MAX_TRACEMODEL_EDGES + 1];		// trm edges
	int numPolys;
	cm_trmPolygon_t polys[MAX_TRACEMODEL_POLYS];	// trm polygons
	cm_model_t* model;								// model colliding with
	idVec3 start;									// start of trace
	idVec3 end;										// end of trace
	idVec3 dir;										// trace direction
	idBounds bounds;								// bounds of full trace
	idBounds size;									// bounds of transformed trm relative to start
	idVec3 extents;									// largest of abs(size[0]) and abs(size[1]) for BSP trace
	int contents;									// ignore polygons that do not have any of these contents flags
	trace_t trace;									// collision detection result

	bool rotation;									// true if calculating rotational collision
	bool pointTrace;								// true if only tracing a point
	bool positionTest;								// true if not tracing but doing a position test
	bool isConvex;									// true if the trace model is convex
	bool axisIntersectsTrm;							// true if the rotation axis intersects the trace model
	bool getContacts;								// true if retrieving contacts
	bool quickExit;									// set to quickly stop the collision detection calculations

	idVec3 origin;									// origin of rotation in model space
	idVec3 axis;									// rotation axis in model space
	idMat3 matrix;									// rotates axis of rotation to the z-axis
	float angle;									// angle for rotational collision
	float maxTan;									// max tangent of half the positive angle used instead of fraction
	float radius;									// rotation radius of trm start
	idRotation modelVertexRotation;					// inverse rotation for model vertices

	contactInfo_t* contacts;						// array with contacts
	int maxContacts;								// max size of contact array
	int numContacts;								// number of contacts found

	idPlane heartPlane1;							// polygons should be near anough the trace heart planes
	float maxDistFromHeartPlane1;
	idPlane heartPlane2;
	float maxDistFromHeartPlane2;
	idPluecker polygonEdgePlueckerCache[CM_MAX_POLYGON_EDGES];
	idPluecker polygonVertexPlueckerCache[CM_MAX_POLYGON_EDGES];
	idVec3 polygonRotationOriginCache[CM_MAX_POLYGON_EDGES];
};

/*
===============================================================================

Collision Map

===============================================================================
*/

struct cm_procNode_t
{
	idPlane plane;
	int children[2];	// negative numbers are (-1 - areaNumber), 0 = solid
};

class idCollisionModelManagerLocal : public idCollisionModelManager {
public:
	// load collision models from a map file
	void			LoadMap( const idMapFile* mapFile );
	// frees all the collision models
	void			FreeMap();

	void			Preload( const char* mapName );
	// get clip handle for model
	cmHandle_t		LoadModel( const char* modelName );
	// sets up a trace model for collision with other trace models
	cmHandle_t		SetupTrmModel( const idTraceModel& trm, const idMaterial* material );
	// create trace model from a collision model, returns true if succesfull
	bool			TrmFromModel( const char* modelName, idTraceModel& trm );

	// name of the model
	const char* 	GetModelName( cmHandle_t model ) const;
	// bounds of the model
	bool			GetModelBounds( cmHandle_t model, idBounds& bounds ) const;
	// all contents flags of brushes and polygons ored together
	bool			GetModelContents( cmHandle_t model, int& contents ) const;
	// get the vertex of a model
	bool			GetModelVertex( cmHandle_t model, int vertexNum, idVec3& vertex ) const;
	// get the edge of a model
	bool			GetModelEdge( cmHandle_t model, int edgeNum, idVec3& start, idVec3& end ) const;
	// get the polygon of a model
	bool			GetModelPolygon( cmHandle_t model, int polygonNum, idFixedWinding& winding ) const;

	// translates a trm and reports the first collision if any
	void			Translation( trace_t* results, const idVec3& start, const idVec3& end,
								 const idTraceModel* trm, const idMat3& trmAxis, int contentMask,
								 cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis );
	// rotates a trm and reports the first collision if any
	void			Rotation( trace_t* results, const idVec3& start, const idRotation& rotation,
							  const idTraceModel* trm, const idMat3& trmAxis, int contentMask,
							  cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis );
	// returns the contents the trm is stuck in or 0 if the trm is in free space
	int				Contents( const idVec3& start,
							  const idTraceModel* trm, const idMat3& trmAxis, int contentMask,
							  cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis );
	// stores all contact points of the trm with the model, returns the number of contacts
	int				Contacts( contactInfo_t* contacts, const int maxContacts, const idVec3& start, const idVec6& dir, const float depth,
							  const idTraceModel* trm, const idMat3& trmAxis, int contentMask,
							  cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis );
	// test collision detection
	void			DebugOutput( const idVec3& origin );
	// draw a model
	void			DrawModel( cmHandle_t model, const idVec3& origin, const idMat3& axis, const idVec3& viewOrigin, const float radius );
	// print model information, use -1 handle for accumulated model info
	void			ModelInfo( cmHandle_t model );
	// list all loaded models
	void			ListModels();
	// write a collision model file for the map entity
	bool			WriteCollisionModelForMapEntity( const idMapEntity* mapEnt, const char* filename, const bool testTraceModel = true );

private:			// CollisionMap_translate.cpp
	void			TranslateTrmEdgeThroughPolygon( cm_traceWork_t* tw, cm_polygon_t* poly, cm_trmEdge_t* trmEdge ) const;
	void			TranslateTrmVertexThroughPolygon( cm_traceWork_t* tw, cm_polygon_t* poly, cm_trmVertex_t* v, int bitNum ) const;
	void			TranslatePointThroughPolygon( cm_traceWork_t* tw, cm_polygon_t* poly, cm_trmVertex_t* v ) const;
	void			TranslateVertexThroughTrmPolygon( cm_traceWork_t* tw, cm_trmPolygon_t* trmpoly, cm_polygon_t* poly, cm_vertex_t* v, idVec3& endp, idPluecker& pl ) const;
	bool			TranslateTrmThroughPolygon( cm_traceWork_t* tw, cm_polygon_t* p ) const;
	void			SetupTrm( cm_traceWork_t* tw, const idTraceModel* trm ) const;

private:			// CollisionMap_rotate.cpp
	int				CollisionBetweenEdgeBounds( cm_traceWork_t* tw, const idVec3& va, const idVec3& vb,
												const idVec3& vc, const idVec3& vd, float tanHalfAngle,
												idVec3& collisionPoint, idVec3& collisionNormal ) const;
	int				RotateEdgeThroughEdge( cm_traceWork_t* tw, const idPluecker& pl1,
										   const idVec3& vc, const idVec3& vd,
										   const float minTan, float& tanHalfAngle ) const;
	int				EdgeFurthestFromEdge( cm_traceWork_t* tw, const idPluecker& pl1,
										  const idVec3& vc, const idVec3& vd,
										  float& tanHalfAngle, float& dir ) const;
	void			RotateTrmEdgeThroughPolygon( cm_traceWork_t* tw, cm_polygon_t* poly, cm_trmEdge_t* trmEdge ) const;
	int				RotatePointThroughPlane( const cm_traceWork_t* tw, const idVec3& point, const idPlane& plane,
											 const float angle, const float minTan, float& tanHalfAngle ) const;
	int				PointFurthestFromPlane( const cm_traceWork_t* tw, const idVec3& point, const idPlane& plane,
											const float angle, float& tanHalfAngle, float& dir ) const;
	int				RotatePointThroughEpsilonPlane( const cm_traceWork_t* tw, const idVec3& point, const idVec3& endPoint,
													const idPlane& plane, const float angle, const idVec3& origin,
													float& tanHalfAngle, idVec3& collisionPoint, idVec3& endDir ) const;
	void			RotateTrmVertexThroughPolygon( cm_traceWork_t* tw, cm_polygon_t* poly, cm_trmVertex_t* v, int vertexNum ) const;
	void			RotateVertexThroughTrmPolygon( cm_traceWork_t* tw, cm_trmPolygon_t* trmpoly, cm_polygon_t* poly, cm_vertex_t* v, idVec3& rotationOrigin ) const;
	bool			RotateTrmThroughPolygon( cm_traceWork_t* tw, cm_polygon_t* p ) const;
	void			BoundsForRotation( const idVec3& origin, const idVec3& axis, const idVec3& start, const idVec3& end, idBounds& bounds ) const;
	void			Rotation180( trace_t* results, const idVec3& rorg, const idVec3& axis,
								 const float startAngle, const float endAngle, const idVec3& start,
								 const idTraceModel* trm, const idMat3& trmAxis, int contentMask,
								 cmHandle_t model, const idVec3& origin, const idMat3& modelAxis );

private:			// CollisionMap_contents.cpp
	bool			TestTrmVertsInBrush( cm_traceWork_t* tw, cm_brush_t* b ) const;
	bool			TestTrmInPolygon( cm_traceWork_t* tw, cm_polygon_t* p ) const;
	cm_node_t * 	PointNode( const idVec3& p, cm_model_t* model ) const;
	int				PointContents( const idVec3 p, cmHandle_t model ) const;
	int				TransformedPointContents( const idVec3& p, cmHandle_t model, const idVec3& origin, const idMat3& modelAxis ) const;
	int				ContentsTrm( trace_t* results, const idVec3& start,
								 const idTraceModel* trm, const idMat3& trmAxis, int contentMask,
								 cmHandle_t model, const idVec3& modelOrigin, const idMat3& modelAxis );

private:			// CollisionMap_trace.cpp
	void			TraceTrmThroughNode( cm_traceWork_t* tw, cm_node_t* node ) const;
	void			TraceThroughAxialBSPTree_r( cm_traceWork_t* tw, cm_node_t* node, float p1f, float p2f, idVec3& p1, idVec3& p2 ) const;
	void			TraceThroughModel( cm_traceWork_t* tw ) const;
	///void			RecurseProcBSP_r( trace_t* results, int parentNodeNum, int nodeNum, float p1f, float p2f, const idVec3& p1, const idVec3& p2 ) const;

private:			// CollisionMap_load.cpp
	void			Clear();
	void			FreeTrmModelStructure();
	// model deallocation
	void			RemovePolygonReferences_r( cm_node_t* node, cm_polygon_t* p ) const;
	void			RemoveBrushReferences_r( cm_node_t* node, cm_brush_t* b ) const;
	void			FreeNode( cm_node_t* node ) const;
	void			FreePolygonReference( cm_polygonRef_t* pref ) const;
	void			FreeBrushReference( cm_brushRef_t* bref ) const;
	void			FreePolygon( cm_model_t* model, cm_polygon_t* poly ) const;
	void			FreeBrush( cm_model_t* model, cm_brush_t* brush ) const;
	void			FreeTree_r( cm_model_t* model, cm_node_t* headNode, cm_node_t* node ) const;
	void			FreeModel( cm_model_t* model ) const;
	// merging polygons
	void			ReplacePolygons( cm_model_t* model, cm_node_t* node, cm_polygon_t* p1, cm_polygon_t* p2, cm_polygon_t* newp ) const;
	cm_polygon_t* 	TryMergePolygons( cm_model_t* model, cm_polygon_t* p1, cm_polygon_t* p2 );
	bool			MergePolygonWithTreePolygons( cm_model_t* model, cm_node_t* node, cm_polygon_t* polygon );
	void			MergeTreePolygons( cm_model_t* model, cm_node_t* node );
	// finding internal edges
	bool			PointInsidePolygon( cm_model_t* model, cm_polygon_t* p, idVec3& v );
	void			FindInternalEdgesOnPolygon( cm_model_t* model, cm_polygon_t* p1, cm_polygon_t* p2 );
	void			FindInternalPolygonEdges( cm_model_t* model, cm_node_t* node, cm_polygon_t* polygon );
	void			FindInternalEdges( cm_model_t* model, cm_node_t* node );
	void			FindContainedEdges( cm_model_t* model, cm_polygon_t* p );
	// loading of proc BSP tree
	void			ParseProcNodes( idLexer& src );
	void			LoadProcBSP( const char* name );
	// removal of contained polygons
	int				R_ChoppedAwayByProcBSP( int nodeNum, idFixedWinding* w, const idVec3& normal, const idVec3& origin, const float radius );
	int				ChoppedAwayByProcBSP( const idFixedWinding& w, const idPlane& plane, int contents );
	void			ChopWindingListWithBrush( cm_windingList_t* list, cm_brush_t* b );
	void			R_ChopWindingListWithTreeBrushes( cm_windingList_t* list, cm_node_t* node );
	idFixedWinding* WindingOutsideBrushes( idFixedWinding* w, const idPlane& plane, int contents, int patch, cm_node_t* headNode );
	// creation of axial BSP tree
	cm_model_t* 	AllocModel() const;
	cm_node_t* 		AllocNode( cm_model_t* model, int blockSize ) const;
	cm_polygonRef_t* AllocPolygonReference( cm_model_t* model, int blockSize ) const;
	cm_brushRef_t* 	AllocBrushReference( cm_model_t* model, int blockSize ) const;
	cm_polygon_t* 	AllocPolygon( cm_model_t* model, int numEdges ) const;
	cm_brush_t* 	AllocBrush( cm_model_t* model, int numPlanes ) const;
	void			AddPolygonToNode( cm_model_t* model, cm_node_t* node, cm_polygon_t* p ) const;
	void			AddBrushToNode( cm_model_t* model, cm_node_t* node, cm_brush_t* b ) const;
	void			SetupTrmModelStructure();
	void			R_FilterPolygonIntoTree( cm_model_t* model, cm_node_t* node, cm_polygonRef_t* pref, cm_polygon_t* p ) const;
	void			R_FilterBrushIntoTree( cm_model_t* model, cm_node_t* node, cm_brushRef_t* pref, cm_brush_t* b ) const;
	cm_node_t* 		R_CreateAxialBSPTree( cm_model_t* model, cm_node_t* node, const idBounds& bounds ) const;
	cm_node_t* 		CreateAxialBSPTree( cm_model_t* model, cm_node_t* node ) const;
	// creation of raw polygons
	void			SetupHash();
	void			ShutdownHash();
	void			ClearHash( idBounds& bounds );
	int				HashVec( const idVec3& vec );
	int				GetVertex( cm_model_t* model, const idVec3& v, int* vertexNum );
	int				GetEdge( cm_model_t* model, const idVec3& v1, const idVec3& v2, int* edgeNum, int v1num );
	void			CreatePolygon( cm_model_t* model, idFixedWinding* w, const idPlane& plane, const idMaterial* material, int primitiveNum );
	void			PolygonFromWinding( cm_model_t* model, idFixedWinding* w, const idPlane& plane, const idMaterial* material, int primitiveNum );
	void			CalculateEdgeNormals( cm_model_t* model, cm_node_t* node );
	void			CreatePatchPolygons( cm_model_t* model, idSurface_Patch& mesh, const idMaterial* material, int primitiveNum );
	void			ConvertPatch( cm_model_t* model, const idMapPatch* patch, int primitiveNum );
	void			ConvertBrushSides( cm_model_t* model, const idMapBrush* mapBrush, int primitiveNum );
	void			ConvertBrush( cm_model_t* model, const idMapBrush* mapBrush, int primitiveNum );
	// RB: support new .map format
	void			ConvertMesh( cm_model_t* model, const MapPolygonMesh* mesh, int primitiveNum );
	// RB end
	void			PrintModelInfo( const cm_model_t* model );
	void			AccumulateModelInfo( cm_model_t* model );
	void			RemapEdges( cm_node_t* node, int* edgeRemap );
	void			OptimizeArrays( cm_model_t* model );
	void			FinishModel( cm_model_t* model );
	void			BuildModels( const idMapFile* mapFile );
	cmHandle_t		FindModel( const char* name );
	cm_model_t* 	CollisionModelForMapEntity( const idMapEntity* mapEnt );	// brush/patch model from .map
	cm_model_t* 	LoadRenderModel( const char* fileName );					// ASE/LWO models
	cm_model_t* 	LoadBinaryModel( const char* fileName, ID_TIME_T sourceTimeStamp );
	cm_model_t* 	LoadBinaryModelFromFile( idFile* fileIn, ID_TIME_T sourceTimeStamp );
	void			WriteBinaryModel( cm_model_t* model, const char* fileName, ID_TIME_T sourceTimeStamp );
	void			WriteBinaryModelToFile( cm_model_t* model, idFile* fileOut, ID_TIME_T sourceTimeStamp );
	bool			TrmFromModel_r( idTraceModel& trm, cm_node_t* node );
	bool			TrmFromModel( const cm_model_t* model, idTraceModel& trm );

private:			// CollisionMap_files.cpp
	// writing
	void			WriteNodes( idFile* fp, cm_node_t* node );
	int				CountPolygonMemory( cm_node_t* node ) const;
	void			WritePolygons( idFile* fp, cm_node_t* node );
	int				CountBrushMemory( cm_node_t* node ) const;
	void			WriteBrushes( idFile* fp, cm_node_t* node );
	void			WriteCollisionModel( idFile* fp, cm_model_t* model );
	void			WriteCollisionModelsToFile( const char* filename, int firstModel, int lastModel, unsigned int mapFileCRC );
	// loading
	cm_node_t* 		ParseNodes( idLexer& src, cm_model_t* model, cm_node_t* parent );
	void			ParseVertices( idLexer& src, cm_model_t* model );
	void			ParseEdges( idLexer& src, cm_model_t* model );
	void			ParsePolygons( idLexer& src, cm_model_t* model );
	void			ParseBrushes( idLexer& src, cm_model_t* model );
	cm_model_t* 	ParseCollisionModel( idLexer& src );
	bool			LoadCollisionModelFile( const char* name, unsigned int mapFileCRC );

private:			// CollisionMap_debug
	int				ContentsFromString( const char* string ) const;
	const char* 	StringFromContents( const int contents ) const;
	void			DrawEdge( cm_model_t* model, int edgeNum, const idVec3& origin, const idMat3& axis );
	void			DrawPolygon( cm_model_t* model, cm_polygon_t* p, const idVec3& origin, const idMat3& axis, const idVec3& viewOrigin );
	void			DrawNodePolygons( cm_model_t* model, cm_node_t* node, const idVec3& origin, const idMat3& axis, const idVec3& viewOrigin, const float radius );

private: // collision map data

	idStr				mapName;
	ID_TIME_T			mapFileTime;
	int					loaded;
	// for multi-check avoidance
	int					checkCount;
	// models
	int					maxModels;
	int					numModels;
	cm_model_t ** 		models;
	// polygons and brush for trm model
	cm_polygonRef_t *	trmPolygons[MAX_TRACEMODEL_POLYS];
	cm_brushRef_t * 	trmBrushes[1];
	const idMaterial *	trmMaterial;
	// for data pruning
	int					numProcNodes;
	cm_procNode_t * 	procNodes;
	// for retrieving contact points
	bool				getContacts;
	contactInfo_t * 	contacts;
	int					maxContacts;
	int					numContacts;
};

// for debugging
extern idCVar cm_debugCollision;
