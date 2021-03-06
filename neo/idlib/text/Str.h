/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 Robert Beckebans

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

#ifndef __STR_H__
#define __STR_H__

/*
===============================================================================

	Character string

===============================================================================
*/

#define ASSERT_ENUM_STRING( string, index )		( 1 / (int)!( string - index ) ) ? #string : ""

enum utf8Encoding_t
{
	UTF8_PURE_ASCII,		// no characters with values > 127
	UTF8_ENCODED_BOM,		// characters > 128 encoded with UTF8, but no byte-order-marker at the beginning
	UTF8_ENCODED_NO_BOM,	// characters > 128 encoded with UTF8, with a byte-order-marker at the beginning
	UTF8_INVALID,			// has values > 127 but isn't valid UTF8
	UTF8_INVALID_BOM		// has a byte-order-marker at the beginning, but isn't valuid UTF8 -- it's messed up
};

// these library functions should not be used for cross platform compatibility
#define strcmp			idStr::Cmp		// use_idStr_Cmp
#define strlen			idStr::Length	// returns int instead of size_t
#define strncmp			use_idStr_Cmpn

#if defined( StrCmpN )
#undef StrCmpN
#endif
#define StrCmpN			use_idStr_Cmpn

#if defined( strcmpi )
#undef strcmpi
#endif
#define strcmpi			use_idStr_Icmp

#if defined( StrCmpI )
#undef StrCmpI
#endif
#define StrCmpI			use_idStr_Icmp

#if defined( StrCmpNI )
#undef StrCmpNI
#endif
#define StrCmpNI		use_idStr_Icmpn

#define stricmp			idStr::Icmp		// use_idStr_Icmp

#undef strcasecmp // DG: redefining this without undefining it causes tons of compiler warnings

#define _stricmp		use_idStr_Icmp
#define strcasecmp		use_idStr_Icmp
#define strnicmp		use_idStr_Icmpn
#define _strnicmp		use_idStr_Icmpn
#define _memicmp		use_idStr_Icmpn
#define StrCmpNI		use_idStr_Icmpn
#define snprintf		use_idStr_snPrintf
#define _snprintf		use_idStr_snPrintf
#define vsnprintf		use_idStr_vsnPrintf
#define _vsnprintf		use_idStr_vsnPrintf
#define _vsnwprintf		use_idWStr_vsnPrintf
#define vsnwprintf		use_idWStr_vsnPrintf

class idVec4;
class idCmdArgs;

#ifndef FILE_HASH_SIZE
#define FILE_HASH_SIZE		1024
#endif

const int COLOR_BITS				= 15;	// 31
const int DW_COLOR_BITS				= 31;

//SEA: MAGENTA <-> ORANGE ?

// color escape character
const int C_COLOR_ESCAPE			= '^';
const int C_COLOR_DEFAULT			= '0';
const int C_COLOR_RED				= '1';
const int C_COLOR_GREEN				= '2';
const int C_COLOR_YELLOW			= '3';
const int C_COLOR_BLUE				= '4';
const int C_COLOR_CYAN				= '5';
const int C_COLOR_ORANGE			= '6';
const int C_COLOR_WHITE				= '7';
const int C_COLOR_GRAY				= '8';
const int C_COLOR_BLACK				= '9';
const int C_COLOR_LT_GREY			= ':';
const int C_COLOR_MD_GREEN			= '<';
const int C_COLOR_MD_YELLOW			= '=';
const int C_COLOR_MD_BLUE			= '>';
const int C_COLOR_MD_RED			= '?';
const int C_COLOR_LT_ORANGE			= 'A';
const int C_COLOR_MD_CYAN			= 'B';
const int C_COLOR_MD_PURPLE			= 'C';
const int C_COLOR_MAGENTA			= 'D';

// color escape string
#define S_COLOR_DEFAULT				"^0"
#define S_COLOR_RED					"^1"
#define S_COLOR_GREEN				"^2"
#define S_COLOR_YELLOW				"^3"
#define S_COLOR_BLUE				"^4"
#define S_COLOR_CYAN				"^5"
#define S_COLOR_ORANGE				"^6"
#define S_COLOR_WHITE				"^7"
#define S_COLOR_GRAY				"^8"
#define S_COLOR_BLACK				"^9"
#define S_COLOR_LT_GREY				"^:"
#define S_COLOR_MD_GREEN			"^<"
#define S_COLOR_MD_YELLOW			"^="
#define S_COLOR_MD_BLUE				"^>"
#define S_COLOR_MD_RED				"^?"
#define S_COLOR_LT_ORANGE			"^A"
#define S_COLOR_MD_CYAN				"^B"
#define S_COLOR_MD_PURPLE			"^C"
#define S_COLOR_MAGENTA				"^D"

// make idStr a multiple of 16 bytes long
// don't make too large to keep memory requirements to a minimum
const int STR_ALLOC_BASE			= 20;
const int STR_ALLOC_GRAN			= 32;

enum measure_t
{
	MEASURE_SIZE = 0,
	MEASURE_BANDWIDTH
};

class idStr {
public:
	struct hmsFormat_t {
		hmsFormat_t() : showZeroMinutes( false ), showZeroHours( false ), showZeroSeconds( true ) {}
		bool showZeroMinutes;
		bool showZeroHours;
		bool showZeroSeconds;
	};

	idStr();
	idStr( const idStr& text );
	idStr( const idStr& text, int start, int end );
	idStr( const char* text );
	idStr( const char* text, int start, int end );
	explicit idStr( const bool b );
	explicit idStr( const char c );
	explicit idStr( const int i );
	explicit idStr( const unsigned u );
	explicit idStr( const float f );
	~idStr();

