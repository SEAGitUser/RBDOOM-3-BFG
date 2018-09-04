
#ifndef __ANIM_FCMDS_H__
#define __ANIM_FCMDS_H__

extern idCVar g_debugFrameCommands;
extern idCVar g_debugFrameCommandsFilter;

enum frameCommandType_t 
{
	FC_SCRIPTFUNCTION = 0,
	FC_SCRIPTFUNCTIONOBJECT,
	FC_EVENTFUNCTION,
	FC_SOUND,
	FC_SOUND_VOICE,
	FC_SOUND_VOICE2,
	FC_SOUND_BODY,
	FC_SOUND_BODY2,
	FC_SOUND_BODY3,
	FC_SOUND_WEAPON,
	FC_SOUND_ITEM,
	FC_SOUND_GLOBAL,
	FC_SOUND_CHATTER,
	FC_SKIN,
	FC_TRIGGER,
	FC_TRIGGER_SMOKE_PARTICLE,
	FC_MELEE,
	FC_DIRECTDAMAGE,
	FC_BEGINATTACK,
	FC_ENDATTACK,
	FC_MUZZLEFLASH,
	FC_CREATEMISSILE,
	FC_LAUNCHMISSILE,
	FC_FIREMISSILEATTARGET,
	FC_FOOTSTEP,
	FC_LEFTFOOT,
	FC_RIGHTFOOT,
	FC_ENABLE_EYE_FOCUS,
	FC_DISABLE_EYE_FOCUS,
	FC_FX,
	FC_DISABLE_GRAVITY,
	FC_ENABLE_GRAVITY,
	FC_JUMP,
	FC_ENABLE_CLIP,
	FC_DISABLE_CLIP,
	FC_ENABLE_WALK_IK,
	FC_DISABLE_WALK_IK,
	FC_ENABLE_LEG_IK,
	FC_DISABLE_LEG_IK,
	FC_RECORDDEMO,
	FC_AVIGAME,
	FC_LAUNCH_PROJECTILE,
	FC_TRIGGER_FX,
	FC_START_EMITTER,
	FC_STOP_EMITTER,
};
static const char * frameCommandTypeNames[] = {
	"FC_SCRIPTFUNCTION",
	"FC_SCRIPTFUNCTIONOBJECT",
	"FC_EVENTFUNCTION",
	"FC_SOUND",
	"FC_SOUND_VOICE",
	"FC_SOUND_VOICE2",
	"FC_SOUND_BODY",
	"FC_SOUND_BODY2",
	"FC_SOUND_BODY3",
	"FC_SOUND_WEAPON",
	"FC_SOUND_ITEM",
	"FC_SOUND_GLOBAL",
	"FC_SOUND_CHATTER",
	"FC_SKIN",
	"FC_TRIGGER",
	"FC_TRIGGER_SMOKE_PARTICLE",
	"FC_MELEE",
	"FC_DIRECTDAMAGE",
	"FC_BEGINATTACK",
	"FC_ENDATTACK",
	"FC_MUZZLEFLASH",
	"FC_CREATEMISSILE",
	"FC_LAUNCHMISSILE",
	"FC_FIREMISSILEATTARGET",
	"FC_FOOTSTEP",
	"FC_LEFTFOOT",
	"FC_RIGHTFOOT",
	"FC_ENABLE_EYE_FOCUS",
	"FC_DISABLE_EYE_FOCUS",
	"FC_FX",
	"FC_DISABLE_GRAVITY",
	"FC_ENABLE_GRAVITY",
	"FC_JUMP",
	"FC_ENABLE_CLIP",
	"FC_DISABLE_CLIP",
	"FC_ENABLE_WALK_IK",
	"FC_DISABLE_WALK_IK",
	"FC_ENABLE_LEG_IK",
	"FC_DISABLE_LEG_IK",
	"FC_RECORDDEMO",
	"FC_AVIGAME",
	"FC_LAUNCH_PROJECTILE",
	"FC_TRIGGER_FX",
	"FC_START_EMITTER",
	"FC_STOP_EMITTER",
};

class idAnimFrameCommand {
private:
	frameCommandType_t			type;
	class idStr *				string; // could be whatever
	union {
		const idSoundShader *	soundShader;
		const function_t *		function;
		const idDeclSkin *		skin;
		int						index;
	};
public:
	ID_INLINE const char * GetTypeName() const { return frameCommandTypeNames[ type ]; }
	ID_INLINE frameCommandType_t GetType() const { return type; }

	// returns error str or NULL if success
	const char * Init( class idLexer & src, const class idAnim & );
	void Run( idEntity*, const idAnim &, int frame ) const;

	void Free();

	ID_INLINE const idStr *	GetString() const { assert( string != nullptr ); return string; }
	void Copy( const idAnimFrameCommand &src );
};



#endif /*__ANIM_FCMDS_H__*/
