#include "../include.h"
#include "../side_scripts/export.hpp"
#include "../utils/entry.hpp"
#include "../utils/elist.hpp"

namespace level_alter
{
	// removes draw list properties from cameras of all zone entries
	void wipe_draw_lists(ELIST& elist)
	{
		for (auto& ntry : elist)
		{
			if (ntry.get_entry_type() != EntryType::Zone)
				continue;

			int32_t cam_count = ntry.get_cam_item_count() / 3;
			for (int32_t j = 0; j < cam_count; j++)
			{
				build_entity_alter(ntry, 2 + 3 * j, build_rem_property, ENTITY_PROP_CAM_DRAW_LIST_A, NULL);
				build_entity_alter(ntry, 2 + 3 * j, build_rem_property, ENTITY_PROP_CAM_DRAW_LIST_B, NULL);
			}
		}
	}

	// removes non-camera entities from all zone entries
	void wipe_entities(ELIST& elist)
	{
		for (auto& ntry : elist)
		{
			if (ntry.get_entry_type() != EntryType::Zone)
				continue;

			int32_t cam_item_count = ntry.get_cam_item_count();
			int32_t entity_count = ntry.get_entity_count();

			for (int32_t j = entity_count - 1; j >= 0; j--)
			{
				ntry.remove_nth_item(2 + cam_item_count + j);
			}

			uint8_t* item0 = ntry.get_nth_item(0);
			*(int32_t*)(item0 + C2_ENTITY_COUNT_OFF) = 0;
		}
	}

	// converts old draw override props to proper new ones
	// (used to reuse box count properties)
	void convert_old_dl_override(ELIST& elist)
	{
		for (auto& ntry : elist)
		{
			if (ntry.get_entry_type() != EntryType::Zone)
				continue;

			int32_t entity_count = ntry.get_entity_count();
			int32_t cam_item_count = ntry.get_cam_item_count();

			for (int32_t j = 0; j < entity_count; j++)
			{
				int32_t entity_n = 2 + cam_item_count + j;
				uint8_t* nth_entity = ntry.get_nth_entity(j);
				int32_t id = PROPERTY::get_value(nth_entity, ENTITY_PROP_ID);
				int32_t type = PROPERTY::get_value(nth_entity, ENTITY_PROP_TYPE);
				int32_t subt = PROPERTY::get_value(nth_entity, ENTITY_PROP_SUBTYPE);
				// skip box counter object
				if (type == 4 && subt == 17)
					continue;

				int32_t old_override_id = PROPERTY::get_value(nth_entity, ENTITY_PROP_OVERRIDE_DRAW_ID_OLD);
				int32_t old_override_mult = PROPERTY::get_value(nth_entity, ENTITY_PROP_OVERRIDE_DRAW_MULT_OLD);
				PROPERTY prop_override_id = PROPERTY::get_full(nth_entity, ENTITY_PROP_OVERRIDE_DRAW_ID_OLD);
				PROPERTY prop_override_mult = PROPERTY::get_full(nth_entity, ENTITY_PROP_OVERRIDE_DRAW_MULT_OLD);

				printf("zone %s -id %d: -ts %d-%d\t\tid %d mult %d\n", eid2str(ntry.m_eid), id, type, subt, (int16_t)(old_override_id >> 8), (int16_t)(old_override_mult >> 8));

				if (old_override_id != -1)
				{
					build_entity_alter(ntry, entity_n, build_rem_property, ENTITY_PROP_OVERRIDE_DRAW_ID_OLD, NULL);
					*(int16_t*)(prop_override_id.header) = ENTITY_PROP_OVERRIDE_DRAW_ID;
					build_entity_alter(ntry, entity_n, build_add_property, ENTITY_PROP_OVERRIDE_DRAW_ID, &prop_override_id);
				}
				if (old_override_mult != -1)
				{
					build_entity_alter(ntry, entity_n, build_rem_property, ENTITY_PROP_OVERRIDE_DRAW_MULT_OLD, NULL);
					*(int16_t*)(prop_override_mult.header) = ENTITY_PROP_OVERRIDE_DRAW_MULT;
					build_entity_alter(ntry, entity_n, build_add_property, ENTITY_PROP_OVERRIDE_DRAW_MULT, &prop_override_mult);
				}
			}
		}
	}