	size_t					Size() const;
	const char* 			c_str() const;
	operator				const char* () const;
	operator				const char* ();

	char					operator[]( int index ) const;
	char& 					operator[]( int index );

	void					operator=( const idStr& text );
	void					operator=( const char* text );

	friend idStr			operator+( const idStr& a, const idStr& b );
	friend idStr			operator+( const idStr& a, const char* b );
	friend idStr			operator+( const char* a, const idStr& b );

	friend idStr			operator+( const idStr& a, const float b );
	friend idStr			operator+( const idStr& a, const int b );
	friend idStr			operator+( const idStr& a, const unsigned b );
	friend idStr			operator+( const idStr& a, const bool b );
	friend idStr			operator+( const idStr& a, const char b );

	idStr& 					operator+=( const idStr& a );
	idStr& 					operator+=( const char* a );
	idStr& 					operator+=( const float a );
	idStr& 					operator+=( const char a );
	idStr& 					operator+=( const int a );
	idStr& 					operator+=( const unsigned a );
	idStr& 					operator+=( const bool a );

	// case sensitive compare
	friend bool				operator==( const idStr& a, const idStr& b );
	friend bool				operator==( const idStr& a, const char* b );
	friend bool				operator==( const char* a, const idStr& b );

	// case sensitive compare
	friend bool				operator!=( const idStr& a, const idStr& b );
	friend bool				operator!=( const idStr& a, const char* b );
	friend bool				operator!=( const char* a, const idStr& b );

	// case sensitive compare
	int						Cmp( const char* text ) const;
	int						Cmpn( const char* text, int n ) const;
	int						CmpPrefix( const char* text ) const;

	// case insensitive compare
	int						Icmp( const char* text ) const;
	int						Icmpn( const char* text, int n ) const;
	int						IcmpPrefix( const char* text ) const;

	// case insensitive compare ignoring color
	int						IcmpNoColor( const char* text ) const;

	// compares paths and makes sure folders come first
	int						IcmpPath( const char* text ) const;
	int						IcmpnPath( const char* text, int n ) const;
	int						IcmpPrefixPath( const char* text ) const;

	int						Length() const;
	int						Allocated() const;
	void					Empty();
	bool					IsEmpty() const;
	void					Clear();
	void					Append( const char a );
	void					Append( const idStr& text );
	void					Append( const char* text );
	void					Append( const char* text, int len );
	void					Append( int count, const char c );
	void					Insert( const char a, int index );
	void					Insert( const char* text, int index );
	void					ToLower();
	void					ToUpper();
	bool					IsNumeric() const;
	bool					IsColor() const;
	bool					IsHexColor( void ) const;
	bool					HasHexColorAlpha( void ) const;
	bool					HasLower() const;
	bool					HasUpper() const;
	int						LengthWithoutColors() const;
	idStr& 					RemoveColors();
	void					CapLength( int );
	void					Fill( const char ch, int newlen );
	void					Swap( idStr& rhs );

	ID_INLINE int			UTF8Length();
	ID_INLINE uint32		UTF8Char( int& idx );
	static int				UTF8Length( const byte* s );
	static ID_INLINE uint32 UTF8Char( const char* s, int& idx );
	static uint32			UTF8Char( const byte* s, int& idx );
	void					AppendUTF8Char( uint32 c );
	ID_INLINE void			ConvertToUTF8();
	static bool				IsValidUTF8( const uint8* s, const int maxLen, utf8Encoding_t& encoding );
	static ID_INLINE bool	IsValidUTF8( const char* s, const int maxLen, utf8Encoding_t& encoding ) { return IsValidUTF8( ( const uint8* )s, maxLen, encoding ); }
	static ID_INLINE bool	IsValidUTF8( const uint8* s, const int maxLen );
	static ID_INLINE bool	IsValidUTF8( const char* s, const int maxLen ) { return IsValidUTF8( ( const uint8* )s, maxLen ); }

	int						Find( const char c, int start = 0, int end = INVALID_POSITION ) const;
	int						Find( const char* text, bool casesensitive = true, int start = 0, int end = INVALID_POSITION ) const;
	const char*				FindString( const char* text, bool casesensitive = true, int start = 0, int end = INVALID_POSITION ) const;
	int						CountChar( const char c );
	bool					Filter( const char* filter, bool casesensitive ) const;
	// return the index to the last occurance of 'c', returns INVALID_POSITION if not found
	int						Last( const char c, int index = INVALID_POSITION ) const;
	// return the index to the last occurance of 'str', returns INVALID_POSITION if not found
	int						Last( const char* str, bool casesensitive = true, int index = INVALID_POSITION ) const;	
	// store the leftmost 'len' characters in the result
	const char* 			Left( int len, idStr& result ) const;
	// store the rightmost 'len' characters in the result
	const char* 			Right( int len, idStr& result ) const;
	// store 'len' characters starting at 'start' in result
	const char* 			Mid( int start, int len, idStr& result ) const;	
	// return the leftmost 'len' characters
	idStr					Left( int len ) const;
	// return the rightmost 'len' characters
	idStr					Right( int len ) const;
	// return 'len' characters starting at 'start'
	idStr					Mid( int start, int len ) const;
	// perform a threadsafe sprintf to the string
	template< uint32 _size_ = MAX_STRING_CHARS > idStr & Format( VERIFY_FORMAT_STRING const char* fmt, ... ); // MAX_PRINT_MSG
	// formats an integer as a value with commas
	static idStr			FormatInt( const int num, bool isCash = false );
	static idStr			FormatCash( const int num ) { return FormatInt( num, true ); }

