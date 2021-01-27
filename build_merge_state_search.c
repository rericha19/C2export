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
            if (list_find(permaloaded, elist[i].EID) != -1)
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
                    if (model_ref != elist[i].EID)
                        continue;

                    unsigned int anim_eid = elist[j].EID;
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

                if (((entry1 == elist[i].EID) && (list_find(anims, entry2) != -1)) ||
                    ((entry2 == elist[i].EID) && (list_find(anims, entry1) != -1)) )
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
            if (list_find(permaloaded, elist[i].EID) != -1)
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

                if (((entry1 == elist[i].EID) && (list_find(slsts, entry2) != -1)) ||
                    ((entry2 == elist[i].EID) && (list_find(slsts, entry1) != -1)) )
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
    int permaloaded_chunk_end_index = *chunk_count;

    build_assign_primary_chunks_all(elist, entry_count, chunk_count);
    build_state_search_premerge(elist, entry_count, chunk_border_sounds, chunk_count, config, permaloaded);
    build_state_search_solve(elist, entry_count, permaloaded_chunk_end_index, chunk_count, perma_chunk_count, permaloaded);
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
                          ENTRY* temp_elist, int first_nonperma_chunk, int perma_count, int* max_pay) {

    int chunk_count = build_state_search_str_chunk_max(state, key_length) + 1;
    for (int i = 0; i < key_length; i++)
        temp_elist[i].chunk = state->entry_chunk_array[i];

    /*int *chunk_sizes = (int*) malloc(chunk_count * sizeof(int));
    for (int i = 0; i < chunk_count; i++)
        chunk_sizes[i] = 0x14;

    for (int i = 0; i < key_length; i++)
        chunk_sizes[state->entry_chunk_array[i]] += (4 + temp_elist[i].esize);


    for (int i = 0; i < chunk_count; i++)
        if (chunk_sizes[i] > CHUNKSIZE) {
            free(chunk_sizes);
            return STATE_SEARCH_EVAL_INVALID;
        }

    free(chunk_sizes);
    */


    unsigned int eval = 0;
    int maxp = 0;
    int *chunks = (int*) malloc(chunk_count * sizeof(int));

    for (int i = 0; i < load_list_snapshot_count; i++)
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

/** \brief
 *  Creates a new state based on the input state, puts entries from chunk2 into chunk1 (attempts to merge).
 *
 * \param state A_STAR_STR*             input state
 * \param chunk1 unsigned int           index of first chunk
 * \param chunk2 unsigned int           index of second chunk (higher than chunk1)
 * \param key_length int              amount of involved entries
 * \return A_STAR_STR*                  new state
 */
STATE_SEARCH_STR* build_state_search_merge_chunks(STATE_SEARCH_STR* state, unsigned int chunk1, unsigned int chunk2, int key_length, int first_nonperma_chunk, ENTRY* temp_elist) {
    int i;
    STATE_SEARCH_STR* new_state = build_state_search_str_init(key_length);

    int resulting_size = 0x14;
    for (i = 0; i < key_length; i++) {
        // makes sure chunk2 gets merged into chunk1
        unsigned short int curr_chunk = state->entry_chunk_array[i];
        if (curr_chunk == chunk2)
            curr_chunk = chunk1;
        if (curr_chunk == chunk2 || curr_chunk == chunk1)
            resulting_size += (4 + temp_elist[i].esize);
        new_state->entry_chunk_array[i] = curr_chunk;
    }

    if (resulting_size > CHUNKSIZE) {
        build_state_search_str_destroy(new_state);
        return NULL;
    }

    /*// only one 'hole' can be created at a time, so only doing it once will suffice
    int last_chunk = build_state_search_str_chunk_max(new_state, key_length);
    int *chunks = (int*) calloc( (last_chunk + 1), sizeof(int));

    for (i = 0; i < key_length; i++) {
        chunks[new_state->entry_chunk_array[i]] = 1;
    }

    int empty_chunk_index = -1;
    for (i = first_nonperma_chunk + 1; i <= last_chunk; i++) {
        if (!chunks[i]) {
            empty_chunk_index = i;
            break;
        }
    }

    free(chunks);
    if (empty_chunk_index != -1) {
        for (i = 0; i < key_length; i++)
            if (new_state->entry_chunk_array[i] == last_chunk)
                new_state->entry_chunk_array[i] = empty_chunk_index;
    }*/
    /*for (int i = 0; i < key_length; i++)
        if (new_state->entry_chunk_array[i] > chunk2)
            new_state->entry_chunk_array[i] -= 1;*/

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


LIST* build_state_search_eval_util(ENTRY *elist, int entry_count, ENTRY *temp_elist, unsigned int *EID_list, int key_length, LIST permaloaded, int* load_list_snapshot_count) {
    for (int i = 0; i < key_length; i++) {
        temp_elist[i].EID = EID_list[i];
        int master_elist_index = build_get_index(EID_list[i], elist, entry_count);
        temp_elist[i].data = elist[master_elist_index].data;
        temp_elist[i].esize = elist[master_elist_index].esize;
    }

    LIST *stored_load_lists = NULL;

    int snapshot_counter = 0;
    int i, j, k, l, m;
    for (i = 0; i < entry_count; i++)
        if (build_entry_type(elist[i]) == ENTRY_TYPE_ZONE && elist[i].data != NULL)
        {
            int cam_count = build_get_cam_item_count(elist[i].data) / 3;
            for (j = 0; j < cam_count; j++)
            {
                // part that gets full load lists
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

                // part that stores 'snapshots' used by eval_state
                LIST list = init_list();
                for (l = 0; l < load_list.count; l++) {
                        if (load_list.array[l].type == 'A')
                            for (m = 0; m < load_list.array[l].list_length; m++) {
                                unsigned int eid = load_list.array[l].list[m];
                                int index = build_get_index(eid, temp_elist, key_length);
                                if (index == -1)
                                    continue;

                                list_add(&list, index);
                            }

                        if (load_list.array[l].type == 'B')
                            for (m = 0; m < load_list.array[l].list_length; m++) {
                                unsigned int eid = load_list.array[l].list[m];
                                int index = build_get_index(eid, temp_elist, key_length);
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

    *load_list_snapshot_count = snapshot_counter;
    return stored_load_lists;
}


void build_state_search_solve_cleanup(STATE_SEARCH_HEAP *heap, HASH_TABLE *table, LIST *stored_load_lists, ENTRY *temp_elist, unsigned int* EID_list) {
    heap_destroy(heap);
    hash_destroy_table(table);
    free(stored_load_lists);
    free(temp_elist);
    free(EID_list);
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
 * \return void
 */
void build_state_search_solve(ENTRY *elist, int entry_count, int start_chunk_index, int *chunk_count, int perma_chunk_count, LIST permaloaded) {

    int key_length;
    unsigned int* EID_list = build_state_search_init_elist_convert(elist, entry_count, start_chunk_index, &key_length);
    STATE_SEARCH_STR* init_state = build_state_search_init_state_convert(elist, entry_count, start_chunk_index, key_length);

    HASH_TABLE* table = hash_init_table(hash_func, key_length);
    hash_add(table, init_state->entry_chunk_array);

    STATE_SEARCH_HEAP* heap = heap_init_heap();
    heap_add(heap, init_state);

    int load_list_snapshot_count;
    ENTRY *temp_elist = (ENTRY *) malloc(key_length * sizeof(ENTRY));                   // freed here
    LIST *stored_load_lists = build_state_search_eval_util(elist, entry_count, temp_elist, EID_list, key_length, permaloaded, &load_list_snapshot_count);


    STATE_SEARCH_STR* top;
    while(!heap_is_empty(*heap)) {

        // qsort(heap->heap_array, heap->length, sizeof(STATE_SEARCH_STR*), cmp_state_search_a);
        top = heap_pop(heap);
        int temp;
        build_state_search_eval_state(stored_load_lists, load_list_snapshot_count, top, key_length, temp_elist, start_chunk_index, perma_chunk_count, &temp);
        printf("Top: %p %d, max: %d\n", top, top->estimated, temp);

        int end_index = build_state_search_str_chunk_max(top, key_length);
        //int end_index = *chunk_count;
        for (unsigned int i = start_chunk_index; i < (unsigned) end_index; i++) {

            if (build_state_search_is_empty_chunk(top, i, key_length))
                    continue;

            for (unsigned int j = start_chunk_index; j < i; j++) {

                // theres no reason to try to merge if the second chunk (the one that gets merged into the first one) is empty
                if (build_state_search_is_empty_chunk(top, j, key_length))
                    continue;

                STATE_SEARCH_STR* new_state = build_state_search_merge_chunks(top, j, i, key_length, start_chunk_index, temp_elist);
                // merge would result in an invalid state
                if (new_state == NULL)
                    continue;

                // has been considered already
                if (hash_find(table, new_state->entry_chunk_array) != NULL) {
                    build_state_search_str_destroy(new_state);
                    continue;
                }

                new_state->elapsed = top->elapsed + ELAPSED_INCREMENT;
                new_state->estimated =
                    build_state_search_eval_state(stored_load_lists, load_list_snapshot_count, new_state, key_length, temp_elist, start_chunk_index, perma_chunk_count, NULL);

                // shouldnt happen anymore (moved size check to merge_chunks)
                if (new_state->estimated == STATE_SEARCH_EVAL_INVALID) {
                    build_state_search_str_destroy(new_state);
                    continue;
                }

                // remember as considered
                hash_add(table, new_state->entry_chunk_array);

                // add to state queue
                heap_add(heap, new_state);

                if (new_state->estimated == STATE_SEARCH_EVAL_SUCCESS) {
                    for (int i = 0; i < key_length; i++)
                        elist[build_get_index(EID_list[i], elist, entry_count)].chunk = new_state->entry_chunk_array[i];

                    build_state_search_solve_cleanup(heap, table, stored_load_lists, temp_elist, EID_list);
                    printf("Solved\n");
                    return;
                }
            }
        }
        build_state_search_str_destroy(top);
    }
    printf("Ran out of states\n");
    build_state_search_solve_cleanup(heap, table, stored_load_lists, temp_elist, EID_list);
    return;
}
