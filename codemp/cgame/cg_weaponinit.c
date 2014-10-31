//
// cg_weaponinit.c -- events and effects dealing with weapons
#include "cg_local.h"
#include "fx_local.h"


/*
=================
CG_RegisterWeapon

The server says this item is used on this level
=================
*/
void CG_RegisterWeapon( int weaponNum) {
	weaponInfo_t	*weaponInfo;
	gitem_t			*item, *ammo;
	char			path[MAX_QPATH];
	vec3_t			mins, maxs;
	int				i;

	if (weaponNum < WP_NONE) 
	{// Missing SP weapons...
		return;
	}

	weaponInfo = &cg_weapons[weaponNum];

	if ( weaponNum == 0 ) {
		return;
	}

	if ( weaponInfo->registered ) {
		return;
	}

	//if ( cgs.wDisable & (1<<weaponNum) )
	//	return;

	memset( weaponInfo, 0, sizeof( *weaponInfo ) );
	weaponInfo->registered = qtrue;

	for ( item = bg_itemlist + 1 ; item->classname ; item++ ) {
		if ( item->giType == IT_WEAPON && item->giTag == weaponNum ) {
			weaponInfo->item = item;
			break;
		}
	}
	if ( !item->classname ) {
		trap->Error( ERR_DROP, "Couldn't find weapon %i", weaponNum );
		return;
	}
	CG_RegisterItemVisuals( item - bg_itemlist );

	// load cmodel before model so filecache works
	weaponInfo->weaponModel = trap->R_RegisterModel( item->world_model[0] );
	// load in-view model also
	weaponInfo->viewModel = trap->R_RegisterModel(item->view_model);

	// calc midpoint for rotation
	trap->R_ModelBounds( weaponInfo->weaponModel, mins, maxs );
	for ( i = 0 ; i < 3 ; i++ ) {
		weaponInfo->weaponMidpoint[i] = mins[i] + 0.5 * ( maxs[i] - mins[i] );
	}

	weaponInfo->weaponIcon = trap->R_RegisterShader( item->icon );
	weaponInfo->ammoIcon = trap->R_RegisterShader( item->icon );

	for ( ammo = bg_itemlist + 1 ; ammo->classname ; ammo++ ) {
		if ( ammo->giType == IT_AMMO && ammo->giTag == weaponNum ) {
			break;
		}
	}
	if ( ammo->classname && ammo->world_model[0] ) {
		weaponInfo->ammoModel = trap->R_RegisterModel( ammo->world_model[0] );
	}

//	strcpy( path, item->view_model );
//	COM_StripExtension( path, path );
//	strcat( path, "_flash.md3" );
	weaponInfo->flashModel = 0;//trap->R_RegisterModel( path );

	if (weaponNum == WP_DISRUPTOR ||
		weaponNum == WP_FLECHETTE ||
		weaponNum == WP_REPEATER ||
		weaponNum == WP_ROCKET_LAUNCHER ||
		weaponNum == WP_CONCUSSION ||
		weaponNum == WP_Z6_BLASTER_CANON)
	{
		Q_strncpyz( path, item->view_model, sizeof(path) );
		COM_StripExtension( path, path, sizeof( path ) );
		Q_strcat( path, sizeof(path), "_barrel.md3" );
		weaponInfo->barrelModel = trap->R_RegisterModel( path );
	}
	else if (weaponNum == WP_STUN_BATON)
	{ //only weapon with more than 1 barrel..
		trap->R_RegisterModel("models/weapons2/stun_baton/baton_barrel.md3");
		trap->R_RegisterModel("models/weapons2/stun_baton/baton_barrel2.md3");
		trap->R_RegisterModel("models/weapons2/stun_baton/baton_barrel3.md3");
	}
	else
	{
		weaponInfo->barrelModel = 0;
	}

	if (weaponNum != WP_SABER)
	{
		Q_strncpyz( path, item->view_model, sizeof(path) );
		COM_StripExtension( path, path, sizeof( path ) );
		Q_strcat( path, sizeof(path), "_hand.md3" );
		weaponInfo->handsModel = trap->R_RegisterModel( path );
	}
	else
	{
		weaponInfo->handsModel = 0;
	}

	switch ( weaponNum ) {
	case WP_STUN_BATON:
	case WP_MELEE:

		trap->FX_RegisterEffect( "stunBaton/flesh_impact" );

		if (weaponNum == WP_STUN_BATON)
		{
			trap->S_RegisterSound( "sound/weapons/baton/idle.wav" );
			weaponInfo->flashSound[0] = trap->S_RegisterSound( "sound/weapons/baton/fire.mp3" );
			weaponInfo->altFlashSound[0] = trap->S_RegisterSound( "sound/weapons/baton/fire.mp3" );
		}
		else
		{
		
		}
		break;
	case WP_SABER:
		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		weaponInfo->firingSound = trap->S_RegisterSound( "sound/weapons/saber/saberhum1.wav" );
		weaponInfo->missileModel		= trap->R_RegisterModel( "models/weapons2/saber/saber_w.glm" );
		break;

	case WP_CONCUSSION:
		weaponInfo->selectSound			= trap->S_RegisterSound("sound/weapons/concussion/select.wav");
		weaponInfo->flashSound[0]		= NULL_SOUND;
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap->FX_RegisterEffect( "concussion/muzzle_flash" );
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_ConcussionProjectileThink;
		weaponInfo->altFlashSound[0]	= NULL_SOUND;
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= trap->S_RegisterSound( "sound/weapons/bryar/altcharge.wav");
		weaponInfo->altMuzzleEffect		= trap->FX_RegisterEffect( "concussion/altmuzzle_flash" );
		weaponInfo->altMissileModel		= NULL_HANDLE;
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_ConcussionProjectileThink;
		cgs.effects.disruptorAltMissEffect		= trap->FX_RegisterEffect( "disruptor/alt_miss" );
		cgs.effects.concussionShotEffect		= trap->FX_RegisterEffect( "concussion/shot" );
		cgs.effects.concussionImpactEffect		= trap->FX_RegisterEffect( "concussion/explosion" );
		trap->R_RegisterShader("gfx/effects/blueLine");
		trap->R_RegisterShader("gfx/misc/whiteline2");
		break;

	case WP_BRYAR_PISTOL:
		weaponInfo->item->classname = "Pistol";
		weaponInfo->selectSound = trap->S_RegisterSound("sound/weapons/bryar/select.wav");
		weaponInfo->flashSound[0] = trap->S_RegisterSound("sound/weapons/bryar/fire.wav");
		weaponInfo->firingSound = NULL_SOUND;
		weaponInfo->chargeSound = NULL_SOUND;
		weaponInfo->muzzleEffect = trap->FX_RegisterEffect("bryar/muzzle_flash");
		weaponInfo->missileModel = NULL_HANDLE;
		weaponInfo->missileSound = NULL_SOUND;
		weaponInfo->missileDlight = 0;
		weaponInfo->missileHitSound = NULL_SOUND;
		weaponInfo->missileTrailFunc = FX_BryarProjectileThink;
		weaponInfo->missileRenderfx = NULL_FX;
		weaponInfo->altMissileRenderfx = NULL_FX;
		weaponInfo->powerupShotRenderfx = NULL_FX;
		weaponInfo->altFlashSound[0] = trap->S_RegisterSound("sound/weapons/bryar/alt_fire.wav");
		weaponInfo->altFiringSound = NULL_SOUND;
		weaponInfo->altMuzzleEffect = trap->FX_RegisterEffect("bryar/muzzle_flash");
		weaponInfo->altMissileModel = NULL_HANDLE;
		weaponInfo->altMissileSound = NULL_SOUND;
		weaponInfo->altMissileDlight = 0;
		weaponInfo->altMissileHitSound = NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_BryarAltProjectileThink;
		cgs.effects.bryarShotEffect = trap->FX_RegisterEffect("bryar/shot");
		cgs.effects.bryarShotEffect1 = trap->FX_RegisterEffect("bryar/lvl3_shot");
		cgs.effects.bryarPowerupShotEffect = trap->FX_RegisterEffect("bryar/crackleShot");
		cgs.effects.bryarWallImpactEffect = trap->FX_RegisterEffect("bryar/wall_impact");
		cgs.effects.bryarWallImpactEffectEnhancedFX = trap->FX_RegisterEffect("bryar/wall_impact_enhanced2");
		cgs.effects.bryarWallImpactEffect2 = trap->FX_RegisterEffect("bryar/wall_impact2");
		cgs.effects.bryarWallImpactEffect2EnhancedFX = trap->FX_RegisterEffect("bryar/wall_impact2_enhanced2");
		cgs.effects.bryarWallImpactEffect3 = trap->FX_RegisterEffect("bryar/wall_impact3");
		cgs.effects.bryarWallImpactEffect3EnhancedFX = trap->FX_RegisterEffect("bryar/wall_impact3_enhanced2");
		cgs.effects.bryarFleshImpactEffect = trap->FX_RegisterEffect("bryar/flesh_impact");
		cgs.effects.bryarDroidImpactEffect = trap->FX_RegisterEffect("bryar/droid_impact");
		cgs.media.bryarFrontFlash = trap->R_RegisterShader("gfx/effects/bryarFrontFlash");

		break;

	case WP_BRYAR_OLD:
		weaponInfo->item->classname = "Old Bayar Pistol";
		weaponInfo->selectSound = trap->S_RegisterSound("sound/weapons/bryar/select.wav");
		weaponInfo->flashSound[0] = trap->S_RegisterSound("sound/weapons/bryar/fire.wav");
		weaponInfo->firingSound = NULL_SOUND;
		weaponInfo->chargeSound = NULL_SOUND;
		weaponInfo->muzzleEffect = trap->FX_RegisterEffect("bryar/muzzle_flash");
		weaponInfo->missileModel = NULL_HANDLE;
		weaponInfo->missileSound = NULL_SOUND;
		weaponInfo->missileDlight = 0;
		weaponInfo->missileHitSound = NULL_SOUND;
		weaponInfo->missileTrailFunc = FX_BryarProjectileThink;
		weaponInfo->missileRenderfx = NULL_FX;
		weaponInfo->altMissileRenderfx = NULL_FX;
		weaponInfo->powerupShotRenderfx = NULL_FX;
		weaponInfo->altFlashSound[0] = trap->S_RegisterSound("sound/weapons/bryar/alt_fire.wav");
		weaponInfo->altFiringSound = NULL_SOUND;
		weaponInfo->altMuzzleEffect = trap->FX_RegisterEffect("bryar/muzzle_flash");
		weaponInfo->altMissileModel = NULL_HANDLE;
		weaponInfo->altMissileSound = NULL_SOUND;
		weaponInfo->altMissileDlight = 0;
		weaponInfo->altMissileHitSound = NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_BryarAltProjectileThink;
		cgs.effects.bryarShotEffect = trap->FX_RegisterEffect("bryar/shot");
		cgs.effects.bryarShotEffect1 = trap->FX_RegisterEffect("bryar/lvl3_shot");
		cgs.effects.bryarPowerupShotEffect = trap->FX_RegisterEffect("bryar/crackleShot");
		cgs.effects.bryarWallImpactEffect = trap->FX_RegisterEffect("bryar/wall_impact");
		cgs.effects.bryarWallImpactEffectEnhancedFX = trap->FX_RegisterEffect("bryar/wall_impact_enhanced2");
		cgs.effects.bryarWallImpactEffect2 = trap->FX_RegisterEffect("bryar/wall_impact2");
		cgs.effects.bryarWallImpactEffect2EnhancedFX = trap->FX_RegisterEffect("bryar/wall_impact2_enhanced2");
		cgs.effects.bryarWallImpactEffect3 = trap->FX_RegisterEffect("bryar/wall_impact3");
		cgs.effects.bryarWallImpactEffect3EnhancedFX = trap->FX_RegisterEffect("bryar/wall_impact3_enhanced2");
		cgs.effects.bryarFleshImpactEffect = trap->FX_RegisterEffect("bryar/flesh_impact");
		cgs.effects.bryarDroidImpactEffect = trap->FX_RegisterEffect("bryar/droid_impact");
		cgs.media.bryarFrontFlash = trap->R_RegisterShader("gfx/effects/bryarFrontFlash");

	case WP_WESTER_PISTOL:
		weaponInfo->item->classname = "Westar Pistol";
		weaponInfo->selectSound = trap->S_RegisterSound("sound/weapons/bryar/select.wav");
		weaponInfo->flashSound[0] = trap->S_RegisterSound("sound/weapons/westar/fire.wav");
		weaponInfo->firingSound = NULL_SOUND;
		weaponInfo->chargeSound = NULL_SOUND;
		weaponInfo->muzzleEffect = trap->FX_RegisterEffect("bryar/muzzle_flash");
		weaponInfo->missileModel = NULL_HANDLE;
		weaponInfo->missileSound = NULL_SOUND;
		weaponInfo->missileDlight = 0;
		weaponInfo->missileHitSound = NULL_SOUND;
		weaponInfo->missileTrailFunc = FX_BryarProjectileThink;
		weaponInfo->missileRenderfx = NULL_FX;
		weaponInfo->altMissileRenderfx = NULL_FX;
		weaponInfo->powerupShotRenderfx = NULL_FX;
		weaponInfo->altFlashSound[0] = trap->S_RegisterSound("sound/weapons/westar/alt_fire.wav");
		weaponInfo->altFiringSound = NULL_SOUND;
		weaponInfo->altMuzzleEffect = trap->FX_RegisterEffect("bryar/muzzle_flash");
		weaponInfo->altMissileModel = NULL_HANDLE;
		weaponInfo->altMissileSound = NULL_SOUND;
		weaponInfo->altMissileDlight = 0;
		weaponInfo->altMissileHitSound = NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_BryarAltProjectileThink;
		cgs.effects.bryarShotEffect = trap->FX_RegisterEffect("bryar/shot");
		cgs.effects.bryarShotEffect1 = trap->FX_RegisterEffect("bryar/lvl3_shot");
		cgs.effects.bryarPowerupShotEffect = trap->FX_RegisterEffect("bryar/crackleShot");
		cgs.effects.bryarWallImpactEffect = trap->FX_RegisterEffect("bryar/wall_impact");
		cgs.effects.bryarWallImpactEffectEnhancedFX = trap->FX_RegisterEffect("bryar/wall_impact_enhanced2");
		cgs.effects.bryarWallImpactEffect2 = trap->FX_RegisterEffect("bryar/wall_impact2");
		cgs.effects.bryarWallImpactEffect2EnhancedFX = trap->FX_RegisterEffect("bryar/wall_impact2_enhanced2");
		cgs.effects.bryarWallImpactEffect3 = trap->FX_RegisterEffect("bryar/wall_impact3");
		cgs.effects.bryarWallImpactEffect3EnhancedFX = trap->FX_RegisterEffect("bryar/wall_impact3_enhanced2");
		cgs.effects.bryarFleshImpactEffect = trap->FX_RegisterEffect("bryar/flesh_impact");
		cgs.effects.bryarDroidImpactEffect = trap->FX_RegisterEffect("bryar/droid_impact");
		cgs.media.bryarFrontFlash = trap->R_RegisterShader("gfx/effects/bryarFrontFlash");
		break;

	case WP_ELG_3A:
		weaponInfo->item->classname = "ELG-3A Diplomat's Pistol";
		weaponInfo->selectSound = trap->S_RegisterSound("sound/weapons/bryar/select.wav");
		weaponInfo->flashSound[0] = trap->S_RegisterSound("sound/weapons/bryar/fire.wav");
		weaponInfo->firingSound = NULL_SOUND;
		weaponInfo->chargeSound = NULL_SOUND;
		weaponInfo->muzzleEffect = trap->FX_RegisterEffect("blaster/muzzle_flash");
		weaponInfo->missileModel = NULL_HANDLE;
		weaponInfo->missileSound = NULL_SOUND;
		weaponInfo->missileDlight = 0;
		weaponInfo->missileHitSound = NULL_SOUND;
		weaponInfo->missileTrailFunc = FX_BryarProjectileThink;
		weaponInfo->missileRenderfx = NULL_FX;
		weaponInfo->altMissileRenderfx = NULL_FX;
		weaponInfo->powerupShotRenderfx = NULL_FX;
		weaponInfo->altFlashSound[0] = trap->S_RegisterSound("sound/weapons/bryar/alt_fire.wav");
		weaponInfo->altFiringSound = NULL_SOUND;
		weaponInfo->altMuzzleEffect = trap->FX_RegisterEffect("blaster/muzzle_flash");
		weaponInfo->altMissileModel = NULL_HANDLE;
		weaponInfo->altMissileSound = NULL_SOUND;
		weaponInfo->altMissileDlight = 0;
		weaponInfo->altMissileHitSound = NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_BryarAltProjectileThink;

		weaponInfo->missileRenderfx = trap->FX_RegisterEffect("bryar/shot");
		weaponInfo->altMissileRenderfx = trap->FX_RegisterEffect("bryar/lvl3_shot");
		weaponInfo->missileWallImpactfx = trap->FX_RegisterEffect("bryar/wall_impact");
		weaponInfo->altMissileWallImpactfx = trap->FX_RegisterEffect("bryar/wall_impact_enhanced2");
		weaponInfo->fleshImpactEffect = trap->FX_RegisterEffect("bryar/flesh_impact");
		weaponInfo->altFleshImpactEffect = trap->FX_RegisterEffect("bryar/flesh_impact");
		weaponInfo->wallImpactEffectEnhancedFX = trap->FX_RegisterEffect("bryar/wall_impact3_enhanced2");
		weaponInfo->shotEffectFx = trap->FX_RegisterEffect("bryar/shot");

		cgs.effects.bryarShotEffect = trap->FX_RegisterEffect("bryar/shot");
		cgs.effects.bryarShotEffect1 = trap->FX_RegisterEffect("bryar/lvl3_shot");
		cgs.effects.bryarPowerupShotEffect = trap->FX_RegisterEffect("bryar/crackleShot");
		cgs.effects.bryarWallImpactEffect = trap->FX_RegisterEffect("bryar/wall_impact");
		cgs.effects.bryarWallImpactEffectEnhancedFX = trap->FX_RegisterEffect("bryar/wall_impact_enhanced2");
		cgs.effects.bryarWallImpactEffect2 = trap->FX_RegisterEffect("bryar/wall_impact2");
		cgs.effects.bryarWallImpactEffect2EnhancedFX = trap->FX_RegisterEffect("bryar/wall_impact2_enhanced2");
		cgs.effects.bryarWallImpactEffect3 = trap->FX_RegisterEffect("bryar/wall_impact3");
		cgs.effects.bryarWallImpactEffect3EnhancedFX = trap->FX_RegisterEffect("bryar/wall_impact3_enhanced2");
		cgs.effects.bryarFleshImpactEffect = trap->FX_RegisterEffect("bryar/flesh_impact");
		cgs.effects.bryarDroidImpactEffect = trap->FX_RegisterEffect("bryar/droid_impact");
		cgs.media.bryarFrontFlash = trap->R_RegisterShader("gfx/effects/bryarFrontFlash");
		break;

	case WP_S5_PISTOL:
		weaponInfo->item->classname = "s5 Pistol";
		weaponInfo->selectSound = trap->S_RegisterSound("sound/weapons/bryar/select.wav");
		weaponInfo->flashSound[0] = trap->S_RegisterSound("sound/weapons/bryar/fire.wav");
		weaponInfo->firingSound = NULL_SOUND;
		weaponInfo->chargeSound = NULL_SOUND;
		weaponInfo->muzzleEffect = trap->FX_RegisterEffect("blaster/muzzle_flash");
		weaponInfo->missileModel = NULL_HANDLE;
		weaponInfo->missileSound = NULL_SOUND;
		weaponInfo->missileDlight = 0;
		weaponInfo->missileHitSound = NULL_SOUND;
		weaponInfo->missileTrailFunc = FX_BryarProjectileThink;
		weaponInfo->missileRenderfx = NULL_FX;
		weaponInfo->altMissileRenderfx = NULL_FX;
		weaponInfo->powerupShotRenderfx = NULL_FX;
		weaponInfo->altFlashSound[0] = trap->S_RegisterSound("sound/weapons/bryar/alt_fire.wav");
		weaponInfo->altFiringSound = NULL_SOUND;
		weaponInfo->altMuzzleEffect = trap->FX_RegisterEffect("blaster/muzzle_flash");
		weaponInfo->altMissileModel = NULL_HANDLE;
		weaponInfo->altMissileSound = NULL_SOUND;
		weaponInfo->altMissileDlight = 0;
		weaponInfo->altMissileHitSound = NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_BryarAltProjectileThink;

		weaponInfo->missileRenderfx = trap->FX_RegisterEffect("bryar/shot");
		weaponInfo->altMissileRenderfx = trap->FX_RegisterEffect("bryar/lvl3_shot");
		weaponInfo->missileWallImpactfx = trap->FX_RegisterEffect("bryar/wall_impact");
		weaponInfo->altMissileWallImpactfx = trap->FX_RegisterEffect("bryar/wall_impact_enhanced2");
		weaponInfo->fleshImpactEffect = trap->FX_RegisterEffect("bryar/flesh_impact");
		weaponInfo->altFleshImpactEffect = trap->FX_RegisterEffect("bryar/flesh_impact");
		weaponInfo->wallImpactEffectEnhancedFX = trap->FX_RegisterEffect("bryar/wall_impact3_enhanced2");
		weaponInfo->shotEffectFx = trap->FX_RegisterEffect("bryar/shot");

		cgs.effects.bryarShotEffect = trap->FX_RegisterEffect("bryar/shot");
		cgs.effects.bryarShotEffect1 = trap->FX_RegisterEffect("bryar/lvl3_shot");
		cgs.effects.bryarPowerupShotEffect = trap->FX_RegisterEffect("bryar/crackleShot");
		cgs.effects.bryarWallImpactEffect = trap->FX_RegisterEffect("bryar/wall_impact");
		cgs.effects.bryarWallImpactEffectEnhancedFX = trap->FX_RegisterEffect("bryar/wall_impact_enhanced2");
		cgs.effects.bryarWallImpactEffect2 = trap->FX_RegisterEffect("bryar/wall_impact2");
		cgs.effects.bryarWallImpactEffect2EnhancedFX = trap->FX_RegisterEffect("bryar/wall_impact2_enhanced2");
		cgs.effects.bryarWallImpactEffect3 = trap->FX_RegisterEffect("bryar/wall_impact3");
		cgs.effects.bryarWallImpactEffect3EnhancedFX = trap->FX_RegisterEffect("bryar/wall_impact3_enhanced2");
		cgs.effects.bryarFleshImpactEffect = trap->FX_RegisterEffect("bryar/flesh_impact");
		cgs.effects.bryarDroidImpactEffect = trap->FX_RegisterEffect("bryar/droid_impact");
		cgs.media.bryarFrontFlash = trap->R_RegisterShader("gfx/effects/bryarFrontFlash");
		break;

	case WP_A280: // UQ1: Example. Should have it's own fx...
		weaponInfo->item->classname =	"A280 Clone Blaster";
		weaponInfo->selectSound			= trap->S_RegisterSound("sound/weapons/disruptor/select.wav");
		weaponInfo->flashSound[0]		= trap->S_RegisterSound("sound/weapons/A280/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap->FX_RegisterEffect("disruptor/muzzle_flash");
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileRenderfx		= trap->FX_RegisterEffect("blaster/shot");
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= 0;
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/A280/alt_fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= trap->S_RegisterSound("sound/weapons/disruptor/altCharge.wav");
		weaponInfo->altMuzzleEffect		= trap->FX_RegisterEffect("disruptor/muzzle_flash");
		weaponInfo->altMissileModel		= NULL_HANDLE;
		weaponInfo->altMissileRenderfx  = trap->FX_RegisterEffect("blaster/shot");
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = 0;

		weaponInfo->missileRenderfx = trap->FX_RegisterEffect("blaster/shot");
		weaponInfo->altMissileRenderfx = trap->FX_RegisterEffect("blaster/shot");
		weaponInfo->missileWallImpactfx = trap->FX_RegisterEffect("blaster/wall_impact");
		weaponInfo->altMissileWallImpactfx = trap->FX_RegisterEffect("blaster/wall_impact_enhanced2");
		weaponInfo->fleshImpactEffect = trap->FX_RegisterEffect("blaster/flesh_impact");
		weaponInfo->altFleshImpactEffect = trap->FX_RegisterEffect("blaster/flesh_impact");
		weaponInfo->wallImpactEffectEnhancedFX = trap->FX_RegisterEffect("blaster/wall_impact_enhanced2");
		weaponInfo->shotEffectFx = trap->FX_RegisterEffect("blaster/shot");

		trap->R_RegisterShader( "gfx/effects/redLine" );
		trap->R_RegisterShader( "gfx/misc/whiteline2" );
		trap->R_RegisterShader( "gfx/effects/smokeTrail" );
		trap->S_RegisterSound("sound/weapons/disruptor/zoomstart.wav");
		trap->S_RegisterSound("sound/weapons/disruptor/zoomend.wav");

		// Disruptor gun zoom interface
		cgs.media.disruptorMask			= trap->R_RegisterShader( "gfx/2d/cropCircle2");
		cgs.media.disruptorInsert		= trap->R_RegisterShader( "gfx/2d/cropCircle");
		cgs.media.disruptorLight		= trap->R_RegisterShader( "gfx/2d/cropCircleGlow" );
		cgs.media.disruptorInsertTick	= trap->R_RegisterShader( "gfx/2d/insertTick" );
		cgs.media.disruptorChargeShader	= trap->R_RegisterShaderNoMip("gfx/2d/crop_charge");
		cgs.media.disruptorZoomLoop		= trap->S_RegisterSound("sound/weapons/disruptor/zoomloop.wav");
		break;

	case WP_DC15:
		weaponInfo->item->classname = "DC-15 Blaster";
		weaponInfo->selectSound = trap->S_RegisterSound("sound/weapons/repeater/select.wav");
		weaponInfo->flashSound[0] = trap->S_RegisterSound("sound/weapons/repeater/fire.wav");
		weaponInfo->firingSound = NULL_SOUND;
		weaponInfo->chargeSound = NULL_SOUND;
		weaponInfo->muzzleEffect = trap->FX_RegisterEffect("clone_blaster/muzzle_flash"); //Is working like it should, KEEP it like that
		weaponInfo->missileModel = NULL_HANDLE;
		weaponInfo->missileSound = NULL_SOUND;
		weaponInfo->missileDlight = 0;
		weaponInfo->missileHitSound = NULL_SOUND;
		weaponInfo->missileTrailFunc = FX_RepeaterProjectileThink;
		weaponInfo->missileRenderfx = trap->FX_RegisterEffect("clone_rifle/projectile");
		weaponInfo->altMissileRenderfx = trap->FX_RegisterEffect("clone_blaster/altshot");
		weaponInfo->powerupShotRenderfx = NULL_FX;
		weaponInfo->altFlashSound[0] = trap->S_RegisterSound("sound/weapons/repeater/alt_fire.wav");
		weaponInfo->altFiringSound = NULL_SOUND;
		weaponInfo->altChargeSound = trap->S_RegisterSound("sound/weapons/SBDarm/cannon_charge.mp3");
		weaponInfo->altMuzzleEffect = trap->FX_RegisterEffect("clone_blaster/altmuzzle_flash");
		weaponInfo->altMissileModel = NULL_HANDLE;
		weaponInfo->altMissileSound = NULL_SOUND;
		weaponInfo->altMissileDlight = 0;
		weaponInfo->altMissileHitSound = NULL_SOUND;
		weaponInfo->spinSound = trap->S_RegisterSound("sound/weapons/z6/spinny.wav");
		weaponInfo->spindownSound = trap->S_RegisterSound("sound/weapons/z6/chaingun_spindown.wav");

		weaponInfo->altMissileTrailFunc = FX_ConcussionProjectileThink;
		weaponInfo->missileWallImpactfx = trap->FX_RegisterEffect("clone_blaster/wall_impact");
		weaponInfo->altMissileWallImpactfx = trap->FX_RegisterEffect("clone_rifle/wall_impact_enhanced2"); // etc here.. then..
		weaponInfo->wallImpactEffectEnhancedFX = trap->FX_RegisterEffect("clone_rifle/wall_impact_enhanced2");
		weaponInfo->fleshImpactEffect = trap->FX_RegisterEffect("clone_blaster/flesh_impact");
		weaponInfo->altFleshImpactEffect = trap->FX_RegisterEffect("clone_blaster/concussion"); // not sure about this one
		weaponInfo->shotEffectFx = trap->FX_RegisterEffect("clone_blaster/shot");

		break;

	case WP_WESTARM5:
		weaponInfo->item->classname = "Westarm 5 Blaster";
		weaponInfo->selectSound = trap->S_RegisterSound("sound/weapons/repeater/select.wav");
		weaponInfo->flashSound[0] = trap->S_RegisterSound("sound/weapons/repeater/fire.wav");
		weaponInfo->firingSound = NULL_SOUND;
		weaponInfo->chargeSound = NULL_SOUND;
		weaponInfo->muzzleEffect = trap->FX_RegisterEffect("clone_blaster/muzzle_flash");
		weaponInfo->missileModel = NULL_HANDLE;
		weaponInfo->missileSound = NULL_SOUND;
		weaponInfo->missileDlight = 0;
		weaponInfo->missileHitSound = NULL_SOUND;
		weaponInfo->missileTrailFunc = FX_RepeaterProjectileThink;
		weaponInfo->missileRenderfx = trap->FX_RegisterEffect("clone_rifle/projectile");
		weaponInfo->altMissileRenderfx = trap->FX_RegisterEffect("clone_rifle/alt_projectile");
		weaponInfo->powerupShotRenderfx = NULL_FX;
		weaponInfo->altFlashSound[0] = trap->S_RegisterSound("sound/weapons/repeater/alt_fire.wav");
		weaponInfo->altFiringSound = NULL_SOUND;
		weaponInfo->altChargeSound = trap->S_RegisterSound("sound/weapons/SBDarm/cannon_charge.mp3");
		weaponInfo->altMuzzleEffect = trap->FX_RegisterEffect("clone_rifle/muzzle_flash");
		weaponInfo->altMissileModel = NULL_HANDLE;
		weaponInfo->altMissileSound = NULL_SOUND;
		weaponInfo->altMissileDlight = 0;
		weaponInfo->altMissileHitSound = NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_RepeaterAltProjectileThink;
		weaponInfo->spinSound = trap->S_RegisterSound("sound/weapons/z6/spinny.wav");
		weaponInfo->spindownSound = trap->S_RegisterSound("sound/weapons/z6/chaingun_spindown.wav");

		weaponInfo->missileWallImpactfx = trap->FX_RegisterEffect("clone_rifle/wall_impact");
		weaponInfo->altMissileWallImpactfx = trap->FX_RegisterEffect("clone_rifle/wall_impact_enhanced2"); // etc here.. then..
		weaponInfo->fleshImpactEffect = trap->FX_RegisterEffect("clone_rifle/flesh_impact");
		weaponInfo->altFleshImpactEffect = trap->FX_RegisterEffect("clone_rifle/concussion"); // not sure about this one
		weaponInfo->wallImpactEffectEnhancedFX = trap->FX_RegisterEffect("clone_rifle/wall_impact_enhanced2");
		weaponInfo->shotEffectFx = trap->FX_RegisterEffect("clone_rifle/shot");
		weaponInfo->explotionImpactEffect = trap->FX_RegisterEffect("clone_rifle/explosion");
		weaponInfo->ionBlastShotEffect = trap->FX_RegisterEffect("clone_rifle/ionblast");
		break;

	case WP_T21:
		weaponInfo->item->classname = "T-21 Blaster Rifle";
		weaponInfo->selectSound = trap->S_RegisterSound("sound/weapons/flechette/select.wav");
		weaponInfo->flashSound[0] = trap->S_RegisterSound("sound/weapons/T-21/fire.mp3");
		weaponInfo->firingSound = NULL_SOUND;
		weaponInfo->chargeSound = NULL_SOUND;
		weaponInfo->muzzleEffect = trap->FX_RegisterEffect("blaster/muzzle_flash");
		weaponInfo->missileModel = trap->R_RegisterModel("models/weapons2/golan_arms/projectileMain.md3");
		weaponInfo->missileSound = NULL_SOUND;
		weaponInfo->missileDlight = 0;
		weaponInfo->missileHitSound = NULL_SOUND;
		weaponInfo->missileTrailFunc = FX_T21ProjectileThink;
		weaponInfo->missileRenderfx = NULL_FX;
		weaponInfo->altMissileRenderfx = NULL_FX;
		weaponInfo->powerupShotRenderfx = NULL_FX;
		weaponInfo->altFlashSound[0] = trap->S_RegisterSound("sound/weapons/T-21/alt_fire.mp3");
		weaponInfo->altFiringSound = trap->S_RegisterSound("sound/weapons/T-21/alt_fire.mp3");
		weaponInfo->altChargeSound = NULL_SOUND;
		weaponInfo->altMuzzleEffect = trap->FX_RegisterEffect("blaster/muzzle_flash");
		weaponInfo->altMissileModel = trap->R_RegisterModel("models/weapons2/golan_arms/projectile.md3");
		weaponInfo->altMissileSound = NULL_SOUND;
		weaponInfo->altMissileDlight = 0;
		weaponInfo->altMissileHitSound = NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_T21ProjectileThink;
		weaponInfo->shotEffectFx = trap->FX_RegisterEffect("T-21/shot");
		break;
		
	case WP_EE3:
		weaponInfo->item->classname = "EE-3 Blaster";
		weaponInfo->selectSound = trap->S_RegisterSound("sound/weapons/blaster/select.wav");
		weaponInfo->flashSound[0] = trap->S_RegisterSound("sound/weapons/ee3/fire.wav");
		weaponInfo->firingSound = NULL_SOUND;
		weaponInfo->chargeSound = NULL_SOUND;
		weaponInfo->muzzleEffect = trap->FX_RegisterEffect("blaster/muzzle_flash");
		weaponInfo->missileModel = NULL_HANDLE;
		
		weaponInfo->missileSound = NULL_SOUND;
		weaponInfo->missileDlight = 0;
		weaponInfo->missileHitSound = NULL_SOUND;
		weaponInfo->missileTrailFunc = FX_BlasterProjectileThink;
		weaponInfo->altFlashSound[0] = trap->S_RegisterSound("sound/weapons/ee3/sniperfire.mp3");
		weaponInfo->altFiringSound = NULL_SOUND;
		weaponInfo->altChargeSound = NULL_SOUND;
		
		weaponInfo->altMissileModel = NULL_HANDLE;
		weaponInfo->altMissileRenderfx  = trap->FX_RegisterEffect( "ee3/sniper_shot" );
		weaponInfo->altMissileSound = NULL_SOUND;
		weaponInfo->altMissileDlight = 0;
		weaponInfo->altMissileHitSound = NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_BlasterProjectileThink;
		trap->FX_RegisterEffect("blaster/deflect");

		weaponInfo->missileRenderfx = trap->FX_RegisterEffect("ee3/blaster_shot");
		weaponInfo->altMuzzleEffect = trap->FX_RegisterEffect("blaster/muzzle_flash");
		weaponInfo->missileWallImpactfx = trap->FX_RegisterEffect("ee3/wall_impact");
		weaponInfo->wallImpactEffectEnhancedFX = trap->FX_RegisterEffect("flechette/wall_impact_enhanced2");
		weaponInfo->fleshImpactEffect = trap->FX_RegisterEffect("ee3/flesh_impact");
		weaponInfo->altFleshImpactEffect = trap->FX_RegisterEffect("blaster/droid_impact");
		weaponInfo->shotEffectFx = trap->FX_RegisterEffect("ee3/blaster_shot");

		//Need this later on for scope code
		cgs.media.GunRifleMask = trap->R_RegisterShaderNoMip("gfx/2D/arcMask");
		cgs.media.GunsMasks = trap->R_RegisterShaderNoMip("gfx/2d/a280cropCircle2");
		cgs.media.GunInsert = trap->R_RegisterShaderNoMip("gfx/2d/a280cropCircle");
		break;

	case WP_CLONE_PISTOL1:
		weaponInfo->item->classname = "Clone Pistol 1";
		weaponInfo->selectSound = trap->S_RegisterSound("sound/weapons/demp2/select.wav");
		weaponInfo->flashSound[0] = trap->S_RegisterSound("sound/weapons/demp2/fire.wav");
		weaponInfo->firingSound = NULL_SOUND;
		weaponInfo->chargeSound = NULL_SOUND;
		weaponInfo->muzzleEffect = trap->FX_RegisterEffect("clone_blaster/muzzle_flash");
		weaponInfo->missileModel = NULL_HANDLE;
		weaponInfo->missileRenderfx  = trap->FX_RegisterEffect("clone_westars/shot");
		weaponInfo->missileSound = NULL_SOUND;
		weaponInfo->missileDlight = 0;
		weaponInfo->missileHitSound = NULL_SOUND;
		weaponInfo->missileTrailFunc = FX_CLONEPISTOL_ProjectileThink;
		weaponInfo->powerupShotRenderfx = NULL_FX;
		weaponInfo->altFlashSound[0] = trap->S_RegisterSound("sound/weapons/demp2/altfire.wav");
		weaponInfo->altFiringSound = NULL_SOUND;
		weaponInfo->altChargeSound = trap->S_RegisterSound("sound/weapons/demp2/altCharge.wav");
		weaponInfo->altMuzzleEffect = trap->FX_RegisterEffect("clone_pistol_1/muzzle_flash");
		weaponInfo->altMissileModel = NULL_HANDLE;
		weaponInfo->altMissileRenderfx  = trap->FX_RegisterEffect("clone_westars/shot");
		weaponInfo->altMissileSound = NULL_SOUND;
		weaponInfo->altMissileDlight = 0;
		weaponInfo->altMissileHitSound = NULL_SOUND;
		weaponInfo->altMissileTrailFunc = 0;//FX_DEMP2_BounceWall;
		weaponInfo->missileWallImpactfx = trap->FX_RegisterEffect("clone_pistol_1/wall_impact");
		weaponInfo->wallImpactEffectEnhancedFX = trap->FX_RegisterEffect("clone_pistol_1/wall_impact_enhanced2");
		weaponInfo->shotEffectFx = trap->FX_RegisterEffect("clone_pistol_1/lvl3_shot");
		weaponInfo->WallBounceEffectEnhancedFX = trap->FX_RegisterEffect("clone_pistol_1/wall_bounce_enhanced2");
		weaponInfo->WallBounceEffectFX = trap->FX_RegisterEffect("clone_pistol_1/wall_bounce");
		weaponInfo->ProjectileEffectFX = trap->FX_RegisterEffect("clone_pistol_1/projectile");
		cgs.media.demp2Shell = trap->R_RegisterModel("models/items/sphere.md3");
		cgs.media.demp2ShellShader = trap->R_RegisterShader("gfx/effects/demp2shell");
		cgs.media.lightningFlash = trap->R_RegisterShader("gfx/misc/lightningFlash");
		break;

	case WP_DLT20A:
		weaponInfo->item->classname = "DLT-20a";
		weaponInfo->selectSound = trap->S_RegisterSound("sound/weapons/flechette/select.wav");
		weaponInfo->flashSound[0] = trap->S_RegisterSound("sound/weapons/A280/fire.wav");
		weaponInfo->firingSound = NULL_SOUND;
		weaponInfo->chargeSound = NULL_SOUND;
		weaponInfo->muzzleEffect = trap->FX_RegisterEffect("blaster/muzzle_flash");
		weaponInfo->missileModel = trap->R_RegisterModel("models/weapons3/golan_arms/projectileMain.md3");
		weaponInfo->missileSound = NULL_SOUND;
		weaponInfo->missileDlight = 0;
		weaponInfo->missileHitSound = NULL_SOUND;
		weaponInfo->missileTrailFunc = FX_T21ProjectileThink;
		weaponInfo->missileRenderfx = NULL_FX;
		weaponInfo->altMissileRenderfx = NULL_FX;
		weaponInfo->powerupShotRenderfx = NULL_FX;
		weaponInfo->altFlashSound[0] = trap->S_RegisterSound("sound/weapons/A280/alt_fire.wav");
		weaponInfo->altFiringSound = trap->S_RegisterSound("sound/weapons/A280/alt_fire.wav");
		weaponInfo->altChargeSound = NULL_SOUND;
		weaponInfo->altMuzzleEffect = trap->FX_RegisterEffect("blaster/muzzle_flash");
		weaponInfo->altMissileModel = trap->R_RegisterModel("models/weapons3/golan_arms/projectile.md3");
		weaponInfo->altMissileSound = NULL_SOUND;
		weaponInfo->altMissileDlight = 0;
		weaponInfo->altMissileHitSound = NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_FlechetteAltProjectileThink;

		weaponInfo->missileWallImpactfx = trap->FX_RegisterEffect("flechette/wall_impact");
		weaponInfo->altFleshImpactEffect = trap->FX_RegisterEffect("flechette/flesh_impact");
		weaponInfo->wallImpactEffectEnhancedFX = trap->FX_RegisterEffect("flechette/wall_impact_enhanced2");
		weaponInfo->shotEffectFx = trap->FX_RegisterEffect("flechette/shot");
		weaponInfo->altMissileRenderfx = trap->FX_RegisterEffect("flechette/alt_shot");
		cgs.effects.ee3ShotEffect = trap->FX_RegisterEffect("ee3/blaster_shot");
		cgs.effects.ee3AltShotEffect = trap->FX_RegisterEffect("ee3/sniper_shot");

		//Need this later on for scope code
		//cgs.media.GunRifleInsert = trap->R_RegisterShaderNoMip("gfx/2D/arcMask");
		cgs.media.GunRifleMask = trap->R_RegisterShaderNoMip("gfx/2d/projMask");
		cgs.media.GunInsert = trap->R_RegisterShaderNoMip("gfx/2d/projInsert");
		break;

	case WP_CLONERIFLE:
		weaponInfo->item->classname = "Clone Trooper Rifle";
		weaponInfo->selectSound = trap->S_RegisterSound("sound/weapons/repeater/select.wav");
		weaponInfo->flashSound[0] = trap->S_RegisterSound("sound/weapons/repeater/fire.wav");
		weaponInfo->firingSound = NULL_SOUND;
		weaponInfo->chargeSound = NULL_SOUND;
		weaponInfo->muzzleEffect = trap->FX_RegisterEffect("clone_blaster/muzzle_flash");
		weaponInfo->missileModel = NULL_HANDLE;
		weaponInfo->missileSound = NULL_SOUND;
		weaponInfo->missileDlight = 0;
		weaponInfo->missileHitSound = NULL_SOUND;
		weaponInfo->missileTrailFunc = FX_RepeaterProjectileThink;
		weaponInfo->missileRenderfx = trap->FX_RegisterEffect("clone_rifle/projectile");
		weaponInfo->altMissileRenderfx = trap->FX_RegisterEffect("clone_rifle/alt_projectile");
		weaponInfo->powerupShotRenderfx = NULL_FX;
		weaponInfo->altFlashSound[0] = trap->S_RegisterSound("sound/weapons/repeater/alt_fire.wav");
		weaponInfo->altFiringSound = NULL_SOUND;
		weaponInfo->altChargeSound = trap->S_RegisterSound("sound/weapons/SBDarm/cannon_charge.mp3");
		weaponInfo->altMuzzleEffect = trap->FX_RegisterEffect("clone_rifle/muzzle_flash");
		weaponInfo->altMissileModel = NULL_HANDLE;
		weaponInfo->altMissileSound = NULL_SOUND;
		weaponInfo->altMissileDlight = 0;
		weaponInfo->altMissileHitSound = NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_RepeaterAltProjectileThink;
		weaponInfo->spinSound = trap->S_RegisterSound("sound/weapons/z6/spinny.wav");
		weaponInfo->spindownSound = trap->S_RegisterSound("sound/weapons/z6/chaingun_spindown.wav");

		weaponInfo->missileWallImpactfx = trap->FX_RegisterEffect("clone_rifle/wall_impact");
		weaponInfo->altMissileWallImpactfx = trap->FX_RegisterEffect("clone_rifle/wall_impact_enhanced2"); // etc here.. then..
		weaponInfo->fleshImpactEffect = trap->FX_RegisterEffect("clone_rifle/flesh_impact");
		weaponInfo->altFleshImpactEffect = trap->FX_RegisterEffect("clone_rifle/concussion"); // not sure about this one
		weaponInfo->wallImpactEffectEnhancedFX = trap->FX_RegisterEffect("clone_rifle/wall_impact_enhanced2");
		weaponInfo->shotEffectFx = trap->FX_RegisterEffect("clone_rifle/shot");
		weaponInfo->explotionImpactEffect = trap->FX_RegisterEffect("clone_rifle/explosion");
		weaponInfo->ionBlastShotEffect = trap->FX_RegisterEffect("clone_rifle/ionblast");
		break;

	case WP_Z6_BLASTER_CANON:
		weaponInfo->item->classname = "Z-6 rotary blaster cannon";
		weaponInfo->selectSound = trap->S_RegisterSound("sound/weapons/repeater/select.wav");
		weaponInfo->flashSound[0] = trap->S_RegisterSound("sound/weapons/minigun/fire.wav");
		weaponInfo->firingSound = NULL_SOUND;
		weaponInfo->chargeSound = NULL_SOUND;
		weaponInfo->muzzleEffect = trap->FX_RegisterEffect("clone_blaster/muzzle_flash");
		weaponInfo->missileModel = NULL_HANDLE;
		weaponInfo->missileSound = NULL_SOUND;
		weaponInfo->missileDlight = 0;
		weaponInfo->missileHitSound = NULL_SOUND;
		weaponInfo->missileTrailFunc = FX_RepeaterProjectileThink;
		weaponInfo->missileRenderfx = trap->FX_RegisterEffect("clone_rifle/projectile");
		weaponInfo->altMissileRenderfx = trap->FX_RegisterEffect("clone_rifle/projectile");
		weaponInfo->powerupShotRenderfx = NULL_FX;
		weaponInfo->altFlashSound[0] = trap->S_RegisterSound("sound/weapons/repeater/alt_fire.wav");
		weaponInfo->altFiringSound = NULL_SOUND;
		weaponInfo->altChargeSound = trap->S_RegisterSound("sound/weapons/SBDarm/cannon_charge.mp3");
		weaponInfo->altMuzzleEffect = trap->FX_RegisterEffect("clone_rifle/muzzle_flash");
		weaponInfo->altMissileModel = NULL_HANDLE;
		weaponInfo->altMissileSound = NULL_SOUND;
		weaponInfo->altMissileDlight = 0;
		weaponInfo->altMissileHitSound = NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_RepeaterProjectileThink;
		weaponInfo->spinSound = trap->S_RegisterSound("sound/weapons/z6/spinny.wav");
		weaponInfo->spindownSound = trap->S_RegisterSound("sound/weapons/z6/chaingun_spindown.wav");

		weaponInfo->missileWallImpactfx = trap->FX_RegisterEffect("clone_rifle/wall_impact");
		weaponInfo->altMissileWallImpactfx = trap->FX_RegisterEffect("clone_rifle/wall_impact_enhanced2"); // etc here.. then..
		weaponInfo->fleshImpactEffect = trap->FX_RegisterEffect("clone_rifle/flesh_impact");
		weaponInfo->altFleshImpactEffect = trap->FX_RegisterEffect("clone_rifle/concussion"); // not sure about this one
		weaponInfo->wallImpactEffectEnhancedFX = trap->FX_RegisterEffect("clone_rifle/wall_impact_enhanced2");
		weaponInfo->shotEffectFx = trap->FX_RegisterEffect("clone_rifle/shot");
		break;

	case WP_BLASTER:
	case WP_EMPLACED_GUN: //rww - just use the same as this for now..
		weaponInfo->selectSound = trap->S_RegisterSound("sound/weapons/blaster/select.wav");
		weaponInfo->flashSound[0] = trap->S_RegisterSound("sound/weapons/blasterMB/fire.wav");
		weaponInfo->firingSound = NULL_SOUND;
		weaponInfo->chargeSound = NULL_SOUND;
		weaponInfo->muzzleEffect = trap->FX_RegisterEffect("blaster/muzzle_flash");
		weaponInfo->missileModel = NULL_HANDLE;
		weaponInfo->missileSound = NULL_SOUND;
		weaponInfo->missileDlight = 0;
		weaponInfo->missileHitSound = NULL_SOUND;
		weaponInfo->missileTrailFunc = FX_BlasterProjectileThink;
		weaponInfo->missileRenderfx = NULL_FX;
		weaponInfo->altMissileRenderfx = NULL_FX;
		weaponInfo->powerupShotRenderfx = NULL_FX;
		weaponInfo->altFlashSound[0] = trap->S_RegisterSound("sound/weapons/blasterMB/alt_fire.wav");
		weaponInfo->altFiringSound = NULL_SOUND;
		weaponInfo->altChargeSound = NULL_SOUND;
		weaponInfo->altMuzzleEffect = trap->FX_RegisterEffect("blaster/muzzle_flash");
		weaponInfo->altMissileModel = NULL_HANDLE;
		weaponInfo->altMissileSound = NULL_SOUND;
		weaponInfo->altMissileDlight = 0;
		weaponInfo->altMissileHitSound = NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_BlasterProjectileThink;

		weaponInfo->missileRenderfx = trap->FX_RegisterEffect("blaster/shot");
		weaponInfo->altMissileRenderfx = trap->FX_RegisterEffect("blaster/shot");
		weaponInfo->missileWallImpactfx = trap->FX_RegisterEffect("blaster/wall_impact");
		weaponInfo->altMissileWallImpactfx = trap->FX_RegisterEffect("blaster/wall_impact_enhanced2");
		weaponInfo->fleshImpactEffect = trap->FX_RegisterEffect("blaster/flesh_impact");
		weaponInfo->altFleshImpactEffect = trap->FX_RegisterEffect("blaster/flesh_impact");
		weaponInfo->wallImpactEffectEnhancedFX = trap->FX_RegisterEffect("blaster/wall_impact_enhanced2");
		weaponInfo->shotEffectFx = trap->FX_RegisterEffect("blaster/shot");

		trap->FX_RegisterEffect("blaster/deflect");
		cgs.effects.blasterShotEffect = trap->FX_RegisterEffect("blaster/shot");
		cgs.effects.blasterWallImpactEffect = trap->FX_RegisterEffect("blaster/wall_impact");
		cgs.effects.blasterWallImpactEffectEnhancedFX = trap->FX_RegisterEffect("blaster/wall_impact_enhanced2");
		cgs.effects.blasterFleshImpactEffect = trap->FX_RegisterEffect("blaster/flesh_impact");
		cgs.effects.blasterDroidImpactEffect = trap->FX_RegisterEffect("blaster/droid_impact");
		break;

	case WP_DISRUPTOR:
		weaponInfo->selectSound			= trap->S_RegisterSound("sound/weapons/disruptor/select.wav");
		weaponInfo->flashSound[0]		= trap->S_RegisterSound( "sound/weapons/disruptor/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap->FX_RegisterEffect( "disruptor/muzzle_flash" );
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= 0;
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/disruptor/alt_fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= trap->S_RegisterSound("sound/weapons/disruptor/altCharge.wav");
		weaponInfo->altMuzzleEffect		= trap->FX_RegisterEffect( "disruptor/muzzle_flash" );
		weaponInfo->altMissileModel		= NULL_HANDLE;
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = 0;
		cgs.effects.disruptorRingsEffect		= trap->FX_RegisterEffect( "disruptor/rings" );
		cgs.effects.disruptorProjectileEffect	= trap->FX_RegisterEffect( "disruptor/projectile" );
		cgs.effects.disruptorWallImpactEffect	= trap->FX_RegisterEffect( "disruptor/wall_impact" );
		cgs.effects.disruptorFleshImpactEffect	= trap->FX_RegisterEffect( "disruptor/flesh_impact" );
		cgs.effects.disruptorAltMissEffect		= trap->FX_RegisterEffect( "disruptor/alt_miss" );
		cgs.effects.disruptorAltHitEffect		= trap->FX_RegisterEffect( "disruptor/alt_hit" );
		trap->R_RegisterShader( "gfx/effects/redLine" );
		trap->R_RegisterShader( "gfx/misc/whiteline2" );
		trap->R_RegisterShader( "gfx/effects/smokeTrail" );
		trap->S_RegisterSound("sound/weapons/disruptor/zoomstart.wav");
		trap->S_RegisterSound("sound/weapons/disruptor/zoomend.wav");

		// Disruptor gun zoom interface
		cgs.media.disruptorMask			= trap->R_RegisterShader( "gfx/2d/cropCircle2");
		cgs.media.disruptorInsert		= trap->R_RegisterShader( "gfx/2d/cropCircle");
		cgs.media.disruptorLight		= trap->R_RegisterShader( "gfx/2d/cropCircleGlow" );
		cgs.media.disruptorInsertTick	= trap->R_RegisterShader( "gfx/2d/insertTick" );
		cgs.media.disruptorChargeShader	= trap->R_RegisterShaderNoMip("gfx/2d/crop_charge");
		cgs.media.disruptorZoomLoop		= trap->S_RegisterSound( "sound/weapons/disruptor/zoomloop.wav" );
		break;

	case WP_BOWCASTER:

		weaponInfo->altFlashSound[0] = NULL_SOUND;
		weaponInfo->altFiringSound = NULL_SOUND;
		weaponInfo->altChargeSound = NULL_SOUND;
		weaponInfo->altMuzzleEffect = trap->FX_RegisterEffect("blaster/muzzle_flash");
		weaponInfo->altMissileModel = NULL_HANDLE;
		weaponInfo->altMissileSound = NULL_SOUND;
		weaponInfo->altMissileDlight = 0;
		weaponInfo->altMissileHitSound = NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_BowcasterProjectileThink;

		weaponInfo->flashSound[0] = trap->S_RegisterSound("sound/weapons/bowcaster/fire.wav");
		weaponInfo->altFlashSound[0] = trap->S_RegisterSound("sound/weapons/bowcaster/fire.wav");
		weaponInfo->chargeSound = trap->S_RegisterSound("sound/weapons/bowcaster/altcharge.wav");
		weaponInfo->firingSound = NULL_SOUND;
		weaponInfo->muzzleEffect = trap->FX_RegisterEffect("blaster/muzzle_flash");
		weaponInfo->missileModel = NULL_HANDLE;
		weaponInfo->missileSound = NULL_SOUND;
		weaponInfo->missileDlight = 0;
		weaponInfo->missileHitSound = NULL_SOUND;
		weaponInfo->missileTrailFunc = FX_BowcasterAltProjectileThink;
		weaponInfo->missileRenderfx = NULL_FX;
		weaponInfo->altMissileRenderfx = NULL_FX;
		weaponInfo->powerupShotRenderfx = NULL_FX;
		

		cgs.effects.bowcasterShotEffect = trap->FX_RegisterEffect("bowcaster/shot");
		cgs.effects.bowcasterImpactEffect = trap->FX_RegisterEffect("bowcaster/wall_impact");
		cgs.effects.bowcasterImpactEffectEnhancedFX = trap->FX_RegisterEffect("bowcaster/wall_impact_enhanced2");
		cgs.media.greenFrontFlash = trap->R_RegisterShader("gfx/effects/redfrontflash");

		cgs.media.bowcasterMask = trap->R_RegisterShaderNoMip("gfx/2d/bowMask");
		cgs.media.bowcasterInsert = trap->R_RegisterShaderNoMip("gfx/2d/bowInsert");

		/*weaponInfo->selectSound			= trap->S_RegisterSound("sound/weapons/bowcaster/select.wav");
		weaponInfo->altFlashSound[0]		= trap->S_RegisterSound( "sound/weapons/bowcaster/fire.wav");
		weaponInfo->altFiringSound			= NULL_SOUND;
		weaponInfo->altChargeSound			= NULL_SOUND;
		weaponInfo->altMuzzleEffect		= trap->FX_RegisterEffect( "bowcaster/muzzle_flash" );
		weaponInfo->altMissileModel		= NULL_HANDLE;
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight		= 0;
		weaponInfo->altMissileHitSound		= NULL_SOUND;
		weaponInfo->altMissileTrailFunc	= FX_BowcasterProjectileThink;
		weaponInfo->flashSound[0]	= trap->S_RegisterSound( "sound/weapons/bowcaster/fire.wav");
		weaponInfo->firingSound		= NULL_SOUND;
		weaponInfo->chargeSound		= trap->S_RegisterSound( "sound/weapons/bowcaster/altcharge.wav");
		weaponInfo->muzzleEffect		= trap->FX_RegisterEffect( "bowcaster/muzzle_flash" );
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight	= 0;
		weaponInfo->missileHitSound	= NULL_SOUND;
		weaponInfo->missileTrailFunc = FX_BowcasterAltProjectileThink;
		cgs.effects.bowcasterShotEffect		= trap->FX_RegisterEffect( "bowcaster/shot" );
		cgs.effects.bowcasterImpactEffect	= trap->FX_RegisterEffect( "bowcaster/explosion" );
		trap->FX_RegisterEffect( "bowcaster/deflect" );
		cgs.media.greenFrontFlash = trap->R_RegisterShader( "gfx/effects/greenFrontFlash" );*/
		break;

	case WP_REPEATER:
		weaponInfo->selectSound			= trap->S_RegisterSound("sound/weapons/imperial_repeater/select.wav");
		weaponInfo->flashSound[0]		= trap->S_RegisterSound( "sound/weapons/imperial_repeater/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap->FX_RegisterEffect( "imperial_repeater/muzzle_flash" );
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_RepeaterProjectileThink;
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/imperial_repeater/alt_fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= NULL_SOUND;
		weaponInfo->altMuzzleEffect		= trap->FX_RegisterEffect( "imperial_repeater/muzzle_flash" );
		weaponInfo->altMissileModel		= NULL_HANDLE;
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_RepeaterAltProjectileThink;
		cgs.effects.repeaterProjectileEffect	= trap->FX_RegisterEffect( "imperial_repeater/projectile" );
		cgs.effects.repeaterAltProjectileEffect	= trap->FX_RegisterEffect( "imperial_repeater/alt_projectile" );
		cgs.effects.repeaterWallImpactEffect	= trap->FX_RegisterEffect( "imperial_repeater/wall_impact" );
		cgs.effects.repeaterFleshImpactEffect	= trap->FX_RegisterEffect( "imperial_repeater/flesh_impact" );
		//cgs.effects.repeaterAltWallImpactEffect	= trap->FX_RegisterEffect( "repeater/alt_wall_impact" );
		cgs.effects.repeaterAltWallImpactEffect	= trap->FX_RegisterEffect( "imperial_repeater/concussion" );
		cgs.effects.repeaterWallImpactEffectEnhancedFX = trap->FX_RegisterEffect("repeater/wall_impact_enhanced2");
		break;

	case WP_DEMP2:
		weaponInfo->selectSound			= trap->S_RegisterSound("sound/weapons/demp2/select.wav");
		weaponInfo->flashSound[0]		= trap->S_RegisterSound("sound/weapons/demp2/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap->FX_RegisterEffect("demp2/muzzle_flash");
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_DEMP2_ProjectileThink;
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound("sound/weapons/demp2/altfire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= trap->S_RegisterSound("sound/weapons/demp2/altCharge.wav");
		weaponInfo->altMuzzleEffect		= trap->FX_RegisterEffect("demp2/muzzle_flash");
		weaponInfo->altMissileModel		= NULL_HANDLE;
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = 0;
		cgs.effects.demp2ProjectileEffect		= trap->FX_RegisterEffect( "demp2/projectile" );
		cgs.effects.demp2WallImpactEffect		= trap->FX_RegisterEffect( "demp2/wall_impact" );
		cgs.effects.demp2FleshImpactEffect		= trap->FX_RegisterEffect( "demp2/flesh_impact" );
		cgs.media.demp2Shell = trap->R_RegisterModel( "models/items/sphere.md3" );
		cgs.media.demp2ShellShader = trap->R_RegisterShader( "gfx/effects/demp2shell" );
		cgs.media.lightningFlash = trap->R_RegisterShader("gfx/misc/lightningFlash");
		break;

	case WP_FLECHETTE:
		weaponInfo->selectSound			= trap->S_RegisterSound("sound/weapons/flechette/select.wav");
		weaponInfo->flashSound[0]		= trap->S_RegisterSound( "sound/weapons/flechette/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap->FX_RegisterEffect( "flechette/muzzle_flash" );
		weaponInfo->missileModel		= trap->R_RegisterModel("models/weapons2/golan_arms/projectileMain.md3");
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_FlechetteProjectileThink;
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/flechette/alt_fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= NULL_SOUND;
		weaponInfo->altMuzzleEffect		= trap->FX_RegisterEffect( "flechette/muzzle_flash" );
		weaponInfo->altMissileModel		= trap->R_RegisterModel( "models/weapons2/golan_arms/projectile.md3" );
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_FlechetteAltProjectileThink;
		cgs.effects.flechetteShotEffect			= trap->FX_RegisterEffect( "flechette/shot" );
		cgs.effects.flechetteAltShotEffect		= trap->FX_RegisterEffect( "flechette/alt_shot" );
		cgs.effects.flechetteWallImpactEffect	= trap->FX_RegisterEffect( "flechette/wall_impact" );
		cgs.effects.flechetteFleshImpactEffect	= trap->FX_RegisterEffect( "flechette/flesh_impact" );
		break;

	case WP_ROCKET_LAUNCHER:
		weaponInfo->selectSound			= trap->S_RegisterSound("sound/weapons/rocket/select.wav");
		weaponInfo->flashSound[0]		= trap->S_RegisterSound( "sound/weapons/rocket/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= trap->FX_RegisterEffect( "rocket/muzzle_flash" ); 
		weaponInfo->missileModel		= trap->R_RegisterModel( "models/weapons2/merr_sonn/projectile.md3" );
		weaponInfo->missileSound		= trap->S_RegisterSound( "sound/weapons/rocket/missleloop.wav");
		weaponInfo->missileDlight		= 125;
		VectorSet(weaponInfo->missileDlightColor, 1.0, 1.0, 0.5);
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_RocketProjectileThink;
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/rocket/alt_fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= NULL_SOUND;
		weaponInfo->altMuzzleEffect		= trap->FX_RegisterEffect( "rocket/altmuzzle_flash" );
		weaponInfo->altMissileModel		= trap->R_RegisterModel( "models/weapons2/merr_sonn/projectile.md3" );
		weaponInfo->altMissileSound		= trap->S_RegisterSound( "sound/weapons/rocket/missleloop.wav");
		weaponInfo->altMissileDlight	= 125;
		VectorSet(weaponInfo->altMissileDlightColor, 1.0, 1.0, 0.5);
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = FX_RocketAltProjectileThink;
		cgs.effects.rocketShotEffect			= trap->FX_RegisterEffect( "rocket/shot" );
		cgs.effects.rocketExplosionEffect		= trap->FX_RegisterEffect( "rocket/explosion" );
		trap->R_RegisterShaderNoMip( "gfx/2d/wedge" );
		trap->R_RegisterShaderNoMip( "gfx/2d/lock" );
		trap->S_RegisterSound( "sound/weapons/rocket/lock.wav" );
		trap->S_RegisterSound( "sound/weapons/rocket/tick.wav" );
		break;

	case WP_THERMAL:
		weaponInfo->selectSound			= trap->S_RegisterSound("sound/weapons/thermal/select.wav");
		weaponInfo->flashSound[0]		= trap->S_RegisterSound( "sound/weapons/thermal/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= trap->S_RegisterSound( "sound/weapons/thermal/charge.wav");
		weaponInfo->muzzleEffect		= NULL_FX;
		weaponInfo->missileModel		= trap->R_RegisterModel( "models/weapons2/thermal/thermal_proj.md3" );
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= 0;
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/thermal/fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= trap->S_RegisterSound( "sound/weapons/thermal/charge.wav");
		weaponInfo->altMuzzleEffect		= NULL_FX;
		weaponInfo->altMissileModel		= trap->R_RegisterModel( "models/weapons2/thermal/thermal_proj.md3" );
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = 0;
		/*cgs.effects.thermalExplosionEffect		= trap->FX_RegisterEffect( "thermal/explosion" );
		cgs.effects.thermalShockwaveEffect		= trap->FX_RegisterEffect( "thermal/shockwave" );*/
		cgs.effects.thermalShockwaveEffect = trap->FX_RegisterEffect("thermal/shockwave");
		cgs.effects.thermalExplosionEffectEnhancedFX = trap->FX_RegisterEffect("ships/mine_impact_enhanced2");
		cgs.effects.thermalExplosionAltEffect = trap->FX_RegisterEffect("ships/swoop_explosion");
		cgs.effects.thermalExplosionAltEffectEnhancedFX = trap->FX_RegisterEffect("ships/swoop_explosion_enhanced2");

		cgs.media.grenadeBounce1		= trap->S_RegisterSound( "sound/weapons/thermal/bounce1.wav" );
		cgs.media.grenadeBounce2		= trap->S_RegisterSound( "sound/weapons/thermal/bounce2.wav" );
		trap->S_RegisterSound( "sound/weapons/thermal/thermloop.wav" );
		trap->S_RegisterSound( "sound/weapons/thermal/warning.wav" );
		break;

	case WP_TRIP_MINE:
		weaponInfo->selectSound			= trap->S_RegisterSound("sound/weapons/detpack/select.wav");
		weaponInfo->flashSound[0]		= trap->S_RegisterSound( "sound/weapons/laser_trap/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= NULL_FX;
		weaponInfo->missileModel		= 0;//trap->R_RegisterModel( "models/weapons2/laser_trap/laser_trap->w.md3" );
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= 0;
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/laser_trap/fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= NULL_SOUND;
		weaponInfo->altMuzzleEffect		= NULL_FX;
		weaponInfo->altMissileModel		= 0;//trap->R_RegisterModel( "models/weapons2/laser_trap/laser_trap->w.md3" );
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = 0;
		cgs.effects.tripmineLaserFX = trap->FX_RegisterEffect("tripMine/laserMP.efx");
		cgs.effects.tripmineGlowFX = trap->FX_RegisterEffect("tripMine/glowbit.efx");
		trap->FX_RegisterEffect( "tripMine/explosion" );
		// NOTENOTE temp stuff
		trap->S_RegisterSound( "sound/weapons/laser_trap/stick.wav" );
		trap->S_RegisterSound( "sound/weapons/laser_trap/warning.wav" );
		break;

	case WP_DET_PACK:
		weaponInfo->selectSound			= trap->S_RegisterSound("sound/weapons/detpack/select.wav");
		weaponInfo->flashSound[0]		= trap->S_RegisterSound( "sound/weapons/detpack/fire.wav");
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= NULL_FX;
		weaponInfo->missileModel		= trap->R_RegisterModel( "models/weapons2/detpack/det_pack.md3" );
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= 0;
		weaponInfo->altFlashSound[0]	= trap->S_RegisterSound( "sound/weapons/detpack/fire.wav");
		weaponInfo->altFiringSound		= NULL_SOUND;
		weaponInfo->altChargeSound		= NULL_SOUND;
		weaponInfo->altMuzzleEffect		= NULL_FX;
		weaponInfo->altMissileModel		= trap->R_RegisterModel( "models/weapons2/detpack/det_pack.md3" );
		weaponInfo->altMissileSound		= NULL_SOUND;
		weaponInfo->altMissileDlight	= 0;
		weaponInfo->altMissileHitSound	= NULL_SOUND;
		weaponInfo->altMissileTrailFunc = 0;
		trap->R_RegisterModel( "models/weapons2/detpack/det_pack.md3" );
		trap->S_RegisterSound( "sound/weapons/detpack/stick.wav" );
		trap->S_RegisterSound( "sound/weapons/detpack/warning.wav" );
		trap->S_RegisterSound( "sound/weapons/explosions/explode5.wav" );
		break;

	case WP_TURRET:
		weaponInfo->flashSound[0]		= NULL_SOUND;
		weaponInfo->firingSound			= NULL_SOUND;
		weaponInfo->chargeSound			= NULL_SOUND;
		weaponInfo->muzzleEffect		= NULL_HANDLE;
		weaponInfo->missileModel		= NULL_HANDLE;
		weaponInfo->missileSound		= NULL_SOUND;
		weaponInfo->missileDlight		= 0;
		weaponInfo->missileHitSound		= NULL_SOUND;
		weaponInfo->missileTrailFunc	= FX_TurretProjectileThink;
		trap->FX_RegisterEffect("effects/blaster/wall_impact.efx");
		trap->FX_RegisterEffect("effects/blaster/flesh_impact.efx");
		break;
	 default:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 1 );
		weaponInfo->flashSound[0] = trap->S_RegisterSound( "sound/weapons/rocket/rocklf1a.wav" );
		break;
	}
}