	// strip char from front as many times as the char occurs
	void					StripLeading( const char c );
	// strip string from front as many times as the string occurs
	void					StripLeading( const char* string );
	// strip string from front just once if it occurs
	bool					StripLeadingOnce( const char* string );
	// strip char from end as many times as the char occurs
	void					StripTrailing( const char c );
	// strip string from end as many times as the string occurs
	void					StripTrailing( const char* string );
	// strip string from end just once if it occurs
	bool					StripTrailingOnce( const char* string );
	// strip char from front and end as many times as the char occurs
	void					Strip( const char c );
	// strip string from front and end as many times as the string occurs
	void					Strip( const char* string );
	// strip leading white space characters
	void					StripLeadingWhiteSpace();
	// strip trailing white space characters
	void					StripTrailingWhitespace();
	// strip quotes around string
	idStr& 					StripQuotes();
	bool					Replace( const char* old, const char* nw );
	void					ReplaceFirst( const char *old, const char *nw );
	bool					ReplaceChar( const char oldChar, const char newChar );
	void					EraseRange( int start, int len = INVALID_POSITION );
	void					EraseChar( const char c, int start = 0 );

	ID_INLINE void			CopyRange( const char* text, int start, int end );

	// File name methods

	// hash key for the filename (skips extension)
	int						FileNameHash( const int hashSize = FILE_HASH_SIZE ) const;
	// where possible removes /../ and /./ from path
	idStr &					CollapsePath();	
	// convert slashes
	idStr& 					BackSlashesToSlashes();
	// convert slashes
	idStr& 					SlashesToBackSlashes();	
	// set the given file extension
	idStr& 					SetFileExtension( const char* extension );
	// remove any file extension
	idStr& 					StripFileExtension();
	// remove any file extension looking from front (useful if there are multiple .'s)
	idStr& 					StripAbsoluteFileExtension();	
	// if there's no file extension use the default
	idStr& 					DefaultFileExtension( const char* extension );
	// if there's no path use the default
	idStr& 					DefaultPath( const char* basepath );
	// append a partial path
	void					AppendPath( const char* text );	
	// remove the filename from a path
	idStr& 					StripFilename();
	// remove the path from the filename
	idStr& 					StripPath();
	// copy the file path to another string
	void					ExtractFilePath( idStr& dest ) const;
	// copy the filename to another string
	void					ExtractFileName( idStr& dest ) const;
	// copy the filename minus the extension to another string
	void					ExtractFileBase( idStr& dest ) const;
	// copy the file extension to another string
	void					ExtractFileExtension( idStr& dest ) const;		
	bool					CheckExtension( const char* ext );
	// remove C++ and C style comments
	idStr&					StripComments();
	// indents brace-delimited text, preserving tabs in the middle of lines
	idStr& 					Indent();
	// unindents brace-delimited text, preserving tabs in the middle of lines
	idStr& 					Unindent();
	// strips bad characters
	idStr&					CleanFilename();							
	bool					IsValidEmailAddress();
	// removes redundant color codes
	idStr&					CollapseColors();							

// jscott: like the declManager version, but globally accessible
	void					MakeNameCanonical();

	// Char * methods to replace library functions

	static int				Length( const char* s );
	static char* 			ToLower( char* s );
	static char* 			ToUpper( char* s );
	static bool				IsNumeric( const char* s );
	static bool				IsColor( const char* s );
	static bool				IsHexColor( const char *s );
	static bool				HasHexColorAlpha( const char *s );
	static bool				HasLower( const char* s );
	static bool				HasUpper( const char* s );
	static int				LengthWithoutColors( const char* s );
	static char* 			RemoveColors( char* s );
	static bool				IsBadFilenameChar( char c );
	static char *			CleanFilename( char *s );
	static char *			StripFilename( char *s );
	static char *			StripPath( char *s );
	static int				Cmp( const char* s1, const char* s2 );
	static int				Cmpn( const char* s1, const char* s2, int n );
	static int				Icmp( const char* s1, const char* s2 );
	static int				Icmpn( const char* s1, const char* s2, int n );
	static int				IcmpNoColor( const char* s1, const char* s2 );
	// compares paths and makes sure folders come first
	static int				IcmpPath( const char* s1, const char* s2 );	
	// compares paths and makes sure folders come first
	static int				IcmpnPath( const char* s1, const char* s2, int n );	
	static void				Append( char* dest, int size, const char* src );
	static void				Copynz( char* dest, const char* src, size_t destsize );
	static void				Copy( char* dest, const char* src );
	static int				snPrintf( char* dest, int size, VERIFY_FORMAT_STRING const char* fmt, ... );
	static int				vsnPrintf( char* dest, int size, const char* fmt, va_list argptr );
	static int				FindChar( const char* str, const char c, int start = 0, int end = INVALID_POSITION );
	static int				FindText( const char* str, const char* text, bool casesensitive = true, int start = 0, int end = INVALID_POSITION );
	static const char*		FindString( const char *str, const char *text, bool casesensitive = true, int start = 0, int end = INVALID_POSITION );
	static int				CountChar( const char *str, const char c );
	static bool				Filter( const char* filter, const char* name, bool casesensitive );
	static void				StripMediaName( const char* name, idStr& mediaName );
	static bool				CheckExtension( const char* name, const char* ext );
	static const char* 		FloatArrayToString( const float* array, const int length, const int precision );
	static const char* 		CStyleQuote( const char* str );
	static const char* 		CStyleUnQuote( const char* str );
	static void				IndentAndPad( int indent, int pad, idStr &str, const char *fmt, ... );  // indent and pad out formatted text

