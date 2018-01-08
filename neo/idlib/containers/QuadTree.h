// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __QUADTREE_H__
#define __QUADTREE_H__

/*
==============================================================================

	idQuadTree

		Direct Access QuadTree Lookup template

		Children are orientated in the following order:

			+---+---+
			| 2 | 3 |
			+---+---+
			| 0 | 1 |
			+---+---+
		(0,0)

		Neighbors are orientated in the following order:

			 3
			1+2
			 0

==============================================================================
*/

template< class type >
class idQuadTree {
public:
	typedef struct nodePosition_s {
		int		level;
		int		x;
		int		y;
	} nodePosition_t;

	typedef type dataType;

	class idQuadNode {
	public:
								idQuadNode( void ) {
									parent = NULL;
									memset( children, 0, sizeof( children ) );
									memset( neighbors, 0, sizeof( neighbors ) );
									data = NULL;
									bounds.Clear();
								}
								explicit idQuadNode( const idBounds &bounds ) {
									parent = NULL;
									memset( children, 0, sizeof( children ) );
									memset( neighbors, 0, sizeof( neighbors ) );
									data = NULL;
									this->bounds = bounds;
								}
								explicit idQuadNode( type *data, const idBounds &bounds ) {
									assert( data != NULL );
									parent = NULL;
									memset( children, 0, sizeof( children ) );
									memset( neighbors, 0, sizeof( neighbors ) );
									this->data = data;
									this->bounds = bounds;
								}
		virtual					~idQuadNode( void ) {
									assert( data == NULL );
								}

		void					SetData( type *data ) {
									this->data = data;
								}
		type *					GetData( void ) const {
									return data;
								}

		void					SetParent( idQuadNode &parent ) {
									this->parent = &parent;
								}
		void					SetChild( idQuadNode &child, const int index ) {
									assert( index >= 0 && index < 4 );
									children[ index ] = &child;
								}
		void					SetNeighbor( idQuadNode &neighbor, const int index ) {
									assert( index >= 0 && index < 4 );
									neighbors[ index ] = &neighbor;
								}
		void					SetBounds( const idBounds &bounds ) {
									this->bounds = bounds;
								}

		idQuadNode *			GetParent( void ) const {
									return parent;
								}
		idQuadNode *			GetChild( const int index ) const {
									assert( index >= 0 && index < 4 );
									return children[ index ];
								}
		idQuadNode *			GetNeighbor( const int index ) const {
									assert( index >= 0 && index < 4 );
									return neighbors[ index ];
								}
		idBounds &				GetBounds( void ) {
									return bounds;
								}

		void					SetNodePosition( const int nodeLevel, const int nodeX, const int nodeY ) {
									nodePosition.level = nodeLevel;
									nodePosition.x = nodeX;
									nodePosition.y = nodeY;
								}
		const nodePosition_t &	GetNodePosition( void ) const {
									return nodePosition;
								}

		bool					HasChildren( void ) {
									return( children[0] || children[1] || children[2] || children[3] );
								}
		void					ClearChildren( void ) {
									memset( children, 0, sizeof( children ) );
								}

	private:
		idQuadNode *	parent;
		idQuadNode *	children[4];
		idQuadNode *	neighbors[4];
		type *			data;

		idBounds		bounds;

		// keep track of the position in the tree
		nodePosition_t	nodePosition;
	};

						explicit idQuadTree( const idBounds &bounds, const int depth = 6 );
	virtual				~idQuadTree( void );

	void				BuildQuadTree( void );
	void				BuildQuadTree( idQuadNode &node );

	const int			GetDepth( void ) const { return depth; }
	const int			GetUsedDepth( void ) const;

	const idQuadNode *	GetHeadNode( void ) const { return headNode; }
	idQuadNode *		GetHeadNode( void ) { return headNode; }

	idQuadNode *		FindNode( const idVec3 &point );

