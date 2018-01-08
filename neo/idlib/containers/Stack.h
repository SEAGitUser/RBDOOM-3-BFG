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

#ifndef __STACK_H__
#define __STACK_H__

/*
===============================================================================

	Stack template

===============================================================================
*/

#define idStackT( type, next )		idStackTemplate<type, (int)&(((type*)NULL)->next)>

template< class type, int nextOffset >
class idStackTemplate
{
public:
	idStackTemplate();
	
	void					Add( type* element );
	type* 					Get();
	
private:
	type* 					top;
	type* 					bottom;
};

#define STACK_NEXT_PTR( element )		(*(type**)(((byte*)element)+nextOffset))

template< class type, int nextOffset >
idStackTemplate<type, nextOffset>::idStackTemplate()
{
	top = bottom = NULL;
}

template< class type, int nextOffset >
void idStackTemplate<type, nextOffset>::Add( type* element )
{
	STACK_NEXT_PTR( element ) = top;
	top = element;
	if( !bottom )
	{
		bottom = element;
	}
}

template< class type, int nextOffset >
type* idStackTemplate<type, nextOffset>::Get()
{
	type* element;
	
	element = top;
	if( element )
	{
		top = STACK_NEXT_PTR( top );
		if( bottom == element )
		{
			bottom = NULL;
		}
		STACK_NEXT_PTR( element ) = NULL;
	}
	return element;
}

/*
===============================================================================

	Stack template 2

===============================================================================
*/


template< class T >
class idStack {
public:
	T&				Push();
	T&				Push( const T& element );
	void			Pop();
	T&				Top();
	const T&		Top() const;

	void			Clear();
	void			DeleteContents( bool clear );
	void			SetGranularity( int newGranularity );
	bool			Empty() const;

	int				Num() const;
	void			Swap( idStack& rhs );

private:
	idList< T >		stack;
};

/*
============
idStack< T >::Push
============
*/
template< class T >
ID_INLINE T& idStack< T >::Push() {
	return stack.Alloc();
}

/*
============
idStack< T >::Push
============
*/
template< class T >
ID_INLINE T& idStack< T >::Push( const T& element ) {
	T& ref = stack.Alloc();
	ref = element;
	return ref;
}

/*
============
idStack< T >::Pop
============
*/
template< class T >
ID_INLINE void idStack< T >::Pop() {
	stack.RemoveIndex( Num() - 1 );
}

/*
============
idStack< T >::Top
============
*/
template< class T >
ID_INLINE T& idStack< T >::Top() {
	return stack[ Num() - 1 ];
}

/*
============
idStack< T >::Top
============
*/
template< class T >
ID_INLINE const T& idStack< T >::Top() const {
	return stack[ Num() - 1 ];
}

/*
============
idStack< T >::Clear
============
*/
template< class T >
ID_INLINE void idStack< T >::Clear() {
	stack.Clear();
}

/*
============
idStack< T >::DeleteContents
============
*/
template< class T >
ID_INLINE void idStack< T >::DeleteContents( bool clear ) {
	stack.DeleteContents( clear );
}

/*
============
idStack< T >::SetGranularity
============
*/
template< class T >
ID_INLINE void idStack< T >::SetGranularity( int newGranularity ) {
	stack.SetGranularity( newGranularity );
}

/*
============
idStack< T >::Empty
============
*/
template< class T >
ID_INLINE bool idStack< T >::Empty() const {
	return stack.Num() == 0;
}

/*
============
idStack< T >::Num
============
*/
template< class T >
ID_INLINE int idStack< T >::Num() const {
	return stack.Num();
}

/*
============
idStack< T >::Num
============
*/
template< class T >
ID_INLINE void idStack< T >::Swap( idStack& rhs ) {
	stack.Swap( rhs.stack );
}

/* jrad - we need to use a more traditional stack
template< typename type >
class idStackNode {
public:
				idStackNode( void ) { next = NULL; }

	type *		GetNext( void ) const { return next; }
	void		SetNext( type *next ) { this->next = next; }

private:
	type *		next;
};

template< typename type, idStackNode<type> type::*nodePtr >
class idStack {
public:
				idStack( void );

	void		Add( type *element );
	type *		RemoveFirst( void );

	static void	Test( void );

private:
	type *		first;
	type *		last;
};

template< typename type, idStackNode<type> type::*nodePtr >
idStack<type,nodePtr>::idStack( void ) {
	first = last = NULL;
}

template< typename type, idStackNode<type> type::*nodePtr >
void idStack<type,nodePtr>::Add( type *element ) {
	(element->*nodePtr).SetNext( first );
	first = element;
	if ( last == NULL ) {
		last = element;
	}
}

template< typename type, idStackNode<type> type::*nodePtr >
type *idStack<type,nodePtr>::RemoveFirst( void ) {
	type *element;

	element = first;
	if ( element ) {
		first = (first->*nodePtr).GetNext();
		if ( last == element ) {
			last = NULL;
		}
		(element->*nodePtr).SetNext( NULL );
	}
	return element;
}

template< typename type, idStackNode<type> type::*nodePtr >
void idStack<type,nodePtr>::Test( void ) {

	class idMyType {
	public:
		idStackNode<idMyType> stackNode;
	};

	idStack<idMyType,&idMyType::stackNode> myStack;

	idMyType *element = new idMyType;
	myStack.Add( element );
	element = myStack.RemoveFirst();
	delete element;
}
*/


#endif /* !__STACK_H__ */
