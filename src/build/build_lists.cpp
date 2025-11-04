#include "../include.h"
#include "../utils/elist.hpp"
#include "../utils/entry.hpp"

void ELIST::print_draw_position_overrides()
{
	printf("\nDraw list gen position overrides:\n");

	for (auto& ntry : *this)
	{
		if (ntry.get_entry_type() != EntryType::Zone)
			continue;

		for (int32_t i = 0; i < ntry.get_entity_count(); i++)
		{
			uint8_t* entity = ntry.get_nth_entity(i);
			int32_t ent_id = PROPERTY::get_value(entity, ENTITY_PROP_ID);
			if (ent_id == -1)
				continue;

			int32_t draw_mult = ntry.get_nth_entity_draw_override_mult(i);
			if (draw_mult != 100)
				printf("          %s entity ID %3d uses distance multiplier %.2fx (%d)\n", ntry.m_ename, ent_id, double(draw_mult) / 100, draw_mult);

			auto [present, entity_idx] = ntry.get_nth_entity_draw_override_ent_idx(i);
			if (!present)
				continue;

			int32_t pos_override_id = PROPERTY::get_value(entity, ENTITY_PROP_OVERRIDE_DRAW_ID) >> 8;

			if (entity_idx != -1)
				printf("          %s entity ID %3d uses position from entity %3d\n", ntry.m_ename, ent_id, pos_override_id);
			else
				printf("[warning] %s entity ID %3d cannot use position override %3d\n", ntry.m_ename, ent_id, pos_override_id);
		}
	}
}


// Searches the entry and for each collision type it adds its dependencies to the lists 
void ELIST::add_neighbour_coll_dependencies(FULL_LABELED_LL& full_list, ENTRY& ntry)
{
	LIST neighbours = ntry.get_neighbours();
	for (int32_t x = 0; x < neighbours.count(); x++)
	{
		int32_t index = get_index(neighbours[x]);
		if (index == -1)
			continue;

		uint8_t* item1 = at(index).get_nth_item(1);
		int32_t item_len = at(index).get_nth_item_offset(2) - at(index).get_nth_item_offset(1);

		for (int32_t offset = 0x24; offset < item_len; offset += 2)
		{
			int16_t type = from_u16(item1 + offset);
			if (type % 2 == 0)
				continue;

			for (auto& coll_dep : m_coll_deps)
			{
				if (type != coll_dep.type)
					continue;

				for (auto& sublist : full_list)
					sublist.copy_in(coll_dep.dependencies, "collision_" + int_to_hex4(type));
			}
		}
	}
}

