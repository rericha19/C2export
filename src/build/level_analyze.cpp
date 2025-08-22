#include "../include.h"

void build_ll_check_load_list_integrity(ENTRY* elist, int32_t entry_count)
{
	printf("\nLoad list integrity check:\n");
	bool issue_found = false;
	char eid1[6] = "";
	char eid2[6] = "";

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (!(build_entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].data != NULL))
			continue;

		int32_t cam_count = build_get_cam_item_count(elist[i].data) / 3;
		for (int32_t j = 0; j < cam_count; j++)
		{
			LOAD_LIST load_list{};
			build_get_load_lists(load_list, elist[i].data, 2 + 3 * j);

			// load and deload everything in 'positive' direction, whatever remains in list is undeloaded
			LIST list{};
			for (auto& sublist : load_list)
			{
				if (sublist.type == 'A')
				{
					for (auto& item: sublist.list.eids())
					{						
						if (list.find(item) != -1)
						{
							issue_found = true;
							printf("Duplicate/already loaded entry %s in zone %s path %d load list A point %d\n",
								eid_conv(item, eid1), eid_conv(elist[i].eid, eid2), j, sublist.index);
						}
					}
				}
				else if (sublist.type == 'B')
				{
					list.remove_all(sublist.list);					
				}
			}

			// load and deload everything in 'negative' direction, whatever remains in list2 was not loaded
			LIST list2{};
			for (size_t l = load_list.size() - 1; l >= 0; l--)
			{
				auto& sublist = load_list[l];
				if (sublist.type == 'A')
				{
					list2.remove_all(sublist.list);					
				}
				else if (sublist.type == 'B')
				{
					for (auto& item: sublist.list.eids())
					{						
						if (list2.find(item) != -1)
						{
							issue_found = true;
							printf("Duplicate/already loaded entry %s in zone %s path %d load list B point %d\n",
								eid_conv(item, eid1), eid_conv(elist[i].eid, eid2), j, sublist.index);
						}
					}
				}
			}

			if (list.count() || list2.count())
			{
				issue_found = true;
				printf("Zone %5s cam path %d load list incorrect:\n", eid_conv2(elist[i].eid), j);
			}

			for (int32_t l = 0; l < list.count(); l++)
				printf("\t%5s (never deloaded)\n", eid_conv2(list[l]));
			for (int32_t l = 0; l < list2.count(); l++)
				printf("\t%5s (deloaded before loaded)\n", eid_conv2(list2[l]));
		}
	}
	if (!issue_found)
		printf("No load list issues were found\n\n");
	else
		printf("\n");
}