	idQuadNode *		GetNode( const idBounds &bounds );
	idQuadNode *		GetNode( const nodePosition_t &nodePosition );
	idQuadNode **		GetNodes( const int nodeLevel, int &numNodes ) {
							assert( nodeLevel >=0 && nodeLevel < depth );
							numNodes = idMath::Pow( 2, nodeLevel * 2 );
							return nodes[ nodeLevel ];
						}

	const int			GetNumLeafNodes( void ) const;

	void				CreateChildren( idQuadNode &parent );
	void				FreeChildren( idQuadNode &parent ) {
							int i;

							for ( i = 0; i < 4; i++ ) {
								if ( parent.GetChild(i) ) {
									FreeNode( *parent.GetChild(i) );
								}
							}

							parent.ClearChildren();
						}

private:
	void				GetUsedDepth_r( idQuadNode &node, const int currentDepth, int *maxReachedDepth ) const;
	void				GetNumLeafNodes_r( idQuadNode &node, int *numLeafNodes ) const;
	idQuadNode *		AllocNode( idQuadNode **node, int nodeLevel, int x, int y );
	void				FindChildren_r( idQuadNode &parent, const int nodeLevel );
	void				FindChildren_r( idQuadNode &parent, const int nodeLevel, const int parentX, const int parentY );
	void				FindNeighbors_r( const int nodeLevel );

	void				FreeNode( idQuadNode &node ) {
							nodes[ node.GetNodePosition().level ][ ( node.GetNodePosition().y << ( node.GetNodePosition().level ) ) + node.GetNodePosition().x ] = NULL;
							delete &node;
						}

private:
	idQuadNode *		headNode;
	idQuadNode ***		nodes;

	int					depth;
	idBounds			bounds;
	idVec2				nodeScale;
};

/*
================
idQuadTree<type>::idQuadTree( const int depth )
================
*/
template< class type >
ID_INLINE idQuadTree<type>::idQuadTree( const idBounds &bounds, const int depth )
{
	assert( depth > 0 );

	this->depth = depth;
	this->bounds = bounds;

	// expand by 1 unit so everything fits completely in it
	this->bounds.ExpandSelf( 1.f );

	nodeScale.x = idMath::Pow( 2, depth - 1 ) / ( this->bounds[ 1 ].x - this->bounds[ 0 ].x );
	nodeScale.y = idMath::Pow( 2, depth - 1 ) / ( this->bounds[ 1 ].y - this->bounds[ 0 ].y );

	nodes = new idQuadNode**[ depth ];

	for( int i = 0; i < depth; i++ )
	{
		int nCells = idMath::Pow( 2, i * 2 );

		nodes[ i ] = new idQuadNode*[ nCells ];
		memset( nodes[ i ], 0, nCells * sizeof( idQuadNode* ) );
	}

	// create head node
	headNode = new idQuadNode;
	headNode->SetBounds( bounds );

	// put in node array
	nodes[ 0 ][ 0 ] = headNode;
}

/*
================
idQuadTree<type>::~idQuadTree
================
*/
template< class type >
ID_INLINE idQuadTree<type>::~idQuadTree( void )
{
	if( nodes )
	{
		for( int i = 0; i < depth; i++ )
		{
			int nCells = static_cast< int >( idMath::Pow( 2.f, i * 2.f ) );

			for( int j = 0; j < nCells; j++ )
			{
				if( nodes[ i ][ j ] )
				{
					delete nodes[ i ][ j ];
				}
			}
			delete[] nodes[ i ];
		}
		delete[] nodes;
	}
}

/*
================
idQuadTree<type>::GetUsedDepth_r
================
*/
template< class type >
void idQuadTree<type>::GetUsedDepth_r( idQuadNode &node, const int currentDepth, int *maxReachedDepth ) const
{
	int			i;
	idQuadNode*	child;

	if( currentDepth > *maxReachedDepth )
	{
		*maxReachedDepth = currentDepth;
	}

	for( i = 0; i < 4; i++ )
	{
		child = node.GetChild( i );
		if( child )
		{
			if( currentDepth + 1 < depth )
			{
				GetUsedDepth_r( *child, currentDepth + 1, maxReachedDepth );
			}
		}
	}
}

