#include "macros.h"


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
void build_merge_main(ENTRY *elist, int entry_count, int chunk_border_sounds, int *chunk_count, int* config, LIST permaloaded) {
    build_permaloaded_merge(elist, entry_count, chunk_border_sounds, chunk_count, permaloaded);
    build_assign_primary_chunks_all(elist, entry_count, chunk_count, config);
    build_matrix_merge_main(elist, entry_count, chunk_border_sounds, chunk_count, config[2]);
    deprecate_build_payload_merge(elist, entry_count, chunk_border_sounds, chunk_count);
    build_dumb_merge(elist, chunk_border_sounds, chunk_count, entry_count);
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
void build_matrix_merge_main(ENTRY *elist, int entry_count, int chunk_border_sounds, int* chunk_count, int merge_flag) {
    int i, j, l, m;
    LIST entries = init_list();

    for (i = 0; i < entry_count; i++) {
        switch (build_entry_type(elist[i])) {
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
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].data != NULL) {
            int item1off = from_u32(elist[i].data + 0x10);
            int cam_count = from_u32(elist[i].data + item1off + 0x188) / 3;

            for (j = 0; j < cam_count; j++) {
                int cam_offset = from_u32(elist[i].data + 0x10 + 4 * (2 + 3 * j));
                LOAD_LIST load_list = build_get_lists(ENTITY_PROP_CAM_LOAD_LIST_A, elist[i].data, cam_offset);
                int cam_length = build_get_path_length(elist[i].data + cam_offset);

                LIST list = init_list();
                switch(merge_flag) {
                    // per point
                    case 1: {
                        int sublist_index = 0;
                        int rating = 0;
                        for (l = 0; l < cam_length; l++) {
                            rating++;
                            if (load_list.array[sublist_index].index == l) {
                                if (load_list.array[sublist_index].type == 'A')
                                    for (m = 0; m < load_list.array[sublist_index].list_length; m++)
                                        list_add(&list, load_list.array[sublist_index].list[m]);

                                if (load_list.array[sublist_index].type == 'B')
                                    for (m = 0; m < load_list.array[sublist_index].list_length; m++)
                                        list_remove(&list, load_list.array[sublist_index].list[m]);

                                sublist_index++;
                                build_increment_common(list, entries, entry_matrix, 1);
                                rating = 0;
                            }
                        }
                        break;
                    }
                    // per delta item
                    case 0: {
                        for (l = 0; l < load_list.count; l++) {
                            if (load_list.array[l].type == 'A')
                                for (m = 0; m < load_list.array[l].list_length; m++)
                                    list_add(&list, load_list.array[l].list[m]);

                            if (load_list.array[l].type == 'B')
                                for (m = 0; m < load_list.array[l].list_length; m++)
                                    list_remove(&list, load_list.array[l].list[m]);

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
 *  Merges chunks prioritising highest combined size.
 *  Does not consider anything else.
 *
 * \param elist ENTRY*                  entry list
 * \param chunk_index_start int         start of the range
 * \param chunk_index_end int*          end of the range
 * \param entry_count int               entry count
 * \return void
 */
void build_dumb_merge(ENTRY *elist, int chunk_index_start, int *chunk_index_end, int entry_count) {
    int i, j, k;
    while(1) {
        int merge_happened = 0;
        for (i = chunk_index_start; i < *chunk_index_end; i++) {
            int size1 = 0;
            int count1 = 0;

            for (j = 0; j < entry_count; j++)
                if (elist[j].chunk == i) {
                    size1 += elist[j].esize;
                    count1++;
                }

            int maxsize = 0;
            int maxentry_count = 0;

            for (j = i + 1; j < *chunk_index_end; j++) {
                int size2 = 0;
                int count2 = 0;
                for (k = 0; k < entry_count; k++)
                    if (elist[k].chunk == j) {
                        size2 += elist[k].esize;
                        count2++;
                    }

                if ((size1 + size2 + 4 * count1 + 4 * count2 + 0x14) <= CHUNKSIZE)
                    if (size2 > maxsize) {
                        maxsize = size2;
                        maxentry_count = j;
                    }
            }

            if (maxentry_count) {
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
 *  For each pair of entries it increments corresponding matrix tile.
 *
 * \param list LIST                     current load list
 * \param entries LIST                  list of valid normal chunk entries
 * \param entry_matrix int**            triangle matrix that contains amount of common load list occurences of entries[i], entries[j] on each tile [i][j]
 * \param rating int                    increment value (used to properly consider all camera points without doing it for every single point)
 * \return void
 */
void build_increment_common(LIST list, LIST entries, int **entry_matrix, int rating) {
    for (int i = 0; i < list.count; i++)
        for (int j = i + 1; j < list.count; j++) {
            int indexA = list_find(entries, list.eids[i]);
            int indexB = list_find(entries, list.eids[j]);

            if (indexA < indexB)
                swap_ints(&indexA, &indexB);

            if (indexA == -1 || indexB == -1)
                continue;

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
void build_matrix_merge_util(RELATIONS relations, ENTRY *elist, int entry_count, LIST entries) {
    for (int x = 0; x < relations.count; x++) {
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
RELATIONS build_transform_matrix(LIST entries, int **entry_matrix) {
    RELATIONS relations;
    relations.count = (entries.count * (entries.count - 1)) / 2;
    relations.relations = (RELATION *) malloc(relations.count * sizeof(RELATION));

    int i, j, indexer = 0;
    for (i = 0; i < entries.count; i++)
        for (j = 0; j < i; j++) {
            relations.relations[indexer].value = entry_matrix[i][j];
            relations.relations[indexer].index1 = i;
            relations.relations[indexer].index2 = j;
            indexer++;
        }

    qsort(relations.relations, relations.count, sizeof(RELATION), relations_cmp);
    return relations;
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
void build_permaloaded_merge(ENTRY *elist, int entry_count, int chunk_border_sounds, int *chunk_count, LIST permaloaded) {
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
            if (sizes[j] + 4 + list[i].esize <= CHUNKSIZE) {
                list[i].chunk = count + j;
                sizes[j] += 4 + list[i].esize;
                break;
            }

    for (i = 0; i < entry_count; i++)
        for (j = 0; j < perma_normal_entry_count; j++)
            if (elist[i].EID == list[j].EID)
                elist[i].chunk = list[j].chunk;

    int counter = 0;
    for (i = 0; i < 8; i++)
        if (sizes[i] > 0x14)
            counter = i + 1;

    *chunk_count = count + counter;
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
int build_remove_empty_chunks(int index_start, int index_end, int entry_count, ENTRY *entry_list) {
    int i, j, is_occupied = 0, empty_chunk = 0;
    while (1) {
        for (i = index_start; i < index_end; i++) {
            is_occupied = 0;
            empty_chunk = -1;
            for (j = 0; j < entry_count; j++)
                if (entry_list[j].chunk == i) is_occupied++;

            if (!is_occupied) {
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

