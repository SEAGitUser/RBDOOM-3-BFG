// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __BITFIELD_H__
#define __BITFIELD_H__

template< int MAX_BITS >
class idBitField {
private:
	static const int	INTBITS		= sizeof( int ) * 8;
	static const int	INTBITS_X	= INTBITS - 1;
	static const int	SIZE		= ( MAX_BITS / INTBITS ) + 1;

public:
						idBitField() { Clear(); }

	void				operator= ( const idBitField& other ) { memcpy( bits, other.bits, sizeof( bits ) ); }
	int					operator[] ( int bit ) const { return Get( bit ); }
	bool				operator!= ( const idBitField& other ) const { return memcmp( bits, other.bits, sizeof( bits ) ) != 0; }
	bool				operator== ( const idBitField& other ) const { return !(operator!=(other)); }
	int					Get( int bit ) const { return ( ( bits[ bit / INTBITS ] >> ( bit & INTBITS_X ) ) & 1 ); }
	void				Set( int bit ) { bits[ bit / INTBITS ] |= 1 << ( bit & INTBITS_X ); }
	void				Clear( int bit ) { bits[ bit / INTBITS ] &= ~( 1 << ( bit & INTBITS_X ) ); }

	void				Clear() { memset( bits, 0, sizeof( bits ) ); }
	void				SetAll() { memset( bits, 1, sizeof( bits ) ); }

	int*				GetDirect() { return bits; }
	const int*			GetDirect() const { return bits; }
	int					Size() const { return SIZE; }

private:
	int					bits[ SIZE ];
};

class idBitField_Dynamic {
private:
	static const int	INTBITS		= sizeof( int ) * 8;
	static const int	INTBITS_X	= INTBITS - 1;

public:
						idBitField_Dynamic() { bits = NULL; size = 0; }
						~idBitField_Dynamic() { Shutdown(); }

	static int			SizeForBits( int bits ) { return ( bits / INTBITS ) + 1; }

	void				Shutdown() { delete[] bits; bits = NULL; size = 0; }
	void				Init( int MAX_BITS ) { SetSize( SizeForBits( MAX_BITS ) ); }

	int					operator[] ( int bit ) const { return Get( bit ); }
	int					Get( int bit ) const { return ( ( bits[ bit / INTBITS ] >> ( bit & INTBITS_X ) ) & 1 ); }
	void				Set( int bit ) { bits[ bit / INTBITS ] |= 1 << ( bit & INTBITS_X ); }
	void				Clear( int bit ) { bits[ bit / INTBITS ] &= ~( 1 << ( bit & INTBITS_X ) ); }

	void				Clear() { if( bits ) { memset( bits, 0, size * sizeof( int ) ); } }
	void				SetAll() { memset( bits, 1, size * sizeof( int ) ); }

	int					GetSize() const { return size; }
	void				SetSize( int size ) { if ( this->size == size ) { return; } Shutdown(); this->size = size; if ( size > 0 ) { bits = new int[ size ]; } }
	int&				GetDirect( int index ) { return bits[ index ]; }
	const int&			GetDirect( int index ) const { return bits[ index ]; }

private:
	int*				bits;
	int					size;
};

class idBitField_Stack {
private:
	static const int	INTBITS		= sizeof( int ) * 8;
	static const int	INTBITS_X	= INTBITS - 1;

public:
						idBitField_Stack() { bits = NULL; }
						~idBitField_Stack() { ; }

	void				Init( int* bits, int size ) { this->size = size; assert( size > 0 ); this->bits = bits; }
	static int			GetSizeForMaxBits( int MAX_BITS ) { return ( MAX_BITS / INTBITS ) + 1; }

	int					operator[] ( int bit ) const { return Get( bit ); }
	int					Get( int bit ) const { return ( ( bits[ bit / INTBITS ] >> ( bit & INTBITS_X ) ) & 1 ); }
	void				Set( int bit ) { bits[ bit / INTBITS ] |= 1 << ( bit & INTBITS_X ); }
	void				Set( int bit, int to ) { bits[ bit / INTBITS ] ^= ( Get( bit ) ^ to ) << ( bit & INTBITS_X ); }
	void				Clear( int bit ) { bits[ bit / INTBITS ] &= ~( 1 << ( bit & INTBITS_X ) ); }

	void				Clear() { if( bits ) { memset( bits, 0, size * sizeof( int ) ); } }
	void				SetAll() { memset( bits, 1, size * sizeof( int ) ); }

	int					GetSize() const { return size; }
	int&				GetDirect( int index ) { return bits[ index ]; }
	const int&			GetDirect( int index ) const { return bits[ index ]; }

private:
	int*				bits;
	int					size;
};

template< typename Type >
class idBitFlags {
	Type flags;
public:
	idBitFlags() { flags = ( Type )0; }
	idBitFlags( Type other ) : flags( other ) {}

	// set specific flag(s)
	void SetFlag( const Type flag )
	{
		this->flags |= flag;
	}
	// clear specific flag(s)
	void ClearFlag( const Type flag )
	{
		this->flags &= ~flag;
	}
	// test for existance of specific flag(s)
	bool HasFlag( const Type flag ) const
	{
		return ( this->flags & flag ) != 0;
	}
	void Clear()
	{
		flags = ( Type )0;
	}

	void operator = ( Type other )
	{
		flags = other;
	}
	size_t Size() const
	{
		return sizeof( *this );
	}
	bool IsEmpty() const
	{
		return( flags == ( Type )0 );
	}
};

#endif /* !__BITFIELD_H__ */
