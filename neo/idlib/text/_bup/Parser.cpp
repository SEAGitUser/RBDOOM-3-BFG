/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014 Robert Beckebans

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

//#define DEBUG_EVAL
#define MAX_DEFINEPARMS				128
#define DEFINEHASHSIZE				2048

#define TOKEN_FL_RECURSIVE_DEFINE	1

idParser::defineList_t idParser::globalDefines;

/*
================
idParser::SetBaseFolder
================
*/
void idParser::SetBaseFolder( const char* path )
{
	idLexer::SetBaseFolder( path );
}

/*
================
idParser::AddGlobalDefine
================
*/
bool idParser::AddGlobalDefine( const char *string )
{
	auto define = DefineFromString( string );
	if( !define ) {
		return false;
	}

	//globalDefines.AddToFront( define );
	globalDefines.AddToEnd( define );
	return true;
}

/*
================
idParser::RemoveGlobalDefine
================
*/
bool idParser::RemoveGlobalDefine( const char *name )
{
	define_t *def = nullptr;
	for( def = globalDefines.GetFirst(); def != NULL; def = globalDefines.GetNext( def ) )
	{
		if( !idStr::Cmp( def->name, name ) ) {
			break;
		}
	}
	if( def != nullptr )
	{
		globalDefines.Remove( def );
		FreeDefine( def );
		return true;
	}

	return false;
}

/*
================
idParser::RemoveAllGlobalDefines
================
*/
void idParser::RemoveAllGlobalDefines( void )
{
	for( auto def = globalDefines.GetFirst(); def != NULL; def = globalDefines.GetNext( def ) )
	{
		globalDefines.Remove( def );
		FreeDefine( def );
	}
}

/*
===============================================================================

idParser

===============================================================================
*/

/*
================
idParser::PrintDefine
================
*/
void idParser::PrintDefine( define_t* define )
{
	idLib::Printf( "define->name = %s\n", define->name );
	idLib::Printf( "define->flags = %d\n", define->flags );
	idLib::Printf( "define->builtin = %d\n", define->builtin );
	idLib::Printf( "define->numparms = %d\n", define->numparms );
}

/*
================
PC_PrintDefineHashTable
================
* /
static void PC_PrintDefineHashTable(define_t **definehash) {
	int i;
	define_t *d;

	for (i = 0; i < DEFINEHASHSIZE; i++) {
		Log_Write("%4d:", i);
		for (d = definehash[i]; d; d = d->hashnext) {
			Log_Write(" %s", d->name);
		}
		Log_Write("\n");
	}
}
*/

/*
================
PC_NameHash
================
*/
ID_INLINE int PC_NameHash( const char* name )
{
	int hash = idStr::Hash( name );
	hash = ( hash ^ ( hash >> 10 ) ^ ( hash >> 20 ) ) & ( DEFINEHASHSIZE - 1 );
	return hash;
}

/*
================
idParser::AddDefineToHash
================
*/
void idParser::AddDefineToHash( define_t* define, define_t** definehash )
{
	int hash = PC_NameHash( define->name );
	define->hashnext = definehash[ hash ];
	definehash[ hash ] = define;
}

/*
================
FindHashedDefine
================
*/
define_t *idParser::FindHashedDefine( define_t **definehash, const char *name )
{
	int hash = PC_NameHash( name );
	for( define_t *d = definehash[ hash ]; d != NULL; d = d->hashnext )
	{
		if( !idStr::Cmp( d->name, name ) )
		{
			return d;
		}
	}
	return NULL;
}

/*
================
idParser::FindDefine
================
*/
define_t *idParser::FindDefine( define_t *defines, const char *name )
{
	for( define_t *d = defines; d != NULL; d = d->next )
	{
		if( !idStr::Cmp( d->name, name ) )
		{
			return d;
		}
	}
	return NULL;
}

/*
================
idParser::FindDefineParm
================
*/
int idParser::FindDefineParm( define_t *define, const char *name )
{
	int i = 0;
	for( idToken *p = define->parms; p != NULL; p = p->next )
	{
		if( ( *p ) == name )
		{
			return i;
		}
		i++;
	}
	return -1;
}

/*
================
idParser::CopyDefine
================
*/
define_t *idParser::CopyDefine( define_t *define )
{
	idToken *token, *newtoken, *lasttoken;

	int nameLength = idStr::Length( define->name ) + 1;
	auto newdefine = idMem::Alloc<define_t, TAG_IDLIB_PARSER>( sizeof( define_t ) + nameLength );

	// copy the define name
	newdefine->name = ( char * )newdefine + sizeof( define_t );
	idStr::Copynz( newdefine->name, define->name, nameLength );

	newdefine->flags = define->flags;
	newdefine->builtin = define->builtin;
	newdefine->numparms = define->numparms;
	newdefine->scope = define->scope;

	// the define is not linked
	newdefine->next = NULL;
	newdefine->hashnext = NULL;

	// copy the define tokens
	newdefine->tokens = NULL;
	for( lasttoken = NULL, token = define->tokens; token != NULL; token = token->next )
	{
		newtoken = new( TAG_IDLIB_PARSER ) idToken( *token );
		newtoken->next = NULL;
		if( lasttoken != NULL )
		{
			lasttoken->next = newtoken;
		}
		else {
			newdefine->tokens = newtoken;
		}
		lasttoken = newtoken;
	}

	// copy the define parameters
	newdefine->parms = NULL;
	for( lasttoken = NULL, token = define->parms; token; token = token->next )
	{
		newtoken = new( TAG_IDLIB_PARSER ) idToken( *token );
		newtoken->next = NULL;
		if( lasttoken != NULL )
		{
			lasttoken->next = newtoken;
		}
		else {
			newdefine->parms = newtoken;
		}
		lasttoken = newtoken;
	}

	return newdefine;
}

/*
================
idParser::FreeDefine
================
*/
void idParser::FreeDefine( define_t *define )
{
	idToken *t, *next;

	// free the define parameters
	for( t = define->parms; t != NULL; t = next )
	{
		next = t->next;
		delete t;
	}
	// free the define tokens
	for( t = define->tokens; t != NULL; t = next )
	{
		next = t->next;
		delete t;
	}
	// free the define
	idMem::Free( define );
}

/*
================
idParser::DefineFromString
================
*/
define_t *idParser::DefineFromString( const char *string )
{
	idParser src;
	if( !src.LoadMemory( string, idStr::Length( string ), "*defineString" ) )
	{
		return NULL;
	}
	// create a define from the source
	if( !src.Directive_define( false ) )
	{
		src.FreeSource();
		return NULL;
	}
	auto def = src.CopyFirstDefine();
	src.FreeSource();
	// if the define was created succesfully
	return def;
}

/*
================
idParser::Error
================
*/
void idParser::Error( const char *str, ... ) const
{
	char text[ MAX_STRING_CHARS ];
	va_list ap;

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );
	if( scriptstack != NULL )
	{
		scriptstack->Error( text );
	}
}

/*
================
idParser::Warning
================
*/
void idParser::Warning( const char *str, ... ) const
{
	char text[ MAX_STRING_CHARS ];
	va_list ap;

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );
	if( scriptstack != NULL )
	{
		scriptstack->Warning( text );
	}
}

/*
================
idParser::PushIndent
================
*/
void idParser::PushIndent( int type, int skip, int skipElse )
{
	auto indent = idMem::Alloc<indent_t, TAG_IDLIB_PARSER>( sizeof( indent_t ) );
	indent->type = type;
	indent->script = this->scriptstack;
	indent->skip = ( skip != 0 );
	indent->skipElse = ( skipElse != 0 );
	this->skip += indent->skip;

	indent->next = this->indentstack;
	this->indentstack = indent;
}

/*
================
idParser::PopIndent
================
*/
void idParser::PopIndent( int &type, int &skip, int &skipElse )
{
	type = 0;
	skip = 0;

	indent_t *indent = this->indentstack;
	if( !indent )
	{
		return;
	}

	// must be an indent from the current script
	if( this->indentstack->script != this->scriptstack )
	{
		return;
	}

	type = indent->type;
	skip = indent->skip;
	skipElse = indent->skipElse;
	this->indentstack = this->indentstack->next;
	this->skip -= indent->skip;
	idMem::Free( indent );
}

/*
================
idParser::PushScript
================
*/
bool idParser::PushScript( idLexer *script )
{
	for( idLexer *s = scriptstack; s != NULL; s = s->next )
	{
		if( !idStr::Icmp( s->GetFileName(), script->GetFileName() ) )
		{
			Warning( "'%s' recursively included", script->GetFileName() );
			return false;
		}
	}
	//push the script on the script stack
	script->next = scriptstack;
	scriptstack = script;

	// add dependency
	if( OSPath )
	{
		const char* relativePath = idLib::fileSystem->OSPathToRelativePath( script->GetFileName() );
		if( relativePath == NULL || relativePath[ 0 ] == '\0' )
		{
			dependencies.AddUnique( script->GetFileName() );
		}
		else {
			dependencies.AddUnique( relativePath );
		}
	}
	else {
		dependencies.AddUnique( script->GetFileName() );
	}

	return true;
}

/*
============
idParser::PushDependencies
============
*/
void idParser::PushDependencies()
{
	dependencyStateStack.Push( dependencies.Num() );
}

/*
============
idParser::PopDependencies
============
*/
void idParser::PopDependencies()
{
	if( dependencyStateStack.Empty() )
	{
		idLib::common->Error( "idParser::PopDependencies: stack underflow" );
	}
	int num = GetCurrentDependency();
	dependencies.SetNum( num );
	dependencyStateStack.Pop();
}

/*
============
idParser::GetNextDependency
============
*/
const char* idParser::GetNextDependency( int& index ) const
{
	if( index < 0 || index >= dependencies.Num() )
	{
		return NULL;
	}
	return dependencies[ index++ ];
}

/*
============
idParser::GetCurrentDependency
============
*/
int idParser::GetCurrentDependency() const
{
	return dependencyStateStack.Top();
}

/*
================
idParser::ReadSourceToken
================
*/
bool idParser::ReadSourceToken( idToken* token )
{
	int type, skip, skipElse, changedScript;

	if( !scriptstack )
	{
		idLib::common->FatalError( "idParser::ReadSourceToken: not loaded" );
		return false;
	}
	changedScript = 0;
	// if there's no token already available
	while( !tokens )
	{
		// if there's a token to read from the script
		if( scriptstack->ReadToken( token ) )
		{
			token->linesCrossed += changedScript;

			// set the marker based on the start of the token read in
			if( !marker_p )
			{
				marker_p = token->whiteSpaceEnd_p;
			}
			return true;
		}
		// if at the end of the script
		if( scriptstack->EndOfFile() )
		{
			// remove all indents of the script
			while( this->indentstack != NULL && this->indentstack->script == this->scriptstack )
			{
				Warning( "missing #endif" );
				PopIndent( type, skip, skipElse );
			}
			changedScript = 1;
		}
		// if this was the initial script
		if( !scriptstack->next )
		{
			return false;
		}
		// remove the script and return to the previous one
		idLexer* script = scriptstack;
		scriptstack = scriptstack->next;
		delete script;
	}
	
	// copy the already available token
	*token = *tokens; //SEA *?
	
	// remove the token from the source
	idToken* t = tokens;
	assert( tokens != NULL );
	tokens = tokens->next;
	delete t;

	return true;
}

/*
================
idParser::UnreadSourceToken
================
*/
void idParser::UnreadSourceToken( const idToken& token )
{
	idToken* t = new( TAG_IDLIB_PARSER ) idToken( token );
	t->next = tokens;
	tokens = t;
}

