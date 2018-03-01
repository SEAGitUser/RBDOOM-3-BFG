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

#ifndef __PARSER_H__
#define __PARSER_H__

/*
===============================================================================

	C/C++ compatible pre-compiler

===============================================================================
*/

#define DEFINE_FIXED			0x0001

#define BUILTIN_LINE			1
#define BUILTIN_FILE			2
#define BUILTIN_DATE			3
#define BUILTIN_TIME			4
#define BUILTIN_STDC			5

#define INDENT_IF				0x0001
#define INDENT_ELSE				0x0002
#define INDENT_ELIF				0x0004
#define INDENT_IFDEF			0x0008
#define INDENT_IFNDEF			0x0010

//SEA: TODO define scope

// macro definitions
struct define_t {
	char *			name;						// define name
	int				scope;
	int				flags;						// define flags
	int				builtin;					// > 0 if builtin define
	int				numparms;					// number of define parameters
	idToken *		parms;						// define parameters
	idToken *		tokens;						// macro tokens (possibly containing parm tokens)
	define_t *		next;						// next defined macro in a list
	define_t *		hashnext;					// next define in the hash chain

	//idListNode<define_t> listNode; 
	//idListNode<define_t> hashNode;
};

// indents used for conditional compilation directives:
// #if, #else, #elif, #ifdef, #ifndef
struct indent_t {
	int				type;						// indent type
	int				skip;						// true if skipping current indent
	int				skipElse;					// true if any following else sections should be skipped
	idLexer *		script;						// script the indent was in
	indent_t *		next;						// next indent on the indent stack
};

typedef void( *pragmaFunc_t )( void *data, const char *pragma );

class idParser {
public:
	// constructor
	idParser();
	idParser( int flags );
	idParser( const char* filename, int flags = 0, bool OSPath = false );
	idParser( const char* ptr, int length, const char* name, int flags = 0 );
	// destructor
	~idParser();
	// load a source file
	bool				LoadFile( const char *filename, bool OSPath = false, int startLine = 1 );
	// load a source from the given memory with the given length
	// NOTE: the ptr is expected to point at a valid C string: ptr[length] == '\0'
	bool				LoadMemory( const char *ptr, int length, const char *name, int startLine = 1 );
	
	bool				LoadMemoryBinary( const byte *ptr, int length, const char *name, idTokenCache* globals );
	void				WriteBinary( idFile*, idTokenCache* = NULL );
	void				ResetBinaryParsing();

	// free the current source
	void				FreeSource( bool keepDefines = false );

	// returns true if a source is loaded
	ID_INLINE bool		IsLoaded() const { return loaded; }

