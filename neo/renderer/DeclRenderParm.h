#ifndef __DECLRENDERPARM_H__
#define __DECLRENDERPARM_H__

/*
===============================================================================

idDeclRenderParm

	DefaultDefinition(  );
	Parse(  );
	Print(  );
	List(  );
	ParseStringToValue(  );
	ParseVectorConstant(  );
	ParseImageLine(  );
	SetTemporary(  );
	GetParmIndex(  );
	GetType(  );
	GetCreator(  );
	CubeFilterTexture(  );
	GetDefaultFloat(  );
	GetDefaultVec2(  );
	GetDefaultVec3(  );
	GetDefaultVec4(  );
	GetDefaultImage(  );
	GetDefaultSampler(  );
	GetDefaultProgram(  );
	GetDefaultString(  );
	SetDeclaredFloat(  );
	GetDeclaredFloat(  );
	GetDeclaredValue(  );
		int parmIndex;
		parmType_t parmType;
		parmCreator_t creator;
		bool cubeFilterTexture;
		parmValue_t declaredValue;
		parmEdit_t edit;
		float int editRange;
	ParseValue(  );

	enum parmType_t {
		PT_VECTOR,
		PT_TEXTURE_2D,
		PT_TEXTURE_3D,
		PT_TEXTURE_CUBE,
		PT_TEXTURE_SHADOW_2D,
		PT_TEXTURE_SHADOW_3D,
		PT_TEXTURE_SHADOW_CUBE,
		PT_TEXTURE_DIM_SHADOW_2D,
		PT_TEXTURE_MULTISAMPLE_2D,
		PT_SAMPLER,
		PT_PROGRAM,
		PT_STRING,
		PT_MAX
	};

	union parmValue_t {
		float				value[4];
		idImage *			image;
		idTextureSampler *	sampler;
		idDeclRenderProg *	program;
		char *				string;
		unsigned short		swizzle;

	//operator== ( const parmValue_t& );
	//operator!= ( const parmValue_t& );
	};


	parmIndex   int 
	parmType_t  int parmType    
	int parmCreator_t   creator 
	bool    cubeFilterTexture   *   
	parmValue_t    declaredValue   
	parmEdit_t  edit    
	float   int editRange   
	int idDeclRenderProg    int renamed 
	renderProgData_t    data    
	*   idDeclRenderProg    *   inheritsFromDecl    [6] 
	unsigned int    vertexProgramChecksum   
	unsigned int    fragmentProgramChecksum 
	bool    hasDerived  
	bool    partiallyLoaded 
	idArray < const idDeclRenderProg * , 3 >    [7] versions    
	
	float   value   *   
	idImage image   
	idTextureSampler    sampler 
	idDeclRenderProg     program 
	char    string  
	swizzle unsigned short  
	
	type    
	short   parmIndexDest   
	short   parmIndexA  
	short   int parmIndexB  int 
	idList < expOp_t , TAG_RENDERPARM > int 
	idList < parmValue_t , TAG_RENDERPARM > constants

	//D4

	editorStippleLine       editorStippleScale      editorStipplePattern    
	Reentrant rendering     { vec 0 }
	HhðB   borderClamp clampS  bc4 bc6h    LuminanceAlpha  pad2    pad4    pad8    pad16   fullScaleBias   early_fragment_tests    
	rgba32f rgba16f rg32f   rg16f   r11f_g11f_b10f  r32f    r16f    rgba32ui        rgba16ui        rgb10_a2ui      rgba8ui rg32ui  
	rg16ui  rg8ui   r32ui   r16ui   r8ui    rgba32i rgba16i rgba8i  rg32i   rg16i   rg8i    r32i    r16i    r8i rgba16      rgb10_a2    
	rgba8   rg16    rg8 r16 r8  rgba16_snorm    rgba8_snorm     rg16_snorm      rg8_snorm       r16_snorm       r8_snorm        
	freqLow freqHigh        Invalid set frequency   Bad negative number '%s'        Non numeric term in vector constant    
	Over four elements in a vector constant Missing comma in vector constant        Vector constant with no elements        
	Non numeric constant    no renderParm name set
	writable_cs     Buffer Object does not specify any type as a structure
	RenderParm %s does NOT have a matching structured buffer in the manager!
	renderParm named "%s" is not a typed buffer
	Bad renderParm type: %s
	value string    Tex Tex2D   Tex3D       TexCube TexShadow2D     TexShadow3D     TexShadowCube   TexArray2D      TexArrayCube    
	TexMultisample2D        environment     Sampler Program Vec String      StructuredBuffer        UniformBuffer   imageBuffer     
	imageStoreBuffer2D      imageStoreBuffer2DArray imageStoreBuffer3D  Bool    srgb    srgba   Range       setFreqLow      Unknown edit specifier '%s'
	Vector %f %f %f %f
	Tex2D       WARNING: NULL
	Tex3D   TexCube         TexShadow2D     TexShadow3D     TexShadowCube   TexArray2D      TexArrayCube    TexMultisample2D        Sampler NULL

	--------- COMMIT ---------
	%s model commits
	%s light commits
	%s models total
	%s lights total
	%s approx-light models
	%s add-always models
	%s trans-sort quads
	%s trans-add quads
	---------- VIEW ----------
	%s world chunks
	%s models
	%s lights
	%s shadows
	%s surfaces
	%s worldTris
	%s dynamicTris
	------------- GPU ------------- DrawCalls ---
	%s ms depth        [%s]
	%s ms shadows      [%s]
	%s ms opaque       [%s]
	%s ms particles    [%s]
	%s ms p.light atlas[%s]
	%s ms misc effects [%s]
	%s ms guis         [%s]
	%s ms post process [%s]
	%s ms misc         [%s]
	%s ms total        [%s]

===============================================================================
*/

