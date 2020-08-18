#include "macros.h"


/** \brief
 *  Returns a value that makes sound entries aligned as they should be.
 *
 * \param input int                     input offset
 * \return int                          aligned offset
 */
int build_align_sound(int input)
{
    for (int i = 0; i < 16; i++)
        if ((input + i) % 16 == 8)
            return (input + i);

    return input;
}

/** \brief
 *  Just returns the model reference of an animation.
 *  Only looks at the first frame of the animation.
 *
 * \param anim unsigned char*           pointer to the data of the searched animation
 * \return unsigned int                 EID of the animation's model reference (as unsigned int)
 */
unsigned int build_get_model(unsigned char *anim) {
    return from_u32(anim + 0x10 + from_u32(anim + 0x10));
}


/** \brief
 *  If a chunk is empty it gets replaced with the last existing chunk.
 *  This goes on until no further replacements are done (no chunks are empty).
 *
 * \param index_start int               start of the range of chunks to be fixed
 * \param index_end int                 end of the range of chunks to be fixed (not in the range anymore)
 * \param entry_count int               current amount of entries
 * \param entry_list ENTRY*             entry list
 * \return int                          new chunk count
 */
int build_remove_empty_chunks(int index_start, int index_end, int entry_count, ENTRY *entry_list)
{
    int i, j, is_occupied = 0, empty_chunk = 0;
    while (1)
    {
        for (i = index_start; i < index_end; i++)
        {
            is_occupied = 0;
            empty_chunk = -1;
            for (j = 0; j < entry_count; j++)
                if (entry_list[j].chunk == i) is_occupied++;

            if (!is_occupied)
            {
                empty_chunk = i;
                break;
            }
        }

        if (empty_chunk == -1)
            break;

        for (i = 0; i < entry_count; i++)
            if (entry_list[i].chunk == index_end - 1)
                entry_list[i].chunk = empty_chunk;

        index_end--;
    }

    return index_end;
}


/** \brief
 *  Removes references to entries that are only in the base level.
 *
 * \param elist ENTRY*                  list of entries
 * \param entry_count int               current amount of entries
 * \param entry_count_base int
 * \return void
 */
void build_remove_invalid_references(ENTRY *elist, int entry_count, int entry_count_base)
{
    int i, j, k;
    unsigned int new_relatives[250];

    for (i = 0; i < entry_count; i++)
    {
        if (elist[i].related == NULL) continue;
        for (j = 1; (unsigned) j < elist[i].related[0] + 1; j++)
        {
            int relative_index;
            relative_index = build_get_index(elist[i].related[j], elist, entry_count);
            if (relative_index < entry_count_base) elist[i].related[j] = 0;
            if (elist[i].related[j] == elist[i].EID) elist[i].related[j] = 0;

            for (k = j + 1; (unsigned) k < elist[i].related[0] + 1; k++)
                if (elist[i].related[j] == elist[i].related[k])
                    elist[i].related[k] = 0;
        }

        int counter = 0;
        for (j = 1; (unsigned) j < elist[i].related[0] + 1; j++)
            if (elist[i].related[j] != 0) new_relatives[counter++] = elist[i].related[j];

        if (counter == 0)
        {
            elist[i].related = (unsigned int *) realloc(elist[i].related, 0);
            continue;
        }

        elist[i].related = (unsigned int *) realloc(elist[i].related, (counter + 1) * sizeof(unsigned int));
        elist[i].related[0] = counter;
        for (j = 0; j < counter; j++)
            elist[i].related[j + 1] = new_relatives[j];
    }
}

/** \brief
 *  Used during reading from the folder to prevent duplicate texture chunks.
 *  Searches existing chunks (base level's chunks) and if the texture EID matches
 *  it returns the index of the matching chunk, else returns -1
 *
 * \param textr unsigned int            searched texture chunk eid
 * \param chunks unsigned char**        array of chunks to be searched, chunk is a 64kB array
 * \param index_end int                 end index of the chunk array, aka current chunk count
 * \return int                          index of a chunk that matches search, or -1
 */
int build_get_base_chunk_border(unsigned int textr, unsigned char **chunks, int index_end)
{
    int i, retrn = -1;

    for (i = 0; i < index_end; i++)
        if (from_u32(chunks[i] + 4) == textr) retrn = i;

    return retrn;
}


/** \brief
 *  For each gool entry it searches related animatons and adds the model entries
 *  referenced by the animations to the gool entry's relatives.
 *
 * \param elist ENTRY*                  current entry list
 * \param entry_count int               current amount of entries
 * \return void
 */
void build_get_model_references(ENTRY *elist, int entry_count)
{
    int i, j;
    unsigned int new_relatives[250];
    for (i = 0; i < entry_count; i++)
    {
        if ((elist[i].related != NULL) && (from_u32(elist[i].data + 8) == 0xB))
        {
            int relative_index;
            int new_counter = 0;
            for (j = 0; (unsigned) j < elist[i].related[0]; j++)
                if ((relative_index = build_get_index(elist[i].related[j + 1], elist, entry_count)) >= 0)
                    if (elist[relative_index].data != NULL && (from_u32(elist[relative_index].data + 8) == 1))
                        new_relatives[new_counter++] = build_get_model(elist[relative_index].data);

            if (new_counter)
            {
                int relative_count;
                relative_count = elist[i].related[0];
                elist[i].related = (unsigned int *) realloc(elist[i].related, (relative_count + new_counter + 1) * sizeof(unsigned int));
                for (j = 0; j < new_counter; j++)
                    elist[i].related[j + relative_count + 1] = new_relatives[j];
                elist[i].related[0] += new_counter;
            }
        }
    }
}

/** \brief
 *  Searches the entry list looking for the specified EID.
 *  Binary search, entry list should be sorted by EID (ascending).
 *
 * \param eid unsigned int              searched EID
 * \param elist ENTRY*                  list of entries
 * \param entry_count int               amount of entries
 * \return int                          index of the searched EID or -1
 */
int build_get_index(unsigned int eid, ENTRY *elist, int entry_count)
{
    int first = 0;
    int last = entry_count - 1;
    int middle = (first + last)/2;

    while (first <= last)
    {
        if (elist[middle].EID < eid)
            first = middle + 1;
        else if (elist[middle].EID == eid)
            return middle;
        else
            last = middle - 1;

        middle = (first + last)/2;
    }

    return -1;
}

/** \brief
 *  Searches the properties, finds the offset of the slst property,
 *  returns EID of the slst.
 *
 * \param item unsigned char*           camera entity data
 * \return unsigned int                 slst reference or 0 if theres no slst reference property
 */
unsigned int build_get_slst(unsigned char *item)
{
    int i, offset = 0;
    for (i = 0; i < item[0xC]; i++)
        if ((from_u16(item + 0x10 + 8 * i)) == ENTITY_PROP_CAM_SLST)
            offset = 0xC + from_u16(item + 0x10 + 8 * i + 2);

    if (offset) return from_u32(item + offset + 4);
        else return 0;
}

/** \brief
 *  Returns path length of the input item.
 *
 * \param item unsigned char*           item whose length it returns
 * \return unsigned int                 path length
 */
unsigned int build_get_path_length(unsigned char *item)
{
    int i, offset = 0;
    for (i = 0; i < item[0xC]; i++)
        if ((from_u16(item + 0x10 + 8 * i)) == ENTITY_PROP_PATH)
            offset = 0xC + from_u16(item + 0x10 + 8 * i + 2);

    if (offset) return from_u32(item + offset);
        else return 0;
}

/** \brief
 *  Returns path the entity has & its length.
 *
 * \param elist ENTRY*                  entry list
 * \param zone_index int                index of entry in the list
 * \param item_index int                index of item of the entry whose path it will return
 * \param path_len int*                 path length return value
 * \return short int*                   path
 */
short int *build_get_path(ENTRY *elist, int zone_index, int item_index, int *path_len)
{
    unsigned char *item = elist[zone_index].data + from_u32(elist[zone_index].data + 0x10 + 4 * item_index);
    int i, offset = 0;
    for (i = 0; i < item[0xC]; i++)
        if ((from_u16(item + 0x10 + 8 * i)) == ENTITY_PROP_PATH)
            offset = 0xC + from_u16(item + 0x10 + 8 * i + 2);

    if (offset)
        *path_len = from_u32(item + offset);
    else
    {
        *path_len = 0;
        return NULL;
    }

    short int* coords = (short int *) malloc(3 * sizeof(short int) * *path_len);
    memcpy(coords, item + offset + 4, 6 * *path_len);
    return coords;
}

/** \brief
 *  Searches the entity, if it has (correct) type and subtype and coords property,
 *  returns them as int[3]. Usually set to accept willy and checkpoint entities.
 *
 * \param item unsigned char*           entity data
 * \return int*                         int[3] with xyz coords the entity if it meets criteria or NULL
 */
int* build_seek_spawn(unsigned char *item)
{
    int i, code, offset, type = -1, subtype = -1, coords_offset = -1;
    for (i = 0; (unsigned) i < from_u32(item + 0xC); i++)
    {
        code = from_u16(item + 0x10 + 8*i);
        offset = from_u16(item + 0x12 + 8*i) + OFFSET;
        if (code == ENTITY_PROP_TYPE)
            type = from_u32(item + offset + 4);
        if (code == ENTITY_PROP_SUBTYPE)
            subtype = from_u32(item + offset + 4);
        if (code == ENTITY_PROP_PATH)
            coords_offset = offset;
    }

    if ((!type && !subtype && coords_offset != -1) || (type == 34 && subtype == 4 && coords_offset != -1))
    {
        int *coords = (int*) malloc(3 * sizeof(int));
        for (i = 0; i < 3; i++)
            coords[i] = (*(short int*)(item + coords_offset + 4 + 2 * i)) * 4;
        return coords;
    }

    return NULL;
}


/** \brief
 *  Gets zone's neighbour count.
 *
 * \param entry unsigned char*          entry data
 * \return int                          neighbour count
 */
int build_get_neighbour_count(unsigned char *entry){
    int item1off = from_u32(entry + 0x10);
    return entry[item1off + C2_NEIGHBOURS_START];
}

/** \brief
 *  Returns a list of neighbours the entry (zone) has.
 *
 * \param entry unsigned char*         entry data
 * \return LIST                        list containing neighbour eids
 */
LIST build_get_neighbours(unsigned char *entry)
{
    int item1off = from_u32(entry + 0x10);
    int count = entry[item1off + C2_NEIGHBOURS_START];

    LIST neighbours = init_list();
    for (int k = 0; k < count; k++)
    {
        int neighbour_eid = from_u32(entry + item1off + C2_NEIGHBOURS_START + 4 + 4 * k);
        list_insert(&neighbours, neighbour_eid);
    }

    return neighbours;
}
/** \brief
 *  Gets zone's camera entity count.
 *
 * \param entry unsigned char*          entry data
 * \return int                          camera entity count (total count, not camera path count)
 */
int build_get_cam_count(unsigned char *entry){
    int item1off = from_u32(entry + 0x10);
    return entry[item1off + 0x188];
}

/** \brief
 *  Gets zone's scenery reference count.
 *
 * \param entry unsigned char*          entry data
 * \return int                          scenery reference count
 */
int build_get_scen_count(unsigned char *entry){
    int item1off = from_u32(entry + 0x10);
    return entry[item1off];
}

/** \brief
 *  Gets zone's regular entity count.
 *
 * \param entry unsigned char*          entry data
 * \return int                          entity count (not including camera entities)
 */
int build_get_entity_count(unsigned char *entry){
    int item1off = from_u32(entry + 0x10);
    return entry[item1off + 0x18C];
}

/** \brief
 *  Gets relatives of zones.
 *
 * \param entry unsigned char*          entry data
 * \param spawns SPAWNS*                during relative collection it searches for possible spawns
 * \return unsigned int*                array of relatives relatives[count + 1], relatives[0] contains count or NULL
 */
