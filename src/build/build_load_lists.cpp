#include "../include.h"
#include "../utils/utils.hpp"
#include "../game_structs/entry.hpp"

// responsible for generating load lists

/*
A function that for each zone's each camera path creates new load lists
using the provided list of permanently loaded entries and the provided list
of dependencies of certain entitites, gets models from the animations. For
the zone itself and its linked neighbours it adds all relatives and their
dependencies. Loads only one sound per sound chunk (sound chunks should
already be built by this point). Load list properties get removed and then
replaced by the provided list using 'camera_alter' function. Load lists get
sorted later during the nsd writing process.
*/
void build_remake_load_lists(ELIST& elist, uint32_t* gool_table,
	LIST permaloaded,
	DEPENDENCIES& subtype_info,
	DEPENDENCIES& collision,
	DEPENDENCIES& mus_deps,
	int32_t* config)
{
	int32_t load_list_sound_entry_inc_flag = config[CNFG_IDX_LL_SND_INCLUSION_FLAG];
	bool dbg_print = false;
	int32_t entry_count = int32_t(elist.size());

	// gets a list of sound eids (one per chunk) to make load lists smaller
	int32_t chunks[8]{};
	int32_t sound_chunk_count = 0;
	uint32_t sounds_to_load[8]{};

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (elist[i].entry_type() != ENTRY_TYPE_SOUND)
			continue;

		bool is_there = false;
		for (int32_t j = 0; j < sound_chunk_count; j++)
			if (chunks[j] == elist[i].chunk)
			{
				is_there = true;
				break;
			}

		if (!is_there)
		{
			chunks[sound_chunk_count] = elist[i].chunk;
			sounds_to_load[sound_chunk_count] = elist[i].eid;
			sound_chunk_count++;
		}
	}

	// for each zone entry do load list (also for each zone's each camera)
	for (int32_t i = 0; i < entry_count; i++)
	{
		if (elist[i].entry_type() != ENTRY_TYPE_ZONE)
			continue;

		int32_t cam_count = build_get_cam_item_count(elist[i]._data()) / 3;
		if (cam_count == 0)
			continue;

		printf("Making load lists for %s\n", elist[i].ename);

		LIST special_entries = build_get_special_entries(elist[i], elist);
		uint32_t music_ref = build_get_zone_track(elist[i]._data());

		for (int32_t j = 0; j < cam_count; j++)
		{
			printf("\t cam path %d\n", j);
			int32_t cam_offset = elist[i].get_nth_item_offset(2 + 3 * j);
			int32_t cam_length = build_get_path_length(elist[i]._data() + cam_offset);

			// full non-delta load list used to represent the load list during building
			std::vector<LIST> full_load(cam_length);

			// add permaloaded entries
			for (int32_t k = 0; k < cam_length; k++)
				full_load[k].copy_in(permaloaded);

			for (auto& mus_dep : mus_deps)
			{
				if (mus_dep.type == music_ref)
				{
					for (int32_t k = 0; k < cam_length; k++)
						full_load[k].copy_in(mus_dep.dependencies);

					if (dbg_print)
					{
						printf("\t\tcopied in %d music deps\n", mus_dep.dependencies.count());
						for (auto& d : mus_dep.dependencies)
							printf("\t\t\t %s\n", eid2str(d));
					}
				}
			}

			if (dbg_print)
				printf("Copied in permaloaded and music refs\n");

			// add current zone's special entries
			for (int32_t k = 0; k < cam_length; k++)
				full_load[k].copy_in(special_entries);

			if (dbg_print)
				printf("Copied in special %d\n", special_entries.count());

			// add relatives (directly related entries like slsts, neighbours, scenery
			// might not be necessary								
			for (int32_t l = 0; l < cam_length; l++)
				full_load[l].copy_in(elist[i].related);

			if (dbg_print)
				printf("Copied in deprecate relatives\n");

			// all sounds
			if (load_list_sound_entry_inc_flag == 0)
			{
				for (int32_t k = 0; k < entry_count; k++)
				{
					if (elist[k].entry_type() != ENTRY_TYPE_SOUND)
						continue;

					for (int32_t l = 0; l < cam_length; l++)
						full_load[l].add(elist[k].eid);
				}
			}
			// one sound per chunk
			if (load_list_sound_entry_inc_flag == 1)
			{
				for (int32_t k = 0; k < cam_length; k++)
					for (int32_t l = 0; l < sound_chunk_count; l++)
						full_load[k].add(sounds_to_load[l]);
			}

			if (dbg_print)
				printf("Copied in sounds\n");

			// add direct neighbours
			LIST neighbours = elist[i].get_neighbours();
			for (int32_t k = 0; k < cam_length; k++)
				full_load[k].copy_in(neighbours);

			if (dbg_print)
				printf("Copied in neighbours\n");

			// for each scenery in current zone's scen reference list, add its
			// textures to the load list				
			LIST sceneries = build_get_sceneries(elist[i]._data());
			for (auto& scenery : sceneries)
			{
				int32_t scenery_index = elist.get_index(scenery);
				for (int32_t l = 0; l < cam_length; l++)
				{
					build_add_scen_textures_to_list(elist[scenery_index]._data(),
						&full_load[l]);
				}
			}

			if (dbg_print)
				printf("Copied in some scenery stuff\n");

			// path link, draw list and other bs dependent additional load list
			// improvements
			build_load_list_util(i,
				2 + 3 * j,
				full_load,
				cam_length,
				elist,
				subtype_info,
				collision,
				config);

			if (dbg_print)
				printf("Load list util ran\n");

			// checks whether the load list doesnt try to load more than 8 textures,
			build_texture_count_check(elist, full_load, cam_length, i, j);

			if (dbg_print)
				printf("Texture chunk was checked\n");

			// creates and initialises delta representation of the load list
			std::vector<LIST> listA(cam_length);
			std::vector<LIST> listB(cam_length);

			// converts full load list to delta based, then creates game-format load
			// list properties based on the delta lists
			build_load_list_to_delta(full_load, listA, listB, cam_length, elist, 0);
			PROPERTY prop_0x208 = build_make_load_list_prop(listA, cam_length, 0x208);
			PROPERTY prop_0x209 = build_make_load_list_prop(listB, cam_length, 0x209);

			if (dbg_print)
				printf("Converted full list to delta and delta to props\n");

			// removes existing load list properties, inserts newly made ones
			build_entity_alter(elist[i], 2 + 3 * j, build_rem_property, 0x208, NULL);
			build_entity_alter(elist[i], 2 + 3 * j, build_rem_property, 0x209, NULL);
			build_entity_alter(elist[i], 2 + 3 * j, build_add_property, 0x208, &prop_0x208);
			build_entity_alter(elist[i], 2 + 3 * j, build_add_property, 0x209, &prop_0x209);

			if (dbg_print)
				printf("Replaced load list props\n");

			if (dbg_print)
				printf("Freed some stuff, end\n");
			// free(prop_0x208.data);
			// free(prop_0x209.data);
		}

	}
}