// check and add neighbouring entry's (single) entities to draw list of curr zone if within draw range
void ELIST::draw_list_gen_handle_neighbour(std::vector<LIST>& full_draw, ENTRY& curr, int32_t cam_idx, ENTRY& neighbour, int32_t neigh_ref_idx)
{
	LIST remember_nopath{};

	int32_t cam_mode = PROPERTY::get_value(curr.get_nth_main_cam(cam_idx), ENTITY_PROP_CAMERA_MODE);
	ENTITY_PATH campath = curr.get_ent_path(2 + cam_idx * 3);
	ENTITY_PATH angles = curr.get_ent_path(2 + cam_idx * 3 + 1);

	int32_t neigh_ents = neighbour.get_entity_count();
	int32_t neigh_camitems = neighbour.get_cam_item_count();

	if (campath.length() != angles.length() / 2)
	{
		printf("[warning] Zone %s camera %d camera path and angles path length mismatch (%d %d)\n", curr.m_ename, cam_idx, campath.length(), angles.length());
	}

	std::vector<int32_t> avg_angles(campath.length());
	if (cam_mode == C2_CAM_MODE_3D || cam_mode == C2_CAM_MODE_CUTSCENE)
	{
		for (int32_t n = 0; n < avg_angles.size() * 2; n += 2)
		{
			int32_t angle1 = c2yaw_to_deg(angles[3 * n + 1]);
			int32_t angle2 = c2yaw_to_deg(angles[3 * n + 4]);
			avg_angles[n / 2] = normalize_angle(average_angles(angle1, angle2));
		}
	}

	// for each point of the camera path
	for (int32_t m = 0; m < campath.length(); m++)
	{
		int32_t cam_x = campath[3 * m + 0] + from_u32(curr.get_nth_item(1) + 0);
		int32_t cam_y = campath[3 * m + 1] + from_u32(curr.get_nth_item(1) + 4);
		int32_t cam_z = campath[3 * m + 2] + from_u32(curr.get_nth_item(1) + 8);

		// check all neighbour entities
		for (int32_t n = 0; n < neigh_ents; n++)
		{
			uint8_t* entity = neighbour.get_nth_entity(n);
			int32_t ent_id = PROPERTY::get_value(entity, ENTITY_PROP_ID);

			if (ent_id == -1)
				continue;

			int32_t ref_ent_idx = n;
			auto [present, override_idx] = neighbour.get_nth_entity_draw_override_ent_idx(n);
			if (present && override_idx != -1)
				ref_ent_idx = override_idx;

			ENTITY_PATH ent_path = neighbour.get_ent_path(2 + neigh_camitems + ref_ent_idx);
			if (!ent_path.length())
			{
				int32_t id = PROPERTY::get_value(neighbour.get_nth_entity(ref_ent_idx), ENTITY_PROP_ID);
				if (remember_nopath.find(id) == -1)
				{
					printf("[warning] entity %d in zone %s has no path\n", id, neighbour.m_ename);
					remember_nopath.add(id);
				}
				continue;
			}

			int32_t ent_override_mult = neighbour.get_nth_entity_draw_override_mult(n);

			int32_t allowed_dist_x = ((ent_override_mult * m_config[DL_Distance_Cap_X]) / 100);
			int32_t allowed_dist_y = ((ent_override_mult * m_config[DL_Distance_Cap_Y]) / 100);
			int32_t allowed_dist_xz = ((ent_override_mult * m_config[DL_Distance_Cap_XZ]) / 100);
			int32_t allowed_angledist = m_config[DL_Distance_Cap_Angle3D];

			// check all entity points to see whether its visible
			for (int32_t o = 0; o < ent_path.length(); o++)
			{
				int32_t ent_x = (4 * ent_path[3 * o + 0]) + from_s32(neighbour.get_nth_item(1) + 0);
				int32_t ent_y = (4 * ent_path[3 * o + 1]) + from_s32(neighbour.get_nth_item(1) + 4);
				int32_t ent_z = (4 * ent_path[3 * o + 2]) + from_s32(neighbour.get_nth_item(1) + 8);

				int32_t dist_x = abs(cam_x - ent_x);
				int32_t dist_y = abs(cam_y - ent_y);
				int32_t dist_z = abs(cam_z - ent_z);
				int32_t dist_xz = sqrt(pow(dist_x, 2) + pow(dist_z, 2));

				if (allowed_dist_x && dist_x > allowed_dist_x)
					continue;
				if (allowed_dist_y && dist_y > allowed_dist_y)
					continue;
				if (allowed_dist_xz && dist_xz > allowed_dist_xz)
					continue;

				if (cam_mode == C2_CAM_MODE_2D || cam_mode == C2_CAM_MODE_VERTICAL)
				{
					full_draw[m].add(neigh_ref_idx | (ent_id << 8) | (n << 24));
					break;
				}
				else if (cam_mode == C2_CAM_MODE_3D || cam_mode == C2_CAM_MODE_CUTSCENE)
				{
					// in 3D, entities closer than 20% of the distance cap are drawn regardless of the check
					if (dist_xz < (allowed_dist_xz / 5) || !allowed_dist_xz)
					{
						full_draw[m].add(neigh_ref_idx | (ent_id << 8) | (n << 24));
						break;
					}

					// simple frustum check, Zs swapped because of the z axis orientation in c2
					double t = atan2(cam_z - ent_z, ent_x - cam_x);
					int32_t angle_to_ent = (int32_t)(t * (180.0 / PI));
					int32_t camera_angle = avg_angles[m];
					int32_t angle_dist = angle_distance(angle_to_ent, camera_angle);

					if (angle_dist < allowed_angledist && dist_xz < allowed_dist_xz)
					{
						full_draw[m].add(neigh_ref_idx | (ent_id << 8) | (n << 24));
						break;
					}
				}
				else
				{
					printf("[warning] Unknown camera mode %d in %s cam %d\n", cam_mode, curr.m_ename, cam_idx);
				}
			}
		}
	}
}

