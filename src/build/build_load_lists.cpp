#include "../include.h"
#include "../utils/utils.hpp"
#include "../utils/entry.hpp"
#include "../utils/elist.hpp"

// todo refactor and move

// generating load lists during build

/*
Handles most load list creating, except for permaloaded and sound entries.
First part is used for neighbours & scenery & slsts.
Second part is used for draw lists.
*/
void build_load_list_util(ENTRY& ntry, int32_t camera_index, FULL_LABELED_LL& full_list, int32_t cam_length, ELIST& elist)
{
	// neighbours, slsts, scenery
	LIST links = ntry.get_links(camera_index);
	for (auto& link : links)
	{
		build_load_list_zone_refs(ntry, camera_index, link, full_list, cam_length, elist);
	}

	// draw lists
	for (int32_t i = 0; i < cam_length; i++)
	{
		LIST neighbour_list{};
		// get a list of entities drawn within set distance of current camera point
		LIST entity_list = ntry.get_entities_to_load(elist, neighbour_list, camera_index, i);

		// get a list of types and subtypes from the entity list
		// copy in dependency list for each found type/subtype
		LIST types_subtypes = elist.get_types_subtypes_from_ids(entity_list, neighbour_list);
		for (auto& info : types_subtypes)
		{
			int32_t type = info >> 16;
			int32_t subtype = info & 0xFF;

			for (auto& info : elist.m_subt_deps)
			{
				if (info.subtype != subtype || info.type != type)
					continue;

				full_list[i].copy_in(info.dependencies, "dep_" + std::to_string(type) + "_" + std::to_string(subtype));
			}
		}
	}
}



// Deals with slst and neighbour/scenery references of path linked entries.
void build_load_list_zone_refs(ENTRY& ntry, int32_t cam_index, int32_t link_int, FULL_LABELED_LL& full_list, int32_t cam_length, ELIST& elist)
{
	int32_t slst_distance = elist.m_config[LL_SLST_Distance];
	int32_t neig_distance = elist.m_config[LL_Neighbour_Distance];
	int32_t preloading_flag = elist.m_config[LL_Transition_Preloading_Type];

	double backwards_penalty = config_to_double(elist.m_config[LL_Backwards_Loading_Penalty_DBL]);

	int32_t distance = 0;

	CAMERA_LINK link(link_int);

	uint32_t neighbour_eid = from_u32(ntry.get_nth_item(0) + C2_NEIGHBOURS_START + 4 + 4 * link.zone_index);
	int32_t neigh_idx = elist.get_index(neighbour_eid);
	if (neigh_idx == -1)
		return;

	// if preloading nothing
	uint32_t neighbour_flg = from_u32(ntry.get_nth_item(0) + C2_NEIGHBOURS_START + 4 + 4 * link.zone_index + 0x20);
	if (preloading_flag == PRELOADING_NOTHING && (neighbour_flg == 0xF || neighbour_flg == 0x1F))
		return;

	if (preloading_flag == PRELOADING_TEXTURES_ONLY || preloading_flag == PRELOADING_ALL)
	{
		for (auto& scenery : elist[neigh_idx].get_sceneries())
		{
			int32_t scen_index = elist.get_index(scenery);
			if (scen_index == -1)
				continue;

			if (link.type == 1)
			{
				int32_t end_index = (cam_length - 1) / 2 - 1;
				for (int32_t j = 0; j < end_index; j++)
					full_list[j].copy_in(elist[scen_index].get_scenery_textures(), std::string("scen_tex_neigh_") + elist[scen_index].m_ename);
			}
			else if (link.type == 2)
			{
				int32_t start_index = (cam_length - 1) / 2 + 1;
				for (int32_t j = start_index; j < cam_length; j++)
					full_list[j].copy_in(elist[scen_index].get_scenery_textures(), std::string("scen_tex_neigh_") + elist[scen_index].m_ename);
			}
			else
			{
				printf("[warning] Link type %d ??\n", link.type);
			}
		}
	}

	// if preloading only textures
	if (preloading_flag == PRELOADING_TEXTURES_ONLY && (neighbour_flg == 0xF || neighbour_flg == 0x1F))
		return;

	for (int32_t j = 0; j < cam_length; j++)
		full_list[j].copy_in(elist[neigh_idx].m_related, "neigh_dir_rel");

	int32_t neigh_cam_count = elist[neigh_idx].get_cam_item_count() / 3;
	if (link.cam_index >= neigh_cam_count)
	{
		printf("[warning] Zone %s is linked to zone %s's %d. camera path (indexing from 0) when it only has %d paths\n",
			ntry.m_ename, elist[neigh_idx].m_ename, link.cam_index, neigh_cam_count);
		return;
	}

	uint32_t slst = elist[neigh_idx].get_nth_slst(link.cam_index);
	for (int32_t i = 0; i < cam_length; i++)
		full_list[i].add(slst, "neigh_slst");

	elist.add_neighbour_coll_dependencies(full_list, ntry);
	elist.add_neighbour_coll_dependencies(full_list, elist[neigh_idx]);

	ENTITY_PATH coords = elist[neigh_idx].get_ent_path(2 + 3 * link.cam_index);
	distance += build_get_distance(coords.data(), 0, coords.length() - 1, -1, NULL);

	LIST layer2 = elist[neigh_idx].get_links(2 + 3 * link.cam_index);
	for (auto _l2 : layer2)
	{
		CAMERA_LINK link2(_l2);

		uint32_t neighbour_eid2 = from_u32(elist[neigh_idx].get_nth_item(0) + C2_NEIGHBOURS_START + 4 + 4 * link2.zone_index);
		int32_t neigh_idx2 = elist.get_index(neighbour_eid2);
		if (neigh_idx2 == -1)
			continue;

		// not preloading everything
		uint32_t neighbour_flg2 = from_u32(elist[neigh_idx].get_nth_item(0) + C2_NEIGHBOURS_START + 4 + 4 * link2.zone_index + 0x20);
		if ((preloading_flag == PRELOADING_NOTHING || preloading_flag == PRELOADING_TEXTURES_ONLY) && (neighbour_flg2 == 0xF || neighbour_flg2 == 0x1F))
			continue;

		int32_t neigh_cam_count2 = elist[neigh_idx2].get_cam_item_count() / 3;
		if (link2.cam_index >= neigh_cam_count2)
		{
			printf("[warning] Zone %s is linked to zone %s's %d. camera path (indexing from 0) when it only has %d paths\n",
				elist[neigh_idx].m_ename, elist[neigh_idx2].m_ename, link2.cam_index, neigh_cam_count2);
			continue;
		}

		LIST neig_list = elist[neigh_idx2].get_neighbours();
		neig_list.copy_in(elist[neigh_idx2].get_sceneries());

		LIST slst_list{};
		slst_list.add(elist[neigh_idx2].get_nth_slst(link2.cam_index));

		elist.add_neighbour_coll_dependencies(full_list, elist[neigh_idx2]);
		coords = ntry.get_ent_path(cam_index);

		bool is_before = ENTRY::is_before(ntry, cam_index / 3, elist[neigh_idx2], link2.cam_index);
		int32_t slst_dist_w_orientation = is_before ? slst_distance : distance_with_penalty(slst_distance, backwards_penalty);
		int32_t neig_dist_w_orientation = is_before ? neig_distance : distance_with_penalty(neig_distance, backwards_penalty);

		if ((link.type == 2 && link.flag == 2 && link2.type == 1) || (link.type == 2 && link.flag == 1 && link2.type == 2))
		{
			build_load_list_zone_refs_forw(cam_length, full_list, distance, slst_dist_w_orientation, coords.data(), coords.length(), slst_list);
			build_load_list_zone_refs_forw(cam_length, full_list, distance, neig_dist_w_orientation, coords.data(), coords.length(), neig_list);
		}
		if ((link.type == 1 && link.flag == 2 && link2.type == 1) || (link.type == 1 && link.flag == 1 && link2.type == 2))
		{
			build_load_list_zone_refs_back(cam_length, full_list, distance, slst_dist_w_orientation, coords.data(), coords.length(), slst_list);
			build_load_list_zone_refs_back(cam_length, full_list, distance, neig_dist_w_orientation, coords.data(), coords.length(), neig_list);
		}
	}
}