	static void				StringToBinaryString( idStr& out, void *pv, int size);
	static bool				BinaryStringToString( const char* str,  void *pv, int size );

	static bool				IsValidEmailAddress( const char* address );

	static const char*		MS2HMS( double ms, const hmsFormat_t& formatSpec = defaultHMSFormat );
//	static const char *		FormatInt( const int num );	 // formats an integer as a value with commas

	// Hash keys

	static int				Hash( const char* string );
	static int				Hash( const char* string, int length );
	// case insensitive
	static int				IHash( const char* string );
	// case insensitive
	static int				IHash( const char* string, int length );
	// hash key for the filename (skips extension)
	static int				FileNameHash( const char *string, const int hashSize );	

	// character methods
	static char				ToLower( char c );
	static char				ToUpper( char c );
	static bool				CharIsPrintable( int c );
	static bool				CharIsLower( int c );
	static bool				CharIsUpper( int c );
	static bool				CharIsAlpha( int c );
	static bool				CharIsNumeric( int c );
	static bool				CharIsNewLine( char c );
	static bool				CharIsTab( char c );
	static bool				CharIsHex( int c );
	static int				ColorIndex( int c );
	static const idVec4 &	ColorForIndex( int i );
	static const idVec4 &	ColorForChar( int c );
	static const char*		StrForColorIndex( int i );
	static int				HexForChar( int c );

	friend int				sprintf( idStr &dest, const char *fmt, ... );
	friend int				vsprintf( idStr &dest, const char *fmt, va_list ap );

	// reallocate string data buffer
	void					ReAllocate( int amount, bool keepold );
	// free allocated string memory
	void					FreeData();									

	// format value in the given measurement with the best unit, returns the best unit
	int						BestUnit( const char* format, float value, measure_t measure );
	// format value in the requested unit and measurement
	void					SetUnit( const char* format, float value, int unit, measure_t measure );

	static void				InitMemory();
	static void				ShutdownMemory();
	static void				PurgeMemory();
	static void				ShowMemoryUsage_f( const idCmdArgs& args );

	int						DynamicMemoryUsed() const;
	static idStr			FormatNumber( int number );

	static void				Test( void );

protected:
	int						len;
	char* 					data;
	int						allocedAndFlag;	// top bit is used to store a flag that indicates if the string data is static or not
	char					baseBuffer[ STR_ALLOC_BASE ];

	// ensure string data buffer is large anough
	void					EnsureAlloced( int amount, bool keepold = true );	

	// sets the data point to the specified buffer... note that this ignores makes the passed buffer empty and ignores
	// anything currently in the idStr's dynamic buffer.  This method is intended to be called only from a derived class's constructor.
	ID_INLINE void			SetStaticBuffer( char* buffer, const int bufferLength );

	static hmsFormat_t		defaultHMSFormat;

private:
	// initialize string using base buffer... call ONLY FROM CONSTRUCTOR
	ID_INLINE void			Construct();

	static const uint32		STATIC_BIT	= 31;
	static const uint32		STATIC_MASK	= 1u << STATIC_BIT;
	static const uint32		ALLOCED_MASK = STATIC_MASK - 1;


	ID_INLINE int			GetAlloced() const { return allocedAndFlag & ALLOCED_MASK; }
	ID_INLINE void			SetAlloced( const int a ) { allocedAndFlag = ( allocedAndFlag & STATIC_MASK ) | ( a & ALLOCED_MASK ); }

	ID_INLINE bool			IsStatic() const { return ( allocedAndFlag & STATIC_MASK ) != 0; }
	ID_INLINE void			SetStatic( const bool isStatic ) { allocedAndFlag = ( allocedAndFlag & ALLOCED_MASK ) | ( isStatic << STATIC_BIT ); }

public:
	static const int		INVALID_POSITION = -1;
};

char* 						va( VERIFY_FORMAT_STRING const char* fmt, ... ) ID_STATIC_ATTRIBUTE_PRINTF( 1, 2 );

/*
================================================================================================

	Sort routines for sorting idList<idStr>

================================================================================================
*/

class idSort_Str : public idSort_Quick< idStr, idSort_Str > {
public:
	int Compare( const idStr& a, const idStr& b ) const
	{
		return a.Icmp( b );
	}
};

class idSort_PathStr : public idSort_Quick< idStr, idSort_PathStr > {
public:
	int Compare( const idStr& a, const idStr& b ) const
	{
		return a.IcmpPath( b );
	}
};


/*
========================
idStr::Format

perform a threadsafe sprintf to the string
========================
*/
template< uint32 _size_ >
idStr & idStr::Format( const char* fmt, ... )
{
	va_list argptr;
	char text[ _size_ ]; // default MAX_PRINT_MSG

	va_start( argptr, fmt );
	int len = idStr::vsnPrintf( text, sizeof( text ) - 1, fmt, argptr );
	va_end( argptr );
	text[ sizeof( text ) - 1 ] = '\0';

	if( ( size_t )len >= sizeof( text ) - 1 )
	{
		idLib::FatalError( "Tried to set a large buffer using %s", fmt );
	}
	*this = text;

	return *this;
}

/*
========================
idStr::Construct
========================
*/
ID_INLINE void idStr::Construct()
{
	SetStatic( false );
	SetAlloced( STR_ALLOC_BASE );
	data = baseBuffer;
	len = 0;
	data[ 0 ] = '\0';
#ifdef ID_DEBUG_UNINITIALIZED_MEMORY
	memset( baseBuffer, 0, sizeof( baseBuffer ) );
#endif
}


