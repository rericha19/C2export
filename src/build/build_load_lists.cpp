#include "../include.h"
#include "../utils/utils.hpp"
#include "../utils/entry.hpp"
#include "../utils/elist.hpp"

// generating load lists during build

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
void build_remake_load_lists(ELIST& elist)
{
	bool dbg_print = false;

	// for each zone entry do load list (also for each zone's each camera)
	for (auto& ntry : elist)
	{
		if (ntry.get_entry_type() != EntryType::Zone)
			continue;

		int32_t cam_count = ntry.get_cam_item_count() / 3;
		if (!cam_count)
			continue;

		printf("Making load lists for %s\n", ntry.m_ename);

		LIST special_entries = ntry.get_special_entries_extended(elist);
		uint32_t music_ref = ntry.get_zone_track();

		for (int32_t j = 0; j < cam_count; j++)
		{
			printf("\t cam path %d\n", j);
			int32_t cam_length = ntry.get_ent_path_len(2 + 3 * j);

			// full non-delta load list used to represent the load list during building
			std::vector<LIST> full_load(cam_length);

			// add permaloaded entries
			for (int32_t k = 0; k < cam_length; k++)
				full_load[k].copy_in(elist.m_permaloaded);

			for (auto& mus_dep : elist.m_musi_deps)
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
				full_load[l].copy_in(ntry.m_related);

			if (dbg_print)
				printf("Copied in deprecate relatives\n");

			// all sounds
			// if (load_list_sound_entry_inc_flag == 0)
			for (auto& ntry2 : elist)
			{
				if (ntry2.get_entry_type() != EntryType::Sound)
					continue;

				for (int32_t l = 0; l < cam_length; l++)
					full_load[l].add(ntry2.m_eid);
			}

			if (dbg_print)
				printf("Copied in sounds\n");

			// add direct neighbours
			LIST neighbours = ntry.get_neighbours();
			for (int32_t k = 0; k < cam_length; k++)
				full_load[k].copy_in(neighbours);

			if (dbg_print)
				printf("Copied in neighbours\n");

			// for each scenery in current zone's scen reference list, add its
			// textures to the load list				
			LIST sceneries = ntry.get_sceneries();
			for (auto& scenery : sceneries)
			{
				int32_t scenery_index = elist.get_index(scenery);
				if (scenery_index == -1)
				{
					printf("[warning] invalid scenery %s in zone %s\n", eid2str(scenery), ntry.m_ename);
					continue;
				}

				for (int32_t l = 0; l < cam_length; l++)
					full_load[l].copy_in(elist[scenery_index].get_scenery_textures());
			}

			if (dbg_print)
				printf("Copied in some scenery stuff\n");

			// oh boy
			build_load_list_util(ntry, 2 + 3 * j, full_load, cam_length, elist);

			if (dbg_print)
				printf("Load list util ran\n");

			// checks whether the load list doesnt try to load more than 8 textures,
			ntry.texture_count_check(elist, full_load, j);

			if (dbg_print)
				printf("Texture chunk was checked\n");

			// creates and initialises delta representation of the load list
			std::vector<LIST> listA(cam_length);
			std::vector<LIST> listB(cam_length);

			// converts full load list to delta based, then creates game-format load
			// list properties based on the delta lists
			build_load_list_to_delta(full_load, listA, listB, cam_length, elist, false);
			PROPERTY prop_0x208 = PROPERTY::make_list_prop(listA, 0x208);
			PROPERTY prop_0x209 = PROPERTY::make_list_prop(listB, 0x209);

			if (dbg_print)
				printf("Converted full list to delta and delta to props\n");

			// removes existing load list properties, inserts newly made ones
			build_entity_alter(ntry, 2 + 3 * j, build_rem_property, 0x208, NULL);
			build_entity_alter(ntry, 2 + 3 * j, build_rem_property, 0x209, NULL);
			build_entity_alter(ntry, 2 + 3 * j, build_add_property, 0x208, &prop_0x208);
			build_entity_alter(ntry, 2 + 3 * j, build_add_property, 0x209, &prop_0x209);

			if (dbg_print)
				printf("Replaced load list props\n");

			if (dbg_print)
				printf("Freed some stuff, end\n");
		}

	}
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
void build_load_list_util(ENTRY& ntry, int32_t camera_index, std::vector<LIST>& full_list, int32_t cam_length, ELIST& elist)
{
	// neighbours, slsts, scenery
	LIST links = ntry.get_links(camera_index);
	for (auto& link: links)
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

				full_list[i].copy_in(info.dependencies);
			}
		}
	}
}



