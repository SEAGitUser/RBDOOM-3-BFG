// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __IDLIB_HANDLES_H__
#define __IDLIB_HANDLES_H__

#include "../Handle.h"

/*
============
idHandles
class to manage a set of handles to items
============
*/
template< class T >
class idHandles {
public:
	typedef idHandle< int, -1 > handle_t;

	handle_t			Acquire();
	void				Release( handle_t& handle );
	T&					operator[]( const handle_t& handle );
	const T&			operator[]( const handle_t& handle ) const;

	handle_t			GetFirst() const;
	handle_t			GetNext( const handle_t& handle ) const; 

	void				SetGranularity( int newGranularity );

	// returns the total number of slots available, use IsValid to make sure an element is actually usable
	int					Num() const;
	bool				IsValid( const handle_t& handle ) const;
					
	// all handles will be invalidated
	void				DeleteContents();

	void				Swap( idHandles& rhs );

private:
	idList< T >			items;
};

/*
============
idHandles< T >::Acquire
============
*/
template< class T >
typename idHandles< T >::handle_t idHandles< T >::Acquire() 
{
	for( int i = 0; i < items.Num(); i++ ) {
		if( items[ i ] == NULL ) {
			return handle_t( i );
		}
	}	
	return handle_t( items.Append( T() )); 
}

/*
============
idHandles< T >::Release
============
*/
template< class T >
void idHandles< T >::Release( handle_t& handle ) 
{
	assert( handle.IsValid() );
	items[ handle ] = NULL;
	handle.Release();
}

/*
============
idHandles< T >::operator[]
============
*/
template< class T >
T& idHandles< T >::operator[]( const handle_t& handle ) 
{
	assert( handle.IsValid() );
	return items[ handle ];
}

/*
============
idHandles< T >::operator[]
============
*/
template< class T >
const T& idHandles< T >::operator[]( const handle_t& handle ) const 
{
	assert( handle.IsValid() );
	return items[ handle ];	
}

/*
============
idHandles< T >::SetGranularity
============
*/
template< class T >
void idHandles< T >::SetGranularity( int newGranularity ) 
{
	items.SetGranularity( newGranularity );
}

/*
============
idHandles< T >::Num
============
*/
template< class T >
int idHandles< T >::Num() const
{
	return items.Num();
}

/*
============
idHandles< T >::IsValid
============
*/
template< class T >
bool idHandles< T >::IsValid( const handle_t& handle ) const 
{
	if( !handle.IsValid() ) {
		return false;
	}
	return items[ handle ] != NULL;
}

/*
============
idHandles< T >::DeleteContents
============
*/
template< class T >
void idHandles< T >::DeleteContents() 
{
	items.DeleteContents( true );
}

/*
============
idHandles< T >::GetFirst
============
*/
template< class T >
typename idHandles< T >::handle_t idHandles< T >::GetFirst() const 
{
	for( int i = 0; i < items.Num(); i++ ) {
		if( IsValid( i ) ) {
			return handle_t( i );
		}
	}
	return handle_t();
}

/*
============
idHandles< T >::GetNext
============
*/
template< class T >
typename idHandles< T >::handle_t idHandles< T >::GetNext( const handle_t& handle ) const 
{
	if( !handle.IsValid() || handle >= items.Num() ) {
		return handle_t();
	}

	for( int i = handle + 1; i < items.Num(); i++ ) {
		if( IsValid( i ) ) {
			return handle_t( i );
		}
	}
	return handle_t();
}

/*
============
idHandles< T >::Swap
============
*/
template< class T >
void idHandles< T >::Swap( idHandles& rhs ) 
{
	items.Swap( rhs.items );
}


#endif // ! __IDLIB_HANDLES_H__
