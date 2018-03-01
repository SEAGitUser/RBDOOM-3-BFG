#ifndef __DECLRENDERPROG_H__
#define __DECLRENDERPROG_H__

//#include "tr_local.h"

/*
======================================================
======================================================
*/

enum progTarget_t {
	PT_PC,
	PT_PC_D3D,
	PT_360,
	PT_DURANGO,
	PT_PS3,
	PT_ORBIS,
	PT_NUM_TARGETS
};

/*
r_loadPrecompiledShaders
r_dumpGeneratedGLSL
r_amdShaderLinkCrashWorkaroundTest

	struct progValidation_t 
	{
		idTokenStatic<512> varType;
		idTokenStatic<512> varName;
		idTokenStatic<512> varSemantic;
		operator=();
	};


	struct shaderConstructInfo_t {
		Clear();
		bool	parsed;
		idStr	headerString;
		idStr	sourceString;
		idStr	outputAttribInitString;
		progValidation_t	inAttribs;
		progValidation_t	outAttribs;
		~shaderConstructInfo_t();
	};

	 RPF_PERFORATED,
	 RPF_SKY,
	 RPF_MODELFADE,
	renderProgFlags_t

	 PC_PARSED,
	 PC_CONSTANT,
	 PC_TEMPORARY,
	parmCreator_t

	 ATTRIB_SIZE_XYZ,
	 ATTRIB_SIZE_ST,
	 ATTRIB_SIZE_NORMAL,
	 ATTRIB_SIZE_COLOR,
	 ATTRIB_SIZE_TANGENT,
	 ATTRIB_SIZE_XYZ_SHORT,
	 ATTRIB_SIZE_ST_SHORT,
	attribSize_t

	 PE_NO_EDIT,
	 PE_BOOL,
	 PE_RANGE,
	 PE_COLOR,
	 PE_NO_DISPLAY,
	parmEdit_t

	enum materialProg_t {
		MPR_COVERAGE,
		MPR_INTERACTION,
		MPR_SHADOW,
		MPR_SSS,
		MPR_SSSMASK,
		MPR_ADD,
		MPR_BLEND,
		MPR_STAGE,
		MPR_FADE_PERTURB,
		MPR_MAX
	};

	DECALPROJ_PLANAR,
	DECALPROJ_SPHERICAL,
	decalProjType_t
	
*/

static const int32 PC_ATTRIB_INDEX_POSITION		= 0;
static const int32 PC_ATTRIB_INDEX_ST			= 1;
static const int32 PC_ATTRIB_INDEX_NORMAL		= 2;
static const int32 PC_ATTRIB_INDEX_TANGENT		= 3;
static const int32 PC_ATTRIB_INDEX_COLOR		= 4;
static const int32 PC_ATTRIB_INDEX_COLOR2		= 5;

/*
================================================
 vertexMask_e
 
 NOTE: There is a PS3 dependency between the bit flag specified here and the vertex
 attribute index and attribute semantic specified in DeclRenderProg.cpp because the
 stored render prog vertexMask is initialized with cellCgbGetVertexConfiguration().
 The ATTRIB_INDEX_ defines are used to make sure the vertexMask_e and attrib assignment
 in DeclRenderProg.cpp are in sync.
 
 Even though VERTEX_MASK_XYZ_SHORT and VERTEX_MASK_ST_SHORT are not real attributes,
 they come before the VERTEX_MASK_MORPH to reduce the range of vertex program
 permutations defined by the vertexMask_e bits on the Xbox 360 (see MAX_VERTEX_DECLARATIONS).
================================================
*/
enum vertexMask_e : int32 
{
	VERTEX_MASK_POSITION	= BIT( PC_ATTRIB_INDEX_POSITION ),
	VERTEX_MASK_ST			= BIT( PC_ATTRIB_INDEX_ST ),
	VERTEX_MASK_NORMAL		= BIT( PC_ATTRIB_INDEX_NORMAL ),
	VERTEX_MASK_TANGENT		= BIT( PC_ATTRIB_INDEX_TANGENT ),
	VERTEX_MASK_COLOR		= BIT( PC_ATTRIB_INDEX_COLOR ),
	VERTEX_MASK_COLOR2		= BIT( PC_ATTRIB_INDEX_COLOR2 ),
};
typedef idBitFlags<int32> vertexMask_t;

enum vertexAttributeIndex_e 
{
	VS_IN_POSITION,
	VS_IN_NORMAL,
	VS_IN_COLOR,
	VS_IN_COLOR2,
	VS_IN_ST,
	VS_IN_TANGENT,
	VS_IN_MAX
};

enum vertexFormat_t 
{
	VFMT_HALF_FLOAT,
	VFMT_FLOAT​,
	VFMT_DOUBLE,
	// INTS Integer types; these are converted to floats automatically. If normalized​ is GL_TRUE, then the value will be converted to a
	// float via integer normalization (an unsigned byte value of 255 becomes 1.0f). If normalized​ is GL_FALSE, it will be converted
	// directly to a float as if by C-style casting (255 becomes 255.0f, regardless of the size of the integer).
	VFMT_BYTE​,
	VFMT_UNSIGNED_BYTE​,
	VFMT_SHORT​,
	UNSIGNED_SHORT​,
	INT​,
	UNSIGNED_INT​,
	INT_2_10_10_10_REV​,
	UNSIGNED_INT_2_10_10_10_REV,
	UNSIGNED_INT_10F_11F_11F_REV,
	VFMT_MAX,
};

enum shaderType_e 
{
	SHT_VERTEX,
	SHT_TESS_CTRL,		// GL_TESS_CONTROL / DX_HULL
	SHT_TESS_EVAL,		// GL_TESS_EVALUATION / DX_DOMAIN
	SHT_GEOMETRY,
	SHT_FRAGMENT,
	SHT_MAX,
	SHT_COMPUTE = SHT_MAX,
};

