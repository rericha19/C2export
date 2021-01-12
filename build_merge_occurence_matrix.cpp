#include "macros.h"
// contains functions responsible for the current chunk merging algorithm implementation

/** \brief
 *  Main merge function.
 *  First merges the permaloaded entries, then using waren's matrix merge idea merges other normal chunk entries.
 *  Deprecate payload merge and dumb merge do not matter much, payload merge is for metrics, dumb merge is just
 *  in case some minor possible merge slipped thru the cracks.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_border_sounds int       index of last sound chunk
 * \param chunk_count int*              chunk count
 * \param config int*                   config[2]: 0/1, determines whether each point of cam paths or only main/deltas are considered
 * \return void
 */
void build_merge_main(ENTRY *elist, int entry_count, int chunk_border_sounds, int *chunk_count, int* config, LIST permaloaded) {
    clock_t time_start = clock();

    build_permaloaded_merge(elist, entry_count, chunk_border_sounds, chunk_count, permaloaded); // merge permaloaded entries' chunks as well as possible
    build_assign_primary_chunks_all(elist, entry_count, chunk_count);                           // chunks start off having one entry per chunk
    build_matrix_merge_main(elist, entry_count, chunk_border_sounds, chunk_count, config);      // current best algorithm
    deprecate_build_payload_merge(elist, entry_count, chunk_border_sounds, chunk_count);        // for payload printout, doesnt do much anymore
    build_dumb_merge(elist, chunk_border_sounds, chunk_count, entry_count);                     // jic something didnt get merged it gets merged

    clock_t time_end = clock();
    printf("Merge took %.3fs\n", ((double) time_end - time_start) / CLOCKS_PER_SEC);
}


/** \brief
 *  Assigns primary chunks to all entries, merges need them.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_count int*              chunk count
 * \return int                          number of entries that have been assigned a chunk in this function
 */
int build_assign_primary_chunks_all(ENTRY *elist, int entry_count, int *chunk_count) {
    int counter = 0;
    for (int i = 0; i < entry_count; i++)
        if (build_is_normal_chunk_entry(elist[i]) && elist[i].chunk == -1) {
            elist[i].chunk = (*chunk_count)++;
            counter++;
        }
    return counter;
}