void build_ll_check_draw_list_integrity(ENTRY* elist, int32_t entry_count)
{
	printf("Draw list integrity check:\n");
	bool issue_found = false;

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE)
		{
			int32_t cam_count = build_get_cam_item_count(elist[i].data) / 3;
			for (int32_t j = 0; j < cam_count; j++)
			{
				LOAD_LIST draw_list{};
				build_get_draw_lists(draw_list, elist[i].data, 2 + 3 * j);

				LIST ids{};
				for (auto& sublist: draw_list)
				{
					if (sublist.type == 'B')
					{
						for (auto& draw_uint: sublist.list.eids())
						{
							DRAW_ITEM draw_item = build_decode_draw_item(draw_uint);							
							if (ids.find(draw_item.ID) != -1)
							{
								issue_found = true;
								printf("Duplicate/already drawn ID %4d in zone %s path %d load list B point %d\n",
									draw_item.ID, eid_conv2(elist[i].eid), j, sublist.index);
							}
						}
					}
					else if (sublist.type == 'A')
					{
						for (auto& draw_uint: sublist.list.eids())
						{
							DRAW_ITEM draw_item = build_decode_draw_item(draw_uint);
							ids.remove(draw_item.ID);
						}
					}
				}

				LIST ids2{};
				for (size_t k = draw_list.size() - 1; k >= 0; k--)
				{
					auto& sublist = draw_list[k];
					if (sublist.type == 'B')
					{
						for (auto& draw_uint: sublist.list.eids())
						{
							DRAW_ITEM draw_item = build_decode_draw_item(draw_uint);
							ids2.remove(draw_item.ID);
						}
					}
					else if (sublist.type == 'A')
					{
						for (auto& draw_uint : sublist.list.eids())
						{
							DRAW_ITEM draw_item = build_decode_draw_item(draw_uint);																					
							if (ids2.find(draw_item.ID) != -1)
							{
								issue_found = true;
								printf("Duplicate/already drawn ID %4d in zone %s path %d load list B point %d\n",
									draw_item.ID, eid_conv2(elist[i].eid), j, sublist.index);
							}
						}
					}
				}

				char eid1[100] = "";
				char eid2[100] = "";
				if (ids.count() || ids2.count())
				{
					issue_found = true;
					printf("Zone %s cam path %d draw list incorrect:\n", eid_conv2(elist[i].eid), j);
				}

				for (int32_t l = 0; l < ids.count(); l++)
					printf("\t%5d (never undrawn)\n", ids[l]);
				for (int32_t l = 0; l < ids2.count(); l++)
					printf("\t%5d (undrawn before drawn)\n", ids2[l]);

				for (auto& sublist: draw_list)
				{
					for (auto& draw_uint: sublist.list.eids())
					{
						DRAW_ITEM draw_item = build_decode_draw_item(draw_uint);

						int32_t item1off = build_get_nth_item_offset(elist[i].data, 0);
						int32_t neighbour_eid = from_u32(elist[i].data + item1off + C2_NEIGHBOURS_START + 4 + 4 * draw_item.neighbour_zone_index);

						int32_t neighbour_index = build_get_index(neighbour_eid, elist, entry_count);
						if (neighbour_index == -1)
						{
							issue_found = true;
							printf("Zone %s cam path %d drawing ent %4d (list%c pt %2d), tied to invalid neighbour %s (neigh.index %d)\n",
								eid_conv(elist[i].eid, eid1), j, draw_item.ID, sublist.type, sublist.index,
								eid_conv(neighbour_eid, eid2), draw_item.neighbour_zone_index);
							continue;
						}

						int32_t neighbour_cam_item_count = build_get_cam_item_count(elist[neighbour_index].data);
						int32_t neighbour_entity_count = build_get_entity_count(elist[neighbour_index].data);

						if (draw_item.neighbour_item_index >= neighbour_entity_count)
						{
							issue_found = true;
							printf("Zone %s cam path %d drawing ent %4d (list%c pt %2d), tied to %s's %2d. entity, which doesnt exist\n",
								eid_conv(elist[i].eid, eid1), j, draw_item.ID, sublist.type, sublist.index,
								eid_conv(neighbour_eid, eid2), draw_item.neighbour_item_index);
							continue;
						}

						int32_t neighbour_entity_offset = build_get_nth_item_offset(elist[neighbour_index].data,
							2 + neighbour_cam_item_count + draw_item.neighbour_item_index);
						int32_t neighbour_entity_ID = build_get_entity_prop(elist[neighbour_index].data + neighbour_entity_offset, ENTITY_PROP_ID);

						if (draw_item.ID != neighbour_entity_ID)
						{
							issue_found = true;
							printf("Zone %s cam path %d drawing ent %4d (list%c pt %2d), tied to %s's %2d. entity, which has ID %4d\n",
								eid_conv(elist[i].eid, eid1), j, draw_item.ID, sublist.type, sublist.index,
								eid_conv(neighbour_eid, eid2), draw_item.neighbour_item_index, neighbour_entity_ID);
						}
					}
				}
			}
		}
	}
	if (!issue_found)
		printf("No draw list issues were found\n");
	printf("\n");
}

