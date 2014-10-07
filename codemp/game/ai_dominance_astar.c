// =======================================================================================================================================
//
//
//
//                       UNIQUEONE'S ALL NEW ULTRA UBER AWESOME ULTIMATE SUPER MAGICAL ASTAR PATHFINDER
//
//                        (actually not as fast as the old one, but provides much, much better routes)
//
//                         --------------------------------------------------------------------------
//
//                                                      TODO LIST:
//									   Multithread this baby to another/other core(s).
//                                 Work out some way to optimize? - My head hurts already!
//                    Waypoint Super Highways? - Save the long waypoint lists for fast access by other AI?
//           Buy a new mouse! - This one's buttons unclick at the wrong moments all the time - it's driving me insane!
//
//
// =======================================================================================================================================

// Disable stupid warnings...
#pragma warning( disable : 4710 )

#include "ai_main.h"

#ifdef __DOMINANCE_AI__

#define MAX_NODELINKS       32
#define NODE_INVALID -1

extern int FRAME_TIME;
extern int gWPNum;
extern wpobject_t *gWPArray[MAX_WPARRAY_SIZE];

extern float HeightDistance(vec3_t v1, vec3_t v2);
extern int DOM_FindIdealPathtoWP(bot_state_t *bs, int from, int to, int badwp2, int *pathlist);

qboolean PATHING_IGNORE_FRAME_TIME = qfalse;

int			*openlist;												//add 1 because it's a binary heap, and they don't use 0 - 1 is the first used index
float		*gcost;
int			*fcost;
char		*list;														//0 is neither, 1 is open, 2 is closed - char because it's the smallest data type
int			*parent;

qboolean PATHFINDING_MEMORY_ALLOCATED = qfalse;

void AllocatePathFindingMemory()
{
	if (PATHFINDING_MEMORY_ALLOCATED) return;

	openlist = (int *)G_Alloc(sizeof(int)*(MAX_WPARRAY_SIZE));
	gcost = (float *)G_Alloc(sizeof(float)*(MAX_WPARRAY_SIZE));
	fcost = (int *)G_Alloc(sizeof(int)*(MAX_WPARRAY_SIZE));
	list = (char *)G_Alloc(sizeof(char)*(MAX_WPARRAY_SIZE));
	parent = (int *)G_Alloc(sizeof(int)*(MAX_WPARRAY_SIZE));

	PATHFINDING_MEMORY_ALLOCATED = qtrue;
}

qboolean ASTAR_COSTS_DONE = qfalse;

void ASTAR_InitWaypointCosts ( void )
{
	int i, j;

	if (ASTAR_COSTS_DONE || gWPNum <= 0) return;

#pragma omp parallel //num_threads(8)
	{
#pragma omp parallel for
		// Init the waypoint link costs...
		for (i = 0; i < gWPNum; i++)
		{
#pragma omp parallel for
			for (j = 0; j < gWPArray[i]->neighbornum; j++)
			{
				float ht = 0, hd = 0;

				gWPArray[i]->neighbors[j].cost = Distance(gWPArray[i]->origin, gWPArray[gWPArray[i]->neighbors[j].num]->origin);

				// UQ1: Prefer flat...
				ht = gWPArray[i]->origin[2] - gWPArray[gWPArray[i]->neighbors[j].num]->origin[2];
				
				if (ht < 0) ht *= -1.0f;

				ht += 1.0;

				gWPArray[i]->neighbors[j].cost *= ht;
			}
		}
	}

	ASTAR_COSTS_DONE = qtrue;
}

int ASTAR_GetFCost(gentity_t *bot, int to, int num, int parentNum, float *gcost)
{
	float	gc = 0;
	float	hc = 0;
	float	height_diff = 0;

	if (gcost[num] == -1)
	{
		if (parentNum != -1)
		{
			gc = gcost[parentNum];
			gc += Distance(gWPArray[num]->origin, gWPArray[parentNum]->origin);
		}

		gcost[num] = gc;
	}
	else
	{
		gc = gcost[num];
	}

	hc = Distance(gWPArray[to]->origin, gWPArray[num]->origin);

	return (int)((gc*0.1) + (hc*0.1));
	//return (int)(gc + hc);
}

