
// reading input data (nsf/folder), parsing and storing it

#include "../include.h"
#include "../utils/utils.hpp"
#include "../utils/entry.hpp"

// Reads the info from file the user has to provide, first part has permaloaded entries,
// second has a list of type/subtype dependencies
int32_t build_read_entry_config(LIST& perma, DEPENDENCIES& subinfo, DEPENDENCIES& coll, DEPENDENCIES& mus_d, ELIST& elist, uint32_t* gool_table, int32_t* config)
{
	int32_t remaking_load_lists_flag = config[CNFG_IDX_LL_REMAKE_FLAG];

	char* line = NULL;
	int32_t line_r_off = 0;
	int32_t line_len, read;
	char eid_buff[6];

	char fpaths[BUILD_FPATH_COUNT][MAX] = { 0 }; // paths to files, fpaths contains user-input metadata like perma list file
	build_ask_list_paths(fpaths, config);

	File file0(fpaths[0], "r");
	if (file0.f == NULL)
	{
		printf("[ERROR] File with permaloaded entries could not be opened\n");
		return false;
	}

	while ((read = getline(&line, &line_len, file0.f)) != -1)
	{
		if (line[0] == '#')
			continue;

		sscanf(line, "%5s", eid_buff);
		int32_t index = elist.get_index(eid_to_int(eid_buff));
		if (index == -1)
		{
			printf("[ERROR] invalid permaloaded entry, won't proceed :\t%s\n", eid_buff);
			return false;
		}

		perma.add(eid_to_int(eid_buff));

		if (elist[index].entry_type() == ENTRY_TYPE_ANIM)
		{
			LIST models = elist[index].get_models();
			perma.copy_in(models);

			for (auto& model : models)
			{
				int32_t model_index = elist.get_index(model);
				if (model_index == -1)
				{
					printf("[warning] unknown entry reference in object dependency list, will be skipped:\t %s\n", 
						eid2str(model));
					continue;
				}

				build_add_model_textures_to_list(elist[model_index]._data(), &perma);
			}
		}
	}

	// if making load lists
	if (remaking_load_lists_flag == 1)
	{
		File file1(fpaths[1], "r");
		if (file1.f == NULL)
		{
			printf("[ERROR] File with type/subtype dependencies could not be opened\n");
			return false;
		}

		int32_t i = 0;
		int32_t subcount = 0;
		int32_t type, subtype;

		while (1)
		{
			read = getline(&line, &line_len, file1.f);
			if (read == -1)
			{
				break;
			}

			if (line[0] == '#')
				continue;

			if (2 > sscanf(line, "%d, %d", &type, &subtype))
				break;

			DEPENDENCY new_subt_dep{ type, subtype };
			new_subt_dep.dependencies.add(gool_table[type]);

			line_r_off = 6;

			while (1)
			{
				if (sscanf(line + line_r_off, ", %5[^\n]", eid_buff) < 1)
					break;
				eid_buff[5] = 0;
				line_r_off += 7;
				int32_t index = elist.get_index(eid_to_int(eid_buff));
				if (index == -1)
				{
					printf("[warning] unknown entry reference in object dependency list, will be skipped:\t %s\n", eid_buff);
					continue;
				}
				new_subt_dep.dependencies.add(eid_to_int(eid_buff));

				if (elist[index].entry_type() == ENTRY_TYPE_ANIM)
				{
					for (auto& model : elist[index].get_models())
					{
						int32_t model_index = elist.get_index(model);
						if (model_index == -1)
						{
							printf("[warning] unknown entry reference in object dependency list, will be skipped:\t %s\n", 
								eid2str(model));
							continue;
						}

						new_subt_dep.dependencies.add(model);
						build_add_model_textures_to_list(elist[model_index]._data(), &new_subt_dep.dependencies);
					}
				}
			}
			subinfo.push_back(new_subt_dep);
		}

		File file2(fpaths[2], "r");
		if (file2.f == NULL)
		{
			printf("\nFile with collision dependencies could not be opened\n");
			printf("Assuming file is not necessary\n");
		}
		else
		{
			int32_t coll_count = 0;
			int32_t code;

			while (1)
			{
				read = getline(&line, &line_len, file2.f);
				if (read == -1)
				{
					break;
				}

				if (line[0] == '#')
					continue;

				if (1 > sscanf(line, "%x", &code))
					break;

				DEPENDENCY new_coll_dep(type, -1);

				line_r_off = 4;
				while (1)
				{
					if (sscanf(line + line_r_off, ", %5[^\n]", eid_buff) < 1)
						break;
					eid_buff[5] = 0;
					line_r_off += 7;
					int32_t index = elist.get_index(eid_to_int(eid_buff));
					if (index == -1)
					{
						printf("[warning] unknown entry reference in collision dependency list, will be skipped: %s\n", eid_buff);
						continue;
					}

					new_coll_dep.dependencies.add(eid_to_int(eid_buff));

					if (elist[index].entry_type() == ENTRY_TYPE_ANIM)
					{
						for (auto& model : elist[index].get_models())
						{
							int32_t model_index = elist.get_index(model);
							if (model_index == -1)
							{
								printf("[warning] unknown entry reference in collision dependency list, will be skipped: %5s\n",
									eid2str(model));
								continue;
							}

							new_coll_dep.dependencies.add(model);
							build_add_model_textures_to_list(elist[model_index]._data(), &new_coll_dep.dependencies);
						}
					}
				}
				coll.push_back(new_coll_dep);
			}
		}

		File file3(fpaths[3], "r");
		if (file3.f == NULL)
		{
			printf("\nFile with music entry dependencies could not be opened\n");
			printf("Assuming file is not necessary\n");
		}
		else
		{
			int32_t mus_d_count = 0;
			char eid_buf[6] = "";
			while (1)
			{

				read = getline(&line, &line_len, file3.f);
				if (read == -1)
					break;

				if (line[0] == '#')
					continue;

				if (1 > sscanf(line, "%5s", eid_buf))
					break;

				DEPENDENCY new_mus_dep(eid_to_int(eid_buf), -1);
				line_r_off = 5;

				while (1)
				{
					if (sscanf(line + line_r_off, ", %5[^\n]", eid_buf) < 1)
						break;
					line_r_off += 7;

					int32_t index = elist.get_index(eid_to_int(eid_buf));
					if (index == -1)
					{
						printf("[warning] unknown entry reference in music ref dependency list, will be skipped: %s\n", eid_buf);
						continue;
					}

					new_mus_dep.dependencies.add(eid_to_int(eid_buf));

					if (elist[index].entry_type() == ENTRY_TYPE_ANIM)
					{
						for (auto& model : elist[index].get_models())
						{
							int32_t model_index = elist.get_index(model);
							if (model_index == -1)
							{
								printf("[warning] unknown entry reference in music ref dependency list, will be skipped: %5s\n", 
									eid2str(model));
								continue;
							}

							new_mus_dep.dependencies.add(model);
							build_add_model_textures_to_list(elist[model_index]._data(), &new_mus_dep.dependencies);
						}
					}
				}
				mus_d.push_back(new_mus_dep);
			}
		}
	}

	return true;
}