	// util function for swapping collision nodes
	void swap_node(uint16_t* a, uint16_t* b)
	{
		uint16_t temp = *a;
		*a = *b;
		*b = temp;
	}

	struct CollisionNodeInfoSingle
	{
		uint16_t offset_value{};
		uint16_t children_count{};
		LIST found_at{};

		// debug print for node info struct
		void dbg_print_info()
		{
			auto node = this;
			printf("  NodeInfo: offset_value=0x%04X, children_count=%d, found_at.count=%d, found_at.eids=[",
				node->offset_value, node->children_count, node->found_at.count());
			for (int32_t j = 0; j < node->found_at.count(); j++)
			{
				printf("%s0x%04X", (j > 0) ? ", " : "", node->found_at[j]);
			}
			printf("]\n");
		}
	};

	struct CollisionNodeInfo
	{
		CollisionNodeInfoSingle nodes[3000];
		int32_t count = 0;
	};

	// recursively retrieves list of collision nodes found in a collision item
	void collflip_list_nodes(uint8_t* item, int32_t offset, int32_t depth, int32_t maxx, int32_t maxy, int32_t maxz, CollisionNodeInfo* node_info, int32_t parent_offset)
	{
		if (offset < 0x1C)
		{
			printf("!!!!!!!!!!!!! unexpected offset value %4x (parent %4x)\n", offset, parent_offset);
			return;
		}
		uint16_t* children = (uint16_t*)(item + offset);

		int32_t num_children = 1;
		if (depth)
		{
			if (depth <= maxx)
				num_children <<= 1;
			if (depth <= maxy)
				num_children <<= 1;
			if (depth <= maxz)
				num_children <<= 1;
		}

		bool is_new = true;
		for (int32_t i = 0; i < node_info->count; i++)
		{
			if (node_info->nodes[i].offset_value == offset && node_info->nodes[i].children_count == num_children)
			{
				node_info->nodes[i].found_at.add(parent_offset);
				is_new = false;
				break;
			}
		}
		if (is_new)
		{
			int32_t add_idx = node_info->count;
			node_info->nodes[add_idx].offset_value = offset;
			node_info->nodes[add_idx].children_count = num_children;
			node_info->nodes[add_idx].found_at.add(parent_offset);
			node_info->count++;
		}

		for (int32_t i = 0; i < num_children; i++)
		{
			bool is_leaf = children[i] == 0 || (children[i] & 1);

			if (!is_leaf)
			{
				if (children[i] < 0x1C)
				{
					printf("!!!!!!!!!!!!! child unexpected offset %4x (offset %x)\n", children[i], offset);
					continue;
				}
				collflip_list_nodes(item, children[i], depth + 1, maxx, maxy, maxz, node_info, offset + 2 * i);
			}
		}
	}

	// function for getting rid of problematic data overlaps in collision items
	// required for collision flipping, as overlapping data flip would otherwise get scrambled
	bool collfip_unwrap_overlaps(uint8_t* item, CollisionNodeInfo* node_info, int32_t* new_size)
	{
		for (int32_t i = 0; i < node_info->count; i++)
		{
			CollisionNodeInfoSingle nodeA = node_info->nodes[i];
			uint16_t offsetA = nodeA.offset_value;
			uint16_t childrenA = nodeA.children_count;
			uint16_t endA = offsetA + 2 * childrenA;

			for (int32_t j = 0; j < i; j++)
			{
				CollisionNodeInfoSingle nodeB = node_info->nodes[j];
				uint32_t offsetB = nodeB.offset_value;
				uint16_t childrenB = nodeB.children_count;
				uint16_t endB = offsetB + 2 * childrenB;

				// exact reuse of the same node, allowed
				if (offsetA == offsetB && childrenA == childrenB)
					continue;

				int32_t new_offset = *new_size;
				int32_t case1 = (offsetA < offsetB && endA > offsetB);
				int32_t case2 = (offsetB < offsetA && endB > offsetA);
				int32_t case3 = (offsetA != offsetB && endA == endB);

				// if A is a subset of B its ok i think?
				int32_t case4 = (offsetA == offsetB && childrenA != childrenB);
				bool a_in_b = LIST::is_subset(nodeA.found_at, nodeB.found_at);
				bool b_in_a = LIST::is_subset(nodeB.found_at, nodeA.found_at);
				if (a_in_b || b_in_a)
					case4 = 0;

				// if overlapping, move node B to the end of the item, update refs
				if (case1 || case2 || case3 || case4)
				{
					// printf("swapping case %d%d%d%d\n", case1, case2, case3, case4);
					//nodeA.dbg_print_info();
					//nodeB.dbg_print_info();					
					// printf("\n");

					memcpy(item + new_offset, item + offsetB, 2 * childrenB);
					if (case4)
					{
						for (int32_t k = 0; k < nodeB.found_at.count(); k++)
						{
							*(uint16_t*)(item + nodeB.found_at[k]) = new_offset;
						}
					}
					else
					{
						for (int32_t k = 0x24; k < new_offset; k += 2)
						{
							if (*(uint16_t*)(item + k) == offsetB)
								*(uint16_t*)(item + k) = new_offset;
						}
					}
					*new_size += 2 * childrenB;
					return true;
				}
			}
		}

		return false;
	}