/*
================
idParser::ReadDefineParms
================
*/
bool idParser::ReadDefineParms( define_t *define, idToken **parms, int maxparms )
{
	define_t *newdefine;
	idToken token, *t, *last;
	int i, done, lastcomma, numparms, indent;

	if( !ReadSourceToken( &token ) )
	{
		Error( "define '%s' missing parameters", define->name );
		return false;
	}
	if( define->numparms > maxparms )
	{
		Error( "define with more than %d parameters", maxparms );
		return false;
	}

	for( i = 0; i < define->numparms; i++ )
	{
		parms[ i ] = NULL;
	}
	// if no leading "("
	if( token != "(" )
	{
		UnreadSourceToken( token );
		Error( "define '%s' missing parameters", define->name );
		return false;
	}

	// read the define parameters
	for( done = 0, numparms = 0, indent = 1; !done; )
	{
		if( numparms >= maxparms )
		{
			Error( "define '%s' with too many parameters", define->name );
			return false;
		}

		parms[ numparms ] = NULL;
		lastcomma = 1;
		last = NULL;
		while( !done )
		{
			if( !ReadSourceToken( &token ) )
			{
				Error( "define '%s' incomplete", define->name );
				return false;
			}

			if( token == "," )
			{
				if( indent <= 1 )
				{
					if( lastcomma )
					{
						Error( "define '%s' has too many commas", define->name );
					}
					if( numparms >= define->numparms )
					{
						Error( "define '%s' has too many define parameters", define->name );
					}
					lastcomma = 1;
					break;
				}
			}
			else if( token == "(" )
			{
				indent++;
			}
			else if( token == ")" )
			{
				indent--;
				if( indent <= 0 )
				{
					if( !parms[ define->numparms - 1 ] )
					{
						Error( "define '%s' has too few define parameters", define->name );
					}
					done = 1;
					break;
				}
			}
			else if( token.type == TT_NAME )
			{
				newdefine = FindHashedDefine( definehash, token.c_str() );
				if( newdefine )
				{
					if( !ExpandDefineIntoSource( &token, newdefine ) )
					{
						return false;
					}

					continue;
				}
			}

			lastcomma = 0;

			if( numparms < define->numparms )
			{
				t = new( TAG_IDLIB_PARSER ) idToken( token );
				t->next = NULL;
				if( last != NULL )
				{
					last->next = t;
				}
				else
				{
					parms[ numparms ] = t;
				}
				last = t;
			}
		}
		numparms++;
	}

	return true;
}

/*
================
idParser::StringizeTokens
================
*/
void idParser::StringizeTokens( idToken* tokens, idToken* token )
{
	token->type = TT_STRING;
	token->whiteSpaceStart_p = NULL;
	token->whiteSpaceEnd_p = NULL;
	( *token ) = "";
	for( idToken* t = tokens; t != NULL; t = t->next )
	{
		token->Append( t->c_str() );
	}
}

/*
================
idParser::MergeTokens
================
*/
bool idParser::MergeTokens( idToken* t1, idToken* t2 )
{
	// merging of a name with a name or number
	if( t1->type == TT_NAME && ( t2->type == TT_NAME || ( t2->type == TT_NUMBER && !( t2->subtype & TT_FLOAT ) ) ) )
	{
		t1->Append( t2->c_str() );
		return true;
	}
	// merging of two strings
	if( t1->type == TT_STRING && t2->type == TT_STRING )
	{
		t1->Append( t2->c_str() );
		return true;
	}
	// merging of two numbers
	if( t1->type == TT_NUMBER && t2->type == TT_NUMBER &&
		!( t1->subtype & ( TT_HEX | TT_BINARY ) ) && !( t2->subtype & ( TT_HEX | TT_BINARY ) ) &&
		( !( t1->subtype & TT_FLOAT ) || !( t2->subtype & TT_FLOAT ) ) )
	{
		t1->Append( t2->c_str() );
		return true;
	}

	return false;
}

/*
================
idParser::AddBuiltinDefines
================
*/
void idParser::AddBuiltinDefines()
{
	struct builtin_t {
		const char* string;
		int id;
	} builtin[] = {
		{ "__LINE__",	BUILTIN_LINE },
		{ "__FILE__",	BUILTIN_FILE },
		{ "__DATE__",	BUILTIN_DATE },
		{ "__TIME__",	BUILTIN_TIME },
		{ "__STDC__", BUILTIN_STDC },
		{ NULL, 0 }
	};

	for( int i = 0; builtin[ i ].string; ++i )
	{
		auto define = idMem::Alloc<define_t, TAG_IDLIB_PARSER>( sizeof( define_t ) + idStr::Length( builtin[ i ].string ) + 1 );
		define->name = ( char* )define + sizeof( define_t );
		strcpy( define->name, builtin[ i ].string );
		define->flags = DEFINE_FIXED;
		define->builtin = builtin[ i ].id;
		define->numparms = 0;
		define->parms = NULL;
		define->tokens = NULL;
		// add the define to the source
		AddDefineToHash( define, definehash );
	}
}

/*
================
idParser::CopyFirstDefine
================
*/
define_t * idParser::CopyFirstDefine()
{
	for( int i = 0; i < DEFINEHASHSIZE; ++i )
	{
		if( definehash[ i ] != NULL )
		{
			return CopyDefine( definehash[ i ] );
		}
	}
	return NULL;
}

static idStr PreProcessorDate()
{
	time_t t = time( NULL );
	char* curtime = ctime( &t );
	if( idStr::Length( curtime ) < 24 )
	{
		return idStr( "*** BAD CURTIME ***" );
	}
	idStr str = "\"";
	// skip DAY, extract MMM DD
	for( int i = 4; i < 10; i++ )
	{
		str.Append( curtime[ i ] );
	}
	// skip time, extract space+YYYY
	for( int i = 19; i < 24; i++ )
	{
		str.Append( curtime[ i ] );
	}
	str.Append( "\"" );
	return str;
}

static idStr PreProcessorTime()
{
	time_t t = time( NULL );
	char* curtime = ctime( &t );
	if( idStr::Length( curtime ) < 24 )
	{
		return idStr( "*** BAD CURTIME ***" );
	}

	idStr str = "\"";
	for( int i = 11; i < 19; i++ )
	{
		str.Append( curtime[ i ] );
	}
	str.Append( "\"" );
	return str;
}

CONSOLE_COMMAND( TestPreprocessorMacros, "check analyze warning", 0 )
{
	idLib::Printf( "%s : %s\n", __DATE__, PreProcessorDate().c_str() );
	idLib::Printf( "%s : %s\n", __TIME__, PreProcessorTime().c_str() );
}

/*
================
idParser::ExpandBuiltinDefine
================
*/
void idParser::ExpandBuiltinDefine( idToken* deftoken, define_t* define, idToken** firsttoken, idToken** lasttoken )
{
	auto token = new( TAG_IDLIB_PARSER ) idToken( deftoken );
	switch( define->builtin )
	{
		case BUILTIN_LINE:
		{
			char buf[ MAX_STRING_CHARS ];
			sprintf( buf, "%d", deftoken->line );
			( *token ) = buf;
			token->intvalue = deftoken->line;
			token->floatvalue = deftoken->line;
			token->type = TT_NUMBER;
			token->subtype = TT_DECIMAL | TT_INTEGER | TT_VALUESVALID;
			token->line = deftoken->line;
			token->linesCrossed = deftoken->linesCrossed;
			token->flags = 0;
			*firsttoken = token;
			*lasttoken = token;
			break;
		}
		case BUILTIN_FILE:
		{
			( *token ) = scriptstack->GetFileName();
			token->type = TT_NAME;
			token->subtype = token->Length();
			token->line = deftoken->line;
			token->linesCrossed = deftoken->linesCrossed;
			token->flags = 0;
			*firsttoken = token;
			*lasttoken = token;
			break;
		}
		case BUILTIN_DATE:
		{
			*token = PreProcessorDate();
			token->type = TT_STRING;
			token->subtype = token->Length();
			token->line = deftoken->line;
			token->linesCrossed = deftoken->linesCrossed;
			token->flags = 0;
			*firsttoken = token;
			*lasttoken = token;
			break;
		}
		case BUILTIN_TIME:
		{
			*token = PreProcessorTime();
			token->type = TT_STRING;
			token->subtype = token->Length();
			token->line = deftoken->line;
			token->linesCrossed = deftoken->linesCrossed;
			token->flags = 0;
			*firsttoken = token;
			*lasttoken = token;
			break;
		}
		case BUILTIN_STDC:
		{
			Warning( "__STDC__ not supported" );
			*firsttoken = NULL;
			*lasttoken = NULL;
			break;
		}
		default:
		{
			*firsttoken = NULL;
			*lasttoken = NULL;
			break;
		}
	}
}

/*
================
idParser::ExpandDefine
================
*/
bool idParser::ExpandDefine( idToken* deftoken, define_t* define, idToken** firsttoken, idToken** lasttoken )
{
	idToken* parms[ MAX_DEFINEPARMS ], *dt, *pt, *t;
	idToken* t1, *t2, *first, *last, *nextpt, token;
	int parmnum, i;

	// if it is a builtin define
	if( define->builtin )
	{
		ExpandBuiltinDefine( deftoken, define, firsttoken, lasttoken );
		return true;
	}

	// if the define has parameters
	if( define->numparms )
	{
		if( !ReadDefineParms( define, parms, MAX_DEFINEPARMS ) )
		{
			return false;
		}
	#ifdef DEBUG_EVAL
		for( i = 0; i < define->numparms; i++ )
		{
			Log_Write( "define parms %d:", i );
			for( pt = parms[ i ]; pt; pt = pt->next )
			{
				Log_Write( "%s", pt->c_str() );
			}
		}
	#endif //DEBUG_EVAL
	}
	// empty list at first
	first = NULL;
	last = NULL;
	// create a list with tokens of the expanded define
	for( dt = define->tokens; dt != NULL; dt = dt->next )
	{
		parmnum = -1;
		// if the token is a name, it could be a define parameter
		if( dt->type == TT_NAME )
		{
			parmnum = FindDefineParm( define, dt->c_str() );
		}
		// if it is a define parameter
		if( parmnum >= 0 )
		{
			for( pt = parms[ parmnum ]; pt; pt = pt->next )
			{
				t = new( TAG_IDLIB_PARSER ) idToken( *pt );
				//add the token to the list
				t->next = NULL;
				if( last )
				{
					last->next = t;
				}
				else
				{
					first = t;
				}
				last = t;
			}
		}
		else {
			// if stringizing operator
			if( ( *dt ) == "#" )
			{
				// the stringizing operator must be followed by a define parameter
				if( dt->next != NULL )
				{
					parmnum = FindDefineParm( define, dt->next->c_str() );
				}
				else {
					parmnum = -1;
				}

				if( parmnum >= 0 )
				{
					// step over the stringizing operator
					dt = dt->next;

					// stringize the define parameter tokens
					StringizeTokens( parms[ parmnum ], &token );

					t = new( TAG_IDLIB_PARSER ) idToken( token );
					t->line = deftoken->line;
				}
				else {
					Warning( "stringizing operator without define parameter" );
					continue;
				}
			}
			else
			{
				t = new( TAG_IDLIB_PARSER ) idToken( *dt );
				t->line = deftoken->line;
			}
			// add the token to the list
			t->next = NULL;
			// the token being read from the define list should use the line number of
			// the original file, not the header file
			t->line = deftoken->line;

			if( last )
			{
				last->next = t;
			}
			else {
				first = t;
			}
			last = t;
		}
	}
	// check for the merging operator
	for( t = first; t != NULL; )
	{
		if( t->next != NULL )
		{
			// if the merging operator
			if( ( *t->next ) == "##" )
			{
				t1 = t;
				t2 = t->next->next;
				if( t2 != NULL )
				{
					if( !MergeTokens( t1, t2 ) )
					{
						Error( "can't merge '%s' with '%s'", t1->c_str(), t2->c_str() );
						return false;
					}
					delete t1->next;
					t1->next = t2->next;
					if( t2 == last )
					{
						last = t1;
					}
					delete t2;
					continue;
				}
			}
		}
		t = t->next;
	}
	// store the first and last token of the list
	*firsttoken = first;
	*lasttoken = last;
	// free all the parameter tokens
	for( i = 0; i < define->numparms; i++ )
	{
		for( pt = parms[ i ]; pt != NULL; pt = nextpt )
		{
			nextpt = pt->next;
			delete pt;
		}
	}

	return true;
}