int ASTAR_FindPathFast(int from, int to, int *pathlist, qboolean shorten)
{
	//all the data we have to hold...since we can't do dynamic allocation, has to be MAX_WPARRAY_SIZE
	//we can probably lower this later - eg, the open list should never have more than at most a few dozen items on it
	int			badwp = -1;
	int			numOpen = 0;
	int			atNode, temp, newnode = -1;
	qboolean	found = qfalse;
	int			count = -1;
	float		gc;
	int			i, j, u, v, m;
	gentity_t	*bot = NULL;
	//int			debug_max_threads = 0;

	if (!PATHING_IGNORE_FRAME_TIME && trap->Milliseconds() - FRAME_TIME > 300)
	{// Never path on an already long frame time...
		return -1;
	}

	if ((from == NODE_INVALID) || (to == NODE_INVALID) || (from >= gWPNum) || (to >= gWPNum) || (from == to))
	{
		//trap->Print("Bad from or to node.\n");
		return (-1);
	}

	// Check if memory needs to be allocated...
	AllocatePathFindingMemory();

	// Init waypoint link costs if needed...
	ASTAR_InitWaypointCosts();

	memset(openlist, 0, (sizeof(int)* (gWPNum + 1)));
	memset(gcost, 0, (sizeof(float)* gWPNum));
	memset(fcost, 0, (sizeof(int)* gWPNum));
	memset(list, 0, (sizeof(char)* gWPNum));
	memset(parent, 0, (sizeof(int)* gWPNum));

#pragma omp parallel //num_threads(8)
	{
#pragma omp parallel for
		for (i = 0; i < gWPNum; i++)
		{
			gcost[i] = Distance(gWPArray[i]->origin, gWPArray[to]->origin);
		}
	}

	openlist[gWPNum + 1] = 0;

	openlist[1] = from;																	//add the starting node to the open list
	numOpen++;
	gcost[from] = 0;																	//its f and g costs are obviously 0
	fcost[from] = 0;

	while (1)
	{
		if (numOpen != 0)																//if there are still items in the open list
		{
			//pop the top item off of the list
			atNode = openlist[1];
			list[atNode] = 2;															//put the node on the closed list so we don't check it again
			numOpen--;
			openlist[1] = openlist[numOpen + 1];										//move the last item in the list to the top position
			v = 1;

			//this while loop reorders the list so that the new lowest fcost is at the top again
			while (1)
			{
				u = v;
				if ((2 * u + 1) < numOpen)											//if both children exist
				{
					if (fcost[openlist[u]] >= fcost[openlist[2 * u]])
					{
						v = 2 * u;
					}

					if (fcost[openlist[v]] >= fcost[openlist[2 * u + 1]])
					{
						v = 2 * u + 1;
					}
				}
				else
				{
					if ((2 * u) < numOpen)											//if only one child exists
					{
						if (fcost[openlist[u]] >= fcost[openlist[2 * u]])
						{
							v = 2 * u;
						}
					}
				}

				if (u != v)															//if they're out of order, swap this item with its parent
				{
					temp = openlist[u];
					openlist[u] = openlist[v];
					openlist[v] = temp;
				}
				else
				{
					break;
				}
			}

			for (i = 0; i < gWPArray[atNode]->neighbornum && i < MAX_NODELINKS; i++)								//loop through all the links for this node
			{
				newnode = gWPArray[atNode]->neighbors[i].num;

				if (newnode >= gWPNum)
					continue;

				if (newnode < 0)
					continue;

				if (list[newnode] == 2)
				{																		//if this node is on the closed list, skip it
					continue;
				}

				if (list[newnode] != 1)												//if this node is not already on the open list
				{
					openlist[++numOpen] = newnode;										//add the new node to the open list
					list[newnode] = 1;
					parent[newnode] = atNode;											//record the node's parent

					if (newnode == to)
					{																	//if we've found the goal, don't keep computing paths!
						break;															//this will break the 'for' and go all the way to 'if (list[to] == 1)'
					}

					fcost[newnode] = ASTAR_GetFCost(bot, to, newnode, parent[newnode], gcost);	//store it's f cost value
					
					if (fcost[newnode] <= 0 && newnode != from)
					{
						trap->Print("ASTAR WARNING: Missing fcost for node %i. This should not happen!\n", newnode);
					}

					//this loop re-orders the heap so that the lowest fcost is at the top
					m = numOpen;

					while (m != 1)													//while this item isn't at the top of the heap already
					{
						if (fcost[openlist[m]] <= fcost[openlist[m / 2]])				//if it has a lower fcost than its parent
						{
							temp = openlist[m / 2];
							openlist[m / 2] = openlist[m];
							openlist[m] = temp;											//swap them
							m /= 2;
						}
						else
						{
							break;
						}
					}
				}
				else										//if this node is already on the open list
				{
					gc = gcost[atNode];

					if (gWPArray[atNode]->neighbors[i].cost > 0)// && gWPArray[atNode]->neighbors[i].cost < 32768/*9999*/)
					{// UQ1: Already have a cost value, skip the calculations!
						gc += gWPArray[atNode]->neighbors[i].cost;
					}
					else
					{
						vec3_t	vec;

						VectorSubtract(gWPArray[newnode]->origin, gWPArray[atNode]->origin, vec);
						gc += VectorLength(vec);				//calculate what the gcost would be if we reached this node along the current path
						gWPArray[atNode]->neighbors[i].cost = VectorLength(vec);
						trap->Print("ASTAR WARNING: Missing cost for node %i neighbour %i. This should not happen!\n", atNode, i);
					}

					if (gc < gcost[newnode])				//if the new gcost is less (ie, this path is shorter than what we had before)
					{
						parent[newnode] = atNode;			//set the new parent for this node
						gcost[newnode] = gc;				//and the new g cost

						for (j = 1; j < numOpen; j++)		//loop through all the items on the open list
						{
							if (openlist[j] == newnode)	//find this node in the list
							{
								//calculate the new fcost and store it
								fcost[newnode] = ASTAR_GetFCost(bot, to, newnode, parent[newnode], gcost);

								//reorder the list again, with the lowest fcost item on top
								m = j;

								while (m != 1)
								{
									if (fcost[openlist[m]] < fcost[openlist[m / 2]])	//if the item has a lower fcost than it's parent
									{
										temp = openlist[m / 2];
										openlist[m / 2] = openlist[m];
										openlist[m] = temp;								//swap them
										m /= 2;
									}
									else
									{
										break;
									}
								}
								break;													//exit the 'for' loop because we already changed this node
							}															//if
						}																//for
					}											//if (gc < gcost[newnode])
				}												//if (list[newnode] != 1) --> else
			}													//for (loop through links)
		}														//if (numOpen != 0)
		else
		{
			found = qfalse;										//there is no path between these nodes
			break;
		}

		if (list[to] == 1)									//if the destination node is on the open list, we're done
		{
			found = qtrue;
			break;
		}
	}															//while (1)

	if (found == qtrue)							//if we found a path, and are trying to store the pathlist...
	{
		count = 0;
		temp = to;												//start at the end point

		while (temp != from)									//travel along the path (backwards) until we reach the starting point
		{
			if (count + 1 >= MAX_WPARRAY_SIZE)
			{
				trap->Print("ASTAR WARNING: pathlist count > MAX_WPARRAY_SIZE.\n");
				return -1; // UQ1: Added to stop crash if path is too long for the memory allocation...
			}

			pathlist[count++] = temp;							//add the node to the pathlist and increment the count
			temp = parent[temp];								//move to the parent of this node to continue the path
		}

		pathlist[count++] = from;								//add the beginning node to the end of the pathlist

		// Debug output the pathlist...
		/*trap->Print("Pathsize is [%i]. Path [", count);
		for (i = 0; i < count; i++)
		{
			trap->Print(" %i", pathlist[i]);
		}
		trap->Print(" ]\n");*/

		return (count);
	}

	//trap->Print("Failed to find path.\n");
	return (-1);											//return the number of nodes in the path, -1 if not found
}

