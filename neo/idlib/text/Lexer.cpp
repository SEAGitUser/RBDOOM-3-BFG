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

#include "../precompiled.h"
#pragma hdrstop

#define PUNCTABLE

//longer punctuations first
static punctuation_t default_punctuations[] = {
	// binary operators
	{">>=",P_RSHIFT_ASSIGN},
	{"<<=",P_LSHIFT_ASSIGN},
	//
	{"...",P_PARMS},
	{"->*",P_POINTER_TO_MEMBER_POINTER},
	// define merge operator
	{"##",P_PRECOMPMERGE},				// pre-compiler
	// logic operators
	{"&&",P_LOGIC_AND},					// pre-compiler
	{"||",P_LOGIC_OR},					// pre-compiler
	{">=",P_LOGIC_GEQ},					// pre-compiler
	{"<=",P_LOGIC_LEQ},					// pre-compiler
	{"==",P_LOGIC_EQ},					// pre-compiler
	{"!=",P_LOGIC_UNEQ},				// pre-compiler
	// arithmetic operators
	{"*=",P_MUL_ASSIGN},
	{"/=",P_DIV_ASSIGN},
	{"%=",P_MOD_ASSIGN},
	{"+=",P_ADD_ASSIGN},
	{"-=",P_SUB_ASSIGN},
	{"++",P_INC},
	{"--",P_DEC},
	// binary operators
	{"&=",P_BIN_AND_ASSIGN},
	{"|=",P_BIN_OR_ASSIGN},
	{"^=",P_BIN_XOR_ASSIGN},
	{">>",P_RSHIFT},					// pre-compiler
	{"<<",P_LSHIFT},					// pre-compiler
	// member selection
	{"->",P_MEMBER_SELECTION_POINTER},
	{"::",P_SCOPE_RESOLUTION},
	{".*",P_POINTER_TO_MEMBER_OBJECT},
	// arithmetic operators
	{"*",P_MUL},						// pre-compiler
	{"/",P_DIV},						// pre-compiler
	{"%",P_MOD},						// pre-compiler
	{"+",P_ADD},						// pre-compiler
	{"-",P_SUB},						// pre-compiler
	{"=",P_ASSIGN},
	// binary operators
	{"&",P_BIN_AND},					// pre-compiler
	{"|",P_BIN_OR},						// pre-compiler
	{"^",P_BIN_XOR},					// pre-compiler
	{"~",P_BIN_NOT},					// pre-compiler
	// logic operators
	{"!",P_LOGIC_NOT},					// pre-compiler
	{">",P_LOGIC_GREATER},				// pre-compiler
	{"<",P_LOGIC_LESS},					// pre-compiler
	// member selection
	{".",P_MEMBER_SELECTION_OBJECT},
	// separators
	{",",P_COMMA},						// pre-compiler
	{";",P_SEMICOLON},
	// label indication
	{":",P_COLON},						// pre-compiler
	// if statement
	{"?",P_QUESTIONMARK},				// pre-compiler
	// embracements
	{"(",P_PARENTHESESOPEN},			// pre-compiler
	{")",P_PARENTHESESCLOSE},			// pre-compiler
	{"{",P_BRACEOPEN},					// pre-compiler
	{"}",P_BRACECLOSE},					// pre-compiler
	{"[",P_SQBRACKETOPEN},
	{"]",P_SQBRACKETCLOSE},
	//
	{"\\",P_BACKSLASH},
	// precompiler operator
	{"#",P_PRECOMP},					// pre-compiler
	{"$",P_DOLLAR},
	{NULL, 0}
};

int default_punctuationtable[ 256 ];
int default_nextpunctuation[ sizeof( default_punctuations ) / sizeof( punctuation_t ) ];
int default_setup;

char idLexer::baseFolder[ 256 ];

/*
================
idLexer::CreatePunctuationTable
================
*/
void idLexer::CreatePunctuationTable( const punctuation_t* punctuations )
{
	int i, n, lastp;
	const punctuation_t* p, *newp;

	//get memory for the table
	if( punctuations == default_punctuations )
	{
		punctuationtable = default_punctuationtable;
		nextpunctuation = default_nextpunctuation;
		if( default_setup )
		{
			return;
		}
		default_setup = true;
		i = sizeof( default_punctuations ) / sizeof( punctuation_t );
	}
	else
	{
		if( !punctuationtable || punctuationtable == default_punctuationtable )
		{
			punctuationtable = idMem::Alloc< int, TAG_IDLIB_LEXER >( 256 * sizeof( int ) );
		}
		if( nextpunctuation && nextpunctuation != default_nextpunctuation )
		{
			idMem::Free( nextpunctuation );
		}
		for( i = 0; punctuations[ i ].p; i++ )
		{
		}
		nextpunctuation = idMem::Alloc< int, TAG_IDLIB_LEXER >( i * sizeof( int ) );
	}
	memset( punctuationtable, 0xFF, 256 * sizeof( int ) );
	memset( nextpunctuation, 0xFF, i * sizeof( int ) );
	//add the punctuations in the list to the punctuation table
	for( i = 0; punctuations[ i ].p; i++ )
	{
		newp = &punctuations[ i ];
		lastp = -1;
		//sort the punctuations in this table entry on length (longer punctuations first)
		for( n = punctuationtable[ ( unsigned int )newp->p[ 0 ] ]; n >= 0; n = nextpunctuation[ n ] )
		{
			p = &punctuations[ n ];
			if( idStr::Length( p->p ) < idStr::Length( newp->p ) )
			{
				nextpunctuation[ i ] = n;
				if( lastp >= 0 )
				{
					nextpunctuation[ lastp ] = i;
				}
				else
				{
					punctuationtable[ ( unsigned int )newp->p[ 0 ] ] = i;
				}
				break;
			}
			lastp = n;
		}
		if( n < 0 )
		{
			nextpunctuation[ i ] = -1;
			if( lastp >= 0 )
			{
				nextpunctuation[ lastp ] = i;
			}
			else
			{
				punctuationtable[ ( unsigned int )newp->p[ 0 ] ] = i;
			}
		}
	}
}

/*
================
idLexer::GetPunctuationFromId
================
*/
const char *idLexer::GetPunctuationFromId( int id ) const
{
	for( int i = 0; punctuations[ i ].p; i++ )
	{
		if( punctuations[ i ].n == id )
		{
			return punctuations[ i ].p;
		}
	}
	return "unknown punctuation";
}

/*
================
idLexer::GetPunctuationId
================
*/
int idLexer::GetPunctuationId( const char *p ) const
{
	for( int i = 0; punctuations[ i ].p; ++i )
	{
		if( !idStr::Cmp( punctuations[ i ].p, p ) )
		{
			return punctuations[ i ].n;
		}
	}
	return 0;
}

