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

#include "precompiled.h"
#pragma hdrstop

#ifdef USE_STRING_DATA_ALLOCATOR
static idDynamicBlockAlloc < char, 1 << 18, 128, TAG_STRING >	stringDataAllocator;
#endif

idStr::hmsFormat_t	idStr::defaultHMSFormat;

struct strColor_t {
	ALIGNTYPE16 idVec4 color;
	const char* str;
} g_color_table[] = {
	{	idVec4( 0.0f,  0.0f,  0.0f,  1.0f ), "^0" },			// 0 - S_COLOR_DEFAULT			0
	{	idVec4( 1.0f,  0.0f,  0.0f,  1.0f ), "^1" }, 			// 1 - S_COLOR_RED				1
	{	idVec4( 0.0f,  1.0f,  0.0f,  1.0f ), "^2" }, 			// 2 - S_COLOR_GREEN			2
	{	idVec4( 1.0f,  1.0f,  0.0f,  1.0f ), "^3" }, 			// 3 - S_COLOR_YELLOW			3
	{	idVec4( 0.0f,  0.0f,  1.0f,  1.0f ), "^4" }, 			// 4 - S_COLOR_BLUE				4
	{	idVec4( 0.0f,  1.0f,  1.0f,  1.0f ), "^5" }, 			// 5 - S_COLOR_CYAN				5
	{   idVec4( 1.0f,  0.5f,  0.0f,  1.0f ), "^6" },			// 6 - S_COLOR_ORANGE			6
	{	idVec4( 1.0f,  1.0f,  1.0f,  1.0f ), "^7" }, 			// 7 - S_COLOR_WHITE			7
	{	idVec4( 0.5f,  0.5f,  0.5f,  1.0f ), "^8" }, 			// 8 - S_COLOR_GRAY				8
	{	idVec4( 0.0f,  0.0f,  0.0f,  1.0f ), "^9" }, 			// 9 - S_COLOR_BLACK			9
	{	idVec4( 0.75f, 0.75f, 0.75f, 1.0f ), "^:" }, 			// : - S_COLOR_LT_GREY			10
	{	idVec4( 0.25f, 0.25f, 0.25f, 1.0f ), "^;" }, 			// ; - S_COLOR_DK_GREY			11
	{	idVec4( 0.0f,  0.5f,  0.0f,  1.0f ), "^<" }, 			// < - S_COLOR_MD_GREEN			12
	{	idVec4( 0.5f,  0.5f,  0.0f,  1.0f ), "^=" }, 			// = - S_COLOR_MD_YELLOW		13
	{	idVec4( 0.0f,  0.0f,  0.5f,  1.0f ), "^>" }, 			// > - S_COLOR_MD_BLUE			14
	{	idVec4( 0.5f,  0.0f,  0.0f,  1.0f ), "^?" }, 			// ? - S_COLOR_MD_RED			15
#if 0 //SEA: Console colors gone mad with this :(	TOFIX!
	{	idVec4( 0.5f,  0.25f, 0.0f,  1.0f ), "^@" }, 			// @ - md.orange				16
	{	idVec4( 1.0f,  0.6f,  0.1f,  1.0f ), "^A" }, 			// A - lt.orange				17
	{	idVec4( 0.0f,  0.5f,  0.5f,  1.0f ), "^B" }, 			// B - md.cyan					18
	{	idVec4( 0.5f,  0.0f,  0.5f,  1.0f ), "^C" }, 			// C - md.purple				19
	{	idVec4( 1.0f,  0.0f,  1.0f,  1.0f ), "^D" }, 			// D - orange					20
	{	idVec4( 0.5f,  0.0f,  1.0f,  1.0f ), "^E" }, 			// E							21
	{	idVec4( 0.2f,  0.6f,  0.8f,  1.0f ), "^F" }, 			// F							22
	{	idVec4( 0.8f,  1.0f,  0.8f,  1.0f ), "^G" }, 			// G							23
	{	idVec4( 0.0f,  0.4f,  0.2f,  1.0f ), "^H" }, 			// H							24
	{	idVec4( 1.0f,  0.0f,  0.2f,  1.0f ), "^I" }, 			// I							25
	{	idVec4( 0.7f,  0.1f,  0.1f,  1.0f ), "^J" }, 			// J							26
	{	idVec4( 0.6f,  0.2f,  0.0f,  1.0f ), "^K" }, 			// K							27
	{	idVec4( 0.8f,  0.6f,  0.2f,  1.0f ), "^L" }, 			// L							28
	{	idVec4( 0.6f,  0.6f,  0.2f,  1.0f ), "^M" }, 			// M							29
	{	idVec4( 1.0f,  1.0f,  0.75f, 1.0f ), "^N" }, 			// N							30
	{	idVec4( 1.0f,  1.0f,  0.5f,  1.0f ), "^O" }, 			// O							31
#endif
};

static const int COLOR_BITS = _countof( g_color_table ) - 1;

const char* units[2][4] =
{
	{ "B", "KB", "MB", "GB" },
	{ "B/s", "KB/s", "MB/s", "GB/s" }
};

/*
============
idStr::ColorForIndex
============
*/
const idVec4 & idStr::ColorForIndex( int i )
{
	return g_color_table[ i & COLOR_BITS ].color;
}

/*
============
idStr::ColorForChar
============
*/
const idVec4 & idStr::ColorForChar( int c )
{
	return g_color_table[ ColorIndex( c ) ].color;
}

/*
============
idStr::StrForColorIndex
============
*/
const char* idStr::StrForColorIndex( int i )
{
	return g_color_table[ i & COLOR_BITS ].str;
}

/*
============
idStr::ReAllocate
============
*/
void idStr::ReAllocate( int amount, bool keepold )
{
	char*	newbuffer;
	int		newsize;
	int		mod;
	
	//assert( data );
	assert( amount > 0 );
	
	mod = amount % STR_ALLOC_GRAN;
	if( !mod )
	{
		newsize = amount;
	}
	else
	{
		newsize = amount + STR_ALLOC_GRAN - mod;
	}
	SetAlloced( newsize );
	
#ifdef USE_STRING_DATA_ALLOCATOR
	newbuffer = stringDataAllocator.Alloc( GetAlloced() );
#else
	newbuffer = new( TAG_STRING ) char[ GetAlloced() ];
#endif
	if( keepold && data )
	{
		data[ len ] = '\0';
		strcpy( newbuffer, data );
	}
	
	if( data && data != baseBuffer )
	{
#ifdef USE_STRING_DATA_ALLOCATOR
		stringDataAllocator.Free( data );
#else
		delete [] data;
#endif
	}
	
	data = newbuffer;
}

/*
============
idStr::FreeData
============
*/
void idStr::FreeData()
{
	if( IsStatic() )
	{
		return;
	}
	
	if( data && data != baseBuffer )
	{
#ifdef USE_STRING_DATA_ALLOCATOR
		stringDataAllocator.Free( data );
#else
		delete[] data;
#endif
		data = baseBuffer;
	}
}

/*
============
idStr::operator=
============
*/
void idStr::operator=( const char* text )
{	
	if( !text )
	{
		// safe behavior if NULL
		EnsureAlloced( 1, false );
		data[ 0 ] = '\0';
		len = 0;
		return;
	}
	
	if( text == data )
	{
		return; // copying same thing
	}
	
	// check if we're aliasing
	if( text >= data && text <= data + len )
	{
		int diff = text - data;
		
		assert( idStr::Length( text ) < (int)len );
		
		int i;
		for( i = 0; text[ i ]; i++ ) {
			data[ i ] = text[ i ];
		} data[ i ] = '\0';
		
		len -= diff;
		
		return;
	}
	
	int l = Length( text );
	EnsureAlloced( l + 1, false );
	strcpy( data, text );
	len = l;
}

/*
============
idStr::FindChar

returns INVALID_POSITION if not found otherwise the index of the char
============
*/
int idStr::FindChar( const char* str, const char c, int start, int end )
{
	if( end == INVALID_POSITION )
	{
		end = Length( str ) - 1;
	}
	for( int i = start; i <= end; i++ )
	{
		if( str[i] == c )
		{
			return i;
		}
	}
	return INVALID_POSITION;
}

/*
============
idStr::FindText

returns INVALID_POSITION if not found otherwise the index of the text
============
*/
int idStr::FindText( const char* str, const char* text, bool casesensitive, int start, int end )
{
	int l, i, j;
	
	if( end == INVALID_POSITION )
	{
		end = Length( str );
	}
	l = end - Length( text );
	
	for( i = start; i <= l; i++ )
	{
		if( casesensitive )
		{
			for( j = 0; text[j]; j++ )
			{
				if( str[i + j] != text[j] )
				{
					break;
				}
			}
		}
		else
		{
			for( j = 0; text[j]; j++ )
			{
				if( ::toupper( str[i + j] ) != ::toupper( text[j] ) )
				{
					break;
				}
			}
		}
		if( !text[j] )
		{
			return i;
		}
	}
	return INVALID_POSITION;
}

