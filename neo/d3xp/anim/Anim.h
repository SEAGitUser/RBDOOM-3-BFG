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
#ifndef __ANIM_H__
#define __ANIM_H__

//
// animation channels
// these can be changed by modmakers and licensees to be whatever they need.
const int ANIM_MaxAnimsPerChannel	= 3;
const int ANIM_MaxSyncedAnims		= 3;

//
// animation channels.  make sure to change script/doom_defs.script if you add any channels, or change their order
//
enum animChannel_t 
{
	ANIMCHANNEL_ALL = 0,
	ANIMCHANNEL_TORSO,
	ANIMCHANNEL_LEGS,
	ANIMCHANNEL_HEAD,
	ANIMCHANNEL_EYELIDS,

	ANIM_NumAnimChannels,
};

// for converting from 24 frames per second to milliseconds
#define ANIM_FRAMERATE 24
ID_INLINE int FRAME2MS( int framenum ) {
	return idMath::FRAME2MS( framenum, ANIM_FRAMERATE );
}

class idRenderModel;
class idAnimator;
class idAnimBlend;
class function_t;
class idEntity;
class idSaveGame;
class idRestoreGame;

struct frameBlend_t {
	int		cycleCount;	// how many times the anim has wrapped to the begining (0 for clamped anims)
	int		frame1;
	int		frame2;
	float	frontlerp;
	float	backlerp;
};

struct jointAnimInfo_t {
	short					nameIndex;
	short					parentNum;
	short					animBits;
	short					firstComponent;
};

struct jointInfo_t {
	jointHandle_t			num;
	jointHandle_t			parentNum;
	animChannel_t			channel;
};

//
// joint modifier modes.  make sure to change script/doom_defs.script if you add any, or change their order.
//
enum jointModTransform_t {
	JOINTMOD_NONE,				// no modification
	JOINTMOD_LOCAL,				// modifies the joint's position or orientation in joint local space
	JOINTMOD_LOCAL_OVERRIDE,	// sets the joint's position or orientation in joint local space
	JOINTMOD_WORLD,				// modifies joint's position or orientation in model space
	JOINTMOD_WORLD_OVERRIDE		// sets the joint's position or orientation in model space
};

struct jointMod_t {
	idMat3					mat;
	idVec3					pos;
	jointHandle_t			jointnum;
	jointModTransform_t		transform_pos;
	jointModTransform_t		transform_axis;
};

#define	ANIM_TX				BIT( 0 )
#define	ANIM_TY				BIT( 1 )
#define	ANIM_TZ				BIT( 2 )
#define	ANIM_QX				BIT( 3 )
#define	ANIM_QY				BIT( 4 )
#define	ANIM_QZ				BIT( 5 )

struct frameLookup_t {
	int						num;
	int						firstCommand;
};

#include "Anim_FrameCommands.h"

struct animFlags_t {
	bool					prevent_idle_override : 1;
	bool					random_cycle_start : 1;
	bool					ai_no_turn : 1;
//	bool					ai_fixed_forward : 1;
	bool					anim_turn : 1;
//	bool					no_pitch : 1;
};

/*
==============================================================================================

	idMD5Anim

==============================================================================================
*/

class idMD5Anim {
public:
	idMD5Anim();
	~idMD5Anim();

	void					Free();
	bool					Reload();
	size_t					Allocated() const;
	size_t					Size() const { return sizeof( *this ) + Allocated(); };
	bool					LoadAnim( const char* filename );
	bool					LoadBinary( idFile* file, ID_TIME_T sourceTimeStamp );
	void					WriteBinary( idFile* file, ID_TIME_T sourceTimeStamp );

	void					IncreaseRefs() const { ref_count++; }
	void					DecreaseRefs() const { ref_count--; }
	int						NumRefs() const { return ref_count; }

	void					CheckModelHierarchy( const idRenderModel* model ) const;
	void					GetInterpolatedFrame( frameBlend_t& frame, idJointQuat* joints, const int* index, int numIndexes ) const;
	void					GetSingleFrame( int framenum, idJointQuat* joints, const int* index, int numIndexes ) const;
	int						Length() const { return animLength; }
	int						NumFrames() const { return numFrames; }
	int						NumJoints() const { return numJoints; }
	const idVec3&			TotalMovementDelta() const { return totaldelta; }
	const char*				Name() const { return name; }

	void					GetFrameBlend( int framenum, frameBlend_t& frame ) const;	// frame 1 is first frame
	void					ConvertTimeToFrame( int time, int cyclecount, frameBlend_t& frame ) const;

