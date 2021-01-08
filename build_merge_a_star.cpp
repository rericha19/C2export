#include "macros.h"
#include "build_merge_a_star_heap.h"

void build_merge_experimental(ENTRY *elist, int entry_count, int chunk_border_sounds, int *chunk_count, int* config, LIST permaloaded) {
    build_permaloaded_merge(elist, entry_count, chunk_border_sounds, chunk_count, permaloaded);     // merge permaloaded entries' chunks as well as possible

    int permaloaded_chunk_end_index = *chunk_count;
    int mergee_count = build_assign_primary_chunks_all(elist, entry_count, chunk_count);            // chunks start off having one entry per chunk
    // probably do some initial merge of very related entries to make the set of possible states in a_star smaller
    A_STAR_STR* solution = a_star_solve(elist, entry_count, permaloaded_chunk_end_index, chunk_count, mergee_count);
    if (solution == NULL) {
        // handle
    }
    else {
        // merge chunks accordingly
    }
}


A_STAR_STR* a_star_str_init(int length) {
    A_STAR_STR* temp = (A_STAR_STR*) malloc(sizeof(A_STAR_STR));
    temp->elapsed = 0;
    temp->estimated = 0;
    temp->array_length = length;
    temp->entry_chunk_array = (unsigned int**) malloc(length * sizeof(unsigned int*));
    for (int i = 0; i < length; i++)
        temp->entry_chunk_array[i] = (unsigned int*) malloc(2 * sizeof(unsigned int));
    return temp;
}


int build_a_star_evaluate(A_STAR_STR* state) {
    // TODO
    // use get payload ladder thingy
    return A_STAR_EVAL_INVALID;
}


int build_a_star_str_chunk_max(A_STAR_STR* state) {
    // TODO
    return 0;
}

A_STAR_STR* build_a_star_merge_chunks(A_STAR_STR* state, int chunk1, int chunk2, int mergee_count) {
    A_STAR_STR* new_state = a_star_str_init(mergee_count);
    // TODO

    return new_state;
}

A_STAR_STR* build_a_star_init_state_convert(ENTRY* elist, int entry_count, int start_chunk_index, int mergee_count) {
    A_STAR_STR* init_state = a_star_str_init(mergee_count);
    // TODO

    return init_state;
}

A_STAR_STR* a_star_solve(ENTRY *elist, int entry_count, int start_chunk_index, int *chunk_count, int mergee_count) {
    A_STAR_STR* init_state = build_a_star_init_state_convert(elist, entry_count, start_chunk_index, mergee_count);
    A_STAR_HEAP* heap = heap_init_heap();
    // TODO remember as considered
    heap_add(heap, init_state);

    A_STAR_STR* top;
    while(!heap_is_empty(*heap)) {
        top = heap_pop(heap);

        int end_index = build_a_star_str_chunk_max(top);
        for (int i = start_chunk_index; i < end_index; i++)
            for (int j = start_chunk_index; j < i; j++) {
                A_STAR_STR* new_state = build_a_star_merge_chunks(top, i, j, mergee_count);
                // TODO if has been considered already
                    // continue;

                new_state->elapsed = top->elapsed + 1; // IDFK
                new_state->estimated = build_a_star_evaluate(new_state);

                if (new_state->estimated == A_STAR_EVAL_INVALID) {
                    // TODO remember as considered
                    continue;
                }
                if (new_state->estimated == A_STAR_EVAL_SUCCESS) {
                    // TODO free all the garbage (heap's contents, heap)
                    return new_state;
                }

                // TODO remember as considered
                heap_add(heap, new_state);
            }
        free(top);
    }

    // free all the garbage (heap's contents, heap)
    return NULL;
}
