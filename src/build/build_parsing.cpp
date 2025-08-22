#include "../include.h"
// reading input data (nsf/folder), parsing and storing it

/** \brief
 *  Reads the info from file the user has to provide, first part has permaloaded entries,
 *  second has a list of type/subtype dependencies
 *
 * \param permaloaded LIST*             list of permaloaded entries (created here)
 * \param subtype_info DEPENDENCIES*    list of type/subtype dependencies (created here)
 * \param file_path char *               path of the input file
 * \param elist ENTRY*                  entry list
 * \param entry_count int32_t               entry count
 * \return int32_t                          1 if all went good, 0 if something is wrong
 */
int32_t build_read_entry_config(LIST& perma, DEPENDENCIES& subinfo, DEPENDENCIES& coll, DEPENDENCIES& mus_d, ENTRY* elist, int32_t entry_count, uint32_t* gool_table, int32_t* config)
{
	int32_t remaking_load_lists_flag = config[CNFG_IDX_LL_REMAKE_FLAG];

	char* line = NULL;
	int32_t line_r_off = 0;
	int32_t line_len, read;
	char eid_buff[6];

	char fpaths[BUILD_FPATH_COUNT][MAX] = { 0 }; // paths to files, fpaths contains user-input metadata like perma list file
	build_ask_list_paths(fpaths, config);

	FILE* file = fopen(fpaths[0], "r");
	if (file == NULL)
	{
		printf("[ERROR] File with permaloaded entries could not be opened\n");
		return false;
	}

	while ((read = getline(&line, &line_len, file)) != -1)
	{
		if (line[0] == '#')
			continue;

		sscanf(line, "%5s", eid_buff);
		int32_t index = build_get_index(eid_to_int(eid_buff), elist, entry_count);
		if (index == -1)
		{
			printf("[ERROR] invalid permaloaded entry, won't proceed :\t%s\n", eid_buff);
			return false;
		}

		perma.add(eid_to_int(eid_buff));

		if (build_entry_type(elist[index]) == ENTRY_TYPE_ANIM)
		{
			LIST models = build_get_models(elist[index].data);
			perma.copy_in(models);

			for (int32_t l = 0; l < models.count(); l++)
			{
				uint32_t model = models[l];
				int32_t model_index = build_get_index(model, elist, entry_count);
				if (model_index == -1)
				{
					printf("[warning] unknown entry reference in object dependency list, will be skipped:\t %s\n", eid_conv(model, eid_buff));
					continue;
				}

				build_add_model_textures_to_list(elist[model_index].data, &perma);
			}
		}
	}
	fclose(file);

	// if making load lists
	if (remaking_load_lists_flag == 1)
	{
		file = fopen(fpaths[1], "r");
		if (file == NULL)
		{
			printf("[ERROR] File with type/subtype dependencies could not be opened\n");
			return false;
		}

		int32_t i = 0;
		int32_t subcount = 0;
		int32_t type, subtype;

		while (1)
		{
			read = getline(&line, &line_len, file);
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
				line_r_off += 7;
				int32_t index = build_get_index(eid_to_int(eid_buff), elist, entry_count);
				if (index == -1)
				{
					printf("[warning] unknown entry reference in object dependency list, will be skipped:\t %s\n", eid_buff);
					continue;
				}
				new_subt_dep.dependencies.add(eid_to_int(eid_buff));

				if (build_entry_type(elist[index]) == ENTRY_TYPE_ANIM)
				{
					LIST models = build_get_models(elist[index].data);

					for (int32_t l = 0; l < models.count(); l++)
					{
						uint32_t model = models[l];
						int32_t model_index = build_get_index(model, elist, entry_count);
						if (model_index == -1)
						{
							printf("[warning] unknown entry reference in object dependency list, will be skipped:\t %s\n", eid_conv2(model));
							continue;
						}

						new_subt_dep.dependencies.add(model);
						build_add_model_textures_to_list(elist[model_index].data, &new_subt_dep.dependencies);
					}
				}
			}
			subinfo.push_back(new_subt_dep);
		}
		fclose(file);

		file = fopen(fpaths[2], "r");
		if (file == NULL)
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
				read = getline(&line, &line_len, file);
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
					line_r_off += 7;
					int32_t index = build_get_index(eid_to_int(eid_buff), elist, entry_count);
					if (index == -1)
					{
						printf("[warning] unknown entry reference in collision dependency list, will be skipped: %s\n", eid_buff);
						continue;
					}

					new_coll_dep.dependencies.add(eid_to_int(eid_buff));

					if (build_entry_type(elist[index]) == ENTRY_TYPE_ANIM)
					{
						LIST models = build_get_models(elist[index].data);

						for (int32_t l = 0; l < models.count(); l++)
						{
							uint32_t model = models[l];
							int32_t model_index = build_get_index(model, elist, entry_count);
							if (model_index == -1)
							{
								printf("[warning] unknown entry reference in collision dependency list, will be skipped: %5s\n", eid_conv2(model));
								continue;
							}

							new_coll_dep.dependencies.add(model);
							build_add_model_textures_to_list(elist[model_index].data, &new_coll_dep.dependencies);
						}
					}
				}
				coll.push_back(new_coll_dep);
			}
			fclose(file);
		}

		file = fopen(fpaths[3], "r");
		if (file == NULL)
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

				read = getline(&line, &line_len, file);
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

					int32_t index = build_get_index(eid_to_int(eid_buf), elist, entry_count);
					if (index == -1)
					{
						printf("[warning] unknown entry reference in music ref dependency list, will be skipped: %s\n", eid_buf);
						continue;
					}

					new_mus_dep.dependencies.add(eid_to_int(eid_buf));

					if (build_entry_type(elist[index]) == ENTRY_TYPE_ANIM)
					{
						LIST models = build_get_models(elist[index].data);

						for (int32_t l = 0; l < models.count(); l++)
						{
							uint32_t model = models[l];
							int32_t model_index = build_get_index(model, elist, entry_count);
							if (model_index == -1)
							{
								printf("[warning] unknown entry reference in music ref dependency list, will be skipped: %5s\n", eid_conv2(model));
								continue;
							}

							new_mus_dep.dependencies.add(model);
							build_add_model_textures_to_list(elist[model_index].data, &new_mus_dep.dependencies);
						}
					}
				}
				mus_d.push_back(new_mus_dep);
			}			
		}
	}
	
	return true;
}

