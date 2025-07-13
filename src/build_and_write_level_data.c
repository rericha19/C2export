#include "macros.h"
// contains functions responsible for transforming existing entry->chunk assignmened data
// into playable/output nsd/nsf files

/** \brief
 *  Calls a function that builds chunks based on previous merge decisions or other chunk assignments
 *  Writes to the already open output nsf file, closes it.
 *
 * \param nsfnew FILE*                  output file
 * \param elist ENTRY*                  entry list
 * \param entry_count int32_t               entry count
 * \param chunk_border_sounds int32_t       index of the first normal chunk, only chunks afterwards get built (normal chunks)
 * \param chunk_count int32_t               chunk count
 * \param chunks uint8_t**        array with built chunks
 * \return void
 */
void build_write_nsf(FILE *nsfnew, ENTRY *elist, int32_t entry_count, int32_t chunk_border_sounds, int32_t chunk_count, uint8_t **chunks, FILE *nsfnew2)
{
    for (int32_t i = 0; i < chunk_count; i++)
        fwrite(chunks[i], sizeof(uint8_t), CHUNKSIZE, nsfnew);
    if (nsfnew2 != NULL)
        for (int32_t i = 0; i < chunk_count; i++)
            fwrite(chunks[i], sizeof(uint8_t), CHUNKSIZE, nsfnew2);
}

/** \brief
 *  Builds chunks according to entries' assigned chunks, shouldnt require any further patches.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int32_t               entry count
 * \param chunk_border_sounds int32_t       index where sounds end
 * \param chunk_count int32_t               chunk count
 * \param chunks uint8_t**        array of chunks
 * \return void
 */
void build_normal_chunks(ENTRY *elist, int32_t entry_count, int32_t chunk_border_sounds, int32_t chunk_count, uint8_t **chunks, int32_t do_end_print)
{
    int32_t i, j, sum = 0;
    // texture, wavebank and sound chunks are already taken care of, thats why it starts after sounds
    for (i = chunk_border_sounds; i < chunk_count; i++)
    {
        int32_t chunk_no = 2 * i + 1;
        int32_t local_entry_count = 0;
        for (j = 0; j < entry_count; j++)
            if (elist[j].chunk == i)
                local_entry_count++;

        chunks[i] = (uint8_t *)calloc(CHUNKSIZE, sizeof(uint8_t)); // freed by build_main
        // header stuff
        *(uint16_t *)chunks[i] = MAGIC_CHUNK;
        *(uint16_t *)(chunks[i] + 4) = chunk_no;
        *(uint16_t *)(chunks[i] + 8) = local_entry_count;

        uint32_t *offsets = (uint32_t *)malloc((local_entry_count + 2) * sizeof(uint32_t)); // idr why +2 but ok, freed here
        // calculates offsets
        int32_t indexer = 0;
        offsets[0] = 0x10 + (local_entry_count + 1) * 4;

        for (j = 0; j < entry_count; j++)
            if (elist[j].chunk == i)
            {
                offsets[indexer + 1] = offsets[indexer] + elist[j].esize;
                indexer++;
            }

        // writes offsets
        for (j = 0; j < local_entry_count + 1; j++)
            *(uint32_t *)(chunks[i] + 0x10 + j * 4) = offsets[j];

        // writes entries
        int32_t curr_offset = offsets[0];
        for (j = 0; j < entry_count; j++)
        {
            if (elist[j].chunk == i)
            {
                memcpy(chunks[i] + curr_offset, elist[j].data, elist[j].esize);
                curr_offset += elist[j].esize;
            }
        }

        sum += curr_offset;                                                          // for avg
        *((uint32_t *)(chunks[i] + CHUNK_CHECKSUM_OFFSET)) = nsfChecksum(chunks[i]); // chunk checksum
        free(offsets);
    }

    if (do_end_print)
        printf("Average normal chunk portion taken: %.3f%c\n", (100 * (double)sum / (chunk_count - chunk_border_sounds)) / CHUNKSIZE, '%');
}

uint32_t build_get_ldat_eid(int32_t level_ID)
{
    char eid[6] = "DATxL\0";

    const char charset[] =
        "0123456789"
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "_!";

    eid[3] = charset[level_ID];

    return eid_to_int(eid);
}

/** \brief
 *  Builds nsd, sorts load lists according to the nsd entry table order.
 *  Saves at the specified path.
 *  < 0x400 stays empty
 *  The rest should be ok. (its not)
 *  Lets you pick the main spawn.
 *
 * \param path char *                    path to the nsd
 * \param elist ENTRY*                  entry list
 * \param entry_count int32_t               entry count
 * \param chunk_count int32_t               chunk count
 * \param spawns SPAWNS                 spawns struct
 * \param gool_table uint32_t*      table that contains gool eids on corresponding indices
 * \param level_ID int32_t                  level ID the user wanted
 * \return void
 */