//  Gets relatives of zones.
void build_get_zone_relatives(ENTRY& ntry, SPAWNS* spawns)
{
	auto entry = ntry._data();
	int32_t item0off = ntry.get_nth_item_offset(0);
	int32_t item1off = ntry.get_nth_item_offset(1);
	int32_t item0len = item1off - item0off;

	if (!(item0len == 0x358 || item0len == 0x318))
	{
		printf("[warning] Zone %s item0 size 0x%X ??\n", ntry.ename, item0len);
		return;
	}

	int32_t cam_count = ntry.cam_item_count() / 3;
	for (int32_t i = 0; i < cam_count; i++)
	{
		uint8_t* item = ntry.get_nth_main_cam(i);
		ntry.related.add(build_get_slst(item));
	}

	ntry.related.copy_in(ntry.get_neighbours());
	ntry.related.copy_in(ntry.get_sceneries());

	uint32_t music_ref = ntry.get_zone_track();
	if (music_ref != EID_NONE)
		ntry.related.add(music_ref);

	int32_t entity_count = ntry.entity_count();
	for (int32_t i = 0; i < entity_count; i++)
	{
		int32_t* coords_ptr = build_seek_spawn(entry + from_u32(entry + 0x18 + 12 * cam_count + 4 * i));
		if (coords_ptr != NULL && spawns != NULL)
		{
			SPAWN new_spawn{};
			new_spawn.zone = from_u32(entry + 0x4);
			new_spawn.x = coords_ptr[0] + *(int32_t*)(entry + item1off);
			new_spawn.y = coords_ptr[1] + *(int32_t*)(entry + item1off + 4);
			new_spawn.z = coords_ptr[2] + *(int32_t*)(entry + item1off + 8);
			spawns->push_back(new_spawn);
			free(coords_ptr);
		}
	}
}

// if entity has willy/check type and subtype and coords property, returns as int32_t[3]
int32_t* build_seek_spawn(uint8_t* item)
{
	int32_t code, offset, type = -1, subtype = -1, coords_offset = -1;
	for (int32_t i = 0; (unsigned)i < from_u32(item + 0xC); i++)
	{
		code = from_u16(item + 0x10 + 8 * i);
		offset = from_u16(item + 0x12 + 8 * i) + OFFSET;
		if (code == ENTITY_PROP_TYPE)
			type = from_u32(item + offset + 4);
		if (code == ENTITY_PROP_SUBTYPE)
			subtype = from_u32(item + offset + 4);
		if (code == ENTITY_PROP_PATH)
			coords_offset = offset + 4;
	}

	if (coords_offset == -1)
		return NULL;

	if ((type == 0 && subtype == 0) || (type == 34 && subtype == 4))
	{
		int32_t* coords = (int32_t*)try_malloc(3 * sizeof(int32_t)); // freed by caller
		for (int32_t i = 0; i < 3; i++)
			coords[i] = (*(int16_t*)(item + coords_offset + 2 * i)) * 4;
		return coords;
	}

	return NULL;
}