ID_INLINE void idStr::EnsureAlloced( int amount, bool keepold )
{
	// static string's can't reallocate
	if( IsStatic() )
	{
		release_assert( amount <= GetAlloced() );
		return;
	}
	if( amount > GetAlloced() )
	{
		ReAllocate( amount, keepold );
	}
}

/*
========================
idStr::SetStaticBuffer
========================
*/
ID_INLINE void idStr::SetStaticBuffer( char* buffer, const int bufferLength )
{
	// this should only be called on a freshly constructed idStr
	assert( data == baseBuffer );
	data = buffer;
	len = 0;
	SetAlloced( bufferLength );
	SetStatic( true );
}

ID_INLINE idStr::idStr()
{
	Construct();
}

ID_INLINE idStr::idStr( const idStr& text )
{
	Construct();
	int l;

	l = text.Length();
	EnsureAlloced( l + 1 );
	strcpy( data, text.data );
	len = l;
}

ID_INLINE idStr::idStr( const idStr& text, int start, int end )
{
	Construct();
	int i;
	int l;

	if( end > text.Length() )
	{
		end = text.Length();
	}
	if( start > text.Length() )
	{
		start = text.Length();
	}
	else if( start < 0 )
	{
		start = 0;
	}

	l = end - start;
	if( l < 0 )
	{
		l = 0;
	}

	EnsureAlloced( l + 1 );

	for( i = 0; i < l; i++ )
	{
		data[ i ] = text[ start + i ];
	}

	data[ l ] = '\0';
	len = l;
}

ID_INLINE idStr::idStr( const char* text )
{
	Construct();
	if( text )
	{
		int l = Length( text );
		EnsureAlloced( l + 1 );
		strcpy( data, text );
		len = l;
	}
}

ID_INLINE idStr::idStr( const char* text, int start, int end )
{
	Construct();
	int l = Length( text );

	if( end > l )
	{
		end = l;
	}
	if( start > l )
	{
		start = l;
	}
	else if( start < 0 )
	{
		start = 0;
	}

	l = end - start;
	if( l < 0 )
	{
		l = 0;
	}

	EnsureAlloced( l + 1 );

	for( int i = 0; i < l; i++ )
	{
		data[ i ] = text[ start + i ];
	}

	data[ l ] = '\0';
	len = l;
}

ID_INLINE idStr::idStr( const bool b )
{
	Construct();
	EnsureAlloced( 2 );
	data[ 0 ] = b ? '1' : '0';
	data[ 1 ] = '\0';
	len = 1;
}

ID_INLINE idStr::idStr( const char c )
{
	Construct();
	EnsureAlloced( 2 );
	data[ 0 ] = c;
	data[ 1 ] = '\0';
	len = 1;
}

ID_INLINE idStr::idStr( const int i )
{
	Construct();
	char text[ 64 ];
	int l;

	l = sprintf( text, "%d", i );
	EnsureAlloced( l + 1 );
	strcpy( data, text );
	len = l;
}

ID_INLINE idStr::idStr( const unsigned u )
{
	Construct();
	char text[ 64 ];
	int l = sprintf( text, "%u", u );
	EnsureAlloced( l + 1 );
	strcpy( data, text );
	len = l;
}

ID_INLINE idStr::idStr( const float f )
{
	Construct();
	char text[ 64 ];
	int l = idStr::snPrintf( text, sizeof( text ), "%f", f );
	while( l > 0 && text[l - 1] == '0' ) text[--l] = '\0';
	while( l > 0 && text[l - 1] == '.' ) text[--l] = '\0';
	EnsureAlloced( l + 1 );
	strcpy( data, text );
	len = l;
}

ID_INLINE idStr::~idStr()
{
	FreeData();
}

ID_INLINE size_t idStr::Size() const
{
	return sizeof( *this ) + Allocated();
}

ID_INLINE const char* idStr::c_str() const
{
	return data;
}

ID_INLINE idStr::operator const char* ()
{
	return c_str();
}

ID_INLINE idStr::operator const char* () const
{
	return c_str();
}

ID_INLINE char idStr::operator[]( int index ) const
{
	assert( ( index >= 0 ) && ( index <= len ) );
	return data[ index ];
}

ID_INLINE char& idStr::operator[]( int index )
{
	assert( ( index >= 0 ) && ( index <= len ) );
	return data[ index ];
}

ID_INLINE void idStr::operator=( const idStr& text )
{
	int l = text.Length();
	EnsureAlloced( l + 1, false );
	memcpy( data, text.data, l );
	data[l] = '\0';
	len = l;
}

ID_INLINE idStr operator+( const idStr& a, const idStr& b )
{
	idStr result( a );
	result.Append( b );
	return result;
}

ID_INLINE idStr operator+( const idStr& a, const char* b )
{
	idStr result( a );
	result.Append( b );
	return result;
}

ID_INLINE idStr operator+( const char* a, const idStr& b )
{
	idStr result( a );
	result.Append( b );
	return result;
}

ID_INLINE idStr operator+( const idStr& a, const bool b )
{
	idStr result( a );
	result.Append( b ? "true" : "false" );
	return result;
}

ID_INLINE idStr operator+( const idStr& a, const char b )
{
	idStr result( a );
	result.Append( b );
	return result;
}

ID_INLINE idStr operator+( const idStr& a, const float b )
{
	char	text[ 64 ];
	idStr	result( a );

	sprintf( text, "%f", b );
	result.Append( text );

	return result;
}

ID_INLINE idStr operator+( const idStr& a, const int b )
{
	char	text[ 64 ];
	idStr	result( a );

	sprintf( text, "%d", b );
	result.Append( text );

	return result;
}

