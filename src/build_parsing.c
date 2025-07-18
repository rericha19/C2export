#include "macros.h"
// reading input data (nsf/folder), parsing and storing it

/** \brief
 *  Reads the nsf, puts it straight into chunks, does not collect references or anything.
 *  Collects relatives. Likely slightly deprecate.
 *
 * \param elist ENTRY*                  list of entries
 * \param chunk_border_base int32_t         chunk count of the base nsf
 * \param chunks uint8_t**        chunk array, base level gets put straight into them
 * \param chunk_border_texture int32_t*     index of the last texture chunk
 * \param entry_count int32_t*              entry count
 * \param nsf FILE*                     nsf file
 * \param gool_table uint32_t*      gool table gets created, contains GOOL entries associated with the type/index they are on
 * \return void
 */
void build_read_nsf(ENTRY *elist, int32_t chunk_border_base, uint8_t **chunks, int32_t *chunk_border_texture, int32_t *entry_count, FILE *nsf, uint32_t *gool_table)
{
    int32_t i, j, offset;
    uint8_t chunk[CHUNKSIZE];

    for (i = 0; i < chunk_border_base; i++)
    {
        fread(chunk, CHUNKSIZE, sizeof(uint8_t), nsf);
        chunks[*chunk_border_texture] = (uint8_t *)calloc(CHUNKSIZE, sizeof(uint8_t)); // freed by build_main
        memcpy(chunks[*chunk_border_texture], chunk, CHUNKSIZE);
        (*chunk_border_texture)++;
        if (chunk[2] != 1)
            for (j = 0; j < chunk[8]; j++)
            {
                offset = *(int32_t *)(chunk + 0x10 + j * 4);
                elist[*entry_count].eid = from_u32(chunk + offset + 4);
                elist[*entry_count].chunk = i;
                elist[*entry_count].esize = -1;
                if (*(int32_t *)(chunk + offset + 8) == ENTRY_TYPE_GOOL && build_item_count(chunk + offset) == 6)
                {
                    int32_t item1_offset = *(int32_t *)(chunk + offset + 0x10);
                    gool_table[*(int32_t *)(chunk + offset + item1_offset)] = elist[*entry_count].eid;
                }
                (*entry_count)++;
            }
    }
}

/** \brief
 *  For each valid file it adds it to the nsf(texture) or to the entry list.
 *  Collects relatives.
 *
 * \param df DIR*                       directory/folder
 * \param dirpath char *                 path to the directory/folder
 * \param chunks uint8_t**        array of 64kB chunks
 * \param elist ENTRY*                  entry list
 * \param chunk_border_texture int32_t*     max index of a texture chunk
 * \param entry_count int32_t*              entry count
 * \param spawns SPAWNS*                spawns, spawns get updated here
 * \param gool_table uint32_t*      table that contains GOOL entries on indexes that correspond to their types
 * \return void
 */