unsigned int* build_get_zone_relatives(unsigned char *entry, SPAWNS *spawns)
{
    int entity_count, item1len, relcount, item1off, camcount, neighbourcount, scencount, i, entry_count = 0;
    unsigned int* relatives;

    item1off = from_u32(entry + 0x10);
    item1len = from_u32(entry + 0x14) - item1off;
    if (!(item1len == 0x358 || item1len == 0x318)) return NULL;

    camcount = build_get_cam_count(entry);
    if (camcount == 0) return NULL;

    entity_count = build_get_entity_count(entry);
    scencount = build_get_scen_count(entry);
    neighbourcount = build_get_neighbour_count(entry);
    relcount = camcount/3 + neighbourcount + scencount + 1;

    if (relcount == 1) return NULL;

    if (from_u32(&entry[item1off + C2_MUSIC_REF + item1len - 0x318]) != NONE)
        relcount++;
    relatives = (unsigned int *) malloc(relcount * sizeof(unsigned int));

    relatives[entry_count++] = relcount - 1;

    for (i = 0; i < (camcount/3); i++)
        relatives[entry_count++] = build_get_slst(entry + from_u32(entry + 0x18 + 0xC * i));

    LIST neighbours = build_get_neighbours(entry);
    for (i = 0; i < neighbours.count; i++)
        relatives[entry_count++] = neighbours.eids[i];

    for (i = 0; i < scencount; i++)
        relatives[entry_count++] = from_u32(entry + item1off + 0x4 + 0x30 * i);

    if (from_u32(&entry[item1off + C2_MUSIC_REF + item1len - 0x318]) != NONE)
        relatives[entry_count++] = from_u32(&entry[item1off + C2_MUSIC_REF + item1len - 0x318]);

    for (i = 0; i < entity_count; i++)
    {
        int *coords_ptr;
        coords_ptr = build_seek_spawn(entry + from_u32(entry + 0x18 + 4 * camcount + 4 * i));
        if (coords_ptr != NULL)
        {
            spawns->spawn_count += 1;
            int cnt = spawns->spawn_count;
            spawns->spawns = (SPAWN *) realloc(spawns->spawns, cnt * sizeof(SPAWN));
            spawns->spawns[cnt -1].x = coords_ptr[0] + *(int*)(entry + from_u32(entry + 0x14));
            spawns->spawns[cnt -1].y = coords_ptr[1] + *(int*)(entry + from_u32(entry + 0x14) + 4);
            spawns->spawns[cnt -1].z = coords_ptr[2] + *(int*)(entry + from_u32(entry + 0x14) + 8);
            spawns->spawns[cnt -1].zone = from_u32(entry + 0x4);
        }
    }
    return relatives;
}

/** \brief
 *  Returns type of an entry.
 *
 * \param entry ENTRY                   entry struct
 * \return int                          -1 if entry does not have data allocated, else real entry type
 */
int build_entry_type(ENTRY entry){
    if (entry.data == NULL) return -1;
    return *(int *)(entry.data + 8);
}


/** \brief
 *  Gets gool relatives excluding models.
 *
 * \param entry unsigned char*          entry data
 * \param entrysize int                 entry size
 * \return unsigned int*                array of relatives relatives[count + 1], relatives[0] == count
 */