/*
============
idStr::CountChar
============
*/
int idStr::CountChar( const char *str, const char c )
{
	int count = 0;
	for ( int i = 0; str[i] != '\0'; i++ ) {
		if ( str[i] == c ) {
			count++;
		}
	}
	return count;
}

/*
============
idStr::Filter

Returns true if the string conforms the given filter.
Several metacharacter may be used in the filter.

*          match any string of zero or more characters
?          match any single character
[abc...]   match any of the enclosed characters; a hyphen can
		   be used to specify a range (e.g. a-z, A-Z, 0-9)

============
*/
bool idStr::Filter( const char* filter, const char* name, bool casesensitive )
{
	idStr buf;
	int i, found, index;
	
	while( *filter )
	{
		if( *filter == '*' )
		{
			filter++;
			buf.Empty();
			for( i = 0; *filter; i++ )
			{
				if( *filter == '*' || *filter == '?' || ( *filter == '[' && *( filter + 1 ) != '[' ) )
				{
					break;
				}
				buf += *filter;
				if( *filter == '[' )
				{
					filter++;
				}
				filter++;
			}
			if( buf.Length() )
			{
				index = idStr( name ).Find( buf.c_str(), casesensitive );
				if( index == INVALID_POSITION )
				{
					return false;
				}
				name += index + idStr::Length( buf );
			}
		}
		else if( *filter == '?' )
		{
			filter++;
			name++;
		}
		else if( *filter == '[' )
		{
			if( *( filter + 1 ) == '[' )
			{
				if( *name != '[' )
				{
					return false;
				}
				filter += 2;
				name++;
			}
			else
			{
				filter++;
				found = false;
				while( *filter && !found )
				{
					if( *filter == ']' && *( filter + 1 ) != ']' )
					{
						break;
					}
					if( *( filter + 1 ) == '-' && *( filter + 2 ) && ( *( filter + 2 ) != ']' || *( filter + 3 ) == ']' ) )
					{
						if( casesensitive )
						{
							if( *name >= *filter && *name <= *( filter + 2 ) )
							{
								found = true;
							}
						}
						else
						{
							if( ::toupper( *name ) >= ::toupper( *filter ) && ::toupper( *name ) <= ::toupper( *( filter + 2 ) ) )
							{
								found = true;
							}
						}
						filter += 3;
					}
					else
					{
						if( casesensitive )
						{
							if( *filter == *name )
							{
								found = true;
							}
						}
						else
						{
							if( ::toupper( *filter ) == ::toupper( *name ) )
							{
								found = true;
							}
						}
						filter++;
					}
				}
				if( !found )
				{
					return false;
				}
				while( *filter )
				{
					if( *filter == ']' && *( filter + 1 ) != ']' )
					{
						break;
					}
					filter++;
				}
				filter++;
				name++;
			}
		}
		else
		{
			if( casesensitive )
			{
				if( *filter != *name )
				{
					return false;
				}
			}
			else
			{
				if( ::toupper( *filter ) != ::toupper( *name ) )
				{
					return false;
				}
			}
			filter++;
			name++;
		}
	}
	return true;
}

/*
=============
idStr::StripMediaName

  makes the string lower case, replaces backslashes with forward slashes, and removes extension
=============
*/
void idStr::StripMediaName( const char* name, idStr& mediaName )
{
	char c;
	
	mediaName.Empty();
	
	for( c = *name; c; c = *( ++name ) )
	{
		// truncate at an extension
		if( c == '.' )
		{
			break;
		}
		// convert backslashes to forward slashes
		if( c == '\\' )
		{
			mediaName.Append( '/' );
		}
		else
		{
			mediaName.Append( ToLower( c ) );
		}
	}
}

/*
=============
idStr::CheckExtension
=============
*/
bool idStr::CheckExtension( const char* name, const char* ext )
{
	const char* s1 = name + Length( name ) - 1;
	const char* s2 = ext + Length( ext ) - 1;
	int c1, c2, d;
	
	do
	{
		c1 = *s1--;
		c2 = *s2--;
		
		d = c1 - c2;
		while( d )
		{
			if( c1 <= 'Z' && c1 >= 'A' )
			{
				d += ( 'a' - 'A' );
				if( !d )
				{
					break;
				}
			}
			if( c2 <= 'Z' && c2 >= 'A' )
			{
				d -= ( 'a' - 'A' );
				if( !d )
				{
					break;
				}
			}
			return false;
		}
	}
	while( s1 > name && s2 > ext );
	
	return ( s1 >= name );
}

/*
=============
idStr::FloatArrayToString
=============
*/
const char* idStr::FloatArrayToString( const float* array, const int length, const int precision )
{
	static int index = 0;
	static char str[4][16384];	// in case called by nested functions
	int i, n;
	char format[16], *s;
	
	// use an array of string so that multiple calls won't collide
	s = str[ index ];
	index = ( index + 1 ) & 3;
	
	snPrintf( format, sizeof( format ), "%%.%df", precision );
	n = snPrintf( s, sizeof( str[0] ), format, array[0] );
	if( precision > 0 )
	{
		while( n > 0 && s[n - 1] == '0' ) s[--n] = '\0';
		while( n > 0 && s[n - 1] == '.' ) s[--n] = '\0';
	}
	snPrintf( format, sizeof( format ), " %%.%df", precision );
	for( i = 1; i < length; i++ )
	{
		n += snPrintf( s + n, sizeof( str[0] ) - n, format, array[i] );
		if( precision > 0 )
		{
			while( n > 0 && s[n - 1] == '0' ) s[--n] = '\0';
			while( n > 0 && s[n - 1] == '.' ) s[--n] = '\0';
		}
	}
	return s;
}

/*
========================
idStr::CStyleQuote
========================
*/
const char* idStr::CStyleQuote( const char* str )
{
	static int index = 0;
	static char buffers[4][16384];	// in case called by nested functions
	unsigned int i;
	char* buf;
	
	buf = buffers[index];
	index = ( index + 1 ) & 3;
	
	buf[0] = '\"';
	for( i = 1; i < sizeof( buffers[0] ) - 2; i++ )
	{
		int c = *str++;
		switch( c ) {
			case '\0': buf[i++] = '\"'; buf[i] = '\0'; return buf;
			case '\\': buf[i++] = '\\'; buf[i] = '\\'; break;
			case '\n': buf[i++] = '\\'; buf[i] = 'n'; break;
			case '\r': buf[i++] = '\\'; buf[i] = 'r'; break;
			case '\t': buf[i++] = '\\'; buf[i] = 't'; break;
			case '\v': buf[i++] = '\\'; buf[i] = 'v'; break;
			case '\b': buf[i++] = '\\'; buf[i] = 'b'; break;
			case '\f': buf[i++] = '\\'; buf[i] = 'f'; break;
			case '\a': buf[i++] = '\\'; buf[i] = 'a'; break;
			case '\'': buf[i++] = '\\'; buf[i] = '\''; break;
			case '\"': buf[i++] = '\\'; buf[i] = '\"'; break;
			case '\?': buf[i++] = '\\'; buf[i] = '\?'; break;
			default: buf[i] = c; break;
		}
	}
	buf[i++] = '\"';
	buf[i] = '\0';
	return buf;
}

/*
===============
idStr::CStyleUnQuote
========================
*/
const char* idStr::CStyleUnQuote( const char* str )
{
	if( str[0] != '\"' )
	{
		return str;
	}
	
	static int index = 0;
	static char buffers[4][16384];	// in case called by nested functions
	unsigned int i;
	char* buf;
	
	buf = buffers[index];
	index = ( index + 1 ) & 3;
	
	str++;
	for( i = 0; i < sizeof( buffers[0] ) - 1; i++ )
	{
		int c = *str++;
		if ( c == '\0' ) {
			break;
		}
		else if ( c == '\\' ) 
		{
			c = *str++;
			switch( c ) {
				case '\\': buf[i] = '\\'; break;
				case 'n': buf[i] = '\n'; break;
				case 'r': buf[i] = '\r'; break;
				case 't': buf[i] = '\t'; break;
				case 'v': buf[i] = '\v'; break;
				case 'b': buf[i] = '\b'; break;
				case 'f': buf[i] = '\f'; break;
				case 'a': buf[i] = '\a'; break;
				case '\'': buf[i] = '\''; break;
				case '\"': buf[i] = '\"'; break;
				case '\?': buf[i] = '\?'; break;
			}
		} 
		else {
			buf[i] = c;
		}
	}
	assert( buf[i - 1] == '\"' );
	buf[i - 1] = '\0';
	return buf;
}

/*
============
idStr::Last

returns INVALID_POSITION if not found otherwise the index of the char
============
*/
int idStr::Last( const char c, int index ) const 
{
	if( index == INVALID_POSITION ) {
		index = Length();
	}
	for( ; index >= 0; index-- ) {
		if ( data[ index ] == c ) {
			return index;
		}
	}
	return INVALID_POSITION;
}

