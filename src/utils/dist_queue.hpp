#pragma once

#include "../include.h"
#include "entry.hpp"

#define QUEUE_ITEM_COUNT 500

// used when figuring out cam paths' distance to spawn
class DIST_GRAPH_Q
{
public:
	DIST_GRAPH_Q()
	{
		add_index = 0;
		pop_index = 0;
		for (int32_t i = 0; i < QUEUE_ITEM_COUNT; i++)
		{
			zone_indices[i] = -1;
			camera_indices[i] = -1;
		}
	}

	int32_t add_index;
	int32_t pop_index;
	int32_t zone_indices[QUEUE_ITEM_COUNT];
	int32_t camera_indices[QUEUE_ITEM_COUNT];

	// graph add new
	inline void graph_add(ELIST& elist, int32_t zone_index, int32_t camera_index)
	{
		int32_t n = add_index;
		zone_indices[n] = zone_index;
		camera_indices[n] = camera_index;
		add_index++;

		elist[zone_index].distances[camera_index] = n;
		elist[zone_index].visited[camera_index] = true;
	}

	// graph pop out
	inline void graph_pop(int32_t* zone_index, int32_t* cam_index)
	{
		int32_t n = pop_index;
		*zone_index = zone_indices[n];
		*cam_index = camera_indices[n];
		pop_index++;
	}

	// For determining whether a path is before or after another path relative to current spawn.
	// For each entry zone's cam path it calculates its distance from the selected spawn (in cam paths) according
	// to level's path links.
	static void run_distance_graph(ELIST& elist, SPAWN& spawn0)
	{

		for (auto& ntry : elist)
		{
			if (ntry.entry_type() != ENTRY_TYPE_ZONE)
				continue;

			int32_t cam_count = ntry.cam_item_count() / 3;
			ntry.distances = (uint32_t*)try_malloc(cam_count * sizeof(uint32_t)); // freed by build_main
			ntry.visited = (bool*)try_malloc(cam_count * sizeof(bool));           // freed by build_main

			for (int32_t j = 0; j < cam_count; j++)
			{
				ntry.distances[j] = INT_MAX;
				ntry.visited[j] = false;
			}
		}

		DIST_GRAPH_Q graph{};
		int32_t start_index = elist.get_index(spawn0.zone);
		graph.graph_add(elist, start_index, 0);

		while (1)
		{
			int32_t top_zone;
			int32_t top_cam;
			graph.graph_pop(&top_zone, &top_cam);
			if (top_zone == -1 && top_cam == -1)
				break;

			LIST links = elist[top_zone].get_links(2 + 3 * top_cam);
			for (int32_t i = 0; i < links.count(); i++)
			{
				CAMERA_LINK camlink(links[i]);								
				LIST neighbours = elist[top_zone].get_neighbours();
				
				int32_t neigh_idx = elist.get_index(neighbours[camlink.zone_index]);
				if (neigh_idx == -1)
				{
					printf("[warning] %s references %s that does not seem to be present (link %d neighbour %d)\n",
						elist[top_zone].ename, eid2str(neighbours[camlink.zone_index]), i, camlink.zone_index);
					continue;
				}
				int32_t path_count = elist[neigh_idx].cam_item_count() / 3;
				if (camlink.cam_index >= path_count)
				{
					printf("[warning] %s links to %s's cam path %d which doesnt exist (link %d neighbour %d)\n",
						elist[top_zone].ename, eid2str(neighbours[camlink.zone_index]), camlink.cam_index, i, camlink.zone_index);
					continue;
				}

				if (elist[neigh_idx].visited[camlink.cam_index] == 0)
					graph.graph_add(elist, neigh_idx, camlink.cam_index);
			}
		}
	}


};