	// expands supplied collision item into a form that doesnt have data/node overlaps
	uint8_t* coll_get_expanded(uint8_t* item1, int32_t* new_size, int32_t maxx, int32_t maxy, int32_t maxz)
	{
		bool found_overlap = true;
		uint8_t* item1_edited = (uint8_t*)try_calloc(CHUNKSIZE, 1);
		memcpy(item1_edited, item1, *new_size);
		while (found_overlap)
		{
			// maybe could be done nicer (not in a while loop) but good enough
			CollisionNodeInfo node_info;
			node_info.count = 0;
			collflip_list_nodes(item1_edited, 0x1C, 0, maxx, maxy, maxz, &node_info, 0);
			found_overlap = collfip_unwrap_overlaps(item1_edited, &node_info, new_size);
		}
		return item1_edited;
	}

	// flips/mirrors the (expanded) collision item either on x or y
	void collision_flip(uint8_t* item, int32_t offset, int32_t depth, int32_t maxx, int32_t maxy, int32_t maxz, LIST* flipped, bool is_x)
	{
		if (offset < 0x1C)
		{
			printf("!!!!!!!!!!!!! collision_flip x:%d || offset %4x\n", is_x, offset);
			return;
		}

		if (flipped->find(offset) != -1)
			return;

		flipped->add(offset);
		uint16_t* children = (uint16_t*)(item + offset);
		int32_t num_children = 1;
		if (depth)
		{
			if (depth <= maxx)
				num_children <<= 1;
			if (depth <= maxy)
				num_children <<= 1;
			if (depth <= maxz)
				num_children <<= 1;
		}

		int32_t M[] = { maxx, maxy, maxz };
		qsort(M, 3, sizeof(int32_t), cmp_func_int);
		int32_t min = M[0];
		// int32_t min2 = M[1];
		int32_t max = M[2];

		if (is_x)
		{
			if (num_children == 8)
			{
				// good
				swap_node(&children[0], &children[4]);
				swap_node(&children[1], &children[5]);
				swap_node(&children[2], &children[6]);
				swap_node(&children[3], &children[7]);
			}
			else if (num_children == 4 && min != maxx)
			{
				// e.g 665/656 good?
				if (maxx == maxy || maxx == maxz)
				{
					swap_node(&children[0], &children[2]);
					swap_node(&children[1], &children[3]);
				}
			}
			else if (num_children == 2 && max == maxx)
			{
				// eg 655
				swap_node(&children[0], &children[1]);
			}
		}
		else
		{
			if (num_children == 8)
			{
				// e.g 555
				swap_node(&children[0], &children[2]);
				swap_node(&children[1], &children[3]);
				swap_node(&children[4], &children[6]);
				swap_node(&children[5], &children[7]);
			}
			else if (num_children == 4 && min != maxy)
			{
				// e.g 566/665
				swap_node(&children[0], &children[2]);
				swap_node(&children[1], &children[3]);
			}
			else if (num_children == 2 && max == maxy)
			{
				// 565
				swap_node(&children[0], &children[1]);
			}
		}

		for (int32_t i = 0; i < num_children; i++)
		{
			bool is_leaf = children[i] == 0 || (children[i] & 1);
			if (!is_leaf)
				collision_flip(item, children[i], depth + 1, maxx, maxy, maxz, flipped, is_x);
		}
	}