void build_read_folder(DIR *df, char *dirpath, uint8_t **chunks, ENTRY *elist, int32_t *chunk_border_texture, int32_t *entry_count, SPAWNS *spawns, uint32_t *gool_table)
{
    struct dirent *de;
    char temp[500];
    FILE *file = NULL;
    int32_t fsize;
    uint8_t entry[CHUNKSIZE];

    while ((de = readdir(df)) != NULL)
        if ((de->d_name)[0] != '.')
        {
            sprintf(temp, "%s\\%s", dirpath, de->d_name);
            if (file != NULL)
            {
                fclose(file);
                file = NULL;
            }
            if ((file = fopen(temp, "rb")) == NULL)
                continue;
            fseek(file, 0, SEEK_END);
            fsize = ftell(file);
            rewind(file);
            fread(entry, fsize, sizeof(uint8_t), file);
            if (fsize == CHUNKSIZE && from_u16(entry + 0x2) == CHUNK_TYPE_TEXTURE)
            {
                if (build_get_base_chunk_border(from_u32(entry + 4), chunks, *chunk_border_texture) > 0)
                    continue;
                chunks[*chunk_border_texture] = (uint8_t *)calloc(CHUNKSIZE, sizeof(uint8_t)); // freed by build_main
                memcpy(chunks[*chunk_border_texture], entry, CHUNKSIZE);
                elist[*entry_count].eid = from_u32(entry + 4);
                elist[*entry_count].chunk = *chunk_border_texture;
                elist[*entry_count].data = NULL;
                elist[*entry_count].visited = NULL;
                elist[*entry_count].related = NULL;
                elist[*entry_count].distances = NULL;
                (*entry_count)++;
                (*chunk_border_texture)++;
                continue;
            }

            if (from_u32(entry) != MAGIC_ENTRY)
                continue;
            if (build_get_index(from_u32(entry + 4), elist, *entry_count) >= 0)
                continue;

            elist[*entry_count].eid = from_u32(entry + 4);
            elist[*entry_count].chunk = -1;
            elist[*entry_count].esize = fsize;

            if (entry[8] == ENTRY_TYPE_ZONE)
                build_check_item_count(entry, elist[*entry_count].eid);
            if (entry[8] == ENTRY_TYPE_ZONE)
                elist[*entry_count].related = build_get_zone_relatives(entry, spawns);
            if (entry[8] == ENTRY_TYPE_GOOL && build_item_count(entry) == 6)
                elist[*entry_count].related = build_get_gool_relatives(entry, fsize);

            elist[*entry_count].data = (uint8_t *)malloc(fsize * sizeof(uint8_t)); // freed by build_main at its end
            memcpy(elist[*entry_count].data, entry, fsize);
            if (build_entry_type(elist[*entry_count]) == ENTRY_TYPE_GOOL && *(elist[*entry_count].data + 8) > 3)
            {
                int32_t item1_offset = *(int32_t *)(elist[*entry_count].data + 0x10);
                int32_t gool_type = *(int32_t *)(elist[*entry_count].data + item1_offset);
                if (/*gool_type >= C2_GOOL_TABLE_SIZE || */ gool_type < 0)
                {
                    printf("[warning] GOOL entry %s has invalid type specified in the first item (%2d)!\n", eid_conv(elist[*entry_count].eid, temp), gool_type);
                    continue;
                }
                gool_table[gool_type] = elist[*entry_count].eid;
            }
            (*entry_count)++;
            qsort(elist, *entry_count, sizeof(ENTRY), cmp_func_eid);
        }

    fclose(file);
}

