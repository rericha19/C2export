#include "macros.h"

/** \brief
 *  Used by payload method, deprecate.
 *  Creates a payloads object that contains each zone and chunks that zone loads.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_min int                 used to get rid of invalid chunks/entries
 * \return PAYLOADS                     payloads struct
 */
PAYLOADS deprecate_build_get_payload_ladder(ENTRY *elist, int entry_count, int chunk_min)
{
    PAYLOADS payloads;
    payloads.count = 0;
    payloads.arr = NULL;
    int i, j, k, l, m;
    for (i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].data != NULL)
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

                        payload = deprecate_build_get_payload(elist, entry_count, list, elist[i].EID, chunk_min);
                        deprecate_build_insert_payload(&payloads, payload);
                    }
            }
        }

    return payloads;
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
void deprecate_build_payload_merge(ENTRY *elist, int entry_count, int chunk_min, int *chunk_count)
{
    for (int x = 0; ; x++)
    {
        PAYLOADS payloads = deprecate_build_get_payload_ladder(elist, entry_count, chunk_min);
        qsort(payloads.arr, payloads.count, sizeof(PAYLOAD), pay_cmp);

        printf("%3d  ", x);
        deprecate_build_print_payload(payloads.arr[0], 0);

        if (payloads.arr[0].count < 19) break;

        qsort(payloads.arr[0].chunks, payloads.arr[0].count, sizeof(int), cmpfunc);

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
            if (deprecate_build_merge_thing(elist, entry_count, chunks, count))
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

    build_dumb_merge(elist, chunk_min, chunk_count, entry_count);
}



/** \brief
 *  Adds a payload to a payloads struct, overwrites if its already there.
 *
 * \param payloads PAYLOADS*            payloads struct
 * \param insertee PAYLOAD              payload struct
 * \return void
 */
void deprecate_build_insert_payload(PAYLOADS *payloads, PAYLOAD insertee)
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
 *  Prints payload.
 *
 * \param payload PAYLOAD               payload struct
 * \param stopper int                   something dumb
 * \return void
 */
void deprecate_build_print_payload(PAYLOAD payload, int stopper)
{
    char temp[100];
    printf("Zone: %s; payload: %3d", eid_conv(payload.zone, temp), payload.count);
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
int deprecate_build_merge_thing(ENTRY *elist, int entry_count, int *chunks, int chunk_count)
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
                int common_count = deprecate_build_get_common(relatives1, relative_count1, relatives2, relative_count2);
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
int deprecate_build_get_common(int* listA, int countA, int *listB, int countB)
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
 *  Merges the chunks, usually 2.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunks int*                   array of chunks to be considered by the merge thing
 * \param chunk_count int               amount of those chunks
 * \return void
 */
void deprecate_build_chunk_merge(ENTRY *elist, int entry_count, int *chunks, int chunk_count)
{
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
PAYLOAD deprecate_build_get_payload(ENTRY *elist, int entry_count, LIST list, unsigned int zone, int chunk_min)
{
    int chunks[1024];
    int count = 0;
    int curr_chunk;
    int is_there;
    char help[100];

    for (int i = 0; i < list.count; i++)
    {
        int elist_index = build_get_index(list.eids[i], elist, entry_count);
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
 *  Deprecate merge method function based on common relative count.
 *  Do not use.
 *
 * \param elist ENTRY*                  entry list
 * \param chunk_index_start int         start of the range
 * \param chunk_index_end int*          end of the range (excl.)
 * \param entry_count int               entry count
 * \return void
 */
void deprecate_build_gool_merge(ENTRY *elist, int chunk_index_start, int *chunk_index_end, int entry_count)
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
                        has_relative += deprecate_build_is_relative(elist[k].EID, relatives, relative_counter);
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
int deprecate_build_is_relative(unsigned int searched, unsigned int *array, int count)
{
    for (int i = 0; i < count; i++)
        if (searched == array[i]) return 1; //(count - i)*(count - i)/2;

    return 0;
}