// Starting from the 0th index it finds a path index at which the distance
// reaches cap distance and loads the entries specified by the additions list
// from point 0 to end index.
void build_load_list_zone_refs_back(int32_t cam_length, FULL_LABELED_LL& full_list, 
	int32_t distance, int32_t final_distance, int16_t* coords, int32_t path_length, LIST additions)
{
	int32_t end_index = 0;
	build_get_distance(coords, 0, path_length - 1, final_distance - distance, &end_index);

	if (end_index == 0)
		return;

	for (int32_t j = 0; j < end_index; j++)
		full_list[j].copy_in(additions, "refs_back");
}


// Starting from the n-1th index it finds a path index at which the distance
// reaches cap distance and loads the entries specified by the additions list
// from point start index n - 1.
void build_load_list_zone_refs_forw(int32_t cam_length, FULL_LABELED_LL& full_list,
	int32_t distance, int32_t final_distance, int16_t* coords, int32_t path_length, LIST additions)
{
	int32_t start_index = cam_length - 1;
	build_get_distance(coords, path_length - 1, 0, final_distance - distance, &start_index);

	if (start_index == cam_length - 1)
		return;

	for (int32_t j = start_index; j < cam_length; j++)
		full_list[j].copy_in(additions, "refs_forw");
}

// Calculates total distance the path has between given points, if cap is set
// it stop at given cap and returns index at which it stopped (final distance).

int32_t build_get_distance(int16_t* coords, int32_t start_index, int32_t end_index, int32_t cap, int32_t* final_index)
{
	int32_t distance = 0;
	int32_t index = start_index;

	int step = (start_index < end_index) ? 1 : -1;
	for (int32_t j = start_index; j != end_index; j += step)
	{
		if (cap != -1 && distance >= cap)
			break;

		const int16_t* p1 = &coords[j * 3];
		const int16_t* p2 = &coords[(j + step) * 3];

		distance += point_distance_3D(p1, p2);
		index = j;
	}

	if (cap != -1 && distance < cap)
		index += step;

	if (final_index)
		*final_index = index;

	return distance;
}