/*
============
idStr::Last

returns INVALID_POSITION if not found otherwise the index of the string
============
*/
int idStr::Last( const char* str, bool casesensitive, int index ) const
{
	if( index == INVALID_POSITION ) {
		index = Length();
	}
	int searchLength = Length( str ); 
	if( len - index > searchLength ) {
		index -= searchLength;
	}
	for( ; index >= 0; index-- ) {
		if( ( casesensitive && Cmpn( &data[ index ], str, searchLength ) == 0 ) ||
			( !casesensitive && Icmpn( &data[ index ], str, searchLength ) == 0 ))
		{
			return index;
		}
	}
	return INVALID_POSITION;
}

/*
========================
idStr::Format

perform a threadsafe sprintf to the string
========================
*/
void idStr::Format( const char* fmt, ... )
{
	va_list argptr;
	char text[MAX_PRINT_MSG];
	
	va_start( argptr, fmt );
	int len = idStr::vsnPrintf( text, sizeof( text ) - 1, fmt, argptr );
	va_end( argptr );
	text[ sizeof( text ) - 1 ] = '\0';
	
	if( ( size_t )len >= sizeof( text ) - 1 )
	{
		idLib::common->FatalError( "Tried to set a large buffer using %s", fmt );
	}
	*this = text;
}

/*
========================
idStr::FormatInt

Formats integers with commas for readability.
========================
*/
idStr idStr::FormatInt( const int num, bool isCash )
{
	idStr val = va( "%d", num );
	int len = val.Length();
	for( int i = 0 ; i < ( ( len - 1 ) / 3 ); i++ )
	{
		int pos = val.Length() - ( ( i + 1 ) * 3 + i );
		if( pos > 1 || val[0] != '-' )
		{
			val.Insert( ',', pos );
		}
	}
	
	if( isCash )
	{
		val.Insert( '$', val[0] == '-' ? 1 : 0 );
	}
	
	return val;
}

/*
============
idStr::StripLeading
============
*/
void idStr::StripLeading( const char c )
{
	while( data[ 0 ] == c )
	{
		memmove( &data[ 0 ], &data[ 1 ], len );
		len--;
	}
}

/*
============
idStr::StripLeading
============
*/
void idStr::StripLeading( const char* string )
{
	int l = Length( string );
	if( l > 0 )
	{
		while( !Cmpn( string, l ) )
		{
			memmove( data, data + l, len - l + 1 );
			len -= l;
		}
	}
}

/*
============
idStr::StripLeadingOnce
============
*/
bool idStr::StripLeadingOnce( const char* string )
{
	int l = Length( string );
	if( ( l > 0 ) && !Cmpn( string, l ) )
	{
		memmove( data, data + l, len - l + 1 );
		len -= l;
		return true;
	}
	return false;
}

/*
============
idStr::StripTrailing
============
*/
void idStr::StripTrailing( const char c )
{
	for( int i = Length(); i > 0 && data[ i - 1 ] == c; i-- )
	{
		data[ i - 1 ] = '\0';
		len--;
	}
}

/*
============
idStr::StripLeading
============
*/
void idStr::StripTrailing( const char* string )
{
	int l = Length( string );
	if( l > 0 )
	{
		while( ( len >= l ) && !Cmpn( string, data + len - l, l ) )
		{
			len -= l;
			data[len] = '\0';
		}
	}
}

/*
============
idStr::StripTrailingOnce
============
*/
bool idStr::StripTrailingOnce( const char* string )
{
	int l = Length( string );
	if( ( l > 0 ) && ( len >= l ) && !Cmpn( string, data + len - l, l ) )
	{
		len -= l;
		data[len] = '\0';
		return true;
	}
	return false;
}

/*
============
idStr::Replace
============
*/
bool  idStr::ReplaceChar( const char old, const char nw )
{
	bool replaced = false;
	for( int i = 0; i < Length(); i++ )
	{
		if( data[i] == old )
		{
			data[i] = nw;
			replaced = true;
		}
	}
	return replaced;
}

/*
============
idStr::Replace
============
*/
bool idStr::Replace( const char* old, const char* nw )
{
	if ( Length( old ) == 0 ) {
		return false;
	}
	int oldLen = Length( old );
	int newLen = Length( nw );
	
	// Work out how big the new string will be
	int count = 0;
	for( int i = 0; i < Length(); i++ )
	{
		if( Cmpn( &data[i], old, oldLen ) == 0 )
		{
			count++;
			i += oldLen - 1;
		}
	}
	
	if( count )
	{
		idStr oldString( data );
		
		EnsureAlloced( len + ( ( newLen - oldLen ) * count ) + 2, false );
		
		// Replace the old data with the new data
		int j = 0;
		for( int i = 0; i < oldString.Length(); i++ )
		{
			if( Cmpn( &oldString[i], old, oldLen ) == 0 )
			{
				memcpy( data + j, nw, newLen );
				i += oldLen - 1;
				j += newLen;
			}
			else
			{
				data[j] = oldString[i];
				j++;
			}
		}
		data[j] = '\0';
		len = Length( data );
		return true;
	}
	return false;
}

/*
============
idStr::ReplaceFirst
============
*/
void idStr::ReplaceFirst( const char *old, const char *nw )
{
	if( Length( old ) == 0 ) {
		return;
	}

	int	i;
	bool present;

	int oldLen = Length( old );
	int newLen = Length( nw );

	// Work out how big the new string will be
	present = false;
	for( i = 0; i < len; i++ ) 
	{
		if( !Cmpn( &data[i], old, oldLen ) ) 
		{
			present = true;
			i += oldLen - 1;
			break;
		}
	}

	if( present )
	{
		idStr oldString( data );
		int	j;

		EnsureAlloced( len + ( newLen - oldLen ) + 2, false );

		// Replace the old data with the new data
		for( i = 0, j = 0; i < oldString.Length(); i++ )
		{
			if( !Cmpn( &oldString[i], old, oldLen ) ) 
			{
				memcpy( data + j, nw, newLen );
				i += oldLen;
				j += newLen;
				break;
			} 
			else {
				data[j] = oldString[i];
				j++;
			}
		}
		memcpy( data + j, &oldString[i], oldString.Length() - i );
		data[j + oldString.Length() - i] = '\0';
		len = Length( data );
	}
}

/*
============
idStr::Mid
============
*/
const char* idStr::Mid( int start, int len, idStr& result ) const
{
	assert( &result != this );
	
	result.Empty();
	
	int i = Length();
	if( i == 0 || len <= 0 || start >= i )
	{
		return NULL;
	}
	
	if( start + len >= i )
	{
		len = i - start;
	}
	
	result.Append( &data[ start ], len );
	return result;
}

/*
============
idStr::Mid
============
*/
idStr idStr::Mid( int start, int len ) const
{
	idStr result;
	
	int i = Length();
	if( i == 0 || len <= 0 || start >= i )
	{
		return result;
	}
	
	if( start + len >= i )
	{
		len = i - start;
	}
	
	result.Append( &data[ start ], len );
	return result;
}

/*
============
idStr::StripLeadingWhiteSpace
============
*/
void idStr::StripLeadingWhiteSpace() 
{
	int i;
	
	// cast to unsigned char to prevent stripping off high-ASCII characters
	for ( i = 0; i < Length() && (unsigned char)(data[ i ]) <= ' '; i++ );

	if ( i > 0 && i != Length() ) {
		memmove( data, data + i, len - i + 1 );
		len -= i;
	}
}

/*
============
idStr::StripTrailingWhitespace
============
*/
void idStr::StripTrailingWhitespace()
{
	// cast to unsigned char to prevent stripping off high-ASCII characters
	for( int i = Length(); i > 0 && ( unsigned char )( data[ i - 1 ] ) <= ' '; i-- )
	{
		data[ i - 1 ] = '\0';
		len--;
	}
}

/*
============
idStr::StripQuotes

Removes the quotes from the beginning and end of the string
============
*/
idStr& idStr::StripQuotes()
{
	if( data[0] != '\"' )
	{
		return *this;
	}
	
	// Remove the trailing quote first
	if( data[len - 1] == '\"' )
	{
		data[len - 1] = '\0';
		len--;
	}
	
	// Strip the leading quote now
	len--;
	memmove( &data[ 0 ], &data[ 1 ], len );
	data[len] = '\0';
	
	return *this;
}

/*
=====================================================================

  filename methods

=====================================================================
*/

/*
============
idStr::FileNameHash
============
*/
int idStr::FileNameHash( const char *string, const int hashSize ) 
{
	int hash = 0;
	int i = 0;

	char letter;
	while( string[ i ] != '\0' )
	{
		letter = idStr::ToLower( string[ i ] );
		if( letter == '.' )
		{
			break;				// don't include extension
		}
		if( letter == '\\' )
		{
			letter = '/';
		}
		hash += ( int )letter * ( i + 119 );
		i++;
	}
	hash &= ( hashSize - 1 );
	return hash;
}

/*
============
idStr::BackSlashesToSlashes
============
*/
idStr& idStr::BackSlashesToSlashes()
{
	for( int i = 0; i < len; i++ )
	{
		if( data[ i ] == '\\' )
		{
			data[ i ] = '/';
		}
	}
	return *this;
}

