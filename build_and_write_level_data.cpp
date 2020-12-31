#include "macros.h"


void build_write_nsf(FILE *nsfnew, ENTRY *elist, int entry_count, int chunk_border_sounds, int chunk_count, unsigned char** chunks) {
    build_normal_chunks(elist, entry_count, chunk_border_sounds, chunk_count, chunks);
    for (int i = 0; i < chunk_count; i++)
        fwrite(chunks[i], sizeof(unsigned char), CHUNKSIZE, nsfnew);
    fclose(nsfnew);
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
void build_normal_chunks(ENTRY *elist, int entry_count, int chunk_border_sounds, int chunk_count, unsigned char **chunks) {
    int i, j, sum = 0;
    for (i = chunk_border_sounds; i < chunk_count; i++) {
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
        if (elist[j].chunk == i) {
            offsets[indexer + 1] = offsets[indexer] + elist[j].esize;
            indexer++;
        }

        // writes offsets
        for (j = 0; j < local_entry_count + 1; j++)
            *(unsigned int *) (chunks[i] + 0x10 + j * 4) = offsets[j];

        // writes entries
        int curr_offset = offsets[0];
        for (j = 0; j < entry_count; j++) {
            if (elist[j].chunk == i) {
                memcpy(chunks[i] + curr_offset, elist[j].data, elist[j].esize);
                curr_offset += elist[j].esize;
            }
        }

        sum += curr_offset;
        *((unsigned int *)(chunks[i] + 0xC)) = nsfChecksum(chunks[i]);
    }

    printf("Average normal chunk portion taken: %.3f\n", (100 * (double) sum / (chunk_count - chunk_border_sounds)) / CHUNKSIZE);
}


/** \brief
 *  Builds nsd, sorts load lists according to the nsd entry table order.
 *  Saves at the specified path.
 *  < 0x400 stays empty
 *  The rest should be ok. (its not)
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
void build_write_nsd(FILE *nsd, ENTRY *elist, int entry_count, int chunk_count, SPAWNS spawns, unsigned int* gool_table, int level_ID) {
    int i, x = 0;

    unsigned char* nsddata = (unsigned char*) calloc(CHUNKSIZE, 1);
    *(int *)(nsddata + C2_NSD_CHUNK_COUNT) = chunk_count;

    for (i = 0; i < entry_count; i++)
        if (elist[i].chunk != -1) {
            *(int *)(nsddata + C2_NSD_ENTRY_TABLE + 8*x) = elist[i].chunk * 2 + 1;
            *(int *)(nsddata + C2_NSD_ENTRY_TABLE + 4 + 8*x) = elist[i].EID;
            x++;
        }

    *(int *)(nsddata + C2_NSD_ENTRY_COUNT) = x;

    int end = C2_NSD_ENTRY_TABLE + x * 8;
    *(int *)(nsddata + end) = spawns.spawn_count;
    *(int *)(nsddata + end + 8) = level_ID;
    end += 0x10;

    for (i = 0; i < 0x40; i++)
        *(int *)(nsddata + end + i * 4) = gool_table[i];

    end += 0x1CC;

    for (i = 0; i < spawns.spawn_count; i++) {
        *(int *)(nsddata + end) = spawns.spawns[i].zone;
        *(int *)(nsddata + end + 0x0C) = spawns.spawns[i].x * 256;
        *(int *)(nsddata + end + 0x10) = spawns.spawns[i].y * 256;
        *(int *)(nsddata + end + 0x14) = spawns.spawns[i].z * 256;
        end += 0x18;
    }

    fwrite(nsddata, 1, end, nsd);
    fclose(nsd);

    // sorts load lists
    for (i = 0; i < entry_count; i++) {
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].data != NULL) {
            int cam_count = build_get_cam_count(elist[i].data) / 3;

            for (int j = 0; j < cam_count; j++) {
                int cam_offset = from_u32(elist[i].data + 0x18 + 0xC * j);
                for (int k = 0; (unsigned) k < from_u32(elist[i].data + cam_offset + 0xC); k++) {
                    int code = from_u16(elist[i].data + cam_offset + 0x10 + 8 * k);
                    int offset = from_u16(elist[i].data + cam_offset + 0x12 + 8 * k) + OFFSET + cam_offset;
                    int list_count = from_u16(elist[i].data + cam_offset + 0x16 + 8 * k);
                    if (code == ENTITY_PROP_CAM_LOAD_LIST_A || code == ENTITY_PROP_CAM_LOAD_LIST_B) {
                        int sub_list_offset = offset + 4 * list_count;
                        for (int l = 0; l < list_count; l++) {
                            int item_count = from_u16(elist[i].data + offset + l * 2);
                            ITEM list[item_count];
                            for (int m = 0; m < item_count; m++) {
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
}