ID_INLINE idStr operator+( const idStr& a, const unsigned b )
{
	char	text[ 64 ];
	idStr	result( a );

	sprintf( text, "%u", b );
	result.Append( text );

	return result;
}

ID_INLINE idStr& idStr::operator+=( const float a )
{
	char text[ 64 ];

	sprintf( text, "%f", a );
	Append( text );

	return *this;
}

ID_INLINE idStr& idStr::operator+=( const int a )
{
	char text[ 64 ];

	sprintf( text, "%d", a );
	Append( text );

	return *this;
}

ID_INLINE idStr& idStr::operator+=( const unsigned a )
{
	char text[ 64 ];

	sprintf( text, "%u", a );
	Append( text );

	return *this;
}

ID_INLINE idStr& idStr::operator+=( const idStr& a )
{
	Append( a );
	return *this;
}

ID_INLINE idStr& idStr::operator+=( const char* a )
{
	Append( a );
	return *this;
}

ID_INLINE idStr& idStr::operator+=( const char a )
{
	Append( a );
	return *this;
}

ID_INLINE idStr& idStr::operator+=( const bool a )
{
	Append( a ? "true" : "false" );
	return *this;
}

ID_INLINE bool operator==( const idStr& a, const idStr& b )
{
	return ( !idStr::Cmp( a.data, b.data ) );
}

ID_INLINE bool operator==( const idStr& a, const char* b )
{
	assert( b );
	return ( !idStr::Cmp( a.data, b ) );
}

ID_INLINE bool operator==( const char* a, const idStr& b )
{
	assert( a );
	return ( !idStr::Cmp( a, b.data ) );
}

ID_INLINE bool operator!=( const idStr& a, const idStr& b )
{
	return !( a == b );
}

ID_INLINE bool operator!=( const idStr& a, const char* b )
{
	return !( a == b );
}

ID_INLINE bool operator!=( const char* a, const idStr& b )
{
	return !( a == b );
}

ID_INLINE int idStr::Cmp( const char* text ) const
{
	assert( text );
	return idStr::Cmp( data, text );
}

ID_INLINE int idStr::Cmpn( const char* text, int n ) const
{
	assert( text );
	return idStr::Cmpn( data, text, n );
}

ID_INLINE int idStr::CmpPrefix( const char* text ) const
{
	assert( text );
	return Cmpn( data, text, Length( text ) );
}

ID_INLINE int idStr::Icmp( const char* text ) const
{
	assert( text );
	return Icmp( data, text );
}

ID_INLINE int idStr::Icmpn( const char* text, int n ) const
{
	assert( text );
	return Icmpn( data, text, n );
}

ID_INLINE int idStr::IcmpPrefix( const char* text ) const
{
	assert( text );
	return Icmpn( data, text, Length( text ) );
}

ID_INLINE int idStr::IcmpNoColor( const char* text ) const
{
	assert( text );
	return IcmpNoColor( data, text );
}

ID_INLINE int idStr::IcmpPath( const char* text ) const
{
	assert( text );
	return IcmpPath( data, text );
}

ID_INLINE int idStr::IcmpnPath( const char* text, int n ) const
{
	assert( text );
	return IcmpnPath( data, text, n );
}

ID_INLINE int idStr::IcmpPrefixPath( const char* text ) const
{
	assert( text );
	return IcmpnPath( data, text, Length( text ) );
}

ID_INLINE int idStr::Length() const
{
	return len;
}

ID_INLINE int idStr::Allocated() const
{
	return ( data != baseBuffer )? GetAlloced() : 0;
}

ID_INLINE void idStr::Empty()
{
	EnsureAlloced( 1 );
	data[ 0 ] = '\0';
	len = 0;
}

ID_INLINE bool idStr::IsEmpty() const
{
	return( idStr::Cmp( data, "" ) == 0 );
}

ID_INLINE void idStr::Clear()
{
	if( IsStatic() )
	{
		len = 0;
		data[ 0 ] = '\0';
		return;
	}
	FreeData();
	Construct();
}

ID_INLINE void idStr::Append( const char a )
{
	EnsureAlloced( len + 2 );
	data[ len ] = a;
	len++;
	data[ len ] = '\0';
}

ID_INLINE void idStr::Append( const idStr& text )
{
	int newLen = len + text.Length();
	EnsureAlloced( newLen + 1 );
	for( int i = 0; i < text.len; ++i )
	{
		data[ len + i ] = text[ i ];
	}
	len = newLen;
	data[ len ] = '\0';
}

ID_INLINE void idStr::Append( const char* text )
{
	if( text )
	{
		int newLen = len + Length( text );
		EnsureAlloced( newLen + 1 );
		for( int i = 0; text[ i ]; ++i )
		{
			data[ len + i ] = text[ i ];
		}
		len = newLen;
		data[ len ] = '\0';
	}
}

ID_INLINE void idStr::Append( const char* text, int l )
{
	if( text && l )
	{
		int newLen = len + l;
		EnsureAlloced( newLen + 1 );
		for( int i = 0; text[ i ] && i < l; ++i )
		{
			data[ len + i ] = text[ i ];
		}
		len = newLen;
		data[ len ] = '\0';
	}
}

ID_INLINE void idStr::Insert( const char a, int index )
{
	if( index < 0 )
	{
		index = 0;
	}
	else if( index > len )
	{
		index = len;
	}

	int l = 1;
	EnsureAlloced( len + l + 1 );
	for( int i = len; i >= index; --i )
	{
		data[i + l] = data[i];
	}
	data[index] = a;
	++len;
}

