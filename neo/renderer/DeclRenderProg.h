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
	PT_PC_VK,
	//PT_360,
	//PT_DURANGO,
	//PT_PS3,
	//PT_ORBIS,
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

/*enum vertexAttributeIndex_e
{
	VS_IN_POSITION,
	VS_IN_NORMAL,
	VS_IN_COLOR,
	VS_IN_COLOR2,
	VS_IN_ST,
	VS_IN_TANGENT,
	VS_IN_MAX
};*/

/*enum vertexFormat_t
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
};*/

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

/*enum autospriteType_t
{
	AUTOSPRITE_NONE,
	AUTOSPRITE_VIEW_ORIENTED,
	AUTOSPRITE_LONGEST_AXIS_ALIGNED,
	AUTOSPRITE_X_AXIS_ALIGNED,
	AUTOSPRITE_Y_AXIS_ALIGNED,
	AUTOSPRITE_Z_AXIS_ALIGNED,
};*/

/*enum parmUsage_t
{
	PARM_USAGE_VERTEX,
	PARM_USAGE_FRAGMENT,
	PARM_USAGE_TEXTURE,
	PARM_USAGE_SAMPLER,
	PARM_USAGE_ALL,
};*/

typedef idList< const idDeclRenderParm *, TAG_RENDERPROG> progParmList_t;

enum renderProgFlags_t
{
	//hasInteractions				= BIT( 1 ),
	//hasAlphaToCoverage			= BIT( 2 ),
	HAS_COVERAGE_OUTPUT				= BIT( 3 ),
	HAS_STENCIL_OUTPUT				= BIT( 4 ),
	//has16BitScaleBias				= BIT( 5 ),
	HAS_HARDWARE_SKINNING			= BIT( 6 ),
	//hasVertexTexture				= BIT( 7 ),
	HAS_OPTIONAL_SKINNING			= BIT( 8 ),
	HAS_FRAG_OUTPUT					= BIT( 9 ),
	HAS_MRT_OUTPUT					= BIT( 10 ),
	HAS_FRAG_CLIP					= BIT( 11 ),
	HAS_DEPTH_OUTPUT				= BIT( 12 ),
	HAS_FORCED_EARLY_FRAG_TESTS		= BIT( 13 ),
	//hasBranchStripping			= BIT( 14 ),
};

struct renderProgData_t
{
	//int inheritsFrom;
	//int compoundCheckSum;
	//int parentCheckSum;
	idStrList dependencies;
	ID_TIME_T dependenciesTimestamp;
	//int parmBlock;
	//int vertexParms;
	//int fragmentParms;
	//int textureParms;
	//int samplerParms;
	progParmList_t		vecParms;
	progParmList_t		texParms;

	idBitFlags<uint64>	glState;
	idBitFlags<uint32>	flags;
	//int stageSort;

	vertexMask_t	vertexMask;
	//int fragmentOutputs;
	//int registerCountPS3;
	//int registerCount360;
	//int vertexCode;
	//int fragmentCode;

	void Clear() {
		dependencies.Clear();
		dependenciesTimestamp = 0;
		vecParms.Clear();
		texParms.Clear();
		glState.Clear();
		flags.Clear();
		vertexMask.Clear();
	}
	size_t Size() const {
		size_t size = 0;
		size += dependencies.Size();
		size += sizeof( dependenciesTimestamp );
		size += vecParms.Size();
		size += texParms.Size();
		size += glState.Size();
		size += flags.Size();
		size += vertexMask.Size();
		return size;
	}
	void operator = ( const renderProgData_t & other )
	{
		dependencies = other.dependencies;
		dependenciesTimestamp = other.dependenciesTimestamp;

		vecParms = other.vecParms;
		texParms = other.texParms;

		glState = other.glState;
		flags = other.flags;
		vertexMask = other.vertexMask;
	}

	void Read( idFile * );
	void Write( idFile * );
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
	virtual const char *	DefaultDefinition() const final;
	virtual bool			Parse( const char* text, const int textLength, bool allowBinaryVersion = false ) final;
	virtual size_t			Size() const final;
	virtual void			FreeData() final;

	virtual bool			SourceFileChanged() const;

	virtual void			Print() const;
	virtual void			List() const;

	void					Clear();

	//void ReloadIfStale();
	//void GetRenamed();
	//void SetRenamed();

	bool					HasFlag( renderProgFlags_t flag ) const { return data.flags.HasFlag( flag ); }
	//bool					Has16BitScaleBias() const { return data.has16BitScaleBias; }
	// the joints buffer should only be bound for vertex programs that use joints
	bool					HasHardwareSkinning() const { return HasFlag( HAS_HARDWARE_SKINNING ); }
	// the rpEnableSkinning render parm should only be set for vertex programs that use it
	bool					HasOptionalSkinning() const { return HasFlag( HAS_OPTIONAL_SKINNING ); }
	bool					HasClipFragments() const { return HasFlag( HAS_FRAG_CLIP ); }
	bool					HasDepthOutput() const { return HasFlag( HAS_DEPTH_OUTPUT ); }
	bool					HasFragOutput() const { return HasFlag( HAS_FRAG_OUTPUT ); }
	bool					HasMRT() const { return HasFlag( HAS_MRT_OUTPUT ); }

	auto					GetGLState() const { return data.glState; }
	auto					GetVertexMask() const { return data.vertexMask; }

	bool					ProgUsesParm( const idDeclRenderParm * parm ) const;
	ID_INLINE const progParmList_t & GetVecParms() const { return data.vecParms; }
	ID_INLINE const progParmList_t & GetTexParms() const { return data.texParms; }

	//void GetParmBlock();
	//void GetStageSort();

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

	void *					GetAPIObject() const { return apiObject; }
	bool					IsValidAPIObject() const { return( apiObject != nullptr ); }

	//const idUniformBuffer &	GetParmsUBO() const { return ubo; }

private:

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
	renderProgData_t		data;
	void *					apiObject;
	//idUniformBuffer		ubo;
};

#endif /*__DECLRENDERPROG_H__*/