void build_ll_print_avg(ENTRY* elist, int32_t entry_count)
{
	/* part that counts avg portion taken of used normal chunks */
	int32_t chunk_count = 0;

	for (int32_t i = 0; i < entry_count; i++)
		if (elist[i].chunk >= chunk_count)
			chunk_count = elist[i].chunk + 1;

	int32_t* chunksizes = (int32_t*)try_calloc(chunk_count, sizeof(int32_t));
	for (int32_t i = 0; i < entry_count; i++)
	{
		if (build_is_normal_chunk_entry(elist[i]))
		{
			int32_t chunk = elist[i].chunk;
			if (chunksizes[chunk] == 0)
				chunksizes[chunk] = 0x14;
			chunksizes[chunk] += 4 + elist[i].esize;
		}
	}

	int32_t normal_chunk_counter = 0;
	int32_t total_size = 0;
	for (int32_t i = 0; i < chunk_count; i++)
		if (chunksizes[i] != 0)
		{
			normal_chunk_counter++;
			total_size += chunksizes[i];
		}
	free(chunksizes);
	printf("Average normal chunk portion taken: %.3f%c\n", (100 * (double)total_size / normal_chunk_counter) / CHUNKSIZE, '%');
}

int32_t pay_cmp2(const void* a, const void* b)
{
	PAYLOAD x = *(PAYLOAD*)a;
	PAYLOAD y = *(PAYLOAD*)b;

	if (x.zone != y.zone)
		return x.zone - y.zone;
	else
		return x.cam_path - y.cam_path;
}

void build_ll_print_full_payload_info(ENTRY* elist, int32_t entry_count, int32_t print_full)
{
	/* gets and prints payload ladder */
	PAYLOADS payloads = build_get_payload_ladder(elist, entry_count, 0);
	qsort(payloads.arr, payloads.count, sizeof(PAYLOAD), pay_cmp2);

	printf("\nFull payload info (max payload of each camera path):\n");
	for (int32_t k = 0; k < payloads.count; k++)
	{
		printf("%d\t", k + 1);
		build_print_payload(payloads.arr[k]);

		if (payloads.arr[k].count >= 21 || print_full)
		{
			qsort(payloads.arr[k].chunks, payloads.arr[k].count, sizeof(int32_t), cmp_func_int);
			printf("    chunks:");
			for (int32_t l = 0; l < payloads.arr[k].count; l++)
				printf(" %3d", 1 + 2 * payloads.arr[k].chunks[l]);
			printf("\n");
		}

		if (payloads.arr[k].tcount >= 9 || print_full)
		{
			if (payloads.arr[k].tcount >= 9)
				printf("    !!!tpages:");
			else
				printf("    tpages:");

			for (int32_t l = 0; l < payloads.arr[k].tcount; l++)
				printf(" %5s", eid_conv2(payloads.arr[k].tchunks[l]));
			printf("\n");
		}

		if (payloads.arr[k].chunks)
			free(payloads.arr[k].chunks);
		if (payloads.arr[k].tchunks)
			free(payloads.arr[k].tchunks);

		if (print_full)
			printf("\n");
	}
	if (!print_full)
		printf("\n");
}