unsigned int* build_get_gool_relatives(unsigned char *entry, int entrysize)
{
    int curr_off, type = 0, help;
    int i, counter = 0;
    char temp[6];
    int curr_eid;
    unsigned int local[256];
    unsigned int *relatives = NULL;

    curr_off = from_u32(entry + 0x24);

    while (curr_off < entrysize)
        switch (entry[curr_off])
        {
            case 1:
                if (!type)
                {
                    help = from_u32(entry + curr_off + 0xC) & 0xFF00FFFF;
                    if (help <= 05 && help > 0) type = 2;
                        else type = 3;
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
                        curr_eid = from_u32(entry + curr_off + 4 + 4*i);
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


    if (!counter) return NULL;

    relatives = (unsigned int *) malloc((counter + 1) * sizeof(unsigned int));
    relatives[0] = counter;
    for (i = 0; i < counter; i++) relatives[i + 1] = local[i];

    return relatives;
}


/** \brief
 *  Reads the nsf, puts it straight into chunks, does not collect references or anything.
 *  Collects relatives.
 *
 * \param elist ENTRY*                  list of entries
 * \param chunk_border_base int         chunk count of the base nsf
 * \param chunks unsigned char**        chunk array, base level gets put straight into them
 * \param chunk_border_texture int*     index of the last texture chunk
 * \param entry_count int*              entry count
 * \param nsf FILE*                     nsf file
 * \param gool_table unsigned int*      gool table gets created, contains GOOL entries associated with the type/index they are on
 * \return void
 */
void build_read_nsf(ENTRY *elist, int chunk_border_base, unsigned char **chunks, int *chunk_border_texture, int *entry_count, FILE *nsf, unsigned int *gool_table)
{
    int i, j , offset;
    unsigned char chunk[CHUNKSIZE];

    for (i = 0; i < chunk_border_base; i++)
    {
        fread(chunk, CHUNKSIZE, sizeof(unsigned char), nsf);
        chunks[*chunk_border_texture] = (unsigned char*) calloc(CHUNKSIZE, sizeof(unsigned char));
        memcpy(chunks[*chunk_border_texture], chunk, CHUNKSIZE);
        (*chunk_border_texture)++;
        if (chunk[2] != 1)
            for (j = 0; j < chunk[8]; j++)
            {
                offset = *(int*) (chunk + 0x10 + j * 4);
                elist[*entry_count].EID = from_u32(chunk + offset + 4);
                elist[*entry_count].chunk = i;
                elist[*entry_count].esize = -1;
                if (*(int *)(chunk + offset + 8) == ENTRY_TYPE_GOOL && *(int*)(chunk + offset + 0xC) > 3)
                {
                    int item1_offset = *(int *)(chunk + offset + 0x10);
                    gool_table[*(int*)(chunk + offset + item1_offset)] = elist[*entry_count].EID;
                }
                (*entry_count)++;
            }
    }
}

/** \brief
 *  Merges chunks prioritising highest combined size.
 *  Does not consider anything else.
 *
 * \param elist ENTRY*                  entry list
 * \param chunk_index_start int         start of the range
 * \param chunk_index_end int*          end of the range
 * \param entry_count int               entry count
 * \return void
 */
void build_dumb_merge(ENTRY *elist, int chunk_index_start, int *chunk_index_end, int entry_count)
{
    int i, j, k;
    while(1)
    {
        int merge_happened = 0;
        for (i = chunk_index_start; i < *chunk_index_end; i++)
        {
            int size1 = 0;
            int count1 = 0;

            for (j = 0; j < entry_count; j++)
                if (elist[j].chunk == i)
                {
                    size1 += elist[j].esize;
                    count1++;
                }

            int maxsize = 0;
            int maxentry_count = 0;

            for (j = i + 1; j < *chunk_index_end; j++)
            {
                int size2 = 0;
                int count2 = 0;
                for (k = 0; k < entry_count; k++)
                    if (elist[k].chunk == j)
                    {
                        size2 += elist[k].esize;
                        count2++;
                    }

                if ((size1 + size2 + 4 * count1 + 4 * count2 + 0x14) <= CHUNKSIZE)
                    if (size2 > maxsize)
                    {
                        maxsize = size2;
                        maxentry_count = j;
                    }
            }

            if (maxentry_count)
            {
                for (j = 0; j < entry_count; j++)
                    if (elist[j].chunk == maxentry_count) elist[j].chunk = i;
                merge_happened++;
            }
        }
        if (!merge_happened) break;
    }

    *chunk_index_end = build_remove_empty_chunks(chunk_index_start, *chunk_index_end, entry_count, elist);
}


/** \brief
 *  For each valid file it adds it to the nsf(texture) or to the entry list.
 *  Collects relatives.
 *
 * \param df DIR*                       directory/folder
 * \param dirpath char*                 path to the directory/folder
 * \param chunks unsigned char**        array of 64kB chunks
 * \param elist ENTRY*                  entry list
 * \param chunk_border_texture int*     max index of a texture chunk
 * \param entry_count int*              entry count
 * \param spawns SPAWNS*                spawns, spawns get updated here
 * \param gool_table unsigned int*      table that contains GOOL entries on indexes that correspond to their types
 * \return void
 */
void build_read_folder(DIR *df, char *dirpath, unsigned char **chunks, ENTRY *elist, int *chunk_border_texture, int *entry_count, SPAWNS *spawns, unsigned int* gool_table)
{
    struct dirent *de;
    char temp[500];
    FILE *file = NULL;
    int fsize;
    unsigned char entry[CHUNKSIZE];

    while ((de = readdir(df)) != NULL)
    if ((de->d_name)[0] != '.')
    {
        sprintf(temp, "%s\\%s", dirpath, de->d_name);
        if (file != NULL)
        {
            fclose(file);
            file = NULL;
        }
        if ((file = fopen(temp, "rb")) == NULL) continue;
        fseek(file, 0, SEEK_END);
        fsize = ftell(file);
        rewind(file);
        fread(entry, fsize, sizeof(unsigned char), file);
        if (fsize == CHUNKSIZE && from_u16(entry + 0x2) == CHUNK_TYPE_TEXTURE)
        {
            if (build_get_base_chunk_border(from_u32(entry + 4), chunks, *chunk_border_texture) > 0) continue;
            chunks[*chunk_border_texture] = (unsigned char *) calloc(CHUNKSIZE, sizeof(unsigned char));
            memcpy(chunks[*chunk_border_texture], entry, CHUNKSIZE);
            elist[*entry_count].EID = from_u32(entry + 4);
            elist[*entry_count].chunk = *chunk_border_texture;
            elist[*entry_count].data = NULL;
            elist[*entry_count].related = NULL;
            (*entry_count)++;
            (*chunk_border_texture)++;
            continue;
        }

        if (from_u32(entry) != MAGIC_ENTRY) continue;
        if (build_get_index(from_u32(entry + 4), elist, *entry_count) >= 0) continue;

        elist[*entry_count].EID = from_u32(entry + 4);
        elist[*entry_count].chunk = -1;
        elist[*entry_count].esize = fsize;

        if (entry[8] == 7)
            elist[*entry_count].related = build_get_zone_relatives(entry, spawns);
        if (entry[8] == 11 && entry[0xC] == 6)
            elist[*entry_count].related = build_get_gool_relatives(entry, fsize);

        elist[*entry_count].data = (unsigned char *) malloc(fsize * sizeof(unsigned char));
        memcpy(elist[*entry_count].data, entry, fsize);
        if (build_entry_type(elist[*entry_count]) == ENTRY_TYPE_GOOL && *(elist[*entry_count].data + 8) > 3)
        {
            int item1_offset = *(int *)(elist[*entry_count].data + 0x10);
            gool_table[*(int*)(elist[*entry_count].data + item1_offset)] = elist[*entry_count].EID;
        }
        (*entry_count)++;
        qsort(elist, *entry_count, sizeof(ENTRY), cmp_entry_eid);
    }

    fclose(file);
}


/** \brief
 *  Prints entries and their relatives.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \return void
 */
void build_print_relatives(ENTRY *elist, int entry_count)
{
    int i, j;
    char temp[100];
    for (i = 0; i < entry_count; i++)
    {
        printf("%04d %s %02d %d\n", i, eid_conv(elist[i].EID, temp), elist[i].chunk, elist[i].esize);
        if (elist[i].related != NULL)
        {
            printf("------ %5d\n", elist[i].related[0]);
            for (j = 0; j < (signed) elist[i].related[0]; j++)
            {
                int relative = elist[i].related[j+1];
                printf("--%2d-- %s %2d %5d\n", j + 1, eid_conv(relative, temp),
                       elist[build_get_index(relative, elist, entry_count)].chunk, elist[build_get_index(relative, elist, entry_count)].esize);
            }
        }
    }
}

/** \brief
 *  Swaps spawns.
 *
 * \param spawns SPAWNS                 struct that contains all spanwns
 * \param spawnA int                    index1
 * \param spawnB int                    index2
 * \return void
 */
void build_swap_spawns(SPAWNS spawns, int spawnA, int spawnB)
{
    SPAWN temp = spawns.spawns[spawnA];
    spawns.spawns[spawnA] = spawns.spawns[spawnB];
    spawns.spawns[spawnB] = temp;
}

/** \brief
 *  Builds nsd, sorts load lists according to the nsd entry table order.
 *  Saves at the specified path.
 *  < 0x400 stays empty
 *  The rest should be ok.
 *  Lets you pick the main spawn.
 *
 * \param path char*                    path to the nsd
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_count int               chunk count
 * \param spawns SPAWNS                 spawns struct
 * \param gool_table unsigned int*      table that contains gool eids on corresponding indices
 * \param level_ID int                  level ID the user wanted
 * \return void
 */
void build_write_nsd(char *path, ENTRY *elist, int entry_count, int chunk_count, SPAWNS spawns, unsigned int* gool_table, int level_ID)
{
    int i, x = 0, input;
    char temp[100];
    FILE *nsd = fopen(path, "wb");
    if (nsd == NULL) return;

    unsigned char* nsddata = (unsigned char*) calloc(CHUNKSIZE, 1);
    *(int *)(nsddata + 0x400) = chunk_count;

    // lets u pick a spawn point
    printf("Pick a spawn:\n");
    for (i = 0; i < spawns.spawn_count; i++)
        printf("Spawn %d:\tZone: %s\n", i + 1, eid_conv(spawns.spawns[i].zone, temp));

    scanf("%d", &input);
    if (input - 1 > spawns.spawn_count || input <= 0) {
        printf("No such spawn, defaulting to first one\n");
        input = 1;
    }

    if (input - 1)
        build_swap_spawns(spawns, 0, input - 1);

    for (i = 0; i < entry_count; i++)
        if (elist[i].chunk != -1)
        {
            *(int *)(nsddata + 0x520 + 8*x) = elist[i].chunk * 2 + 1;
            *(int *)(nsddata + 0x524 + 8*x) = elist[i].EID;
            x++;
        }

    *(int *)(nsddata + 0x404) = x;

    int end = 0x520 + x * 8;
    *(int *)(nsddata + end) = spawns.spawn_count;
    *(int *)(nsddata + end + 8) = level_ID;
    end += 0x10;

    for (i = 0; i < 0x40; i++)
        *(int *)(nsddata + end + i * 4) = gool_table[i];

    end += 0x1CC;

    for (i = 0; i < spawns.spawn_count; i++)
    {
        *(int *)(nsddata + end) = spawns.spawns[i].zone;
        *(int *)(nsddata + end + 0x0C) = spawns.spawns[i].x * 256;
        *(int *)(nsddata + end + 0x10) = spawns.spawns[i].y * 256;
        *(int *)(nsddata + end + 0x14) = spawns.spawns[i].z * 256;
        end += 0x18;
    }

    fwrite(nsddata, 1, end, nsd);
    fclose(nsd);

    // sorts load lists
    for (i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].data != NULL)
        {
            int item1off = from_u32(elist[i].data + 0x10);
            int cam_count = from_u32(elist[i].data + item1off + 0x188) / 3;

            for (int j = 0; j < cam_count; j++)
            {
                int cam_offset = from_u32(elist[i].data + 0x18 + 0xC * j);
                for (int k = 0; (unsigned) k < from_u32(elist[i].data + cam_offset + 0xC); k++)
                {
                    int code = from_u16(elist[i].data + cam_offset + 0x10 + 8 * k);
                    int offset = from_u16(elist[i].data + cam_offset + 0x12 + 8 * k) + OFFSET + cam_offset;
                    int list_count = from_u16(elist[i].data + cam_offset + 0x16 + 8 * k);
                    if (code == ENTITY_PROP_CAM_LOAD_LIST_A || code == ENTITY_PROP_CAM_LOAD_LIST_B)
                    {
                        int sub_list_offset = offset + 4 * list_count;
                        for (int l = 0; l < list_count; l++)
                        {
                            int item_count = from_u16(elist[i].data + offset + l * 2);
                            ITEM list[item_count];
                            for (int m = 0; m < item_count; m++)
                            {
                                list[m].eid = from_u32(elist[i].data + sub_list_offset + 4 * m);
                                list[m].index = build_get_index(list[m].eid, elist, entry_count);
                            }
                            qsort(list, item_count, sizeof(ITEM), load_list_sort);
                            for (int m = 0; m < item_count; m++)
                                *(unsigned int*)(elist[i].data + sub_list_offset + 4 * m) = list[m].eid;
                            sub_list_offset += item_count * 4;
                        }
                    }
                }
            }
        }
}



/** \brief
 *  For each pair of entries it increments corresponding matrix tile.
 *
 * \param list LIST                     current load list
 * \param entries LIST                  list of valid normal chunk entries
 * \param entry_matrix int**            triangle matrix that contains amount of common load list occurences of entries[i], entries[j] on each tile [i][j]
 * \param rating int                    increment value (used to properly consider all camera points without doing it for every single point)
 * \return void
 */
void build_increment_common(LIST list, LIST entries, int **entry_matrix, int rating)
{
    for (int i = 0; i < list.count; i++)
        for (int j = i + 1; j < list.count; j++)
        {
            int indexA = list_find(entries, list.eids[i]);
            int indexB = list_find(entries, list.eids[j]);

            if (indexA < indexB)
                swap_ints(&indexA, &indexB);

            if (indexA == -1 || indexB == -1) continue;

            entry_matrix[indexA][indexB] += rating;
        }
}


/** \brief
 *  For each entry pair it finds out what chunk each is in and attempts to merge.
 *  Starts with entries with highest common occurence count.
 *
 * \param relations RELATIONS           array form of the common load list matrix sorted high to low
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param entries LIST                  valid entries list
 * \return void
 */
void build_matrix_merge_util(RELATIONS relations, ENTRY *elist, int entry_count, LIST entries)
{
    for (int x = 0; x < relations.count; x++)
    {
        int index1 = relations.relations[x].index1;
        int index2 = relations.relations[x].index2;

        int elist_index1 = build_get_index(entries.eids[index1], elist, entry_count);
        int elist_index2 = build_get_index(entries.eids[index2], elist, entry_count);

        int chunk_index1 = elist[elist_index1].chunk;
        int chunk_index2 = elist[elist_index2].chunk;

        int size = 0x14;
        for (int i = 0; i < entry_count; i++)
            if (elist[i].chunk == chunk_index1 || elist[i].chunk == chunk_index2)
                size += elist[i].esize + 4;

        if (size < CHUNKSIZE)
            for (int i = 0; i < entry_count; i++)
                if (elist[i].chunk == chunk_index2)
                    elist[i].chunk = chunk_index1;
    }
}


/** \brief
 *  Creates an array representation of the common load list occurence matrix, sorts high to low.
 *
 * \param entries LIST                  valid entries list
 * \param entry_matrix int**            matrix that contains common load list occurences
 * \return RELATIONS
 */
RELATIONS build_transform_matrix(LIST entries, int **entry_matrix)
{
    RELATIONS relations;
    relations.count = (entries.count * (entries.count - 1)) / 2;
    relations.relations = (RELATION *) malloc(relations.count * sizeof(RELATION));

    int i, j, indexer = 0;
    for (i = 0; i < entries.count; i++)
        for (j = 0; j < i; j++)
        {
            relations.relations[indexer].value = entry_matrix[i][j];
            relations.relations[indexer].index1 = i;
            relations.relations[indexer].index2 = j;
            indexer++;
        }

    qsort(relations.relations, relations.count, sizeof(RELATION), relations_cmp);
    return relations;
}

/** \brief
 *  Deconstructs the load or draw lists and saves into a convenient struct.
 *
 * \param prop_code int                 first of the two list properties (either 0x13B or 0x208)
 * \param entry unsigned char*          entry data
 * \param cam_offset int                offset of the camera item
 * \return LOAD_LIST                    load or draw list struct
 */
LOAD_LIST build_get_lists(int prop_code, unsigned char *entry, int cam_offset)
{
    int k, l;
    LOAD_LIST load_list = init_load_list();
    for (k = 0; (unsigned) k < from_u32(entry + cam_offset + 0xC); k++)
    {
        int code = from_u16(entry + cam_offset + 0x10 + 8 * k);
        int offset = from_u16(entry + cam_offset + 0x12 + 8 * k) + OFFSET + cam_offset;
        int list_count = from_u16(entry + cam_offset + 0x16 + 8 * k);
        if (code == prop_code || code == prop_code + 1)
        {
            int sub_list_offset = offset + 4 * list_count;
            int point;
            int load_list_item_count;
            for (l = 0; l < list_count; l++, sub_list_offset += load_list_item_count * 4)
            {
                load_list_item_count = from_u16(entry + offset + l * 2);
                point = from_u16(entry + offset + l * 2 + list_count * 2);

                load_list.array[load_list.count].list_length = load_list_item_count;
                load_list.array[load_list.count].list = (unsigned int *) malloc(load_list_item_count * sizeof(unsigned int));
                memcpy(load_list.array[load_list.count].list, entry + sub_list_offset, load_list_item_count * sizeof(unsigned int));
                if (code == prop_code)
                    load_list.array[load_list.count].type = 'A';
                else
                    load_list.array[load_list.count].type = 'B';
                load_list.array[load_list.count].index = point;
                (load_list.count)++;
                qsort(load_list.array, load_list.count, sizeof(LOAD), comp);
            }
        }
    }

    // printf("lived\n");
    return load_list;
}

/** \brief
 *  Current best chunk merge method based on common load list occurence count of each pair of entries,
 *  therefore relies on proper and good load lists ideally with delta items.
 *  After the matrix is created and transformed into a sorted array it attemps to merge.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_border_sounds int       unused
 * \param chunk_count int*              chunk count
 * \param merge_flag int                per-point if 1, per-delta-item if 0
 * \return void
 */
void build_matrix_merge_main(ENTRY *elist, int entry_count, int chunk_border_sounds, int* chunk_count, int merge_flag)
{
    int i, j, l, m;
    LIST entries = init_list();

    for (i = 0; i < entry_count; i++)
    {
        switch (build_entry_type(elist[i]))
        {
            case -1:
            case 5:
            case 6:
            case 12:
            case 14:
            case 20:
            case 21:
                continue;
            default: ;
        }

        list_insert(&entries, elist[i].EID);
    }

    int **entry_matrix = (int **) malloc(entries.count * sizeof(int*));
    for (i = 0; i < entries.count; i++)
        entry_matrix[i] = (int *) calloc((i), sizeof(int));

    for (i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].data != NULL)
        {
            int item1off = from_u32(elist[i].data + 0x10);
            int cam_count = from_u32(elist[i].data + item1off + 0x188) / 3;

            for (j = 0; j < cam_count; j++)
            {
                int cam_offset = from_u32(elist[i].data + 0x10 + 4 * (2 + 3 * j));
                char temp[100];
                // printf("zone: %s\n", eid_conv(elist[i].EID, temp));
                LOAD_LIST load_list = build_get_lists(ENTITY_PROP_CAM_LOAD_LIST_A, elist[i].data, cam_offset);
                int cam_length = build_get_path_length(elist[i].data + cam_offset);


                LIST list = init_list();
                switch(merge_flag)
                {
                    case 1:     // per point
                    {
                        int sublist_index = 0;
                        int rating = 0;
                        for (l = 0; l < cam_length; l++)
                        {
                            rating++;
                            if (load_list.array[sublist_index].index == l)
                            {
                                if (load_list.array[sublist_index].type == 'A')
                                    for (m = 0; m < load_list.array[sublist_index].list_length; m++)
                                        list_add(&list, load_list.array[sublist_index].list[m]);

                                if (load_list.array[sublist_index].type == 'B')
                                    for (m = 0; m < load_list.array[sublist_index].list_length; m++)
                                        list_rem(&list, load_list.array[sublist_index].list[m]);

                                sublist_index++;
                                build_increment_common(list, entries, entry_matrix, 1);
                                rating = 0;
                            }
                        }
                        break;
                    }
                    case 0:     // per delta item
                    {
                        for (l = 0; l < load_list.count; l++)
                        {
                            if (load_list.array[l].type == 'A')
                                for (m = 0; m < load_list.array[l].list_length; m++)
                                    list_add(&list, load_list.array[l].list[m]);

                            if (load_list.array[l].type == 'B')
                                for (m = 0; m < load_list.array[l].list_length; m++)
                                    list_rem(&list, load_list.array[l].list[m]);

                            if (list.count)
                                build_increment_common(list, entries, entry_matrix, 1);
                        }
                        break;
                    }
                    default:
                        break;
                }

                delete_load_list(load_list);
            }
        }

    RELATIONS array_representation = build_transform_matrix(entries, entry_matrix);

    for (i = 0; i < entries.count; i++)
        free(entry_matrix[i]);
    free(entry_matrix);

    build_matrix_merge_util(array_representation, elist, entry_count, entries);
    *chunk_count = build_remove_empty_chunks(chunk_border_sounds, *chunk_count, entry_count, elist);
}



/** \brief
 *  Builds chunks according to entries' assigned chunks, shouldnt require any
 *  further patches.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_border_sounds int       index where sounds end
 * \param chunk_count int               chunk count
 * \param chunks unsigned char**        array of chunks
 * \return void
 */
