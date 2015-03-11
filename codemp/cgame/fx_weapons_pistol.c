// Bryar Pistol Weapon Effects

#include "cg_local.h"
#include "fx_local.h"

/*
-------------------------

	ALT FIRE

-------------------------
FX_BryarAltProjectileThink
-------------------------
*/
void FX_BryarAltProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;
	int t;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	// see if we have some sort of extra charge going on
	for (t = 1; t < cent->currentState.generic1; t++ )
	{
		// just add ourselves over, and over, and over when we are charged
		PlayEffectID( cgs.effects.bryarPowerupShotEffect, cent->lerpOrigin, forward, -1, -1, qfalse );
	}

	//	for ( int t = 1; t < cent->gent->count; t++ )	// The single player stores the charge in count, which isn't accessible on the client
	if (weapon->altMissileRenderfx)
	{
		PlayEffectID(weapon->altMissileRenderfx, cent->lerpOrigin, forward, -1, -1, qfalse);
	}
	else
	{
		PlayEffectID(cgs.effects.bryarShotEffect, cent->lerpOrigin, forward, -1, -1, qfalse);
	}

	AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 1.0f, 1.0f, 1.0f );
}

/*
-------------------------
FX_BryarAltHitWall
-------------------------
*/
void FX_BryarAltHitWall(vec3_t origin, vec3_t normal, int power, int weapon, qboolean altFire)
{
	// Set fx to primary weapon fx.
	fxHandle_t fx = cg_weapons[weapon].missileWallImpactfx;
	fxHandle_t fx2 = cg_weapons[weapon].wallImpactEffectEnhancedFX;

	if (!fx) {
		// If there is no primary (missileWallImpactfx) fx. Use original blaster fx.
		fx = cgs.effects.blasterWallImpactEffect;

		// If falling back to normal concussion fx, we have no enhanced.
		fx2 = fx; // Force normal fx.
	}

	if (altFire) {
		// If this is alt fire. Override all fx with alt fire fx...
		if (cg_weapons[weapon].altMissileWallImpactfx)
		{// We have alt fx for this weapon. Use it.
			fx = cg_weapons[weapon].altMissileWallImpactfx;
		}

		if (cg_weapons[weapon].altWallImpactEffectEnhancedFX)
		{// We have enhanced alt. Use it.
			fx2 = cg_weapons[weapon].altWallImpactEffectEnhancedFX;
		}
		else
		{// We have no alt enhanced fx.
			fx2 = fx; // Force normal fx.
		}
	}

	// If fx2 (enhanced) does not exist (set fx2 to -1 above), this should return normal fx.
	fx = CG_EnableEnhancedFX(fx, fx2);

	if (fx)
	{// We have fx for this. Play it.
		PlayEffectID(fx, origin, normal, -1, -1, qfalse);
	}
	else
	{// This should never be possible, but just in case, fall back to concussion here.
		PlayEffectID(cgs.effects.bryarWallImpactEffect, origin, normal, -1, -1, qfalse);
	}
}


/*
---------------------------
FX_ConcAltShot
---------------------------
*/
static vec3_t WHITE	={1.0f,1.0f,1.0f};
static vec3_t BRIGHT={0.75f,0.5f,1.0f};

void FX_ConcAltShot( vec3_t start, vec3_t end, int weapon )
{
	if (weapon != WP_CONCUSSION 
		&& (cg_weapons[weapon].missileDlightColor[0] > 0 || cg_weapons[weapon].missileDlightColor[1] > 0 || cg_weapons[weapon].missileDlightColor[2] > 0))
	{
		vec3_t mainColor, beefColor;

		VectorCopy(cg_weapons[weapon].missileDlightColor, mainColor);
		VectorCopy(cg_weapons[weapon].missileDlightColor, beefColor);
		beefColor[0] *= 0.4;
		beefColor[1] *= 0.4;
		beefColor[2] *= 0.4;

		//"concussion/beam"
		trap->FX_AddLine( start, end, 0.1f, 10.0f, 0.0f,
							1.0f, 0.0f, 0.0f,
							mainColor, mainColor, 0.0f,
							175, trap->R_RegisterShader( "gfx/effects/whiteline2" ),
							FX_SIZE_LINEAR | FX_ALPHA_LINEAR );

		
		// add some beef
		trap->FX_AddLine( start, end, 0.1f, 7.0f, 0.0f,
						1.0f, 0.0f, 0.0f,
						beefColor, beefColor, 0.0f,
						150, trap->R_RegisterShader( "gfx/misc/whiteline2" ),
						FX_SIZE_LINEAR | FX_ALPHA_LINEAR );
	}
	else
	{
		//"concussion/beam"
		trap->FX_AddLine( start, end, 0.1f, 10.0f, 0.0f,
							1.0f, 0.0f, 0.0f,
							WHITE, WHITE, 0.0f,
							175, trap->R_RegisterShader( "gfx/effects/blueLine" ),
							FX_SIZE_LINEAR | FX_ALPHA_LINEAR );

		// add some beef
		trap->FX_AddLine( start, end, 0.1f, 7.0f, 0.0f,
						1.0f, 0.0f, 0.0f,
						BRIGHT, BRIGHT, 0.0f,
						150, trap->R_RegisterShader( "gfx/misc/whiteline2" ),
						FX_SIZE_LINEAR | FX_ALPHA_LINEAR );
	}
}