/*
============
idStr::SlashesToBackSlashes
============
*/
idStr& idStr::SlashesToBackSlashes()
{
	for( int i = 0; i < len; i++ )
	{
		if( data[ i ] == '/' )
		{
			data[ i ] = '\\';
		}
	}
	return *this;
}

/*
============
idStr::CollapsePath

	Removes '..' from path and changes backslashes to slashes.

	Example:
	W:/ETQW/base/../code/game/../idlib/../game/Game_local.h

	Becomes:
	W:/ETQW/code/game/Game_local.h
============
*/
idStr &idStr::CollapsePath() 
{
	int i, length = 0;
	for ( i = 0; i < len; i++ ) 
	{
		if ( data[i] == '.' )
		{
			if ( data[i+1] == '.' && ( data[i+2] == '/' || data[i+2] == '\\' ) ) {		//   ../
				if ( length >= 2 && ( data[length-1] == '/' || data[length-1] == '\\' ) ) 
				{
					if ( length == 2 || data[length-2] != '.' || data[length-3] != '.' )
					{
						length--;
						while( length > 0 && data[length-1] != '/' && data[length-1] != '\\' ) 
						{
							length--;
						}
						i += 2;
						continue;
					}
				}
				data[length++] = data[i++];
				data[length++] = data[i++];
				data[length++] = data[i];
			} 
			else if ( data[i+1] == '/' || data[i+1] == '\\' ) {							//	./
				i++;
			}
			else {
				data[length++] = data[i];
			}
		}
		else {
			data[length++] = data[i];
		}
	}
	data[length] = '\0';
	len = length;
	return *this;
}

/*
============
idStr::SetFileExtension
============
*/
idStr& idStr::SetFileExtension( const char* extension )
{
	StripFileExtension();
	if( *extension != '.' )
	{
		Append( '.' );
	}
	Append( extension );
	return *this;
}

/*
============
idStr::StripFileExtension
============
*/
idStr& idStr::StripFileExtension()
{
	for( int i = len - 1; i >= 0; i-- )
	{
		//if ( data[i] == '/' || data[i] == '\\' ) {
		//	break;
		//} ?
		if( data[i] == '.' )
		{
			data[i] = '\0';
			len = i;
			break;
		}
	}
	return *this;
}

/*
============
idStr::StripAbsoluteFileExtension
============
*/
idStr& idStr::StripAbsoluteFileExtension()
{
	for( int i = 0; i < len; i++ )
	{
		if( data[i] == '.' )
		{
			data[i] = '\0';
			len = i;
			break;
		}
	}
	
	return *this;
}

/*
==================
idStr::DefaultFileExtension
==================
*/
idStr& idStr::DefaultFileExtension( const char* extension )
{
	// do nothing if the string already has an extension
	for( int i = len - 1; i >= 0; i-- )
	{
		if( data[i] == '.' )
		{
			return *this;
		}
	}
	if( *extension != '.' )
	{
		Append( '.' );
	}
	Append( extension );
	return *this;
}

/*
==================
idStr::DefaultPath
==================
*/
idStr& idStr::DefaultPath( const char* basepath )
{
	if( ( ( *this )[ 0 ] == '/' ) || ( ( *this )[ 0 ] == '\\' ) )
	{
		// absolute path location
		return *this;
	}
	
	*this = basepath + *this;
	return *this;
}

/*
====================
idStr::AppendPath
====================
*/
void idStr::AppendPath( const char* text )
{
	int i = 0;	
	if( text && text[i] )
	{
		int pos = len;
		EnsureAlloced( len + Length( text ) + 2 );
		
		if( pos )
		{
			if( data[ pos - 1 ] != '/' )
			{
				data[ pos++ ] = '/';
			}
		}
		if( text[i] == '/' )
		{
			i++;
		}
		
		for( ; text[ i ]; i++ )
		{
			if( text[ i ] == '\\' )
			{
				data[ pos++ ] = '/';
			}
			else
			{
				data[ pos++ ] = text[ i ];
			}
		}
		len = pos;
		data[ pos ] = '\0';
	}
}

/*
==================
idStr::StripFilename
==================
*/
idStr& idStr::StripFilename()
{
	int pos = Length() - 1;
	while( ( pos > 0 ) && ( ( *this )[ pos ] != '/' ) && ( ( *this )[ pos ] != '\\' ) )
	{
		pos--;
	}
	
	if( pos < 0 )
	{
		pos = 0;
	}
	
	CapLength( pos );
	return *this;
}

/*
==================
idStr::StripPath
==================
*/
idStr& idStr::StripPath()
{
	int pos = Length();
	while( ( pos > 0 ) && ( ( *this )[ pos - 1 ] != '/' ) && ( ( *this )[ pos - 1 ] != '\\' ) )
	{
		pos--;
	}
	
	*this = Right( Length() - pos );
	return *this;
}

/*
====================
idStr::ExtractFilePath
====================
*/
void idStr::ExtractFilePath( idStr& dest ) const
{
	//
	// back up until a \ or the start
	//
	int pos = Length();
	while( ( pos > 0 ) && ( ( *this )[ pos - 1 ] != '/' ) && ( ( *this )[ pos - 1 ] != '\\' ) )
	{
		pos--;
	}
	
	Left( pos, dest );
}

/*
====================
idStr::ExtractFileName
====================
*/
void idStr::ExtractFileName( idStr& dest ) const
{
	//
	// back up until a \ or the start
	//
	int pos = Length() - 1;
	while( ( pos > 0 ) && ( ( *this )[ pos - 1 ] != '/' ) && ( ( *this )[ pos - 1 ] != '\\' ) )
	{
		pos--;
	}
	
	Right( Length() - pos, dest );
}

/*
====================
idStr::ExtractFileBase
====================
*/
void idStr::ExtractFileBase( idStr& dest ) const
{
	//
	// back up until a \ or the start
	//
	int pos = Length() - 1;
	while( ( pos > 0 ) && ( ( *this )[ pos - 1 ] != '/' ) && ( ( *this )[ pos - 1 ] != '\\' ) )
	{
		pos--;
	}
	
	int start = pos;
	while( ( pos < Length() ) && ( ( *this )[ pos ] != '.' ) )
	{
		pos++;
	}
	
	Mid( start, pos - start, dest );
}

/*
====================
idStr::ExtractFileExtension
====================
*/
void idStr::ExtractFileExtension( idStr& dest ) const
{
	//
	// back up until a . or the start
	//
	int pos = Length() - 1;
	while( ( pos > 0 ) && ( ( *this )[ pos - 1 ] != '.' ) )
	{
		pos--;
	}
	
	if( !pos )
	{
		// no extension
		dest.Empty();
	}
	else
	{
		Right( Length() - pos, dest );
	}
}

/*
============
idStr::MakeNameCanonical
============
*/
void idStr::MakeNameCanonical() 
{
	ToLower();
	BackSlashesToSlashes();
	StripFileExtension();
}

/*
=====================================================================

  char * methods to replace library functions

=====================================================================
*/

/*
============
idStr::IsNumeric

Checks a string to see if it contains only numerical values.
============
*/
bool idStr::IsNumeric( const char* s )
{
	int		i;
	bool	dot;
	
	if( *s == '-' )
	{
		s++;
	}
	
	dot = false;
	for( i = 0; s[i]; i++ )
	{
		if( !isdigit( ( const unsigned char )s[i] ) )
		{
			if( ( s[ i ] == '.' ) && !dot )
			{
				dot = true;
				continue;
			}
			return false;
		}
	}
	
	return true;
}

/*
============
idStr::HasLower

Checks if a string has any lowercase chars
============
*/
bool idStr::HasLower( const char* s )
{
	if( !s )
	{
		return false;
	}
	
	while( *s )
	{
		if( CharIsLower( *s ) )
		{
			return true;
		}
		s++;
	}
	
	return false;
}

/*
============
idStr::HasUpper

Checks if a string has any uppercase chars
============
*/
bool idStr::HasUpper( const char* s )
{
	if( !s )
	{
		return false;
	}
	
	while( *s )
	{
		if( CharIsUpper( *s ) )
		{
			return true;
		}
		s++;
	}
	
	return false;
}

/*
================
idStr::Cmp
================
*/
int idStr::Cmp( const char* s1, const char* s2 )
{
	int c1, c2, d;
	
	do {
		c1 = *s1++;
		c2 = *s2++;
		
		d = c1 - c2;
		if( d )
		{
			return ( INT32_SIGNBITNOTSET( d ) << 1 ) - 1;
		}
	}
	while( c1 );
	
	return 0;		// strings are equal
}

/*
================
idStr::Cmpn
================
*/
int idStr::Cmpn( const char* s1, const char* s2, int n )
{
	int c1, c2, d;
	
	assert( n >= 0 );
	
	do {
		c1 = *s1++;
		c2 = *s2++;
		
		if( !n-- )
		{
			return 0;		// strings are equal until end point
		}
		
		d = c1 - c2;
		if( d )
		{
			return ( INT32_SIGNBITNOTSET( d ) << 1 ) - 1;
		}
	}
	while( c1 );
	
	return 0;		// strings are equal
}