	// read a token from the source
	bool				ReadToken( idToken* token );
	// expect a certain token, reads the token when available
	bool				ExpectTokenString( const char *string, idToken* other = NULL );
	// expect a certain token type
	bool				ExpectTokenType( int type, int subtype, idToken & );
	// expect a token
	bool				ExpectAnyToken( idToken* token );
	// returns true if the next token equals the given string and removes the token from the source
	bool				CheckTokenString( const char* string );
	// returns true if the next token equals the given type and removes the token from the source
	bool				CheckTokenType( int type, int subtype, idToken* token );
	// returns true if the next token equals the given string but does not remove the token from the source
	bool				PeekTokenString( const char* string );
	// returns true if the next token equals the given type but does not remove the token from the source
	bool				PeekTokenType( int type, int subtype, idToken* token );
	// skip tokens until the given token string is read
	bool				SkipUntilString( const char *string, idToken *token = nullptr );
	// skip the rest of the current line
	bool				SkipRestOfLine();
	// skip the braced section
	bool				SkipBracedSection( bool parseFirstBrace = true );
	// parse a braced section into a string ( expands precompilation ).
	const char *		ParseBracedSection( idStr &out, int tabs = -1, bool parseFirstBrace = true, char intro = '{', char outro = '}' );
	// parse a braced section into a string, maintaining indents and newlines, no precompilation.
	bool 				ParseBracedSectionExact( idStr& out, int tabs = -1, bool parseFirstBrace = true );
	// parse the rest of the line
	const char* 		ParseRestOfLine( idStr& out );
	// unread the given token
	void				UnreadToken( const idToken& );
	// read a token only if on the current line
	bool				ReadTokenOnLine( idToken* );
	// read a signed integer
	int					ParseInt();
	// read a boolean
	bool				ParseBool();
	// read a floating point number
	float				ParseFloat( bool* hadError = NULL );
	// parse matrices with floats
	bool				Parse1DMatrix( int x, float* m );
	bool				Parse2DMatrix( int y, int x, float* m );
	bool				Parse3DMatrix( int z, int y, int x, float* m );
	// retrieves the white space after the last read token
	int					GetNextWhiteSpace( idStr &whiteSpace, bool currentLine );
	// retrieves the white space before the last read token
	int					GetLastWhiteSpace( idStr& whiteSpace ) const;
	// Set a marker in the source file (there is only one marker)
	void				SetMarker();
	// Get the string from the marker to the current position
	void				GetStringFromMarker( idStr& out, bool clean = false );
	// add a define to the source
	bool				AddDefine( const char* string );
	// remove define from the source
	void				RemoveDefine( const char* string );
	///void				RemoveDefines( bool freeDefineHashTable );
	// add includes to the source
	int					AddIncludes( const idStrList& includes );
	// add an include to the source
	bool				AddInclude( const char *string );
	// add builtin defines
	void				AddBuiltinDefines();
	// set the source include path
	void				SetIncludePath( const char* path );
	// set pragma callback
	void				SetPragmaCallback( void *data, pragmaFunc_t func );
	// set the punctuation set
	void				SetPunctuations( const punctuation_t* p );
	// returns a pointer to the punctuation with the given id
	const char* 		GetPunctuationFromId( int id );
	// get the id for the given punctuation
	int					GetPunctuationId( const char* p );
	// set lexer flags
	void				SetFlags( int flags );
	// get lexer flags
	int					GetFlags() const;
	// returns the current filename
	const char* 		GetFileName() const;
	// get current offset in current script
	const int			GetFileOffset() const;
	// get file time for current script
	const ID_TIME_T		GetFileTime() const;
	// returns the current line number
	const int			GetLineNum() const;
	// print an error message
	void				Error( VERIFY_FORMAT_STRING const char* str, ... ) const;
	// print a warning message
	void				Warning( VERIFY_FORMAT_STRING const char* str, ... ) const;
	// for enumerating only the current level
	int					GetCurrentDependency() const;
	// walk all included files, start from 0 to enumerate all
	const char*			GetNextDependency( int &index ) const;
	// returns true if at the end of the file
	bool				EndOfFile();
	// add a global define that will be added to all opened sources
	static bool			AddGlobalDefine( const char* string );
	// remove the given global define
	static bool			RemoveGlobalDefine( const char* name );
	// remove all global defines
	static void			RemoveAllGlobalDefines();
	// set the base folder to load files from
	static void			SetBaseFolder( const char* path );

	// save off the state of the dependencies list
	void				PushDependencies();
	void				PopDependencies();

	void PushDefineScope( const char* scopeName );
	void PopDefineScope();

protected:

	///typedef idLinkedList<define_t, &define_t::listNode> defineList_t;

	bool				loaded;						// set when a source file is loaded from file or memory
	bool				OSPath;						// true if the file was loaded from an OS path
	idStr				filename;					// file name of the script
	idStr				includepath;				// path to include files
	const punctuation_t * punctuations;			// punctuations to use
	int					flags;						// flags used for script parsing
	idLexer * 			scriptstack;				// stack with scripts of the source
	idToken * 			tokens;						// tokens to read first
/*SEA*/int				defineScope;
	define_t * 			defines;					// list with macro definitions
	define_t ** 		definehash;					// hash chain with defines
	indent_t * 			indentstack;				// stack with indents
	int					skip;						// > 0 if skipping conditional code
	pragmaFunc_t		pragmaCallback;				// called when a #pragma is parsed
	void *				pragmaData;
	int					startLine;					// line offset
	const char *		marker_p;