void build_ll_various_stats(ENTRY* elist, int32_t entry_count)
{
	int32_t total_entity_count = 0;
	DEPENDENCIES deps{};

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE)
		{
			int32_t camera_count = build_get_cam_item_count(elist[i].data);
			int32_t entity_count = build_get_entity_count(elist[i].data);

			for (int32_t j = 0; j < entity_count; j++)
			{
				uint8_t* entity = build_get_nth_item(elist[i].data, (2 + camera_count + j));
				int32_t type = build_get_entity_prop(entity, ENTITY_PROP_TYPE);
				int32_t subt = build_get_entity_prop(entity, ENTITY_PROP_SUBTYPE);
				int32_t id = build_get_entity_prop(entity, ENTITY_PROP_ID);

				if (id == -1)
					continue;
				total_entity_count++;

				bool found_before = false;
				for (size_t k = 0; k < deps.size(); k++)
					if (deps[k].type == type && deps[k].subtype == subt)
					{
						deps[k].dependencies.add(total_entity_count);
						found_before = true;
					}
				if (!found_before)
				{
					DEPENDENCY new_dep{ type, subt };
					new_dep.dependencies.add(total_entity_count);
					deps.push_back(std::move(new_dep));
				}
			}
		}
	}

	std::sort(deps.begin(), deps.end(), [](DEPENDENCY x, DEPENDENCY y)
		{
			if (x.type != y.type)
				return x.type < y.type;
			else
				return x.subtype < y.subtype;
		});

	printf("Entity type/subtype usage:\n");
	for (auto& dep : deps)
		printf("Type %2d, subtype %2d: %3d entities\n", dep.type, dep.subtype, dep.dependencies.count());
	printf("\n");

	printf("Miscellaneous stats:\n");
	printf("Entity count:\t%4d\n", total_entity_count);
	printf("Entry count:\t%4d\n", entry_count);
}

void build_ll_check_zone_references(ENTRY* elist, int32_t entry_count)
{
	printf("\nZones' references check (only zones with camera paths are checked):\n");
	bool issue_found = false;
	char eid1[100] = "";
	char eid2[100] = "";
	LIST valid_referenced_eids{};

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE)
		{
			int32_t cam_item_count = build_get_cam_item_count(elist[i].data);
			if (cam_item_count == 0)
				continue;

			LIST sceneries = build_get_sceneries(elist[i].data);
			LIST neighbours = build_get_neighbours(elist[i].data);
			uint32_t music_entry = build_get_zone_track(elist[i].data);

			int32_t music_entry_index = build_get_index(music_entry, elist, entry_count);
			if (music_entry_index == -1 && music_entry != EID_NONE)
			{
				issue_found = true;
				printf("Zone %5s references track %5s, which isnt present in the level\n",
					eid_conv(elist[i].eid, eid1), eid_conv(music_entry, eid2));
			}
			else
				valid_referenced_eids.add(music_entry);

			for (int32_t j = 0; j < sceneries.count(); j++)
			{
				uint32_t scenery_ref = sceneries[j];
				int32_t elist_index = build_get_index(scenery_ref, elist, entry_count);
				if (elist_index == -1)
				{
					issue_found = true;
					printf("Zone %5s references scenery %5s, which isnt present in the level\n",
						eid_conv(elist[i].eid, eid1), eid_conv(scenery_ref, eid2));
				}
				else
					valid_referenced_eids.add(scenery_ref);
			}

			for (int32_t j = 0; j < neighbours.count(); j++)
			{
				uint32_t neighbour_ref = neighbours[j];
				int32_t elist_index = build_get_index(neighbour_ref, elist, entry_count);
				if (elist_index == -1)
				{
					issue_found = true;
					printf("Zone %5s references neighbour %5s, which isnt present in the level\n",
						eid_conv(elist[i].eid, eid1), eid_conv(neighbour_ref, eid2));
				}
				else
					valid_referenced_eids.add(neighbour_ref);
			}

			for (int32_t j = 0; j < cam_item_count / 3; j++)
			{
				uint8_t* item = build_get_nth_item_offset(elist[i].data, 2 + 3 * j) + elist[i].data;
				uint32_t slst = build_get_slst(item);
				int32_t slst_index = build_get_index(slst, elist, entry_count);
				if (slst_index == -1)
				{
					issue_found = true;
					printf("Zone %s cam path %d references SLST %5s, which isnt present in the level\n",
						eid_conv(elist[i].eid, eid1), j, eid_conv(slst, eid2));
				}
				else
					valid_referenced_eids.add(slst);
			}
		}
	}

	if (!issue_found)
		printf("No zone reference issues were found\n");

	printf("\nUnused entries (not referenced by any zone w/ a camera path):\n");
	bool unused_entries_found = false;

	for (int32_t i = 0; i < entry_count; i++)
	{
		int32_t entry_type = build_entry_type(elist[i]);
		if (!(entry_type == ENTRY_TYPE_SCENERY || entry_type == ENTRY_TYPE_ZONE || entry_type == ENTRY_TYPE_MIDI || entry_type == ENTRY_TYPE_SLST))
			continue;

		if (valid_referenced_eids.find(elist[i].eid) == -1)
		{
			printf("Entry %5s not referenced\n", eid_conv2(elist[i].eid));
			unused_entries_found = true;
		}
	}

	if (!unused_entries_found)
		printf("No unused entries were found\n");
}