/*
================
idParser::ExpandDefineIntoSource
================
*/
bool idParser::ExpandDefineIntoSource( idToken* deftoken, define_t* define )
{
	idToken* firsttoken, *lasttoken;

	if( !ExpandDefine( deftoken, define, &firsttoken, &lasttoken ) )
	{
		return false;
	}
	// if the define is not empty
	if( firsttoken != NULL && lasttoken != NULL )
	{
		firsttoken->linesCrossed += deftoken->linesCrossed;
		lasttoken->next = this->tokens;
		this->tokens = firsttoken;
	}
	return true;
}

/*
================
idParser::ReadLine

reads a token from the current line, continues reading on the next
line only if a backslash '\' is found
================
*/
bool idParser::ReadLine( idToken *token, bool multiline )
{
	int crossline;

	if( multiline )
	{
		while( ReadSourceToken( token ) )
		{
			if( *token == "%>" )
			{
				return false;
			}
			return true;
		}
		return false;
	}

	crossline = 0;
	do
	{
		if( !ReadSourceToken( token ) )
		{
			return false;
		}

		if( token->linesCrossed > crossline )
		{
			UnreadSourceToken( *token );
			return false;
		}
		crossline = 1;
	} while( ( *token ) == "\\" );
	
	return true;
}

/*
================
idParser::AddDefine
================
*/
bool idParser::AddDefine( const char *string )
{
	if( definehash == NULL ) {
		definehash = idMem::ClearedAlloc<define_t*, TAG_IDLIB_PARSER>( DEFINEHASHSIZE * sizeof( define_t* ) );
	}
	auto define = DefineFromString( string );
	if( define == NULL ) {
		return false;
	}
	define->scope = defineScope;
	AddDefineToHash( define, definehash );
	return true;
}

/*
================
idParser::RemoveDefine
================
*/
bool idParser::RemoveDefine( const char* string )
{
	define_t *define, *lastdefine;
	int hash = PC_NameHash( string );
	for( lastdefine = NULL, define = definehash[ hash ]; define; define = define->hashnext )
	{
		if( !idStr::Cmp( define->name, string ) )
		{
			if( define->flags & DEFINE_FIXED )
			{
				Warning( "RemoveDefine( %s ) can't undef", string );
				return false;
			}
			else {
				if( lastdefine )
				{
					lastdefine->hashnext = define->hashnext;
				}
				else {
					definehash[ hash ] = define->hashnext;
				}
				FreeDefine( define );
				return true;
			}
			break;
		}
		lastdefine = define;
	}
	return false;	
}

/*
================
idParser::AddGlobalDefinesToSource
================
*/
void idParser::AddGlobalDefinesToSource()
{
	for( auto def = globalDefines.GetFirst(); def; def = globalDefines.GetNext( def ) )
	{
		auto newDef = CopyDefine( def );
		AddDefineToHash( newDef, definehash );
	}
}

/*
================
idParser::AddInclude
================
*/
bool idParser::AddInclude( const char *string )
{
	idStrStatic<128> str;
	str.Format( "#include <%s>", string );
	idLexer src( str.c_str(), str.Length(), "AddInclude", LEXFL_ALLOWPATHNAMES, GetLineNum() );
	idToken token1, token2, token3, token4, token5;
	src.ReadToken( &token1 );
	src.ReadToken( &token2 );
	src.ReadToken( &token3 );
	src.ReadToken( &token4 );
	src.ReadToken( &token5 );
	UnreadToken( token5 );
	UnreadToken( token4 );
	UnreadToken( token3 );
	UnreadToken( token2 );
	UnreadToken( token1 );

	//return Directive_include();
	return true;
}

/*
============
idParser::AddIncludes
============
*/
int	idParser::AddIncludes( const idStrList& includes )
{
	int result = 0;
	for( int i = includes.Num() - 1; i >= 0; i-- )
	{
		result |= ( int )AddInclude( includes[ i ] );
	}
	return result;
}

/*
================
idParser::UnreadSignToken
================
*/
void idParser::UnreadSignToken()
{
	idToken token;

	token.line = scriptstack->GetLineNum();
	token.whiteSpaceStart_p = NULL;
	token.whiteSpaceEnd_p = NULL;
	token.linesCrossed = 0;
	token.flags = 0;
	token = "-";
	token.type = TT_PUNCTUATION;
	token.subtype = P_SUB;

	UnreadSourceToken( token );
}

/*
================
idParser::EvaluateTokens
================
*/
struct operator_t {
	int op;
	int priority;
	int parentheses;
	operator_t *	prev;
	operator_t *	next;
};

struct value_t {
	enum {
		TYPE_STRING,
		TYPE_INT,
		TYPE_FLOAT
	}				type;
	idStr			string;
	int				intvalue;
	double floatvalue;
	int parentheses;
	value_t *		prev;
	value_t *		next;
};

int PC_OperatorPriority( int op )
{
	switch( op )
	{
		case P_MUL: return 15;
		case P_DIV: return 15;
		case P_MOD: return 15;
		case P_ADD: return 14;
		case P_SUB: return 14;

		case P_LOGIC_AND: return 7;
		case P_LOGIC_OR: return 6;
		case P_LOGIC_GEQ: return 12;
		case P_LOGIC_LEQ: return 12;
		case P_LOGIC_EQ: return 11;
		case P_LOGIC_UNEQ: return 11;

		case P_LOGIC_NOT: return 16;
		case P_LOGIC_GREATER: return 12;
		case P_LOGIC_LESS: return 12;

		case P_RSHIFT: return 13;
		case P_LSHIFT: return 13;

		case P_BIN_AND: return 10;
		case P_BIN_OR: return 8;
		case P_BIN_XOR: return 9;
		case P_BIN_NOT: return 16;

		case P_COLON: return 5;
		case P_QUESTIONMARK: return 5;
	}
	return 0;
}

//#define AllocValue()			GetClearedMemory(sizeof(value_t));
//#define FreeValue(val)		FreeMemory(val)
//#define AllocOperator(op)		op = (operator_t *) GetClearedMemory(sizeof(operator_t));
//#define FreeOperator(op)		FreeMemory(op);

#define MAX_VALUES		64
#define MAX_OPERATORS	64

#define AllocValue( val )						\
	if ( numvalues >= MAX_VALUES ) {			\
		Error( "out of value space" );			\
		error = 1;								\
		break;									\
	} else {									\
		val = &value_heap[numvalues++];			\
	}

#define FreeValue(val)

#define AllocOperator( op )						\
	if ( numoperators >= MAX_OPERATORS ) {		\
		Error( "out of operator space" );		\
		error = 1;								\
		break;									\
	} else {									\
		op = &operator_heap[numoperators++];	\
	}

#define FreeOperator( op )

