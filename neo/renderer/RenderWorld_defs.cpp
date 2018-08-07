/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2014 Robert Beckebans

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

#include "tr_local.h"

/*
=================================================================================

	WORLD MODEL & LIGHT DEFS

=================================================================================
*/

/*
===================
 R_ListRenderLightDefs_f
===================
*/
void R_ListRenderLightDefs_f( const idCmdArgs& args )
{
	if( !tr.primaryWorld )
	{
		return;
	}
	int active = 0;
	int	totalRef = 0;
	int	totalIntr = 0;

	for( int i = 0; i < tr.primaryWorld->lightDefs.Num(); ++i )
	{
		auto ldef = tr.primaryWorld->lightDefs[ i ];
		if( !ldef )
		{
			common->Printf( "%4i: FREED\n", i );
			continue;
		}

		// count up the interactions
		int	iCount = 0;
		for( idInteraction* inter = ldef->firstInteraction; inter != NULL; inter = inter->lightNext )
		{
			iCount++;
		}
		totalIntr += iCount;

		// count up the references
		int	rCount = 0;
		for( areaReference_t* ref = ldef->references; ref; ref = ref->ownerNext )
		{
			rCount++;
		}
		totalRef += rCount;

		common->Printf( "%4i: %3i intr %2i refs %s\n", i, iCount, rCount, ldef->lightShader->GetName() );
		active++;
	}

	common->Printf( "%i lightDefs, %i interactions, %i areaRefs\n", active, totalIntr, totalRef );
}

/*
===================
 R_ListRenderEntityDefs_f
===================
*/
void R_ListRenderEntityDefs_f( const idCmdArgs& args )
{
	if( !tr.primaryWorld )
	{
		return;
	}
	int active = 0;
	int	totalRef = 0;
	int	totalIntr = 0;

	for( int i = 0; i < tr.primaryWorld->entityDefs.Num(); ++i )
	{
		auto mdef = tr.primaryWorld->entityDefs[ i ];
		if( !mdef )
		{
			common->Printf( "%4i: FREED\n", i );
			continue;
		}

		// count up the interactions
		int	iCount = 0;
		for( idInteraction* inter = mdef->firstInteraction; inter != NULL; inter = inter->entityNext )
		{
			iCount++;
		}
		totalIntr += iCount;

		// count up the references
		int	rCount = 0;
		for( areaReference_t* ref = mdef->entityRefs; ref; ref = ref->ownerNext )
		{
			rCount++;
		}
		totalRef += rCount;

		common->Printf( "%4i: %3i intr %2i refs %s\n", i, iCount, rCount, mdef->GetModel()->Name() );
		active++;
	}

	common->Printf( "total active: %i\n", active );
}

/*
===================
R_FreeDerivedData

ReloadModels and RegenerateWorld call this
===================
*/
void R_FreeDerivedData()
{
	for( int j = 0; j < tr.worlds.Num(); j++ )
	{
		auto rw = tr.worlds[ j ];

		for( int i = 0; i < rw->entityDefs.Num(); ++i )
		{
			if( rw->entityDefs[ i ] )
			{
				rw->entityDefs[ i ]->FreeDerivedData( false, false );
			}
		}

		for( int i = 0; i < rw->lightDefs.Num(); ++i )
		{
			if( rw->lightDefs[ i ] )
			{
				rw->lightDefs[ i ]->FreeDerivedData();
			}
		}
	}
}

/*
===================
R_CheckForEntityDefsUsingModel
===================
*/
void R_CheckForEntityDefsUsingModel( idRenderModel* model )
{
	for( int j = 0; j < tr.worlds.Num(); j++ )
	{
		auto rw = tr.worlds[ j ];

		for( int i = 0; i < rw->entityDefs.Num(); ++i )
		{
			auto ent = rw->entityDefs[ i ];
			if( ent )
			{
				if( ent->GetModel() == model )
				{
					//assert( 0 );
					// this should never happen but Radiant messes it up all the time so just free the derived data
					ent->FreeDerivedData( false, false );
				}
			}
		}
	}
}

/*
===================
R_ReCreateWorldReferences

ReloadModels and RegenerateWorld call this
===================
*/
void R_ReCreateWorldReferences()
{
	// let the interaction generation code know this
	// shouldn't be optimized for a particular view
	tr.viewDef = nullptr;

	for( int j = 0; j < tr.worlds.Num(); ++j )
	{
		auto rw = tr.worlds[ j ];

		for( int i = 0; i < rw->entityDefs.Num(); ++i )
		{
			auto def = rw->entityDefs[ i ];
			if( def )
			{
				// the world model entities are put specifically in a single
				// area, instead of just pushing their bounds into the tree
				if( i < rw->numPortalAreas )
				{
					rw->AddEntityRefToArea( def, &rw->portalAreas[ i ] );
				}
				else
				{
					def->CreateReferences();
				}
			}
		}

		for( int i = 0; i < rw->lightDefs.Num(); ++i )
		{
			auto light = rw->lightDefs[ i ];
			if( light )
			{
				light->GetOwner()->FreeLightDef( i );
				rw->UpdateLightDef( i, &light->GetParms() );
			}
		}
	}
}

/*
====================
R_ModulateLights_f

	Modifies the shaderParms on all the lights so the level
	designers can easily test different color schemes
====================
*/
void R_ModulateLights_f( const idCmdArgs& args )
{
	if( !tr.primaryWorld )
	{
		return;
	}
	if( args.Argc() != 4 )
	{
		common->Printf( "usage: modulateLights <redFloat> <greenFloat> <blueFloat>\n" );
		return;
	}

	float modulate[ 3 ];
	for( int i = 0; i < 3; i++ )
	{
		modulate[ i ] = atof( args.Argv( i + 1 ) );
	}

	int count = 0;
	for( int i = 0; i < tr.primaryWorld->lightDefs.Num(); ++i )
	{
		idRenderLightLocal* light = tr.primaryWorld->lightDefs[ i ];
		if( light != NULL )
		{
			count++;
			for( int j = 0; j < 3; j++ )
			{
				auto parms = const_cast< renderLightParms_t* >( &light->GetParms() );
				parms->shaderParms[ j ] *= modulate[ j ];
			}
		}
	}
	common->Printf( "modulated %i lights\n", count );
}