void build_ll_check_gool_references(ENTRY* elist, int32_t entry_count, uint32_t* gool_table)
{
	LIST used_types{};

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE)
		{
			int32_t camera_count = build_get_cam_item_count(elist[i].data);
			int32_t entity_count = build_get_entity_count(elist[i].data);

			for (int32_t j = 0; j < entity_count; j++)
			{
				uint8_t* entity = build_get_nth_item(elist[i].data, (2 + camera_count + j));
				int32_t type = build_get_entity_prop(entity, ENTITY_PROP_TYPE);
				int32_t id = build_get_entity_prop(entity, ENTITY_PROP_ID);

				if (type >= 0 && id != -1)
					used_types.add(type);
			}
		}
	}

	bool unused_gool_entries = false;
	printf("\nUnused GOOL entries:\n");
	for (int32_t i = 0; i < C3_GOOL_TABLE_SIZE; i++)
	{
		if (gool_table[i] == EID_NONE)
			continue;

		if (used_types.find(i) == -1)
		{
			unused_gool_entries = true;
			printf("GOOL entry %5s type %2d not directly used by any entity (could still be used by other means)\n",
				eid_conv2(gool_table[i]), i);
		}
	}

	if (!unused_gool_entries)
		printf("No unused gool entries were found\n");

	LIST gool_i6_references{};
	for (int32_t i = 0; i < entry_count; i++)
	{
		if (build_entry_type(elist[i]) == ENTRY_TYPE_GOOL && build_item_count(elist[i].data) == 6)
		{
			uint32_t i6_off = build_get_nth_item_offset(elist[i].data, 5);
			uint32_t i6_end = build_get_nth_item_offset(elist[i].data, 6);
			uint32_t i6_len = i6_end - i6_off;

			for (uint32_t j = 0; j < i6_len / 4; j++)
			{
				uint32_t potential_eid = from_u32(elist[i].data + i6_off + 4 * j);
				if (build_get_index(potential_eid, elist, entry_count) == -1)
					continue;

				gool_i6_references.add(potential_eid);
			}
		}
	}

	bool unreferenced_animations = false;
	printf("\nAnimations not referenced by item6 of any GOOL entry:\n");
	LIST model_references{};
	for (int32_t i = 0; i < entry_count; i++)
	{
		if (build_entry_type(elist[i]) == ENTRY_TYPE_ANIM)
		{
			if (gool_i6_references.find(elist[i].eid) == -1)
			{
				unreferenced_animations = true;
				printf("Animation %5s not referenced by any GOOL's i6\n", eid_conv2(elist[i].eid));
			}

			LIST animations_models = build_get_models(elist[i].data);
			model_references.copy_in(animations_models);
		}
	}

	if (!unreferenced_animations)
		printf("No unreferenced animations found\n");

	bool unreferenced_models = false;
	printf("\nModels not referenced by any animation:\n");
	for (int32_t i = 0; i < entry_count; i++)
	{
		if (build_entry_type(elist[i]) == ENTRY_TYPE_MODEL)
		{
			if (model_references.find(elist[i].eid) == -1)
			{
				unreferenced_models = true;
				printf("Model %5s not referenced by any animation\n", eid_conv2(elist[i].eid));
			}
		}
	}

	if (!unreferenced_models)
		printf("No unreferenced models found\n");
}