bool idParser::EvaluateTokens( idToken* tokens, signed int* intvalue, double* floatvalue, int integer )
{
	operator_t* o, *firstoperator, *lastoperator;
	value_t* v, *firstvalue, *lastvalue, *v1, *v2;
	idToken* t;
	int brace = 0;
	int parentheses = 0;
	int error = 0;
	int lastwasvalue = 0;
	int negativevalue = 0;
	int questmarkintvalue = 0;
	double questmarkfloatvalue = 0;
	int gotquestmarkvalue = false;
	int lastoperatortype = 0;
	//
	operator_t operator_heap[ MAX_OPERATORS ];
	int numoperators = 0;
	value_t value_heap[ MAX_VALUES ];
	int numvalues = 0;

	firstoperator = lastoperator = NULL;
	firstvalue = lastvalue = NULL;
	if( intvalue != NULL )
	{
		*intvalue = 0;
	}
	if( floatvalue != NULL )
	{
		*floatvalue = 0;
	}
	for( t = tokens; t != NULL; t = t->next )
	{
		switch( t->type )
		{
			case TT_NAME:
			{
				if( lastwasvalue || negativevalue )
				{
					Error( "syntax error in #if/#elif" );
					error = 1;
					break;
				}
				if( ( *t ) != "defined" )
				{
					Error( "undefined name '%s' in #if/#elif", t->c_str() );
					error = 1;
					break;
				}
				t = t->next;
				if( ( *t ) == "(" )
				{
					brace = true;
					t = t->next;
				}
				if( !t || t->type != TT_NAME )
				{
					Error( "defined() without name in #if/#elif" );
					error = 1;
					break;
				}
				AllocValue( v );
				if( FindHashedDefine( definehash, t->c_str() ) )
				{
					v->intvalue = 1;
					v->floatvalue = 1;
				}
				else
				{
					v->intvalue = 0;
					v->floatvalue = 0;
				}
				v->type = value_t::TYPE_INT;
				v->parentheses = parentheses;
				v->next = NULL;
				v->prev = lastvalue;
				if( lastvalue != NULL )
				{
					lastvalue->next = v;
				}
				else
				{
					firstvalue = v;
				}
				lastvalue = v;
				if( brace )
				{
					t = t->next;
					if( !t || ( *t ) != ")" )
					{
						Error( "defined missing ) in #if/#elif" );
						error = 1;
						break;
					}
				}
				brace = false;
				// defined() creates a value
				lastwasvalue = 1;
				break;
			}
			case TT_STRING:
			{
				if( lastwasvalue )
				{
					Error( "syntax error in #if/#elif" );
					error = 1;
					break;
				}
				if( lastoperator != NULL )
				{
					if( lastoperator->op != P_LOGIC_EQ && lastoperator->op != P_LOGIC_UNEQ )
					{
						Error( "only == and != are allowed on strings" );
						error = 1;
						break;
					}
				}
				AllocValue( v );
				v->string = *t;
				v->type = value_t::TYPE_STRING;
				v->parentheses = parentheses;
				v->next = NULL;
				v->prev = lastvalue;
				if( lastvalue != NULL )
				{
					lastvalue->next = v;
				}
				else
				{
					firstvalue = v;
				}
				lastvalue = v;
				// last token was a value
				lastwasvalue = 1;
				negativevalue = 0;
				break;
			}
			case TT_NUMBER:
			{
				if( lastwasvalue )
				{
					Error( "syntax error in #if/#elif" );
					error = 1;
					break;
				}
				AllocValue( v );
				if( negativevalue )
				{
					v->intvalue = -t->GetIntValue();
					v->floatvalue = -t->GetFloatValue();
				}
				else
				{
					v->intvalue = t->GetIntValue();
					v->floatvalue = t->GetFloatValue();
				}
				if( ( t->subtype & TT_INTEGER ) != 0 )
				{
					v->type = value_t::TYPE_INT;
				}
				else
				{
					v->type = value_t::TYPE_FLOAT;
				}
				v->parentheses = parentheses;
				v->next = NULL;
				v->prev = lastvalue;
				if( lastvalue != NULL )
				{
					lastvalue->next = v;
				}
				else
				{
					firstvalue = v;
				}
				lastvalue = v;
				// last token was a value
				lastwasvalue = 1;
				negativevalue = 0;
				break;
			}
			case TT_PUNCTUATION:
			{
				if( negativevalue )
				{
					Error( "misplaced minus sign in #if/#elif" );
					error = 1;
					break;
				}
				if( t->subtype == P_PARENTHESESOPEN )
				{
					parentheses++;
					break;
				}
				else if( t->subtype == P_PARENTHESESCLOSE )
				{
					parentheses--;
					if( parentheses < 0 )
					{
						Error( "too many ) in #if/#elsif" );
						error = 1;
					}
					break;
				}
				//check for invalid operators on floating point values
				if( !integer )
				{
					if( t->subtype == P_BIN_NOT || t->subtype == P_MOD ||
						t->subtype == P_RSHIFT || t->subtype == P_LSHIFT ||
						t->subtype == P_BIN_AND || t->subtype == P_BIN_OR ||
						t->subtype == P_BIN_XOR )
					{
						Error( "illegal operator '%s' on floating point operands", t->c_str() );
						error = 1;
						break;
					}
				}
				if( lastwasvalue && lastvalue->type == value_t::TYPE_STRING )
				{
					if( t->subtype != P_LOGIC_EQ && t->subtype != P_LOGIC_UNEQ )
					{
						Error( "illegal operator '%s' on string operands", t->c_str() );
						error = 1;
						break;
					}
				}
				switch( t->subtype )
				{
					case P_LOGIC_NOT:
					case P_BIN_NOT:
					{
						if( lastwasvalue )
						{
							Error( "! or ~ after value in #if/#elif" );
							error = 1;
							break;
						}
						break;
					}
					case P_INC:
					case P_DEC:
					{
						Error( "++ or -- used in #if/#elif" );
						break;
					}
					case P_SUB:
					{
						if( !lastwasvalue )
						{
							negativevalue = 1;
							break;
						}
					}

					case P_MUL:
					case P_DIV:
					case P_MOD:
					case P_ADD:

					case P_LOGIC_AND:
					case P_LOGIC_OR:
					case P_LOGIC_GEQ:
					case P_LOGIC_LEQ:
					case P_LOGIC_EQ:
					case P_LOGIC_UNEQ:

					case P_LOGIC_GREATER:
					case P_LOGIC_LESS:

					case P_RSHIFT:
					case P_LSHIFT:

					case P_BIN_AND:
					case P_BIN_OR:
					case P_BIN_XOR:

					case P_COLON:
					case P_QUESTIONMARK:
					{
						if( !lastwasvalue )
						{
							Error( "operator '%s' after operator in #if/#elif", t->c_str() );
							error = 1;
							break;
						}
						break;
					}
					default:
					{
						Error( "invalid operator '%s' in #if/#elif", t->c_str() );
						error = 1;
						break;
					}
				}
				if( !error && !negativevalue )
				{
					//o = (operator_t *) GetClearedMemory(sizeof(operator_t));
					AllocOperator( o );
					o->op = t->subtype;
					o->priority = PC_OperatorPriority( t->subtype );
					o->parentheses = parentheses;
					o->next = NULL;
					o->prev = lastoperator;
					if( lastoperator != NULL )
					{
						lastoperator->next = o;
					}
					else
					{
						firstoperator = o;
					}
					lastoperator = o;
					lastwasvalue = 0;
				}
				break;
			}
			default:
			{
				Error( "unknown '%s' in #if/#elif", t->c_str() );
				error = 1;
				break;
			}
		}
		if( error )
		{
			break;
		}
	}
	if( !error )
	{
		if( !lastwasvalue )
		{
			Error( "trailing operator in #if/#elif" );
			error = 1;
		}
		else if( parentheses )
		{
			Error( "too many ( in #if/#elif" );
			error = 1;
		}
	}

	gotquestmarkvalue = false;
	questmarkintvalue = 0;
	questmarkfloatvalue = 0;
	//while there are operators
	while( !error && firstoperator )
	{
		v = firstvalue;
		for( o = firstoperator; o->next; o = o->next )
		{
			// if the current operator is nested deeper in parentheses than the next operator
			if( o->parentheses > o->next->parentheses )
			{
				break;
			}
			//if the current and next operator are nested equally deep in parentheses
			if( o->parentheses == o->next->parentheses )
			{
				// if the priority of the current operator is equal or higher than the priority of the next operator
				if( o->priority >= o->next->priority )
				{
					break;
				}
			}
			//if the arity of the operator isn't equal to 1
			if( o->op != P_LOGIC_NOT && o->op != P_BIN_NOT )
			{
				v = v->next;
			}
			//if there's no value or no next value
			if( !v )
			{
				Error( "mising values in #if/#elif" );
				error = 1;
				break;
			}
		}
		if( error )
		{
			break;
		}
		v1 = v;
		v2 = v->next;
	#ifdef DEBUG_EVAL
		if( integer )
		{
			Log_Write( "operator %s, value1 = %d", scriptstack->getPunctuationFromId( o->op ), v1->intvalue );
			if( v2 ) Log_Write( "value2 = %d", v2->intvalue );
		}
		else
		{
			Log_Write( "operator %s, value1 = %f", scriptstack->getPunctuationFromId( o->op ), v1->floatvalue );
			if( v2 ) Log_Write( "value2 = %f", v2->floatvalue );
		}
	#endif //DEBUG_EVAL
		switch( o->op )
		{
			case P_LOGIC_NOT:
				v1->intvalue = !v1->intvalue;
				v1->floatvalue = !v1->floatvalue;
				break;
			case P_BIN_NOT:
				v1->intvalue = ~v1->intvalue;
				break;
			case P_MUL:
				v1->intvalue *= v2->intvalue;
				v1->floatvalue *= v2->floatvalue;
				break;
			case P_DIV:
				if( !v2->intvalue || !v2->floatvalue )
				{
					Error( "divide by zero in #if/#elif" );
					error = 1;
					break;
				}
				v1->intvalue /= v2->intvalue;
				v1->floatvalue /= v2->floatvalue;
				break;
			case P_MOD:
				if( !v2->intvalue )
				{
					Error( "divide by zero in #if/#elif" );
					error = 1;
					break;
				}
				v1->intvalue %= v2->intvalue;
				break;
			case P_ADD:
				v1->intvalue += v2->intvalue;
				v1->floatvalue += v2->floatvalue;
				break;
			case P_SUB:
				v1->intvalue -= v2->intvalue;
				v1->floatvalue -= v2->floatvalue;
				break;
			case P_LOGIC_AND:
				v1->intvalue = v1->intvalue && v2->intvalue;
				v1->floatvalue = v1->floatvalue && v2->floatvalue;
				break;
			case P_LOGIC_OR:
				v1->intvalue = v1->intvalue || v2->intvalue;
				v1->floatvalue = v1->floatvalue || v2->floatvalue;
				break;
			case P_LOGIC_GEQ:
				v1->intvalue = v1->intvalue >= v2->intvalue;
				v1->floatvalue = v1->floatvalue >= v2->floatvalue;
				break;
			case P_LOGIC_LEQ:
				v1->intvalue = v1->intvalue <= v2->intvalue;
				v1->floatvalue = v1->floatvalue <= v2->floatvalue;
				break;
			case P_LOGIC_EQ:		
				v1->intvalue = ( v1->type == value_t::TYPE_STRING ) ? ( v1->string.Icmp( v2->string ) == 0 ) : ( v1->intvalue == v2->intvalue );
				v1->floatvalue = ( v1->type == value_t::TYPE_STRING ) ? ( v1->string.Icmp( v2->string ) == 0 ) : ( v1->floatvalue == v2->floatvalue ); 
				break;
			case P_LOGIC_UNEQ:		
				v1->intvalue = ( v1->type == value_t::TYPE_STRING ) ? ( v1->string.Icmp( v2->string ) != 0 ) : ( v1->intvalue != v2->intvalue );
				v1->floatvalue = ( v1->type == value_t::TYPE_STRING ) ? ( v1->string.Icmp( v2->string ) != 0 ) : ( v1->floatvalue != v2->floatvalue );
				break;
			case P_LOGIC_GREATER:
				v1->intvalue = v1->intvalue > v2->intvalue;
				v1->floatvalue = v1->floatvalue > v2->floatvalue;
				break;
			case P_LOGIC_LESS:
				v1->intvalue = v1->intvalue < v2->intvalue;
				v1->floatvalue = v1->floatvalue < v2->floatvalue;
				break;
			case P_RSHIFT:
				v1->intvalue >>= v2->intvalue;
				break;
			case P_LSHIFT:
				v1->intvalue <<= v2->intvalue;
				break;
			case P_BIN_AND:
				v1->intvalue &= v2->intvalue;
				break;
			case P_BIN_OR:
				v1->intvalue |= v2->intvalue;
				break;
			case P_BIN_XOR:
				v1->intvalue ^= v2->intvalue;
				break;
			case P_COLON:
			{
				if( !gotquestmarkvalue )
				{
					Error( ": without ? in #if/#elif" );
					error = 1;
					break;
				}
				if( integer )
				{
					if( !questmarkintvalue )
					{
						v1->intvalue = v2->intvalue;
					}
				}
				else
				{
					if( !questmarkfloatvalue )
					{
						v1->floatvalue = v2->floatvalue;
					}
				}
				gotquestmarkvalue = false;
				break;
			}
			case P_QUESTIONMARK:
			{
				if( gotquestmarkvalue )
				{
					Error( "? after ? in #if/#elif" );
					error = 1;
					break;
				}
				questmarkintvalue = v1->intvalue;
				questmarkfloatvalue = v1->floatvalue;
				gotquestmarkvalue = true;
				break;
			}
		}
	#ifdef DEBUG_EVAL
		if( integer ) Log_Write( "result value = %d", v1->intvalue );
		else Log_Write( "result value = %f", v1->floatvalue );
	#endif //DEBUG_EVAL
		if( error ) {
			break;
		}
		lastoperatortype = o->op;
		//if not an operator with arity 1
		if( o->op != P_LOGIC_NOT && o->op != P_BIN_NOT )
		{
			//remove the second value if not question mark operator
			if( o->op != P_QUESTIONMARK )
			{
				v = v->next;
			}
			if( v->prev != NULL )
			{
				v->prev->next = v->next;
			}
			else
			{
				firstvalue = v->next;
			}
			if( v->next != NULL )
			{
				v->next->prev = v->prev;
			}
			else
			{
				lastvalue = v->prev;
			}
			FreeValue( v );
		}
		//remove the operator
		if( o->prev != NULL )
		{
			o->prev->next = o->next;
		}
		else
		{
			firstoperator = o->next;
		}
		if( o->next != NULL )
		{
			o->next->prev = o->prev;
		}
		else
		{
			lastoperator = o->prev;
		}
		FreeOperator( o );
	}
	if( firstvalue != NULL )
	{
		if( intvalue != NULL ) *intvalue = firstvalue->intvalue;
		if( floatvalue != NULL ) *floatvalue = firstvalue->floatvalue;
	}
	for( o = firstoperator; o != NULL; o = lastoperator )
	{
		lastoperator = o->next;
		FreeOperator( o );
	}
	for( v = firstvalue; v != NULL; v = lastvalue )
	{
		lastvalue = v->next;
		FreeValue( v );
	}
	if( !error )
	{
		return true;
	}
	if( intvalue != NULL )
	{
		*intvalue = 0;
	}
	if( floatvalue != NULL )
	{
		*floatvalue = 0;
	}
	return false;
}

/*
================
idParser::Evaluate

  sing-line evalulate
================
*/
bool idParser::Evaluate( signed int* intvalue, double* floatvalue, int integer )
{
	idToken token, *firsttoken, *lasttoken;
	idToken* t, *nexttoken;
	define_t* define;
	int defined = false;

	if( intvalue != NULL )
	{
		*intvalue = 0;
	}
	if( floatvalue != NULL )
	{
		*floatvalue = 0;
	}

	if( !ReadLine( &token, false ) )
	{
		Error( "no value after #if/#elif" );
		return false;
	}

	firsttoken = NULL;
	lasttoken = NULL;
	do {
		//if the token is a name
		if( token.type == TT_NAME )
		{
			if( defined )
			{
				defined = false;
				t = new( TAG_IDLIB_PARSER ) idToken( token );
				t->next = NULL;
				if( lasttoken != NULL )
				{
					lasttoken->next = t;
				}
				else
				{
					firsttoken = t;
				}
				lasttoken = t;
			}
			else if( token == "defined" )
			{
				defined = true;
				t = new( TAG_IDLIB_PARSER ) idToken( token );
				t->next = NULL;
				if( lasttoken != NULL )
				{
					lasttoken->next = t;
				}
				else
				{
					firsttoken = t;
				}
				lasttoken = t;
			}
			else
			{
				//then it must be a define
				define = FindHashedDefine( idParser::definehash, token.c_str() );
				if( !define )
				{
					Error( "can't Evaluate '%s', not defined", token.c_str() );
					return false;
				}
				if( !ExpandDefineIntoSource( &token, define ) )
				{
					return false;
				}
			}
		}
		//if the token is a number or a punctuation
		else if( token.type == TT_NUMBER || token.type == TT_PUNCTUATION )
		{
			t = new( TAG_IDLIB_PARSER ) idToken( token );
			t->next = NULL;
			if( lasttoken != NULL )
			{
				lasttoken->next = t;
			}
			else
			{
				firsttoken = t;
			}
			lasttoken = t;
		}
		else
		{
			Error( "can't Evaluate '%s'", token.c_str() );
			return false;
		}
	} while( ReadLine( &token, false ) );
	//
	if( !EvaluateTokens( firsttoken, intvalue, floatvalue, integer ) )
	{
		return false;
	}
	//
#ifdef DEBUG_EVAL
	Log_Write( "eval:" );
#endif //DEBUG_EVAL
	for( t = firsttoken; t; t = nexttoken )
	{
	#ifdef DEBUG_EVAL
		Log_Write( " %s", t->c_str() );
	#endif //DEBUG_EVAL
		nexttoken = t->next;
		delete t;
	} //end for
#ifdef DEBUG_EVAL
	if( integer ) Log_Write( "eval result: %d", *intvalue );
	else Log_Write( "eval result: %f", *floatvalue );
#endif //DEBUG_EVAL

	return true;
}