	void					GetOrigin( idVec3& offset, int currentTime, int cyclecount ) const;
	void					GetOriginRotation( idQuat& rotation, int time, int cyclecount ) const;
	void					GetBounds( idBounds& bounds, int currentTime, int cyclecount ) const;
	int						GetFrameRate() const { return frameRate; }

private:
	int						numFrames;
	int						frameRate;
	int						animLength;
	int						numJoints;
	int						numAnimatedComponents;
	idList<idBounds, TAG_MD5_ANIM>		bounds;
	idList<jointAnimInfo_t, TAG_MD5_ANIM>	jointInfo;
	idList<idJointQuat, TAG_MD5_ANIM>		baseFrame;
	idList<float, TAG_MD5_ANIM>			componentFrames;
	idStr					name;
	idVec3					totaldelta;
	mutable int				ref_count;

};

/*
==============================================================================================

	idAnim

==============================================================================================
*/

class idAnim {
public:
	idAnim();
	idAnim( const idDeclModelDef* modelDef, const idAnim* anim );
	~idAnim();

	void						SetAnim( const idDeclModelDef* modelDef, const char* sourcename, const char* animname, int num, const idMD5Anim* md5anims[ ANIM_MaxSyncedAnims ] );
	const char*					Name() const { return name; }
	const char*					FullName() const { return realname; }
	const idMD5Anim*			MD5Anim( int num ) const { return ( anims[ 0 ] == NULL )? NULL : anims[ num ]; }; // index 0 will never be NULL. Any anim >= NumAnims will return NULL.
	const idDeclModelDef*		ModelDef() const { return modelDef; }
	int							Length() const { return anims[ 0 ] ? anims[ 0 ]->Length() : 0; }
	int							NumFrames() const { return anims[ 0 ] ? anims[ 0 ]->NumFrames() : 0; }
	int							NumAnims() const { return numAnims; }
	const idVec3&				TotalMovementDelta() const { return anims[ 0 ] ? anims[ 0 ]->TotalMovementDelta() : vec3_zero; }
	bool						GetOrigin( idVec3& offset, int animNum, int time, int cyclecount ) const;
	bool						GetOriginRotation( idQuat& rotation, int animNum, int currentTime, int cyclecount ) const;
	bool						GetBounds( idBounds& bounds, int animNum, int time, int cyclecount ) const;
	const char*					AddFrameCommand( const class idDeclModelDef* modelDef, int framenum, idLexer& src, const idDict* def );
	void						CallFrameCommands( idEntity* ent, int from, int to ) const;
	bool						HasFrameCommands() const { return( frameCommands.Num() > 0 ); }
	void						ClearFrameCommands();

	// returns first frame (zero based) that command occurs.  returns -1 if not found.
	int							FindFrameForFrameCommand( frameCommandType_t framecommand, const idAnimFrameCommand ** command ) const;

	void						SetAnimFlags( const animFlags_t& animflags ) { flags = animflags; }
	const animFlags_t&			GetAnimFlags() const { return flags; }

private:
	const class idDeclModelDef*	modelDef;
	const idMD5Anim*			anims[ ANIM_MaxSyncedAnims ];
	int							numAnims;
	idStr						name;
	idStr						realname;
	idList<frameLookup_t, TAG_ANIM>	frameLookup;
	idList<idAnimFrameCommand, TAG_ANIM> frameCommands;
	animFlags_t					flags;
};

/*
==============================================================================================

	idDeclModelDef

==============================================================================================
*/

class idDeclModelDef : public idDecl {
public:
	idDeclModelDef();
	virtual ~idDeclModelDef();

	virtual size_t				Size() const;
	virtual const char* 		DefaultDefinition() const;
	virtual bool				Parse( const char* text, const int textLength, bool allowBinaryVersion );
	virtual void				FreeData();

	void						Touch() const;

	const idDeclSkin* 			GetDefaultSkin() const { return skin; }
	const idJointQuat* 			GetDefaultPose() const { return modelHandle->GetDefaultPose(); }
	void						SetupJoints( int* numJoints, idJointMat** jointList, idBounds& frameBounds, bool removeOriginOffset ) const;
	void						GetJointList( const char* jointnames, idList<jointHandle_t>& jointList ) const;
	const jointInfo_t* 			FindJoint( const char* name ) const;