// builds a triangular matrix that will contain the amount of common load list occurences of i-th and j-th entry
int ** build_get_occurence_matrix(ENTRY *elist, int entry_count, LIST entries, int merge_flag) {
    int i,j,l,m;
    int **entry_matrix = (int **) malloc(entries.count * sizeof(int*));
    for (i = 0; i < entries.count; i++)
        entry_matrix[i] = (int *) calloc((i), sizeof(int));

    // for each zone's each camera path gets load list and based on it increments values of common load list occurences of pairs of entries
    for (i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].data != NULL) {
            int cam_count = build_get_cam_item_count(elist[i].data) / 3;
            for (j = 0; j < cam_count; j++) {
                int cam_offset = get_nth_item_offset(elist[i].data, 2 + 3 * j);
                LOAD_LIST load_list = build_get_lists(ENTITY_PROP_CAM_LOAD_LIST_A, elist[i].data, cam_offset);
                int cam_length = build_get_path_length(elist[i].data + cam_offset);

                LIST list = init_list();
                switch(merge_flag) {
                    // per point (seems to generally be better), but its actually kinda fucky because if its not it gives worse results
                    // if it were properly per point, the 'build_increment_common' function would use the 'counter' variable as 4th arg instead of 1
                    case 2:
                    case 1: {
                        int sublist_index = 0;
                        int counter = 0;
                        for (l = 0; l < cam_length; l++) {
                            counter++;
                            if (load_list.array[sublist_index].index == l) {
                                if (load_list.array[sublist_index].type == 'A')
                                    for (m = 0; m < load_list.array[sublist_index].list_length; m++)
                                        list_add(&list, load_list.array[sublist_index].list[m]);

                                if (load_list.array[sublist_index].type == 'B')
                                    for (m = 0; m < load_list.array[sublist_index].list_length; m++)
                                        list_remove(&list, load_list.array[sublist_index].list[m]);

                                sublist_index++;
                                if (merge_flag == 1)
                                    build_increment_common(list, entries, entry_matrix, 1);
                                if (merge_flag == 2)
                                    build_increment_common(list, entries, entry_matrix, counter);
                                counter = 0;
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
    return entry_matrix;
}

LIST build_get_normal_entry_list(ENTRY *elist, int entry_count) {

    LIST entries = init_list();
    // add all normal chunk entries to a new temporary array
    for (int i = 0; i < entry_count; i++) {
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
    return entries;
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
void build_matrix_merge_main(ENTRY *elist, int entry_count, int chunk_border_sounds, int* chunk_count, int* config) {

    LIST entries = build_get_normal_entry_list(elist, entry_count);
    int **entry_matrix = build_get_occurence_matrix(elist, entry_count, entries, config[2]);

    // put the matrix's contents in an array and sort (so the matrix doesnt have to be searched every time)
    // frees the contents of the matrix
    RELATIONS array_representation = build_transform_matrix(entries, entry_matrix, config);

    // do the merges according to the relation array, get rid of holes afterwards
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
 *  For each pair of entries it increments corresponding triangular matrix tile.
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

            // matrix is triangular
            if (indexA < indexB)
                swap_ints(&indexA, &indexB);

            // something done goofed, shouldnt happen
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
    // for each relation it calculates size of the chunk that would be created by the merge
    // if the size is ok it merges, there are empty chunks afterwards that get cleaned up by a function called
    // after this function
    for (int x = 0; x < relations.count; x++) {
        // awkward index shenanishans because entries in the matrix and later relation array werent indexed the same way
        // elist is, so theres the need to get index of the elist entry and then that one's chunk
        int index1 = relations.relations[x].index1;
        int index2 = relations.relations[x].index2;

        int elist_index1 = build_get_index(entries.eids[index1], elist, entry_count);
        int elist_index2 = build_get_index(entries.eids[index2], elist, entry_count);

        int chunk_index1 = elist[elist_index1].chunk;
        int chunk_index2 = elist[elist_index2].chunk;

        int merged_chunk_size = 0x14;
        for (int i = 0; i < entry_count; i++)
            if (elist[i].chunk == chunk_index1 || elist[i].chunk == chunk_index2)
                merged_chunk_size += elist[i].esize + 4;

        if (merged_chunk_size <= CHUNKSIZE)          // find out whether can do <= instead of < (needs testing)
            for (int i = 0; i < entry_count; i++)
                if (elist[i].chunk == chunk_index2)
                    elist[i].chunk = chunk_index1;
    }
}


/** \brief
 *  Creates an array representation of the common load list occurence matrix, sorts high to low.
 *  To prevent having to search the entire matrix every time in the main merge util thing.
 *
 * \param entries LIST                  valid entries list
 * \param entry_matrix int**            matrix that contains common load list occurences
 * \return RELATIONS
 */
RELATIONS build_transform_matrix(LIST entries, int **entry_matrix, int* config) {
    RELATIONS relations;
    relations.count = (entries.count * (entries.count - 1)) / 2;
    relations.relations = (RELATION *) malloc(relations.count * sizeof(RELATION));

    int i, j, indexer = 0;
    for (i = 0; i < entries.count; i++)
        for (j = 0; j < i; j++) {
            relations.relations[indexer].value = entry_matrix[i][j];
            // experimental
            int temp = 0;
            for (int k = 0; k < i; k++)
                temp += entry_matrix[i][k];
            for (int k = 0; k < j; k++)
                temp += entry_matrix[j][k];
            relations.relations[indexer].total_occurences = temp;
            relations.relations[indexer].index1 = i;
            relations.relations[indexer].index2 = j;
            indexer++;
        }

    if (config[8] == 0)
        qsort(relations.relations, relations.count, sizeof(RELATION), relations_cmp);           // the 'consistent' one
    if (config[8] == 1)
        qsort(relations.relations, relations.count, sizeof(RELATION), relations_cmp2);

    for (i = 0; i < entries.count; i++)
        free(entry_matrix[i]);
    free(entry_matrix);
    return relations;
}


/** \brief
 *  Specially merges permaloaded entries as they dont require any association.
 *  Works similarly to the sound chunk one.
 *  Biggest first sort packing algorithm kinda thing, seems to work pretty ok.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_border_sounds int       index from which normal chunks start
 * \param chunk_count int*              current chunk count
 * \param permaloaded LIST              list with permaloaded entries
 * \return int                          permaloaded chunk count
 */
int build_permaloaded_merge(ENTRY *elist, int entry_count, int chunk_border_sounds, int *chunk_count, LIST permaloaded) {
    int i, j;
    int temp_count = *chunk_count;
    int perma_normal_entry_count = 0;

    // find all permaloaded entries, add them to a temporary list, sort the list in descending order by size (biggest first)
    for (i = 0; i < entry_count; i++)
        if (list_find(permaloaded, elist[i].EID) != -1 && build_is_normal_chunk_entry(elist[i]))
            perma_normal_entry_count++;

    ENTRY perma_elist[perma_normal_entry_count];
    int indexer = 0;
    for (i = 0; i < entry_count; i++)
        if (list_find(permaloaded, elist[i].EID) != -1 && build_is_normal_chunk_entry(elist[i]))
            perma_elist[indexer++] = elist[i];

    qsort(perma_elist, perma_normal_entry_count, sizeof(ENTRY), cmp_entry_size);

    // keep putting them into existing chunks if they fit
    int perma_chunk_count = 250;
    int sizes[perma_chunk_count];
    for (i = 0; i < perma_chunk_count; i++)
        sizes[i] = 0x14;

    // place where fits
    for (i = 0; i < perma_normal_entry_count; i++)
        for (j = 0; j < MAX; j++)
            if (sizes[j] + 4 + perma_elist[i].esize <= CHUNKSIZE) {
                perma_elist[i].chunk = temp_count + j;
                sizes[j] += 4 + perma_elist[i].esize;
                break;
            }
    // to edit the array of entries that is actually used
    for (i = 0; i < entry_count; i++)
        for (j = 0; j < perma_normal_entry_count; j++)
            if (elist[i].EID == perma_elist[j].EID)
                elist[i].chunk = perma_elist[j].chunk;

    // counts used chunks
    int counter = 0;
    for (i = 0; i < 8; i++)
        if (sizes[i] > 0x14)
            counter = i + 1;

    *chunk_count = temp_count + counter;
    return counter;
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

