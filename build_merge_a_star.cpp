#include "macros.h"
#include "build_merge_a_star.h"



/** \brief
 *  Assigns primary chunks to all entries, merges need them.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param chunk_count int*              chunk count
 * \return int                          number of entries that have been assigned a chunk in this function
 */
int build_assign_primary_chunks_all_premerge(ENTRY *elist, int entry_count, int *chunk_count, int* config) {
    int chunk_counter = 0;
    for (int i = 0; i < entry_count; i++)
        if (build_is_normal_chunk_entry(elist[i]) && elist[i].chunk == -1) {
            elist[i].chunk = (*chunk_count)++;
            chunk_counter++;
        }
    LIST entries = build_get_normal_entry_list(elist, entry_count);
    int **entry_matrix = build_get_occurence_matrix(elist, entry_count, entries, config[2]);
    return chunk_counter;
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
void build_merge_experimental(ENTRY *elist, int entry_count, int chunk_border_sounds, int *chunk_count, int* config, LIST permaloaded) {

    // merge permaloaded entries' chunks as well as possible
    int perma_chunk_count = build_permaloaded_merge(elist, entry_count, chunk_border_sounds, chunk_count, permaloaded);

    int permaloaded_chunk_end_index = *chunk_count;
    int key_length = build_assign_primary_chunks_all_premerge(elist, entry_count, chunk_count, config);            // chunks start off having one entry per chunk
    // TODO probably do some initial merge of very related entries to make the set of possible states in a_star_solve smaller
    A_STAR_STRUCT* solution = a_star_solve(elist, entry_count, permaloaded_chunk_end_index, chunk_count, key_length, perma_chunk_count);
    if (solution == NULL) {
        // handle
    }
    else {
        // merge chunks accordingly
    }
}


/** \brief
 *  Initialises the struct, allocates memory.
 *
 * \param length int                    amount of entry-chunk assignments
 * \return A_STAR_STR*                  initialised state with allocated memory
 */
A_STAR_STRUCT* build_a_star_str_init(int length) {
    A_STAR_STRUCT* temp = (A_STAR_STRUCT*) malloc(sizeof(A_STAR_STRUCT));
    temp->elapsed = 0;
    temp->estimated = 0;
    temp->entry_chunk_array = (unsigned short int*) malloc(length * sizeof(unsigned short int));
    return temp;
}


/** \brief
 *  Frees all memory allocated by the state.
 *
 * \param state A_STAR_STR*             state to destroy
 * \return void
 */
void build_a_star_str_destroy(A_STAR_STRUCT* state) {
    free(state->entry_chunk_array);
    free(state);
}


int build_a_star_evaluate(A_STAR_STRUCT* state, int key_length, ENTRY *elist, int entry_count, unsigned int* EID_list, int perma_count) {

    ENTRY temp_elist[key_length];
    for (int i = 0; i < key_length; i++) {
        temp_elist[i].EID = EID_list[i];
        temp_elist[i].chunk = state->entry_chunk_array[i];
        int master_elist_index = build_get_index(EID_list[i], elist, entry_count);
        temp_elist[i].data = elist[master_elist_index].data;
        temp_elist[i].esize = elist[master_elist_index].esize;
    }

    for (int curr_chunk = 0; curr_chunk < build_a_star_str_chunk_max(state, key_length); curr_chunk++) {
        int curr_chunk_size = 0x14;
        for (int j = 0; j < key_length; j++)
            if (temp_elist[j].chunk == curr_chunk)
                curr_chunk_size += 4 + temp_elist[j].esize;
        if (curr_chunk_size > CHUNKSIZE)
            return A_STAR_EVAL_INVALID;
    }

    int eval = 0;
    int maxp = 0;
    PAYLOADS payloads = deprecate_build_get_payload_ladder(temp_elist, key_length, 0); // TODO optimise, this takes a fuckton of time
    for (int i = 0; i < payloads.count; i++) {
        int load = payloads.arr[i].count;
        eval += load + perma_count;
        maxp = max(maxp, load);
    }
    printf("Max payload: %3d\n", maxp);
    if (maxp <= 20)
        return A_STAR_EVAL_SUCCESS;
    return eval;
}


/** \brief
 *  Returns value of the highest used chunk in the state.
 *
 * \param state A_STAR_STR*             input state
 * \return int                          index of the last used chunk
 */
int build_a_star_str_chunk_max(A_STAR_STRUCT* state, int key_length) {
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
A_STAR_STRUCT* build_a_star_merge_chunks(A_STAR_STRUCT* state, unsigned int chunk1, unsigned int chunk2, int key_length, int perma_count) {
    A_STAR_STRUCT* new_state = build_a_star_str_init(key_length);

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
A_STAR_STRUCT* build_a_star_init_state_convert(ENTRY* elist, int entry_count, int start_chunk_index, int key_length) {
    A_STAR_STRUCT* init_state = build_a_star_str_init(key_length);
    int indexer = 0;
    for (int i = 0; i < entry_count; i++)
        if (elist[i].chunk >= start_chunk_index) {
            init_state->entry_chunk_array[indexer] = elist[i].chunk;
            indexer++;
        }

    return init_state;
}


int build_a_star_is_empty_chunk(A_STAR_STRUCT* state, unsigned int chunk_index, int key_length) {

    int has_something = 0;
    for (int k = 0; k < key_length; k++) {
        if (state->entry_chunk_array[k] == chunk_index)
            has_something = 1;
    }

    return !has_something;
}

unsigned int* build_a_star_init_elist_convert(ENTRY *elist, int entry_count, int start_chunk_index, int key_length) {
    unsigned int* EID_list =(unsigned int*) malloc(key_length * sizeof(unsigned int));
    int indexer = 0;
    for (int i = 0; i < entry_count; i++)
        if (elist[i].chunk >= start_chunk_index)
            EID_list[indexer++] = elist[i].EID;

    return EID_list;
}

/** \brief
 *  Chunk merge method based on a* algorithm. WIP.
 *
 * \param elist ENTRY*                  entry list
 * \param entry_count int               entry count
 * \param start_chunk_index int         index of the first chunk the entering entries are in
 * \param chunk_count int*              unused
 * \param key_length int              amount of entries entering the process (non-permaloaded normal chunk entries)
 * \return A_STAR_STR*                  struct with good enough solution or NULL
 */
A_STAR_STRUCT* a_star_solve(ENTRY *elist, int entry_count, int start_chunk_index, int *chunk_count, int key_length, int perma_chunk_count) {

    unsigned int* EID_list = build_a_star_init_elist_convert(elist, entry_count, start_chunk_index, key_length);
    A_STAR_STRUCT* init_state = build_a_star_init_state_convert(elist, entry_count, start_chunk_index, key_length);

    HASH_TABLE* table = hash_init_table(hash_func_chek, key_length);
    hash_add(table, init_state->entry_chunk_array);

    A_STAR_HEAP* heap = heap_init_heap();
    heap_add(heap, init_state);


    A_STAR_STRUCT* top;
    int temp = 0;
    int ITERATION_LIMIT = 1000000;
    while(!heap_is_empty(*heap)) {

        temp++;
        printf("A-STAR Heap iteration %10d\n", temp);
        if (temp >= ITERATION_LIMIT) {
            // TODO let user pick whether to break or continue, let user pick at the start whether to ask them every N iterations (or ask them N)
            printf("A-STAR Iteration limit reached\n");
            break;
        }

        top = heap_pop(heap);

        //int end_index = build_a_star_str_chunk_max(top); // dont check last existing chunk, keep it the same
        int end_index = *chunk_count;
        for (unsigned int i = start_chunk_index; i < (unsigned) end_index; i++) {
            printf("i: %10d\n", i);
            for (unsigned int j = start_chunk_index; j < i; j++) {

                // theres no reason to try to merge if the second chunk (the one that gets merged into the first one) is empty
                if (build_a_star_is_empty_chunk(top, j, key_length))
                    continue;

                A_STAR_STRUCT* new_state = build_a_star_merge_chunks(top, i, j, key_length, perma_chunk_count);

                // has been considered already
                if (hash_find(table, new_state->entry_chunk_array) != NULL)
                    continue;

                hash_add(table, new_state->entry_chunk_array);                    // remember as considered
                new_state->elapsed = top->elapsed + 0;  // 1; // IDFK
                new_state->estimated = build_a_star_evaluate(new_state, key_length, elist, entry_count, EID_list, perma_chunk_count);

                if (new_state->estimated == A_STAR_EVAL_INVALID) {
                    continue;
                }
                if (new_state->estimated == A_STAR_EVAL_SUCCESS) {
                    heap_destroy(heap);
                    hash_destroy_table(table);
                    return new_state;
                }

                heap_add(heap, new_state);
            }
        }
        build_a_star_str_destroy(top); //maybe dont? do i need to care about things getting to the same configuration in less merges (relaxing)? i hope not
    }
    printf("A-STAR Ran out of states\n");
    heap_destroy(heap);
    hash_destroy_table(table);
    return NULL;
}