// search entities of neighbours and get types and subtypes from them
LIST ELIST::get_types_subtypes_from_ids(LIST& entity_ids, LIST& neighbours)
{
	LIST type_subtype_list{};
	for (auto& neigh : neighbours)
	{
		int32_t curr_index = get_index(neigh);
		if (curr_index == -1)
			continue;

		int32_t entity_count = at(curr_index).get_entity_count();
		for (int32_t j = 0; j < entity_count; j++)
		{
			uint8_t* entity = at(curr_index).get_nth_entity(j);
			int32_t ID = PROPERTY::get_value(entity, ENTITY_PROP_ID);
			if (entity_ids.find(ID) != -1)
			{
				int32_t type = PROPERTY::get_value(entity, ENTITY_PROP_TYPE);
				int32_t subtype = PROPERTY::get_value(entity, ENTITY_PROP_SUBTYPE);
				type_subtype_list.add((type << 16) | subtype);
			}
		}
	}

	return type_subtype_list;
}

// main function for remaking draw lists according to config and zone data
// selects entities to draw, converts to delta form and replaces camera properties
void ELIST::remake_draw_lists()
{
	LIST pos_overrides{};

	for (auto& ntry : *this)
	{
		if (ntry.get_entry_type() != EntryType::Zone)
			continue;

		int32_t cam_count = ntry.get_cam_item_count() / 3;
		if (!cam_count)
			continue;

		uint32_t skipflag = from_u32(ntry.get_nth_item(0) + C2_SPECIAL_METADATA_OFFSET) & SPECIAL_METADATA_MASK_SKIPFLAG;
		if (skipflag)
		{
			printf("\nSkipping draw list making for %s\n", ntry.m_ename);
			continue;
		}

		printf("\nMaking draw lists for %s\n", ntry.m_ename);

		for (int32_t j = 0; j < cam_count; j++)
		{
			int32_t cam_length = ntry.get_ent_path_len(2 + 3 * j);
			printf("\tcam path %d (%d points)\n", j, cam_length);

			// full non-delta draw list used to represent the draw list during its building
			std::vector<LIST> full_draw(cam_length);

			int32_t neighbour_count = ntry.get_neighbour_count();
			LIST visited_neighbours{};
			for (int32_t neigh_ref_idx = 0; neigh_ref_idx < neighbour_count; neigh_ref_idx++)
			{
				uint32_t neighbour_eid = from_u32(ntry.get_nth_item(0) + C2_NEIGHBOURS_START + 4 + (4 * neigh_ref_idx));
				if (visited_neighbours.find(neighbour_eid) != -1)
				{
					printf("[warning] Duplicate neighbour %s, skipping\n", eid2str(neighbour_eid));
					continue;
				}
				visited_neighbours.add(neighbour_eid);

				int32_t neigh_idx = get_index(neighbour_eid);
				if (neigh_idx == -1)
				{
					printf("[warning] Invalid neighbour %s\n", eid2str(neighbour_eid));
					continue;
				}
				draw_list_gen_handle_neighbour(full_draw, ntry, j, at(neigh_idx), neigh_ref_idx);
			}

			int32_t max_c = 0;
			int32_t max_p = 0;
			printf("\t");
			for (int32_t k = 0; k < cam_length; k++)
			{
				printf("%d,", full_draw[k].count());
				if (full_draw[k].count() > max_c)
				{
					max_c = full_draw[k].count();
					max_p = k;
				}
			}
			printf("\n\tMax count: %d (point %d)\n", max_c, max_p);

			// creates and initialises delta representation of the draw list
			std::vector<LIST> listA(cam_length);
			std::vector<LIST> listB(cam_length);

			// converts full draw list to delta based, then creates game-format draw list properties based on the delta lists
			// listA and listB args are switched around because draw lists are like that
			list_to_delta(full_draw, listB, listA, true);
			PROPERTY prop_0x13B = PROPERTY::make_list_prop(listA, 0x13B);
			PROPERTY prop_0x13C = PROPERTY::make_list_prop(listB, 0x13C);

			// removes existing draw list properties, inserts newly made ones
			ntry.entity_alter(2 + 3 * j, PROPERTY::item_rem_property, 0x13B, NULL);
			ntry.entity_alter(2 + 3 * j, PROPERTY::item_rem_property, 0x13C, NULL);

			if (max_c && prop_0x13B.length && prop_0x13C.length)
			{
				ntry.entity_alter(2 + 3 * j, PROPERTY::item_add_property, 0x13B, &prop_0x13B);
				ntry.entity_alter(2 + 3 * j, PROPERTY::item_add_property, 0x13C, &prop_0x13C);
			}
		}
	}
}

std::vector<LIST> ELIST::labeled_ll_to_unlabeled(FULL_LABELED_LL& labeled_ll)
{
	std::vector<LIST> unlabeled_ll(labeled_ll.size());
	for (int32_t i = 0; i < labeled_ll.size(); i++)
	{
		for (int32_t j = 0; j < labeled_ll[i].count(); j++)
			unlabeled_ll[i].add(labeled_ll[i][j].first);
	}
	return unlabeled_ll;
}

