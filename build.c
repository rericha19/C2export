#include "macros.h"
#define PRINTING 0


/** \brief
 *  Just returns the model reference of an animation.
 *  Only looks at the first frame of the animation.
 *
 * \param anim unsigned char*           pointer to the data of the searched animation
 * \return unsigned int                 EID of the animation's model reference (as unsigned int)
 */
unsigned int get_model(unsigned char *anim) {
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
int remove_empty_chunks(int index_start, int index_end, int entry_count, ENTRY *entry_list)
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
 *  Might have become deprecate already.
 *
 * \param elist ENTRY*                  list of entries
 * \param entry_count int               current amount of entries
 * \param entry_count_base int
 * \return void
 */
void remove_invalid_references(ENTRY *elist, int entry_count, int entry_count_base)
{
    int i, j, k;
    unsigned int new_relatives[250];

    for (i = 0; i < entry_count; i++)
    {
        if (elist[i].related == NULL) continue;
        for (j = 1; (unsigned) j < elist[i].related[0] + 1; j++)
        {
            int relative_index;
            relative_index = get_index(elist[i].related[j], elist, entry_count);
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

        //qsort(elist[i].related + 1, elist[i].related[0], sizeof(unsigned int), cmpfunc);
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
int get_base_chunk_border(unsigned int textr, unsigned char **chunks, int index_end)
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
void get_model_references(ENTRY *elist, int entry_count)
// gets model references lmao
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
                if ((relative_index = get_index(elist[i].related[j + 1], elist, entry_count)) >= 0)
                    if (elist[relative_index].data != NULL && (from_u32(elist[relative_index].data + 8) == 1))
                        new_relatives[new_counter++] = get_model(elist[relative_index].data);

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
int get_index(unsigned int eid, ENTRY *elist, int entry_count)
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
unsigned int get_slst(unsigned char *item)
{
    int i, offset = 0;
    for (i = 0; i < item[0xC]; i++)
        if ((from_u16(item + 0x10 + 8 * i)) == ENTITY_PROP_CAM_SLST)
            offset = 0xC + from_u16(item + 0x10 + 8 * i + 4);

    if (offset) return from_u32(item + offset + 4);
        else return 0;
}


/** \brief
 *  Searches the entity, if it has (correct) type and subtype and coords property,
 *  returns them as int[3]. Usually set to accept willy and checkpoint entities.
 *
 * \param item unsigned char*           entity data
 * \return int*                         int[3] with xyz coords the entity if it meets criteria or NULL
 */
int* seek_spawn(unsigned char *item)
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
            coords[i] = *(short int*)(item + coords_offset + 4 + 2 * i) * 4;
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
int get_neighbour_count(unsigned char *entry){
    int item1off = from_u32(entry + 0x10);
    return entry[item1off + 0x190];
}

/** \brief
 *  Gets zone's camera entity count.
 *
 * \param entry unsigned char*          entry data
 * \return int                          camera entity count (total count, not camera path count)
 */
int get_cam_count(unsigned char *entry){
    int item1off = from_u32(entry + 0x10);
    return entry[item1off + 0x188];
}

/** \brief
 *  Gets zone's scenery reference count.
 *
 * \param entry unsigned char*          entry data
 * \return int                          scenery reference count
 */
int get_scen_count(unsigned char *entry){
    int item1off = from_u32(entry + 0x10);
    return entry[item1off];
}

/** \brief
 *  Gets zone's regular entity count.
 *
 * \param entry unsigned char*          entry data
 * \return int                          entity count (not including camera entities)
 */
int get_entity_count(unsigned char *entry){
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
unsigned int* get_relatives(unsigned char *entry, SPAWNS *spawns)
{
    int entity_count, item1len, relcount, item1off, camcount, neighbourcount, scencount, i, entry_count = 0;
    unsigned int* relatives;

    item1off = from_u32(entry + 0x10);
    item1len = from_u32(entry + 0x14) - item1off;
    if (!(item1len == 0x358 || item1len == 0x318)) return NULL;

    camcount = get_cam_count(entry);
    if (camcount == 0) return NULL;

    entity_count = get_entity_count(entry);
    scencount = get_scen_count(entry);
    neighbourcount = get_neighbour_count(entry);
    relcount = camcount/3 + neighbourcount + scencount + 1;

    if (relcount == 1) return NULL;

    if (from_u32(&entry[item1off + C2_MUSIC_REF + item1len - 0x318]) != NONE)
        relcount++;
    relatives = (unsigned int *) malloc(relcount * sizeof(unsigned int));

    relatives[entry_count++] = relcount - 1;

    for (i = 0; i < (camcount/3); i++)
        relatives[entry_count++] = get_slst(entry + from_u32(entry + 0x18 + 0xC * i));

    for (i = 0; i < neighbourcount; i++)
        relatives[entry_count++] = from_u32(entry + item1off + 0x194 + 4*i);

    for (i = 0; i < scencount; i++)
        relatives[entry_count++] = from_u32(entry + item1off + 0x4 + 0x30 * i);

    if (from_u32(&entry[item1off + C2_MUSIC_REF + item1len - 0x318]) != NONE)
        relatives[entry_count++] = from_u32(&entry[item1off + C2_MUSIC_REF + item1len - 0x318]);

    for (i = 0; i < entity_count; i++)
    {
        int *coords_ptr;
        coords_ptr = seek_spawn(entry + from_u32(entry + 0x18 + 4 * camcount + 4 * i));
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
int entry_type(ENTRY entry){
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
unsigned int* GOOL_relatives(unsigned char *entry, int entrysize)
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
void read_nsf(ENTRY *elist, int chunk_border_base, unsigned char **chunks, int *chunk_border_texture, int *entry_count, FILE *nsf, unsigned int *gool_table)
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
void dumb_merge(ENTRY *elist, int chunk_index_start, int *chunk_index_end, int entry_count)
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

    *chunk_index_end = remove_empty_chunks(chunk_index_start, *chunk_index_end, entry_count, elist);
}


/** \brief
 *  Deprecate, was used in a bad merge method.
 *  Checks whether a searched eid is a relative.
 *
 * \param searched unsigned int         searched eid
 * \param array unsigned int*           relatives array
 * \param count int                     length of relatives array
 * \return int                          1 if its a relative, else 0
 */
int is_relative(unsigned int searched, unsigned int *array, int count)
{
    for (int i = 0; i < count; i++)
        if (searched == array[i]) return 1; //(count - i)*(count - i)/2;

    return 0;
}

/** \brief
 *  Deprecate merge method function based on common relative count.
 *  Do not use.
 *
 * \param elist ENTRY*                  entry list
 * \param chunk_index_start int         start of the range
 * \param chunk_index_end int*          end of the range (excl.)
 * \param entry_count int               entry count
 * \return void
 */
void gool_merge(ENTRY *elist, int chunk_index_start, int *chunk_index_end, int entry_count)
{
    int i, j, k;
    while(1)
    {
        int merge_happened = 0;
        for (i = chunk_index_start; i < *chunk_index_end; i++)
        {
            int size1 = 0, count1 = 0, max_rating = 0, max_entry_count = 0;;
            unsigned int relatives[250];
            int relative_counter = 0;

            for (j = 0; j < entry_count; j++)
                if (elist[j].chunk == i)
                {
                    size1 += elist[j].esize;
                    count1++;
                    if (elist[j].related != NULL)
                        for (k = 0; (unsigned) k < elist[j].related[0]; k++)
                        {
                            relatives[relative_counter] = elist[j].related[k];
                            relative_counter++;
                        }
                }

            for (j = i + 1; j < *chunk_index_end; j++)
            {
                int size2 = 0;
                int count2 = 0;
                int has_relative = 0;

                for (k = 0; k < entry_count; k++)
                    if (elist[k].chunk == j)
                    {
                        size2 += elist[k].esize;
                        count2++;
                        has_relative += is_relative(elist[k].EID, relatives, relative_counter);
                    }

                if ((size1 + size2 + 4 * count1 + 4 * count2 + 0x14) <= CHUNKSIZE)
                {
                    int rating = has_relative;
                    if (rating > max_rating)
                    {
                        max_rating = rating;
                        max_entry_count = j;
                    }
                }
            }

            if (max_entry_count)
            {
                for (j = 0; j < entry_count; j++)
                    if (elist[j].chunk == max_entry_count) elist[j].chunk = i;
                merge_happened++;
                //printf("merge happened\n");
            }
        }
        if (!merge_happened) break;
    }

    *chunk_index_end = remove_empty_chunks(chunk_index_start, *chunk_index_end, entry_count, elist);
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
void read_folder(DIR *df, char *dirpath, unsigned char **chunks, ENTRY *elist, int *chunk_border_texture, int *entry_count, SPAWNS *spawns, unsigned int* gool_table)
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
        if (fsize == CHUNKSIZE && from_u32(entry) == MAGIC_TEXTURE)
        {
            if (get_base_chunk_border(from_u32(entry + 4), chunks, *chunk_border_texture) > 0) continue;
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
        if (get_index(from_u32(entry + 4), elist, *entry_count) >= 0) continue;

        elist[*entry_count].EID = from_u32(entry + 4);
        elist[*entry_count].chunk = -1;
        elist[*entry_count].esize = fsize;

        if (entry[8] == 7)
            elist[*entry_count].related = get_relatives(entry, spawns);
        if (entry[8] == 11 && entry[0xC] == 6)
            elist[*entry_count].related = GOOL_relatives(entry, fsize);

        elist[*entry_count].data = (unsigned char *) malloc(fsize * sizeof(unsigned char));
        memcpy(elist[*entry_count].data, entry, fsize);
        if (entry_type(elist[*entry_count]) == ENTRY_TYPE_GOOL && *(elist[*entry_count].data + 8) > 3)
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
void print(ENTRY *elist, int entry_count)
// prints entries and references, UNUSED
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
                       elist[get_index(relative, elist, entry_count)].chunk, elist[get_index(relative, elist, entry_count)].esize);
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
void swap_spawns(SPAWNS spawns, int spawnA, int spawnB)
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
 *  Lets you pick a primary spawn.
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
void write_nsd(char *path, ENTRY *elist, int entry_count, int chunk_count, SPAWNS spawns, unsigned int* gool_table, int level_ID)
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
    if (input - 1 > spawns.spawn_count || input <= 0)
    {
        printf("No such spawn, defaulting to first one\n");
        input = 1;
    }

    if (input - 1)
        swap_spawns(spawns, 0, input - 1);

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
        if (entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].data != NULL)
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
                                list[m].index = get_index(list[m].eid, elist, entry_count);
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
 *  Spawns object init function.
 *
 * \return SPAWNS
 */
SPAWNS init_spawns(){
    SPAWNS temp;
    temp.spawn_count = 0;
    temp.spawns = NULL;

    return temp;
}

/** \brief
 *  Calculates the amount of normal chunks loaded by the zone, their list.
 *  Used by a deprecate merge method.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param list LIST                     current load list
 * \param zone unsigned int             current zone
 * \param chunk_min int                 used to weed out sound and instrument entries (nsf structure this program produces is texture - wavebank - sound - normal)
 * \return PAYLOAD                      payload object that contains a list of chunks that are loaded in this zone, their count and the current zone eid
 */
PAYLOAD get_payload(ENTRY *elist, int entry_count, LIST list, unsigned int zone, int chunk_min)
{
    int chunks[1024];
    int count = 0;
    int curr_chunk;
    int is_there;
    char help[100];

    for (int i = 0; i < list.count; i++)
    {
        int elist_index = get_index(list.eids[i], elist, entry_count);
        curr_chunk = elist[elist_index].chunk;
        is_there = 0;
        for (int j = 0; j < count; j++)
            if (chunks[j] == curr_chunk) is_there = 1;

        if (!is_there && eid_conv(elist[elist_index].EID, help)[4] != 'T' && curr_chunk != -1 && curr_chunk >= chunk_min)
        {
            chunks[count] = curr_chunk;
            count++;
        }
    }

    PAYLOAD temp;
    temp.zone = zone;
    temp.count = count;
    temp.chunks = (int *) malloc(count * sizeof(int));
    memcpy(temp.chunks, chunks, sizeof(int) * count);
    return temp;
}

/** \brief
 *  List struct init function.
 *
 * \return LIST
 */
LIST init_list(){
    LIST list;
    list.count = 0;
    list.eids = NULL;

    return list;
}

/** \brief
 *  Binary search in a sorted list.
 *
 * \param list LIST                     list to be searched
 * \param searched unsigned int         searched item
 * \return int                          index the item has or -1 if item wasnt found
 */
int list_find(LIST list, unsigned int searched)
{
    int first = 0;
    int last = list.count - 1;
    int middle = (first + last)/2;

    while (first <= last)
    {
        if (list.eids[middle] < searched)
            first = middle + 1;
        else if (list.eids[middle] == searched)
            return middle;
        else
            last = middle - 1;

        middle = (first + last)/2;
    }

    return -1;
}

/** \brief
 *  Adds an item to the list.
 *
 * \param list LIST*                    list to be added into
 * \param eid unsigned int              item to be added
 * \return void
 */
void list_add(LIST *list, unsigned int eid)
{
    list->eids = (unsigned int *) realloc(list->eids, (list->count + 1) * sizeof(unsigned int *));
    list->eids[list->count] = eid;
    list->count++;
    qsort(list->eids, list->count, sizeof(unsigned int), list_comp);
}

/** \brief
 *  Removes given item from the list if it exists.
 *
 * \param list LIST*                    list to be removed from
 * \param eid unsigned int              item to be removed
 * \return void
 */
void list_rem(LIST *list, unsigned int eid)
{
    int index = list_find(*list, eid);
    if (index == -1) return;

    list->eids[index] = list->eids[list->count - 1];
    list->eids = (unsigned int *) realloc(list->eids, (list->count - 1) * sizeof(unsigned int *));
    list->count--;
    qsort(list->eids, list->count, sizeof(unsigned int), list_comp);
}

/** \brief
 *  Adds an item to the list if it's not there yet.
 *
 * \param list LIST*                    list to be inserted into
 * \param eid unsigned int              item to be inserted
 * \return void
 */
void list_insert(LIST *list, unsigned int eid){
    if (list_find(*list, eid) == -1)
        list_add(list, eid);
}

/** \brief
 *  Prints payload.
 *
 * \param payload PAYLOAD               payload struct
 * \param stopper int                   something dumb
 * \return void
 */
void print_payload(PAYLOAD payload, int stopper)
{
    char temp[100];
    printf("Zone: %s; payload: %3d", eid_conv(payload.zone, temp), payload.count);
    if (stopper) printf("; stopper: %2d", stopper);
    printf("\n");
}

/** \brief
 *  Inits load list struct.
 *
 * \return LOAD_LIST
 */
LOAD_LIST init_load_list(){
    LOAD_LIST temp;
    temp.count = 0;

    return temp;
}


/** \brief
 *  Adds a payload to a payloads struct, overwrites if its already there.
 *
 * \param payloads PAYLOADS*            payloads struct
 * \param insertee PAYLOAD              payload struct
 * \return void
 */
void insert_payload(PAYLOADS *payloads, PAYLOAD insertee)
{
    for (int i = 0; i < payloads->count; i++)
        if (payloads->arr[i].zone == insertee.zone)
        {
            if (payloads->arr[i].count < insertee.count)
            {
                    payloads->arr[i].count = insertee.count;
                    free(payloads->arr[i].chunks);
                    payloads->arr[i].chunks = insertee.chunks;
                    return;
            }
            else return;
        }

    payloads->arr = (PAYLOAD *) realloc(payloads->arr, (payloads->count + 1) * sizeof(PAYLOAD));
    payloads->arr[payloads->count] = insertee;
    (payloads->count)++;
}

/** \brief
 *  Used by payload method, deprecate.
 *  Creates a payloads object that contains each zone and chunks that zone loads.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_min int                 used to get rid of invalid chunks/entries
 * \return PAYLOADS                     payloads struct
 */
PAYLOADS get_payload_ladder(ENTRY *elist, int entry_count, int chunk_min)
{
    PAYLOADS payloads;
    payloads.count = 0;
    payloads.arr = NULL;
    int i, j, k, l, m;
    for (i = 0; i < entry_count; i++)
        if (entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].data != NULL)
        {
            int item1off = from_u32(elist[i].data + 0x10);
            int cam_count = from_u32(elist[i].data + item1off + 0x188) / 3;

            for (j = 0; j < cam_count; j++)
            {
                int cam_offset = from_u32(elist[i].data + 0x18 + 0xC * j);
                LOAD_LIST load_list = init_load_list();
                LIST list = init_list();
                PAYLOAD payload;
                for (k = 0; (unsigned) k < from_u32(elist[i].data + cam_offset + 0xC); k++)
                {
                    int code = from_u16(elist[i].data + cam_offset + 0x10 + 8 * k);
                    int offset = from_u16(elist[i].data + cam_offset + 0x12 + 8 * k) + OFFSET + cam_offset;
                    int list_count = from_u16(elist[i].data + cam_offset + 0x16 + 8 * k);
                    if (code == ENTITY_PROP_CAM_LOAD_LIST_A || code == ENTITY_PROP_CAM_LOAD_LIST_B)
                    {
                        int sub_list_offset = offset + 4 * list_count;
                        int point;
                        int load_list_item_count;
                        for (l = 0; l < list_count; l++, sub_list_offset += load_list_item_count * 4)
                        {
                            load_list_item_count = from_u16(elist[i].data + offset + l * 2);
                            point = from_u16(elist[i].data + offset + l * 2 + list_count * 2);

                            load_list.array[load_list.count].list_length = load_list_item_count;
                            load_list.array[load_list.count].list = (unsigned int *) malloc(load_list_item_count * sizeof(unsigned int));
                            memcpy(load_list.array[load_list.count].list, elist[i].data + sub_list_offset, load_list_item_count * sizeof(unsigned int*));
                            if (code == ENTITY_PROP_CAM_LOAD_LIST_A)
                                load_list.array[load_list.count].type = 'A';
                            else
                                load_list.array[load_list.count].type = 'B';
                            load_list.array[load_list.count].index = point;
                            load_list.count++;
                        }
                    }
                    qsort(load_list.array, load_list.count, sizeof(LOAD), comp);
                }

                for (l = 0; l < load_list.count; l++)
                    {
                        if (load_list.array[l].type == 'A')
                            for (m = 0; m < load_list.array[l].list_length; m++)
                                list_add(&list, load_list.array[l].list[m]);

                        if (load_list.array[l].type == 'B')
                            for (m = 0; m < load_list.array[l].list_length; m++)
                                list_rem(&list, load_list.array[l].list[m]);

                        payload = get_payload(elist, entry_count, list, elist[i].EID, chunk_min);
                        insert_payload(&payloads, payload);
                    }
            }
        }

    return payloads;
}


/** \brief
 *  Merges the chunks, usually 2.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunks int*                   array of chunks to be considered by the merge thing
 * \param chunk_count int               amount of those chunks
 * \return void
 */
void chunk_merge(ENTRY *elist, int entry_count, int *chunks, int chunk_count)
{
    for (int i = 0; i < entry_count; i++)
        for (int j = 0; j < chunk_count; j++)
            if (elist[i].chunk == chunks[j])
                elist[i].chunk = chunks[0];

}

/** \brief
 *  Used by some payload merge method (deprecate).
 *
 * \param listA int*                    list of relatives A
 * \param countA int                    count of relatives A
 * \param listB int*                    list of relatives B
 * \param countB int                    count of relatives B
 * \return int                          amount of common items
 */
int get_common(int* listA, int countA, int *listB, int countB)
{
    int i, j, copy_countA = 0, copy_countB = 0;
    int copyA[100];
    int copyB[100];
    qsort(listA, countA, sizeof(int), cmpfunc);
    qsort(listB, countB, sizeof(int), cmpfunc);

    for (i = 0; i < countA; i++)
        if (i)
        {
            if (listA[i] != copyA[copy_countA - 1])
                copyA[copy_countA++] = listA[i];
        }
        else copyA[copy_countA++] = listA[i];

    for (i = 0; i < countB; i++)
        if (i)
        {
            if (listB[i] != copyB[copy_countB - 1])
                copyB[copy_countB++] = listB[i];
        }
        else copyB[copy_countB++] = listA[i];

    int counter = 0;
    for (i = 0; i < copy_countA; i++)
        for (j = 0; j < copy_countB; j++)
        if (copyA[i] == copyB[j])
            counter++;

    return counter;
}

/** \brief
 *  Merges chunks from the chunks list based on common relative count I think.
 *  Deprecate, used by payload ladder method.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunks int*                   chunks array
 * \param chunk_count int               chunk count
 * \return int                          1 if a merge occured, else 0
 */
int merge_thing(ENTRY *elist, int entry_count, int *chunks, int chunk_count)
{
    int i, j, k;
    int best[2] = {-1, -1};
    int relatives1[1024], relatives2[1024];
    int relative_count1, relative_count2;

    int max = 0;

    for (i = 0; i < chunk_count; i++)
    {
        int size1 = 0;
        int count1 = 0;
        relative_count1 = 0;

        for (j = 0; j < entry_count; j++)
            if (elist[j].chunk == chunks[i])
            {
                size1 += elist[j].esize;
                count1++;
                relatives1[relative_count1++] = elist[j].EID;
                if (elist[j].related != NULL)
                    for (k = 0; (unsigned ) k < elist[j].related[0]; k++)
                    relatives1[relative_count1++] = elist[j].related[k + 1];
            }


        for (j = i + 1; j < chunk_count; j++)
        {
            int size2 = 0;
            int count2 = 0;
            relative_count2 = 0;
            for (k = 0; k < entry_count; k++)
                if (elist[k].chunk == chunks[j])
                {
                    size2 += elist[k].esize;
                    count2++;
                    relatives2[relative_count2++] = elist[j].EID;
                }

            if ((size1 + size2 + 4 * count1 + 4 * count2 + 0x14) <= CHUNKSIZE)
            {
                int common_count = get_common(relatives1, relative_count1, relatives2, relative_count2);
                if (common_count > max)
                {
                    max = common_count;
                    best[0] = chunks[i];
                    best[1] = chunks[j];
                }
            }
        }
    }

    if (best[0] != -1 && best[0] != -1)
    {
        chunk_merge(elist, entry_count, best, 2);
        return 1;
    }

    return 0;
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
void increment_common(LIST list, LIST entries, int **entry_matrix, int rating)
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
void waren_merge(RELATIONS relations, ENTRY *elist, int entry_count, LIST entries)
{
    for (int x = 0; x < relations.count; x++)
    {
        int index1 = relations.relations[x].index1;
        int index2 = relations.relations[x].index2;

        int elist_index1 = get_index(entries.eids[index1], elist, entry_count);
        int elist_index2 = get_index(entries.eids[index2], elist, entry_count);

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
RELATIONS transform_matrix(LIST entries, int **entry_matrix)
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
void waren_load_list_merge(ENTRY *elist, int entry_count, int chunk_border_sounds, int* chunk_count, int merge_flag)
{
    int i, j, k, l, m;
    LIST entries = init_list();

    for (i = 0; i < entry_count; i++)
    {
        switch (entry_type(elist[i]))
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
        if (entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].data != NULL)
        {
            int item1off = from_u32(elist[i].data + 0x10);
            int cam_count = from_u32(elist[i].data + item1off + 0x188) / 3;

            for (j = 0; j < cam_count; j++)
            {
                int cam_offset = from_u32(elist[i].data + 0x18 + 0xC * j);
                LOAD_LIST load_list = init_load_list();
                LIST list = init_list();
                int cam_length = 0;
                for (k = 0; (unsigned) k < from_u32(elist[i].data + cam_offset + 0xC); k++)
                {
                    int code = from_u16(elist[i].data + cam_offset + 0x10 + 8 * k);
                    int offset = from_u16(elist[i].data + cam_offset + 0x12 + 8 * k) + OFFSET + cam_offset;
                    int list_count = from_u16(elist[i].data + cam_offset + 0x16 + 8 * k);
                    if (code == ENTITY_PROP_CAM_LOAD_LIST_A || code == ENTITY_PROP_CAM_LOAD_LIST_B)
                    {
                        int sub_list_offset = offset + 4 * list_count;
                        int point;
                        int load_list_item_count;
                        for (l = 0; l < list_count; l++, sub_list_offset += load_list_item_count * 4)
                        {
                            load_list_item_count = from_u16(elist[i].data + offset + l * 2);
                            point = from_u16(elist[i].data + offset + l * 2 + list_count * 2);

                            load_list.array[load_list.count].list_length = load_list_item_count;
                            load_list.array[load_list.count].list = (unsigned int *) malloc(load_list_item_count * sizeof(unsigned int));
                            memcpy(load_list.array[load_list.count].list, elist[i].data + sub_list_offset, load_list_item_count * sizeof(unsigned int*));
                            if (code == ENTITY_PROP_CAM_LOAD_LIST_A)
                                load_list.array[load_list.count].type = 'A';
                            else
                                load_list.array[load_list.count].type = 'B';
                            load_list.array[load_list.count].index = point;
                            load_list.count++;
                            qsort(load_list.array, load_list.count, sizeof(LOAD), comp);
                        }
                    }

                    if (code == ENTITY_PROP_PATH)
                        cam_length = from_u32(elist[i].data + offset);
                }

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
                                increment_common(list, entries, entry_matrix, 1);
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
                                increment_common(list, entries, entry_matrix, 1);
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }

    RELATIONS array_representation = transform_matrix(entries, entry_matrix);
    /*FILE *out = fopen("C:\\Users\\samo1\\Desktop\\Matrix.txt", "w");
    char temp[100];
    fprintf(out, "     |");
    for (i = 0; i < entries.count; i++)
        fprintf(out, "%s|", eid_conv(entries.eids[i], temp));
    fprintf(out, "\n");
    for (i = 0; i < entries.count; i++)
    {
        fprintf(out, "%s|", eid_conv(entries.eids[i], temp));
        for (j = 0; j < i; j++)
            fprintf(out, "%5d ", entry_matrix[i][j]);
        fprintf(out, "\n");
    }*/

    for (i = 0; i < entries.count; i++)
        free(entry_matrix[i]);
    free(entry_matrix);

    printf("Done constructing the relation matrix\n");
    waren_merge(array_representation, elist, entry_count, entries);
    printf("Done merging\n");
}


/** \brief
 *  Deprecate merge function based on payloads.
 *  In each iteration gets a payload ladder and tries to merge chunks loaded by
 *  the zone with highest payload, one at a time.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_min int                 start of normal chunk range
 * \param chunk_count int*              end of normal chunk range
 * \return void
 */
void payload_merge(ENTRY *elist, int entry_count, int chunk_min, int *chunk_count)
{
    for (int x = 0; ; x++)
    {
        PAYLOADS payloads = get_payload_ladder(elist, entry_count, chunk_min);
        qsort(payloads.arr, payloads.count, sizeof(PAYLOAD), pay_cmp);

        printf("%3d  ", x);
        print_payload(payloads.arr[0], 0);

        if (payloads.arr[0].count < 19) break;

        qsort(payloads.arr[0].chunks, payloads.arr[0].count, sizeof(int), cmpfunc);

        for (int i = 0; i < payloads.arr[0].count; i++)
            printf("%3d ", payloads.arr[0].chunks[i] * 2 + 1);
        printf("\n");

        int chunks[1024];
        int count;
        int check = 0;

        for (int i = 0; i < payloads.count && check != 1; i++)
        {
            count = 0;

            for (int j = 0; j < payloads.arr[i].count; j++)
            {
                int curr_chunk = payloads.arr[i].chunks[j];
                if (curr_chunk >= chunk_min)
                    chunks[count++] = curr_chunk;
            }
            qsort(chunks, count, sizeof(int), cmpfunc);
            if (merge_thing(elist, entry_count, chunks, count))
                check = 1;

            if (check)
            for (int j = chunk_min; j < *chunk_count; j++)
            {
                int empty = 1;
                for (int k = 0; k < entry_count; k++)
                    if (elist[k].chunk == j)
                        empty = 0;

                if (empty)
                {
                    for (int k = 0; k < entry_count; k++)
                        if (elist[k].chunk == (*chunk_count - 1))
                            elist[k].chunk = j;
                    (*chunk_count)--;
                }
            }
        }

        if (check == 0) break;
    }

    dumb_merge(elist, chunk_min, chunk_count, entry_count);
}

/** \brief
 *  Assigns gool entries and their relatives initial chunks.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param real_chunk_count int*         chunk count
 * \param grouping_flag int             1 if one by one, 0 if pre-grouping
 * \return void
 */
void create_proto_chunks_gool(ENTRY *elist, int entry_count, int *real_chunk_count, int grouping_flag)
{
    int i, j;
    int size;
    int counter;
    int relative_index;
    int chunk_count = *real_chunk_count;

    switch(grouping_flag)
    {
        case 0:  // group gool with its kids
        {
            for (i = 0; i < entry_count; i++)
            {
                if (entry_type(elist[i]) == ENTRY_TYPE_GOOL)
                {
                    elist[i].chunk = chunk_count;
                    size = elist[i].esize;
                    counter = 0;
                    if (elist[i].related != NULL)
                    for (j = 0; (unsigned) j < elist[i].related[0]; j++)
                    {
                        relative_index = get_index(elist[i].related[j + 1], elist, entry_count);
                        if (elist[relative_index].chunk != -1) continue;
                        if ((elist[relative_index].esize + size + 0x10 + 4 * (counter + 2)) > CHUNKSIZE)
                        {
                            chunk_count++;
                            counter = 0;
                            size = 0;
                        }
                        elist[relative_index].chunk = chunk_count;
                        size += elist[relative_index].esize;
                        counter++;
                    }
                    chunk_count++;
                }
            }
            break;
        }
        case 1: // one by one
        {
            for (i = 0; i < entry_count; i++)
            {

                if (entry_type(elist[i]) == ENTRY_TYPE_GOOL)
                {
                    elist[i].chunk = chunk_count++;

                    if (elist[i].related != NULL)
                    for (j = 0; (unsigned) j < elist[i].related[0]; j++)
                    {
                        relative_index = get_index(elist[i].related[j + 1], elist, entry_count);
                        if (elist[relative_index].chunk != -1) continue;

                        elist[relative_index].chunk = chunk_count++;
                    }
                }
            }
            break;
        }
        default: break;
    }

    *real_chunk_count = chunk_count;
}


/** \brief
 *  Builds chunks according to entries' assigned chunks, shouldnt require any
 *  further patches.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_border_texture int      index where texture chunks end
 * \param chunk_border_sounds int       index where sounds end
 * \param chunk_count int               chunk count
 * \param chunks unsigned char**        array of chunks
 * \return void
 */
void build_real_chunks(ENTRY *elist, int entry_count, int chunk_border_texture, int chunk_border_sounds, int chunk_count, unsigned char **chunks)
{
    int i, j, sum = 0;
    for (i = chunk_border_texture; i < chunk_count; i++)
    {
        int chunk_no = 2 * i + 1;
        int local_entry_count = 0;
        chunks[i] = (unsigned char*) calloc(CHUNKSIZE, sizeof(unsigned char));

        for (j = 0; j < entry_count; j++)
            if (elist[j].chunk == i) local_entry_count++;

        unsigned int offsets[local_entry_count + 2];

        *(unsigned short int *) chunks[i] = 0x1234;
        *((unsigned short int*) (chunks[i] + 4)) = chunk_no;
        *((unsigned short int*) (chunks[i] + 8)) = local_entry_count;
        if (i < chunk_border_sounds) *(short int*)(chunks[i] + 2) = 3;

        local_entry_count++;
        int indexer = 0;
        offsets[indexer] = 0x10 + local_entry_count * 4;

        // calculates offsets
        for (j = 0; j < entry_count; j++)
            if (elist[j].chunk == i)
            {
                offsets[indexer + 1] = offsets[indexer] + elist[j].esize;
                indexer++;
            }

        // writes offsets
        for (j = 0; j < local_entry_count; j++)
            *((unsigned int *) (chunks[i] + 0x10 + j * 4)) = offsets[j];

        // writes entries
        int curr_offset = offsets[0];
        for (j = 0; j < entry_count; j++)
        {
            if (elist[j].chunk == i)
            {
                memcpy(chunks[i] + curr_offset, elist[j].data, elist[j].esize);
                curr_offset += elist[j].esize;
            }

            // if its an instrument entry/chunk
            if (entry_type(elist[j]) == ENTRY_TYPE_INST && elist[j].chunk == i)
                *(short int*)(chunks[i] + 2) = 4;
        }

        if (i >= chunk_border_sounds)
                sum += curr_offset;

        // writes checksum
        *((unsigned int *)(chunks[i] + 0xC)) = nsfChecksum(chunks[i]);
    }

    printf("AVG: %.3f\n", (100 * (double) sum / (chunk_count - chunk_border_sounds)) / CHUNKSIZE);
}

/** \brief
 *  Initial chunk assignment for zones and their relatives, either one-by-one or pre-grouped.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param real_chunk_count int*         chunk count
 * \param grouping_flag int             one-by-one if 1, pre-groups if 0
 * \return void
 */
void create_proto_chunks_zones(ENTRY *elist, int entry_count, int *real_chunk_count, int grouping_flag)
{
    int i, j;
    int chunk_count = *real_chunk_count;
    int size;
    int counter;

    switch(grouping_flag)
    {
        case 0: // group all relatives together
        {
            for (i = 0; i < entry_count; i++)
            if (entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].related != NULL)
            {
                if (elist[i].chunk != -1) continue;
                elist[i].chunk = chunk_count;
                size = elist[i].esize;
                counter = 0;
                if (elist[i].related != NULL)
                    for (j = 0; (unsigned) j < elist[i].related[0]; j++)
                    {
                        int relative_index = get_index(elist[i].related[j + 1], elist, entry_count);
                        if (elist[relative_index].chunk != -1 || elist[relative_index].related != NULL) continue;
                        if ((elist[relative_index].esize + size + 0x10 + 4 * (counter + 2)) > CHUNKSIZE)
                        {
                            chunk_count++;
                            counter = 0;
                            size = 0;
                        }
                        elist[relative_index].chunk = chunk_count;
                        size += elist[relative_index].esize;
                        counter++;
                    }
                chunk_count++;
            }
            break;
        }
        case 1: // one by one
        {
            for (i = 0; i < entry_count; i++)
            if (entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].related != NULL)
            {
                if (elist[i].chunk != -1) continue;
                elist[i].chunk = chunk_count++;
                if (elist[i].related != NULL)
                    for (j = 0; (unsigned) j < elist[i].related[0]; j++)
                    {
                        int relative_index = get_index(elist[i].related[j + 1], elist, entry_count);
                        if (elist[relative_index].chunk != -1 || elist[relative_index].related != NULL) continue;
                        elist[relative_index].chunk = chunk_count++;
                    }
            }
            break;
        }
        default:
            break;
    }

    *real_chunk_count = chunk_count;
}


/** \brief
 *  Gets entity's type.
 *
 * \param entity unsigned char*         entity data
 * \return int                          entity's type or -1
 */
int get_entity_type(unsigned char *entity)
{
    unsigned int i;
    for (i = 0; i < from_u32(entity + 0xC); i++)
    {
        int code = from_u16(entity + 0x10 + 8 * i);
        int offset = from_u16(entity + 0x12 + 8 * i) + OFFSET;

        if (code == ENTITY_PROP_TYPE)
            return from_u32(entity + offset + 4);
    }

    return -1;
}

/** \brief
 *  Gets entity's subtype.
 *
 * \param entity unsigned char*     entity data
 * \return int                      entity's subtype or -1
 */
int get_entity_subtype(unsigned char *entity)
{
    unsigned int i;
    for (i = 0; i < from_u32(entity + 0xC); i++)
    {
        int code = from_u16(entity + 0x10 + 8 * i);
        int offset = from_u16(entity + 0x12 + 8 * i) + OFFSET;

        if (code == ENTITY_PROP_SUBTYPE)
            return from_u32(entity + offset + 4);
    }

    return -1;
}


/** \brief
 *  Gets texture references from a scenery entry and inserts them to the list.
 *
 * \param scenery unsigned char*            scenery data
 * \param list LIST*                        current load list
 * \return void
 */
void add_texture_refs(unsigned char *scenery, LIST *list)
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
 *  Inserts all stuff loaded by a zone (scenery dependencies, entity dependencies, its own relatives).
 *
 * \param eid unsigned int              entry whose children are to be added
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param list LIST*                    current load list
 * \param gool_table unsigned int*      gool table
 * \param dependencies DEPENDENCIES     stuff loaded when a certain type/subtype is used
 * \return void
 */
void ll_add_children(unsigned int eid, ENTRY *elist, int entry_count, LIST *list, unsigned int *gool_table, DEPENDENCIES dependencies)
{
    int i, j, k, l;
    int index = get_index(eid, elist, entry_count);
    if (index == -1) return;

    if (elist[index].related != NULL)
        for (i = 0; (unsigned) i < elist[index].related[0]; i++)
            list_insert(list, elist[index].related[i + 1]);

    list_insert(list, elist[index].EID);

    if (entry_type(elist[index]) == ENTRY_TYPE_ZONE)
    {
        int item1off = from_u32(elist[index].data + 0x10);
        int neighbourcount = get_neighbour_count(elist[index].data);

        for (i = 0; i < neighbourcount; i++)
        {
            int neighbour_index = get_index(from_u32(elist[index].data + item1off + 0x194 + 4*i), elist, entry_count);
            if (neighbour_index == -1) continue;

            int entity_count = get_entity_count(elist[neighbour_index].data);
            int cam_count = get_cam_count(elist[neighbour_index].data);

            for (j = 0; j < entity_count; j++)
            {
                int entity_type = get_entity_type(elist[neighbour_index].data + from_u32(elist[neighbour_index].data + 0x18 + 4 * cam_count + 4 * j));
                int entity_subt = get_entity_subtype(elist[neighbour_index].data + from_u32(elist[neighbour_index].data + 0x18 + 4 * cam_count + 4 * j));
                for (k = 0; k < dependencies.count; k++)
                    if (dependencies.array[k].type == entity_type && dependencies.array[k].subtype == entity_subt)
                        for (l = 0; l < dependencies.array[k].dependencies.count; l++)
                        {
                            list_insert(list, dependencies.array[k].dependencies.eids[l]);
                            int index = get_index(dependencies.array[k].dependencies.eids[l], elist, entry_count);
                            if (index == -1) continue;
                            if (entry_type(elist[index]) == ENTRY_TYPE_ANIM)
                                list_insert(list, get_model(elist[index].data));
                        }
            }
        }

        int scenery_count = get_scen_count(elist[index].data);
        for (i = 0; i < scenery_count; i++)
        {
            int scenery_index = get_index(from_u32(elist[index].data + item1off + 0x4 + 0x30 * i), elist, entry_count);
            add_texture_refs(elist[scenery_index].data, list);
        }
    }
}


/** \brief
 *  Creates a load list and inserts it basically.
 *
 * \param code unsigned int             code of the property to be added
 * \param item unsigned char*           data of item where the property will be added
 * \param item_size int*                item size
 * \param list LIST*                    load list to be added
 * \return unsigned char*               new item data
 */
unsigned char *add_property(unsigned int code, unsigned char *item, int* item_size, LIST *list)
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

    property_sizes[insertion_index] = 4 + list->count * 4;
    *(short int*) property_headers[insertion_index] = code;
    *(int *)(property_headers[insertion_index] + 4) = 0x00010424;

    int pathlen = 0;
    for (i = 0; i < property_count + 1; i++)
        if (from_u16(property_headers[i]) == ENTITY_PROP_PATH)
            pathlen = from_u32(properties[i]);

    properties[insertion_index] = (unsigned char *) malloc(4 + list->count * 4);
    *(short int *)(properties[insertion_index]) = list->count;
    if (code == ENTITY_PROP_CAM_LOAD_LIST_B)
        *(short int *)(properties[insertion_index] + 2) = pathlen - 1;
    else
        *(short int *)(properties[insertion_index] + 2) = 0;

    for (i = 0; i < list->count; i++)
        *(int *)(properties[insertion_index] + 4 + 4 * i) = list->eids[i];

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
 * \param list LIST*                    unused
 * \return unsigned char*               new item data
 */
unsigned char* remove_property(unsigned int code, unsigned char *item, int* item_size, LIST *list)
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
void camera_alter(ENTRY *zone, int item_index, unsigned char *(func_arg)(unsigned int, unsigned char *, int *, LIST *), LIST *list, int property_code)
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

    items[item_index] = func_arg(property_code, items[item_index], &item_lengths[item_index], list);

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
 * \param link_count int*               amount of links
 * \param cam_index int                 index of the camera item
 * \return int*                         array of indexes of neighbours
 */
int *get_linked_neighbours(unsigned char *entry, int *link_count, int cam_index)
{
    int k, l;
    int *neighbour_indices;
    int cam_offset = from_u32(entry + 0x18 + 0xC * cam_index);

    for (k = 0; (unsigned) k < from_u32(entry + cam_offset + 0xC); k++)
    {
        int code = from_u16(entry + cam_offset + 0x10 + 8 * k);
        int offset = from_u16(entry + cam_offset + 0x12 + 8 * k) + OFFSET + cam_offset;
        int prop_len = from_u16(entry + cam_offset + 0x16 + 8 * k);

        if (code == ENTITY_PROP_CAM_PATH_LINKS)
        {
            if (prop_len == 1)
            {
                *link_count = from_u16(entry + offset);
                neighbour_indices = (int *) malloc(*link_count * sizeof(int));
                for (l = 0; l < *link_count; l++)
                    neighbour_indices[l] = *(entry + offset + 0x6);
            }
            else
            {
                *link_count = max(1, from_u16(entry + offset)) + max(1, from_u16(entry + offset + 2));
                neighbour_indices = (int *) malloc(*link_count * sizeof(int));
                for (l = 0; l < *link_count; l++)
                    neighbour_indices[l] = *(entry + offset + 0xA + 4 * l);
            }
        }
    }

    return neighbour_indices;
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
void make_load_lists(ENTRY *elist, int entry_count, unsigned int *gool_table, LIST permaloaded, DEPENDENCIES subtype_info)
{
    int i, j, k;

    for (i = 0; i < entry_count; i++)
        if (entry_type(elist[i]) == ENTRY_TYPE_ZONE)
        {
            int item1off = from_u32(elist[i].data + 0x10);
            int cam_count = get_cam_count(elist[i].data) / 3;

            for (j = 0; j < cam_count; j++)
            {
                LIST list = init_list();

                list_insert(&list, eid_to_int("Cr10T"));
                list_insert(&list, eid_to_int("Cr20T"));

                for (k = 0; k < permaloaded.count; k++)
                    list_insert(&list, permaloaded.eids[k]);

                ll_add_children(elist[i].EID, elist, entry_count, &list, gool_table, subtype_info);

                for (k = 0; k < entry_count; k++)
                    if (entry_type(elist[k]) == ENTRY_TYPE_SOUND)
                        list_insert(&list, elist[k].EID);

                int link_count = 0;
                int *neighbour_indices = get_linked_neighbours(elist[i].data, &link_count, j);

                for (k = 0; k < link_count; k++)
                {
                    int eid = from_u32(elist[i].data + item1off + 0x194 + 4 * neighbour_indices[k]);
                    if (neighbour_indices[k] != 0)
                        ll_add_children(eid, elist, entry_count, &list, gool_table, subtype_info);
                }

                camera_alter(&elist[i], 2 + 3 * j, remove_property, &list, 0x208);
                camera_alter(&elist[i], 2 + 3 * j, remove_property, &list, 0x209);
                camera_alter(&elist[i], 2 + 3 * j, add_property, &list, 0x208);
                camera_alter(&elist[i], 2 + 3 * j, add_property, &list, 0x209);
            }
        }
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
int get_info(LIST *permaloaded, DEPENDENCIES *subtype_info, char *file_path, ENTRY *elist, int entry_count)
{
    int i, j, perma_count, subcount, valid = 1;
    char temp[6];
    LIST perma = init_list();

    FILE *file = fopen(file_path, "r");
    if (file == NULL)
        return 0;


    fscanf(file, "%d", &perma_count);
    for (i = 0; i < perma_count; i++)
    {
        fscanf(file, "%5s\n", temp);
        list_insert(&perma, eid_to_int(temp));
        int index = get_index(eid_to_int(temp), elist, entry_count);
        if (index == -1)
        {
            printf("INVALID ENTRY REFERENCE:\t%s\n", temp);
            valid = 0;
            continue;
        }

        if (entry_type(elist[index]) == ENTRY_TYPE_ANIM)
            list_insert(&perma, get_model(elist[index].data));
    }

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
        for (j = 0; j < counter; j++)
        {
            fscanf(file, ", %5s", temp);
            list_insert(&(subinfo.array[i].dependencies), eid_to_int(temp));
            int index = get_index(eid_to_int(temp), elist, entry_count);
            if (index == -1)
            {
                printf("INVALID ENTRY REFERENCE:\t%s", temp);
                valid = 0;
            }
        }
    }

    *permaloaded = perma;
    *subtype_info = subinfo;
    if (!valid)
    {
        printf("Cannot proceed with invalid items, fix that\n");
        return 0;
    }
    return 1;
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
// main function
{
    DIR *df = NULL;
    FILE *nsf = NULL, *nsfnew = NULL;
    SPAWNS spawns = init_spawns();

    char temp[MAX], lcltemp[MAX];
    unsigned int gool_table[0x40];
    for (int i = 0; i < 0x40; i++)
        gool_table[i] = NONE;

    ENTRY elist[2500];
    unsigned char *chunks[1024];

    // config:
    // [gool initial merge flag]    0 - group       |   1 - one by one
    // [zone initial merge flag]    0 - group       |   1 - one by one
    // [merge type flag]            0 - per delta   |   1 - per point
    int config[] = {1, 1, 1};

    int i, level_ID;
    int chunk_border_base       = 0,
        chunk_border_texture    = 0,
        // chunk_border_gool       = 0,
        chunk_border_instruments= 0,
        chunk_count             = 0,
        entry_count_base        = 0,
        entry_count             = 0,
        chunk_border_sounds;


    // opening and stuff
    if ((nsf = fopen(nsfpath,"rb")) == NULL)
    {
        printf("[ERROR] Could not open selected NSF\n");
        return;
    }

    if ((df = opendir(dirpath)) == NULL)
    {
        printf("[ERROR] Could not open selected directory\n");
        fclose(nsf);
        return;
    }

    printf("\nWhat ID do you want your level to have? [CAN OVERWRITE EXISTING FILES!]\n");
    scanf("%x", &level_ID);
    if (level_ID < 0 || level_ID > 0x3F)
    {
        printf("Invalid ID, defaulting to 0\n");
        level_ID = 0;
    }

    fseek(nsf, 0, SEEK_END);
    chunk_border_base = ftell(nsf) / CHUNKSIZE;
    rewind(nsf);

    printf("\nReading from the NSF.\n");
    //read_nsf(elist, chunk_border_base, chunks, &chunk_border_texture, &entry_count, nsf, gool_table);
    entry_count_base = entry_count;

    printf("Reading from the folder.\n");
    read_folder(df, dirpath, chunks, elist, &chunk_border_texture, &entry_count, &spawns, gool_table);

    printf("Getting model references.\n");
    get_model_references(elist, entry_count);

    printf("Removing invalid references.\n");
    remove_invalid_references(elist, entry_count, entry_count_base);
    chunk_count = chunk_border_texture;
    qsort(elist, entry_count, sizeof(ENTRY), cmp_entry_eid);

    // include instrument entries
    for (i = 0; i < entry_count; i++)
        if (entry_type(elist[i]) == ENTRY_TYPE_INST)
            elist[i].chunk = chunk_count++;
    chunk_border_instruments = chunk_count;

    // include sounds, merge sound chunks
    for (i = 0; i < entry_count; i++)
        if (entry_type(elist[i]) == ENTRY_TYPE_SOUND)
            elist[i].chunk = chunk_count++;

    dumb_merge(elist, chunk_border_instruments, &chunk_count, entry_count);
    chunk_border_sounds = chunk_count;

    char fpath[1000];
    printf("Input the path to the file with permaloaded entries and type/subtype dependencies:\n");
    scanf(" %[^\n]",fpath);
    if (fpath[0]=='\"')
    {
        strcpy(fpath,fpath+1);
        *(strchr(fpath,'\0')-1) = '\0';
    }

    LIST permaloaded;
    DEPENDENCIES subtype_info;
    if (!get_info(&permaloaded, &subtype_info, fpath, elist, entry_count))
    {
        printf("File could not be opened or a different error occured\n");
        return;
    }
    make_load_lists(elist, entry_count, gool_table, permaloaded, subtype_info);

    printf("Building T11s' chunks.\n");
    // tries to do T11 and their relatives' chunk assignment
    // 0 - grouped, 1 - one by one
    create_proto_chunks_gool(elist, entry_count, &chunk_count, config[0]);

    // include demo and vcol entries, merge into gool_chunks
    for (i = 0; i < entry_count; i++)
        if (entry_type(elist[i]) == ENTRY_TYPE_DEMO || entry_type(elist[i]) == ENTRY_TYPE_VCOL)
            elist[i].chunk = chunk_count++;

    // chunk_border_gool = chunk_count;


    printf("Building zones' chunks.\n");
    // tries to do zones and their relatives' chunk assignment
    // 0 - grouped, 1 - one by one
    create_proto_chunks_zones(elist, entry_count, &chunk_count, config[1]);

    printf("Merging protochunks\n");
    // 0 - per delta load list check, 1 - per point load list check
    waren_load_list_merge(elist, entry_count, chunk_border_sounds, &chunk_count, config[2]);
    chunk_count = remove_empty_chunks(chunk_border_sounds, chunk_count, entry_count, elist);

    // load list merge might be used just as a metric
    payload_merge(elist, entry_count, chunk_border_sounds, &chunk_count);
    dumb_merge(elist, chunk_border_sounds, &chunk_count, entry_count);

    // only opens the nsf, does not write yet
    *(strrchr(nsfpath,'\\') + 1) = '\0';
    sprintf(lcltemp,"%s\\S00000%02X.NSF", nsfpath, level_ID);
    nsfnew = fopen(lcltemp, "wb");

    // opens nsd, writes it, sorts load lists
    *(strchr(lcltemp, '\0') - 1) = 'D';
    write_nsd(lcltemp, elist, entry_count, chunk_count, spawns, gool_table, level_ID);

    printf("Building actual chunks.\n");
    build_real_chunks(elist, entry_count, chunk_border_texture, chunk_border_sounds, chunk_count, chunks);

    // prints entries and their stats and relatives, then chunks and their EIDs
    if (PRINTING)
    {
        print(elist, entry_count);
        for (i = 0; i < chunk_count; i++)
            printf("%03d: %s\n",i, eid_conv(from_u32(chunks[i] + 4), temp));
    }

    printf("Writing new NSF.\n");
    // writes into a new nsf
    for (i = 0; i < chunk_count; i++)
        fwrite(chunks[i], sizeof(unsigned char), CHUNKSIZE, nsfnew);

    // frees stuff and closes stuff
    for (i = 0; i < chunk_count; i++)
        free(chunks[i]);

    for (i = 0; i < entry_count; i++)
    {
        if (elist[i].data != NULL)
            free(elist[i].data);
        if (elist[i].related != NULL)
            free(elist[i].related);
    }

    fclose(nsfnew);
    fclose(nsf);
    closedir(df);
}