/** \brief
 *  Gets list of special entries in the zone's first item. For more info see function below.
 *
 * \param zone uint8_t*           zone to get the stuff from
 * \return LIST                         list of special entries
 */
LIST build_read_special_entries(uint8_t* zone)
{
	LIST special_entries{};
	int32_t item1off = from_u32(zone + 0x10);
	uint8_t* metadata_ptr = zone + item1off + C2_SPECIAL_METADATA_OFFSET;
	int32_t special_entry_count = from_u32(metadata_ptr) & SPECIAL_METADATA_MASK_LLCOUNT;

	for (int32_t i = 1; i <= special_entry_count; i++)
	{
		uint32_t entry = from_u32(metadata_ptr + i * 4);
		// *(uint32_t *)(metadata_ptr + i * 4) = 0;
		special_entries.add(entry);
	}

	//*(uint32_t *)metadata_ptr = 0;
	return special_entries;
}

/** \brief
 *  Adds entries specified in the zone's first item by the user. Usually entries that cannot be tied to a specific object or collision.
 *  Similar to the permaloaded and dependency list, it also checks whether the items are valid and in the case of animations adds their model
 *  and the model's texture to the list.
 *
 * \param full_load LIST*               non-delta load lists
 * \param cam_length int32_t                length of the camera path and load list array
 * \param zone ENTRY                    zone to get the stuff from
 * \return void
 */
LIST build_get_special_entries(ENTRY zone, ENTRY* elist, int32_t entry_count)
{
	LIST special_entries = build_read_special_entries(zone.data);
	LIST iteration_clone = {};
	iteration_clone.copy_in(special_entries);

	for (int32_t i = 0; i < iteration_clone.count(); i++)
	{
		char eid1[100] = "";
		char eid2[100] = "";
		char eid3[100] = "";
		int32_t item = iteration_clone[i];
		int32_t index = build_get_index(item, elist, entry_count);
		if (index == -1)
		{
			printf("[error] Zone %s special entry list contains entry %s which is not present.\n", eid_conv(zone.eid, eid1), eid_conv(item, eid2));
			special_entries.remove(item);
			continue;
		}

		if (build_entry_type(elist[index]) == ENTRY_TYPE_ANIM)
		{
			uint32_t model = build_get_model(elist[index].data, 0);
			int32_t model_index = build_get_index(model, elist, entry_count);
			if (model_index == -1 || build_entry_type(elist[model_index]) != ENTRY_TYPE_MODEL)
			{
				printf("[error] Zone %s special entry list contains animation %s that uses model %s that is not present or is not a model\n",
					eid_conv(zone.eid, eid1), eid_conv(item, eid2), eid_conv(model, eid3));
				continue;
			}

			special_entries.add(model);
			build_add_model_textures_to_list(elist[model_index].data, &special_entries);
		}
	}

	return special_entries;
}