// Deals with slst and neighbour/scenery references of path linked entries.
void build_load_list_zone_refs(ENTRY& ntry, int32_t cam_index, int32_t link_int, std::vector<LIST>& full_list, int32_t cam_length, ELIST& elist)
{
	int32_t slst_distance = elist.m_config[LL_SLST_Distance];
	int32_t neig_distance = elist.m_config[LL_Neighbour_Distance];
	int32_t preloading_flag = elist.m_config[LL_Transition_Preloading_Type];

	auto distance_with_penalty = [&](int32_t distance)
		{
			double backwards_penalty = config_to_double(elist.m_config[LL_Backwards_Loading_Penalty_DBL]);
			return int32_t(distance * (1.0 - backwards_penalty));
		};

	int32_t distance = 0;

	CAMERA_LINK link(link_int);

	uint32_t neighbour_eid = from_u32(ntry.get_nth_item(0) + C2_NEIGHBOURS_START + 4 + 4 * link.zone_index);
	uint32_t neighbour_flg = from_u32(ntry.get_nth_item(0) + C2_NEIGHBOURS_START + 4 + 4 * link.zone_index + 0x20);

	int32_t neigh_idx = elist.get_index(neighbour_eid);
	if (neigh_idx == -1)
		return;

	// if preloading nothing
	if (preloading_flag == PRELOADING_NOTHING && (neighbour_flg == 0xF || neighbour_flg == 0x1F))
		return;

	if (preloading_flag == PRELOADING_TEXTURES_ONLY || preloading_flag == PRELOADING_ALL)
	{
		for (auto& scenery : elist[neigh_idx].get_sceneries())
		{
			int32_t scenery_index = elist.get_index(scenery);

			if (link.type == 1)
			{
				int32_t end_index = (cam_length - 1) / 2 - 1;
				for (int32_t j = 0; j < end_index; j++)
				{
					full_list[j].copy_in(elist[scenery_index].get_scenery_textures());
				}
			}
			else if (link.type == 2)
			{
				int32_t start_index = (cam_length - 1) / 2 + 1;
				for (int32_t j = start_index; j < cam_length; j++)
				{
					full_list[j].copy_in(elist[scenery_index].get_scenery_textures());
				}
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
		full_list[j].copy_in(elist[neigh_idx].m_related);

	int32_t neigh_cam_count = elist[neigh_idx].get_cam_item_count() / 3;
	if (link.cam_index >= neigh_cam_count)
	{
		printf("[warning] Zone %s is linked to zone %s's %d. camera path (indexing from 0) when it only has %d paths\n",
			ntry.m_ename, elist[neigh_idx].m_ename, link.cam_index, neigh_cam_count);
		return;
	}

	uint32_t slst = elist[neigh_idx].get_nth_slst(link.cam_index);
	for (int32_t i = 0; i < cam_length; i++)
		full_list[i].add(slst);

	elist.add_neighbour_coll_dependencies(full_list, ntry);
	elist.add_neighbour_coll_dependencies(full_list, elist[neigh_idx]);

	ENTITY_PATH coords = elist[neigh_idx].get_ent_path(2 + 3 * link.cam_index);
	distance += build_get_distance(coords.data(), 0, coords.length() - 1, -1, NULL);

	LIST layer2 = elist[neigh_idx].get_links(2 + 3 * link.cam_index);
	for (int32_t i = 0; i < layer2.count(); i++)
	{
		CAMERA_LINK link2(layer2[i]);

		uint32_t neighbour_eid2 = from_u32(elist[neigh_idx].get_nth_item(0) + C2_NEIGHBOURS_START + 4 + 4 * link2.zone_index);
		uint32_t neighbour_flg2 = from_u32(elist[neigh_idx].get_nth_item(0) + C2_NEIGHBOURS_START + 4 + 4 * link2.zone_index + 0x20);

		int32_t neigh_idx2 = elist.get_index(neighbour_eid2);
		if (neigh_idx2 == -1)
			continue;

		// not preloading everything
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
		int32_t slst_dist_w_orientation = is_before ? slst_distance : distance_with_penalty(slst_distance);
		int32_t neig_dist_w_orientation = is_before ? neig_distance : distance_with_penalty(neig_distance);

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
void build_load_list_zone_refs_back(int32_t cam_length, std::vector<LIST>& full_list, int32_t distance, int32_t final_distance, int16_t* coords, int32_t path_length, LIST additions)
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
void build_load_list_zone_refs_forw(int32_t cam_length, std::vector<LIST>& full_list,
	int32_t distance, int32_t final_distance, int16_t* coords, int32_t path_length, LIST additions)
{
	int32_t start_index = cam_length - 1;
	int32_t j;

	build_get_distance(coords, path_length - 1, 0, final_distance - distance, &start_index);
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

// reads draw lists of the camera of the zone, returns in a non-delta form.
std::vector<LIST> ENTRY::get_expanded_draw_list(int32_t cam_index)
{
	int32_t cam_length = get_ent_path_len(cam_index);

	std::vector<LIST> draw_list_full(cam_length);
	DRAW_LIST draw_list_delta = get_draw_lists(cam_index);

	LIST list = {};
	int32_t sublist_index = 0;
	for (size_t i = 0; i < size_t(cam_length) && sublist_index < draw_list_delta.size(); i++)
	{
		bool new_read = false;
		auto& sublist = draw_list_delta[sublist_index];
		if (sublist.index == i)
		{
			if (draw_list_delta[sublist_index].type == 'B')
			{
				for (auto& draw_uint : sublist.list)
				{
					DRAW_ITEM draw_item(draw_uint);
					list.add(draw_item.ID);
				}
			}

			if (draw_list_delta[sublist_index].type == 'A')
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
		draw_list_full[i].copy_in(list);

		// fixes case when both draw list A and B containsn sublist w/ the same index
		if (new_read)
			i--;
	}

	return draw_list_full;
}

// deconstructs, alters specified item using the func_arg function, reconstructs
void build_entity_alter(ENTRY& zone, int32_t item_index, uint8_t* (func_arg)(uint32_t, uint8_t*, int32_t*, PROPERTY*), int32_t property_code, PROPERTY* prop)
{
	int32_t i, offset;
	int32_t item_count = zone.get_item_count();
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
	*(int32_t*)(new_data + 0x4) = zone.m_eid;
	*(int32_t*)(new_data + 0x8) = int32_t(EntryType::Zone);
	*(int32_t*)(new_data + 0xC) = item_count;

	for (offset = first_item_offset, i = 0; i < item_count + 1;
		offset += item_lengths[i], i++)
		*(int32_t*)(new_data + 0x10 + i * 4) = offset;

	for (offset = first_item_offset, i = 0; i < item_count;
		offset += item_lengths[i], i++)
		memcpy(new_data + offset, items[i], item_lengths[i]);

	zone.m_esize = new_size;
	zone.m_data.resize(new_size);
	memcpy(zone._data(), new_data, new_size);

	for (int32_t i = 0; i < item_count; i++)
		free(items[i]);
	free(items);
	free(item_lengths);
	free(new_data);
}


// injects a property into an item
uint8_t* build_add_property(uint32_t code, uint8_t* item, int32_t* item_size, PROPERTY* prop)
{
	int32_t offset, property_count = PROPERTY::count(item);

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
	properties[insertion_index] = (uint8_t*)try_malloc(prop->length);
	memcpy(properties[insertion_index], prop->data.data(), prop->length);

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
	int32_t property_count = PROPERTY::count(item);

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