/*
================
idLexer::Error
================
*/
void idLexer::Error( const char* str, ... )
{
	char text[ MAX_STRING_CHARS ];
	va_list ap;

	hadError = true;

	if( flags & LEXFL_NOERRORS )
	{
		return;
	}

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );

	if( flags & LEXFL_VCSTYLEREPORTS )
	{
		idStr path = filename;
		path.SlashesToBackSlashes();
		idLib::common->Printf( "%s(%d) : error : %s\n", path.c_str(), line, text );
	}
	else if( idLexer::flags & LEXFL_NOFATALERRORS )
	{
		idLib::common->Warning( "file %s, line %d: %s", filename.c_str(), line, text );
	}
	else
	{
		idLib::common->Error( "file %s, line %d: %s", filename.c_str(), line, text );
	}
}

/*
================
idLexer::Warning
================
*/
void idLexer::Warning( const char* str, ... )
{
	char text[ MAX_STRING_CHARS ];
	va_list ap;

	hadWarning = true;

	if( flags & LEXFL_NOWARNINGS )
	{
		return;
	}

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );

	if( flags & LEXFL_VCSTYLEREPORTS )
	{
		idStr path = filename;
		path.SlashesToBackSlashes();
		idLib::common->Printf( "%s(%d) : warning : %s\n", path.c_str(), line, text );
	}
	else
	{
		idLib::common->Warning( "file %s, line %d: %s", filename.c_str(), line, text );
	}
}

/*
================
idLexer::SetPunctuations
================
*/
void idLexer::SetPunctuations( const punctuation_t* p )
{
	release_assert( !loaded );		// punctuations must be set before loading the file
#ifdef PUNCTABLE
	if( p != NULL )
	{
		CreatePunctuationTable( p );
	}
	else
	{
		CreatePunctuationTable( default_punctuations );
	}
#endif //PUNCTABLE
	if( p != NULL )
	{
		punctuations = p;
	}
	else
	{
		punctuations = default_punctuations;
	}
}

/*
========================
idLexer::SkipWhiteSpace

	Reads spaces, tabs, C-like comments etc. 
	When a newline character is found, the scripts line counter is increased. 
	Returns false if there is no token left to be read.
========================
*/
bool idLexer::SkipWhiteSpace( bool currentLine )
{
	while( 1 )
	{
		assert( script_p <= end_p );
		if( script_p == end_p )
		{
			return false;
		}
		// skip white space
		while( *script_p <= ' ' )
		{
			if( script_p == end_p )
			{
				return false;
			}
			if( !*script_p )
			{
				return false;
			}
			if( *script_p == '\n' )
			{
				line++;
				if( currentLine )
				{
					script_p++;
					return true;
				}
			}
			script_p++;
		}
		// skip comments
		if( *script_p == '/' )
		{
			// comments //
			if( *( script_p + 1 ) == '/' )
			{
				script_p++;
				do
				{
					script_p++;
					if( !*script_p )
					{
						return false;
					}
				} while( *script_p != '\n' );
				line++;
				script_p++;
				if( currentLine )
				{
					return true;
				}
				if( !*script_p )
				{
					return false;
				}
				continue;
			}
			// comments /* */
			else if( *( script_p + 1 ) == '*' )
			{
				script_p++;
				while( 1 )
				{
					script_p++;
					if( !*script_p )
					{
						return false;
					}
					if( *script_p == '\n' )
					{
						line++;
					}
					else if( *script_p == '/' )
					{
						if( *( script_p - 1 ) == '*' )
						{
							break;
						}
						if( *( script_p + 1 ) == '*' )
						{
							Warning( "nested comment" );
						}
					}
				}
				script_p++;
				if( !*script_p )
				{
					return false;
				}
				continue;
			}
		}
		break;
	}
	return true;
}

/*
================
idLexer::ReadWhiteSpace

	Reads spaces, tabs, C-like comments etc.
	When a newline character is found the scripts line counter is increased.
================
*/
bool idLexer::ReadWhiteSpace()
{
	while( 1 )
	{
		// skip white space
		while( *script_p <= ' ' )
		{
			if( !*script_p )
			{
				return 0;
			}
			if( *script_p == '\n' )
			{
				line++;
			}
			script_p++;
		}
		// skip comments
		if( *script_p == '/' )
		{
			// comments //
			if( *( script_p + 1 ) == '/' )
			{
				script_p++;
				do {
					script_p++;
					if( !*script_p )
					{
						return 0;
					}
				} while( *script_p != '\n' );
				line++;
				script_p++;
				if( !*script_p )
				{
					return 0;
				}
				continue;
			}
			// comments /* */
			else if( *( script_p + 1 ) == '*' )
			{
				script_p++;
				while( 1 )
				{
					script_p++;
					if( !*script_p )
					{
						return 0;
					}
					if( *script_p == '\n' )
					{
						line++;
					}
					else if( *script_p == '/' )
					{
						if( *( script_p - 1 ) == '*' )
						{
							break;
						}
						if( *( script_p + 1 ) == '*' )
						{
							Warning( "nested comment" );
						}
					}
				}
				script_p++;
				if( !*script_p )
				{
					return false;
				}
				script_p++;
				if( !*script_p )
				{
					return false;
				}
				continue;
			}
		}
		break;
	}

	return true;
}

/*
================
idLexer::ReadEscapeCharacter
================
*/
bool idLexer::ReadEscapeCharacter( char* ch )
{
	int c, val, i;

	// step over the leading '\\'
	script_p++;

	// determine the escape character
	if( !( flags & LEXFL_NOEMITSTRINGESCAPECHARS ) )
	{
		switch( *script_p )
		{
			case '\\': c = '\\'; break;
			case 'n': c = '\n'; break;
			case 'r': c = '\r'; break;
			case 't': c = '\t'; break;
			case 'v': c = '\v'; break;
			case 'b': c = '\b'; break;
			case 'f': c = '\f'; break;
			case 'a': c = '\a'; break;
			case '\'': c = '\''; break;
			case '\"': c = '\"'; break;
			case '\?': c = '\?'; break;
			case 'x':
			{
				script_p++;
				for( i = 0, val = 0; ; i++, script_p++ )
				{
					c = *script_p;
					if( c >= '0' && c <= '9' )
						c = c - '0';
					else if( c >= 'A' && c <= 'Z' )
						c = c - 'A' + 10;
					else if( c >= 'a' && c <= 'z' )
						c = c - 'a' + 10;
					else
						break;
					val = ( val << 4 ) + c;
				}
				script_p--;
				if( val > 0xFF )
				{
					Warning( "too large value in escape character" );
					val = 0xFF;
				}
				c = val;
				break;
			}
			default: //NOTE: decimal ASCII code, NOT octal
			{
				if( *script_p < '0' || *script_p > '9' )
				{
					Error( "unknown escape char" );
					return false; //SEA ???
				}

				for( i = 0, val = 0; ; i++, script_p++ )
				{
					c = *script_p;
					if( c >= '0' && c <= '9' )
						c = c - '0';
					else
						break;
					val = val * 10 + c;
				}
				script_p--;
				if( val > 0xFF )
				{
					Warning( "too large value in escape character" );
					val = 0xFF;
				}
				c = val;
				break;
			}
		}
	}
	else {
		c = *script_p;
	}

	// step over the escape character or the last digit of the number
	script_p++;
	// store the escape character
	*ch = c;
	
	// succesfully read escape character
	return true;
}