/*
================
idParser::DollarEvaluate

  multi-line evaluate
================
*/
bool idParser::DollarEvaluate( signed int *intvalue, double *floatvalue, int integer )
{
	int indent, defined = false;
	idToken token, *firsttoken, *lasttoken;
	idToken *t, *nexttoken;
	define_t *define;

	if( intvalue != NULL )
	{
		*intvalue = 0;
	}
	if( floatvalue != NULL )
	{
		*floatvalue = 0;
	}

	if( !ReadSourceToken( &token ) || token != "(" )
	{
		Error( "no leading ( after $evalint/$evalfloat" );
		return false;
	}
	if( !ReadSourceToken( &token ) )
	{
		Error( "nothing to Evaluate" );
		return false;
	}
	indent = 1;
	firsttoken = NULL;
	lasttoken = NULL;
	do {
		//if the token is a name
		if( token.type == TT_NAME )
		{
			if( defined )
			{
				defined = false;
				t = new( TAG_IDLIB_PARSER ) idToken( token );
				t->next = NULL;
				if( lasttoken != NULL )
				{
					lasttoken->next = t;
				}
				else {
					firsttoken = t;
				}
				lasttoken = t;
			}
			else if( token == "defined" )
			{
				defined = true;
				t = new( TAG_IDLIB_PARSER ) idToken( token );
				t->next = NULL;
				if( lasttoken != NULL )
				{
					lasttoken->next = t;
				}
				else
				{
					firsttoken = t;
				}
				lasttoken = t;
			}
			else {
				//then it must be a define
				define = FindHashedDefine( definehash, token.c_str() );
				if( !define )
				{
					Warning( "can't Evaluate '%s', not defined", token.c_str() );
					return false;
				}
				if( !ExpandDefineIntoSource( &token, define ) )
				{
					return false;
				}
			}
		}
		else if( token.type == TT_STRING || token.type == TT_NUMBER || token.type == TT_PUNCTUATION )
		{		// if the token is a string, number or a punctuation
			if( token[ 0 ] == '(' )
			{
				indent++;
			}
			else if( token[ 0 ] == ')' )
			{
				indent--;
			}
			if( indent <= 0 )
			{
				break;
			}
			t = new( TAG_IDLIB_PARSER ) idToken( token );
			t->next = NULL;
			if( lasttoken != NULL )
			{
				lasttoken->next = t;
			}
			else
			{
				firsttoken = t;
			}
			lasttoken = t;
		}
		else
		{
			Error( "can't Evaluate '%s'", token.c_str() );
			return false;
		}
	} while( ReadSourceToken( &token ) );

	if( !EvaluateTokens( firsttoken, intvalue, floatvalue, integer ) )
	{
		return false;
	}

#ifdef DEBUG_EVAL
	Log_Write( "$eval:" );
#endif //DEBUG_EVAL
	for( t = firsttoken; t != NULL; t = nexttoken )
	{
	#ifdef DEBUG_EVAL
		Log_Write( " %s", t->c_str() );
	#endif //DEBUG_EVAL
		nexttoken = t->next;
		delete t;
	}
#ifdef DEBUG_EVAL
	if( integer ) Log_Write( "$eval result: %d", *intvalue );
	else Log_Write( "$eval result: %f", *floatvalue );
#endif //DEBUG_EVAL

	return true;
}

/*
================
idParser::Directive_include
================
*/
// RB: added token as parameter
bool idParser::Directive_include( idToken* token, bool supressWarning )
{
	idLexer* script;
	idStr path;

	if( !ReadSourceToken( token ) )
	{
		Error( "#include without file name" );
		return false;
	}
	if( token->linesCrossed > 0 )
	{
		Error( "#include without file name" );
		return false;
	}

	if( token->type == TT_STRING )
	{
		script = new( TAG_IDLIB_PARSER ) idLexer( flags );
		script->SetPunctuations( punctuations );
		// try relative to the current file
		path = scriptstack->GetFileName();
		path.StripFilename();
		path += "/";
		path += *token;
		if( !script->LoadFile( path, OSPath, startLine ) )
		{
			// try absolute path
			path = *token;
			if( !script->LoadFile( path, OSPath, startLine ) )
			{
				// try from the include path
				path = includepath + *token;
				if( !script->LoadFile( path, OSPath, startLine ) )
				{
					delete script;
					script = NULL;
				}
			}
		}
	}
	else if( token->type == TT_PUNCTUATION && *token == "<" )
	{
		path = includepath;
		while( ReadSourceToken( token ) )
		{
			if( token->linesCrossed > 0 )
			{
				UnreadSourceToken( token );
				break;
			}
			if( token->type == TT_PUNCTUATION && *token == ">" )
			{
				break;
			}
			path += *token;
		}
		if( *token != ">" )
		{
			Warning( "#include missing trailing >" );
		}

		if( !path.Length() )
		{
			Error( "#include without file name between < >" );
			return false;
		}
		if( flags & LEXFL_NOBASEINCLUDES )
		{
			return true;
		}

		script = new( TAG_IDLIB_PARSER ) idLexer( flags );
		script->SetPunctuations( punctuations );
		if( !script->LoadFile( includepath + path, OSPath, startLine ) )
		{
			delete script;
			script = NULL;
		}
	}
	else {
		Error( "#include without file name" );
		return false;
	}

	if( !script )
	{
		if( !supressWarning )
		{
			Error( "file '%s' not found", path.c_str() );
		}
		return false;
	}

	PushScript( script );
	return true;
}
// RB end

/*
================
idParser::Directive_include
================
*/
bool idParser::Directive_include()
{
	idLexer *script;
	idToken token;
	idStr path;

	if( !ReadSourceToken( &token ) )
	{
		Error( "#include without file name" );
		return false;
	}
	if( token.linesCrossed > 0 )
	{
		Error( "#include without file name" );
		return false;
	}

	if( token.type == TT_STRING )
	{
		script = new( TAG_IDLIB_PARSER ) idLexer( flags );
		script->SetPunctuations( punctuations );
		// try relative to the current file
		path = scriptstack->GetFileName();
		path.StripFilename();
		path += "/";
		path += token;
		if( !script->LoadFile( path, OSPath, startLine ) )
		{
			// try absolute path
			path = token;
			if( !script->LoadFile( path, OSPath, startLine ) )
			{
				// try from the include path
				path = includepath + token;
				if( !script->LoadFile( path, OSPath, startLine ) )
				{
					delete script;
					script = NULL;
				}
			}
		}
	}
	else if( token.type == TT_PUNCTUATION && token == "<" )
	{
		path = includepath;
		while( ReadSourceToken( &token ) )
		{
			if( token.linesCrossed > 0 )
			{
				UnreadSourceToken( token );
				break;
			}
			if( token.type == TT_PUNCTUATION && token == ">" )
			{
				break;
			}
			path += token;
		}
		if( token != ">" )
		{
			Warning( "#include missing trailing >" );
		}

		if( !path.Length() )
		{
			Error( "#include without file name between < >" );
			return false;
		}
		if( flags & LEXFL_NOBASEINCLUDES )
		{
			return true;
		}

		script = new( TAG_IDLIB_PARSER ) idLexer( flags );
		script->SetPunctuations( punctuations );
		if( !script->LoadFile( includepath + path, OSPath, startLine ) )
		{
			delete script;
			script = NULL;
		}
	}
	else {
		Error( "#include without file name" );
		return false;
	}

	if( script == NULL )
	{
		Error( "file '%s' not found", path.c_str() );
		return false;
	}

	if( !PushScript( script ) )
	{
		delete script;
		script = NULL;
	}

	return true;
}

/*
================
idParser::Directive_define
================
*/
bool idParser::Directive_define( bool isTemplate )
{
	idToken token, *t, *last;
	define_t* define;

	if( !ReadLine( &token, isTemplate ) )
	{
		Error( "#define without name" );
		return false;
	}
	if( token.type != TT_NAME )
	{
		UnreadSourceToken( token );
		Error( "expected name after #define, found '%s'", token.c_str() );
		return false;
	}

	// check if the define already exists
	define = FindHashedDefine( definehash, token.c_str() );
	if( define )
	{
		if( define->flags & DEFINE_FIXED )
		{
			Error( "can't redefine '%s'", token.c_str() );
			return false;
		}

		Warning( "redefinition of '%s'", token.c_str() );
		// unread the define name before executing the #undef directive
		UnreadSourceToken( token );
		if( !Directive_undef() )
		{
			return false;
		}

		// if the define was not removed (define->flags & DEFINE_FIXED)
		define = FindHashedDefine( definehash, token.c_str() );
	}

	// allocate define
	define = idMem::ClearedAlloc<define_t, TAG_IDLIB_PARSER >( sizeof( define_t ) + token.Length() + 1 );
	define->name = ( char* )define + sizeof( define_t );
	strcpy( define->name, token.c_str() );
	// add the define to the source
	AddDefineToHash( define, definehash );
	// if nothing is defined, just return
	if( !ReadLine( &token, isTemplate ) )
	{
		return true;
	}

	// if it is a define with parameters
	if( token.WhiteSpaceBeforeToken() == 0 && token == "(" )
	{
		// read the define parameters
		last = NULL;
		if( !CheckTokenString( ")" ) )
		{
			while( 1 )
			{
				if( !ReadLine( &token, isTemplate ) )
				{
					Error( "expected define parameter" );
					return false;
				}
				// if it isn't a name
				if( token.type != TT_NAME )
				{
					Error( "invalid define parameter" );
					return false;
				}

				if( FindDefineParm( define, token.c_str() ) >= 0 )
				{
					Error( "two the same define parameters" );
					return false;
				}

				// add the define parm
				t = new( TAG_IDLIB_PARSER ) idToken( token );
				t->ClearTokenWhiteSpace();
				t->next = NULL;
				if( last )
				{
					last->next = t;
				}
				else
				{
					define->parms = t;
				}
				last = t;
				define->numparms++;
				// read next token
				if( !ReadLine( &token, isTemplate ) )
				{
					Error( "define parameters not terminated" );
					return false;
				}

				if( token == ")" )
				{
					break;
				}
				// then it must be a comma
				if( token != "," )
				{
					Error( "define not terminated" );
					return false;
				}
			}
		}
		if( !ReadLine( &token, isTemplate ) )
		{
			return true;
		}
	}

	// read the defined stuff
	last = NULL;
	do {
		if( isTemplate && token.type == TT_PUNCTUATION && token == "$" )
		{
			idToken end;
			ReadLine( &end, isTemplate );
			if( end == "endtemplate" )
			{
				break;
			}
			UnreadSourceToken( end );
		}
		t = new( TAG_IDLIB_PARSER ) idToken( token );
		if( t->type == TT_NAME && !idStr::Cmp( t->c_str(), define->name ) )
		{
			t->flags |= TOKEN_FL_RECURSIVE_DEFINE;
			Warning( "recursive define of '%s' (removed recursion)", define->name );
		}
		t->whiteSpaceStart_p = t->whiteSpaceEnd_p = NULL;
		t->next = NULL;
		if( last != NULL )
		{
			last->next = t;
		}
		else
		{
			define->tokens = t;
		}
		last = t;
	} while( ReadLine( &token, isTemplate ) );

	if( last != NULL )
	{
		// check for merge operators at the beginning or end
		if( ( *define->tokens ) == "##" || ( *last ) == "##" )
		{
			Error( "define with misplaced ##" );
			return false;
		}
	}

	return true;
}


