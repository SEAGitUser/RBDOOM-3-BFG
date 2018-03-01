// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __STRINGBUILDER_H__
#define __STRINGBUILDER_H__

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#endif

/*
===============================================================================

	Build up character strings without incurring multiple heap allocations/reallocations

===============================================================================
*/


/*
===============================================================================

	Policy classes for allocating data buffers

===============================================================================
*/
template< int Granularity, class CharType >
class idCharAllocator_Heap {
public:
	static const int GRANULARITY = Granularity;
	static CharType*	Alloc( int size ) { return new CharType[ size ]; }
	static void			Free( CharType* buffer ) { delete [] buffer; };
};

template< class StrType >
class idStringBuilder_Traits {
};

template<>
class idStringBuilder_Traits< idStr > {
public:
	typedef char CharType;
	typedef idStr StrType;
	static const CharType terminator;

	static CharType* StrnCpy( CharType* dest, const CharType * source, size_t count ) {
		return strncpy( dest, source, count );
	}
};


template<>
class idStringBuilder_Traits< idWStr > {
public:
	typedef wchar_t					CharType;
	typedef idWStr					StrType;
	static const CharType terminator;

	static CharType* StrnCpy( CharType* dest, const CharType * source, size_t count ) {
		return wcsncpy( dest, source, count );
	}
};


/*
===============================================================================

	String builder

===============================================================================
*/
template< class StrType, class CharAllocator >
class idStringBuilder {
public:
	typedef CharAllocator						Allocator;
	typedef idStringBuilder_Traits< StrType >	Traits;
	typedef typename Traits::CharType			CharType;

						idStringBuilder();
	explicit			idStringBuilder( const CharType* baseString );

	explicit			idStringBuilder( const idStringBuilder& rhs );
						~idStringBuilder();

	void				Clear();
	
						// take care when using c_str and an alloca-based allocator
						// and passing to a new function
	const CharType*		c_str() const;
	void				ToString( StrType& out ) const;
	void				Append( const CharType* rhs );
	void				Append( const CharType* rhs, int len );
	void				Append( CharType rhs );
	void				AppendNoColors( const CharType* rhs );
	int					Length() const;
	

	idStringBuilder &	operator+=( CharType rhs );
	idStringBuilder &	operator+=( const CharType* rhs );
	idStringBuilder 	operator+( const CharType* rhs );
	idStringBuilder &	operator=( const CharType* rhs );

protected:
	void				EnsureAlloced( int amount );

private:
	CharType	baseBuffer[ Allocator::GRANULARITY ];
	CharType*	data;
	int			len;
	int			alloced;
};

template< class StrType, class CharAllocator >
ID_INLINE idStringBuilder< StrType, CharAllocator >::idStringBuilder()
{
	data = &baseBuffer[ 0 ];
	Clear();
}

template< class StrType, class CharAllocator >
ID_INLINE idStringBuilder< StrType, CharAllocator >::idStringBuilder( const CharType* baseString )
{
	data = &baseBuffer[ 0 ];
	Clear();
	Append( baseString );
}

template< class StrType, class CharAllocator >
ID_INLINE idStringBuilder< StrType, CharAllocator >::idStringBuilder( const idStringBuilder& rhs ) 
{
	data = &baseBuffer[ 0 ];
	Clear();
	if( rhs.data != NULL && rhs.len > 0 )
	{
		EnsureAlloced( rhs.len + 1 );
		for( int i = 0; rhs.data[ i ]; ++i ) {
			data[ i ] = rhs.data[ i ];
		}
		data[ len ] = '\0';
	}
}

template< class StrType, class CharAllocator >
ID_INLINE idStringBuilder< StrType, CharAllocator >::~idStringBuilder()
{
	Clear();
}

template< class StrType, class CharAllocator >
ID_INLINE void idStringBuilder< StrType, CharAllocator >::Clear() 
{
	if( data != &baseBuffer[ 0 ] ) {
		Allocator::Free( data );
	}
	
	data = &baseBuffer[ 0 ];
	data[ 0 ] = '\0';
	len = 0;
	alloced = Allocator::GRANULARITY;
}