// parsing input info for rebuilding from a nsf file
int32_t build_read_and_parse_rebld(int32_t* level_ID, FILE** nsfnew, FILE** nsd, int32_t* chunk_border_texture, uint32_t* gool_table,
	ELIST& elist, uint8_t** chunks, SPAWNS* spawns, bool stats_only, const char* fpath)
{
	FILE* nsf = NULL;
	char nsfpath[MAX], fname[MAX + 20];

	if (fpath)
	{
		if ((nsf = fopen(fpath, "rb")) == NULL)
		{
			printf("[ERROR] Could not open selected NSF\n\n");
			return true;
		}
	}
	else
	{
		if (!stats_only)
		{
			printf("Input the path to the level (.nsf) you want to rebuild:\n");
			scanf(" %[^\n]", nsfpath);
			path_fix(nsfpath);

			if ((nsf = fopen(nsfpath, "rb")) == NULL)
			{
				printf("[ERROR] Could not open selected NSF\n\n");
				return true;
			}

			*level_ID = build_ask_ID();

			*(strrchr(nsfpath, '\\') + 1) = '\0';
			sprintf(fname, "%s\\S00000%02X.NSF", nsfpath, *level_ID);
			*nsfnew = fopen(fname, "wb");
			*(strchr(fname, '\0') - 1) = 'D';
			*nsd = fopen(fname, "wb");
		}
		else
		{
			printf("Input the path to the level (.nsf):\n");
			scanf(" %[^\n]", nsfpath);
			path_fix(nsfpath);

			if ((nsf = fopen(nsfpath, "rb")) == NULL)
			{
				printf("[ERROR] Could not open selected NSF\n\n");
				return true;
			}
		}
	}

	int32_t nsf_chunk_count = build_get_chunk_count_base(nsf);
	int32_t lcl_chunk_border_texture = 0;

	static uint8_t buffer[CHUNKSIZE]{};
	for (int32_t i = 0; i < nsf_chunk_count; i++)
	{
		fread(buffer, sizeof(uint8_t), CHUNKSIZE, nsf);
		uint32_t checksum_calc = nsfChecksum(buffer);
		uint32_t checksum_used = *(uint32_t*)(buffer + CHUNK_CHECKSUM_OFFSET);
		if (checksum_calc != checksum_used)
			printf("Chunk %3d has invalid checksum\n", 2 * i + 1);
		if (build_chunk_type(buffer) == CHUNK_TYPE_TEXTURE)
		{
			if (chunks != NULL)
			{
				chunks[lcl_chunk_border_texture] = (uint8_t*)try_calloc(CHUNKSIZE, sizeof(uint8_t)); // freed by build_main
				memcpy(chunks[lcl_chunk_border_texture], buffer, CHUNKSIZE);
			}
			ENTRY tpage{};
			tpage.is_tpage = true;
			tpage.eid = from_u32(buffer + 4);
			strncpy(tpage.ename, ENTRY::eid2s(tpage.eid).c_str(), 5);
			tpage.chunk = lcl_chunk_border_texture;
			lcl_chunk_border_texture++;
			elist.push_back(tpage);
			elist.sort_by_eid();
		}
		else
		{			
			int32_t chunk_entry_count = from_u32(buffer + 0x8);
			for (int32_t j = 0; j < chunk_entry_count; j++)
			{
				int32_t start_offset = ENTRY::get_nth_item_offset(buffer, j);
				int32_t end_offset = ENTRY::get_nth_item_offset(buffer, j + 1);
				int32_t entry_size = end_offset - start_offset;

				ENTRY ntry{};
				ntry.eid = from_u32(buffer + start_offset + 0x4);
				strncpy(ntry.ename, ENTRY::eid2s(ntry.eid).c_str(), 5);
				ntry.esize = entry_size;
				ntry.data.resize(entry_size);
				memcpy(ntry._data(), buffer + start_offset, entry_size);

				if (!stats_only || ntry.entry_type() == ENTRY_TYPE_SOUND)
					ntry.chunk = -1;
				else
					ntry.chunk = i;

				if (ntry.entry_type() == ENTRY_TYPE_ZONE)
				{
					if (!ntry.check_item_count())
						return false;
					build_get_zone_relatives(ntry, spawns);
				}

				if (ntry.entry_type() == ENTRY_TYPE_GOOL && ntry.item_count() == 6)
				{
					int32_t item1_offset = *(int32_t*)(ntry._data() + 0x10);
					int32_t gool_type = *(int32_t*)(ntry._data() + item1_offset);
					if (gool_type < 0)
					{
						printf("[warning] GOOL entry %s has invalid type specified in the third item (%2d)!\n", ntry.ename, gool_type);
						continue;
					}
					if (gool_table != NULL && gool_table[gool_type] == EID_NONE)
						gool_table[gool_type] = ntry.eid;
				}

				elist.push_back(ntry);
				elist.sort_by_eid();
			}
		}
	}
	fclose(nsf);
	if (chunk_border_texture != NULL)
		*chunk_border_texture = lcl_chunk_border_texture;
	return false;
}
