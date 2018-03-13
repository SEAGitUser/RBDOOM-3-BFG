
#pragma hdrstop
#include "precompiled.h"

#include "tr_local.h"

#include "DeclRenderParm.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

void ImageEvaluator( const idDeclRenderParm *parm )
{
}
void VectorEvaluator( const idDeclRenderParm *parm )
{
}

#define DECLPARM_FLAGS ( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | \
	LEXFL_NODOLLARPRECOMPILE | LEXFL_NOBASEINCLUDES | LEXFL_ALLOWFLOATEXCEPTIONS | LEXFL_NOEMITSTRINGESCAPECHARS )

/*
=====================================
idDeclRenderParm::DefaultDefinition
=====================================
*/
const char* idDeclRenderParm::DefaultDefinition() const
{
	switch( GetParmType() )
	{
		case PT_VECTOR:
			return "{ Vec { 0.0, 0.0, 0.0, 1.0 } }\n";

		case PT_TEXTURE:
			return "{ Tex { _default } }\n";
	}

	return "{ Vec { 0.0, 0.0, 0.0, 1.0 } }\n";
}

/*
=====================================
 idDeclRenderParm::ParseVector
=====================================
*/
bool idDeclRenderParm::ParseVector( idLexer& src )
{
	idToken	token;

	m_defaults.vector[0] = 0.0;
	m_defaults.vector[1] = 0.0;
	m_defaults.vector[2] = 0.0;
	m_defaults.vector[3] = 1.0;

	/*if( src.CheckTokenString( "PERFRAME" ) )
	{
		SetInfrequentIndex( 0 );
	}
	else if( src.CheckTokenString( "PERSURF" ) )
	{
		SetInfrequentIndex( 1 );
	}*/

	// Parse vector values

	if( !src.ExpectTokenString( "{" ) )
		return false;

	if( !src.ExpectTokenType( TT_NUMBER, 0, token ) ) {
		return false;
	}
	m_defaults.vector[ 0 ] = token.GetFloatValue();
	if( src.CheckTokenString( "}" ) )
	{
		// If we have single float value copy it to all array elements
		m_defaults.vector[ 3 ] = m_defaults.vector[ 2 ] = m_defaults.vector[ 1 ] = m_defaults.vector[ 0 ];
		return true;
	}

	if( src.ExpectTokenString( "," ) )
	{
		if( src.ExpectTokenType( TT_NUMBER, 0, token ) )
		{
			m_defaults.vector[ 1 ] = token.GetFloatValue();

			if( src.CheckTokenString( "}" ) )
				return true;

			if( src.ExpectTokenString( "," ) )
			{
				if( src.ExpectTokenType( TT_NUMBER, 0, token ) )
				{
					m_defaults.vector[ 2 ] = token.GetFloatValue();

					if( src.CheckTokenString( "}" ) )
						return true;

					if( src.ExpectTokenString( "," ) )
					{
						if( src.ExpectTokenType( TT_NUMBER, 0, token ) )
						{
							m_defaults.vector[ 3 ] = token.GetFloatValue();

							if( src.ExpectTokenString( "}" ) )
								return true;
						}
					}
				}
			}
		}
	}
	return false;
#if 0
	if( !src.ExpectTokenString( "," ) )
		return false;

	if( !src.ExpectTokenType( TT_NUMBER, TT_FLOAT, &token ) )
		return false;

	defaults.vector[ 1 ] = token.GetFloatValue();

	if( src.CheckTokenString( "}" ) )
		return true;

	if( !src.ExpectTokenString(",") )
		return false;

	if( !src.ExpectTokenType( TT_NUMBER, TT_FLOAT, &token ) )
		return false;

	defaults.vector[ 2 ] = token.GetFloatValue();

	if( src.CheckTokenString( "}" ) )
		return true;

	if( !src.ExpectTokenString( "," ) )
		return false;

	if( !src.ExpectTokenType( TT_NUMBER, TT_FLOAT, &token ) )
		return false;

	defaults.vector[ 3 ] = token.GetFloatValue();

	return src.ExpectTokenString( "}" );

	//////////////////////////////////////////

	if( !src.ExpectTokenString( "{" ) ) {
		return false;
	}

	idStrStatic<128> value;
	src.ParseBracedSection( value, -1, false );
	if( src.HadError() || src.HadWarning() || value.IsEmpty() ) {
		return false;
	}
	idLexer lex( value.c_str(), value.Length(), "ParseVector", DECLPARM_FLAGS, GetLineNum() );

	int i = 0;
	defaults.vector.Set( 0.0, 0.0, 0.0, 1.0 );
	while( !lex.EndOfFile() )
	{
		if( !src.ExpectTokenType( TT_NUMBER, TT_FLOAT, &token ) ) {
			return false;
		}
		defaults.vector[ i ] = token.GetFloatValue();

		if( !src.CheckTokenString(",") )
		{
			if( i == 0 ) {
				defaults.vector[ 3 ] = defaults.vector[ 2 ] = defaults.vector[ 1 ] = defaults.vector[ 0 ];
			}
			break;
		}
	}
	return true;
#endif
}