	int							NumAnims() const { return( anims.Num() + 1 ); }
	const idAnim* 				GetAnim( int index ) const { return ( ( index < 1 ) || ( index > anims.Num() ) )? nullptr : anims[ index - 1 ]; }
	int							GetSpecificAnim( const char* name ) const;
	int							GetAnim( const char* name ) const;
	bool						HasAnim( const char* name ) const;
	const idDeclSkin* 			GetSkin() const { return skin; }
	const char* 				GetModelName() const { return modelHandle ? modelHandle->Name() : ""; }
	const idList<jointInfo_t>& 	Joints() const { return joints; }
	const int* 					JointParents() const { return jointParents.Ptr(); }
	int							NumJoints() const { return joints.Num(); }
	const jointInfo_t* 			GetJoint( int jointHandle ) const;
	const char* 				GetJointName( int jointHandle ) const;
	int							NumJointsOnChannel( animChannel_t channel ) const { assert( !( ( channel < 0 ) || ( channel >= ANIM_NumAnimChannels ) ) ); return channelJoints[ channel ].Num(); }
	const int* 					GetChannelJoints( animChannel_t channel ) const { assert( !( ( channel < 0 ) || ( channel >= ANIM_NumAnimChannels ) ) ); return channelJoints[ channel ].Ptr(); }
	int							NumChannels() const { return numChannels; }
	const idVec3 & 				GetVisualOffset() const { return offset; }
	idRenderModel * 			ModelHandle() const { return modelHandle; }

private:
	void						CopyDecl( const idDeclModelDef* decl );
	bool						ParseAnim( idLexer& src, int numDefaultAnims );

private:
	idVec3						offset;
	idList<jointInfo_t, TAG_ANIM> joints;
	idList<int, TAG_ANIM>		jointParents;
	idList<int, TAG_ANIM>		channelJoints[ ANIM_NumAnimChannels ];
	idRenderModel* 				modelHandle;
	idList<idAnim*, TAG_ANIM>	anims;
	const idDeclSkin* 			skin;
	int										numChannels; //SEA: added
};

/*
==============================================================================================

	idAnimBlend

==============================================================================================
*/

class idAnimBlend {
public:
	idAnimBlend();
	void						Save( idSaveGame* savefile ) const;
	void						Restore( idRestoreGame* savefile, const idDeclModelDef* modelDef );
	const char*					AnimName() const;
	const char*					AnimFullName() const;
	float						GetWeight( int currenttime ) const;
	float						GetFinalWeight() const { return blendEndValue; }
	void						SetWeight( float newweight, int currenttime, int blendtime );
	int							NumSyncedAnims() const;
	bool						SetSyncedAnimWeight( int num, float weight );
	void						Clear( int currentTime, int clearTime );
	bool						IsDone( int currentTime ) const;
	bool						FrameHasChanged( int currentTime ) const;
	int							GetCycleCount() const { return cycle; }
	void						SetCycleCount( int count );
	void						SetPlaybackRate( int currentTime, float newRate );
	float						GetPlaybackRate() const { return rate; }
	void						SetStartTime( int startTime );
	int							GetStartTime() const { return ( !animNum )? 0 : starttime; }
	int							GetEndTime() const { return ( !animNum )? 0 : endtime; }
	int							GetFrameNumber( int currenttime ) const;
	int							AnimTime( int currenttime ) const;
	int							NumFrames() const;
	int							Length() const;
	int							PlayLength() const { if( !animNum ) return 0; if( endtime < 0 ) return -1; return endtime - starttime + timeOffset; }
	void						AllowMovement( bool allow ) { allowMove = allow; }
	void						AllowFrameCommands( bool allow ) { allowFrameCommands = allow; }
	const idAnim*				Anim() const { return modelDef ? modelDef->GetAnim( animNum ) : nullptr; }
	int							AnimNum() const { return animNum; }

private:
	const class idDeclModelDef*	modelDef;
	int							starttime;
	int							endtime;
	int							timeOffset;
	float						rate;

	int							blendStartTime;
	int							blendDuration;
	float						blendStartValue;
	float						blendEndValue;

	float						animWeights[ ANIM_MaxSyncedAnims ];
	short						cycle;
	short						frame;
	short						animNum;
	bool						allowMove;
	bool						allowFrameCommands;

	friend class				idAnimator;