enum autospriteType_t 
{
	AUTOSPRITE_NONE,
	AUTOSPRITE_VIEW_ORIENTED,
	AUTOSPRITE_LONGEST_AXIS_ALIGNED,
	AUTOSPRITE_X_AXIS_ALIGNED,
	AUTOSPRITE_Y_AXIS_ALIGNED,
	AUTOSPRITE_Z_AXIS_ALIGNED,
};

enum parmUsage_t
{
	PARM_USAGE_VERTEX,
	PARM_USAGE_FRAGMENT,
	PARM_USAGE_TEXTURE,
	PARM_USAGE_SAMPLER,
	PARM_USAGE_ALL,
};

struct renderProgData_t
{
	//int inheritsFrom;
	//int compoundCheckSum;
	//int parentCheckSum;
	//int dependencies;
	//int dependenciesTimestamp;
	uint64			glState;
	//int parmBlock;
	//int vertexParms;
	//int fragmentParms;
	//int textureParms;
	//int samplerParms;
	uint32			flags;
	//int stageSort;
	//bool			hasInteractions;
	//bool			hasAlphaToCoverage;
	bool			hasCoverageOutput;
	bool			hasStencilOutput;
	//bool			has16BitScaleBias;
	bool			hasHardwareSkinning;
	//bool			hasVertexTexture;
	bool			hasOptionalSkinning;
	bool			hasFragClip;
	bool			hasDepthOutput;
	//bool hasBranchStripping;
	vertexMask_t	vertexMask;
	//int fragmentOutputs;
	//int registerCountPS3;
	//int registerCount360;
	//int vertexCode;
	//int fragmentCode;

	void Clear() {
		memset( this, 0, sizeof( renderProgData_t ) );
	}
	void Read();
	void Write();
};

/*struct shaderConstructInfo_t {
	void Clear();
	bool parsed;
	idStr headerString;
	idStr sourceString;
	idStr outputAttribInitString;
	int inAttribs;
	int outAttribs;
};*/

typedef idList< const idDeclRenderParm *, TAG_RENDERPROG> progParmList_t;

/*class idParmBlock {
public:
	parmList_t parms;
	const float * values;
	uint32 size;
};*/


/*
======================================================
	idDeclRenderProg
======================================================
*/
class idDeclRenderProg : public idDecl {
public:

	idDeclRenderProg();
	virtual	~idDeclRenderProg();

	// Override from idDecl
	virtual const char *			DefaultDefinition() const final;
	virtual bool					Parse( const char* text, const int textLength, bool allowBinaryVersion = false ) final;
	virtual size_t					Size() const final;
	virtual void					FreeData() final;

	virtual void					Print() const;
	virtual void					List() const;

	void							Clear();

	//void ReloadIfStale();
	//void GetRenamed();
	//void SetRenamed();
	
	bool				HasFlag( int flag ) const { return(( data.flags & flag ) != 0); }
	//bool				Has16BitScaleBias() const { return data.has16BitScaleBias; }
	bool				HasHardwareSkinning() const { return data.hasHardwareSkinning; }
	bool				HasOptionalSkinning() const { return data.hasOptionalSkinning; }
	bool				HasClipFragments() const { return data.hasFragClip; }
	bool				WritesFragmentDepth() const { return data.hasDepthOutput; }
	bool				ProgUsesParm( const idDeclRenderParm * parm ) const;
	uint64				GetGLState() const { return data.glState; }
	//void GetParmBlock();
	//void GetStageSort();
	const vertexMask_t & GetVertexMask() const { return data.vertexMask; }

	//void GetRenderProgData( renderProgData_t, renderProgParms_t, shaderConstructInfo_t );
	//void VersionForLight();
	//void BindForImmediate();
	//void SetTextureParm();
	//void BindTexture( int texUnit, int image, int sampler );
	//void ResetProgramState();
	//void ResetTextureState();
	//void Disable();
	//void Enable();
	//void GetGlslProgram();
	//void Build360();
	//void BuildDurango();
	//void BuildPCD3D();
	//void BuildPS3();
	//void BuildOrbis();
	//void OpenFileReadWithOverride();
	//void IsInOverrideMode();

	void *				GetAPIObject() const { return apiObject; }
	bool				IsValidAPIObject() const { return( !!apiObject ); }

	const progParmList_t &	GetVecParms() const { return vecParms; }
	const progParmList_t &	GetTexParms() const { return texParms; }

	const idUniformBuffer &	GetParmsUBO() const { return ubo; }

private:

	void				SetFlag( const int flag ) { data.flags |= flag; }
	void				ClearFlag( const int flag ) { data.flags &= ~flag; }
	
	//bool				renamed;
	
	//int					inheritsFromDecl;
	//uint32				shaderChecksum[ SHT_MAX ];
	//bool				hasDerived;
	//bool				partiallyLoaded;
	//int					versions; // idArray < const idDeclRenderProg * , 3 >    [7] versions
	
	///void UploadTargetCode( int glslBinaryFilename, int path, int fBin, int fileData, int linked, int infologLength );

	//uint16				vertexProgram;		// shaderConstructInfo_t
	//uint16				fragmentProgram;	// shaderConstructInfo_t

	//uint16				vertexUniforms;		// renderProgParms_t
	//uint16				fragmentUniforms;	// renderProgParms_t

private:

	renderProgData_t	data;
	progParmList_t		vecParms;
	progParmList_t		texParms;
	void *				apiObject;
	idUniformBuffer		ubo;
};

#endif /*__DECLRENDERPROG_H__*/