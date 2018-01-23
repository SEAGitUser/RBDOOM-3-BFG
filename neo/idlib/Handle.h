// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __IDLIB_HANDLE_H__
#define __IDLIB_HANDLE_H__


/*
============
idHandle
============
*/
template< typename T, T invalidValue >
class idHandle {
public:
	typedef T valueType_t;

	idHandle() :
	value ( invalidValue ) {}

	idHandle( const valueType_t& rhs ) :
	value ( rhs ) {}

	idHandle& operator=( const valueType_t& rhs ) {
		value = rhs;
		return *this;
	}

	bool operator==( const idHandle< valueType_t, invalidValue >& rhs ) {
		return value == rhs.value;
	}

	bool operator!() {
		return !IsValid();
	}

	bool IsValid() const { 
		return value != INVALID_VALUE;
	}

	void Release() { 
		value = INVALID_VALUE;
	}

	operator valueType_t() const {
		return value;
	}

	valueType_t GetValue() const {
		return value;
	}

	valueType_t GetInvalidValue() const {
		return INVALID_VALUE;
	}

private:
	valueType_t value;
	static valueType_t INVALID_VALUE;
};

template< class T, T invalidValue > T idHandle< T, invalidValue >::INVALID_VALUE = invalidValue;

typedef idHandle<int, MAX_TYPE( int )>	idGenericHandle;
typedef idHandle<unsigned int, MAX_UNSIGNED_TYPE( unsigned int )>	idGenericUHandle;

typedef idHandle<short, MAX_TYPE( short )>	idGenericShortHandle;
typedef idHandle<unsigned short, MAX_UNSIGNED_TYPE( unsigned short )>	idGenericUShortHandle;


#endif // __IDLIB_HANDLE_H__
