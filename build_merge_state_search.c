#include "macros.h"
#include "build_merge_state_search.h"


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

    clock_t time_start = clock();

    // merge permaloaded entries' chunks as well as possible
    int perma_chunk_count = build_permaloaded_merge(elist, entry_count, chunk_border_sounds, chunk_count, permaloaded);
    int permaloaded_chunk_end_index = *chunk_count;

    // chunks start off partially merged with very related entries being placed together to lower the state count in a_star_solve
    build_assign_primary_chunks_all(elist, entry_count, chunk_count);
    //build_matrix_merge_relative_util(elist, entry_count, chunk_border_sounds, chunk_count, config, permaloaded, 0.05);

    build_state_search_solve(elist, entry_count, permaloaded_chunk_end_index, chunk_count, perma_chunk_count);

    deprecate_build_payload_merge(elist, entry_count, chunk_border_sounds, chunk_count);
    build_dumb_merge(elist, chunk_border_sounds, chunk_count, entry_count);

    clock_t time_end = clock();
    printf("Merge took %.3fs\n", ((double) time_end - time_start) / CLOCKS_PER_SEC);
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


unsigned int build_state_search_eval_state(LIST* stored_load_lists, int total_cam_count, STATE_SEARCH_STR* state, int key_length,
                          ENTRY* temp_elist, int first_nonperma_chunk, int perma_count, int* max_pay) {

    for (int i = 0; i < key_length; i++)
        temp_elist[i].chunk = state->entry_chunk_array[i];

    for (int curr_chunk = 0; curr_chunk <= build_state_search_str_chunk_max(state, key_length); curr_chunk++) {
        int curr_chunk_size = 0x14;
        for (int j = 0; j < key_length; j++)
            if (state->entry_chunk_array[j] == curr_chunk)
                curr_chunk_size += 4 + temp_elist[j].esize;
        if (curr_chunk_size > CHUNKSIZE)
            return STATE_SEARCH_EVAL_INVALID;
    }

    unsigned int eval = 0;
    int maxp = 0;

    int chunk_count = build_state_search_str_chunk_max(state, key_length) + 1;
    int *chunks = (int*) malloc(chunk_count * sizeof(int));

    for (int i = 0; i < total_cam_count; i++)
    {
        for (int j = 0; j < chunk_count; j++)
            chunks[j] = 0;

        for (int j = 0; j < stored_load_lists[i].count; j++){
            int index = stored_load_lists[i].eids[j];
            //int index = build_elist_get_index(eid, temp_elist, key_length);
            int chunk = temp_elist[index].chunk;

            chunks[chunk] = 1;
        }

        int counter = 0;
        for (int j = first_nonperma_chunk; j < chunk_count; j++)
            if (chunks[j] == 1)
                counter++;

        counter += perma_count;
        eval += counter;
        maxp = max(maxp, counter);
    }

    free(chunks);

    if (max_pay != NULL)
        *max_pay = maxp;

    if (maxp <= 21)
        return STATE_SEARCH_EVAL_SUCCESS;
    return eval;
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

/** \brief
 *  Creates a new state based on the input state, puts entries from chunk2 into chunk1 (attempts to merge).
 *
 * \param state A_STAR_STR*             input state
 * \param chunk1 unsigned int           index of first chunk
 * \param chunk2 unsigned int           index of second chunk
 * \param key_length int              amount of involved entries
 * \return A_STAR_STR*                  new state
 */
STATE_SEARCH_STR* build_state_search_merge_chunks(STATE_SEARCH_STR* state, unsigned int chunk1, unsigned int chunk2, int key_length, int perma_count) {
    STATE_SEARCH_STR* new_state = build_state_search_str_init(key_length);

    for (int i = 0; i < key_length; i++) {
        // makes sure chunk2 gets merged into chunk1
        unsigned short int curr_chunk = state->entry_chunk_array[i];
        if (curr_chunk == chunk2)
            curr_chunk = chunk1;
        new_state->entry_chunk_array[i] = curr_chunk;
    }

    return new_state;
}


/** \brief
 *  Converts input entry list with initial chunk assignments to a struct used in the a_star for storing states.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param start_chunk_index int         chunk where involved entries start
 * \param key_length int              amount of involved entries
 * \return A_STAR_STR*                  struct with entry-chunk assignments recorded
 */
STATE_SEARCH_STR* build_state_search_init_state_convert(ENTRY* elist, int entry_count, int start_chunk_index, int key_length) {
    STATE_SEARCH_STR* init_state = build_state_search_str_init(key_length);
    int indexer = 0;
    for (int i = 0; i < entry_count; i++)
        if (elist[i].chunk >= start_chunk_index) {
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


unsigned int* build_state_search_init_elist_convert(ENTRY *elist, int entry_count, int start_chunk_index, int *key_length) {

    int counter = 0;
    for (int i = 0; i < entry_count; i++)
        if (elist[i].chunk >= start_chunk_index)
            counter++;

    unsigned int* EID_list =(unsigned int*) malloc(counter * sizeof(unsigned int));     // freed by caller

    counter = 0;
    for (int i = 0; i < entry_count; i++)
        if (elist[i].chunk >= start_chunk_index)
            EID_list[counter++] = elist[i].EID;

    *key_length = counter;
    return EID_list;
}


void build_state_search_eval_util(ENTRY *elist, int entry_count, ENTRY *temp_elist, unsigned int *EID_list, int key_length, LIST* stored_load_lists) {
    for (int i = 0; i < key_length; i++) {
        temp_elist[i].EID = EID_list[i];
        int master_elist_index = build_elist_get_index(EID_list[i], elist, entry_count);
        temp_elist[i].data = elist[master_elist_index].data;
        temp_elist[i].esize = elist[master_elist_index].esize;
    }

    int cam_path_counter = 0;
    int i, j, k, l, m;
    for (i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].data != NULL)
        {
            int cam_count = build_get_cam_item_count(elist[i].data) / 3;
            for (j = 0; j < cam_count; j++)
            {
                int cam_offset = from_u32(elist[i].data + 0x18 + 0xC * j);
                LOAD_LIST load_list = init_load_list();
                for (k = 0; (unsigned) k < from_u32(elist[i].data + cam_offset + 0xC); k++)
                {
                    int code = from_u16(elist[i].data + cam_offset + 0x10 + 8 * k);
                    int offset = from_u16(elist[i].data + cam_offset + 0x12 + 8 * k) + OFFSET + cam_offset;
                    int list_count = from_u16(elist[i].data + cam_offset + 0x16 + 8 * k);
                    if (code == ENTITY_PROP_CAM_LOAD_LIST_A || code == ENTITY_PROP_CAM_LOAD_LIST_B)
                    {
                        int sub_list_offset = offset + 4 * list_count;
                        int point;
                        int load_list_item_count = 0;
                        for (l = 0; l < list_count; l++, sub_list_offset += load_list_item_count * 4) {
                            load_list_item_count = from_u16(elist[i].data + offset + l * 2);
                            point = from_u16(elist[i].data + offset + l * 2 + list_count * 2);

                            load_list.array[load_list.count].list_length = load_list_item_count;
                            load_list.array[load_list.count].list = (unsigned int *)
                                        malloc(load_list_item_count * sizeof(unsigned int));        // freed here
                            memcpy(load_list.array[load_list.count].list, elist[i].data + sub_list_offset, load_list_item_count * sizeof(unsigned int));
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

                LIST list = init_list();
                for (l = 0; l < load_list.count; l++) {
                        if (load_list.array[l].type == 'A')
                            for (m = 0; m < load_list.array[l].list_length; m++) {
                                int index = build_elist_get_index(load_list.array[l].list[m], temp_elist, key_length);
                                if (index == -1)
                                    continue;
                                //unsigned int eid = load_list.array[l].list[m];
                                list_add(&list, index);
                            }

                        /*if (load_list.array[l].type == 'B')
                            for (m = 0; m < load_list.array[l].list_length; m++)
                                list_remove(&list, load_list.array[l].list[m]);*/
                    }

                stored_load_lists[cam_path_counter] = list;
                cam_path_counter++;
                delete_load_list(load_list);
            }
        }

}


void build_state_search_solve_cleanup(STATE_SEARCH_HEAP *heap, HASH_TABLE *table, LIST *stored_load_lists, ENTRY *temp_elist) {
    heap_destroy(heap);
    hash_destroy_table(table);
    free(stored_load_lists);
    free(temp_elist);
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
 * \param start_chunk_index int         index of the first chunk the entering entries are in
 * \param chunk_count int*              unused
 * \param key_length int                amount of entries entering the process (non-permaloaded normal chunk entries)
 * \return A_STAR_STR*                  struct with good enough solution or NULL
 */
STATE_SEARCH_STR* build_state_search_solve(ENTRY *elist, int entry_count, int start_chunk_index, int *chunk_count, int perma_chunk_count) {

    int key_length;
    unsigned int* EID_list = build_state_search_init_elist_convert(elist, entry_count, start_chunk_index, &key_length);
    STATE_SEARCH_STR* init_state = build_state_search_init_state_convert(elist, entry_count, start_chunk_index, key_length);

    HASH_TABLE* table = hash_init_table(hash_func_chek, key_length);
    hash_add(table, init_state->entry_chunk_array);

    STATE_SEARCH_HEAP* heap = heap_init_heap();
    heap_add(heap, init_state);


    int total_cam_path_count = 0;
    for (int i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE) {
            int cam_count = build_get_cam_item_count(elist[i].data) / 3;
            total_cam_path_count += cam_count;
        }

    LIST *stored_load_lists = (LIST *) malloc(total_cam_path_count * sizeof(LIST));     // freed here
    ENTRY *temp_elist = (ENTRY *) malloc(key_length * sizeof(ENTRY));                   // freed here
    build_state_search_eval_util(elist, entry_count, temp_elist, EID_list, key_length, stored_load_lists);


    STATE_SEARCH_STR* top;
    while(!heap_is_empty(*heap)) {

        qsort(heap->heap_array, heap->length, sizeof(STATE_SEARCH_STR*), cmp_state_search_a);
        top = heap_pop(heap);
        int temp;
        build_state_search_eval_state(stored_load_lists, total_cam_path_count, top, key_length, temp_elist, start_chunk_index, perma_chunk_count, &temp);
        printf("Top: %p %d, max: %d\n", top, top->estimated, temp);

        //int end_index = build_a_star_str_chunk_max(top); // dont check last existing chunk, keep it the same
        int end_index = *chunk_count;
        for (unsigned int i = start_chunk_index; i < (unsigned) end_index; i++) {
            // printf("i: %10d\n", i);
            for (unsigned int j = start_chunk_index; j < i; j++) {

                // theres no reason to try to merge if the second chunk (the one that gets merged into the first one) is empty
                if (build_state_search_is_empty_chunk(top, j, key_length))
                    continue;

                STATE_SEARCH_STR* new_state = build_state_search_merge_chunks(top, i, j, key_length, perma_chunk_count);

                // has been considered already
                if (hash_find(table, new_state->entry_chunk_array) != NULL) {
                    build_state_search_str_destroy(new_state);
                    continue;
                }

                hash_add(table, new_state->entry_chunk_array);                    // remember as considered
                new_state->estimated = build_state_search_eval_state(stored_load_lists, total_cam_path_count, new_state, key_length, temp_elist, start_chunk_index, perma_chunk_count, NULL);
                new_state->elapsed = top->elapsed + 3;

                if (new_state->estimated == STATE_SEARCH_EVAL_INVALID) {
                    build_state_search_str_destroy(new_state);
                    continue;
                }

                if (new_state->estimated == STATE_SEARCH_EVAL_SUCCESS) {
                    printf("Solved\n");

                    for (int i = 0; i < key_length; i++) {
                        elist[build_elist_get_index(EID_list[i], elist, entry_count)].chunk = new_state->entry_chunk_array[i];
                        char temp[100] = "";
                        printf("entry %s size %5d: chunk %2d\n", eid_conv(EID_list[i], temp), elist[build_elist_get_index(EID_list[i], elist, entry_count)].esize, new_state->entry_chunk_array[i]);
                    }
                    build_state_search_str_destroy(new_state);
                    build_state_search_solve_cleanup(heap, table, stored_load_lists, temp_elist);
                    free(EID_list);
                    return NULL;
                }

                heap_add(heap, new_state);
            }
        }
        build_state_search_str_destroy(top); //maybe dont? do i need to care about things getting to the same configuration in less merges (relaxing)? i hope not
    }
    printf("A-STAR Ran out of states\n");
    build_state_search_solve_cleanup(heap, table, stored_load_lists, temp_elist);
    free(EID_list);
    return NULL;
}