/*
================
idStr::Icmp
================
*/
int idStr::Icmp( const char* s1, const char* s2 )
{
	int c1, c2, d;
	
	do {
		c1 = *s1++;
		c2 = *s2++;
		
		d = c1 - c2;
		while( d )
		{
			if( c1 <= 'Z' && c1 >= 'A' )
			{
				d += ( 'a' - 'A' );
				if( !d )
				{
					break;
				}
			}
			if( c2 <= 'Z' && c2 >= 'A' )
			{
				d -= ( 'a' - 'A' );
				if( !d )
				{
					break;
				}
			}
			return ( INT32_SIGNBITNOTSET( d ) << 1 ) - 1;
		}
	}
	while( c1 );
	
	return 0;		// strings are equal
}

/*
================
idStr::Icmpn
================
*/
int idStr::Icmpn( const char* s1, const char* s2, int n )
{
	int c1, c2, d;
	
	assert( n >= 0 );
	
	do {
		c1 = *s1++;
		c2 = *s2++;
		
		if( !n-- )
		{
			return 0;		// strings are equal until end point
		}
		
		d = c1 - c2;
		while( d )
		{
			if( c1 <= 'Z' && c1 >= 'A' )
			{
				d += ( 'a' - 'A' );
				if( !d )
				{
					break;
				}
			}
			if( c2 <= 'Z' && c2 >= 'A' )
			{
				d -= ( 'a' - 'A' );
				if( !d )
				{
					break;
				}
			}
			return ( INT32_SIGNBITNOTSET( d ) << 1 ) - 1;
		}
	}
	while( c1 );
	
	return 0;		// strings are equal
}

/*
================
idStr::Icmp
================
*/
int idStr::IcmpNoColor( const char* s1, const char* s2 )
{
	int c1, c2, d;
	
	do {
		while( IsColor( s1 ) )
		{
			s1 += 2;
		}
		while( IsColor( s2 ) )
		{
			s2 += 2;
		}
		c1 = *s1++;
		c2 = *s2++;
		
		d = c1 - c2;
		while( d )
		{
			if( c1 <= 'Z' && c1 >= 'A' )
			{
				d += ( 'a' - 'A' );
				if( !d )
				{
					break;
				}
			}
			if( c2 <= 'Z' && c2 >= 'A' )
			{
				d -= ( 'a' - 'A' );
				if( !d )
				{
					break;
				}
			}
			return ( INT32_SIGNBITNOTSET( d ) << 1 ) - 1;
		}
	}
	while( c1 );
	
	return 0;		// strings are equal
}

/*
================
idStr::IcmpPath
================
*/
int idStr::IcmpPath( const char* s1, const char* s2 )
{
	int c1, c2, d;
	
#if 0
//#if !defined( ID_PC_WIN )
	idLib::common->Printf( "WARNING: IcmpPath used on a case-sensitive filesystem?\n" );
#endif
	
	do {
		c1 = *s1++;
		c2 = *s2++;
		
		d = c1 - c2;
		while( d )
		{
			if( c1 <= 'Z' && c1 >= 'A' )
			{
				d += ( 'a' - 'A' );
				if( !d )
				{
					break;
				}
			}
			if( c1 == '\\' )
			{
				d += ( '/' - '\\' );
				if( !d )
				{
					break;
				}
			}
			if( c2 <= 'Z' && c2 >= 'A' )
			{
				d -= ( 'a' - 'A' );
				if( !d )
				{
					break;
				}
			}
			if( c2 == '\\' )
			{
				d -= ( '/' - '\\' );
				if( !d )
				{
					break;
				}
			}
			// make sure folders come first
			while( c1 )
			{
				if( c1 == '/' || c1 == '\\' )
				{
					break;
				}
				c1 = *s1++;
			}
			while( c2 )
			{
				if( c2 == '/' || c2 == '\\' )
				{
					break;
				}
				c2 = *s2++;
			}
			if( c1 && !c2 )
			{
				return -1;
			}
			else if( !c1 && c2 )
			{
				return 1;
			}
			// same folder depth so use the regular compare
			return ( INT32_SIGNBITNOTSET( d ) << 1 ) - 1;
		}
	}
	while( c1 );
	
	return 0;
}

/*
================
idStr::IcmpnPath
================
*/
int idStr::IcmpnPath( const char* s1, const char* s2, int n )
{
	int c1, c2, d;
	
#if 0
//#if !defined( ID_PC_WIN )
	idLib::common->Printf( "WARNING: IcmpPath used on a case-sensitive filesystem?\n" );
#endif
	
	assert( n >= 0 );
	
	do {
		c1 = *s1++;
		c2 = *s2++;
		
		if( !n-- )
		{
			return 0;		// strings are equal until end point
		}
		
		d = c1 - c2;
		while( d )
		{
			if( c1 <= 'Z' && c1 >= 'A' )
			{
				d += ( 'a' - 'A' );
				if( !d )
				{
					break;
				}
			}
			if( c1 == '\\' )
			{
				d += ( '/' - '\\' );
				if( !d )
				{
					break;
				}
			}
			if( c2 <= 'Z' && c2 >= 'A' )
			{
				d -= ( 'a' - 'A' );
				if( !d )
				{
					break;
				}
			}
			if( c2 == '\\' )
			{
				d -= ( '/' - '\\' );
				if( !d )
				{
					break;
				}
			}
			// make sure folders come first
			while( c1 )
			{
				if( c1 == '/' || c1 == '\\' )
				{
					break;
				}
				c1 = *s1++;
			}
			while( c2 )
			{
				if( c2 == '/' || c2 == '\\' )
				{
					break;
				}
				c2 = *s2++;
			}
			if( c1 && !c2 )
			{
				return -1;
			}
			else if( !c1 && c2 )
			{
				return 1;
			}
			// same folder depth so use the regular compare
			return ( INT32_SIGNBITNOTSET( d ) << 1 ) - 1;
		}
	}
	while( c1 );
	
	return 0;
}

/*
=============
idStr::Copynz

	Safe strncpy that ensures a trailing zero
	NOTE: the specs indicate strncpy pads with zeros up to destination size, which be a bit wasteful
=============
*/
void idStr::Copynz( char* dest, const char* src, int destsize )
{
	if( !src )
	{
		idLib::common->Warning( "idStr::Copynz: NULL src" );
		return;
	}
	if( destsize < 1 )
	{
		idLib::common->Warning( "idStr::Copynz: destsize < 1" );
		return;
	}
	
	strncpy( dest, src, destsize - 1 );
	dest[ destsize - 1 ] = '\0';
}
/*
=============
	Copies the C string pointed by source into the array pointed by destination, 
	including the terminating null character (and stopping at that point).

	To avoid overflows, the size of the array pointed by destination shall be long enough to contain 
	the same C string as source (including the terminating null character), 
	and should not overlap in memory with source.
=============
*/
void idStr::Copy( char* dest, const char* src )
{
	if( !src )
	{
		idLib::common->Warning( "idStr::Copy: NULL src" );
		return;
	}
	int src_size = Length( src );
	if( src_size < 1 )
	{
		idLib::common->Warning( "idStr::Copy: src_size < 1" );
		return;
	}

	strncpy( dest, src, src_size - 1 );
	dest[ src_size - 1 ] = '\0';
}


/*
================
idStr::Append

  never goes past bounds or leaves without a terminating 0
================
*/
void idStr::Append( char* dest, int size, const char* src )
{
	int l1 = Length( dest );
	if( l1 >= size )
	{
		idLib::common->Error( "idStr::Append: already overflowed" );
	}
	Copynz( dest + l1, src, size - l1 );
}