/*
================
idQuadTree<type>::GetUsedDepth
================
*/
template< class type >
ID_INLINE const int idQuadTree<type>::GetUsedDepth( void ) const
{
	int		maxReachedDepth = 0;

	GetUsedDepth_r( *headNode, 1, &maxReachedDepth );

	return maxReachedDepth + 1;
}

/*
================
idQuadTree<type>::BuildQuadTree
================
*/
template< class type >
ID_INLINE void idQuadTree<type>::BuildQuadTree( void )
{
#if 1
	FindChildren_r( *headNode, 1, 0, 0 );
#else
	FindChildren_r( *headNode, 1 );
#endif
	FindNeighbors_r( 1 );
}

/*
================
idQuadTree<type>::BuildQuadTree
================
*/
template< class type >
ID_INLINE void idQuadTree<type>::BuildQuadTree( typename idQuadTree<type>::idQuadNode &node )
{
	// TODO
}

/*
================
idQuadTree<type>::FindNode
================
*/
template< class type >
ID_INLINE typename idQuadTree<type>::idQuadNode * idQuadTree<type>::FindNode( const idVec3 &point )
{
	int			nodeDepth, x, y;
	idQuadNode	*node;

	if( !bounds.ContainsPoint( point ) )
	{
		return NULL;
	}

	x = ( int )( ( point.x - bounds[ 0 ].x ) * nodeScale.x );
	y = ( int )( ( point.y - bounds[ 0 ].y ) * nodeScale.y );

	for( nodeDepth = depth - 1; nodeDepth >= 0; nodeDepth--, x >>= 1, y >>= 1 )
	{
		node = nodes[ nodeDepth ][ ( y << nodeDepth ) + x ];

		if( node )
		{
			return node;
		}
	}

	// should never happen
	return NULL;
}

/*
================
idQuadTree<type>::GetNode( const idBounds & )
================
*/
template< class type >
ID_INLINE typename idQuadTree<type>::idQuadNode * idQuadTree<type>::GetNode( const idBounds &bounds )
{
	int x = ( int )( ( bounds[ 0 ].x - this->bounds[ 0 ].x ) * nodeScale.x );
	int y = ( int )( ( bounds[ 0 ].y - this->bounds[ 0 ].y ) * nodeScale.y );
	int xR = x ^ ( int )( ( bounds[ 1 ].x - this->bounds[ 0 ].x ) * nodeScale.x );
	int yR = y ^ ( int )( ( bounds[ 1 ].y - this->bounds[ 0 ].y ) * nodeScale.y );

	int nodeDepth = depth;

	// OPTIMIZE: for x86, optimise using BSR ?
	int shifted = 0;

	while( xR + yR != 0 )
	{
		xR >>= 1;
		yR >>= 1;
		nodeDepth--;
		shifted++;
	}

	x >>= shifted;
	y >>= shifted;

	idQuadNode** node = &nodes[ nodeDepth - 1 ][ ( y << ( nodeDepth - 1 ) ) + x ];

	if( *node )
	{
		return *node;
	}
	else
	{
		return AllocNode( node, nodeDepth - 1, x, y );
	}
}

/*
================
idQuadTree<type>::GetNode( const nodePosition_t &nodePosition  )
================
*/
template< class type >
ID_INLINE typename idQuadTree<type>::idQuadNode * idQuadTree<type>::GetNode( const nodePosition_t &nodePosition )
{
	idQuadNode** node = &nodes[ nodePosition.level ][ ( nodePosition.y << ( nodePosition.level ) ) + nodePosition.x ];

	if( *node )
	{
		return *node;
	}
	else
	{
		return AllocNode( node, nodePosition.level, nodePosition.x, nodePosition.y );
	}
}

