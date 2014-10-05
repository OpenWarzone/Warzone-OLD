//
// NPC.cpp - generic functions
//
#include "b_local.h"
#include "anims.h"
#include "say.h"
#include "icarus/Q3_Interface.h"
#include "ai_main.h"

#define __NPC_STRAFE__
//#define __NPC_BBOX_ADJUST__

#define WAYPOINT_NONE -1

extern vec3_t playerMins;
extern vec3_t playerMaxs;
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );
extern void PM_SetTorsoAnimTimer( gentity_t *ent, int *torsoAnimTimer, int time );
extern void PM_SetLegsAnimTimer( gentity_t *ent, int *legsAnimTimer, int time );
extern void NPC_BSNoClip ( void );
extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern void NPC_ApplyRoff (void);
extern void NPC_TempLookTarget ( gentity_t *self, int lookEntNum, int minLookTime, int maxLookTime );
extern void NPC_CheckPlayerAim ( void );
extern void NPC_CheckAllClear ( void );
extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern qboolean NPC_CheckLookTarget( gentity_t *self );
extern void NPC_SetLookTarget( gentity_t *self, int entNum, int clearTime );
extern void Mark1_dying( gentity_t *self );
extern void NPC_BSCinematic( void );
extern int GetTime ( int lastTime );
extern void NPC_BSGM_Default( void );
extern void NPC_CheckCharmed( void );
extern qboolean Boba_Flying( gentity_t *self );

// Conversations...
extern void NPC_NPCConversation();
extern void NPC_FindConversationPartner();
extern void NPC_StormTrooperConversation();

//Local Variables
npcStatic_t NPCS;

void NPC_SetAnim(gentity_t	*ent,int type,int anim,int priority);
void pitch_roll_for_slope( gentity_t *forwhom, vec3_t pass_slope );
extern void GM_Dying( gentity_t *self );

extern int eventClearTime;

float VectorDistanceNoHeight ( vec3_t v1, vec3_t v2 )
{
	vec3_t	dir;
	vec3_t	v1a, v2a;
	VectorCopy( v1, v1a );
	VectorCopy( v2, v2a );
	v2a[2] = v1a[2];
	VectorSubtract( v2a, v1a, dir );
	return ( VectorLength( dir) );
}

void CorpsePhysics( gentity_t *self )
{
	// run the bot through the server like it was a real client
	memset( &NPCS.ucmd, 0, sizeof( NPCS.ucmd ) );
	ClientThink( self->s.number, &NPCS.ucmd );
	//VectorCopy( self->s.origin, self->s.origin2 );
	//rww - don't get why this is happening.

	if ( self->client->NPC_class == CLASS_GALAKMECH )
	{
		GM_Dying( self );
	}
	//FIXME: match my pitch and roll for the slope of my groundPlane
	if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE && !(self->s.eFlags&EF_DISINTEGRATION) )
	{//on the ground
		//FIXME: check 4 corners
		pitch_roll_for_slope( self, NULL );
	}

	if ( eventClearTime == level.time + ALERT_CLEAR_TIME )
	{//events were just cleared out so add me again
		if ( !(self->client->ps.eFlags&EF_NODRAW) )
		{
			AddSightEvent( self->enemy, self->r.currentOrigin, 384, AEL_DISCOVERED, 0.0f );
		}
	}

	if ( level.time - self->s.time > 3000 )
	{//been dead for 3 seconds
		if ( g_dismember.integer < 11381138 && !g_saberRealisticCombat.integer )
		{//can't be dismembered once dead
			if ( self->client->NPC_class != CLASS_PROTOCOL )
			{
			//	self->client->dismembered = qtrue;
			}
		}
	}

	//if ( level.time - self->s.time > 500 )
	if (self->client->respawnTime < (level.time+500))
	{//don't turn "nonsolid" until about 1 second after actual death

		if (self->client->ps.eFlags & EF_DISINTEGRATION)
		{
			self->r.contents = 0;
		}
		else if ((self->client->NPC_class != CLASS_MARK1) && (self->client->NPC_class != CLASS_INTERROGATOR))	// The Mark1 & Interrogator stays solid.
		{
			self->r.contents = CONTENTS_CORPSE;
			//self->r.maxs[2] = -8;
		}

		if ( self->message )
		{
			self->r.contents |= CONTENTS_TRIGGER;
		}
	}
}

/*
----------------------------------------
NPC_RemoveBody

Determines when it's ok to ditch the corpse
----------------------------------------
*/
#define REMOVE_DISTANCE		128
#define REMOVE_DISTANCE_SQR (REMOVE_DISTANCE * REMOVE_DISTANCE)

void NPC_RemoveBody( gentity_t *self )
{
	CorpsePhysics( self );

	self->nextthink = level.time + FRAMETIME;

	if ( self->NPC->nextBStateThink <= level.time )
	{
		trap->ICARUS_MaintainTaskManager(self->s.number);
	}
	self->NPC->nextBStateThink = level.time + FRAMETIME;

	if ( self->message )
	{//I still have a key
		return;
	}

	// I don't consider this a hack, it's creative coding . . .
	// I agree, very creative... need something like this for ATST and GALAKMECH too!
	if (self->client->NPC_class == CLASS_MARK1)
	{
		Mark1_dying( self );
	}

	// Since these blow up, remove the bounding box.
	if ( self->client->NPC_class == CLASS_REMOTE
		|| self->client->NPC_class == CLASS_SENTRY
		|| self->client->NPC_class == CLASS_PROBE
		|| self->client->NPC_class == CLASS_INTERROGATOR
		|| self->client->NPC_class == CLASS_MARK2 )
	{
		//if ( !self->taskManager || !self->taskManager->IsRunning() )
		if (!trap->ICARUS_IsRunning(self->s.number))
		{
			if ( !self->activator || !self->activator->client || !(self->activator->client->ps.eFlags2&EF2_HELD_BY_MONSTER) )
			{//not being held by a Rancor
				G_FreeEntity( self );
			}
		}
		return;
	}

	//FIXME: don't ever inflate back up?
	self->r.maxs[2] = self->client->renderInfo.eyePoint[2] - self->r.currentOrigin[2] + 4;
	if ( self->r.maxs[2] < -8 )
	{
		self->r.maxs[2] = -8;
	}

	if ( self->client->NPC_class == CLASS_GALAKMECH )
	{//never disappears
		return;
	}
	if ( self->NPC && self->NPC->timeOfDeath <= level.time )
	{
		self->NPC->timeOfDeath = level.time + 1000;
		// Only do all of this nonsense for Scav boys ( and girls )
	///	if ( self->client->playerTeam == NPCTEAM_SCAVENGERS || self->client->playerTeam == NPCTEAM_KLINGON
	//		|| self->client->playerTeam == NPCTEAM_HIROGEN || self->client->playerTeam == NPCTEAM_MALON )
		// should I check NPC_class here instead of TEAM ? - dmv
		if( self->client->playerTeam == NPCTEAM_ENEMY || self->client->NPC_class == CLASS_PROTOCOL )
		{
			self->nextthink = level.time + FRAMETIME; // try back in a second

			/*
			if ( DistanceSquared( g_entities[0].r.currentOrigin, self->r.currentOrigin ) <= REMOVE_DISTANCE_SQR )
			{
				return;
			}

			if ( (InFOV( self, &g_entities[0], 110, 90 )) ) // generous FOV check
			{
				if ( (NPC_ClearLOS2( &g_entities[0], self->r.currentOrigin )) )
				{
					return;
				}
			}
			*/
			//Don't care about this for MP I guess.
		}

		//FIXME: there are some conditions - such as heavy combat - in which we want
		//			to remove the bodies... but in other cases it's just weird, like
		//			when they're right behind you in a closed room and when they've been
		//			placed as dead NPCs by a designer...
		//			For now we just assume that a corpse with no enemy was
		//			placed in the map as a corpse
		if ( self->enemy )
		{
			//if ( !self->taskManager || !self->taskManager->IsRunning() )
			if (!trap->ICARUS_IsRunning(self->s.number))
			{
				if ( !self->activator || !self->activator->client || !(self->activator->client->ps.eFlags2&EF2_HELD_BY_MONSTER) )
				{//not being held by a Rancor
					if ( self->client && self->client->ps.saberEntityNum > 0 && self->client->ps.saberEntityNum < ENTITYNUM_WORLD )
					{
						gentity_t *saberent = &g_entities[self->client->ps.saberEntityNum];
						if ( saberent )
						{
							G_FreeEntity( saberent );
						}
					}
					G_FreeEntity( self );
				}
			}
		}
	}
}

/*
----------------------------------------
NPC_RemoveBody

Determines when it's ok to ditch the corpse
----------------------------------------
*/

int BodyRemovalPadTime( gentity_t *ent )
{
	int	time;

	if ( !ent || !ent->client )
		return 0;
/*
	switch ( ent->client->playerTeam )
	{
	case NPCTEAM_KLINGON:	// no effect, we just remove them when the player isn't looking
	case NPCTEAM_SCAVENGERS:
	case NPCTEAM_HIROGEN:
	case NPCTEAM_MALON:
	case NPCTEAM_IMPERIAL:
	case NPCTEAM_STARFLEET:
		time = 10000; // 15 secs.
		break;

	case NPCTEAM_BORG:
		time = 2000;
		break;

	case NPCTEAM_STASIS:
		return qtrue;
		break;

	case NPCTEAM_FORGE:
		time = 1000;
		break;

	case NPCTEAM_BOTS:
//		if (!Q_stricmp( ent->NPC_type, "mouse" ))
//		{
			time = 0;
//		}
//		else
//		{
//			time = 10000;
//		}
		break;

	case NPCTEAM_8472:
		time = 2000;
		break;

	default:
		// never go away
		time = Q3_INFINITE;
		break;
	}
*/
	// team no longer indicates species/race, so in this case we'd use NPC_class, but
	switch( ent->client->NPC_class )
	{
	case CLASS_MOUSE:
	case CLASS_GONK:
	case CLASS_R2D2:
	case CLASS_R5D2:
	//case CLASS_PROTOCOL:
	case CLASS_MARK1:
	case CLASS_MARK2:
	case CLASS_PROBE:
	case CLASS_SEEKER:
	case CLASS_REMOTE:
	case CLASS_SENTRY:
	case CLASS_INTERROGATOR:
		time = 0;
		break;
	default:
		// never go away
	//	time = Q3_INFINITE;
		// for now I'm making default 10000
		time = 10000;
		break;

	}


	return time;
}


/*
----------------------------------------
NPC_RemoveBodyEffect

Effect to be applied when ditching the corpse
----------------------------------------
*/

static void NPC_RemoveBodyEffect(void)
{
//	vec3_t		org;
//	gentity_t	*tent;

	if ( !NPCS.NPC || !NPCS.NPC->client || (NPCS.NPC->s.eFlags & EF_NODRAW) )
		return;
/*
	switch(NPC->client->playerTeam)
	{
	case NPCTEAM_STARFLEET:
		//FIXME: Starfleet beam out
		break;

	case NPCTEAM_BOTS:
//		VectorCopy( NPC->r.currentOrigin, org );
//		org[2] -= 16;
//		tent = G_TempEntity( org, EV_BOT_EXPLODE );
//		tent->owner = NPC;

		break;

	default:
		break;
	}
*/


	// team no longer indicates species/race, so in this case we'd use NPC_class, but

	// stub code
	switch(NPCS.NPC->client->NPC_class)
	{
	case CLASS_PROBE:
	case CLASS_SEEKER:
	case CLASS_REMOTE:
	case CLASS_SENTRY:
	case CLASS_GONK:
	case CLASS_R2D2:
	case CLASS_R5D2:
	//case CLASS_PROTOCOL:
	case CLASS_MARK1:
	case CLASS_MARK2:
	case CLASS_INTERROGATOR:
	case CLASS_ATST: // yeah, this is a little weird, but for now I'm listing all droids
	//	VectorCopy( NPC->r.currentOrigin, org );
	//	org[2] -= 16;
	//	tent = G_TempEntity( org, EV_BOT_EXPLODE );
	//	tent->owner = NPC;
		break;
	default:
		break;
	}


}


/*
====================================================================
void pitch_roll_for_slope( gentity_t *forwhom, vec3_t pass_slope )

MG

This will adjust the pitch and roll of a monster to match
a given slope - if a non-'0 0 0' slope is passed, it will
use that value, otherwise it will use the ground underneath
the monster.  If it doesn't find a surface, it does nothinh\g
and returns.
====================================================================
*/

void pitch_roll_for_slope( gentity_t *forwhom, vec3_t pass_slope )
{
	vec3_t	slope;
	vec3_t	nvf, ovf, ovr, startspot, endspot, new_angles = { 0, 0, 0 };
	float	pitch, mod, dot;

	//if we don't have a slope, get one
	if( !pass_slope || VectorCompare( vec3_origin, pass_slope ) )
	{
		trace_t trace;

		VectorCopy( forwhom->r.currentOrigin, startspot );
		startspot[2] += forwhom->r.mins[2] + 4;
		VectorCopy( startspot, endspot );
		endspot[2] -= 300;
		trap->Trace( &trace, forwhom->r.currentOrigin, vec3_origin, vec3_origin, endspot, forwhom->s.number, MASK_SOLID, qfalse, 0, 0 );
//		if(trace_fraction>0.05&&forwhom.movetype==MOVETYPE_STEP)
//			forwhom.flags(-)FL_ONGROUND;

		if ( trace.fraction >= 1.0 )
			return;

		if( !( &trace.plane ) )
			return;

		if ( VectorCompare( vec3_origin, trace.plane.normal ) )
			return;

		VectorCopy( trace.plane.normal, slope );
	}
	else
	{
		VectorCopy( pass_slope, slope );
	}


	AngleVectors( forwhom->r.currentAngles, ovf, ovr, NULL );

	vectoangles( slope, new_angles );
	pitch = new_angles[PITCH] + 90;
	new_angles[ROLL] = new_angles[PITCH] = 0;

	AngleVectors( new_angles, nvf, NULL, NULL );

	mod = DotProduct( nvf, ovr );

	if ( mod<0 )
		mod = -1;
	else
		mod = 1;

	dot = DotProduct( nvf, ovf );

	if ( forwhom->client )
	{
		float oldmins2;

		forwhom->client->ps.viewangles[PITCH] = dot * pitch;
		forwhom->client->ps.viewangles[ROLL] = ((1-Q_fabs(dot)) * pitch * mod);
		oldmins2 = forwhom->r.mins[2];
		forwhom->r.mins[2] = -24 + 12 * fabs(forwhom->client->ps.viewangles[PITCH])/180.0f;
		//FIXME: if it gets bigger, move up
		if ( oldmins2 > forwhom->r.mins[2] )
		{//our mins is now lower, need to move up
			//FIXME: trace?
			forwhom->client->ps.origin[2] += (oldmins2 - forwhom->r.mins[2]);
			forwhom->r.currentOrigin[2] = forwhom->client->ps.origin[2];
			trap->LinkEntity( (sharedEntity_t *)forwhom );
		}
	}
	else
	{
		forwhom->r.currentAngles[PITCH] = dot * pitch;
		forwhom->r.currentAngles[ROLL] = ((1-Q_fabs(dot)) * pitch * mod);
	}
}


/*
----------------------------------------
DeadThink
----------------------------------------
*/
static void DeadThink ( void )
{
	trace_t	trace;

	//HACKHACKHACKHACKHACK
	//We should really have a seperate G2 bounding box (seperate from the physics bbox) for G2 collisions only
	//FIXME: don't ever inflate back up?
	NPCS.NPC->r.maxs[2] = NPCS.NPC->client->renderInfo.eyePoint[2] - NPCS.NPC->r.currentOrigin[2] + 4;
	if ( NPCS.NPC->r.maxs[2] < -8 )
	{
		NPCS.NPC->r.maxs[2] = -8;
	}
	if ( VectorCompare( NPCS.NPC->client->ps.velocity, vec3_origin ) )
	{//not flying through the air
		if ( NPCS.NPC->r.mins[0] > -32 )
		{
			NPCS.NPC->r.mins[0] -= 1;
			trap->Trace (&trace, NPCS.NPC->r.currentOrigin, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, NPCS.NPC->r.currentOrigin, NPCS.NPC->s.number, NPCS.NPC->clipmask, qfalse, 0, 0 );
			if ( trace.allsolid )
			{
				NPCS.NPC->r.mins[0] += 1;
			}
		}
		if ( NPCS.NPC->r.maxs[0] < 32 )
		{
			NPCS.NPC->r.maxs[0] += 1;
			trap->Trace (&trace, NPCS.NPC->r.currentOrigin, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, NPCS.NPC->r.currentOrigin, NPCS.NPC->s.number, NPCS.NPC->clipmask, qfalse, 0, 0 );
			if ( trace.allsolid )
			{
				NPCS.NPC->r.maxs[0] -= 1;
			}
		}
		if ( NPCS.NPC->r.mins[1] > -32 )
		{
			NPCS.NPC->r.mins[1] -= 1;
			trap->Trace (&trace, NPCS.NPC->r.currentOrigin, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, NPCS.NPC->r.currentOrigin, NPCS.NPC->s.number, NPCS.NPC->clipmask, qfalse, 0, 0 );
			if ( trace.allsolid )
			{
				NPCS.NPC->r.mins[1] += 1;
			}
		}
		if ( NPCS.NPC->r.maxs[1] < 32 )
		{
			NPCS.NPC->r.maxs[1] += 1;
			trap->Trace (&trace, NPCS.NPC->r.currentOrigin, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, NPCS.NPC->r.currentOrigin, NPCS.NPC->s.number, NPCS.NPC->clipmask, qfalse, 0, 0 );
			if ( trace.allsolid )
			{
				NPCS.NPC->r.maxs[1] -= 1;
			}
		}
	}
	//HACKHACKHACKHACKHACK

	//FIXME: tilt and fall off of ledges?
	//NPC_PostDeathThink();

	/*
	if ( !NPCInfo->timeOfDeath && NPC->client != NULL && NPCInfo != NULL )
	{
		//haven't finished death anim yet and were NOT given a specific amount of time to wait before removal
		int				legsAnim	= NPC->client->ps.legsAnim;
		animation_t		*animations	= knownAnimFileSets[NPC->client->clientInfo.animFileIndex].animations;

		NPC->bounceCount = -1; // This is a cheap hack for optimizing the pointcontents check below

		//ghoul doesn't tell us this anymore
		//if ( NPC->client->renderInfo.legsFrame == animations[legsAnim].firstFrame + (animations[legsAnim].numFrames - 1) )
		{
			//reached the end of the death anim
			NPCInfo->timeOfDeath = level.time + BodyRemovalPadTime( NPC );
		}
	}
	else
	*/
	{
		//death anim done (or were given a specific amount of time to wait before removal), wait the requisite amount of time them remove
		if ( level.time >= NPCS.NPCInfo->timeOfDeath + BodyRemovalPadTime( NPCS.NPC ) )
		{
			if ( NPCS.NPC->client->ps.eFlags & EF_NODRAW )
			{
				if (!trap->ICARUS_IsRunning(NPCS.NPC->s.number))
				//if ( !NPC->taskManager || !NPC->taskManager->IsRunning() )
				{
					NPCS.NPC->think = G_FreeEntity;
					NPCS.NPC->nextthink = level.time + FRAMETIME;
				}
			}
			else
			{
				class_t	npc_class;

				// Start the body effect first, then delay 400ms before ditching the corpse
				NPC_RemoveBodyEffect();

				//FIXME: keep it running through physics somehow?
				NPCS.NPC->think = NPC_RemoveBody;
				NPCS.NPC->nextthink = level.time + FRAMETIME;
			//	if ( NPC->client->playerTeam == NPCTEAM_FORGE )
			//		NPCInfo->timeOfDeath = level.time + FRAMETIME * 8;
			//	else if ( NPC->client->playerTeam == NPCTEAM_BOTS )
				npc_class = NPCS.NPC->client->NPC_class;
				// check for droids
				if ( npc_class == CLASS_SEEKER || npc_class == CLASS_REMOTE || npc_class == CLASS_PROBE || npc_class == CLASS_MOUSE ||
					 npc_class == CLASS_GONK || npc_class == CLASS_R2D2 || npc_class == CLASS_R5D2 ||
					 npc_class == CLASS_MARK2 || npc_class == CLASS_SENTRY )//npc_class == CLASS_PROTOCOL ||
				{
					NPCS.NPC->client->ps.eFlags |= EF_NODRAW;
					NPCS.NPCInfo->timeOfDeath = level.time + FRAMETIME * 8;
				}
				else
					NPCS.NPCInfo->timeOfDeath = level.time + FRAMETIME * 4;
			}
			return;
		}
	}

	// If the player is on the ground and the resting position contents haven't been set yet...(BounceCount tracks the contents)
	if ( NPCS.NPC->bounceCount < 0 && NPCS.NPC->s.groundEntityNum >= 0 )
	{
		// if client is in a nodrop area, make him/her nodraw
		int contents = NPCS.NPC->bounceCount = trap->PointContents( NPCS.NPC->r.currentOrigin, -1 );

		if ( ( contents & CONTENTS_NODROP ) )
		{
			NPCS.NPC->client->ps.eFlags |= EF_NODRAW;
		}
	}

	CorpsePhysics( NPCS.NPC );
}


/*
===============
SetNPCGlobals

local function to set globals used throughout the AI code
===============
*/
void SetNPCGlobals( gentity_t *ent )
{
	NPCS.NPC = ent;
	NPCS.NPCInfo = ent->NPC;
	NPCS.client = ent->client;
	memset( &NPCS.ucmd, 0, sizeof( usercmd_t ) );
}

npcStatic_t _saved_NPCS;

void SaveNPCGlobals(void)
{
	memcpy( &_saved_NPCS, &NPCS, sizeof( _saved_NPCS ) );
}

void RestoreNPCGlobals(void)
{
	memcpy( &NPCS, &_saved_NPCS, sizeof( _saved_NPCS ) );
}

//We MUST do this, other funcs were using NPC illegally when "self" wasn't the global NPC
void ClearNPCGlobals( void )
{
	NPCS.NPC = NULL;
	NPCS.NPCInfo = NULL;
	NPCS.client = NULL;
}
//===============

