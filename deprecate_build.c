#include "macros.h"
// contains deprecate but not entirely unused implementation of the chunk merging/building algorithm
// mainly payload based decisions and some other older stuff

/** \brief
 *  Used by payload method, deprecate.
 *  Creates a payloads object that contains each zone and chunks that zone loads.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_min int                 used to get rid of invalid chunks/entries
 * \return PAYLOADS                     payloads struct
 */
PAYLOADS deprecate_build_get_payload_ladder(ENTRY *elist, int entry_count, int chunk_min) {
    PAYLOADS payloads;
    payloads.count = 0;
    payloads.arr = NULL;
    int i, j, l, m;
    for (i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].data != NULL)
        {
            int cam_count = build_get_cam_item_count(elist[i].data) / 3;
            for (j = 0; j < cam_count; j++)
            {
                LOAD_LIST load_list = build_get_load_lists(elist[i].data, 2 + 3 * j);

                LIST list = init_list();
                PAYLOAD payload;
                for (l = 0; l < load_list.count; l++)
                {
                    if (load_list.array[l].type == 'A')
                        for (m = 0; m < load_list.array[l].list_length; m++)
                            list_add(&list, load_list.array[l].list[m]);

                    if (load_list.array[l].type == 'B')
                        for (m = 0; m < load_list.array[l].list_length; m++)
                            list_remove(&list, load_list.array[l].list[m]);

                    // for simultaneous loads and deloads
                    if (l + 1 != load_list.count)
                        if (load_list.array[l].type == 'A' && load_list.array[l + 1].type == 'B')
                            if (load_list.array[l].index == load_list.array[l + 1].index)
                                continue;

                    payload = deprecate_build_get_payload(elist, entry_count, list, elist[i].eid, chunk_min);
                    payload.cam_path = j;
                    deprecate_build_insert_payload(&payloads, payload);
                }
                delete_load_list(load_list);
            }
        }

    return payloads;
}

