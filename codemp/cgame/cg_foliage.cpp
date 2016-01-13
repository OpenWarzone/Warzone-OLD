extern "C" {
#include "../qcommon/q_shared.h"
#include "../cgame/cg_local.h"
#include "../ui/ui_shared.h"
#include "../game/surfaceflags.h"
#include "../qcommon/inifile.h"

extern qboolean InFOV( vec3_t spot, vec3_t from, vec3_t fromAngles, int hFOV, int vFOV );

	// =======================================================================================================================================
	//
	//                                                             Foliage Rendering...
	//
	// =======================================================================================================================================


#define			FOLIAGE_MAX_FOLIAGES 1048576

	//
	// BEGIN - FOLIAGE OPTIONS
	//

	//#define		__NO_PLANTS__ // Disable plants...

#define			__FOLIAGE_DENSITY__ cg_foliageDensity.value
	//
	// END - FOLIAGE OPTIONS
	//

float		NUM_PLANT_SHADERS = 0;

#define		PLANT_SCALE_MULTIPLIER 1.0

#define		MAX_PLANT_SHADERS 100

	static const char *GoodPlantsList[] = {
		"models/warzone/foliage/plant01.png",
		"models/warzone/foliage/plant02.png",
		"models/warzone/foliage/plant03.png",
		"models/warzone/foliage/plant04.png",
		"models/warzone/foliage/plant05.png",
		"models/warzone/foliage/plant06.png",
		"models/warzone/foliage/plant07.png",
		"models/warzone/foliage/plant08.png",
		"models/warzone/foliage/plant09.png",
		"models/warzone/foliage/plant10.png",
		"models/warzone/foliage/plant11.png",
		"models/warzone/foliage/plant12.png",
		"models/warzone/foliage/plant13.png",
		"models/warzone/foliage/plant14.png",
		"models/warzone/foliage/plant15.png",
		"models/warzone/foliage/plant16.png",
		"models/warzone/foliage/plant17.png",
		"models/warzone/foliage/plant18.png",
		"models/warzone/foliage/plant19.png",
		"models/warzone/foliage/plant20.png",
		"models/warzone/foliage/plant21.png",
		"models/warzone/foliage/plant22.png",
		"models/warzone/foliage/plant23.png",
		"models/warzone/foliage/plant24.png",
		"models/warzone/foliage/plant25.png",
		"models/warzone/foliage/plant26.png",
		"models/warzone/foliage/plant27.png",
		"models/warzone/foliage/plant28.png",
		"models/warzone/foliage/plant29.png",
		"models/warzone/foliage/plant30.png",
		"models/warzone/foliage/plant31.png",
		"models/warzone/foliage/plant32.png",
		"models/warzone/foliage/plant33.png",
		"models/warzone/foliage/plant34.png",
		"models/warzone/foliage/plant35.png",
		"models/warzone/foliage/plant36.png",
		"models/warzone/foliage/plant37.png",
		"models/warzone/foliage/plant38.png",
		"models/warzone/foliage/plant39.png",
		"models/warzone/foliage/plant40.png",
		"models/warzone/foliage/plant41.png",
		"models/warzone/foliage/plant42.png",
		"models/warzone/foliage/plant43.png",
		"models/warzone/foliage/plant44.png",
		"models/warzone/foliage/plant45.png",
		"models/warzone/foliage/plant46.png",
		"models/warzone/foliage/plant47.png",
		"models/warzone/foliage/plant48.png",
		"models/warzone/foliage/plant49.png",
		"models/warzone/foliage/plant50.png",
		"models/warzone/foliage/plant51.png",
		"models/warzone/foliage/plant52.png",
		"models/warzone/foliage/plant53.png",
		"models/warzone/foliage/plant54.png",
		"models/warzone/foliage/plant55.png",
		"models/warzone/foliage/plant56.png",
		"models/warzone/foliage/plant57.png",
		"models/warzone/foliage/plant58.png",
		"models/warzone/foliage/plant59.png",
		"models/warzone/foliage/plant60.png",
		"models/warzone/foliage/plant61.png",
		"models/warzone/foliage/plant62.png",
		"models/warzone/foliage/plant63.png",
		"models/warzone/foliage/plant64.png",
		"models/warzone/foliage/plant65.png",
		"models/warzone/foliage/plant66.png",
		"models/warzone/foliage/plant67.png",
		"models/warzone/foliage/plant68.png",
		"models/warzone/foliage/plant69.png",
		"models/warzone/foliage/plant70.png",
		"models/warzone/foliage/plant71.png",
		"models/warzone/foliage/plant72.png",
		"models/warzone/foliage/plant73.png",
		"models/warzone/foliage/plant74.png",

		// More of these because they add color
		"models/warzone/foliage/plant60.png",
		"models/warzone/foliage/plant61.png",
		"models/warzone/foliage/plant62.png",
		"models/warzone/foliage/plant63.png",
		"models/warzone/foliage/plant67.png",
		"models/warzone/foliage/plant69.png",
		"models/warzone/foliage/plant70.png",
		"models/warzone/foliage/plant60.png",
		"models/warzone/foliage/plant61.png",
		"models/warzone/foliage/plant62.png",
		"models/warzone/foliage/plant63.png",
		"models/warzone/foliage/plant67.png",
		"models/warzone/foliage/plant69.png",
		"models/warzone/foliage/plant70.png",
		"models/warzone/foliage/plant60.png",
		"models/warzone/foliage/plant61.png",
		"models/warzone/foliage/plant62.png",
		"models/warzone/foliage/plant63.png",
		"models/warzone/foliage/plant67.png",
		"models/warzone/foliage/plant69.png",
		"models/warzone/foliage/plant70.png",
		"models/warzone/foliage/plant60.png",
		"models/warzone/foliage/plant61.png",
		"models/warzone/foliage/plant62.png",
		"models/warzone/foliage/plant63.png",
		"models/warzone/foliage/plant67.png",
	};

	float		FOLIAGE_AREA_SIZE =				512;
	float		FOLIAGE_VISIBLE_DISTANCE =		FOLIAGE_AREA_SIZE*cg_foliageGrassRangeMult.value;
	float		FOLIAGE_TREE_VISIBLE_DISTANCE = FOLIAGE_AREA_SIZE*cg_foliageTreeRangeMult.value;

#define		FOLIAGE_AREA_MAX				65550
#define		FOLIAGE_AREA_MAX_FOLIAGES		256

	int			FOLIAGE_AREAS_COUNT = 0;
	int			FOLIAGE_AREAS_LIST_COUNT[FOLIAGE_AREA_MAX];
	int			FOLIAGE_AREAS_LIST[FOLIAGE_AREA_MAX][FOLIAGE_AREA_MAX_FOLIAGES];
	vec3_t		FOLIAGE_AREAS_MINS[FOLIAGE_AREA_MAX];
	vec3_t		FOLIAGE_AREAS_MAXS[FOLIAGE_AREA_MAX];


	qboolean	FOLIAGE_LOADED = qfalse;
	int			FOLIAGE_NUM_POSITIONS = 0;
	vec3_t		FOLIAGE_POSITIONS[FOLIAGE_MAX_FOLIAGES];
	vec3_t		FOLIAGE_NORMALS[FOLIAGE_MAX_FOLIAGES];
	int			FOLIAGE_PLANT_SELECTION[FOLIAGE_MAX_FOLIAGES];
	float		FOLIAGE_PLANT_ANGLES[FOLIAGE_MAX_FOLIAGES];
	float		FOLIAGE_PLANT_SCALE[FOLIAGE_MAX_FOLIAGES];
	int			FOLIAGE_TREE_SELECTION[FOLIAGE_MAX_FOLIAGES];
	float		FOLIAGE_TREE_ANGLES[FOLIAGE_MAX_FOLIAGES];
	float		FOLIAGE_TREE_SCALE[FOLIAGE_MAX_FOLIAGES];


	qhandle_t	FOLIAGE_PLANT_MODEL[3] = { 0 };
	qhandle_t	FOLIAGE_GRASS_BILLBOARD_SHADER[5] = { 0 };
	qhandle_t	FOLIAGE_PLANT_BILLBOARD_MODEL[27] = { 0 };
	qhandle_t	FOLIAGE_TREE_MODEL[3] = { 0 };
	float		FOLIAGE_TREE_RADIUS[3] = { 0 };
	float		FOLIAGE_TREE_ZOFFSET[3] = { 0 };
	qhandle_t	FOLIAGE_TREE_BILLBOARD_SHADER[3] = { 0 };
	float		FOLIAGE_TREE_BILLBOARD_SIZE[3] = { 0 };

	qhandle_t	FOLIAGE_PLANT_SHADERS[MAX_PLANT_SHADERS] = {0};

	int IN_RANGE_AREAS_LIST_COUNT = 0;
	int IN_RANGE_AREAS_LIST[1024];
	int IN_RANGE_TREE_AREAS_LIST_COUNT = 0;
	int IN_RANGE_TREE_AREAS_LIST[8192];

#define FOLIAGE_SOLID_TREES_MAX 4
	int			FOLIAGE_SOLID_TREES[FOLIAGE_SOLID_TREES_MAX];
	float		FOLIAGE_SOLID_TREES_DIST[FOLIAGE_SOLID_TREES_MAX];

	float OLD_FOLIAGE_DENSITY = 64.0;

	qboolean FOLIAGE_In_Bounds( int areaNum, int foliageNum )
	{
		if (foliageNum >= FOLIAGE_NUM_POSITIONS) return qfalse;

		if (FOLIAGE_AREAS_MINS[areaNum][0] < FOLIAGE_POSITIONS[foliageNum][0]
		&& FOLIAGE_AREAS_MINS[areaNum][1] < FOLIAGE_POSITIONS[foliageNum][1]
		&& FOLIAGE_AREAS_MAXS[areaNum][0] >= FOLIAGE_POSITIONS[foliageNum][0]
		&& FOLIAGE_AREAS_MAXS[areaNum][1] >= FOLIAGE_POSITIONS[foliageNum][1])
		{
			return qtrue;
		}

		return qfalse;
	}

	void FOLIAGE_Setup_Foliage_Areas( void )
	{
		int		areaNum = 0, i = 0;
		vec3_t	mins, maxs, mapMins, mapMaxs;

		VectorSet(mapMins, 128000, 128000, 0);
		VectorSet(mapMaxs, -128000, -128000, 0);

		// Find map bounds first... Reduce area numbers...
		for (i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
		{
			if (FOLIAGE_POSITIONS[i][0] < mapMins[0])
				mapMins[0] = FOLIAGE_POSITIONS[i][0];

			if (FOLIAGE_POSITIONS[i][0] > mapMaxs[0])
				mapMaxs[0] = FOLIAGE_POSITIONS[i][0];

			if (FOLIAGE_POSITIONS[i][1] < mapMins[1])
				mapMins[1] = FOLIAGE_POSITIONS[i][1];

			if (FOLIAGE_POSITIONS[i][1] > mapMaxs[1])
				mapMaxs[1] = FOLIAGE_POSITIONS[i][1];
		}

		mapMins[0] -= 1024.0;
		mapMins[1] -= 1024.0;
		mapMaxs[0] += 1024.0;
		mapMaxs[1] += 1024.0;

		VectorSet(mins, mapMins[0], mapMins[1], 0);
		VectorSet(maxs, mapMins[0] + FOLIAGE_AREA_SIZE, mapMins[1] + FOLIAGE_AREA_SIZE, 0);

		int DENSITY_REMOVED = 0;
		FOLIAGE_AREAS_COUNT = 0;

		for (areaNum = 0; areaNum < FOLIAGE_AREA_MAX; areaNum++)
		{
			if (mins[1] > mapMaxs[1]) break; // found our last area...

			FOLIAGE_AREAS_LIST_COUNT[areaNum] = 0;

			while (FOLIAGE_AREAS_LIST_COUNT[areaNum] == 0 && mins[1] <= mapMaxs[1])
			{// While loop is so we can skip zero size areas for speed...
				VectorCopy(mins, FOLIAGE_AREAS_MINS[areaNum]);
				VectorCopy(maxs, FOLIAGE_AREAS_MAXS[areaNum]);

				// Assign foliages to the area lists...
				for (i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
				{
					if (FOLIAGE_In_Bounds(areaNum, i))
					{
						qboolean OVER_DENSITY = qfalse;

						if (FOLIAGE_AREAS_LIST_COUNT[areaNum] > FOLIAGE_AREA_MAX_FOLIAGES)
						{
							//trap->Print("*** Area %i has more then %i foliages ***\n", areaNum, (int)FOLIAGE_AREA_MAX_FOLIAGES);
							break;
						}

						if (FOLIAGE_TREE_SELECTION[i] <= 0)
						{// Never remove trees...
							for (int j = 0; j < FOLIAGE_AREAS_LIST_COUNT[areaNum]; j++)
							{// Let's use a density setting to improve FPS...
								if (DistanceHorizontal(FOLIAGE_POSITIONS[i], FOLIAGE_POSITIONS[FOLIAGE_AREAS_LIST[areaNum][j]]) < __FOLIAGE_DENSITY__)
								{// Adding this would go over density setting...
									OVER_DENSITY = qtrue;
									DENSITY_REMOVED++;
									break;
								}
							}
						}

						if (!OVER_DENSITY)
						{
							FOLIAGE_AREAS_LIST[areaNum][FOLIAGE_AREAS_LIST_COUNT[areaNum]] = i;
							FOLIAGE_AREAS_LIST_COUNT[areaNum]++;
						}
					}
				}

				mins[0] += FOLIAGE_AREA_SIZE;
				maxs[0] = mins[0] + FOLIAGE_AREA_SIZE;

				if (mins[0] > mapMaxs[0])
				{
					mins[0] = mapMins[0];
					maxs[0] = mapMins[0] + FOLIAGE_AREA_SIZE;

					mins[1] += FOLIAGE_AREA_SIZE;
					maxs[1] = mins[1] + FOLIAGE_AREA_SIZE;
				}
			}
		}

		FOLIAGE_AREAS_COUNT = areaNum;
		OLD_FOLIAGE_DENSITY = __FOLIAGE_DENSITY__;

		trap->Print("Generated %i foliage areas. %i used of %i total foliages. %i removed by density setting.\n", FOLIAGE_AREAS_COUNT, FOLIAGE_NUM_POSITIONS - DENSITY_REMOVED, FOLIAGE_NUM_POSITIONS, DENSITY_REMOVED);
	}

	void FOLIAGE_Check_CVar_Change ( void )
	{
		if (__FOLIAGE_DENSITY__ != OLD_FOLIAGE_DENSITY)
		{
			FOLIAGE_Setup_Foliage_Areas();
		}
	}
}

qboolean FOLIAGE_Box_In_FOV ( vec3_t mins, vec3_t maxs )
{
	vec3_t edge, edge2;

	VectorSet(edge, maxs[0], mins[1], maxs[2]);
	VectorSet(edge2, mins[0], maxs[1], maxs[2]);

	if (!InFOV( mins, cg.refdef.vieworg, cg.refdef.viewangles, 100, 180 )
		&& !InFOV( maxs, cg.refdef.vieworg, cg.refdef.viewangles, 100, 180 )
		&& !InFOV( edge, cg.refdef.vieworg, cg.refdef.viewangles, 100, 180 )
		&& !InFOV( edge2, cg.refdef.vieworg, cg.refdef.viewangles, 100, 180 ))
		return qfalse;

	return qtrue;
}

vec3_t		LAST_ORG = { 0 };
vec3_t		LAST_ANG = { 0 };

void FOLIAGE_Calc_In_Range_Areas( void )
{
	for (int i = 0; i < FOLIAGE_SOLID_TREES_MAX; i++)
	{
		FOLIAGE_SOLID_TREES[i] = -1;
		FOLIAGE_SOLID_TREES_DIST[i] = 65536.0;
	}

	FOLIAGE_VISIBLE_DISTANCE =		FOLIAGE_AREA_SIZE*cg_foliageGrassRangeMult.value;
	FOLIAGE_TREE_VISIBLE_DISTANCE = FOLIAGE_AREA_SIZE*cg_foliageTreeRangeMult.value;

	if (cg_foliageTreeRangeMult.value < 8.0) 
	{
		FOLIAGE_TREE_VISIBLE_DISTANCE = FOLIAGE_AREA_SIZE*8.0;
		trap->Cvar_Set("cg_foliageTreeRangeMult", "8.0");
		trap->Print("WARNING: Minimum tree range multiplier is 8.0. Cvar has been changed.\n");
	}

	if (Distance(cg.refdef.vieworg, LAST_ORG) > 128.0 || Distance(cg.refdef.viewangles, LAST_ANG) > 50.0)
	{// Update in range list...
		VectorCopy(cg.refdef.vieworg, LAST_ORG);
		VectorCopy(cg.refdef.viewangles, LAST_ANG);

		IN_RANGE_AREAS_LIST_COUNT = 0;
		IN_RANGE_TREE_AREAS_LIST_COUNT = 0;

		// Calculate currently-in-range areas to use...
		for (int i = 0; i < FOLIAGE_AREAS_COUNT; i++)
		{
			if (DistanceHorizontal(FOLIAGE_AREAS_MINS[i], cg.refdef.vieworg) < FOLIAGE_VISIBLE_DISTANCE 
				|| DistanceHorizontal(FOLIAGE_AREAS_MAXS[i], cg.refdef.vieworg) < FOLIAGE_VISIBLE_DISTANCE)
			{
				if (FOLIAGE_Box_In_FOV( FOLIAGE_AREAS_MINS[i], FOLIAGE_AREAS_MAXS[i] ))
				{
					IN_RANGE_AREAS_LIST[IN_RANGE_AREAS_LIST_COUNT] = i;
					IN_RANGE_AREAS_LIST_COUNT++;
				}
			}
			else if (DistanceHorizontal(FOLIAGE_AREAS_MINS[i], cg.refdef.vieworg) < FOLIAGE_TREE_VISIBLE_DISTANCE
				|| DistanceHorizontal(FOLIAGE_AREAS_MAXS[i], cg.refdef.vieworg) < FOLIAGE_TREE_VISIBLE_DISTANCE)
			{
				if (FOLIAGE_Box_In_FOV( FOLIAGE_AREAS_MINS[i], FOLIAGE_AREAS_MAXS[i] ))
				{
					IN_RANGE_TREE_AREAS_LIST[IN_RANGE_TREE_AREAS_LIST_COUNT] = i;
					IN_RANGE_TREE_AREAS_LIST_COUNT++;
				}
			}
		}

		//trap->Print("There are %i foliage areas in range. %i tree areas.\n", IN_RANGE_AREAS_LIST_COUNT, IN_RANGE_TREE_AREAS_LIST_COUNT);
	}
}

extern "C" {
	qboolean FOLIAGE_TreeSolidBlocking_AWP(vec3_t moveOrg)
	{
		int	CLOSE_AREA_LIST[8192];
		int	CLOSE_AREA_LIST_COUNT = 0;

		for (int areaNum = 0; areaNum < FOLIAGE_AREAS_COUNT; areaNum++)
		{
			float DIST = DistanceHorizontal(FOLIAGE_AREAS_MINS[areaNum], moveOrg);
			float DIST2 = DistanceHorizontal(FOLIAGE_AREAS_MAXS[areaNum], moveOrg);

			if (DIST < FOLIAGE_AREA_SIZE * 2.0 || DIST2 < FOLIAGE_AREA_SIZE * 2.0)
			{
				CLOSE_AREA_LIST[CLOSE_AREA_LIST_COUNT] = areaNum;
				CLOSE_AREA_LIST_COUNT++;
			}
		}

		for (int areaListPos = 0; areaListPos < CLOSE_AREA_LIST_COUNT; areaListPos++)
		{
			int areaNum = CLOSE_AREA_LIST[areaListPos];

			for (int treeNum = 0; treeNum < FOLIAGE_AREAS_LIST_COUNT[areaNum]; treeNum++)
			{
				int		THIS_TREE_NUM = FOLIAGE_AREAS_LIST[areaNum][treeNum];
				int		THIS_TREE_TYPE = FOLIAGE_TREE_SELECTION[THIS_TREE_NUM]-1;
				float	TREE_RADIUS = FOLIAGE_TREE_RADIUS[THIS_TREE_TYPE] * FOLIAGE_TREE_SCALE[THIS_TREE_NUM];
				float	DIST = DistanceHorizontal(FOLIAGE_POSITIONS[THIS_TREE_NUM], moveOrg);

				TREE_RADIUS += 64.0; // Extra space around the tree for player body to fit as well...

				if (FOLIAGE_TREE_SELECTION[THIS_TREE_NUM] > 0 && DIST <= TREE_RADIUS)
				{
					return qtrue;
				}
			}
		}

		return qfalse;
	}

	qboolean FOLIAGE_TreeSolidBlocking_AWP_Path(vec3_t from, vec3_t to)
	{
		int	CLOSE_AREA_LIST[8192];
		int	CLOSE_AREA_LIST_COUNT = 0;

		float fullDist = DistanceHorizontal(from, to);

		for (int areaNum = 0; areaNum < FOLIAGE_AREAS_COUNT; areaNum++)
		{
			float DIST = DistanceHorizontal(FOLIAGE_AREAS_MINS[areaNum], to);
			float DIST2 = DistanceHorizontal(FOLIAGE_AREAS_MAXS[areaNum], to);

			if (DIST < FOLIAGE_AREA_SIZE * 2.0 || DIST2 < FOLIAGE_AREA_SIZE * 2.0)
			{
				CLOSE_AREA_LIST[CLOSE_AREA_LIST_COUNT] = areaNum;
				CLOSE_AREA_LIST_COUNT++;
			}
		}

		vec3_t dir, angles, forward;
		VectorSubtract( to, from, dir );
		vectoangles(dir, angles);
		AngleVectors( angles, forward, NULL, NULL );

		for (int areaListPos = 0; areaListPos < CLOSE_AREA_LIST_COUNT; areaListPos++)
		{
			int areaNum = CLOSE_AREA_LIST[areaListPos];

			for (int treeNum = 0; treeNum < FOLIAGE_AREAS_LIST_COUNT[areaNum]; treeNum++)
			{
				int		THIS_TREE_NUM = FOLIAGE_AREAS_LIST[areaNum][treeNum];
				int		THIS_TREE_TYPE = FOLIAGE_TREE_SELECTION[THIS_TREE_NUM]-1;
				float	TREE_RADIUS = FOLIAGE_TREE_RADIUS[THIS_TREE_TYPE] * FOLIAGE_TREE_SCALE[THIS_TREE_NUM];
				float	DIST = DistanceHorizontal(FOLIAGE_POSITIONS[THIS_TREE_NUM], from);

				if (FOLIAGE_TREE_SELECTION[THIS_TREE_NUM] <= 0) continue;
				if ( fullDist < DIST ) continue;

				TREE_RADIUS += 64.0; // Extra space around the tree for player body to fit as well...

				// Check at positions along this path...
				for (int test = 8; test < DIST; test += 8)
				{
					vec3_t pos;
					
					VectorMA( from, test, forward, pos );

					float DIST2 = DistanceHorizontal(FOLIAGE_POSITIONS[THIS_TREE_NUM], pos);

					if (DIST2 <= TREE_RADIUS)
					{
						return qtrue;
					}
				}
			}
		}

		return qfalse;
	}

	qboolean FOLIAGE_TreeSolidBlocking(vec3_t moveOrg)
	{
		if (cg.predictedPlayerState.pm_type == PM_SPECTATOR) return qfalse;
		if (cgs.clientinfo[cg.clientNum].team == FACTION_SPECTATOR) return qfalse;

		for (int tree = 0; tree < FOLIAGE_SOLID_TREES_MAX; tree++)
		{
			if (FOLIAGE_SOLID_TREES[tree] >= 0)
			{
				int	  THIS_TREE_NUM = FOLIAGE_SOLID_TREES[tree];
				int	  THIS_TREE_TYPE = FOLIAGE_TREE_SELECTION[THIS_TREE_NUM]-1;
				float TREE_RADIUS = FOLIAGE_TREE_RADIUS[THIS_TREE_TYPE] * FOLIAGE_TREE_SCALE[THIS_TREE_NUM];
				float DIST = DistanceHorizontal(FOLIAGE_POSITIONS[THIS_TREE_NUM], moveOrg);

				float hDist = 0;

				if (cg.renderingThirdPerson)
					hDist = DistanceHorizontal(FOLIAGE_POSITIONS[THIS_TREE_NUM], cg_entities[cg.clientNum].lerpOrigin);
				else
					hDist = DistanceHorizontal(FOLIAGE_POSITIONS[THIS_TREE_NUM], cg.refdef.vieworg);

				if (DIST <= TREE_RADIUS 
					&& DIST < hDist				// Move pos would be closer
					&& hDist > TREE_RADIUS)		// Not already stuck in tree...
				{
					//trap->Print("CLIENT: Blocked by tree %i. Radius %f. Distance %f. Type %i.\n", tree, TREE_RADIUS, DIST, THIS_TREE_TYPE);
					return qtrue;
				}
			}
		}

		return qfalse;
	}

	void FOLIAGE_AddFoliageEntityToScene ( refEntity_t *ent )
	{
		AddRefEntityToScene(ent);
	}

	void FOLIAGE_AddToScreen( int num, qboolean treeOnly ) {
		refEntity_t		re;
		vec3_t			angles;
		float			dist = Distance(FOLIAGE_POSITIONS[num], cg.refdef.vieworg);
		float			distFadeScale = 1.0;

		memset( &re, 0, sizeof( re ) );

		VectorCopy(FOLIAGE_POSITIONS[num], re.origin);

		re.reType = RT_MODEL;

		if (!treeOnly)
		{// Graw grass...
			if (FOLIAGE_PLANT_SELECTION[num] != 0)
			{// Add plant model as well...
				float PLANT_SCALE = FOLIAGE_PLANT_SCALE[num]*PLANT_SCALE_MULTIPLIER*distFadeScale;

				re.customShader = FOLIAGE_PLANT_SHADERS[FOLIAGE_PLANT_SELECTION[num]-1];

				re.reType = RT_MODEL;

				if (dist < 512)
				{
					re.hModel = FOLIAGE_PLANT_MODEL[0];
				}
				else if (dist < 1024)
				{
					re.hModel = FOLIAGE_PLANT_MODEL[1];
				}
				else
				{
					re.hModel = FOLIAGE_PLANT_MODEL[2];
				}

				VectorSet(re.modelScale, PLANT_SCALE, PLANT_SCALE, PLANT_SCALE);

				vectoangles( FOLIAGE_NORMALS[num], angles );
				angles[PITCH] += 90;

				VectorCopy(angles, re.angles);
				AnglesToAxis(angles, re.axis);
				ScaleModelAxis( &re );

				FOLIAGE_AddFoliageEntityToScene( &re );
			}
			else
			{
				float GRASS_SCALE = FOLIAGE_PLANT_SCALE[num]*PLANT_SCALE_MULTIPLIER*distFadeScale;

				re.reType = RT_MODEL;

				//re.origin[2] -= 16.0;

				if (dist < 128)
				{
					re.customShader = FOLIAGE_GRASS_BILLBOARD_SHADER[0];
					re.hModel = FOLIAGE_PLANT_MODEL[0];
				}
				else if (dist < 256)
				{
					re.customShader = FOLIAGE_GRASS_BILLBOARD_SHADER[1];
					re.hModel = FOLIAGE_PLANT_MODEL[0];
				}
				else if (dist < 384)
				{
					re.customShader = FOLIAGE_GRASS_BILLBOARD_SHADER[2];
					re.hModel = FOLIAGE_PLANT_MODEL[0];
				}
				else if (dist < 512)
				{
					re.customShader = FOLIAGE_GRASS_BILLBOARD_SHADER[2];
					re.hModel = FOLIAGE_PLANT_MODEL[0];
				}
				else if (dist < 1024)
				{
					re.customShader = FOLIAGE_GRASS_BILLBOARD_SHADER[3];
					re.hModel = FOLIAGE_PLANT_MODEL[1];
				}
				else
				{
					re.customShader = FOLIAGE_GRASS_BILLBOARD_SHADER[4];
					re.hModel = FOLIAGE_PLANT_MODEL[2];
				}

				VectorSet(re.modelScale, GRASS_SCALE, GRASS_SCALE, GRASS_SCALE);

				vectoangles( FOLIAGE_NORMALS[num], angles );
				angles[PITCH] += 90;

				VectorCopy(angles, re.angles);
				AnglesToAxis(angles, re.axis);
				ScaleModelAxis( &re );

				FOLIAGE_AddFoliageEntityToScene( &re );
			}
		}

		VectorCopy(FOLIAGE_POSITIONS[num], re.origin);
		re.customShader = 0;
		re.renderfx = 0;

		if (FOLIAGE_TREE_SELECTION[num] > 0)
		{// Add the tree model...
			if (dist > FOLIAGE_AREA_SIZE*4.8)
			{
				re.reType = RT_SPRITE;

				re.radius = FOLIAGE_TREE_SCALE[num]*2.5*FOLIAGE_TREE_BILLBOARD_SIZE[FOLIAGE_TREE_SELECTION[num]-1];

				re.customShader = FOLIAGE_TREE_BILLBOARD_SHADER[FOLIAGE_TREE_SELECTION[num]-1];

				re.shaderRGBA[0] = 255;
				re.shaderRGBA[1] = 255;
				re.shaderRGBA[2] = 255;
				re.shaderRGBA[3] = 255;

				re.origin[2] += re.radius;
				re.origin[2] += FOLIAGE_TREE_ZOFFSET[FOLIAGE_TREE_SELECTION[num]-1];

				angles[PITCH] = angles[ROLL] = 0.0f;
				angles[YAW] = FOLIAGE_TREE_ANGLES[num];

				VectorCopy(angles, re.angles);
				AnglesToAxis(angles, re.axis);

				FOLIAGE_AddFoliageEntityToScene( &re );
			}
			else
			{
				re.reType = RT_MODEL;
				re.hModel = FOLIAGE_TREE_MODEL[FOLIAGE_TREE_SELECTION[num]-1];

				VectorSet(re.modelScale, FOLIAGE_TREE_SCALE[num]*2.5, FOLIAGE_TREE_SCALE[num]*2.5, FOLIAGE_TREE_SCALE[num]*2.5);

				re.origin[2] += FOLIAGE_TREE_ZOFFSET[FOLIAGE_TREE_SELECTION[num]-1];

				angles[PITCH] = angles[ROLL] = 0.0f;
				angles[YAW] = FOLIAGE_TREE_ANGLES[num];
				VectorCopy(angles, re.angles);
				AnglesToAxis(angles, re.axis);

				ScaleModelAxis( &re );

				FOLIAGE_AddFoliageEntityToScene( &re );

				//
				// Create a list of the closest trees for generating solids...
				//

				float	furthestDist = 0.0;
				int		furthestNum = 0;

				for (int tree = 0; tree < FOLIAGE_SOLID_TREES_MAX; tree++)
				{
					if (FOLIAGE_SOLID_TREES_DIST[tree] > furthestDist)
					{
						furthestNum = tree;
						furthestDist = FOLIAGE_SOLID_TREES_DIST[tree];
					}
				}


				float hDist = 0;

				if (cg.renderingThirdPerson)
					hDist = DistanceHorizontal(FOLIAGE_POSITIONS[num], cg_entities[cg.clientNum].lerpOrigin);
				else
					hDist = DistanceHorizontal(FOLIAGE_POSITIONS[num], cg.refdef.vieworg);

				if (hDist < FOLIAGE_SOLID_TREES_DIST[furthestNum])
				{
					//trap->Print("Set solid tree %i at %f %f %f. Dist %f.\n", furthestNum, FOLIAGE_POSITIONS[num][0], FOLIAGE_POSITIONS[num][1], FOLIAGE_POSITIONS[num][2], dist);
					FOLIAGE_SOLID_TREES[furthestNum] = num;
					FOLIAGE_SOLID_TREES_DIST[furthestNum] = hDist;
				}
			}
		}
	}

	qboolean FOLIAGE_LoadFoliagePositions( void )
	{
		fileHandle_t	f;
		int				i = 0;

		trap->FS_Open( va( "foliage/%s.foliage", cgs.currentmapname), &f, FS_READ );

		if ( !f )
		{
			return qfalse;
		}

		trap->FS_Read( &FOLIAGE_NUM_POSITIONS, sizeof(int), f );

		for (i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
		{
			trap->FS_Read( &FOLIAGE_POSITIONS[i], sizeof(vec3_t), f );
			trap->FS_Read( &FOLIAGE_NORMALS[i], sizeof(vec3_t), f );
			trap->FS_Read( &FOLIAGE_PLANT_SELECTION[i], sizeof(int), f );
			trap->FS_Read( &FOLIAGE_PLANT_ANGLES[i], sizeof(float), f );
			trap->FS_Read( &FOLIAGE_PLANT_SCALE[i], sizeof(float), f );
			trap->FS_Read( &FOLIAGE_TREE_SELECTION[i], sizeof(int), f );
			trap->FS_Read( &FOLIAGE_TREE_ANGLES[i], sizeof(float), f );
			trap->FS_Read( &FOLIAGE_TREE_SCALE[i], sizeof(float), f );
		}

		trap->FS_Close(f);

		trap->Print( "^1*** ^3%s^5: Successfully loaded %i foliage points from foliage file ^7foliage/%s.foliage^5.\n", GAME_VERSION,
			FOLIAGE_NUM_POSITIONS, cgs.currentmapname );

		FOLIAGE_Setup_Foliage_Areas();

		return qtrue;
	}

	qboolean FOLIAGE_SaveFoliagePositions( void )
	{
		fileHandle_t	f;
		int				i = 0;

		trap->FS_Open( va( "foliage/%s.foliage", cgs.currentmapname), &f, FS_WRITE );

		if ( !f )
		{
			trap->Print( "^1*** ^3%s^5: Failed to open foliage file ^7foliage/%s.foliage^5 for save.\n", GAME_VERSION, cgs.currentmapname );
			return qfalse;
		}

		for (i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
		{
			trace_t tr;
			vec3_t	up, down;
			VectorCopy(FOLIAGE_POSITIONS[i], up);
			up[2]+=128;
			VectorCopy(FOLIAGE_POSITIONS[i], down);
			down[2]-=128;
			CG_Trace(&tr, up, NULL, NULL, down, -1, MASK_SOLID);
			VectorCopy(tr.plane.normal, FOLIAGE_NORMALS[i]);
		}

		trap->FS_Write( &FOLIAGE_NUM_POSITIONS, sizeof(int), f );

		for (i = 0; i < FOLIAGE_NUM_POSITIONS; i++)
		{
			trap->FS_Write( &FOLIAGE_POSITIONS[i], sizeof(vec3_t), f );
			trap->FS_Write( &FOLIAGE_NORMALS[i], sizeof(vec3_t), f );
			trap->FS_Write( &FOLIAGE_PLANT_SELECTION[i], sizeof(int), f );
			trap->FS_Write( &FOLIAGE_PLANT_ANGLES[i], sizeof(float), f );
			trap->FS_Write( &FOLIAGE_PLANT_SCALE[i], sizeof(float), f );
			trap->FS_Write( &FOLIAGE_TREE_SELECTION[i], sizeof(int), f );
			trap->FS_Write( &FOLIAGE_TREE_ANGLES[i], sizeof(float), f );
			trap->FS_Write( &FOLIAGE_TREE_SCALE[i], sizeof(float), f );
		}

		trap->FS_Close(f);

		FOLIAGE_Setup_Foliage_Areas();

		trap->Print( "^1*** ^3%s^5: Successfully saved %i grass points to foliage file ^7foliage/%s.foliage^5.\n", GAME_VERSION,
			FOLIAGE_NUM_POSITIONS, cgs.currentmapname );

		return qtrue;
	}

	qboolean FOLIAGE_IgnoreFoliageOnMap( void )
	{
		if (StringContainsWord(cgs.currentmapname, "eisley")
			|| StringContainsWord(cgs.currentmapname, "desert")
			|| StringContainsWord(cgs.currentmapname, "tatooine")
			|| StringContainsWord(cgs.currentmapname, "hoth")
			|| StringContainsWord(cgs.currentmapname, "mp/ctf1")
			|| StringContainsWord(cgs.currentmapname, "mp/ctf2")
			|| StringContainsWord(cgs.currentmapname, "mp/ctf4")
			|| StringContainsWord(cgs.currentmapname, "mp/ctf5")
			|| StringContainsWord(cgs.currentmapname, "mp/ffa1")
			|| StringContainsWord(cgs.currentmapname, "mp/ffa2")
			|| StringContainsWord(cgs.currentmapname, "mp/ffa3")
			|| StringContainsWord(cgs.currentmapname, "mp/ffa4")
			|| StringContainsWord(cgs.currentmapname, "mp/ffa5")
			|| StringContainsWord(cgs.currentmapname, "mp/duel1")
			|| StringContainsWord(cgs.currentmapname, "mp/duel2")
			|| StringContainsWord(cgs.currentmapname, "mp/duel3")
			|| StringContainsWord(cgs.currentmapname, "mp/duel4")
			|| StringContainsWord(cgs.currentmapname, "mp/duel5")
			|| StringContainsWord(cgs.currentmapname, "mp/duel7")
			|| StringContainsWord(cgs.currentmapname, "mp/duel9")
			|| StringContainsWord(cgs.currentmapname, "mp/duel10")
			|| StringContainsWord(cgs.currentmapname, "bespin_streets")
			|| StringContainsWord(cgs.currentmapname, "bespin_platform"))
		{// Ignore this map... We know we don't need grass here...
			return qtrue;
		}

		return qfalse;
	}

	void FOLIAGE_DrawGrass( void )
	{
		int spot = 0;

		if (FOLIAGE_IgnoreFoliageOnMap())
		{// Ignore this map... We know we don't need grass here...
			FOLIAGE_NUM_POSITIONS = 0;
			FOLIAGE_LOADED = qtrue;
			return;
		}

		if (!FOLIAGE_LOADED)
		{
			FOLIAGE_LoadFoliagePositions();
			FOLIAGE_LOADED = qtrue;
		}

		if (FOLIAGE_NUM_POSITIONS <= 0)
		{
			return;
		}

		if (!FOLIAGE_PLANT_MODEL[0])
		{// Init/register all foliage models...
			FOLIAGE_PLANT_MODEL[0] = trap->R_RegisterModel( "models/warzone/foliage/grass01_LOD0.md3" );
			FOLIAGE_PLANT_MODEL[1] = trap->R_RegisterModel( "models/warzone/foliage/grass01_LOD1.md3" );
			FOLIAGE_PLANT_MODEL[2] = trap->R_RegisterModel( "models/warzone/foliage/grass01_LOD2.md3" );

#if 1
			FOLIAGE_GRASS_BILLBOARD_SHADER[0] = trap->R_RegisterShader( "models/warzone/foliage/maingrass1024.png" );
			FOLIAGE_GRASS_BILLBOARD_SHADER[1] = trap->R_RegisterShader( "models/warzone/foliage/maingrass512.png" );
			FOLIAGE_GRASS_BILLBOARD_SHADER[2] = trap->R_RegisterShader( "models/warzone/foliage/maingrass256.png" );
			FOLIAGE_GRASS_BILLBOARD_SHADER[3] = trap->R_RegisterShader( "models/warzone/foliage/maingrass128.png" );
			FOLIAGE_GRASS_BILLBOARD_SHADER[4] = trap->R_RegisterShader( "models/warzone/foliage/maingrass64.png" );

			//FOLIAGE_TREE_MODEL[0] = trap->R_RegisterModel( "models/map_objects/yavin/tree08_b.md3" );

			FOLIAGE_TREE_MODEL[0] = trap->R_RegisterModel( "models/warzone/trees/fanpalm1.md3" );
			FOLIAGE_TREE_MODEL[1] = trap->R_RegisterModel( "models/warzone/trees/giant1.md3" );
			FOLIAGE_TREE_MODEL[2] = trap->R_RegisterModel( "models/warzone/trees/anvilpalm1.md3" );

			FOLIAGE_TREE_BILLBOARD_SHADER[0] = trap->R_RegisterShader("models/warzone/trees/fanpalm1");
			FOLIAGE_TREE_BILLBOARD_SHADER[1] = trap->R_RegisterShader("models/warzone/trees/giant1");
			FOLIAGE_TREE_BILLBOARD_SHADER[2] = trap->R_RegisterShader("models/warzone/trees/anvilpalm1");

			FOLIAGE_TREE_BILLBOARD_SIZE[0] = 64.0;
			FOLIAGE_TREE_BILLBOARD_SIZE[1] = 204.0;
			FOLIAGE_TREE_BILLBOARD_SIZE[2] = 112.0;

			FOLIAGE_TREE_RADIUS[0] = 24.0;
			FOLIAGE_TREE_RADIUS[1] = 72.0;
			FOLIAGE_TREE_RADIUS[2] = 52.0;

			FOLIAGE_TREE_ZOFFSET[0] = -64.0;
			FOLIAGE_TREE_ZOFFSET[1] = 0.0;
			FOLIAGE_TREE_ZOFFSET[2] = 0.0;

			for (int i = 0; i < MAX_PLANT_SHADERS; i++)
			{
				FOLIAGE_PLANT_SHADERS[i] = trap->R_RegisterShader(GoodPlantsList[i]);
			}
#else
			FOLIAGE_GRASS_BILLBOARD_SHADER[0] = trap->R_RegisterShader( IniRead(va("foliage/%s.ini", cgs.currentmapname),"GRASS","GRASS_SHADER_LOD_0","models/warzone/foliage/models/warzone/foliage/maingrass1024.png") );

			if (!FOLIAGE_GRASS_BILLBOARD_SHADER[0])
			{// Have no base grass texture... Use defaults...
				FOLIAGE_GRASS_BILLBOARD_SHADER[0] = trap->R_RegisterShader( "models/warzone/foliage/maingrass1024.png" );
				FOLIAGE_GRASS_BILLBOARD_SHADER[1] = trap->R_RegisterShader( "models/warzone/foliage/maingrass512.png" );
				FOLIAGE_GRASS_BILLBOARD_SHADER[2] = trap->R_RegisterShader( "models/warzone/foliage/maingrass256.png" );
				FOLIAGE_GRASS_BILLBOARD_SHADER[3] = trap->R_RegisterShader( "models/warzone/foliage/maingrass128.png" );
				FOLIAGE_GRASS_BILLBOARD_SHADER[4] = trap->R_RegisterShader( "models/warzone/foliage/maingrass64.png" );
			}
			else
			{// Have a grass base shader, check the rest of the LOD levels from ini. If not found, use base...
				FOLIAGE_GRASS_BILLBOARD_SHADER[1] = trap->R_RegisterShader( IniRead(va("foliage/%s.ini", cgs.currentmapname),"GRASS","GRASS_SHADER_LOD_1","models/warzone/foliage/models/warzone/foliage/maingrass512.png") );
				if (!FOLIAGE_GRASS_BILLBOARD_SHADER[1]) FOLIAGE_GRASS_BILLBOARD_SHADER[1] = FOLIAGE_GRASS_BILLBOARD_SHADER[0];

				FOLIAGE_GRASS_BILLBOARD_SHADER[2] = trap->R_RegisterShader( IniRead(va("foliage/%s.ini", cgs.currentmapname),"GRASS","GRASS_SHADER_LOD_2","models/warzone/foliage/models/warzone/foliage/maingrass256.png") );
				if (!FOLIAGE_GRASS_BILLBOARD_SHADER[2]) FOLIAGE_GRASS_BILLBOARD_SHADER[2] = FOLIAGE_GRASS_BILLBOARD_SHADER[0];

				FOLIAGE_GRASS_BILLBOARD_SHADER[3] = trap->R_RegisterShader( IniRead(va("foliage/%s.ini", cgs.currentmapname),"GRASS","GRASS_SHADER_LOD_3","models/warzone/foliage/models/warzone/foliage/maingrass128.png") );
				if (!FOLIAGE_GRASS_BILLBOARD_SHADER[3]) FOLIAGE_GRASS_BILLBOARD_SHADER[3] = FOLIAGE_GRASS_BILLBOARD_SHADER[0];

				FOLIAGE_GRASS_BILLBOARD_SHADER[4] = trap->R_RegisterShader( IniRead(va("foliage/%s.ini", cgs.currentmapname),"GRASS","GRASS_SHADER_LOD_4","models/warzone/foliage/models/warzone/foliage/maingrass64.png") );
				if (!FOLIAGE_GRASS_BILLBOARD_SHADER[4]) FOLIAGE_GRASS_BILLBOARD_SHADER[4] = FOLIAGE_GRASS_BILLBOARD_SHADER[0];
			}

			FOLIAGE_PLANT_SHADERS[0] = trap->R_RegisterShader( IniRead(va("foliage/%s.ini", cgs.currentmapname),"PLANTS","PLANT_SHADER_0","seemtohavenone" ));

			if (!FOLIAGE_PLANT_SHADERS[0])
			{// Have no base plant shader in ini, use default list...
				for (int i = 0; i < MAX_PLANT_SHADERS; i++)
				{
					FOLIAGE_PLANT_SHADERS[i] = trap->R_RegisterShader(GoodPlantsList[i]);
				}

				NUM_PLANT_SHADERS = MAX_PLANT_SHADERS;
			}
			else
			{
				for (int i = 0; i < MAX_PLANT_SHADERS; i++)
				{
					FOLIAGE_PLANT_SHADERS[i] = trap->R_RegisterShader( IniRead(va("foliage/%s.ini", cgs.currentmapname),"PLANTS",va("PLANT_SHADER_%i", i),(char *)GoodPlantsList[i]) );
					
					if (!FOLIAGE_PLANT_SHADERS[i])
					{// Hit the end of the ini's list... Record max and exit...
						NUM_PLANT_SHADERS = i;
						break;
					}
				}

				if (NUM_PLANT_SHADERS < MAX_PLANT_SHADERS)
				{// We need to copy from what they have...
					int j = 0;

					for (int i = NUM_PLANT_SHADERS; i < MAX_PLANT_SHADERS; i++)
					{// Copy from begining of their list, until we hit max number...
						FOLIAGE_PLANT_SHADERS[i] = FOLIAGE_PLANT_SHADERS[j];
						j++;
					}
				}
			}

			FOLIAGE_TREE_MODEL[0] = trap->R_RegisterModel( IniRead(va("foliage/%s.ini", cgs.currentmapname),"TREES","TREE_MODEL_0","models/warzone/trees/fanpalm1.md3" ) );
			FOLIAGE_TREE_MODEL[1] = trap->R_RegisterModel( IniRead(va("foliage/%s.ini", cgs.currentmapname),"TREES","TREE_MODEL_1","models/warzone/trees/giant1.md3" ) );
			FOLIAGE_TREE_MODEL[2] = trap->R_RegisterModel( IniRead(va("foliage/%s.ini", cgs.currentmapname),"TREES","TREE_MODEL_2","models/warzone/trees/anvilpalm1.md3" ) );

			FOLIAGE_TREE_BILLBOARD_SHADER[0] = trap->R_RegisterShader( IniRead(va("foliage/%s.ini", cgs.currentmapname),"TREES","TREE_BILLBOARD_SHADER_0","models/warzone/trees/fanpalm1") );
			FOLIAGE_TREE_BILLBOARD_SHADER[1] = trap->R_RegisterShader( IniRead(va("foliage/%s.ini", cgs.currentmapname),"TREES","TREE_BILLBOARD_SHADER_1","models/warzone/trees/giant1") );
			FOLIAGE_TREE_BILLBOARD_SHADER[2] = trap->R_RegisterShader( IniRead(va("foliage/%s.ini", cgs.currentmapname),"TREES","TREE_BILLBOARD_SHADER_2","models/warzone/trees/anvilpalm1") );

			FOLIAGE_TREE_RADIUS[0] = atof( IniRead(va("foliage/%s.ini", cgs.currentmapname),"TREES","TREE_RADIUS_0","24.0") );
			FOLIAGE_TREE_RADIUS[1] = atof( IniRead(va("foliage/%s.ini", cgs.currentmapname),"TREES","TREE_RADIUS_1","72.0") );
			FOLIAGE_TREE_RADIUS[2] = atof( IniRead(va("foliage/%s.ini", cgs.currentmapname),"TREES","TREE_RADIUS_2","52.0") );

			FOLIAGE_TREE_ZOFFSET[0] = atof( IniRead(va("foliage/%s.ini", cgs.currentmapname),"TREES","TREE_ZOFFSET_0","0.0") );
			FOLIAGE_TREE_ZOFFSET[1] = atof( IniRead(va("foliage/%s.ini", cgs.currentmapname),"TREES","TREE_ZOFFSET_1","0.0") );
			FOLIAGE_TREE_ZOFFSET[2] = atof( IniRead(va("foliage/%s.ini", cgs.currentmapname),"TREES","TREE_ZOFFSET_2","0.0") );
#endif
		}

		FOLIAGE_Check_CVar_Change();

		vec3_t viewOrg, viewAngles;

		VectorCopy(cg.refdef.vieworg, viewOrg);
		VectorCopy(cg.refdef.viewangles, viewAngles);

		FOLIAGE_Calc_In_Range_Areas();

		for (int CURRENT_AREA = 0; CURRENT_AREA < IN_RANGE_AREAS_LIST_COUNT; CURRENT_AREA++)
		{
			int CURRENT_AREA_ID = IN_RANGE_AREAS_LIST[CURRENT_AREA];

			for (int spot = 0; spot < FOLIAGE_AREAS_LIST_COUNT[CURRENT_AREA_ID]; spot++)
			{
				FOLIAGE_AddToScreen( FOLIAGE_AREAS_LIST[CURRENT_AREA_ID][spot], qfalse );
			}
		}

		for (int CURRENT_AREA = 0; CURRENT_AREA < IN_RANGE_TREE_AREAS_LIST_COUNT; CURRENT_AREA++)
		{
			int CURRENT_AREA_ID = IN_RANGE_TREE_AREAS_LIST[CURRENT_AREA];

			for (int spot = 0; spot < FOLIAGE_AREAS_LIST_COUNT[CURRENT_AREA_ID]; spot++)
			{
				FOLIAGE_AddToScreen( FOLIAGE_AREAS_LIST[CURRENT_AREA_ID][spot], qtrue );
			}
		}
	}

	// =======================================================================================================================================
	//
	//                                                             Foliage Generation...
	//
	// =======================================================================================================================================

	extern void AIMod_GetMapBounts ( void );
	extern float RoofHeightAt ( vec3_t org );

	extern float aw_percent_complete;
	extern char task_string1[255];
	extern char task_string2[255];
	extern char task_string3[255];
	extern char last_node_added_string[255];
	extern clock_t	aw_stage_start_time;

	qboolean MaterialIsValidForGrass(int materialType)
	{
		switch( materialType )
		{
		case MATERIAL_SHORTGRASS:		// 5					// manicured lawn
		case MATERIAL_LONGGRASS:		// 6					// long jungle grass
		case MATERIAL_MUD:				// 17					// wet soil
		case MATERIAL_DIRT:				// 7					// hard mud
			return qtrue;
			break;
		default:
			break;
		}

		return qfalse;
	}

	qboolean FOLIAGE_FloorIsGrassAt ( vec3_t org )
	{
		trace_t tr;
		vec3_t org1, org2;
		float pitch = 0;

		VectorCopy(org, org1);
		VectorCopy(org, org2);
		org2[2] -= 20.0f;

		CG_Trace( &tr, org1, NULL, NULL, org2, -1, CONTENTS_SOLID|CONTENTS_TERRAIN );

		if (tr.startsolid || tr.allsolid)
		{
			return qfalse;
		}

		if (tr.fraction == 1.0)
		{
			return qfalse;
		}

		if (Distance(org, tr.endpos) >= 20.0)
		{
			return qfalse;
		}

		if (MaterialIsValidForGrass(int(tr.surfaceFlags & MATERIAL_MASK)))
			return qtrue;

		return qfalse;
	}


#define GRASS_MODEL_WIDTH cg_foliageModelWidth.value
#define GRASS_SLOPE_MAX_DIFF cg_foliageMaxSlopeChange.value
#define GRASS_HEIGHT_MAX_DIFF cg_foliageMaxHeightChange.value
#define GRASS_SLOPE_UP_HEIGHT cg_foliageSlopeCheckHeight.value

	qboolean FOLIAGE_CheckSlopesAround(vec3_t pos, vec3_t down, vec3_t groundpos, vec3_t slope, float scale)
	{
		trace_t		tr;
		vec3_t		pos2, down2;
		float		HEIGHT_SCALE = 1.0;

		if (scale <= 0.3)
			HEIGHT_SCALE = 0.5;
		else if (scale <= 0.5)
			HEIGHT_SCALE = 0.7;
		else if (scale <= 0.7)
			HEIGHT_SCALE = 0.9;

		VectorCopy(pos, pos2);
		VectorCopy(down, down2);
		pos2[0] += (GRASS_MODEL_WIDTH * scale);
		down2[0] += (GRASS_MODEL_WIDTH * scale);

		CG_Trace( &tr, pos2, NULL, NULL, down2, ENTITYNUM_NONE, MASK_PLAYERSOLID|CONTENTS_WATER );

		// Slope too different...
		if (!MaterialIsValidForGrass(int(tr.surfaceFlags & MATERIAL_MASK))) return qfalse;
		if (Distance(slope, tr.plane.normal) > GRASS_SLOPE_MAX_DIFF) return qfalse;
		if (DistanceVertical(groundpos, tr.endpos) > GRASS_HEIGHT_MAX_DIFF * HEIGHT_SCALE /*&& Distance(slope, tr.plane.normal) > 0.0*/) return qfalse;

		VectorCopy(pos, pos2);
		VectorCopy(down, down2);
		pos2[0] -= (GRASS_MODEL_WIDTH * scale);
		down2[0] -= (GRASS_MODEL_WIDTH * scale);

		CG_Trace( &tr, pos2, NULL, NULL, down2, ENTITYNUM_NONE, MASK_PLAYERSOLID|CONTENTS_WATER );

		// Slope too different...
		if (!MaterialIsValidForGrass(int(tr.surfaceFlags & MATERIAL_MASK))) return qfalse;
		if (Distance(slope, tr.plane.normal) > GRASS_SLOPE_MAX_DIFF) return qfalse;
		if (DistanceVertical(groundpos, tr.endpos) > GRASS_HEIGHT_MAX_DIFF * HEIGHT_SCALE /*&& Distance(slope, tr.plane.normal) > 0.0*/) return qfalse;

		VectorCopy(pos, pos2);
		VectorCopy(down, down2);
		pos2[1] += (GRASS_MODEL_WIDTH * scale);
		down2[1] += (GRASS_MODEL_WIDTH * scale);

		CG_Trace( &tr, pos2, NULL, NULL, down2, ENTITYNUM_NONE, MASK_PLAYERSOLID|CONTENTS_WATER );

		// Slope too different...
		if (!MaterialIsValidForGrass(int(tr.surfaceFlags & MATERIAL_MASK))) return qfalse;
		if (Distance(slope, tr.plane.normal) > GRASS_SLOPE_MAX_DIFF) return qfalse;
		if (DistanceVertical(groundpos, tr.endpos) > GRASS_HEIGHT_MAX_DIFF * HEIGHT_SCALE /*&& Distance(slope, tr.plane.normal) > 0.0*/) return qfalse;

		VectorCopy(pos, pos2);
		VectorCopy(down, down2);
		pos2[1] -= (GRASS_MODEL_WIDTH * scale);
		down2[1] -= (GRASS_MODEL_WIDTH * scale);

		CG_Trace( &tr, pos2, NULL, NULL, down2, ENTITYNUM_NONE, MASK_PLAYERSOLID|CONTENTS_WATER );

		// Slope too different...
		if (!MaterialIsValidForGrass(int(tr.surfaceFlags & MATERIAL_MASK))) return qfalse;
		if (Distance(slope, tr.plane.normal) > GRASS_SLOPE_MAX_DIFF) return qfalse;
		if (DistanceVertical(groundpos, tr.endpos) > GRASS_HEIGHT_MAX_DIFF * HEIGHT_SCALE /*&& Distance(slope, tr.plane.normal) > 0.0*/) return qfalse;

		VectorCopy(pos, pos2);
		VectorCopy(down, down2);
		pos2[0] += (GRASS_MODEL_WIDTH * scale);
		pos2[1] += (GRASS_MODEL_WIDTH * scale);
		down2[0] += (GRASS_MODEL_WIDTH * scale);
		down2[1] += (GRASS_MODEL_WIDTH * scale);

		CG_Trace( &tr, pos2, NULL, NULL, down2, ENTITYNUM_NONE, MASK_PLAYERSOLID|CONTENTS_WATER );

		// Slope too different...
		if (!MaterialIsValidForGrass(int(tr.surfaceFlags & MATERIAL_MASK))) return qfalse;
		if (Distance(slope, tr.plane.normal) > GRASS_SLOPE_MAX_DIFF) return qfalse;
		if (DistanceVertical(groundpos, tr.endpos) > GRASS_HEIGHT_MAX_DIFF * HEIGHT_SCALE /*&& Distance(slope, tr.plane.normal) > 0.0*/) return qfalse;

		VectorCopy(pos, pos2);
		VectorCopy(down, down2);
		pos2[0] += (GRASS_MODEL_WIDTH * scale);
		pos2[1] -= (GRASS_MODEL_WIDTH * scale);
		down2[0] += (GRASS_MODEL_WIDTH * scale);
		down2[1] -= (GRASS_MODEL_WIDTH * scale);

		CG_Trace( &tr, pos2, NULL, NULL, down2, ENTITYNUM_NONE, MASK_PLAYERSOLID|CONTENTS_WATER );

		// Slope too different...
		if (!MaterialIsValidForGrass(int(tr.surfaceFlags & MATERIAL_MASK))) return qfalse;
		if (Distance(slope, tr.plane.normal) > GRASS_SLOPE_MAX_DIFF) return qfalse;
		if (DistanceVertical(groundpos, tr.endpos) > GRASS_HEIGHT_MAX_DIFF * HEIGHT_SCALE /*&& Distance(slope, tr.plane.normal) > 0.0*/) return qfalse;

		VectorCopy(pos, pos2);
		VectorCopy(down, down2);
		pos2[0] -= (GRASS_MODEL_WIDTH * scale);
		pos2[1] += (GRASS_MODEL_WIDTH * scale);
		down2[0] -= (GRASS_MODEL_WIDTH * scale);
		down2[1] += (GRASS_MODEL_WIDTH * scale);

		CG_Trace( &tr, pos2, NULL, NULL, down2, ENTITYNUM_NONE, MASK_PLAYERSOLID|CONTENTS_WATER );

		// Slope too different...
		if (!MaterialIsValidForGrass(int(tr.surfaceFlags & MATERIAL_MASK))) return qfalse;
		if (Distance(slope, tr.plane.normal) > GRASS_SLOPE_MAX_DIFF) return qfalse;
		if (DistanceVertical(groundpos, tr.endpos) > GRASS_HEIGHT_MAX_DIFF * HEIGHT_SCALE /*&& Distance(slope, tr.plane.normal) > 0.0*/) return qfalse;

		VectorCopy(pos, pos2);
		VectorCopy(down, down2);
		pos2[0] -= (GRASS_MODEL_WIDTH * scale);
		pos2[1] -= (GRASS_MODEL_WIDTH * scale);
		down2[0] -= (GRASS_MODEL_WIDTH * scale);
		down2[1] -= (GRASS_MODEL_WIDTH * scale);

		CG_Trace( &tr, pos2, NULL, NULL, down2, ENTITYNUM_NONE, MASK_PLAYERSOLID|CONTENTS_WATER );

		// Slope too different...
		if (!MaterialIsValidForGrass(int(tr.surfaceFlags & MATERIAL_MASK))) return qfalse;
		if (Distance(slope, tr.plane.normal) > GRASS_SLOPE_MAX_DIFF) return qfalse;
		if (DistanceVertical(groundpos, tr.endpos) > GRASS_HEIGHT_MAX_DIFF * HEIGHT_SCALE /*&& Distance(slope, tr.plane.normal) > 0.0*/) return qfalse;

		return qtrue;
	}

	void FOLIAGE_GenerateFoliage_Real ( float density, float tree_density, int num_dense_areas, qboolean ADD_MORE )
	{
		int				i;
		vec3_t			vec;

		if (FOLIAGE_IgnoreFoliageOnMap())
		{// Ignore this map... We know we don't need grass here...
			FOLIAGE_NUM_POSITIONS = 0;
			FOLIAGE_LOADED = qtrue;
			return;
		}

		if (!ADD_MORE)
		{
			FOLIAGE_NUM_POSITIONS = 0;

			for (i = 0; i < FOLIAGE_MAX_FOLIAGES; i++)
			{// Make sure our list is empty...
				FOLIAGE_PLANT_SELECTION[i] = 0;
				FOLIAGE_TREE_SELECTION[i] = 0;
			}
		}

		i = 0;


		float		startx = -65530, starty = -65530, startz = -65530;
		int			grassSpotCount = 0;
		vec3_t		*grassSpotList;
		float		*grassSpotScale;
		float		map_size, temp;
		vec3_t		mapMins, mapMaxs;
		int			start_time = trap->Milliseconds();
		int			update_timer = 0;
		clock_t		previous_time = 0;
		float		offsetY = 0.0;

		trap->UpdateScreen();

		AIMod_GetMapBounts();

		if (!cg.mapcoordsValid)
		{
			trap->Print("^4*** ^3AUTO-FOLIAGE^4: ^7Map Coordinates are invalid. Can not use auto-waypointer!\n");
			return;
		}

		grassSpotList = (vec3_t *)malloc((sizeof(vec3_t)+1)*FOLIAGE_MAX_FOLIAGES);
		grassSpotScale = (float *)malloc((sizeof(float)+1)*FOLIAGE_MAX_FOLIAGES);

		memset(grassSpotScale, 0, (sizeof(float)+1)*FOLIAGE_MAX_FOLIAGES);

		VectorCopy(cg.mapcoordsMins, mapMins);
		VectorCopy(cg.mapcoordsMaxs, mapMaxs);

		if (mapMaxs[0] < mapMins[0])
		{
			temp = mapMins[0];
			mapMins[0] = mapMaxs[0];
			mapMaxs[0] = temp;
		}

		if (mapMaxs[1] < mapMins[1])
		{
			temp = mapMins[1];
			mapMins[1] = mapMaxs[1];
			mapMaxs[1] = temp;
		}

		if (mapMaxs[2] < mapMins[2])
		{
			temp = mapMins[2];
			mapMins[2] = mapMaxs[2];
			mapMaxs[2] = temp;
		}

		trap->S_Shutup(qtrue);

		startx = mapMaxs[0];
		starty = mapMaxs[1];
		startz = mapMaxs[2];

		map_size = Distance(mapMins, mapMaxs);

		trap->Print( va( "^4*** ^3AUTO-FOLIAGE^4: ^5Map bounds are ^3%.2f %.2f %.2f ^5to ^3%.2f %.2f %.2f^5.\n", mapMins[0], mapMins[1], mapMins[2], mapMaxs[0], mapMaxs[1], mapMaxs[2]) );
		strcpy( task_string1, va("^5Map bounds are ^3%.2f %.2f %.2f ^7to ^3%.2f %.2f %.2f^5.", mapMins[0], mapMins[1], mapMins[2], mapMaxs[0], mapMaxs[1], mapMaxs[2]) );
		trap->UpdateScreen();

		trap->Print( va( "^4*** ^3AUTO-FOLIAGE^4: ^5Generating foliage points. This could take a while... (Map size ^3%.2f^5)\n", map_size) );
		strcpy( task_string2, va("^5Generating foliage points. This could take a while... (Map size ^3%.2f^5)", map_size) );
		trap->UpdateScreen();

		trap->Print( va( "^4*** ^3AUTO-FOLIAGE^4: ^5Finding foliage points...\n") );
		strcpy( task_string3, va("^5Finding foliage points...") );
		trap->UpdateScreen();

		//
		// Create bulk temporary nodes...
		//

		previous_time = clock();
		aw_stage_start_time = clock();
		aw_percent_complete = 0;

		grassSpotCount = 0;

		// Create the map...
		vec3_t MAP_INFO_SIZE;

		MAP_INFO_SIZE[0] = mapMaxs[0] - mapMins[0];
		MAP_INFO_SIZE[1] = mapMaxs[1] - mapMins[1];
		MAP_INFO_SIZE[2] = mapMaxs[2] - mapMins[2];

		float yoff = density * 0.5;

		//#pragma omp parallel for schedule(dynamic)
		for (int x = (int)mapMins[0]; x <= (int)mapMaxs[0]; x += density)
		{
			if (grassSpotCount >= FOLIAGE_MAX_FOLIAGES)
			{
				continue;
			}

			float current =  MAP_INFO_SIZE[0] - (mapMaxs[0] - (float)x);
			float complete = current / MAP_INFO_SIZE[0];

			aw_percent_complete = (float)(complete * 100.0);

			if (yoff == density * 0.75)
				yoff = density * 1.25;
			else if (yoff == density * 1.25)
				yoff = density;
			else
				yoff = density * 0.75;

			for (float y = mapMins[1]; y <= mapMaxs[1]; y += yoff/*density*/)
			{
				if (grassSpotCount >= FOLIAGE_MAX_FOLIAGES)
				{
					break;
				}

				for (float z = mapMaxs[2]; z >= mapMins[2]; z -= 48.0)
				{
					if (grassSpotCount >= FOLIAGE_MAX_FOLIAGES)
					{
						break;
					}

					if(omp_get_thread_num() == 0)
					{// Draw a nice little progress bar ;)
						if (clock() - previous_time > 500) // update display every 500ms...
						{
							previous_time = clock();
							trap->UpdateScreen();
						}
					}

					trace_t		tr;
					vec3_t		pos, down;
					qboolean	FOUND = qfalse;

					VectorSet(pos, x, y, z);
					pos[2] += 8.0;
					VectorCopy(pos, down);
					down[2] = mapMins[2];

					CG_Trace( &tr, pos, NULL, NULL, down, ENTITYNUM_NONE, MASK_PLAYERSOLID|CONTENTS_WATER );

					if (tr.endpos[2] <= mapMins[2])
					{// Went off map...
						break;
					}

					if ( tr.surfaceFlags & SURF_SKY )
					{// Sky...
						continue;
					}

					if ( tr.surfaceFlags & SURF_NODRAW )
					{// don't generate a drawsurface at all
						continue;
					}

					if ( tr.contents & CONTENTS_WATER )
					{// Anything below here is underwater...
						break;
					}

					if (MaterialIsValidForGrass((tr.surfaceFlags & MATERIAL_MASK)))
					{
						// Look around here for a different slope angle... Cull if found...
						for (float scale = 1.00; scale >= 0.05; scale -= 0.05)
						{
							if (FOLIAGE_CheckSlopesAround(pos, down, tr.endpos, tr.plane.normal, scale))
							{
#pragma omp critical (__ADD_TEMP_NODE__)
								{
									sprintf(last_node_added_string, "^5Adding foliage point ^3%i ^5at ^7%f %f %f^5.", grassSpotCount, tr.endpos[0], tr.endpos[1], tr.endpos[2]+8);

									if (scale == 1.0)
										grassSpotScale[grassSpotCount] = (irand(75, 100) / 100.0);
									else
										grassSpotScale[grassSpotCount] = scale;

									VectorSet(grassSpotList[grassSpotCount], tr.endpos[0], tr.endpos[1], tr.endpos[2]+8);
									grassSpotCount++;
									FOUND = qtrue;
								}
							}

							if (FOUND) break;
						}

						if (FOUND) break;
					}
				}
			}
		}

		if (grassSpotCount >= FOLIAGE_MAX_FOLIAGES)
		{
			trap->Print( "^1*** ^3%s^5: Too many foliage points detected... Try again with a higher density value...\n", GAME_VERSION );
			return;
		}

		trap->S_Shutup(qfalse);

		aw_percent_complete = 0.0f;

		if (!ADD_MORE) 
		{
			FOLIAGE_NUM_POSITIONS = 0;

			for (i = 0; i < FOLIAGE_MAX_FOLIAGES; i++)
			{// Make sure our list is empty...
				FOLIAGE_PLANT_SELECTION[i] = 0;
				FOLIAGE_TREE_SELECTION[i] = 0;
			}
		}

		if (grassSpotCount > 0)
		{// Ok, we have spots, copy and set up foliage types/scales/angles, and save to file...
			vec3_t	MAP_SCALE;
			vec3_t	DENSE_SPOTS[1024];

			MAP_SCALE[0] = mapMaxs[0] - mapMins[0];
			MAP_SCALE[1] = mapMaxs[1] - mapMins[1];
			MAP_SCALE[2] = 0;

			for (int d = 0; d <= num_dense_areas; d++)
			{
				DENSE_SPOTS[d][0] = mapMins[0] + irand(0, MAP_SCALE[0]);
				DENSE_SPOTS[d][1] = mapMins[1] + irand(0, MAP_SCALE[1]);
				DENSE_SPOTS[d][2] = 0;
			}

			for (i = 0; i < grassSpotCount; i++)
			{
				vec[0] = grassSpotList[i][0];
				vec[1] = grassSpotList[i][1];
				vec[2] = grassSpotList[i][2];

				FOLIAGE_PLANT_SELECTION[FOLIAGE_NUM_POSITIONS] = 0;
				FOLIAGE_PLANT_ANGLES[FOLIAGE_NUM_POSITIONS] = 0.0f;
				FOLIAGE_PLANT_SCALE[FOLIAGE_NUM_POSITIONS] = grassSpotScale[i];

				FOLIAGE_TREE_SELECTION[FOLIAGE_NUM_POSITIONS] = 0;
				FOLIAGE_TREE_ANGLES[FOLIAGE_NUM_POSITIONS] = 0.0f;
				FOLIAGE_TREE_SCALE[FOLIAGE_NUM_POSITIONS] = 0.0f;

				VectorCopy(vec, FOLIAGE_POSITIONS[FOLIAGE_NUM_POSITIONS]);
				FOLIAGE_POSITIONS[FOLIAGE_NUM_POSITIONS][2] -= 18.0;

				FOLIAGE_PLANT_ANGLES[FOLIAGE_NUM_POSITIONS] = (int)(random() * 180);
				
				if (FOLIAGE_PLANT_SCALE[FOLIAGE_NUM_POSITIONS] == 0)
					FOLIAGE_PLANT_SCALE[FOLIAGE_NUM_POSITIONS] = (float)((float)irand(35,125) / 100.0);

				qboolean IS_DENSE = qfalse;

				for (int d = 0; d <= num_dense_areas; d++)
				{
					if (DistanceHorizontal(DENSE_SPOTS[d], FOLIAGE_POSITIONS[FOLIAGE_NUM_POSITIONS]) <= 1024)
					{
						IS_DENSE = qtrue;
						break;
					}
				}

				float USE_TREE_DENSITY = tree_density;

				if (density >= 48)
				{
					if (IS_DENSE) USE_TREE_DENSITY = tree_density / 4;

					if (irand(0, USE_TREE_DENSITY) >= USE_TREE_DENSITY && RoofHeightAt(vec) - vec[2] > 1024.0)
					{// Add tree...
						FOLIAGE_TREE_SELECTION[FOLIAGE_NUM_POSITIONS] = irand(0, 3);
						FOLIAGE_TREE_ANGLES[FOLIAGE_NUM_POSITIONS] = (int)(random() * 180);
						FOLIAGE_TREE_SCALE[FOLIAGE_NUM_POSITIONS] = (float)((float)irand(65,150) / 100.0);
					}
					else if (irand(0, 1) == 1)
					{// Add plant... 1 in every 2 positions...
						FOLIAGE_PLANT_SELECTION[FOLIAGE_NUM_POSITIONS] = irand(1,MAX_PLANT_SHADERS-1);
					}
				}
				else
				{
					if (IS_DENSE) USE_TREE_DENSITY = tree_density / 4;

					if (irand(0, USE_TREE_DENSITY) >= USE_TREE_DENSITY && RoofHeightAt(vec) - vec[2] > 1024.0)
					{// Add tree... 
						FOLIAGE_TREE_SELECTION[FOLIAGE_NUM_POSITIONS] = irand(0, 3);
						FOLIAGE_TREE_ANGLES[FOLIAGE_NUM_POSITIONS] = (int)(random() * 180);
						FOLIAGE_TREE_SCALE[FOLIAGE_NUM_POSITIONS] = (float)((float)irand(65,150) / 100.0);
					}
					else if (irand(0, 3) >= 1)
					{// Add plant... 1 in every 1.33 positions...
						FOLIAGE_PLANT_SELECTION[FOLIAGE_NUM_POSITIONS] = irand(1,MAX_PLANT_SHADERS-1);
					}
				}

				trace_t tr;
				vec3_t	up, down;
				VectorCopy(FOLIAGE_POSITIONS[FOLIAGE_NUM_POSITIONS], up);
				up[2]+=128;
				VectorCopy(FOLIAGE_POSITIONS[FOLIAGE_NUM_POSITIONS], down);
				down[2]-=128;
				CG_Trace(&tr, up, NULL, NULL, down, -1, MASK_SOLID);
				VectorCopy(tr.plane.normal, FOLIAGE_NORMALS[FOLIAGE_NUM_POSITIONS]);

				FOLIAGE_NUM_POSITIONS++;
			}

			trap->Print( "^1*** ^3%s^5: Successfully generated %i grass points...\n", GAME_VERSION, FOLIAGE_NUM_POSITIONS );

			// Save the generated info to a file for next time...
			FOLIAGE_SaveFoliagePositions();
		}
		else
		{
			trap->Print( "^1*** ^3%s^5: Did not find any grass points on this map...\n", GAME_VERSION );
		}

		free(grassSpotList);

		FOLIAGE_LOADED = qtrue;
	}

	extern qboolean CPU_CHECKED;
	extern int UQ_Get_CPU_Info( void );

	void FOLIAGE_GenerateFoliage ( void )
	{
		char	str[MAX_TOKEN_CHARS];

		// UQ1: Check if we have an SSE CPU.. It can speed up our memory allocation by a lot!
		if (!CPU_CHECKED)
			UQ_Get_CPU_Info();

		if ( trap->Cmd_Argc() < 2 )
		{
			trap->Print( "^4*** ^3AUTO-FOLIAGE^4: ^7Usage:\n" );
			trap->Print( "^4*** ^3AUTO-FOLIAGE^4: ^3/genfoliage <method> <density> <tree_density> <num_tree_dense_areas>^5. Density, Tree Density, and Num Tree Dense Areas are optional.\n" );
			trap->Print( "^4*** ^3AUTO-FOLIAGE^4: ^5Available methods are:\n" );
			trap->Print( "^4*** ^3AUTO-FOLIAGE^4: ^3\"standard\" ^5- Standard method. Allows you to set a density.\n");
			trap->Print( "^4*** ^3AUTO-FOLIAGE^4: ^3\"add\" ^5- Standard method. Allows you to set a density. This one adds to current list of foliages.\n");
			trap->UpdateScreen();
			return;
		}

		trap->Cmd_Argv( 1, str, sizeof(str) );

		if ( Q_stricmp( str, "standard") == 0 )
		{
			if ( trap->Cmd_Argc() >= 2 )
			{// Override normal density...
				int dist = 32;
				int tree_dist = 48;
				int num_dense_areas = 0;

				trap->Cmd_Argv( 2, str, sizeof(str) );
				dist = atoi(str);

				if (dist <= 4)
				{// Fallback and warning...
					dist = 32;
					trap->Print( "^4*** ^3AUTO-FOLIAGE^4: ^7Warning: ^5Invalid density set (%i). Using default (%i)...\n", atoi(str), 32 );
				}

				trap->Cmd_Argv( 3, str, sizeof(str) );
				tree_dist = atoi(str);

				if (tree_dist <= 8)
				{// Fallback and warning...
					tree_dist = 64;
					trap->Print( "^4*** ^3AUTO-FOLIAGE^4: ^7Warning: ^5Invalid tree density set (%i). Using default (%i)...\n", atoi(str), 64 );
				}

				trap->Cmd_Argv( 4, str, sizeof(str) );
				num_dense_areas = atoi(str);

				FOLIAGE_GenerateFoliage_Real((float)dist, (float)tree_dist, num_dense_areas, qfalse);
			}
			else
			{
				FOLIAGE_GenerateFoliage_Real(32.0, 48.0, 0, qfalse);
			}
		}
		else if ( Q_stricmp( str, "add") == 0 )
		{
			if ( trap->Cmd_Argc() >= 2 )
			{// Override normal density...
				int dist = 32;
				int tree_dist = 64;
				int num_dense_areas = 0;

				trap->Cmd_Argv( 2, str, sizeof(str) );
				dist = atoi(str);

				if (dist <= 4)
				{// Fallback and warning...
					dist = 32;
					trap->Print( "^4*** ^3AUTO-FOLIAGE^4: ^7Warning: ^5Invalid density set (%i). Using default (%i)...\n", atoi(str), 32 );
				}

				trap->Cmd_Argv( 3, str, sizeof(str) );
				tree_dist = atoi(str);

				if (tree_dist <= 8)
				{// Fallback and warning...
					tree_dist = 64;
					trap->Print( "^4*** ^3AUTO-FOLIAGE^4: ^7Warning: ^5Invalid tree density set (%i). Using default (%i)...\n", atoi(str), 64 );
				}

				trap->Cmd_Argv( 4, str, sizeof(str) );
				num_dense_areas = atoi(str);

				FOLIAGE_GenerateFoliage_Real((float)dist, (float)tree_dist, num_dense_areas, qtrue);
			}
			else
			{
				FOLIAGE_GenerateFoliage_Real(32.0, 48.0, 0, qtrue);
			}
		}
	}
}
