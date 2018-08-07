#ifndef __RENDERPARM_H__
#define __RENDERPARM_H__

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
	idVec4							value;
	const class idImage *			image;
	const class idSampler *			sampler;
	const class idDeclRenderProg *	program;
	idStr							string;
	int32							swizzle;

	bool operator== ( const parmValue_t& );
	bool operator!= ( const parmValue_t& );
};

enum expOpType_t {
	OP_TYPE_UNMASKED_MOVE_CONSTANT,
	OP_TYPE_UNMASKED_MOVE,
	OP_TYPE_MOVE_CONSTANT,
	OP_TYPE_MOVE,
	OP_TYPE_SWIZZLE,
	OP_TYPE_ADD,
	OP_TYPE_SUBTRACT,
	OP_TYPE_MULTIPLY,
	OP_TYPE_DOT3,
	OP_TYPE_DOT4,
	OP_TYPE_DIVIDE,
	OP_TYPE_MOD,
	OP_TYPE_GT,
	OP_TYPE_GE,
	OP_TYPE_LT,
	OP_TYPE_LE,
	OP_TYPE_EQ,
	OP_TYPE_NE,
	OP_TYPE_AND,
	OP_TYPE_OR,
	OP_TYPE_TABLE,
	OP_MAX
};

struct expOp_t {
	parmType_t	type;
	int16	parmIndexDest;
	int16	parmIndexA;
	int16	parmIndexB;

	Clear();
	SetOpType();
	SetConstantType();
	SetDestWriteMask();
	GetOpType();
	GetConstantType();
	GetDestWriteMask();
	const idDeclRenderParm *	GetDest();
	const idDeclRenderParm *	GetA();
	const idDeclRenderParm *	GetB();

	bool operator == ( const expOp_t& ) const;
	bool operator != ( const expOp_t& ) const;
};


class idParmBlock {
public:
	idParmBlock();
	~idParmBlock();

	bool operator = ( idParmBlock & );
	bool operator == ( idParmBlock & );

	void Clear();
	void CopyFrom( idParmBlock & );
	void Num();
	void GetOp();
	void GetConstant();
	void AddOp( expOp_t op, parmValue_t constant );
	void Append( idParmBlock block );
	void SetParm( idDeclRenderParm * parm, idDeclRenderParm * other );
	void SetParm( idDeclRenderParm * parm, parmValue_t parmValue );
	void SetFloat( idDeclRenderParm * parm, float );
	void SetVec2( idDeclRenderParm *, const idVec2 & );
	void SetVec3( idDeclRenderParm *, const idVec3 & );
	void SetVec4( idDeclRenderParm *, const idVec4 & );
	void SetFloat4( idDeclRenderParm *, float[ 4 ] );
	void SetImage( idDeclRenderParm *, idImage * );
	void SetSampler( idDeclRenderParm * );
	void SetProgram( idDeclRenderParm *, idDeclRenderProg * );
	void SetString( idDeclRenderParm *, const char * );
	void ClearParm( idDeclRenderParm * );
	void SetsRenderParm( idDeclRenderParm *parm );
	void RemoveRenderParmsNotNeededInProduction();
	void IsEqualSimple( const idParmBlock & other );
	float *				GetFloat( idDeclRenderParm * parm, idParmBlock & parentBloc );
	int *				GetInteger( idDeclRenderParm * parm, idParmBlock & parentBloc );
	idVec4 *			GetVector( idDeclRenderParm * parm, idParmBlock & parentBloc );
	idImage *			GetImage( idDeclRenderParm * parm, idParmBlock & parentBloc );
	class idSampler *	GetSampler( idDeclRenderParm * parm, idParmBlock & parentBloc );
	idDeclRenderProg *	GetProgram( idDeclRenderParm * parm, idParmBlock & parentBloc );
	const char *		GetString( idDeclRenderParm * parm, idParmBlock & parentBloc );
	void Print();
	void WriteString( idStr suffix );
	void Save( idFile * );
	void Load( idFile *num );
	void MapUpdate();
	void MapMakeDirty();

private:
	idList<expOp_t, TAG_RENDER>		ops;
	idList<parmValue_t, TAG_RENDER>	constants;

	void RemoveRedundantOperations( bool opIsRedundant );
	void MapRebuild();
	void GetRenderParmFromOps_r( parmValue_t parmValue, int numOps, int parmIndex, idParmBlock & parentBlock );
	void WriteConstant( idStr str, parmType_t parmType, parmValue_t parmValue );
	void WriteTerm( idStr str, idStr term );
};

template< uint Count >
class idStaticParmBlock {
	Clear();
	operator=();
	idStaticList<expOp_t, Count> staticOps;
	idStaticList<parmValue_t, Count> staticConstants;
};

////////////////////////////////////////////////////////////////////////////
// ParmState.cpp	w:\build\tech5\engine\renderer\jobs\render\ParmState.cpp 





//idParmState	renderThreadParmStateObject;


#endif /*__RENDERPARM_H__*/