//  Converts the full load list into delta form.
void ELIST::list_to_delta(std::vector<LIST>& full_load, std::vector<LIST>& listA, std::vector<LIST>& listB, bool is_draw)
{
	int32_t cam_length = int32_t(full_load.size());
	// full item, listA point 0
	for (int32_t i = 0; i < full_load[0].count(); i++)
		if (is_draw || get_index(full_load[0][i]) != -1)
			listA[0].add(full_load[0][i]);

	// full item, listB point n-1
	int32_t n = cam_length - 1;
	for (int32_t i = 0; i < full_load[n].count(); i++)
		if (is_draw || get_index(full_load[n][i]) != -1)
			listB[n].add(full_load[n][i]);

	// creates delta items
	// for each point it takes full load list and for each entry checks whether it
	// has just become loaded/deloaded
	for (int32_t i = 1; i < cam_length; i++)
	{
		for (int32_t j = 0; j < full_load[i].count(); j++)
		{
			uint32_t curr_eid = full_load[i][j];
			if (!is_draw && get_index(curr_eid) == -1)
				continue;

			// is loaded on i-th point but not on i-1th point -> just became loaded,
			// add to listA[i]
			if (full_load[i - 1].find(curr_eid) == -1)
				listA[i].add(curr_eid);
		}

		for (int32_t j = 0; j < full_load[i - 1].count(); j++)
		{
			uint32_t curr_eid = full_load[i - 1][j];
			if (!is_draw && get_index(curr_eid) == -1)
				continue;

			// is loaded on i-1th point but not on i-th point -> no longer loaded, add
			// to listB[i - 1]
			if (full_load[i].find(curr_eid) == -1)
				listB[i - 1].add(curr_eid);
		}
	}

	// gets rid of cases when an item is in both listA and listB on the same index
	// (getting loaded and instantly deloaded) if its in both it removes it
	for (int32_t i = 0; i < cam_length; i++)
	{
		LIST iter_copy{};
		iter_copy.copy_in(listA[i]);

		for (int32_t j = 0; j < iter_copy.count(); j++)
		{
			if (listB[i].find(iter_copy[j]) == -1)
				continue;

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
A function that for each zone's each camera path creates new load lists
using the provided list of permanently loaded entries and the provided list
of dependencies of certain entitites, gets models from the animations. For
the zone itself and its linked neighbours it adds all relatives and their
dependencies. Loads only one sound per sound chunk (sound chunks should
already be built by this point). Load list properties get removed and then
replaced by the provided list using 'camera_alter' function. Load lists get
sorted later during the nsd writing process.
*/
void ELIST::remake_load_lists()
{
	bool dbg_print = false;
	m_load_list_logs = nlohmann::json::object();
	m_load_list_logs["zone_info"] = nlohmann::json::object();

	nlohmann::json perma = nlohmann::json::array();
	for (uint32_t eid : m_permaloaded)
		perma.push_back(eid2str(eid));

	nlohmann::json sounds = nlohmann::json::array();
	for (auto& entry : *this)
	{
		if (entry.get_entry_type() == EntryType::Sound)
			sounds.push_back(entry.m_ename);
	}

	m_load_list_logs["full_perma[" + std::to_string(perma.size()) + "]"] = perma;
	m_load_list_logs["sounds[" + std::to_string(sounds.size()) + "]"] = sounds;

	// for each zone entry do load list (also for each zone's each camera)
	for (auto& ntry : *this)
	{
		if (ntry.get_entry_type() != EntryType::Zone)
			continue;

		int32_t cam_count = ntry.get_cam_item_count() / 3;
		if (!cam_count)
			continue;

		printf("Making load lists for %s\n", ntry.m_ename);

		LIST special_entries = ntry.get_special_entries_extended(*this);
		uint32_t music_ref = ntry.get_zone_track();

		for (int32_t j = 0; j < cam_count; j++)
		{
			printf("\t cam path %d\n", j);
			int32_t cam_length = ntry.get_ent_path_len(2 + 3 * j);

			// full non-delta load list used to represent the load list during building
			FULL_LABELED_LL full_load(cam_length);

			// add permaloaded entries
			for (int32_t k = 0; k < cam_length; k++)
				full_load[k].copy_in(m_permaloaded, "perma");

			for (auto& mus_dep : m_musi_deps)
			{
				if (mus_dep.type == music_ref)
				{
					for (int32_t k = 0; k < cam_length; k++)
						full_load[k].copy_in(mus_dep.dependencies, "mus_" + ENTRY::eid2s(music_ref));

					if (dbg_print)
					{
						printf("\t\tcopied in %d music deps\n", mus_dep.dependencies.count());
						for (auto& d : mus_dep.dependencies)
							printf("\t\t\t %s\n", eid2str(d));
					}
				}
			}

			if (dbg_print) printf("Copied in permaloaded and music refs\n");

			// add current zone's special entries
			for (int32_t k = 0; k < cam_length; k++)
				full_load[k].copy_in(special_entries, "special_ll");

			if (dbg_print) printf("Copied in special %d\n", special_entries.count());

			// add relatives (directly related entries like slsts, neighbours, scenery
			// might not be necessary								
			for (int32_t l = 0; l < cam_length; l++)
				full_load[l].copy_in(ntry.m_related, "dir_rel");

			if (dbg_print) printf("Copied in deprecate relatives\n");

			// all sounds
			// if (load_list_sound_entry_inc_flag == 0)
			for (auto& ntry2 : *this)
			{
				if (ntry2.get_entry_type() != EntryType::Sound)
					continue;

				for (int32_t l = 0; l < cam_length; l++)
					full_load[l].add(ntry2.m_eid, "sound");
			}

			if (dbg_print) printf("Copied in sounds\n");

			// for each scenery in current zone's scen reference list, add its
			// textures to the load list				
			LIST sceneries = ntry.get_sceneries();
			for (auto& scenery : sceneries)
			{
				int32_t scen_index = get_index(scenery);
				if (scen_index == -1)
				{
					printf("[warning] invalid scenery %s in zone %s\n", eid2str(scenery), ntry.m_ename);
					continue;
				}

				for (int32_t l = 0; l < cam_length; l++)
					full_load[l].copy_in(at(scen_index).get_scenery_textures(), std::string("tex_scen_") + at(scen_index).m_ename);
			}

			if (dbg_print) printf("Copied in some scenery stuff\n");

			// oh boy
			build_load_list_util(ntry, 2 + 3 * j, full_load, cam_length, *this);

			if (dbg_print) printf("Load list util ran\n");

			std::string prev_dump{};
			for (int32_t u = 0; u < full_load.size(); u++)
			{
				char key_buf[200] = "";
				int32_t interesting_entries_count = int32_t(full_load[u].size() - perma.size() - sounds.size());
				sprintf(key_buf, "%5s-%d-%03d/%03d[%d|%d]", ntry.m_ename, j, u, int32_t(full_load.size()), 
					interesting_entries_count, int32_t(full_load[u].size()));

				nlohmann::json load_info = nlohmann::json::object();
				for (auto& loaded : full_load[u])
				{
					if (loaded.second == "sound" || loaded.second.find("perma") != std::string::npos)
						continue;

					load_info[eid2str(loaded.first)] = loaded.second;
				}

				auto curr_dump = load_info.dump();
				if (prev_dump != curr_dump)
					m_load_list_logs["zone_info"][key_buf] = load_info;
				prev_dump = curr_dump;
			}

			auto unlabeled_full_load = labeled_ll_to_unlabeled(full_load);

			// checks whether the load list doesnt try to load more than 8 textures,
			ntry.texture_count_check(*this, unlabeled_full_load, j);

			if (dbg_print) printf("Texture chunk was checked\n");

			// creates and initialises delta representation of the load list
			std::vector<LIST> listA(cam_length);
			std::vector<LIST> listB(cam_length);

			// converts full load list to delta based, then creates game-format load
			// list properties based on the delta lists
			list_to_delta(unlabeled_full_load, listA, listB, false);
			PROPERTY prop_0x208 = PROPERTY::make_list_prop(listA, 0x208);
			PROPERTY prop_0x209 = PROPERTY::make_list_prop(listB, 0x209);

			if (dbg_print) printf("Converted full list to delta and delta to props\n");

			// removes existing load list properties, inserts newly made ones
			ntry.entity_alter(2 + 3 * j, PROPERTY::item_rem_property, 0x208, NULL);
			ntry.entity_alter(2 + 3 * j, PROPERTY::item_rem_property, 0x209, NULL);
			ntry.entity_alter(2 + 3 * j, PROPERTY::item_add_property, 0x208, &prop_0x208);
			ntry.entity_alter(2 + 3 * j, PROPERTY::item_add_property, 0x209, &prop_0x209);

			if (dbg_print) printf("Replaced load list props\n");
		}
	}
}
