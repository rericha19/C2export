#include "../include.h"
#include "../utils/entry.hpp"

// caps the angle to 0-360
int32_t normalize_angle(int32_t angle)
{
	while (angle < 0)
		angle += 360;
	while (angle >= 360)
		angle -= 360;

	return angle;
}

// converts the ingame yaw value range to regular range
int32_t c2yaw_to_deg(int32_t yaw)
{
	int32_t angle = (yaw * 360) / 4096;
	angle += 90;

	return normalize_angle(angle);
}

// converts 0-360 range angle to ingame angle value
int32_t deg_to_c2yaw(int32_t deg)
{
	int32_t angle = deg - 90;
	int32_t yaw = (angle * 4096) / 360;
	while (yaw < 0)
		yaw += 4096;
	yaw = yaw % 4096;
	return yaw;
}

// calculates angle distance between 2 angles
int32_t angle_distance(int32_t angle1, int32_t angle2)
{
	angle1 = normalize_angle(angle1);
	angle2 = normalize_angle(angle2);

	int32_t diff1 = normalize_angle(angle2 - angle1);
	int32_t diff2 = normalize_angle(angle1 - angle2);

	return min(diff1, diff2);
}

// averages 2 angles
int32_t average_angles(int32_t angle1, int32_t angle2)
{
	// Convert degrees to radians
	double a1 = angle1 * PI / 180.0;
	double a2 = angle2 * PI / 180.0;

	// Convert to Cartesian coordinates
	double x = cos(a1) + cos(a2);
	double y = sin(a1) + sin(a2);

	// Compute the averaged angle in degrees
	return (int32_t)round(atan2(y, x) * 180.0 / PI);
}

// gets full draw list for all points of a camera path according to config
void build_draw_list_util(ELIST& elist, std::vector<LIST>& full_draw, int32_t* config, int32_t curr_idx, int32_t neigh_idx, int32_t cam_idx, int32_t neigh_ref_idx, LIST* pos_overrides)
{
	int32_t entry_count = int32_t(elist.size());

	int32_t cam_len = 0;
	int32_t ent_len = 0;
	int32_t angles_len = 0;

	ENTRY& curr = elist[curr_idx];
	ENTRY& neighbour = elist[neigh_idx];
	LIST remember_nopath{};

	int32_t cam_mode = PROPERTY::get_value(curr.get_nth_main_cam(cam_idx), ENTITY_PROP_CAMERA_MODE);
	int16_t* path = build_get_path(elist, curr_idx, 2 + cam_idx * 3, &cam_len);
	int16_t* angles = build_get_path(elist, curr_idx, 2 + cam_idx * 3 + 1, &angles_len);

	int32_t neigh_ents = neighbour.entity_count();
	int32_t neigh_camitems = neighbour.cam_item_count();

	if (cam_len != angles_len / 2)
	{
		printf("[warning] Zone %s camera %d camera path and angles path length mismatch (%d %d)\n", curr.ename, cam_idx, cam_len, angles_len);
	}

	std::vector<int32_t> avg_angles{};
	avg_angles.reserve(cam_len);

	if (cam_mode == C2_CAM_MODE_3D || cam_mode == C2_CAM_MODE_CUTSCENE)
	{
		for (int32_t n = 0; n < angles_len; n += 2)
		{
			int32_t angle1 = c2yaw_to_deg(angles[3 * n + 1]);
			int32_t angle2 = c2yaw_to_deg(angles[3 * n + 4]);
			avg_angles[n / 2] = normalize_angle(average_angles(angle1, angle2));
		}
	}

	// for each point of the camera path
	for (int32_t m = 0; m < cam_len; m++)
	{
		int32_t cam_x = path[3 * m + 0] + from_u32(curr.get_nth_item(1) + 0);
		int32_t cam_y = path[3 * m + 1] + from_u32(curr.get_nth_item(1) + 4);
		int32_t cam_z = path[3 * m + 2] + from_u32(curr.get_nth_item(1) + 8);

		// check all neighbour entities
		for (int32_t n = 0; n < neigh_ents; n++)
		{
			uint8_t* entity = neighbour.get_nth_entity(n);
			int32_t ent_id = PROPERTY::get_value(entity, ENTITY_PROP_ID);
			int32_t ent_override_mult = PROPERTY::get_value(entity, ENTITY_PROP_OVERRIDE_DRAW_MULT);
			int32_t pos_override_id = PROPERTY::get_value(entity, ENTITY_PROP_OVERRIDE_DRAW_ID);

			int32_t type = PROPERTY::get_value(entity, ENTITY_PROP_TYPE);
			int32_t subt = PROPERTY::get_value(entity, ENTITY_PROP_SUBTYPE);

			if (ent_id == -1)
				continue;

			int32_t ref_ent_idx = n;
			if (pos_override_id != -1 && !(type == 4 && subt == 17))
			{
				pos_override_id = pos_override_id / 0x100;
				for (int32_t o = 0; o < neigh_ents; o++)
				{
					int32_t ent_id2 = PROPERTY::get_value(neighbour.get_nth_entity(o), ENTITY_PROP_ID);
					if (ent_id2 == pos_override_id)
					{
						ref_ent_idx = o;
						if (pos_overrides->find(ent_id) == -1)
						{
							printf("%d using position from another entity %d\n", ent_id, ent_id2);
							pos_overrides->add(ent_id);
						}
						break;
					}
				}
			}

			if (ent_override_mult == -1)
				ent_override_mult = 100;
			else
				ent_override_mult = ent_override_mult / 0x100;

			int32_t allowed_dist_x = ((ent_override_mult * config[CNFG_IDX_DRAW_LIST_GEN_CAP_X]) / 100);
			int32_t allowed_dist_y = ((ent_override_mult * config[CNFG_IDX_DRAW_LIST_GEN_CAP_Y]) / 100);
			int32_t allowed_dist_xz = ((ent_override_mult * config[CNFG_IDX_DRAW_LIST_GEN_CAP_XZ]) / 100);
			int32_t allowed_angledist = config[CNFG_IDX_DRAW_LIST_GEN_ANGLE_3D];

			int16_t* ent_path = build_get_path(elist, neigh_idx, 2 + neigh_camitems + ref_ent_idx, &ent_len);

			if (ent_len == 0)
			{
				int32_t id = PROPERTY::get_value(elist[neigh_idx].get_nth_entity(ref_ent_idx), ENTITY_PROP_ID);
				if (remember_nopath.find(id) == -1)
				{
					printf("[warning] entity %d in zone %s has no path\n", id, elist[neigh_idx].ename);
					remember_nopath.add(id);
				}
			}

			// check all entity points to see whether its visible
			for (int32_t o = 0; o < ent_len; o++)
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

					// simple frustum check
					// Zs swapped because of the z axis orientation in c2
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
					printf("[warning] Unknown camera mode %d in %s cam %d\n", cam_mode, curr.ename, cam_idx);
				}
			}
		}
	}
}