	void						Reset( const idDeclModelDef* _modelDef );
	void						CallFrameCommands( idEntity* ent, int fromtime, int totime ) const;
	void						SetFrame( const idDeclModelDef* modelDef, int animnum, int frame, int currenttime, int blendtime );
	void						CycleAnim( const idDeclModelDef* modelDef, int animnum, int currenttime, int blendtime );
	void						PlayAnim( const idDeclModelDef* modelDef, int animnum, int currenttime, int blendtime );
	bool						BlendAnim( int currentTime, animChannel_t channel, int numJoints, idJointQuat* blendFrame, float& blendWeight, bool removeOrigin, bool overrideBlend, bool printInfo ) const;
	void						BlendOrigin( int currentTime, idVec3& blendPos, float& blendWeight, bool removeOriginOffset ) const;
	void						BlendDelta( int fromtime, int totime, idVec3& blendDelta, float& blendWeight ) const;
	void						BlendDeltaRotation( int fromtime, int totime, idQuat& blendDelta, float& blendWeight ) const;
	bool						AddBounds( int currentTime, idBounds& bounds, bool removeOriginOffset, const bool ignoreLastFrame = false ) const;

};

/*
==============================================================================================

	idAFPoseJointMod

==============================================================================================
*/

enum AFJointModType_t {
	AF_JOINTMOD_AXIS,
	AF_JOINTMOD_ORIGIN,
	AF_JOINTMOD_BOTH
};

class idAFPoseJointMod {
public:
	ID_INLINE idAFPoseJointMod::idAFPoseJointMod() : mod( AF_JOINTMOD_AXIS )
	{
		axis.Identity();
		origin.Zero();
	}

	idMat3						axis;
	idVec3						origin;
	AFJointModType_t			mod;
};

/*
==============================================================================================

	idAnimator

==============================================================================================
*/

class idAnimator {
public:
	idAnimator();
	~idAnimator();

	size_t						Allocated() const;
	size_t						Size() const;

	void						Save( idSaveGame* savefile ) const;					// archives object for save game file
	void						Restore( idRestoreGame* savefile );					// unarchives object from save game file

	void						SetEntity( idEntity* ent ) { entity = ent; }
	idEntity*					GetEntity() const { return entity; }
	void						RemoveOriginOffset( bool remove ) { removeOriginOffset = remove; }
	bool						RemoveOrigin() const { return removeOriginOffset; }

	void						GetJointList( const char* jointnames, idList<jointHandle_t>& jointList ) const;

	int							NumAnims() const { return modelDef ? modelDef->NumAnims() : 0; }
	const idAnim*				GetAnim( int index ) const { return modelDef ? modelDef->GetAnim( index ) : nullptr; }
	int							GetAnim( const char* name ) const { return modelDef ? modelDef->GetAnim( name ) : 0; }
	bool						HasAnim( const char* name ) const { return modelDef ? modelDef->HasAnim( name ) : false; }

	void						ServiceAnims( int fromtime, int totime );
	bool						IsAnimating( int currentTime ) const;
	bool						IsAnimatingOnChannel( animChannel_t channelNum, int currentTime ) const;			//SEA:
	bool						IsPlayingAnim( animChannel_t channel, int animNum, int currentTime ) const;			// added
	bool						IsPlayingAnimPrimary( animChannel_t channel, int animNum, int currentTime ) const;	//
	bool						IsCyclingAnim( animChannel_t channel, int animNum, int currentTime ) const;			//

	void						GetJoints( int* numJoints, idJointMat** jointsPtr );
	int							NumJoints() const { return numJoints; }
	jointHandle_t				GetFirstChild( jointHandle_t jointnum ) const;
	jointHandle_t				GetFirstChild( const char* name ) const;

	idRenderModel*				SetModel( const char* modelname );
	idRenderModel*				ModelHandle() const { return modelDef ? modelDef->ModelHandle() : nullptr; }
	const idDeclModelDef*		ModelDef() const { return modelDef; }

	void						ForceUpdate();
	void						ClearForceUpdate();
	bool						CreateFrame( int animtime, bool force );
	bool						FrameHasChanged( int animtime ) const;
	void						GetDelta( int fromtime, int totime, idVec3& delta, const int maxChannels = ANIM_MaxAnimsPerChannel ) const;
	bool						GetDeltaRotation( int fromtime, int totime, idMat3& delta, const int maxChannels = ANIM_MaxAnimsPerChannel ) const;
	void						GetOrigin( int currentTime, idVec3& pos, const int maxChannels = ANIM_MaxAnimsPerChannel ) const;
	bool						GetBounds( int currentTime, idBounds& bounds, const bool force = false, const int maxChannels = ANIM_MaxAnimsPerChannel );

	// Gets the bounding box in joint space of the specified mesh, this is uncached and will cause reskinning so use sparingly
	///bool						GetMeshBounds( jointHandle_t jointnum, int meshHandle, int currentTime, idBounds& bounds, bool useDefaultAnim );