/*
================
idLexer::ReadString

	Escape characters are interpretted.
	Reads two strings with only a white space between them as one string.
================
*/
bool idLexer::ReadString( idToken* token, int quote )
{
	int tmpline;
	const char* tmpscript_p;
	char ch;

	if( quote == '\"' ) {
		token->type = TT_STRING;
	} else {
		token->type = TT_LITERAL;
	}

	// leading quote
	script_p++;

	while( 1 )
	{
		// if there is an escape character and escape characters are allowed
		if( *script_p == '\\' && !( flags & LEXFL_NOSTRINGESCAPECHARS ) )
		{
			if( ( flags & LEXFL_NOEMITSTRINGESCAPECHARS ) )
			{
				token->AppendDirty( '\\' );
			}

			if( !ReadEscapeCharacter( &ch ) )
			{
				return false;
			}
			token->AppendDirty( ch );
		}
		// if a trailing quote
		else if( *script_p == quote )
		{
			// step over the quote
			script_p++;
			// if consecutive strings should not be concatenated
			if( ( flags & LEXFL_NOSTRINGCONCAT ) &&
				( !( flags & LEXFL_ALLOWBACKSLASHSTRINGCONCAT ) || ( quote != '\"' ) ) )
			{
				break;
			}

			tmpscript_p = script_p;
			tmpline = line;
			// read white space between possible two consecutive strings
			if( !SkipWhiteSpace( false ) ) // !idLexer::ReadWhiteSpace()
			{
				script_p = tmpscript_p;
				line = tmpline;
				break;
			}

			if( flags & LEXFL_NOSTRINGCONCAT )
			{
				if( *script_p != '\\' )
				{
					script_p = tmpscript_p;
					line = tmpline;
					break;
				}
				// step over the '\\'
				script_p++;
				if( !SkipWhiteSpace( false ) || ( *script_p != quote ) ) // !idLexer::ReadWhiteSpace()
				{
					Error( "expecting string after '\\' terminated line" );
					return false;
				}
			}

			// if there's no leading qoute
			if( *script_p != quote )
			{
				script_p = tmpscript_p;
				line = tmpline;
				break;
			}
			// step over the new leading quote
			script_p++;
		}
		else
		{
			if( *script_p == '\0' )
			{
				Error( "missing trailing quote" );
				return false;
			}
			if( *script_p == '\n' )
			{
				Error( "newline inside string" );
				return false;
			}
			token->AppendDirty( *script_p++ );
		}
	}
	token->data[ token->len ] = '\0';

	if( token->type == TT_LITERAL )
	{
		if( !( flags & LEXFL_ALLOWMULTICHARLITERALS ) )
		{
			if( token->Length() != 1 )
			{
				Warning( "literal is not one character long" );
			}
		}
		token->subtype = ( *token )[ 0 ];
	}
	else
	{
		// the sub type is the length of the string
		token->subtype = token->Length();
	}

	return true;
}

/*
================
idLexer::ReadName
================
*/
bool idLexer::ReadName( idToken* token )
{
	char c;
	token->type = TT_NAME;
	do {
		token->AppendDirty( *script_p++ );
		c = *script_p;
	} while( ( c >= 'a' && c <= 'z' ) ||
		( c >= 'A' && c <= 'Z' ) ||
		( c >= '0' && c <= '9' ) ||
		c == '_' ||
		// if treating all tokens as strings, don't parse '-' as a separate token
		( ( flags & LEXFL_ONLYSTRINGS ) && ( c == '-' ) ) ||
		// if special path name characters are allowed
		( ( flags & LEXFL_ALLOWPATHNAMES ) && ( c == '/' || c == '\\' || c == ':' || c == '.' ) ) );

	token->data[ token->len ] = '\0';
	//the sub type is the length of the name
	token->subtype = token->Length();
	
	return true;
}

/*
================
idLexer::CheckString
================
*/
ID_INLINE bool idLexer::CheckString( const char* str ) const
{
	for( int i = 0; str[ i ]; ++i )
	{
		if( script_p[ i ] != str[ i ] )
		{
			return false;
		}
	}
	return true;
}

