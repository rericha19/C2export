#include "../include.h"
#include "../utils/payloads.hpp"
#include "../utils/utils.hpp"
#include "../game_structs/entry.hpp"

void build_ll_check_load_list_integrity(ELIST& elist)
{
	printf("\nLoad list integrity check:\n");
	int32_t entry_count = int32_t(elist.size());
	bool issue_found = false;
	char eid1[6] = "";
	char eid2[6] = "";

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (!(build_entry_type(elist[i]) == ENTRY_TYPE_ZONE && !elist[i].data.empty()))
			continue;

		int32_t cam_count = build_get_cam_item_count(elist[i]._data()) / 3;
		for (int32_t j = 0; j < cam_count; j++)
		{
			LOAD_LIST load_list = get_load_lists(elist[i]._data(), 2 + 3 * j);

			// load and deload everything in 'positive' direction, whatever remains in list is undeloaded
			LIST list{};
			for (auto& sublist : load_list)
			{
				if (sublist.type == 'A')
				{
					for (auto& item : sublist.list)
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
			for (int32_t l = int32_t(load_list.size()) - 1; l >= 0; l--)
			{
				auto& sublist = load_list[l];
				if (sublist.type == 'A')
				{
					list2.remove_all(sublist.list);
				}
				else if (sublist.type == 'B')
				{
					for (auto& item : sublist.list)
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

void build_ll_check_draw_list_integrity(ELIST& elist)
{
	printf("Draw list integrity check:\n");
	int32_t entry_count = int32_t(elist.size());
	bool issue_found = false;

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (build_entry_type(elist[i]) != ENTRY_TYPE_ZONE)
			continue;

		int32_t cam_count = build_get_cam_item_count(elist[i]._data()) / 3;
		for (int32_t j = 0; j < cam_count; j++)
		{
			DRAW_LIST draw_list = get_draw_lists(elist[i]._data(), 2 + 3 * j);

			LIST ids{};
			for (auto& sublist : draw_list)
			{
				if (sublist.type == 'B')
				{
					for (auto& draw_uint : sublist.list)
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
					for (auto& draw_uint : sublist.list)
					{
						DRAW_ITEM draw_item = build_decode_draw_item(draw_uint);
						ids.remove(draw_item.ID);
					}
				}
			}

			LIST ids2{};
			for (int32_t k = int32_t(draw_list.size()) - 1; k >= 0; k--)
			{
				auto& sublist = draw_list[k];
				if (sublist.type == 'B')
				{
					for (auto& draw_uint : sublist.list)
					{
						DRAW_ITEM draw_item = build_decode_draw_item(draw_uint);
						ids2.remove(draw_item.ID);
					}
				}
				else if (sublist.type == 'A')
				{
					for (auto& draw_uint : sublist.list)
					{
						DRAW_ITEM draw_item = build_decode_draw_item(draw_uint);
						if (ids2.find(draw_item.ID) != -1)
						{
							issue_found = true;
							printf("Duplicate/already drawn ID %4d in zone %s path %d load list B point %d\n",
								draw_item.ID, elist[i].ename, j, sublist.index);
						}
					}
				}
			}

			char eid1[100] = "";
			char eid2[100] = "";
			if (ids.count() || ids2.count())
			{
				issue_found = true;
				printf("Zone %s cam path %d draw list incorrect:\n", elist[i].ename, j);
			}

			for (int32_t l = 0; l < ids.count(); l++)
				printf("\t%5d (never undrawn)\n", ids[l]);
			for (int32_t l = 0; l < ids2.count(); l++)
				printf("\t%5d (undrawn before drawn)\n", ids2[l]);

			for (auto& sublist : draw_list)
			{
				for (auto& draw_uint : sublist.list)
				{
					DRAW_ITEM draw_item = build_decode_draw_item(draw_uint);

					int32_t item1off = build_get_nth_item_offset(elist[i]._data(), 0);
					int32_t neighbour_eid = from_u32(elist[i]._data() + item1off + C2_NEIGHBOURS_START + 4 + 4 * draw_item.neighbour_zone_index);

					int32_t neighbour_index = elist.get_index(neighbour_eid);
					if (neighbour_index == -1)
					{
						issue_found = true;
						printf("Zone %s cam path %d drawing ent %4d (list%c pt %2d), tied to invalid neighbour %s (neigh.index %d)\n",
							eid_conv(elist[i].eid, eid1), j, draw_item.ID, sublist.type, sublist.index,
							eid_conv(neighbour_eid, eid2), draw_item.neighbour_zone_index);
						continue;
					}

					int32_t neighbour_cam_item_count = build_get_cam_item_count(elist[neighbour_index]._data());
					int32_t neighbour_entity_count = build_get_entity_count(elist[neighbour_index]._data());

					if (draw_item.neighbour_item_index >= neighbour_entity_count)
					{
						issue_found = true;
						printf("Zone %s cam path %d drawing ent %4d (list%c pt %2d), tied to %s's %2d. entity, which doesnt exist\n",
							eid_conv(elist[i].eid, eid1), j, draw_item.ID, sublist.type, sublist.index,
							eid_conv(neighbour_eid, eid2), draw_item.neighbour_item_index);
						continue;
					}

					int32_t neighbour_entity_offset = build_get_nth_item_offset(elist[neighbour_index]._data(),
						2 + neighbour_cam_item_count + draw_item.neighbour_item_index);
					int32_t neighbour_entity_ID = build_get_entity_prop(elist[neighbour_index]._data() + neighbour_entity_offset, ENTITY_PROP_ID);

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

	if (!issue_found)
		printf("No draw list issues were found\n");
	printf("\n");
}

void build_ll_print_avg(ELIST& elist)
{
	int32_t entry_count = int32_t(elist.size());
	int32_t chunk_count = 0;

	for (int32_t i = 0; i < entry_count; i++)
		if (elist[i].chunk >= chunk_count)
			chunk_count = elist[i].chunk + 1;

	std::vector<int32_t> chunksizes{};
	chunksizes.resize(chunk_count);

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
	{
		if (chunksizes[i] != 0)
		{
			normal_chunk_counter++;
			total_size += chunksizes[i];
		}
	}

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

void build_ll_print_full_payload_info(ELIST& elist, int32_t print_full)
{
	PAYLOADS payloads = PAYLOADS::get_payload_ladder_ll(elist, 0);

	printf("\nFull payload info (max payload of each camera path):\n");
	int32_t k = 0;
	for (auto& payload : payloads)
	{
		printf("%d\t", ++k);
		payload.print_info();

		if (payload.page_count() >= 21 || print_full)
		{
			std::sort(payload.chunks.begin(), payload.chunks.end());
			printf("    chunks:");
			for (auto& c : payload.chunks)
				printf(" %3d", 1 + 2 * c);
			printf("\n");
		}

		if (payload.tpage_count() >= 9 || print_full)
		{
			if (payload.page_count() >= 9)
				printf("    !!!tpages:");
			else
				printf("    tpages:");

			for (auto& tc : payload.tchunks)
				printf(" %5s", eid_conv2(tc));
			printf("\n");
		}

		if (print_full)
			printf("\n");
	}
	if (!print_full)
		printf("\n");
}

void build_ll_various_stats(ELIST& elist)
{
	int32_t total_entity_count = 0;
	int32_t entry_count = int32_t(elist.size());
	DEPENDENCIES deps{};

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (build_entry_type(elist[i]) != ENTRY_TYPE_ZONE)
			continue;

		int32_t camera_count = build_get_cam_item_count(elist[i]._data());
		int32_t entity_count = build_get_entity_count(elist[i]._data());

		for (int32_t j = 0; j < entity_count; j++)
		{
			uint8_t* entity = build_get_nth_item(elist[i]._data(), (2 + camera_count + j));
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

void build_ll_check_zone_references(ELIST& elist)
{
	printf("\nZones' references check (only zones with camera paths are checked):\n");
	bool issue_found = false;
	int32_t entry_count = int32_t(elist.size());
	char eid1[100] = "";
	char eid2[100] = "";
	LIST valid_referenced_eids{};

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE)
		{
			int32_t cam_item_count = build_get_cam_item_count(elist[i]._data());
			if (!cam_item_count)
				continue;


			uint32_t music_entry = build_get_zone_track(elist[i]._data());
			int32_t music_entry_index = elist.get_index(music_entry);
			if (music_entry_index == -1 && music_entry != EID_NONE)
			{
				issue_found = true;
				printf("Zone %5s references track %5s, which isnt present in the level\n",
					eid_conv(elist[i].eid, eid1), eid_conv(music_entry, eid2));
			}
			else
			{
				valid_referenced_eids.add(music_entry);
			}

			LIST sceneries = build_get_sceneries(elist[i]._data());
			for (auto& scenery_ref : sceneries)
			{
				int32_t elist_index = elist.get_index(scenery_ref);
				if (elist_index == -1)
				{
					issue_found = true;
					printf("Zone %5s references scenery %5s, which isnt present in the level\n",
						eid_conv(elist[i].eid, eid1), eid_conv(scenery_ref, eid2));
				}
				else
					valid_referenced_eids.add(scenery_ref);
			}

			LIST neighbours = ENTRY_2::get_neighbours(elist[i]._data());
			for (auto& neighbour_ref : neighbours)
			{
				int32_t elist_index = elist.get_index(neighbour_ref);
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
				uint8_t* item = build_get_nth_item(elist[i]._data(), 2 + 3 * j);
				uint32_t slst = build_get_slst(item);
				int32_t slst_index = elist.get_index(slst);
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

void build_ll_check_gool_references(ELIST& elist, uint32_t* gool_table)
{
	LIST used_types{};
	int32_t entry_count = int32_t(elist.size());

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (build_entry_type(elist[i]) != ENTRY_TYPE_ZONE)
			continue;

		int32_t camera_count = build_get_cam_item_count(elist[i]._data());
		int32_t entity_count = build_get_entity_count(elist[i]._data());

		for (int32_t j = 0; j < entity_count; j++)
		{
			uint8_t* entity = build_get_nth_item(elist[i]._data(), (2 + camera_count + j));
			int32_t type = build_get_entity_prop(entity, ENTITY_PROP_TYPE);
			int32_t id = build_get_entity_prop(entity, ENTITY_PROP_ID);

			if (type >= 0 && id != -1)
				used_types.add(type);
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
				ENTRY_2::eid_to_str(gool_table[i]).c_str(), i);
		}
	}

	if (!unused_gool_entries)
		printf("No unused gool entries were found\n");

	LIST gool_i6_references{};
	for (int32_t i = 0; i < entry_count; i++)
	{
		if (build_entry_type(elist[i]) == ENTRY_TYPE_GOOL && build_item_count(elist[i]._data()) == 6)
		{
			uint32_t i6_off = build_get_nth_item_offset(elist[i]._data(), 5);
			uint32_t i6_end = build_get_nth_item_offset(elist[i]._data(), 6);
			uint32_t i6_len = i6_end - i6_off;

			for (uint32_t j = 0; j < i6_len / 4; j++)
			{
				uint32_t potential_eid = from_u32(elist[i]._data() + i6_off + 4 * j);
				if (elist.get_index(potential_eid) == -1)
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
				printf("Animation %5s not referenced by any GOOL's i6\n", ENTRY_2::eid_to_str(elist[i].eid).c_str());
			}

			LIST animations_models = ENTRY_2::get_models(elist[i]._data());
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
				printf("Model %5s not referenced by any animation\n", elist[i].ename);
			}
		}
	}

	if (!unreferenced_models)
		printf("No unreferenced models found\n");
}

void build_ll_id_usage(ELIST& elist)
{
	printf("\nID usage:\n");
	int32_t max_id = 10;
	int32_t entry_count = int32_t(elist.size());

	std::vector<std::string> lists[1024]{};

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (build_entry_type(elist[i]) != ENTRY_TYPE_ZONE)
			continue;

		int32_t entity_count = build_get_entity_count(elist[i]._data());
		int32_t camera_count = build_get_cam_item_count(elist[i]._data());
		for (int32_t j = 0; j < entity_count; j++)
		{
			uint8_t* entity = build_get_nth_item(elist[i]._data(), (2 + camera_count + j));
			int32_t id = build_get_entity_prop(entity, ENTITY_PROP_ID);

			if (id == -1 || id >= 1024)
				continue;

			max_id = max(max_id, id);
			lists[id].push_back(elist[i].ename);
		}
	}

	for (int32_t i = 0; i <= max_id; i++)
	{
		printf("id %4d: %2d\t", i, int32_t(lists[i].size()));

		if (i < 10)
			printf("reserved ");

		for (auto& eid : lists[i])
			printf("%5s ", eid.c_str());

		printf("\n");
	}
	printf("\n");
}

void build_ll_check_tpag_references(ELIST& elist)
{
	printf("\nTexture reference check:\n");
	bool unreferenced_textures = false;
	int32_t entry_count = int32_t(elist.size());

	LIST text_refs{};
	for (int32_t i = 0; i < entry_count; i++)
	{
		int32_t entry_type = build_entry_type(elist[i]);
		switch (entry_type)
		{
		case ENTRY_TYPE_MODEL:
			build_add_model_textures_to_list(elist[i]._data(), &text_refs);
			break;
		case ENTRY_TYPE_SCENERY:
			build_add_scen_textures_to_list(elist[i]._data(), &text_refs);
			break;
		case ENTRY_TYPE_GOOL:
		{
			int32_t ic = build_item_count(elist[i]._data());
			if (ic == 6)
			{
				uint32_t i6_off = build_get_nth_item_offset(elist[i]._data(), 5);
				uint32_t i6_end = build_get_nth_item_offset(elist[i]._data(), 6);
				uint32_t i6_len = i6_end - i6_off;

				for (uint32_t j = 0; j < i6_len / 4; j++)
				{
					uint32_t potential_eid = from_u32(elist[i]._data() + i6_off + 4 * j);
					if (elist.get_index(potential_eid) == -1)
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
		if (elist[i].data.empty() && eid_conv2(elist[i].eid)[4] == 'T')
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
				printf("Texture %5s not referenced by any GOOL, scenery or model\n", elist[i].ename);
			}
		}
	}

	if (!unreferenced_textures)
		printf("No unreferenced textures found.\n");
}

void build_ll_check_sound_references(ELIST& elist)
{
	printf("\nSound reference check:\n");
	LIST potential_sounds{};
	bool unreferenced_sounds = false;

	for (auto& ntry : elist)
	{
		if (build_entry_type(ntry) != ENTRY_TYPE_GOOL)
			continue;

		if (build_item_count(ntry._data()) == 6)
		{
			uint32_t i2off = build_get_nth_item_offset(ntry._data(), 2);
			uint32_t i3off = build_get_nth_item_offset(ntry._data(), 3);
			uint32_t i2len = i3off - i2off;

			for (uint32_t j = 0; j < i2len / 4; j++)
			{
				uint32_t potential_eid = from_u32(ntry._data() + i2off + 4 * j);
				if (elist.get_index(potential_eid) == -1)
					continue;

				potential_sounds.add(potential_eid);
			}
		}
	}

	for (auto& ntry : elist)
	{
		if (build_entry_type(ntry) != ENTRY_TYPE_SOUND)
			continue;

		bool reffound = false;
		for (int32_t j = 0; j < potential_sounds.count(); j++)
		{
			if (potential_sounds[j] == ntry.eid)
			{
				reffound = true;
				break;
			}
		}

		if (!reffound)
		{
			unreferenced_sounds = true;
			printf("Sound %5s not referenced by any GOOL\n", ntry.ename);
		}
	}

	if (!unreferenced_sounds)
		printf("No unreferenced sounds found\n");
}

void build_ll_check_gool_types(ELIST& elist)
{
	printf("\nGOOL types list:\n");

	int32_t entry_count = int32_t(elist.size());
	std::vector<std::string> lists[C3_GOOL_TABLE_SIZE]{};

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (build_entry_type(elist[i]) != ENTRY_TYPE_GOOL)
			continue;

		int32_t item1_offset = build_get_nth_item_offset(elist[i]._data(), 0);
		int32_t gool_type = from_u32(elist[i]._data() + item1_offset);
		if (gool_type < 0 || gool_type >= C3_GOOL_TABLE_SIZE)
		{
			printf("Invalid GOOL type: %s type %d\n", elist[i].ename, gool_type);
		}
		else
		{
			lists[gool_type].push_back(elist[i].ename);
		}
	}

	for (int32_t i = 0; i < C3_GOOL_TABLE_SIZE; i++)
	{
		printf("%2d:\t", i);
		for (auto& eid : lists[i])
			printf("%s\t", eid.c_str());

		printf("\n");
	}
	printf("\n");
}

void ll_payload_info_main()
{
	ELIST elist{};
	uint32_t gool_table[C3_GOOL_TABLE_SIZE];
	for (int32_t i = 0; i < C3_GOOL_TABLE_SIZE; i++)
		gool_table[i] = EID_NONE;

	if (build_read_and_parse_rebld(NULL, NULL, NULL, NULL, gool_table, elist, NULL, NULL, 1, NULL))
		return;

	build_ll_print_full_payload_info(elist, 1);
	printf("Done.\n\n");
}

// dumb thing for box counting, might be broken
void build_get_box_count(ELIST& elist)
{
	int32_t entry_count = int32_t(elist.size());
	int32_t box_counter = 0;
	int32_t nitro_counter = 0;
	int32_t entity_counter = 0;

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (build_entry_type(elist[i]) != ENTRY_TYPE_ZONE)
			continue;

		int32_t entity_count = build_get_entity_count(elist[i]._data());
		int32_t camera_count = build_get_cam_item_count(elist[i]._data());
		for (int32_t j = 0; j < entity_count; j++)
		{
			uint8_t* entity = build_get_nth_item(elist[i]._data(), (2 + camera_count + j));
			int32_t type = build_get_entity_prop(entity, ENTITY_PROP_TYPE);
			int32_t subt = build_get_entity_prop(entity, ENTITY_PROP_SUBTYPE);
			int32_t id = build_get_entity_prop(entity, ENTITY_PROP_ID);

			if (id == -1)
				continue;

			entity_counter++;

			if ((type >= 34 && type <= 43) && (subt == 0 || subt == 2 || subt == 3 || subt == 4 || subt == 6 || subt == 8 || subt == 9 || subt == 10 || subt == 11 || subt == 12 || subt == 17 || subt == 18 || subt == 23 || subt == 25 || subt == 26))
			{
				box_counter++;
			}

			if ((type >= 34 && type <= 43) && subt == 18)
			{
				nitro_counter++;
			}
		}
	}

	printf("Box count:      %4d\n", box_counter);
	printf("Nitro count:    %4d\n", nitro_counter);
}

void build_ll_analyze()
{
	ELIST elist{};
	uint32_t gool_table[C3_GOOL_TABLE_SIZE];
	for (int32_t i = 0; i < C3_GOOL_TABLE_SIZE; i++)
		gool_table[i] = EID_NONE;

	if (build_read_and_parse_rebld(NULL, NULL, NULL, NULL, gool_table, elist, NULL, NULL, 1, NULL))
		return;

	build_ll_id_usage(elist);
	build_ll_various_stats(elist);
	build_get_box_count(elist);
	build_ll_print_avg(elist);
	build_ll_print_full_payload_info(elist, 0);
	build_ll_check_gool_types(elist);
	build_ll_check_gool_references(elist, gool_table);
	build_ll_check_tpag_references(elist);
	build_ll_check_sound_references(elist);
	build_ll_check_zone_references(elist);
	build_ll_check_load_list_integrity(elist);
	build_ll_check_draw_list_integrity(elist);

	printf("Done.\n\n");
}