/*
================
idQuadTree<type>::GetNumLeafNodes_r
================
*/
template< class type >
void idQuadTree<type>::GetNumLeafNodes_r( idQuadNode &node, int *numLeafNodes ) const
{
	int i;
	idQuadNode *child;

	if( !node.HasChildren() )
	{
		( *numLeafNodes )++;
		return;
	}

	for( i = 0; i < 4; i++ )
	{
		child = node.GetChild( i );
		if( child )
		{
			GetNumLeafNodes_r( *child, numLeafNodes );
		}
	}
}

/*
================
idQuadTree<type>::GetNumLeafNodes
================
*/
template< class type >
const int idQuadTree<type>::GetNumLeafNodes( void ) const
{
	int	numLeafNodes = 0;

	GetNumLeafNodes_r( *headNode, &numLeafNodes );

	return numLeafNodes;
}

/*
================
idQuadTree<type>::AllocNode
================
*/
template< class type >
ID_INLINE typename idQuadTree<type>::idQuadNode * idQuadTree<type>::AllocNode( idQuadNode **node, int nodeLevel, int x, int y )
{
	int			levelDimensions = idMath::Pow( 2, nodeLevel );
	idVec2		cellSize( ( headNode->GetBounds()[ 1 ].x - headNode->GetBounds()[ 0 ].x ) / levelDimensions,
		( headNode->GetBounds()[ 1 ].y - headNode->GetBounds()[ 0 ].y ) / levelDimensions );
	idBounds	nodeBounds;
	idVec2		nodeMins, pCellsize;
	int			pX, pY, pNodeLevel;

	// create the new node
	nodeBounds.Clear();
	nodeMins.Set( headNode->GetBounds()[ 0 ].x + x * cellSize.x, headNode->GetBounds()[ 0 ].y + y * cellSize.y );
	nodeBounds.AddPoint( idVec3( nodeMins.x, nodeMins.y, 0.f ) );
	nodeBounds.AddPoint( idVec3( nodeMins.x + cellSize.x, nodeMins.y + cellSize.y, 0.f ) );
	*node = new idQuadNode( nodeBounds );

	( *node )->SetNodePosition( nodeLevel, x, y );

	// find (and create) all its parents
	idQuadNode** parent;
	idQuadNode** child = node;
	pX = x;
	pY = y;
	pNodeLevel = nodeLevel;
	do
	{
		pX >>= 1;
		pY >>= 1;
		pNodeLevel--;

		parent = &nodes[ pNodeLevel ][ ( pY << ( pNodeLevel ) ) + pX ];

		if( !( *parent ) )
		{
			levelDimensions = idMath::Pow( 2, pNodeLevel );
			pCellsize.Set( ( headNode->GetBounds()[ 1 ].x - headNode->GetBounds()[ 0 ].x ) / levelDimensions,
				( headNode->GetBounds()[ 1 ].y - headNode->GetBounds()[ 0 ].y ) / levelDimensions );

			// create the new node
			nodeBounds.Clear();
			nodeMins.Set( headNode->GetBounds()[ 0 ].x + pX * pCellsize.x, headNode->GetBounds()[ 0 ].y + pY * pCellsize.y );
			nodeBounds.AddPoint( idVec3( nodeMins.x, nodeMins.y, 0.f ) );
			nodeBounds.AddPoint( idVec3( nodeMins.x + pCellsize.x, nodeMins.y + pCellsize.y, 0.f ) );
			*parent = new idQuadNode( nodeBounds );

			( *parent )->SetNodePosition( pNodeLevel, pX, pY );
		}

		( *child )->SetParent( *( *parent ) );

		child = parent;
	} while( *parent != headNode && !( *child )->GetParent() );

	// create its siblings
	pX = x & ~1;
	pY = y & ~1;
	for( x = pX; x < pX + 2; x++ )
	{
		for( y = pY; y < pY + 2; y++ )
		{
			idQuadNode** sibling = &nodes[ nodeLevel ][ ( y << nodeLevel ) + x ];

			if( sibling == node )
			{
				continue;
			}

			// create the new node
			nodeBounds.Clear();
			nodeMins.Set( headNode->GetBounds()[ 0 ].x + x * cellSize.x, headNode->GetBounds()[ 0 ].y + y * cellSize.y );
			nodeBounds.AddPoint( idVec3( nodeMins.x, nodeMins.y, 0.f ) );
			nodeBounds.AddPoint( idVec3( nodeMins.x + cellSize.x, nodeMins.y + cellSize.y, 0.f ) );
			*sibling = new idQuadNode( nodeBounds );
			( *sibling )->SetParent( *( ( *node )->GetParent() ) );
			( *sibling )->SetNodePosition( nodeLevel, x, y );
		}
	}

	return *node;
}