extern	qboolean	showBBoxes;
vec3_t NPCDEBUG_RED = {1.0, 0.0, 0.0};
vec3_t NPCDEBUG_GREEN = {0.0, 1.0, 0.0};
vec3_t NPCDEBUG_BLUE = {0.0, 0.0, 1.0};
vec3_t NPCDEBUG_LIGHT_BLUE = {0.3f, 0.7f, 1.0};
extern void G_Cube( vec3_t mins, vec3_t maxs, vec3_t color, float alpha );
extern void G_Line( vec3_t start, vec3_t end, vec3_t color, float alpha );
extern void G_Cylinder( vec3_t start, vec3_t end, float radius, vec3_t color );

void NPC_ShowDebugInfo (void)
{
	if ( showBBoxes )
	{
		gentity_t	*found = NULL;
		vec3_t		mins, maxs;

		while( (found = G_Find( found, FOFS(classname), "NPC" ) ) != NULL )
		{
			if ( trap->InPVS( found->r.currentOrigin, g_entities[0].r.currentOrigin ) )
			{
				VectorAdd( found->r.currentOrigin, found->r.mins, mins );
				VectorAdd( found->r.currentOrigin, found->r.maxs, maxs );
				G_Cube( mins, maxs, NPCDEBUG_RED, 0.25 );
			}
		}
	}
}

void NPC_ApplyScriptFlags (void)
{
	if ( NPCS.NPCInfo->scriptFlags & SCF_CROUCHED )
	{
		if ( NPCS.NPCInfo->charmedTime > level.time && (NPCS.ucmd.forwardmove || NPCS.ucmd.rightmove) )
		{//ugh, if charmed and moving, ignore the crouched command
		}
		else
		{
			NPCS.ucmd.upmove = -127;
		}
	}

	if(NPCS.NPCInfo->scriptFlags & SCF_RUNNING)
	{
		NPCS.ucmd.buttons &= ~BUTTON_WALKING;
	}
	else if(NPCS.NPCInfo->scriptFlags & SCF_WALKING)
	{
		if ( NPCS.NPCInfo->charmedTime > level.time && (NPCS.ucmd.forwardmove || NPCS.ucmd.rightmove) )
		{//ugh, if charmed and moving, ignore the walking command
		}
		else
		{
			NPCS.ucmd.buttons |= BUTTON_WALKING;
		}
	}
/*
	if(NPCInfo->scriptFlags & SCF_CAREFUL)
	{
		ucmd.buttons |= BUTTON_CAREFUL;
	}
*/
	if(NPCS.NPCInfo->scriptFlags & SCF_LEAN_RIGHT)
	{
		NPCS.ucmd.buttons |= BUTTON_USE;
		NPCS.ucmd.rightmove = 127;
		NPCS.ucmd.forwardmove = 0;
		NPCS.ucmd.upmove = 0;
	}
	else if(NPCS.NPCInfo->scriptFlags & SCF_LEAN_LEFT)
	{
		NPCS.ucmd.buttons |= BUTTON_USE;
		NPCS.ucmd.rightmove = -127;
		NPCS.ucmd.forwardmove = 0;
		NPCS.ucmd.upmove = 0;
	}

	if ( (NPCS.NPCInfo->scriptFlags & SCF_ALT_FIRE) && (NPCS.ucmd.buttons & BUTTON_ATTACK) )
	{//Use altfire instead
		NPCS.ucmd.buttons |= BUTTON_ALT_ATTACK;
	}
}

void Q3_DebugPrint( int level, const char *format, ... );
void NPC_HandleAIFlags (void)
{
	//FIXME: make these flags checks a function call like NPC_CheckAIFlagsAndTimers
	if ( NPCS.NPCInfo->aiFlags & NPCAI_LOST )
	{//Print that you need help!
		//FIXME: shouldn't remove this just yet if cg_draw needs it
		NPCS.NPCInfo->aiFlags &= ~NPCAI_LOST;

		/*
		if ( showWaypoints )
		{
			Q3_DebugPrint(WL_WARNING, "%s can't navigate to target %s (my wp: %d, goal wp: %d)\n", NPC->targetname, NPCInfo->goalEntity->targetname, NPC->waypoint, NPCInfo->goalEntity->waypoint );
		}
		*/

		if ( NPCS.NPCInfo->goalEntity && NPCS.NPCInfo->goalEntity == NPCS.NPC->enemy )
		{//We can't nav to our enemy
			//Drop enemy and see if we should search for him
			NPC_LostEnemyDecideChase();
		}
	}

	//MRJ Request:
	/*
	if ( NPCInfo->aiFlags & NPCAI_GREET_ALLIES && !NPC->enemy )//what if "enemy" is the greetEnt?
	{//If no enemy, look for teammates to greet
		//FIXME: don't say hi to the same guy over and over again.
		if ( NPCInfo->greetingDebounceTime < level.time )
		{//Has been at least 2 seconds since we greeted last
			if ( !NPCInfo->greetEnt )
			{//Find a teammate whom I'm facing and who is facing me and within 128
				NPCInfo->greetEnt = NPC_PickAlly( qtrue, 128, qtrue, qtrue );
			}

			if ( NPCInfo->greetEnt && !Q_irand(0, 5) )
			{//Start greeting someone
				qboolean	greeted = qfalse;

				//TODO:  If have a greetscript, run that instead?

				//FIXME: make them greet back?
				if( !Q_irand( 0, 2 ) )
				{//Play gesture anim (press gesture button?)
					greeted = qtrue;
					NPC_SetAnim( NPC, SETANIM_TORSO, Q_irand( BOTH_GESTURE1, BOTH_GESTURE3 ), SETANIM_FLAG_NORMAL|SETANIM_FLAG_HOLD );
					//NOTE: play full-body gesture if not moving?
				}

				if( !Q_irand( 0, 2 ) )
				{//Play random voice greeting sound
					greeted = qtrue;
					//FIXME: need NPC sound sets

					//G_AddVoiceEvent( NPC, Q_irand(EV_GREET1, EV_GREET3), 2000 );
				}

				if( !Q_irand( 0, 1 ) )
				{//set looktarget to them for a second or two
					greeted = qtrue;
					NPC_TempLookTarget( NPC, NPCInfo->greetEnt->s.number, 1000, 3000 );
				}

				if ( greeted )
				{//Did at least one of the things above
					//Don't greet again for 2 - 4 seconds
					NPCInfo->greetingDebounceTime = level.time + Q_irand( 2000, 4000 );
					NPCInfo->greetEnt = NULL;
				}
			}
		}
	}
	*/
	//been told to play a victory sound after a delay
	if ( NPCS.NPCInfo->greetingDebounceTime && NPCS.NPCInfo->greetingDebounceTime < level.time )
	{
		G_AddVoiceEvent( NPCS.NPC, Q_irand(EV_VICTORY1, EV_VICTORY3), Q_irand( 2000, 4000 ) );
		NPCS.NPCInfo->greetingDebounceTime = 0;
	}

	if ( NPCS.NPCInfo->ffireCount > 0 )
	{
		if ( NPCS.NPCInfo->ffireFadeDebounce < level.time )
		{
			NPCS.NPCInfo->ffireCount--;
			//Com_Printf( "drop: %d < %d\n", NPCInfo->ffireCount, 3+((2-g_npcspskill.integer)*2) );
			NPCS.NPCInfo->ffireFadeDebounce = level.time + 3000;
		}
	}
	if ( d_patched.integer )
	{//use patch-style navigation
		if ( NPCS.NPCInfo->consecutiveBlockedMoves > 20 )
		{//been stuck for a while, try again?
			NPCS.NPCInfo->consecutiveBlockedMoves = 0;
		}
	}
}

void NPC_AvoidWallsAndCliffs (void)
{
	//...
}

void NPC_CheckAttackScript(void)
{
	if(!(NPCS.ucmd.buttons & BUTTON_ATTACK))
	{
		return;
	}

	G_ActivateBehavior(NPCS.NPC, BSET_ATTACK);
}

float NPC_MaxDistSquaredForWeapon (void);
void NPC_CheckAttackHold(void)
{
	vec3_t		vec;

	// If they don't have an enemy they shouldn't hold their attack anim.
	if ( !NPCS.NPC->enemy )
	{
		NPCS.NPCInfo->attackHoldTime = 0;
		return;
	}

/*	if ( ( NPC->client->ps.weapon == WP_BORG_ASSIMILATOR ) || ( NPC->client->ps.weapon == WP_BORG_DRILL ) )
	{//FIXME: don't keep holding this if can't hit enemy?

		// If they don't have shields ( been disabled) they shouldn't hold their attack anim.
		if ( !(NPC->NPC->aiFlags & NPCAI_SHIELDS) )
		{
			NPCInfo->attackHoldTime = 0;
			return;
		}

		VectorSubtract(NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, vec);
		if( VectorLengthSquared(vec) > NPC_MaxDistSquaredForWeapon() )
		{
			NPCInfo->attackHoldTime = 0;
			PM_SetTorsoAnimTimer(NPC, &NPC->client->ps.torsoAnimTimer, 0);
		}
		else if( NPCInfo->attackHoldTime && NPCInfo->attackHoldTime > level.time )
		{
			ucmd.buttons |= BUTTON_ATTACK;
		}
		else if ( ( NPCInfo->attackHold ) && ( ucmd.buttons & BUTTON_ATTACK ) )
		{
			NPCInfo->attackHoldTime = level.time + NPCInfo->attackHold;
			PM_SetTorsoAnimTimer(NPC, &NPC->client->ps.torsoAnimTimer, NPCInfo->attackHold);
		}
		else
		{
			NPCInfo->attackHoldTime = 0;
			PM_SetTorsoAnimTimer(NPC, &NPC->client->ps.torsoAnimTimer, 0);
		}
	}
	else*/
	{//everyone else...?  FIXME: need to tie this into AI somehow?
		VectorSubtract(NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, vec);
		if( VectorLengthSquared(vec) > NPC_MaxDistSquaredForWeapon() )
		{
			NPCS.NPCInfo->attackHoldTime = 0;
		}
		else if( NPCS.NPCInfo->attackHoldTime && NPCS.NPCInfo->attackHoldTime > level.time )
		{
			NPCS.ucmd.buttons |= BUTTON_ATTACK;
		}
		else if ( ( NPCS.NPCInfo->attackHold ) && ( NPCS.ucmd.buttons & BUTTON_ATTACK ) )
		{
			NPCS.NPCInfo->attackHoldTime = level.time + NPCS.NPCInfo->attackHold;
		}
		else
		{
			NPCS.NPCInfo->attackHoldTime = 0;
		}
	}
}

/*
void NPC_KeepCurrentFacing(void)

Fills in a default ucmd to keep current angles facing
*/
void NPC_KeepCurrentFacing(void)
{
	if(!NPCS.ucmd.angles[YAW])
	{
		NPCS.ucmd.angles[YAW] = ANGLE2SHORT( NPCS.client->ps.viewangles[YAW] ) - NPCS.client->ps.delta_angles[YAW];
	}

	if(!NPCS.ucmd.angles[PITCH])
	{
		NPCS.ucmd.angles[PITCH] = ANGLE2SHORT( NPCS.client->ps.viewangles[PITCH] ) - NPCS.client->ps.delta_angles[PITCH];
	}
}

qboolean NPC_CanUseAdvancedFighting()
{// UQ1: Evasion/Weapon Switching/etc...
	// Who can evade???
	switch (NPCS.NPC->client->NPC_class)
	{
	//case CLASS_ATST:
	case CLASS_BARTENDER:
	case CLASS_BESPIN_COP:		
	case CLASS_CLAW:
	case CLASS_COMMANDO:
	case CLASS_DESANN:		
	//case CLASS_FISH:
	//case CLASS_FLIER2:
	case CLASS_GALAK:
	//case CLASS_GLIDER:
	//case CLASS_GONK:				// droid
	case CLASS_GRAN:
	//case CLASS_HOWLER:
	case CLASS_IMPERIAL:
	case CLASS_IMPWORKER:
	//case CLASS_INTERROGATOR:		// droid 
	case CLASS_JAN:				
	case CLASS_JEDI:
	case CLASS_KYLE:				
	case CLASS_LANDO:			
	//case CLASS_LIZARD:
	case CLASS_LUKE:				
	case CLASS_MARK1:			// droid
	case CLASS_MARK2:			// droid
	//case CLASS_GALAKMECH:		// droid
	//case CLASS_MINEMONSTER:
	case CLASS_MONMOTHA:			
	case CLASS_MORGANKATARN:
	//case CLASS_MOUSE:			// droid
	case CLASS_MURJJ:
	case CLASS_PRISONER:
	//case CLASS_PROBE:			// droid
	case CLASS_PROTOCOL:			// droid
	//case CLASS_R2D2:				// droid
	//case CLASS_R5D2:				// droid
	case CLASS_REBEL:
	case CLASS_REBORN:
	case CLASS_REELO:
	//case CLASS_REMOTE:
	case CLASS_RODIAN:
	//case CLASS_SEEKER:			// droid
	//case CLASS_SENTRY:
	case CLASS_SHADOWTROOPER:
	case CLASS_STORMTROOPER:
	case CLASS_SWAMP:
	case CLASS_SWAMPTROOPER:
	case CLASS_TAVION:
	case CLASS_TRANDOSHAN:
	case CLASS_UGNAUGHT:
	case CLASS_JAWA:
	case CLASS_WEEQUAY:
	case CLASS_BOBAFETT:
	//case CLASS_VEHICLE:
	//case CLASS_RANCOR:
	//case CLASS_WAMPA:
		// OK... EVADE AWAY!!!
		break;
	default:
		// NOT OK...
		return qfalse;
		break;
	}

	return qtrue;
}

/*
-------------------------
NPC_BehaviorSet_Charmed
-------------------------
*/

void NPC_BehaviorSet_Charmed( int bState )
{
	switch( bState )
	{
	case BS_FOLLOW_LEADER://# 40: Follow your leader and shoot any enemies you come across
		NPC_BSFollowLeader();
		break;
	case BS_REMOVE:
		NPC_BSRemove();
		break;
	case BS_SEARCH:			//# 43: Using current waypoint as a base, search the immediate branches of waypoints for enemies
		NPC_BSSearch();
		break;
	case BS_WANDER:			//# 46: Wander down random waypoint paths
		NPC_BSWander();
		break;
	case BS_FLEE:
		NPC_BSFlee();
		break;
	default:
	case BS_DEFAULT://whatever
		NPC_BSDefault();
		break;
	}
}
/*
-------------------------
NPC_BehaviorSet_Default
-------------------------
*/