void build_normal_chunks(ENTRY *elist, int entry_count, int chunk_border_sounds, int chunk_count, unsigned char **chunks)
{
    int i, j, sum = 0;
    for (i = chunk_border_sounds; i < chunk_count; i++)
    {
        int chunk_no = 2 * i + 1;
        int local_entry_count = 0;
        chunks[i] = (unsigned char*) calloc(CHUNKSIZE, sizeof(unsigned char));

        for (j = 0; j < entry_count; j++)
            if (elist[j].chunk == i) local_entry_count++;

        unsigned int offsets[local_entry_count + 2];
        *(unsigned short int*)  chunks[i] = MAGIC_CHUNK;
        *(unsigned short int*) (chunks[i] + 4) = chunk_no;
        *(unsigned short int*) (chunks[i] + 8) = local_entry_count;

        // calculates offsets
        int indexer = 0;
        offsets[indexer] = 0x14 + local_entry_count * 4;

        for (j = 0; j < entry_count; j++)
        if (elist[j].chunk == i)
        {
            offsets[indexer + 1] = offsets[indexer] + elist[j].esize;
            indexer++;
        }

        // writes offsets
        for (j = 0; j < local_entry_count + 1; j++)
            *(unsigned int *) (chunks[i] + 0x10 + j * 4) = offsets[j];

        // writes entries
        int curr_offset = offsets[0];
        for (j = 0; j < entry_count; j++)
        {
            if (elist[j].chunk == i)
            {
                memcpy(chunks[i] + curr_offset, elist[j].data, elist[j].esize);
                curr_offset += elist[j].esize;
            }

        }

        sum += curr_offset;
        *((unsigned int *)(chunks[i] + 0xC)) = nsfChecksum(chunks[i]);
    }

    printf("AVG: %.3f\n", (100 * (double) sum / (chunk_count - chunk_border_sounds)) / CHUNKSIZE);
}

/** \brief
 *  Returns value of the specified property. Only works on generic, single-value (4B length 4B value) properties.
 *
 * \param entity unsigned char*         entity data
 * \param prop_code int                 property to return
 * \return int                          property value
 */
int build_get_entity_prop(unsigned char *entity, int prop_code)
{
    unsigned int i;
    for (i = 0; i < from_u32(entity + 0xC); i++)
    {
        int code = from_u16(entity + 0x10 + 8 * i);
        int offset = from_u16(entity + 0x12 + 8 * i) + OFFSET;

        if (code == prop_code)
            return from_u32(entity + offset + 4);
    }

    return -1;
}

/** \brief
 *  Gets texture references from a scenery entry and inserts them to the list.
 *
 * \param scenery unsigned char*            scenery data
 * \param list LIST*                        list to be added to
 * \return void
 */
void build_add_scen_textures_to_list(unsigned char *scenery, LIST *list)
{
    int item1off = from_u32(scenery + 0x10);
    int texture_count = from_u32(scenery + item1off + 0x28);
    for (int i = 0; i < texture_count; i++)
    {
        unsigned int eid = from_u32(scenery + item1off + 0x2C + 4 * i);
        list_insert(list, eid);
    }
}

/** \brief
 *  Gets texture references from a model and adds them to the list.
 *
 * \param list LIST*                        list to be added to
 * \param model unsigned char*              model data
 * \return void
 */
void build_add_model_textures_to_list(unsigned char *model, LIST *list)
{
    int item1off = from_u32(model + 0x10);

    for (int i = 0; i < 8; i++)
    {
        unsigned int reference = from_u32(model + item1off + 0xC + 0x4 * i);
        if (reference)
            list_insert(list, reference);
    }
}


/** \brief
 *  Creates a load list and inserts it basically.
 *
 * \param code unsigned int             code of the property to be added
 * \param item unsigned char*           data of item where the property will be added
 * \param item_size int*                item size
 * \param list LIST *                   load list to be added
 * \return unsigned char*               new item data
 */
unsigned char *build_add_property(unsigned int code, unsigned char *item, int* item_size, PROPERTY *prop)
{
    int offset, i, property_count = from_u32(item + 0xC);
    unsigned char property_headers[property_count + 1][8] = {0};
    int property_sizes[property_count + 1];
    unsigned char *properties[property_count + 1];

    for (i = 0; i < property_count; i++)
    {
        if (from_u16(item + 0x10 + 8 * i) > code)
            memcpy(property_headers[i + 1], item + 0x10 + 8 * i, 8);
        else
            memcpy(property_headers[i], item + 0x10 + 8 * i, 8);
    }

    int insertion_index;
    for (i = 0; i < property_count + 1; i++)
        if (from_u32(property_headers[i]) == 0 && from_u32(property_headers[i] + 4) == 0)
            insertion_index = i;


    for (i = 0; i < property_count + 1; i++)
    {
        if (i < insertion_index - 1)
            property_sizes[i] = from_u16(property_headers[i + 1] + 2) - from_u16(property_headers[i] + 2);
        if (i == insertion_index - 1)
            property_sizes[i] = from_u16(property_headers[i + 2] + 2) - from_u16(property_headers[i] + 2);
        if (i == insertion_index)
            ;
        if (i > insertion_index)
        {
            if (i == property_count)
                property_sizes[i] = from_u16(item) - from_u16(property_headers[i] + 2);
            else
                property_sizes[i] = from_u16(property_headers[i + 1] + 2) - from_u16(property_headers[i] + 2);
        }
    }

    offset = 0x10 + 8 * property_count;
    for (i = 0; i < property_count + 1; i++)
    {
        if (i == insertion_index) continue;

        properties[i] = (unsigned char *) malloc(property_sizes[i]);
        memcpy(properties[i], item + offset, property_sizes[i]);
        offset += property_sizes[i];
    }

    property_sizes[insertion_index] = prop->length;
    memcpy(property_headers[insertion_index], prop->header, 8);
    properties[insertion_index] = prop->data;

    int new_size = 0x10 + 8 * (property_count + 1);
    for (i = 0; i < property_count + 1; i++)
        new_size += property_sizes[i];


    unsigned char *copy = (unsigned char *) malloc(new_size);
    *(int *)(copy) = new_size - 0xC;
    *(int *)(copy + 4) = 0;
    *(int *)(copy + 8) = 0;
    *(int *)(copy + 0xC) = property_count + 1;

    offset = 0x10 + 8 * (property_count + 1);
    for (i = 0; i < property_count + 1; i++)
    {
        *(short int*) (property_headers[i] + 2) = offset - 0xC;
        memcpy(copy + 0x10 + 8 * i, property_headers[i], 8);
        memcpy(copy + offset, properties[i], property_sizes[i]);
        offset += property_sizes[i];
    }

    *item_size = new_size - 0xC;
    free(item);
    return copy;
}

/** \brief
 *  Removes the specified property.
 *
 * \param code unsigned int             code of the property to be removed
 * \param item unsigned char*           data of the item thats to be changed
 * \param item_size int*                size of the item
 * \param prop PROPERTY                 unused
 * \return unsigned char*               new item data
 */
unsigned char* build_rem_property(unsigned int code, unsigned char *item, int* item_size, PROPERTY *prop)
{
    int offset, i, property_count = from_u32(item + 0xC);
    unsigned char property_headers[property_count][8];
    int property_sizes[property_count];
    unsigned char *properties[property_count];

    int found = 0;
    for (i = 0; i < property_count; i++)
    {
        memcpy(property_headers[i], item + 0x10 + 8 * i, 8);
        if (from_u16(property_headers[i]) == code)
            found++;
    }

    if (!found)
        return item;

    for (i = 0; i < property_count - 1; i++)
        property_sizes[i] = from_u16(property_headers[i + 1] + 2) - from_u16(property_headers[i] + 2);
    property_sizes[i] = from_u16(item) - 0xC - from_u16(property_headers[i] + 2);

    offset = 0x10 + 8 * property_count;
    for (i = 0; i < property_count; offset += property_sizes[i], i++)
    {
        properties[i] = (unsigned char *) malloc(property_sizes[i]);
        memcpy(properties[i], item + offset, property_sizes[i]);
    }

    int new_size = 0x10 + 8 * (property_count - 1);
    for (i = 0; i < property_count; i++)
    {
        if (from_u16(property_headers[i]) == code) continue;
        new_size += property_sizes[i];
    }

    unsigned char *copy = (unsigned char *) malloc(new_size);
    *(int *)(copy) = new_size;
    *(int *)(copy + 4) = 0;
    *(int *)(copy + 8) = 0;
    *(int *)(copy + 0xC) = property_count - 1;

    offset = 0x10 + 8 * (property_count - 1);
    int indexer = 0;
    for (i = 0; i < property_count; i++)
    {
        if (from_u16(property_headers[i]) != code)
        {
            *(short int*) (property_headers[i] + 2) = offset - 0xC;
            memcpy(copy + 0x10 + 8 * indexer, property_headers[i], 8);
            memcpy(copy + offset, properties[i], property_sizes[i]);
            indexer++;
            offset += property_sizes[i];
        }
    }

    *item_size = new_size;
    free(item);
    return copy;
}


/** \brief
 *  Deconstructs the zone, alters specified item using the func_arg function, reconstructs zhe zone.
 *
 * \param zone ENTRY*                   zone data
 * \param item_index int                index of the item to be altered
 * \param func_arg unsigned char*       function to be used, either remove or add property
 * \param list LIST*                    list to be added (might be unused)
 * \param property_code int             property code
 * \return void
 */
void build_entity_alter(ENTRY *zone, int item_index, unsigned char *(func_arg)(unsigned int, unsigned char *, int *, PROPERTY *), PROPERTY *prop, int property_code)
{
    int i, offset;
    int item_count = from_u32(zone->data + 0xC);

    int item_lengths[item_count];
    unsigned char *items[item_count];
    for (i = 0; i < item_count; i++)
        item_lengths[i] = from_u32(zone->data + 0x14 + 4 * i) - from_u32(zone->data + 0x10 + 4 * i);

    offset = 0x10 + 4 * item_count + 4;
    for (i = 0; i < item_count; i++)
    {
        items[i] = (unsigned char *) malloc(item_lengths[i]);
        memcpy(items[i], zone->data + offset, item_lengths[i]);
        offset += item_lengths[i];
    }

    items[item_index] = func_arg(property_code, items[item_index], &item_lengths[item_index], prop);

    int new_size = 0x14;
    for (i = 0; i < item_count; i++)
        new_size += 4 + item_lengths[i];

    unsigned char *copy = (unsigned char *) malloc(new_size);
    *(int *)(copy) = 0x100FFFF;
    *(int *)(copy + 4) = zone->EID;
    *(int *)(copy + 8) = 7;
    *(int *)(copy + 0xC) = item_count;

    for (offset = 0x10 + 4 * item_count + 4, i = 0; i < item_count + 1; offset += item_lengths[i], i++)
        *(int *)(copy + 0x10 + i * 4) = offset;

    for (offset = 0x10 + 4 * item_count + 4, i = 0; i < item_count; offset += item_lengths[i], i++)
        memcpy(copy + offset, items[i], item_lengths[i]);

    free(zone->data);
    zone->data = copy;
    zone->esize = new_size;

    for (i = 0; i < item_count; i++)
        free(items[i]);
}

/** \brief
 *  Gets indexes of camera linked neighbours specified in the camera link porperty.
 *
 * \param entry unsigned char*          entry data
 * \param cam_index int                 index of the camera item
 * \return void
 */
LIST build_get_links(unsigned char *entry, int cam_index)
{
    int i, k, l, link_count = 0;
    int *links;
    int cam_offset = from_u32(entry + 0x10 + 4 * cam_index);

    for (k = 0; (unsigned) k < from_u32(entry + cam_offset + 0xC); k++)
    {
        int code = from_u16(entry + cam_offset + 0x10 + 8 * k);
        int offset = from_u16(entry + cam_offset + 0x12 + 8 * k) + OFFSET + cam_offset;
        int prop_len = from_u16(entry + cam_offset + 0x16 + 8 * k);

        if (code == ENTITY_PROP_CAM_PATH_LINKS)
        {
            if (prop_len == 1)
            {
                link_count = from_u16(entry + offset);
                links = (int *) malloc(link_count * sizeof(int));
                for (l = 0; l < link_count; l++)
                    links[l] = from_u32(entry + offset + 4 + 4 * l);
            }
            else
            {
                link_count = max(1, from_u16(entry + offset)) + max(1, from_u16(entry + offset + 2));
                links = (int *) malloc(link_count * sizeof(int));
                for (l = 0; l < link_count; l++)
                    links[l] = from_u32(entry + offset + 0x8 + 4 * l);
            }
        }
    }

    LIST links_list = init_list();
    for (i = 0; i < link_count; i++)
        list_insert(&links_list, links[i]);

    return links_list;
}


/** \brief
 *  Starting from the 0th index it finds a path index at which the distance reaches cap distance and loads
 *  the entries specified by the additions list from point 0 to end index.
 *
 * \param cam_length int                length of the considered camera path
 * \param full_list LIST*               load list
 * \param distance int                  distance so far
 * \param final_distance int            distance cap
 * \param coords short int*             path of the camera
 * \param path_length int               length of the camera
 * \param additions LIST                list with entries to add
 * \return void
 */