// main function for payload merge (calls needed merge functions for this method)
void deprecate_build_payload_merge_main(ENTRY* elist, int entry_count, int chunk_border_sounds, int* chunk_count, int* config, LIST permaloaded) {

    build_permaloaded_merge(elist, entry_count, chunk_border_sounds, chunk_count, permaloaded);     // good

    // primary chunk assignments
    deprecate_build_assign_primary_chunks_gool(elist, entry_count, chunk_count, config[CNFG_IDX_DPR_MERGE_GOOL_FLAG]);
    deprecate_build_assign_primary_chunks_zones(elist, entry_count, chunk_count, config[CNFG_IDX_DPR_MERGE_ZONE_FLAG]);
    deprecate_build_assign_primary_chunks_rest(elist, entry_count, chunk_count);

    deprecate_build_payload_merge(elist, entry_count, chunk_border_sounds, chunk_count, PAYLOAD_MERGE_FULL_USE);
    build_dumb_merge(elist, chunk_border_sounds, chunk_count, entry_count);
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
void deprecate_build_payload_merge(ENTRY *elist, int entry_count, int chunk_min, int *chunk_count, int stats_only_flag) {
    while (1)    {
        PAYLOADS payloads = deprecate_build_get_payload_ladder(elist, entry_count, chunk_min);
        qsort(payloads.arr, payloads.count, sizeof(PAYLOAD), cmp_func_payload);

        printf("\n\"Heaviest\" zones:\n");
        for (int k = 0; k < 10; k++) {
            printf("%d\t", k + 1);
            deprecate_build_print_payload(payloads.arr[k], 0);
        }
        printf("\n");

        if (stats_only_flag)
            break;

        if (payloads.arr[0].count < 19) {
            for (int i = 0; i < payloads.count; i++)
                free(payloads.arr[i].chunks);
            break;
        }

        qsort(payloads.arr[0].chunks, payloads.arr[0].count, sizeof(int), cmp_func_int);

        int chunks[1024];
        int count;
        int check = 0;

        for (int i = 0; i < payloads.count && check != 1; i++) {
            count = 0;

            for (int j = 0; j < payloads.arr[i].count; j++) {
                int curr_chunk = payloads.arr[i].chunks[j];
                if (curr_chunk >= chunk_min)
                    chunks[count++] = curr_chunk;
            }
            qsort(chunks, count, sizeof(int), cmp_func_int);
            if (deprecate_build_merge_thing(elist, entry_count, chunks, count))
                check = 1;

            if (check)
            for (int j = chunk_min; j < *chunk_count; j++) {
                int empty = 1;
                for (int k = 0; k < entry_count; k++)
                    if (elist[k].chunk == j)
                        empty = 0;

                if (empty) {
                    for (int k = 0; k < entry_count; k++)
                        if (elist[k].chunk == (*chunk_count - 1))
                            elist[k].chunk = j;
                    (*chunk_count)--;
                }
            }
        }

        for (int i = 0; i < payloads.count; i++)
                free(payloads.arr[i].chunks);
        if (check == 0)
            break;
    }
}



/** \brief
 *  Adds a payload to a payloads struct, overwrites if its already there.
 *
 * \param payloads PAYLOADS*            payloads struct
 * \param insertee PAYLOAD              payload struct
 * \return void
 */
void deprecate_build_insert_payload(PAYLOADS *payloads, PAYLOAD insertee)  {
    for (int i = 0; i < payloads->count; i++)
        if (payloads->arr[i].zone == insertee.zone && payloads->arr[i].cam_path == insertee.cam_path) {
            if (payloads->arr[i].count < insertee.count) {
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
 *  Prints payload.
 *
 * \param payload PAYLOAD               payload struct
 * \param stopper int                   something dumb
 * \return void
 */
void deprecate_build_print_payload(PAYLOAD payload, int stopper) {
    char temp[100] = "";
    printf("Zone: %s; cam path %d; payload: %3d", eid_conv(payload.zone, temp), payload.cam_path, payload.count);
    if (stopper) printf("; stopper: %2d", stopper);
    printf("\n");
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
int deprecate_build_merge_thing(ENTRY *elist, int entry_count, int *chunks, int chunk_count) {
    int i, j, k;
    int best[2] = {-1, -1};
    int relatives1[1024], relatives2[1024];
    int relative_count1, relative_count2;

    int max = 0;

    for (i = 0; i < chunk_count; i++) {
        int size1 = 0;
        int count1 = 0;
        relative_count1 = 0;

        for (j = 0; j < entry_count; j++)
            if (elist[j].chunk == chunks[i]) {
                size1 += elist[j].esize;
                count1++;
                relatives1[relative_count1++] = elist[j].eid;
                if (elist[j].related != NULL)
                    for (k = 0; (unsigned ) k < elist[j].related[0]; k++)
                    relatives1[relative_count1++] = elist[j].related[k + 1];
            }


        for (j = i + 1; j < chunk_count; j++) {
            int size2 = 0;
            int count2 = 0;
            relative_count2 = 0;
            for (k = 0; k < entry_count; k++)
                if (elist[k].chunk == chunks[j])
                {
                    size2 += elist[k].esize;
                    count2++;
                    relatives2[relative_count2++] = elist[j].eid;
                }

            if ((size1 + size2 + 4 * count1 + 4 * count2 + 0x14) <= CHUNKSIZE) {
                int common_count = deprecate_build_get_common(relatives1, relative_count1, relatives2, relative_count2);
                if (common_count > max) {
                    max = common_count;
                    best[0] = chunks[i];
                    best[1] = chunks[j];
                }
            }
        }
    }

    if (best[0] != -1 && best[0] != -1) {
        deprecate_build_chunk_merge(elist, entry_count, best, 2);
        return 1;
    }

    return 0;
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
int deprecate_build_get_common(int* listA, int countA, int *listB, int countB) {
    int i, j, copy_countA = 0, copy_countB = 0;
    int copyA[100];
    int copyB[100];
    qsort(listA, countA, sizeof(int), cmp_func_int);
    qsort(listB, countB, sizeof(int), cmp_func_int);

    for (i = 0; i < countA; i++)
        if (i) {
            if (listA[i] != copyA[copy_countA - 1])
                copyA[copy_countA++] = listA[i];
        }
        else copyA[copy_countA++] = listA[i];

    for (i = 0; i < countB; i++)
        if (i) {
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
 *  Merges the chunks, usually 2.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunks int*                   array of chunks to be considered by the merge thing
 * \param chunk_count int               amount of those chunks
 * \return void
 */
void deprecate_build_chunk_merge(ENTRY *elist, int entry_count, int *chunks, int chunk_count) {
    for (int i = 0; i < entry_count; i++)
        for (int j = 0; j < chunk_count; j++)
            if (elist[i].chunk == chunks[j])
                elist[i].chunk = chunks[0];

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
PAYLOAD deprecate_build_get_payload(ENTRY *elist, int entry_count, LIST list, unsigned int zone, int chunk_min) {
    int chunks[1024];
    int count = 0;
    int curr_chunk;
    int is_there;

    for (int i = 0; i < list.count; i++) {
        int elist_index = build_get_index(list.eids[i], elist, entry_count);
        curr_chunk = elist[elist_index].chunk;

        is_there = 0;
        for (int j = 0; j < count; j++)
            if (chunks[j] == curr_chunk) is_there = 1;

        char temp[100] = "";
        if (!is_there && eid_conv(elist[elist_index].eid, temp)[4] != 'T' && curr_chunk != -1 && curr_chunk >= chunk_min) {
            chunks[count] = curr_chunk;
            count++;
        }
    }

    PAYLOAD temp;
    temp.zone = zone;
    temp.count = count;
    temp.chunks = (int *) malloc(count * sizeof(int));      // freed by payload ladder function, caller 3 layers up iirc
    memcpy(temp.chunks, chunks, sizeof(int) * count);
    return temp;
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
void deprecate_build_gool_merge(ENTRY *elist, int chunk_index_start, int *chunk_index_end, int entry_count) {
    int i, j, k;
    while(1) {
        int merge_happened = 0;
        for (i = chunk_index_start; i < *chunk_index_end; i++) {
            int size1 = 0, count1 = 0, max_rating = 0, max_entry_count = 0;;
            unsigned int relatives[250];
            int relative_counter = 0;

            for (j = 0; j < entry_count; j++)
                if (elist[j].chunk == i) {
                    size1 += elist[j].esize;
                    count1++;
                    if (elist[j].related != NULL)
                        for (k = 0; (unsigned) k < elist[j].related[0]; k++) {
                            relatives[relative_counter] = elist[j].related[k];
                            relative_counter++;
                        }
                }

            for (j = i + 1; j < *chunk_index_end; j++) {
                int size2 = 0;
                int count2 = 0;
                int has_relative = 0;

                for (k = 0; k < entry_count; k++)
                    if (elist[k].chunk == j) {
                        size2 += elist[k].esize;
                        count2++;
                        has_relative += deprecate_build_is_relative(elist[k].eid, relatives, relative_counter);
                    }

                if ((size1 + size2 + 4 * count1 + 4 * count2 + 0x14) <= CHUNKSIZE) {
                    int rating = has_relative;
                    if (rating > max_rating)
                    {
                        max_rating = rating;
                        max_entry_count = j;
                    }
                }
            }

            if (max_entry_count) {
                for (j = 0; j < entry_count; j++)
                    if (elist[j].chunk == max_entry_count) elist[j].chunk = i;
                merge_happened++;
                //printf("merge happened\n");
            }
        }
        if (!merge_happened)
            break;
    }

    *chunk_index_end = build_remove_empty_chunks(chunk_index_start, *chunk_index_end, entry_count, elist);
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
int deprecate_build_is_relative(unsigned int searched, unsigned int *array, int count) {
    for (int i = 0; i < count; i++)
        if (searched == array[i]) return 1; //(count - i)*(count - i)/2;

    return 0;
}

/** \brief
 *  Inserts all stuff loaded by a zone (scenery dependencies, entity dependencies, its own relatives) to the list.
 *  DEPRECATE.
 *
 * \param eid unsigned int              entry whose children are to be added
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param list LIST*                    current load list
 * \param gool_table unsigned int*      gool table
 * \param dependencies DEPENDENCIES     stuff loaded when a certain type/subtype is used
 * \return void
 */
void deprecate_deprecate_build_ll_add_children(unsigned int eid, ENTRY *elist, int entry_count, LIST *list, unsigned int *gool_table, DEPENDENCIES dependencies)
{
    int i, j, k, l;
    int index = build_get_index(eid, elist, entry_count);
    if (index == -1) return;

    if (elist[index].related != NULL)
        for (i = 0; (unsigned) i < elist[index].related[0]; i++)
            list_add(list, elist[index].related[i + 1]);

    list_add(list, elist[index].eid);

    if (build_entry_type(elist[index]) == ENTRY_TYPE_ZONE) {
        int item1off = from_u32(elist[index].data + 0x10);
        LIST neighbours = build_get_neighbours(elist[index].data);

        for (i = 0; i < neighbours.count; i++) {
            int neighbour_index = build_get_index(neighbours.eids[i], elist, entry_count);
            if (neighbour_index == -1) continue;

            int entity_count = build_get_entity_count(elist[neighbour_index].data);
            int cam_count = build_get_cam_item_count(elist[neighbour_index].data);

            for (j = 0; j < entity_count; j++) {
                int entity_offset = from_u32(elist[neighbour_index].data + 0x18 + 4 * cam_count + 4 * j);
                int entity_type = build_get_entity_prop(elist[neighbour_index].data + entity_offset, ENTITY_PROP_TYPE);
                list_add(list, gool_table[entity_type]);
                int entity_subt = build_get_entity_prop(elist[neighbour_index].data + entity_offset, ENTITY_PROP_SUBTYPE);
                int match_found = 0;

                for (k = 0; k < dependencies.count; k++)
                    if (dependencies.array[k].type == entity_type && dependencies.array[k].subtype == entity_subt) {
                        for (l = 0; l < dependencies.array[k].dependencies.count; l++) {
                            list_add(list, dependencies.array[k].dependencies.eids[l]);
                            int index2 = build_get_index(dependencies.array[k].dependencies.eids[l], elist, entry_count);
                            if (index2 == -1) continue;
                            if (build_entry_type(elist[index2]) == ENTRY_TYPE_ANIM) {
                                LIST models = build_get_models(elist[index2].data);
                                for (int m = 0; m < models.count; m++) {
                                    unsigned int model = models.eids[m];

                                    int model_index = build_get_index(model, elist, entry_count);
                                    if (model_index == -1) continue;

                                    list_add(list, model);
                                    build_add_model_textures_to_list(elist[model_index].data, list);
                                }
                            }
                        }
                        match_found++;
                    }

                if (!match_found)
                    printf("Entity type/subtype with no known dependencies [%2d;%2d]\n", entity_type, entity_subt);
            }
        }

        int scenery_count = build_get_scen_count(elist[index].data);
        for (i = 0; i < scenery_count; i++) {
            int scenery_index = build_get_index(from_u32(elist[index].data + item1off + 0x4 + 0x30 * i), elist, entry_count);
            build_add_scen_textures_to_list(elist[scenery_index].data, list);
        }
    }
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
void deprecate_build_assign_primary_chunks_gool(ENTRY *elist, int entry_count, int *real_chunk_count, int grouping_flag) {
    int i, j;
    int size;
    int counter;
    int relative_index;
    int chunk_count = *real_chunk_count;

    switch(grouping_flag) {
        case 0:  // group gool with its kids
        {
            for (i = 0; i < entry_count; i++)
            {
                if (build_entry_type(elist[i]) == ENTRY_TYPE_GOOL)
                {
                    elist[i].chunk = chunk_count;
                    size = elist[i].esize;
                    counter = 0;
                    if (elist[i].related != NULL)
                    for (j = 0; (unsigned) j < elist[i].related[0]; j++)
                    {
                        relative_index = build_get_index(elist[i].related[j + 1], elist, entry_count);
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

                if (build_entry_type(elist[i]) == ENTRY_TYPE_GOOL)
                {
                    elist[i].chunk = chunk_count++;

                    if (elist[i].related != NULL)
                    for (j = 0; (unsigned) j < elist[i].related[0]; j++)
                    {
                        relative_index = build_get_index(elist[i].related[j + 1], elist, entry_count);
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
 *  Just assigns the rest of the entries initial chunks, which is  necessary for the merges.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_count int*              chunk count
 * \return void
 */
void deprecate_build_assign_primary_chunks_rest(ENTRY *elist, int entry_count, int *chunk_count)
{
    for (int i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_DEMO || build_entry_type(elist[i]) == ENTRY_TYPE_VCOL)
            elist[i].chunk = (*chunk_count)++;
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
void deprecate_build_assign_primary_chunks_zones(ENTRY *elist, int entry_count, int *real_chunk_count, int grouping_flag)
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
            if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].related != NULL)
            {
                if (elist[i].chunk != -1) continue;
                elist[i].chunk = chunk_count;
                size = elist[i].esize;
                counter = 0;
                if (elist[i].related != NULL)
                    for (j = 0; (unsigned) j < elist[i].related[0]; j++)
                    {
                        int relative_index = build_get_index(elist[i].related[j + 1], elist, entry_count);
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
            if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].related != NULL)
            {
                if (elist[i].chunk != -1) continue;
                elist[i].chunk = chunk_count++;
                if (elist[i].related != NULL)
                    for (j = 0; (unsigned) j < elist[i].related[0]; j++)
                    {
                        int relative_index = build_get_index(elist[i].related[j + 1], elist, entry_count);
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
 *  Prints entries and their relatives.
 *  Actually unused i think.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \return void
 */
void deprecate_build_print_relatives(ENTRY *elist, int entry_count) {
    char temp[100] = "";
    int i, j;
    for (i = 0; i < entry_count; i++) {
        printf("%04d %s %02d %d\n", i, eid_conv(elist[i].eid, temp), elist[i].chunk, elist[i].esize);
        if (elist[i].related != NULL) {
            printf("------ %5d\n", elist[i].related[0]);
            for (j = 0; j < (signed) elist[i].related[0]; j++) {
                int relative = elist[i].related[j+1];
                printf("--%2d-- %s %2d %5d\n", j + 1, eid_conv(relative, temp),
                       elist[build_get_index(relative, elist, entry_count)].chunk, elist[build_get_index(relative, elist, entry_count)].esize);
            }
        }
    }
}
