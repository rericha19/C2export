#include "macros.h"
#include "build_merge_state_search.h"

// assumes one entry per chunk, ignores permaloaded entries
void build_premerge_anim_model(ENTRY *elist, int entry_count, int chunk_border_sounds, int* chunk_count, int* config, LIST permaloaded) {
    LIST temp_entries = build_get_normal_entry_list(elist, entry_count);
    LIST anims_models = init_list();
    for (int i = 0; i < temp_entries.count; i++) {
        int entry_type = build_entry_type(elist[build_get_index(temp_entries.eids[i], elist, entry_count)]);
        if (entry_type == ENTRY_TYPE_ANIM || entry_type == ENTRY_TYPE_MODEL)
            if (list_find(permaloaded, temp_entries.eids[i] == -1))
                list_insert(&anims_models, temp_entries.eids[i]);
    }

    int** entry_matrix = build_get_occurence_matrix(elist, entry_count, anims_models, config);
    RELATIONS relation_array = build_transform_matrix(anims_models, entry_matrix, config);
    for (int i = 0; i < anims_models.count; i++)
        free(entry_matrix[i]);
    free(entry_matrix);

     for (int i = 0; i < entry_count; i++) {
        if (build_entry_type(elist[i]) == ENTRY_TYPE_MODEL)
        {
            if (list_find(permaloaded, elist[i].eid) != -1)
                    continue;

            // for each model get a list of animations it uses slsts (already ignore invalid ones or ones that dont fit in a chunk with the zone)
            LIST anims = init_list();
            for (int j = 0; j < entry_count; j++)
                if (build_entry_type(elist[j]) == ENTRY_TYPE_ANIM)
                {
                    int potential_combined_size = 0x14 + 4 * 2 + elist[i].esize + elist[j].esize;
                    if (potential_combined_size > CHUNKSIZE)
                        continue;

                    unsigned int model_ref = build_get_model(elist[j].data);
                    if (model_ref != elist[i].eid)
                        continue;

                    unsigned int anim_eid = elist[j].eid;
                    if (list_find(permaloaded, anim_eid) != -1)
                        continue;

                    list_insert(&anims, anim_eid);
                }

            // find one that gets loaded together with the model the most (most occurences)
            int max_value = -1;
            int chunk_index1 = 0;
            int chunk_index2 = 0;

            for (int j = 0; j < relation_array.count; j++) {
                unsigned int entry1 = anims_models.eids[relation_array.relations[j].index1];
                unsigned int entry2 = anims_models.eids[relation_array.relations[j].index2];
                int value = relation_array.relations[j].value;

                if (((entry1 == elist[i].eid) && (list_find(anims, entry2) != -1)) ||
                    ((entry2 == elist[i].eid) && (list_find(anims, entry1) != -1)) )
                {
                    /*char temp[100]= "", temp2[100] = "";
                    printf("entry %s and %s\n", eid_conv(entry1, temp), eid_conv(entry2, temp2));*/
                    if (value > max_value) {
                        max_value = value;
                        chunk_index1 = elist[build_get_index(entry1, elist, entry_count)].chunk;
                        chunk_index2 = elist[build_get_index(entry2, elist, entry_count)].chunk;
                    }
                }
            }

            // put entry2 into chunk1
            if (max_value > -1)
                for (int j = 0; j < entry_count; j++)
                    if (elist[j].chunk == chunk_index2)
                        elist[j].chunk = chunk_index1;
        }
    }

    *chunk_count = build_remove_empty_chunks(chunk_border_sounds, *chunk_count, entry_count, elist);
}