	// for flipping entity/camera path when doing flip_y
	void flip_entity_y(uint8_t* item, int32_t offset_y)
	{
		int32_t offset = PROPERTY::get_offset(item, ENTITY_PROP_PATH);
		if (!offset)
			return;

		int32_t length = from_u32(item + offset);
		for (int32_t k = 0; k < length; k++)
		{
			*(int16_t*)(item + offset + 4 + 6 * k + 2) *= -1;
			*(int16_t*)(item + offset + 4 + 6 * k + 2) += offset_y;
		}
	}

	// level alter utility - level flip y
	void flip_level_y(ELIST& elist, int32_t* chunk_count)
	{
		for (auto& ntry : elist)
		{
			if (ntry.get_entry_type() == EntryType::Scenery)
			{
				// vertices
				uint8_t* item0 = ntry.get_nth_item(0);
				uint8_t* item1 = ntry.get_nth_item(1);

				int32_t vert_count = from_u32(item0 + 0x10);
				for (int32_t j = 0; j < vert_count; j++)
				{
					uint8_t* vert = item1 + j * 4;
					int16_t y = from_s16(vert + 2) & 0xFFF0;
					int16_t rest = from_s16(vert + 2) & 0xF;
					y = -y;
					y -= 0x10;
					*(int16_t*)(vert + 2) = y | rest;
				}

				// position
				*(int32_t*)(item0 + 0x4) *= -1;
			}

			if (ntry.get_entry_type() == EntryType::Zone)
			{
				uint8_t* item1 = ntry.get_nth_item(1);
				int32_t size =
					ntry.get_nth_item_offset(2) - ntry.get_nth_item_offset(1);

				// zone position
				*(int32_t*)(item1 + 0x4) *= -1;
				*(int32_t*)(item1 + 0x4) -= from_s32(item1 + 0x10);

				// flip collision
				int32_t maxx = from_u16(item1 + 0x1E);
				int32_t maxy = from_u16(item1 + 0x20);
				int32_t maxz = from_u16(item1 + 0x22);
				printf("y flipping coll in zone %s, %d %d %d\n", eid2str(ntry.m_eid), maxx, maxy, maxz);
				uint8_t* item1_edited = coll_get_expanded(item1, &size, maxx, maxy, maxz);
				LIST flipped{};
				collision_flip(item1_edited, 0x1C, 0, maxx, maxy, maxz, &flipped, 0);
				ntry.replace_nth_item(1, item1_edited, size);

				// object positions
				int32_t cam_item_count = ntry.get_cam_item_count();
				int32_t entity_count = ntry.get_entity_count();

				for (int32_t j = 0; j < cam_item_count / 3; j++)
				{
					uint8_t* item = ntry.get_nth_main_cam(j);
					flip_entity_y(item, from_s32(item1 + 0x10));
				}

				for (int32_t j = 0; j < entity_count; j++)
				{
					uint8_t* item = ntry.get_nth_entity(j);
					flip_entity_y(item, from_s32(item1 + 0x10) / 4);
				}

				ntry.m_chunk = *chunk_count;
				*chunk_count += 1;
			}
		}
	}

	// for flipping entity/camera path when doing flip_x
	void flip_entity_x(uint8_t* item, int32_t offset_x)
	{
		int32_t id = PROPERTY::get_value(item, ENTITY_PROP_ID);
		int32_t type = PROPERTY::get_value(item, ENTITY_PROP_TYPE);
		int32_t subtype = PROPERTY::get_value(item, ENTITY_PROP_SUBTYPE);
		int32_t offset = PROPERTY::get_offset(item, ENTITY_PROP_PATH);

		if (!offset)
			return;

		// fish dont flip
		if (type == 47 && subtype == 5)
		{
			printf("\t fish 47-5 (id %3d), skipping\n", id);
			return;
		}

		int32_t length = from_u32(item + offset);
		for (int32_t k = 0; k < length; k++)
		{
			*(int16_t*)(item + offset + 4 + 6 * k + 0) *= -1;
			*(int16_t*)(item + offset + 4 + 6 * k + 0) += offset_x;
		}
	}

