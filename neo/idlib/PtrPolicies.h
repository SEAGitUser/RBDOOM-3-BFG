// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __LIB_PTRPOLICIES_H__
#define __LIB_PTRPOLICIES_H__

/*
============
 idNULLPtrCheck
============
*/
template< class T >
class idNULLPtrCheck {
public:
	typedef T* Pointer;
	static void Check( const Pointer p ) {
		assert( p != NULL && "NULL pointer dereferenced!" );
	}
};

/*
============
 idNoPtrCheck
============
*/
template< class T >
class idNoPtrCheck {
public:
	typedef T* Pointer;
	static void Check( const Pointer p ) {}
};

/*
============
 idDefaultCleanupPolicy
	standard cleanup via delete
============
*/
template< class T >
class idDefaultCleanupPolicy {
public:
	typedef T* Pointer;

	static void Free( Pointer p ) {
		delete p;
	}
};

/*
============
 idWeakCleanupPolicy
	do nothing
============
*/
template< class T >
class idWeakCleanupPolicy {
public:
	typedef T* Pointer;

	static void Free( Pointer p ) {
	}
};

/*
============
 idArrayCleanupPolicy
============
*/
template< class T >
class idArrayCleanupPolicy {
public:
	typedef T* Pointer;

	static void Free( Pointer p ) {
		delete [] p;
	}
};

/*
============
 idAlignedMemCleanupPolicy
============
*/
template< class T >
class idAlignedMemCleanupPolicy {
public:
	typedef T* Pointer;

	static void Free( Pointer p ) {
		Mem_Free16( p );
	}
};

#endif // ! __LIB_PTRPOLICIES_H__