// main function for remaking draw lists according to config and zone data
// selects entities to draw, converts to delta form and replaces camera properties
void build_remake_draw_lists(ELIST& elist, int32_t* config)
{
	bool dbg_print = false;
	int32_t entry_count = int32_t(elist.size());
	LIST pos_overrides{};

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (elist[i].entry_type() != ENTRY_TYPE_ZONE)
			continue;

		int32_t cam_count = elist[i].cam_item_count() / 3;
		if (!cam_count)
			continue;

		uint32_t skipflag = from_u32(elist[i].get_nth_item(0) + C2_SPECIAL_METADATA_OFFSET) & SPECIAL_METADATA_MASK_SKIPFLAG;
		if (skipflag)
		{
			printf("\nSkipping draw list making for %s\n", elist[i].ename);
			continue;
		}

		printf("\nMaking draw lists for %s\n", elist[i].ename);

		for (int32_t j = 0; j < cam_count; j++)
		{
			int32_t cam_offset = elist[i].get_nth_item_offset(2 + 3 * j);

			int32_t cam_length = build_get_path_length(elist[i]._data() + cam_offset);
			printf("\tcam path %d (%d points)\n", j, cam_length);

			// initialize full non-delta draw list used to represent the draw list during its building
			std::vector<LIST> full_draw{};
			full_draw.resize(cam_length);

			int32_t neighbour_count = elist[i].neighbour_count();
			LIST visited_neighbours{};
			for (int32_t l = 0; l < neighbour_count; l++)
			{
				ENTRY& curr = elist[i];
				uint32_t neighbour_eid = from_u32(curr.get_nth_item(0) + C2_NEIGHBOURS_START + 4 + (4 * l));
				if (visited_neighbours.find(neighbour_eid) != -1)
				{
					printf("[warning] Duplicate neighbour %s, skipping\n", eid2str(neighbour_eid));
					continue;
				}

				visited_neighbours.add(neighbour_eid);
				int32_t idx = elist.get_index(neighbour_eid);
				if (idx == -1)
				{
					printf("[warning] Invalid neighbour %s\n", eid2str(neighbour_eid));
					continue;
				}
				build_draw_list_util(elist, full_draw, config, i, idx, j, l, &pos_overrides);
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
			std::vector<LIST> listA{};
			std::vector<LIST> listB{};
			listA.resize(cam_length);
			listB.resize(cam_length);

			// converts full draw list to delta based, then creates game-format draw list properties based on the delta lists
			// listA and listB args are switched around because draw lists are like that
			build_load_list_to_delta(full_draw, listB, listA, cam_length, elist, 1);
			PROPERTY prop_0x13B = build_make_load_list_prop(listA, cam_length, 0x13B);
			PROPERTY prop_0x13C = build_make_load_list_prop(listB, cam_length, 0x13C);

			if (dbg_print)
				printf("Converted full list to delta form and delta to props\n");

			// removes existing draw list properties, inserts newly made ones
			build_entity_alter(elist[i], 2 + 3 * j, build_rem_property, 0x13B, NULL);
			build_entity_alter(elist[i], 2 + 3 * j, build_rem_property, 0x13C, NULL);

			if (max_c && prop_0x13B.length && prop_0x13C.length)
			{
				build_entity_alter(elist[i], 2 + 3 * j, build_add_property, 0x13B, &prop_0x13B);
				build_entity_alter(elist[i], 2 + 3 * j, build_add_property, 0x13C, &prop_0x13C);
			}

			if (dbg_print)
				printf("Replaced draw list props\n");

			if (dbg_print)
				printf("Freed some stuff, end\n");
		}
	}
}