void build_load_list_util_util_forw(int cam_length, LIST *full_list, int distance, int final_distance, short int* coords, int path_length, LIST additions)
{
    int end_index = 0;
    int j, distance2 = 0;

    if (distance + distance2 < final_distance) end_index++;
    build_get_distance(coords, 0, path_length - 1, final_distance - (distance + BACKWARDS_PENALTY), &end_index);

    if (end_index <= 0) return;

    for (j = 0; j < end_index; j++)
        list_copy_in(&full_list[j], additions);
}

/** \brief
 *  Starting from the n-1th index it finds a path index at which the distance reaches cap distance and loads
 *  the entries specified by the additions list from point start index n - 1.
 *
 * \param cam_length int                length of the considered camera path
 * \param full_list LIST*               load list
 * \param distance int                  distance so far
 * \param final_distance int            distance cap
 * \param coords short int*             path of the camera
 * \param path_length int               length of the camera
 * \param additions LIST                list with entries to add
 * \return void
 */
void build_load_list_util_util_back(int cam_length, LIST *full_list, int distance, int final_distance, short int* coords, int path_length, LIST additions)
{
    int start_index = cam_length - 1;
    int j;

    build_get_distance(coords, path_length - 1, 0, final_distance - distance, &start_index);
    if (start_index == cam_length - 1) return;

    for (j = start_index; j < cam_length; j++)
        list_copy_in(&full_list[j], additions);
}


/** \brief
 *  Searches the entry and for each collision type it adds its dependencies (if there are any)
 *  to the list, copies it into items start_index to end_index (exc.).
 *
 * \param full_list LIST*               current load list
 * \param start_index int               camera point index from which the additions are copied in
 * \param end_index int                 camera point index until which the additions are copied in
 * \param entry unsigned char*          entry in which it searches collision types
 * \param collisions DEPENDENCIES       list that contains list of entries tied to a specific collision type
 * \param elist ENTRY*                  entry_list
 * \param entry_count int               entry_count
 * \return void
 */
void build_add_collision_dependencies(LIST *full_list, int start_index, int end_index, unsigned char *entry, DEPENDENCIES collisions, ENTRY *elist, int entry_count)
{
    int x, i, j, k;
    LIST neighbours = build_get_neighbours(entry);

    for (x = 0; x < neighbours.count; x++)
    {
        int index = build_get_index(neighbours.eids[x], elist, entry_count);
        if (index == -1)
            continue;
        int item2off = from_u32(elist[index].data + 0x14);
        unsigned char *item = elist[index].data + item2off;
        int count = *(int *) item + 0x18;
        for (i = 0; i < count + 2; i++)
        {
            short int type = *(short int *) item + 0x24 + 2 * i;
            if (type % 2 == 0) continue;

            for (j = 0; j < collisions.count; j++)
                if (type == collisions.array[j].type)
                    for (k = start_index; k < end_index; k++)
                        list_copy_in(&full_list[k], collisions.array[j].dependencies);
        }
    }
}

/** \brief
 *  Deals with slst and neighbour/scenery references of path linked entries.
 *
 * \param zone_index int                zone entry index
 * \param cam_index int                 camera item index
 * \param link_int unsigned int         link item int
 * \param listA LIST*                   representation of load list A
 * \param listB LIST*                   representation of laod list B
 * \param cam_length int                length of the camera's path
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \return void
 */
void build_load_list_util_util(int zone_index, int cam_index, int link_int, LIST *full_list, int cam_length, ENTRY * elist, int entry_count, int* config, DEPENDENCIES collisisons)
{
    int i, j, item1off = from_u32(elist[zone_index].data + 0x10);
    short int* coords;
    int path_length, distance = 0;

    LINK link = int_to_link(link_int);

    unsigned int neighbour_eid = from_u32(elist[zone_index].data + item1off + C2_NEIGHBOURS_START + 4 + 4 * link.zone_index);
    int neighbour_index = build_get_index(neighbour_eid, elist, entry_count);
    if (neighbour_index == -1) return;
    int offset = from_u32(elist[neighbour_index].data + 0x10 + 4 * (2 + 3 * link.cam_index));

    int scenery_count = build_get_scen_count(elist[neighbour_index].data);
    for (i = 0; i < scenery_count; i++)
    {
        int item1off_neigh = from_u32(elist[neighbour_index].data + 0x10);
        int scenery_index = build_get_index(from_u32(elist[neighbour_index].data + item1off_neigh + 0x4 + 0x30 * i), elist, entry_count);
        /*if (link.type == 1)
        {
            int end = (cam_length - 1)/2 - 1;
            for (j = 0; j < end; j++)
                build_add_scen_textures_to_list(elist[scenery_index].data, &full_list[j]);
        }

        if (link.type == 2)
        {
            int start = (cam_length - 1)/2 + 1;
            for (j = start; j < cam_length; j++)
                build_add_scen_textures_to_list(elist[scenery_index].data, &full_list[j]);
        }*/
    }

    if (elist[neighbour_index].related != NULL)
        for (i = 0; (unsigned) i < elist[neighbour_index].related[0]; i++)
            for (j = 0; j < cam_length; j++)
                list_insert(&full_list[j], elist[neighbour_index].related[i + 1]);


    unsigned int slst = build_get_slst(elist[neighbour_index].data + offset);
    for (i = 0; i < cam_length; i++)
        list_insert(&full_list[i], slst);
    build_add_collision_dependencies(full_list, 0, cam_length, elist[zone_index].data, collisisons, elist, entry_count);
    build_add_collision_dependencies(full_list, 0, cam_length, elist[neighbour_index].data, collisisons, elist, entry_count);

    coords = build_get_path(elist, neighbour_index, 2 + 3 * link.cam_index, &path_length);
    distance += build_get_distance(coords, 0, path_length - 1, -1, NULL);


    LIST layer2 = build_get_links(elist[neighbour_index].data, 2 + 3 * link.cam_index);
    for (i = 0; i < layer2.count; i++)
    {
        int item1off2 = from_u32(elist[neighbour_index].data + 0x10);
        LINK link2 = int_to_link(layer2.eids[i]);

        unsigned int neighbour_eid2 = from_u32(elist[neighbour_index].data + item1off2 + C2_NEIGHBOURS_START + 4 + 4 * link2.zone_index);
        int neighbour_index2 = build_get_index(neighbour_eid2, elist, entry_count);
        if (neighbour_index2 == -1) continue;
        int offset2 = from_u32(elist[neighbour_index2].data + 0x10 + 4 * (2 + 3 * link2.cam_index));
        unsigned int slst2 = build_get_slst(elist[neighbour_index2].data + offset2);

        LIST neig_list = build_get_neighbours(elist[neighbour_index2].data);
        LIST slst_list = init_list();
        list_insert(&slst_list, slst2);

        build_add_collision_dependencies(full_list, 0, cam_length, elist[neighbour_index2].data, collisisons, elist, entry_count);
        coords = build_get_path(elist, zone_index, cam_index, &path_length);
        if ((link.type == 2 && link.flag == 2 && link2.type == 1) || (link.type == 2 && link.flag == 1 && link2.type == 2))
        {
            build_load_list_util_util_back(cam_length, full_list, distance, config[3], coords, path_length, slst_list);
            build_load_list_util_util_back(cam_length, full_list, distance, config[4], coords, path_length, neig_list);
        }
        if ((link.type == 1 && link.flag == 2 && link2.type == 1) || (link.type == 1 && link.flag == 1 && link2.type == 2))
        {
            build_load_list_util_util_forw(cam_length, full_list, distance, config[3], coords, path_length, slst_list);
            build_load_list_util_util_forw(cam_length, full_list, distance, config[4], coords, path_length, neig_list);
        }
    }
}

/** \brief
 *  Reads draw lists of the camera of the zone, returns in a non-delta form.
 *
 * \param elist ENTRY*                  entry list
 * \param zone_index int                index of the zone entry
 * \param cam_index int                 index of the camera item
 * \param cam_length int                length of the path of the camera item
 * \return LIST*                        array of lists as a representation of the draw list (changed to full draw list instead of delta-based)
 */
LIST *build_get_complete_draw_list(ENTRY *elist, int zone_index, int cam_index, int cam_length)
{
    int i, m;
    LIST *draw_list = (LIST *) malloc(cam_length * sizeof(LIST));
    LIST list = init_list();
    for (i = 0; i < cam_length; i++)
        draw_list[i] = init_list();

    int cam_offset = from_u32(elist[zone_index].data + 0x10 + 4 * cam_index);
    char temp[5];
    LOAD_LIST draw_list2 = build_get_lists(ENTITY_PROP_CAM_DRAW_LIST_A, elist[zone_index].data, cam_offset);

    int sublist_index = 0;
    for (i = 0; i < cam_length; i++)
    {
        if (draw_list2.array[sublist_index].index == i)
        {
            if (draw_list2.array[sublist_index].type == 'B')
                for (m = 0; m < draw_list2.array[sublist_index].list_length; m++)
                    list_insert(&list, (draw_list2.array[sublist_index].list[m] & 0xFFFF00) >> 8);

            if (draw_list2.array[sublist_index].type == 'A')
                for (m = 0; m < draw_list2.array[sublist_index].list_length; m++)
                    list_rem(&list, (draw_list2.array[sublist_index].list[m] & 0xFFFF00) >> 8);

            sublist_index++;
        }
        list_copy_in(&draw_list[i], list);
    }

    delete_load_list(draw_list2);

    return draw_list;
}

/** \brief
 *  Retrieves a list of types & subtypes from the IDs and zones.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param entity_list LIST              list of entity IDs
 * \param neighbour_list LIST           list of entries to look into
 * \return LIST                         list of [2B] type & [2B] subtype items
 */
LIST build_get_types_subtypes(ENTRY *elist, int entry_count, LIST entity_list, LIST neighbour_list)
{
    LIST type_subtype_list = init_list();
    int i, j;
    for (i = 0; i < neighbour_list.count; i++)
    {
        int curr_index = build_get_index(neighbour_list.eids[i], elist, entry_count);
        if (curr_index == -1) continue;
        int cam_count = build_get_cam_count(elist[curr_index].data);
        int entity_count = build_get_entity_count(elist[curr_index].data);
        for (j = 0; j < entity_count; j++)
        {
            int entity_offset = from_u32(elist[curr_index].data + 0x10 + 4 * (2 + cam_count + j));
            int ID = build_get_entity_prop(elist[curr_index].data + entity_offset, ENTITY_PROP_ID);
            if (list_find(entity_list, ID) != -1)
            {
                int type = build_get_entity_prop(elist[curr_index].data + entity_offset, ENTITY_PROP_TYPE);
                int subtype = build_get_entity_prop(elist[curr_index].data + entity_offset, ENTITY_PROP_SUBTYPE);
                list_insert(&type_subtype_list, (type << 16) + subtype);
            }
        }
    }

    return type_subtype_list;
}

/** \brief
 *  Calculates total distance the path has between given points, if cap is set it stop at given cap and returns
 *  index at which it stopped (final distance).
 *
 * \param coords short int*             path
 * \param start_index int               start point of the path to be considered
 * \param end_index int                 end point of the path to be considered
 * \param cap int                       distance at which to stop
 * \param final_index int*              index of point where it reached the distance cap
 * \return int                          reached distance
 */
int build_get_distance(short int *coords, int start_index, int end_index, int cap, int *final_index)
{
    int j, distance = 0;
    int index = start_index;

    if (start_index > end_index)
    {
        index = start_index;
        for (j = start_index; j > end_index; j--)
        {
            if (cap != -1)
                if (distance >= cap) break;
            short int x1 = coords[j * 3 - 3];
            short int y1 = coords[j * 3 - 2];
            short int z1 = coords[j * 3 - 1];
            short int x2 = coords[j * 3 + 0];
            short int y2 = coords[j * 3 + 1];
            short int z2 = coords[j * 3 + 2];
            distance += point_distance_3D(x1, x2, y1, y2, z1, z2);
            index = j;
        }
        if (distance < cap)
            index--;
    }
    if (start_index < end_index)
    {
        index = start_index;
        for (j = start_index; j < end_index; j++)
        {
            if (cap != -1)
                if (distance >= cap) break;
            short int x1 = coords[j * 3 + 0];
            short int y1 = coords[j * 3 + 1];
            short int z1 = coords[j * 3 + 2];
            short int x2 = coords[j * 3 + 3];
            short int y2 = coords[j * 3 + 4];
            short int z2 = coords[j * 3 + 5];
            distance += point_distance_3D(x1, x2, y1, y2, z1, z2);
            index = j;
        }
        if (distance < cap)
            index++;
    }


    if (final_index != NULL) *final_index = index;
    return distance;
}

