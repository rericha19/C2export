
#include "macros.h"

void build_matrix_merge_random_main(ENTRY *elist, int entry_count, int chunk_border_sounds, int *chunk_count, int* config, LIST permaloaded) {

    build_permaloaded_merge(elist, entry_count, chunk_border_sounds, chunk_count, permaloaded);                     // merge permaloaded entries' chunks as well as possible

    double mult;
    int iter_count;
    int seed = 0;
    int max_payload_limit;

    printf("\nMax payload limit? (usually 21 or 20)\n");
    scanf("%d", &max_payload_limit);
    printf("\n");

    printf("Max iteration count? (0 for no limit)\n");
    scanf("%d", &iter_count);
    printf("\n");
    if (iter_count < 0)
        iter_count = 100;

    printf("Randomness multiplier? (> 1)\n");
    scanf("%lf", &mult);
    printf("\n");

    printf("Seed? (0 for random)\n");
    scanf("%d", &seed);
    printf("\n");
    if (seed == 0) {
        time_t raw_time;
        time(&raw_time);
        srand(raw_time);
        seed = rand();
        printf("Seed used: %d\n", seed);
    }
    srand(seed);

    ENTRY clone_elist[ELIST_DEFAULT_SIZE];
    ENTRY best_elist[ELIST_DEFAULT_SIZE];
    int best_max = entry_count;
    int tmp_chunk_count;
    unsigned int best_zone = 0;
    char temp[6] = "";
    char temp2[6] = "";

    for (int i = 0; i < entry_count; i++) {
        best_elist[i] = elist[i];
    }

    for (int i = 0; i < iter_count || iter_count == 0; i++) {
        for (int j = 0; j < entry_count; j++)
            clone_elist[j] = elist[j];
        tmp_chunk_count = *chunk_count;

        build_assign_primary_chunks_all(clone_elist, entry_count, &tmp_chunk_count);                                               // chunks start off having one entry per chunk
        build_matrix_merge(clone_elist, entry_count, chunk_border_sounds, &tmp_chunk_count, config, permaloaded, 1.0, mult);       // current best algorithm + some random
        build_dumb_merge(clone_elist, chunk_border_sounds, &tmp_chunk_count, entry_count);                                         // jic something didnt get merged it gets merged

        PAYLOADS payloads = deprecate_build_get_payload_ladder(clone_elist, entry_count, chunk_border_sounds);
        qsort(payloads.arr, payloads.count, sizeof(PAYLOAD), cmp_func_payload);

        int curr = payloads.arr[0].count;
        printf("Iter %3d, curr_max %2d (%5s), best_max %2d (%5s)\n", i, curr, eid_conv(payloads.arr[0].zone, temp), best_max, eid_conv(best_zone, temp2));

        if (curr < best_max) {
            for (int j = 0; j < entry_count; j++)
                best_elist[j] = clone_elist[j];
            best_max = curr;
            best_zone = payloads.arr[0].zone;
        }

        if (best_max <= max_payload_limit)
            break;
    }

    for (int i = 0; i < entry_count; i++) {
        elist[i] = best_elist[i];
        if (elist[i].chunk >= *chunk_count)
            *chunk_count = elist[i].chunk + 1;
    }
}



