#include "macros.h"

void build_merge_experimental(ENTRY *elist, int entry_count, int chunk_border_sounds, int *chunk_count, int* config, LIST permaloaded) {
    build_permaloaded_merge(elist, entry_count, chunk_border_sounds, chunk_count, permaloaded); // merge permaloaded entries' chunks as well as possible
    build_assign_primary_chunks_all(elist, entry_count, chunk_count);                           // chunks start off having one entry per chunk
    A_STAR_STR solution = a_star_solve(elist, entry_count, chunk_count);
}

A_STAR_STR a_star_solve(ENTRY *elist, int entry_count, int *chunk_count) {

}