/*
=====================================
 idDeclRenderParm::Parse
=====================================
*/
bool idDeclRenderParm::ParseTexture( idLexer& src )
{
	m_defaults.texture.image = renderImageManager->defaultImage;
	m_defaults.texture.defaultLayout = IMG_LAYOUT_2D;
	m_defaults.texture.defaultUsage = TD_DEFAULT;

	if( !src.ExpectTokenString( "{" ) )
		return false;

	idToken	token;

	// Check usage settings

	/*/*/if( src.CheckTokenString( "bump" ) )
	{
		m_defaults.texture.defaultUsage = TD_BUMP;
	} 
	else if( src.CheckTokenString( "diffuse" ) )
	{
		m_defaults.texture.defaultUsage = TD_DIFFUSE;
	}
	else if( src.CheckTokenString( "specular" ) )
	{
		m_defaults.texture.defaultUsage = TD_SPECULAR;
	}
	else if( src.CheckTokenString( "shadow" ) )
	{
		m_defaults.texture.defaultUsage = TD_SHADOWMAP;
	}
	else if( src.CheckTokenString( "coverage" ) )
	{
		m_defaults.texture.defaultUsage = TD_COVERAGE;
	}

	// Check texture layout settings

	/*/*/if( src.CheckTokenString( "cubemap" ) )
	{
		m_defaults.texture.defaultLayout = IMG_LAYOUT_CUBE_NATIVE;
	}
	else if( src.CheckTokenString( "cameraCubemap" ) )
	{
		m_defaults.texture.defaultLayout = IMG_LAYOUT_CUBE_CAMERA;
	}

	// The rest of the line is the name

	if( !src.ExpectTokenType( TT_NAME, 0, token ) )
		return false;

	m_defaults.texture.image = renderImageManager->GetImage( token );
	if( !m_defaults.texture.image ) 
	{
		m_defaults.texture.image = renderImageManager->defaultImage;
		src.Error("ParseTexture() %s image not found", token.c_str() );
		return false;
	}
	//m_defaults.texture.defaultLayout = m_defaults.texture.image->layout;
	//m_defaults.texture.defaultUsage = m_defaults.texture.image->usage;
	//SEA: ??? TODO!!!


	return src.ExpectTokenString( "}" );
}

/*
=====================================
 idDeclRenderParm::Parse
=====================================
*/
bool idDeclRenderParm::Parse( const char* text, const int textLength, bool allowBinaryVersion )
{
	const punctuation_t parm_punctuations[] = {
		{ "(", P_PARENTHESESOPEN },	
		{ ")", P_PARENTHESESCLOSE },
		{ "{", P_BRACEOPEN },
		{ "}", P_BRACECLOSE },
		{ ",", P_COMMA },
		{ ".", P_MEMBER_SELECTION_OBJECT },
		{ NULL, 0 }
	};
	idLexer src( DECLPARM_FLAGS );
	src.SetPunctuations( parm_punctuations );
	if( !src.LoadMemory( text, textLength, GetName(), GetLineNum() ) ) {
		return false;
	}

	src.SkipUntilString( "{" );

	idToken	token;
	src.ExpectTokenType( TT_NAME, 0, token );

	if( token.is("Vec") )
	{
		m_type = PT_VECTOR;
		return ParseVector( src );
	}
	else if( token.is( "Tex" ) )
	{
		m_type = PT_TEXTURE;
		return ParseTexture( src );
	}
	/*else if( token.is( "Smp" ) )
	{
		m_type = PT_SAMPLER;
		return ParseTexture( src );
	}*/
	else {
		src.Error("File %s. Line %i. Parm %s. Unknown binding type.", GetFileName(), GetLineNum(), GetName() );
		return false;
	}

	return src.ExpectTokenString( "}" );
}

/*
=====================================
idDeclRenderParm::SetDefaultData
=====================================
*/
void idDeclRenderParm::SetDefaultData() const
{
	m_data = m_defaults;
}

/*
=====================================
idDeclRenderParm::FreeData
=====================================
*/
void idDeclRenderParm::FreeData()
{

}

/*
=====================================
idDeclRenderParm::List
=====================================
*/
void idDeclRenderParm::Print() const
{
	if( GetState() == DS_PARSED )
	{
		if( GetParmType() == PT_VECTOR )
		{
			common->Printf( "%s Vec { %f, %f, %f, %f }\n", GetName(), GetDefaultVector()[ 0 ], GetDefaultVector()[ 1 ], GetDefaultVector()[ 2 ], GetDefaultVector()[ 3 ] );
			return;
		}
		else if( GetParmType() == PT_TEXTURE )
		{
			const char * name = m_defaults.texture.image ? m_defaults.texture.image->GetName() : "Empty Image";
			common->Printf( "%s Tex { %s }\n", GetName(), name );
			return;
		}
		else if( GetParmType() == PT_ATTRIB )
		{
			common->Printf( "%s Attrib { TODO? }\n", GetName() );
			return;
		}
	}

	common->Printf( "%s Invalid parm\n", GetName() );
}

/*
=================================================
ListFramebuffers_f
=================================================
*/
/*void ListRenderParms_f( const idCmdArgs& args )
{
	for( int i = 0; i < renderDestManager.renderDestList.Num(); i++ )
	{
		auto dest = renderDestManager.renderDestList[ i ];
		if( dest != nullptr )
		{
			common->Printf( S_COLOR_BLUE "RD%i:" S_COLOR_DEFAULT " %s w:%i, h:%i\n", i, dest->GetName(), dest->GetTargetWidth(), dest->GetTargetHeight() );
			dest->Print();
		}
		else
		{
			common->Printf( S_COLOR_BLUE "RD%i:" S_COLOR_DEFAULT " NULL\n", i );
		}
	}
}*/