// assumes one entry per chunk, ignores permaloaded entries
void build_premerge_slst_zone(ENTRY *elist, int entry_count, int chunk_border_sounds, int* chunk_count, int* config, LIST permaloaded) {

    // first part gets relation structs with common occurences of slsts and zones
    LIST temp_entries = build_get_normal_entry_list(elist, entry_count);
    LIST zones_slsts = init_list();
    for (int i = 0; i < temp_entries.count; i++) {
        int index = build_get_index(temp_entries.eids[i], elist, entry_count);
        int entry_type = build_entry_type(elist[index]);
        if (entry_type == ENTRY_TYPE_ZONE || entry_type == ENTRY_TYPE_SLST)
            if (list_find(permaloaded, temp_entries.eids[i] == -1))
                list_insert(&zones_slsts, temp_entries.eids[i]);
    }

    int** entry_matrix = build_get_occurence_matrix(elist, entry_count, zones_slsts, config);
    RELATIONS relation_array = build_transform_matrix(zones_slsts, entry_matrix, config);
    for (int i = 0; i < zones_slsts.count; i++)
        free(entry_matrix[i]);
    free(entry_matrix);

    for (int i = 0; i < entry_count; i++) {
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE)
        {
            if (list_find(permaloaded, elist[i].eid) != -1)
                    continue;

            // for each zone get a list of its slsts (already ignore invalid ones or ones that dont fit in a chunk with the zone)
            LIST slsts = init_list();
            int cam_count = build_get_cam_item_count(elist[i].data) / 3;
            for (int j = 0; j < cam_count; j++) {
                unsigned int offset = build_get_nth_item_offset(elist[i].data, 2 + 3 * j);
                unsigned int slst = build_get_slst(elist[i].data + offset);
                int slst_index = build_get_index(slst, elist, entry_count);
                if (slst_index == -1)
                    continue;

                int potential_combined_size = 0x14 + 4 * 2 + elist[i].esize + elist[slst_index].esize;
                if (potential_combined_size > CHUNKSIZE)
                    continue;

                if (list_find(permaloaded, slst) != -1)
                    continue;

                list_insert(&slsts, slst);
            }

            // find one that gets loaded together with the zone the most (most occurences)
            int max_value = -1;
            int chunk_index1 = 0;
            int chunk_index2 = 0;

            for (int j = 0; j < relation_array.count; j++) {
                unsigned int entry1 = zones_slsts.eids[relation_array.relations[j].index1];
                unsigned int entry2 = zones_slsts.eids[relation_array.relations[j].index2];
                int value = relation_array.relations[j].value;

                if (((entry1 == elist[i].eid) && (list_find(slsts, entry2) != -1)) ||
                    ((entry2 == elist[i].eid) && (list_find(slsts, entry1) != -1)) )
                {
                    /*char temp[100]= "", temp2[100] = "";
                    printf("entry %s and %s\n", eid_conv(entry1, temp), eid_conv(entry2, temp2));*/
                    if (value > max_value) {
                        max_value = value;
                        chunk_index1 = elist[build_get_index(entry1, elist, entry_count)].chunk;
                        chunk_index2 = elist[build_get_index(entry2, elist, entry_count)].chunk;
                    }
                }
            }

            // put entry2 into chunk1
            if (max_value > -1)
                for (int j = 0; j < entry_count; j++)
                    if (elist[j].chunk == chunk_index2)
                        elist[j].chunk = chunk_index1;
        }
    }

    *chunk_count = build_remove_empty_chunks(chunk_border_sounds, *chunk_count, entry_count, elist);
}


void build_state_search_premerge(ENTRY *elist, int entry_count, int chunk_border_sounds, int* chunk_count, int *config, LIST permaloaded) {
    int type = 0;
    double merge_ratio = 0.0;
    build_ask_premerge(&type, &merge_ratio);

    switch(type) {
        default:
        case 0:
            break;
        case 1:
            build_matrix_merge(elist, entry_count, chunk_border_sounds, chunk_count, config, permaloaded, merge_ratio);
            break;
        case 2:
            build_matrix_merge_relative(elist, entry_count, chunk_border_sounds, chunk_count, config, permaloaded, merge_ratio);
            break;
        case 3:
            build_premerge_anim_model(elist, entry_count, chunk_border_sounds, chunk_count, config, permaloaded);
            break;
        case 4:
            build_premerge_slst_zone(elist, entry_count, chunk_border_sounds, chunk_count, config, permaloaded);
            break;
        case 5:
            build_premerge_anim_model(elist, entry_count, chunk_border_sounds, chunk_count, config, permaloaded);
            build_premerge_slst_zone(elist, entry_count, chunk_border_sounds, chunk_count, config, permaloaded);
            break;
    }
}


/** \brief
 *  Main function for the a* merge implementation. Used only for non-permaloaded entries.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_border_sounds int       index of the first normal chunk
 * \param chunk_count int*              chunk count
 * \param config int*                   config array
 * \param permaloaded LIST              list of permaloaded entries
 * \return void
 */
