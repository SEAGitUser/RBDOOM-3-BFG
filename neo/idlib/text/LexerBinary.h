
// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __LEXERBINARY_H__
#define __LEXERBINARY_H__

/*
===============================================================================

	Binary Lexer

===============================================================================
*/

#define LEXB_EXTENSION		".lexb"
#define LEXB_VERSION		"LEXB01"

class idTokenCache {
public:
							idTokenCache();
							~idTokenCache();

	unsigned short			FindToken( const idToken& token );
	void					Swap( idTokenCache& rhs );
	void					Clear();

	bool					Write( class idFile* );
	bool					Read( class idFile* );
	bool					ReadBuffer( const byte* buffer, int length );
	int						Num() const;
	size_t					Allocated() const { return uniqueTokens.Allocated() + uniqueTokenHash.Allocated(); }

	const idToken&			operator[]( int index ) const { return uniqueTokens[ index ]; }

private:
	idList<idToken>			uniqueTokens;
	idHashIndexUShort		uniqueTokenHash;
};

ID_INLINE idTokenCache::idTokenCache() 
{
	uniqueTokens.SetGranularity( 128 );
	uniqueTokenHash.SetGranularity( 128 );
}

ID_INLINE idTokenCache::~idTokenCache() {
}

ID_INLINE void idTokenCache::Swap( idTokenCache& rhs ) 
{
	uniqueTokens.Swap( rhs.uniqueTokens );
	uniqueTokenHash.Swap( rhs.uniqueTokenHash );
}

ID_INLINE void idTokenCache::Clear()
{	
	uniqueTokens.Clear();
	uniqueTokenHash.Clear();
}

ID_INLINE int idTokenCache::Num( void ) const 
{
	return uniqueTokens.Num();
}

class idLexerBinary {
public:
							idLexerBinary();
							~idLexerBinary();

	bool					Write( class idFile* );
	bool					Read( class idFile* );
	bool					ReadBuffer( const byte* buffer, int length );

	bool					IsLoaded() const { return isLoaded; }
	bool					EndOfFile() const;
	
	void					AddToken( const idToken& token, idTokenCache* cache = NULL );

	bool					ReadToken( idToken *token );
	void					Swap( idLexerBinary& rhs );	

	void					Clear();
	void					ResetParsing();

	int						NumUniqueTokens() const;
	int						NumTokens() const;
	const idList<idToken>&	GetUniqueTokens() const;

	void					SetData( const idList<unsigned short>* indices, const idTokenCache* cache );
	
	const idList<unsigned short>& GetTokenStream() const { return tokens; }

	size_t					Allocated() const { return tokens.Allocated() + tokenCache.Allocated(); }

private:
	bool					isLoaded;
	int						nextToken;
	
	idTokenCache			tokenCache;		
	idList<unsigned short>	tokens;
	
									// allow clients to set their own data
	const idList<unsigned short>*	tokensData;
	const idTokenCache*				tokenCacheData;
};

ID_INLINE idLexerBinary::idLexerBinary()
{
	Clear();
	tokens.SetGranularity( 256 );
}

ID_INLINE idLexerBinary::~idLexerBinary() 
{
}

ID_INLINE void idLexerBinary::Swap( idLexerBinary& rhs )
{
	tokens.Swap( rhs.tokens );
	tokenCache.Swap( rhs.tokenCache );
	SwapValues( nextToken, rhs.nextToken );
	SwapValues( isLoaded, rhs.isLoaded );
}

ID_INLINE void idLexerBinary::Clear()
{	
	isLoaded = false;
	nextToken = 0;
	tokens.Clear();
	tokenCache.Clear();
	tokensData = NULL;
	tokenCacheData = NULL;
}

ID_INLINE void idLexerBinary::ResetParsing() 
{
	nextToken = 0;
}

ID_INLINE int idLexerBinary::NumTokens() const 
{
	return tokensData != NULL ? tokensData->Num() : tokens.Num();
}

ID_INLINE void idLexerBinary::SetData( const idList<unsigned short>* indices, const idTokenCache* cache )
{
	tokenCacheData = cache;
	tokensData = indices;
	isLoaded = true;
}


ID_INLINE bool idLexerBinary::EndOfFile() const
{
	return( nextToken >= tokens.Num() );
}

#endif /* !__LEXERBINARY_H__ */