	// for flipping camera
	void flip_camera_angle_x(uint8_t* item2)
	{
		int32_t offset = PROPERTY::get_offset(item2, ENTITY_PROP_PATH);
		if (!offset)
			return;

		int32_t length = from_u32(item2 + offset);
		for (int32_t k = 0; k < length; k++)
		{
			int32_t angle = *(int16_t*)(item2 + offset + 4 + 6 * k + 2);
			angle = c2yaw_to_deg(-angle);
			angle = normalize_angle(angle);
			angle = deg_to_c2yaw(angle);
			*(int16_t*)(item2 + offset + 4 + 6 * k + 2) = angle;
		}
	}

	// fixes elevator destinations for elevators when doing level flip x
	void flip_fix_misc_entity_props(uint8_t* item)
	{
		int32_t id = PROPERTY::get_value(item, ENTITY_PROP_ID);
		int32_t type = PROPERTY::get_value(item, ENTITY_PROP_TYPE);
		int32_t subtype = PROPERTY::get_value(item, ENTITY_PROP_SUBTYPE);
		int32_t offset = PROPERTY::get_offset(item, ENTITY_PROP_ARGUMENTS);

		if (!offset)
			return;

		int32_t count = from_u32(item + offset);
		// basic elev, then bonus return elev
		if ((count == 8 && type == 9 && subtype == 6) ||
			(count == 8 && type == 9 && subtype == 31))
		{
			printf("\tpatched elevator entity %3d (type %2d subtype %2d)\n", id, type, subtype);
			*(int32_t*)(item + offset + 6 * 4) *= -1;
		}

		if (count == 4 && type == 32 && subtype == 4)
		{
			printf("\tpatched ice slide entity %3d (type %2d subtype %2d)\n", id, type, subtype);
			*(int32_t*)(item + offset + 4) *= -1;
		}
	}

	// fixes warp in coordinates property when doing level flip x
	void flip_fix_prop_198_x(uint8_t* item)
	{
		int32_t offset = PROPERTY::get_offset(item, ENTITY_PROP_WARPIN_COORDS);
		if (!offset)
			return;

		if (from_s32(item + offset) == 5)
			*(int32_t*)(item + offset + 0xC) *= -1;
	}

	// fixes sidescrolling camera distances, if possible
	void flip_camera_dist_2D_x(uint8_t* item0, uint8_t* item1)
	{
		int32_t mode = PROPERTY::get_value(item0, ENTITY_PROP_CAMERA_MODE);
		if (mode != C2_CAM_MODE_2D)
			return;

		PROPERTY prop = PROPERTY::get_full(item1, ENTITY_PROP_CAM_DISTANCE);
		int32_t offset = PROPERTY::get_offset(item1, ENTITY_PROP_CAM_DISTANCE);

		int32_t row_c = from_u16(prop.header + 6);

		int32_t meta_length = 0;
		if (row_c == 1)
			meta_length = 4;
		else if (row_c == 2 || row_c == 3)
			meta_length = 8;
		else
		{
			printf("\t !!! unhandled sidescrolling camera distance, row count %d\n", row_c);
			return;
		}

		for (int32_t i = 0; i < row_c; i++)
		{
			int16_t value1 = *(int16_t*)(item1 + offset + meta_length + i * 8 + 0);
			int16_t value2 = *(int16_t*)(item1 + offset + meta_length + i * 8 + 2);
			int16_t value3 = *(int16_t*)(item1 + offset + meta_length + i * 8 + 4);
			int16_t value4 = *(int16_t*)(item1 + offset + meta_length + i * 8 + 6);

			*(int16_t*)(item1 + offset + meta_length + i * 8 + 0) = -value4;
			*(int16_t*)(item1 + offset + meta_length + i * 8 + 2) = -value3;
			*(int16_t*)(item1 + offset + meta_length + i * 8 + 4) = -value2;
			*(int16_t*)(item1 + offset + meta_length + i * 8 + 6) = -value1;
		}
	}