/** \brief
 *  Collects list of IDs using draw lists of neighbouring camera paths, depth max 2, distance less than DRAW_DISTANCE-
 *  Another function handles retrieving entry/subtype list from the ID list.
 *
 * \param point_index int               index of the point currently considered
 * \param zone_index int                index of the entry in the entry list
 * \param camera_index int              index of the camera item currently considered
 * \param cam_length int                length of the camera
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param neighbours LIST*              list of entries it stumbled upon (used later to find types/subtypes of collected IDs)
 * \return LIST                         list of IDs it came across during the search
 */
LIST build_get_entity_list(int point_index, int zone_index, int camera_index, int cam_length, ENTRY *elist, int entry_count, LIST *neighbours, int draw_dist)
{
    LIST entity_list = init_list();
    int i, j, k, coord_count;

    list_copy_in(neighbours, build_get_neighbours(elist[zone_index].data));

    LIST *draw_list_zone = build_get_complete_draw_list(elist, zone_index, camera_index, cam_length);
    for (i = 0; i < cam_length; i++)
        list_copy_in(&entity_list, draw_list_zone[i]);

    short int *coords = build_get_path(elist, zone_index, camera_index, &coord_count);
    LIST links = build_get_links(elist[zone_index].data, camera_index);
    for (i = 0; i < links.count; i++)
    {
        int distance = 0;
        LINK link = int_to_link(links.eids[i]);
        if (link.type == 1)
            distance += build_get_distance(coords, point_index, 0, -1, NULL);
        if (link.type == 2)
            distance += build_get_distance(coords, point_index, cam_length - 1, -1, NULL);

        unsigned int eid_offset = from_u32(elist[zone_index].data + 0x10) + 4 + link.zone_index * 4 + C2_NEIGHBOURS_START;
        unsigned int neighbour_eid = from_u32(elist[zone_index].data + eid_offset);
        int neighbour_index = build_get_index(neighbour_eid, elist, entry_count);
        if (neighbour_index == -1)
            continue;
        list_copy_in(neighbours, build_get_neighbours(elist[neighbour_index].data));

        int neighbour_cam_length;
        short int *coords2 = build_get_path(elist, neighbour_index, 2 + 3 * link.cam_index, &neighbour_cam_length);
        LIST* draw_list_neighbour1 = build_get_complete_draw_list(elist, neighbour_index, 2 + 3 * link.cam_index, neighbour_cam_length);


        int point_index2;
        if (link.flag == 1)
        {
            distance += build_get_distance(coords2, 0, neighbour_cam_length - 1, draw_dist - (distance + BACKWARDS_PENALTY), &point_index2);
            for (j = 0; j < point_index2; j++)
                list_copy_in(&entity_list, draw_list_neighbour1[j]);
        }
        if (link.flag == 2)
        {
            distance += build_get_distance(coords2, neighbour_cam_length - 1, 0, draw_dist - distance, &point_index2);
            for (j = point_index2; j < neighbour_cam_length - 1; j++)
                list_copy_in(&entity_list, draw_list_neighbour1[j]);
        }

        /*char temp[100], temp2[100];
        printf("%s point %2d neighbour %s: got to point %2d / %2d\n",
               eid_conv(elist[zone_index].EID, temp), point_index, eid_conv(elist[neighbour_index].EID, temp2), point_index2 + 1, neighbour_cam_length);*/

        if (distance >= draw_dist)
            continue;

        LIST layer2 = build_get_links(elist[neighbour_index].data, 2 + 3 * link.cam_index);
        for (j = 0; j < layer2.count; j++)
        {
            LINK link2 = int_to_link(layer2.eids[j]);
            unsigned int eid_offset2 = from_u32(elist[neighbour_index].data + 0x10) + 4 + link2.zone_index * 4 + C2_NEIGHBOURS_START;
            unsigned int neighbour_eid2 = from_u32(elist[neighbour_index].data + eid_offset2);
            int neighbour_index2 = build_get_index(neighbour_eid2, elist, entry_count);
            if (neighbour_index2 == -1) continue;
            list_copy_in(neighbours, build_get_neighbours(elist[neighbour_index2].data));

            int neighbour_cam_length2;
            short int *coords3 = build_get_path(elist, neighbour_index2, 2 + 3 * link2.cam_index, &neighbour_cam_length2);
            LIST* draw_list_neighbour2 = build_get_complete_draw_list(elist, neighbour_index2, 2 + 3 * link2.cam_index, neighbour_cam_length2);


            int point_index3 = -2;
            // start to end
            if ((link.flag == 1 && link2.type == 2 && link2.flag == 1) ||
                (link.flag == 2 && link2.type == 1 && link2.flag == 1))
            {
                build_get_distance(coords3, 0, neighbour_cam_length2 - 1, draw_dist - distance, &point_index3);
                for (k = 0; k < point_index3; k++)
                    list_copy_in(&entity_list, draw_list_neighbour2[k]);
            }

            // end to start
            if ((link.flag == 1 && link2.type == 2 && link2.flag == 2) ||
                (link.flag == 2 && link2.type == 1 && link2.flag == 2))
            {
                build_get_distance(coords3, neighbour_cam_length2 - 1, 0, draw_dist - (distance + BACKWARDS_PENALTY), &point_index3);
                for (k = point_index3; k < neighbour_cam_length2; k++)
                    list_copy_in(&entity_list, draw_list_neighbour2[k]);
            }

            /*char temp[100], temp2[100];
            if (point_index3 != -2)
                printf("%s point %2d neighbour %s: got to point %2d / %2d\n",
                   eid_conv(elist[zone_index].EID, temp), point_index, eid_conv(elist[neighbour_index2].EID, temp2), point_index3 + 1, neighbour_cam_length2);*/
        }
    }

    return entity_list;
}

/** \brief
 *  Handles most load list creating, except for permaloaded and sound entries.
 *  First part is used for neighbours & scenery & slsts.
 *  Second part is used for draw lists.
 *
 * \param zone_index int                index of the zone in the entry list
 * \param camera_index int              index of the camera item in the zone
 * \param listA LIST*                   load list A, later transformed into actual load list data etc
 * \param listB LIST*                   load list B, later transformed into actual load list data and placed into the entry
 * \param cam_length int                length of the camera
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \return void
 */
void build_load_list_util(int zone_index, int camera_index, LIST* full_list, int cam_length, ENTRY *elist, int entry_count, DEPENDENCIES sub_info, DEPENDENCIES collisions, int *config)
{
    int i, j, k;

    LIST links = build_get_links(elist[zone_index].data, camera_index);
    for (i = 0; i < links.count; i++)
        build_load_list_util_util(zone_index, camera_index, links.eids[i], full_list, cam_length, elist, entry_count, config, collisions);

    for (i = 0; i < cam_length; i++)
    {
        LIST neighbour_list = init_list();
        char temp[100];
        // printf("gonna do %s\n", eid_conv(elist[zone_index].EID, temp));
        LIST entity_list = build_get_entity_list(i, zone_index, camera_index, cam_length, elist, entry_count, &neighbour_list, config[5]);
        LIST types_subtypes = build_get_types_subtypes(elist, entry_count, entity_list, neighbour_list);

        for (j = 0; j < types_subtypes.count; j++)
        {
            int type = types_subtypes.eids[j] >> 16;
            int subtype = types_subtypes.eids[j] & 0xFF;

            // char temp[100];
            // printf("%s point %2d to load type %2d subtype %2d stuff\n", eid_conv(elist[zone_index].EID, temp), i, type, subtype);
            for (k = 0; k < sub_info.count; k++)
                if (sub_info.array[k].subtype == subtype && sub_info.array[k].type == type)
                    list_copy_in(&full_list[i], sub_info.array[k].dependencies);
        }
    }

}

/** \brief
 *  Makes a load list property from the input arrays, later its put into the item.
 *
 * \param list_array LIST*              array of lists containing the full load list (already in delta form)
 * \param cam_length int                length of the camera and so the array too
 * \param code int                      property code
 * \return PROPERTY                     property with game's format
 */
PROPERTY build_make_load_list_prop(LIST *list_array, int cam_length, int code)
{
    int i, j, delta_counter = 0, total_length = 0;
    PROPERTY prop;

    for (i = 0; i < cam_length; i++)
        if (list_array[i].count != 0)
        {
            total_length += list_array[i].count * 4 + 4;
            delta_counter++;
        }

    *(short int *) (prop.header) = code;
    *(short int *) (prop.header + 4) = 0x0464;
    *(short int *) (prop.header + 6) = delta_counter;

    prop.length = total_length;
    prop.data = (unsigned char *) malloc(total_length * sizeof(unsigned char));

    int indexer = 0;
    int offset = 4 * delta_counter;
    for (i = 0; i < cam_length; i++)
        if (list_array[i].count != 0)
        {
            *(short int *) (prop.data + 2 * indexer) = list_array[i].count;
            *(short int *) (prop.data + 2 * (indexer + delta_counter)) = i;
            for (j = 0; j < list_array[i].count; j++)
                *(int *) (prop.data + offset + 4 * j) = list_array[i].eids[j];
            offset += list_array[i].count * 4;
            indexer++;
        }

    return prop;
}

/** \brief
 *  Lets the user know whether there are any entities whose type/subtype has no specified dependencies.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param sub_info DEPENDENCIES         list of types and subtypes and their potential dependencies
 * \return void
 */
void build_find_unspecified_entities(ENTRY *elist, int entry_count, DEPENDENCIES sub_info)
{
    int i, j, k;
    LIST considered = init_list();
    for (i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE)
        {
            int cam_count = build_get_cam_count(elist[i].data);
            int entity_count = build_get_entity_count(elist[i].data);
            for (j = 0; j < entity_count; j++)
            {
                unsigned char *entity = elist[i].data + from_u32(elist[i].data + 0x10 + (2 + cam_count + j) * 4);
                int type = build_get_entity_prop(entity, ENTITY_PROP_TYPE);
                int subt = build_get_entity_prop(entity, ENTITY_PROP_SUBTYPE);
                if (list_find(considered, (type << 16) + subt) != -1)
                    continue;

                list_insert(&considered, (type << 16) + subt);

                int found = 0;
                for (k = 0; k < sub_info.count; k++)
                    if (sub_info.array[k].subtype == subt && sub_info.array[k].type == type)
                        found = 1;
                if (!found)
                    printf("Entity with type %2d subtype %2d has no specified dependency list!\n", type, subt);
            }
        }
}

/** \brief
 *  Converts the full load list into delta form.
 *
 * \param full_load LIST*               full form of load list
 * \param listA LIST*                   new delta based load list, A side
 * \param listB LIST*                   new delta based load list, B side
 * \param cam_length int                length of the current camera
 * \return void
 */
void build_load_list_to_delta(LIST *full_load, LIST *listA, LIST *listB, int cam_length, ENTRY *elist, int entry_count)
{
    int i, j;
    for (i = 0; i < full_load[0].count; i++)
        if (build_get_index(full_load[0].eids[i], elist, entry_count) != -1)
            list_insert(&listA[0], full_load[0].eids[i]);

    int n = cam_length - 1;
    for (i = 0; i < full_load[n].count; i++)
        if (build_get_index(full_load[n].eids[i], elist, entry_count) != -1)
            list_insert(&listB[n], full_load[n].eids[i]);

    for (i = 1; i < cam_length; i++)
    {
        for (j = 0; j < full_load[i].count; j++)
            if ((list_find(full_load[i - 1], full_load[i].eids[j]) == -1) && build_get_index(full_load[i].eids[j], elist, entry_count) != -1)
                list_insert(&listA[i], full_load[i].eids[j]);

        for (j = 0; j < full_load[i - 1].count; j++)
            if ((list_find(full_load[i], full_load[i - 1].eids[j]) == -1) && build_get_index( full_load[i - 1].eids[j], elist, entry_count) != -1)
                list_insert(&listB[i - 1], full_load[i - 1].eids[j]);
    }


    for (i = 0; i < cam_length; i++)
    {
        LIST copy = init_list();
        list_copy_in(&copy, listA[i]);

        for (j = 0; j < copy.count; j++)
            if (list_find(listB[i], copy.eids[j]) != -1)
            {
                list_rem(&listA[i], copy.eids[j]);
                list_rem(&listB[i], copy.eids[j]);
            }
    }
}


/** \brief
 *  A function that for each zone's each camera path creates new load lists using
 *  the provided list of permanently loaded entries and
 *  the provided list of dependencies of certain entitites, gets models from the animations.
 *  For the zone itself and its linked neighbours it adds all relatives and their dependencies.
 *  All sounds always get added, Cr10T and Cr20T always get added.
 *  Load list properties get removed and then replaced by the provided list using 'camera_alter' function.
 *  Load lists get sorted later during the nsd writing process.
 *  Checks for invalid references and does not proceed if any are found.
 *
 * \param elist ENTRY*                  list of current entries, array of ENTRY
 * \param entry_count int               current amount of entries
 * \param gool_table unsigned int*      table that contains GOOL entries on their expected slot
 * \param permaloaded LIST              list of permaloaded entries
 * \param subtype_info DEPENDENCIES     list of {type, subtype, count, dependencies[count]}
 *
 * \return void
 */