/*
================
idQuadTree<type>::FindChildren_r
================
*/
template< class type >
void idQuadTree<type>::FindChildren_r( idQuadNode &parent, const int nodeLevel )
{
	int			x, y;
	int			levelDimensions = idMath::Pow( 2, nodeLevel );
	idQuadNode*	child;

	// find all nodes with this node as a parent
	for( x = 0; x < levelDimensions; x++ )
	{
		for( y = 0; y < levelDimensions; y++ )
		{
			child = nodes[ nodeLevel ][ ( y << nodeLevel ) + x ];

			if( child && child->GetParent() == &parent )
			{
				parent.SetChild( *child, ( y % 2 ) * 2 + ( x % 2 ) );

				if( nodeLevel + 1 < depth )
				{
					FindChildren_r( *child, nodeLevel + 1 );
				}
			}
		}
	}
}

/*
================
idQuadTree<type>::FindChildren_r
================
*/
template< class type >
void idQuadTree<type>::FindChildren_r( idQuadNode &parent, const int nodeLevel, const int parentX, const int parentY )
{
	int			x, y;
	idQuadNode*	child;

	// find all nodes with this node as a parent
	for( x = parentX; x < parentX + 2; x++ )
	{
		for( y = parentY; y < parentY + 2; y++ )
		{
			child = nodes[ nodeLevel ][ ( y << nodeLevel ) + x ];

			if( child )
			{
				parent.SetChild( *child, ( ( y - parentY ) << 1 ) + ( x - parentX ) );

				if( nodeLevel + 1 < depth )
				{
					// transform parent coordinates to lower level coordinates
					FindChildren_r( *child, nodeLevel + 1, x << 1, y << 1 );
				}
			}
		}
	}
}

