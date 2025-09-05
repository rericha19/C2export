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
void ELIST::add_neighbour_coll_dependencies(std::vector<LIST>& full_list, ENTRY& ntry)
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
					sublist.copy_in(coll_dep.dependencies);
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
	bool dbg_print = false;
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
			build_load_list_to_delta(full_draw, listB, listA, cam_length, *this, true);
			PROPERTY prop_0x13B = PROPERTY::make_list_prop(listA, 0x13B);
			PROPERTY prop_0x13C = PROPERTY::make_list_prop(listB, 0x13C);

			if (dbg_print)
				printf("Converted full list to delta form and delta to props\n");

			// removes existing draw list properties, inserts newly made ones
			build_entity_alter(ntry, 2 + 3 * j, build_rem_property, 0x13B, NULL);
			build_entity_alter(ntry, 2 + 3 * j, build_rem_property, 0x13C, NULL);

			if (max_c && prop_0x13B.length && prop_0x13C.length)
			{
				build_entity_alter(ntry, 2 + 3 * j, build_add_property, 0x13b, &prop_0x13B);
				build_entity_alter(ntry, 2 + 3 * j, build_add_property, 0x13C, &prop_0x13C);
			}

			if (dbg_print)
				printf("Replaced draw list props, end\n");
		}
	}
}