ID_INLINE void idStr::Insert( const char* text, int index )
{
	if( index < 0 )
	{
		index = 0;
	}
	else if( index > len )
	{
		index = len;
	}

	int l = Length( text );
	EnsureAlloced( len + l + 1 );
	for( int i = len; i >= index; --i )
	{
		data[i + l] = data[i];
	}
	for( int i = 0; i < l; ++i )
	{
		data[index + i] = text[i];
	}
	len += l;
}

ID_INLINE void idStr::ToLower()
{
	for( int i = 0; data[i]; ++i )
	{
		if( CharIsUpper( data[i] ) )
		{
			data[i] += ( 'a' - 'A' );
		}
	}
}

ID_INLINE void idStr::ToUpper()
{
	for( int i = 0; data[i]; ++i )
	{
		if( CharIsLower( data[i] ) )
		{
			data[i] -= ( 'a' - 'A' );
		}
	}
}

ID_INLINE bool idStr::IsNumeric() const
{
	return IsNumeric( data );
}

ID_INLINE bool idStr::IsColor() const
{
	return IsColor( data );
}

ID_INLINE bool idStr::IsHexColor( void ) const {
	return IsHexColor( data );
}

ID_INLINE bool idStr::HasHexColorAlpha( void ) const {
	return HasHexColorAlpha( data );
}

ID_INLINE bool idStr::HasLower() const
{
	return HasLower( data );
}

ID_INLINE bool idStr::HasUpper() const
{
	return HasUpper( data );
}

ID_INLINE idStr& idStr::RemoveColors()
{
	RemoveColors( data );
	len = Length( data );
	return *this;
}

ID_INLINE int idStr::LengthWithoutColors() const
{
	return LengthWithoutColors( data );
}

ID_INLINE void idStr::CapLength( int newlen )
{
	if( len <= newlen )
	{
		return;
	}
	data[ newlen ] = '\0';
	len = newlen;
}

ID_INLINE void idStr::Fill( const char ch, int newlen )
{
	EnsureAlloced( newlen + 1 );
	len = newlen;
	memset( data, ch, len );
	data[ len ] = '\0';
}

ID_INLINE const char* idStr::FindString( const char* text, bool casesensitive, int start, int end ) const
{
	if ( end == INVALID_POSITION ) {
		end = len;
	}

	int i = FindText( data, text, casesensitive, start, end );
	if ( i == INVALID_POSITION ) {
		return NULL;
	}

	return &data[ i ];
}

ID_INLINE const char* idStr::FindString( const char *str, const char *text, bool casesensitive, int start, int end )
{
	if ( end == INVALID_POSITION ) {
		end = idStr::Length( str );
	}

	int i = FindText( str, text, casesensitive, start, end );
	if ( i == INVALID_POSITION ) {
		return NULL;
	}

	return &str[ i ];
}

/*
========================
idStr::UTF8Length
========================
*/
ID_INLINE int idStr::UTF8Length()
{
	return UTF8Length( ( byte* )data );
}

/*
========================
idStr::UTF8Char
========================
*/
ID_INLINE uint32 idStr::UTF8Char( int& idx )
{
	return UTF8Char( ( byte* )data, idx );
}

/*
========================
idStr::ConvertToUTF8
========================
*/
ID_INLINE void idStr::ConvertToUTF8()
{
	idStr temp( *this );
	Clear();
	for( int index = 0; index < temp.Length(); ++index )
	{
		AppendUTF8Char( temp[index] );
	}
}

/*
========================
idStr::UTF8Char
========================
*/
ID_INLINE uint32 idStr::UTF8Char( const char* s, int& idx )
{
	return UTF8Char( ( byte* )s, idx );
}

/*
========================
idStr::IsValidUTF8
========================
*/
ID_INLINE bool idStr::IsValidUTF8( const uint8* s, const int maxLen )
{
	utf8Encoding_t encoding;
	return IsValidUTF8( s, maxLen, encoding );
}

ID_INLINE int idStr::Find( const char c, int start, int end ) const
{
	if( end == -1 )
	{
		end = len;
	}
	return idStr::FindChar( data, c, start, end );
}

ID_INLINE int idStr::Find( const char* text, bool casesensitive, int start, int end ) const
{
	if( end == -1 )
	{
		end = len;
	}
	return idStr::FindText( data, text, casesensitive, start, end );
}

ID_INLINE bool idStr::Filter( const char* filter, bool casesensitive ) const
{
	return idStr::Filter( filter, data, casesensitive );
}

ID_INLINE const char* idStr::Left( int len, idStr& result ) const
{
	return Mid( 0, len, result );
}

ID_INLINE const char* idStr::Right( int len, idStr& result ) const
{
	if( len >= Length() )
	{
		result = *this;
		return result;
	}
	return Mid( Length() - len, len, result );
}

ID_INLINE idStr idStr::Left( int len ) const
{
	return Mid( 0, len );
}

ID_INLINE idStr idStr::Right( int len ) const
{
	if( len >= Length() )
	{
		return *this;
	}
	return Mid( Length() - len, len );
}

ID_INLINE void idStr::Strip( const char c )
{
	StripLeading( c );
	StripTrailing( c );
}

ID_INLINE void idStr::Strip( const char* string )
{
	StripLeading( string );
	StripTrailing( string );
}

ID_INLINE bool idStr::CheckExtension( const char* ext )
{
	return idStr::CheckExtension( data, ext );
}

ID_INLINE idStr& idStr::CleanFilename()
{
	CleanFilename( data );
	len = Length( data );
	return *this;
}

ID_INLINE int idStr::Length( const char* s )
{
	int i;
	for( i = 0; s[i]; ++i ) {}
	return i;
}