/*
================
idLexer::ReadNumber
================
*/
bool idLexer::ReadNumber( idToken* token )
{
	int i;
	int dot;
	char c, c2;

	token->type = TT_NUMBER;
	token->subtype = 0;
	token->intvalue = 0;
	token->floatvalue = 0;

	c = *script_p;
	c2 = *( script_p + 1 );

	if( c == '0' && c2 != '.' )
	{
		// check for a hexadecimal number
		if( c2 == 'x' || c2 == 'X' )
		{
			token->AppendDirty( *script_p++ );
			token->AppendDirty( *script_p++ );
			c = *script_p;
			while( ( c >= '0' && c <= '9' ) ||
				( c >= 'a' && c <= 'f' ) ||
				( c >= 'A' && c <= 'F' ) )
			{
				token->AppendDirty( c );
				c = *( ++script_p );
			}
			token->subtype = TT_HEX | TT_INTEGER;
		}
		// check for a binary number
		else if( c2 == 'b' || c2 == 'B' )
		{
			token->AppendDirty( *script_p++ );
			token->AppendDirty( *script_p++ );
			c = *script_p;
			while( c == '0' || c == '1' )
			{
				token->AppendDirty( c );
				c = *( ++script_p );
			}
			token->subtype = TT_BINARY | TT_INTEGER;
		}
		// its an octal number
		else
		{
			token->AppendDirty( *script_p++ );
			c = *script_p;
			while( c >= '0' && c <= '7' )
			{
				token->AppendDirty( c );
				c = *( ++script_p );
			}
			token->subtype = TT_OCTAL | TT_INTEGER;
		}
	}
	else {
		// decimal integer or floating point number or ip address
		dot = 0;
		while( 1 )
		{
			if( c >= '0' && c <= '9' )
			{
			}
			else if( c == '.' )
			{
				dot++;
			}
			else
			{
				break;
			}
			token->AppendDirty( c );
			c = *( ++script_p );
		}
		// if a floating point number
		if( dot == 1 || c == 'e' )
		{
			token->subtype = TT_DECIMAL | TT_FLOAT;
			// check for floating point exponent
			if( c == 'e' )
			{
				c = *( ++script_p );
				if( c == '-' )
				{
					token->AppendDirty( c );
					c = *( ++script_p );
				}
				else if( c == '+' )
				{
					token->AppendDirty( c );
					c = *( ++script_p );
				}
				while( c >= '0' && c <= '9' )
				{
					token->AppendDirty( c );
					c = *( ++script_p );
				}
			}
			// check for floating point exception infinite 1.#INF or indefinite 1.#IND or NaN
			else if( c == '#' )
			{
				token->AppendDirty( c );
				c = *( ++script_p );
				if( CheckString( "INF" ) )
				{
					token->subtype |= TT_INFINITE;
					c2 = 3;
				}
				else if( CheckString( "IND" ) )
				{
					token->subtype |= TT_INDEFINITE;
					c2 = 3;
				}
				else if( CheckString( "NAN" ) )
				{
					token->subtype |= TT_NAN;
					c2 = 3;
				}
				else if( CheckString( "QNAN" ) )
				{
					token->subtype |= TT_NAN;
					c2 = 4;
				}
				else if( CheckString( "SNAN" ) )
				{
					token->subtype |= TT_NAN;
					c2 = 4;
				}
				for( i = 0; i < c2; i++ )
				{
					token->AppendDirty( c );
					c = *( ++script_p );
				}
				while( c >= '0' && c <= '9' )
				{
					token->AppendDirty( c );
					c = *( ++script_p );
				}
				if( !( flags & LEXFL_ALLOWFLOATEXCEPTIONS ) )
				{
					token->AppendDirty( 0 );	// zero terminate for c_str
					Error( "parsed %s", token->c_str() );
				}
			}
		}
		else if( dot > 1 )
		{
			if( !( flags & LEXFL_ALLOWIPADDRESSES ) )
			{
				Error( "more than one dot in number" );
				return false;
			}
			if( dot != 3 )
			{
				Error( "ip address should have three dots" );
				return false;
			}

			token->subtype = TT_IPADDRESS;
		}
		else {
			token->subtype = TT_DECIMAL | TT_INTEGER;
		}
	}

	if( token->subtype & TT_FLOAT )
	{
		if( c > ' ' )
		{
			// single-precision: float
			if( c == 'f' || c == 'F' )
			{
				token->subtype |= TT_SINGLE_PRECISION;
				script_p++;
			}
			// extended-precision: long double
			else if( c == 'l' || c == 'L' )
			{
				token->subtype |= TT_EXTENDED_PRECISION;
				script_p++;
			}
			// default is double-precision: double
			else
			{
				token->subtype |= TT_DOUBLE_PRECISION;
			}
		}
		else
		{
			token->subtype |= TT_DOUBLE_PRECISION;
		}
	}
	else if( token->subtype & TT_INTEGER )
	{
		if( c > ' ' )
		{
			// default: signed long
			for( i = 0; i < 2; i++ )
			{
				// long integer
				if( c == 'l' || c == 'L' )
				{
					token->subtype |= TT_LONG;
				}
				// unsigned integer
				else if( c == 'u' || c == 'U' )
				{
					token->subtype |= TT_UNSIGNED;
				}
				else
				{
					break;
				}
				c = *( ++script_p );
			}
		}
	}
	else if( token->subtype & TT_IPADDRESS )
	{
		if( c == ':' )
		{
			token->AppendDirty( c );
			c = *( ++script_p );
			while( c >= '0' && c <= '9' )
			{
				token->AppendDirty( c );
				c = *( ++script_p );
			}
			token->subtype |= TT_IPPORT;
		}
	}

	token->data[ token->len ] = '\0';
	return true;
}

/*
================
idLexer::ReadRawStringBlock
================
*/
bool idLexer::ReadRawStringBlock( idToken *token )
{
	token->type = TT_STRING;

	// leading identifier
	script_p += 2;

	while( true )
	{
		// if a trailing identifier
		if( script_p[ 0 ] == '%' && script_p[ 1 ] == '>' )
		{
			// step over the identifier
			script_p += 2;

			break;
		}
		else
		{
			if( *script_p == '\0' )
			{
				Error( "missing trailing identifier" );
				return false;
			}
			if( *script_p == '\n' )
			{
				line++;
			}
			token->AppendDirty( *script_p++ );
		}
	}
	token->data[ token->len ] = '\0';

	// the sub type is the length of the string
	token->subtype = token->Length();

	return true;
}