void NPC_BehaviorSet_Default( int bState )
{
	if ( NPCS.NPC->enemy && NPCS.NPC->enemy->inuse && NPCS.NPC->enemy->health > 0)
	{// UQ1: Have an anemy... Check if we should use advanced fighting for this NPC...
		if ( NPC_CanUseAdvancedFighting() )
		{// UQ1: This NPC can use advanced tactics... Use them!!!
			NPC_BSJedi_Default();
			return;
		}
	}

	switch( bState )
	{
	case BS_ADVANCE_FIGHT://head toward captureGoal, shoot anything that gets in the way
		NPC_BSAdvanceFight ();
		break;
	case BS_SLEEP://Follow a path, looking for enemies
		NPC_BSSleep ();
		break;
	case BS_FOLLOW_LEADER://# 40: Follow your leader and shoot any enemies you come across
		NPC_BSFollowLeader();
		break;
	case BS_JUMP:			//41: Face navgoal and jump to it.
		NPC_BSJump();
		break;
	case BS_REMOVE:
		NPC_BSRemove();
		break;
	case BS_SEARCH:			//# 43: Using current waypoint as a base, search the immediate branches of waypoints for enemies
		NPC_BSSearch();
		break;
	case BS_NOCLIP:
		NPC_BSNoClip();
		break;
	case BS_WANDER:			//# 46: Wander down random waypoint paths
		NPC_BSWander();
		break;
	case BS_FLEE:
		NPC_BSFlee();
		break;
	case BS_WAIT:
		NPC_BSWait();
		break;
	case BS_CINEMATIC:
		NPC_BSCinematic();
		break;
	default:
	case BS_DEFAULT://whatever
		NPC_BSDefault();
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Interrogator
-------------------------
*/
void NPC_BehaviorSet_Interrogator( int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSInterrogator_Default();
		break;
	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

void NPC_BSImperialProbe_Attack( void );
void NPC_BSImperialProbe_Patrol( void );
void NPC_BSImperialProbe_Wait(void);

/*
-------------------------
NPC_BehaviorSet_ImperialProbe
-------------------------
*/
void NPC_BehaviorSet_ImperialProbe( int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSImperialProbe_Default();
		break;
	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}


void NPC_BSSeeker_Default( void );

/*
-------------------------
NPC_BehaviorSet_Seeker
-------------------------
*/
void NPC_BehaviorSet_Seeker( int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSSeeker_Default();
		break;
	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

void NPC_BSRemote_Default( void );

/*
-------------------------
NPC_BehaviorSet_Remote
-------------------------
*/
void NPC_BehaviorSet_Remote( int bState )
{
	NPC_BSRemote_Default();
}

void NPC_BSSentry_Default( void );

/*
-------------------------
NPC_BehaviorSet_Sentry
-------------------------
*/
void NPC_BehaviorSet_Sentry( int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSSentry_Default();
		break;
	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Grenadier
-------------------------
*/
void NPC_BehaviorSet_Grenadier( int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSGrenadier_Default();
		break;

	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}
/*
-------------------------
NPC_BehaviorSet_Sniper
-------------------------
*/
void NPC_BehaviorSet_Sniper( int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		//NPC_BSSniper_Default();
		NPC_BSST_Default();
		break;

	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}
/*
-------------------------
NPC_BehaviorSet_Stormtrooper
-------------------------
*/

void NPC_BehaviorSet_Stormtrooper( int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSST_Default();
		break;

	case BS_INVESTIGATE:
		NPC_BSST_Investigate();
		break;

	case BS_SLEEP:
		NPC_BSST_Sleep();
		break;

	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Jedi
-------------------------
*/

void NPC_BehaviorSet_Jedi( int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSJedi_Default();
		break;

	case BS_FOLLOW_LEADER:
		NPC_BSJedi_FollowLeader();
		break;

	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Droid
-------------------------
*/
void NPC_BehaviorSet_Droid( int bState )
{
	switch( bState )
	{
	case BS_DEFAULT:
	case BS_STAND_GUARD:
	case BS_PATROL:
		NPC_BSDroid_Default();
		break;
	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Mark1
-------------------------
*/
void NPC_BehaviorSet_Mark1( int bState )
{
	switch( bState )
	{
	case BS_DEFAULT:
	case BS_STAND_GUARD:
	case BS_PATROL:
		NPC_BSMark1_Default();
		break;
	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Mark2
-------------------------
*/
void NPC_BehaviorSet_Mark2( int bState )
{
	switch( bState )
	{
	case BS_DEFAULT:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
		NPC_BSMark2_Default();
		break;
	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_ATST
-------------------------
*/
void NPC_BehaviorSet_ATST( int bState )
{
	switch( bState )
	{
	case BS_DEFAULT:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
		NPC_BSATST_Default();
		break;
	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_MineMonster
-------------------------
*/
void NPC_BehaviorSet_MineMonster( int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSMineMonster_Default();
		break;
	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Howler
-------------------------
*/
void NPC_BehaviorSet_Howler( int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSHowler_Default();
		break;
	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Rancor
-------------------------
*/
void NPC_BehaviorSet_Rancor( int bState )
{
	switch( bState )
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSRancor_Default();
		break;
	default:
		NPC_BehaviorSet_Default( bState );
		break;
	}
}

void ST_SelectBestWeapon( void )
{
	if (NPCS.NPC->next_weapon_switch > level.time) return;

	if (!(NPCS.NPC->client->ps.eFlags & EF_FAKE_NPC_BOT)) return;

	if (!NPCS.NPC->enemy) return;

	NPC_ChangeWeapon( WP_BLASTER );
}

void Commando_SelectBestWeapon( void )
{
	if (NPCS.NPC->next_weapon_switch > level.time) return;

	if (!(NPCS.NPC->client->ps.eFlags & EF_FAKE_NPC_BOT)) return;

	if (!NPCS.NPC->enemy) return;

	if (NPCS.NPC->client->ps.weapon != WP_DISRUPTOR 
		&& DistanceSquared( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin )>(700*700) )
	{
		NPC_ChangeWeapon( WP_DISRUPTOR );
	}
	else if ( NPCS.NPC->client->ps.weapon != WP_ROCKET_LAUNCHER 
		&& DistanceSquared( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin )>(600*600) )
	{
		NPC_ChangeWeapon( WP_ROCKET_LAUNCHER );
	}
	else if (NPCS.NPC->client->ps.weapon != WP_BLASTER 
		&& DistanceSquared( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin )>(300*300) )
	{
		NPC_ChangeWeapon( WP_BLASTER );
	}
}

void Commando2_SelectBestWeapon( void )
{
	if (NPCS.NPC->next_weapon_switch > level.time) return;

	if (!NPCS.NPC->enemy) return;

	if (!(NPCS.NPC->client->ps.eFlags & EF_FAKE_NPC_BOT)) return;

	if (NPCS.NPC->client->ps.weapon != WP_DISRUPTOR 
		&& DistanceSquared( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin )>(700*700) )
	{
		NPC_ChangeWeapon( WP_DISRUPTOR );
	}
	else if ( NPCS.NPC->client->ps.weapon != WP_DEMP2 
		&& DistanceSquared( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin )>(500*500) )
	{
		NPC_ChangeWeapon( WP_DEMP2 );
	}
	else if (NPCS.NPC->client->ps.weapon != WP_BLASTER 
		&& DistanceSquared( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin )>(300*300) )
	{
		NPC_ChangeWeapon( WP_THERMAL );
	}
	else
	{
		NPC_ChangeWeapon( WP_BLASTER );
	}
}

void Sniper_SelectBestWeapon( void )
{
	if (NPCS.NPC->next_weapon_switch > level.time) return;

	if (!(NPCS.NPC->client->ps.eFlags & EF_FAKE_NPC_BOT)) return;

	if (!NPCS.NPC->enemy) return;

	if (NPCS.NPC->client->ps.weapon != WP_DISRUPTOR 
		&& DistanceSquared( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin )>(700*700) )
	{
		NPC_ChangeWeapon( WP_DISRUPTOR );
	}
	else if ( NPCS.NPC->client->ps.weapon != WP_BLASTER 
		&& DistanceSquared( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin )>(300*300) )
	{
		NPC_ChangeWeapon( WP_BLASTER );
	}
}

void Rocketer_SelectBestWeapon( void )
{
	if (NPCS.NPC->next_weapon_switch > level.time) return;

	if (!(NPCS.NPC->client->ps.eFlags & EF_FAKE_NPC_BOT)) return;

	if (!NPCS.NPC->enemy) return;

	if ( NPCS.NPC->client->ps.weapon != WP_ROCKET_LAUNCHER 
		&& DistanceSquared( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin )>(600*600) )
	{
		NPC_ChangeWeapon( WP_ROCKET_LAUNCHER );
	}
	else if ( NPCS.NPC->client->ps.weapon != WP_BLASTER 
		&& DistanceSquared( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin )>(300*300) )
	{
		NPC_ChangeWeapon( WP_BLASTER );
	}
}

/*
-------------------------
NPC_RunBehavior
-------------------------
*/
extern void NPC_BSEmplaced( void );
extern qboolean NPC_CheckSurrender( void );
extern void Boba_FlyStop( gentity_t *self );
extern void NPC_BSWampa_Default( void );
extern qboolean Jedi_CultistDestroyer( gentity_t *self );
void NPC_RunBehavior( int team, int bState )
{
	if (NPCS.NPC->s.NPC_class == CLASS_VEHICLE &&
		NPCS.NPC->m_pVehicle)
	{ //vehicles don't do AI!
		return;
	}

	if ( bState == BS_CINEMATIC )
	{
		NPC_BSCinematic();
	}
	else if ( NPCS.NPC->client->ps.weapon == WP_EMPLACED_GUN )
	{
		NPC_BSEmplaced();
		NPC_CheckCharmed();
		return;
	}
	else if ( NPCS.NPC->client->NPC_class == CLASS_JEDI 
		|| NPCS.NPC->client->NPC_class == CLASS_REBORN
		|| NPCS.NPC->client->NPC_class == CLASS_TAVION
		|| NPCS.NPC->client->NPC_class == CLASS_ALORA
		|| NPCS.NPC->client->NPC_class == CLASS_DESANN
		|| NPCS.NPC->client->NPC_class == CLASS_KYLE
		|| NPCS.NPC->client->NPC_class == CLASS_LUKE
		|| NPCS.NPC->client->NPC_class == CLASS_MORGANKATARN
		|| NPCS.NPC->client->NPC_class == CLASS_MONMOTHA
		|| NPCS.NPC->client->NPC_class == CLASS_SHADOWTROOPER
		|| NPCS.NPC->client->NPC_class == CLASS_JAN
		|| (NPCS.NPC->client->ps.eFlags & EF_FAKE_NPC_BOT) /* temporary */ )
	{//jedi
		NPC_BehaviorSet_Jedi( bState );
	}
	else if ( NPCS.NPC->client->NPC_class == CLASS_WAMPA )
	{//wampa
		NPC_BSWampa_Default();
	}
	else if ( NPCS.NPC->client->NPC_class == CLASS_RANCOR )
	{//rancor
		NPC_BehaviorSet_Rancor( bState );
	}
	else if ( NPCS.NPC->client->NPC_class == CLASS_REMOTE )
	{
		NPC_BehaviorSet_Remote( bState );
	}
	else if ( NPCS.NPC->client->NPC_class == CLASS_SEEKER )
	{
		NPC_BehaviorSet_Seeker( bState );
	}
	else if ( NPCS.NPC->client->NPC_class == CLASS_BOBAFETT )
	{//bounty hunter
		if ( Boba_Flying( NPCS.NPC ) )
		{
			NPC_BehaviorSet_Seeker(bState);
		}
		else
		{
			NPC_BehaviorSet_Jedi( bState );
		}
	}
	else if ( Jedi_CultistDestroyer( NPCS.NPC ) )
	{
		NPC_BSJedi_Default();
	}
	/*else if ( NPCS.NPCInfo->scriptFlags & SCF_FORCED_MARCH )
	{//being forced to march
		NPC_BSDefault();
	}*/
	else
	{
		switch( team )
		{
		// not sure if TEAM_ENEMY is appropriate here, I think I should be using NPC_class to check for behavior - dmv
		case NPCTEAM_ENEMY:
			// special cases for enemy droids
			switch( NPCS.NPC->client->NPC_class)
			{
			case CLASS_ATST:
				NPC_BehaviorSet_ATST( bState );
				return;
			case CLASS_PROBE:
				NPC_BehaviorSet_ImperialProbe(bState);
				return;
			case CLASS_REMOTE:
				NPC_BehaviorSet_Remote( bState );
				return;
			case CLASS_SENTRY:
				NPC_BehaviorSet_Sentry(bState);
				return;
			case CLASS_INTERROGATOR:
				NPC_BehaviorSet_Interrogator( bState );
				return;
			case CLASS_MINEMONSTER:
				NPC_BehaviorSet_MineMonster( bState );
				return;
			case CLASS_HOWLER:
				NPC_BehaviorSet_Howler( bState );
				return;
			case CLASS_MARK1:
				NPC_BehaviorSet_Mark1( bState );
				return;
			case CLASS_MARK2:
				NPC_BehaviorSet_Mark2( bState );
				return;
			case CLASS_GALAKMECH:
				NPC_BSGM_Default();
				return;
			case CLASS_TUSKEN:
				Sniper_SelectBestWeapon();

				if ( NPCS.NPC->client->ps.weapon == WP_DISRUPTOR && (NPCS.NPCInfo->scriptFlags & SCF_ALT_FIRE) )
				{//a sniper
					NPC_BehaviorSet_Sniper( bState );
					return;
				}
				else
				{
					NPC_BehaviorSet_Stormtrooper( bState );
					return;
				}
				return;
			case CLASS_SABOTEUR:
			case CLASS_HAZARD_TROOPER:
			case CLASS_REELO:
			case CLASS_COMMANDO:
				Commando2_SelectBestWeapon();

				if ( NPCS.NPC->client->ps.weapon == WP_DISRUPTOR && (NPCS.NPCInfo->scriptFlags & SCF_ALT_FIRE) )
				{//a sniper
					NPC_BehaviorSet_Sniper( bState );
					return;
				}
				else
				{
					NPC_BehaviorSet_Stormtrooper( bState );
					return;
				}
				return;
			case CLASS_ASSASSIN_DROID:
			case CLASS_IMPERIAL:
			case CLASS_RODIAN:
			case CLASS_TRANDOSHAN:
				Commando_SelectBestWeapon();

				if ( NPCS.NPC->client->ps.weapon == WP_DISRUPTOR && (NPCS.NPCInfo->scriptFlags & SCF_ALT_FIRE) )
				{//a sniper
					NPC_BehaviorSet_Sniper( bState );
					return;
				}
				else
				{
					NPC_BehaviorSet_Stormtrooper( bState );
					return;
				}
				return;
			case CLASS_NOGHRI:
			case CLASS_SABER_DROID:
				//NPC_BehaviorSet_Jedi( bState );
				NPC_BSJedi_Default();
				return;
			case CLASS_STORMTROOPER:
				ST_SelectBestWeapon();
				NPC_BehaviorSet_Stormtrooper( bState );
				return;
			case CLASS_ALORA:
				NPC_BehaviorSet_Jedi( bState );
				return;
			case CLASS_ROCKETTROOPER:
				Rocketer_SelectBestWeapon();
				NPC_BehaviorSet_Stormtrooper( bState );
				return;
			default:
				break;
			}

			/*if ( NPCS.NPC->enemy && NPCS.NPC->s.weapon == WP_NONE && bState != BS_HUNT_AND_KILL && !trap->ICARUS_TaskIDPending( (sharedEntity_t *)NPCS.NPC, TID_MOVE_NAV ) )
			{//if in battle and have no weapon, run away, fixme: when in BS_HUNT_AND_KILL, they just stand there
				if ( bState != BS_FLEE )
				{
					NPC_StartFlee( NPCS.NPC->enemy, NPCS.NPC->enemy->r.currentOrigin, AEL_DANGER_GREAT, 5000, 10000 );
				}
				else
				{
					NPC_BSFlee();
				}
				return;
			}*/
			/*else if ( NPCS.NPC->client->ps.weapon == WP_SABER )
			{//special melee exception
				NPC_BehaviorSet_Default( bState );
				return;
			}*/
			/*else*/ if ( NPCS.NPC->client->ps.weapon == WP_DISRUPTOR && (NPCS.NPCInfo->scriptFlags & SCF_ALT_FIRE) )
			{//a sniper
				NPC_BehaviorSet_Sniper( bState );
				return;
			}
			else if ( NPCS.NPC->client->ps.weapon == WP_THERMAL || NPCS.NPC->client->ps.weapon == WP_STUN_BATON )//FIXME: separate AI for melee fighters
			{//a grenadier
				NPC_BehaviorSet_Grenadier( bState );
				return;
			}
			else if ( NPCS.NPC->client->ps.weapon == WP_SABER )
			{//special melee exception
				//NPC_BehaviorSet_Default( bState );
				NPC_BehaviorSet_Jedi( bState );
				return;
			}
			else if ( NPC_CheckSurrender() )
			{
				return;
			}

			NPC_BehaviorSet_Stormtrooper( bState );
			break;

		case NPCTEAM_NEUTRAL:

			// special cases for enemy droids
			if ( NPCS.NPC->client->NPC_class == CLASS_PROTOCOL || NPCS.NPC->client->NPC_class == CLASS_UGNAUGHT ||
				NPCS.NPC->client->NPC_class == CLASS_JAWA)
			{
				NPC_BehaviorSet_Default(bState);
			}
			else if ( NPCS.NPC->client->NPC_class == CLASS_VEHICLE )
			{
				// TODO: Add vehicle behaviors here.
				NPC_UpdateAngles( qtrue, qtrue );//just face our spawn angles for now
			}
			else
			{
				// Just one of the average droids
				NPC_BehaviorSet_Droid( bState );
			}
			break;

		default:
			if ( NPCS.NPC->client->NPC_class == CLASS_SEEKER )
			{
				NPC_BehaviorSet_Seeker(bState);
			}
			else
			{
				if ( NPCS.NPCInfo->charmedTime > level.time )
				{
					NPC_BehaviorSet_Charmed( bState );
				}
				else
				{
					NPC_BehaviorSet_Default( bState );
				}
				NPC_CheckCharmed();
			}
			break;
		}
	}
}

/*
===============
NPC_ExecuteBState

  MCG

NPC Behavior state thinking

===============
*/
void NPC_ExecuteBState ( gentity_t *self)//, int msec )
{
	bState_t	bState;

	NPC_HandleAIFlags();

	//FIXME: these next three bits could be a function call, some sort of setup/cleanup func
	//Lookmode must be reset every think cycle
	if(NPCS.NPC->delayScriptTime && NPCS.NPC->delayScriptTime <= level.time)
	{
		G_ActivateBehavior( NPCS.NPC, BSET_DELAYED);
		NPCS.NPC->delayScriptTime = 0;
	}

	//Clear this and let bState set it itself, so it automatically handles changing bStates... but we need a set bState wrapper func
	NPCS.NPCInfo->combatMove = qfalse;

	//Execute our bState
	if(NPCS.NPCInfo->tempBehavior)
	{//Overrides normal behavior until cleared
		bState = NPCS.NPCInfo->tempBehavior;
	}
	else
	{
		if(!NPCS.NPCInfo->behaviorState)
			NPCS.NPCInfo->behaviorState = NPCS.NPCInfo->defaultBehavior;

		bState = NPCS.NPCInfo->behaviorState;
	}

	//Pick the proper bstate for us and run it
	NPC_RunBehavior( self->client->playerTeam, bState );


//	if(bState != BS_POINT_COMBAT && NPCInfo->combatPoint != -1)
//	{
		//level.combatPoints[NPCInfo->combatPoint].occupied = qfalse;
		//NPCInfo->combatPoint = -1;
//	}

	//Here we need to see what the scripted stuff told us to do
//Only process snapshot if independent and in combat mode- this would pick enemies and go after needed items
//	ProcessSnapshot();

//Ignore my needs if I'm under script control- this would set needs for items
//	CheckSelf();

	//Back to normal?  All decisions made?

	//FIXME: don't walk off ledges unless we can get to our goal faster that way, or that's our goal's surface
	//NPCPredict();

	if ( NPCS.NPC->enemy )
	{
		if ( !NPCS.NPC->enemy->inuse )
		{//just in case bState doesn't catch this
			G_ClearEnemy( NPCS.NPC );
		}
	}

	if ( NPCS.NPC->client->ps.saberLockTime && NPCS.NPC->client->ps.saberLockEnemy != ENTITYNUM_NONE )
	{
		NPC_SetLookTarget( NPCS.NPC, NPCS.NPC->client->ps.saberLockEnemy, level.time+1000 );
	}
	else if ( !NPC_CheckLookTarget( NPCS.NPC ) )
	{
		if ( NPCS.NPC->enemy )
		{
			NPC_SetLookTarget( NPCS.NPC, NPCS.NPC->enemy->s.number, 0 );
		}
	}

	if ( NPCS.NPC->enemy )
	{
		if(NPCS.NPC->enemy->flags & FL_DONT_SHOOT)
		{
			NPCS.ucmd.buttons &= ~BUTTON_ATTACK;
			NPCS.ucmd.buttons &= ~BUTTON_ALT_ATTACK;
		}
		else if ( NPCS.NPC->client->playerTeam != NPCTEAM_ENEMY && NPCS.NPC->enemy->NPC && (NPCS.NPC->enemy->NPC->surrenderTime > level.time || (NPCS.NPC->enemy->NPC->scriptFlags&SCF_FORCED_MARCH)) )
		{//don't shoot someone who's surrendering if you're a good guy
			NPCS.ucmd.buttons &= ~BUTTON_ATTACK;
			NPCS.ucmd.buttons &= ~BUTTON_ALT_ATTACK;
		}

		if(NPCS.client->ps.weaponstate == WEAPON_IDLE)
		{
			NPCS.client->ps.weaponstate = WEAPON_READY;
		}
	}
	else
	{
		if(NPCS.client->ps.weaponstate == WEAPON_READY)
		{
			NPCS.client->ps.weaponstate = WEAPON_IDLE;
		}
	}

	if(!(NPCS.ucmd.buttons & BUTTON_ATTACK) && NPCS.NPC->attackDebounceTime > level.time)
	{//We just shot but aren't still shooting, so hold the gun up for a while
		if(NPCS.client->ps.weapon == WP_SABER )
		{//One-handed
			NPC_SetAnim(NPCS.NPC,SETANIM_TORSO,TORSO_WEAPONREADY1,SETANIM_FLAG_NORMAL);
		}
		else if(NPCS.client->ps.weapon == WP_BRYAR_PISTOL)
		{//Sniper pose
			NPC_SetAnim(NPCS.NPC,SETANIM_TORSO,TORSO_WEAPONREADY3,SETANIM_FLAG_NORMAL);
		}
		/*//FIXME: What's the proper solution here?
		else
		{//heavy weapon
			NPC_SetAnim(NPC,SETANIM_TORSO,TORSO_WEAPONREADY3,SETANIM_FLAG_NORMAL);
		}
		*/
	}
	else if ( !NPCS.NPC->enemy )//HACK!
	{
//		if(client->ps.weapon != WP_TRICORDER)
		{
			if( NPCS.NPC->s.torsoAnim == TORSO_WEAPONREADY1 || NPCS.NPC->s.torsoAnim == TORSO_WEAPONREADY3 )
			{//we look ready for action, using one of the first 2 weapon, let's rest our weapon on our shoulder
				NPC_SetAnim(NPCS.NPC,SETANIM_TORSO,TORSO_WEAPONIDLE3,SETANIM_FLAG_NORMAL);
			}
		}
	}

	NPC_CheckAttackHold();
	NPC_ApplyScriptFlags();

	//cliff and wall avoidance
	NPC_AvoidWallsAndCliffs();

	// run the bot through the server like it was a real client
//=== Save the ucmd for the second no-think Pmove ============================
	NPCS.ucmd.serverTime = level.time - 50;
	memcpy( &NPCS.NPCInfo->last_ucmd, &NPCS.ucmd, sizeof( usercmd_t ) );
	if ( !NPCS.NPCInfo->attackHoldTime )
	{
		NPCS.NPCInfo->last_ucmd.buttons &= ~(BUTTON_ATTACK|BUTTON_ALT_ATTACK);//so we don't fire twice in one think
	}
//============================================================================
	NPC_CheckAttackScript();
	NPC_KeepCurrentFacing();

	if ( !NPCS.NPC->next_roff_time || NPCS.NPC->next_roff_time < level.time )
	{//If we were following a roff, we don't do normal pmoves.
		ClientThink( NPCS.NPC->s.number, &NPCS.ucmd );
	}
	else
	{
		NPC_ApplyRoff();
	}

	// end of thinking cleanup
	NPCS.NPCInfo->touchedByPlayer = NULL;

	NPC_CheckPlayerAim();
	NPC_CheckAllClear();

	/*if( ucmd.forwardmove || ucmd.rightmove )
	{
		int	i, la = -1, ta = -1;

		for(i = 0; i < MAX_ANIMATIONS; i++)
		{
			if( NPC->client->ps.legsAnim == i )
			{
				la = i;
			}

			if( NPC->client->ps.torsoAnim == i )
			{
				ta = i;
			}

			if(la != -1 && ta != -1)
			{
				break;
			}
		}

		if(la != -1 && ta != -1)
		{//FIXME: should never play same frame twice or restart an anim before finishing it
			Com_Printf("LegsAnim: %s(%d) TorsoAnim: %s(%d)\n", animTable[la].name, NPC->renderInfo.legsFrame, animTable[ta].name, NPC->client->renderInfo.torsoFrame);
		}
	}*/
}

void NPC_CheckInSolid(void)
{
	trace_t	trace;
	vec3_t	point;
	VectorCopy(NPCS.NPC->r.currentOrigin, point);
	point[2] -= 0.25;

	trap->Trace(&trace, NPCS.NPC->r.currentOrigin, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, point, NPCS.NPC->s.number, NPCS.NPC->clipmask, qfalse, 0, 0);
	if(!trace.startsolid && !trace.allsolid)
	{
		VectorCopy(NPCS.NPC->r.currentOrigin, NPCS.NPCInfo->lastClearOrigin);
	}
	else
	{
		if(VectorLengthSquared(NPCS.NPCInfo->lastClearOrigin))
		{
//			Com_Printf("%s stuck in solid at %s: fixing...\n", NPC->script_targetname, vtos(NPC->r.currentOrigin));
			G_SetOrigin(NPCS.NPC, NPCS.NPCInfo->lastClearOrigin);
			trap->LinkEntity((sharedEntity_t *)NPCS.NPC);
		}
	}
}

void G_DroidSounds( gentity_t *self )
{
	if ( self->client )
	{//make the noises
		if ( TIMER_Done( self, "patrolNoise" ) && !Q_irand( 0, 20 ) )
		{
			switch( self->client->NPC_class )
			{
			case CLASS_R2D2:				// droid
				G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/r2d2/misc/r2d2talk0%d.wav",Q_irand(1, 3)) );
				break;
			case CLASS_R5D2:				// droid
				G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/r5d2/misc/r5talk%d.wav",Q_irand(1, 4)) );
				break;
			case CLASS_PROBE:				// droid
				G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/probe/misc/probetalk%d.wav",Q_irand(1, 3)) );
				break;
			case CLASS_MOUSE:				// droid
				G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/mouse/misc/mousego%d.wav",Q_irand(1, 3)) );
				break;
			case CLASS_GONK:				// droid
				G_SoundOnEnt(self, CHAN_AUTO, va("sound/chars/gonk/misc/gonktalk%d.wav",Q_irand(1, 2)) );
				break;
			default:
				break;
			}
			TIMER_Set( self, "patrolNoise", Q_irand( 2000, 4000 ) );
		}
	}
}

qboolean NPC_IsCivilian(gentity_t *NPC)
{
	if (NPC->client->NPC_class == CLASS_CIVILIAN
		|| NPC->client->NPC_class == CLASS_CIVILIAN_R2D2
		|| NPC->client->NPC_class == CLASS_CIVILIAN_R5D2
		|| NPC->client->NPC_class == CLASS_CIVILIAN_PROTOCOL
		|| NPC->client->NPC_class == CLASS_CIVILIAN_WEEQUAY)
	{
		return qtrue;
	}

	return qfalse;
}

qboolean NPC_IsCivilianHumanoid(gentity_t *NPC)
{
	if (NPC->client->NPC_class == CLASS_CIVILIAN
		|| NPC->client->NPC_class == CLASS_CIVILIAN_WEEQUAY)
	{
		return qtrue;
	}

	return qfalse;
}

void NPC_PickRandomIdleAnimantionCivilian(gentity_t *NPC)
{
	int randAnim = irand(0,10);

	switch (randAnim)
	{
	case 0:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		break;
	case 1:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND4, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		break;
	case 2:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND6, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		break;
	case 3:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND8, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		break;
	case 4:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND9, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		break;
	case 5:
	case 6:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND9IDLE1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		break;
	case 7:
	case 8:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_GUARD_IDLE1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		break;
	case 9:
	default:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_GUARD_LOOKAROUND1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		break;
	}
}

void NPC_PickRandomIdleAnimantion(gentity_t *NPC)
{
	int randAnim = irand(0,10);

	if (!NPC || !NPC->client) return;

	if (NPC->enemy) return; // No idle anims when we got an enemy...

	switch (NPC->client->ps.legsAnim)
	{
		case BOTH_STAND1:
		case BOTH_STAND2:
		case BOTH_STAND3:
		case BOTH_STAND4:
		case BOTH_STAND5:
		case BOTH_STAND6:
		case BOTH_STAND8:
		case BOTH_STAND9:
		case BOTH_STAND10:
		case BOTH_STAND9IDLE1:
		case BOTH_GUARD_IDLE1:
		case BOTH_GUARD_LOOKAROUND1:
			// Check torso also...
			if (NPC->client->ps.torsoAnim == NPC->client->ps.legsAnim)
				return; // Already running an idle animation...
		break;
	default:
		break;
	}

	if (NPC->client->lookTime > level.time) return; // Wait before next anim...

	NPC->client->lookTime = level.time + irand(5000, 15000);

	if (NPC_IsCivilian(NPC))
	{
		NPC_PickRandomIdleAnimantionCivilian(NPC);
		return;
	}

	switch (randAnim)
	{
	case 0:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND3, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
	case 1:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND4, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
	case 2:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND6, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
	case 3:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND8, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
	case 4:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND9, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
	case 5:
	case 6:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND9IDLE1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		break;
	case 7:
	case 8:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_GUARD_IDLE1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		break;
	case 9:
	default:
		NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_GUARD_LOOKAROUND1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		break;
	}
}

qboolean NPC_SetCivilianMoveAnim( void )
{
	if (NPC_IsCivilianHumanoid(NPCS.NPC))
	{// Set better torso anims when not holding a weapon.
		if (NPCS.ucmd.forwardmove < 0) 
			NPC_SetAnim( NPCS.NPC, SETANIM_LEGS, BOTH_WALKBACK2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		else 
			NPC_SetAnim( NPCS.NPC, SETANIM_LEGS, BOTH_WALK2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );

		NPC_SetAnim(NPCS.NPC, SETANIM_TORSO, BOTH_STAND9IDLE1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);

		NPCS.NPC->client->ps.legsTimer = 200;
		NPCS.NPC->client->ps.torsoTimer = 200;

		//trap->Print("Civilian %s NPC anim set. Weapon %i.\n", NPCS.NPC->client->modelname, NPCS.NPC->client->ps.weapon);
		return qtrue;
	}

	return qfalse;
}

void NPC_SelectMoveAnimation(qboolean walk)
{
	if (NPCS.NPC->client->ps.crouchheight <= 0)
		NPCS.NPC->client->ps.crouchheight = CROUCH_MAXS_2;

	if (NPCS.NPC->client->ps.standheight <= 0)
		NPCS.NPC->client->ps.standheight = DEFAULT_MAXS_2;

	if (NPC_SetCivilianMoveAnim()) 
	{
		return;
	}

	if ((NPCS.ucmd.buttons & BUTTON_ATTACK) || (NPCS.ucmd.buttons & BUTTON_ALT_ATTACK)) 
	{
		return;
	}

	if (NPCS.ucmd.forwardmove == 0 
		&& NPCS.ucmd.rightmove == 0 
		&& NPCS.ucmd.upmove == 0
		&& VectorLength(NPCS.NPC->client->ps.velocity) < 4)
	{// Standing still...
		if (NPCS.NPC->client->ps.pm_flags & PMF_DUCKED)
		{
			NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_CROUCH1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		}
		else if ( NPCS.NPC->client->ps.eFlags2 & EF2_USE_ALT_ANIM )
		{//holding someone
			NPC_SetAnim( NPCS.NPC, SETANIM_BOTH, BOTH_STAND4, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}
		else if ( NPCS.NPC->client->ps.eFlags2 & EF2_ALERTED )
		{//have an enemy or have had one since we spawned
			NPC_SetAnim( NPCS.NPC, SETANIM_BOTH, BOTH_STAND2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}
		else
		{//just stand there
			//NPC_SetAnim( NPCS.NPC, SETANIM_BOTH, BOTH_STAND1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			NPC_PickRandomIdleAnimantion(NPCS.NPC);
		}

		NPCS.NPC->client->ps.torsoTimer = 200;
		NPCS.NPC->client->ps.legsTimer = 200;
	}
	else if (walk)
	{// Use walking anims..
		if (NPCS.ucmd.forwardmove < 0)
		{
			if (NPCS.NPC->client->ps.pm_flags & PMF_DUCKED)
			{
				NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_CROUCH1WALKBACK, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
			}
			else if (NPCS.NPC->client->ps.weapon == WP_SABER)
			{// Walk with saber...
				NPC_SetAnim( NPCS.NPC, SETANIM_LEGS, BOTH_WALKBACK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				NPC_SetAnim( NPCS.NPC, SETANIM_TORSO, BOTH_WALKBACK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			}
			else
			{// Standard walk anim..
				NPC_SetAnim( NPCS.NPC, SETANIM_LEGS, BOTH_WALKBACK2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				NPC_SetAnim( NPCS.NPC, SETANIM_TORSO, TORSO_WEAPONREADY3, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			}
			//trap->Print("Walking Back.\n");
		}
		else
		{
			if (NPCS.NPC->client->ps.pm_flags & PMF_DUCKED)
			{
				NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_CROUCH1WALK, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
			}
			else if (NPCS.NPC->client->ps.weapon == WP_SABER)
			{// Walk with saber...
				NPC_SetAnim( NPCS.NPC, SETANIM_LEGS, BOTH_WALK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				NPC_SetAnim( NPCS.NPC, SETANIM_TORSO, TORSO_WEAPONREADY3, SETANIM_FLAG_NORMAL );
			}
			else
			{// Standard walk anim..
				NPC_SetAnim( NPCS.NPC, SETANIM_LEGS, BOTH_WALK2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				NPC_SetAnim( NPCS.NPC, SETANIM_TORSO, TORSO_WEAPONREADY3, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			}

			//trap->Print("Walking Forward.\n");
		}

		NPCS.NPC->client->ps.torsoTimer = 200;
		NPCS.NPC->client->ps.legsTimer = 200;
	}
	else if ( NPCS.NPC->client->ps.eFlags2 & EF2_USE_ALT_ANIM )
	{//full on run, on all fours
		if (NPCS.NPC->client->ps.pm_flags & PMF_DUCKED)
		{
			NPC_SetAnim( NPCS.NPC, SETANIM_BOTH, BOTH_CROUCH1WALK, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
		}
		else 
		{
			NPC_SetAnim( NPCS.NPC, SETANIM_BOTH, BOTH_RUN1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}

		NPCS.NPC->client->ps.torsoTimer = 200;
		NPCS.NPC->client->ps.legsTimer = 200;
	}
	else
	{//regular, upright run
		if (NPCS.ucmd.forwardmove < 0)
		{
			if (NPCS.NPC->client->ps.pm_flags & PMF_DUCKED)
			{
				NPC_SetAnim(NPCS.NPC, SETANIM_BOTH, BOTH_CROUCH1WALKBACK, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
			}
			else 
			{
				NPC_SetAnim( NPCS.NPC, SETANIM_BOTH, BOTH_RUNBACK2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			}

			//trap->Print("Running Back.\n");
		}
		else
		{
			if (NPCS.NPC->client->ps.pm_flags & PMF_DUCKED)
			{
				NPC_SetAnim( NPCS.NPC, SETANIM_BOTH, BOTH_CROUCH1WALK, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
			}
			else 
			{
				NPC_SetAnim( NPCS.NPC, SETANIM_BOTH, BOTH_RUN2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			}

			//trap->Print("Running Forward.\n");
		}

		NPCS.NPC->client->ps.torsoTimer = 200;
		NPCS.NPC->client->ps.legsTimer = 200;
	}
}

qboolean DOM_NPC_ClearPathToSpot( gentity_t *NPC, vec3_t dest, int impactEntNum );
extern int DOM_GetRandomCloseVisibleWP(gentity_t *ent, vec3_t org, int ignoreEnt, int badwp);

int NPC_GetPatrolWP(gentity_t *NPC)
{
	int		i, NUM_FOUND = 0, FOUND_LIST[1024];
	float	PATROL_RANGE = NPC->patrol_range;
	float	flLen;

	for (i = 0; i < gWPNum; i++)
	{
		if (gWPArray[i] 
			&& gWPArray[i]->inuse 
			&& HeightDistance(gWPArray[i]->origin, NPC->r.currentOrigin) <= 32.0//24.0
			&& HeightDistance(gWPArray[i]->origin, NPC->spawn_pos) <= 32.0)//24.0)
		{
			vec3_t org, org2;

			flLen = Distance(NPC->r.currentOrigin, gWPArray[i]->origin);

			if (flLen < PATROL_RANGE && flLen >= PATROL_RANGE * 0.4/*128.0*/)
			{
				VectorCopy(NPC->r.currentOrigin, org);
				org[2]+=8;

				VectorCopy(gWPArray[i]->origin, org2);
				org2[2]+=8;

				if (DOM_NPC_ClearPathToSpot( NPC, org2/*gWPArray[i]->origin*/, NPC->s.number ))
				//if (OrgVisible(org, org2, NPC->s.number))
				{
					FOUND_LIST[NUM_FOUND] = i;
					NUM_FOUND++;

					if (NUM_FOUND > 1024) break; // hit max num...
				}
			}
		}
	}

	if (NUM_FOUND <= 0)
	{
		//trap->Print("No patrol point found.\n");
		return -1;
	}

	//trap->Print("Found %i patrol points.\n", NUM_FOUND);

	// Return a random one...
	return FOUND_LIST[Q_irand(0, NUM_FOUND)];
}

qboolean NPC_FindNewPatrolWaypoint()
{
	gentity_t *NPC = NPCS.NPC;

	if (NPC->noWaypointTime > level.time)
	{// Only try to find a new waypoint every 25 seconds...
		NPC_PickRandomIdleAnimantion(NPC);
		return qfalse;
	}

	//NPC->patrol_range = 512.0;
	NPC->patrol_range = 384.0;
	//NPC->patrol_range = 450.0;

	NPC->noWaypointTime = level.time + 15000 + irand (0, 15000); // 15 to 30 seconds before we try again... (it will run avoidance in the meantime)

	NPC->wpCurrent = NPC_GetPatrolWP(NPC);

	if (NPC->wpCurrent <= 0 || NPC->wpCurrent >= gWPNum)
	{
		NPC_PickRandomIdleAnimantion(NPC);
		return qfalse;
	}

	NPC->wpNext = NPC->wpCurrent;
	NPC->longTermGoal = NPC->wpCurrent;

	NPC->wpTravelTime = level.time + 10000;

	if (NPC->wpSeenTime < NPC->noWaypointTime)
		NPC->wpSeenTime = NPC->noWaypointTime; // also make sure we don't try to make a new route for the same length of time...

	//G_Printf("NPC Waypointing Debug: NPC %i [%s] (spawn pos %f %f %f) found a patrol waypoint for itself at %f %f %f (patrol range %f).", NPC->s.number, NPC->NPC_type, NPC->spawn_pos[0], NPC->spawn_pos[1], NPC->spawn_pos[2], gWPArray[NPC->wpCurrent]->origin[0], gWPArray[NPC->wpCurrent]->origin[1], gWPArray[NPC->wpCurrent]->origin[2], NPC->patrol_range);
	return qtrue; // all good, we have a new waypoint...
}

typedef enum
{// Avoidance methods...
	AVOIDANCE_NONE,
	AVOIDANCE_STRAFE_RIGHT,
	AVOIDANCE_STRAFE_LEFT,
	AVOIDANCE_STRAFE_CROUCH,
	AVOIDANCE_STRAFE_JUMP,
} avoidanceMethods_t;

vec3_t jumpLandPosition;

qboolean NPC_NeedJump()
{
	trace_t		tr;
	vec3_t		org1, org2;
	vec3_t		forward;
	gentity_t	*NPC = NPCS.NPC;

	VectorCopy(NPC->r.currentOrigin, org1);

	AngleVectors( NPC->r.currentAngles, forward, NULL, NULL );

	// Check jump...
	org1[2] += 8;
	forward[PITCH] = forward[ROLL] = 0;
	VectorMA( org1, 64, forward, org2 );

	if (NPC->waterlevel > 0)
	{// Always jump out of water...
		VectorCopy(org2, jumpLandPosition);
		return qtrue;
	}

	trap->Trace( &tr, org1, NULL, NULL, org2, NPC->s.number, MASK_PLAYERSOLID, 0, 0, 0 );

	if (tr.fraction < 1.0f)
	{// Looks like we might need to jump... Check if it would work...
		VectorCopy(NPC->r.currentOrigin, org1);
		org1[2] += 32;
		VectorMA( org1, 64, forward, org2 );
		trap->Trace( &tr, org1, NULL, NULL, org2, NPC->s.number, MASK_PLAYERSOLID, 0, 0, 0 );

		if (tr.fraction >= 0.7f)
		{// Close enough...
			//G_Printf("need jump");
			VectorCopy(org2, jumpLandPosition);
			return qtrue;
		}
	}

	return qfalse;
}

int NPC_SelectBestAvoidanceMethod()
{// Just find the largest visible distance direction...
	trace_t		tr;
	vec3_t		org1, org2;
	vec3_t		forward, right;
	int			i = 0;
	qboolean	SKIP_RIGHT = qfalse;
	qboolean	SKIP_LEFT = qfalse;
	gentity_t	*NPC = NPCS.NPC;

	if (NPC->bot_strafe_right_timer > level.time)
		return AVOIDANCE_STRAFE_RIGHT;

	if (NPC->bot_strafe_left_timer > level.time)
		return AVOIDANCE_STRAFE_LEFT;

	if (NPC->bot_strafe_crouch_timer > level.time)
		return AVOIDANCE_STRAFE_CROUCH;

	if (NPC->bot_strafe_jump_timer > level.time)
		return AVOIDANCE_STRAFE_JUMP;

	//if (NPC_FindTemporaryWaypoint())
	//	return AVOIDANCE_NONE;

	VectorSubtract( gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin, NPC->movedir );
	AngleVectors( NPC->move_vector, NPC->movedir, NULL, NULL );

	VectorCopy(NPC->r.currentOrigin, org1);
	org1[2] += STEPSIZE;

	VectorCopy(gWPArray[NPC->wpCurrent]->origin, org2);
	org2[2] += STEPSIZE;

	trap->Trace( &tr, org1, NPC->r.mins, NPC->r.maxs, org2, NPC->s.number, MASK_PLAYERSOLID, 0, 0, 0 );
		
	if (tr.fraction == 1.0f)
	{// It is accessable normally...
		return AVOIDANCE_NONE;
	}

	// OK, our waypoint is not accessable normally, we need to select a strafe direction...
	for (i = STEPSIZE; i <= STEPSIZE*4; i += STEPSIZE)
	{// First one to make it is the winner... The race is on!
		if (!SKIP_RIGHT)
		{// Check right side...
			VectorCopy(NPC->r.currentOrigin, org1);
			org1[2] += STEPSIZE;
			AngleVectors( NPC->move_vector, forward, right, NULL );
			VectorMA( org1, i, right, org1 );

			if (!OrgVisible(NPC->r.currentOrigin, org1, NPC->s.number)) 
				SKIP_RIGHT = qtrue;

			if (!SKIP_RIGHT)
			{
				trap->Trace( &tr, org1, NPC->r.mins, NPC->r.maxs, org2, NPC->s.number, MASK_PLAYERSOLID, 0, 0, 0 );
		
				if (tr.fraction == 1.0f)
				{
					//if (JKG_CheckBelowPoint(org1))
					//{
						return AVOIDANCE_STRAFE_RIGHT;
					//}
				}
			}
		}

		if (!SKIP_LEFT)
		{// Check left side...
			VectorCopy(NPC->r.currentOrigin, org1);
			org1[2] += STEPSIZE;
			AngleVectors( NPC->move_vector, forward, right, NULL );
			VectorMA( org1, 0 - i, right, org1 );
		
			if (!OrgVisible(NPC->r.currentOrigin, org1, NPC->s.number)) 
				SKIP_LEFT = qtrue;

			if (!SKIP_LEFT)
			{
				trap->Trace( &tr, org1, NPC->r.mins, NPC->r.maxs, org2, NPC->s.number, MASK_PLAYERSOLID, 0, 0, 0 );
		
				if (tr.fraction == 1.0f)
				{
					//if (JKG_CheckBelowPoint(org1))
					//{
						return AVOIDANCE_STRAFE_LEFT;
					//}
				}
			}
		}
	}

	return AVOIDANCE_NONE;
}

qboolean NPC_NPCBlockingPath()
{
	int			i, num;
	int			touch[MAX_GENTITIES];
	vec3_t		mins, maxs;
	vec3_t		range = { 128, 128, 128 };
	int			BEST_METHOD = AVOIDANCE_NONE;
	gentity_t	*NPC = NPCS.NPC;

	//VectorSubtract( gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin, NPC->movedir );
	//AngleVectors( NPC->move_vector, NPC->movedir, NULL, NULL );

	VectorSubtract( NPC->r.currentOrigin, range, mins );
	VectorAdd( NPC->r.currentOrigin, range, maxs );

	num = trap->EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i = 0; i < num; i++)
	{
		gentity_t *ent = &g_entities[touch[i]];

		if (!ent) continue;
		if (ent == NPC) continue; // UQ1: OLD JKG Mod was missing this :)
		if (ent->s.eType != ET_PLAYER && ent->s.eType != ET_NPC) continue;
		//if (Distance(ent->r.currentOrigin, NPC->r.currentOrigin) > 64) continue;

		//if (InFOV3( ent->r.currentOrigin, NPC->r.currentOrigin, NPC->move_vector, 90, 120 ))
		if (/*trap->InPVS(NPC->r.currentOrigin, ent->r.currentOrigin) &&*/ InFOV2(ent->r.currentOrigin, NPC, 90, 180))
		{
			NPC->bot_strafe_left_timer = level.time + 200;
			return qtrue;
		}
	}

	NPC->bot_strafe_left_timer = 0;
	NPC->bot_strafe_right_timer = 0;

	/*
	BEST_METHOD = NPC_SelectBestAvoidanceMethod();
	
	switch (BEST_METHOD)
	{
	case AVOIDANCE_STRAFE_RIGHT:
		NPC->bot_strafe_right_timer = level.time + 200;
		return qtrue;
		break;
	case AVOIDANCE_STRAFE_LEFT:
		NPC->bot_strafe_left_timer = level.time + 200;
		return qtrue;
		break;
	case AVOIDANCE_STRAFE_CROUCH:
		NPC->bot_strafe_crouch_timer = level.time + 200;
		break;
	case AVOIDANCE_STRAFE_JUMP:
		break;
	default:
		break;
	}
	*/

	return qfalse;
}

//Adjusts the moveDir to account for strafing
void NPC_AdjustforStrafe(vec3_t moveDir)
{
	vec3_t right, angles;
	gentity_t	*NPC = NPCS.NPC;

	if (!(NPC->bot_strafe_right_timer > level.time)
		&& !(NPC->bot_strafe_left_timer > level.time))
		return;

	if (NPC->bot_strafe_right_timer > level.time + 2000)
	{
		// Obviously incorrect...
		NPC->bot_strafe_right_timer = 0;
		return;
	}
	else if (NPC->bot_strafe_left_timer > level.time + 2000)
	{
		// Obviously incorrect...
		NPC->bot_strafe_left_timer = 0;
		return;
	}

	vectoangles(moveDir, angles);
	AngleVectors(angles, NULL, right, NULL);

	//flaten up/down
	right[2] = 0;

	if (NPC->bot_strafe_left_timer > level.time)
	{//strafing left
		VectorScale(right, -64, right);
	}
	else if (NPC->bot_strafe_right_timer > level.time)
	{//strafing right
		VectorScale(right, 64, right);
	}

	//We assume that moveDir has been normalized before this function.
	VectorAdd(moveDir, right, moveDir);
	VectorNormalize(moveDir);
}

//===========================================================================
// Routine      : UQ1_UcmdMoveForDir

// Description  : Set a valid ucmd move for the current move direction... A working one, unlike raven's joke...
void UQ1_UcmdMoveForDir ( gentity_t *self, usercmd_t *cmd, vec3_t dir, qboolean walk )
{
	vec3_t	forward, right, up;

	//float	speed = 127.0f;
	float	speed = 100.0f;
	//if (walk) speed = 64.0f;
	//if (walk) speed = 80.0f;
	//if (walk) speed = 48.0f;
	//if (walk) speed = 56.0f;
	if (walk) speed = 64.0f;

	AngleVectors( self->r.currentAngles, forward, right, up );

	dir[2] = 0;
	VectorNormalize( dir );

	cmd->forwardmove = DotProduct( forward, dir ) * speed;
	cmd->rightmove = DotProduct( right, dir ) * speed;

#ifdef __NPC_STRAFE__
	if (self->wpCurrent >= 0) NPC_NPCBlockingPath();
	//NPC_AdjustforStrafe(dir);
	if (self->bot_strafe_left_timer > level.time) cmd->rightmove -= 48.0;
#endif //__NPC_STRAFE__

	if (NPCS.NPC->s.eType == ET_PLAYER)
	{
		if (NPCS.ucmd.buttons & BUTTON_WALKING)
		{
			trap->EA_Action(NPCS.NPC->s.number, 0x0080000);
			trap->EA_Move(NPCS.NPC->s.number, dir, 5000.0);

			if (self->bot_strafe_left_timer > level.time)
				trap->EA_MoveLeft(NPCS.NPC->s.number);
		}
		else
		{
			trap->EA_Move(NPCS.NPC->s.number, dir, 5000.0);

			if (self->bot_strafe_left_timer > level.time)
				trap->EA_MoveLeft(NPCS.NPC->s.number);
		}
	}

	//cmd->upmove = abs(forward[3] ) * dir[3] * speed;
	/*if (NPCS.NPCInfo->jumpState == JS_CROUCHING || (self->NPC->scriptFlags & SCF_CROUCHED))
		cmd->upmove = -64.0;
	else if (NPCS.NPCInfo->jumpState == JS_JUMPING)
		cmd->upmove = 64.0;*/

	//NPC_SelectMoveAnimation(walk);

#ifdef __NPC_BBOX_ADJUST__
	// Adjust the NPC's bbox size to make it smaller and let it move around easier...
	if (self->r.maxs[0] > 0)
	{// UQ1: Assuming HUMANOID NPCs... Exceptions may need to be made...
		self->r.maxs[0] = 0;
		self->r.maxs[1] = 0;
	
		self->r.mins[0] = 0;
		self->r.mins[1] = 0;
		trap->LinkEntity((sharedEntity_t *)self);
	}
#endif //__NPC_BBOX_ADJUST__
}

qboolean NPC_OrgVisible(vec3_t org1, vec3_t org2, int ignore)
{
	trace_t tr;
	vec3_t	from, to;

	VectorCopy(org1, from);
	from[2] += 32;
	VectorCopy(org2, to);
	to[2] += 18;

	trap->Trace(&tr, from, NULL, NULL, to, ignore, MASK_SOLID, 0, 0, 0);

	if (tr.fraction == 1)
	{
		return qtrue;
	}

	return qfalse;
}

qboolean NPC_HaveValidEnemy( void )
{
	gentity_t	*NPC = NPCS.NPC;

	if (NPC->enemy 
		&& NPC->enemy->health > 0
		&& !(NPC->enemy->s.eFlags & EF_DEAD))
		return qtrue;

	return qfalse;
}

extern gentity_t *NPC_PickEnemyExt( qboolean checkAlerts );
extern qboolean NPC_MoveDirClear( int forwardmove, int rightmove, qboolean reset );
extern qboolean DOM_FakeNPC_Parse_UCMD (bot_state_t *bs, gentity_t *bot);
extern void G_UcmdMoveForDir( gentity_t *self, usercmd_t *cmd, vec3_t dir );

qboolean NPC_PatrolArea( void ) 
{// Quick method of patroling...
	gentity_t *NPC = NPCS.NPC;
	vec3_t		velocity_vec;
	float		velocity;
	qboolean	ENEMY_VISIBLE = qfalse;
	qboolean	HUNTING_ENEMY = qfalse;
	qboolean	FORCED_COVERSPOT_FIND = qfalse;

	if (gWPNum <= 0)
	{// No waypoints available...
		//trap->Print("PATROL: No waypoints.\n");
		NPC_PickRandomIdleAnimantion(NPC);
		return qfalse;
	}

	if (NPC->enemy)
	{// Chase them...
		//trap->Print("PATROL: Have enemy.\n");
		NPC->return_home = qtrue;
		return qfalse;
	}
	else if (NPC->return_home)
	{// Returning home after chase and kill...
		//trap->Print("PATROL: Return home.\n");
		return qfalse;
	}

	if ( !NPC->enemy )
	{
		switch (NPC->client->NPC_class)
		{
		case CLASS_CIVILIAN:
		case CLASS_CIVILIAN_R2D2:
		case CLASS_CIVILIAN_R5D2:
		case CLASS_CIVILIAN_PROTOCOL:
		case CLASS_CIVILIAN_WEEQUAY:
		case CLASS_GENERAL_VENDOR:
		case CLASS_WEAPONS_VENDOR:
		case CLASS_ARMOR_VENDOR:
		case CLASS_SUPPLIES_VENDOR:
		case CLASS_FOOD_VENDOR:
		case CLASS_MEDICAL_VENDOR:
		case CLASS_GAMBLER_VENDOR:
		case CLASS_TRADE_VENDOR:
		case CLASS_ODDITIES_VENDOR:
		case CLASS_DRUG_VENDOR:
		case CLASS_TRAVELLING_VENDOR:
			// These guys have no enemies...
			break;
		default:
			if ( NPC->client->enemyTeam != NPCTEAM_NEUTRAL )
			{
				NPC->enemy = NPC_PickEnemyExt( qtrue );

				if (NPC->enemy)
				{
					if (NPC->client->ps.weapon == WP_SABER)
						G_AddVoiceEvent( NPC, Q_irand( EV_JDETECTED1, EV_JDETECTED3 ), 15000 + irand(0, 30000) );
					else
					{
						G_AddVoiceEvent( NPC, Q_irand( EV_DETECTED1, EV_DETECTED5 ), 15000 + irand(0, 30000) );
					}
				}
			}
			break;
		}
	}

	if (NPC->NPC->conversationPartner)
	{// Chatting with another NPC... Stay still!
		NPC_NPCConversation();

		if (NPC->NPC->conversationPartner)
			NPC_FacePosition( NPC->NPC->conversationPartner->r.currentOrigin, qfalse );

		return qfalse;
	}

	if (!NPC->enemy && !NPC->NPC->conversationPartner)
	{// UQ1: Strange place to do this, but whatever... ;)
		NPC_FindConversationPartner();
	}

	if (NPC->NPC->conversationPartner)
	{// Chatting with another NPC... Stay still!
		NPC_FacePosition( NPC->NPC->conversationPartner->r.currentOrigin, qfalse );
		return qfalse;
	}

	VectorCopy(NPC->client->ps.velocity, velocity_vec);
	velocity = VectorLength(velocity_vec);

	if (!NPC->return_home
		&& (NPC->r.currentOrigin[2] > NPC->spawn_pos[2]+24 || NPC->r.currentOrigin[2] < NPC->spawn_pos[2]-24))
	{// We have fallen... Set this spot as our new patrol location...
		VectorCopy(NPC->r.currentOrigin, NPC->spawn_pos);
		NPC->longTermGoal = -1;
		NPC->wpCurrent = -1;
		NPC->pathsize = -1;
	}

	if (NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum || NPC->wpTravelTime < level.time)
	{// Patrol Point...
		NPC_FindNewPatrolWaypoint();
		NPC->return_home = qfalse;
		//trap->Print("PATROL: New Waypoint [%i].\n", NPC->wpCurrent);
		NPC_PickRandomIdleAnimantion(NPC);
		return qfalse; // next think...
	}

	if (Distance(gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin) < 64)
	{// We're at out goal! Find a new goal...
		NPC->longTermGoal = -1;
		NPC->wpCurrent = -1;
		NPC->pathsize = -1;
		//trap->Print("PATROL: Hit goal.\n");
		NPC_PickRandomIdleAnimantion(NPC);
		return qfalse; // next think...
	}

	if (NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum || NPC->longTermGoal < 0 || NPC->longTermGoal >= gWPNum)
	{// FIXME: Try to roam out of problems...
		//trap->Print("PATROL: Lost.\n");
		return qfalse; // next think...
	}

	NPC_FacePosition( gWPArray[NPC->wpCurrent]->origin, qfalse );
	VectorSubtract( gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin, NPC->movedir );
	UQ1_UcmdMoveForDir( NPC, &NPCS.ucmd, NPC->movedir, qtrue );
	VectorCopy( NPC->movedir, NPC->client->ps.moveDir );

	NPC_SelectMoveAnimation(qtrue);

	return qtrue;
}

/*///////////////////////////////////////////////////
NPC_GetNextNode
if the bot has reached a node, this function selects the next node
that he will go to, and returns it
right now it's being developed, feel free to experiment
*////////////////////////////////////////////////////

int NPC_GetNextNode(gentity_t *NPC)
{
	short int node = WAYPOINT_NONE;

	//we should never call this in BOTSTATE_MOVE with no goal
	//setup the goal/path in HandleIdleState
	if (NPC->longTermGoal == WAYPOINT_NONE)
	{
		return WAYPOINT_NONE;
	}

	if (NPC->pathsize <= 0)	//if the bot is at the end of his path, this shouldn't have been called
	{
		//NPC->longTermGoal = WAYPOINT_NONE;	//reset to having no goal
		return WAYPOINT_NONE;
	}

	node = NPC->pathlist[NPC->pathsize-1];	//pathlist is in reverse order
	NPC->pathsize--;	//mark that we've moved another node

	if (NPC->pathsize <= 0)
	{
		if (NPC->wpCurrent < 0)
		{
			NPC->wpCurrent = NPC->longTermGoal;
		}
		else
		{
			node = NPC->longTermGoal;
		}
	}
	return node;
}

//extern qboolean NPC_CoverpointVisible ( gentity_t *NPC, int coverWP );
extern int DOM_GetRandomCloseWP(vec3_t org, int badwp, int unused);
extern int DOM_GetNearestVisibleWP(vec3_t org, int ignore, int badwp);
extern int DOM_GetNearestVisibleWP_Goal(vec3_t org, int ignore, int badwp);
extern int DOM_GetNearestVisibleWP_NOBOX(vec3_t org, int ignore, int badwp);

qboolean NPC_FindNewWaypoint()
{
	gentity_t	*NPC = NPCS.NPC;

	//if (NPC->noWaypointTime > level.time)
	//{// Only try to find a new waypoint every 5 seconds...
	//	NPC_PickRandomIdleAnimantion(NPC);
	//	return qfalse;
	//}

	//NPC->wpCurrent = DOM_GetRandomCloseVisibleWP(NPC, NPC->r.currentOrigin, NPC->s.number, -1);
	NPC->wpCurrent = DOM_GetRandomCloseWP(NPC->r.currentOrigin, NPC->wpCurrent, -1);
	//NPC->noWaypointTime = level.time + 3000; // 3 seconds before we try again... (it will run avoidance in the meantime)

	//if (NPC->wpSeenTime < NPC->noWaypointTime)
	//	NPC->wpSeenTime = NPC->noWaypointTime; // also make sure we don't try to make a new route for the same length of time...

	if (NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum)
	{
		NPC_PickRandomIdleAnimantion(NPC);
		//G_Printf("NPC Waypointing Debug: NPC %i (%s) failed to find a waypoint for itself.", NPC->s.number, NPC->NPC_type);
		return qfalse; // failed... try again after som avoidance code...
	}

	return qtrue; // all good, we have a new waypoint...
}

void NPC_SetEnemyGoal()
{
	qboolean IS_COVERPOINT = qfalse;
	int			COVERPOINT_WP = -1;
	int			COVERPOINT_OFC_WP = -1;
	gentity_t	*NPC = NPCS.NPC;

	//if (NPC->wpSeenTime > level.time)
	//	return; // wait for next route creation...

	/*
	if (NPC->wpTravelTime < level.time)
		G_Printf("wp travel time\n");
	else 
		G_Printf("Bad wps (lt: %i) (ps: %i) (wc: %i) (wn: %i)\n", NPC->longTermGoal, NPC->pathsize, NPC->wpCurrent, NPC->wpNext);
	*/

	if (!NPC_FindNewWaypoint())
		return; // wait before trying to get a new waypoint...

	// UQ1: Gunner NPCs find cover...
	if (NPC->client->ps.weapon != WP_SABER)
	{// Should we find a cover point???
		if (NPC->enemy->wpCurrent <= 0 || NPC->enemy->wpCurrent < gWPNum)
		{// Find a new waypoint for them...
			NPC->enemy->wpCurrent = DOM_GetRandomCloseVisibleWP(NPC->enemy, NPC->enemy->r.currentOrigin, NPC->enemy->s.number, -1);
		}

		if (NPC->enemy->wpCurrent > 0 
			&& NPC->enemy->wpCurrent < gWPNum
			&& Distance(gWPArray[NPC->enemy->wpCurrent]->origin, NPC->enemy->r.currentOrigin) <= 256)
		{
			/*int i = 0;

			for (i = 0; i < num_cover_spots; i++)
			{
				qboolean BAD = qfalse;

				if (Distance(NPC->r.currentOrigin, gWPArray[cover_nodes[i]]->origin) <= 2048.0f
					&& Distance(NPC->enemy->r.currentOrigin, gWPArray[cover_nodes[i]]->origin) <= 2048.0f)
				{// Range looks good from both places...
					int thisWP = cover_nodes[i];
					
					// OK, looking good so far... Let's see how the visibility is...
					if (NPC_IsCoverpointFor(thisWP, NPC->enemy))
					{// Looks good for a cover point...
						int j = 0;
						int z = 0;

						for (z = 0; z < MAX_GENTITIES; z++)
						{// Now just check to make sure noone else is using it... 30 stormies behind a barrel anyone???
							gentity_t *ent = &g_entities[z];

							if (!ent) continue;
							if (!ent->inuse) continue;

							if (ent->coverpointGoal == thisWP
								|| ent->wpCurrent == thisWP
								|| ent->wpNext == thisWP)
							{// Meh, he already claimed it!
								BAD = qtrue;
								break;
							}
						}

						// Twas a stormie barrel... *sigh*
						if (BAD) continue;

						// So far, so good... Now check if a link from it can see the enemy.. (to dip in and out of cover to/from)
						for (j = 0; j < gWPArray[thisWP]->neighbornum; j++)
						{
							int lookWP = gWPArray[thisWP]->neighbors[j].num;

							if (!NPC_IsCoverpointFor(lookWP, NPC->enemy))
							{// Yes! Found one!
								COVERPOINT_WP = thisWP;
								COVERPOINT_OFC_WP = lookWP;
								IS_COVERPOINT = qtrue;
								break;
							}
						}

						if (IS_COVERPOINT) break; // We got one!
					}
				}

				if (IS_COVERPOINT) break; // We got one!
			}

			if (IS_COVERPOINT)
			{// WooHoo!!!! We got one! *dance*
				NPC->longTermGoal = NPC->coverpointGoal = COVERPOINT_WP;
				NPC->coverpointOFC = COVERPOINT_OFC_WP;
			}*/

			if (NPC->longTermGoal <= 0)
			{// Fallback...
				NPC->longTermGoal = DOM_GetRandomCloseVisibleWP(NPC->enemy, NPC->enemy->r.currentOrigin, NPC->enemy->s.number, -1);
			}
		}
		else
		{// Just head toward them....
			NPC->longTermGoal = DOM_GetRandomCloseVisibleWP(NPC->enemy, NPC->enemy->r.currentOrigin, NPC->enemy->s.number, -1);
		}
	}
	else
	{
		NPC->longTermGoal = DOM_GetRandomCloseVisibleWP(NPC->enemy, NPC->enemy->r.currentOrigin, NPC->enemy->s.number, -1);
	}

	if (NPC->longTermGoal > 0)
	{
		memset(NPC->pathlist, WAYPOINT_NONE, MAX_WPARRAY_SIZE);
		NPC->pathsize = ASTAR_FindPathFast(NPC->wpCurrent, NPC->longTermGoal, NPC->pathlist, qfalse);

		//if (NPC->pathsize <= 0) // Use the alternate (older) A* pathfinding code as alternative/fallback...
			//NPC->pathsize = DOM_FindIdealPathtoWP(NULL, NPC->wpCurrent, NPC->longTermGoal, -1, NPC->pathlist);
			//NPC->pathsize = ASTAR_FindPathFast(NPC->wpCurrent, NPC->longTermGoal, NPC->pathlist, qtrue);
		
		if (NPC->pathsize > 0)
		{
			/*
			if (NPC->enemy->s.eType == ET_PLAYER)
			{
				if (IS_COVERPOINT)
					G_Printf("NPC Waypointing Debug: NPC %i (%s) created a %i waypoint COVERPOINT path between waypoints %i and %i for enemy player %s.", NPC->s.number, NPC->NPC_type, NPC->pathsize, NPC->wpCurrent, NPC->longTermGoal, NPC->enemy->client->pers.netname);
				else
					G_Printf("NPC Waypointing Debug: NPC %i (%s) created a %i waypoint path between waypoints %i and %i for enemy player %s.", NPC->s.number, NPC->NPC_type, NPC->pathsize, NPC->wpCurrent, NPC->longTermGoal, NPC->enemy->client->pers.netname);
			}
			else
			{
				if (IS_COVERPOINT)
					G_Printf("NPC Waypointing Debug: NPC %i (%s) created a %i waypoint COVERPOINT path between waypoints %i and %i for enemy %s.", NPC->s.number, NPC->NPC_type, NPC->pathsize, NPC->wpCurrent, NPC->longTermGoal, NPC->enemy->classname);
				else
					G_Printf("NPC Waypointing Debug: NPC %i (%s) created a %i waypoint path between waypoints %i and %i for enemy %s.", NPC->s.number, NPC->NPC_type, NPC->pathsize, NPC->wpCurrent, NPC->longTermGoal, NPC->enemy->classname);
			}
			*/

			NPC->wpLast = NPC->wpCurrent;
			NPC->wpNext = NPC_GetNextNode(NPC);		//move to this node first, since it's where our path starts from

			//G_Printf("New: wps (lt: %i) (ps: %i) (wc: %i) (wn: %i)\n", NPC->longTermGoal, NPC->pathsize, NPC->wpCurrent, NPC->wpNext);

			if (NPC->client->ps.weapon == WP_SABER)
			{
				G_AddVoiceEvent( NPC, Q_irand( EV_JCHASE1, EV_JCHASE3 ), 15000 + irand(0, 30000) );
			}
			else
			{
				int choice = irand(0,13);

				switch (choice)
				{
				case 0:
					G_AddVoiceEvent( NPC, EV_OUTFLANK1, 15000 + irand(0, 30000) );
					break;
				case 1:
					G_AddVoiceEvent( NPC, EV_OUTFLANK2, 15000 + irand(0, 30000) );
					break;
				case 2:
					G_AddVoiceEvent( NPC, EV_CHASE1, 15000 + irand(0, 30000) );
					break;
				case 3:
					G_AddVoiceEvent( NPC, EV_CHASE2, 15000 + irand(0, 30000) );
					break;
				case 4:
					G_AddVoiceEvent( NPC, EV_CHASE3, 15000 + irand(0, 30000) );
					break;
				case 5:
					G_AddVoiceEvent( NPC, EV_COVER1, 15000 + irand(0, 30000) );
					break;
				case 6:
					G_AddVoiceEvent( NPC, EV_COVER2, 15000 + irand(0, 30000) );
					break;
				case 7:
					G_AddVoiceEvent( NPC, EV_COVER3, 15000 + irand(0, 30000) );
					break;
				case 8:
					G_AddVoiceEvent( NPC, EV_COVER4, 15000 + irand(0, 30000) );
					break;
				case 9:
					G_AddVoiceEvent( NPC, EV_COVER5, 15000 + irand(0, 30000) );
					break;
				case 10:
					G_AddVoiceEvent( NPC, EV_ESCAPING1, 15000 + irand(0, 30000) );
					break;
				case 11:
					G_AddVoiceEvent( NPC, EV_ESCAPING2, 15000 + irand(0, 30000) );
					break;
				case 12:
					G_AddVoiceEvent( NPC, EV_ESCAPING3, 15000 + irand(0, 30000) );
					break;
				default:
					G_AddVoiceEvent( NPC, EV_COVER5, 15000 + irand(0, 30000) );
					break;
				}
			}
		}
		else if (NPC->enemy->s.eType == ET_PLAYER)
		{
			//G_Printf("NPC Waypointing Debug: NPC %i (%s) failed to create a route between waypoints %i and %i for enemy player %s.", NPC->s.number, NPC->NPC_type, NPC->wpCurrent, NPC->longTermGoal, NPC->enemy->client->pers.netname);
			NPC->longTermGoal = NPC->coverpointOFC = NPC->coverpointGoal = -1;
			// Delay before next route creation...
			NPC->wpSeenTime = level.time + 2000;
			return;
		}
	}
	else
	{
		//if (NPC->enemy->s.eType == ET_PLAYER)
		//	G_Printf("NPC Waypointing Debug: NPC %i (%s) failed to find a waypoint for enemy player %s.", NPC->s.number, NPC->NPC_type, NPC->enemy->client->pers.netname);

		NPC->longTermGoal = NPC->coverpointOFC = NPC->coverpointGoal = -1;

		// Delay before next route creation...
		NPC->wpSeenTime = level.time + 2000;
		return;
	}

	// Delay before next route creation...
	NPC->wpSeenTime = level.time + 2000;
	// Delay before giving up on this new waypoint/route...
	NPC->wpTravelTime = level.time + 10000;
}

qboolean NPC_CopyPathFromNearbyNPC()
{
	gentity_t	*NPC = NPCS.NPC;
	int i = 0;

	for (i = MAX_CLIENTS; i < MAX_GENTITIES; i++)
	{
		gentity_t *test = &g_entities[i];

		if (i == NPC->s.number) continue;
		if (!test) continue;
		if (!test->inuse) continue;
		if (test->s.eType != ET_NPC) continue;
		if (test->pathsize <= 0) continue;
		if (test->client->NPC_class != NPC->client->NPC_class) continue; // Only copy from same NPC classes???
		if (Distance(NPC->r.currentOrigin, test->r.currentOrigin) > 128) continue;
		if (test->wpCurrent <= 0) continue;
		if (test->longTermGoal <= 0) continue;
		if (test->npc_dumb_route_time > level.time) continue;
		
		// Don't let them be copied again for 2 seconds...
		test->npc_dumb_route_time = level.time + 2000;

		// Seems we found one!
		memcpy(NPC->pathlist, test->pathlist, sizeof(int)*test->pathsize);
		NPC->pathsize = test->pathsize;
		NPC->wpCurrent = test->wpCurrent;
		NPC->wpNext = test->wpNext;
		NPC->wpLast = test->wpLast;
		NPC->longTermGoal = test->longTermGoal;
		
		// Delay before next route creation...
		NPC->wpSeenTime = level.time + 2000;
		// Delay before giving up on this new waypoint/route...
		NPC->wpTravelTime = level.time + 10000;
		
		// Don't let me be copied for 5 seconds...
		NPC->npc_dumb_route_time = level.time + 5000;

		//G_Printf("NPC Waypointing Debug: NPC %i (%s) copied a %i waypoint path between waypoints %i and %i from %i (%s).", NPC->s.number, NPC->NPC_type, NPC->pathsize, NPC->wpCurrent, NPC->longTermGoal, test->s.number, test->NPC_type);
		return qtrue;
	}

	return qfalse;
}

int NPC_FindGoal( gentity_t *NPC )
{
	int waypoint = irand(0, gWPNum-1);
	int tries = 0;

	while (gWPArray[waypoint]->inuse == qfalse || gWPArray[waypoint]->wpIsBad == qtrue)
	{
		if (tries > 10) return -1; // Try again next frame...

		waypoint = irand(0, gWPNum-1);
		tries++;
	}

	return waypoint;
}

void NPC_SetNewGoalAndPath()
{
	gentity_t	*NPC = NPCS.NPC;

	//if (NPC->client->NPC_class != CLASS_TRAVELLING_VENDOR)
	//	if (NPC_CopyPathFromNearbyNPC()) 
	//		return;

	if (NPC->wpSeenTime > level.time)
	{
		NPC_PickRandomIdleAnimantion(NPC);
		return; // wait for next route creation...
	}

	if (!NPC_FindNewWaypoint())
	{
		//trap->Print("Unable to find waypoint.\n");
		//player_die(NPC, NPC, NPC, 99999, MOD_CRUSH);
		return; // wait before trying to get a new waypoint...
	}

	if (NPC->return_home)
	{// Returning home...
		NPC->longTermGoal = DOM_GetRandomCloseVisibleWP(NPC, NPC->spawn_pos, NPC->s.number, -1);
	}
	else
	{// Find a new generic goal...
		NPC->longTermGoal = NPC_FindGoal( NPC );
	}

	if (NPC->longTermGoal >= 0)
	{
		memset(NPC->pathlist, WAYPOINT_NONE, sizeof(int)*MAX_WPARRAY_SIZE);
		NPC->pathsize = ASTAR_FindPathFast(NPC->wpCurrent, NPC->longTermGoal, NPC->pathlist, qtrue);

		//if (NPC->pathsize <= 0) // Use the alternate (older) A* pathfinding code as alternative/fallback...
		//	NPC->pathsize = DOM_FindIdealPathtoWP(NULL, NPC->wpCurrent, NPC->longTermGoal, -1, NPC->pathlist);

		if (NPC->pathsize > 0)
		{
			//G_Printf("NPC Waypointing Debug: NPC %i created a %i waypoint path for a random goal between waypoints %i and %i.", NPC->s.number, NPC->pathsize, NPC->wpCurrent, NPC->longTermGoal);
			NPC->wpLast = -1;
			NPC->wpNext = NPC_GetNextNode(NPC);		//move to this node first, since it's where our path starts from
		}
		else
		{
			//G_Printf("NPC Waypointing Debug: NPC %i failed to create a route between waypoints %i and %i.", NPC->s.number, NPC->wpCurrent, NPC->longTermGoal);
			// Delay before next route creation...
			NPC->wpSeenTime = level.time + 1000;//30000;
			NPC_PickRandomIdleAnimantion(NPC);
			return;
		}
	}
	else
	{
		//G_Printf("NPC Waypointing Debug: NPC %i failed to find a goal waypoint.", NPC->s.number);
		
		trap->Print("Unable to find goal waypoint.\n");

		// Delay before next route creation...
		NPC->wpSeenTime = level.time + 1000;//30000;
		NPC_PickRandomIdleAnimantion(NPC);
		return;
	}

	// Delay before next route creation...
	NPC->wpSeenTime = level.time + 1000;//30000;
	// Delay before giving up on this new waypoint/route...
	NPC->wpTravelTime = level.time + 10000;
}

/*
void NPC_SetNewWarzoneGoalAndPath()
{
	gentity_t	*NPC = NPCS.NPC;

	//if (NPC->client->NPC_class == CLASS_TRAVELLING_VENDOR)
	//{
	//	NPC_SetNewGoalAndPath(); // Use normal waypointing...
	//	return;
	//}

	if (NPC->wpSeenTime > level.time)
	{
		NPC_PickRandomIdleAnimantion(NPC);
		return; // wait for next route creation...
	}

	if (!NPC_FindNewWaypoint())
	{
		return; // wait before trying to get a new waypoint...
	}

	// Find a new warzone goal...
	NPC->longTermGoal = NPC_FindWarzoneGoal( NPC );

	if (NPC->longTermGoal <= 0) // Backup - Find a new generic goal...
		NPC->longTermGoal = NPC_FindGoal( NPC );

	if (NPC->longTermGoal >= 0)
	{
		memset(NPC->pathlist, WAYPOINT_NONE, sizeof(int)*MAX_WPARRAY_SIZE);
		NPC->pathsize = ASTAR_FindPathFast(NPC->wpCurrent, NPC->longTermGoal, NPC->pathlist, qtrue);

		if (NPC->pathsize > 0)
		{
			//G_Printf("NPC Waypointing Debug: NPC %i created a %i waypoint path for a random goal between waypoints %i and %i.", NPC->s.number, NPC->pathsize, NPC->wpCurrent, NPC->longTermGoal);
			NPC->wpLast = -1;
			NPC->wpNext = NPC_GetNextNode(NPC);		//move to this node first, since it's where our path starts from
		}
		else
		{
			//G_Printf("NPC Waypointing Debug: NPC %i failed to create a route between waypoints %i and %i.", NPC->s.number, NPC->wpCurrent, NPC->longTermGoal);
			// Delay before next route creation...
			NPC->wpSeenTime = level.time + 1000;//30000;
			NPC_PickRandomIdleAnimantion(NPC);
			return;
		}
	}
	else
	{
		//G_Printf("NPC Waypointing Debug: NPC %i failed to find a goal waypoint.", NPC->s.number);

		// Delay before next route creation...
		NPC->wpSeenTime = level.time + 1000;//30000;
		NPC_PickRandomIdleAnimantion(NPC);
		return;
	}

	// Delay before next route creation...
	NPC->wpSeenTime = level.time + 1000;//30000;
	// Delay before giving up on this new waypoint/route...
	//NPC->wpTravelTime = level.time + 10000;
}
*/

qboolean DOM_NPC_ClearPathToSpot( gentity_t *NPC, vec3_t dest, int impactEntNum )
{
	trace_t	trace;
	vec3_t	/*start, end, dir,*/ org, destorg;
//	float	dist, drop;
//	float	i;

	//Offset the step height
	//vec3_t	mins = {-18, -18, -24};
	//vec3_t	mins = {-8, -8, -6};
	vec3_t	mins = {-10, -10, -6};
	//vec3_t	maxs = {18, 18, 48};
	//vec3_t	maxs = {8, 8, NPC->client->ps.crouchheight};
	//vec3_t	maxs = {8, 8, 16};
	vec3_t	maxs = {10, 10, 16};

	VectorCopy(NPC->s.origin, org);
	//org[2]+=STEPSIZE;
	org[2]+=16;

	VectorCopy(dest, destorg);
	//destorg[2]+=STEPSIZE;
	destorg[2]+=16;

	trap->Trace( &trace, org, NULL/*mins*/, NULL/*maxs*/, destorg, NPC->s.number, MASK_PLAYERSOLID/*NPC->clipmask*/, 0, 0, 0 );

	//Do a simple check
	if ( trace.allsolid || trace.startsolid )
	{//inside solid
		//G_Printf("SOLID!\n");
		return qfalse;
	}

	/*
	if ( trace.fraction < 1.0f )
	{//hit something
		if ( (impactEntNum != ENTITYNUM_NONE && trace.entityNum == impactEntNum ))
		{//hit what we're going after
			//G_Printf("OK!\n");
			return qtrue;
		}
		else
		{
			//G_Printf("TRACE FAIL! - NPC %i hit entity %i (%s).\n", NPC->s.number, trace.entityNum, g_entities[trace.entityNum].classname);
			return qfalse;
		}
	}
	*/

	if ( trace.fraction < 1.0f )
		return qfalse;

	/*
	//otherwise, clear path in a straight line.  
	//Now at intervals of my size, go along the trace and trace down STEPSIZE to make sure there is a solid floor.
	VectorSubtract( dest, NPC->r.currentOrigin, dir );
	dist = VectorNormalize( dir );
	if ( dest[2] > NPC->r.currentOrigin[2] )
	{//going up, check for steps
		drop = STEPSIZE;
	}
	else
	{//going down or level, check for moderate drops
		drop = 64;
	}
	for ( i = NPC->r.maxs[0]*2; i < dist; i += NPC->r.maxs[0]*2 )
	{//FIXME: does this check the last spot, too?  We're assuming that should be okay since the enemy is there?
		VectorMA( NPC->r.currentOrigin, i, dir, start );
		VectorCopy( start, end );
		end[2] -= drop;
		trap->Trace( &trace, start, mins, NPC->r.maxs, end, NPC->s.number, NPC->clipmask );//NPC->r.mins?
		if ( trace.fraction < 1.0f || trace.allsolid || trace.startsolid )
		{//good to go
			continue;
		}
		G_Printf("FLOOR!\n");
		//no floor here! (or a long drop?)
		return qfalse;
	}*/
	//we made it!
	return qtrue;
}

qboolean NPC_ClearPathToJump( gentity_t *NPC, vec3_t dest, int impactEntNum )
{
	trace_t	trace;
	vec3_t	mins, start, end, dir;
	float	dist, drop;
	float	i;

	//Offset the step height
	VectorSet( mins, NPC->r.mins[0], NPC->r.mins[1], NPC->r.mins[2] + STEPSIZE );
	
	trap->Trace( &trace, NPC->r.currentOrigin, mins, NPC->r.maxs, dest, NPC->s.number, NPC->clipmask, 0, 0, 0 );

	//Do a simple check
	if ( trace.allsolid || trace.startsolid )
	{//inside solid
		return qfalse;
	}

	if ( trace.fraction < 1.0f )
	{//hit something
		if ( impactEntNum != ENTITYNUM_NONE && trace.entityNum == impactEntNum )
		{//hit what we're going after
			return qtrue;
		}
		else
		{
			return qfalse;
		}
	}

	//otherwise, clear path in a straight line.  
	//Now at intervals of my size, go along the trace and trace down STEPSIZE to make sure there is a solid floor.
	VectorSubtract( dest, NPC->r.currentOrigin, dir );
	dist = VectorNormalize( dir );
	if ( dest[2] > NPC->r.currentOrigin[2] )
	{//going up, check for steps
		drop = STEPSIZE;
	}
	else
	{//going down or level, check for moderate drops
		drop = 64;
	}
	for ( i = NPC->r.maxs[0]*2; i < dist; i += NPC->r.maxs[0]*2 )
	{//FIXME: does this check the last spot, too?  We're assuming that should be okay since the enemy is there?
		VectorMA( NPC->r.currentOrigin, i, dir, start );
		VectorCopy( start, end );
		end[2] -= drop;
		trap->Trace( &trace, start, mins, NPC->r.maxs, end, NPC->s.number, NPC->clipmask, 0, 0, 0 );//NPC->r.mins?
		if ( trace.fraction < 1.0f || trace.allsolid || trace.startsolid )
		{//good to go
			continue;
		}
		//no floor here! (or a long drop?)
		return qfalse;
	}
	//we made it!
	return qtrue;
}

//#define	APEX_HEIGHT		200.0f
#define	APEX_HEIGHT		128.0f
#define	PARA_WIDTH		128.0f
//#define	PARA_WIDTH		(sqrt(APEX_HEIGHT)+sqrt(APEX_HEIGHT))
//#define	JUMP_SPEED		200.0f
#define	JUMP_SPEED		128.0f

static qboolean NPC_Jump( gentity_t *NPC, vec3_t dest )
{//FIXME: if land on enemy, knock him down & jump off again
	if ( 1 )
	{
		float	targetDist, shotSpeed = 300, travelTime, impactDist, bestImpactDist = Q3_INFINITE;//fireSpeed, 
		vec3_t	targetDir, shotVel, failCase; 
		trace_t	trace;
		trajectory_t	tr;
		qboolean	blocked;
		int		elapsedTime, timeStep = 500, hitCount = 0, maxHits = 7;
		vec3_t	lastPos, testPos, bottom;

		while ( hitCount < maxHits )
		{
			VectorSubtract( dest, NPC->r.currentOrigin, targetDir );
			targetDist = VectorNormalize( targetDir );

			VectorScale( targetDir, shotSpeed, shotVel );
			travelTime = targetDist/shotSpeed;
			shotVel[2] += travelTime * 0.5 * NPC->client->ps.gravity;

			if ( !hitCount )		
			{//save the first one as the worst case scenario
				VectorCopy( shotVel, failCase );
			}

			if ( 1 )//tracePath )
			{//do a rough trace of the path
				blocked = qfalse;

				VectorCopy( NPC->r.currentOrigin, tr.trBase );
				VectorCopy( shotVel, tr.trDelta );
				tr.trType = TR_GRAVITY;
				tr.trTime = level.time;
				travelTime *= 1000.0f;
				VectorCopy( NPC->r.currentOrigin, lastPos );
				
				//This may be kind of wasteful, especially on long throws... use larger steps?  Divide the travelTime into a certain hard number of slices?  Trace just to apex and down?
				for ( elapsedTime = timeStep; elapsedTime < floor(travelTime)+timeStep; elapsedTime += timeStep )
				{
					if ( (float)elapsedTime > travelTime )
					{//cap it
						elapsedTime = floor( travelTime );
					}
					BG_EvaluateTrajectory( &tr, level.time + elapsedTime, testPos );
					if ( testPos[2] < lastPos[2] )
					{//going down, ignore botclip
						trap->Trace( &trace, lastPos, NPC->r.mins, NPC->r.maxs, testPos, NPC->s.number, NPC->clipmask, 0, 0, 0 );
					}
					else
					{//going up, check for botclip
						trap->Trace( &trace, lastPos, NPC->r.mins, NPC->r.maxs, testPos, NPC->s.number, NPC->clipmask|CONTENTS_BOTCLIP, 0, 0, 0 );
					}

					if ( trace.allsolid || trace.startsolid )
					{
						blocked = qtrue;
						break;
					}
					if ( trace.fraction < 1.0f )
					{//hit something
						if ( Distance( trace.endpos, dest ) < 96 )
						{//hit the spot, that's perfect!
							//Hmm, don't want to land on him, though...
							break;
						}
						else 
						{
							if ( trace.contents & CONTENTS_BOTCLIP )
							{//hit a do-not-enter brush
								blocked = qtrue;
								break;
							}
							if ( trace.plane.normal[2] > 0.7 && DistanceSquared( trace.endpos, dest ) < 4096 )//hit within 64 of desired location, should be okay
							{//close enough!
								break;
							}
							else
							{//FIXME: maybe find the extents of this brush and go above or below it on next try somehow?
								impactDist = DistanceSquared( trace.endpos, dest );
								if ( impactDist < bestImpactDist )
								{
									bestImpactDist = impactDist;
									VectorCopy( shotVel, failCase );
								}
								blocked = qtrue;
								break;
							}
						}
					}
					if ( elapsedTime == floor( travelTime ) )
					{//reached end, all clear
						if ( trace.fraction >= 1.0f )
						{//hmm, make sure we'll land on the ground...
							//FIXME: do we care how far below ourselves or our dest we'll land?
							VectorCopy( trace.endpos, bottom );
							bottom[2] -= 128;
							trap->Trace( &trace, trace.endpos, NPC->r.mins, NPC->r.maxs, bottom, NPC->s.number, NPC->clipmask, 0, 0, 0 );
							if ( trace.fraction >= 1.0f )
							{//would fall too far
								blocked = qtrue;
							}
						}
						break;
					}
					else
					{
						//all clear, try next slice
						VectorCopy( testPos, lastPos );
					}
				}
				if ( blocked )
				{//hit something, adjust speed (which will change arc)
					hitCount++;
					shotSpeed = 300 + ((hitCount-2) * 100);//from 100 to 900 (skipping 300)
					if ( hitCount >= 2 )
					{//skip 300 since that was the first value we tested
						shotSpeed += 100;
					}
				}
				else
				{//made it!
					break;
				}
			}
			else
			{//no need to check the path, go with first calc
				break;
			}
		}

		if ( hitCount >= maxHits )
		{//NOTE: worst case scenario, use the one that impacted closest to the target (or just use the first try...?)
			//NOTE: or try failcase?
			VectorCopy( failCase, NPC->client->ps.velocity );
		}
		VectorCopy( shotVel, NPC->client->ps.velocity );
	}
	else
	{//a more complicated jump
		vec3_t		dir, p1, p2, apex;
		float		time, height, forward, z, xy, dist, apexHeight;

		if ( NPC->r.currentOrigin[2] > dest[2] )//NPCInfo->goalEntity->r.currentOrigin
		{
			VectorCopy( NPC->r.currentOrigin, p1 );
			VectorCopy( dest, p2 );//NPCInfo->goalEntity->r.currentOrigin
		}
		else if ( NPC->r.currentOrigin[2] < dest[2] )//NPCInfo->goalEntity->r.currentOrigin
		{
			VectorCopy( dest, p1 );//NPCInfo->goalEntity->r.currentOrigin
			VectorCopy( NPC->r.currentOrigin, p2 );
		}
		else
		{
			VectorCopy( NPC->r.currentOrigin, p1 );
			VectorCopy( dest, p2 );//NPCInfo->goalEntity->r.currentOrigin
		}

		//z = xy*xy
		VectorSubtract( p2, p1, dir );
		dir[2] = 0;

		//Get xy and z diffs
		xy = VectorNormalize( dir );
		z = p1[2] - p2[2];

		apexHeight = APEX_HEIGHT/2;

		//Determine most desirable apex height
		//FIXME: length of xy will change curve of parabola, need to account for this
		//somewhere... PARA_WIDTH
		/*
		apexHeight = (APEX_HEIGHT * PARA_WIDTH/xy) + (APEX_HEIGHT * z/128);
		if ( apexHeight < APEX_HEIGHT * 0.5 )
		{
			apexHeight = APEX_HEIGHT*0.5;
		}
		else if ( apexHeight > APEX_HEIGHT * 2 )
		{
			apexHeight = APEX_HEIGHT*2;
		}
		*/

		z = (sqrt(apexHeight + z) - sqrt(apexHeight));

		assert(z >= 0);

//		Com_Printf("apex is %4.2f percent from p1: ", (xy-z)*0.5/xy*100.0f);

		xy -= z;
		xy *= 0.5;
		
		assert(xy > 0);

		VectorMA( p1, xy, dir, apex );
		apex[2] += apexHeight;

		VectorCopy(apex, NPC->pos1);
		
		//Now we have the apex, aim for it
		height = apex[2] - NPC->r.currentOrigin[2];
		time = sqrt( height / ( .5 * NPC->client->ps.gravity ) );//was 0.5, but didn't work well for very long jumps
		if ( !time ) 
		{
			//Com_Printf( S_COLOR_RED"ERROR: no time in jump\n" );
			return qfalse;
		}

		VectorSubtract ( apex, NPC->r.currentOrigin, NPC->client->ps.velocity );
		NPC->client->ps.velocity[2] = 0;
		dist = VectorNormalize( NPC->client->ps.velocity );

		forward = dist / time * 1.25;//er... probably bad, but...
		VectorScale( NPC->client->ps.velocity, forward, NPC->client->ps.velocity );

		//FIXME:  Uh.... should we trace/EvaluateTrajectory this to make sure we have clearance and we land where we want?
		NPC->client->ps.velocity[2] = time * NPC->client->ps.gravity;

		//Com_Printf("Jump Velocity: %4.2f, %4.2f, %4.2f\n", NPC->client->ps.velocity[0], NPC->client->ps.velocity[1], NPC->client->ps.velocity[2] );
	}
	return qtrue;
}

//extern void G_SoundOnEnt( gentity_t *ent, int channel, const char *soundPath );
extern qboolean PM_InKnockDown( playerState_t *ps );

static qboolean NPC_TryJump( gentity_t *NPC, vec3_t goal )
{//FIXME: never does a simple short, regular jump...
	usercmd_t	ucmd = NPCS.ucmd;

	if ( TIMER_Done( NPC, "jumpChaseDebounce" ) )
	{
		{
			if ( !PM_InKnockDown( &NPC->client->ps ) && !BG_InRoll( &NPC->client->ps, NPC->client->ps.legsAnim ) )
			{//enemy is on terra firma
				vec3_t goal_diff;
				float goal_z_diff;
				float goal_xy_dist;
				VectorSubtract( goal, NPC->r.currentOrigin, goal_diff );
				goal_z_diff = goal_diff[2];
				goal_diff[2] = 0;
				goal_xy_dist = VectorNormalize( goal_diff );
				if ( goal_xy_dist < 550 && goal_z_diff > -400/*was -256*/ )//for now, jedi don't take falling damage && (NPC->health > 20 || goal_z_diff > 0 ) && (NPC->health >= 100 || goal_z_diff > -128 ))//closer than @512
				{
					qboolean debounce = qfalse;
					if ( NPC->health < 150 && ((NPC->health < 30 && goal_z_diff < 0) || goal_z_diff < -128 ) )
					{//don't jump, just walk off... doesn't help with ledges, though
						debounce = qtrue;
					}
					else if ( goal_z_diff < 32 && goal_xy_dist < 200 )
					{//what is their ideal jump height?
						
						ucmd.upmove = 127;
						debounce = qtrue;
					}
					else
					{
						/*
						//NO!  All Jedi can jump-navigate now...
						if ( NPCInfo->rank != RANK_CREWMAN && NPCInfo->rank <= RANK_LT_JG )
						{//can't do acrobatics
							return qfalse;
						}
						*/
						if ( goal_z_diff > 0 || goal_xy_dist > 128 )
						{//Fake a force-jump
							//Screw it, just do my own calc & throw
							vec3_t dest;
							VectorCopy( goal, dest );
							
							{
								int	sideTry = 0;
								while( sideTry < 10 )
								{//FIXME: make it so it doesn't try the same spot again?
									trace_t	trace;
									vec3_t	bottom;

									VectorCopy( dest, bottom );
									bottom[2] -= 128;
									trap->Trace( &trace, dest, NPC->r.mins, NPC->r.maxs, bottom, NPC->s.number, NPC->clipmask, 0, 0, 0 );
									if ( trace.fraction < 1.0f )
									{//hit floor, okay to land here
										break;
									}
									sideTry++;
								}
								if ( sideTry >= 10 )
								{//screw it, just jump right at him?
									VectorCopy( goal, dest );
								}
							}

							if ( NPC_Jump( NPC, dest ) )
							{
								//Com_Printf( "(%d) pre-checked force jump\n", level.time );

								//FIXME: store the dir we;re going in in case something gets in the way of the jump?
								//? = vectoyaw( NPC->client->ps.velocity );
								/*
								if ( NPC->client->ps.velocity[2] < 320 )
								{
									NPC->client->ps.velocity[2] = 320;
								}
								else
								*/
								{//FIXME: make this a function call
									int jumpAnim;
									//FIXME: this should be more intelligent, like the normal force jump anim logic
									//if ( NPC->client->NPC_class == CLASS_BOBAFETT 
									//	||( NPCInfo->rank != RANK_CREWMAN && NPCInfo->rank <= RANK_LT_JG ) )
									//{//can't do acrobatics
									//	jumpAnim = BOTH_FORCEJUMP1;
									//}
									//else
									//{
										jumpAnim = BOTH_FLIP_F;
									//}
									G_SetAnim( NPC, &ucmd, SETANIM_BOTH, jumpAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
								}

								NPC->client->ps.fd.forceJumpZStart = NPC->r.currentOrigin[2];
								//NPC->client->ps.pm_flags |= PMF_JUMPING;

								NPC->client->ps.weaponTime = NPC->client->ps.torsoTimer;
								NPC->client->ps.fd.forcePowersActive |= ( 1 << FP_LEVITATION );
								
								//if ( NPC->client->NPC_class == CLASS_BOBAFETT )
								//{
								//	G_SoundOnEnt( NPC, CHAN_ITEM, "sound/boba/jeton.wav" );
								//	NPC->client->jetPackTime = level.time + Q_irand( 1000, 15000 + irand(0, 30000) );
								//}
								//else
								//{
								//	G_SoundOnEnt( NPC, CHAN_BODY, "sound/weapons/force/jump.wav" );
								//}

								TIMER_Set( NPC, "forceJumpChasing", Q_irand( 2000, 3000 ) );
								debounce = qtrue;
							}
						}
					}

					if ( debounce )
					{
						//Don't jump again for another 2 to 5 seconds
						TIMER_Set( NPC, "jumpChaseDebounce", Q_irand( 2000, 5000 ) );
						ucmd.forwardmove = 127;
						VectorClear( NPC->client->ps.moveDir );
						TIMER_Set( NPC, "duck", -level.time );
						return qtrue;
					}
				}
			}
		}
	}

	return qfalse;
}

void NPC_ClearPathData ( gentity_t *NPC )
{
	NPC->longTermGoal = -1;
	NPC->wpCurrent = -1;
	NPC->pathsize = -1;
	NPC->longTermGoal = NPC->coverpointOFC = NPC->coverpointGoal = -1;

	//NPC->wpSeenTime = 0;
}

//#define __OLD_NPC_WAYPOINTING__

qboolean NPC_FollowRoutes( void ) 
{// Quick method of following bot routes...
	gentity_t	*NPC = NPCS.NPC;
	usercmd_t	ucmd = NPCS.ucmd;
	float		wpDist = 0.0;

	if ( !NPC_HaveValidEnemy() )
	{
		switch (NPC->client->NPC_class)
		{
		case CLASS_CIVILIAN:
		case CLASS_CIVILIAN_R2D2:
		case CLASS_CIVILIAN_R5D2:
		case CLASS_CIVILIAN_PROTOCOL:
		case CLASS_CIVILIAN_WEEQUAY:
		case CLASS_GENERAL_VENDOR:
		case CLASS_WEAPONS_VENDOR:
		case CLASS_ARMOR_VENDOR:
		case CLASS_SUPPLIES_VENDOR:
		case CLASS_FOOD_VENDOR:
		case CLASS_MEDICAL_VENDOR:
		case CLASS_GAMBLER_VENDOR:
		case CLASS_TRADE_VENDOR:
		case CLASS_ODDITIES_VENDOR:
		case CLASS_DRUG_VENDOR:
		case CLASS_TRAVELLING_VENDOR:
			NPC->r.contents = 0;
			NPC->clipmask = MASK_NPCSOLID&~CONTENTS_BODY;
			// These guys have no enemies...
			break;
		default:
			if ( NPC->client->enemyTeam != NPCTEAM_NEUTRAL )
			{
				NPC->enemy = NPC_PickEnemyExt( qtrue );

				if (NPC_HaveValidEnemy())
				{
					if (NPC->client->ps.weapon == WP_SABER)
						G_AddVoiceEvent( NPC, Q_irand( EV_JDETECTED1, EV_JDETECTED3 ), 15000 + irand(0, 30000) );
					else
					{
						G_AddVoiceEvent( NPC, Q_irand( EV_DETECTED1, EV_DETECTED5 ), 15000 + irand(0, 30000) );
					}

					return qfalse;
				}
			}
			break;
		}
	}

	if (NPC->NPC->conversationPartner)
	{// Chatting with another NPC... Stay still!
		NPC_FacePosition( NPC->NPC->conversationPartner->r.currentOrigin, qfalse );
		NPC_NPCConversation();
		return qfalse;
	}

	if (!NPC->enemy && !NPC->NPC->conversationPartner)
	{// UQ1: Strange place to do this, but whatever... ;)
		NPC_FindConversationPartner();
	}

	if (NPC->NPC->conversationPartner)
	{// Chatting with another NPC... Stay still!
		NPC_FacePosition( NPC->NPC->conversationPartner->r.currentOrigin, qfalse );
		return qfalse;
	}

	G_ClearEnemy(NPC);

	if (VectorDistanceNoHeight(NPC->r.currentOrigin, NPC->npc_previous_pos) > 3)
	{
		NPC->last_move_time = level.time;
		VectorCopy(NPC->r.currentOrigin, NPC->npc_previous_pos);
	}

	if ( NPC->wpCurrent >= 0 && NPC->wpCurrent < gWPNum )
	{
		vec3_t upOrg, upOrg2;

		VectorCopy(NPC->r.currentOrigin, upOrg);
		upOrg[2]+=18;

		VectorCopy(gWPArray[NPC->wpCurrent]->origin, upOrg2);
		upOrg2[2]+=18;

		if (OrgVisible(upOrg, upOrg2, NPC->s.number))
		{
			NPC->wpSeenTime = level.time;
		}

		wpDist = Distance(gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin);
	}

	if (NPC->wpSeenTime >= level.time - 5000
		&& NPC->wpCurrent >= 0 
		&& NPC->wpCurrent < gWPNum
		&& wpDist > 512)
	{

	}
	else if ( NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum 
		|| NPC->longTermGoal < 0 || NPC->longTermGoal >= gWPNum 
		|| wpDist > 512
		|| NPC->wpSeenTime < level.time - 5000
		|| NPC->wpTravelTime < level.time 
		|| NPC->last_move_time < level.time - 5000 )
	{// We hit a problem in route, or don't have one yet.. Find a new goal and path...
		//if (wpDist > 512) trap->Print("wpCurrent too far.\n");
		//if (NPC->wpSeenTime < level.time - 5000) trap->Print("wpSeenTime.\n");
		//if (NPC->wpTravelTime < level.time) trap->Print("wpTravelTime.\n");
		//if (NPC->last_move_time < level.time - 5000) trap->Print("last_move_time.\n");

		NPC_ClearPathData(NPC);
		NPC_SetNewGoalAndPath();

		if (!(NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum || NPC->longTermGoal < 0 || NPC->longTermGoal >= gWPNum))
		{
			NPC->wpTravelTime = level.time + 10000;
			NPC->wpSeenTime = level.time;
			NPC->last_move_time = level.time;
		}
	}

	if (NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum || NPC->longTermGoal < 0 || NPC->longTermGoal >= gWPNum)
	{// FIXME: Try to roam out of problems...
		NPC_ClearPathData(NPC);
		ucmd.forwardmove = 0;
		ucmd.rightmove = 0;
		ucmd.upmove = 0;
		NPC_PickRandomIdleAnimantion(NPC);
		return qfalse; // next think...
	}

	if (VectorDistanceNoHeight(gWPArray[NPC->longTermGoal]->origin, NPC->r.currentOrigin) < 32)
	{// We're at out goal! Find a new goal...
		//trap->Print("HIT GOAL!\n");
		NPC_ClearPathData(NPC);
		ucmd.forwardmove = 0;
		ucmd.rightmove = 0;
		ucmd.upmove = 0;
		NPC_PickRandomIdleAnimantion(NPC);
		return qfalse; // next think...
	}

	if (VectorDistanceNoHeight(gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin) < 32)
	{// At current node.. Pick next in the list...
		//trap->Print("HIT WP %i. Next WP is %i.\n", NPC->wpCurrent, NPC->wpNext);

		NPC->wpLast = NPC->wpCurrent;
		NPC->wpCurrent = NPC->wpNext;
		NPC->wpNext = NPC_GetNextNode(NPC);

		if (NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum || NPC->longTermGoal < 0 || NPC->longTermGoal >= gWPNum)
		{// FIXME: Try to roam out of problems...
			NPC_ClearPathData(NPC);
			ucmd.forwardmove = 0;
			ucmd.rightmove = 0;
			ucmd.upmove = 0;
			NPC_PickRandomIdleAnimantion(NPC);
			return qfalse; // next think...
		}

		NPC->wpTravelTime = level.time + 10000;
		NPC->wpSeenTime = level.time;
	}

	NPC_FacePosition( gWPArray[NPC->wpCurrent]->origin, qfalse );
	VectorSubtract( gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin, NPC->movedir );
	
	if (g_gametype.integer == GT_WARZONE || (NPC->r.svFlags & SVF_BOT))
		UQ1_UcmdMoveForDir( NPC, &NPCS.ucmd, NPC->movedir, qfalse );
	else 
		UQ1_UcmdMoveForDir( NPC, &NPCS.ucmd, NPC->movedir, !NPC_HaveValidEnemy() );

	VectorCopy( NPC->movedir, NPC->client->ps.moveDir );

	if (g_gametype.integer == GT_WARZONE || (NPC->r.svFlags & SVF_BOT))
		NPC_SelectMoveAnimation(qfalse);
	else
		NPC_SelectMoveAnimation(!NPC_HaveValidEnemy());

	return qtrue;
}

void NPC_SetNewEnemyGoalAndPath()
{
	gentity_t	*NPC = NPCS.NPC;

	if (NPC->wpSeenTime > level.time)
	{
		NPC_PickRandomIdleAnimantion(NPC);
		return; // wait for next route creation...
	}

	if (!NPC_FindNewWaypoint())
	{
		return; // wait before trying to get a new waypoint...
	}

	//NPC->longTermGoal = DOM_GetRandomCloseVisibleWP(NPC, NPC->enemy->r.currentOrigin, NPC->s.number, -1);
	NPC->longTermGoal = DOM_GetRandomCloseWP(NPCS.NPCInfo->goalEntity->r.currentOrigin, NPC->wpCurrent, -1);

	if (NPC->longTermGoal >= 0)
	{
		memset(NPC->pathlist, WAYPOINT_NONE, sizeof(int)*MAX_WPARRAY_SIZE);
		NPC->pathsize = ASTAR_FindPathFast(NPC->wpCurrent, NPC->longTermGoal, NPC->pathlist, qtrue);

		if (NPC->pathsize > 0)
		{
			//G_Printf("NPC Waypointing Debug: NPC %i created a %i waypoint path for a random goal between waypoints %i and %i.", NPC->s.number, NPC->pathsize, NPC->wpCurrent, NPC->longTermGoal);
			NPC->wpLast = -1;
			NPC->wpNext = NPC_GetNextNode(NPC);		//move to this node first, since it's where our path starts from
		}
		else
		{
			//G_Printf("NPC Waypointing Debug: NPC %i failed to create a route between waypoints %i and %i.", NPC->s.number, NPC->wpCurrent, NPC->longTermGoal);
			// Delay before next route creation...
			NPC->wpSeenTime = level.time + 1000;//30000;
			NPC_PickRandomIdleAnimantion(NPC);
			return;
		}
	}
	else
	{
		//G_Printf("NPC Waypointing Debug: NPC %i failed to find a goal waypoint.", NPC->s.number);
		
		//trap->Print("Unable to find goal waypoint.\n");

		// Delay before next route creation...
		NPC->wpSeenTime = level.time + 1000;//30000;
		NPC_PickRandomIdleAnimantion(NPC);
		return;
	}

	// Delay before next route creation...
	NPC->wpSeenTime = level.time + 1000;//30000;
	// Delay before giving up on this new waypoint/route...
	NPC->wpTravelTime = level.time + 10000;
}

extern int jediSpeechDebounceTime[TEAM_NUM_TEAMS];//used to stop several jedi AI from speaking all at once
extern int groupSpeechDebounceTime[TEAM_NUM_TEAMS];//used to stop several group AI from speaking all at once

qboolean NPC_FollowEnemyRoute( void ) 
{// Quick method of following bot routes...
	gentity_t	*NPC = NPCS.NPC;
	usercmd_t	ucmd = NPCS.ucmd;

	if ( !NPC_HaveValidEnemy() )
	{
		return qfalse;
	}

	if ((NPC->client->ps.weapon == WP_SABER || NPC->client->ps.weapon == WP_MELEE)
		&& Distance(NPC->r.currentOrigin, NPCS.NPCInfo->goalEntity->r.currentOrigin) <= 48)
	{// Close enough already... Don't move...
		//trap->Print("close!\n");
		return qfalse;
	}
	else if ( !(NPC->client->ps.weapon == WP_SABER || NPC->client->ps.weapon == WP_MELEE)
		&& NPC_ClearLOS4( NPCS.NPCInfo->goalEntity ))
	{// Already visible to sloot... Don't move...
		//trap->Print("close wp!\n");
		return qfalse;
	}

	if (VectorDistanceNoHeight(NPC->r.currentOrigin, NPC->npc_previous_pos) > 3)
	{
		NPC->last_move_time = level.time;
		VectorCopy(NPC->r.currentOrigin, NPC->npc_previous_pos);
	}

	if (NPC->wpSeenTime > level.time)
	{

	}
	else if ( NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum 
		|| NPC->longTermGoal < 0 || NPC->longTermGoal >= gWPNum 
		|| Distance(gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin) > 512
		|| NPC->wpSeenTime < level.time - 5000
		|| NPC->wpTravelTime < level.time 
		|| NPC->last_move_time < level.time - 5000
		|| Distance(gWPArray[NPC->longTermGoal]->origin, NPC->enemy->r.currentOrigin) > 128.0)
	{// We hit a problem in route, or don't have one yet.. Find a new goal and path...
		NPC_ClearPathData(NPC);
		NPC_SetNewEnemyGoalAndPath();
		G_ClearEnemy(NPC); // UQ1: Give up...

		if (NPC_IsJedi(NPCS.NPC))
		{
			if ( !Q_irand( 0, 10 ) && NPCS.NPCInfo->blockedSpeechDebounceTime < level.time && jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] < level.time )
			{
				G_AddVoiceEvent( NPCS.NPC, Q_irand( EV_JLOST1, EV_JLOST3 ), 10000 );
				jediSpeechDebounceTime[NPCS.NPC->client->playerTeam] = NPCS.NPCInfo->blockedSpeechDebounceTime = level.time + 10000;
			}
		}
		else
		{
			if ( !Q_irand( 0, 10 ) && NPCS.NPCInfo->blockedSpeechDebounceTime < level.time && groupSpeechDebounceTime[NPCS.NPC->client->playerTeam] < level.time )
			{
				G_AddVoiceEvent( NPCS.NPC, Q_irand( EV_GIVEUP1, EV_GIVEUP4 ), 10000 );
				groupSpeechDebounceTime[NPCS.NPC->client->playerTeam] = NPCS.NPCInfo->blockedSpeechDebounceTime = level.time + 10000;
			}
		}

		if (!(NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum || NPC->longTermGoal < 0 || NPC->longTermGoal >= gWPNum))
		{
			NPC->wpTravelTime = level.time + 10000;
			NPC->wpSeenTime = level.time;
			NPC->last_move_time = level.time;
		}
	}

	if (NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum || NPC->longTermGoal < 0 || NPC->longTermGoal >= gWPNum)
	{// FIXME: Try to roam out of problems...
		NPC_ClearPathData(NPC);
		ucmd.forwardmove = 0;
		ucmd.rightmove = 0;
		ucmd.upmove = 0;
		NPC_PickRandomIdleAnimantion(NPC);
		//trap->Print("NO WP!\n");
		return qfalse; // next think...
	}

	if (VectorDistanceNoHeight(gWPArray[NPC->longTermGoal]->origin, NPC->r.currentOrigin) < 32)
	{// We're at out goal! Find a new goal...
		NPC_ClearPathData(NPC);
		ucmd.forwardmove = 0;
		ucmd.rightmove = 0;
		ucmd.upmove = 0;
		NPC_PickRandomIdleAnimantion(NPC);
		//trap->Print("AT DEST!\n");
		return qfalse; // next think...
	}

	if (VectorDistanceNoHeight(gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin) < 32)
	{// At current node.. Pick next in the list...
		NPC->wpLast = NPC->wpCurrent;
		NPC->wpCurrent = NPC->wpNext;
		NPC->wpNext = NPC_GetNextNode(NPC);

		if (NPC->wpCurrent < 0 || NPC->wpCurrent >= gWPNum || NPC->longTermGoal < 0 || NPC->longTermGoal >= gWPNum)
		{// FIXME: Try to roam out of problems...
			NPC_ClearPathData(NPC);
			ucmd.forwardmove = 0;
			ucmd.rightmove = 0;
			ucmd.upmove = 0;
			NPC_PickRandomIdleAnimantion(NPC);
			//trap->Print("????\n");
			return qfalse; // next think...
		}

		NPC->wpTravelTime = level.time + 10000;
		NPC->wpSeenTime = level.time;
	}

	{
		vec3_t upOrg, upOrg2;

		VectorCopy(NPC->r.currentOrigin, upOrg);
		upOrg[2]+=18;

		VectorCopy(gWPArray[NPC->wpCurrent]->origin, upOrg2);
		upOrg2[2]+=18;

		if (OrgVisible(upOrg, upOrg2, NPC->s.number))
		{
			NPC->wpSeenTime = level.time;
		}
	}

	NPC_FacePosition( gWPArray[NPC->wpCurrent]->origin, qfalse );
	VectorSubtract( gWPArray[NPC->wpCurrent]->origin, NPC->r.currentOrigin, NPC->movedir );
	UQ1_UcmdMoveForDir( NPC, &NPCS.ucmd, NPC->movedir, qfalse );
	VectorCopy( NPC->movedir, NPC->client->ps.moveDir );
	NPC_SelectMoveAnimation(qfalse);

	return qtrue;
}

/*
===============
NPC_Think

Main NPC AI - called once per frame
===============
*/
extern gentity_t *NPC_PickEnemyExt( qboolean checkAlerts );
extern qboolean NPC_FindEnemy( qboolean checkAlerts );

#if	AI_TIMERS
extern int AITime;
#endif//	AI_TIMERS
void NPC_Think ( gentity_t *self)//, int msec )
{
	vec3_t	oldMoveDir;
	int i = 0;
	//gentity_t *player;

	self->nextthink = level.time + FRAMETIME;

	SetNPCGlobals( self );

	//if (!(self->s.eFlags & EF_CLIENTSMOOTH)) self->s.eFlags |= EF_CLIENTSMOOTH;

	memset( &NPCS.ucmd, 0, sizeof( NPCS.ucmd ) );

	VectorCopy( self->client->ps.moveDir, oldMoveDir );

	// UQ1: Testing with this removed...
	if (self->s.NPC_class != CLASS_VEHICLE)
	{ //YOU ARE BREAKING MY PREDICTION. Bad clear.
		VectorClear( self->client->ps.moveDir );
	}

	if(!self || !self->NPC || !self->client)
	{
		return;
	}

	// dead NPCs have a special think, don't run scripts (for now)
	//FIXME: this breaks deathscripts
	if ( self->health <= 0 )
	{
		DeadThink();
		if ( NPCS.NPCInfo->nextBStateThink <= level.time )
		{
			trap->ICARUS_MaintainTaskManager(self->s.number);
		}
		VectorCopy(self->r.currentOrigin, self->client->ps.origin);
		return;
	}

	// see if NPC ai is frozen
	if ( d_npcfreeze.value || (NPCS.NPC->r.svFlags&SVF_ICARUS_FREEZE) )
	{
		NPC_UpdateAngles( qtrue, qtrue );
		ClientThink(self->s.number, &NPCS.ucmd);
		//VectorCopy(self->s.origin, self->s.origin2 );
		VectorCopy(self->r.currentOrigin, self->client->ps.origin);
		return;
	}

	self->nextthink = level.time + FRAMETIME/2;

	// UQ1: WTF - an empty loop??!?!?!?!?!?!!?!
/*
	while (i < MAX_CLIENTS)
	{
		player = &g_entities[i];

		if (player == self) continue;

		if (player->inuse && player->client && player->client->sess.sessionTeam != TEAM_SPECTATOR &&
			!(player->client->ps.pm_flags & PMF_FOLLOW))
		{
			//if ( player->client->ps.viewEntity == self->s.number )
			if (0) //rwwFIXMEFIXME: Allow controlling ents
			{//being controlled by player
				G_DroidSounds( self );
				//FIXME: might want to at least make sounds or something?
				//NPC_UpdateAngles(qtrue, qtrue);
				//Which ucmd should we send?  Does it matter, since it gets overridden anyway?
				NPCS.NPCInfo->last_ucmd.serverTime = level.time - 50;
				ClientThink( NPCS.NPC->s.number, &NPCS.ucmd );
				//VectorCopy(self->s.origin, self->s.origin2 );
				VectorCopy(self->r.currentOrigin, self->client->ps.origin);
				return;
			}
		}
		i++;
	}
*/

	if ( self->client->NPC_class == CLASS_VEHICLE)
	{
		if (self->client->ps.m_iVehicleNum)
		{//we don't think on our own
			//well, run scripts, though...
			trap->ICARUS_MaintainTaskManager(self->s.number);
			return;
		}
		else
		{
			VectorClear(self->client->ps.moveDir);
			self->client->pers.cmd.forwardmove = 0;
			self->client->pers.cmd.rightmove = 0;
			self->client->pers.cmd.upmove = 0;
			self->client->pers.cmd.buttons = 0;
			memcpy(&self->m_pVehicle->m_ucmd, &self->client->pers.cmd, sizeof(usercmd_t));
		}
	}
	else if ( NPCS.NPC->s.m_iVehicleNum )
	{//droid in a vehicle?
		G_DroidSounds( self );
	}

	if ( NPCS.NPCInfo->nextBStateThink <= level.time
		&& !NPCS.NPC->s.m_iVehicleNum )//NPCs sitting in Vehicles do NOTHING
	{
#if	AI_TIMERS
		int	startTime = GetTime(0);
#endif//	AI_TIMERS
		if ( NPCS.NPC->s.eType != ET_NPC && NPCS.NPC->s.eType != ET_PLAYER )
		{//Something drastic happened in our script
			return;
		}

		if ( NPCS.NPC->s.weapon == WP_SABER && g_npcspskill.integer >= 2 && NPCS.NPCInfo->rank > RANK_LT_JG )
		{//Jedi think faster on hard difficulty, except low-rank (reborn)
			NPCS.NPCInfo->nextBStateThink = level.time + FRAMETIME/2;
		}
		else
		{//Maybe even 200 ms?
			NPCS.NPCInfo->nextBStateThink = level.time + FRAMETIME;
		}

		memcpy( &NPCS.ucmd, &NPCS.NPCInfo->last_ucmd, sizeof( usercmd_t ) );
		NPCS.ucmd.buttons = 0; // init buttons...

		//nextthink is set before this so something in here can override it
		if (self->s.NPC_class != CLASS_VEHICLE ||
			!self->m_pVehicle)
		{ //ok, let's not do this at all for vehicles.
			if (self->enemy 
				&& (!NPC_IsValidNPCEnemy(self->enemy) || Distance(self->r.currentOrigin, self->enemy->r.currentOrigin) > 2048.0))
			{// If NPC Bot's enemy is invalid (eg: a dead NPC) or too far away, clear it!
				G_ClearEnemy(self);
			}

			if (!self->enemy)
			{
				NPC_FindEnemy( qtrue );
			}

			if ( self->enemy ) 
			{
				NPC_FaceEnemy( qtrue );
			}

			if (!self->enemy)
			{
				if (g_gametype.integer != GT_INSTANCE && g_gametype.integer != GT_SINGLE_PLAYER 
					&& (NPC_IsCivilian(self) || self->r.svFlags & SVF_BOT) // UQ1: Changed - only NPC bots and civilians roam now... Maybe add jedi/sith later...
					&& NPC_FollowRoutes()) 
				{
					//trap->Print("NPCBOT DEBUG: NPC is following routes.\n");

					if ( NPCS.client->ps.weaponstate == WEAPON_READY )
					{
						NPCS.client->ps.weaponstate = WEAPON_IDLE;
					}

					if ( NPCS.NPC->s.torsoAnim == TORSO_WEAPONREADY1 || NPCS.NPC->s.torsoAnim == TORSO_WEAPONREADY3 )
					{//we look ready for action, using one of the first 2 weapon, let's rest our weapon on our shoulder
						NPC_SetAnim(NPCS.NPC,SETANIM_TORSO,TORSO_WEAPONIDLE3,SETANIM_FLAG_NORMAL);
					}

					NPC_CheckAttackHold();
					NPC_ApplyScriptFlags();

					//cliff and wall avoidance
					NPC_AvoidWallsAndCliffs();

					// run the bot through the server like it was a real client
					//=== Save the ucmd for the second no-think Pmove ============================
					NPCS.ucmd.serverTime = level.time - 50;
					memcpy( &NPCS.NPCInfo->last_ucmd, &NPCS.ucmd, sizeof( usercmd_t ) );
					if ( !NPCS.NPCInfo->attackHoldTime )
					{
						NPCS.NPCInfo->last_ucmd.buttons &= ~(BUTTON_ATTACK|BUTTON_ALT_ATTACK);//so we don't fire twice in one think
					}
					//============================================================================
					NPC_CheckAttackScript();
					NPC_KeepCurrentFacing();

					if ( NPC_IsCivilianHumanoid(NPCS.NPC) )
					{// Set better torso anims when not holding a weapon.
						NPC_SetAnim(NPCS.NPC, SETANIM_TORSO, BOTH_STAND9IDLE1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
						NPCS.NPC->client->ps.torsoTimer = 200;
						//trap->Print(va("%s set torso anim.\n", NPCS.client->modelname));
					}

					if ( !NPCS.NPC->next_roff_time || NPCS.NPC->next_roff_time < level.time )
					{//If we were following a roff, we don't do normal pmoves.
						ClientThink( NPCS.NPC->s.number, &NPCS.ucmd );
					}
					else
					{
						NPC_ApplyRoff();
					}

					// end of thinking cleanup
					NPCS.NPCInfo->touchedByPlayer = NULL;

					NPC_CheckPlayerAim();
					NPC_CheckAllClear();
				}
				else if ((g_gametype.integer == GT_INSTANCE || g_gametype.integer == GT_SINGLE_PLAYER || !NPC_IsCivilian(self))
					&& NPC_PatrolArea())
				{
					//trap->Print("NPCBOT DEBUG: NPC is patroling.\n");

					if ( NPCS.client->ps.weaponstate == WEAPON_READY )
					{
						NPCS.client->ps.weaponstate = WEAPON_IDLE;
					}

					if ( NPCS.NPC->s.torsoAnim == TORSO_WEAPONREADY1 || NPCS.NPC->s.torsoAnim == TORSO_WEAPONREADY3 )
					{//we look ready for action, using one of the first 2 weapon, let's rest our weapon on our shoulder
						NPC_SetAnim(NPCS.NPC,SETANIM_TORSO,TORSO_WEAPONIDLE3,SETANIM_FLAG_NORMAL);
					}

					NPC_CheckAttackHold();
					NPC_ApplyScriptFlags();

					//cliff and wall avoidance
					NPC_AvoidWallsAndCliffs();

					// run the bot through the server like it was a real client
					//=== Save the ucmd for the second no-think Pmove ============================
					NPCS.ucmd.serverTime = level.time - 50;
					memcpy( &NPCS.NPCInfo->last_ucmd, &NPCS.ucmd, sizeof( usercmd_t ) );
					if ( !NPCS.NPCInfo->attackHoldTime )
					{
						NPCS.NPCInfo->last_ucmd.buttons &= ~(BUTTON_ATTACK|BUTTON_ALT_ATTACK);//so we don't fire twice in one think
					}
					//============================================================================
					NPC_CheckAttackScript();
					NPC_KeepCurrentFacing();

					if ( NPC_IsCivilianHumanoid(NPCS.NPC) )
					{// Set better torso anims when not holding a weapon.
						NPC_SetAnim(NPCS.NPC, SETANIM_TORSO, BOTH_STAND9IDLE1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
						NPCS.NPC->client->ps.torsoTimer = 200;
						//trap->Print(va("%s set torso anim.\n", NPCS.client->modelname));
					}

					if ( !NPCS.NPC->next_roff_time || NPCS.NPC->next_roff_time < level.time )
					{//If we were following a roff, we don't do normal pmoves.
						ClientThink( NPCS.NPC->s.number, &NPCS.ucmd );
					}
					else
					{
						NPC_ApplyRoff();
					}

					// end of thinking cleanup
					NPCS.NPCInfo->touchedByPlayer = NULL;

					NPC_CheckPlayerAim();
					NPC_CheckAllClear();
				}
				else
				{
					//trap->Print("NPCBOT DEBUG: NPC is stuck.\n");

					NPCS.ucmd.forwardmove = 0;
					NPCS.ucmd.rightmove = 0;
					NPCS.ucmd.upmove = 0;
					NPCS.ucmd.buttons = 0;

#ifdef __NPC_BBOX_ADJUST__
					// Adjust the NPC's bbox size to make it smaller and let it move around easier...
					if (self->r.maxs[0] != 8)
					{// UQ1: Assuming HUMANOID NPCs... Exceptions may need to be made...
						self->r.maxs[0] = 8;
						self->r.maxs[1] = 8;
	
						self->r.mins[0] = -8;
						self->r.mins[1] = -8;
						trap->LinkEntity((sharedEntity_t *)self);
					}
#endif //__NPC_BBOX_ADJUST__

					if (!NPCS.NPC->NPC->conversationPartner)
					{// Not chatting with another NPC... Set idle animation...
						NPC_PickRandomIdleAnimantion(NPCS.NPC);
					}

					if ( NPCS.client->ps.weaponstate == WEAPON_READY )
					{
						NPCS.client->ps.weaponstate = WEAPON_IDLE;
					}

					//if ( NPCS.NPC->s.torsoAnim == TORSO_WEAPONREADY1 || NPCS.NPC->s.torsoAnim == TORSO_WEAPONREADY3 )
					//{//we look ready for action, using one of the first 2 weapon, let's rest our weapon on our shoulder
					//	NPC_SetAnim(NPCS.NPC,SETANIM_TORSO,TORSO_WEAPONIDLE3,SETANIM_FLAG_NORMAL);
					//}

					NPC_CheckAttackHold();
					NPC_ApplyScriptFlags();

					//cliff and wall avoidance
					NPC_AvoidWallsAndCliffs();

					// run the bot through the server like it was a real client
					//=== Save the ucmd for the second no-think Pmove ============================
					NPCS.ucmd.serverTime = level.time - 50;
					memcpy( &NPCS.NPCInfo->last_ucmd, &NPCS.ucmd, sizeof( usercmd_t ) );
					if ( !NPCS.NPCInfo->attackHoldTime )
					{
						NPCS.NPCInfo->last_ucmd.buttons &= ~(BUTTON_ATTACK|BUTTON_ALT_ATTACK);//so we don't fire twice in one think
					}
					//============================================================================
					NPC_CheckAttackScript();
					NPC_KeepCurrentFacing();

					if ( !NPCS.NPC->next_roff_time || NPCS.NPC->next_roff_time < level.time )
					{//If we were following a roff, we don't do normal pmoves.
						ClientThink( NPCS.NPC->s.number, &NPCS.ucmd );
					}
					else
					{
						NPC_ApplyRoff();
					}

					// end of thinking cleanup
					NPCS.NPCInfo->touchedByPlayer = NULL;

					NPC_CheckPlayerAim();
					NPC_CheckAllClear();
				}
			}
			else
			{
				//trap->Print("NPCBOT DEBUG: NPC is attacking.\n");

#ifdef __NPC_BBOX_ADJUST__
				// Adjust the NPC's bbox size to make it smaller and let it move around easier...
				if (self->r.maxs[0] != 8)
				{// UQ1: Assuming HUMANOID NPCs... Exceptions may need to be made...
					self->r.maxs[0] = 8;
					self->r.maxs[1] = 8;
	
					self->r.mins[0] = -8;
					self->r.mins[1] = -8;
					trap->LinkEntity((sharedEntity_t *)self);
				}
#endif //__NPC_BBOX_ADJUST__

				NPC_ExecuteBState( self );

				if (self->enemy 
					&& NPC_IsJedi(self) 
					&& Distance(self->r.currentOrigin, self->enemy->r.currentOrigin) > 64)
				{
					// UQ1: Always force move to any goal they might have...
					NPCS.NPCInfo->goalEntity = self->enemy;
					
					if (UpdateGoal())
						NPC_MoveToGoal( qtrue );
				}
				else if (self->enemy 
					&& Distance(self->r.currentOrigin, self->enemy->r.currentOrigin) > 64
					&& NPC_CheckVisibility ( NPCS.NPC->enemy, CHECK_360|CHECK_VISRANGE ) < VIS_FOV)
				{
					// UQ1: Enemy is not in our view, move toward it...
					NPCS.NPCInfo->goalEntity = self->enemy;
					
					if (UpdateGoal())
						if (!NPC_MoveToGoal( qfalse ))
							NPC_MoveToGoal( qtrue );
				}
			}
		}
		else
		{
			//trap->Print("NPCBOT DEBUG: NPC is movetogoal.\n");
			// UQ1: Always force move to any goal they might have...
			NPC_MoveToGoal( qtrue );
		}

#if	AI_TIMERS
		int addTime = GetTime( startTime );
		if ( addTime > 50 )
		{
			Com_Printf( S_COLOR_RED"ERROR: NPC number %d, %s %s at %s, weaponnum: %d, using %d of AI time!!!\n", NPC->s.number, NPC->NPC_type, NPC->targetname, vtos(NPC->r.currentOrigin), NPC->s.weapon, addTime );
		}
		AITime += addTime;
#endif//	AI_TIMERS

	}
	else
	{
		VectorCopy( oldMoveDir, self->client->ps.moveDir );

		//or use client->pers.lastCommand?
		NPCS.NPCInfo->last_ucmd.serverTime = level.time - 50;
		if ( !NPCS.NPC->next_roff_time || NPCS.NPC->next_roff_time < level.time )
		{//If we were following a roff, we don't do normal pmoves.
			//FIXME: firing angles (no aim offset) or regular angles?
			if (self->enemy) NPC_UpdateAngles(qtrue, qtrue);
			memcpy( &NPCS.ucmd, &NPCS.NPCInfo->last_ucmd, sizeof( usercmd_t ) );
			//UQ1_UcmdMoveForDir( self, &NPCS.ucmd, self->movedir, ( NPCS.ucmd.buttons & BUTTON_WALKING ) );
			ClientThink(NPCS.NPC->s.number, &NPCS.ucmd);
		}
		else
		{
			//UQ1_UcmdMoveForDir( self, &NPCS.ucmd, self->movedir, ( NPCS.ucmd.buttons & BUTTON_WALKING ) );
			NPC_ApplyRoff();
		}
		//VectorCopy(self->s.origin, self->s.origin2 );
	}

	//must update icarus *every* frame because of certain animation completions in the pmove stuff that can leave a 50ms gap between ICARUS animation commands
	trap->ICARUS_MaintainTaskManager(self->s.number);

	VectorCopy(self->r.currentOrigin, self->client->ps.origin);
}

void NPC_InitAI ( void )
{
	/*
	trap->Cvar_Register(&g_saberRealisticCombat, "g_saberRealisticCombat", "0", CVAR_CHEAT);

	trap->Cvar_Register(&debugNoRoam, "d_noroam", "0", CVAR_CHEAT);
	trap->Cvar_Register(&debugNPCAimingBeam, "d_npcaiming", "0", CVAR_CHEAT);
	trap->Cvar_Register(&debugBreak, "d_break", "0", CVAR_CHEAT);
	trap->Cvar_Register(&d_npcai, "d_npcai", "0", CVAR_CHEAT);
	trap->Cvar_Register(&debugNPCFreeze, "d_npcfreeze", "0", CVAR_CHEAT);
	trap->Cvar_Register(&d_JediAI, "d_JediAI", "0", CVAR_CHEAT);
	trap->Cvar_Register(&d_noGroupAI, "d_noGroupAI", "0", CVAR_CHEAT);
	trap->Cvar_Register(&d_asynchronousGroupAI, "d_asynchronousGroupAI", "0", CVAR_CHEAT);

	//0 = never (BORING)
	//1 = kyle only
	//2 = kyle and last enemy jedi
	//3 = kyle and any enemy jedi
	//4 = kyle and last enemy in a group
	//5 = kyle and any enemy
	//6 = also when kyle takes pain or enemy jedi dodges player saber swing or does an acrobatic evasion

	trap->Cvar_Register(&d_slowmodeath, "d_slowmodeath", "0", CVAR_CHEAT);

	trap->Cvar_Register(&d_saberCombat, "d_saberCombat", "0", CVAR_CHEAT);

	trap->Cvar_Register(&g_npcspskill, "g_npcspskill", "0", CVAR_ARCHIVE | CVAR_USERINFO);
	*/
}

/*
==================================
void NPC_InitAnimTable( void )

  Need to initialize this table.
  If someone tried to play an anim
  before table is filled in with
  values, causes tasks that wait for
  anim completion to never finish.
  (frameLerp of 0 * numFrames of 0 = 0)
==================================
*/
/*
void NPC_InitAnimTable( void )
{
	int i;

	for ( i = 0; i < MAX_ANIM_FILES; i++ )
	{
		for ( int j = 0; j < MAX_ANIMATIONS; j++ )
		{
			level.knownAnimFileSets[i].animations[j].firstFrame = 0;
			level.knownAnimFileSets[i].animations[j].frameLerp = 100;
			level.knownAnimFileSets[i].animations[j].initialLerp = 100;
			level.knownAnimFileSets[i].animations[j].numFrames = 0;
		}
	}
}
*/

#ifdef __DOMINANCE_AI__
extern void NPC_ShadowTrooper_Precache( void );
extern void NPC_Gonk_Precache( void );
extern void NPC_Mouse_Precache( void );
extern void NPC_Seeker_Precache( void );
extern void NPC_Remote_Precache( void );
extern void	NPC_R2D2_Precache(void);
extern void	NPC_R5D2_Precache(void);
extern void NPC_Probe_Precache(void);
extern void NPC_Interrogator_Precache(gentity_t *self);
extern void NPC_MineMonster_Precache( void );
extern void NPC_Howler_Precache( void );
extern void NPC_ATST_Precache(void);
extern void NPC_Sentry_Precache(void);
extern void NPC_Mark1_Precache(void);
extern void NPC_Mark2_Precache(void);
extern void NPC_GalakMech_Precache( void );
extern void NPC_GalakMech_Init( gentity_t *ent );
extern void NPC_Protocol_Precache( void );
extern void Boba_Precache( void );
extern void NPC_Wampa_Precache( void );
#endif //__DOMINANCE_AI__

void NPC_InitGame( void )
{
//	globals.NPCs = (gNPC_t *) trap->TagMalloc(game.maxclients * sizeof(game.bots[0]), TAG_GAME);
//	trap->Cvar_Register(&debugNPCName, "d_npc", "0", CVAR_CHEAT);

	NPC_LoadParms();
	NPC_InitAI();
//	NPC_InitAnimTable();
	/*
	ResetTeamCounters();
	for ( int team = NPCTEAM_FREE; team < NPCTEAM_NUM_TEAMS; team++ )
	{
		teamLastEnemyTime[team] = -10000;
	}
	*/

	
#ifdef __DOMINANCE_AI__
	NPC_ShadowTrooper_Precache();
	NPC_Gonk_Precache();
	NPC_Mouse_Precache();
	NPC_Seeker_Precache();
	NPC_Remote_Precache();
	NPC_R2D2_Precache();
	NPC_R5D2_Precache();
	NPC_Probe_Precache();
	//NPC_Interrogator_Precache(gentity_t *self);
	NPC_MineMonster_Precache();
	NPC_Howler_Precache();
	NPC_ATST_Precache();
	NPC_Sentry_Precache();
	NPC_Mark1_Precache();
	NPC_Mark2_Precache();
	NPC_GalakMech_Precache();
	NPC_Protocol_Precache();
	Boba_Precache();
	NPC_Wampa_Precache();
#endif //__DOMINANCE_AI__
	
}

extern void BG_SetAnim(playerState_t *ps, animation_t *animations, int setAnimParts,int anim,int setAnimFlags, int blendTime);
extern void BG_SetAnimFinal(playerState_t *ps, animation_t *animations, int setAnimParts,int anim,int setAnimFlags);

void NPC_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask )
{
	trap->Trace(results, start, mins, maxs, end, passEntityNum, contentmask, 0, 0, 0);
}

//#define __NEW__

void NPC_SetAnim(gentity_t *ent, int setAnimParts, int anim, int setAnimFlags)
{	// FIXME : once torsoAnim and legsAnim are in the same structure for NCP and Players
	// rename PM_SETAnimFinal to PM_SetAnim and have both NCP and Players call PM_SetAnim
#ifndef __NEW__
	G_SetAnim(ent, NULL, setAnimParts, anim, setAnimFlags, 0);
#endif //__NEW__

#ifdef __NEW__
	if(ent && ent->inuse && ent->client)
	{//Players, NPCs
		//if (setAnimFlags&SETANIM_FLAG_OVERRIDE)
		{
			pmove_t pmv;

			memset (&pmv, 0, sizeof(pmv));
			pmv.ps = &ent->client->ps;
			pmv.animations = bgAllAnims[ent->localAnimIndex].anims;
			pmv.cmd = ent->client->pers.cmd;
			pmv.trace = NPC_Trace;
			pmv.pointcontents = trap->PointContents;
			pmv.gametype = level.gametype;

			//don't need to bother with ghoul2 stuff, it's not even used in PM_SetAnim.
			pm = &pmv;

			if (setAnimParts & SETANIM_TORSO)
			{
				if( (setAnimFlags & SETANIM_FLAG_RESTART) || ent->client->ps.torsoAnim != anim )
				{
					//PM_SetTorsoAnimTimer( ent, &ent->client->ps.torsoTimer, 0 );
					PM_SetTorsoAnimTimer( ent, &ent->client->ps.torsoTimer, 0 );
				}
			}
			if (setAnimParts & SETANIM_LEGS)
			{
				if( (setAnimFlags & SETANIM_FLAG_RESTART) || ent->client->ps.legsAnim != anim )
				{
					//PM_SetLegsAnimTimer( ent, &ent->client->ps.legsAnimTimer, 0 );
					PM_SetLegsAnimTimer( ent, &ent->client->ps.legsTimer, 0 );
				}
			}
		}

		//PM_SetAnimFinal(&ent->client->ps.torsoAnim,&ent->client->ps.legsAnim,setAnimParts,anim,setAnimFlags,
		//	&ent->client->ps.torsoAnimTimer,&ent->client->ps.legsAnimTimer,ent);

		BG_SetAnimFinal(&ent->client->ps, bgAllAnims[ent->localAnimIndex].anims, setAnimParts, anim, setAnimFlags);
		//ent->client->ps.torsoTimer
	}
	/*else
	{//bodies, etc.
		if (setAnimFlags&SETANIM_FLAG_OVERRIDE)
		{
			if (setAnimParts & SETANIM_TORSO)
			{
				if( (setAnimFlags & SETANIM_FLAG_RESTART) || ent->s.torsoAnim != anim )
				{
					PM_SetTorsoAnimTimer( ent, &ent->s.torsoAnimTimer, 0 );
				}
			}
			if (setAnimParts & SETANIM_LEGS)
			{
				if( (setAnimFlags & SETANIM_FLAG_RESTART) || ent->s.legsAnim != anim )
				{
					PM_SetLegsAnimTimer( ent, &ent->s.legsAnimTimer, 0 );
				}
			}
		}

		PM_SetAnimFinal(&ent->s.torsoAnim,&ent->s.legsAnim,setAnimParts,anim,setAnimFlags,
			&ent->s.torsoAnimTimer,&ent->s.legsAnimTimer,ent);
	}*/
	else
	{
		G_SetAnim(ent, NULL, setAnimParts, anim, setAnimFlags, 0);
	}
#endif //__NEW__
}