void build_ll_id_usage(ENTRY* elist, int32_t entry_count)
{
	printf("\nID usage:\n");
	int32_t max_id = 10;

	LIST lists[1024]{};

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE)
		{
			int32_t entity_count = build_get_entity_count(elist[i].data);
			int32_t camera_count = build_get_cam_item_count(elist[i].data);
			for (int32_t j = 0; j < entity_count; j++)
			{
				uint8_t* entity = build_get_nth_item(elist[i].data, (2 + camera_count + j));
				int32_t id = build_get_entity_prop(entity, ENTITY_PROP_ID);

				if (id == -1 || id >= 1024)
					continue;

				max_id = max(max_id, id);
				lists[id].add(elist[i].eid);
			}
		}
	}

	for (int32_t i = 0; i <= max_id; i++)
	{
		printf("id %4d: %2d\t", i, lists[i].count());

		if (i < 10)
			printf("reserved ");

		for (int32_t j = 0; j < lists[i].count(); j++)
			printf("%5s ", eid_conv2(lists[i][j]));

		printf("\n");
	}
	printf("\n");
}

void build_ll_check_tpag_references(ENTRY* elist, int32_t entry_count)
{
	printf("\nTexture reference check:\n");
	bool unreferenced_textures = false;

	LIST text_refs{};
	for (int32_t i = 0; i < entry_count; i++)
	{
		int32_t entry_type = build_entry_type(elist[i]);
		switch (entry_type)
		{
		case ENTRY_TYPE_MODEL:
			build_add_model_textures_to_list(elist[i].data, &text_refs);
			break;
		case ENTRY_TYPE_SCENERY:
			build_add_scen_textures_to_list(elist[i].data, &text_refs);
			break;
		case ENTRY_TYPE_GOOL:
		{
			int32_t ic = build_item_count(elist[i].data);
			if (ic == 6)
			{
				uint32_t i6_off = build_get_nth_item_offset(elist[i].data, 5);
				uint32_t i6_end = build_get_nth_item_offset(elist[i].data, 6);
				uint32_t i6_len = i6_end - i6_off;

				for (uint32_t j = 0; j < i6_len / 4; j++)
				{
					uint32_t potential_eid = from_u32(elist[i].data + i6_off + 4 * j);
					if (build_get_index(potential_eid, elist, entry_count) == -1)
						continue;

					text_refs.add(potential_eid);
				}
			}
			break;
		}
		default:
			break;
		}
	}

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (elist[i].data == NULL && eid_conv2(elist[i].eid)[4] == 'T')
		{
			bool reffound = false;
			for (int32_t j = 0; j < text_refs.count(); j++)
			{
				if (elist[i].eid == text_refs[j])
				{
					reffound = true;
					break;
				}
			}
			if (!reffound)
			{
				unreferenced_textures = true;
				printf("Texture %5s not referenced by any GOOL, scenery or model\n", eid_conv2(elist[i].eid));
			}
		}
	}

	if (!unreferenced_textures)
		printf("No unreferenced textures found.\n");
}