/*
================
idLexer::ReadPunctuation
================
*/
bool idLexer::ReadPunctuation( idToken* token )
{
	int l, n, i;
	const char* p;
	const punctuation_t* punc;

#ifdef PUNCTABLE
	for( n = punctuationtable[ ( unsigned int ) * ( script_p ) ]; n >= 0; n = nextpunctuation[ n ] )
	{
		punc = &( punctuations[ n ] );
	#else
	for( n = 0; punctuations[ n ].p; n++ )
	{
		punc = &punctuations[ n ];
	#endif
		p = punc->p;
		// check for this punctuation in the script
		for( l = 0; p[ l ] && script_p[ l ]; l++ )
		{
			if( script_p[ l ] != p[ l ] )
			{
				break;
			}
		}
		if( !p[ l ] )
		{
			//
			token->EnsureAlloced( l + 1, false );
			for( i = 0; i <= l; i++ )
			{
				token->data[ i ] = p[ i ];
			}
			token->len = l;
			//
			script_p += l;
			token->type = TT_PUNCTUATION;
			// sub type is the punctuation id
			token->subtype = punc->n;
			return true;
		}
	}
	return false;
}

/*
================
idLexer::ReadToken
================
*/
bool idLexer::ReadToken( idToken* token )
{
	if( !loaded )
	{
		idLib::common->Error( "idLexer::ReadToken: no file loaded" );
		return false;
	}
	if( script_p == NULL )
	{
		return false;
	}

	// if there is a token available (from unreadToken)
	if( tokens.Num() )
	{
		*token = tokens[ tokens.Num() - 1 ];
		tokens.RemoveIndex( tokens.Num() - 1 );
		return true;
	}

	// read from binary file if loaded
	if( binary.IsLoaded() ) 
	{
		return binary.ReadToken( token );
	}

	// save script pointer
	lastScript_p = script_p;
	// save line counter
	lastline = line;
	// clear the token stuff
	token->data[ 0 ] = '\0';
	token->len = 0;
	// start of the white space
	whiteSpaceStart_p = script_p;
	token->whiteSpaceStart_p = script_p;
	// read white space before token
	if( !SkipWhiteSpace( false ) )
	//if( !ReadWhiteSpace() )
	{
		return false;
	}

	// end of the white space
	whiteSpaceEnd_p = script_p;
	token->whiteSpaceEnd_p = script_p;
	// line the token is on
	token->line = line;
	// number of lines crossed before token
	token->linesCrossed = line - lastline;
	// clear token flags
	token->flags = 0;

	int c = *script_p;

	// if we're keeping everything as whitespace deliminated strings
	if( flags & LEXFL_ONLYSTRINGS )
	{
		// if there is a leading quote
		if( c == '\"' || c == '\'' )
		{
			if( !ReadString( token, c ) )
			{
				return false;
			}
		}
		else if( !ReadName( token ) )
		{
			return false;
		}
	}
	// if there is a number
	else if( ( c >= '0' && c <= '9' ) ||
		( c == '.' && ( *( script_p + 1 ) >= '0' && *( script_p + 1 ) <= '9' ) ) )
	{
		if( !ReadNumber( token ) )
		{
			return false;
		}
		// if names are allowed to start with a number
		if( flags & LEXFL_ALLOWNUMBERNAMES )
		{
			c = *script_p;
			if( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || c == '_' )
			{
				if( !ReadName( token ) )
				{
					return false;
				}
			}
		}
	}
	// if there is a leading quote
	else if( c == '\"' || c == '\'' )
	{
		if( !ReadString( token, c ) )
		{
			return false;
		}
	}
	// if there is a name
	else if( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || c == '_' )
	{
		if( !ReadName( token ) )
		{
			return false;
		}
	}
	// names may also start with a slash when pathnames are allowed
	else if( ( flags & LEXFL_ALLOWPATHNAMES ) && ( ( c == '/' || c == '\\' ) || c == '.' ) )
	{
		if( !ReadName( token ) )
		{
			return false;
		}
	}
	// if there is a raw string block
	else if( ( flags & LEXFL_ALLOWRAWSTRINGBLOCKS ) && ( script_p[ 0 ] == '<' && script_p[ 1 ] == '%' ) )
	{
		if( !ReadRawStringBlock( token ) )
		{
			return false;
		}
	}
	// check for punctuations
	else if( !ReadPunctuation( token ) )
	{
		Error( "unknown punctuation %c", c );
		return false;
	}

	// succesfully read a token
	return true;
}

/*
================
idLexer::ExpectTokenString
================
*/
bool idLexer::ExpectTokenString( const char *string )
{
	idToken token;

	if( !ReadToken( &token ) )
	{
		Error( "couldn't find expected '%s'", string );
		return false;
	}
	if( token != string )
	{
		Error( "expected '%s' but found '%s'", string, token.c_str() );
		return false;
	}

	return true;
}

/*
================
idLexer::ExpectTokenType
================
*/
bool idLexer::ExpectTokenType( int type, int subtype, idToken & token )
{
	idStr str;

	if( !ReadToken( &token ) )
	{
		Error( "couldn't read expected token" );
		return false;
	}

	if( token.type != type )
	{
		switch( type )
		{
			case TT_STRING: str = "string"; break;
			case TT_LITERAL: str = "literal"; break;
			case TT_NUMBER: str = "number"; break;
			case TT_NAME: str = "name"; break;
			case TT_PUNCTUATION: str = "punctuation"; break;
			default: str = "unknown type"; break;
		}
		Error( "expected a %s but found '%s'", str.c_str(), token.c_str() );
		return false;
	}
	if( token.type == TT_NUMBER )
	{
		if( ( token.subtype & subtype ) != subtype )
		{
			str.Clear();
			if( subtype & TT_DECIMAL ) str = "decimal ";
			if( subtype & TT_HEX ) str = "hex ";
			if( subtype & TT_OCTAL ) str = "octal ";
			if( subtype & TT_BINARY ) str = "binary ";
			if( subtype & TT_UNSIGNED ) str += "unsigned ";
			if( subtype & TT_LONG ) str += "long ";
			if( subtype & TT_FLOAT ) str += "float ";
			if( subtype & TT_INTEGER ) str += "integer ";
			str.StripTrailing( ' ' );
			Error( "expected %s but found '%s'", str.c_str(), token.c_str() );
			return false;
		}
	}
	else if( token.type == TT_PUNCTUATION )
	{
		if( subtype < 0 )
		{
			Error( "BUG: wrong punctuation subtype" );
			return false;
		}
		if( token.subtype != subtype )
		{
			Error( "expected '%s' but found '%s'", GetPunctuationFromId( subtype ), token.c_str() );
			return false;
		}
	}

	return true;
}

/*
================
idLexer::ExpectAnyToken
================
*/
bool idLexer::ExpectAnyToken( idToken *token )
{
	if( !ReadToken( token ) )
	{
		Error( "couldn't read expected token" );
		return false;
	}
	return true;
}

/*
================
idLexer::CheckTokenString
================
*/
bool idLexer::CheckTokenString( const char *string, idToken *tok )
{
	idToken _token;
	if( !tok ) {
		tok = &_token;
	}

	if( !ReadToken( tok ) )
	{
		return false;
	}
	// if the given string is available
	if( *tok == string )
	{
		return true;
	}
	// unread token
	UnreadToken( tok );
	return false;
}

/*
================
idLexer::CheckTokenType
================
*/
bool idLexer::CheckTokenType( int type, int subtype, idToken *token )
{
	idToken tok;

	if( !ReadToken( &tok ) )
	{
		return false;
	}
	// if the type matches
	if( tok.type == type && ( tok.subtype & subtype ) == subtype )
	{
		*token = tok;
		return true;
	}
	// unread token
	UnreadToken( &tok );
	return false;
}

/*
================
idLexer::PeekTokenString
================
*/
bool idLexer::PeekTokenString( const char* string )
{
	idToken tok;

	if( !ReadToken( &tok ) )
	{
		return false;
	}

	// unread token
	UnreadToken( &tok );

	// if the given string is available
	if( tok == string )
	{
		return true;
	}

	return false;
}

/*
================
idLexer::PeekTokenType
================
*/
bool idLexer::PeekTokenType( int type, int subtype, idToken* token )
{
	idToken tok;

	if( !ReadToken( &tok ) )
	{
		return false;
	}

	// unread token
	UnreadToken( &tok );

	// if the type matches
	if( tok.type == type && ( tok.subtype & subtype ) == subtype )
	{
		*token = tok;
		return true;
	}

	return false;
}

/*
================
idLexer::SkipUntilString
================
*/
bool idLexer::SkipUntilString( const char *string, idToken* token )
{
	idToken _token;
	if( !token )
	{
		token = &_token;
	}

	while( ReadToken( token ) )
	{
		if( *token == string )
		{
			return true;
		}
	}

	return false;
}

/*
================
idLexer::SkipRestOfLine
================
*/
bool idLexer::SkipRestOfLine()
{
	idToken token;

	while( ReadToken( &token ) )
	{
		if( token.linesCrossed )
		{
			UnreadToken( &token );
			return true;
		}
	}
	return false;
}

/*
=================
idLexer::SkipBracedSection

Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
bool idLexer::SkipBracedSection( bool parseFirstBrace )
{
	idToken token;
	int depth = parseFirstBrace ? 0 : 1;
	do {
		if( !ReadToken( &token ) )
		{
			return false;
		}
		if( token.type == TT_PUNCTUATION )
		{
			if( token == "{" )
			{
				depth++;
			}
			else if( token == "}" )
			{
				depth--;
			}
		}
	} 
	while( depth );

	return true;
}

/*
=================
idLexer::SkipBracedSectionExact

Skips until a matching close brace is found.
Internal brace depths are properly skipped.
Reads exact characters between braces.
=================
*/
bool idLexer::SkipBracedSectionExact( int tabs, bool parseFirstBrace )
{
	if( parseFirstBrace )
	{
		if( !ExpectTokenString( "{" ) )
		{
			return false;
		}
	}

	int depth = 1;
	bool skipWhite = false;
	bool doTabs = ( tabs >= 0 );

	while( depth )
	{
		if( !*script_p )
		{
			return false;
		}

		char c = *( script_p++ );

		switch( c )
		{
			case '\t':
			case ' ':
			{
				if( skipWhite )
				{
					continue;
				}
				break;
			}
			case '\n':
			{
				if( doTabs )
				{
					skipWhite = true;
					continue;
				}
				line++;
				break;
			}
			case '{':
			{
				depth++;
				tabs++;
				break;
			}
			case '}':
			{
				depth--;
				tabs--;
				break;
			}
		}

		if( skipWhite )
		{
			skipWhite = false;
		}
	}

	return true;
}

/*
================
idLexer::UnreadToken
================
*/
void idLexer::UnreadToken( const idToken *token )
{
	idToken& tokenLocal = tokens.Alloc();
	tokenLocal = *token;
}

/*
================
idLexer::ReadTokenOnLine
================
*/
bool idLexer::ReadTokenOnLine( idToken *token )
{
	idToken tok;

	if( !ReadToken( &tok ) )
	{
		return false;
	}

	// if no lines were crossed before this token
	if( !tok.linesCrossed )
	{
		*token = tok;
		return true;
	}

	// restore our position
	UnreadToken( &tok );

	token->Clear();
	return false;
}

/*
================
idLexer::ReadRestOfLine
================
*/
const char*	idLexer::ReadRestOfLine( idStr& out )
{
	while( 1 )
	{
		if( *script_p == '\n' )
		{
			line++;
			break;
		}

		if( !*script_p )
		{
			break;
		}

		if( *script_p <= ' ' )
		{
			out += " ";
		}
		else
		{
			out += *script_p;
		}
		script_p++;
	}

	out.Strip( ' ' );
	return out.c_str();
}

/*
================
idLexer::ParseInt
================
*/
int idLexer::ParseInt()
{
	idToken token;

	if( !idLexer::ReadToken( &token ) )
	{
		Error( "couldn't read expected integer" );
		return 0;
	}
	if( token.type == TT_PUNCTUATION && token == "-" )
	{
		ExpectTokenType( TT_NUMBER, TT_INTEGER, token );
		return -( ( signed int )token.GetIntValue() );
	}
	else if( token.type != TT_NUMBER || token.subtype == TT_FLOAT )
	{
		Error( "expected integer value, found '%s'", token.c_str() );
	}
	return token.GetIntValue();
}

/*
================
idLexer::ParseBool
================
*/
bool idLexer::ParseBool()
{
	idToken token;

	if( !ExpectTokenType( TT_NUMBER, 0, token ) )
	{
		Error( "couldn't read expected boolean" );
		return false;
	}
	return( token.GetIntValue() != 0 );
}

/*
================
idLexer::ParseFloat
================
*/
float idLexer::ParseFloat( bool* errorFlag )
{
	idToken token;

	if( errorFlag )
	{
		*errorFlag = false;
	}

	if( !ReadToken( &token ) )
	{
		if( errorFlag )
		{
			Warning( "couldn't read expected floating point number" );
			*errorFlag = true;
		}
		else {
			Error( "couldn't read expected floating point number" );
		}
		return 0;
	}
	if( token.type == TT_PUNCTUATION && token == "-" )
	{
		ExpectTokenType( TT_NUMBER, 0, token );
		return -token.GetFloatValue();
	}
	else if( token.type != TT_NUMBER )
	{
		if( errorFlag )
		{
			Warning( "expected float value, found '%s'", token.c_str() );
			*errorFlag = true;
		}
		else {
			Error( "expected float value, found '%s'", token.c_str() );
		}
	}
	return token.GetFloatValue();
}

/*
================
idLexer::Parse1DMatrix
================
*/
bool idLexer::Parse1DMatrix( int x, float *m, bool expectCommas )
{
	if( !idLexer::ExpectTokenString( "(" ) )
	{
		return false;
	}

	for( int i = 0; i < x; i++ )
	{
		m[ i ] = ParseFloat();
		if( expectCommas && i != x - 1 )
		{
			if( !ExpectTokenString( "," ) )
			{
				return false;
			}
		}
	}

	if( !ExpectTokenString( ")" ) )
	{
		return false;
	}

	return true;
}

// RB begin
bool idLexer::Parse1DMatrixJSON( int x, float* m )
{
	if( !ExpectTokenString( "[" ) )
	{
		return false;
	}

	for( int i = 0; i < x; i++ )
	{
		m[ i ] = ParseFloat();

		if( i < ( x - 1 ) && !ExpectTokenString( "," ) )
		{
			return false;
		}
	}

	if( !ExpectTokenString( "]" ) )
	{
		return false;
	}

	return true;
}
// RB end

/*
================
idLexer::Parse2DMatrix
================
*/
bool idLexer::Parse2DMatrix( int y, int x, float* m )
{
	if( !ExpectTokenString( "(" ) )
	{
		return false;
	}

	for( int i = 0; i < y; i++ )
	{
		if( !Parse1DMatrix( x, m + i * x ) )
		{
			return false;
		}
	}

	if( !ExpectTokenString( ")" ) )
	{
		return false;
	}

	return true;
}

/*
================
idLexer::Parse3DMatrix
================
*/
bool idLexer::Parse3DMatrix( int z, int y, int x, float* m )
{
	if( !ExpectTokenString( "(" ) )
	{
		return false;
	}

	for( int i = 0; i < z; i++ )
	{
		if( !Parse2DMatrix( y, x, m + i * x * y ) )
		{
			return false;
		}
	}

	if( !ExpectTokenString( ")" ) )
	{
		return false;
	}

	return true;
}

/*
=================
idLexer::ParseBracedSectionExact

The next token should be an open brace.
Parses until a matching close brace is found.
Maintains exact characters between braces.

  FIXME: this should use ReadToken and replace the token white space with correct indents and newlines
=================
*/
bool idLexer::ParseBracedSectionExact( idStr &out, int tabs, bool parseFirstBrace )
{
	int		depth;
	bool	doTabs;
	bool	skipWhite;

	out.Empty();

	if( parseFirstBrace )
	{
		if( !ExpectTokenString( "{" ) )
		{
			return false;
		}
		out = "{";
	}

	depth = 1;
	skipWhite = false;
	doTabs = ( tabs >= 0 );

	while( depth )
	{
		if( !*script_p )
		{
			return false;
		}

		char c = *( script_p++ );

		switch( c )
		{
			case '\t':
			case ' ':
			{
				if( skipWhite )
				{
					continue;
				}
				break;
			}
			case '\n':
			{
				if( doTabs )
				{
					skipWhite = true;
					out += c;
					continue;
				}
				line++;
				break;
			}
			case '{':
			{
				depth++;
				tabs++;
				break;
			}
			case '}':
			{
				depth--;
				tabs--;
				break;
			}
		}

		if( skipWhite )
		{
			int i = tabs;
			if( c == '{' )
			{
				i--;
			}
			skipWhite = false;
			for( ; i > 0; i-- )
			{
				out += '\t';
			}
		}
		out += c;
	}

	return true;
}

/*
=================
idLexer::ParseBracedSection

The next token should be an open brace.
Parses until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
const char *idLexer::ParseBracedSection( idStr &out, int tabs, bool parseFirstBrace, char intro, char outro )
{
	char temp[ 2 ] = { '\0', '\0' };
	temp[ 0 ] = intro;

	out.Empty();
	if( parseFirstBrace )
	{
		if( !ExpectTokenString( temp ) )
		{
			return out.c_str();
		}
		out = temp;
	}

	idToken token;
	int i, depth = 1;
	bool doTabs = ( tabs >= 0 );

	do {
		if( !ReadToken( &token ) )
		{
			Error( "missing closing brace" );
			return out.c_str();
		}

		// if the token is on a new line
		for( i = 0; i < token.linesCrossed; i++ )
		{
			out += "\r\n";
		}

		if( doTabs && token.linesCrossed )
		{
			i = tabs;
			if( token[ 0 ] == outro && i > 0 )
			{
				i--;
			}
			while( i-- > 0 )
			{
				out += "\t";
			}
		}
	#if 0
		if( token.type != TT_STRING )
		{
			if( token[ 0 ] == intro )
			{
				depth++;
				if( doTabs )
				{
					tabs++;
				}
			}
			else if( token[ 0 ] == outro )
			{
				depth--;
				if( doTabs )
				{
					tabs--;
				}
			}
			if( out.Length() )
			{
				out += " ";
			}
			out += token;
		}
		else
		{
			if( out.Length() )
			{
				out += " ";
			}
			out += "\"" + token + "\"";
		}
	#endif
		if( token.type == TT_STRING )
		{
			if( out.Length() )
			{
				out += " ";
			}
			out += "\"" + token + "\"";
		}
		else if ( token.type == TT_LITERAL )
		{
			if( out.Length() )
			{
				out += " ";
			}
			out += "\'" + token + "\'";
		}
		else
		{
			if( token[ 0 ] == intro )
			{
				depth++;
				if( doTabs )
				{
					tabs++;
				}
			}
			else if( token[ 0 ] == outro )
			{
				depth--;
				if( doTabs )
				{
					tabs--;
				}
			}

			if( out.Length() )
			{
				out += " ";
			}
			out += token;
		}
	} 
	while( depth );

	return out.c_str();
}

/*
=================
idLexer::ParseRestOfLine

	Like ParseCompleteLine, but doesn't advance to next line and doesn't return \n
	It does, however, trim leading and trailing white space
=================
*/
const char *idLexer::ParseRestOfLine( idStr &out )
{
	idToken token;
	const char* start = script_p;

	while( 1 )
	{
		// end of buffer
		if( *script_p == 0 )
		{
			break;
		}
		if( *script_p == '\n' )
		{
			break;
		}
		script_p++;
	}

	// Trim leading white space
	while( *start <= ' ' && start < script_p )
	{
		start++;
	}
	// Trim trailing white space
	while( *( script_p - 1 ) <= ' ' && script_p > start )
	{
		script_p--;
	}

	out.Empty();
	out.Append( start, script_p - start );

	return out.c_str();
}

/*
========================
idLexer::ParseCompleteLine

Returns a string up to the \n, but doesn't eat any whitespace at the beginning of the next line.
========================
*/
const char* idLexer::ParseCompleteLine( idStr& out )
{
	idToken token;
	const char* start = script_p;

	while( 1 )
	{
		// end of buffer
		if( *script_p == 0 )
		{
			break;
		}
		if( *script_p == '\n' )
		{
			line++;
			script_p++;
			break;
		}
		script_p++;
	}

	out.Empty();
	out.Append( start, script_p - start );

	return out.c_str();
}

/*
================
idLexer::GetNextWhiteSpace
================
*/
int idLexer::GetNextWhiteSpace( idStr &whiteSpace, bool currentLine )
{
	whiteSpaceStart_p = script_p;
	SkipWhiteSpace( currentLine );
	whiteSpaceEnd_p = script_p;
	return GetLastWhiteSpace( whiteSpace );
}

/*
================
idLexer::GetLastWhiteSpace
================
*/
int idLexer::GetLastWhiteSpace( idStr& whiteSpace ) const
{
	whiteSpace.Clear();
	for( const char* p = whiteSpaceStart_p; p < whiteSpaceEnd_p; p++ )
	{
		whiteSpace.Append( *p );
	}
	return whiteSpace.Length();
}

/*
================
idLexer::GetLastWhiteSpaceStart
================
*/
int idLexer::GetLastWhiteSpaceStart() const
{
	return whiteSpaceStart_p - buffer;
}

/*
================
idLexer::GetLastWhiteSpaceEnd
================
*/
int idLexer::GetLastWhiteSpaceEnd() const
{
	return whiteSpaceEnd_p - buffer;
}

/*
================
idLexer::Reset
================
*/
void idLexer::Reset()
{
	// pointer in script buffer
	script_p = buffer;
	// pointer in script buffer before reading token
	lastScript_p = buffer;
	// begin of white space
	whiteSpaceStart_p = NULL;
	// end of white space
	whiteSpaceEnd_p = NULL;

	tokens.Clear();

	line = 1;
	lastline = 1;
}

/*
================
idLexer::EndOfFile
================
*/
bool idLexer::EndOfFile( void ) const
{
	if( binary.IsLoaded() )
	{
		return binary.EndOfFile();
	}
	return( script_p >= end_p );
}

/*
================
idLexer::NumLinesCrossed
================
*/
int idLexer::NumLinesCrossed() const
{
	return( line - lastline );
}

/*
================
idLexer::LoadFile
================
*/
bool idLexer::LoadFile( const char *filename, bool OSPath, int startLine )
{
	idFile* fp;
	idStr pathname;
	int length;
	char* buf;

	if( loaded )
	{
		idLib::common->Error( "idLexer::LoadFile: another script already loaded" );
		return false;
	}
	/*
		if ( !OSPath ) {
			if ( binary.ReadFile( filename ) ) {
				this->loaded = true;
				this->filename = fileSystem->RelativePathToOSPath( filename, "fs_basepath" );
				this->filename.CollapsePath();
				return true;
			}
		}
	*/
	if( !OSPath && ( baseFolder[ 0 ] != '\0' ) )
	{
		pathname = va( "%s/%s", baseFolder, filename );
	}
	else
	{
		pathname = filename;
	}
	if( OSPath )
	{
		fp = idLib::fileSystem->OpenExplicitFileRead( pathname );
	}
	else
	{
		fp = idLib::fileSystem->OpenFileRead( pathname );
	}
	if( !fp )
	{
		return false;
	}

	length = fp->Length();
	buf = idMem::Alloc< char, TAG_IDLIB_LEXER >( length + 1 );
	buf[ length ] = '\0';
	fp->Read( buf, length );
	this->fileTime = fp->Timestamp();
	if( OSPath )
	{
		this->filename = fp->GetFullPath();
	}
	else
	{
		this->filename = pathname;
	}
	this->filename.CollapsePath();
	idLib::fileSystem->CloseFile( fp );

	this->buffer = buf;
	this->length = length;
	// pointer in script buffer
	this->script_p = this->buffer;
	// pointer in script buffer before reading token
	this->lastScript_p = this->buffer;
	// pointer to end of script buffer
	this->end_p = &( this->buffer[ length ] );

	tokens.Clear();

	this->line = startLine;
	this->lastline = startLine;
	this->allocated = true;
	this->loaded = true;
	/*
		if ( !OSPath ) {
			binary.WriteFile( *this, pathname );
		}
	*/
	return true;
}

/*
================
idLexer::LoadMemory
================
*/
bool idLexer::LoadMemory( const char *ptr, int length, const char *name, int startLine )
{
	if( loaded )
	{
		idLib::common->Error( "idLexer::LoadMemory: another script already loaded" );
		return false;
	}
	filename = name;
	filename.CollapsePath();
	buffer = ptr;
	fileTime = 0;
	this->length = length;
	// pointer in script buffer
	script_p = buffer;
	// pointer in script buffer before reading token
	lastScript_p = buffer;
	// pointer to end of script buffer
	end_p = &( buffer[ length ] );

	tokens.Clear();
	line = startLine;
	lastline = startLine;
	allocated = false;
	loaded = true;

	return true;
}

/*
================
idLexer::LoadMemoryBinary
================
*/
bool idLexer::LoadMemoryBinary( const byte* ptr, int length, const char *name, idTokenCache* globals )
{
	if( ptr == NULL || length == 0 )
	{
		return false;
	}
	filename = name;
	filename.CollapsePath();
	buffer = NULL;
	fileTime = 0;
	this->length = 0;
	// pointer in script buffer
	script_p = NULL;

	lastScript_p = NULL;
	// pointer to end of script buffer
	end_p = NULL;

	tokens.Clear();
	line = 0;
	lastline = 0;
	allocated = false;
	loaded = true;

	//idLibFileMemoryPtr memFile( idLib::fileSystem->OpenMemoryFile( name ) );
	//memFile->SetData( reinterpret_cast< const char* >( ptr ), length );
	//SEA ???
	idFile_Memory mf( name, reinterpret_cast< const char* >( ptr ), length );
	idLibFileMemoryPtr memFile( &mf );

	bool retVal = binary.Read( memFile.Get() );
	binary.SetData( NULL, globals );
	return retVal;
}

/*
============
idLexer::LoadTokenStream
============
*/
bool idLexer::LoadTokenStream( const idList<unsigned short>& indices, const idTokenCache& tokens, const char* name )
{
	filename = name;
	filename.CollapsePath();
	buffer = NULL;
	fileTime = 0;
	length = 0;
	// pointer in script buffer
	script_p = NULL;

	lastScript_p = NULL;
	// pointer to end of script buffer
	end_p = NULL;

	this->tokens.Clear();
	line = 0;
	lastline = 0;
	allocated = false;
	loaded = true;

	binary.Clear();
	binary.SetData( &indices, &tokens );

	return true;
}

/*
================
idLexer::FreeSource
================
*/
void idLexer::FreeSource()
{
#ifdef PUNCTABLE
	if( punctuationtable && punctuationtable != default_punctuationtable )
	{
		idMem::Free( ( void* )punctuationtable );
		punctuationtable = NULL;
	}
	if( nextpunctuation && nextpunctuation != default_nextpunctuation )
	{
		idMem::Free( ( void* )nextpunctuation );
		nextpunctuation = NULL;
	}
#endif //PUNCTABLE
	if( allocated )
	{
		idMem::Free( ( void* )buffer );
		buffer = NULL;
		allocated = false;
	}
	filename.Clear();
	tokens.Clear();
	loaded = false;
	binary.Clear();
}

/*
================
idLexer::idLexer
================
*/
idLexer::idLexer()
{
	loaded = false;
	filename = "";
	flags = 0;
	SetPunctuations( NULL );
	allocated = false;
	fileTime = 0;
	length = 0;
	line = 0;
	lastline = 0;
	tokens.Clear();
	next = NULL;
	hadError = false;
	hadWarning = false;
}

/*
================
idLexer::idLexer
================
*/
idLexer::idLexer( int flags )
{
	loaded = false;
	filename = "";
	this->flags = flags;
	SetPunctuations( NULL );
	allocated = false;
	fileTime = 0;
	length = 0;
	line = 0;
	lastline = 0;
	tokens.Clear();
	next = NULL;
	hadError = false;
	hadWarning = false;
}

/*
================
idLexer::idLexer
================
*/
idLexer::idLexer( const char *filename, int flags, bool OSPath, int startLine )
{
	idLexer::loaded = false;
	this->flags = flags;
	SetPunctuations( NULL );
	allocated = false;
	tokens.Clear();
	next = NULL;
	hadError = false;
	hadWarning = false;
	LoadFile( filename, OSPath, startLine );
}

/*
================
idLexer::idLexer
================
*/
idLexer::idLexer( const char *ptr, int length, const char *name, int flags, int startLine )
{
	loaded = false;
	this->flags = flags;
	SetPunctuations( NULL );
	allocated = false;
	tokens.Clear();
	next = NULL;
	hadError = false;
	hadWarning = false;
	LoadMemory( ptr, length, name, startLine );
}

/*
================
idLexer::~idLexer
================
*/
idLexer::~idLexer()
{
	idLexer::FreeSource();
}

/*
================
idLexer::SetBaseFolder
================
*/
void idLexer::SetBaseFolder( const char* path )
{
	idStr::Copynz( baseFolder, path, sizeof( baseFolder ) );
}