	// includeLevel

	// list with global defines added to every source loaded
	///static defineList_t	globalDefines;
	static define_t *	globaldefines;

	idStrList			dependencies;				// list of filenames that have been included
	idStack< int >		dependencyStateStack;		// stack of the number of dependencies

	/*
	struct idDependency {
		includeLevel;
		fileName;
	}
	idList<idParser::indent_t,11>
	idList<idParser::idDependency,11>
	*/

protected:

	void				PushIndent( int type, int skip, int skipElse );
	void				PopIndent( int &type, int &skip, int &skipElse );
	bool				PushScript( idLexer *script );
	bool				ReadSourceToken( idToken* token );
	bool				ReadLine( idToken *token, bool multiline );
	void				UnreadSourceToken( const idToken& token );
	bool				ReadDefineParms( define_t* define, idToken** parms, int maxparms );
	void				StringizeTokens( idToken* tokens, idToken* token );
	bool				MergeTokens( idToken* t1, idToken* t2 );
	void				ExpandBuiltinDefine( idToken* deftoken, define_t* define, idToken** firsttoken, idToken** lasttoken );
	bool				ExpandDefine( idToken* deftoken, define_t* define, idToken** firsttoken, idToken** lasttoken );
	bool				ExpandDefineIntoSource( idToken* deftoken, define_t* define );
	void				AddGlobalDefinesToSource();
	define_t * 			CopyDefine( define_t* define );
	define_t * 			FindHashedDefine( define_t** definehash, const char* name );
	int					FindDefineParm( define_t* define, const char* name );
	void				AddDefineToHash( define_t* define, define_t** definehash );
	static void			PrintDefine( define_t* define );
	static void			FreeDefine( define_t* define );
	static define_t *	FindDefine( define_t* defines, const char* name );
	static define_t *	DefineFromString( const char* string );
	define_t * 			CopyFirstDefine();
	void				UnreadSignToken();

	bool				EvaluateTokens( idToken* tokens, signed int* intvalue, double* floatvalue, int integer );
	bool				Evaluate( signed int* intvalue, double* floatvalue, int integer );
	bool				DollarEvaluate( signed int* intvalue, double* floatvalue, int integer );

	bool				Directive_include();
	bool				Directive_define( bool isTemplate );
	bool				Directive_undef();
	bool				Directive_if_def( int type );
	bool				Directive_ifdef();
	bool				Directive_ifndef();
	bool				Directive_else();
	bool				Directive_endif();
	bool				Directive_elif();
	bool				Directive_if();
	bool				Directive_line();
	bool				Directive_error();
	bool				Directive_warning();
	bool				Directive_pragma();
	bool				Directive_eval();
	bool				Directive_evalfloat();
	bool				ReadDirective();

	bool				DollarDirective_if_def( int type );
	bool				DollarDirective_ifdef();
	bool				DollarDirective_ifndef();
	bool				DollarDirective_else();
	bool				DollarDirective_endif();
	bool				DollarDirective_elif();
	bool				DollarDirective_if();
	bool				DollarDirective_evalint();
	bool				DollarDirective_evalfloat();
	bool				ReadDollarDirective();
};

ID_INLINE const char *idParser::GetFileName() const
{
	return ( scriptstack )? scriptstack->GetFileName() : "" ;
}

ID_INLINE const int idParser::GetFileOffset() const
{
	return ( scriptstack )? scriptstack->GetFileOffset() : 0 ;
}

ID_INLINE const ID_TIME_T idParser::GetFileTime() const
{
	return ( scriptstack )? scriptstack->GetFileTime() : 0 ;
}

ID_INLINE const int idParser::GetLineNum() const
{
	return ( scriptstack )? scriptstack->GetLineNum() : 0 ;
}

#endif /* !__PARSER_H__ */