void build_ll_check_sound_references(ENTRY* elist, int32_t entry_count)
{
	printf("\nSound reference check:\n");
	LIST potential_sounds{};
	bool unreferenced_sounds = false;

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (build_entry_type(elist[i]) == ENTRY_TYPE_GOOL)
		{
			int32_t ic = build_item_count(elist[i].data);
			if (ic == 6)
			{
				uint32_t i3off = build_get_nth_item_offset(elist[i].data, 2);
				uint32_t i4off = build_get_nth_item_offset(elist[i].data, 3);
				uint32_t i3len = i4off - i3off;

				for (uint32_t j = 0; j < i3len / 4; j++)
				{
					uint32_t potential_eid = from_u32(elist[i].data + i3off + 4 * j);
					if (build_get_index(potential_eid, elist, entry_count) == -1)
						continue;

					potential_sounds.add(potential_eid);
				}
			}
		}
	}

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (build_entry_type(elist[i]) == ENTRY_TYPE_SOUND)
		{
			bool reffound = false;
			for (int32_t j = 0; j < potential_sounds.count(); j++)
			{
				if (potential_sounds[j] == elist[i].eid)
				{
					reffound = true;
					break;
				}
			}

			if (!reffound)
			{
				unreferenced_sounds = true;
				printf("Sound %5s not referenced by any GOOL\n", eid_conv2(elist[i].eid));
			}
		}
	}

	if (!unreferenced_sounds)
		printf("No unreferenced sounds found\n");
}

void build_ll_check_gool_types(ENTRY* elist, int32_t entry_count)
{
	printf("\nGOOL types list:\n");

	LIST lists[C3_GOOL_TABLE_SIZE]{};

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (build_entry_type(elist[i]) == ENTRY_TYPE_GOOL)
		{
			int32_t item1_offset = build_get_nth_item_offset(elist[i].data, 0);
			int32_t gool_type = from_u32(elist[i].data + item1_offset);
			if (gool_type < 0 || gool_type >= C3_GOOL_TABLE_SIZE)
			{
				printf("Invalid GOOL type: %s type %d\n", eid_conv2(elist[i].eid), gool_type);
			}
			else
			{
				lists[gool_type].add(elist[i].eid);
			}
		}
	}

	for (int32_t i = 0; i < 64; i++)
	{
		printf("%2d:\t", i);
		for (int32_t j = 0; j < lists[i].count(); j++)
		{
			printf("%s\t", eid_conv2(lists[i][j]));
		}
		printf("\n");
	}
	printf("\n");
}

void ll_payload_info_main()
{
	ENTRY* elist = (ENTRY*)try_calloc(sizeof(ENTRY), ELIST_DEFAULT_SIZE);
	int32_t entry_count = 0;
	uint32_t gool_table[C3_GOOL_TABLE_SIZE];
	for (int32_t i = 0; i < C3_GOOL_TABLE_SIZE; i++)
		gool_table[i] = EID_NONE;

	if (build_read_and_parse_rebld(NULL, NULL, NULL, NULL, gool_table, elist, &entry_count, NULL, NULL, 1, NULL))
		return;

	build_ll_print_full_payload_info(elist, entry_count, 1);
	build_cleanup_elist(elist, entry_count);
	printf("Done.\n\n");
}

void build_ll_analyze()
{
	ENTRY* elist = (ENTRY*)try_calloc(sizeof(ENTRY), ELIST_DEFAULT_SIZE);
	int32_t entry_count = 0;
	uint32_t gool_table[C3_GOOL_TABLE_SIZE];
	for (int32_t i = 0; i < C3_GOOL_TABLE_SIZE; i++)
		gool_table[i] = EID_NONE;

	if (build_read_and_parse_rebld(NULL, NULL, NULL, NULL, gool_table, elist, &entry_count, NULL, NULL, 1, NULL))
		return;

	build_ll_id_usage(elist, entry_count);
	build_ll_various_stats(elist, entry_count);
	build_get_box_count(elist, entry_count);
	build_ll_print_avg(elist, entry_count);
	build_ll_print_full_payload_info(elist, entry_count, 0);
	build_ll_check_gool_types(elist, entry_count);
	build_ll_check_gool_references(elist, entry_count, gool_table);
	build_ll_check_tpag_references(elist, entry_count);
	build_ll_check_sound_references(elist, entry_count);
	build_ll_check_zone_references(elist, entry_count);
	build_ll_check_load_list_integrity(elist, entry_count);
	build_ll_check_draw_list_integrity(elist, entry_count);

	build_cleanup_elist(elist, entry_count);
	printf("Done.\n\n");
}
