#include "macros.h"
#include "build_merge_a_star.h"

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

    build_permaloaded_merge(elist, entry_count, chunk_border_sounds, chunk_count, permaloaded);     // merge permaloaded entries' chunks as well as possible

    int permaloaded_chunk_end_index = *chunk_count;
    int mergee_count = build_assign_primary_chunks_all(elist, entry_count, chunk_count);            // chunks start off having one entry per chunk
    // probably do some initial merge of very related entries to make the set of possible states in a_star smaller
    A_STAR_STRUCT* solution = a_star_solve(elist, entry_count, permaloaded_chunk_end_index, chunk_count, mergee_count);
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


int build_a_star_evaluate(A_STAR_STRUCT* state, ENTRY *elist, int entry_count, unsigned int* EID_list) {
    // TODO evaluation based on payload ladder (max and total or average idk), return SUCCESS, INVALID or some value in between
    // actually probably gonna be fucky as fuck cuz oh god oh lord jesus
    // probably dont use get payload ladder thingy

    // check if sizes of merged chunks are even valid, if not return INVALID

    // get some list of payloads
    // if max payload is < 20

    return A_STAR_EVAL_INVALID;
}


/** \brief
 *  Returns value of the highest used chunk in the state.
 *
 * \param state A_STAR_STR*             input state
 * \return int                          index of the last used chunk
 */
int build_a_star_str_chunk_max(A_STAR_STRUCT* state, int mergee_count) {
    unsigned short int chunk_last = 0;
    for (int i = 0; i < mergee_count; i++)
        chunk_last = max(chunk_last, state->entry_chunk_array[i]);

    return chunk_last;
}

/** \brief
 *  Creates a new state based on the input state, puts entries from chunk2 into chunk1 (attempts to merge).
 *
 * \param state A_STAR_STR*             input state
 * \param chunk1 unsigned int           index of first chunk
 * \param chunk2 unsigned int           index of second chunk
 * \param mergee_count int              amount of involved entries
 * \return A_STAR_STR*                  new state
 */
A_STAR_STRUCT* build_a_star_merge_chunks(A_STAR_STRUCT* state, unsigned int chunk1, unsigned int chunk2, int mergee_count) {
    A_STAR_STRUCT* new_state = build_a_star_str_init(mergee_count);

    for (int i = 0; i < mergee_count; i++) {
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
 * \param mergee_count int              amount of involved entries
 * \return A_STAR_STR*                  struct with entry-chunk assignments recorded
 */
A_STAR_STRUCT* build_a_star_init_state_convert(ENTRY* elist, int entry_count, int start_chunk_index, int mergee_count) {
    A_STAR_STRUCT* init_state = build_a_star_str_init(mergee_count);
    int indexer = 0;
    for (int i = 0; i < entry_count; i++)
        if (elist[i].chunk >= start_chunk_index) {
            init_state->entry_chunk_array[indexer] = elist[i].chunk;
            indexer++;
        }

    return init_state;
}


int build_a_star_is_empty_chunk(A_STAR_STRUCT* state, unsigned int chunk_index, int mergee_count) {

    int has_something = 0;
    for (int k = 0; k < mergee_count; k++) {
        if (state->entry_chunk_array[k] == chunk_index)
            has_something = 1;
    }

    return !has_something;
}

unsigned int* build_a_star_init_elist_convert(ENTRY *elist, int entry_count, int start_chunk_index, int mergee_count) {
    unsigned int* EID_list =(unsigned int*) malloc(mergee_count * sizeof(unsigned int));
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
 * \param mergee_count int              amount of entries entering the process (non-permaloaded normal chunk entries)
 * \return A_STAR_STR*                  struct with good enough solution or NULL
 */
A_STAR_STRUCT* a_star_solve(ENTRY *elist, int entry_count, int start_chunk_index, int *chunk_count, int mergee_count) {

    unsigned int* EID_list = build_a_star_init_elist_convert(elist, entry_count, start_chunk_index, mergee_count);
    A_STAR_STRUCT* init_state = build_a_star_init_state_convert(elist, entry_count, start_chunk_index, mergee_count);
    A_STAR_HEAP* heap = heap_init_heap();
    // TODO remember as considered
    heap_add(heap, init_state);

    A_STAR_STRUCT* top;
    int temp = 0;
    int ITERATION_LIMIT = 1000000;
    while(!heap_is_empty(*heap)) {

        temp++;
        if (temp >= ITERATION_LIMIT) {
            // TODO let user pick whether to break or continue, let user pick at the start whether to ask them every N iterations (or ask them N)
            break;
        }

        top = heap_pop(heap);

        //int end_index = build_a_star_str_chunk_max(top); // dont check last existing chunk, keep it the same
        int end_index = *chunk_count;
        for (unsigned int i = start_chunk_index; i < (unsigned) end_index; i++)
            for (unsigned int j = start_chunk_index; j < i; j++) {

                // theres no reason to try to merge if the second chunk (the one that gets merged into the first one) is empty
                if (build_a_star_is_empty_chunk(top, j, mergee_count))
                    continue;

                A_STAR_STRUCT* new_state = build_a_star_merge_chunks(top, i, j, mergee_count);
                // TODO if has been considered already
                    // continue;

                new_state->elapsed = top->elapsed + 1; // IDFK
                new_state->estimated = build_a_star_evaluate(new_state, elist, entry_count, EID_list);

                if (new_state->estimated == A_STAR_EVAL_INVALID) {
                    // TODO remember as considered
                    continue;
                }
                if (new_state->estimated == A_STAR_EVAL_SUCCESS) {
                    heap_destroy(heap);
                    return new_state;
                }

                // TODO remember as considered
                heap_add(heap, new_state);
            }

        build_a_star_str_destroy(top); //maybe dont? do i need to care about things getting to the same configuration in less merges (relaxing)? i hope not
    }
    heap_destroy(heap);
    return NULL;
}
