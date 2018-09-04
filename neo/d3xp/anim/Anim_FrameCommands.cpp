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

#pragma hdrstop
#include "precompiled.h"

#include "../Game_local.h"

idCVar g_debugFrameCommands( "g_debugFrameCommands", "0", CVAR_GAME | CVAR_BOOL | CVAR_CHEAT, "Prints out frame commands as they are called" );
idCVar g_debugFrameCommandsFilter( "g_debugFrameCommandsFilter", "", CVAR_GAME | CVAR_CHEAT, "Filter the type of framecommands" );

/*
========================================
idAnimFrameCommand::Copy
========================================
*/
void idAnimFrameCommand::Copy( const idAnimFrameCommand &src )
{
	*this = src;
	if( src.string )
	{
		string = new( TAG_ANIM ) idStr( *src.string );
	}
}

/*
========================================
idAnimFrameCommand::Free
========================================
*/
void idAnimFrameCommand::Free()
{
	delete string;
	string = nullptr;
}

/*
========================================
 idAnimFrameCommand::Init
========================================
*/
const char * idAnimFrameCommand::Init( idLexer & src, const idAnim & anim )
{
	idStr text;
	idStr funcname;

	idToken token;
	if( !src.ReadTokenOnLine( &token ) )
	{
		return "Unexpected end of line";
	}

	if( token == "call" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_SCRIPTFUNCTION;
		this->function = gameLocal.program.FindFunction( token );
		if( !this->function )
		{
			return va( "Function '%s' not found", token.c_str() );
		}
	}
	else if( token == "object_call" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_SCRIPTFUNCTIONOBJECT;
		this->string = new( TAG_ANIM ) idStr( token );
	}
	else if( token == "event" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_EVENTFUNCTION;
		const idEventDef* ev = idEventDef::FindEvent( token );
		if( !ev )
		{
			return va( "Event '%s' not found", token.c_str() );
		}
		if( ev->GetNumArgs() != 0 )
		{
			return va( "Event '%s' has arguments", token.c_str() );
		}
		this->string = new( TAG_ANIM ) idStr( token );
	}
	else if( token == "sound" )
	{
		if( token.Cmpn( "sound", 5 ) )
		{
			if( token.Find( "_voice", false, 5 ) )
			{
				SND_CHANNEL_VOICE;
			}
			else if( token.Find( "_voice2", false, 5 ) )
			{
				SND_CHANNEL_VOICE2;
			}
			else if( token.Find( "_body", false, 5 ) )
			{
				SND_CHANNEL_BODY;
			}
			else if( token.Find( "_body2", false, 5 ) )
			{
				SND_CHANNEL_BODY2;
			}
			else if( token.Find( "_body3", false, 5 ) )
			{
				SND_CHANNEL_BODY3;
			}
			else if( token.Find( "_weapon", false, 5 ) )
			{
				SND_CHANNEL_WEAPON;
			}
			else if( token.Find( "_item", false, 5 ) )
			{
				SND_CHANNEL_ITEM;
			}
			else if( token.Find( "_global", false, 5 ) )
			{
				SND_CHANNEL_ANY;
			}
			else if( token.Find( "_chatter", false, 5 ) )
			{
				SND_CHANNEL_VOICE;
			}

			SND_CHANNEL_ANY;
		}




		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_SOUND;
		if( !token.Cmpn( "snd_", 4 ) )
		{
			this->string = new( TAG_ANIM ) idStr( token );
		}
		else
		{
			this->soundShader = declManager->FindSound( token );
			if( this->soundShader->GetState() == DS_DEFAULTED )
			{
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	}
	else if( token == "sound_voice" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_SOUND_VOICE;
		if( !token.Cmpn( "snd_", 4 ) )
		{
			this->string = new( TAG_ANIM ) idStr( token );
		}
		else
		{
			this->soundShader = declManager->FindSound( token );
			if( this->soundShader->GetState() == DS_DEFAULTED )
			{
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	}
	else if( token == "sound_voice2" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_SOUND_VOICE2;
		if( !token.Cmpn( "snd_", 4 ) )
		{
			this->string = new( TAG_ANIM ) idStr( token );
		}
		else
		{
			this->soundShader = declManager->FindSound( token );
			if( this->soundShader->GetState() == DS_DEFAULTED )
			{
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	}
	else if( token == "sound_body" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_SOUND_BODY;
		if( !token.Cmpn( "snd_", 4 ) )
		{
			this->string = new( TAG_ANIM ) idStr( token );
		}
		else
		{
			this->soundShader = declManager->FindSound( token );
			if( this->soundShader->GetState() == DS_DEFAULTED )
			{
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	}
	else if( token == "sound_body2" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_SOUND_BODY2;
		if( !token.Cmpn( "snd_", 4 ) )
		{
			this->string = new( TAG_ANIM ) idStr( token );
		}
		else
		{
			this->soundShader = declManager->FindSound( token );
			if( this->soundShader->GetState() == DS_DEFAULTED )
			{
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	}
	else if( token == "sound_body3" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_SOUND_BODY3;
		if( !token.Cmpn( "snd_", 4 ) )
		{
			this->string = new( TAG_ANIM ) idStr( token );
		}
		else
		{
			this->soundShader = declManager->FindSound( token );
			if( this->soundShader->GetState() == DS_DEFAULTED )
			{
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	}
	else if( token == "sound_weapon" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_SOUND_WEAPON;
		if( !token.Cmpn( "snd_", 4 ) )
		{
			this->string = new( TAG_ANIM ) idStr( token );
		}
		else
		{
			this->soundShader = declManager->FindSound( token );
			if( this->soundShader->GetState() == DS_DEFAULTED )
			{
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	}
	else if( token == "sound_global" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_SOUND_GLOBAL;
		if( !token.Cmpn( "snd_", 4 ) )
		{
			this->string = new( TAG_ANIM ) idStr( token );
		}
		else
		{
			this->soundShader = declManager->FindSound( token );
			if( this->soundShader->GetState() == DS_DEFAULTED )
			{
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	}
	else if( token == "sound_item" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_SOUND_ITEM;
		if( !token.Cmpn( "snd_", 4 ) )
		{
			this->string = new( TAG_ANIM ) idStr( token );
		}
		else
		{
			this->soundShader = declManager->FindSound( token );
			if( this->soundShader->GetState() == DS_DEFAULTED )
			{
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	}
	else if( token == "sound_chatter" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_SOUND_CHATTER;
		if( !token.Cmpn( "snd_", 4 ) )
		{
			this->string = new( TAG_ANIM ) idStr( token );
		}
		else
		{
			this->soundShader = declManager->FindSound( token );
			if( this->soundShader->GetState() == DS_DEFAULTED )
			{
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	}
	else if( token == "skin" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_SKIN;
		if( token == "none" )
		{
			this->skin = NULL;
		}
		else
		{
			this->skin = declManager->FindSkin( token );
			if( !this->skin )
			{
				return va( "Skin '%s' not found", token.c_str() );
			}
		}
	}
	else if( token == "fx" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_FX;
		if( !declManager->FindType( DECL_FX, token.c_str() ) )
		{
			return va( "fx '%s' not found", token.c_str() );
		}
		this->string = new( TAG_ANIM ) idStr( token );
	}
	else if( token == "trigger" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_TRIGGER;
		this->string = new( TAG_ANIM ) idStr( token );
	}
	else if( token == "triggerSmokeParticle" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_TRIGGER_SMOKE_PARTICLE;
		if( !declManager->FindType( DECL_PARTICLE, token.c_str() ) )
		{
			return va( "Particle '%s' not found", token.c_str() );
		}
		this->string = new( TAG_ANIM ) idStr( token );
	}
	else if( token == "melee" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_MELEE;
		if( !gameLocal.FindEntityDef( token.c_str(), false ) )
		{
			return va( "Unknown entityDef '%s'", token.c_str() );
		}
		this->string = new( TAG_ANIM ) idStr( token );
	}
	else if( token == "direct_damage" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_DIRECTDAMAGE;
		if( !gameLocal.FindEntityDef( token.c_str(), false ) )
		{
			return va( "Unknown entityDef '%s'", token.c_str() );
		}
		this->string = new( TAG_ANIM ) idStr( token );
	}
	else if( token == "attack_begin" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_BEGINATTACK;
		if( !gameLocal.FindEntityDef( token.c_str(), false ) )
		{
			return va( "Unknown entityDef '%s'", token.c_str() );
		}
		this->string = new( TAG_ANIM ) idStr( token );
	}
	else if( token == "attack_end" )
	{
		this->type = FC_ENDATTACK;
	}
	else if( token == "muzzle_flash" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		if( ( token != "" ) && !anim.ModelDef()->FindJoint( token ) )
		{
			return va( "Joint '%s' not found", token.c_str() );
		}
		this->type = FC_MUZZLEFLASH;
		this->string = new( TAG_ANIM ) idStr( token );
	}
	else if( token == "create_missile" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		if( !anim.ModelDef()->FindJoint( token ) )
		{
			return va( "Joint '%s' not found", token.c_str() );
		}
		this->type = FC_CREATEMISSILE;
		this->string = new( TAG_ANIM ) idStr( token );
	}
	else if( token == "launch_missile" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		if( !anim.ModelDef()->FindJoint( token ) )
		{
			return va( "Joint '%s' not found", token.c_str() );
		}
		this->type = FC_LAUNCHMISSILE;
		this->string = new( TAG_ANIM ) idStr( token );
	}
	else if( token == "fire_missile_at_target" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		auto jointInfo = anim.ModelDef()->FindJoint( token );
		if( !jointInfo )
		{
			return va( "Joint '%s' not found", token.c_str() );
		}
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_FIREMISSILEATTARGET;
		this->string = new( TAG_ANIM ) idStr( token );
		this->index = jointInfo->num;
	}
	else if( token == "launch_projectile" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		if( !declManager->FindDeclWithoutParsing( DECL_ENTITYDEF, token, false ) )
		{
			return "Unknown projectile def";
		}
		this->type = FC_LAUNCH_PROJECTILE;
		this->string = new( TAG_ANIM ) idStr( token );
	}
	else if( token == "trigger_fx" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		auto jointInfo = anim.ModelDef()->FindJoint( token );
		if( !jointInfo )
		{
			return va( "Joint '%s' not found", token.c_str() );
		}
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		if( !declManager->FindType( DECL_FX, token, false ) )
		{
			return "Unknown FX def";
		}

		this->type = FC_TRIGGER_FX;
		this->string = new( TAG_ANIM ) idStr( token );
		this->index = jointInfo->num;
	}
	else if( token == "start_emitter" )
	{
		idStr str;
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		str = token + " ";

		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		auto jointInfo = anim.ModelDef()->FindJoint( token );
		if( !jointInfo )
		{
			return va( "Joint '%s' not found", token.c_str() );
		}
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		if( !declManager->FindType( DECL_PARTICLE, token.c_str() ) )
		{
			return va( "Particle '%s' not found", token.c_str() );
		}
		str += token;
		this->type = FC_START_EMITTER;
		this->string = new( TAG_ANIM ) idStr( str );
		this->index = jointInfo->num;
	}
	else if( token == "stop_emitter" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_STOP_EMITTER;
		this->string = new( TAG_ANIM ) idStr( token );
	}
	else if( token == "footstep" )
	{
		this->type = FC_FOOTSTEP;
	}
	else if( token == "leftfoot" )
	{
		this->type = FC_LEFTFOOT;
	}
	else if( token == "rightfoot" )
	{
		this->type = FC_RIGHTFOOT;
	}
	else if( token == "enableEyeFocus" )
	{
		this->type = FC_ENABLE_EYE_FOCUS;
	}
	else if( token == "disableEyeFocus" )
	{
		this->type = FC_DISABLE_EYE_FOCUS;
	}
	else if( token == "disableGravity" )
	{
		this->type = FC_DISABLE_GRAVITY;
	}
	else if( token == "enableGravity" )
	{
		this->type = FC_ENABLE_GRAVITY;
	}
	else if( token == "jump" )
	{
		this->type = FC_JUMP;
	}
	else if( token == "enableClip" )
	{
		this->type = FC_ENABLE_CLIP;
	}
	else if( token == "disableClip" )
	{
		this->type = FC_DISABLE_CLIP;
	}
	else if( token == "enableWalkIK" )
	{
		this->type = FC_ENABLE_WALK_IK;
	}
	else if( token == "disableWalkIK" )
	{
		this->type = FC_DISABLE_WALK_IK;
	}
	else if( token == "enableLegIK" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_ENABLE_LEG_IK;
		this->index = atoi( token );
	}
	else if( token == "disableLegIK" )
	{
		if( !src.ReadTokenOnLine( &token ) )
		{
			return "Unexpected end of line";
		}
		this->type = FC_DISABLE_LEG_IK;
		this->index = atoi( token );
	}
	else if( token == "recordDemo" )
	{
		this->type = FC_RECORDDEMO;
		if( src.ReadTokenOnLine( &token ) )
		{
			this->string = new( TAG_ANIM ) idStr( token );
		}
	}
	else if( token == "aviGame" )
	{
		this->type = FC_AVIGAME;
		if( src.ReadTokenOnLine( &token ) )
		{
			this->string = new( TAG_ANIM ) idStr( token );
		}
	}
	else
	{
		return va( "Unknown command '%s'", token.c_str() );
	}

	return NULL;
}

/*
========================================
 idAnimFrameCommand::Run
========================================
*/
void idAnimFrameCommand::Run( idEntity* ent, const idAnim & anim, int frame ) const
{
	switch( this->type )
	{
		case FC_SCRIPTFUNCTION:
		{
			gameLocal.CallFrameCommand( ent, this->function );
			break;
		}
		case FC_SCRIPTFUNCTIONOBJECT:
		{
			gameLocal.CallObjectFrameCommand( ent, this->string->c_str() );
			break;
		}
		case FC_EVENTFUNCTION:
		{
			const idEventDef* ev = idEventDef::FindEvent( this->string->c_str() );
			ent->ProcessEvent( ev );
			break;
		}
		case FC_SOUND:
		{
			if( !this->soundShader )
			{
				if( !ent->StartSound( this->string->c_str(), SND_CHANNEL_ANY, 0, false, NULL ) )
				{
					gameLocal.Warning( "FrameCmd 'sound' on entity '%s', anim '%s', frame %d: Could not find sound '%s'",
									   ent->name.c_str(), anim.FullName(), frame + 1, this->string->c_str() );
				}
			}
			else
			{
				ent->StartSoundShader( this->soundShader, SND_CHANNEL_ANY, 0, false, NULL );
			}
			break;
		}
		case FC_SOUND_VOICE:
		{
			if( !this->soundShader )
			{
				if( !ent->StartSound( this->string->c_str(), SND_CHANNEL_VOICE, 0, false, NULL ) )
				{
					gameLocal.Warning( "FrameCmd 'sound_voice' on entity '%s', anim '%s', frame %d: Could not find sound '%s'",
									   ent->name.c_str(), anim.FullName(), frame + 1, this->string->c_str() );
				}
			}
			else
			{
				ent->StartSoundShader( this->soundShader, SND_CHANNEL_VOICE, 0, false, NULL );
			}
			break;
		}
		case FC_SOUND_VOICE2:
		{
			if( !this->soundShader )
			{
				if( !ent->StartSound( this->string->c_str(), SND_CHANNEL_VOICE2, 0, false, NULL ) )
				{
					gameLocal.Warning( "FrameCmd 'sound_voice2' on entity '%s', anim '%s', frame %d: Could not find sound '%s'",
									   ent->name.c_str(), anim.FullName(), frame + 1, this->string->c_str() );
				}
			}
			else
			{
				ent->StartSoundShader( this->soundShader, SND_CHANNEL_VOICE2, 0, false, NULL );
			}
			break;
		}
		case FC_SOUND_BODY:
		{
			if( !this->soundShader )
			{
				if( !ent->StartSound( this->string->c_str(), SND_CHANNEL_BODY, 0, false, NULL ) )
				{
					gameLocal.Warning( "FrameCmd 'sound_body' on entity '%s', anim '%s', frame %d: Could not find sound '%s'",
									   ent->name.c_str(), anim.FullName(), frame + 1, this->string->c_str() );
				}
			}
			else
			{
				ent->StartSoundShader( this->soundShader, SND_CHANNEL_BODY, 0, false, NULL );
			}
			break;
		}
		case FC_SOUND_BODY2:
		{
			if( !this->soundShader )
			{
				if( !ent->StartSound( this->string->c_str(), SND_CHANNEL_BODY2, 0, false, NULL ) )
				{
					gameLocal.Warning( "FrameCmd 'sound_body2' on entity '%s', anim '%s', frame %d: Could not find sound '%s'",
									   ent->name.c_str(), anim.FullName(), frame + 1, this->string->c_str() );
				}
			}
			else
			{
				ent->StartSoundShader( this->soundShader, SND_CHANNEL_BODY2, 0, false, NULL );
			}
			break;
		}
		case FC_SOUND_BODY3:
		{
			if( !this->soundShader )
			{
				if( !ent->StartSound( this->string->c_str(), SND_CHANNEL_BODY3, 0, false, NULL ) )
				{
					gameLocal.Warning( "FrameCmd 'sound_body3' on entity '%s', anim '%s', frame %d: Could not find sound '%s'",
									   ent->name.c_str(), anim.FullName(), frame + 1, this->string->c_str() );
				}
			}
			else
			{
				ent->StartSoundShader( this->soundShader, SND_CHANNEL_BODY3, 0, false, NULL );
			}
			break;
		}
		case FC_SOUND_WEAPON:
		{
			if( !this->soundShader )
			{
				if( !ent->StartSound( this->string->c_str(), SND_CHANNEL_WEAPON, 0, false, NULL ) )
				{
					gameLocal.Warning( "FrameCmd 'sound_weapon' on entity '%s', anim '%s', frame %d: Could not find sound '%s'",
									   ent->name.c_str(), anim.FullName(), frame + 1, this->string->c_str() );
				}
			}
			else
			{
				ent->StartSoundShader( this->soundShader, SND_CHANNEL_WEAPON, 0, false, NULL );
			}
			break;
		}
		case FC_SOUND_GLOBAL:
		{
			if( !this->soundShader )
			{
				if( !ent->StartSound( this->string->c_str(), SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL ) )
				{
					gameLocal.Warning( "FrameCmd 'sound_global' on entity '%s', anim '%s', frame %d: Could not find sound '%s'",
									   ent->name.c_str(), anim.FullName(), frame + 1, this->string->c_str() );
				}
			}
			else
			{
				ent->StartSoundShader( this->soundShader, SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL );
			}
			break;
		}
		case FC_SOUND_ITEM:
		{
			if( !this->soundShader )
			{
				if( !ent->StartSound( this->string->c_str(), SND_CHANNEL_ITEM, 0, false, NULL ) )
				{
					gameLocal.Warning( "FrameCmd 'sound_item' on entity '%s', anim '%s', frame %d: Could not find sound '%s'",
									   ent->name.c_str(), anim.FullName(), frame + 1, this->string->c_str() );
				}
			}
			else
			{
				ent->StartSoundShader( this->soundShader, SND_CHANNEL_ITEM, 0, false, NULL );
			}
			break;
		}
		case FC_SOUND_CHATTER:
		{
			if( ent->CanPlayChatterSounds() )
			{
				if( !this->soundShader )
				{
					if( !ent->StartSound( this->string->c_str(), SND_CHANNEL_VOICE, 0, false, NULL ) )
					{
						gameLocal.Warning( "FrameCmd 'sound_chatter' on entity '%s', anim '%s', frame %d: Could not find sound '%s'",
										   ent->name.c_str(), anim.FullName(), frame + 1, this->string->c_str() );
					}
				}
				else
				{
					ent->StartSoundShader( this->soundShader, SND_CHANNEL_VOICE, 0, false, NULL );
				}
			}
			break;
		}
		case FC_SKIN:
		{
			ent->SetSkin( this->skin );
			break;
		}
		case FC_FX:
		{
			idEntityFx::StartFx( this->string->c_str(), NULL, NULL, ent, true );
			break;
		}
		case FC_TRIGGER:
		{
			idEntity* target = gameLocal.FindEntity( this->string->c_str() );
			if( target )
			{
				SetTimeState ts( target->timeGroup );
				target->Signal( SIG_TRIGGER );
				target->ProcessEvent( &EV_Activate, ent );
				target->TriggerGuis();
			}
			else {
				gameLocal.Warning( "FrameCmd 'trigger' on entity '%s', anim '%s', frame %d: Could not find entity '%s'",
								   ent->name.c_str(), anim.FullName(), frame + 1, this->string->c_str() );
			}
			break;
		}
		case FC_TRIGGER_SMOKE_PARTICLE:
		{
			ent->ProcessEvent( &AI_TriggerParticles, this->string->c_str() );
			break;
		}
		case FC_MELEE:
		{
			ent->ProcessEvent( &AI_AttackMelee, this->string->c_str() );
			break;
		}
		case FC_DIRECTDAMAGE:
		{
			ent->ProcessEvent( &AI_DirectDamage, this->string->c_str() );
			break;
		}
		case FC_BEGINATTACK:
		{
			ent->ProcessEvent( &AI_BeginAttack, this->string->c_str() );
			break;
		}
		case FC_ENDATTACK:
		{
			ent->ProcessEvent( &AI_EndAttack );
			break;
		}
		case FC_MUZZLEFLASH:
		{
			ent->ProcessEvent( &AI_MuzzleFlash, this->string->c_str() );
			break;
		}
		case FC_CREATEMISSILE:
		{
			ent->ProcessEvent( &AI_CreateMissile, this->string->c_str() );
			break;
		}
		case FC_LAUNCHMISSILE:
		{
			ent->ProcessEvent( &AI_AttackMissile, this->string->c_str() );
			break;
		}
		case FC_FIREMISSILEATTARGET:
		{
			ent->ProcessEvent( &AI_FireMissileAtTarget, anim.ModelDef()->GetJointName( this->index ), this->string->c_str() );
			break;
		}
		case FC_LAUNCH_PROJECTILE:
		{
			ent->ProcessEvent( &AI_LaunchProjectile, this->string->c_str() );
			break;
		}
		case FC_TRIGGER_FX:
		{
			ent->ProcessEvent( &AI_TriggerFX, anim.ModelDef()->GetJointName( this->index ), this->string->c_str() );
			break;
		}
		case FC_START_EMITTER:
		{
			int index = this->string->Find( " " );
			if( index >= 0 )
			{
				idStr name = this->string->Left( index );
				idStr particle = this->string->Right( this->string->Length() - index - 1 );
				ent->ProcessEvent( &AI_StartEmitter, name.c_str(), anim.ModelDef()->GetJointName( this->index ), particle.c_str() );
			}
		}

		case FC_STOP_EMITTER:
		{
			ent->ProcessEvent( &AI_StopEmitter, this->string->c_str() );
		}
		case FC_FOOTSTEP:
		{
			ent->ProcessEvent( &EV_Footstep );
			break;
		}
		case FC_LEFTFOOT:
		{
			ent->ProcessEvent( &EV_FootstepLeft );
			break;
		}
		case FC_RIGHTFOOT:
		{
			ent->ProcessEvent( &EV_FootstepRight );
			break;
		}
		case FC_ENABLE_EYE_FOCUS:
		{
			ent->ProcessEvent( &AI_EnableEyeFocus );
			break;
		}
		case FC_DISABLE_EYE_FOCUS:
		{
			ent->ProcessEvent( &AI_DisableEyeFocus );
			break;
		}
		case FC_DISABLE_GRAVITY:
		{
			ent->ProcessEvent( &AI_DisableGravity );
			break;
		}
		case FC_ENABLE_GRAVITY:
		{
			ent->ProcessEvent( &AI_EnableGravity );
			break;
		}
		case FC_JUMP:
		{
			ent->ProcessEvent( &AI_JumpFrame );
			break;
		}
		case FC_ENABLE_CLIP:
		{
			ent->ProcessEvent( &AI_EnableClip );
			break;
		}
		case FC_DISABLE_CLIP:
		{
			ent->ProcessEvent( &AI_DisableClip );
			break;
		}
		case FC_ENABLE_WALK_IK:
		{
			ent->ProcessEvent( &EV_EnableWalkIK );
			break;
		}
		case FC_DISABLE_WALK_IK:
		{
			ent->ProcessEvent( &EV_DisableWalkIK );
			break;
		}
		case FC_ENABLE_LEG_IK:
		{
			ent->ProcessEvent( &EV_EnableLegIK, this->index );
			break;
		}
		case FC_DISABLE_LEG_IK:
		{
			ent->ProcessEvent( &EV_DisableLegIK, this->index );
			break;
		}
		case FC_RECORDDEMO:
		{
			if( this->string )
			{
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "recordDemo %s", this->string->c_str() ) );
			}
			else
			{
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "stoprecording" );
			}
			break;
		}
		case FC_AVIGAME:
		{
			if( this->string )
			{
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "aviGame %s", this->string->c_str() ) );
			}
			else
			{
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "aviGame" );
			}
			break;
		}
	}
}