// initialise dependency struct
DEPENDENCIES build_init_dep()
{
    DEPENDENCIES dep;
    dep.array = NULL;
    dep.count = 0;

    return dep;
}

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
int32_t build_read_entry_config(LIST *permaloaded, DEPENDENCIES *subtype_info, DEPENDENCIES *collisions, DEPENDENCIES *music_deps, ENTRY *elist, int32_t entry_count, uint32_t *gool_table, int32_t *config)
{

    int32_t remaking_load_lists_flag = config[CNFG_IDX_LL_REMAKE_FLAG];
    int32_t valid = 1;

    char *line = NULL;
    int32_t line_r_off = 0;
    int32_t line_len, read;
    char temp[6];

    char fpaths[BUILD_FPATH_COUNT][MAX] = {0}; // paths to files, fpaths contains user-input metadata like perma list file
    build_ask_list_paths(fpaths, config);

    LIST perma = init_list();
    DEPENDENCIES coll;
    coll.count = 0;
    coll.array = NULL;

    DEPENDENCIES subinfo;
    subinfo.count = 0;
    subinfo.array = NULL;

    DEPENDENCIES mus_d;
    mus_d.count = 0;
    mus_d.array = NULL;

    FILE *file = fopen(fpaths[0], "r");
    if (file == NULL)
    {
        printf("[ERROR] File with permaloaded entries could not be opened\n");
        return 0;
    }

    while ((read = getline(&line, &line_len, file)) != -1)
    {

        if (line[0] == '#')
            continue;

        sscanf(line, "%5s", temp);
        int32_t index = build_get_index(eid_to_int(temp), elist, entry_count);
        if (index == -1)
        {
            printf("[ERROR] invalid permaloaded entry, won't proceed :\t%s\n", temp);
            valid = 0;
            continue;
        }

        list_add(&perma, eid_to_int(temp));

        if (build_entry_type(elist[index]) == ENTRY_TYPE_ANIM)
        {
            LIST models = build_get_models(elist[index].data);
            list_copy_in(&perma, models);

            for (int32_t l = 0; l < models.count; l++)
            {
                uint32_t model = models.eids[l];
                int32_t model_index = build_get_index(model, elist, entry_count);
                if (model_index == -1)
                {
                    printf("[warning] unknown entry reference in object dependency list, will be skipped:\t %s\n", eid_conv(model, temp));
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
            return 0;
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

            i = subcount;
            subcount++;
            if (subcount == 1)
                subinfo.array = (DEPENDENCY *)malloc(subcount * sizeof(DEPENDENCY)); // freed by caller
            else
                subinfo.array = (DEPENDENCY *)realloc(subinfo.array, subcount * sizeof(DEPENDENCY));

            subinfo.array[i].type = type;
            subinfo.array[i].subtype = subtype;
            subinfo.array[i].dependencies = init_list();
            list_add(&subinfo.array[i].dependencies, gool_table[type]);

            line_r_off = 6;

            while (1)
            {
                if (sscanf(line + line_r_off, ", %5[^\n]", temp) < 1)
                    break;
                line_r_off += 7;
                int32_t index = build_get_index(eid_to_int(temp), elist, entry_count);
                if (index == -1)
                {
                    printf("[warning] unknown entry reference in object dependency list, will be skipped:\t %s\n", temp);
                    continue;
                }
                list_add(&(subinfo.array[i].dependencies), eid_to_int(temp));

                if (build_entry_type(elist[index]) == ENTRY_TYPE_ANIM)
                {
                    LIST models = build_get_models(elist[index].data);

                    for (int32_t l = 0; l < models.count; l++)
                    {
                        uint32_t model = models.eids[l];
                        int32_t model_index = build_get_index(model, elist, entry_count);
                        if (model_index == -1)
                        {
                            printf("[warning] unknown entry reference in object dependency list, will be skipped:\t %s\n", eid_conv(model, temp));
                            continue;
                        }

                        list_add(&subinfo.array[i].dependencies, model);
                        build_add_model_textures_to_list(elist[model_index].data, &subinfo.array[i].dependencies);
                    }
                }
            }
        }
        subinfo.count = subcount;
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

                i = coll_count;
                coll_count++;
                if (coll_count == 1)
                    coll.array = (DEPENDENCY *)malloc(coll_count * sizeof(DEPENDENCY)); // freed by caller
                else
                    coll.array = (DEPENDENCY *)realloc(coll.array, coll_count * sizeof(DEPENDENCY));
                coll.array[i].type = code;
                coll.array[i].subtype = -1;
                coll.array[i].dependencies = init_list();

                line_r_off = 4;
                while (1)
                {
                    if (sscanf(line + line_r_off, ", %5[^\n]", temp) < 1)
                        break;
                    line_r_off += 7;
                    int32_t index = build_get_index(eid_to_int(temp), elist, entry_count);
                    if (index == -1)
                    {
                        printf("[warning] unknown entry reference in collision dependency list, will be skipped: %s\n", temp);
                        continue;
                    }

                    list_add(&(coll.array[i].dependencies), eid_to_int(temp));

                    if (build_entry_type(elist[index]) == ENTRY_TYPE_ANIM)
                    {
                        LIST models = build_get_models(elist[index].data);

                        for (int32_t l = 0; l < models.count; l++)
                        {
                            uint32_t model = models.eids[l];
                            int32_t model_index = build_get_index(model, elist, entry_count);
                            if (model_index == -1)
                            {
                                printf("[warning] unknown entry reference in collision dependency list, will be skipped: %5s\n",
                                       eid_conv(model, temp));
                                continue;
                            }

                            list_add(&coll.array[i].dependencies, model);
                            build_add_model_textures_to_list(elist[model_index].data, &coll.array[i].dependencies);
                        }
                    }
                }
            }
            coll.count = coll_count;
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
            char temp[6] = "";
            while (1)
            {

                read = getline(&line, &line_len, file);
                if (read == -1)
                    break;

                if (line[0] == '#')
                    continue;

                if (1 > sscanf(line, "%5s", temp))
                    break;

                i = mus_d_count;
                mus_d_count++;
                if (mus_d_count == 1)
                    mus_d.array = (DEPENDENCY *)malloc(mus_d_count * sizeof(DEPENDENCY)); // freed by caller
                else
                    mus_d.array = (DEPENDENCY *)realloc(mus_d.array, mus_d_count * sizeof(DEPENDENCY));

                mus_d.array[i].type = eid_to_int(temp);
                mus_d.array[i].subtype = -1;
                mus_d.array[i].dependencies = init_list();

                line_r_off = 5;

                while (1)
                {
                    if (sscanf(line + line_r_off, ", %5[^\n]", temp) < 1)
                        break;
                    line_r_off += 7;

                    int32_t index = build_get_index(eid_to_int(temp), elist, entry_count);
                    if (index == -1)
                    {
                        printf("[warning] unknown entry reference in music ref dependency list, will be skipped: %s\n", temp);
                        continue;
                    }

                    list_add(&(mus_d.array[i].dependencies), eid_to_int(temp));

                    if (build_entry_type(elist[index]) == ENTRY_TYPE_ANIM)
                    {
                        LIST models = build_get_models(elist[index].data);

                        for (int32_t l = 0; l < models.count; l++)
                        {
                            uint32_t model = models.eids[l];
                            int32_t model_index = build_get_index(model, elist, entry_count);
                            if (model_index == -1)
                            {
                                printf("[warning] unknown entry reference in music ref dependency list, will be skipped: %5s\n",
                                       eid_conv(model, temp));
                                continue;
                            }

                            list_add(&mus_d.array[i].dependencies, model);
                            build_add_model_textures_to_list(elist[model_index].data, &mus_d.array[i].dependencies);
                        }
                    }
                }
            }

            mus_d.count = mus_d_count;
            fclose(file);
        }
    }

    /*printf("mus_d_count: %d\n", mus_d.count);
    for (int32_t i = 0; i < mus_d.count; i++) {
        char temp[100] = "";
        printf("\nType %s subtype %2d\n", eid_conv(mus_d.array[i].type, temp), mus_d.array[i].subtype);
        for (int32_t j = 0; j < mus_d.array[i].dependencies.count; j++) {
            char temp[100] = "";
            printf("\t%s\n", eid_conv(mus_d.array[i].dependencies.eids[j], temp));
        }
    }*/

    *permaloaded = perma;
    *subtype_info = subinfo;
    *collisions = coll;
    *music_deps = mus_d;
    if (!valid)
    {
        printf("Cannot proceed with invalid items, fix that\n");
        return 0;
    }
    return 1;
}