/*
========================
idStr::IsValidUTF8
========================
*/
bool idStr::IsValidUTF8( const uint8* s, const int maxLen, utf8Encoding_t& encoding )
{
	struct local_t
	{
		static int GetNumEncodedUTF8Bytes( const uint8 c )
		{
			if( c < 0x80 )
			{
				return 1;
			}
			else if( ( c >> 5 ) == 0x06 )
			{
				// 2 byte encoding - the next byte must begin with
				return 2;
			}
			else if( ( c >> 4 ) == 0x0E )
			{
				// 3 byte encoding
				return 3;
			}
			else if( ( c >> 5 ) == 0x1E )
			{
				// 4 byte encoding
				return 4;
			}
			// this isnt' a valid UTF-8 precursor character
			return 0;
		}
		static bool RemainingCharsAreUTF8FollowingBytes( const uint8* s, const int curChar, const int maxLen, const int num )
		{
			if( maxLen - curChar < num )
			{
				return false;
			}
			for( int i = curChar + 1; i <= curChar + num; i++ )
			{
				if( s[ i ] == '\0' )
				{
					return false;
				}
				if( ( s[ i ] >> 6 ) != 0x02 )
				{
					return false;
				}
			}
			return true;
		}
	};
	
	// check for byte-order-marker
	encoding = UTF8_PURE_ASCII;
	utf8Encoding_t utf8Type = UTF8_ENCODED_NO_BOM;
	if( maxLen > 3 && s[ 0 ] == 0xEF && s[ 1 ] == 0xBB && s[ 2 ] == 0xBF )
	{
		utf8Type = UTF8_ENCODED_BOM;
	}
	
	for( int i = 0; s[ i ] != '\0' && i < maxLen; i++ )
	{
		int numBytes = local_t::GetNumEncodedUTF8Bytes( s[ i ] );
		if( numBytes == 1 )
		{
			continue;	// just low ASCII
		}
		else if( numBytes == 2 )
		{
			// 2 byte encoding - the next byte must begin with bit pattern 10
			if( !local_t::RemainingCharsAreUTF8FollowingBytes( s, i, maxLen, 1 ) )
			{
				return false;
			}
			// skip over UTF-8 character
			i += 1;
			encoding = utf8Type;
		}
		else if( numBytes == 3 )
		{
			// 3 byte encoding - the next 2 bytes must begin with bit pattern 10
			if( !local_t::RemainingCharsAreUTF8FollowingBytes( s, i, maxLen, 2 ) )
			{
				return false;
			}
			// skip over UTF-8 character
			i += 2;
			encoding = utf8Type;
		}
		else if( numBytes == 4 )
		{
			// 4 byte encoding - the next 3 bytes must begin with bit pattern 10
			if( !local_t::RemainingCharsAreUTF8FollowingBytes( s, i, maxLen, 3 ) )
			{
				return false;
			}
			// skip over UTF-8 character
			i += 3;
			encoding = utf8Type;
		}
		else
		{
			// this isnt' a valid UTF-8 character
			if( utf8Type == UTF8_ENCODED_BOM )
			{
				encoding = UTF8_INVALID_BOM;
			}
			else
			{
				encoding = UTF8_INVALID;
			}
			return false;
		}
	}
	return true;
}

/*
========================
idStr::UTF8Length
========================
*/
int idStr::UTF8Length( const byte* s )
{
	int mbLen = 0;
	int charLen = 0;
	while( s[ mbLen ] != '\0' )
	{
		uint32 cindex;
		cindex = s[ mbLen ];
		if( cindex < 0x80 )
		{
			mbLen++;
		}
		else
		{
			int trailing = 0;
			if( cindex >= 0xc0 )
			{
				static const byte trailingBytes[ 64 ] = {
					1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
					2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
				};
				trailing = trailingBytes[ cindex - 0xc0 ];
			}
			mbLen += trailing + 1;
		}
		charLen++;
	}
	return charLen;
}


/*
========================
idStr::AppendUTF8Char
========================
*/
void idStr::AppendUTF8Char( uint32 c )
{
	if( c < 0x80 )
	{
		Append( ( char )c );
	}
	else if( c < 0x800 )      // 11 bits
	{
		Append( ( char )( 0xC0 | ( c >> 6 ) ) );
		Append( ( char )( 0x80 | ( c & 0x3F ) ) );
	}
	else if( c < 0x10000 )      // 16 bits
	{
		Append( ( char )( 0xE0 | ( c >> 12 ) ) );
		Append( ( char )( 0x80 | ( ( c >> 6 ) & 0x3F ) ) );
		Append( ( char )( 0x80 | ( c & 0x3F ) ) );
	}
	else if( c < 0x200000 )  	// 21 bits
	{
		Append( ( char )( 0xF0 | ( c >> 18 ) ) );
		Append( ( char )( 0x80 | ( ( c >> 12 ) & 0x3F ) ) );
		Append( ( char )( 0x80 | ( ( c >> 6 ) & 0x3F ) ) );
		Append( ( char )( 0x80 | ( c & 0x3F ) ) );
	}
	else
	{
		// UTF-8 can encode up to 6 bytes. Why don't we support that?
		// This is an invalid Unicode character
		Append( '?' );
	}
}

/*
========================
idStr::UTF8Char
========================
*/
uint32 idStr::UTF8Char( const byte* s, int& idx )
{
	if( idx >= 0 )
	{
		while( s[ idx ] != '\0' )
		{
			uint32 cindex = s[ idx ];
			if( cindex < 0x80 )
			{
				idx++;
				return cindex;
			}
			int trailing = 0;
			if( cindex >= 0xc0 )
			{
				static const byte trailingBytes[ 64 ] =
				{
					1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
					2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
				};
				trailing = trailingBytes[ cindex - 0xc0 ];
			}
			static const uint32 trailingMask[ 6 ] = { 0x0000007f, 0x0000001f, 0x0000000f, 0x00000007, 0x00000003, 0x00000001 };
			cindex &= trailingMask[ trailing  ];
			while( trailing-- > 0 )
			{
				cindex <<= 6;
				cindex += s[ ++idx ] & 0x0000003f;
			}
			idx++;
			return cindex;
		}
	}
	idx++;
	return 0;	// return a null terminator if out of range
}

/*
================
idStr::LengthWithoutColors
================
*/
int idStr::LengthWithoutColors( const char* s )
{
	int len;
	const char* p;
	
	if( !s )
	{
		return 0;
	}
	
	len = 0;
	p = s;
	while( *p )
	{
		if( IsColor( p ) )
		{
			p += 2;
			continue;
		}
		p++;
		len++;
	}
	
	return len;
}

/*
================
idStr::RemoveColors
================
*/
char* idStr::RemoveColors( char* string )
{
	char* d;
	char* s;
	int c;
	
	s = string;
	d = string;
	while( ( c = *s ) != 0 )
	{
		if( IsColor( s ) )
		{
			s++;
		}
		else
		{
			*d++ = c;
		}
		s++;
	}
	*d = '\0';
	
	return string;
}

/*
================
idStr::IsBadFilenameChar
================
*/
bool idStr::IsBadFilenameChar( char c ) {
	static char badFilenameChars[] = { ':', ';', '&', '(', ')', '|', '<', '>', '*', '?', '[', ']', '~', '+', '@', '!', '\\', '/', ' ', '\t', '\'', '"', '\0' };

	for ( int i = 0; badFilenameChars[i] != '\0'; i++ ) {
		if ( c == badFilenameChars[i] ) {
			return true;
		}
	}

	return false;
}

/*
================
idStr::CleanFilename
================
*/
char* idStr::CleanFilename( char* string ) 
{
	char* s = string;
	char* d = string;

	// clear leading .'s
	while ( *s == '.' ) {
		s++;
	}

	while ( *s != '\0' ) {
		if ( !IsBadFilenameChar( *s ) ) {
			*d++ = *s;
		}
		s++;
	}
	*d = '\0';

	return string;
}

/*
================
idStr::StripFilename
================
*/
char* idStr::StripFilename( char* string ) 
{
	int pos = idStr::Length( string ) - 1;
	while( ( pos > 0 ) && ( string[ pos ] != '/' ) && ( string[ pos ] != '\\' ) ) {
		pos--;
	}

	if ( pos < 0 ) {
		pos = 0;
	}

	string[ pos ] = '\0';

	return string;
}

/*
==================
idStr::StripPath
==================
*/
char* idStr::StripPath( char* string ) {
	int pos, length;

	length = pos = idStr::Length( string );
	while( ( pos > 0 ) && ( string[ pos - 1 ] != '/' ) && ( string[ pos - 1 ] != '\\' ) ) {
		pos--;
	}

	return &string[ pos ];
}

/*
================
idStr::snPrintf
================
*/
int idStr::snPrintf( char* dest, int size, const char* fmt, ... )
{
	int len;
	va_list argptr;
	char buffer[32000];	// big, but small enough to fit in PPC stack
	
	va_start( argptr, fmt );
	len = vsprintf( buffer, fmt, argptr );
	va_end( argptr );
	if( len >= sizeof( buffer ) )
	{
		idLib::common->Error( "idStr::snPrintf: overflowed buffer" );
	}
	if( len >= size )
	{
		idLib::common->Warning( "idStr::snPrintf: overflow of %i in %i\n", len, size );
		len = size;
	}
	idStr::Copynz( dest, buffer, size );
	return len;
}

/*
============
idStr::vsnPrintf

vsnprintf portability:

C99 standard: vsnprintf returns the number of characters (excluding the trailing
'\0') which would have been written to the final string if enough space had been available
snprintf and vsnprintf do not write more than size bytes (including the trailing '\0')

win32: _vsnprintf returns the number of characters written, not including the terminating null character,
or a negative value if an output error occurs. If the number of characters to write exceeds count, then count
characters are written and -1 is returned and no trailing '\0' is added.

idStr::vsnPrintf: always appends a trailing '\0', returns number of characters written (not including terminal \0)
or returns -1 on failure or if the buffer would be overflowed.
============
*/
int idStr::vsnPrintf( char* dest, int size, const char* fmt, va_list argptr )
{
	int ret;
	
// RB begin
#ifdef _WIN32
#undef _vsnprintf
	ret = _vsnprintf( dest, size - 1, fmt, argptr );
#define _vsnprintf	use_idStr_vsnPrintf
#else
#undef vsnprintf
	ret = vsnprintf( dest, size, fmt, argptr );
#define vsnprintf	use_idStr_vsnPrintf
#endif
// RB end

	dest[size - 1] = '\0';
	if( ret < 0 || ret >= size )
	{
		return -1;
	}
	return ret;
}