void build_merge_state_search_main(ENTRY *elist, int entry_count, int chunk_border_sounds, int *chunk_count, int* config, LIST permaloaded) {

    int perma_chunk_count = build_permaloaded_merge(elist, entry_count, chunk_border_sounds, chunk_count, permaloaded);
    int first_nonperma_chunk_index = *chunk_count;

    build_assign_primary_chunks_all(elist, entry_count, chunk_count);
    build_state_search_premerge(elist, entry_count, chunk_border_sounds, chunk_count, config, permaloaded);
    build_state_search_solve(elist, entry_count, first_nonperma_chunk_index, perma_chunk_count);
    deprecate_build_payload_merge(elist, entry_count, chunk_border_sounds, chunk_count, PAYLOAD_MERGE_STATS_ONLY);
    build_dumb_merge(elist, chunk_border_sounds, chunk_count, entry_count);
}


/** \brief
 *  Initialises the struct, allocates memory.
 *
 * \param length int                    amount of entry-chunk assignments
 * \return A_STAR_STR*                  initialised state with allocated memory
 */
STATE_SEARCH_STR* build_state_search_str_init(int length) {
    STATE_SEARCH_STR* temp = (STATE_SEARCH_STR*) malloc(sizeof(STATE_SEARCH_STR));                  // freed by caller's caller (i hope)
    temp->estimated = 0;
    temp->elapsed = 0;
    temp->entry_chunk_array = (unsigned short int*) malloc(length * sizeof(unsigned short int));    // freed by caller's caller (i hope)
    return temp;
}


/** \brief
 *  Frees all memory allocated by the state.
 *
 * \param state A_STAR_STR*             state to destroy
 * \return void
 */
void build_state_search_str_destroy(STATE_SEARCH_STR* state) {
    free(state->entry_chunk_array);
    free(state);
}


unsigned int build_state_search_eval_state(LIST* stored_load_lists, int load_list_snapshot_count, STATE_SEARCH_STR* state, int key_length,
                                           int first_nonperma_chunk, int perma_count, int max_payload_limit, int* max_pay) {

    int chunk_count = build_state_search_str_chunk_max(state, key_length) + 1;

    int i, j;
    int index, chunk, counter, maxp = 0, eval = 0;

    unsigned char *chunks = (unsigned char *) malloc(chunk_count * sizeof(unsigned char));
    unsigned char step = 0;

    for (i = 0; i < load_list_snapshot_count; i++) {

        counter = 0;
        step++;
        if (step == 1)
            memset(chunks, 0, chunk_count);

        for (j = 0; j < stored_load_lists[i].count; j++){
            index = stored_load_lists[i].eids[j];
            chunk = state->entry_chunk_array[index];

            if (chunk >= first_nonperma_chunk && chunks[chunk]!= step) {
                chunks[chunk] = step;
                counter++;
            }
        }

        eval += counter;
        maxp = max(maxp, counter);
    }

    eval += perma_count * load_list_snapshot_count;
    maxp += perma_count;
    free(chunks);

    if (max_pay != NULL)
        *max_pay = maxp;

    if (maxp <= max_payload_limit)
        return STATE_SEARCH_EVAL_SUCCESS;
    return eval + maxp * load_list_snapshot_count;
}


/** \brief
 *  Returns value of the highest used chunk in the state.
 *
 * \param state A_STAR_STR*             input state
 * \return int                          index of the last used chunk
 */
int build_state_search_str_chunk_max(STATE_SEARCH_STR* state, int key_length) {
    unsigned short int chunk_last = 0;
    for (int i = 0; i < key_length; i++)
        chunk_last = max(chunk_last, state->entry_chunk_array[i]);

    return chunk_last;
}

int chunk_size_sort_cmp(const void *a, const void *b) {
    CHUNK_STR itemA = *(CHUNK_STR*) a;
    CHUNK_STR itemB = *(CHUNK_STR*) b;

    if (itemA.chunk_size != itemB.chunk_size)
        return itemB.chunk_size - itemA.chunk_size;

    return itemB.entry_count - itemA.entry_count;
}

/** \brief
 *  Creates a new state based on the input state, puts entries from chunk2 into chunk1 (attempts to merge).
 *
 * \param state A_STAR_STR*             input state
 * \param chunk1 unsigned int           index of first chunk
 * \param chunk2 unsigned int           index of second chunk (higher than chunk1)
 * \param key_length int              amount of involved entries
 * \return A_STAR_STR*                  new state
 */