	// level alter utility - level flip x
	void flip_level_x(ELIST& elist, int32_t* chunk_count)
	{
		printf("X move offset (optional, hex):\n");
		int32_t xmove_off = 0;
		scanf("%i", &xmove_off);
		printf("Using x move offset %d (%x)\n", xmove_off, xmove_off);

		for (auto& ntry : elist)
		{
			if (ntry.get_entry_type() == EntryType::Scenery)
			{
				// vertices
				uint8_t* item0 = ntry.get_nth_item(0);
				uint8_t* item1 = ntry.get_nth_item(1);

				int32_t vert_count = from_u32(item0 + 0x10);
				for (int32_t j = 0; j < vert_count; j++)
				{
					uint8_t* vert = item1 + j * 4;
					int16_t x = from_s16(vert + 0) & 0xFFF0;
					int16_t rest = from_s16(vert + 0) & 0xF;
					x = -x;
					x -= 0x10;
					*(int16_t*)(vert + 0) = x | rest;
				}

				// position
				*(int32_t*)(item0 + 0x0) *= -1;
				*(int32_t*)(item0 + 0x0) += xmove_off;
			}

			if (ntry.get_entry_type() == EntryType::Zone)
			{
				uint8_t* item1 = ntry.get_nth_item(1);
				int32_t new_size =
					ntry.get_nth_item_offset(2) - ntry.get_nth_item_offset(1);

				// zone position
				*(int32_t*)(item1 + 0x0) *= -1;
				*(int32_t*)(item1 + 0x0) -= from_s32(item1 + 0xC);
				*(int32_t*)(item1 + 0x0) += xmove_off;

				// flip collision
				int32_t maxx = from_u16(item1 + 0x1E);
				int32_t maxy = from_u16(item1 + 0x20);
				int32_t maxz = from_u16(item1 + 0x22);
				printf("x flipping coll in zone %s, %d %d %d\n", eid2str(ntry.m_eid), maxx, maxy, maxz);
				uint8_t* item1_edited = coll_get_expanded(item1, &new_size, maxx, maxy, maxz);
				LIST flipped{};
				collision_flip(item1_edited, 0x1C, 0, maxx, maxy, maxz, &flipped, 1);
				ntry.replace_nth_item(1, item1_edited, new_size);

				// object positions
				int32_t cam_count = ntry.get_cam_item_count() / 3;
				int32_t entity_count = ntry.get_entity_count();

				for (int32_t j = 0; j < cam_count; j++)
				{
					uint8_t* cam_item0 = ntry.get_nth_item(2 + 3 * j);
					flip_entity_x(cam_item0, from_s32(item1 + 0xC));
					flip_fix_prop_198_x(cam_item0);

					uint8_t* cam_item1 = ntry.get_nth_item(2 + 3 * j + 1);
					flip_camera_angle_x(cam_item1);
					flip_camera_dist_2D_x(cam_item0, cam_item1);
					// todo camera switching property fix?
				}

				for (int32_t j = 0; j < entity_count; j++)
				{
					uint8_t* item = ntry.get_nth_entity(j);
					flip_entity_x(item, from_s32(item1 + 0xC) / 4);
					flip_fix_misc_entity_props(item);
				}

				ntry.m_chunk = *chunk_count;
				*chunk_count += 1;
			}

			if (ntry.get_entry_type() == EntryType::VCOL)
			{
				printf("TODO (unimplemented) x flip of t15 vcol collision entry %s\n", eid2str(ntry.m_eid));

				// todo fix/implement
				/*
				int32_t data_start = 0x1C + build_get_nth_item_offset(ntry.data, 0);
				int32_t data_size = ntry.esize - data_start;

				for (int32_t off = 0; off < data_size; off += 4)
				{
					uint8_t val1 = from_u8(ntry.data + data_start + off);
					uint8_t val2 = from_u8(ntry.data + data_start + off + 2);
					if (val1 == 0xFF || val2 == 0xFF)
						continue;

					// uint8_t flags1 = from_u8(ntry.data + data_start + off + 1);
					// uint8_t flags2 = from_u8(ntry.data + data_start + off + 3);
					// *(uint8_t *)(ntry.data + data_start + off + 1) = flags2;
					// *(uint8_t *)(ntry.data + data_start + off + 3) = flags1;

					// todo fix
					*(uint8_t *)(ntry.data + data_start + off) = 0xFF - val2;
					*(uint8_t *)(ntry.data + data_start + off + 2) = 0xFF - val1;
				}*/
			}
		}
	}