void build_make_load_lists(ENTRY *elist, int entry_count, unsigned int *gool_table, LIST permaloaded, DEPENDENCIES subtype_info, DEPENDENCIES collision, int *config)
{
    int i, j, k, l;

    for (i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE)
        {
            int item1off = from_u32(elist[i].data + 0x10);
            int cam_count = build_get_cam_count(elist[i].data) / 3;

            char temp[100];
            printf("Doing load lists for zone %s\n", eid_conv(elist[i].EID, temp));
            for (j = 0; j < cam_count; j++)
            {
                printf("\t cam path %d\n", j);
                int cam_length = build_get_path_length(elist[i].data + from_u32(elist[i].data + 0x10 + 4 * (2 + 3 * j)));
                LIST full_load[cam_length] = {init_list()};

                for (k = 0; k < cam_length; k++)
                    list_copy_in(&full_load[k], permaloaded);


                if (elist[i].related != NULL)
                for (k = 0; (unsigned) k < elist[i].related[0]; k++)
                    for (l = 0; l < cam_length; l++)
                        list_insert(&full_load[l], elist[i].related[k + 1]);


                for (k = 0; k < entry_count; k++)
                if (build_entry_type(elist[k]) == ENTRY_TYPE_SOUND) {
                    for (l = 0; l < cam_length; l++)
                        list_insert(&full_load[l], elist[k].EID);
                }

                LIST neighbours = build_get_neighbours(elist[i].data);
                for (k = 0; k < cam_length; k++)
                    list_copy_in(&full_load[k], neighbours);


                int scenery_count = build_get_scen_count(elist[i].data);
                for (k = 0; k < scenery_count; k++)
                {
                    int scenery_index = build_get_index(from_u32(elist[i].data + item1off + 0x4 + 0x30 * k), elist, entry_count);
                    for (l = 0; l < cam_length; l++)
                        build_add_scen_textures_to_list(elist[scenery_index].data, &full_load[l]);
                }

                build_load_list_util(i, 2 + 3 * j, full_load, cam_length, elist, entry_count, subtype_info, collision, config);

                LIST listA[cam_length] = {init_list()};
                LIST listB[cam_length] = {init_list()};
                build_load_list_to_delta(full_load, listA, listB, cam_length, elist, entry_count);

                PROPERTY prop_0x208 = build_make_load_list_prop(listA, cam_length, 0x208);
                PROPERTY prop_0x209 = build_make_load_list_prop(listB, cam_length, 0x209);

                build_entity_alter(&elist[i], 2 + 3 * j, build_rem_property, NULL, 0x208);
                build_entity_alter(&elist[i], 2 + 3 * j, build_rem_property, NULL, 0x209);
                build_entity_alter(&elist[i], 2 + 3 * j, build_add_property, &prop_0x208, 0x208);
                build_entity_alter(&elist[i], 2 + 3 * j, build_add_property, &prop_0x209, 0x209);
            }
        }

    build_find_unspecified_entities(elist, entry_count, subtype_info);
}

/** \brief
 *  Reads the info from file the user has to provide, first part has permaloaded entries,
 *  second has a list of type/subtype dependencies
 *
 * \param permaloaded LIST*             list of permaloaded entries (created here)
 * \param subtype_info DEPENDENCIES*    list of type/subtype dependencies (created here)
 * \param file_path char*               path of the input file
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \return int                          1 if all went good, 0 if something is wrong
 */
int build_read_entry_config(LIST *permaloaded, DEPENDENCIES *subtype_info, DEPENDENCIES *collisions, char fpaths[FPATH_COUNT][MAX], ENTRY *elist, int entry_count, unsigned int *gool_table)
{
    int i, j, perma_count, subcount, valid = 1;
    char temp[6];
    LIST perma = init_list();

    FILE *file = fopen(fpaths[0], "r");
    if (file == NULL)
        return 0;


    fscanf(file, "%d", &perma_count);
    for (i = 0; i < perma_count; i++)
    {
        fscanf(file, "%5s\n", temp);
        int index = build_get_index(eid_to_int(temp), elist, entry_count);
        if (index == -1)
        {
            printf("[ERROR] invalid permaloaded entry, won't proceed :\t%s\n", temp);
            valid = 0;
            continue;
        }

        list_insert(&perma, eid_to_int(temp));

        if (build_entry_type(elist[index]) == ENTRY_TYPE_ANIM)
        {
            unsigned int model = build_get_model(elist[index].data);
            list_insert(&perma, model);

            int model_index = build_get_index(model, elist, entry_count);
            if (model_index == -1) continue;

            build_add_model_textures_to_list(elist[model_index].data, &perma);
        }
    }

    fclose(file);
    file = fopen(fpaths[1], "r");
    if (file == NULL)
        return 0;
    fscanf(file, "%d", &subcount);

    DEPENDENCIES subinfo;
    subinfo.count = subcount;
    subinfo.array = (INF *) malloc(subcount * sizeof(INF));
    int type, subtype, counter;
    for (i = 0; i < subcount; i++)
    {
        fscanf(file, "%d, %d, %d", &type, &subtype, &counter);
        subinfo.array[i].type = type;
        subinfo.array[i].subtype = subtype;
        subinfo.array[i].dependencies = init_list();
        list_insert(&subinfo.array[i].dependencies, gool_table[type]);
        for (j = 0; j < counter; j++)
        {
            fscanf(file, ", %5s", temp);
            int index = build_get_index(eid_to_int(temp), elist, entry_count);
            if (index == -1)
            {
                printf("[warning] unknown entry reference, will be skipped:\t%s\n", temp);
                continue;
            }
            list_insert(&(subinfo.array[i].dependencies), eid_to_int(temp));

            if (build_entry_type(elist[index]) == ENTRY_TYPE_ANIM)
            {
                unsigned int model = build_get_model(elist[index].data);
                list_insert(&subinfo.array[i].dependencies, model);

                int model_index = build_get_index(model, elist, entry_count);
                if (model_index == -1) continue;

                build_add_model_textures_to_list(elist[model_index].data, &subinfo.array[i].dependencies);
            }
        }
    }

    fclose(file);
    file = fopen(fpaths[2], "r");
    if (file == NULL)
        return 0;

    int coll_count;
    fscanf(file,"%d", &coll_count);
    DEPENDENCIES coll;
    coll.count = coll_count;
    coll.array = (INF *) malloc(coll_count * sizeof(INT));
    for (i = 0; i < coll_count; i++)
    {
        int code;
        fscanf(file, "%x, %d", &code, &counter);
        coll.array[i].type = code;
        coll.array[i].subtype = -1;
        coll.array[i].dependencies = init_list();
        for (j = 0; j < counter; j++)
        {
            fscanf(file, ", %5s", temp);
            int index = build_get_index(eid_to_int(temp), elist, entry_count);
            if (index == -1)
            {
                printf("[warning] unknown entry reference, will be skipped:\t%s\n", temp);
                continue;
            }

            list_insert(&(coll.array[i].dependencies), eid_to_int(temp));

            if (build_entry_type(elist[index]) == ENTRY_TYPE_ANIM)
            {
                unsigned int model = build_get_model(elist[index].data);
                list_insert(&coll.array[i].dependencies, model);

                int model_index = build_get_index(model, elist, entry_count);
                if (model_index == -1) continue;

                build_add_model_textures_to_list(elist[model_index].data, &coll.array[i].dependencies);
            }
        }

    }

    *permaloaded = perma;
    *subtype_info = subinfo;
    *collisions = coll;
    fclose(file);
    if (!valid)
    {
        printf("Cannot proceed with invalid items, fix that\n");
        return 0;
    }
    return 1;
}

/** \brief
 *  Just calculates the amount of chunks in the provided nsf file.
 *
 * \param nsf FILE*                     provided nsf
 * \return int                          amount of chunks
 */
int build_get_chunk_count_base(FILE *nsf)
{
    fseek(nsf, 0, SEEK_END);
    int result = ftell(nsf) / CHUNKSIZE;
    rewind(nsf);

    return result;
}

/** \brief
 *  Asks the user for built level's ID.
 *
 * \return int                          picked level ID
 */
int build_ask_ID()
{
    int level_ID;
    printf("\nWhat ID do you want your level to have? (hex 0 - 3F) [CAN OVERWRITE EXISTING FILES!]\n");
    scanf("%x", &level_ID);
    if (level_ID < 0 || level_ID > 0x3F) {
        printf("Invalid ID, defaulting to 1\n");
        level_ID = 1;
    }

    return level_ID;
}

/** \brief
 *  Asks for path to the list with the permaloaded stuff and type/subtype info.
 *
 * \param fpath char*                   path to the file
 * \return void
 */
void build_ask_list_paths(char fpaths[FPATH_COUNT][MAX])
{
    printf("\nInput the path to the file with permaloaded entries:\n");
    scanf(" %[^\n]",fpaths[0]);
    if (fpaths[0][0]=='\"')
    {
        strcpy(fpaths[0],fpaths[0] + 1);
        *(strchr(fpaths[0],'\0') - 1) = '\0';
    }

    /*printf("\nInput the path to the file with type/subtype dependencies:\n");
    scanf(" %[^\n]",fpath2);
    if (fpath2[0]=='\"')
    {
        strcpy(fpath2,fpath2 + 1);
        *(strchr(fpath2,'\0') - 1) = '\0';
    }*/
    strcpy(fpaths[1], "complete entity list vanilla.txt");
    strcpy(fpaths[2], "collision list.txt");
}

/** \brief
 *  Creates and builds chunks for the instrument entries according to their format.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_count int*              chunk count
 * \param chunks unsigned char**        chunks, array of ptrs to data, allocated here
 * \return void
 */
void build_instrument_chunks(ENTRY *elist, int entry_count, int *chunk_count, unsigned char** chunks)
{
    int count = *chunk_count;
    for (int i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_INST)
        {
            elist[i].chunk = count;
            chunks[count] = (unsigned char *) calloc(CHUNKSIZE, sizeof(unsigned char));
            *(unsigned short int *)(chunks[count]) = MAGIC_CHUNK;
            *(unsigned short int *)(chunks[count] + 2) = CHUNK_TYPE_INSTRUMENT;
            *(unsigned short int *)(chunks[count] + 4) = 2 * count + 1;

            *(unsigned int *)(chunks[count] + 8) = 1;
            *(unsigned int *)(chunks[count] + 0x10) = 0x24;
            *(unsigned int *)(chunks[count] + 0x14) = 0x24 + elist[i].esize;

            memcpy(chunks[count] + 0x24, elist[i].data, elist[i].esize);
            *((unsigned int *)(chunks[count] + 0xC)) = nsfChecksum(chunks[count]);
            count++;
        }

    *chunk_count = count;
}

/** \brief
 *  Creates and builds sound chunks, formats accordingly.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_count int*              chunk count
 * \param chunks unsigned char**        chunks, array of ptrs to data, allocated here
 * \return void
 */
void build_sound_chunks(ENTRY *elist, int entry_count, int *chunk_count, unsigned char** chunks)
{
    int indexer, i, j, count = *chunk_count;
    int sound_entry_count = 0;

    for (i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_SOUND)
            sound_entry_count++;

    ENTRY sound_list[sound_entry_count];

    indexer = 0;
    for (i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_SOUND)
            sound_list[indexer++] = elist[i];


    qsort(sound_list, sound_entry_count, sizeof(ENTRY), cmp_entry_size);

    int sizes[8];
    for (i = 0; i < 8; i++)
        sizes[i] = 0x14;

    for (i = 0; i < sound_entry_count; i++)
        for (j = 0; j < 8; j++)
            if (sizes[j] + 4 + (((sound_list[i].esize + 15) >> 4) << 4) <= CHUNKSIZE)
            {
                sound_list[i].chunk = count + j;
                sizes[j] += 4 + (((sound_list[i].esize + 15) >> 4) << 4);
                break;
            }


    int snd_chunk_count;
    for (i = 0; i < 8; i++)
        if (sizes[i] > 0x14)
            snd_chunk_count = i + 1;

    qsort(sound_list, sound_entry_count, sizeof(ENTRY), cmp_entry_eid);

    for (i = 0; i < snd_chunk_count; i++)
    {
        int local_entry_count = 0;
        int chunk_no = 2 * (count + i) + 1;
        chunks[count + i] = (unsigned char*) calloc(CHUNKSIZE, sizeof(unsigned char));

        for (j = 0; j < sound_entry_count; j++)
            if (sound_list[j].chunk == count + i)
                local_entry_count++;

        unsigned int offsets[local_entry_count + 2];
        *(unsigned short int *) chunks[count + i] = MAGIC_CHUNK;
        *(unsigned short int *)(chunks[count + i] + 2) = CHUNK_TYPE_SOUND;
        *(unsigned short int *)(chunks[count + i] + 4) = chunk_no;
        *(unsigned short int *)(chunks[count + i] + 8) = local_entry_count;

        indexer = 0;
        offsets[indexer] = build_align_sound(0x10 + (local_entry_count + 1) * 4);

        for (j = 0; j < sound_entry_count; j++)
            if (sound_list[j].chunk == count + i)
            {
                offsets[indexer + 1] = build_align_sound(offsets[indexer] + sound_list[j].esize);
                indexer++;
            }


        for (j = 0; j < local_entry_count + 1; j++)
            *(unsigned int *) (chunks[count + i] + 0x10 + j * 4) = offsets[j];

        indexer = 0;
        for (j = 0; j < sound_entry_count; j++)
            if (sound_list[j].chunk == count + i)
                memcpy(chunks[count + i] + offsets[indexer++], sound_list[j].data, sound_list[j].esize);


        *(unsigned int*)(chunks[count + i] + 0xC) = nsfChecksum(chunks[count + i]);
    }

    for (i = 0; i < entry_count; i++)
        for (j = 0; j < sound_entry_count; j++)
            if (elist[i].EID == sound_list[j].EID)
                elist[i].chunk = sound_list[j].chunk;

    *chunk_count = count + snd_chunk_count;
}