STATE_SEARCH_STR* build_state_search_merge_chunks(STATE_SEARCH_STR* state, unsigned int chunk1, unsigned int chunk2,
                                                  int key_length, int first_nonperma_chunk, ENTRY* temp_elist) {
    int i;
    STATE_SEARCH_STR* new_state = build_state_search_str_init(key_length);

    // tries to merge chunk2 into chunk1, if the resulting chunk would have invalid size, returns NULL
    int resulting_size = 0x14;
    for (i = 0; i < key_length; i++) {
        unsigned short int curr_chunk = state->entry_chunk_array[i];
        if (curr_chunk == chunk2)
            curr_chunk = chunk1;

        if (curr_chunk == chunk1)
            resulting_size += (4 + temp_elist[i].esize);
        new_state->entry_chunk_array[i] = curr_chunk;
    }

    if (resulting_size > CHUNKSIZE) {
        build_state_search_str_destroy(new_state);
        return NULL;
    }

    // orders the chunks in the state in descending order by used chunk size
    /*
    int chunk_count = 1 + build_state_search_str_chunk_max(new_state, key_length);

    CHUNK_STR *chunks_to_sort = (CHUNK_STR*) malloc(chunk_count * sizeof(CHUNK_STR));
    for (i = 0; i < chunk_count; i++) {
        chunks_to_sort[i].chunk_size = 0x14;
        chunks_to_sort[i].chunk_index = i;
        chunks_to_sort[i].entry_count = 0;
    }

    for (i = 0; i < key_length; i++) {
        int chunk_index = new_state->entry_chunk_array[i];
        chunks_to_sort[chunk_index].chunk_size += (4 + temp_elist[i].esize);
        chunks_to_sort[chunk_index].entry_count += 1;
    }

    qsort(chunks_to_sort, chunk_count, sizeof(CHUNK_STR), chunk_size_sort_cmp);
    for (i = 0; i < key_length; i++) {
        int curr_chunk = new_state->entry_chunk_array[i];

        for (int j = 0; j < chunk_count; j++)
            if (chunks_to_sort[j].chunk_index == curr_chunk) {
                new_state->entry_chunk_array[i] = j + first_nonperma_chunk;
                break;
            }
    }

    free(chunks_to_sort);*/
    return new_state;
}


/** \brief
 *  Converts input entry list with initial chunk assignments to a struct used in the a_star for storing states.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param first_nonperma_chunk_index int         chunk where involved entries start
 * \param key_length int              amount of involved entries
 * \return A_STAR_STR*                  struct with entry-chunk assignments recorded
 */
STATE_SEARCH_STR* build_state_search_init_state_convert(ENTRY* elist, int entry_count, int first_nonperma_chunk_index, int key_length) {
    STATE_SEARCH_STR* init_state = build_state_search_str_init(key_length);
    int indexer = 0;
    for (int i = 0; i < entry_count; i++)
        if (elist[i].chunk >= first_nonperma_chunk_index) {
            init_state->entry_chunk_array[indexer] = elist[i].chunk;
            indexer++;
        }

    return init_state;
}


int build_state_search_is_empty_chunk(STATE_SEARCH_STR* state, unsigned int chunk_index, int key_length) {

    int has_something = 0;
    for (int k = 0; k < key_length; k++) {
        if (state->entry_chunk_array[k] == chunk_index)
            has_something = 1;
    }

    return !has_something;
}


unsigned int* build_state_search_init_elist_convert(ENTRY *elist, int entry_count, int first_nonperma_chunk_index, int *key_length) {

    int counter = 0;
    for (int i = 0; i < entry_count; i++)
        if (elist[i].chunk >= first_nonperma_chunk_index)
            counter++;

    unsigned int* eid_list =(unsigned int*) malloc(counter * sizeof(unsigned int));     // freed by caller

    counter = 0;
    for (int i = 0; i < entry_count; i++)
        if (elist[i].chunk >= first_nonperma_chunk_index)
            eid_list[counter++] = elist[i].eid;

    *key_length = counter;
    return eid_list;
}