// for sorting draw list properties by id
int32_t cmp_func_draw(const void* a, const void* b)
{
	uint32_t int_a = *(uint32_t*)a;
	uint32_t int_b = *(uint32_t*)b;

	DRAW_ITEM item_a(int_a);
	DRAW_ITEM item_b(int_b);

	return item_a.ID - item_b.ID;
}


//  Converts the full load list into delta form.
void build_load_list_to_delta(std::vector<LIST>& full_load, std::vector<LIST>& listA, std::vector<LIST>& listB, int32_t cam_length, ELIST& elist, bool is_draw)
{
	// full item, listA point 0
	for (int32_t i = 0; i < full_load[0].count(); i++)
		if (is_draw || elist.get_index(full_load[0][i]) != -1)
			listA[0].add(full_load[0][i]);

	// full item, listB point n-1
	int32_t n = cam_length - 1;
	for (int32_t i = 0; i < full_load[n].count(); i++)
		if (is_draw || elist.get_index(full_load[n][i]) != -1)
			listB[n].add(full_load[n][i]);

	// creates delta items
	// for each point it takes full load list and for each entry checks whether it
	// has just become loaded/deloaded
	for (int32_t i = 1; i < cam_length; i++)
	{
		for (int32_t j = 0; j < full_load[i].count(); j++)
		{
			uint32_t curr_eid = full_load[i][j];
			if (!is_draw && elist.get_index(curr_eid) == -1)
				continue;

			// is loaded on i-th point but not on i-1th point -> just became loaded,
			// add to listA[i]
			if (full_load[i - 1].find(curr_eid) == -1)
				listA[i].add(curr_eid);
		}

		for (int32_t j = 0; j < full_load[i - 1].count(); j++)
		{
			uint32_t curr_eid = full_load[i - 1][j];
			if (!is_draw && elist.get_index(curr_eid) == -1)
				continue;

			// is loaded on i-1th point but not on i-th point -> no longer loaded, add
			// to listB[i - 1]
			if (full_load[i].find(curr_eid) == -1)
				listB[i - 1].add(curr_eid);
		}
	}

	// gets rid of cases when an item is in both listA and listB on the same index
	// (getting loaded and instantly deloaded or vice versa) if its in both it
	// removes it from both
	for (int32_t i = 0; i < cam_length; i++)
	{
		LIST iter_copy{};
		iter_copy.copy_in(listA[i]);

		for (int32_t j = 0; j < iter_copy.count(); j++)
			if (listB[i].find(iter_copy[j]) != -1)
			{
				listA[i].remove(iter_copy[j]);
				listB[i].remove(iter_copy[j]);
			}
	}

	if (is_draw)
	{
		auto cmp_func_draw = [](uint32_t a, uint32_t b)
			{
				DRAW_ITEM item_a(a);
				DRAW_ITEM item_b(b);

				return item_a.ID < item_b.ID;
			};

		// sort by entity id
		for (int32_t i = 0; i < cam_length; i++)
		{
			std::sort(listA[i].begin(), listA[i].end(), cmp_func_draw);
			std::sort(listB[i].begin(), listB[i].end(), cmp_func_draw);
		}
	}
}

/*
Handles most load list creating, except for permaloaded and sound entries.
First part is used for neighbours & scenery & slsts.
Second part is used for draw lists.
*/
void build_load_list_util(
	int32_t zone_index,
	int32_t camera_index,
	std::vector<LIST>& full_list,
	int32_t cam_length,
	ELIST& elist,
	DEPENDENCIES sub_info,
	DEPENDENCIES collisions,
	int32_t* config)
{
	// neighbours, slsts, scenery
	LIST links = elist[zone_index].get_links(camera_index);
	for (int32_t i = 0; i < links.count(); i++)
	{
		build_load_list_util_util(
			zone_index, camera_index, links[i], full_list, cam_length, elist, config, collisions);
	}

	// draw lists
	for (int32_t i = 0; i < cam_length; i++)
	{
		LIST neighbour_list{};
		// get a list of entities drawn within set distance of current camera point
		LIST entity_list = build_get_entity_list(i, zone_index, camera_index, cam_length, elist, &neighbour_list, config);

		// get a list of types and subtypes from the entity list
		LIST types_subtypes = build_get_types_subtypes(elist, entity_list, neighbour_list);

		// copy in dependency list for each found type/subtype
		for (int32_t j = 0; j < types_subtypes.count(); j++)
		{
			int32_t type = types_subtypes[j] >> 16;
			int32_t subtype = types_subtypes[j] & 0xFF;

			for (auto& info : sub_info)
			{
				if (info.subtype == subtype && info.type == type)
				{
					full_list[i].copy_in(info.dependencies);
				}
			}
		}
	}
}


// Makes a load list property from the input arrays
PROPERTY build_make_load_list_prop(std::vector<LIST>& list_array, int32_t cam_length, int32_t code)
{
	int32_t delta_counter = 0;
	int32_t total_length = 0;
	PROPERTY prop;

	// count total length and individual deltas
	for (int32_t i = 0; i < cam_length; i++)
		if (list_array[i].count() != 0)
		{
			total_length += list_array[i].count() * 4; // space individual load list items of
			// current sublist will take up
			total_length += 4; // each load list sublist uses 2 bytes for its length
			// and 2 bytes for its index
			delta_counter++;
		}

	// header info
	*(int16_t*)(prop.header) = code;
	if (code == ENTITY_PROP_CAM_LOAD_LIST_A || code == ENTITY_PROP_CAM_LOAD_LIST_B)
	{
		if (delta_counter > 1)
			*(int16_t*)(prop.header + 4) = 0x0464;
		else
			*(int16_t*)(prop.header + 4) = 0x0424;
	}
	if (code == ENTITY_PROP_CAM_DRAW_LIST_A || code == ENTITY_PROP_CAM_DRAW_LIST_B)
	{
		if (delta_counter > 1)
			*(int16_t*)(prop.header + 4) = 0x0473;
		else
			*(int16_t*)(prop.header + 4) = 0x0433;
	}
	*(int16_t*)(prop.header + 6) = delta_counter;

	prop.length = total_length;
	prop.data = (uint8_t*)try_malloc(total_length * sizeof(uint8_t)); // freed by caller

	int32_t indexer = 0;
	int32_t offset = 4 * delta_counter;
	for (int32_t i = 0; i < cam_length; i++)
		if (list_array[i].count() != 0)
		{
			*(int16_t*)(prop.data + 2 * indexer) = list_array[i].count(); // i-th sublist's length (item count)
			*(int16_t*)(prop.data + 2 * (indexer + delta_counter)) = i; // i-th sublist's index
			for (int32_t j = 0; j < list_array[i].count(); j++)
				*(int32_t*)(prop.data + offset + 4 * j) = list_array[i][j]; // individual items
			offset += list_array[i].count() * 4;
			indexer++;
		}

	return prop;
}