/*
================
idParser::Directive_undef
================
*/
bool idParser::Directive_undef()
{
	idToken token;
	if( !ReadLine( &token, false ) )
	{
		Error( "undef without name" );
		return false;
	}
	if( token.type != TT_NAME )
	{
		UnreadSourceToken( token );
		Error( "expected name but found '%s'", token.c_str() );
		return false;
	}

	define_t *define, *lastdefine;
	int hash = PC_NameHash( token.c_str() );
	for( lastdefine = NULL, define = definehash[ hash ]; define; define = define->hashnext )
	{
		if( !idStr::Cmp( define->name, token.c_str() ) )
		{
			if( define->flags & DEFINE_FIXED )
			{
				idParser::Warning( "can't undef '%s'", token.c_str() );
			}
			else
			{
				if( lastdefine )
				{
					lastdefine->hashnext = define->hashnext;
				}
				else
				{
					definehash[ hash ] = define->hashnext;
				}
				FreeDefine( define );
			}
			break;
		}
		lastdefine = define;
	}

	return true;
}

/*
================
idParser::Directive_if_def
================
*/
bool idParser::Directive_if_def( int type )
{
	idToken token;

	if( !ReadLine( &token, false ) )
	{
		Error( "#ifdef without name" );
		return false;
	}
	if( token.type != TT_NAME )
	{
		UnreadSourceToken( token );
		Error( "expected name after #ifdef, found '%s'", token.c_str() );
		return false;
	}

	auto d = FindHashedDefine( definehash, token.c_str() );
	int skip = ( type == INDENT_IFDEF ) == ( d == NULL );
	PushIndent( type, skip, !skip );
	return true;
}

/*
================
idParser::Directive_ifdef
================
*/
bool idParser::Directive_ifdef()
{
	return Directive_if_def( INDENT_IFDEF );
}

/*
================
idParser::Directive_ifndef
================
*/
bool idParser::Directive_ifndef()
{
	return Directive_if_def( INDENT_IFNDEF );
}

/*
================
idParser::Directive_else
================
*/
bool idParser::Directive_else()
{
	int type, skip, skipElse;

	PopIndent( type, skip, skipElse );
	if( !type )
	{
		Error( "misplaced #else" );
		return false;
	}
	if( type == INDENT_ELSE )
	{
		Error( "#else after #else" );
		return false;
	}

	PushIndent( INDENT_ELSE, skipElse, 1 );
	return true;
}

/*
================
idParser::Directive_endif
================
*/
bool idParser::Directive_endif()
{
	int type, skip, skipElse;

	PopIndent( type, skip, skipElse );
	if( !type )
	{
		Error( "misplaced #endif" );
		return false;
	}

	return true;
}

/*
================
idParser::Directive_if
================
*/
bool idParser::Directive_if()
{
	signed int value = 0;
	int skip;

	if( !this->skip )
	{
		if( !Evaluate( &value, NULL, true ) )
		{
			return false;
		}
		skip = ( value == 0 );
	}
	else
	{
		skip = 1;
	}

	PushIndent( INDENT_IF, skip, !skip );
	return true;
}

/*
================
idParser::Directive_elif
================
*/
bool idParser::Directive_elif()
{
	signed int value = 0;
	int type, skip, skipElse;

	PopIndent( type, skip, skipElse );
	if( !type || type == INDENT_ELSE )
	{
		Error( "misplaced #elif" );
		return false;
	}
	if( !this->skip )
	{
		if( !Evaluate( &value, NULL, true ) )
		{
			return false;
		}
		skip = skipElse || ( value == 0 );
	}
	else {
		skip = 1;
	}

	skipElse = skipElse || !skip;
	PushIndent( INDENT_ELIF, skip, skipElse );
	return true;
}

/*
================
idParser::Directive_line
================
*/
bool idParser::Directive_line()
{
	idToken token;

	Error( "#line directive not supported" );
	while( ReadLine( &token, false ) )
	{
	}
	return true;
}

/*
================
idParser::Directive_error
================
*/
bool idParser::Directive_error()
{
	idToken token;

	if( !ReadLine( &token, false ) || token.type != TT_STRING )
	{
		Error( "#error without string" );
		return false;
	}

	Error( "#error: %s", token.c_str() );
	return true;
}

/*
================
idParser::Directive_warning
================
*/
bool idParser::Directive_warning()
{
	idToken token;

	if( !ReadLine( &token, false ) || token.type != TT_STRING )
	{
		Warning( "#warning without string" );
		return false;
	}

	Warning( "#warning: %s", token.c_str() );
	return true;
}

/*
================
idParser::Directive_pragma
================
*/
bool idParser::Directive_pragma()	//SEA no error code ?
{
	idToken token;
	idStr pragma;

	while( ReadLine( &token, false ) )
	{
		pragma += token + " ";
	}
	pragma.StripTrailing( " " );

	if( pragmaCallback != NULL )
	{
		pragmaCallback( pragmaData, pragma );
	}
	else {
		Warning( "#pragma directive '%s' not supported", pragma.c_str() );
	}

	return true;
}

/*
================
idParser::Directive_eval
================
*/
bool idParser::Directive_eval()
{
	signed int value; // DG: use int instead of long for 64bit compatibility
	idToken token;
	char buf[ 128 ];

	if( !Evaluate( &value, NULL, true ) )
	{
		return false;
	}

	token.line = scriptstack->GetLineNum();
	token.whiteSpaceStart_p = NULL;
	token.whiteSpaceEnd_p = NULL;
	token.linesCrossed = 0;
	token.flags = 0;
	sprintf( buf, "%i", idMath::Abs( value ) );
	token = buf;
	token.type = TT_NUMBER;
	token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL;
	UnreadSourceToken( token );
	if( value < 0 )
	{
		UnreadSignToken();
	}

	return true;
}

/*
================
idParser::Directive_evalfloat
================
*/
bool idParser::Directive_evalfloat()
{
	double value;
	idToken token;
	char buf[ 128 ];

	if( !Evaluate( NULL, &value, false ) )
	{
		return false;
	}

	token.line = scriptstack->GetLineNum();
	token.whiteSpaceStart_p = NULL;
	token.whiteSpaceEnd_p = NULL;
	token.linesCrossed = 0;
	token.flags = 0;
	sprintf( buf, "%1.2f", idMath::Fabs( value ) );
	token = buf;
	token.type = TT_NUMBER;
	token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL;
	UnreadSourceToken( token );
	if( value < 0 )
	{
		UnreadSignToken();
	}

	return true;
}

/*
================
idParser::ReadDirective
================
*/
bool idParser::ReadDirective()
{
	idToken token;

	//read the directive name
	if( !ReadSourceToken( &token ) )
	{
		Error( "found '#' without name" );
		return false;
	}
	//directive name must be on the same line
	if( token.linesCrossed > 0 )
	{
		UnreadSourceToken( token );
		Error( "found '#' at end of line" );
		return false;
	}

	//if it is a name
	if( token.type == TT_NAME )
	{
		if( token == "if" )
		{
			return Directive_if();
		}
		else if( token == "ifdef" )
		{
			return Directive_ifdef();
		}
		else if( token == "ifndef" )
		{
			return Directive_ifndef();
		}
		else if( token == "elif" )
		{
			return Directive_elif();
		}
		else if( token == "else" )
		{
			return Directive_else();
		}
		else if( token == "endif" )
		{
			return Directive_endif();
		}

		if( skip > 0 )
		{
			// skip the rest of the line
			while( ReadLine( &token, false ) )
			{
			}
			return true;
		}

		if( token == "include" )
		{
			return Directive_include();
		}
		else if( token == "define" )
		{
			return Directive_define( false );
		}
		else if( token == "undef" )
		{
			return Directive_undef();
		}
		else if( token == "line" )
		{
			return Directive_line();
		}
		else if( token == "error" )
		{
			return Directive_error();
		}
		else if( token == "warning" )
		{
			return Directive_warning();
		}
		else if( token == "pragma" )
		{
			return Directive_pragma();
		}
		else if( token == "eval" )
		{
			return Directive_eval();
		}
		else if( token == "evalfloat" )
		{
			return Directive_evalfloat();
		}
	}
	Error( "unknown precompiler directive '%s'", token.c_str() );
	return false;
}

/*
================
idParser::DollarDirective_if_def

   multi-line if with parenthesis
================
*/
bool idParser::DollarDirective_if_def( int type )
{
	idToken token;

	if( !ReadSourceToken( &token ) )
	{
		Error( "$ifdef without name" );
		return false;
	}
	if( token.type != TT_NAME )
	{
		UnreadSourceToken( token );
		Error( "expected name after $ifdef, found '%s'", token.c_str() );
		return false;
	}

	auto d = FindHashedDefine( definehash, token.c_str() );
	int skip = ( type == INDENT_IFDEF ) == ( d == NULL );
	PushIndent( type, skip, !skip );
	return true;
}

/*
================
idParser::DollarDirective_ifdef
================
*/
bool idParser::DollarDirective_ifdef( void )
{
	return DollarDirective_if_def( INDENT_IFDEF );
}

/*
================
idParser::DollarDirective_ifndef
================
*/
bool idParser::DollarDirective_ifndef( void )
{
	return DollarDirective_if_def( INDENT_IFNDEF );
}

/*
================
idParser::DollarDirective_else
================
*/
bool idParser::DollarDirective_else()
{
	int type, skip, skipElse;

	PopIndent( type, skip, skipElse );
	if( !type )
	{
		Error( "misplaced $else" );
		return false;
	}
	if( type == INDENT_ELSE )
	{
		Error( "$else after $else" );
		return false;
	}

	PushIndent( INDENT_ELSE, skipElse, 1 );
	return true;
}

/*
================
idParser::DollarDirective_endif
================
*/
bool idParser::DollarDirective_endif()
{
	int type, skip, skipElse;

	PopIndent( type, skip, skipElse );
	if( !type )
	{
		Error( "misplaced $endif" );
		return false;
	}

	return true;
}

/*
================
idParser::DollarDirective_if
================
*/
bool idParser::DollarDirective_if()
{
	signed int value = 0;
	int skip;

	if( !this->skip )
	{
		if( !DollarEvaluate( &value, NULL, true ) )
		{
			return false;
		}
		skip = ( value == 0 );
	}
	else
	{
		skip = 1;
	}

	PushIndent( INDENT_IF, skip, !skip );
	return true;
}

/*
================
idParser::DollarDirective_elif

   multi-line elif with parenthesis
================
*/
bool idParser::DollarDirective_elif()
{
	signed int value = 0;
	int type, skip, skipElse;

	PopIndent( type, skip, skipElse );
	if( !type || type == INDENT_ELSE )
	{
		Error( "misplaced $elif" );
		return false;
	}
	if( !this->skip )
	{
		if( !DollarEvaluate( &value, NULL, true ) )
		{
			return false;
		}
		skip = skipElse || ( value == 0 );
	}
	else
	{
		skip = 1;
	}

	skipElse = skipElse || !skip;
	PushIndent( INDENT_ELIF, skip, skipElse );
	return true;
}

/*
================
idParser::DollarDirective_evalint

   multi-line evaluate integer with parenthesis
================
*/
bool idParser::DollarDirective_evalint()
{
	signed int value;
	idToken token;
	char buf[ 128 ];

	if( !DollarEvaluate( &value, NULL, true ) )
	{
		return false;
	}

	token.line = scriptstack->GetLineNum();
	token.whiteSpaceStart_p = NULL;
	token.whiteSpaceEnd_p = NULL;
	token.linesCrossed = 0;
	token.flags = 0;
	sprintf( buf, "%d", abs( value ) );
	token = buf;
	token.type = TT_NUMBER;
	token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL | TT_VALUESVALID;
	token.intvalue = idMath::Abs( value );
	token.floatvalue = idMath::Abs( value );
	UnreadSourceToken( token );
	if( value < 0 )
	{
		UnreadSignToken();
	}

	return true;
}