LIST* build_state_search_eval_util(ENTRY *elist, int entry_count, int first_nonperma_chunk, ENTRY **temp_elist, int *key_length, int* ll_snap_count) {

    int lcl_key_length = 0;;
    // also gets key length
    unsigned int* eid_list = build_state_search_init_elist_convert(elist, entry_count, first_nonperma_chunk, &lcl_key_length);
    ENTRY *local_temp_elist = (ENTRY *) malloc(lcl_key_length * sizeof(ENTRY));

    for (int i = 0; i < lcl_key_length; i++) {
        local_temp_elist[i].eid = eid_list[i];
        int master_elist_index = build_get_index(eid_list[i], elist, entry_count);
        local_temp_elist[i].data = elist[master_elist_index].data;
        local_temp_elist[i].esize = elist[master_elist_index].esize;
    }
    free(eid_list);

    LIST *stored_load_lists = NULL;

    int snapshot_counter = 0;
    int j, l, m;
    for (int i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].data != NULL)
        {
            int cam_count = build_get_cam_item_count(elist[i].data) / 3;
            for (j = 0; j < cam_count; j++)
            {
                // part that gets load lists
                LOAD_LIST load_list = build_get_load_lists(elist[i].data, 2 + 3 * j);

                // part that stores 'snapshots' used by eval_state
                LIST list = init_list();
                for (l = 0; l < load_list.count; l++) {
                    if (load_list.array[l].type == 'A')
                        for (m = 0; m < load_list.array[l].list_length; m++) {
                            unsigned int eid = load_list.array[l].list[m];
                            int index = build_get_index(eid, local_temp_elist, lcl_key_length);
                            if (index == -1)
                                continue;

                            list_insert(&list, index);
                        }

                    if (load_list.array[l].type == 'B')
                        for (m = 0; m < load_list.array[l].list_length; m++) {
                            unsigned int eid = load_list.array[l].list[m];
                            int index = build_get_index(eid, local_temp_elist, lcl_key_length);
                            if (index == -1)
                                continue;

                            list_remove(&list, index);
                        }

                    if (stored_load_lists == NULL)
                        stored_load_lists = (LIST*) malloc(1 * sizeof(LIST));
                    else
                        stored_load_lists = (LIST*) realloc(stored_load_lists, (snapshot_counter + 1) * sizeof(LIST));

                    stored_load_lists[snapshot_counter] = init_list();
                    list_copy_in(&stored_load_lists[snapshot_counter], list);
                    snapshot_counter++;
                }

                delete_load_list(load_list);
            }
        }

    while (1) {
        int found_condensation = 0;
        for (int i = 0; i < snapshot_counter; i++) {
            // to prevent possibly breaking things
            if (found_condensation)
                break;
            LIST listA = stored_load_lists[i];

            // see whether any list is a superset of listA
            for (int j = 0; j < i; j++) {
                LIST listB = stored_load_lists[j];
                int is_proper_subset = 1;

                // check whether everything from listA is in listB (listA c ListB)
                for (int k = 0; k < listA.count; k++) {
                    if (list_find(listB, listA.eids[k]) == -1) {
                        is_proper_subset = 0;
                        break;
                    }
                }

                // if listA is subset of current listB, listA is not necessary
                // shove the last thing there, realloc array
                if (is_proper_subset) {
                    found_condensation = 1;
                    stored_load_lists[i] = stored_load_lists[snapshot_counter - 1];
                    snapshot_counter--;
                    stored_load_lists = realloc(stored_load_lists, snapshot_counter * sizeof(LIST));
                    break;
                }
            }
        }

        if (!found_condensation)
            break;
    }

    *temp_elist = local_temp_elist;
    *key_length = lcl_key_length;
    *ll_snap_count = snapshot_counter;
    return stored_load_lists;
}


void build_state_search_solve_cleanup(STATE_SEARCH_HEAP *heap, HASH_TABLE *table, LIST *stored_load_lists, ENTRY *temp_elist, STATE_SEARCH_STR *best) {
    heap_destroy(heap);
    hash_destroy_table(table);
    free(stored_load_lists);
    free(temp_elist);
    build_state_search_str_destroy(best);
}

int cmp_state_search_a(const void *a, const void *b) {
    STATE_SEARCH_STR* itemA = *(STATE_SEARCH_STR**) a;
    STATE_SEARCH_STR* itemB = *(STATE_SEARCH_STR**) b;

    int valueA = itemA->elapsed + itemA->estimated;
    int valueB = itemB->elapsed + itemB->estimated;

    if (valueA - valueB != 0)
        return (valueA - valueB);
    return itemA->estimated - itemB->estimated;
}

/** \brief
 *  Chunk merge method based on a* algorithm. WIP.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 *\param first_nonperma_chunk_index int index of the first chunk the entering entries are in
 * \param perma_chunk_count int         number of permaloaded chunks (so perma chunks and entries arent involved in state eval etc)
 * \return void
 */