	void level_recolor(ELIST& elist)
	{
		int32_t r_wanted, g_wanted, b_wanted;
		printf("R G B? [hex]\n");
		scanf("%x %x %x", &r_wanted, &g_wanted, &b_wanted);
		int32_t sum_wanted = r_wanted + g_wanted + b_wanted;

		float mult = 1.f;
		printf("brigtness mutliplicator? (float)\n");
		scanf("%f", &mult);

		for (auto& ntry : elist)
		{
			if (ntry.get_entry_type() != EntryType::Scenery)
				continue;

			int32_t offset5 = ntry.get_nth_item_offset(5);
			int32_t offset6 = ntry.get_nth_item_offset(6);
			uint8_t* item5 = ntry.get_nth_item(5);

			printf("%s colors: %d\n", eid2str(ntry.m_eid), (offset6 - offset5) / 4);
			for (int32_t j = 0; j < (offset6 - offset5) / 4; j++)
			{
				// read current color
				uint8_t r = from_u8(item5 + 4 * j + 0);
				uint8_t g = from_u8(item5 + 4 * j + 1);
				uint8_t b = from_u8(item5 + 4 * j + 2);

				// get pseudograyscale of the current color
				int32_t sum = r + g + b;

				// get new color
				int32_t r_new = (sum * r_wanted) / sum_wanted;
				int32_t g_new = (sum * g_wanted) / sum_wanted;
				int32_t b_new = (sum * b_wanted) / sum_wanted;

				r_new *= mult;
				g_new *= mult;
				b_new *= mult;

				// clip it at 0xFF
				r_new = min(r_new, 0xFF);
				g_new = min(g_new, 0xFF);
				b_new = min(b_new, 0xFF);

				// write back
				*(item5 + 4 * j + 0) = r_new;
				*(item5 + 4 * j + 1) = g_new;
				*(item5 + 4 * j + 2) = b_new;
			}
		}
	}