/*
================
idParser::DollarDirective_evalfloat

   multi-line evalulate float with parenthesis
================
*/
bool idParser::DollarDirective_evalfloat()
{
	double value;
	idToken token;
	char buf[ 128 ];

	if( !DollarEvaluate( NULL, &value, false ) )
	{
		return false;
	}

	token.line = scriptstack->GetLineNum();
	token.whiteSpaceStart_p = NULL;
	token.whiteSpaceEnd_p = NULL;
	token.linesCrossed = 0;
	token.flags = 0;
	sprintf( buf, "%1.2f", idMath::Fabs( value ) );
	token = buf;
	token.type = TT_NUMBER;
	token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL | TT_VALUESVALID;
	token.intvalue = ( unsigned int )idMath::Fabs( value );
	token.floatvalue = idMath::Fabs( value );
	UnreadSourceToken( token );
	if( value < 0 )
	{
		UnreadSignToken();
	}

	return true;
}

/*
================
idParser::ReadDollarDirective
================
*/
bool idParser::ReadDollarDirective()
{
	idToken token;

	// read the directive name
	if( !ReadSourceToken( &token ) )
	{
		Error( "found '$' without name" );
		return false;
	}
	// directive name must be on the same line
	if( token.linesCrossed > 0 )
	{
		UnreadSourceToken( token );
		Error( "found '$' at end of line" );
		return false;
	}

	// if it is a name
	if( token.type == TT_NAME )
	{
		if( token == "if" )
		{
			return DollarDirective_if();
		}
		else if( token == "ifdef" )
		{
			return DollarDirective_ifdef();
		}
		else if( token == "ifndef" )
		{
			return DollarDirective_ifndef();
		}
		else if( token == "elif" )
		{
			return DollarDirective_elif();
		}
		else if( token == "else" )
		{
			return DollarDirective_else();
		}
		else if( token == "endif" )
		{
			return DollarDirective_endif();
		}
		else if( token == "breakonparse" )
		{
			return true;
		}

		if( skip > 0 ) {
			return true;
		}

		if( token == "evalint" )
		{
			return DollarDirective_evalint();
		}
		else if( token == "evalfloat" )
		{
			return DollarDirective_evalfloat();
		}
		else if( token == "template" )
		{
			return Directive_define( true );
		}
	}

	UnreadSourceToken( token );
	return false;
}

/*
================
idParser::ReadToken
================
*/
bool idParser::ReadToken( idToken* token )
{
	while( 1 )
	{
		if( !ReadSourceToken( token ) ) {
			return false;
		}

		// if dollar directives are allowed
		if( ( scriptstack->GetFlags() & LEXFL_NODOLLARPRECOMPILE ) == 0 )
		{
			// check for special precompiler directives
			if( token->type == TT_PUNCTUATION && ( *token )[ 0 ] == '$' && ( *token )[ 1 ] == '\0' )
			{
				// read the precompiler directive
				if( ReadDollarDirective() ) {
					continue;
				}

				return true;
			}
		}

		// check for precompiler directives
		if( token->type == TT_PUNCTUATION && ( *token )[ 0 ] == '#' && ( *token )[ 1 ] == '\0' )
		{
			// read the precompiler directive
			if( !ReadDirective() ) {
				return false;
			}

			continue;
		}

		// if skipping source because of conditional compilation
		if( skip ) {
			continue;
		}

		// recursively concatenate strings that are behind each other still resolving defines
		if( token->type == TT_STRING && !( scriptstack->GetFlags() & LEXFL_NOSTRINGCONCAT ) )
		{
			idToken newtoken;
			if( ReadToken( &newtoken ) )
			{
				if( newtoken.type == TT_STRING )
				{
					token->Append( newtoken.c_str() );
				}
				else {
					UnreadSourceToken( newtoken );
				}
			}
		}
		// if the token is a name
		if( token->type == TT_NAME && !( token->flags & TOKEN_FL_RECURSIVE_DEFINE ) )
		{
			// check if the name is a define macro
			auto define = FindHashedDefine( definehash, token->c_str() );
			// if it is a define macro
			if( define != NULL )
			{
				// expand the defined macro
				if( !ExpandDefineIntoSource( token, define ) ) {
					return false;
				}

				continue;
			}
		}
		// found a token
		return true;
	}

	return false;
}

/*
================
idParser::ExpectTokenString
================
*/
bool idParser::ExpectTokenString( const char* string, idToken* other )
{
	idToken temp;
	idToken* token = other ? other : &temp;

	if( !ReadToken( token ) )
	{
		Error( "couldn't find expected '%s'", string );
		return false;
	}

	if( *token != string )
	{
		Error( "expected '%s' but found '%s'", string, token->c_str() );
		return false;
	}

	return true;
}

/*
================
idParser::ExpectTokenType
================
*/
bool idParser::ExpectTokenType( int type, int subtype, idToken* token )
{
	idStr str;

	if( !ReadToken( token ) )
	{
		Error( "couldn't read expected token" );
		return false;
	}

	if( token->type != type )
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
		Error( "expected a %s but found '%s'", str.c_str(), token->c_str() );
		return false;
	}
	if( token->type == TT_NUMBER )
	{
		if( ( token->subtype & subtype ) != subtype )
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
			Error( "expected %s but found '%s'", str.c_str(), token->c_str() );
			return false;
		}
	}
	else if( token->type == TT_PUNCTUATION )
	{
		if( subtype < 0 )
		{
			Error( "BUG: wrong punctuation subtype" );
			return false;
		}
		if( token->subtype != subtype )
		{
			Error( "expected '%s' but found '%s'", scriptstack->GetPunctuationFromId( subtype ), token->c_str() );
			return false;
		}
	}

	return true;
}

/*
================
idParser::ExpectAnyToken
================
*/
bool idParser::ExpectAnyToken( idToken* token )
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
idParser::CheckTokenString
================
*/
bool idParser::CheckTokenString( const char* string )
{
	idToken tok;

	if( !ReadToken( &tok ) )
	{
		return false;
	}
	//if the token is available
	if( tok == string )
	{
		return true;
	}

	UnreadSourceToken( tok );
	return false;
}

/*
================
idParser::CheckTokenType
================
*/
bool idParser::CheckTokenType( int type, int subtype, idToken* token )
{
	idToken tok;

	if( !ReadToken( &tok ) )
	{
		return false;
	}
	//if the type matches
	if( tok.type == type && ( tok.subtype & subtype ) == subtype )
	{
		*token = tok;
		return true;
	}

	UnreadSourceToken( tok );
	return false;
}

/*
================
idParser::PeekTokenString
================
*/
bool idParser::PeekTokenString( const char* string )
{
	idToken tok;

	if( !ReadToken( &tok ) )
	{
		return false;
	}

	UnreadSourceToken( tok );

	// if the token is available
	if( tok == string )
	{
		return true;
	}

	return false;
}

/*
================
idParser::PeekTokenType
================
*/
bool idParser::PeekTokenType( int type, int subtype, idToken* token )
{
	idToken tok;

	if( !ReadToken( &tok ) )
	{
		return false;
	}

	UnreadSourceToken( tok );

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
idParser::SkipUntilString
================
*/
bool idParser::SkipUntilString( const char *string, idToken* token )
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
idParser::SkipRestOfLine
================
*/
bool idParser::SkipRestOfLine()
{
	idToken token;

	while( ReadToken( &token ) )
	{
		if( token.linesCrossed )
		{
			UnreadSourceToken( token );
			return true;
		}
	}

	return false;
}

/*
=================
idParser::SkipBracedSection

Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
bool idParser::SkipBracedSection( bool parseFirstBrace )
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
idParser::ParseBracedSectionExact

	The next token should be an open brace.
	Parses until a matching close brace is found.
	Maintains the exact formating of the braced section

	FIXME: what about precompilation ?
=================
*/
bool idParser::ParseBracedSectionExact( idStr& out, int tabs, bool parseFirstBrace )
{
	return scriptstack->ParseBracedSectionExact( out, tabs, parseFirstBrace );
}

/*
========================
idParser::ParseBracedSection

	The next token should be an open brace.
	Parses until a matching close brace is found.
	Internal brace depths are properly skipped.
========================
*/
const char* idParser::ParseBracedSection( idStr& out, int tabs, bool parseFirstBrace, char intro, char outro )
{
	idStringBuilder_Heap builder;

	char temp[ 2 ] = { '\0', '\0' };
	*temp = intro;

	out.Empty();
	if( parseFirstBrace )
	{
		if( !ExpectTokenString( temp ) )
		{
			return out.c_str();
		}
		builder = temp;
	}

	idToken token;
	int i, depth = 1;
	bool doTabs = ( tabs >= 0 );

	do {
		if( !ReadToken( &token ) )
		{
			Error( "missing closing brace" );
			builder.ToString( out );
			return out.c_str();
		}

		// if the token is on a new line
		for( i = 0; i < token.linesCrossed; i++ )
		{
			builder += "\r\n";
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
				builder += "\t";
			}
		}

		if( token.type == TT_STRING )
		{
			if( builder.Length() /*&& token.WhiteSpaceBeforeToken()*/ )
			{
				builder += " ";
			}
			builder += "\"" + token + "\"";
		}
		else if( token.type == TT_LITERAL )
		{
			if( builder.Length() /*&& token.WhiteSpaceBeforeToken()*/ )
			{
				builder += " ";
			}
			builder += "\'" + token + "\'";
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

			if( builder.Length() /*&& token.WhiteSpaceBeforeToken()*/ )
			{
				builder += " ";
			}
			builder += token;
		}

		//out += " ";
	}
	while( depth );

	builder.ToString( out );
	return out.c_str();
}

/*
=================
idParser::ParseRestOfLine

  parse the rest of the line
=================
*/
const char* idParser::ParseRestOfLine( idStr& out )
{
	idToken token;

	out.Empty();
	while( ReadToken( &token ) )
	{
		if( token.linesCrossed )
		{
			UnreadSourceToken( token );
			break;
		}
		if( out.Length() )
		{
			out += " ";
		}
		out += token;
	}
	return out.c_str();
}

/*
================
idParser::UnreadToken
================
*/
void idParser::UnreadToken( const idToken& token )
{
	UnreadSourceToken( token );
}

/*
================
idParser::ReadTokenOnLine
================
*/
bool idParser::ReadTokenOnLine( idToken* token )
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

	UnreadSourceToken( tok );
	token->Clear();
	return false;
}