/** \brief
 *  Gets relatives of zones.
 *
 * \param entry uint8_t*          entry data
 * \param spawns SPAWNS*                during relative collection it searches for possible spawns
 * \return uint32_t*                array of relatives relatives[count + 1], relatives[0] contains count or NULL
 */
uint32_t* build_get_zone_relatives(uint8_t* entry, SPAWNS* spawns)
{
	int32_t entity_count, item1len, relcount, camcount, neighbourcount, scencount, entry_count = 0;
	uint32_t* relatives;

	int32_t item1off = build_get_nth_item_offset(entry, 0);
	int32_t item2off = build_get_nth_item_offset(entry, 1);
	item1len = item2off - item1off;
	if (!(item1len == 0x358 || item1len == 0x318))
		return NULL;

	camcount = build_get_cam_item_count(entry);
	// if (camcount == 0) return NULL;

	entity_count = build_get_entity_count(entry);
	scencount = build_get_scen_count(entry);
	neighbourcount = build_get_neighbour_count(entry);
	relcount = camcount / 3 + neighbourcount + scencount + 1;

	if (relcount == 0)
		return NULL;

	uint32_t music_ref = build_get_zone_track(entry);
	if (music_ref != EID_NONE)
		relcount++;
	relatives = (uint32_t*)try_malloc(relcount * sizeof(uint32_t)); // freed by build_main

	relatives[entry_count++] = relcount - 1;

	for (int32_t i = 0; i < (camcount / 3); i++)
	{
		int32_t offset = build_get_nth_item_offset(entry, 2 + 3 * i);
		relatives[entry_count++] = build_get_slst(entry + offset);
	}

	LIST neighbours = build_get_neighbours(entry);
	for (int32_t i = 0; i < neighbours.count(); i++)
		relatives[entry_count++] = neighbours[i];

	for (int32_t i = 0; i < scencount; i++)
		relatives[entry_count++] = from_u32(entry + item1off + 0x4 + 0x30 * i);

	if (music_ref != EID_NONE)
		relatives[entry_count++] = music_ref;

	for (int32_t i = 0; i < entity_count; i++)
	{
		int32_t* coords_ptr;
		coords_ptr = build_seek_spawn(entry + from_u32(entry + 0x18 + 4 * camcount + 4 * i));
		if (coords_ptr != NULL && spawns != NULL)
		{
			spawns->spawn_count += 1;
			int32_t cnt = spawns->spawn_count;
			spawns->spawns = (SPAWN*)try_realloc(spawns->spawns, cnt * sizeof(SPAWN));
			spawns->spawns[cnt - 1].x = coords_ptr[0] + *(int32_t*)(entry + item2off);
			spawns->spawns[cnt - 1].y = coords_ptr[1] + *(int32_t*)(entry + item2off + 4);
			spawns->spawns[cnt - 1].z = coords_ptr[2] + *(int32_t*)(entry + item2off + 8);
			spawns->spawns[cnt - 1].zone = from_u32(entry + 0x4);
			free(coords_ptr);
		}
	}
	return relatives;
}

/** \brief
 *  Searches the entity, if it has (correct) type and subtype and coords property,
 *  returns them as int32_t[3]. Usually set to accept willy and checkpoint entities.
 *
 * \param item uint8_t*           entity data
 * \return int32_t*                         int32_t[3] with xyz coords the entity if it meets criteria or NULL
 */
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

	if (coords_offset != -1)
		if ((type == 0 && subtype == 0) || (type == 34 && subtype == 4))
		{
			int32_t* coords = (int32_t*)try_malloc(3 * sizeof(int32_t)); // freed by caller
			for (int32_t i = 0; i < 3; i++)
				coords[i] = (*(int16_t*)(item + coords_offset + 2 * i)) * 4;
			return coords;
		}

	return NULL;
}

/** \brief
 *  For each gool entry it searches related animatons and adds the model entries
 *  referenced by the animations to the gool entry's relatives.
 *
 * \param elist ENTRY*                  current entry list
 * \param entry_count int32_t               current amount of entries
 * \return void
 */