	idAnimBlend*				CurrentAnim( animChannel_t channelNum );
	void						Clear( animChannel_t channelNum, int currentTime, int cleartime );
	void						SetFrame( animChannel_t channelNum, int animnum, int frame, int currentTime, int blendtime );
	void						CycleAnim( animChannel_t channelNum, int animnum, int currentTime, int blendtime );
	void						PlayAnim( animChannel_t channelNum, int animnum, int currentTime, int blendTime );

	// copies the current anim from fromChannelNum to channelNum.
	// the copied anim will have frame commands disabled to avoid executing them twice.
	void						SyncAnimChannels( animChannel_t channelNum, animChannel_t fromChannelNum, int currentTime, int blendTime );

	void						SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const idVec3& pos );
	void						SetJointAxis( jointHandle_t jointnum, jointModTransform_t transform_type, const idMat3& mat );
	void						ClearJoint( jointHandle_t jointnum );
	void						ClearAllJoints();

	void						InitAFPose();
	void						SetAFPoseJointMod( const jointHandle_t jointNum, const AFJointModType_t mod, const idMat3& axis, const idVec3& origin );
	void						FinishAFPose( int animnum, const idBounds& bounds, const int time );
	void						SetAFPoseBlendWeight( float blendWeight );
	bool						BlendAFPose( idJointQuat* blendFrame ) const;
	void						ClearAFPose();

	void						ClearAllAnims( int currentTime, int cleartime );

	jointHandle_t				GetJointHandle( const char* name ) const;
	const char* 				GetJointName( jointHandle_t handle ) const;
	animChannel_t				GetChannelForJoint( jointHandle_t joint ) const;
	bool						GetJointTransform( jointHandle_t jointHandle, int currentTime, idVec3& offset, idMat3& axis );
	bool						GetJointTransform( jointHandle_t jointHandle, int currentTime, idVec3& offset );		//SEA:
	bool						GetJointTransform( jointHandle_t jointHandle, int currentTime, idMat3& axis );			// added
	bool						GetJointLocalTransform( jointHandle_t jointHandle, int currentTime, idVec3& offset, idMat3& axis );
	bool						GetJointLocalTransform( jointHandle_t jointHandle, int currentTime, idVec3& offset );	//
	bool						GetJointLocalTransform( jointHandle_t jointHandle, int currentTime, idMat3& axis );		//

	jointHandle_t				GetJointParent( jointHandle_t jointHandle ) const;

	const animFlags_t			GetAnimFlags( int animnum ) const;
	int							NumFrames( int animnum ) const;
	int							NumSyncedAnims( int animnum ) const;
	const char*					AnimName( int animnum ) const;
	const char*					AnimFullName( int animnum ) const;
	int							AnimLength( int animnum ) const;
	const idVec3&				TotalMovementDelta( int animnum ) const;

	int							GetLastTransformTime( void ) const { return lastTransformTime; }
	int							GetTransformCount( void ) const { return transformCount; }

private:
	void						FreeData();
	void						PushAnims( animChannel_t channel, int currentTime, int blendTime );

private:
	const idDeclModelDef* 		modelDef;
	idEntity* 					entity;

	idAnimBlend					channels[ ANIM_NumAnimChannels ][ ANIM_MaxAnimsPerChannel ];
	idList<jointMod_t*, TAG_ANIM> jointMods;
	int							numJoints;
	idJointMat* 				joints;

	mutable int					lastTransformTime;		// mutable because the value is updated in CreateFrame
	mutable int					transformCount; //SEA: added
	mutable bool				stoppedAnimatingUpdate;
	bool						removeOriginOffset;
	bool						forceUpdate;

	idBounds					frameBounds;

	float						AFPoseBlendWeight;
	idList<int, TAG_ANIM>		AFPoseJoints;
	idList<idAFPoseJointMod, TAG_ANIM> AFPoseJointMods;
	idList<idJointQuat, TAG_ANIM> AFPoseJointFrame;
	idBounds					AFPoseBounds;
	int							AFPoseTime;
};

/*
==============================================================================================

	idAnimManager

==============================================================================================
*/

class idAnimManager {
public:
	idAnimManager();
	~idAnimManager();

	static bool					forceExport;

	void						Shutdown();
	idMD5Anim* 					GetAnim( const char* name );
	void						Preload( const idPreloadManifest& );
	void						ReloadAnims();
	void						ListAnims() const;
	int							JointIndex( const char* name );
	const char* 				JointName( int index ) const;

	void						ClearAnimsInUse();
	void						FlushUnusedAnims();

private:
	idHashTable<idMD5Anim*>		animations;
	idStrList					jointnames;
	idHashIndex					jointnamesHash;
};

#endif /* !__ANIM_H__ */