/*
================
idParser::ParseInt
================
*/
int idParser::ParseInt()
{
	idToken token;

	if( !ReadToken( &token ) )
	{
		Error( "couldn't read expected integer" );
		return 0;
	}
	if( token.type == TT_PUNCTUATION && token == "-" )
	{
		ExpectTokenType( TT_NUMBER, TT_INTEGER, &token );
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
idParser::ParseBool
================
*/
bool idParser::ParseBool()
{
	idToken token;

	if( !ExpectTokenType( TT_NUMBER, 0, &token ) )
	{
		Error( "couldn't read expected boolean" );
		return false;
	}
	return( token.GetIntValue() != 0 );
}

/*
================
idParser::ParseFloat
================
*/
float idParser::ParseFloat( bool* hadError )
{
	idToken token;

	if( hadError != NULL )
	{
		*hadError = false;
	}

	if( !ReadToken( &token ) )
	{
		if( hadError != NULL )
		{
			*hadError = true;
		}
		Error( "couldn't read expected floating point number" );
		return 0.0f;
	}
	if( token.type == TT_PUNCTUATION && token == "-" )
	{
		ExpectTokenType( TT_NUMBER, 0, &token );
		return -token.GetFloatValue();
	}
	else if( token.type != TT_NUMBER )
	{
		if( hadError != NULL )
		{
			*hadError = true;
		}
		Error( "expected float value, found '%s'", token.c_str() );
	}
	return token.GetFloatValue();
}

/*
================
idParser::Parse1DMatrix
================
*/
bool idParser::Parse1DMatrix( int x, float* m )
{
	if( !ExpectTokenString( "(" ) )
	{
		return false;
	}

	for( int i = 0; i < x; i++ )
	{
		m[ i ] = ParseFloat();
	}

	if( !ExpectTokenString( ")" ) )
	{
		return false;
	}
	return true;
}

/*
================
idParser::Parse2DMatrix
================
*/
bool idParser::Parse2DMatrix( int y, int x, float* m )
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
idParser::Parse3DMatrix
================
*/
bool idParser::Parse3DMatrix( int z, int y, int x, float *m )
{
	if( !ExpectTokenString( "(" ) )
	{
		return false;
	}

	for( int i = 0; i < z; i++ )
	{
		if( !Parse2DMatrix( y, x, m + i * x*y ) )
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
idParser::GetNextWhiteSpace
================
*/
int idParser::GetNextWhiteSpace( idStr &whiteSpace, bool currentLine )
{
	if( scriptstack )
	{
		scriptstack->GetNextWhiteSpace( whiteSpace, currentLine );
	}
	else
	{
		whiteSpace.Clear();
	}
	return whiteSpace.Length();
}

/*
================
idParser::GetLastWhiteSpace
================
*/
int idParser::GetLastWhiteSpace( idStr& whiteSpace ) const
{
	if( scriptstack )
	{
		scriptstack->GetLastWhiteSpace( whiteSpace );
	}
	else
	{
		whiteSpace.Clear();
	}
	return whiteSpace.Length();
}

/*
================
idParser::SetMarker
================
*/
void idParser::SetMarker()
{
	marker_p = NULL;
}

/*
================
idParser::GetStringFromMarker

  FIXME: this is very bad code, the script isn't even garrenteed to still be around
================
*/
void idParser::GetStringFromMarker( idStr& out, bool clean )
{
	char*	p;
	char	save;

	if( marker_p == NULL )
	{
		marker_p = scriptstack->buffer;
	}

	if( tokens )
	{
		p = ( char* )tokens->whiteSpaceStart_p;
	}
	else
	{
		p = ( char* )scriptstack->script_p;
	}

	// Set the end character to NULL to give us a complete string
	save = *p;
	*p = 0;

	// If cleaning then reparse
	if( clean )
	{
		idParser temp( marker_p, idStr::Length( marker_p ), "temp", flags );
		idToken token;
		while( temp.ReadToken( &token ) )
		{
			out += token;
		}
	}
	else
	{
		out = marker_p;
	}

	// restore the character we set to NULL
	*p = save;
}

/*
================
idParser::SetIncludePath
================
*/
void idParser::SetIncludePath( const char* path )
{
	includepath = path;
	// add trailing path seperator
	if( includepath[ includepath.Length() - 1 ] != '\\' && includepath[ includepath.Length() - 1 ] != '/' )
	{
		includepath += PATHSEPARATOR_STR;
	}
}

/*
================
idParser::SetPragmaCallback
================
*/
void idParser::SetPragmaCallback( void *data, pragmaFunc_t func )
{
	pragmaCallback = func;
	pragmaData = data;
}

/*
================
idParser::SetPunctuations
================
*/
void idParser::SetPunctuations( const punctuation_t* p )
{
	punctuations = p;
}

/*
================
idParser::GetPunctuationFromId
================
*/
const char* idParser::GetPunctuationFromId( int id )
{
	if( !punctuations )
	{
		idLexer lex;
		return lex.GetPunctuationFromId( id );
	}

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
idParser::GetPunctuationId
================
*/
int idParser::GetPunctuationId( const char* p )
{
	if( !punctuations )
	{
		idLexer lex;
		return lex.GetPunctuationId( p );
	}

	for( int i = 0; punctuations[ i ].p; i++ )
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
idParser::SetFlags
================
*/
void idParser::SetFlags( int flags )
{
	this->flags = flags;
	for( idLexer * s = scriptstack; s != NULL; s = s->next )
	{
		s->SetFlags( flags );
	}
}

/*
================
idParser::GetFlags
================
*/
int idParser::GetFlags() const
{
	return flags;
}

/*
================
idParser::LoadFile
================
*/
bool idParser::LoadFile( const char *filename, bool OSPath, int startLine )
{
	if( loaded )
	{
		idLib::common->FatalError( "idParser::loadFile: another source already loaded" );
		return false;
	}

	auto script = new( TAG_IDLIB_PARSER ) idLexer( this->flags );
	script->SetPunctuations( this->punctuations );
	if( !script->LoadFile( filename, OSPath, startLine ) )
	{
		delete script;
		return false;
	}
	script->next = NULL;
	this->OSPath = OSPath;
	this->filename = filename;
	scriptstack = script;
	tokens = NULL;
	indentstack = NULL;
	skip = 0;
	loaded = true;
	pragmaCallback = NULL;
	pragmaData = NULL;

	if( !definehash )
	{
		defines = NULL;
		definehash = idMem::ClearedAlloc< define_t*, TAG_IDLIB_PARSER >( DEFINEHASHSIZE * sizeof( define_t* ) );
		AddGlobalDefinesToSource();
	}
	return true;
}

/*
================
idParser::LoadMemory
================
*/
bool idParser::LoadMemory( const char *ptr, int length, const char *name, int startLine )
{
	this->startLine = startLine;

	if( loaded )
	{
		idLib::common->FatalError( "idParser::LoadMemory: another source already loaded" );
		return false;
	}

	auto script = new( TAG_IDLIB_PARSER ) idLexer( this->flags );
	script->SetPunctuations( this->punctuations );
	if( !script->LoadMemory( ptr, length, name, startLine ) )
	{
		delete script;
		return false;
	}

	script->next = NULL;
	filename = name;
	scriptstack = script;
	tokens = NULL;
	indentstack = NULL;
	skip = 0;
	loaded = true;
	pragmaCallback = NULL;
	pragmaData = NULL;

	if( !definehash )
	{
		defines = NULL;
		definehash = idMem::ClearedAlloc< define_t*, TAG_IDLIB_PARSER >( DEFINEHASHSIZE * sizeof( define_t* ) );
		AddGlobalDefinesToSource();
	}
	return true;
}

/*
================
idParser::LoadMemoryBinary
================
*/
bool idParser::LoadMemoryBinary( const byte *ptr, int length, const char *name, idTokenCache* globals )
{
	this->startLine = 0;

	if( loaded )
	{
		idLib::common->FatalError( "idParser::LoadMemoryBinary: another source already loaded" );
		return false;
	}

	auto script = new( TAG_IDLIB_PARSER ) idLexer( this->flags );
	script->SetPunctuations( this->punctuations );
	if( !script->LoadMemoryBinary( ptr, length, name, globals ) )
	{
		delete script;
		return false;
	}

	script->next = NULL;
	filename = name;
	scriptstack = script;
	tokens = NULL;
	indentstack = NULL;
	skip = 0;
	loaded = true;
	pragmaCallback = NULL;
	pragmaData = NULL;

	if( definehash == NULL )
	{
		defines = NULL;
		definehash = idMem::ClearedAlloc< define_t*, TAG_IDLIB_PARSER >( DEFINEHASHSIZE * sizeof( define_t* ) );
		AddGlobalDefinesToSource();
	}
	return true;
}

/*
================
idParser::FreeSource
================
*/
void idParser::FreeSource( bool keepDefines )
{
	dependencies.Clear();

	// free all the scripts
	while( scriptstack != NULL )
	{
		idLexer* script = scriptstack;
		scriptstack = scriptstack->next;
		delete script;
	}
	// free all the tokens
	while( tokens != NULL )
	{
		idToken* token = tokens;
		tokens = tokens->next;
		delete token;
	}
	// free all indents
	while( indentstack != NULL )
	{
		indent_t* indent = indentstack;
		indentstack = indentstack->next;
		idMem::Free( indent );
	}
	if( !keepDefines )
	{
		if( definehash != NULL )
		{
			// free defines
			for( int i = 0; i < DEFINEHASHSIZE; ++i )
			{
				while( definehash[ i ] != NULL )
				{
					auto define = definehash[ i ];
					definehash[ i ] = definehash[ i ]->hashnext;
					FreeDefine( define );
				}
			}
			defines = NULL;

			// free hash table
			idMem::Free( definehash );
			definehash = NULL;
		}
	}
	loaded = false;
}

/*
================
idParser::idParser
================
*/
idParser::idParser()
{
	loaded = false;
	OSPath = false;
	punctuations = 0;
	flags = 0;
	scriptstack = NULL;
	indentstack = NULL;
	definehash = NULL;
	defines = NULL;
	tokens = NULL;
	pragmaCallback = NULL;
	pragmaData = NULL;
	startLine = 1;
	skip = 0;
	defineScope = 0;
}

/*
================
idParser::idParser
================
*/
idParser::idParser( int flags )
{
	loaded = false;
	OSPath = false;
	punctuations = 0;
	this->flags = flags;
	scriptstack = NULL;
	indentstack = NULL;
	definehash = NULL;
	defines = NULL;
	tokens = NULL;
	pragmaCallback = NULL;
	pragmaData = NULL;
	startLine = 1;
	skip = 0;
	defineScope = 0;
}

/*
================
idParser::idParser
================
*/
idParser::idParser( const char *filename, int _flags, bool _OSPath )
{
	loaded = false;
	OSPath = _OSPath;
	punctuations = 0;
	flags = _flags;
	scriptstack = NULL;
	indentstack = NULL;
	definehash = NULL;
	defines = NULL;
	tokens = NULL;
	pragmaCallback = NULL;
	pragmaData = NULL;
	startLine = 1;
	skip = 0;
	defineScope = 0;
	LoadFile( filename, OSPath );
}

/*
================
idParser::idParser
================
*/
idParser::idParser( const char *ptr, int length, const char *name, int flags )
{
	loaded = false;
	OSPath = false;
	punctuations = 0;
	this->flags = flags;
	scriptstack = NULL;
	indentstack = NULL;
	definehash = NULL;
	defines = NULL;
	tokens = NULL;
	pragmaCallback = NULL;
	pragmaData = NULL;
	startLine = 1;
	skip = 0;
	defineScope = 0;
	LoadMemory( ptr, length, name, startLine );
}

/*
================
idParser::~idParser
================
*/
idParser::~idParser()
{
	FreeSource( false );
}

/*
================
idParser::WriteBinary
================
*/
void idParser::WriteBinary( idFile* f, idTokenCache* tokenCache )
{
	if( scriptstack != NULL )
	{
		idToken token;
		idLexer* base = scriptstack;

		int numTokens = 0;
		while( ReadToken( &token ) )
		{
			base->GetBinary().AddToken( token, tokenCache );
			numTokens++;
		}
		base->GetBinary().Write( f );

		/*
		idLib::common->DPrintf( "%s: %i total tokens\n", filename.c_str(), numTokens );
		*/

		base->GetBinary().SetData( NULL, tokenCache );

		// dump out a text version of everything
	#if 0
		ResetBinaryParsing();

		idStr filename = this->filename;
		filename.SetFileExtension( ".bdump" );
		idFile* f = idLib::fileSystem->OpenFileAppend( filename );
		if( f != NULL )
		{
			while( ReadToken( &token ) )
			{
				if( token.linesCrossed )
				{
					f->WriteFloatString( "\n" );
				}
				f->WriteFloatString( "%s%s%s ", token.type == TT_STRING ? "\"" : "", token.c_str(), token.type == TT_STRING ? "\"" : "" );
			}
			f->WriteFloatString( "\n" );
			f->WriteFloatString( "\n" );
		}
		idLib::fileSystem->CloseFile( f );
	#endif
	}
}

/*
============
idParser::ResetBinaryParsing
============
*/
void idParser::ResetBinaryParsing()
{
	if( scriptstack == NULL )
	{
		return;
	}
	scriptstack->GetBinary().ResetParsing();
}

/*
========================
idParser::EndOfFile
========================
*/
bool idParser::EndOfFile()
{
	if( scriptstack != NULL )
	{
		return scriptstack->EndOfFile();
	}
	return true;
}