/*
===============
idStr::Test
test those snPrintf/vsnPrintf functions
mostly to check the behaviour between win32 and other platforms is the same
===============
*/
void idStr::Test( void ) {
	char buffer[10];
	int ret;
	idStr test;

	idLib::common->Printf( "idStr::Test\n" );

	idStr::Copynz( buffer, "012345678", sizeof( buffer ) );
	assert( buffer[9] == '\0' );

	ret = test.snPrintf( buffer, 10, "%s", "876543210" );
	assert( buffer[9] == '\0' );
	idLib::common->Printf( "%d %s\n", ret, buffer );

	ret = test.snPrintf( buffer, 10, "%s", "0123456789" );
	assert( buffer[9] == '\0' );
	idLib::common->Printf( "%d %s\n", ret, buffer );
}

/*
============
sprintf

Sets the value of the string using a printf interface.
============
*/
int sprintf( idStr& string, const char* fmt, ... )
{
	int l;
	va_list argptr;
	char buffer[32000];
	
	va_start( argptr, fmt );
	l = idStr::vsnPrintf( buffer, sizeof( buffer ) - 1, fmt, argptr );
	va_end( argptr );
	buffer[sizeof( buffer ) - 1] = '\0';
	
	string = buffer;
	return l;
}

/*
============
vsprintf

Sets the value of the string using a vprintf interface.
============
*/
int vsprintf( idStr& string, const char* fmt, va_list argptr )
{
	int l;
	char buffer[32000];
	
	l = idStr::vsnPrintf( buffer, sizeof( buffer ) - 1, fmt, argptr );
	buffer[sizeof( buffer ) - 1] = '\0';
	
	string = buffer;
	return l;
}

/*
============
va

does a varargs printf into a temp buffer
NOTE: not thread safe
============
*/
char* va( const char* fmt, ... )
{
	va_list argptr;
	static int index = 0;
	static char string[4][16384];	// in case called by nested functions
	char* buf;
	
	buf = string[index];
	index = ( index + 1 ) & 3;
	
	va_start( argptr, fmt );
	vsprintf( buf, fmt, argptr );
	va_end( argptr );
	
	return buf;
}



/*
============
idStr::BestUnit
============
*/
int idStr::BestUnit( const char* format, float value, measure_t measure )
{
	int unit = 1;
	while( unit <= 3 && ( 1 << ( unit * 10 ) < value ) )
	{
		unit++;
	}
	unit--;
	value /= 1 << ( unit * 10 );
	sprintf( *this, format, value );
	*this += " ";
	*this += units[ measure ][ unit ];
	return unit;
}

/*
============
idStr::SetUnit
============
*/
void idStr::SetUnit( const char* format, float value, int unit, measure_t measure )
{
	value /= 1 << ( unit * 10 );
	sprintf( *this, format, value );
	*this += " ";
	*this += units[ measure ][ unit ];
}

/*
================
idStr::InitMemory
================
*/
void idStr::InitMemory()
{
#ifdef USE_STRING_DATA_ALLOCATOR
	stringDataAllocator.Init();
#endif
}

/*
================
idStr::ShutdownMemory
================
*/
void idStr::ShutdownMemory()
{
#ifdef USE_STRING_DATA_ALLOCATOR
	stringDataAllocator.Shutdown();
#endif
}

/*
================
idStr::PurgeMemory
================
*/
void idStr::PurgeMemory()
{
#ifdef USE_STRING_DATA_ALLOCATOR
	stringDataAllocator.FreeEmptyBaseBlocks();
#endif
}

/*
================
idStr::ShowMemoryUsage_f
================
*/
void idStr::ShowMemoryUsage_f( const idCmdArgs& args )
{
#ifdef USE_STRING_DATA_ALLOCATOR
	idLib::common->Printf( "%6d KB string memory (%d KB free in %d blocks, %d empty base blocks)\n",
						   stringDataAllocator.GetBaseBlockMemory() >> 10, stringDataAllocator.GetFreeBlockMemory() >> 10,
						   stringDataAllocator.GetNumFreeBlocks(), stringDataAllocator.GetNumEmptyBaseBlocks() );
#endif
}

/*
================
idStr::FormatNumber
================
*/
struct formatList_t
{
	int			gran;
	int			count;
};

// elements of list need to decend in size
formatList_t formatList[] =
{
	{ 1000000000, 0 },
	{ 1000000, 0 },
	{ 1000, 0 }
};

int numFormatList = sizeof( formatList ) / sizeof( formatList[0] );


idStr idStr::FormatNumber( int number )
{
	idStr string;
	bool hit;
	
	// reset
	for( int i = 0; i < numFormatList; i++ )
	{
		formatList_t* li = formatList + i;
		li->count = 0;
	}
	
	// main loop
	do
	{
		hit = false;
		
		for( int i = 0; i < numFormatList; i++ )
		{
			formatList_t* li = formatList + i;
			
			if( number >= li->gran )
			{
				li->count++;
				number -= li->gran;
				hit = true;
				break;
			}
		}
	}
	while( hit );
	
	// print out
	bool found = false;
	
	for( int i = 0; i < numFormatList; i++ )
	{
		formatList_t* li = formatList + i;
		
		if( li->count )
		{
			if( !found )
			{
				string += va( "%i,", li->count );
			}
			else
			{
				string += va( "%3.3i,", li->count );
			}
			found = true;
		}
		else if( found )
		{
			string += va( "%3.3i,", li->count );
		}
	}
	
	if( found )
	{
		string += va( "%3.3i", number );
	}
	else
	{
		string += va( "%i", number );
	}
	
	// pad to proper size
	int count = 11 - string.Length();
	
	for( int i = 0; i < count; i++ )
	{
		string.Insert( " ", 0 );
	}
	
	return string;
}

CONSOLE_COMMAND( testStrId, "prints a localized string", 0 )
{
	if( args.Argc() != 2 )
	{
		idLib::Printf( "need a str id like 'STR_SWF_ACCEPT' without the hash, it gets parsed as a separate argument\n" );
		return;
	}
	
	idStrId str( va( "#%s", args.Argv( 1 ) ) );
	idLib::Printf( "%s = %s\n", args.Argv( 1 ), str.GetLocalizedString() );
}

/*
===============
idStr::IndentAndPad

adds a formated, indented line to a string.  The line is indented "indent" 
characters and the formatted string is written. If the string size is less than 
the pad size, the remaining characters in the string are filled with spaces up
to the "pad" position.
===============
*/
void idStr::IndentAndPad( int indent, int pad, idStr &str, const char *fmt, ... )
{
	assert( pad >= 0 );
	if ( pad < 0 ) {
		pad = 0;
	}
	int max = 1024;
	char *buff = (char *)_alloca( 1024 + 1 );
	memset( buff, 0x20, indent > 128 ? 128 : indent );

	va_list	argptr;
	va_start( argptr, fmt );
	vsnPrintf( buff + indent, max - indent, fmt, argptr );
	va_end( argptr );

	int len = Length( buff );
	if ( pad && len <= pad ) 
	{
		memset( buff + len, 0x20, pad - len );
		buff[ pad ] = '\0';
	} 
	else {
		// ensure there's at least 1 space of padding if the formatted string
		// exceeded the pad size
		buff[ len ] = ' ';
		buff[ len + 1 ] = '\0';
	}

	str += buff;
}

/*
============
idStr::EraseRange
============
*/
void idStr::EraseRange( int start, int len )
{
	if( IsEmpty() || len == 0 ) {
		return;
	}

	if( start < 0 ) {
		start = 0;
	}

	if( start >= this->len ) {
		return;
	}

	int totalLength = Length();
	if( len == INVALID_POSITION ) {
		len = totalLength - start;
	}
	
	if( len == totalLength ) {
		// erase the whole thing
		Empty();
		return;
	}


	if( totalLength - start - len ) {
		memmove( &data[ start ], &data[ start + len ], totalLength - start - len );
	}
	
	data[ totalLength - len ] = '\0';
	this->len -= len;
}

/*
============
idStr::EraseChar
============
*/
void idStr::EraseChar( const char c, int start )
{
	if( start < 0 ) {
		start = 0;
	}

	int totalLength = Length();
	while( start < totalLength - 1 ) 
	{
		int offset = start + 1;
		while( data[ start ] == c && offset < totalLength )
		{
			SwapValues( data[ start ], data[ offset ] );
			offset++;
		}
		start++;
	}

	start = totalLength - 1;
	while( start > 0 && data[ start ] == c )
	{
		data[ start ] = '\0';
		start--;
	}
	len = start + 1;
}

/*
============
idStr::Append
============
*/
void idStr::Append( int count, const char c )
{
	EnsureAlloced( len + count + 1 );
	int start = len;
	int end = len + count;
	while( start < end ) {
		data[ start ] = c;
		start++;
	}
	data[ start ] = '\0';
	len += count;
}