void build_state_search_solve(ENTRY *elist, int entry_count, int first_nonperma_chunk, int perma_chunk_count) {

    int key_length, ll_count;
    ENTRY *temp_elist;
    LIST *load_lists = build_state_search_eval_util(elist, entry_count, first_nonperma_chunk, &temp_elist, &key_length, &ll_count);

    STATE_SEARCH_STR* init_state = build_state_search_init_state_convert(elist, entry_count, first_nonperma_chunk, key_length);

    HASH_TABLE* table = hash_init_table(hash_func, key_length);
    STATE_SEARCH_HEAP* heap = heap_init_heap();

    hash_add(table, init_state->entry_chunk_array);
    heap_add(heap, init_state);

    STATE_SEARCH_STR* top;
    STATE_SEARCH_STR* best = build_state_search_str_init(key_length);

    int max_payload_limit = 21;
    printf("Max payload limit? [usually 21 or 20]\n");
    scanf("%d", &max_payload_limit);
    printf("\n");

    long long int iter_limit = 0;
    int iter_print = 1000;
    printf("Iteration limit? [try 10000 if you're not sure] [0 removes the limit]\n");
    scanf("%I64d", &iter_limit);
    printf("\n");

    int temp_max, temp_eval, min = 0x7FFFFFFF;
    printf("Printing status line every time new best is reached, or every %d iterations\n", iter_print);
    for (long long int iter = 0; (!heap_is_empty(*heap)); iter++) {

        if ((iter_limit != 0) && (iter > iter_limit))
                break;
        // qsort(heap->heap_array, heap->length, sizeof(STATE_SEARCH_STR*), cmp_state_search_a);
        top = heap_pop(heap);
        int new_best = 0;
        temp_eval =
            build_state_search_eval_state(load_lists, ll_count, top, key_length, first_nonperma_chunk, perma_chunk_count, max_payload_limit, &temp_max);
        if (temp_eval < min) {
            for (int i = 0; i < key_length; i++)
                best->entry_chunk_array[i] = top->entry_chunk_array[i];
            min = temp_eval;
            new_best = 1;
        }
        if (new_best || (iter + 1) % iter_print == 0) {
            char temp[100] = "";
            if (iter_limit != 0)
                sprintf(temp, "/%I64d", iter_limit);
            printf("Iter %I64d%s; Top: %p %5d, max: %3d, heap length: %d\n", iter+1, temp, top, temp_eval, temp_max, heap->length);
        }

        int end_index = build_state_search_str_chunk_max(top, key_length);
        for (unsigned int i = first_nonperma_chunk; i < (unsigned) end_index; i++)
        {
            // dont merge into empty chunks
            if (build_state_search_is_empty_chunk(top, i, key_length))
                continue;

            for (unsigned int j = first_nonperma_chunk; j < i; j++)
            {
                // theres no reason to try to merge if the second chunk (the one that gets merged into the first one) is empty
                if (build_state_search_is_empty_chunk(top, j, key_length))
                    continue;

                STATE_SEARCH_STR* new_state = build_state_search_merge_chunks(top, j, i, key_length, first_nonperma_chunk, temp_elist);
                // merge would result in an invalid state
                if (new_state == NULL)
                    continue;

                // has been considered already
                if (hash_find(table, new_state->entry_chunk_array) != NULL) {
                    build_state_search_str_destroy(new_state);
                    continue;
                }

                new_state->elapsed = 0; //top->elapsed + ELAPSED_INCREMENT;
                new_state->estimated =
                    build_state_search_eval_state(load_lists, ll_count, new_state, key_length, first_nonperma_chunk, perma_chunk_count, max_payload_limit, NULL);

                // attempt to make it aggressive and to cut down on the state count
                if (new_state->estimated == top->estimated) {
                    build_state_search_str_destroy(new_state);
                    continue;
                }

                 // remember as considered
                hash_add(table, new_state->entry_chunk_array);

                // add to state queue
                heap_add(heap, new_state);

                if (new_state->estimated == STATE_SEARCH_EVAL_SUCCESS) {
                    for (int i = 0; i < key_length; i++)
                        elist[build_get_index(temp_elist[i].eid, elist, entry_count)].chunk = new_state->entry_chunk_array[i];

                    build_state_search_solve_cleanup(heap, table, load_lists, temp_elist, best);
                    printf("Solved\n");
                    return;
                }
            }
        }
        build_state_search_str_destroy(top);
    }
    for (int i = 0; i < key_length; i++)
        elist[build_get_index(temp_elist[i].eid, elist, entry_count)].chunk = best->entry_chunk_array[i];

    build_state_search_solve_cleanup(heap, table, load_lists, temp_elist, best);
    printf("Ran out of states or iteration limit reached\n");
    return;
}