template< class StrType, class CharAllocator >
ID_INLINE void idStringBuilder< StrType, CharAllocator >::EnsureAlloced( int amount )
{
	if( alloced >= amount ) {
		return;
	}

	int	newsize;
	assert( amount > 0 );

	int mod = amount % Allocator::GRANULARITY;
	if ( !mod ) {
		newsize = amount;
	}
	else {
		newsize = amount + Allocator::GRANULARITY - mod;
	}
	alloced = newsize;

	CharType* newbuffer = Allocator::Alloc( alloced );

	if( data != NULL ) 
	{
		if( len ) {
			Traits::StrnCpy( newbuffer, data, len );
			newbuffer[ len ] = '\0';
		}
		else {
			newbuffer[ 0 ] = '\0';
		}
	}

	if( data != &baseBuffer[ 0 ] ) {
		Allocator::Free( data );
	}

	data = newbuffer;
}

template< class StrType, class CharAllocator >
ID_INLINE void idStringBuilder< StrType, CharAllocator >::Append( const CharType* rhs )
{
	if( rhs != NULL )
	{
		int newLen = len + StrType::Length( rhs );
		EnsureAlloced( newLen + 1 );
		for( int i = 0; rhs[ i ]; ++i ) {
			data[ len + i ] = rhs[ i ];
		}
		len = newLen;
		data[ len ] = '\0';
	}
}

template< class StrType, class CharAllocator >
ID_INLINE void idStringBuilder< StrType, CharAllocator >::AppendNoColors( const CharType* rhs )
{
	if( rhs != NULL ) 
	{
		int newLen = len + StrType::LengthWithoutColors( rhs );
		EnsureAlloced( newLen + 1 );
		int offset = 0;
		for( int i = 0; rhs[ i ]; ++i )
		{
			if( StrType::IsColor( &rhs[ i ] ) ) {
				i++;
			} 
			else {
				data[ len + offset ] = rhs[ i ];
				offset++;
			}			
		}
		len = newLen;
		data[ len ] = '\0';
	}
}

template< class StrType, class CharAllocator >
ID_INLINE void idStringBuilder< StrType, CharAllocator >::Append( const CharType* rhs, int numToCopy )
{
	assert( numToCopy > 0 );
	if( rhs != NULL && numToCopy > 0 ) 
	{
		numToCopy = idMath::Min( StrType::Length( rhs ), numToCopy );
		int newLen = len + numToCopy;
		EnsureAlloced( newLen + 1 );
		for( int i = 0; i < numToCopy; ++i ) {
			data[ len + i ] = rhs[ i ];
		}
		len = newLen;
		data[ len ] = '\0';
	}
}

template< class StrType, class CharAllocator >
ID_INLINE void idStringBuilder< StrType, CharAllocator >::Append( CharType rhs )
{
	int newLen = len + 1;
	EnsureAlloced( newLen + 1 );
	data[ len ] = rhs;
	len = newLen;
	data[ len ] = '\0';
}

template< class StrType, class CharAllocator >
ID_INLINE idStringBuilder< StrType, CharAllocator >& idStringBuilder< StrType, CharAllocator >::operator+=( const CharType* str )
{
	Append( str );
	return *this;
}

template< class StrType, class CharAllocator >
ID_INLINE idStringBuilder< StrType, CharAllocator >& idStringBuilder< StrType, CharAllocator >::operator=( const CharType* str )
{
	len = 0;
	data[ len ] = '\0';
	Append( str );
	return *this;
}

template< class StrType, class CharAllocator >
ID_INLINE idStringBuilder< StrType, CharAllocator >& idStringBuilder< StrType, CharAllocator >::operator+=( CharType c )
{
	Append( c );
	return *this;
}

template< class StrType, class CharAllocator >
ID_INLINE idStringBuilder< StrType, CharAllocator > idStringBuilder< StrType, CharAllocator >::operator+( const CharType* str )
{
	idStringBuilder< StrType, CharAllocator > temp( this );
	temp += str;
	return temp;
}

template< class StrType, class CharAllocator >
ID_INLINE void idStringBuilder< StrType, CharAllocator >::ToString( StrType& out ) const
{
	if( data == NULL || len == 0 ) {
		out.Clear();
		return;
	}
	out = data;
}

template< class StrType, class CharAllocator >
ID_INLINE const typename idStringBuilder< StrType, CharAllocator >::CharType* idStringBuilder< StrType, CharAllocator >::c_str() const 
{
	return data;
}

template< class StrType, class CharAllocator >
ID_INLINE int idStringBuilder< StrType, CharAllocator >::Length() const
{
	return len;
}

typedef idStringBuilder< idStr, idCharAllocator_Heap< 512, char > > idStringBuilder_Heap;
typedef idStringBuilder< idWStr, idCharAllocator_Heap< 512, wchar_t > > idWStringBuilder_Heap;

#endif //__STRINGBUILDER_H__