/*
================
idQuadTree<type>::FindNeighbors_r
================
*/
template< class type >
void idQuadTree<type>::FindNeighbors_r( const int nodeLevel )
{
	int			x, y;
	int			nbX, nbY;
	int			pX, pY, pNodeLevel;
	int			levelDimensions = idMath::Pow( 2, nodeLevel );
	idQuadNode	*node, *neighbor;

	for( x = 0; x < levelDimensions; x++ )
	{
		for( y = 0; y < levelDimensions; y++ )
		{
			node = nodes[ nodeLevel ][ ( y << nodeLevel ) + x ];

			if( !node )
			{
				continue;
			}

			// bottom neighbor (0)
			if( y > 0 )
			{
				nbX = x;
				nbY = y - 1;

				neighbor = nodes[ nodeLevel ][ ( nbY << nodeLevel ) + nbX ];

				// first see if we have a neighbor on this level
				if( neighbor )
				{
					node->SetNeighbor( *neighbor, 0 );
				}
				else if( !( y & 1 ) )
				{
					// try higher levels ( till > 0 as the headnode doesn't have any neighbors )
					for( pNodeLevel = nodeLevel - 1, pX = nbX >> 1, pY = nbY >> 1; pNodeLevel > 0; pNodeLevel--, pX >>= 1, pY >>= 1 )
					{
						neighbor = nodes[ pNodeLevel ][ ( pY << ( pNodeLevel ) ) + pX ];

						if( neighbor )
						{
							node->SetNeighbor( *neighbor, 0 );
							break;
						}
					}
				}
			}

			// left neighbor (1)
			if( x > 0 )
			{
				nbX = x - 1;
				nbY = y;

				neighbor = nodes[ nodeLevel ][ ( nbY << nodeLevel ) + nbX ];

				// first see if we have a neighbor on this level
				if( neighbor )
				{
					node->SetNeighbor( *neighbor, 1 );
				}
				else if( !( x & 1 ) )
				{
					// try higher levels ( till > 0 as the headnode doesn't have any neighbors )
					for( pNodeLevel = nodeLevel - 1, pX = nbX >> 1, pY = nbY >> 1; pNodeLevel > 0; pNodeLevel--, pX >>= 1, pY >>= 1 )
					{
						neighbor = nodes[ pNodeLevel ][ ( pY << ( pNodeLevel ) ) + pX ];

						if( neighbor )
						{
							node->SetNeighbor( *neighbor, 1 );
							break;
						}
					}
				}
			}

			// right neighbor (2)
			if( x < levelDimensions - 1 )
			{
				nbX = x + 1;
				nbY = y;

				neighbor = nodes[ nodeLevel ][ ( nbY << nodeLevel ) + nbX ];

				// first see if we have a neighbor on this level
				if( neighbor )
				{
					node->SetNeighbor( *neighbor, 2 );
				}
				else if( x & 1 )
				{
					// try higher levels ( till > 0 as the headnode doesn't have any neighbors )
					for( pNodeLevel = nodeLevel - 1, pX = nbX >> 1, pY = nbY >> 1; pNodeLevel > 0; pNodeLevel--, pX >>= 1, pY >>= 1 )
					{
						neighbor = nodes[ pNodeLevel ][ ( pY << ( pNodeLevel ) ) + pX ];

						if( neighbor )
						{
							node->SetNeighbor( *neighbor, 2 );
							break;
						}
					}
				}
			}

			// top neighbor (3)
			if( y < levelDimensions - 1 )
			{
				nbX = x;
				nbY = y + 1;

				neighbor = nodes[ nodeLevel ][ ( nbY << nodeLevel ) + nbX ];

				// first see if we have a neighbor on this level
				if( neighbor )
				{
					node->SetNeighbor( *neighbor, 3 );
				}
				else if( y & 1 )
				{
					// try higher levels ( till > 0 as the headnode doesn't have any neighbors )
					for( pNodeLevel = nodeLevel - 1, pX = nbX >> 1, pY = nbY >> 1; pNodeLevel > 0; pNodeLevel--, pX >>= 1, pY >>= 1 )
					{
						neighbor = nodes[ pNodeLevel ][ ( pY << ( pNodeLevel ) ) + pX ];

						if( neighbor )
						{
							node->SetNeighbor( *neighbor, 3 );
							break;
						}
					}
				}
			}
		}
	}

	if( nodeLevel + 1 < depth )
	{
		FindNeighbors_r( nodeLevel + 1 );
	}
}

/*
================
idQuadTree<type>::FindNeighbors_r
================
*/
template< class type >
void idQuadTree<type>::CreateChildren( idQuadNode &parent )
{
	int				x, y, parentX, parentY;
	idQuadNode**	child;
	nodePosition_t	parentNodePosition = parent.GetNodePosition();

	if( parentNodePosition.level + 1 >= depth )
	{
		return;
	}

	parentX = parentNodePosition.x << 1;
	parentY = parentNodePosition.y << 1;

	// create all the nodes children
	for( x = parentX; x < parentX + 2; x++ )
	{
		for( y = parentY; y < parentY + 2; y++ )
		{
			child = &nodes[ parentNodePosition.level + 1 ][ ( y << ( parentNodePosition.level + 1 ) ) + x ];

			if( !( *child ) )
			{
				AllocNode( child, parentNodePosition.level + 1, x, y );
			}

			parent.SetChild( **child, ( ( y - parentY ) << 1 ) + ( x - parentX ) );
		}
	}
}

#endif /* !__QUADTREE_H__ */