// Deals with slst and neighbour/scenery references of path linked entries.
void build_load_list_util_util(int32_t zone_index,
	int32_t cam_index,
	int32_t link_int,
	std::vector<LIST>& full_list,
	int32_t cam_length,
	ELIST& elist,
	int32_t* config,
	DEPENDENCIES collisisons)
{
	char eid1[100] = "";
	char eid2[100] = "";

	int32_t slst_distance = config[CNFG_IDX_LL_SLST_DIST_VALUE];
	int32_t neig_distance = config[CNFG_IDX_LL_NEIGH_DIST_VALUE];
	int32_t preloading_flag = config[CNFG_IDX_LL_TRNS_PRLD_FLAG];
	int32_t backwards_penalty = config[CNFG_IDX_LL_BACKWARDS_PENALTY];

	int32_t item1off = elist[zone_index].get_nth_item_offset(0);
	int16_t* coords;
	int32_t path_length = 0;
	int32_t distance = 0;

	CAMERA_LINK link(link_int);

	uint32_t neighbour_eid = from_u32(elist[zone_index]._data() + item1off + C2_NEIGHBOURS_START + 4 + 4 * link.zone_index);
	uint32_t neighbour_flg = from_u32(elist[zone_index]._data() + item1off + C2_NEIGHBOURS_START + 4 + 4 * link.zone_index + 0x20);

	int32_t neigh_idx = elist.get_index(neighbour_eid);
	if (neigh_idx == -1)
		return;

	// if preloading nothing
	if (preloading_flag == PRELOADING_NOTHING && (neighbour_flg == 0xF || neighbour_flg == 0x1F))
		return;

	if (preloading_flag == PRELOADING_TEXTURES_ONLY || preloading_flag == PRELOADING_ALL)
	{
		for (auto& scenery : build_get_sceneries(elist[neigh_idx]._data()))
		{
			int32_t scenery_index = elist.get_index(scenery);

			if (link.type == 1)
			{
				int32_t end_index = (cam_length - 1) / 2 - 1;
				for (int32_t j = 0; j < end_index; j++)
					build_add_scen_textures_to_list(elist[scenery_index]._data(),
						&full_list[j]);
			}
			else if (link.type == 2)
			{
				int32_t start_index = (cam_length - 1) / 2 + 1;
				for (int32_t j = start_index; j < cam_length; j++)
					build_add_scen_textures_to_list(elist[scenery_index]._data(),
						&full_list[j]);
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
		full_list[j].copy_in(elist[neigh_idx].related);

	int32_t neighbour_cam_count = build_get_cam_item_count(elist[neigh_idx]._data()) / 3;
	if (link.cam_index >= neighbour_cam_count)
	{
		printf("[warning] Zone %s is linked to zone %s's %d. camera path (indexing from 0) when it only has %d paths\n",
			elist[zone_index].ename, elist[neigh_idx].ename, link.cam_index, neighbour_cam_count);
		return;
	}

	uint8_t* item = elist[neigh_idx].get_nth_item(2 + 3 * link.cam_index);
	uint32_t slst = build_get_slst(item);
	for (int32_t i = 0; i < cam_length; i++)
		full_list[i].add(slst);

	build_add_collision_dependencies(full_list, 0, cam_length, elist[zone_index], collisisons, elist);
	build_add_collision_dependencies(full_list, 0, cam_length, elist[neigh_idx], collisisons, elist);

	coords = build_get_path(elist, neigh_idx, 2 + 3 * link.cam_index, &path_length);
	distance += build_get_distance(coords, 0, path_length - 1, -1, NULL);

	LIST layer2 = elist[neigh_idx].get_links(2 + 3 * link.cam_index);
	for (int32_t i = 0; i < layer2.count(); i++)
	{
		int32_t item1off2 = from_u32(elist[neigh_idx]._data() + 0x10);
		CAMERA_LINK link2(layer2[i]);

		uint32_t neighbour_eid2 = from_u32(elist[neigh_idx]._data() + item1off2 + C2_NEIGHBOURS_START + 4 + 4 * link2.zone_index);
		uint32_t neighbour_flg2 = from_u32(elist[neigh_idx]._data() + item1off2 + C2_NEIGHBOURS_START + 4 + 4 * link2.zone_index + 0x20);

		int32_t neigh_idx2 = elist.get_index(neighbour_eid2);
		if (neigh_idx2 == -1)
			continue;

		// not preloading everything
		if ((preloading_flag == PRELOADING_NOTHING || preloading_flag == PRELOADING_TEXTURES_ONLY) && (neighbour_flg2 == 0xF || neighbour_flg2 == 0x1F))
			continue;

		int32_t neighbour_cam_count2 = build_get_cam_item_count(elist[neigh_idx2]._data()) / 3;
		if (link2.cam_index >= neighbour_cam_count2)
		{
			printf("[warning] Zone %s is linked to zone %s's %d. camera path (indexing from 0) when it only has %d paths\n",
				elist[neigh_idx].ename, elist[neigh_idx2].ename, link2.cam_index, neighbour_cam_count2);
			continue;
		}

		uint8_t* item2 = elist[neigh_idx2].get_nth_item(2 + 3 * link2.cam_index);
		uint32_t slst2 = build_get_slst(item2);

		LIST neig_list = elist[neigh_idx2].get_neighbours();
		neig_list.copy_in(build_get_sceneries(elist[neigh_idx2]._data()));

		LIST slst_list{};
		slst_list.add(slst2);

		build_add_collision_dependencies(full_list, 0, cam_length, elist[neigh_idx2], collisisons, elist);
		coords = build_get_path(elist, zone_index, cam_index, &path_length);

		int32_t slst_dist_w_orientation = slst_distance;
		int32_t neig_dist_w_orientation = neig_distance;

		if (build_is_before(elist, zone_index, cam_index / 3, neigh_idx2, link2.cam_index))
		{
			slst_dist_w_orientation = build_dist_w_penalty(slst_distance, backwards_penalty);
			neig_dist_w_orientation = build_dist_w_penalty(neig_distance, backwards_penalty);
		}

		if ((link.type == 2 && link.flag == 2 && link2.type == 1) || (link.type == 2 && link.flag == 1 && link2.type == 2))
		{
			build_load_list_util_util_forw(cam_length, full_list, distance, slst_dist_w_orientation, coords, path_length, slst_list);
			build_load_list_util_util_forw(cam_length, full_list, distance, neig_dist_w_orientation, coords, path_length, neig_list);
		}
		if ((link.type == 1 && link.flag == 2 && link2.type == 1) || (link.type == 1 && link.flag == 1 && link2.type == 2))
		{
			build_load_list_util_util_back(cam_length, full_list, distance, slst_dist_w_orientation, coords, path_length, slst_list);
			build_load_list_util_util_back(cam_length, full_list, distance, neig_dist_w_orientation, coords, path_length, neig_list);
		}
	}
}

// Starting from the 0th index it finds a path index at which the distance
// reaches cap distance and loads the entries specified by the additions list
// from point 0 to end index.
void build_load_list_util_util_back(int32_t cam_length, std::vector<LIST>& full_list, int32_t distance, int32_t final_distance, int16_t* coords, int32_t path_length, LIST additions)
{
	int32_t end_index = 0;
	build_get_distance(coords, 0, path_length - 1, final_distance - distance, &end_index);

	if (end_index == 0)
		return;

	for (int32_t j = 0; j < end_index; j++)
		full_list[j].copy_in(additions);
}


// Starting from the n-1th index it finds a path index at which the distance
// reaches cap distance and loads the entries specified by the additions list
// from point start index n - 1.
void build_load_list_util_util_forw(int32_t cam_length, std::vector<LIST>& full_list,
	int32_t distance, int32_t final_distance, int16_t* coords, int32_t path_length, LIST additions)
{
	int32_t start_index = cam_length - 1;
	int32_t j;

	build_get_distance(
		coords, path_length - 1, 0, final_distance - distance, &start_index);
	if (start_index == cam_length - 1)
		return;

	for (j = start_index; j < cam_length; j++)
		full_list[j].copy_in(additions);
}

// Calculates total distance the path has between given points, if cap is set
// it stop at given cap and returns index at which it stopped (final distance).
int32_t build_get_distance(int16_t* coords, int32_t start_index, int32_t end_index, int32_t cap, int32_t* final_index)
{
	int32_t distance = 0;
	int32_t index = start_index;

	if (start_index > end_index)
	{
		index = start_index;
		for (int32_t j = start_index; j > end_index; j--)
		{
			if (cap != -1)
				if (distance >= cap)
					break;
			int16_t x1 = coords[j * 3 - 3];
			int16_t y1 = coords[j * 3 - 2];
			int16_t z1 = coords[j * 3 - 1];
			int16_t x2 = coords[j * 3 + 0];
			int16_t y2 = coords[j * 3 + 1];
			int16_t z2 = coords[j * 3 + 2];
			distance += point_distance_3D(x1, x2, y1, y2, z1, z2);
			index = j;
		}
		if (distance < cap)
			index--;
	}

	if (start_index < end_index)
	{
		index = start_index;
		for (int32_t j = start_index; j < end_index; j++)
		{
			if (cap != -1)
				if (distance >= cap)
					break;
			int16_t x1 = coords[j * 3 + 0];
			int16_t y1 = coords[j * 3 + 1];
			int16_t z1 = coords[j * 3 + 2];
			int16_t x2 = coords[j * 3 + 3];
			int16_t y2 = coords[j * 3 + 4];
			int16_t z2 = coords[j * 3 + 5];
			distance += point_distance_3D(x1, x2, y1, y2, z1, z2);
			index = j;
		}
		if (distance < cap)
			index++;
	}

	if (final_index != NULL)
		*final_index = index;
	return distance;
}

// recalculated distance cap for backwards loading, its fixed point real number
// cuz config is an int32_t array and im not passing more arguments thru this
// hell
int32_t
build_dist_w_penalty(int32_t distance, int32_t backwards_penalty)
{
	return (
		(int32_t)((1.0 - ((double)backwards_penalty) / PENALTY_MULT_CONSTANT) * distance));
}

// checks whether a cam path is backwards relative to another cam path
int32_t build_is_before(ELIST& elist,
	int32_t zone_index,
	int32_t camera_index,
	int32_t neigh_idx,
	int32_t neighbour_cam_index)
{
	int32_t distance_neighbour = elist[neigh_idx].distances[neighbour_cam_index];
	int32_t distance_current = elist[zone_index].distances[camera_index];

	return (distance_neighbour < distance_current);
}

// collects list of IDs using draw lists of neighbouring camera paths
LIST build_get_entity_list(int32_t point_index, int32_t zone_index, int32_t camera_index, int32_t cam_length, ELIST& elist, LIST* neighbours, int32_t* config)
{
	int32_t draw_dist = config[CNFG_IDX_LL_DRAW_DIST_VALUE];
	int32_t preloading_flag = config[CNFG_IDX_LL_TRNS_PRLD_FLAG];
	int32_t backwards_penalty = config[CNFG_IDX_LL_BACKWARDS_PENALTY];

	LIST entity_list{};

	neighbours->copy_in(elist[zone_index].get_neighbours());

	std::vector<LIST> draw_list_zone = build_get_complete_draw_list(elist, elist[zone_index], camera_index, cam_length);
	for (int32_t i = 0; i < cam_length; i++)
		entity_list.copy_in(draw_list_zone[i]);

	int32_t coord_count;
	int16_t* coords = build_get_path(elist, zone_index, camera_index, &coord_count);
	if (!coords)
		return entity_list;

	LIST links = elist[zone_index].get_links(camera_index);
	for (int32_t i = 0; i < links.count(); i++)
	{
		int32_t distance = 0;
		CAMERA_LINK link(links[i]);
		if (link.type == 1)
			distance += build_get_distance(coords, point_index, 0, -1, NULL);
		if (link.type == 2)
			distance += build_get_distance(coords, point_index, cam_length - 1, -1, NULL);

		uint32_t eid_offset = from_u32(elist[zone_index]._data() + 0x10) + 4 + link.zone_index * 4 + C2_NEIGHBOURS_START;
		uint32_t neighbour_eid = from_u32(elist[zone_index]._data() + eid_offset);
		uint32_t neighbour_flg = from_u32(elist[zone_index]._data() + eid_offset + 0x20);

		int32_t neigh_idx = elist.get_index(neighbour_eid);
		if (neigh_idx == -1)
			continue;

		// preloading everything check
		if (preloading_flag != PRELOADING_ALL && (neighbour_flg == 0xF || neighbour_flg == 0x1F))
			continue;

		neighbours->copy_in(elist[neigh_idx].get_neighbours());

		int32_t neighbour_cam_length;
		int16_t* coords2 = build_get_path(elist, neigh_idx, 2 + 3 * link.cam_index, &neighbour_cam_length);
		if (!coords2)
			continue;

		std::vector<LIST> draw_list_neighbour1 = build_get_complete_draw_list(elist, elist[neigh_idx], 2 + 3 * link.cam_index, neighbour_cam_length);

		int32_t draw_dist_w_orientation = draw_dist;
		if (build_is_before(elist, zone_index, camera_index / 3, neigh_idx, link.cam_index))
		{
			draw_dist_w_orientation = build_dist_w_penalty(draw_dist, backwards_penalty);
		}

		int32_t point_index2;
		if (link.flag == 1)
		{
			distance += build_get_distance(coords2, 0, neighbour_cam_length - 1, draw_dist_w_orientation - distance, &point_index2);

			for (int32_t j = 0; j < point_index2; j++)
				entity_list.copy_in(draw_list_neighbour1[j]);
		}
		if (link.flag == 2)
		{
			distance += build_get_distance(coords2, neighbour_cam_length - 1, 0, draw_dist_w_orientation - distance, &point_index2);
			for (int32_t j = point_index2; j < neighbour_cam_length - 1; j++)
				entity_list.copy_in(draw_list_neighbour1[j]);
		}

		if (distance >= draw_dist_w_orientation)
			continue;

		LIST layer2 = elist[neigh_idx].get_links(2 + 3 * link.cam_index);
		for (int32_t j = 0; j < layer2.count(); j++)
		{
			CAMERA_LINK link2(layer2[j]);
			uint32_t eid_offset2 = from_u32(elist[neigh_idx]._data() + 0x10) + 4 + link2.zone_index * 4 + C2_NEIGHBOURS_START;
			uint32_t neighbour_eid2 = from_u32(elist[neigh_idx]._data() + eid_offset2);
			uint32_t neighbour_flg2 = from_u32(elist[neigh_idx]._data() + eid_offset2 + 0x20);

			int32_t neigh_idx2 = elist.get_index(neighbour_eid2);
			if (neigh_idx2 == -1)
				continue;

			// preloading everything check
			if (preloading_flag != PRELOADING_ALL && (neighbour_flg2 == 0xF || neighbour_flg2 == 0x1F))
				continue;

			neighbours->copy_in(elist[neigh_idx2].get_neighbours());

			int32_t neighbour_cam_length2;
			int16_t* coords3 = build_get_path(elist, neigh_idx2, 2 + 3 * link2.cam_index, &neighbour_cam_length2);
			if (!coords3)
				continue;

			std::vector<LIST> draw_list_neighbour2 = build_get_complete_draw_list(elist, elist[neigh_idx2], 2 + 3 * link2.cam_index, neighbour_cam_length2);

			int32_t point_index3;
			draw_dist_w_orientation = draw_dist;
			if (build_is_before(elist, zone_index, camera_index / 3, neigh_idx2, link2.cam_index))
			{
				draw_dist_w_orientation = build_dist_w_penalty(draw_dist, backwards_penalty);
			}

			// start to end
			if ((link.type == 2 && link2.type == 2 && link2.flag == 1) || (link.type == 1 && link2.type == 1 && link2.flag == 1))
			{
				build_get_distance(coords3, 0, neighbour_cam_length2 - 1, draw_dist_w_orientation - distance, &point_index3);
				for (int32_t k = 0; k < point_index3; k++)
					entity_list.copy_in(draw_list_neighbour2[k]);
			}

			// end to start
			if ((link.type == 2 && link2.type == 2 && link2.flag == 1) || (link.type == 1 && link2.type == 1 && link2.flag == 2))
			{
				build_get_distance(coords3, neighbour_cam_length2 - 1, 0, draw_dist_w_orientation - distance, &point_index3);
				for (int32_t k = point_index3; k < neighbour_cam_length2; k++)
					entity_list.copy_in(draw_list_neighbour2[k]);
			}
		}
	}

	return entity_list;
}

//  Retrieves a list of types & subtypes from the IDs and zones.
LIST build_get_types_subtypes(ELIST& elist, LIST entity_list, LIST neighbour_list)
{
	LIST type_subtype_list{};
	for (int32_t i = 0; i < neighbour_list.count(); i++)
	{
		int32_t curr_index = elist.get_index(neighbour_list[i]);
		if (curr_index == -1)
			continue;

		int32_t cam_count = build_get_cam_item_count(elist[curr_index]._data());
		int32_t entity_count = build_get_entity_count(elist[curr_index]._data());

		for (int32_t j = 0; j < entity_count; j++)
		{
			uint8_t* entity = elist[curr_index].get_nth_item(2 + cam_count + j);
			int32_t ID = build_get_entity_prop(entity, ENTITY_PROP_ID);
			if (entity_list.find(ID) != -1)
			{
				int32_t type = build_get_entity_prop(entity, ENTITY_PROP_TYPE);
				int32_t subtype = build_get_entity_prop(entity, ENTITY_PROP_SUBTYPE);
				type_subtype_list.add((type << 16) + subtype);
			}
		}
	}

	return type_subtype_list;
}


// checks/prints if there are any entities whose type/subtype has no dependencies.
void build_find_unspecified_entities(ELIST& elist, DEPENDENCIES sub_info)
{
	int32_t entry_count = elist.count();
	printf("\n");

	LIST considered{};
	for (int32_t i = 0; i < entry_count; i++)
	{
		if (elist[i].entry_type() != ENTRY_TYPE_ZONE)
			continue;

		int32_t cam_count = build_get_cam_item_count(elist[i]._data());
		int32_t entity_count = build_get_entity_count(elist[i]._data());
		for (int32_t j = 0; j < entity_count; j++)
		{
			uint8_t* entity = elist[i]._data() + from_u32(elist[i]._data() + 0x10 + (2 + cam_count + j) * 4);
			int32_t type = build_get_entity_prop(entity, ENTITY_PROP_TYPE);
			int32_t subt = build_get_entity_prop(entity, ENTITY_PROP_SUBTYPE);
			int32_t enID = build_get_entity_prop(entity, ENTITY_PROP_ID);
			if (type >= 64 || type < 0 || subt < 0)
			{
				printf("[warning] Zone %s entity %2d is invalid! (type %2d subtype %2d)\n", elist[i].ename, j, type, subt);
				continue;
			}

			if (considered.find((type << 16) + subt) != -1)
				continue;

			considered.add((type << 16) + subt);

			bool found = false;
			for (auto& dep : sub_info)
			{
				if (dep.subtype == subt && dep.type == type)
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				printf("[warning] Entity with type %2d subtype %2d has no dependency list! (e.g. Zone %s entity %2d ID %3d)\n",
					type, subt, elist[i].ename, j, enID);
			}
		}

	}
}


// reads draw lists of the camera of the zone, returns in a non-delta form.
std::vector<LIST> build_get_complete_draw_list(ELIST& elist, ENTRY& zone, int32_t cam_index, int32_t cam_length)
{
	std::vector<LIST> draw_list{};
	draw_list.resize(cam_length);

	LIST list = {};

	DRAW_LIST draw_list2 = get_draw_lists(zone, cam_index);

	int32_t sublist_index = 0;
	for (size_t i = 0; i < size_t(cam_length) && sublist_index < draw_list2.size(); i++)
	{
		bool new_read = false;
		auto& sublist = draw_list2[sublist_index];
		if (sublist.index == i)
		{
			if (draw_list2[sublist_index].type == 'B')
			{
				for (auto& draw_uint : sublist.list)
				{
					DRAW_ITEM draw_item(draw_uint);
					list.add(draw_item.ID);
				}
			}

			if (draw_list2[sublist_index].type == 'A')
			{
				for (auto& draw_uint : sublist.list)
				{
					DRAW_ITEM draw_item(draw_uint);
					list.remove(draw_item.ID);
				}
			}

			sublist_index++;
			new_read = true;
		}
		draw_list[i].copy_in(list);

		// fixes cases when both draw list A and B contained sublist w/ the same
		// indexes, subsequent sublists didnt get
		if (new_read)
			i--;
	}

	return draw_list;
}

// Searches the entry and for each collision type it adds its dependencies to the lists 
void build_add_collision_dependencies(std::vector<LIST>& full_list,
	int32_t start_index,
	int32_t end_index,
	ENTRY& entry,
	DEPENDENCIES collisions,
	ELIST& elist)
{
	LIST neighbours = entry.get_neighbours();
	for (int32_t x = 0; x < neighbours.count(); x++)
	{
		int32_t index = elist.get_index(neighbours[x]);
		if (index == -1)
			continue;

		int32_t item2off = from_u32(elist[index]._data() + 0x14);
		uint8_t* item = elist[index]._data() + item2off;
		int32_t count = from_u32(item + 0x18);

		for (int32_t i = 0; i < count + 2; i++)
		{
			int16_t type = from_u16(item + 0x24 + 2 * i);
			if (type % 2 == 0)
				continue;

			for (auto& coll_dep : collisions)
				if (type == coll_dep.type)
					for (int32_t k = start_index; k < end_index; k++)
						full_list[k].copy_in(coll_dep.dependencies);
		}
	}
}

//  Checks for whether the current camera isnt trying to load more than 8 textures
void build_texture_count_check(ELIST& elist, std::vector<LIST>& full_load, int32_t cam_length, int32_t i, int32_t j)
{
	int32_t over_count = 0;
	uint32_t over_textures[20];
	int32_t point = 0;

	for (int32_t k = 0; k < cam_length; k++)
	{
		int32_t texture_count = 0;
		uint32_t textures[20]{};
		for (int32_t l = 0; l < full_load[k].count(); l++)
		{
			int32_t idx = elist.get_index(full_load[k][l]);
			if (idx == -1)			
				printf("Trying to load invalid entry %s\n", eid2str(full_load[k][l]));			
			else if (elist[idx].entry_type() == -1 && eid2str(full_load[k][l])[4] == 'T')
				textures[texture_count++] = full_load[k][l];
		}

		if (texture_count > over_count)
		{
			over_count = texture_count;
			point = k;
			memcpy(over_textures, textures, texture_count * sizeof(uint32_t));
		}
	}

	if (over_count > 8)
	{
		printf("[warning] Zone %s cam path %d trying to load %d textures! (eg on point %d)\n",
			elist[i].ename, j, over_count, point);

		for (int32_t k = 0; k < over_count; k++)
			printf("\t%s", eid2str(over_textures[k]));
		printf("\n");
	}
}

// replaces nth item in a zone with an item with specified data and length
void build_replace_item(ENTRY& zone,
	int32_t item_index,
	uint8_t* new_item,
	int32_t item_size)
{
	int32_t i, offset;
	int32_t item_count = build_item_count(zone._data());
	int32_t first_item_offset = 0x14 + 4 * item_count;

	int32_t* item_lengths = (int32_t*)try_malloc(item_count * sizeof(int32_t));
	uint8_t** items = (uint8_t**)try_malloc(item_count * sizeof(uint8_t**));

	for (int32_t i = 0; i < item_count; i++)
		item_lengths[i] = zone.get_nth_item_offset(i + 1) - zone.get_nth_item_offset(i);

	for (offset = first_item_offset, i = 0; i < item_count;
		offset += item_lengths[i], i++)
	{
		items[i] = (uint8_t*)try_malloc(item_lengths[i]);
		memcpy(items[i], zone._data() + offset, item_lengths[i]);
	}

	item_lengths[item_index] = item_size;
	items[item_index] = new_item;

	int32_t new_size = first_item_offset;
	for (int32_t i = 0; i < item_count; i++)
		new_size += item_lengths[i];

	uint8_t* new_data = (uint8_t*)try_malloc(new_size);
	*(int32_t*)(new_data) = MAGIC_ENTRY;
	*(int32_t*)(new_data + 0x4) = zone.eid;
	*(int32_t*)(new_data + 0x8) = ENTRY_TYPE_ZONE;
	*(int32_t*)(new_data + 0xC) = item_count;

	for (offset = first_item_offset, i = 0; i < item_count + 1;
		offset += item_lengths[i], i++)
		*(int32_t*)(new_data + 0x10 + i * 4) = offset;

	for (offset = first_item_offset, i = 0; i < item_count;
		offset += item_lengths[i], i++)
		memcpy(new_data + offset, items[i], item_lengths[i]);

	zone.esize = new_size;
	zone.data.resize(new_size);
	memcpy(zone._data(), new_data, new_size);

	for (int32_t i = 0; i < item_count; i++)
		free(items[i]);
	free(items);
	free(item_lengths);
}

// deconstructs, alters specified item using the func_arg function, reconstructs
void build_entity_alter(ENTRY& zone, int32_t item_index, uint8_t* (func_arg)(uint32_t, uint8_t*, int32_t*, PROPERTY*), int32_t property_code, PROPERTY* prop)
{
	int32_t i, offset;
	int32_t item_count = build_item_count(zone._data());
	int32_t first_item_offset = 0x14 + 4 * item_count;

	int32_t* item_lengths = (int32_t*)try_malloc(item_count * sizeof(int32_t));
	uint8_t** items = (uint8_t**)try_malloc(item_count * sizeof(uint8_t**));
	for (int32_t i = 0; i < item_count; i++)
		item_lengths[i] = zone.get_nth_item_offset(i + 1) - zone.get_nth_item_offset(i);

	for (offset = first_item_offset, i = 0; i < item_count;
		offset += item_lengths[i], i++)
	{
		items[i] = (uint8_t*)try_malloc(item_lengths[i]);
		memcpy(items[i], zone._data() + offset, item_lengths[i]);
	}

	items[item_index] = func_arg(property_code, items[item_index], &item_lengths[item_index], prop);

	int32_t new_size = first_item_offset;
	for (int32_t i = 0; i < item_count; i++)
		new_size += item_lengths[i];

	uint8_t* new_data = (uint8_t*)try_malloc(new_size);
	*(int32_t*)(new_data) = MAGIC_ENTRY;
	*(int32_t*)(new_data + 0x4) = zone.eid;
	*(int32_t*)(new_data + 0x8) = ENTRY_TYPE_ZONE;
	*(int32_t*)(new_data + 0xC) = item_count;

	for (offset = first_item_offset, i = 0; i < item_count + 1;
		offset += item_lengths[i], i++)
		*(int32_t*)(new_data + 0x10 + i * 4) = offset;

	for (offset = first_item_offset, i = 0; i < item_count;
		offset += item_lengths[i], i++)
		memcpy(new_data + offset, items[i], item_lengths[i]);

	zone.esize = new_size;
	zone.data.resize(new_size);
	memcpy(zone._data(), new_data, new_size);

	for (int32_t i = 0; i < item_count; i++)
		free(items[i]);
	free(items);
	free(item_lengths);
}


// injects a property into an item
uint8_t* build_add_property(uint32_t code, uint8_t* item, int32_t* item_size, PROPERTY* prop)
{
	int32_t offset, property_count = build_prop_count(item);

	int32_t* property_sizes = (int32_t*)try_malloc((property_count + 1) * sizeof(int32_t));
	uint8_t** properties = (uint8_t**)try_malloc((property_count + 1) * sizeof(uint8_t*));
	uint8_t** property_headers = (uint8_t**)try_malloc((property_count + 1) * sizeof(uint8_t*));

	for (int32_t i = 0; i < property_count + 1; i++)
	{
		property_headers[i] = (uint8_t*)try_malloc(8 * sizeof(uint8_t));
		for (int32_t j = 0; j < 8; j++)
			property_headers[i][j] = 0;
	}

	for (int32_t i = 0; i < property_count; i++)
	{
		if (from_u16(item + 0x10 + 8 * i) > code)
			memcpy(property_headers[i + 1], item + 0x10 + 8 * i, 8);
		else
			memcpy(property_headers[i], item + 0x10 + 8 * i, 8);
	}

	int32_t insertion_index = 0;
	for (int32_t i = 0; i < property_count + 1; i++)
		if (from_u32(property_headers[i]) == 0 && from_u32(property_headers[i] + 4) == 0)
			insertion_index = i;

	if (insertion_index != property_count)
	{
		for (int32_t i = 0; i < property_count + 1; i++)
		{
			property_sizes[i] = 0;
			if (i < insertion_index - 1)
				property_sizes[i] = from_u16(property_headers[i + 1] + 2) - from_u16(property_headers[i] + 2);
			if (i == insertion_index - 1)
				property_sizes[i] = from_u16(property_headers[i + 2] + 2) - from_u16(property_headers[i] + 2);
			// if (i == insertion_index) {}
			if (i > insertion_index)
			{
				if (i == property_count)
					property_sizes[i] = from_u16(item) - from_u16(property_headers[i] + 2);
				else
					property_sizes[i] = from_u16(property_headers[i + 1] + 2) - from_u16(property_headers[i] + 2);
			}
		}
	}
	else
		for (int32_t i = 0; i < property_count; i++)
		{
			if (i != property_count - 1)
				property_sizes[i] = from_u16(property_headers[i + 1] + 2) - from_u16(property_headers[i] + 2);
			else
				property_sizes[i] = from_u16(item) - OFFSET - from_u16(property_headers[i] + 2);
		}

	offset = 0x10 + 8 * property_count;
	for (int32_t i = 0; i < property_count + 1; i++)
	{
		if (i == insertion_index)
			continue;

		properties[i] = (uint8_t*)try_malloc(property_sizes[i]);
		memcpy(properties[i], item + offset, property_sizes[i]);
		offset += property_sizes[i];
	}

	property_sizes[insertion_index] = prop->length;
	memcpy(property_headers[insertion_index], prop->header, 8);
	properties[insertion_index] = prop->data;

	int32_t new_size = 0x10 + 8 * (property_count + 1);
	for (int32_t i = 0; i < property_count + 1; i++)
		new_size += property_sizes[i];
	if (insertion_index == property_count)
		new_size += OFFSET;

	uint8_t* new_item = (uint8_t*)try_malloc(new_size);
	*(int32_t*)(new_item) = new_size - OFFSET;

	*(int32_t*)(new_item + 0x4) = 0;
	*(int32_t*)(new_item + 0x8) = 0;
	*(int32_t*)(new_item + 0xC) = property_count + 1;

	offset = 0x10 + 8 * (property_count + 1);
	for (int32_t i = 0; i < property_count + 1; i++)
	{
		*(int16_t*)(property_headers[i] + 2) = offset - OFFSET;
		memcpy(new_item + 0x10 + 8 * i, property_headers[i], 8);
		memcpy(new_item + offset, properties[i], property_sizes[i]);
		offset += property_sizes[i];
	}

	*item_size = new_size - OFFSET;
	free(item);
	for (int32_t i = 0; i < property_count + 1; i++)
	{
		free(properties[i]);
		free(property_headers[i]);
	}
	free(properties);
	free(property_headers);
	free(property_sizes);
	return new_item;
}


// Removes the specified property.
uint8_t* build_rem_property(uint32_t code, uint8_t* item, int32_t* item_size, PROPERTY* prop)
{
	int32_t property_count = build_prop_count(item);

	int32_t* property_sizes = (int32_t*)try_malloc(property_count * sizeof(int32_t));
	uint8_t** properties = (uint8_t**)try_malloc(property_count * sizeof(uint8_t*));
	uint8_t** property_headers = (uint8_t**)try_malloc(property_count * sizeof(uint8_t*));

	bool found = false;
	for (int32_t i = 0; i < property_count; i++)
	{
		property_headers[i] = (uint8_t*)try_malloc(8 * sizeof(uint8_t*));
		memcpy(property_headers[i], item + 0x10 + 8 * i, 8);
		if (from_u16(property_headers[i]) == code)
			found = true;
	}

	if (!found)
		return item;

	for (int32_t i = 0; i < property_count - 1; i++)
		property_sizes[i] = from_u16(property_headers[i + 1] + 2) - from_u16(property_headers[i] + 2);
	property_sizes[property_count - 1] = from_u16(item) - OFFSET - from_u16(property_headers[property_count - 1] + 2);

	int32_t offset = 0x10 + 8 * property_count;
	for (int32_t i = 0; i < property_count; offset += property_sizes[i], i++)
	{
		properties[i] = (uint8_t*)try_malloc(property_sizes[i]);
		memcpy(properties[i], item + offset, property_sizes[i]);
	}

	int32_t new_size = 0x10 + 8 * (property_count - 1);
	for (int32_t i = 0; i < property_count; i++)
	{
		if (from_u16(property_headers[i]) == code)
			continue;
		new_size += property_sizes[i];
	}

	uint8_t* new_item = (uint8_t*)try_malloc(new_size);
	*(int32_t*)(new_item) = new_size;
	*(int32_t*)(new_item + 4) = 0;
	*(int32_t*)(new_item + 8) = 0;
	*(int32_t*)(new_item + 0xC) = property_count - 1;

	offset = 0x10 + 8 * (property_count - 1);
	int32_t indexer = 0;
	for (int32_t i = 0; i < property_count; i++)
	{
		if (from_u16(property_headers[i]) != code)
		{
			*(int16_t*)(property_headers[i] + 2) = offset - OFFSET;
			memcpy(new_item + 0x10 + 8 * indexer, property_headers[i], 8);
			memcpy(new_item + offset, properties[i], property_sizes[i]);
			indexer++;
			offset += property_sizes[i];
		}
	}

	*item_size = new_size;
	for (int32_t i = 0; i < property_count; i++)
	{
		free(properties[i]);
		free(property_headers[i]);
	}
	free(properties);
	free(property_headers);
	free(property_sizes);
	free(item);
	return new_item;
}

// removes nth item from an entry
void build_remove_nth_item(ENTRY& entry, int32_t n)
{
	int32_t item_count = build_item_count(entry._data());
	if (item_count < n)
	{
		printf("[Warning] Trying to remove item %d from entry with only %d items!\n", n, item_count);
		return;
	}

	int32_t i, offset;
	int32_t first_item_offset = 0x14 + 4 * item_count;

	int32_t* item_lengths = (int32_t*)try_malloc(item_count * sizeof(int32_t));
	uint8_t** items = (uint8_t**)try_malloc(item_count * sizeof(uint8_t**));

	for (int32_t i = 0; i < item_count; i++)
	{
		int32_t next_start = entry.get_nth_item_offset(i + 1);
		if (i == n)
			next_start = entry.esize;

		item_lengths[i] = next_start - entry.get_nth_item_offset(i);
	}

	for (offset = first_item_offset, i = 0; i < item_count;
		offset += item_lengths[i], i++)
	{
		items[i] = (uint8_t*)try_malloc(item_lengths[i]);
		memcpy(items[i], entry._data() + offset, item_lengths[i]);
	}

	item_lengths[n] = 0;
	first_item_offset -= 4;

	int32_t new_size = first_item_offset;
	for (int32_t i = 0; i < item_count; i++)
		new_size += item_lengths[i];

	uint8_t* new_data = (uint8_t*)try_malloc(new_size);
	*(int32_t*)(new_data) = MAGIC_ENTRY;
	*(int32_t*)(new_data + 0x4) = entry.eid;
	*(int32_t*)(new_data + 0x8) = *(int32_t*)(entry._data() + 8);
	*(int32_t*)(new_data + 0xC) = item_count - 1;

	for (offset = first_item_offset, i = 0; i < item_count + 1;
		offset += item_lengths[i], i++)
	{
		if (i == n)
			continue;

		int32_t curr = i;
		if (i > n)
			curr--;
		*(int32_t*)(new_data + 0x10 + curr * 4) = offset;
	}

	for (offset = first_item_offset, i = 0; i < item_count;
		offset += item_lengths[i], i++)
	{
		if (i == n)
			continue;
		memcpy(new_data + offset, items[i], item_lengths[i]);
	}

	entry.data.resize(new_size);
	memcpy(entry._data(), new_data, new_size);
	entry.esize = new_size;

	for (int32_t i = 0; i < item_count; i++)
		free(items[i]);
	free(items);
	free(item_lengths);
}

// Gets texture references from a model and adds them to the list.
void build_add_model_textures_to_list(uint8_t* model, LIST* list)
{
	int32_t item1off = from_u32(model + 0x10);
	int32_t tex_count = from_u32(model + item1off + 0x40);

	for (int32_t i = 0; i < tex_count; i++)
	{
		uint32_t scen_reference = from_u32(model + item1off + 0xC + 0x4 * i);
		if (scen_reference)
			list->add(scen_reference);
	}
}

// Gets texture references from a scenery entry and inserts them to the list.
void build_add_scen_textures_to_list(uint8_t* scenery, LIST* list)
{
	int32_t item1off = from_u32(scenery + 0x10);
	int32_t texture_count = from_u32(scenery + item1off + 0x28);
	for (int32_t i = 0; i < texture_count; i++)
	{
		uint32_t eid = from_u32(scenery + item1off + 0x2C + 4 * i);
		list->add(eid);
	}
}

// Returns path the entity has & its length.
int16_t* build_get_path(ELIST& elist, int32_t zone_index, int32_t item_index, int32_t* path_len)
{
	uint8_t* item = elist[zone_index].get_nth_item(item_index);
	int32_t offset = build_get_prop_offset(item, ENTITY_PROP_PATH);

	if (offset)
		*path_len = from_u32(item + offset);
	else
	{
		*path_len = 0;
		return NULL;
	}

	int16_t* coords = (int16_t*)try_malloc(3 * sizeof(int16_t) * *path_len);
	memcpy(coords, item + offset + 4, 6 * (*path_len));
	return coords;
}

// Gets zone's scenery reference count.
int32_t build_get_scen_count(uint8_t* entry)
{
	int32_t item1off = ENTRY::get_nth_item_offset(entry, 0);
	return entry[item1off];
}

// get scenery list from entry data
LIST build_get_sceneries(uint8_t* entry)
{
	int32_t scen_count = build_get_scen_count(entry);
	int32_t item1off = ENTRY::get_nth_item_offset(entry, 0);
	LIST list{};
	for (int32_t i = 0; i < scen_count; i++)
	{
		uint32_t scen = from_u32(entry + item1off + 0x4 + 0x30 * i);
		list.add(scen);
	}

	return list;
}