ID_INLINE char* idStr::ToLower( char* s )
{
	for( int i = 0; s[i]; ++i )
	{
		if( CharIsUpper( s[i] ) )
		{
			s[i] += ( 'a' - 'A' );
		}
	}
	return s;
}

ID_INLINE char* idStr::ToUpper( char* s )
{
	for( int i = 0; s[i]; ++i )
	{
		if( CharIsLower( s[i] ) )
		{
			s[i] -= ( 'a' - 'A' );
		}
	}
	return s;
}

ID_INLINE int idStr::Hash( const char* string )
{
	int i, hash = 0;
	for( i = 0; *string != '\0'; ++i )
	{
		hash += ( *string++ ) * ( i + 119 );
	}
	return hash;
}

ID_INLINE int idStr::Hash( const char* string, int length )
{
	int i, hash = 0;
	for( i = 0; i < length; ++i )
	{
		hash += ( *string++ ) * ( i + 119 );
	}
	return hash;
}

ID_INLINE int idStr::IHash( const char* string )
{
	int i, hash = 0;
	for( i = 0; *string != '\0'; ++i )
	{
		hash += ToLower( *string++ ) * ( i + 119 );
	}
	return hash;
}

ID_INLINE int idStr::IHash( const char* string, int length )
{
	int i, hash = 0;
	for( i = 0; i < length; ++i )
	{
		hash += ToLower( *string++ ) * ( i + 119 );
	}
	return hash;
}

ID_INLINE int idStr::FileNameHash( const int hashSize ) const
{
	return FileNameHash( data, hashSize );
}

ID_INLINE bool idStr::IsColor( const char* s )
{
	return ( s[0] == C_COLOR_ESCAPE && s[1] != '\0' && s[1] != ' ' );
}

ID_INLINE bool idStr::IsHexColor( const char *s )
{
	int i;
	for ( i = 0; s[i] && i < 6; ++i ) {
		if( !CharIsHex( s[i] ) ) {
			return false;
		}
	}
	return( i == 6 );
}

ID_INLINE bool idStr::HasHexColorAlpha( const char *s )
{
	int i;
	for ( i = 6; s[i] && i < 8; ++i ) {
		if( !CharIsHex( s[i] ) ) {
			return false;
		}
	}
	return( i == 8 );
}

ID_INLINE char idStr::ToLower( char c )
{
	if( c <= 'Z' && c >= 'A' )
	{
		return ( c + ( 'a' - 'A' ) );
	}
	return c;
}

ID_INLINE char idStr::ToUpper( char c )
{
	if( c >= 'a' && c <= 'z' )
	{
		return ( c - ( 'a' - 'A' ) );
	}
	return c;
}

ID_INLINE bool idStr::CharIsPrintable( int c )
{
	// test for regular ascii and western European high-ascii chars
	return ( c >= 0x20 && c <= 0x7E ) || ( c >= 0xA1 && c <= 0xFF );
}

ID_INLINE bool idStr::CharIsLower( int c )
{
	// test for regular ascii and western European high-ascii chars
	return ( c >= 'a' && c <= 'z' ) || ( c >= 0xE0 && c <= 0xFF );
}

ID_INLINE bool idStr::CharIsUpper( int c )
{
	// test for regular ascii and western European high-ascii chars
	return ( c <= 'Z' && c >= 'A' ) || ( c >= 0xC0 && c <= 0xDF );
}

ID_INLINE bool idStr::CharIsAlpha( int c )
{
	// test for regular ascii and western European high-ascii chars
	return ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= 0xC0 && c <= 0xFF ) );
}

ID_INLINE bool idStr::CharIsNumeric( int c )
{
	return ( c <= '9' && c >= '0' );
}

ID_INLINE bool idStr::CharIsNewLine( char c )
{
	return ( c == '\n' || c == '\r' || c == '\v' );
}

ID_INLINE bool idStr::CharIsTab( char c )
{
	return ( c == '\t' );
}

ID_INLINE bool idStr::CharIsHex( int c ) {
	return ( ( c >= '0' && c <= '9' ) || ( c >= 'A' && c <= 'F' ) || ( c >= 'a' && c <= 'f' ) );
}

ID_INLINE int idStr::ColorIndex( int c )
{
	//return ( c & COLOR_BITS );
	return ( ( c - '0' ) & COLOR_BITS );
}

ID_INLINE int idStr::HexForChar( int c ) {
	return ( c > '9' ? ( c >= 'a' ? ( c - 'a' + 10 ) : ( c - '7' ) ) : ( c - '0' ) );
}

ID_INLINE int idStr::CountChar( const char c )
{
	return CountChar( data, c );
}

ID_INLINE bool idStr::IsValidEmailAddress()
{
	return IsValidEmailAddress( data );
}

ID_INLINE int idStr::DynamicMemoryUsed() const
{
	return ( data == baseBuffer ) ? 0 : GetAlloced();
}

ID_INLINE void idStr::CopyRange( const char* text, int start, int end )
{
	int l = end - start;
	if( l < 0 ) l = 0;

	EnsureAlloced( l + 1 );

	for( int i = 0; i < l; ++i ) {
		data[ i ] = text[ start + i ];
	}
	data[ l ] = '\0';
	len = l;
}
/*
ID_INLINE void idStr::Swap( idStr& rhs )
{
	if( rhs.data != rhs.baseBuffer && data != baseBuffer )
	{
		SwapValues( data, rhs.data );
		SwapValues( len, rhs.len );
		SwapValues( alloced, rhs.alloced );
	}
	else
	{
		SwapValues( *this, rhs );
	}
}*/

#endif /* !__STR_H__ */