	// mix of rebuild and keeping original stuff
	// useful for making operations on a level then saving it again
	void ll_alter(int32_t alter_type)
	{
		// start unpacking stuff
		char nsfpath[MAX] = "";
		char nsdpath[MAX] = "";
		char nsfpathout[MAX] = "";
		char nsdpathout[MAX] = "";

		printf("Input the path to the level (.nsf):\n");
		scanf(" %[^\n]", nsfpath);
		path_fix(nsfpath);

		strcpy(nsdpath, nsfpath);
		nsdpath[strlen(nsdpath) - 1] = 'D';

		ELIST elist{};

		if (alter_type == AT_LevelExport)
			sscanf(nsfpath + strlen(nsfpath) - 6, "%02X.", &elist.m_level_ID);
		else
			elist.m_level_ID = ask_level_ID();

		uint8_t* chunks[CHUNK_LIST_DEFAULT_SIZE] = { NULL };  // array of pointers to potentially built chunks, fixed length cuz lazy
		uint8_t* chunks2[CHUNK_LIST_DEFAULT_SIZE] = { NULL }; // keeping original non-normal chunks to keep level order

		if (build_read_and_parse_rebld(elist, chunks, 1, nsfpath))
			return;

		// get original (texture/sound/instrument) chunk data
		// its opened and read again :))) dont care
		File nsf_og;
		nsf_og.open(nsfpath, "rb");
		if (!nsf_og.ok())
		{
			printf("[ERROR] Could not open selected NSF\n\n");
			return;
		}

		// make output paths
		strcpy(nsfpathout, nsfpath);
		sprintf(nsfpathout + strlen(nsfpathout) - 6, "%02X.NSF", elist.m_level_ID);
		strcpy(nsdpathout, nsfpathout);
		nsdpathout[strlen(nsdpathout) - 1] = 'D';

		int32_t nsf_chunk_count = chunk_count_base(nsf_og.f);
		uint8_t buffer[CHUNKSIZE];
		for (int32_t i = 0; i < nsf_chunk_count; i++)
		{
			fread(buffer, sizeof(uint8_t), CHUNKSIZE, nsf_og.f);
			ChunkType ct = chunk_type(buffer);
			if (ct == ChunkType::TEXTURE || ct == ChunkType::SOUND || ct == ChunkType::INSTRUMENT)
			{
				chunks2[i] = (uint8_t*)try_malloc(CHUNKSIZE);
				memcpy(chunks2[i], buffer, CHUNKSIZE);
			}
		}
		elist.m_chunk_count = nsf_chunk_count;

		// just to get original spawn
		FILE* nsd_og = fopen(nsdpath, "rb");
		if (nsd_og)
		{
			uint8_t nsd_buf[CHUNKSIZE];
			fseek(nsd_og, 0, SEEK_END);
			int32_t nsd_size = ftell(nsd_og);
			rewind(nsd_og);
			fread(nsd_buf, nsd_size, 1, nsd_og);

			int32_t spawn0_offset =
				C2_NSD_ENTRY_TABLE_OFFSET +
				(from_u32(nsd_buf + C2_NSD_ENTRY_COUNT_OFFSET) * 8) +
				0x10 + 0x1CC;

			SPAWN s0{};
			s0.zone = from_u32(nsd_buf + spawn0_offset);
			s0.x = from_s32(nsd_buf + spawn0_offset + 0xC) >> 8;
			s0.y = from_s32(nsd_buf + spawn0_offset + 0x10) >> 8;
			s0.z = from_s32(nsd_buf + spawn0_offset + 0x14) >> 8;

			elist.m_spawns.clear();
			elist.m_spawns.push_back(s0);
			elist.m_spawns.push_back(s0);
		}
		// end unpacking stuff

		// apply alterations
		switch (alter_type)
		{
		case AT_WipeDL:
			wipe_draw_lists(elist);
			break;
		case AT_WipeEnts:
			wipe_draw_lists(elist);
			wipe_entities(elist);
			break;
		case AT_Old_DL_Override:
			convert_old_dl_override(elist);
			break;
		case AT_FlipScenY:
			printf("!!!Zones are put into indidual chunks at the end\n\n");
			printf("!!!Y flip does not (yet) patch camera distances/angles, t15col and warps/elevator links!");
			flip_level_y(elist, &elist.m_chunk_count);
			break;
		case AT_FlipScenX:
			printf("!!!Zones are put into indidual chunks at the end\n\n");
			flip_level_x(elist, &elist.m_chunk_count);
			break;
		case AT_LevelRecolor:
			level_recolor(elist);
			break;
		case AT_LevelExport:
			// export doesnt make an output nsf
			ll_export::export_level(elist.m_level_ID, elist, chunks, chunks2, elist.m_chunk_count);
			for (int32_t i = 0; i < elist.m_chunk_count; i++)
			{
				if (chunks2[i])
					free(chunks2[i]);
				if (chunks[i])
					free(chunks[i]);
			}
			return;
			break;
		default:
			break;
		}

		// start repacking stuff
		for (auto& ntry : elist)
		{
			EntryType et = ntry.get_entry_type();
			if (et == EntryType::Sound || et == EntryType::Inst || et == EntryType::__invalid)
				ntry.m_chunk = -1;
		}

		elist.m_nsf_out.open(nsfpathout, "wb");
		if (!elist.m_nsf_out.ok())
		{
			printf("\n[ERROR] Couldn't open out NSF - path %s\n\n", nsfpathout);
			return;
		}

		elist.m_nsd_out.open(nsdpathout, "wb");
		if (!elist.m_nsd_out.ok())
		{
			printf("\n[ERROR] Couldn't open out NSD - path %s\n\n", nsdpathout);
			return;
		}

		elist.write_nsd();
		elist.sort_load_lists();
		elist.build_normal_chunks(chunks);
		// re-use original non-normal chunks
		for (int32_t i = 0; i < elist.m_chunk_count; i++)
		{
			if (chunks2[i] != NULL)
			{
				chunks[i] = (uint8_t*)try_malloc(CHUNKSIZE);
				memcpy(chunks[i], chunks2[i], CHUNKSIZE);
			}
		}
		elist.write_nsf(chunks);

		// cleanup
		for (int32_t i = 0; i < elist.m_chunk_count; i++)
		{
			if (chunks2[i])
				free(chunks2[i]);
			if (chunks[i])
				free(chunks[i]);
		}

		printf("\nSaved as %s / .NSD\n", nsfpathout);
		printf("Saving with CrashEdit recommended\n");
		printf("Done.\n\n");
	}

}