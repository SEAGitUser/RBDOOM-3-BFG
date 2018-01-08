// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __AUTOPTR_H__
#define __AUTOPTR_H__

/*
============
	A pointer template class that deletes its pointee when it is destroyed
	Not intended for use in containers
	An assignment causes the RHS to release its pointer, ie the pointee is only be owned by one idAutoPtr at a time
============
*/

#ifdef _DEBUG
template< class T, class CleanupPolicy = idDefaultCleanupPolicy< T >, class CheckingPolicy = idNULLPtrCheck< T > >
#else
template< class T, class CleanupPolicy = idDefaultCleanupPolicy< T >, class CheckingPolicy = idNoPtrCheck< T > >
#endif

class idAutoPtr {
public:
	typedef CleanupPolicy cleanupPolicy;
	typedef CheckingPolicy checkingPolicy;

	explicit idAutoPtr( T* p = NULL ) :
		Pointee ( p )
	#ifdef _DEBUG
		,size( 0 )
	#endif
	{ ; }
	~idAutoPtr() { cleanupPolicy::Free( Pointee ); }

	idAutoPtr( idAutoPtr< T, CleanupPolicy, CheckingPolicy >& rhs ) {
		Pointee = rhs.Release();
	#ifdef _DEBUG
		size = rhs.size;
	#endif
	}
	
	idAutoPtr< T, CleanupPolicy, CheckingPolicy >& operator=( idAutoPtr< T, CleanupPolicy, CheckingPolicy >& rhs ) {
		if( &rhs != this ) {
	#ifdef _DEBUG
			Reset( rhs.Release(), rhs.size );
	#else
			Reset( rhs.Release() );
	#endif			
		}
		return (*this);
	}

	T& operator[]( size_t index ) 
	{
		checkingPolicy::Check( Pointee );
	#ifdef _DEBUG
		if( size != 0 && ( index >= size ) ) {
			idLib::Error( "Index '%u' out of range '%u'", index, size );
		}
	#endif
		return Get()[ index ];
	}
	const T& operator[]( size_t index ) const 
	{
		checkingPolicy::Check( Pointee );
	#ifdef _DEBUG
		if( size != 0 && ( index >= size ) ) {
			idLib::Error( "Index '%u' out of range '%u'", index, size );
		}
	#endif
		return Get()[ index ];
	}

	bool		IsValid() const			{ return Pointee != NULL; }
	T*			operator->()			{ checkingPolicy::Check( Pointee ); return Get(); }
	const T*	operator->() const		{ checkingPolicy::Check( Pointee ); return Get(); }
	T&			operator*()				{ checkingPolicy::Check( Pointee ); return *Get(); }
	const T&	operator*() const		{ checkingPolicy::Check( Pointee ); return *Get(); }
	T*			Get()					{ return Pointee; }
	const T*	Get() const				{ return Pointee; }
	
	/*
	============
	Release
	============
	*/
	T*	Release() {
		T* tmp = Pointee;
		Pointee = NULL; 
		return tmp;
	}

	/*
	============
	Reset
	============
	*/
#ifdef _DEBUG
	void Reset( T* p = NULL, size_t size = 0 ) {
		this->size = size;
#else
	void Reset( T* p = NULL ) {
#endif
		if( p != Pointee ) {
			cleanupPolicy::Free( Pointee );
		}
		Pointee = p;
	}

	/*
	============
	Reset
	============
	*/
	template< class U >
#ifdef _DEBUG
	void Reset( U* p = NULL, size_t size = 0 ) {
		this->size = size;
#else
	void Reset( U* p = NULL ) {
#endif
		if( p != Pointee ) {
			cleanupPolicy::Free( Pointee );
		}
		Pointee = p;
	}

private:
	T* Pointee;
#ifdef _DEBUG
	size_t size;
#endif
};


#endif //__AUTOPTR_H__