/** \brief
 *  Assigns primary chunks to all entries, merges need them.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_count int*              chunk count
 * \param config int*                   config
 * \return void
 */
void build_assign_primary_chunks_all(ENTRY *elist, int entry_count, int *chunk_count, int *config)
{
    for (int i = 0; i < entry_count; i++)
        if (build_is_normal_chunk_entry(elist[i]) && elist[i].chunk == -1)
            elist[i].chunk = (*chunk_count)++;
}

/** \brief
 *  Checks whether the entry is meant to be placed into a normal chunk.
 *
 * \param entry ENTRY                   entry to be tested
 * \return int                          1 if it belongs to a normal chunk, else 0
 */
int build_is_normal_chunk_entry(ENTRY entry)
{
    int type = build_entry_type(entry);
    if (type == ENTRY_TYPE_ANIM ||
        type == ENTRY_TYPE_DEMO ||
        type == ENTRY_TYPE_ZONE ||
        type == ENTRY_TYPE_MODEL ||
        type == ENTRY_TYPE_SCENERY ||
        type == ENTRY_TYPE_SLST ||
        type == ENTRY_TYPE_DEMO ||
        type == ENTRY_TYPE_VCOL ||
        type == ENTRY_TYPE_MIDI ||
        type == ENTRY_TYPE_GOOL )
    return 1;

    return 0;
}

/** \brief
 *  Specially merges permaloaded entries as they dont require any association.
 *  Works similarly to the sound chunk one.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_border_sounds int       index from which normal chunks start
 * \param chunk_count int*              current chunk count
 * \param permaloaded LIST              list with permaloaded entries
 * \return void
 */
void build_permaloaded_merge(ENTRY *elist, int entry_count, int chunk_border_sounds, int *chunk_count, LIST permaloaded)
{
    int i, j;
    int count = *chunk_count;
    int perma_normal_entry_count = 0;

    for (i = 0; i < entry_count; i++)
        if (list_find(permaloaded, elist[i].EID) != -1 && build_is_normal_chunk_entry(elist[i]))
            perma_normal_entry_count++;

    ENTRY list[perma_normal_entry_count];
    int indexer = 0;
    for (i = 0; i < entry_count; i++)
        if (list_find(permaloaded, elist[i].EID) != -1 && build_is_normal_chunk_entry(elist[i]))
            list[indexer++] = elist[i];

    qsort(list, perma_normal_entry_count, sizeof(ENTRY), cmp_entry_size);

    int perma_chunk_count = 250;
    int sizes[perma_chunk_count];
    for (i = 0; i < perma_chunk_count; i++)
        sizes[i] = 0x14;

    for (i = 0; i < perma_normal_entry_count; i++)
        for (j = 0; j < MAX; j++)
            if (sizes[j] + 4 + list[i].esize <= CHUNKSIZE)
            {
                list[i].chunk = count + j;
                sizes[j] += 4 + list[i].esize;
                break;
            }

    for (i = 0; i < entry_count; i++)
        for (j = 0; j < perma_normal_entry_count; j++)
            if (elist[i].EID == list[j].EID)
                elist[i].chunk = list[j].chunk;

    int counter;
    for (i = 0; i < 8; i++)
        if (sizes[i] > 0x14)
            counter = i + 1;

    *chunk_count = count + counter;
}

/** \brief
 *  Calls merge functions.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_border_sounds int       index of last sound chunk
 * \param chunk_count int*              chunk count
 * \param config2 int                   0/1, determines whether each point of cam paths or only main/deltas are considered
 * \return void
 */
void build_merge_main(ENTRY *elist, int entry_count, int chunk_border_sounds, int *chunk_count, int config2, LIST permaloaded)
{
    build_matrix_merge_main(elist, entry_count, chunk_border_sounds, chunk_count, config2);
    deprecate_build_payload_merge(elist, entry_count, chunk_border_sounds, chunk_count);
    build_dumb_merge(elist, chunk_border_sounds, chunk_count, entry_count);
}

/** \brief
 *  Gets rid of some dynamically allocated stuff and closes files.
 *
 * \param nsf FILE*                     nsf file
 * \param nsfnew FILE*                  created nsf file
 * \param df DIR*                       directory with the entries
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunks unsigned char**        built chunks
 * \param chunk_count int               chunk count
 * \return void
 */
void build_final_cleanup(FILE *nsf, FILE *nsfnew, DIR *df, ENTRY *elist, int entry_count, unsigned char **chunks, int chunk_count)
{
    for (int i = 0; i < chunk_count; i++)
        free(chunks[i]);

    for (int i = 0; i < entry_count; i++) {
        if (elist[i].data != NULL)
            free(elist[i].data);
        if (elist[i].related != NULL)
            free(elist[i].related);
    }

    fclose(nsfnew);
    fclose(nsf);
    closedir(df);
}

void build_ask_distances(int *config)
{
    int temp;
    printf("\nSLST distance? (recommended is approx 6000)\n");
    scanf("%d", &temp);
    config[3] = temp;

    printf("\nNeighbour distance? (recommended is approx 6000)\n");
    scanf("%d", &temp);
    config[4] = temp;

    printf("\nDraw list distance? (recommended is approx 6000)\n");
    scanf("%d", &temp);
    config[5] = temp;
}

void build_get_box_count(ENTRY *elist, int entry_count)
{
    int counter = 0;
    int nitro_counter = 0;
    for (int i = 0; i < entry_count; i++)
    {
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE)
        {
            int entity_count = build_get_entity_count(elist[i].data);
            int camera_count = build_get_cam_count(elist[i].data);
            for (int j = 0; j < entity_count; j++)
            {
                unsigned char *entity = elist[i].data + from_u32(elist[i].data + 0x10 + 4 * (2 + camera_count + j));
                int type = build_get_entity_prop(entity, ENTITY_PROP_TYPE);
                int subt = build_get_entity_prop(entity, ENTITY_PROP_SUBTYPE);
                int ID = build_get_entity_prop(entity, ENTITY_PROP_ID);

                if ((type >= 34 && type <= 43) &&
                    (subt == 0 || subt == 2 || subt == 3 || subt == 4 || subt == 6 || subt == 8 || subt == 9 ||
                     subt == 10 || subt == 11 || subt == 18 || subt == 23 || subt == 25 || subt == 26))
                /*if (ID != -1)*/
                {
                    counter++;
                    char temp[100];
                    printf("Zone: %5s, type: %2d, subtype: %2d, ID: %3d\n", eid_conv(elist[i].EID, temp), type, subt, ID);
                }

                if ((type >= 34 && type <= 43) && subt == 18)
                {
                    nitro_counter++;
                    if (type == 43)
                        printf("gotem %d\n", ID);
                    printf("NITRO %3d\n", ID);
                }
            }
        }
    }
    printf("BOX COUNT:   %3d\n", counter);
    printf("NITRO COUNT: %3d\n", nitro_counter);
}


/** \brief
 *  Reads nsf, reads folder, collects relatives, assigns proto chunks, calls some merge functions, makes load lists, makes nsd, makes nsf, end.
 *
 * \param nsfpath char*                 path to the base nsf
 * \param dirpath char*                 path to the folder whose contents are to be added
 * \param chunkcap int                  unused
 * \param status INFO                   used for prints and stuff
 * \param time char*                    string with time used for saving or not
 * \return void
 */
void build_main(char *nsfpath, char *dirpath, int chunkcap, INFO status, char *time)
{
    char lcltemp[MAX], fpaths[FPATH_COUNT][MAX] = {0};
    DIR *df = NULL;
    FILE *nsf = NULL, *nsfnew = NULL;
    SPAWNS spawns = init_spawns();
    ENTRY elist[2500];
    unsigned char *chunks[1024];
    LIST permaloaded;
    DEPENDENCIES subtype_info;
    DEPENDENCIES collisions;
    int i, level_ID;
    int chunk_border_base       = 0,
        chunk_border_texture    = 0,
        chunk_border_sounds     = 0,
        chunk_count             = 0,
        entry_count_base        = 0,
        entry_count             = 0;

    unsigned int gool_table[0x40];
    for (i = 0; i < 0x40; i++)
        gool_table[i] = NONE;

    /* config: */
    // 0 - [gool initial merge flag]    0 - group       |   1 - one by one
    // 1 - [zone initial merge flag]    0 - group       |   1 - one by one
    // 2 - [merge type flag]            0 - per delta   |   1 - per point
    // 3 - [slst distance]
    // 4 - [neighbour distance]
    // 5 - [draw list distance]
    int config[6] = {1, 1, 1, 0, 0, 0};


    if ((nsf = fopen(nsfpath,"rb")) == NULL) {
        printf("[ERROR] Could not open selected NSF\n");
        return;
    }

    if ((df = opendir(dirpath)) == NULL) {
        printf("[ERROR] Could not open selected directory\n");
        fclose(nsf);
        return;
    }

    level_ID = build_ask_ID();

    chunk_border_base = build_get_chunk_count_base(nsf);
    //build_read_nsf(elist, chunk_border_base, chunks, &chunk_border_texture, &entry_count, nsf, gool_table);
    entry_count_base = entry_count;
    build_read_folder(df, dirpath, chunks, elist, &chunk_border_texture, &entry_count, &spawns, gool_table);

    build_get_model_references(elist, entry_count);
    build_remove_invalid_references(elist, entry_count, entry_count_base);
    chunk_count = chunk_border_texture;
    qsort(elist, entry_count, sizeof(ENTRY), cmp_entry_eid);

    build_instrument_chunks(elist, entry_count, &chunk_count, chunks);
    build_sound_chunks(elist, entry_count, &chunk_count, chunks);
    chunk_border_sounds = chunk_count;

    build_ask_list_paths(fpaths);
    if (!build_read_entry_config(&permaloaded, &subtype_info, &collisions, fpaths, elist, entry_count, gool_table)) {
        printf("File could not be opened or a different error occured\n");
        return;
    }

    build_ask_distances(config);
    build_make_load_lists(elist, entry_count, gool_table, permaloaded, subtype_info, collisions, config);
    build_permaloaded_merge(elist, entry_count, chunk_border_sounds, &chunk_count, permaloaded);
    build_assign_primary_chunks_all(elist, entry_count, &chunk_count, config);
    build_merge_main(elist, entry_count, chunk_border_sounds, &chunk_count, config[2], permaloaded);

    //build_get_box_count(elist, entry_count); // snow no bullshit

    *(strrchr(nsfpath,'\\') + 1) = '\0';
    sprintf(lcltemp,"%s\\S00000%02X.NSF", nsfpath, level_ID);
    nsfnew = fopen(lcltemp, "wb");
    *(strchr(lcltemp, '\0') - 1) = 'D';

    // build_print_relatives(elist, entry_count);

    build_write_nsd(lcltemp, elist, entry_count, chunk_count, spawns, gool_table, level_ID);
    build_normal_chunks(elist, entry_count, chunk_border_sounds, chunk_count, chunks);
    for (i = 0; i < chunk_count; i++)
        fwrite(chunks[i], sizeof(unsigned char), CHUNKSIZE, nsfnew);

    build_final_cleanup(nsf, nsfnew, df, elist, entry_count, chunks, chunk_count);
}
