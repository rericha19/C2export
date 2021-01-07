#include "macros.h"

void build_merge_experimental(ENTRY *elist, int entry_count, int chunk_border_sounds, int *chunk_count, int* config, LIST permaloaded) {
    build_permaloaded_merge(elist, entry_count, chunk_border_sounds, chunk_count, permaloaded);     // merge permaloaded entries' chunks as well as possible

    int permaloaded_chunk_end_index = *chunk_count;
    int mergee_count = build_assign_primary_chunks_all(elist, entry_count, chunk_count);            // chunks start off having one entry per chunk
    // probably do some initial merge of very related entries to make the set of possible states in a_star smaller
    A_STAR_STR* solution = a_star_solve(elist, entry_count, permaloaded_chunk_end_index, chunk_count, mergee_count);
}

A_STAR_STR a_star_str_init(int length) {
    A_STAR_STR temp;
    temp.elapsed = 0;
    temp.array_length = length;
    temp.entry_chunk_array = (unsigned int**) malloc(length * sizeof(unsigned int*));
    for (int i = 0; i < length; i++)
        temp.entry_chunk_array[i] = (unsigned int*) malloc(2 * sizeof(unsigned int));
    return temp;
}

A_STAR_QUEUE::A_STAR_QUEUE(int len) {
    length = len;
    que = NULL;
}

void A_STAR_QUEUE::add(A_STAR_STR item) {

}

A_STAR_STR A_STAR_QUEUE::pop(){

}


A_STAR_STR* a_star_solve(ENTRY *elist, int entry_count, int start_chunk_index, int *chunk_count, int mergee_count) {
    static A_STAR_STR init_state = a_star_str_init(mergee_count);
    A_STAR_QUEUE state_queue(0);

    return &init_state;;
}