extern int BG_GetTime();
extern int DOM_GetBestWaypoint(vec3_t org, int ignore, int badwp);

void AIMod_TimeMapPaths()
{
	int			startTime = trap->Milliseconds();
	/*short*/ int	pathlist[MAX_WPARRAY_SIZE];
	int			pathsize;
	gentity_t	*ent = NULL;
	int			i, j;
	int			current_wp, longTermGoal;
	int			NUM_PATHS = 0;
	int			PATH_DISTANCES[MAX_GENTITIES];
	int			TOTAL_DISTANCE = 0;
	int			AVERAGE_DISTANCE = 0;

	ent = G_Find(ent, FOFS(classname), "info_player_deathmatch");

	if (!ent)
		trap->Print("No spawnpoint found!\n");

	current_wp = DOM_GetBestWaypoint(ent->r.currentOrigin, -1, -1);

	if (!current_wp)
		trap->Print("No waypoint found!\n");

	trap->Print("Finding bot objectives at node number %i (%f %f %f).\n",
		current_wp, gWPArray[current_wp]->origin[0], gWPArray[current_wp]->origin[1],
		gWPArray[current_wp]->origin[2]);

	PATHING_IGNORE_FRAME_TIME = qtrue;

	for (i = 0; i < MAX_GENTITIES; i++)
	{
		gentity_t	*goal = &g_entities[i];

		if (!goal || !goal->inuse) continue;

		if (!goal->classname
			|| !goal->classname[0]
			|| !Q_stricmp(goal->classname, "freed")
			|| !Q_stricmp(goal->classname, "noclass"))
			continue;

		if (i == ent->s.number) continue;

		longTermGoal = DOM_GetBestWaypoint(goal->s.origin, -1, -1);

		//pathsize = ASTAR_FindPath(current_wp, longTermGoal, pathlist);
		//pathsize = ASTAR_FindPathWithTimeLimit(current_wp, longTermGoal, pathlist);
		//pathsize = ASTAR_FindPathFast(current_wp, longTermGoal, pathlist, qtrue);
		pathsize = ASTAR_FindPathFast(current_wp, longTermGoal, pathlist, qfalse);
		//pathsize = DOM_FindIdealPathtoWP(NULL, current_wp, longTermGoal, -1, pathlist);

		if (pathsize > 0)
		{
			int j;

			PATH_DISTANCES[NUM_PATHS] = 0;

			for (j = 0; j < pathsize - 1; j++)
			{
				PATH_DISTANCES[NUM_PATHS] += Distance(gWPArray[pathlist[j]]->origin, gWPArray[pathlist[j + 1]]->origin);
			}

			NUM_PATHS++;

			trap->Print("Objective %i (%s) pathsize is %i.\n", i, goal->classname, pathsize);
		}
	}

	for (j = 0; j < NUM_PATHS; j++)
	{
		TOTAL_DISTANCE += PATH_DISTANCES[j];
	}

	AVERAGE_DISTANCE = TOTAL_DISTANCE / NUM_PATHS;

	trap->Print("Completed %i paths in %i seconds. Average path distance is %i\n", NUM_PATHS, (int)((int)(trap->Milliseconds() - startTime) / 1000), AVERAGE_DISTANCE);

	PATHING_IGNORE_FRAME_TIME = qfalse;
}

#endif //__DOMINANCE_AI__