void build_get_model_references(ENTRY* elist, int32_t entry_count)
{
	uint32_t new_relatives[250];
	for (int32_t i = 0; i < entry_count; i++)
	{
		if ((elist[i].related != NULL) && (from_u32(elist[i].data + 8) == ENTRY_TYPE_GOOL))
		{
			int32_t relative_index;
			int32_t new_counter = 0;
			for (int32_t j = 0; (unsigned)j < elist[i].related[0]; j++)
				if ((relative_index = build_get_index(elist[i].related[j + 1], elist, entry_count)) >= 0)
					if (elist[relative_index].data != NULL && (from_u32(elist[relative_index].data + 8) == 1))
						new_relatives[new_counter++] = build_get_model(elist[relative_index].data, 0);

			if (new_counter)
			{
				int32_t relative_count;
				relative_count = elist[i].related[0];
				elist[i].related = (uint32_t*)try_realloc(elist[i].related, (relative_count + new_counter + 1) * sizeof(uint32_t));
				for (int32_t j = 0; j < new_counter; j++)
					elist[i].related[j + relative_count + 1] = new_relatives[j];
				elist[i].related[0] += new_counter;
			}
		}
	}
}

/** \brief
 *  For each entry zone's cam path it calculates its distance from the selected spawn (in cam paths) according
 *  to level's path links.
 *  Heuristic for determining whether a path is before or after another path relative to current spawn.
 *
 * \param elist ENTRY*                  list of entries
 * \param entry_count int32_t               entry count
 * \param spawns SPAWNS                 struct with found/potential spawns
 * \return void
 */
void build_get_distance_graph(ENTRY* elist, int32_t entry_count, SPAWNS spawns)
{
	char eid1[100] = "";
	char eid2[100] = "";

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE)
		{
			int32_t cam_count = build_get_cam_item_count(elist[i].data) / 3;
			elist[i].distances = (uint32_t*)try_malloc(cam_count * sizeof(uint32_t)); // freed by build_main
			elist[i].visited = (bool*)try_malloc(cam_count * sizeof(bool));           // freed by build_main

			for (int32_t j = 0; j < cam_count; j++)
			{
				elist[i].distances[j] = INT_MAX;
				elist[i].visited[j] = false;
			}
		}
		else
		{
			elist[i].distances = NULL;
			elist[i].visited = NULL;
		}
	}

	DIST_GRAPH_Q graph = graph_init();
	int32_t start_index = build_get_index(spawns.spawns[0].zone, elist, entry_count);
	graph_add(&graph, elist, start_index, 0);

	while (1)
	{
		int32_t top_zone;
		int32_t top_cam;
		graph_pop(&graph, &top_zone, &top_cam);
		if (top_zone == -1 && top_cam == -1)
			break;

		LIST links = build_get_links(elist[top_zone].data, 2 + 3 * top_cam);
		for (int32_t i = 0; i < links.count(); i++)
		{
			CAMERA_LINK link = int_to_link(links[i]);
			int32_t neighbour_count = build_get_neighbour_count(elist[top_zone].data);
			uint32_t* neighbours = (uint32_t*)try_malloc(neighbour_count * sizeof(uint32_t)); // freed here
			int32_t item1off = from_u32(elist[top_zone].data + 0x10);
			for (int32_t j = 0; j < neighbour_count; j++)
				neighbours[j] = from_u32(elist[top_zone].data + item1off + C2_NEIGHBOURS_START + 4 + 4 * j);
			int32_t neighbour_index = build_get_index(neighbours[link.zone_index], elist, entry_count);
			if (neighbour_index == -1)
			{
				printf("[warning] %s references %s that does not seem to be present (link %d neighbour %d)\n",
					eid_conv(elist[top_zone].eid, eid1), eid_conv(neighbours[link.zone_index], eid2), i, link.zone_index);
				continue;
			}
			int32_t path_count = build_get_cam_item_count(elist[neighbour_index].data) / 3;
			if (link.cam_index >= path_count)
			{
				printf("[warning] %s links to %s's cam path %d which doesnt exist (link %d neighbour %d)\n",
					eid_conv(elist[top_zone].eid, eid1), eid_conv(neighbours[link.zone_index], eid2), link.cam_index, i, link.zone_index);
				continue;
			}

			if (elist[neighbour_index].visited[link.cam_index] == 0)
				graph_add(&graph, elist, neighbour_index, link.cam_index);
			free(neighbours);
		}
	}
}

/** \brief
 *  Removes references to entries that are only in the base level.
 *
 * \param elist ENTRY*                  list of entries
 * \param entry_count int32_t               current amount of entries
 * \param entry_count_base int32_t
 * \return void
 */