class idImage;
class idCinematic;
enum textureUsage_t;
enum textureLayout_t;

enum renderParmType_e 
{
	PT_VECTOR,
	PT_TEXTURE,
	PT_ATTRIB,
	PT_MAX
};

class idDeclRenderParm : public idDecl {
public:

	typedef void (*evaluatorCallback_t)( const idDeclRenderParm * );

									idDeclRenderParm() :
										m_evaluator( NoOpEvaluator ) {
										memset( &m_defaults, 0, sizeof( m_defaults ) );
										m_infrequent = 0;
										memset( &m_data, 0, sizeof( m_data ) );
									}

	virtual							~idDeclRenderParm() {}

	// Override from idDecl
	virtual const char*				DefaultDefinition() const;
	virtual bool					Parse( const char* text, const int textLength, bool allowBinaryVersion = false );
	virtual size_t					Size() const { return sizeof( idDeclRenderParm ); }
	virtual void					FreeData();
	virtual void					Print() const;

	renderParmType_e				GetParmType() const { return m_type; }
	bool							IsTexture() const { return( GetParmType() == PT_TEXTURE ); }
	bool							IsVector() const { return( GetParmType() == PT_VECTOR ); }

	void							Set( const float f ) const;
	void							Set( const float f1, const float f2, const float f3, const float f4 ) const;
	void							Set( const float params[4] ) const;
	void							Set( const idVec3& v ) const;
	void							Set( const idVec4& v ) const;
	void							Set( idImage* image ) const;
	///void							Set( idCinematic* cinematic ) const;

	float *							GetDefaultVector() const { return const_cast< float* >( m_defaults.vector ); }
	idImage *						GetDefaultImage() const { return const_cast< idImage* >( m_defaults.texture.image ); }
	int								GetDefaultAttribIndex() const { return m_defaults.attrib; }
	textureUsage_t					GetDefaultTextureUsage() const { return m_defaults.texture.defaultUsage; } //SEA: ToFix!
	textureLayout_t					GetDefaultTextureLayout() const { return m_defaults.texture.defaultLayout; } //SEA: ToFix!