/** \brief
 *  Gets list of special entries in the zone's first item. For more info see function below.
 *
 * \param zone uint8_t*           zone to get the stuff from
 * \return LIST                         list of special entries
 */
LIST build_read_special_entries(uint8_t *zone)
{
    LIST special_entries = init_list();
    int32_t item1off = from_u32(zone + 0x10);
    uint8_t *metadata_ptr = zone + item1off + C2_SPECIAL_METADATA_OFFSET;
    int32_t special_entry_count = from_u32(metadata_ptr) & SPECIAL_METADATA_MASK_LLCOUNT;

    for (int32_t i = 1; i <= special_entry_count; i++)
    {
        uint32_t entry = from_u32(metadata_ptr + i * 4);
        // *(uint32_t *)(metadata_ptr + i * 4) = 0;
        list_add(&special_entries, entry);
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
LIST build_get_special_entries(ENTRY zone, ENTRY *elist, int32_t entry_count)
{
    LIST special_entries = build_read_special_entries(zone.data);
    LIST iteration_clone = init_list();
    list_copy_in(&iteration_clone, special_entries);

    for (int32_t i = 0; i < iteration_clone.count; i++)
    {
        char temp[100] = "";
        char temp2[100] = "";
        char temp3[100] = "";
        int32_t item = iteration_clone.eids[i];
        int32_t index = build_get_index(item, elist, entry_count);
        if (index == -1)
        {
            printf("[error] Zone %s special entry list contains entry %s which is not present.\n", eid_conv(zone.eid, temp), eid_conv(item, temp2));
            list_remove(&special_entries, item);
            continue;
        }

        if (build_entry_type(elist[index]) == ENTRY_TYPE_ANIM)
        {
            uint32_t model = build_get_model(elist[index].data, 0);
            int32_t model_index = build_get_index(model, elist, entry_count);
            if (model_index == -1 || build_entry_type(elist[model_index]) != ENTRY_TYPE_MODEL)
            {
                printf("[error] Zone %s special entry list contains animation %s that uses model %s that is not present or is not a model\n",
                       eid_conv(zone.eid, temp), eid_conv(item, temp2), eid_conv(model, temp3));
                continue;
            }

            list_add(&special_entries, model);
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
uint32_t *build_get_zone_relatives(uint8_t *entry, SPAWNS *spawns)
{
    int32_t entity_count, item1len, relcount, camcount, neighbourcount, scencount, i, entry_count = 0;
    uint32_t *relatives;

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
    relatives = (uint32_t *)malloc(relcount * sizeof(uint32_t)); // freed by build_main

    relatives[entry_count++] = relcount - 1;

    for (i = 0; i < (camcount / 3); i++)
    {
        int32_t offset = build_get_nth_item_offset(entry, 2 + 3 * i);
        relatives[entry_count++] = build_get_slst(entry + offset);
    }

    LIST neighbours = build_get_neighbours(entry);
    for (i = 0; i < neighbours.count; i++)
        relatives[entry_count++] = neighbours.eids[i];

    for (i = 0; i < scencount; i++)
        relatives[entry_count++] = from_u32(entry + item1off + 0x4 + 0x30 * i);

    if (music_ref != EID_NONE)
        relatives[entry_count++] = music_ref;

    for (i = 0; i < entity_count; i++)
    {
        int32_t *coords_ptr;
        coords_ptr = build_seek_spawn(entry + from_u32(entry + 0x18 + 4 * camcount + 4 * i));
        if (coords_ptr != NULL && spawns != NULL)
        {
            spawns->spawn_count += 1;
            int32_t cnt = spawns->spawn_count;
            spawns->spawns = (SPAWN *)realloc(spawns->spawns, cnt * sizeof(SPAWN));
            spawns->spawns[cnt - 1].x = coords_ptr[0] + *(int32_t *)(entry + item2off);
            spawns->spawns[cnt - 1].y = coords_ptr[1] + *(int32_t *)(entry + item2off + 4);
            spawns->spawns[cnt - 1].z = coords_ptr[2] + *(int32_t *)(entry + item2off + 8);
            spawns->spawns[cnt - 1].zone = from_u32(entry + 0x4);
            free(coords_ptr);
        }
    }
    return relatives;
}

/** \brief
 *  Gets gool relatives excluding models.
 *
 * \param entry uint8_t*          entry data
 * \param entrysize int32_t                 entry size
 * \return uint32_t*                array of relatives relatives[count + 1], relatives[0] == count
 */
uint32_t *build_get_gool_relatives(uint8_t *entry, int32_t entrysize)
{
    int32_t curr_off, type = 0, help;
    int32_t i, counter = 0;
    char temp[6];
    int32_t curr_eid;
    uint32_t local[256];
    uint32_t *relatives = NULL;

    curr_off = build_get_nth_item_offset(entry, 5);

    while (curr_off < entrysize)
        switch (entry[curr_off])
        {
        case 1:
            if (type == 0)
            {
                help = from_u32(entry + curr_off + 0xC) & 0xFF00FFFF;
                if (help <= 05 && help > 0)
                    type = 2;
                else
                    type = 3;
            }

            if (type == 2)
            {
                curr_eid = from_u32(entry + curr_off + 4);
                eid_conv(curr_eid, temp);
                if (temp[4] == 'G' || temp[4] == 'V')
                    local[counter++] = curr_eid;
                curr_off += 0xC;
            }
            else
            {
                for (i = 0; i < 4; i++)
                {
                    curr_eid = from_u32(entry + curr_off + 4 + 4 * i);
                    eid_conv(curr_eid, temp);
                    if (temp[4] == 'G' || temp[4] == 'V')
                        local[counter++] = curr_eid;
                }
                curr_off += 0x14;
            }
            break;
        case 2:
            curr_off += 0x8 + 0x10 * entry[curr_off + 2];
            break;
        case 3:
            curr_off += 0x8 + 0x14 * entry[curr_off + 2];
            break;
        case 4:
            curr_off = entrysize;
            break;
        case 5:
            curr_off += 0xC + 0x18 * entry[curr_off + 2] * entry[curr_off + 8];
            break;
        }

    if (!counter)
        return NULL;

    relatives = (uint32_t *)malloc((counter + 1) * sizeof(uint32_t)); // freed by build_main
    relatives[0] = counter;
    for (i = 0; i < counter; i++)
        relatives[i + 1] = local[i];

    return relatives;
}

/** \brief
 *  Searches the entity, if it has (correct) type and subtype and coords property,
 *  returns them as int32_t[3]. Usually set to accept willy and checkpoint entities.
 *
 * \param item uint8_t*           entity data
 * \return int32_t*                         int32_t[3] with xyz coords the entity if it meets criteria or NULL
 */
int32_t *build_seek_spawn(uint8_t *item)
{
    int32_t i, code, offset, type = -1, subtype = -1, coords_offset = -1;
    for (i = 0; (unsigned)i < from_u32(item + 0xC); i++)
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
            int32_t *coords = (int32_t *)malloc(3 * sizeof(int32_t)); // freed by caller
            for (i = 0; i < 3; i++)
                coords[i] = (*(int16_t *)(item + coords_offset + 2 * i)) * 4;
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
void build_get_model_references(ENTRY *elist, int32_t entry_count)
{
    int32_t i, j;
    uint32_t new_relatives[250];
    for (i = 0; i < entry_count; i++)
    {
        if ((elist[i].related != NULL) && (from_u32(elist[i].data + 8) == ENTRY_TYPE_GOOL))
        {
            int32_t relative_index;
            int32_t new_counter = 0;
            for (j = 0; (unsigned)j < elist[i].related[0]; j++)
                if ((relative_index = build_get_index(elist[i].related[j + 1], elist, entry_count)) >= 0)
                    if (elist[relative_index].data != NULL && (from_u32(elist[relative_index].data + 8) == 1))
                        new_relatives[new_counter++] = build_get_model(elist[relative_index].data, 0);

            if (new_counter)
            {
                int32_t relative_count;
                relative_count = elist[i].related[0];
                elist[i].related = (uint32_t *)realloc(elist[i].related, (relative_count + new_counter + 1) * sizeof(uint32_t));
                for (j = 0; j < new_counter; j++)
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
void build_get_distance_graph(ENTRY *elist, int32_t entry_count, SPAWNS spawns)
{
    for (int32_t i = 0; i < entry_count; i++)
    {
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE)
        {
            int32_t cam_count = build_get_cam_item_count(elist[i].data) / 3;
            elist[i].distances = (uint32_t *)malloc(cam_count * sizeof(uint32_t)); // freed by build_main
            elist[i].visited = (uint32_t *)malloc(cam_count * sizeof(uint32_t));   // freed by build_main

            for (int32_t j = 0; j < cam_count; j++)
            {
                elist[i].distances[j] = INT_MAX;
                elist[i].visited[j] = 0;
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
        for (int32_t i = 0; i < links.count; i++)
        {
            CAMERA_LINK link = int_to_link(links.eids[i]);
            int32_t neighbour_count = build_get_neighbour_count(elist[top_zone].data);
            uint32_t *neighbours = (uint32_t *)malloc(neighbour_count * sizeof(uint32_t)); // freed here
            int32_t item1off = from_u32(elist[top_zone].data + 0x10);
            for (int32_t j = 0; j < neighbour_count; j++)
                neighbours[j] = from_u32(elist[top_zone].data + item1off + C2_NEIGHBOURS_START + 4 + 4 * j);
            int32_t neighbour_index = build_get_index(neighbours[link.zone_index], elist, entry_count);
            if (neighbour_index == -1)
            {
                char temp1[100] = "";
                char temp2[100] = "";
                printf("[warning] %s references %s that does not seem to be present (link %d neighbour %d)\n",
                       eid_conv(elist[top_zone].eid, temp1), eid_conv(neighbours[link.zone_index], temp2), i, link.zone_index);
                continue;
            }
            int32_t path_count = build_get_cam_item_count(elist[neighbour_index].data) / 3;
            if (link.cam_index >= path_count)
            {
                char temp1[100] = "";
                char temp2[100] = "";
                printf("[warning] %s links to %s's cam path %d which doesnt exist (link %d neighbour %d)\n",
                       eid_conv(elist[top_zone].eid, temp1), eid_conv(neighbours[link.zone_index], temp2), link.cam_index, i, link.zone_index);
                continue;
            }

            if (elist[neighbour_index].visited[link.cam_index] == 0)
                graph_add(&graph, elist, neighbour_index, link.cam_index);
            free(neighbours);
        }
    }

    /*for (int32_t i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE)
        {
            int32_t cam_count = build_get_cam_count(elist[i].data)/3;
            for (int32_t j = 0; j < cam_count; j++)
                printf("Zone %s campath %d has distance %d\n", eid_conv(elist[i].eid, temp), j, elist[i].distances[j]);
        }*/
}

/** \brief
 *  Removes references to entries that are only in the base level.
 *
 * \param elist ENTRY*                  list of entries
 * \param entry_count int32_t               current amount of entries
 * \param entry_count_base int32_t
 * \return void
 */
void build_remove_invalid_references(ENTRY *elist, int32_t entry_count, int32_t entry_count_base)
{
    int32_t i, j, k;
    uint32_t new_relatives[250];

    for (i = 0; i < entry_count; i++)
    {
        if (elist[i].related == NULL)
            continue;
        for (j = 1; (unsigned)j < elist[i].related[0] + 1; j++)
        {
            int32_t relative_index;
            relative_index = build_get_index(elist[i].related[j], elist, entry_count);
            if (relative_index < entry_count_base)
                elist[i].related[j] = 0;
            if (elist[i].related[j] == elist[i].eid)
                elist[i].related[j] = 0;

            for (k = j + 1; (unsigned)k < elist[i].related[0] + 1; k++)
                if (elist[i].related[j] == elist[i].related[k])
                    elist[i].related[k] = 0;
        }

        int32_t counter = 0;
        for (j = 1; (unsigned)j < elist[i].related[0] + 1; j++)
            if (elist[i].related[j] != 0)
                new_relatives[counter++] = elist[i].related[j];

        if (counter == 0)
        {
            elist[i].related = (uint32_t *)realloc(elist[i].related, 0);
            continue;
        }

        elist[i].related = (uint32_t *)realloc(elist[i].related, (counter + 1) * sizeof(uint32_t));
        elist[i].related[0] = counter;
        for (j = 0; j < counter; j++)
            elist[i].related[j + 1] = new_relatives[j];
    }
}

// parsing input info for rebuilding using files from folder (not really supported)
int32_t build_read_and_parse_build(int32_t *level_ID, FILE **nsfnew, FILE **nsd, int32_t *chunk_border_texture, uint32_t *gool_table,
                                   ENTRY *elist, int32_t *entry_count, uint8_t **chunks, SPAWNS *spawns)
{
    char dirpath[MAX], nsfpath[MAX], lcltemp[MAX + 20];
    printf("Input the path to the base level (.nsf)[CAN BE A BLANK FILE]:\n");
    scanf(" %[^\n]", nsfpath);
    path_fix(nsfpath);

    printf("\nInput the path to the folder whose contents you want to import:\n");
    scanf(" %[^\n]", dirpath);
    path_fix(dirpath);

    FILE *nsf = NULL;
    if ((nsf = fopen(nsfpath, "rb")) == NULL)
    {
        printf("[ERROR] Could not open selected NSF\n\n");
        return 1;
    }

    DIR *df = NULL;
    if ((df = opendir(dirpath)) == NULL)
    {
        printf("[ERROR] Could not open selected directory\n\n");
        fclose(nsf);
        return 1;
    }

    *level_ID = build_ask_ID();

    *(strrchr(nsfpath, '\\') + 1) = '\0';
    sprintf(lcltemp, "%s\\S00000%02X.NSF", nsfpath, *level_ID);
    *nsfnew = fopen(lcltemp, "wb");
    *(strchr(lcltemp, '\0') - 1) = 'D';
    *nsd = fopen(lcltemp, "wb");

    // chunk_border_base = build_get_chunk_count_base(nsf);
    // build_read_nsf(elist, chunk_border_base, chunks, chunk_border_texture, &entry_count, nsf, gool_table);    // rn base level is not considered
    build_read_folder(df, dirpath, chunks, elist, chunk_border_texture, entry_count, spawns, gool_table);
    fclose(nsf);
    return 0;
}

// parsing input info for rebuilding from a nsf file
int32_t build_read_and_parse_rebld(int32_t *level_ID, FILE **nsfnew, FILE **nsd, int32_t *chunk_border_texture, uint32_t *gool_table,
                                   ENTRY *elist, int32_t *entry_count, uint8_t **chunks, SPAWNS *spawns, int32_t stats_only, char *fpath)
{
    FILE *nsf = NULL;
    char nsfpath[MAX], lcltemp[MAX + 20];

    if (fpath)
    {
        if ((nsf = fopen(fpath, "rb")) == NULL)
        {
            printf("[ERROR] Could not open selected NSF\n\n");
            return 1;
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
                return 1;
            }

            *level_ID = build_ask_ID();

            *(strrchr(nsfpath, '\\') + 1) = '\0';
            sprintf(lcltemp, "%s\\S00000%02X.NSF", nsfpath, *level_ID);
            *nsfnew = fopen(lcltemp, "wb");
            *(strchr(lcltemp, '\0') - 1) = 'D';
            *nsd = fopen(lcltemp, "wb");
        }
        else
        {
            printf("Input the path to the level (.nsf):\n");
            scanf(" %[^\n]", nsfpath);
            path_fix(nsfpath);

            if ((nsf = fopen(nsfpath, "rb")) == NULL)
            {
                printf("[ERROR] Could not open selected NSF\n\n");
                return 1;
            }
        }
    }

    int32_t nsf_chunk_count = build_get_chunk_count_base(nsf);
    int32_t lcl_chunk_border_texture = 0;

    uint8_t buffer[CHUNKSIZE];
    for (int32_t i = 0; i < nsf_chunk_count; i++)
    {
        fread(buffer, sizeof(uint8_t), CHUNKSIZE, nsf);
        uint32_t checksum_calc = nsfChecksum(buffer);
        uint32_t checksum_used = *(uint32_t *)(buffer + CHUNK_CHECKSUM_OFFSET);
        if (checksum_calc != checksum_used)
            printf("Chunk %3d has invalid checksum\n", 2 * i + 1);
        int32_t chunk_entry_count = from_u32(buffer + 0x8);
        if (build_chunk_type(buffer) == CHUNK_TYPE_TEXTURE)
        {
            if (chunks != NULL)
            {
                chunks[lcl_chunk_border_texture] = (uint8_t *)calloc(CHUNKSIZE, sizeof(uint8_t)); // freed by build_main
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
                elist[*entry_count].data = (uint8_t *)malloc(entry_size); // freed by build_main
                memcpy(elist[*entry_count].data, buffer + start_offset, entry_size);

                if (!stats_only || build_entry_type(elist[*entry_count]) == ENTRY_TYPE_SOUND)
                    elist[*entry_count].chunk = -1;
                else
                    elist[*entry_count].chunk = i;

                // elist[*entry_count].og_chunk = i;

                if (build_entry_type(elist[*entry_count]) == ENTRY_TYPE_ZONE)
                    build_check_item_count(elist[*entry_count].data, elist[*entry_count].eid);
                if (build_entry_type(elist[*entry_count]) == ENTRY_TYPE_ZONE)
                    elist[*entry_count].related = build_get_zone_relatives(elist[*entry_count].data, spawns);
                /*if (build_entry_type(elist[*entry_count]) == ENTRY_TYPE_GOOL && build_item_count(elist[*entry_count].data) == 6)
                    elist[*entry_count].related = build_get_gool_relatives(elist[*entry_count].data, entry_size);*/
                // causes issues sometimes

                if (build_entry_type(elist[*entry_count]) == ENTRY_TYPE_GOOL && build_item_count(elist[*entry_count].data) == 6)
                {
                    int32_t item1_offset = *(int32_t *)(elist[*entry_count].data + 0x10);
                    int32_t gool_type = *(int32_t *)(elist[*entry_count].data + item1_offset);
                    if (/*gool_type >= C2_GOOL_TABLE_SIZE || */ gool_type < 0)
                    {
                        char temp[100] = "";
                        printf("[warning] GOOL entry %s has invalid type specified in the third item (%2d)!\n", eid_conv(elist[*entry_count].eid, temp), gool_type);
                        continue;
                    }
                    if (gool_table != NULL && gool_table[gool_type] == EID_NONE)
                        gool_table[gool_type] = elist[*entry_count].eid;
                }

                *entry_count += 1;
                qsort(elist, *entry_count, sizeof(ENTRY), cmp_func_eid);
            }
    }
    fclose(nsf);
    if (chunk_border_texture != NULL)
        *chunk_border_texture = lcl_chunk_border_texture;
    return 0;
}