/*
============
idStr::StripComments
============
*/
idStr & idStr::StripComments() 
{
	// handle C++-style comments
	int startIndex = Find( "//" );
	int endIndex = Find( "\n", true, startIndex + 2 );

	while( startIndex != -1 && endIndex != -1 ) 
	{
		int oldLength = len;
		EraseRange( startIndex, endIndex - startIndex );
		if( len == oldLength ) 
		{
			idLib::common->Warning( "StripCommentsFromString: Couldn't strip comments" );
			break;	//avoid infinite loops
		}
		startIndex = Find( "//" );
		endIndex = Find( "\n", true, startIndex + 2 );
	}

	// handle C-style comments
	startIndex = Find( "/*" );
	endIndex = Find( "*/", true, startIndex + 2 );

	if( ( startIndex != -1 && endIndex == -1 ) || ( startIndex == -1 && endIndex != -1 ))
	{
		idLib::common->Warning( "StripCommentsFromString: mismatched /* */ comment" );
		return *this;
	}	

	while( startIndex != -1 && endIndex != -1 ) 
	{
		int oldLength = len;
		EraseRange( startIndex, endIndex - startIndex + 2 );
		if( len == oldLength ) 
		{
			idLib::common->Warning( "StripCommentsFromString: Couldn't strip comments" );
			break;	//avoid infinite loops
		}
		startIndex = Find( "/*" );
		endIndex = Find( "*/", true, startIndex + 2 );
	}

	return *this;
}

/*
============
idStr::Indent
braces within C or C++ style comments are ignored
============
*/
idStr& idStr::Indent() 
{
	Replace( "\r\n", "\n" );	// kill windows line endings
	EraseChar( '\r' );			// kill broken windows line endings

	// strip out tabs at the beginning of lines
	for( int i = 0; i < len; ++i ) {
		if( data[ i ] == '\n' ) {
			++i;
			while( i < len && data[ i ] == '\t' ) {
				EraseRange( i, 1 );
			}
			--i;
		}
	}

	idStr output;
	output.EnsureAlloced( len, false );
	int indent = 0;
	for( int i = 0; i < len; i++ ) 
	{
		// skip braces within comments
		if( i + 1 < len ) 
		{
			if( data[ i ] == '/' && data[ i + 1 ] == '/' ) 
			{
				while( i < len && data[ i ] != '\n' ) {
					output += data[ i ];
					i++;
				}
			}  
			else if( data[ i ] == '/' && data[ i + 1 ] == '*' ) 
			{
				while( i < len && !( data[ i ] == '*' && data[ i + 1 ] == '/' )) 
				{
					output += data[ i ];
					i++;
				}
			}
		}

		if( data[ i ] == '{' )
		{
			indent++;
		} 
		else if( data[ i ] == '}' ) 
		{
			indent--;
			// unindent closing braces
			output.StripTrailingOnce( "\t" );
		}

		output += data[ i ];

		if( data[ i ] == '\n' && indent > 0 ) {
			output.Append( indent, '\t' );
		}		
	}

	*this = output;
	return *this;
}

/*
============
idStr::Unindent
unindent all lines; tabs are preserved if they are in the middle of a line
============
*/
idStr& idStr::Unindent() 
{
	idStr output;
	output.EnsureAlloced( len, false );
	for( int i = 0; i < len; ++i )
	{	
		if( data[ i ] == '\t' && i > 0 && data[ i - 1 ] == '\n' ) 
		{
			// strip leading tabs
			while( i < len && data[ i ] == '\t' ) {
				++i;
			}
		} 
		else if( data[ i ] == '\t' && i > 0 && data[ i - 1 ] != '\n' ) 
		{
			// strip trailing tabs 
			int temp = i;
			while( temp < len ) {
				if( data[ temp ] == '\r' || data[ temp ] == '\n' ) {
					i = temp;
					break;
				} else if( data[ temp ] != '\t' ){
					break;
				}
				++temp;
			}
		}
		output += data[ i ];				
	}

	*this = output;
	return *this;
}

static const char* const hexDigits = "0123456789ABCDEF";

/*
============
idStr::StringToBinaryString
============
*/
void idStr::StringToBinaryString( idStr& out, void *pv, int size ) 
{
	//idAutoPtr< unsigned char, idArrayCleanupPolicy< unsigned char > > in( new unsigned char[ size ] );
	auto in = std::make_unique<unsigned char[]>( size );

	memset( in.get(), 0, size );
	memcpy( in.get(), pv, size );

	for( int i = 0; i < size; i++ ) {
		unsigned char c = in[ i ];
		out += hexDigits[ c >> 4 ];
		out += hexDigits[ c & 0x0f ];
	}
}

/*
============
idStr::BinaryStringToString
============
*/
bool idStr::BinaryStringToString( const char* str,  void* pv, int size )
{
	bool ret = false;

	int length = idStr::Length( str );

	if( length / 2 == size )
	{
		//idAutoPtr< unsigned char, idArrayCleanupPolicy< unsigned char > > out( new unsigned char[ size ] );
		auto out = std::make_unique<unsigned char[]>( size );

		int j = 0;
		for( int i = 0; i < length; i += 2 ) 
		{
			char c;
			if( str[ i ] > '9' ) {
				c = str[ i ] - 'A' + 0x0a;
			} else {
				c = str[ i ] - 0x30;
			}
			c <<= 4;
			if( str[ i + 1 ] > '9' ) {
				c |= str[ i + 1 ] - 'A' + 0x0a;
			} else {
				c |= str[ i + 1 ] - 0x30;
			}
			assert( j < ( size ) );
			out[ j++ ] = c;
		}

		memcpy( pv, out.get(), size );
		ret = true;
	}
	else {
		assert( !"Invalid size for binary string" );
	}
	return ret;
}

/*
============
idStr::IsValidEmailAddress
============
*/
bool idStr::IsValidEmailAddress( const char* address ) 
{
	int count = 0;
	const char* c;
	const char* domain;
	static const char* rfc822 = "()<>@,;:\\\"[]";

	// validate name
	for( c = address; *c != '\0'; c++ )
	{
		if( *c == '\"' && ( c == address || *(c - 1) == '.' || *(c - 1) == '\"' ) )
		{
			while( *++c ) 
			{
				if ( *c == '\"' ) {
					break;
				}
				if ( *c == '\\' && ( *++c == ' ' ) ) {
					continue;
				}
				if ( *c <= ' ' || *c >= 127 ) {
					return 0;
				}
			}
			if ( *c++ == '\0' ) {
				return false;
			}
			if ( *c == '@' ) {
				break;
			}
			if ( *c == '.' ) {
				return false;
			}
			continue;
		}
		if ( *c == '@' ) {
			break;
		}
		if ( *c <= ' ' || *c >= 127 ) {
			return false;
		}
		if ( FindChar( rfc822, *c ) != INVALID_POSITION ) {
			return false;
		}
	}

	if ( c == address || *(c - 1 ) == '.' ) {
		return false;
	}

	// validate domain
	if ( *( domain = ++c ) == '\0' ) {
		return false;
	}

	do {
		if ( *c == '.' ) {
			if ( c == domain || *(c - 1) == '.' ) {
				return false;
			}
			count++;
		}
		if ( *c <= ' ' || *c >= 127 ) {
			return false;
		}
		if ( FindChar( rfc822, *c ) != INVALID_POSITION ) {
			return false;
		}
	} while ( *++c );

	return ( count >= 1 );
}

/*
============
idStr::MS2HMS
============
*/
const char*	idStr::MS2HMS( double ms, const hmsFormat_t& formatSpec ) 
{
	if ( ms < 0.0 ) {
		ms = 0.0;
	}

	int sec = idMath::Ftoi( MS2SEC( ms ) );

	if( sec == 0 && formatSpec.showZeroSeconds == false ) {
		return "";
	}

	int min = sec / 60;
	int hour = min / 60;
		
	sec -= min * 60;
	min -= hour * 60;

	// don't show minutes if they're zeroed
	if( min == 0 && hour == 0 && formatSpec.showZeroMinutes == false && formatSpec.showZeroHours == false ) {
		return va( "%02i", sec );
	}

	// don't show hours if they're zeroed
	if( hour == 0 && formatSpec.showZeroHours == false ) {
		return va( "%02i:%02i", min, sec );
	}
	return va( "%02i:%02i:%02i", hour, min, sec );
}

/*
============
idStr::CollapseColors
============
*/
idStr& idStr::CollapseColors( void ) 
{
	int colorBegin = -1;
	int lastColor = -1;
	for( int i = 0; i < len; i++ ) {
		while( idStr::IsColor( &data[ i ] ) && i < len ) {
			if( colorBegin == -1 ) {
				colorBegin = i;
			}
			lastColor = i;
			i += 2;
		}
		if( colorBegin != -1 && lastColor != colorBegin ) {
			EraseRange( colorBegin, lastColor - colorBegin );
			i -= lastColor - colorBegin;
		}
		colorBegin = -1;
		lastColor = -1;
	}
	return *this;
}