	float *							GetVecPtr() const { return m_data.vector; }
	idImage *						GetImage() const { return m_data.texture.image; }
	uint32							GetAttribIndex() const { return m_data.attrib; }

	idVec3 &						GetVec3() const { return *(idVec3 *)m_data.vector; }
	idVec4 &						GetVec4() const { return *(idVec4 *)m_data.vector; }
	textureUsage_t					GetTextureUsage() const { return m_data.texture.defaultUsage; }
	textureLayout_t					GetTextureLayout() const { return m_data.texture.defaultLayout; }

	int								Infrequent() const { return m_infrequent; }
	void							SetInfrequentIndex( const int index ) const { m_infrequent = index; }
	
	// Only for renderer use, the rest of the engine HANDS OFF THE GOODS!
	void							SetEvaluator( evaluatorCallback_t evaluator ) const { m_evaluator = evaluator; }
	void							ClearEvaluator() const { m_evaluator = NoOpEvaluator; }
	void							Evaluate() const;

private:
	bool							ParseVector( idLexer& src );
	bool							ParseTexture( idLexer& src );
	///bool							ParseAttrib( idLexer& src );

	static void						NoOpEvaluator( const idDeclRenderParm * ) {}

private:
	struct textureData_t {
		idImage *		image;
		///idCinematic *	cinematic;
		textureUsage_t	defaultUsage;
		textureLayout_t	defaultLayout;
	};

	ALIGNTYPE16 union renderParmData_t {
		float			vector[4];
		textureData_t	texture;
		uint32			attrib;
	};

	renderParmType_e				m_type = PT_VECTOR;
	renderParmData_t				m_defaults;
	mutable int						m_infrequent;
	mutable renderParmData_t		m_data;

	mutable evaluatorCallback_t		m_evaluator;

};


ID_INLINE void idDeclRenderParm::Set( const float f ) const
{
	m_data.vector[ 0 ] = m_data.vector[ 1 ] = m_data.vector[ 2 ] = m_data.vector[ 3 ] = f;
}

ID_INLINE void idDeclRenderParm::Set( const float f1, const float f2, const float f3, const float f4 ) const
{
	m_data.vector[ 0 ] = f1;
	m_data.vector[ 1 ] = f2;
	m_data.vector[ 2 ] = f3;
	m_data.vector[ 3 ] = f4;
}

ID_INLINE void idDeclRenderParm::Set( const float vector[4] ) const 
{
	m_data.vector[ 0 ] = vector[ 0 ];
	m_data.vector[ 1 ] = vector[ 1 ];
	m_data.vector[ 2 ] = vector[ 2 ];
	m_data.vector[ 3 ] = vector[ 3 ];
}

ID_INLINE void idDeclRenderParm::Set( const idVec3 &v ) const 
{
	m_data.vector[ 0 ] = v[ 0 ];
	m_data.vector[ 1 ] = v[ 1 ];
	m_data.vector[ 2 ] = v[ 2 ];
	m_data.vector[ 3 ] = 1.0f;
}

ID_INLINE void idDeclRenderParm::Set( const idVec4 &v ) const 
{
	m_data.vector[ 0 ] = v[ 0 ];
	m_data.vector[ 1 ] = v[ 1 ];
	m_data.vector[ 2 ] = v[ 2 ];
	m_data.vector[ 3 ] = v[ 3 ];
}

ID_INLINE void idDeclRenderParm::Set( idImage* image ) const 
{
	m_data.texture.image = image;
}

ID_INLINE void idDeclRenderParm::Evaluate() const 
{
	if ( m_evaluator != NoOpEvaluator ) {
		(*m_evaluator)( this );
	}
}

#endif /* !__DECLRENDERPARM_H__ */
