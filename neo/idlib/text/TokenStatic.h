#ifndef __TOKENSTATIC_H__
#define __TOKENSTATIC_H__

template< int _size_ >
class idTokenStatic : public idStrStatic< _size_ > {
	friend class idParser;
	friend class idLexer;
	friend class idLexerBinary;
	friend class idTokenCache;

public:
	int				type;				// token type
	int				subtype;			// token sub type
	int				line;				// line in script the token was on
	int				linesCrossed;		// number of lines crossed in white space before token
	int				flags;				// token flags, used for recursive defines

public:
	idTokenStatic() : idStrStatic(),
		intvalue( 0 ),
		floatvalue( 0.0 ),
		whiteSpaceStart_p( NULL ),
		whiteSpaceEnd_p( NULL ),
		next( NULL ),
		type( 0 ),
		subtype( 0 ),
		line( 0 ),
		linesCrossed( 0 ),
		flags( 0 ),
		binaryIndex( 0 )
	{
	}
	idTokenStatic( const idTokenStatic* token )
	{
		*this = *token;
	}
	~idTokenStatic()
	{
	}

	void			operator=( const idStrStatic& text ) { idStrStatic<_size_>::operator=( text ); }
	void			operator=( const idStr& text ) { idStr::operator=( text ); }
	void			operator=( const char* text ) { idStr::operator=( text ); }

	// double value of TT_NUMBER
	double			GetDoubleValue();

	// float value of TT_NUMBER
	float			GetFloatValue() { return ( float )GetDoubleValue(); }

	// unsigned int value of TT_NUMBER
	unsigned int	GetUnsignedIntValue();

	// int value of TT_NUMBER
	int				GetIntValue() { return ( int )GetUnsignedIntValue(); }

	// returns length of whitespace before token
	bool			WhiteSpaceBeforeToken() const { return( whiteSpaceEnd_p > whiteSpaceStart_p ); }
	
	// forget whitespace before token
	void			ClearTokenWhiteSpace();

	// token index in a binary stream
	unsigned short	GetBinaryIndex() const { return binaryIndex; }

	void			NumberValue();		// calculate values for a TT_NUMBER

private:
	unsigned int	intvalue;			// integer value
	double			floatvalue;			// floating point value
	const char * 	whiteSpaceStart_p;	// start of white space before token, only used by idLexer
	const char * 	whiteSpaceEnd_p;	// end of white space before token, only used by idLexer
	idTokenStatic *	next;				// next token in chain, only used by idParser
	unsigned short	binaryIndex;		// token index in a binary stream

	// only to be used by parsers

	// append character without adding trailing zero
	void			AppendDirty( const char a );
	void			SetIntValue( unsigned int intvalue ) { this->intvalue = intvalue; }
	void			SetFloatValue( double floatvalue ) { this->floatvalue = floatvalue; }
};

template< int _size_ >
ID_INLINE double idTokenStatic<_size_>::GetDoubleValue()
{
	if( type != TT_NUMBER )
	{
		return 0.0;
	}
	if( !( subtype & TT_VALUESVALID ) )
	{
		NumberValue();
	}
	return floatvalue;
}

template< int _size_ >
ID_INLINE unsigned int idTokenStatic<_size_>::GetUnsignedIntValue()
{
	if( type != TT_NUMBER ) {
		return 0;
	}
	if( !( subtype & TT_VALUESVALID ) )
	{
		NumberValue();
	}
	return intvalue;
}

template< int _size_ >
ID_INLINE void idTokenStatic<_size_>::AppendDirty( const char a )
{
	EnsureAlloced( len + 2, true );
	data[ len++ ] = a;
}

template< int _size_ >
ID_INLINE void idToken::ClearTokenWhiteSpace()
{
	whiteSpaceStart_p = NULL;
	whiteSpaceEnd_p = NULL;
	linesCrossed = 0;
}

#endif /*__TOKENSTATIC_H__*/