void build_remove_invalid_references(ENTRY* elist, int32_t entry_count, int32_t entry_count_base)
{
	uint32_t new_relatives[250];

	for (int32_t i = 0; i < entry_count; i++)
	{
		if (elist[i].related == NULL)
			continue;
		for (int32_t j = 1; (unsigned)j < elist[i].related[0] + 1; j++)
		{
			int32_t relative_index;
			relative_index = build_get_index(elist[i].related[j], elist, entry_count);
			if (relative_index < entry_count_base)
				elist[i].related[j] = 0;
			if (elist[i].related[j] == elist[i].eid)
				elist[i].related[j] = 0;

			for (int32_t k = j + 1; (unsigned)k < elist[i].related[0] + 1; k++)
				if (elist[i].related[j] == elist[i].related[k])
					elist[i].related[k] = 0;
		}

		int32_t counter = 0;
		for (int32_t j = 1; (unsigned)j < elist[i].related[0] + 1; j++)
			if (elist[i].related[j] != 0)
				new_relatives[counter++] = elist[i].related[j];

		if (counter == 0)
		{
			free(elist[i].related);
			elist[i].related = NULL;
			continue;
		}

		elist[i].related = (uint32_t*)try_realloc(elist[i].related, (counter + 1) * sizeof(uint32_t));
		elist[i].related[0] = counter;
		for (int32_t j = 0; j < counter; j++)
			elist[i].related[j + 1] = new_relatives[j];
	}
}

// parsing input info for rebuilding from a nsf file
int32_t build_read_and_parse_rebld(int32_t* level_ID, FILE** nsfnew, FILE** nsd, int32_t* chunk_border_texture, uint32_t* gool_table,
	ENTRY* elist, int32_t* entry_count, uint8_t** chunks, SPAWNS* spawns, bool stats_only, char* fpath)
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

	uint8_t* buffer = (uint8_t*)calloc(CHUNKSIZE, sizeof(uint8_t));
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
			elist[*entry_count].eid = from_u32(buffer + 4);
			elist[*entry_count].chunk = lcl_chunk_border_texture;
			// elist[*entry_count].og_chunk = i;
			elist[*entry_count].data = NULL;
			elist[*entry_count].related = NULL;
			elist[*entry_count].visited = NULL;
			elist[*entry_count].distances = NULL;
			*entry_count += 1;
			lcl_chunk_border_texture++;
			qsort(elist, *entry_count, sizeof(ENTRY), cmp_func_eid);
		}
		else
		{
			int32_t chunk_entry_count = from_u32(buffer + 0x8);
			for (int32_t j = 0; j < chunk_entry_count; j++)
			{
				int32_t start_offset = build_get_nth_item_offset(buffer, j);
				int32_t end_offset = build_get_nth_item_offset(buffer, j + 1);
				int32_t entry_size = end_offset - start_offset;

				elist[*entry_count].eid = from_u32(buffer + start_offset + 0x4);
				elist[*entry_count].esize = entry_size;
				elist[*entry_count].related = NULL;
				elist[*entry_count].visited = NULL;
				elist[*entry_count].distances = NULL;
				elist[*entry_count].data = (uint8_t*)try_malloc(entry_size); // freed by build_main
				memcpy(elist[*entry_count].data, buffer + start_offset, entry_size);

				if (!stats_only || build_entry_type(elist[*entry_count]) == ENTRY_TYPE_SOUND)
					elist[*entry_count].chunk = -1;
				else
					elist[*entry_count].chunk = i;

				if (build_entry_type(elist[*entry_count]) == ENTRY_TYPE_ZONE)
					build_check_item_count(elist[*entry_count].data, elist[*entry_count].eid);
				if (build_entry_type(elist[*entry_count]) == ENTRY_TYPE_ZONE)
					elist[*entry_count].related = build_get_zone_relatives(elist[*entry_count].data, spawns);

				if (build_entry_type(elist[*entry_count]) == ENTRY_TYPE_GOOL && build_item_count(elist[*entry_count].data) == 6)
				{
					int32_t item1_offset = *(int32_t*)(elist[*entry_count].data + 0x10);
					int32_t gool_type = *(int32_t*)(elist[*entry_count].data + item1_offset);
					if (/*gool_type >= C3_GOOL_TABLE_SIZE || */ gool_type < 0)
					{
						printf("[warning] GOOL entry %s has invalid type specified in the third item (%2d)!\n", eid_conv2(elist[*entry_count].eid), gool_type);
						continue;
					}
					if (gool_table != NULL && gool_table[gool_type] == EID_NONE)
						gool_table[gool_type] = elist[*entry_count].eid;
				}

				*entry_count += 1;
				qsort(elist, *entry_count, sizeof(ENTRY), cmp_func_eid);
			}
		}
	}
	free(buffer);
	fclose(nsf);
	if (chunk_border_texture != NULL)
		*chunk_border_texture = lcl_chunk_border_texture;
	return false;
}