void build_write_nsd(FILE *nsd, FILE *nsd2, ENTRY *elist, int32_t entry_count, int32_t chunk_count, SPAWNS spawns, uint32_t *gool_table, int32_t level_ID)
{
    int32_t real_entry_count = 0;

    // arbitrarily doing 64kB because convenience
    uint8_t *nsddata = (uint8_t *)calloc(CHUNKSIZE, 1); // freed here

    // count actual entries (some entries might not have been assigned a chunk for a reason, those are left out)
    for (int32_t i = 0; i < entry_count; i++)
        if (elist[i].chunk != -1)
        {
            *(int32_t *)(nsddata + C2_NSD_ENTRY_TABLE_OFFSET + 8 * real_entry_count) = elist[i].chunk * 2 + 1;
            *(int32_t *)(nsddata + C2_NSD_ENTRY_TABLE_OFFSET + 8 * real_entry_count + 4) = elist[i].eid;
            real_entry_count++;
        }

    // write chunk and real entry count
    *(int32_t *)(nsddata + C2_NSD_CHUNK_COUNT_OFFSET) = chunk_count;
    *(int32_t *)(nsddata + C2_NSD_ENTRY_COUNT_OFFSET) = real_entry_count;
    *(uint32_t *)(nsddata + C2_NSD_DATxL_EID) = build_get_ldat_eid(level_ID);

    // write spawn count, level ID
    int32_t real_nsd_size = C2_NSD_ENTRY_TABLE_OFFSET + real_entry_count * 8;
    *(int32_t *)(nsddata + real_nsd_size) = spawns.spawn_count + 1;
    *(int32_t *)(nsddata + real_nsd_size + 8) = level_ID;
    real_nsd_size += 0x10;

    // gool table, idr why nsd size gets incremented by 0x1CC, theres some dumb gap in the nsd after the table
    for (int32_t i = 0; i < 0x40; i++)
        *(int32_t *)(nsddata + real_nsd_size + i * 4) = gool_table[i];
    real_nsd_size += 0x1CC;

    // write spawns, assumes camera 'spawns' on the zone's first cam path's first point (first == 0th)
    *(int32_t *)(nsddata + real_nsd_size) = spawns.spawns[0].zone;
    *(int32_t *)(nsddata + real_nsd_size + 0x0C) = spawns.spawns[0].x << 8;
    *(int32_t *)(nsddata + real_nsd_size + 0x10) = spawns.spawns[0].y << 8;
    *(int32_t *)(nsddata + real_nsd_size + 0x14) = spawns.spawns[0].z << 8;
    real_nsd_size += 0x18;

    for (int32_t i = 0; i < spawns.spawn_count; i++)
    {
        *(int32_t *)(nsddata + real_nsd_size) = spawns.spawns[i].zone;
        *(int32_t *)(nsddata + real_nsd_size + 0x0C) = spawns.spawns[i].x << 8;
        *(int32_t *)(nsddata + real_nsd_size + 0x10) = spawns.spawns[i].y << 8;
        *(int32_t *)(nsddata + real_nsd_size + 0x14) = spawns.spawns[i].z << 8;
        real_nsd_size += 0x18;
    }

    // write and close nsd
    fwrite(nsddata, 1, real_nsd_size, nsd);
    if (nsd2 != NULL)
        fwrite(nsddata, 1, real_nsd_size, nsd2);
    free(nsddata);
}

/** \brief
 *  Sorts load lists according to the NSD entry table order. I think.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int32_t               entry list length
 * \return void
 */
void build_sort_load_lists(ENTRY *elist, int32_t entry_count)
{
    // for each zone entry's each camera path it searches properties, if its a load list property it reads the sublists one by one,
    // sorts the load list entries so that the order matches the nsd's entry table's order
    for (int32_t i = 0; i < entry_count; i++)
    {
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].data != NULL)
        {
            int32_t cam_count = build_get_cam_item_count(elist[i].data) / 3;

            for (int32_t j = 0; j < cam_count; j++)
            {
                int32_t cam_offset = build_get_nth_item_offset(elist[i].data, 2 + 3 * j);
                for (int32_t k = 0; (unsigned)k < build_prop_count(elist[i].data + cam_offset); k++)
                {
                    int32_t code = from_u16(elist[i].data + cam_offset + 0x10 + 8 * k);
                    int32_t offset = from_u16(elist[i].data + cam_offset + 0x12 + 8 * k) + OFFSET + cam_offset;
                    int32_t list_count = from_u16(elist[i].data + cam_offset + 0x16 + 8 * k);
                    if (code == ENTITY_PROP_CAM_LOAD_LIST_A || code == ENTITY_PROP_CAM_LOAD_LIST_B)
                    {
                        int32_t sub_list_offset = offset + 4 * list_count;
                        for (int32_t l = 0; l < list_count; l++)
                        {
                            int32_t item_count = from_u16(elist[i].data + offset + l * 2);
                            LOAD_LIST_ITEM_UTIL *item_list = (LOAD_LIST_ITEM_UTIL *)malloc(item_count * sizeof(LOAD_LIST_ITEM_UTIL)); // freed here
                            for (int32_t m = 0; m < item_count; m++)
                            {
                                item_list[m].eid = from_u32(elist[i].data + sub_list_offset + 4 * m);
                                item_list[m].index = build_get_index(item_list[m].eid, elist, entry_count);
                            }
                            qsort(item_list, item_count, sizeof(LOAD_LIST_ITEM_UTIL), load_list_sort);
                            for (int32_t m = 0; m < item_count; m++)
                                *(uint32_t *)(elist[i].data + sub_list_offset + 4 * m) = item_list[m].eid;
                            sub_list_offset += item_count * 4;
                            free(item_list);
                        }
                    }
                }
            }
        }
    }
}

// used to sort load lists by index
int32_t load_list_sort(const void *a, const void *b)
{
    LOAD_LIST_ITEM_UTIL x = *(LOAD_LIST_ITEM_UTIL *)a;
    LOAD_LIST_ITEM_UTIL y = *(LOAD_LIST_ITEM_UTIL *)b;

    return (x.index - y.index);
}
